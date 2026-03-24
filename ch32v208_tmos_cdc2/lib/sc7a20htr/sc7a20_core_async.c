#include "sc7a20_core.h"
#include "sc7a20_async.h"

#if SC7A20_ASYNC_SUPPORT

#include <string.h>

/* Keep a local copy because the table in sc7a20_core.c is file-static. */
static const float sensitivity_table[] = {
    [SC7A20_ACCEL_FS_2G]  = 0.9765625f,
    [SC7A20_ACCEL_FS_4G]  = 1.953125f,
    [SC7A20_ACCEL_FS_8G]  = 3.90625f,
    [SC7A20_ACCEL_FS_16G] = 7.8125f,
};

typedef struct
{
    int16_t *x;
    int16_t *y;
    int16_t *z;
} sc7a20_raw_out_t;

enum
{
    SC7A20_ASYNC_OP_NONE = 0,
    SC7A20_ASYNC_OP_REG_READ,
    SC7A20_ASYNC_OP_ACCEL_READ,
    SC7A20_ASYNC_OP_RAW_READ,
};

static sc7a20_handle_t g_async_pending_handle = NULL;
static sc7a20_raw_out_t g_raw_out_ctx;

static void sc7a20_async_finish(sc7a20_handle_t handle, sc7a20_status_t status)
{
    void (*user_cb)(sc7a20_handle_t handle, sc7a20_status_t status);

    if (handle == NULL)
    {
        return;
    }

    user_cb = handle->async_ctx.callback;

    if (status == SC7A20_OK)
    {
        handle->last_status = SC7A20_OK;
    }
    else
    {
        handle->last_status = status;
        handle->error_count++;
    }

    handle->async_ctx.state = SC7A20_ASYNC_IDLE;
    handle->async_ctx.processed_bytes = SC7A20_ASYNC_OP_NONE;
    handle->async_ctx.callback = NULL;
    g_async_pending_handle = NULL;

    if (user_cb != NULL)
    {
        user_cb(handle, status);
    }
}

void sc7a20_convert_raw_to_accel_data(sc7a20_handle_t handle,
                                      const uint8_t *raw_data,
                                      sc7a20_accel_data_t *accel_data)
{
    int16_t raw_x;
    int16_t raw_y;
    int16_t raw_z;

    if (handle == NULL || raw_data == NULL || accel_data == NULL)
    {
        return;
    }

    if (handle->endian == 0U)
    {
        raw_x = (int16_t)((raw_data[1] << 8) | raw_data[0]);
        raw_y = (int16_t)((raw_data[3] << 8) | raw_data[2]);
        raw_z = (int16_t)((raw_data[5] << 8) | raw_data[4]);
    }
    else
    {
        raw_x = (int16_t)((raw_data[0] << 8) | raw_data[1]);
        raw_y = (int16_t)((raw_data[2] << 8) | raw_data[3]);
        raw_z = (int16_t)((raw_data[4] << 8) | raw_data[5]);
    }

    accel_data->x = raw_x >> 4;
    accel_data->y = raw_y >> 4;
    accel_data->z = raw_z >> 4;

    accel_data->x_g = accel_data->x * handle->sensitivity;
    accel_data->y_g = accel_data->y * handle->sensitivity;
    accel_data->z_g = accel_data->z * handle->sensitivity;
}

void sc7a20_process_async_operation(sc7a20_handle_t handle)
{
    (void)handle;
}

bool sc7a20_check_async_timeout(sc7a20_handle_t handle)
{
    if (handle == NULL)
    {
        return false;
    }

    if (handle->async_ctx.state == SC7A20_ASYNC_IDLE)
    {
        return false;
    }

    return false;
}

void sc7a20_internal_read_callback(void *user_data, sc7a20_status_t status)
{
    sc7a20_handle_t handle = (sc7a20_handle_t)user_data;

    if (handle == NULL)
    {
        handle = g_async_pending_handle;
    }

    if (handle == NULL)
    {
        return;
    }

    if ((status == SC7A20_OK) && (handle->async_ctx.read_data != NULL))
    {
        if (handle->async_ctx.processed_bytes == SC7A20_ASYNC_OP_ACCEL_READ)
        {
            sc7a20_convert_raw_to_accel_data(handle,
                                             handle->internal_buffer,
                                             (sc7a20_accel_data_t *)handle->async_ctx.read_data);
            handle->read_count++;
        }
        else if (handle->async_ctx.processed_bytes == SC7A20_ASYNC_OP_RAW_READ)
        {
            sc7a20_raw_out_t *out = (sc7a20_raw_out_t *)handle->async_ctx.read_data;
            sc7a20_accel_data_t temp;

            if (out != NULL)
            {
                sc7a20_convert_raw_to_accel_data(handle, handle->internal_buffer, &temp);
                if (out->x != NULL)
                {
                    *(out->x) = temp.x;
                }
                if (out->y != NULL)
                {
                    *(out->y) = temp.y;
                }
                if (out->z != NULL)
                {
                    *(out->z) = temp.z;
                }
                handle->read_count++;
            }
        }
        else if (handle->async_ctx.processed_bytes == SC7A20_ASYNC_OP_REG_READ)
        {
            handle->read_count++;
        }
    }

    sc7a20_async_finish(handle, status);
}

void sc7a20_internal_write_callback(void *user_data, sc7a20_status_t status)
{
    sc7a20_handle_t handle = (sc7a20_handle_t)user_data;

    if (handle == NULL)
    {
        handle = g_async_pending_handle;
    }

    if (handle == NULL)
    {
        return;
    }

    sc7a20_async_finish(handle, status);
}

sc7a20_status_t sc7a20_write_register_async(sc7a20_handle_t handle,
                                            uint8_t reg,
                                            const uint8_t *data,
                                            uint16_t len,
                                            void (*callback)(sc7a20_handle_t handle, sc7a20_status_t status))
{
    sc7a20_status_t status;

    if (handle == NULL || data == NULL || callback == NULL || len == 0U)
    {
        return SC7A20_INVALID_PARAM;
    }

    if (!handle->flags.is_initialized)
    {
        return SC7A20_NOT_INIT;
    }

    if (handle->async_ctx.state != SC7A20_ASYNC_IDLE)
    {
        return SC7A20_ERROR;
    }

    if (handle->ops.write_async == NULL)
    {
        return SC7A20_INVALID_PARAM;
    }

    handle->async_ctx.state = SC7A20_ASYNC_WRITE_DATA;
    handle->async_ctx.reg_addr = reg;
    handle->async_ctx.write_data = data;
    handle->async_ctx.read_data = NULL;
    handle->async_ctx.data_len = len;
    handle->async_ctx.processed_bytes = SC7A20_ASYNC_OP_NONE;
    handle->async_ctx.is_read_operation = false;
    handle->async_ctx.callback = callback;
    handle->async_ctx.start_time = 0U;
    handle->async_ctx.timeout_ms = SC7A20_ASYNC_DEFAULT_TIMEOUT_MS;

    g_async_pending_handle = handle;
    handle->ops.user_data = handle;

    status = handle->ops.write_async(reg, data, len, sc7a20_internal_write_callback);
    if (status != SC7A20_OK)
    {
        handle->async_ctx.state = SC7A20_ASYNC_IDLE;
        handle->async_ctx.callback = NULL;
        handle->async_ctx.processed_bytes = SC7A20_ASYNC_OP_NONE;
        g_async_pending_handle = NULL;
        handle->last_status = status;
        return status;
    }

    return SC7A20_OK;
}

sc7a20_status_t sc7a20_read_register_async(sc7a20_handle_t handle,
                                           uint8_t reg,
                                           uint8_t *data,
                                           uint16_t len,
                                           void (*callback)(sc7a20_handle_t handle, sc7a20_status_t status))
{
    sc7a20_status_t status;

    if (handle == NULL || data == NULL || callback == NULL || len == 0U)
    {
        return SC7A20_INVALID_PARAM;
    }

    if (!handle->flags.is_initialized)
    {
        return SC7A20_NOT_INIT;
    }

    if (handle->async_ctx.state != SC7A20_ASYNC_IDLE)
    {
        return SC7A20_ERROR;
    }

    if (handle->ops.read_async == NULL)
    {
        return SC7A20_INVALID_PARAM;
    }

    handle->async_ctx.state = SC7A20_ASYNC_READ_DATA;
    handle->async_ctx.reg_addr = reg;
    handle->async_ctx.write_data = NULL;
    handle->async_ctx.read_data = data;
    handle->async_ctx.data_len = len;
    handle->async_ctx.processed_bytes = SC7A20_ASYNC_OP_REG_READ;
    handle->async_ctx.is_read_operation = true;
    handle->async_ctx.callback = callback;
    handle->async_ctx.start_time = 0U;
    handle->async_ctx.timeout_ms = SC7A20_ASYNC_DEFAULT_TIMEOUT_MS;

    g_async_pending_handle = handle;
    handle->ops.user_data = handle;

    status = handle->ops.read_async(reg, data, len, sc7a20_internal_read_callback);
    if (status != SC7A20_OK)
    {
        handle->async_ctx.state = SC7A20_ASYNC_IDLE;
        handle->async_ctx.callback = NULL;
        handle->async_ctx.processed_bytes = SC7A20_ASYNC_OP_NONE;
        g_async_pending_handle = NULL;
        handle->last_status = status;
        return status;
    }

    return SC7A20_OK;
}

sc7a20_status_t sc7a20_read_acceleration_async(sc7a20_handle_t handle,
                                               sc7a20_accel_data_t *data,
                                               void (*callback)(sc7a20_handle_t handle, sc7a20_status_t status))
{
    sc7a20_status_t status;

    if (handle == NULL || data == NULL || callback == NULL)
    {
        return SC7A20_INVALID_PARAM;
    }

    if (!handle->flags.is_initialized)
    {
        return SC7A20_NOT_INIT;
    }

    if (handle->async_ctx.state != SC7A20_ASYNC_IDLE)
    {
        return SC7A20_ERROR;
    }

    if (handle->ops.read_async == NULL)
    {
        return SC7A20_INVALID_PARAM;
    }

    handle->async_ctx.state = SC7A20_ASYNC_READ_DATA;
    handle->async_ctx.reg_addr = SC7A20_OUTX_L | 0x80U;
    handle->async_ctx.write_data = NULL;
    handle->async_ctx.read_data = (uint8_t *)data;
    handle->async_ctx.data_len = 6U;
    handle->async_ctx.processed_bytes = SC7A20_ASYNC_OP_ACCEL_READ;
    handle->async_ctx.is_read_operation = true;
    handle->async_ctx.callback = callback;
    handle->async_ctx.start_time = 0U;
    handle->async_ctx.timeout_ms = SC7A20_ASYNC_DEFAULT_TIMEOUT_MS;

    g_async_pending_handle = handle;
    handle->ops.user_data = handle;

    status = handle->ops.read_async(SC7A20_OUTX_L | 0x80U,
                                    handle->internal_buffer,
                                    6U,
                                    sc7a20_internal_read_callback);
    if (status != SC7A20_OK)
    {
        handle->async_ctx.state = SC7A20_ASYNC_IDLE;
        handle->async_ctx.callback = NULL;
        handle->async_ctx.processed_bytes = SC7A20_ASYNC_OP_NONE;
        g_async_pending_handle = NULL;
        handle->last_status = status;
        return status;
    }

    return SC7A20_OK;
}

sc7a20_status_t sc7a20_read_raw_data_async(sc7a20_handle_t handle,
                                           int16_t *x,
                                           int16_t *y,
                                           int16_t *z,
                                           void (*callback)(sc7a20_handle_t handle, sc7a20_status_t status))
{
    sc7a20_status_t status;

    if (handle == NULL || x == NULL || y == NULL || z == NULL || callback == NULL)
    {
        return SC7A20_INVALID_PARAM;
    }

    if (!handle->flags.is_initialized)
    {
        return SC7A20_NOT_INIT;
    }

    if (handle->async_ctx.state != SC7A20_ASYNC_IDLE)
    {
        return SC7A20_ERROR;
    }

    if (handle->ops.read_async == NULL)
    {
        return SC7A20_INVALID_PARAM;
    }

    g_raw_out_ctx.x = x;
    g_raw_out_ctx.y = y;
    g_raw_out_ctx.z = z;

    handle->async_ctx.state = SC7A20_ASYNC_READ_DATA;
    handle->async_ctx.reg_addr = SC7A20_OUTX_L | 0x80U;
    handle->async_ctx.write_data = NULL;
    handle->async_ctx.read_data = (uint8_t *)&g_raw_out_ctx;
    handle->async_ctx.data_len = 6U;
    handle->async_ctx.processed_bytes = SC7A20_ASYNC_OP_RAW_READ;
    handle->async_ctx.is_read_operation = true;
    handle->async_ctx.callback = callback;
    handle->async_ctx.start_time = 0U;
    handle->async_ctx.timeout_ms = SC7A20_ASYNC_DEFAULT_TIMEOUT_MS;

    g_async_pending_handle = handle;
    handle->ops.user_data = handle;

    status = handle->ops.read_async(SC7A20_OUTX_L | 0x80U,
                                    handle->internal_buffer,
                                    6U,
                                    sc7a20_internal_read_callback);
    if (status != SC7A20_OK)
    {
        handle->async_ctx.state = SC7A20_ASYNC_IDLE;
        handle->async_ctx.callback = NULL;
        handle->async_ctx.processed_bytes = SC7A20_ASYNC_OP_NONE;
        g_async_pending_handle = NULL;
        handle->last_status = status;
        return status;
    }

    return SC7A20_OK;
}

sc7a20_status_t sc7a20_set_range_async(sc7a20_handle_t handle,
                                       sc7a20_accel_fs_t range,
                                       void (*callback)(sc7a20_handle_t handle, sc7a20_status_t status))
{
    sc7a20_status_t status;

    if (handle == NULL || callback == NULL)
    {
        return SC7A20_INVALID_PARAM;
    }

    status = sc7a20_set_range(handle, range);
    if (status == SC7A20_OK)
    {
        handle->config.range = range;
        if (range < (sizeof(sensitivity_table) / sizeof(sensitivity_table[0])))
        {
            handle->sensitivity = sensitivity_table[range] / 1000.0f;
        }
    }

    callback(handle, status);
    return status;
}

sc7a20_status_t sc7a20_set_output_data_rate_async(sc7a20_handle_t handle,
                                                   sc7a20_accel_odr_t odr,
                                                   void (*callback)(sc7a20_handle_t handle, sc7a20_status_t status))
{
    sc7a20_status_t status;

    if (handle == NULL || callback == NULL)
    {
        return SC7A20_INVALID_PARAM;
    }

    status = sc7a20_set_output_data_rate(handle, odr);
    callback(handle, status);
    return status;
}

sc7a20_status_t sc7a20_set_axis_enable_async(sc7a20_handle_t handle,
                                             bool x_enable,
                                             bool y_enable,
                                             bool z_enable,
                                             void (*callback)(sc7a20_handle_t handle, sc7a20_status_t status))
{
    sc7a20_status_t status;

    if (handle == NULL || callback == NULL)
    {
        return SC7A20_INVALID_PARAM;
    }

    status = sc7a20_set_axis_enable(handle, x_enable, y_enable, z_enable);
    callback(handle, status);
    return status;
}

sc7a20_status_t sc7a20_set_power_mode_async(sc7a20_handle_t handle,
                                            sc7a20_power_mode_t mode,
                                            void (*callback)(sc7a20_handle_t handle, sc7a20_status_t status))
{
    sc7a20_status_t status;

    if (handle == NULL || callback == NULL)
    {
        return SC7A20_INVALID_PARAM;
    }

    status = sc7a20_set_power_mode(handle, mode);
    callback(handle, status);
    return status;
}

#endif /* SC7A20_ASYNC_SUPPORT */
