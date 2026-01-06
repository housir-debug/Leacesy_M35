#ifndef SCPIMANAGER_H
#define SCPIMANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "scpi/scpi.h"
#include "scpi/types.h"

#ifdef __cplusplus
}
#endif

#include <QObject>
#include <QByteArray>
#include <QMutex>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(scpi)

class ScpiManager : public QObject {
    Q_OBJECT

public:
    explicit ScpiManager(QObject *parent = nullptr);
    ~ScpiManager();

    bool init(const QString &manufacturer, const QString &model,
              const QString &serial, const QString &version);

    QByteArray processCommand(const QByteArray &command);

    scpi_t* context() { return &m_scpiContext; } // 获取SCPI上下文（用于调试等）
    static ScpiManager* instance(){return s_instance;};

private:
    // ===== 静态回调函数（C接口）=====
    static size_t staticWriteCallback(scpi_t* context, const char* data, size_t len);
    static int staticErrorCallback(scpi_t* context, int_fast16_t err);
    static scpi_result_t staticControlCallback(scpi_t* context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val);
    static scpi_result_t staticFlushCallback(scpi_t* context);
    static scpi_result_t staticResetCallback(scpi_t* context);
    // ===== 实例处理函数 =====
    size_t handleWrite(const char* data, size_t len);
    int handleError(int_fast16_t err);
    scpi_result_t handleControl(scpi_ctrl_name_t ctrl, scpi_reg_val_t val);
    scpi_result_t handleFlush();
    scpi_result_t handleReset();

    // ===== SCPI命令实现函数 =====
    static scpi_result_t scpiIdentify(scpi_t* context);
    static scpi_result_t scpiReset(scpi_t* context);
    static scpi_result_t scpiCls(scpi_t* context);
    static scpi_result_t scpiEsrQ(scpi_t* context);
    static scpi_result_t scpiNetworkIpQ(scpi_t* context);

private:
    static ScpiManager* s_instance;

    scpi_t m_scpiContext;
    scpi_interface_t interface;
    static const scpi_command_t m_scpiCommands[];

    // 响应缓冲区
    QByteArray m_responseBuffer;
    QMutex m_bufferMutex;

    // 仪器信息
    QByteArray m_idnManufacturer;
    QByteArray m_idnModel;
    QByteArray m_idnSerial;
    QByteArray m_idnVersion;
};

#endif // SCPIMANAGER_H
