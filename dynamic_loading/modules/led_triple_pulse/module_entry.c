#include "module_abi.h"

extern char __global_pointer$[];

static void module_init(module_ctx_t *ctx, const module_host_api_t *host)
{
    ctx->words[0] = 0u;
    if (host != 0 && host->log_text != 0)
    {
        host->log_text(host->user, "triple_pulse init");
    }
}

static void module_tick(module_ctx_t *ctx, const module_host_api_t *host, uint32_t tick)
{
    uint32_t phase = tick % 12U;
    uint32_t level = (phase == 0U || phase == 2U || phase == 4U) ? 1U : 0U;

    ctx->words[0] = phase;
    ctx->words[1] = level;
    if (host != 0 && host->set_output != 0)
    {
        host->set_output(host->user, MODULE_HOST_OUTPUT_STATUS_LED, level);
    }
}

static void module_deinit(module_ctx_t *ctx, const module_host_api_t *host)
{
    ctx->words[0] = 0u;
    ctx->words[1] = 0u;
    if (host != 0 && host->set_output != 0)
    {
        host->set_output(host->user, MODULE_HOST_OUTPUT_STATUS_LED, 0u);
    }
    if (host != 0 && host->log_text != 0)
    {
        host->log_text(host->user, "triple_pulse deinit");
    }
}

static const module_exports_t g_module_exports = {
    .magic = MODULE_ABI_MAGIC,
    .abi_version = MODULE_ABI_VERSION,
    .name = "triple_pulse",
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
