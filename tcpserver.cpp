#include "tcpserver.h"
#include "canworker.h"
#include <QNetworkInterface>
#include <QDateTime>
#include <QCoreApplication>
#include <QElapsedTimer>

// CRC16计算（简单实现，可根据需要替换更复杂的算法）
static quint16 crc16(const quint8 *data, int length)
{
    quint16 crc = 0xFFFF;

    for (int i = 0; i < length; i++) {
        crc ^= (quint16)data[i] << 8;

        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

bool TcpServerManager::validatePacket(const ControlPacket &packet)
{
    // 检查魔法头
    if (packet.magic != 0xBEEF) {
        qWarning() << "Invalid magic header:" << QString::number(packet.magic, 16);
        return false;
    }

    // 检查数据长度
    if (packet.dataLength > 8) {
        qWarning() << "Invalid data length:" << packet.dataLength;
        return false;
    }

    // 计算并验证CRC（从command开始到data结束）
    QByteArray crcData(reinterpret_cast<const char*>(&packet.command),
                      sizeof(packet) - offsetof(ControlPacket, command) - sizeof(packet.crc));
    quint16 calculatedCrc = crc16(reinterpret_cast<const quint8*>(crcData.constData()), crcData.size());

    if (calculatedCrc != packet.crc) {
        qWarning() << "CRC mismatch: expected" << QString::number(packet.crc, 16)
                  << "got" << QString::number(calculatedCrc, 16);
        return false;
    }

    return true;
}


TcpServerManager::TcpServerManager( QObject *parent)
    : QObject(parent)
    , m_port(502)
    , m_interfaceName("can0")
{
    // 创建TCP服务器（注意：对象在调用线程创建）
    m_tcpServer = new QTcpServer(this);

    // 创建独立线程处理TCP通信
    m_serverThread = new QThread(this);
    m_serverThread->setObjectName("TcpServer");

    // 定时器：心跳包和清理
    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setInterval(30000); // 30秒心跳
    connect(m_heartbeatTimer, &QTimer::timeout, this, [this]() {
        QByteArray heartbeat;
        heartbeat.append("HEARTBEAT");
        sendToAllClients(heartbeat);
    });

    m_cleanupTimer = new QTimer(this);
    m_cleanupTimer->setInterval(5000); // 5秒清理一次
    connect(m_cleanupTimer, &QTimer::timeout,
            this, &TcpServerManager::cleanupDisconnectedClients);

    // 连接信号槽
    connect(m_tcpServer, &QTcpServer::newConnection,
            this, &TcpServerManager::onNewConnection);

    qDebug() << "TcpServerManager created, port:" << m_port;
}

void TcpServerManager::cleanupDisconnectedClients()
{
    QMutexLocker locker(&m_clientMutex);

    QList<QTcpSocket*> toRemove;

    for (QTcpSocket *client : qAsConst(m_clients)) {
        if (client->state() != QAbstractSocket::ConnectedState) {
            toRemove.append(client);
        }
    }

    for (QTcpSocket *client : toRemove) {
        m_clients.removeOne(client);
        client->deleteLater();
        qDebug() << "Cleaned up disconnected client:" << client->objectName();
    }
}

void TcpServerManager::onNewConnection()
{
    QTcpSocket *client = m_tcpServer->nextPendingConnection();
    if (!client) {
        return;
    }

    // 检查最大连接数
    {
        QMutexLocker locker(&m_clientMutex);
        if (m_clients.size() >= 10) {
            qWarning() << "Max client limit reached, rejecting connection from"
                      << client->peerAddress().toString();
            client->disconnectFromHost();
            delete client;
            return;
        }

        m_clients.append(client);
        m_totalClients++;
    }

    // 设置客户端属性
    QString clientInfo = QString("%1:%2")
                        .arg(client->peerAddress().toString())
                        .arg(client->peerPort());

    client->setObjectName(clientInfo);

    // 连接信号槽
    connect(client, &QTcpSocket::readyRead,
            this, &TcpServerManager::onClientReadyRead);
    connect(client, &QTcpSocket::disconnected,
            this, &TcpServerManager::onClientDisconnected);
    connect(client, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &TcpServerManager::onSocketError);

    qInfo() << "New client connected:" << clientInfo
           << ", total clients:" << m_clients.size();;
    //emit clientConnected(clientInfo);

    // 发送欢迎消息
    QByteArray welcome = QString("Welcome to Meacesy Server (Clients: %1)\n")
                        .arg(m_clients.size()).toUtf8();
    client->write(welcome);
}


void TcpServerManager::onClientReadyRead()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) {return;}

    QByteArray rawData = client->readAll();
    m_totalBytesReceived += rawData.size();
    qDebug() << "Received from" << client->objectName()<< "size:" << rawData.size()
            << "hex:" << rawData.toHex();

    m_clientBuffers[client].append(rawData);
    // 异步处理，立即放入事件队列，不阻塞事件循环
    QTimer::singleShot(0, this, [this, client]() {
        processClientData(client);
    });
}

void TcpServerManager::processClientData(QTcpSocket *client)
{
    QByteArray &buffer = m_clientBuffers[client];

    qDebug() << "Processing buffer for" << client->objectName()
                << "size:" << buffer.size()
                << "compare:" << static_cast<int>(sizeof(ControlPacket));

    while (true) {
        if (buffer.trimmed().toUpper() == "HEARTBEAT") {// 文本格式的心跳包
            client->write("HEARTBEAT_RESPONSE\n");
            m_testtimer.start();
            //emit test();
            quint32 testId = 0x321;
            const QByteArray &testData = QByteArray::fromHex("1122334455667788");
            emit canSendRequest(testId, testData);
            buffer.remove(0, 9);
        }
        else if (buffer.trimmed().toUpper() == "STATUS") {// 文本格式的状态查询
            QString status = QString(
                "=== CAN TCP Server Status ===\n"
                "Server State: %1\n"
                "Port: %2\n"
                "Clients: %3/%4\n"
                "Bytes Sent: %5\n"
                "Bytes Received: %6\n"
                "=============================\n"
            ).arg(m_running.load() ? "Running" : "Stopped")
             .arg(m_port)
             .arg(m_clients.size()).arg(10)  // 当前连接数/最大连接数
             .arg(m_totalBytesSent.load())
             .arg(m_totalBytesReceived.load());

            client->write(status.toUtf8());
            buffer.remove(0, 6);
        }
        else if (buffer.size() >= static_cast<int>(sizeof(ControlPacket))) {//二进制控制指令包
            ControlPacket packet;
            memcpy(&packet, buffer.constData(), sizeof(packet));

            // 验证魔法头和CRC
            if (validatePacket(packet)) {
                handleCommand(client, packet);
                buffer.remove(0, sizeof(ControlPacket));// 移除已处理的数据
                continue;  // 继续处理缓冲区中可能的下一个包
            } else {
                qWarning() << "Invalid packet from" << client->objectName();
                QByteArray errorMsg = "ERROR: Invalid packet format\n";
                client->write(errorMsg);
                buffer.remove(0, 4);
                continue;
            }
        }
        else {// 短文本-未知命令（查找换行符）
            int newlinePos = buffer.indexOf('\n');
            if (newlinePos != -1) {
                QByteArray command = buffer.left(newlinePos);
                //processTextCommand(client, command);
                buffer.remove(0, newlinePos + 1);  // 移除命令和换行符
                qDebug() << command;
                continue;
            }
            //qWarning() << "Unknown command from" << client->objectName()<< ":" << buffer;
            buffer.remove(0, buffer.size());
            break;// 没有完整的包了，退出循环
        }
    }
}

void TcpServerManager::handleCommand(QTcpSocket *client, const ControlPacket &packet)
{
    if (!client || client->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    switch (packet.command) {
        case 0x01:{// 心跳包
            qDebug() << "Heartbeat from" << client->objectName();
            QByteArray response;
            response.append("HEARTBEAT_OK\n");
            client->write(response);
            break;
        }

        case 0x02: {// 发送CAN帧
            QByteArray canData(reinterpret_cast<const char*>(packet.data), packet.dataLength);
            qDebug() << "CAN send request from" << client->objectName()
                    << "ID:" << QString::number(packet.canId, 16)
                    << "Data:" << canData.toHex();

            // 转发给CanWorker
            emit canSendRequest(packet.canId, canData);

            // 发送确认响应
            QByteArray response = QString("CAN_SEND_ACK ID:0x%1\n")
                                 .arg(packet.canId, 0, 16).toUtf8();
            client->write(response);
            break;
        }

        case 0x03:{// 查询状态
            qDebug() << "Status query from" << client->objectName();
            QString status = QString(
                "=== CAN TCP Server Status ===\n"
                "Server State: %1\n"
                "Port: %2\n"
                "Clients: %3/%4\n"
                "Interface: %5\n"
                "Bytes Sent: %6\n"
                "Bytes Received: %7\n"
                "Uptime: %8 seconds\n"
                "=============================\n"
            ).arg(m_running.load() ? "Running" : "Stopped")
             .arg(m_port)
             .arg(m_clients.size()).arg(10)  // 当前连接数/最大连接数
             .arg(m_interfaceName)
             .arg(m_totalBytesSent.load())
             .arg(m_totalBytesReceived.load())
             .arg(m_serverThread ? m_serverThread->objectName() : "Unknown");

            client->write(status.toUtf8());
            break;
        }

        default:{//未知命令
        qWarning() << "Unknown command type:" << packet.command
                  << "from" << client->objectName();

        QByteArray error = QString("ERROR: Unknown command 0x%1\n")
                          .arg(packet.command, 2, 16, QChar('0')).toUtf8();
        client->write(error);
        break;
        }
    }
}

void TcpServerManager::onClientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) {
        return;
    }

    QString clientInfo = client->objectName();

    {
        QMutexLocker locker(&m_clientMutex);
        m_clients.removeOne(client);
    }

    client->deleteLater();

    qInfo() << "Client disconnected:" << clientInfo
           << ", remaining clients:" << m_clients.size();
    emit clientDisconnected(clientInfo);
}

void TcpServerManager::onSocketError(QAbstractSocket::SocketError error)
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) {
        return;
    }

    qWarning() << "Socket error from" << client->objectName()
              << ":" << client->errorString() << "|" << error;
}


TcpServerManager::~TcpServerManager()
{
    stopServer();

    if (m_serverThread && m_serverThread->isRunning()) {
        m_serverThread->quit();
        m_serverThread->wait(1000);
    }

    qDebug() << "TcpServerManager destroyed";
}

void TcpServerManager::stopServer()
{
    if (m_state.load() == STATE_STOPPED) {return;}

    m_state.store(STATE_STOPPING);
    m_running.store(false);

    // 停止定时器
    m_heartbeatTimer->stop();
    m_cleanupTimer->stop();

    // 关闭所有客户端连接
    {
        QMutexLocker locker(&m_clientMutex);
        for (QTcpSocket *client : qAsConst(m_clients)) {
            client->disconnectFromHost();
            if (client->state() == QAbstractSocket::ConnectedState) {
                client->waitForDisconnected(1000);
            }
        }
        m_clients.clear();
    }

    // 停止服务器
    if (m_tcpServer->isListening()) {
        m_tcpServer->close();
    }

    m_state.store(STATE_STOPPED);
    qInfo() << "TcpServer stopped";
}


bool TcpServerManager::startServer()
{
    if (m_state.load() != STATE_STOPPED) {
        qWarning() << "TcpServer is not in stopped state";
        return false;
    }

    m_state.store(STATE_STARTING);

    // 将 TCP 服务器及其相关的业务逻辑对象移动到独立线程
    if (thread() != m_serverThread) {
        this->moveToThread(m_serverThread);
        m_tcpServer->moveToThread(m_serverThread);
    }
    if (!m_serverThread->isRunning()) {
        m_serverThread->start();
    }

    // 在线程中启动服务器
    QMetaObject::invokeMethod(this, [this]() {
        if (m_tcpServer->listen(QHostAddress::Any, m_port)) {
            m_state.store(STATE_RUNNING);
            m_running.store(true);

            m_heartbeatTimer->start();
            m_cleanupTimer->start();

            qInfo() << "TcpServer started on port" << m_port
                   << ", thread:" << QThread::currentThread()->objectName();
        } else {
            m_state.store(STATE_STOPPED);
            QString error = QString("Failed to start TCP server: %1")
                           .arg(m_tcpServer->errorString());
            qCritical() << error;
            emit errorOccurred(error);
        }
    }, Qt::QueuedConnection);

    return true;
}


void TcpServerManager::forwardCanData(quint32 canId, const QByteArray &data, qint64 timestamp)
{
    if (!m_running.load()) {
        return;
    }

    // 构建CAN数据包
    CanDataPacket packet;
    memset(&packet, 0, sizeof(packet));

    // 填充包头
    packet.magic = 0xCAFE;
    packet.length = sizeof(packet) - sizeof(packet.magic) - sizeof(packet.length);
    packet.timestamp = timestamp;

    // 接口名（保证以null结尾）
    QString interfaceName = m_interfaceName;
    QByteArray interfaceBytes = interfaceName.left(15).toUtf8();
    memcpy(packet.interface, interfaceBytes.constData(), interfaceBytes.size());

    // CAN数据
    packet.canId = canId;
    int dataSize = qMin(data.size(), 8);
    packet.data[0] = static_cast<quint8>(dataSize);
    memcpy(packet.data + 1, data.constData(), dataSize);

    // 计算CRC（从timestamp开始计算）
    QByteArray crcData(reinterpret_cast<const char*>(&packet.timestamp),
                      sizeof(packet) - offsetof(CanDataPacket, timestamp) - sizeof(packet.crc));
    packet.crc = crc16(reinterpret_cast<const quint8*>(crcData.constData()), crcData.size());

    // 发送给所有客户端
    QByteArray packetData(reinterpret_cast<const char*>(&packet), sizeof(packet));
    sendToAllClients(packetData);
}

void TcpServerManager::sendToAllClients(const QByteArray &data)
{
    QMutexLocker locker(&m_clientMutex);

    int successCount = 0;
    int failedCount = 0;

    for (QTcpSocket *client : qAsConst(m_clients)) {
        if (client->state() == QAbstractSocket::ConnectedState) {
            qint64 bytesWritten = client->write(data);
            if (bytesWritten == -1) {
                qWarning() << "Failed to send data to client" << client->objectName();
                failedCount++;
            } else {
                successCount++;
                m_totalBytesSent += bytesWritten;

                // 确保数据发送出去
                client->flush();
            }
        } else {
            failedCount++;
        }
    }

    if (successCount == m_clients.size()) {
        if (data != "HEARTBEAT"){
            qint64 elapsed = m_testtimer.elapsed();
            qDebug() << "eth-can test time:" <<elapsed;
        }
        emit dataSentToClients(successCount, data.size());
    }

    // 如果有失败的发送，记录日志
    if (failedCount > 0) {
        qDebug() << "Data sent to" << successCount << "clients,"
                << failedCount << "failed";
    }
}

void TcpServerManager::sendToClient(QTcpSocket *client, const QByteArray &data)
{
    if (!client || client->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    qint64 bytesWritten = client->write(data);
    if (bytesWritten == -1) {
        qWarning() << "Failed to send to client" << client->objectName();
    } else {
        m_totalBytesSent += bytesWritten;
        client->flush();
    }
}


