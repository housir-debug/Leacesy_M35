#include "serialworker.h"

// ========================== 初始化部分 ===================================

Q_LOGGING_CATEGORY(uart, "uart:")

SerialWorker::SerialWorker(QObject *parent): QObject(parent){}

bool SerialWorker::initSerialPort(const QString &portName,
                                 qint32 baudRate,
                                 QSerialPort::DataBits dataBits,
                                 QSerialPort::Parity parity,
                                 QSerialPort::StopBits stopBits)
{
    if (m_serialPort) {return false;}

    if (!m_serialThread){
        m_portName = portName;

        m_serialPort = new QSerialPort(this);
        m_serialPort->setFlowControl(QSerialPort::NoFlowControl);// 大多数情况使用
        //m_serialPort->setReadBufferSize(1024 * 1024); // 1MB buffer

        m_serialPort->setPortName(portName);
        m_serialPort->setBaudRate(baudRate);
        m_serialPort->setDataBits(dataBits);
        m_serialPort->setParity(parity);
        m_serialPort->setStopBits(stopBits);

        m_refreshtimer = new QTimer;
        m_refreshtimer->setInterval(600); // ms

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

        connect(m_serialPort, &QSerialPort::readyRead, this, &SerialWorker::handleReadyRead);
        connect(m_serialPort, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
            if (error == QSerialPort::NoError) {return;}
            qCWarning(uart) << QString("Serial port: %1 error: %2").arg(this->m_portName,m_serialPort->errorString());
        });
        connect(m_refreshtimer,&QTimer::timeout,this,[this]{
            static int step = 0;
            switch (step) {
                case 0:  writeFrame(0x04,0x80,0x01,""); break;
                case 1:  writeFrame(0x04,0x81,0x01,""); break;
                case 2:  writeFrame(0x05,0x80,0x01,""); break;
            }
            step = (step + 1) % 3;

            /*writeFrame(0x04,0x80,0x01,"");QThread::msleep(7);
            writeFrame(0x04,0x81,0x01,"");QThread::msleep(7);
            writeFrame(0x05,0x80,0x01,"");*/
        });

        QMetaObject::invokeMethod(this, [this]() {
            if (!m_serialPort->open(QIODevice::ReadWrite)) {
                qCWarning(uart) << "Failed open port:"<< m_portName << "Error:" << m_serialPort->errorString();
                delete m_serialPort;
                m_serialPort = nullptr;
            }
            m_refreshtimer->start();
            // startLoopbackTest();   // Self-assessment
         }, Qt::QueuedConnection);

        return true;
    }

    return false;
}

// ========================== 信息处理部分 ===================================

void SerialWorker::writeFrame(quint8 cmd, quint8 func, quint8 ch, const QByteArray& param) {
    quint8 length = 4 + param.size();  // （ Command+Function+Channel+CheckSum ）+Parameter
    quint8 checksum = length + cmd + func + ch; // The check code is taken from the lowest 8 bits.
    for (char byte : param) {checksum += static_cast<quint8>(byte);}

    QByteArray frame;
    frame.reserve(length + 4);  // Pre-allocation enhances performance

    frame.append(HEADER_HIGH);
    frame.append(HEADER_LOW);
    frame.append(length);
    frame.append(cmd);
    frame.append(func);
    frame.append(ch);
    frame.append(param);
    frame.append(checksum);
    frame.append(END_MARKER);

    writeSerialData(frame);
}

void SerialWorker::writeSerialData(const QByteArray &data)
{
    QMutexLocker locker(&m_WriteMutex);

    if (!m_serialPort) {return;}
    qCDebug(uart)<< m_portName << "Send: " << data.toHex(' ');

    if (m_serialPort->write(data) != data.size()) {
        qCWarning(uart) << m_portName << "written buffer overflow!!!";
        return;
    }

    // m_serialPort->flush();   //The same event is sent multiple times, and multiple messages will be sent together.
    // qCDebug(uart) << "Flush result:" << flushed<< "bytes waiting:" << m_serialPort->bytesToWrite();
}

void SerialWorker::handleReadyRead()
{
    QByteArray data = m_serialPort->readAll();
    if (data.isEmpty()){return;}

    emit serialDataReceived(data);

    if(m_portName=="/dev/ttyS4"){qCDebug(uart) <<m_portName<<"(我)Received" << data.toHex(' ');
    }else{qCDebug(uart) <<m_portName<<"(电芯)Received" << data.toHex(' ');}

    // Test progressing
    if (m_isTesting.load()) {
        m_bytesReceived += data.size();

        if (m_bytesReceived >= 1024) {  // 1 KB
            qint64 elapsed = m_testTimer.elapsed(); // ms
            double speedKBps =  (1024 * 1000.0) / (elapsed * 1024);
            double speedBps = 1024 * 1000.0 / elapsed;

            qCDebug(uart) << "\n" << QString(
                "Loopback Test Result:"
                "Time elapsed: %1 ms"
                "Speed: %2 KB/s (%3 bps)"
            ).arg(elapsed).arg(speedKBps, 0, 'f', 2).arg(speedBps * 8, 0, 'f', 0);

            m_isTesting.store(false);
            return;
        }

        return;
    }

    // Normal response processing of the protocol
    if (data.size() >= 3){
        if(static_cast<quint8>(data[0]) == HEADER_LOW && static_cast<quint8>(data[1]) == HEADER_HIGH){
            quint8 lengthA = static_cast<quint8>(data[2]);

            if(data.size() == lengthA + 4){
                if(static_cast<quint8>(data[lengthA + 3]) != END_MARKER){
                    qCWarning(uart) <<m_portName<<"Received data format Error!!!";
                    return;
                }

                handleuartrequest(lengthA,data);
                data.remove(0,lengthA + 4);
                if(data != nullptr){handleReadyRead();}
                return;
            }
        }
    }

    // Handling of abnormal protocol responses
    m_buffer.append(data);
    if (m_buffer.size() >= 3){
        if(static_cast<quint8>(m_buffer[0]) == HEADER_LOW && static_cast<quint8>(m_buffer[1]) == HEADER_HIGH){
            quint8 lengthB = static_cast<quint8>(m_buffer[2]);
            if (m_buffer.size() < lengthB + 4){return;}

            if(static_cast<quint8>(data[lengthB + 3]) == END_MARKER){
                handleuartrequest(lengthB,m_buffer);
                m_buffer.remove(0,lengthB + 4);
                return;
            }
        }

        m_buffer.clear();
        return;
    }
}

bool SerialWorker::handleuartrequest(quint8 length,const QByteArray &data){
    quint8 cmd = static_cast<quint8>(data[3]);
    quint8 func = static_cast<quint8>(data[4]);
    quint8 ch = static_cast<quint8>(data[5]);
    quint8 Checksum = length + cmd + func + ch; // The check code is taken from the lowest 8 bits.

    QByteArray param;
    for (int i = 6; i < length+2; i++) {
        param.append(static_cast<quint8>(data[i]));
        Checksum += static_cast<quint8>(data[i]);
    }

    if(static_cast<quint8>(data[length + 2]) != Checksum){
        qCDebug(uart) <<m_portName<<"Received data incorrect!!!";
        return false;
    }

    switch (cmd) {
        case 0x01:
            handleOutputcmd(func,ch,param);            break;

        case 0x02:
            handleSettingcmd(func, ch, param);         break;

        case 0x03:
            handleControlcmd(func, ch, param);         break;

        case 0x04:
            handleMeasurementcmd(func, ch, param);     break;

        case 0x05:
            handleRegistercmd(func, ch, param);        break;

        case 0x06:
            handleCalibratecmd(func, ch, param);       break;

        case 0x07:
            handleCalibrationcmd(func, ch, param);     break;

        case 0x08:
            handleTriggercmd(func, ch, param);         break;

        case 0x09:
            handleISPcmd(func, ch, param);             break;

        case 0x10:
            handleSNcmd(func, ch, param);              break;

        case 0x11:
            handleIDcmd(func, ch, param);              break;

        case 0xFF:
            handleErrorcmd(func, ch, param);              break;
    }

    return true;
}

// ========================== 协议处理部分 ===================================

void SerialWorker::handleOutputcmd(quint8 func, quint8 ch, const QByteArray& param){
    Q_UNUSED(ch);Q_UNUSED(param);
    switch (func){
        case 0x80:break;
        case 0x00:break;
        case 0x01:break;
        case 0x08:break;
        case 0x88:break;
        case 0x09:break;
        case 0x89:break;
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
    switch (func){
        case 0x80: // voltage /V
                qCDebug(uart) << "voltage raw:" << shf << "last:" << lastVoltage;
                if (qAbs(shf - lastVoltage) >= EPSILON) {
                    emit voltageChanged(shf);
                    lastVoltage = shf;
                }
                break;
        case 0x81: // current /A
                qCDebug(uart) << "current raw:" << shf << "last:" << lastCurrent;
                if (qAbs(shf - lastCurrent) >= EPSILON) {
                    emit currentChanged(shf);
                    lastCurrent = shf;
                }
                break;
        case 0x82: // small current /mA
                qCDebug(uart) << "small current raw:" << shf << "last:" << lastSmallCurrent;
                if (qAbs(shf - lastSmallCurrent) >= EPSILON) {
                    emit smallcurrentChanged(shf);
                    lastSmallCurrent = shf;
                }
                break;
        case 0x83: // board temperature /degC
                qCDebug(uart) << "board temperature raw:" << shf << "last:" << lasttemper;
                if (qAbs(shf - lasttemper) >= EPSILON) {
                    emit temperatureChanged(shf);
                    lasttemper = shf;
                }
                break;
        case 0x84: // heatsink temperature /degC
                qCDebug(uart) << "heatsink temperature raw:" << shf << "last:" << lastheatsinktemper;
                if (qAbs(shf - lastheatsinktemper) >= EPSILON) {
                    emit sinktemperatureChanged(shf);
                    lastheatsinktemper = shf;
                }
                break;
        case 0x85: // DVM ACDC voltage /V
                qCDebug(uart) << "DVM ACDC voltage raw:" << shf << "last:" << lastDVMACDCVoltage;
                if (qAbs(shf - lastDVMACDCVoltage) >= EPSILON) {
                    emit DVMACDCVoltageChanged(shf);
                    lastDVMACDCVoltage = shf;
                }
                break;
        case 0x86: // DVM voltage /V
                qCDebug(uart) << "DVM voltage raw:" << shf << "last:" << lastDVMVoltage;
                if (qAbs(shf - lastDVMVoltage) >= EPSILON) {
                    emit DVMVoltageChanged(shf);
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
    Q_UNUSED(ch);Q_UNUSED(param);
    switch (func){
        case 0x80:break;
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
        case 0x00:qCDebug(uart) << "Error Response: CheckSum error";      break;
        case 0x01:qCDebug(uart) << "Error Response: Unknow command";      break;
        case 0x02:qCDebug(uart) << "Error Response: Unknow function";     break;
        case 0x03:qCDebug(uart) << "Error Response: Error length";        break;
        case 0x04:qCDebug(uart) << "Error Response: Invalid parameter";   break;
        case 0x05:qCDebug(uart) << "Error Response: Illegal command";     break;
        case 0x06:qCDebug(uart) << "Error Response: Unsupported command"; break;
    }
}

// ========================== 析构部分 ===================================

SerialWorker::~SerialWorker()
{
    qCDebug(uart) << "Serial~ delete finished ："<< m_portName;
    closeSerial();
}

void SerialWorker::closeSerial()
{
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

// ========================== 模块性能自测 ===================================

void SerialWorker::startLoopbackTest()
{
    if (m_isTesting.load()) {return;}

    m_isTesting.store(true);
    m_bytesReceived = 0;

    // 生成测试数据（1KB）
    QByteArray testData;
    testData.resize(1024);
    for (int i = 0; i < 1024; i++) {testData[i] = i % 256;}

    qCDebug(uart) << "Starting loopback test (connect TX to RX for testing)...";
    qCDebug(uart) << "Sending" << testData.size() << "bytes of test data";
    m_testTimer.start();
    writeSerialData(testData);
}

