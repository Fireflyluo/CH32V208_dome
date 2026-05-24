#ifndef MODULE_ABI_H
#define MODULE_ABI_H

#include <stdint.h>

#define MODULE_ABI_MAGIC 0x4D4F4432u
#define MODULE_ABI_VERSION 3u
#define MODULE_CTX_WORDS 64u

typedef enum
{
    MODULE_HOST_OUTPUT_STATUS_LED = 1u,
} module_host_output_id_t;

typedef struct
{
    void *user;
    void (*set_output)(void *user, uint32_t output_id, uint32_t value);
    void (*log_text)(void *user, const char *msg);
} module_host_api_t;

typedef struct
{
    uint32_t words[MODULE_CTX_WORDS];
} module_ctx_t;

typedef struct module_exports
{
    uint32_t magic;
    uint32_t abi_version;
    const char *name;
    uintptr_t global_pointer;
    uint32_t ctx_size;
    void (*init)(module_ctx_t *ctx, const module_host_api_t *host);
    void (*tick)(module_ctx_t *ctx, const module_host_api_t *host, uint32_t tick);
    int32_t (*call)(module_ctx_t *ctx, const module_host_api_t *host, uint32_t command, const void *input, void *output);
    void (*deinit)(module_ctx_t *ctx, const module_host_api_t *host);
} module_exports_t;

typedef const module_exports_t *(*module_get_exports_fn)(void);

#endif /* MODULE_ABI_H */
