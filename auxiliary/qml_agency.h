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
    Q_PROPERTY(QString GPIBid MEMBER m_GPIBid NOTIFY gpibId_Changed)
    Q_PROPERTY(QString CANid MEMBER m_CANid NOTIFY canId_Changed)

    //const
    Q_PROPERTY(QString SoftVer MEMBER m_SoftVer NOTIFY softver_Changed)
    Q_PROPERTY(QString HardVer MEMBER m_HardVer NOTIFY hardver_Changed)
    Q_PROPERTY(QVariantList ChannelSV MEMBER m_ChannelSV NOTIFY channelSVChanged)
    Q_PROPERTY(QVariantList ChannelHV MEMBER m_ChannelHV NOTIFY channelHVChanged)

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
    void to_GPIBid(QString id);
    void to_CANid(QString id);
    #define CHANNEL(n) void to_UartChannel##n(quint8 cmd, quint8 func, const QByteArray& param,bool isScpi);

    CHANNEL_1_TO_33
    #undef CHANNEL

    // to qml property update
    void isRemote_Changed();
    void ipAdress_Changed();
    void sm_Changed();
    void gpibId_Changed();
    void canId_Changed();

    //const
    void softver_Changed();
    void hardver_Changed();
    void channelSVChanged();
    void channelHVChanged();

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
    QString m_GPIBid;
    QString m_CANid;

    //const
    QString m_SoftVer;
    QString m_HardVer;
    QVariantList m_ChannelSV;
    QVariantList m_ChannelHV;

    #define CHANNEL(n) \
        std::atomic<float> mCH##n##_Voltage{0.0f}; \
        std::atomic<float> mCH##n##_Current{0.0f}; \
        QString mCH##n##_CurrentUnit{"A"}; \
        QString mCH##n##_Status; \
        std::atomic<float> mCH##n##_cv{0.0f}; \
        std::atomic<float> mCH##n##_cc{1.0f}; \
        std::atomic<float> mCH##n##_ovp{8.0f}; \
        std::atomic<bool> mCH##n##_isOutput{false};

    CHANNEL_1_TO_33
    #undef CHANNEL

    //web
    QJsonArray getAllChannelsData();

    // C++ model signal to this for qml engine
    void update_Voltage(int ch,float voltage);
    void update_CurrentAndUnit(int ch,float current);
    void update_Status(int ch,quint16 status);
    void update_Cv(int ch,float cv);
    void update_Cc(int ch,float cc);
    void update_Ovp(int ch,float ovp);
    void update_IsOutput(int ch,bool status);
    void update_SoftVer(int ch,const QString &ver);
    void update_HardVer(int ch,const QString &ver);

    //Q_INVOKABLE And C++
    Q_INVOKABLE void update_remotemodel(bool is_remote);
    Q_INVOKABLE void update_Configuration(int model,const QString& val);
    void refresh_interfaces(const QString& ip, const QString& netmask);

    // qml procress
    Q_INVOKABLE void setChannel_Output(int channel,bool switchs);
    Q_INVOKABLE void setChannel_Setstatus(int channel,int model,const QString& val);
    Q_INVOKABLE QString setChannel_CurrentUnit();
    void to_Channel(int channel,quint8 cmd,quint8 func,const QByteArray& param);
};
