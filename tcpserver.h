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

class TcpServerManager : public QObject
{
    Q_OBJECT

public:
    explicit TcpServerManager(QObject *parent = nullptr);
    ~TcpServerManager();

    bool startServer();
    void stopServer();

    bool isRunning() const { return m_running.load(); }

signals:
    void clientConnected(const QString &clientInfo);
    void clientDisconnected(const QString &clientInfo);
    void errorOccurred(const QString &error);

    // 控制指令信号（Can发送）
    void canSendRequest(quint32 canId, const QByteArray &data);
    // TCP发送完成信号
    void dataSentToClients(int clientCount, int dataSize);
    void test();

public slots:
    // 接收CAN数据并转发给所有TCP客户端
    void forwardCanData(quint32 canId, const QByteArray &data, qint64 timestamp);

private slots:
    void cleanupDisconnectedClients();
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);

private:
    // 协议相关
    #pragma pack(push, 1)
    struct CanDataPacket {
        quint32 magic;          // 魔法头: 0xCAFE
        quint32 length;         // 数据长度
        qint64 timestamp;       // 时间戳（毫秒）
        char interface[16];     // 接口名（如"can0"）
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

    bool validatePacket(const ControlPacket &packet);
    void processClientData(QTcpSocket *client);
    void handleCommand(QTcpSocket *client, const ControlPacket &packet);

    void sendToAllClients(const QByteArray &data);
    void sendToClient(QTcpSocket *client, const QByteArray &data);

private:
    enum ServerState {
        STATE_STOPPED,
        STATE_STARTING,
        STATE_RUNNING,
        STATE_STOPPING
    };

    mutable QMutex m_clientMutex;

    QTcpServer *m_tcpServer;
    QList<QTcpSocket*> m_clients;
    QHash<QTcpSocket*, QByteArray> m_clientBuffers;

    QThread *m_serverThread;
    std::atomic<bool> m_running{false};
    std::atomic<ServerState> m_state{STATE_STOPPED};

    QTimer *m_heartbeatTimer;
    QTimer *m_cleanupTimer;
    QElapsedTimer m_testtimer;

    quint16 m_port;
    QString m_interfaceName;

    std::atomic<qint64> m_totalBytesSent{0};
    std::atomic<qint64> m_totalBytesReceived{0};
    std::atomic<int> m_totalClients{0};
};

#endif // TCPSERVER_H
