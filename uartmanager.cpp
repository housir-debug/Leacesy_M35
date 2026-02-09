#include "uartmanager.h"
#include <QtCore>

Q_LOGGING_CATEGORY(uart_bridge, "UART_BRIDGE:")

SerialBridge::SerialBridge(QObject *parent) : QObject(parent) {}

// =========================== Qml调用处理 ===========================

void SerialBridge::setChannel_Output(int channel,bool switchs){
    qDebug(uart_bridge) << "onChannel_2_Toggled当前状态为："<<switchs;
    quint8 func = switchs ? 0x01 : 0x00;

    switch (channel) {
    case 1: // ch_1
        return emit sendFrame_Uart4(0x01,func,"");
    case 2: // ch_2
        return emit sendFrame_Uart5(0x01,func,"");
    default:// all
        return toAll_Channel(0x01,func,"");
    }
}

void SerialBridge::setChannel_Setstatus(int channel,int model,float value){
    quint32 intValue;
    memcpy(&intValue, &value, sizeof(float));
    intValue = qToBigEndian(intValue);

    m_Status_buffer.clear();
    m_Status_buffer.append(reinterpret_cast<const char*>(&intValue),sizeof(quint32));

    switch (channel){
        case 1: // ch_1
            switch (model){
            case 1:// cv
                return emit sendFrame_Uart4(0x02,0x00,m_Status_buffer);
            case 2:// cc
                return emit sendFrame_Uart4(0x02,0x01,m_Status_buffer);
            case 3:// ovp
                return emit sendFrame_Uart4(0x02,0x03,m_Status_buffer);
            default:
                return;
            }
        case 2: // ch_2
            switch (model){
            case 1:// cv
                return emit sendFrame_Uart5(0x02,0x00,m_Status_buffer);
            case 2:// cc
                return emit sendFrame_Uart5(0x02,0x01,m_Status_buffer);
            case 3:// ovp
                return emit sendFrame_Uart5(0x02,0x03,m_Status_buffer);
            default:
                return;
            }
        default: // all
            switch (model){
            case 1:// cv
                return toAll_Channel(0x02,0x00,m_Status_buffer);
            case 2:// cc
                return toAll_Channel(0x02,0x01,m_Status_buffer);
            case 3:// ovp
                return toAll_Channel(0x02,0x03,m_Status_buffer);
            default:
                return;
            }
    }
}

QString SerialBridge::setChannel_CurrentUnit(){
    QString unit = "";

    m_Unit_buffer.clear();
    static int step = 0;
    switch (step) {
        case 0:
            unit = "mA";
            m_Unit_buffer.append(0x01);    // default A ->mA
            break;
        case 1:
            unit = "Auto";
            m_Unit_buffer.append(0x10);    // mA ->auto
            break;
        case 2:
            unit = "A";
            m_Unit_buffer.append(1,0x00);  // auto ->A
            break;
    }
    toAll_Channel(0x04,0x0E,m_Unit_buffer);
    step = (step + 1) % 3;

    return unit;
}

// - Auxiliary function ----------

void SerialBridge::toAll_Channel(quint8 cmd,quint8 func,QByteArray param){
    emit sendFrame_Uart4(cmd,func,param);
    emit sendFrame_Uart5(cmd,func,param);
}

// ============================  槽函数  =============================

void SerialBridge::update_Voltage(int ch,float voltage){
    switch (ch){
    case 1:
        mCH1_Voltage = voltage;
        emit CH1_VoltageChanged();
        return;
    case 2:
        mCH2_Voltage = voltage;
        emit CH2_VoltageChanged();
        return;
    }
}

void SerialBridge::update_CurrentAndUnit(int ch,float current){
    bool newUnit = (qAbs(current) < 1e-4); // true: mA   false: A
    if (newUnit) {current *= 1000.0f;}

    switch (ch){
        case 1:
            mCH1_Current = current;
            emit CH1_CurrentChanged();
            if (mCH1_Current_Unit != newUnit){ // true: mA   false: A
                mCH1_Current_Unit = !mCH1_Current_Unit;
                emit CH1_Current_Unit_Changed();
            }
            return;
        case 2:
            mCH2_Current = current;
            emit CH2_CurrentChanged();
            if (mCH2_Current_Unit != newUnit){
                mCH2_Current_Unit = !mCH2_Current_Unit;
                emit CH2_Current_Unit_Changed();
            }
            return;
    }
}

void SerialBridge::update_status(int ch,QByteArray status){
    if (status.size() < 2) {return;}

    quint16 value = (static_cast<quint8>(status[0]) << 8) | static_cast<quint8>(status[1]);
    QString binaryStr = QString("%1").arg(value, 16, 2, QLatin1Char('0'));

    switch (ch){
        case 1:
            if (mCH1_status != binaryStr) {
                mCH1_status = binaryStr;
                qCDebug(uart_bridge) << "事件状态改变为: " << binaryStr;

                mCH1_status_v = 0;
                if (mCH1_status[14] == "1"){   // cv
                    mCH1_status_v = 1;
                    qCDebug(uart_bridge) << "状态: " << mCH1_status_v;
                }
                if (mCH1_status[13] == "1"){   // cc
                    mCH1_status_v = 2;
                    qCDebug(uart_bridge) << "状态: " << mCH1_status_v;
                }
                if (mCH1_status[11] == "1"){   // ov
                    mCH1_status_v = 3;
                    qCDebug(uart_bridge) << "状态: " << mCH1_status_v;
                }

                emit CH1_StatusChanged();
            }
            return;
        case 2:
            if (mCH2_status != binaryStr) {
                mCH2_status = binaryStr;
                qCDebug(uart_bridge) << "事件状态改变为: " << binaryStr;

                mCH2_status_v = 0;
                if (mCH2_status[14] == "1"){   // cv
                    mCH2_status_v = 1;
                    qCDebug(uart_bridge) << "状态: " << mCH2_status_v;
                }
                if (mCH2_status[13] == "1"){   // cc
                    mCH2_status_v = 2;
                    qCDebug(uart_bridge) << "状态: " << mCH2_status_v;
                }
                if (mCH2_status[11] == "1"){   // ov
                    mCH2_status_v = 3;
                    qCDebug(uart_bridge) << "状态: " << mCH2_status_v;
                }

                emit CH1_StatusChanged();
            }
            return;
    }
}
