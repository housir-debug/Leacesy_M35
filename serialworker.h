#ifndef SERIALWORKER_H
#define SERIALWORKER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QThread>
#include <QDebug>
#include <QMutex>
#include <QElapsedTimer>
#include <QLoggingCategory>


Q_DECLARE_LOGGING_CATEGORY(uart)

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
    void closeSerial();

    void writeSerialData(const QByteArray &data);

    void startLoopbackTest();

signals:
    void serialDataReceived(const QByteArray &data);

private:
    void handleReadyRead();

private:
    QMutex m_mutex;
    QElapsedTimer m_testTimer;
    std::atomic<bool> m_isTesting{false};
    std::atomic<qint64> m_bytesReceived{0};

    QString m_portName{""};
    QThread *m_serialThread{nullptr};
    QSerialPort *m_serialPort{nullptr};
};

#endif // SERIALWORKER_H
