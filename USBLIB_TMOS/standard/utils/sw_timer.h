#ifndef __SW_TIMER_H
#define __SW_TIMER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*sw_timer_cb_t)(void *arg);

typedef struct sw_timer
{
    uint8_t active;
    uint8_t periodic;
    uint16_t rounds;
    uint16_t slot;
    uint32_t period_ticks;
    sw_timer_cb_t cb;
    void *arg;
    struct sw_timer *next;
} sw_timer_t;

void sw_timer_wheel_init(uint32_t tick_ms);
int sw_timer_start(sw_timer_t *timer, uint32_t delay_ms, uint32_t period_ms, sw_timer_cb_t cb, void *arg);
void sw_timer_stop(sw_timer_t *timer);
void sw_timer_tick_isr(void);
void sw_timer_process(void);

#ifdef __cplusplus
}
#endif

#endif /* __SW_TIMER_H */

