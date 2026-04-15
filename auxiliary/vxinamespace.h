#pragma once
#include <QtCore>

namespace Vxi11 {
    constexpr int HTTP_PORT = 80;
    constexpr int WEB_PORT = 8080;

    constexpr int VXI_PORT = 5025;
    constexpr quint8  HEADER = 0x80;
    constexpr quint32 DEVICE_CORE  = 395183;

    enum CoreProcedure : quint32 {
        GET_PORT              = 3,    // Useless
        CREATE_LINK           = 10,
        DEVICE_WRITE          = 11,
        DEVICE_READ           = 12,
        DEVICE_READSTB        = 13,
        DEVICE_TRIGGER        = 14,   // Excess
        DEVICE_CLEAR          = 15,
        DEVICE_REMOTE         = 16,
        DEVICE_LOCAL          = 17,
        DEVICE_LOCK           = 18,
        DEVICE_UNLOCK         = 19,
        DEVICE_ENABLE_SRQ     = 20,   // enable/disable SRQ .Not Support
        DEVICE_DOCMD          = 22,   // Excess
        DESTROY_LINK          = 23,
        CREATE_INTER_CHAN     = 25,   // For use in SRQ notifications. Not Support
        DESTROY_INTER_CHAN    = 26    // Not Support
        // 0-2,4-9,21,24,27+ to preserve value
    };

    enum Rpc_MsgType : quint32 {
        CALL  = 0,
        REPLY = 1
    };

    enum Rpc_ReplyStat : quint32 {
        MSG_ACCEPTED = 0,
        MSG_DENIED   = 1
    };

    enum auth_flavor : quint32 {
        AUTH_NONE  = 0,   // (verf.length = 0)
        AUTH_SYS   = 1,
        AUTH_SHORT = 2,
        AUTH_DES   = 3,
        VERF_LENG  = 0
    };

    enum Rpc_AcceptStat : quint32 {
        SUCCESS       = 0,
        PROG_UNAVAIL  = 1,
        PROG_MISMATCH = 2,
        PROC_UNAVAIL  = 3,
        GARBAGE_ARGS  = 4,
        SYSTEM_ERR    = 5
    };

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
        // 2,7,10,13,14,16,18,19,20,22,24-28,30+ to preserve value
    };

    enum AsyncProcedure : quint32 {
        DEVICE_ABORT        = 1     // 中止正在执行的操作
    };

    enum IntrProcedure : quint32 {
        DEVICE_INTR_SRQ     = 30    // 设备发送服务请求（SRQ）中断
    };
}

