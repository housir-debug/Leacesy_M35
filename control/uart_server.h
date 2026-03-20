#pragma once
#include "auxiliary/scpi_handle.h"
#include "auxiliary/qml_agency.h"

Q_DECLARE_LOGGING_CATEGORY(uart_server)

class UartServerManager : public QObject
{
    Q_OBJECT

public:
    explicit UartServerManager(ScpiManager* scpi,SerialBridge* qml,QObject *parent = nullptr);
    ~UartServerManager();

    bool startServer(const QString &portName,
                     qint32 baudRate = QSerialPort::Baud115200,
                     QSerialPort::DataBits dataBits = QSerialPort::Data8,
                     QSerialPort::Parity parity = QSerialPort::NoParity,
                     QSerialPort::StopBits stopBits = QSerialPort::OneStop);

private:
    void handleReadyRead();

private:
    QByteArray m_readbuffer;
    QByteArray m_responsebuffer;

    ScpiManager* m_scpiManager{nullptr};
    SerialBridge* m_qmlbridge{nullptr};
    QSerialPort *m_uartServer{nullptr};
    QThread *m_serverThread{nullptr};
};
