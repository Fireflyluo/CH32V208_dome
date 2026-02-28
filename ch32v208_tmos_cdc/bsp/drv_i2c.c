/**
 ******************************************************************************
 * @file    I2C.c
 * @brief
 ******************************************************************************
 * @note    本文件内实现I2C相关的初始化代码
 *
 ******************************************************************************
 */

#include "drv_i2c.h"

#include <string.h>

#include "board.h"

void I2C1_EV_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void I2C1_ER_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void I2C2_EV_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void I2C2_ER_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

__attribute__((weak)) void i2c_master_tx_cplt_callback(i2c_num_t i2c_num);
__attribute__((weak)) void i2c_master_rx_cplt_callback(i2c_num_t i2c_num);
__attribute__((weak)) void i2c_error_callback(i2c_num_t i2c_num, i2c_ErrCode_t errcode);
/* ========================== 内部常量定义 ========================== */

#define I2C_TIMEOUT 0xFFFF     // 超时计数值
#define I2C_ERR_TIMEOUT 0x1000 // 标志超时错误码

static void CommTimeOut_CallBack(i2c_ErrCode_t errcode);
// 通信控制标志
static CommCtrl_t Comm_Flag = C_READY;
// I2C外设指针数组
static I2C_TypeDef *const i2c_periph[] = {I2C1, I2C2};
// 异步通信上下文
static i2c_async_ctx_t i2c_async_ctx[2] = {0};

static uint8_t i2c_async_reg_buf[2][1];  // [I2C_NUM][1] 用于寄存器地址
static uint8_t i2c_async_byte_buf[2][2]; // [I2C_NUM][2] 用于单字节操作

static uint8_t i2c_async_write_buf[2][I2C_MAX_WRITE_LEN]; // [I2C_NUM][MAX_LEN]
// GPIO配置结构
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

// I2C硬件配置表
static const i2c_hw_config_t i2c_hw_config[] = {
    [I2C_NUM_1] = {.scl_pin      = GPIO_Pin_6,
                   .sda_pin      = GPIO_Pin_7,
                   .gpio_port    = GPIOB,
                   .rcc_gpio_clk = RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO,
                   .rcc_i2c_clk  = RCC_APB1Periph_I2C1,
                   .remap_config = 0,
                   .mode         = I2C1_MODE},

    [I2C_NUM_2] = {.scl_pin      = GPIO_Pin_10,
                   .sda_pin      = GPIO_Pin_11,
                   .gpio_port    = GPIOB,
                   .rcc_gpio_clk = RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO,
                   .rcc_i2c_clk  = RCC_APB1Periph_I2C2,
                   .remap_config = 0, // 无需重映射
                   .mode         = I2C2_MODE}};

/* ========================== 内部辅助函数 ========================== */

/**
 * @brief 获取I2C外设指针
 */
static inline I2C_TypeDef *get_i2c_periph(i2c_num_t i2c_num)
{
    if (i2c_num >= sizeof(i2c_periph) / sizeof(i2c_periph[0]))
    {
        return NULL;
    }
    return i2c_periph[i2c_num];
}

/**
 * @brief 等待标志位设置（带超时）
 */
static int wait_flag_timeout(I2C_TypeDef *i2c, uint32_t flag, FlagStatus status)
{
    uint32_t timeout = I2C_TIMEOUT;

    if (status == SET)
    {
        while (!I2C_GetFlagStatus(i2c, flag))
        {
            if (timeout-- == 0)
                return I2C_ERR_TIMEOUT;
        }
    }
    else
    {
        while (I2C_GetFlagStatus(i2c, flag))
        {
            if (timeout-- == 0)
                return I2C_ERR_TIMEOUT;
        }
    }
    return 0;
}

/**
 * @brief 等待事件（带超时）
 */
static int wait_event_timeout(I2C_TypeDef *i2c, uint32_t event)
{
    uint32_t timeout = I2C_TIMEOUT;

    while (!I2C_CheckEvent(i2c, event))
    {
        if (timeout-- == 0)
            return I2C_FLAG_TIMEOUT;
    }
    return 0;
}

#if (COMM_RECOVER_MODE == MODULE_SELF_RESET)
/**
 * @brief 软件复位 I2C 模块（通过 GPIO 模拟 STOP 并复位外设）
 */
static void IIC_SWReset(void)
{
    // 1. 禁用所有 I2C 中断
    I2C_ITConfig(I2C1, I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR, DISABLE);
    I2C_ITConfig(I2C2, I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR, DISABLE);

    // 2. 禁用 I2C 外设
    I2C_Cmd(I2C1, DISABLE);
    I2C_Cmd(I2C2, DISABLE);

    // 3. 将 I2C 引脚切换为 GPIO 模式，强制释放总线
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    // 配置 I2C1 引脚 (PB6=SCL, PB7=SDA)
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_OD; // 开漏输出
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // 4. 生成 9 个时钟脉冲（强制从机释放 SDA）
    for (int i = 0; i < 9; i++)
    {
        // SCL 低
        GPIO_ResetBits(GPIOB, GPIO_Pin_6);
        Delay_Us(5); // 至少 5us（对应 100kHz）

        // SCL 高
        GPIO_SetBits(GPIOB, GPIO_Pin_6);
        Delay_Us(5);
    }

    // 5. 生成 STOP 条件：SCL 高时，SDA 从低到高
    // 先确保 SDA 为低
    GPIO_ResetBits(GPIOB, GPIO_Pin_7);
    Delay_Us(5);
    // SCL 拉高
    GPIO_SetBits(GPIOB, GPIO_Pin_6);
    Delay_Us(5);
    // SDA 拉高（STOP）
    GPIO_SetBits(GPIOB, GPIO_Pin_7);
    Delay_Us(10);

    // 6. 恢复 I2C 外设和 GPIO 复用功能
    // 重新初始化 I2C1
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    I2C_DeInit(I2C1);
    I2C_Cmd(I2C1, ENABLE);

    // 7. 重置通信状态标志
    Comm_Flag = C_READY;

    // 8. 清除所有异步上下文
    memset(&i2c_async_ctx, 0, sizeof(i2c_async_ctx));
}
#elif (COMM_RECOVER_MODE == MODULE_RCC_RESET)
/**
 * @brief 复位时钟
 */
static void IIC_RCCReset(void) {}
#elif (COMM_RECOVER_MODE == SYSTEM_NVIC_RESET)
// 系统NVIC复位模式
SystemNVICReset();
#endif
/* ========================== 公共接口实现 ========================== */

/**
 * @brief 初始化I2C控制器
 */
int i2c_init(i2c_num_t i2c_num, const i2c_config_t *config)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    I2C_InitTypeDef I2C_InitStructure   = {0};
    const i2c_hw_config_t *hw_cfg;
    I2C_TypeDef *i2c;

    // 参数检查
    if (i2c_num >= sizeof(i2c_hw_config) / sizeof(i2c_hw_config[0]))
    {
        return -1;
    }

    if (config == NULL)
    {
        return -1;
    }

    hw_cfg = &i2c_hw_config[i2c_num];
    i2c    = get_i2c_periph(i2c_num);

    if (i2c == NULL)
    {
        return -1;
    }

    // 使能时钟
    RCC_APB2PeriphClockCmd(hw_cfg->rcc_gpio_clk, ENABLE);
    RCC_APB1PeriphClockCmd(hw_cfg->rcc_i2c_clk, ENABLE);

    // 配置GPIO
    GPIO_InitStructure.GPIO_Pin   = hw_cfg->scl_pin | hw_cfg->sda_pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_OD; // 开漏输出
    GPIO_Init(hw_cfg->gpio_port, &GPIO_InitStructure);

    // 如果I2C1需要重映射
    if (i2c_num == I2C_NUM_1 && hw_cfg->remap_config != 0)
    {
        GPIO_PinRemapConfig(hw_cfg->remap_config, ENABLE);
    }

    // 配置I2C
    I2C_StructInit(&I2C_InitStructure);
    I2C_InitStructure.I2C_ClockSpeed  = config->clock_speed;
    I2C_InitStructure.I2C_DutyCycle   = config->duty_cycle;
    I2C_InitStructure.I2C_OwnAddress1 = config->own_address;
    I2C_InitStructure.I2C_Ack         = config->enable_ack ? I2C_Ack_Enable : I2C_Ack_Disable;
    I2C_InitStructure.I2C_AcknowledgedAddress =
        config->is_7_bit_address ? I2C_AcknowledgedAddress_7bit : I2C_AcknowledgedAddress_10bit;

    I2C_Init(i2c, &I2C_InitStructure);
    I2C_Cmd(i2c, ENABLE);

    if (hw_cfg->mode == I2C_MODE_IT)
    {
        // 配置NVIC中断优先级
        NVIC_InitTypeDef NVIC_InitStructure = {0};
        if (i2c_num == I2C_NUM_1)
        {
            NVIC_InitStructure.NVIC_IRQChannel                   = I2C1_EV_IRQn;
            NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
            NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
            NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
            NVIC_Init(&NVIC_InitStructure);

            NVIC_InitStructure.NVIC_IRQChannel = I2C1_ER_IRQn;
            NVIC_Init(&NVIC_InitStructure);
        }
        else
        {
            NVIC_InitStructure.NVIC_IRQChannel                   = I2C2_EV_IRQn;
            NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
            NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 1;
            NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
            NVIC_Init(&NVIC_InitStructure);

            NVIC_InitStructure.NVIC_IRQChannel = I2C2_ER_IRQn;
            NVIC_Init(&NVIC_InitStructure);
        }
        I2C_ITConfig(i2c, I2C_IT_BUF, ENABLE);
        I2C_ITConfig(i2c, I2C_IT_EVT, ENABLE);
        I2C_ITConfig(i2c, I2C_IT_ERR, ENABLE);
    }

    return 0;
}

/**
 * @brief 反初始化I2C控制器
 */
void i2c_deinit(i2c_num_t i2c_num)
{
    I2C_TypeDef *i2c;

    if (i2c_num >= sizeof(i2c_hw_config) / sizeof(i2c_hw_config[0]))
    {
        return;
    }

    i2c = get_i2c_periph(i2c_num);

    if (i2c == NULL)
    {
        return;
    }

    // 禁用I2C
    I2C_Cmd(i2c, DISABLE);

    // 复位I2C
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
}

/**
 * @brief I2C写数据
 */
int i2c_write(i2c_num_t i2c_num, uint8_t dev_addr, const uint8_t *data, uint16_t len)
{
    I2C_TypeDef *i2c;
    int ret;
    uint16_t i;

    if (data == NULL || len == 0)
    {
        return -1;
    }

    i2c = get_i2c_periph(i2c_num);
    if (i2c == NULL)
    {
        return -1;
    }

    // 等待总线空闲
    ret = wait_flag_timeout(i2c, I2C_FLAG_BUSY, RESET);
    if (ret != 0)
    {
        CommTimeOut_CallBack(MASTER_BUSY); // 总线忙
        goto error_cleanup;
    }

    // 发送START条件
    if (Comm_Flag == C_READY)
    {
        Comm_Flag = C_START_BIT;
        I2C_GenerateSTART(i2c, ENABLE);
    }

    // 等待EV5事件（START条件已发送）
    ret = wait_event_timeout(i2c, I2C_EVENT_MASTER_MODE_SELECT);
    if (ret != 0)
    {
        CommTimeOut_CallBack(MASTER_MODE); // 主模式错误
        goto error_cleanup;
    }

    // 发送设备地址（写模式）
    I2C_Send7bitAddress(i2c, dev_addr << 1, I2C_Direction_Transmitter);

    // 等待EV6事件（地址已发送，收到ACK）
    ret = wait_event_timeout(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED);
    if (ret != 0)
    {
        CommTimeOut_CallBack(MASTER_TXMODE);
        goto error_cleanup;
    }
    Comm_Flag = C_READY;
    // 发送数据
    for (i = 0; i < len; i++)
    {
        // 等待数据寄存器空
        ret = wait_flag_timeout(i2c, I2C_FLAG_TXE, SET);
        if (ret != 0)
        {
            CommTimeOut_CallBack(MASTER_SENDING);
            goto error_cleanup;
        }

        I2C_SendData(i2c, data[i]);
    }

    // 等待字节传输完成
    ret = wait_event_timeout(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED);
    if (ret != 0)
    {
        CommTimeOut_CallBack(MASTER_SENDED);
        goto error_cleanup;
    }

    // 发送STOP 条件
    if (Comm_Flag == C_READY)
    {
        Comm_Flag = C_STOP_BIT;
        I2C_GenerateSTOP(i2c, ENABLE);
    }
    // 等待总线空闲
    ret = wait_flag_timeout(i2c, I2C_FLAG_BUSY, RESET);
    if (ret != 0)
    {
        CommTimeOut_CallBack(MASTER_BUSY); // 总线忙
        goto error_cleanup;
    }

    Comm_Flag = C_READY; // 就绪

    return 0;

error_cleanup:
    I2C_GenerateSTOP(i2c, ENABLE);
    Comm_Flag = C_READY;

    return -1;
}

/**
 * @brief I2C读数据
 */
int i2c_read(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t *data, uint16_t len)
{
    I2C_TypeDef *i2c;
    int ret;
    uint16_t i;

    if (data == NULL || len == 0)
    {
        return -1;
    }

    i2c = get_i2c_periph(i2c_num);
    if (i2c == NULL)
    {
        return -1;
    }

    // 等待总线空闲
    ret = wait_flag_timeout(i2c, I2C_FLAG_BUSY, RESET);
    if (ret != 0)
    {
        CommTimeOut_CallBack(MASTER_BUSY); // 总线忙
        goto error_cleanup;
    }

    // 发送START条件
    if (Comm_Flag == C_READY)
    {
        Comm_Flag = C_START_BIT;
        I2C_GenerateSTART(i2c, ENABLE);
    }

    // 等待EV5事件
    ret = wait_event_timeout(i2c, I2C_EVENT_MASTER_MODE_SELECT);
    if (ret != 0)
    {
        CommTimeOut_CallBack(MASTER_MODE);
        goto error_cleanup;
    }

    // 发送设备地址（读模式）
    I2C_Send7bitAddress(i2c, dev_addr << 1, I2C_Direction_Receiver);

    // 等待EV6事件
    ret = wait_event_timeout(i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED);
    if (ret != 0)
    {
        CommTimeOut_CallBack(MASTER_TXMODE);
        goto error_cleanup;
    }
    Comm_Flag = C_READY;
    // 读取数据
    for (i = 0; i < len; i++)
    {
        if (i == len - 1)
        {
            // 最后一个字节，发送NACK
            I2C_AcknowledgeConfig(i2c, DISABLE);
        }

        // 等待数据就绪
        ret = wait_flag_timeout(i2c, I2C_FLAG_RXNE, SET);
        if (ret != 0)
        {
            CommTimeOut_CallBack(MASTER_RECVD);
            goto error_cleanup;
        }

        data[i] = I2C_ReceiveData(i2c);
    }

    // 发送STOP条件
    I2C_GenerateSTOP(i2c, ENABLE);

    // 等待总线空闲
    ret = wait_flag_timeout(i2c, I2C_FLAG_BUSY, RESET);
    if (ret != 0)
    {
        CommTimeOut_CallBack(MASTER_BUSY); // 总线忙
        goto error_cleanup;
    }
    Comm_Flag = C_READY;
    // 重新使能ACK
    I2C_AcknowledgeConfig(i2c, ENABLE);

    return 0;

error_cleanup:
    I2C_GenerateSTOP(i2c, ENABLE);
    Comm_Flag = C_READY;
    I2C_AcknowledgeConfig(i2c, ENABLE);

    return -1;
}

/**
 * @brief 写寄存器（先写寄存器地址，再写数据）
 */
int i2c_write_register(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t len)
{
    I2C_TypeDef *i2c;
    int ret;
    uint16_t i;

    if (data == NULL || len == 0)
    {
        return -1;
    }

    i2c = get_i2c_periph(i2c_num);
    if (i2c == NULL)
    {
        return -1;
    }

    // 等待总线空闲
    ret = wait_flag_timeout(i2c, I2C_FLAG_BUSY, RESET);
    if (ret != 0)
    {
        CommTimeOut_CallBack(MASTER_BUSY);
        goto error_cleanup;
    }

    // 发送START条件
    if (Comm_Flag == C_READY)
    {
        Comm_Flag = C_START_BIT;
        I2C_GenerateSTART(i2c, ENABLE);
    }
    // 等待EV5事件
    ret = wait_event_timeout(i2c, I2C_EVENT_MASTER_MODE_SELECT);
    if (ret != 0)
    {
        CommTimeOut_CallBack(MASTER_MODE); // 主模式错误
        goto error_cleanup;
    }

    // 发送设备地址（写模式）
    I2C_Send7bitAddress(i2c, dev_addr << 1, I2C_Direction_Transmitter);

    // 等待EV6事件
    ret = wait_event_timeout(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED);
    if (ret != 0)
    {
        CommTimeOut_CallBack(MASTER_TXMODE);
        goto error_cleanup;
    }
    Comm_Flag = C_READY;

    // 发送寄存器地址
    ret = wait_flag_timeout(i2c, I2C_FLAG_TXE, SET);
    if (ret != 0)
    {
        CommTimeOut_CallBack(MASTER_TXMODE);
        goto error_cleanup;
    }
    I2C_SendData(i2c, reg);

    // 发送数据
    for (i = 0; i < len; i++)
    {
        ret = wait_flag_timeout(i2c, I2C_FLAG_TXE, SET);
        if (ret != 0)
        {
            CommTimeOut_CallBack(MASTER_SENDING);
            goto error_cleanup;
        }
        I2C_SendData(i2c, data[i]);
    }

    // 等待传输完成
    ret = wait_event_timeout(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED);
    if (ret != 0)
    {
        CommTimeOut_CallBack(MASTER_SENDED);
        goto error_cleanup;
    }

    // 发送STOP条件
    if (Comm_Flag == C_READY)
    {
        Comm_Flag = C_STOP_BIT;
        I2C_GenerateSTOP(i2c, ENABLE);
    }
    // 等待总线空闲
    ret = wait_flag_timeout(i2c, I2C_FLAG_BUSY, RESET);
    if (ret != 0)
    {
        CommTimeOut_CallBack(MASTER_BUSY);
        goto error_cleanup;
    }
    Comm_Flag = C_READY;
    return 0;

error_cleanup:
    I2C_GenerateSTOP(i2c, ENABLE);
    Comm_Flag = C_READY;
    return -1;
}

/**
 * @brief 读寄存器（先写寄存器地址，再读数据）
 */
int i2c_read_register(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    I2C_TypeDef *i2c;
    int ret;

    if (data == NULL || len == 0)
    {
        return -1;
    }

    i2c = get_i2c_periph(i2c_num);
    if (i2c == NULL)
    {
        return -1;
    }

    // 先写寄存器地址
    ret = i2c_write(i2c_num, dev_addr, &reg, 1);
    if (ret != 0)
    {
        return ret;
    }

    // 然后读数据
    return i2c_read(i2c_num, dev_addr, data, len);
}

/**
 * @brief 写单个字节到寄存器
 */
int i2c_write_byte(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, uint8_t data)
{
    uint8_t buffer[2] = {reg, data};
    return i2c_write(i2c_num, dev_addr, buffer, 2);
}

/**
 * @brief 从寄存器读取单个字节
 */
int i2c_read_byte(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, uint8_t *data)
{
    return i2c_read_register(i2c_num, dev_addr, reg, data, 1);
}

/* ========================== 非阻塞异步接口 ========================== */

int i2c_write_async(i2c_num_t i2c_num, uint8_t dev_addr, const uint8_t *data, uint16_t len)
{
    I2C_TypeDef *i2c              = get_i2c_periph(i2c_num);
    const i2c_hw_config_t *hw_cfg = &i2c_hw_config[i2c_num];

    if (!i2c || !data || len == 0 || hw_cfg->mode != I2C_MODE_IT)
    {
        return -1;
    }

    // 检查总线是否空闲
    if (I2C_GetFlagStatus(i2c, I2C_FLAG_BUSY) == SET)
    {
        return -2; // 总线忙
    }

    i2c_async_ctx_t *ctx = &i2c_async_ctx[i2c_num];
    memset(ctx, 0, sizeof(i2c_async_ctx_t));
    ctx->dev_addr  = dev_addr;
    ctx->tx_buffer = data;
    ctx->tx_len    = len;
    ctx->rx_len    = 0;

    ctx->is_reg_write = false;


    I2C_GenerateSTART(i2c, ENABLE);
    return 0;
}

int i2c_read_async(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t *data, uint16_t len)
{
    I2C_TypeDef *i2c              = get_i2c_periph(i2c_num);
    const i2c_hw_config_t *hw_cfg = &i2c_hw_config[i2c_num];

    if (!i2c || !data || len == 0 || hw_cfg->mode != I2C_MODE_IT)
    {
        return -1;
    }

    // 检查总线是否空闲
    if (I2C_GetFlagStatus(i2c, I2C_FLAG_BUSY) == SET)
    {
        return -2; // 总线忙
    }

    i2c_async_ctx_t *ctx = &i2c_async_ctx[i2c_num];
    memset(ctx, 0, sizeof(i2c_async_ctx_t));
    ctx->dev_addr  = dev_addr;
    ctx->rx_buffer = data;
    ctx->rx_len    = len;
    ctx->tx_len    = 0;

    ctx->is_reg_write = false;


    I2C_GenerateSTART(i2c, ENABLE);
    return 0;
}

// 寄存器写（异步）
int i2c_write_register_async(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t len)
{
    I2C_TypeDef *i2c              = get_i2c_periph(i2c_num);
    const i2c_hw_config_t *hw_cfg = &i2c_hw_config[i2c_num];

    if (!i2c || !data || len == 0 || hw_cfg->mode != I2C_MODE_IT)
    {
        return -1;
    }

    // 检查总线是否空闲
    if (I2C_GetFlagStatus(i2c, I2C_FLAG_BUSY) == SET)
    {
        return -2; // 总线忙
    }

    i2c_async_ctx_t *ctx = &i2c_async_ctx[i2c_num];
    memset(ctx, 0, sizeof(i2c_async_ctx_t));

    // 使用专用缓冲区避免并发冲突
    i2c_async_write_buf[i2c_num][0] = reg;               // 第0字节：寄存器地址
    memcpy(&i2c_async_write_buf[i2c_num][1], data, len); // 后续字节：数据

    ctx->dev_addr  = dev_addr;
    ctx->tx_buffer = i2c_async_write_buf[i2c_num];
    ctx->tx_len    = len + 1; // 地址(1) + 数据(len)
    ctx->rx_len    = 0;

    ctx->is_reg_write = false; // 标记第一阶段是寄存器地址


    I2C_GenerateSTART(i2c, ENABLE);
    return 0;
}

// 寄存器读（异步）
int i2c_read_register_async(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    I2C_TypeDef *i2c              = get_i2c_periph(i2c_num);
    const i2c_hw_config_t *hw_cfg = &i2c_hw_config[i2c_num];

    if (!i2c || !data || len == 0 || hw_cfg->mode != I2C_MODE_IT)
    {
        return -1;
    }

    // 检查总线是否空闲
    if (I2C_GetFlagStatus(i2c, I2C_FLAG_BUSY) == SET)
    {
        return -2; // 总线忙
    }

    i2c_async_ctx_t *ctx = &i2c_async_ctx[i2c_num];
    memset(ctx, 0, sizeof(i2c_async_ctx_t));
    // 使用专用缓冲区避免并发冲突
    i2c_async_reg_buf[i2c_num][0] = reg;

    ctx->dev_addr     = dev_addr;
    ctx->tx_buffer    = i2c_async_reg_buf[i2c_num];
    ctx->tx_len       = 1;
    ctx->rx_len       = len; // 注意：这里设置 rx_len 为要读取的长度
    ctx->rx_buffer    = data;
    ctx->is_reg_write = true; // 第一阶段：写寄存器地址


    I2C_GenerateSTART(i2c, ENABLE);
    return 0;
}

// 单字节简化接口
int i2c_write_byte_async(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg, uint8_t data)
{
    // 使用专用缓冲区避免并发冲突
    i2c_async_byte_buf[i2c_num][0] = reg;
    i2c_async_byte_buf[i2c_num][1] = data;
    return i2c_write_async(i2c_num, dev_addr, i2c_async_byte_buf[i2c_num], 2);
}

int i2c_read_byte_async(i2c_num_t i2c_num, uint8_t dev_addr, uint8_t reg)
{
    static uint8_t dummy;
    return i2c_read_register_async(i2c_num, dev_addr, reg, &dummy, 1);
}

/***************************************** I2C工具函数 ***************************************** */
/**
 * @brief 检查I2C总线是否繁忙
 */
bool i2c_is_busy(i2c_num_t i2c_num)
{
    I2C_TypeDef *i2c = get_i2c_periph(i2c_num);
    if (i2c == NULL)
        return true;
    return I2C_GetFlagStatus(i2c, I2C_FLAG_BUSY) == SET;
}

/**
 * @brief 检查设备是否存在
 */
int i2c_check_device(i2c_num_t i2c_num, uint8_t dev_addr)
{
    I2C_TypeDef *i2c;
    int ret;

    i2c = get_i2c_periph(i2c_num);
    if (i2c == NULL)
    {
        return -1;
    }

    // 等待总线空闲
    ret = wait_flag_timeout(i2c, I2C_FLAG_BUSY, RESET);
    if (ret != 0)
        return ret;

    // 发送START条件
    I2C_GenerateSTART(i2c, ENABLE);

    // 等待EV5事件
    ret = wait_event_timeout(i2c, I2C_EVENT_MASTER_MODE_SELECT);
    if (ret != 0)
        return ret;

    // 发送设备地址
    I2C_Send7bitAddress(i2c, dev_addr << 1, I2C_Direction_Transmitter);

    // 检查是否收到ACK
    ret = wait_event_timeout(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED);

    // 发送STOP条件
    I2C_GenerateSTOP(i2c, ENABLE);

    return (ret == 0) ? 0 : -1; // 0表示设备存在
}
/**
 * @brief 探测指定的I2C地址是否有设备响应
 * @param I2Cx I2C外设指针
 * @param dev_addr 7位设备地址
 * @return 0:设备存在, 1:设备不存在, 2:总线忙, 3:其他错误
 */
uint8_t I2C_Probe_Address(I2C_TypeDef *I2Cx, uint8_t dev_addr)
{
    uint32_t timeout            = 10000; // 超时计数
    volatile uint32_t temp_time = 0;

    // 1. 等待总线空闲
    while (I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY))
    {
        if (timeout-- == 0)
            return 2; // 总线忙超时
    }

    // 2. 发送起始条件
    I2C_GenerateSTART(I2Cx, ENABLE);
    timeout = 10000;
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT))
    {
        if (timeout-- == 0)
        {
            I2C_GenerateSTOP(I2Cx, ENABLE);
            return 3; // 起始条件失败
        }
    }

    // 3. 发送设备地址(写)
    I2C_Send7bitAddress(I2Cx, dev_addr, I2C_Direction_Transmitter);

    // 4. 等待ACK
    timeout = 10000;
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
        // 检查是否有NACK
        if (I2C_GetFlagStatus(I2Cx, I2C_FLAG_AF))
        {
            // 清除AF标志
            I2C_ClearFlag(I2Cx, I2C_FLAG_AF);
            I2C_GenerateSTOP(I2Cx, ENABLE);

            // 等待STOP条件完成
            temp_time = 10000;
            while (temp_time--)
                ;

            return 1; // 设备无响应(NACK)
        }

        if (timeout-- == 0)
        {
            I2C_GenerateSTOP(I2Cx, ENABLE);
            return 3; // 超时
        }
    }

    // 5. 发送停止条件(设备存在)
    I2C_GenerateSTOP(I2Cx, ENABLE);

    // 6. 短暂延时
    temp_time = 10000;
    while (temp_time--)
        ;

    return 0; // 设备存在
}
/**
 * @brief 扫描I2C总线上的设备
 * @param I2Cx I2C外设指针(I2C1, I2C2等)
 * @param 打印结果(1=打印,0=不打印)
 * @return 找到的设备数量
 */
uint8_t I2C_Scan(I2C_TypeDef *I2Cx, uint8_t print)
{
    uint8_t found_devices = 0;
    uint8_t i, ret;
    // uint8_t test_byte = 0x00;

    if (print)
    {
        printf("\r\nStarting I2C bus scan...\r\n");
        printf("I2C Address Range: 0x08 - 0x77 (excluding reserved addresses)\r\n");
        printf("Scanning...\r\n");
    }

    // 遍历所有可能的I2C地址(7位地址)
    // 注意: 0x00-0x07和0x78-0x7F是保留地址
    for (i = 0x08; i <= 0x77; i++)
    {
        ret = I2C_Probe_Address(I2Cx, i << 1);

        if (ret == 0) // 设备响应
        {
            found_devices++;

            if (print)
            {
                printf("  Device found at address: 0x%02X (0x%02X for write, 0x%02X for read)\r\n", i, i << 1,
                       (i << 1) | 0x01);
            }
        }
    }

    if (print)
    {
        printf("\r\nScan complete. Found %d device(s).\r\n\r\n", found_devices);
    }

    return found_devices;
}

/***************************************** I2C设备通信错误回调 ***************************************** */
/**
 * @brief  通信超时回调函数
 * @param errcode 错误代码
 *
 * 处理I2C通信超时错误，根据配置的恢复模式进行相应处理
 */
static void CommTimeOut_CallBack(i2c_ErrCode_t errcode)
{
    // 将通信标志设置为就绪状态，表示可以进行新的通信
    Comm_Flag = C_READY;

    // 根据错误代码执行不同的处理逻辑（预留）
    switch (errcode)
    {
    case MASTER_BUSY:
        // 主设备忙错误处理
        // TODO: 添加具体处理逻辑
        break;

    case MASTER_MODE:
        // 主模式错误处理
        // TODO: 添加具体处理逻辑
        break;

    case MASTER_TXMODE:
        // 主发送模式错误处理
        // TODO: 添加具体处理逻辑
        break;

    case MASTER_RXMODE:
        // 主接收模式错误处理
        // TODO: 添加具体处理逻辑
        break;

    case MASTER_SENDING:
        // 主发送中错误处理
        // TODO: 添加具体处理逻辑
        break;

    case MASTER_SENDED:
        // 主发送完成错误处理
        // TODO: 添加具体处理逻辑
        break;

    case MASTER_RECVD:
        // 主接收错误处理
        // TODO: 添加具体处理逻辑
        break;

    case MASTER_BYTEF:
        // 主字节完成错误处理
        // TODO: 添加具体处理逻辑
        break;

    case MASTER_BUSERR:
        // 主总线错误处理
        // TODO: 添加具体处理逻辑
        break;

    case MASTER_UNKNOW:
        // 主未知错误处理
        // TODO: 添加具体处理逻辑
        break;

    case SLAVE_BUSY:
        // 从设备忙错误处理
        // TODO: 添加具体处理逻辑
        break;

    case SLAVE_MODE:
        // 从模式错误处理
        // TODO: 添加具体处理逻辑
        break;

    case SLAVE_BUSERR:
        // 从总线错误处理
        // TODO: 添加具体处理逻辑
        break;

    case SLAVE_UNKNOW:
        // 从未知错误处理
        // TODO: 添加具体处理逻辑
        break;

    default:
        // 默认错误处理
        // TODO: 添加具体处理逻辑
        break;
    }

    // 根据配置的恢复模式执行相应的复位操作
#if (COMM_RECOVER_MODE == MODULE_SELF_RESET)
    // 模块自复位模式
    // TODO: 实现具体的模块自复位逻辑
     IIC_SWReset();
#elif (COMM_RECOVER_MODE == MODULE_RCC_RESET)
    // 模块时钟复位模式
    IIC_RCCReset();
#elif (COMM_RECOVER_MODE == SYSTEM_NVIC_RESET)
    // 系统NVIC复位模式
    SystemNVICReset();
#endif
}

// ==================== I2C 中断处理 ====================

/**
 * @brief I2C设备通信中断处理函数
 * @param i2c I2C外设指针(I2C1, I2C2等)
 * @param num I2C设备编号(I2C1, I2C2等)
 */
static void I2C_EV_IRQHandler_Handler(I2C_TypeDef *i2c, i2c_num_t num)
{
    i2c_async_ctx_t *ctx = &i2c_async_ctx[num];

    // 1. START 条件 (SB)
    if (I2C_GetITStatus(i2c, I2C_IT_SB) != RESET)
    {
        if (ctx->is_reg_write)
        {
            // 发送设备地址（写模式）
            I2C_Send7bitAddress(i2c, ctx->dev_addr << 1, I2C_Direction_Transmitter);
        }
        else if (ctx->rx_len > 0)
        {
            // 发送设备地址（读模式）
            I2C_Send7bitAddress(i2c, ctx->dev_addr << 1, I2C_Direction_Receiver);
        }
        else
        {
            I2C_Send7bitAddress(i2c, ctx->dev_addr << 1, I2C_Direction_Transmitter);
        }
    }
    // 2. 地址发送完成 (ADDR)
    else if (I2C_GetITStatus(i2c, I2C_IT_ADDR) != RESET)
    {
        // 清 ADDR 标志
        volatile uint16_t temp = i2c->STAR1;
        (void)temp;
        temp = i2c->STAR2;
        (void)temp;

        // 如果是接收模式且只剩1字节，提前禁用 ACK
        if (ctx->rx_len == 1 && !ctx->is_reg_write)
        {
            I2C_AcknowledgeConfig(i2c, DISABLE);
        }
    }
    // 3. 发送缓冲区空中断 (TXE)
    else if (I2C_GetITStatus(i2c, I2C_IT_TXE) != RESET)
    {
        if (ctx->tx_len > 0)
        {
            I2C_SendData(i2c, *(ctx->tx_buffer++));
            ctx->tx_len--;
        }
        else
        {
            // 所有 TX 数据已发送完毕
            if (ctx->rx_len > 0 && ctx->is_reg_write)
            {
                // （寄存器读）寄存器地址写入完成，准备发送重复 START 进行读操作
                ctx->is_reg_write = false;
                I2C_GenerateSTART(i2c, ENABLE); // 触发 Repeated Start
            }
            else
            {
                // 普通写操作
                I2C_GenerateSTOP(i2c, ENABLE);
                i2c_master_tx_cplt_callback(num);
            }
        }
    }
    // 4. 接收缓冲区非空 (RXNE)
    else if (I2C_GetITStatus(i2c, I2C_IT_RXNE) != RESET)
    {
        if (ctx->rx_len > 0)
        {
            uint8_t data        = I2C_ReceiveData(i2c);
            *(ctx->rx_buffer++) = data;
            ctx->rx_len--;

            if (ctx->rx_len == 0)
            {
                // 所有数据接收完毕
                I2C_GenerateSTOP(i2c, ENABLE);
                I2C_AcknowledgeConfig(i2c, ENABLE); // 恢复 ACK
                if (ctx->rx_buffer != NULL)
                { // 这是一个读操作
                    i2c_master_rx_cplt_callback(num);
                }
            }
            else if (ctx->rx_len == 1)
            {
                // 下一个字节是最后一个，提前发送 NACK
                I2C_AcknowledgeConfig(i2c, DISABLE);
            }
        }
    }
    // 5. 字节传输完成 (BTF)
    else if (I2C_GetITStatus(i2c, I2C_IT_BTF) != RESET)
    {
        // BTF 清除：读 STAR1 + 读/写 DATAR
        volatile uint16_t temp = i2c->STAR1;
        (void)temp;
        // 如果是写操作且 DR 为空，可以写下一个字节
    }
}

static void I2C_ER_IRQHandler_Handler(I2C_TypeDef *i2c, i2c_num_t num)
{
    i2c_async_ctx_t *ctx   = &i2c_async_ctx[num];
    i2c_ErrCode_t err_code = MASTER_UNKNOW;

    // 1. ACK Failure (从机无响应)
    if (I2C_GetFlagStatus(i2c, I2C_FLAG_AF) != RESET)
    {
        I2C_ClearFlag(i2c, I2C_FLAG_AF);
        I2C_GenerateSTOP(i2c, ENABLE);
        err_code = MASTER_TXMODE; // 或 MASTER_RXMODE，可根据上下文细化
    }
    // 2. 总线错误（如仲裁丢失、非法 START/STOP）
    else if (I2C_GetFlagStatus(i2c, I2C_FLAG_BERR) != RESET)
    {
        I2C_ClearFlag(i2c, I2C_FLAG_BERR);
        I2C_GenerateSTOP(i2c, ENABLE);
        err_code = MASTER_BUSERR;
    }
    // 3. Arbitration Lost
    else if (I2C_GetFlagStatus(i2c, I2C_FLAG_ARLO) != RESET)
    {
        I2C_ClearFlag(i2c, I2C_FLAG_ARLO);
        err_code = MASTER_BUSERR;
    }

    // 错误回调
    CommTimeOut_CallBack(err_code);

    // 用户回调
    i2c_error_callback(num, err_code);

    // 重置
    memset(ctx, 0, sizeof(i2c_async_ctx_t));
}

void I2C1_EV_IRQHandler(void) { I2C_EV_IRQHandler_Handler(I2C1, I2C_NUM_1); }
void I2C1_ER_IRQHandler(void) { I2C_ER_IRQHandler_Handler(I2C1, I2C_NUM_1); }
void I2C2_EV_IRQHandler(void) { I2C_EV_IRQHandler_Handler(I2C2, I2C_NUM_2); }
void I2C2_ER_IRQHandler(void) { I2C_ER_IRQHandler_Handler(I2C2, I2C_NUM_2); }

// 默认的弱函数实现

/**
 * @brief I2C设备通信完成回调函数
 * @param i2c_num I2C设备编号(I2C1, I2C2等)
 */
__attribute__((weak)) void i2c_master_tx_cplt_callback(i2c_num_t i2c_num)
{
    // 默认空实现，用户可重写
    (void)i2c_num;
}
/**
 * @brief I2C设备通信完成回调函数
 * @param i2c_num I2C设备编号(I2C1, I2C2等)
 */
__attribute__((weak)) void i2c_master_rx_cplt_callback(i2c_num_t i2c_num) { (void)i2c_num; }
/**
 * @brief I2C设备通信错误回调函数
 * @param i2c_num I2C设备编号(I2C1, I2C2等)
 * @param errcode 错误码
 */
__attribute__((weak)) void i2c_error_callback(i2c_num_t i2c_num, i2c_ErrCode_t errcode)
{
    (void)i2c_num;
    (void)errcode;
}
