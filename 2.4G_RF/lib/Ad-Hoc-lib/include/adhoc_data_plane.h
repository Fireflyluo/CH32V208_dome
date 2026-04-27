#ifndef ADHOC_DATA_PLANE_H
#define ADHOC_DATA_PLANE_H

#include "adhoc_frame.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ADHOC_DATA_MSG_USER_LEN 18u
#define ADHOC_DATA_ACK_MAX_PER_FRAME 4u
#define ADHOC_DATA_DEDUP_CAPACITY 64u
#define ADHOC_DATA_ACK_QUEUE_CAPACITY 32u
#define ADHOC_DATA_TX_QUEUE_CAPACITY 16u
#define ADHOC_DATA_TX_REPORT_QUEUE_CAPACITY 16u
#define ADHOC_DATA_TX_RETRY_MAX 30u
#define ADHOC_DATA_DEFAULT_DEDUP_WINDOW_MS (6u * 60u * 60u * 1000u)
#define ADHOC_DATA_GATEWAY_ID_MAX 1000000u

typedef enum
{
    ADHOC_DATA_ORIGIN_UNKNOWN = 0u,
    ADHOC_DATA_ORIGIN_SOURCE = 1u,
    ADHOC_DATA_ORIGIN_FORWARD = 2u
} adhoc_data_origin_t;

typedef enum
{
    ADHOC_DATA_RX_IGNORED = 0u,
    ADHOC_DATA_RX_ACCEPTED = 1u,
    ADHOC_DATA_RX_DUPLICATE = 2u,
    ADHOC_DATA_RX_ACK = 3u
} adhoc_data_rx_result_t;

typedef struct
{
    adhoc_payload_id_t source;
    uint16_t seq_no;
    uint8_t user[ADHOC_DATA_MSG_USER_LEN];
} adhoc_data_msg_t;

typedef struct
{
    adhoc_payload_id_t source;
    uint16_t seq_no;
} adhoc_data_ack_item_t;

typedef enum
{
    ADHOC_DATA_TX_REPORT_NONE = 0u,
    ADHOC_DATA_TX_REPORT_ACKED = 1u,
    ADHOC_DATA_TX_REPORT_RETRY_EXHAUSTED = 2u
} adhoc_data_tx_report_code_t;

typedef struct
{
    uint8_t code;
    adhoc_data_ack_item_t item;
    uint8_t retry_count;
} adhoc_data_tx_report_t;

typedef struct
{
    uint16_t domain_id;
    uint32_t node_id;
    uint8_t gateway_no;
    uint8_t role_gateway;
    uint32_t t5_us;
    uint32_t dedup_window_ms;
    uint8_t tx_retry_max;
} adhoc_data_plane_cfg_t;

typedef struct
{
    uint8_t used;
    adhoc_payload_id_t source;
    uint16_t seq_no;
    uint32_t last_seen_ms;
} adhoc_data_dedup_entry_t;

typedef struct
{
    uint8_t used;
    adhoc_data_ack_item_t item;
} adhoc_data_ack_queue_entry_t;

typedef struct
{
    uint8_t used;
    adhoc_data_msg_t msg;
    uint8_t level;
    uint8_t gateway_no;
    uint8_t retry_count;
    uint32_t next_tx_us;
} adhoc_data_tx_entry_t;

typedef struct
{
    uint8_t used;
    adhoc_data_tx_report_t report;
} adhoc_data_tx_report_entry_t;

typedef struct
{
    uint8_t inited;
    adhoc_data_plane_cfg_t cfg;
    uint8_t role_gateway;
    adhoc_data_dedup_entry_t dedup[ADHOC_DATA_DEDUP_CAPACITY];
    adhoc_data_ack_queue_entry_t ack_queue[ADHOC_DATA_ACK_QUEUE_CAPACITY];
    uint8_t ack_head;
    uint8_t ack_tail;
    uint8_t ack_count;
    uint32_t next_ack_tx_us;
    adhoc_data_tx_entry_t tx_queue[ADHOC_DATA_TX_QUEUE_CAPACITY];
    adhoc_data_tx_report_entry_t tx_report_queue[ADHOC_DATA_TX_REPORT_QUEUE_CAPACITY];
    uint8_t tx_report_head;
    uint8_t tx_report_tail;
    uint8_t tx_report_count;
    uint8_t joined_level;
    uint8_t upstream_no;
    uint8_t upstream_gateway_no;
    uint8_t has_last_rx_data;
    adhoc_data_msg_t last_rx_data;
    adhoc_data_origin_t last_rx_origin;
    uint8_t last_rx_duplicate;
    uint8_t last_ack_hit_count;
    uint8_t last_rx_ack_count;
    adhoc_data_ack_item_t last_rx_acks[ADHOC_DATA_ACK_MAX_PER_FRAME];
} adhoc_data_plane_t;

int adhoc_data_msg_pack(const adhoc_data_msg_t *msg, uint8_t payload_out[ADHOC_FRAME_PAYLOAD_LEN]);
int adhoc_data_msg_unpack(const uint8_t payload[ADHOC_FRAME_PAYLOAD_LEN], adhoc_data_msg_t *msg_out);
int adhoc_data_ack_payload_pack(const adhoc_data_ack_item_t *items, uint8_t item_count,
                                uint8_t payload_out[ADHOC_FRAME_PAYLOAD_LEN]);
int adhoc_data_ack_payload_unpack(const uint8_t payload[ADHOC_FRAME_PAYLOAD_LEN],
                                  adhoc_data_ack_item_t items_out[ADHOC_DATA_ACK_MAX_PER_FRAME],
                                  uint8_t *item_count_out);

int adhoc_data_plane_init(adhoc_data_plane_t *plane, const adhoc_data_plane_cfg_t *cfg);
int adhoc_data_plane_set_role(adhoc_data_plane_t *plane, uint8_t role_gateway);
int adhoc_data_plane_set_route(adhoc_data_plane_t *plane, uint8_t joined_level, uint8_t upstream_gateway_no, uint8_t upstream_no);
void adhoc_data_plane_reset(adhoc_data_plane_t *plane);
int adhoc_data_plane_submit_source_data(adhoc_data_plane_t *plane, uint8_t source_id_flag, uint16_t seq_no,
                                        const uint8_t user[ADHOC_DATA_MSG_USER_LEN], uint8_t level, uint8_t gateway_no,
                                        uint32_t now_us);
int adhoc_data_plane_pop_tx_report(adhoc_data_plane_t *plane, adhoc_data_tx_report_t *out_report);
adhoc_data_rx_result_t adhoc_data_plane_on_rx(adhoc_data_plane_t *plane, const adhoc_frame_fields_t *fields, uint32_t ts_us);
int adhoc_data_plane_poll_gateway_ack(adhoc_data_plane_t *plane, uint32_t now_us, adhoc_frame_fields_t *out_fields, uint8_t *out_has_tx);
int adhoc_data_plane_poll_forward_tx(adhoc_data_plane_t *plane, uint32_t now_us, adhoc_frame_fields_t *out_fields, uint8_t *out_has_tx);

#ifdef __cplusplus
}
#endif

#endif /* ADHOC_DATA_PLANE_H */
