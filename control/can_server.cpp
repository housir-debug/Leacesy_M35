#include "can_server.h"
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <QtCore>
#include <net/if.h>
#include <unistd.h>
#include <fcntl.h>

// ========================== 初始化部分 ===================================

Q_LOGGING_CATEGORY(can, "CAN:");

CanServerManager::CanServerManager(QObject *parent): QObject(parent){}
CanServerManager::~CanServerManager()
{
   if (m_serverThread && m_readNotifier && m_writeNotifier) {
       qCDebug(can)<<"[~CanServerManager]CAN~ delete finished";
       delete m_readNotifier;
       m_readNotifier = nullptr;
       delete m_writeNotifier;
       m_writeNotifier = nullptr;
       shutdown(m_socketFd, SHUT_RDWR);
       close(m_socketFd);

       m_serverThread->quit();
       m_serverThread->wait(1000); // wait 1s
       m_serverThread->deleteLater();
       delete m_serverThread;
       m_serverThread = nullptr;
   }
}

void CanServerManager::sendFrame(quint32 canId, const QByteArray &data)
{
    if (data.size() <= 8) {
        struct can_frame frame{};
        frame.can_id = canId;
        frame.can_dlc = static_cast<quint8>(data.size());
        memcpy(frame.data, data.constData(), data.size());

        // Queue rate limiting (to prevent infinite growth)
        if (m_sendQueue.size() <= 9999) {
            if (m_sendQueue.isEmpty()) {
                m_writeNotifier->setEnabled(true);
            }

            qCDebug(can)<<"[sendFrame]:Sent ID: "<<frame.can_id<<", Data: "<<data.toHex()<<", QueueRemain: "<<m_sendQueue.size()-1;
            m_sendQueue.enqueue(frame);
            return;
        }

        qCWarning(can)<<"[sendFrame]:Send queue overflow, size: "<<m_sendQueue.size();
        return;
    }

    qCWarning(can)<<"[sendFrame]:CAN data length cannot exceed 8 bytes";
}

bool CanServerManager::startServer(const QString &interface, quint32 bitrate)
{
    if (!m_serverThread && !m_readNotifier && !m_writeNotifier){
        QStringList commands;
        commands << QString("ip link set %1 down").arg(interface);
        commands << QString("ip link set %1 type can bitrate %2").arg(interface).arg(bitrate);
        commands << QString("ip link set %1 type can restart-ms 18").arg(interface);
        commands << QString("ip link set %1 txqueuelen 1000").arg(interface);
        commands << QString("ip link set %1 type can loopback on").arg(interface);
        commands << QString("ip link set %1 up").arg(interface);

        for (const QString &cmd : qAsConst(commands)) {
            if (system(cmd.toUtf8().constData()) != 0) {
                qCWarning(can)<<"[startServer]:Command failed: "<<cmd;
                return false;
            }
        }

        if (createSocket(interface)) {
            m_serverThread = new QThread(this);
            m_serverThread->setObjectName("CanServer");

            this->moveToThread(m_serverThread);
            m_readNotifier->moveToThread(m_serverThread);
            m_writeNotifier->moveToThread(m_serverThread);
            m_serverThread->start();

            connect(m_readNotifier, &QSocketNotifier::activated,this, [this](){
                // Prevent re-entry
                m_readNotifier->setEnabled(false);
                struct can_frame frame;

                while (true) {
                    ssize_t nbytes = read(m_socketFd, &frame, sizeof(frame));

                    if (nbytes == sizeof(frame)) { // always 16Bytes
                        QByteArray data(reinterpret_cast<const char*>(frame.data), frame.can_dlc);
                        qCDebug(can)<<"[startServer]:Received ID: "<<frame.can_id<<", Data: "<<data.toHex();
                        processFrame(frame.can_id,data);
                    } else if (nbytes < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // Data processing completed
                            m_readNotifier->setEnabled(true);
                            return;
                        }

                        qCWarning(can)<<"[startServer]:Read error: "<<strerror(errno);
                    }}}, Qt::DirectConnection);
            connect(m_writeNotifier, &QSocketNotifier::activated,this, [this](){
                // Prevent re-entry
                m_writeNotifier->setEnabled(false);

                while (!m_sendQueue.isEmpty()) {
                    const struct can_frame &frame = m_sendQueue.head();
                    ssize_t sent = write(m_socketFd, &frame, sizeof(frame));

                    if (sent == sizeof(frame)) {
                        m_sendQueue.dequeue();
                    } else if (sent < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // sending buffer is full. Please try again later.
                            m_writeNotifier->setEnabled(true);
                            return;
                        }

                        qCWarning(can)<<"[startServer]:Write error: "<<strerror(errno);
                    }}}, Qt::DirectConnection);

            QMetaObject::invokeMethod(this, [this]() {
                testLoopback();
            }, Qt::QueuedConnection);
            return true;
        }
    }

    return false;
}

bool CanServerManager::createSocket(const QString &interface)
{
    m_socketFd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (m_socketFd < 0) {
        qCWarning(can)<<"[createSocket]:Failed to create CAN socket: "<<strerror(errno);
        return false;
    }

    struct ifreq ifr;
    strncpy(ifr.ifr_name, interface.toUtf8().constData(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    if (ioctl(m_socketFd, SIOCGIFINDEX, &ifr) < 0) {
        qCWarning(can)<<"[createSocket]:Failed to get interface index: "<<strerror(errno);
        close(m_socketFd);
        m_socketFd = -1;
        return false;
    }

    struct sockaddr_can addr{};
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(m_socketFd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        qCWarning(can)<<"[createSocket]:Failed to bind socket: "<<strerror(errno);
        close(m_socketFd);
        m_socketFd = -1;
        return false;
    }

    // Set non-blocking mode
    int flags = fcntl(m_socketFd, F_GETFL, 0);
    if (fcntl(m_socketFd, F_SETFL, flags | O_NONBLOCK) < 0) {
        qCWarning(can)<<"[createSocket]:Failed to set non-blocking mode: "<<strerror(errno);
        close(m_socketFd);
        m_socketFd = -1;
        return false;
    }

    int sndbuf = 1024 * 1024;  // send buffer -> 1MB = 62500frame
    setsockopt(m_socketFd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));

    int rcvbuf = 1024 * 1024;  // receive buffer -> 1MB = 62500frame
    setsockopt(m_socketFd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));
    // The event loop's operation of handling other events also leads to timeout receive buffer overflow.

    int recv_own_msgs = 0;
    setsockopt(m_socketFd, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS,&recv_own_msgs, sizeof(recv_own_msgs));
    // {0|1} Return directly to the receiving buffer---Does not affect physical transmission

    m_readNotifier = new QSocketNotifier(m_socketFd, QSocketNotifier::Read, this);
    m_readNotifier->setEnabled(true);

    m_writeNotifier = new QSocketNotifier(m_socketFd, QSocketNotifier::Write, this);
    m_writeNotifier->setEnabled(false);
    // Initially disabled, enabled only when there is data.

    return true;
}

// ---------------------------------------------------------------------------------------------------------------

void CanServerManager::processFrame(quint32 canId,const QByteArray &data)
{
    Q_UNUSED(canId);Q_UNUSED(data);
    if (m_testing.load()) {
        m_receivedCount++;

        if (m_receivedCount >= 10000) {
            qint64 elapsed = m_testtimer.elapsed();
            double frameRate = (10000 * 1000.0) / elapsed;

            qCDebug(can) << QString(
                "[processFrame]:CAN Loopback Test Result: "
                "Test duration: %1 ms  "
                "Frame rate: %2 fps  "
            ).arg(elapsed).arg(frameRate, 0, 'f', 2);

            m_testing.store(false);
        }

        return;
    }
}

void CanServerManager::testLoopback()
{
    if (!m_testing.load()) {
        int count = 10000;
        quint32 testId = 0x123;
        QByteArray data = QByteArray::fromHex("1122334455667788");

        qCDebug(can) << QString("[testLoopback]:Start test ID: 0x%1, total: %2").arg(testId, 0, 16).arg(count);
        m_testing.store(true);
        m_testtimer.start();

        for (int i = 0; i < count; i++) {
           sendFrame(testId, data);
        }
    }
}

