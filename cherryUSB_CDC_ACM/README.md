# CH32V208 CherryUSB CDC ACM 示例工程

这是一个基于 CH32V208 微控制器和 CherryUSB 库开发的虚拟串口（CDC ACM）示例项目。该项目实现了 USB CDC 类设备功能，可以将 CH32V208 连接到计算机后识别为虚拟串口。

## 项目概述

- **目标平台**: CH32V208 微控制器
- **USB 栈**: CherryUSB（轻量级 USB 协议栈）
- **功能**: USB CDC ACM 虚拟串口
- **构建系统**: xmake

## 功能特性

1. **虚拟串口通信**:
   - 实现标准 CDC ACM 类协议
   - 支持双向数据传输
   - DTR 信号检测功能

2. **硬件兼容性**:
   - CH32V208 特定的 USB 初始化
   - 正确的时钟配置（支持多种频率）
   - USB 引脚（PA11/PA12）配置

3. **调试功能**:
   - 串口输出系统时钟信息
   - CDC 连接状态轮询检测

## 文件结构

```
├── User                    # 用户应用代码
│   ├── Main.c              # 主程序入口
│   ├── usb_cdc_app.c       # USB CDC 应用实现
│   ├── usb_cdc_app.h       # USB CDC 应用接口
│   ├── ch32v20x_it.c       # 中断处理函数
│   ├── ch32v20x_it.h       # 中断处理函数声明
│   ├── system_ch32v20x.c   # 系统初始化
│   └── system_ch32v20x.h   # 系统配置头文件
├── cherryUSB              # CherryUSB 协议栈
│   ├── core/              # 核心层
│   ├── class/cdc/         # CDC 类驱动
│   └── port/fsdev/        # CH32V208 硬件抽象层
├── sdk/                   # CH32V208 SDK
├── xmake.lua              # 构建配置文件
└── docs/                  # 文档
```

## 移植方法

### 1. 环境准备

确保安装了以下工具：
- WCH RISC-V GCC 工具链（版本 12 或 15）
- xmake 构建工具

### 2. 修改硬件相关配置

#### 2.1 USB 时钟配置
在 [Main.c](file:///d:/Desktop/ch32/0.ch32v208_dome/cherryUSB_CDC_ACM/User/Main.c) 中，检查 [USB_RCC_Init](file:///d:/Desktop/ch32/0.ch32v208_dome/cherryUSB_CDC_ACM/User/Main.c#L25-L50) 函数是否适配你的系统时钟频率：

```c
if (rcc_clocks_status.SYSCLK_Frequency == 144000000) {
    RCC_USBCLKConfig (RCC_USBCLKSource_PLLCLK_Div3);
} else if (rcc_clocks_status.SYSCLK_Frequency == 96000000) {
    RCC_USBCLKConfig (RCC_USBCLKSource_PLLCLK_Div2);
} else if (rcc_clocks_status.SYSCLK_Frequency == 48000000) {
    RCC_USBCLKConfig (RCC_USBCLKSource_PLLCLK_Div1);
}
```

#### 2.2 USB 引脚配置
如果使用不同的引脚，请修改 [usb_dc_low_level_init](file:///d:/Desktop/ch32/0.ch32v208_dome/cherryUSB_CDC_ACM/User/Main.c#L74-L98) 函数中的 GPIO 配置：

```c
GPIOA->CFGHR &= 0xFFF00FFF;  // 根据实际引脚调整
GPIOA->OUTDR &= ~(3 << 11);  // 根据实际引脚调整
GPIOA->CFGHR |= 0x00044000;  // 根据实际引脚调整
```

#### 2.3 中断配置
根据使用的中断线修改 [USB_Interrupts_Config](file:///d:/Desktop/ch32/0.ch32v208_dome/cherryUSB_CDC_ACM/User/Main.c#L53-L72) 函数。

### 3. 修改设备描述符

在 [usb_cdc_app.c](file:///d:/Desktop/ch32/0.ch32v208_dome/cherryUSB_CDC_ACM/User/usb_cdc_app.c) 中可以修改设备描述符：

```c
#define USBD_VID 0x1A86  // 修改为自己的厂商 ID
#define USBD_PID 0xFE0C  // 修改为自己的产品 ID

static const char *string_descriptors[] = {
    (const char[]) { 0x09, 0x04 },
    "wch.cn",           // 制造商字符串
    "USB Serial",       // 产品字符串
    "CH32V208-CDC"      // 序列号字符串
};
```

### 4. 构建项目

运行以下命令构建项目：

```bash
xmake
```

生成的固件位于 `build/cross/riscv/debug/CH32V208GBU.elf`，同时会生成 `.hex`、`.map` 等文件。

## 使用方法

1. 将 CH32V208 开发板连接到计算机
2. 下载并烧录固件到芯片
3. 系统将识别为虚拟串口设备
4. 使用串口助手等工具进行通信测试

## 调试技巧

- 通过串口打印查看系统时钟信息
- 使用 [cdc_acm_poll()](file:///d:/Desktop/ch32/0.ch32v208_dome/cherryUSB_CDC_ACM/User/usb_cdc_app.h#L7-L7) 函数检测连接状态
- 监控 DTR 信号状态变化

## CherryUSB 优势

1. **模块化设计**: 协议栈、类驱动、硬件抽象层分离，便于维护
2. **易于扩展**: 添加新功能或组合多个 USB 类更容易
3. **调试友好**: 清晰的回调接口，便于定位问题
4. **跨平台**: 抽象层设计使得移植到其他硬件平台更简单

## 注意事项

- 首次使用需要正确配置 WCH 工具链路径
- USB 初始化需要正确的引脚和时钟设置
- 在某些主机上可能需要额外的 CDC 参数配置