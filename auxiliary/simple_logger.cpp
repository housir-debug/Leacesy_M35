#include "simple_logger.h"
#include "auxiliary/config_manager.h"
#include <QMutex>
#include <QDateTime>
#include <QDir>


Q_LOGGING_CATEGORY(log, "log:")
Q_LOGGING_CATEGORY(app, "app:")

namespace {
    struct LoggerData {
        QMutex mutex;
        QFile* file = nullptr;
        QTextStream* stream = nullptr;

        LoggerData() = default;
        ~LoggerData() {
            if (stream) delete stream;
            if (file) delete file;
        }
    };

    LoggerData& getLoggerData() {
        static LoggerData data;
        return data;
    }

    void embeddedMessageHandler(QtMsgType type,const QMessageLogContext &context,const QString &msg) {
        Q_UNUSED(type); //debug warning ...
        QString formattedMsg = QString("%1:%2").arg(context.category,msg);
        fprintf(stderr, "%s\n", qPrintable(formattedMsg));

        auto& data = getLoggerData();
        if (data.stream && data.file && data.file->isOpen()) {
            QMutexLocker locker(&data.mutex);

            *data.stream << formattedMsg << "\n";
            data.stream->flush();

            if (data.file->size() > 6291456) {  // 6 MB
                data.file->close();

                QString currentPath = data.file->fileName();
                QFileInfo fileInfo(currentPath);

                QString dirPath = fileInfo.absolutePath();
                QString baseName = fileInfo.baseName();
                QString suffix = fileInfo.completeSuffix();
                if (suffix.isEmpty()) { suffix = "log";}
                QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");

                // 重命名(当处于同一秒内，文件名称与上次轮转相同，导致重命名失败)
                QString newPath = QDir(dirPath).filePath(QString("%1_%2.%3").arg(baseName,timestamp,suffix));
                if (!QFile::rename(currentPath, newPath)) {
                    qCWarning(log) << "Failed to rename log file to:" << newPath;
                    data.file->open(QIODevice::WriteOnly | QIODevice::Append);
                    return;
                }

                qCDebug(log) << "Log file rotated. New archive:" << newPath;

                // 删除最早的日志文件
                QDir dir(dirPath);
                QStringList nameFilters = QStringList() << QString("%1_*.%2").arg(baseName,suffix);
                QStringList logFiles = dir.entryList(nameFilters, QDir::Files, QDir::Time); // 按时间排序
                if (logFiles.size() >= 6) {  // file counts
                    QString oldestPath = QDir(dirPath).filePath(logFiles.last());
                    if (!QFile::remove(oldestPath)) {qWarning(log) << "Failed to remove old log file:" << oldestPath;}
                    logFiles.removeLast();
                }

                // 创建新的日志文件
                data.file->setFileName(currentPath);
                if (!data.file->open(QIODevice::WriteOnly | QIODevice::Append)) {
                    qCritical(log) << "Failed to reopen log file after rotation:" << currentPath;
                    delete data.file;
                    data.file = nullptr;
                    delete data.stream;
                    data.stream = nullptr;
                    return;
                }

                data.stream->setDevice(data.file);
            }
        }else{qCDebug(log) << "Log file openning failed!!!" ;};
    }
}

void loggermanage(const QString &loglevel,const QString &parentPath) {
    QString rules;
    if (loglevel == "debug"){rules = "*.debug=true\n*.info=true\n*.warning=true";}
    else if (loglevel == "warning") {rules = "*.debug=false\n*.info=false\n*.warning=true";}
    else if (loglevel == "release") {rules = "*.debug=false\n*.info=false\n*.warning=false";}
    else if (loglevel == "self-define"){
        rules = "app.debug=false"
                "log.debug=false"
                "can.debug=false"
                "web.debug=false"
                "tcp.debug=false"
                "scpi.debug=false"
                "libtripc.debug=false"
                "uart_channel.debug=false"
                "uart_bridge.debug=false"
                "*.info=false"
                "*.warning=false";
    }else {rules = "";}
    QLoggingCategory::setFilterRules(rules);

    //格式化加时间，比直接时间戳延时更多
    //qSetMessagePattern("[%{time HH:mm:ss.zzzzzz}] [%{category}] %{message}");

    qCDebug(app) << "config file write&reading normal" ;
    if (!ConfigManager::s_enablelogfile){return;}

    auto& data = getLoggerData();
    if (data.stream) {
        data.stream->flush();
        delete data.stream;
        data.stream = nullptr;
    }

    if (data.file) {
        if (data.file->isOpen()) {data.file->close();}
        delete data.file;
        data.file = nullptr;
    }

    qInstallMessageHandler(nullptr);  // 恢复默认处理器
    QString fullPath = parentPath + "/logs";

    QDir dir(fullPath);
    if (dir.exists() || dir.mkpath(".")) {
        QString logFilePath = QDir(fullPath).filePath("run.log");
        data.file = new QFile(logFilePath);

        if (data.file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            data.stream = new QTextStream(data.file);   // goal
            data.stream->setCodec("UTF-8");

            *data.stream << "========================================\n";
            *data.stream << "Application Log - Started at: "<< QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
            *data.stream << "========================================\n";

            qInstallMessageHandler(embeddedMessageHandler);   //registration processing function
            qCDebug(log) << "File logging:" << logFilePath;
        } else {
            delete data.file;
            data.file = nullptr;
            qCWarning(log) << "Failed to open log file!";
        }
    }
    else{qCWarning(log) << "Failed to create log directory, file logging disabled";}
}

void shutdownLogger() {
    auto& data = getLoggerData();
    qInstallMessageHandler(nullptr);

    QMutexLocker locker(&data.mutex);

    if (data.stream) {
        data.stream->flush();
        delete data.stream;
        data.stream = nullptr;
    }

    if (data.file) {
        if (data.file->isOpen()) {data.file->close();}
        delete data.file;
        data.file = nullptr;
    }
}


