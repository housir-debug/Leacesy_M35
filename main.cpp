#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTimer>
#include <QDir>
#include <QThread>
#include <QLoggingCategory>
#include "vxi_11/tcpserver.h"
#include "vxi_11/web_server.h"
#include "auxiliary/simple_logger.h"
#include "auxiliary/config_manager.h"
#include "serialworker.h"
#include "canworker.h"

void Test_eth_can(const QString &cansocket){
    CanWorker *canWorker = new CanWorker();
    QThread *canThread = new QThread();

    canWorker->moveToThread(canThread);
    canThread->setObjectName("can_worker");
    canThread->start();

    QObject::connect(QGuiApplication::instance(), &QGuiApplication::aboutToQuit,canWorker, &CanWorker::closeCan);
    QObject::connect(canThread, &QThread::finished,canWorker, &QObject::deleteLater);
    QObject::connect(canThread, &QThread::finished,canThread, &QObject::deleteLater);

    QMetaObject::invokeMethod(canWorker, [canWorker, &cansocket]() {
        canWorker->initialize(cansocket, 1000000);
    }, Qt::BlockingQueuedConnection);

    TcpServerManager *tcpServer = new TcpServerManager();
    tcpServer->startServer();

    QObject::connect(QGuiApplication::instance(), &QGuiApplication::aboutToQuit,tcpServer, &TcpServerManager::stopServer);
    QObject::connect(QGuiApplication::instance(), &QGuiApplication::aboutToQuit,tcpServer, &QObject::deleteLater);

    QObject::connect(canWorker, &CanWorker::frameReceived,tcpServer, &TcpServerManager::forwardCanData);
    QObject::connect(tcpServer, &TcpServerManager::canSendRequest,canWorker, &CanWorker::sendFrame);
}

void Test_eth_Serial(const QString &portName)
{
    SerialWorker *serialWorker = new SerialWorker();
    serialWorker->initSerialPort(portName, QSerialPort::Baud115200);

    QObject::connect(QGuiApplication::instance(), &QGuiApplication::aboutToQuit,serialWorker, &SerialWorker::closeSerial);
    QObject::connect(QGuiApplication::instance(), &QGuiApplication::aboutToQuit,serialWorker, &QObject::deleteLater);

    TcpServerManager *tcpServer = new TcpServerManager();
    tcpServer->startServer();

    QObject::connect(QGuiApplication::instance(), &QGuiApplication::aboutToQuit,tcpServer, &TcpServerManager::stopServer);
    QObject::connect(QGuiApplication::instance(), &QGuiApplication::aboutToQuit,tcpServer, &QObject::deleteLater);

    QObject::connect(serialWorker, &SerialWorker::serialDataReceived,tcpServer, &TcpServerManager::forwardSerialData);
    QObject::connect(tcpServer, &TcpServerManager::SerialSendRequest,serialWorker, &SerialWorker::writeSerialData);
}

void Test_can_serial(const QString &cansocket,const QString &portName)
{
    SerialWorker *serialWorker = new SerialWorker();
    serialWorker->initSerialPort(portName, QSerialPort::Baud115200);

    QObject::connect(QGuiApplication::instance(), &QGuiApplication::aboutToQuit,serialWorker, &SerialWorker::closeSerial);
    QObject::connect(QGuiApplication::instance(), &QGuiApplication::aboutToQuit,serialWorker, &QObject::deleteLater);

    CanWorker *canWorker = new CanWorker();
    QThread *canThread = new QThread();

    canWorker->moveToThread(canThread);
    canThread->setObjectName("can_worker");
    canThread->start();

    QObject::connect(QGuiApplication::instance(), &QGuiApplication::aboutToQuit,canWorker, &CanWorker::closeCan);
    QObject::connect(canThread, &QThread::finished,canWorker, &QObject::deleteLater);
    QObject::connect(canThread, &QThread::finished,canThread, &QObject::deleteLater);

    QObject::connect(serialWorker, &SerialWorker::serialDataReceived,canWorker, &CanWorker::forwardSerialData);
    QObject::connect(canWorker, &CanWorker::SerialSendRequest,serialWorker, &SerialWorker::writeSerialData);

    QMetaObject::invokeMethod(canWorker, [canWorker, &cansocket]() {
        canWorker->initialize(cansocket, 1000000);
        canWorker->testserialloop();
    }, Qt::BlockingQueuedConnection);
}



int main(int argc, char *argv[])
{
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

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

    std::unique_ptr<WebServer> webServer;
    if (ConfigManager::s_enableWebServer){
        webServer.reset(new WebServer(&app));
        if (!webServer->start()) {return 1;}
    }

    std::unique_ptr<TcpServerManager> vxiServer;
    if (ConfigManager::s_enableVXIServer){
        vxiServer.reset(new TcpServerManager());
        if(!vxiServer->startServer()){return 1;}
    }

    std::unique_ptr<SerialWorker> Uart_4;
    std::unique_ptr<SerialWorker> Uart_5;
    if (ConfigManager::s_enableUartMess){
        Uart_4.reset(new SerialWorker());
        if(!Uart_4->initSerialPort("/dev/ttyS4", QSerialPort::Baud38400)){return 1;}
        Uart_5.reset(new SerialWorker());
        if(!Uart_5->initSerialPort("/dev/ttyS5", QSerialPort::Baud38400)){return 1;}

        //QMetaObject::invokeMethod(Uart_4.get(), &SerialWorker::startLoopbackTest);
        //QTimer::singleShot(300, &app, &QGuiApplication::quit);
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

    QQmlApplicationEngine engine;
    if (ConfigManager::s_enableDisplay){
        engine.addImportPath(QStringLiteral("qrc:/qml"));
        const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
        QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,&app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl){QCoreApplication::exit(-1);}
        }, Qt::QueuedConnection);
        engine.load(url);
    }

    //Test_eth_can("can1");
    //Test_eth_Serial("/dev/ttyS5");
    //Test_can_serial("can0","/dev/ttyS4");

    return app.exec();
}

