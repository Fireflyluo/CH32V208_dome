#include "adhoc_link_win32.h"
#include "adhoc_logger.h"

#include <stdlib.h>
#include <string.h>

static adhoc_link_win32_ctx_t *cast_ctx(void *ctx)
{
    return (adhoc_link_win32_ctx_t *)ctx;
}

static int link_init(void *ctx)
{
    adhoc_link_win32_ctx_t *lctx = cast_ctx(ctx);
    if (lctx == NULL)
    {
        return ADHOC_LINK_EINVAL;
    }
    adhoc_logger_log(lctx->log_slot, "[NODE:%u] link_init", lctx->node_id);
    return ADHOC_LINK_OK;
}

static int link_start_rx(void *ctx)
{
    (void)ctx;
    return ADHOC_LINK_OK;
}

static int link_tx(void *ctx, const uint8_t *buf, uint16_t len)
{
    adhoc_link_win32_ctx_t *lctx = cast_ctx(ctx);

    if (lctx == NULL || buf == NULL)
    {
        return ADHOC_LINK_EINVAL;
    }
    if (len != ADHOC_FRAME_LEN)
    {
        return ADHOC_LINK_EINVAL;
    }

    if (!adhoc_channel_tx(lctx->channel, lctx->node_idx, buf, (uint8_t)len))
    {
        return ADHOC_LINK_EBUSY;
    }

    adhoc_logger_log(lctx->log_slot, "[NODE:%u] TX class=%c level=%u",
                     lctx->node_id, (buf[0] & 0x80u) ? 'D' : 'A', buf[1]);

    return ADHOC_LINK_OK;
}

static int link_poll_rx(void *ctx, uint8_t *buf, uint16_t *len, int8_t *rssi)
{
    adhoc_link_win32_ctx_t *lctx = cast_ctx(ctx);
    uint8_t rx_len = 0u;

    if (lctx == NULL || buf == NULL || rssi == NULL)
    {
        return ADHOC_LINK_EINVAL;
    }

    if (!adhoc_channel_poll_rx(lctx->channel, lctx->node_idx, buf, &rx_len, rssi))
    {
        if (len != NULL)
        {
            *len = 0u;
        }
        return ADHOC_LINK_RX_EMPTY;
    }

    if (len != NULL)
    {
        *len = (uint16_t)rx_len;
    }

    adhoc_logger_log(lctx->log_slot, "[NODE:%u] RX class=%c level=%u rssi=%d",
                     lctx->node_id, (buf[0] & 0x80u) ? 'D' : 'A', buf[1], *rssi);

    return ADHOC_LINK_OK;
}

static uint32_t link_now_us(void *ctx)
{
    adhoc_link_win32_ctx_t *lctx = cast_ctx(ctx);
    if (lctx == NULL || lctx->vt == NULL)
    {
        return 0u;
    }
    return adhoc_virtual_time_now_us(lctx->vt);
}

static uint16_t link_rand_u16(void *ctx)
{
    adhoc_link_win32_ctx_t *lctx = cast_ctx(ctx);
    uint16_t r;

    if (lctx == NULL)
    {
        return (uint16_t)(rand() & 0xFFFFu);
    }

    r = (uint16_t)(rand() & 0xFFFFu);
    lctx->rand_state = (lctx->rand_state * 1103515245u + 12345u) ^ (uint32_t)r;
    return (uint16_t)(lctx->rand_state & 0xFFFFu);
}

const adhoc_link_ops_t g_adhoc_link_win32_ops = {
    link_init,
    link_start_rx,
    link_tx,
    link_poll_rx,
    link_now_us,
    link_rand_u16
};

void adhoc_link_win32_ctx_init(adhoc_link_win32_ctx_t *ctx,
                               adhoc_channel_t *channel, uint8_t node_idx,
                               uint8_t log_slot,
                               adhoc_virtual_time_t *vt,
                               uint32_t node_id)
{
    if (ctx == NULL)
    {
        return;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->channel    = channel;
    ctx->node_idx   = node_idx;
    ctx->log_slot   = log_slot;
    ctx->vt         = vt;
    ctx->node_id    = node_id;
    ctx->rand_state = (uint32_t)(node_id ^ 0xA5A55A5Au);
}
