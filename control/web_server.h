#pragma once
#include <QLoggingCategory>
#include <QTcpServer>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QMutex>
#include <QMap>
#include <QFile>
#include <QByteArray>

Q_DECLARE_LOGGING_CATEGORY(web)

class WebServer : public QObject
{
    Q_OBJECT
public:
    explicit WebServer(QObject *parent = nullptr);
    ~WebServer();

    bool start(int httpPort = 80, int wsPort = 8080);

    void updateChannelData(int channel, double voltage, double current,
                          const QString& status = "normal");

private:
    void onHttpNewConnection();
    void handleHttpRequest(QTcpSocket *client);
    void serveResourceFile(QTcpSocket *client, const QString &path);
    void handleApiRequest(QTcpSocket *client, const QString &path);
    QByteArray loadResourceFile(const QString &path);
    QString getMimeType(const QString &filePath);
    void sendHttpResponse(QTcpSocket *client, const QByteArray &content,
                          const QString &contentType = "text/html",int statusCode = 200);

    void onWsNewConnection();
    void onWsTextMessageReceived(QWebSocket *socket,const QString &message);
    QString executeScpiCommand(const QString &command);

private:
    QMutex m_httpmutex;
    QTcpServer *m_httpServer{nullptr};
    QMap<QTcpSocket*, QByteArray> m_httpBuffers;
    QMap<QString, QByteArray> m_fileCache;

    QMutex m_webmutex;
    QWebSocketServer *m_wsServer{nullptr};
    QMap<QWebSocket*, QString> m_wsClients;
    struct ChannelData {
        double voltage = 0.0;
        double current = 0.0;
        QString status = "";
    };
    QMap<int, ChannelData> m_channelData;
};
