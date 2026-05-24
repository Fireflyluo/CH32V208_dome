/**
 * @file    ch32v20x_it.c
 * @brief   CH32V208微控制器中断服务例程实现文件
 * @details 本文件包含所有已启用中断的中断服务例程(ISR)实现。
 *          主要处理以下中断：
 *          - 系统异常中断（NMI、HardFault）
 *          - BLE基带中断（BB_IRQHandler）
 *          - 定时器2中断（TIM2_IRQHandler）
 *          - I2C1事件和错误中断（I2C1_EV_IRQHandler、I2C1_ER_IRQHandler）
 *          - DMA通道6和7中断（用于I2C1的TX/RX DMA传输）
 *          
 * @author  WCH (南京沁恒微电子股份有限公司)
 * @version V1.0.0
 * @date    2022/06/16
 * @note    所有中断服务例程都使用WCH-Interrupt-fast属性，以确保快速响应
 */

/*********************************************************************
 * INCLUDES
 */
#include "ch32v20x_it.h"
#include "CONFIG.h"
#include "debug.h"
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
#if (DEBUG != DEBUG_UART2_DMA)
void DMA1_Channel6_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void DMA1_Channel7_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
#endif
/*********************************************************************
 * @fn      NMI_Handler
 *
 * @brief   不可屏蔽中断(NMI)异常处理函数
 *
 * @details 当系统发生不可屏蔽中断时调用此函数。
 *          在当前应用中，NMI处理为空操作。
 *
 * @return  None
 */
void NMI_Handler(void)
{
}

/*********************************************************************
 * @fn      HardFault_Handler
 *
 * @brief   硬件故障异常处理函数
 *
 * @details 当系统发生严重硬件错误（如非法内存访问、栈溢出等）
 *          导致无法继续正常执行时，会触发此异常。
 *          本实现中直接复位系统以恢复到初始状态。
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
 * @brief   BLE基带(BB)中断处理函数
 *
 * @details 处理BLE协议栈相关的基带层中断事件。
 *          调用BB_IRQLibHandler()函数来处理具体的BLE事件。
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
 * @brief   定时器2(TIM2)中断处理函数
 *
 * @details 处理定时器2产生的中断事件。
 *          调用驱动层的定时器中断处理函数drv_tim_irq_handler()。
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
 * @brief   I2C1事件中断处理函数
 *
 * @details 处理I2C1总线上的事件中断（如START、STOP、ADDR等）。
 *          调用板级支持包(BSP)中的I2C中断处理函数。
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
 * @brief   I2C1错误中断处理函数
 *
 * @details 处理I2C1总线上的错误中断（如总线错误、仲裁丢失等）。
 *          与事件中断共用同一个处理函数，通过参数指定I2C编号。
 *
 * @return  none
 */
void I2C1_ER_IRQHandler(void)
{
    bsp_i2c_irq_handler(I2C_NUM_1);
}

#if (DEBUG != DEBUG_UART2_DMA)
/*********************************************************************
 * @fn      DMA1_Channel6_IRQHandler
 *
 * @brief   DMA1通道6中断处理函数（I2C1 TX DMA）
 *
 * @details 处理I2C1发送方向的DMA传输完成中断。
 *          调用BSP层的I2C DMA发送中断处理函数。
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
 * @brief   DMA1通道7中断处理函数（I2C1 RX DMA）
 *
 * @details 处理I2C1接收方向的DMA传输完成中断。
 *          调用BSP层的I2C DMA接收中断处理函数。
 *
 * @return  none
 */
void DMA1_Channel7_IRQHandler(void)
{
    bsp_i2c_dma_rx_irq_handler(I2C_NUM_1);
}
#endif /* DEBUG != DEBUG_UART2_DMA */