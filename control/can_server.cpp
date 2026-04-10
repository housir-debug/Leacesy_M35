#include "can_server.h"
#include <QtCore>

// Linux SocketCAN
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <fcntl.h>
//#include <linux/can.h>
//#include <sys/socket.h>
//#include <cstring>

// ========================== 初始化部分 ===================================

Q_LOGGING_CATEGORY(can, "CAN:");

CanServerManager::CanServerManager(QObject *parent): QObject(parent){}
CanServerManager::~CanServerManager()
{
    qCDebug(can) << "CAN~ delete finished";
    m_stopListen.store(true);
    m_testing.store(false);
   if (m_listenThread) {
       m_listenThread->quit();
       m_listenThread->wait(1000); // 等待1秒
       m_listenThread->deleteLater();
       delete m_listenThread;
       m_listenThread = nullptr;// 内存释放 + 指针安全
   }

   for (auto it = m_fdToInterface.keyBegin(); it != m_fdToInterface.keyEnd(); ++it) {
       int fd = *it;
       if (fd >= 0) {
           shutdown(fd, SHUT_RDWR);
           close(fd);
       }
   }

    qDeleteAll(m_writeNotifiers);
}

bool CanServerManager::initialize(const QString &interface, int bitrate)
{
    QStringList canInterfaces = (interface == "all") ? QStringList{"can0", "can1", "can2"} : QStringList{interface};

    QStringList commands;
    for (const QString &iface : qAsConst(canInterfaces)) {
        commands << QString("ip link set %1 down").arg(iface);
        commands << QString("ip link set %1 type can bitrate %2").arg(iface).arg(bitrate);
        commands << QString("ip link set %1 type can restart-ms 18").arg(iface);
        commands << QString("ip link set %1 txqueuelen 1000").arg(iface);
        commands << QString("ip link set %1 type can loopback on").arg(iface);
        commands << QString("ip link set %1 up").arg(iface);

        qCDebug(can) << "Configuring CAN interface:";
        for (const QString &cmd : qAsConst(commands)) {
            qCDebug(can) << cmd;
            if (system(cmd.toUtf8().constData()) != 0) {
                return false;
            }
        }
        commands.clear();

        if (!initializeSocket(iface)) {return false;}
    }

    if (!m_listenThread) {
        m_listenThread = QThread::create([this]() {this->listenLoop();});
        m_listenThread->setObjectName(QString("%1_Listener").arg(interface));
        m_listenThread->start();
    }

    return true;
}

bool CanServerManager::initializeSocket(const QString &interfaceName)
{
    int socketFd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socketFd < 0) {
        qCCritical(can) << "Create CAN socket for" << interfaceName << "failed:" << strerror(errno);
        return false;
    }

    struct ifreq ifr;
    strncpy(ifr.ifr_name, interfaceName.toUtf8().constData(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    if (ioctl(socketFd, SIOCGIFINDEX, &ifr) < 0) {
        qCCritical(can) << "Cannot get interface:" << strerror(errno);
        close(socketFd);
        return false;
    }

    struct sockaddr_can addr{};
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(socketFd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        qCCritical(can) << "Error: Bind CAN interface failed:" << strerror(errno);
        close(socketFd);
        return false;
    }

    // Set non-blocking mode
    int flags = fcntl(socketFd, F_GETFL, 0);
    fcntl(socketFd, F_SETFL, flags | O_NONBLOCK);

    int recv_own_msgs = 0; // {0|1} Return directly to the receiving buffer---Does not affect physical transmission
    setsockopt(socketFd, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &recv_own_msgs, sizeof(recv_own_msgs));

    auto* notifier = new QSocketNotifier(socketFd,QSocketNotifier::Write,this);
    notifier->setEnabled(false);
    connect(notifier, &QSocketNotifier::activated,this, &CanServerManager::handleWriteReady);

    m_writeNotifiers[interfaceName] = notifier;
    m_fdToInterface[socketFd] = interfaceName;

    return true;
}

// ========================== 监听部分 ===================================

void CanServerManager::listenLoop()
{
    int epoll_fd = epoll_create1(EPOLL_CLOEXEC); // close-on-exec mark，Automatic shutdown with progress exec
    if (epoll_fd < 0) {
        qCWarning(can)<<"Epoll create failed: "<<strerror(errno);
        return;
    }

    for (auto it = m_fdToInterface.keyBegin(); it != m_fdToInterface.keyEnd(); ++it) {
        int fd = *it;
        struct epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
            qCWarning(can) << "Epoll add failed for: " << strerror(errno);
            close(epoll_fd);
            return;
        }
    }

    int eventcounts = 10;
    struct epoll_event events[eventcounts]; // At most, 10 events can be processed at a time.
    struct can_frame frame;
    //m_responsebuffer.reserve(8);

    while (!m_stopListen.load()) {
        int nfds = epoll_wait(epoll_fd, events, eventcounts, 1000); // time-out 1000ms Check if loop should continue | -1 = Blocking
        if (nfds < 0) {
            if (errno == EINTR) {continue;}
            qCWarning(can)<<"Epoll wait error:"<<strerror(errno);
            break;
        }

        for (int i = 0; i < nfds; i++) {
            while (true) {
                int nbytes = read(events[i].data.fd, &frame, sizeof(frame));
                if (nbytes > 0) {
                    if (nbytes == sizeof(frame)) { // always 16Bytes
                        m_responsebuffer = QByteArray::fromRawData(reinterpret_cast<const char*>(frame.data), frame.can_dlc); // Obtain valid bytes
                        auto it = m_fdToInterface.find(events[i].data.fd);
                        if (it == m_fdToInterface.end()) {
                            qCCritical(can) << "Unknown socket in handleWriteReady:" << events[i].data.fd;
                            return;
                        }
                        const QString& interface = *it;
                        listenProcessing(frame.can_id,interface);
                    }
                }else{
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {break;}
                    qCWarning(can)<<"Read error:"<<strerror(errno);
                    m_stopListen.store(true);
                    break;
                }
            }
        }
    }

    close(epoll_fd);
    qCDebug(can) << "CAN listener thread stopped";
    QThread::currentThread()->quit();
}

void CanServerManager::listenProcessing(quint32 canId,const QString &canface)
{
    if (m_testing.load()) {
        m_receivedCount++;
        qCDebug(can)  << "Test mode - Received frame " << m_receivedCount<< ": ID: 0x" << QString::number(canId, 16).toUpper()
                      << ", data: " << QString(m_responsebuffer.toHex(' ').toUpper())<< ", face: " << canface;

        if (m_receivedCount >= 10000) {
            qint64 elapsed = m_testtimer.elapsed();
            double frameRate = (10000 * 1000.0) / elapsed;

            qCDebug(can) << QString(
                "CAN Loopback Test Result: "
                "Test duration: %1 ms  "
                "Frame rate: %2 fps  "
            ).arg(elapsed).arg(frameRate, 0, 'f', 2);

            m_testing.store(false);
        }
        return;
    }

    qCDebug(can) << QString("Normal mode - Received: ID: 0x%1, Data: %2, Total received: %3 frames")
                .arg(canId, 0, 16).arg(QString(m_responsebuffer.toHex(' ').toUpper())).arg(m_receivedCount);

}

// ========================== 发送部分 ===================================

bool CanServerManager::sendFrame(quint32 canId, const QByteArray &data,const QString &canface)
{
    if (data.size() > 8) {
        qCWarning(can) << "CAN data length cannot exceed 8 bytes";
        return false;
    }

    struct can_frame frame{};
    frame.can_id = canId;
    frame.can_dlc = static_cast<quint8>(data.size());
    memcpy(frame.data, data.constData(), data.size());

    if (m_sendQueues[canface].size() <= 10000) {
        m_sendQueues[canface].enqueue(frame);

        if (auto* notifier = m_writeNotifiers.value(canface)) {
            notifier->setEnabled(true);
            return true;
        }
    }

    return false;
}

void CanServerManager::handleWriteReady(int socketFd)
{
    auto it = m_fdToInterface.find(socketFd);
    if (it == m_fdToInterface.end()) {
        qCCritical(can) << "Unknown socket in handleWriteReady:" << socketFd;
        return;
    }
    const QString& interface = *it;

    auto& queue = m_sendQueues[interface];
    while (!queue.isEmpty()) {
       const auto& frame = queue.head();
       ssize_t bytesSent = ::write(socketFd, &frame, sizeof(struct can_frame));

       if (bytesSent == sizeof(frame)) {
           qCDebug(can) << "CAN sent - ID:" << QString("CAN ID: 0x%1").arg(frame.can_id, 0, 16)
                        << "Data:" << QByteArray(reinterpret_cast<const char*>(&frame), sizeof(frame)).toHex(' ').toUpper()
                           //QByteArray(reinterpret_cast<const char*>(frame.data), frame.can_dlc).toHex(' ').toUpper()
                        << "face:" << interface
                        << "queue:" << queue.size();
           queue.dequeue(); // send success, Leave the queue
       }else {
           if (errno == EAGAIN || errno == EWOULDBLOCK) {break;}
           qCCritical(can) << "Failed to send CAN frame on" << interface << ":" << strerror(errno);
           // queue.clear();
           break; // retry
       }
   }

   if (queue.isEmpty()) {
       if (auto* notifier = m_writeNotifiers.value(interface)) {
           notifier->setEnabled(false);
       }
   }
}

void CanServerManager::testLoopback()
{
    if (m_testing.load()) {return;}

    quint32 testId = 0x123;
    QByteArray data = QByteArray::fromHex("1122334455667788");
    int count = 10000;

    m_testing.store(true);
    qCDebug(can) << QString("开始回环测试...ID: 0x%1, 总帧数: %2").arg(testId, 0, 16).arg(count);
    m_testtimer.start();

    for (int i = 0; i < count; i++) {
       if (!sendFrame(testId, data,"can0")) {
           qCDebug(can) << QString("第%1帧: 发送失败").arg(i+1);return;
       }
    }
}

