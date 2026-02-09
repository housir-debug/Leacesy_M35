#include "scpimanager.h"
#include "auxiliary/config_manager.h"
#include <QNetworkInterface>

Q_LOGGING_CATEGORY(scpi, "SCPI:")

// ======================== 指令表定义 =========================

const scpi_command_t ScpiManager::m_scpiCommands[] = {
    { "*CLS",      SCPI_CoreCls,    0 },
    { "*ESE",      SCPI_CoreEse,    0 },
    { "*ESE?",     SCPI_CoreEseQ,   0 },
    { "*ESR?",     SCPI_CoreEsrQ,   0 },
    { "*IDN?",     SCPI_CoreIdnQ,   0 },
    { "*OPC",      SCPI_CoreOpc,    0 },
    { "*OPC?",     SCPI_CoreOpcQ,   0 },
    { "*RST",      SCPI_CoreRst,    0 },
    { "*SRE",      SCPI_CoreSre,    0 },
    { "*SRE?",     SCPI_CoreSreQ,   0 },
    { "*STB?",     SCPI_CoreStbQ,   0 },
    // -----------user-defined----------


    { "NETWork:IP?", ScpiManager::scpiNetworkIpQ, 0 },

    SCPI_CMD_LIST_END
};

// ======================== 初化化模块 =================================

ScpiManager::ScpiManager(QObject *parent) : QObject(parent) {
    // QString -> QByteArray
    m_idnManufacturer = ConfigManager::s_manufacturer.toUtf8();
    m_idnModel        = ConfigManager::s_model.toUtf8();
    m_idnSerialNumber = ConfigManager::s_serialNumber.toUtf8();
    m_idnVersion      = ConfigManager::s_firmwareVersion.toUtf8();

    m_interface.write   = staticWrite;
    m_interface.error   = staticError;
    m_interface.reset   = staticReset;
    m_interface.flush   = staticFlush;
    m_interface.control = staticControl;

    SCPI_Init(&m_scpiContext,
              m_scpiCommands,                               // 命令表
              &m_interface,                                 // 接口回调
              nullptr,                                      // 单位定义
              m_idnManufacturer.constData(),
              m_idnModel.constData(),
              m_idnSerialNumber.constData(),
              m_idnVersion.constData(),
              m_inputBuffer,                                // 输入缓冲区
              sizeof(m_inputBuffer),
              m_errorQueue,                                 // 错误队列
              sizeof(m_errorQueue) / sizeof(scpi_error_t));

    m_scpiContext.user_context = this;
}

// ======================= 静态回调实现 ===================================

size_t ScpiManager::staticWrite(scpi_t* context, const char* data, size_t len) {
    auto* self = static_cast<ScpiManager*>(context->user_context);
    if (self && len > 0) {
        self->m_responseBuffer.append(data, len);
        qCDebug(scpi) << "SCPI Query Response:" << data;
    }
    return len;
    // Automatically add \r\n
}

int ScpiManager::staticError(scpi_t* context, int_fast16_t err) {
    Q_UNUSED(context);
    qCWarning(scpi) << "SCPI Error Code:" << err << "Desc:" << SCPI_ErrorTranslate(err);
    return 0;
}

scpi_result_t ScpiManager::staticReset(scpi_t* context) {
    auto* self = static_cast<ScpiManager*>(context->user_context);
    Q_UNUSED(self);
    qCDebug(scpi) << "Executing hardware reset...";
    // 这里执行具体的硬件复位动作
    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::staticFlush(scpi_t* context) {
    Q_UNUSED(context);
    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::staticControl(scpi_t* context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val) {
    Q_UNUSED(context);
    qCDebug(scpi) << "Control Signal:" << (int)ctrl << "Value:" << val;
    return SCPI_RES_OK;
}

// ======================== 自定义指令处理 ================================

scpi_result_t ScpiManager::scpiNetworkIpQ(scpi_t* context) {
    QString ip = "127.0.0.1";
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress &addr : qAsConst(addresses)) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol && addr != QHostAddress::LocalHost) {
            ip = addr.toString();
            break;
        }
    }

    SCPI_ResultCharacters(context, ip.toUtf8().constData(), ip.length());
    return SCPI_RES_OK;
}

// ======================== 外部调用接口 =================================

QByteArray ScpiManager::processCommand(const QByteArray &command) {
    if (command.isEmpty()){ return QByteArray();}

    QMutexLocker locker(&m_mutex);
    m_responseBuffer.clear();

    // Support streaming input
    SCPI_Input(&m_scpiContext, command.constData(), command.size());
    return m_responseBuffer; // if not query，return emtry
}

