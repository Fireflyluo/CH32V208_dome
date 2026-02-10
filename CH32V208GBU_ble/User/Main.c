/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2021/08/08
 * Description        : Main program body.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*
 *@Note
 *Example routine to emulate a simulate USB-CDC Device, USE USART2(PA2/PA3);
 *Please note: This code uses the default serial port 1 for debugging,
 *if you need to modify the debugging serial port, please do not use USART2
 */
#include "CONFIG.h"
#include "HAL.h"
#include "UART.h"
#include "debug.h"
#include "usb_cdc.h"
#include "usb_lib.h"

#include "board.h"
#include "wchble.H"
/*********************************************************************
 * GLOBAL TYPEDEFS
 */
__attribute__((aligned(4))) u32 MEM_BUF[BLE_MEMHEAP_SIZE / 4];

#if (defined(BLE_MAC)) && (BLE_MAC == TRUE)
uint8_t const MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif

void GPIO_Toggle_INIT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}
/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);
    printf("SystemClk:%d\r\n", SystemCoreClock);
    printf("ChipID:%08x\r\n", DBGMCU_GetCHIPID());
    printf("USBD SimulateCDC Demo\r\n");

    RCC_Configuration();
    WCHBLE_Init();
    board_init();
    // 初始化CDC虚拟串口
    CDC_VirtualUartInit();

    while (1)
    {
        // 处理来自虚拟串口的数据
        uint8_t rx_buffer[64];
        uint16_t rx_len = CDC_ReceiveData(rx_buffer, sizeof(rx_buffer));

        if (rx_len > 0)
        {
            // 回显接收到的数据
            CDC_SendData(rx_buffer, rx_len);
        }
        else
        {
            static uint16_t seq_counter = 0;
            char status_msg[64];
            // 每秒发送一次状态信息
            snprintf(status_msg, sizeof(status_msg), "Status: OK,seq:%d\r\n", seq_counter++);
            CDC_SendData((uint8_t *)status_msg, strlen(status_msg));
        }

        HAL_Delay(1000); // 延时1秒
    }
}

/*********************************************************************
 * GLOBAL TYPEDEFS
 */
__attribute__((aligned(4))) u32 MEM_BUF[BLE_MEMHEAP_SIZE / 4];

#if (defined(BLE_MAC)) && (BLE_MAC == TRUE)
uint8_t const MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif