// uartmanager.cpp
#include "uartmanager.h"
#include <QSerialPortInfo>

SerialBridge::SerialBridge(QObject *parent) : QObject(parent) {}

SerialBridge::~SerialBridge() {}

// ********************* 槽函数具体实现 ****************************

void SerialBridge::update_Uart4_Voltage(float voltage){
    mUart4_Voltage = voltage;
    // qDebug()<<"chuandiwancheng"<<mUart4_Voltage;
    emit Uart4_VoltageChanged();
}

void SerialBridge::update_Uart4_Current(float current){
    mUart4_Current = current;
    emit Uart4_CurrentChanged();
}

void SerialBridge::update_Uart5_Voltage(float voltage){
    mUart5_Voltage = voltage;
    emit Uart5_VoltageChanged();
}

void SerialBridge::update_Uart5_Current(float current){
    mUart5_Current = current;
    emit Uart5_CurrentChanged();
}
