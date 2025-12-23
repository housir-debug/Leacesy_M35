#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QMutex>
#include <QTimer>
#include <atomic>
#include <memory>
#include <QLoggingCategory>


Q_DECLARE_LOGGING_CATEGORY(tcp)

class TcpServerManager : public QObject
{
    Q_OBJECT

public:
    explicit TcpServerManager(QObject *parent = nullptr);
    ~TcpServerManager();

    bool startServer();
    void stopServer();

    void forwardCanData(quint32 canId, const QByteArray &data, qint64 timestamp);
    void forwardSerialData(const QByteArray &data);


signals:
    void errorOccurred(const QString &error);
    void canSendRequest(quint32 canId, const QByteArray &data);
    void SerialSendRequest(const QByteArray &data);

private:
    // 协议相关
    #pragma pack(push, 1)
    struct CanDataPacket {
        quint32 magic;          // 魔法头: 0xCAFE
        quint32 length;         // 数据长度
        qint64 timestamp;       // 时间戳（毫秒）
        quint32 canId;          // CAN ID
        quint8 data[8];         // CAN数据（最多8字节）
        quint16 crc;            // CRC16校验
    };
    struct ControlPacket {
        quint32 magic;          // 魔法头: 0xBEEF
        quint8 command;         // 指令类型
        quint32 canId;          // CAN ID（某些指令使用）
        quint8 dataLength;      // 数据长度（0-8）
        quint8 data[8];         // 指令数据
        quint16 crc;            // CRC16校验
    };//20个字节
    #pragma pack(pop)

    void cleanupDisconnectedClients();
    void onNewConnection();

    void onSocketError(QAbstractSocket::SocketError error);
    void onClientDisconnected();
    void onClientReadyRead();

    void processClientData(QTcpSocket *client,const QByteArray newdata);
    bool validatePacket(const ControlPacket &packet);
    void handleCommand(QTcpSocket *client, const ControlPacket &packet);

    void sendToAllClients(const QByteArray &data);
    void sendToClient(QTcpSocket *client, const QByteArray &data);

private:
    QMutex m_Mutex;
    QList<QTcpSocket*> m_clients;
    QElapsedTimer m_testtimer;

    quint16 m_port{502};
    enum ServerState {
        STATE_STOPPED,
        STATE_STARTING,
        STATE_RUNNING,
        STATE_STOPPING
    };
    std::atomic<ServerState> m_state{STATE_STOPPED};
    std::atomic<qint64> m_totalBytesSent{0};
    std::atomic<qint64> m_totalBytesReceived{0};

    QThread *m_serverThread{nullptr};
    QTcpServer *m_tcpServer{nullptr};
    QTimer *m_heartbeatTimer{nullptr};
    QTimer *m_cleanupTimer{nullptr};
};

#endif // TCPSERVER_H
