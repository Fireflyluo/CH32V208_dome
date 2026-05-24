#ifndef MODULE_IMAGE_H
#define MODULE_IMAGE_H

#include <stdint.h>

#define MODULE_IMAGE_MAGIC 0x474D494Du
#define MODULE_IMAGE_VERSION 1u

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t header_size;
    uint32_t payload_size;
    uint32_t payload_crc32;
    uint32_t load_offset;
} module_image_header_t;

#endif /* MODULE_IMAGE_H */
