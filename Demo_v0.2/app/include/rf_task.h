#ifndef __RF_TASK_H
#define __RF_TASK_H

#include <stdint.h>

typedef struct
{
    uint8_t tg_id;
    uint8_t rf_inited;
    uint8_t synced;
    uint32_t tx_cnt;
    uint32_t rx_cnt;
    int8_t last_rssi;
    uint16_t last_rx_tc;
} rf_task_status_t;

void rf_task_init(void);
void rf_task_set_tg_id(uint8_t tg_id);
uint8_t rf_task_get_tg_id(void);
void rf_task_get_status(rf_task_status_t *out);

#endif /* __RF_TASK_H */
