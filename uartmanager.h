#pragma once

#include <QObject>
#include <QDebug>

class SerialBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(float ch1_Voltage READ Uart4_Voltage NOTIFY Uart4_VoltageChanged)
    Q_PROPERTY(float ch1_Current READ Uart4_Current NOTIFY Uart4_CurrentChanged)
    Q_PROPERTY(float ch2_Voltage READ Uart5_Voltage NOTIFY Uart5_VoltageChanged)
    Q_PROPERTY(float ch2_Current READ Uart5_Current NOTIFY Uart5_CurrentChanged)

public:
    explicit SerialBridge(QObject *parent = nullptr);
    ~SerialBridge();

    // uart model slot function
    void update_Uart4_Voltage(float voltage);
    void update_Uart4_Current(float current);
    void update_Uart5_Voltage(float voltage);
    void update_Uart5_Current(float current);

    // Qt property call
    float Uart4_Voltage() const { return mUart4_Voltage; }
    float Uart4_Current() const { return mUart4_Current; }
    float Uart5_Voltage() const { return mUart5_Voltage; }
    float Uart5_Current() const { return mUart5_Current; }

signals:
    void Uart4_VoltageChanged();
    void Uart4_CurrentChanged();
    void Uart5_VoltageChanged();
    void Uart5_CurrentChanged();

private:
    float mUart4_Voltage{0.0f};
    float mUart4_Current{0.0f};
    float mUart5_Voltage{0.0f};
    float mUart5_Current{0.0f};

    // Q_INVOKABLE float getCurrent(int channel = 1) const;
    // static constexpr float EPSILON = 0.001f;
};
