#pragma once
#include "auxiliary/scpi_handle.h"
#include "auxiliary/qml_agency.h"
#include "auxiliary/battery_model.h"
#include <QWebSocketServer>
#include <QLoggingCategory>
#include <QTcpServer>


Q_DECLARE_LOGGING_CATEGORY(web)

class WebServerManager : public QObject
{
    Q_OBJECT
public:
    explicit WebServerManager(QObject *parent = nullptr);
    ~WebServerManager();

    bool startServer();

    std::shared_ptr<GuiBridge> m_qmlbridge;
    std::shared_ptr<ScpiManager> m_scpiManager;
    std::shared_ptr<BatteryModelManager> m_BatteryManager;

private:
    void setupRoutes();
    using ApiHandler = std::function<void(QTcpSocket*)>;
    using WebHandler = std::function<void(QWebSocket*, const QJsonObject&)>;

    void handleHttpRequest(QTcpSocket *client);
    void sendHttpResponse(QTcpSocket *client, const QByteArray &content,
             const QString &contentType = "text/html",int statusCode = 200);

    void onWsTextMessageReceived(QWebSocket *socket,const QString &message);
    bool addModelFromNetwork(const QString &modelName, const QJsonArray &modelData);
    bool removeModel(const QString &modelName);
    QJsonObject getModelsInfo() const;

private:
    QMutex m_httpmutex;
    QList<QTcpSocket*> m_clients;
    QMap<QString, QString> m_mimeTypes;
    QMap<QString, QString> m_staticFiles;
    QMap<QString, ApiHandler> m_apiRoutes;
    QMap<QString, QByteArray> m_fileCache;

    QMutex m_webmutex;
    QList<QWebSocket*> m_sockets;
    QMap<QString, WebHandler> m_webRoutes;

    QThread* m_webThread{nullptr};
    QTcpServer *m_httpServer{nullptr};
    QWebSocketServer *m_wsServer{nullptr};
};
