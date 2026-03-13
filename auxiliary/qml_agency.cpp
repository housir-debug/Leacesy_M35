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
    bool newUnit = (qAbs(current) < 1e-4); // true: mA   false: A

    switch(ch) {
        #define CHANNEL(n) \
            case n: { \
                mCH##n##_Current.store(newUnit ? current * 1000.0f : current); \
                emit CH##n##_CurrentChanged(); \
                qCDebug(uart_bridge) << "Channel" << n << "current updated to:" << current; \
                \
                if (mCH##n##_CurrentUnit.load() != newUnit) { \
                    mCH##n##_CurrentUnit.store(newUnit); \
                    emit CH##n##_CurrentUnit_Changed(); \
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
            channel["ovSetpoint"] = mCH##n##_ov.load(); \
            channel["status"] = mCH##n##_Status; \
            channel["enabled"] = mCH##n##_isOutput.load(); \
            channel["current_unit"] = mCH##n##_CurrentUnit.load(); \
            channels.append(channel); \
        } while(0);

    CHANNEL_1_TO_33
    #undef CHANNEL

    return channels;
}

void SerialBridge::update_remotemodel(bool is_remote){
    m_isRemote.store(is_remote);
    emit isRemote_Change();
}


// =========================== Q_INVOKABLE ===========================

void SerialBridge::setChannel_Output(int channel,bool switchs){
    quint8 func = switchs ? 0x01 : 0x00;
    qCDebug(uart_bridge) << "setChannel_Output - channel:" << channel << "switch:" << switchs;
    return to_Channel(channel,0x01, func, "");
}

void SerialBridge::setChannel_Setstatus(int channel,int model,float value){
    quint32 intValue;
    memcpy(&intValue, &value, sizeof(float));
    intValue = qToBigEndian(intValue);
    QByteArray Status_buffer(reinterpret_cast<const char*>(&intValue), sizeof(quint32));

    return to_Channel(channel,0x02, model, Status_buffer);
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


