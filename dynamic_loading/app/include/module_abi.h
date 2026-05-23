#ifndef MODULE_ABI_H
#define MODULE_ABI_H

#include <stdint.h>

#define MODULE_ABI_MAGIC 0x4D4F4432u
#define MODULE_ABI_VERSION 1u
#define MODULE_CTX_WORDS 16u

typedef struct
{
    void (*led_write)(uint8_t on);
    void (*log)(const char *msg);
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
    uint32_t ctx_size;
    void (*init)(module_ctx_t *ctx, const module_host_api_t *host);
    void (*tick)(module_ctx_t *ctx, const module_host_api_t *host, uint32_t tick);
    void (*deinit)(module_ctx_t *ctx, const module_host_api_t *host);
} module_exports_t;

typedef const module_exports_t *(*module_get_exports_fn)(void);

#endif /* MODULE_ABI_H */
