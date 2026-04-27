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
#include "sc7a20_platform.h"
#include "sht40_hal.h"
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
    sc7a20_accel_data_t accel_data;
    float temperature = 0.0f;
    float humidity = 0.0f;
    int accel_ret;
    uint8_t sht_ret;


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
            accel_ret = accel_read_data(&accel_data);
            sht_ret = SHT40_Read_Temperature_Humidity(&temperature, &humidity);

            if ((accel_ret == 0 || accel_ret == 1) && sht_ret == 0U)
            {
                if (accel_ret == 0)
                {
                    snprintf(sensor_line,
                             sizeof(sensor_line),
                             "SC7A20[g]: X=%.3f Y=%.3f Z=%.3f | SHT40: T=%.2fC RH=%.2f%%\r\n",
                             accel_data.x_g,
                             accel_data.y_g,
                             accel_data.z_g,
                             temperature,
                             humidity);
                }
                else
                {
                    snprintf(sensor_line,
                             sizeof(sensor_line),
                             "SC7A20: pending | SHT40: T=%.2fC RH=%.2f%%\r\n",
                             temperature,
                             humidity);
                }
            }
            else
            {
                snprintf(sensor_line,
                         sizeof(sensor_line),
                         "Sensor read err: sc7a20=%d sht40=%u\r\n",
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

    gpio_init();
    gpio_mode(LED_PIN, PIN_MODE_OUTPUT);
    led_task_init();

    // GAPRole_PeripheralInit();
    // Peripheral_Init();
    Main_Circulation();
}

/******************************** endfile @ main ******************************/
