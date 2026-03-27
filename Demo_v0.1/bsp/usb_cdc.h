/**
 ******************************************************************************
 * @file    usb_cdc.h
 * @brief   USB CDC虚拟串口驱动头文件
 ******************************************************************************
 * @details 本文件定义了USB CDC（Communication Device Class）虚拟串口的驱动API接口，
 *          将USB设备模拟为串口，提供与传统UART兼容的数据收发功能。
 *          
 *          主要特性：
 *          - 基于CH32V208内置USB外设实现
 *          - 支持64字节数据包传输
 *          - 提供环形缓冲区管理接收数据
 *          - 与标准串口API兼容，便于应用层移植
 *          
 *          使用流程：
 *          1. 调用CDC_VirtualUartInit()初始化虚拟串口
 *          2. 使用CDC_SendData()发送数据到主机
 *          3. 使用CDC_ReceiveData()从主机接收数据
 *          4. 接收到的USB数据通过CDC_StoreReceivedData()存入缓冲区
 *
 ******************************************************************************
 */
#ifndef __USB_CDC_H__
#define __USB_CDC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "debug.h"
#include "ringbuffer.h"
#include <stdio.h>
#include <string.h>

// USB CDC配置参数
#define CDC_MAX_PACKET_SIZE 64          ///< CDC最大数据包大小（字节）
#define CDC_RX_BUF_LEN      (4 * 512)   ///< 接收缓冲区大小（4KB）
#define CDC_TX_BUF_LEN      (2 * 512)   ///< 发送缓冲区大小（2KB）

/**
 * @brief  CDC操作错误码枚举
 */
typedef enum {
    CDC_SUCCESS = 0,            ///< 操作成功
    CDC_EER_DATA_NULL,          ///< 数据指针为空或长度为0
    CDC_EER_NOT_READY,          ///< USB设备未就绪（未配置完成）
    CDC_EER_BUSY,               ///< 发送忙（端点忙）
    CDC_EER_TIMEOUT,            ///< 操作超时（当前未使用）
} CDC_ErrCode_t;

/**
 * @brief  CDC设备状态结构体
 */
typedef struct __attribute__((packed)) cdc_struct_t {
    vu8 is_initialized;         ///< 初始化标志（1=已初始化）
    vu8 USB_Up_Pack0_Flag;      ///< USB上行包0标志（保留字段）
    uint16_t timeout_cnt;       ///< 超时计数器（保留字段）
} cdc_struct_t;

extern volatile cdc_struct_t cdc_device;

/**
 * @brief  发送数据到虚拟串口
 * @details 将指定数据通过USB CDC发送到主机，支持最大64字节的数据包。
 * @param[in] data 要发送的数据缓冲区指针
 * @param[in] length 数据长度（字节）
 * @return 操作结果（CDC_SUCCESS表示成功）
 */
CDC_ErrCode_t CDC_SendData(uint8_t *data, uint16_t length);

/**
 * @brief  接收虚拟串口数据
 * @details 从接收缓冲区读取数据，非阻塞操作。
 * @param[out] buffer 接收数据缓冲区指针
 * @param[in] max_length 最大接收长度（字节）
 * @return 实际接收到的数据长度（字节），0表示无数据
 */
uint16_t CDC_ReceiveData(uint8_t *buffer, uint16_t max_length);

/**
 * @brief  存储接收到的USB数据
 * @details 将从USB接收到的数据存入接收缓冲区，由USB中断调用。
 * @param[in] data 接收到的数据缓冲区指针
 * @param[in] length 数据长度（字节）
 */
void CDC_StoreReceivedData(uint8_t *data, uint16_t length);

/**
 * @brief  初始化CDC虚拟串口
 * @details 初始化USB CDC虚拟串口，包括环形缓冲区和设备状态。
 *          此函数应在USB初始化完成后调用。
 */
void CDC_VirtualUartInit(void);

#ifdef __cplusplus
}
#endif

#endif /* __USB_CDC_H__ */