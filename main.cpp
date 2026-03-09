#include <QDir>
#include <QThread>
#include <QQmlContext>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "auxiliary/simple_logger.h"
#include "auxiliary/config_manager.h"
#include "auxiliary/scpi_handle.h"
#include "auxiliary/qml_agency.h"
#include "channel/uart_channel.h"
#include "channel/canworker.h"
#include "control/tcp_server.h"
#include "control/web_server.h"
#include "control/uart_server.h"


using Signal_Bridge = void (SerialBridge::*)(quint8 cmd, quint8 func, const QByteArray& param,bool isScpi);
std::vector<Signal_Bridge> qml_signal = {
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel1),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel2),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel3),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel4),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel5),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel6),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel7),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel8),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel9),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel10),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel11),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel12),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel13),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel14),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel15),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel16),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel17),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel18),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel19),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel20),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel21),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel22),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel23),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel24),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel25),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel26),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel27),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel28),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel29),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel30),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel31),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel32),
    static_cast<Signal_Bridge>(&SerialBridge::to_UartChannel33),
};

using SignalType = void (ScpiManager::*)(quint8 cmd, quint8 func, const QByteArray& param,bool isScpi);
std::vector<SignalType> scpi_signal = {
    static_cast<SignalType>(&ScpiManager::to_UartChannel1),
    static_cast<SignalType>(&ScpiManager::to_UartChannel2),
    static_cast<SignalType>(&ScpiManager::to_UartChannel3),
    static_cast<SignalType>(&ScpiManager::to_UartChannel4),
    static_cast<SignalType>(&ScpiManager::to_UartChannel5),
    static_cast<SignalType>(&ScpiManager::to_UartChannel6),
    static_cast<SignalType>(&ScpiManager::to_UartChannel7),
    static_cast<SignalType>(&ScpiManager::to_UartChannel8),
    static_cast<SignalType>(&ScpiManager::to_UartChannel9),
    static_cast<SignalType>(&ScpiManager::to_UartChannel10),
    static_cast<SignalType>(&ScpiManager::to_UartChannel11),
    static_cast<SignalType>(&ScpiManager::to_UartChannel12),
    static_cast<SignalType>(&ScpiManager::to_UartChannel13),
    static_cast<SignalType>(&ScpiManager::to_UartChannel14),
    static_cast<SignalType>(&ScpiManager::to_UartChannel15),
    static_cast<SignalType>(&ScpiManager::to_UartChannel16),
    static_cast<SignalType>(&ScpiManager::to_UartChannel17),
    static_cast<SignalType>(&ScpiManager::to_UartChannel18),
    static_cast<SignalType>(&ScpiManager::to_UartChannel19),
    static_cast<SignalType>(&ScpiManager::to_UartChannel20),
    static_cast<SignalType>(&ScpiManager::to_UartChannel21),
    static_cast<SignalType>(&ScpiManager::to_UartChannel22),
    static_cast<SignalType>(&ScpiManager::to_UartChannel23),
    static_cast<SignalType>(&ScpiManager::to_UartChannel24),
    static_cast<SignalType>(&ScpiManager::to_UartChannel25),
    static_cast<SignalType>(&ScpiManager::to_UartChannel26),
    static_cast<SignalType>(&ScpiManager::to_UartChannel27),
    static_cast<SignalType>(&ScpiManager::to_UartChannel28),
    static_cast<SignalType>(&ScpiManager::to_UartChannel29),
    static_cast<SignalType>(&ScpiManager::to_UartChannel30),
    static_cast<SignalType>(&ScpiManager::to_UartChannel31),
    static_cast<SignalType>(&ScpiManager::to_UartChannel32),
    static_cast<SignalType>(&ScpiManager::to_UartChannel33),
};

int main(int argc, char *argv[])
{
    //qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setApplicationName("Leacesy Instrument");
    QGuiApplication app(argc, argv);

    QString appPath = QGuiApplication::applicationDirPath();
    QDir appDir(appPath);
    if (!appDir.cdUp()){return 1;}
    QString parentPath = appDir.absolutePath();
    if (!ConfigManager::init(parentPath)) {return 1;}

    loggermanage(ConfigManager::s_loglevel,parentPath);
    if (ConfigManager::s_enablelogfile){
        QObject::connect(&app, &QGuiApplication::aboutToQuit, &shutdownLogger);
    }

    std::unique_ptr<CanWorker> canWorker;
    std::unique_ptr<QThread> canThread;
    if (ConfigManager::s_enableCanMess){
        canWorker = std::make_unique<CanWorker>();
        canThread = std::make_unique<QThread>();

        canWorker->moveToThread(canThread.get());
        canThread->setObjectName("can_worker");
        canThread->start();

        QMetaObject::invokeMethod(canWorker.get(), [worker = canWorker.get()]() {
            worker->initialize("all", 1000000);  // all
            worker->testLoopback();
        }, Qt::QueuedConnection);//Blocking
        //QTimer::singleShot(300, &app, &QGuiApplication::quit);
    }

    std::unique_ptr<ScpiManager> Scpi_process;
    std::unique_ptr<SerialBridge> Uart_bridge;
    std::vector<std::unique_ptr<SerialWorker>> Uart_Channels;
    if (ConfigManager::s_enableUartMess){
        Uart_bridge = std::make_unique<SerialBridge>();
        Scpi_process = std::make_unique<ScpiManager>();

        for (const auto& config : configs) {
            auto channel = std::make_unique<SerialWorker>(Scpi_process.get(),Uart_bridge.get());
            if (!channel->initSerialPort(config.port, config.baudRate)) {return 1;}

            // QmlUI -> Uart
            QObject::connect(Uart_bridge.get(),qml_signal[config.channel-1],channel.get(),&SerialWorker::writeFrame,Qt::QueuedConnection);
            // Scpi -> uart
            QObject::connect(Scpi_process.get(),scpi_signal[config.channel-1],channel.get(),&SerialWorker::writeFrame,Qt::QueuedConnection);

            Uart_Channels.push_back(std::move(channel));
        }

        //QObject::connect(Uart_Channels[0].get(),&SerialWorker::serialDataReceived,Uart_Channels[1].get(),&SerialWorker::writeSerialData);
        //QObject::connect(Uart_Channels[1].get(),&SerialWorker::serialDataReceived,Uart_Channels[0].get(),&SerialWorker::writeSerialData);
        //QTimer::singleShot(300, &app, &QGuiApplication::quit);
    }

    QQmlApplicationEngine engine;
    if (ConfigManager::s_enableDisplay){
        engine.addImportPath(QStringLiteral("qrc:/qml"));
        engine.rootContext()->setContextProperty("Uart_bridge", Uart_bridge.get());

        const QUrl url(QStringLiteral("qrc:/qml/main.qml"));   //main.qml   Component/test.qml
        QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,&app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl){QCoreApplication::exit(-1);}
        }, Qt::QueuedConnection);
        engine.load(url);
    }

    std::unique_ptr<WebServer> webServer;
    if (ConfigManager::s_enableWEBServer){
        webServer = std::make_unique<WebServer>();
        if (!webServer->start()) {return 1;}
    }

    std::unique_ptr<TcpServerManager> vxiServer;
    if (ConfigManager::s_enableLANServer){
        vxiServer = std::make_unique<TcpServerManager>(Scpi_process.get(),Uart_bridge.get());
        if (!vxiServer->startServer()) {return 1;}
    }

    std::unique_ptr<UartServerManager> uartServer;
    if (ConfigManager::s_enableUARTServer){
        uartServer = std::make_unique<UartServerManager>(Scpi_process.get());
        if (!uartServer->startServer("/dev/ttyWCH27",QSerialPort::Baud38400)) {return 1;}
    }

    return app.exec();
}

