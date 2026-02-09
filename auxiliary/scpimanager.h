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

Q_DECLARE_LOGGING_CATEGORY(scpi)

class ScpiManager : public QObject {
    Q_OBJECT

signals:
    // to C++ model control
    void sendFrame_Uart4(quint8 cmd, quint8 func, const QByteArray& param);
    void sendFrame_Uart5(quint8 cmd, quint8 func, const QByteArray& param);

public:
    explicit ScpiManager(QObject *parent = nullptr);
    ~ScpiManager() override = default;

    QByteArray processCommand(const QByteArray &command);

    // --- 查询写入 -command Auxiliary function ---
    void processCHStateResponse(bool state);
    void processCHvalueResponse(float value);

private:
    // --- libscpi 静态回调接口 ---
    static size_t staticWrite(scpi_t* context, const char* data, size_t len);
    static int    staticError(scpi_t* context, int_fast16_t err);
    static scpi_result_t staticReset(scpi_t* context);
    static scpi_result_t staticFlush(scpi_t* context);
    static scpi_result_t staticControl(scpi_t* context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val);

    // --- 执行 -command function ---
    static scpi_result_t SCPI_OutputState(scpi_t* context);
    static scpi_result_t SCPI_OutputBand(scpi_t* context);
    static scpi_result_t SCPI_OutputCompMode(scpi_t* context);

    // --- 查询 -command function ---
    static scpi_result_t SCPI_OutputStateQ(scpi_t* context);
    static scpi_result_t SCPI_OutputBandQ(scpi_t* context);
    static scpi_result_t SCPI_OutputCompModeQ(scpi_t* context);

private:
    static const scpi_command_t m_scpiCommands[];

    bool m_CHStateReturn{false};
    float m_CHvalueReturn{0.0f};

    QMutex m_syncMutex;
    QWaitCondition m_syncCondition;
    bool m_UartResponse_Return = false;

    scpi_t m_scpiContext;
    scpi_interface_t m_interface;
    char m_inputBuffer[256];
    scpi_error_t m_errorQueue[10];

    QByteArray m_responseBuffer;
    QByteArray m_idnManufacturer;
    QByteArray m_idnModel;
    QByteArray m_idnSerialNumber;
    QByteArray m_idnVersion;
};
