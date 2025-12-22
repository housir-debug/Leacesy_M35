#ifndef SIMPLE_LOGGER_H
#define SIMPLE_LOGGER_H

#include <QLoggingCategory>
#include <QString>
#include <QMutex>

//申明避免多次导入-多次创建
Q_DECLARE_LOGGING_CATEGORY(Log)
Q_DECLARE_LOGGING_CATEGORY(app)
Q_DECLARE_LOGGING_CATEGORY(can)
Q_DECLARE_LOGGING_CATEGORY(tcp)
Q_DECLARE_LOGGING_CATEGORY(uart)


struct LoggerConfig {
    bool enableFile = false;              // 是否启用文件日志
    QString logDir = "logs";              // 日志目录
    QString logFileName = "run.log";      // 日志文件名
    qint64 maxFileSize = 6 * 1024 * 1024; // 单个文件最大大小（默认6MB）
    int maxFileCount = 10;                // 最大保留文件数
    QString logLevel = "info";            // 日志级别
};


void initSimpleLogger();
void cleanupLogger();

#endif // SIMPLE_LOGGER_H
