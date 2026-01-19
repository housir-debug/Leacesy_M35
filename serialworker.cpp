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

    m_serialPort = new QSerialPort(this);
    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(dataBits);
    m_serialPort->setParity(parity);
    m_serialPort->setStopBits(stopBits);

    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);// 大多数情况使用
    // - HardwareControl: 需要硬件流控的设备NoFlowControl
    // - SoftwareControl: XON/XOFF软件流控
    m_serialPort->setReadBufferSize(1024 * 1024); // 1MB缓冲区
    m_portName = portName;

    if (!m_serialThread){
        m_serialThread = new QThread(this);
        m_serialThread->setObjectName(QString("%1_worker").arg(portName));
    }

    if (thread() != m_serialThread) {
        this->moveToThread(m_serialThread);
        m_serialPort->moveToThread(m_serialThread);
    }

    if (!m_serialThread->isRunning()) {
        m_serialThread->start();

        connect(m_serialPort, &QSerialPort::readyRead, this, &SerialWorker::handleReadyRead);
        connect(m_serialPort, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
            if (error == QSerialPort::NoError) {return;}
            qCWarning(uart) << QString("Serial port: %1 error: %2").arg(this->m_portName,m_serialPort->errorString());
        });

        QMetaObject::invokeMethod(this, [this]() {
            if (!m_serialPort->open(QIODevice::ReadWrite)) {
                qCWarning(uart) << "Failed open port:"<< m_portName << "Error:" << m_serialPort->errorString();
                delete m_serialPort;
                m_serialPort = nullptr;
            }
         }, Qt::QueuedConnection);
        return true;
    }
    return false;
}

// ========================== 信息处理部分 ===================================

void SerialWorker::handleReadyRead()
{
    QByteArray data = m_serialPort->readAll();

    if (data.isEmpty()){return;}
    qCDebug(uart) <<m_portName<<" Received" << data.toHex(' ');

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
        }
        return;
    }

    // 正常模式
    emit serialDataReceived(data);
}

void SerialWorker::writeSerialData(const QByteArray &data)
{
    QMutexLocker locker(&m_mutex);

    if (!m_serialPort || !m_serialPort->isOpen()) {
        qCWarning(uart) << "Serial port is not open!";
        return;
    }

    qint64 bytesWritten = m_serialPort->write(data);
    if (bytesWritten != data.size()) {qCWarning(uart) << m_portName << "Partial data written :" << bytesWritten << "of" << data.size();
    } else {qCDebug(uart) << "Successfully wrote" << bytesWritten << "bytes to" << m_portName;}
}

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

// ========================== 析构部分 ===================================

SerialWorker::~SerialWorker()
{
    qCDebug(uart) << "Serial~ delete finished ："<< m_portName;
    closeSerial();
}

void SerialWorker::closeSerial()
{
    QMutexLocker locker(&m_mutex);

    m_isTesting.store(false);
    if (m_serialPort) {
        if (m_serialPort->isOpen()) {m_serialPort->close();}
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
}


