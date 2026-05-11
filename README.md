# CH32V208_dome 验证

## 项目概述

本项目是 CH32V208 的学习记录和驱动示例。该验证板放了一颗温湿度计(sht40)和加速度计(sc7a20htr)，用于学习和验证 CH32V208 的各项功能。

## 硬件资源

验证板搭载的主要硬件资源包括：

- **主控芯片**: CH32V208gbu6/wbu6
- **板载设备**:
  - 2.4G射频模块（型号: si24r1 ）
  - oled屏幕（型号: i2c屏 ）
- **其他外设**:
  - LED 指示灯
  - 按键

## 项目结构

```c
├─bsp                                   // ch32v208 板级驱动  
│  ├─ bus                               // 硬件抽象层
│  ├─ drivers                           // 硬件抽象层
│  └─ board.c                           // 板级初始化
├─project_templete                      // 模板工程
│      ├─ eide                            // eide工程
│      ├─ MR2                             // MR2工程
│      └─ xmmake_cmake                  // cmake/xmake 工程
├─tmos project                          // 使用 tmos 的模板工程
├─cherryUSB_CDC_ACM                     // cherryusb 移植模板（CDC ACM示例）
├─USBLIB_TMOS                           // 使用官方usb库的tmos工程
│      ├─ basic                           // 基础模板
│      ├─ complex                         // MR2工程 
│      └─ standard                        // cmake/xmake工程
├─zig_ch32v208_poc                      // zig 尝试性测试工程
└─Demo_v0.x                             // 简单完整模板工程
```

## 开发环境


- **IDE/编译器**: MR2/vscode eide/cmake/xmake
- **编程语言**: C/CPP
- **调试工具**: wch-link
- **依赖库**: 标准库

## 快速开始


## 已实现功能

- [x] LED控制（GPIO）
- [x] I2C通信示例（轮询、异步中断模式）
- [x] 串口通信示例
- [x] i2c总线驱动（轮询与异步）及i2c总线仲裁层（单总线多任务冲突）
- [x] 加速度计、温湿度计、oled驱动
- [x] usb cdc 虚拟串口
- [x] cherryUSB CDC 串口通信
- [x] 完整2.4G_rf+usb_CDC+i2c+tmos的demo 


## 使用说明


## 更新日志

- **2026-2-10**: 初始版本提交
  - 添加模板工程(eide、MR2、xmake_cmake)
  - 修改工程结构和文档描述
- **2026-2-20**: 优化模板工程
  - 添加bsp包，优化硬件抽象层(增加gpio驱动、i2c驱动(轮询/中断)、uart驱动(轮询/中断)、usb_cdc驱动)
  - 修改工程结构和文档描述
- **2026-2-20**: 驱动优化
  - sc7a20驱动增加异步接口和示例
  - 修改工程结构和文档描述
- **2026-4-8**: cherryusb(CDC ACM) 移植
  - USBLIB 路径（旧 USB Device 内核 / PMA 结构）
- **2026-4-23**: 增加2.4G射频以及射频协议的dome
- **2026-4-23**: 增加一个加速度碰撞检测和位移估算dome
- **2026-4-27**: 增加一个2.4G射频自组网协议的dome
- **2026-5-02**: 增加一个zig 的尝试，以.a形式引入
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
