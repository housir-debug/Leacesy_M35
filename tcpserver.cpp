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

    if (m_scpiManager->init(getManufacturer,getModel,getSerialNumber,getFirmwareVersion)) {
        qCDebug(tcp) << "SCPI管理器初始化成功";
    } else {qCCritical(tcp) << "SCPI管理器初始化失败";}
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
                failedCount++;
            } else {successCount++;}   //client->flush();
        } else {failedCount++;}
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

        connect(m_cleanupTimer, &QTimer::timeout,
                this, &TcpServerManager::cleanupDisconnectedClients);
        connect(m_tcpServer, &QTcpServer::newConnection,
                this, &TcpServerManager::onNewConnection);

        QMetaObject::invokeMethod(this, [this]() {
            if (m_tcpServer->listen(QHostAddress::Any, m_port)) {
                m_state.store(STATE_RUNNING);
                m_cleanupTimer->start();
                qCDebug(tcp) << "TcpServer started on port" << m_port;
            } else {
                m_state.store(STATE_STOPPED);
                stopServer();
                qCCritical(tcp) << QString("Failed to start TCP server: %1").arg(m_tcpServer->errorString());
            }
        }, Qt::QueuedConnection);

        //QTimer::singleShot(1000, this, &TcpServerManager::registerWithRpcbind);
        return true;
    }
    return false;
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

    //connect(client, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
    //        this, &TcpServerManager::onSocketError);
    connect(client, &QTcpSocket::errorOccurred,this, [client](QAbstractSocket::SocketError error){
         qCWarning(tcp) << "Socket error from" << client->objectName()<< ":" << client->errorString() << "|" << error;
    });
    connect(client, &QTcpSocket::disconnected,this, [this, client](){
        this->m_clients.removeOne(client);
        client->deleteLater();
        qCDebug(tcp) << "Client disconnected:" << client->objectName()<< ", remaining clients:" << m_clients.size();
        for (auto it = this->m_deviceLinks.begin(); it != this->m_deviceLinks.end(); ++it) {
            if (it->client == client) {
                m_deviceLinks.remove(it->id);
                qCWarning(tcp) << "client:" << client->objectName() << "已断开连接，删除存在链接号:" << it->id;
            }
        }
    });
    connect(client, &QTcpSocket::readyRead,this, [this, client](){
        QByteArray rawData = client->readAll();
        this->processClientData(client,rawData);
        qCDebug(tcp) << "RECEIVED FROM" << client->objectName()<< "size:" << rawData.size()<< "hex:" << rawData.toHex(' ');
    });

    qCDebug(tcp) << "New client connected:" << clientInfo<< ", total clients:" << m_clients.size();;
}

bool TcpServerManager::registerWithRpcbind()
{
    if (TirpcDynamicLoader::instance().load()) {
        bool result = TirpcDynamicLoader::instance().pmap_set(   //pmap_unset
            Vxi11::DEVICE_CORE,      // VXI-11程序号
            Vxi11::DEVICE_CORE_VERSION, // 版本
            IPPROTO_TCP,             // TCP协议
            m_port                   // 端口
        );

        if (result) {qCDebug(tcp) << "✅ 使用 libtirpc 成功注册 VXI-11 服务到端口" << m_port;}
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
            call.map_port = htonl(m_port);   //

            sendto(sockfd, &call, sizeof(call), 0,(struct sockaddr*)&addr, sizeof(addr));
            close(sockfd);
            qCDebug(tcp) << "手动注册 VXI-11 服务到端口" << m_port;
        }
    }else{qCWarning(tcp)<<"TirpcDynamicLoader::instance() error";}
    return true;
}

// ===================== 信息处理部分 =================================

void TcpServerManager::onSocketError(QAbstractSocket::SocketError error)
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) {return;}

    qCWarning(tcp) << "Socket error from" << client->objectName()<< ":" << client->errorString() << "|" << error;
}

void TcpServerManager::onClientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) {return;}

    {
        QMutexLocker locker(&m_Mutex);
        m_clients.removeOne(client);
    }

    client->deleteLater();
    qCDebug(tcp) << "Client disconnected:" << client->objectName()<< ", remaining clients:" << m_clients.size();
}

void TcpServerManager::onClientReadyRead()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) {return;}

    QByteArray rawData = client->readAll();
    qCDebug(tcp) << "RECEIVED FROM" << client->objectName()<< "size:" << rawData.size()<< "hex:" << rawData.toHex(' ');

    QTimer::singleShot(0, this, [this, client,rawData]() { processClientData(client,rawData);});// 异步处理，立即放入事件队列
}

void TcpServerManager::processClientData(QTcpSocket *client,const QByteArray newdata)
{
    if (newdata.size() >= 20) {   // VXI-11
        const uchar* rawData = reinterpret_cast<const uchar*>(newdata.constData());
        quint32 progNum = qFromBigEndian<quint32>(rawData + 16);
        if(progNum == Vxi11::DEVICE_CORE){
            qCDebug(tcp) << "VXI-11 RPC call detected";
            handleVxi11RpcCall(client, newdata);
            return;
        }
    }

    QString message = QString::fromUtf8(newdata).trimmed();
    if (message.startsWith("*") || message.contains(":")) {   // SCPI-ASCAL
        qCDebug(tcp) << "SCPI command detected:" << message;

        QByteArray response = m_scpiManager->processCommand(newdata);
        if (!response.isEmpty() && client->state() == QAbstractSocket::ConnectedState) {
            client->write(response);
            qCDebug(tcp) << "SCPI响应发送:" << response << ",长度:" << response.size() << "bytes";
        } else {qCDebug(tcp) << "SCPI响应为空或客户端未连接";}
        return;
    }

    qCWarning(tcp) << "Invalid brief inform!----Throw!" ;
    return;
}

void TcpServerManager::handleVxi11RpcCall(QTcpSocket* client, const QByteArray &data)
{
    if (!client || data.size() < 28) {return;}

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
    link.id = m_nextLinkId++;
    link.lock = 0;
    link.createTime = QDateTime::currentDateTime();
    link.client = client;
    link.aborted = false;
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
    // device_write参数结构：
    // lid(4), io_timeout(4), lock_timeout(4), flags(4), data<>

    if (data.size() < 44) {
        QByteArray response = createErrorResponse(xid, Vxi11::PARAMETER_ERROR);
        client->write(response);
        return;
    }

    // 提取链接ID (位置36-39)
    const quint32* lidPtr = reinterpret_cast<const quint32*>(data.constData() + 36);
    quint32 lid = qFromBigEndian<quint32>(lidPtr);

    // 验证链接
    QByteArray errorResp;
    if (!validateLink(client, lid, errorResp, xid)) {
        client->write(errorResp);
        return;
    }

    // 提取数据长度和数据
    const quint32* dataLenPtr = reinterpret_cast<const quint32*>(data.constData() + 52);
    quint32 dataLen = qFromBigEndian<quint32>(dataLenPtr);

    if (data.size() < 56 + dataLen) {
        QByteArray response = createErrorResponse(xid, Vxi11::PARAMETER_ERROR);
        client->write(response);
        return;
    }

    // 提取实际数据
    QByteArray scpiData;
    if (dataLen > 0) {
        scpiData = data.mid(56, dataLen);
        qCDebug(tcp) << "DEVICE_WRITE: 链接" << lid << "收到SCPI命令:" << QString::fromUtf8(scpiData).trimmed();

        // 处理SCPI命令
        QByteArray response = m_scpiManager->processCommand(scpiData);
        qCDebug(tcp) << "SCPI响应:" << QString::fromUtf8(response).trimmed();
    }

    // 构建成功响应
    QByteArray response;
    response.reserve(36);
    QDataStream stream(&response, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    stream << quint32(0x80000020);     // RPC头部长度
    stream << xid;
    stream << quint32(1);              // REPLY
    stream << quint32(0);              // MSG_ACCEPTED
    stream << quint32(0);              // AUTH_NULL
    stream << quint32(0);
    stream << quint32(0);              // SUCCESS
    stream << quint32(0);              // error_code: NO_ERROR
    stream << dataLen;                 // 实际写入的字节数

    client->write(response);
    qCDebug(tcp) << "DEVICE_WRITE: 链接" << lid << "成功写入" << dataLen << "字节";
}

void TcpServerManager::handleDeviceRead(QTcpSocket* client, const QByteArray &data,const quint32 &xid)
{
    Q_UNUSED(data)
    for (auto it = m_deviceLinks.begin(); it != m_deviceLinks.end(); ++it) {
        if (it->client == client) {
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
    }
}

void TcpServerManager::handleDeviceReadStb(QTcpSocket* client, const QByteArray &data, const quint32 &xid)
{
    // 解析参数：lid(4), flags(4), lock_timeout(4), io_timeout(4)
    if (data.size() < 44) {
        QByteArray response = createErrorResponse(xid, Vxi11::PARAMETER_ERROR);
        client->write(response);
        return;
    }

    // 提取链接ID (位置44-47)
    const quint32* lidPtr = reinterpret_cast<const quint32*>(data.constData() + 44);
    quint32 lid = qFromBigEndian<quint32>(lidPtr);

    // 验证链接
    QByteArray errorResp;
    if (!validateLink(client, lid, errorResp, xid)) {
        client->write(errorResp);
        return;
    }

    // 构建成功响应
    QByteArray response;
    response.reserve(32);
    QDataStream stream(&response, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    // RPC头部
    stream << quint32(0x80000018);     // RPC头部长度
    stream << xid;
    stream << quint32(1);              // REPLY
    stream << quint32(0);              // MSG_ACCEPTED
    stream << quint32(0);              // AUTH_NULL
    stream << quint32(0);
    stream << quint32(0);              // SUCCESS

    // device_readstb响应体
    stream << quint32(0);              // error: NO_ERROR
    stream << quint8(0x00);            // 状态字节 (0x00 = 设备就绪)

    // 对齐到4字节
    stream << quint8(0);               // 填充字节
    stream << quint16(0);              // 填充字节

    client->write(response);
    qCDebug(tcp) << "DEVICE_READSTB: 链接" << lid << "返回状态字节";
}

void TcpServerManager::handleDeviceTrigger(QTcpSocket* client, const QByteArray &data, const quint32 &xid)
{
    // 参数结构同device_readstb
    if (data.size() < 44) {
        QByteArray response = createErrorResponse(xid, Vxi11::PARAMETER_ERROR);
        client->write(response);
        return;
    }

    const quint32* lidPtr = reinterpret_cast<const quint32*>(data.constData() + 44);
    quint32 lid = qFromBigEndian<quint32>(lidPtr);

    QByteArray errorResp;
    if (!validateLink(client, lid, errorResp, xid)) {
        client->write(errorResp);
        return;
    }

    // 触发操作（如果有硬件触发需求可以在这里实现）
    qCDebug(tcp) << "DEVICE_TRIGGER: 链接" << lid << "收到触发信号";

    // 简单返回成功
    QByteArray response = createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(response);
}

void TcpServerManager::handleDeviceClear(QTcpSocket* client, const QByteArray &data, const quint32 &xid)
{
    if (data.size() < 44) {
        QByteArray response = createErrorResponse(xid, Vxi11::PARAMETER_ERROR);
        client->write(response);
        return;
    }

    const quint32* lidPtr = reinterpret_cast<const quint32*>(data.constData() + 44);
    quint32 lid = qFromBigEndian<quint32>(lidPtr);

    QByteArray errorResp;
    if (!validateLink(client, lid, errorResp, xid)) {
        client->write(errorResp);
        return;
    }

    // 执行清除操作（如果有需要清除的状态可以在这里实现）
    qCDebug(tcp) << "DEVICE_CLEAR: 链接" << lid << "执行清除操作";

    QByteArray response = createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(response);
}

void TcpServerManager::handleDeviceRemote(QTcpSocket* client, const QByteArray &data, const quint32 &xid)
{
    if (data.size() < 44) {
        QByteArray response = createErrorResponse(xid, Vxi11::PARAMETER_ERROR);
        client->write(response);
        return;
    }

    const quint32* lidPtr = reinterpret_cast<const quint32*>(data.constData() + 44);
    quint32 lid = qFromBigEndian<quint32>(lidPtr);

    QByteArray errorResp;
    if (!validateLink(client, lid, errorResp, xid)) {
        client->write(errorResp);
        return;
    }

    qCDebug(tcp) << "DEVICE_REMOTE: 链接" << lid << "设置为远程模式";

    QByteArray response = createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(response);
}

void TcpServerManager::handleDeviceLocal(QTcpSocket* client, const QByteArray &data, const quint32 &xid)
{
    if (data.size() < 44) {
        QByteArray response = createErrorResponse(xid, Vxi11::PARAMETER_ERROR);
        client->write(response);
        return;
    }

    const quint32* lidPtr = reinterpret_cast<const quint32*>(data.constData() + 44);
    quint32 lid = qFromBigEndian<quint32>(lidPtr);

    QByteArray errorResp;
    if (!validateLink(client, lid, errorResp, xid)) {
        client->write(errorResp);
        return;
    }

    qCDebug(tcp) << "DEVICE_LOCAL: 链接" << lid << "设置为本地模式";

    QByteArray response = createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(response);
}

void TcpServerManager::handleDeviceLock(QTcpSocket* client, const QByteArray &data, const quint32 &xid)
{
    // device_lock参数：lid(4), flags(4), lock_timeout(4)
    if (data.size() < 40) {
        QByteArray response = createErrorResponse(xid, Vxi11::PARAMETER_ERROR);
        client->write(response);
        return;
    }

    const quint32* lidPtr = reinterpret_cast<const quint32*>(data.constData() + 36);
    quint32 lid = qFromBigEndian<quint32>(lidPtr);

    QByteArray errorResp;
    if (!validateLink(client, lid, errorResp, xid)) {
        client->write(errorResp);
        return;
    }

    // 检查设备是否已被锁定
    DeviceLink& link = m_deviceLinks[lid];
    if (link.lock != 0 && link.lock != lid) {
        QByteArray response = createErrorResponse(xid, Vxi11::DEVICE_LOCKED_BY_ANOTHER_LINK);
        client->write(response);
        return;
    }

    // 锁定设备
    link.lock = lid;
    qCDebug(tcp) << "DEVICE_LOCK: 链接" << lid << "锁定设备";

    QByteArray response = createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(response);
}

void TcpServerManager::handleDeviceUnlock(QTcpSocket* client, const QByteArray &data, const quint32 &xid)
{
    // device_unlock参数只有lid(4)
    if (data.size() < 36) {
        QByteArray response = createErrorResponse(xid, Vxi11::PARAMETER_ERROR);
        client->write(response);
        return;
    }

    const quint32* lidPtr = reinterpret_cast<const quint32*>(data.constData() + 32);
    quint32 lid = qFromBigEndian<quint32>(lidPtr);

    QByteArray errorResp;
    if (!validateLink(client, lid, errorResp, xid)) {
        client->write(errorResp);
        return;
    }

    // 检查是否持有锁
    DeviceLink& link = m_deviceLinks[lid];
    if (link.lock != lid) {
        QByteArray response = createErrorResponse(xid, Vxi11::NO_LOCK_HELD_BY_THIS_LINK);
        client->write(response);
        return;
    }

    // 解锁设备
    link.lock = 0;
    qCDebug(tcp) << "DEVICE_UNLOCK: 链接" << lid << "解锁设备";

    QByteArray response = createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(response);
}

void TcpServerManager::handleDeviceEnableSrq(QTcpSocket* client, const QByteArray &data, const quint32 &xid)
{
    // 参数：lid(4), enable(4), handle<40>
    if (data.size() < 48) {
        QByteArray response = createErrorResponse(xid, Vxi11::PARAMETER_ERROR);
        client->write(response);
        return;
    }

    const quint32* lidPtr = reinterpret_cast<const quint32*>(data.constData() + 36);
    quint32 lid = qFromBigEndian<quint32>(lidPtr);

    QByteArray errorResp;
    if (!validateLink(client, lid, errorResp, xid)) {
        client->write(errorResp);
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
    for (auto it = m_deviceLinks.begin(); it != m_deviceLinks.end(); ++it) {
        if (it->client == client) {
            // 处理设备命令（这里处理SCPI命令）
            // 简化实现：返回成功
            QByteArray response = createErrorResponse(xid, Vxi11::NO_ERROR);
            client->write(response);
            qCDebug(tcp) << "DEVICE_DOCMD: 处理设备命令";
        }
    }
}

void TcpServerManager::handleDestroyLink(QTcpSocket* client, const QByteArray &data,const quint32 &xid)
{
    Q_UNUSED(data)
    for (auto it = m_deviceLinks.begin(); it != m_deviceLinks.end(); ++it) {
        if (it->client == client) {
            // 销毁链接
            quint32 linkId = it->id;

            createErrorResponse(xid, Vxi11::NO_ERROR);
            qCDebug(tcp) << "DESTROY_LINK: 销毁链接" << linkId;
        }
    }
}

void TcpServerManager::handleCreateIntrChan(QTcpSocket* client, const QByteArray &data, const quint32 &xid)
{
    // 参数：hostAddr(4), hostPort(2), progNum(4), progVers(4), progFamily(4)
    if (data.size() < 54) {
        QByteArray response = createErrorResponse(xid, Vxi11::PARAMETER_ERROR);
        client->write(response);
        return;
    }

    // 检查是否已建立中断通道
    for (auto it = m_deviceLinks.begin(); it != m_deviceLinks.end(); ++it) {
        if (it->client == client) {
            // 这里应该检查是否已为该client建立了中断通道
            // 简化实现：总是返回通道已建立
            QByteArray response = createErrorResponse(xid, Vxi11::CHANNEL_ALREADY_ESTABLISHED);
            client->write(response);
            return;
        }
    }

    qCDebug(tcp) << "CREATE_INTR_CHAN: 客户端请求创建中断通道";

    // 简化实现：不支持中断通道
    QByteArray response = createErrorResponse(xid, Vxi11::CHANNEL_NOT_ESTABLISHED);
    client->write(response);
}

void TcpServerManager::handleDestroyIntrChan(QTcpSocket* client, const QByteArray &data, const quint32 &xid)
{
    Q_UNUSED(data)

    qCDebug(tcp) << "DESTROY_INTR_CHAN: 客户端请求销毁中断通道";

    // 简化实现：没有中断通道可销毁
    QByteArray response = createErrorResponse(xid, Vxi11::CHANNEL_NOT_ESTABLISHED);
    client->write(response);
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


// 辅助函数：验证链接有效性
bool TcpServerManager::validateLink(QTcpSocket* client, quint32 lid, QByteArray& errorResponse, quint32 xid)
{
    if (!m_deviceLinks.contains(lid)) {
        errorResponse = createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        return false;
    }

    if (m_deviceLinks[lid].client != client) {
        errorResponse = createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        return false;
    }

    return true;
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

    m_deviceLinks.remove(linkId);

    qCDebug(tcp) << "✅ 销毁VXI-11链接:" << linkId << "client:" << clientInfo;

    if (link.client && link.client->state() == QAbstractSocket::ConnectedState) {
        qCDebug(tcp) << "链接" << linkId << "的client仍然连接，保持TCP连接";
    }
}


// ==================== 析构部分 ====================

TcpServerManager::~TcpServerManager()
{
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

    m_cleanupTimer->stop();
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



