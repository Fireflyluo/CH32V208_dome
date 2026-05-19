#ifndef ADHOC_VIRTUAL_TIME_H
#define ADHOC_VIRTUAL_TIME_H

#include <stdint.h>
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    LARGE_INTEGER freq;
} adhoc_virtual_time_t;

static inline void adhoc_virtual_time_init(adhoc_virtual_time_t *vt)
{
    QueryPerformanceFrequency(&vt->freq);
}

static inline uint32_t adhoc_virtual_time_now_us(adhoc_virtual_time_t *vt)
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (uint32_t)((now.QuadPart * 1000000ULL) / (unsigned long long)vt->freq.QuadPart);
}

#ifdef __cplusplus
}
#endif

#endif /* ADHOC_VIRTUAL_TIME_H */
