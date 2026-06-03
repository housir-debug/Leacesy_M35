#include <QDir>
#include <QThread>
#include <QScreen>
#include <QWindow>
#include <QQuickView>
#include <QQmlContext>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "auxiliary/simple_logger.h"
#include "auxiliary/config_manager.h"
#include "auxiliary/battery_model.h"
#include "auxiliary/scpi_handle.h"
#include "auxiliary/qml_agency.h"
#include "channel/uart_channel.h"
//#include "channel/can_channel.h"
#include "control/tcp_server.h"
#include "control/web_server.h"
#include "control/can_server.h"
#include "control/uart_server.h"

Q_LOGGING_CATEGORY(application, "APP")

using CanSign_toUartCh = void (CanServerManager::*)(quint8 cmd, quint8 func, const QByteArray& param,bool isScpi);
std::vector<CanSign_toUartCh> can_signal = {
    #define CHANNEL(n) static_cast<CanSign_toUartCh>(&CanServerManager::to_UartChannel##n),
    CHANNEL_COUNT
    #undef CHANNEL
};

using QmlSign_toUartCh = void (GuiBridge::*)(quint8 cmd, quint8 func, const QByteArray& param,bool isScpi);
std::vector<QmlSign_toUartCh> qml_signal = {
    #define CHANNEL(n) static_cast<QmlSign_toUartCh>(&GuiBridge::to_UartChannel##n),
    CHANNEL_COUNT
    #undef CHANNEL
};

using ScpiSign_toUartCh = void (ScpiManager::*)(quint8 cmd, quint8 func, const QByteArray& param,bool isScpi);
std::vector<ScpiSign_toUartCh> scpi_signal = {
    #define CHANNEL(n) static_cast<ScpiSign_toUartCh>(&ScpiManager::to_UartChannel##n),
    CHANNEL_COUNT
    #undef CHANNEL
};


int main(int argc, char *argv[])
{
    // create APP-gui
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setApplicationName("Leacesy_hrx");
    QGuiApplication app(argc, argv);

    // get App parentpath
    QString appPath = QGuiApplication::applicationDirPath();
    QDir appDir(appPath);
    if (!appDir.cdUp()){
        qCWarning(application) << "app parentPath not exist!";
        return 1;   // error
    }

    // config log Setting And global variable
    QString parentPath = appDir.absolutePath();
    if (!ConfigManager::init(parentPath)) {
        qCWarning(application) << "app get global config not exist!";
        return 1;   // error
    }

    loggermanage(ConfigManager::s_loglevel , parentPath);
    QObject::connect(&app, &QGuiApplication::aboutToQuit, &shutdownLogger);

    // screen GUI engine and gui-bridge create
    std::shared_ptr<BatteryModelManager> BatteryModel_share = std::make_shared<BatteryModelManager>(parentPath);
    std::shared_ptr<GuiBridge> GuiBridge_share = std::make_shared<GuiBridge>();
    GuiBridge_share->m_modelManager = BatteryModel_share;
    QQmlApplicationEngine engine;
    if (ConfigManager::s_enableDisplay){
        engine.addImportPath(QStringLiteral("qrc:/qml"));
        engine.rootContext()->setContextProperty("Uart_bridge", GuiBridge_share.get());

        const QUrl url(QStringLiteral("qrc:/qml/main.qml"));   //main.qml   Component/test.qml
        QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl){
                qCWarning(application) << "Object not exist and the URL matches.!";
                QCoreApplication::exit(-1);
            }}, Qt::QueuedConnection);

        engine.load(url);
        GuiBridge_share->load_BatteryModel();
    }

    // CAN Server create
    std::unique_ptr<CanServerManager> canServer;
    if (ConfigManager::s_enableCANServer){
        canServer = std::make_unique<CanServerManager>();
        canServer->m_qmlbridge = GuiBridge_share;
        if (!canServer->startServer()) {
            qCWarning(application) << "canServer not Normal start!";
            return 1;
        }

        QObject::connect(GuiBridge_share.get(),&GuiBridge::to_CANid,canServer.get(),&CanServerManager::change_canid,Qt::QueuedConnection);
    }

    // can channel create
    /*Not necessary for the time being.*/

    // SCPI parser and uart channel create
    std::shared_ptr<ScpiManager> Scpi_share = std::make_shared<ScpiManager>();
    std::vector<std::unique_ptr<UartChannelManager>> Channel_list;
    if (ConfigManager::s_enableUartMess){
        // config form config_manager
        for (const auto& config : configs) {
            auto channel = std::make_unique<UartChannelManager>();
            channel->m_qmlbridge = GuiBridge_share;
            channel->m_scpiManager = Scpi_share;

            if (!channel->initSerialPort(config.port, config.baudRate)) {
                qCWarning(application) << "uart channel "<< config.port <<" Initialization failed!";
                return 1;
            }

            QObject::connect(GuiBridge_share.get(),qml_signal[config.channel-1],channel.get(),&UartChannelManager::writeFrame,Qt::QueuedConnection);
            QObject::connect(Scpi_share.get(),scpi_signal[config.channel-1],channel.get(),&UartChannelManager::writeFrame,Qt::QueuedConnection);

            if (ConfigManager::s_enableCANServer){
                QObject::connect(canServer.get(),can_signal[config.channel-1],channel.get(),&UartChannelManager::writeFrame,Qt::QueuedConnection);
                QObject::connect(channel.get(),&UartChannelManager::to_CanServer,canServer.get(),&CanServerManager::sendFrame,Qt::QueuedConnection);
            }

            Channel_list.push_back(std::move(channel)); // move set <channel> can move
        }
    }

    // LAN Server create
    std::unique_ptr<WebServerManager> webServer;
    if (ConfigManager::s_enableWEBServer){
        webServer = std::make_unique<WebServerManager>();
        webServer->m_BatteryManager = BatteryModel_share;
        webServer->m_qmlbridge = GuiBridge_share;
        webServer->m_scpiManager = Scpi_share;
        if (!webServer->startServer()) {
            qCWarning(application) << "WebServerManager not Normal start!";
            return 1;
        }
    }

    std::unique_ptr<TcpServerManager> vxiServer;
    if (ConfigManager::s_enableLANServer){
        vxiServer = std::make_unique<TcpServerManager>();
        vxiServer->m_qmlbridge = GuiBridge_share;
        vxiServer->m_scpiManager = Scpi_share;
        if (!vxiServer->startServer()) {
            qCWarning(application) << "TcpServer not Normal start!";
            return 1;
        }
    }

    // UART Server create
    std::unique_ptr<UartServerManager> uartServer;
    if (ConfigManager::s_enableUARTServer){
        uartServer = std::make_unique<UartServerManager>();
        uartServer->m_qmlbridge = GuiBridge_share;
        uartServer->m_scpiManager = Scpi_share;
        if (!uartServer->startServer("/dev/ttyWCH27",QSerialPort::Baud38400)) {
            qCWarning(application) << "UartServer not Normal start!";
            return 1;
        }
    }

    // GPIB server create
    /*Not necessary for the time being.*/

    //QTimer::singleShot(9000, &app, &QGuiApplication::quit); // 9s
    return app.exec();
}

