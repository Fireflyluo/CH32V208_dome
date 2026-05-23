#include "drv_tim.h"
#include "sw_timer.h"

static volatile uint32_t g_tim2_tick_ms = 0;

void drv_tim_init(uint32_t tick_hz)
{
    TIM_TimeBaseInitTypeDef tim_base = {0};

    if (tick_hz == 0)
    {
        tick_hz = 1000;
    }

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM_DeInit(TIM2);

    tim_base.TIM_ClockDivision = TIM_CKD_DIV1;
    tim_base.TIM_CounterMode = TIM_CounterMode_Up;
    tim_base.TIM_Prescaler = (SystemCoreClock / 1000000U) - 1U; /* 1 MHz timer clock */
    tim_base.TIM_Period = (1000000U / tick_hz) - 1U;
    TIM_TimeBaseInit(TIM2, &tim_base);

    TIM_ClearFlag(TIM2, TIM_FLAG_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_SetPriority(TIM2_IRQn, 2);
    NVIC_EnableIRQ(TIM2_IRQn);

    g_tim2_tick_ms = 0;
    sw_timer_wheel_init(1);

    TIM_Cmd(TIM2, ENABLE);
}

void drv_tim_irq_handler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        g_tim2_tick_ms++;
        sw_timer_tick_isr();
    }
}

uint32_t drv_tim_get_tick_ms(void)
{
    return g_tim2_tick_ms;
}

uint64_t drv_tim_get_time_us(void)
{
    uint32_t ms_a;
    uint32_t ms_b;
    uint32_t cnt;

    /* Double-read to avoid race across TIM2 update interrupt boundary. */
    do
    {
        ms_a = g_tim2_tick_ms;
        cnt = TIM_GetCounter(TIM2);
        ms_b = g_tim2_tick_ms;
    } while (ms_a != ms_b);

    return ((uint64_t)ms_a * 1000ULL) + (uint64_t)cnt;
}

void drv_tim_delay_ms(uint32_t ms)
{
    uint32_t start = drv_tim_get_tick_ms();
    while ((drv_tim_get_tick_ms() - start) < ms)
    {
    }
}
