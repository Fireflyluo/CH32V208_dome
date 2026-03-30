#include "display_task.h"

#include "OLED.h"
#include "log_print.h"
#include "protocol_task.h"
#include "rf_task.h"
#include "sensor_task.h"
#include "wchble.h"

#include <stdio.h>
#include <string.h>

#define DISPLAY_EVT_INIT    (0x0001u << 0)
#define DISPLAY_EVT_RENDER  (0x0001u << 1)
#define DISPLAY_EVT_FLUSH   (0x0001u << 2)

/* 渲染频率与刷屏分离：降低单次I2C阻塞时间 */
#define DISPLAY_RENDER_MS   400u
#define DISPLAY_FLUSH_MS    20u
#define DISPLAY_TILE_WIDTH  32u

static tmosTaskID s_display_task_id = INVALID_TASK_ID;
static uint8_t s_flush_page = 0u;
static uint8_t s_flush_x = 0u;

static tmosEvents display_task_process_event(tmosTaskID task_id, tmosEvents events);
static void display_render_snapshot(const sensor_snapshot_t *snap, const protocol_status_t *pst);
static void display_put_line(uint8_t row, const char *text);
static void display_flush_next_tile(void);

void display_task_init(void)
{
    if (s_display_task_id != INVALID_TASK_ID)
    {
        return;
    }

    s_display_task_id = TMOS_ProcessEventRegister(display_task_process_event);
    if (s_display_task_id == INVALID_TASK_ID)
    {
        LOG_PRINT("display task register failed\r\n");
        return;
    }

    tmos_set_event(s_display_task_id, DISPLAY_EVT_INIT);
}

static void display_put_line(uint8_t row, const char *text)
{
    uint8_t y = (uint8_t)(row * 8u);
    OLED_ShowString(0, y, (char *)text, OLED_6X8);
}

static void display_render_snapshot(const sensor_snapshot_t *snap, const protocol_status_t *pst)
{
    char line[32];
    int32_t t_int;
    int32_t t_frac;
    int32_t h_int;
    rf_task_status_t rf_st;

    OLED_Clear();

    rf_task_get_status(&rf_st);

    if (snap->ready == 0u)
    {
        display_put_line(0, "Sensor init...");
        snprintf(line, sizeof(line), "TG:%u RF:%u",
                 (unsigned)rf_st.tg_id,
                 (unsigned)rf_st.synced);
        display_put_line(1, line);
        return;
    }

    snprintf(line, sizeof(line), "TG:%u RF:%u A:%u S:%u",
             (unsigned)rf_st.tg_id,
             (unsigned)rf_st.synced,
             (unsigned)snap->accel_hz,
             (unsigned)snap->sht_hz);
    display_put_line(0, line);

    snprintf(line, sizeof(line), "RFTX:%lu RFRX:%lu",
             (unsigned long)rf_st.tx_cnt,
             (unsigned long)rf_st.rx_cnt);
    display_put_line(1, line);

    snprintf(line, sizeof(line), "A:%ld,%ld,%ld",
             (long)snap->accel_mg_x,
             (long)snap->accel_mg_y,
             (long)snap->accel_mg_z);
    display_put_line(2, line);

    t_int = snap->temp_centi_c / 100;
    t_frac = snap->temp_centi_c % 100;
    if (t_frac < 0)
    {
        t_frac = -t_frac;
    }
    h_int = snap->rh_centi_pct / 100;

    snprintf(line, sizeof(line), "T:%ld.%02ld H:%ld%%",
             (long)t_int, (long)t_frac, (long)h_int);
    display_put_line(3, line);

    snprintf(line, sizeof(line), "TX:%c SQ:%u Q:%u",
             (pst->last_tx_type == 0u) ? '-' : (char)pst->last_tx_type,
             (unsigned)pst->last_tx_seq,
             (unsigned)pst->tx_cnt_q);
    display_put_line(4, line);

    snprintf(line, sizeof(line), "UP:%u %u/%u O:%u",
             (unsigned)pst->upload_active,
             (unsigned)pst->upload_cursor,
             (unsigned)pst->upload_total,
             (unsigned)pst->last_over_threshold);
    display_put_line(5, line);

    snprintf(line, sizeof(line), "TH:%u-%u A:%u",
             (unsigned)pst->cfg_threshold_low,
             (unsigned)pst->cfg_threshold_high,
             (unsigned)pst->last_accel12);
    display_put_line(6, line);

    snprintf(line, sizeof(line), "N:%u M:%u I:%u H:%u",
             (unsigned)pst->tx_cnt_n,
             (unsigned)pst->tx_cnt_m,
             (unsigned)pst->tx_cnt_i,
             (unsigned)pst->tx_cnt_h);
    display_put_line(7, line);

}

static void display_flush_next_tile(void)
{
    OLED_UpdateArea(s_flush_x, (uint8_t)(s_flush_page * 8u), DISPLAY_TILE_WIDTH, 8u);

    s_flush_x = (uint8_t)(s_flush_x + DISPLAY_TILE_WIDTH);
    if (s_flush_x >= 128u)
    {
        s_flush_x = 0u;
        s_flush_page++;
        if (s_flush_page >= 8u)
        {
            s_flush_page = 0u;
        }
    }
}

static tmosEvents display_task_process_event(tmosTaskID task_id, tmosEvents events)
{
    sensor_snapshot_t snap;
    protocol_status_t pst;
    tmosEvents remain = events;

    (void)task_id;

    if (remain & DISPLAY_EVT_INIT)
    {
        OLED_Init();
        OLED_Clear();
        OLED_Update();
        tmos_start_reload_task(s_display_task_id, DISPLAY_EVT_RENDER, MS1_TO_SYSTEM_TIME(DISPLAY_RENDER_MS));
        tmos_start_reload_task(s_display_task_id, DISPLAY_EVT_FLUSH, MS1_TO_SYSTEM_TIME(DISPLAY_FLUSH_MS));
        tmos_set_event(s_display_task_id, DISPLAY_EVT_RENDER);
        remain ^= DISPLAY_EVT_INIT;
    }

    if (remain & DISPLAY_EVT_RENDER)
    {
        sensor_task_get_snapshot(&snap);
        protocol_task_get_status(&pst);
        display_render_snapshot(&snap, &pst);
        remain ^= DISPLAY_EVT_RENDER;
    }

    if (remain & DISPLAY_EVT_FLUSH)
    {
        display_flush_next_tile();
        remain ^= DISPLAY_EVT_FLUSH;
    }

    return remain;
}
