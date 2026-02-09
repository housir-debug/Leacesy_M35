#include "serialworker.h"
#include <QtCore>

// ========================== 初始化部分 ===================================

Q_LOGGING_CATEGORY(uart_channel, "UART_CHANNEL:")

SerialWorker::SerialWorker(QObject *parent): QObject(parent){
    portChannelMap["/dev/ttyS4"] = 0x01;
    portChannelMap["/dev/ttyS5"] = 0x02;
    portChannelMap["/dev/ttyS6"] = 0x03;
    portChannelMap["/dev/ttyS7"] = 0x04;
}
SerialWorker::~SerialWorker()
{
    qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Serial~Destruct Finished.";
    m_isTesting.store(false);
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

bool SerialWorker::initSerialPort(const QString &portName,
                                 qint32 baudRate,
                                 QSerialPort::DataBits dataBits,
                                 QSerialPort::Parity parity,
                                 QSerialPort::StopBits stopBits)
{
    if (!m_serialThread){
        auto it = portChannelMap.find(portName);
        if (it != portChannelMap.end()) {
            m_channel = it.value();
        } else {
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

        connect(m_serialPort, &QSerialPort::readyRead, this, &SerialWorker::handleReadyRead, Qt::DirectConnection);
        connect(m_serialPort, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
            if (error == QSerialPort::NoError) {return;}
            qCWarning(uart_channel) <<"Channel_"<<m_channel<<" Occur Error: "<<m_serialPort->errorString();
        }, Qt::DirectConnection);
        connect(m_refreshtimer,&QTimer::timeout,this,[this]{
            static int step = 0;
            switch (step) {
                case 0:  writeFrame(0x04,0x80,""); break;
                case 1:  writeFrame(0x04,0x81,""); break;
                case 2:  writeFrame(0x05,0x80,""); break;
            }
            step = (step + 1) % 3;
        }, Qt::DirectConnection);

        QMetaObject::invokeMethod(this, [this]() {
            if (m_serialPort->open(QIODevice::ReadWrite)) {
                m_writebuffer.append(QByteArray::fromHex("aa 55 04 01 00 01 06 ee"));
                writeSerialData(m_writebuffer,true);
                m_writebuffer.clear();
                m_writebuffer.append(QByteArray::fromHex("aa 55 05 04 0e 01 00 18 ee"));
                writeSerialData(m_writebuffer,true);
                m_writebuffer.clear();
                m_writebuffer.append(QByteArray::fromHex("aa 55 08 04 0c 01 3f 80 00 00 d8 ee"));
                writeSerialData(m_writebuffer,true);
                m_writebuffer.clear();
                m_writebuffer.append(QByteArray::fromHex("aa 55 08 04 1e 01 37 82 dc bf 7f ee"));
                writeSerialData(m_writebuffer,true);
                m_writebuffer.clear();
                m_writebuffer.append(QByteArray::fromHex("aa 55 05 04 0f 01 01 1a ee"));
                writeSerialData(m_writebuffer,true);
                m_writebuffer.clear();
                m_writebuffer.append(QByteArray::fromHex("aa 55 08 02 00 01 00 00 00 00 0b ee"));
                writeSerialData(m_writebuffer,true);
                m_writebuffer.clear();
                m_writebuffer.append(QByteArray::fromHex("aa 55 08 02 01 01 3f 80 00 00 cb ee"));
                writeSerialData(m_writebuffer,true);
                m_writebuffer.clear();
                m_writebuffer.append(QByteArray::fromHex("aa 55 08 04 1f 01 ff ff ff ff 28 ee"));
                writeSerialData(m_writebuffer,true);
                m_writebuffer.clear();
                m_writebuffer.append(QByteArray::fromHex("aa 55 08 02 03 01 41 00 00 00 4f ee"));
                writeSerialData(m_writebuffer,true);
                m_writebuffer.clear();
                m_writebuffer.append(QByteArray::fromHex("aa 55 06 04 1d 01 00 01 29 ee"));
                writeSerialData(m_writebuffer,true);
                m_writebuffer.clear();
                m_writebuffer.append(QByteArray::fromHex("aa 55 08 02 02 01 00 00 00 00 0d ee"));
                writeSerialData(m_writebuffer,true);
                m_writebuffer.clear();

                m_refreshtimer->start();
                // startLoopbackTest();   // Self-assessment
                // QTimer::singleShot(0,this,[this](){writeFrame();});
            }
        }, Qt::QueuedConnection);

        return true;
    }

    return false;
}

// ========================== 信息处理部分 ===================================

void SerialWorker::writeFrame(quint8 cmd, quint8 func, const QByteArray& param) {
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
}

void SerialWorker::writeSerialData(const QByteArray& data,bool isforce)
{
    if (!m_serialPort) {return;}
    qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Send: " << data.toHex(' ');

    int result = m_serialPort->write(data);
    if (result != data.size()) {
        qCCritical(uart_channel)<<"Channel_"<<m_channel<<" QSerialPort Written Buffer Overflow!!!";
    }

    if(isforce){
        m_serialPort->flush();   // The same event is sent multiple times, and multiple messages will be sent together.
        qCDebug(uart_channel)<<"Channel_"<<m_channel<<"QSerialPort Force Flush ";
        QThread::msleep(6);
    }
}

void SerialWorker::handleReadyRead()
{
    m_readbuffer.append(m_serialPort->readAll());
    if (m_readbuffer.isEmpty()){return;}

    emit serialDataReceived(m_readbuffer,false);
    if(m_channel==1){qCDebug(uart_channel) <<"Channel_"<<m_channel<<" (我<-收)Received" << m_readbuffer.toHex(' ');
    }else{qCDebug(uart_channel) <<"Channel_"<<m_channel<<" (发->电芯)Received" << m_readbuffer.toHex(' ');}

    // Test progressing
    if (m_isTesting.load()) {
        if (m_readbuffer.size() >= 1024) {  // 1 KB
            qint64 elapsed = m_testTimer.elapsed(); // ms
            double speedKBps =  (1024 * 1000.0) / (elapsed * 1024);
            double speedBps = 1024 * 1000.0 / elapsed;

            qCDebug(uart_channel) << "\n" << QString(
                "Loopback Test Result:"
                "Time elapsed: %1 ms"
                "Speed: %2 KB/s (%3 bps)"
            ).arg(elapsed).arg(speedKBps, 0, 'f', 2).arg(speedBps * 8, 0, 'f', 0);

            m_isTesting.store(false);
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
                handleuartrequest(lengthB,m_readbuffer);
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

bool SerialWorker::handleuartrequest(quint8 length,const QByteArray& data){
    quint8 cmd = static_cast<quint8>(data[3]);
    quint8 func = static_cast<quint8>(data[4]);
    quint8 ch = static_cast<quint8>(data[5]);
    quint8 Checksum = length + cmd + func + ch; // The check code is taken from the lowest 8 bits.

    m_readparam.clear();
    m_readparam= data.mid(6, length - 4);   // length - Command+Function+Channel+CheckSum
    for (char byte : qAsConst(m_readparam)) {Checksum += static_cast<quint8>(byte);}

    if(static_cast<quint8>(data[length + 2]) != Checksum){
        qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Received Data Incorrect!!!";
        return false;
    }

    switch (cmd) {
        case 0x01:
            handleOutputcmd(func,ch,m_readparam);            break;

        case 0x02:
            handleSettingcmd(func, ch, m_readparam);         break;

        case 0x03:
            handleControlcmd(func, ch, m_readparam);         break;

        case 0x04:
            handleMeasurementcmd(func, ch, m_readparam);     break;

        case 0x05:
            handleRegistercmd(func, ch, m_readparam);        break;

        case 0x06:
            handleCalibratecmd(func, ch, m_readparam);       break;

        case 0x07:
            handleCalibrationcmd(func, ch, m_readparam);     break;

        case 0x08:
            handleTriggercmd(func, ch, m_readparam);         break;

        case 0x09:
            handleISPcmd(func, ch, m_readparam);             break;

        case 0x10:
            handleSNcmd(func, ch, m_readparam);              break;

        case 0x11:
            handleIDcmd(func, ch, m_readparam);              break;

        case 0xFF:
            handleErrorcmd(func, ch, m_readparam);           break;

        default:
            qCWarning(uart_channel)<<"Channel_"<<m_channel<<" Occuring Unknown Command!!!";
            return false;
    }

    return true;
}

// ========================== 协议处理部分 ===================================

void SerialWorker::handleOutputcmd(quint8 func, quint8 ch, const QByteArray& param){
    Q_UNUSED(ch);

    switch (func){
        case 0x80:{   // query output status
            quint8 raw = static_cast<quint8>(param[0]);
            if (raw == 0){
                emit channelreturnstatus(false);
                qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Output Query Off.";}
            else{
                emit channelreturnstatus(true);
                qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Output Query On.";}
            return;
        }
        case 0x00:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Output Been OFF";return;
        case 0x01:qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Output Been ON";return;
        case 0x08:{
            quint8 raw = static_cast<quint8>(param[0]);
            if (raw == 0){
                qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Output Bandwidth Been LOW.";}
            else{
                qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Output Bandwidth Been HIGH.";}
            return;
        }
        case 0x88:{   // query output bandwidth
            quint8 raw = static_cast<quint8>(param[0]);
            if (raw == 0){
                emit channelreturnstatus(false);
                qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Output Query bandwidth low.";}
            else{
                emit channelreturnstatus(true);
                qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Output Query bandwidth high.";}
            return;
        }
        case 0x09:{
            quint8 raw = static_cast<quint8>(param[0]);
            switch (raw) {
                case 1:
                    qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Output COMPMODE Been Llocal.";return;

                case 2:
                    qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Output COMPMODE Been Lremote.";return;

                case 3:
                    qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Output COMPMODE Been Hlocal.";return;

                case 4:
                    qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Output COMPMODE Been Hremote.";return;

                default:
                    return;
            }
        }
        case 0x89:{   // query output compmode
            quint8 raw = static_cast<quint8>(param[0]);
            switch (raw) {
                case 1:
                    emit channelreturnvalue(1);
                    qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Output Query COMPMODE Llocal.";return;
                case 2:
                    emit channelreturnvalue(2);
                    qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Output Query COMPMODE Lremote.";return;
                case 3:
                    emit channelreturnvalue(3);
                    qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Output Query COMPMODE Hlocal.";return;
                case 4:
                    emit channelreturnvalue(4);
                    qCDebug(uart_channel)<<"Channel_"<<m_channel<<" Output Query COMPMODE Hremote.";return;
                default:
                    return;
            }
        }
    }
}

void SerialWorker::handleSettingcmd(quint8 func, quint8 ch, const QByteArray& param){
    Q_UNUSED(ch);Q_UNUSED(param);

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
        case 0x85:break;
        case 0x05:break;
        case 0x86:break;
        case 0x06:break;
        case 0x87:break;
        case 0x07:break;
        case 0x88:break;
        case 0x08:break;
    }
}

void SerialWorker::handleControlcmd(quint8 func, quint8 ch, const QByteArray& param){
    Q_UNUSED(ch);Q_UNUSED(param);

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
        case 0x85:break;
        case 0x05:break;
        case 0x06:break;
        case 0x07:break;
        case 0x88:break;
        case 0x08:break;
        case 0x89:break;
        case 0x09:break;
    }
}

void SerialWorker::handleMeasurementcmd(quint8 func, quint8 ch, const QByteArray& param){
    Q_UNUSED(ch);

    quint32 raw = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(param.constData()));
    float shf;   memcpy(&shf, &raw, sizeof(float));
    float EPSILON = qAbs(shf) < 1e-4? 0.00000001f : 0.00001f;

    switch (func){
        case 0x80: // voltage /V
                qCDebug(uart_channel)<<"Voltage Present:"<<shf<<"Last:"<<lastVoltage;
                if (qAbs(shf - lastVoltage) >= EPSILON) {
                    emit voltageChanged(m_channel,shf);
                    // emit channelreturnvalue(shf);
                    lastVoltage = shf;
                }
                break;
        case 0x81: // current /A
                qCDebug(uart_channel)<<"Current Present:"<<shf<<"Last:"<<lastCurrent;
                if (qAbs(shf - lastCurrent) >= EPSILON) {
                    emit currentChanged(m_channel,shf);
                    // emit channelreturnvalue(shf);
                    lastCurrent = shf;
                }
                break;
        case 0x82: // small current /mA
                qCDebug(uart_channel)<<"Small Current Present:"<<shf<<"Last:"<<lastSmallCurrent;
                if (qAbs(shf - lastSmallCurrent) >= EPSILON) {
                    emit smallcurrentChanged(m_channel,shf);
                    // emit channelreturnvalue(shf);
                    lastSmallCurrent = shf;
                }
                break;
        case 0x83: // board temperature /degC
                qCDebug(uart_channel)<<"Board Temperature Present:"<<shf<<"Last:"<<lasttemper;
                if (qAbs(shf - lasttemper) >= EPSILON) {
                    emit temperatureChanged(m_channel,shf);
                    // emit channelreturnvalue(shf);
                    lasttemper = shf;
                }
                break;
        case 0x84: // heatsink temperature /degC
                qCDebug(uart_channel)<<"Heatsink Temperature Present:"<<shf<<"Last:"<<lastheatsinktemper;
                if (qAbs(shf - lastheatsinktemper) >= EPSILON) {
                    emit sinktemperatureChanged(m_channel,shf);
                    // emit channelreturnvalue(shf);
                    lastheatsinktemper = shf;
                }
                break;
        case 0x85: // DVM ACDC voltage /V
                qCDebug(uart_channel)<<"DVM ACDC Voltage Present:"<<shf<<"Last:"<<lastDVMACDCVoltage;
                if (qAbs(shf - lastDVMACDCVoltage) >= EPSILON) {
                    emit DVMACDCVoltageChanged(m_channel,shf);
                    // emit channelreturnvalue(shf);
                    lastDVMACDCVoltage = shf;
                }
                break;
        case 0x86: // DVM voltage /V
                qCDebug(uart_channel) << "DVM Voltage Present:"<<shf<<"Last:"<<lastDVMVoltage;
                if (qAbs(shf - lastDVMVoltage) >= EPSILON) {
                    emit DVMVoltageChanged(m_channel,shf);
                    // emit channelreturnvalue(shf);
                    lastDVMVoltage = shf;
                }
                break;
        case 0x87:break;
        case 0x9D:break;
        case 0x89:break;
        case 0x8A:break;
        case 0x8B:break;
        case 0x8D:break;
        case 0xb0:break;
        case 0xb1:break;
        case 0xb2:break;
        case 0xb6:break;
        case 0x8C:break;
        case 0x0C:break;
        case 0x9F:break;
        case 0x1F:break;
        case 0x8E:break;
        case 0x0E:break;
        case 0x8F:break;
        case 0x0F:break;
        case 0x90:break;
        case 0x10:break;
        case 0x91:break;
        case 0x92:break;
        case 0x93:break;
        case 0x94:break;
        case 0x95:break;
        case 0x96:break;
        case 0x97:break;
        case 0x98:break;
        case 0x99:break;
        case 0x9A:break;
        case 0x9B:break;
        case 0x9C:break;
        case 0xa3:break;
        case 0x23:break;
        // case 0x9d:break;
        case 0x1d:break;
        case 0x9e:break;
        case 0x1e:break;
        case 0xa0:break;
        case 0xa1:break;
        case 0xa2:break;

    }
}

void SerialWorker::handleRegistercmd(quint8 func, quint8 ch, const QByteArray& param){
    Q_UNUSED(ch);

    switch (func){
        case 0x80:emit statusChanged(m_channel,param);break;
        case 0x81:break;
        case 0x82:break;
        case 0x83:break;
        case 0x03:break;
        case 0x84:break;
        case 0x85:break;
    }
}

void SerialWorker::handleCalibratecmd(quint8 func, quint8 ch, const QByteArray& param){
    Q_UNUSED(ch);Q_UNUSED(param);

    switch (func){
        case 0x00:break;
        case 0x01:break;
        case 0x02:break;
        case 0x03:break;
        case 0x84:break;
        case 0x04:break;
        case 0x05:break;
        case 0x06:break;
        case 0x07:break;
        case 0x08:break;
        case 0x10:break;
        case 0x11:break;
    }
}

void SerialWorker::handleCalibrationcmd(quint8 func, quint8 ch, const QByteArray& param){
    Q_UNUSED(func);Q_UNUSED(ch);Q_UNUSED(param);
}

void SerialWorker::handleTriggercmd(quint8 func, quint8 ch, const QByteArray& param){
    Q_UNUSED(ch);Q_UNUSED(param);

    switch (func){
        case 0x00:break;
        case 0x01:break;
        case 0x02:break;
        case 0x03:break;
        case 0x04:break;
        case 0x05:break;
        case 0x85:break;
        case 0x06:break;
        case 0x86:break;
        case 0x07:break;
        case 0x87:break;
        case 0x08:break;
        case 0x88:break;
        case 0x09:break;
        case 0x89:break;
        case 0x8A:break;
        case 0x0A:break;
        case 0x8B:break;
        case 0x0B:break;
        case 0x8C:break;
        case 0x0C:break;
    }
}

void SerialWorker::handleISPcmd(quint8 func, quint8 ch, const QByteArray& param){
    Q_UNUSED(ch);Q_UNUSED(param);

    switch (func){
        case 0x80:break;
        case 0x00:break;
        case 0x01:break;
        case 0x02:break;
    }
}

void SerialWorker::handleSNcmd(quint8 func, quint8 ch, const QByteArray& param){
    Q_UNUSED(ch);Q_UNUSED(param);

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

void SerialWorker::handleIDcmd(quint8 func, quint8 ch, const QByteArray& param){
    Q_UNUSED(ch);Q_UNUSED(param);

    switch (func){
        case 0x81:break;
        case 0x82:break;
        case 0x83:break;
        case 0x84:break;
    }
}

void SerialWorker::handleErrorcmd(quint8 func, quint8 ch, const QByteArray& param){
    Q_UNUSED(ch);Q_UNUSED(param);

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

void SerialWorker::startLoopbackTest()
{
    if (m_isTesting.load()) {return;}

    m_isTesting.store(true);

    // 生成测试数据（1KB）
    QByteArray testData;
    testData.resize(1024);
    for (int i = 0; i < 1024; i++) {testData[i] = i % 256;}

    qCDebug(uart_channel)<<"Starting Loopback Test (Connect TX to RX for testing)...";
    qCDebug(uart_channel)<<"Sending"<<testData.size()<<"bytes of Test Data";
    m_testTimer.start();
    writeSerialData(testData,false);
}
