#include "web_server.h"
#include <QNetworkInterface>
#include <QDebug>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>

// ===================== 初始化/启动部分 =================================

Q_LOGGING_CATEGORY(web, "web:")

WebServer::WebServer(QObject *parent) : QObject(parent){}

bool WebServer::start()
{
    // 复杂后加入线程
    m_server = new QTcpServer(this);

    if (!m_server->listen(QHostAddress::Any, 80)) {
        qCWarning(web) << "Failed to start web server:" << m_server->errorString();
        delete m_server;
        m_server = nullptr;
        return false;
    }

    connect(m_server, &QTcpServer::newConnection,this, &WebServer::onNewConnection);
    return true;
}

void WebServer::onNewConnection()
{
    QTcpSocket *client = m_server->nextPendingConnection();
    if (!client) {return;}

    qCDebug(web) << "New client connected from:" << client->peerAddress().toString()<<client->peerPort();

    connect(client, &QTcpSocket::errorOccurred,this, [client](QAbstractSocket::SocketError error){
         qCWarning(web) << "Socket error from" << client->peerAddress().toString()<<client->peerPort()<< ":" << client->errorString() << "|" << error;
    });
    connect(client, &QTcpSocket::disconnected,this, [client](){
        qCDebug(web) << "Client disconnected:" << client->peerAddress().toString()<<client->peerPort();
        client->deleteLater();
    });
    connect(client, &QTcpSocket::readyRead,this,[this,client]{
        QByteArray rawData = client->readAll();
        qCDebug(web) << "RECEIVED FROM" << client->peerAddress().toString()<<client->peerPort()<< "Raw request:";
        qCDebug(web) << rawData.constData();
        this->handleHttpRequest(client, rawData);
    });
}

// ===================== 信息处理部分 =================================

void WebServer::handleHttpRequest(QTcpSocket *client, const QByteArray &request)
{
    QString requestStr = QString::fromUtf8(request);
    QStringList lines = requestStr.split("\r\n");   // HTTP
    if (lines.isEmpty()) {
        sendHttpResponse(client, "Bad Request", "text/plain", 400);
        return;
    }

    QString requestLine = lines[0];
    QStringList parts = requestLine.split(" ");
    if (parts.size() < 3) {
        sendHttpResponse(client, "Bad Request", "text/plain", 400);
        return;
    }

    QString path = parts[1];   // parts[0]=method
    if (path == "/" || path == "/index.html" || path == "/systemweb") {sendHttpResponse(client, generateHtmlPage());}
    else if (path == "/favicon.ico") {sendHttpResponse(client, "", "image/x-icon", 204);}
    else {
        QString notFound = R"(
            <h1>404 Not Found</h1>
            <p>The requested URL was not found on this server.</p>
        )";
        sendHttpResponse(client, notFound, "text/html", 404);
    }
}

void WebServer::sendHttpResponse(QTcpSocket *client,const QString &content,const QString &contentType,int statusCode)
{
    QMutexLocker locker(&m_Mutex);
    if (!client || !client->isOpen()){ return;}

    QString statusText;
    switch (statusCode) {
        case 204: statusText = "No Content"; break;
        case 400: statusText = "Bad Request"; break;
        case 404: statusText = "Not Found"; break;
        case 405: statusText = "Method Not Allowed"; break;
        default: statusText = "OK"; break;
    }

    QString response = QString(
        "HTTP/1.1 %1 %2\r\n"
        "Content-Type: %3; charset=utf-8\r\n"
        "Content-Length: %4\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%5"
    ).arg(QString::number(statusCode),statusText,contentType,QString::number(content.toUtf8().size()),content);

    client->write(response.toUtf8());
    client->flush();
    client->close();   // disconnectFromHost()用于客户端断开，close完全关闭用于服务器处理
}

// ===================== 网页部分 =================================

QString WebServer::generateHtmlPage()
{
    QString html = R"(
<!DOCTYPE html>
<html><head>
    <meta charset="UTF-8">
    <title>Leacesy Instrument</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 50px auto;
            padding: 20px;
            background: #f5f5f5;
        }
        .container {
            background: white;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        h1 {
            color: #0066cc;
            border-bottom: 2px solid #0066cc;
            padding-bottom: 10px;
        }
        .status {
            background: #e8f4ff;
            padding: 15px;
            border-radius: 5px;
            margin: 20px 0;
        }
        .online {
            color: green;
            font-weight: bold;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin: 20px 0;
        }
        th, td {
            padding: 10px;
            text-align: left;
            border-bottom: 1px solid #ddd;
        }
        th {
            background: #f0f0f0;
        }
        .footer {
            margin-top: 30px;
            text-align: center;
            color: #666;
            font-size: 0.9em;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>📡   Leacesy Instrument Device</h1>

        <div class="status">
            <p>Status: <span class="online">● Online</span></p>
            <p>Web Interface is working correctly......</p>
        </div>

        <h2>Device Information</h2>
        <table>
            <tbody><tr><th>Property</th><th>Value</th></tr>
            <tr><td>Model</td><td>%1</td></tr>
            <tr><td>Serial Number</td><td>%2</td></tr>
            <tr><td>Firmware Version</td><td>%3</td></tr>
            <tr><td>Last Update</td><td>%4</td></tr>
        </tbody></table>

        <h2>Connection Info</h2>
        <p>NI-MAX should be able to detect this device and open this web page.</p>

    </div>
</body></html>
    )";

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    return html.arg(ConfigManager::s_model,ConfigManager::s_serialNumber,
                    ConfigManager::s_firmwareVersion ,timestamp);
}

// ==================== 析构部分 ====================

WebServer::~WebServer()
{
    stop();
    qCDebug(web)<< "webServer destroyed";
}

void WebServer::stop()
{
    if (m_server) {
        m_server->close();
        delete m_server;
        m_server = nullptr;
    }
}
