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

    void testLoopback();
    void sendFrame(quint32 canId, const QByteArray &data);
    bool startServer(const QString &interface, quint32 bitrate);

private:
    bool createSocket(const QString &interface);
    void processFrame(quint32 canId,const QByteArray &data);

private:
    std::atomic<qint64> m_receivedCount{0};
    QElapsedTimer m_testtimer;
    std::atomic<bool> m_testing{false};

    int m_socketFd{-1};
    QQueue<struct can_frame> m_sendQueue;

    QThread *m_serverThread{nullptr};
    QSocketNotifier *m_readNotifier{nullptr};
    QSocketNotifier *m_writeNotifier{nullptr};
};






