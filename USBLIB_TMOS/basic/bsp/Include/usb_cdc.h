/********************************** (C) 版权所有 *******************************
 * 文件名称          : UART.H
 * 作者             : WCH
 * 版本            : V1.01
 * 日期               : 2022/12/13
 * 描述              : UART通信相关的头文件
 *******************************************************************************
 * 版权所有 (c) 2021 南京沁恒微电子有限公司
 * 注意: 本软件(修改与否)及二进制文件仅供南京沁恒微电子有限公司生产的单片机使用
 *******************************************************************************/

#ifndef __USB_CDC_H__
#define __USB_CDC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "string.h"
#include "ringbuffer.h"
/******************************************************************************/
/* 相关宏定义 */
// /* 串口缓冲区相关定义 */
// #define DEF_UARTx_RX_BUF_LEN     (4 * 512)                                    /* 串口x接收缓冲区大小 */
// #define DEF_UARTx_TX_BUF_LEN     (2 * 512)                                    /* 串口x发送缓冲区大小 */
// #define DEF_USB_FS_PACK_LEN      64                                           /* USB全速模式下串口x数据包大小 */
// #define DEF_UARTx_TX_BUF_NUM_MAX (DEF_UARTx_TX_BUF_LEN / DEF_USB_FS_PACK_LEN) /* 串口x发送缓冲区大小 */

// /* 串口接收超时相关宏定义 */
// #define DEF_UARTx_BAUDRATE       115200 /* 串口默认波特率 */
// #define DEF_UARTx_STOPBIT        0      /* 串口默认停止位 */
// #define DEF_UARTx_PARITY         0      /* 串口默认校验位 */
// #define DEF_UARTx_DATABIT        8      /* 串口默认数据位 */
// #define DEF_UARTx_RX_TIMEOUT     30     /* 串口接收超时时间，单位为100uS */
// #define DEF_UARTx_USB_UP_TIMEOUT 60000  /* 串口接收上传超时时间，单位为100uS */

// /* 串口收发DMA通道相关宏定义 */
// #define DEF_UART2_TX_DMA_CH DMA1_Channel7 /* 串口2发送通道DMA通道 */
// #define DEF_UART2_RX_DMA_CH DMA1_Channel6 /* 串口1发送通道DMA通道 */

// /************************************************************/
// /* 串口X相关结构体定义 */
// typedef struct __attribute__((packed)) _UART_CTL {
//     uint16_t Rx_LoadPtr;            /* 串口x数据接收缓冲区加载指针 */
//     uint16_t Rx_DealPtr;            /* 串口x数据接收缓冲区处理指针 */
//     volatile uint16_t Rx_RemainLen; /* 串口x数据接收缓冲区剩余未处理长度 */
//     uint8_t Rx_TimeOut;             /* 串口x数据接收超时 */
//     uint8_t Rx_TimeOutMax;          /* 串口x数据接收超时最大值 */

//  volatile uint16_t Tx_LoadNum;                           /* 串口x数据发送缓冲区加载数量 */
//  volatile uint16_t Tx_DealNum;                           /* 串口x数据发送缓冲区处理数量 */
//  volatile uint16_t Tx_RemainNum;                         /* 串口x数据发送缓冲区剩余未处理数量 */
//  volatile uint16_t Tx_PackLen[DEF_UARTx_TX_BUF_NUM_MAX]; /* 串口x数据发送缓冲区当前包长度 */
//  uint8_t Tx_Flag;                                        /* 串口x数据发送状态 */
//  uint8_t Recv1;
//  uint16_t Tx_CurPackLen; /* 串口x当前发送的数据包长度 */
//  uint16_t Tx_CurPackPtr; /* 串口x当前正在发送的数据包指针 */

//  uint8_t USB_Up_IngFlag; /* 串口x USB数据包正在上传标志 */
//  uint8_t Recv2;
//  uint16_t USB_Up_TimeOut;   /* 串口x USB数据包上传超时计时器 */
//  uint8_t USB_Up_Pack0_Flag; /* 串口x USB数据需要上传零长度包标志 */
//  uint8_t USB_Down_StopFlag; /* 串口x USB数据包停止下载标志 */

//  uint8_t Com_Cfg[8]; /* 串口x参数配置(默认波特率为115200，1个停止位，无校验，8个数据位) */
//  uint8_t Recv3;
//  uint8_t USB_Int_UpFlag;       /* 串口x中断上传状态 */
//  uint16_t USB_Int_UpTimeCount; /* 串口x中断上传定时 */
// } UART_CTL, *PUART_CTL;

// /***********************************************************************************************************************/
// /* 常量、变量范围 */
// /* 以下是串口发送和接收相关变量和缓冲区 */
// extern volatile UART_CTL Uart;                                                 /* 串口x控制相关结构体 */
// extern volatile uint32_t UARTx_Rx_DMACurCount;                                 /* 串口x接收DMA当前计数 */
// extern volatile uint32_t UARTx_Rx_DMALastCount;                                /* 串口x最后一次DMA接收计数 */
// extern __attribute__((aligned(4))) uint8_t UART2_Tx_Buf[DEF_UARTx_TX_BUF_LEN]; /* 串口x发送缓冲区 */
// extern __attribute__((aligned(4))) uint8_t UART2_Rx_Buf[DEF_UARTx_RX_BUF_LEN]; /* 串口x接收缓冲区 */

// /***********************************************************************************************************************/
// /* 功能扩展 */
// extern uint8_t RCC_Configuration(void);
// extern void TIM2_Init(void);
// extern void UART2_CfgInit(uint32_t baudrate, uint8_t stopbits, uint8_t parity);            /* UART1初始化 */
// extern void UART2_ParaInit(uint8_t mode);                                                  /* 串口参数初始化 */
// extern void UART2_DMAInit(uint8_t type, uint8_t *pbuf, uint32_t len);                      /* 串口1相关DMA初始化 */
// extern void UART2_Init(uint8_t mode, uint32_t baudrate, uint8_t stopbits, uint8_t parity); /* 串口1初始化 */
// extern void UART2_DataTx_Deal(void);                                                       /* 串口1数据发送处理 */
// extern void UART2_DataRx_Deal(void);                                                       /* 串口1数据接收处理 */
// extern void UART2_USB_Init(void);                                                          /* USB串口初始化 */

/********************** 相关宏定义 ********************************/
#define CDC_MAX_PACKET_SIZE 64        // USB CDC最大数据包大小
#define CDC_RX_BUF_LEN      (4 * 512) // USB CDC接收缓冲区大小
#define CDC_TX_BUF_LEN      (2 * 512) // USB CDC发送缓冲区大小

typedef enum {
    CDC_SUCCESS = 0,   // 操作成功
    CDC_EER_DATA_NULL, // 数据为空
    CDC_EER_TIMEOUT,   // 操作超时
} CDC_ErrCode_t;

// 定义CDC结构体
typedef struct __attribute__((packed)) cdc_struct_t {

    vu8 is_initialized;     // 初始化标志
    vu8 USB_Up_Pack0_Flag;  // USB数据需要上传零长度包标志
    uint16_t timeout_cnt;    // 超时计数器

} cdc_struct_t;

extern volatile cdc_struct_t cdc_device;

CDC_ErrCode_t CDC_SendData(uint8_t *data, uint16_t length);

uint16_t CDC_ReceiveData(uint8_t *buffer, uint16_t max_length);
void CDC_StoreReceivedData(uint8_t *data, uint16_t length);
void CDC_VirtualUartInit(void);
#ifdef __cplusplus
}
#endif

#endif /* __USB_CDC_H */