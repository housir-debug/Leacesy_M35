#include "tcp_server.h"
#include "tirpc_loader.h"
#include "auxiliary/vxinamespace.h"
#include <QTcpSocket>
#include <unistd.h>
#include <arpa/inet.h>

Q_LOGGING_CATEGORY(tcp, "TCP:")

TcpServerManager::TcpServerManager(QObject *parent):QObject(parent){}
TcpServerManager::~TcpServerManager()
{
    if (m_tcpServer) {
        for (QTcpSocket *client : qAsConst(m_clients)) {
            client->disconnectFromHost();
            client->waitForDisconnected(600);
        }

        m_clients.clear();
        m_tcpServer->close();
        delete m_tcpServer;
        m_tcpServer = nullptr;
    }

    if (m_serverThread) {
        m_serverThread->quit();
        m_serverThread->wait(1000); // wait 1s
        m_serverThread->deleteLater();
        delete m_serverThread;
        m_serverThread = nullptr;
    }

    qCDebug(tcp)<<"[~TcpServerManager]:TcpServerManager Destroyed!!!";
}

bool TcpServerManager::startServer()
{
    if (!m_serverThread && !m_tcpServer){
        m_tcpServer = new QTcpServer(this);
        m_serverThread = new QThread(this);
        m_serverThread->setObjectName("VxiServer");

        this->moveToThread(m_serverThread);
        m_tcpServer->moveToThread(m_serverThread);
        m_serverThread->start();

        connect(m_tcpServer,&QTcpServer::newConnection,this,[this](){
            QTcpSocket* client = m_tcpServer->nextPendingConnection();
            if (m_clients.size() <= 9) {
                client->setObjectName(QString("%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort()));
                qCDebug(tcp)<<"[startServer]:New Connected Client: "<<client->objectName();

                connect(client, &QTcpSocket::disconnected,this, [this, client](){
                    qCDebug(tcp)<<"[startServer]:Disconnected "<<client->objectName()<<" Remain Connect Clients: "<< m_clients.size()-1;
                    for (auto it = m_deviceLinks.begin(); it != m_deviceLinks.end(); ++it) {
                        if (it->client == client) { // client->objectName()
                            m_deviceLinks.erase(it);
                            break;
                        }
                    }

                    m_clients.removeOne(client);
                    client->deleteLater();
                }, Qt::DirectConnection);
                connect(client, &QTcpSocket::errorOccurred,this, [client](QAbstractSocket::SocketError error){
                    qCWarning(tcp)<<"[startServer]:ERROR Socket: ["<< client->objectName()<<"]"<< error <<client->errorString();
                }, Qt::DirectConnection);
                connect(client, &QTcpSocket::readyRead,this, [this, client](){
                    processClientData(client);
                }, Qt::DirectConnection);

                m_clients.append(client);
                return;
            }

            qCWarning(tcp)<<"[startServer]:Connected Clients Exceeds 9. Refused Clients:"<<client->peerAddress().toString();
            client->disconnectFromHost();
            client->deleteLater();
        }, Qt::DirectConnection);

        QMetaObject::invokeMethod(this, [this]() {
            if (m_tcpServer->listen(QHostAddress::Any, Vxi11::VXI_PORT)) {
                if (TirpcDynamicLoader::instance().load()) {
                    TirpcDynamicLoader::instance().smart_pmap_set(
                        Vxi11::DEVICE_CORE,         // VXI-11 program
                        1,                          // version
                        IPPROTO_TCP,                // TCP protocol
                        Vxi11::VXI_PORT
                    );
                    return;
                }}
            qCWarning(tcp)<<"[startServer]: m_tcpServer listen failed!";
        }, Qt::QueuedConnection);

        return true;
    }

    qCWarning(tcp)<<"[startServer]: already exist A certain member";
    return false;
}

void TcpServerManager::processClientData(QTcpSocket *client)
{
    QMutexLocker locker(&m_sycmutex);

    m_responsebuffer.clear();
    m_qmlbridge->update_remotemodel(true);
    m_readbuffer.append(client->readAll());
    qCDebug(tcp)<<"[processClientData]: "<<client->objectName()<<" Received Hex: "<< m_readbuffer.toHex(' ');

    // SOCKET ASCll Define(0x00-0x7F)
    QString message = QString::fromUtf8(m_readbuffer).trimmed();
    if (message.startsWith("*") || message.startsWith(":")) {
        qCDebug(tcp)<<"[processClientData]:SOCKET SCPI Request: "<<message;
        m_responsebuffer = m_scpiManager->processCommand(m_readbuffer);

        if (!m_responsebuffer.isEmpty()){
            qCDebug(tcp)<<"[processClientData]:SOCKET SCPI Response: "<<m_responsebuffer;
            client->write(m_responsebuffer);
        }}
    // VXI-11 0x800000 + length
    else if (m_readbuffer.size() > 44 && static_cast<quint8>(m_readbuffer[0]) == Vxi11::HEADER){
        const uchar* readAddress = reinterpret_cast<const uchar*>(m_readbuffer.constData());
        quint8 lengthB = static_cast<quint8>(m_readbuffer[3]);

        if (m_readbuffer.size() >= lengthB + 4){
            if(qFromBigEndian<quint32>(readAddress+8)==Vxi11::CALL && qFromBigEndian<quint32>(readAddress+16)==Vxi11::DEVICE_CORE){
                quint32 xid = qFromBigEndian<quint32>(readAddress + 4);
                quint32 procedure = qFromBigEndian<quint32>(readAddress + 24);
                qCDebug(tcp)<<"[processClientData]:VXI-11 Current Procedure: "<<procedure<<",XID: "<<xid;

                switch (procedure) {
                    case Vxi11::CREATE_LINK:           handleCreateLink      (client, xid);                  break;
                    case Vxi11::DEVICE_WRITE:          handleDeviceWrite     (client, xid, readAddress);     break;
                    case Vxi11::DEVICE_READ:           handleDeviceRead      (client, xid, readAddress);     break;
                    case Vxi11::DEVICE_READSTB:        handleDeviceReadStb   (client, xid, readAddress);     break;
                    case Vxi11::DEVICE_TRIGGER:        handleDeviceTrigger   (client, xid, readAddress);     break;
                    case Vxi11::DEVICE_CLEAR:          handleDeviceClear     (client, xid, readAddress);     break;
                    case Vxi11::DEVICE_REMOTE:         handleDeviceRemote    (client, xid, readAddress);     break;
                    case Vxi11::DEVICE_LOCAL:          handleDeviceLocal     (client, xid, readAddress);     break;
                    case Vxi11::DEVICE_LOCK:           handleDeviceLock      (client, xid, readAddress);     break;
                    case Vxi11::DEVICE_UNLOCK:         handleDeviceUnlock    (client, xid, readAddress);     break;
                    case Vxi11::DEVICE_ENABLE_SRQ:     handleDeviceEnableSrq (client, xid, readAddress);     break;
                    case Vxi11::DEVICE_DOCMD:          handleDeviceDocmd     (client, xid, readAddress);     break;
                    case Vxi11::DESTROY_LINK:          handleDestroyLink     (client, xid, readAddress);     break;
                    case Vxi11::CREATE_INTER_CHAN:     handleCreateIntrChan  (client, xid, readAddress);     break;
                    case Vxi11::DESTROY_INTER_CHAN:    handleDestroyIntrChan (client, xid, readAddress);     break;
                    default:qCWarning(tcp)<<"[processClientData]:Unknown VXI-11 Procedure: "<<procedure;     break;
                }

                m_readbuffer.remove(0,lengthB + 4); // delete process finish data.

                if(!m_readbuffer.isEmpty()){
                    qCDebug(tcp)<<"[processClientData]:VXI-11 continue next processing";
                    processClientData(client);
                }

                return; // finish.
            }

            m_readbuffer.clear(); // format error
        }

        return; // data not Complete. wait next information
    }

    m_readbuffer.clear();  // socket and format error
    return;
}

//---------------------------------------------------------------------------------

void TcpServerManager::handleCreateLink(QTcpSocket* client,const quint32 xid)
{
    for (auto it = m_deviceLinks.begin(); it != m_deviceLinks.end(); ++it) {
        if (it->client->peerAddress() == client->peerAddress()) {   // client->objectName()
            qCDebug(tcp)<<"[handleCreateLink]:Client: "<<client->objectName()<<" Existing Link: "<<it->id;
            buildfoundResponse(xid, Vxi11::CHANNEL_ALREADY_ESTABLISHED);
            client->write(m_responsebuffer);
            client->disconnectFromHost();
            return;
        }
    }

    DeviceLink link;
    link.client = client;
    link.id = m_nextLinkId++; // Assign the value first and then increment.
    m_deviceLinks.insert(link.id, link);
    qCDebug(tcp)<<"[handleCreateLink]:Create VXI-11 Link: "<<link.id<<" of Client:"<<client->objectName();

    buildfoundResponse(xid, Vxi11::NO_ERROR, 12);
    QDataStream stream(&m_responsebuffer, QIODevice::Append);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << link.id;
    stream << quint32(0);     // abort_port .Not Support -> 0
    stream << quint32(2048);  // max_recv_size

    client->write(m_responsebuffer);
    qCDebug(tcp)<<"[handleCreateLink]:CREATE_LINK: "<<link.id<<" Response: "<<m_responsebuffer.toHex(' ');
}

int TcpServerManager::checkRequestlinkid(QTcpSocket* client,const quint32 xid,const uchar* address)
{
    if(client){
        quint8 lid = qFromBigEndian<quint32>(address + 44);
        if (m_deviceLinks.contains(lid) && m_deviceLinks[lid].client == client) {
            return lid;
        }

        buildfoundResponse(xid, Vxi11::INVALID_LINK_IDENTIFIER);
        client->write(m_responsebuffer);
    }

    return 0;
}

void TcpServerManager::handleDeviceWrite(QTcpSocket* client,const quint32 xid,const uchar* address)
{
    quint8 lid = checkRequestlinkid(client,xid,address);

    if (lid !=0) {
        quint32 cmdlen = qFromBigEndian<quint32>(address + 60);
        QByteArray scpicmd = m_readbuffer.mid(64, cmdlen); // read scpi command
        qCDebug(tcp)<<"[handleDeviceWrite]:client: "<<client->objectName()<<" Received SCPI Command: "<<scpicmd;

        DeviceLink& link = m_deviceLinks[lid];
        link.VxiScpi_response = m_scpiManager->processCommand(scpicmd);

        buildfoundResponse(xid, Vxi11::NO_ERROR, 4);
        QDataStream stream(&m_responsebuffer, QIODevice::Append);
        stream.setByteOrder(QDataStream::BigEndian);
        stream << cmdlen; // write_size

        client->write(m_responsebuffer);
        qCDebug(tcp)<<"[handleDeviceWrite]:DEVICE_WRITE: "<<client->objectName()<<" Response: "<<m_responsebuffer.toHex(' ');
    }
}

void TcpServerManager::handleDeviceRead(QTcpSocket* client,const quint32 xid,const uchar* address)
{
    quint8 lid = checkRequestlinkid(client,xid,address);

    if (lid !=0) {
        DeviceLink& link = m_deviceLinks[lid];
        qCDebug(tcp)<<"[handleDeviceRead]:client: "<<client->objectName()<<" Return SCPI Response: "<<link.VxiScpi_response;

        if (!link.VxiScpi_response.isEmpty()){
            buildfoundResponse(xid, Vxi11::NO_ERROR, 8 + link.VxiScpi_response.size());
            QDataStream stream(&m_responsebuffer, QIODevice::Append);
            stream.setByteOrder(QDataStream::BigEndian);
            stream << quint32(4);              // END_FLAG
            stream << link.VxiScpi_response;   // QDataStream will write the length prefix for QByteArray.

            client->write(m_responsebuffer);
            qCDebug(tcp)<<"[handleDeviceRead]:DEVICE_READ: "<<client->objectName()<<" Response:"<<m_responsebuffer.toHex(' ');
        }
    }
}

void TcpServerManager::handleDeviceReadStb(QTcpSocket* client, const quint32 xid,const uchar* address)
{
    quint8 lid = checkRequestlinkid(client,xid,address);

    if (lid !=0) {
        bool ok;
        DeviceLink& link = m_deviceLinks[lid];
        link.VxiScpi_response = m_scpiManager->processCommand("*STB?\n");
        quint32 statusByte = link.VxiScpi_response.trimmed().toInt(&ok);

        if (!ok){
            qCWarning(tcp) << "[handleDeviceReadStb]:DEVICE_READSTB: Failed to parse response:" << link.VxiScpi_response;
            return;
        }

        buildfoundResponse(xid, Vxi11::NO_ERROR, 4);
        QDataStream stream(&m_responsebuffer, QIODevice::Append);
        stream.setByteOrder(QDataStream::BigEndian);
        stream << statusByte; // status bytes

        client->write(m_responsebuffer);
        qCDebug(tcp)<<"[handleDeviceReadStb]:DEVICE_READSTB: "<<client->objectName()<<" Response: "<<m_responsebuffer.toHex(' ');
    }
}

void TcpServerManager::handleDeviceTrigger(QTcpSocket* client, const quint32 xid,const uchar* address)
{
    // Repeats the SCPI instruction. Not supported.
    quint8 lid = checkRequestlinkid(client,xid,address);

    if (lid !=0) {
        qCDebug(tcp)<<"[handleDeviceTrigger]:DEVICE_TRIGGER: "<<client->objectName();
        buildfoundResponse(xid, Vxi11::OPERATION_NOT_SUPPORTED);
        client->write(m_responsebuffer);
    }
}

void TcpServerManager::handleDeviceClear(QTcpSocket* client, const quint32 xid,const uchar* address)
{
    quint8 lid = checkRequestlinkid(client,xid,address);

    if (lid !=0) {
        // Clear buffer zone
        m_readbuffer.clear();
        m_responsebuffer.clear();

        // Terminate operation and return to idle state
        m_scpiManager->processCommand("*CLS\n");
        m_scpiManager->processCommand("ABORt\n");
        /*m_scpiManager->processCommand("*RST\n");*/

        qCDebug(tcp)<<"[handleDeviceClear]:DEVICE_CLEAR: "<<client->objectName();
        buildfoundResponse(xid, Vxi11::NO_ERROR);
        client->write(m_responsebuffer);
    }
}

void TcpServerManager::handleDeviceRemote(QTcpSocket* client, const quint32 xid,const uchar* address)
{
    quint8 lid = checkRequestlinkid(client,xid,address);

    if (lid !=0) {
        m_qmlbridge->update_remotemodel(true);

        qCDebug(tcp)<<"[handleDeviceRemote]:DEVICE_REMOTE: "<<client->objectName();
        buildfoundResponse(xid, Vxi11::NO_ERROR);
        client->write(m_responsebuffer);
    }
}

void TcpServerManager::handleDeviceLocal(QTcpSocket* client, const quint32 xid,const uchar* address)
{
    quint8 lid = checkRequestlinkid(client,xid,address);

    if (lid !=0) {
        m_qmlbridge->update_remotemodel(false);

        qCDebug(tcp)<<"[handleDeviceLocal]:DEVICE_LOCAL: "<<client->objectName();
        buildfoundResponse(xid, Vxi11::NO_ERROR);
        client->write(m_responsebuffer);
    }
}

void TcpServerManager::handleDeviceLock(QTcpSocket* client, const quint32 xid,const uchar* address)
{
    // Natural mutual exclusivity, no need for additional exclusive
    quint8 lid = checkRequestlinkid(client,xid,address);

    if (lid !=0) {
        qCDebug(tcp)<<"[handleDeviceLock]:DEVICE_LOCK: "<<client->objectName();
        buildfoundResponse(xid, Vxi11::NO_ERROR);
        client->write(m_responsebuffer);
    }
}

void TcpServerManager::handleDeviceUnlock(QTcpSocket* client, const quint32 xid,const uchar* address)
{
    // Natural mutual exclusivity, no need for additional exclusive
    quint8 lid = checkRequestlinkid(client,xid,address);

    if (lid !=0) {
        qCDebug(tcp)<<"[handleDeviceUnlock]:DEVICE_UNLOCK: "<<client->objectName();
        buildfoundResponse(xid, Vxi11::NO_ERROR);
        client->write(m_responsebuffer);
    }
}

void TcpServerManager::handleDeviceEnableSrq(QTcpSocket* client, const quint32 xid,const uchar* address)
{
    // Srq .Not supported for the time being.
    quint8 lid = checkRequestlinkid(client,xid,address);

    if (lid !=0) {
        qCDebug(tcp)<<"[handleDeviceEnableSrq]:DEVICE_ENABLE_SRQ: "<<client->objectName();
        buildfoundResponse(xid, Vxi11::OPERATION_NOT_SUPPORTED);
        client->write(m_responsebuffer);
    }
}

void TcpServerManager::handleDeviceDocmd(QTcpSocket* client,const quint32 xid,const uchar* address)
{
    // No specific instructions provided. Not supported.
    quint8 lid = checkRequestlinkid(client,xid,address);

    if (lid !=0) {
        qCDebug(tcp)<<"[handleDeviceDocmd]:DEVICE_DOCMD: "<<client->objectName();
        buildfoundResponse(xid, Vxi11::OPERATION_NOT_SUPPORTED);
        client->write(m_responsebuffer);
    }
}

void TcpServerManager::handleDestroyLink(QTcpSocket* client,const quint32 xid,const uchar* address)
{
    quint8 lid = checkRequestlinkid(client,xid,address);

    if (lid !=0) {
        qCDebug(tcp)<<"[handleDestroyLink]:DESTROY_LINK: "<<client->objectName();
        buildfoundResponse(xid, Vxi11::NO_ERROR);
        client->write(m_responsebuffer);
        client->disconnectFromHost();
        //disconnect -> remove m_deviceLinks[lid]
    }
}

void TcpServerManager::handleCreateIntrChan(QTcpSocket* client, const quint32 xid,const uchar* address)
{
    // Srq .Not supported for the time being.
    quint8 lid = checkRequestlinkid(client,xid,address);

    if (lid !=0) {
        qCDebug(tcp)<<"[handleCreateIntrChan]:CREATE_INTR_CHAN: "<<client->objectName();
        buildfoundResponse(xid, Vxi11::OPERATION_NOT_SUPPORTED);
        client->write(m_responsebuffer);
    }
}

void TcpServerManager::handleDestroyIntrChan(QTcpSocket* client, const quint32 xid,const uchar* address)
{
    // Srq .Not supported for the time being.
    quint8 lid = checkRequestlinkid(client,xid,address);

    if (lid !=0) {
        qCDebug(tcp)<<"[handleDestroyIntrChan]:DESTROY_INTR_CHAN: "<<client->objectName();
        buildfoundResponse(xid, Vxi11::OPERATION_NOT_SUPPORTED);
        client->write(m_responsebuffer);
    }
}

void TcpServerManager::buildfoundResponse(quint32 xid,quint32 error,quint32 extraleng)
{
    QDataStream stream(&m_responsebuffer, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    quint32 fragHead = 0x8000001c + extraleng; // (0x80000000 + 7*4) + extraLen
    m_responsebuffer.reserve(32 + extraleng);
    stream << fragHead;
    stream << xid;
    stream << quint32(Vxi11::REPLY);
    stream << quint32(Vxi11::MSG_ACCEPTED);
    stream << quint32(Vxi11::AUTH_NONE);
    stream << quint32(Vxi11::VERF_LENG);
    stream << quint32(Vxi11::SUCCESS);
    stream << error;
}
