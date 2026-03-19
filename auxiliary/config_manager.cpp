#include "config_manager.h"
#include <QFile>

std::vector<UartConfig> configs = {
    //{"/dev/ttyS3",    QSerialPort::Baud38400, 0x01}, // debug-Uart
    /*{"/dev/ttyS4",    QSerialPort::Baud38400, 0x01},
    {"/dev/ttyS5",    QSerialPort::Baud38400, 0x02},
    {"/dev/ttyS7",    QSerialPort::Baud38400, 0x03},
    {"/dev/ttyS8",    QSerialPort::Baud38400, 0x04},
    {"/dev/ttyS9",    QSerialPort::Baud38400, 0x05},
    {"/dev/ttyWCH0",  QSerialPort::Baud38400, 0x06},
    {"/dev/ttyWCH1",  QSerialPort::Baud38400, 0x07},
    {"/dev/ttyWCH2",  QSerialPort::Baud38400, 0x08},
    {"/dev/ttyWCH3",  QSerialPort::Baud38400, 0x09},
    {"/dev/ttyWCH4",  QSerialPort::Baud38400, 0x0a},
    {"/dev/ttyWCH5",  QSerialPort::Baud38400, 0x0b},
    {"/dev/ttyWCH6",  QSerialPort::Baud38400, 0x0c},
    {"/dev/ttyWCH7",  QSerialPort::Baud38400, 0x0d},
    {"/dev/ttyWCH8",  QSerialPort::Baud38400, 0x0e},
    {"/dev/ttyWCH9",  QSerialPort::Baud38400, 0x0f},
    {"/dev/ttyWCH10", QSerialPort::Baud38400, 0x10},
    {"/dev/ttyWCH11", QSerialPort::Baud38400, 0x11},
    {"/dev/ttyWCH12", QSerialPort::Baud38400, 0x12},
    {"/dev/ttyWCH13", QSerialPort::Baud38400, 0x13},
    {"/dev/ttyWCH14", QSerialPort::Baud38400, 0x14},
    {"/dev/ttyWCH15", QSerialPort::Baud38400, 0x15},
    {"/dev/ttyWCH16", QSerialPort::Baud38400, 0x16},
    {"/dev/ttyWCH17", QSerialPort::Baud38400, 0x17},
    {"/dev/ttyWCH18", QSerialPort::Baud38400, 0x18},
    {"/dev/ttyWCH19", QSerialPort::Baud38400, 0x19},
    {"/dev/ttyWCH20", QSerialPort::Baud38400, 0x1a},
    {"/dev/ttyWCH21", QSerialPort::Baud38400, 0x1b},
    {"/dev/ttyWCH22", QSerialPort::Baud38400, 0x1c},
    {"/dev/ttyWCH23", QSerialPort::Baud38400, 0x1d},
    {"/dev/ttyWCH24", QSerialPort::Baud38400, 0x1e},
    {"/dev/ttyWCH25", QSerialPort::Baud38400, 0x1f},
    {"/dev/ttyWCH26", QSerialPort::Baud38400, 0x20},
    {"/dev/ttyWCH27", QSerialPort::Baud38400, 0x21},   // 33*/
    {"/dev/ttyS4",    QSerialPort::Baud38400, 0x01},   // test
};

// ===================== 静态成员初始化 =====================

QString ConfigManager::s_configFile = "instrument_config.ini";
QSettings* ConfigManager::s_settings = nullptr;

// global variable
QString ConfigManager::s_manufacturer = "Leacesy";
QString ConfigManager::s_model = "66004";
QString ConfigManager::s_serialNumber = "SN123456789";
QString ConfigManager::s_firmwareVersion = "1.0.0";
QString ConfigManager::s_hardwareVersion = "1.0.0";
QString ConfigManager::s_IP = "192.168.137.33";
QString ConfigManager::s_SM = "255.255.255.0";
QString ConfigManager::s_GPIBid = "0";
QString ConfigManager::s_CANid = "0";

QString ConfigManager::s_loglevel = "release";
bool ConfigManager::s_enablelogfile = false;

// channel switch
bool ConfigManager::s_enableUartMess = true;
bool ConfigManager::s_enableCanMess = true;

// control switch
bool ConfigManager::s_enableLANServer = true;
bool ConfigManager::s_enableWEBServer = false;
bool ConfigManager::s_enableUARTServer = true;
bool ConfigManager::s_enableDisplay = true;

// ===================== 初始化方法 =====================

bool ConfigManager::init(const QString &configDir)
{
    QString fullPath = configDir + "/" + s_configFile;
    if (!QFile::exists(fullPath)) {return false;}

    if (!s_settings) {
        s_settings = new QSettings(fullPath, QSettings::IniFormat);
    }

    // global variable
    s_model = s_settings->value("Device/Model").toString();
    s_serialNumber = s_settings->value("Device/SerialNumber").toString();
    s_IP = s_settings->value("Device/IP").toString();
    s_SM = s_settings->value("Device/SM").toString();
    s_GPIBid = s_settings->value("Device/GPIBID").toString();
    s_CANid = s_settings->value("Device/CANID").toString();

    s_loglevel = s_settings->value("Logger/logLevel").toString();
    s_enablelogfile = s_settings->value("Logger/EnablelogFile").toBool();

    // channel switch
    s_enableUartMess = s_settings->value("Channel/EnableUartMess").toBool();
    s_enableCanMess = s_settings->value("Channel/EnableCanMess").toBool();

    // control switch
    s_enableLANServer = s_settings->value("Control/EnableLANServer").toBool();
    s_enableWEBServer = s_settings->value("Control/EnableWEBServer").toBool();
    s_enableUARTServer = s_settings->value("Control/EnableUARTServer").toBool();
    s_enableDisplay = s_settings->value("Control/EnableDisplay").toBool();

    return true;
}

bool ConfigManager::setConfigValue(const QString &key, const QVariant &value)
{
    if (!s_settings) {return false;}

    QVariant oldValue = s_settings->value(key);
    if (oldValue == value && !value.isNull()) {return true;}

    s_settings->setValue(key, value); // 暂时写入内存
    //s_settings->sync(); // 立即写入文件，可选
    return true;
}
