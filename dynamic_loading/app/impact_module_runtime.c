#include "impact_module_runtime.h"

#include "impact_module_programs.h"
#include "log_print.h"
#include "module_loader.h"
#include "module_manager.h"

#include <string.h>

/* impact runtime 只保留自己的宿主视角状态；单槽位协调由 module_manager 负责。 */
static module_runtime_t s_impact_runtime;

static int32_t impact_call(uint32_t command, const void *input, void *output)
{
    return module_manager_call_command(&s_impact_runtime,
                                       (int32_t)IMPACT_DISP_ERR_STATE,
                                       command,
                                       input,
                                       output);
}

bool impact_module_runtime_init(void)
{
    const impact_module_program_desc_t *program = NULL;
    module_host_api_t host_api;
    module_manager_status_t status;
    const char *version = NULL;
    int32_t call_status;

    if (module_manager_runtime_is_active(&s_impact_runtime))
    {
        return true;
    }

    if (g_impact_module_program_count == 0u)
    {
        LOG_PRINT("impact module: no program available\r\n");
        return false;
    }

    program = &g_impact_module_programs[0];
    memset(&host_api, 0, sizeof(host_api));
    module_runtime_setup(&s_impact_runtime, &host_api);

    status = module_manager_load(&s_impact_runtime, "impact", program->name, program->blob, program->size);
    if (status == MODULE_MANAGER_BUSY)
    {
        LOG_PRINT("impact module: slot busy owner=%s program=%s\r\n",
                  module_manager_active_owner() != NULL ? module_manager_active_owner() : "unknown",
                  module_manager_active_program() != NULL ? module_manager_active_program() : "unknown");
        return false;
    }
    if (status != MODULE_MANAGER_OK)
    {
        LOG_PRINT("impact module: load failed status=%u\r\n", (unsigned)status);
        return false;
    }

    status = module_manager_call_init(&s_impact_runtime);
    if (status != MODULE_MANAGER_OK)
    {
        LOG_PRINT("impact module: init failed status=%u\r\n", (unsigned)status);
        (void)module_manager_unload(&s_impact_runtime);
        return false;
    }

    /* 装载完成后做一次轻量命令调用，确认 call 链路和 gp 切换都正常。 */
    call_status = impact_call(IMPACT_MODULE_CMD_GET_VERSION, NULL, &version);
    LOG_PRINT("impact module: loaded name=%s size=%lu slot=%p version=%s call=%ld\r\n",
              s_impact_runtime.exports != NULL && s_impact_runtime.exports->name != NULL ? s_impact_runtime.exports->name : program->name,
              (unsigned long)program->size,
              (void *)module_loader_slot_base(),
              (version != NULL) ? version : "unknown",
              (long)call_status);
    return true;
}

bool impact_module_runtime_ready(void)
{
    return module_manager_runtime_is_active(&s_impact_runtime);
}

int32_t impact_module_runtime_get_default_cfg(impact_disp_cfg_t *cfg)
{
    if (cfg == NULL)
    {
        return (int32_t)IMPACT_DISP_ERR_ARG;
    }
    if (!impact_module_runtime_init())
    {
        return (int32_t)IMPACT_DISP_ERR_STATE;
    }
    return impact_call(IMPACT_MODULE_CMD_GET_DEFAULT_CFG, NULL, cfg);
}

int32_t impact_module_runtime_configure(const impact_disp_cfg_t *cfg)
{
    if (cfg == NULL)
    {
        return (int32_t)IMPACT_DISP_ERR_ARG;
    }
    if (!impact_module_runtime_init())
    {
        return (int32_t)IMPACT_DISP_ERR_STATE;
    }
    return impact_call(IMPACT_MODULE_CMD_CONFIGURE, cfg, NULL);
}

int32_t impact_module_runtime_reset(void)
{
    if (!impact_module_runtime_init())
    {
        return (int32_t)IMPACT_DISP_ERR_STATE;
    }
    return impact_call(IMPACT_MODULE_CMD_RESET, NULL, NULL);
}

int32_t impact_module_runtime_begin_event(uint32_t event_id)
{
    if (!impact_module_runtime_init())
    {
        return (int32_t)IMPACT_DISP_ERR_STATE;
    }
    return impact_call(IMPACT_MODULE_CMD_BEGIN_EVENT, &event_id, NULL);
}

int32_t impact_module_runtime_set_baseline(float bx_mg, float by_mg, float bz_mg)
{
    impact_module_baseline_t baseline;

    if (!impact_module_runtime_init())
    {
        return (int32_t)IMPACT_DISP_ERR_STATE;
    }

    baseline.bx_mg = bx_mg;
    baseline.by_mg = by_mg;
    baseline.bz_mg = bz_mg;
    return impact_call(IMPACT_MODULE_CMD_SET_BASELINE, &baseline, NULL);
}

int32_t impact_module_runtime_feed_sample(const impact_disp_sample_t *sample)
{
    if (sample == NULL)
    {
        return (int32_t)IMPACT_DISP_ERR_ARG;
    }
    if (!impact_module_runtime_init())
    {
        return (int32_t)IMPACT_DISP_ERR_STATE;
    }
    return impact_call(IMPACT_MODULE_CMD_FEED_SAMPLE, sample, NULL);
}

int32_t impact_module_runtime_end_event(impact_module_end_response_t *response)
{
    if (response == NULL)
    {
        return (int32_t)IMPACT_DISP_ERR_ARG;
    }
    if (!impact_module_runtime_init())
    {
        return (int32_t)IMPACT_DISP_ERR_STATE;
    }
    memset(response, 0, sizeof(*response));
    return impact_call(IMPACT_MODULE_CMD_END_EVENT, NULL, response);
}

int32_t impact_module_runtime_get_diag(impact_module_diag_t *diag)
{
    if (diag == NULL)
    {
        return (int32_t)IMPACT_DISP_ERR_ARG;
    }
    if (!impact_module_runtime_init())
    {
        return (int32_t)IMPACT_DISP_ERR_STATE;
    }
    memset(diag, 0, sizeof(*diag));
    return impact_call(IMPACT_MODULE_CMD_GET_DIAG, NULL, diag);
}
