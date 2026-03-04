#include "tcp_server.h"
#include "tirpc_loader.h"
#include <QtCore>
#include <QTcpSocket>
#include <unistd.h>
#include <arpa/inet.h>
//#include <QRandomGenerator>
//#include <QtEndian>
//#include <sys/socket.h>

Q_LOGGING_CATEGORY(tcp, "TCP:")

TcpServerManager::TcpServerManager(ScpiManager* scpi,QObject *parent): QObject(parent), m_scpiManager(scpi) {}
TcpServerManager::~TcpServerManager()
{
    qCDebug(tcp)<<"TcpServerManager Destroyed!!!";
    delete m_scpiManager;
    m_scpiManager = nullptr;

    m_cleanupTimer->stop();
    delete m_cleanupTimer;
    m_cleanupTimer = nullptr;

    for (QTcpSocket *client : qAsConst(m_clients)) {
        client->disconnectFromHost();
        if (client->state() != QAbstractSocket::UnconnectedState) {
            client->waitForDisconnected(1000);
        }
    }
    m_clients.clear();
    m_tcpServer->close();
    delete m_tcpServer;
    m_tcpServer = nullptr;

    if (m_serverThread) {
        m_serverThread->quit();
        m_serverThread->wait(1000);// wait 1s
        m_serverThread->deleteLater();
        delete m_serverThread;
        m_serverThread = nullptr;
    }
}

void TcpServerManager::send_toAllClients(const QByteArray &data,bool isforce)
{
    for (QTcpSocket *client : qAsConst(m_clients)) {
        if (client->state() == QAbstractSocket::ConnectedState) {
            int result = client->write(data);
            if (result != data.size()) {
                qCWarning(tcp)<<"TCPserver Written Buffer Overflow!!!" << client->objectName();
            }

            if(isforce){
                client->flush();   // The same event is sent multiple times, and multiple messages will be sent together.
                qCDebug(tcp)<<"TCPserver Force Flush ";
                QThread::msleep(6);
            }
        }
    }
}

// ===================== 启动部分 =================================

bool TcpServerManager::startServer()
{
    if (!m_serverThread){
        m_tcpServer = new QTcpServer(this);

        m_cleanupTimer = new QTimer(this);
        m_cleanupTimer->setInterval(60000*6); // 60s * 6 = 6 minutes

        m_serverThread = new QThread(this);
        m_serverThread->setObjectName("TcpServer");
    }

    if (thread() != m_serverThread) {
        this->moveToThread(m_serverThread);
        m_tcpServer->moveToThread(m_serverThread);
        m_scpiManager->moveToThread(m_serverThread);
        m_cleanupTimer->moveToThread(m_serverThread);
    }

    if (!m_serverThread->isRunning()) {
        m_serverThread->start();

        connect(m_cleanupTimer, &QTimer::timeout,this, [this]{
            for (int i = this->m_clients.size()-1; i >= 0; --i) {
                QTcpSocket* client = this->m_clients.at(i);
                if (client->state() != QAbstractSocket::ConnectedState) {
                    qCDebug(tcp)<<"Cleaned Disconnected Client:"<<client->objectName();
                    this->m_clients.removeAt(i);
                    client->deleteLater();
                }
            }
        }, Qt::DirectConnection);
        connect(m_tcpServer, &QTcpServer::newConnection,this, &TcpServerManager::onNewConnection, Qt::DirectConnection);

        QMetaObject::invokeMethod(this, [this]() {
            if (m_tcpServer->listen(QHostAddress::Any, m_vxiport)) {
                if (!registerWithRpcbind()){qCWarning(tcp)<<"Rpcbind(111) Register Failed!!!";}
                m_cleanupTimer->start();
            }
        }, Qt::QueuedConnection);

        return true;
    }
    return false;
}

bool TcpServerManager::registerWithRpcbind()
{
    if (!TirpcDynamicLoader::instance().load()) {return false;}

    bool result = TirpcDynamicLoader::instance().smart_pmap_set(
        Vxi11::DEVICE_CORE,         // VXI-11 program
        1,                          // version
        IPPROTO_TCP,                // TCP protocol
        m_vxiport
    );

    return result;
}

void TcpServerManager::onNewConnection()
{
    QTcpSocket *client = m_tcpServer->nextPendingConnection();
    if (m_clients.size() >= 10) {
        qCWarning(tcp)<<"Connected Clients Exceeds Limit. Refused Connection of :"<<client->peerAddress().toString();
        client->disconnectFromHost();
        delete client;
        return;
    }

    m_clients.append(client);
    client->setObjectName(QString("%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort()));
    qCDebug(tcp)<<"New Connected Client: "<<client->objectName()<<", Total Clients: "<<m_clients.size();

    connect(client, &QTcpSocket::errorOccurred,this, [client](QAbstractSocket::SocketError error){
        qCWarning(tcp)<<client->objectName()<<" Socket Error: ["<<error<<"]"<<client->errorString();
    }, Qt::DirectConnection);
    connect(client, &QTcpSocket::disconnected,this, [this, client](){
        qCDebug(tcp)<<client->objectName()<<" Disconnected, Remain Connect Clients: "<<m_clients.size();
        m_clients.removeOne(client);
        client->deleteLater();
    }, Qt::DirectConnection);
    connect(client, &QTcpSocket::readyRead,this, [this, client](){ processClientData(client);}, Qt::DirectConnection);
}

void TcpServerManager::processClientData(QTcpSocket *client)
{
    m_readbuffer.append(client->readAll());
    if (m_readbuffer.isEmpty()){return;}

    qCDebug(tcp)<<client->objectName()<<" Received Hex: "<<m_readbuffer.toHex(' ');

    QString message = QString::fromUtf8(m_readbuffer).trimmed();
    if (message.startsWith("*") || message.contains(":")) {     // SOCKET ASCll Define(0x00-0x7F)
        qCDebug(tcp)<<"SOCKET SCPI Request Commend: "<<message;
        QByteArray response = m_scpiManager->processCommand(m_readbuffer);
        qCDebug(tcp)<<"SOCKET SCPI Response: "<<response;
        if (!response.isEmpty()){ client->write(response);}
        m_readbuffer.clear();
        return;
    }

    if (m_readbuffer.size() > 44){
        if (static_cast<quint8>(m_readbuffer[0]) == Vxi11::HEADER){ // VXI-11 0x80
            quint8 lengthB = static_cast<quint8>(m_readbuffer[3]);
            if (m_readbuffer.size() < lengthB + 4){return;}

            const uchar* readAddress = reinterpret_cast<const uchar*>(m_readbuffer.constData());
            if (qFromBigEndian<quint32>(readAddress+8)==Vxi11::CALL && qFromBigEndian<quint32>(readAddress+16)==Vxi11::DEVICE_CORE){
                handleVxi11RpcCall(client,readAddress);
                m_readbuffer.remove(0,lengthB + 4);
                if(!m_readbuffer.isEmpty()){processClientData(client);}
                return;
            }
        }

        qCWarning(tcp)<<client->objectName()<<" Received Format Error!!!";
        m_readbuffer.clear();
        return;
    }
}

void TcpServerManager::handleVxi11RpcCall(QTcpSocket* client,const uchar* address)
{
    quint32 xid = qFromBigEndian<quint32>(address + 4);
    quint32 procedure = qFromBigEndian<quint32>(address + 24);
    qCDebug(tcp)<<"VXI-11 Current Procedure: "<<procedure<<",XID: "<<xid;

    switch (procedure) {
        case Vxi11::CREATE_LINK:
            return handleCreateLink      (client, xid);

        case Vxi11::DEVICE_WRITE:
            return handleDeviceWrite     (client, xid, address);

        case Vxi11::DEVICE_READ:
            return handleDeviceRead      (client, xid, address);

        case Vxi11::DEVICE_READSTB:
            return handleDeviceReadStb   (client, xid, address);

        case Vxi11::DEVICE_TRIGGER:
            return handleDeviceTrigger   (client, xid, address);

        case Vxi11::DEVICE_CLEAR:
            return handleDeviceClear     (client, xid, address);

        case Vxi11::DEVICE_REMOTE:
            return handleDeviceRemote    (client, xid, address);

        case Vxi11::DEVICE_LOCAL:
            return handleDeviceLocal     (client, xid, address);

        case Vxi11::DEVICE_LOCK:
            return handleDeviceLock      (client, xid, address);

        case Vxi11::DEVICE_UNLOCK:
            return handleDeviceUnlock    (client, xid, address);

        case Vxi11::DEVICE_ENABLE_SRQ:
            return handleDeviceEnableSrq (client, xid, address);

        case Vxi11::DEVICE_DOCMD:
            return handleDeviceDocmd     (client, xid, address);

        case Vxi11::DESTROY_LINK:
            return handleDestroyLink     (client, xid, address);

        case Vxi11::CREATE_INTR_CHAN:
            return handleCreateIntrChan  (client, xid, address);

        case Vxi11::DESTROY_INTR_CHAN:
            return handleDestroyIntrChan (client, xid, address);

        default:
            qCWarning(tcp)<<"Unknown VXI-11 Procedure: "<<procedure;
            return;
    }
}

// ===================== VXI-11 RPC处理 =================================

void TcpServerManager::handleCreateLink(QTcpSocket* client,const quint32 xid)
{
    for (auto it = m_deviceLinks.begin(); it != m_deviceLinks.end(); ++it) {
        if (it->client->peerAddress() == client->peerAddress()) {
            qCWarning(tcp)<<"Client: "<<client->objectName()<<" Existing Link : "<<it->id;
            createErrorResponse(xid, Vxi11::CHANNEL_ALREADY_ESTABLISHED);
            client->write(m_responsebuffer);
            m_responsebuffer.clear();
            return;
        }
    }

    DeviceLink link;
    link.client = client;
    link.id = m_nextLinkId++;
    link.createTime = QDateTime::currentDateTime();
    m_deviceLinks.insert(link.id, link);
    qCDebug(tcp)<<"Create VXI-11 Link: "<<link.id<<" of Client:"<<client->objectName();

    QDataStream stream(&m_responsebuffer, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    m_responsebuffer.reserve(44);

    stream << quint32(0x80000028);     // RPC + size
    stream << xid;                     // XID
    stream << quint32(1);              // msg_type: REPLY (1)
    stream << quint32(0);              // reply_stat: MSG_ACCEPTED (0)
    stream << quint32(0);              // auth_flavor: AUTH_NULL (0)
    stream << quint32(0);              // auth_length: 0
    stream << quint32(0);              // accept_stat: SUCCESS (0)
    stream << quint32(0);              // error_code: 0 (no error)
    stream << link.id;                 // link_id (Device_Link ID)
    stream << quint32(0);              // abort_port
    stream << quint32(2048);           // max_recv_size

    qCDebug(tcp)<<"CREATE_LINK: "<<link.id<<" Response: "<<m_responsebuffer.toHex(' ');
    client->write(m_responsebuffer);
    m_responsebuffer.clear();
}

void TcpServerManager::handleDeviceWrite(QTcpSocket* client,const quint32 xid,const uchar* address)
{
    quint32 lid = qFromBigEndian<quint32>(address + 44);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(m_responsebuffer);
        m_responsebuffer.clear();
        return;
    }

    quint32 cmdlen = qFromBigEndian<quint32>(address + 60);
    if (cmdlen <= 0){return;};
    QByteArray scpicmd = m_readbuffer.mid(64, cmdlen);
    qCDebug(tcp)<<"VXI-11 Link: " <<lid<<" Received SCPI Command: "<<QString::fromUtf8(scpicmd).trimmed();

    DeviceLink& link = m_deviceLinks[lid];
    link.VxiScpi_response = m_scpiManager->processCommand(scpicmd);

    QDataStream stream(&m_responsebuffer, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    m_responsebuffer.reserve(36);

    stream << quint32(0x80000020);     // RPC头部长度
    stream << xid;
    stream << quint32(1);              // REPLY
    stream << quint32(0);              // MSG_ACCEPTED
    stream << quint32(0);              // AUTH_NULL
    stream << quint32(0);              // AUTH_LENGTH
    stream << quint32(0);              // SUCCESS
    stream << quint32(0);              // error_code: NO_ERROR
    stream << cmdlen;                  // write_size

    qCDebug(tcp)<<"DEVICE_WRITE: "<<lid<<" Response: "<<m_responsebuffer.toHex(' ');
    client->write(m_responsebuffer);
    m_responsebuffer.clear();
}

void TcpServerManager::handleDeviceRead(QTcpSocket* client,const quint32 xid,const uchar* address)
{
    quint32 lid = qFromBigEndian<quint32>(address + 44);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(m_responsebuffer);
        m_responsebuffer.clear();
        return;
    }

    DeviceLink& link = m_deviceLinks[lid];
    if (link.VxiScpi_response.isEmpty()){ return;}
    qCDebug(tcp)<<"VXI-11 Link: "<<lid<<" Return Response: "<<link.VxiScpi_response;

    quint32 requestSize = qFromBigEndian<quint32>(address + 48);
    quint32 reason = 0x00000004;
    if (requestSize > 0 && link.VxiScpi_response.size() > static_cast<int>(requestSize)) {
        link.VxiScpi_response.resize(requestSize);
        reason = 0x00000001;  // REQCNT:
    }

    quint32 alignedLen = (link.VxiScpi_response.size() + 3) & ~3;
    link.VxiScpi_response.append(alignedLen - link.VxiScpi_response.size(), '\0');
    int totallen = 40+alignedLen;
    quint32 fragHead = 0x80000000 | (totallen - 4);

    QDataStream stream(&m_responsebuffer, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    m_responsebuffer.reserve(totallen);

    stream << fragHead;                // RPC头部长度
    stream << xid;
    stream << quint32(1);              // REPLY
    stream << quint32(0);              // MSG_ACCEPTED
    stream << quint32(0);              // AUTH_NULL
    stream << quint32(0);              // AUTH_LENGTH
    stream << quint32(0);              // SUCCESS
    stream << quint32(0);              // error_code: NO_ERROR
    stream << reason;                  // END_FLAG
    stream << link.VxiScpi_response;   // QDataStream 会为 QByteArray 写入长度前缀

    link.VxiScpi_response.clear();
    qCDebug(tcp)<<"DEVICE_READ: "<<lid<<" Response:"<<m_responsebuffer.toHex(' ');
    client->write(m_responsebuffer);
    m_responsebuffer.clear();
}

void TcpServerManager::handleDeviceReadStb(QTcpSocket* client, const quint32 xid,const uchar* address)
{
    quint32 lid = qFromBigEndian<quint32>(address + 44);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(m_responsebuffer);
        m_responsebuffer.clear();
        return;
    }

    QDataStream stream(&m_responsebuffer, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    m_responsebuffer.reserve(36);

    stream << quint32(0x80000020);     // RPC头部长度
    stream << xid;
    stream << quint32(1);              // REPLY
    stream << quint32(0);              // MSG_ACCEPTED
    stream << quint32(0);              // AUTH_NULL
    stream << quint32(0);              // AUTH_LENGTH
    stream << quint32(0);              // SUCCESS
    stream << quint32(0);              // error: NO_ERROR
    stream << quint32(0);              // 状态字节 (0 = 设备就绪)

    qCDebug(tcp)<<"DEVICE_READSTB: "<<lid<<" Response: "<<m_responsebuffer.toHex(' ');
    client->write(m_responsebuffer);
    m_responsebuffer.clear();
}

void TcpServerManager::handleDeviceTrigger(QTcpSocket* client, const quint32 xid,const uchar* address)
{
    quint32 lid = qFromBigEndian<quint32>(address + 44);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(m_responsebuffer);
        m_responsebuffer.clear();
        return;
    }

    qCDebug(tcp)<<"DEVICE_TRIGGER: "<<lid<<" Received Trigger Signal";
    createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(m_responsebuffer);
    m_responsebuffer.clear();
}

void TcpServerManager::handleDeviceClear(QTcpSocket* client, const quint32 xid,const uchar* address)
{
    quint32 lid = qFromBigEndian<quint32>(address + 44);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(m_responsebuffer);
        m_responsebuffer.clear();
        return;
    }

    qCDebug(tcp)<<"DEVICE_CLEAR: "<<lid<<" Perform Deletion Operation";
    createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(m_responsebuffer);
    m_responsebuffer.clear();
}

void TcpServerManager::handleDeviceRemote(QTcpSocket* client, const quint32 xid,const uchar* address)
{
    quint32 lid = qFromBigEndian<quint32>(address + 44);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(m_responsebuffer);
        m_responsebuffer.clear();
        return;
    }

    qCDebug(tcp)<<"DEVICE_REMOTE: "<<lid<<" Set Remote Mode";
    emit is_Remotemodel(true);
    createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(m_responsebuffer);
    m_responsebuffer.clear();
}

void TcpServerManager::handleDeviceLocal(QTcpSocket* client, const quint32 xid,const uchar* address)
{
    quint32 lid = qFromBigEndian<quint32>(address + 44);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(m_responsebuffer);
        m_responsebuffer.clear();
        return;
    }

    qCDebug(tcp)<<"DEVICE_LOCAL: "<<lid<<" Set Local Mode";
    emit is_Remotemodel(false);
    createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(m_responsebuffer);
    m_responsebuffer.clear();
}

void TcpServerManager::handleDeviceLock(QTcpSocket* client, const quint32 xid,const uchar* address)
{
    quint32 lid = qFromBigEndian<quint32>(address + 44);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(m_responsebuffer);
        m_responsebuffer.clear();
        return;
    }

    DeviceLink& link = m_deviceLinks[lid];
    if (link.lock) {
        createErrorResponse(xid, Vxi11::DEVICE_LOCKED_BY_ANOTHER_LINK);
        client->write(m_responsebuffer);
        m_responsebuffer.clear();
        return;
    }

    link.lock = true;
    qCDebug(tcp)<<"DEVICE_LOCK: "<<lid<<" Lock Device";
    createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(m_responsebuffer);
    m_responsebuffer.clear();
}

void TcpServerManager::handleDeviceUnlock(QTcpSocket* client, const quint32 xid,const uchar* address)
{
    quint32 lid = qFromBigEndian<quint32>(address + 44);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(m_responsebuffer);
        m_responsebuffer.clear();
        return;
    }

    DeviceLink& link = m_deviceLinks[lid];
    if (!link.lock) {
        createErrorResponse(xid, Vxi11::NO_LOCK_HELD_BY_THIS_LINK);
        client->write(m_responsebuffer);
        m_responsebuffer.clear();
        return;
    }

    link.lock = false;
    qCDebug(tcp)<<"DEVICE_UNLOCK: "<<lid<<" Unlock Deivce";
    createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(m_responsebuffer);
    m_responsebuffer.clear();
}

void TcpServerManager::handleDeviceEnableSrq(QTcpSocket* client, const quint32 xid,const uchar* address)
{
    quint32 lid = qFromBigEndian<quint32>(address + 44);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(m_responsebuffer);
        m_responsebuffer.clear();
        return;
    }

    quint32 enable = qFromBigEndian<quint32>(address + 40);
    qCDebug(tcp)<<"DEVICE_ENABLE_SRQ: "<<lid<<(enable ? "Enable" : "Disable") << "Server Request";
    createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(m_responsebuffer);
    m_responsebuffer.clear();
}

void TcpServerManager::handleDeviceDocmd(QTcpSocket* client,const quint32 xid,const uchar* address)
{
    quint32 lid = qFromBigEndian<quint32>(address + 44);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(m_responsebuffer);
        m_responsebuffer.clear();
        return;
    }

    qCDebug(tcp)<<"DEVICE_DOCMD: "<<lid<<" Handling Specific Commands Equipment";
    createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(m_responsebuffer);
    m_responsebuffer.clear();
}

void TcpServerManager::handleDestroyLink(QTcpSocket* client,const quint32 xid,const uchar* address)
{
    quint32 lid = qFromBigEndian<quint32>(address + 44);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(m_responsebuffer);
        m_responsebuffer.clear();
        return;
    }

    qCWarning(tcp)<<"Client: "<<client->objectName()<<" Disconnect, Delete Link:"<<lid;
    client->disconnectFromHost();
    m_deviceLinks.remove(lid);
    m_nextLinkId--;

    createErrorResponse(xid, Vxi11::NO_ERROR);
    client->write(m_responsebuffer);
    m_responsebuffer.clear();
}

void TcpServerManager::handleCreateIntrChan(QTcpSocket* client, const quint32 xid,const uchar* address)
{
    quint32 lid = qFromBigEndian<quint32>(address + 44);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(m_responsebuffer);
        m_responsebuffer.clear();
        return;
    }

    qCDebug(tcp)<<"CREATE_INTR_CHAN: "<<lid<<" Request Establish Relay Channel";
    createErrorResponse(xid, Vxi11::CHANNEL_NOT_ESTABLISHED);
    client->write(m_responsebuffer);
    m_responsebuffer.clear();
}

void TcpServerManager::handleDestroyIntrChan(QTcpSocket* client, const quint32 xid,const uchar* address)
{
    quint32 lid = qFromBigEndian<quint32>(address + 44);
    if (!m_deviceLinks.contains(lid) || m_deviceLinks[lid].client != client) {
        createErrorResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(m_responsebuffer);
        m_responsebuffer.clear();
        return;
    }

    qCDebug(tcp)<<"DESTROY_INTR_CHAN: "<<lid<<" Request Destroy Relay Channel";
    createErrorResponse(xid, Vxi11::CHANNEL_NOT_ESTABLISHED);
    client->write(m_responsebuffer);
    m_responsebuffer.clear();
}

void TcpServerManager::createErrorResponse(quint32 xid, quint32 error)
{
    QDataStream stream(&m_responsebuffer, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    m_responsebuffer.reserve(32);

    stream << quint32(0x8000001c);;     // RPC头部长度
    stream << xid;
    stream << quint32(1);               // msg_type: REPLY
    stream << quint32(0);               // reply_stat: MSG_ACCEPTED
    stream << quint32(0);               // verf_flavor: AUTH_NULL
    stream << quint32(0);               // verf_length: 0
    stream << quint32(0);               // accept_stat: SUCCESS
    stream << error;                    // error:
}
