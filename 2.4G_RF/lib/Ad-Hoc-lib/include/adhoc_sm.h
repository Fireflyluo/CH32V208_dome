#ifndef ADHOC_SM_H
#define ADHOC_SM_H

#include "adhoc_frame.h"
#include "adhoc_reply_list.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ADHOC_SM_GATEWAY_ID_MAX 1000000u
#define ADHOC_SM_BEACON_ID_MIN 1000001u
#define ADHOC_SM_DEFAULT_RETRY_MAX 30u
#define ADHOC_SM_DEFAULT_NETWORK_WINDOW_US 30000000u
#define ADHOC_SM_RSSI_STRONG_MIN_DBM (-65)
#define ADHOC_SM_RSSI_MEDIUM_MIN_DBM (-80)
#define ADHOC_SM_JOIN_LEVEL_MAX 254u
#define ADHOC_SM_UPSTREAM_BINDING_CAPACITY 6u
#define ADHOC_SM_NEIGHBOR_CACHE_CAPACITY 6u
#define ADHOC_SM_UPSTREAM_LOSS_CYCLES 3u

typedef enum
{
    ADHOC_SM_STATE_ST0 = 0u,
    ADHOC_SM_STATE_ST1 = 1u,
    ADHOC_SM_STATE_U1 = 2u,
    ADHOC_SM_STATE_C1 = 3u,
    ADHOC_SM_STATE_UN = 4u,
    ADHOC_SM_STATE_CN = 5u
} adhoc_sm_state_t;

typedef enum
{
    ADHOC_SM_RX_IGNORED = 0,
    ADHOC_SM_RX_ACCEPTED = 1,
    ADHOC_SM_RX_STATE_CHANGED = 2
} adhoc_sm_rx_result_t;

typedef enum
{
    ADHOC_SM_ROLE_BEACON = 0u,
    ADHOC_SM_ROLE_GATEWAY = 1u
} adhoc_sm_role_t;

typedef struct
{
    uint8_t role;
    uint16_t domain_id;
    uint32_t node_id;
    uint8_t gateway_no;
    uint32_t t2_us;
    uint32_t t5_us;
    uint8_t retry_max;
    uint32_t network_window_us;
    uint32_t regroup_interval_us;
} adhoc_sm_cfg_t;

typedef struct
{
    uint8_t msg_class;
    uint8_t gateway_no;
    uint8_t level;
    adhoc_sender_t sender;
    uint8_t payload[ADHOC_FRAME_PAYLOAD_LEN];
    int8_t rssi;
    uint32_t ts_us;
} adhoc_sm_rx_event_t;

typedef struct
{
    adhoc_sm_state_t state;
    uint8_t joined_level;
    uint32_t upstream_id;
    uint8_t upstream_no;
    uint8_t upstream_gateway_no;
    uint8_t retry_count;
    uint32_t upstream_last_seen_us;
    uint8_t gateway_network_started;
    uint8_t gateway_network_locked;
    uint32_t gateway_network_start_us;
    uint32_t gateway_network_end_us;
    uint8_t network_lock_active;
    uint8_t network_lock_closed;
    uint32_t network_lock_end_us;
} adhoc_sm_snapshot_t;

typedef struct
{
    uint8_t active;
    uint8_t upstream_level;
    uint8_t upstream_no;
    uint8_t gateway_no;
    uint32_t upstream_id;
    uint8_t signal_rank;
    uint8_t required_hits;
    uint8_t window_cycles;
    uint8_t success_hits;
    int8_t last_rssi;
    uint32_t first_cycle_idx;
    uint32_t last_hit_cycle_idx;
    uint32_t last_seen_us;
    uint32_t network_end_us;
    uint8_t has_last_hit_cycle;
} adhoc_sm_candidate_t;

typedef struct
{
    uint8_t used;
    uint8_t upstream_no;
    uint8_t gateway_no;
    uint8_t upstream_level;
    uint32_t upstream_id;
} adhoc_sm_upstream_binding_t;

typedef struct
{
    uint8_t inited;
    adhoc_sm_cfg_t cfg;
    uint8_t role;
    adhoc_sm_state_t state;
    uint8_t joined_level;
    uint32_t upstream_id;
    uint8_t upstream_no;
    uint8_t upstream_gateway_no;
    uint8_t retry_count;
    uint32_t next_tx_us;
    uint32_t upstream_last_seen_us;
    uint8_t gateway_network_started;
    uint8_t gateway_network_locked;
    uint32_t gateway_network_start_us;
    uint32_t gateway_network_end_us;
    uint8_t gateway_rx_window_open;
    uint32_t gateway_rx_window_start_us;
    uint32_t gateway_rx_window_end_us;
    uint8_t network_lock_active;
    uint8_t network_lock_closed;
    uint32_t network_lock_end_us;
    uint8_t regroup_timer_active;
    uint32_t regroup_start_us;
    adhoc_sm_candidate_t candidate;
    adhoc_sm_candidate_t neighbor_cache[ADHOC_SM_NEIGHBOR_CACHE_CAPACITY];
    adhoc_sm_upstream_binding_t upstream_bindings[ADHOC_SM_UPSTREAM_BINDING_CAPACITY];
    adhoc_reply_list_t downlink_confirm_list;
} adhoc_sm_t;

int adhoc_sm_init(adhoc_sm_t *sm, const adhoc_sm_cfg_t *cfg);
int adhoc_sm_set_role(adhoc_sm_t *sm, uint8_t role);
void adhoc_sm_reset(adhoc_sm_t *sm);
adhoc_sm_rx_result_t adhoc_sm_on_rx(adhoc_sm_t *sm, const adhoc_sm_rx_event_t *event);
int adhoc_sm_poll(adhoc_sm_t *sm, uint32_t now_us, adhoc_frame_fields_t *out_fields, uint8_t *out_has_tx);
void adhoc_sm_get_snapshot(const adhoc_sm_t *sm, adhoc_sm_snapshot_t *out_snapshot);

#ifdef __cplusplus
}
#endif

#endif /* ADHOC_SM_H */
