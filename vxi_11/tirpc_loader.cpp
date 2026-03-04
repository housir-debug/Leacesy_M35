// tirpcloader.cpp
#include "tirpc_loader.h"
#include <netinet/in.h>
#include <arpa/inet.h>

Q_LOGGING_CATEGORY(libtripc, "LIBTRIPC:")

TirpcDynamicLoader& TirpcDynamicLoader::instance(){
    static TirpcDynamicLoader loader;
    return loader;
}

bool TirpcDynamicLoader::load(){
    QMutexLocker locker(&m_mutex);
    if (m_loaded) {return true;}

    const char* lib_names[] = {"libtirpc.so.3","libtirpc.so.1","libtirpc.so",nullptr};

    for (int i = 0; lib_names[i] != nullptr; i++) {
        m_library.setFileName(lib_names[i]);
        if (m_library.load()) {
            if (resolveFunctions()) {
                m_loaded = true;
                qCDebug(libtripc)<<"Dynamic loading:"<<lib_names[i];
                return true;
            }

            m_library.unload();
        }
    }

    qCDebug(libtripc)<<"Failed Loaded Libtirpc Library";
    return false;
}

bool TirpcDynamicLoader::resolveFunctions(){
    m_pmap_set = (pmap_set_t)m_library.resolve("pmap_set");
    m_pmap_unset = (pmap_unset_t)m_library.resolve("pmap_unset");
    m_pmap_getport = (pmap_getport_t)m_library.resolve("pmap_getport");

    return (m_pmap_set && m_pmap_unset && m_pmap_getport);
}

// ================================= 具体操作部分 =================================

bool TirpcDynamicLoader::smart_pmap_set(quint32 program, quint32 version, int protocol, quint16 port) {
    QMutexLocker locker(&m_mutex);
    if (!m_loaded){ return false;}

    quint16 currentPort = pmap_getport(program, version, protocol);
    if (currentPort == port) {
        qCDebug(libtripc)<<"Program: "<<program<<"Already Registered Port: "<<port;
        return true;
    }

    if (currentPort != 0) {
        qCDebug(libtripc)<<"(Found "<<currentPort<< ").Port Mismatch Re-Updating...";
        m_pmap_unset(program, version); // Directly bind the function
    }

    bool_t res = m_pmap_set(program, version, protocol, port);
    if (res) {
        qCDebug(libtripc)<<"Successfully Registered Program: "<<program<<" port: "<<port;
    } else {
        qCWarning(libtripc)<<"Failed Register Program: "<<program;
    }

    return res != 0;
}

quint16 TirpcDynamicLoader::pmap_getport(quint32 program, quint32 version, int protocol) {
    if (!m_loaded || !m_pmap_getport) {return 0;}

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // check local rpcbind

    return m_pmap_getport(&addr, program, version, protocol);
}

bool TirpcDynamicLoader::pmap_set(quint32 program, quint32 version, int protocol, quint16 port)
{
    return m_loaded && m_pmap_set && m_pmap_set(program, version, protocol, port);
}

bool TirpcDynamicLoader::pmap_unset(quint32 program, quint32 version)
{
    return m_loaded && m_pmap_unset && m_pmap_unset(program, version);
}
