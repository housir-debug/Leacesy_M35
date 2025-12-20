#include "canworker.h"

CanWorker::CanWorker(QObject *parent)
    : QObject(parent)
{
}

bool CanWorker::initialize(const QString &interfaceName, int bitrate)
{
    QMutexLocker locker(&m_Mutex);

    if (m_canSocket >= 0) {
        qWarning() << "CAN interface already opened";
        return true;
    }

    m_interfaceName = interfaceName;
    m_bitrate = bitrate;

    // 1. 配置CAN接口
    if (!configureInterface(bitrate)) {
        emit errorOccurred(QString("Failed to configure CAN interface %1").arg(interfaceName));
        return false;
    }

    // 2. 初始化Socket
    if (!initializeSocket()) {
        emit errorOccurred(QString("Failed to initialize CAN socket for %1").arg(interfaceName));
        return false;
    }

    qDebug() << "CAN interface" << interfaceName << "initialized with bitrate" << bitrate;
    qDebug() << m_interfaceName << "opened successfully";

    QObject::connect(this, &CanWorker::frameReceived,this, &CanWorker::listenProcessing);
    startListening();

    //sleep(1);
    //testserialloop();
    /*QTimer::singleShot(16000,this,[this]() {
        QMetaObject::invokeMethod(this, &CanWorker::testserialloop);
    });*/
    return true;
}

bool CanWorker::configureInterface(int bitrate)
{
    QStringList commands;

    commands << QString("ip link set %1 down").arg(m_interfaceName);
    commands << QString("ip link set %1 type can bitrate %2").arg(m_interfaceName,QString::number(bitrate));
    commands << QString("ip link set %1 type can restart-ms 18").arg(m_interfaceName);
    commands << QString("ip link set %1 txqueuelen 1000").arg(m_interfaceName);
    commands << QString("ip link set %1 type can loopback on").arg(m_interfaceName);
    commands << QString("ip link set %1 up").arg(m_interfaceName);

    qDebug() << "Configuring CAN interface:";
    for (const QString &cmd : qAsConst(commands)) {
        qDebug() << cmd;
        int ret = system(cmd.toUtf8().constData());
        if (ret != 0) {
           qWarning() << "Command failed:" << cmd;
           return false;
        }
    }

    QThread::msleep(10);
    return true;
}

bool CanWorker::initializeSocket()
{
    m_canSocket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (m_canSocket < 0) {
        qCritical()<< "Create CAN socket failed:" << strerror(errno);
        return false;
    }

    // 获取接口索引
    struct ifreq ifr;
    strncpy(ifr.ifr_name, m_interfaceName.toUtf8().constData(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    if (ioctl(m_canSocket, SIOCGIFINDEX, &ifr) < 0) {
        qCritical() << "Cannot get interface:" << strerror(errno);
        close(m_canSocket);
        m_canSocket = -1;
        return false;
    }

    // 绑定套接字到CAN接口
    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(m_canSocket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        qCritical() << "Error: Bind CAN interface failed:" << strerror(errno);
        close(m_canSocket);
        m_canSocket = -1;
        return false;
    }

    // 设置非阻塞模式
    int flags = fcntl(m_canSocket, F_GETFL, 0);
    fcntl(m_canSocket, F_SETFL, flags | O_NONBLOCK);

    // 是否开启回环
    int recv_own_msgs = 0; // 0表示不接收自己发送的帧--默认
    //int recv_own_msgs = 1; //在内核socket中直接环回到接收缓冲区--不阻塞物理发送
    setsockopt(m_canSocket, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &recv_own_msgs, sizeof(recv_own_msgs));

    return true;
}

void CanWorker::startListening()
{
    if (m_listening.load()) {
        qWarning() << "Already listening";
        return;
    }

    if (m_canSocket < 0) {
        emit errorOccurred("CAN not initialized");
        qWarning() << "CAN not initialized";
        return;
    }

    m_listenThread = QThread::create([this]() {
            this->listenLoop();
        });

    m_stopRequested.store(false);
    m_listening.store(true);

    m_listenThread->setObjectName(QString("%1_Listener").arg(m_interfaceName));
    m_listenThread->start();
}


void CanWorker::listenLoop()
{
    // 这个函数在专用监听线程中运行
    // 注意：不能直接访问成员变量，需要加锁或使用原子变量
    qDebug() << "Entering listenLoop in thread:" << QThread::currentThread();

    int canSocket = -1;
    {
        QMutexLocker locker(&m_Mutex);
        canSocket = m_canSocket;
    }
    if (canSocket < 0) {
            return;
    }

    // epoll是Linux特有的高效I/O多路复用机制，用于监控多个文件描述符的状态
    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);//设置close-on-exec标志，进程exec时自动关闭
    if (epoll_fd < 0) {
        QString error = QString("Epoll create failed: %1").arg(strerror(errno));
        emit errorOccurred(error);
        return;
    }

    // 添加CAN套接字到epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    /*- EPOLLIN:  文件描述符可读
      - EPOLLOUT: 文件描述符可写
      - EPOLLERR: 发生错误
      - EPOLLHUP: 挂起（对端关闭）
      - EPOLLET:  边缘触发模式（默认是水平触发）*/
    ev.data.fd = canSocket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, canSocket, &ev) < 0) { // - EPOLL_CTL_ADD: 添加监控
        QString error = QString("Epoll add failed: %1").arg(strerror(errno));
        emit errorOccurred(error);
        close(epoll_fd);
        return;
    }

    struct epoll_event events[10];// 数组大小10表示一次最多处理10个就绪事件

    while (!m_stopRequested.load()) {
        // - 10: 最多返回的事件数量（数组大小）  - 100: 超时时间(毫秒)，100ms后即使没有事件也返回
        int nfds = epoll_wait(epoll_fd, events, 10, 100);
        if (nfds < 0) {
            if (errno == EINTR) {
                continue; // 被信号中断，继续
            }
            QString error = QString("Epoll wait error: %1").arg(strerror(errno));
            emit errorOccurred(error);
            break;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == canSocket) {
                // 因为epoll是水平触发（默认），只要缓冲区有数据就会一直报告可读
                while (true) {
                    struct can_frame frame;
                    int nbytes = read(canSocket, &frame, sizeof(frame));

                    if (nbytes <= 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break; // 没有更多数据 ← 跳出内层while->进入epoll内核态等待
                        }
                        QString error = QString("Read error: %1").arg(strerror(errno));
                        emit errorOccurred(error);
                        m_stopRequested.store(true);
                        break;
                    }

                    if (nbytes == sizeof(frame)) {
                        QByteArray data(reinterpret_cast<const char*>(frame.data), frame.can_dlc);
                        qint64 timestamp = QDateTime::currentMSecsSinceEpoch();

                        /*qDebug() << QString("CAN recv - ID: 0x%1, Len: %2, Data: %3,time: %4")
                                    .arg(frame.can_id, 3, 16, QChar('0'))
                                    .arg(frame.can_dlc)
                                    .arg(QString(data.toHex(' ').toUpper()))
                                    .arg(timestamp);*/

                        emit frameReceived(frame.can_id, data, timestamp);
                    }
                }
            }
            // 如果有多个socket被监控，这里会有其他else if分支
        }
    }

    close(epoll_fd);
    qDebug() << "CAN listener thread stopped";
    QThread::currentThread()->quit();
}

void CanWorker::listenProcessing(quint32 canId, const QByteArray &data, qint64 timestamp)
{
    if (!m_listening.load()) {return;}

    m_receivedCount++;
    if (m_testing.load()) {
        qDebug() << QString("Test mode - Received frame %1: ID: 0x%2, Len: %3, data: %7, Total received: %5 frames, Time: %6)")
                    .arg(m_receivedCount)
                    .arg(canId, 0, 16)
                    .arg(data.size())
                    .arg(m_receivedCount)
                    .arg(timestamp)
                    .arg(QString(data.toHex(' ').toUpper()));

        // 检查测试是否完成（比较测试函数的设定的数据量count的设定）
        if (m_receivedCount >= 100 ) {
            qint64 elapsed = m_testtimer.elapsed();

            if (elapsed > 0) {
                double frameRate = (m_receivedCount * 1000.0) / elapsed;

                QString result = QString(
                    "================"
                    "CAN Loopback Test Result: "
                    "Test duration: %1 ms  "
                    "Frames sent: %2  "
                    "Frames received: %3  "
                    "Frame rate: %5 fps  "
                    "Packet loss: %8%  "
                    "================"
                ).arg(elapsed)
                 .arg(m_sentCount)
                 .arg(m_receivedCount)
                 .arg(frameRate, 0, 'f', 2)
                 .arg(100.0 * (m_sentCount -m_receivedCount) / m_sentCount, 0, 'f', 2);

                qDebug() << result;
            }
            m_testing.store(false);
        }

        //qint64 elapsed = m_testtimer.elapsed();
        //qDebug() << "serial-can test time:" <<elapsed;

    } else {
        qDebug() << QString("Normal mode - Received: ID: 0x%1, Data: %2, Total received: %3 frames, Time: %4")
                    .arg(canId, 0, 16)
                    .arg(QString(data.toHex(' ').toUpper()))
                    .arg(m_receivedCount)
                    .arg(timestamp);
    }
}


void CanWorker::testLoopback()
{
    quint32 testId = 0x123;
    const QByteArray &testData = QByteArray::fromHex("1122334455667788");
    int count = 100;
    int intervalMs = 0;

    if (m_canSocket < 0) {return;}

    QByteArray data = testData;
    if (data.size() > 8) {
        data = data.left(8); //取前八位
    }

    if (!m_testing.load()){
        m_testing.store(true);
        qDebug() << QString("开始回环测试...ID: 0x%1, 数据: %2, 次数: %3, 间隔: %4ms")
                    .arg(testId, 0, 16)
                    .arg(QString(data.toHex()))
                    .arg(count)
                    .arg(intervalMs);
    }else{
        qCritical() << "Testing has now begun" ;
    }

    m_testtimer.start();

    for (int i = 0; i < count; i++) {
       // 将 i 的最低字节存入 data 数组的最后一个位置
       if (data.size() > 0) {
           data[data.size() - 1] = static_cast<char>(i & 0xFF);
       }

       if (!sendFrame(testId, data)) {
           qDebug() << QString("  第%1帧: 发送失败").arg(i+1);
       }
    }
}

void CanWorker::testserialloop(){
    //m_testing.store(true);
    m_testtimer.start();
    const QByteArray &testData = QByteArray::fromHex("1122334455667788");
    emit SerialSendRequest(testData);
}


CanWorker::~CanWorker()
{
    closeCan();
    qDebug() << "CAN~ delete finished";
}

void CanWorker::closeCan()
{
    QMutexLocker locker(&m_Mutex);

    if (m_listening.load()) {
        m_stopRequested.store(true);

       if (m_listenThread && m_listenThread->isRunning()) {
           m_listenThread->quit();
           m_listenThread->wait(1000); // 等待1秒
           m_listenThread->deleteLater();
           delete m_listenThread;
           m_listenThread = nullptr;// 内存释放 + 指针安全
           m_listening.store(false);
           qDebug() << "CAN listening closed";
       }
    }

    if (m_canSocket >= 0) {
        shutdown(m_canSocket, SHUT_RDWR);
        close(m_canSocket);
        m_canSocket = -1;
        qDebug() << "CAN socket closed";
    }

}


void CanWorker::forwardSerialData(const QByteArray &data)
{
    quint32 testId = 0x123;
    qint64 elapsed = m_testtimer.elapsed();
    qDebug() << "serial-can test time:" <<elapsed;
    sendFrame(testId, data);
}

bool CanWorker::sendFrame(quint32 canId, const QByteArray &data)
{
    QMutexLocker locker(&m_Mutex);

    if (m_canSocket < 0) {
        qWarning() << "CAN socket not open";
        emit errorOccurred("CAN socket not open");
        return false;
    }

    if (data.size() > 8) {
        qWarning() << "CAN data length cannot exceed 8 bytes";
        emit errorOccurred("Data too long");
        return false;
    }

    struct can_frame frame;
    memset(&frame, 0, sizeof(frame));

    frame.can_id = canId;
    frame.can_dlc = static_cast<quint8>(data.size());
    memcpy(frame.data, data.constData(), data.size());

    return sendFrame_en(frame);
}

bool CanWorker::sendFrame_en(const can_frame &frame)
{
    if (m_canSocket < 0) {
        qWarning() << "CAN socket not open";
        emit errorOccurred("CAN socket not open");
        return false;
    }

    int bytesSent = write(m_canSocket, &frame, sizeof(frame));
    if (bytesSent != sizeof(frame)) {
        QString error = QString("Send failed: %1").arg(strerror(errno));
        qCritical() << "Create CAN socket failed:" << error;
        emit errorOccurred(error);
        emit frameSented(frame.can_id, false);
        return false;
    }

    m_sentCount++;
    QByteArray dataArray(reinterpret_cast<const char*>(frame.data), frame.can_dlc);
    qDebug() << QString("CAN sent - ID: 0x%1, Len: %3, Data: %2, Total received: %4")
                .arg(frame.can_id, 3, 16, QChar('0'))
                .arg(QString(dataArray.toHex(' ').toUpper()))
                .arg(frame.can_dlc)
                .arg(m_sentCount);

    emit frameSented(frame.can_id, true);
    return true;
}


// ============================================================================
// 调用示例
/*
#include "canworker.h"

void canmanager(const QString &cansocket)
{
    CanWorker *canWorker = new CanWorker();
    QThread *canThread = new QThread();
    canWorker->moveToThread(canThread);

    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         canWorker, &CanWorker::closeCan);
    QObject::connect(canWorker, &CanWorker::canClosed,
                     canWorker, &QObject::deleteLater);

    QObject::connect(canWorker, &CanWorker::errorOccurred,
                     [](const QString &error) {
        qCritical() << "CAN Error:" << error;
    });
    canThread->setObjectName("can_worker");
    canThread->start();

    //canWorker->initialize("can0", 1000000);
    //canWorker->testLoopback();

    bool Initialized = false;
    QMetaObject::invokeMethod(canWorker, [canWorker, &Initialized, &cansocket]() {
        Initialized = canWorker->initialize(cansocket, 1000000);
    }, Qt::BlockingQueuedConnection);

    if (Initialized) {
         QMetaObject::invokeMethod(canWorker, &CanWorker::testLoopback);
         QTimer::singleShot(1000, QCoreApplication::instance(), &QCoreApplication::quit);
    }
}
*/
