#include "adhoc_link_aros.h"

#include "adhoc_port_ch32.h"
#include "aros_rf.h"
#include "wchble.h"

#include <string.h>

static int adhoc_link_aros_init_impl(void *ctx_mem)
{
    adhoc_link_aros_ctx_t *ctx = (adhoc_link_aros_ctx_t *)ctx_mem;

    if (ctx == NULL)
    {
        return -1;
    }
    ctx->init_cnt++;
    if (ctx->inited != 0u)
    {
        return 0;
    }

    (void)RF_RoleInit();
    arf_Init();
    adhoc_port_ch32_critical_enter();
    ctx->inited = 1u;
    if (ctx->tc_us == 0u)
    {
        ctx->tc_us = (uint32_t)ARF_TC_us;
    }
    if (ctx->rand_state == 0u)
    {
        ctx->rand_state = (adhoc_port_ch32_now_tc() << 16) ^ 0x3C2F1A0Bu;
    }
    adhoc_port_ch32_critical_exit();
    return 0;
}

static int adhoc_link_aros_start_rx_impl(void *ctx_mem)
{
    adhoc_link_aros_ctx_t *ctx = (adhoc_link_aros_ctx_t *)ctx_mem;

    if (ctx == NULL)
    {
        return -1;
    }
    if (ctx->inited == 0u)
    {
        ctx->start_rx_err_cnt++;
        return -1;
    }
    ctx->start_rx_cnt++;
    if (arf_isTxBusy() != 0)
    {
        ctx->start_rx_skip_txbusy_cnt++;
        return 0;
    }
    arf_RxStart();
    return 0;
}

static int adhoc_link_aros_tx_impl(void *ctx_mem, const uint8_t *buf, uint16_t len)
{
    adhoc_link_aros_ctx_t *ctx = (adhoc_link_aros_ctx_t *)ctx_mem;
    int ret;

    if (ctx == NULL)
    {
        return -1;
    }
    ctx->tx_req_cnt++;
    if (ctx->inited == 0u || buf == NULL || len == 0u || len > ARF_MsgN)
    {
        ctx->tx_fail_cnt++;
        ctx->last_tx_ret = -1;
        return -1;
    }

    ret = arf_TxTrySend((uint8_t *)buf, (int)len);
    ctx->last_tx_ret = ret;
    if (ret == (int)len)
    {
        ctx->tx_ok_cnt++;
        return 0;
    }
    if (ret == 0)
    {
        ctx->tx_busy_cnt++;
        return -2;
    }
    ctx->tx_fail_cnt++;
    return -3;
}

static int adhoc_link_aros_poll_rx_impl(void *ctx_mem, uint8_t *buf, uint16_t *len, int8_t *rssi)
{
    adhoc_link_aros_ctx_t *ctx = (adhoc_link_aros_ctx_t *)ctx_mem;
    uint8_t *rx;

    if (ctx == NULL || buf == NULL || len == NULL)
    {
        return -1;
    }
    ctx->rx_poll_cnt++;
    if (ctx->inited == 0u)
    {
        ctx->rx_err_cnt++;
        ctx->last_poll_ret = -1;
        return -1;
    }
    if (*len < ARF_MsgN)
    {
        ctx->rx_err_cnt++;
        ctx->last_poll_ret = -2;
        return -2;
    }

    rx = arf_isRxFinish();
    if (rx == NULL)
    {
        *len = 0u;
        ctx->rx_empty_cnt++;
        ctx->last_poll_ret = 1;
        return 1;
    }

    memcpy(buf, rx, ARF_MsgN);
    *len = ARF_MsgN;
    ctx->last_rssi = (int8_t)arf_RSSI(rx);
    ctx->last_rx_tc = arf_u16toi(rx + ARF_MsgTC);
    if (rssi != NULL)
    {
        *rssi = ctx->last_rssi;
    }
    ctx->rx_ok_cnt++;
    ctx->last_poll_ret = 0;
    return 0;
}

static uint32_t adhoc_link_aros_now_us_impl(void *ctx_mem)
{
    adhoc_link_aros_ctx_t *ctx = (adhoc_link_aros_ctx_t *)ctx_mem;
    uint32_t tc_us = (uint32_t)ARF_TC_us;

    if (ctx != NULL && ctx->tc_us != 0u)
    {
        tc_us = ctx->tc_us;
    }
    return adhoc_port_ch32_now_us(tc_us);
}

static uint16_t adhoc_link_aros_rand_u16_impl(void *ctx_mem)
{
    adhoc_link_aros_ctx_t *ctx = (adhoc_link_aros_ctx_t *)ctx_mem;

    if (ctx == NULL)
    {
        return adhoc_port_ch32_rand_u16(NULL, 0x4B5A6978u);
    }
    return adhoc_port_ch32_rand_u16(&ctx->rand_state, ctx->rand_state ^ 0x4B5A6978u);
}

void adhoc_link_aros_ctx_init(adhoc_link_aros_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        return;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->tc_us = (uint32_t)ARF_TC_us;
    ctx->rand_state = 0x1D2C3B4Au;
    ctx->last_tx_ret = 0;
    ctx->last_poll_ret = 0;
    ctx->last_rssi = 0;
    ctx->last_rx_tc = 0u;
}

void adhoc_link_aros_get_status(const adhoc_link_aros_ctx_t *ctx, adhoc_link_aros_status_t *out)
{
    if (out == NULL)
    {
        return;
    }
    memset(out, 0, sizeof(*out));
    if (ctx == NULL)
    {
        return;
    }

    out->inited = ctx->inited;
    out->tc_us = ctx->tc_us;
    out->init_cnt = ctx->init_cnt;
    out->start_rx_cnt = ctx->start_rx_cnt;
    out->start_rx_err_cnt = ctx->start_rx_err_cnt;
    out->start_rx_skip_txbusy_cnt = ctx->start_rx_skip_txbusy_cnt;
    out->tx_req_cnt = ctx->tx_req_cnt;
    out->tx_ok_cnt = ctx->tx_ok_cnt;
    out->tx_busy_cnt = ctx->tx_busy_cnt;
    out->tx_fail_cnt = ctx->tx_fail_cnt;
    out->rx_poll_cnt = ctx->rx_poll_cnt;
    out->rx_ok_cnt = ctx->rx_ok_cnt;
    out->rx_empty_cnt = ctx->rx_empty_cnt;
    out->rx_err_cnt = ctx->rx_err_cnt;
    out->last_tx_ret = ctx->last_tx_ret;
    out->last_poll_ret = ctx->last_poll_ret;
    out->last_rssi = ctx->last_rssi;
    out->last_rx_tc = ctx->last_rx_tc;
}

const adhoc_link_ops_t *adhoc_link_aros_get_ops(void)
{
    static const adhoc_link_ops_t s_ops = {
        adhoc_link_aros_init_impl,
        adhoc_link_aros_start_rx_impl,
        adhoc_link_aros_tx_impl,
        adhoc_link_aros_poll_rx_impl,
        adhoc_link_aros_now_us_impl,
        adhoc_link_aros_rand_u16_impl};

    return &s_ops;
}
