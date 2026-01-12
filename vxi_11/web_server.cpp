#include "web_server.h"
#include <QNetworkInterface>
#include <QDebug>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>

WebServer::WebServer(QObject *parent) : QObject(parent)
    , m_server(nullptr)
    , m_port(8080)
    , m_ip("127.0.0.1")
{
}

WebServer::~WebServer()
{
    stop();
}

bool WebServer::start(quint16 port)
{
    m_port = port;
    m_ip = getLocalIP();

    // 创建TCP服务器
    m_server = new QTcpServer(this);

    if (!m_server->listen(QHostAddress::Any, port)) {
        qWarning() << "Failed to start web server:" << m_server->errorString();
        delete m_server;
        m_server = nullptr;
        return false;
    }

    // 连接信号
    connect(m_server, &QTcpServer::newConnection,
            this, &WebServer::onNewConnection);

    qDebug() << "========================================";
    qDebug() << "Simple Web Server Started";
    qDebug() << "URL: http://" << m_ip << ":" << m_port;
    qDebug() << "========================================";

    return true;
}

void WebServer::stop()
{
    if (m_server) {
        m_server->close();
        delete m_server;
        m_server = nullptr;
    }
}

QString WebServer::getServerUrl() const
{
    return QString("http://%1:%2").arg(m_ip).arg(m_port);
}

void WebServer::onNewConnection()
{
    QTcpSocket *client = m_server->nextPendingConnection();
    if (!client) return;

    qDebug() << "New client connected from:" << client->peerAddress().toString();

    connect(client, &QTcpSocket::readyRead,
            this, &WebServer::onClientReadyRead);
    connect(client, &QTcpSocket::disconnected,
            this, &WebServer::onClientDisconnected);
    connect(client, &QTcpSocket::disconnected,
            client, &QTcpSocket::deleteLater);
}

void WebServer::onClientReadyRead()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client || !client->bytesAvailable()) return;

    QByteArray requestData = client->readAll();
    handleHttpRequest(client, requestData);
}

void WebServer::onClientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (client) {
        qDebug() << "Client disconnected:" << client->peerAddress().toString();
    }
}

void WebServer::handleHttpRequest(QTcpSocket *client, const QByteArray &request)
{
    QString requestStr = QString::fromUtf8(request);
    QStringList lines = requestStr.split("\r\n");

    if (lines.isEmpty()) {
        sendHttpResponse(client, "Bad Request", "text/plain", 400);
        return;
    }

    // 解析请求行（第一行）
    QString requestLine = lines[0];
    QStringList parts = requestLine.split(" ");
    if (parts.size() < 3) {
        sendHttpResponse(client, "Bad Request", "text/plain", 400);
        return;
    }

    QString method = parts[0];
    QString path = parts[1];

    qDebug() << "HTTP Request:" << method << path;

    // 处理不同的路径
    if (path == "/" || path == "/index.html") {
        sendHttpResponse(client, generateHtmlPage());
    }
    else if (path == "/niwebdiscovery") {
        // NI-MAX 发现的特殊路径
        sendHttpResponse(client, generateNIDiscoveryJson(), "application/json");
    }
    else if (path == "/systemweb") {
        // NI SystemWeb 重定向
        sendHttpResponse(client, generateHtmlPage());
    }
    else if (path == "/favicon.ico") {
        // 返回空favicon
        sendHttpResponse(client, "", "image/x-icon", 204); // No Content
    }
    else {
        // 404 页面
        QString notFound = R"(
            <h1>404 Not Found</h1>
            <p>The requested URL was not found on this server.</p>
        )";
        sendHttpResponse(client, notFound, "text/html", 404);
    }
}

void WebServer::sendHttpResponse(QTcpSocket *client,
                                const QString &content,
                                const QString &contentType,
                                int statusCode)
{
    if (!client || !client->isOpen()) return;

    QString statusText;
    switch (statusCode) {
        case 200: statusText = "OK"; break;
        case 404: statusText = "Not Found"; break;
        default: statusText = "OK"; break;
    }

    QString response = QString(
        "HTTP/1.1 %1 %2\r\n"
        "Server: MyInstrument/1.0\r\n"
        "Content-Type: %3; charset=utf-8\r\n"
        "Content-Length: %4\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%5"
    ).arg(statusCode).arg(statusText).arg(contentType).arg(content.toUtf8().size()).arg(content);

    client->write(response.toUtf8());
    client->flush();
    client->close();
}

QString WebServer::generateHtmlPage()
{
    QString html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <!-- NI-MAX 识别标签 -->
    <meta name="ni-device-type" content="Instrument">
    <meta name="ni-model" content="MyInstrument-100">
    <meta name="ni-serial" content="SN123456789">
    <meta name="ni-firmware" content="1.0.0">
    <meta name="ni-vendor" content="YourCompany">
    <title>MyInstrument Device</title>
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
        <h1>📡 MyInstrument Device</h1>

        <div class="status">
            <p>Status: <span class="online">● Online</span></p>
            <p>Web Interface is working correctly.</p>
        </div>

        <h2>Device Information</h2>
        <table>
            <tr><th>Property</th><th>Value</th></tr>
            <tr><td>Model</td><td>MyInstrument-100</td></tr>
            <tr><td>Serial Number</td><td>SN123456789</td></tr>
            <tr><td>Firmware</td><td>1.0.0</td></tr>
            <tr><td>IP Address</td><td>%1</td></tr>
            <tr><td>Web Port</td><td>%2</td></tr>
            <tr><td>Last Update</td><td>%3</td></tr>
        </table>

        <h2>Connection Info</h2>
        <p>NI-MAX should be able to detect this device and open this web page.</p>
        <p>Discovery URL: <code>http://%1:%2/niwebdiscovery</code></p>

        <div class="footer">
            <p>This is a minimal test page for NI-MAX compatibility.</p>
            <p>Server Time: %3</p>
        </div>
    </div>
</body>
</html>
    )";

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    return html.arg(m_ip).arg(m_port).arg(timestamp);
}

QString WebServer::generateNIDiscoveryJson()
{
    QJsonObject niInfo;
    niInfo["manufacturer"] = "YourCompany";
    niInfo["model"] = "MyInstrument-100";
    niInfo["serial"] = "SN123456789";
    niInfo["firmware"] = "1.0.0";
    niInfo["deviceType"] = "Instrument";
    niInfo["webPort"] = (int)m_port;
    niInfo["webPath"] = "/";
    niInfo["discoveryTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    niInfo["apiVersion"] = "1.0";

    QJsonDocument doc(niInfo);
    return doc.toJson(QJsonDocument::Compact);
}

QString WebServer::getLocalIP()
{
    // 尝试获取真实IP，失败则返回本地环回
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces()) {
        if (interface.flags().testFlag(QNetworkInterface::IsUp) &&
            interface.flags().testFlag(QNetworkInterface::IsRunning) &&
            !interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {

            foreach (const QNetworkAddressEntry &entry, interface.addressEntries()) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    return entry.ip().toString();
                }
            }
        }
    }
    return "127.0.0.1";
}
