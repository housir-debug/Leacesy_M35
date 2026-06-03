#include "qml_agency.h"
#include <QtCore>

Q_LOGGING_CATEGORY(uart_bridge, "UART_BRIDGE:")

QJsonArray GuiBridge::getAllChannelsData() {
    QJsonArray channels;
    #define CHANNEL(n) \
        do { \
            if (mCH##n##_sv != "0.0.0.0") { \
                QJsonObject channel; \
                channel["channel"] = n; \
                channel["isOutput"] = mCH##n##_isOutput.load(); \
                channel["voltage"] = mCH##n##_Voltage.load(); \
                channel["current"] = mCH##n##_Current.load(); \
                channel["current_unit"] = mCH##n##_CurrentUnit; \
                channel["cvSetpoint"] = mCH##n##_cv.load(); \
                channel["ccSetpoint"] = mCH##n##_cc.load(); \
                channel["ovSetpoint"] = mCH##n##_ovp.load(); \
                channel["status"] = mCH##n##_Status; \
                channels.append(channel); \
        }} while(0);

    CHANNEL_COUNT
    #undef CHANNEL
    return channels;
}

GuiBridge::GuiBridge(QObject *parent) : QObject(parent) {
    m_SoftVer = ConfigManager::s_firmwareVersion;
    m_HardVer = ConfigManager::s_hardwareVersion;

    m_IPaddress = ConfigManager::s_IP;
    m_SM = ConfigManager::s_SM;
    m_GPIBid = ConfigManager::s_GPIBid;
    m_CANid = ConfigManager::s_CANid;
}

void GuiBridge::load_BatteryModel(){
    if (m_modelManager && m_modelManager->getAvailableModels().isEmpty()) {
        QTimer::singleShot(0, this, [this]() {
            if (m_modelManager->loadAllModels()) {
                m_currentModelList = m_modelManager->getAvailableModels();
                if (!m_currentModelList.isEmpty()) {
                    #define CHANNEL(n) \
                    mCH##n##_batteryModel = m_currentModelList[0]; \
                    mCH##n##_activeModel = m_modelManager->getModel(m_currentModelList[0]); \
                    emit CH##n##_BatteryModelChanged();
                    CHANNEL_COUNT
                    #undef CHANNEL
                }}});
    }
}


// ============================  C++ for qml engine =============================

void GuiBridge::update_Voltage(int ch,float voltage){
    switch(ch) {
        #define CHANNEL(n) \
            case n: \
                mCH##n##_Voltage.store(voltage); \
                emit CH##n##_VoltageChanged(); \
                qCDebug(uart_bridge) << "Channel" << n << "voltage updated to:" << voltage; \
                \
                if(mCH##n##_activebattery.load()){\
                    quint32 ocvint; \
                    float newocv = mCH##n##_activeModel->getOCV(mCH##n##_currentSOC); \
                    memcpy(&ocvint, &newocv, sizeof(float)); \
                    ocvint = qToBigEndian(ocvint); \
                    QByteArray Status_buffer(reinterpret_cast<const char*>(&ocvint), sizeof(quint32)); \
                    to_Channel(ch,0x02, 0x00, Status_buffer); \
                    mCH##n##_cv.store(newocv); \
                    emit CH##n##_cvChanged(); \
                    \
                    quint32 esrint; \
                    float newesr = mCH##n##_activeModel->getESR(mCH##n##_currentSOC); \
                    memcpy(&esrint, &newesr, sizeof(float)); \
                    esrint = qToBigEndian(esrint); \
                    QByteArray Statusub_buffer(reinterpret_cast<const char*>(&esrint), sizeof(quint32)); \
                    to_Channel(ch,0x02, 0x02, Statusub_buffer); \
                    mCH##n##_imp.store(esrint); \
                    emit CH##n##_impChanged(); \
                    \
                    if (mCH##n##_activeModel->isOver(mCH##n##_currentSOC)){ \
                        mCH##n##_activebattery.store(false); \
                        mCH##n##_timerStarted.store(false); \
                    } \
                } \
                return;
        CHANNEL_COUNT
        #undef CHANNEL
        default: return;
    }
}

void GuiBridge::update_CurrentAndUnit(int ch,float current){
    QString newUnit = (qAbs(current) < 1e-4) ? "mA" : "A"; // true: mA   false: A

    switch(ch) {
        #define CHANNEL(n) \
            case n: { \
                mCH##n##_Current.store((qAbs(current) < 1e-4) ? current * 1000.0f : current); \
                emit CH##n##_CurrentChanged(); \
                qCDebug(uart_bridge) << "Channel" << n << "current updated to:" << current; \
                \
                if (mCH##n##_CurrentUnit != newUnit) { \
                    mCH##n##_CurrentUnit = newUnit; \
                    emit CH##n##_CurrentUnitChanged(); \
                } \
                \
                if(mCH##n##_activebattery.load()){\
                    if (!mCH##n##_timerStarted.load()){ \
                        mCH##n##_timerStarted.store(true); \
                        mCH##n##_integralTimer.restart(); \
                        return; \
                    } \
                    \
                    qint64 elapsedMs = mCH##n##_integralTimer.elapsed();\
                    float deltaTimeHours = elapsedMs / 3600000.0f; \
                    float capacityAH = mCH##n##_capacityAH.load(); \
                    float deltaSOC = (current * deltaTimeHours * 100) / capacityAH; \
                    float currentsoc = mCH##n##_currentSOC.load(); \
                    if (deltaSOC < 0 && qAbs(deltaSOC)>=currentsoc){ \
                        mCH##n##_currentSOC.store(0.0f); \
                        qCDebug(uart_bridge) << "Battery depleted!"; \
                    } \
                    else if(deltaSOC > 0 && (currentsoc+deltaSOC)>=100.0f){ \
                        mCH##n##_currentSOC.store(100.0f); \
                        qDebug() << "Battery fully charged!"; \
                    } \
                    else{ \
                        mCH##n##_currentSOC.store(currentsoc+deltaSOC); \
                        mCH##n##_integralTimer.restart(); \
                    } \
                    emit CH##n##_CurrentSOCChanged(); \
                }\
                return; \
            }
        CHANNEL_COUNT
        #undef CHANNEL
        default: return;
    }
}

void GuiBridge::update_Status(int ch,quint16 status){
    QString binaryStr = QString("%1").arg(status, 16, 2, QLatin1Char('0'));

    switch(ch) {
        #define CHANNEL(n) \
            case n: \
                if (mCH##n##_Status != binaryStr) { \
                    mCH##n##_Status = binaryStr; \
                    emit CH##n##_StatusChanged(); \
                }return;

        CHANNEL_COUNT
        #undef CHANNEL
        default: return;
    }
}

void GuiBridge::update_Cv(int ch,float cv){
    switch(ch) {
        #define CHANNEL(n) \
            case n: \
                mCH##n##_cv.store(cv); \
                emit CH##n##_cvChanged(); \
                return;

        CHANNEL_COUNT
        #undef CHANNEL
        default: return;
    }
}

void GuiBridge::update_Cc(int ch,float cc){
    switch(ch) {
        #define CHANNEL(n) \
            case n: \
                mCH##n##_cc.store(cc); \
                emit CH##n##_ccChanged(); \
                return;

        CHANNEL_COUNT
        #undef CHANNEL
        default: return;
    }
}

void GuiBridge::update_Imp(int ch,float imp){
    switch(ch) {
        #define CHANNEL(n) \
            case n: \
                mCH##n##_imp.store(imp); \
                emit CH##n##_impChanged(); \
                return;

        CHANNEL_COUNT
        #undef CHANNEL
        default: return;
    }
}

void GuiBridge::update_Ovp(int ch,float ovp){
    switch(ch) {
        #define CHANNEL(n) \
            case n: \
                mCH##n##_ovp.store(ovp); \
                emit CH##n##_ovpChanged(); \
                return;

        CHANNEL_COUNT
        #undef CHANNEL
        default: return;
    }
}

void GuiBridge::update_IsOutput(int ch,bool status){
    switch(ch) {
        #define CHANNEL(n) \
            case n: \
                mCH##n##_isOutput.store(status); \
                emit CH##n##_isOutputChanged(); \
                return;

        CHANNEL_COUNT
        #undef CHANNEL
        default: return;
    }
}

void GuiBridge::update_SoftVer(int ch,const QString &ver){
    switch(ch) {
        #define CHANNEL(n) \
            case n: \
                mCH##n##_sv = ver; \
                emit CH##n##_svChanged(); \
                qCDebug(uart_bridge) << "Channel" << n << "SV updated to:" << ver; \
                return;

        CHANNEL_COUNT
        #undef CHANNEL
        default: return;
    }
}

void GuiBridge::update_HardVer(int ch,const QString &ver){
    switch(ch) {
        #define CHANNEL(n) \
            case n: \
                mCH##n##_hv = ver; \
                emit CH##n##_hvChanged(); \
                qCDebug(uart_bridge) << "Channel" << n << "HV updated to:" << ver; \
                return;

        CHANNEL_COUNT
        #undef CHANNEL
        default: return;
    }
}

// =========================== Q_INVOKABLE And C++ ===========================

void GuiBridge::update_remotemodel(quint8 reface){
    if (m_remoteStatus.load() != reface){
        m_remoteStatus.store(reface);
        emit isRemote_Changed();
    }
}

void GuiBridge::update_Configuration(int model,const QString& val){
    switch(model){
        case 0:{
            // IP
            if(ConfigManager::refresh_interfaces(val,m_SM)){
                m_IPaddress = val;
                emit ipAdress_Changed();
                ConfigManager::s_IP = val;
            }
            return;
        }
        case 1:{
            // SM
            if(ConfigManager::refresh_interfaces(m_IPaddress,val)){
                m_SM = val;
                emit sm_Changed();
                ConfigManager::s_SM = val;
            }
            return;
        }
        case 2:{
            // GPIB
            m_GPIBid = val;
            emit gpibId_Changed();
            ConfigManager::setConfigValue("Device/GPIBID",val);
            emit to_GPIBid(val);
            return;
        }
        case 3:{
            // can
            m_CANid = val;
            emit canId_Changed();
            ConfigManager::setConfigValue("Device/CANID",val);
            emit to_CANid(val);
            return;
        }
    }
}



// =========================== Q_INVOKABLE ===========================

QVariantList GuiBridge::getActiveChannels()
{
    QVariantList channels;
    #define CHANNEL(n) if(mCH##n##_sv == "0.0.0.0"){channels.append(n);}
    CHANNEL_COUNT
    #undef CHANNEL

    return channels;
}

void GuiBridge::setChannel_Output(int channel,bool switchs){
    quint8 func = switchs ? 0x01 : 0x00;
    qCDebug(uart_bridge) << "setChannel_Output - channel:" << channel << "switch:" << switchs;
    return to_Channel(channel,0x01, func, "");
}

void GuiBridge::setChannel_Setstatus(int channel,int model,const QString& val){
    // model: 0 - CV ; 1 - CC ; 3 - OVP;
    quint32 intValue;
    float value = val.toFloat();
    memcpy(&intValue, &value, sizeof(float));
    intValue = qToBigEndian(intValue);
    QByteArray Status_buffer(reinterpret_cast<const char*>(&intValue), sizeof(quint32));

    return to_Channel(channel,0x02, model, Status_buffer);
}

QString GuiBridge::setChannel_CurrentUnit(int channel){
    static int step = 0;
    QString unit;
    quint8 unitCode;

    switch (step) {
        case 0:  unitCode = 0x01;unit = "mA";   break;
        case 1:  unitCode = 0x10;unit = "Auto"; break;
        case 2:  unitCode = 0x00;unit = "A";    break;
        default: unitCode = 0x00;unit = "A";    break;
    }
    step = (step + 1) % 3;

    QByteArray Unit_buffer;
    Unit_buffer.append(char(unitCode));
    to_Channel(channel,0x04, 0x0E, Unit_buffer);

    qCDebug(uart_bridge) << "Current unit changed to:" << unit;
    return unit;
}

void GuiBridge::setChannel_BatteryOutput(int channel,bool switchs){
    switch(channel) {
        #define CHANNEL(n) \
            case n:{ \
                if (mCH##n##_batterystaticmode){ \
                    quint32 ocvint; \
                    float newocv = mCH##n##_activeModel->getOCV(mCH##n##_currentSOC); \
                    memcpy(&ocvint, &newocv, sizeof(float)); \
                    ocvint = qToBigEndian(ocvint); \
                    QByteArray Status_buffer(reinterpret_cast<const char*>(&ocvint), sizeof(quint32)); \
                    to_Channel(channel,0x02, 0x00, Status_buffer); \
                    mCH##n##_cv.store(newocv); \
                    emit CH##n##_cvChanged(); \
                    \
                    quint32 esrint; \
                    float newesr = mCH##n##_activeModel->getESR(mCH##n##_currentSOC); \
                    memcpy(&esrint, &newesr, sizeof(float)); \
                    esrint = qToBigEndian(esrint); \
                    QByteArray Statusub_buffer(reinterpret_cast<const char*>(&esrint), sizeof(quint32)); \
                    to_Channel(channel,0x02, 0x02, Statusub_buffer); \
                    mCH##n##_imp.store(esrint); \
                    emit CH##n##_impChanged(); \
                } else{ \
                    mCH##n##_activebattery.store(switchs); \
                    mCH##n##_timerStarted.store(false); \
                } \
                \
                setChannel_Output(channel,switchs); \
                return; \
            }
        CHANNEL_COUNT
        #undef CHANNEL
        default:
            qCWarning(uart_bridge) << "Invalid channel:" << channel;
            return;
    }
}

void GuiBridge::setChannel_InitSOC(int channel,const QString& val){
    float value = val.toFloat();

    // All channel
    if (channel == 0) {
        #define CHANNEL(n)  \
        mCH##n##_currentSOC.store(value); \
        emit CH##n##_CurrentSOCChanged();
        CHANNEL_COUNT
        #undef CHANNEL
        return;
    }

    // Single channel send
    switch(channel) {
        #define CHANNEL(n) \
            case n: \
                mCH##n##_currentSOC.store(value); \
                emit CH##n##_CurrentSOCChanged(); \
                return;
        CHANNEL_COUNT
        #undef CHANNEL
        default:
            qCWarning(uart_bridge) << "Invalid channel:" << channel;
            return;
    }

}

void GuiBridge::setChannel_Capacity(int channel,const QString& val){
    float value = val.toFloat();

    // All channel
    if (channel == 0) {
        #define CHANNEL(n)  \
        mCH##n##_capacityAH.store(value); \
        emit CH##n##_CapacityAHChanged();
        CHANNEL_COUNT
        #undef CHANNEL
        return;
    }

    // Single channel send
    switch(channel) {
        #define CHANNEL(n) \
            case n: \
                mCH##n##_capacityAH.store(value); \
                emit CH##n##_CapacityAHChanged(); \
                return;
        CHANNEL_COUNT
        #undef CHANNEL
        default:
            qCWarning(uart_bridge) << "Invalid channel:" << channel;
            return;
    }

}

QString GuiBridge::setChannel_BatteryModel(int channel){
    if (m_currentModelList.isEmpty()) return "";
    static int currentIndex = 0;
    //int mCH##n##currentIndex = m_currentModelList.indexOf(mCH##n##_batteryMode);
    //int mCH##n##nextIndex = (mCH##n##currentIndex + 1) % m_currentModelList.size();
    //                mCH##n##_batteryMode =  m_currentModelList[mCH##n##nextIndex];

    if (channel == 0) {
        #define CHANNEL(n)  \
        currentIndex = (currentIndex + 1) % m_currentModelList.size(); \
        mCH##n##_batteryModel =  m_currentModelList[currentIndex]; \
        mCH##n##_activeModel = m_modelManager->getModel(mCH##n##_batteryModel); \
        emit CH##n##_BatteryModelChanged();
        CHANNEL_COUNT
        #undef CHANNEL
        return mCH1_batteryModel;
    }

    // Single channel send
    switch(channel) {
        #define CHANNEL(n) \
            case n:{ \
                currentIndex = (currentIndex + 1) % m_currentModelList.size(); \
                mCH##n##_batteryModel =  m_currentModelList[currentIndex]; \
                mCH##n##_activeModel = m_modelManager->getModel(mCH##n##_batteryModel); \
                emit CH##n##_BatteryModelChanged(); \
                return mCH##n##_batteryModel; \
            }
        CHANNEL_COUNT
        #undef CHANNEL
        default:
            qCWarning(uart_bridge) << "Invalid channel:" << channel;
            return "";
    }
}

void GuiBridge::setChannel_Batterymode(int channel,bool staticmode){
    switch(channel) {
        #define CHANNEL(n) \
            case n:{ \
                mCH##n##_batterystaticmode.store(staticmode); \
                return; \
            }
        CHANNEL_COUNT
        #undef CHANNEL
        default:
            qCWarning(uart_bridge) << "Invalid channel:" << channel;
            return;
    }
}

void GuiBridge::to_Channel(int channel,quint8 cmd,quint8 func,const QByteArray& param){
    // All channel send
    if (channel == 0) {
        #define CHANNEL(n) emit to_UartChannel##n(cmd, func, param,false);
        CHANNEL_COUNT
        #undef CHANNEL
        return;
    }

    // Single channel send
    switch(channel) {
        #define CHANNEL(n) case n: return emit to_UartChannel##n(cmd, func, param,false);
        CHANNEL_COUNT
        #undef CHANNEL
        default:
            qCWarning(uart_bridge) << "Invalid channel:" << channel;
            return;
    }
}

