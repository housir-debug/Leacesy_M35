#include "config_manager.h"
#include <QNetworkInterface>
#include <QFile>

Q_LOGGING_CATEGORY(config, "CONFIG:")
QSettings* ConfigManager::s_settings = nullptr;

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
    {"/dev/ttyS5",    QSerialPort::Baud38400, 0x01},   // test
};

// log config
bool ConfigManager::s_enablelogfile = false;
QString ConfigManager::s_loglevel = "release";

// channel switch
bool ConfigManager::s_enableUartMess = true;
bool ConfigManager::s_enableCanMess = true;
// control switch
bool ConfigManager::s_enableLANServer = true;
bool ConfigManager::s_enableWEBServer = false;
bool ConfigManager::s_enableCANServer = true;
bool ConfigManager::s_enableUARTServer = true;
bool ConfigManager::s_enableDisplay = true;

// global variable - Internal fixation
QString ConfigManager::s_firmwareVersion = "1.0.0";
QString ConfigManager::s_hardwareVersion = "1.0.0";
QString ConfigManager::s_manufacturer = "Leacesy";
// global variable - System reading
QString ConfigManager::s_IP = "192.168.137.36";
QString ConfigManager::s_SM = "255.255.255.0";

// global variable - config reading
QString ConfigManager::s_serialNumber = "SN-12306";
QString ConfigManager::s_model = "66004";
QString ConfigManager::s_GPIBid = "0";
QString ConfigManager::s_CANid = "0";

//---------------------------------------------------------------------------------

bool ConfigManager::init(const QString &configDir)
{
    if (getNetworkConfig()){
        QString fullPath = configDir + "/instrument_config.ini";

        if (QFile::exists(fullPath) && !s_settings) {
            s_settings = new QSettings(fullPath, QSettings::IniFormat);

            s_loglevel = s_settings->value("Logger/logLevel").toString();
            s_enablelogfile = s_settings->value("Logger/EnablelogFile").toBool();

            // channel switch
            s_enableUartMess = s_settings->value("Channel/EnableUartMess").toBool();
            s_enableCanMess = s_settings->value("Channel/EnableCanMess").toBool();
            // control switch
            s_enableLANServer = s_settings->value("Control/EnableLANServer").toBool();
            s_enableWEBServer = s_settings->value("Control/EnableWEBServer").toBool();
            s_enableCANServer = s_settings->value("Control/EnableCANServer").toBool();
            s_enableUARTServer = s_settings->value("Control/EnableUARTServer").toBool();
            s_enableDisplay = s_settings->value("Control/EnableDisplay").toBool();

            // global variable
            s_serialNumber = s_settings->value("Device/SerialNumber").toString();
            s_model = s_settings->value("Device/Model").toString();
            s_GPIBid = s_settings->value("Device/GPIBID").toString();
            s_CANid = s_settings->value("Device/CANID").toString();

            return true;
        }

        qCWarning(config) << "[init]:Cannot open config file for writing";
    }
    return false;
}

bool ConfigManager::setConfigValue(const QString &key, const QVariant &value)
{
    if (s_settings) {
        QVariant oldValue = s_settings->value(key);
        if (oldValue != value && !value.isNull()) {
            // Automatically written at the end
            s_settings->setValue(key, value);
            // s_settings->sync(); // Write immediately to the file
            return true;
        }

        qCWarning(config) << "[setConfigValue]:config new value invalue.";
    }
    return false;
}

bool ConfigManager::getNetworkConfig() {
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& iface : qAsConst(interfaces)) {
        if (iface.name() != "eth0") continue;

        const auto entries = iface.addressEntries();
        for (const QNetworkAddressEntry& entry : qAsConst(entries)) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                s_IP = entry.ip().toString();
                s_SM = entry.netmask().toString();
                return true;
            }
        }

        qCWarning(config) << "[getNetworkConfig]:reading IP&SM failed!";
    }
    return false;
}

bool ConfigManager::refresh_interfaces(const QString& ip, const QString& netmask){
    QString configContent =
        "# interface file auto-generated by buildroot\n\n"
        "auto lo\n"
        "iface lo inet loopback\n\n"
        "auto eth0\n"
        "iface eth0 inet static\n"
        "    address " + ip + "\n"
               "    netmask " + netmask;

    QFile file("/etc/network/interfaces");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        // WriteOnly -> Clear the original content
        QTextStream out(&file);
        out << configContent;
        file.close();

        QString restartCmd = "/etc/init.d/S37network restart";
        if (system(restartCmd.toStdString().c_str()) == 0) {
            qCWarning(config) << "[refresh_interfaces]:Network changed successfully to:" << ip << "/" << netmask;
            return true;
        }
    }

    qCWarning(config) << "[refresh_interfaces]:Cannot open interfaces file for writing";
    return false;
}
