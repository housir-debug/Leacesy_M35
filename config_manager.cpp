#include "config_manager.h"
#include <QFile>
#include <QDebug>
#include <QCoreApplication>

// ===================== 静态成员初始化 =====================

QString ConfigManager::s_configFile = "instrument_config.ini";
QSettings* ConfigManager::s_settings = nullptr;

QString ConfigManager::s_manufacturer = "Leacesy";
QString ConfigManager::s_model = "Leacesy Instrument";
QString ConfigManager::s_serialNumber = "SN123456789";
QString ConfigManager::s_firmwareVersion = "1.0.0";

QString ConfigManager::s_ipAddress = "127.0.0.1";
int ConfigManager::s_webPort = 80;
int ConfigManager::s_vxiPort = 5025;

bool ConfigManager::s_enableWebServer = true;
bool ConfigManager::s_enableVXIServer = true;

// ===================== 初始化方法 =====================

bool ConfigManager::init()
{
    if (!QFile::exists(s_configFile)) {
        qWarning() << "[Config] File not found:" << s_configFile;
        qWarning() << "[Config] Using default values";
        return false;
    }

    if (s_settings) {delete s_settings;}
    s_settings = new QSettings(s_configFile, QSettings::IniFormat);
    // (setvalue) function change config value

    s_manufacturer = s_settings->value("Device/Manufacturer").toString();
    s_model = s_settings->value("Device/Model").toString();
    s_serialNumber = s_settings->value("Device/SerialNumber").toString();
    s_firmwareVersion = s_settings->value("Device/FirmwareVersion").toString();

    s_ipAddress = s_settings->value("Network/IPAddress").toString();
    s_webPort = s_settings->value("Network/WebPort").toInt();
    s_vxiPort = s_settings->value("Network/VXIPort").toInt();

    s_enableWebServer = s_settings->value("Services/EnableWebServer").toBool();
    s_enableVXIServer = s_settings->value("Services/EnableVXI").toBool();

    return true;
}
