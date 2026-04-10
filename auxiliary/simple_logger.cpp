#include "simple_logger.h"
#include "auxiliary/config_manager.h"
#include <QMutex>
#include <QDateTime>
#include <QDir>

Q_LOGGING_CATEGORY(log, "LOG:")

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

    // self-define log information procession
    void embeddedMessageHandler(QtMsgType type,const QMessageLogContext &context,const QString &msg) {
        Q_UNUSED(type); // type: debug warning ...

        // context: app log uart ...
        QString formattedMsg = QString("%1:%2").arg(context.category,msg);
        // Console print information
        fprintf(stderr, "%s\n", qPrintable(formattedMsg));

        auto& data = getLoggerData();
        QMutexLocker locker(&data.mutex);

        if (data.stream && data.file && data.file->isOpen()) {
            // record print information
            *data.stream << formattedMsg << "\n";
            // data.stream->flush();   Automatic writing Conserve resources

            // Rotating log files   // 6 MB
            if (data.file->size() > 6291456) {
                data.file->close();

                QString currentPath = data.file->fileName();
                QFileInfo fileInfo(currentPath);

                QString dirPath = fileInfo.absolutePath();
                QString baseName = fileInfo.baseName();
                QString suffix = fileInfo.completeSuffix();
                QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");

                // rename -> change data.file to targeted file
                QString newPath = QDir(dirPath).filePath(QString("%1_%2.%3").arg(baseName,timestamp,suffix));
                if (!QFile::rename(currentPath, newPath)) {
                    // 当处于同一秒内，本次轮转文件名与上次轮转文件名相同，导致重命名失败
                    qCWarning(log) << "[embeddedMessageHandler]:Failed to rename log file to:" << newPath;
                    data.file->open(QIODevice::WriteOnly | QIODevice::Append);
                    return;
                }

                // Filter the files and sort them by date time.
                QDir dir(dirPath);
                QStringList nameFilters = QStringList() << QString("%1_*.%2").arg(baseName,suffix);
                QStringList logFiles = dir.entryList(nameFilters, QDir::Files, QDir::Time);
                // Delete the earliest log file   // 6
                if (logFiles.size() >= 6) {
                    QString oldestPath = QDir(dirPath).filePath(logFiles.last());
                    if (!QFile::remove(oldestPath)) {
                        qWarning(log) << "[embeddedMessageHandler]:Failed to remove old log file:" << oldestPath;
                        data.file->open(QIODevice::WriteOnly | QIODevice::Append);
                        return;
                    }
                }

                // create new run.log
                data.file->setFileName(currentPath);
                data.stream->setDevice(data.file);
                return;
            }
        }

        qWarning(log) << "[embeddedMessageHandler]:Log file openning failed! or data.stream and data.file not exist!" ;
    }
}

void loggermanage(const QString &loglevel,const QString &parentPath) {
    QString rules;
    if (loglevel == "debug"){rules = "*.debug=true\n*.info=true\n*.warning=true";}
    else if (loglevel == "warning") {rules = "*.debug=false\n*.info=false\n*.warning=true";}
    else if (loglevel == "release") {rules = "*.debug=false\n*.info=false\n*.warning=false";}
    else if (loglevel == "self-define"){
        rules = "LOG:.debug=false\n"
                "CAN:.debug=false\n"
                "WEB:.debug=true\n"
                "TCP:.debug=true\n"
                "SCPI:.debug=true\n"
                "LIBTRIPC:.debug=false\n"
                "UART_CHANNEL:.debug=true\n"
                "UART_BRIDGE:.debug=false\n"
                "UART_SERVER:.debug=false\n"
                "*.info=true\n"
                "*.warning=true";
    }else {rules = "";}
    QLoggingCategory::setFilterRules(rules);

    qCDebug(log) << "[loggermanage]:loglevel loaded And configfile reading normal" ;
    if (!ConfigManager::s_enablelogfile){return;}

    // Enable log files
    auto& data = getLoggerData();
    if (data.stream && data.file) {
        data.stream->flush();
        delete data.stream;
        data.stream = nullptr;

        if (data.file->isOpen()) {data.file->close();}
        delete data.file;
        data.file = nullptr;
    }

    // Reset default loader
    qInstallMessageHandler(nullptr);
    QString fullPath = parentPath + "/logs";
    QDir dir(fullPath);
    if (dir.exists() || dir.mkpath(".")) {
        QString logFilePath = QDir(fullPath).filePath("run.log");

        data.file = new QFile(logFilePath); // add file
        if (data.file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            data.stream = new QTextStream(data.file);   // add stream
            data.stream->setCodec("UTF-8");

            *data.stream << "========================================\n";
            *data.stream << "Application Log - Started at: "<< QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
            *data.stream << "========================================\n";

            //registration processing function
            qInstallMessageHandler(embeddedMessageHandler);
            qCDebug(log) << "[loggermanage]:File logging:" << logFilePath;
            return;
        }

        delete data.file;
        data.file = nullptr;
    }

    qCWarning(log) << "[loggermanage]:Failed to create log directory or Failed to open log file!";
}

void shutdownLogger() {
    qInstallMessageHandler(nullptr);

    auto& data = getLoggerData();
    QMutexLocker locker(&data.mutex);

    if (data.stream && data.file) {
        data.stream->flush();
        delete data.stream;
        data.stream = nullptr;

        if (data.file->isOpen()) {data.file->close();}
        delete data.file;
        data.file = nullptr;
    }
}
