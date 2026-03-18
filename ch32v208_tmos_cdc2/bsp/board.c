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
#include "OLED.h"
#include "UART.h"
#include "drv_i2c.h"
#include "sc7a20_core.h"
#include "sht40_hal.h"
#include "usb_lib.h"

/* ========================== SysTick定时器初始化 ========================== */
static void oled_test(void);

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

vu8 tx_flag = 0;
void i2c_master_tx_cplt_callback(i2c_num_t i2c_num)
{
    tx_flag = 0;
}

/* ========================== 板级初始化 ========================== */
static void board_i2c_init(void)
{
    bsp_i2c_config_t config = {
        .clock_speed = 400000, // 400kHz
        .duty_cycle = I2C_DutyCycle_2,
        .own_address = 0,
        .enable_ack = true,
        .is_7_bit_address = true,
        .mode = I2C_MODE_IT // 使用中断模式
    };
    bsp_i2c_init(I2C_NUM_1, &config);

    // 注册发送完成回调函数
    bsp_i2c_register_tx_callback(I2C_NUM_1, i2c_master_tx_cplt_callback);
    //     I2C_Scan(I2C1, 1);
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
// uint8_t test_id = 0;
void board_init(void)
{

    GPIO_Toggle_INIT();
    // 3.初始化I2C
    board_i2c_init();
    // 4.初始化spi

    // 5.初始化usb
    Set_USBConfig();
    USB_Init();
    USB_Interrupts_Config();

    // // 6.初始化定时器
    TIM2_Init();
    //    SYSTICK_Init_Config(SystemCoreClock / 1000 - 1);
    // 6.外设检查

    // i2c_read_register_async(I2C_NUM_1, 0x19, 0x0F, &test_id, 1, accel_read_callback);
    // accel_init();
    // SHT40_Init();
    oled_test();
}

// 硬件操作函数实现
static sc7a20_status_t i2c_write_reg(uint8_t reg, const uint8_t *data, uint16_t len)
{
    // 使用新的 HAL 风格 API - 中断模式
    tx_flag = 1;
    bsp_i2c_write_register_it(I2C_NUM_1, 0x19, reg, (uint8_t *)data, len);
    while (tx_flag == 1)
        ;
    return SC7A20_OK;
}

static sc7a20_status_t i2c_read_reg(uint8_t reg, uint8_t *data, uint16_t len)
{
    // 使用新的 HAL 风格 API - 中断模式
    bsp_i2c_read_register_it(I2C_NUM_1, 0x19, reg, (uint8_t *)data, len);
    return SC7A20_OK;
}

// 硬件操作接口
static sc7a20_ops_t g_accel_ops = {
    .write = i2c_write_reg, .read = i2c_read_reg, .delay_ms = Delay_Ms, .user_data = NULL};

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
            printf("加速度数据: X=%fg, Y=%fg, Z=%fg\n", accel_data.x_g, accel_data.y_g, accel_data.z_g);
            printf("原始数据: X=%d, Y=%d, Z=%d\n", accel_data.x, accel_data.y, accel_data.z);
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
static void oled_test(void)
{
    OLED_Init();

    // /*在(0, 0)位置显示字符'A'，字体大小为8*16点阵*/
    OLED_ShowChar(0, 0, 'A', OLED_8X16);

    // /*在(16, 0)位置显示字符串"Hello World!"，字体大小为8*16点阵*/
    OLED_ShowString(16, 0, "Hello World!", OLED_8X16);

    // /*在(0, 18)位置显示字符'A'，字体大小为6*8点阵*/
    // OLED_ShowChar(0, 18, 'A', OLED_6X8);

    // /*在(16, 18)位置显示字符串"Hello World!"，字体大小为6*8点阵*/
    // OLED_ShowString(16, 18, "Hello World!", OLED_6X8);

    // /*在(0, 28)位置显示数字12345，长度为5，字体大小为6*8点阵*/
    // OLED_ShowNum(0, 28, 12345, 5, OLED_6X8);

    // /*在(40, 28)位置显示有符号数字-66，长度为2，字体大小为6*8点阵*/
    // OLED_ShowSignedNum(40, 28, -66, 2, OLED_6X8);

    // /*在(70, 28)位置显示十六进制数字0xA5A5，长度为4，字体大小为6*8点阵*/
    // OLED_ShowHexNum(70, 28, 0xA5A5, 4, OLED_6X8);

    // /*在(0, 38)位置显示二进制数字0xA5，长度为8，字体大小为6*8点阵*/
    // OLED_ShowBinNum(0, 38, 0xA5, 8, OLED_6X8);

    // /*在(60, 38)位置显示浮点数字123.45，整数部分长度为3，小数部分长度为2，字体大小为6*8点阵*/
    // OLED_ShowFloatNum(60, 38, 123.45, 3, 2, OLED_6X8);

    // /*在(0, 48)位置显示汉字串"你好，世界。"，字体大小为固定的16*16点阵*/
    // OLED_ShowChinese(0, 48, "你好，世界。");

    // /*在(96, 48)位置显示图像，宽16像素，高16像素，图像数据为Diode数组*/
    // OLED_ShowImage(96, 48, 16, 16, Diode);

    // /*在(96, 18)位置打印格式化字符串，字体大小为6*8点阵，格式化字符串为"[%02d]"*/
    // OLED_Printf(96, 18, OLED_6X8, "[%02d]", 6);

    // /*调用OLED_Update函数，将OLED显存数组的内容更新到OLED硬件进行显示*/
    OLED_Update();

    // /*延时3000ms，观察现象*/
    // Delay_Ms(3000);

    // /*清空OLED显存数组*/
    // OLED_Clear();

    // /*在(5, 8)位置画点*/
    // OLED_DrawPoint(5, 8);

    // /*获取(5, 8)位置的点*/
    // if (OLED_GetPoint(5, 8))
    // {
    //     /*如果指定点点亮，则在(10, 4)位置显示字符串"YES"，字体大小为6*8点阵*/
    //     OLED_ShowString(10, 4, "YES", OLED_6X8);
    // }
    // else
    // {
    //     /*如果指定点未点亮，则在(10, 4)位置显示字符串"NO "，字体大小为6*8点阵*/
    //     OLED_ShowString(10, 4, "NO ", OLED_6X8);
    // }

    // /*在(40, 0)和(127, 15)位置之间画直线*/
    // OLED_DrawLine(40, 0, 127, 15);

    // /*在(40, 15)和(127, 0)位置之间画直线*/
    // OLED_DrawLine(40, 15, 127, 0);

    // /*在(0, 20)位置画矩形，宽12像素，高15像素，未填充*/
    // OLED_DrawRectangle(0, 20, 12, 15, OLED_UNFILLED);

    // /*在(0, 40)位置画矩形，宽12像素，高15像素，填充*/
    // OLED_DrawRectangle(0, 40, 12, 15, OLED_FILLED);

    // /*在(20, 20)、(40, 25)和(30, 35)位置之间画三角形，未填充*/
    // OLED_DrawTriangle(20, 20, 40, 25, 30, 35, OLED_UNFILLED);

    // /*在(20, 40)、(40, 45)和(30, 55)位置之间画三角形，填充*/
    // OLED_DrawTriangle(20, 40, 40, 45, 30, 55, OLED_FILLED);

    // /*在(55, 27)位置画圆，半径8像素，未填充*/
    // OLED_DrawCircle(55, 27, 8, OLED_UNFILLED);

    // /*在(55, 47)位置画圆，半径8像素，填充*/
    // OLED_DrawCircle(55, 47, 8, OLED_FILLED);

    // /*在(82, 27)位置画椭圆，横向半轴12像素，纵向半轴8像素，未填充*/
    // OLED_DrawEllipse(82, 27, 12, 8, OLED_UNFILLED);

    // /*在(82, 47)位置画椭圆，横向半轴12像素，纵向半轴8像素，填充*/
    // OLED_DrawEllipse(82, 47, 12, 8, OLED_FILLED);

    // /*在(110, 18)位置画圆弧，半径15像素，起始角度25度，终止角度125度，未填充*/
    // OLED_DrawArc(110, 18, 15, 25, 125, OLED_UNFILLED);

    // /*在(110, 38)位置画圆弧，半径15像素，起始角度25度，终止角度125度，填充*/
    // OLED_DrawArc(110, 38, 15, 25, 125, OLED_FILLED);

    // /*调用OLED_Update函数，将OLED显存数组的内容更新到OLED硬件进行显示*/
    // OLED_Update();

    /*延时3000ms，观察现象*/
    // Delay_Ms(3000);

    // while (1)
    // {
    //     for (uint8_t i = 0; i < 4; i++)
    //     {
    //         /*将OLED显存数组部分数据取反，从(0, i * 16)位置开始，宽128像素，高16像素*/
    //         OLED_ReverseArea(0, i * 16, 128, 16);

    //         /*调用OLED_Update函数，将OLED显存数组的内容更新到OLED硬件进行显示*/
    //         OLED_Update();

    //         /*延时1000ms，观察现象*/
    //         Delay_Ms(1000);

    //         /*把取反的内容翻转回来*/
    //         OLED_ReverseArea(0, i * 16, 128, 16);
    //     }

    //     /*将OLED显存数组全部数据取反*/
    //     OLED_Reverse();

    //     /*调用OLED_Update函数，将OLED显存数组的内容更新到OLED硬件进行显示*/
    //     OLED_Update();

    //     /*延时1000ms，观察现象*/
    //     Delay_Ms(1000);
    // }
}