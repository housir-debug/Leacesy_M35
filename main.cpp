#include <QDir>
#include <QThread>
#include <QQmlContext>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "auxiliary/simple_logger.h"
#include "auxiliary/config_manager.h"
#include "auxiliary/battery_model.h"
#include "auxiliary/scpi_handle.h"
#include "auxiliary/qml_agency.h"
#include "channel/uart_channel.h"
#include "channel/can_channel.h"
#include "control/tcp_server.h"
#include "control/web_server.h"
#include "control/can_server.h"
#include "control/uart_server.h"

Q_LOGGING_CATEGORY(application, "APP")

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
    // qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setApplicationName("Leacesy_Instrument-hrx");
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
    loggermanage(ConfigManager::s_loglevel , parentPath);
    QObject::connect(&app, &QGuiApplication::aboutToQuit, &shutdownLogger);
    if (!ConfigManager::init(parentPath)) {
        qCWarning(application) << "app get global config not exist!";
        return 1;   // error
    }

    // share model pointer create
    std::shared_ptr<ScpiManager> Scpi_share = std::make_shared<ScpiManager>();
    std::shared_ptr<BatteryModelManager> BatteryModel_share = std::make_shared<BatteryModelManager>(parentPath);
    std::shared_ptr<GuiBridge> GuiBridge_share = std::make_shared<GuiBridge>();
    GuiBridge_share->m_modelManager = BatteryModel_share;

    // lack of can channel

    // uart channel create
    std::vector<std::unique_ptr<UartChannelManager>> Uart_Channels;
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

            // QmlUI -> Uart
            QObject::connect(GuiBridge_share.get(),qml_signal[config.channel-1],channel.get(),&UartChannelManager::writeFrame,Qt::QueuedConnection);
            // Scpi -> uart
            QObject::connect(Scpi_share.get(),scpi_signal[config.channel-1],channel.get(),&UartChannelManager::writeFrame,Qt::QueuedConnection);

            Uart_Channels.push_back(std::move(channel));
        }

        //QObject::connect(Uart_Channels[0].get(),&UartChannelManager::serialDataReceived,Uart_Channels[1].get(),&UartChannelManager::writeSerialData);
        //QObject::connect(Uart_Channels[1].get(),&UartChannelManager::serialDataReceived,Uart_Channels[0].get(),&UartChannelManager::writeSerialData);
        //QTimer::singleShot(300, &app, &QGuiApplication::quit);
    }

    // screen GUI engine create
    if (ConfigManager::s_enableDisplay){
        QQmlApplicationEngine engine;
        engine.addImportPath(QStringLiteral("qrc:/qml"));
        engine.rootContext()->setContextProperty("Uart_bridge", GuiBridge_share.get());

        const QUrl url(QStringLiteral("qrc:/qml/Component/test.qml"));   //main.qml   Component/test.qml
        QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl){
                qCWarning(application) << "Object not exist and the URL matches.!";
                QCoreApplication::exit(-1);
            }
        }, Qt::QueuedConnection);

        engine.load(url);
    }

    if (ConfigManager::s_enableWEBServer){
        std::unique_ptr<WebServerManager> webServer = std::make_unique<WebServerManager>();
        webServer->m_BatteryManager = BatteryModel_share;
        webServer->m_qmlbridge = GuiBridge_share;
        webServer->m_scpiManager = Scpi_share;
        if (!webServer->startServer()) {
            qCWarning(application) << "WebServerManager not Normal start!";
            return 1;
        }
    }

    if (ConfigManager::s_enableLANServer){
        std::unique_ptr<TcpServerManager> vxiServer = std::make_unique<TcpServerManager>();
        vxiServer->m_qmlbridge = GuiBridge_share;
        vxiServer->m_scpiManager = Scpi_share;
        if (!vxiServer->startServer()) {
            qCWarning(application) << "TcpServer not Normal start!";
            return 1;
        }
    }

    if (ConfigManager::s_enableUARTServer){
        std::unique_ptr<UartServerManager> uartServer = std::make_unique<UartServerManager>();
        uartServer->m_qmlbridge = GuiBridge_share;
        uartServer->m_scpiManager = Scpi_share;
        if (!uartServer->startServer("/dev/ttyWCH27",QSerialPort::Baud38400)) {
            qCWarning(application) << "UartServer not Normal start!";
            return 1;
        }
    }

    // GPIB server create

    /*
    if (ConfigManager::s_enableCanMess){
        std::unique_ptr<CanServerManager> canServer = std::make_unique<CanServerManager>();
        std::unique_ptr<QThread> canThread = std::make_unique<QThread>();

        canServer->moveToThread(canThread.get());
        canThread->setObjectName("can_worker");
        canThread->start();

        QMetaObject::invokeMethod(canServer.get(), [worker = canServer.get()]() {
            worker->initialize("all", 1000000);  // all
            worker->testLoopback();
        }, Qt::QueuedConnection);//Blocking
        //QTimer::singleShot(300, &app, &QGuiApplication::quit);
    }*/

    return app.exec();
}

