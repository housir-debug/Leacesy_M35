// uartmanager.cpp
#include "uartmanager.h"

Q_LOGGING_CATEGORY(ubridge, "ubridge:")

SerialBridge::SerialBridge(QObject *parent) : QObject(parent) {}

SerialBridge::~SerialBridge() {}

// ********************* qml调用处理 **************************

void SerialBridge::setChannel_Output(int channel,bool switchs){
    qDebug(ubridge) << "onChannel_2_Toggled当前状态为："<<switchs;
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

// --------------------------  辅助函数  ----------------------------

void SerialBridge::toAll_Channel(quint8 cmd,quint8 func,QByteArray param){
    emit sendFrame_Uart4(cmd,func,param);
    emit sendFrame_Uart5(cmd,func,param);
}

/*void SerialBridge::setupQmlConnections(QQmlApplicationEngine &engine)
{
    const auto rootObjects = engine.rootObjects();
    if (rootObjects.isEmpty()) {
        qWarning() << "No root objects loaded yet!";
        return;
    }

    QObject *rootObject = rootObjects.first();

    QObject *channel1 = rootObject->findChild<QObject*>("channel_1");
    if (channel1) {
        QObject::connect(channel1, SIGNAL(channelToggled(bool)),
                        this, SLOT(onChannel_1_Toggled(bool)));
    }

    QObject *channel2 = rootObject->findChild<QObject*>("channel_2");
    if (channel2) {
        QObject::connect(channel2, SIGNAL(channelToggled(bool)),
                        this, SLOT(onChannel_2_Toggled(bool)));
    }
}*/

// ********************* C++槽函数具体实现 ****************************

void SerialBridge::update_Uart4_Voltage(float voltage){
    mUart4_Voltage = voltage;
    emit Uart4_VoltageChanged();
}

void SerialBridge::update_Uart4_Current(float current){
    mUart4_Current = current;

    if (qAbs(mUart4_Current) < 1e-4){
        mUart4_Current *= 1000.0f;
        if (mUart4_Current_Unit == false){
            mUart4_Current_Unit = true;  // mA
            emit Uart4_Current_Unit_Changed();
        }
    }else{
        if (mUart4_Current_Unit == true){
            mUart4_Current_Unit = false;  // A
            emit Uart4_Current_Unit_Changed();
        }
    }

    emit Uart4_CurrentChanged();
}

void SerialBridge::update_Uart4_status(QByteArray status){
    if (status.size() < 2) {return;}

    quint16 value = (static_cast<quint8>(status[0]) << 8) | static_cast<quint8>(status[1]);
    QString binaryStr = QString("%1").arg(value, 16, 2, QLatin1Char('0'));

    if (mUart4_status != binaryStr) {
        mUart4_status = binaryStr;
        qCDebug(ubridge) << "事件状态改变为: " << binaryStr;

        mUart4_status_v = 0;
        if (mUart4_status[14] == "1"){   // cv
            mUart4_status_v = 1;
            qCDebug(ubridge) << "状态: " << mUart4_status_v;
        }
        if (mUart4_status[13] == "1"){   // cc
            mUart4_status_v = 2;
            qCDebug(ubridge) << "状态: " << mUart4_status_v;
        }
        if (mUart4_status[11] == "1"){   // ov
            mUart4_status_v = 3;
            qCDebug(ubridge) << "状态: " << mUart4_status_v;
        }

        emit Uart4_StatusChanged();
    }
}

void SerialBridge::update_Uart5_Voltage(float voltage){
    mUart5_Voltage = voltage;
    emit Uart5_VoltageChanged();
}

void SerialBridge::update_Uart5_Current(float current){
    mUart5_Current = current;

    if (qAbs(mUart5_Current) < 1e-4){
        mUart5_Current *= 1000.0f;
        if (mUart5_Current_Unit == false){
            mUart5_Current_Unit = true;  // mA
            emit Uart5_Current_Unit_Changed();
        }
    }else{
        if (mUart5_Current_Unit == true){
            mUart5_Current_Unit = false;  // A
            emit Uart5_Current_Unit_Changed();
        }
    }

    emit Uart5_CurrentChanged();
}

void SerialBridge::update_Uart5_status(QByteArray status){
    if (status.size() < 2) {return;}

    quint16 value = (static_cast<quint8>(status[0]) << 8) | static_cast<quint8>(status[1]);
    QString binaryStr = QString("%1").arg(value, 16, 2, QLatin1Char('0'));

    if (mUart5_status != binaryStr) {
        mUart5_status = binaryStr;
        qDebug(ubridge) << "事件状态: " << binaryStr;
        if (mUart5_status[14] == "1"){   // cv
            mUart5_status_v = 1;
            qCDebug(ubridge) << "状态: " << mUart5_status_v;
        }
        if (mUart5_status[13] == "1"){   // cc
            mUart5_status_v = 2;
            qCDebug(ubridge) << "状态: " << mUart5_status_v;
        }
        if (mUart5_status[11] == "1"){   // ov
            mUart5_status_v = 3;
            qCDebug(ubridge) << "状态: " << mUart5_status_v;
        }
        emit Uart5_StatusChanged();
    }
}
