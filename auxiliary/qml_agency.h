#pragma once
#include <QMutex>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include "auxiliary/config_manager.h"
#include "auxiliary/battery_model.h"

Q_DECLARE_LOGGING_CATEGORY(uart_bridge)

class GuiBridge : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString SoftVer MEMBER m_SoftVer NOTIFY softver_Changed)
    Q_PROPERTY(QString HardVer MEMBER m_HardVer NOTIFY hardver_Changed)
    Q_PROPERTY(quint8 reface MEMBER m_remoteStatus NOTIFY isRemote_Changed)

    Q_PROPERTY(QString IPaddress MEMBER m_IPaddress NOTIFY ipAdress_Changed)
    Q_PROPERTY(QString SM MEMBER m_SM NOTIFY sm_Changed)
    Q_PROPERTY(QString CANid MEMBER m_CANid NOTIFY canId_Changed)
    Q_PROPERTY(QString GPIBid MEMBER m_GPIBid NOTIFY gpibId_Changed)

    #define CHANNEL(n) \
        Q_PROPERTY(QString ch##n##_sv MEMBER mCH##n##_sv NOTIFY CH##n##_svChanged) \
        Q_PROPERTY(QString ch##n##_hv MEMBER mCH##n##_hv NOTIFY CH##n##_hvChanged) \
        \
        Q_PROPERTY(float ch##n##_Voltage MEMBER mCH##n##_Voltage NOTIFY CH##n##_VoltageChanged) \
        Q_PROPERTY(float ch##n##_Current MEMBER mCH##n##_Current NOTIFY CH##n##_CurrentChanged) \
        Q_PROPERTY(QString ch##n##_Status MEMBER mCH##n##_Status NOTIFY CH##n##_StatusChanged) \
        Q_PROPERTY(QString ch##n##_CurrentUnit MEMBER mCH##n##_CurrentUnit NOTIFY CH##n##_CurrentUnitChanged) \
        \
        Q_PROPERTY(float ch##n##_cv MEMBER mCH##n##_cv NOTIFY CH##n##_cvChanged) \
        Q_PROPERTY(float ch##n##_cc MEMBER mCH##n##_cc NOTIFY CH##n##_ccChanged) \
        Q_PROPERTY(float ch##n##_ovp MEMBER mCH##n##_ovp NOTIFY CH##n##_ovpChanged) \
        Q_PROPERTY(bool ch##n##_isOutput MEMBER mCH##n##_isOutput NOTIFY CH##n##_isOutputChanged) \
        \
        Q_PROPERTY(float ch##n##_imp MEMBER mCH##n##_imp NOTIFY CH##n##_impChanged) \
        Q_PROPERTY(float ch##n##_CurrentSOC MEMBER mCH##n##_currentSOC NOTIFY CH##n##_CurrentSOCChanged) \
        Q_PROPERTY(float ch##n##_CapacityAH MEMBER mCH##n##_capacityAH NOTIFY CH##n##_CapacityAHChanged) \
        Q_PROPERTY(QString ch##n##_BatteryModel MEMBER mCH##n##_batteryModel NOTIFY CH##n##_BatteryModelChanged)

    CHANNEL_COUNT
    #undef CHANNEL

signals:
    void softver_Changed();
    void hardver_Changed();
    void isRemote_Changed();

    void ipAdress_Changed();
    void sm_Changed();
    void gpibId_Changed();
    void canId_Changed();

    #define CHANNEL(n) \
        void CH##n##_svChanged(); \
        void CH##n##_hvChanged(); \
        \
        void CH##n##_VoltageChanged(); \
        void CH##n##_CurrentChanged(); \
        void CH##n##_StatusChanged(); \
        void CH##n##_CurrentUnitChanged(); \
        \
        void CH##n##_cvChanged(); \
        void CH##n##_ccChanged(); \
        void CH##n##_ovpChanged(); \
        void CH##n##_isOutputChanged(); \
        \
        void CH##n##_impChanged(); \
        void CH##n##_CurrentSOCChanged(); \
        void CH##n##_CapacityAHChanged(); \
        void CH##n##_BatteryModelChanged();

    CHANNEL_COUNT
    #undef CHANNEL

public:
    QString m_SoftVer;
    QString m_HardVer;
    std::atomic<quint8> m_remoteStatus{0};

    QString m_IPaddress;
    QString m_SM;
    QString m_GPIBid;
    QString m_CANid;

    #define CHANNEL(n) \
        QString mCH##n##_sv{"0.0.0.0"}; \
        QString mCH##n##_hv{"0.0.0.0"}; \
        \
        std::atomic<float> mCH##n##_Voltage{0.0f}; \
        std::atomic<float> mCH##n##_Current{0.0f}; \
        QString mCH##n##_Status{"0000000000000000"}; \
        QString mCH##n##_CurrentUnit{"mA"}; \
        \
        std::atomic<float> mCH##n##_cv{0.0f}; \
        std::atomic<float> mCH##n##_cc{1.0f}; \
        std::atomic<float> mCH##n##_ovp{8.0f}; \
        std::atomic<bool> mCH##n##_isOutput{false}; \
        \
        std::atomic<float> mCH##n##_imp{0.0f}; \
        std::atomic<float> mCH##n##_currentSOC{81}; \
        std::atomic<float> mCH##n##_capacityAH{36}; \
        QString mCH##n##_batteryModel{"Lithium"}; \
        \
        std::atomic<bool> mCH##n##_batterystaticmode{false}; \
        QSharedPointer<BatteryModel> mCH##n##_activeModel; \
        std::atomic<bool> mCH##n##_enablebattery{false}; \
        QElapsedTimer mCH##n##_integralTimer;

    CHANNEL_COUNT
    #undef CHANNEL

signals:
    void to_CANid(QString id);
    void to_GPIBid(QString id);
    #define CHANNEL(n) \
        void to_UartChannel##n(quint8 cmd, quint8 func, const QByteArray& param,bool isScpi);

    CHANNEL_COUNT
    #undef CHANNEL

public:
    // Internal trigger function
    void update_SoftVer(int ch,const QString &ver);
    void update_HardVer(int ch,const QString &ver);

    void update_Voltage(int ch,float voltage);
    void update_CurrentAndUnit(int ch,float current);
    void update_Status(int ch,quint16 status);

    void update_Cv(int ch,float cv);
    void update_Cc(int ch,float cc);
    void update_Ovp(int ch,float ovp);
    void update_IsOutput(int ch,bool status);
    void update_Imp(int ch,float imp);

    // Screen And Internal trigger function
    Q_INVOKABLE void update_remotemodel(quint8 reface);
    Q_INVOKABLE void update_Configuration(int model,const QString& val);
    /*Both remotely and locally, the function will be invoked.*/

    // Screen trigger function
    Q_INVOKABLE QString setChannel_CurrentUnit(int channel);
    Q_INVOKABLE void setChannel_Output(int channel,bool switchs);
    Q_INVOKABLE void setChannel_Setstatus(int channel,int model,const QString& val);

    Q_INVOKABLE void setChannel_BatteryOutput(int channel,bool switchs);
    Q_INVOKABLE void setChannel_InitSOC(int channel,const QString& val);
    Q_INVOKABLE void setChannel_Capacity(int channel,const QString& val);
    Q_INVOKABLE void setChannel_Batterymode(int channel,bool staticmode);
    Q_INVOKABLE QString setChannel_BatteryModel(int channel);

    Q_INVOKABLE QVariantList getActiveChannels();
    void to_Channel(int channel,quint8 cmd,quint8 func,const QByteArray& param);

public:
    explicit GuiBridge(QObject *parent = nullptr);
    ~GuiBridge() override = default;

    QJsonArray getAllChannelsData();
    void load_BatteryModel();

    std::shared_ptr<BatteryModelManager> m_modelManager{nullptr};
    QStringList m_currentModelList;
};
