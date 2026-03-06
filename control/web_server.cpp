#include "web_server.h"
#include "auxiliary/config_manager.h"
#include <QTcpSocket>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>

Q_LOGGING_CATEGORY(web, "WEB:")

WebServer::WebServer(QObject *parent) : QObject(parent){}
WebServer::~WebServer(){
    if (m_httpServer) {
        m_httpServer->close();
        delete m_httpServer;
    }

    if (m_wsServer) {
        m_wsServer->close();
        delete m_wsServer;
    }

    for (auto it = m_wsClients.begin(); it != m_wsClients.end(); ++it) {
        delete it.key();
    }
    m_wsClients.clear();
}

bool WebServer::start(int httpPort, int wsPort){
    m_httpServer = new QTcpServer(this);
    if (!m_httpServer->listen(QHostAddress::Any, httpPort)) {
        qCWarning(web) << "Failed to start HTTP server:" << m_httpServer->errorString();
        delete m_httpServer;
        m_httpServer = nullptr;
        return false;
    }
    connect(m_httpServer, &QTcpServer::newConnection,this, &WebServer::onHttpNewConnection,Qt::DirectConnection);
    qCDebug(web) << "HTTP server started on port" << httpPort;

    m_wsServer = new QWebSocketServer("Leacesy_Instrument",QWebSocketServer::NonSecureMode, this);
    if (!m_wsServer->listen(QHostAddress::Any, wsPort)) {
        qCWarning(web) << "Failed to start WebSocket server:" << m_wsServer->errorString();
        delete m_wsServer;
        m_wsServer = nullptr;
        return false;
    }
    connect(m_wsServer, &QWebSocketServer::newConnection,this, &WebServer::onWsNewConnection,Qt::DirectConnection);
    qCDebug(web) << "WebSocket server started on port" << wsPort;

    return true;
}

// ===================== HTTP处理 =====================

void WebServer::onHttpNewConnection(){
    QTcpSocket *client = m_httpServer->nextPendingConnection();
    if (!client) return;

    qCDebug(web) << "New HTTP client:" << client->peerAddress().toString()<< ":" << client->peerPort();

    connect(client, &QTcpSocket::errorOccurred,this, [client](QAbstractSocket::SocketError error){
        qCWarning(web)<<client->objectName()<<"HTTP  Socket Error: ["<<error<<"]"<<client->errorString();
    }, Qt::DirectConnection);
    connect(client, &QTcpSocket::disconnected,this, [this, client](){
        qCDebug(web)<<client->objectName()<<"HTTP  Disconnected, Remain Connect Clients: "<<m_httpBuffers.size();
        m_httpBuffers.remove(client);
        client->deleteLater();
    }, Qt::DirectConnection);
    connect(client, &QTcpSocket::readyRead,this, [this, client](){ handleHttpRequest(client);}, Qt::DirectConnection);

    m_httpBuffers[client] = QByteArray();
}

void WebServer::handleHttpRequest(QTcpSocket *client){
    QMutexLocker locker(&m_httpmutex);

    m_httpBuffers[client].append(client->readAll());
    if (m_httpBuffers[client].isEmpty()){return;}

    if (!m_httpBuffers[client].contains("\r\n\r\n")) {
        sendHttpResponse(client, "WebServer is running", "text/plain");
        m_httpBuffers.remove(client);
        return;
    }

    QString requestStr = QString::fromUtf8(m_httpBuffers[client]);
    QStringList lines = requestStr.split("\r\n");
    QString requestLine = lines[0];
    QStringList parts = requestLine.split(" ");
    if (lines.isEmpty() || parts.size() < 3) {
        sendHttpResponse(client, "Bad Request", "text/plain", 400);
        m_httpBuffers.remove(client);
        return;
    }

    QString method = parts[0];
    QString path = QUrl::fromPercentEncoding(parts[1].toUtf8());

    qCDebug(web) << "HTTP request:" << method << path;
    if (path.startsWith("/api/")) {
        handleApiRequest(client, path);
        m_httpBuffers.remove(client);
        return;
    }

    serveResourceFile(client, path);
    m_httpBuffers.remove(client);
    return;
}

void WebServer::handleApiRequest(QTcpSocket *client, const QString &path)
{
    QJsonObject response;
    if (path == "/api/channels") {
        QJsonArray channels;

        for (auto it = m_channelData.begin(); it != m_channelData.end(); ++it) {
            QJsonObject channel;
            channel["channel"] = it.key();
            channel["voltage"] = it.value().voltage;
            channel["current"] = it.value().current;
            channel["status"] = it.value().status;
            channels.append(channel);
        }

        response["channels"] = channels;
        response["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    }
    else if (path == "/api/scpi_commands") {
        QJsonArray commands = {
            "MEASure:VOLTage? (@1)",
            "MEASure:CURRent? (@1)",
            "MEASure:POWer? (@1)",
            "CONFigure:VOLTage:DC 10,(@1)",
            "OUTPut:STATe ON,(@1)",
            "OUTPut:STATe OFF,(@1)",
            "SYSTem:ERRor?",
            "*IDN?",
            "*RST",
            "STATus:QUEStionable?",
            "DISPlay:ENABle ON",
            "DISPlay:ENABle OFF"
        };

        response["commands"] = commands;
    }
    else if (path == "/api/device/info") {
        response["model"] = ConfigManager::s_model;
        response["serialNumber"] = ConfigManager::s_serialNumber;
        response["firmwareVersion"] = ConfigManager::s_firmwareVersion;
        response["uptime"] = QDateTime::currentDateTime().toString("HH:mm:ss");
    }
    else {
        response["error"] = "API endpoint not found";
        response["code"] = 404;
    }

    QByteArray jsonData = QJsonDocument(response).toJson();
    sendHttpResponse(client, jsonData, "application/json");
}

void WebServer::serveResourceFile(QTcpSocket *client, const QString &path)
{
    QString resourcePath;

    if (path == "/" || path == "/index.html") {
        resourcePath = ":/web/web/index.html";
    } else if (path == "/scpi.html") {
        resourcePath = ":/web/web/scpi.html";
    } else if (path == "/channels.html") {
        resourcePath = ":/web/web/channels.html";
    } else if (path == "/css/style.css") {
        resourcePath = ":/web/web/css/style.css";
    } else if (path == "/js/app.js") {
        resourcePath = ":/web/web/js/app.js";
    } else if (path == "/js/scpi.js") {
        resourcePath = ":/web/web/js/scpi.js";
    } else if (path == "/js/channels.js") {
        resourcePath = ":/web/web/js/channels.js";
    } else {
        sendHttpResponse(client, "404 Not Found", "text/plain", 404);
        return;
    }

    QByteArray content = loadResourceFile(resourcePath);
    if (content.isEmpty()) {
        sendHttpResponse(client, "500 Internal Server Error", "text/plain", 500);
        return;
    }

    QString suffix = QFileInfo(resourcePath).suffix();
    QString contentType = getMimeType(suffix);
    qCDebug(web)<<"dangqianluj"<< resourcePath;
    qCDebug(web)<<"houzhui"<< suffix;
    sendHttpResponse(client, content, contentType);
}

QByteArray WebServer::loadResourceFile(const QString &path)
{
    if (m_fileCache.contains(path)) {
        return m_fileCache[path];
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }

    QByteArray content = file.readAll();
    file.close();

    m_fileCache[path] = content;
    return content;
}

QString WebServer::getMimeType(const QString &suffix){
    static QMap<QString, QString> mimeTypes = {
        {"html", "text/html"},
        {"htm", "text/html"},
        {"css", "text/css"},
        {"js", "application/javascript"},
        {"json", "application/json"},
        {"png", "image/png"},
        {"jpg", "image/jpeg"},
        {"jpeg", "image/jpeg"},
        {"gif", "image/gif"},
        {"svg", "image/svg+xml"},
        {"ico", "image/x-icon"},
        {"txt", "text/plain"},
        {"xml", "application/xml"},
        {"pdf", "application/pdf"}
    };

    return mimeTypes.value(suffix.toLower(), "application/octet-stream");
}

void WebServer::sendHttpResponse(QTcpSocket *client, const QByteArray &content,const QString &contentType, int statusCode){
    QString statusText;
    switch (statusCode) {
        case 200: statusText = "OK"; break;
        case 204: statusText = "No Content"; break;
        case 400: statusText = "Bad Request"; break;
        case 404: statusText = "Not Found"; break;
        case 405: statusText = "Method Not Allowed"; break;
        case 500: statusText = "Internal Server Error"; break;
        default: statusText = "OK"; break;
    }

    QString header = QString(
        "HTTP/1.1 %1 %2\r\n"
        "Content-Type: %3; charset=utf-8\r\n"
        "Content-Length: %4\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n"
    ).arg(QString::number(statusCode),statusText,contentType,QString::number(content.size()));

    client->write(header.toUtf8());
    client->write(content);
    //client->flush();
    client->close();
}

// ===================== WebSocket处理 =====================

void WebServer::onWsNewConnection()
{
    QWebSocket *socket = m_wsServer->nextPendingConnection();
    if (!socket) return;

    qCDebug(web) << "WebSocket client connected:" << socket->peerAddress().toString();

    connect(socket, &QWebSocket::disconnected,this, [this, socket](){
        qCDebug(web)<<socket->objectName()<<"HTTP  Disconnected, Remain Connect Clients: "<<m_httpBuffers.size();
        m_wsClients.remove(socket);
        socket->deleteLater();
    }, Qt::DirectConnection);
    connect(socket, &QWebSocket::textMessageReceived,this, [this, socket](const QString &message){ onWsTextMessageReceived(socket,message);}, Qt::DirectConnection);

    QJsonObject welcome;
    welcome["type"] = "connection";
    welcome["message"] = "Connected to instrument WebSocket";
    socket->sendTextMessage(QJsonDocument(welcome).toJson());

    QJsonObject initialData;
    initialData["type"] = "initial_data";
    QJsonArray channels;

    {
        QMutexLocker locker(&m_webmutex);
        m_wsClients[socket] = socket->peerAddress().toString();

        for (auto it = m_channelData.begin(); it != m_channelData.end(); ++it) {
            QJsonObject channel;
            channel["channel"] = it.key();
            channel["voltage"] = it.value().voltage;
            channel["current"] = it.value().current;
            channel["status"] = it.value().status;
            channels.append(channel);
        }
    }

    initialData["channels"] = channels;
    socket->sendTextMessage(QJsonDocument(initialData).toJson());
}

void WebServer::onWsTextMessageReceived(QWebSocket *socket,const QString &message)
{
    QMutexLocker locker(&m_webmutex);

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull()){ return;}

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    if (type == "scpi_command") {
        QString command = obj["command"].toString();
        qCDebug(web) << "SCPI command received:" << command;

        QString result = executeScpiCommand(command);

        QJsonObject response;
        response["type"] = "scpi_response";
        response["command"] = command;
        response["result"] = result;
        response["status"] = "success";

        socket->sendTextMessage(QJsonDocument(response).toJson());
    }
    else if (type == "subscribe_channels") {
        QJsonArray channels = obj["channels"].toArray();
        // 处理订阅逻辑
    }
}

QString WebServer::executeScpiCommand(const QString &command)
{
    // 这里实现实际的SCPI命令处理
    // 示例实现
    if (command == "*IDN?") {
        return QString("%1,%2,%3").arg(ConfigManager::s_model)
                                  .arg(ConfigManager::s_serialNumber)
                                  .arg(ConfigManager::s_firmwareVersion);
    }
    else if (command.startsWith("MEASure:VOLTage?")) {
        // 返回第一个通道的电压
        if (!m_channelData.isEmpty()) {
            return QString::number(m_channelData.first().voltage);
        }
        return "0.0";
    }
    else if (command == "*RST") {
        // 复位设备
        return "OK";
    }

    return "Command executed";
}

void WebServer::updateChannelData(int channel, double voltage, double current,
                                  const QString& status)
{
    {
        QMutexLocker locker(&m_webmutex);

        ChannelData data;
        data.voltage = voltage;
        data.current = current;
        data.status = status;
        m_channelData[channel] = data;
    }

    // Broadcast update
    QJsonObject update;
    update["type"] = "channel_update";
    update["channel"] = channel;
    update["voltage"] = voltage;
    update["current"] = current;
    update["status"] = status;
    update["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    QByteArray jsonData = QJsonDocument(update).toJson();

    for (auto client : m_wsClients.keys()) {
        client->sendTextMessage(jsonData);
    }
}
