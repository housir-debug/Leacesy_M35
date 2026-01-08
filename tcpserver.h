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
#include <QDateTime>
#include <QLoggingCategory>
#include <QMap>
#include <QRegularExpression>
#include <QLibrary>
#include <unistd.h>
#include "scpimanager.h"

Q_DECLARE_LOGGING_CATEGORY(tcp)

// ===================== 动态加载所需类型定义 =====================
#ifdef __cplusplus
extern "C" {
#endif

// 定义 bool_t 类型（SUN RPC 标准类型）
typedef int bool_t;

// 定义 rpcprog_t, rpcvers_t, rpcproc_t
typedef unsigned int rpcprog_t;
typedef unsigned int rpcvers_t;
typedef unsigned int rpcproc_t;

// RPC 状态枚举
enum clnt_stat {
    RPC_SUCCESS = 0,
    RPC_CANTENCODEARGS = 1,
    RPC_CANTDECODERES = 2,
    RPC_CANTSEND = 3,
    RPC_CANTRECV = 4,
    RPC_TIMEDOUT = 5,
    RPC_CANTDECODEARGS = 6,
    RPC_CANTSENDTO = 7,
    RPC_CANTRECVFROM = 8,
    RPC_CANTSENDTO_LOCAL = 9,
    RPC_CANTRECVFROM_LOCAL = 10,
    RPC_CANTSENDTO_REMOTE = 11,
    RPC_CANTRECVFROM_REMOTE = 12,
    RPC_UNKNOWNHOST = 13,
    RPC_UNKNOWNADDR = 14,
    RPC_UNKNOWNPROTO = 15,
    RPC_UNKNOWNERR = 16
};

// 认证类型
enum auth_flavor {
    AUTH_NULL = 0,
    AUTH_UNIX = 1,
    AUTH_SHORT = 2,
    AUTH_DES = 3,
    AUTH_KERB = 4
};

// XDR 方向
enum xdr_op {
    XDR_ENCODE = 0,
    XDR_DECODE = 1,
    XDR_FREE = 2
};

#ifdef __cplusplus
}
#endif

// XDR 函数指针类型（前向声明）
struct XDR;
typedef bool_t (*xdrproc_t)(XDR*, void*);

// VXI-11 RPC相关定义
namespace Vxi11 {
    // VXI-11核心程序号
    constexpr quint32 DEVICE_CORE = 395183;
    constexpr quint32 DEVICE_CORE_VERSION = 1;

    // VXI-11设备错误码
    enum ErrorCode : quint32 {
        NO_ERROR = 0,
        OPERATION_IN_PROGRESS = 1,
        OPERATION_IN_QUEUE = 2,
        OPERATION_COMPLETED = 3,
        OPERATION_ABORTED = 4,
        OPERATION_FAILED = 5,
        DEVICE_LOCKED = 6,
        DEVICE_UNLOCKED = 7,
        DEVICE_NOT_FOUND = 8,
        PARAMETER_ERROR = 9,
        CHANNEL_NOT_ESTABLISHED = 10,
        OPERATION_NOT_SUPPORTED = 11,
        OUT_OF_RESOURCES = 12,
        DEVICE_DEAD = 13,
        INVALID_LINK_IDENTIFIER = 14
    };

    // RPC消息类型
    enum RpcMsgType : quint32 {
        CALL = 0,
        REPLY = 1
    };

    // RPC回复状态
    enum RpcReplyStat : quint32 {
        MSG_ACCEPTED = 0,
        MSG_DENIED = 1
    };

    // 接受状态
    enum RpcAcceptStat : quint32 {
        SUCCESS = 0,
        PROG_UNAVAIL = 1,
        PROG_MISMATCH = 2,
        PROC_UNAVAIL = 3,
        GARBAGE_ARGS = 4,
        SYSTEM_ERR = 5
    };

    enum Procedure : quint32 {
        GET_PORT = 3,
        CREATE_LINK = 10,
        DEVICE_WRITE = 11,
        DEVICE_READ = 12,
        DEVICE_DOCMD = 15,
        DESTROY_LINK = 23
    };
}

class TirpcDynamicLoader
{

public:
    static TirpcDynamicLoader& instance();

    bool load();
    bool isLoaded() const { return m_loaded; }

    // PORTMAP函数
    bool pmap_set(quint32 program, quint32 version, int protocol, quint16 port);
    bool pmap_unset(quint32 program, quint32 version, int protocol, quint16 port);

    // XDR编码函数指针类型
    void* getFunction(const char* name);

private:
    TirpcDynamicLoader() = default;
    ~TirpcDynamicLoader() = default;

    bool resolveFunctions();

    QLibrary m_library;
    bool m_loaded = false;

    // 函数指针类型定义
    typedef bool_t (*pmap_set_t)(quint32, quint32, int, quint16);
    typedef bool_t (*pmap_unset_t)(quint32, quint32, int, quint16);

    pmap_set_t m_pmap_set = nullptr;
    pmap_unset_t m_pmap_unset = nullptr;
};

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

    // VXI-11链接管理
    struct DeviceLink {
        quint32 id;
        quint32 lock;
        QDateTime createTime;
        QTcpSocket* client;
        bool aborted;

        DeviceLink() : id(0), lock(0), client(nullptr), aborted(false) {}
    };

    ScpiManager* m_scpiManager{nullptr};
    QString handleScpiCommand(const QByteArray& command);

    void sendToAllClients(const QByteArray &data);
    void cleanupDisconnectedClients();
    void onNewConnection();
    bool registerWithRpcbind();

    void onSocketError(QAbstractSocket::SocketError error);
    void onClientDisconnected();
    void onClientReadyRead();
    void processClientData(QTcpSocket *client,const QByteArray newdata);
    void handleVxi11RpcCall(QTcpSocket* client, const QByteArray &data);
    bool isVxi11RpcCall(const QByteArray &data);

    // VXI-11处理函数
    void handleCreateLink(QTcpSocket* client, const QByteArray &data);
    void handleDeviceWrite(QTcpSocket* client, const QByteArray &data);
    void handleDeviceRead(QTcpSocket* client, const QByteArray &data);
    void handleDeviceDocmd(QTcpSocket* client, const QByteArray &data);
    void handleDestroyLink(QTcpSocket* client, const QByteArray &data);

    // 辅助函数
    quint32 extractXid(const QByteArray &data);
    quint32 extractProcedure(const QByteArray &data);
    QByteArray createErrorResponse(quint32 xid, quint32 error);
    QByteArray createLinkResponse(quint32 xid, quint32 lid);
    DeviceLink* createLink(QTcpSocket* client);
    void destroyLink(quint32 linkId);

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
    QElapsedTimer m_testtimer;

    const QString getManufacturer{"Leacesy"};
    const QString getModel{"M35-Current-Measuring"};
    const QString getSerialNumber{"SN-001"};
    const QString getFirmwareVersion{ "1.0.0" };

    quint16 m_port{5025};
    bool m_vxi11Enabled{true};

    std::atomic<ServerState> m_state{STATE_STOPPED};

    QMap<quint32, DeviceLink> m_deviceLinks;
    quint32 m_nextLinkId{1};
    QMutex m_linkMutex;

    QThread *m_serverThread{nullptr};
    QTcpServer *m_tcpServer{nullptr};
    QTimer *m_cleanupTimer{nullptr};
    QTimer *m_linkCleanupTimer{nullptr};
};

#endif // TCPSERVER_H
