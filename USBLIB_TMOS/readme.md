# USBLIB_TMOS 

## 项目概述

本工程是基于官方usb库的tmos工程。三个版本仅复杂度（完成度）的区别，基础版是仅移植usb库的简单版本；标准版引入xmake_cmake构建、移除官方例程的额外依赖；复杂版增加i2c总线仲裁层，oled、传感器等。

## 各版本详细介绍

### 基础版 (/basic)
基础版是TMOS和USB库的最简化集成版本，主要用于验证基本功能。

- **核心功能**：
  - TMOS（Tiny Multi-task Operating System）基础框架
  - USB CDC（Communication Device Class）虚拟串口功能
  - 基础GPIO控制（LED指示灯）
  
- **特点**：
  - 最小化配置，适合入门学习
  - 仅包含必要的USB库和TMOS框架
  - 支持MounRiver Studio (MR2) 和 VSCode EIDE 直接打开使用
  - 包含基础的USB CDC功能示例

- **适用场景**：
  - CH32V208 USB CDC功能学习
  - TMOS基础概念理解
  - 快速原型验证

### 标准版 (/standard)
标准版在基础版之上增加了更完善的外设支持和现代化的构建系统。

- **核心功能**：
  - 保留基础版的所有功能
  - 引入xmake和cmake构建系统
  - 集成OLED显示屏驱动
  - 集成SC7A20加速度计和SHT40温湿度传感器驱动
  - 整合2.4G射频模块（SI24R1）
  
- **特点**：
  - 支持xmake和cmake现代化构建方式
  - 仍兼容MounRiver Studio
  - 模块化设计，便于扩展
  - 包含更完整的外设驱动示例
  
- **适用场景**：
  - 需要外设集成的项目开发
  - 学习现代化构建系统在嵌入式项目中的应用
  - 需要传感器数据采集的应用

### 复杂版 (/complex)
复杂版是最完整和功能丰富的版本，包含了高级特性和复杂的任务管理系统。

- **核心功能**：
  - 包含标准版的所有功能
  - 实现I2C总线仲裁机制，支持IT/DMA混合传输
  - 完整的多任务系统（传感器任务、显示任务、串口上报任务）
  - SC7A20三轴加速度采集（10Hz采样）
  - SHT40温湿度采集（1Hz采样，两段式）
  - OLED显示任务（2Hz刷新）
  - USB CDC串口上报任务（每秒上报数据）
  
- **特点**：
  - 高级I2C总线仲裁，防止多设备冲突
  - 完整的数据采集、显示、上报任务体系
  - 支持xmake和cmake构建系统
  - 仍兼容MounRiver Studio
  - 包含错误检查和稳定性处理机制
  
- **适用场景**：
  - 需要多传感器协同工作的复杂项目
  - 需要高稳定性和可靠性的产品级应用
  - 学习高级嵌入式系统设计模式

## 硬件资源

验证板搭载的主要硬件资源包括：

- **主控芯片**: CH32V208gbu6/wbu6
- **板载设备**:
  - 2.4G射频模块（型号: si24r1 ）
  - oled屏幕（型号: i2c屏 ）
- **其他外设**:
  - LED 指示灯
  - 按键
- **传感器**:
  - SC7A20 加速度计
  - SHT40 温湿度传感器

## 项目结构

```c
├─ \basic                   // 基础移植，最小化功能验证
├─ \complex                 // 完善板，包含完整传感器和任务系统
├─ \standard                // 标准版，包含外设驱动和现代构建系统
└─ readme.md                           
```

## 开发环境

- **IDE/编译器**: MR2/vscode eide/cmake/xmake
- **编程语言**: C/CPP
- **调试工具**: wch-link
- **依赖库**: 官方标准外设库

## 使用说明
- **/basic** 中直接打开MR2或者VSCode EIDE插件即可；
- **/standard** 使用xmake或cmake构建；同时支持MR2；
- **/complex** 使用xmake或cmake构建；同时支持MR2。

## 更新日志

- **2026-4-27**: 初始版本提交
  - 添加模板工程(eide、MR2、xmake_cmake)
  - 修改工程结构和文档描述

## 联系方式

- 作者: Fireflyluo
- qq: 2161486135
- 邮箱: 2161486135@qq.com

## 注意事项

gcc交叉编译工具链请使用自己的安装位置
```lua
-- 工具链路径（填你自己的工具链路径，以下是示例）
local TOOLCHAIN_FOLDER = "E:/APP/MRS2/MounRiver_Studio2/resources/app/resources/win32/components/WCH/Toolchain/RISC-V Embedded GCC12"
local TOOLCHAIN_PREFIX = "riscv-wch-elf-"
```
---

*持续更新中...*
