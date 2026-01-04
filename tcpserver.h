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
#include <QMap>
#include <QRegularExpression>
#include <QUdpSocket>  // 新增：用于VXI-11发现


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
    #pragma pack(push, 1)
    struct CanDataPacket {
        quint32 magic;          // 魔法头: 0xCAFE
        quint32 length;         // 数据长度
        qint64 timestamp;       // 时间戳（毫秒）
        quint32 canId;          // CAN ID
        quint8 data[8];         // CAN数据（最多8字节）
        quint16 crc;            // CRC16校验
    };
    //20个字节
    #pragma pack(pop)

    // SCPI命令回调函数类型
    typedef std::function<QString(const QStringList&)> ScpiHandler;
    struct ScpiNode {
        QString key;
        QMap<QString, ScpiNode*> children;
        ScpiHandler handler;
        QString description;

        ScpiNode(const QString& k = "", ScpiHandler h = nullptr,
                 const QString& desc = "")
            : key(k), handler(h), description(desc) {}

        ~ScpiNode() {
            qDeleteAll(children);
        }
    };


    void initScpiCommandTree();
    void registerScpiCommand(const QString& command, ScpiHandler handler,const QString& description = "");
    void handleScpiSystemReset(const QStringList& args);

    void sendToAllClients(const QByteArray &data);
    void cleanupDisconnectedClients();
    void onNewConnection();
    void registerWithRpcbind();

    void onSocketError(QAbstractSocket::SocketError error);
    void onClientDisconnected();
    void onClientReadyRead();
    void processClientData(QTcpSocket *client,const QByteArray newdata);
    void handleVxi11RpcCall(QTcpSocket* client, const QByteArray &data);
    QByteArray buildVxi11Response(quint32 xid, quint32 procedure, quint32 result = 0);
    bool isVxi11RpcCall(const QByteArray &data);

private:
    enum ScpiError {
        NO_ERROR = 0,
        COMMAND_ERROR = -100,
        EXECUTION_ERROR = -200,
        DEVICE_SPECIFIC_ERROR = -300,
        QUERY_ERROR = -400,
        PARAMETER_ERROR = -500
    };
    enum ServerState {
        STATE_STOPPED,
        STATE_STARTING,
        STATE_RUNNING,
        STATE_STOPPING
    };


    static const QString SCPI_QUERY_SYMBOL;

    QHostAddress m_deviceAddress;
    QString m_deviceIp;

    QMutex m_Mutex;
    QList<QTcpSocket*> m_clients;
    QList<QPair<int, QString>> m_scpiErrors;
    QElapsedTimer m_testtimer;

    const QString getManufacturer{"National Instruments"};
    const QString getModel{"RK3568-CAN-Gateway"};
    const QString getSerialNumber{"SN-001"};
    const QString getFirmwareVersion{ "1.0.0" };

    quint16 m_port{502};
    bool m_scpiEnabled{true};
    bool m_vxi11Enabled{true};

    std::atomic<ServerState> m_state{STATE_STOPPED};
    std::atomic<qint64> m_totalBytesSent{0};
    std::atomic<qint64> m_totalBytesReceived{0};

    ScpiNode* m_scpiRoot{nullptr};
    QThread *m_serverThread{nullptr};
    QTcpServer *m_tcpServer{nullptr};
    QUdpSocket* m_vxi11UdpSocket{nullptr};
    QTimer *m_heartbeatTimer{nullptr};
    QTimer *m_cleanupTimer{nullptr};
    QTimer* m_discoveryTimer{nullptr};
};

#endif // TCPSERVER_H
