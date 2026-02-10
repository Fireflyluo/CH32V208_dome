/**
 ******************************************************************************
 * @file    board.c
 * @brief   Board specific initialization
 ******************************************************************************
 * @note    本文件内实现与具体开发板以及芯片外设相关的初始化代码
 *
 ******************************************************************************
 */

#include "board.h"
#include "UART.h"
#include "i2c.h"
#include "sc7a20_core.h"
#include "sht40_hal.h"
#include "usb_lib.h"

/* ========================== SysTick定时器初始化 ========================== */

vu32 sys_tick_counter = 0;
uint32_t HAL_GetTick(void)
{
    return sys_tick_counter;
}
// 初始化SysTick定时器
static void SYSTICK_Init_Config(u64 ticks)
{
    SysTick->SR = 0;
    SysTick->CNT = 0;
    SysTick->CMP = ticks;
    SysTick->CTLR = 0xF;

    sys_tick_counter = 0;

    NVIC_SetPriority(SysTicK_IRQn, 1);
    NVIC_EnableIRQ(SysTicK_IRQn);
}

void HAL_Delay(uint32_t Delay)
{
    uint32_t start = HAL_GetTick();
    uint32_t wait = Delay;
    /* Add a freq to guarantee minimum wait */
    if (wait < 0xFFFFFFFFU)
    {
        wait += 1;
    }

    // 处理计数器回绕
    while ((HAL_GetTick() - start) < wait)
    {
    }
}
/* ========================== 板级初始化 ========================== */
static void bsp_i2c_init(void)
{

    i2c_config_t config = I2C_DEFAULT_CONFIG;
    i2c_init(I2C_NUM_1, &config);
    I2C_Scan(I2C1, 1);
}

/*
 * @fn      GPIO_Toggle_INIT
 *
 * @brief   Initializes GPIOA.0
 *
 * @return  none
 */
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
    // 3.初始化I2C
    bsp_i2c_init();
    // 4.初始化spi

    // 5.初始化usb
    Set_USBConfig();
    USB_Init();
    USB_Interrupts_Config();

    // // 6.初始化定时器
    TIM2_Init();
    SYSTICK_Init_Config(SystemCoreClock / 1000 - 1);
    // // 6.外设检查
    accel_init();
    SHT40_Init();
}

// 硬件操作函数实现
static sc7a20_status_t i2c_write_reg(uint8_t reg, const uint8_t *data, uint16_t len)
{
    // 实际实现I2C写操作
    // I2C_Master_Transmit_Polling(0x19 << 1, reg, (uint8_t *)data, len);
    i2c_write_register(I2C_NUM_1, 0x19, reg, (uint8_t *)data, len);

    return SC7A20_OK;
}

static sc7a20_status_t i2c_read_reg(uint8_t reg, uint8_t *data, uint16_t len)
{
    // 实际实现I2C读操作
    // I2C_Master_Receive_Polling(0x19 << 1, reg, (uint8_t *)data, len);
    i2c_read_register(I2C_NUM_1, 0x19, reg, (uint8_t *)data, len);
    return SC7A20_OK;
}

// 硬件操作接口
static sc7a20_ops_t g_accel_ops = {
    .write = i2c_write_reg,
    .read = i2c_read_reg,
    .delay_ms = HAL_Delay,
    .user_data = NULL};

// 静态分配设备结构体
static sc7a20_dev_t g_accel_dev;
// 初始化函数
int accel_init(void)
{
    sc7a20_config_t config = {
        .i2c_addr = SC7A20_I2C_ADDR_H, // 根据硬件连接选择地址
        .range = SC7A20_ACCEL_FS_2G,   // ±4g量程
        .odr = SC7A20_ACCEL_ODR_50HZ,  // 100Hz输出率
        .enable_axis = {1, 1, 1},      // 三轴都使能
        .block_data_update = true,     // 使能块数据更新
        .high_resolution_mode = true,  // 正常分辨率
        .low_power_mode = false        // 正常功耗模式
    };

    sc7a20_status_t status = sc7a20_init(&g_accel_dev, &g_accel_ops, &config);

    if (status != SC7A20_OK)
    {
        printf("SC7A20: ERR!! %d\n", status);
        return -1;
    }

    printf("SC7A20: OK!\n");
    return 0;
}
// 读取加速度数据示例
void read_acceleration_data(void)
{
    sc7a20_accel_data_t accel_data;
    sc7a20_status_t status;

    // 检查数据是否就绪
    bool data_ready = 1;
    // status = sc7a20_is_data_ready(&g_accel_dev, &data_ready);
    status = 0;
    if (status == SC7A20_OK && data_ready)
    {
        // 读取加速度数据
        status = sc7a20_read_acceleration(&g_accel_dev, &accel_data);

        if (status == SC7A20_OK)
        {
            printf("加速度数据: X=%fg, Y=%fg, Z=%fg\n",
                   accel_data.x_g, accel_data.y_g, accel_data.z_g);
            printf("原始数据: X=%d, Y=%d, Z=%d\n",
                   accel_data.x, accel_data.y, accel_data.z);
        }
    }
}

// 读取原始数据示例
void read_raw_data_example(void)
{
    int16_t x, y, z;
    sc7a20_status_t status;

    status = sc7a20_read_raw_data(&g_accel_dev, &x, &y, &z);

    if (status == SC7A20_OK)
    {
        printf("原始数据 - X:%d, Y:%d, Z:%d\n", x, y, z);
    }
}