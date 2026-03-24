/**
 ******************************************************************************
 * @file    drv_i2c.h
 * @brief   I2C 驱动程序头文件 - HAL 库风格，支持轮询、中断和 DMA 模式
 ******************************************************************************
 * @note    本文件内定义 I2C 相关的初始化代码和 API
 *          参考 STM32 HAL 库设计状态机和异步传输机制
 ******************************************************************************
 */
#ifndef __DRV_I2C_H
#define __DRV_I2C_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "ch32v20x.h"
#include "debug.h"
#include <stdbool.h>
#include <stdint.h>

/* ========================== 配置宏定义 ========================== */

// 通信恢复模式
#define MODULE_SELF_RESET     0  // 模块自复位
#define MODULE_RCC_RESET      1  // 模块时钟复位
#define COMM_RECOVER_MODE     MODULE_SELF_RESET

// I2C 默认模式配置
#define I2C1_MODE             I2C_MODE_DMA  // I2C1 使用 DMA 模式
#define I2C2_MODE             I2C_MODE_IT   // I2C2 使用中断模式

// 操作模式
#define I2C_MODE_POLLING      0  // 轮询模式
#define I2C_MODE_IT           1  // 中断模式
#define I2C_MODE_DMA          2  // DMA 模式

// 最大传输长度
#define I2C_MAX_WRITE_LEN     256
#define I2C_MAX_RX_BUFFER     256

// 超时定义
#define I2C_TIMEOUT_DEFAULT   0xFFFF

/* ========================== 类型定义 ========================== */

// I2C 编号枚举
typedef enum
{
    I2C_NUM_1 = 0,
    I2C_NUM_2 = 1,
    I2C_NUM_MAX
} i2c_num_t;

// I2C 状态枚举（参考 HAL 库设计）
typedef enum {
    I2C_STATE_IDLE        = 0x00,  ///< 空闲状态
    I2C_STATE_BUSY        = 0x01,  ///< 忙状态
    I2C_STATE_BUSY_TX     = 0x02,  ///< 忙 - 发送中
    I2C_STATE_BUSY_RX     = 0x03,  ///< 忙 - 接收中
    I2C_STATE_ERROR       = 0x80   ///< 错误状态
} I2C_StateTypeDef;

// I2C 错误码枚举
typedef enum {
    I2C_OK                = 0,       ///< 成功
    I2C_ERR_TIMEOUT       = 0x01,    ///< 超时错误
    I2C_ERR_BUSY          = 0x02,    ///< 总线忙
    I2C_ERR_AF            = 0x04,    ///< ACK 失败
    I2C_ERR_BERR          = 0x08,    ///< 总线错误
    I2C_ERR_ARLO          = 0x10,    ///< 仲裁丢失
    I2C_ERR_DMA           = 0x20,    ///< DMA 错误
    I2C_ERR_INVALID_PARAM = 0x80     ///< 无效参数
} I2C_ErrorCode_t;

// 回调函数类型
typedef void (*bsp_i2c_callback_t)(i2c_num_t i2c_num);
typedef void (*bsp_i2c_error_callback_t)(i2c_num_t i2c_num, uint32_t error_code);

// I2C 配置结构
typedef struct {
    uint32_t clock_speed;       ///< 时钟频率 (Hz)，最大 400kHz
    uint16_t duty_cycle;        ///< 占空比
    uint16_t own_address;       ///< 自身地址
    bool enable_ack;            ///< 使能 ACK
    bool is_7_bit_address;      ///< 7 位地址模式
    uint8_t mode;               ///< 模式: I2C_MODE_POLLING/IT/DMA
} bsp_i2c_config_t;

// I2C 句柄结构（类似 HAL 库的 HandleTypeDef）
typedef struct {
    I2C_TypeDef *Instance;                  ///< I2C 外设实例
    I2C_StateTypeDef State;                 ///< 当前状态
    uint8_t *pTxBuffer;                     ///< 发送缓冲区指针
    uint8_t *pRxBuffer;                     ///< 接收缓冲区指针
    uint16_t TxLength;                      ///< 待发送长度
    uint16_t RxLength;                      ///< 待接收长度
    uint16_t TxCount;                       ///< 已发送计数
    uint16_t RxCount;                       ///< 已接收计数
    uint8_t DevAddr;                        ///< 设备地址
    bool IsRegWrite;                        ///< 是否寄存器操作
    uint8_t RegAddr;                        ///< 寄存器地址
    uint8_t Mode;                           ///< 当前操作模式
    uint32_t ErrorCode;                     ///< 错误码
    
    // DMA 相关
    DMA_Channel_TypeDef *TxDmaChannel;      ///< 发送 DMA 通道
    DMA_Channel_TypeDef *RxDmaChannel;      ///< 接收 DMA 通道
    bool DmaEnabled;                        ///< DMA 是否使能
    
    // 回调函数
    bsp_i2c_callback_t TxCpltCallback;      ///< 发送完成回调
    bsp_i2c_callback_t RxCpltCallback;      ///< 接收完成回调
    bsp_i2c_error_callback_t ErrorCallback; ///< 错误回调
} I2C_HandleTypeDef;

// 全局句柄
extern I2C_HandleTypeDef I2C1_Handle;
extern I2C_HandleTypeDef I2C2_Handle;

/* ========================== 函数声明 ========================== */

/* ===== 初始化和反初始化 ===== */
int bsp_i2c_init(i2c_num_t i2c_num, const bsp_i2c_config_t *init_cfg);
void bsp_i2c_deinit(i2c_num_t i2c_num);

/* ===== 状态查询 ===== */
I2C_StateTypeDef bsp_i2c_get_state(i2c_num_t i2c_num);
bool bsp_i2c_is_busy(i2c_num_t i2c_num);
uint32_t bsp_i2c_get_error(i2c_num_t i2c_num);
void bsp_i2c_recover(i2c_num_t i2c_num);

/* ===== 轮询模式 - 同步读写 ===== */
int bsp_i2c_write_polling(i2c_num_t i2c_num, uint8_t dev_addr, const uint8_t *data, uint16_t len, uint32_t timeout);
int bsp_i2c_read_polling(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t *data, uint16_t len, uint32_t timeout);
int bsp_i2c_write_register_polling(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t len, uint32_t timeout);
int bsp_i2c_read_register_polling(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len, uint32_t timeout);

/* ===== 中断模式 - 异步读写 ===== */
int bsp_i2c_write_it(i2c_num_t i2c_num, uint8_t dev_addr, const uint8_t *data, uint16_t len);
int bsp_i2c_read_it(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t *data, uint16_t len);
int bsp_i2c_write_register_it(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t len);
int bsp_i2c_read_register_it(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len);

/* ===== DMA 模式 - 异步启动 ===== */
int bsp_i2c_write_dma(i2c_num_t i2c_num, uint8_t dev_addr, const uint8_t *data, uint16_t len);
int bsp_i2c_read_dma(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t *data, uint16_t len);
int bsp_i2c_write_register_dma(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t len);
int bsp_i2c_read_register_dma(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len);

/* ===== 简化接口（根据配置自动选择模式） ===== */
int bsp_i2c_write(i2c_num_t i2c_num, uint8_t dev_addr, const uint8_t *data, uint16_t len);
int bsp_i2c_read(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t *data, uint16_t len);
int bsp_i2c_write_register(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t len);
int bsp_i2c_read_register(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len);
int bsp_i2c_write_byte(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, uint8_t data);
int bsp_i2c_read_byte(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, uint8_t *data);

/* ===== DMA 配置 ===== */
void bsp_i2c_dma_init(i2c_num_t i2c_num, DMA_Channel_TypeDef *tx_channel, DMA_Channel_TypeDef *rx_channel);

/* ===== 设备探测 ===== */
int bsp_i2c_check_device(i2c_num_t i2c_num, uint8_t dev_addr, uint32_t timeout);
uint8_t bsp_i2c_scan(i2c_num_t i2c_num, uint8_t *found_addr, uint8_t max_count);

/* ===== 回调函数注册 ===== */
void bsp_i2c_register_tx_callback(i2c_num_t i2c_num, bsp_i2c_callback_t callback);
void bsp_i2c_register_rx_callback(i2c_num_t i2c_num, bsp_i2c_callback_t callback);
void bsp_i2c_register_error_callback(i2c_num_t i2c_num, bsp_i2c_error_callback_t callback);

/* ===== 中断处理（需在应用中实现） ===== */
void bsp_i2c_irq_handler(i2c_num_t i2c_num);
void bsp_i2c_dma_tx_irq_handler(i2c_num_t i2c_num);
void bsp_i2c_dma_rx_irq_handler(i2c_num_t i2c_num);

/* ===== 弱定义默认回调（用户可重写） ===== */
__attribute__((weak)) void bsp_i2c1_tx_cplt_callback(void);
__attribute__((weak)) void bsp_i2c1_rx_cplt_callback(void);
__attribute__((weak)) void bsp_i2c1_error_callback(uint32_t error_code);
__attribute__((weak)) void bsp_i2c2_tx_cplt_callback(void);
__attribute__((weak)) void bsp_i2c2_rx_cplt_callback(void);
__attribute__((weak)) void bsp_i2c2_error_callback(uint32_t error_code);

#ifdef __cplusplus
}
#endif

#endif /* __DRV_I2C_H */
