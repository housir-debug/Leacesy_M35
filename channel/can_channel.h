#pragma once
#include <QMutex>
#include <QQueue>
#include <QElapsedTimer>
#include <QSocketNotifier>
#include <QLoggingCategory>
#include <linux/can/raw.h>

Q_DECLARE_LOGGING_CATEGORY(can);

class CanWorker : public QObject
{
    Q_OBJECT

signals:
    void SerialSendRequest(const QByteArray &data);

public:
    explicit CanWorker(QObject *parent = nullptr);
    ~CanWorker();

    bool sendFrame(quint32 canId, const QByteArray &data,const QString &canface);

    bool initialize(const QString  &interface = "can0", int bitrate = 1000000);
    void testLoopback();

private:
    bool initializeSocket(const QString &interfaceName);
    void handleWriteReady(int socketFd);
    void listenLoop();
    void listenProcessing(quint32 canId,const QString &canface);

private:
    QHash<QString, QSocketNotifier*> m_writeNotifiers;
    QHash<QString, QQueue<struct can_frame>> m_sendQueues;
    QHash<int, QString> m_fdToInterface;

    QThread *m_listenThread{nullptr};
    std::atomic<bool> m_stopListen{false};
    std::atomic<qint64> m_receivedCount{0};
    QByteArray m_responsebuffer;

    std::atomic<bool> m_testing{false};
    QElapsedTimer m_testtimer;

};

