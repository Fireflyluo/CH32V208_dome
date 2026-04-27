#ifndef SC7A20_NEW_CH32_ADAPTER_H
#define SC7A20_NEW_CH32_ADAPTER_H

#include "../inc/sc7a20_core.h"
#include "drv_i2c.h"

typedef struct {
    i2c_num_t i2c_num;
    uint8_t dev_addr;
} sc7a20_ch32_bus_ctx_t;

typedef enum {
    SC7A20_FIFO_DMA_POLICY_FORCE_IT = 0u,
    SC7A20_FIFO_DMA_POLICY_FORCE_DMA = 1u,
    SC7A20_FIFO_DMA_POLICY_AUTO = 2u,
} sc7a20_fifo_dma_policy_t;

typedef struct {
    uint32_t dma_try_cnt;
    uint32_t dma_ok_cnt;
    uint32_t it_fallback_cnt;
    uint32_t it_direct_cnt;
    uint32_t fifo_dma_try_cnt;
    uint32_t fifo_dma_ok_cnt;
    uint32_t fifo_dma_fail_cnt;
    uint32_t fifo_dma_backoff_skip_cnt;
    uint16_t fifo_dma_backoff_left;
    uint8_t fifo_dma_policy;
} sc7a20_ch32_xfer_stats_t;

extern const sc7a20_bus_ops_t g_sc7a20_ch32_i2c_ops;

void sc7a20_ch32_get_xfer_stats(sc7a20_ch32_xfer_stats_t *out);
void sc7a20_ch32_reset_xfer_stats(void);
void sc7a20_ch32_set_fifo_dma_policy(sc7a20_fifo_dma_policy_t policy);
sc7a20_fifo_dma_policy_t sc7a20_ch32_get_fifo_dma_policy(void);

#endif
