# CH32V208 TMOS CDC 工程说明文档

## 项目概述

这是一个基于 CH32V208 微控制器的嵌入式开发项目，集成了 TMOS（Tiny Multi-task Operating System）操作系统和 USB CDC（Communication Device Class）功能。该项目主要用作 CH32V208 学习记录和驱动示例，整合了多种外设和传感器，为用户提供了一个完整的开发验证平台。

## 硬件资源配置

### 主控芯片
- **型号**: CH32V208gbu6/wbu6

### 板载设备
- **2.4G射频模块**: SI24R1 型号
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
├── app/               # 应用层代码
│   ├── include/       # 应用头文件
│   ├── tasks/         # TMOS 任务
│   └── 源文件
├── ble_profile/       # BLE 配置文件
├── bsp/               # 板级支持包
│   ├── include/       # BSP 头文件
│   ├── UART/          # 串口驱动
│   └── 驱动源文件
├── lib/               # 外设驱动库
│   ├── oled/          # OLED 显示屏驱动
│   ├── sc7a20htr/     # 加速度计驱动
│   └── sht40/         # 温湿度传感器驱动
├── utils/             # 公共组件
├── sdk/               # SDK 库文件
│   ├── Core/          # RISC-V 核心代码
│   ├── Debug/         # 调试功能
│   ├── HAL/           # 硬件抽象层
│   ├── LIB/           # 库文件
│   ├── Peripheral/    # 外设驱动
│   └── USBLIB/        # USB 库
└── tools/             # 开发工具配置
```

### 关键模块说明

#### 1. BSP（板级支持包）
- **[drv_gpio.c/h](file:///d:/Desktop/ch32/0.CH32V208_dome/ch32v208_tmos_cdc/bsp/drv_gpio.c)**: GPIO 驱动接口
- **[drv_i2c.c/h](file:///d:/Desktop/ch32/0.CH32V208_dome/ch32v208_tmos_cdc/bsp/drv_i2c.c)**: I2C 通信驱动
- **[usb_cdc.c/h](file:///d:/Desktop/ch32/0.CH32V208_dome/ch32v208_tmos_cdc/bsp/usb_cdc.c)**: USB CDC 通信接口
- **UART（已废弃）**: 历史串口实现，当前工程以 USB CDC 为主

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
- ✅ **USB CDC 串口通信**: 支持虚拟串口通信
- ✅ **USB CDC**: 虚拟串口功能
- ✅ **传感器集成**: 支持加速度计和温湿度传感器



### 待实现功能
- ❏ **SPI通信**: SPI接口通信示例
- ❏ **定时器应用**: 定时器功能演示
- ❏ **中断处理**: 中断服务程序示例
- ❏ **低功耗模式**: 省电模式实现
- ❏ **更多外设驱动**: 扩展其他外设支持

## 开发环境配置

### 编译工具链
- **IDE/编译器**: MR2/vscode eide/cmake/xmake
- **调试工具**: WCH-LINK
- **编程语言**: C/C++

### 项目构建
项目采用模块化设计，支持使用 EIDE 以及 MR2 进行编译和调试。

## 编码规范

- 工程文本文件统一使用 UTF-8 编码。
- 在 VS Code 中已通过 `.editorconfig` 和 `.vscode/settings.json` 固化 UTF-8 配置。
- 若使用 PowerShell 查看文件内容，建议显式使用 `Get-Content -Encoding utf8`，避免因终端默认编码导致“看起来像乱码”的显示问题。

## 系统初始化流程

1. **系统时钟配置**: 通过 [system_ch32v20x.c](file:///d:/Desktop/ch32/0.CH32V208_dome/ch32v208_tmos_cdc/app/system_ch32v20x.c) 进行系统时钟初始化
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

### 串口调试
- 单次读取（推荐快速验证）：
  `python scripts/serial_reader.py --port COM8 --baud 115200 --encoding utf-8 --once`
- 持续读取：
  `python scripts/serial_reader.py --port COM8 --baud 115200 --encoding utf-8`
- 如设备输出使用非 UTF-8 编码，可切换 `--encoding`（例如 `gbk`）。

## 技术支持

- **作者**: Fireflyluo
- **QQ**: 2161486135
- **邮箱**: 2161486135@qq.com

## 版本信息

- **创建日期**: 2026-2-10
- **当前状态**: 初始版本，持续更新中

## 注意事项

1. 项目仍在开发阶段，部分功能有待完善
2. 使用前请确认硬件连接正确
3. 调试时建议使用WCH-LINK调试器
4. 传感器驱动可根据实际需求进行调整优化
