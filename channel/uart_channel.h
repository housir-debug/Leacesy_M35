#pragma once
#include <QTimer>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include "auxiliary/config_manager.h"
#include "auxiliary/scpi_handle.h"
#include "auxiliary/qml_agency.h"

Q_DECLARE_LOGGING_CATEGORY(uart_channel)

class SerialWorker : public QObject
{
    Q_OBJECT

signals:
    // Transit
    void serialDataReceived(const QByteArray& data,bool isforce);

    // to qml display update
    /*void statusChanged(int ch,const QByteArray& status);
    void voltageChanged(int ch,float measure);
    void currentChanged(int ch,float measure);
    void currentUnitChanged(int ch,bool status);

    // to SCPI Command Query
    void channelreturnstatus(bool state);
    void channelreturnvalue(float value);
    void channelreturnintvalue(int value);*/

public:
    explicit SerialWorker(ScpiManager* scpi,SerialBridge* qml,QObject *parent = nullptr);
    ~SerialWorker();

    bool initSerialPort(const QString &portName,
                        qint32 baudRate = QSerialPort::Baud115200,
                        QSerialPort::DataBits dataBits = QSerialPort::Data8,
                        QSerialPort::Parity parity = QSerialPort::NoParity,
                        QSerialPort::StopBits stopBits = QSerialPort::OneStop);

    void writeFrame(quint8 cmd, quint8 func, const QByteArray& param,bool isScpi);
    void writeSerialData(const QByteArray& data,bool isforce);

private:
    void sendNextCommand(QMap<int, QByteArray>::iterator it,QMap<int, QByteArray>::iterator end);
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

private:
    static constexpr quint8 HEADER_HIGH = 0xAA;
    static constexpr quint8 HEADER_LOW = 0x55;
    static constexpr quint8 END_MARKER = 0xEE;
    QMap<int, QByteArray> m_commands;

    QSerialPort *m_serialPort{nullptr};
    QTimer *m_refreshtimer{nullptr};
    QThread *m_serialThread{nullptr};
    quint8 m_channel{0};
    bool m_isSCPIrequest{false};
    ScpiManager* m_scpiManager{nullptr};
    SerialBridge* m_qmlbridge{nullptr};

    QByteArray m_writebuffer;
    QByteArray m_readbuffer;
    QByteArray m_readparam;

    QElapsedTimer m_testTimer;
    bool m_isTesting{false};
};

