#include "usb_cdc.h"

#include "usb_desc.h"
#include "usb_lib.h"
#include "usb_prop.h"

#include "UART.h"
#include "board.h"
// CDC虚拟串口缓冲区
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
// 初始化CDC虚拟串口
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

// 发送数据到虚拟串口
CDC_ErrCode_t CDC_SendData(uint8_t *data, uint16_t length)
{

    cdc_device.timeout_cnt = HAL_GetTick();
    // 1.检查
    if (data == NULL || cdc_device.is_initialized == 0)
    {
        return CDC_EER_DATA_NULL; // 数据为空或未初始化
    }
    uint32_t remain_len = length;
    uint16_t offset = 0;
    uint16_t prev_offset = 0; // 记录上一次成功发送的偏移量
    uint16_t packlen;         // USB发送的数据包长度
    // 2.检查是否有数据待上传
    while (remain_len > 0)
    {
            // 保存当前偏移量，用于失败时回退
            prev_offset = offset;

            // 超过最大包长度，分包发送
            if (remain_len >= CDC_MAX_PACKET_SIZE)
            {
                packlen = CDC_MAX_PACKET_SIZE;
            }
            else
            {
                packlen = remain_len;
            }
            // 关中断
            NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
            NVIC_DisableIRQ(USB_HP_CAN1_TX_IRQn);

            // 发送数据包
            uint8_t result = USBD_ENDPx_DataUp(ENDP3, &data[offset], packlen);

            // 开中断
            NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
            NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn);

            // 检查发送结果
            if (result != USB_SUCCESS)
            {
                // 发送失败，恢复到之前的偏移量

                offset = prev_offset;         // 回退偏移量
                remain_len = length - offset; // 重新计算剩余长度

                // 等待一段时间后重试，避免死循环
                uint32_t retry_start_time = HAL_GetTick();
                while ((HAL_GetTick() - retry_start_time) < 50) // 等待10ms后重试
                {
                    // 可以添加其他处理逻辑
                }

                continue; // 重试当前包
            }

            // 更新偏移和剩余长度
            offset += packlen;
            remain_len -= packlen;
            // 检查是否需要发送零长度包（尾包）
            // 如果发送的数据包长度正好是CDC_MAX_PACKET_SIZE的整数倍，且不是最后一包，则需要发送零长度包
            if (packlen == CDC_MAX_PACKET_SIZE && remain_len == 0 && (length % CDC_MAX_PACKET_SIZE) == 0)
            {

                NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
                NVIC_DisableIRQ(USB_HP_CAN1_TX_IRQn);

                USBD_ENDPx_DataUp(ENDP3, NULL, 0); // 发送零长度包

                NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
                NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn);
            }
        
        if (cdc_device.timeout_cnt >= 600) // 600ms超时
        {
            USBD_Endp3_Busy = 0;
            return CDC_EER_TIMEOUT;
        }
    }
    return CDC_SUCCESS;
}

// 接收虚拟串口数据
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

// 将接收到的USB数据放入缓冲区
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
