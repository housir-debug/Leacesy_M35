#include <QDir>
#include <QThread>
#include <QQmlContext>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "auxiliary/simple_logger.h"
#include "auxiliary/config_manager.h"
#include "auxiliary/scpi_handle.h"
#include "channel/uart_channel.h"
#include "vxi_11/tcp_server.h"
#include "qml/web_server.h"
#include "qml/qml_agency.h"
#include "canworker.h"

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

    QQmlApplicationEngine engine;
    std::unique_ptr<SerialBridge> Uart_bridge;
    if (ConfigManager::s_enableDisplay){
        Uart_bridge = std::make_unique<SerialBridge>();
        engine.rootContext()->setContextProperty("Uart_bridge", Uart_bridge.get());
        engine.addImportPath(QStringLiteral("qrc:/qml"));

        const QUrl url(QStringLiteral("qrc:/qml/main.qml"));   //main.qml   Component/test.qml
        QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,&app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl){QCoreApplication::exit(-1);}
        }, Qt::QueuedConnection);
        engine.load(url);
    }

    QHash<SerialWorker*,quint8> UartChannel_Map;
    std::vector<std::unique_ptr<SerialWorker>> Uart_Channels;
    if (ConfigManager::s_enableUartMess){
        for (const auto& config : configs) {
            auto channel = std::make_unique<SerialWorker>();
            if (!channel->initSerialPort(config.port, config.baudRate)) {return 1;}

            if (ConfigManager::s_enableDisplay){
                // Uart -> Qml-UI
                QObject::connect(channel.get(),&SerialWorker::voltageChanged,Uart_bridge.get(),&SerialBridge::update_Voltage,Qt::DirectConnection);
                QObject::connect(channel.get(),&SerialWorker::currentChanged,Uart_bridge.get(),&SerialBridge::update_CurrentAndUnit,Qt::DirectConnection);
                QObject::connect(channel.get(),&SerialWorker::statusChanged,Uart_bridge.get(),&SerialBridge::update_status,Qt::DirectConnection);
                // QmlUI -> Uart
                // The order correspondence is not one-to-one binding.
                QObject::connect(Uart_bridge.get(),qml_signal[config.channel-1],channel.get(),&SerialWorker::writeFrame,Qt::QueuedConnection);
            }

            UartChannel_Map[channel.get()] = config.channel-1;
            Uart_Channels.push_back(std::move(channel));
        }

        //QObject::connect(Uart_Channels[0].get(),&SerialWorker::serialDataReceived,Uart_Channels[1].get(),&SerialWorker::writeSerialData);
        //QObject::connect(Uart_Channels[1].get(),&SerialWorker::serialDataReceived,Uart_Channels[0].get(),&SerialWorker::writeSerialData);
        //QTimer::singleShot(300, &app, &QGuiApplication::quit);
    }

    std::unique_ptr<ScpiManager> vxi_scpi;
    std::unique_ptr<TcpServerManager> vxiServer;
    std::unique_ptr<WebServer> webServer;
    if (ConfigManager::s_enableVXIServer){
        webServer = std::make_unique<WebServer>();
        if (!webServer->start()) {return 1;}
        vxi_scpi = std::make_unique<ScpiManager>();
        vxiServer = std::make_unique<TcpServerManager>(vxi_scpi.get());
        if (!vxiServer->startServer()) {return 1;}

        for (size_t i = 0; i < Uart_Channels.size(); ++i) {
            // Uart -> scpi
            QObject::connect(Uart_Channels[i].get(),&SerialWorker::channelreturnstatus,vxi_scpi.get(),&ScpiManager::processCHStateResponse,Qt::DirectConnection);
            QObject::connect(Uart_Channels[i].get(),&SerialWorker::channelreturnvalue,vxi_scpi.get(),&ScpiManager::processCHvalueResponse,Qt::DirectConnection);

            // Scpi -> uart
            auto it = UartChannel_Map.find(Uart_Channels[i].get());
            if (it != UartChannel_Map.end()) {
                 QObject::connect(vxi_scpi.get(),scpi_signal[it.value()],Uart_Channels[i].get(),&SerialWorker::writeFrame,Qt::QueuedConnection);
            }
        }
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

    return app.exec();
}

