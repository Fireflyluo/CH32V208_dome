#include "usb_cdc_app.h"

#include <stdio.h>

#include "ch32v20x.h"
#include "usbd_core.h"
#include "usbd_cdc_acm.h"

#define CDC_IN_EP 0x81
#define CDC_OUT_EP 0x02
#define CDC_INT_EP 0x83

#ifndef USBD_VID
#define USBD_VID 0x1A86
#endif

#ifndef USBD_PID
#define USBD_PID 0xFE0C
#endif

#define USBD_MAX_POWER 100

#define USB_CONFIG_SIZE (9 + CDC_ACM_DESCRIPTOR_LEN)

#ifdef CONFIG_USB_HS
#define CDC_MAX_MPS 512
#else
#define CDC_MAX_MPS 64
#endif

static const uint8_t device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x02, 0x00, 0x00, USBD_VID, USBD_PID, 0x0100, 0x01)
};

static const uint8_t config_descriptor[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, CDC_MAX_MPS, 0x02)
};

static const uint8_t device_quality_descriptor[] = {
    0x0A,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x00,
    0x00,
};

static const char *string_descriptors[] = {
    (const char[]) { 0x09, 0x04 },
    "wch.cn",
    "USB Serial",
    "CH32V208-CDC"
};

static const uint8_t *device_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return device_descriptor;
}

static const uint8_t *config_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return config_descriptor;
}

static const uint8_t *device_quality_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return device_quality_descriptor;
}

static const char *string_descriptor_callback(uint8_t speed, uint8_t index)
{
    (void)speed;

    if (index >= (sizeof(string_descriptors) / sizeof(string_descriptors[0]))) {
        return NULL;
    }
    return string_descriptors[index];
}

static const struct usb_descriptor cdc_descriptor = {
    .device_descriptor_callback = device_descriptor_callback,
    .config_descriptor_callback = config_descriptor_callback,
    .device_quality_descriptor_callback = device_quality_descriptor_callback,
    .string_descriptor_callback = string_descriptor_callback
};

static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t cdc_read_buffer[CDC_MAX_MPS];
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t cdc_write_buffer[CDC_MAX_MPS];

static volatile bool cdc_tx_busy;
static volatile bool cdc_dtr_set;
static volatile uint32_t cdc_drop_count;
static volatile uint8_t cdc_tx_length;
static volatile uint32_t cdc_reset_count;
static volatile uint32_t cdc_configured_count;
static volatile uint32_t cdc_bulk_out_count;
static volatile uint32_t cdc_bulk_in_count;

static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    switch (event) {
    case USBD_EVENT_RESET:
        cdc_tx_busy = false;
        cdc_tx_length = 0;
        cdc_drop_count = 0;
        cdc_dtr_set = false;
        cdc_reset_count++;
        break;
    case USBD_EVENT_CONFIGURED:
        cdc_tx_busy = false;
        cdc_tx_length = 0;
        cdc_configured_count++;
        usbd_ep_start_read(busid, CDC_OUT_EP, cdc_read_buffer, CDC_MAX_MPS);
        break;
    default:
        break;
    }
}

void usbd_cdc_acm_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)ep;
    cdc_bulk_out_count++;

    if (nbytes > CDC_MAX_MPS) {
        nbytes = CDC_MAX_MPS;
    }

    if ((nbytes > 0U) && (!cdc_tx_busy)) {
        memcpy(cdc_write_buffer, cdc_read_buffer, nbytes);
        cdc_tx_busy = true;
        cdc_tx_length = (uint8_t)nbytes;
        usbd_ep_start_write(busid, CDC_IN_EP, cdc_write_buffer, nbytes);
    } else if ((nbytes > 0U) && cdc_tx_busy) {
        cdc_drop_count++;
    }

    usbd_ep_start_read(busid, CDC_OUT_EP, cdc_read_buffer, CDC_MAX_MPS);
}

void usbd_cdc_acm_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    cdc_bulk_in_count++;
    if ((nbytes > 0U) && ((nbytes % usbd_get_ep_mps(busid, ep)) == 0U)) {
        usbd_ep_start_write(busid, CDC_IN_EP, NULL, 0);
    } else {
        cdc_tx_busy = false;
        cdc_tx_length = 0;
    }
}

struct usbd_endpoint cdc_out_ep = {
    .ep_addr = CDC_OUT_EP,
    .ep_cb = usbd_cdc_acm_bulk_out
};

struct usbd_endpoint cdc_in_ep = {
    .ep_addr = CDC_IN_EP,
    .ep_cb = usbd_cdc_acm_bulk_in
};

static struct usbd_interface intf0;
static struct usbd_interface intf1;

void cdc_acm_init(uint8_t busid, uintptr_t reg_base)
{
    cdc_tx_busy = false;
    cdc_dtr_set = false;
    cdc_drop_count = 0;
    cdc_tx_length = 0;
    cdc_reset_count = 0;
    cdc_configured_count = 0;
    cdc_bulk_out_count = 0;
    cdc_bulk_in_count = 0;

    usbd_desc_register(busid, &cdc_descriptor);

    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf0));
    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf1));
    usbd_add_endpoint(busid, &cdc_out_ep);
    usbd_add_endpoint(busid, &cdc_in_ep);
    usbd_initialize(busid, reg_base, usbd_event_handler);
}

void usbd_cdc_acm_set_dtr(uint8_t busid, uint8_t intf, bool dtr)
{
    (void)busid;
    (void)intf;
    cdc_dtr_set = dtr;
}

bool cdc_acm_dtr_is_set(void)
{
    return cdc_dtr_set;
}

void cdc_acm_poll(void)
{
    static uint32_t tick_10ms;
    static uint32_t last_reset_count;
    static uint32_t last_configured_count;
    static bool printed_once;

    tick_10ms++;

    /* Print about once per second (if called every ~10ms). */
    if ((tick_10ms % 100U) != 0U) {
        return;
    }

    if (!printed_once) {
        printed_once = true;
    } else if ((cdc_reset_count == last_reset_count) && (cdc_configured_count == last_configured_count)) {
        return;
    }

    last_reset_count = cdc_reset_count;
    last_configured_count = cdc_configured_count;

    printf("usbdev: reset=%lu cfg=%lu dtr=%u drop=%lu out=%lu in=%lu ext=0x%08lx\r\n",
           (unsigned long)cdc_reset_count,
           (unsigned long)cdc_configured_count,
           cdc_dtr_set ? 1U : 0U,
           (unsigned long)cdc_drop_count,
           (unsigned long)cdc_bulk_out_count,
           (unsigned long)cdc_bulk_in_count,
           (unsigned long)EXTEN->EXTEN_CTR);
}
