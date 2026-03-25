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
#include "drv_tim.h"

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void BB_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void I2C1_EV_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void I2C1_ER_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void DMA1_Channel6_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void DMA1_Channel7_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
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
    drv_tim_irq_handler();
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

/*********************************************************************
 * @fn      DMA1_Channel6_IRQHandler
 *
 * @brief   This function handles I2C1 TX DMA interrupt.
 *
 * @return  none
 */
void DMA1_Channel6_IRQHandler(void)
{
    bsp_i2c_dma_tx_irq_handler(I2C_NUM_1);
}

/*********************************************************************
 * @fn      DMA1_Channel7_IRQHandler
 *
 * @brief   This function handles I2C1 RX DMA interrupt.
 *
 * @return  none
 */
void DMA1_Channel7_IRQHandler(void)
{
    bsp_i2c_dma_rx_irq_handler(I2C_NUM_1);
}
