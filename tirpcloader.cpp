// tirpcloader.cpp
#include "tirpcloader.h"

Q_LOGGING_CATEGORY(libtripc, "libtripc:")

typedef int bool_t;

TirpcDynamicLoader& TirpcDynamicLoader::instance()
{
    static TirpcDynamicLoader loader;
    return loader;
}

bool TirpcDynamicLoader::load()
{
    if (m_loaded) {
        return true;
    }

    const char* lib_names[] = {
        "libtirpc.so.3",
        "libtirpc.so",
        "libtirpc.so.3.0.0",
        "libtirpc.so.1",
        "libtirpc.so.2",
        nullptr
    };

    for (int i = 0; lib_names[i] != nullptr; i++) {
        m_library.setFileName(lib_names[i]);
        qCDebug(libtripc) << "尝试加载库:" << lib_names[i];

        if (m_library.load()) {
            if (resolveFunctions()) {
                m_loaded = true;
                qCDebug(libtripc) << "成功加载 libtirpc:" << lib_names[i];
                return true;
            }
            m_library.unload();
        } else {
            qCDebug(libtripc) << "加载失败:" << m_library.errorString();
        }
    }

    qCWarning(libtripc) << "无法加载 libtirpc 库，将使用手动RPC实现";
    return false;
}

bool TirpcDynamicLoader::resolveFunctions()
{
    m_pmap_set = (pmap_set_t)m_library.resolve("pmap_set");
    m_pmap_unset = (pmap_unset_t)m_library.resolve("pmap_unset");

    if (!m_pmap_set || !m_pmap_unset) {
        qCWarning(libtripc) << "函数解析失败:";
        qCWarning(libtripc) << "  pmap_set:" << (m_pmap_set ? "成功" : "失败");
        qCWarning(libtripc) << "  pmap_unset:" << (m_pmap_unset ? "成功" : "失败");
        return false;
    }

    return true;
}

bool TirpcDynamicLoader::pmap_set(quint32 program, quint32 version, int protocol, quint16 port)
{
    if (!m_loaded || !m_pmap_set) {
        return false;
    }

    bool_t result = m_pmap_set(program, version, protocol, port);
    qCDebug(libtripc) << "pmap_set called: program=" << program
                 << ", version=" << version
                 << ", protocol=" << protocol
                 << ", port=" << port
                 << ", result=" << result;

    return result != 0;
}

bool TirpcDynamicLoader::pmap_unset(quint32 program, quint32 version, int protocol, quint16 port)
{
    if (!m_loaded || !m_pmap_unset) {
        return false;
    }

    bool_t result = m_pmap_unset(program, version, protocol, port);
    qCDebug(libtripc) << "pmap_unset called: program=" << program
                 << ", version=" << version
                 << ", protocol=" << protocol
                 << ", port=" << port
                 << ", result=" << result;

    return result != 0;
}
