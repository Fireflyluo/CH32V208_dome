#ifndef MODULE_LOADER_H
#define MODULE_LOADER_H

#include "module_abi.h"
#include "module_image.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    MODULE_LOADER_OK = 0,
    MODULE_LOADER_INVALID_ARGUMENT = 1,
    MODULE_LOADER_INVALID_SOURCE = 2,
    MODULE_LOADER_OUT_OF_RANGE = 3,
    MODULE_LOADER_VERIFY_FAILED = 4,
    MODULE_LOADER_INVALID_EXPORTS = 5,
    MODULE_LOADER_INVALID_IMAGE = 6,
} module_loader_status_t;

typedef struct
{
    uint8_t *ram_base;
    uint32_t ram_size;
    const uint8_t *flash_base;
    uint32_t flash_size;
} module_loader_layout_t;

const module_loader_layout_t *module_loader_get_layout(void);
uint8_t *module_loader_slot_base(void);
uint32_t module_loader_slot_capacity(void);
uintptr_t module_loader_read_global_pointer(void);
void module_loader_write_global_pointer(uintptr_t value);
bool module_loader_flash_contains(const void *flash_src, uint32_t size);
module_loader_status_t module_loader_validate_image(const void *flash_src, uint32_t image_size, const module_image_header_t **out_header);
module_loader_status_t module_loader_copy_from_flash(const void *flash_src, uint32_t size, uint32_t slot_offset);
module_loader_status_t module_loader_resolve_exports(const module_exports_t **out_exports);

#endif /* MODULE_LOADER_H */
