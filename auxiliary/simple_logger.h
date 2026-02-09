#pragma once

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(log)
Q_DECLARE_LOGGING_CATEGORY(app)

void loggermanage(const QString &loglevel,const QString &parentPath);
void shutdownLogger();
