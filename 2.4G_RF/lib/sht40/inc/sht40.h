/**
 * @file    sht40.h
 * @brief   SHT40温湿度传感器驱动公共API头文件
 * @details 本文件定义了SHT40数字温湿度传感器的公共API接口，支持同步和异步操作模式。
 *          SHT40是一款高精度、低功耗的数字温湿度传感器，具有可配置的测量精度和加热器功能。
 *          
 *          驱动特性：
 *          - 支持同步和异步两种API模式
 *          - 提供多种测量精度选项（高/中/低精度）
 *          - 支持内置加热器控制（用于湿度校准或除湿）
 *          - 基于统一的总线抽象层，便于移植到不同平台
 *          - 支持设备序列号读取
 *          
 *          使用流程：
 *          1. 实现总线操作接口（sht40_bus_ops_t）
 *          2. 初始化设备结构体（sht40_dev_t）
 *          3. 调用sht40_init()进行初始化
 *          4. 使用sht40_read_sample()获取温湿度数据
 *
 * @author  WCH (南京沁恒微电子股份有限公司)
 * @version V1.0.0
 * @date    2022/06/16
 */

#ifndef SHT40_NEW_H
#define SHT40_NEW_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sht40_core.h"

/**
 * @brief  SHT40设备初始化
 * @details 初始化SHT40设备，验证设备通信并设置默认参数。
 * 
 * @param[in,out] dev 指向SHT40设备结构体的指针
 * @return 0表示成功，负数表示错误码
 */
int sht40_init(sht40_dev_t *dev);

/**
 * @brief  SHT40软复位
 * @details 执行软件复位操作，将设备恢复到上电状态。
 * 
 * @param[in,out] dev 指向SHT40设备结构体的指针
 * @return 0表示成功，负数表示错误码
 */
int sht40_soft_reset(sht40_dev_t *dev);

/**
 * @brief  读取设备序列号
 * @details 读取SHT40的32位设备序列号，用于设备识别。
 * 
 * @param[in,out] dev 指向SHT40设备结构体的指针
 * @param[out] serial 指向存储序列号的变量指针
 * @return 0表示成功，负数表示错误码
 */
int sht40_read_serial(sht40_dev_t *dev, uint32_t *serial);

/**
 * @brief  同步读取温湿度样本
 * @details 根据指定的精度等级执行温湿度测量并返回结果。
 * 
 * @param[in,out] dev 指向SHT40设备结构体的指针
 * @param[in] precision 测量精度等级（高/中/低）
 * @param[out] out 指向存储测量结果的结构体指针
 * @return 0表示成功，负数表示错误码
 */
int sht40_read_sample(sht40_dev_t *dev, sht40_precision_t precision, sht40_sample_t *out);

/**
 * @brief  控制内置加热器
 * @details 控制SHT40的内置加热器，可用于湿度校准或快速除湿。
 * 
 * @param[in,out] dev 指向SHT40设备结构体的指针
 * @param[in] cmd 加热器控制命令（开启/关闭，不同功率等级）
 * @return 0表示成功，负数表示错误码
 */
int sht40_heater(sht40_dev_t *dev, sht40_heater_cmd_t cmd);

/**
 * @brief  异步软复位
 * @details 异步方式执行软件复位操作，操作完成后调用回调函数。
 * 
 * @param[in,out] dev 指向SHT40设备结构体的指针
 * @param[in] cb 完成回调函数指针
 * @param[in] user 用户上下文指针（传递给回调函数）
 * @return 0表示成功提交请求，负数表示错误码
 */
int sht40_soft_reset_async(sht40_dev_t *dev, sht40_done_cb_t cb, void *user);

/**
 * @brief  异步读取温湿度样本
 * @details 异步方式执行温湿度测量，操作完成后调用回调函数。
 * 
 * @param[in,out] dev 指向SHT40设备结构体的指针
 * @param[in] precision 测量精度等级
 * @param[in] cb 完成回调函数指针
 * @param[in] user 用户上下文指针（传递给回调函数）
 * @return 0表示成功提交请求，负数表示错误码
 */
int sht40_read_sample_async(sht40_dev_t *dev,
                            sht40_precision_t precision,
                            sht40_sample_cb_t cb,
                            void *user);

/**
 * @brief  取消异步操作
 * @details 取消当前正在进行的异步操作（如果支持的话）。
 * 
 * @param[in,out] dev 指向SHT40设备结构体的指针
 * @return 0表示成功，负数表示错误码
 */
int sht40_cancel_async(sht40_dev_t *dev);

#ifdef __cplusplus
}
#endif

#endif