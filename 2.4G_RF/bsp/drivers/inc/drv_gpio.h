/**
 ******************************************************************************
 * @file    drv_gpio.h
 * @brief   GPIO驱动头文件
 ******************************************************************************
 * @details 本文件定义了CH32V208微控制器的GPIO驱动API接口，提供统一的引脚操作接口。
 *          支持RT-Thread风格的引脚编号方式，便于跨平台移植。
 *          
 *          主要功能：
 *          - 引脚模式配置（输入/输出/上拉/下拉/开漏）
 *          - 引脚电平读写和翻转
 *          - 外部中断配置和管理
 *          - 引脚名称查询
 *          
 *          使用方法：
 *          1. 调用gpio_init()初始化GPIO驱动
 *          2. 使用GET_PIN宏获取逻辑引脚号（如GET_PIN(A, 5)表示PA5）
 *          3. 调用gpio_mode()配置引脚模式
 *          4. 使用gpio_write()/gpio_read()进行引脚操作
 *          5. 如需外部中断，调用gpio_attach_irq()和gpio_irq_enable()
 *
 ******************************************************************************
 */
#ifndef __DRV_GPIO_H__
#define __DRV_GPIO_H__

#include <stdint.h>
#include <stdbool.h>
#include "ch32v20x.h"
#ifdef __cplusplus
extern "C" {
#endif

/* 使用RT-Thread风格的宏定义 */
#define __CH32_PORT(port)   GPIO##port##_BASE
#define GET_PIN(PORTx, PIN) (uint16_t)((16 * (((uint16_t)__CH32_PORT(PORTx) - (uint16_t)GPIOA_BASE) / (0x0400UL))) + PIN)

/* 基础类型定义 */
typedef int32_t gpio_err_t;         ///< GPIO操作返回错误码类型
typedef uint16_t gpio_pin_t;        ///< GPIO引脚类型（使用uint16_t兼容GET_PIN宏）

/* 错误码定义 */
#define GPIO_OK     0               ///< 操作成功
#define GPIO_ERROR  -1              ///< 通用错误
#define GPIO_EINVAL -2              ///< 无效参数
#define GPIO_ERANGE -3              ///< 参数超出范围

/* 引脚电平定义 */
typedef enum {
    GPIO_PIN_RESET = 0U,            ///< 引脚低电平
    GPIO_PIN_SET                    ///< 引脚高电平
} DRV_GPIO_PinState;

/* 引脚模式定义 */
typedef enum {
    PIN_MODE_INPUT = 0,             ///< 输入模式（默认，内部下拉）
    PIN_MODE_OUTPUT,                ///< 输出模式（推挽输出）
    PIN_MODE_INPUT_PULLUP,          ///< 输入模式，内部上拉
    PIN_MODE_INPUT_PULLDOWN,        ///< 输入模式，内部下拉
    PIN_MODE_OUTPUT_OD,             ///< 输出模式，开漏输出
} pin_mode_t;

/* 中断模式定义 */
typedef enum {
    PIN_IRQ_MODE_RISING = 0,        ///< 上升沿触发
    PIN_IRQ_MODE_FALLING,           ///< 下降沿触发
    PIN_IRQ_MODE_RISING_FALLING,    ///< 双边沿触发
} pin_irq_mode_t;

/* 中断使能定义 */
typedef enum {
    GPIO_IRQ_DISABLE = 0,           ///< 禁用中断
    GPIO_IRQ_ENABLE  = 1            ///< 使能中断
} gpio_irq_enable_t;

/* 引脚索引结构 - 适配新的宏定义 */
struct gpio_pin_index {
    gpio_pin_t logical_pin;         ///< 逻辑引脚号（由GET_PIN宏生成）
    GPIO_TypeDef *gpio;             ///< GPIO端口指针
    uint16_t pin_bit;               ///< 引脚位掩码
    const char *name;               ///< 引脚名称（可选，如"PA0"）
};

/* API函数声明 */

/**
 * @brief  GPIO驱动初始化
 * @details 初始化GPIO驱动，使能所有GPIO端口时钟。
 * @return 操作结果（GPIO_OK表示成功）
 */
gpio_err_t gpio_init(void);

/**
 * @brief  配置GPIO引脚模式
 * @details 配置指定引脚的工作模式（输入/输出/上拉/下拉/开漏）。
 * @param[in] pin 逻辑引脚号（使用GET_PIN宏获取）
 * @param[in] mode 引脚模式
 * @return 操作结果
 */
gpio_err_t gpio_mode(gpio_pin_t pin, pin_mode_t mode);

/**
 * @brief  写GPIO引脚电平
 * @details 设置指定引脚的输出电平。
 * @param[in] pin 逻辑引脚号
 * @param[in] value 电平值（GPIO_PIN_RESET/GPIO_PIN_SET）
 */
void gpio_write(gpio_pin_t pin, DRV_GPIO_PinState value);

/**
 * @brief  读GPIO引脚电平
 * @details 读取指定引脚的当前电平状态。
 * @param[in] pin 逻辑引脚号
 * @return 引脚电平状态
 */
DRV_GPIO_PinState gpio_read(gpio_pin_t pin);

/**
 * @brief  翻转GPIO引脚电平
 * @details 切换指定引脚的输出电平状态（高变低，低变高）。
 * @param[in] pin 逻辑引脚号
 * @return 操作结果
 */
gpio_err_t gpio_toggle(gpio_pin_t pin);

/**
 * @brief  绑定GPIO中断处理函数
 * @details 为指定引脚注册中断处理回调函数，但不立即使能中断。
 * @param[in] pin 逻辑引脚号
 * @param[in] mode 中断触发模式
 * @param[in] hdr 中断处理回调函数指针
 * @param[in] args 传递给回调函数的参数
 * @return 操作结果
 */
gpio_err_t gpio_attach_irq(gpio_pin_t pin, pin_irq_mode_t mode,
                           void (*hdr)(void *args), void *args);

/**
 * @brief  解绑GPIO中断处理函数
 * @details 移除指定引脚的中断处理回调函数。
 * @param[in] pin 逻辑引脚号
 * @return 操作结果
 */
gpio_err_t gpio_detach_irq(gpio_pin_t pin);

/**
 * @brief  使能/禁用GPIO中断
 * @details 使能或禁用指定引脚的外部中断功能。
 * @param[in] pin 逻辑引脚号
 * @param[in] enabled 使能状态（GPIO_IRQ_ENABLE/GPIO_IRQ_DISABLE）
 * @return 操作结果
 */
gpio_err_t gpio_irq_enable(gpio_pin_t pin, gpio_irq_enable_t enabled);

/**
 * @brief  获取引脚名称
 * @details 获取指定逻辑引脚号对应的字符串名称（如"PA0"）。
 * @param[in] pin 逻辑引脚号
 * @return 引脚名称字符串
 */
const char *gpio_get_pin_name(gpio_pin_t pin);

#ifdef __cplusplus
}
#endif

#endif /* __DRV_GPIO_H__ */