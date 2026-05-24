#ifndef IMPACT_MODULE_PROGRAMS_H
#define IMPACT_MODULE_PROGRAMS_H

#include <stdint.h>

typedef struct
{
    const char *name;
    const uint8_t *blob;
    uint32_t size;
} impact_module_program_desc_t;

extern const impact_module_program_desc_t g_impact_module_programs[];
extern const uint32_t g_impact_module_program_count;

#endif /* IMPACT_MODULE_PROGRAMS_H */
