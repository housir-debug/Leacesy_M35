#include "scpimanager.h"
#include <QDebug>
#include <QNetworkInterface>
#include <cstring>

// ======================== 初化模块部分 =================================

ScpiManager* ScpiManager::s_instance = nullptr;
Q_LOGGING_CATEGORY(scpi, "scpi:")

ScpiManager::ScpiManager(QObject *parent) : QObject(parent) {
    memset(&m_scpiContext, 0, sizeof(scpi_t));
    s_instance = this;  // 设置单例实例
}

bool ScpiManager::init(const QString &manufacturer, const QString &model,
                       const QString &serial, const QString &version)
{
    memset(&interface, 0, sizeof(interface));

    interface.error = staticErrorCallback;
    interface.write = staticWriteCallback;
    interface.control = staticControlCallback;
    interface.flush = staticFlushCallback;
    interface.reset = staticResetCallback;

    const scpi_unit_def_t* units = NULL;

    m_idnManufacturer = manufacturer.toUtf8();
    m_idnModel = model.toUtf8();
    m_idnSerial = serial.toUtf8();
    m_idnVersion = version.toUtf8();

    const char* idn1 = m_idnManufacturer.constData();  // 制造商
    const char* idn2 = m_idnModel.constData();         // 型号
    const char* idn3 = m_idnSerial.constData();        // 序列号
    const char* idn4 = m_idnVersion.constData();       // 固件版本

    static char input_buffer[256];
    static scpi_error_t error_queue[10];

    SCPI_Init(&m_scpiContext,
              m_scpiCommands,        // 命令表
              &interface,            // 接口回调
              units,                 // 单位定义（可为NULL）
              idn1, idn2, idn3, idn4, // 识别信息
              input_buffer,          // 输入缓冲区
              sizeof(input_buffer),  // 缓冲区大小
              error_queue,           // 错误队列
              sizeof(error_queue) / sizeof(scpi_error_t)); // 队列大小

    return true;
}

// ======================= 接口回调部分 ===================================

int ScpiManager::staticErrorCallback(scpi_t* context, int_fast16_t err) {
    Q_UNUSED(context);
    if (s_instance) {
        return s_instance->handleError(err);
    }
    return 0;
}
int ScpiManager::handleError(int_fast16_t err)
{
    qCWarning(scpi) << "SCPI match error:" << err;
    return 0;  // 返回0表示成功处理错误
}

size_t ScpiManager::staticWriteCallback(scpi_t* context, const char* data, size_t len) {
    Q_UNUSED(context);
    if (s_instance) {
        return s_instance->handleWrite(data, len);
    }
    return 0;
}
size_t ScpiManager::handleWrite(const char* data, size_t len) {
    QMutexLocker locker(&m_bufferMutex);
    m_responseBuffer.append(data, len);
    qCDebug(scpi) << "SCPI write:" << len << "bytes，"<< "connent:" << data;
    return len;
}

scpi_result_t ScpiManager::staticControlCallback(scpi_t* context,scpi_ctrl_name_t ctrl,scpi_reg_val_t val) {
    Q_UNUSED(context);
    if (s_instance) {
        return s_instance->handleControl(ctrl, val);
    }
    return SCPI_RES_ERR;
}
scpi_result_t ScpiManager::handleControl(scpi_ctrl_name_t ctrl, scpi_reg_val_t val) {
    qCDebug(scpi) << "SCPI control:" << ctrl << "value:" << val;
    // 根据控制类型处理
    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::staticFlushCallback(scpi_t* context) {
    Q_UNUSED(context);
    if (s_instance) {
        return s_instance->handleFlush();
    }
    return SCPI_RES_ERR;
}
scpi_result_t ScpiManager::handleFlush() {
    // 压出缓冲区-强制发送
    // qCDebug(scpi) << "SCPI flush";
    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::staticResetCallback(scpi_t* context) {
    Q_UNUSED(context);
    if (s_instance) {
        return s_instance->handleReset();
    }
    return SCPI_RES_ERR;
}
scpi_result_t ScpiManager::handleReset() {
    qCDebug(scpi) << "SCPI reset";
    // 重置仪器状态
    return SCPI_RES_OK;
}

// ======================= 命令处理部分 ===================================

const scpi_command_t ScpiManager::m_scpiCommands[] = {   // 格式：{ "命令模式", 回调函数, 标签(整数) }
    { "*IDN?", ScpiManager::scpiIdentify,0},
    { "*RST", ScpiManager::scpiReset, 0 },
    { "*CLS", ScpiManager::scpiCls, 0 },
    { "*ESR?", ScpiManager::scpiEsrQ, 0 },

    { "NETWork", NULL, 0 },
    { "NETWork:IP?", ScpiManager::scpiNetworkIpQ, 0 },

    SCPI_CMD_LIST_END
    // 命令处理函数调用->SCPI库函数->接口回调函数
};

scpi_result_t ScpiManager::scpiIdentify(scpi_t* context) {
    if (!s_instance){ return SCPI_RES_ERR;}

    QString idn = QString("%1,%2,%3,%4").arg(QString::fromUtf8(s_instance->m_idnManufacturer),
                                           QString::fromUtf8(s_instance->m_idnModel),
                                           QString::fromUtf8(s_instance->m_idnSerial),
                                           QString::fromUtf8(s_instance->m_idnVersion));

    SCPI_ResultCharacters(context, idn.toUtf8().constData(), idn.length());
    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::scpiReset(scpi_t* context) {
    Q_UNUSED(context);
    if (s_instance) {
        s_instance->handleReset();
    }
    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::scpiCls(scpi_t* context) {
    Q_UNUSED(context);
    qCDebug(scpi) << "*CLS command";
    // 清除状态寄存器等
    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::scpiEsrQ(scpi_t* context) {
    Q_UNUSED(context);
    SCPI_ResultInt32(context, 0);// 返回事件状态寄存器值
    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::scpiNetworkIpQ(scpi_t* context) {
    if (!s_instance) return SCPI_RES_ERR;

    QString ip = "127.0.0.1";
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress &addr : qAsConst(addresses)) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol &&
            addr != QHostAddress::LocalHost) {
            ip = addr.toString();
            break;
        }
    }

    SCPI_ResultCharacters(context, ip.toUtf8().constData(), ip.length());
    return SCPI_RES_OK;
}

// ===================== 外部调用处理部分 =================================

QByteArray ScpiManager::processCommand(const QByteArray &command) {
    if (!s_instance) {
        qCCritical(scpi) << "错误：实例为空";
        return QByteArray("ERROR: Instance null\n");
    }

    if (!m_scpiContext.cmdlist || !m_scpiContext.interface) {
        qCCritical(scpi) << "错误：上下文未初始化";
        return QByteArray("ERROR: Context not initialized\n");
    }

    m_responseBuffer.clear();

    QByteArray cmd = command;
    if (!cmd.endsWith('\n')) {cmd.append('\n');}

    SCPI_Input(&m_scpiContext, cmd.constData(), cmd.size());
    return m_responseBuffer;
}

// ======================= 析构部分 ===================================

ScpiManager::~ScpiManager() {
    s_instance = nullptr;
}
