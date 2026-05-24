#include "module_manager.h"

#include "module_loader.h"

#include <string.h>

static module_runtime_t *s_active_runtime = NULL;

/*
 * 模块在自己的 gp 下运行，但它回调宿主时，宿主函数必须恢复到宿主 gp。
 * 否则像日志打印、GPIO 控制这类普通 C 函数一旦访问自身全局/常量，就可能直接跑飞。
 */
static void module_manager_bridge_set_output(void *user, uint32_t output_id, uint32_t value)
{
    module_runtime_t *runtime = (module_runtime_t *)user;
    uintptr_t module_gp = 0u;

    if (runtime == NULL || runtime->host_impl.set_output == NULL)
    {
        return;
    }

    module_gp = module_loader_read_global_pointer();
    module_loader_write_global_pointer(runtime->host_gp);
    runtime->host_impl.set_output(runtime->host_impl.user, output_id, value);
    module_loader_write_global_pointer(module_gp);
}

static void module_manager_bridge_log_text(void *user, const char *msg)
{
    module_runtime_t *runtime = (module_runtime_t *)user;
    uintptr_t module_gp = 0u;

    if (runtime == NULL || runtime->host_impl.log_text == NULL)
    {
        return;
    }

    module_gp = module_loader_read_global_pointer();
    module_loader_write_global_pointer(runtime->host_gp);
    runtime->host_impl.log_text(runtime->host_impl.user, msg);
    module_loader_write_global_pointer(module_gp);
}

static bool module_runtime_is_bound(const module_runtime_t *runtime)
{
    return runtime != NULL && runtime->exports != NULL;
}

static void module_runtime_clear_loaded_state(module_runtime_t *runtime)
{
    if (runtime == NULL)
    {
        return;
    }

    runtime->owner = NULL;
    runtime->program_name = NULL;
    runtime->exports = NULL;
    runtime->host_gp = 0u;
    memset(&runtime->ctx, 0, sizeof(runtime->ctx));
}

static module_manager_status_t module_manager_load_internal(module_runtime_t *runtime,
                                                            const char *owner,
                                                            const char *program_name,
                                                            const void *blob,
                                                            uint32_t size,
                                                            bool allow_takeover)
{
    const module_exports_t *exports = NULL;
    module_loader_status_t load_status;

    if (runtime == NULL || owner == NULL || program_name == NULL || blob == NULL || size == 0u)
    {
        return MODULE_MANAGER_INVALID_ARGUMENT;
    }

    if (module_manager_runtime_is_active(runtime))
    {
        if (runtime->program_name != NULL && strcmp(runtime->program_name, program_name) == 0)
        {
            return MODULE_MANAGER_OK;
        }

        (void)module_manager_unload(runtime);
    }
    else if (s_active_runtime != NULL)
    {
        if (!allow_takeover)
        {
            return MODULE_MANAGER_BUSY;
        }
        /* 显式 takeover 才允许把当前槽位所有者卸载掉。 */
        (void)module_manager_unload(s_active_runtime);
    }

    load_status = module_loader_copy_from_flash(blob, size, 0u);
    if (load_status != MODULE_LOADER_OK)
    {
        return MODULE_MANAGER_LOAD_FAILED;
    }

    load_status = module_loader_resolve_exports(&exports);
    if (load_status != MODULE_LOADER_OK || exports == NULL)
    {
        return MODULE_MANAGER_INVALID_EXPORTS;
    }

    if (exports->ctx_size > sizeof(runtime->ctx))
    {
        return MODULE_MANAGER_CONTEXT_TOO_SMALL;
    }

    memset(&runtime->ctx, 0, sizeof(runtime->ctx));
    runtime->owner = owner;
    runtime->program_name = program_name;
    runtime->exports = exports;
    s_active_runtime = runtime;
    return MODULE_MANAGER_OK;
}

static void module_manager_call_with_gp(module_runtime_t *runtime,
                                        void (*fn)(module_ctx_t *ctx, const module_host_api_t *host))
{
    uintptr_t host_gp = 0u;
    uintptr_t module_gp = 0u;

    if (runtime == NULL || runtime->exports == NULL || fn == NULL)
    {
        return;
    }

    host_gp = module_loader_read_global_pointer();
    runtime->host_gp = host_gp;
    module_gp = runtime->exports->global_pointer;
    if (module_gp != 0u)
    {
        module_loader_write_global_pointer(module_gp);
    }

    fn(&runtime->ctx, &runtime->host);
    module_loader_write_global_pointer(host_gp);
}

void module_runtime_setup(module_runtime_t *runtime, const module_host_api_t *host)
{
    if (runtime == NULL)
    {
        return;
    }

    module_runtime_clear_loaded_state(runtime);
    memset(&runtime->host, 0, sizeof(runtime->host));
    memset(&runtime->host_impl, 0, sizeof(runtime->host_impl));
    if (host != NULL)
    {
        runtime->host_impl = *host;
    }

    runtime->host.user = runtime;
    runtime->host.set_output = module_manager_bridge_set_output;
    runtime->host.log_text = module_manager_bridge_log_text;
}

bool module_manager_runtime_is_active(const module_runtime_t *runtime)
{
    return runtime != NULL && s_active_runtime == runtime && module_runtime_is_bound(runtime);
}

const char *module_manager_active_owner(void)
{
    if (s_active_runtime == NULL)
    {
        return NULL;
    }
    return s_active_runtime->owner;
}

const char *module_manager_active_program(void)
{
    if (s_active_runtime == NULL)
    {
        return NULL;
    }
    return s_active_runtime->program_name;
}

module_manager_status_t module_manager_load(module_runtime_t *runtime,
                                            const char *owner,
                                            const char *program_name,
                                            const void *blob,
                                            uint32_t size)
{
    return module_manager_load_internal(runtime, owner, program_name, blob, size, false);
}

module_manager_status_t module_manager_takeover(module_runtime_t *runtime,
                                                const char *owner,
                                                const char *program_name,
                                                const void *blob,
                                                uint32_t size)
{
    return module_manager_load_internal(runtime, owner, program_name, blob, size, true);
}

module_manager_status_t module_manager_call_init(module_runtime_t *runtime)
{
    if (!module_manager_runtime_is_active(runtime))
    {
        return MODULE_MANAGER_NOT_ACTIVE;
    }

    if (runtime->exports->init != NULL)
    {
        module_manager_call_with_gp(runtime, runtime->exports->init);
    }
    return MODULE_MANAGER_OK;
}

module_manager_status_t module_manager_call_tick(module_runtime_t *runtime, uint32_t tick)
{
    uintptr_t host_gp = 0u;
    uintptr_t module_gp = 0u;

    if (!module_manager_runtime_is_active(runtime))
    {
        return MODULE_MANAGER_NOT_ACTIVE;
    }

    if (runtime->exports->tick == NULL)
    {
        return MODULE_MANAGER_OK;
    }

    host_gp = module_loader_read_global_pointer();
    runtime->host_gp = host_gp;
    module_gp = runtime->exports->global_pointer;
    if (module_gp != 0u)
    {
        module_loader_write_global_pointer(module_gp);
    }

    runtime->exports->tick(&runtime->ctx, &runtime->host, tick);
    module_loader_write_global_pointer(host_gp);
    return MODULE_MANAGER_OK;
}

int32_t module_manager_call_command(module_runtime_t *runtime,
                                    int32_t unavailable_status,
                                    uint32_t command,
                                    const void *input,
                                    void *output)
{
    uintptr_t host_gp = 0u;
    uintptr_t module_gp = 0u;
    int32_t (*call_fn)(module_ctx_t *ctx, const module_host_api_t *host, uint32_t command, const void *input, void *output) = NULL;

    if (!module_manager_runtime_is_active(runtime) || runtime->exports->call == NULL)
    {
        return unavailable_status;
    }

    host_gp = module_loader_read_global_pointer();
    runtime->host_gp = host_gp;
    module_gp = runtime->exports->global_pointer;
    call_fn = runtime->exports->call;
    if (module_gp != 0u)
    {
        module_loader_write_global_pointer(module_gp);
    }

    unavailable_status = call_fn(&runtime->ctx, &runtime->host, command, input, output);
    module_loader_write_global_pointer(host_gp);
    return unavailable_status;
}

module_manager_status_t module_manager_unload(module_runtime_t *runtime)
{
    if (runtime == NULL)
    {
        return MODULE_MANAGER_INVALID_ARGUMENT;
    }

    if (module_manager_runtime_is_active(runtime) && runtime->exports->deinit != NULL)
    {
        module_manager_call_with_gp(runtime, runtime->exports->deinit);
    }

    if (s_active_runtime == runtime)
    {
        s_active_runtime = NULL;
    }
    module_runtime_clear_loaded_state(runtime);
    return MODULE_MANAGER_OK;
}
