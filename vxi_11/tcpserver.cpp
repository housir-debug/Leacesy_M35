#include "tcpserver.h"
#include <QElapsedTimer>
#include <QRandomGenerator>
#include <QDataStream>
#include <QtEndian>
#include <sys/socket.h>
#include <arpa/inet.h>

// ===================== 初始化部分 =================================

Q_LOGGING_CATEGORY(tcp, "tcp:")

TcpServerManager::TcpServerManager(QObject *parent)
    : QObject(parent)
{
    m_scpiManager = new ScpiManager(this);

    if (m_scpiManager->init()) {
        qCDebug(tcp) << "SCPI管理器初始化成功";} else {qCCritical(tcp) << "SCPI管理器初始化失败";}
}

void TcpServerManager::forwardCanData(quint32 canId, const QByteArray &data, qint64 timestamp)
{
    if (m_state.load() != STATE_RUNNING) {return;}

    Q_UNUSED(canId)
    Q_UNUSED(timestamp)

    sendToAllClients(data);
}

void TcpServerManager::forwardSerialData(const QByteArray &data){
    if (m_state.load() != STATE_RUNNING) {return;}

    sendToAllClients(data);
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
                failedCount++;} else {successCount++;}   //client->flush();
        }
    }

    if (successCount == m_clients.size()) {
        if (data != "HEARTBEAT"){
            qint64 elapsed = m_testtimer.elapsed();
            qCDebug(tcp) << "eth-can test time:" <<elapsed;
        }
    }else{qCDebug(tcp) << "tcpsendclient failcount:" <<failedCount<< "clients,"<< successCount << "clients";}
}

// ===================== 启动部分 =================================

bool TcpServerManager::startServer()
{
    if (m_state.load() != STATE_STOPPED) {return false;}

    if (!m_serverThread){
        m_cleanupTimer = new QTimer(this);
        m_cleanupTimer->setInterval(5000); // 5秒

        m_tcpServer = new QTcpServer(this);
        m_serverThread = new QThread(this);
        m_serverThread->setObjectName("TcpServer");
    }

    if (thread() != m_serverThread) {
        m_state.store(STATE_STARTING);
        this->moveToThread(m_serverThread);
        m_tcpServer->moveToThread(m_serverThread);
        m_cleanupTimer->moveToThread(m_serverThread);
    }

    if (!m_serverThread->isRunning()) {
        m_serverThread->start();

        connect(m_cleanupTimer, &QTimer::timeout,this, [this]{
            for (int i = this->m_clients.size() - 1; i >= 0; --i) {
                QTcpSocket* client = this->m_clients.at(i);
                if (client->state() != QAbstractSocket::ConnectedState) {
                    qCDebug(tcp) << "Cleaned up disconnected client:"<< client->objectName();
                    this->m_clients.removeAt(i);
                    client->deleteLater();
                }
            }
        });
        connect(m_tcpServer, &QTcpServer::newConnection,
                this, &TcpServerManager::onNewConnection);

        QMetaObject::invokeMethod(this, [this]() {
            if (m_tcpServer->listen(QHostAddress::Any, ConfigManager::s_vxiPort)) {
                m_state.store(STATE_RUNNING);
                m_cleanupTimer->start();
                qCDebug(tcp) << "TcpServer started on port" << ConfigManager::s_vxiPort;
            } else {
                m_state.store(STATE_STOPPED);
                stopServer();
                qCCritical(tcp) << QString("Failed to start TCP server: %1").arg(m_tcpServer->errorString());
            }
        }, Qt::QueuedConnection);

        QTimer::singleShot(1000, this, &TcpServerManager::registerWithRpcbind);
        return true;
    }
    return false;
}

void TcpServerManager::onNewConnection()
{
    QTcpSocket *client = m_tcpServer->nextPendingConnection();
    if (!client) {return;}

    {
        QMutexLocker locker(&m_Mutex);
        if (m_clients.size() >= 10) {
            qCWarning(tcp) << "client limit reached, rejecting connection "<< client->peerAddress().toString();
            client->disconnectFromHost();
            delete client;
            return;
        }

        m_clients.append(client);
    }

    QString clientInfo = QString("%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort());
    client->setObjectName(clientInfo);

    connect(client, &QTcpSocket::errorOccurred,this, [client](QAbstractSocket::SocketError error){
         qCWarning(tcp) << "Socket error from" << client->objectName()<< ":" << client->errorString() << "|" << error;
    });
    connect(client, &QTcpSocket::disconnected,this, [this, client](){
        qCDebug(tcp) << "Client disconnected:" << client->objectName()<< ", remaining clients:" << m_clients.size();
        this->m_clients.removeOne(client);
        client->deleteLater();
    });
    connect(client, &QTcpSocket::readyRead,this, [this, client](){
        QByteArray rawData = client->readAll();
        qCDebug(tcp) << "RECEIVED FROM" << client->objectName()<< "size:" << rawData.size()<< "hex:" << rawData.toHex(' ');
        this->processClientData(client,rawData);
    });

    qCDebug(tcp) << "New client connected:" << clientInfo<< ", total clients:" << m_clients.size();;
}

bool TcpServerManager::registerWithRpcbind()
{
    if (TirpcDynamicLoader::instance().load()) {
        bool result = TirpcDynamicLoader::instance().pmap_set(   //pmap_unset
            Vxi11::DEVICE_CORE,         // VXI-11程序号
            Vxi11::DEVICE_CORE_VERSION, // 版本
            IPPROTO_TCP,                // TCP协议
            ConfigManager::s_vxiPort
        );

        if (result) {qCDebug(tcp) << "✅ 使用 libtirpc 成功注册 VXI-11 服务到端口" << ConfigManager::s_vxiPort;}
        else {qCWarning(tcp) << "❌ 使用 libtirpc 注册失败，尝试手动注册";
            int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            if (sockfd < 0) {
                qCWarning(tcp) << "创建socket失败:" << strerror(errno);
                return false;
            }

            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(111);
            inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

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
            quint32 xid = QRandomGenerator::global()->generate();
            call.xid = htonl(xid);
            call.msg_type = htonl(0);        // CALL
            call.rpcvers = htonl(2);         // RPC版本2
            call.prog = htonl(100000);       // PORTMAP
            call.vers = htonl(2);            // PORTMAP版本2
            call.proc = htonl(1);            // SET过程
            call.map_prog = htonl(395183);   // VXI-11程序号
            call.map_vers = htonl(1);        // VXI-11版本1
            call.map_prot = htonl(6);        // TCP协议
            call.map_port = htonl(ConfigManager::s_vxiPort);   //

            sendto(sockfd, &call, sizeof(call), 0,(struct sockaddr*)&addr, sizeof(addr));
            close(sockfd);
            qCDebug(tcp) << "手动注册 VXI-11 服务到端口" << ConfigManager::s_vxiPort;
        }

    }else{qCWarning(tcp)<<"TirpcDynamicLoader::instance() error";}
    return true;
}

// ===================== 信息处理部分 =================================

void TcpServerManager::processClientData(QTcpSocket *client,const QByteArray newdata)
{
    if (newdata.size() >= 32) {   // VXI-11
        const uchar* rawData = reinterpret_cast<const uchar*>(newdata.constData());
        quint32 progNum = qFromBigEndian<quint32>(rawData + 16);
        if(progNum == Vxi11::DEVICE_CORE){
            handleVxi11RpcCall(client, newdata);
            return;
        }
    }

    QString message = QString::fromUtf8(newdata).trimmed();
    if (message.startsWith("*") || message.contains(":")) {   // SCPI-ASCAL
        qCDebug(tcp) << "SCPI command detected:" << message;

        QByteArray response = m_scpiManager->processCommand(newdata);
        client->write(response);
        qCDebug(tcp) << "SCPI响应发送:" << response << ",长度:" << response.size() << "bytes";
        return;
    }

    qCWarning(tcp) << "Invalid brief inform!----Throw!" ;
    return;
}

void TcpServerManager::handleVxi11RpcCall(QTcpSocket* client, const QByteArray &data)
{
    const quint32* ptr = reinterpret_cast<const quint32*>(data.constData() + 4);
    quint32 xid = qFromBigEndian<quint32>(ptr);

    const quint32* ptr_d = reinterpret_cast<const quint32*>(data.constData() + 24);
    quint32 procedure = qFromBigEndian<quint32>(ptr_d);
    qCDebug(tcp) << "VXI-11 RPC调用,过程:" << procedure << ",XID:"<<xid;

    switch (procedure) {
        case Vxi11::CREATE_LINK:
            handleCreateLink(client, data, xid);break;

        case Vxi11::DEVICE_WRITE:
            handleDeviceWrite(client, data, xid);break;

        case Vxi11::DEVICE_READ:
            handleDeviceRead(client, data, xid);break;

        case Vxi11::DEVICE_READSTB:
            handleDeviceReadStb(client, data, xid);break;

        case Vxi11::DEVICE_TRIGGER:
            handleDeviceTrigger(client, data, xid);break;

        case Vxi11::DEVICE_CLEAR:
            handleDeviceClear(client, data, xid);break;

        case Vxi11::DEVICE_REMOTE:
            handleDeviceRemote(client, data, xid);break;

        case Vxi11::DEVICE_LOCAL:
            handleDeviceLocal(client, data, xid);break;

        case Vxi11::DEVICE_LOCK:
            handleDeviceLock(client, data, xid);break;

        case Vxi11::DEVICE_UNLOCK:
            handleDeviceUnlock(client, data, xid);break;

        case Vxi11::DEVICE_ENABLE_SRQ:
            handleDeviceEnableSrq(client, data, xid);break;

        case Vxi11::DEVICE_DOCMD:
            handleDeviceDocmd(client, data, xid);break;

        case Vxi11::DESTROY_LINK:
            handleDestroyLink(client, data, xid);break;

        case Vxi11::CREATE_INTR_CHAN:
            handleCreateIntrChan(client, data, xid);break;

        case Vxi11::DESTROY_INTR_CHAN:
            handleDestroyIntrChan(client, data, xid);break;

        default:
            qCWarning(tcp) << "未知的VXI-11过程:" << procedure;
            break;
    }
}

// ===================== VXI-11 RPC处理 =================================

void TcpServerManager::handleCreateLink(QTcpSocket* client, const QByteArray &data,const quint32 &xid)
{
    Q_UNUSED(data)
    for (auto it = m_deviceLinks.begin(); it != m_deviceLinks.end(); ++it) {
        if (it->client->peerAddress() == client->peerAddress()) {
            qCWarning(tcp) << "client:" << client->objectName() << "已存在链接号:" << it->id<< "拒绝创建新连接";
            QByteArray response = createErrorResponse(xid, Vxi11::CHANNEL_ALREADY_ESTABLISHED);
            client->write(response);
            return;
        }
    }

    DeviceLink link;
    link.client = client;
    link.id = m_nextLinkId++;
    link.createTime = QDateTime::currentDateTime();
    m_deviceLinks.insert(link.id, link);
    qCDebug(tcp) << "✅ 创建VXI-11链接:" << link.id<< "for client:" << client->objectName();

    DeviceLink* D_link = &m_deviceLinks[link.id];
    QByteArray response;
    response.reserve(44);
    QDataStream stream(&response, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    stream << quint32(0x80000028);     // RPC + size
    stream << xid;                     // XID
    stream << quint32(1);              // msg_type: REPLY (1)
    stream << quint32(0);              // reply_stat: MSG_ACCEPTED (0)
    stream << quint32(0);              // auth_flavor: AUTH_NULL (0)
    stream << quint32(0);              // auth_length: 0
    stream << quint32(0);              // accept_stat: SUCCESS (0)
    stream << quint32(0);              // error_code: 0 (no error)
    stream << D_link->id;              // link_id (Device_Link ID)
    stream << quint32(0);              // abort_port
    stream << quint32(2048);           // max_recv_size

    client->write(response);
    qCDebug(tcp) << "CREATE_LINK: " << D_link->id << "Response:" << response.toHex(' ');
}

void TcpServerManager::handleDeviceWrite(QTcpSocket* client, const QByteArray &data,const quint32 &xid)
{
    const quint32* lidPtr = reinterpret_cast<const quint32*>(data.constData() + 44);
    quint32 lid = qFromBigEndian<quint32>(lidPtr);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        QByteArray errorResponse = createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(errorResponse);
        return;
    }

    const quint32* dataLenPtr = reinterpret_cast<const quint32*>(data.constData() + 60);
    quint32 dataLen = qFromBigEndian<quint32>(dataLenPtr);
    if (dataLen <= 0){
        qCWarning(tcp)<<"current write command is nullptr!";
        return;
    };

    QByteArray scpiData = data.mid(64, dataLen);
    qCDebug(tcp) << "DEVICE_WRITE: 链接" << lid << "收到SCPI命令:" << QString::fromUtf8(scpiData).trimmed();
    DeviceLink& link = m_deviceLinks[lid];
    link.pending_Vxi_Scpi_response = m_scpiManager->processCommand(scpiData);

    QByteArray response;
    response.reserve(36);
    QDataStream stream(&response, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    stream << quint32(0x80000020);     // RPC头部长度
    stream << xid;
    stream << quint32(1);              // REPLY
    stream << quint32(0);              // MSG_ACCEPTED
    stream << quint32(0);              // AUTH_NULL
    stream << quint32(0);              // AUTH_LENGTH
    stream << quint32(0);              // SUCCESS
    stream << quint32(0);              // error_code: NO_ERROR
    stream << dataLen;                 // write_size

    client->write(response);
    qCDebug(tcp) << "DEVICE_WRITE: " << lid << "Response:" <<  response.toHex(' ');
}

void TcpServerManager::handleDeviceRead(QTcpSocket* client, const QByteArray &data,const quint32 &xid)
{
    const quint32* lidPtr = reinterpret_cast<const quint32*>(data.constData() + 44);
    quint32 lid = qFromBigEndian<quint32>(lidPtr);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        QByteArray errorResponse = createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(errorResponse);
        return;
    }

    DeviceLink& link = m_deviceLinks[lid];
    QByteArray responseData = link.pending_Vxi_Scpi_response;
    link.pending_Vxi_Scpi_response.clear();
    qCDebug(tcp) << "DEVICE_READ: 返回SCPI响应，内容:"<< responseData;

    const quint32* requestSizePtr = reinterpret_cast<const quint32*>(data.constData() + 48);
    quint32 requestSize = qFromBigEndian<quint32>(requestSizePtr);
    qCDebug(tcp) << "DEVICE_READ: 链接" << lid << "请求读取数据，最大" << requestSize << "字节";
    quint32 reason = 0x00000004;
    if (requestSize > 0 && responseData.size() > static_cast<int>(requestSize)) {
        qCWarning(tcp)<<"current response > request restrict!";
        responseData.resize(requestSize);
        reason = 0x00000001;  // REQCNT:
    }

    quint32 alignedLen = (responseData.size() + 3) & ~3;
    responseData.append(alignedLen - responseData.size(), '\0');
    QByteArray response;
    int totallen = 40+alignedLen;
    quint32 fragHead = 0x80000000 | (totallen - 4);
    response.reserve(totallen);
    QDataStream stream(&response, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    stream << fragHead;                // RPC头部长度
    stream << xid;
    stream << quint32(1);              // REPLY
    stream << quint32(0);              // MSG_ACCEPTED
    stream << quint32(0);              // AUTH_NULL
    stream << quint32(0);              // AUTH_LENGTH
    stream << quint32(0);              // SUCCESS
    stream << quint32(0);              // error_code: NO_ERROR
    stream << reason;                  // END_FLAG
    stream << responseData;            // QDataStream 会为 QByteArray 写入长度前缀

    client->write(response);
    qCDebug(tcp) << "DEVICE_READ: " << lid << "Response:" <<  response.toHex(' ');
}

void TcpServerManager::handleDeviceReadStb(QTcpSocket* client, const QByteArray &data, const quint32 &xid)
{
    const quint32* lidPtr = reinterpret_cast<const quint32*>(data.constData() + 44);
    quint32 lid = qFromBigEndian<quint32>(lidPtr);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        QByteArray errorResponse = createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(errorResponse);
        return;
    }

    QByteArray response;
    response.reserve(36);
    QDataStream stream(&response, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    stream << quint32(0x80000020);     // RPC头部长度
    stream << xid;
    stream << quint32(1);              // REPLY
    stream << quint32(0);              // MSG_ACCEPTED
    stream << quint32(0);              // AUTH_NULL
    stream << quint32(0);              // AUTH_LENGTH
    stream << quint32(0);              // SUCCESS
    stream << quint32(0);              // error: NO_ERROR
    stream << quint32(0);              // 状态字节 (0 = 设备就绪)

    client->write(response);
    qCDebug(tcp) << "DEVICE_READSTB: 链接" << lid <<"Response:" <<  response.toHex(' ');
}

void TcpServerManager::handleDeviceTrigger(QTcpSocket* client, const QByteArray &data, const quint32 &xid)
{
    const quint32* lidPtr = reinterpret_cast<const quint32*>(data.constData() + 44);
    quint32 lid = qFromBigEndian<quint32>(lidPtr);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        QByteArray errorResponse = createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(errorResponse);
        return;
    }

    qCDebug(tcp) << "DEVICE_TRIGGER: 链接" << lid << "收到触发信号";
    QByteArray response = createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(response);
}

void TcpServerManager::handleDeviceClear(QTcpSocket* client, const QByteArray &data, const quint32 &xid)
{
    const quint32* lidPtr = reinterpret_cast<const quint32*>(data.constData() + 44);
    quint32 lid = qFromBigEndian<quint32>(lidPtr);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        QByteArray errorResponse = createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(errorResponse);
        return;
    }

    qCDebug(tcp) << "DEVICE_CLEAR: 链接" << lid << "执行清除操作";
    QByteArray response = createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(response);
}

void TcpServerManager::handleDeviceRemote(QTcpSocket* client, const QByteArray &data, const quint32 &xid)
{
    const quint32* lidPtr = reinterpret_cast<const quint32*>(data.constData() + 44);
    quint32 lid = qFromBigEndian<quint32>(lidPtr);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        QByteArray errorResponse = createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(errorResponse);
        return;
    }

    qCDebug(tcp) << "DEVICE_REMOTE: 链接" << lid << "设置为远程模式";
    QByteArray response = createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(response);
}

void TcpServerManager::handleDeviceLocal(QTcpSocket* client, const QByteArray &data, const quint32 &xid)
{
    const quint32* lidPtr = reinterpret_cast<const quint32*>(data.constData() + 44);
    quint32 lid = qFromBigEndian<quint32>(lidPtr);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        QByteArray errorResponse = createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(errorResponse);
        return;
    }

    qCDebug(tcp) << "DEVICE_LOCAL: 链接" << lid << "设置为本地模式";
    QByteArray response = createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(response);
}

void TcpServerManager::handleDeviceLock(QTcpSocket* client, const QByteArray &data, const quint32 &xid)
{
    const quint32* lidPtr = reinterpret_cast<const quint32*>(data.constData() + 44);
    quint32 lid = qFromBigEndian<quint32>(lidPtr);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        QByteArray errorResponse = createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(errorResponse);
        return;
    }

    DeviceLink& link = m_deviceLinks[lid];
    if (link.lock) {
        QByteArray response = createErrorResponse(xid, Vxi11::DEVICE_LOCKED_BY_ANOTHER_LINK);
        client->write(response);
        return;
    }

    link.lock = true;
    qCDebug(tcp) << "DEVICE_LOCK: 链接" << lid << "锁定设备";
    QByteArray response = createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(response);
}

void TcpServerManager::handleDeviceUnlock(QTcpSocket* client, const QByteArray &data, const quint32 &xid)
{
    const quint32* lidPtr = reinterpret_cast<const quint32*>(data.constData() + 44);
    quint32 lid = qFromBigEndian<quint32>(lidPtr);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        QByteArray errorResponse = createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(errorResponse);
        return;
    }

    DeviceLink& link = m_deviceLinks[lid];
    if (!link.lock) {
        QByteArray response = createErrorResponse(xid, Vxi11::NO_LOCK_HELD_BY_THIS_LINK);
        client->write(response);
        return;
    }

    link.lock = false;
    qCDebug(tcp) << "DEVICE_UNLOCK: 链接" << lid << "解锁设备";
    QByteArray response = createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(response);
}

void TcpServerManager::handleDeviceEnableSrq(QTcpSocket* client, const QByteArray &data, const quint32 &xid)
{
    const quint32* lidPtr = reinterpret_cast<const quint32*>(data.constData() + 44);
    quint32 lid = qFromBigEndian<quint32>(lidPtr);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        QByteArray errorResponse = createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(errorResponse);
        return;
    }
    const quint32* enablePtr = reinterpret_cast<const quint32*>(data.constData() + 40);
    quint32 enable = qFromBigEndian<quint32>(enablePtr);

    qCDebug(tcp) << "DEVICE_ENABLE_SRQ: 链接" << lid << (enable ? "启用" : "禁用") << "服务请求";

    QByteArray response = createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(response);
}

void TcpServerManager::handleDeviceDocmd(QTcpSocket* client, const QByteArray &data,const quint32 &xid)
{
    Q_UNUSED(data)
    const quint32* lidPtr = reinterpret_cast<const quint32*>(data.constData() + 44);
    quint32 lid = qFromBigEndian<quint32>(lidPtr);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        QByteArray errorResponse = createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(errorResponse);
        return;
    }

    qCDebug(tcp) << "DEVICE_DOCMD: 处理设备特定命令";
    QByteArray response = createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(response);
}

void TcpServerManager::handleDestroyLink(QTcpSocket* client, const QByteArray &data,const quint32 &xid)
{
    Q_UNUSED(data)
    for (auto it = m_deviceLinks.begin(); it != m_deviceLinks.end(); ++it) {
        if (it->client == client) {
            qCWarning(tcp) << "client:" << client->objectName() << "已断开连接，删除存在链接号:" << it->id;
            m_nextLinkId--;
            m_deviceLinks.remove(it->id);
            createErrorResponse(xid,Vxi11::NO_ERROR);
            client->disconnectFromHost();
            return;
        }
    }
}

void TcpServerManager::handleCreateIntrChan(QTcpSocket* client, const QByteArray &data, const quint32 &xid)
{
    const quint32* lidPtr = reinterpret_cast<const quint32*>(data.constData() + 44);
    quint32 lid = qFromBigEndian<quint32>(lidPtr);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        QByteArray errorResponse = createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(errorResponse);
        return;
    }

    qCDebug(tcp) << "CREATE_INTR_CHAN: 客户端请求创建中断通道";
    QByteArray response = createErrorResponse(xid, Vxi11::CHANNEL_NOT_ESTABLISHED);
    client->write(response);
}

void TcpServerManager::handleDestroyIntrChan(QTcpSocket* client, const QByteArray &data, const quint32 &xid)
{
    const quint32* lidPtr = reinterpret_cast<const quint32*>(data.constData() + 44);
    quint32 lid = qFromBigEndian<quint32>(lidPtr);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        QByteArray errorResponse = createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(errorResponse);
        return;
    }

    qCDebug(tcp) << "DESTROY_INTR_CHAN: 客户端请求销毁中断通道";
    QByteArray response = createErrorResponse(xid, Vxi11::CHANNEL_NOT_ESTABLISHED);
    client->write(response);
}

QByteArray TcpServerManager::createErrorResponse(quint32 xid, quint32 error)
{
    QByteArray response;
    response.reserve(32);
    QDataStream stream(&response, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    stream << quint32(0x8000001c);;     // RPC头部长度
    stream << xid;
    stream << quint32(1);               // msg_type: REPLY
    stream << quint32(0);               // reply_stat: MSG_ACCEPTED
    stream << quint32(0);               // verf_flavor: AUTH_NULL
    stream << quint32(0);               // verf_length: 0
    stream << quint32(0);               // accept_stat: SUCCESS
    stream << error;                    // error:

    return response;
}

// ==================== 析构部分 ====================

TcpServerManager::~TcpServerManager()
{
    stopServer();
    qCDebug(tcp) << "TcpServerManager destroyed";
}

void TcpServerManager::stopServer()
{
    QMutexLocker locker(&m_Mutex);

    if (m_state.load() == STATE_STOPPED) {return;}

    m_state.store(STATE_STOPPING);
    m_deviceLinks.clear();
    m_cleanupTimer->stop();
    delete m_cleanupTimer;

    {
        for (QTcpSocket *client : qAsConst(m_clients)) {
            client->disconnectFromHost();
            if (client->state() != QAbstractSocket::UnconnectedState) {
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



