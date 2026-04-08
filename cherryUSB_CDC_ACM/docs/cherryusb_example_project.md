# CH32V208 CherryUSB CDC 示例工程说明

本文档描述当前 `CH32V208GBU_cherryusb` 示例工程的：

- 工程做了什么（功能范围、当前行为）
- CherryUSB 在 CH32V208 上的移植方法（关键点与原因）
- 构建/下载方式（xmake + OpenOCD tasks）
- 测试脚本的用途与用法（包含自动寻口）

## 1. 工程概览

本工程在 CH32V208 上运行 CherryUSB Device 协议栈，提供一个 CDC ACM（虚拟串口）设备。

- USB 设备信息（当前默认）：
  - `VID:PID = 1A86:FE0C`
  - Manufacturer/Product/String：`wch.cn` / `USB Serial` / `CH32V208-CDC`
- USB 功能（当前实现）：
  - CDC 数据通道为“回显模式”：PC 发送的字节会从设备原样回传。
  - 设备不会主动向 CDC 串口持续输出日志；如果只打开串口监视“纯读”，通常看不到数据（除非你先发送数据触发回显）。

相关实现集中在：

- `User/usb_cdc_app.c`：CDC 描述符 + 端点回调（回显）+ `cdc_acm_init()`
- `User/Main.c`：USB 低层初始化（时钟/IO/上拉/中断）+ 启动 CDC
- `User/ch32v20x_it.c`：USB 中断入口转发到 CherryUSB

## 2. 移植路线选择：为什么用 `fsdev` 而不是 `USBFS(0x5000...)`

CH32V208 常见两套 USB 设备“硬件核/寄存器模型”：

1. USBLIB 兼容的 USB Device FS（PMA/EP0R/CNTR/ISTR）：
   - 中断通常是 `USB_LP_CAN1_RX0_IRQn`
   - 寄存器基址通常是 `0x40005C00`
2. 另一套 USBFS 寄存器模型：
   - 中断是 `USBFS_IRQn`
   - 寄存器基址通常是 `0x50000000`

本示例工程最终采用 **CherryUSB 的 `fsdev` 端口**，对应的就是第 1 套（与官方 USBLIB CDC 示例同类硬件核）。

经验结论：

- 如果你能用官方 USBLIB 的 CDC 工程稳定枚举，那么 CherryUSB 优先走 `fsdev + 0x40005C00 + USB_LP_CAN1_RX0`，成功率最高。
- 如果选错了硬件核，典型现象是：单步/下载都正常，但 Windows 设备管理器完全没有新 USB 设备出现。

## 3. CherryUSB 移植关键点（按“能枚举”为目标）

### 3.1 编译层：加入 CherryUSB 源码与头文件

本工程用 xmake 组织，关键在 `xmake.lua`：

- 加入核心与 CDC 类：
  - `cherryUSB/core/usbd_core.c`
  - `cherryUSB/class/cdc/usbd_cdc_acm.c`
- 选择 `fsdev` 端口（与 `0x40005C00` 的寄存器模型一致）：
  - `cherryUSB/port/fsdev/usb_dc_fsdev.c`
- 指定 `USB_BASE`（供 `fsdev` 端口寄存器头使用）：
  - `add_defines(..., "USB_BASE=0x40005C00")`

### 3.2 运行层：实现 `usb_dc_low_level_init()`

CherryUSB 的 `usb_dc_init()` 会调用你提供的 `usb_dc_low_level_init()`（本工程在 `User/Main.c` 提供）。

最低要求是完成“让主机认为设备插入并可响应”的条件：

1. USB 时钟与外设时钟
   - 按系统主频配置 USB 分频到 48MHz（`RCC_USBCLKConfig(...)`）
   - 使能 USB 外设时钟：`RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE)`
2. DP/DM 引脚状态（PA11/PA12）
   - 使能 GPIOA
   - 将 PA11/PA12 设置为浮空输入
3. 打开内部 USB 上拉（attach 的关键）
   - `EXTEN->EXTEN_CTR |= EXTEN_USBD_PU_EN`
   - `EXTEN->EXTEN_CTR &= ~EXTEN_USBD_LS`（保持全速）
4. 中断配置
   - 配置 `USB_LP_CAN1_RX0_IRQn`（USB 事务中断）
   - 配置 `USBWakeUp_IRQn` + EXTI18（唤醒/线电平变化相关）

本工程的实现位置：

- `User/Main.c`：`usb_dc_low_level_init()` 和 `USB_Interrupts_Config()`

### 3.3 中断入口：把 USB IRQ 转发给 CherryUSB

关键规则：USB 中断里必须最终调用 `USBD_IRQHandler(0)`。

本工程实现：

- `User/ch32v20x_it.c`：`USB_LP_CAN1_RX0_IRQHandler()` -> `USBD_IRQHandler(0)`
- `User/ch32v20x_it.c`：`USBWakeUp_IRQHandler()` 清 EXTI18

### 3.4 应用层：CDC 描述符与回显逻辑

本工程 CDC 应用层在 `User/usb_cdc_app.c`：

- `cdc_acm_init(busid, reg_base)`：
  - 注册描述符
  - 添加接口（2 个 interface：控制接口 + 数据接口）
  - 添加端点（Bulk OUT、Bulk IN；通知端点由描述符提供）
  - 调用 `usbd_initialize(busid, reg_base, event_handler)`
- 回显逻辑：
  - 在 `usbd_cdc_acm_bulk_out()` 收到 OUT 数据后，原样写回 IN 端点

## 4. 构建与下载

### 4.1 xmake

构建产物默认输出：

- `build/cross/riscv/debug/CH32V208GBU.elf`
- 同目录下会生成：`.hex/.lst/.map`

常用命令：

```powershell
xmake f -p cross -a riscv -m debug -y
xmake
```

### 4.2 OpenOCD 下载

本工程的 OpenOCD 配置：

- `tools/wch-interface.cfg`
- VSCode task：`flash` 会下载 `build/cross/riscv/debug/CH32V208GBU.elf`

## 5. 功能与使用说明

### 5.1 CDC 端口说明

- Windows 枚举后会出现一个新的 `USB 串行设备 (COMx)`。
- COM 号不固定（会随系统/插拔变化），所以本工程的测试脚本支持按 `VID/PID` 自动寻找 CDC 串口。

### 5.2 “为什么打开串口监视没有数据”

当前固件 CDC 是回显模式，不会主动发送：

- 只开监视“纯读”：通常不会有任何输出。
- 发送任意字节：会立即收到设备回显。

如果需要“设备主动周期性输出日志到 CDC”，需要在固件里增加定时 `usbd_ep_start_write()` 发送逻辑（后续可扩展）。

## 6. 测试脚本：作用与用法

### 6.1 `scripts/cdc_echo_test.py`（推荐：验证 CDC 数据通路）

用途：

- 自动寻找 CDC 串口
- 发送一段字符串并等待回显
- 适合做“是否真的通了”的快速判断

用法示例：

```powershell
python scripts/cdc_echo_test.py --auto --vid 1A86 --pid FE0C --baud 115200 --message hello_cherryusb
```

说明：

- `--auto --vid --pid`：按 USB VID/PID 寻找串口，避免 COM 号变化导致失败

### 6.2 `scripts/serial_reader.py`（监视设备主动输出）

用途：

- 读取串口上由“设备主动发送”的数据并打印
- 适合用于你在固件里实现了 CDC 主动日志或其他主动输出后进行观测

用法示例：

```powershell
python scripts/serial_reader.py --auto --vid 1A86 --pid FE0C --baud 115200 --encoding utf-8
```

注意：

- 当前固件默认不会主动向 CDC 输出，所以这个脚本不一定能看到数据；它不会向设备发送字节。

## 7. VSCode Tasks（工程内置的一键入口）

`/.vscode/tasks.json` 已提供：

- `build`：xmake 配置并编译
- `flash`：OpenOCD 下载 elf
- `build and flash`：顺序执行 build + flash
- `serial monitor CDC(auto)`：用 VID/PID 自动找到 CDC 串口并监视（仅读取）
- `cdc echo test CDC(auto)`：用 VID/PID 自动找到 CDC 串口并做回显测试

## 8. 常见问题速查

1. 设备管理器没有新设备
   - 优先检查是否选对硬件核：`fsdev + 0x40005C00 + USB_LP_CAN1_RX0`
   - 检查 `EXTEN_USBD_PU_EN` 是否置位
   - 检查 PA11/PA12 IO 配置与硬件连接
   - 检查 USB 时钟是否为 48MHz

2. 有 COM 口但回显不通
   - 用 `scripts/cdc_echo_test.py` 验证（它会发送数据）
   - 若 `cfg` 计数一直不增长，说明未配置成功（需排查枚举与端点）

