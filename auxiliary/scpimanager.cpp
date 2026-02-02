#include "scpimanager.h"
#include <QDebug>
#include <QNetworkInterface>
#include <cstring>

// ======================== 初化模块部分 =================================

ScpiManager* ScpiManager::s_instance = nullptr;
Q_LOGGING_CATEGORY(scpi, "scpi:")

ScpiManager::ScpiManager(QObject *parent) : QObject(parent) {
    s_instance = this;
    memset(&m_scpiContext, 0, sizeof(scpi_t));
    memset(&interface, 0, sizeof(interface));

    interface.error = staticErrorCallback;
    interface.control = staticControlCallback;
    interface.flush = staticFlushCallback;
    interface.reset = staticResetCallback;
    interface.write = staticWriteCallback;

    const scpi_unit_def_t* units = NULL;

    static char input_buffer[256];
    static scpi_error_t error_queue[10];

    SCPI_Init(&m_scpiContext,
              m_scpiCommands,                              // 命令表
              &interface,                                  // 接口回调
              units,                                       // 单位定义（可为NULL）
              nullptr, nullptr, nullptr, nullptr,          // 识别信息（指针）
              input_buffer,                                // 输入缓冲区
              sizeof(input_buffer),                        // 缓冲区大小
              error_queue,                                 // 错误队列
              sizeof(error_queue) / sizeof(scpi_error_t)); // 队列大小
}

const scpi_command_t ScpiManager::m_scpiCommands[] = {
    // 格式：{ "命令模式", 回调函数, 标签(整数) }
    // ? 查询命令-必须返回响应  |  动作命令-不返回响应
    { "*IDN?", ScpiManager::scpiIdentify,0},
    { "*RST", ScpiManager::scpiReset, 0 },
    { "*CLS", ScpiManager::scpiCls, 0 },
    { "*ESR?", ScpiManager::scpiEsrQ, 0 },

    { "NETWork", NULL, 0 },
    { "NETWork:IP?", ScpiManager::scpiNetworkIpQ, 0 },

    SCPI_CMD_LIST_END
};

// ======================= 命令-处理部分 ===================================

size_t ScpiManager::staticWriteCallback(scpi_t* context, const char* data, size_t len = 0) {   // 自动调用两次添加结束符\r\n
    Q_UNUSED(context);
    s_instance->m_responseBuffer.append(data, len);
    qCDebug(scpi) << "SCPI write:" << len << "bytes，"<< "connent:" << data;
    return len;
}

scpi_result_t ScpiManager::scpiIdentify(scpi_t* context) {
    QString idn = QString("%1,%2,%3,%4").arg(ConfigManager::s_manufacturer,
                                             ConfigManager::s_model,
                                             ConfigManager::s_serialNumber,
                                             ConfigManager::s_firmwareVersion);

    staticWriteCallback(context, idn.toUtf8().constData(), idn.length());
    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::scpiReset(scpi_t* context) {
    Q_UNUSED(context);
    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::scpiCls(scpi_t* context) {
    Q_UNUSED(context);
    s_instance->m_esrRegister = 0x00;
    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::scpiEsrQ(scpi_t* context) {
    const char* esrData = reinterpret_cast<const char*>(&s_instance->m_esrRegister);
    staticWriteCallback(context,esrData, 1);// 返回事件状态寄存器值
    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::scpiNetworkIpQ(scpi_t* context) {
    QString ip = "127.0.0.1";
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress &addr : qAsConst(addresses)) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol &&
            addr != QHostAddress::LocalHost) {
            ip = addr.toString();
            break;
        }
    }

    staticWriteCallback(context, ip.toUtf8().constData(), ip.length());
    return SCPI_RES_OK;
}

// ===================== 外部调用处理部分 =================================

QByteArray ScpiManager::processCommand(const QByteArray &command) {
    if (!m_scpiContext.cmdlist || !m_scpiContext.interface) {
        qCCritical(scpi) << "错误：上下文未初始化！";
        return QByteArray("ERROR: Context not initialized\n");
    }

    QByteArray cmd = command;
    if (!cmd.endsWith('\n')) {cmd.append('\n');}

    QMutexLocker locker(&m_bufferMutex);
    m_responseBuffer.clear();

    SCPI_Input(&m_scpiContext, cmd.constData(), cmd.size());
    return m_responseBuffer;
}

// ======================= 析构部分 ===================================

ScpiManager::~ScpiManager() {
    m_responseBuffer.clear();
    s_instance = nullptr;
}
