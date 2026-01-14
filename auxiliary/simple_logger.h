#ifndef SIMPLE_LOGGER_H
#define SIMPLE_LOGGER_H

#include <QLoggingCategory>
#include <QString>
#include <QMutex>
#include "auxiliary/config_manager.h"

Q_DECLARE_LOGGING_CATEGORY(log)
Q_DECLARE_LOGGING_CATEGORY(app)

void loggermanage(const QString &loglevel,const QString &parentPath);
void shutdownLogger();

#endif // SIMPLE_LOGGER_H
