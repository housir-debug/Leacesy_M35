#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <QSettings>
#include <QString>

class ConfigManager
{
public:
    static QString s_configFile;
    static QSettings* s_settings;
    static bool init(const QString &configDir);

    static QString s_manufacturer;
    static QString s_model;
    static QString s_serialNumber;
    static QString s_firmwareVersion;

    static QString s_ipAddress;
    static bool s_enableWebServer;
    static bool s_enableVXIServer;

    static QString s_loglevel;
    static bool s_enablelogfile;

    static bool s_enableDisplay;

private:
    ConfigManager() = delete;
    ~ConfigManager() = delete;
    Q_DISABLE_COPY(ConfigManager)
};

#endif // CONFIG_MANAGER_H
