/**
 * @file    sht40_core.h
 * @brief   SHT40温湿度传感器驱动核心头文件
 * @details 本文件定义了SHT40驱动的核心数据结构、类型定义和内部API。
 *          包含设备结构体、总线操作接口、通信消息格式、精度枚举等核心组件。
 *          
 *          设计原则：
 *          - 基于统一的总线抽象层（sht40_bus_ops_t）
 *          - 支持同步和异步操作模式
 *          - 提供内存操作自定义选项（SHT40_USE_CUSTOM_MEMOPS）
 *          - 线程安全的设备访问控制（in_use标志）
 *          - 支持用户自定义延迟函数（用于测量等待）
 *          
 *          核心组件：
 *          - sht40_comm_msg_t: 通信消息结构体，用于描述I2C事务
 *          - sht40_bus_ops_t: 总线操作接口，由用户实现
 *          - sht40_dev_t: 设备结构体，包含所有设备状态信息
 *          - sht40_async_ctx_t: 异步操作上下文，管理异步请求状态
 *
 * @author  WCH (南京沁恒微电子股份有限公司)
 * @version V1.0.0
 * @date    2022/06/16
 */

#ifndef SHT40_NEW_CORE_H
#define SHT40_NEW_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#ifndef SHT40_USE_CUSTOM_MEMOPS
#define SHT40_USE_CUSTOM_MEMOPS 0
#endif

#if SHT40_USE_CUSTOM_MEMOPS
#ifndef SHT40_MEMSET
#error "SHT40_USE_CUSTOM_MEMOPS=1 requires SHT40_MEMSET to be defined"
#endif
#ifndef SHT40_MEMCPY
#error "SHT40_USE_CUSTOM_MEMOPS=1 requires SHT40_MEMCPY to be defined"
#endif
#else
#include <string.h>
#ifndef SHT40_MEMSET
#define SHT40_MEMSET memset
#endif
#ifndef SHT40_MEMCPY
#define SHT40_MEMCPY memcpy
#endif
#endif

#ifndef SHT40_COMM_WRITE
#define SHT40_COMM_WRITE (1u << 0)
#endif
#ifndef SHT40_COMM_READ
#define SHT40_COMM_READ  (1u << 1)
#endif
#ifndef SHT40_COMM_STOP
#define SHT40_COMM_STOP  (1u << 2)
#endif

#ifndef SHT40_I2C_ADDR
#define SHT40_I2C_ADDR 0x46u
#endif

/**
 * @brief  SHT40测量精度等级枚举
 */
typedef enum {
    SHT40_PRECISION_HIGH = 0,       /**< 高精度模式 */
    SHT40_PRECISION_MEDIUM,         /**< 中等精度模式 */
    SHT40_PRECISION_LOW             /**< 低精度模式 */
} sht40_precision_t;

/**
 * @brief  SHT40加热器控制命令枚举
 * @details 定义了不同功率和持续时间的加热器命令。
 */
typedef enum {
    SHT40_HEATER_200MW_1S = 0x39,   /**< 200mW, 1秒 */
    SHT40_HEATER_200MW_100MS = 0x32,/**< 200mW, 100毫秒 */
    SHT40_HEATER_110MW_1S = 0x2F,   /**< 110mW, 1秒 */
    SHT40_HEATER_110MW_100MS = 0x24,/**< 110mW, 100毫秒 */
    SHT40_HEATER_20MW_1S = 0x1E,    /**< 20mW, 1秒 */
    SHT40_HEATER_20MW_100MS = 0x15  /**< 20mW, 100毫秒 */
} sht40_heater_cmd_t;

/**
 * @brief  SHT40通信消息结构体
 * @details 描述单个I2C通信事务的消息结构。
 */
typedef struct {
    uint8_t *buf;       /**< 数据缓冲区指针 */
    uint16_t len;       /**< 数据长度 */
    uint8_t flags;      /**< 通信标志（READ/WRITE/STOP） */
} sht40_comm_msg_t;

/**
 * @brief  总线操作完成回调函数类型
 * @param[in] user 用户上下文指针
 * @param[in] status 操作状态（0表示成功，负数表示错误）
 */
typedef void (*sht40_bus_done_cb_t)(void *user, int status);

/**
 * @brief  SHT40总线操作接口
 * @details 用户需要实现此结构体中的函数来提供底层总线访问能力。
 */
typedef struct {
    /**
     * @brief  执行总线传输操作
     * @param[in] ctx 总线上下文指针
     * @param[in] msgs 通信消息数组指针
     * @param[in] cnt 消息数量
     * @param[in] cb 完成回调函数指针
     * @param[in] user 用户上下文指针
     * @return 0表示成功提交，负数表示错误
     */
    int (*xfer)(void *ctx,
                const sht40_comm_msg_t *msgs,
                uint8_t cnt,
                sht40_bus_done_cb_t cb,
                void *user);
    /**
     * @brief  取消当前传输操作
     * @param[in] ctx 总线上下文指针
     * @return 0表示成功，负数表示错误
     */
    int (*cancel)(void *ctx);
} sht40_bus_ops_t;

/**
 * @brief  延迟函数回调类型
 * @details 用于在测量过程中执行精确的延时等待。
 * @param[in] ctx 用户上下文指针
 * @param[in] ms 延迟时间（毫秒）
 */
typedef void (*sht40_delay_ms_fn)(void *ctx, uint32_t ms);

/**
 * @brief  SHT40温湿度样本结构体
 * @details 存储温度和湿度的工程单位值。
 */
typedef struct {
    float temperature_c;    /**< 温度（摄氏度） */
    float humidity_rh;      /**< 相对湿度（%RH） */
} sht40_sample_t;

/**
 * @brief  通用操作完成回调函数类型
 * @param[in] user 用户上下文指针
 * @param[in] status 操作状态（0表示成功，负数表示错误）
 */
typedef void (*sht40_done_cb_t)(void *user, int status);

/**
 * @brief  温湿度样本读取完成回调函数类型
 * @param[in] user 用户上下文指针
 * @param[in] sample 指向温湿度样本的指针
 * @param[in] status 操作状态（0表示成功，负数表示错误）
 */
typedef void (*sht40_sample_cb_t)(void *user, const sht40_sample_t *sample, int status);

/**
 * @brief  异步操作类型枚举
 */
typedef enum {
    SHT40_ASYNC_NONE = 0,           /**< 无操作 */
    SHT40_ASYNC_READ_SAMPLE,        /**< 读取样本操作 */
    SHT40_ASYNC_SOFT_RESET          /**< 软复位操作 */
} sht40_async_op_t;

/**
 * @brief  异步操作上下文结构体
 * @details 管理当前正在进行的异步操作的状态和参数。
 */
typedef struct {
    sht40_async_op_t op;            /**< 当前异步操作类型 */
    uint8_t cmd;                    /**< 命令字节 */
    uint8_t rx[6];                  /**< 接收数据缓冲区 */
    sht40_sample_cb_t sample_cb;    /**< 样本完成回调 */
    sht40_done_cb_t done_cb;        /**< 通用完成回调 */
    void *user;                     /**< 用户上下文 */
} sht40_async_ctx_t;

/**
 * @brief  SHT40设备结构体
 * @details 包含设备的所有状态信息和配置参数。
 */
typedef struct {
    const sht40_bus_ops_t *ops;     /**< 总线操作接口指针 */
    void *bus_ctx;                  /**< 总线上下文指针 */
    uint8_t addr;                   /**< 设备I2C地址 */

    sht40_delay_ms_fn delay_ms;     /**< 延迟函数指针 */
    void *delay_ctx;                /**< 延迟函数上下文 */

    bool initialized;               /**< 初始化状态标志 */
    volatile uint8_t in_use;        /**< 使用中标志（线程安全） */
    sht40_async_ctx_t async;        /**< 异步操作上下文 */
} sht40_dev_t;

/**
 * @brief  尝试获取设备锁
 * @details 在多线程环境中尝试获取设备访问锁，防止并发访问冲突。
 *          使用原子操作检查并设置 in_use 标志。
 * 
 * @param[in,out] dev 指向 SHT40 设备结构体的指针
 * @return 0 表示成功获取锁，-1 表示设备正忙
 */
int sht40_core_try_lock(sht40_dev_t *dev);

/**
 * @brief  释放设备锁
 * @details 释放之前获取的设备访问锁，将 in_use 标志清零。
 *          允许其他线程访问设备。
 * 
 * @param[in,out] dev 指向 SHT40 设备结构体的指针
 */
void sht40_core_unlock(sht40_dev_t *dev);

/**
 * @brief  验证设备结构体有效性
 * @details 检查设备结构体的各个字段是否有效，包括：
 *          - 总线操作接口指针非空
 *          - xfer 和 cancel 函数指针非空
 *          - 延迟函数指针非空
 *          - 设备已初始化
 * 
 * @param[in] dev 指向 SHT40 设备结构体的指针
 * @return 0 表示有效，负数表示无效（-1 表示空指针，-2 表示未初始化等）
 */
int sht40_core_validate_dev(const sht40_dev_t *dev);

/**
 * @brief  映射总线状态码
 * @details 将底层总线驱动的状态码映射为统一的错误码。
 *          便于上层应用处理错误，屏蔽底层差异。
 * 
 * @param[in] status 底层总线状态码
 * @return 统一的错误码（0 表示成功，负数表示错误）
 */
int sht40_core_map_bus_status(int status);

/**
 * @brief  根据精度等级获取测量命令
 * @details 将精度枚举值转换为对应的 I2C 命令字节。
 *          不同精度对应不同的测量时间和功耗。
 * 
 * @param[in] precision 精度等级（高/中/低）
 * @return 对应的命令字节
 */
uint8_t sht40_core_precision_cmd(sht40_precision_t precision);

/**
 * @brief  获取测量命令对应的延迟时间
 * @details 根据测量命令返回所需的等待时间（毫秒）。
 *          高精度模式需要更长的测量时间。
 * 
 * @param[in] cmd 测量命令字节
 * @return 所需的延迟时间（毫秒）
 */
uint32_t sht40_core_measure_delay_ms(uint8_t cmd);

/**
 * @brief  执行同步总线传输
 * @details 执行底层的同步 I2C 读写操作。
 *          构建通信消息并调用总线接口的 xfer 函数，
 *          等待传输完成后返回结果。
 * 
 * @param[in,out] dev 指向 SHT40 设备结构体的指针
 * @param[in,out] buf 数据缓冲区指针（读操作时用于接收数据，写操作时用于发送数据）
 * @param[in] len 数据长度（字节数）
 * @param[in] read true 表示读操作，false 表示写操作
 * @return 0 表示成功，负数表示错误
 */
int sht40_core_xfer_sync(sht40_dev_t *dev, uint8_t *buf, uint16_t len, bool read);

/**
 * @brief  解析温湿度样本数据
 * @details 将 6 字节的原始数据解析为工程单位的温湿度值。
 *          前 2 字节为温度原始值，第 3 字节为温度 CRC；
 *          第 4-5 字节为湿度原始值，第 6 字节为湿度 CRC。
 *          计算公式：
 *          - 温度 (°C) = -45 + 175 * raw_temp / 65535
 *          - 湿度 (%RH) = -6 + 125 * raw_humidity / 65535
 * 
 * @param[in] rx 6 字节原始数据数组（包含温度和湿度的原始值及 CRC）
 * @param[out] out 指向输出样本结构体的指针（存储解析后的温湿度值）
 * @return 0 表示成功，负数表示错误（如 CRC 校验失败）
 */
int sht40_core_read_sample_parse(const uint8_t rx[6], sht40_sample_t *out);

#ifdef __cplusplus
}
#endif

#endif
