/**
 ******************************************************************************
 * @file    sc7a20_platform.h
 * @brief   SC7A20 platform adaptation interface
 ******************************************************************************
 */
#ifndef SC7A20_PLATFORM_H
#define SC7A20_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sc7a20_core.h"
#include "drv_i2c.h"

int accel_init(void);
int accel_read_data(sc7a20_accel_data_t *accel_data);

#if SC7A20_ASYNC_SUPPORT
int accel_init_async(void);
void accel_adapter_i2c_tx_cplt_callback(i2c_num_t i2c_num);
void accel_adapter_i2c_rx_cplt_callback(i2c_num_t i2c_num);
void accel_adapter_i2c_error_callback(i2c_num_t i2c_num, uint32_t error_code);
#endif

void read_acceleration_data(void);

#ifdef __cplusplus
}
#endif

#endif /* SC7A20_PLATFORM_H */
