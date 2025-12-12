#ifndef SERIALWORKER_H
#define SERIALWORKER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QThread>
#include <QDebug>
#include <QMutex>
#include <QElapsedTimer>

class SerialWorker : public QObject
{
    Q_OBJECT

public:
    explicit SerialWorker(QObject *parent = nullptr);
    ~SerialWorker();

    // 串口初始化函数
    bool initSerialPort(const QString &portName,
                        qint32 baudRate = QSerialPort::Baud115200,
                        QSerialPort::DataBits dataBits = QSerialPort::Data8,
                        QSerialPort::Parity parity = QSerialPort::NoParity,
                        QSerialPort::StopBits stopBits = QSerialPort::OneStop);

    // 串口数据写入函数
    void writeSerialData(const QByteArray &data);

    // 关闭串口
    void closeSerial();

    // ============ 新增：回环测试方法 ============
    void startLoopbackTest();      // 开始回环测试
    bool isTesting() const{return m_isTesting.load();}        // 是否正在测试
    // ==========================================

signals:
    void serialDataReceived(const QByteArray &data);
    void serialErrorOccurred(const QString &error);
    void finished();

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);

private:
    QSerialPort *m_serialPort;
    bool m_isListening;
    QMutex m_mutex;

    // ============ 新增：测试相关成员 ============
    std::atomic<bool> m_isTesting{false};// 测试标志
    QElapsedTimer m_testTimer;      // 测试计时器
    QByteArray m_testData;          // 测试数据
    int m_bytesReceived;           // 接收字节数
    // ==========================================
};

#endif // SERIALWORKER_H
