#pragma once
#include <QSerialPort>
#include <QSettings>
#include <QString>

#define CHANNEL_1_TO_33 \
    CHANNEL(1) CHANNEL(2) CHANNEL(3) CHANNEL(4) CHANNEL(5) CHANNEL(6) CHANNEL(7) CHANNEL(8) \
    CHANNEL(9) CHANNEL(10) CHANNEL(11) CHANNEL(12) CHANNEL(13) CHANNEL(14) CHANNEL(15) CHANNEL(16) \
    CHANNEL(17) CHANNEL(18) CHANNEL(19) CHANNEL(20) CHANNEL(21) CHANNEL(22) CHANNEL(23) CHANNEL(24) \
    CHANNEL(25) CHANNEL(26) CHANNEL(27) CHANNEL(28) CHANNEL(29) CHANNEL(30) CHANNEL(31) CHANNEL(32) \
    CHANNEL(33)

struct UartConfig {
    QString port;
    QSerialPort::BaudRate baudRate;
    quint8 channel;
};

extern std::vector<UartConfig> configs;

class ConfigManager
{
public:
    static bool init(const QString &configDir);
    static QSettings* s_settings;
    static QString s_configFile;

    static QString s_manufacturer;
    static QString s_model;
    static QString s_serialNumber;
    static QString s_firmwareVersion;

    static QString s_loglevel;
    static bool s_enablelogfile;

    static bool s_enableUartMess;
    static bool s_enableCanMess;

    static bool s_enableLANServer;
    static bool s_enableWEBServer;
    static bool s_enableUARTServer;
    static bool s_enableDisplay;

private:
    ConfigManager() = delete;
    ~ConfigManager() = delete;
    Q_DISABLE_COPY(ConfigManager)
};
