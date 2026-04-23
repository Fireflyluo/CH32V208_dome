#include "sc7a20_ch32_adapter.h"
#include "i2c_bus_arbiter.h"

#include <errno.h>

#ifndef SC7A20_I2C_USE_DMA_BURST_READ
#define SC7A20_I2C_USE_DMA_BURST_READ 1
#endif

#ifndef SC7A20_I2C_DMA_BURST_MIN_LEN
#define SC7A20_I2C_DMA_BURST_MIN_LEN 6u
#endif

#ifndef SC7A20_CH32_XFER_STATS_ENABLE
#define SC7A20_CH32_XFER_STATS_ENABLE 0
#endif

#ifndef SC7A20_I2C_FIFO_DATA_DMA_POLICY_DEFAULT
#define SC7A20_I2C_FIFO_DATA_DMA_POLICY_DEFAULT SC7A20_FIFO_DMA_POLICY_AUTO
#endif

#ifndef SC7A20_I2C_FIFO_DMA_RETRY_BACKOFF
#define SC7A20_I2C_FIFO_DMA_RETRY_BACKOFF 32u
#endif

#define SC7A20_I2C_REQ_TIMEOUT_MS 50U
#define SC7A20_I2C_OWNER_ID       1U

static volatile uint8_t s_fifo_dma_policy =
    (uint8_t)SC7A20_I2C_FIFO_DATA_DMA_POLICY_DEFAULT;
static volatile uint16_t s_fifo_dma_backoff_left = 0u;

#if SC7A20_CH32_XFER_STATS_ENABLE
static volatile sc7a20_ch32_xfer_stats_t s_xfer_stats = {0};
#endif

static bool ch32_fifo_dma_allowed(void)
{
    uint8_t policy = s_fifo_dma_policy;

    if (policy == (uint8_t)SC7A20_FIFO_DMA_POLICY_FORCE_DMA)
    {
        return true;
    }
    if (policy == (uint8_t)SC7A20_FIFO_DMA_POLICY_FORCE_IT)
    {
        return false;
    }

    if (s_fifo_dma_backoff_left > 0u)
    {
        s_fifo_dma_backoff_left--;
#if SC7A20_CH32_XFER_STATS_ENABLE
        s_xfer_stats.fifo_dma_backoff_skip_cnt++;
#endif
        return false;
    }
    return true;
}

static int ch32_do_two_msg_xfer(sc7a20_ch32_bus_ctx_t *ctx, const sc7a20_comm_msg_t *msgs)
{
    const uint8_t reg_raw = msgs[0].buf[0];
    const uint8_t reg = (uint8_t)(reg_raw & 0x7Fu);
    const bool auto_inc = (reg_raw & 0x80u) != 0u;
    const bool is_fifo_data_reg = (reg == SC7A20_FIFO_DATA);
    const bool dma_prefer = is_fifo_data_reg ? ch32_fifo_dma_allowed() : true;
    const uint8_t reg_for_xfer = (uint8_t)((auto_inc && msgs[1].len > 1u) ? (reg | 0x80u) : reg);
    i2c_bus_request_t req;

    if ((msgs[0].flags & SC7A20_COMM_WRITE) == 0u || msgs[0].len != 1u)
    {
        return -EINVAL;
    }

    if ((msgs[1].flags & SC7A20_COMM_READ) != 0u)
    {
        int rc;

        req.bus = ctx->i2c_num;
        req.type = I2C_BUS_REQ_READ_REG;
        req.owner_id = SC7A20_I2C_OWNER_ID;
        req.dev_addr = ctx->dev_addr;
        req.reg = reg_for_xfer;
        req.wbuf = NULL;
        req.rbuf = msgs[1].buf;
        req.len = msgs[1].len;
        req.prio = I2C_BUS_PRIO_HIGH;
        req.timeout_ms = SC7A20_I2C_REQ_TIMEOUT_MS;

#if SC7A20_I2C_USE_DMA_BURST_READ
        if (dma_prefer && msgs[1].len >= SC7A20_I2C_DMA_BURST_MIN_LEN)
        {
#if SC7A20_CH32_XFER_STATS_ENABLE
            s_xfer_stats.dma_try_cnt++;
            if (is_fifo_data_reg)
            {
                s_xfer_stats.fifo_dma_try_cnt++;
            }
#endif
            req.mode_hint = I2C_MODE_DMA;
            rc = i2c_bus_submit_sync(&req);
            if (rc == 0)
            {
                if (is_fifo_data_reg && s_fifo_dma_policy == (uint8_t)SC7A20_FIFO_DMA_POLICY_AUTO)
                {
                    s_fifo_dma_backoff_left = 0u;
                }
#if SC7A20_CH32_XFER_STATS_ENABLE
                s_xfer_stats.dma_ok_cnt++;
                if (is_fifo_data_reg)
                {
                    s_xfer_stats.fifo_dma_ok_cnt++;
                }
#endif
                return 0;
            }
            if (is_fifo_data_reg)
            {
                if (s_fifo_dma_policy == (uint8_t)SC7A20_FIFO_DMA_POLICY_AUTO)
                {
                    s_fifo_dma_backoff_left = SC7A20_I2C_FIFO_DMA_RETRY_BACKOFF;
                }
#if SC7A20_CH32_XFER_STATS_ENABLE
                s_xfer_stats.fifo_dma_fail_cnt++;
#endif
            }
            /* DMA burst失败后回退IT，提升抗干扰能力 */
#if SC7A20_CH32_XFER_STATS_ENABLE
            s_xfer_stats.it_fallback_cnt++;
#endif
        }
#endif
        req.mode_hint = I2C_MODE_IT;
#if SC7A20_CH32_XFER_STATS_ENABLE
        s_xfer_stats.it_direct_cnt++;
#endif
        return i2c_bus_submit_sync(&req);
    }

    if ((msgs[1].flags & SC7A20_COMM_WRITE) != 0u)
    {
        req.bus = ctx->i2c_num;
        req.type = I2C_BUS_REQ_WRITE_REG;
        req.owner_id = SC7A20_I2C_OWNER_ID;
        req.dev_addr = ctx->dev_addr;
        req.reg = reg_for_xfer;
        req.wbuf = msgs[1].buf;
        req.rbuf = NULL;
        req.len = msgs[1].len;
        req.mode_hint = I2C_MODE_IT;
        req.prio = I2C_BUS_PRIO_HIGH;
        req.timeout_ms = SC7A20_I2C_REQ_TIMEOUT_MS;

        return i2c_bus_submit_sync(&req);
    }

    return -EINVAL;
}

static int ch32_i2c_xfer(void *bus_ctx,
                         const sc7a20_comm_msg_t *msgs,
                         uint8_t cnt,
                         sc7a20_bus_done_cb_t cb,
                         void *user)
{
    sc7a20_ch32_bus_ctx_t *ctx;
    int rc;

    if (bus_ctx == NULL || msgs == NULL || cnt == 0u)
    {
        return -EINVAL;
    }

    ctx = (sc7a20_ch32_bus_ctx_t *)bus_ctx;

    if (cnt == 2u)
    {
        rc = ch32_do_two_msg_xfer(ctx, msgs);
    }
    else
    {
        rc = -ENOTSUP;
    }

    if (cb != NULL)
    {
        cb(user, rc);
    }

    return rc;
}

static int ch32_i2c_cancel(void *bus_ctx)
{
    sc7a20_ch32_bus_ctx_t *ctx;

    if (bus_ctx == NULL)
    {
        return -EINVAL;
    }

    ctx = (sc7a20_ch32_bus_ctx_t *)bus_ctx;
    bsp_i2c_recover(ctx->i2c_num);
    return 0;
}

const sc7a20_bus_ops_t g_sc7a20_ch32_i2c_ops = {
    .xfer = ch32_i2c_xfer,
    .cancel = ch32_i2c_cancel,
};

void sc7a20_ch32_get_xfer_stats(sc7a20_ch32_xfer_stats_t *out)
{
    if (out == NULL)
    {
        return;
    }
#if SC7A20_CH32_XFER_STATS_ENABLE
    out->dma_try_cnt = s_xfer_stats.dma_try_cnt;
    out->dma_ok_cnt = s_xfer_stats.dma_ok_cnt;
    out->it_fallback_cnt = s_xfer_stats.it_fallback_cnt;
    out->it_direct_cnt = s_xfer_stats.it_direct_cnt;
    out->fifo_dma_try_cnt = s_xfer_stats.fifo_dma_try_cnt;
    out->fifo_dma_ok_cnt = s_xfer_stats.fifo_dma_ok_cnt;
    out->fifo_dma_fail_cnt = s_xfer_stats.fifo_dma_fail_cnt;
    out->fifo_dma_backoff_skip_cnt = s_xfer_stats.fifo_dma_backoff_skip_cnt;
#else
    out->dma_try_cnt = 0u;
    out->dma_ok_cnt = 0u;
    out->it_fallback_cnt = 0u;
    out->it_direct_cnt = 0u;
    out->fifo_dma_try_cnt = 0u;
    out->fifo_dma_ok_cnt = 0u;
    out->fifo_dma_fail_cnt = 0u;
    out->fifo_dma_backoff_skip_cnt = 0u;
#endif
    out->fifo_dma_backoff_left = s_fifo_dma_backoff_left;
    out->fifo_dma_policy = s_fifo_dma_policy;
}

void sc7a20_ch32_reset_xfer_stats(void)
{
#if SC7A20_CH32_XFER_STATS_ENABLE
    s_xfer_stats.dma_try_cnt = 0u;
    s_xfer_stats.dma_ok_cnt = 0u;
    s_xfer_stats.it_fallback_cnt = 0u;
    s_xfer_stats.it_direct_cnt = 0u;
    s_xfer_stats.fifo_dma_try_cnt = 0u;
    s_xfer_stats.fifo_dma_ok_cnt = 0u;
    s_xfer_stats.fifo_dma_fail_cnt = 0u;
    s_xfer_stats.fifo_dma_backoff_skip_cnt = 0u;
#endif
}

void sc7a20_ch32_set_fifo_dma_policy(sc7a20_fifo_dma_policy_t policy)
{
    if (policy > SC7A20_FIFO_DMA_POLICY_AUTO)
    {
        policy = SC7A20_FIFO_DMA_POLICY_FORCE_IT;
    }
    s_fifo_dma_policy = (uint8_t)policy;
    s_fifo_dma_backoff_left = 0u;
}

sc7a20_fifo_dma_policy_t sc7a20_ch32_get_fifo_dma_policy(void)
{
    uint8_t policy = s_fifo_dma_policy;
    if (policy > (uint8_t)SC7A20_FIFO_DMA_POLICY_AUTO)
    {
        policy = (uint8_t)SC7A20_FIFO_DMA_POLICY_FORCE_IT;
    }
    return (sc7a20_fifo_dma_policy_t)policy;
}
