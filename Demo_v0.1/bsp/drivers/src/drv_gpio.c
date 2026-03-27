#include "drv_gpio.h"
// #include "board.h"

/* ========== 使用新的宏定义生成引脚映射表 ========== */

static const struct gpio_pin_index gpio_pins[] = {
/* 使用GET_PIN宏生成引脚映射，确保与RT-Thread兼容 */
#if defined(GPIOA)
    {GET_PIN(A, 0), GPIOA, GPIO_Pin_0, "PA0"},
    {GET_PIN(A, 1), GPIOA, GPIO_Pin_1, "PA1"},
    {GET_PIN(A, 2), GPIOA, GPIO_Pin_2, "PA2"},
    {GET_PIN(A, 3), GPIOA, GPIO_Pin_3, "PA3"},
    {GET_PIN(A, 4), GPIOA, GPIO_Pin_4, "PA4"},
    {GET_PIN(A, 5), GPIOA, GPIO_Pin_5, "PA5"},
    {GET_PIN(A, 6), GPIOA, GPIO_Pin_6, "PA6"},
    {GET_PIN(A, 7), GPIOA, GPIO_Pin_7, "PA7"},
    {GET_PIN(A, 8), GPIOA, GPIO_Pin_8, "PA8"},
    {GET_PIN(A, 9), GPIOA, GPIO_Pin_9, "PA9"},
    {GET_PIN(A, 10), GPIOA, GPIO_Pin_10, "PA10"},
    {GET_PIN(A, 11), GPIOA, GPIO_Pin_11, "PA11"},
    {GET_PIN(A, 12), GPIOA, GPIO_Pin_12, "PA12"},
    {GET_PIN(A, 13), GPIOA, GPIO_Pin_13, "PA13"},
    {GET_PIN(A, 14), GPIOA, GPIO_Pin_14, "PA14"},
    {GET_PIN(A, 15), GPIOA, GPIO_Pin_15, "PA15"},
#endif
#if defined(GPIOB)
    {GET_PIN(B, 0), GPIOB, GPIO_Pin_0, "PB0"},
    {GET_PIN(B, 1), GPIOB, GPIO_Pin_1, "PB1"},
    {GET_PIN(B, 2), GPIOB, GPIO_Pin_2, "PB2"},
    {GET_PIN(B, 3), GPIOB, GPIO_Pin_3, "PB3"},
    {GET_PIN(B, 4), GPIOB, GPIO_Pin_4, "PB4"},
    {GET_PIN(B, 5), GPIOB, GPIO_Pin_5, "PB5"},
    {GET_PIN(B, 6), GPIOB, GPIO_Pin_6, "PB6"},
    {GET_PIN(B, 7), GPIOB, GPIO_Pin_7, "PB7"},
    {GET_PIN(B, 8), GPIOB, GPIO_Pin_8, "PB8"},
    {GET_PIN(B, 9), GPIOB, GPIO_Pin_9, "PB9"},
    {GET_PIN(B, 10), GPIOB, GPIO_Pin_10, "PB10"},
    {GET_PIN(B, 11), GPIOB, GPIO_Pin_11, "PB11"},
    {GET_PIN(B, 12), GPIOB, GPIO_Pin_12, "PB12"},
    {GET_PIN(B, 13), GPIOB, GPIO_Pin_13, "PB13"},
    {GET_PIN(B, 14), GPIOB, GPIO_Pin_14, "PB14"},
    {GET_PIN(B, 15), GPIOB, GPIO_Pin_15, "PB15"},
#endif
#if defined(GPIOC)
    {GET_PIN(C, 0), GPIOC, GPIO_Pin_0, "PC0"},
    {GET_PIN(C, 1), GPIOC, GPIO_Pin_1, "PC1"},
    {GET_PIN(C, 2), GPIOC, GPIO_Pin_2, "PC2"},
    {GET_PIN(C, 3), GPIOC, GPIO_Pin_3, "PC3"},
    {GET_PIN(C, 4), GPIOC, GPIO_Pin_4, "PC4"},
    {GET_PIN(C, 5), GPIOC, GPIO_Pin_5, "PC5"},
    {GET_PIN(C, 6), GPIOC, GPIO_Pin_6, "PC6"},
    {GET_PIN(C, 7), GPIOC, GPIO_Pin_7, "PC7"},
    {GET_PIN(C, 8), GPIOC, GPIO_Pin_8, "PC8"},
    {GET_PIN(C, 9), GPIOC, GPIO_Pin_9, "PC9"},
    {GET_PIN(C, 10), GPIOC, GPIO_Pin_10, "PC10"},
    {GET_PIN(C, 11), GPIOC, GPIO_Pin_11, "PC11"},
    {GET_PIN(C, 12), GPIOC, GPIO_Pin_12, "PC12"},
    {GET_PIN(C, 13), GPIOC, GPIO_Pin_13, "PC13"},
    {GET_PIN(C, 14), GPIOC, GPIO_Pin_14, "PC14"},
    {GET_PIN(C, 15), GPIOC, GPIO_Pin_15, "PC15"},
#endif
#if defined(GPIOD)
    {GET_PIN(D, 0), GPIOD, GPIO_Pin_0, "PD0"},
    {GET_PIN(D, 1), GPIOD, GPIO_Pin_1, "PD1"},
    {GET_PIN(D, 2), GPIOD, GPIO_Pin_2, "PD2"},
    {GET_PIN(D, 3), GPIOD, GPIO_Pin_3, "PD3"},
    {GET_PIN(D, 4), GPIOD, GPIO_Pin_4, "PD4"},
    {GET_PIN(D, 5), GPIOD, GPIO_Pin_5, "PD5"},
    {GET_PIN(D, 6), GPIOD, GPIO_Pin_6, "PD6"},
    {GET_PIN(D, 7), GPIOD, GPIO_Pin_7, "PD7"},
    {GET_PIN(D, 8), GPIOD, GPIO_Pin_8, "PD8"},
    {GET_PIN(D, 9), GPIOD, GPIO_Pin_9, "PD9"},
    {GET_PIN(D, 10), GPIOD, GPIO_Pin_10, "PD10"},
    {GET_PIN(D, 11), GPIOD, GPIO_Pin_11, "PD11"},
    {GET_PIN(D, 12), GPIOD, GPIO_Pin_12, "PD12"},
    {GET_PIN(D, 13), GPIOD, GPIO_Pin_13, "PD13"},
    {GET_PIN(D, 14), GPIOD, GPIO_Pin_14, "PD14"},
    {GET_PIN(D, 15), GPIOD, GPIO_Pin_15, "PD15"},
#endif
#if defined(GPIOE)
    {GET_PIN(E, 0), GPIOE, GPIO_Pin_0, "PE0"},
    {GET_PIN(E, 1), GPIOE, GPIO_Pin_1, "PE1"},
    {GET_PIN(E, 2), GPIOE, GPIO_Pin_2, "PE2"},
    {GET_PIN(E, 3), GPIOE, GPIO_Pin_3, "PE3"},
    {GET_PIN(E, 4), GPIOE, GPIO_Pin_4, "PE4"},
    {GET_PIN(E, 5), GPIOE, GPIO_Pin_5, "PE5"},
    {GET_PIN(E, 6), GPIOE, GPIO_Pin_6, "PE6"},
    {GET_PIN(E, 7), GPIOE, GPIO_Pin_7, "PE7"},
    {GET_PIN(E, 8), GPIOE, GPIO_Pin_8, "PE8"},
    {GET_PIN(E, 9), GPIOE, GPIO_Pin_9, "PE9"},
    {GET_PIN(E, 10), GPIOE, GPIO_Pin_10, "PE10"},
    {GET_PIN(E, 11), GPIOE, GPIO_Pin_11, "PE11"},
    {GET_PIN(E, 12), GPIOE, GPIO_Pin_12, "PE12"},
    {GET_PIN(E, 13), GPIOE, GPIO_Pin_13, "PE13"},
    {GET_PIN(E, 14), GPIOE, GPIO_Pin_14, "PE14"},
    {GET_PIN(E, 15), GPIOE, GPIO_Pin_15, "PE15"},
#endif
    /* 可以继续添加其他端口... */
};

/* 计算引脚映射表大小 */
#define GPIO_PIN_COUNT (sizeof(gpio_pins) / sizeof(gpio_pins[0]))

/* ========== 工具函数 ========== */
/// 将引脚位转换为索引
static IRQn_Type get_exti_irqn(uint8_t pin_index)
{
    if (pin_index < 5)
    {
        return (IRQn_Type)(EXTI0_IRQn + pin_index);
    }
    else if (pin_index < 10)
    {
        return EXTI9_5_IRQn;
    }
    else
    {
        return EXTI15_10_IRQn;
    }
}
// 获取GPIO端口源 - 用于AFIO配置
static uint8_t get_gpio_port_source(GPIO_TypeDef *gpio)
{
    if (gpio == GPIOA)
        return GPIO_PortSourceGPIOA;
    if (gpio == GPIOB)
        return GPIO_PortSourceGPIOB;
    if (gpio == GPIOC)
        return GPIO_PortSourceGPIOC;
    if (gpio == GPIOD)
        return GPIO_PortSourceGPIOD;

    return 0xFF; // 无效端口
}
/* 根据引脚号查找引脚信息 - 优化版本 */
static const struct gpio_pin_index *gpio_get_pin(gpio_pin_t pin)
{
    /* 由于GET_PIN宏生成的引脚号是连续的，我们可以直接使用索引查找 */
    if (pin < GPIO_PIN_COUNT)
    {
        const struct gpio_pin_index *index = &gpio_pins[pin];
        /* 检查引脚是否有效 */
        if (index->gpio != NULL)
        {
            return index;
        }
    }
    return NULL;
}
/* 获取引脚名称函数 */
const char *gpio_get_pin_name(gpio_pin_t pin)
{
    const struct gpio_pin_index *index;

    index = gpio_get_pin(pin);
    if (index == NULL)
    {
        return "UNKNOWN";
    }

    return index->name;
}
/* 位值转换为位索引 - 修复返回类型 */
static uint8_t gpio_bit_to_index(uint16_t bit)
{
    for (uint8_t i = 0; i < 16; i++)
    {
        if ((1U << i) == bit)
        {
            return i;
        }
    }
    return 0xFF; /* 无效索引 */
}

/* ========== GPIO基础API实现 ========== */

/* GPIO驱动初始化 */
gpio_err_t gpio_init(void)
{
    /* 初始化时钟等硬件资源 */
    /* 这里可以初始化所有GPIO端口的时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    return GPIO_OK;
}

/* 设置GPIO模式 */
gpio_err_t gpio_mode(gpio_pin_t pin, pin_mode_t mode)
{
    const struct gpio_pin_index *index;
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    index = gpio_get_pin(pin);
    if (index == NULL)
    {
        return GPIO_EINVAL;
    }

    /* 配置GPIO引脚 */
    GPIO_InitStruct.GPIO_Pin = index->pin_bit;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;

    switch (mode)
    {
    case PIN_MODE_OUTPUT:
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;

        break;

    case PIN_MODE_INPUT:
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPD;

        break;

    case PIN_MODE_INPUT_PULLUP:
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;

        break;

    case PIN_MODE_INPUT_PULLDOWN:
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPD;

        break;

    case PIN_MODE_OUTPUT_OD:
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_OD;

        break;

    default:
        return GPIO_EINVAL;
    }
    GPIO_Init(index->gpio, &GPIO_InitStruct);
    return GPIO_OK;
}

/* 写GPIO引脚 */
void gpio_write(gpio_pin_t pin, DRV_GPIO_PinState value)
{
    const struct gpio_pin_index *index;

    index = gpio_get_pin(pin);
    if (index == NULL)
    {
        return;
    }
    BitAction action = (value == GPIO_PIN_SET) ? Bit_SET : Bit_RESET;
    GPIO_WriteBit(index->gpio, index->pin_bit, action);

}

/* 读GPIO引脚 */
DRV_GPIO_PinState gpio_read(gpio_pin_t pin)
{
    const struct gpio_pin_index *index;

    index = gpio_get_pin(pin);
    if (index == NULL)
    {
        return GPIO_PIN_RESET;
    }

    DRV_GPIO_PinState state = GPIO_ReadInputDataBit(index->gpio, index->pin_bit);
    return (state == GPIO_PIN_SET) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

/* 翻转GPIO引脚 */
gpio_err_t gpio_toggle(gpio_pin_t pin)
{
    const struct gpio_pin_index *index;

    index = gpio_get_pin(pin);
    if (index == NULL)
    {
        return GPIO_EINVAL;
    }

    if (((index->gpio)->INDR & index->pin_bit) != 0x00u)
    {
        (index->gpio)->BCR = (uint32_t)index->pin_bit;
    }
    else
    {
        (index->gpio)->BSHR = (uint32_t)index->pin_bit;
    }

    return GPIO_OK;
}

/* ========== 中断处理API ========== */

/* 中断映射表 */
struct pin_irq_map
{
    uint16_t pin_bit;
    IRQn_Type irqno;
};

/* 中断处理程序表 */
static struct
{
    gpio_pin_t pin;
    uint8_t mode;
    void (*hdr)(void *args);
    void *args;
    uint8_t enabled;
} gpio_irq_handlers[16];

static uint32_t gpio_irq_enable_mask = 0;

/* 绑定中断处理函数 */
gpio_err_t gpio_attach_irq(gpio_pin_t pin, pin_irq_mode_t mode,
                           void (*hdr)(void *args), void *args)
{
    const struct gpio_pin_index *index;
    uint8_t irq_index;

    if (hdr == NULL)
    {
        return GPIO_EINVAL;
    }

    index = gpio_get_pin(pin);
    if (index == NULL)
    {
        return GPIO_EINVAL;
    }

    irq_index = gpio_bit_to_index(index->pin_bit);
    if (irq_index >= 16)
    {
        return GPIO_EINVAL;
    }

    /* 临界区保护 */
    __disable_irq();

    /* 注册中断处理程序 */
    gpio_irq_handlers[irq_index].pin = pin;
    gpio_irq_handlers[irq_index].mode = (uint8_t)mode;
    gpio_irq_handlers[irq_index].hdr = hdr;
    gpio_irq_handlers[irq_index].args = args;
    gpio_irq_handlers[irq_index].enabled = 0;

    __enable_irq();

    return GPIO_OK;
}

/* 解绑中断处理函数 */
gpio_err_t gpio_detach_irq(gpio_pin_t pin)
{
    const struct gpio_pin_index *index;
    uint8_t irq_index;

    index = gpio_get_pin(pin);
    if (index == NULL)
    {
        return GPIO_EINVAL;
    }

    irq_index = gpio_bit_to_index(index->pin_bit);
    if (irq_index >= 16)
    {
        return GPIO_EINVAL;
    }

    /* 临界区保护 */
    __disable_irq();

    /* 清除中断处理程序 */
    gpio_irq_handlers[irq_index].pin = 0xFFFF;
    gpio_irq_handlers[irq_index].hdr = NULL;
    gpio_irq_handlers[irq_index].mode = 0;
    gpio_irq_handlers[irq_index].args = NULL;
    gpio_irq_handlers[irq_index].enabled = 0;

    __enable_irq();

    return GPIO_OK;
}

/* 使能/禁用中断 */
gpio_err_t gpio_irq_enable(gpio_pin_t pin, gpio_irq_enable_t enabled)
{
    const struct gpio_pin_index *index;
    uint8_t irq_index;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    EXTI_InitTypeDef EXTI_InitStructure = {0};

    index = gpio_get_pin(pin);
    if (index == NULL)
    {
        return GPIO_EINVAL;
    }

    irq_index = gpio_bit_to_index(index->pin_bit);
    if (irq_index >= 16)
    {
        return GPIO_EINVAL;
    }

    /* 临界区保护 */
    __disable_irq();

    if (enabled == GPIO_IRQ_ENABLE)
    {
        if (gpio_irq_handlers[irq_index].hdr == NULL)
        {
            __enable_irq();
            return GPIO_EINVAL;
        }

        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

        /* 配置GPIO为中断模式 */
        GPIO_InitStruct.GPIO_Pin = index->pin_bit;
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;

        EXTI_InitStructure.EXTI_Line = index->pin_bit;
        EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
        EXTI_InitStructure.EXTI_LineCmd = ENABLE;

        switch (gpio_irq_handlers[irq_index].mode)
        {
        case PIN_IRQ_MODE_RISING:
            GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPD;
            EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
            break;
        case PIN_IRQ_MODE_FALLING:
            GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
            EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
            break;
        case PIN_IRQ_MODE_RISING_FALLING:
            GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
            EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
            break;
        default:
            __enable_irq();
            return GPIO_EINVAL;
        }

        GPIO_Init(index->gpio, &GPIO_InitStruct);

        uint8_t gpio_port_source = get_gpio_port_source(index->gpio);
        if (gpio_port_source == 0xFF)
        {
            __enable_irq();
            return GPIO_EINVAL;
        }
        GPIO_EXTILineConfig(gpio_port_source, (uint8_t)irq_index);

        EXTI_Init(&EXTI_InitStructure);

        IRQn_Type irqn = get_exti_irqn(irq_index);
        NVIC_SetPriority(irqn, 5);
        NVIC_EnableIRQ(irqn);

        gpio_irq_handlers[irq_index].enabled = 1;
        gpio_irq_enable_mask |= (1UL << irq_index); // 只使用一个掩码
    }
    else if (enabled == GPIO_IRQ_DISABLE)
    {
        uint8_t irq_idx = gpio_bit_to_index(index->pin_bit);

        gpio_irq_handlers[irq_idx].enabled = 0;
        gpio_irq_enable_mask &= ~(1UL << irq_idx);

        /* 检查是否需要禁用NVIC中断 */
        if (irq_idx < 5)
        {
            // 只有当该中断线上没有其他使能的引脚时才禁用NVIC
            if ((gpio_irq_enable_mask & (1UL << irq_idx)) == 0)
            {
                NVIC_DisableIRQ(get_exti_irqn(irq_idx));
            }
        }
        else if (irq_idx < 10)
        {
            // 检查5-9线组中是否还有其他中断使能
            if ((gpio_irq_enable_mask & 0x3E0) == 0) // 检查bit 5-9
            {
                NVIC_DisableIRQ(EXTI9_5_IRQn);
            }
        }
        else
        {
            // 检查10-15线组中是否还有其他中断使能
            if ((gpio_irq_enable_mask & 0xFC00) == 0) // 检查bit 10-15
            {
                NVIC_DisableIRQ(EXTI15_10_IRQn);
            }
        }
    }

    __enable_irq();
    return GPIO_OK;
}

/* ========== 中断服务程序 ========== */

static void gpio_irq_handler(uint8_t irq_index)
{
    if (irq_index < 16 && gpio_irq_handlers[irq_index].hdr != NULL)
    {
        gpio_irq_handlers[irq_index].hdr(gpio_irq_handlers[irq_index].args);
    }
}

/* 具体的中断服务程序 */
void EXTI0_IRQHandler(void)
{
    GET_INT_SP();
    if (EXTI_GetITStatus(EXTI_Line0) != RESET)
    {
        gpio_irq_handler(0);
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
    FREE_INT_SP();
}

void EXTI1_IRQHandler(void)
{
    GET_INT_SP();
    if (EXTI_GetITStatus(EXTI_Line1) != RESET)
    {
        gpio_irq_handler(1);
        EXTI_ClearITPendingBit(EXTI_Line1);
    }
    FREE_INT_SP();
}

void EXTI2_IRQHandler(void)
{
    GET_INT_SP();
    if (EXTI_GetITStatus(EXTI_Line2) != RESET)
    {
        gpio_irq_handler(2);
        EXTI_ClearITPendingBit(EXTI_Line2);
    }
    FREE_INT_SP();
}

void EXTI3_IRQHandler(void)
{
    GET_INT_SP();
    if (EXTI_GetITStatus(EXTI_Line3) != RESET)
    {
        gpio_irq_handler(3);
        EXTI_ClearITPendingBit(EXTI_Line3);
    }
    FREE_INT_SP();
}

void EXTI4_IRQHandler(void)
{
    GET_INT_SP();
    if (EXTI_GetITStatus(EXTI_Line4) != RESET)
    {
        gpio_irq_handler(4);
        EXTI_ClearITPendingBit(EXTI_Line4);
    }
    FREE_INT_SP();
}

void EXTI9_5_IRQHandler(void)
{
    GET_INT_SP();
    if (EXTI_GetITStatus(EXTI_Line5) != RESET)
    {
        gpio_irq_handler(5);
        EXTI_ClearITPendingBit(EXTI_Line5);
    }
    if (EXTI_GetITStatus(EXTI_Line6) != RESET)
    {
        gpio_irq_handler(6);
        EXTI_ClearITPendingBit(EXTI_Line6);
    }
    if (EXTI_GetITStatus(EXTI_Line7) != RESET)
    {
        gpio_irq_handler(7);
        EXTI_ClearITPendingBit(EXTI_Line7);
    }
    if (EXTI_GetITStatus(EXTI_Line8) != RESET)
    {
        gpio_irq_handler(8);
        EXTI_ClearITPendingBit(EXTI_Line8);
    }
    if (EXTI_GetITStatus(EXTI_Line9) != RESET)
    {
        gpio_irq_handler(9);
        EXTI_ClearITPendingBit(EXTI_Line9);
    }

    FREE_INT_SP();
}
