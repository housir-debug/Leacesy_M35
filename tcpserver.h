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
    // SCPI命令回调函数类型
        typedef std::function<QString(const QStringList&)> ScpiHandler;

        // SCPI命令树节点
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

        // 错误代码
        enum ScpiError {
            NO_ERROR = 0,
            COMMAND_ERROR = -100,
            EXECUTION_ERROR = -200,
            DEVICE_SPECIFIC_ERROR = -300,
            QUERY_ERROR = -400,
            PARAMETER_ERROR = -500
        };
    #pragma pack(pop)

    // 初始化SCPI命令树
        void initScpiCommandTree();

        // 注册SCPI命令
        void registerScpiCommand(const QString& command, ScpiHandler handler,
                                const QString& description = "");

        // 解析和处理SCPI命令
        void processScpiCommand(QTcpSocket* client, const QString& command);

        // 遍历执行SCPI命令
        QString executeScpiCommand(const QString& command);

        // 支持的通配符模式匹配
        bool matchScpiPattern(const QString& pattern, const QString& command);

        // SCPI命令处理器
        QString handleScpiIdentify(const QStringList& args);
        QString handleScpiSystemReset(const QStringList& args);
        QString handleScpiNetworkInterface(const QStringList& args);

        // 生成SCPI响应
        QString generateScpiResponse(const QString& response);

        QString getScpiErrors();

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
    // SCPI命令树
        ScpiNode* m_scpiRoot;

        // SCPI查询符号
        static const QString SCPI_QUERY_SYMBOL;

        // 添加SCPI错误队列
        QList<QPair<int, QString>> m_scpiErrors;
        void addScpiError(int code, const QString& message);


        // SCPI支持状态
        bool m_scpiEnabled{true};

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
