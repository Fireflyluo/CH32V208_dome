#include "module_loader.h"

#include <string.h>

extern uint8_t __module_ram_start__[];
extern uint8_t __module_ram_end__[];
extern uint8_t __module_flash_start__[];
extern uint8_t __module_flash_end__[];

/* 轻量实现即可满足镜像完整性校验，不额外依赖平台 CRC 外设。 */
static uint32_t module_loader_crc32(const uint8_t *data, uint32_t size)
{
    uint32_t crc = 0xFFFFFFFFu;
    uint32_t i = 0u;

    while (i < size)
    {
        uint32_t byte = data[i++];
        uint32_t bit = 0u;

        crc ^= byte;
        while (bit < 8u)
        {
            uint32_t mask = 0u - (crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
            bit++;
        }
    }

    return ~crc;
}

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
    /* 刚把新指令写进 RAM，后续取指前必须做 fence.i。 */
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

uintptr_t module_loader_read_global_pointer(void)
{
    uintptr_t value = 0u;

    /* 供宿主在进入模块前保存当前 gp。 */
    __asm volatile("mv %0, gp" : "=r"(value));
    return value;
}

void module_loader_write_global_pointer(uintptr_t value)
{
    /* 供宿主在进入模块前切到模块 gp，退出后再恢复。 */
    __asm volatile("mv gp, %0" : : "r"(value));
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

module_loader_status_t module_loader_validate_image(const void *flash_src, uint32_t image_size, const module_image_header_t **out_header)
{
    const module_image_header_t *header = (const module_image_header_t *)flash_src;
    const uint8_t *payload = NULL;
    uint32_t computed_crc = 0u;

    if (out_header == NULL)
    {
        return MODULE_LOADER_INVALID_ARGUMENT;
    }

    *out_header = NULL;
    if (flash_src == NULL || image_size < sizeof(module_image_header_t))
    {
        return MODULE_LOADER_INVALID_ARGUMENT;
    }

    if (!module_loader_flash_contains(flash_src, image_size))
    {
        return MODULE_LOADER_INVALID_SOURCE;
    }

    /* 头部字段先过滤掉明显错误或非本协议格式的镜像。 */
    if (header->magic != MODULE_IMAGE_MAGIC || header->version != MODULE_IMAGE_VERSION)
    {
        return MODULE_LOADER_INVALID_IMAGE;
    }

    if (header->header_size != sizeof(module_image_header_t) || header->payload_size == 0u)
    {
        return MODULE_LOADER_INVALID_IMAGE;
    }

    if (header->load_offset != 0u)
    {
        return MODULE_LOADER_INVALID_IMAGE;
    }

    if (header->header_size > image_size || header->payload_size > (image_size - header->header_size))
    {
        return MODULE_LOADER_INVALID_IMAGE;
    }

    payload = ((const uint8_t *)flash_src) + header->header_size;
    computed_crc = module_loader_crc32(payload, header->payload_size);
    if (computed_crc != header->payload_crc32)
    {
        /* 当前是完整性校验，不是签名认证。 */
        return MODULE_LOADER_VERIFY_FAILED;
    }

    *out_header = header;
    return MODULE_LOADER_OK;
}

module_loader_status_t module_loader_copy_from_flash(const void *flash_src, uint32_t size, uint32_t slot_offset)
{
    uint32_t slot_capacity = module_loader_ram_size();
    uint8_t *dst = NULL;
    const module_image_header_t *header = NULL;
    const uint8_t *payload = NULL;
    module_loader_status_t status;

    if (flash_src == NULL || size == 0U)
    {
        return MODULE_LOADER_INVALID_ARGUMENT;
    }

    status = module_loader_validate_image(flash_src, size, &header);
    if (status != MODULE_LOADER_OK || header == NULL)
    {
        return status;
    }

    payload = ((const uint8_t *)flash_src) + header->header_size;

    /* 单槽位模型下，payload 必须完整落进 RAM_MODULE 的预留区域。 */
    if (slot_offset > slot_capacity || header->payload_size > (slot_capacity - slot_offset))
    {
        return MODULE_LOADER_OUT_OF_RANGE;
    }

    dst = __module_ram_start__ + slot_offset;
    memcpy(dst, payload, header->payload_size);
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

    /*
     * 模块被固定链接到 RAM 槽位起点，因此这里可以直接把槽位基址
     * 当成 module_get_exports() 入口来调用。
     */
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
