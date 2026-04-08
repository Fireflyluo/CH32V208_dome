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

#include "debug.h"
#include "usb_cdc_app.h"

#define CDC_BUSID 0
#define USB_DEV_FS_BASE ((uintptr_t)0x40005C00UL)

/*********************************************************************
 * @fn      USB_RCC_Init
 *
 * @brief   Initializes the USB clock configuration.
 *
 * @return  none
 */
void USB_RCC_Init (void) {
    RCC_ClocksTypeDef rcc_clocks_status = {0};

    RCC_GetClocksFreq (&rcc_clocks_status);

    if (rcc_clocks_status.SYSCLK_Frequency == 144000000) {
        RCC_USBCLKConfig (RCC_USBCLKSource_PLLCLK_Div3);
    } else if (rcc_clocks_status.SYSCLK_Frequency == 96000000) {
        RCC_USBCLKConfig (RCC_USBCLKSource_PLLCLK_Div2);
    } else if (rcc_clocks_status.SYSCLK_Frequency == 48000000) {
        RCC_USBCLKConfig (RCC_USBCLKSource_PLLCLK_Div1);
    }
#if defined(CH32V20x_D8W) || defined(CH32V20x_D8)
    else if ((rcc_clocks_status.SYSCLK_Frequency == 240000000) && (RCC_USB5PRE_JUDGE() == SET)) {
        RCC_USBCLKConfig (RCC_USBCLKSource_PLLCLK_Div5);
    }
#endif
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
}

static void USB_Interrupts_Config(void)
{
    EXTI_InitTypeDef EXTI_InitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    EXTI_ClearITPendingBit(EXTI_Line18);
    EXTI_InitStructure.EXTI_Line = EXTI_Line18;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = USBWakeUp_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_Init(&NVIC_InitStructure);
}

void usb_dc_low_level_init(void)
{
    USB_RCC_Init();

    /* Ensure DP/DM pins are in the required state for USB device core. */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIOA->CFGHR &= 0xFFF00FFF;
    GPIOA->OUTDR &= ~(3 << 11); /* PA11/PA12 = 0 */
    GPIOA->CFGHR |= 0x00044000; /* floating input */

    /* Enable internal USB pull-up for FS device attach. */
    EXTEN->EXTEN_CTR |= EXTEN_USBD_PU_EN;
    EXTEN->EXTEN_CTR &= ~EXTEN_USBD_LS;

    USB_Interrupts_Config();

    Delay_Us(100);
}

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main (void) {
    NVIC_PriorityGroupConfig (NVIC_PriorityGroup_1);
    SystemCoreClockUpdate ();
    Delay_Init();
    USART_Printf_Init (115200);
    printf ("SystemClk:%lu\r\n", (unsigned long)SystemCoreClock);
    printf ("CherryUSB CDC ACM on CH32V208\r\n");

    cdc_acm_init(CDC_BUSID, USB_DEV_FS_BASE);

    while (1) {
        cdc_acm_poll();
        Delay_Ms(10);
    }
}
