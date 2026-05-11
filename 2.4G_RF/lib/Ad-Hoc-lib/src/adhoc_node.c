#include "adhoc_api.h"
#include "adhoc_data_plane.h"
#include "adhoc_frame.h"
#include "adhoc_sm.h"
#include "adhoc_timing.h"

#include <string.h>

typedef struct
{
    uint8_t inited;
    adhoc_role_t role;
    adhoc_cfg_t cfg;
    const adhoc_link_ops_t *link_ops;
    void *link_ctx;
    adhoc_frame_t pending_tx;
    uint8_t has_pending_tx;
    adhoc_frame_fields_t last_rx_fields;
    uint8_t has_last_rx;
    adhoc_timing_plan_t timing_plan;
    adhoc_sm_t sm;
    adhoc_data_plane_t data_plane;
} adhoc_node_ctx_t;

static adhoc_node_ctx_t *adhoc_cast(void *node)
{
    return (adhoc_node_ctx_t *)node;
}

static int adhoc_node_sync_route_context(adhoc_node_ctx_t *ctx)
{
    adhoc_sm_snapshot_t snapshot;

    if (ctx == NULL || ctx->inited == 0u)
    {
        return 0;
    }
    if (ctx->role == ADHOC_ROLE_GATEWAY)
    {
        return adhoc_data_plane_set_route(&ctx->data_plane, 0u, 0u, 0u);
    }

    memset(&snapshot, 0, sizeof(snapshot));
    adhoc_sm_get_snapshot(&ctx->sm, &snapshot);
    return adhoc_data_plane_set_route(&ctx->data_plane,
                                      snapshot.joined_level,
                                      snapshot.upstream_gateway_no,
                                      snapshot.upstream_no);
}

static int adhoc_cfg_valid(const adhoc_cfg_t *cfg)
{
    if (cfg == NULL)
    {
        return 0;
    }
    if (cfg->domain_id > 0x07FFu || cfg->node_id == 0u || cfg->node_id > 0x1FFFFFFFu || cfg->gateway_no > 7u)
    {
        return 0;
    }
    if (cfg->retry_max == 0u)
    {
        return 0;
    }
    return 1;
}

uint32_t adhoc_node_required_size(void)
{
    return (uint32_t)sizeof(adhoc_node_ctx_t);
}

adhoc_rc_t adhoc_node_init(void *node_mem, uint32_t node_mem_size,
                           const adhoc_cfg_t *cfg,
                           const adhoc_link_ops_t *link_ops, void *link_ctx)
{
    adhoc_node_ctx_t *node;
    adhoc_timing_cfg_t timing_cfg;
    adhoc_sm_cfg_t sm_cfg;
    adhoc_data_plane_cfg_t data_cfg;
    int init_ret;

    if (node_mem == NULL || link_ops == NULL)
    {
        return ADHOC_EINVAL;
    }
    if (node_mem_size < (uint32_t)sizeof(adhoc_node_ctx_t) || !adhoc_cfg_valid(cfg))
    {
        return ADHOC_EINVAL;
    }

    node = adhoc_cast(node_mem);
    memset(node, 0, sizeof(*node));
    node->cfg = *cfg;
    node->role = ADHOC_ROLE_BEACON;
    node->link_ops = link_ops;
    node->link_ctx = link_ctx;
    timing_cfg.t1_us = cfg->t1_us;
    timing_cfg.t2_us = cfg->t2_us;
    timing_cfg.t3_us = cfg->t3_us;
    timing_cfg.t4_us = cfg->t4_us;

    if (!adhoc_timing_build(&timing_cfg, &node->timing_plan))
    {
        return ADHOC_EINVAL;
    }
    sm_cfg.domain_id = cfg->domain_id;
    sm_cfg.node_id = cfg->node_id;
    sm_cfg.gateway_no = cfg->gateway_no;
    sm_cfg.t2_us = node->timing_plan.t2_us;
    sm_cfg.t5_us = node->timing_plan.t5_us;
    sm_cfg.retry_max = cfg->retry_max;
    sm_cfg.network_window_us = cfg->network_window_us;
    sm_cfg.regroup_interval_us = cfg->regroup_interval_us;
    sm_cfg.role = ADHOC_SM_ROLE_BEACON;
    if (!adhoc_sm_init(&node->sm, &sm_cfg))
    {
        return ADHOC_EINVAL;
    }
    data_cfg.domain_id = cfg->domain_id;
    data_cfg.node_id = cfg->node_id;
    data_cfg.gateway_no = cfg->gateway_no;
    data_cfg.role_gateway = 0u;
    data_cfg.t5_us = node->timing_plan.t5_us;
    data_cfg.dedup_window_ms = 0u;
    data_cfg.tx_retry_max = cfg->retry_max;
    if (!adhoc_data_plane_init(&node->data_plane, &data_cfg))
    {
        return ADHOC_EINVAL;
    }

    if (node->link_ops->init != NULL)
    {
        init_ret = node->link_ops->init(node->link_ctx);
        if (init_ret != 0)
        {
            return ADHOC_ESTATE;
        }
    }

    node->inited = 1u;
    if (!adhoc_node_sync_route_context(node))
    {
        return ADHOC_ESTATE;
    }
    return ADHOC_OK;
}

adhoc_rc_t adhoc_node_set_role(void *node, adhoc_role_t role)
{
    adhoc_node_ctx_t *ctx = adhoc_cast(node);

    if (ctx == NULL || ctx->inited == 0u)
    {
        return ADHOC_ESTATE;
    }
    if (role != ADHOC_ROLE_BEACON && role != ADHOC_ROLE_GATEWAY)
    {
        return ADHOC_EINVAL;
    }

    ctx->role = role;
    if (!adhoc_sm_set_role(&ctx->sm, role == ADHOC_ROLE_GATEWAY ? ADHOC_SM_ROLE_GATEWAY : ADHOC_SM_ROLE_BEACON))
    {
        return ADHOC_ESTATE;
    }
    if (!adhoc_data_plane_set_role(&ctx->data_plane, role == ADHOC_ROLE_GATEWAY ? 1u : 0u))
    {
        return ADHOC_ESTATE;
    }
    if (!adhoc_node_sync_route_context(ctx))
    {
        return ADHOC_ESTATE;
    }
    return ADHOC_OK;
}

adhoc_rc_t adhoc_node_reset(void *node)
{
    adhoc_node_ctx_t *ctx = adhoc_cast(node);

    if (ctx == NULL || ctx->inited == 0u)
    {
        return ADHOC_ESTATE;
    }

    ctx->has_pending_tx = 0u;
    memset(&ctx->pending_tx, 0, sizeof(ctx->pending_tx));
    ctx->has_last_rx = 0u;
    memset(&ctx->last_rx_fields, 0, sizeof(ctx->last_rx_fields));
    adhoc_sm_reset(&ctx->sm);
    adhoc_data_plane_reset(&ctx->data_plane);
    if (!adhoc_node_sync_route_context(ctx))
    {
        return ADHOC_ESTATE;
    }
    return ADHOC_OK;
}

adhoc_rc_t adhoc_node_on_rx(void *node, const adhoc_frame_t *rx)
{
    adhoc_node_ctx_t *ctx = adhoc_cast(node);
    adhoc_frame_fields_t parsed;
    adhoc_data_rx_result_t data_rc;
    adhoc_sm_rx_event_t event;

    if (ctx == NULL || rx == NULL)
    {
        return ADHOC_EINVAL;
    }
    if (ctx->inited == 0u)
    {
        return ADHOC_ESTATE;
    }
    if (rx->len != ADHOC_FRAME_LEN)
    {
        return ADHOC_EINVAL;
    }
    if (!adhoc_frame_parse(rx->bytes, &parsed))
    {
        return ADHOC_EINVAL;
    }
    if (parsed.sender.domain_id != ctx->cfg.domain_id)
    {
        return ADHOC_EINVAL;
    }
    if (!adhoc_timing_slot_parity_match(parsed.level, parsed.slot_high4))
    {
        return ADHOC_EINVAL;
    }

    ctx->last_rx_fields = parsed;
    ctx->has_last_rx = 1u;

    if (parsed.msg_class == ADHOC_MSG_CLASS_D)
    {
        if (!adhoc_node_sync_route_context(ctx))
        {
            return ADHOC_ESTATE;
        }
        data_rc = adhoc_data_plane_on_rx(&ctx->data_plane, &parsed, rx->ts_us);
        if (data_rc == ADHOC_DATA_RX_IGNORED)
        {
            return ADHOC_EINVAL;
        }
        return ADHOC_OK;
    }

    if (parsed.msg_class == ADHOC_MSG_CLASS_A &&
        (ctx->role == ADHOC_ROLE_BEACON || ctx->role == ADHOC_ROLE_GATEWAY))
    {
        memset(&event, 0, sizeof(event));
        event.msg_class = parsed.msg_class;
        event.gateway_no = parsed.gateway_no;
        event.level = parsed.level;
        event.sender = parsed.sender;
        memcpy(event.content, parsed.content, sizeof(event.content));
        event.rssi = rx->rssi;
        event.ts_us = rx->ts_us;
        (void)adhoc_sm_on_rx(&ctx->sm, &event);
        if (!adhoc_node_sync_route_context(ctx))
        {
            return ADHOC_ESTATE;
        }
    }

    return ADHOC_OK;
}

adhoc_rc_t adhoc_node_poll(void *node, uint32_t now_us)
{
    adhoc_node_ctx_t *ctx = adhoc_cast(node);
    adhoc_frame_fields_t data_tx_fields;
    uint8_t data_has_tx = 0u;
    adhoc_frame_fields_t tx_fields;
    uint8_t has_tx = 0u;
    adhoc_frame_fields_t forward_tx_fields;
    uint8_t forward_has_tx = 0u;

    if (ctx == NULL || ctx->inited == 0u)
    {
        return ADHOC_ESTATE;
    }

    if (ctx->link_ops != NULL && ctx->link_ops->start_rx != NULL)
    {
        if (ctx->link_ops->start_rx(ctx->link_ctx) != ADHOC_LINK_OK)
        {
            return ADHOC_ESTATE;
        }
    }

    if (ctx->role == ADHOC_ROLE_GATEWAY)
    {
        if (!adhoc_node_sync_route_context(ctx))
        {
            return ADHOC_ESTATE;
        }
        if (!adhoc_data_plane_poll_gateway_ack(&ctx->data_plane, now_us, &data_tx_fields, &data_has_tx))
        {
            return ADHOC_ESTATE;
        }
        if (data_has_tx != 0u)
        {
            if (ctx->has_pending_tx != 0u)
            {
                return ADHOC_EBUSY;
            }
            memset(&ctx->pending_tx, 0, sizeof(ctx->pending_tx));
            ctx->pending_tx.len = ADHOC_FRAME_LEN;
            ctx->pending_tx.ts_us = now_us;
            if (!adhoc_frame_build(&data_tx_fields, ctx->pending_tx.bytes))
            {
                return ADHOC_ESTATE;
            }
            ctx->has_pending_tx = 1u;
            return ADHOC_OK;
        }
    }

    if (ctx->role == ADHOC_ROLE_BEACON || ctx->role == ADHOC_ROLE_GATEWAY)
    {
        if (!adhoc_sm_poll(&ctx->sm, now_us, &tx_fields, &has_tx))
        {
            return ADHOC_ESTATE;
        }
        if (!adhoc_node_sync_route_context(ctx))
        {
            return ADHOC_ESTATE;
        }
        if (has_tx != 0u)
        {
            if (ctx->has_pending_tx != 0u)
            {
                return ADHOC_EBUSY;
            }
            memset(&ctx->pending_tx, 0, sizeof(ctx->pending_tx));
            ctx->pending_tx.len = ADHOC_FRAME_LEN;
            ctx->pending_tx.ts_us = now_us;
            if (!adhoc_frame_build(&tx_fields, ctx->pending_tx.bytes))
            {
                return ADHOC_ESTATE;
            }
            ctx->has_pending_tx = 1u;
        }
    }

    if (ctx->has_pending_tx == 0u)
    {
        if (!adhoc_node_sync_route_context(ctx))
        {
            return ADHOC_ESTATE;
        }
        if (!adhoc_data_plane_poll_forward_tx(&ctx->data_plane, now_us, &forward_tx_fields, &forward_has_tx))
        {
            return ADHOC_ESTATE;
        }
        if (forward_has_tx != 0u)
        {
            memset(&ctx->pending_tx, 0, sizeof(ctx->pending_tx));
            ctx->pending_tx.len = ADHOC_FRAME_LEN;
            ctx->pending_tx.ts_us = now_us;
            if (!adhoc_frame_build(&forward_tx_fields, ctx->pending_tx.bytes))
            {
                return ADHOC_ESTATE;
            }
            ctx->has_pending_tx = 1u;
        }
    }

    return ADHOC_OK;
}

adhoc_rc_t adhoc_node_fetch_tx(void *node, adhoc_frame_t *tx)
{
    adhoc_node_ctx_t *ctx = adhoc_cast(node);

    if (ctx == NULL || tx == NULL)
    {
        return ADHOC_EINVAL;
    }
    if (ctx->inited == 0u)
    {
        return ADHOC_ESTATE;
    }
    if (ctx->has_pending_tx == 0u)
    {
        return ADHOC_ENOFRAME;
    }

    *tx = ctx->pending_tx;
    ctx->has_pending_tx = 0u;
    memset(&ctx->pending_tx, 0, sizeof(ctx->pending_tx));
    return ADHOC_OK;
}

adhoc_rc_t adhoc_node_submit_data(void *node, uint8_t source_id_flag, uint32_t lmt_d,
                                  const uint8_t user[ADHOC_DATA_USER_LEN], uint32_t now_us)
{
    adhoc_node_ctx_t *ctx = adhoc_cast(node);
    adhoc_sm_snapshot_t snapshot;
    uint32_t submit_lmt_d;

    if (ctx == NULL || user == NULL)
    {
        return ADHOC_EINVAL;
    }
    if (ctx->inited == 0u)
    {
        return ADHOC_ESTATE;
    }
    if (source_id_flag > ADHOC_PAYLOAD_ID_FLAG_MAX)
    {
        return ADHOC_EINVAL;
    }
    if (ctx->role != ADHOC_ROLE_BEACON)
    {
        return ADHOC_ESTATE;
    }

    memset(&snapshot, 0, sizeof(snapshot));
    adhoc_sm_get_snapshot(&ctx->sm, &snapshot);
    if (snapshot.joined_level == 0u || snapshot.upstream_id == 0u || snapshot.upstream_gateway_no > 7u)
    {
        return ADHOC_ESTATE;
    }

    submit_lmt_d = lmt_d == 0u ? adhoc_lmt_d_from_us(now_us) : (lmt_d & ADHOC_DATA_LMT_D_MAX);
    if (!adhoc_data_plane_submit_source_data(&ctx->data_plane, source_id_flag, submit_lmt_d, user,
                                             snapshot.joined_level, snapshot.upstream_gateway_no, now_us))
    {
        return ADHOC_EBUSY;
    }
    return ADHOC_OK;
}

adhoc_rc_t adhoc_node_fetch_data_tx_report(void *node, adhoc_node_data_tx_report_t *out_report)
{
    adhoc_node_ctx_t *ctx = adhoc_cast(node);
    adhoc_data_tx_report_t report;

    if (ctx == NULL || out_report == NULL)
    {
        return ADHOC_EINVAL;
    }
    if (ctx->inited == 0u)
    {
        return ADHOC_ESTATE;
    }
    if (!adhoc_data_plane_pop_tx_report(&ctx->data_plane, &report))
    {
        return ADHOC_ENOFRAME;
    }

    memset(out_report, 0, sizeof(*out_report));
    if (report.code == ADHOC_DATA_TX_REPORT_ACKED)
    {
        out_report->code = ADHOC_NODE_DATA_TX_REPORT_ACKED;
    }
    else if (report.code == ADHOC_DATA_TX_REPORT_RETRY_EXHAUSTED)
    {
        out_report->code = ADHOC_NODE_DATA_TX_REPORT_RETRY_EXHAUSTED;
    }
    else
    {
        out_report->code = ADHOC_NODE_DATA_TX_REPORT_NONE;
    }
    out_report->source_id_flag = report.item.source.id_flag;
    out_report->source_node_id = report.item.source.node_id;
    out_report->lmt_d = report.item.lmt_d;
    out_report->retry_count = report.retry_count;
    return ADHOC_OK;
}

adhoc_rc_t adhoc_node_get_runtime_status(void *node, adhoc_node_runtime_status_t *out_status)
{
    adhoc_node_ctx_t *ctx = adhoc_cast(node);
    adhoc_sm_snapshot_t snapshot;

    if (ctx == NULL || out_status == NULL)
    {
        return ADHOC_EINVAL;
    }
    if (ctx->inited == 0u)
    {
        return ADHOC_ESTATE;
    }

    memset(&snapshot, 0, sizeof(snapshot));
    memset(out_status, 0, sizeof(*out_status));
    adhoc_sm_get_snapshot(&ctx->sm, &snapshot);

    out_status->state = (uint8_t)snapshot.state;
    out_status->joined_level = snapshot.joined_level;
    out_status->upstream_id = snapshot.upstream_id;
    out_status->upstream_no = snapshot.upstream_no;
    out_status->upstream_gateway_no = snapshot.upstream_gateway_no;
    out_status->retry_count = snapshot.retry_count;
    out_status->upstream_last_seen_us = snapshot.upstream_last_seen_us;
    out_status->gateway_network_started = snapshot.gateway_network_started;
    out_status->gateway_network_locked = snapshot.gateway_network_locked;
    out_status->gateway_network_start_us = snapshot.gateway_network_start_us;
    out_status->gateway_network_end_us = snapshot.gateway_network_end_us;
    out_status->network_lock_active = snapshot.network_lock_active;
    out_status->network_lock_closed = snapshot.network_lock_closed;
    out_status->network_lock_end_us = snapshot.network_lock_end_us;
    return ADHOC_OK;
}
