/**
 ******************************************************************************
 * @file    board.h
 * @brief   板级初始化与时间基准接口声明
 ******************************************************************************
 */
#ifndef __BOARD_H
#define __BOARD_H

#include "debug.h"

/* HAL 依赖的系统时间接口 */
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t ms);

/* 板级统一初始化入口 */
void board_init(void);

#endif /* __BOARD_H */
