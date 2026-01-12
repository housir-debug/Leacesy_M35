#include "simple_logger.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QCoreApplication>


Q_LOGGING_CATEGORY(log, "log:")
Q_LOGGING_CATEGORY(app, "app:")

namespace {
    struct LoggerData {
        LoggerConfig config;
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
}

namespace {
    void embeddedMessageHandler(QtMsgType type,const QMessageLogContext &context,const QString &msg) {
        Q_UNUSED(context);
        fprintf(stderr, "%s\n", qPrintable(msg));// 总是输出到控制台（Qt自带颜色）

        auto& data = getLoggerData();

        if (data.stream && data.file && data.file->isOpen()) {
            QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
            QString levelStr;

            switch(type) {
                case QtDebugMsg:    levelStr = "DEBUG"; break;
                case QtInfoMsg:     levelStr = "INFO"; break;
                case QtWarningMsg:  levelStr = "WARN"; break;
                case QtCriticalMsg: levelStr = "ERROR"; break;
                case QtFatalMsg:    levelStr = "FATAL"; break;
            }

            *data.stream << timestamp << " [" << levelStr << ":] " << msg << "\n";
            data.stream->flush();

            if (data.file->size() > data.config.maxFileSize) {
                data.file->close();

                QString currentPath = data.file->fileName();
                QFileInfo fileInfo(currentPath);
                QString dirPath = fileInfo.absolutePath();
                QString baseName = fileInfo.baseName();
                QString suffix = fileInfo.completeSuffix();
                if (suffix.isEmpty()) { suffix = "log";}
                QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");

                // 重命名当前轮转文件
                QString newFileName = QString("%1_%2.%3").arg(baseName,timestamp,suffix);
                QString newPath = QDir(dirPath).filePath(newFileName);
                if (!QFile::rename(currentPath, newPath)) {
                    qCWarning(log) << "Failed to rename log file to:" << newPath;
                    data.file->open(QIODevice::WriteOnly | QIODevice::Append);
                    return;
                }

                // 删除最早的日志文件
                QDir dir(dirPath);
                QStringList nameFilters = QStringList() << QString("%1_*.%2").arg(baseName,suffix);
                QStringList logFiles = dir.entryList(nameFilters, QDir::Files, QDir::Time); // 按时间排序
                while (logFiles.size() >= data.config.maxFileCount) {
                    QString oldestFile = logFiles.last();
                    QString oldestPath = QDir(dirPath).filePath(oldestFile);
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
                qCDebug(log) << "Log file rotated. New archive:" << newPath;
            }
        }
    }
}

void initSimpleLogger() {
    auto& data = getLoggerData();
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

    qInstallMessageHandler(nullptr);  // 恢复默认处理器

    if (data.config.enableFile) {
        QDir dir(data.config.logDir);
        if (dir.exists() || dir.mkpath(".")) {
            QString logFilePath = QDir(data.config.logDir).filePath(data.config.logFileName);
            data.file = new QFile(logFilePath);

            if (data.file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
                data.stream = new QTextStream(data.file);   // goal
                data.stream->setCodec("UTF-8");

                *data.stream << "========================================\n";
                *data.stream << "Application Log - Started at: "<< QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
                *data.stream << "========================================\n";
                data.stream->flush();

                qInstallMessageHandler(embeddedMessageHandler);   //registration processing function
                qCDebug(log) << "File logging:" << logFilePath;
            } else {
                delete data.file;
                data.file = nullptr;
                data.config.enableFile = false;
                qCWarning(log) << "Failed to open log file!";
            }
        }
        else{qCWarning(log) << "Failed to create log directory, file logging disabled";}
    }

    qCDebug(log) << "File:" << (data.config.enableFile ? "on" : "off");
}

void loggermanage(const QString &loglevel){
    QString rules;
    if (loglevel == "debug")        {rules = "*.debug=true\n*.info=true\n*.warning=true";}
    else if (loglevel == "warning") {rules = "*.debug=false\n*.info=false\n*.warning=true";}
    else {rules = "*.debug=false\n*.info=false\n*.warning=true";}
    QLoggingCategory::setFilterRules(rules);

    //格式化加时间，比直接时间戳延时更多
    //qSetMessagePattern("[%{time HH:mm:ss.zzzzzz}] [%{category}] %{message}");
}



