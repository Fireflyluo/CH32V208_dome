#ifndef LED_MODULE_PROGRAMS_H
#define LED_MODULE_PROGRAMS_H

#include <stdint.h>

typedef struct
{
    const char *name;
    const uint8_t *blob;
    uint32_t size;
} led_module_program_desc_t;

extern const led_module_program_desc_t g_led_module_programs[];
extern const uint32_t g_led_module_program_count;

#endif /* LED_MODULE_PROGRAMS_H */
