#include "usb_cdc.h"

#include "usb_desc.h"
#include "usb_lib.h"
#include "usb_prop.h"
#include "usb_pwr.h"

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
