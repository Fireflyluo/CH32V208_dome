/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2020/08/06
 * Description        : Peripheral slave application main function and task system initialization
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
/* Header file contains */
#include "CONFIG.h"
#include "HAL.h"
#include "drv_gpio.h"
#include "drv_i2c.h"
// #include "gattprofile.h"
// #include "peripheral.h"

#include "board.h"
#include "sc7a20.h"
#include "sc7a20_ch32_adapter.h"
#include "sht40.h"
#include "sht40_ch32_adapter.h"
#include "tmos_task.h"
#include "usb_cdc.h"
#include "sw_timer.h"
#include <stdio.h>
#include <string.h>
/*********************************************************************
 * GLOBAL TYPEDEFS
 */
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

#define SENSOR_REPORT_PERIOD_MS 1000U
#define SENSOR_LINE_MAX_LEN     128U

#if (defined(BLE_MAC)) && (BLE_MAC == TRUE)
const uint8_t MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif

static sc7a20_dev_t g_sc7a20_dev;
static sc7a20_ch32_bus_ctx_t g_sc7a20_bus = {
    .i2c_num = I2C_NUM_1,
    .dev_addr = SC7A20_I2C_ADDR_H,
};

static sht40_dev_t g_sht40_dev;
static sht40_ch32_bus_ctx_t g_sht40_bus = {
    .i2c_num = I2C_NUM_1,
    .dev_addr = SHT40_I2C_ADDR,
};
static uint8_t g_sensors_ready = 0U;

static void sht40_delay_adapter(void *ctx, uint32_t ms)
{
    (void)ctx;
    HAL_Delay(ms);
}

static int sensors_init_new(void)
{
    int rc;
    sc7a20_cfg_t cfg;
    uint32_t serial = 0;

    cfg = g_sc7a20_default_cfg;
    cfg.range = SC7A20_ACCEL_FS_2G;
    cfg.odr = SC7A20_ACCEL_ODR_50HZ;
    cfg.high_resolution = true;

    g_sc7a20_dev.ops = &g_sc7a20_ch32_i2c_ops;
    g_sc7a20_dev.bus_ctx = &g_sc7a20_bus;
    g_sc7a20_dev.addr = g_sc7a20_bus.dev_addr;

    rc = sc7a20_init_with_config(&g_sc7a20_dev, &cfg);
    if (rc != 0)
    {
        g_sc7a20_bus.dev_addr = SC7A20_I2C_ADDR_L;
        g_sc7a20_dev.addr = g_sc7a20_bus.dev_addr;
        rc = sc7a20_init_with_config(&g_sc7a20_dev, &cfg);
    }
    if (rc != 0)
    {
        PRINT("SC7A20(new) init failed: %d\r\n", rc);
        return -1;
    }

    g_sht40_dev.ops = &g_sht40_ch32_i2c_ops;
    g_sht40_dev.bus_ctx = &g_sht40_bus;
    g_sht40_dev.addr = g_sht40_bus.dev_addr;
    g_sht40_dev.delay_ms = sht40_delay_adapter;
    g_sht40_dev.delay_ctx = NULL;

    rc = sht40_init(&g_sht40_dev);
    if (rc != 0)
    {
        PRINT("SHT40(new) init failed: %d\r\n", rc);
        return -2;
    }

    if (sht40_read_serial(&g_sht40_dev, &serial) == 0)
    {
        PRINT("SHT40(new) serial: 0x%08lx\r\n", serial);
    }

    g_sensors_ready = 1U;
    return 0;
}

static int read_sc7a20_sample(sc7a20_vec3i16_t *raw, sc7a20_vec3f_t *g)
{
    int rc;

    if (g_sensors_ready == 0U)
    {
        return -19;
    }

    rc = sc7a20_read_xyz_raw(&g_sc7a20_dev, raw);
    if (rc != 0)
    {
        return rc;
    }

    g->x = raw->x * g_sc7a20_dev.sensitivity_g_per_lsb;
    g->y = raw->y * g_sc7a20_dev.sensitivity_g_per_lsb;
    g->z = raw->z * g_sc7a20_dev.sensitivity_g_per_lsb;
    return 0;
}

static void cdc_send_text(const char *text)
{
    uint16_t remaining;
    uint16_t chunk_len;
    CDC_ErrCode_t cdc_ret;

    if (text == NULL)
    {
        return;
    }

    remaining = (uint16_t)strlen(text);
    while (remaining > 0U)
    {
        chunk_len = (remaining > CDC_MAX_PACKET_SIZE) ? CDC_MAX_PACKET_SIZE : remaining;
        cdc_ret = CDC_SendData((uint8_t *)text, chunk_len);
        if (cdc_ret != CDC_SUCCESS)
        {
            break;
        }
        text += chunk_len;
        remaining = (uint16_t)(remaining - chunk_len);
    }
}


/*********************************************************************
 * @fn      Main_Circulation
 *
 * @brief   Main loop
 *
 * @return  none
 */
__attribute__((section(".highcode"))) __attribute__((noinline)) void Main_Circulation(void)
{
    uint8_t usb_rx_buf[64];
    uint8_t usb_tx_buf[64];
    uint16_t usb_rx_len = 0;
    uint16_t usb_tx_len = 0;
    uint32_t last_report_tick = 0;
    uint32_t now_tick;
    char sensor_line[SENSOR_LINE_MAX_LEN];
    sc7a20_vec3i16_t accel_raw;
    sc7a20_vec3f_t accel_g;
    sht40_sample_t sht_sample;
    int accel_ret;
    int sht_ret;


    while (1)
    {
        TMOS_SystemProcess();
        sw_timer_process();
        usb_rx_len = CDC_ReceiveData(usb_rx_buf, sizeof(usb_rx_buf));
        if (usb_rx_len > 0)
        {
            usb_tx_len = (usb_rx_len > 58U) ? 58U : usb_rx_len;
            memcpy(usb_tx_buf, "ECHO: ", 6U);
            memcpy(&usb_tx_buf[6], usb_rx_buf, usb_tx_len);
            CDC_SendData(usb_tx_buf, (uint16_t)(usb_tx_len + 6U));
        }

        now_tick = HAL_GetTick();
        if ((uint32_t)(now_tick - last_report_tick) >= SENSOR_REPORT_PERIOD_MS)
        {
            last_report_tick = now_tick;
            if (g_sensors_ready == 0U)
            {
                if (sensors_init_new() != 0)
                {
                    snprintf(sensor_line, sizeof(sensor_line), "Sensors init retry failed\r\n");
                    PRINT("%s", sensor_line);
                    cdc_send_text(sensor_line);
                    continue;
                }
                snprintf(sensor_line, sizeof(sensor_line), "Sensors re-init success\r\n");
                PRINT("%s", sensor_line);
                cdc_send_text(sensor_line);
            }
            accel_ret = read_sc7a20_sample(&accel_raw, &accel_g);
            sht_ret = sht40_read_sample(&g_sht40_dev, SHT40_PRECISION_HIGH, &sht_sample);

            if (accel_ret == 0 && sht_ret == 0)
            {
                snprintf(sensor_line,
                         sizeof(sensor_line),
                         "SC7A20[g]: X=%.3f Y=%.3f Z=%.3f | SHT40: T=%.2fC RH=%.2f%%\r\n",
                         accel_g.x,
                         accel_g.y,
                         accel_g.z,
                         sht_sample.temperature_c,
                         sht_sample.humidity_rh);
            }
            else
            {
                snprintf(sensor_line,
                         sizeof(sensor_line),
                         "Sensor read err: sc7a20=%d sht40=%d\r\n",
                         accel_ret,
                         sht_ret);
            }

            PRINT("%s", sensor_line);
            cdc_send_text(sensor_line);
        }
    }
}
#define LED_PIN GET_PIN(C, 9)
/*********************************************************************
 * @fn      main
 *
 * @brief   Main function
 *
 * @return  none
 */
int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
#ifdef DEBUG
    USART_Printf_Init(115200);
#endif
    PRINT("%s\n", VER_LIB);
    WCHBLE_Init();
    board_init();
    CDC_VirtualUartInit();

    HAL_Init();
    if (sensors_init_new() != 0)
    {
        g_sensors_ready = 0U;
        PRINT("Initial sensor init failed, will retry in loop\r\n");
    }

    gpio_init();
    gpio_mode(LED_PIN, PIN_MODE_OUTPUT);
    led_task_init();

    // GAPRole_PeripheralInit();
    // Peripheral_Init();
    Main_Circulation();
}

/******************************** endfile @ main ******************************/
