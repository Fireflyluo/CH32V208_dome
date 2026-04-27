/**
 * @file    sensor_task.c
 * @brief   传感器采集任务：SC7A20(100Hz) + SHT40(1Hz，两段式)
 * @details 本文件实现了基于TMOS的多传感器数据采集任务，主要功能包括：
 *          - SC7A20加速度计初始化和100Hz数据采集
 *          - SHT40温湿度传感器初始化和1Hz数据采集（两段式：发命令+读结果）
 *          - 传感器数据快照管理（线程安全的seqlock机制）
 *          - 传感器状态监控和错误统计
 *          - 自动重试机制处理传感器初始化失败
 *          
 *          采样策略：
 *          - 加速度计：100Hz连续采样（10ms间隔）
 *          - 温湿度传感器：1Hz采样（1秒间隔），采用两段式避免阻塞
 *          - 统计信息：每秒更新采样频率和错误计数
 *          
 *          任务事件：
 *          - SENSOR_EVT_INIT: 初始化事件，尝试初始化传感器
 *          - SENSOR_EVT_ACCEL: 加速度计采样事件
 *          - SENSOR_EVT_SHT_CMD: SHT40发送测量命令事件
 *          - SENSOR_EVT_SHT_READ: SHT40读取测量结果事件
 *          - SENSOR_EVT_STATS: 统计信息更新事件
 *
 * @author  WCH (南京沁恒微电子股份有限公司)
 * @version V1.0.0
 * @date    2022/06/16
 */

#include "sensor_task.h"

#include "board.h"
#include "i2c_bus_arbiter.h"
#include "log_print.h"
#include "sc7a20.h"
#include "sc7a20_ch32_adapter.h"
#include "sht40.h"
#include "sht40_ch32_adapter.h"
#include "wchble.h"

#define SENSOR_EVT_INIT       (0x0001u << 0)
#define SENSOR_EVT_ACCEL      (0x0001u << 1)
#define SENSOR_EVT_SHT_CMD    (0x0001u << 2)
#define SENSOR_EVT_STATS      (0x0001u << 3)
#define SENSOR_EVT_SHT_READ   (0x0001u << 4)

/* 采样与统计周期 */
#define ACCEL_SAMPLE_MS       100u
#define SHT_SAMPLE_MS         1000u
#define SHT_MEASURE_DELAY_MS  3u
#define STATS_PERIOD_MS       1000u
#define INIT_RETRY_MS         2000u

static tmosTaskID s_sensor_task_id = INVALID_TASK_ID;
static volatile uint32_t s_snapshot_seq = 0u;
static sensor_snapshot_t s_snapshot = {0};

static sc7a20_dev_t s_sc7a20_dev = {0};
static sc7a20_ch32_bus_ctx_t s_sc7a20_bus = {
    .i2c_num = I2C_NUM_1,
    .dev_addr = SC7A20_I2C_ADDR_H,
};

static sht40_dev_t s_sht40_dev = {0};
static sht40_ch32_bus_ctx_t s_sht40_bus = {
    .i2c_num = I2C_NUM_1,
    .dev_addr = 0x46u,
};

static uint8_t s_accel_ready = 0u;
static uint8_t s_sht_ready = 0u;
static uint8_t s_timers_started = 0u;
static uint16_t s_accel_samples_window = 0u;
static uint16_t s_sht_samples_window = 0u;

static tmosEvents sensor_task_process_event(tmosTaskID task_id, tmosEvents events);
static void sensor_publish_ready(void);
static int sensor_try_init_accel(void);
static int sensor_try_init_sht(void);
static void sensor_sample_accel(void);
static int sensor_sht40_send_cmd(void);
static void sensor_sht40_read_result(void);
static void sensor_sht40_delay_adapter(void *ctx, uint32_t ms);

/* 无锁快照写入标记：奇数=写入中，偶数=稳定态 */
static void sensor_snapshot_write_begin(void)
{
    s_snapshot_seq++;
}

static void sensor_snapshot_write_end(void)
{
    s_snapshot_seq++;
}

/**
 * @brief  获取传感器数据快照
 * @details 使用seqlock机制确保读取到一致的数据快照，
 *          避免在写入过程中读取到撕裂的数据。
 * 
 * @param[out] out 指向输出缓冲区的指针，用于存储快照数据
 */
void sensor_task_get_snapshot(sensor_snapshot_t *out)
{
    uint32_t seq_start;
    uint32_t seq_end;

    if (out == NULL)
    {
        return;
    }

    do /* seqlock 读法，避免读取到撕裂数据 */
    {
        seq_start = s_snapshot_seq;
        if (seq_start & 1u)
        {
            continue;
        }
        *out = s_snapshot;
        seq_end = s_snapshot_seq;
    } while (seq_start != seq_end || (seq_end & 1u));
}

/**
 * @brief  传感器任务初始化函数
 * @details 注册TMOS传感器任务并触发初始化事件。
 *          如果任务已存在或注册失败，则直接返回。
 */
void sensor_task_init(void)
{
    if (s_sensor_task_id != INVALID_TASK_ID)
    {
        return;
    }

    s_sensor_task_id = TMOS_ProcessEventRegister(sensor_task_process_event);
    if (s_sensor_task_id == INVALID_TASK_ID)
    {
        LOG_PRINT("sensor task register failed\r\n");
        return;
    }

    tmos_set_event(s_sensor_task_id, SENSOR_EVT_INIT);
}

/**
 * @brief  SHT40延迟适配器函数
 * @details 为SHT40驱动提供延迟功能，使用HAL层的延时函数。
 * 
 * @param[in] ctx 用户上下文（未使用）
 * @param[in] ms 延迟时间（毫秒）
 */
static void sensor_sht40_delay_adapter(void *ctx, uint32_t ms)
{
    (void)ctx;
    HAL_Delay(ms);
}

/**
 * @brief  发布传感器就绪状态
 * @details 更新快照中的ready标志，表示至少有一个传感器已就绪。
 */
static void sensor_publish_ready(void)
{
    sensor_snapshot_write_begin();
    s_snapshot.ready = (uint8_t)((s_accel_ready != 0u || s_sht_ready != 0u) ? 1u : 0u);
    sensor_snapshot_write_end();
}

/**
 * @brief  尝试初始化SC7A20加速度计
 * @details 配置SC7A20为2g量程、100Hz采样率、高分辨率模式。
 *          如果初始化成功，设置s_accel_ready标志并打印日志。
 * 
 * @return 0表示成功，负数表示失败
 */
static int sensor_try_init_accel(void)
{
    int rc;
    sc7a20_cfg_t cfg = g_sc7a20_default_cfg;

    if (s_accel_ready != 0u)
    {
        return 0;
    }

    cfg.range = SC7A20_ACCEL_FS_2G;   /* 量程 2g */
    cfg.odr = SC7A20_ACCEL_ODR_100HZ;
    cfg.high_resolution = true;

    s_sc7a20_dev.ops = &g_sc7a20_ch32_i2c_ops;
    s_sc7a20_dev.bus_ctx = &s_sc7a20_bus;
    s_sc7a20_dev.addr = s_sc7a20_bus.dev_addr;

    rc = sc7a20_init_with_config(&s_sc7a20_dev, &cfg);
    if (rc == 0)
    {
        s_accel_ready = 1u;
        LOG_PRINT("SC7A20 init ok\r\n");
    }
    else
    {
        LOG_PRINT("SC7A20 init failed: %d\r\n", rc);
    }
    return rc;
}

/**
 * @brief  尝试初始化SHT40温湿度传感器
 * @details 尝试两个可能的I2C地址（0x46和0x44）进行SHT40初始化。
 *          如果任一地址初始化成功，设置s_sht_ready标志并打印日志。
 * 
 * @return 0表示成功，-1表示失败
 */
static int sensor_try_init_sht(void)
{
    int rc;
    uint8_t i;
    /* 兼容不同硬件焊接地址 */
    static const uint8_t k_addr_try[2] = {0x46u, 0x44u};

    if (s_sht_ready != 0u)
    {
        return 0;
    }

    s_sht40_dev.ops = &g_sht40_ch32_i2c_ops;
    s_sht40_dev.bus_ctx = &s_sht40_bus;
    s_sht40_dev.delay_ms = sensor_sht40_delay_adapter;
    s_sht40_dev.delay_ctx = NULL;

    for (i = 0u; i < 2u; i++)
    {
        s_sht40_bus.dev_addr = k_addr_try[i];
        s_sht40_dev.addr = s_sht40_bus.dev_addr;
        s_sht40_dev.initialized = false;
        rc = sht40_init(&s_sht40_dev);
        if (rc == 0)
        {
            s_sht_ready = 1u;
            LOG_PRINT("SHT40 init ok at 0x%02X\r\n", s_sht40_bus.dev_addr);
            return 0;
        }
        LOG_PRINT("SHT40 init failed at 0x%02X: %d\r\n", s_sht40_bus.dev_addr, rc);
    }

    return -1;
}

/**
 * @brief  采样加速度计数据
 * @details 从SC7A20读取原始三轴数据，并转换为mg单位存储到快照中。
 */
static void sensor_sample_accel(void)
{
    sc7a20_vec3i16_t raw;
    int rc;

    if (s_accel_ready == 0u)
    {
        return;
    }

    rc = sc7a20_read_xyz_raw(&s_sc7a20_dev, &raw);
    if (rc != 0)
    {
        return;
    }

    sensor_snapshot_write_begin();
    s_snapshot.accel_raw_x = raw.x;
    s_snapshot.accel_raw_y = raw.y;
    s_snapshot.accel_raw_z = raw.z;
    s_snapshot.accel_mg_x = (int32_t)((float)raw.x * s_sc7a20_dev.sensitivity_g_per_lsb * 1000.0f);
    s_snapshot.accel_mg_y = (int32_t)((float)raw.y * s_sc7a20_dev.sensitivity_g_per_lsb * 1000.0f);
    s_snapshot.accel_mg_z = (int32_t)((float)raw.z * s_sc7a20_dev.sensitivity_g_per_lsb * 1000.0f);
    sensor_snapshot_write_end();

    s_accel_samples_window++;
}

/**
 * @brief  发送SHT40测量命令
 * @details 向SHT40发送低精度触发测量命令（0xE0）。
 *          如果发送失败，增加错误计数。
 * 
 * @return 0表示成功，-1表示失败
 */
static int sensor_sht40_send_cmd(void)
{
    uint8_t cmd = 0xE0u; /* 低精度触发测量命令 */
    i2c_bus_request_t req;
    int rc;

    if (s_sht_ready == 0u)
    {
        return -1;
    }

    req.bus = I2C_NUM_1;
    req.type = I2C_BUS_REQ_WRITE;
    req.owner_id = 2u;
    req.dev_addr = s_sht40_bus.dev_addr;
    req.reg = 0u;
    req.wbuf = &cmd;
    req.rbuf = NULL;
    req.len = 1u;
    req.mode_hint = I2C_MODE_IT;
    req.prio = I2C_BUS_PRIO_NORMAL;
    req.timeout_ms = 50u;

    rc = i2c_bus_submit_sync(&req);
    if (rc != 0)
    {
        sensor_snapshot_write_begin();
        s_snapshot.sht_err_cnt++;
        sensor_snapshot_write_end();
    }

    return rc;
}

/**
 * @brief  读取SHT40测量结果
 * @details 从SHT40读取6字节的测量数据，解析温度和湿度值，
 *          并转换为工程单位（0.01°C和0.01%RH）存储到快照中。
 */
static void sensor_sht40_read_result(void)
{
    uint8_t rx[6] = {0};
    uint16_t t_raw;
    uint16_t h_raw;
    float t_c;
    float rh;
    i2c_bus_request_t req;
    int rc;

    if (s_sht_ready == 0u)
    {
        return;
    }

    req.bus = I2C_NUM_1;
    req.type = I2C_BUS_REQ_READ;
    req.owner_id = 2u;
    req.dev_addr = s_sht40_bus.dev_addr;
    req.reg = 0u;
    req.wbuf = NULL;
    req.rbuf = rx;
    req.len = 6u;
    req.mode_hint = I2C_MODE_IT;
    req.prio = I2C_BUS_PRIO_NORMAL;
    req.timeout_ms = 50u;

    rc = i2c_bus_submit_sync(&req);
    if (rc != 0)
    {
        sensor_snapshot_write_begin();
        s_snapshot.sht_err_cnt++;
        sensor_snapshot_write_end();
        return;
    }

    /* 按 SHT40 数据格式解析，并换算成工程值 */
    t_raw = (uint16_t)(((uint16_t)rx[0] << 8) | rx[1]);
    h_raw = (uint16_t)(((uint16_t)rx[3] << 8) | rx[4]);
    t_c = -45.0f + 175.0f * ((float)t_raw / 65535.0f);
    rh = -6.0f + 125.0f * ((float)h_raw / 65535.0f);
    if (rh < 0.0f)
    {
        rh = 0.0f;
    }
    if (rh > 100.0f)
    {
        rh = 100.0f;
    }

    sensor_snapshot_write_begin();
    s_snapshot.temp_centi_c = (int32_t)(t_c * 100.0f);
    s_snapshot.rh_centi_pct = (int32_t)(rh * 100.0f);
    s_snapshot.sht_ok_cnt++;
    sensor_snapshot_write_end();

    s_sht_samples_window++;
}

/**
 * @brief  传感器任务事件处理函数
 * @details 处理传感器任务的各类事件：
 *          - SENSOR_EVT_INIT: 初始化传感器并启动定时器
 *          - SENSOR_EVT_ACCEL: 采样加速度计数据
 *          - SENSOR_EVT_SHT_CMD: 发送SHT40测量命令
 *          - SENSOR_EVT_SHT_READ: 读取SHT40测量结果
 *          - SENSOR_EVT_STATS: 更新统计信息
 * 
 * @param[in] task_id 当前任务ID
 * @param[in] events 待处理的事件位图
 * @return 未处理的事件
 */
static tmosEvents sensor_task_process_event(tmosTaskID task_id, tmosEvents events)
{
    tmosEvents remain = events;

    (void)task_id;

    if (remain & SENSOR_EVT_INIT)
    {
        (void)sensor_try_init_accel();
        (void)sensor_try_init_sht();
        sensor_publish_ready();

        if (s_timers_started == 0u)
        {
            s_timers_started = 1u;
            tmos_start_reload_task(s_sensor_task_id, SENSOR_EVT_ACCEL, MS1_TO_SYSTEM_TIME(ACCEL_SAMPLE_MS));
            tmos_start_reload_task(s_sensor_task_id, SENSOR_EVT_SHT_CMD, MS1_TO_SYSTEM_TIME(SHT_SAMPLE_MS));
            tmos_start_reload_task(s_sensor_task_id, SENSOR_EVT_STATS, MS1_TO_SYSTEM_TIME(STATS_PERIOD_MS));
            LOG_PRINT("sensor task started: accel=100Hz, sht40=1Hz\r\n");
        }

        if (s_accel_ready == 0u || s_sht_ready == 0u)
        {
            tmos_start_task(s_sensor_task_id, SENSOR_EVT_INIT, MS1_TO_SYSTEM_TIME(INIT_RETRY_MS));
        }

        remain ^= SENSOR_EVT_INIT;
    }

    if (remain & SENSOR_EVT_ACCEL)
    {
        sensor_sample_accel();
        remain ^= SENSOR_EVT_ACCEL;
    }

    if (remain & SENSOR_EVT_SHT_CMD)
    {
        /* 两段式：先发测量命令，延时后再读结果，避免任务内阻塞等待 */
        if (sensor_sht40_send_cmd() == 0)
        {
            tmos_start_task(s_sensor_task_id, SENSOR_EVT_SHT_READ, MS1_TO_SYSTEM_TIME(SHT_MEASURE_DELAY_MS));
        }
        remain ^= SENSOR_EVT_SHT_CMD;
    }

    if (remain & SENSOR_EVT_SHT_READ)
    {
        sensor_sht40_read_result();
        remain ^= SENSOR_EVT_SHT_READ;
    }

    if (remain & SENSOR_EVT_STATS)
    {
        sensor_snapshot_write_begin();
        s_snapshot.accel_hz = s_accel_samples_window;
        s_snapshot.sht_hz = s_sht_samples_window;
        sensor_snapshot_write_end();

        s_accel_samples_window = 0u;
        s_sht_samples_window = 0u;
        sensor_publish_ready();
        remain ^= SENSOR_EVT_STATS;
    }

    return remain;
}