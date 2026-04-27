#ifndef ADHOC_LINK_AROS_H
#define ADHOC_LINK_AROS_H

#include "adhoc_link.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint8_t inited;
    uint32_t tc_us;
    uint32_t rand_state;
    uint32_t init_cnt;
    uint32_t start_rx_cnt;
    uint32_t start_rx_err_cnt;
    uint32_t start_rx_skip_txbusy_cnt;
    uint32_t tx_req_cnt;
    uint32_t tx_ok_cnt;
    uint32_t tx_busy_cnt;
    uint32_t tx_fail_cnt;
    uint32_t rx_poll_cnt;
    uint32_t rx_ok_cnt;
    uint32_t rx_empty_cnt;
    uint32_t rx_err_cnt;
    int32_t last_tx_ret;
    int32_t last_poll_ret;
    int8_t last_rssi;
    uint16_t last_rx_tc;
} adhoc_link_aros_ctx_t;

typedef struct
{
    uint8_t inited;
    uint32_t tc_us;
    uint32_t init_cnt;
    uint32_t start_rx_cnt;
    uint32_t start_rx_err_cnt;
    uint32_t start_rx_skip_txbusy_cnt;
    uint32_t tx_req_cnt;
    uint32_t tx_ok_cnt;
    uint32_t tx_busy_cnt;
    uint32_t tx_fail_cnt;
    uint32_t rx_poll_cnt;
    uint32_t rx_ok_cnt;
    uint32_t rx_empty_cnt;
    uint32_t rx_err_cnt;
    int32_t last_tx_ret;
    int32_t last_poll_ret;
    int8_t last_rssi;
    uint16_t last_rx_tc;
} adhoc_link_aros_status_t;

void adhoc_link_aros_ctx_init(adhoc_link_aros_ctx_t *ctx);
void adhoc_link_aros_get_status(const adhoc_link_aros_ctx_t *ctx, adhoc_link_aros_status_t *out);
const adhoc_link_ops_t *adhoc_link_aros_get_ops(void);

#ifdef __cplusplus
}
#endif

#endif /* ADHOC_LINK_AROS_H */
