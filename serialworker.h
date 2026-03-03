#pragma once
#include <QMutex>
#include <QTimer>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QSerialPort>
//#include <QSerialPortInfo>
//#include <QMap>
//#include <QThread>

Q_DECLARE_LOGGING_CATEGORY(uart_channel)

#define CHANNEL_1_TO_33 \
    CHANNEL(1) CHANNEL(2) CHANNEL(3) CHANNEL(4) CHANNEL(5) CHANNEL(6) CHANNEL(7) CHANNEL(8) \
    CHANNEL(9) CHANNEL(10) CHANNEL(11) CHANNEL(12) CHANNEL(13) CHANNEL(14) CHANNEL(15) CHANNEL(16) \
    CHANNEL(17) CHANNEL(18) CHANNEL(19) CHANNEL(20) CHANNEL(21) CHANNEL(22) CHANNEL(23) CHANNEL(24) \
    CHANNEL(25) CHANNEL(26) CHANNEL(27) CHANNEL(28) CHANNEL(29) CHANNEL(30) CHANNEL(31) CHANNEL(32) \
    CHANNEL(33)

class SerialWorker : public QObject
{
    Q_OBJECT

signals:
    // to qml display update
    // void serialDataReceived(const QByteArray &data,bool isforce); // Transit
    void statusChanged(int ch,QByteArray status);
    void voltageChanged(int ch,float measure);
    void currentChanged(int ch,float measure);
    void currentUnitChanged(int ch,bool status);
    void smallcurrentChanged(int ch,float measure);
    void temperatureChanged(int ch,float measure);
    void sinktemperatureChanged(int ch,float measure);
    void DVMACDCVoltageChanged(int ch,float measure);
    void DVMVoltageChanged(int ch,float measure);

    // to SCPI Command Query
    void channelreturnstatus(bool state);
    void channelreturnvalue(float value);
    void channelreturnintvalue(int value);

public:
    explicit SerialWorker(QObject *parent = nullptr);
    ~SerialWorker();

    bool initSerialPort(const QString &portName,
                        qint32 baudRate = QSerialPort::Baud115200,
                        QSerialPort::DataBits dataBits = QSerialPort::Data8,
                        QSerialPort::Parity parity = QSerialPort::NoParity,
                        QSerialPort::StopBits stopBits = QSerialPort::OneStop);

    void writeFrame(quint8 cmd, quint8 func, const QByteArray& param);
    void writeSerialData(const QByteArray& data,bool isforce);

private:
    void handleReadyRead();
    void handleuartrequest       (quint8 length);
    void handleOutputcmd         (quint8 func, quint8 ch);
    void handleSettingcmd        (quint8 func, quint8 ch);
    void handleControlcmd        (quint8 func, quint8 ch);
    void handleMeasurementcmd    (quint8 func, quint8 ch);
    void handleRegistercmd       (quint8 func, quint8 ch);
    void handleCalibratecmd      (quint8 func, quint8 ch);
    void handleCalibrationcmd    (quint8 func, quint8 ch);
    void handleTriggercmd        (quint8 func, quint8 ch);
    void handleISPcmd            (quint8 func, quint8 ch);
    void handleSNcmd             (quint8 func, quint8 ch);
    void handleIDcmd             (quint8 func, quint8 ch);
    void handleErrorcmd          (quint8 func);
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

    QMap<QString, quint8> portChannelMap;
    quint8 m_channel{0};

    QSerialPort *m_serialPort{nullptr};
    QTimer *m_refreshtimer{nullptr};
    QThread *m_serialThread{nullptr};

    QByteArray m_writebuffer;
    QByteArray m_readbuffer;
    QByteArray m_readparam;

    QElapsedTimer m_testTimer;
    std::atomic<bool> m_isTesting{false};
};

