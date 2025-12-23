#ifndef SIMPLE_LOGGER_H
#define SIMPLE_LOGGER_H

#include <QLoggingCategory>
#include <QString>
#include <QMutex>

Q_DECLARE_LOGGING_CATEGORY(log)
Q_DECLARE_LOGGING_CATEGORY(app)


struct LoggerConfig {
    bool enableFile = false;              // 是否启用文件日志
    QString logDir = "logs";              // 日志目录
    QString logFileName = "run.log";      // 日志文件名
    qint64 maxFileSize = 6 * 1024 * 1024; // 单个文件最大大小（默认6MB）
    int maxFileCount = 10;                // 最大保留文件数

    // 添加构造函数，避免非POD警告
    LoggerConfig() = default;
};


void initSimpleLogger();
void cleanupLogger();

#endif // SIMPLE_LOGGER_H
