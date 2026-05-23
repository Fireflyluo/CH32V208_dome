/**
 * @file drv_i2c.h
 * @brief CH32 I2C 驱动接口（支持轮询/中断/DMA）
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

/* 恢复策略 */
#define MODULE_SELF_RESET     0
#define MODULE_RCC_RESET      1
#define COMM_RECOVER_MODE     MODULE_SELF_RESET

/* 默认工作模式 */
#define I2C1_MODE             I2C_MODE_DMA
#define I2C2_MODE             I2C_MODE_IT

/* 传输模式 */
#define I2C_MODE_POLLING      0
#define I2C_MODE_IT           1
#define I2C_MODE_DMA          2

/* 缓冲与超时 */
#define I2C_MAX_WRITE_LEN     256
#define I2C_MAX_RX_BUFFER     256
#define I2C_TIMEOUT_DEFAULT   0xFFFF

/* I2C 外设编号 */
typedef enum
{
    I2C_NUM_1 = 0,
    I2C_NUM_2 = 1,
    I2C_NUM_MAX
} i2c_num_t;

/* I2C 运行状态 */
typedef enum {
    I2C_STATE_IDLE        = 0x00,
    I2C_STATE_BUSY        = 0x01,
    I2C_STATE_BUSY_TX     = 0x02,
    I2C_STATE_BUSY_RX     = 0x03,
    I2C_STATE_ERROR       = 0x80
} I2C_StateTypeDef;

/* 错误码 */
typedef enum {
    I2C_OK                = 0,
    I2C_ERR_TIMEOUT       = 0x01,
    I2C_ERR_BUSY          = 0x02,
    I2C_ERR_AF            = 0x04,
    I2C_ERR_BERR          = 0x08,
    I2C_ERR_ARLO          = 0x10,
    I2C_ERR_DMA           = 0x20,
    I2C_ERR_INVALID_PARAM = 0x80
} I2C_ErrorCode_t;

/* 回调函数类型 */
typedef void (*bsp_i2c_callback_t)(i2c_num_t i2c_num);
typedef void (*bsp_i2c_error_callback_t)(i2c_num_t i2c_num, uint32_t error_code);

/* 初始化参数 */
typedef struct {
    uint32_t clock_speed;
    uint16_t duty_cycle;
    uint16_t own_address;
    bool enable_ack;
    bool is_7_bit_address;
    uint8_t mode;
} bsp_i2c_config_t;

/* 运行句柄 */
typedef struct {
    I2C_TypeDef *Instance;
    I2C_StateTypeDef State;
    uint8_t *pTxBuffer;
    uint8_t *pRxBuffer;
    uint16_t TxLength;
    uint16_t RxLength;
    uint16_t TxCount;
    uint16_t RxCount;
    uint8_t DevAddr;
    bool IsRegWrite;
    uint8_t RegAddr;
    uint8_t Mode;
    bsp_i2c_config_t InitCfg;
    uint32_t ErrorCode;

    DMA_Channel_TypeDef *TxDmaChannel;
    DMA_Channel_TypeDef *RxDmaChannel;
    bool DmaEnabled;
    bool XferUseDma;

    bsp_i2c_callback_t TxCpltCallback;
    bsp_i2c_callback_t RxCpltCallback;
    bsp_i2c_error_callback_t ErrorCallback;
} I2C_HandleTypeDef;

extern I2C_HandleTypeDef I2C1_Handle;
extern I2C_HandleTypeDef I2C2_Handle;

/* 初始化与状态 */
int bsp_i2c_init(i2c_num_t i2c_num, const bsp_i2c_config_t *init_cfg);
void bsp_i2c_deinit(i2c_num_t i2c_num);
I2C_StateTypeDef bsp_i2c_get_state(i2c_num_t i2c_num);
bool bsp_i2c_is_busy(i2c_num_t i2c_num);
uint32_t bsp_i2c_get_error(i2c_num_t i2c_num);
void bsp_i2c_recover(i2c_num_t i2c_num);

/* 轮询模式 */
int bsp_i2c_write_polling(i2c_num_t i2c_num, uint8_t dev_addr, const uint8_t *data, uint16_t len, uint32_t timeout);
int bsp_i2c_read_polling(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t *data, uint16_t len, uint32_t timeout);
int bsp_i2c_write_register_polling(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t len, uint32_t timeout);
int bsp_i2c_read_register_polling(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len, uint32_t timeout);

/* 中断模式 */
int bsp_i2c_write_it(i2c_num_t i2c_num, uint8_t dev_addr, const uint8_t *data, uint16_t len);
int bsp_i2c_read_it(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t *data, uint16_t len);
int bsp_i2c_write_register_it(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t len);
int bsp_i2c_read_register_it(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len);

/* DMA 模式 */
int bsp_i2c_write_dma(i2c_num_t i2c_num, uint8_t dev_addr, const uint8_t *data, uint16_t len);
int bsp_i2c_read_dma(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t *data, uint16_t len);
int bsp_i2c_write_register_dma(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t len);
int bsp_i2c_read_register_dma(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len);

/* 自动按当前模式分发 */
int bsp_i2c_write(i2c_num_t i2c_num, uint8_t dev_addr, const uint8_t *data, uint16_t len);
int bsp_i2c_read(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t *data, uint16_t len);
int bsp_i2c_write_register(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t len);
int bsp_i2c_read_register(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len);
int bsp_i2c_write_byte(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, uint8_t data);
int bsp_i2c_read_byte(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, uint8_t *data);

/* DMA 与设备探测 */
void bsp_i2c_dma_init(i2c_num_t i2c_num, DMA_Channel_TypeDef *tx_channel, DMA_Channel_TypeDef *rx_channel);
int bsp_i2c_check_device(i2c_num_t i2c_num, uint8_t dev_addr, uint32_t timeout);
uint8_t bsp_i2c_scan(i2c_num_t i2c_num, uint8_t *found_addr, uint8_t max_count);

/* 回调注册 */
void bsp_i2c_register_tx_callback(i2c_num_t i2c_num, bsp_i2c_callback_t callback);
void bsp_i2c_register_rx_callback(i2c_num_t i2c_num, bsp_i2c_callback_t callback);
void bsp_i2c_register_error_callback(i2c_num_t i2c_num, bsp_i2c_error_callback_t callback);

/* 中断入口 */
void bsp_i2c_irq_handler(i2c_num_t i2c_num);
void bsp_i2c_dma_tx_irq_handler(i2c_num_t i2c_num);
void bsp_i2c_dma_rx_irq_handler(i2c_num_t i2c_num);

/* 弱符号回调（用户可重写） */
__attribute__((weak)) void bsp_i2c1_tx_cplt_callback(void);
__attribute__((weak)) void bsp_i2c1_rx_cplt_callback(void);
__attribute__((weak)) void bsp_i2c1_error_callback(uint32_t error_code);
__attribute__((weak)) void bsp_i2c2_tx_cplt_callback(void);
__attribute__((weak)) void bsp_i2c2_rx_cplt_callback(void);
__attribute__((weak)) void bsp_i2c2_error_callback(uint32_t error_code);

#ifdef __cplusplus
}
#endif

#endif
