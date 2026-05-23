#include "drv_rtc.h"

#include "debug.h"
#include "ch32v20x_rtc.h"
#include "ch32v20x_rcc.h"

#define DRV_RTC_COUNTER_HZ_LSI 16000u
#define DRV_RTC_COUNTER_HZ_LSE 16384u
#define DRV_RTC_RTCSRC_MASK 0x00000300u

/* TMOS/HAL 使用 RTC Counter 作为系统时基，频率为 CAB_LSIFQ / 2 */
#define DRV_RTC_COUNTER_HZ DRV_RTC_COUNTER_HZ_LSI

static volatile uint8_t  g_rtc_ready = 0u;
static volatile uint32_t g_tick_hz = DRV_RTC_COUNTER_HZ;
static volatile uint8_t  g_using_lse = 0u;
static volatile uint32_t g_tick_anchor = 0u;
static volatile uint64_t g_bdt_anchor_us = 0u;

static uint32_t drv_rtc_detect_counter_hz(void)
{
    uint32_t rtc_src = RCC->BDCTLR & DRV_RTC_RTCSRC_MASK;
    if (rtc_src == RCC_RTCCLKSource_LSE)
    {
        g_using_lse = 1u;
        return DRV_RTC_COUNTER_HZ_LSE;
    }

    g_using_lse = 0u;
    return DRV_RTC_COUNTER_HZ_LSI;
}

static uint64_t drv_rtc_calc_bdt_us_from_ticks(uint32_t tick_now)
{
    uint32_t delta_tick = tick_now - g_tick_anchor;
    uint64_t delta_us = ((uint64_t)delta_tick * 1000000ull) / (uint64_t)g_tick_hz;
    return g_bdt_anchor_us + delta_us;
}

void drv_rtc_get_default_init_cfg(drv_rtc_init_cfg_t *out_cfg)
{
    if (out_cfg == 0)
    {
        return;
    }
    out_cfg->clock_source = DRV_RTC_CLK_AUTO;
    out_cfg->lse_timeout_ms = 0u;
    out_cfg->lsi_timeout_ms = 0u;
    out_cfg->keep_time_if_configured = 1u;
    out_cfg->force_reinit = 0u;
    out_cfg->default_bdt_seconds = 0u;
}

int drv_rtc_init(const drv_rtc_init_cfg_t *cfg)
{
    drv_rtc_init_cfg_t local_cfg;
    uint32_t now_tick;

    drv_rtc_get_default_init_cfg(&local_cfg);
    if (cfg != 0)
    {
        local_cfg = *cfg;
    }

    g_tick_hz = drv_rtc_detect_counter_hz();
    if (g_tick_hz == 0u)
    {
        g_tick_hz = DRV_RTC_COUNTER_HZ_LSI;
    }

    now_tick = RTC_GetCounter();
    g_tick_anchor = now_tick;
    g_bdt_anchor_us = (uint64_t)local_cfg.default_bdt_seconds * 1000000ull;
    g_rtc_ready = 1u;
    return 0;
}

uint8_t drv_rtc_is_ready(void)
{
    return g_rtc_ready;
}

uint8_t drv_rtc_is_using_lse(void)
{
    return g_using_lse;
}

uint32_t drv_rtc_get_bdt_seconds(void)
{
    uint64_t bdt_us;

    if (g_rtc_ready == 0u)
    {
        return 0u;
    }
    bdt_us = drv_rtc_calc_bdt_us_from_ticks(RTC_GetCounter());
    return (uint32_t)(bdt_us / 1000000ull);
}

void drv_rtc_set_bdt_seconds(uint32_t bdt_seconds)
{
    drv_rtc_set_bdt_time_us(bdt_seconds, 0u);
}

void drv_rtc_set_bdt_time_us(uint32_t bdt_seconds, uint32_t sub_us)
{
    if (g_rtc_ready == 0u)
    {
        return;
    }
    if (sub_us >= 1000000u)
    {
        bdt_seconds += (sub_us / 1000000u);
        sub_us %= 1000000u;
    }
    g_tick_anchor = RTC_GetCounter();
    g_bdt_anchor_us = ((uint64_t)bdt_seconds * 1000000ull) + (uint64_t)sub_us;
}

uint64_t drv_rtc_get_bdt_time_us(void)
{
    if (g_rtc_ready == 0u)
    {
        return 0u;
    }
    return drv_rtc_calc_bdt_us_from_ticks(RTC_GetCounter());
}

void drv_rtc_sync_subsecond_base(void)
{
    uint64_t bdt_us;

    if (g_rtc_ready == 0u)
    {
        return;
    }
    bdt_us = drv_rtc_calc_bdt_us_from_ticks(RTC_GetCounter());
    g_tick_anchor = RTC_GetCounter();
    g_bdt_anchor_us = bdt_us;
}

uint32_t drv_rtc_bdt_to_unix_seconds(uint32_t bdt_seconds)
{
    return bdt_seconds + DRV_RTC_BDT_EPOCH_UNIX_SEC;
}

uint32_t drv_rtc_unix_to_bdt_seconds(uint32_t unix_seconds)
{
    if (unix_seconds <= DRV_RTC_BDT_EPOCH_UNIX_SEC)
    {
        return 0u;
    }
    return unix_seconds - DRV_RTC_BDT_EPOCH_UNIX_SEC;
}
