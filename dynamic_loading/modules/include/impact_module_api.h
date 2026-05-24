#ifndef IMPACT_MODULE_API_H
#define IMPACT_MODULE_API_H

#include "impact_displacement.h"

#include <stdint.h>

/* impact_displacement 走 command/call 型 ABI，宿主和模块通过这些命令交互。 */
typedef enum
{
    IMPACT_MODULE_CMD_GET_VERSION = 1u,
    IMPACT_MODULE_CMD_GET_DEFAULT_CFG = 2u,
    IMPACT_MODULE_CMD_CONFIGURE = 3u,
    IMPACT_MODULE_CMD_RESET = 4u,
    IMPACT_MODULE_CMD_BEGIN_EVENT = 5u,
    IMPACT_MODULE_CMD_SET_BASELINE = 6u,
    IMPACT_MODULE_CMD_FEED_SAMPLE = 7u,
    IMPACT_MODULE_CMD_END_EVENT = 8u,
    IMPACT_MODULE_CMD_GET_DIAG = 9u,
} impact_module_command_t;

/* 宿主先算出事件前窗基线均值，再把结果传给模块。 */
typedef struct
{
    float bx_mg;
    float by_mg;
    float bz_mg;
} impact_module_baseline_t;

/* 诊断信息单独抽出来，便于宿主在失败路径和成功路径都能读取算法状态。 */
typedef struct
{
    impact_disp_status_t status;
    impact_disp_state_t state;
    uint32_t quality_flags;
    uint16_t release_count;
    uint16_t release_count_min;
    uint16_t max_dt_ms;
    uint16_t reserved0;
} impact_module_diag_t;

/* 结束事件时一次返回主结果和诊断，减少宿主二次查询。 */
typedef struct
{
    impact_disp_status_t status;
    impact_disp_result_t result;
    impact_module_diag_t diag;
} impact_module_end_response_t;

#endif /* IMPACT_MODULE_API_H */
