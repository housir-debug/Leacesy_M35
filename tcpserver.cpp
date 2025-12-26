#include "tcpserver.h"
#include "canworker.h"
#include <QNetworkInterface>
#include <QDateTime>
#include <QCoreApplication>
#include <QElapsedTimer>


Q_LOGGING_CATEGORY(tcp, "tcp:")
const QString TcpServerManager::SCPI_QUERY_SYMBOL = "?";

TcpServerManager::TcpServerManager( QObject *parent)
    : QObject(parent)
{
     initScpiCommandTree();
}

void TcpServerManager::initScpiCommandTree()
{
    m_scpiRoot = new ScpiNode("ROOT");

    // *IDN? 查询设备标识
    registerScpiCommand("*IDN",
        [this](const QStringList& args) { return handleScpiIdentify(args); },
        "Query device identification");

    // *RST 系统复位
    registerScpiCommand("*RST",
        [this](const QStringList& args) { return handleScpiSystemReset(args); },
        "Reset system to default state");

    // *CLS 清除状态
    registerScpiCommand("*CLS",
        [this](const QStringList& args) {
            Q_UNUSED(args);
            m_scpiErrors.clear();
            return "0";
        },
        "Clear status");

    // *ESR? 事件状态寄存器查询
    registerScpiCommand("*ESR",
        [this](const QStringList& /*args*/) {
            return QString::number(m_scpiErrors.isEmpty() ? 0 : 1);
        },
        "Event status register query");

    // NETWORK子系统
    registerScpiCommand("NETWork:INTERFace",
        [this](const QStringList& args) { return handleScpiNetworkInterface(args); },
        "Query network interfaces");

    registerScpiCommand("NETWork:IP",
        [](const QStringList& /*args*/) {
            QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
            QStringList ipList;
            for (const QHostAddress &address : qAsConst(addresses)) {
                if (address.protocol() == QAbstractSocket::IPv4Protocol &&
                    address != QHostAddress::LocalHost) {
                    ipList.append(address.toString());
                }
            }
            return ipList.join(",");
        },
        "Query IP addresses");

    qCInfo(tcp) << "SCPI command tree initialized with commands";
}

void TcpServerManager::registerScpiCommand(const QString& command,
                                          ScpiHandler handler,
                                          const QString& description)
{
    QStringList parts = command.split(':');
    ScpiNode* current = m_scpiRoot;

    for (const QString& part : qAsConst(parts)) {
        QString key = part.toUpper();
        if (!current->children.contains(key)) {
            current->children[key] = new ScpiNode(key);
        }
        current = current->children[key];
    }

    current->handler = handler;
    current->description = description;
}

void TcpServerManager::processScpiCommand(QTcpSocket* client, const QString& command)
{
    if (!m_scpiEnabled) {
        QString response = generateScpiResponse("ERROR:SCPI disabled");
        client->write(response.toUtf8());
        return;
    }

    QString trimmedCmd = command.trimmed();
    if (trimmedCmd.isEmpty()) {
        return;
    }

    qCDebug(tcp) << "Processing SCPI command:" << trimmedCmd
                 << "from" << client->objectName();

    QString result = executeScpiCommand(trimmedCmd);
    QString response = generateScpiResponse(result);

    client->write(response.toUtf8());
}

QString TcpServerManager::executeScpiCommand(const QString& command)
{
    QString cmd = command.trimmed().toUpper();
    bool isQuery = cmd.endsWith(SCPI_QUERY_SYMBOL);

    if (isQuery) {
        cmd = cmd.left(cmd.length() - 1); // 移除查询符号
    }

    // 检查是否为通用命令（以*开头）
    if (cmd.startsWith("*")) {
        // 处理通用SCPI命令
        if (cmd == "*IDN" && isQuery) {
            return handleScpiIdentify(QStringList());
        }
        else if (cmd == "*RST" && !isQuery) {
            return handleScpiSystemReset(QStringList());
        }
        else if (cmd == "*CLS" && !isQuery) {
            m_scpiErrors.clear();
            return "0";
        }
        else if (cmd == "*ESR" && isQuery) {
            return QString::number(m_scpiErrors.isEmpty() ? 0 : 1);
        }
        else {
            addScpiError(COMMAND_ERROR, QString("Unknown generic command: %1").arg(cmd));
            return "ERROR:Undefined header";
        }
    }

    // 解析层级命令
    QStringList parts = cmd.split(':');
    ScpiNode* current = m_scpiRoot;

    for (const QString& part : qAsConst(parts)) {
        QString key = part.toUpper();

        // 检查是否有精确匹配
        if (current->children.contains(key)) {
            current = current->children[key];
        }
        // 检查通配符匹配
        else {
            bool matched = false;
            for (const QString& pattern : current->children.keys()) {
                if (matchScpiPattern(pattern, key)) {
                    current = current->children[pattern];
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                addScpiError(COMMAND_ERROR, QString("Unknown command: %1").arg(cmd));
                return "ERROR:Undefined header";
            }
        }
    }

    if (!current->handler) {
        addScpiError(COMMAND_ERROR, QString("No handler for: %1").arg(cmd));
        return "ERROR:Undefined header";
    }

    // 执行命令
    try {
        return current->handler(QStringList());
    } catch (const std::exception& e) {
        addScpiError(EXECUTION_ERROR, QString("Execution error: %1").arg(e.what()));
        return QString("ERROR:Execution error: %1").arg(e.what());
    }
}

bool TcpServerManager::matchScpiPattern(const QString& pattern, const QString& command)
{
    // 支持简单的通配符匹配
    // 例如：PATtern 匹配 PAT, PATT, PATTERN
    if (pattern.length() > command.length()) {
        return false;
    }

    for (int i = 0; i < pattern.length(); ++i) {
        if (pattern[i] != command[i]) {
            return false;
        }
    }
    return true;
}

QString TcpServerManager::generateScpiResponse(const QString& response)
{
    if (response.startsWith("ERROR:")) {
        return response + "\n";
    } else {
        return response + "\n";
    }
}

void TcpServerManager::addScpiError(int code, const QString& message)
{
    m_scpiErrors.append(qMakePair(code, message));

    // 保持错误队列大小
    if (m_scpiErrors.size() > 10) {
        m_scpiErrors.removeFirst();
    }

    qCWarning(tcp) << "SCPI Error [" << code << "]: " << message;
}

QString TcpServerManager::getScpiErrors()
{
    if (m_scpiErrors.isEmpty()) {
        return "0,\"No error\"";
    }

    auto error = m_scpiErrors.takeLast();
    return QString("%1,\"%2\"")
           .arg(error.first)
           .arg(error.second);
}


QString TcpServerManager::handleScpiIdentify(const QStringList& args)
{
    Q_UNUSED(args);

    QString manufacturer = "YourCompany";
    QString model = "RK3568-CAN-Gateway";
    QString serial = "SN-001";
    QString firmware = "1.0.0";

    return QString("%1,%2,%3,%4")
           .arg(manufacturer, model, serial, firmware);
}

QString TcpServerManager::handleScpiSystemReset(const QStringList& args)
{
    Q_UNUSED(args);

    // 停止服务器
    stopServer();

    // 清空数据
    m_totalBytesSent.store(0);
    m_totalBytesReceived.store(0);
    m_scpiErrors.clear();

    // 重新启动
    QTimer::singleShot(1000, this, [this]() {
        startServer();
    });

    return "0";
}

QString TcpServerManager::handleScpiNetworkInterface(const QStringList& args)
{
    Q_UNUSED(args);

    QStringList interfaces;
    QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();

    for (const QNetworkInterface &interface :  qAsConst(allInterfaces)) {
        if (interface.flags() & QNetworkInterface::IsUp &&
            !(interface.flags() & QNetworkInterface::IsLoopBack)) {
            QString info = QString("%1|%2|%3")
                          .arg(interface.name(),interface.humanReadableName(),interface.hardwareAddress());
            interfaces.append(info);
        }
    }

    return interfaces.join(";");
}







bool TcpServerManager::startServer()
{
    if (m_state.load() != STATE_STOPPED) {
        qCWarning(tcp) << "TcpServer is not in stopped state";
        return false;
    }

    m_state.store(STATE_STARTING);

    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setInterval(30000); // 30秒心跳
    m_cleanupTimer = new QTimer(this);
    m_cleanupTimer->setInterval(5000); // 5秒清理一次
    m_tcpServer = new QTcpServer(this);

    if (!m_serverThread){
        m_serverThread = new QThread(this);
        m_serverThread->setObjectName("TcpServer");
    }
    if (thread() != m_serverThread) {
        this->moveToThread(m_serverThread);
        m_tcpServer->moveToThread(m_serverThread);
        m_heartbeatTimer->moveToThread(m_serverThread);
        m_cleanupTimer->moveToThread(m_serverThread);
    }
    if (!m_serverThread->isRunning()) {
        m_serverThread->start();
    }

    connect(m_heartbeatTimer, &QTimer::timeout, this, [this]() {
        QByteArray heartbeat;
        heartbeat.append("HEARTBEAT");
        sendToAllClients(heartbeat);
    });
    connect(m_cleanupTimer, &QTimer::timeout,
            this, &TcpServerManager::cleanupDisconnectedClients);
    connect(m_tcpServer, &QTcpServer::newConnection,
            this, &TcpServerManager::onNewConnection);

    QMetaObject::invokeMethod(this, [this]() {
        if (m_tcpServer->listen(QHostAddress::Any, m_port)) {
            m_state.store(STATE_RUNNING);

            m_heartbeatTimer->start();
            m_cleanupTimer->start();

            qCInfo(tcp) << "TcpServer started on port" << m_port
                   << ", thread:" << QThread::currentThread()->objectName();
        } else {
            m_state.store(STATE_STOPPED);
            QString error = QString("Failed to start TCP server: %1")
                           .arg(m_tcpServer->errorString());
            qCCritical(tcp) << error;
            emit errorOccurred(error);
            stopServer();
        }
    }, Qt::QueuedConnection);

    return true;
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
                qCWarning(tcp) << "Failed to send data to client" << client->objectName();
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
            qCDebug(tcp) << "eth-can test time:" <<elapsed;
        }
    }else{
        qCDebug(tcp) << "tcpsendclient failcount:" <<failedCount<< "clients,"<< successCount << "clients";
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
        qCDebug(tcp) << "Cleaned up disconnected client:" << client->objectName();
    }
}

void TcpServerManager::onNewConnection()
{
    QTcpSocket *client = m_tcpServer->nextPendingConnection();
    if (!client) {return;}

    {
        QMutexLocker locker(&m_Mutex);
        if (m_clients.size() >= 10) {
            qCWarning(tcp) << "Max client limit reached, rejecting connection from"
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

    qCInfo(tcp) << "New client connected:" << clientInfo
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

    qCWarning(tcp) << "Socket error from" << client->objectName()
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

    qCInfo(tcp) << "Client disconnected:" << clientInfo
           << ", remaining clients:" << m_clients.size();
}

void TcpServerManager::onClientReadyRead()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) {return;}

    QByteArray rawData = client->readAll();
    m_totalBytesReceived += rawData.size();
    qCDebug(tcp) << "Received from" << client->objectName()<< "size:" << rawData.size()<< "hex:" << rawData.toHex();

    QTimer::singleShot(0, this, [this, client,rawData]() {
        // 异步处理，立即放入事件队列，不阻塞事件循环
        processClientData(client,rawData);
    });
}


void TcpServerManager::processClientData(QTcpSocket *client,const QByteArray newdata)
{
    qCDebug(tcp) << "Processing buffer for" << client->objectName()<< "size:" << newdata.size();

    // 检查是否是SCPI命令（文本格式）
    QString dataStr = QString::fromUtf8(newdata).trimmed();

    // SCPI命令通常以*或大写字母开头
    if (m_scpiEnabled && !dataStr.isEmpty() &&
        (dataStr.startsWith('*') ||
         (dataStr.length() >= 2 && dataStr[0].isUpper() && dataStr[1].isUpper()))) {

        // 处理SCPI命令
        processScpiCommand(client, dataStr);
        return;
    }

    if (newdata.size() == static_cast<int>(sizeof(ControlPacket))) {//二进制控制指令包
            ControlPacket packet;
            memcpy(&packet, newdata.constData(), sizeof(packet));//复制到指定位置

            if (validatePacket(packet)) {
                handleCommand(client, packet);
            } else {
                qCWarning(tcp) << "Invalid packet from" << client->objectName();
                QByteArray errorMsg = "ERROR: Invalid packet format\n";
                client->write(errorMsg);
            }
            return;
        }
    else if (newdata.trimmed().toUpper() == "HEARTBEAT") {// 文本格式的心跳包
            client->write("HEARTBEAT_RESPONSE\n");
            m_testtimer.start();
            //emit test();
            quint32 testId = 0x321;
            const QByteArray &testData = QByteArray::fromHex("1122334455667788");
            //emit SerialSendRequest(testData);
            emit canSendRequest(testId, testData);
            return;
        }
    else if (newdata.trimmed().toUpper() == "STATUS") {// 文本格式的状态查询
            QString status = QString(
                "=== CAN TCP Server Status ===\n"
                "Port: %1\n"
                "Clients: %2/%3\n"
                "Bytes Sent: %4\n"
                "Bytes Received: %5\n"
                "=============================\n"
            ).arg(m_port)
             .arg(m_clients.size()).arg(10)  // 当前连接数/最大连接数
             .arg(m_totalBytesSent.load())
             .arg(m_totalBytesReceived.load());

            client->write(status.toUtf8());
            return;
        }
    else if(newdata.size() >= static_cast<int>(sizeof(ControlPacket))){
            qCDebug(tcp) << "Data larger than packet size from" << client->objectName()
                             << "expected max:" << sizeof(ControlPacket)
                             << "actual:" << newdata.size();

            // 多个数据包
            if (newdata.size() % sizeof(ControlPacket) == 0) {
                int packetCount = newdata.size() / sizeof(ControlPacket);
                qCDebug(tcp) << "Detected" << packetCount << "binary packets in stream";

                const char* dataPtr = newdata.constData();
                for (int i = 0; i < packetCount; i++) {
                    ControlPacket packet;
                    memcpy(&packet, dataPtr + i * sizeof(ControlPacket), sizeof(packet));

                    if (validatePacket(packet)) {
                        handleCommand(client, packet);
                    } else {
                        qCWarning(tcp) << "Invalid packet" << i << "in batch from" << client->objectName();
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
                qCDebug(tcp) << "Valid packet found at beginning, extra data:"
                         << newdata.size() - sizeof(ControlPacket) << "bytes";

                // 处理有效的包
                handleCommand(client, packet);

                // 检查额外数据是否为文本命令（如换行符+HEARTBEAT）
                QByteArray extraData = newdata.mid(sizeof(ControlPacket));
                extraData = extraData.trimmed();  // 移除可能的换行符/空格

                if (!extraData.isEmpty()) {
                    qCDebug(tcp) << "Processing extra data:" << extraData;
                    // 递归处理剩余数据（会进入文本命令分支）
                    processClientData(client, extraData);
                }
                return;
            }

            // 可能是文本命令带了参数或格式问题
            QByteArray trimmed = newdata.trimmed();
            QByteArray upperTrimmed = trimmed.toUpper();
            if (upperTrimmed.startsWith("HEARTBEAT")) {
                qCDebug(tcp) << "HEARTBEAT command with extra data:" << trimmed;
                // 只响应基本心跳，忽略额外数据
                client->write("HEARTBEAT_RESPONSE\n");
                return;
            }
            else if (upperTrimmed.startsWith("STATUS")) {
                qCDebug(tcp) << "STATUS command with extra data:" << trimmed;
                // 只响应状态查询，忽略额外参数
                QString status = QString(
                    "=== CAN TCP Server Status ===\n"
                    "Port: %1\n"
                    "Clients: %2/%3\n"
                    "Bytes Sent: %4\n"
                    "Bytes Received: %5\n"
                    "Note: Extra parameters ignored\n"
                    "=============================\n"
                ).arg(m_port)
                 .arg(m_clients.size()).arg(10)
                 .arg(m_totalBytesSent.load())
                 .arg(m_totalBytesReceived.load());

                client->write(status.toUtf8());
                return;
            }

            // 情况D：未知的大数据格式
            qCWarning(tcp) << "Unrecognized large data from" << client->objectName()
                      << "size:" << newdata.size() << "content: " << newdata;
            client->write("ERROR: Data format not recognized\n");
            return;
        }
    else{
            qCWarning(tcp) << "Invalid brief inform!Throw!" ;
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
        qCWarning(tcp) << "Invalid magic header:" << QString::number(packet.magic, 16);
        return false;
    }

    // 检查数据长度
    if (packet.dataLength > 8) {
        qCWarning(tcp) << "Invalid data length:" << packet.dataLength;
        return false;
    }

    // 计算并验证CRC（从command开始到data结束）
    QByteArray crcData(reinterpret_cast<const char*>(&packet.command),
                      sizeof(packet) - offsetof(ControlPacket, command) - sizeof(packet.crc));
    quint16 calculatedCrc = crc16(reinterpret_cast<const quint8*>(crcData.constData()), crcData.size());

    if (calculatedCrc != packet.crc) {
        qCWarning(tcp) << "CRC mismatch: expected" << QString::number(packet.crc, 16)
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
            qCDebug(tcp) << "Heartbeat from" << client->objectName();
            QByteArray response;
            response.append("HEARTBEAT_OK\n");
            client->write(response);
            break;
        }

        case 0x02: {// 发送CAN帧
            QByteArray canData(reinterpret_cast<const char*>(packet.data), packet.dataLength);
            qCDebug(tcp) << "CAN send request from" << client->objectName()
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
            qCDebug(tcp) << "Status query from" << client->objectName();
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
        qCWarning(tcp) << "Unknown command type:" << packet.command
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
    qCDebug(tcp) << "TcpServerManager destroyed";
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
    qCInfo(tcp) << "TcpServer stopped";
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
    if (!client || client->state() != QAbstractSocket::ConnectedState) {return;}

    qint64 bytesWritten = client->write(data);
    if (bytesWritten == -1) {
        qCWarning(tcp) << "Failed to send to client" << client->objectName();
    } else {
        m_totalBytesSent += bytesWritten;
        //client->flush();
    }
}


// ============================================================================
// 调用示例
/*
#include "tcpserver.h"

void TcpManager(CanWorker *canWorker)
{
    TcpServerManager *tcpServer = new TcpServerManager();

    tcpServer->startServer();

    QObject::connect(canWorker, &CanWorker::frameReceived,
                     tcpServer, &TcpServerManager::forwardCanData,
                     Qt::QueuedConnection);
    QObject::connect(tcpServer, &TcpServerManager::canSendRequest,
                     canWorker, &CanWorker::sendFrame,
                     Qt::QueuedConnection);


    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                     tcpServer, &TcpServerManager::stopServer,
                     Qt::QueuedConnection);
    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                     tcpServer, &QObject::deleteLater,
                     Qt::QueuedConnection);
}
*/

