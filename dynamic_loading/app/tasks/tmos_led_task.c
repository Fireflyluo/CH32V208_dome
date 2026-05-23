#include "drv_gpio.h"
#include "led_module_programs.h"
#include "log_print.h"
#include "module_loader.h"
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
static module_ctx_t s_module_ctx;
static module_host_api_t s_host_api;
static const module_exports_t *s_led_program_exports = NULL;

static tmosEvents led_task_process_event(tmosTaskID task_id, tmosEvents events);

static void led_write_local(uint8_t on)
{
    s_led_is_on = on ? 1u : 0u;
    gpio_write(LED_PIN, s_led_is_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void led_host_log(const char *msg)
{
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
    if (s_led_program_exports != NULL && s_led_program_exports->deinit != NULL)
    {
        s_led_program_exports->deinit(&s_module_ctx, &s_host_api);
    }
    s_led_program_exports = NULL;
    memset(&s_module_ctx, 0, sizeof(s_module_ctx));
}

static void led_load_program(uint8_t index)
{
    const led_module_program_desc_t *program = NULL;
    const module_exports_t *exports = NULL;
    module_loader_status_t status;

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
    led_unload_current_program();

    status = module_loader_copy_from_flash(program->blob, program->size, 0u);
    if (status != MODULE_LOADER_OK)
    {
        LOG_PRINT("led module: load failed idx=%u status=%u\r\n",
                  (unsigned int)index,
                  (unsigned int)status);
        return;
    }

    status = module_loader_resolve_exports(&exports);
    if (status != MODULE_LOADER_OK || exports == NULL)
    {
        LOG_PRINT("led module: exports invalid idx=%u status=%u\r\n",
                  (unsigned int)index,
                  (unsigned int)status);
        return;
    }

    if (exports->ctx_size > sizeof(s_module_ctx))
    {
        LOG_PRINT("led module: ctx too large idx=%u need=%lu have=%lu\r\n",
                  (unsigned int)index,
                  (unsigned long)exports->ctx_size,
                  (unsigned long)sizeof(s_module_ctx));
        return;
    }

    memset(&s_module_ctx, 0, sizeof(s_module_ctx));
    s_led_program_exports = exports;
    s_active_program = index;
    s_switch_countdown = LED_MODULE_SWITCH_TICK;
    s_tick_serial = 0u;

    if (s_led_program_exports->init != NULL)
    {
        s_led_program_exports->init(&s_module_ctx, &s_host_api);
    }

    led_log_program_change(index, program);
}

void led_task_init(void)
{
    if (s_led_task_id != INVALID_TASK_ID)
    {
        return;
    }

    gpio_mode(LED_PIN, PIN_MODE_OUTPUT);

    s_host_api.led_write = led_write_local;
    s_host_api.log = led_host_log;

    s_led_task_id = TMOS_ProcessEventRegister(led_task_process_event);
    if (s_led_task_id == INVALID_TASK_ID)
    {
        return;
    }

    s_led_is_on = 0u;
    s_active_program = 0u;
    s_switch_countdown = 0u;
    s_tick_serial = 0u;
    memset(&s_module_ctx, 0, sizeof(s_module_ctx));
    s_led_program_exports = NULL;

    tmos_set_event(s_led_task_id, LED_EVT_INIT);
}

static tmosEvents led_task_process_event(tmosTaskID task_id, tmosEvents events)
{
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
        if (s_led_program_exports != NULL && s_led_program_exports->tick != NULL)
        {
            s_led_program_exports->tick(&s_module_ctx, &s_host_api, s_tick_serial);
        }
        else
        {
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
