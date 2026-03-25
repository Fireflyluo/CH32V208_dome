/**
 ******************************************************************************
 * @file    board.c
 * @brief   Board specific initialization
 ******************************************************************************
 */

#include "board.h"
#include "OLED.h"
#include "drv_i2c.h"
#include "drv_tim.h"
#include "sc7a20_platform.h"
#include "sht40_hal.h"
#include "usb_lib.h"

static void oled_test(void);

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
#if SC7A20_ASYNC_SUPPORT
    accel_adapter_i2c_tx_cplt_callback(i2c_num);
#else
    (void)i2c_num;
#endif
}

void i2c_master_rx_cplt_callback(i2c_num_t i2c_num)
{
#if SC7A20_ASYNC_SUPPORT
    accel_adapter_i2c_rx_cplt_callback(i2c_num);
#else
    (void)i2c_num;
#endif
}

void i2c_master_error_callback(i2c_num_t i2c_num, uint32_t error_code)
{
#if SC7A20_ASYNC_SUPPORT
    accel_adapter_i2c_error_callback(i2c_num, error_code);
#else
    (void)i2c_num;
    (void)error_code;
#endif
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
        .mode = I2C_MODE_DMA,
    };

    bsp_i2c_init(I2C_NUM_1, &config);
    bsp_i2c_register_tx_callback(I2C_NUM_1, i2c_master_tx_cplt_callback);
    bsp_i2c_register_rx_callback(I2C_NUM_1, i2c_master_rx_cplt_callback);
    bsp_i2c_register_error_callback(I2C_NUM_1, i2c_master_error_callback);
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

    accel_init();
    SHT40_Init();
    oled_test();
}

static void oled_test(void)
{
    OLED_Init();
    OLED_ShowChar(0, 0, 'A', OLED_8X16);
    OLED_ShowString(16, 0, "Hello World!", OLED_8X16);
    OLED_Update();
}
