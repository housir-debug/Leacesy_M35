#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTimer>
#include <QThread>
#include "serialworker.h"
#include "canworker.h"
#include <signal.h>


void SerialManager(const QString &portName)
{
    SerialWorker *serialWorker = new SerialWorker();
    QThread *serialThread = new QThread();
    serialWorker->moveToThread(serialThread);

    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                     serialWorker, &SerialWorker::closeSerial);
    QObject::connect(serialWorker, &SerialWorker::finished,
                     serialThread, &QThread::quit);
    QObject::connect(serialThread, &QThread::finished,
                     serialWorker, &QObject::deleteLater);
    QObject::connect(serialThread, &QThread::finished,
                     serialThread, &QObject::deleteLater);

    QObject::connect(serialWorker, &SerialWorker::serialDataReceived,
                     [](const QByteArray &data) {
                         qDebug() << "[Normal Data]" << data.size() << "bytes";
                     });
    QObject::connect(serialWorker, &SerialWorker::serialErrorOccurred,
                     [](const QString &error) {
                         qWarning() << "[Error]" << error;
                     });
    serialThread->setObjectName(QString("%1_worker").arg(portName));
    serialThread->start();

    bool serialInitialized = false;
    QMetaObject::invokeMethod(serialWorker, [serialWorker, &serialInitialized, &portName]() {
        serialInitialized = serialWorker->initSerialPort(portName, QSerialPort::Baud115200);
    }, Qt::BlockingQueuedConnection);

    if (serialInitialized) {
        //挂起 一秒后执行---在lambda中使用局部变量，但生命周期问题singleShot(1000, [](),所以使用捕获singleShot(1000, [serialWorker]()
        QTimer::singleShot(1000, serialWorker,[serialWorker]() {
            qDebug() << "\n=== Starting Loopback Test ===";
            qDebug() << "Please connect TX and RX pins of the serial port!";

            QMetaObject::invokeMethod(serialWorker, &SerialWorker::startLoopbackTest);
        });

        // 3秒后自动退出
        QTimer::singleShot(3000, QCoreApplication::instance(), &QCoreApplication::quit);

    } else {
        qCritical() << "✗ Failed to open any serial port!";

        QStringList portsToTry = {"/dev/ttyS9", "/dev/ttyS8", "/dev/ttyS7", "/dev/ttyS5", "/dev/ttyS5"};
        bool initialized = false;

        for (const QString &port : portsToTry) {
            qDebug() << "Trying to open" << port << "...";

            QMetaObject::invokeMethod(serialWorker, [serialWorker, port, &initialized]() {
                initialized = serialWorker->initSerialPort(port, QSerialPort::Baud115200);
            }, Qt::BlockingQueuedConnection);

            if (initialized) {
                qDebug() << "✓ Successfully opened" << port;
                break;
            } else {
                qDebug() << "✗ Failed to open" << port;
            }
        }

        QTimer::singleShot(1000, QCoreApplication::instance(), &QCoreApplication::quit);
    }
}

void canmanager()
{
    CanWorker *canWorker = new CanWorker();

    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         canWorker, &CanWorker::closeCan);
    QObject::connect(canWorker, &CanWorker::canClosed,
                     canWorker, &QObject::deleteLater);

    QObject::connect(canWorker, &CanWorker::errorOccurred,
                     [](const QString &error) {
        qCritical() << "CAN Error:" << error;
    });
    QObject::connect(canWorker, &CanWorker::frameReceived,
                     [](quint32 canId, const QByteArray &data, qint64 timestamp) {
        qDebug() << "Received CAN:" << canId << "Data:" << data.toHex() << "time:" << timestamp;
        // 在这里处理接收到的数据
    });

    bool initialized = canWorker->initialize("can0", 1000000);
    if (initialized) {
        canWorker->startListening();
        canWorker->testLoopback();

        // 示例：1秒后发送测试帧
        /*QTimer::singleShot(1000,canWorker,  [canWorker]() {
            QByteArray data = QByteArray::fromHex("1122334455667788");
            canWorker->sendFrame(0x123, data.mid(0, 8)); // 限制8字节
        });//定时器而非延时*/
        // 2秒后自动退出
        QTimer::singleShot(2000, QCoreApplication::instance(), &QCoreApplication::quit);
    }
}

void signalHandler(int signal)
{
    qDebug() << "收到信号:" << signal;
    if (signal == SIGINT) {
        qDebug() << "Ctrl+C 被按下，开始清理...";
        // 调用清理函数
        QCoreApplication::quit();  // 优雅退出
    }

    // 注册信号处理函数-IDE结束信号
    //signal(SIGTERM, signalHandler);
}

int main(int argc, char *argv[])
{
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);

    /*QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);*/

    canmanager();
    //SerialManager("/dev/ttyS4");

    // Ctrl+C 或 kill -9
    // 1. 直接终止进程，不调用任何析构函数
    // 2. 操作系统强制回收所有资源
    return app.exec();
}
