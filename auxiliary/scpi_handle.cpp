#include "scpi_handle.h"
#include <QtCore>

Q_LOGGING_CATEGORY(scpi, "SCPI:")

// ======================== 指令表定义及处理 =========================

const scpi_command_t ScpiManager::m_scpiCommands[] = {
    { "*CLS",      SCPI_CoreCls,    0 },
    { "*ESE",      SCPI_CoreEse,    0 },
    { "*OPC",      SCPI_CoreOpc,    0 },
    { "*RST",      SCPI_CoreRst,    0 },
    { "*SRE",      SCPI_CoreSre,    0 },
    { "*ESE?",     SCPI_CoreEseQ,   0 },
    { "*ESR?",     SCPI_CoreEsrQ,   0 },
    { "*IDN?",     SCPI_CoreIdnQ,   0 },
    { "*OPC?",     SCPI_CoreOpcQ,   0 },
    { "*SRE?",     SCPI_CoreSreQ,   0 },
    { "*STB?",     SCPI_CoreStbQ,   0 },

    // --------------------------- user-defined Execute --------------------
    { ":OUTPut#",                                     ScpiManager::SCPI_OutputState,          0 },
    { ":OUTPut#:STATe",                               ScpiManager::SCPI_OutputState,          0 },
    { ":OUTPut#:BANDwidth",                           ScpiManager::SCPI_OutputBand,           0 },
    { ":OUTPut#:COMPensation:MODE",                   ScpiManager::SCPI_OutputCompMode,       0 },
    // --- Setting -
    { ":OUTPut#:IMPedance",                           ScpiManager::SCPI_SettingImpedance,     0 },
    { ":SOURce#:VOLTage",                             ScpiManager::SCPI_SettingVolt,          0 },
    { ":SOURce#:VOLTage:LEVel",                       ScpiManager::SCPI_SettingVolt,          0 },
    { ":SOURce#:VOLTage:LEVel:AMPLitude",             ScpiManager::SCPI_SettingVolt,          0 },
    { ":SOURce#:VOLTage:PROTection",                  ScpiManager::SCPI_SettingProt,          0 },
    { ":SOURce#:CURRent",                             ScpiManager::SCPI_SettingCurr,          0 },
    { ":SOURce#:CURRent:LIMit",                       ScpiManager::SCPI_SettingCurr,          0 },
    { ":SOURce#:CURRent:LIMit:VALue",                 ScpiManager::SCPI_SettingCurr,          0 },
    { ":THERmal#",                                    ScpiManager::SCPI_SettingTher,          0 },
    { ":THERmal#:PROTection",                         ScpiManager::SCPI_SettingTher,          0 },
    { ":THERmal#:PROTection:TEMPerature",             ScpiManager::SCPI_SettingTher,          0 },
    { ":LOAD#:CURRent",                               ScpiManager::SCPI_SettingLoad,          0 },
    { ":LOAD#:CURRent:LIMit",                         ScpiManager::SCPI_SettingLoad,          0 },
    { ":LOAD#:CURRent:LIMit:VALue",                   ScpiManager::SCPI_SettingLoad,          0 },
    // --- Control -
    { ":SOURce#:VOLTage:PROTection:CLAMp",            ScpiManager::SCPI_ControlClamp,         0 },
    { ":SOURce#:CURRent:TYPE",                        ScpiManager::SCPI_ControlCurr,          0 },
    { ":SOURce#:CURRent:LIMit:TYPE",                  ScpiManager::SCPI_ControlCurr,          0 },
    { ":SYSTem:POSetup",                              ScpiManager::SCPI_ControlSyst,          0 },
    { "*SAV",                                         ScpiManager::SCPI_ControlSAV,           0 },
    { "*RCL",                                         ScpiManager::SCPI_ControlRCL,           0 },
    { ":LOAD#:INDEpendence",                          ScpiManager::SCPI_ControlInde,          0 },
    { ":LOAD#:INDEpendence:STATe",                    ScpiManager::SCPI_ControlInde,          0 },
    { ":LOAD#:CURRent:TYPE",                          ScpiManager::SCPI_SettingLoad,          0 },
    { ":LOAD#:CURRent:LIMit:TYPE",                    ScpiManager::SCPI_SettingLoad,          0 },
    // --- Measurement -
    { ":SENSe#:NPLCycles",                            ScpiManager::SCPI_MeasureNplc,          0 },
    { ":SENSe#:CURRent:RANGe:TIMe",                   ScpiManager::SCPI_MeasureTime,          0 },
    { ":SENSe#:CURRent:DC:RANGe:TIMe",                ScpiManager::SCPI_MeasureTime,          0 },
    { ":SENSe#:CURRent:RANGe",                        ScpiManager::SCPI_MeasureRang,          0 },
    { ":SENSe#:CURRent:DC:RANGe",                     ScpiManager::SCPI_MeasureRang,          0 },
    { ":SENSe#:CURRent:RANGe:UPPer",                  ScpiManager::SCPI_MeasureRang,          0 },
    { ":SENSe#:CURRent:DC:RANGe:UPPer",               ScpiManager::SCPI_MeasureRang,          0 },
    { ":SENSe#:AVERage",                              ScpiManager::SCPI_MeasureAver,          0 },
    { ":SENSe#:FUNCtion",                             ScpiManager::SCPI_MeasureFunc,          0 },
    { ":SENSe#:SWEep::OFFS:POINts",                   ScpiManager::SCPI_MeasureOffs,          0 },
    { ":SENSe#:SWEep:POINts",                         ScpiManager::SCPI_MeasurePoin,          0 },
    { ":SENSe#:SWEep:TINTerval",                      ScpiManager::SCPI_MeasureTint,          0 },
    // --- Register -
    { ":STATus#:QUEue:CLEar",                         ScpiManager::SCPI_RegisterCle,          0 },
    // --- Calibrate -
    { ":CALIbrate#:EXIT",                             ScpiManager::SCPI_CalibrateExit,        0 },
    { ":CALIbrate#:INITialize",                       ScpiManager::SCPI_CalibrateInit,        0 },
    { ":CALIbrate#:RESTore",                          ScpiManager::SCPI_CalibrateRest,        0 },
    { ":CALIbrate#:SAVE",                             ScpiManager::SCPI_CalibrateSave,        0 },
    { ":CALIbrate#:STARt",                            ScpiManager::SCPI_CalibrateAll,         0 },
    { ":CALIbrate#:STARt:ALL",                        ScpiManager::SCPI_CalibrateAll,         0 },
    { ":CALIbrate#:STARt:ADC",                        ScpiManager::SCPI_CalibrateAdc,         0 },
    { ":CALIbrate#:STARt:DAC",                        ScpiManager::SCPI_CalibrateDac,         0 },
    { ":CALIbrate#:STARt:ENABle",                     ScpiManager::SCPI_CalibrateEnab,        0 },
    { ":CALIbrate#:STARt:IMPedance",                  ScpiManager::SCPI_CalibrateImp,         0 },
    { ":CALIbrate#:STARt:DCPositiveOffset",           ScpiManager::SCPI_CalibrateDcp,         0 },
    { ":CALIbrate#:STARt:DCNegativeOffset",           ScpiManager::SCPI_CalibrateDcn,         0 },
    { ":CALIbrate#:STEP",                             ScpiManager::SCPI_CalibrateStep,        0 },
    // --- Trigger -
    { ":ABORt#",                                      ScpiManager::SCPI_TriggerAbort,         0 },
    { ":INITiate#:SEQuence",                          ScpiManager::SCPI_TriggerSequene,       0 },
    { ":INITiate#:IMMediate:SEQuence",                ScpiManager::SCPI_TriggerSequene,       0 },
    { ":INITiate#:CONTinuous:SEQuence1",              ScpiManager::SCPI_TriggerCoquene,       0 },
    { ":INITiate#:CONTinuous:NAME TRANsient",         ScpiManager::SCPI_TriggerCoquene,       0 },
    { ":TRIGger#",                                    ScpiManager::SCPI_TriggerSeq1,          0 },
    { ":TRIGger#:SEQ1",                               ScpiManager::SCPI_TriggerSeq1,          0 },
    { ":TRIGger#:IMMediate",                          ScpiManager::SCPI_TriggerSeq1,          0 },
    { ":TRIGger#:SEQ1:IMMediate",                     ScpiManager::SCPI_TriggerSeq1,          0 },
    { ":TRIGger#:SEQ2",                               ScpiManager::SCPI_TriggerSeq2,          0 },
    { ":TRIGger#:SEQ2:IMMediate",                     ScpiManager::SCPI_TriggerSeq2,          0 },
    { ":TRIGger#:SEQ2:SOURce",                        ScpiManager::SCPI_TriggerSeq2So,        0 },
    { ":TRIGger#:SEQ2:COUNt:CURRent",                 ScpiManager::SCPI_TriggerSeq2Co,        0 },
    { ":TRIGger#:SEQ2:COUNt:DVM",                     ScpiManager::SCPI_TriggerSeq2Co,        0 },
    { ":TRIGger#:SEQ2:COUNt:VOLTage",                 ScpiManager::SCPI_TriggerSeq2Co,        0 },
    { ":TRIGger#:SEQ2:HYSTeresis:CURRent",            ScpiManager::SCPI_TriggerSeq2Hy,        0 },
    { ":TRIGger#:SEQ2:HYSTeresis:DVM",                ScpiManager::SCPI_TriggerSeq2Hy,        0 },
    { ":TRIGger#:SEQ2:HYSTeresis:VOLTage",            ScpiManager::SCPI_TriggerSeq2Hy,        0 },
    { ":TRIGger#:SEQ2:LEVel:CURRent",                 ScpiManager::SCPI_TriggerSeq2Le,        0 },
    { ":TRIGger#:SEQ2:LEVel:DVM",                     ScpiManager::SCPI_TriggerSeq2Le,        0 },
    { ":TRIGger#:SEQ2:LEVel:VOLTage",                 ScpiManager::SCPI_TriggerSeq2Le,        0 },
    { ":TRIGger#:SEQ2:SLOPe:CURRent",                 ScpiManager::SCPI_TriggerSeq2Sl,        0 },
    { ":TRIGger#:SEQ2:SLOPe:DVM",                     ScpiManager::SCPI_TriggerSeq2Sl,        0 },
    { ":TRIGger#:SEQ2:SLOPe:VOLTage",                 ScpiManager::SCPI_TriggerSeq2Sl,        0 },
    { ":SOURce#:VOLTage:AMPL:TRIG",                   ScpiManager::SCPI_TriggerAmpl,          0 },
    { ":SOURce#:CURRent:TRIG",                        ScpiManager::SCPI_TriggerCurr,          0 },
    { ":SOURce#:RES:TRIG",                            ScpiManager::SCPI_TriggerRes,           0 },

    // --------------------------- user-defined  Query  --------------------
    { ":OUTPut#?",                                    ScpiManager::SCPI_OutputStateQ,         0 },
    { ":OUTPut#:STATe?",                              ScpiManager::SCPI_OutputStateQ,         0 },
    { ":OUTPut#:BANDwidth?",                          ScpiManager::SCPI_OutputBandQ,          0 },
    { ":OUTPut#:COMPensation:MODE?",                  ScpiManager::SCPI_OutputCompModeQ,      0 },
    // --- Setting -
    { ":OUTPut#:IMPedance?",                          ScpiManager::SCPI_SettingImpedanceQ,    0 },
    { ":SOURce#:VOLTage?",                            ScpiManager::SCPI_SettingVoltQ,         0 },
    { ":SOURce#:VOLTage:LEVel?",                      ScpiManager::SCPI_SettingVoltQ,         0 },
    { ":SOURce#:VOLTage:LEVel:AMPLitude?",            ScpiManager::SCPI_SettingVoltQ,         0 },
    { ":SOURce#:VOLTage:PROTection?",                 ScpiManager::SCPI_SettingProtQ,         0 },
    { ":SOURce#:CURRent?",                            ScpiManager::SCPI_SettingCurrQ,         0 },
    { ":SOURce#:CURRent:LIMit?",                      ScpiManager::SCPI_SettingCurrQ,         0 },
    { ":SOURce#:CURRent:LIMit:VALue?",                ScpiManager::SCPI_SettingCurrQ,         0 },
    { ":THERmal#?",                                   ScpiManager::SCPI_SettingTherQ,         0 },
    { ":THERmal#:PROTection?",                        ScpiManager::SCPI_SettingTherQ,         0 },
    { ":THERmal#:PROTection:TEMPerature?",            ScpiManager::SCPI_SettingTherQ,         0 },
    { ":LOAD#:CURRent?",                              ScpiManager::SCPI_SettingLoadQ,         0 },
    { ":LOAD#:CURRent:LIMit?",                        ScpiManager::SCPI_SettingLoadQ,         0 },
    { ":LOAD#:CURRent:LIMit:VALue?",                  ScpiManager::SCPI_SettingLoadQ,         0 },
    // --- Control -
    { ":SOURce#:VOLTage:PROTection:CLAMp?",           ScpiManager::SCPI_ControlClampQ,        0 },
    { ":SOURce#:CURRent:TYPE?",                       ScpiManager::SCPI_ControlCurrQ,         0 },
    { ":SOURce#:CURRent:LIMit:TYPE?",                 ScpiManager::SCPI_ControlCurrQ,         0 },
    { ":SYSTem:POSetup?",                             ScpiManager::SCPI_ControlSystQ,         0 },
    { ":LOAD#:INDEpendence?",                         ScpiManager::SCPI_ControlIndeQ,         0 },
    { ":LOAD#:INDEpendence:STATe?",                   ScpiManager::SCPI_ControlIndeQ,         0 },
    { ":LOAD#:CURRent:TYPE?",                         ScpiManager::SCPI_SettingLoadQ,         0 },
    { ":LOAD#:CURRent:LIMit:TYPE?",                   ScpiManager::SCPI_SettingLoadQ,         0 },
    // --- Measurement -
    { ":MEASure#:VOLTage?",                           ScpiManager::SCPI_MeasureVoltQ,         0 },
    { ":MEASure#:VOLTage:DC?",                        ScpiManager::SCPI_MeasureVoltQ,         0 },
    { ":MEASure#:CURRent?",                           ScpiManager::SCPI_MeasureCurrQ,         0 },
    { ":MEASure#:CURRent:DC?",                        ScpiManager::SCPI_MeasureCurrQ,         0 },
    { ":MEASure#:SCURrent?",                          ScpiManager::SCPI_MeasureScurQ,         0 },
    { ":MEASure#:SCURrent:DC?",                       ScpiManager::SCPI_MeasureScurQ,         0 },
    { ":MEASure#:BTMPerature?",                       ScpiManager::SCPI_MeasureBtmpQ,         0 },
    { ":MEASure#:HTMPerature?",                       ScpiManager::SCPI_MeasureHtmpQ,         0 },
    { ":MEASure#:DVMeter:ACDC?",                      ScpiManager::SCPI_MeasureAcdcQ,         0 },
    { ":MEASure#:DVMeter?",                           ScpiManager::SCPI_MeasureDvmQ,          0 },
    { ":MEASure#:DVMeter:DC?",                        ScpiManager::SCPI_MeasureDvmQ,          0 },
    { ":MEASure#:DVMeter:AC?",                        ScpiManager::SCPI_MeasureDvmacQ,        0 },
    { ":MEASure#:TMP1?",                              ScpiManager::SCPI_MeasureTemp1Q,        0 },
    { ":MEASure#:TMP2?",                              ScpiManager::SCPI_MeasureTemp2Q,        0 },
    { ":MEASure#:TMP3?",                              ScpiManager::SCPI_MeasureTemp3Q,        0 },
    { ":MEASure#:ADcOFfset:VOLTage?",                 ScpiManager::SCPI_MeasureAdofVoltQ,     0 },
    { ":MEASure#:ADcOFfset:CURRent?",                 ScpiManager::SCPI_MeasureAdofCurrQ,     0 },
    { ":MEASure#:ADcOFfset:SmallCURrent?",            ScpiManager::SCPI_MeasureAdofScurQ,     0 },
    { ":MEASure#:ADcOFfset:DVMeter?",                 ScpiManager::SCPI_MeasureAdofDvmQ,      0 },
    { ":MEASure#:ARRay:CURRent?",                     ScpiManager::SCPI_MeasureArrCurrQ,      0 },
    { ":MEASure#:ARRay:CURRent:DC?",                  ScpiManager::SCPI_MeasureArrCurrQ,      0 },
    { ":MEASure#:ARRay:VOLTage?",                     ScpiManager::SCPI_MeasureArrVoltQ,      0 },
    { ":MEASure#:ARRay:VOLTage:DC?",                  ScpiManager::SCPI_MeasureArrVoltQ,      0 },
    { ":MEASure#:ARRay:DVMeter?",                     ScpiManager::SCPI_MeasureArrDvmQ,       0 },
    { ":THERmal#:FAN?",                               ScpiManager::SCPI_MeasureFanQ,          0 },
    { ":THERmal#:DUTY?",                              ScpiManager::SCPI_MeasureDutyQ,         0 },
    { ":READ?",                                       ScpiManager::SCPI_MeasureReadQ,         0 },
    { ":SENSe#:NPLCycles?",                           ScpiManager::SCPI_MeasureNplcQ,         0 },
    { ":SENSe#:CURRent:RANGe:TIMe?",                  ScpiManager::SCPI_MeasureTimeQ,         0 },
    { ":SENSe#:CURRent:DC:RANGe:TIMe?",               ScpiManager::SCPI_MeasureTimeQ,         0 },
    { ":SENSe#:CURRent:RANGe?",                       ScpiManager::SCPI_MeasureRangQ,         0 },
    { ":SENSe#:CURRent:DC:RANGe?",                    ScpiManager::SCPI_MeasureRangQ,         0 },
    { ":SENSe#:CURRent:RANGe:UPPer?",                 ScpiManager::SCPI_MeasureRangQ,         0 },
    { ":SENSe#:CURRent:DC:RANGe:UPPer?",              ScpiManager::SCPI_MeasureRangQ,         0 },
    { ":SENSe#:CURRent:RANGe:AUTO?",                  ScpiManager::SCPI_MeasureRangQ,         0 },
    { ":SENSe#:CURRent:DC:RANGe:AUTO?",               ScpiManager::SCPI_MeasureRangQ,         0 },
    { ":SENSe#:AVERage?",                             ScpiManager::SCPI_MeasureAverQ,         0 },
    { ":SENSe#:FUNCtion?",                            ScpiManager::SCPI_MeasureFuncQ,         0 },
    { ":SENSe#:SWEep:OFFS:POINts?",                   ScpiManager::SCPI_MeasureOffsQ,         0 },
    { ":SENSe#:SWEep:POINts?",                        ScpiManager::SCPI_MeasurePoinQ,         0 },
    { ":SENSe#:SWEep:TINTerval?",                     ScpiManager::SCPI_MeasureTintQ,         0 },
    { ":FETCh#:CURRent:HIGH?",                        ScpiManager::SCPI_MeasureCurrHighQ,     0 },
    { ":FETCh#:CURRent:LOW?",                         ScpiManager::SCPI_MeasureCurrLowQ,      0 },
    { ":FETCh#:CURRent:MAXimum?",                     ScpiManager::SCPI_MeasureCurrMaxQ,      0 },
    { ":FETCh#:CURRent:MINimum?",                     ScpiManager::SCPI_MeasureCurrMinQ,      0 },
    { ":FETCh#:DVMeter:HIGH?",                        ScpiManager::SCPI_MeasureDvmHighQ,      0 },
    { ":FETCh#:DVMeter:LOW?",                         ScpiManager::SCPI_MeasureDvmLowQ,       0 },
    { ":FETCh#:DVMeter:MAXimum?",                     ScpiManager::SCPI_MeasureDvmMaxQ,       0 },
    { ":FETCh#:DVMeter:MINimum?",                     ScpiManager::SCPI_MeasureDvmMinQ,       0 },
    { ":FETCh#:VOLTage:HIGH?",                        ScpiManager::SCPI_MeasureVoltHighQ,     0 },
    { ":FETCh#:VOLTage:LOW?",                         ScpiManager::SCPI_MeasureVoltLowQ,      0 },
    { ":FETCh#:VOLTage:MAXimum?",                     ScpiManager::SCPI_MeasureVoltMaxQ,      0 },
    { ":FETCh#:VOLTage:MINimum?",                     ScpiManager::SCPI_MeasureVoltMinQ,      0 },
    // --- Register -
    { ":STATus#:OPERation?",                          ScpiManager::SCPI_RegisterEvenQ,        0 },
    { ":STATus#:OPERation:EVENt?",                    ScpiManager::SCPI_RegisterEvenQ,        0 },
    { ":STATus#:OPERation:ENAB?",                     ScpiManager::SCPI_RegisterEnabQ,        0 },
    { ":STATus#:OPERation:CONDition?",                ScpiManager::SCPI_RegisterCondQ,        0 },
    { ":STATus#:QUEue?",                              ScpiManager::SCPI_RegisterNextQ,        0 },
    { ":STATus#:QUEue:NEXT?",                         ScpiManager::SCPI_RegisterNextQ,        0 },
    // --- Calibrate -
    { ":CALIbrate#:STARt?",                           ScpiManager::SCPI_CalibrateAllQ,        0 },
    { ":CALIbrate#:STARt:ALL?",                       ScpiManager::SCPI_CalibrateAllQ,        0 },
    { ":CALIbrate#:STEP?",                            ScpiManager::SCPI_CalibrateStepQ,       0 },
    // --- Trigger -
    { ":TRIGger#:SEQ2:SOURce?",                       ScpiManager::SCPI_TriggerSeq2SoQ,       0 },
    { ":TRIGger#:SEQ2:COUNt:CURRent?",                ScpiManager::SCPI_TriggerSeq2CoQ,       0 },
    { ":TRIGger#:SEQ2:COUNt:DVM?",                    ScpiManager::SCPI_TriggerSeq2CoQ,       0 },
    { ":TRIGger#:SEQ2:COUNt:VOLTage?",                ScpiManager::SCPI_TriggerSeq2CoQ,       0 },
    { ":TRIGger#:SEQ2:HYSTeresis:CURRent?",           ScpiManager::SCPI_TriggerSeq2HyQ,       0 },
    { ":TRIGger#:SEQ2:HYSTeresis:DVM?",               ScpiManager::SCPI_TriggerSeq2HyQ,       0 },
    { ":TRIGger#:SEQ2:HYSTeresis:VOLTage?",           ScpiManager::SCPI_TriggerSeq2HyQ,       0 },
    { ":TRIGger#:SEQ2:LEVel:CURRent?",                ScpiManager::SCPI_TriggerSeq2LeQ,       0 },
    { ":TRIGger#:SEQ2:LEVel:DVM?",                    ScpiManager::SCPI_TriggerSeq2LeQ,       0 },
    { ":TRIGger#:SEQ2:LEVel:VOLTage?",                ScpiManager::SCPI_TriggerSeq2LeQ,       0 },
    { ":TRIGger#:SEQ2:SLOPe:CURRent?",                ScpiManager::SCPI_TriggerSeq2SlQ,       0 },
    { ":TRIGger#:SEQ2:SLOPe:DVM?",                    ScpiManager::SCPI_TriggerSeq2SlQ,       0 },
    { ":TRIGger#:SEQ2:SLOPe:VOLTage?",                ScpiManager::SCPI_TriggerSeq2SlQ,       0 },
    { ":SOURce#:VOLTage:AMPL:TRIG?",                  ScpiManager::SCPI_TriggerAmplQ,         0 },
    { ":SOURce#:CURRent:TRIG?",                       ScpiManager::SCPI_TriggerCurrQ,         0 },
    { ":SOURce#:RES:TRIG?",                           ScpiManager::SCPI_TriggerResQ,          0 },

    SCPI_CMD_LIST_END
};


// --- 执行 -command function ---

scpi_result_t ScpiManager::SCPI_OutputState(scpi_t* context) {
    scpi_bool_t OpenOut;
    if(!SCPI_ParamBool(context, &OpenOut,true)){return SCPI_RES_ERR;}
    qCDebug(scpi)<<"SCPI_Output OpenOut: "<<OpenOut;
    quint8 func = OpenOut ? 0x01 : 0x00;

    return sendCmd(context,0x01,func,"");
}

scpi_result_t ScpiManager::SCPI_MeasureFunc(scpi_t* context) {
    int32_t choice;
    if (!SCPI_ParamChoice(context, m_CmdparaChoices, &choice, true)) {return SCPI_RES_ERR;}
    qCDebug(scpi) << "Channel choice:" << choice;

    auto* self = static_cast<ScpiManager*>(context->user_context);
    self->m_measurefunchoice = choice;
    if (choice > 2){return SCPI_RES_OK;}

    QByteArray data(1, static_cast<quint8>(choice));
    return sendCmd(context,0x04,0x10,data);
}

scpi_result_t ScpiManager::SCPI_CalibrateStep(scpi_t* context) {
    int32_t step;
    double parameter;

    if (SCPI_ParamInt(context, &step, true)) {
        if (SCPI_ParamDouble(context, &parameter, true)) {
            float f = static_cast<float>(parameter);
            QByteArray data(4, 0);
            qToLittleEndian(f, reinterpret_cast<uint8_t*>(data.data()));
            return sendCmd(context,0x07,step,data);
        }
    }

    return SCPI_RES_ERR;
}


// --- 辅助 -

scpi_result_t ScpiManager::sendCmd(scpi_t* context, quint8 cmd, quint8 func, const QByteArray &data) {
    int32_t channel;
    if(!SCPI_CommandNumbers(context, &channel, 1, 0)){return SCPI_RES_ERR;} // Array 1, Default Channel 0
    qCDebug(scpi)<<"SCPI_Processing Channel: "<<channel;

    auto* self = static_cast<ScpiManager*>(context->user_context);
    switch (channel){
        case 1:emit self->to_UartChannel1(cmd,func,data,true);break;
        case 2:emit self->to_UartChannel2(cmd,func,data,true);break;
        default:return SCPI_RES_ERR;;
    }

    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::sendIntCmd(scpi_t* context, quint8 cmd, quint8 func,quint8 bytes) {
    int value;
    if (!SCPI_ParamInt(context, &value, true)) {return SCPI_RES_ERR;}
    qCDebug(scpi) << "Channel Set Value to:" << value;

    QByteArray data;
    if (bytes == 1) {
        data.append(static_cast<quint8>(value));
    } else if (bytes == 2) {
        quint16 val = static_cast<quint16>(value);
        data.append(static_cast<quint8>(val & 0xFF));
        data.append(static_cast<quint8>((val >> 8) & 0xFF));
    }

    return sendCmd(context,cmd,func,data);
}

const scpi_choice_def_t ScpiManager::m_CmdparaChoices[] = {
    // --- output -
    {"HIGH",     1},
    {"LOW",      0},
    {"Llocal",   1},
    {"Lremote",  2},
    {"Hlocal",   3},
    {"Hremote",  4},
    // --- control -
    {"TRIP",     1},
    {"CC",       0},
    {"RESET",    0},
    {"SAVED0",   1},
    {"SAVED1",   2},
    {"SAVED2",   3},
    {"SAVED3",   4},
    {"SAVED4",   5},
    // --- measure -
    {"CURR",     0},
    {"DVM",      1},
    {"VOLT",     2},
    // -------------
    {"SCRR",     3},
    {"BTMP",     4},
    {"HTMP",     5},
    {"TMP1",     6},
    {"TMP2",     7},
    {"TMP3",     8},
    // --- trigger -
    {"BUS",      1},
    {"INT",      2},
    {"EXT",      3},
    {"POSitive", 1},
    {"NEGative", 2},
    {"EITHer",   3},
    SCPI_CHOICE_LIST_END
};

scpi_result_t ScpiManager::sendChoiceCmd(scpi_t* context,quint8 cmd, quint8 func) {
    int32_t choice;
    if (!SCPI_ParamChoice(context, m_CmdparaChoices, &choice, true)) {return SCPI_RES_ERR;}
    qCDebug(scpi) << "Channel choice:" << choice;

    QByteArray data(1, static_cast<quint8>(choice));
    return sendCmd(context,cmd,func,data);
}

scpi_result_t ScpiManager::sendFloatCmd(scpi_t* context, quint8 cmd, quint8 func) {
    double value;
    if (!SCPI_ParamDouble(context, &value, true)) {return SCPI_RES_ERR;}
    qCDebug(scpi) << "Channel Set Value to:" << value;

    float f = static_cast<float>(value);
    QByteArray data(4, 0);
    qToLittleEndian(f, reinterpret_cast<uint8_t*>(data.data()));

    return sendCmd(context,cmd,func,data);
}

scpi_result_t ScpiManager::sendBoolCmd(scpi_t* context, quint8 cmd, quint8 func) {
    scpi_bool_t OpenOut;
    if(!SCPI_ParamBool(context, &OpenOut,true)){return SCPI_RES_ERR;}
    qCDebug(scpi)<<"SCPI_Output OpenOut: "<<OpenOut;
    quint8 swi = OpenOut ? 0x01 : 0x00;

    QByteArray data(1, swi);
    return sendCmd(context,cmd,func,data);
}


bool ScpiManager::sendQueryCmd(scpi_t* context, quint8 cmd, quint8 func) {
    int32_t channel;
    if(!SCPI_CommandNumbers(context, &channel, 1, 0)){return false;} // Array 1, Default Channel 0
    qCDebug(scpi)<<"SCPI_Processing Channel: "<<channel;

    auto* self = static_cast<ScpiManager*>(context->user_context);
    QMutexLocker locker(&self->m_syncMutex);
    self->m_UartResponse_Return = false;

    switch (channel){
        case 1:emit self->to_UartChannel1(cmd,func,"",true);break;
        case 2:emit self->to_UartChannel2(cmd,func,"",true);break;
        default:return false;
    }

    // wait == unload m_syncMutex and wait
    if (!self->m_syncCondition.wait(&self->m_syncMutex, 600)) { // 600ms
        qCWarning(scpi) << "Query timeout - cmd:" << cmd << "func:" << func;
        return false;
    }

    if (!self->m_UartResponse_Return) {return false;}
    return true;
}

scpi_result_t ScpiManager::sendQueryIntCmd(scpi_t* context, quint8 cmd, quint8 func) {
    if(!sendQueryCmd(context,cmd,func)){return SCPI_RES_ERR;};

    auto* self = static_cast<ScpiManager*>(context->user_context);
    SCPI_ResultInt(context, self->m_CHintvalueReturn);
    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::sendQueryBoolCmd(scpi_t* context, quint8 cmd, quint8 func) {
    if(!sendQueryCmd(context,cmd,func)){return SCPI_RES_ERR;};

    auto* self = static_cast<ScpiManager*>(context->user_context);
    SCPI_ResultBool(context, self->m_CHStateReturn);
    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::sendQueryFloatCmd(scpi_t* context, quint8 cmd, quint8 func) {
    if(!sendQueryCmd(context,cmd,func)){return SCPI_RES_ERR;};

    auto* self = static_cast<ScpiManager*>(context->user_context);
    SCPI_ResultFloat(context, self->m_CHvalueReturn);
    return SCPI_RES_OK;
}


// --- 查询 -command function ---

scpi_result_t ScpiManager::SCPI_OutputBandQ(scpi_t* context) {
    if(!sendQueryCmd(context,0x01,0x88)){return SCPI_RES_ERR;};

    auto* self = static_cast<ScpiManager*>(context->user_context);
    if (self->m_CHStateReturn){
        SCPI_ResultMnemonic(context, "HIGH");
    }else {
        SCPI_ResultMnemonic(context, "LOW");
    }

    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::SCPI_OutputCompModeQ(scpi_t* context) {
    if(!sendQueryCmd(context,0x01,0x89)){return SCPI_RES_ERR;};

    auto* self = static_cast<ScpiManager*>(context->user_context);
    switch (self->m_CHintvalueReturn) {
        case 1:SCPI_ResultMnemonic(context, "Llocal");break;
        case 2:SCPI_ResultMnemonic(context, "Lremote");break;
        case 3:SCPI_ResultMnemonic(context, "Hlocal");break;
        case 4:SCPI_ResultMnemonic(context, "Hremote");break;
        default:return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::SCPI_ControlCurrQ(scpi_t* context) {
    if(!sendQueryCmd(context,0x03,0x82)){return SCPI_RES_ERR;};

    auto* self = static_cast<ScpiManager*>(context->user_context);
    if (self->m_CHStateReturn){
        SCPI_ResultMnemonic(context, "TRIP");
    }else {
        SCPI_ResultMnemonic(context, "CC");
    }

    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::SCPI_ControlSystQ(scpi_t* context) {
    if(!sendQueryCmd(context,0x03,0x85)){return SCPI_RES_ERR;};

    auto* self = static_cast<ScpiManager*>(context->user_context);
    switch (self->m_CHintvalueReturn) {
        case 0:SCPI_ResultMnemonic(context, "Reset");break;
        case 1:SCPI_ResultMnemonic(context, "Saved0");break;
        case 2:SCPI_ResultMnemonic(context, "Saved1");break;
        case 3:SCPI_ResultMnemonic(context, "Saved2");break;
        case 4:SCPI_ResultMnemonic(context, "Saved3");break;
        case 5:SCPI_ResultMnemonic(context, "Saved4");break;
        default:return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::SCPI_ControlLoadQ(scpi_t* context) {
    if(!sendQueryCmd(context,0x03,0x89)){return SCPI_RES_ERR;};

    auto* self = static_cast<ScpiManager*>(context->user_context);
    if (self->m_CHStateReturn){
        SCPI_ResultMnemonic(context, "TRIP");
    }else {
        SCPI_ResultMnemonic(context, "CC");
    }

    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::SCPI_MeasureFuncQ(scpi_t* context) {
    if(!sendQueryCmd(context,0x04,0x90)){return SCPI_RES_ERR;};

    auto* self = static_cast<ScpiManager*>(context->user_context);
    switch (self->m_CHintvalueReturn) {
        case 0:SCPI_ResultMnemonic(context, "CURR");break;
        case 1:SCPI_ResultMnemonic(context, "DVM");break;
        case 2:SCPI_ResultMnemonic(context, "VOLT");break;
        default:return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::SCPI_MeasureReadQ(scpi_t* context) {
    auto* self = static_cast<ScpiManager*>(context->user_context);
    switch (self->m_measurefunchoice){
        case 0: return sendQueryFloatCmd(context,0x04,0x81);
        case 1: return sendQueryFloatCmd(context,0x04,0x86);
        case 2: return sendQueryFloatCmd(context,0x04,0x80);
        case 3: return sendQueryFloatCmd(context,0x04,0x82);
        case 4: return sendQueryFloatCmd(context,0x04,0x83);
        case 5: return sendQueryFloatCmd(context,0x04,0x84);
        case 6: return sendQueryFloatCmd(context,0x04,0x8a);
        case 7: return sendQueryFloatCmd(context,0x04,0x8b);
        case 8: return sendQueryFloatCmd(context,0x04,0x8d);
        default:return SCPI_RES_OK;
    }
}

scpi_result_t ScpiManager::SCPI_CalibrateAllQ(scpi_t* context) {
    if(!sendQueryCmd(context,0x06,0x84)){return SCPI_RES_ERR;};

    auto* self = static_cast<ScpiManager*>(context->user_context);
    switch (self->m_CHintvalueReturn) {
        case 0:SCPI_ResultMnemonic(context, "OFF");break;
        case 1:SCPI_ResultMnemonic(context, "ALL");break;
        case 2:SCPI_ResultMnemonic(context, "ADC");break;
        case 3:SCPI_ResultMnemonic(context, "DAC");break;
        default:return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::SCPI_CalibrateStepQ(scpi_t* context) {
    int value;
    if (!SCPI_ParamInt(context, &value, true)) {return SCPI_RES_ERR;}
    qCDebug(scpi) << "Channel Set Value to:" << value;

    return sendQueryFloatCmd(context,0x07,value);
}

scpi_result_t ScpiManager::SCPI_TriggerSeq2SoQ(scpi_t* context) {
    if(!sendQueryCmd(context,0x08,0x85)){return SCPI_RES_ERR;};

    auto* self = static_cast<ScpiManager*>(context->user_context);
    switch (self->m_CHintvalueReturn) {
        case 1:SCPI_ResultMnemonic(context, "BUS");break;
        case 2:SCPI_ResultMnemonic(context, "INT");break;
        case 3:SCPI_ResultMnemonic(context, "EXT");break;
        default:return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::SCPI_TriggerSeq2SlQ(scpi_t* context) {
    if(!sendQueryCmd(context,0x08,0x89)){return SCPI_RES_ERR;};

    auto* self = static_cast<ScpiManager*>(context->user_context);
    switch (self->m_CHintvalueReturn) {
        case 1:SCPI_ResultMnemonic(context, "POSitive");break;
        case 2:SCPI_ResultMnemonic(context, "NEGative");break;
        case 3:SCPI_ResultMnemonic(context, "EITHer");break;
        default:return SCPI_RES_ERR;
    }

    return SCPI_RES_OK;
}


// --- Query写入 -command Auxiliary function ---

void ScpiManager::processCHStateResponse(bool state) {
    m_syncMutex.lock();

    m_CHStateReturn = state;
    m_UartResponse_Return = true;

    m_syncMutex.unlock();
    m_syncCondition.wakeAll();
}

void ScpiManager::processCHvalueResponse(float value) {
    m_syncMutex.lock();

    m_CHvalueReturn = value;
    m_UartResponse_Return = true;

    m_syncMutex.unlock();
    m_syncCondition.wakeAll();
}

void ScpiManager::processCHIntvalueResponse(int value) {
    m_syncMutex.lock();

    m_CHintvalueReturn = value;
    m_UartResponse_Return = true;

    m_syncMutex.unlock();
    m_syncCondition.wakeAll();
}


// SCPI Command API

QByteArray ScpiManager::processCommand(const QByteArray &command) {
    QMutexLocker locker(&m_callMutex);

    m_responseBuffer.clear();
    if (command.isEmpty()){return nullptr;}

    // Support streaming input
    SCPI_Input(&m_scpiContext, command.constData(), command.size());
    return m_responseBuffer; // if not query，return emtry
}

// ======================== 初化化模块 =================================

QByteArray ScpiManager::m_idnManufacturer;
QByteArray ScpiManager::m_idnModel;
QByteArray ScpiManager::m_idnSerialNumber;
QByteArray ScpiManager::m_idnVersion;

scpi_interface_t ScpiManager::m_interface;
char ScpiManager::m_inputBuffer[256] = {0};       // 初始化为0
scpi_error_t ScpiManager::m_errorQueue[10] = {};  // 初始化为空

ScpiManager::ScpiManager(QObject *parent) : QObject(parent) {
    // QString -> QByteArray
    m_idnManufacturer = ConfigManager::s_manufacturer.toUtf8();
    m_idnModel        = ConfigManager::s_model.toUtf8();
    m_idnSerialNumber = ConfigManager::s_serialNumber.toUtf8();
    m_idnVersion      = ConfigManager::s_firmwareVersion.toUtf8();

    m_interface.write   = staticWrite;
    m_interface.error   = staticError;
    m_interface.reset   = staticReset;
    m_interface.flush   = staticFlush;
    m_interface.control = staticControl;

    SCPI_Init(&m_scpiContext,
              m_scpiCommands,                               // 命令表
              &m_interface,                                 // 接口回调
              nullptr,                                      // 单位定义
              m_idnManufacturer.constData(),
              m_idnModel.constData(),
              m_idnSerialNumber.constData(),
              m_idnVersion.constData(),
              m_inputBuffer,                                // 输入缓冲区
              sizeof(m_inputBuffer),
              m_errorQueue,                                 // 错误队列
              sizeof(m_errorQueue) / sizeof(scpi_error_t));

    m_scpiContext.user_context = this;
}

size_t ScpiManager::staticWrite(scpi_t* context, const char* data, size_t len) {
    auto* self = static_cast<ScpiManager*>(context->user_context);
    if (self && len > 0) {
        self->m_responseBuffer.append(data, len);
        qCDebug(scpi) << "SCPI Query Response:" << data;
    }
    return len;
    // Automatically add \r\n
}

int ScpiManager::staticError(scpi_t* context, int_fast16_t err) {
    Q_UNUSED(context);
    qCWarning(scpi) << "SCPI Error Code:" << err << "Desc:" << SCPI_ErrorTranslate(err);
    return 0;
}

scpi_result_t ScpiManager::staticReset(scpi_t* context) {
    auto* self = static_cast<ScpiManager*>(context->user_context);
    Q_UNUSED(self);
    qCDebug(scpi) << "Executing hardware reset...";
    // 这里执行具体的硬件复位动作
    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::staticFlush(scpi_t* context) {
    Q_UNUSED(context);
    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::staticControl(scpi_t* context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val) {
    Q_UNUSED(context);
    qCDebug(scpi) << "Control Signal:" << (int)ctrl << "Value:" << val;
    return SCPI_RES_OK;
}
