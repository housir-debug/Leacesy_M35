#pragma once
#include <QLoggingCategory>
#include <QLibrary>

Q_DECLARE_LOGGING_CATEGORY(libtripc)

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

    bool pmap_set(quint32 program, quint32 version, int protocol, quint16 port);
    bool pmap_unset(quint32 program, quint32 version, int protocol, quint16 port);

private:
    TirpcDynamicLoader() = default;
    ~TirpcDynamicLoader() = default;

    bool resolveFunctions();

private:
    QLibrary m_library;
    bool m_loaded{false};

    typedef bool_t (*pmap_set_t)(quint32, quint32, int, quint16);
    typedef bool_t (*pmap_unset_t)(quint32, quint32, int, quint16);

    pmap_set_t m_pmap_set = nullptr;
    pmap_unset_t m_pmap_unset = nullptr;
};
