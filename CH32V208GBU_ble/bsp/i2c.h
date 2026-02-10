/**
 ******************************************************************************
 * @file    I2C.c
 * @brief
 ******************************************************************************
 * @note    本文件内定义I2C相关的初始化代码
 *
 ******************************************************************************
 */
#ifndef __I2C_H
#define __I2C_H

#include "debug.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "ch32v20x.h"
#include <stdint.h>
#include <stdbool.h>

#define MODULE_SELF_RESET 0 // 模块自复位
#define MODULE_RCC_RESET  1 // 模块时钟复位

// 系统复位方式
#define COMM_RECOVER_MODE MODULE_SELF_RESET
/* ========================== 类型定义 ========================== */

// I2C编号枚举
typedef enum {
    I2C_NUM_1 = 0,
    I2C_NUM_2 = 1
} i2c_num_t;

typedef enum {
    C_READY = 0, // 就绪状态
    C_START_BIT, // 起始位状态
    C_STOP_BIT   // 停止位状态
} CommCtrl_t;

/**
 * @brief 错误代码枚举类型
 * 定义I2C主从设备可能出现的错误类型
 */
typedef enum {
    MASTER_OK = 0,  // 主设备正常
    MASTER_BUSY,    // 主设备忙
    MASTER_MODE,    // 主模式错误
    MASTER_TXMODE,  // 主发送模式错误
    MASTER_RXMODE,  // 主接收模式错误
    MASTER_SENDING, // 主发送中错误
    MASTER_SENDED,  // 主发送完成错误
    MASTER_RECVD,   // 主接收错误
    MASTER_BYTEF,   // 主字节完成错误
    MASTER_BUSERR,  // 主总线错误
    MASTER_UNKNOW,  // 主未知错误
    SLAVE_OK = 20,  // 从设备正常
    SLAVE_BUSY,     // 从设备忙
    SLAVE_MODE,     // 从模式错误
    SLAVE_BUSERR,   // 从总线错误
    SLAVE_UNKNOW,   // 从未知错误

} i2c_ErrCode_t;

// I2C配置结构
typedef struct {
    uint32_t clock_speed;  // 时钟频率(Hz)
    uint16_t duty_cycle;   // 占空比
    uint16_t own_address;  // 自身地址
    bool enable_ack;       // 使能ACK
    bool is_7_bit_address; // 7位地址模式
} i2c_config_t;

// 默认配置
#define I2C_DEFAULT_CONFIG {                 \
    .clock_speed      = 100000, /* 100kHz */ \
    .duty_cycle       = I2C_DutyCycle_16_9,  \
    .own_address      = 0,                   \
    .enable_ack       = true,                \
    .is_7_bit_address = true}

/* ========================== 函数声明 ========================== */

// 初始化函数
int i2c_init(i2c_num_t i2c_num, const i2c_config_t *config);
void i2c_deinit(i2c_num_t i2c_num);

// 基础读写函数
int i2c_write(i2c_num_t i2c_num, uint8_t dev_addr, const uint8_t *data, uint16_t len);
int i2c_read(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t *data, uint16_t len);

// 寄存器读写函数
int i2c_write_register(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t len);
int i2c_read_register(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len);

// 单字节读写简化函数
int i2c_write_byte(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, uint8_t data);
int i2c_read_byte(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, uint8_t *data);

// 状态检查函数
bool i2c_is_busy(i2c_num_t i2c_num);
int i2c_check_device(i2c_num_t i2c_num, uint8_t dev_addr);

#ifdef __cplusplus
}
#endif

#endif /* __I2C_H */