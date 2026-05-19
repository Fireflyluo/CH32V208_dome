#include "adhoc_channel.h"

#include <stdlib.h>
#include <string.h>

void adhoc_channel_init(adhoc_channel_t *ch, uint8_t max_nodes)
{
    if (ch == NULL || max_nodes == 0u || max_nodes > ADHOC_CHANNEL_MAX_NODES)
    {
        return;
    }

    memset(ch, 0, sizeof(*ch));
    ch->max_nodes = max_nodes;
    ch->drop_rate_pct = 0u;
    InitializeCriticalSection(&ch->lock);
}

void adhoc_channel_deinit(adhoc_channel_t *ch)
{
    if (ch == NULL)
    {
        return;
    }
    DeleteCriticalSection(&ch->lock);
    memset(ch, 0, sizeof(*ch));
}

void adhoc_channel_set_topo(adhoc_channel_t *ch, uint8_t from_idx, uint8_t to_idx, int8_t rssi)
{
    if (ch == NULL || from_idx >= ch->max_nodes || to_idx >= ch->max_nodes)
    {
        return;
    }

    EnterCriticalSection(&ch->lock);
    if (rssi < -127)
    {
        ch->topo[from_idx][to_idx] = 0u;
    }
    else
    {
        ch->topo[from_idx][to_idx] = (uint8_t)(rssi & 0xFF);
    }
    LeaveCriticalSection(&ch->lock);
}

void adhoc_channel_set_drop_rate(adhoc_channel_t *ch, uint8_t drop_rate_pct)
{
    if (ch == NULL)
    {
        return;
    }
    EnterCriticalSection(&ch->lock);
    ch->drop_rate_pct = drop_rate_pct > 100u ? 100u : drop_rate_pct;
    LeaveCriticalSection(&ch->lock);
}

int adhoc_channel_tx(adhoc_channel_t *ch, uint8_t from_idx, const uint8_t *frame, uint8_t len)
{
    uint8_t to_idx;
    adhoc_channel_node_rx_t *rx;
    int8_t rssi;
    uint8_t drop_rate;

    if (ch == NULL || frame == NULL || len != ADHOC_FRAME_LEN || from_idx >= ch->max_nodes)
    {
        return 0;
    }

    EnterCriticalSection(&ch->lock);

    for (to_idx = 0u; to_idx < ch->max_nodes; ++to_idx)
    {
        if (to_idx == from_idx)
        {
            continue;
        }

        rssi = (int8_t)ch->topo[from_idx][to_idx];
        if (rssi == 0)
        {
            continue;
        }

        drop_rate = ch->drop_rate_pct;
        if (drop_rate != 0u && ((uint8_t)(rand() % 100u) < drop_rate))
        {
            continue;
        }

        rx = &ch->nodes[to_idx];
        if (rx->count >= ADHOC_CHANNEL_RX_FIFO_SIZE)
        {
            continue;
        }

        memcpy(rx->fifo[rx->tail].frame, frame, ADHOC_FRAME_LEN);
        rx->fifo[rx->tail].len = len;
        rx->fifo[rx->tail].rssi = rssi;
        rx->fifo[rx->tail].used = 1u;
        rx->tail = (uint16_t)((rx->tail + 1u) % ADHOC_CHANNEL_RX_FIFO_SIZE);
        rx->count++;
    }

    LeaveCriticalSection(&ch->lock);
    return 1;
}

int adhoc_channel_poll_rx(adhoc_channel_t *ch, uint8_t node_idx, uint8_t *frame_out, uint8_t *len_out, int8_t *rssi_out)
{
    adhoc_channel_node_rx_t *rx;
    adhoc_channel_rx_entry_t *entry;

    if (ch == NULL || frame_out == NULL || node_idx >= ch->max_nodes)
    {
        return 0;
    }

    EnterCriticalSection(&ch->lock);

    rx = &ch->nodes[node_idx];
    if (rx->count == 0u)
    {
        LeaveCriticalSection(&ch->lock);
        return 0;
    }

    entry = &rx->fifo[rx->head];
    if (entry->used == 0u)
    {
        LeaveCriticalSection(&ch->lock);
        return 0;
    }

    memcpy(frame_out, entry->frame, ADHOC_FRAME_LEN);
    if (len_out != NULL)
    {
        *len_out = entry->len;
    }
    if (rssi_out != NULL)
    {
        *rssi_out = entry->rssi;
    }

    memset(entry, 0, sizeof(*entry));
    rx->head = (uint16_t)((rx->head + 1u) % ADHOC_CHANNEL_RX_FIFO_SIZE);
    rx->count--;

    LeaveCriticalSection(&ch->lock);
    return 1;
}

void adhoc_channel_reset_node_rx(adhoc_channel_t *ch, uint8_t node_idx)
{
    if (ch == NULL || node_idx >= ch->max_nodes)
    {
        return;
    }

    EnterCriticalSection(&ch->lock);
    memset(&ch->nodes[node_idx], 0, sizeof(ch->nodes[node_idx]));
    LeaveCriticalSection(&ch->lock);
}
