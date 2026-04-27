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
#include "gattprofile.h"
#include "peripheral.h"

#include "UART.h"
#include "board.h"
#include "tmos_task.h"
/*********************************************************************
 * GLOBAL TYPEDEFS
 */
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

#if (defined(BLE_MAC)) && (BLE_MAC == TRUE)
const uint8_t MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif

/*********************************************************************
 * @fn      Main_Circulation
 *
 * @brief   Main loop
 *
 * @return  none
 */
__attribute__((section(".highcode"))) __attribute__((noinline)) void Main_Circulation(void)
{
    while (1)
    {
        TMOS_SystemProcess();
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
    RCC_Configuration();
    WCHBLE_Init();
    board_init();

    HAL_Init();

    gpio_init();
    gpio_mode(LED_PIN, PIN_MODE_OUTPUT);
    led_task_init();

    // GAPRole_PeripheralInit();
    // Peripheral_Init();
    Main_Circulation();
}

/******************************** endfile @ main ******************************/
