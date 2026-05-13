#include "can_server.h"
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <QtCore>
#include <net/if.h>
#include <unistd.h>
#include <fcntl.h>

// ========================== 初始化部分 ===================================

Q_LOGGING_CATEGORY(can, "CAN:");

CanServerManager::CanServerManager(QObject *parent): QObject(parent)
{
    m_canToUart = {
        // output
        {0x036001, 0x0180},
        {0x106001, 0x01},
        {0x107006, 0x0108},
        {0x037006, 0x0188},
        {0x107007, 0x0109},
        {0x037007, 0x0189},
        // setting
        {0x036002, 0x0280},
        {0x016002, 0x0200},
        {0x036003, 0x0281},
        {0x106003, 0x0201},
        {0x036004, 0x0282},
        {0x106004, 0x0202},
        {0x036005, 0x0283},
        {0x106005, 0x0203},
        {0x036006, 0x0284},
        {0x106006, 0x0204},
        {0x036040, 0x0285},
        {0x106040, 0x0205},
        // control
        {0x036007, 0x0380},
        {0x106007, 0x0300},
        {0x036008, 0x0381},
        {0x106008, 0x0301},
        {0x036009, 0x0382},
        {0x106009, 0x0302},
        {0x03600A, 0x0383},
        {0x10600A, 0x0303},
        {0x03600B, 0x0384},
        {0x10600B, 0x0304},
        {0x03600C, 0x0385},
        {0x10600C, 0x0305},
        {0x036041, 0x0388},
        {0x106041, 0x0308},
        {0x036042, 0x0389},
        {0x106042, 0x0309},
        // measurement
        {0x03600D, 0x048F},
        {0x10600D, 0x040F},

        {0x03600E, 0x0480},
        {0x03600F, 0x0481},
        {0x036010, 0x0482},
        {0x036011, 0x0483},
        {0x036012, 0x0484},
        {0x036013, 0x0485},
        {0x036014, 0x0486},
        {0x036015, 0x0487},

        {0x036029, 0x048A},
        {0x03602A, 0x048B},
        {0x03602B, 0x048D},

        {0x03601E, 0x0489},

        {0x106030, 0x0410},
        {0x036031, 0x0491},
        {0x036032, 0x0492},
        {0x036033, 0x0493},
        {0x036034, 0x0494},
        {0x036035, 0x0495},
        {0x036036, 0x0496},
        {0x036037, 0x0497},
        {0x036038, 0x0498},
        {0x036039, 0x0499},
        {0x03603a, 0x049A},
        {0x03603b, 0x049B},
        {0x03603c, 0x049C},
        // register
        {0x036016, 0x0580},
        {0x036017, 0x0581},
        {0x036018, 0x0582},
        {0x036019, 0x0583},
        {0x10601A, 0x0503},
        {0x03601B, 0x0584},
        {0x03601C, 0x0585},
        // trigger
        {0x107000, 0x0800},
        {0x107001, 0x0801},
        {0x107002, 0x0802},
        {0x107003, 0x0803},
        {0x107004, 0x0804},
        {0x107005, 0x0805},
        {0x037005, 0x0885},
        {0x107011, 0x0806},
        {0x037011, 0x0886},
        {0x107021, 0x0807},
        {0x037021, 0x0887},
        {0x107031, 0x0808},
        {0x037031, 0x0888},
        {0x107041, 0x0809},
        {0x037041, 0x0889},
        {0x037050, 0x088A},
        {0x107050, 0x080A},
        {0x037051, 0x088B},
        {0x107051, 0x080B},
        {0x037052, 0x088C},
        {0x107052, 0x080C},
        // calibrate
        {0x106102, 0x0601},
        {0x106103, 0x0602},
        {0x106104, 0x0603},
        {0x036105, 0x0684},
        {0x106106, 0x0604},
        {0x106107, 0x0605},
        {0x106108, 0x0606},
        {0x1062, 0x07},
        // error
        {0xffffff,0xffff},
   };

    for (auto it = m_canToUart.begin(); it != m_canToUart.end(); ++it) {
        m_uartToCan[it.value()] = it.key();
    }
}
CanServerManager::~CanServerManager()
{
   if (m_readNotifier && m_writeNotifier) {
       delete m_readNotifier;
       m_readNotifier = nullptr;
       delete m_writeNotifier;
       m_writeNotifier = nullptr;
       shutdown(m_socketFd, SHUT_RDWR);
       close(m_socketFd);
   }

    if (m_serverThread) {
       m_serverThread->quit();
       m_serverThread->wait(1000); // wait 1s
       m_serverThread->deleteLater();
       delete m_serverThread;
       m_serverThread = nullptr;
   }

   qCDebug(can)<<"[~CanServerManager]CAN~ delete finished";
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

bool CanServerManager::startServer()
{
    if (!m_serverThread && !m_readNotifier && !m_writeNotifier){
        QString interface = "can1";
        QStringList commands;
        commands << QString("ip link set %1 down").arg(interface);
        commands << QString("ip link set %1 type can bitrate 1000000").arg(interface);
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
                        if (m_canid == frame.can_id){
                            QByteArray data(reinterpret_cast<const char*>(frame.data), frame.can_dlc);
                            qCDebug(can)<<"[startServer]:Received Data: "<<data.toHex();
                            if (m_qmlbridge->m_remoteStatus.load()==1){
                                processFrame(data);
                            }
                            else if (m_qmlbridge->m_remoteStatus.load()==0){
                                m_qmlbridge->update_remotemodel(1);
                                processFrame(data);
                            }
                            else{
                                quint8 channel = static_cast<quint8>(data[0]);
                                qCDebug(can)<<"[startServer]: Currently in an alternative remote mode";
                                sendFrame(channel,0xffff,"");
                            }
                        }
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

            return true;
        }
    }

    qCWarning(can)<<"[startServer]: already exist A certain member";
    return false;
}

// ---------------------------------------------------------------------------------------------------------------

void CanServerManager::processFrame(const QByteArray &data)
{
    quint8 channel = static_cast<quint8>(data[0]);
    quint16 val = (static_cast<quint32>(data[1]) << 8) |(static_cast<quint32>(data[2]));
    if (val != 0x1062){
        val = (static_cast<quint32>(data[1]) << 16) |(static_cast<quint32>(data[2]) << 8)
                |(static_cast<quint32>(data[3]));
    }

    quint16 cmf = m_canToUart.value(val);
    quint8 highByte = (cmf >> 8) & 0xFF;
    quint8 lowByte = cmf & 0xFF;

    QByteArray one = data.mid(7);
    QByteArray two = data.mid(4,2);
    QByteArray four = data.mid(4,4);

    switch (cmf){
    // output
        case 0x0180:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x01:{
            quint8 func = static_cast<quint8>(data[7]);
            to_Channel(channel,lowByte,func,"");
            return;}
        case 0x0108:
            to_Channel(channel,highByte,lowByte,one);
            return;
        case 0x0188:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0109:
            to_Channel(channel,highByte,lowByte,one);
            return;
        case 0x0189:
            to_Channel(channel,highByte,lowByte,"");
            return;
    // setting
        case 0x0280:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0200:
            to_Channel(channel,highByte,lowByte,four);
            return;
        case 0x0281:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0201:
            to_Channel(channel,highByte,lowByte,four);
            return;
        case 0x0282:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0202:
            to_Channel(channel,highByte,lowByte,four);
            return;
        case 0x0283:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0203:
            to_Channel(channel,highByte,lowByte,four);
            return;
        case 0x0284:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0204:
            to_Channel(channel,highByte,lowByte,four);
            return;
        case 0x0285:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0205:
            to_Channel(channel,highByte,lowByte,four);
            return;
    // control
        case 0x0380:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0300:
            to_Channel(channel,highByte,lowByte,one);
            return;
        case 0x0381:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0301:
            to_Channel(channel,highByte,lowByte,one);
            return;
        case 0x0382:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0302:
            to_Channel(channel,highByte,lowByte,one);
            return;
        case 0x0383:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0303:
            to_Channel(channel,highByte,lowByte,one);
            return;
        case 0x0384:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0304:
            to_Channel(channel,highByte,lowByte,one);
            return;
        case 0x0385:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0305:
            to_Channel(channel,highByte,lowByte,one);
            return;
        case 0x0388:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0308:
            to_Channel(channel,highByte,lowByte,one);
            return;
        case 0x0389:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0309:
            to_Channel(channel,highByte,lowByte,one);
            return;
    // measurement
        case 0x048F:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x040F:
            to_Channel(channel,highByte,lowByte,four);
            return;
        case 0x0480:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0481:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0482:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0483:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0484:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0485:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0486:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0487:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x048A:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x048B:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x048C:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0489:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0410:
            to_Channel(channel,highByte,lowByte,one);
            return;
        case 0x0491:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0492:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0493:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0494:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0495:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0496:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0497:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0498:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0499:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x049A:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x049B:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x049C:
            to_Channel(channel,highByte,lowByte,"");
            return;
    // register
        case 0x0580:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0581:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0582:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0583:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0503:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0584:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0585:
            to_Channel(channel,highByte,lowByte,"");
            return;
    // trigger
        case 0x0800:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0801:
            to_Channel(channel,highByte,lowByte,one);
            return;
        case 0x0802:
            to_Channel(channel,highByte,lowByte,one);
            return;
        case 0x0803:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0804:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0805:
            to_Channel(channel,highByte,lowByte,one);
            return;
        case 0x0885:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0806:
            to_Channel(channel,highByte,lowByte,two);
            return;
        case 0x0886:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0807:
            to_Channel(channel,highByte,lowByte,four);
            return;
        case 0x0887:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0808:
            to_Channel(channel,highByte,lowByte,four);
            return;
        case 0x0888:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0809:
            to_Channel(channel,highByte,lowByte,one);
            return;
        case 0x0889:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x088A:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x080A:
            to_Channel(channel,highByte,lowByte,four);
            return;
        case 0x088B:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x080B:
            to_Channel(channel,highByte,lowByte,four);
            return;
        case 0x088C:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x080C:
            to_Channel(channel,highByte,lowByte,four);
            return;
    // calibrate
        case 0x0601:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0602:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0603:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0684:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0604:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0605:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x0606:
            to_Channel(channel,highByte,lowByte,"");
            return;
        case 0x07:{
            m_calibrate_step = static_cast<quint8>(data[3]);
            to_Channel(channel,lowByte,m_calibrate_step,four);
            return;}
        default:qCWarning(can)<<"[processFrame]error case";return;
    }
}

void CanServerManager::to_Channel(int channel,quint8 cmd,quint8 func,const QByteArray& param)
{
    // All channel send
    if (channel == 0) {
        #define CHANNEL(n) emit to_UartChannel##n(cmd, func, param,true);
        CHANNEL_COUNT
        #undef CHANNEL
        return;
    }

    // Single channel send
    switch(channel) {
        #define CHANNEL(n) case n: return emit to_UartChannel##n(cmd, func, param,true);
        CHANNEL_COUNT
        #undef CHANNEL
        default:
            qCWarning(can)<<"[to_Channel]Invalid channel: "<<channel;
            return;
    }
}

void CanServerManager::sendFrame(quint8 ch,quint16 uart,const QByteArray &param)
{
    // Queue rate limiting (to prevent infinite growth)
    if (m_sendQueue.size() <= 9999) {
        struct can_frame frame{};
        frame.can_id = m_canid;

        QByteArray data;
        quint32 cmf = m_canToUart.value(uart);
        if (cmf < 0x10000) {
            cmf = (cmf << 8) | static_cast<quint32>(m_calibrate_step);
        }
        cmf = (cmf & 0x00FFFFFF) | (static_cast<quint32>(ch) << 24);

        data.append(static_cast<char>((cmf >> 24) & 0xFF));
        data.append(static_cast<char>((cmf >> 16) & 0xFF));
        data.append(static_cast<char>((cmf >> 8) & 0xFF));
        data.append(static_cast<char>(cmf & 0xFF));

        QByteArray paddedParam(4, 0);
        int copyLen = qMin(param.size(), 4);
        memcpy(paddedParam.data() + (4 - copyLen), param.data(), copyLen);

        data.append(paddedParam);
        frame.can_dlc = static_cast<quint8>(data.size());
        memcpy(frame.data, data.constData(), data.size());

        qCDebug(can)<<"[sendFrame]:Sent Data: "<<data.toHex()<<", QueueRemain: "<<m_sendQueue.size()-1;
        if (m_sendQueue.isEmpty()) {m_writeNotifier->setEnabled(true);}
        m_sendQueue.enqueue(frame);
        return;
    }

    qCWarning(can)<<"[sendFrame]:Send queue overflow, size: "<<m_sendQueue.size();
}

void CanServerManager::change_canid(QString id){
    bool ok;
    m_canid = static_cast<quint8>(id.toUInt(&ok));

    if (!ok){
        qCWarning(can)<<"[change_canid]:tranform failed!!!";
    }
}
