/**
 ******************************************************************************
 * @file    board.c
 * @brief   Board specific initialization
 ******************************************************************************
 */

#include "board.h"
#include "OLED.h"
#include "drv_i2c.h"
#include "i2c_bus_arbiter.h"
#include "drv_tim.h"
#include "usb_lib.h"

static void oled_test(void);

/* I2C1 传输模式开关：I2C_MODE_IT / I2C_MODE_DMA */
#ifndef BOARD_I2C1_TRANSFER_MODE
#define BOARD_I2C1_TRANSFER_MODE I2C_MODE_DMA
#endif

uint32_t HAL_GetTick(void)
{
    return drv_tim_get_tick_ms();
}

void HAL_Delay(uint32_t Delay)
{
    drv_tim_delay_ms(Delay);
}

vu8 tx_flag = 0;
void i2c_master_tx_cplt_callback(i2c_num_t i2c_num)
{
    tx_flag = 0;
    (void)i2c_num;
}

void i2c_master_rx_cplt_callback(i2c_num_t i2c_num)
{
    (void)i2c_num;
}

void i2c_master_error_callback(i2c_num_t i2c_num, uint32_t error_code)
{
    (void)i2c_num;
    (void)error_code;
    tx_flag = 0;
}

static void board_i2c_init(void)
{
    bsp_i2c_config_t config = {
        .clock_speed = 100000,
        .duty_cycle = I2C_DutyCycle_16_9,
        .own_address = 0,
        .enable_ack = true,
        .is_7_bit_address = true,
        .mode = BOARD_I2C1_TRANSFER_MODE,
    };

    bsp_i2c_init(I2C_NUM_1, &config);
#if (BOARD_I2C1_TRANSFER_MODE == I2C_MODE_DMA)
    /* CH32V20x: I2C1_TX=DMA1_CH6, I2C1_RX=DMA1_CH7 */
    bsp_i2c_dma_init(I2C_NUM_1, DMA1_Channel6, DMA1_Channel7);

    NVIC_InitTypeDef nvic_init = {0};
    nvic_init.NVIC_IRQChannelPreemptionPriority = 1;
    nvic_init.NVIC_IRQChannelSubPriority = 2;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;

    nvic_init.NVIC_IRQChannel = DMA1_Channel6_IRQn;
    NVIC_Init(&nvic_init);
    nvic_init.NVIC_IRQChannel = DMA1_Channel7_IRQn;
    NVIC_Init(&nvic_init);
#endif

    bsp_i2c_register_tx_callback(I2C_NUM_1, i2c_master_tx_cplt_callback);
    bsp_i2c_register_rx_callback(I2C_NUM_1, i2c_master_rx_cplt_callback);
    bsp_i2c_register_error_callback(I2C_NUM_1, i2c_master_error_callback);

    i2c_bus_arbiter_init(I2C_NUM_1);
}

static void GPIO_Toggle_INIT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

void board_init(void)
{
    GPIO_Toggle_INIT();
    board_i2c_init();

    Set_USBConfig();
    USB_Init();
    USB_Interrupts_Config();

    drv_tim_init(1000);
    oled_test();
}

static void oled_test(void)
{
    OLED_Init();
    OLED_ShowChar(0, 0, 'A', OLED_8X16);
    OLED_ShowString(16, 0, "Hello World!", OLED_8X16);
    OLED_Update();
}
