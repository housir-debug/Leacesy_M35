#include "canworker.h"
#include <QtCore>

// Linux SocketCAN
#include <linux/can/raw.h>
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

CanWorker::CanWorker(QObject *parent): QObject(parent){}
CanWorker::~CanWorker()
{
    qCDebug(can) << "CAN~ delete finished";
    m_stopRequested.store(true);
    m_testing.store(false);
   if (m_listenThread) {
       m_listenThread->quit();
       m_listenThread->wait(1000); // 等待1秒
       m_listenThread->deleteLater();
       delete m_listenThread;
       m_listenThread = nullptr;// 内存释放 + 指针安全
   }

    if (m_can0Socket >= 0) {
        shutdown(m_can0Socket, SHUT_RDWR);
        close(m_can0Socket);
        m_can0Socket = -1;
    }
    if (m_can1Socket >= 0) {
        shutdown(m_can1Socket, SHUT_RDWR);
        close(m_can1Socket);
        m_can1Socket = -1;
    }
    if (m_can2Socket >= 0) {
        shutdown(m_can2Socket, SHUT_RDWR);
        close(m_can2Socket);
        m_can2Socket = -1;
    }
}

bool CanWorker::initialize(const QString &interface, int bitrate)
{
    if (m_can0Socket>=0||m_can1Socket>=0||m_can2Socket>=0) {return false;}

    if(interface == "all"){
        QStringList canInterfaces = {"can0", "can1", "can2"};
        QStringList commands;
        for (const QString &iface : canInterfaces) {
                commands << QString("ip link set %1 down").arg(iface);
                commands << QString("ip link set %1 type can bitrate %2").arg(iface).arg(bitrate);
                commands << QString("ip link set %1 type can restart-ms 18").arg(iface);
                commands << QString("ip link set %1 txqueuelen 1000").arg(iface);
                commands << QString("ip link set %1 type can loopback on").arg(iface);
                commands << QString("ip link set %1 up").arg(iface);
        }

        qCDebug(can) << "Configuring CAN interface:";
        for (const QString &cmd : qAsConst(commands)) {
            qCDebug(can) << cmd;
            int ret = system(cmd.toUtf8().constData());
            if (ret != 0) {return false;}
        }

        bool success= true;
        success= success && initializeSocket("can0",m_can0Socket);
        success= success && initializeSocket("can1",m_can1Socket);
        success= success && initializeSocket("can2",m_can2Socket);
        if (!success) {
            qCCritical(can) << "Failed to initialize all CAN sockets";
            return false;
        }
    }else{
        QStringList commands;
        commands << QString("ip link set %1 down").arg(interface);
        commands << QString("ip link set %1 type can bitrate %2").arg(interface,QString::number(bitrate));
        commands << QString("ip link set %1 type can restart-ms 18").arg(interface);
        commands << QString("ip link set %1 txqueuelen 1000").arg(interface);
        commands << QString("ip link set %1 type can loopback on").arg(interface);
        commands << QString("ip link set %1 up").arg(interface);

        qCDebug(can) << "Configuring CAN interface:";
        for (const QString &cmd : qAsConst(commands)) {
            qCDebug(can) << cmd;
            int ret = system(cmd.toUtf8().constData());
            if (ret != 0) {return false;}
        }

        bool success= false;
        if(interface == "can0"){success=initializeSocket(interface,m_can0Socket);}
        if(interface == "can1"){success=initializeSocket(interface,m_can1Socket);}
        if(interface == "can2"){success=initializeSocket(interface,m_can2Socket);}
        if (!success) {
            qCCritical(can) << "Failed to initialize all CAN sockets";
            return false;
        }
    }

    m_listenThread = QThread::create([this]() {this->listenLoop();});
    m_listenThread->setObjectName(QString("%1_Listener").arg(interface));
    m_listenThread->start();

    qCDebug(can) << "can socket opened successfully with bitrate" << bitrate;
    return true;
}

bool CanWorker::initializeSocket(const QString &interfaceName, int &socketFd)
{
    socketFd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
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
        socketFd = -1;
        return false;
    }

    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(socketFd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        qCCritical(can) << "Error: Bind CAN interface failed:" << strerror(errno);
        close(socketFd);
        socketFd = -1;
        return false;
    }

    // 设置非阻塞模式
    int flags = fcntl(socketFd, F_GETFL, 0);
    fcntl(socketFd, F_SETFL, flags | O_NONBLOCK);

    int recv_own_msgs = 0; // 0不接收 | 1; 在内核socket中直接环回到接收缓冲区--不阻塞物理发送
    setsockopt(socketFd, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &recv_own_msgs, sizeof(recv_own_msgs));

    return true;
}

// ========================== 监听部分 ===================================

void CanWorker::listenLoop()
{
    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);//close-on-exec mark，Automatic shutdown with progress exec
    if (epoll_fd < 0) {
        qCWarning(can)<<"Epoll create failed: "<<strerror(errno);
        return;
    }

    int can0Socket = -1;int can1Socket = -1;int can2Socket = -1;
    {
        if (m_can0Socket > 0) {can0Socket = m_can0Socket;}
        if (m_can1Socket > 0) {can1Socket = m_can1Socket;}
        if (m_can2Socket > 0) {can2Socket = m_can2Socket;}
    }

    if (can0Socket >0){
        struct epoll_event ev0;
        ev0.events = EPOLLIN;
        ev0.data.fd = can0Socket;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, can0Socket, &ev0) < 0) { // - EPOLL_CTL_ADD: 添加监控
            qCWarning(can)<<"Epoll add failed: "<<strerror(errno);
            close(epoll_fd);
            return;
        }
    }
    if (can1Socket >0){
        struct epoll_event ev1;
        ev1.events = EPOLLIN;
        ev1.data.fd = can1Socket;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, can1Socket, &ev1) < 0) { // - EPOLL_CTL_ADD: 添加监控
            qCWarning(can)<<"Epoll add failed: "<<strerror(errno);
            close(epoll_fd);
            return;
        }
    }
    if (can2Socket >0){
        struct epoll_event ev2;
        ev2.events = EPOLLIN;
        ev2.data.fd = can2Socket;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, can2Socket, &ev2) < 0) { // - EPOLL_CTL_ADD: 添加监控
            qCWarning(can)<<"Epoll add failed: "<<strerror(errno);
            close(epoll_fd);
            return;
        }
    }
    /*- EPOLLIN:  文件描述符可读
      - EPOLLOUT: 文件描述符可写
      - EPOLLERR: 发生错误
      - EPOLLHUP: 挂起（对端关闭）
      - EPOLLET:  边缘触发模式（默认是水平触发）*/

    struct epoll_event events[10];// 数组大小10表示一次最多处理10个就绪事件
    struct can_frame frame;

    while (!m_stopRequested.load()) {
        // - 10: 最多返回的事件数量（数组大小）  - 100: 超时时间(毫秒)，100ms后即使没有事件也返回
        int nfds = epoll_wait(epoll_fd, events, 10, 100);
        if (nfds < 0) {
            if (errno == EINTR) {continue;}
            qCWarning(can)<<"Epoll wait error:"<<strerror(errno);
            break;
        }

        for (int i = 0; i < nfds; i++) {
            // 因为epoll是水平触发（默认），只要缓冲区有数据就会一直报告可读
            while (true) {
                int nbytes = read(events[i].data.fd, &frame, sizeof(frame));
                if (nbytes <= 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {break;}
                    qCWarning(can)<<"Read error: %1"<<strerror(errno);
                    m_stopRequested.store(true);
                    break;
                }

                if (nbytes != sizeof(frame)) {qCWarning(can)<<"Occurred incomplete content: "<<frame.data<<"valid btyes: "<<frame.can_dlc;continue;}

                QByteArray data(reinterpret_cast<const char*>(frame.data), frame.can_dlc);
                if (events[i].data.fd == can0Socket) {listenProcessing(frame.can_id, data,"can0");}
                if (events[i].data.fd == can1Socket) {listenProcessing(frame.can_id, data,"can1");}
                if (events[i].data.fd == can2Socket) {listenProcessing(frame.can_id, data,"can2");}
            }
        }
    }

    close(epoll_fd);
    qCDebug(can) << "CAN listener thread stopped";
    QThread::currentThread()->quit();
}

void CanWorker::listenProcessing(quint32 canId, const QByteArray &data,const QString &canface)
{
    if (m_testing.load()) {
        m_receivedCount++;
        qCDebug(can)  << "Test mode - Received frame " << m_receivedCount<< ": ID: 0x" << QString::number(canId, 16).toUpper()
                      << ", Len: " << data.size()<< ", data: " << QString(data.toHex(' ').toUpper())<< ", face: " << canface;

        if (m_receivedCount >= 1000) {
            qint64 elapsed = m_testtimer.elapsed();
            double frameRate = (1000 * 1000.0) / elapsed;

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
                .arg(canId, 0, 16).arg(QString(data.toHex(' ').toUpper())).arg(m_receivedCount);

}

// ========================== 发送部分 ===================================

bool CanWorker::sendFrame(quint32 canId, const QByteArray &data,const QString &canface)
{
    if (data.size() > 8) {
        qCWarning(can) << "CAN data length cannot exceed 8 bytes";
        return false;
    }

    struct can_frame frame;
    memset(&frame, 0, sizeof(frame));

    frame.can_id = canId;
    frame.can_dlc = static_cast<quint8>(data.size());
    memcpy(frame.data, data.constData(), data.size());

    int bytesSent = 0;
    if (canface=="can0"){bytesSent=write(m_can0Socket, &frame, sizeof(frame));}
    if (canface=="can1"){bytesSent=write(m_can1Socket, &frame, sizeof(frame));}
    if (canface=="can2"){bytesSent=write(m_can2Socket, &frame, sizeof(frame));}

    usleep(18);

    if (bytesSent != sizeof(frame)) {
        qCCritical(can) << "Send failed: %1" <<strerror(errno);
        return false;
    }

    qCDebug(can) << QString("CAN sent - ID: %1 Data:%2 ").arg(canId, 0, 16).arg(QString(data.toHex(' ').toUpper()));
    return true;
}

void CanWorker::testLoopback()
{
    if (m_testing.load()) {return;}

    quint32 testId = 0x123;
    QByteArray data = QByteArray::fromHex("1122334455667788");
    int count = 1000;

    m_testing.store(true);
    qCDebug(can) << QString("开始回环测试...ID: 0x%1, 总帧数: %2").arg(testId, 0, 16).arg(count);

    m_testtimer.start();
    for (int i = 0; i < count; i++) {
       //data[data.size() - 1] = static_cast<char>(i & 0xFF);
       if (!sendFrame(testId, data,"can0")) {
           qCDebug(can) << QString("第%1帧: 发送失败").arg(i+1);return;
       }
    }
}
