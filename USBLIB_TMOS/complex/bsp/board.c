/**
 ******************************************************************************
 * @file    board.c
 * @brief   板级外设初始化实现
 ******************************************************************************
 * @details 初始化顺序：
 *          1) 基础系统：中断分组、时钟、延时、调试串口、毫秒定时器
 *          2) I2C1 与 DMA 通道
 *          3) I2C 总线仲裁器
 *          4) USB CDC
 ******************************************************************************
 */

#include "board.h"

#include "drv_i2c.h"
#include "drv_tim.h"
#include "i2c_bus_arbiter.h"
#include "usb_cdc.h"
#include "hw_config.h"
#include "usb_core.h"
#include "usb_init.h"

/* 提供给 HAL 的毫秒计时接口 */
uint32_t HAL_GetTick(void)
{
    return drv_tim_get_tick_ms();
}

/* 提供给 HAL 的阻塞延时接口 */
void HAL_Delay(uint32_t ms)
{
    drv_tim_delay_ms(ms);
}

/* 板级初始化入口 */
void board_init(void)
{
    bsp_i2c_config_t i2c_cfg;
    NVIC_InitTypeDef nvic_init = {0};

    /* 基础系统初始化 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);
    drv_tim_init(1000u);

    /* I2C1 初始化（默认 IT，DMA 用于大块传输） */
    i2c_cfg.clock_speed = 100000u;
    i2c_cfg.duty_cycle = I2C_DutyCycle_16_9;
    i2c_cfg.own_address = 0u;
    i2c_cfg.enable_ack = true;
    i2c_cfg.is_7_bit_address = true;
    i2c_cfg.mode = I2C_MODE_IT;
    (void)bsp_i2c_init(I2C_NUM_1, &i2c_cfg);
    bsp_i2c_dma_init(I2C_NUM_1, DMA1_Channel6, DMA1_Channel7);

    /* DMA 中断：CH6=I2C1_TX, CH7=I2C1_RX */
    nvic_init.NVIC_IRQChannelPreemptionPriority = 1;
    nvic_init.NVIC_IRQChannelSubPriority = 2;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    nvic_init.NVIC_IRQChannel = DMA1_Channel6_IRQn;
    NVIC_Init(&nvic_init);
    nvic_init.NVIC_IRQChannel = DMA1_Channel7_IRQn;
    NVIC_Init(&nvic_init);

    /* I2C 请求仲裁器 */
    i2c_bus_arbiter_init(I2C_NUM_1);

    /* USB CDC */
    Set_USBConfig();
    USB_Init();
    USB_Interrupts_Config();
    CDC_VirtualUartInit();
}
