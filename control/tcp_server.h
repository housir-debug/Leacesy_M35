#pragma once
#include <QTcpServer>
#include <QDataStream>
#include "auxiliary/scpi_handle.h"
#include "auxiliary/qml_agency.h"

Q_DECLARE_LOGGING_CATEGORY(tcp)

struct DeviceLink {
    QTcpSocket* client;
    quint32 id;
    QByteArray VxiScpi_response;

    DeviceLink(): client(nullptr), id(0) {}
};

class TcpServerManager : public QObject
{
    Q_OBJECT

public:
    explicit TcpServerManager(QObject *parent = nullptr);
    ~TcpServerManager();

    bool startServer();

    std::shared_ptr<GuiBridge> m_qmlbridge{nullptr};
    std::shared_ptr<ScpiManager> m_scpiManager{nullptr};

private:
    void processClientData      (QTcpSocket* client);
    void handleCreateLink       (QTcpSocket* client,const quint32 xid);
    int  checkRequestlinkid     (QTcpSocket* client,const quint32 xid,const uchar* address);
    void handleDeviceWrite      (QTcpSocket* client,const quint32 xid,const uchar* address);
    void handleDeviceRead       (QTcpSocket* client,const quint32 xid,const uchar* address);
    void handleDeviceReadStb    (QTcpSocket* client,const quint32 xid,const uchar* address);
    void handleDeviceTrigger    (QTcpSocket* client,const quint32 xid,const uchar* address);
    void handleDeviceClear      (QTcpSocket* client,const quint32 xid,const uchar* address);
    void handleDeviceRemote     (QTcpSocket* client,const quint32 xid,const uchar* address);
    void handleDeviceLocal      (QTcpSocket* client,const quint32 xid,const uchar* address);
    void handleDeviceLock       (QTcpSocket* client,const quint32 xid,const uchar* address);
    void handleDeviceUnlock     (QTcpSocket* client,const quint32 xid,const uchar* address);
    void handleDeviceEnableSrq  (QTcpSocket* client,const quint32 xid,const uchar* address);
    void handleDeviceDocmd      (QTcpSocket* client,const quint32 xid,const uchar* address);
    void handleDestroyLink      (QTcpSocket* client,const quint32 xid,const uchar* address);
    void handleCreateIntrChan   (QTcpSocket* client,const quint32 xid,const uchar* address);
    void handleDestroyIntrChan  (QTcpSocket* client,const quint32 xid,const uchar* address);
    void buildfoundResponse     (quint32 xid,quint32 error,quint32 extraleng = 0);

private:
    quint32 m_nextLinkId{1};
    QMap<quint8, DeviceLink> m_deviceLinks;

    QMutex m_sycmutex;
    QByteArray m_readbuffer;
    QByteArray m_responsebuffer;

    QList<QTcpSocket*> m_clients;
    QTcpServer *m_tcpServer{nullptr};
    QThread *m_serverThread{nullptr};
};

