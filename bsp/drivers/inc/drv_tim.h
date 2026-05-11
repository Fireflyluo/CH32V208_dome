/**
 ******************************************************************************
 * @file    drv_tim.h
 * @brief   定时器驱动头文件
 ******************************************************************************
 * @details 本文件定义了基于TIM2的毫秒级定时器驱动API接口，提供系统时间基准和延时功能。
 *          TIM2被配置为1ms中断周期，用于提供精确的毫秒计时。
 *          
 *          主要功能：
 *          - 系统毫秒计时（drv_tim_get_tick_ms）
 *          - 毫秒级阻塞延时（drv_tim_delay_ms）
 *          - 软件定时器支持（通过sw_timer_wheel_init集成）
 *          
 *          使用说明：
 *          1. 调用drv_tim_init()初始化定时器（通常在board_init中完成）
 *          2. 使用drv_tim_get_tick_ms()获取系统运行时间
 *          3. 使用drv_tim_delay_ms()执行精确延时
 *          4. 软件定时器功能通过sw_timer库提供，由定时器中断自动维护
 *
 ******************************************************************************
 */
#ifndef __DRV_TIM_H
#define __DRV_TIM_H

#include "ch32v20x.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  初始化毫秒定时器
 * @details 配置TIM2为指定频率的周期性中断，提供系统毫秒计时基准。
 *          同时初始化软件定时器轮（如果使能）。
 * @param[in] tick_hz 定时器中断频率（Hz），通常设为1000表示1ms中断
 */
void drv_tim_init(uint32_t tick_hz);

/**
 * @brief  定时器中断处理函数
 * @details 处理TIM2的更新中断，更新系统毫秒计数并触发软件定时器处理。
 *          此函数应在TIM2_IRQn中断服务程序中调用。
 */
void drv_tim_irq_handler(void);

/**
 * @brief  获取系统毫秒计数
 * @details 返回自系统启动以来经过的毫秒数，用于时间测量和超时判断。
 * @return 当前系统毫秒计数
 */
uint32_t drv_tim_get_tick_ms(void);

/**
 * @brief  获取系统微秒时间戳
 * @details 基于TIM2当前计数值和毫秒tick组合得到单调递增时间。
 *          该接口用于高频采样的时间戳记录，避免仅毫秒分辨率带来的误差。
 * @return 当前系统运行时间（微秒）
 */
uint64_t drv_tim_get_time_us(void);

/**
 * @brief  毫秒级阻塞延时
 * @details 执行指定毫秒数的阻塞延时，期间CPU不执行其他任务。
 * @param[in] ms 延时毫秒数
 */
void drv_tim_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* __DRV_TIM_H */
