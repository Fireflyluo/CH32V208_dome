# CH32V208 模板工程示例说明

## 项目概述

这是一个基于 **CH32V208** 微控制器的通用模板工程项目，旨在为开发者提供一个完整的开发框架和学习参考。该项目采用多构建系统支持，提供了多个IDE环境的适配版本。

- 提供多种构建系统选择，适应不同开发习惯
- 模块化设计，易于扩展和维护
- 预配置的开发环境，开箱即用

## 工程特点

### 多IDE支持
- **MR2**: 专用开发环境配置
- **eide**: VSCode EIDE 集成开发环境
- **xmake_cmake**: 支持 CMake 和 xmake 构建系统

### 核心组件

#### 1. 应用层 (APP)
- **[peripheral_main.c](file:///d:/Desktop/ch32/0.CH32V208_dome/ch32v208_templete/eide/peripheral_main.c)**: 外设主程序入口
- **[ch32v20x_it.c](file:///d:/Desktop/ch32/0.CH32V208_dome/ch32v208_templete/eide/ch32v20x_it.c)**: 中断处理函数
- **系统配置**: 时钟和外设初始化配置

#### 2. 硬件抽象层 (HAL)
- **[LED.c/h](file:///d:/Desktop/ch32/0.CH32V208_dome/ch32v208_templete/eide/HAL/LED.c)**: LED 控制接口
- **[KEY.c/h](file:///d:/Desktop/ch32/0.CH32V208_dome/ch32v208_templete/eide/HAL/KEY.c)**: 按键输入处理
- **[RTC.c/h](file:///d:/Desktop/ch32/0.CH32V208_dome/ch32v208_templete/eide/HAL/RTC.c)**: 实时时钟功能
- **[SLEEP.c/h](file:///d:/Desktop/ch32/0.CH32V208_dome/ch32v208_templete/eide/HAL/SLEEP.c)**: 低功耗管理

#### 3. SDK 库文件
- **Core**: RISC-V 核心支持
- **Peripheral**: 外设驱动库（GPIO、I2C、SPI、USART、USB 等）
- **Debug**: 调试功能支持

#### 4. 任务管理系统 (TMOS)
- **[tmos_task.h](file:///d:/Desktop/ch32/0.CH32V208_dome/ch32v208_templete/xmake_cmake/task/inc/tmos_task.h)**: 任务调度头文件
- **[tmos_led_task.c](file:///d:/Desktop/ch32/0.CH32V208_dome/ch32v208_templete/xmake_cmake/task/tmos_led_task.c)**: LED 任务实现

## 已实现功能

- ✅ **GPIO控制**: LED指示灯控制
- ✅ **按键处理**: 用户输入响应
- ✅ **基础外设**: 各种常用外设接口
- ✅ **多任务系统**: TMOS轻量级操作系统支持

## 使用说明
待完善...

## 开发支持

- **调试工具**: WCH-LINK
- **联系方式**: Fireflyluo (QQ: 2161486135)

---
*这是一个持续更新的标准模板*