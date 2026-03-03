#include "uartmanager.h"
#include <QtCore>

Q_LOGGING_CATEGORY(uart_bridge, "UART_BRIDGE:")

SerialBridge::SerialBridge(QObject *parent) : QObject(parent) {}

// =========================== Qml调用处理 ===========================

void SerialBridge::setChannel_Output(int channel,bool switchs){
    qCDebug(uart_bridge) << "setChannel_Output - channel:" << channel << "switch:" << switchs;
    quint8 func = switchs ? 0x01 : 0x00;

    if (channel == 0) {
        return toAll_Channel(0x01, func, "");
    }

    switch(channel) {
        #define CHANNEL(n) case n: return emit to_UartChannel##n(0x01, func, "");
        CHANNEL_1_TO_33
        #undef CHANNEL
        default:
            qCWarning(uart_bridge) << "Invalid channel:" << channel;
            return;
    }
}

void SerialBridge::setChannel_Setstatus(int channel,int model,float value){
    quint32 intValue;
    memcpy(&intValue, &value, sizeof(float));
    intValue = qToBigEndian(intValue);

    QByteArray Status_buffer;
    Status_buffer.append(reinterpret_cast<const char*>(&intValue),sizeof(quint32));

    quint8 func = 0x00;
    switch (model) {
        case 1: func = 0x00; break;  // CV
        case 2: func = 0x01; break;  // CC
        case 3: func = 0x03; break;  // OVP
        default:
            qCWarning(uart_bridge) << "Invalid model:" << model;
            return;
    }

    if (channel == 0) {
        return toAll_Channel(0x02, func, Status_buffer);
    }

    switch(channel) {
        #define CHANNEL(n) case n: return emit to_UartChannel##n(0x02, func, Status_buffer);
        CHANNEL_1_TO_33
        #undef CHANNEL
        default:
            qCWarning(uart_bridge) << "Invalid channel:" << channel;
            return;
    }
}

QString SerialBridge::setChannel_CurrentUnit(){
    static int step = 0;
    QString unit;
    quint8 unitCode;

    switch (step) {
        case 0: unit = "mA"; unitCode = 0x01; break;
        case 1: unit = "Auto"; unitCode = 0x10; break;
        case 2: unit = "A"; unitCode = 0x00; break;
        default: unit = "A"; unitCode = 0x00; break;
    }

    QByteArray Unit_buffer;
    Unit_buffer.append(char(unitCode));
    toAll_Channel(0x04, 0x0E, Unit_buffer);

    step = (step + 1) % 3;
    qCDebug(uart_bridge) << "Current unit changed to:" << unit;
    return unit;
}

// - Auxiliary function ----------

void SerialBridge::toAll_Channel(quint8 cmd,quint8 func,const QByteArray& param){
    #define CHANNEL(n) emit to_UartChannel##n(cmd, func, param);
    CHANNEL_1_TO_33
    #undef CHANNEL
}

// Modify the corresponding channel information individually
// ============================  槽函数  =============================

void SerialBridge::update_Voltage(int ch,float voltage){
    switch(ch) {
        #define CHANNEL(n) \
            case n: \
                mCH##n##_Voltage = voltage; \
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
                mCH##n##_Current =  newUnit ? current * 1000.0f : current; \
                emit CH##n##_CurrentChanged(); \
                qCDebug(uart_bridge) << "Channel" << n << "current updated to:" << mCH##n##_Current; \
                \
                if (mCH##n##_Current_Unit != newUnit) { \
                    mCH##n##_Current_Unit = newUnit; \
                    emit CH##n##_Current_Unit_Changed(); \
                } \
                return; \
            }
        CHANNEL_1_TO_33
        #undef CHANNEL
        default: return;
    }
}

void SerialBridge::update_status(int ch,QByteArray status){
    if (status.size() < 2) {
        qCWarning(uart_bridge) << "update_status - status size too small:" << status.size();
        return;
    }

    quint16 value = (static_cast<quint8>(status[0]) << 8) | static_cast<quint8>(status[1]);
    QString binaryStr = QString("%1").arg(value, 16, 2, QLatin1Char('0'));

    switch(ch) {
        #define CHANNEL(n) \
            case n: { \
                if (mCH##n##_status == binaryStr) {return;} \
                \
                mCH##n##_status = binaryStr; \
                qCDebug(uart_bridge) << "Channel" << n << "status changed to:" << binaryStr; \
                \
                if (binaryStr.length() > 14 && binaryStr[14] == '1') { \
                    mCH##n##_status_v = 1;  /* CV */ \
                    qCDebug(uart_bridge) << "Channel" << n << "mode: CV"; \
                } else if (binaryStr.length() > 13 && binaryStr[13] == '1') { \
                    mCH##n##_status_v = 2;  /* CC */ \
                    qCDebug(uart_bridge) << "Channel" << n << "mode: CC"; \
                } else if (binaryStr.length() > 11 && binaryStr[11] == '1') { \
                    mCH##n##_status_v = 3;  /* OV */ \
                    qCDebug(uart_bridge) << "Channel" << n << "mode: OV"; \
                } \
                \
                emit CH##n##_StatusChanged(); \
                return; \
            }
        CHANNEL_1_TO_33
        #undef CHANNEL
        default: return;
    }
}
