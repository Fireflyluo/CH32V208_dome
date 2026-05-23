# CherryUSB vs 官方 USBLIB 对比分析

> 对比条件：两者均运行于 CH32V208 (RV32, 128KB Flash, 64KB RAM)，均启用 CDC ACM 虚拟串口，
> 均使用 WCH RISC-V GCC 工具链，Debug 模式 `-O0 -g`。

---

## 1. 代码体积

### 1.1 USB 库源码规模

| 维度 | CherryUSB | 官方 USBLIB |
|------|-----------|-------------|
| USB 库 C 源文件数 | **3 个** | **12 个** |
| USB 库总代码行数 | **~2,086 行** | **~3,093 行** |
| 分层结构 | core / class / port 三层 | USB-Driver / CONFIG 两层 |

**CherryUSB 各文件：**
| 文件 | 行数 | 角色 |
|------|------|------|
| `usbd_core.c` | 1,390 | 设备核心：setup 包解析、标准请求、描述符管理、EP0 状态机 |
| `usbd_cdc_acm.c` | 134 | CDC ACM 类处理器 |
| `usb_dc_fsdev.c` (或 `usb_dc_usbfs.c`) | 452 / 330 | FSDEV/USBFS 硬件端口层 (PMA、寄存器、ISR) |

**官方 USBLIB 各文件：**
| 文件 | 行数 | 角色 |
|------|------|------|
| `usb_core.c` | 867 | 控制端点状态机、标准请求处理 |
| `usb_regs.c` | 829 | 寄存器访问函数封装 (~40 个函数) |
| `usb_prop.c` | 386 | 设备属性表、CDC 类请求 |
| `usb_istr.c` | 178 | 主 USB 中断处理 |
| `hw_config.c` | 157 | USB 时钟/GPIO/中断配置 |
| `usb_int.c` | 116 | CTR 中断高低优先级处理 |
| `usb_pwr.c` | 195 | USB 电源管理 (挂起/唤醒) |
| `usb_desc.c` | 113 | USB 描述符定义 |
| `usb_endp.c` | 87 | 端点回调 + 数据收发 |
| `usb_mem.c` | 65 | PMA <-> 用户缓冲区拷贝 |
| `usb_sil.c` | 65 | 简化接口层 |
| `usb_init.c` | 35 | USB 初始化 |

### 1.2 编译后二进制体积 (Debug -O0)

| 尺寸项 | CherryUSB | 官方 USBLIB |
|--------|-----------|-------------|
| **ELF 文件** | **141,668 B (138 KB)** | **405,244 B (396 KB)** |
| **HEX 文件** | **94,584 B (92 KB)** | **220,458 B (215 KB)** |
| **实际 ROM (Flash)** | **~32 KB** | **~78 KB** |
| **实际 RAM** (data+bss+stack) | **~4.6 KB** | **~18.4 KB** |

> 注意：官方项目 ROM 中包含 BLE 协议栈 (`libwchble.a` ~15 KB)、OLED 驱动、SC7A20 加速度计驱动、
> SHT40 温湿度驱动、I2C 驱动、TMOS 调度器、软件定时器、环形缓冲区等，因此整体尺寸大很多。
> CherryUSB 项目仅包含 USB CDC 回显功能 + SDK 外设库。

**可比的纯 USB 栈代码预估：**
- CherryUSB USB 部分 ROM 占用：**约 8-10 KB** (根据 map 符号统计)
- 官方 USBLIB USB 部分 ROM 占用：**约 12-15 KB** (含 hw_config,描述符,中断等)

### 1.3 ELF Sections 对比

**CherryUSB ELF Sections:**
| Section | 大小 | 位置 |
|---------|------|------|
| `.init` | 4 B | Flash |
| `.vector` | 316 B | Flash |
| `.text` (含 .rodata) | **31,964 B** | Flash |
| `.data` | 184 B | Flash → RAM |
| `.noncacheable` | 1,148 B | RAM (USB 缓冲区) |
| `.bss` | 1,336 B | RAM |
| `.stack` | 2,048 B | RAM |

**官方 USBLIB ELF Sections:**
| Section | 大小 | 位置 |
|---------|------|------|
| `.init` | 4 B | Flash |
| `.vector` | 316 B | Flash |
| `.highcode` | 5,540 B | Flash (含 BLE 代码) |
| `.text` | **72,008 B** | Flash |
| `.data` | 400 B | Flash → RAM |
| `.bss` | 15,972 B | RAM |
| `.stack` | 2,048 B | RAM |

---

## 2. 架构对比

### 2.1 分层设计

| 方面 | CherryUSB | 官方 USBLIB |
|------|-----------|-------------|
| **架构** | 标准三层: core → class → port | 两层: USB-Driver + CONFIG |
| **职责分离** | 清晰，core 与硬件无关 | 耦合较紧 |
| **多实例支持** | 内置 (`busid` 参数) | 不支持，全局变量 |
| **OTG 支持** | 有 (`usbotg_core.c`) | 无 |
| **Host 栈** | 独立 (`usbh_core.c`)，可选编译 | 无 |
| **描述符管理** | 注册回调函数，灵活 | 编译期静态定义 |
| **跨平台** | 支持 10+ MCU 系列 | 仅 CH32 |

### 2.2 CDC ACM 功能实现

| 功能 | CherryUSB | 官方 USBLIB |
|------|-----------|-------------|
| Set/Get Line Coding | ✅ | ✅ |
| Set Control Line State (DTR/RTS) | ✅ | ✅ |
| Send Break | ✅ | ✅ |
| Serial State 通知 | 需应用层实现 | 需应用层实现 |
| 多接口 CDC | 支持 (IAD) | 支持 (IAD) |
| 数据流控 | 用户回调 | `USB_SIL_Read/Write` |
| 端点数量 | 3 (OUT + IN + INT) | 3 (OUT + IN + INT) |
| 最大包长 (FS) | 64 B | 64 B |

### 2.3 中断处理

| 方面 | CherryUSB | 官方 USBLIB |
|------|-----------|-------------|
| ISR 处理 | `USBD_IRQHandler` (单函数) | `USB_Istr()` + CTR_LP + CTR_HP |
| EP0 状态机 | core 层统一处理 | `usb_core.c` + `usb_prop.c` 配合 |
| 事件驱动 | `usbd_event_*_handler` 回调系列 | 直接函数调用 |
| 中断嵌套 | 裸机单级 | 支持高/低优先级 CTR |

### 2.4 RAM 使用

| 方面 | CherryUSB | 官方 USBLIB |
|------|-----------|-------------|
| EP0 请求缓冲区 | 512 B (可配) | 固定 |
| CDC 数据缓冲区 | `noncacheable` 区分配 | `usb_conf.h` 定义 PMA 地址 |
| PMA 缓冲区管理 | `usb_dc_fsdev.c` 内 | `usb_mem.c` `UserToPMABufferCopy` |
| 全局状态结构体 | `g_usbd_core` + `g_ch32_usbfs_udc` | 多个全局变量分散 |

---

## 3. 优缺点总结

### CherryUSB

**优点：**
- 代码体积小、层次清晰，适合学习与二次开发
- 跨平台，一套 API 可移植到其他 MCU
- 同时支持 Device 和 Host 栈
- 事件驱动架构，扩展新 class 只需实现回调
- 活跃的开源社区 (GitHub cherrry-io)

**缺点：**
- 应用层需自行管理描述符表和数据收发
- 文档相对较少 (中文为主)
- CH32 特定硬件层 (fsdev) 仍有待完善

### 官方 USBLIB

**优点：**
- WCH 官方维护，与 CH32 硬件深度适配
- 集成度高，`Set_USBConfig()` + `USB_Init()` 即完成初始化
- 配套 TMOS 和 BLE 协议栈，适合复合产品
- 中断优先级管理更精细 (CTR_LP/CTR_HP)
- 示例完善，兼容 MounRiver Studio

**缺点：**
- 代码量大 (寄存器封装冗余，`usb_regs.c` 829 行含 40+ 函数)
- 只支持 CH32 系列，不可移植
- 无 Host 栈支持
- 全局变量多，不适合多实例
- 耦合度高，理解全貌需阅读多个文件

---

## 4. 场景建议

| 场景 | 推荐 |
|------|------|
| 仅需要 CDC 串口，追求精简 | **CherryUSB** (~32 KB ROM) |
| 需要 BLE + USB 复合设备 | **官方 USBLIB** (原生 BLE 支持) |
| 跨平台项目 (如未来换 MCU) | **CherryUSB** |
| 快速出原型，MRS 生态 | **官方 USBLIB** |
| 学习 USB 协议栈架构 | **CherryUSB** (清晰分层) |
| 产品稳定优先 | **官方 USBLIB** (官方支持) |
