/**
 ******************************************************************************
 * @file    drv_rtc.h
 * @brief   RTC 驱动接口（BDT 秒计时基线）
 ******************************************************************************
 * @details
 * TMOS/HAL 会占用 RTC 计数器作为系统时基，本驱动不改 RTC 硬件配置。
 * 本驱动只做“RTC tick -> BDT 时间（2006-01-01 起）”的软件映射。
 * 协议时间戳生成建议使用 BDT 语义，避免与上位机/网关时间定义偏差。
 ******************************************************************************
 */
#ifndef __DRV_RTC_H
#define __DRV_RTC_H

#include "ch32v20x.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** BDT 起点对应 Unix 时间戳（2006-01-01 00:00:00 UTC） */
#define DRV_RTC_BDT_EPOCH_UNIX_SEC 1136073600u

typedef enum
{
    DRV_RTC_CLK_AUTO = 0u, /**< 优先 LSE，失败回退 LSI */
    DRV_RTC_CLK_LSE  = 1u, /**< 强制 LSE */
    DRV_RTC_CLK_LSI  = 2u  /**< 强制 LSI */
} drv_rtc_clock_source_t;

typedef struct
{
    drv_rtc_clock_source_t clock_source;            /**< 预留字段：由系统全局配置决定 */
    uint32_t               lse_timeout_ms;          /**< 预留字段 */
    uint32_t               lsi_timeout_ms;          /**< 预留字段 */
    uint8_t                keep_time_if_configured; /**< 预留字段 */
    uint8_t                force_reinit;            /**< 预留字段 */
    uint32_t               default_bdt_seconds;     /**< 软件映射初始 BDT 秒值 */
} drv_rtc_init_cfg_t;

void drv_rtc_get_default_init_cfg(drv_rtc_init_cfg_t *out_cfg);
int drv_rtc_init(const drv_rtc_init_cfg_t *cfg);

uint8_t drv_rtc_is_ready(void);
uint8_t drv_rtc_is_using_lse(void);

uint32_t drv_rtc_get_bdt_seconds(void);
void drv_rtc_set_bdt_seconds(uint32_t bdt_seconds);
void drv_rtc_set_bdt_time_us(uint32_t bdt_seconds, uint32_t sub_us);
uint64_t drv_rtc_get_bdt_time_us(void);
void drv_rtc_sync_subsecond_base(void);

uint32_t drv_rtc_bdt_to_unix_seconds(uint32_t bdt_seconds);
uint32_t drv_rtc_unix_to_bdt_seconds(uint32_t unix_seconds);

#ifdef __cplusplus
}
#endif

#endif /* __DRV_RTC_H */
