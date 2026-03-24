#include "platform.h"
#include "debug.h"
#include "drv_i2c.h"

#if SC7A20_ASYNC_SUPPORT

extern sc7a20_handle_t get_accel_handle(void);

static sc7a20_accel_data_t g_accel_data;
static volatile bool g_accel_data_ready = false;
static volatile sc7a20_status_t g_accel_operation_status = SC7A20_OK;

static void delay_ms(uint32_t ms)
{
    Delay_Ms(ms);
}

static uint32_t get_millis(void)
{
    static uint32_t counter = 0;
    return counter++;
}

static void accel_read_callback(sc7a20_handle_t handle, sc7a20_status_t status)
{
    (void)handle;
    if (status == SC7A20_OK)
    {
        g_accel_data_ready = true;
        printf("accel async read done\n");
    }
    else
    {
        printf("accel async read failed: %d\n", status);
    }
    g_accel_operation_status = status;
}

int main_init(void)
{
    int ret;

    printf("init SC7A20 async...\n");
    ret = accel_init_async();
    if (ret != 0)
    {
        printf("init failed\n");
        return -1;
    }

    printf("init ok\n");
    return 0;
}

void main_loop(void)
{
    static uint32_t last_read_time = 0;
    const uint32_t read_interval = 100;

    while (1)
    {
        uint32_t current_time = get_millis();

        if ((current_time - last_read_time) >= read_interval)
        {
            sc7a20_handle_t handle = get_accel_handle();
            if (handle->async_ctx.state == SC7A20_ASYNC_IDLE)
            {
                sc7a20_status_t status = sc7a20_read_acceleration_async(handle, &g_accel_data, accel_read_callback);
                if (status != SC7A20_OK)
                {
                    printf("start async read failed: %d\n", status);
                }
                last_read_time = current_time;
            }
        }

        if (g_accel_data_ready)
        {
            printf("Accel[g]: X=%f Y=%f Z=%f\n", g_accel_data.x_g, g_accel_data.y_g, g_accel_data.z_g);
            g_accel_data_ready = false;
        }

        if (g_accel_operation_status != SC7A20_OK)
        {
            g_accel_operation_status = SC7A20_OK;
        }

        delay_ms(10);
    }
}

#endif /* SC7A20_ASYNC_SUPPORT */
