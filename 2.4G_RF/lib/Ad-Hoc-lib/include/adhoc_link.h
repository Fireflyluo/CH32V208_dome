#ifndef ADHOC_LINK_H
#define ADHOC_LINK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int (*init)(void *ctx);
    int (*start_rx)(void *ctx);
    int (*tx)(void *ctx, const uint8_t *buf, uint16_t len);
    int (*poll_rx)(void *ctx, uint8_t *buf, uint16_t *len, int8_t *rssi);
    uint32_t (*now_us)(void *ctx);
    uint16_t (*rand_u16)(void *ctx);
} adhoc_link_ops_t;

#ifdef __cplusplus
}
#endif

#endif /* ADHOC_LINK_H */
