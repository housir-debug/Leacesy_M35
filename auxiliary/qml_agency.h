#pragma once
#include <QMutex>
#include <QLoggingCategory>
#include "auxiliary/config_manager.h"

Q_DECLARE_LOGGING_CATEGORY(uart_bridge)

class SerialBridge : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isRemote MEMBER m_isRemote NOTIFY isRemote_Change)

    #define CHANNEL(n) \
        Q_PROPERTY(QString ch##n##_Status MEMBER mCH##n##_Status NOTIFY CH##n##_StatusChanged) \
        Q_PROPERTY(float ch##n##_Voltage MEMBER mCH##n##_Voltage NOTIFY CH##n##_VoltageChanged) \
        Q_PROPERTY(float ch##n##_Current MEMBER mCH##n##_Current NOTIFY CH##n##_CurrentChanged) \
        Q_PROPERTY(bool ch##n##_CurrentUnit MEMBER mCH##n##_CurrentUnit NOTIFY CH##n##_CurrentUnit_Changed)

    CHANNEL_1_TO_33
    #undef CHANNEL

signals:
    // to C++ model control
    #define CHANNEL(n) void to_UartChannel##n(quint8 cmd, quint8 func, const QByteArray& param,bool isScpi);

    CHANNEL_1_TO_33
    #undef CHANNEL

    // to qml property update
    void isRemote_Change();

    #define CHANNEL(n) \
        void CH##n##_StatusChanged(); \
        void CH##n##_VoltageChanged(); \
        void CH##n##_CurrentChanged(); \
        void CH##n##_CurrentUnit_Changed();

    CHANNEL_1_TO_33
    #undef CHANNEL

public:
    explicit SerialBridge(QObject *parent = nullptr);
    ~SerialBridge() override = default;

    // to qml engine property variate
    std::atomic<bool> m_isRemote{false};

    #define CHANNEL(n) \
        std::atomic<float> mCH##n##_Voltage{0.0f}; \
        std::atomic<float> mCH##n##_Current{0.0f}; \
        QString mCH##n##_Status; \
        std::atomic<bool> mCH##n##_CurrentUnit{false}; \
        \
        std::atomic<float> mCH##n##_cv{0.0f}; \
        std::atomic<float> mCH##n##_cc{0.0f}; \
        std::atomic<float> mCH##n##_ov{0.0f}; \
        std::atomic<bool> mCH##n##_isOutput{false};

    CHANNEL_1_TO_33
    #undef CHANNEL

    // C++ model signal to this for qml engine
    void update_Voltage(int ch,float voltage);
    void update_CurrentAndUnit(int ch,float current);
    void update_status(int ch,const QByteArray& status);
    void update_remotemodel(bool is_remote);
    QJsonArray getAllChannelsData();

    // qml procress
    Q_INVOKABLE void setChannel_Output(int channel,bool switchs);
    Q_INVOKABLE void setChannel_Setstatus(int channel,int model,float value);
    Q_INVOKABLE QString setChannel_CurrentUnit();
    Q_INVOKABLE void switch_remotemodel(bool is_remote);
    void to_Channel(int channel,quint8 cmd,quint8 func,const QByteArray& param);
};
