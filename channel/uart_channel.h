#pragma once
#include <QTimer>
#include <QDataStream>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include "auxiliary/config_manager.h"
#include "auxiliary/scpi_handle.h"
#include "auxiliary/qml_agency.h"

Q_DECLARE_LOGGING_CATEGORY(uart_channel)

struct Command {
    quint8 cmd;
    quint8 func;
    QByteArray param;
    bool isScpi;
};

class UartChannelManager : public QObject
{
    Q_OBJECT

signals:
    // Transit
    void serialDataReceived(const QByteArray& data,bool isforce);

public:
    explicit UartChannelManager(QObject *parent = nullptr);
    ~UartChannelManager();

    bool initSerialPort(const QString &portName,qint32 baudRate);
    void writeFrame(quint8 cmd, quint8 func, const QByteArray& param,bool isScpi);

    std::shared_ptr<ScpiManager> m_scpiManager{nullptr};
    std::shared_ptr<GuiBridge> m_qmlbridge{nullptr};

private:
    void handleReadyRead();
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

private:
    void sendInitCommand();
    void startLoopbackTest();
    QElapsedTimer m_testTimer;

    QByteArray m_readparam;
    QDataStream m_stream{&m_readparam, QIODevice::ReadOnly};

    QByteArray m_readbuffer;
    bool m_isSCPIrequest{false};
    QByteArray m_responsebuffer;

    quint8 m_channel{0};
    quint8 m_InitIndex{0};

    QTimer *m_refreshtimer{nullptr};
    QThread *m_serialThread{nullptr};
    QSerialPort *m_serialPort{nullptr};

    static const QVector<Command> m_initCommands;
    static constexpr quint8 HEADER_HIGH = 0xAA;
    static constexpr quint8 HEADER_LOW = 0x55;
    static constexpr quint8 END_MARKER = 0xEE;
};

