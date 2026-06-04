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
    if (m_modelManager) {
        QTimer::singleShot(0, this, [this]() {
            if (m_modelManager->loadAllModels()) {
                m_currentModelList = m_modelManager->getAvailableModels();
                if (!m_currentModelList.isEmpty()) {
                    #define CHANNEL(n) \
                    mCH##n##_batteryModel = m_currentModelList[0]; \
                    mCH##n##_activeModel = m_modelManager->getModel(mCH##n##_batteryModel); \
                    emit CH##n##_BatteryModelChanged();
                    CHANNEL_COUNT
                    #undef CHANNEL
                }}});
    }
}

// ============================  Internal trigger function =============================

void GuiBridge::update_SoftVer(int ch,const QString &ver){
    switch(ch) {
        #define CHANNEL(n) \
        case n: \
            mCH##n##_sv = ver; \
            emit CH##n##_svChanged(); \
            return;/*qCDebug(uart_bridge) << "Channel" << n << "SV updated to:" << ver;*/

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
            return;/*qCDebug(uart_bridge) << "Channel" << n << "HV updated to:" << ver;*/

        CHANNEL_COUNT
        #undef CHANNEL
        default: return;
    }
}

void GuiBridge::update_Voltage(int ch,float voltage){
    switch(ch) {
        #define CHANNEL(n) \
            case n: \
                mCH##n##_Voltage.store(voltage); \
                emit CH##n##_VoltageChanged(); \
                \
                if(mCH##n##_enablebattery.load()){\
                    float newocv = mCH##n##_activeModel->getOCV(mCH##n##_currentSOC); \
                    QByteArray ocvbuffer(reinterpret_cast<const char*>(&newocv), sizeof(float)); \
                    std::reverse(ocvbuffer.begin(), ocvbuffer.end()); \
                    to_Channel(ch,0x02, 0x00, ocvbuffer); \
                    \
                    float newesr = mCH##n##_activeModel->getESR(mCH##n##_currentSOC); \
                    QByteArray esrbuffer(reinterpret_cast<const char*>(&newesr), sizeof(float)); \
                    std::reverse(esrbuffer.begin(), esrbuffer.end()); \
                    to_Channel(ch,0x02, 0x02, esrbuffer); \
                    \
                    if (mCH##n##_activeModel->isOver(mCH##n##_currentSOC)){ \
                        mCH##n##_enablebattery.store(false); \
                    } \
                } \
                return;/*qCDebug(uart_bridge) << "Channel" << n << "voltage updated to:" << voltage;*/
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
                if (mCH##n##_CurrentUnit != newUnit) { \
                    mCH##n##_CurrentUnit = newUnit; \
                    emit CH##n##_CurrentUnitChanged(); \
                } \
                mCH##n##_Current.store((qAbs(current) < 1e-4) ? current * 1000.0f : current); \
                emit CH##n##_CurrentChanged(); \
                \
                if(mCH##n##_enablebattery.load()){\
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
                    \
                    emit CH##n##_CurrentSOCChanged(); \
                }\
                return; \
            }/*qCDebug(uart_bridge) << "Channel" << n << "current updated to:" << current;*/
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
            //m_GPIBid = val;
            //emit gpibId_Changed();
            //ConfigManager::setConfigValue("Device/GPIBID",val);
            //emit to_GPIBid(val);
            return;
        }
        case 3:{
            // can
            m_CANid = val;
            emit to_CANid(val);
            emit canId_Changed();
            ConfigManager::s_CANid = val;
            ConfigManager::setConfigValue("Device/CANID",val);
            return;
        }
    }
}

// =========================== Screen trigger function ===========================

QString GuiBridge::setChannel_CurrentUnit(int channel){
    quint8 unitCode; QString unit;
    static int step = 0;

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

void GuiBridge::setChannel_Output(int channel,bool switchs){
    quint8 func = switchs ? 0x01 : 0x00;
    qCDebug(uart_bridge) << "setChannel_Output - channel:" << channel << "switch:" << switchs;
    return to_Channel(channel,0x01, func, "");
}

void GuiBridge::setChannel_Setstatus(int channel,int model,const QString& val){
    // model: 0 - CV ; 1 - CC ; 3 - OVP;
    float value = val.toFloat();
    QByteArray buffer(reinterpret_cast<const char*>(&value), sizeof(float));
    std::reverse(buffer.begin(), buffer.end()); // big-endian order

    return to_Channel(channel,0x02, model, buffer);
}

void GuiBridge::setChannel_BatteryOutput(int channel,bool switchs){
    if (m_currentModelList.isEmpty()) {return;}
    //if (QSysInfo::ByteOrder == QSysInfo::LittleEndian) {}

    switch(channel) {
        #define CHANNEL(n) \
            case n:{ \
                if (mCH##n##_batterystaticmode){ \
                    float newocv = mCH##n##_activeModel->getOCV(mCH##n##_currentSOC); \
                    QByteArray ocvbuffer(reinterpret_cast<const char*>(&newocv), sizeof(float)); \
                    std::reverse(ocvbuffer.begin(), ocvbuffer.end()); \
                    to_Channel(channel,0x02, 0x00, ocvbuffer); \
                    \
                    float newesr = mCH##n##_activeModel->getESR(mCH##n##_currentSOC); \
                    QByteArray esrbuffer(reinterpret_cast<const char*>(&newesr), sizeof(float)); \
                    std::reverse(esrbuffer.begin(), esrbuffer.end()); \
                    to_Channel(channel,0x02, 0x02, esrbuffer); \
                    \
                } else{ \
                    mCH##n##_enablebattery.store(switchs); \
                } \
                setChannel_Output(channel,switchs); \
                mCH##n##_integralTimer.restart(); \
                return; \
            }/*qCDebug(uart_bridge)<<"newesr"<<newesr<<",mCH##n##_imp"<<mCH##n##_imp;*/
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

void GuiBridge::setChannel_Batterymode(int channel,bool staticmode){
    if (channel == 0) {
        #define CHANNEL(n) mCH##n##_batterystaticmode.store(staticmode);
        CHANNEL_COUNT
        #undef CHANNEL
        return;
    }

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

QString GuiBridge::setChannel_BatteryModel(int channel){
    if (m_currentModelList.isEmpty()) {return "";}

    static int currentIndex = 0;
    currentIndex = (currentIndex + 1) % m_currentModelList.size();

    if (channel == 0) {
        #define CHANNEL(n)  \
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

QVariantList GuiBridge::getActiveChannels()
{
    QVariantList channels;
    #define CHANNEL(n) if(mCH##n##_sv == "0.0.0.0"){channels.append(n);}
    CHANNEL_COUNT
    #undef CHANNEL

    return channels;
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

