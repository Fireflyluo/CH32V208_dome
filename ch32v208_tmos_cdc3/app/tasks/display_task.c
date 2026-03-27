#include "display_task.h"

#include "OLED.h"
#include "log_print.h"
#include "sensor_task.h"
#include "wchble.h"

#define DISPLAY_EVT_INIT       (0x0001u << 0)
#define DISPLAY_EVT_REFRESH    (0x0001u << 1)

/* 2Hz: 兼顾可读性与 I2C 占用 */
#define DISPLAY_REFRESH_MS     500u

static tmosTaskID s_display_task_id = INVALID_TASK_ID;

static tmosEvents display_task_process_event(tmosTaskID task_id, tmosEvents events);
static void display_render_snapshot(const sensor_snapshot_t *snap);
static void display_show_axis(uint8_t y, const char *name, int32_t mg);
static void display_show_temp_humi(const sensor_snapshot_t *snap);

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

static void display_render_snapshot(const sensor_snapshot_t *snap)
{
    if (snap->ready == 0u)
    {
        OLED_Clear();
        OLED_ShowString(0, 0, "Sensor init", OLED_8X16);
        OLED_Update();
        return;
    }

    OLED_Clear();
    display_show_axis(0, "AX", snap->accel_mg_x);
    display_show_axis(16, "AY", snap->accel_mg_y);
    display_show_axis(32, "AZ", snap->accel_mg_z);
    display_show_temp_humi(snap);
    OLED_Update();
}

static void display_show_axis(uint8_t y, const char *name, int32_t mg)
{
    OLED_ShowString(0, y, (char *)name, OLED_8X16);
    OLED_ShowChar(16, y, ':', OLED_8X16);
    OLED_ShowSignedNum(24, y, mg, 5, OLED_8X16);
    OLED_ShowString(80, y, "mg", OLED_8X16);
}

static void display_show_temp_humi(const sensor_snapshot_t *snap)
{
    int32_t t_int = snap->temp_centi_c / 100;
    uint32_t h_int = (snap->rh_centi_pct >= 0) ? (uint32_t)(snap->rh_centi_pct / 100) : 0u;

    OLED_ShowChar(0, 48, 'T', OLED_8X16);
    OLED_ShowSignedNum(16, 48, t_int, 3, OLED_8X16);
    OLED_ShowChar(56, 48, 'C', OLED_8X16);

    OLED_ShowChar(72, 48, 'H', OLED_8X16);
    OLED_ShowNum(88, 48, h_int, 3, OLED_8X16);
    OLED_ShowChar(120, 48, '%', OLED_8X16);
}

static tmosEvents display_task_process_event(tmosTaskID task_id, tmosEvents events)
{
    sensor_snapshot_t snap;
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
        display_render_snapshot(&snap);
        return (events ^ DISPLAY_EVT_REFRESH);
    }

    return 0;
}
