/**
 * @file    sc7a20_core.h
 * @brief   SC7A20加速度计驱动核心头文件
 * @details 本文件定义了SC7A20驱动的核心数据结构、类型定义和内部API。
 *          包含设备结构体、总线操作接口、通信消息格式、配置参数等核心组件。
 *          
 *          设计原则：
 *          - 基于统一的总线抽象层（sc7a20_bus_ops_t）
 *          - 支持同步和异步操作模式
 *          - 提供内存操作自定义选项（SC7A20_USE_CUSTOM_MEMOPS）
 *          - 线程安全的设备访问控制（in_use标志）
 *          
 *          核心组件：
 *          - sc7a20_comm_msg_t: 通信消息结构体，用于描述I2C/SPI事务
 *          - sc7a20_bus_ops_t: 总线操作接口，由用户实现
 *          - sc7a20_dev_t: 设备结构体，包含所有设备状态信息
 *          - sc7a20_async_ctx_t: 异步操作上下文，管理异步请求状态
 *
 * @author  WCH (南京沁恒微电子股份有限公司)
 * @version V1.0.0
 * @date    2022/06/16
 */

#ifndef SC7A20_NEW_CORE_H
#define SC7A20_NEW_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef SC7A20_USE_CUSTOM_MEMOPS
#define SC7A20_USE_CUSTOM_MEMOPS 0
#endif

#if SC7A20_USE_CUSTOM_MEMOPS
#ifndef SC7A20_MEMSET
#error "SC7A20_USE_CUSTOM_MEMOPS=1 requires SC7A20_MEMSET to be defined"
#endif
#ifndef SC7A20_MEMCPY
#error "SC7A20_USE_CUSTOM_MEMOPS=1 requires SC7A20_MEMCPY to be defined"
#endif
#else
#include <string.h>
#ifndef SC7A20_MEMSET
#define SC7A20_MEMSET memset
#endif
#ifndef SC7A20_MEMCPY
#define SC7A20_MEMCPY memcpy
#endif
#endif

#include "../sc7a20_reg.h"

#ifndef SC7A20_COMM_WRITE
#define SC7A20_COMM_WRITE (1u << 0)
#endif
#ifndef SC7A20_COMM_READ
#define SC7A20_COMM_READ  (1u << 1)
#endif
#ifndef SC7A20_COMM_STOP
#define SC7A20_COMM_STOP  (1u << 2)
#endif

/**
 * @brief  SC7A20通信消息结构体
 * @details 描述单个I2C/SPI通信事务的消息结构，用于构建复合事务。
 */
typedef struct {
    uint8_t *buf;       /**< 数据缓冲区指针 */
    uint16_t len;       /**< 数据长度 */
    uint8_t flags;      /**< 通信标志（READ/WRITE/STOP） */
} sc7a20_comm_msg_t;

/**
 * @brief  总线操作完成回调函数类型
 * @param[in] user 用户上下文指针
 * @param[in] status 操作状态（0表示成功，负数表示错误）
 */
typedef void (*sc7a20_bus_done_cb_t)(void *user, int status);

/**
 * @brief  SC7A20总线操作接口
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
                const sc7a20_comm_msg_t *msgs,
                uint8_t cnt,
                sc7a20_bus_done_cb_t cb,
                void *user);
    /**
     * @brief  取消当前传输操作
     * @param[in] ctx 总线上下文指针
     * @return 0表示成功，负数表示错误
     */
    int (*cancel)(void *ctx);
} sc7a20_bus_ops_t;

/**
 * @brief  SC7A20设备配置结构体
 * @details 包含SC7A20的所有可配置参数。
 */
typedef struct {
    sc7a20_accel_fs_t range;            /**< 量程选择 */
    sc7a20_accel_odr_t odr;             /**< 输出数据速率 */
    bool axis_x_en;                     /**< X轴使能 */
    bool axis_y_en;                     /**< Y轴使能 */
    bool axis_z_en;                     /**< Z轴使能 */
    bool block_data_update;             /**< 块数据更新模式 */
    bool high_resolution;               /**< 高分辨率模式 */
    bool low_power;                     /**< 低功耗模式 */
} sc7a20_cfg_t;

/**
 * @brief  16位整数三轴向量结构体
 * @details 用于存储原始加速度数据（X、Y、Z轴）。
 */
typedef struct {
    int16_t x;  /**< X轴数据 */
    int16_t y;  /**< Y轴数据 */
    int16_t z;  /**< Z轴数据 */
} sc7a20_vec3i16_t;

/**
 * @brief  浮点型三轴向量结构体
 * @details 用于存储工程单位加速度数据（单位：g）。
 */
typedef struct {
    float x;    /**< X轴数据（g） */
    float y;    /**< Y轴数据（g） */
    float z;    /**< Z轴数据（g） */
} sc7a20_vec3f_t;

/**
 * @brief  通用操作完成回调函数类型
 * @param[in] user 用户上下文指针
 * @param[in] status 操作状态（0表示成功，负数表示错误）
 */
typedef void (*sc7a20_done_cb_t)(void *user, int status);

/**
 * @brief  三轴数据读取完成回调函数类型
 * @param[in] user 用户上下文指针
 * @param[in] xyz 指向三轴数据的指针
 * @param[in] status 操作状态（0表示成功，负数表示错误）
 */
typedef void (*sc7a20_read_xyz_cb_t)(void *user, const sc7a20_vec3i16_t *xyz, int status);

/**
 * @brief  异步操作类型枚举
 */
typedef enum {
    SC7A20_ASYNC_OP_NONE = 0,       /**< 无操作 */
    SC7A20_ASYNC_OP_READ_REG,       /**< 寄存器读取操作 */
    SC7A20_ASYNC_OP_WRITE_REG,      /**< 寄存器写入操作 */
    SC7A20_ASYNC_OP_READ_XYZ        /**< 三轴数据读取操作 */
} sc7a20_async_op_t;

/**
 * @brief  异步操作上下文结构体
 * @details 管理当前正在进行的异步操作的状态和参数。
 */
typedef struct {
    sc7a20_async_op_t op;           /**< 当前异步操作类型 */
    uint8_t reg_addr;               /**< 寄存器地址 */
    uint8_t *read_buf;              /**< 读取数据缓冲区 */
    uint16_t read_len;              /**< 读取数据长度 */
    const uint8_t *write_buf;       /**< 写入数据缓冲区 */
    uint16_t write_len;             /**< 写入数据长度 */
    uint8_t raw_xyz[6];             /**< 原始三轴数据缓冲区 */
    sc7a20_done_cb_t done_cb;       /**< 通用完成回调 */
    sc7a20_read_xyz_cb_t xyz_cb;    /**< 三轴数据完成回调 */
    void *user;                     /**< 用户上下文 */
} sc7a20_async_ctx_t;

/**
 * @brief  SC7A20设备结构体
 * @details 包含设备的所有状态信息和配置参数。
 */
typedef struct {
    const sc7a20_bus_ops_t *ops;    /**< 总线操作接口指针 */
    void *bus_ctx;                  /**< 总线上下文指针 */
    uint8_t addr;                   /**< 设备I2C地址 */

    sc7a20_cfg_t cfg;               /**< 当前配置 */
    bool initialized;               /**< 初始化状态标志 */
    uint8_t who_am_i;               /**< 设备ID */
    uint8_t endian_ble;             /**< 字节序标志 */
    float sensitivity_g_per_lsb;    /**< 灵敏度（g/LSB） */

    volatile uint8_t in_use;        /**< 使用中标志（线程安全） */
    sc7a20_async_ctx_t async;       /**< 异步操作上下文 */
} sc7a20_dev_t;

/** @brief  SC7A20默认配置常量 */
extern const sc7a20_cfg_t g_sc7a20_default_cfg;

/**
 * @brief  尝试获取设备锁
 * @details 在多线程环境中尝试获取设备访问锁，防止并发访问冲突。
 *          使用原子操作或临界区保护 in_use 标志。
 * 
 * @param[in,out] dev 指向SC7A20设备结构体的指针
 * @return 0表示成功获取锁，-1表示设备正忙
 */
int sc7a20_core_try_lock(sc7a20_dev_t *dev);

/**
 * @brief  释放设备锁
 * @details 释放之前获取的设备访问锁，允许其他线程访问设备。
 * 
 * @param[in,out] dev 指向SC7A20设备结构体的指针
 */
void sc7a20_core_unlock(sc7a20_dev_t *dev);

/**
 * @brief  映射总线状态码
 * @details 将底层总线驱动的状态码映射为统一的错误码。
 *          确保不同总线实现返回一致的错误码。
 * 
 * @param[in] status 底层总线状态码
 * @return 统一的错误码（0表示成功，负数表示错误）
 */
int sc7a20_core_map_bus_status(int status);

/**
 * @brief  验证设备结构体有效性
 * @details 检查设备结构体的各个字段是否有效，包括：
 *          - 总线操作接口指针非空
 *          - 设备已初始化
 *          - 其他必要字段的有效性
 * 
 * @param[in] dev 指向SC7A20设备结构体的指针
 * @return 0表示有效，负数表示无效
 */
int sc7a20_core_validate_dev(const sc7a20_dev_t *dev);

/**
 * @brief  核心寄存器读取函数
 * @details 执行底层寄存器读取操作，不包含参数验证。
 *          构建通信消息并调用总线传输接口。
 * 
 * @param[in,out] dev 指向SC7A20设备结构体的指针
 * @param[in] reg 寄存器地址
 * @param[out] data 数据缓冲区指针
 * @param[in] len 数据长度
 * @return 0表示成功，负数表示错误
 */
int sc7a20_core_read_reg(sc7a20_dev_t *dev, uint8_t reg, uint8_t *data, uint16_t len);

/**
 * @brief  核心寄存器写入函数
 * @details 执行底层寄存器写入操作，不包含参数验证。
 *          构建通信消息并调用总线传输接口。
 * 
 * @param[in,out] dev 指向SC7A20设备结构体的指针
 * @param[in] reg 寄存器地址
 * @param[in] data 数据缓冲区指针
 * @param[in] len 数据长度
 * @return 0表示成功，负数表示错误
 */
int sc7a20_core_write_reg(sc7a20_dev_t *dev, uint8_t reg, const uint8_t *data, uint16_t len);

/**
 * @brief  核心寄存器更新函数
 * @details 执行寄存器位域更新操作（读 - 修改 - 写）。
 *          先读取寄存器当前值，按掩码修改指定位，再写回寄存器。
 * 
 * @param[in,out] dev 指向SC7A20设备结构体的指针
 * @param[in] reg 寄存器地址
 * @param[in] mask 位掩码，指定要修改的位
 * @param[in] value 要设置的值（应与掩码对齐）
 * @return 0表示成功，负数表示错误
 */
int sc7a20_core_update_reg(sc7a20_dev_t *dev, uint8_t reg, uint8_t mask, uint8_t value);

/**
 * @brief  解码三轴原始数据
 * @details 将6字节的原始数据解码为三轴16位整数格式。
 *          根据设备的字节序标志（endian_ble）进行正确的字节排列。
 * 
 * @param[in] dev 指向SC7A20设备结构体的指针
 * @param[in] raw 6字节原始数据数组（X低、X高、Y低、Y高、Z低、Z高）
 * @param[out] out 指向输出三轴数据结构体的指针
 * @return 0表示成功，负数表示错误
 */
int sc7a20_core_decode_xyz(const sc7a20_dev_t *dev, const uint8_t raw[6], sc7a20_vec3i16_t *out);

/**
 * @brief  应用设备配置
 * @details 将配置结构体中的参数应用到硬件寄存器中。
 *          配置包括量程、输出数据速率、轴使能、工作模式等。
 * 
 * @param[in,out] dev 指向SC7A20设备结构体的指针
 * @param[in] cfg 指向配置结构体的指针
 * @return 0表示成功，负数表示错误
 */
int sc7a20_core_apply_config(sc7a20_dev_t *dev, const sc7a20_cfg_t *cfg);

#ifdef __cplusplus
}
#endif

#endif
