#pragma once
#include <QLoggingCategory>
#include <QTcpServer>
#include <QMutex>
//#include <QString>



Q_DECLARE_LOGGING_CATEGORY(web)

class WebServer : public QObject
{
    Q_OBJECT
public:
    explicit WebServer(QObject *parent = nullptr);
    ~WebServer();

    bool start();
    void stop();

private:
    void onNewConnection();

    void handleHttpRequest(QTcpSocket *client, const QByteArray &request);
    void sendHttpResponse(QTcpSocket *client,const QString &content,
         const QString &contentType = "text/html",int statusCode = 200);

    QString generateHtmlPage();

private:
    QMutex m_Mutex;
    QTcpServer *m_server{nullptr};
    //QThread *m_serverThread{nullptr};
};
