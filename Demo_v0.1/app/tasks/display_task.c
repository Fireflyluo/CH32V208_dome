#include "display_task.h"

#include "OLED.h"
#include "log_print.h"
#include "protocol_task.h"
#include "sensor_task.h"
#include "wchble.h"

#include <stdio.h>
#include <string.h>

#define DISPLAY_EVT_INIT    (0x0001u << 0)
#define DISPLAY_EVT_REFRESH (0x0001u << 1)

/* 5Hz: 小字信息更密，刷新稍快 */
#define DISPLAY_REFRESH_MS 200u

static tmosTaskID s_display_task_id = INVALID_TASK_ID;

static tmosEvents display_task_process_event(tmosTaskID task_id, tmosEvents events);
static void display_render_snapshot(const sensor_snapshot_t *snap, const protocol_status_t *pst);
static void display_put_line(uint8_t row, const char *text);

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

    OLED_Clear();

    if (snap->ready == 0u)
    {
        display_put_line(0, "Sensor init...");
        OLED_Update();
        return;
    }

    snprintf(line, sizeof(line), "RDY:%u AHz:%u SHz:%u",
             (unsigned)snap->ready,
             (unsigned)snap->accel_hz,
             (unsigned)snap->sht_hz);
    display_put_line(0, line);

    snprintf(line, sizeof(line), "A:%ld,%ld,%ld",
             (long)snap->accel_mg_x,
             (long)snap->accel_mg_y,
             (long)snap->accel_mg_z);
    display_put_line(1, line);

    t_int = snap->temp_centi_c / 100;
    t_frac = snap->temp_centi_c % 100;
    if (t_frac < 0)
    {
        t_frac = -t_frac;
    }
    h_int = snap->rh_centi_pct / 100;

    snprintf(line, sizeof(line), "T:%ld.%02ld H:%ld%%",
             (long)t_int, (long)t_frac, (long)h_int);
    display_put_line(2, line);

    snprintf(line, sizeof(line), "TX:%c SQ:%u",
             (pst->last_tx_type == 0u) ? '-' : (char)pst->last_tx_type,
             (unsigned)pst->last_tx_seq);
    display_put_line(3, line);

    snprintf(line, sizeof(line), "UP:%u %u/%u SM:%u",
             (unsigned)pst->upload_active,
             (unsigned)pst->upload_cursor,
             (unsigned)pst->upload_total,
             (unsigned)pst->sample_count);
    display_put_line(4, line);

    snprintf(line, sizeof(line), "TH:%u-%u A12:%u",
             (unsigned)pst->cfg_threshold_low,
             (unsigned)pst->cfg_threshold_high,
             (unsigned)pst->last_accel12);
    display_put_line(5, line);

    snprintf(line, sizeof(line), "N:%u M:%u I:%u H:%u",
             (unsigned)pst->tx_cnt_n,
             (unsigned)pst->tx_cnt_m,
             (unsigned)pst->tx_cnt_i,
             (unsigned)pst->tx_cnt_h);
    display_put_line(6, line);

    snprintf(line, sizeof(line), "Q:%u T2:%ums OT:%u",
             (unsigned)pst->tx_cnt_q,
             (unsigned)pst->cfg_sample_ms,
             (unsigned)pst->last_over_threshold);
    display_put_line(7, line);

    OLED_Update();
}

static tmosEvents display_task_process_event(tmosTaskID task_id, tmosEvents events)
{
    sensor_snapshot_t snap;
    protocol_status_t pst;

    (void)task_id;

    if (events & DISPLAY_EVT_INIT)
    {
        OLED_Init();
        OLED_Clear();
        OLED_Update();
        tmos_start_reload_task(s_display_task_id, DISPLAY_EVT_REFRESH, MS1_TO_SYSTEM_TIME(DISPLAY_REFRESH_MS));
        return (events ^ DISPLAY_EVT_INIT);
    }

    if (events & DISPLAY_EVT_REFRESH)
    {
        sensor_task_get_snapshot(&snap);
        protocol_task_get_status(&pst);
        display_render_snapshot(&snap, &pst);
        return (events ^ DISPLAY_EVT_REFRESH);
    }

    return 0;
}
