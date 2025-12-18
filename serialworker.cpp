#include "serialworker.h"

SerialWorker::SerialWorker(QObject *parent)
    : QObject(parent)
{
}

bool SerialWorker::initSerialPort(const QString &portName,
                                 qint32 baudRate,
                                 QSerialPort::DataBits dataBits,
                                 QSerialPort::Parity parity,
                                 QSerialPort::StopBits stopBits)
{
    QMutexLocker locker(&m_mutex);

    if (m_serialPort && m_serialPort->isOpen()) {
        m_serialPort->close();
        delete m_serialPort;
        m_serialPort = nullptr;
    }

    m_postName = portName;
    m_serialPort = new QSerialPort(this);
    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(dataBits);
    m_serialPort->setParity(parity);
    m_serialPort->setStopBits(stopBits);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);//设置为无流控（握手-简单应用）
    //m_serialPort->setFlowControl(QSerialPort::HardwareControl);// 硬件流控（可靠传输）
    //m_serialPort->setFlowControl(QSerialPort::SoftwareControl);// 软件流控（无专用线路）
    // 设置超时控制（避免阻塞）
    //m_serialPort->setReadBufferSize(1024 * 1024); // 1MB缓冲区
    /*// 清空缓冲区
        m_serialPort->clear();

        // 设置串口控制信号
        m_serialPort->setDataTerminalReady(true);    // DTR信号
        m_serialPort->setRequestToSend(true);        // RTS信号

        // 5. 配置底层描述符（可选，高级用法）
        #ifdef Q_OS_WIN
        // Windows特有配置
        #elif defined(Q_OS_LINUX)
        // Linux特有配置
        int fd = m_serialPort->handle();
        if (fd != -1) {
            // 设置最小读取字符数和超时
            struct termios tio;
            tcgetattr(fd, &tio);
            tio.c_cc[VMIN] = 0;      // 非阻塞模式
            tio.c_cc[VTIME] = 1;     // 0.1秒超时
            tcsetattr(fd, TCSANOW, &tio);
        }
        #endif*/

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
    }

    connect(m_serialPort, &QSerialPort::readyRead, this, &SerialWorker::handleReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &SerialWorker::handleError);
    //connect(m_serialPort, &QSerialPort::bytesWritten, this, &SerialWorker::handleBytesWritten);//速度无区别，不用缓冲区，甚至有开销

    QMetaObject::invokeMethod(this, [this]() {
        if (!m_serialPort->open(QIODevice::ReadWrite)) {
            qWarning() << "Failed to open serial port:"<< m_postName
                       << "Error:" << m_serialPort->errorString();
            emit serialErrorOccurred(QStringLiteral("Failed to open serial port %1: %2")
                                     .arg(m_postName, m_serialPort->errorString()));
            delete m_serialPort;
            m_serialPort = nullptr;

            return; //返回初始化失败
        }

        qDebug() << "Serial port" << m_postName << "opened successfully";
        m_isListening = true;//设置了监听标志，控制handleReadyRead处理数据
     }, Qt::QueuedConnection);

    return true;
}


void SerialWorker::handleReadyRead()
{
    QMutexLocker locker(&m_mutex);

    if (!m_isListening) return;

    // readAll()会读取串口缓冲区中的所有数据
    QByteArray data = m_serialPort->readAll();//可能会出现毡包
    if (!data.isEmpty()) {
        if (m_isTesting.load()) {
            m_bytesReceived += data.size();

            qDebug() << "Received" << data.size() << "bytes during test"
                     << "Total received:" << m_bytesReceived << "bytes";

            if (m_bytesReceived >= m_testData.size()) {
                qint64 elapsed = m_testTimer.elapsed();
                double speedKBps = (m_testData.size() * 1000.0) / (elapsed * 1024.0);
                double speedBps = m_testData.size() * 1000.0 / elapsed;

                QString result = QString(
                    "Loopback Test Result:"
                    "--------------------"
                    "Data size: %1 bytes"
                    "Time elapsed: %2 ms"
                    "Speed: %3 KB/s (%4 bps)"
                ).arg(m_testData.size())
                 .arg(elapsed)
                 .arg(speedKBps, 0, 'f', 2)
                 .arg(speedBps * 8, 0, 'f', 0);

                qDebug() << "\n" << result;

                m_isTesting.store(false);
            }else {
                // 打印接收到的数据（十六进制格式）
                //qDebug() << "Serial received:" << data.toHex(' ');
            }
        } else {
            // 正常模式
            emit serialDataReceived(data);
            // 打印接收到的数据（十六进制格式）
            qDebug() << "Serial received:" << data.toHex(' ');
        }
    }
    // 如果没有数据
}

void SerialWorker::handleError(QSerialPort::SerialPortError error)
{
    // 检查是否为非正常错误
    if (error != QSerialPort::NoError) {
        QString errorMsg = QString("Serial port error: %1").arg(m_serialPort->errorString());
        emit serialErrorOccurred(errorMsg);
        qWarning() << errorMsg;

        // 测试中遇到错误，停止测试
        if (m_isTesting.load()) {
            m_isTesting.store(false);
        }
    }
    // 如果是NoError
}


SerialWorker::~SerialWorker()
{
    closeSerial();
    qDebug() << "Serial~ delete finished";
}

void SerialWorker::closeSerial()
{
    QMutexLocker locker(&m_mutex);

    if (m_serialPort) {
        if (m_serialPort->isOpen()) {
            m_serialPort->close();
            qDebug() << "Serial port closed";
        }
        delete m_serialPort;
        m_serialPort = nullptr;
    }

    if (m_serialThread && m_serialThread->isRunning()) {
        m_serialThread->quit();
        m_serialThread->wait(1000);// 等待1秒
        m_serialThread->deleteLater();
        delete m_serialThread;
    }

    m_isListening = false;
    m_isTesting.store(false);
}


void SerialWorker::writeSerialData(const QByteArray &data)
{
    QMutexLocker locker(&m_mutex);

    if (!m_serialPort || !m_serialPort->isOpen()) {
        qWarning() << "Serial port is not open!";
        return;
    }

    // write()函数是异步的，写入缓冲区后立即返回
    qint64 bytesWritten = m_serialPort->write(data);
    if (bytesWritten == -1){
        // 写入失败
        qWarning() << "Failed to write data to serial port:" << m_serialPort->errorString();
    } else if (bytesWritten != data.size()) {
        // 部分写入
        qWarning() << "Partial data written to serial port:" << bytesWritten << "of" << data.size();
    } else {
        // 完全写入
        qDebug() << "Successfully wrote" << bytesWritten << "bytes to serial port";
    }
    // 注意：不需要调用推入flush()，QSerialPort会自动处理
}


void SerialWorker::startLoopbackTest()
{
    QMutexLocker locker(&m_mutex);

    if (m_isTesting.load()) {
        qWarning() << "Test is already running!";
        return;
    }

    if (!m_serialPort || !m_serialPort->isOpen()) {
        emit serialErrorOccurred("Serial port is not open!");
        return;
    }

    m_isTesting.store(true);
    m_bytesReceived = 0;

    // 生成测试数据（1KB）
    m_testData.resize(1024);
    for (int i = 0; i < 1024; i++) {
        m_testData[i] = i % 256;  // 填充0-255的序列
    }

    qDebug() << "Starting loopback test (connect TX to RX for testing)...";
    qDebug() << "Sending" << m_testData.size() << "bytes of test data";
    locker.unlock();  // 显式解锁
    m_testTimer.start();

    writeSerialData(m_testData);
    qDebug() << "Sending success!";
}

/**********************************************************************
 * 串口模块
#include "serialworker.h"

void SerialManager(const QString &portName)
{
    SerialWorker *serialWorker = new SerialWorker();
    QThread *serialThread = new QThread();
    serialWorker->moveToThread(serialThread);

    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                     serialWorker, &SerialWorker::closeSerial);
    QObject::connect(serialWorker, &SerialWorker::finished,
                     serialThread, &QThread::quit);
    QObject::connect(serialThread, &QThread::finished,
                     serialWorker, &QObject::deleteLater);
    QObject::connect(serialThread, &QThread::finished,
                     serialThread, &QObject::deleteLater);

    QObject::connect(serialWorker, &SerialWorker::serialDataReceived,
                     [](const QByteArray &data) {
                         qDebug() << "[Normal Data]" << data.size() << "bytes";
                     });
    QObject::connect(serialWorker, &SerialWorker::serialErrorOccurred,
                     [](const QString &error) {
                         qWarning() << "[Error]" << error;
                     });
    serialThread->setObjectName(QString("%1_worker").arg(portName));
    serialThread->start();

    bool serialInitialized = false;
    QMetaObject::invokeMethod(serialWorker, [serialWorker, &serialInitialized, &portName]() {
        serialInitialized = serialWorker->initSerialPort(portName, QSerialPort::Baud115200);
    }, Qt::BlockingQueuedConnection);

    if (serialInitialized) {
        //挂起 一秒后执行---在lambda中使用局部变量，但生命周期问题singleShot(1000, [](),所以使用捕获singleShot(1000, [serialWorker]()
        QTimer::singleShot(1000, serialWorker,[serialWorker]() {
            qDebug() << "\n=== Starting Loopback Test ===";
            qDebug() << "Please connect TX and RX pins of the serial port!";

            QMetaObject::invokeMethod(serialWorker, &SerialWorker::startLoopbackTest);
        });

        // 3秒后自动退出
        QTimer::singleShot(3000, QCoreApplication::instance(), &QCoreApplication::quit);

    } else {
        qCritical() << "✗ Failed to open any serial port!";

        QStringList portsToTry = {"/dev/ttyS9", "/dev/ttyS8", "/dev/ttyS7", "/dev/ttyS5", "/dev/ttyS5"};
        bool initialized = false;

        for (const QString &port : portsToTry) {
            qDebug() << "Trying to open" << port << "...";

            QMetaObject::invokeMethod(serialWorker, [serialWorker, port, &initialized]() {
                initialized = serialWorker->initSerialPort(port, QSerialPort::Baud115200);
            }, Qt::BlockingQueuedConnection);

            if (initialized) {
                qDebug() << "✓ Successfully opened" << port;
                break;
            } else {
                qDebug() << "✗ Failed to open" << port;
            }
        }

        QTimer::singleShot(1000, QCoreApplication::instance(), &QCoreApplication::quit);
    }
}


SerialManager("/dev/ttyS4");
 *
 *********************************************************************/
