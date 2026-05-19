#ifndef ADHOC_CHANNEL_H
#define ADHOC_CHANNEL_H

#include <stdint.h>
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ADHOC_CHANNEL_MAX_NODES      64u
#define ADHOC_CHANNEL_RX_FIFO_SIZE   256u
#define ADHOC_FRAME_LEN              32u

typedef struct
{
    uint8_t  frame[ADHOC_FRAME_LEN];
    uint8_t  len;
    int8_t   rssi;
    uint8_t  used;
} adhoc_channel_rx_entry_t;

typedef struct
{
    adhoc_channel_rx_entry_t fifo[ADHOC_CHANNEL_RX_FIFO_SIZE];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} adhoc_channel_node_rx_t;

typedef struct
{
    uint8_t  max_nodes;
    uint8_t  topo[ADHOC_CHANNEL_MAX_NODES][ADHOC_CHANNEL_MAX_NODES];
    adhoc_channel_node_rx_t nodes[ADHOC_CHANNEL_MAX_NODES];
    uint8_t  drop_rate_pct;
    CRITICAL_SECTION lock;
} adhoc_channel_t;

void adhoc_channel_init(adhoc_channel_t *ch, uint8_t max_nodes);
void adhoc_channel_deinit(adhoc_channel_t *ch);
void adhoc_channel_set_topo(adhoc_channel_t *ch, uint8_t from_idx, uint8_t to_idx, int8_t rssi);
void adhoc_channel_set_drop_rate(adhoc_channel_t *ch, uint8_t drop_rate_pct);
int  adhoc_channel_tx(adhoc_channel_t *ch, uint8_t from_idx, const uint8_t *frame, uint8_t len);
int  adhoc_channel_poll_rx(adhoc_channel_t *ch, uint8_t node_idx, uint8_t *frame_out, uint8_t *len_out, int8_t *rssi_out);
void adhoc_channel_reset_node_rx(adhoc_channel_t *ch, uint8_t node_idx);

#ifdef __cplusplus
}
#endif

#endif /* ADHOC_CHANNEL_H */
