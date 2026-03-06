#pragma once
#include <QLoggingCategory>
#include <QLibrary>
#include <QMutex>

Q_DECLARE_LOGGING_CATEGORY(libtripc)

typedef int bool_t;

class TirpcDynamicLoader
{
public:
    static TirpcDynamicLoader& instance();
    bool load();

    bool smart_pmap_set(quint32 program, quint32 version, int protocol, quint16 port);

    quint16 pmap_getport(quint32 program, quint32 version, int protocol);
    bool pmap_set(quint32 program, quint32 version, int protocol, quint16 port);
    bool pmap_unset(quint32 program, quint32 version);
    //bool pmap_unset(quint32 program, quint32 version, int protocol, quint16 port);

private:
    TirpcDynamicLoader() = default;
    ~TirpcDynamicLoader() = default;

    bool resolveFunctions();

    QLibrary m_library;
    bool m_loaded{false};
    QMutex m_mutex;

    typedef bool_t (*pmap_set_t)(unsigned long, unsigned long, unsigned int, unsigned short);
    typedef bool_t (*pmap_unset_t)(unsigned long, unsigned long);
    typedef unsigned short (*pmap_getport_t)(struct sockaddr_in *, unsigned long, unsigned long, unsigned int);

    pmap_set_t m_pmap_set = nullptr;
    pmap_unset_t m_pmap_unset = nullptr;
    pmap_getport_t m_pmap_getport = nullptr;
};
