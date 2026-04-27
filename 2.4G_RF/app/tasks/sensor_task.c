/**
 * @file    sensor_task.c
 * @brief   传感器采集任务：SC7A20(ODR 400Hz，任务读出约400Hz) + SHT40(1Hz，两段式) + 碰撞位移算法
 * @details 本文档实现了基于TMOS的多传感器数据采集任务，主要功能包括：
 *          - SC7A20加速度计初始化和高频数据采集
 *          - SHT40温湿度传感器初始化和1Hz数据采集（两段式：发命令+读结果）
 *          - 传感器数据快照管理（线程安全的seqlock机制）
 *          - 传感器状态监测和错误统计
 *          - 自动重试机制处理传感器初始化失败
 *          - 碰撞事件检测与位移计算（impact_displacement 算法）
 *
 *          采样策略：
 *          - 加速度计：TMOS任务3ms读出（约400Hz），SC7A20内部ODR=400Hz
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
 * @author fireflyluo
 * @version V1.0.0
 * @date    2022/06/16
 */
#include "sensor_task.h"

#include "board.h"
#include "drv_tim.h"
#include "i2c_bus_arbiter.h"
#include "impact_displacement.h"
#include "log_print.h"
#include "sc7a20.h"
#include "sc7a20_ch32_adapter.h"
#include "sht40.h"
#include "sht40_ch32_adapter.h"
#include "wchble.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define SENSOR_EVT_INIT (0x0001u << 0)
#define SENSOR_EVT_ACCEL (0x0001u << 1)
#define SENSOR_EVT_SHT_CMD (0x0001u << 2)
#define SENSOR_EVT_STATS (0x0001u << 3)
#define SENSOR_EVT_SHT_READ (0x0001u << 4)

/* 采样与统计周期 */
#define ACCEL_SAMPLE_MS 3u
#define ACCEL_POLL_MS 3u
#define SHT_SAMPLE_MS 1000u
#define SHT_MEASURE_DELAY_MS 5u
#define STATS_PERIOD_MS 1000u
#define INIT_RETRY_MS 2000u

#define ACCEL_FIFO_WATERMARK 4u
#define ACCEL_FIFO_MAX_DRAIN 8u
#define ACCEL_FIFO_BURST_BYTES (ACCEL_FIFO_MAX_DRAIN * 6u)
#define SENSOR_ACCEL_USE_FIFO 1u

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
static uint8_t s_accel_fifo_enabled = 0u;
static uint16_t s_accel_fifo_empty_streak = 0u;
static uint16_t s_accel_fifo_fail_streak = 0u;
static uint16_t s_accel_samples_window = 0u;
static uint16_t s_sht_samples_window = 0u;

/* 碰撞位移算法相关 */
enum
{
    IMPACT_RING_SIZE = 512,  /* 环形缓冲区大小 (~1.5s @ 333Hz) */
    IMPACT_PRE_SAMPLES = 80, /* 基准窗口的样本数 */
    IMPACT_GRAVITY_EMA_TAU_MS = 500,
    IMPACT_ALGO_MAX_DT_MS = 35,
    IMPACT_ALGO_MAX_EVENT_MS = 700,
    SENSOR_ACCEL_FULL_SCALE_MG = 16000u,

    SENSOR_PARAM_T1_MAX_MS = 10000u,
    SENSOR_PARAM_T2_MIN_MS = 20u,
    SENSOR_PARAM_T2_MAX_MS = 2000u,
    SENSOR_PARAM_T3_MAX_MS = 10000u,
    SENSOR_PARAM_T4_MAX_MS = 20000u,

    SENSOR_DEFAULT_T1_MS = 6u,
    SENSOR_DEFAULT_T2_MS = 100u,
    SENSOR_DEFAULT_T3_MS = 15u,
    SENSOR_DEFAULT_T4_MS = 800u,
    SENSOR_DEFAULT_THRESHOLD_H12 = 320u,
    SENSOR_DEFAULT_THRESHOLD_L12 = 48u,
    SENSOR_TRIGGER_ARM_GUARD_MS = 300u,
};

typedef enum
{
    IMPACT_TRIG_STATE_IDLE = 0,      ///< 空闲状态
    IMPACT_TRIG_STATE_ARMED,         ///< 预备状态
    IMPACT_TRIG_STATE_ACTIVE,        ///< 激活状态
    IMPACT_TRIG_STATE_RELEASED,      ///< 释放状态
    IMPACT_TRIG_STATE_COOLDOWN,      ///< 冷却状态
} impact_trigger_state_t;

typedef struct
{
    uint16_t trigger_high_mg;        ///< 触发高阈值(mg)
    uint16_t release_low_mg;         ///< 释放低阈值(mg)
    uint16_t trigger_hold_samples;   ///< 触发保持样本数
    uint16_t release_hold_samples;   ///< 释放保持样本数
    uint32_t trigger_hold_ms;        ///< 触发保持时间(ms)
    uint32_t sample_period_ms;       ///< 采样周期(ms)
    uint32_t release_hold_ms;        ///< 释放保持时间(ms)
    uint32_t cooldown_ms;            ///< 冷却时间(ms)
} impact_trigger_cfg_t;

typedef struct
{
    impact_disp_sample_t samples[IMPACT_RING_SIZE];  ///< 采样数据环形缓冲区
    uint16_t head;                                   ///< 缓冲区头部索引
    uint16_t count;                                  ///< 缓冲区中元素数量
    uint32_t event_id;                               ///< 事件ID
    uint32_t cooldown_until_us;                      ///< 冷却结束时间(us)
    uint16_t trigger_idx;                            ///< 触发索引
    uint16_t peak_dynamic_mg;                        ///< 峰值动态加速度(mg)
    uint8_t trigger_valid;                           ///< 触发是否有效
    impact_trigger_state_t state;                    ///< 当前触发状态
    uint32_t state_enter_us;                         ///< 状态进入时间(us)
    uint32_t arm_guard_until_us;                     ///< 预备保护截止时间(us)
} impact_ring_ctx_t;

static impact_ring_ctx_t s_impact_ring = {0};
static impact_disp_ctx_t s_impact_ctx = {0};
static uint8_t s_impact_algo_ready = 0u;
static impact_disp_cfg_t s_impact_cfg = {0};
static sensor_peak_params_t s_peak_params = {
    .t1_ms = SENSOR_DEFAULT_T1_MS,
    .t2_ms = SENSOR_DEFAULT_T2_MS,
    .t3_ms = SENSOR_DEFAULT_T3_MS,
    .t4_ms = SENSOR_DEFAULT_T4_MS,
    .threshold_high = SENSOR_DEFAULT_THRESHOLD_H12,
    .threshold_low = SENSOR_DEFAULT_THRESHOLD_L12,
};
static impact_trigger_cfg_t s_trigger_cfg = {0};

/**
 * @brief  初始化碰撞检测环形缓冲区
 * @param  reset_event_id  重置事件ID
 */
static void impact_ring_init(uint8_t reset_event_id);

/**
 * @brief  向碰撞检测环形缓冲区推送数据
 * @param  ax_mg  X轴加速度(mg)
 * @param  ay_mg  Y轴加速度(mg)
 * @param  az_mg  Z轴加速度(mg)
 * @param  ts_us  时间戳(微秒)
 */
static void impact_ring_push(int16_t ax_mg, int16_t ay_mg, int16_t az_mg, uint32_t ts_us);

/**
 * @brief  碰撞检测事件
 * @param  ax_mg  X轴加速度(mg)
 * @param  ay_mg  Y轴加速度(mg)
 * @param  az_mg  Z轴加速度(mg)
 * @param  ts_us  时间戳(微秒)
 */
static void impact_detect_event(int16_t ax_mg, int16_t ay_mg, int16_t az_mg, uint32_t ts_us);

/**
 * @brief  计算并报告碰撞结果
 */
static void impact_compute_and_report(void);

/**
 * @brief  结束碰撞事件
 * @param  ts_us   时间戳(微秒)
 * @param  reason  结束原因
 */
static void impact_finish_event(uint32_t ts_us, const char *reason);

/**
 * @brief  计算环形缓冲区中两点间的距离
 * @param  start_idx  起始索引
 * @param  end_idx    结束索引
 * @return 两点间距离
 */
static uint16_t impact_ring_distance(uint16_t start_idx, uint16_t end_idx);

/**
 * @brief  在环形缓冲区中前进指定步数
 * @param  idx   当前索引
 * @param  step  步数
 * @return 新索引
 */
static uint16_t impact_ring_advance(uint16_t idx, uint16_t step);

/**
 * @brief  将毫米值转换为十分之一毫米
 * @param  mm  毫米值
 * @return 十分之一毫米值
 */
static int32_t impact_mm_to_tenths(float mm);

/**
 * @brief  获取整数的绝对值并转换为uint32
 * @param  v  输入值
 * @return 绝对值
 */
static uint32_t impact_abs_u32_from_i32(int32_t v);

/**
 * @brief  将12位阈值转换为mg
 * @param  value_12  12位阈值
 * @return mg值
 */
static uint16_t impact_u12_to_mg(uint16_t value_12);

/**
 * @brief  将毫秒转换为样本数
 * @param  ms  毫秒数
 * @return 样本数
 */
static uint16_t impact_ms_to_samples(uint32_t ms);

/**
 * @brief  将释放样本数转换为算法最小值
 * @param  release_hold_samples  释放保持样本数
 * @return 算法最小值
 */
static uint16_t impact_release_samples_to_algo_min(uint16_t release_hold_samples);

/**
 * @brief  计算经过的毫秒数
 * @param  now_us     当前时间(us)
 * @param  start_us   开始时间(us)
 * @return 经过的毫秒数
 */
static uint32_t impact_elapsed_ms(uint32_t now_us, uint32_t start_us);

/**
 * @brief  校验传感器峰值参数
 * @param  params  参数指针
 */
static void sensor_peak_params_sanitize(sensor_peak_params_t *params);

/**
 * @brief  应用传感器峰值参数
 * @param  params          参数指针
 * @param  from_protocol   是否来自协议
 */
static void sensor_peak_params_apply(const sensor_peak_params_t *params, uint8_t from_protocol);

/**
 * @brief  传感器任务事件处理函数
 * @param  task_id  当前任务ID
 * @param  events   待处理的事件位图
 * @return 未处理的事件
 */
static tmosEvents sensor_task_process_event(tmosTaskID task_id, tmosEvents events);

/**
 * @brief  发布传感器就绪状态
 */
static void sensor_publish_ready(void);

/**
 * @brief  尝试初始化SC7A20加速度计
 * @return 0表示成功，负数表示失败
 */
static int sensor_try_init_accel(void);

/**
 * @brief  尝试初始化SHT40温湿度传感器
 * @return 0表示成功，-1表示失败
 */
static int sensor_try_init_sht(void);

#if SENSOR_ACCEL_USE_FIFO
/**
 * @brief  尝试配置加速度计FIFO
 * @return 0表示成功，非0表示失败
 */
static int sensor_try_configure_accel_fifo(void);
#endif

/**
 * @brief  采样加速度计数据
 */
static void sensor_sample_accel(void);

/**
 * @brief  处理加速度计采样数据
 * @param  raw    原始数据
 * @param  ts_us  时间戳(微秒)
 */
static void sensor_process_accel_sample(const sc7a20_vec3i16_t *raw, uint32_t ts_us);

/**
 * @brief  发送SHT40命令
 * @return 0表示成功，-1表示失败
 */
static int sensor_sht40_send_cmd(void);

/**
 * @brief  读取SHT40测量结果
 */
static void sensor_sht40_read_result(void);

/**
 * @brief  SHT40延迟适配器函数
 * @param  ctx  用户上下文
 * @param  ms   延迟时间(毫秒)
 */
static void sensor_sht40_delay_adapter(void *ctx, uint32_t ms);

/* 无锁快照写入标记：奇数=写入中，偶数=稳定性 */
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
 * @details 使用seqlock机制确保读取一致的数据快照，
 *          避免在写入过程中读取到混乱的数据。
 *
 * @param[out] out 指向输出缓冲区的指针，用于存储快照数据
 */
void sensor_task_get_snapshot(sensor_snapshot_t *out)
{
    uint32_t seq_start;
    uint32_t seq_end = 0u;

    if (out == NULL)
    {
        return;
    }

    do /* seqlock 读法，避免读取到脏数据 */
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
 * @brief  获取传感器峰值参数
 * @param  out  输出参数结构体指针
 */
void sensor_task_get_peak_params(sensor_peak_params_t *out)
{
    if (out == NULL)
    {
        return;
    }
    *out = s_peak_params;
}

/**
 * @brief  设置传感器峰值参数
 * @param  params  参数结构体指针
 */
void sensor_task_set_peak_params(const sensor_peak_params_t *params)
{
    if (params == NULL)
    {
        return;
    }
    sensor_peak_params_apply(params, 1u);
}

/**
 * @brief  检查碰撞检测是否忙碌
 * @return 忙碌状态
 */
uint8_t sensor_task_impact_busy(void)
{
    return (uint8_t)((s_impact_ring.state == IMPACT_TRIG_STATE_ARMED ||
                      s_impact_ring.state == IMPACT_TRIG_STATE_ACTIVE ||
                      s_impact_ring.state == IMPACT_TRIG_STATE_RELEASED)
                         ? 1u
                         : 0u);
}

static uint16_t impact_u12_to_mg(uint16_t value_12)
{
    uint32_t v = (uint32_t)(value_12 & 0x0FFFu);
    return (uint16_t)((v * SENSOR_ACCEL_FULL_SCALE_MG + 2047u) / 4095u);
}

static uint16_t impact_ms_to_samples(uint32_t ms)
{
    uint32_t samples;

    if (ms == 0u)
    {
        return 0u;
    }
    samples = (ms + (ACCEL_SAMPLE_MS - 1u)) / ACCEL_SAMPLE_MS;
    if (samples > 0xFFFFu)
    {
        samples = 0xFFFFu;
    }
    return (uint16_t)samples;
}

static uint16_t impact_release_samples_to_algo_min(uint16_t release_hold_samples)
{
    uint32_t algo_min;

    if (release_hold_samples <= 1u)
    {
        return 1u;
    }

    /* Relax algorithm tail requirement to half of T3 window.
     * Trigger FSM still enforces full T3; this avoids over-penalizing NO_RELEASE
     * when dynamic-magnitude definitions differ slightly between layers. */
    algo_min = ((uint32_t)release_hold_samples + 1u) / 2u;
    if (algo_min == 0u)
    {
        algo_min = 1u;
    }
    if (algo_min > 0xFFFFu)
    {
        algo_min = 0xFFFFu;
    }
    return (uint16_t)algo_min;
}

static uint32_t impact_elapsed_ms(uint32_t now_us, uint32_t start_us)
{
    return (uint32_t)(now_us - start_us) / 1000u;
}

static void sensor_peak_params_sanitize(sensor_peak_params_t *params)
{
    uint16_t t;

    if (params == NULL)
    {
        return;
    }

    params->t1_ms &= 0xFFFFFFu;
    params->t2_ms &= 0xFFFFFFu;
    params->t3_ms &= 0xFFFFFFu;
    params->t4_ms &= 0xFFFFFFu;
    params->threshold_high &= 0x0FFFu;
    params->threshold_low &= 0x0FFFu;

    if (params->t1_ms > SENSOR_PARAM_T1_MAX_MS)
    {
        params->t1_ms = SENSOR_PARAM_T1_MAX_MS;
    }
    if (params->t2_ms < SENSOR_PARAM_T2_MIN_MS)
    {
        params->t2_ms = SENSOR_PARAM_T2_MIN_MS;
    }
    if (params->t2_ms > SENSOR_PARAM_T2_MAX_MS)
    {
        params->t2_ms = SENSOR_PARAM_T2_MAX_MS;
    }
    if (params->t3_ms > SENSOR_PARAM_T3_MAX_MS)
    {
        params->t3_ms = SENSOR_PARAM_T3_MAX_MS;
    }
    if (params->t4_ms > SENSOR_PARAM_T4_MAX_MS)
    {
        params->t4_ms = SENSOR_PARAM_T4_MAX_MS;
    }

    if (params->threshold_low > params->threshold_high)
    {
        t = params->threshold_low;
        params->threshold_low = params->threshold_high;
        params->threshold_high = t;
    }
}

static void sensor_peak_params_apply(const sensor_peak_params_t *params, uint8_t from_protocol)
{
    sensor_peak_params_t p;

    if (params == NULL)
    {
        return;
    }

    p = *params;
    sensor_peak_params_sanitize(&p);
    s_peak_params = p;

    s_trigger_cfg.trigger_high_mg = impact_u12_to_mg(p.threshold_high);
    s_trigger_cfg.release_low_mg = impact_u12_to_mg(p.threshold_low);
    s_trigger_cfg.trigger_hold_samples = impact_ms_to_samples(p.t1_ms);
    s_trigger_cfg.release_hold_samples = impact_ms_to_samples(p.t3_ms);
    s_trigger_cfg.trigger_hold_ms = p.t1_ms;
    s_trigger_cfg.sample_period_ms = p.t2_ms;
    s_trigger_cfg.release_hold_ms = p.t3_ms;
    s_trigger_cfg.cooldown_ms = p.t4_ms;

    s_impact_cfg.release_threshold_mg = s_trigger_cfg.release_low_mg;
    s_impact_cfg.release_count_min =
        impact_release_samples_to_algo_min(s_trigger_cfg.release_hold_samples);

    if (s_impact_algo_ready != 0u)
    {
        (void)impact_disp_init(&s_impact_ctx, &s_impact_cfg);
        impact_ring_init(0u);
    }

    LOG_PRINT("IMPACT params%s: T1=%lums T2=%lums T3=%lums T4=%lums H=%u(%umg) L=%u(%umg)\r\n",
              (from_protocol != 0u) ? "[P]" : "[BOOT]",
              (unsigned long)s_peak_params.t1_ms,
              (unsigned long)s_peak_params.t2_ms,
              (unsigned long)s_peak_params.t3_ms,
              (unsigned long)s_peak_params.t4_ms,
              (unsigned)s_peak_params.threshold_high,
              (unsigned)s_trigger_cfg.trigger_high_mg,
              (unsigned)s_peak_params.threshold_low,
              (unsigned)s_trigger_cfg.release_low_mg);
    LOG_PRINT("IMPACT map: t1=%u samp t3=%u samp algo_rel_min=%u sample_ms=%lums\r\n",
              (unsigned)s_trigger_cfg.trigger_hold_samples,
              (unsigned)s_trigger_cfg.release_hold_samples,
              (unsigned)s_impact_cfg.release_count_min,
              (unsigned long)s_trigger_cfg.sample_period_ms);
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

    sensor_peak_params_apply(&s_peak_params, 0u);

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
 * @details 为SHT40驱动提供延迟功能，使用HAL层的延迟函数。
 *
 * @param[in] ctx 用户上下文（未使用）
 * @param[in] ms  延迟时间（毫秒）
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
 * @details 配置SC7A20为16g量程、400Hz采样率、高分辨率模式。
 *          如果初始化成功，设置s_accel_ready标志并打印日志。
 *
 * @return 0表示成功，负数表示失败
 */
static int sensor_try_init_accel(void)
{
    int rc;
    impact_disp_status_t impact_st;
    sc7a20_cfg_t cfg = g_sc7a20_default_cfg;

    if (s_accel_ready != 0u)
    {
        return 0;
    }

    cfg.range = SC7A20_ACCEL_FS_16G; /* 量程 16g */
    cfg.odr = SC7A20_ACCEL_ODR_400HZ;
    cfg.high_resolution = true;

    s_sc7a20_dev.ops = &g_sc7a20_ch32_i2c_ops;
    s_sc7a20_dev.bus_ctx = &s_sc7a20_bus;
    s_sc7a20_dev.addr = s_sc7a20_bus.dev_addr;

    rc = sc7a20_init_with_config(&s_sc7a20_dev, &cfg);
    if (rc == 0)
    {
        s_accel_fifo_enabled = 0u;
        s_accel_fifo_empty_streak = 0u;
        s_accel_fifo_fail_streak = 0u;
#if SENSOR_ACCEL_USE_FIFO
        {
            int fifo_rc = sensor_try_configure_accel_fifo();
            if (fifo_rc != 0)
            {
                s_accel_fifo_enabled = 0u;
                s_accel_fifo_empty_streak = 0u;
                s_accel_fifo_fail_streak = 0u;
                LOG_PRINT("SC7A20 FIFO disabled, fallback to direct read rc=%d\r\n", fifo_rc);
            }
        }
#endif

        impact_disp_get_default_cfg(&s_impact_cfg);
        s_impact_cfg.sample_rate_hz = (uint16_t)((1000u + (ACCEL_SAMPLE_MS / 2u)) / ACCEL_SAMPLE_MS);
        s_impact_cfg.release_threshold_mg = s_trigger_cfg.release_low_mg;
        s_impact_cfg.release_count_min =
            impact_release_samples_to_algo_min(s_trigger_cfg.release_hold_samples);
        s_impact_cfg.max_dt_ms = IMPACT_ALGO_MAX_DT_MS;
        s_impact_cfg.max_event_ms = IMPACT_ALGO_MAX_EVENT_MS;
        s_impact_cfg.gravity_ema_tau_ms = IMPACT_GRAVITY_EMA_TAU_MS;

        impact_st = impact_disp_init(&s_impact_ctx, &s_impact_cfg);
        if (impact_st != IMPACT_DISP_OK)
        {
            s_accel_ready = 0u;
            s_impact_algo_ready = 0u;
            LOG_PRINT("impact_disp_init failed: %d\r\n", impact_st);
            return -1;
        }

        s_accel_ready = 1u;
        s_impact_algo_ready = 1u;
        impact_ring_init(1u);
        LOG_PRINT("SC7A20 init ok (16g, 400Hz, fifo=%u), accel_poll=%ums impact cfg rate=%uHz H=%umg L=%umg T1=%ums T3=%ums T4=%ums rel_cnt=%u grav_ema=%ums max_dt=%ums max_evt=%ums\r\n",
                  (unsigned)s_accel_fifo_enabled,
                  (unsigned)ACCEL_POLL_MS,
                  (unsigned)s_impact_cfg.sample_rate_hz,
                  (unsigned)s_trigger_cfg.trigger_high_mg,
                  (unsigned)s_impact_cfg.release_threshold_mg,
                  (unsigned)s_trigger_cfg.trigger_hold_ms,
                  (unsigned)s_trigger_cfg.release_hold_ms,
                  (unsigned)s_trigger_cfg.cooldown_ms,
                  (unsigned)s_impact_cfg.release_count_min,
                  (unsigned)s_impact_cfg.gravity_ema_tau_ms,
                  (unsigned)s_impact_cfg.max_dt_ms,
                  (unsigned)s_impact_cfg.max_event_ms);
    }
    else
    {
        s_accel_fifo_enabled = 0u;
        s_accel_fifo_empty_streak = 0u;
        s_accel_fifo_fail_streak = 0u;
        LOG_PRINT("SC7A20 init failed: %d\r\n", rc);
    }
    return rc;
}

#if SENSOR_ACCEL_USE_FIFO
static int sensor_try_configure_accel_fifo(void)
{
    int rc;
    sc7a20_fifo_dma_policy_t fifo_dma_policy;
    sc7a20_ctrl3_t ctrl3;
    sc7a20_ctrl5_t ctrl5;
    sc7a20_fifo_ctrl_t fifo_ctrl;
    sc7a20_fifo_src_t fifo_src;

    ctrl3.reg = 0u;
    rc = sc7a20_read_reg(&s_sc7a20_dev, SC7A20_CTRL3, &ctrl3.reg, 1u);
    if (rc != 0)
    {
        return rc;
    }
    ctrl3.bit.fifo_mode = 0u; /* 0: 12-bit FIFO data mode */
    rc = sc7a20_write_reg(&s_sc7a20_dev, SC7A20_CTRL3, &ctrl3.reg, 1u);
    if (rc != 0)
    {
        return rc;
    }

    ctrl5.reg = 0u;
    rc = sc7a20_read_reg(&s_sc7a20_dev, SC7A20_CTRL5, &ctrl5.reg, 1u);
    if (rc != 0)
    {
        return rc;
    }

    ctrl5.bit.FIFO_EN = 1u;
    rc = sc7a20_write_reg(&s_sc7a20_dev, SC7A20_CTRL5, &ctrl5.reg, 1u);
    if (rc != 0)
    {
        return rc;
    }

    fifo_ctrl.reg = 0u;
    fifo_ctrl.bit.FM = (uint8_t)SC7A20_FIFO_STREAM_MODE;
    fifo_ctrl.bit.FTH = (uint8_t)(ACCEL_FIFO_WATERMARK & 0x1Fu);
    rc = sc7a20_write_reg(&s_sc7a20_dev, SC7A20_FIFO_CTRL, &fifo_ctrl.reg, 1u);
    if (rc != 0)
    {
        return rc;
    }

    fifo_src.reg = 0u;
    rc = sc7a20_read_reg(&s_sc7a20_dev, SC7A20_FIFO_SRC, &fifo_src.reg, 1u);
    if (rc != 0)
    {
        return rc;
    }

    s_accel_fifo_enabled = 1u;
    s_accel_fifo_empty_streak = 0u;
    s_accel_fifo_fail_streak = 0u;
    fifo_dma_policy = sc7a20_ch32_get_fifo_dma_policy();
    LOG_PRINT("SC7A20 FIFO cfg ok: FM=%u FTH=%u FIFO_MODE=%u DMA_POLICY=%u\r\n",
              (unsigned)fifo_ctrl.bit.FM,
              (unsigned)fifo_ctrl.bit.FTH,
              (unsigned)ctrl3.bit.fifo_mode,
              (unsigned)fifo_dma_policy);
    return 0;
}
#endif

/**
 * @brief  尝试初始化SHT40温湿度传感器
 * @details 尝试两个可能的I2C地址：0x46和0x44，进行SHT40初始化。
 *          如果任一地址初始化成功，设置s_sht_ready标志并打印日志。
 *
 * @return 0表示成功，-1表示失败
 */
static int sensor_try_init_sht(void)
{
    int rc;
    uint8_t i;
    /* 兼容不同焊接到板子上的地址 */
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
 * @brief  处理加速度计采样数据
 * @details 从SC7A20读取原始三轴数据，并转换为mg单位存储到快照中。
 * 
 * @param  raw    原始数据指针
 * @param  ts_us  时间戳(微秒)
 */
static void sensor_process_accel_sample(const sc7a20_vec3i16_t *raw, uint32_t ts_us)
{
    int32_t ax_mg;
    int32_t ay_mg;
    int32_t az_mg;

    if (raw == NULL)
    {
        return;
    }

    ax_mg = (int32_t)((float)raw->x * s_sc7a20_dev.sensitivity_g_per_lsb * 1000.0f);
    ay_mg = (int32_t)((float)raw->y * s_sc7a20_dev.sensitivity_g_per_lsb * 1000.0f);
    az_mg = (int32_t)((float)raw->z * s_sc7a20_dev.sensitivity_g_per_lsb * 1000.0f);

    sensor_snapshot_write_begin();
    s_snapshot.accel_raw_x = raw->x;
    s_snapshot.accel_raw_y = raw->y;
    s_snapshot.accel_raw_z = raw->z;
    s_snapshot.accel_mg_x = ax_mg;
    s_snapshot.accel_mg_y = ay_mg;
    s_snapshot.accel_mg_z = az_mg;
    sensor_snapshot_write_end();

    s_accel_samples_window++;

    if (s_impact_algo_ready)
    {
        impact_ring_push((int16_t)ax_mg, (int16_t)ay_mg, (int16_t)az_mg, ts_us);
        impact_detect_event((int16_t)ax_mg, (int16_t)ay_mg, (int16_t)az_mg, ts_us);
    }
}

/**
 * @brief  采样加速度计数据
 */
static void sensor_sample_accel(void)
{
    sc7a20_vec3i16_t raw;
    uint8_t fifo_raw[ACCEL_FIFO_BURST_BYTES];
    sc7a20_fifo_src_t fifo_src;
    uint8_t samples_to_drain = 1u;
    uint8_t i;
    int rc;
    uint32_t ts_now_us;
    uint32_t ts_first_us;
    uint32_t sample_step_us = ACCEL_SAMPLE_MS * 1000u;

    if (s_accel_ready == 0u)
    {
        return;
    }

    if (s_accel_fifo_enabled != 0u)
    {
        fifo_src.reg = 0u;
        rc = sc7a20_read_reg(&s_sc7a20_dev, SC7A20_FIFO_SRC, &fifo_src.reg, 1u);
        if (rc == 0)
        {
            s_accel_fifo_fail_streak = 0u;
            if (fifo_src.bit.EMPTY != 0u)
            {
                if (s_accel_fifo_empty_streak < 0xFFFFu)
                {
                    s_accel_fifo_empty_streak++;
                }
                if (s_accel_fifo_empty_streak >= 64u)
                {
                    s_accel_fifo_enabled = 0u;
                    LOG_PRINT("SC7A20 FIFO auto-disabled: empty streak=%u\r\n",
                              (unsigned)s_accel_fifo_empty_streak);
                }
                samples_to_drain = 1u;
            }
            else
            {
                s_accel_fifo_empty_streak = 0u;
                samples_to_drain = fifo_src.bit.FSS;
                if (samples_to_drain == 0u)
                {
                    samples_to_drain = 1u;
                }
                if (samples_to_drain > ACCEL_FIFO_MAX_DRAIN)
                {
                    samples_to_drain = ACCEL_FIFO_MAX_DRAIN;
                }
            }
        }
        else
        {
            if (s_accel_fifo_fail_streak < 0xFFFFu)
            {
                s_accel_fifo_fail_streak++;
            }
            if (s_accel_fifo_fail_streak >= 16u)
            {
                s_accel_fifo_enabled = 0u;
                LOG_PRINT("SC7A20 FIFO auto-disabled: src read fail streak=%u\r\n",
                          (unsigned)s_accel_fifo_fail_streak);
            }
            samples_to_drain = 1u;
        }
    }

    ts_now_us = (uint32_t)drv_tim_get_time_us();
    ts_first_us = ts_now_us - (uint32_t)((samples_to_drain - 1u) * sample_step_us);

    if (s_accel_fifo_enabled != 0u && samples_to_drain > 0u)
    {
        uint16_t fifo_len = (uint16_t)(samples_to_drain * 6u);
        rc = sc7a20_read_reg(&s_sc7a20_dev, SC7A20_FIFO_DATA, fifo_raw, fifo_len);
        if (rc == 0)
        {
            s_accel_fifo_fail_streak = 0u;
            for (i = 0u; i < samples_to_drain; ++i)
            {
                rc = sc7a20_core_decode_xyz(&s_sc7a20_dev, &fifo_raw[(uint16_t)i * 6u], &raw);
                if (rc != 0)
                {
                    break;
                }
                sensor_process_accel_sample(&raw, ts_first_us + (uint32_t)i * sample_step_us);
            }
            if (rc == 0)
            {
                return;
            }
        }
        else
        {
            if (s_accel_fifo_fail_streak < 0xFFFFu)
            {
                s_accel_fifo_fail_streak++;
            }
            if (s_accel_fifo_fail_streak >= 16u)
            {
                s_accel_fifo_enabled = 0u;
                LOG_PRINT("SC7A20 FIFO auto-disabled: data read fail streak=%u\r\n",
                          (unsigned)s_accel_fifo_fail_streak);
            }
            samples_to_drain = 1u;
        }
    }

    for (i = 0u; i < samples_to_drain; ++i)
    {
        rc = sc7a20_read_xyz_raw(&s_sc7a20_dev, &raw);
        if (rc != 0)
        {
            if (i == 0u)
            {
                return;
            }
            break;
        }
        sensor_process_accel_sample(&raw, ts_first_us + (uint32_t)i * sample_step_us);
    }
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

    /* 按SHT40 数据格式解析，并换算成工程值 */
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
 * @param[in] task_id  当前任务ID
 * @param[in] events   待处理的事件位图
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
            tmos_start_reload_task(s_sensor_task_id, SENSOR_EVT_ACCEL, MS1_TO_SYSTEM_TIME(ACCEL_POLL_MS));
            tmos_start_reload_task(s_sensor_task_id, SENSOR_EVT_SHT_CMD, MS1_TO_SYSTEM_TIME(SHT_SAMPLE_MS));
            tmos_start_reload_task(s_sensor_task_id, SENSOR_EVT_STATS, MS1_TO_SYSTEM_TIME(STATS_PERIOD_MS));
            LOG_PRINT("sensor task started: accel_poll=%ums sample_base=%ums, sht40=1Hz\r\n",
                      (unsigned)ACCEL_POLL_MS,
                      (unsigned)ACCEL_SAMPLE_MS);
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

/* ============================================================================
 * 碰撞位移算法集成
 * ============================================================================ */

static void impact_ring_init(uint8_t reset_event_id)
{
    uint32_t next_event_id = s_impact_ring.event_id;
    uint32_t now_us = (uint32_t)drv_tim_get_time_us();

    if (next_event_id == 0u || reset_event_id != 0u)
    {
        next_event_id = 1u;
    }

    memset(&s_impact_ring, 0, sizeof(s_impact_ring));
    s_impact_ring.event_id = next_event_id;
    s_impact_ring.trigger_idx = 0u;
    s_impact_ring.trigger_valid = 0u;
    s_impact_ring.state = IMPACT_TRIG_STATE_IDLE;
    s_impact_ring.state_enter_us = now_us;
    s_impact_ring.arm_guard_until_us = now_us + (SENSOR_TRIGGER_ARM_GUARD_MS * 1000u);
}

static void impact_ring_push(int16_t ax_mg, int16_t ay_mg, int16_t az_mg, uint32_t ts_us)
{
    uint16_t next_head = (s_impact_ring.head + 1u) % IMPACT_RING_SIZE;

    s_impact_ring.samples[s_impact_ring.head].timestamp_us = ts_us;
    s_impact_ring.samples[s_impact_ring.head].ax_mg = ax_mg;
    s_impact_ring.samples[s_impact_ring.head].ay_mg = ay_mg;
    s_impact_ring.samples[s_impact_ring.head].az_mg = az_mg;
    s_impact_ring.samples[s_impact_ring.head].flags = 0u;
    s_impact_ring.samples[s_impact_ring.head].reserved[0] = 0u;
    s_impact_ring.samples[s_impact_ring.head].reserved[1] = 0u;
    s_impact_ring.samples[s_impact_ring.head].reserved[2] = 0u;

    s_impact_ring.head = next_head;
    if (s_impact_ring.count < IMPACT_RING_SIZE)
    {
        s_impact_ring.count++;
    }
}

static uint16_t impact_ring_advance(uint16_t idx, uint16_t step)
{
    return (uint16_t)((idx + step) % IMPACT_RING_SIZE);
}

static int32_t impact_mm_to_tenths(float mm)
{
    if (mm >= 0.0f)
    {
        return (int32_t)(mm * 10.0f + 0.5f);
    }
    return (int32_t)(mm * 10.0f - 0.5f);
}

static uint32_t impact_abs_u32_from_i32(int32_t v)
{
    return (uint32_t)((v < 0) ? -v : v);
}

static void impact_finish_event(uint32_t ts_us, const char *reason)
{
    uint32_t cooldown_until_us;

    impact_compute_and_report();
    s_impact_ring.trigger_valid = 0u;
    s_impact_ring.peak_dynamic_mg = 0u;

    cooldown_until_us = ts_us + (uint32_t)(s_trigger_cfg.cooldown_ms * 1000u);
    s_impact_ring.cooldown_until_us = cooldown_until_us;
    s_impact_ring.state = IMPACT_TRIG_STATE_COOLDOWN;
    s_impact_ring.state_enter_us = ts_us;

    LOG_PRINT("IMPACT state: RELEASED -> COOLDOWN reason=%s cooldown=%lums event_id=%lu\r\n",
              (reason != NULL) ? reason : "none",
              (unsigned long)s_trigger_cfg.cooldown_ms,
              (unsigned long)s_impact_ring.event_id);

    s_impact_ring.event_id++;
}

/* Number of samples in [start_idx, end_idx), modulo ring size. */
static uint16_t impact_ring_distance(uint16_t start_idx, uint16_t end_idx)
{
    if (end_idx >= start_idx)
    {
        return (uint16_t)(end_idx - start_idx);
    }
    return (uint16_t)(IMPACT_RING_SIZE - start_idx + end_idx);
}

static void impact_detect_event(int16_t ax_mg, int16_t ay_mg, int16_t az_mg, uint32_t ts_us)
{
    float mag;
    uint16_t dynamic_mg;
    uint32_t hold_ms;

    mag = sqrtf((float)ax_mg * ax_mg + (float)ay_mg * ay_mg + (float)az_mg * az_mg);
    dynamic_mg = (uint16_t)(fabsf(mag - 1000.0f) + 0.5f);

    if ((int32_t)(ts_us - s_impact_ring.arm_guard_until_us) < 0)
    {
        return;
    }

    if (s_impact_ring.state == IMPACT_TRIG_STATE_COOLDOWN)
    {
        if ((int32_t)(ts_us - s_impact_ring.cooldown_until_us) < 0)
        {
            return;
        }
        LOG_PRINT("IMPACT state: COOLDOWN -> IDLE event_id=%lu\r\n",
                  (unsigned long)s_impact_ring.event_id);
        s_impact_ring.state = IMPACT_TRIG_STATE_IDLE;
        s_impact_ring.state_enter_us = ts_us;
    }

    if (s_impact_ring.state == IMPACT_TRIG_STATE_IDLE)
    {
        if (dynamic_mg >= s_trigger_cfg.trigger_high_mg)
        {
            s_impact_ring.trigger_idx = (uint16_t)((s_impact_ring.head + IMPACT_RING_SIZE - 1u) % IMPACT_RING_SIZE);
            s_impact_ring.trigger_valid = 1u;
            s_impact_ring.peak_dynamic_mg = dynamic_mg;
            s_impact_ring.state = IMPACT_TRIG_STATE_ARMED;
            s_impact_ring.state_enter_us = ts_us;
            LOG_PRINT("IMPACT state: IDLE -> ARMED event_id=%lu dyn=%umg H=%umg\r\n",
                      (unsigned long)s_impact_ring.event_id,
                      (unsigned)dynamic_mg,
                      (unsigned)s_trigger_cfg.trigger_high_mg);
            if (s_trigger_cfg.trigger_hold_ms == 0u)
            {
                s_impact_ring.state = IMPACT_TRIG_STATE_ACTIVE;
                s_impact_ring.state_enter_us = ts_us;
                LOG_PRINT("IMPACT state: ARMED -> ACTIVE immediate (T1=0)\r\n");
            }
        }
        return;
    }

    if (s_impact_ring.state == IMPACT_TRIG_STATE_ARMED)
    {
        if (dynamic_mg >= s_trigger_cfg.trigger_high_mg)
        {
            if (dynamic_mg > s_impact_ring.peak_dynamic_mg)
            {
                s_impact_ring.peak_dynamic_mg = dynamic_mg;
            }
            hold_ms = impact_elapsed_ms(ts_us, s_impact_ring.state_enter_us);
            if (hold_ms >= s_trigger_cfg.trigger_hold_ms)
            {
                s_impact_ring.state = IMPACT_TRIG_STATE_ACTIVE;
                s_impact_ring.state_enter_us = ts_us;
                LOG_PRINT("IMPACT state: ARMED -> ACTIVE hold=%lums peak=%umg\r\n",
                          (unsigned long)hold_ms,
                          (unsigned)s_impact_ring.peak_dynamic_mg);
            }
        }
        else
        {
            hold_ms = impact_elapsed_ms(ts_us, s_impact_ring.state_enter_us);
            LOG_PRINT("IMPACT reject: armed too short hold=%lums need=%lums peak=%umg\r\n",
                      (unsigned long)hold_ms,
                      (unsigned long)s_trigger_cfg.trigger_hold_ms,
                      (unsigned)s_impact_ring.peak_dynamic_mg);
            s_impact_ring.trigger_valid = 0u;
            s_impact_ring.peak_dynamic_mg = 0u;
            s_impact_ring.state = IMPACT_TRIG_STATE_IDLE;
            s_impact_ring.state_enter_us = ts_us;
        }
        return;
    }

    if (s_impact_ring.state == IMPACT_TRIG_STATE_ACTIVE)
    {
        if (dynamic_mg > s_impact_ring.peak_dynamic_mg)
        {
            s_impact_ring.peak_dynamic_mg = dynamic_mg;
        }
        if (dynamic_mg <= s_trigger_cfg.release_low_mg)
        {
            s_impact_ring.state = IMPACT_TRIG_STATE_RELEASED;
            s_impact_ring.state_enter_us = ts_us;
            LOG_PRINT("IMPACT state: ACTIVE -> RELEASED dyn=%umg L=%umg event_id=%lu\r\n",
                      (unsigned)dynamic_mg,
                      (unsigned)s_trigger_cfg.release_low_mg,
                      (unsigned long)s_impact_ring.event_id);
            if (s_trigger_cfg.release_hold_ms == 0u)
            {
                impact_finish_event(ts_us, "T3=0");
            }
        }
        return;
    }

    if (s_impact_ring.state == IMPACT_TRIG_STATE_RELEASED)
    {
        if (dynamic_mg <= s_trigger_cfg.release_low_mg)
        {
            hold_ms = impact_elapsed_ms(ts_us, s_impact_ring.state_enter_us);
            if (hold_ms >= s_trigger_cfg.release_hold_ms)
            {
                impact_finish_event(ts_us, "release stable");
            }
        }
        else
        {
            hold_ms = impact_elapsed_ms(ts_us, s_impact_ring.state_enter_us);
            LOG_PRINT("IMPACT reject: release unstable hold=%lums need=%lums dyn=%umg\r\n",
                      (unsigned long)hold_ms,
                      (unsigned long)s_trigger_cfg.release_hold_ms,
                      (unsigned)dynamic_mg);
            s_impact_ring.state = IMPACT_TRIG_STATE_ACTIVE;
            s_impact_ring.state_enter_us = ts_us;
            if (dynamic_mg > s_impact_ring.peak_dynamic_mg)
            {
                s_impact_ring.peak_dynamic_mg = dynamic_mg;
            }
        }
    }
}

static void impact_compute_and_report(void)
{
    uint16_t event_count;
    uint16_t post_count;
    uint16_t start_idx;
    uint16_t pre;
    uint16_t idx;
    uint32_t i;
    float sum_ax = 0.0f;
    float sum_ay = 0.0f;
    float sum_az = 0.0f;
    impact_disp_result_t result;
    impact_disp_status_t status;

    LOG_PRINT("IMPACT compute: event_id=%lu samples=%u peak=%umg\r\n",
              (unsigned long)s_impact_ring.event_id,
              (unsigned)s_impact_ring.count,
              (unsigned)s_impact_ring.peak_dynamic_mg);

    if (s_impact_ring.trigger_valid == 0u)
    {
        LOG_PRINT("IMPACT reject: no valid trigger (peak not reached or canceled)\r\n");
        return;
    }
    if (s_impact_ring.count < 20u)
    {
        LOG_PRINT("IMPACT reject: event too short, samples=%u\r\n", (unsigned)s_impact_ring.count);
        return;
    }

    post_count = impact_ring_distance(s_impact_ring.trigger_idx, s_impact_ring.head);
    if (post_count == 0u && s_impact_ring.count == IMPACT_RING_SIZE)
    {
        post_count = IMPACT_RING_SIZE;
    }
    if (post_count == 0u)
    {
        LOG_PRINT("IMPACT reject: invalid event window (post_count=0)\r\n");
        return;
    }

    event_count = (uint16_t)(post_count + IMPACT_PRE_SAMPLES);
    if (event_count > s_impact_ring.count)
    {
        event_count = s_impact_ring.count;
    }
    if (event_count < 2u)
    {
        LOG_PRINT("IMPACT reject: event window too short, count=%u\r\n", (unsigned)event_count);
        return;
    }

    pre = IMPACT_PRE_SAMPLES;
    if (pre >= event_count)
    {
        pre = (uint16_t)(event_count / 4u);
    }
    if (pre == 0u)
    {
        pre = 1u;
    }

    start_idx = (uint16_t)((s_impact_ring.head + IMPACT_RING_SIZE - event_count) % IMPACT_RING_SIZE);

    LOG_PRINT("IMPACT compute: start_idx=%u count=%u pre=%u\r\n",
              (unsigned)start_idx, (unsigned)event_count, (unsigned)pre);

    status = impact_disp_reset(&s_impact_ctx);
    if (status != IMPACT_DISP_OK)
    {
        LOG_PRINT("impact_disp_reset failed: %d\r\n", status);
        return;
    }

    status = impact_disp_begin_event(&s_impact_ctx, s_impact_ring.event_id);
    if (status != IMPACT_DISP_OK)
    {
        LOG_PRINT("impact_disp_begin_event failed: %d\r\n", status);
        return;
    }

    idx = start_idx;
    for (i = 0u; i < pre; ++i)
    {
        sum_ax += (float)s_impact_ring.samples[idx].ax_mg;
        sum_ay += (float)s_impact_ring.samples[idx].ay_mg;
        sum_az += (float)s_impact_ring.samples[idx].az_mg;
        idx = impact_ring_advance(idx, 1u);
    }

    status = impact_disp_set_baseline_mg(&s_impact_ctx,
                                         sum_ax / (float)pre,
                                         sum_ay / (float)pre,
                                         sum_az / (float)pre);
    if (status != IMPACT_DISP_OK)
    {
        LOG_PRINT("impact_disp_set_baseline_mg failed: %d\r\n", status);
        return;
    }

    idx = start_idx;
    for (i = 0u; i < event_count; ++i)
    {
        status = impact_disp_feed_sample(&s_impact_ctx, &s_impact_ring.samples[idx]);
        if (status != IMPACT_DISP_OK)
        {
            LOG_PRINT("impact_disp_feed_sample failed: %d idx=%u\r\n", status, (unsigned)idx);
            return;
        }
        idx = impact_ring_advance(idx, 1u);
    }

    status = impact_disp_end_event(&s_impact_ctx, &result);

    if (status == IMPACT_DISP_OK)
    {
        int32_t disp_tenth = impact_mm_to_tenths(result.disp_mm);
        int32_t dx_tenth = impact_mm_to_tenths(result.dx_mm);
        int32_t dy_tenth = impact_mm_to_tenths(result.dy_mm);
        int32_t dz_tenth = impact_mm_to_tenths(result.dz_mm);

        LOG_PRINT("IMPACT result: event_id=%lu disp=%ld.%01lumm dx=%ld.%01lu dy=%ld.%01lu dz=%ld.%01lu conf=%u%% flags=0x%08lX\r\n",
                  (unsigned long)result.event_id,
                  (long)(disp_tenth / 10), (unsigned long)(impact_abs_u32_from_i32(disp_tenth % 10)),
                  (long)(dx_tenth / 10), (unsigned long)(impact_abs_u32_from_i32(dx_tenth % 10)),
                  (long)(dy_tenth / 10), (unsigned long)(impact_abs_u32_from_i32(dy_tenth % 10)),
                  (long)(dz_tenth / 10), (unsigned long)(impact_abs_u32_from_i32(dz_tenth % 10)),
                  result.confidence, (unsigned long)result.quality_flags);
        if ((result.quality_flags & IMPACT_DISP_QF_NO_RELEASE) != 0u)
        {
            LOG_PRINT("IMPACT reject-hint: release insufficient (T3=%ums, L=%umg, rel_cnt=%u/%u)\r\n",
                      (unsigned)s_trigger_cfg.release_hold_ms,
                      (unsigned)s_trigger_cfg.release_low_mg,
                      (unsigned)s_impact_ctx.release_count,
                      (unsigned)s_impact_ctx.cfg.release_count_min);
        }
        if ((result.quality_flags & IMPACT_DISP_QF_DT_GAP) != 0u)
        {
            LOG_PRINT("IMPACT reject-hint: dt gap (max_dt=%ums, duration=%lums, samples=%lu)\r\n",
                      (unsigned)s_impact_ctx.cfg.max_dt_ms,
                      (unsigned long)result.duration_ms,
                      (unsigned long)result.sample_count);
        }
    }
    else
    {
        LOG_PRINT("impact_disp_end_event failed: %d (release_hold=%ums)\r\n",
                  status, (unsigned)s_trigger_cfg.release_hold_ms);
    }
}
