#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QString>

class WebServer : public QObject
{
    Q_OBJECT
public:
    explicit WebServer(QObject *parent = nullptr);
    ~WebServer();

    bool start(quint16 port = 8080);
    void stop();

    QString getServerUrl() const;

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();

private:
    QTcpServer *m_server;
    quint16 m_port;
    QString m_ip;

    // 处理HTTP请求
    void handleHttpRequest(QTcpSocket *client, const QByteArray &request);

    // 发送HTTP响应
    void sendHttpResponse(QTcpSocket *client,
                         const QString &content,
                         const QString &contentType = "text/html",
                         int statusCode = 200);

    // 生成HTML页面
    QString generateHtmlPage();
    QString generateNIDiscoveryJson();

    // 获取本地IP
    QString getLocalIP();
};

#endif // WEBSERVER_H
