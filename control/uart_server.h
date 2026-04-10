#pragma once
#include "auxiliary/scpi_handle.h"
#include "auxiliary/qml_agency.h"

Q_DECLARE_LOGGING_CATEGORY(uart_server)

class UartServerManager : public QObject
{
    Q_OBJECT

public:
    explicit UartServerManager(QObject *parent = nullptr);
    ~UartServerManager();

    bool startServer(const QString &portName,qint32 baudRate);

    std::shared_ptr<ScpiManager> m_scpiManager{nullptr};
    std::shared_ptr<GuiBridge> m_qmlbridge{nullptr};

private:
    void handleReadyRead();

    QByteArray m_readbuffer;
    QByteArray m_responsebuffer;

    QSerialPort *m_uartServer{nullptr};
    QThread *m_serverThread{nullptr};
};
