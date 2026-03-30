/**
 ******************************************************************************
 * @file    drv_i2c.c
 * @brief   I2C 驱动程序 - HAL 库风格，支持轮询、中断和 DMA 模式
 ******************************************************************************
 * @note    本文件内实现 I2C 相关的驱动代码
 *          支持 CH32V208 的 I2C1 和 I2C2
 ******************************************************************************
 */

#include "drv_i2c.h"
#include "ch32v20x_dma.h"
#include "ch32v20x_rcc.h"
#include <string.h>

/* ========================== 全局句柄定义 ========================== */

I2C_HandleTypeDef I2C1_Handle = {0};
I2C_HandleTypeDef I2C2_Handle = {0};

/* ========================== 内部常量定义 ========================== */

// 硬件配置结构体
typedef struct
{
    uint32_t scl_pin;
    uint32_t sda_pin;
    GPIO_TypeDef *gpio_port;
    uint32_t rcc_gpio_clk;
    uint32_t rcc_i2c_clk;
    uint8_t remap_config;
    uint8_t mode;
} i2c_hw_config_t;

// 硬件配置表
static const i2c_hw_config_t i2c_hw_config[] = {
    [I2C_NUM_1] = {
        .scl_pin = GPIO_Pin_6,
        .sda_pin = GPIO_Pin_7,
        .gpio_port = GPIOB,
        .rcc_gpio_clk = RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO,
        .rcc_i2c_clk = RCC_APB1Periph_I2C1,
        .remap_config = 0,
        .mode = I2C1_MODE},
    [I2C_NUM_2] = {.scl_pin = GPIO_Pin_10, .sda_pin = GPIO_Pin_11, .gpio_port = GPIOB, .rcc_gpio_clk = RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, .rcc_i2c_clk = RCC_APB1Periph_I2C2, .remap_config = 0, .mode = I2C2_MODE}};

// 获取句柄指针
static inline I2C_HandleTypeDef *get_handle(i2c_num_t i2c_num)
{
    return (i2c_num == I2C_NUM_1) ? &I2C1_Handle : &I2C2_Handle;
}

// 获取外设指针
static inline I2C_TypeDef *get_periph(i2c_num_t i2c_num)
{
    return (i2c_num == I2C_NUM_1) ? I2C1 : I2C2;
}

static void i2c_recover_bus(i2c_num_t i2c_num);
static void i2c_reinit_from_handle(i2c_num_t i2c_num);

/* ========================== 内部辅助函数 ========================== */

/**
 * @brief 等待标志位（带超时）
 */
static int wait_flag(I2C_TypeDef *i2c, uint32_t flag, FlagStatus status, uint32_t timeout)
{
    while (I2C_GetFlagStatus(i2c, flag) != status)
    {
        if (timeout-- == 0)
            return I2C_ERR_TIMEOUT;
    }
    return I2C_OK;
}

/**
 * @brief 等待事件（带超时）
 */
static int wait_event(I2C_TypeDef *i2c, uint32_t event, uint32_t timeout)
{
    while (!I2C_CheckEvent(i2c, event))
    {
        if (timeout-- == 0)
            return I2C_ERR_TIMEOUT;
    }
    return I2C_OK;
}

/**
 * @brief 错误处理
 */
static void i2c_error_handler(I2C_HandleTypeDef *hi2c, uint32_t error_code)
{
    i2c_num_t num = (hi2c->Instance == I2C1) ? I2C_NUM_1 : I2C_NUM_2;
    hi2c->ErrorCode = error_code;

    // 错误时先停 DMA，避免 DMA/I2C 状态机互相拖挂
    if (hi2c->TxDmaChannel != NULL)
    {
        DMA_Cmd(hi2c->TxDmaChannel, DISABLE);
    }
    if (hi2c->RxDmaChannel != NULL)
    {
        DMA_Cmd(hi2c->RxDmaChannel, DISABLE);
    }
    I2C_DMALastTransferCmd(hi2c->Instance, DISABLE);
    I2C_DMACmd(hi2c->Instance, DISABLE);

    // 生成 STOP 条件
    I2C_GenerateSTOP(hi2c->Instance, ENABLE);
    I2C_AcknowledgeConfig(hi2c->Instance, ENABLE);
    I2C_ITConfig(hi2c->Instance, I2C_IT_BUF, DISABLE);
    hi2c->State = I2C_STATE_IDLE;
    hi2c->XferUseDma = false;

    // BERR/ARLO/TIMEOUT 才做重型总线恢复；AF(NACK) 常见于设备忙/地址无应答，不直接做重置
    if (error_code == I2C_ERR_BERR || error_code == I2C_ERR_ARLO || error_code == I2C_ERR_TIMEOUT)
    {
        i2c_recover_bus(num);
    }

    // 调用错误回调
    if (hi2c->ErrorCallback)
    {
        hi2c->ErrorCallback(num, error_code);
    }
}

static void i2c_reinit_from_handle(i2c_num_t i2c_num)
{
    I2C_HandleTypeDef *hi2c = get_handle(i2c_num);
    I2C_TypeDef *i2c = get_periph(i2c_num);
    I2C_InitTypeDef i2c_init = {0};

    I2C_StructInit(&i2c_init);
    i2c_init.I2C_ClockSpeed = hi2c->InitCfg.clock_speed;
    i2c_init.I2C_DutyCycle = hi2c->InitCfg.duty_cycle;
    i2c_init.I2C_OwnAddress1 = hi2c->InitCfg.own_address;
    i2c_init.I2C_Ack = hi2c->InitCfg.enable_ack ? I2C_Ack_Enable : I2C_Ack_Disable;
    i2c_init.I2C_AcknowledgedAddress = hi2c->InitCfg.is_7_bit_address ? I2C_AcknowledgedAddress_7bit : I2C_AcknowledgedAddress_10bit;

    I2C_DeInit(i2c);
    I2C_Init(i2c, &i2c_init);
    I2C_Cmd(i2c, ENABLE);
    I2C_AcknowledgeConfig(i2c, ENABLE);

    if (hi2c->Mode == I2C_MODE_IT || hi2c->Mode == I2C_MODE_DMA)
    {
        I2C_ITConfig(i2c, I2C_IT_EVT | I2C_IT_ERR, ENABLE);
        I2C_ITConfig(i2c, I2C_IT_BUF, DISABLE);
    }

    if (hi2c->Mode == I2C_MODE_DMA)
    {
        I2C_DMACmd(i2c, ENABLE);
    }
}

/**
 * @brief 软件复位 I2C
 */
static void i2c_sw_reset(i2c_num_t i2c_num)
{
    I2C_TypeDef *i2c = get_periph(i2c_num);
    const i2c_hw_config_t *hw = &i2c_hw_config[i2c_num];

    // 1. 禁用 I2C 和中断
    I2C_ITConfig(i2c, I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR, DISABLE);
    I2C_Cmd(i2c, DISABLE);

    // 2. 将引脚配置为 GPIO 输出
    GPIO_InitTypeDef gpio_init = {0};
    RCC_APB2PeriphClockCmd(hw->rcc_gpio_clk, ENABLE);
    gpio_init.GPIO_Pin = hw->scl_pin | hw->sda_pin;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_Init(hw->gpio_port, &gpio_init);

    // 3. 生成 9 个时钟脉冲
    for (int i = 0; i < 9; i++)
    {
        GPIO_ResetBits(hw->gpio_port, hw->scl_pin);
        Delay_Us(5);
        GPIO_SetBits(hw->gpio_port, hw->scl_pin);
        Delay_Us(5);
    }

    // 4. 生成 STOP 条件
    GPIO_ResetBits(hw->gpio_port, hw->sda_pin);
    Delay_Us(5);
    GPIO_SetBits(hw->gpio_port, hw->scl_pin);
    Delay_Us(5);
    GPIO_SetBits(hw->gpio_port, hw->sda_pin);
    Delay_Us(10);

    // 5. 恢复 I2C 功能
    gpio_init.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(hw->gpio_port, &gpio_init);

    i2c_reinit_from_handle(i2c_num);
}

static void i2c_recover_bus(i2c_num_t i2c_num)
{
#if (COMM_RECOVER_MODE == MODULE_SELF_RESET)
    i2c_sw_reset(i2c_num);
#elif (COMM_RECOVER_MODE == MODULE_RCC_RESET)
    if (i2c_num == I2C_NUM_1)
    {
        RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, ENABLE);
        RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, DISABLE);
    }
    else
    {
        RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C2, ENABLE);
        RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C2, DISABLE);
    }
    i2c_reinit_from_handle(i2c_num);
#endif
}

/* ========================== 初始化函数 ========================== */

int bsp_i2c_init(i2c_num_t i2c_num, const bsp_i2c_config_t *init_cfg)
{
    if (i2c_num >= I2C_NUM_MAX || init_cfg == NULL)
    {
        return I2C_ERR_INVALID_PARAM;
    }

    I2C_HandleTypeDef *hi2c = get_handle(i2c_num);
    const i2c_hw_config_t *hw = &i2c_hw_config[i2c_num];
    I2C_TypeDef *i2c = get_periph(i2c_num);

    // 保存配置
    hi2c->Instance = i2c;
    hi2c->InitCfg = *init_cfg;
    hi2c->Mode = init_cfg->mode;
    hi2c->State = I2C_STATE_IDLE;
    hi2c->XferUseDma = false;
    hi2c->ErrorCode = I2C_OK;

    // 使能时钟
    RCC_APB2PeriphClockCmd(hw->rcc_gpio_clk, ENABLE);
    RCC_APB1PeriphClockCmd(hw->rcc_i2c_clk, ENABLE);

    // 配置 GPIO
    GPIO_InitTypeDef gpio_init = {0};
    gpio_init.GPIO_Pin = hw->scl_pin | hw->sda_pin;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(hw->gpio_port, &gpio_init);

    // 重映射配置（如果需要）
    if (i2c_num == I2C_NUM_1 && hw->remap_config != 0)
    {
        GPIO_PinRemapConfig(hw->remap_config, ENABLE);
    }

    // 配置 I2C
    I2C_InitTypeDef i2c_init = {0};
    I2C_StructInit(&i2c_init);
    i2c_init.I2C_ClockSpeed = init_cfg->clock_speed;
    i2c_init.I2C_DutyCycle = init_cfg->duty_cycle;
    i2c_init.I2C_OwnAddress1 = init_cfg->own_address;
    i2c_init.I2C_Ack = init_cfg->enable_ack ? I2C_Ack_Enable : I2C_Ack_Disable;
    i2c_init.I2C_AcknowledgedAddress = init_cfg->is_7_bit_address ? I2C_AcknowledgedAddress_7bit : I2C_AcknowledgedAddress_10bit;

    I2C_Init(i2c, &i2c_init);
    I2C_Cmd(i2c, ENABLE);

    // 配置中断（如果使能中断模式）
    if (init_cfg->mode == I2C_MODE_IT || init_cfg->mode == I2C_MODE_DMA)
    {
        NVIC_InitTypeDef nvic_init = {0};

        if (i2c_num == I2C_NUM_1)
        {
            nvic_init.NVIC_IRQChannel = I2C1_EV_IRQn;
            nvic_init.NVIC_IRQChannelPreemptionPriority = 1;
            nvic_init.NVIC_IRQChannelSubPriority = 0;
            nvic_init.NVIC_IRQChannelCmd = ENABLE;
            NVIC_Init(&nvic_init);

            nvic_init.NVIC_IRQChannel = I2C1_ER_IRQn;
            NVIC_Init(&nvic_init);
        }
        else
        {
            nvic_init.NVIC_IRQChannel = I2C2_EV_IRQn;
            nvic_init.NVIC_IRQChannelPreemptionPriority = 1;
            nvic_init.NVIC_IRQChannelSubPriority = 1;
            nvic_init.NVIC_IRQChannelCmd = ENABLE;
            NVIC_Init(&nvic_init);

            nvic_init.NVIC_IRQChannel = I2C2_ER_IRQn;
            NVIC_Init(&nvic_init);
        }

        // 默认只开 EVT/ERR，BUF 在真正发起传输时再打开，避免空闲态被 TXE 打断
        I2C_ITConfig(i2c, I2C_IT_EVT | I2C_IT_ERR, ENABLE);
        I2C_ITConfig(i2c, I2C_IT_BUF, DISABLE);
    }

    // 配置 DMA（如果使能 DMA 模式）
    if (init_cfg->mode == I2C_MODE_DMA)
    {
        I2C_DMACmd(i2c, ENABLE);
    }

    return I2C_OK;
}

void bsp_i2c_deinit(i2c_num_t i2c_num)
{
    if (i2c_num >= I2C_NUM_MAX)
        return;

    I2C_HandleTypeDef *hi2c = get_handle(i2c_num);
    I2C_TypeDef *i2c = get_periph(i2c_num);

    // 禁用 I2C
    I2C_Cmd(i2c, DISABLE);

    // 禁用中断
    I2C_ITConfig(i2c, I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR, DISABLE);

    // 复位 I2C
    if (i2c_num == I2C_NUM_1)
    {
        RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, ENABLE);
        RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, DISABLE);
    }
    else
    {
        RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C2, ENABLE);
        RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C2, DISABLE);
    }

    // 清空句柄
    memset(hi2c, 0, sizeof(I2C_HandleTypeDef));
}

/* ========================== 状态查询函数 ========================== */

I2C_StateTypeDef bsp_i2c_get_state(i2c_num_t i2c_num)
{
    if (i2c_num >= I2C_NUM_MAX)
        return I2C_STATE_ERROR;

    I2C_HandleTypeDef *hi2c = get_handle(i2c_num);
    // 兜底：在轮询状态查询时顺带服务 DMA 完成，避免仅依赖 NVIC IRQ。
    // 注意这里不能仅看 hi2c->Mode，因为上层可能采用“全局IT + 单次请求DMA”的混合模式。
    if (hi2c->DmaEnabled && hi2c->XferUseDma)
    {
        if (hi2c->State == I2C_STATE_BUSY_TX)
        {
            bsp_i2c_dma_tx_irq_handler(i2c_num);
        }
        else if (hi2c->State == I2C_STATE_BUSY_RX)
        {
            bsp_i2c_dma_rx_irq_handler(i2c_num);
        }
    }

    return hi2c->State;
}

bool bsp_i2c_is_busy(i2c_num_t i2c_num)
{
    if (i2c_num >= I2C_NUM_MAX)
        return true;
    I2C_TypeDef *i2c = get_periph(i2c_num);
    return I2C_GetFlagStatus(i2c, I2C_FLAG_BUSY) == SET;
}

uint32_t bsp_i2c_get_error(i2c_num_t i2c_num)
{
    if (i2c_num >= I2C_NUM_MAX)
        return I2C_ERR_INVALID_PARAM;
    return get_handle(i2c_num)->ErrorCode;
}

void bsp_i2c_recover(i2c_num_t i2c_num)
{
    if (i2c_num >= I2C_NUM_MAX)
    {
        return;
    }

    I2C_HandleTypeDef *hi2c = get_handle(i2c_num);
    I2C_TypeDef *i2c = get_periph(i2c_num);

    i2c_recover_bus(i2c_num);
    I2C_GenerateSTOP(i2c, ENABLE);
    I2C_AcknowledgeConfig(i2c, ENABLE);
    hi2c->State = I2C_STATE_IDLE;
    hi2c->ErrorCode = I2C_ERR_TIMEOUT;
}

/* ========================== 轮询模式函数 ========================== */

int bsp_i2c_write_polling(i2c_num_t i2c_num, uint8_t dev_addr,
                          const uint8_t *data, uint16_t len, uint32_t timeout)
{
    if (i2c_num >= I2C_NUM_MAX || data == NULL || len == 0)
    {
        return I2C_ERR_INVALID_PARAM;
    }

    I2C_HandleTypeDef *hi2c = get_handle(i2c_num);
    I2C_TypeDef *i2c = get_periph(i2c_num);
    int ret;
    bool restore_evt_irq = false;

    // 当当前工作模式为 IT/DMA 时，临时关闭 EVT/BUF，避免轮询流程被中断处理抢占导致时序混乱
    if (hi2c->Mode != I2C_MODE_POLLING)
    {
        I2C_ITConfig(i2c, I2C_IT_EVT | I2C_IT_BUF, DISABLE);
        restore_evt_irq = true;
    }

    hi2c->State = I2C_STATE_BUSY_TX;

    // 等待总线空闲
    ret = wait_flag(i2c, I2C_FLAG_BUSY, RESET, timeout);
    if (ret != I2C_OK)
        goto error;

    // 发送 START
    I2C_GenerateSTART(i2c, ENABLE);
    ret = wait_event(i2c, I2C_EVENT_MASTER_MODE_SELECT, timeout);
    if (ret != I2C_OK)
        goto error;

    // 发送地址
    I2C_Send7bitAddress(i2c, dev_addr << 1, I2C_Direction_Transmitter);
    ret = wait_event(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, timeout);
    if (ret != I2C_OK)
        goto error;

    // 发送数据
    for (uint16_t i = 0; i < len; i++)
    {
        ret = wait_flag(i2c, I2C_FLAG_TXE, SET, timeout);
        if (ret != I2C_OK)
            goto error;
        I2C_SendData(i2c, data[i]);
    }

    // 等待传输完成
    ret = wait_event(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED, timeout);
    if (ret != I2C_OK)
        goto error;

    // 发送 STOP
    I2C_GenerateSTOP(i2c, ENABLE);

    hi2c->State = I2C_STATE_IDLE;
    if (restore_evt_irq)
    {
        I2C_ITConfig(i2c, I2C_IT_EVT | I2C_IT_ERR, ENABLE);
        I2C_ITConfig(i2c, I2C_IT_BUF, DISABLE);
    }
    return I2C_OK;

error:
    if (restore_evt_irq)
    {
        I2C_ITConfig(i2c, I2C_IT_EVT | I2C_IT_ERR, ENABLE);
        I2C_ITConfig(i2c, I2C_IT_BUF, DISABLE);
    }
    if (ret == I2C_ERR_TIMEOUT)
    {
        i2c_recover_bus(i2c_num);
    }
    i2c_error_handler(hi2c, (uint32_t)ret);
    return ret;
}

int bsp_i2c_read_polling(i2c_num_t i2c_num, uint8_t dev_addr,
                         uint8_t *data, uint16_t len, uint32_t timeout)
{
    if (i2c_num >= I2C_NUM_MAX || data == NULL || len == 0)
    {
        return I2C_ERR_INVALID_PARAM;
    }

    I2C_HandleTypeDef *hi2c = get_handle(i2c_num);
    I2C_TypeDef *i2c = get_periph(i2c_num);
    int ret;
    bool restore_evt_irq = false;

    // 当当前工作模式为 IT/DMA 时，临时关闭 EVT/BUF，避免轮询流程被中断处理抢占导致时序混乱
    if (hi2c->Mode != I2C_MODE_POLLING)
    {
        I2C_ITConfig(i2c, I2C_IT_EVT | I2C_IT_BUF, DISABLE);
        restore_evt_irq = true;
    }

    hi2c->State = I2C_STATE_BUSY_RX;

    // 等待总线空闲
    ret = wait_flag(i2c, I2C_FLAG_BUSY, RESET, timeout);
    if (ret != I2C_OK)
        goto error;

    // 发送 START
    I2C_GenerateSTART(i2c, ENABLE);
    ret = wait_event(i2c, I2C_EVENT_MASTER_MODE_SELECT, timeout);
    if (ret != I2C_OK)
        goto error;

    // 发送地址（读模式）
    I2C_Send7bitAddress(i2c, dev_addr << 1, I2C_Direction_Receiver);
    ret = wait_event(i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED, timeout);
    if (ret != I2C_OK)
        goto error;

    // 读取数据
    for (uint16_t i = 0; i < len; i++)
    {
        if (i == len - 1)
        {
            // 最后一个字节，发送 NACK
            I2C_AcknowledgeConfig(i2c, DISABLE);
        }

        ret = wait_flag(i2c, I2C_FLAG_RXNE, SET, timeout);
        if (ret != I2C_OK)
            goto error;

        data[i] = I2C_ReceiveData(i2c);
    }

    // 发送 STOP
    I2C_GenerateSTOP(i2c, ENABLE);
    I2C_AcknowledgeConfig(i2c, ENABLE);

    hi2c->State = I2C_STATE_IDLE;
    if (restore_evt_irq)
    {
        I2C_ITConfig(i2c, I2C_IT_EVT | I2C_IT_ERR, ENABLE);
        I2C_ITConfig(i2c, I2C_IT_BUF, DISABLE);
    }
    return I2C_OK;

error:
    I2C_AcknowledgeConfig(i2c, ENABLE);
    if (restore_evt_irq)
    {
        I2C_ITConfig(i2c, I2C_IT_EVT | I2C_IT_ERR, ENABLE);
        I2C_ITConfig(i2c, I2C_IT_BUF, DISABLE);
    }
    if (ret == I2C_ERR_TIMEOUT)
    {
        i2c_recover_bus(i2c_num);
    }
    i2c_error_handler(hi2c, (uint32_t)ret);
    return ret;
}

int bsp_i2c_write_register_polling(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg,
                                   const uint8_t *data, uint16_t len, uint32_t timeout)
{
    if (i2c_num >= I2C_NUM_MAX || data == NULL || len == 0)
    {
        return I2C_ERR_INVALID_PARAM;
    }

    I2C_HandleTypeDef *hi2c = get_handle(i2c_num);
    I2C_TypeDef *i2c = get_periph(i2c_num);
    int ret;
    bool restore_evt_irq = false;

    // 当当前工作模式为 IT/DMA 时，临时关闭 EVT/BUF，避免轮询流程被中断处理抢占导致时序混乱
    if (hi2c->Mode != I2C_MODE_POLLING)
    {
        I2C_ITConfig(i2c, I2C_IT_EVT | I2C_IT_BUF, DISABLE);
        restore_evt_irq = true;
    }

    hi2c->State = I2C_STATE_BUSY_TX;

    // 等待总线空闲
    ret = wait_flag(i2c, I2C_FLAG_BUSY, RESET, timeout);
    if (ret != I2C_OK)
        goto error;

    // 发送 START
    I2C_GenerateSTART(i2c, ENABLE);
    ret = wait_event(i2c, I2C_EVENT_MASTER_MODE_SELECT, timeout);
    if (ret != I2C_OK)
        goto error;

    // 发送地址
    I2C_Send7bitAddress(i2c, dev_addr << 1, I2C_Direction_Transmitter);
    ret = wait_event(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, timeout);
    if (ret != I2C_OK)
        goto error;

    // 发送寄存器地址
    ret = wait_flag(i2c, I2C_FLAG_TXE, SET, timeout);
    if (ret != I2C_OK)
        goto error;
    I2C_SendData(i2c, reg);

    // 发送数据
    for (uint16_t i = 0; i < len; i++)
    {
        ret = wait_flag(i2c, I2C_FLAG_TXE, SET, timeout);
        if (ret != I2C_OK)
            goto error;
        I2C_SendData(i2c, data[i]);
    }

    // 等待传输完成
    ret = wait_event(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED, timeout);
    if (ret != I2C_OK)
        goto error;

    // 发送 STOP
    I2C_GenerateSTOP(i2c, ENABLE);

    hi2c->State = I2C_STATE_IDLE;
    if (restore_evt_irq)
    {
        I2C_ITConfig(i2c, I2C_IT_EVT | I2C_IT_ERR, ENABLE);
        I2C_ITConfig(i2c, I2C_IT_BUF, DISABLE);
    }
    return I2C_OK;

error:
    if (restore_evt_irq)
    {
        I2C_ITConfig(i2c, I2C_IT_EVT | I2C_IT_ERR, ENABLE);
        I2C_ITConfig(i2c, I2C_IT_BUF, DISABLE);
    }
    if (ret == I2C_ERR_TIMEOUT)
    {
        i2c_recover_bus(i2c_num);
    }
    i2c_error_handler(hi2c, (uint32_t)ret);
    return ret;
}

int bsp_i2c_read_register_polling(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg,
                                  uint8_t *data, uint16_t len, uint32_t timeout)
{
    if (i2c_num >= I2C_NUM_MAX || data == NULL || len == 0)
    {
        return I2C_ERR_INVALID_PARAM;
    }

    int ret;

    // 先写寄存器地址
    ret = bsp_i2c_write_polling(i2c_num, dev_addr, &reg, 1, timeout);
    if (ret != I2C_OK)
        return ret;

    // 再读数据
    return bsp_i2c_read_polling(i2c_num, dev_addr, data, len, timeout);
}

/* ========================== 中断模式函数 ========================== */

int bsp_i2c_write_it(i2c_num_t i2c_num, uint8_t dev_addr, const uint8_t *data, uint16_t len)
{
    if (i2c_num >= I2C_NUM_MAX || data == NULL || len == 0)
    {
        return I2C_ERR_INVALID_PARAM;
    }

    I2C_HandleTypeDef *hi2c = get_handle(i2c_num);
    I2C_TypeDef *i2c = get_periph(i2c_num);

    if (hi2c->State != I2C_STATE_IDLE)
    {
        return I2C_ERR_BUSY;
    }

    // 配置传输参数
    hi2c->DevAddr = dev_addr;
    hi2c->pTxBuffer = (uint8_t *)data;
    hi2c->TxLength = len;
    hi2c->TxCount = 0;
    hi2c->RxLength = 0;
    hi2c->IsRegWrite = false;
    hi2c->XferUseDma = false;
    hi2c->State = I2C_STATE_BUSY_TX;
    hi2c->ErrorCode = I2C_OK;

    I2C_ITConfig(i2c, I2C_IT_BUF, ENABLE);
    // 启动传输
    I2C_GenerateSTART(i2c, ENABLE);

    return I2C_OK;
}

int bsp_i2c_read_it(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t *data, uint16_t len)
{
    if (i2c_num >= I2C_NUM_MAX || data == NULL || len == 0)
    {
        return I2C_ERR_INVALID_PARAM;
    }

    I2C_HandleTypeDef *hi2c = get_handle(i2c_num);
    I2C_TypeDef *i2c = get_periph(i2c_num);

    if (hi2c->State != I2C_STATE_IDLE)
    {
        return I2C_ERR_BUSY;
    }

    // 配置传输参数
    hi2c->DevAddr = dev_addr;
    hi2c->pRxBuffer = data;
    hi2c->RxLength = len;
    hi2c->RxCount = 0;
    hi2c->TxLength = 0;
    hi2c->IsRegWrite = false;
    hi2c->XferUseDma = false;
    hi2c->State = I2C_STATE_BUSY_RX;
    hi2c->ErrorCode = I2C_OK;

    I2C_ITConfig(i2c, I2C_IT_BUF, ENABLE);
    // 如果是单字节接收，提前禁用 ACK
    if (len == 1)
    {
        I2C_AcknowledgeConfig(i2c, DISABLE);
    }

    // 启动传输
    I2C_GenerateSTART(i2c, ENABLE);

    return I2C_OK;
}

int bsp_i2c_write_register_it(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg,
                              const uint8_t *data, uint16_t len)
{
    if (i2c_num >= I2C_NUM_MAX || data == NULL || len == 0 || len > I2C_MAX_WRITE_LEN - 1)
    {
        return I2C_ERR_INVALID_PARAM;
    }

    I2C_HandleTypeDef *hi2c = get_handle(i2c_num);
    I2C_TypeDef *i2c = get_periph(i2c_num);

    if (hi2c->State != I2C_STATE_IDLE)
    {
        return I2C_ERR_BUSY;
    }

    // 使用内部缓冲区构造数据（寄存器地址 + 数据）
    static uint8_t tx_buf[I2C_NUM_MAX][I2C_MAX_WRITE_LEN];
    tx_buf[i2c_num][0] = reg;
    memcpy(&tx_buf[i2c_num][1], data, len);

    // 配置传输参数
    hi2c->DevAddr = dev_addr;
    hi2c->pTxBuffer = tx_buf[i2c_num];
    hi2c->TxLength = len + 1;
    hi2c->TxCount = 0;
    hi2c->RxLength = 0;
    hi2c->IsRegWrite = false;
    hi2c->XferUseDma = false;
    hi2c->State = I2C_STATE_BUSY_TX;
    hi2c->ErrorCode = I2C_OK;

    I2C_ITConfig(i2c, I2C_IT_BUF, ENABLE);
    // 启动传输
    I2C_GenerateSTART(i2c, ENABLE);

    return I2C_OK;
}

int bsp_i2c_read_register_it(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg,
                             uint8_t *data, uint16_t len)
{
    if (i2c_num >= I2C_NUM_MAX || data == NULL || len == 0)
    {
        return I2C_ERR_INVALID_PARAM;
    }

    I2C_HandleTypeDef *hi2c = get_handle(i2c_num);
    I2C_TypeDef *i2c = get_periph(i2c_num);

    if (hi2c->State != I2C_STATE_IDLE)
    {
        return I2C_ERR_BUSY;
    }

    // 配置传输参数（先写寄存器地址，再读）
    hi2c->DevAddr = dev_addr;
    hi2c->RegAddr = reg;
    hi2c->pRxBuffer = data;
    hi2c->RxLength = len;
    hi2c->RxCount = 0;
    hi2c->TxLength = 1;
    hi2c->TxCount = 0;
    hi2c->IsRegWrite = true;
    hi2c->XferUseDma = false;
    hi2c->State = I2C_STATE_BUSY_TX;
    hi2c->ErrorCode = I2C_OK;

    I2C_ITConfig(i2c, I2C_IT_BUF, ENABLE);
    // 启动传输
    I2C_GenerateSTART(i2c, ENABLE);

    return I2C_OK;
}

/* ========================== DMA 模式函数 ========================== */

void bsp_i2c_dma_init(i2c_num_t i2c_num, DMA_Channel_TypeDef *tx_channel, DMA_Channel_TypeDef *rx_channel)
{
    if (i2c_num >= I2C_NUM_MAX)
        return;

    I2C_HandleTypeDef *hi2c = get_handle(i2c_num);

    hi2c->TxDmaChannel = tx_channel;
    hi2c->RxDmaChannel = rx_channel;
    hi2c->DmaEnabled = (tx_channel != NULL) || (rx_channel != NULL);

    // 使能 DMA 时钟
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
}

static void i2c_dma_config(DMA_Channel_TypeDef *channel, uint32_t periph_addr,
                           uint32_t mem_addr, uint16_t size, uint32_t direction)
{
    DMA_InitTypeDef dma_init = {0};
    DMA_StructInit(&dma_init);

    dma_init.DMA_PeripheralBaseAddr = periph_addr;
    dma_init.DMA_MemoryBaseAddr = mem_addr;
    dma_init.DMA_DIR = direction;
    dma_init.DMA_BufferSize = size;
    dma_init.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma_init.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma_init.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma_init.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dma_init.DMA_Mode = DMA_Mode_Normal;
    dma_init.DMA_Priority = DMA_Priority_High;
    dma_init.DMA_M2M = DMA_M2M_Disable;

    DMA_Init(channel, &dma_init);
}

static uint32_t i2c_dma_tc_it_from_channel(DMA_Channel_TypeDef *channel)
{
    if (channel == DMA1_Channel1) return DMA1_IT_TC1;
    if (channel == DMA1_Channel2) return DMA1_IT_TC2;
    if (channel == DMA1_Channel3) return DMA1_IT_TC3;
    if (channel == DMA1_Channel4) return DMA1_IT_TC4;
    if (channel == DMA1_Channel5) return DMA1_IT_TC5;
    if (channel == DMA1_Channel6) return DMA1_IT_TC6;
    if (channel == DMA1_Channel7) return DMA1_IT_TC7;
    if (channel == DMA1_Channel8) return DMA1_IT_TC8;
    return 0;
}

int bsp_i2c_write_dma(i2c_num_t i2c_num, uint8_t dev_addr, const uint8_t *data, uint16_t len)
{
    if (i2c_num >= I2C_NUM_MAX || data == NULL || len == 0)
    {
        return I2C_ERR_INVALID_PARAM;
    }

    I2C_HandleTypeDef *hi2c = get_handle(i2c_num);
    I2C_TypeDef *i2c = get_periph(i2c_num);

    if (hi2c->State != I2C_STATE_IDLE)
    {
        return I2C_ERR_BUSY;
    }

    if (!hi2c->DmaEnabled || hi2c->TxDmaChannel == NULL)
    {
        return I2C_ERR_INVALID_PARAM;
    }

    // 配置 DMA
    i2c_dma_config(hi2c->TxDmaChannel, (uint32_t)&i2c->DATAR,
                   (uint32_t)data, len, DMA_DIR_PeripheralDST);

    // 配置传输参数
    hi2c->DevAddr = dev_addr;
    hi2c->pTxBuffer = (uint8_t *)data;
    hi2c->TxLength = len;
    hi2c->TxCount = 0;
    hi2c->RxLength = 0;
    hi2c->IsRegWrite = false;
    hi2c->XferUseDma = true;
    hi2c->State = I2C_STATE_BUSY_TX;
    hi2c->ErrorCode = I2C_OK;

    // 避免 IT 残留路径干扰 DMA 数据发送
    I2C_ITConfig(i2c, I2C_IT_BUF, DISABLE);
    I2C_ITConfig(i2c, I2C_IT_EVT | I2C_IT_ERR, ENABLE);

    // 使能 DMA 中断
    DMA_ITConfig(hi2c->TxDmaChannel, DMA_IT_TC, ENABLE);
    DMA_ClearFlag(i2c_dma_tc_it_from_channel(hi2c->TxDmaChannel));
    DMA_ClearITPendingBit(i2c_dma_tc_it_from_channel(hi2c->TxDmaChannel));

    // 先保持 DMA 通道关闭，等待 ADDR 事件后再开启，避免首字节在地址阶段前被写入 DATAR
    I2C_DMACmd(i2c, ENABLE);
    DMA_Cmd(hi2c->TxDmaChannel, DISABLE);
    I2C_GenerateSTART(i2c, ENABLE);

    return I2C_OK;
}

int bsp_i2c_read_dma(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t *data, uint16_t len)
{
    if (i2c_num >= I2C_NUM_MAX || data == NULL || len == 0)
    {
        return I2C_ERR_INVALID_PARAM;
    }

    I2C_HandleTypeDef *hi2c = get_handle(i2c_num);
    I2C_TypeDef *i2c = get_periph(i2c_num);

    if (hi2c->State != I2C_STATE_IDLE)
    {
        return I2C_ERR_BUSY;
    }

    if (!hi2c->DmaEnabled || hi2c->RxDmaChannel == NULL)
    {
        return I2C_ERR_INVALID_PARAM;
    }

    // 单字节接收在 I2C 上对 ACK/STOP 时序要求苛刻，直接走轮询更稳
    if (len == 1)
    {
        return bsp_i2c_read_polling(i2c_num, dev_addr, data, len, I2C_TIMEOUT_DEFAULT);
    }

    // 配置 DMA
    i2c_dma_config(hi2c->RxDmaChannel, (uint32_t)&i2c->DATAR,
                   (uint32_t)data, len, DMA_DIR_PeripheralSRC);

    // 配置传输参数
    hi2c->DevAddr = dev_addr;
    hi2c->pRxBuffer = data;
    hi2c->RxLength = len;
    hi2c->RxCount = 0;
    hi2c->TxLength = 0;
    hi2c->IsRegWrite = false;
    hi2c->XferUseDma = true;
    hi2c->State = I2C_STATE_BUSY_RX;
    hi2c->ErrorCode = I2C_OK;

    // 避免 IT 残留路径干扰 DMA 接收
    I2C_ITConfig(i2c, I2C_IT_BUF, DISABLE);
    I2C_ITConfig(i2c, I2C_IT_EVT | I2C_IT_ERR, ENABLE);

    // DMA 读默认保持 ACK 打开（单字节读已走轮询分支）
    I2C_AcknowledgeConfig(i2c, ENABLE);

    // 多字节 DMA 接收需要 LAST，确保硬件在最后一个字节正确收尾
    I2C_DMALastTransferCmd(i2c, ENABLE);

    // 使能 DMA 中断
    DMA_ITConfig(hi2c->RxDmaChannel, DMA_IT_TC, ENABLE);
    DMA_ClearFlag(i2c_dma_tc_it_from_channel(hi2c->RxDmaChannel));
    DMA_ClearITPendingBit(i2c_dma_tc_it_from_channel(hi2c->RxDmaChannel));

    // 先保持 DMA 通道关闭，等待 ADDR 事件后再开启
    I2C_DMACmd(i2c, ENABLE);
    DMA_Cmd(hi2c->RxDmaChannel, DISABLE);
    I2C_GenerateSTART(i2c, ENABLE);

    return I2C_OK;
}

int bsp_i2c_write_register_dma(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg,
                               const uint8_t *data, uint16_t len)
{
    if (i2c_num >= I2C_NUM_MAX || data == NULL || len == 0 || len > I2C_MAX_WRITE_LEN - 1)
    {
        return I2C_ERR_INVALID_PARAM;
    }

    I2C_HandleTypeDef *hi2c = get_handle(i2c_num);

    if (hi2c->State != I2C_STATE_IDLE)
    {
        return I2C_ERR_BUSY;
    }

    // 使用内部缓冲区构造数据
    static uint8_t tx_buf[I2C_NUM_MAX][I2C_MAX_WRITE_LEN];
    tx_buf[i2c_num][0] = reg;
    memcpy(&tx_buf[i2c_num][1], data, len);

    return bsp_i2c_write_dma(i2c_num, dev_addr, tx_buf[i2c_num], len + 1);
}

int bsp_i2c_read_register_dma(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg,
                              uint8_t *data, uint16_t len)
{
    if (i2c_num >= I2C_NUM_MAX || data == NULL || len == 0)
    {
        return I2C_ERR_INVALID_PARAM;
    }

    I2C_HandleTypeDef *hi2c = get_handle(i2c_num);
    if (hi2c->State != I2C_STATE_IDLE)
    {
        return I2C_ERR_BUSY;
    }

    // 对于 DMA 模式，先使用中断模式发送寄存器地址，然后使用 DMA 接收数据
    // 这里简化为先用轮询发送寄存器地址
    int ret = bsp_i2c_write_polling(i2c_num, dev_addr, &reg, 1, I2C_TIMEOUT_DEFAULT);
    if (ret != I2C_OK)
        return ret;

    // 等待 STOP 真正释放总线，再开始 DMA 读阶段
    ret = wait_flag(get_periph(i2c_num), I2C_FLAG_BUSY, RESET, I2C_TIMEOUT_DEFAULT);
    if (ret != I2C_OK)
    {
        bsp_i2c_recover(i2c_num);
        return ret;
    }

    return bsp_i2c_read_dma(i2c_num, dev_addr, data, len);
}

/* ========================== 简化接口 ========================== */

int bsp_i2c_write(i2c_num_t i2c_num, uint8_t dev_addr, const uint8_t *data, uint16_t len)
{
    if (i2c_num >= I2C_NUM_MAX)
        return I2C_ERR_INVALID_PARAM;

    uint8_t mode = get_handle(i2c_num)->Mode;

    switch (mode)
    {
    case I2C_MODE_POLLING:
        return bsp_i2c_write_polling(i2c_num, dev_addr, data, len, I2C_TIMEOUT_DEFAULT);
    case I2C_MODE_IT:
        return bsp_i2c_write_it(i2c_num, dev_addr, data, len);
    case I2C_MODE_DMA:
        return bsp_i2c_write_dma(i2c_num, dev_addr, data, len);
    default:
        return I2C_ERR_INVALID_PARAM;
    }
}

int bsp_i2c_read(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t *data, uint16_t len)
{
    if (i2c_num >= I2C_NUM_MAX)
        return I2C_ERR_INVALID_PARAM;

    uint8_t mode = get_handle(i2c_num)->Mode;

    switch (mode)
    {
    case I2C_MODE_POLLING:
        return bsp_i2c_read_polling(i2c_num, dev_addr, data, len, I2C_TIMEOUT_DEFAULT);
    case I2C_MODE_IT:
        return bsp_i2c_read_it(i2c_num, dev_addr, data, len);
    case I2C_MODE_DMA:
        return bsp_i2c_read_dma(i2c_num, dev_addr, data, len);
    default:
        return I2C_ERR_INVALID_PARAM;
    }
}

int bsp_i2c_write_register(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg,
                           const uint8_t *data, uint16_t len)
{
    if (i2c_num >= I2C_NUM_MAX)
        return I2C_ERR_INVALID_PARAM;

    uint8_t mode = get_handle(i2c_num)->Mode;

    switch (mode)
    {
    case I2C_MODE_POLLING:
        return bsp_i2c_write_register_polling(i2c_num, dev_addr, reg, data, len, I2C_TIMEOUT_DEFAULT);
    case I2C_MODE_IT:
        return bsp_i2c_write_register_it(i2c_num, dev_addr, reg, data, len);
    case I2C_MODE_DMA:
        return bsp_i2c_write_register_dma(i2c_num, dev_addr, reg, data, len);
    default:
        return I2C_ERR_INVALID_PARAM;
    }
}

int bsp_i2c_read_register(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg,
                          uint8_t *data, uint16_t len)
{
    if (i2c_num >= I2C_NUM_MAX)
        return I2C_ERR_INVALID_PARAM;

    uint8_t mode = get_handle(i2c_num)->Mode;

    switch (mode)
    {
    case I2C_MODE_POLLING:
        return bsp_i2c_read_register_polling(i2c_num, dev_addr, reg, data, len, I2C_TIMEOUT_DEFAULT);
    case I2C_MODE_IT:
        return bsp_i2c_read_register_it(i2c_num, dev_addr, reg, data, len);
    case I2C_MODE_DMA:
        return bsp_i2c_read_register_dma(i2c_num, dev_addr, reg, data, len);
    default:
        return I2C_ERR_INVALID_PARAM;
    }
}

int bsp_i2c_write_byte(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, uint8_t data)
{
    return bsp_i2c_write_register(i2c_num, dev_addr, reg, &data, 1);
}

int bsp_i2c_read_byte(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, uint8_t *data)
{
    return bsp_i2c_read_register(i2c_num, dev_addr, reg, data, 1);
}

/* ========================== 设备探测 ========================== */

int bsp_i2c_check_device(i2c_num_t i2c_num, uint8_t dev_addr, uint32_t timeout)
{
    if (i2c_num >= I2C_NUM_MAX)
        return I2C_ERR_INVALID_PARAM;

    I2C_TypeDef *i2c = get_periph(i2c_num);
    int ret;

    // 等待总线空闲
    ret = wait_flag(i2c, I2C_FLAG_BUSY, RESET, timeout);
    if (ret != I2C_OK)
        return ret;

    // 发送 START
    I2C_GenerateSTART(i2c, ENABLE);
    ret = wait_event(i2c, I2C_EVENT_MASTER_MODE_SELECT, timeout);
    if (ret != I2C_OK)
        goto error;

    // 发送地址
    I2C_Send7bitAddress(i2c, dev_addr << 1, I2C_Direction_Transmitter);

    // 等待 ACK
    ret = wait_event(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, timeout);

error:
    // 发送 STOP
    I2C_GenerateSTOP(i2c, ENABLE);

    return (ret == I2C_OK) ? I2C_OK : I2C_ERR_AF;
}

uint8_t bsp_i2c_scan(i2c_num_t i2c_num, uint8_t *found_addr, uint8_t max_count)
{
    if (i2c_num >= I2C_NUM_MAX || found_addr == NULL || max_count == 0)
    {
        return 0;
    }

    uint8_t count = 0;

    // 扫描 0x08 - 0x77 范围内的地址
    for (uint8_t addr = 0x08; addr <= 0x77 && count < max_count; addr++)
    {
        if (bsp_i2c_check_device(i2c_num, addr, 1000) == I2C_OK)
        {
            found_addr[count++] = addr;
        }
    }

    return count;
}

/* ========================== 回调函数注册 ========================== */

void bsp_i2c_register_tx_callback(i2c_num_t i2c_num, bsp_i2c_callback_t callback)
{
    if (i2c_num >= I2C_NUM_MAX)
        return;
    get_handle(i2c_num)->TxCpltCallback = callback;
}

void bsp_i2c_register_rx_callback(i2c_num_t i2c_num, bsp_i2c_callback_t callback)
{
    if (i2c_num >= I2C_NUM_MAX)
        return;
    get_handle(i2c_num)->RxCpltCallback = callback;
}

void bsp_i2c_register_error_callback(i2c_num_t i2c_num, bsp_i2c_error_callback_t callback)
{
    if (i2c_num >= I2C_NUM_MAX)
        return;
    get_handle(i2c_num)->ErrorCallback = callback;
}

/* ========================== 中断处理 ========================== */

static void i2c_ev_handler(I2C_HandleTypeDef *hi2c, i2c_num_t num)
{
    I2C_TypeDef *i2c = hi2c->Instance;
    bool dma_active = hi2c->XferUseDma;

    if (hi2c->State == I2C_STATE_IDLE)
    {
        I2C_ITConfig(i2c, I2C_IT_BUF, DISABLE);
        return;
    }

    // 1. SB - START 条件已发送
    if (I2C_GetITStatus(i2c, I2C_IT_SB))
    {
        if (hi2c->IsRegWrite && hi2c->RxLength > 0)
        {
            // 寄存器读操作，先写寄存器地址
            I2C_Send7bitAddress(i2c, hi2c->DevAddr << 1, I2C_Direction_Transmitter);
        }
        else if (hi2c->RxLength > 0)
        {
            // 直接读
            I2C_Send7bitAddress(i2c, hi2c->DevAddr << 1, I2C_Direction_Receiver);
        }
        else
        {
            // 写操作
            I2C_Send7bitAddress(i2c, hi2c->DevAddr << 1, I2C_Direction_Transmitter);
        }
    }
    // 2. ADDR - 地址已发送
    else if (I2C_GetITStatus(i2c, I2C_IT_ADDR))
    {
        // 清除 ADDR 标志
        __IO uint32_t tmp = i2c->STAR1;
        tmp = i2c->STAR2;
        (void)tmp;

        // DMA 事务在地址阶段完成后再开启 DMA 通道，避免首字节被地址写入覆盖
        if (dma_active)
        {
            if (hi2c->State == I2C_STATE_BUSY_TX && hi2c->TxDmaChannel != NULL)
            {
                DMA_Cmd(hi2c->TxDmaChannel, ENABLE);
            }
            else if (hi2c->State == I2C_STATE_BUSY_RX && hi2c->RxDmaChannel != NULL)
            {
                DMA_Cmd(hi2c->RxDmaChannel, ENABLE);
            }
        }

        // 如果是接收模式且只剩1字节，提前禁用 ACK
        if (hi2c->RxLength == 1 && !hi2c->IsRegWrite)
        {
            I2C_AcknowledgeConfig(i2c, DISABLE);
        }
    }
    // DMA 模式下，数据搬运与收尾由 DMA 路径处理，避免与 IT 路径并发
    else if (dma_active)
    {
        return;
    }
    // 3. BTF - 当前字节真正发送完成，用于收尾或重复起始
    else if (I2C_GetITStatus(i2c, I2C_IT_BTF))
    {
        if (hi2c->State == I2C_STATE_BUSY_TX && hi2c->TxCount >= hi2c->TxLength)
        {
            if (hi2c->IsRegWrite && hi2c->RxLength > 0)
            {
                hi2c->IsRegWrite = false;
                hi2c->State = I2C_STATE_BUSY_RX;
                if (hi2c->RxLength == 1)
                {
                    I2C_AcknowledgeConfig(i2c, DISABLE);
                }
                I2C_GenerateSTART(i2c, ENABLE);
            }
            else
            {
                I2C_GenerateSTOP(i2c, ENABLE);
                I2C_ITConfig(i2c, I2C_IT_BUF, DISABLE);
                hi2c->State = I2C_STATE_IDLE;
                hi2c->XferUseDma = false;

                if (hi2c->TxCpltCallback)
                {
                    hi2c->TxCpltCallback(num);
                }
            }
        }
    }
    // 4. TXE - 发送缓冲区空，只负责继续喂数据
    else if (I2C_GetITStatus(i2c, I2C_IT_TXE))
    {
        if (hi2c->TxCount < hi2c->TxLength)
        {
            if (hi2c->IsRegWrite && hi2c->TxCount == 0)
            {
                // 发送寄存器地址
                I2C_SendData(i2c, hi2c->RegAddr);
            }
            else
            {
                I2C_SendData(i2c, hi2c->pTxBuffer[hi2c->TxCount]);
            }
            hi2c->TxCount++;
        }
    }
    // 5. RXNE - 接收缓冲区非空
    else if (I2C_GetITStatus(i2c, I2C_IT_RXNE))
    {
        if (hi2c->RxCount < hi2c->RxLength)
        {
            hi2c->pRxBuffer[hi2c->RxCount++] = I2C_ReceiveData(i2c);

            if (hi2c->RxCount == hi2c->RxLength - 1)
            {
                // 下一个字节是最后一个，禁用 ACK
                I2C_AcknowledgeConfig(i2c, DISABLE);
            }
            else if (hi2c->RxCount >= hi2c->RxLength)
            {
                // 接收完成
                I2C_GenerateSTOP(i2c, ENABLE);
                I2C_AcknowledgeConfig(i2c, ENABLE);
                I2C_ITConfig(i2c, I2C_IT_BUF, DISABLE);
                hi2c->State = I2C_STATE_IDLE;
                hi2c->XferUseDma = false;

                if (hi2c->RxCpltCallback)
                {
                    hi2c->RxCpltCallback(num);
                }
            }
        }
    }
}

static void i2c_er_handler(I2C_HandleTypeDef *hi2c, i2c_num_t num)
{
    I2C_TypeDef *i2c = hi2c->Instance;
    uint32_t error = I2C_OK;

    // 检查错误标志
    if (I2C_GetFlagStatus(i2c, I2C_FLAG_AF))
    {
        I2C_ClearFlag(i2c, I2C_FLAG_AF);
        error = I2C_ERR_AF;
    }
    if (I2C_GetFlagStatus(i2c, I2C_FLAG_BERR))
    {
        I2C_ClearFlag(i2c, I2C_FLAG_BERR);
        error = I2C_ERR_BERR;
    }
    if (I2C_GetFlagStatus(i2c, I2C_FLAG_ARLO))
    {
        I2C_ClearFlag(i2c, I2C_FLAG_ARLO);
        error = I2C_ERR_ARLO;
    }
    if (I2C_GetFlagStatus(i2c, I2C_FLAG_OVR))
    {
        I2C_ClearFlag(i2c, I2C_FLAG_OVR);
        error = I2C_ERR_DMA;
    }

    if (error != I2C_OK)
    {
        i2c_error_handler(hi2c, error);
    }
}

void bsp_i2c_irq_handler(i2c_num_t i2c_num)
{
    if (i2c_num >= I2C_NUM_MAX)
        return;

    I2C_HandleTypeDef *hi2c = get_handle(i2c_num);
    I2C_TypeDef *i2c = hi2c->Instance;

    // 事件中断
    if (I2C_GetITStatus(i2c, I2C_IT_SB) ||
        I2C_GetITStatus(i2c, I2C_IT_ADDR) ||
        I2C_GetITStatus(i2c, I2C_IT_TXE) ||
        I2C_GetITStatus(i2c, I2C_IT_RXNE) ||
        I2C_GetITStatus(i2c, I2C_IT_BTF))
    {
        i2c_ev_handler(hi2c, i2c_num);
    }

    // 错误中断：只在真实错误标志出现时处理，避免调试时反复误入
    if (I2C_GetFlagStatus(i2c, I2C_FLAG_AF) ||
        I2C_GetFlagStatus(i2c, I2C_FLAG_BERR) ||
        I2C_GetFlagStatus(i2c, I2C_FLAG_ARLO) ||
        I2C_GetFlagStatus(i2c, I2C_FLAG_OVR))
    {
        i2c_er_handler(hi2c, i2c_num);
    }
}

void bsp_i2c_dma_tx_irq_handler(i2c_num_t i2c_num)
{
    if (i2c_num >= I2C_NUM_MAX)
        return;

    I2C_HandleTypeDef *hi2c = get_handle(i2c_num);
    I2C_TypeDef *i2c = hi2c->Instance;
    DMA_Channel_TypeDef *channel = hi2c->TxDmaChannel;
    uint32_t tc_it = i2c_dma_tc_it_from_channel(channel);
    bool dma_tx_active = (hi2c->XferUseDma && (hi2c->State == I2C_STATE_BUSY_TX));

    /*
     * 仅在“当前事务确实为 DMA-TX”时处理 TC。
     * 否则（例如命令走 IT、数据走 DMA 的切换窗口）若误入此中断，会提前发 STOP 打断当前页事务。
     */
    if (!dma_tx_active)
    {
        if (tc_it != 0U && DMA_GetITStatus(tc_it))
        {
            DMA_ClearITPendingBit(tc_it);
        }
        return;
    }

    if (tc_it != 0 && DMA_GetITStatus(tc_it))
    {
        // 防止残留 TC 标志导致“提前完成”误判
        if (DMA_GetCurrDataCounter(channel) != 0U)
        {
            DMA_ClearITPendingBit(tc_it);
            return;
        }

        DMA_ClearITPendingBit(tc_it);
        DMA_Cmd(channel, DISABLE);

        // 等待 BTF
        uint32_t timeout = I2C_TIMEOUT_DEFAULT;
        while (!I2C_GetFlagStatus(i2c, I2C_FLAG_BTF))
        {
            if (timeout-- == 0)
            {
                i2c_error_handler(hi2c, I2C_ERR_TIMEOUT);
                return;
            }
        }

        // 发送 STOP
        I2C_GenerateSTOP(i2c, ENABLE);
        I2C_DMACmd(i2c, DISABLE);

        // 等待总线真正空闲，避免下一笔过早启动导致首字节异常
        timeout = I2C_TIMEOUT_DEFAULT;
        while (I2C_GetFlagStatus(i2c, I2C_FLAG_BUSY))
        {
            if (timeout-- == 0)
            {
                i2c_error_handler(hi2c, I2C_ERR_TIMEOUT);
                return;
            }
        }

        hi2c->State = I2C_STATE_IDLE;
        hi2c->XferUseDma = false;

        if (hi2c->TxCpltCallback)
        {
            hi2c->TxCpltCallback(i2c_num);
        }
    }
}

void bsp_i2c_dma_rx_irq_handler(i2c_num_t i2c_num)
{
    if (i2c_num >= I2C_NUM_MAX)
        return;

    I2C_HandleTypeDef *hi2c = get_handle(i2c_num);
    I2C_TypeDef *i2c = hi2c->Instance;
    DMA_Channel_TypeDef *channel = hi2c->RxDmaChannel;
    uint32_t tc_it = i2c_dma_tc_it_from_channel(channel);
    bool dma_rx_active = (hi2c->XferUseDma && (hi2c->State == I2C_STATE_BUSY_RX));

    /* 同上：非 DMA-RX 事务误入时只清标志，不介入收尾 */
    if (!dma_rx_active)
    {
        if (tc_it != 0U && DMA_GetITStatus(tc_it))
        {
            DMA_ClearITPendingBit(tc_it);
        }
        return;
    }

    if (tc_it != 0 && DMA_GetITStatus(tc_it))
    {
        // 防止残留 TC 标志导致“提前完成”误判
        if (DMA_GetCurrDataCounter(channel) != 0U)
        {
            DMA_ClearITPendingBit(tc_it);
            return;
        }

        DMA_ClearITPendingBit(tc_it);
        DMA_Cmd(channel, DISABLE);

        // 发送 STOP
        I2C_GenerateSTOP(i2c, ENABLE);
        I2C_AcknowledgeConfig(i2c, ENABLE);
        I2C_DMALastTransferCmd(i2c, DISABLE);
        I2C_DMACmd(i2c, DISABLE);

        // 等待总线真正空闲
        uint32_t timeout = I2C_TIMEOUT_DEFAULT;
        while (I2C_GetFlagStatus(i2c, I2C_FLAG_BUSY))
        {
            if (timeout-- == 0)
            {
                i2c_error_handler(hi2c, I2C_ERR_TIMEOUT);
                return;
            }
        }

        hi2c->State = I2C_STATE_IDLE;
        hi2c->XferUseDma = false;

        if (hi2c->RxCpltCallback)
        {
            hi2c->RxCpltCallback(i2c_num);
        }
    }
}

/* ========================== 弱定义回调 ========================== */

__attribute__((weak)) void bsp_i2c1_tx_cplt_callback(void)
{
}
__attribute__((weak)) void bsp_i2c1_rx_cplt_callback(void)
{
}
__attribute__((weak)) void bsp_i2c1_error_callback(uint32_t error_code)
{
    (void)error_code;
}
__attribute__((weak)) void bsp_i2c2_tx_cplt_callback(void)
{
}
__attribute__((weak)) void bsp_i2c2_rx_cplt_callback(void)
{
}
__attribute__((weak)) void bsp_i2c2_error_callback(uint32_t error_code)
{
    (void)error_code;
}




