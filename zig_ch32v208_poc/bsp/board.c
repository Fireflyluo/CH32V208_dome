#include "board.h"

#include "ch32v20x.h"
#include "debug.h"

// Demo projects in the parent directory use PC9 as the status LED.
#define BOARD_LED_GPIO GPIOC
#define BOARD_LED_PIN  GPIO_Pin_9

static int s_led_on = 0;

void board_led_write(int on)
{
    s_led_on = on ? 1 : 0;
    if (s_led_on)
    {
        GPIO_SetBits(BOARD_LED_GPIO, BOARD_LED_PIN);
    }
    else
    {
        GPIO_ResetBits(BOARD_LED_GPIO, BOARD_LED_PIN);
    }
}

void board_led_toggle(void)
{
    board_led_write(!s_led_on);
}

void board_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    gpio.GPIO_Pin = BOARD_LED_PIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(BOARD_LED_GPIO, &gpio);

    board_led_write(0);
    PRINT("board_init ok, SystemCoreClock=%lu\r\n", (unsigned long)SystemCoreClock);
}

