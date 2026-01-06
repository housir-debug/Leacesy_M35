#include "tcpserver.h"
#include "canworker.h"
#include <QNetworkInterface>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QRandomGenerator>
#include <QHostInfo>
#include <QDataStream>
#include <QtEndian>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>


// ===================== 初始化部分 =================================

Q_LOGGING_CATEGORY(tcp, "tcp:")
const QString TcpServerManager::SCPI_QUERY_SYMBOL = "?";

TcpServerManager::TcpServerManager(QObject *parent)
    : QObject(parent)
{
    m_scpiManager = new ScpiManager(this);
    bool initSuccess = m_scpiManager->init(
        getManufacturer,
        getModel,
        getSerialNumber,
        getFirmwareVersion
    );

    if (initSuccess) {
        qCInfo(tcp) << "SCPI管理器初始化成功";
    } else {
        qCCritical(tcp) << "SCPI管理器初始化失败";
    }

    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress &address :qAsConst(addresses)) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol &&
            address != QHostAddress::LocalHost) {
            m_deviceIp = address.toString();
            m_deviceAddress = address;
            break;
        }
    }
    if (m_deviceIp.isEmpty()) {
        m_deviceIp = "127.0.0.1";
    }

    m_linkCleanupTimer = new QTimer(this);
    m_linkCleanupTimer->setInterval(60000); // 1分钟清理一次过期链接
    connect(m_linkCleanupTimer, &QTimer::timeout, this, [this]() {
        QMutexLocker locker(&m_linkMutex);
        QDateTime now = QDateTime::currentDateTime();
        QList<quint32> toRemove;

        for (auto it = m_deviceLinks.begin(); it != m_deviceLinks.end(); ++it) {
            // 清理创建超过1小时的链接
            if (it->createTime.secsTo(now) > 3600) {
                toRemove.append(it.key());
            }
        }

        for (quint32 linkId : toRemove) {
            qCDebug(tcp) << "清理过期VXI-11链接:" << linkId;
            m_deviceLinks.remove(linkId);
        }
    });

    qCInfo(tcp) << "Device IP:" << m_deviceIp;
}

static quint16 crc16(const quint8 *data, int length)
{
    quint16 crc = 0xFFFF;

    for (int i = 0; i < length; i++) {
        crc ^= (quint16)data[i] << 8;

        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

void TcpServerManager::forwardCanData(quint32 canId, const QByteArray &data, qint64 timestamp)
{
    if (m_state.load() != STATE_RUNNING) {return;}

    CanDataPacket packet;
    memset(&packet, 0, sizeof(packet));

    packet.magic = 0xCAFE;
    packet.length = sizeof(packet) - sizeof(packet.magic) - sizeof(packet.length);
    packet.timestamp = timestamp;

    packet.canId = canId;
    int dataSize = qMin(data.size(), 8);
    packet.data[0] = static_cast<quint8>(dataSize);
    memcpy(packet.data + 1, data.constData(), dataSize);

    QByteArray crcData(reinterpret_cast<const char*>(&packet.timestamp),
                      sizeof(packet) - offsetof(CanDataPacket, timestamp) - sizeof(packet.crc));
    packet.crc = crc16(reinterpret_cast<const quint8*>(crcData.constData()), crcData.size());

    QByteArray packetData(reinterpret_cast<const char*>(&packet), sizeof(packet));
    sendToAllClients(packetData);
}

void TcpServerManager::forwardSerialData(const QByteArray &data){
    sendToAllClients(data);
}

// ===================== 启动部分 =================================

bool TcpServerManager::startServer()
{
    if (m_state.load() != STATE_STOPPED) {
        qCWarning(tcp) << "TcpServer is not in stopped state";
        return false;
    }

    m_state.store(STATE_STARTING);

    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setInterval(30000); // 30秒心跳
    m_cleanupTimer = new QTimer(this);
    m_cleanupTimer->setInterval(5000); // 5秒清理一次
    m_tcpServer = new QTcpServer(this);

    if (!m_serverThread){
        m_serverThread = new QThread(this);
        m_serverThread->setObjectName("TcpServer");
    }
    if (thread() != m_serverThread) {
        this->moveToThread(m_serverThread);
        m_tcpServer->moveToThread(m_serverThread);
        m_heartbeatTimer->moveToThread(m_serverThread);
        m_cleanupTimer->moveToThread(m_serverThread);
    }
    if (!m_serverThread->isRunning()) {
        m_serverThread->start();
    }

    connect(m_heartbeatTimer, &QTimer::timeout, this, [this]() {
        QByteArray heartbeat;
        heartbeat.append("HEARTBEAT");
        sendToAllClients(heartbeat);
    });
    connect(m_cleanupTimer, &QTimer::timeout,
            this, &TcpServerManager::cleanupDisconnectedClients);
    connect(m_tcpServer, &QTcpServer::newConnection,
            this, &TcpServerManager::onNewConnection);

    QMetaObject::invokeMethod(this, [this]() {
        if (m_tcpServer->listen(QHostAddress::Any, m_port)) {
            m_state.store(STATE_RUNNING);

            m_heartbeatTimer->start();
            m_cleanupTimer->start();

            qCInfo(tcp) << "TcpServer started on port" << m_port;
        } else {
            m_state.store(STATE_STOPPED);
            QString error = QString("Failed to start TCP server: %1")
                           .arg(m_tcpServer->errorString());
            qCCritical(tcp) << error;
            emit errorOccurred(error);
            stopServer();
        }
    }, Qt::QueuedConnection);

    QTimer::singleShot(1000, this, &TcpServerManager::registerWithRpcbind);
    return true;
}

void TcpServerManager::sendToAllClients(const QByteArray &data)
{
    QMutexLocker locker(&m_Mutex);

    int successCount = 0;
    int failedCount = 0;

    for (QTcpSocket *client : qAsConst(m_clients)) {
        if (client->state() == QAbstractSocket::ConnectedState) {
            qint64 bytesWritten = client->write(data);
            if (bytesWritten == -1) {
                qCWarning(tcp) << "Failed to send data to client" << client->objectName();
                failedCount++;
            } else {
                successCount++;
                m_totalBytesSent += bytesWritten;
                //client->flush();
            }
        } else {
            failedCount++;
        }
    }

    if (successCount == m_clients.size()) {
        if (data != "HEARTBEAT"){
            qint64 elapsed = m_testtimer.elapsed();
            qCDebug(tcp) << "eth-can test time:" <<elapsed;
        }
    }else{
        qCDebug(tcp) << "tcpsendclient failcount:" <<failedCount<< "clients,"<< successCount << "clients";
    }
}

void TcpServerManager::cleanupDisconnectedClients()
{
    QList<QTcpSocket*> toRemove;

    for (QTcpSocket *client : qAsConst(m_clients)) {
        if (client->state() != QAbstractSocket::ConnectedState) {
            toRemove.append(client);
        }
    }

    for (QTcpSocket *client : toRemove) {
        m_clients.removeOne(client);
        client->deleteLater();
        qCDebug(tcp) << "Cleaned up disconnected client:" << client->objectName();
    }
}

void TcpServerManager::onNewConnection()
{
    QTcpSocket *client = m_tcpServer->nextPendingConnection();
    if (!client) {return;}

    {
        QMutexLocker locker(&m_Mutex);
        if (m_clients.size() >= 10) {
            qCWarning(tcp) << "Max client limit reached, rejecting connection from"
                      << client->peerAddress().toString();
            client->disconnectFromHost();
            delete client;
            return;
        }

        m_clients.append(client);
    }

    QString clientInfo = QString("%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort());

    client->setObjectName(clientInfo);

    connect(client, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &TcpServerManager::onSocketError);
    connect(client, &QTcpSocket::disconnected,
            this, &TcpServerManager::onClientDisconnected);
    connect(client, &QTcpSocket::readyRead,
            this, &TcpServerManager::onClientReadyRead);

    qCInfo(tcp) << "New client connected:" << clientInfo<< ", total clients:" << m_clients.size();;
}

bool TcpServerManager::registerWithRpcbind()
{
    if (TirpcDynamicLoader::instance().load()) {
        qCDebug(tcp) << "使用 libtirpc 注册 VXI-11 服务";

        bool result = TirpcDynamicLoader::instance().pmap_set(   //pmap_unset
            Vxi11::DEVICE_CORE,      // VXI-11程序号
            Vxi11::DEVICE_CORE_VERSION, // 版本
            IPPROTO_TCP,             // TCP协议
            m_port                   // 端口
        );

        if (result) {
            qCInfo(tcp) << "✅ 使用 libtirpc 成功注册 VXI-11 服务到端口" << m_port;
            return true;
        } else {
            qCWarning(tcp) << "❌ 使用 libtirpc 注册失败，尝试手动注册";
        }
    }

    qCDebug(tcp) << "使用手动RPC注册 VXI-11 服务";

    // 创建RPC SET请求：注册VXI-11服务
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        qCWarning(tcp) << "创建socket失败:" << strerror(errno);
        return false;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(111); // rpcbind端口
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    // RPC CALL头部
    quint32 xid = QRandomGenerator::global()->generate();

    #pragma pack(push, 1)
    struct RpcCall {
        quint32 xid;          // 事务ID
        quint32 msg_type;     // CALL=0
        quint32 rpcvers;      // RPC版本2
        quint32 prog;         // PORTMAP程序号=100000
        quint32 vers;         // PORTMAP版本=2
        quint32 proc;         // SET=1
        quint32 cred_flavor;  // AUTH_NULL=0
        quint32 cred_length;  // 0
        quint32 verf_flavor;  // AUTH_NULL=0
        quint32 verf_length;  // 0
        quint32 map_prog;     // VXI-11程序号=395183
        quint32 map_vers;     // VXI-11版本=1
        quint32 map_prot;     // TCP协议=6
        quint32 map_port;     // 我们的端口
    };
    #pragma pack(pop)

    RpcCall call;
    memset(&call, 0, sizeof(call));

    // 填充字段（转换为网络字节序）
    call.xid = htonl(xid);
    call.msg_type = htonl(0);        // CALL
    call.rpcvers = htonl(2);         // RPC版本2
    call.prog = htonl(100000);       // PORTMAP
    call.vers = htonl(2);            // PORTMAP版本2
    call.proc = htonl(1);            // SET过程
    call.map_prog = htonl(395183);   // VXI-11程序号
    call.map_vers = htonl(1);        // VXI-11版本1
    call.map_prot = htonl(6);        // TCP协议
    call.map_port = htonl(m_port);   // 我们的端口

    // 发送请求
    sendto(sockfd, &call, sizeof(call), 0,
           (struct sockaddr*)&addr, sizeof(addr));

    close(sockfd);
    qCDebug(tcp) << "手动注册 VXI-11 服务到端口" << m_port;
    // 由于是UDP发送，不等待响应
    return true;
}

// ===================== 信息处理部分 =================================

void TcpServerManager::onSocketError(QAbstractSocket::SocketError error)
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) {
        return;
    }

    qCWarning(tcp) << "Socket error from" << client->objectName()
              << ":" << client->errorString() << "|" << error;
}

void TcpServerManager::onClientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) {
        return;
    }

    QString clientInfo = client->objectName();

    {
        QMutexLocker locker(&m_Mutex);
        m_clients.removeOne(client);
    }

    client->deleteLater();

    qCInfo(tcp) << "Client disconnected:" << clientInfo
           << ", remaining clients:" << m_clients.size();
}

void TcpServerManager::onClientReadyRead()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) {return;}

    QByteArray rawData = client->readAll();
    m_totalBytesReceived += rawData.size();
    qCDebug(tcp) << "Received from" << client->objectName()<< "size:" << rawData.size()<< "hex:" << rawData.toHex();

    QTimer::singleShot(0, this, [this, client,rawData]() {
        // 异步处理，立即放入事件队列，不阻塞事件循环
        processClientData(client,rawData);
    });
}

void TcpServerManager::processClientData(QTcpSocket *client,const QByteArray newdata)
{
    if (m_vxi11Enabled && isVxi11RpcCall(newdata)) {
        qCDebug(tcp) << "VXI-11 RPC call detected";
        handleVxi11RpcCall(client, newdata);
        return;
    }

    QString message = QString::fromUtf8(newdata).trimmed();
    if (message.startsWith("*") || message.contains(":")) {
        qCDebug(tcp) << "SCPI command detected:" << message;

        QByteArray response = m_scpiManager->processCommand(newdata);

        if (!response.isEmpty() && client->state() == QAbstractSocket::ConnectedState) {
            if (!response.endsWith("\n")) {
                response.append("\n");
            }

            client->write(response);
            m_totalBytesSent += response.size();

            qCDebug(tcp) << "SCPI响应发送:" << response.trimmed()
                      << "长度:" << response.size() << "bytes";
        } else {
            qCDebug(tcp) << "SCPI响应为空或客户端未连接";
        }
        return;
    }

    if (newdata.trimmed().toUpper() == "HEARTBEAT") {// 文本格式的心跳包
        client->write("HEARTBEAT_RESPONSE\n");
        m_testtimer.start();
        //emit test();
        quint32 testId = 0x321;
        const QByteArray &testData = QByteArray::fromHex("1122334455667788");
        //emit SerialSendRequest(testData);
        emit canSendRequest(testId, testData);
        return;
    }else{
        qCWarning(tcp) << "Invalid brief inform!Throw!" ;
        return;
    }
}

bool TcpServerManager::isVxi11RpcCall(const QByteArray &data)
{
    if (data.size() < 24) {
        return false;
    }

    // 检查是否是VXI-11 RPC调用（程序号395183）
    const uchar* rawData = reinterpret_cast<const uchar*>(data.constData());

    // 程序号在偏移16字节处
    if (data.size() >= 20) {
        quint32 progNum = qFromBigEndian<quint32>(rawData + 16);
        return (progNum == 395183); // VXI-11程序号
    }

    return false;
}

void TcpServerManager::handleVxi11RpcCall(QTcpSocket* client, const QByteArray &data)
{
    if (!client || data.size() < 28) {
        return;
    }

    quint32 procedure = extractProcedure(data);
    quint32 xid = extractXid(data);

    qCDebug(tcp) << "VXI-11 RPC调用, XID:" << xid << "过程:" << procedure;

    switch (procedure) {
        case Vxi11::GETPORT:
                handleGetPort(client, data);
                break;

        case Vxi11::CREATE_LINK:
            handleCreateLink(client, data);
            break;

        case Vxi11::DEVICE_WRITE:
            handleDeviceWrite(client, data);
            break;

        case Vxi11::DEVICE_READ:
            handleDeviceRead(client, data);
            break;

        case Vxi11::DEVICE_DOCMD:
            handleDeviceDocmd(client, data);
            break;

        case Vxi11::DESTROY_LINK:
            handleDestroyLink(client, data);
            break;

        default:
            // 返回程序不可用
            qCWarning(tcp) << "未知的VXI-11过程:" << procedure;
            QByteArray response = buildVxi11Response(xid, procedure, Vxi11::PROC_UNAVAIL);
            client->write(response);
            break;
    }
}

QByteArray TcpServerManager::buildVxi11Response(quint32 xid, quint32 procedure, quint32 result)
{
    QByteArray response;

    // XID（原样返回）
    quint32 beXid = qToBigEndian<quint32>(xid);
    response.append(reinterpret_cast<const char*>(&beXid), 4);

    // 消息类型：REPLY=1
    quint32 beMsgType = qToBigEndian<quint32>(1);
    response.append(reinterpret_cast<const char*>(&beMsgType), 4);

    // 回复状态：接受=0
    quint32 beReplyStat = qToBigEndian<quint32>(0);
    response.append(reinterpret_cast<const char*>(&beReplyStat), 4);

    if (result == 0) {
        // 接受状态：成功=0
        quint32 beAcceptStat = qToBigEndian<quint32>(0);
        response.append(reinterpret_cast<const char*>(&beAcceptStat), 4);

        // 根据过程号返回不同数据
        if (procedure == 3) { // CREATE_LINK
            quint32 beLinkId = qToBigEndian<quint32>(1); // 链路ID
            response.append(reinterpret_cast<const char*>(&beLinkId), 4);
        } else if (procedure == 10 || procedure == 11) {
            // DEVICE_WRITE/READ返回成功状态
            quint32 beResult = qToBigEndian<quint32>(0);
            response.append(reinterpret_cast<const char*>(&beResult), 4);
        }
    } else {
        // 程序不可用
        quint32 beAcceptStat = qToBigEndian<quint32>(1);
        response.append(reinterpret_cast<const char*>(&beAcceptStat), 4);
    }

    return response;
}

// ===================== VXI-11 RPC处理 =================================

quint32 TcpServerManager::extractXid(const QByteArray &data)
{
    if (data.size() < 4) return 0;
    const quint32* ptr = reinterpret_cast<const quint32*>(data.constData());
    return qFromBigEndian<quint32>(ptr);
}

quint32 TcpServerManager::extractProcedure(const QByteArray &data)
{
    if (data.size() < 24) return 0;
    const quint32* ptr = reinterpret_cast<const quint32*>(data.constData() + 20);
    return qFromBigEndian<quint32>(ptr);
}

QByteArray TcpServerManager::createErrorResponse(quint32 xid, quint32 error)
{
    QByteArray response;
    response.reserve(28);

    QDataStream stream(&response, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    // RPC回复头部
    stream << xid;                      // xid
    stream << quint32(1);               // msg_type: REPLY
    stream << quint32(0);               // reply_stat: MSG_ACCEPTED

    // AUTH_NULL验证器
    stream << quint32(0);               // verf_flavor: AUTH_NULL
    stream << quint32(0);               // verf_length: 0

    // 接受状态和错误
    stream << quint32(0);               // accept_stat: SUCCESS
    stream << error;                    // 具体错误码

    return response;
}

TcpServerManager::DeviceLink* TcpServerManager::findLinkByClient(QTcpSocket* client)
{
    QMutexLocker locker(&m_linkMutex);

    if (!client) {
        qCWarning(tcp) << "findLinkByClient: 无效的client指针";
        return nullptr;
    }

    // 遍历所有链接查找匹配的client
    for (auto it = m_deviceLinks.begin(); it != m_deviceLinks.end(); ++it) {
        if (it->client == client) {
            qCDebug(tcp) << "找到链接:" << it->id << "对应的client:" << client->objectName();
            return &(*it); // 返回引用
        }
    }

    qCDebug(tcp) << "未找到client:" << client->objectName() << "对应的链接";
    return nullptr;
}

TcpServerManager::DeviceLink* TcpServerManager::createLink(QTcpSocket* client)
{
    QMutexLocker locker(&m_linkMutex);

    if (!client) {
        qCWarning(tcp) << "createLink: 无效的client指针";
        return nullptr;
    }

    // 检查是否已存在该client的链接
    for (auto it = m_deviceLinks.begin(); it != m_deviceLinks.end(); ++it) {
        if (it->client == client) {
            qCWarning(tcp) << "client:" << client->objectName() << "已存在链接:" << it->id;
            return &(*it);
        }
    }

    // 创建新链接
    DeviceLink link;
    link.id = m_nextLinkId++;
    link.client = client;
    link.createTime = QDateTime::currentDateTime();
    link.lock = 0;  // 初始未锁定
    link.aborted = false;

    // 插入到映射表中
    m_deviceLinks.insert(link.id, link);

    qCInfo(tcp) << "✅ 创建VXI-11链接:" << link.id
               << "for client:" << client->objectName()
               << "(IP:" << client->peerAddress().toString()
               << ":" << client->peerPort() << ")";

    // 返回新创建的链接（注意：这是map中对象的引用）
    return &m_deviceLinks[link.id];
}

void TcpServerManager::destroyLink(quint32 linkId)
{
    QMutexLocker locker(&m_linkMutex);

    if (!m_deviceLinks.contains(linkId)) {
        qCWarning(tcp) << "destroyLink: 链接" << linkId << "不存在";
        return;
    }

    DeviceLink& link = m_deviceLinks[linkId];
    QString clientInfo = link.client ? link.client->objectName() : "null";

    // 从映射表中移除
    m_deviceLinks.remove(linkId);

    qCInfo(tcp) << "✅ 销毁VXI-11链接:" << linkId << "client:" << clientInfo;

    // 如果client仍然连接，可以发送断开通知
    if (link.client && link.client->state() == QAbstractSocket::ConnectedState) {
        qCDebug(tcp) << "链接" << linkId << "的client仍然连接，保持TCP连接";
    }
}

void TcpServerManager::handleGetPort(QTcpSocket* client, const QByteArray &data)
{
    quint32 xid = extractXid(data);

    qCDebug(tcp) << "处理 GETPORT 请求, XID:" << xid
                 << "client:" << (client ? client->objectName() : "null");

    // GETPORT请求应该返回我们的端口号
    QByteArray response;
    response.reserve(32);

    QDataStream stream(&response, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    // RPC回复头部
    stream << xid;                      // xid
    stream << quint32(Vxi11::REPLY);    // msg_type: REPLY
    stream << quint32(Vxi11::MSG_ACCEPTED); // reply_stat: MSG_ACCEPTED

    // AUTH_NULL验证器
    stream << quint32(AUTH_NULL);       // verf_flavor: AUTH_NULL
    stream << quint32(0);               // verf_length: 0

    // 接受状态
    stream << quint32(Vxi11::SUCCESS);  // accept_stat: SUCCESS
    stream << quint32(m_port);          // 返回端口号

    if (client) {
        client->write(response);
        qCDebug(tcp) << "GETPORT: 返回端口" << m_port << "给client:" << client->objectName();
    }
}

void TcpServerManager::handleCreateLink(QTcpSocket* client, const QByteArray &data)
{
    quint32 xid = extractXid(data);

    // 检查是否已存在链接
    DeviceLink* existingLink = findLinkByClient(client);
    if (existingLink) {
        // 如果已存在，返回现有链接ID
        QByteArray response = createErrorResponse(xid, existingLink->id);
        client->write(response);
        qCDebug(tcp) << "CREATE_LINK: 使用现有链接" << existingLink->id;
        return;
    }

    // 创建新链接
    DeviceLink* link = createLink(client);
    if (!link) {
        // 创建失败，返回错误
        QByteArray response = createErrorResponse(xid, Vxi11::OUT_OF_RESOURCES);
        client->write(response);
        qCWarning(tcp) << "CREATE_LINK: 创建链接失败";
        return;
    }

    // 返回成功响应
    QByteArray response = createErrorResponse(xid, link->id);
    client->write(response);
    qCDebug(tcp) << "CREATE_LINK: 创建新链接" << link->id;
}

void TcpServerManager::handleDeviceWrite(QTcpSocket* client, const QByteArray &data)
{
    quint32 xid = extractXid(data);

    // 查找链接
    DeviceLink* link = findLinkByClient(client);
    if (!link) {
        // 无有效链接，返回错误
        QByteArray response = createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(response);
        qCWarning(tcp) << "DEVICE_WRITE: 无有效链接";
        return;
    }

    // 解析写入参数（简化处理）
    // 在实际实现中，这里应该解析XDR编码的参数
    if (data.size() >= 32) {
        // 从数据中提取写入的内容（简化版本）
        // 实际应该使用XDR解码

        qCDebug(tcp) << "DEVICE_WRITE: 链接" << link->id << "收到数据，大小:" << data.size();

        // 返回成功响应
        QByteArray response = createErrorResponse(xid, Vxi11::NO_ERROR);
        client->write(response);
    } else {
        QByteArray response = createErrorResponse(xid, Vxi11::PARAMETER_ERROR);
        client->write(response);
        qCWarning(tcp) << "DEVICE_WRITE: 参数错误";
    }
}

void TcpServerManager::handleDeviceRead(QTcpSocket* client, const QByteArray &data)
{
    quint32 xid = extractXid(data);

    // 查找链接
    DeviceLink* link = findLinkByClient(client);
    if (!link) {
        QByteArray response = createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(response);
        qCWarning(tcp) << "DEVICE_READ: 无有效链接";
        return;
    }

    // 构建读取响应
    // 这里可以返回一些设备状态信息
    QByteArray response;
    response.reserve(40);

    QDataStream stream(&response, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    // RPC回复头部
    stream << xid;                      // xid
    stream << quint32(1);               // msg_type: REPLY
    stream << quint32(0);               // reply_stat: MSG_ACCEPTED

    // AUTH_NULL验证器
    stream << quint32(0);               // verf_flavor: AUTH_NULL
    stream << quint32(0);               // verf_length: 0

    // 接受状态
    stream << quint32(0);               // accept_stat: SUCCESS

    // 读取响应数据
    stream << quint32(0);               // error: NO_ERROR

    // 返回模拟的设备信息
    QString deviceInfo = QString("*IDN? response: %1,%2,%3,%4")
                         .arg(getManufacturer, getModel, getSerialNumber, getFirmwareVersion);
    QByteArray infoData = deviceInfo.toUtf8();

    stream << quint32(infoData.size());  // 数据长度
    stream.writeRawData(infoData.constData(), infoData.size());

    client->write(response);
    qCDebug(tcp) << "DEVICE_READ: 返回设备信息，大小:" << response.size();
}

void TcpServerManager::handleDeviceDocmd(QTcpSocket* client, const QByteArray &data)
{
    quint32 xid = extractXid(data);

    // 查找链接
    DeviceLink* link = findLinkByClient(client);
    if (!link) {
        QByteArray response = createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(response);
        qCWarning(tcp) << "DEVICE_DOCMD: 无有效链接";
        return;
    }

    // 处理设备命令（这里处理SCPI命令）
    // 简化实现：返回成功
    QByteArray response = createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(response);
    qCDebug(tcp) << "DEVICE_DOCMD: 处理设备命令";
}

void TcpServerManager::handleDestroyLink(QTcpSocket* client, const QByteArray &data)
{
    quint32 xid = extractXid(data);

    // 查找链接
    DeviceLink* link = findLinkByClient(client);
    if (!link) {
        // 即使没有找到链接，也返回成功
        QByteArray response = createErrorResponse(xid, Vxi11::NO_ERROR);
        client->write(response);
        qCDebug(tcp) << "DESTROY_LINK: 无链接可销毁";
        return;
    }

    // 销毁链接
    quint32 linkId = link->id;
    destroyLink(linkId);

    // 返回成功响应
    QByteArray response = createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(response);
    qCDebug(tcp) << "DESTROY_LINK: 销毁链接" << linkId;
}

// ==================== 析构部分 ====================

TcpServerManager::~TcpServerManager()
{
    // 停止链接清理定时器
    if (m_linkCleanupTimer) {
        m_linkCleanupTimer->stop();
        delete m_linkCleanupTimer;
        m_linkCleanupTimer = nullptr;
    }
    // 清理所有链接
    {
        QMutexLocker locker(&m_linkMutex);
        m_deviceLinks.clear();
    }

    stopServer();
    qCDebug(tcp) << "TcpServerManager destroyed";
}

void TcpServerManager::stopServer()
{
    QMutexLocker locker(&m_Mutex);

    if (m_state.load() == STATE_STOPPED) {return;}

    m_state.store(STATE_STOPPING);

    m_heartbeatTimer->stop();
    m_cleanupTimer->stop();
    delete m_heartbeatTimer;
    delete m_cleanupTimer;

    {
        for (QTcpSocket *client : qAsConst(m_clients)) {
            client->disconnectFromHost();
            if (client->state() == QAbstractSocket::ConnectedState) {
                client->waitForDisconnected(1000);
            }
        }
        m_clients.clear();

        if (m_tcpServer->isListening()) {
            m_tcpServer->close();
        }
    }

    if (m_serverThread && m_serverThread->isRunning()) {
        m_serverThread->quit();
        m_serverThread->wait(1000);// 等待1秒
        m_serverThread->deleteLater();
        delete m_serverThread;
    }

    m_state.store(STATE_STOPPED);
    qCInfo(tcp) << "TcpServer stopped";
}

// ============================================================================
// 调用示例
/*
#include "tcpserver.h"

void TcpManager(CanWorker *canWorker)
{
    TcpServerManager *tcpServer = new TcpServerManager();

    tcpServer->startServer();

    QObject::connect(canWorker, &CanWorker::frameReceived,
                     tcpServer, &TcpServerManager::forwardCanData,
                     Qt::QueuedConnection);
    QObject::connect(tcpServer, &TcpServerManager::canSendRequest,
                     canWorker, &CanWorker::sendFrame,
                     Qt::QueuedConnection);


    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                     tcpServer, &TcpServerManager::stopServer,
                     Qt::QueuedConnection);
    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                     tcpServer, &QObject::deleteLater,
                     Qt::QueuedConnection);
}
*/

