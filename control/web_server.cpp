#include "web_server.h"
#include "auxiliary/config_manager.h"
#include <QtCore>
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

WebServer::WebServer(ScpiManager* scpi,SerialBridge* qml,QObject *parent):
    QObject(parent),m_scpiManager(scpi), m_qmlbridge(qml){}
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

bool WebServer::start(){
    if (!m_webThread){
        m_httpServer = new QTcpServer(this);
        m_wsServer = new QWebSocketServer("Leacesy_Instrument",QWebSocketServer::NonSecureMode, this);

        m_webThread = new QThread(this);
        m_webThread->setObjectName("TcpServer");
    }

    if (thread() != m_webThread) {
        this->moveToThread(m_webThread);
        m_httpServer->moveToThread(m_webThread);
        m_wsServer->moveToThread(m_webThread);
    }

    if (!m_webThread->isRunning()) {
        m_webThread->start();

        connect(m_httpServer, &QTcpServer::newConnection,this, &WebServer::onHttpNewConnection,Qt::DirectConnection);
        connect(m_wsServer, &QWebSocketServer::newConnection,this, &WebServer::onWsNewConnection,Qt::DirectConnection);

        QMetaObject::invokeMethod(this, [this]() {
            if (!m_httpServer->listen(QHostAddress::Any, httpPort)) {
                qCWarning(web) << "Failed to start HTTP server:" << m_httpServer->errorString();
                delete m_httpServer;
                m_httpServer = nullptr;
            }

            if (!m_wsServer->listen(QHostAddress::Any, wsPort)) {
                qCWarning(web) << "Failed to start WebSocket server:" << m_wsServer->errorString();
                delete m_wsServer;
                m_wsServer = nullptr;
            }
        }, Qt::QueuedConnection);

        return true;
    }
    return false;
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
    if (path == "/api/device/info") {
        response["model"] = ConfigManager::s_model;
        response["serial"] = ConfigManager::s_serialNumber;
        response["software"] = ConfigManager::s_firmwareVersion;
        response["hardware"] = ConfigManager::s_hardwareVersion;
    }
    else if (path == "/api/scpi_commands") {
        int i = 0;
        QJsonArray commands;

        while (true) {
            const char* pattern = ScpiManager::m_scpiCommands[i].pattern;
            if (pattern == nullptr || strlen(pattern) == 0) {
                break; // End loop
            }

            QString cmd = QString::fromLatin1(pattern);
            commands.append(cmd);
            i++;
        }

        response["commands"] = commands;
    }
    else if(path == "/api/channels") {
        response["channels"] = m_qmlbridge->getAllChannelsData();
    }
    else if(path == "/api/models") {
        response["models"] = getModelsInfo();
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
    // html
    if (path == "/" || path == "/index.html") {
        resourcePath = ":/web/web/index.html";
    } else if (path == "/channels.html") {
        resourcePath = ":/web/web/channels.html";
    } else if (path == "/import.html") {
        resourcePath = ":/web/web/import.html";
    // js
    } else if (path == "/js/index.js") {
        resourcePath = ":/web/web/js/index.js";
    } else if (path == "/js/channels.js") {
        resourcePath = ":/web/web/js/channels.js";
    } else if (path == "/js/import.js") {
        resourcePath = ":/web/web/js/import.js";
    } else if (path == "/js/header.js") {
        resourcePath = ":/web/web/js/header.js";
    // css
    } else if (path == "/css/style.css") {
        resourcePath = ":/web/web/css/style.css";
    } else if (path == "/css/index.css") {
        resourcePath = ":/web/web/css/index.css";
    } else if (path == "/css/channels.css") {
        resourcePath = ":/web/web/css/channels.css";
    } else if (path == "/css/import.css") {
        resourcePath = ":/web/web/css/import.css";
    } else if (path == "/css/global.css") {
        resourcePath = ":/web/web/css/global.css";
    // icon
    } else if (path == "/icon/leacesylogo.png") {
        resourcePath = ":/web/web/icon/leacesylogo.png";
    } else if (path == "/icon/leacesyicon.png") {
        resourcePath = ":/web/web/icon/leacesyicon.png";
    // error
    } else {
        sendHttpResponse(client, "404 Not Found", "text/plain", 404);
        return;
    }

    QByteArray content;
    if (m_fileCache.contains(path)) {
        content = m_fileCache[path];
    }else{
        QFile file(resourcePath);
        if (!file.open(QIODevice::ReadOnly)) {
            content = QByteArray();
        }

        content = file.readAll();
        file.close();
        m_fileCache[path] = content;
    }

    if (content.isEmpty()) {
        sendHttpResponse(client, "500 Internal Server Error", "text/plain", 500);
        return;
    }

    QString suffix = QFileInfo(resourcePath).suffix();
    QString contentType = getMimeType(suffix);
    sendHttpResponse(client, content, contentType);
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

    connect(socket,QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),this, [socket](QAbstractSocket::SocketError error){
        qCWarning(web)<<socket->objectName()<<"HTTP  Socket Error: ["<<error<<"]"<<socket->errorString();
    }, Qt::DirectConnection);
    connect(socket, &QWebSocket::disconnected,this, [this, socket](){
        qCDebug(web)<<socket->objectName()<<"HTTP  Disconnected, Remain Connect Clients: "<<m_httpBuffers.size();
        m_wsClients.remove(socket);
        socket->deleteLater();
    }, Qt::DirectConnection);
    connect(socket, &QWebSocket::textMessageReceived,this, [this, socket](const QString &message){ onWsTextMessageReceived(socket,message);}, Qt::DirectConnection);

    m_wsClients[socket] = socket->peerAddress().toString();

    if (m_wsClients.size() > 1) {
        qCDebug(web) << "Maximum clients reached, disconnecting oldest connection";

        QWebSocket *oldestSocket = m_wsClients.firstKey();
        if (oldestSocket) {
            oldestSocket->sendTextMessage("{\"type\":\"system\",\"message\":\"Connection limit reached, disconnecting\"}");
            oldestSocket->close(QWebSocketProtocol::CloseCodeGoingAway, "Maximum clients limit reached");
            m_wsClients.remove(oldestSocket);
            oldestSocket->deleteLater();
        }
    }
}

void WebServer::onWsTextMessageReceived(QWebSocket *socket,const QString &message)
{
    QMutexLocker locker(&m_webmutex);

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        qCWarning(web) << "Invalid JSON message received";
        return;
    }

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();
    m_responsebuffer.clear();

    if (type == "scpi_command") {
        QString cmdString = obj["command"].toString();
        QByteArray cmd = (cmdString+"\n").toUtf8();
        qCDebug(web) << "SCPI command received:" << cmd;

        m_qmlbridge->update_remotemodel(true);
        m_responsebuffer = m_scpiManager->processCommand(cmd);
        if (m_responsebuffer.isEmpty()){return;}

        QJsonObject response;
        response["type"] = "scpi_response";
        response["result"] = QString::fromUtf8(m_responsebuffer);
        socket->sendTextMessage(QJsonDocument(response).toJson());
    }
    else if (type == "channels_update") {
        QJsonObject channelData;
        channelData["type"] = "channels_response";
        channelData["channels"] = m_qmlbridge->getAllChannelsData();
        socket->sendTextMessage(QJsonDocument(channelData).toJson());
    }
    else if (type == "model_upload") {
        QJsonObject content = obj["content"].toObject();
        QString modelName = content["name"].toString();
        QJsonArray modelData = content["data"].toArray();
        bool success = addModelFromNetwork(modelName,modelData);

        if (success) {
            QJsonObject syncMessage;
            syncMessage["type"] = "model_sync";
            syncMessage["models"] = getModelsInfo();
            socket->sendTextMessage(QJsonDocument(syncMessage).toJson());
        } else {
            QJsonObject response;
            response["type"] = "error";
            response["message"] = QString("Failed to save model \"%1\"").arg(modelName);
            socket->sendTextMessage(QJsonDocument(response).toJson());
        }
    }
    else if (type == "model_delete") {
        QString modelName = obj["name"].toString();
        bool success = m_qmlbridge->m_modelManager->removeModel(modelName);

        if (success) {
            QJsonObject syncMessage;
            syncMessage["type"] = "model_sync";
            syncMessage["models"] = getModelsInfo();
            socket->sendTextMessage(QJsonDocument(syncMessage).toJson());
        } else {
            QJsonObject response;
            response["type"] = "error";
            response["message"] = QString("Failed to delete model \"%1\"").arg(modelName);
            socket->sendTextMessage(QJsonDocument(response).toJson());
        }
    }
}

bool WebServer::addModelFromNetwork(const QString &modelName, const QJsonArray &modelData) {
    if (modelName.isEmpty()) {
        qWarning() << "addModelFromNetwork: 模型名称不能为空";
        return false;
    }

    if (m_qmlbridge->m_modelManager->m_models.contains(modelName)) {
        qWarning() << "addModelFromNetwork: 模型已存在";
        return false;
    }

    auto model = QSharedPointer<BatteryModel>::create();
    model->name = modelName;

    for (const auto &item : modelData) {
        if (!item.isObject() || item.isNull()) {
            qWarning() << "createModelFromJson: 数据项不是JSON对象,或者有一行为空";
            return false;
        }

        QJsonObject pointObj = item.toObject();
        if (!pointObj.contains("soc") || !pointObj.contains("ocv") || !pointObj.contains("imp")) {
            qWarning() << "createModelFromJson: 缺少必需字段(soc/ocv/imp)";
            return false;
        }

        BatteryDataPoint point;
        point.soc = pointObj["soc"].toDouble();
        point.ocv = pointObj["ocv"].toDouble();
        point.imp = pointObj["ocv"].toDouble();
        model->data_points.append(point);
    }

    std::sort(model->data_points.begin(), model->data_points.end(),
              [](const BatteryDataPoint &a, const BatteryDataPoint &b) {
                  return a.soc < b.soc;
              });

    if (!m_qmlbridge->m_modelManager->saveModel(model, modelName)) {
        // 如果保存失败，从内存中移除已添加的模型
        m_qmlbridge->m_modelManager->m_models.remove(modelName);
        qWarning() << "addModelFromNetwork: 保存模型文件失败:";
        return false;
    }
    return true;
}


QJsonObject  WebServer::getModelsInfo() const {
    QJsonObject result;
    QJsonArray modelsArray;

    for (auto it = m_qmlbridge->m_modelManager->m_models.begin(); it != m_qmlbridge->m_modelManager->m_models.end(); ++it) {
        const QString &modelName = it.key();
        const auto &model = it.value();

        QJsonObject modelInfo;
        modelInfo["name"] = modelName;

        QJsonArray dataArray;
        const auto &points = model->data_points;
        for (const auto &point : points) {
            QJsonObject pointObj;
            pointObj["soc"] = point.soc;
            pointObj["ocv"] = point.ocv;
            pointObj["esr"] = point.imp;
            dataArray.append(pointObj);
        }
        modelInfo["data"] = dataArray;

        modelsArray.append(modelInfo);
    }

    result["models"] = modelsArray;
    result["status"] = "success";

    return result;
}
