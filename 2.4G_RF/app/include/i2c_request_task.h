#ifndef __I2C_REQUEST_TASK_H
#define __I2C_REQUEST_TASK_H

#include "i2c_bus_arbiter.h"
#include "wchble.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*i2c_request_done_cb_t)(const i2c_bus_request_t *req, int status, void *user_ctx);

void i2c_request_task_init(void);
tmosTaskID i2c_request_task_id_get(void);
int i2c_request_task_submit(const i2c_bus_request_t *req, i2c_request_done_cb_t done_cb, void *user_ctx);

#ifdef __cplusplus
}
#endif

#endif /* __I2C_REQUEST_TASK_H */
