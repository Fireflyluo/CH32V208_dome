#include "sw_timer.h"
#include "ch32v20x.h"
#include <stddef.h>
#include <string.h>

#define SW_TIMER_WHEEL_SIZE 256U

static sw_timer_t *g_wheel[SW_TIMER_WHEEL_SIZE];
static sw_timer_t *g_expired_list;
static uint16_t g_current_slot;
static uint32_t g_tick_ms;

static uint32_t ms_to_ticks(uint32_t ms)
{
    uint32_t ticks = ms / g_tick_ms;
    if ((ms % g_tick_ms) != 0U)
    {
        ticks++;
    }
    return (ticks == 0U) ? 1U : ticks;
}

static void wheel_insert(sw_timer_t *timer, uint32_t ticks)
{
    uint32_t slot_offset = ticks % SW_TIMER_WHEEL_SIZE;
    timer->rounds = (uint16_t)(ticks / SW_TIMER_WHEEL_SIZE);
    timer->slot = (uint16_t)((g_current_slot + slot_offset) % SW_TIMER_WHEEL_SIZE);
    timer->next = g_wheel[timer->slot];
    g_wheel[timer->slot] = timer;
    timer->active = 1U;
}

void sw_timer_wheel_init(uint32_t tick_ms)
{
    if (tick_ms == 0U)
    {
        tick_ms = 1U;
    }

    __disable_irq();
    memset(g_wheel, 0, sizeof(g_wheel));
    g_expired_list = NULL;
    g_current_slot = 0U;
    g_tick_ms = tick_ms;
    __enable_irq();
}

void sw_timer_stop(sw_timer_t *timer)
{
    sw_timer_t **head;
    sw_timer_t *prev = NULL;
    sw_timer_t *node;

    if (timer == NULL || timer->active == 0U)
    {
        return;
    }

    __disable_irq();
    head = &g_wheel[timer->slot];
    node = *head;
    while (node != NULL)
    {
        if (node == timer)
        {
            if (prev == NULL)
            {
                *head = node->next;
            }
            else
            {
                prev->next = node->next;
            }
            timer->next = NULL;
            timer->active = 0U;
            break;
        }
        prev = node;
        node = node->next;
    }
    __enable_irq();
}

int sw_timer_start(sw_timer_t *timer, uint32_t delay_ms, uint32_t period_ms, sw_timer_cb_t cb, void *arg)
{
    uint32_t delay_ticks;

    if (timer == NULL || cb == NULL)
    {
        return -1;
    }

    if (g_tick_ms == 0U)
    {
        return -2;
    }

    delay_ticks = ms_to_ticks(delay_ms);
    timer->period_ticks = (period_ms == 0U) ? 0U : ms_to_ticks(period_ms);
    timer->periodic = (timer->period_ticks != 0U) ? 1U : 0U;
    timer->cb = cb;
    timer->arg = arg;

    sw_timer_stop(timer);

    __disable_irq();
    wheel_insert(timer, delay_ticks);
    __enable_irq();

    return 0;
}

void sw_timer_tick_isr(void)
{
    sw_timer_t *node;
    sw_timer_t *prev = NULL;
    sw_timer_t *next;

    g_current_slot = (uint16_t)((g_current_slot + 1U) % SW_TIMER_WHEEL_SIZE);
    node = g_wheel[g_current_slot];

    while (node != NULL)
    {
        next = node->next;
        if (node->rounds > 0U)
        {
            node->rounds--;
            prev = node;
        }
        else
        {
            if (prev == NULL)
            {
                g_wheel[g_current_slot] = next;
            }
            else
            {
                prev->next = next;
            }
            node->next = g_expired_list;
            g_expired_list = node;
        }
        node = next;
    }
}

void sw_timer_process(void)
{
    sw_timer_t *local_list;
    sw_timer_t *node;

    __disable_irq();
    local_list = g_expired_list;
    g_expired_list = NULL;
    __enable_irq();

    while (local_list != NULL)
    {
        node = local_list;
        local_list = local_list->next;
        node->next = NULL;
        node->active = 0U;

        if (node->cb != NULL)
        {
            node->cb(node->arg);
        }

        if (node->periodic != 0U)
        {
            __disable_irq();
            wheel_insert(node, node->period_ticks);
            __enable_irq();
        }
    }
}

