#include "uart_channel.h"
#include <QtCore>

// ========================== 初始化部分 ===================================

Q_LOGGING_CATEGORY(uart_channel, "UART_CHANNEL:")

UartChannelManager::UartChannelManager(QObject *parent):
    QObject(parent){
    m_initCommands = {
        {0x01, 0x00, "", false},// Turnoff output
        {0x04, 0x0e, QByteArray::fromHex("00"), false},//set A Unit
        {0x04, 0x0c, QByteArray::fromHex("3f 80 00 00"), false},//set NPLC =1
        {0x04, 0x1e, QByteArray::fromHex("37 82 dc bf"), false},//set Tint =1.56e-05
        {0x04, 0x0f, QByteArray::fromHex("01"), false},//set measure average =1
        {0x02, 0x00, QByteArray::fromHex("00 00 00 00"), false},//set cv =0
        {0x02, 0x01, QByteArray::fromHex("3f 80 00 00"), false},//set cc =1
        {0x04, 0x1f, QByteArray::fromHex("ff ff ff ff"), false},//set relarge
        {0x02, 0x03, QByteArray::fromHex("41 00 00 00"), false},//set ovp =8
        {0x04, 0x1d, QByteArray::fromHex("00 01"), false},//set Output impedance step =1
        {0x02, 0x02, QByteArray::fromHex("00 00 00 00"), false},//set Output impedance =0 -> cv model
        {0x05, 0x84, "", false},//query software
        {0x05, 0x85, "", false},//query Hardware
    };
}
UartChannelManager::~UartChannelManager()
{
    qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Serial~Destruct Finished.";
    m_isTesting = false;
    m_refreshtimer->stop();

    if (m_serialPort) {
        if (m_serialPort->isOpen()) {m_serialPort->close();}
        delete m_serialPort;
        delete m_refreshtimer;
        m_serialPort = nullptr;
        m_refreshtimer = nullptr;
    }

    if (m_serialThread) {
        m_serialThread->quit();
        m_serialThread->wait(1000);// 等待1秒
        m_serialThread->deleteLater();
        delete m_serialThread;
        m_serialThread = nullptr;
    }
}

bool UartChannelManager::initSerialPort(const QString &portName,
                                 qint32 baudRate,
                                 QSerialPort::DataBits dataBits,
                                 QSerialPort::Parity parity,
                                 QSerialPort::StopBits stopBits)
{
    if (!m_serialThread){
        for (const auto& config : configs) {
            if (config.port == portName) {
                m_channel = config.channel;
                break;
            }
        }

        if (m_channel == 0) {
            qCWarning(uart_channel) << "Undefined Channel Serial Port: " << portName;
            return false;
        }

        m_serialPort = new QSerialPort(this);
        m_serialPort->setFlowControl(QSerialPort::NoFlowControl); // In the majority situation
        //m_serialPort->setReadBufferSize(1024 * 1024); // 1MB buffer

        m_serialPort->setPortName(portName);
        m_serialPort->setBaudRate(baudRate);
        m_serialPort->setDataBits(dataBits);
        m_serialPort->setParity(parity);
        m_serialPort->setStopBits(stopBits);

        m_refreshtimer = new QTimer;
        m_refreshtimer->setInterval(180); // ms

        m_serialThread = new QThread(this);
        m_serialThread->setObjectName(QString("%1_worker").arg(portName));
    }

    if (thread() != m_serialThread) {
        this->moveToThread(m_serialThread);
        m_serialPort->moveToThread(m_serialThread);
        m_refreshtimer->moveToThread(m_serialThread);
    }

    if (!m_serialThread->isRunning()) {
        m_serialThread->start();

        connect(m_serialPort, &QSerialPort::readyRead, this, &UartChannelManager::handleReadyRead, Qt::DirectConnection);
        connect(m_serialPort, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
            if (error == QSerialPort::NoError) {return;}
            qCWarning(uart_channel) <<"Channel_"<<m_channel<<" Occur Error: "<<m_serialPort->errorString();
        }, Qt::DirectConnection);
        connect(m_refreshtimer,&QTimer::timeout,this,[this]{
            static int step = 0;
            switch (step) {
                case 0:  writeFrame(0x04,0x80,"",false); break;
                case 1:  writeFrame(0x04,0x81,"",false); break;
                case 2:  writeFrame(0x05,0x80,"",false); break;
            }
            step = (step + 1) % 3;
        }, Qt::DirectConnection);

        QMetaObject::invokeMethod(this, [this]() {
            if (m_serialPort->open(QIODevice::ReadWrite)) {
                sendInitCommand();
            }
        }, Qt::QueuedConnection);

        return true;
    }

    return false;
}

void UartChannelManager::sendInitCommand()
{
    if (m_currentInitIndex >= m_initCommands.size()) {
        qCDebug(uart_channel)<<"Channel_"<< m_channel<< "All init commands sent, starting refresh timer";
        if(ConfigManager::s_enableDisplay || ConfigManager::s_enableWEBServer){
            //m_refreshtimer->start();
        }
        return;
    }

    const Command& cmd = m_initCommands[m_currentInitIndex];
    writeFrame(cmd.cmd, cmd.func, cmd.param, cmd.isScpi);
    m_currentInitIndex++;

    // 60ms
    QTimer::singleShot(60, this, &UartChannelManager::sendInitCommand);
}

// ========================== 信息处理部分 ===================================

void UartChannelManager::writeFrame(quint8 cmd, quint8 func, const QByteArray& param,bool isScpi) {
    quint8 length = 4 + param.size();  //  Command+Function+Channel+CheckSum  + Parameter
    quint8 checksum = length + cmd + func + m_channel; // The check code is taken from the lowest 8 bits.
    for (char byte : param) {checksum += static_cast<quint8>(byte);}

    m_writebuffer.clear();
    m_writebuffer.reserve(length + 4);  // Pre-allocation enhances performance

    m_writebuffer.append(HEADER_HIGH);
    m_writebuffer.append(HEADER_LOW);
    m_writebuffer.append(length);
    m_writebuffer.append(cmd);
    m_writebuffer.append(func);
    m_writebuffer.append(m_channel);
    m_writebuffer.append(param);
    m_writebuffer.append(checksum);
    m_writebuffer.append(END_MARKER);

    writeSerialData(m_writebuffer,false);
    m_isSCPIrequest = isScpi;
}

void UartChannelManager::writeSerialData(const QByteArray& data,bool isforce)
{
    qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Send: " << data.toHex(' ');

    if (m_serialPort->write(data) != data.size()) {
        qCCritical(uart_channel)<<"Channel_"<<m_channel<<" QSerialPort Written Buffer Overflow!!!";
    }

    if(isforce){
        m_serialPort->flush();   // The same event is sent multiple times, and multiple messages will be sent together.
    }
}

void UartChannelManager::handleReadyRead()
{
    m_readbuffer.append(m_serialPort->readAll());
    if (m_readbuffer.isEmpty()){return;}

    // emit serialDataReceived(m_readbuffer,false); // Transit
    qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Received: "<< m_readbuffer.toHex(' '); // <<(m_channel==1?"电芯发":"控制发")

    // Test progressing
    if (m_isTesting) {
        if (m_readbuffer.size() >= 1024) {  // 1 KB
            qint64 elapsed = m_testTimer.elapsed(); // ms
            double speedKBps =  (1024 * 1000.0) / (elapsed * 1024);
            double speedBps = 1024 * 1000.0 / elapsed;

            qCDebug(uart_channel) << "\n" << QString(
                "Loopback Test Result:"
                "Time elapsed: %1 ms"
                "Speed: %2 KB/s (%3 bps)"
            ).arg(elapsed).arg(speedKBps, 0, 'f', 2).arg(speedBps * 8, 0, 'f', 0);

            m_isTesting=false;
            m_readbuffer.clear();
        }
        return;
    }

    // Normal response processing of the protocol
    if (m_readbuffer.size() >= 3){
        if(static_cast<quint8>(m_readbuffer[0]) == HEADER_LOW && static_cast<quint8>(m_readbuffer[1]) == HEADER_HIGH){
            quint8 lengthB = static_cast<quint8>(m_readbuffer[2]);
            if (m_readbuffer.size() < lengthB + 4){return;}

            if(static_cast<quint8>(m_readbuffer[lengthB + 3]) == END_MARKER){
                handleuartrequest(lengthB);
                m_readbuffer.remove(0,lengthB + 4);
                if(!m_readbuffer.isEmpty()){handleReadyRead();}
                return;
            }
        }

        qCWarning(uart_channel)<<"Channel_"<<m_channel<<" Received Data Format Error!!!";
        m_readbuffer.clear();
        return;
    }
}

void UartChannelManager::handleuartrequest(quint8 length){
    quint8 cmd = static_cast<quint8>(m_readbuffer[3]);
    quint8 func = static_cast<quint8>(m_readbuffer[4]);
    quint8 ch = static_cast<quint8>(m_readbuffer[5]);
    quint8 Checksum = length + cmd + func + ch; // The check code is taken from the lowest 8 bits.

    m_readparam.clear();
    m_readparam = m_readbuffer.mid(6, length - 4);   // length - Command+Function+Channel+CheckSum
    for (char byte : qAsConst(m_readparam)) {Checksum += static_cast<quint8>(byte);}

    if(static_cast<quint8>(m_readbuffer[length + 2]) != Checksum){
        qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Received Data Incorrect!!!";
        return;
    }

    switch (cmd) {
        case 0x01:handleOutputcmd          (func);return;
        case 0x02:handleSettingcmd         (func);return;
        case 0x03:handleControlcmd         (func);return;
        case 0x04:handleMeasurementcmd     (func);return;
        case 0x05:handleRegistercmd        (func);return;
        case 0x06:handleCalibratecmd       (func);return;
        case 0x07:handleCalibrationcmd     (func);return;
        case 0x08:handleTriggercmd         (func);return;
        case 0x09:handleISPcmd             (func);return;
        case 0x10:handleSNcmd              (func);return;
        case 0x11:handleIDcmd              (func);return;
        case 0xFF:handleErrorcmd           (func);return;

        default:
            qCWarning(uart_channel)<<"Channel_"<<m_channel<<" Occuring Unknown Command!!!";
            return;
    }
}

// ========================== 协议处理部分 ===================================

void UartChannelManager::handleOutputcmd(quint8 func){
    quint8 raw = -1;
    bool status = false;
    if (!m_readparam.isEmpty()){
        raw = static_cast<quint8>(m_readparam[0]);
        status = raw==0 ? false:true;
    }

    switch (func){
        case 0x80:{
            m_scpiManager->processCHStateResponse(status);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Output Query:"<<status;
            return;
        }
        case 0x00:
            m_qmlbridge->update_IsOutput(m_channel,false);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Output Been OFF";
            return;
        case 0x01:
            m_qmlbridge->update_IsOutput(m_channel,true);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Output Been ON";
            return;
        case 0x08:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Output Bandwidth Been:"<<(raw==0 ? "LOW":"HIGH");return;
        case 0x88:{
            m_scpiManager->processCHStateResponse(status);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Output Query:"<<(status ? "HIGH":"LOW");
            return;
        }
        case 0x09:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Output COMPMODE(1=Llocal->4=Hremote) Been:"<<raw;return;
        case 0x89:{
            m_scpiManager->processCHIntResponse(raw);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Output COMPMODE(1=Llocal->4=Hremote) Been:"<<raw;
            return;
        }

        default:qCCritical(uart_channel)<<"unknown Output func!!!";return;
    }
}

void UartChannelManager::handleSettingcmd(quint8 func){
    float shf{0.0f};
    if (m_readparam.size()==4){
        quint32 raw = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(m_readparam.constData()));
        memcpy(&shf, &raw, sizeof(float));
    }

    switch (func){
        case 0x80:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Setting Volt:"<<shf;
            return;
        case 0x00:
            m_qmlbridge->update_Cv(m_channel,shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Setting Volt:"<<shf;
            return;
        case 0x81:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Setting Curr:"<<shf;
            return;
        case 0x01:
            m_qmlbridge->update_Cc(m_channel,shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Setting Curr:"<<shf;
            return;
        case 0x82:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Setting Tmpe:"<<shf;
            return;
        case 0x02:
            m_qmlbridge->update_Imp(m_channel,shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Setting Tmpe:"<<shf;
            return;
        case 0x83:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Setting Prot:"<<shf;
            return;
        case 0x03:
            m_qmlbridge->update_Ovp(m_channel,shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Setting OVP Prot:"<<shf;
            return;
        case 0x84:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Setting Ther:"<<shf;
            return;
        case 0x04:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Setting Ther:"<<shf;return;
        case 0x85:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Setting Load:"<<shf;
            return;
        case 0x05:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Setting Load:"<<shf;return;
        case 0x86:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Setting RepeatVolt:"<<shf;
            return;
        case 0x06:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Setting RepeatVolt:"<<shf;return;
        case 0x87:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Setting RepeatCurr:"<<shf;
            return;
        case 0x07:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Setting RepeatCurr:"<<shf;return;
        case 0x88:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Setting RepeatTmpe:"<<shf;
            return;
        case 0x08:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Setting RepeatTmpe:"<<shf;return;

        default:qCCritical(uart_channel)<<"unknown Setting func!!!";return;
    }
}

void UartChannelManager::handleControlcmd(quint8 func){
    quint8 raw = -1;
    if (!m_readparam.isEmpty()){
        raw = static_cast<quint8>(m_readparam[0]);
    }

    switch (func){
        case 0x80:
            m_scpiManager->processCHIntResponse(raw);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Control load:"<<raw;
            return;
        case 0x00:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Control load:"<<(raw==0 ? "Disable":"Enable");return;
        case 0x81:
            m_scpiManager->processCHIntResponse(raw);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Control OVP:"<<raw;
            return;
        case 0x01:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Control OVP:"<<(raw==0 ? "Off":"On");return;
        case 0x82:
            m_scpiManager->processCHIntResponse(raw);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Control OCP:"<<raw;
            return;
        case 0x02:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Control OCP:"<<(raw==0 ? "Off":"On");return;
        case 0x83:
            m_scpiManager->processCHIntResponse(raw);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Control OTP:"<<raw;
            return;
        case 0x03:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Control OTP:"<<(raw==0 ? "Off":"On");return;
        case 0x84:
            m_scpiManager->processCHIntResponse(raw);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Control impedance:"<<raw;
            return;
        case 0x04:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Control impedance:"<<(raw==0 ? "Disable":"Enable");return;
        case 0x85:
            m_scpiManager->processCHIntResponse(raw);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Control POS:"<<raw;
            return;
        case 0x05:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Control POS:"<<raw;return;
        case 0x06:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Control SAV:"<<raw;return;
        case 0x07:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Control RCL:"<<raw;return;
        case 0x88:
            m_scpiManager->processCHIntResponse(raw);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Control stat:"<<raw;
            return;
        case 0x08:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Control stat:"<<raw;return;
        case 0x89:
            m_scpiManager->processCHIntResponse(raw);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Control loadOCP:"<<raw;
            return;
        case 0x09:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Control loadOCP:"<<raw;return;

        default:qCCritical(uart_channel)<<"unknown Control func!!!";return;
    }
}

void UartChannelManager::handleMeasurementcmd(quint8 func){
    float shf{0.0f};
    quint16 sht{0};
    quint8 shts{0};

    if (m_readparam.size() == 4){
        quint32 raw = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(m_readparam.constData()));
        memcpy(&shf, &raw, sizeof(float));
    }else if (m_readparam.size() == 2){
        quint16 raw = qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(m_readparam.constData()));
        memcpy(&sht, &raw, 2);
    }else if (m_readparam.size() == 1){
        shts = static_cast<quint8>(m_readparam[0]);
    }

    switch (func){
        case 0x80:
            m_qmlbridge->update_Voltage(m_channel,shf);
            if (m_isSCPIrequest) {m_scpiManager->processCHFloatResponse(shf);}
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement Volt:"<<shf;
            return;
        case 0x81:
            m_qmlbridge->update_CurrentAndUnit(m_channel,shf);
            if (m_isSCPIrequest) {m_scpiManager->processCHFloatResponse(shf);}
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement Curr:"<<shf;
            return;
        case 0x82:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement Scur:"<<shf;
            return;
        case 0x83:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement Btmp:"<<shf;
            return;
        case 0x84:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement Htmp:"<<shf;
            return;
        case 0x85:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement Acdc:"<<shf;
            return;
        case 0x86:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement Dvm:"<<shf;
            return;
        case 0x87:
            m_scpiManager->processCHIntResponse(sht);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement Fan:"<<sht;
            return;
        case 0x9D:
            m_scpiManager->processCHIntResponse(sht);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement Duty:"<<sht;
            return;
        case 0x89:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement Dvmac:"<<shf;
            return;
        case 0x8A:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement Temp1:"<<shf;
            return;
        case 0x8B:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement Temp2:"<<shf;
            return;
        case 0x8D:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement Temp3:"<<shf;
            return;
        case 0xb0:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement AdofVolt:"<<shf;
            return;
        case 0xb1:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement AdofCurr:"<<shf;
            return;
        case 0xb2:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement AdofScur:"<<shf;
            return;
        case 0xb6:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement AdofDvm3:"<<shf;
            return;
        case 0x8C:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement NPLC";return;
        case 0x0C:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Measurement NPLC:"<<shf;return;
        case 0x9F:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement Time:"<<shf;
            return;
        case 0x1F:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Measurement Time:"<<shf;return;
        case 0x8E:
            m_scpiManager->processCHIntResponse(shts);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement Rang:"<<shts;
            return;
        case 0x0E:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Measurement Rang:"<<shts;return;
        case 0x8F:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement average";return;
        case 0x0F:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Measurement average:"<<shts;return;
        case 0x90:
            m_scpiManager->processCHIntResponse(shts);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement Func:"<<shts;
            return;
        case 0x10:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Measurement Func:"<<shts;return;
        case 0x91:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement CurrHigh:"<<shf;
            return;
        case 0x92:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement CurrLow:"<<shf;
            return;
        case 0x93:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement CurrMax:"<<shf;
            return;
        case 0x94:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement CurrMin:"<<shf;
            return;
        case 0x95:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement DvmHigh:"<<shf;
            return;
        case 0x96:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement DvmLow:"<<shf;
            return;
        case 0x97:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement DvmMax:"<<shf;
            return;
        case 0x98:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement DvmMin:"<<shf;
            return;
        case 0x99:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement VoltHigh:"<<shf;
            return;
        case 0x9A:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement VoltHigh:"<<shf;
            return;
        case 0x9B:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement VoltHigh:"<<shf;
            return;
        case 0x9C:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement VoltHigh:"<<shf;
            return;
        case 0xa3:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement Offs:"<<shf;
            return;
        case 0x23:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Measurement Offs:"<<shf;return;
        //case 0x9d:break;
        case 0x1d:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Measurement Poin:"<<sht;return;
        case 0x9e:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement Tint:"<<shf;
            return;
        case 0x1e:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Measurement Tint:"<<shf;return;
        case 0xa0:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement ArrCurr:"<<shf;
            return;
        case 0xa1:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement ArrVolt:"<<shf;
            return;
        case 0xa2:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Measurement ArrDvm:"<<shf;
            return;
    }
}

void UartChannelManager::handleRegistercmd(quint8 func){
    QString verString;
    quint16 sht{0};
    quint8 shts{0};

    if (m_readparam.size() == 4){
        verString = QString("%1.%2.%3.%4")
                      .arg(static_cast<quint8>(m_readparam[0]))
                      .arg(static_cast<quint8>(m_readparam[1]))
                      .arg(static_cast<quint8>(m_readparam[2]))
                      .arg(static_cast<quint8>(m_readparam[3]));
    }else if (m_readparam.size() == 2){
        quint16 raw = qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(m_readparam.constData()));
        memcpy(&sht, &raw, 2);
    }else if (m_readparam.size() == 1){
        shts = static_cast<quint8>(m_readparam[0]);
    }

    switch (func){
        case 0x80:
            m_qmlbridge->update_Status(m_channel,sht);
            if (m_isSCPIrequest){m_scpiManager->processCHIntResponse(sht);}
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Register Status:"<<sht;
            return;
        case 0x81:
            m_scpiManager->processCHIntResponse(sht);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Register Enable:"<<sht;
            return;
        case 0x82:
            m_scpiManager->processCHIntResponse(sht);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Register Condition:"<<sht;
            return;
        case 0x83:
            m_scpiManager->processCHIntResponse(shts);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Register Error:"<<shts;
            return;
        case 0x03:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Register QUECLE";return;
        case 0x84:
            m_qmlbridge->update_SoftVer(m_channel,verString);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Register Software:"<<verString;
            return;
        case 0x85:
            m_qmlbridge->update_HardVer(m_channel,verString);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Register Hardware:"<<verString;
            return;
    }
}

void UartChannelManager::handleCalibratecmd(quint8 func){
    quint8 raw = -1;
    if (!m_readparam.isEmpty()){
        raw = static_cast<quint8>(m_readparam[0]);
    }

    switch (func){
        case 0x00:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Calibrate Exit";return;
        case 0x01:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Calibrate Init";return;
        case 0x02:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Calibrate Rest";return;
        case 0x03:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Calibrate Save";return;
        case 0x84:
            m_scpiManager->processCHIntResponse(raw);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Calibrate All:"<<raw;
            return;
        case 0x04:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Calibrate All:"<<raw;return;
        case 0x05:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Calibrate Adc";return;
        case 0x06:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Calibrate Dac";return;
        case 0x07:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Calibrate Enab";return;
        case 0x08:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Calibrate Imp";return;
        case 0x10:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Calibrate Dcp";return;
        case 0x11:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Calibrate Dcn";return;
    }
}

void UartChannelManager::handleCalibrationcmd(quint8 func){
    float shf{0.0f};
    if (m_readparam.size()==4){
        quint32 raw = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(m_readparam.constData()));
        memcpy(&shf, &raw, sizeof(float));
    }

    m_scpiManager->processCHFloatResponse(shf);
    qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Calibration step:"<<func<<" Cail:"<<shf;
}

void UartChannelManager::handleTriggercmd(quint8 func){
    float shf{0.0f};
    quint16 sht{0};
    quint8 shts{0};

    if (m_readparam.size() == 4){
        quint32 raw = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(m_readparam.constData()));
        memcpy(&shf, &raw, sizeof(float));
    }else if (m_readparam.size() == 2){
        quint16 raw = qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(m_readparam.constData()));
        memcpy(&sht, &raw, 2);
    }else if (m_readparam.size() == 1){
        shts = static_cast<quint8>(m_readparam[0]);
    }

    switch (func){
        case 0x00:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Trigger Abort";return;
        case 0x01:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Trigger Sequene:"<<shts;return;
        case 0x02:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Trigger Coquene:"<<shts;return;
        case 0x03:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Trigger Seq1";return;
        case 0x04:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Trigger Seq2";return;
        case 0x05:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Trigger Seq2So"<<shts;return;
        case 0x85:
            m_scpiManager->processCHIntResponse(shts);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Trigger Seq2So:"<<shts;
            return;
        case 0x06:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Trigger Seq2Co"<<sht;return;
        case 0x86:
            m_scpiManager->processCHIntResponse(sht);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Trigger Seq2Co:"<<sht;
            return;
        case 0x07:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Trigger Seq2Hy"<<shf;return;
        case 0x87:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Trigger Seq2Hy:"<<shf;
            return;
        case 0x08:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Trigger Seq2Le"<<shf;return;
        case 0x88:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Trigger Seq2Le:"<<shf;
            return;
        case 0x09:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Trigger Seq2Sl"<<shts;return;
        case 0x89:
            m_scpiManager->processCHIntResponse(shts);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Trigger Seq2Sl:"<<shts;
            return;
        case 0x8A:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Trigger Ampl:"<<shf;
            return;
        case 0x0A:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Trigger Ampl:"<<shf;return;
        case 0x8B:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Trigger Curr:"<<shf;
            return;
        case 0x0B:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Trigger Curr:"<<shf;return;
        case 0x8C:
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Query Trigger Res:"<<shf;
            return;
        case 0x0C:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Trigger Res:"<<shf;return;
    }
}

void UartChannelManager::handleISPcmd(quint8 func){
    switch (func){
        case 0x80:break;
        case 0x00:break;
        case 0x01:break;
        case 0x02:break;
    }
}

void UartChannelManager::handleSNcmd(quint8 func){
    switch (func){
        case 0x80:break;
        case 0x00:break;
        case 0x81:break;
        case 0x01:break;
        case 0x82:break;
        case 0x02:break;
        case 0x83:break;
        case 0x03:break;
        case 0x84:break;
        case 0x04:break;
    }
}

void UartChannelManager::handleIDcmd(quint8 func){
    switch (func){
        case 0x81:break;
        case 0x82:break;
        case 0x83:break;
        case 0x84:break;
    }
}

void UartChannelManager::handleErrorcmd(quint8 func){
    switch (func){
        case 0x00:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Error Response: CheckSum Error";      break;
        case 0x01:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Error Response: Unknow Command";      break;
        case 0x02:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Error Response: Unknow Function";     break;
        case 0x03:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Error Response: Error Length";        break;
        case 0x04:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Error Response: Invalid Parameter";   break;
        case 0x05:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Error Response: Illegal Command";     break;
        case 0x06:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Error Response: Unsupported Command"; break;
    }
}

// ========================== 模块性能自测 ===================================

void UartChannelManager::startLoopbackTest()
{
    if (m_isTesting) {return;}
    m_isTesting=true;

    QByteArray testData;
    testData.resize(1024); // 1KB
    for (int i = 0; i < 1024; i++) {testData[i] = i % 256;}

    qCDebug(uart_channel)<<"Starting Loopback Test (Connect TX to RX for testing)...";
    qCDebug(uart_channel)<<"Sending 1024 bytes of Test Data";
    m_testTimer.start();
    writeSerialData(testData,false);
}
