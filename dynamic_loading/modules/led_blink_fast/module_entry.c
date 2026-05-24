#include "module_abi.h"

extern char __global_pointer$[];

static void module_init(module_ctx_t *ctx, const module_host_api_t *host)
{
    ctx->words[0] = 0u;
    if (host != 0 && host->log_text != 0)
    {
        host->log_text(host->user, "blink_fast init");
    }
}

static void module_tick(module_ctx_t *ctx, const module_host_api_t *host, uint32_t tick)
{
    uint32_t level = tick & 1U;

    ctx->words[0] = level;
    if (host != 0 && host->set_output != 0)
    {
        host->set_output(host->user, MODULE_HOST_OUTPUT_STATUS_LED, level);
    }
}

static void module_deinit(module_ctx_t *ctx, const module_host_api_t *host)
{
    ctx->words[0] = 0u;
    if (host != 0 && host->set_output != 0)
    {
        host->set_output(host->user, MODULE_HOST_OUTPUT_STATUS_LED, 0u);
    }
    if (host != 0 && host->log_text != 0)
    {
        host->log_text(host->user, "blink_fast deinit");
    }
}

static const module_exports_t g_module_exports = {
    .magic = MODULE_ABI_MAGIC,
    .abi_version = MODULE_ABI_VERSION,
    .name = "blink_fast",
    .global_pointer = (uintptr_t)__global_pointer$,
    .ctx_size = sizeof(module_ctx_t),
    .init = module_init,
    .tick = module_tick,
    .call = 0,
    .deinit = module_deinit,
};

__attribute__((section(".text.module_get_exports")))
const module_exports_t *module_get_exports(void)
{
    return &g_module_exports;
}
