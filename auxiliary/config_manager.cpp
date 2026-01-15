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

QString ConfigManager::s_loglevel = "debug";
QString ConfigManager::s_logdir = "logs";
QString ConfigManager::s_logfilename = "run.log";
int ConfigManager::s_maxfilesize = 6291456;
int ConfigManager::s_maxfilecount = 10;

bool ConfigManager::s_enablelogfile = false;
bool ConfigManager::s_enableWebServer = true;
bool ConfigManager::s_enableVXIServer = true;
bool ConfigManager::s_enableDisplay = true;

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

    s_loglevel = s_settings->value("Logger/logLevel").toString();
    s_logdir = s_settings->value("Logger/LogDir").toString();
    s_logfilename = s_settings->value("Logger/LogfileNmae").toString();
    s_maxfilesize = s_settings->value("Logger/MaxfileSize").toInt();
    s_maxfilecount = s_settings->value("Logger/MaxfileCount").toInt();

    s_enablelogfile = s_settings->value("Switch/EnablelogFile").toBool();
    s_enableWebServer = s_settings->value("Switch/EnableWebServer").toBool();
    s_enableVXIServer = s_settings->value("Switch/EnableVXIServer").toBool();
    s_enableDisplay = s_settings->value("Switch/EnableDisplay").toBool();

    return true;
}
