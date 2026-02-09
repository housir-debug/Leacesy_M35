#pragma once
#include <QLoggingCategory>
//#include <QQmlApplicationEngine>

Q_DECLARE_LOGGING_CATEGORY(uart_bridge)

class SerialBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int ch1_status_v MEMBER mUart4_status_v NOTIFY Uart4_StatusChanged)
    //Q_PROPERTY(QString ch1_status MEMBER mUart4_status NOTIFY Uart4_StatusChanged)
    Q_PROPERTY(float ch1_Voltage MEMBER mUart4_Voltage NOTIFY Uart4_VoltageChanged)
    Q_PROPERTY(float ch1_Current MEMBER mUart4_Current NOTIFY Uart4_CurrentChanged)
    Q_PROPERTY(bool ch1_Current_Unit MEMBER mUart4_Current_Unit NOTIFY Uart4_Current_Unit_Changed)

    Q_PROPERTY(int ch2_status_v MEMBER mUart5_status_v NOTIFY Uart5_StatusChanged)
    //Q_PROPERTY(QString ch2_status MEMBER mUart5_status NOTIFY Uart5_StatusChanged)
    Q_PROPERTY(float ch2_Voltage MEMBER mUart5_Voltage NOTIFY Uart5_VoltageChanged)
    Q_PROPERTY(float ch2_Current MEMBER mUart5_Current NOTIFY Uart5_CurrentChanged)
    Q_PROPERTY(bool ch2_Current_Unit MEMBER mUart5_Current_Unit NOTIFY Uart5_Current_Unit_Changed)

signals:
    // to C++ model control
    void sendFrame_Uart4(quint8 cmd, quint8 func, const QByteArray& param);
    void sendFrame_Uart5(quint8 cmd, quint8 func, const QByteArray& param);

    // to qml engine property
    void Uart4_StatusChanged();
    void Uart4_VoltageChanged();
    void Uart4_CurrentChanged();
    void Uart4_Current_Unit_Changed();

    void Uart5_StatusChanged();
    void Uart5_VoltageChanged();
    void Uart5_CurrentChanged();
    void Uart5_Current_Unit_Changed();

private:
    // to qml engine property variate
    int mUart4_status_v{0};
    QString mUart4_status{""};
    float mUart4_Voltage{0.0f};
    float mUart4_Current{0.0f};
    bool mUart4_Current_Unit{false};

    int mUart5_status_v{0};
    QString mUart5_status{""};
    float mUart5_Voltage{0.0f};
    float mUart5_Current{0.0f};
    bool mUart5_Current_Unit{false};

    // Own member variables
    QByteArray m_Unit_buffer;
    QByteArray m_Status_buffer;

public:
    explicit SerialBridge(QObject *parent = nullptr);
    ~SerialBridge() override = default;

    // C++ model signal to this for qml engine
    void update_Uart4_Voltage(float voltage);
    void update_Uart4_Current(float current);
    void update_Uart4_status(QByteArray status);
    void update_Uart5_Voltage(float voltage);
    void update_Uart5_Current(float current);
    void update_Uart5_status(QByteArray status);

    // qml procress
    Q_INVOKABLE void setChannel_Output(int channel,bool switchs);
    Q_INVOKABLE void setChannel_Setstatus(int channel,int model,float value);
    Q_INVOKABLE QString setChannel_CurrentUnit();
    void toAll_Channel(quint8 cmd,quint8 func,QByteArray param);
    // void setupQmlConnections(QQmlApplicationEngine &engine);
};
