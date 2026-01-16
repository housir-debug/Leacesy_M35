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

void canmanager(const QString &cansocket)
{
    CanWorker *canWorker = new CanWorker();
    QThread *canThread = new QThread();
    canWorker->moveToThread(canThread);

    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         canWorker, &CanWorker::closeCan);
    QObject::connect(canThread, &QThread::finished,
                     canWorker, &QObject::deleteLater);
    QObject::connect(canThread, &QThread::finished,
                     canThread, &QObject::deleteLater);

    canThread->setObjectName("can_worker");
    canThread->start();

    bool Initialized = false;
    QMetaObject::invokeMethod(canWorker, [canWorker, &Initialized, &cansocket]() {
        Initialized = canWorker->initialize(cansocket, 1000000);
    }, Qt::QueuedConnection);//Blocking

    sleep(5);

    if (Initialized) {
         QMetaObject::invokeMethod(canWorker, &CanWorker::testLoopback);
         QTimer::singleShot(6000, QCoreApplication::instance(), &QCoreApplication::quit);
    }
}

void Test_eth_can(const QString &cansocket){
    CanWorker *canWorker = new CanWorker();
    QThread *canThread = new QThread();
    canWorker->moveToThread(canThread);

    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         canWorker, &CanWorker::closeCan);
    QObject::connect(canThread, &QThread::finished,
                     canWorker, &QObject::deleteLater);
    QObject::connect(canThread, &QThread::finished,
                     canThread, &QObject::deleteLater);

    canThread->setObjectName("can_worker");
    canThread->start();

    bool Initialized = false;
    QMetaObject::invokeMethod(canWorker, [canWorker, &Initialized, &cansocket]() {
        Initialized = canWorker->initialize(cansocket, 1000000);
    }, Qt::BlockingQueuedConnection);


    TcpServerManager *tcpServer = new TcpServerManager();
    tcpServer->startServer();

    QObject::connect(canWorker, &CanWorker::frameReceived,
                     tcpServer, &TcpServerManager::forwardCanData);
    QObject::connect(tcpServer, &TcpServerManager::canSendRequest,
                     canWorker, &CanWorker::sendFrame);

    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                     tcpServer, &TcpServerManager::stopServer);
    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                     tcpServer, &QObject::deleteLater);
}

void Test_eth_Serial(const QString &portName)
{
    SerialWorker *serialWorker = new SerialWorker();
    serialWorker->initSerialPort(portName, QSerialPort::Baud115200);

    QObject::connect(QGuiApplication::instance(), &QGuiApplication::aboutToQuit,
                     serialWorker, &SerialWorker::closeSerial);
    QObject::connect(QGuiApplication::instance(), &QGuiApplication::aboutToQuit,
                     serialWorker, &QObject::deleteLater);

    TcpServerManager *tcpServer = new TcpServerManager();
    tcpServer->startServer();

    QObject::connect(serialWorker, &SerialWorker::serialDataReceived,
                     tcpServer, &TcpServerManager::forwardSerialData);
    QObject::connect(tcpServer, &TcpServerManager::SerialSendRequest,
                     serialWorker, &SerialWorker::writeSerialData);

    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                     tcpServer, &TcpServerManager::stopServer);
    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                     tcpServer, &QObject::deleteLater);
}

void Test_can_serial(const QString &cansocket,const QString &portName)
{
    CanWorker *canWorker = new CanWorker();
    QThread *canThread = new QThread();
    canWorker->moveToThread(canThread);

    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         canWorker, &CanWorker::closeCan);
    QObject::connect(canThread, &QThread::finished,
                     canWorker, &QObject::deleteLater);
    QObject::connect(canThread, &QThread::finished,
                     canThread, &QObject::deleteLater);

    canThread->setObjectName("can_worker");
    canThread->start();

    bool Initialized = false;
    QMetaObject::invokeMethod(canWorker, [canWorker, &Initialized, &cansocket]() {
        Initialized = canWorker->initialize(cansocket, 1000000);
    }, Qt::QueuedConnection);//Blocking

    //sleep(1);

    SerialWorker *serialWorker = new SerialWorker();
    serialWorker->initSerialPort(portName, QSerialPort::Baud115200);

    QObject::connect(serialWorker, &SerialWorker::serialDataReceived,
                     canWorker, &CanWorker::forwardSerialData);
    QObject::connect(canWorker, &CanWorker::SerialSendRequest,
                     serialWorker, &SerialWorker::writeSerialData);

    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                     serialWorker, &SerialWorker::closeSerial);
    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                     serialWorker, &QObject::deleteLater);

    QMetaObject::invokeMethod(canWorker, &CanWorker::testserialloop);//存在异步析构耗时开销事件循环问题导致延时
}



int main(int argc, char *argv[])
{
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName("Leacesy Instrument");
    QString appPath = QGuiApplication::applicationDirPath();
    QDir appDir(appPath);

    if (appDir.cdUp()){
        QString parentPath = appDir.absolutePath();
        if (!ConfigManager::init(parentPath)) {return 1;}

        loggermanage(ConfigManager::s_loglevel,parentPath);
        if (ConfigManager::s_enablelogfile){QObject::connect(&app, &QGuiApplication::aboutToQuit, []{shutdownLogger();});}
    }else{return 1;}

    if (ConfigManager::s_enableWebServer){
        WebServer *webServer =  new WebServer(&app);
        if (!webServer->start()) {return 1;}
    }

    if (ConfigManager::s_enableVXIServer){
        TcpServerManager *vxiServer = new TcpServerManager();
        if(!vxiServer->startServer()){return 1;}

        QObject::connect(&app, &QGuiApplication::aboutToQuit,vxiServer, &TcpServerManager::stopServer);
        QObject::connect(&app, &QGuiApplication::aboutToQuit,vxiServer, &QObject::deleteLater);
    }

    if (ConfigManager::s_enableUartMess){
        SerialWorker *Uart_8 = new SerialWorker();
        if(!Uart_8->initSerialPort("/dev/ttyS8", QSerialPort::Baud115200)){return 1;}

        QObject::connect(&app, &QGuiApplication::aboutToQuit,Uart_8, &SerialWorker::closeSerial);
        QObject::connect(&app, &QGuiApplication::aboutToQuit,Uart_8, &QObject::deleteLater);

        QMetaObject::invokeMethod(Uart_8, &SerialWorker::startLoopbackTest);
        //QTimer::singleShot(3000, QGuiApplication::instance(), &QCoreApplication::quit);
    }

    if (ConfigManager::s_enableDisplay){
        QQmlApplicationEngine engine;
        const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
        QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,&app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl){QCoreApplication::exit(-1);}
        }, Qt::QueuedConnection);
        engine.load(url);
    }

    //canmanager("can0");

    //Test_eth_can("can0");
    //Test_eth_Serial("/dev/ttyS4");
    //Test_can_serial("can0","/dev/ttyS4");

    return app.exec();
}

