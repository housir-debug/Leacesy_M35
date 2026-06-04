#include "uart_channel.h"
#include <QtCore>

Q_LOGGING_CATEGORY(uart_channel, "UART_CHANNEL:")

UartChannelManager::UartChannelManager(QObject *parent):QObject(parent){}
UartChannelManager::~UartChannelManager()
{
    if (m_refreshtimer) {
        m_refreshtimer->stop();
        delete m_refreshtimer;
        m_refreshtimer = nullptr;
    }

    if (m_serialPort) {
        m_serialPort->close();
        delete m_serialPort;
        m_serialPort = nullptr;
    }

    if (m_serialThread) {
        m_serialThread->quit();
        m_serialThread->wait(1000);// 等待1秒
        m_serialThread->deleteLater();
        delete m_serialThread;
        m_serialThread = nullptr;
    }

    qCDebug(uart_channel)<<"[~UartChannelManager]:Channel_"<<m_channel<<" Destroyed!!!";
}

bool UartChannelManager::initSerialPort(const QString &portName,qint32 baudRate)
{
    if (!m_serialThread && !m_serialPort && !m_refreshtimer){
        for (const auto& config : configs) {
            if (config.port == portName) {
                m_channel = config.channel;
                m_serialPort = new QSerialPort(this);

                m_serialPort->setPortName(portName);
                m_serialPort->setBaudRate(baudRate); // QSerialPort::Baud115200

                // Set the data bit to 8 bits, For example: Data5 - Data8
                m_serialPort->setDataBits(QSerialPort::Data8);
                // Not use parity check bits, the upper-level protocol ensures data integrity.
                m_serialPort->setParity(QSerialPort::NoParity);
                // Use 1 stop bit, Mark the end of A data byte
                m_serialPort->setStopBits(QSerialPort::OneStop);
                // HardwareControl: Requires wiring support | SoftwareControl: Applicable only to written text
                m_serialPort ->setFlowControl(QSerialPort::NoFlowControl);

                m_refreshtimer = new QTimer;
                m_refreshtimer->setInterval(180); // ms -> 60ms < target < at will

                m_serialThread = new QThread(this);
                m_serialThread->setObjectName(QString("UartChannel_%1_worker").arg(m_channel));

                this->moveToThread(m_serialThread);
                m_serialPort->moveToThread(m_serialThread);
                m_refreshtimer->moveToThread(m_serialThread);
                m_serialThread->start();

                connect(m_serialPort, &QSerialPort::readyRead, this, &UartChannelManager::handleReadyRead, Qt::DirectConnection);
                connect(m_serialPort, &QSerialPort::errorOccurred, this, [this]() {
                    qCWarning(uart_channel)<<"[initSerialPort]:Channel_"<<m_channel<<" Error: "<<m_serialPort->errorString();
                }, Qt::DirectConnection);
                connect(m_refreshtimer,&QTimer::timeout,this,[this]{
                    static int step = 0;
                    step = (step + 1) % 3;
                    switch (step) {
                        case 0:  writeFrame(0x04,0x80,"",false); break;
                        case 1:  writeFrame(0x04,0x81,"",false); break;
                        case 2:  writeFrame(0x05,0x80,"",false); break;
                    }}, Qt::DirectConnection);

                QMetaObject::invokeMethod(this, [this]() {
                    if (m_serialPort->open(QIODevice::ReadWrite)) {
                        //startLoopbackTest();
                        sendInitCommand();
                        return;
                    }
                    qCWarning(uart_channel)<<"[initSerialPort]:m_serialPort open failed!";
                }, Qt::QueuedConnection);

                return true;
            }
        }
    }

    qCWarning(uart_channel)<<"[initSerialPort]:Undefined Channel Serial Port: "<<portName;
    return false;
}

const QVector<Command> UartChannelManager::m_initCommands = {
    {0x05, 0x84, ""},// query software
    {0x05, 0x85, ""},// query Hardware
    {0x01, 0x00, ""},// Turnoff output
    {0x04, 0x0e, QByteArray::fromHex("00")},// set A Unit
    {0x04, 0x0c, QByteArray::fromHex("3f 80 00 00")},// set NPLC =1
    {0x04, 0x1e, QByteArray::fromHex("37 82 dc bf")},// set Tint =1.56e-05
    {0x04, 0x0f, QByteArray::fromHex("01")},// set measure average =1
    {0x02, 0x00, QByteArray::fromHex("00 00 00 00")},// set cv =0
    {0x02, 0x01, QByteArray::fromHex("3f 80 00 00")},// set cc =1
    //{0x04, 0x1f, QByteArray::fromHex("ff ff ff ff")},// set relarge, not vaild parameter
    {0x02, 0x03, QByteArray::fromHex("41 00 00 00")},// set ovp =8
    {0x04, 0x1d, QByteArray::fromHex("00 01")},// set Output impedance step =1
    {0x02, 0x02, QByteArray::fromHex("00 00 00 00")},// set Output impedance =0 -> cv model
};

void UartChannelManager::sendInitCommand()
{
    if (m_InitIndex < m_initCommands.size()) {
        const Command& cmd = m_initCommands[m_InitIndex];
        writeFrame(cmd.cmd, cmd.func, cmd.param, false);

        QTimer::singleShot(60, this, &UartChannelManager::sendInitCommand);// 60ms
        m_InitIndex++;
        return;
    }

    if(ConfigManager::s_enableDisplay || ConfigManager::s_enableWEBServer){
        //m_refreshtimer->start();
    }
}

void UartChannelManager::startLoopbackTest()
{
    qCDebug(uart_channel)<<"[startLoopbackTest]:Starting Loopback Test.";
    QByteArray testData(1024, 0);

    m_testTimer.start();
    m_serialPort->write(testData);
}

void UartChannelManager::writeFrame(quint8 cmd, quint8 func, const QByteArray& param,bool isScpi) {
    m_waitingForRes.store(true);
    quint8 length = 4 + param.size();  //  Command+Function+Channel+CheckSum  + Parameter
    quint8 checksum = length + cmd + func + m_channel; // The check code is taken from the lowest 8 bits.
    for (char byte : param) {checksum += static_cast<quint8>(byte);}

    m_responsebuffer.clear();
    m_responsebuffer.reserve(length + 4);  // Pre-allocation enhances performance
    m_responsebuffer.append(HEADER_HIGH);
    m_responsebuffer.append(HEADER_LOW);
    m_responsebuffer.append(length);
    m_responsebuffer.append(cmd);
    m_responsebuffer.append(func);
    m_responsebuffer.append(m_channel);
    m_responsebuffer.append(param);
    m_responsebuffer.append(checksum);
    m_responsebuffer.append(END_MARKER);

    //m_serialPort->flush();//immediately
    m_serialPort->write(m_responsebuffer);
    if (isScpi){m_scpiCommand = (static_cast<quint16>(cmd) << 8) | func;}
    qCDebug(uart_channel)<<"[writeFrame]:Channel_"<<m_channel<<" Send: "<<m_responsebuffer.toHex(' ');

    for (int i = 0; i < 18; i++) {
        if (!m_waitingForRes.load()) {
            return;
        }

        QThread::msleep(1); // 1ms total 18ms
    }
}

void UartChannelManager::handleReadyRead()
{
    m_readbuffer.append(m_serialPort->readAll());

    // Normal response processing of the protocol
    if (m_readbuffer.size() >= 3){
        if(static_cast<quint8>(m_readbuffer[0]) == HEADER_LOW && static_cast<quint8>(m_readbuffer[1]) == HEADER_HIGH){
            qCDebug(uart_channel)<<"[handleReadyRead]:Channel_"<<m_channel<<" Received: "<<m_readbuffer.toHex(' ');
            quint8 lengthB = static_cast<quint8>(m_readbuffer[2]);

            if (m_readbuffer.size() >= lengthB + 4){
                if(static_cast<quint8>(m_readbuffer[lengthB + 3]) == END_MARKER){
                    quint8 cmd = static_cast<quint8>(m_readbuffer[3]);
                    quint8 func = static_cast<quint8>(m_readbuffer[4]);
                    quint8 ch = static_cast<quint8>(m_readbuffer[5]);

                    quint8 Checksum = lengthB + cmd + func + ch; // The check code is taken from the lowest 8 bits.
                    m_readparam = m_readbuffer.mid(6, lengthB - 4);   // length - Command+Function+Channel+CheckSum
                    for (char byte : qAsConst(m_readparam)) {Checksum += static_cast<quint8>(byte);}

                    if(static_cast<quint8>(m_readbuffer[lengthB + 2]) == Checksum){
                        m_waitingForRes.store(false);
                        switch (cmd) {
                            case 0x01:handleOutputcmd          (func);break;
                            case 0x02:handleSettingcmd         (func);break;
                            case 0x03:handleControlcmd         (func);break;
                            case 0x04:handleMeasurementcmd     (func);break;
                            case 0x05:handleRegistercmd        (func);break;
                            case 0x06:handleCalibratecmd       (func);break;
                            case 0x07:handleCalibrationcmd     (func);break;
                            case 0x08:handleTriggercmd         (func);break;
                            case 0x09:handleISPcmd             (func);break;
                            case 0x10:handleSNcmd              (func);break;
                            case 0x11:handleIDcmd              (func);break;
                            case 0xFF:handleErrorcmd           (func);break;
                            default:qCWarning(uart_channel)<<"[handleReadyRead]:Channel_"<<m_channel<<" Occuring Unknown Command!!!";
                        }

                        m_readbuffer.remove(0,lengthB + 4);
                        if(!m_readbuffer.isEmpty()){
                            qCDebug(uart_channel)<<"[handleReadyRead]:Channel_"<<m_channel<<" continue next process";
                            handleReadyRead();
                        }

                        return; // finish.
                    }
                }

                qCWarning(uart_channel)<<"[handleReadyRead]:Channel_"<<m_channel<<" Received For Error!!";
                m_readbuffer.clear();
                return;
            }

            return; // Insufficient length; wait to be completed.
        }

        qCWarning(uart_channel)<<"[handleReadyRead]:Channel_"<<m_channel<<" Received Format Error!!!";
        m_readbuffer.clear();
    }

    // Test progressing
    /*if (m_readbuffer.size() >= 1024) { // 1KB
        qint64 elapsed = m_testTimer.elapsed(); // ms
        double speedKBps =  (1024 * 1000.0) / (elapsed * 1024);
        double speedBps = 1024 * 1000.0 / elapsed;

        qCDebug(uart_channel) << "\n" << QString(
            "Loopback Test Result:"
            "Time elapsed: %1 ms"
            "Speed: %2 KB/s (%3 bps)"
        ).arg(elapsed).arg(speedKBps, 0, 'f', 2).arg(speedBps * 8, 0, 'f', 0);

        m_readbuffer.clear();
    }*/
}

//---------------------------------------------------------------------------------

void UartChannelManager::handleOutputcmd(quint8 func){
    quint32 shts{0};quint8 sh{0};bool status{false};

    if (m_readparam.size()==1){
        sh = static_cast<quint8>(m_readparam[0]);
        status = sh!=0;
    } else if(m_readparam.size()==4){
        shts = qFromBigEndian<quint32>(m_readparam.constData());
    }

    switch (func){
        case 0x80: // :OUTP[:STAT]?
            m_scpiManager->processCHStateResponse(status);
            emit to_CanServer(m_channel,0x0180,m_readparam);
            qCDebug(uart_channel)<<"[handleOutputcmd]:Channel_"<<m_channel<<" Query[0x80] "<<status;
            return;
        case 0x00: // :OUTP[:STAT] {OFF|0}
            m_scpiManager->processCHVoidResponse();
            m_qmlbridge->update_IsOutput(m_channel,false);
            emit to_CanServer(m_channel,0x01,m_readparam);
            qCDebug(uart_channel)<<"[handleOutputcmd]:Channel_"<<m_channel<<" Set[0x00] OFF";
            return;
        case 0x01: // :OUTP[:STAT] {ON|1}
            m_scpiManager->processCHVoidResponse();
            m_qmlbridge->update_IsOutput(m_channel,true);
            m_readparam.append(1);
            emit to_CanServer(m_channel,0x01,m_readparam);
            qCDebug(uart_channel)<<"[handleOutputcmd]:Channel_"<<m_channel<<" Set[0x01] ON";
            return;
        case 0x02: // :OUTPut[ch]:MODE <n>
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleOutputcmd]:Channel_"<<m_channel<<" Set[0x02] "<<sh;
            return;
        case 0x82: // :OUTPut[ch]:MODE?
            m_scpiManager->processCHIntResponse(sh);
            qCDebug(uart_channel)<<"[handleOutputcmd]:Channel_"<<m_channel<<" Set[0x82] "<<sh;
            return;
        case 0x08: // :OUTPut[ch]:BAND:HIGH|LOW
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0108,m_readparam);
            qCDebug(uart_channel)<<"[handleOutputcmd]:Channel_"<<m_channel<<" Set[0x08] "<<(sh==0 ? "LOW":"HIGH");
            return;
        case 0x88: // :OUTPut[ch]:BAND?
            m_scpiManager->processCHStateResponse(status);
            emit to_CanServer(m_channel,0x0188,m_readparam);
            qCDebug(uart_channel)<<"[handleOutputcmd]:Channel_"<<m_channel<<" Query[0x88] "<<(status ? "HIGH":"LOW");
            return;
        case 0x09: // :OUTPut[ch]:COMP:MODE type
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0109,m_readparam);
            qCDebug(uart_channel)<<"[handleOutputcmd]:Channel_"<<m_channel<<" Set[0x09] "<<sh;
            return;
        case 0x89: // :OUTPut[ch]:COMP:MODE?
            m_scpiManager->processCHIntResponse(sh);
            emit to_CanServer(m_channel,0x0189,m_readparam);
            qCDebug(uart_channel)<<"[handleOutputcmd]:Channel_"<<m_channel<<" Query[0x89] "<<sh;
            return;
        case 0x90: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleOutputcmd]:Channel_"<<m_channel<<" Query[0x90]"<<shts;
            return;
        case 0x10: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleOutputcmd]:Channel_"<<m_channel<<" Set[0x10]"<<shts;
            return;
        case 0x91: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleOutputcmd]:Channel_"<<m_channel<<" Query[0x91]"<<shts;
            return;
        case 0x11: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleOutputcmd]:Channel_"<<m_channel<<" Set[0x11]"<<shts;
            return;

        default:qCCritical(uart_channel)<<"[handleOutputcmd]:unknown func!!!"; return;
    }
}

void UartChannelManager::handleSettingcmd(quint8 func){
    float shf{0.0f};

    if(m_readparam.size()==4){
        shf = qFromBigEndian<float>(m_readparam.constData());
    }

    switch (func){
        case 0x80: // :SOUR:VOLT[:LEV][:AMPL]?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x0280,m_readparam);
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Query[0x80] "<<shf;
            return;
        case 0x00: // :SOUR:VOLT[:LEV][:AMPL] <NRf>
            m_qmlbridge->update_Cv(m_channel,shf);
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0200,m_readparam);
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Set[0x00] "<<shf;
            return;
        case 0x81: // :SOUR:CURR[:LIM][:VAL]?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x0281,m_readparam);
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Query[0x81] "<<shf;
            return;
        case 0x01: // :SOUR:CURR[:LIM][:VAL] <NRf>
            m_qmlbridge->update_Cc(m_channel,shf);
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0201,m_readparam);
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Set[0x01] "<<shf;
            return;
        case 0x82: // :OUTP:IMP?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x0282,m_readparam);
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Query[0x82] "<<shf;
            return;
        case 0x02: // :OUTP:IMP <NRf>
            m_qmlbridge->update_Imp(m_channel,shf);
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0202,m_readparam);
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Set[0x02] "<<shf;
            return;
        case 0x83: // :SOUR:VOLT:PROT?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x0283,m_readparam);
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Query[0x83]"<<shf;
            return;
        case 0x03: // :SOUR:VOLT:PROT <NRf>
            m_qmlbridge->update_Ovp(m_channel,shf);
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0203,m_readparam);
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Set[0x03] "<<shf;
            return;
        case 0x84: // :THER[:PROT][:TEMP]?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x0284,m_readparam);
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Query[0x84] "<<shf;
            return;
        case 0x04: // :THER[:PROT][:TEMP] <NRf>
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0204,m_readparam);
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Set[0x04] "<<shf;
            return;
        case 0x85: // :LOAD:CURR[:LIM][:VAL]?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x0285,m_readparam);
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Query[0x85] "<<shf;
            return;
        case 0x05: // :LOAD:CURR[:LIM][:VAL] <NRf>
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0205,m_readparam);
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Set[0x05] "<<shf;
            return;
        case 0x86: // SCPI cmd Repeat
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Query[0x86] "<<shf;
            return;
        case 0x06: // SCPI cmd Repeat
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Set[0x06] "<<shf;
            return;
        case 0x87: // SCPI cmd Repeat
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Query[0x87] "<<shf;
            return;
        case 0x07: // SCPI cmd Repeat
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Set[0x07] "<<shf;
            return;
        case 0x88: // SCPI cmd Repeat
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Query[0x88] "<<shf;
            return;
        case 0x08: // SCPI cmd Repeat
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Set[0x08] "<<shf;
            return;
        case 0x89: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Query[0x89] "<<shf;
            return;
        case 0x09: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Set[0x09] "<<shf;
            return;

        default:qCCritical(uart_channel)<<"[handleSettingcmd]:unknown func!!!"; return;
    }
}

void UartChannelManager::handleControlcmd(quint8 func){
    float shf{0.0f};quint16 sht{0};quint8 sh{0};

    if (m_readparam.size()==1){
        sh = static_cast<quint8>(m_readparam[0]);
    } else if(m_readparam.size()==2){
        sht = qFromBigEndian<quint16>(m_readparam.constData());
    } else if(m_readparam.size()==4){
        shf = qFromBigEndian<float>(m_readparam.constData());
    }

    switch (func){
        case 0x80: // not SCPI cmd
            emit to_CanServer(m_channel,0x0380,m_readparam);
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Query[0x80] "<<sh;
            return;
        case 0x00: // not SCPI cmd
            emit to_CanServer(m_channel,0x0300,m_readparam);
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Set[0x00] "<<(sh==0 ? "Disable":"Enable");
            return;
        case 0x81: // :SOUR:VOLT:PROT:CLAM?
            m_scpiManager->processCHIntResponse(sh);
            emit to_CanServer(m_channel,0x0381,m_readparam);
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Query[0x81] "<<sh;
            return;
        case 0x01: // :SOUR:VOLT:PROT:CLAM <b>
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0301,m_readparam);
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Set[0x01] "<<(sh==0 ? "Off":"On");
            return;
        case 0x82: // :SOUR:CURR[:LIM]:TYPE?
            m_scpiManager->processCHIntResponse(sh);
            emit to_CanServer(m_channel,0x0382,m_readparam);
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Query[0x82] "<<sh;
            return;
        case 0x02: // :SOUR:CURR[:LIM]:TYPE <name>
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0302,m_readparam);
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Set[0x02] "<<(sh==0 ? "Off":"On");
            return;
        case 0x83: // not SCPI cmd
            emit to_CanServer(m_channel,0x0383,m_readparam);
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Query[0x83] "<<sh;
            return;
        case 0x03: // not SCPI cmd
            emit to_CanServer(m_channel,0x0303,m_readparam);
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Set[0x03] "<<(sh==0 ? "Off":"On");
            return;
        case 0x84: // not SCPI cmd
            emit to_CanServer(m_channel,0x0384,m_readparam);
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Query[0x84] "<<sh;
            return;
        case 0x04: // not SCPI cmd
            emit to_CanServer(m_channel,0x0304,m_readparam);
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Set[0x04] "<<(sh==0 ? "Disable":"Enable");
            return;
        case 0x85: // :SYST:POS?
            m_scpiManager->processCHIntResponse(sh);
            emit to_CanServer(m_channel,0x0385,m_readparam);
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Query[0x85] "<<sh;
            return;
        case 0x05: // :SYST:POS <name>
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0305,m_readparam);
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Set[0x05] "<<sh;
            return;
        case 0x06: // *SAV <NRf>
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Set[0x06] "<<sh;
            return;
        case 0x07: // *RCL <NRf>
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Set[0x07] "<<sh;
            return;
        case 0x88: // :LOAD:INDE:STAT?
            m_scpiManager->processCHIntResponse(sh);
            emit to_CanServer(m_channel,0x0388,m_readparam);
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Query[0x88] "<<sh;
            return;
        case 0x08: // :LOAD:INDE:STAT <b>
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0308,m_readparam);
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Set[0x08] "<<sh;
            return;
        case 0x89: // :LOAD:CURR[:LIM]:TYPE?
            m_scpiManager->processCHIntResponse(sh);
            emit to_CanServer(m_channel,0x0389,m_readparam);
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Query[0x89] "<<sh;
            return;
        case 0x09: // :LOAD:CURR[:LIM]:TYPE <name>
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0309,m_readparam);
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Set[0x09] "<<sh;
            return;
        case 0x8a: // :SOUR:CURR[:LIM]:PRIO?
            m_scpiManager->processCHIntResponse(sh);
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Query[0x8a] "<<sh;
            return;
        case 0x0a: // :SOUR:CURR[:LIM]:PRIO <b>
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Set[0x0a] "<<sh;
            return;
        case 0x8b: // :SOUR:VOLT:PROT:ABS?
            m_scpiManager->processCHIntResponse(sh);
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Query[0x8b] "<<sh;
            return;
        case 0x0b: // :SOUR:VOLT:PROT:ABS?
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Set[0x0b] "<<sh;
            return;
        case 0x8c: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Query[0x8c] "<<sh;
            return;
        case 0x0c: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Set[0x0c] "<<(sh==0 ? "CP":"OPP");
            return;
        case 0x90: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleControlcmd]:Channel_"<<m_channel<<" Query[0x90] "<<sht;
            return;
        case 0x91: // :SOUR:VOLT:PROT:TIME?
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Query[0x91] "<<shf;
            return;
        case 0x11: // :SOUR:VOLT:PROT:TIME <NRf>
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Set[0x11] "<<shf;
            return;
        case 0x92: // :SOUR:CURR[:LIM]:TIME?
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Query[0x92] "<<shf;
            return;
        case 0x12: // :SOUR:CURR[:LIM]:TIME <NRf>
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Set[0x12] "<<shf;
            return;
        case 0x93: // :THER[:PROT]:TIME?
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Query[0x93] "<<shf;
            return;
        case 0x13: // :THER[:PROT]:TIME <NRf>
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Set[0x13] "<<shf;
            return;
        case 0x94: // :THER[:PROT]:TIME?
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Query[0x94] "<<shf;
            return;
        case 0x14: // :THER[:PROT]:TIME <NRf>
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleSettingcmd]:Channel_"<<m_channel<<" Set[0x14] "<<shf;
            return;

        default:qCCritical(uart_channel)<<"[handleControlcmd]:unknown func!!!";return;
    }
}

void UartChannelManager::handleMeasurementcmd(quint8 func){ // new protocol Needs to be completed
    float shf{0.0f};quint16 sht{0};quint8 sh{0};

    if (m_readparam.size() == 4){
        shf = qFromBigEndian<float>(m_readparam.constData());
    }else if (m_readparam.size() == 2){
        sht = qFromBigEndian<quint16>(m_readparam.constData());
    }else if (m_readparam.size() == 1){
        sh = static_cast<quint8>(m_readparam[0]);
    }

    switch (func){
        case 0x80: // :MEAS:VOLT[:DC]?
            m_qmlbridge->update_Voltage(m_channel,shf);
            if (m_scpiCommand == 0x0480){
                m_scpiManager->processCHFloatResponse(shf);
                emit to_CanServer(m_channel,0x0480,m_readparam);
            }
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[0x80] "<<shf;
            return;
        case 0x81: // :MEAS:CURR[:DC]?
            m_qmlbridge->update_CurrentAndUnit(m_channel,shf);
            if (m_scpiCommand == 0x0481){
                m_scpiManager->processCHFloatResponse(shf);
                emit to_CanServer(m_channel,0x0481,m_readparam);
            }
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[0x81] "<<shf;
            return;
        case 0x82: // :MEAS:SCUR[:DC]?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x0482,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[0x82] "<<shf;
            return;
        case 0x83: // :MEAS:BTMP?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x0483,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[0x83] "<<shf;
            return;
        case 0x84: // :MEAS:HTMP?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x0484,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[0x84] "<<shf;
            return;
        case 0x85: // :MEASure:DVMeter:ACDC?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x0485,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[0x85] "<<shf;
            return;
        case 0x86: // :MEASure:DVMeter?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x0486,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[0x86] "<<shf;
            return;
        case 0x87: // :THER[:PROT]:FAN?
            m_scpiManager->processCHIntResponse(sht);
            emit to_CanServer(m_channel,0x0487,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[0x87] "<<sht;
            return;
        case 0x9D: // :THER[:PROT]:DUTY?
            m_scpiManager->processCHIntResponse(sht);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<sht;
            return;
        case 0x89: // :MEASure:DVMeter:AC?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x0489,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0x8A: // :MEAS:TMP1?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x048A,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0x8B: // :MEAS:TMP2?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x048B,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0x8D: // :MEAS:TMP3?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x048D,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0xb0: // :MEASure:ADcOFfset:VOLTage?
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0xb1: // :MEASure:ADcOFfset:CURRent?
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0xb2: // :MEASure:ADcOFfset:SmallCURrent?
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0xb6: // :MEASure:ADcOFfset:DVMeter?
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0x8C: // :SENS:NPLC?
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[]";
            return;
        case 0x0C: // :SENS:NPLC <NRf>
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Set[] "<<shf;
            return;
        case 0x9F: // :SENS:CURR:RANG:TIMe?
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0x1F: // :SENS:CURR:RANG:TIMe n
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Set[] "<<shf;
            return;
        case 0x8E: // :SENS:CURR[:DC]:RANG[:UPP]?
            m_scpiManager->processCHIntResponse(sh);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<sh;
            return;
        case 0x0E: // :SENS:CURR[:DC]:RANG[:UPP] <NRf>
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Set[] "<<sh;
            return;
        case 0x8F: // :SENS:AVER?
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x048F,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[]";
            return;
        case 0x0F: // :SENS:AVER <NRf>
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x040F,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Set[] "<<sh;
            return;
        case 0x90: // :SENS:FUNC?
            m_scpiManager->processCHIntResponse(sh);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<sh;
            return;
        case 0x10: // :SENS:FUNC "{CURR|DVM|VOLT}"
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0410,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Set[] "<<sh;
            return;
        case 0x91: // :FETC:CURR:HIGH?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x0491,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0x92: // :FETC:CURR:LOW?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x0492,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0x93: // :FETC:CURR:MAX?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x0493,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0x94: // :FETC:CURR:MIN?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x0494,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0x95: // :FETC:DVM:HIGH?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x0495,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0x96: // :FETC:DVM:LOW?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x0496,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0x97: // :FETC:DVM:MAX?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x0497,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0x98: // :FETC:DVM:MIN?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x0498,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0x99: // :FETC:VOLT:HIGH?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x0499,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0x9A: // :FETC:VOLT:LOW?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x049A,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0x9B: // :FETC:VOLT:MAX?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x049B,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0x9C: // :FETC:VOLT:MIN?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x049C,m_readparam);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0xa3: // :SENS:SWE:OFFS:POIN?
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0x23: // :SENS:SWE:OFFS:POIN <NRf>
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Set[] "<<shf;
            return;
        //case 0x9d:break;
        case 0x1d: // :SENS:SWE:POIN <NRf>
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Set[] "<<sht;
            return;
        case 0x9e: // :SENS:SWE:TINT?
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0x1e: // :SENS:SWE:TINT <NRf>
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Set[] "<<shf;
            return;
        case 0xa0: // :MEAS:ARR:VOLT?
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0xa1: // :MEAS:ARR:CURR?
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0xa2: // :MEASure:ARR:DVMeter?
            m_scpiManager->processCHFloatResponse(shf);
            qCDebug(uart_channel)<<"[handleMeasurementcmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
    }
}

void UartChannelManager::handleRegistercmd(quint8 func){
    quint16 sht{0};quint8 sh{0};QString str;

    if (m_readparam.size() == 2){
        sht = qFromBigEndian<quint16>(m_readparam.constData());
    } else if (m_readparam.size() == 4){
        const quint8* bytes = reinterpret_cast<const quint8*>(m_readparam.constData());
        str = QString("%1.%2.%3.%4").arg(bytes[0]).arg(bytes[1]).arg(bytes[2]).arg(bytes[3]);
    } else if (m_readparam.size() == 1){
        sh = static_cast<quint8>(m_readparam[0]);
    }

    switch (func){
        case 0x80: // :STAT:OPER[:EVEN]?
            m_qmlbridge->update_Status(m_channel,sht);
            if (m_scpiCommand == 0x0580){
                m_scpiManager->processCHIntResponse(sht);
                emit to_CanServer(m_channel,0x0580,m_readparam);
            }
            qCDebug(uart_channel)<<"[handleRegistercmd]:Channel_"<<m_channel<<" Query[0x80] "<<sht;
            return;
        case 0x00: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleRegistercmd]:Channel_"<<m_channel<<" Set[0x00] "<<sht;
            return;
        case 0x81: // :SYST:ENAB?
            m_scpiManager->processCHIntResponse(sht);
            emit to_CanServer(m_channel,0x0581,m_readparam);
            qCDebug(uart_channel)<<"[handleRegistercmd]:Channel_"<<m_channel<<" Query[0x81]"<<sht;
            return;
        case 0x82: // :STAT:OPER:COND?
            m_scpiManager->processCHIntResponse(sht);
            emit to_CanServer(m_channel,0x0582,m_readparam);
            qCDebug(uart_channel)<<"[handleRegistercmd]:Channel_"<<m_channel<<" Query[0x82]"<<sht;
            return;
        case 0x02: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleRegistercmd]:Channel_"<<m_channel<<" Set[0x02]"<<sht;
            return;
        case 0x86: // :SYST:CONF?
            m_scpiManager->processCHIntResponse(sht);
            qCDebug(uart_channel)<<"[handleRegistercmd]:Channel_"<<m_channel<<" Query[0x86]"<<sht;
            return;
        case 0x83: // :STAT:QUE[:NEXT]?
            m_scpiManager->processCHIntResponse(sh);
            emit to_CanServer(m_channel,0x0583,m_readparam);
            qCDebug(uart_channel)<<"[handleRegistercmd]:Channel_"<<m_channel<<" Query[0x83]"<<sh;
            return;
        case 0x03: // :STAT:QUE:CLE
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0503,m_readparam);
            qCDebug(uart_channel)<<"[handleRegistercmd]:Channel_"<<m_channel<<" Set[0x03]";
            return;
        case 0x84: // :SYST:SWVersion?
            m_qmlbridge->update_SoftVer(m_channel,str);
            m_scpiManager->processCHStringResponse(str);
            emit to_CanServer(m_channel,0x0584,m_readparam);
            qCDebug(uart_channel)<<"[handleRegistercmd]:Channel_"<<m_channel<<" Query[0x84]"<<str;
            return;
        case 0x85: // :SYST:HWVersion?
            m_qmlbridge->update_HardVer(m_channel,str);
            m_scpiManager->processCHStringResponse(str);
            emit to_CanServer(m_channel,0x0585,m_readparam);
            qCDebug(uart_channel)<<"[handleRegistercmd]:Channel_"<<m_channel<<" Query[0x85]"<<str;
            return;
        case 0xff: // :SYST:REG?<addr>
            m_qmlbridge->update_HardVer(m_channel,str);
            m_scpiManager->processCHStringResponse(str);
            qCDebug(uart_channel)<<"[handleRegistercmd]:Channel_"<<m_channel<<" Query[0xff]"<<str;
            return;

        default:qCCritical(uart_channel)<<"[handleRegistercmd]:unknown func!!!";return;
    }
}

void UartChannelManager::handleCalibratecmd(quint8 func){
    quint8 sh{0};

    if (m_readparam.size() == 1){
        sh = static_cast<quint8>(m_readparam[0]);
    }

    switch (func){
        case 0x00: // :CALI:EXIT
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleCalibratecmd]:Channel_"<<m_channel<<" Set[0x00]";
            return;
        case 0x01: // :CALI:INIT
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0601,m_readparam);
            qCDebug(uart_channel)<<"[handleCalibratecmd]:Channel_"<<m_channel<<" Set[0x01]";
            return;
        case 0x02: // :CALI:REST
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0602,m_readparam);
            qCDebug(uart_channel)<<"[handleCalibratecmd]:Channel_"<<m_channel<<" Set[0x02]";
            return;
        case 0x03: // :CALI:SAVE
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0603,m_readparam);
            qCDebug(uart_channel)<<"[handleCalibratecmd]:Channel_"<<m_channel<<" Set[0x03]";
            return;
        case 0x84: // :CALI:STAR[:ALL]?
            m_scpiManager->processCHIntResponse(sh);
            emit to_CanServer(m_channel,0x0684,m_readparam);
            qCDebug(uart_channel)<<"[handleCalibratecmd]:Channel_"<<m_channel<<" Query[0x84] "<<sh;
            return;
        case 0x04: // :CALI:STAR[:ALL]
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0604,m_readparam);
            qCDebug(uart_channel)<<"[handleCalibratecmd]:Channel_"<<m_channel<<" Set[0x04] "<<sh;
            return;
        case 0x05: // :CALI:STAR:ADC
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0605,m_readparam);
            qCDebug(uart_channel)<<"[handleCalibratecmd]:Channel_"<<m_channel<<" Set[0x05]";
            return;
        case 0x06: // :CALI:STAR:DAC
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0606,m_readparam);
            qCDebug(uart_channel)<<"[handleCalibratecmd]:Channel_"<<m_channel<<" Set[0x06]";
            return;
        case 0x07: // :CALI:STAR:ENABle
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleCalibratecmd]:Channel_"<<m_channel<<" Set[0x07]";
            return;
        case 0x08: // :CALI:STAR:IMPedance
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleCalibratecmd]:Channel_"<<m_channel<<" Set[0x08]";
            return;
        case 0x10: // :CALI:STAR:DCPositiveOffset
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleCalibratecmd]:Channel_"<<m_channel<<" Set[0x10]";
            return;
        case 0x11: // :CALI:STAR:DCNegativeOffset
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleCalibratecmd]:Channel_"<<m_channel<<" Set[0x11]";
            return;
        case 0x16: // :CALI:CLearTC
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleCalibratecmd]:Channel_"<<m_channel<<" Set[0x16]";
            return;
        case 0x17: // :CALI:SAveTC
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleCalibratecmd]:Channel_"<<m_channel<<" Set[0x17]";
            return;
        case 0x18: // :CALI:UPdateTC
            m_scpiManager->processCHVoidResponse();
            qCDebug(uart_channel)<<"[handleCalibratecmd]:Channel_"<<m_channel<<" Set[0x18]";
            return;

        default:qCCritical(uart_channel)<<"[handleCalibratecmd]:unknown func!!!";return;
    }
}

void UartChannelManager::handleCalibrationcmd(quint8 func){
    float shf{0.0f}; // step = func

    if (m_readparam.size()==4){ // // Query and Set
        shf = qFromBigEndian<float>(m_readparam.constData());

        m_scpiManager->processCHFloatResponse(shf);
        emit to_CanServer(m_channel,0x07,m_readparam);
        qCDebug(uart_channel)<<"[handleCalibrationcmd]:Channel_"<<m_channel<<" Query/Set step: "<<func<<" CD: "<<shf;
    }
}

void UartChannelManager::handleTriggercmd(quint8 func){ // new protocol Needs to be change
    float shf{0.0f};quint16 sht{0};quint8 sh{0};

    if (m_readparam.size() == 4){
        shf = qFromBigEndian<float>(m_readparam.constData());
    }else if (m_readparam.size() == 2){
        sht = qFromBigEndian<quint16>(m_readparam.constData());
    }else if (m_readparam.size() == 1){
        sh = static_cast<quint8>(m_readparam[0]);
    }

    switch (func){
        case 0x00: // :ABORt[ch]
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0800,m_readparam);
            qCDebug(uart_channel)<<"[handleTriggercmd]:Channel_"<<m_channel<<" set[]";
            return;
        case 0x01: // :INITiate[ch][:IMMediate]:SEQuence[<n>]
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0801,m_readparam);
            qCDebug(uart_channel)<<"[handleTriggercmd]:Channel_"<<m_channel<<" set[] "<<sh;
            return;
        case 0x02: // :INITiate:CONTinuous:SEQuence1
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0802,m_readparam);
            qCDebug(uart_channel)<<"[handleTriggercmd]:Channel_"<<m_channel<<" set[] "<<sh;
            return;
        case 0x03: // :TRIG:[SEQ1]:[IMM]
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0803,m_readparam);
            qCDebug(uart_channel)<<"[handleTriggercmd]:Channel_"<<m_channel<<" set[]";
            return;
        case 0x04: // :TRIG:SEQ2:[IMM]
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0804,m_readparam);
            qCDebug(uart_channel)<<"[handleTriggercmd]:Channel_"<<m_channel<<" set[]";
            return;
        case 0x05: // :TRIG:SEQ2:SOUR <source>
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0805,m_readparam);
            qCDebug(uart_channel)<<"[handleTriggercmd]:Channel_"<<m_channel<<" set[] "<<sh;
            return;
        case 0x85: // :TRIG:SEQ2:SOUR?
            m_scpiManager->processCHIntResponse(sh);
            emit to_CanServer(m_channel,0x0885,m_readparam);
            qCDebug(uart_channel)<<"[handleTriggercmd]:Channel_"<<m_channel<<" Query[] "<<sh;
            return;
        case 0x06: // :TRIG:SEQ2:COUN:{CURR|DVM|VOLT} n
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0806,m_readparam);
            qCDebug(uart_channel)<<"[handleTriggercmd]:Channel_"<<m_channel<<" set[] "<<sht;
            return;
        case 0x86: // :TRIG:SEQ2:COUN:{CURR|DVM|VOLT}?
            m_scpiManager->processCHIntResponse(sht);
            emit to_CanServer(m_channel,0x0886,m_readparam);
            qCDebug(uart_channel)<<"[handleTriggercmd]:Channel_"<<m_channel<<" Query[] "<<sht;
            return;
        case 0x07: // :TRIG:SEQ2:HYST:{CURR|DVM|VOLT} n
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0807,m_readparam);
            qCDebug(uart_channel)<<"[handleTriggercmd]:Channel_"<<m_channel<<" set[] "<<shf;
            return;
        case 0x87: // :TRIG:SEQ2:HYST:{CURR|DVM|VOLT}?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x0887,m_readparam);
            qCDebug(uart_channel)<<"[handleTriggercmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0x08: // :TRIG:SEQ2:LEV:{CURR|DVM|VOLT} n
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0808,m_readparam);
            qCDebug(uart_channel)<<"[handleTriggercmd]:Channel_"<<m_channel<<" set[] "<<shf;
            return;
        case 0x88: // :TRIG:SEQ2:LEV:{CURR|DVM|VOLT}?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x0888,m_readparam);
            qCDebug(uart_channel)<<"[handleTriggercmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0x09: // :TRIG:SEQ2:SLOP:{CURR|DVM|VOLT} Type
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x0809,m_readparam);
            qCDebug(uart_channel)<<"[handleTriggercmd]:Channel_"<<m_channel<<" set[] "<<sh;
            return;
        case 0x89: // :TRIG:SEQ2:SLOP:{CURR|DVM|VOLT}?
            m_scpiManager->processCHIntResponse(sh);
            emit to_CanServer(m_channel,0x0889,m_readparam);
            qCDebug(uart_channel)<<"[handleTriggercmd]:Channel_"<<m_channel<<" Query[] "<<sh;
            return;
        case 0x8A: // :SOUR:VOLT:AMPL:TRIG?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x088A,m_readparam);
            qCDebug(uart_channel)<<"[handleTriggercmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0x0A: // :SOUR:VOLT:AMPL:TRIG <NRf>
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x080A,m_readparam);
            qCDebug(uart_channel)<<"[handleTriggercmd]:Channel_"<<m_channel<<" set[] "<<shf;
            return;
        case 0x8B: // :SOUR:CURR:TRIG?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x088B,m_readparam);
            qCDebug(uart_channel)<<"[handleTriggercmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0x0B: // :SOUR:CURR:TRIG <NRf>
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x080B,m_readparam);
            qCDebug(uart_channel)<<"[handleTriggercmd]:Channel_"<<m_channel<<" set[] "<<shf;
            return;
        case 0x8C: // :SOUR:RES:TRIG?
            m_scpiManager->processCHFloatResponse(shf);
            emit to_CanServer(m_channel,0x088C,m_readparam);
            qCDebug(uart_channel)<<"[handleTriggercmd]:Channel_"<<m_channel<<" Query[] "<<shf;
            return;
        case 0x0C: // :SOUR:RES:TRIG <NRf>
            m_scpiManager->processCHVoidResponse();
            emit to_CanServer(m_channel,0x080C,m_readparam);
            qCDebug(uart_channel)<<"[handleTriggercmd]:Channel_"<<m_channel<<" set[] "<<shf;
            return;

        default:qCCritical(uart_channel)<<"[handleTriggercmd]:unknown func!!!";return;
    }
}

// new protocol needs to be complete other section - battery

void UartChannelManager::handleISPcmd(quint8 func){
    switch (func){
        case 0x80: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleISPcmd]:Channel_"<<m_channel<<" Query[0x80] ";
            return;
        case 0x00: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleISPcmd]:Channel_"<<m_channel<<" Set[0x00] ";
            return;
        case 0x01: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleISPcmd]:Channel_"<<m_channel<<" Set[0x01] ";
            return;
        case 0x02: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleISPcmd]:Channel_"<<m_channel<<" Set[0x02] ";
            return;

        default:qCCritical(uart_channel)<<"[handleISPcmd]:unknown func!!!";return;
    }
}

void UartChannelManager::handleSNcmd(quint8 func){
    switch (func){
        case 0x80: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleSNcmd]:Channel_"<<m_channel<<" Query[0x80] ";
            return;
        case 0x00: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleSNcmd]:Channel_"<<m_channel<<" Set[0x00] ";
            return;
        case 0x81: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleSNcmd]:Channel_"<<m_channel<<" Query[0x81] ";
            return;
        case 0x01: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleSNcmd]:Channel_"<<m_channel<<" Set[0x01] ";
            return;
        case 0x82: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleSNcmd]:Channel_"<<m_channel<<" Query[0x82] ";
            return;
        case 0x02: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleSNcmd]:Channel_"<<m_channel<<" Set[0x02] ";
            return;
        case 0x83: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleSNcmd]:Channel_"<<m_channel<<" Query[0x83] ";
            return;
        case 0x03: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleSNcmd]:Channel_"<<m_channel<<" Set[0x03] ";
            return;
        case 0x84: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleSNcmd]:Channel_"<<m_channel<<" Query[0x84] ";
            return;
        case 0x04: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleSNcmd]:Channel_"<<m_channel<<" Set[0x04] ";
            return;
        case 0x05: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleSNcmd]:Channel_"<<m_channel<<" Set[0x05] ";
            return;

        default:qCCritical(uart_channel)<<"[handleSNcmd]:unknown func!!!";return;
    }
}

void UartChannelManager::handleIDcmd(quint8 func){
    switch (func){
        case 0x81: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleIDcmd]:Channel_"<<m_channel<<" Query[0x81] ";
            return;
        case 0x82: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleIDcmd]:Channel_"<<m_channel<<" Query[0x82] ";
            return;
        case 0x83: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleIDcmd]:Channel_"<<m_channel<<" Query[0x83] ";
            return;
        case 0x84: // not SCPI cmd
            qCDebug(uart_channel)<<"[handleIDcmd]:Channel_"<<m_channel<<" Query[0x84] ";
            return;

        default:qCCritical(uart_channel)<<"[handleIDcmd]:unknown func!!!";return;
    }
}

void UartChannelManager::handleErrorcmd(quint8 func){
    switch (func){
        case 0x00:qCDebug(uart_channel)<<"[handleErrorcmd]:Channel_"<<m_channel<<" Error Response: CheckSum Error";      break;
        case 0x01:qCDebug(uart_channel)<<"[handleErrorcmd]:Channel_"<<m_channel<<" Error Response: Unknow Command";      break;
        case 0x02:qCDebug(uart_channel)<<"[handleErrorcmd]:Channel_"<<m_channel<<" Error Response: Unknow Function";     break;
        case 0x03:qCDebug(uart_channel)<<"[handleErrorcmd]:Channel_"<<m_channel<<" Error Response: Error Length";        break;
        case 0x04:qCDebug(uart_channel)<<"[handleErrorcmd]:Channel_"<<m_channel<<" Error Response: Invalid Parameter";   break;
        case 0x05:qCDebug(uart_channel)<<"[handleErrorcmd]:Channel_"<<m_channel<<" Error Response: Illegal Command";     break;
        case 0x06:qCDebug(uart_channel)<<"[handleErrorcmd]:Channel_"<<m_channel<<" Error Response: Unsupported Command"; break;
        default:qCCritical(uart_channel)<<"[handleErrorcmd]:unknown func!!!";return;
    }
}

// new protocol needs to be complete other section - test
