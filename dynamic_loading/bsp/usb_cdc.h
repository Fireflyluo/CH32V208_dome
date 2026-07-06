#ifndef __USB_CDC_H__
#define __USB_CDC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "debug.h"
#include <stdint.h>

#define CDC_MAX_PACKET_SIZE 64u
#define CDC_RX_BUF_LEN      (4u * 512u)
#define CDC_TX_BUF_LEN      (2u * 512u)

typedef enum {
    CDC_SUCCESS = 0,
    CDC_EER_DATA_NULL,
    CDC_EER_NOT_READY,
    CDC_EER_BUSY,
    CDC_EER_TIMEOUT
} CDC_ErrCode_t;

typedef struct __attribute__((packed)) cdc_struct_t {
    vu8 is_initialized;
    vu8 USB_Up_Pack0_Flag;
    uint16_t timeout_cnt;
} cdc_struct_t;

extern volatile cdc_struct_t cdc_device;

CDC_ErrCode_t CDC_SendData(uint8_t *data, uint16_t length);
uint16_t CDC_ReceiveData(uint8_t *buffer, uint16_t max_length);
void CDC_StoreReceivedData(uint8_t *data, uint16_t length);
void CDC_VirtualUartInit(void);

#ifdef __cplusplus
}
#endif

#endif /* __USB_CDC_H__ */
