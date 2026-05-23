/********************************** (C) COPYRIGHT *******************************
 * File Name          : RTC.c
 * Author             : WCH
 * Version            : V1.2
 * Date               : 2022/01/18
 * Description        : RTC configuration and its initialization
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
/* Header file contains */
#include "HAL.h"

/*********************************************************************
 * CONSTANTS
 */
#define RTC_INIT_TIME_HOUR 0
#define RTC_INIT_TIME_MINUTE 0
#define RTC_INIT_TIME_SECEND 0
#define HAL_TIME_LSE_READY_TIMEOUT_MS 3000u

/***************************************************
 * Global variables
 */
volatile uint32_t RTCTigFlag;

/*******************************************************************************
 * @fn      RTC_SetTignTime
 *
 * @brief   Configure RTC trigger time
 *
 * @param   time    - Trigger time.
 *
 * @return  None.
 */
void RTC_SetTignTime(uint32_t time)
{
    RTC_WaitForLastTask();
    RTC_SetAlarm(time);
    RTC_WaitForLastTask();
    RTCTigFlag = 0;
}

static uint8_t hal_time_wait_lse_ready(uint32_t timeout_ms)
{
    while (timeout_ms > 0u)
    {
        if (RCC_GetFlagStatus(RCC_FLAG_LSERDY) != RESET)
        {
            return 1u;
        }
        Delay_Ms(1u);
        timeout_ms--;
    }
    return 0u;
}

/*******************************************************************************
 * @fn      HAL_Time0Init
 *
 * @brief   System timer initialization
 *
 * @param   None.
 *
 * @return  None.
 */
void HAL_TimeInit(void)
{
    uint8_t state = 0;
    uint8_t lse_ready = 0u;
    uint32_t rtc_src_hz = 32000u;
    uint32_t desired_rtc_sel = RCC_RTCSEL_LSI;
    uint32_t bdctl_before;
    uint32_t bdctl_after;
    bleClockConfig_t conf = {0};

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    PWR_BackupAccessCmd(ENABLE);
    RCC_LSICmd(ENABLE);

#if (CLK_OSC32K)
    RCC_LSEConfig(RCC_LSE_OFF);
    desired_rtc_sel = RCC_RTCSEL_LSI;
    rtc_src_hz = 32000u;
#else
    RCC_LSEConfig(RCC_LSE_ON);
    lse_ready = hal_time_wait_lse_ready(HAL_TIME_LSE_READY_TIMEOUT_MS);
    if (lse_ready != 0u)
    {
        desired_rtc_sel = RCC_RTCSEL_LSE;
        rtc_src_hz = 32768u;
    }
    else
    {
        RCC_LSEConfig(RCC_LSE_OFF);
        desired_rtc_sel = RCC_RTCSEL_LSI;
        rtc_src_hz = 32000u;
        PRINT("rtc: LSE not ready, fallback LSI\r\n");
    }
#endif

    bdctl_before = RCC->BDCTLR;
    if ((bdctl_before & RCC_RTCSEL) != desired_rtc_sel)
    {
        RCC_BackupResetCmd(ENABLE);
        RCC_BackupResetCmd(DISABLE);
        PWR_BackupAccessCmd(ENABLE);

        if (desired_rtc_sel == RCC_RTCSEL_LSE)
        {
            RCC_LSEConfig(RCC_LSE_ON);
            lse_ready = hal_time_wait_lse_ready(HAL_TIME_LSE_READY_TIMEOUT_MS);
            if (lse_ready == 0u)
            {
                RCC_LSEConfig(RCC_LSE_OFF);
                desired_rtc_sel = RCC_RTCSEL_LSI;
                rtc_src_hz = 32000u;
                PRINT("rtc: LSE lost after BDRST, fallback LSI\r\n");
            }
        }
        else
        {
            RCC_LSEConfig(RCC_LSE_OFF);
        }
    }

    RCC->BDCTLR = (RCC->BDCTLR & ~RCC_RTCSEL) | desired_rtc_sel;
    RCC_RTCCLKCmd(ENABLE);
    RTC_WaitForLastTask();
    RTC_WaitForLastTask();
    RTC_SetPrescaler(1);
    RTC_WaitForLastTask();
    RTC_SetCounter(0);
    RTC_WaitForLastTask();

    bdctl_after = RCC->BDCTLR;
    PRINT("rtc: bdctl 0x%08lx -> 0x%08lx, sel=%s, lserdy=%u\r\n",
          (unsigned long)bdctl_before,
          (unsigned long)bdctl_after,
          ((bdctl_after & RCC_RTCSEL) == RCC_RTCSEL_LSE) ? "LSE" : "LSI",
          (unsigned int)((bdctl_after & RCC_LSERDY) != 0u ? 1u : 0u));

#if (CLK_OSC32K)
    Lib_Calibration_LSI();
#endif
    conf.ClockAccuracy = CLK_OSC32K ? 1000 : 100;
    conf.ClockFrequency = rtc_src_hz / 2u;
    conf.ClockMaxCount = 0xFFFFFFFF;
    conf.getClockValue = RTC_GetCounter;
    state = TMOS_TimerInit(&conf);
    if (state)
    {
        PRINT("TMOS_TimerInit err %x\n", state);
    }
}

__attribute__((interrupt("WCH-Interrupt-fast"))) void RTCAlarm_IRQHandler(void)
{
    RTCTigFlag = 1;
    EXTI_ClearITPendingBit(EXTI_Line17);
    RTC_ClearITPendingBit(RTC_IT_ALR);
    RTC_WaitForLastTask();
}

/******************************** endfile @ time ******************************/
