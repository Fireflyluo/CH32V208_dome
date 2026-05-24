#include "impact_module_api.h"
#include "module_abi.h"

extern char __global_pointer$[];

/* 模块私有状态全部收拢到 ctx 对应的内存里，宿主只把它当一块不透明缓冲区。 */
typedef struct
{
    impact_disp_cfg_t cfg;
    impact_disp_ctx_t algo;
    impact_module_diag_t last_diag;
    uint8_t configured;
    uint8_t reserved[3];
} impact_module_state_t;

static impact_module_state_t *impact_state(module_ctx_t *ctx)
{
    return (impact_module_state_t *)(void *)ctx;
}

/* 模块尽量少依赖外部运行时，这里用本地清零避免额外的库依赖。 */
static void impact_zero(void *ptr, uint32_t size)
{
    uint8_t *bytes = (uint8_t *)ptr;
    uint32_t i = 0u;

    for (i = 0u; i < size; ++i)
    {
        bytes[i] = 0u;
    }
}

static void impact_fill_diag(impact_module_diag_t *diag, impact_disp_status_t status, const impact_disp_ctx_t *algo)
{
    if (diag == 0)
    {
        return;
    }

    impact_zero(diag, sizeof(*diag));
    diag->status = status;
    if (algo != 0)
    {
        diag->state = impact_disp_get_state(algo);
        diag->quality_flags = algo->quality_flags;
        diag->release_count = algo->release_count;
        diag->release_count_min = algo->cfg.release_count_min;
        diag->max_dt_ms = algo->cfg.max_dt_ms;
    }
}

/* 除了 get_default_cfg/get_version，其他命令都要求模块先完成 configure。 */
static int32_t impact_require_configured(impact_module_state_t *state)
{
    if (state == 0 || state->configured == 0u)
    {
        return (int32_t)IMPACT_DISP_ERR_STATE;
    }
    return (int32_t)IMPACT_DISP_OK;
}

static void module_init(module_ctx_t *ctx, const module_host_api_t *host)
{
    impact_module_state_t *state = impact_state(ctx);

    impact_zero(state, sizeof(*state));
    if (host != 0 && host->log_text != 0)
    {
        host->log_text(host->user, "impact_displacement init");
    }
}

/*
 * 把算法库原始 API 映射成命令式 ABI。
 * 宿主以后不再直接调用 impact_disp_*，而是统一通过 command/input/output 访问模块。
 */
static int32_t module_call(module_ctx_t *ctx, const module_host_api_t *host, uint32_t command, const void *input, void *output)
{
    impact_module_state_t *state = impact_state(ctx);
    impact_disp_status_t status = IMPACT_DISP_OK;

    (void)host;

    if (state == 0)
    {
        return (int32_t)IMPACT_DISP_ERR_ARG;
    }

    switch (command)
    {
    case IMPACT_MODULE_CMD_GET_VERSION:
        if (output == 0)
        {
            return (int32_t)IMPACT_DISP_ERR_ARG;
        }
        *(const char **)output = impact_disp_get_version();
        return 0;

    case IMPACT_MODULE_CMD_GET_DEFAULT_CFG:
        if (output == 0)
        {
            return (int32_t)IMPACT_DISP_ERR_ARG;
        }
        impact_disp_get_default_cfg((impact_disp_cfg_t *)output);
        return 0;

    case IMPACT_MODULE_CMD_CONFIGURE:
        if (input == 0)
        {
            return (int32_t)IMPACT_DISP_ERR_ARG;
        }
        /* 拷入宿主传来的配置，再初始化模块内部算法上下文。 */
        state->cfg = *(const impact_disp_cfg_t *)input;
        status = impact_disp_init(&state->algo, &state->cfg);
        state->configured = (uint8_t)(status == IMPACT_DISP_OK);
        impact_fill_diag(&state->last_diag, status, &state->algo);
        return (int32_t)status;

    case IMPACT_MODULE_CMD_RESET:
        if (impact_require_configured(state) != IMPACT_DISP_OK)
        {
            return (int32_t)IMPACT_DISP_ERR_STATE;
        }
        status = impact_disp_reset(&state->algo);
        impact_fill_diag(&state->last_diag, status, &state->algo);
        return (int32_t)status;

    case IMPACT_MODULE_CMD_BEGIN_EVENT:
        if (impact_require_configured(state) != IMPACT_DISP_OK || input == 0)
        {
            return (int32_t)IMPACT_DISP_ERR_ARG;
        }
        status = impact_disp_begin_event(&state->algo, *(const uint32_t *)input);
        impact_fill_diag(&state->last_diag, status, &state->algo);
        return (int32_t)status;

    case IMPACT_MODULE_CMD_SET_BASELINE:
        if (impact_require_configured(state) != IMPACT_DISP_OK || input == 0)
        {
            return (int32_t)IMPACT_DISP_ERR_ARG;
        }
        {
            const impact_module_baseline_t *baseline = (const impact_module_baseline_t *)input;
            status = impact_disp_set_baseline_mg(&state->algo, baseline->bx_mg, baseline->by_mg, baseline->bz_mg);
        }
        impact_fill_diag(&state->last_diag, status, &state->algo);
        return (int32_t)status;

    case IMPACT_MODULE_CMD_FEED_SAMPLE:
        if (impact_require_configured(state) != IMPACT_DISP_OK || input == 0)
        {
            return (int32_t)IMPACT_DISP_ERR_ARG;
        }
        status = impact_disp_feed_sample(&state->algo, (const impact_disp_sample_t *)input);
        impact_fill_diag(&state->last_diag, status, &state->algo);
        return (int32_t)status;

    case IMPACT_MODULE_CMD_END_EVENT:
        if (impact_require_configured(state) != IMPACT_DISP_OK || output == 0)
        {
            return (int32_t)IMPACT_DISP_ERR_ARG;
        }
        {
            /* 结束事件时一次性返回结果和诊断，宿主只需要读 response。 */
            impact_module_end_response_t *response = (impact_module_end_response_t *)output;
            impact_zero(response, sizeof(*response));
            status = impact_disp_end_event(&state->algo, &response->result);
            impact_fill_diag(&response->diag, status, &state->algo);
            state->last_diag = response->diag;
            response->status = status;
        }
        return (int32_t)status;

    case IMPACT_MODULE_CMD_GET_DIAG:
        if (output == 0)
        {
            return (int32_t)IMPACT_DISP_ERR_ARG;
        }
        *(impact_module_diag_t *)output = state->last_diag;
        return 0;

    default:
        return (int32_t)IMPACT_DISP_ERR_ARG;
    }
}

static void module_deinit(module_ctx_t *ctx, const module_host_api_t *host)
{
    impact_module_state_t *state = impact_state(ctx);

    impact_zero(state, sizeof(*state));
    if (host != 0 && host->log_text != 0)
    {
        host->log_text(host->user, "impact_displacement deinit");
    }
}

static const module_exports_t g_module_exports = {
    .magic = MODULE_ABI_MAGIC,
    .abi_version = MODULE_ABI_VERSION,
    .name = "impact_displacement",
    /* 复杂模块会访问自己的 .sdata/.data，需要让宿主切到模块 gp。 */
    .global_pointer = (uintptr_t)__global_pointer$,
    /* 宿主据此校验 module_ctx_t 预留空间是否足够容纳真实模块状态。 */
    .ctx_size = sizeof(impact_module_state_t),
    .init = 0,
    .tick = 0,
    .call = module_call,
    .deinit = module_deinit,
};

__attribute__((section(".text.module_get_exports")))
const module_exports_t *module_get_exports(void)
{
    /* RAM 槽位起始地址会先被当成这个函数入口来解析导出表。 */
    return &g_module_exports;
}
