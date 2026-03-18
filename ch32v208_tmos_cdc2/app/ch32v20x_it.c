/********************************** (C) COPYRIGHT *******************************
 * File Name          : ch32v20x_it.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2022/06/16
 * Description        : Main Interrupt Service Routines.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "ch32v20x_it.h"
#include "CONFIG.h"
#include "drv_i2c.h"

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void BB_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void I2C1_EV_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void I2C1_ER_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
/*********************************************************************
 * @fn      NMI_Handler
 *
 * @brief   This function handles NMI exception.
 *
 * @return  None
 */
void NMI_Handler(void)
{
}

/*********************************************************************
 * @fn      HardFault_Handler
 *
 * @brief   This function handles Hard Fault exception.
 *
 * @return  None
 */
void HardFault_Handler(void)
{
    NVIC_SystemReset();
    while (1)
    {
    }
}

/*********************************************************************
 * @fn      BB_IRQHandler
 *
 * @brief   BB Interrupt for BLE.
 *
 * @return  None
 */
void BB_IRQHandler(void)
{
    BB_IRQLibHandler();
}
/*
 * @fn      TIM2_IRQHandler
 *
 * @brief   This function handles TIM2 exception.
 *
 * @return  none
 */
void TIM2_IRQHandler(void)
{

    /* uart timeout counts */
    // Uart.Rx_TimeOut++;
    // Uart.USB_Up_TimeOut++;

    /* clear status */
    TIM2->INTFR = (uint16_t)~TIM_IT_Update;
}

/*********************************************************************
 * @fn      I2C1_EV_IRQHandler
 *
 * @brief   This function handles I2C1 event interrupt.
 *
 * @return  none
 */
void I2C1_EV_IRQHandler(void)
{
    bsp_i2c_irq_handler(I2C_NUM_1);
}

/*********************************************************************
 * @fn      I2C1_ER_IRQHandler
 *
 * @brief   This function handles I2C1 error interrupt.
 *
 * @return  none
 */
void I2C1_ER_IRQHandler(void)
{
    bsp_i2c_irq_handler(I2C_NUM_1);
}
