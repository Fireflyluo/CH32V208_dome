#include "drv_gpio.h"
#include "ad_hoc_task.h"
#include "tmos_task.h"

#define LED_EVT_INIT (0x0001u << 0)
#define LED_EVT_TICK (0x0001u << 1)

#define LED_TASK_TICK_MS 100u

#define LED_DISCONN_ON_TICK 1u
#define LED_DISCONN_CYCLE_TICK 20u

#define LED_CONN_TOGGLE_TICK 2u
/* 连续若干个LED tick都没有收到新包，则判定链路掉线 */
#define LED_LINK_RX_HOLD_TICK 8u

static tmosTaskID s_led_task_id = INVALID_TASK_ID;
static uint8_t s_led_is_on = 0u;
static uint8_t s_connected = 0u;
static uint8_t s_tick_in_cycle = 0u;
static uint32_t s_last_rx_cnt = 0u;
static uint32_t s_last_tx_ack_cnt = 0u;
static uint32_t s_last_submit_ok_cnt = 0u;
static uint8_t s_no_rx_tick = LED_LINK_RX_HOLD_TICK;

static tmosEvents led_task_process_event(tmosTaskID task_id, tmosEvents events);

static void led_write(uint8_t on)
{
    s_led_is_on = on ? 1u : 0u;
    gpio_write(LED_PIN, s_led_is_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void led_apply_pattern(void)
{
    if (s_connected != 0u)
    {
        if ((s_tick_in_cycle % LED_CONN_TOGGLE_TICK) == 0u)
        {
            led_write((uint8_t)(s_led_is_on == 0u));
        }
        return;
    }

    if (s_tick_in_cycle == 0u)
    {
        led_write(1u);
    }
    else if (s_tick_in_cycle >= LED_DISCONN_ON_TICK)
    {
        led_write(0u);
    }
}

static void led_update_link_state(void)
{
    ad_hoc_task_status_t st;

    ad_hoc_task_get_status(&st);
    if (st.inited == 0u)
    {
        s_connected = 0u;
        s_no_rx_tick = LED_LINK_RX_HOLD_TICK;
        s_last_rx_cnt = st.rx_cnt;
        s_last_tx_ack_cnt = st.tx_report_acked;
        s_last_submit_ok_cnt = st.data_submit_ok;
        return;
    }

    if (st.rx_cnt != s_last_rx_cnt ||
        st.tx_report_acked != s_last_tx_ack_cnt ||
        st.data_submit_ok != s_last_submit_ok_cnt)
    {
        s_last_rx_cnt = st.rx_cnt;
        s_last_tx_ack_cnt = st.tx_report_acked;
        s_last_submit_ok_cnt = st.data_submit_ok;
        s_no_rx_tick = 0u;
        s_connected = 1u;
        return;
    }

    if (s_no_rx_tick < 0xFFu)
    {
        s_no_rx_tick++;
    }
    s_connected = (uint8_t)((s_no_rx_tick < LED_LINK_RX_HOLD_TICK) ? 1u : 0u);
}

void led_task_init(void)
{
    if (s_led_task_id != INVALID_TASK_ID)
    {
        return;
    }

    gpio_mode(LED_PIN, PIN_MODE_OUTPUT);

    s_led_task_id = TMOS_ProcessEventRegister(led_task_process_event);
    if (s_led_task_id == INVALID_TASK_ID)
    {
        return;
    }

    s_led_is_on = 0u;
    s_connected = 0u;
    s_tick_in_cycle = 0u;
    s_last_rx_cnt = 0u;
    s_last_tx_ack_cnt = 0u;
    s_last_submit_ok_cnt = 0u;
    s_no_rx_tick = LED_LINK_RX_HOLD_TICK;

    tmos_set_event(s_led_task_id, LED_EVT_INIT);
}

static tmosEvents led_task_process_event(tmosTaskID task_id, tmosEvents events)
{
    (void)task_id;

    if (events & LED_EVT_INIT)
    {
        led_write(0u);
        tmos_start_reload_task(s_led_task_id, LED_EVT_TICK, MS1_TO_SYSTEM_TIME(LED_TASK_TICK_MS));
        return (events ^ LED_EVT_INIT);
    }

    if (events & LED_EVT_TICK)
    {
        led_update_link_state();
        led_apply_pattern();

        s_tick_in_cycle++;
        if (s_tick_in_cycle >= LED_DISCONN_CYCLE_TICK)
        {
            s_tick_in_cycle = 0u;
        }

        return (events ^ LED_EVT_TICK);
    }

    return 0;
}
