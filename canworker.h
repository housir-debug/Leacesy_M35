#ifndef CANWORKER_H
#define CANWORKER_H

#include <QObject>
#include <QMutex>
#include <QThread>
#include <QDebug>
#include <QDateTime>
#include <QAtomicInteger>
#include <QLoggingCategory>

// Linux SocketCAN头文件
#include <linux/can.h>
#include <linux/can/raw.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <net/if.h>
#include <fcntl.h>
#include <cstring>

Q_DECLARE_LOGGING_CATEGORY(can);

class CanWorker : public QObject
{
    Q_OBJECT

public:
    explicit CanWorker(QObject *parent = nullptr);
    ~CanWorker();

    bool initialize(const QString  &interface = "can0", const int &bitrate = 1000000);
    void closeCan();

    bool sendFrame(quint32 canId, const QByteArray &data,const QString &canface);
    void forwardSerialData(const QByteArray &data);

    void testLoopback();
    void testserialloop();

signals:
    void frameReceived(const quint32 &canId, const QByteArray &data,const QString &canface);
    void SerialSendRequest(const QByteArray &data);

private:
    bool initializeSocket(const QString &interfaceName, int &socketFd);
    void listenLoop();
    void listenProcessing(const quint32 &canId, const QByteArray &data,const QString &canface);

private:
    QMutex m_Mutex;
    QElapsedTimer m_testtimer;

    int m_can0Socket{-1};
    int m_can1Socket{-1};
    int m_can2Socket{-1};

    QThread *m_listenThread{nullptr};
    std::atomic<bool> m_stopRequested{false};

    std::atomic<bool> m_testing{false};
    std::atomic<qint64> m_receivedCount{0};
};

#endif // CANWORKER_H
