#include "usb_cdc.h"

#include <stdbool.h>
#include <string.h>

#include "usbd_core.h"
#include "usbd_cdc_acm.h"

#define CDC_BUSID        0u
#define USB_DEV_FS_BASE  ((uintptr_t)0x40005C00UL)
#define CDC_IN_EP        0x81
#define CDC_OUT_EP       0x02
#define CDC_INT_EP       0x83
#define USB_CONFIG_SIZE  (9 + CDC_ACM_DESCRIPTOR_LEN)
#define USBD_MAX_POWER   100

#ifndef USBD_VID
#define USBD_VID 0x1A86
#endif

#ifndef USBD_PID
#define USBD_PID 0xFE0C
#endif

static const uint8_t g_device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x02, 0x00, 0x00, USBD_VID, USBD_PID, 0x0100, 0x01)
};

static const uint8_t g_config_descriptor[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, CDC_MAX_PACKET_SIZE, 0x02)
};

static const uint8_t g_device_quality_descriptor[] = {
    0x0A, USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x40, 0x00, 0x00
};

static const char *g_string_descriptors[] = {
    (const char[]){ 0x09, 0x04 },
    "wch.cn",
    "Dynamic Loading CDC",
    "CH32V208-CDC"
};

static struct usbd_interface g_intf0;
static struct usbd_interface g_intf1;

static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t g_out_ep_buffer[CDC_MAX_PACKET_SIZE];
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t g_in_ep_buffer[CDC_MAX_PACKET_SIZE];

static uint8_t g_rx_ring[CDC_RX_BUF_LEN];
static volatile uint16_t g_rx_write_idx;
static volatile uint16_t g_rx_read_idx;
static uint8_t g_loopback_ring[CDC_RX_BUF_LEN];
static volatile uint16_t g_loopback_write_idx;
static volatile uint16_t g_loopback_read_idx;
static volatile bool g_cdc_tx_busy;
static volatile bool g_cdc_dtr_set;
static volatile bool g_cdc_configured;

volatile cdc_struct_t cdc_device;

static const uint8_t *device_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return g_device_descriptor;
}

static const uint8_t *config_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return g_config_descriptor;
}

static const uint8_t *device_quality_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return g_device_quality_descriptor;
}

static const char *string_descriptor_callback(uint8_t speed, uint8_t index)
{
    (void)speed;
    if (index >= (sizeof(g_string_descriptors) / sizeof(g_string_descriptors[0]))) {
        return NULL;
    }
    return g_string_descriptors[index];
}

static const struct usb_descriptor g_cdc_descriptor = {
    .device_descriptor_callback = device_descriptor_callback,
    .config_descriptor_callback = config_descriptor_callback,
    .device_quality_descriptor_callback = device_quality_descriptor_callback,
    .string_descriptor_callback = string_descriptor_callback
};

static void usb_clock_init(void)
{
    RCC_ClocksTypeDef rcc_clocks_status = {0};

    RCC_GetClocksFreq(&rcc_clocks_status);

    if (rcc_clocks_status.SYSCLK_Frequency == 144000000u) {
        RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_Div3);
    } else if (rcc_clocks_status.SYSCLK_Frequency == 96000000u) {
        RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_Div2);
    } else if (rcc_clocks_status.SYSCLK_Frequency == 48000000u) {
        RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_Div1);
    }
#if defined(CH32V20x_D8W) || defined(CH32V20x_D8)
    else if ((rcc_clocks_status.SYSCLK_Frequency == 240000000u) && (RCC_USB5PRE_JUDGE() == SET)) {
        RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_Div5);
    }
#endif

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
}

static void usb_interrupts_config(void)
{
    EXTI_InitTypeDef exti_init = {0};
    NVIC_InitTypeDef nvic_init = {0};

    EXTI_ClearITPendingBit(EXTI_Line18);
    exti_init.EXTI_Line = EXTI_Line18;
    exti_init.EXTI_Mode = EXTI_Mode_Interrupt;
    exti_init.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    exti_init.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti_init);

    nvic_init.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 2;
    nvic_init.NVIC_IRQChannelSubPriority = 1;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);

    nvic_init.NVIC_IRQChannel = USBWakeUp_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_Init(&nvic_init);
}

void usb_dc_low_level_init(void)
{
    usb_clock_init();

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIOA->CFGHR &= 0xFFF00FFFu;
    GPIOA->OUTDR &= ~(3u << 11);
    GPIOA->CFGHR |= 0x00044000u;

    EXTEN->EXTEN_CTR |= EXTEN_USBD_PU_EN;
    EXTEN->EXTEN_CTR &= ~EXTEN_USBD_LS;

    usb_interrupts_config();
    Delay_Us(100);
}

static void rx_ring_push(const uint8_t *data, uint16_t length)
{
    uint16_t i;

    for (i = 0u; i < length; i++) {
        g_rx_ring[g_rx_write_idx] = data[i];
        g_rx_write_idx = (uint16_t)((g_rx_write_idx + 1u) % CDC_RX_BUF_LEN);
        if (g_rx_write_idx == g_rx_read_idx) {
            g_rx_read_idx = (uint16_t)((g_rx_read_idx + 1u) % CDC_RX_BUF_LEN);
        }
    }
}

static uint16_t rx_ring_pop(uint8_t *buffer, uint16_t max_length)
{
    uint16_t count = 0u;

    while ((count < max_length) && (g_rx_read_idx != g_rx_write_idx)) {
        buffer[count++] = g_rx_ring[g_rx_read_idx];
        g_rx_read_idx = (uint16_t)((g_rx_read_idx + 1u) % CDC_RX_BUF_LEN);
    }

    return count;
}

static void loopback_ring_push(const uint8_t *data, uint16_t length)
{
    uint16_t i;

    for (i = 0u; i < length; i++) {
        g_loopback_ring[g_loopback_write_idx] = data[i];
        g_loopback_write_idx = (uint16_t)((g_loopback_write_idx + 1u) % CDC_RX_BUF_LEN);
        if (g_loopback_write_idx == g_loopback_read_idx) {
            g_loopback_read_idx = (uint16_t)((g_loopback_read_idx + 1u) % CDC_RX_BUF_LEN);
        }
    }
}

static uint16_t loopback_ring_pop(uint8_t *buffer, uint16_t max_length)
{
    uint16_t count = 0u;

    while ((count < max_length) && (g_loopback_read_idx != g_loopback_write_idx)) {
        buffer[count++] = g_loopback_ring[g_loopback_read_idx];
        g_loopback_read_idx = (uint16_t)((g_loopback_read_idx + 1u) % CDC_RX_BUF_LEN);
    }

    return count;
}

static void loopback_try_start_tx(void)
{
    int ret;
    uint16_t chunk_len;

    if (g_cdc_tx_busy || !g_cdc_configured) {
        return;
    }

    chunk_len = loopback_ring_pop(g_in_ep_buffer, CDC_MAX_PACKET_SIZE);
    if (chunk_len == 0u) {
        return;
    }

    g_cdc_tx_busy = true;
    ret = usbd_ep_start_write(CDC_BUSID, CDC_IN_EP, g_in_ep_buffer, chunk_len);
    if (ret < 0) {
        g_cdc_tx_busy = false;
    }
}

static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    (void)busid;

    switch (event) {
    case USBD_EVENT_RESET:
        g_cdc_tx_busy = false;
        g_cdc_dtr_set = false;
        g_cdc_configured = false;
        g_loopback_write_idx = 0u;
        g_loopback_read_idx = 0u;
        break;
    case USBD_EVENT_CONFIGURED:
        g_cdc_tx_busy = false;
        g_cdc_configured = true;
        usbd_ep_start_read(CDC_BUSID, CDC_OUT_EP, g_out_ep_buffer, CDC_MAX_PACKET_SIZE);
        break;
    case USBD_EVENT_DISCONNECTED:
        g_cdc_tx_busy = false;
        g_cdc_dtr_set = false;
        g_cdc_configured = false;
        g_loopback_write_idx = 0u;
        g_loopback_read_idx = 0u;
        break;
    default:
        break;
    }
}

void usbd_cdc_acm_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    (void)ep;

    if (nbytes > CDC_MAX_PACKET_SIZE) {
        nbytes = CDC_MAX_PACKET_SIZE;
    }
    if (nbytes > 0u) {
        rx_ring_push(g_out_ep_buffer, (uint16_t)nbytes);
        loopback_ring_push(g_out_ep_buffer, (uint16_t)nbytes);
        loopback_try_start_tx();
    }
    usbd_ep_start_read(CDC_BUSID, CDC_OUT_EP, g_out_ep_buffer, CDC_MAX_PACKET_SIZE);
}

void usbd_cdc_acm_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    if ((nbytes > 0u) && ((nbytes % usbd_get_ep_mps(busid, ep)) == 0u)) {
        usbd_ep_start_write(busid, CDC_IN_EP, NULL, 0u);
    } else {
        g_cdc_tx_busy = false;
        loopback_try_start_tx();
    }
}

struct usbd_endpoint g_cdc_out_ep = {
    .ep_addr = CDC_OUT_EP,
    .ep_cb = usbd_cdc_acm_bulk_out
};

struct usbd_endpoint g_cdc_in_ep = {
    .ep_addr = CDC_IN_EP,
    .ep_cb = usbd_cdc_acm_bulk_in
};

void usbd_cdc_acm_set_dtr(uint8_t busid, uint8_t intf, bool dtr)
{
    (void)busid;
    (void)intf;
    g_cdc_dtr_set = dtr;
}

void CDC_VirtualUartInit(void)
{
    int ret;

    if (cdc_device.is_initialized != 0u) {
        return;
    }

    g_rx_write_idx = 0u;
    g_rx_read_idx = 0u;
    g_cdc_tx_busy = false;
    g_cdc_dtr_set = false;
    g_cdc_configured = false;
    g_loopback_write_idx = 0u;
    g_loopback_read_idx = 0u;
    cdc_device.timeout_cnt = 0u;
    cdc_device.USB_Up_Pack0_Flag = 0u;

    usbd_desc_register(CDC_BUSID, &g_cdc_descriptor);
    usbd_add_interface(CDC_BUSID, usbd_cdc_acm_init_intf(CDC_BUSID, &g_intf0));
    usbd_add_interface(CDC_BUSID, usbd_cdc_acm_init_intf(CDC_BUSID, &g_intf1));
    usbd_add_endpoint(CDC_BUSID, &g_cdc_out_ep);
    usbd_add_endpoint(CDC_BUSID, &g_cdc_in_ep);

    ret = usbd_initialize(CDC_BUSID, USB_DEV_FS_BASE, usbd_event_handler);
    if (ret == 0) {
        cdc_device.is_initialized = 1u;
    }
}

CDC_ErrCode_t CDC_SendData(uint8_t *data, uint16_t length)
{
    int ret;
    uint16_t packlen;

    cdc_device.timeout_cnt = 0u;

    if ((data == NULL) || (length == 0u) || (cdc_device.is_initialized == 0u)) {
        return CDC_EER_DATA_NULL;
    }

    if (!g_cdc_configured) {
        return CDC_EER_NOT_READY;
    }

    if (g_cdc_tx_busy) {
        return CDC_EER_BUSY;
    }

    packlen = (length > CDC_MAX_PACKET_SIZE) ? CDC_MAX_PACKET_SIZE : length;
    memcpy(g_in_ep_buffer, data, packlen);

    NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
    g_cdc_tx_busy = true;
    ret = usbd_ep_start_write(CDC_BUSID, CDC_IN_EP, g_in_ep_buffer, packlen);
    if (ret < 0) {
        g_cdc_tx_busy = false;
        NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
        return CDC_EER_BUSY;
    }
    NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
    return CDC_SUCCESS;
}

uint16_t CDC_ReceiveData(uint8_t *buffer, uint16_t max_length)
{
    if ((buffer == NULL) || (max_length == 0u)) {
        return 0u;
    }
    return rx_ring_pop(buffer, max_length);
}

void CDC_StoreReceivedData(uint8_t *data, uint16_t length)
{
    if ((data == NULL) || (length == 0u)) {
        return;
    }
    rx_ring_push(data, length);
}
