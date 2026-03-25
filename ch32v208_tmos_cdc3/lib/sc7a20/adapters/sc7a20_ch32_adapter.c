#include "sc7a20_ch32_adapter.h"
#include "i2c_bus_arbiter.h"

#include <errno.h>

#ifndef SC7A20_I2C_USE_DMA_BURST_READ
#define SC7A20_I2C_USE_DMA_BURST_READ 1
#endif

#define SC7A20_I2C_REQ_TIMEOUT_MS 50U
#define SC7A20_I2C_OWNER_ID       1U

static int ch32_do_two_msg_xfer(sc7a20_ch32_bus_ctx_t *ctx, const sc7a20_comm_msg_t *msgs)
{
    const uint8_t reg_raw = msgs[0].buf[0];
    const uint8_t reg = (uint8_t)(reg_raw & 0x7Fu);
    const bool auto_inc = (reg_raw & 0x80u) != 0u;
    const uint8_t reg_for_xfer = (uint8_t)((auto_inc && msgs[1].len > 1u) ? (reg | 0x80u) : reg);
    i2c_bus_request_t req;

    if ((msgs[0].flags & SC7A20_COMM_WRITE) == 0u || msgs[0].len != 1u)
    {
        return -EINVAL;
    }

    if ((msgs[1].flags & SC7A20_COMM_READ) != 0u)
    {
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
        req.mode_hint = (msgs[1].len > 1u) ? I2C_MODE_DMA : I2C_MODE_IT;
#else
        req.mode_hint = I2C_MODE_IT;
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
