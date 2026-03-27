/**
 ******************************************************************************
 * @file    usb_cdc.c
 * @brief   USB CDC虚拟串口驱动实现
 ******************************************************************************
 * @details 本文件实现了USB CDC虚拟串口的核心功能，包括数据发送、接收和缓冲区管理。
 *          使用环形缓冲区管理接收数据，确保数据不会丢失。
 *          
 *          数据流向：
 *          - 发送：应用层 -> CDC_SendData() -> USB端点3 -> 主机
 *          - 接收：主机 -> USB端点 -> CDC_StoreReceivedData() -> 环形缓冲区 -> 应用层
 *          
 *          注意事项：
 *          - 发送操作是非阻塞的，如果端点忙会返回错误
 *          - 接收缓冲区使用环形缓冲区，当缓冲区满时会覆盖最旧的数据
 *          - 所有USB相关操作都需要在中断禁用状态下进行，避免竞争条件
 *
 ******************************************************************************
 */
#include "usb_cdc.h"

#include "usb_desc.h"
#include "usb_lib.h"
#include "usb_prop.h"
#include "usb_pwr.h"

#include "board.h"

// CDC虚拟串口缓冲区配置
#define CDC_BUFFER_SIZE 64
uint8_t cdc_rx_buffer[CDC_BUFFER_SIZE];
volatile uint16_t cdc_rx_write_ptr = 0;
volatile uint16_t cdc_rx_read_ptr = 0;

__attribute__((aligned(4))) uint8_t CDC_Tx_Buf[CDC_RX_BUF_LEN];
__attribute__((aligned(4))) uint8_t CDC_Rx_Buf[CDC_RX_BUF_LEN];

volatile cdc_struct_t cdc_device;
extern uint8_t USBD_Endp3_Busy;

ringbuffer_t rx_ring_buf; // 接收环形缓冲区
ringbuffer_t tx_ring_buf; // 发送环形缓冲区

/**
 * @brief  初始化CDC虚拟串口
 * @details 初始化USB CDC虚拟串口，包括：
 *          - 初始化接收和发送环形缓冲区
 *          - 设置设备初始化标志
 *          - 重置接收缓冲区指针
 */
void CDC_VirtualUartInit(void)
{
    // 初始化环形缓冲区
    ringbuffer_init(&rx_ring_buf, CDC_Rx_Buf, sizeof(CDC_Rx_Buf));
    ringbuffer_init(&tx_ring_buf, CDC_Tx_Buf, sizeof(CDC_Tx_Buf));

    // USB已初始化，只需配置缓冲区
    cdc_device.is_initialized = 1;
    cdc_rx_write_ptr = 0;
    cdc_rx_read_ptr = 0;
}

/**
 * @brief  发送数据到虚拟串口
 * @details 将指定数据通过USB CDC发送到主机，最大支持64字节的数据包。
 *          发送操作是非阻塞的，如果USB端点忙会立即返回错误。
 * @param[in] data 要发送的数据缓冲区指针
 * @param[in] length 数据长度（字节）
 * @return 操作结果
 */
CDC_ErrCode_t CDC_SendData(uint8_t *data, uint16_t length)
{
    uint16_t packlen;
    uint8_t result;

    cdc_device.timeout_cnt = 0;

    if (data == NULL || length == 0 || cdc_device.is_initialized == 0)
    {
        return CDC_EER_DATA_NULL;
    }

    if (bDeviceState != CONFIGURED)
    {
        return CDC_EER_NOT_READY;
    }

    packlen = (length > CDC_MAX_PACKET_SIZE) ? CDC_MAX_PACKET_SIZE : length;

    // 禁用USB中断以避免竞争条件
    NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
    NVIC_DisableIRQ(USB_HP_CAN1_TX_IRQn);
    result = USBD_ENDPx_DataUp(ENDP3, data, packlen);
    NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
    NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn);

    if (result != USB_SUCCESS)
    {
        return CDC_EER_BUSY;
    }

    return CDC_SUCCESS;
}

/**
 * @brief  接收虚拟串口数据
 * @details 从接收缓冲区读取数据，非阻塞操作。
 *          如果没有可用数据，立即返回0。
 * @param[out] buffer 接收数据缓冲区指针
 * @param[in] max_length 最大接收长度（字节）
 * @return 实际接收到的数据长度（字节）
 */
uint16_t CDC_ReceiveData(uint8_t *buffer, uint16_t max_length)
{
    uint16_t count = 0;
    uint16_t available = (cdc_rx_write_ptr >= cdc_rx_read_ptr) ? (cdc_rx_write_ptr - cdc_rx_read_ptr) : (CDC_BUFFER_SIZE - cdc_rx_read_ptr + cdc_rx_write_ptr);

    if (available == 0)
        return 0; // 无数据

    if (max_length > available)
    {
        max_length = available;
    }

    // 从环形缓冲区读取数据
    while (count < max_length && cdc_rx_read_ptr != cdc_rx_write_ptr)
    {
        buffer[count] = cdc_rx_buffer[cdc_rx_read_ptr];
        cdc_rx_read_ptr = (cdc_rx_read_ptr + 1) % CDC_BUFFER_SIZE;
        count++;
    }

    return count;
}

/**
 * @brief  将接收到的USB数据放入缓冲区
 * @details 将从USB接收到的数据存入接收缓冲区，由USB中断服务程序调用。
 *          当缓冲区满时，会覆盖最旧的数据（环形缓冲区特性）。
 * @param[in] data 接收到的数据缓冲区指针
 * @param[in] length 数据长度（字节）
 */
void CDC_StoreReceivedData(uint8_t *data, uint16_t length)
{
    for (uint16_t i = 0; i < length; i++)
    {
        cdc_rx_buffer[cdc_rx_write_ptr] = data[i];
        cdc_rx_write_ptr = (cdc_rx_write_ptr + 1) % CDC_BUFFER_SIZE;

        // 如果缓冲区满了，覆盖旧数据
        if (cdc_rx_write_ptr == cdc_rx_read_ptr)
        {
            cdc_rx_read_ptr = (cdc_rx_read_ptr + 1) % CDC_BUFFER_SIZE;
        }
    }
}