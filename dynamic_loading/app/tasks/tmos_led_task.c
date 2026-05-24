#include "drv_gpio.h"
#include "led_module_programs.h"
#include "log_print.h"
#include "module_loader.h"
#include "module_manager.h"
#include "tmos_task.h"

#include <string.h>

#define LED_EVT_INIT (0x0001u << 0)
#define LED_EVT_TICK (0x0001u << 1)

#define LED_TASK_TICK_MS 100u
#define LED_MODULE_SWITCH_TICK 30u

static tmosTaskID s_led_task_id = INVALID_TASK_ID;
static uint8_t s_led_is_on = 0u;
static uint8_t s_active_program = 0u;
static uint8_t s_switch_countdown = 0u;
static uint32_t s_tick_serial = 0u;
static module_runtime_t s_led_runtime;

static tmosEvents led_task_process_event(tmosTaskID task_id, tmosEvents events);

static void led_write_local(uint8_t on)
{
    s_led_is_on = on ? 1u : 0u;
    gpio_write(LED_PIN, s_led_is_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void led_host_set_output(void *user, uint32_t output_id, uint32_t value)
{
    (void)user;

    if (output_id == MODULE_HOST_OUTPUT_STATUS_LED)
    {
        led_write_local((uint8_t)(value != 0u));
    }
}

static void led_host_log_text(void *user, const char *msg)
{
    (void)user;

    if (msg != NULL)
    {
        LOG_PRINT("led module says: %s\r\n", msg);
    }
}

static void led_log_program_change(uint8_t index, const led_module_program_desc_t *program)
{
    LOG_PRINT("led module: idx=%u name=%s size=%lu slot=%p\r\n",
              (unsigned int)index,
              program->name,
              (unsigned long)program->size,
              (void *)module_loader_slot_base());
}

static void led_unload_current_program(void)
{
    (void)module_manager_unload(&s_led_runtime);
}

static void led_log_busy_slot(void)
{
    LOG_PRINT("led module: slot busy owner=%s program=%s\r\n",
              module_manager_active_owner() != NULL ? module_manager_active_owner() : "unknown",
              module_manager_active_program() != NULL ? module_manager_active_program() : "unknown");
}

static void led_load_program(uint8_t index)
{
    const led_module_program_desc_t *program = NULL;
    module_manager_status_t status;

    if (g_led_module_program_count == 0u)
    {
        led_unload_current_program();
        return;
    }

    if (index >= g_led_module_program_count)
    {
        index = 0u;
    }

    program = &g_led_module_programs[index];
    status = module_manager_load(&s_led_runtime, "led", program->name, program->blob, program->size);
    if (status == MODULE_MANAGER_BUSY)
    {
        led_log_busy_slot();
        return;
    }
    if (status != MODULE_MANAGER_OK)
    {
        LOG_PRINT("led module: load failed idx=%u status=%u\r\n",
                  (unsigned int)index,
                  (unsigned int)status);
        return;
    }

    status = module_manager_call_init(&s_led_runtime);
    if (status != MODULE_MANAGER_OK)
    {
        LOG_PRINT("led module: init failed idx=%u status=%u\r\n",
                  (unsigned int)index,
                  (unsigned int)status);
        (void)module_manager_unload(&s_led_runtime);
        return;
    }

    s_active_program = index;
    s_switch_countdown = LED_MODULE_SWITCH_TICK;
    s_tick_serial = 0u;
    led_log_program_change(index, program);
}

void led_task_init(void)
{
    module_host_api_t host_api;

    if (s_led_task_id != INVALID_TASK_ID)
    {
        return;
    }

    gpio_mode(LED_PIN, PIN_MODE_OUTPUT);

    memset(&host_api, 0, sizeof(host_api));
    host_api.user = NULL;
    host_api.set_output = led_host_set_output;
    host_api.log_text = led_host_log_text;
    module_runtime_setup(&s_led_runtime, &host_api);

    s_led_task_id = TMOS_ProcessEventRegister(led_task_process_event);
    if (s_led_task_id == INVALID_TASK_ID)
    {
        return;
    }

    s_led_is_on = 0u;
    s_active_program = 0u;
    s_switch_countdown = 0u;
    s_tick_serial = 0u;

    tmos_set_event(s_led_task_id, LED_EVT_INIT);
}

static tmosEvents led_task_process_event(tmosTaskID task_id, tmosEvents events)
{
    module_manager_status_t status;

    (void)task_id;

    if (events & LED_EVT_INIT)
    {
        led_write_local(0u);
        led_load_program(0u);
        tmos_start_reload_task(s_led_task_id, LED_EVT_TICK, MS1_TO_SYSTEM_TIME(LED_TASK_TICK_MS));
        return (events ^ LED_EVT_INIT);
    }

    if (events & LED_EVT_TICK)
    {
        status = module_manager_call_tick(&s_led_runtime, s_tick_serial);
        if (status != MODULE_MANAGER_OK)
        {
            if (status == MODULE_MANAGER_NOT_ACTIVE && module_manager_active_owner() != NULL)
            {
                led_log_busy_slot();
            }
            led_write_local(0u);
        }

        s_tick_serial++;
        if (s_switch_countdown > 0u)
        {
            s_switch_countdown--;
        }
        if (s_switch_countdown == 0u && g_led_module_program_count > 0u)
        {
            led_load_program((uint8_t)((s_active_program + 1u) % g_led_module_program_count));
        }

        return (events ^ LED_EVT_TICK);
    }

    return 0;
}
