/**
 * @file    sc7a20.h
 * @brief   SC7A20加速度计驱动公共API头文件
 * @details 本文件定义了SC7A20三轴数字加速度计的公共API接口，支持同步和异步操作模式。
 *          SC7A20是一款低功耗、高精度的MEMS加速度计，支持多种量程（±2g/±4g/±8g/±16g）
 *          和输出数据速率（ODR）配置。
 *          
 *          驱动特性：
 *          - 支持同步和异步两种API模式
 *          - 提供原始数据（16位整数）和工程单位（g）两种读取方式
 *          - 支持运行时动态配置量程、ODR和轴使能
 *          - 基于统一的总线抽象层，便于移植到不同平台
 *          
 *          使用流程：
 *          1. 实现总线操作接口（sc7a20_bus_ops_t）
 *          2. 初始化设备结构体（sc7a20_dev_t）
 *          3. 调用sc7a20_init()或sc7a20_init_with_config()进行初始化
 *          4. 使用读取函数获取加速度数据
 *
 * @author 
 * @version V1.0.0
 * @date    
 */

#ifndef SC7A20_NEW_H
#define SC7A20_NEW_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sc7a20_core.h"

/**
 * @brief  SC7A20设备初始化（使用默认配置）
 * @details 使用默认配置参数初始化SC7A20设备，包括量程、ODR等。
 * 
 * @param[in,out] dev 指向SC7A20设备结构体的指针
 * @return 0表示成功，负数表示错误码
 */
int sc7a20_init(sc7a20_dev_t *dev);

/**
 * @brief  SC7A20设备初始化（使用自定义配置）
 * @details 使用用户提供的配置参数初始化SC7A20设备。
 * 
 * @param[in,out] dev 指向SC7A20设备结构体的指针
 * @param[in] cfg 指向SC7A20配置结构体的指针
 * @return 0表示成功，负数表示错误码
 */
int sc7a20_init_with_config(sc7a20_dev_t *dev, const sc7a20_cfg_t *cfg);

/**
 * @brief  SC7A20设备去初始化
 * @details 释放设备资源，将设备置于低功耗状态。
 * 
 * @param[in,out] dev 指向SC7A20设备结构体的指针
 * @return 0表示成功，负数表示错误码
 */
int sc7a20_deinit(sc7a20_dev_t *dev);

/**
 * @brief  SC7A20软复位
 * @details 执行软件复位操作，将设备恢复到上电状态。
 * 
 * @param[in,out] dev 指向SC7A20设备结构体的指针
 * @return 0表示成功，负数表示错误码
 */
int sc7a20_soft_reset(sc7a20_dev_t *dev);

/**
 * @brief  读取设备ID
 * @details 读取SC7A20的WHO_AM_I寄存器值，用于设备识别。
 * 
 * @param[in,out] dev 指向SC7A20设备结构体的指针
 * @param[out] who_am_i 指向存储设备ID的缓冲区指针
 * @return 0表示成功，负数表示错误码
 */
int sc7a20_get_who_am_i(sc7a20_dev_t *dev, uint8_t *who_am_i);

/**
 * @brief  同步读取寄存器（通用接口）
 * @details 从指定寄存器地址读取指定长度的数据。
 * 
 * @param[in,out] dev 指向SC7A20设备结构体的指针
 * @param[in] reg 寄存器地址
 * @param[out] data 指向数据缓冲区的指针
 * @param[in] len 要读取的数据长度
 * @return 0表示成功，负数表示错误码
 */
int sc7a20_read_reg(sc7a20_dev_t *dev, uint8_t reg, uint8_t *data, uint16_t len);
int sc7a20_read_reg_noinc(sc7a20_dev_t *dev, uint8_t reg, uint8_t *data, uint16_t len);
int sc7a20_read_fifo_data(sc7a20_dev_t *dev, uint8_t *data, uint16_t len);

/**
 * @brief  同步写入寄存器（通用接口）
 * @details 向指定寄存器地址写入指定长度的数据。
 * 
 * @param[in,out] dev 指向SC7A20设备结构体的指针
 * @param[in] reg 寄存器地址
 * @param[in] data 指向要写入数据的指针
 * @param[in] len 要写入的数据长度
 * @return 0表示成功，负数表示错误码
 */
int sc7a20_write_reg(sc7a20_dev_t *dev, uint8_t reg, const uint8_t *data, uint16_t len);

/**
 * @brief  同步读取三轴原始加速度数据
 * @details 读取X、Y、Z三轴的16位原始加速度数据。
 * 
 * @param[in,out] dev 指向SC7A20设备结构体的指针
 * @param[out] out 指向存储三轴数据的结构体指针
 * @return 0表示成功，负数表示错误码
 */
int sc7a20_read_xyz_raw(sc7a20_dev_t *dev, sc7a20_vec3i16_t *out);

/**
 * @brief  同步读取三轴工程单位加速度数据
 * @details 读取X、Y、Z三轴的浮点型加速度数据（单位：g）。
 * 
 * @param[in,out] dev 指向SC7A20设备结构体的指针
 * @param[out] out 指向存储三轴数据的结构体指针
 * @return 0表示成功，负数表示错误码
 */
int sc7a20_read_xyz_g(sc7a20_dev_t *dev, sc7a20_vec3f_t *out);

/**
 * @brief  设置加速度计量程
 * @details 动态设置SC7A20的加速度测量范围。
 * 
 * @param[in,out] dev 指向SC7A20设备结构体的指针
 * @param[in] range 量程选择（SC7A20_ACCEL_FS_2G/4G/8G/16G）
 * @return 0表示成功，负数表示错误码
 */
int sc7a20_set_range(sc7a20_dev_t *dev, sc7a20_accel_fs_t range);

/**
 * @brief  设置输出数据速率(ODR)
 * @details 动态设置SC7A20的数据输出频率。
 * 
 * @param[in,out] dev 指向SC7A20设备结构体的指针
 * @param[in] odr ODR选择（如SC7A20_ACCEL_ODR_100HZ等）
 * @return 0表示成功，负数表示错误码
 */
int sc7a20_set_odr(sc7a20_dev_t *dev, sc7a20_accel_odr_t odr);

/**
 * @brief  设置轴使能状态
 * @details 动态启用或禁用X、Y、Z轴的数据采集。
 * 
 * @param[in,out] dev 指向SC7A20设备结构体的指针
 * @param[in] x_en X轴使能标志（true=使能，false=禁用）
 * @param[in] y_en Y轴使能标志（true=使能，false=禁用）
 * @param[in] z_en Z轴使能标志（true=使能，false=禁用）
 * @return 0表示成功，负数表示错误码
 */
int sc7a20_set_axis_enable(sc7a20_dev_t *dev, bool x_en, bool y_en, bool z_en);

/**
 * @brief  异步读取寄存器（通用接口）
 * @details 异步方式从指定寄存器地址读取数据，操作完成后调用回调函数。
 * 
 * @param[in,out] dev 指向SC7A20设备结构体的指针
 * @param[in] reg 寄存器地址
 * @param[out] data 指向数据缓冲区的指针
 * @param[in] len 要读取的数据长度
 * @param[in] cb 完成回调函数指针
 * @param[in] user 用户上下文指针（传递给回调函数）
 * @return 0表示成功提交请求，负数表示错误码
 */
int sc7a20_read_reg_async(sc7a20_dev_t *dev,
                          uint8_t reg,
                          uint8_t *data,
                          uint16_t len,
                          sc7a20_done_cb_t cb,
                          void *user);

/**
 * @brief  异步写入寄存器（通用接口）
 * @details 异步方式向指定寄存器地址写入数据，操作完成后调用回调函数。
 * 
 * @param[in,out] dev 指向SC7A20设备结构体的指针
 * @param[in] reg 寄存器地址
 * @param[in] data 指向要写入数据的指针
 * @param[in] len 要写入的数据长度
 * @param[in] cb 完成回调函数指针
 * @param[in] user 用户上下文指针（传递给回调函数）
 * @return 0表示成功提交请求，负数表示错误码
 */
int sc7a20_write_reg_async(sc7a20_dev_t *dev,
                           uint8_t reg,
                           const uint8_t *data,
                           uint16_t len,
                           sc7a20_done_cb_t cb,
                           void *user);

/**
 * @brief  异步读取三轴原始加速度数据
 * @details 异步方式读取X、Y、Z三轴的16位原始加速度数据，操作完成后调用回调函数。
 * 
 * @param[in,out] dev 指向SC7A20设备结构体的指针
 * @param[in] cb 完成回调函数指针
 * @param[in] user 用户上下文指针（传递给回调函数）
 * @return 0表示成功提交请求，负数表示错误码
 */
int sc7a20_read_xyz_raw_async(sc7a20_dev_t *dev,
                              sc7a20_read_xyz_cb_t cb,
                              void *user);

/**
 * @brief  取消异步操作
 * @details 取消当前正在进行的异步操作（如果支持的话）。
 * 
 * @param[in,out] dev 指向SC7A20设备结构体的指针
 * @return 0表示成功，负数表示错误码
 */
int sc7a20_cancel_async(sc7a20_dev_t *dev);

#ifdef __cplusplus
}
#endif

#endif
