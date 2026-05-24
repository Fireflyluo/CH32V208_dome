/********************************** (C) COPYRIGHT  *******************************
 * File Name          : debug.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2021/06/06
 * Description        : This file contains all the functions prototypes for UART
 *                      Printf , Delay functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "debug.h"

static uint8_t p_us = 0;
static uint16_t p_ms = 0;
static volatile uint8_t uart_tx_complete = 1;
static volatile uint16_t uart_rx_head = 0u;
static volatile uint16_t uart_rx_tail = 0u;
static volatile uint32_t uart_rx_dropped = 0u;
static uint8_t uart_rx_buf[DEBUG_UART_RX_BUF_SIZE];

#if (DEBUG == DEBUG_UART3_DMA || DEBUG == DEBUG_UART2_DMA)
#define DEBUG_UART_DMA_BUF_SIZE (DEBUG_UART_DMA_HALF_SIZE * 2u)
static uint8_t dma_rx_buf[DEBUG_UART_DMA_BUF_SIZE] __attribute__((aligned(4)));
#endif

#define DEBUG_DATA0_ADDRESS ((volatile uint32_t *)0xE0000380)
#define DEBUG_DATA1_ADDRESS ((volatile uint32_t *)0xE0000384)

static void debug_uart_rx_push(uint8_t byte)
{
    uint16_t next = (uint16_t)(uart_rx_head + 1u);
    if (next >= (uint16_t)DEBUG_UART_RX_BUF_SIZE)
    {
        next = 0u;
    }

    if (next == uart_rx_tail)
    {
        uint16_t new_tail = (uint16_t)(uart_rx_tail + 1u);
        if (new_tail >= (uint16_t)DEBUG_UART_RX_BUF_SIZE)
        {
            new_tail = 0u;
        }
        uart_rx_tail = new_tail;
        uart_rx_dropped++;
    }

    uart_rx_buf[uart_rx_head] = byte;
    uart_rx_head = next;
}

static void debug_uart_rx_push_n(const uint8_t *data, uint16_t len)
{
    uint16_t i;
    for (i = 0u; i < len; i++)
    {
        uint16_t next = (uint16_t)(uart_rx_head + 1u);
        if (next >= (uint16_t)DEBUG_UART_RX_BUF_SIZE)
        {
            next = 0u;
        }
        if (next == uart_rx_tail)
        {
            uint16_t new_tail = (uint16_t)(uart_rx_tail + 1u);
            if (new_tail >= (uint16_t)DEBUG_UART_RX_BUF_SIZE)
            {
                new_tail = 0u;
            }
            uart_rx_tail = new_tail;
            uart_rx_dropped++;
        }
        uart_rx_buf[uart_rx_head] = data[i];
        uart_rx_head = next;
    }
}

#if (DEBUG == DEBUG_UART1 || DEBUG == DEBUG_UART2 || DEBUG == DEBUG_UART3)
static void debug_uart_poll_rx(void)
{
    USART_TypeDef *uart = USART1;
#if (DEBUG == DEBUG_UART2)
    uart = USART2;
#elif (DEBUG == DEBUG_UART3)
    uart = USART3;
#endif

    while (USART_GetFlagStatus(uart, USART_FLAG_RXNE) != RESET)
    {
        debug_uart_rx_push((uint8_t)USART_ReceiveData(uart));
    }
}
#endif

void USART2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
#if (DEBUG == DEBUG_UART3_DMA)
void DMA1_Channel3_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
#endif
#if (DEBUG == DEBUG_UART2_DMA)
void DMA1_Channel6_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
#endif
/*********************************************************************
 * @fn      Delay_Init
 *
 * @brief   Initializes Delay Funcation.
 *
 * @return  none
 */
void Delay_Init(void)
{
    p_us = SystemCoreClock / 8000000;
    p_ms = (uint16_t)p_us * 1000;
}

/*********************************************************************
 * @fn      Delay_Us
 *
 * @brief   Microsecond Delay Time.
 *
 * @param   n - Microsecond number.
 *
 * @return  None
 */
void Delay_Us(uint32_t n)
{
    uint32_t i;

    SysTick->SR &= ~(1 << 0);
    i = (uint32_t)n * p_us;

    SysTick->CMP = i;
    SysTick->CTLR |= (1 << 4);
    SysTick->CTLR |= (1 << 5) | (1 << 0);

    while ((SysTick->SR & (1 << 0)) != (1 << 0))
        ;
    SysTick->CTLR &= ~(1 << 0);
}

/*********************************************************************
 * @fn      Delay_Ms
 *
 * @brief   Millisecond Delay Time.
 *
 * @param   n - Millisecond number.
 *
 * @return  None
 */
void Delay_Ms(uint32_t n)
{
    uint32_t i;

    SysTick->SR &= ~(1 << 0);
    i = (uint32_t)n * p_ms;

    SysTick->CMP = i;
    SysTick->CTLR |= (1 << 4);
    SysTick->CTLR |= (1 << 5) | (1 << 0);

    while ((SysTick->SR & (1 << 0)) != (1 << 0))
        ;
    SysTick->CTLR &= ~(1 << 0);
}

/*********************************************************************
 * @fn      USART_Printf_Init
 *
 * @brief   Initializes the USARTx peripheral.
 *
 * @param   baudrate - USART communication baud rate.
 *
 * @return  None
 */
void USART_Printf_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure = {0};
#if (DEBUG == DEBUG_UART1 || DEBUG == DEBUG_UART1_IT)
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

#elif (DEBUG == DEBUG_UART2 || DEBUG == DEBUG_UART2_IT || DEBUG == DEBUG_UART2_DMA)
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

#elif (DEBUG == DEBUG_UART3 || DEBUG == DEBUG_UART3_IT || DEBUG == DEBUG_UART3_DMA)
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

#endif

    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    uart_rx_head = 0u;
    uart_rx_tail = 0u;
    uart_rx_dropped = 0u;

#if (DEBUG == DEBUG_UART1)
    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);

#elif (DEBUG == DEBUG_UART2)
    USART_Init(USART2, &USART_InitStructure);
    USART_Cmd(USART2, ENABLE);

#elif (DEBUG == DEBUG_UART3)
    USART_Init(USART3, &USART_InitStructure);
    USART_Cmd(USART3, ENABLE);

#elif (DEBUG == DEBUG_UART1_IT)
    USART_Init(USART1, &USART_InitStructure);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART1, USART_IT_TC, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART1, ENABLE);

#elif (DEBUG == DEBUG_UART2_IT)
    USART_Init(USART2, &USART_InitStructure);
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART2, USART_IT_TC, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART2, ENABLE);

#elif (DEBUG == DEBUG_UART3_IT)
    USART_Init(USART3, &USART_InitStructure);
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART3, USART_IT_TC, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART3, ENABLE);

#elif (DEBUG == DEBUG_UART3_DMA)
    {
        DMA_InitTypeDef DMA_InitStructure;

        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

        /* DMA1_Channel3: USART3_RX, circular ping-pong */
        DMA_DeInit(DMA1_Channel3);
        DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART3->DATAR;
        DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t)dma_rx_buf;
        DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralSRC;
        DMA_InitStructure.DMA_BufferSize         = DEBUG_UART_DMA_BUF_SIZE;
        DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
        DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
        DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
        DMA_InitStructure.DMA_Mode               = DMA_Mode_Circular;
        DMA_InitStructure.DMA_Priority           = DMA_Priority_High;
        DMA_InitStructure.DMA_M2M                = DMA_M2M_Disable;
        DMA_Init(DMA1_Channel3, &DMA_InitStructure);

        DMA_ITConfig(DMA1_Channel3, DMA_IT_HT | DMA_IT_TC | DMA_IT_TE, ENABLE);

        USART_Init(USART3, &USART_InitStructure);
        USART_DMACmd(USART3, USART_DMAReq_Rx, ENABLE);
        USART_ITConfig(USART3, USART_IT_IDLE, ENABLE);
        USART_ITConfig(USART3, USART_IT_TC, ENABLE);

        /* NVIC: DMA CH3 prio 1.1, USART3 prio 2.1 */
        NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel3_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);

        NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);

        DMA_Cmd(DMA1_Channel3, ENABLE);
        USART_Cmd(USART3, ENABLE);
    }

#elif (DEBUG == DEBUG_UART2_DMA)
    {
        DMA_InitTypeDef DMA_InitStructure;

        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

        /* DMA1_Channel6: USART2_RX, circular ping-pong */
        DMA_DeInit(DMA1_Channel6);
        DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART2->DATAR;
        DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t)dma_rx_buf;
        DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralSRC;
        DMA_InitStructure.DMA_BufferSize         = DEBUG_UART_DMA_BUF_SIZE;
        DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
        DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
        DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
        DMA_InitStructure.DMA_Mode               = DMA_Mode_Circular;
        DMA_InitStructure.DMA_Priority           = DMA_Priority_High;
        DMA_InitStructure.DMA_M2M                = DMA_M2M_Disable;
        DMA_Init(DMA1_Channel6, &DMA_InitStructure);

        DMA_ITConfig(DMA1_Channel6, DMA_IT_HT | DMA_IT_TC | DMA_IT_TE, ENABLE);

        USART_Init(USART2, &USART_InitStructure);
        USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);
        USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);
        USART_ITConfig(USART2, USART_IT_TC, ENABLE);

        /* NVIC: DMA CH6 prio 1.1, USART2 prio 2.1 */
        NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel6_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);

        NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);

        DMA_Cmd(DMA1_Channel6, ENABLE);
        USART_Cmd(USART2, ENABLE);
    }
#endif
}

/*********************************************************************
 * @fn      SDI_Printf_Enable
 *
 * @brief   Initializes the SDI printf Function.
 *
 * @param   None
 *
 * @return  None
 */
void SDI_Printf_Enable(void)
{
    *(DEBUG_DATA0_ADDRESS) = 0;
    Delay_Init();
    Delay_Ms(1);
}

void Debug_UART_RxFlush(void)
{
    uart_rx_head = 0u;
    uart_rx_tail = 0u;
}

uint16_t Debug_UART_RxAvailable(void)
{
    uint16_t head;
    uint16_t tail;

#if (DEBUG == DEBUG_UART1 || DEBUG == DEBUG_UART2 || DEBUG == DEBUG_UART3)
    debug_uart_poll_rx();
#endif

    head = uart_rx_head;
    tail = uart_rx_tail;
    if (head >= tail)
    {
        return (uint16_t)(head - tail);
    }
    return (uint16_t)(DEBUG_UART_RX_BUF_SIZE - tail + head);
}

uint16_t Debug_UART_RxRead(uint8_t *out, uint16_t max_len)
{
    uint16_t count = 0u;

    if (out == NULL || max_len == 0u)
    {
        return 0u;
    }

#if (DEBUG == DEBUG_UART1 || DEBUG == DEBUG_UART2 || DEBUG == DEBUG_UART3)
    debug_uart_poll_rx();
#endif

    while (count < max_len && uart_rx_tail != uart_rx_head)
    {
        out[count++] = uart_rx_buf[uart_rx_tail];
        uart_rx_tail++;
        if (uart_rx_tail >= (uint16_t)DEBUG_UART_RX_BUF_SIZE)
        {
            uart_rx_tail = 0u;
        }
    }

    return count;
}

uint8_t Debug_UART_RxGetByte(uint8_t *out)
{
    if (out == NULL)
    {
        return 0u;
    }
    return (uint8_t)(Debug_UART_RxRead(out, 1u) == 1u ? 1u : 0u);
}

uint32_t Debug_UART_RxDropped(void)
{
    return uart_rx_dropped;
}

void Debug_UART_SpeedTest(uint32_t duration_sec)
{
    uint32_t total_bytes = 0u;
    uint32_t total_drops;
    uint32_t sec;

    Debug_UART_RxFlush();
    uart_rx_dropped = 0u;

    printf("\r\n--- UART DMA Speed Test ---\r\n");
    printf("Duration: %lu s, Half: %u B, Ring: %u B\r\n",
           duration_sec,
           (unsigned)DEBUG_UART_DMA_HALF_SIZE,
           (unsigned)DEBUG_UART_RX_BUF_SIZE);
    printf("Sec   Bytes    Drops   Bps\r\n");
    printf("------------------------------\r\n");

    for (sec = 1u; sec <= duration_sec; sec++)
    {
        uint32_t sec_bytes = 0u;
        uint8_t drain[64];

        Delay_Ms(1000u);

        while (Debug_UART_RxAvailable() > 0u)
        {
            uint16_t n = Debug_UART_RxRead(drain, sizeof(drain));
            sec_bytes += n;
        }

        total_bytes += sec_bytes;
        total_drops = Debug_UART_RxDropped();

        printf("%-6lu %-8lu %-8lu %-8lu\r\n",
               sec, sec_bytes, total_drops, sec_bytes * 8u);
    }

    printf("------------------------------\r\n");
    printf("Total: %lu bytes, Drops: %lu\r\n",
           total_bytes, Debug_UART_RxDropped());
    if (duration_sec > 0u)
    {
        printf("Avg: %lu B/s (%.1f kbps)\r\n",
               total_bytes / duration_sec,
               (double)(total_bytes * 8u) / (double)duration_sec / 1000.0);
    }
}

/*********************************************************************
 * @fn      _write
 *
 * @brief   Support Printf Function
 *
 * @param   *buf - UART send Data.
 *          size - Data length
 *
 * @return  size: Data length
 */
__attribute__((used)) int _write(int fd, char *buf, int size)
{
    int i = 0;

#if (SDI_PRINT == SDI_PR_OPEN)
    int writeSize = size;

    do
    {

        /**
         * data0  data1 8 byte
         * data0 The storage length of the lowest byte, with a maximum of 7 bytes.
         */

        while ((*(DEBUG_DATA0_ADDRESS) != 0u))
        {
        }

        if (writeSize > 7)
        {
            *(DEBUG_DATA1_ADDRESS) = (*(buf + i + 3)) | (*(buf + i + 4) << 8) | (*(buf + i + 5) << 16) | (*(buf + i + 6) << 24);
            *(DEBUG_DATA0_ADDRESS) = (7u) | (*(buf + i) << 8) | (*(buf + i + 1) << 16) | (*(buf + i + 2) << 24);

            i += 7;
            writeSize -= 7;
        }
        else
        {
            *(DEBUG_DATA1_ADDRESS) = (*(buf + i + 3)) | (*(buf + i + 4) << 8) | (*(buf + i + 5) << 16) | (*(buf + i + 6) << 24);
            *(DEBUG_DATA0_ADDRESS) = (writeSize) | (*(buf + i) << 8) | (*(buf + i + 1) << 16) | (*(buf + i + 2) << 24);

            writeSize = 0;
        }

    } while (writeSize);

#else
    for (i = 0; i < size; i++)
    {
#if (DEBUG == DEBUG_UART1)
        while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
            ;
        USART_SendData(USART1, *buf++);

#elif (DEBUG == DEBUG_UART2)
        while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET)
            ;
        USART_SendData(USART2, *buf++);

#elif (DEBUG == DEBUG_UART3)
        while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET)
            ;
        USART_SendData(USART3, *buf++);

#elif (DEBUG == DEBUG_UART1_IT)
        while (uart_tx_complete == 0)
            ;

        USART_ITConfig(USART1, USART_IT_TC, DISABLE);

        USART_SendData(USART1, *buf++);
        uart_tx_complete = 0;
        USART_ITConfig(USART1, USART_IT_TC, ENABLE);
#elif (DEBUG == DEBUG_UART2_IT)
        while (uart_tx_complete == 0)
            ;

        USART_ITConfig(USART2, USART_IT_TC, DISABLE);

        USART_SendData(USART2, *buf++);
        uart_tx_complete = 0;
        USART_ITConfig(USART2, USART_IT_TC, ENABLE);

#elif (DEBUG == DEBUG_UART3_IT)
        while (uart_tx_complete == 0)
            ;

        USART_ITConfig(USART3, USART_IT_TC, DISABLE);

        USART_SendData(USART3, *buf++);
        uart_tx_complete = 0;
        USART_ITConfig(USART3, USART_IT_TC, ENABLE);

#elif (DEBUG == DEBUG_UART3_DMA)
        while (uart_tx_complete == 0)
            ;

        USART_ITConfig(USART3, USART_IT_TC, DISABLE);

        USART_SendData(USART3, *buf++);
        uart_tx_complete = 0;
        USART_ITConfig(USART3, USART_IT_TC, ENABLE);

#elif (DEBUG == DEBUG_UART2_DMA)
        while (uart_tx_complete == 0)
            ;

        USART_ITConfig(USART2, USART_IT_TC, DISABLE);

        USART_SendData(USART2, *buf++);
        uart_tx_complete = 0;
        USART_ITConfig(USART2, USART_IT_TC, ENABLE);
#endif
    }
#endif
    return size;
}

/*********************************************************************
 * @fn      _sbrk
 *
 * @brief   Change the spatial position of data segment.
 *
 * @return  size: Data length
 */
__attribute__((used)) void *_sbrk(ptrdiff_t incr)
{
    extern char _end[];
    extern char _heap_end[];
    static char *curbrk = _end;

    if ((curbrk + incr < _end) || (curbrk + incr > _heap_end))
        return NULL - 1;

    curbrk += incr;
    return curbrk - incr;
}
/*********************************************************************
 * @fn      USART_IRQHandler
 *
 * @brief   This function handles USART global interrupt request.
 *
 * @return  none
 */
#if (DEBUG == DEBUG_UART1_IT)
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        uint8_t buf[64];
        uint16_t n = 0u;
        while (n < 64u && USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET)
        {
            buf[n++] = (uint8_t)USART_ReceiveData(USART1);
        }
        debug_uart_rx_push_n(buf, n);
    }
    if (USART_GetITStatus(USART1, USART_IT_TC) != RESET)
    {
        USART_ClearFlag(USART1, USART_FLAG_TC);
        uart_tx_complete = 1;
    }
}
#elif (DEBUG == DEBUG_UART2_IT)
void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        uint8_t buf[64];
        uint16_t n = 0u;
        while (n < 64u && USART_GetFlagStatus(USART2, USART_FLAG_RXNE) != RESET)
        {
            buf[n++] = (uint8_t)USART_ReceiveData(USART2);
        }
        debug_uart_rx_push_n(buf, n);
    }
    if (USART_GetITStatus(USART2, USART_IT_TC) != RESET)
    {
        uart_tx_complete = 1;
        USART_ClearFlag(USART2, USART_FLAG_TC);
    }
}
#elif (DEBUG == DEBUG_UART2_DMA)
void USART2_IRQHandler(void)
{
    uint16_t remaining;
    uint16_t bytes_in;

    if (USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)
    {
        (void)USART2->STATR;
        (void)USART2->DATAR;

        remaining = DMA_GetCurrDataCounter(DMA1_Channel6);

        if (remaining >= DEBUG_UART_DMA_HALF_SIZE)
        {
            bytes_in = (uint16_t)(DEBUG_UART_DMA_BUF_SIZE - remaining);
            if (bytes_in > 0u && bytes_in < DEBUG_UART_DMA_HALF_SIZE)
            {
                debug_uart_rx_push_n(&dma_rx_buf[0], bytes_in);
            }
        }
        else
        {
            bytes_in = (uint16_t)(DEBUG_UART_DMA_HALF_SIZE - remaining);
            if (bytes_in > 0u && bytes_in < DEBUG_UART_DMA_HALF_SIZE)
            {
                debug_uart_rx_push_n(&dma_rx_buf[DEBUG_UART_DMA_HALF_SIZE],
                                     bytes_in);
            }
        }
    }

    if (USART_GetITStatus(USART2, USART_IT_TC) != RESET)
    {
        USART_ClearFlag(USART2, USART_FLAG_TC);
        uart_tx_complete = 1;
    }
}
#elif (DEBUG == DEBUG_UART3_IT)
void USART3_IRQHandler(void)
{
    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        uint8_t buf[64];
        uint16_t n = 0u;
        while (n < 64u && USART_GetFlagStatus(USART3, USART_FLAG_RXNE) != RESET)
        {
            buf[n++] = (uint8_t)USART_ReceiveData(USART3);
        }
        debug_uart_rx_push_n(buf, n);
    }
    if (USART_GetITStatus(USART3, USART_IT_TC) != RESET)
    {
        USART_ClearFlag(USART3, USART_FLAG_TC);
        uart_tx_complete = 1;
    }
}
#elif (DEBUG == DEBUG_UART3_DMA)
void USART3_IRQHandler(void)
{
    uint16_t remaining;
    uint16_t bytes_in;

    if (USART_GetITStatus(USART3, USART_IT_IDLE) != RESET)
    {
        (void)USART3->STATR;
        (void)USART3->DATAR;

        remaining = DMA_GetCurrDataCounter(DMA1_Channel3);

        if (remaining >= DEBUG_UART_DMA_HALF_SIZE)
        {
            bytes_in = (uint16_t)(DEBUG_UART_DMA_BUF_SIZE - remaining);
            if (bytes_in > 0u && bytes_in < DEBUG_UART_DMA_HALF_SIZE)
            {
                debug_uart_rx_push_n(&dma_rx_buf[0], bytes_in);
            }
        }
        else
        {
            bytes_in = (uint16_t)(DEBUG_UART_DMA_HALF_SIZE - remaining);
            if (bytes_in > 0u && bytes_in < DEBUG_UART_DMA_HALF_SIZE)
            {
                debug_uart_rx_push_n(&dma_rx_buf[DEBUG_UART_DMA_HALF_SIZE],
                                     bytes_in);
            }
        }
    }

    if (USART_GetITStatus(USART3, USART_IT_TC) != RESET)
    {
        USART_ClearFlag(USART3, USART_FLAG_TC);
        uart_tx_complete = 1;
    }
}
#endif

#if (DEBUG == DEBUG_UART3_DMA || DEBUG == DEBUG_UART2_DMA)

#if (DEBUG == DEBUG_UART3_DMA)
void DMA1_Channel3_IRQHandler(void)
{
    uint32_t flags = DMA1->INTFR;

    if (flags & DMA1_IT_HT3)
    {
        DMA1->INTFCR = DMA1_IT_HT3;
        debug_uart_rx_push_n(&dma_rx_buf[0], DEBUG_UART_DMA_HALF_SIZE);
    }

    if (flags & DMA1_IT_TC3)
    {
        DMA1->INTFCR = DMA1_IT_TC3;
        debug_uart_rx_push_n(&dma_rx_buf[DEBUG_UART_DMA_HALF_SIZE],
                             DEBUG_UART_DMA_HALF_SIZE);
    }

    if (flags & DMA1_IT_TE3)
    {
        DMA1->INTFCR = DMA1_IT_TE3;
    }
}
#endif

#if (DEBUG == DEBUG_UART2_DMA)
void DMA1_Channel6_IRQHandler(void)
{
    uint32_t flags = DMA1->INTFR;

    if (flags & DMA1_IT_HT6)
    {
        DMA1->INTFCR = DMA1_IT_HT6;
        debug_uart_rx_push_n(&dma_rx_buf[0], DEBUG_UART_DMA_HALF_SIZE);
    }

    if (flags & DMA1_IT_TC6)
    {
        DMA1->INTFCR = DMA1_IT_TC6;
        debug_uart_rx_push_n(&dma_rx_buf[DEBUG_UART_DMA_HALF_SIZE],
                             DEBUG_UART_DMA_HALF_SIZE);
    }

    if (flags & DMA1_IT_TE6)
    {
        DMA1->INTFCR = DMA1_IT_TE6;
    }
}
#endif
#endif
