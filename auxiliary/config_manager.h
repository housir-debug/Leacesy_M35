#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <QSettings>
#include <QString>

class ConfigManager
{
public:
    static QString s_configFile;
    static QSettings* s_settings;

    static QString s_manufacturer;
    static QString s_model;
    static QString s_serialNumber;
    static QString s_firmwareVersion;

    static QString s_ipAddress;

    static QString s_loglevel;
    static QString s_logdir;
    static QString s_logfilename;
    static int s_maxfilesize;
    static int s_maxfilecount;

    static bool s_enablelogfile;
    static bool s_enableWebServer;
    static bool s_enableVXIServer;
    static bool s_enableDisplay;

    static bool init(const QString &configDir);

private:
    ConfigManager() = delete;
    ~ConfigManager() = delete;
    Q_DISABLE_COPY(ConfigManager)
};

#endif // CONFIG_MANAGER_H
