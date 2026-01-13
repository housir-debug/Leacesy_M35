#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <QSettings>
#include <QString>

class ConfigManager
{
public:
    static bool init(const QString &configDir);

    static QString getManufacturer() { return s_manufacturer; }
    static QString getModel() { return s_model; }
    static QString getSerialNumber() { return s_serialNumber; }
    static QString getFirmwareVersion() { return s_firmwareVersion; }

    static QString getIPAddress() { return s_ipAddress; }
    static int getWebPort() { return s_webPort; }
    static int getVXIPort() { return s_vxiPort; }

    static bool isWebServerEnabled() { return s_enableWebServer; }
    static bool isVXIEnabled() { return s_enableVXIServer; }

    static QString getConfigFilePath() { return s_configFile; }

private:
    static QString s_configFile;
    static QSettings* s_settings;

    static QString s_manufacturer;
    static QString s_model;
    static QString s_serialNumber;
    static QString s_firmwareVersion;

    static QString s_ipAddress;
    static int s_webPort;
    static int s_vxiPort;

    static bool s_enableWebServer;
    static bool s_enableVXIServer;

    ConfigManager() = delete;
    ~ConfigManager() = delete;
    Q_DISABLE_COPY(ConfigManager)
};

#endif // CONFIG_MANAGER_H
