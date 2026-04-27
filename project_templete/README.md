# CH32V208 模板工程示例说明

## 项目概述

这是一个基于 **CH32V208** 微控制器的通用模板工程项目，旨在为开发者提供一个完整的开发框架和学习参考。该项目采用多构建系统支持，提供了多个IDE环境的适配版本。

- 提供多种构建系统选择，适应不同开发习惯
- 模块化设计，易于扩展和维护
- 预配置的开发环境，开箱即用

## 工程特点

### 多IDE支持
- **MR2**: 专用开发环境配置
- **eide**: VSCode EIDE 集成开发环境（后期已弃用，只保留模板）
- **xmake_cmake**: 支持 CMake 和 xmake 构建系统


### SDK 库文件
- **Core**: RISC-V 核心支持
- **Peripheral**: 外设驱动库（GPIO、I2C、SPI、USART、USB 等）
- **Debug**: 调试功能支持


## 使用说明
三种环境的模板工程,注意使用gcc交叉编译工具链需要从MR/MR2安装路径中找。
```c
-- 工具链路径（填你自己的工具链路径）
local TOOLCHAIN_FOLDER = "E:/APP/MRS2/MounRiver_Studio2/resources/app/resources/win32/components/WCH/Toolchain/RISC-V Embedded GCC12"
local TOOLCHAIN_PREFIX = "riscv-wch-elf-"
```
## 开发支持

- **调试工具**: WCH-LINK
- **联系方式**: Fireflyluo (QQ: 2161486135)

---
*这是一个标准模板*