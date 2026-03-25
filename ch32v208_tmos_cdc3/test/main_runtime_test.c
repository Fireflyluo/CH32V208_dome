#include "main_runtime_test.h"

#include "CONFIG.h"
#include "board.h"
#include "drv_i2c.h"
#include "i2c_bus_arbiter.h"
#include "sc7a20.h"
#include "sc7a20_ch32_adapter.h"
#include "sht40.h"
#include "sht40_ch32_adapter.h"
#include "usb_cdc.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 传感器数据上报周期（毫秒）
#define SENSOR_REPORT_PERIOD_MS 1000U
// I2C统计信息上报周期（毫秒）
#define I2C_STATS_REPORT_PERIOD_MS 5000U
// 传感器数据行最大长度
#define SENSOR_LINE_MAX_LEN 224U

// 全局SC7A20加速度计设备实例
static sc7a20_dev_t g_sc7a20_dev;
// SC7A20的I2C总线上下文配置
static sc7a20_ch32_bus_ctx_t g_sc7a20_bus = {
    .i2c_num = I2C_NUM_1,           ///< 使用I2C1总线
    .dev_addr = SC7A20_I2C_ADDR_H,  ///< 设备地址（高地址模式）
};

// 全局SHT40温湿度传感器设备实例
static sht40_dev_t g_sht40_dev;
// SHT40的I2C总线上下文配置
static sht40_ch32_bus_ctx_t g_sht40_bus = {
    .i2c_num = I2C_NUM_1,     ///< 使用I2C1总线
    .dev_addr = SHT40_I2C_ADDR, ///< 设备地址
};

// 传感器就绪标志
static uint8_t g_sensors_ready = 0U;
// 上次传感器数据上报时间戳
static uint32_t g_last_report_tick = 0U;
// 上次I2C统计信息上报时间戳
static uint32_t g_last_i2c_stats_tick = 0U;

/**
 * @brief 格式化固定点数为带小数的字符串
 * @param value 带符号的整数值
 * @param scale 缩放因子（用于小数点位置）
 * @param frac_digits 小数位数（0, 2, 或 3）
 * @param out 输出缓冲区
 * @param out_len 输出缓冲区长度
 */
static void format_fixed_scaled(int32_t value, uint32_t scale, uint8_t frac_digits, char *out, size_t out_len)
{
    uint32_t abs_v;
    uint32_t ip;      // 整数部分
    uint32_t fp;      // 小数部分
    const char *sign; // 符号

    if (out == NULL || out_len == 0U || scale == 0U)
    {
        return;
    }

    sign = (value < 0) ? "-" : "";
    abs_v = (value < 0) ? (uint32_t)(-value) : (uint32_t)value;
    ip = abs_v / scale;
    fp = abs_v % scale;

    if (frac_digits == 3U)
    {
        (void)snprintf(out, out_len, "%s%lu.%03lu", sign, (unsigned long)ip, (unsigned long)fp);
    }
    else if (frac_digits == 2U)
    {
        (void)snprintf(out, out_len, "%s%lu.%02lu", sign, (unsigned long)ip, (unsigned long)fp);
    }
    else
    {
        (void)snprintf(out, out_len, "%s%lu", sign, (unsigned long)ip);
    }
}

/**
 * @brief SHT40传感器延迟适配器函数
 * @param ctx 上下文指针（未使用）
 * @param ms 延迟时间（毫秒）
 */
static void sht40_delay_adapter(void *ctx, uint32_t ms)
{
    (void)ctx;
    HAL_Delay(ms);
}

/**
 * @brief 初始化传感器（新版本）
 * @return 成功返回0，失败返回负错误码
 */
static int sensors_init_new(void)
{
    int rc;
    sc7a20_cfg_t cfg;
    uint32_t serial = 0U;

    // 配置SC7A20加速度计参数
    cfg = g_sc7a20_default_cfg;
    cfg.range = SC7A20_ACCEL_FS_2G;    ///< 量程：±2g
    cfg.odr = SC7A20_ACCEL_ODR_50HZ;   ///< 输出数据率：50Hz
    cfg.high_resolution = true;        ///< 启用高分辨率模式

    // 设置SC7A20设备操作接口和总线上下文
    g_sc7a20_dev.ops = &g_sc7a20_ch32_i2c_ops;
    g_sc7a20_dev.bus_ctx = &g_sc7a20_bus;
    g_sc7a20_dev.addr = g_sc7a20_bus.dev_addr;

    // 初始化SC7A20
    rc = sc7a20_init_with_config(&g_sc7a20_dev, &cfg);
    if (rc != 0)
    {
        PRINT("SC7A20(new) init failed: %d\r\n", rc);
        return -1;
    }

    // 设置SHT40设备操作接口和总线上下文
    g_sht40_dev.ops = &g_sht40_ch32_i2c_ops;
    g_sht40_dev.bus_ctx = &g_sht40_bus;
    g_sht40_dev.addr = g_sht40_bus.dev_addr;
    g_sht40_dev.delay_ms = sht40_delay_adapter;
    g_sht40_dev.delay_ctx = NULL;

    // 初始化SHT40
    rc = sht40_init(&g_sht40_dev);
    if (rc != 0)
    {
        PRINT("SHT40(new) init failed: %d\r\n", rc);
        return -2;
    }

    // 读取SHT40序列号
    if (sht40_read_serial(&g_sht40_dev, &serial) == 0)
    {
        PRINT("SHT40(new) serial: 0x%08lx\r\n", serial);
    }

    // 标记传感器已就绪并重置I2C统计信息
    g_sensors_ready = 1U;
    i2c_bus_reset_stats(I2C_NUM_1);
    return 0;
}

/**
 * @brief 读取SC7A20加速度计采样数据
 * @param raw 原始数据输出指针
 * @param g 转换后的重力加速度数据输出指针
 * @return 成功返回0，失败返回负错误码
 */
static int read_sc7a20_sample(sc7a20_vec3i16_t *raw, sc7a20_vec3f_t *g)
{
    int rc;

    if (g_sensors_ready == 0U)
    {
        return -19;
    }

    // 读取原始XYZ轴数据
    rc = sc7a20_read_xyz_raw(&g_sc7a20_dev, raw);
    if (rc != 0)
    {
        return rc;
    }

    // 转换为重力加速度单位（g）
    g->x = raw->x * g_sc7a20_dev.sensitivity_g_per_lsb;
    g->y = raw->y * g_sc7a20_dev.sensitivity_g_per_lsb;
    g->z = raw->z * g_sc7a20_dev.sensitivity_g_per_lsb;
    return 0;
}

/**
 * @brief 通过USB CDC发送文本数据
 * @param text 要发送的文本
 */
static void cdc_send_text(const char *text)
{
    uint16_t remaining;
    uint16_t chunk_len;
    CDC_ErrCode_t cdc_ret;

    if (text == NULL)
    {
        return;
    }

    // 分块发送，避免超过单包大小限制
    remaining = (uint16_t)strlen(text);
    while (remaining > 0U)
    {
        chunk_len = (remaining > CDC_MAX_PACKET_SIZE) ? CDC_MAX_PACKET_SIZE : remaining;
        cdc_ret = CDC_SendData((uint8_t *)text, chunk_len);
        if (cdc_ret != CDC_SUCCESS)
        {
            break;
        }
        text += chunk_len;
        remaining = (uint16_t)(remaining - chunk_len);
    }
}

/**
 * @brief 去除字符串末尾的空白字符和换行符
 * @param line 要处理的字符串
 */
static void trim_line(char *line)
{
    size_t n;

    if (line == NULL)
    {
        return;
    }

    n = strlen(line);
    while (n > 0U)
    {
        char c = line[n - 1U];
        if (c == '\r' || c == '\n' || c == ' ' || c == '\t')
        {
            line[n - 1U] = '\0';
            n--;
        }
        else
        {
            break;
        }
    }
}

/**
 * @brief 处理USB命令
 * @param rx 接收到的命令数据
 * @param len 数据长度
 * @param resp 响应缓冲区
 * @param resp_size 响应缓冲区大小
 * @return 成功处理返回1，未处理返回0
 */
static int process_usb_cmd(const uint8_t *rx, uint16_t len, char *resp, size_t resp_size)
{
    char cmd[80];
    size_t copy_len;
    unsigned long v;
    char *endptr;
    int rc;
    i2c_bus_stats_t st;

    if (rx == NULL || len == 0U || resp == NULL || resp_size == 0U)
    {
        return 0;
    }

    // 复制命令并去除末尾空白
    copy_len = (len >= sizeof(cmd)) ? (sizeof(cmd) - 1U) : (size_t)len;
    memcpy(cmd, rx, copy_len);
    cmd[copy_len] = '\0';
    trim_line(cmd);

    // 处理取消指定所有者ID的I2C请求命令
    if (strncmp(cmd, "i2c_cancel owner ", 17) == 0)
    {
        v = strtoul(cmd + 17, &endptr, 0);
        if ((endptr == (cmd + 17)) || (*endptr != '\0'))
        {
            (void)snprintf(resp, resp_size, "CMD ERR: invalid owner id\r\n");
            return 1;
        }
        rc = i2c_bus_cancel_by_owner(I2C_NUM_1, (uint8_t)v);
        (void)snprintf(resp, resp_size, "CMD OK: cancel owner %lu -> %d\r\n", v, rc);
        return 1;
    }

    // 处理取消指定设备地址的I2C请求命令
    if (strncmp(cmd, "i2c_cancel dev ", 15) == 0)
    {
        v = strtoul(cmd + 15, &endptr, 0);
        if ((endptr == (cmd + 15)) || (*endptr != '\0'))
        {
            (void)snprintf(resp, resp_size, "CMD ERR: invalid dev addr\r\n");
            return 1;
        }
        rc = i2c_bus_cancel_by_dev(I2C_NUM_1, (uint8_t)v);
        (void)snprintf(resp, resp_size, "CMD OK: cancel dev 0x%02lX -> %d\r\n", v & 0xFFUL, rc);
        return 1;
    }

    // 处理重置I2C统计信息命令
    if (strcmp(cmd, "i2c_stats reset") == 0)
    {
        i2c_bus_reset_stats(I2C_NUM_1);
        (void)snprintf(resp, resp_size, "CMD OK: i2c stats reset\r\n");
        return 1;
    }

    // 处理显示I2C统计信息命令
    if (strcmp(cmd, "i2c_stats show") == 0)
    {
        i2c_bus_get_stats(I2C_NUM_1, &st);
        (void)snprintf(resp,
                       resp_size,
                       "I2C_ARB_NOW: ok=%lu full=%lu err=%lu to=%lu rec=%lu q=%u/%u run=%u\r\n",
                       (unsigned long)st.submit_ok,
                       (unsigned long)st.submit_queue_full,
                       (unsigned long)st.done_err,
                       (unsigned long)st.timeout_cnt,
                       (unsigned long)st.recover_cnt,
                       (unsigned int)st.queue_depth_curr,
                       (unsigned int)st.queue_depth_peak,
                       (unsigned int)st.running);
        return 1;
    }

    return 0;
}

/**
 * @brief 主运行时测试初始化函数
 */
void main_runtime_test_init(void)
{
    if (sensors_init_new() != 0)
    {
        g_sensors_ready = 0U;
        PRINT("Initial sensor init failed, will retry in loop\r\n");
    }
}

/**
 * @brief 主运行时测试处理函数
 * 
 * 此函数在主循环中定期调用，负责：
 * 1. 处理USB CDC接收的命令
 * 2. 定期读取并上报传感器数据
 * 3. 定期上报I2C总线统计信息
 */
void main_runtime_test_process(void)
{
    uint8_t usb_rx_buf[64];           // USB接收缓冲区
    uint8_t usb_tx_buf[64];           // USB发送缓冲区
    uint16_t usb_rx_len;              // 接收数据长度
    uint16_t usb_tx_len;              // 发送数据长度
    uint32_t now_tick;                // 当前时间戳
    char sensor_line[SENSOR_LINE_MAX_LEN];  // 传感器数据显示行
    sc7a20_vec3i16_t accel_raw;       // 加速度计原始数据
    sc7a20_vec3f_t accel_g;           // 加速度计重力单位数据
    sht40_sample_t sht_sample;        // SHT40采样数据
    int32_t accel_x_mg;               // X轴加速度（毫重力）
    int32_t accel_y_mg;               // Y轴加速度（毫重力）
    int32_t accel_z_mg;               // Z轴加速度（毫重力）
    int32_t temp_centi;               // 温度（百分之一摄氏度）
    int32_t rh_centi;                 // 相对湿度（百分之一百分比）
    char ax_buf[16];                  // X轴数据显示缓冲区
    char ay_buf[16];                  // Y轴数据显示缓冲区
    char az_buf[16];                  // Z轴数据显示缓冲区
    char temp_buf[16];                // 温度数据显示缓冲区
    char rh_buf[16];                  // 湿度数据显示缓冲区
    int accel_ret;                    // 加速度计读取结果
    int sht_ret;                      // SHT40读取结果
    i2c_bus_stats_t i2c_stats;        // I2C统计信息
    int cmd_handled;                  // 命令处理标志

    // 处理USB CDC接收到的数据
    usb_rx_len = CDC_ReceiveData(usb_rx_buf, sizeof(usb_rx_buf));
    if (usb_rx_len > 0U)
    {
        cmd_handled = process_usb_cmd(usb_rx_buf, usb_rx_len, (char *)usb_tx_buf, sizeof(usb_tx_buf));
        if (cmd_handled != 0)
        {
            cdc_send_text((const char *)usb_tx_buf);
        }
        else
        {
            // 回显未识别的命令
            usb_tx_len = (usb_rx_len > 58U) ? 58U : usb_rx_len;
            memcpy(usb_tx_buf, "ECHO: ", 6U);
            memcpy(&usb_tx_buf[6], usb_rx_buf, usb_tx_len);
            (void)CDC_SendData(usb_tx_buf, (uint16_t)(usb_tx_len + 6U));
        }
    }

    // 定期读取和上报传感器数据
    now_tick = HAL_GetTick();
    if ((uint32_t)(now_tick - g_last_report_tick) >= SENSOR_REPORT_PERIOD_MS)
    {
        g_last_report_tick = now_tick;
        if (g_sensors_ready == 0U)
        {
            // 如果传感器未就绪，尝试重新初始化
            if (sensors_init_new() != 0)
            {
                (void)snprintf(sensor_line, sizeof(sensor_line), "Sensors init retry failed\r\n");
                PRINT("%s", sensor_line);
                cdc_send_text(sensor_line);
                return;
            }
            (void)snprintf(sensor_line, sizeof(sensor_line), "Sensors re-init success\r\n");
            PRINT("%s", sensor_line);
            cdc_send_text(sensor_line);
        }

        // 读取传感器数据
        accel_ret = read_sc7a20_sample(&accel_raw, &accel_g);
        sht_ret = sht40_read_sample(&g_sht40_dev, SHT40_PRECISION_HIGH, &sht_sample);
        if (accel_ret == 0 && sht_ret == 0)
        {
            // 转换数据格式
            accel_x_mg = (int32_t)(accel_g.x * 1000.0f);
            accel_y_mg = (int32_t)(accel_g.y * 1000.0f);
            accel_z_mg = (int32_t)(accel_g.z * 1000.0f);
            temp_centi = (int32_t)(sht_sample.temperature_c * 100.0f);
            rh_centi = (int32_t)(sht_sample.humidity_rh * 100.0f);

            // 格式化数据显示
            format_fixed_scaled(accel_x_mg, 1000U, 3U, ax_buf, sizeof(ax_buf));
            format_fixed_scaled(accel_y_mg, 1000U, 3U, ay_buf, sizeof(ay_buf));
            format_fixed_scaled(accel_z_mg, 1000U, 3U, az_buf, sizeof(az_buf));
            format_fixed_scaled(temp_centi, 100U, 2U, temp_buf, sizeof(temp_buf));
            format_fixed_scaled(rh_centi, 100U, 2U, rh_buf, sizeof(rh_buf));

            // 组合传感器数据行
            (void)snprintf(sensor_line,
                           sizeof(sensor_line),
                           "SC7A20[g]: X=%s Y=%s Z=%s (raw:%d,%d,%d) | SHT40: T=%sC RH=%s%%\r\n",
                           ax_buf,
                           ay_buf,
                           az_buf,
                           accel_raw.x,
                           accel_raw.y,
                           accel_raw.z,
                           temp_buf,
                           rh_buf);
        }
        else
        {
            // 传感器读取错误
            (void)snprintf(sensor_line,
                           sizeof(sensor_line),
                           "Sensor read err: sc7a20=%d sht40=%d\r\n",
                           accel_ret,
                           sht_ret);
        }

        // 输出到调试串口和USB CDC
        PRINT("%s", sensor_line);
        cdc_send_text(sensor_line);
    }

    // 定期上报I2C总线统计信息
    if ((uint32_t)(now_tick - g_last_i2c_stats_tick) >= I2C_STATS_REPORT_PERIOD_MS)
    {
        g_last_i2c_stats_tick = now_tick;
        i2c_bus_get_stats(I2C_NUM_1, &i2c_stats);
        (void)snprintf(sensor_line,
                       sizeof(sensor_line),
                       "I2C_ARB: ok=%lu inv=%lu full=%lu start=%lu done_ok=%lu done_err=%lu to=%lu rec=%lu wait(avg/max)=%lu/%lums exec(avg/max)=%lu/%lums q=%u/%u run=%u last=%ld\r\n",
                       (unsigned long)i2c_stats.submit_ok,
                       (unsigned long)i2c_stats.submit_invalid,
                       (unsigned long)i2c_stats.submit_queue_full,
                       (unsigned long)i2c_stats.started,
                       (unsigned long)i2c_stats.done_ok,
                       (unsigned long)i2c_stats.done_err,
                       (unsigned long)i2c_stats.timeout_cnt,
                       (unsigned long)i2c_stats.recover_cnt,
                       (unsigned long)i2c_stats.avg_wait_ms,
                       (unsigned long)i2c_stats.max_wait_ms,
                       (unsigned long)i2c_stats.avg_exec_ms,
                       (unsigned long)i2c_stats.max_exec_ms,
                       (unsigned int)i2c_stats.queue_depth_curr,
                       (unsigned int)i2c_stats.queue_depth_peak,
                       (unsigned int)i2c_stats.running,
                       (long)i2c_stats.last_status);
        PRINT("%s", sensor_line);
        cdc_send_text(sensor_line);
    }
}