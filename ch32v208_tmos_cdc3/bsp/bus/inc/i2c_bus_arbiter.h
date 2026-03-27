/**
 ******************************************************************************
 * @file    i2c_bus_arbiter.h
 * @brief   I2C总线仲裁器头文件
 ******************************************************************************
 * @details 本文件定义了I2C总线仲裁器的核心数据结构和API接口。
 *          I2C总线仲裁器用于在多任务环境中统一管理对I2C总线的访问，
 *          提供以下功能：
 *          - 请求队列管理（支持优先级队列）
 *          - 超时检测和恢复机制
 *          - 统计信息收集
 *          - 同步/异步请求提交
 *          - 请求取消功能
 *          
 *          设计特点：
 *          - 支持多优先级队列（高、正常、低优先级）
 *          - 基于轮询的仲裁机制，避免中断嵌套问题
 *          - 完善的错误处理和总线恢复机制
 *          - 详细的统计信息，便于性能分析和调试
 *
 ******************************************************************************
 */
#ifndef __I2C_BUS_ARBITER_H
#define __I2C_BUS_ARBITER_H

#include "drv_i2c.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I2C总线请求类型枚举
 */
typedef enum
{
    I2C_BUS_REQ_WRITE = 0,    ///< 写操作请求
    I2C_BUS_REQ_READ,         ///< 读操作请求  
    I2C_BUS_REQ_WRITE_REG,    ///< 写寄存器请求
    I2C_BUS_REQ_READ_REG,     ///< 读寄存器请求
} i2c_bus_req_type_t;

/**
 * @brief I2C总线请求优先级枚举
 */
typedef enum
{
    I2C_BUS_PRIO_HIGH = 0,    ///< 高优先级（如传感器关键数据读取）
    I2C_BUS_PRIO_NORMAL = 1,  ///< 正常优先级（默认优先级）
    I2C_BUS_PRIO_LOW = 2,     ///< 低优先级（如配置寄存器写入）
    I2C_BUS_PRIO_MAX          ///< 优先级最大值（用于边界检查）
} i2c_bus_prio_t;

/**
 * @brief I2C总线请求结构体
 * @details 封装了完整的I2C操作请求信息，包括总线、设备地址、数据缓冲区等。
 */
typedef struct
{
    i2c_num_t bus;            ///< I2C总线编号
    i2c_bus_req_type_t type;  ///< 请求类型
    uint8_t owner_id;         ///< 逻辑所有者/模块ID（用于请求取消）
    uint8_t dev_addr;         ///< 设备地址
    uint8_t reg;              ///< 寄存器地址（仅用于寄存器操作）
    const uint8_t *wbuf;      ///< 写缓冲区指针
    uint8_t *rbuf;            ///< 读缓冲区指针
    uint16_t len;             ///< 数据长度
    uint8_t mode_hint;        ///< 模式提示：I2C_MODE_POLLING / I2C_MODE_IT / I2C_MODE_DMA
    uint8_t prio;             ///< 优先级：i2c_bus_prio_t
    uint32_t timeout_ms;      ///< 超时时间（毫秒），0表示使用默认超时
} i2c_bus_request_t;

/**
 * @brief I2C总线统计信息结构体
 * @details 收集I2C总线操作的各种统计信息，用于性能分析和故障诊断。
 */
typedef struct
{
    uint32_t submit_ok;           ///< 成功提交的请求数
    uint32_t submit_invalid;      ///< 无效提交的请求数
    uint32_t submit_queue_full;   ///< 队列满导致提交失败的请求数
    uint32_t started;             ///< 已启动的请求数
    uint32_t done_ok;             ///< 成功完成的请求数
    uint32_t done_err;            ///< 错误完成的请求数
    uint32_t timeout_cnt;         ///< 超时次数
    uint32_t recover_cnt;         ///< 恢复次数
    uint32_t avg_wait_ms;         ///< 平均等待时间（毫秒）
    uint32_t avg_exec_ms;         ///< 平均执行时间（毫秒）
    uint32_t max_wait_ms;         ///< 最大等待时间（毫秒）
    uint32_t max_exec_ms;         ///< 最大执行时间（毫秒）
    int32_t last_status;          ///< 上次操作状态
    uint8_t queue_depth_peak;     ///< 队列深度峰值
    uint8_t queue_depth_curr;     ///< 当前队列深度
    uint8_t running;              ///< 是否正在运行
} i2c_bus_stats_t;

/**
 * @brief 初始化I2C总线仲裁器
 * @details 初始化指定I2C总线的仲裁器上下文，清空队列和统计信息。
 * @param bus I2C总线编号
 */
void i2c_bus_arbiter_init(i2c_num_t bus);

/**
 * @brief 处理I2C总线仲裁器
 * @details 处理I2C总线仲裁器的主循环函数，需要在系统主循环或定时器中定期调用。
 *          执行以下操作：
 *          - 从队列中取出下一个请求并开始执行
 *          - 监控当前请求的执行状态和超时情况
 *          - 处理请求完成或超时事件
 * @param bus I2C总线编号
 */
void i2c_bus_arbiter_process(i2c_num_t bus);

/**
 * @brief 同步提交I2C总线请求
 * @details 同步方式提交I2C请求，阻塞等待直到请求完成或超时。
 *          内部会将请求加入队列并轮询等待完成。
 * @param req I2C总线请求指针
 * @return 成功返回0，失败返回负错误码（-EINVAL, -EBUSY, -ETIMEDOUT等）
 */
int i2c_bus_submit_sync(const i2c_bus_request_t *req);

/**
 * @brief 获取I2C总线统计信息
 * @details 获取指定I2C总线的详细统计信息，用于性能监控和调试。
 * @param bus I2C总线编号
 * @param stats 统计信息结构体指针
 */
void i2c_bus_get_stats(i2c_num_t bus, i2c_bus_stats_t *stats);

/**
 * @brief 重置I2C总线统计信息
 * @details 清空指定I2C总线的所有统计信息，重新开始计数。
 * @param bus I2C总线编号
 */
void i2c_bus_reset_stats(i2c_num_t bus);

/**
 * @brief 根据设备地址取消排队中的请求
 * @details 取消指定设备地址的所有排队请求，用于设备异常时的清理操作。
 * @param bus I2C总线编号
 * @param dev_addr 设备地址
 * @return 取消的请求数量
 */
int i2c_bus_cancel_by_dev(i2c_num_t bus, uint8_t dev_addr);

/**
 * @brief 根据所有者ID取消排队中的请求
 * @details 取消指定所有者ID的所有排队请求，用于模块卸载时的清理操作。
 * @param bus I2C总线编号
 * @param owner_id 所有者ID
 * @return 取消的请求数量
 */
int i2c_bus_cancel_by_owner(i2c_num_t bus, uint8_t owner_id);

#ifdef __cplusplus
}
#endif

#endif