/**
 * @file    sensor_task.h
 * @brief   传感器任务对外接口与数据快照定义
 */
#ifndef __SENSOR_TASK_H
#define __SENSOR_TASK_H

#include <stdint.h>

/* 传感器共享快照（温度/湿度单位为 0.01） */
typedef struct
{
    int16_t accel_raw_x;
    int16_t accel_raw_y;
    int16_t accel_raw_z;
    int32_t accel_mg_x;
    int32_t accel_mg_y;
    int32_t accel_mg_z;
    int32_t temp_centi_c;
    int32_t rh_centi_pct;
    uint16_t accel_hz;
    uint16_t sht_hz;
    uint32_t sht_ok_cnt;
    uint32_t sht_err_cnt;
    uint8_t ready;
} sensor_snapshot_t;

/**
 * @brief 初始化传感器任务
 */
void sensor_task_init(void);

/**
 * @brief 读取最近一次传感器快照
 * @param out 输出缓冲区指针，传入 NULL 时函数直接返回
 */
void sensor_task_get_snapshot(sensor_snapshot_t *out);

#endif /* __SENSOR_TASK_H */
