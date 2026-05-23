#ifndef __AROS_TAG_H__
#define __AROS_TAG_H__

#include <stdint.h>

#include "aros_rf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TG_MAX_ID 20u
#define TG_MSG_DAT_N 24u

typedef struct
{
    uint8_t tg_id;
    uint8_t synced;
    uint16_t dat_no;
    uint32_t tx_cnt;
    uint32_t rx_cnt;
    int8_t last_rssi;
    uint16_t last_rx_tc;
} tg_runtime_status_t;

void tg_init(uint8_t tg_id);
void tg_set_id(uint8_t tg_id);
uint8_t tg_get_id(void);
void tg_set_newdatfunc(arf_newdat_fc newdatf);
void tg_step(void);
void tg_get_status(tg_runtime_status_t *out);

/* Legacy blocking loop entry kept for compatibility. */
void tg_loop(void);

#ifdef __cplusplus
}
#endif

#endif /* __AROS_TAG_H__ */
