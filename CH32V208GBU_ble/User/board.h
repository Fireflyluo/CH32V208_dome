/**
 ******************************************************************************
 * @file    board.c
 * @brief   Board specific initialization
 ******************************************************************************
 * @note    本文件内定义与具体开发板以及芯片外设相关的初始化代码
 *
 ******************************************************************************
 */
#ifndef __BOARD_H
#define __BOARD_H

#include "debug.h"
extern vu32 sys_tick_counter;

uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t ms);
void board_init(void);
void IIC_Init(uint32_t bound, uint16_t address);
uint8_t I2C_Master_Receive_Polling(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t size);
uint8_t I2C_Master_Transmit_Polling(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t size);
int accel_init(void);
uint8_t I2C_Scan(I2C_TypeDef *I2Cx, uint8_t print);
void read_acceleration_data(void);
#endif /* __BOARD_H */