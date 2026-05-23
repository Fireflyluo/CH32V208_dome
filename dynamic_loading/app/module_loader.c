#include "module_loader.h"

#include <string.h>

extern uint8_t __module_ram_start__[];
extern uint8_t __module_ram_end__[];
extern uint8_t __module_flash_start__[];
extern uint8_t __module_flash_end__[];

static uint32_t module_loader_ram_size(void)
{
    return (uint32_t)((uintptr_t)__module_ram_end__ - (uintptr_t)__module_ram_start__);
}

static uint32_t module_loader_flash_size(void)
{
    return (uint32_t)((uintptr_t)__module_flash_end__ - (uintptr_t)__module_flash_start__);
}

static void module_loader_sync_icache(void)
{
    __asm volatile("fence.i");
}

const module_loader_layout_t *module_loader_get_layout(void)
{
    static module_loader_layout_t layout;

    layout.ram_base = __module_ram_start__;
    layout.ram_size = module_loader_ram_size();
    layout.flash_base = __module_flash_start__;
    layout.flash_size = module_loader_flash_size();
    return &layout;
}

uint8_t *module_loader_slot_base(void)
{
    return __module_ram_start__;
}

uint32_t module_loader_slot_capacity(void)
{
    return module_loader_ram_size();
}

bool module_loader_flash_contains(const void *flash_src, uint32_t size)
{
    uintptr_t flash_begin = (uintptr_t)__module_flash_start__;
    uintptr_t flash_end = flash_begin + module_loader_flash_size();
    uintptr_t req_begin = (uintptr_t)flash_src;
    uintptr_t req_end = req_begin + size;

    if (flash_src == NULL || size == 0U || req_end < req_begin)
    {
        return false;
    }

    return req_begin >= flash_begin && req_end <= flash_end;
}

module_loader_status_t module_loader_copy_from_flash(const void *flash_src, uint32_t size, uint32_t slot_offset)
{
    uint32_t slot_capacity = module_loader_ram_size();
    uint8_t *dst = NULL;

    if (flash_src == NULL || size == 0U)
    {
        return MODULE_LOADER_INVALID_ARGUMENT;
    }

    if (!module_loader_flash_contains(flash_src, size))
    {
        return MODULE_LOADER_INVALID_SOURCE;
    }

    if (slot_offset > slot_capacity || size > (slot_capacity - slot_offset))
    {
        return MODULE_LOADER_OUT_OF_RANGE;
    }

    dst = __module_ram_start__ + slot_offset;
    memcpy(dst, flash_src, size);
    module_loader_sync_icache();
    return MODULE_LOADER_OK;
}

module_loader_status_t module_loader_resolve_exports(const module_exports_t **out_exports)
{
    module_get_exports_fn get_exports = NULL;
    const module_exports_t *exports = NULL;

    if (out_exports == NULL)
    {
        return MODULE_LOADER_INVALID_ARGUMENT;
    }

    get_exports = (module_get_exports_fn)(uintptr_t)__module_ram_start__;
    exports = get_exports();
    if (exports == NULL)
    {
        return MODULE_LOADER_INVALID_EXPORTS;
    }

    if (exports->magic != MODULE_ABI_MAGIC || exports->abi_version != MODULE_ABI_VERSION)
    {
        return MODULE_LOADER_INVALID_EXPORTS;
    }

    *out_exports = exports;
    return MODULE_LOADER_OK;
}
