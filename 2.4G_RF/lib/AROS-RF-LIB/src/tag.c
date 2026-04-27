#include "tag.h"

#include <string.h>

#define TG_SlotTC (10000 / ARF_TC_us)

#define TG_N 21
#define LV_N 5
#define TG_LV_N 5

#define tg_lv(tg) (((tg) + TG_LV_N - 1) / TG_LV_N)
#define tg_lvno(tg) (((tg)-1) % TG_LV_N)
#define lv_tg(lv) ((lv)*TG_LV_N - (TG_LV_N - 1))

/* Msg fmt: cmd2b_slot9b_lv5b(2B) msg(30B)
 * lv==0:  msg=id1(4B) no1(2B) id2 no2 ...
 * lv!=0:  msg=source_id(4B) no(2B) dat(24B)
 */
#define MSG_TG 2
#define MSG_NO (MSG_TG + 4)
#define MSG_DAT (MSG_NO + 2)
#define msg_cmd(b) (((*(b) >> 6) & 3))
#define msg_slot(b) ((((*(b) & 63) << 3) + ((*(b + 1) >> 5) & 7)))
#define msg_lv(b) (*(b + 1) & 31)
#define msg_csl(b, cmd, slt, lv) \
    (*(b) = ((cmd & 3) << 6) + ((slt >> 3) & 63), *(b + 1) = ((slt & 7) << 5) + (lv & 31))

static int16_t lv_slot[] = {
    0, 2, 0,
    1, 10, 0, 0, 2, 1,
    2, 10, 0, 2, 20, 2, 1, 25, 2, 0, 2, 2,
    3, 10, 0, 3, 20, 3, 2, 25, 3, 1, 25, 3, 0, 2, 3,
    4, 10, 0, 4, 20, 4, 3, 25, 4, 2, 25, 4, 1, 25, 4, 0, 2, 4};
#define Slot_DefN (sizeof(lv_slot) / sizeof(lv_slot[0]))

typedef struct
{
    uint8_t synced;
    uint8_t stage_ready;
    uint8_t tx_active;
    uint8_t cycle_data_loaded;
    uint16_t stage_idx;
    uint16_t stage_slot_base;
    int16_t tx_lv;
    int16_t tx_slot;
    int16_t tx_slots_total;
    int16_t tx_slots_sent;
    int16_t tx_tg;
    int16_t tx_n;
    int16_t tx_mtg;
} tg_sm_t;

static uint8_t recv_dat[TG_N][ARF_MsgN];
static uint16_t tg_slot_tc0 = 0;
static tg_runtime_status_t s_status = {0};
static tg_sm_t s_sm = {0};
static arf_newdat_fc s_newdatfunc = NULL;

static uint8_t tg_sanitize_id(uint8_t tg_id)
{
    if (tg_id > TG_MAX_ID)
    {
        return TG_MAX_ID;
    }
    return tg_id;
}

static int get_slot(void)
{
    int tc0 = (int)arf_get_tc16();
    int tc = (int)arf_u16(tc0 - (int)tg_slot_tc0);
    return tc / TG_SlotTC;
}

static void set_slot_tc0(uint8_t *rxbuf)
{
#ifndef ARF_WIN
    int slt = (int)msg_slot(rxbuf);
    int tc = (int)arf_u16toi(rxbuf + ARF_MsgTC);
    tg_slot_tc0 = arf_u16(tc - slt * TG_SlotTC);
#else
    (void)rxbuf;
#endif
}

static void tg_ack(uint8_t *id)
{
    (void)id;
}

static void tg_ack1(uint8_t *rxbuf)
{
    set_slot_tc0(rxbuf);
    tg_ack(rxbuf + 2);
}

static void tg_ack0(uint8_t *rxbuf)
{
    int i;
    set_slot_tc0(rxbuf);
    for (i = 2; i < ARF_MsgN; i += 6)
    {
        tg_ack(rxbuf + i);
    }
}

static int tg_recv_once(void)
{
    uint8_t *rxbuf;
    int lv;
    int mlv;

    rxbuf = arf_isRxFinish();
    if (rxbuf == NULL)
    {
        return -1;
    }

    s_status.rx_cnt++;
    s_status.last_rssi = (int8_t)arf_RSSI(rxbuf);
    s_status.last_rx_tc = arf_u16toi(rxbuf + ARF_MsgTC);

    lv = (int)msg_lv(rxbuf);
    mlv = tg_lv((int)s_status.tg_id);

    if (lv == 0)
    {
        if (mlv == 1)
        {
            tg_ack0(rxbuf);
        }
        else
        {
            return -3;
        }
    }
    else if (lv < mlv)
    {
        if (mlv - lv == 1)
        {
            tg_ack1(rxbuf);
        }
        else
        {
            return -3;
        }
    }
    else if (lv - mlv > 1)
    {
        return -3;
    }
    else
    {
        uint32_t ti = arf_u32toi(rxbuf + MSG_TG);
        if (ti >= TG_N)
        {
            return -4;
        }
        memcpy(recv_dat[ti], rxbuf, ARF_MsgN);
    }

    return lv;
}

static void tg_reset_cycle(uint8_t synced)
{
    s_sm.synced = synced;
    s_sm.stage_ready = 0;
    s_sm.tx_active = 0;
    s_sm.cycle_data_loaded = 0;
    s_sm.stage_idx = 0;
    s_sm.stage_slot_base = 0;
}

static void tg_prepare_cycle_data(void)
{
    uint8_t *datbuf;

    if (s_sm.cycle_data_loaded != 0)
    {
        return;
    }

    s_sm.cycle_data_loaded = 1;
    if (s_newdatfunc == NULL)
    {
        return;
    }

    datbuf = recv_dat[s_status.tg_id];
    if ((*s_newdatfunc)(datbuf + MSG_DAT) != 0)
    {
        arf_u32toa(datbuf + MSG_TG, s_status.tg_id);
        arf_u16toa(datbuf + MSG_NO, s_status.dat_no);
        s_status.dat_no++;
    }
}

static void tg_prepare_stage_tx(void)
{
    int lv;
    int nslt;
    int datlv;
    int slt;
    int tg0;

    if (s_sm.stage_idx >= Slot_DefN)
    {
        s_sm.stage_ready = 0;
        s_sm.tx_active = 0;
        return;
    }

    lv = lv_slot[s_sm.stage_idx];
    nslt = lv_slot[s_sm.stage_idx + 1];
    datlv = lv_slot[s_sm.stage_idx + 2];

    s_sm.stage_ready = 1;
    s_sm.tx_active = 0;

    if (lv != tg_lv((int)s_status.tg_id))
    {
        return;
    }

    tg0 = (int)s_status.tg_id;
    slt = (int)s_sm.stage_slot_base;
    if (lv != 0)
    {
        nslt /= TG_LV_N;
        slt += tg_lvno(tg0) * nslt;
    }

    s_sm.tx_lv = (int16_t)lv;
    s_sm.tx_slot = (int16_t)slt;
    s_sm.tx_slots_total = (int16_t)nslt;
    s_sm.tx_slots_sent = 0;

    if (datlv == 0)
    {
        s_sm.tx_tg = (int16_t)tg0;
        s_sm.tx_n = 1;
        s_sm.tx_mtg = -1;
    }
    else
    {
        s_sm.tx_tg = (int16_t)lv_tg(datlv);
        s_sm.tx_n = TG_LV_N;
        s_sm.tx_mtg = (int16_t)tg0;
    }

    if (s_sm.tx_slots_total > 0)
    {
        s_sm.tx_active = 1;
    }
}

static int tg_send_one(void)
{
    uint8_t *dat;
    int ret;

    if (s_sm.tx_active == 0)
    {
        return 0;
    }
    if (s_sm.tx_slots_sent >= s_sm.tx_slots_total)
    {
        s_sm.tx_active = 0;
        arf_RxStart();
        return 1;
    }

    if (s_sm.tx_tg == s_sm.tx_mtg)
    {
        s_sm.tx_tg++;
    }
    if (s_sm.tx_tg < 0 || s_sm.tx_tg >= TG_N)
    {
        s_sm.tx_active = 0;
        return -1;
    }

    dat = recv_dat[s_sm.tx_tg];
    msg_csl(dat, 1, s_sm.tx_slot, s_sm.tx_lv);

    ret = arf_TxTrySend(dat, 0);
    if (ret == 0)
    {
        return 0;
    }
    if (ret < 0)
    {
        s_sm.tx_active = 0;
        return -1;
    }

    s_status.tx_cnt++;
    s_sm.tx_slots_sent++;
    s_sm.tx_slot++;

    s_sm.tx_n--;
    if (s_sm.tx_n > 0)
    {
        s_sm.tx_tg++;
    }

    if (s_sm.tx_slots_sent >= s_sm.tx_slots_total)
    {
        s_sm.tx_active = 0;
        arf_RxStart();
    }

    return 1;
}

void tg_init(uint8_t tg_id)
{
    memset(recv_dat, 0, sizeof(recv_dat));
    memset(&s_status, 0, sizeof(s_status));
    memset(&s_sm, 0, sizeof(s_sm));
    s_newdatfunc = NULL;
    tg_set_id(tg_id);
}

void tg_set_id(uint8_t tg_id)
{
    s_status.tg_id = tg_sanitize_id(tg_id);
    s_status.dat_no = 0;
    s_status.tx_cnt = 0;
    s_status.rx_cnt = 0;
    s_status.last_rssi = 0;
    s_status.last_rx_tc = 0;

    tg_slot_tc0 = arf_get_tc16();
    tg_reset_cycle((s_status.tg_id == 0u) ? 1u : 0u);

    if (s_status.tg_id != 0u)
    {
        arf_RxStart();
    }
}

uint8_t tg_get_id(void)
{
    return s_status.tg_id;
}

void tg_set_newdatfunc(arf_newdat_fc newdatf)
{
    s_newdatfunc = newdatf;
    arf_set_newdatfunc(newdatf);
}

void tg_step(void)
{
    int i;
    int loop_guard;
    if (s_sm.synced == 0)
    {
        if (s_status.tg_id == 0u)
        {
            tg_slot_tc0 = arf_get_tc16();
            tg_reset_cycle(1);
        }
        else
        {
            int rv = tg_recv_once();
            if (rv < 0)
            {
                return;
            }
            tg_reset_cycle(1);
        }
    }
    else
    {
        for (i = 0; i < 4; i++)
        {
            if (tg_recv_once() == -1)
            {
                break;
            }
        }
    }

    tg_prepare_cycle_data();

    for (loop_guard = 0; loop_guard < 32; loop_guard++)
    {
        int lv;
        int nslt;
        int stage_start;
        int stage_end;
        int cur;

        if (s_sm.stage_idx >= Slot_DefN)
        {
            if (s_status.tg_id == 0u)
            {
                tg_slot_tc0 = arf_get_tc16();
                tg_reset_cycle(1);
                tg_prepare_cycle_data();
                continue;
            }

            tg_reset_cycle(0);
            arf_RxStart();
            return;
        }

        lv = lv_slot[s_sm.stage_idx];
        nslt = lv_slot[s_sm.stage_idx + 1];
        stage_start = (int)s_sm.stage_slot_base;
        stage_end = stage_start + nslt;
        cur = get_slot();

        if (cur < stage_start)
        {
            return;
        }

        if (s_sm.stage_ready == 0)
        {
            tg_prepare_stage_tx();
        }

        if (s_sm.tx_active != 0)
        {
            if (cur < s_sm.tx_slot)
            {
                return;
            }

            if (tg_send_one() > 0)
            {
                continue;
            }
            return;
        }

        if (cur >= stage_end)
        {
            s_sm.stage_idx = (uint16_t)(s_sm.stage_idx + 3u);
            s_sm.stage_slot_base = (uint16_t)stage_end;
            s_sm.stage_ready = 0;
            continue;
        }

        if (lv != tg_lv((int)s_status.tg_id))
        {
            return;
        }

        return;
    }
}

void tg_get_status(tg_runtime_status_t *out)
{
    if (out == NULL)
    {
        return;
    }

    *out = s_status;
    out->synced = s_sm.synced;
}

void tg_loop(void)
{
    for (;;)
    {
        tg_step();
    }
}

#ifdef TG_Main
void tg_tmr0(void) { printf("Timer0: %d\n", (int)(arf_get_tc() % 60000)); }
void tg_tmr1(void) { printf("Timer1: %d\n", (int)(arf_get_tc() % 60000)); }
void tg_tmr2(void) { printf("Timer2: %d\n", (int)(arf_get_tc() % 60000)); }

int main(void)
{
    arf_SysInit();
    arf_Init();
    tg_init(0);
    arf_set_timer(0, 1000000, tg_tmr0);
    arf_set_timer(1, 1500000, tg_tmr1);
    arf_set_timer(2, 2400000, tg_tmr2);
    tg_loop();
    return 0;
}
#endif
