#pragma once
#include <QtCore>
#include <QMutex>
#include <QTimer>
#include <QObject>
#include <QThread>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QLoggingCategory>
#include <QElapsedTimer>

Q_DECLARE_LOGGING_CATEGORY(uart)

class SerialWorker : public QObject
{
    Q_OBJECT

signals:
    void serialDataReceived(const QByteArray &data);
    void statusChanged(QByteArray status);
    void voltageChanged(float measure);
    void currentChanged(float measure);
    void currentUnitChanged(bool status);
    void smallcurrentChanged(float measure);
    void temperatureChanged(float measure);
    void sinktemperatureChanged(float measure);
    void DVMACDCVoltageChanged(float measure);
    void DVMVoltageChanged(float measure);


public:
    explicit SerialWorker(QObject *parent = nullptr);
    ~SerialWorker();

    bool initSerialPort(const QString &portName,
                        qint32 baudRate = QSerialPort::Baud115200,
                        QSerialPort::DataBits dataBits = QSerialPort::Data8,
                        QSerialPort::Parity parity = QSerialPort::NoParity,
                        QSerialPort::StopBits stopBits = QSerialPort::OneStop);

    void writeFrame(quint8 cmd, quint8 func, quint8 ch, const QByteArray& param);
    void writeSerialData(const QByteArray& data);

    void closeSerial();

private:
    void handleReadyRead();
    bool handleuartrequest(quint8 length,const QByteArray& data);
    void handleOutputcmd(quint8 func, quint8 ch, const QByteArray& param);
    void handleSettingcmd(quint8 func, quint8 ch, const QByteArray& param);
    void handleControlcmd(quint8 func, quint8 ch, const QByteArray& param);
    void handleMeasurementcmd(quint8 func, quint8 ch, const QByteArray& param);
    void handleRegistercmd(quint8 func, quint8 ch, const QByteArray& param);
    void handleCalibratecmd(quint8 func, quint8 ch, const QByteArray& param);
    void handleCalibrationcmd(quint8 func, quint8 ch, const QByteArray& param);
    void handleTriggercmd(quint8 func, quint8 ch, const QByteArray& param);
    void handleISPcmd(quint8 func, quint8 ch, const QByteArray& param);
    void handleSNcmd(quint8 func, quint8 ch, const QByteArray& param);
    void handleIDcmd(quint8 func, quint8 ch, const QByteArray& param);
    void handleErrorcmd(quint8 func, quint8 ch, const QByteArray& param);
    void startLoopbackTest();

private:
    float lastVoltage{0.0f};
    float lastCurrent{0.0f};
    float lastSmallCurrent{0.0f};
    float lasttemper{0.0f};
    float lastheatsinktemper{0.0f};
    float lastDVMACDCVoltage{0.0f};
    float lastDVMVoltage{0.0f};

    static constexpr quint8 HEADER_HIGH = 0xAA;
    static constexpr quint8 HEADER_LOW = 0x55;
    static constexpr quint8 END_MARKER = 0xEE;

    QMutex m_WriteMutex;
    QByteArray m_readbuffer;
    QByteArray m_writebuffer;

    QTimer *m_refreshtimer{nullptr};
    QThread *m_serialThread{nullptr};
    QSerialPort *m_serialPort{nullptr};
    QString m_portName{""};

    QElapsedTimer m_testTimer;
    std::atomic<bool> m_isTesting{false};
    std::atomic<qint64> m_bytesReceived{0};
};

