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
#include <unistd.h>
#include "auxiliary/scpimanager.h"
#include "tirpcloader.h"
#include "canworker.h"

Q_DECLARE_LOGGING_CATEGORY(tcp)

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
        CALL  = 0,     // 调用
        REPLY = 1      // 回复
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
    explicit TcpServerManager(ScpiManager* scpi,QObject *parent = nullptr);
    ~TcpServerManager();

    bool startServer();
    void stopServer();

    void forwardCanData(quint32 canId, const QByteArray &data,const QString &canface);
    void forwardSerialData(const QByteArray &data);

signals:
    void canSendRequest(quint32 canId, const QByteArray &data,const QString &canface);
    void SerialSendRequest(const QByteArray &data);

private:
    void sendToAllClients(const QByteArray &data);

    void onNewConnection();
    bool registerWithRpcbind();

    void processClientData(QTcpSocket *client,const QByteArray newdata);
    void handleVxi11RpcCall(QTcpSocket* client, const QByteArray &data);

    void handleCreateLink(QTcpSocket* client, const QByteArray &data,const quint32 &xid);
    void handleDeviceWrite(QTcpSocket* client, const QByteArray &data,const quint32 &xid);
    void handleDeviceRead(QTcpSocket* client, const QByteArray &data,const quint32 &xid);
    void handleDeviceReadStb(QTcpSocket* client, const QByteArray &data,const quint32 &xid);
    void handleDeviceTrigger(QTcpSocket* client, const QByteArray &data,const quint32 &xid);
    void handleDeviceClear(QTcpSocket* client, const QByteArray &data,const quint32 &xid);
    void handleDeviceRemote(QTcpSocket* client, const QByteArray &data,const quint32 &xid);
    void handleDeviceLocal(QTcpSocket* client, const QByteArray &data,const quint32 &xid);
    void handleDeviceLock(QTcpSocket* client, const QByteArray &data,const quint32 &xid);
    void handleDeviceUnlock(QTcpSocket* client, const QByteArray &data,const quint32 &xid);
    void handleDeviceEnableSrq(QTcpSocket* client, const QByteArray &data,const quint32 &xid);
    void handleDeviceDocmd(QTcpSocket* client, const QByteArray &data,const quint32 &xid);
    void handleDestroyLink(QTcpSocket* client, const QByteArray &data,const quint32 &xid);
    void handleCreateIntrChan(QTcpSocket* client, const QByteArray &data,const quint32 &xid);
    void handleDestroyIntrChan(QTcpSocket* client, const QByteArray &data,const quint32 &xid);

    QByteArray createErrorResponse(quint32 xid, quint32 error);

private:
    struct DeviceLink {
        QTcpSocket* client;
        quint32 id;
        bool lock;
        bool aborted;
        QDateTime createTime;
        QByteArray pending_Vxi_Scpi_response;

        DeviceLink() :  client(nullptr), id(0), lock(false), aborted(false) {}
    };

    enum ServerState {
        STATE_STOPPED,
        STATE_STARTING,
        STATE_RUNNING,
        STATE_STOPPING
    };

    QMutex m_Mutex;
    QList<QTcpSocket*> m_clients;
    QElapsedTimer m_testtimer;
    QMap<quint32, DeviceLink> m_deviceLinks;

    quint32 m_nextLinkId{1};
    std::atomic<ServerState> m_state{STATE_STOPPED};

    ScpiManager* m_scpiManager{nullptr};
    QThread *m_serverThread{nullptr};
    QTcpServer *m_tcpServer{nullptr};
    QTimer *m_cleanupTimer{nullptr};
};

#endif // TCPSERVER_H
