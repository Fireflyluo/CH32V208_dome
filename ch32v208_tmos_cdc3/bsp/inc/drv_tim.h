#ifndef __DRV_TIM_H
#define __DRV_TIM_H

#include "ch32v20x.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void drv_tim_init(uint32_t tick_hz);
void drv_tim_irq_handler(void);
uint32_t drv_tim_get_tick_ms(void);
void drv_tim_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* __DRV_TIM_H */

