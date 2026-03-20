#include "uart_server.h"
#include <QtCore>

Q_LOGGING_CATEGORY(uart_server, "UART_SERVER:")

UartServerManager::UartServerManager(ScpiManager* scpi,SerialBridge* qml,QObject *parent):
    QObject(parent), m_scpiManager(scpi), m_qmlbridge(qml) {}
UartServerManager::~UartServerManager()
{
    qCDebug(uart_server)<<"UartServerManager Destroyed!!!";
    delete m_scpiManager;
    m_scpiManager = nullptr;

    if (m_uartServer) {
        if (m_uartServer->isOpen()) {m_uartServer->close();}
        delete m_uartServer;
        m_uartServer= nullptr;
    }

    if (m_serverThread) {
        m_serverThread->quit();
        m_serverThread->wait(1000);// wait 1s
        m_serverThread->deleteLater();
        delete m_serverThread;
        m_serverThread = nullptr;
    }
}

// ===================== 启动部分 =================================

bool UartServerManager::startServer(const QString &portName,
                                    qint32 baudRate,
                                    QSerialPort::DataBits dataBits,
                                    QSerialPort::Parity parity,
                                    QSerialPort::StopBits stopBits)
{
    if (!m_serverThread){
        m_uartServer  = new QSerialPort(this);
        m_uartServer ->setFlowControl(QSerialPort::NoFlowControl); // In the majority situation
        //m_serialPort->setReadBufferSize(1024 * 1024); // 1MB buffer

        m_uartServer->setPortName(portName);
        m_uartServer->setBaudRate(baudRate);
        m_uartServer->setDataBits(dataBits);
        m_uartServer->setParity(parity);
        m_uartServer->setStopBits(stopBits);

        m_serverThread = new QThread(this);
        m_serverThread->setObjectName("UartServer");
    }

    if (thread() != m_serverThread) {
        this->moveToThread(m_serverThread);
        m_uartServer->moveToThread(m_serverThread);
    }

    if (!m_serverThread->isRunning()) {
        m_serverThread->start();

        connect(m_uartServer, &QSerialPort::readyRead, this, &UartServerManager::handleReadyRead, Qt::DirectConnection);
        connect(m_uartServer, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
            if (error == QSerialPort::NoError) {return;}
            qCWarning(uart_server)<<"Uartserver Occur Error: "<<m_uartServer->errorString();
        }, Qt::DirectConnection);

        QMetaObject::invokeMethod(this, [this]() {
            m_uartServer->open(QIODevice::ReadWrite);
        }, Qt::QueuedConnection);

        return true;
    }
    return false;
}

void UartServerManager::handleReadyRead()
{
    m_readbuffer.clear();
    m_readbuffer.append(m_uartServer->readAll());
    if (m_readbuffer.isEmpty()){return;}

    QString message = QString::fromUtf8(m_readbuffer).trimmed();   // SOCKET ASCll Define(0x00-0x7F)
    qCDebug(uart_server)<<"Uart SCPI Request Commend: "<<message;

    m_responsebuffer.clear();
    m_qmlbridge->update_remotemodel(true);
    m_responsebuffer = m_scpiManager->processCommand(m_readbuffer);
    m_qmlbridge->update_remotemodel(false);
    qCDebug(uart_server)<<"Uart SCPI Response: "<<m_responsebuffer;
    if (!m_responsebuffer.isEmpty()){m_uartServer->write(m_responsebuffer);}
}
