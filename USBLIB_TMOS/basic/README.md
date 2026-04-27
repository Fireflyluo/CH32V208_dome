# CH32V208 TMOS USBLIB_CDC 工程说明文档

## 工程概述

这是一个基于tmos使用官方usblib库的基础工程，核心使用 TMOS 事件驱动框架和 USB CDC（Communication Device Class）功能。做了一些简单的使用示例。支持MR2、eide和xmake_cmake。

## 工程能力

### 应用层功能 (app/)
- **主程序入口** ([main.c](file:///d:/Desktop/ch32/0.CH32V208_dome/USBLIB_TMOS/basic/app/main.c))：系统初始化流程，包括板级初始化、TMOS系统启动等
- **中断处理** ([ch32v20x_it.c](file:///d:/Desktop/ch32/0.CH32V208_dome/USBLIB_TMOS/basic/ch32v20x_it.c))：系统中断向量表和中断服务函数
- **系统配置** ([system_ch32v20x.c](file:///d:/Desktop/ch32/0.CH32V208_dome/USBLIB_TMOS/basic/app/system_ch32v20x.c))：系统时钟配置和底层初始化
- **外设驱动** ([peripheral.c](file:///d:/Desktop/ch32/0.CH32V208_dome/USBLIB_TMOS/basic/app/peripheral.c))：基础外设操作函数

### 当前任务 (无专门任务模块)
- 该基础版本未实现专门的任务模块，主要功能集中在主循环中实现
- 提供TMOS框架基础，为后续扩展任务功能奠定基础

### 支持的硬件功能
- ✅ **LED控制**: 通过GPIO实现LED指示灯控制
- ✅ **I2C通信**: 实现I2C总线通信及设备扫描功能
- ✅ **串口通信**: 支持UART通信
- ✅ **USB CDC**: 虚拟串口功能
- ✅ **传感器集成**: 支持加速度计和温湿度传感器

## 硬件资源配置

### 主控芯片
- **型号**: CH32V208gbu6/wbu6

### 板载设备
- **OLED显示屏**: I2C 接口屏幕
- **传感器模块**:
  - SC7A20HTR 加速度计
  - SHT40 温湿度传感器

### 其他外设
- **LED指示灯**: GPIO 控制
- **按键**: 用户输入接口

## 项目架构

### 目录结构
```
├── APP/              # 应用层代码
│   ├── include/      # 应用头文件
│   └── 源文件
├── Profile/          # BLE 配置文件
├── bsp/              # 板级支持包
│   ├── Include/      # BSP 头文件
│   ├── UART/         # 串口驱动
│   └── 驱动源文件
├── public/           # 公共组件
├── sdk/              # SDK 库文件
│   ├── Core/         # RISC-V 核心代码
│   ├── Debug/        # 调试功能
│   ├── HAL/          # 硬件抽象层
│   ├── LIB/          # 库文件
│   ├── Peripheral/   # 外设驱动
│   └── USBLIB/       # USB 库
├── drivers/          # 传感器驱动
├── task/             # TMOS 任务
└── tools/            # 开发工具配置
```

### 关键模块说明

#### 1. BSP（板级支持包）
- **[drv_gpio.c/h](file:///d:/Desktop/ch32/0.CH32V208_dome/USBLIB_TMOS/bsp/drv_gpio.c)**: GPIO 驱动接口
- **[drv_i2c.c/h](file:///d:/Desktop/ch32/0.CH32V208_dome/USBLIB_TMOS/bsp/drv_i2c.c)**: I2C 通信驱动
- **[usb_cdc.c/h](file:///d:/Desktop/ch32/0.CH32V208_dome/USBLIB_TMOS/bsp/usb_cdc.c)**: USB CDC 通信接口
- **[UART.c/h](file:///d:/Desktop/ch32/0.CH32V208_dome/USBLIB_TMOS/bsp/UART/UART.c)**: 串口通信功能

#### 2. 传感器驱动
- **SC7A20HTR**: 三轴加速度传感器
- **SHT40**: 数字温湿度传感器

#### 3. 任务管理
- **TMOS**: 轻量级多任务操作系统
- **LED任务**: LED 控制任务管理

#### 4. SDK 组件
- **CH32V208 HAL库**: 硬件抽象层
- **USB库**: USB通信协议栈
- **BLE库**: 无线通信支持

## 功能特性

### 已实现功能
- ✅ **LED控制**: 通过GPIO实现LED指示灯控制
- ✅ **I2C通信**: 实现I2C总线通信及设备扫描功能
- ✅ **串口通信**: 支持UART通信
- ✅ **USB CDC**: 虚拟串口功能
- ✅ **传感器集成**: 支持加速度计和温湿度传感器

## 开发环境配置

### 编译工具链
- **IDE/编译器**: MR2/vscode eide/cmake/xmake
- **调试工具**: WCH-LINK
- **编程语言**: C/C++

### 项目构建
项目采用模块化设计，支持使用 EIDE 以及 MR2 进行编译和调试。

## 系统初始化流程

1. **系统时钟配置**: 通过 [system_ch32v20x.c](file:///d:/Desktop/ch32/0.CH32V208_dome/USBLIB_TMOS/APP/system_ch32v20x.c) 进行系统时钟初始化
2. **外设初始化**: GPIO、I2C、UART、USB等外设初始化
3. **TMOS系统启动**: 任务调度系统启动
4. **传感器初始化**: 加速度计和温湿度传感器初始化
5. **USB CDC激活**: 虚拟串口功能启用

## 使用说明

### 硬件连接
- 通过USB连接PC和开发板
- 可连接外部传感器至I2C接口
- LED和按键可直接使用

### 软件配置
- 修改 [board.c](file:///d:/Desktop/ch32/0.CH32V208_dome/ch32v208_tmos_cdc/bsp/board.c) 文件进行板级配置
- 通过 [config.h](file:///d:/Desktop/ch32/0.CH32V208_dome/ch32v208_tmos_cdc/sdk/HAL/include/config.h) 进行系统参数配置

## 技术支持

- **作者**: Fireflyluo
- **QQ**: 2161486135
- **邮箱**: 2161486135@qq.com

## 版本信息

- **创建日期**: 2026-2-10
- **当前状态**: 初始版本，持续更新中

## 注意事项

1. gcc交叉编译工具链请使用自己的安装位置
2. 使用前请确认硬件连接正确
3. 调试时建议使用WCH-LINK调试器
4. 传感器驱动可根据实际需求进行调整优化