#include <QDir>
#include <QThread>
#include <QQmlContext>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "auxiliary/simple_logger.h"
#include "auxiliary/config_manager.h"
#include "auxiliary/scpimanager.h"
#include "vxi_11/tcpserver.h"
#include "vxi_11/web_server.h"
#include "serialworker.h"
#include "uartmanager.h"
#include "canworker.h"

int main(int argc, char *argv[])
{
    //qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setApplicationName("Leacesy Instrument");
    QGuiApplication app(argc, argv);

    QString appPath = QGuiApplication::applicationDirPath();
    QDir appDir(appPath);

    if (appDir.cdUp()){
        QString parentPath = appDir.absolutePath();
        if (!ConfigManager::init(parentPath)) {return 1;}

        loggermanage(ConfigManager::s_loglevel,parentPath);
        if (ConfigManager::s_enablelogfile){QObject::connect(&app, &QGuiApplication::aboutToQuit, []{shutdownLogger();});}
    }else{return 1;}

    QQmlApplicationEngine engine;
    std::unique_ptr<SerialBridge> Uart_bridge;
    if (ConfigManager::s_enableDisplay){
        Uart_bridge.reset(new SerialBridge());
        engine.rootContext()->setContextProperty("Uart_bridge", Uart_bridge.get());
        engine.addImportPath(QStringLiteral("qrc:/qml"));
        const QUrl url(QStringLiteral("qrc:/qml/main.qml"));   //main.qml   Component/test.qml
        QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,&app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl){QCoreApplication::exit(-1);}
        }, Qt::QueuedConnection);
        engine.load(url);
    }

    std::unique_ptr<SerialWorker> Uart_4;
    std::unique_ptr<SerialWorker> Uart_5;
    if (ConfigManager::s_enableUartMess){
        Uart_4.reset(new SerialWorker());
        if(!Uart_4->initSerialPort("/dev/ttyS4", QSerialPort::Baud38400)){return 1;}
        /*Uart_5.reset(new SerialWorker());
        if(!Uart_5->initSerialPort("/dev/ttyS5", QSerialPort::Baud38400)){return 1;}*/

        //QObject::connect(Uart_4.get(),&SerialWorker::serialDataReceived,Uart_5.get(),&SerialWorker::writeSerialData);
        //QObject::connect(Uart_5.get(),&SerialWorker::serialDataReceived,Uart_4.get(),&SerialWorker::writeSerialData);

        QObject::connect(Uart_4.get(),&SerialWorker::voltageChanged,Uart_bridge.get(),&SerialBridge::update_Voltage,Qt::DirectConnection);
        QObject::connect(Uart_4.get(),&SerialWorker::currentChanged,Uart_bridge.get(),&SerialBridge::update_CurrentAndUnit,Qt::DirectConnection);
        QObject::connect(Uart_4.get(),&SerialWorker::statusChanged,Uart_bridge.get(),&SerialBridge::update_status,Qt::DirectConnection);
        QObject::connect(Uart_bridge.get(),&SerialBridge::sendFrame_Uart4,Uart_4.get(),&SerialWorker::writeFrame,Qt::QueuedConnection);

        // QObject::connect(Uart_5.get(),&SerialWorker::voltageChanged,Uart_bridge.get(),&SerialBridge::update_Uart5_Voltage);
        // QObject::connect(Uart_5.get(),&SerialWorker::currentChanged,Uart_bridge.get(),&SerialBridge::update_Uart5_Current);
        // QObject::connect(Uart_bridge.get(),&SerialBridge::sendFrame_Uart5,Uart_5.get(),&SerialWorker::writeFrame);

        //QTimer::singleShot(300, &app, &QGuiApplication::quit);
    }

    std::unique_ptr<WebServer> webServer;
    if (ConfigManager::s_enableWebServer){
        webServer.reset(new WebServer());
        if (!webServer->start()) {return 1;}
    }

    std::unique_ptr<ScpiManager> vxiscpi;
    std::unique_ptr<TcpServerManager> vxiServer;
    if (ConfigManager::s_enableVXIServer){
        vxiscpi.reset(new ScpiManager());
        vxiServer.reset(new TcpServerManager(vxiscpi.get()));
        if(!vxiServer->startServer()){return 1;}

        QObject::connect(Uart_4.get(),&SerialWorker::channelreturnstatus,vxiscpi.get(),&ScpiManager::processCHStateResponse,Qt::DirectConnection);
        QObject::connect(Uart_4.get(),&SerialWorker::channelreturnvalue,vxiscpi.get(),&ScpiManager::processCHvalueResponse,Qt::DirectConnection);
        QObject::connect(vxiscpi.get(),&ScpiManager::sendFrame_Uart4,Uart_4.get(),&SerialWorker::writeFrame,Qt::QueuedConnection);
    }

    std::unique_ptr<CanWorker> canWorker;
    std::unique_ptr<QThread> canThread;
    if (ConfigManager::s_enableCanMess){
        canWorker.reset(new CanWorker());
        canThread.reset(new QThread());
        canWorker->moveToThread(canThread.get());
        canThread->setObjectName("can_worker");
        canThread->start();

        CanWorker* workerPtr = canWorker.get();
        QMetaObject::invokeMethod(canWorker.get(), [workerPtr]() {
            workerPtr->initialize("all", 1000000);  // all
            workerPtr->testLoopback();
        }, Qt::QueuedConnection);//Blocking
        //QTimer::singleShot(300, &app, &QGuiApplication::quit);
    }

    return app.exec();
}

