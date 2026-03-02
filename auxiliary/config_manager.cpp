#include "config_manager.h"
#include <QFile>

// ===================== 静态成员初始化 =====================

QString ConfigManager::s_configFile = "instrument_config.ini";
QSettings* ConfigManager::s_settings = nullptr;

QString ConfigManager::s_manufacturer = "Leacesy";
QString ConfigManager::s_model = "Leacesy Instrument";
QString ConfigManager::s_serialNumber = "SN123456789";
QString ConfigManager::s_firmwareVersion = "1.0.0";

QString ConfigManager::s_ipAddress = "127.0.0.1";
bool ConfigManager::s_enableVXIServer = true;

QString ConfigManager::s_loglevel = "debug";
bool ConfigManager::s_enablelogfile = false;

bool ConfigManager::s_enableDisplay = true;

bool ConfigManager::s_enableUartMess = true;

bool ConfigManager::s_enableCanMess = true;

// ===================== 初始化方法 =====================

bool ConfigManager::init(const QString &configDir)
{
    QString fullPath = configDir + "/" + s_configFile;
    if (!QFile::exists(fullPath)) {return false;}

    if (s_settings) {delete s_settings;}
    s_settings = new QSettings(fullPath, QSettings::IniFormat);
    // (setvalue) function change config value

    s_manufacturer = s_settings->value("Device/Manufacturer").toString();
    s_model = s_settings->value("Device/Model").toString();
    s_serialNumber = s_settings->value("Device/SerialNumber").toString();
    s_firmwareVersion = s_settings->value("Device/FirmwareVersion").toString();

    s_ipAddress = s_settings->value("Network/IPAddress").toString();
    s_enableVXIServer = s_settings->value("Network/EnableVXIServer").toBool();

    s_loglevel = s_settings->value("Logger/logLevel").toString();
    s_enablelogfile = s_settings->value("Logger/EnablelogFile").toBool();

    s_enableDisplay = s_settings->value("Display/EnableDisplay").toBool();

    s_enableUartMess = s_settings->value("Uart/EnableUartMess").toBool();

    s_enableCanMess = s_settings->value("Can/EnableCanMess").toBool();

    return true;
}
