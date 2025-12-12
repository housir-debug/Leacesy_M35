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

    // CAN接口初始化
    bool initialize(const QString &interfaceName = "can0", int bitrate = 1000000);

    // 发送CAN帧
    bool sendFrame(quint32 canId, const QByteArray &data);

    // 回环速度测试
    void testLoopback();

    // 启动监听
    void startListening();

    // 关闭CAN接口
    void closeCan();

    // 获取状态
    bool isOpen() const { return m_canSocket >= 0; }
    bool isListening() const { return m_listening; }
    QString getInterfaceName() const { return m_interfaceName; }



signals:
    void frameReceived(quint32 canId, const QByteArray &data, qint64 timestamp);
    void errorOccurred(const QString &error);
    void canClosed();
    void frameSent(quint32 canId, bool success);

private slots:
    void listenProcessing(quint32 canId, const QByteArray &data, qint64 timestamp);

private:
    bool initializeSocket();
    bool configureInterface(int bitrate);
    void listenLoop();
    bool sendFrame(const can_frame &frame);

private:
    QMutex m_Mutex;        // 互斥锁
    int m_canSocket;           // CAN套接字描述符
    QString m_interfaceName;   // 接口名称
    int m_bitrate;            // 波特率
    QElapsedTimer m_testtimer;

    // 线程控制
    QThread *m_listenThread{nullptr};   // 监听线程
    std::atomic<bool> m_listening{false};
    std::atomic<bool> m_stopRequested{false};
    std::atomic<bool> m_testing{false};

    // 统计信息（原子操作，无需锁）
    std::atomic<quint64> m_receivedCount{0};
    std::atomic<quint64> m_sentCount{0};
};

#endif // CANWORKER_H
