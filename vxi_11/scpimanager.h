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
#include "config_manager.h"

Q_DECLARE_LOGGING_CATEGORY(scpi)

class ScpiManager : public QObject {
    Q_OBJECT

public:
    explicit ScpiManager(QObject *parent = nullptr);
    ~ScpiManager();

    bool init();
    QByteArray processCommand(const QByteArray &command);

private:
    static int staticErrorCallback(scpi_t* context, int_fast16_t err){Q_UNUSED(context);Q_UNUSED(err);return 0;};
    static scpi_result_t staticControlCallback(scpi_t* context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val){Q_UNUSED(context);Q_UNUSED(ctrl);Q_UNUSED(val);return SCPI_RES_OK;};
    static scpi_result_t staticFlushCallback(scpi_t* context){Q_UNUSED(context);return SCPI_RES_OK;};
    static scpi_result_t staticResetCallback(scpi_t* context){Q_UNUSED(context);return SCPI_RES_OK;};
    static size_t staticWriteCallback(scpi_t* context, const char* data, size_t len);

    static scpi_result_t scpiIdentify(scpi_t* context);
    static scpi_result_t scpiReset(scpi_t* context);
    static scpi_result_t scpiCls(scpi_t* context);
    static scpi_result_t scpiEsrQ(scpi_t* context);
    static scpi_result_t scpiNetworkIpQ(scpi_t* context);

private:
    enum EsrBits {
        ESR_OPC1        = (1 << 0),  // 操作完成
        ESR_CMD_ERROR   = (1 << 2),  // 命令错误
        ESR_EXE_ERROR   = (1 << 3),  // 执行错误
        ESR_DEV_ERROR   = (1 << 4),  // 设备特定错误
        ESR_QUERY_ERROR = (1 << 5),  // 查询错误
        ESR_USER6       = (1 << 6),  // 用户定义6（保留）
        ESR_POWER_ON    = (1 << 7)   // 电源开启
    };

    QMutex m_statusMutex;
    uint8_t m_esrRegister{0x00};
    QMutex m_bufferMutex;
    QByteArray m_responseBuffer;

    scpi_t m_scpiContext;
    scpi_interface_t interface;

    static ScpiManager* s_instance;
    static const scpi_command_t m_scpiCommands[];
};

#endif // SCPIMANAGER_H
