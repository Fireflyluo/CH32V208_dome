/**
 * @file    serial_upload_task.c
 * @brief   串口上报任务：每秒输出最近一次采集数据
 * @details 本文件实现了基于TMOS的串口数据上报任务，主要功能包括：
 *          - 周期性（1Hz）从传感器任务获取最新数据快照
 *          - 格式化输出加速度计、温度、湿度等传感器数据
 *          - 输出传感器统计信息（采样频率、成功/失败计数等）
 *          - 通过USB CDC虚拟串口发送数据（替代传统UART）
 *          
 *          数据格式：
 *          - 加速度数据：ax=xxx ay=xxx az=xxx mg
 *          - 温度数据：t=±xx.xx°C（支持负温度）
 *          - 湿度数据：rh=xx.xx%
 *          - 统计信息：accel_hz=100 sht_hz=1 sht_ok=xxx sht_err=xxx ready=1
 *          
 *          任务事件：
 *          - SERIAL_EVT_INIT: 初始化事件，启动周期性上报
 *          - SERIAL_EVT_UPLOAD: 数据上报事件，执行实际的数据输出
 *
 * @author  WCH (南京沁恒微电子股份有限公司)
 * @version V1.0.0
 * @date    2022/06/16
 */
#include "serial_upload_task.h"

#include "i2c_bus_arbiter.h"
#include "log_print.h"
#include "ad_hoc_task.h"
#include "sensor_task.h"
#include "wchble.h"

#define SERIAL_EVT_INIT   (0x0001u << 0)
#define SERIAL_EVT_UPLOAD (0x0001u << 1)

/* 上报周期 1s */
#define SERIAL_UPLOAD_MS 200u
#define SERIAL_STAT_MS 1000u
#define SERIAL_STAT_DIV (SERIAL_STAT_MS / SERIAL_UPLOAD_MS)

static tmosTaskID s_serial_task_id = INVALID_TASK_ID;
static uint16_t s_stat_div_cnt = 0u;

static tmosEvents serial_upload_task_process_event(tmosTaskID task_id, tmosEvents events);

/**
 * @brief  串口上传任务初始化函数
 * @details 注册TMOS串口上传任务并触发初始化事件。
 *          如果任务已存在或注册失败，则直接返回。
 */
void serial_upload_task_init(void)
{
    if (s_serial_task_id != INVALID_TASK_ID)
    {
        return;
    }

    s_serial_task_id = TMOS_ProcessEventRegister(serial_upload_task_process_event);
    if (s_serial_task_id == INVALID_TASK_ID)
    {
        LOG_PRINT("serial upload task register failed\r\n");
        return;
    }

    tmos_set_event(s_serial_task_id, SERIAL_EVT_INIT);
}

/**
 * @brief  串口上传任务事件处理函数
 * @details 处理串口上传任务的各类事件：
 *          - SERIAL_EVT_INIT: 启动周期性数据上报定时器
 *          - SERIAL_EVT_UPLOAD: 获取传感器数据并格式化输出
 * 
 * @param[in] task_id 当前任务ID
 * @param[in] events 待处理的事件位图
 * @return 未处理的事件
 */
static tmosEvents serial_upload_task_process_event(tmosTaskID task_id, tmosEvents events)
{
    sensor_snapshot_t snap;
    ad_hoc_task_status_t adhoc_st;
    i2c_bus_stats_t i2c_st;
    int32_t t_abs;

    (void)task_id;

    if (events & SERIAL_EVT_INIT)
    {
        tmos_start_reload_task(s_serial_task_id, SERIAL_EVT_UPLOAD, MS1_TO_SYSTEM_TIME(SERIAL_UPLOAD_MS));
        s_stat_div_cnt = 0u;
        return (events ^ SERIAL_EVT_INIT);
    }

    if (events & SERIAL_EVT_UPLOAD)
    {
        /* 读取最近一次传感器结果并格式化输出 */
        sensor_task_get_snapshot(&snap);
        t_abs = (snap.temp_centi_c >= 0) ? snap.temp_centi_c : -snap.temp_centi_c;

        LOG_PRINT("uplink ax=%ld ay=%ld az=%ld mg, t=%s%ld.%02ldC rh=%ld.%02ld%%\r\n",
                  (long)snap.accel_mg_x,
                  (long)snap.accel_mg_y,
                  (long)snap.accel_mg_z,
                  (snap.temp_centi_c < 0) ? "-" : "",
                  (long)(t_abs / 100),
                  (long)(t_abs % 100),
                  (long)(snap.rh_centi_pct / 100),
                  (long)(snap.rh_centi_pct % 100));
        s_stat_div_cnt++;
        if (s_stat_div_cnt >= SERIAL_STAT_DIV)
        {
            s_stat_div_cnt = 0u;
            LOG_PRINT("uplink stat accel_hz=%u sht_hz=%u sht_ok=%lu sht_err=%lu ready=%u\r\n",
                      (unsigned int)snap.accel_hz,
                      (unsigned int)snap.sht_hz,
                      (unsigned long)snap.sht_ok_cnt,
                      (unsigned long)snap.sht_err_cnt,
                      (unsigned int)snap.ready);
            ad_hoc_task_get_status(&adhoc_st);
            LOG_PRINT("adhoc stat role=%s init=%u node=%lu tx=%lu rx=%lu node_err=%lu tx_err=%lu rx_err=%lu\r\n",
                      adhoc_st.role_gateway != 0u ? "GW" : "BCN",
                      (unsigned int)adhoc_st.inited,
                      (unsigned long)adhoc_st.node_id,
                      (unsigned long)adhoc_st.tx_cnt,
                      (unsigned long)adhoc_st.rx_cnt,
                      (unsigned long)adhoc_st.node_err_cnt,
                      (unsigned long)adhoc_st.tx_err_cnt,
                      (unsigned long)adhoc_st.rx_err_cnt);
            LOG_PRINT("adhoc sm st=%u lvl=%u retry=%u up_gw=%u up_no=%u up_id=%lu up_age=%lums gw_start=%u gw_lock=%u gw_left=%lums net_act=%u net_closed=%u net_left=%lums\r\n",
                      (unsigned int)adhoc_st.sm_state,
                      (unsigned int)adhoc_st.sm_joined_level,
                      (unsigned int)adhoc_st.sm_retry_count,
                      (unsigned int)adhoc_st.sm_upstream_gateway_no,
                      (unsigned int)adhoc_st.sm_upstream_no,
                      (unsigned long)adhoc_st.sm_upstream_id,
                      (unsigned long)adhoc_st.sm_upstream_last_seen_age_ms,
                      (unsigned int)adhoc_st.sm_gateway_network_started,
                      (unsigned int)adhoc_st.sm_gateway_network_locked,
                      (unsigned long)adhoc_st.sm_gateway_window_left_ms,
                      (unsigned int)adhoc_st.sm_network_lock_active,
                      (unsigned int)adhoc_st.sm_network_lock_closed,
                      (unsigned long)adhoc_st.sm_network_lock_left_ms);
            LOG_PRINT("adhoc data sub=%lu/%lu busy=%lu skip=%lu fail=%lu ack=%lu rex=%lu\r\n",
                      (unsigned long)adhoc_st.data_submit_ok,
                      (unsigned long)adhoc_st.data_submit_try,
                      (unsigned long)adhoc_st.data_submit_busy,
                      (unsigned long)adhoc_st.data_submit_state_skip,
                      (unsigned long)adhoc_st.data_submit_fail,
                      (unsigned long)adhoc_st.tx_report_acked,
                      (unsigned long)adhoc_st.tx_report_retry_exhausted);
            LOG_PRINT("adhoc link rxstart=%lu skip=%lu tx=%lu/%lu busy=%lu fail=%lu rx=%lu/%lu empty=%lu err=%lu rssi=%d tc=%u\r\n",
                      (unsigned long)adhoc_st.link_start_rx_cnt,
                      (unsigned long)adhoc_st.link_start_rx_skip_txbusy_cnt,
                      (unsigned long)adhoc_st.link_tx_ok_cnt,
                      (unsigned long)adhoc_st.link_tx_req_cnt,
                      (unsigned long)adhoc_st.link_tx_busy_cnt,
                      (unsigned long)adhoc_st.link_tx_fail_cnt,
                      (unsigned long)adhoc_st.link_rx_ok_cnt,
                      (unsigned long)adhoc_st.link_rx_poll_cnt,
                      (unsigned long)adhoc_st.link_rx_empty_cnt,
                      (unsigned long)adhoc_st.link_rx_err_cnt,
                      (int)adhoc_st.link_last_rssi,
                      (unsigned int)adhoc_st.link_last_rx_tc);

            i2c_bus_get_stats(I2C_NUM_1, &i2c_st);
            LOG_PRINT("uplink i2c q=%u/%u run=%u ok=%lu err=%lu to=%lu rec=%lu aw=%lums ae=%lums mw=%lums me=%lums\r\n",
                      (unsigned int)i2c_st.queue_depth_curr,
                      (unsigned int)i2c_st.queue_depth_peak,
                      (unsigned int)i2c_st.running,
                      (unsigned long)i2c_st.done_ok,
                      (unsigned long)i2c_st.done_err,
                      (unsigned long)i2c_st.timeout_cnt,
                      (unsigned long)i2c_st.recover_cnt,
                      (unsigned long)i2c_st.avg_wait_ms,
                      (unsigned long)i2c_st.avg_exec_ms,
                      (unsigned long)i2c_st.max_wait_ms,
                      (unsigned long)i2c_st.max_exec_ms);
        }

        return (events ^ SERIAL_EVT_UPLOAD);
    }

    return 0;
}
