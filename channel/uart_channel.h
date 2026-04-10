#pragma once
#include <QTimer>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include "auxiliary/config_manager.h"
#include "auxiliary/scpi_handle.h"
#include "auxiliary/qml_agency.h"

Q_DECLARE_LOGGING_CATEGORY(uart_channel)

class UartChannelManager : public QObject
{
    Q_OBJECT

signals:
    // Transit
    void serialDataReceived(const QByteArray& data,bool isforce);

public:
    explicit UartChannelManager(QObject *parent = nullptr);
    ~UartChannelManager();

    std::shared_ptr<ScpiManager> m_scpiManager{nullptr};
    std::shared_ptr<GuiBridge> m_qmlbridge{nullptr};

    bool initSerialPort(const QString &portName,
                        qint32 baudRate = QSerialPort::Baud115200,
                        QSerialPort::DataBits dataBits = QSerialPort::Data8,
                        QSerialPort::Parity parity = QSerialPort::NoParity,
                        QSerialPort::StopBits stopBits = QSerialPort::OneStop);

    void writeFrame(quint8 cmd, quint8 func, const QByteArray& param,bool isScpi);
    void writeSerialData(const QByteArray& data,bool isforce);

private:
    void handleReadyRead();
    void handleuartrequest       (quint8 length);
    void handleOutputcmd         (quint8 func);
    void handleSettingcmd        (quint8 func);
    void handleControlcmd        (quint8 func);
    void handleMeasurementcmd    (quint8 func);
    void handleRegistercmd       (quint8 func);
    void handleCalibratecmd      (quint8 func);
    void handleCalibrationcmd    (quint8 func);
    void handleTriggercmd        (quint8 func);
    void handleISPcmd            (quint8 func);
    void handleSNcmd             (quint8 func);
    void handleIDcmd             (quint8 func);
    void handleErrorcmd          (quint8 func);

    void startLoopbackTest();
    void sendInitCommand();

private:
    struct Command {
        quint8 cmd;
        quint8 func;
        QByteArray param;
        bool isScpi;
    };

    QSerialPort *m_serialPort{nullptr};
    QThread *m_serialThread{nullptr};
    QTimer *m_refreshtimer{nullptr};

    quint8 m_channel{0};
    QVector<Command> m_initCommands;
    int m_currentInitIndex{0};
    QElapsedTimer m_testTimer;
    bool m_isTesting{false};

    QByteArray m_writebuffer;
    QByteArray m_readbuffer;
    QByteArray m_readparam;
    bool m_isSCPIrequest{false};

    static constexpr quint8 HEADER_HIGH = 0xAA;
    static constexpr quint8 HEADER_LOW = 0x55;
    static constexpr quint8 END_MARKER = 0xEE;
};

