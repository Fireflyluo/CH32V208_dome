/**
 ******************************************************************************
 * @file    sc7a20_ch32_adapter.h
 * @brief   SC7A20加速度传感器CH32平台适配器头文件
 * @note    本文件定义了SC7A20传感器在CH32V208平台上的I2C总线适配器接口，
 *          包含总线上下文结构体和操作函数表声明。
 ******************************************************************************
 */

#ifndef SC7A20_NEW_CH32_ADAPTER_H
#define SC7A20_NEW_CH32_ADAPTER_H

#include "../inc/sc7a20_core.h"
#include "drv_i2c.h"

/**
 * @brief  SC7A20 CH32平台I2C总线上下文结构体
 * @note   该结构体保存了I2C通信所需的必要信息
 */
typedef struct {
    i2c_num_t i2c_num;    /*!< I2C总线编号 */
    uint8_t dev_addr;     /*!< 设备I2C地址 */
} sc7a20_ch32_bus_ctx_t;

/**
 * @brief  全局SC7A20 CH32 I2C总线操作函数表
 * @note   该函数表实现了sc7a20_bus_ops_t接口，供SC7A20核心库调用
 */
extern const sc7a20_bus_ops_t g_sc7a20_ch32_i2c_ops;

#endif