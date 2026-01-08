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

struct XDR;
typedef bool_t (*xdrproc_t)(XDR*, void*);

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

namespace Vxi11 {
    constexpr quint32 DEVICE_CORE = 395183;    //->device
    constexpr quint32 DEVICE_ASYNC = 395184;   //->device
    constexpr quint32 DEVICE_INTR = 395185;    //<-device

    constexpr quint32 DEVICE_CORE_VERSION = 1;
    constexpr quint32 DEVICE_ASYNC_VERSION  = 1;
    constexpr quint32 DEVICE_INTR_VERSION = 1;

    enum ErrorCode : quint32 {
        NO_ERROR                        = 0,    // 无错误
        SYNTAX_ERROR                    = 1,    // 语法错误
        DEVICE_NOT_ACCESSIBLE           = 3,    // 设备不可访问
        INVALID_LINK_IDENTIFIER         = 4,    // 无效链接标识符
        PARAMETER_ERROR                 = 5,    // 参数错误
        CHANNEL_NOT_ESTABLISHED         = 6,    // 通道未建立
        OPERATION_NOT_SUPPORTED         = 8,    // 操作不支持
        OUT_OF_RESOURCES                = 9,    // 资源不足
        DEVICE_LOCKED_BY_ANOTHER_LINK   = 11,   // 设备被其他链接锁定
        NO_LOCK_HELD_BY_THIS_LINK       = 12,   // 此链接未持有锁
        IO_TIMEOUT                      = 15,   // I/O超时
        IO_ERROR                        = 17,   // I/O错误
        INVALID_ADDRESS                 = 21,   // 无效地址
        ABORT                           = 23,   // 操作被中止
        CHANNEL_ALREADY_ESTABLISHED     = 29    // 通道已建立
        // 2,7,10,13,14,16,18,19,20,22,24-28,30+ 为保留值
    };

    enum CoreProcedure : quint32 {
        GET_PORT              = 3,    // 获取端口号（通常由RPC端口映射器使用）
        CREATE_LINK           = 10,   // 创建与设备的链接，返回链接标识符
        DEVICE_WRITE          = 11,   // 向设备写入数据（ASCII消息）
        DEVICE_READ           = 12,   // 从设备读取数据
        DEVICE_READSTB        = 13,   // 读取设备状态字节（Status Byte）
        DEVICE_TRIGGER        = 14,   // 向设备发送触发信号
        DEVICE_CLEAR          = 15,   // 发送设备清除命令
        DEVICE_REMOTE         = 16,   // 将设备设置为远程模式（禁用前面板）
        DEVICE_LOCAL          = 17,   // 将设备设置为本地模式（启用前面板）
        DEVICE_LOCK           = 18,   // 锁定设备（独占访问）
        DEVICE_UNLOCK         = 19,   // 解锁设备
        DEVICE_ENABLE_SRQ     = 20,   // 启用/禁用服务请求（SRQ）中断
        DEVICE_DOCMD          = 22,   // 执行设备特定命令（通用扩展接口）
        DESTROY_LINK          = 23,   // 销毁与设备的链接，释放资源
        CREATE_INTR_CHAN      = 25,   // 创建中断通道（用于SRQ通知）
        DESTROY_INTR_CHAN     = 26    // 销毁中断通道
        // 0-2,4-9,21,24,27+ 为保留值
    };

    enum AsyncProcedure : quint32 {
        DEVICE_ABORT        = 1     // 中止正在执行的操作
    };

    enum IntrProcedure : quint32 {
        DEVICE_INTR_SRQ     = 30    // 设备发送服务请求（SRQ）中断
    };


    enum Rpc_MsgType : quint32 {
        CALL  = 0,     // 调用远程过程
        REPLY = 1      // 远程过程回复
    };

    enum Rpc_ReplyStat : quint32 {
        MSG_ACCEPTED = 0,   // 消息被接受
        MSG_DENIED   = 1    // 消息被拒绝
    };

    enum Rpc_AcceptStat : quint32 {
        SUCCESS       = 0,  // RPC 调用成功执行
        PROG_UNAVAIL  = 1,  // 远程程序不可用
        PROG_MISMATCH = 2,  // 程序版本不匹配
        PROC_UNAVAIL  = 3,  // 远程过程不可用
        GARBAGE_ARGS  = 4,  // 参数无法解码（垃圾参数）
        SYSTEM_ERR    = 5   // 系统错误（如内存不足）
    };

    enum Rpc_RejectStat : quint32 {
        RPC_MISMATCH  = 0,  // RPC 版本不匹配（必须为2）
        AUTH_ERROR    = 1   // 认证错误
    };
}


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
