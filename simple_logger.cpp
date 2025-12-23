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
    bool rotateLogFile() {

        auto& data = getLoggerData();
        if (!data.file || !data.stream) {return false;}

        data.stream->flush();
        data.file->close();

        QString currentPath = data.file->fileName();
        QFileInfo fileInfo(currentPath);
        QString dirPath = fileInfo.absolutePath();
        QString baseName = fileInfo.baseName();
        QString suffix = fileInfo.completeSuffix();  //文件后缀
        if (suffix.isEmpty()) { suffix = "log";}

        // 重命名新文件
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QString newFileName = QString("%1_%2.%3").arg(baseName).arg(timestamp).arg(suffix);
        QString newPath = QDir(dirPath).filePath(newFileName);
        if (!QFile::rename(currentPath, newPath)) {
            qWarning(log) << "Failed to rename log file to:" << newPath;
            data.file->open(QIODevice::WriteOnly | QIODevice::Append);
            return false;
        }

        // 清理旧文件
        QDir dir(dirPath);
        QStringList nameFilters = QStringList() << QString("%1_*.%2").arg(baseName).arg(suffix);
        QStringList logFiles = dir.entryList(nameFilters, QDir::Files, QDir::Time); // 按时间排序
        while (logFiles.size() >= data.config.maxFileCount) {
            QString oldestFile = logFiles.last();
            QString oldestPath = QDir(dirPath).filePath(oldestFile);
            if (!QFile::remove(oldestPath)) {
                qWarning(log) << "Failed to remove old log file:" << oldestPath;
            }
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
            return false;
        }

        data.stream->setDevice(data.file);
        qInfo(log) << "Log file rotated. New archive:" << newPath;

        return true;
    }

    void embeddedMessageHandler(QtMsgType type,
                                 const QMessageLogContext &context,
                                 const QString &msg) {
        Q_UNUSED(context);   // 编译期静音宏-不提示没使用
        auto& data = getLoggerData();
        QMutexLocker locker(&data.mutex);  // 线程安全

        fprintf(stderr, "%s\n", qPrintable(msg));// 总是输出到控制台（Qt自带颜色）

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
                rotateLogFile();
            }
        }
    }

    bool ensureLogDirectory(const QString& dirPath) {
        QDir dir(dirPath);
        return dir.exists() || dir.mkpath(".");
    }
}

void initSimpleLogger() {
    auto& data = getLoggerData();
    QMutexLocker locker(&data.mutex);

    cleanupLogger();

    if (data.config.enableFile) {
        if (ensureLogDirectory(data.config.logDir)) {
            QString logFilePath = QDir(data.config.logDir).filePath(data.config.logFileName);
            data.file = new QFile(logFilePath);

            if (data.file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
                data.stream = new QTextStream(data.file);
                data.stream->setCodec("UTF-8");

                *data.stream << "========================================\n";
                *data.stream << "Application Log - Started at: "
                            << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
                *data.stream << "========================================\n";
                data.stream->flush();

                qInstallMessageHandler(embeddedMessageHandler);
                qCInfo(log) << "File logging:" << logFilePath;
            } else {
                delete data.file;
                data.file = nullptr;
                data.config.enableFile = false;
            }
        }
        else{qWarning(log) << "Failed to create log directory, file logging disabled";}
    }

    qCInfo(log) << "File:" << (data.config.enableFile ? "on" : "off");
}

void cleanupLogger() {
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
    qCInfo(log) << "Logger cleanup completed";
}




