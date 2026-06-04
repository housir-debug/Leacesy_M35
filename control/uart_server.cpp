#include "uart_server.h"
#include <QtCore>

Q_LOGGING_CATEGORY(uart_server, "UART_SERVER:")

UartServerManager::UartServerManager(QObject *parent): QObject(parent) {}
UartServerManager::~UartServerManager()
{
    if (m_uartServer) {
        m_uartServer->close();
        delete m_uartServer;
        m_uartServer= nullptr;
    }

    if (m_serverThread) {
        m_serverThread->quit();
        m_serverThread->wait(1000); // wait 1s
        m_serverThread->deleteLater();
        delete m_serverThread;
        m_serverThread = nullptr;
    }

    qCDebug(uart_server)<<"[~UartServerManager]:UartServerManager Destroyed!!!";
}

bool UartServerManager::startServer(const QString &portName,qint32 baudRate)
{
    if (!m_serverThread && !m_uartServer){
        m_uartServer  = new QSerialPort(this);

        m_uartServer->setPortName(portName);
        m_uartServer->setBaudRate(baudRate); // QSerialPort::Baud115200

        // Set the data bit to 8 bits, For example: Data5 - Data8
        m_uartServer->setDataBits(QSerialPort::Data8);
        // Not use parity check bits, the upper-level protocol ensures data integrity.
        m_uartServer->setParity(QSerialPort::NoParity);
        // Use 1 stop bit, Mark the end of A data byte
        m_uartServer->setStopBits(QSerialPort::OneStop);
        // HardwareControl: Requires wiring support | SoftwareControl: Applicable only to written text
        m_uartServer ->setFlowControl(QSerialPort::NoFlowControl);

        m_serverThread = new QThread(this);
        m_serverThread->setObjectName("UartServer");

        this->moveToThread(m_serverThread);
        m_uartServer->moveToThread(m_serverThread);
        m_serverThread->start();

        connect(m_uartServer, &QSerialPort::readyRead, this, &UartServerManager::handleReadyRead, Qt::DirectConnection);
        connect(m_uartServer, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
            if (error != QSerialPort::NoError) {
                qCWarning(uart_server)<<"[startServer]:Uartserver Occur Error: "<<m_uartServer->errorString();
            }}, Qt::DirectConnection);

        QMetaObject::invokeMethod(this, [this]() {
            m_uartServer->open(QIODevice::ReadWrite);
        }, Qt::QueuedConnection);
        return true;
    }

    qCWarning(uart_server)<<"[startServer]:already exist A certain member";
    return false;
}

void UartServerManager::handleReadyRead()
{
    if (m_qmlbridge->m_remoteStatus.load()!=3 && m_qmlbridge->m_remoteStatus.load()!=0){
        QByteArray errMsg = QString("Other interfaces of the instrument are currently in operation").toUtf8();
        qCDebug(uart_server)<<"[handleReadyRead]:Currently in an alternative remote mode";
        m_uartServer->write(errMsg);
        return;
    }
    else if (m_qmlbridge->m_remoteStatus.load()==0){
        m_qmlbridge->update_remotemodel(3);
    }

    // read information
    m_readbuffer.clear();
    m_readbuffer.append(m_uartServer->readAll());
    QString message = QString::fromUtf8(m_readbuffer).trimmed();   // SOCKET ASCll Define(0x00-0x7F)
    qCDebug(uart_server)<<"[handleReadyRead]:Uart SCPI Request Commend: "<< message;

    // Return response
    m_responsebuffer.clear();
    m_responsebuffer = m_scpiManager->processCommand(m_readbuffer);
    if (!m_responsebuffer.isEmpty()){
        qCDebug(uart_server)<<"[handleReadyRead]:Uart SCPI Response: "<<m_responsebuffer;
        m_uartServer->write(m_responsebuffer);
    }
}
