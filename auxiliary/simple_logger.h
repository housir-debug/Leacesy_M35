#pragma once

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(log)

void loggermanage(const QString &loglevel,const QString &parentPath);
void shutdownLogger();
