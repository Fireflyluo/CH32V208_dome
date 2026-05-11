#ifndef AD_HOC_TASK_H
#define AD_HOC_TASK_H

#include <stdint.h>

typedef struct
{
    uint8_t inited;
    uint8_t role_gateway;
    uint8_t gateway_no;
    uint16_t domain_id;
    uint32_t node_id;
    uint8_t sm_state;
    uint8_t sm_joined_level;
    uint8_t sm_retry_count;
    uint8_t sm_retry_peak;
    uint32_t sm_retry_exhausted_count;
    uint8_t sm_last_retry_exhausted;
    uint32_t sm_last_retry_exhausted_age_ms;
    uint8_t sm_upstream_gateway_no;
    uint8_t sm_upstream_no;
    uint32_t sm_upstream_id;
    uint32_t sm_upstream_last_seen_age_ms;
    uint8_t sm_gateway_network_started;
    uint8_t sm_gateway_network_locked;
    uint32_t sm_gateway_window_left_ms;
    uint8_t sm_network_lock_active;
    uint8_t sm_network_lock_closed;
    uint32_t sm_network_lock_left_ms;
    uint32_t tx_cnt;
    uint32_t rx_cnt;
    uint32_t tx_err_cnt;
    uint32_t rx_err_cnt;
    uint32_t node_err_cnt;
    uint32_t tx_report_acked;
    uint32_t tx_report_retry_exhausted;
    uint8_t last_tx_report_code;
    uint32_t last_tx_report_lmt_d;
    uint8_t last_tx_report_retry;
    uint32_t data_submit_try;
    uint32_t data_submit_ok;
    uint32_t data_submit_busy;
    uint32_t data_submit_state_skip;
    uint32_t data_submit_fail;
    uint32_t last_data_submit_lmt_d;
    uint32_t link_init_cnt;
    uint32_t link_start_rx_cnt;
    uint32_t link_start_rx_err_cnt;
    uint32_t link_start_rx_skip_txbusy_cnt;
    uint32_t link_tx_req_cnt;
    uint32_t link_tx_ok_cnt;
    uint32_t link_tx_busy_cnt;
    uint32_t link_tx_fail_cnt;
    uint32_t link_rx_poll_cnt;
    uint32_t link_rx_ok_cnt;
    uint32_t link_rx_empty_cnt;
    uint32_t link_rx_err_cnt;
    int32_t link_last_tx_ret;
    int32_t link_last_poll_ret;
    int8_t link_last_rssi;
    uint16_t link_last_rx_tc;
} ad_hoc_task_status_t;

void ad_hoc_task_init(void);
void ad_hoc_task_get_status(ad_hoc_task_status_t *out);

#endif /* AD_HOC_TASK_H */
