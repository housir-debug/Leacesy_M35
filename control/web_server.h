#pragma once
#include "auxiliary/scpi_handle.h"
#include "auxiliary/qml_agency.h"
#include "auxiliary/battery_model.h"
#include <QLoggingCategory>
#include <QTcpServer>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QMutex>
#include <QMap>
#include <QFile>
#include <QByteArray>

Q_DECLARE_LOGGING_CATEGORY(web)

class WebServerManager : public QObject
{
    Q_OBJECT
public:
    explicit WebServerManager(QObject *parent = nullptr);
    ~WebServerManager();

    std::shared_ptr<ScpiManager> m_scpiManager;
    std::shared_ptr<GuiBridge> m_qmlbridge;
    std::shared_ptr<BatteryModelManager> m_BatteryManager;
    bool startServer();

private:
    void onHttpNewConnection();
    void handleHttpRequest(QTcpSocket *client);
    void handleApiRequest(QTcpSocket *client, const QString &path);
    void serveResourceFile(QTcpSocket *client, const QString &path);
    QString getMimeType(const QString &filePath);
    void sendHttpResponse(QTcpSocket *client, const QByteArray &content,
                          const QString &contentType = "text/html",int statusCode = 200);

    void onWsNewConnection();
    void onWsTextMessageReceived(QWebSocket *socket,const QString &message);

    bool addModelFromNetwork(const QString &modelName, const QJsonArray &modelData);
    bool removeModel(const QString &modelName);
    QJsonObject getModelsInfo() const;

private:
    struct ChannelData {
        double voltage = 0.0;
        double current = 0.0;
        double cvSetpoint = 0.0;
        double ccSetpoint = 0.0;
        double ovSetpoint = 0.0;
        int currentMode = 0;
        bool isEnabled = false;

    };
    QThread* m_webThread{nullptr};
    QMap<int, ChannelData> m_channelData;
    QByteArray m_responsebuffer;

    QMutex m_httpmutex;
    int httpPort = 80;
    QTcpServer *m_httpServer{nullptr};
    QMap<QTcpSocket*, QByteArray> m_httpBuffers;
    QMap<QString, QByteArray> m_fileCache;

    QMutex m_webmutex;
    int wsPort = 8080;
    QWebSocketServer *m_wsServer{nullptr};
    QMap<QWebSocket*, QString> m_wsClients;
};
