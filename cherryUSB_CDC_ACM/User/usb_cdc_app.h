#ifndef USB_CDC_APP_H
#define USB_CDC_APP_H

#include <stdint.h>
#include <stdbool.h>

void cdc_acm_init(uint8_t busid, uintptr_t reg_base);
bool cdc_acm_dtr_is_set(void);
void cdc_acm_poll(void);

#endif
