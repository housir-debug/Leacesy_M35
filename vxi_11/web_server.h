#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QLoggingCategory>
#include <QString>
#include <QMutex>
#include "auxiliary/config_manager.h"
#include "scpimanager.h"

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

#endif // WEBSERVER_H
