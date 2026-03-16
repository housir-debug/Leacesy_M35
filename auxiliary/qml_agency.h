#pragma once
#include <QMutex>
#include <QLoggingCategory>
#include "auxiliary/config_manager.h"

Q_DECLARE_LOGGING_CATEGORY(uart_bridge)

class SerialBridge : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isRemote MEMBER m_isRemote NOTIFY isRemote_Changed)
    Q_PROPERTY(QString IPaddress MEMBER m_IPaddress NOTIFY ipAdress_Changed)
    Q_PROPERTY(QString SM MEMBER m_SM NOTIFY sm_Changed)
    Q_PROPERTY(int GPIBid MEMBER m_GPIBid NOTIFY gpibId_Changed)
    Q_PROPERTY(int CANid MEMBER m_CANid NOTIFY canId_Changed)

    #define CHANNEL(n) \
        Q_PROPERTY(float ch##n##_Voltage MEMBER mCH##n##_Voltage NOTIFY CH##n##_VoltageChanged) \
        Q_PROPERTY(float ch##n##_Current MEMBER mCH##n##_Current NOTIFY CH##n##_CurrentChanged) \
        Q_PROPERTY(QString ch##n##_Status MEMBER mCH##n##_Status NOTIFY CH##n##_StatusChanged) \
        Q_PROPERTY(QString ch##n##_CurrentUnit MEMBER mCH##n##_CurrentUnit NOTIFY CH##n##_CurrentUnitChanged) \
        \
        Q_PROPERTY(float ch##n##_cv MEMBER mCH##n##_cv NOTIFY CH##n##_cvChanged) \
        Q_PROPERTY(float ch##n##_cc MEMBER mCH##n##_cc NOTIFY CH##n##_ccChanged) \
        Q_PROPERTY(float ch##n##_ovp MEMBER mCH##n##_ovp NOTIFY CH##n##_ovpChanged) \
        Q_PROPERTY(bool ch##n##_isOutput MEMBER mCH##n##_isOutput NOTIFY CH##n##_isOutputChanged)

    CHANNEL_1_TO_33
    #undef CHANNEL

signals:
    // to C++ model control
    #define CHANNEL(n) void to_UartChannel##n(quint8 cmd, quint8 func, const QByteArray& param,bool isScpi);

    CHANNEL_1_TO_33
    #undef CHANNEL

    // to qml property update
    void isRemote_Changed();
    void ipAdress_Changed();
    void sm_Changed();
    void gpibId_Changed();
    void canId_Changed();

    #define CHANNEL(n) \
        void CH##n##_VoltageChanged(); \
        void CH##n##_CurrentChanged(); \
        void CH##n##_StatusChanged(); \
        void CH##n##_CurrentUnitChanged(); \
        \
        void CH##n##_cvChanged(); \
        void CH##n##_ccChanged(); \
        void CH##n##_ovpChanged(); \
        void CH##n##_isOutputChanged();

    CHANNEL_1_TO_33
    #undef CHANNEL

public:
    explicit SerialBridge(QObject *parent = nullptr);
    ~SerialBridge() override = default;

    // to qml engine property variate
    std::atomic<bool> m_isRemote{false};
    QString m_IPaddress;
    QString m_SM;
    std::atomic<int> m_GPIBid{0};
    std::atomic<int> m_CANid{0};

    #define CHANNEL(n) \
        std::atomic<float> mCH##n##_Voltage{0.0f}; \
        std::atomic<float> mCH##n##_Current{0.0f}; \
        QString mCH##n##_Status; \
        QString mCH##n##_CurrentUnit{"A"}; \
        \
        std::atomic<float> mCH##n##_cv{0.0f}; \
        std::atomic<float> mCH##n##_cc{1.0f}; \
        std::atomic<float> mCH##n##_ovp{8.0f}; \
        std::atomic<bool> mCH##n##_isOutput{false};

    CHANNEL_1_TO_33
    #undef CHANNEL

    // C++ model signal to this for qml engine
    void update_Voltage(int ch,float voltage);
    void update_CurrentAndUnit(int ch,float current);
    void update_status(int ch,const QByteArray& status);
    QJsonArray getAllChannelsData();
    void update_Configuration(int model,const QString& val);

    //Q_INVOKABLE And C++
    Q_INVOKABLE void update_remotemodel(bool is_remote);

    // qml procress
    Q_INVOKABLE void setChannel_Output(int channel,bool switchs);
    Q_INVOKABLE QString setChannel_CurrentUnit();
    Q_INVOKABLE void setChannel_Setstatus(int channel,int model,const QString& val);
    void to_Channel(int channel,quint8 cmd,quint8 func,const QByteArray& param);
};
