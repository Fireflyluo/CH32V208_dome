#include "module_manager_selftest.h"

#include "impact_module_api.h"
#include "impact_module_programs.h"
#include "led_module_programs.h"
#include "log_print.h"
#include "module_manager.h"

#include <string.h>

static void selftest_set_output(void *user, uint32_t output_id, uint32_t value)
{
    (void)user;
    (void)output_id;
    (void)value;
}

static void selftest_log_text(void *user, const char *msg)
{
    const char *owner = (const char *)user;

    if (msg != NULL)
    {
        LOG_PRINT("module selftest [%s]: %s\r\n", owner != NULL ? owner : "unknown", msg);
    }
}

void module_manager_selftest_run(void)
{
    module_runtime_t led_runtime;
    module_runtime_t impact_runtime;
    module_host_api_t led_host;
    module_host_api_t impact_host;
    module_manager_status_t status;
    const char *version = NULL;
    int32_t call_status;

    if (g_led_module_program_count == 0u || g_impact_module_program_count == 0u)
    {
        LOG_PRINT("module selftest: skipped (missing programs)\r\n");
        return;
    }

    memset(&led_host, 0, sizeof(led_host));
    led_host.user = (void *)"led";
    led_host.set_output = selftest_set_output;
    led_host.log_text = selftest_log_text;
    module_runtime_setup(&led_runtime, &led_host);

    memset(&impact_host, 0, sizeof(impact_host));
    impact_host.user = (void *)"impact";
    impact_host.log_text = selftest_log_text;
    module_runtime_setup(&impact_runtime, &impact_host);

    /* 第一步：正常装入一个 LED 模块，占住唯一 RAM 槽位。 */
    status = module_manager_load(&led_runtime,
                                 "selftest-led",
                                 g_led_module_programs[0].name,
                                 g_led_module_programs[0].blob,
                                 g_led_module_programs[0].size);
    LOG_PRINT("module selftest: load led status=%u owner=%s program=%s\r\n",
              (unsigned)status,
              module_manager_active_owner() != NULL ? module_manager_active_owner() : "none",
              module_manager_active_program() != NULL ? module_manager_active_program() : "none");
    if (status != MODULE_MANAGER_OK)
    {
        (void)module_manager_unload(&led_runtime);
        return;
    }

    status = module_manager_call_init(&led_runtime);
    LOG_PRINT("module selftest: led init status=%u\r\n", (unsigned)status);

    /* 第二步：不带 takeover 直接装 impact，预期得到 BUSY。 */
    status = module_manager_load(&impact_runtime,
                                 "selftest-impact",
                                 g_impact_module_programs[0].name,
                                 g_impact_module_programs[0].blob,
                                 g_impact_module_programs[0].size);
    LOG_PRINT("module selftest: impact load without takeover status=%u\r\n", (unsigned)status);

    /* 第三步：显式接管槽位，验证 LED 会先被 deinit，再切到 impact。 */
    status = module_manager_takeover(&impact_runtime,
                                     "selftest-impact",
                                     g_impact_module_programs[0].name,
                                     g_impact_module_programs[0].blob,
                                     g_impact_module_programs[0].size);
    LOG_PRINT("module selftest: takeover impact status=%u owner=%s program=%s\r\n",
              (unsigned)status,
              module_manager_active_owner() != NULL ? module_manager_active_owner() : "none",
              module_manager_active_program() != NULL ? module_manager_active_program() : "none");
    if (status == MODULE_MANAGER_OK)
    {
        status = module_manager_call_init(&impact_runtime);
        call_status = module_manager_call_command(&impact_runtime,
                                                  -1,
                                                  IMPACT_MODULE_CMD_GET_VERSION,
                                                  NULL,
                                                  &version);
        LOG_PRINT("module selftest: impact init status=%u version=%s call=%ld\r\n",
                  (unsigned)status,
                  version != NULL ? version : "unknown",
                  (long)call_status);
    }

    (void)module_manager_unload(&impact_runtime);
    (void)module_manager_unload(&led_runtime);
    LOG_PRINT("module selftest: done\r\n");
}
