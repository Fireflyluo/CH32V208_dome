#ifndef __PROTOCOL_TASK_H
#define __PROTOCOL_TASK_H

#include <stdint.h>

typedef struct {
    uint8_t upload_active;
    uint8_t last_tx_type; /* ascii char */
    uint8_t last_tx_seq;  /* 0..63 */
    uint8_t params_valid;

    uint16_t sample_count;
    uint16_t sample_capacity;
    uint16_t cfg_sample_ms;
    uint16_t cfg_threshold_high; /* 12-bit */
    uint16_t cfg_threshold_low;  /* 12-bit */

    uint16_t upload_cursor;
    uint16_t upload_total;
    uint16_t last_accel12;
    uint8_t last_over_threshold;

    uint16_t tx_cnt_n;
    uint16_t tx_cnt_m;
    uint16_t tx_cnt_i;
    uint16_t tx_cnt_h;
    uint16_t tx_cnt_q;
} protocol_status_t;

void protocol_task_init(void);
void protocol_task_get_status(protocol_status_t *out);

#endif /* __PROTOCOL_TASK_H */
