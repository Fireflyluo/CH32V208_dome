#include "sht40_ch32_adapter.h"
#include "i2c_bus_arbiter.h"

#include <errno.h>

#ifndef SHT40_I2C_USE_IT
#define SHT40_I2C_USE_IT 1
#endif

#define SHT40_I2C_REQ_TIMEOUT_MS 100U
#define SHT40_I2C_OWNER_ID       2U

static uint8_t sht40_mode_select(uint8_t is_read, uint16_t len)
{
#if SHT40_I2C_USE_IT
    (void)is_read;
    (void)len;
    /* SHT40对“发命令后延时再读”时序较敏感，统一IT模式避免混合模式切换问题 */
    return I2C_MODE_IT;
#else
    (void)is_read;
    (void)len;
    return I2C_MODE_POLLING;
#endif
}

static int sht40_ch32_xfer(void *ctx,
                           const sht40_comm_msg_t *msgs,
                           uint8_t cnt,
                           sht40_bus_done_cb_t cb,
                           void *user)
{
    sht40_ch32_bus_ctx_t *bus;
    i2c_bus_request_t req;
    int rc;

    if (ctx == NULL || msgs == NULL || cnt == 0u)
    {
        return -EINVAL;
    }

    bus = (sht40_ch32_bus_ctx_t *)ctx;

    if (cnt != 1u)
    {
        rc = -ENOTSUP;
    }
    else if ((msgs[0].flags & SHT40_COMM_READ) != 0u)
    {
        uint8_t mode = sht40_mode_select(1u, msgs[0].len);
        req.bus = bus->i2c_num;
        req.type = I2C_BUS_REQ_READ;
        req.owner_id = SHT40_I2C_OWNER_ID;
        req.dev_addr = bus->dev_addr;
        req.reg = 0u;
        req.wbuf = NULL;
        req.rbuf = msgs[0].buf;
        req.len = msgs[0].len;
        req.mode_hint = mode;
        req.prio = I2C_BUS_PRIO_NORMAL;
        req.timeout_ms = SHT40_I2C_REQ_TIMEOUT_MS;
        rc = i2c_bus_submit_sync(&req);
    }
    else if ((msgs[0].flags & SHT40_COMM_WRITE) != 0u)
    {
        uint8_t mode = sht40_mode_select(0u, msgs[0].len);
        req.bus = bus->i2c_num;
        req.type = I2C_BUS_REQ_WRITE;
        req.owner_id = SHT40_I2C_OWNER_ID;
        req.dev_addr = bus->dev_addr;
        req.reg = 0u;
        req.wbuf = msgs[0].buf;
        req.rbuf = NULL;
        req.len = msgs[0].len;
        req.mode_hint = mode;
        req.prio = I2C_BUS_PRIO_NORMAL;
        req.timeout_ms = SHT40_I2C_REQ_TIMEOUT_MS;
        rc = i2c_bus_submit_sync(&req);
    }
    else
    {
        rc = -EINVAL;
    }

    if (cb != NULL)
    {
        cb(user, rc);
    }

    return rc;
}

static int sht40_ch32_cancel(void *ctx)
{
    sht40_ch32_bus_ctx_t *bus;

    if (ctx == NULL)
    {
        return -EINVAL;
    }

    bus = (sht40_ch32_bus_ctx_t *)ctx;
    bsp_i2c_recover(bus->i2c_num);
    return 0;
}

const sht40_bus_ops_t g_sht40_ch32_i2c_ops = {
    .xfer = sht40_ch32_xfer,
    .cancel = sht40_ch32_cancel,
};
