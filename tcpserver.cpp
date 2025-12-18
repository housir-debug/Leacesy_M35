#include "tcpserver.h"
#include "canworker.h"
#include <QNetworkInterface>
#include <QDateTime>
#include <QCoreApplication>
#include <QElapsedTimer>




TcpServerManager::TcpServerManager( QObject *parent)
    : QObject(parent)
{
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


    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, &QTcpServer::newConnection,
            this, &TcpServerManager::onNewConnection);

    qDebug() << "TcpServerManager created, port:" << m_port;
}

void TcpServerManager::sendToAllClients(const QByteArray &data)
{
    QMutexLocker locker(&m_Mutex);

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
                //client->flush();
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
    }else{
        qDebug() << "tcpsendclient failcount:" <<failedCount<< "clients,"<< successCount << "clients";
    }
}

void TcpServerManager::cleanupDisconnectedClients()
{
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
    if (!client) {return;}

    {
        QMutexLocker locker(&m_Mutex);
        if (m_clients.size() >= 10) {
            qWarning() << "Max client limit reached, rejecting connection from"
                      << client->peerAddress().toString();
            client->disconnectFromHost();
            delete client;
            return;
        }

        m_clients.append(client);
    }

    QString clientInfo = QString("%1:%2")
                        .arg(client->peerAddress().toString())
                        .arg(client->peerPort());

    client->setObjectName(clientInfo);

    connect(client, &QTcpSocket::readyRead,
            this, &TcpServerManager::onClientReadyRead);
    connect(client, &QTcpSocket::disconnected,
            this, &TcpServerManager::onClientDisconnected);
    connect(client, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &TcpServerManager::onSocketError);

    qInfo() << "New client connected:" << clientInfo
           << ", total clients:" << m_clients.size();;

    QByteArray welcome = QString("Welcome to Meacesy Server (Clients: %1)\n")
                        .arg(m_clients.size()).toUtf8();
    client->write(welcome);
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

void TcpServerManager::onClientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) {
        return;
    }

    QString clientInfo = client->objectName();

    {
        QMutexLocker locker(&m_Mutex);
        m_clients.removeOne(client);
    }

    client->deleteLater();

    qInfo() << "Client disconnected:" << clientInfo
           << ", remaining clients:" << m_clients.size();
}

void TcpServerManager::onClientReadyRead()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) {return;}

    QByteArray rawData = client->readAll();
    m_totalBytesReceived += rawData.size();
    qDebug() << "Received from" << client->objectName()<< "size:" << rawData.size()<< "hex:" << rawData.toHex();

    QTimer::singleShot(0, this, [this, client,rawData]() {
        // 异步处理，立即放入事件队列，不阻塞事件循环
        processClientData(client,rawData);
    });
}


void TcpServerManager::processClientData(QTcpSocket *client,const QByteArray newdata)
{
    qDebug() << "Processing buffer for" << client->objectName()<< "size:" << newdata.size();

    if (newdata.size() == static_cast<int>(sizeof(ControlPacket))) {//二进制控制指令包
            ControlPacket packet;
            memcpy(&packet, newdata.constData(), sizeof(packet));//复制到指定位置

            if (validatePacket(packet)) {
                handleCommand(client, packet);
            } else {
                qWarning() << "Invalid packet from" << client->objectName();
                QByteArray errorMsg = "ERROR: Invalid packet format\n";
                client->write(errorMsg);
            }
            return;
        }
    else if (newdata.trimmed().toUpper() == "HEARTBEAT") {// 文本格式的心跳包
            client->write("HEARTBEAT_RESPONSE\n");
            m_testtimer.start();
            //emit test();
            //quint32 testId = 0x321;
            const QByteArray &testData = QByteArray::fromHex("1122334455667788");
            emit SerialSendRequest(testData);
            //emit canSendRequest(testId, testData);
            return;
        }
    else if (newdata.trimmed().toUpper() == "STATUS") {// 文本格式的状态查询
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
            return;
        }
    else if(newdata.size() >= static_cast<int>(sizeof(ControlPacket))){
            qDebug() << "Data larger than packet size from" << client->objectName()
                             << "expected max:" << sizeof(ControlPacket)
                             << "actual:" << newdata.size();

            // 多个数据包
            if (newdata.size() % sizeof(ControlPacket) == 0) {
                int packetCount = newdata.size() / sizeof(ControlPacket);
                qDebug() << "Detected" << packetCount << "binary packets in stream";

                const char* dataPtr = newdata.constData();
                for (int i = 0; i < packetCount; i++) {
                    ControlPacket packet;
                    memcpy(&packet, dataPtr + i * sizeof(ControlPacket), sizeof(packet));

                    if (validatePacket(packet)) {
                        handleCommand(client, packet);
                    } else {
                        qWarning() << "Invalid packet" << i << "in batch from" << client->objectName();
                        client->write("ERROR: Batch packet validation failed\n");
                        break;
                    }
                }
                return;
            }

            // 二进制包后面跟了额外数据（如换行符）
            ControlPacket packet;
            memcpy(&packet, newdata.constData(), sizeof(packet));
            if (validatePacket(packet)) {
                qDebug() << "Valid packet found at beginning, extra data:"
                         << newdata.size() - sizeof(ControlPacket) << "bytes";

                // 处理有效的包
                handleCommand(client, packet);

                // 检查额外数据是否为文本命令（如换行符+HEARTBEAT）
                QByteArray extraData = newdata.mid(sizeof(ControlPacket));
                extraData = extraData.trimmed();  // 移除可能的换行符/空格

                if (!extraData.isEmpty()) {
                    qDebug() << "Processing extra data:" << extraData;
                    // 递归处理剩余数据（会进入文本命令分支）
                    processClientData(client, extraData);
                }
                return;
            }

            // 可能是文本命令带了参数或格式问题
            QByteArray trimmed = newdata.trimmed();
            QByteArray upperTrimmed = trimmed.toUpper();
            if (upperTrimmed.startsWith("HEARTBEAT")) {
                qDebug() << "HEARTBEAT command with extra data:" << trimmed;
                // 只响应基本心跳，忽略额外数据
                client->write("HEARTBEAT_RESPONSE\n");
                return;
            }
            else if (upperTrimmed.startsWith("STATUS")) {
                qDebug() << "STATUS command with extra data:" << trimmed;
                // 只响应状态查询，忽略额外参数
                QString status = QString(
                    "=== CAN TCP Server Status ===\n"
                    "Server State: %1\n"
                    "Port: %2\n"
                    "Clients: %3/%4\n"
                    "Bytes Sent: %5\n"
                    "Bytes Received: %6\n"
                    "Note: Extra parameters ignored\n"
                    "=============================\n"
                ).arg(m_running.load() ? "Running" : "Stopped")
                 .arg(m_port)
                 .arg(m_clients.size()).arg(10)
                 .arg(m_totalBytesSent.load())
                 .arg(m_totalBytesReceived.load());

                client->write(status.toUtf8());
                return;
            }

            // 情况D：未知的大数据格式
            qWarning() << "Unrecognized large data from" << client->objectName()
                      << "size:" << newdata.size() << "content: " << newdata;
            client->write("ERROR: Data format not recognized\n");
            return;
        }
    else{
            qWarning() << "Invalid brief inform!Throw!" ;
            return;
        }
}

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



void TcpServerManager::handleCommand(QTcpSocket *client, const ControlPacket &packet)
{
    if (!client || client->state() != QAbstractSocket::ConnectedState) {return;}

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
                "Port: %1\n"
                "Clients: %2/%3\n"
                "Bytes Sent: %4\n"
                "Bytes Received: %5\n"
                "Uptime: %6 seconds\n"
                "=============================\n"
            ).arg(m_port)
             .arg(m_clients.size()).arg(10)  // 当前连接数/最大连接数
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



TcpServerManager::~TcpServerManager()
{
    stopServer();
    qDebug() << "TcpServerManager destroyed";
}

void TcpServerManager::stopServer()
{
    QMutexLocker locker(&m_Mutex);

    if (m_state.load() == STATE_STOPPED) {return;}

    m_state.store(STATE_STOPPING);

    m_heartbeatTimer->stop();
    m_cleanupTimer->stop();
    delete m_heartbeatTimer;
    delete m_cleanupTimer;

    {
        for (QTcpSocket *client : qAsConst(m_clients)) {
            client->disconnectFromHost();
            if (client->state() == QAbstractSocket::ConnectedState) {
                client->waitForDisconnected(1000);
            }
        }
        m_clients.clear();

        if (m_tcpServer->isListening()) {
            m_tcpServer->close();
        }
    }

    if (m_serverThread && m_serverThread->isRunning()) {
        m_serverThread->quit();
        m_serverThread->wait(1000);// 等待1秒
        m_serverThread->deleteLater();
        delete m_serverThread;
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

    if (!m_serverThread){
        m_serverThread = new QThread(this);
        m_serverThread->setObjectName("TcpServer");
    }
    if (thread() != m_serverThread) {
        this->moveToThread(m_serverThread);
        m_tcpServer->moveToThread(m_serverThread);
    }
    if (!m_serverThread->isRunning()) {
        m_serverThread->start();
    }

    QMetaObject::invokeMethod(this, [this]() {
        if (m_tcpServer->listen(QHostAddress::Any, m_port)) {
            m_state.store(STATE_RUNNING);

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
            stopServer();
        }
    }, Qt::QueuedConnection);

    return true;
}


void TcpServerManager::forwardCanData(quint32 canId, const QByteArray &data, qint64 timestamp)
{
    if (m_state.load() != STATE_RUNNING) {return;}

    CanDataPacket packet;
    memset(&packet, 0, sizeof(packet));

    packet.magic = 0xCAFE;
    packet.length = sizeof(packet) - sizeof(packet.magic) - sizeof(packet.length);
    packet.timestamp = timestamp;

    packet.canId = canId;
    int dataSize = qMin(data.size(), 8);
    packet.data[0] = static_cast<quint8>(dataSize);
    memcpy(packet.data + 1, data.constData(), dataSize);

    QByteArray crcData(reinterpret_cast<const char*>(&packet.timestamp),
                      sizeof(packet) - offsetof(CanDataPacket, timestamp) - sizeof(packet.crc));
    packet.crc = crc16(reinterpret_cast<const quint8*>(crcData.constData()), crcData.size());

    QByteArray packetData(reinterpret_cast<const char*>(&packet), sizeof(packet));
    sendToAllClients(packetData);
}

void TcpServerManager::forwardSerialData(const QByteArray &data){
    sendToAllClients(data);
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
        //client->flush();
    }
}


