#include "qml_agency.h"
#include <QtCore>

Q_LOGGING_CATEGORY(uart_bridge, "UART_BRIDGE:")

SerialBridge::SerialBridge(QObject *parent) : QObject(parent) {}

// Modify the corresponding channel information individually
// ============================  C++for qml engine =============================

void SerialBridge::update_Voltage(int ch,float voltage){
    switch(ch) {
        #define CHANNEL(n) \
            case n: \
                mCH##n##_Voltage.store(voltage); \
                emit CH##n##_VoltageChanged(); \
                qCDebug(uart_bridge) << "Channel" << n << "voltage updated to:" << voltage; \
                return;
        CHANNEL_1_TO_33
        #undef CHANNEL
        default: return;
    }
}

void SerialBridge::update_CurrentAndUnit(int ch,float current){
    QString newUnit = (qAbs(current) < 1e-4) ? "mA" : "A"; // true: mA   false: A

    switch(ch) {
        #define CHANNEL(n) \
            case n: { \
                mCH##n##_Current.store((qAbs(current) < 1e-4) ? current * 1000.0f : current); \
                emit CH##n##_CurrentChanged(); \
                qCDebug(uart_bridge) << "Channel" << n << "current updated to:" << current; \
                \
                if (mCH##n##_CurrentUnit != newUnit) { \
                    mCH##n##_CurrentUnit == newUnit; \
                    emit CH##n##_CurrentUnitChanged(); \
                } \
                return; \
            }
        CHANNEL_1_TO_33
        #undef CHANNEL
        default: return;
    }
}

void SerialBridge::update_status(int ch,const QByteArray& status){
    if (status.size() < 2) {
        qCCritical(uart_bridge) << "update_status - status size too small:" << status.size();
        return;
    }

    quint16 value = (static_cast<quint8>(status[0]) << 8) | static_cast<quint8>(status[1]);
    QString binaryStr = QString("%1").arg(value, 16, 2, QLatin1Char('0'));

    switch(ch) {
        #define CHANNEL(n) \
            case n: { \
                if (mCH##n##_Status == binaryStr) {return;} \
                \
                mCH##n##_Status = binaryStr; \
                qCDebug(uart_bridge) << "Channel" << n << "status changed to:" << binaryStr; \
                emit CH##n##_StatusChanged(); \
                return; \
            }
        CHANNEL_1_TO_33
        #undef CHANNEL
        default: return;
    }
}

QJsonArray SerialBridge::getAllChannelsData() {
    QJsonArray channels;
    qCDebug(uart_bridge) << "update Web channels data.";

    #define CHANNEL(n) \
        do { \
            QJsonObject channel; \
            channel["channel"] = n; \
            channel["voltage"] = mCH##n##_Voltage.load(); \
            channel["current"] = mCH##n##_Current.load(); \
            channel["cvSetpoint"] = mCH##n##_cv.load(); \
            channel["ccSetpoint"] = mCH##n##_cc.load(); \
            channel["ovSetpoint"] = mCH##n##_ovp.load(); \
            channel["status"] = mCH##n##_Status; \
            channel["enabled"] = mCH##n##_isOutput.load(); \
            channel["current_unit"] = mCH##n##_CurrentUnit; \
            channels.append(channel); \
        } while(0);

    CHANNEL_1_TO_33
    #undef CHANNEL

    return channels;
}

void SerialBridge::update_Configuration(int model,const QString& val){
    Q_UNUSED(val)
    switch(model){
        case 0:{
            // IP-/etc/network/interfaces
            return;
        }
        case 1:{
            // SM-/etc/network/interfaces
            return;
        }
        case 2:{
            // GPIB
            return;
        }
        case 3:{
            // GPIB
            return;
        }
    }
}

// =========================== Q_INVOKABLE And C++ ===========================

void SerialBridge::update_remotemodel(bool isRemote){
    m_isRemote.store(isRemote);
    emit isRemote_Changed();
}


// =========================== Q_INVOKABLE ===========================

void SerialBridge::setChannel_Output(int channel,bool switchs){
    quint8 func = switchs ? 0x01 : 0x00;
    qCDebug(uart_bridge) << "setChannel_Output - channel:" << channel << "switch:" << switchs;
    return to_Channel(channel,0x01, func, "");
}

QString SerialBridge::setChannel_CurrentUnit(){
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
    to_Channel(0,0x04, 0x0E, Unit_buffer);

    qCDebug(uart_bridge) << "Current unit changed to:" << unit;
    return unit;
}

void SerialBridge::setChannel_Setstatus(int channel,int model,const QString& val){
    // model: 0 - CV ; 1 - CC ; 3 - OVP;
    float value = val.toFloat();

    if (model==0){
        if (channel == 0) {
            #define CHANNEL(n) \
            mCH##n##_cv = value; \
            qCDebug(uart_bridge) << "Channel" << n << "status(CV)Value changed to:" << value; \
            emit CH##n##_cvChanged();
            CHANNEL_1_TO_33
            #undef CHANNEL
        }else{
            switch(channel) {
                #define CHANNEL(n) \
                    case n: { \
                        mCH##n##_cv = value; \
                        qCDebug(uart_bridge) << "Channel" << n << "status(CV)Value changed to:" << value; \
                        emit CH##n##_cvChanged(); \
                        break; \
                    }
                CHANNEL_1_TO_33
                #undef CHANNEL
                default: break;
            }
        }
    }else if(model==1){
        if (channel == 0) {
            #define CHANNEL(n) \
            mCH##n##_cc = value; \
            qCDebug(uart_bridge) << "Channel" << n << "status(CC)Value changed to:" << value; \
            emit CH##n##_ccChanged();
            CHANNEL_1_TO_33
            #undef CHANNEL
        }else{
            switch(channel) {
                #define CHANNEL(n) \
                    case n: { \
                        mCH##n##_cc = value; \
                        qCDebug(uart_bridge) << "Channel" << n << "status(CC)Value changed to:" << value; \
                        emit CH##n##_ccChanged(); \
                        break; \
                    }
                CHANNEL_1_TO_33
                #undef CHANNEL
                default: break;
            }
        }
    }else if(model==3) {
        if (channel == 0) {
            #define CHANNEL(n) \
            mCH##n##_ovp = value; \
            qCDebug(uart_bridge) << "Channel" << n << "status(OVP)Value changed to:" << value; \
            emit CH##n##_ovpChanged();
            CHANNEL_1_TO_33
            #undef CHANNEL
        }else{
            switch(channel) {
                #define CHANNEL(n) \
                    case n: { \
                        mCH##n##_ovp = value; \
                        qCDebug(uart_bridge) << "Channel" << n << "status(OVP)Value changed to:" << value; \
                        emit CH##n##_ovpChanged(); \
                        break; \
                    }
                CHANNEL_1_TO_33
                #undef CHANNEL
                default: break;
            }
        }
    }

    quint32 intValue;
    memcpy(&intValue, &value, sizeof(float));
    intValue = qToBigEndian(intValue);
    QByteArray Status_buffer(reinterpret_cast<const char*>(&intValue), sizeof(quint32));

    return to_Channel(channel,0x02, model, Status_buffer);
}

// - Auxiliary function ----------

void SerialBridge::to_Channel(int channel,quint8 cmd,quint8 func,const QByteArray& param){
    // All channel send
    if (channel == 0) {
        #define CHANNEL(n) emit to_UartChannel##n(cmd, func, param,false);
        CHANNEL_1_TO_33
        #undef CHANNEL
        return;
    }

    // Single channel send
    switch(channel) {
        #define CHANNEL(n) case n: return emit to_UartChannel##n(cmd, func, param,false);
        CHANNEL_1_TO_33
        #undef CHANNEL
        default:
            qCWarning(uart_bridge) << "Invalid channel:" << channel;
            return;
    }
}


