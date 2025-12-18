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
    void closeSerial();

    void writeSerialData(const QByteArray &data);

    void startLoopbackTest();

    bool isTesting() const{return m_isTesting.load();}

signals:
    void serialDataReceived(const QByteArray &data);
    void serialErrorOccurred(const QString &error);

private:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);

private:
    QMutex m_mutex;
    QElapsedTimer m_testTimer;
    QByteArray m_testData;

    QThread *m_serialThread{nullptr};
    QSerialPort *m_serialPort{nullptr};
    bool m_isListening{false};
    std::atomic<bool> m_isTesting{false};
    std::atomic<qint64> m_bytesReceived{0};
};

#endif // SERIALWORKER_H
