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

    bool initSerialPort(const QString &portName,
                        qint32 baudRate = QSerialPort::Baud115200,
                        QSerialPort::DataBits dataBits = QSerialPort::Data8,
                        QSerialPort::Parity parity = QSerialPort::NoParity,
                        QSerialPort::StopBits stopBits = QSerialPort::OneStop);

    void writeSerialData(const QByteArray &data);

    void closeSerial();

    void startLoopbackTest();
    bool isTesting() const{return m_isTesting.load();}

signals:
    void serialDataReceived(const QByteArray &data);
    void serialErrorOccurred(const QString &error);
    void finished();

private:
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
