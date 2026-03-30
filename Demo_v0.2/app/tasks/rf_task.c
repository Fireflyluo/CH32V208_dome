#include "rf_task.h"

#include "HAL.h"
#include "aros_rf.h"
#include "board.h"
#include "log_print.h"
#include "sensor_task.h"
#include "tag.h"
#include "wchble.h"

#include <string.h>

#define RF_EVT_INIT (0x0001u << 0)
#define RF_EVT_POLL (0x0001u << 1)

#define RF_POLL_MS      5u
#define RF_PAYLOAD_TYPE 0xA1u

#ifndef RF_TG_ID
#define RF_TG_ID 0u
#endif

static tmosTaskID s_rf_task_id = INVALID_TASK_ID;
static uint8_t s_tg_id = (uint8_t)RF_TG_ID;
static rf_task_status_t s_status = {0};

static tmosEvents rf_task_process_event(tmosTaskID task_id, tmosEvents events);

static int16_t rf_clamp_i16(int32_t v)
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

static void rf_put_u16(uint8_t *dst, uint16_t v)
{
    dst[0] = (uint8_t)((v >> 8) & 0xFFu);
    dst[1] = (uint8_t)(v & 0xFFu);
}

static void rf_put_u32(uint8_t *dst, uint32_t v)
{
    dst[0] = (uint8_t)((v >> 24) & 0xFFu);
    dst[1] = (uint8_t)((v >> 16) & 0xFFu);
    dst[2] = (uint8_t)((v >> 8) & 0xFFu);
    dst[3] = (uint8_t)(v & 0xFFu);
}

static uint8_t rf_task_sanitize_tg_id(uint8_t tg_id)
{
    if (tg_id > TG_MAX_ID)
    {
        return TG_MAX_ID;
    }
    return tg_id;
}

/* Fill tag.c MSG_DAT(24B):
 * [0]=type [1]=ready
 * [2..15]=ax/ay/az/temp/rh/accel_hz/sht_hz (7 x int16/u16)
 * [16..19]=sht_ok_cnt [20..23]=sht_err_cnt
 */
static int rf_task_fill_tag_data(uint8_t *dat)
{
    sensor_snapshot_t snap;
    int16_t ax;
    int16_t ay;
    int16_t az;
    int16_t t;
    uint16_t rh;

    if (dat == NULL)
    {
        return 0;
    }

    sensor_task_get_snapshot(&snap);
    if (snap.ready == 0u)
    {
        return 0;
    }

    ax = rf_clamp_i16(snap.accel_mg_x);
    ay = rf_clamp_i16(snap.accel_mg_y);
    az = rf_clamp_i16(snap.accel_mg_z);
    t = rf_clamp_i16(snap.temp_centi_c);
    rh = (snap.rh_centi_pct < 0) ? 0u : (uint16_t)snap.rh_centi_pct;

    memset(dat, 0, TG_MSG_DAT_N);
    dat[0] = RF_PAYLOAD_TYPE;
    dat[1] = snap.ready;
    rf_put_u16(&dat[2], (uint16_t)ax);
    rf_put_u16(&dat[4], (uint16_t)ay);
    rf_put_u16(&dat[6], (uint16_t)az);
    rf_put_u16(&dat[8], (uint16_t)t);
    rf_put_u16(&dat[10], rh);
    rf_put_u16(&dat[12], snap.accel_hz);
    rf_put_u16(&dat[14], snap.sht_hz);
    rf_put_u32(&dat[16], snap.sht_ok_cnt);
    rf_put_u32(&dat[20], snap.sht_err_cnt);
    return 1;
}

static void rf_task_sync_status(void)
{
    tg_runtime_status_t tg_st;

    tg_get_status(&tg_st);
    s_status.tg_id = tg_st.tg_id;
    s_status.synced = tg_st.synced;
    s_status.tx_cnt = tg_st.tx_cnt;
    s_status.rx_cnt = tg_st.rx_cnt;
    s_status.last_rssi = tg_st.last_rssi;
    s_status.last_rx_tc = tg_st.last_rx_tc;
}

void rf_task_init(void)
{
    if (s_rf_task_id != INVALID_TASK_ID)
    {
        return;
    }

    s_rf_task_id = TMOS_ProcessEventRegister(rf_task_process_event);
    if (s_rf_task_id == INVALID_TASK_ID)
    {
        LOG_PRINT("rf task register failed\r\n");
        return;
    }

    s_tg_id = rf_task_sanitize_tg_id(s_tg_id);
    memset(&s_status, 0, sizeof(s_status));
    s_status.tg_id = s_tg_id;
    tmos_set_event(s_rf_task_id, RF_EVT_INIT);
}

void rf_task_set_tg_id(uint8_t tg_id)
{
    uint8_t next = rf_task_sanitize_tg_id(tg_id);

    if (next == s_tg_id)
    {
        return;
    }

    s_tg_id = next;
    s_status.tg_id = s_tg_id;
    if (s_status.rf_inited != 0u)
    {
        tg_set_id(s_tg_id);
    }
    LOG_PRINT("rf tg_id updated: %u\r\n", (unsigned)s_tg_id);
}

uint8_t rf_task_get_tg_id(void)
{
    return s_tg_id;
}

void rf_task_get_status(rf_task_status_t *out)
{
    if (out == NULL)
    {
        return;
    }
    *out = s_status;
}

static tmosEvents rf_task_process_event(tmosTaskID task_id, tmosEvents events)
{
    (void)task_id;

    if (events & RF_EVT_INIT)
    {
        RF_RoleInit();
        arf_Init();
        tg_init(s_tg_id);
        tg_set_newdatfunc(rf_task_fill_tag_data);
        s_status.rf_inited = 1u;
        rf_task_sync_status();

        tmos_start_reload_task(s_rf_task_id, RF_EVT_POLL, MS1_TO_SYSTEM_TIME(RF_POLL_MS));
        LOG_PRINT("rf tag-sm started: tg_id=%u\r\n", (unsigned)s_tg_id);
        return (events ^ RF_EVT_INIT);
    }

    if (events & RF_EVT_POLL)
    {
        tg_step();
        rf_task_sync_status();
        return (events ^ RF_EVT_POLL);
    }

    return 0;
}
