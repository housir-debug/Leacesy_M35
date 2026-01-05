#include "tcpserver.h"
#include "canworker.h"
#include <QNetworkInterface>
#include <QDateTime>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QRandomGenerator>
#include <QHostInfo>
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
    initScpiCommandTree();

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

    qCInfo(tcp) << "Device IP:" << m_deviceIp;
}

void TcpServerManager::initScpiCommandTree()
{
    m_scpiRoot = new ScpiNode("ROOT");

    registerScpiCommand("*IDN",
        [this](const QStringList& args) {
            Q_UNUSED(args);
            return QString("%1,%2,%3,%4")
                   .arg(getManufacturer,getModel,getSerialNumber,getFirmwareVersion);
        },
        "Query device identification - NI MAX compatible");

    registerScpiCommand("*RST",
        [this](const QStringList& args) {
        handleScpiSystemReset(args);
        return "0";
        },
        "Reset system to default state");

    registerScpiCommand("*CLS",
        [this](const QStringList& args) {
            Q_UNUSED(args);
            m_scpiErrors.clear();
            return "0";
        },
        "Clear status");

    // *ESR? 事件状态寄存器查询
    registerScpiCommand("*ESR",
        [this](const QStringList& /*args*/) {
            return QString::number(m_scpiErrors.isEmpty() ? 0 : 1);
        },
        "Event status register query");


    registerScpiCommand("NETWork:IP",
        [this](const QStringList& /*args*/) {
            return m_deviceIp;
        },
        "Query IP address");

    qCInfo(tcp) << "SCPI command tree initialized with NI MAX compatible commands";
}

void TcpServerManager::registerScpiCommand(const QString& command,ScpiHandler handler,const QString& description)
{
    QStringList parts = command.split(':');
    ScpiNode* current = m_scpiRoot;

    for (const QString& part : qAsConst(parts)) {
        QString key = part.toUpper();
        if (!current->children.contains(key)) {
            current->children[key] = new ScpiNode(key);
        }
        current = current->children[key];
    }

    current->handler = handler;
    current->description = description;
}

void TcpServerManager::handleScpiSystemReset(const QStringList& args)
{
    Q_UNUSED(args);

    // 停止服务器
    stopServer();

    // 清空数据
    m_totalBytesSent.store(0);
    m_totalBytesReceived.store(0);
    m_scpiErrors.clear();

    // 重新启动
    QTimer::singleShot(1000, this, [this]() {
        startServer();
    });

    qCInfo(tcp) << "System reset performed, VXI-11 discovery will restart";
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

    qCInfo(tcp) << "New client connected:" << clientInfo
           << ", total clients:" << m_clients.size();;

    QByteArray welcome = QString("Welcome to Meacesy Server (Clients: %1)\n")
                        .arg(m_clients.size()).toUtf8();
    client->write(welcome);
}

void TcpServerManager::registerWithRpcbind()
{
    // 创建RPC SET请求：注册VXI-11服务
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        return;
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
    call.proc = htonl(3);            // SET过程
    call.map_prog = htonl(395183);   // VXI-11程序号
    call.map_vers = htonl(1);        // VXI-11版本1
    call.map_prot = htonl(6);        // TCP协议
    call.map_port = htonl(m_port);   // 我们的端口

    // 发送请求
    sendto(sockfd, &call, sizeof(call), 0,
           (struct sockaddr*)&addr, sizeof(addr));

    close(sockfd);
    qCDebug(tcp) << "Registered VXI-11 service on port" << m_port;
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
    qCDebug(tcp) << "Processing buffer for" << client->objectName()<< "size:" << newdata.size();

    if (m_vxi11Enabled && isVxi11RpcCall(newdata)) {
        qCDebug(tcp) << "VXI-11 RPC call detected";
        handleVxi11RpcCall(client, newdata);
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

    const uchar* rawData = reinterpret_cast<const uchar*>(data.constData());

    // 提取XID（前4字节）
    quint32 xid = qFromBigEndian<quint32>(rawData);

    // 提取过程号（偏移24字节）
    quint32 procedure = 0;
    if (data.size() >= 28) {
        procedure = qFromBigEndian<quint32>(rawData + 24);
    }

    qCDebug(tcp) << "VXI-11 RPC call, procedure:" << procedure;

    QByteArray response;

    switch (procedure) {
        case 3:  // CREATE_LINK
            response = buildVxi11Response(xid, procedure, 0);
            break;

        case 10: // DEVICE_WRITE
        case 11: // DEVICE_READ
            // 对于NI MAX发现，返回成功即可
            response = buildVxi11Response(xid, procedure, 0);
            break;

        default:
            // 返回程序不可用
            response = buildVxi11Response(xid, procedure, 1);
            break;
    }

    if (!response.isEmpty()) {
        client->write(response);
        qCDebug(tcp) << "Sent VXI-11 response, size:" << response.size();
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

// ==================== 销毁部分 ====================

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

    // 停止VXI-11发现服务
    if (m_discoveryTimer) {
        m_discoveryTimer->stop();
        delete m_discoveryTimer;
        m_discoveryTimer = nullptr;
    }

    if (m_vxi11UdpSocket) {
        m_vxi11UdpSocket->close();
        delete m_vxi11UdpSocket;
        m_vxi11UdpSocket = nullptr;
    }

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

