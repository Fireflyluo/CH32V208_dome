#include "ad_hoc_task.h"

#include "adhoc_api.h"
#include "adhoc_frame.h"
#include "adhoc_link_aros.h"
#include "log_print.h"
#include "sensor_task.h"
#include "tmos_task.h"
#include "drv_rtc.h"

#include <string.h>

#define AD_HOC_EVT_INIT (0x0001u << 0)
#define AD_HOC_EVT_POLL (0x0001u << 1)

#define AD_HOC_POLL_MS 5u
#define AD_HOC_RX_BUDGET_PER_POLL 8u
#define AD_HOC_TX_BUDGET_PER_POLL 8u
#define AD_HOC_TX_HOLD_RETRY_MAX 60u
#define AD_HOC_DATA_PUSH_MS 1000u
#define AD_HOC_SENSOR_PAYLOAD_TYPE 0xA1u
#define AD_HOC_SOURCE_ID_FLAG 0u
#define AD_HOC_NETWORK_WINDOW_US 30000000u
#define AD_HOC_REGROUP_INTERVAL_US 0u
#define AD_HOC_SM_STATE_ST1 1u
#define AD_HOC_SM_STATE_U1 2u
#define AD_HOC_SM_STATE_UN 4u

#ifndef ADHOC_TASK_GATEWAY_T2_US_OVERRIDE
#define ADHOC_TASK_GATEWAY_T2_US_OVERRIDE 0u
#endif

#ifndef RF_TG_ID
#define RF_TG_ID 0u
#endif

#ifndef ADHOC_TASK_DOMAIN_ID
#define ADHOC_TASK_DOMAIN_ID 1u
#endif

#ifndef ADHOC_TASK_FIXED_BDT_SECONDS
#define ADHOC_TASK_FIXED_BDT_SECONDS 0u
#endif

#ifndef ADHOC_TASK_RTC_SYNC_THRESHOLD_SEC
#define ADHOC_TASK_RTC_SYNC_THRESHOLD_SEC 2u
#endif

#ifndef ADHOC_TASK_RTC_SYNC_THRESHOLD_US
#define ADHOC_TASK_RTC_SYNC_THRESHOLD_US (ADHOC_TASK_RTC_SYNC_THRESHOLD_SEC * 1000000u)
#endif

#ifndef ADHOC_TASK_GATEWAY_NO
#define ADHOC_TASK_GATEWAY_NO 0u
#endif

#ifndef ADHOC_TASK_GATEWAY_PRIMARY_TG_ID
#define ADHOC_TASK_GATEWAY_PRIMARY_TG_ID 0u
#endif

#ifndef ADHOC_TASK_GATEWAY_SECONDARY_TG_ID
#define ADHOC_TASK_GATEWAY_SECONDARY_TG_ID 0xFFu
#endif

#ifndef ADHOC_TASK_GATEWAY_SECONDARY_NO
#define ADHOC_TASK_GATEWAY_SECONDARY_NO 1u
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
static adhoc_frame_t s_tx_hold_frame;
static uint8_t s_tx_hold_valid = 0u;
static uint8_t s_tx_hold_retry_cnt = 0u;
static uint8_t s_retry_max_cfg = 30u;
static uint8_t s_prev_sm_state = 0u;
static uint8_t s_prev_sm_retry_count = 0u;
static uint32_t s_last_retry_exhausted_us = 0u;
static uint32_t s_cfg_t5_us = 60000u;
static uint32_t s_cfg_slot_us = 3750u;

static tmosEvents ad_hoc_task_process_event(tmosTaskID task_id, tmosEvents events);
static void ad_hoc_task_sync_link_status(void);
static void ad_hoc_task_sync_runtime_status(uint32_t now_us);
static void ad_hoc_task_sync_protocol_time_base(uint32_t now_us);
static void ad_hoc_task_try_sync_bdt_from_a_frame(const uint8_t frame[ADHOC_FRAME_LEN], uint32_t rx_now_us);

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
}

static void ad_hoc_task_try_submit_sensor_data(uint32_t now_us)
{
    sensor_snapshot_t snap;
    uint8_t user[ADHOC_DATA_USER_LEN];
    uint32_t now_ms;
    uint32_t lmt_d;
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
    lmt_d = adhoc_lmt_d_from_us(now_us);
    s_status.data_submit_try++;
    rc = adhoc_node_submit_data(s_node_mem, AD_HOC_SOURCE_ID_FLAG, lmt_d, user, now_us);
    if (rc == ADHOC_OK)
    {
        s_status.data_submit_ok++;
        s_status.last_data_submit_lmt_d = lmt_d;
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

static uint8_t ad_hoc_task_resolve_gateway(uint8_t tg_id, uint8_t *out_gateway_no)
{
    if (out_gateway_no == 0)
    {
        return 0u;
    }

    if (tg_id == (uint8_t)ADHOC_TASK_GATEWAY_PRIMARY_TG_ID)
    {
        *out_gateway_no = (uint8_t)(ADHOC_TASK_GATEWAY_NO & 0x07u);
        return 1u;
    }

    if ((uint8_t)ADHOC_TASK_GATEWAY_SECONDARY_TG_ID <= 20u &&
        tg_id == (uint8_t)ADHOC_TASK_GATEWAY_SECONDARY_TG_ID)
    {
        *out_gateway_no = (uint8_t)(ADHOC_TASK_GATEWAY_SECONDARY_NO & 0x07u);
        return 1u;
    }

    *out_gateway_no = (uint8_t)(ADHOC_TASK_GATEWAY_NO & 0x07u);
    return 0u;
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

static uint64_t ad_hoc_task_abs_diff_u64(uint64_t a, uint64_t b)
{
    return a >= b ? (a - b) : (b - a);
}

static uint32_t ad_hoc_task_restore_sec_mod25(uint32_t sec_mod25, uint32_t ref_sec)
{
    uint32_t candidate;
    uint32_t best;
    const uint32_t sec_mod = (1u << ADHOC_LMT_A_SEC_BITS);
    uint64_t best_delta;
    uint64_t delta;

    candidate = (ref_sec & ~ADHOC_LMT_A_SEC_MASK) | (sec_mod25 & ADHOC_LMT_A_SEC_MASK);
    best = candidate;
    best_delta = ad_hoc_task_abs_diff_u64((uint64_t)candidate, (uint64_t)ref_sec);

    candidate = best + sec_mod;
    delta = ad_hoc_task_abs_diff_u64((uint64_t)candidate, (uint64_t)ref_sec);
    if (delta < best_delta)
    {
        best = candidate;
        best_delta = delta;
    }

    if (best >= sec_mod)
    {
        candidate = best - sec_mod;
        delta = ad_hoc_task_abs_diff_u64((uint64_t)candidate, (uint64_t)ref_sec);
        if (delta < best_delta)
        {
            best = candidate;
        }
    }

    return best;
}

static void ad_hoc_task_sync_protocol_time_base(uint32_t now_us)
{
    uint64_t bdt_us;
    uint32_t bdt_sec;
    uint32_t bdt_sub_us;
    uint32_t base_mono_us;

    if (drv_rtc_is_ready() == 0u)
    {
        adhoc_time_clear_bdt_base();
        return;
    }

    bdt_us = drv_rtc_get_bdt_time_us();
    bdt_sec = (uint32_t)(bdt_us / 1000000ull);
    bdt_sub_us = (uint32_t)(bdt_us % 1000000ull);
    base_mono_us = now_us - bdt_sub_us;
    adhoc_time_set_bdt_base(base_mono_us, bdt_sec);
}

static void ad_hoc_task_try_sync_bdt_from_a_frame(const uint8_t frame[ADHOC_FRAME_LEN], uint32_t rx_now_us)
{
    adhoc_frame_fields_t fields;
    uint32_t lmt_a_raw;
    uint32_t lmt_a_sec_mod25;
    uint8_t lmt_a_period;
    uint32_t lmt_a_bdt_sec;
    uint32_t remote_sub_us;
    uint32_t base_mono_us;
    uint64_t remote_bdt_us;
    uint64_t rtc_bdt_us;
    uint32_t rtc_bdt_sec_ref = 0u;

    if (frame == 0)
    {
        return;
    }
    if (!adhoc_frame_parse(frame, &fields))
    {
        return;
    }
    if (fields.msg_class != ADHOC_MSG_CLASS_A)
    {
        return;
    }

    if (drv_rtc_is_ready() != 0u)
    {
        rtc_bdt_sec_ref = drv_rtc_get_bdt_seconds();
    }

    lmt_a_raw = adhoc_u32_be_read(&fields.content[0]);
    lmt_a_sec_mod25 = adhoc_lmt_a_sec_get(lmt_a_raw);
    lmt_a_period = adhoc_lmt_a_period_get(lmt_a_raw);
    if (rtc_bdt_sec_ref != 0u)
    {
        lmt_a_bdt_sec = ad_hoc_task_restore_sec_mod25(lmt_a_sec_mod25, rtc_bdt_sec_ref);
    }
    else
    {
        lmt_a_bdt_sec = lmt_a_sec_mod25;
    }
    remote_sub_us = (uint32_t)lmt_a_period * s_cfg_t5_us;
    remote_sub_us += (uint32_t)(fields.slot_high4 & 0x0Fu) * s_cfg_slot_us;
    if (remote_sub_us >= 1000000u)
    {
        lmt_a_bdt_sec += remote_sub_us / 1000000u;
        remote_sub_us %= 1000000u;
    }
    base_mono_us = rx_now_us - remote_sub_us;
    adhoc_time_set_bdt_base(base_mono_us, lmt_a_bdt_sec);

    if (drv_rtc_is_ready() == 0u)
    {
        return;
    }

    rtc_bdt_us = drv_rtc_get_bdt_time_us();
    remote_bdt_us = ((uint64_t)lmt_a_bdt_sec * 1000000ull) + (uint64_t)remote_sub_us;
    if (ad_hoc_task_abs_diff_u64(rtc_bdt_us, remote_bdt_us) > (uint64_t)ADHOC_TASK_RTC_SYNC_THRESHOLD_US)
    {
        drv_rtc_set_bdt_time_us(lmt_a_bdt_sec, remote_sub_us);
    }
}

static void ad_hoc_task_drain_tx(void)
{
    adhoc_frame_t tx_frame;
    adhoc_rc_t rc;
    int tx_rc;
    uint8_t budget = AD_HOC_TX_BUDGET_PER_POLL;

    if (s_link_ops == 0 || s_link_ops->tx == 0)
    {
        s_status.tx_err_cnt++;
        return;
    }

    while (budget > 0u)
    {
        if (s_tx_hold_valid == 0u)
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
            s_tx_hold_frame = tx_frame;
            s_tx_hold_valid = 1u;
            s_tx_hold_retry_cnt = 0u;
        }

        tx_rc = s_link_ops->tx(&s_link_ctx, s_tx_hold_frame.bytes, s_tx_hold_frame.len);
        if (tx_rc == ADHOC_LINK_OK)
        {
            s_status.tx_cnt++;
            memset(&s_tx_hold_frame, 0, sizeof(s_tx_hold_frame));
            s_tx_hold_valid = 0u;
            s_tx_hold_retry_cnt = 0u;
            budget--;
            continue;
        }

        if (tx_rc == ADHOC_LINK_EBUSY)
        {
            return;
        }

        s_status.tx_err_cnt++;
        if (s_tx_hold_retry_cnt < AD_HOC_TX_HOLD_RETRY_MAX)
        {
            s_tx_hold_retry_cnt++;
            return;
        }

        memset(&s_tx_hold_frame, 0, sizeof(s_tx_hold_frame));
        s_tx_hold_valid = 0u;
        s_tx_hold_retry_cnt = 0u;
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
        if (poll_rc == ADHOC_LINK_RX_EMPTY)
        {
            return;
        }
        if (poll_rc != ADHOC_LINK_OK)
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
        ad_hoc_task_try_sync_bdt_from_a_frame(rx_buf, rx_frame.ts_us);
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
        s_status.last_tx_report_lmt_d = report.lmt_d;
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
    uint8_t prev_state;
    uint8_t prev_retry_count;

    memset(&runtime_st, 0, sizeof(runtime_st));
    if (adhoc_node_get_runtime_status(s_node_mem, &runtime_st) != ADHOC_OK)
    {
        return;
    }

    prev_state = s_prev_sm_state;
    prev_retry_count = s_prev_sm_retry_count;

    s_status.sm_state = runtime_st.state;
    s_status.sm_joined_level = runtime_st.joined_level;
    s_status.sm_retry_count = runtime_st.retry_count;
    if (runtime_st.retry_count > s_status.sm_retry_peak)
    {
        s_status.sm_retry_peak = runtime_st.retry_count;
    }
    if ((prev_state == AD_HOC_SM_STATE_U1 || prev_state == AD_HOC_SM_STATE_UN) &&
        prev_retry_count >= s_retry_max_cfg &&
        runtime_st.state == AD_HOC_SM_STATE_ST1 &&
        runtime_st.retry_count == 0u)
    {
        s_status.sm_retry_exhausted_count++;
        s_status.sm_last_retry_exhausted = prev_retry_count;
        s_last_retry_exhausted_us = now_us;
    }
    s_prev_sm_state = runtime_st.state;
    s_prev_sm_retry_count = runtime_st.retry_count;
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

    if (s_last_retry_exhausted_us != 0u && now_us >= s_last_retry_exhausted_us)
    {
        s_status.sm_last_retry_exhausted_age_ms = (now_us - s_last_retry_exhausted_us) / 1000u;
    }
    else
    {
        s_status.sm_last_retry_exhausted_age_ms = 0u;
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
    memset(&s_tx_hold_frame, 0, sizeof(s_tx_hold_frame));
    s_tx_hold_valid = 0u;
    s_tx_hold_retry_cnt = 0u;
    s_retry_max_cfg = 30u;
    s_prev_sm_state = 0u;
    s_prev_sm_retry_count = 0u;
    s_last_retry_exhausted_us = 0u;
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
    uint8_t gateway_no;
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
        gateway_no = (uint8_t)ADHOC_TASK_GATEWAY_NO;
        role_gateway = ad_hoc_task_resolve_gateway((uint8_t)RF_TG_ID, &gateway_no);
        cfg.domain_id = (uint16_t)ADHOC_TASK_DOMAIN_ID;
        cfg.gateway_no = gateway_no;
        cfg.node_id = ad_hoc_task_node_id_from_tg((uint8_t)RF_TG_ID);
        cfg.t1_us = 2500u;
        cfg.t2_us = (role_gateway != 0u && ADHOC_TASK_GATEWAY_T2_US_OVERRIDE != 0u)
                        ? (uint32_t)ADHOC_TASK_GATEWAY_T2_US_OVERRIDE
                        : 57500u;
        cfg.t3_us = 62500u;
        cfg.t4_us = 1937500u;
        cfg.retry_max = 30u;
        cfg.network_window_us = AD_HOC_NETWORK_WINDOW_US;
        cfg.regroup_interval_us = AD_HOC_REGROUP_INTERVAL_US;
        s_retry_max_cfg = cfg.retry_max;
        s_cfg_t5_us = cfg.t1_us + cfg.t2_us;
        if (s_cfg_t5_us == 0u)
        {
            s_cfg_t5_us = 60000u;
        }
        s_cfg_slot_us = s_cfg_t5_us / 16u;
        if (s_cfg_slot_us == 0u)
        {
            s_cfg_slot_us = 3750u;
        }
        adhoc_time_set_lmt_a_t5_us(s_cfg_t5_us);

        if (drv_rtc_is_ready() != 0u)
        {
            if (ADHOC_TASK_FIXED_BDT_SECONDS != 0u)
            {
                drv_rtc_set_bdt_seconds((uint32_t)ADHOC_TASK_FIXED_BDT_SECONDS);
            }
        }

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
        now_us = ad_hoc_task_now_us();
        ad_hoc_task_sync_protocol_time_base(now_us);
        ad_hoc_task_sync_runtime_status(now_us);
        ad_hoc_task_sync_link_status();

        tmos_start_reload_task(s_ad_hoc_task_id, AD_HOC_EVT_POLL, MS1_TO_SYSTEM_TIME(AD_HOC_POLL_MS));
        LOG_PRINT("ad_hoc role map: tg=%u gw0_tg=%u gw1_tg=%u gw1_no=%u\r\n",
                  (unsigned)RF_TG_ID,
                  (unsigned)ADHOC_TASK_GATEWAY_PRIMARY_TG_ID,
                  (unsigned)ADHOC_TASK_GATEWAY_SECONDARY_TG_ID,
                  (unsigned)(ADHOC_TASK_GATEWAY_SECONDARY_NO & 0x07u));
        LOG_PRINT("ad_hoc task started: role=%s domain=%u node=%lu gw_no=%u\r\n",
                  role_gateway != 0u ? "GW" : "BCN",
                  (unsigned)cfg.domain_id,
                  (unsigned long)cfg.node_id,
                  (unsigned)cfg.gateway_no);
        if (role_gateway != 0u && ADHOC_TASK_GATEWAY_T2_US_OVERRIDE != 0u)
        {
            LOG_PRINT("ad_hoc test cfg: gateway t2 override=%luus (retry probe)\r\n",
                      (unsigned long)cfg.t2_us);
        }
        return (events ^ AD_HOC_EVT_INIT);
    }

    if (events & AD_HOC_EVT_POLL)
    {
        if (s_status.inited == 0u)
        {
            return (events ^ AD_HOC_EVT_POLL);
        }

        now_us = ad_hoc_task_now_us();
        ad_hoc_task_sync_protocol_time_base(now_us);
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
