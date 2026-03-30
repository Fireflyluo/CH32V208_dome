#include "protocol_task.h"

#include "board.h"
#include "data_protocol.h"
#include "log_print.h"
#include "ringbuffer.h"
#include "sensor_task.h"
#include "usb_cdc.h"
#include "wchble.h"

#include <string.h>

#define PROTOCOL_EVT_INIT   (0x0001u << 0)
#define PROTOCOL_EVT_POLL   (0x0001u << 1)
#define PROTOCOL_EVT_SAMPLE (0x0001u << 2)

#define PROTOCOL_POLL_MS         20u
#define PROTOCOL_SAMPLE_TICK_MS  20u
#define PROTOCOL_DEFAULT_SMP_MS  100u
#define PROTOCOL_TX_RETRY_MAX    1u
#define PROTOCOL_TX_RETRY_DELAY_MS 0u

#define PROTOCOL_RX_BUF_SIZE      PROTOCOL_BUFFER_SIZE
#define PROTOCOL_TX_FRAME_MAX     PROTOCOL_MAX_FRAME_LEN
#define PROTOCOL_SAMPLE_CAPACITY  128u
#define PROTOCOL_UPLOAD_MAX_ACCEL 120u

#define REPORT_FIRST_ACCEL_CAP 16u
#define REPORT_NEXT_ACCEL_CAP  16u

typedef struct {
    uint16_t temperature_12;
    uint16_t humidity_12;
    uint16_t accel_12;
} protocol_sample_t;

typedef struct {
    uint8_t active;
    uint64_t tag_id;
    uint8_t start_hour;
    uint8_t start_minute;
    uint8_t start_second;
    uint16_t temp12;
    uint16_t hum12;
    uint16_t accel[PROTOCOL_UPLOAD_MAX_ACCEL];
    uint16_t total_accel;
    uint16_t cursor;
    uint32_t chunk_seq;
} upload_ctx_t;

static tmosTaskID s_protocol_task_id = INVALID_TASK_ID;
static char s_rx_buf[PROTOCOL_RX_BUF_SIZE];
static uint16_t s_rx_len = 0u;

static uint8_t s_tx_seq = 0u;
static char s_last_report_frame[PROTOCOL_TX_FRAME_MAX + 2u];
static uint16_t s_last_report_len = 0u;
static uint8_t s_last_report_valid = 0u;
static char s_pending_tx_frame[PROTOCOL_TX_FRAME_MAX + 2u];
static uint16_t s_pending_tx_len = 0u;
static uint16_t s_pending_tx_sent = 0u;
static uint8_t s_pending_tx_valid = 0u;

static upload_ctx_t s_upload;

static ringbuffer_t s_sample_rb;
static uint8_t s_sample_storage[PROTOCOL_SAMPLE_CAPACITY * sizeof(protocol_sample_t)];
static uint16_t s_sample_count = 0u;

static param_data_decoded_t s_last_params;
static uint8_t s_last_params_valid = 0u;

static uint16_t s_cfg_sample_ms = PROTOCOL_DEFAULT_SMP_MS;
static uint16_t s_cfg_threshold_high = 3000u;
static uint16_t s_cfg_threshold_low = 100u;

static uint32_t s_last_sample_tick = 0u;
static uint16_t s_last_accel12 = 0u;
static uint8_t s_last_over_threshold = 0u;

static uint8_t s_last_tx_type = 0u;
static uint8_t s_last_tx_seq = 0u;

static uint16_t s_tx_cnt_n = 0u;
static uint16_t s_tx_cnt_m = 0u;
static uint16_t s_tx_cnt_i = 0u;
static uint16_t s_tx_cnt_h = 0u;
static uint16_t s_tx_cnt_q = 0u;

static tmosEvents protocol_task_process_event(tmosTaskID task_id, tmosEvents events);

static uint8_t protocol_next_tx_seq(void);
static uint8_t protocol_send_frame(const char *frame, uint16_t frame_len);
static uint8_t protocol_flush_pending_tx(void);
static void protocol_send_msg(msg_type_t type, const char *content, uint16_t content_len, uint8_t cache_as_report);
static void protocol_reply_none(void);
static void protocol_reply_middle(void);
static void protocol_reply_repeat_last_or_none(void);

static void protocol_poll_rx_and_process(void);
static void protocol_process_one_frame(const char *frame, uint16_t frame_len);

static uint16_t protocol_snapshot_to_accel12(const sensor_snapshot_t *snap);
static void protocol_sample_once(void);
static uint8_t protocol_try_build_upload(void);
static void protocol_send_next_upload_chunk(void);

void protocol_task_init(void)
{
    if (s_protocol_task_id != INVALID_TASK_ID)
    {
        return;
    }

    s_protocol_task_id = TMOS_ProcessEventRegister(protocol_task_process_event);
    if (s_protocol_task_id == INVALID_TASK_ID)
    {
        LOG_PRINT("protocol task register failed\r\n");
        return;
    }

    s_rx_len = 0u;
    s_tx_seq = 0u;
    s_last_report_valid = 0u;
    s_pending_tx_valid = 0u;
    s_pending_tx_len = 0u;
    s_pending_tx_sent = 0u;
    s_last_params_valid = 0u;
    s_sample_count = 0u;
    s_last_sample_tick = HAL_GetTick();
    s_last_tx_type = 0u;
    s_last_tx_seq = 0u;
    s_tx_cnt_n = 0u;
    s_tx_cnt_m = 0u;
    s_tx_cnt_i = 0u;
    s_tx_cnt_h = 0u;
    s_tx_cnt_q = 0u;
    memset(&s_upload, 0, sizeof(s_upload));

    ringbuffer_init(&s_sample_rb, s_sample_storage, sizeof(s_sample_storage));

    tmos_set_event(s_protocol_task_id, PROTOCOL_EVT_INIT);
}

void protocol_task_get_status(protocol_status_t *out)
{
    if (out == NULL)
    {
        return;
    }

    out->upload_active = s_upload.active;
    out->last_tx_type = s_last_tx_type;
    out->last_tx_seq = s_last_tx_seq;
    out->params_valid = s_last_params_valid;

    out->sample_count = s_sample_count;
    out->sample_capacity = PROTOCOL_SAMPLE_CAPACITY;
    out->cfg_sample_ms = s_cfg_sample_ms;
    out->cfg_threshold_high = s_cfg_threshold_high;
    out->cfg_threshold_low = s_cfg_threshold_low;

    out->upload_cursor = s_upload.cursor;
    out->upload_total = s_upload.total_accel;
    out->last_accel12 = s_last_accel12;
    out->last_over_threshold = s_last_over_threshold;

    out->tx_cnt_n = s_tx_cnt_n;
    out->tx_cnt_m = s_tx_cnt_m;
    out->tx_cnt_i = s_tx_cnt_i;
    out->tx_cnt_h = s_tx_cnt_h;
    out->tx_cnt_q = s_tx_cnt_q;
}

static uint8_t protocol_next_tx_seq(void)
{
    uint8_t seq = s_tx_seq;
    s_tx_seq = (uint8_t)((s_tx_seq + 1u) & 0x3Fu);
    return seq;
}

static uint8_t protocol_send_frame(const char *frame, uint16_t frame_len)
{
    uint16_t sent = 0u;

    if (frame == NULL || frame_len == 0u)
    {
        return 0u;
    }
    if (s_pending_tx_valid != 0u)
    {
        return 0u;
    }

    while (sent < frame_len)
    {
        uint16_t chunk = (uint16_t)(frame_len - sent);
        CDC_ErrCode_t rc = CDC_SUCCESS;
        uint8_t retry = 0u;

        if (chunk > CDC_MAX_PACKET_SIZE)
        {
            chunk = CDC_MAX_PACKET_SIZE;
        }

        while (retry < PROTOCOL_TX_RETRY_MAX)
        {
            rc = CDC_SendData((uint8_t *)&frame[sent], chunk);
            if (rc == CDC_SUCCESS)
            {
                break;
            }
            retry++;
            HAL_Delay(PROTOCOL_TX_RETRY_DELAY_MS);
        }

        if (rc != CDC_SUCCESS)
        {
            if (frame_len <= sizeof(s_pending_tx_frame))
            {
                memcpy(s_pending_tx_frame, frame, frame_len);
                s_pending_tx_len = frame_len;
                s_pending_tx_sent = sent;
                s_pending_tx_valid = 1u;
            }
            LOG_PRINT("protocol tx busy/not-ready rc=%d, queue %u/%u\r\n", (int)rc, (unsigned)sent, (unsigned)frame_len);
            return 0u;
        }

        sent = (uint16_t)(sent + chunk);
    }

    return 1u;
}

static uint8_t protocol_flush_pending_tx(void)
{
    uint16_t sent;

    if (s_pending_tx_valid == 0u)
    {
        return 1u;
    }

    sent = s_pending_tx_sent;
    while (sent < s_pending_tx_len)
    {
        uint16_t chunk = (uint16_t)(s_pending_tx_len - sent);
        CDC_ErrCode_t rc = CDC_SUCCESS;
        uint8_t retry = 0u;

        if (chunk > CDC_MAX_PACKET_SIZE)
        {
            chunk = CDC_MAX_PACKET_SIZE;
        }

        while (retry < PROTOCOL_TX_RETRY_MAX)
        {
            rc = CDC_SendData((uint8_t *)&s_pending_tx_frame[sent], chunk);
            if (rc == CDC_SUCCESS)
            {
                break;
            }
            retry++;
            HAL_Delay(PROTOCOL_TX_RETRY_DELAY_MS);
        }

        if (rc != CDC_SUCCESS)
        {
            s_pending_tx_sent = sent;
            return 0u;
        }

        sent = (uint16_t)(sent + chunk);
    }

    s_pending_tx_valid = 0u;
    s_pending_tx_len = 0u;
    s_pending_tx_sent = 0u;
    return 1u;
}

static void protocol_send_msg(msg_type_t type, const char *content, uint16_t content_len, uint8_t cache_as_report)
{
    char frame[PROTOCOL_TX_FRAME_MAX + 2u];
    uint16_t len;
    uint8_t tx_seq = protocol_next_tx_seq();

    len = build_frame(frame, sizeof(frame), type, tx_seq, content, content_len);
    if (len == 0u)
    {
        return;
    }

    if (protocol_send_frame(frame, len) == 0u)
    {
        return;
    }

    s_last_tx_type = (uint8_t)type;
    s_last_tx_seq = tx_seq;
    if (type == MSG_REPORT_NONE)
    {
        s_tx_cnt_n++;
    }
    else if (type == MSG_REPORT_MIDDLE)
    {
        s_tx_cnt_m++;
    }
    else if (type == MSG_ACK_PARAMS)
    {
        s_tx_cnt_q++;
    }

    if (cache_as_report != 0u)
    {
        if (len <= sizeof(s_last_report_frame))
        {
            memcpy(s_last_report_frame, frame, len);
            s_last_report_len = len;
            s_last_report_valid = 1u;
        }
    }
}

static void protocol_reply_none(void)
{
    protocol_send_msg(MSG_REPORT_NONE, NULL, 0u, 1u);
}

static void protocol_reply_middle(void)
{
    protocol_send_msg(MSG_REPORT_MIDDLE, NULL, 0u, 1u);
}

static void protocol_reply_repeat_last_or_none(void)
{
    if (s_last_report_valid != 0u && s_last_report_len > 0u)
    {
        protocol_send_frame(s_last_report_frame, s_last_report_len);
        return;
    }

    protocol_reply_none();
}

static uint16_t protocol_snapshot_to_accel12(const sensor_snapshot_t *snap)
{
    int32_t ax;
    int32_t ay;
    int32_t az;
    int32_t peak;
    uint32_t peak_u;
    uint32_t full_scale_mg = 2000u; /* sensor range = 2g */

    if (snap == NULL)
    {
        return 0u;
    }

    ax = (snap->accel_mg_x >= 0) ? snap->accel_mg_x : -snap->accel_mg_x;
    ay = (snap->accel_mg_y >= 0) ? snap->accel_mg_y : -snap->accel_mg_y;
    az = (snap->accel_mg_z >= 0) ? snap->accel_mg_z : -snap->accel_mg_z;

    peak = ax;
    if (ay > peak)
    {
        peak = ay;
    }
    if (az > peak)
    {
        peak = az;
    }

    peak_u = (uint32_t)peak;
    if (peak_u > full_scale_mg)
    {
        peak_u = full_scale_mg;
    }

    return (uint16_t)((peak_u * 4095u) / full_scale_mg);
}

static void protocol_sample_push(const protocol_sample_t *s)
{
    if (s == NULL)
    {
        return;
    }

    (void)ringbuffer_put_force(&s_sample_rb, (const uint8_t *)s, sizeof(*s));
    if (s_sample_count < PROTOCOL_SAMPLE_CAPACITY)
    {
        s_sample_count++;
    }
}

static uint8_t protocol_sample_pop(protocol_sample_t *s)
{
    if (s == NULL)
    {
        return 0u;
    }

    if (ringbuffer_get(&s_sample_rb, (uint8_t *)s, sizeof(*s)) != sizeof(*s))
    {
        return 0u;
    }

    if (s_sample_count > 0u)
    {
        s_sample_count--;
    }
    return 1u;
}

static void protocol_sample_once(void)
{
    sensor_snapshot_t snap;
    protocol_sample_t s;
    uint32_t now_tick = HAL_GetTick();

    if ((uint32_t)(now_tick - s_last_sample_tick) < s_cfg_sample_ms)
    {
        return;
    }
    s_last_sample_tick = now_tick;

    sensor_task_get_snapshot(&snap);
    if (snap.ready == 0u)
    {
        return;
    }

    s.temperature_12 = temperature_to_12bit((float)snap.temp_centi_c / 100.0f);
    s.humidity_12 = humidity_to_12bit((float)snap.rh_centi_pct / 100.0f);
    s.accel_12 = protocol_snapshot_to_accel12(&snap);

    s_last_accel12 = s.accel_12;
    s_last_over_threshold = (uint8_t)((s.accel_12 >= s_cfg_threshold_high || s.accel_12 <= s_cfg_threshold_low) ? 1u : 0u);

    protocol_sample_push(&s);
}

static uint8_t protocol_try_build_upload(void)
{
    protocol_sample_t s;
    uint16_t i = 0u;
    uint32_t sec_from_boot;

    if (s_upload.active != 0u)
    {
        return 1u;
    }

    while (i < PROTOCOL_UPLOAD_MAX_ACCEL)
    {
        if (protocol_sample_pop(&s) == 0u)
        {
            break;
        }

        if (i == 0u)
        {
            s_upload.temp12 = s.temperature_12;
            s_upload.hum12 = s.humidity_12;
        }

        s_upload.accel[i] = s.accel_12;
        i++;
    }

    if (i == 0u)
    {
        return 0u;
    }

    sec_from_boot = HAL_GetTick() / 1000u;

    s_upload.active = 1u;
    s_upload.tag_id = 0x123456789ull;
    s_upload.start_hour = (uint8_t)((sec_from_boot / 3600u) % 24u);
    s_upload.start_minute = (uint8_t)((sec_from_boot / 60u) % 60u);
    s_upload.start_second = (uint8_t)(sec_from_boot % 60u);
    s_upload.total_accel = i;
    s_upload.cursor = 0u;
    s_upload.chunk_seq = 1u;

    return 1u;
}

static void protocol_send_next_upload_chunk(void)
{
    report_data_decoded_t rep;
    uint16_t remain;
    uint16_t take;
    msg_type_t out_type;

    if (s_upload.active == 0u)
    {
        protocol_reply_none();
        return;
    }

    remain = (uint16_t)(s_upload.total_accel - s_upload.cursor);
    if (remain == 0u)
    {
        s_upload.active = 0u;
        protocol_reply_none();
        return;
    }

    if (s_upload.cursor == 0u)
    {
        take = (remain > REPORT_FIRST_ACCEL_CAP) ? REPORT_FIRST_ACCEL_CAP : remain;
    }
    else
    {
        take = (remain > REPORT_NEXT_ACCEL_CAP) ? REPORT_NEXT_ACCEL_CAP : remain;
    }

    rep.tag_id = s_upload.tag_id;
    rep.start_hour = s_upload.start_hour;
    rep.start_minute = s_upload.start_minute;
    rep.start_second = s_upload.start_second;
    rep.sequence = s_upload.chunk_seq;
    rep.temperature = s_upload.temp12;
    rep.humidity = s_upload.hum12;
    rep.acceleration = &s_upload.accel[s_upload.cursor];
    rep.accel_count = take;
    rep.has_temp_humidity = (s_upload.cursor == 0u) ? true : false;

    out_type = ((uint16_t)(s_upload.cursor + take) >= s_upload.total_accel) ? MSG_REPORT_LAST : MSG_REPORT_FIRST;

    {
        char frame[PROTOCOL_TX_FRAME_MAX + 2u];
        uint16_t len = build_report_msg(frame, sizeof(frame), protocol_next_tx_seq(), out_type, &rep);
        if (len == 0u)
        {
            s_upload.active = 0u;
            protocol_reply_none();
            return;
        }

        if (protocol_send_frame(frame, len) == 0u)
        {
            return;
        }
        s_last_tx_type = (uint8_t)out_type;
        s_last_tx_seq = (uint8_t)((s_tx_seq + 63u) & 0x3Fu);
        if (out_type == MSG_REPORT_FIRST)
        {
            s_tx_cnt_i++;
        }
        else if (out_type == MSG_REPORT_LAST)
        {
            s_tx_cnt_h++;
        }

        if (len <= sizeof(s_last_report_frame))
        {
            memcpy(s_last_report_frame, frame, len);
            s_last_report_len = len;
            s_last_report_valid = 1u;
        }
    }

    s_upload.cursor = (uint16_t)(s_upload.cursor + take);
    s_upload.chunk_seq++;

    if (s_upload.cursor >= s_upload.total_accel)
    {
        s_upload.active = 0u;
    }
}

static void protocol_apply_params(const param_data_decoded_t *p)
{
    uint32_t sample_ms;

    if (p == NULL)
    {
        return;
    }

    sample_ms = p->T2 & 0xFFFFFFu;
    if (sample_ms < 20u)
    {
        sample_ms = 20u;
    }
    if (sample_ms > 2000u)
    {
        sample_ms = 2000u;
    }

    s_cfg_sample_ms = (uint16_t)sample_ms;
    s_cfg_threshold_high = (uint16_t)(p->threshold_high & 0x0FFFu);
    s_cfg_threshold_low = (uint16_t)(p->threshold_low & 0x0FFFu);

    if (s_cfg_threshold_low > s_cfg_threshold_high)
    {
        uint16_t t = s_cfg_threshold_low;
        s_cfg_threshold_low = s_cfg_threshold_high;
        s_cfg_threshold_high = t;
    }

    /* 参数更新后重置上传上下文与样本队列，避免旧配置数据混入 */
    ringbuffer_init(&s_sample_rb, s_sample_storage, sizeof(s_sample_storage));
    s_sample_count = 0u;
    memset(&s_upload, 0, sizeof(s_upload));
    s_last_report_valid = 0u;
}

static void protocol_process_one_frame(const char *frame, uint16_t frame_len)
{
    msg_type_t type = MSG_UNKNOWN;
    uint8_t seq = 0u;
    uint32_t crc_received = 0u;
    char content[PROTOCOL_MAX_FRAME_LEN + 1u];
    uint16_t content_len = 0u;
    frame_status_t st;

    st = parse_frame(frame, frame_len, &type, &seq, content, &content_len, &crc_received);
    if (st != FRAME_OK)
    {
        LOG_PRINT("protocol parse err=%d\r\n", (int)st);
        return;
    }

    (void)seq;
    (void)crc_received;

    if (type == MSG_SET_PARAMS)
    {
        param_data_decoded_t params;
        char ack[PROTOCOL_MAX_FRAME_LEN + 16u];
        uint16_t ack_len;

        if (!parse_param_data(content, content_len, &params))
        {
            LOG_PRINT("protocol P content invalid\r\n");
            return;
        }

        s_last_params = params;
        s_last_params_valid = 1u;
        protocol_apply_params(&params);

        ack_len = build_ack_msg(ack, sizeof(ack), protocol_next_tx_seq(), content, content_len);
        if (ack_len > 0u)
        {
            if (protocol_send_frame(ack, ack_len) == 0u)
            {
                return;
            }
            s_last_tx_type = (uint8_t)MSG_ACK_PARAMS;
            s_last_tx_seq = (uint8_t)((s_tx_seq + 63u) & 0x3Fu);
            s_tx_cnt_q++;
        }
        return;
    }

    if (type == MSG_QUERY)
    {
        if (s_upload.active == 0u)
        {
            if (s_sample_count == 0u)
            {
                protocol_reply_none();
                return;
            }

            if (s_sample_count < REPORT_FIRST_ACCEL_CAP)
            {
                protocol_reply_middle();
                return;
            }

            if (protocol_try_build_upload() == 0u)
            {
                protocol_reply_none();
                return;
            }
        }

        protocol_send_next_upload_chunk();
        return;
    }

    if (type == MSG_REPEAT_QUERY)
    {
        protocol_reply_repeat_last_or_none();
        return;
    }
}

static void protocol_poll_rx_and_process(void)
{
    uint8_t chunk[CDC_MAX_PACKET_SIZE];
    uint16_t n;
    uint16_t frame_start;
    uint16_t frame_end;

    for (;;)
    {
        n = CDC_ReceiveData(chunk, sizeof(chunk));
        if (n == 0u)
        {
            break;
        }

        if ((uint16_t)(s_rx_len + n) > sizeof(s_rx_buf))
        {
            s_rx_len = 0u;
        }

        memcpy(&s_rx_buf[s_rx_len], chunk, n);
        s_rx_len = (uint16_t)(s_rx_len + n);
    }

    while (is_frame_complete(s_rx_buf, s_rx_len, &frame_start, &frame_end))
    {
        uint16_t frame_len = (uint16_t)(frame_end - frame_start + 1u);
        char frame_copy[PROTOCOL_MAX_FRAME_LEN + 2u];
        uint16_t remove_len = (uint16_t)(frame_end + 1u);

        if (frame_len > PROTOCOL_MAX_FRAME_LEN)
        {
            if (s_rx_len > remove_len)
            {
                memmove(s_rx_buf, &s_rx_buf[remove_len], (size_t)(s_rx_len - remove_len));
                s_rx_len = (uint16_t)(s_rx_len - remove_len);
            }
            else
            {
                s_rx_len = 0u;
            }
            continue;
        }

        memcpy(frame_copy, &s_rx_buf[frame_start], frame_len);
        frame_copy[frame_len] = '\0';
        protocol_process_one_frame(frame_copy, frame_len);

        if (s_rx_len > remove_len)
        {
            memmove(s_rx_buf, &s_rx_buf[remove_len], (size_t)(s_rx_len - remove_len));
            s_rx_len = (uint16_t)(s_rx_len - remove_len);
        }
        else
        {
            s_rx_len = 0u;
        }

        /* 若存在待续发数据，优先发送完成，避免后续查询被吞掉 */
        if (s_pending_tx_valid != 0u)
        {
            break;
        }
    }
}

static tmosEvents protocol_task_process_event(tmosTaskID task_id, tmosEvents events)
{
    (void)task_id;

    if (events & PROTOCOL_EVT_INIT)
    {
        tmos_start_reload_task(s_protocol_task_id, PROTOCOL_EVT_POLL, MS1_TO_SYSTEM_TIME(PROTOCOL_POLL_MS));
        tmos_start_reload_task(s_protocol_task_id, PROTOCOL_EVT_SAMPLE, MS1_TO_SYSTEM_TIME(PROTOCOL_SAMPLE_TICK_MS));
        return (events ^ PROTOCOL_EVT_INIT);
    }

    if (events & PROTOCOL_EVT_POLL)
    {
        if (protocol_flush_pending_tx() == 0u)
        {
            return (events ^ PROTOCOL_EVT_POLL);
        }
        protocol_poll_rx_and_process();
        (void)protocol_flush_pending_tx();
        return (events ^ PROTOCOL_EVT_POLL);
    }

    if (events & PROTOCOL_EVT_SAMPLE)
    {
        protocol_sample_once();
        return (events ^ PROTOCOL_EVT_SAMPLE);
    }

    return 0;
}
