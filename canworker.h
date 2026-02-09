#pragma once
#include <QMutex>
#include <QElapsedTimer>
#include <QLoggingCategory>
//#include <QThread>
//#include <QDateTime>
//#include <QAtomicInteger>

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

