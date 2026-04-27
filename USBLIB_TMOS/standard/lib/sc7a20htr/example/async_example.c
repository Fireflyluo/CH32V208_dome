/**
 ******************************************************************************
 * @file    async_example.c
 * @brief   SC7A20HTR async usage example
 ******************************************************************************
 */

#include "platform.h"
#include "tmos_task.h"

#if SC7A20_ASYNC_SUPPORT

#ifndef ACCEL_READ_TRIGGER_EVENT
#define ACCEL_READ_TRIGGER_EVENT 0x0001
#endif

#ifndef ACCEL_DATA_READY_EVENT
#define ACCEL_DATA_READY_EVENT 0x0002
#endif

extern sc7a20_handle_t get_accel_handle(void);

static sc7a20_accel_data_t g_accel_data;
static bool g_accel_data_ready = false;
static uint8_t accel_task_id = 0;

void accel_read_callback(sc7a20_handle_t handle, sc7a20_status_t status)
{
    (void)handle;

    if (status == SC7A20_OK)
    {
        g_accel_data_ready = true;
        tmos_set_event(accel_task_id, ACCEL_DATA_READY_EVENT);
    }
    else
    {
        printf("accel async read failed: %d\n", status);
    }
}

void accel_task_process(uint8_t task_id, uint16_t events)
{
    accel_task_id = task_id;

    if ((events & ACCEL_READ_TRIGGER_EVENT) != 0U)
    {
        sc7a20_handle_t handle = get_accel_handle();
        sc7a20_status_t status = sc7a20_read_acceleration_async(handle, &g_accel_data, accel_read_callback);

        if (status != SC7A20_OK)
        {
            printf("start accel async read failed: %d\n", status);
        }
    }

    if ((events & ACCEL_DATA_READY_EVENT) != 0U)
    {
        if (g_accel_data_ready)
        {
            printf("X=%fg, Y=%fg, Z=%fg\n", g_accel_data.x_g, g_accel_data.y_g, g_accel_data.z_g);
            g_accel_data_ready = false;
            tmos_set_event(accel_task_id, ACCEL_READ_TRIGGER_EVENT);
        }
    }
}

int app_init(void)
{
    int ret = accel_init_async();
    if (ret != 0)
    {
        return -1;
    }

    return 0;
}

#endif /* SC7A20_ASYNC_SUPPORT */
