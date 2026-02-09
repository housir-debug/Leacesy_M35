#include "scpimanager.h"
#include "auxiliary/config_manager.h"
#include <QNetworkInterface>

Q_LOGGING_CATEGORY(scpi, "SCPI:")

// ======================== 指令表定义及处理 =========================

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
    // --------------------------- user-defined Execute --------------------
    { ":OUTPut#",                      ScpiManager::SCPI_OutputState,       0 },
    { ":OUTPut#:STATe",                ScpiManager::SCPI_OutputState,       0 },
    { ":OUTPut#:BANDwidth",            ScpiManager::SCPI_OutputBand,        0 },
    { ":OUTPut#:COMPensation:MODE",    ScpiManager::SCPI_OutputCompMode,    0 },
    // --------------------------- user-defined  Query  --------------------
    { ":OUTPut#?",                     ScpiManager::SCPI_OutputStateQ,      0 },
    { ":OUTPut#:STATe?",               ScpiManager::SCPI_OutputStateQ,      0 },
    { ":OUTPut#:BANDwidth?",           ScpiManager::SCPI_OutputBandQ,       0 },
    { ":OUTPut#:COMPensation:MODE?",   ScpiManager::SCPI_OutputCompModeQ,   0 },

    SCPI_CMD_LIST_END
};

// --- 执行 -command function ---

scpi_result_t ScpiManager::SCPI_OutputState(scpi_t* context) {
    int32_t channel;
    if(!SCPI_CommandNumbers(context, &channel, 1, 0)){return SCPI_RES_ERR;} // Array 1, Default Channel 0
    qCDebug(scpi)<<"SCPI_Output Channel: "<<channel;

    scpi_bool_t OpenOut;
    if(!SCPI_ParamBool(context, &OpenOut,true)){return SCPI_RES_ERR;}
    qCDebug(scpi)<<"SCPI_Output OpenOut: "<<OpenOut;
    quint8 func = OpenOut ? 0x01 : 0x00;

    auto* self = static_cast<ScpiManager*>(context->user_context);
    switch (channel){
        case 1:
            emit self->sendFrame_Uart4(0x01,func,"");break;
        case 2:
            emit self->sendFrame_Uart5(0x01,func,"");break;
        default:
            break;
    }

    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::SCPI_OutputBand(scpi_t* context) {
    int32_t channel;
    if(!SCPI_CommandNumbers(context, &channel, 1, 0)){return SCPI_RES_ERR;} // Array 1, Default Channel 0
    qCDebug(scpi)<<"SCPI_Output Channel: "<<channel;

    static const scpi_choice_def_t bandChoices[] = {
        {"HIGH", 1},
        {"LOW",  0},
        SCPI_CHOICE_LIST_END
    };

    int32_t selectedBand;
    if (!SCPI_ParamChoice(context, bandChoices, &selectedBand, true)) {return SCPI_RES_ERR;}
    qCDebug(scpi) << "Channel:" << channel << "Set Bandwidth to:" << (selectedBand ? "HIGH" : "LOW");
    quint8 param = selectedBand ? 0x01 : 0x00;
    QByteArray data(1, param);

    auto* self = static_cast<ScpiManager*>(context->user_context);
    switch (channel){
        case 1:
            emit self->sendFrame_Uart4(0x01,0x08,data);break;
        case 2:
            emit self->sendFrame_Uart5(0x01,0x08,data);break;
        default:
            break;
    }

    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::SCPI_OutputCompMode(scpi_t* context) {
    int32_t channel;
    if(!SCPI_CommandNumbers(context, &channel, 1, 0)){return SCPI_RES_ERR;} // Array 1, Default Channel 0
    qCDebug(scpi)<<"SCPI_Output Channel: "<<channel;

    static const scpi_choice_def_t compmodeChoices[] = {
        {"Llocal",   1},
        {"Lremote",  2},
        {"Hlocal",   3},
        {"Hremote",  4},
        SCPI_CHOICE_LIST_END
    };

    int32_t selectedcompmode;
    if (!SCPI_ParamChoice(context, compmodeChoices, &selectedcompmode, true)) {return SCPI_RES_ERR;}
    qCDebug(scpi) << "Channel:" << channel << "Set COMPMODE to:" << selectedcompmode;
    quint8 param = selectedcompmode;
    QByteArray data(1, param);

    auto* self = static_cast<ScpiManager*>(context->user_context);
    switch (channel){
        case 1:
            emit self->sendFrame_Uart4(0x01,0x09,data);break;
        case 2:
            emit self->sendFrame_Uart5(0x01,0x09,data);break;
        default:
            break;
    }

    return SCPI_RES_OK;
}

// --- 查询 -command function ---

scpi_result_t ScpiManager::SCPI_OutputStateQ(scpi_t* context) {
    int32_t channel;
    if(!SCPI_CommandNumbers(context, &channel, 1, 0)){return SCPI_RES_ERR;} // Array 1, Default Channel 0
    qCDebug(scpi)<<"SCPI_Output Channel: "<<channel;

    auto* self = static_cast<ScpiManager*>(context->user_context);
    QMutexLocker locker(&self->m_syncMutex);
    self->m_UartResponse_Return = false;

    switch (channel){
        case 1:
            emit self->sendFrame_Uart4(0x01,0x80,"");break;
        case 2:
            emit self->sendFrame_Uart5(0x01,0x80,"");break;
        default:
            break;
    }

    self->m_syncCondition.wait(&self->m_syncMutex, 600);
    if (!self->m_UartResponse_Return) {return SCPI_RES_ERR;}

    SCPI_ResultBool(context, self->m_CHStateReturn);
    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::SCPI_OutputBandQ(scpi_t* context) {
    int32_t channel;
    if(!SCPI_CommandNumbers(context, &channel, 1, 0)){return SCPI_RES_ERR;} // Array 1, Default Channel 0
    qCDebug(scpi)<<"SCPI_Output Channel: "<<channel;

    auto* self = static_cast<ScpiManager*>(context->user_context);
    QMutexLocker locker(&self->m_syncMutex);
    self->m_UartResponse_Return = false;

    switch (channel){
        case 1:
            emit self->sendFrame_Uart4(0x01,0x88,"");break;
        case 2:
            emit self->sendFrame_Uart5(0x01,0x88,"");break;
        default:
            break;
    }

    self->m_syncCondition.wait(&self->m_syncMutex, 600);
    if (!self->m_UartResponse_Return) {return SCPI_RES_ERR;}

    if (self->m_CHStateReturn){
        SCPI_ResultMnemonic(context, "HIGH");
    }else {
        SCPI_ResultMnemonic(context, "LOW");
    }

    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::SCPI_OutputCompModeQ(scpi_t* context) {
    int32_t channel;
    if(!SCPI_CommandNumbers(context, &channel, 1, 0)){return SCPI_RES_ERR;} // Array 1, Default Channel 0
    qCDebug(scpi)<<"SCPI_Output Channel: "<<channel;

    auto* self = static_cast<ScpiManager*>(context->user_context);
    QMutexLocker locker(&self->m_syncMutex);
    self->m_UartResponse_Return = false;

    switch (channel){
        case 1:
            emit self->sendFrame_Uart4(0x01,0x89,"");break;
        case 2:
            emit self->sendFrame_Uart5(0x01,0x89,"");break;
        default:
            break;
    }

    self->m_syncCondition.wait(&self->m_syncMutex, 600);
    if (!self->m_UartResponse_Return) {return SCPI_RES_ERR;}

    int mode = self->m_CHvalueReturn;
    switch (mode) {
        case 1:
            SCPI_ResultMnemonic(context, "Llocal");break;

        case 2:
            SCPI_ResultMnemonic(context, "Lremote");break;

        case 3:
            SCPI_ResultMnemonic(context, "Hlocal");break;

        case 4:
            SCPI_ResultMnemonic(context, "Hremote");break;

        default:
            break;
    }

    return SCPI_RES_OK;
}

// --- 查询写入 -command Auxiliary function ---

void ScpiManager::processCHStateResponse(bool state) {
    m_syncMutex.lock();
    m_UartResponse_Return = true;

    m_CHStateReturn = state;
    m_syncMutex.unlock();

    m_syncCondition.wakeAll();
}

void ScpiManager::processCHvalueResponse(float value) {
    m_syncMutex.lock();
    m_UartResponse_Return = true;

    m_CHvalueReturn = value;
    m_syncMutex.unlock();

    m_syncCondition.wakeAll();
}

// ======================== 初化化模块 =================================

QByteArray ScpiManager::processCommand(const QByteArray &command) {
    if (command.isEmpty()){ return QByteArray();}
    m_responseBuffer.clear();

    // Support streaming input
    SCPI_Input(&m_scpiContext, command.constData(), command.size());
    return m_responseBuffer; // if not query，return emtry
}

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
