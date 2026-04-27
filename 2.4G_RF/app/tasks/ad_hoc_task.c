#include "ad_hoc_task.h"

#include "adhoc_api.h"
#include "adhoc_link_aros.h"
#include "log_print.h"
#include "sensor_task.h"
#include "tmos_task.h"

#include <string.h>

#define AD_HOC_EVT_INIT (0x0001u << 0)
#define AD_HOC_EVT_POLL (0x0001u << 1)

#define AD_HOC_POLL_MS 5u
#define AD_HOC_RX_BUDGET_PER_POLL 8u
#define AD_HOC_TX_BUDGET_PER_POLL 8u
#define AD_HOC_DATA_PUSH_MS 1000u
#define AD_HOC_SENSOR_PAYLOAD_TYPE 0xA1u
#define AD_HOC_SOURCE_ID_FLAG 0u
#define AD_HOC_NETWORK_WINDOW_US 30000000u
#define AD_HOC_REGROUP_INTERVAL_US 0u

#ifndef RF_TG_ID
#define RF_TG_ID 0u
#endif

#ifndef ADHOC_TASK_DOMAIN_ID
#define ADHOC_TASK_DOMAIN_ID 1u
#endif

#ifndef ADHOC_TASK_GATEWAY_NO
#define ADHOC_TASK_GATEWAY_NO 0u
#endif

#ifndef ADHOC_TASK_NODE_MEM_CAP
#define ADHOC_TASK_NODE_MEM_CAP 4096u
#endif

static tmosTaskID s_ad_hoc_task_id = INVALID_TASK_ID;
static uint8_t s_node_mem[ADHOC_TASK_NODE_MEM_CAP];
static adhoc_link_aros_ctx_t s_link_ctx;
static const adhoc_link_ops_t *s_link_ops = 0;
static ad_hoc_task_status_t s_status = {0};
static uint32_t s_last_data_push_ms = 0u;
static uint16_t s_data_seq = 0u;

static tmosEvents ad_hoc_task_process_event(tmosTaskID task_id, tmosEvents events);
static void ad_hoc_task_sync_link_status(void);
static void ad_hoc_task_sync_runtime_status(uint32_t now_us);

static int16_t ad_hoc_clamp_i16(int32_t v)
{
    if (v > 32767)
    {
        return 32767;
    }
    if (v < -32768)
    {
        return -32768;
    }
    return (int16_t)v;
}

static void ad_hoc_put_u16(uint8_t *dst, uint16_t v)
{
    dst[0] = (uint8_t)((v >> 8) & 0xFFu);
    dst[1] = (uint8_t)(v & 0xFFu);
}

static void ad_hoc_build_sensor_payload(const sensor_snapshot_t *snap, uint8_t out_user[ADHOC_DATA_USER_LEN])
{
    int16_t ax;
    int16_t ay;
    int16_t az;
    int16_t t;
    uint16_t rh;

    if (snap == 0 || out_user == 0)
    {
        return;
    }

    ax = ad_hoc_clamp_i16(snap->accel_mg_x);
    ay = ad_hoc_clamp_i16(snap->accel_mg_y);
    az = ad_hoc_clamp_i16(snap->accel_mg_z);
    t = ad_hoc_clamp_i16(snap->temp_centi_c);
    rh = (snap->rh_centi_pct < 0) ? 0u : (uint16_t)snap->rh_centi_pct;

    memset(out_user, 0, ADHOC_DATA_USER_LEN);
    out_user[0] = AD_HOC_SENSOR_PAYLOAD_TYPE;
    out_user[1] = snap->ready;
    ad_hoc_put_u16(&out_user[2], (uint16_t)ax);
    ad_hoc_put_u16(&out_user[4], (uint16_t)ay);
    ad_hoc_put_u16(&out_user[6], (uint16_t)az);
    ad_hoc_put_u16(&out_user[8], (uint16_t)t);
    ad_hoc_put_u16(&out_user[10], rh);
    ad_hoc_put_u16(&out_user[12], snap->accel_hz);
    ad_hoc_put_u16(&out_user[14], snap->sht_hz);
    out_user[16] = (uint8_t)(snap->sht_ok_cnt & 0xFFu);
    out_user[17] = (uint8_t)(snap->sht_err_cnt & 0xFFu);
}

static void ad_hoc_task_try_submit_sensor_data(uint32_t now_us)
{
    sensor_snapshot_t snap;
    uint8_t user[ADHOC_DATA_USER_LEN];
    uint32_t now_ms;
    adhoc_rc_t rc;

    if (s_status.inited == 0u || s_status.role_gateway != 0u)
    {
        return;
    }

    now_ms = now_us / 1000u;
    if (s_last_data_push_ms != 0u && (uint32_t)(now_ms - s_last_data_push_ms) < AD_HOC_DATA_PUSH_MS)
    {
        return;
    }
    s_last_data_push_ms = now_ms;

    memset(&snap, 0, sizeof(snap));
    sensor_task_get_snapshot(&snap);
    if (snap.ready == 0u)
    {
        return;
    }

    ad_hoc_build_sensor_payload(&snap, user);
    s_status.data_submit_try++;
    rc = adhoc_node_submit_data(s_node_mem, AD_HOC_SOURCE_ID_FLAG, s_data_seq, user, now_us);
    if (rc == ADHOC_OK)
    {
        s_status.data_submit_ok++;
        s_status.last_data_submit_seq = s_data_seq;
        s_data_seq++;
    }
    else if (rc == ADHOC_EBUSY)
    {
        s_status.data_submit_busy++;
    }
    else if (rc == ADHOC_ESTATE)
    {
        s_status.data_submit_state_skip++;
    }
    else
    {
        s_status.data_submit_fail++;
    }
}

static uint8_t ad_hoc_task_is_gateway(uint8_t tg_id)
{
    return tg_id == 0u ? 1u : 0u;
}

static uint32_t ad_hoc_task_node_id_from_tg(uint8_t tg_id)
{
    return tg_id == 0u ? 1u : (1000000u + (uint32_t)tg_id);
}

static uint32_t ad_hoc_task_now_us(void)
{
    if (s_link_ops != 0 && s_link_ops->now_us != 0)
    {
        return s_link_ops->now_us(&s_link_ctx);
    }
    return 0u;
}

static void ad_hoc_task_drain_tx(void)
{
    adhoc_frame_t tx_frame;
    adhoc_rc_t rc;
    uint8_t budget = AD_HOC_TX_BUDGET_PER_POLL;

    while (budget > 0u)
    {
        rc = adhoc_node_fetch_tx(s_node_mem, &tx_frame);
        if (rc == ADHOC_ENOFRAME)
        {
            return;
        }
        if (rc != ADHOC_OK)
        {
            s_status.node_err_cnt++;
            return;
        }
        if (s_link_ops == 0 || s_link_ops->tx == 0)
        {
            s_status.tx_err_cnt++;
            return;
        }
        if (s_link_ops->tx(&s_link_ctx, tx_frame.bytes, tx_frame.len) == 0)
        {
            s_status.tx_cnt++;
        }
        else
        {
            s_status.tx_err_cnt++;
            return;
        }
        budget--;
    }
}

static void ad_hoc_task_drain_rx(void)
{
    uint8_t rx_buf[ADHOC_FRAME_LEN];
    adhoc_frame_t rx_frame;
    uint16_t rx_len;
    int8_t rssi;
    int poll_rc;
    uint8_t budget = AD_HOC_RX_BUDGET_PER_POLL;

    if (s_link_ops == 0 || s_link_ops->poll_rx == 0)
    {
        return;
    }

    while (budget > 0u)
    {
        rx_len = (uint16_t)sizeof(rx_buf);
        rssi = 0;
        poll_rc = s_link_ops->poll_rx(&s_link_ctx, rx_buf, &rx_len, &rssi);
        if (poll_rc == 1)
        {
            return;
        }
        if (poll_rc != 0)
        {
            s_status.rx_err_cnt++;
            return;
        }
        if (rx_len != ADHOC_FRAME_LEN)
        {
            s_status.rx_err_cnt++;
            budget--;
            continue;
        }

        memset(&rx_frame, 0, sizeof(rx_frame));
        memcpy(rx_frame.bytes, rx_buf, ADHOC_FRAME_LEN);
        rx_frame.len = ADHOC_FRAME_LEN;
        rx_frame.rssi = rssi;
        rx_frame.ts_us = ad_hoc_task_now_us();
        if (adhoc_node_on_rx(s_node_mem, &rx_frame) == ADHOC_OK)
        {
            s_status.rx_cnt++;
        }
        else
        {
            s_status.rx_err_cnt++;
        }
        budget--;
    }
}

static void ad_hoc_task_drain_tx_reports(void)
{
    adhoc_node_data_tx_report_t report;
    uint8_t budget = AD_HOC_TX_BUDGET_PER_POLL;

    while (budget > 0u)
    {
        if (adhoc_node_fetch_data_tx_report(s_node_mem, &report) != ADHOC_OK)
        {
            return;
        }
        s_status.last_tx_report_code = report.code;
        s_status.last_tx_report_seq = report.seq_no;
        s_status.last_tx_report_retry = report.retry_count;
        if (report.code == ADHOC_NODE_DATA_TX_REPORT_ACKED)
        {
            s_status.tx_report_acked++;
        }
        else if (report.code == ADHOC_NODE_DATA_TX_REPORT_RETRY_EXHAUSTED)
        {
            s_status.tx_report_retry_exhausted++;
        }
        budget--;
    }
}

static void ad_hoc_task_sync_link_status(void)
{
    adhoc_link_aros_status_t link_st;

    adhoc_link_aros_get_status(&s_link_ctx, &link_st);
    s_status.link_init_cnt = link_st.init_cnt;
    s_status.link_start_rx_cnt = link_st.start_rx_cnt;
    s_status.link_start_rx_err_cnt = link_st.start_rx_err_cnt;
    s_status.link_start_rx_skip_txbusy_cnt = link_st.start_rx_skip_txbusy_cnt;
    s_status.link_tx_req_cnt = link_st.tx_req_cnt;
    s_status.link_tx_ok_cnt = link_st.tx_ok_cnt;
    s_status.link_tx_busy_cnt = link_st.tx_busy_cnt;
    s_status.link_tx_fail_cnt = link_st.tx_fail_cnt;
    s_status.link_rx_poll_cnt = link_st.rx_poll_cnt;
    s_status.link_rx_ok_cnt = link_st.rx_ok_cnt;
    s_status.link_rx_empty_cnt = link_st.rx_empty_cnt;
    s_status.link_rx_err_cnt = link_st.rx_err_cnt;
    s_status.link_last_tx_ret = link_st.last_tx_ret;
    s_status.link_last_poll_ret = link_st.last_poll_ret;
    s_status.link_last_rssi = link_st.last_rssi;
    s_status.link_last_rx_tc = link_st.last_rx_tc;
}

static void ad_hoc_task_sync_runtime_status(uint32_t now_us)
{
    adhoc_node_runtime_status_t runtime_st;
    uint32_t left_us;

    memset(&runtime_st, 0, sizeof(runtime_st));
    if (adhoc_node_get_runtime_status(s_node_mem, &runtime_st) != ADHOC_OK)
    {
        return;
    }

    s_status.sm_state = runtime_st.state;
    s_status.sm_joined_level = runtime_st.joined_level;
    s_status.sm_retry_count = runtime_st.retry_count;
    s_status.sm_upstream_gateway_no = runtime_st.upstream_gateway_no;
    s_status.sm_upstream_no = runtime_st.upstream_no;
    s_status.sm_upstream_id = runtime_st.upstream_id;
    s_status.sm_gateway_network_started = runtime_st.gateway_network_started;
    s_status.sm_gateway_network_locked = runtime_st.gateway_network_locked;
    s_status.sm_network_lock_active = runtime_st.network_lock_active;
    s_status.sm_network_lock_closed = runtime_st.network_lock_closed;

    if (runtime_st.upstream_last_seen_us != 0u && now_us >= runtime_st.upstream_last_seen_us)
    {
        s_status.sm_upstream_last_seen_age_ms = (now_us - runtime_st.upstream_last_seen_us) / 1000u;
    }
    else
    {
        s_status.sm_upstream_last_seen_age_ms = 0u;
    }

    if (runtime_st.gateway_network_end_us != 0u && now_us < runtime_st.gateway_network_end_us)
    {
        left_us = runtime_st.gateway_network_end_us - now_us;
        s_status.sm_gateway_window_left_ms = left_us / 1000u;
    }
    else
    {
        s_status.sm_gateway_window_left_ms = 0u;
    }

    if (runtime_st.network_lock_end_us != 0u && now_us < runtime_st.network_lock_end_us)
    {
        left_us = runtime_st.network_lock_end_us - now_us;
        s_status.sm_network_lock_left_ms = left_us / 1000u;
    }
    else
    {
        s_status.sm_network_lock_left_ms = 0u;
    }
}

void ad_hoc_task_init(void)
{
    if (s_ad_hoc_task_id != INVALID_TASK_ID)
    {
        return;
    }

    s_ad_hoc_task_id = TMOS_ProcessEventRegister(ad_hoc_task_process_event);
    if (s_ad_hoc_task_id == INVALID_TASK_ID)
    {
        LOG_PRINT("ad_hoc task register failed\r\n");
        return;
    }

    memset(&s_status, 0, sizeof(s_status));
    s_last_data_push_ms = 0u;
    s_data_seq = 0u;
    tmos_set_event(s_ad_hoc_task_id, AD_HOC_EVT_INIT);
}

void ad_hoc_task_get_status(ad_hoc_task_status_t *out)
{
    if (out == NULL)
    {
        return;
    }
    *out = s_status;
}

static tmosEvents ad_hoc_task_process_event(tmosTaskID task_id, tmosEvents events)
{
    adhoc_cfg_t cfg;
    uint8_t role_gateway;
    uint32_t node_mem_required;
    uint32_t now_us;
    adhoc_rc_t rc;

    (void)task_id;

    if (events & AD_HOC_EVT_INIT)
    {
        memset(s_node_mem, 0, sizeof(s_node_mem));
        adhoc_link_aros_ctx_init(&s_link_ctx);
        s_link_ops = adhoc_link_aros_get_ops();
        if (s_link_ops == 0)
        {
            LOG_PRINT("ad_hoc link ops missing\r\n");
            return (events ^ AD_HOC_EVT_INIT);
        }

        memset(&cfg, 0, sizeof(cfg));
        role_gateway = ad_hoc_task_is_gateway((uint8_t)RF_TG_ID);
        cfg.domain_id = (uint16_t)ADHOC_TASK_DOMAIN_ID;
        cfg.gateway_no = (uint8_t)ADHOC_TASK_GATEWAY_NO;
        cfg.node_id = ad_hoc_task_node_id_from_tg((uint8_t)RF_TG_ID);
        cfg.t1_us = 2500u;
        cfg.t2_us = 57500u;
        cfg.t3_us = 62500u;
        cfg.t4_us = 1937500u;
        cfg.retry_max = 30u;
        cfg.network_window_us = AD_HOC_NETWORK_WINDOW_US;
        cfg.regroup_interval_us = AD_HOC_REGROUP_INTERVAL_US;

        node_mem_required = adhoc_node_required_size();
        if (node_mem_required > (uint32_t)sizeof(s_node_mem))
        {
            LOG_PRINT("ad_hoc node mem not enough: need=%lu cap=%u\r\n",
                      (unsigned long)node_mem_required, (unsigned)sizeof(s_node_mem));
            return (events ^ AD_HOC_EVT_INIT);
        }

        rc = adhoc_node_init(s_node_mem, (uint32_t)sizeof(s_node_mem), &cfg, s_link_ops, &s_link_ctx);
        if (rc != ADHOC_OK)
        {
            LOG_PRINT("ad_hoc node init failed: %d\r\n", (int)rc);
            return (events ^ AD_HOC_EVT_INIT);
        }

        rc = adhoc_node_set_role(s_node_mem, role_gateway != 0u ? ADHOC_ROLE_GATEWAY : ADHOC_ROLE_BEACON);
        if (rc != ADHOC_OK)
        {
            LOG_PRINT("ad_hoc node role failed: %d\r\n", (int)rc);
            return (events ^ AD_HOC_EVT_INIT);
        }

        s_status.inited = 1u;
        s_status.role_gateway = role_gateway;
        s_status.gateway_no = cfg.gateway_no;
        s_status.domain_id = cfg.domain_id;
        s_status.node_id = cfg.node_id;
        ad_hoc_task_sync_runtime_status(ad_hoc_task_now_us());
        ad_hoc_task_sync_link_status();

        tmos_start_reload_task(s_ad_hoc_task_id, AD_HOC_EVT_POLL, MS1_TO_SYSTEM_TIME(AD_HOC_POLL_MS));
        LOG_PRINT("ad_hoc task started: role=%s domain=%u node=%lu gw_no=%u\r\n",
                  role_gateway != 0u ? "GW" : "BCN",
                  (unsigned)cfg.domain_id,
                  (unsigned long)cfg.node_id,
                  (unsigned)cfg.gateway_no);
        return (events ^ AD_HOC_EVT_INIT);
    }

    if (events & AD_HOC_EVT_POLL)
    {
        if (s_status.inited == 0u)
        {
            return (events ^ AD_HOC_EVT_POLL);
        }

        now_us = ad_hoc_task_now_us();
        ad_hoc_task_try_submit_sensor_data(now_us);
        if (adhoc_node_poll(s_node_mem, now_us) != ADHOC_OK)
        {
            s_status.node_err_cnt++;
        }
        ad_hoc_task_drain_tx();
        ad_hoc_task_drain_rx();
        ad_hoc_task_drain_tx_reports();
        ad_hoc_task_sync_runtime_status(now_us);
        ad_hoc_task_sync_link_status();
        return (events ^ AD_HOC_EVT_POLL);
    }

    return 0;
}
