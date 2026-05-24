#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include "module_abi.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    MODULE_MANAGER_OK = 0,
    MODULE_MANAGER_INVALID_ARGUMENT = 1,
    MODULE_MANAGER_BUSY = 2,
    MODULE_MANAGER_LOAD_FAILED = 3,
    MODULE_MANAGER_INVALID_EXPORTS = 4,
    MODULE_MANAGER_CONTEXT_TOO_SMALL = 5,
    MODULE_MANAGER_NOT_ACTIVE = 6,
} module_manager_status_t;

/*
 * 每个使用者各自维护一个 runtime，里面保存：
 * - 自己的宿主回调表
 * - 模块上下文缓冲区
 * - 当前已经解析出的导出表
 *
 * 真正的 RAM_MODULE 槽位仍然只有一个，module_manager 只负责协调谁在占用它。
 */
typedef struct
{
    const char *owner;
    const char *program_name;
    module_ctx_t ctx;
    module_host_api_t host;
    module_host_api_t host_impl;
    uintptr_t host_gp;
    const module_exports_t *exports;
} module_runtime_t;

void module_runtime_setup(module_runtime_t *runtime, const module_host_api_t *host);
bool module_manager_runtime_is_active(const module_runtime_t *runtime);
const char *module_manager_active_owner(void);
const char *module_manager_active_program(void);
module_manager_status_t module_manager_load(module_runtime_t *runtime,
                                            const char *owner,
                                            const char *program_name,
                                            const void *blob,
                                            uint32_t size);
module_manager_status_t module_manager_takeover(module_runtime_t *runtime,
                                                const char *owner,
                                                const char *program_name,
                                                const void *blob,
                                                uint32_t size);
module_manager_status_t module_manager_call_init(module_runtime_t *runtime);
module_manager_status_t module_manager_call_tick(module_runtime_t *runtime, uint32_t tick);
int32_t module_manager_call_command(module_runtime_t *runtime,
                                    int32_t unavailable_status,
                                    uint32_t command,
                                    const void *input,
                                    void *output);
module_manager_status_t module_manager_unload(module_runtime_t *runtime);

#endif /* MODULE_MANAGER_H */
