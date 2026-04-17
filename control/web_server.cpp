#include "web_server.h"
#include "auxiliary/config_manager.h"
#include "auxiliary/vxinamespace.h"
#include <QTcpSocket>
#include <QWebSocket>

Q_LOGGING_CATEGORY(web, "WEB:")

WebServerManager::WebServerManager(QObject *parent) : QObject(parent){
    m_staticFiles = {
        {"/",                     ":/web/web/index.html"},
        {"/index.html",           ":/web/web/index.html"},
        {"/channels.html",        ":/web/web/channels.html"},
        {"/import.html",          ":/web/web/import.html"},
        {"/js/index.js",          ":/web/web/js/index.js"},
        {"/js/channels.js",       ":/web/web/js/channels.js"},
        {"/js/import.js",         ":/web/web/js/import.js"},
        {"/js/header.js",         ":/web/web/js/header.js"},
        {"/css/style.css",        ":/web/web/css/style.css"},
        {"/css/index.css",        ":/web/web/css/index.css"},
        {"/css/channels.css",     ":/web/web/css/channels.css"},
        {"/css/import.css",       ":/web/web/css/import.css"},
        {"/css/global.css",       ":/web/web/css/global.css"},
        {"/icon/leacesylogo.png", ":/web/web/icon/leacesylogo.png"},
        {"/icon/leacesyicon.png", ":/web/web/icon/leacesyicon.png"}
    };

    m_mimeTypes = {
        {"html",                  "text/html"},
        {"css",                   "text/css"},
        {"js",                    "application/javascript"},
        {"json",                  "application/json"},
        {"png",                   "image/png"},
        {"jpg",                   "image/jpeg"},
        {"svg",                   "image/svg+xml"},
        {"ico",                   "image/x-icon"},
        {"txt",                   "text/plain"},
        {"xml",                   "application/xml"},
        {"pdf",                   "application/pdf"}
    };
}
WebServerManager::~WebServerManager()
{
    if (m_httpServer && m_wsServer && m_webThread) {
        qCDebug(web)<<"[~WebServerManager]:~WebServerManager Destroyed!!!";
        for (QTcpSocket *client : qAsConst(m_clients)) {
            client->disconnectFromHost();
            client->waitForDisconnected(600);
        }
        m_clients.clear();
        m_httpServer->close();
        delete m_httpServer;
        m_httpServer = nullptr;

        m_sockets.clear();
        m_wsServer->close();
        delete m_wsServer;
        m_wsServer = nullptr;

        m_webThread->quit();
        m_webThread->wait(1000); // wait 1s
        m_webThread->deleteLater();
        delete m_webThread;
        m_webThread = nullptr;
    }
}

bool WebServerManager::startServer(){
    if (!m_webThread && !m_httpServer && !m_wsServer){
        m_webThread = new QThread(this);
        m_httpServer = new QTcpServer(this);
        m_wsServer = new QWebSocketServer("Leacesy",QWebSocketServer::NonSecureMode, this);
        m_webThread->setObjectName("WebServer");

        this->moveToThread(m_webThread);
        m_wsServer->moveToThread(m_webThread);
        m_httpServer->moveToThread(m_webThread);
        m_webThread->start();

        connect(m_httpServer, &QTcpServer::newConnection,this,[this](){
            QTcpSocket* client = m_httpServer->nextPendingConnection();
            if (m_clients.size() <= 9) {
                client->setObjectName(QString("%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort()));
                qCDebug(web) <<"[startServer]:New HTTP client: "<< client->objectName();

                connect(client, &QTcpSocket::disconnected,this, [this, client](){
                    qCDebug(web)<<"[startServer]:Disconnected "<<client->objectName();
                    m_clients.removeOne(client);
                    client->deleteLater();
                }, Qt::DirectConnection);
                connect(client, &QTcpSocket::errorOccurred,this, [client](QAbstractSocket::SocketError error){
                    qCWarning(web)<<"[startServer]:ERROR Socket: ["<< client->objectName()<<"]"<< error <<client->errorString();
                }, Qt::DirectConnection);
                connect(client, &QTcpSocket::readyRead,this, [this, client](){
                    handleHttpRequest(client);
                }, Qt::DirectConnection);

                m_clients.append(client);
                return;
            }

            qCWarning(web)<<"[startServer]:Connected Clients Exceeds 9. Refused Clients:"<<client->peerAddress().toString();
            client->disconnectFromHost();
            client->deleteLater();
        },Qt::DirectConnection);

        connect(m_wsServer, &QWebSocketServer::newConnection,this, [this](){
            QWebSocket* socket = m_wsServer->nextPendingConnection();
            if (m_sockets.size() <= 9) {
                socket->setObjectName(QString("%1:%2").arg(socket->peerAddress().toString()).arg(socket->peerPort()));
                qCDebug(web) <<"[startServer]:New WebSocket client: "<< socket->objectName();

                connect(socket, &QWebSocket::disconnected,this, [this, socket](){
                    qCDebug(web)<<"[startServer]:Disconnected "<<socket->objectName();
                    m_sockets.removeOne(socket);
                    socket->deleteLater();
                }, Qt::DirectConnection);
                connect(socket,QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),this, [socket](QAbstractSocket::SocketError error){
                    qCWarning(web)<<"[startServer]:ERROR WebSocket: ["<<socket->objectName()<<"]"<< error <<socket->errorString();
                }, Qt::DirectConnection);

                connect(socket, &QWebSocket::textMessageReceived,this, [this, socket](const QString &message){
                    onWsTextMessageReceived(socket,message);
                }, Qt::DirectConnection);

                m_sockets.append(socket);
                return;
            }

            qCWarning(web)<<"[startServer]:Connected Clients Exceeds 9. Refused Clients:"<<socket->peerAddress().toString();
            socket->disconnect();
            socket->deleteLater();
        },Qt::DirectConnection);

        QMetaObject::invokeMethod(this, [this]() {
            if (m_httpServer->listen(QHostAddress::Any, Vxi11::HTTP_PORT)) {
                if (m_wsServer->listen(QHostAddress::Any, Vxi11::WEB_PORT)) {
                    setupRoutes(); // init route API and Web route
                }}}, Qt::QueuedConnection);

        return true;
    }

    return false;
}

void WebServerManager::setupRoutes()
{
    m_apiRoutes["/api/device/info"] = [this](QTcpSocket* client) {
        QJsonObject response;
        response["model"] = ConfigManager::s_model;
        response["serial"] = ConfigManager::s_serialNumber;
        response["software"] = ConfigManager::s_firmwareVersion;
        response["hardware"] = ConfigManager::s_hardwareVersion;

        QByteArray jsonData = QJsonDocument(response).toJson();
        sendHttpResponse(client, jsonData, "application/json");
    };

    m_apiRoutes["/api/scpi_commands"] = [this](QTcpSocket* client) {
        QJsonObject response;
        QJsonArray commands;
        for (int i = 0; ScpiManager::m_scpiCommands[i].pattern != nullptr; ++i) {
            if (strlen(ScpiManager::m_scpiCommands[i].pattern) > 0) {
                commands.append(QString::fromLatin1(ScpiManager::m_scpiCommands[i].pattern));
            }
        }
        response["commands"] = commands;

        QByteArray jsonData = QJsonDocument(response).toJson();
        sendHttpResponse(client, jsonData, "application/json");
    };

    m_apiRoutes["/api/channels"] = [this](QTcpSocket* client) {
        QJsonObject response;
        response["channels"] = m_qmlbridge->getAllChannelsData();

        QByteArray jsonData = QJsonDocument(response).toJson();
        sendHttpResponse(client, jsonData, "application/json");
    };

    m_apiRoutes["/api/models"] = [this](QTcpSocket* client) {
        QJsonObject response;
        response["models"] = getModelsInfo();

        QByteArray jsonData = QJsonDocument(response).toJson();
        sendHttpResponse(client, jsonData, "application/json");
    };

    // -----------------------------------------------------------------------

    m_webRoutes["scpi_command"] = [this](QWebSocket* socket, const QJsonObject& obj) {
        QByteArray cmd = (obj["command"].toString()+"\n").toUtf8();
        qCDebug(web) << "SCPI command received:" << cmd;

        m_qmlbridge->update_remotemodel(true);
        QByteArray res = m_scpiManager->processCommand(cmd);

        if (!res.isEmpty()){
            QJsonObject response;
            response["type"] = "scpi_response";
            response["result"] = QString::fromUtf8(res);
            socket->sendTextMessage(QJsonDocument(response).toJson());
        }
    };

    m_webRoutes["channels_update"] = [this](QWebSocket* socket, const QJsonObject&) {
        QJsonObject channelData;
        channelData["type"] = "channels_response";
        channelData["channels"] = m_qmlbridge->getAllChannelsData();
        socket->sendTextMessage(QJsonDocument(channelData).toJson());
    };

    m_webRoutes["model_upload"] = [this](QWebSocket* socket, const QJsonObject& obj) {
        QJsonObject content = obj["content"].toObject();
        QString modelName = content["name"].toString();
        QJsonArray modelData = content["data"].toArray();

        QJsonObject response;
        if (addModelFromNetwork(modelName,modelData)) {
            response["type"] = "model_sync";
            response["models"] = getModelsInfo();
            socket->sendTextMessage(QJsonDocument(response).toJson());
        } else {
            response["type"] = "error";
            response["message"] = QString("Failed to save model \"%1\"").arg(modelName);
            socket->sendTextMessage(QJsonDocument(response).toJson());
        }
    };

    m_webRoutes["model_delete"] = [this](QWebSocket* socket, const QJsonObject& obj) {
        QJsonObject response;
        QString modelName = obj["name"].toString();

        if (m_BatteryManager->removeModel(modelName)) {
            response["type"] = "model_sync";
            response["models"] = getModelsInfo();
            socket->sendTextMessage(QJsonDocument(response).toJson());
        } else {
            response["type"] = "error";
            response["message"] = QString("Failed to delete model \"%1\"").arg(modelName);
            socket->sendTextMessage(QJsonDocument(response).toJson());
        }
    };
}

//---------------------------------------------------------------------------------

void WebServerManager::handleHttpRequest(QTcpSocket *client){
    QMutexLocker locker(&m_httpmutex);

    QByteArray data = client->readAll();
    if (data.contains("\r\n\r\n")) {
        QString requestLine = QString::fromUtf8(data).split("\r\n").first();
        QStringList parts = requestLine.split(" ");

        if (parts.size() >= 3) {
            QString path = QUrl::fromPercentEncoding(parts[1].toUtf8());
            qCDebug(web) <<"[handleHttpRequest]:HTTP request: "<< parts[0] << path;

            if (m_apiRoutes.contains(path)) {
                m_apiRoutes[path](client);
                return;
            }
            else if (m_staticFiles.contains(path)) {
                QString resourcePath = m_staticFiles[path];
                if (!m_fileCache.contains(path)) {
                    QFile file(resourcePath);
                    if (!file.open(QIODevice::ReadOnly)) {
                        sendHttpResponse(client, "500 Internal Server Error", "text/plain", 500);
                        return;
                    }

                    m_fileCache[path] = file.readAll();
                }

                QString contentType = m_mimeTypes[QFileInfo(resourcePath).suffix()];
                sendHttpResponse(client, m_fileCache[path], contentType);
                return;
            }

            sendHttpResponse(client, "404 Not Found", "text/plain", 404);
            return;
        }
    }

    sendHttpResponse(client, "Please access using a web browser.", "text/plain");
    return;
}

void WebServerManager::sendHttpResponse(QTcpSocket *client, const QByteArray &content,const QString &contentType, int statusCode){
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
    ).arg(QString::number(statusCode), statusText, contentType, QString::number(content.size()));

    client->write(header.toUtf8());
    client->write(content);
    //client->flush();
    client->close();
}

//---------------------------------------------------------------------------------

void WebServerManager::onWsTextMessageReceived(QWebSocket *socket,const QString &message)
{
    QMutexLocker locker(&m_webmutex);

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    qCDebug(web) <<"[onWsTextMessageReceived]:WEB request: "<<message;
    if (!doc.isNull() && doc.isObject()) {
        QJsonObject obj = doc.object();
        QString type = obj["type"].toString();
        if (m_webRoutes.contains(type)) {
            m_webRoutes[type](socket,obj);
        }
    }
}

bool WebServerManager::addModelFromNetwork(const QString &modelName, const QJsonArray &modelData) {
    if (!modelName.isEmpty() && !m_BatteryManager->m_models.contains(modelName)) {
        auto model = QSharedPointer<BatteryModel>::create();
        model->name = modelName;

        for (const auto &item : modelData) {
            if (!item.isNull() && item.isObject()) {
                QJsonObject pointObj = item.toObject();

                if (pointObj.contains("soc") && pointObj.contains("ocv") && pointObj.contains("imp")) {
                    BatteryDataPoint point;
                    point.soc = pointObj["soc"].toDouble();
                    point.ocv = pointObj["ocv"].toDouble();
                    point.imp = pointObj["ocv"].toDouble();
                    model->data_points.append(point);
                }else{
                    qCWarning(web) << "[addModelFromNetwork]:Parameter lack error!";
                    return false;
                }
            }else{
                qCWarning(web) << "[addModelFromNetwork]:Parameter model format error!";
                return false;
            }
        }

        m_BatteryManager->saveModel(model, modelName);
        return true;
    }

    qCWarning(web) << "[addModelFromNetwork]:Parameter error or model incorrect!";
    return false;
}

QJsonObject  WebServerManager::getModelsInfo() const {
    QJsonObject result;
    QJsonArray modelsArray;

    for (auto it = m_BatteryManager->m_models.begin(); it != m_BatteryManager->m_models.end(); ++it) {
        QJsonObject modelInfo;
        modelInfo["name"] = it.key();

        QJsonArray dataArray;
        const auto &model = it.value();
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
