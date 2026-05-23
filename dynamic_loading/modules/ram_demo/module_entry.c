#include <stdint.h>

__attribute__((section(".text.module_entry")))
uint32_t module_entry(uint32_t seed)
{
    return seed + 7U;
}
