#pragma once
#include <QMutex>
#include <QQueue>
#include <QElapsedTimer>
#include <QSocketNotifier>
#include <QLoggingCategory>
#include <linux/can/raw.h>
#include "auxiliary/config_manager.h"

Q_DECLARE_LOGGING_CATEGORY(can);

class CanServerManager : public QObject
{
    Q_OBJECT

signals:
    #define CHANNEL(n) void to_UartChannel##n(quint8 cmd, quint8 func, const QByteArray& param,bool isScpi);

    CHANNEL_COUNT
    #undef CHANNEL

public:
    explicit CanServerManager(QObject *parent = nullptr);
    ~CanServerManager();

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






