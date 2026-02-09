#pragma once
#include <QLoggingCategory>
//#include <QQmlApplicationEngine>

Q_DECLARE_LOGGING_CATEGORY(uart_bridge)

class SerialBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int ch1_status_v MEMBER mCH1_status_v NOTIFY CH1_StatusChanged)
    //Q_PROPERTY(QString ch1_status MEMBER mCH1_status NOTIFY CH1_StatusChanged)
    Q_PROPERTY(float ch1_Voltage MEMBER mCH1_Voltage NOTIFY CH1_VoltageChanged)
    Q_PROPERTY(float ch1_Current MEMBER mCH1_Current NOTIFY CH1_CurrentChanged)
    Q_PROPERTY(bool ch1_Current_Unit MEMBER mCH1_Current_Unit NOTIFY CH1_Current_Unit_Changed)

    Q_PROPERTY(int ch2_status_v MEMBER mCH2_status_v NOTIFY CH2_StatusChanged)
    //Q_PROPERTY(QString ch2_status MEMBER mCH2_status NOTIFY CH2_StatusChanged)
    Q_PROPERTY(float ch2_Voltage MEMBER mCH2_Voltage NOTIFY CH2_VoltageChanged)
    Q_PROPERTY(float ch2_Current MEMBER mCH2_Current NOTIFY CH2_CurrentChanged)
    Q_PROPERTY(bool ch2_Current_Unit MEMBER mCH2_Current_Unit NOTIFY CH2_Current_Unit_Changed)

signals:
    // to C++ model control
    void sendFrame_Uart4(quint8 cmd, quint8 func, const QByteArray& param);
    void sendFrame_Uart5(quint8 cmd, quint8 func, const QByteArray& param);

    // to qml engine property
    void CH1_StatusChanged();
    void CH1_VoltageChanged();
    void CH1_CurrentChanged();
    void CH1_Current_Unit_Changed();

    void CH2_StatusChanged();
    void CH2_VoltageChanged();
    void CH2_CurrentChanged();
    void CH2_Current_Unit_Changed();

private:
    // to qml engine property variate
    int mCH1_status_v{0};
    QString mCH1_status{""};
    float mCH1_Voltage{0.0f};
    float mCH1_Current{0.0f};
    bool mCH1_Current_Unit{false};

    int mCH2_status_v{0};
    QString mCH2_status{""};
    float mCH2_Voltage{0.0f};
    float mCH2_Current{0.0f};
    bool mCH2_Current_Unit{false};

    // Own member variables
    QByteArray m_Unit_buffer;
    QByteArray m_Status_buffer;

public:
    explicit SerialBridge(QObject *parent = nullptr);
    ~SerialBridge() override = default;

    // C++ model signal to this for qml engine
    void update_Voltage(int ch,float voltage);
    void update_CurrentAndUnit(int ch,float current);
    void update_status(int ch,QByteArray status);

    // qml procress
    Q_INVOKABLE void setChannel_Output(int channel,bool switchs);
    Q_INVOKABLE void setChannel_Setstatus(int channel,int model,float value);
    Q_INVOKABLE QString setChannel_CurrentUnit();
    void toAll_Channel(quint8 cmd,quint8 func,QByteArray param);
};
