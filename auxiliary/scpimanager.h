#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include "scpi/scpi.h"
#ifdef __cplusplus
}
#endif

#include <QObject>
#include <QByteArray>
#include <QMutex>
#include <QWaitCondition>
#include <QLoggingCategory>
#include "serialworker.h"

Q_DECLARE_LOGGING_CATEGORY(scpi)

class ScpiManager : public QObject {
    Q_OBJECT

signals:
    // to C++ model control
    #define CHANNEL(n) void to_UartChannel##n(quint8 cmd, quint8 func, const QByteArray& param,bool isScpi);

    CHANNEL_1_TO_33
    #undef CHANNEL

public:
    explicit ScpiManager(QObject *parent = nullptr);
    ~ScpiManager() override = default;

    QByteArray processCommand(const QByteArray &command);

    // --- 查询写入 -command Auxiliary function ---
    void processCHStateResponse(bool state);
    void processCHvalueResponse(float value);
    void processCHIntvalueResponse(int value);

private:
    // --- 执行 -command function ---
    static scpi_result_t SCPI_OutputState(scpi_t* context);
    static scpi_result_t SCPI_OutputBand(scpi_t* context)            {return sendChoiceCmd(context,0x01,0x08);};
    static scpi_result_t SCPI_OutputCompMode(scpi_t* context)        {return sendChoiceCmd(context,0x01,0x09);};
    static scpi_result_t SCPI_SettingVolt(scpi_t* context)           {return sendFloatCmd(context,0x02,0x00);};
    static scpi_result_t SCPI_SettingCurr(scpi_t* context)           {return sendFloatCmd(context,0x02,0x01);};
    static scpi_result_t SCPI_SettingImpedance(scpi_t* context)      {return sendFloatCmd(context,0x02,0x02);};
    static scpi_result_t SCPI_SettingProt(scpi_t* context)           {return sendFloatCmd(context,0x02,0x03);};
    static scpi_result_t SCPI_SettingTher(scpi_t* context)           {return sendFloatCmd(context,0x02,0x04);};
    static scpi_result_t SCPI_SettingLoad(scpi_t* context)           {return sendFloatCmd(context,0x02,0x05);};
    static scpi_result_t SCPI_ControlClamp(scpi_t* context)          {return sendBoolCmd(context,0x03,0x01);};
    static scpi_result_t SCPI_ControlCurr(scpi_t* context)           {return sendChoiceCmd(context,0x03,0x02);};
    static scpi_result_t SCPI_ControlSyst(scpi_t* context)           {return sendChoiceCmd(context,0x03,0x05);};
    static scpi_result_t SCPI_ControlSAV(scpi_t* context)            {return sendChoiceCmd(context,0x03,0x06);};
    static scpi_result_t SCPI_ControlRCL(scpi_t* context)            {return sendChoiceCmd(context,0x03,0x07);};
    static scpi_result_t SCPI_ControlInde(scpi_t* context)           {return sendBoolCmd(context,0x03,0x08);};
    static scpi_result_t SCPI_ControlLoad(scpi_t* context)           {return sendChoiceCmd(context,0x03,0x09);};
    static scpi_result_t SCPI_MeasureNplc(scpi_t* context)           {return sendFloatCmd(context,0x04,0x0c);};
    static scpi_result_t SCPI_MeasureTime(scpi_t* context)           {return sendFloatCmd(context,0x04,0x1f);};
    static scpi_result_t SCPI_MeasureRang(scpi_t* context)           {return sendIntCmd(context,0x04,0x0e,1);};
    static scpi_result_t SCPI_MeasureAver(scpi_t* context)           {return sendIntCmd(context,0x04,0x0f,1);};
    static scpi_result_t SCPI_MeasureFunc(scpi_t* context);
    static scpi_result_t SCPI_MeasureOffs(scpi_t* context)           {return sendFloatCmd(context,0x04,0x23);};
    static scpi_result_t SCPI_MeasurePoin(scpi_t* context)           {return sendIntCmd(context,0x04,0x1d,2);};
    static scpi_result_t SCPI_MeasureTint(scpi_t* context)           {return sendFloatCmd(context,0x04,0x1e);};
    static scpi_result_t SCPI_RegisterCle(scpi_t* context)           {return sendCmd(context,0x05,0x03,"");};
    static scpi_result_t SCPI_CalibrateExit(scpi_t* context)         {return sendCmd(context,0x06,0x00,"");};
    static scpi_result_t SCPI_CalibrateInit(scpi_t* context)         {return sendCmd(context,0x06,0x01,"");};
    static scpi_result_t SCPI_CalibrateRest(scpi_t* context)         {return sendCmd(context,0x06,0x02,"");};
    static scpi_result_t SCPI_CalibrateSave(scpi_t* context)         {return sendCmd(context,0x06,0x03,"");};
    static scpi_result_t SCPI_CalibrateAll(scpi_t* context)          {return sendCmd(context,0x06,0x04,"");};
    static scpi_result_t SCPI_CalibrateAdc(scpi_t* context)          {return sendCmd(context,0x06,0x05,"");};
    static scpi_result_t SCPI_CalibrateDac(scpi_t* context)          {return sendCmd(context,0x06,0x06,"");};
    static scpi_result_t SCPI_CalibrateEnab(scpi_t* context)         {return sendCmd(context,0x06,0x07,"");};
    static scpi_result_t SCPI_CalibrateImp(scpi_t* context)          {return sendCmd(context,0x06,0x08,"");};
    static scpi_result_t SCPI_CalibrateDcp(scpi_t* context)          {return sendCmd(context,0x06,0x10,"");};
    static scpi_result_t SCPI_CalibrateDcn(scpi_t* context)          {return sendCmd(context,0x06,0x11,"");};
    static scpi_result_t SCPI_CalibrateStep(scpi_t* context);
    static scpi_result_t SCPI_TriggerAbort(scpi_t* context)          {return sendCmd(context,0x08,0x00,"");};
    static scpi_result_t SCPI_TriggerSequene(scpi_t* context)        {return sendIntCmd(context,0x08,0x01,1);};
    static scpi_result_t SCPI_TriggerCoquene(scpi_t* context)        {return sendBoolCmd(context,0x08,0x02);};
    static scpi_result_t SCPI_TriggerSeq1(scpi_t* context)           {return sendCmd(context,0x08,0x03,"");};
    static scpi_result_t SCPI_TriggerSeq2(scpi_t* context)           {return sendCmd(context,0x08,0x04,"");};
    static scpi_result_t SCPI_TriggerSeq2So(scpi_t* context)         {return sendChoiceCmd(context,0x08,0x05);};
    static scpi_result_t SCPI_TriggerSeq2Co(scpi_t* context)         {return sendIntCmd(context,0x08,0x06,2);};
    static scpi_result_t SCPI_TriggerSeq2Hy(scpi_t* context)         {return sendFloatCmd(context,0x08,0x07);};
    static scpi_result_t SCPI_TriggerSeq2Le(scpi_t* context)         {return sendFloatCmd(context,0x08,0x08);};
    static scpi_result_t SCPI_TriggerSeq2Sl(scpi_t* context)         {return sendChoiceCmd(context,0x08,0x09);};
    static scpi_result_t SCPI_TriggerAmpl(scpi_t* context)           {return sendFloatCmd(context,0x08,0x0a);};
    static scpi_result_t SCPI_TriggerCurr(scpi_t* context)           {return sendFloatCmd(context,0x08,0x0b);};
    static scpi_result_t SCPI_TriggerRes(scpi_t* context)            {return sendFloatCmd(context,0x08,0x0c);};

    // --- 辅助 -
    static scpi_result_t sendCmd(scpi_t* context, quint8 cmd, quint8 func, const QByteArray &data);
    static scpi_result_t sendIntCmd(scpi_t* context, quint8 cmd, quint8 func,quint8 bytes);
    static scpi_result_t sendChoiceCmd(scpi_t* context, quint8 cmd, quint8 func);
    static scpi_result_t sendFloatCmd(scpi_t* context, quint8 cmd, quint8 func);
    static scpi_result_t sendBoolCmd(scpi_t* context, quint8 cmd, quint8 func);
    static bool sendQueryCmd(scpi_t* context, quint8 cmd, quint8 func);
    static scpi_result_t sendQueryIntCmd(scpi_t* context, quint8 cmd, quint8 func);
    static scpi_result_t sendQueryBoolCmd(scpi_t* context, quint8 cmd, quint8 func);
    static scpi_result_t sendQueryFloatCmd(scpi_t* context, quint8 cmd, quint8 func);

    // --- 查询 -command function ---
    static scpi_result_t SCPI_OutputStateQ(scpi_t* context)          {return sendQueryBoolCmd(context,0x01,0x80);};
    static scpi_result_t SCPI_OutputBandQ(scpi_t* context);
    static scpi_result_t SCPI_OutputCompModeQ(scpi_t* context);
    static scpi_result_t SCPI_SettingImpedanceQ(scpi_t* context)     {return sendQueryFloatCmd(context,0x02,0x82);};
    static scpi_result_t SCPI_SettingVoltQ(scpi_t* context)          {return sendQueryFloatCmd(context,0x02,0x80);};
    static scpi_result_t SCPI_SettingCurrQ(scpi_t* context)          {return sendQueryFloatCmd(context,0x02,0x81);};
    static scpi_result_t SCPI_SettingProtQ(scpi_t* context)          {return sendQueryFloatCmd(context,0x02,0x83);};
    static scpi_result_t SCPI_SettingTherQ(scpi_t* context)          {return sendQueryFloatCmd(context,0x02,0x84);};
    static scpi_result_t SCPI_SettingLoadQ(scpi_t* context)          {return sendQueryFloatCmd(context,0x02,0x85);};
    static scpi_result_t SCPI_ControlClampQ(scpi_t* context)         {return sendQueryBoolCmd(context,0x03,0x81);};
    static scpi_result_t SCPI_ControlCurrQ(scpi_t* context);
    static scpi_result_t SCPI_ControlSystQ(scpi_t* context);
    static scpi_result_t SCPI_ControlIndeQ(scpi_t* context)          {return sendQueryBoolCmd(context,0x03,0x88);};
    static scpi_result_t SCPI_ControlLoadQ(scpi_t* context);
    static scpi_result_t SCPI_MeasureVoltQ(scpi_t* context)          {return sendQueryFloatCmd(context,0x04,0x80);};
    static scpi_result_t SCPI_MeasureCurrQ(scpi_t* context)          {return sendQueryFloatCmd(context,0x04,0x81);};
    static scpi_result_t SCPI_MeasureScurQ(scpi_t* context)          {return sendQueryFloatCmd(context,0x04,0x82);};
    static scpi_result_t SCPI_MeasureBtmpQ(scpi_t* context)          {return sendQueryFloatCmd(context,0x04,0x83);};
    static scpi_result_t SCPI_MeasureHtmpQ(scpi_t* context)          {return sendQueryFloatCmd(context,0x04,0x84);};
    static scpi_result_t SCPI_MeasureAcdcQ(scpi_t* context)          {return sendQueryFloatCmd(context,0x04,0x85);};
    static scpi_result_t SCPI_MeasureDvmQ(scpi_t* context)           {return sendQueryFloatCmd(context,0x04,0x86);};
    static scpi_result_t SCPI_MeasureFanQ(scpi_t* context)           {return sendQueryIntCmd(context,0x04,0x87);};
    static scpi_result_t SCPI_MeasureDutyQ(scpi_t* context)          {return sendQueryIntCmd(context,0x04,0x9d);};
    static scpi_result_t SCPI_MeasureReadQ(scpi_t* context);
    static scpi_result_t SCPI_MeasureDvmacQ(scpi_t* context)         {return sendQueryFloatCmd(context,0x04,0x89);};
    static scpi_result_t SCPI_MeasureTemp1Q(scpi_t* context)         {return sendQueryFloatCmd(context,0x04,0x8a);};
    static scpi_result_t SCPI_MeasureTemp2Q(scpi_t* context)         {return sendQueryFloatCmd(context,0x04,0x8b);};
    static scpi_result_t SCPI_MeasureTemp3Q(scpi_t* context)         {return sendQueryFloatCmd(context,0x04,0x8d);};
    static scpi_result_t SCPI_MeasureAdofVoltQ(scpi_t* context)      {return sendQueryFloatCmd(context,0x04,0xb0);};
    static scpi_result_t SCPI_MeasureAdofCurrQ(scpi_t* context)      {return sendQueryFloatCmd(context,0x04,0xb1);};
    static scpi_result_t SCPI_MeasureAdofScurQ(scpi_t* context)      {return sendQueryFloatCmd(context,0x04,0xb2);};
    static scpi_result_t SCPI_MeasureAdofDvmQ(scpi_t* context)       {return sendQueryFloatCmd(context,0x04,0xb6);};
    static scpi_result_t SCPI_MeasureNplcQ(scpi_t* context)          {return sendCmd(context,0x04,0x8c,"");};
    static scpi_result_t SCPI_MeasureTimeQ(scpi_t* context)          {return sendQueryFloatCmd(context,0x04,0x9f);};
    static scpi_result_t SCPI_MeasureRangQ(scpi_t* context)          {return sendQueryIntCmd(context,0x04,0x8e);};
    static scpi_result_t SCPI_MeasureAverQ(scpi_t* context)          {return sendCmd(context,0x04,0x8f,"");};
    static scpi_result_t SCPI_MeasureFuncQ(scpi_t* context);
    static scpi_result_t SCPI_MeasureCurrHighQ(scpi_t* context)      {return sendQueryFloatCmd(context,0x04,0x91);};
    static scpi_result_t SCPI_MeasureCurrLowQ(scpi_t* context)       {return sendQueryFloatCmd(context,0x04,0x92);};
    static scpi_result_t SCPI_MeasureCurrMaxQ(scpi_t* context)       {return sendQueryFloatCmd(context,0x04,0x93);};
    static scpi_result_t SCPI_MeasureCurrMinQ(scpi_t* context)       {return sendQueryFloatCmd(context,0x04,0x94);};
    static scpi_result_t SCPI_MeasureDvmHighQ(scpi_t* context)       {return sendQueryFloatCmd(context,0x04,0x95);};
    static scpi_result_t SCPI_MeasureDvmLowQ(scpi_t* context)        {return sendQueryFloatCmd(context,0x04,0x96);};
    static scpi_result_t SCPI_MeasureDvmMaxQ(scpi_t* context)        {return sendQueryFloatCmd(context,0x04,0x97);};
    static scpi_result_t SCPI_MeasureDvmMinQ(scpi_t* context)        {return sendQueryFloatCmd(context,0x04,0x98);};
    static scpi_result_t SCPI_MeasureVoltHighQ(scpi_t* context)      {return sendQueryFloatCmd(context,0x04,0x99);};
    static scpi_result_t SCPI_MeasureVoltLowQ(scpi_t* context)       {return sendQueryFloatCmd(context,0x04,0x9a);};
    static scpi_result_t SCPI_MeasureVoltMaxQ(scpi_t* context)       {return sendQueryFloatCmd(context,0x04,0x9b);};
    static scpi_result_t SCPI_MeasureVoltMinQ(scpi_t* context)       {return sendQueryFloatCmd(context,0x04,0x9c);};
    static scpi_result_t SCPI_MeasureOffsQ(scpi_t* context)          {return sendQueryFloatCmd(context,0x04,0xa3);};
    static scpi_result_t SCPI_MeasurePoinQ(scpi_t* context)          {Q_UNUSED(context);return SCPI_RES_OK;};
    static scpi_result_t SCPI_MeasureTintQ(scpi_t* context)          {return sendQueryFloatCmd(context,0x04,0x9e);};
    static scpi_result_t SCPI_MeasureArrCurrQ(scpi_t* context)       {return sendQueryFloatCmd(context,0x04,0xa0);};
    static scpi_result_t SCPI_MeasureArrVoltQ(scpi_t* context)       {return sendQueryFloatCmd(context,0x04,0xa1);};
    static scpi_result_t SCPI_MeasureArrDvmQ(scpi_t* context)        {return sendQueryFloatCmd(context,0x04,0xa2);};
    static scpi_result_t SCPI_RegisterEvenQ(scpi_t* context)         {return sendQueryIntCmd(context,0x05,0x80);};
    static scpi_result_t SCPI_RegisterEnabQ(scpi_t* context)         {return sendQueryIntCmd(context,0x05,0x81);};
    static scpi_result_t SCPI_RegisterCondQ(scpi_t* context)         {return sendQueryIntCmd(context,0x05,0x82);};
    static scpi_result_t SCPI_RegisterNextQ(scpi_t* context)         {return sendQueryIntCmd(context,0x05,0x83);};
    static scpi_result_t SCPI_CalibrateAllQ(scpi_t* context);
    static scpi_result_t SCPI_CalibrateStepQ(scpi_t* context);
    static scpi_result_t SCPI_TriggerSeq2SoQ(scpi_t* context);
    static scpi_result_t SCPI_TriggerSeq2CoQ(scpi_t* context)        {return sendQueryIntCmd(context,0x08,0x86);};
    static scpi_result_t SCPI_TriggerSeq2HyQ(scpi_t* context)        {return sendQueryFloatCmd(context,0x08,0x87);};
    static scpi_result_t SCPI_TriggerSeq2LeQ(scpi_t* context)        {return sendQueryFloatCmd(context,0x08,0x88);};
    static scpi_result_t SCPI_TriggerSeq2SlQ(scpi_t* context);
    static scpi_result_t SCPI_TriggerAmplQ(scpi_t* context)          {return sendQueryFloatCmd(context,0x08,0x8a);};
    static scpi_result_t SCPI_TriggerCurrQ(scpi_t* context)          {return sendQueryFloatCmd(context,0x08,0x8b);};
    static scpi_result_t SCPI_TriggerResQ(scpi_t* context)           {return sendQueryFloatCmd(context,0x08,0x8c);};

    // --- libscpi 静态回调接口 ---
    static size_t staticWrite(scpi_t* context, const char* data, size_t len);
    static int    staticError(scpi_t* context, int_fast16_t err);
    static scpi_result_t staticReset(scpi_t* context);
    static scpi_result_t staticFlush(scpi_t* context);
    static scpi_result_t staticControl(scpi_t* context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val);

private:
    static const scpi_command_t m_scpiCommands[];
    static const scpi_choice_def_t m_CmdparaChoices[];

    static QByteArray m_idnManufacturer;
    static QByteArray m_idnModel;
    static QByteArray m_idnSerialNumber;
    static QByteArray m_idnVersion;

    static scpi_interface_t m_interface;
    static char m_inputBuffer[256];
    static scpi_error_t m_errorQueue[10];

    quint8 m_measurefunchoice{0};
    QByteArray m_responseBuffer;
    scpi_t m_scpiContext;

    bool m_UartResponse_Return{false};
    QWaitCondition m_syncCondition;
    QMutex m_syncMutex;

    bool m_CHStateReturn{false};
    float m_CHvalueReturn{0.0f};
    int m_CHintvalueReturn{0};

};
