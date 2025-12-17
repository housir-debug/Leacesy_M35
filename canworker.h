#ifndef CANWORKER_H
#define CANWORKER_H

#include <QObject>
#include <QMutex>
#include <QThread>
#include <QDebug>
#include <QDateTime>
#include <QAtomicInteger>

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

class CanWorker : public QObject
{
    Q_OBJECT

public:
    explicit CanWorker(QObject *parent = nullptr);
    ~CanWorker();

    bool initialize(const QString &interfaceName = "can0", int bitrate = 1000000);
    void closeCan();

    bool sendFrame(quint32 canId, const QByteArray &data);
    void testLoopback();

    bool isOpen() const { return m_canSocket >= 0; }
    bool isListening() const { return m_listening; }
    QString getInterfaceName() const { return m_interfaceName; }

signals:
    void errorOccurred(const QString &error);
    void frameSented(quint32 canId, bool success);
    void frameReceived(quint32 canId, const QByteArray &data, qint64 timestamp);
    void canClosed();

private:
    bool initializeSocket();
    bool configureInterface(int bitrate);
    void startListening();

    void listenProcessing(quint32 canId, const QByteArray &data, qint64 timestamp);
    void listenLoop();
    bool sendFrame_en(const can_frame &frame);

private:
    QMutex m_Mutex;
    QElapsedTimer m_testtimer;

    int m_canSocket;
    QString m_interfaceName;
    int m_bitrate;

    int m_receivedCount{0};
    int m_sentCount{0};

    QThread *m_listenThread{nullptr};
    std::atomic<bool> m_listening{false};
    std::atomic<bool> m_stopRequested{false};
    std::atomic<bool> m_testing{false};
};

#endif // CANWORKER_H
