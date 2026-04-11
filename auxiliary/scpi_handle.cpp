#include "scpi_handle.h"
#include <QtCore>

Q_LOGGING_CATEGORY(scpi, "SCPI:")

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
    {"LIMit",    0},
    {"CC",       0},
    {"RST",      0},
    {"SAV0",     1},
    {"SAV1",     2},
    {"SAV2",     3},
    {"SAV3",     4},
    {"SAV4",     5},
    // --- measure -
    {"CURR",     0},
    {"DVM",      1},
    {"VOLT",     2},
    {"SCRR",     3},
    {"BTMP",     4},
    {"HTMP",     5},
    {"TMP1",     6},
    {"TMP2",     7},
    {"TMP3",     8},
    // --- trigger -
    {"TRANsient",0},
    {"OUTPut",   1},
    {"BUS",      1},
    {"INT",      2},
    {"EXT",      3},
    {"POSitive", 1},
    {"NEGative", 2},
    {"EITHer",   3},
    SCPI_CHOICE_LIST_END
};

const scpi_command_t ScpiManager::m_scpiCommands[] = {
    { "*CLS",      SCPI_CoreCls,    0 },//清除所有状态数据结构（寄存器、错误队列）
    { "*ESE",      SCPI_CoreEse,    0 },//设置标准事件状态启用寄存器
    { "*OPC",      SCPI_CoreOpc,    0 },//设置OPC位
    { "*RST",      SCPI_CoreRst,    0 },//复位设备到已知状态
    { "*SRE",      SCPI_CoreSre,    0 },//设置服务请求启用寄存器
    { "*WAI",      SCPI_CoreWai,    0 },//等待操作完成
    { "*ESE?",     SCPI_CoreEseQ,   0 },//读取标准事件状态启用寄存器
    { "*ESR?",     SCPI_CoreEsrQ,   0 },//读取标准事件状态寄存器（读取后自动清零）
    { "*IDN?",     SCPI_CoreIdnQ,   0 },
    { "*OPC?",     SCPI_CoreOpcQ,   0 },//操作完成时设置OPC位
    { "*SRE?",     SCPI_CoreSreQ,   0 },//读取服务请求启用寄存器
    { "*STB?",     SCPI_CoreStbQ,   0 },//读取状态字节寄存器
    { "*TST?",     SCPI_CoreTstQ,   0 },//自测试查询

    { ":READ?",                                       ScpiManager::SCPI_ReadQ,                0 },
    // --- Output -------------------------------------------------------------------------------
    { ":OUTPut#",                                     ScpiManager::SCPI_OutputState,          0 },
    { ":OUTPut#:STATe",                               ScpiManager::SCPI_OutputState,          0 },
    { ":OUTPut#:BANDwidth",                           ScpiManager::SCPI_OutputBand,           0 },
    { ":OUTPut#:COMPensation:MODE",                   ScpiManager::SCPI_OutputCompMode,       0 },
    // -
    { ":OUTPut#?",                                    ScpiManager::SCPI_OutputStateQ,         0 },
    { ":OUTPut#:STATe?",                              ScpiManager::SCPI_OutputStateQ,         0 },
    { ":OUTPut#:BANDwidth?",                          ScpiManager::SCPI_OutputBandQ,          0 },
    { ":OUTPut#:COMPensation:MODE?",                  ScpiManager::SCPI_OutputCompModeQ,      0 },
    // --- Setting ------------------------------------------------------------------------------
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
    // -
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
    // --- Control ------------------------------------------------------------------------------
    { ":SOURce#:VOLTage:PROTection:CLAMp",            ScpiManager::SCPI_ControlClamp,         0 },
    { ":SOURce#:CURRent:TYPE",                        ScpiManager::SCPI_ControlCurr,          0 },
    { ":SOURce#:CURRent:LIMit:TYPE",                  ScpiManager::SCPI_ControlCurr,          0 },
    { ":SYSTem:POSetup",                              ScpiManager::SCPI_ControlSyst,          0 },
    { "*SAV",                                         ScpiManager::SCPI_ControlSAV,           0 },
    { "*RCL",                                         ScpiManager::SCPI_ControlRCL,           0 },
    { ":LOAD#:INDEpendence",                          ScpiManager::SCPI_ControlInde,          0 },
    { ":LOAD#:INDEpendence:STATe",                    ScpiManager::SCPI_ControlInde,          0 },
    { ":LOAD#:CURRent:TYPE",                          ScpiManager::SCPI_ControlLoad,          0 },
    { ":LOAD#:CURRent:LIMit:TYPE",                    ScpiManager::SCPI_ControlLoad,          0 },
    // -
    { ":SOURce#:VOLTage:PROTection:CLAMp?",           ScpiManager::SCPI_ControlClampQ,        0 },
    { ":SOURce#:CURRent:TYPE?",                       ScpiManager::SCPI_ControlCurrQ,         0 },
    { ":SOURce#:CURRent:LIMit:TYPE?",                 ScpiManager::SCPI_ControlCurrQ,         0 },
    { ":SYSTem:POSetup?",                             ScpiManager::SCPI_ControlSystQ,         0 },
    { ":LOAD#:INDEpendence?",                         ScpiManager::SCPI_ControlIndeQ,         0 },
    { ":LOAD#:INDEpendence:STATe?",                   ScpiManager::SCPI_ControlIndeQ,         0 },
    { ":LOAD#:CURRent:TYPE?",                         ScpiManager::SCPI_ControlLoadQ,         0 },
    { ":LOAD#:CURRent:LIMit:TYPE?",                   ScpiManager::SCPI_ControlLoadQ,         0 },
    // --- Measurement --------------------------------------------------------------------------
    { ":SENSe#:NPLCycles",                            ScpiManager::SCPI_MeasureNplc,          0 },
    { ":SENSe#:CURRent:RANGe:TIMe",                   ScpiManager::SCPI_MeasureTime,          0 },
    { ":SENSe#:CURRent:DC:RANGe:TIMe",                ScpiManager::SCPI_MeasureTime,          0 },
    { ":SENSe#:CURRent:RANGe",                        ScpiManager::SCPI_MeasureRang,          0 },
    { ":SENSe#:CURRent:DC:RANGe",                     ScpiManager::SCPI_MeasureRang,          0 },
    { ":SENSe#:CURRent:RANGe:UPPer",                  ScpiManager::SCPI_MeasureRang,          0 },
    { ":SENSe#:CURRent:DC:RANGe:UPPer",               ScpiManager::SCPI_MeasureRang,          0 },
    { ":SENSe#:AVERage",                              ScpiManager::SCPI_MeasureAver,          0 },
    { ":SENSe#:FUNCtion",                             ScpiManager::SCPI_MeasureFunc,          0 },
    { ":SENSe#:SWEep:OFFS:POINts",                    ScpiManager::SCPI_MeasureOffs,          0 },
    { ":SENSe#:SWEep:POINts",                         ScpiManager::SCPI_MeasurePoin,          0 },
    { ":SENSe#:SWEep:TINTerval",                      ScpiManager::SCPI_MeasureTint,          0 },
    // -
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
    // --- Register -----------------------------------------------------------------------------
    { ":STATus#:QUEue:CLEar",                         ScpiManager::SCPI_RegisterCle,          0 },
    // -
    { ":STATus#:OPERation?",                          ScpiManager::SCPI_RegisterEvenQ,        0 },
    { ":STATus#:OPERation:EVENt?",                    ScpiManager::SCPI_RegisterEvenQ,        0 },
    { ":STATus#:OPERation:ENAB?",                     ScpiManager::SCPI_RegisterEnabQ,        0 },
    { ":STATus#:OPERation:CONDition?",                ScpiManager::SCPI_RegisterCondQ,        0 },
    { ":STATus#:QUEue?",                              ScpiManager::SCPI_RegisterNextQ,        0 },
    { ":STATus#:QUEue:NEXT?",                         ScpiManager::SCPI_RegisterNextQ,        0 },
    // --- Calibrate ----------------------------------------------------------------------------
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
    // -
    { ":CALIbrate#:STARt?",                           ScpiManager::SCPI_CalibrateAllQ,        0 },
    { ":CALIbrate#:STARt:ALL?",                       ScpiManager::SCPI_CalibrateAllQ,        0 },
    { ":CALIbrate#:STEP?",                            ScpiManager::SCPI_CalibrateStepQ,       0 },
    // --- Trigger ------------------------------------------------------------------------------
    { ":ABORt#",                                      ScpiManager::SCPI_TriggerAbort,         0 },
    { ":INITiate#:SEQuence",                          ScpiManager::SCPI_TriggerSequene,       0 },
    { ":INITiate#:IMMediate:SEQuence",                ScpiManager::SCPI_TriggerSequene,       0 },
    { ":INITiate#:CONTinuous:SEQuence1",              ScpiManager::SCPI_TriggerCont,          0 },
    { ":INITiate#:CONTinuous:NAME",                   ScpiManager::SCPI_TriggerContname,      0 },
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
    // -
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

    SCPI_CMD_LIST_END // Empty sentry post
};

// --- 执行 -command function --------------------------------------------------------------------

scpi_result_t ScpiManager::SCPI_OutputState(scpi_t* context) {
    scpi_bool_t OpenOut;
    if(SCPI_ParamBool(context, &OpenOut,true)){
        qCDebug(scpi)<<"[SCPI_OutputState]:SCPI_Output OpenOut: "<<OpenOut;
        quint8 func = OpenOut ? 0x01 : 0x00;
        return sendQueryCmd(context,0x01,func,"");
    }

    SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE); // parameter value is invalid.
    return SCPI_RES_ERR;
}

scpi_result_t ScpiManager::SCPI_ControlSAV(scpi_t* context) {
    int value;
    if (SCPI_ParamInt(context, &value, true)) {
        qCDebug(scpi) <<"[SCPI_ControlSAV]:Channel SAV Set Value:"<< value;
        QByteArray data(1, static_cast<quint8>(value));
        sendAllCHCmd(context,0x03,0x06,data);
        return SCPI_RES_OK;
    }

    SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE); // parameter value is invalid.
    return SCPI_RES_ERR;
}

scpi_result_t ScpiManager::SCPI_ControlRCL(scpi_t* context) {
    int value;
    if (SCPI_ParamInt(context, &value, true)) {
        qCDebug(scpi) <<"[SCPI_ControlRCL]:Channel RCL Set Value:" << value;
        QByteArray data(1, static_cast<quint8>(value));
        sendAllCHCmd(context,0x03,0x07,data);
        return SCPI_RES_OK;
    }

    SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE); // parameter value is invalid.
    return SCPI_RES_ERR;
}

scpi_result_t ScpiManager::SCPI_MeasureFunc(scpi_t* context) {
    // 0x04-cmd 0x10-func of little use .conflicts with under. Therefore, selection not supported.
    int32_t choice;
    if (SCPI_ParamChoice(context, m_CmdparaChoices, &choice, true)) {
        qCDebug(scpi) <<"[SCPI_MeasureFunc]:Param choice: "<< choice;
        switch (choice){ // Only query ,not response. Read -> response
            case 0: return sendQueryCmd(context,0x04,0x81);//CURR
            case 1: return sendQueryCmd(context,0x04,0x86);//DVM
            case 2: return sendQueryCmd(context,0x04,0x80);//VOLT
            case 3: return sendQueryCmd(context,0x04,0x82);//SCRR
            case 4: return sendQueryCmd(context,0x04,0x83);//BTMP
            case 5: return sendQueryCmd(context,0x04,0x84);//HTMP
            case 6: return sendQueryCmd(context,0x04,0x8a);//TMP1
            case 7: return sendQueryCmd(context,0x04,0x8b);//TMP2
            case 8: return sendQueryCmd(context,0x04,0x8d);//TMP3
            default:break;
        }
    }

    SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE); // parameter value is invalid.
    return SCPI_RES_ERR;
}

scpi_result_t ScpiManager::SCPI_CalibrateStep(scpi_t* context) {
    int32_t step;
    float parameter;
    if (SCPI_ParamInt(context, &step, true)) {
        if (SCPI_ParamFloat(context, &parameter, true)) {
            QByteArray data(4, 0);
            qToBigEndian(parameter, reinterpret_cast<uint8_t*>(data.data()));
            return sendQueryCmd(context,0x07,step,data);
        }
    }

    SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE); // parameter value is invalid.
    return SCPI_RES_ERR;
}

// --- 查询 -command function --------------------------------------------------------------------

scpi_result_t ScpiManager::SCPI_OutputBandQ(scpi_t* context) {
    auto* self = static_cast<ScpiManager*>(context->user_context);
    if(sendQueryCmd(context,0x01,0x88)){
        SCPI_ResultMnemonic(context, self->m_CHStateReturn ? "HIGH" : "LOW");
        return SCPI_RES_OK;
    }

    return SCPI_RES_ERR;
}

scpi_result_t ScpiManager::SCPI_OutputCompModeQ(scpi_t* context) {
    auto* self = static_cast<ScpiManager*>(context->user_context);
    if(sendQueryCmd(context,0x01,0x89)){
        switch (self->m_CHIntReturn) {
            case 1:SCPI_ResultMnemonic(context, "Llocal"); return SCPI_RES_OK;
            case 2:SCPI_ResultMnemonic(context, "Lremote");return SCPI_RES_OK;
            case 3:SCPI_ResultMnemonic(context, "Hlocal"); return SCPI_RES_OK;
            case 4:SCPI_ResultMnemonic(context, "Hremote");return SCPI_RES_OK;
            default:break;
        }
    }

    return SCPI_RES_ERR;
}

scpi_result_t ScpiManager::SCPI_ControlCurrQ(scpi_t* context) {
    auto* self = static_cast<ScpiManager*>(context->user_context);
    if(sendQueryCmd(context,0x03,0x82)){
        SCPI_ResultMnemonic(context, self->m_CHStateReturn ? "TRIP" : "LIM");
        return SCPI_RES_OK;
    }

    return SCPI_RES_ERR;
}

scpi_result_t ScpiManager::SCPI_ControlSystQ(scpi_t* context) {
    auto* self = static_cast<ScpiManager*>(context->user_context);
    if(sendQueryCmd(context,0x03,0x85)){
        switch (self->m_CHIntReturn) {
            case 0:SCPI_ResultMnemonic(context, "RST"); return SCPI_RES_OK;
            case 1:SCPI_ResultMnemonic(context, "SAV0");return SCPI_RES_OK;
            case 2:SCPI_ResultMnemonic(context, "SAV1");return SCPI_RES_OK;
            case 3:SCPI_ResultMnemonic(context, "SAV2");return SCPI_RES_OK;
            case 4:SCPI_ResultMnemonic(context, "SAV3");return SCPI_RES_OK;
            case 5:SCPI_ResultMnemonic(context, "SAV4");return SCPI_RES_OK;
            default:break;
        }
    }

    return SCPI_RES_ERR;
}

scpi_result_t ScpiManager::SCPI_ControlLoadQ(scpi_t* context) {
    auto* self = static_cast<ScpiManager*>(context->user_context);
    if(sendQueryCmd(context,0x03,0x89)){
        SCPI_ResultMnemonic(context, self->m_CHStateReturn ? "TRIP" : "LIM");
        return SCPI_RES_OK;
    }

    return SCPI_RES_ERR;
}

scpi_result_t ScpiManager::SCPI_MeasureFuncQ(scpi_t* context) {
    auto* self = static_cast<ScpiManager*>(context->user_context);
    if(sendQueryCmd(context,0x04,0x90)){
        switch (self->m_CHIntReturn) {
            case 0:SCPI_ResultMnemonic(context, "CURR");return SCPI_RES_OK;
            case 1:SCPI_ResultMnemonic(context, "DVM"); return SCPI_RES_OK;
            case 2:SCPI_ResultMnemonic(context, "VOLT");return SCPI_RES_OK;
            default:break;
        }
    }

    return SCPI_RES_ERR;
}

scpi_result_t ScpiManager::SCPI_CalibrateAllQ(scpi_t* context) {
    auto* self = static_cast<ScpiManager*>(context->user_context);
    if(sendQueryCmd(context,0x06,0x84)){
        switch (self->m_CHIntReturn) {
            case 0:SCPI_ResultMnemonic(context, "OFF");return SCPI_RES_OK;
            case 1:SCPI_ResultMnemonic(context, "ALL");return SCPI_RES_OK;
            case 2:SCPI_ResultMnemonic(context, "ADC");return SCPI_RES_OK;
            case 3:SCPI_ResultMnemonic(context, "DAC");return SCPI_RES_OK;
            default:break;
        }
    }

    return SCPI_RES_ERR;
}

scpi_result_t ScpiManager::SCPI_CalibrateStepQ(scpi_t* context) {
    int value;
    if (SCPI_ParamInt(context, &value, true)) {
        qCDebug(scpi) <<"[SCPI_CalibrateStepQ]:Step Value: "<< value;
        return sendQuery(context,0x07,value);
    }

    SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE); // parameter value is invalid.
    return SCPI_RES_ERR;
}

scpi_result_t ScpiManager::SCPI_TriggerSeq2SoQ(scpi_t* context) {
    auto* self = static_cast<ScpiManager*>(context->user_context);
    if(sendQueryCmd(context,0x08,0x85)){
        switch (self->m_CHIntReturn) {
            case 1:SCPI_ResultMnemonic(context, "BUS");return SCPI_RES_OK;
            case 2:SCPI_ResultMnemonic(context, "INT");return SCPI_RES_OK;
            case 3:SCPI_ResultMnemonic(context, "EXT");return SCPI_RES_OK;
            default:break;
        }
    }

    return SCPI_RES_ERR;
}

scpi_result_t ScpiManager::SCPI_TriggerSeq2SlQ(scpi_t* context) {
    auto* self = static_cast<ScpiManager*>(context->user_context);
    if(sendQueryCmd(context,0x08,0x89)){
        switch (self->m_CHIntReturn) {
            case 1:SCPI_ResultMnemonic(context, "POSitive");return SCPI_RES_OK;
            case 2:SCPI_ResultMnemonic(context, "NEGative");return SCPI_RES_OK;
            case 3:SCPI_ResultMnemonic(context, "EITHer");  return SCPI_RES_OK;
            default:break;
        }
    }

    return SCPI_RES_ERR;
}

// Auxiliary ---------------------------------------------------------------------------

scpi_result_t ScpiManager::sendQuery(scpi_t* context, quint8 cmd, quint8 func) {
    if(sendQueryCmd(context,cmd,func)){
        return SCPI_ReadQ(context);
    }

    return SCPI_RES_ERR;
}

// Write
scpi_result_t ScpiManager::sendBoolCmd(scpi_t* context, quint8 cmd, quint8 func) {
    scpi_bool_t OpenOut;
    if(SCPI_ParamBool(context, &OpenOut,true)){
        qCDebug(scpi)<<"[sendBoolCmd]:SCPI_Output OpenOut: "<<OpenOut;
        quint8 swi = OpenOut ? 0x01 : 0x00;
        QByteArray data(1, swi);
        return sendQueryCmd(context,cmd,func,data);
    }

    SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE); // parameter value is invalid.
    return SCPI_RES_ERR;
}

scpi_result_t ScpiManager::sendFloatCmd(scpi_t* context, quint8 cmd, quint8 func) {
    float value;
    if (SCPI_ParamFloat(context, &value, true)) {
        qCDebug(scpi) <<"[sendFloatCmd]:Channel Set Value:"<< value;
        QByteArray data(4, 0);
        qToBigEndian(value, reinterpret_cast<uint8_t*>(data.data()));
        return sendQueryCmd(context,cmd,func,data);
    }

    SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE); // parameter value is invalid.
    return SCPI_RES_ERR;
}

scpi_result_t ScpiManager::sendChoiceCmd(scpi_t* context,quint8 cmd, quint8 func) {
    int32_t choice;
    if (SCPI_ParamChoice(context, m_CmdparaChoices, &choice, true)) {
        qCDebug(scpi) <<"[sendChoiceCmd]:Channel choice: "<< choice;
        QByteArray data(1, static_cast<quint8>(choice));
        return sendQueryCmd(context,cmd,func,data);
    }

    SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE); // parameter value is invalid.
    return SCPI_RES_ERR;
}

scpi_result_t ScpiManager::sendIntCmd(scpi_t* context, quint8 cmd, quint8 func,quint8 bytes) {
    int value;
    if (SCPI_ParamInt(context, &value, true)) {
        qCDebug(scpi) <<"[sendIntCmd]:Channel Set Value:"<< value;

        QByteArray data;
        if (bytes == 1) {
            data.append(static_cast<quint8>(value));
        } else {
            quint16 val = static_cast<quint16>(value);
            data.append(static_cast<quint8>((val >> 8) & 0xFF));
            data.append(static_cast<quint8>(val & 0xFF));
        }

        return sendQueryCmd(context,cmd,func,data);
    }

    SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE); // parameter value is invalid.
    return SCPI_RES_ERR;
}

// Common

scpi_result_t ScpiManager::SCPI_ReadQ(scpi_t* context) {
    auto* self = static_cast<ScpiManager*>(context->user_context);
    switch (self->m_ReturnType){
        case 0: return SCPI_RES_OK; // emtry response
        case 1: SCPI_ResultBool(context, self->m_CHStateReturn); return SCPI_RES_OK;
        case 2: SCPI_ResultFloat(context, self->m_CHFloatReturn);return SCPI_RES_OK;
        case 3: SCPI_ResultInt(context, self->m_CHIntReturn);    return SCPI_RES_OK;
        default:break;
    }

    SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR); // Execution error
    return SCPI_RES_ERR;
}

void ScpiManager::sendAllCHCmd(scpi_t* context, quint8 cmd, quint8 func, const QByteArray &data) {
    auto* self = static_cast<ScpiManager*>(context->user_context);
    #define CHANNEL(n) \
        emit self->to_UartChannel##n(cmd,func,data,true);

    CHANNEL_COUNT
    #undef CHANNEL
    return;
}

void ScpiManager::sendSingleCHCmd(scpi_t* context, quint8 cmd, quint8 func, const QByteArray &data) {
    auto* self = static_cast<ScpiManager*>(context->user_context);

    switch (self->m_channel){
        #define CHANNEL(n) \
            case n: emit self->to_UartChannel##n(cmd,func,data,true); return;

        CHANNEL_COUNT
        #undef CHANNEL
        default: return;
    }
}

scpi_result_t ScpiManager::sendQueryCmd(scpi_t* context, quint8 cmd, quint8 func, const QByteArray &data) {
    auto* self = static_cast<ScpiManager*>(context->user_context);
    if(SCPI_CommandNumbers(context, &self->m_channel, 1, 0)){ // Array 1, Default Channel 0
        qCDebug(scpi)<<"[sendQueryCmd]:SCPI_Processing Channel: "<<self->m_channel;

        QMutexLocker locker(&self->m_syncMutex);
        sendSingleCHCmd(context,cmd,func,data);
        if(self->m_syncCondition.wait(&self->m_syncMutex, 600)){
            // unload m_syncMutex and wait . wake = true / timeout = false   // 600ms
            return SCPI_RES_OK;
        }

        SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR); // Execution error
        return SCPI_RES_ERR;
    }

    SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE); // parameter value is invalid.
    return SCPI_RES_ERR;
}

// API ---------------------------------------------------------------------------

void ScpiManager::processCHVoidResponse(){
    m_syncMutex.lock();
    m_ReturnType = 0;
    m_syncMutex.unlock();
    m_syncCondition.wakeAll();
}

void ScpiManager::processCHStateResponse(bool state) {
    m_syncMutex.lock();
    m_ReturnType = 1;
    m_CHStateReturn = state;
    m_syncMutex.unlock();
    m_syncCondition.wakeAll();
}

void ScpiManager::processCHFloatResponse(float chf) {
    m_syncMutex.lock();
    m_ReturnType = 2;
    m_CHFloatReturn = chf;
    m_syncMutex.unlock();
    m_syncCondition.wakeAll();
}

void ScpiManager::processCHIntResponse(int cht) {
    m_syncMutex.lock();
    m_ReturnType = 3;
    m_CHIntReturn = cht;
    m_syncMutex.unlock();
    m_syncCondition.wakeAll();
}

QByteArray ScpiManager::processCommand(const QByteArray &command) {
    QMutexLocker locker(&m_callMutex);

    m_responseBuffer.clear();
    SCPI_Input(&m_scpiContext, command.constData(), command.size());
    return m_responseBuffer; // if not query, return emtry
}

//---------------------------------------------------------------------------------

ScpiManager::ScpiManager(QObject *parent) : QObject(parent) {
    // QString -> QByteArray
    m_idnManufacturer = ConfigManager::s_manufacturer.toUtf8();
    m_idnModel        = ConfigManager::s_model.toUtf8();
    m_idnSerialNumber = ConfigManager::s_serialNumber.toUtf8();
    m_idnVersion      = ConfigManager::s_firmwareVersion.toUtf8();

    m_interface.flush   = staticFlush;
    m_interface.reset   = staticReset;
    m_interface.error   = staticError;
    m_interface.write   = staticWrite;
    m_interface.control = staticControl;

    SCPI_Init(&m_scpiContext,
              m_scpiCommands,
              &m_interface,
              nullptr,
              m_idnManufacturer.constData(),
              m_idnModel.constData(),
              m_idnSerialNumber.constData(),
              m_idnVersion.constData(),
              m_inputBuffer,
              sizeof(m_inputBuffer),
              m_errorQueue,
              sizeof(m_errorQueue) / sizeof(scpi_error_t));

    m_scpiContext.user_context = this;
}

scpi_result_t ScpiManager::staticFlush(scpi_t* context) {
    // Send immediately, no need for forceful pressure
    Q_UNUSED(context);
    return SCPI_RES_OK;
}

scpi_result_t ScpiManager::staticReset(scpi_t* context) {
    auto* self = static_cast<ScpiManager*>(context->user_context);
    qCDebug(scpi) <<"[staticReset]:SCPI and channel Reset";

    #define CHANNEL(n) \
        emit self->to_UartChannel##n##Reset();

    CHANNEL_COUNT
    #undef CHANNEL

    memset(self->m_inputBuffer, 0, sizeof(self->m_inputBuffer));
    memset(self->m_errorQueue, 0, sizeof(self->m_errorQueue));
    self->m_responseBuffer.clear();

    return SCPI_RES_OK;
}

int ScpiManager::staticError(scpi_t* context, int_fast16_t err) {
    auto* self = static_cast<ScpiManager*>(context->user_context);
    qCWarning(scpi) <<"[staticError]:SCPI Error Code:"<< err << "Desc:" << SCPI_ErrorTranslate(err);
    QString errorMsg = QString("%1,\"%2\"").arg(err).arg(SCPI_ErrorTranslate(err));
    self->m_responseBuffer.append(errorMsg.toUtf8());
    return 0;
}

size_t ScpiManager::staticWrite(scpi_t* context, const char* data, size_t len) {
    auto* self = static_cast<ScpiManager*>(context->user_context);
    qCDebug(scpi) <<"[staticWrite]:SCPI Response: "<< data;
    self->m_responseBuffer.append(data, len);
    return len; // Automatically add \r\n
}

scpi_result_t ScpiManager::staticControl(scpi_t* context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val) {
    // To accommodate the hardware interface, it is not supported.
    Q_UNUSED(context);Q_UNUSED(ctrl);Q_UNUSED(val);
    return SCPI_RES_OK;
}
