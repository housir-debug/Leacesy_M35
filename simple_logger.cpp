#include "simple_logger.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QCoreApplication>


namespace {
    LoggerConfig s_loggerConfig;
    QMutex s_logMutex;
    QFile* s_logFile = nullptr;
    QTextStream* s_logStream = nullptr;
}

Q_LOGGING_CATEGORY(log, "log:")
Q_LOGGING_CATEGORY(app, "app:")
Q_LOGGING_CATEGORY(can, "can:")
Q_LOGGING_CATEGORY(tcp, "tcp:")
Q_LOGGING_CATEGORY(uart, "uart:")


static bool rotateLogFile() {
    if (!s_logFile || !s_logStream) return false;

    s_logStream->flush();
    s_logFile->close();

    QString currentPath = s_logFile->fileName();
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
        s_logFile->open(QIODevice::WriteOnly | QIODevice::Append);
        return false;
    }

    // 清理旧文件
    QDir dir(dirPath);
    QStringList nameFilters = QStringList() << QString("%1_*.%2").arg(baseName).arg(suffix);
    QStringList logFiles = dir.entryList(nameFilters, QDir::Files, QDir::Time); // 按时间排序
    while (logFiles.size() >= s_loggerConfig.maxFileCount) {
        QString oldestFile = logFiles.last();
        QString oldestPath = QDir(dirPath).filePath(oldestFile);
        if (!QFile::remove(oldestPath)) {
            qWarning(log) << "Failed to remove old log file:" << oldestPath;
        }
        logFiles.removeLast();
    }

    // 创建新的日志文件
    s_logFile->setFileName(currentPath);
    if (!s_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        qCritical(log) << "Failed to reopen log file after rotation:" << currentPath;
        delete s_logFile;
        s_logFile = nullptr;
        delete s_logStream;
        s_logStream = nullptr;
        return false;
    }

    s_logStream->setDevice(s_logFile);
    qInfo(log) << "Log file rotated. New archive:" << newPath;

    return true;
}

static void embeddedMessageHandler(QtMsgType type,
                                 const QMessageLogContext &context,
                                 const QString &msg) {
    Q_UNUSED(context);   // 编译期静音宏-不提示没使用

    QMutexLocker locker(&s_logMutex);  // 线程安全

    fprintf(stderr, "%s\n", qPrintable(msg));// 总是输出到控制台（Qt自带颜色）

    if (s_logStream && s_logFile && s_logFile->isOpen()) {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
        QString levelStr;

        switch(type) {
            case QtDebugMsg:    levelStr = "DEBUG"; break;
            case QtInfoMsg:     levelStr = "INFO"; break;
            case QtWarningMsg:  levelStr = "WARN"; break;
            case QtCriticalMsg: levelStr = "ERROR"; break;
            case QtFatalMsg:    levelStr = "FATAL"; break;
        }

        *s_logStream << timestamp << " [" << levelStr << ":] " << msg << "\n";
        s_logStream->flush();

        if (s_logFile->size() > s_loggerConfig.maxFileSize) {
            rotateLogFile();
        }
    }
}

static bool ensureLogDirectory(const QString& dirPath) {
    QDir dir(dirPath);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning(log) << "Failed to create log directory:" << dirPath;
            return false;
        }
        qDebug(log) << "Created log directory:" << dirPath;
    }
    return true;
}


void initSimpleLogger() {
    QMutexLocker locker(&s_logMutex);

    if (s_logFile || s_logStream) {
        cleanupLogger();
    }

    qSetMessagePattern("[%{time HH:mm:ss}] [%{category}] %{message}");

    QString rules;
    if (s_loggerConfig.logLevel == "debug") {
        rules = "*.debug=true\n*.info=true\n*.warning=true";
    } else if (s_loggerConfig.logLevel == "info") {
        rules = "*.debug=false\n*.info=true\n*.warning=true";
    } else if (s_loggerConfig.logLevel == "warning") {
        rules = "*.debug=false\n*.info=false\n*.warning=true";
    } else {
        rules = "*.debug=false\n*.info=true\n*.warning=true";
    }
    QLoggingCategory::setFilterRules(rules);

    if (s_loggerConfig.enableFile) {
        if (!ensureLogDirectory(s_loggerConfig.logDir)) {
            qWarning(log) << "Failed to create log directory, file logging disabled";
            s_loggerConfig.enableFile = false;
        } else {
            QString logFilePath = QDir(s_loggerConfig.logDir).filePath(s_loggerConfig.logFileName);
            s_logFile = new QFile(logFilePath);

            if (!s_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
                qCritical(log) << "Failed to open log file:" << logFilePath
                           << "Error:" << s_logFile->errorString();
                delete s_logFile;
                s_logFile = nullptr;
                s_loggerConfig.enableFile = false;
            } else {
                s_logStream = new QTextStream(s_logFile);
                s_logStream->setCodec("UTF-8");

                *s_logStream << "========================================\n";
                *s_logStream << "Application Log - Started at: "
                            << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
                *s_logStream << "Log Level: " << s_loggerConfig.logLevel << "\n";
                *s_logStream << "Max File Size: " << (s_loggerConfig.maxFileSize / (1024*1024)) << " MB\n";
                *s_logStream << "Max File Count: " << s_loggerConfig.maxFileCount << "\n";
                *s_logStream << "========================================\n";
                s_logStream->flush();

                qInstallMessageHandler(embeddedMessageHandler);

                qCInfo(Log) << "File logging enabled:" << logFilePath;
                qCInfo(Log) << "Max file size:" << (s_loggerConfig.maxFileSize / (1024*1024)) << "MB";
                qCInfo(Log) << "Max file count:" << s_loggerConfig.maxFileCount;
            }
        }
    }

    qCInfo(Log) << "Logger initialized. Level:" << s_loggerConfig.logLevel
                << "File logging:" << (s_loggerConfig.enableFile ? "enabled" : "disabled");
}

void cleanupLogger() {
    if (s_logStream) {
        s_logStream->flush();
        delete s_logStream;
        s_logStream = nullptr;
    }

    if (s_logFile) {
        if (s_logFile->isOpen()) {
            s_logFile->close();
        }
        delete s_logFile;
        s_logFile = nullptr;
    }

    qInstallMessageHandler(nullptr);  // 恢复默认处理器
    qCInfo(Log) << "Logger cleanup completed";
}




