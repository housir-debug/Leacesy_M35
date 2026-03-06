#pragma once
#include <QMutex>
#include <QLoggingCategory>
#include "auxiliary/config_manager.h"

Q_DECLARE_LOGGING_CATEGORY(uart_bridge)

class SerialBridge : public QObject
{
    Q_OBJECT

    #define CHANNEL(n) \
        Q_PROPERTY(int ch##n##_status_v MEMBER mCH##n##_status_v NOTIFY CH##n##_StatusChanged) \
        Q_PROPERTY(float ch##n##_Voltage MEMBER mCH##n##_Voltage NOTIFY CH##n##_VoltageChanged) \
        Q_PROPERTY(float ch##n##_Current MEMBER mCH##n##_Current NOTIFY CH##n##_CurrentChanged) \
        Q_PROPERTY(bool ch##n##_Current_Unit MEMBER mCH##n##_Current_Unit NOTIFY CH##n##_Current_Unit_Changed)

    CHANNEL_1_TO_33
    #undef CHANNEL

signals:
    // to C++ model control
    #define CHANNEL(n) void to_UartChannel##n(quint8 cmd, quint8 func, const QByteArray& param,bool isScpi);

    CHANNEL_1_TO_33
    #undef CHANNEL

    // to qml property update
    #define CHANNEL(n) \
        void CH##n##_StatusChanged(); \
        void CH##n##_VoltageChanged(); \
        void CH##n##_CurrentChanged(); \
        void CH##n##_Current_Unit_Changed();

    CHANNEL_1_TO_33
    #undef CHANNEL

private:
    // to qml engine property variate
    #define CHANNEL(n) \
        int mCH##n##_status_v{0}; \
        QString mCH##n##_status{""}; \
        float mCH##n##_Voltage{0.0f}; \
        float mCH##n##_Current{0.0f}; \
        bool mCH##n##_Current_Unit{false};

    CHANNEL_1_TO_33
    #undef CHANNEL

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

    // Auxiliary function
    void toAll_Channel(quint8 cmd,quint8 func,const QByteArray& param);
};
