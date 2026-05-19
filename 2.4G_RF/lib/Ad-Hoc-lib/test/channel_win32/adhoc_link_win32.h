#ifndef ADHOC_LINK_WIN32_H
#define ADHOC_LINK_WIN32_H

#include "adhoc_channel.h"
#include "adhoc_virtual_time.h"

#include "adhoc_link.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    adhoc_channel_t      *channel;
    uint8_t               node_idx;
    uint8_t               log_slot;
    adhoc_virtual_time_t *vt;
    uint32_t              node_id;
    uint32_t              rand_state;
} adhoc_link_win32_ctx_t;

extern const adhoc_link_ops_t g_adhoc_link_win32_ops;

void adhoc_link_win32_ctx_init(adhoc_link_win32_ctx_t *ctx,
                               adhoc_channel_t *channel, uint8_t node_idx,
                               uint8_t log_slot,
                               adhoc_virtual_time_t *vt,
                               uint32_t node_id);

#ifdef __cplusplus
}
#endif

#endif /* ADHOC_LINK_WIN32_H */
