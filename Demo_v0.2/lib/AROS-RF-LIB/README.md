# AROS-RF 协议栈库

## 简介

AROS-RF 是一个轻量级的无线射频协议栈，专为嵌入式系统设计，支持多平台架构。该协议栈提供了完整的 RF 通信功能，包括时隙管理、数据收发、事件处理等核心功能。

## 支持的平台

- **RISC-V**: CH32V208, CH573 等（需要定义 `ARF_RISCV` 宏）
- **ARM**: ARM Cortex-M 系列（无需额外宏定义）
- **Windows**: PC 模拟平台（定义 `ARF_WIN` 宏）

## 目录结构

```
AROS-RF-LIB/
├── README.md              # 本文件
├── include/               # 头文件目录
│   └── aros_rf.h         # 协议栈接口定义
├── src/                   # 源文件目录
│   ├── aros_rf.c         # RF 核心协议层
│   └── tag.c             # 标签应用示例
└── examples/              # 示例代码
    ├── ch32v208_example.c  # CH32V208 使用示例
    └── pc_sim_example.c   # PC 模拟示例
```

## 快速开始

### 1. 集成到项目

将 `AROS-RF-LIB` 文件夹复制到您的项目根目录。

### 2. 配置编译器

在您的项目配置中添加：

**Include 路径:**
```
AROS-RF-LIB/include/
```

**Source 文件:**
```
AROS-RF-LIB/src/aros_rf.c
AROS-RF-LIB/src/tag.c    (可选，仅用于示例)
```

### 3. 定义平台宏

根据您的目标平台，定义相应的宏：

**RISC-V 平台（如 CH32V208/CH573）:**
```c
#define ARF_RISCV
```

**Windows 平台（PC 模拟）:**
```c
#define ARF_WIN
```
或者让编译器自动检测（会自动定义 `ARF_WIN`）

**ARM 平板:**
无需定义宏

## 核心特性

### 时钟配置

协议栈根据平台自动配置时钟周期：

| 平台 | 时钟周期 (μs) | 说明 |
|------|--------------|------|
| RISC-V | 625 | TMOS 时钟 |
| Windows | 1000 | Windows 模拟时钟 |
| ARM | 100 | ARM 通用时钟 |

### 消息格式

```c
#define ARF_MsgN        32      // 消息数据长度
#define ARF_MsgTC       (ARF_MsgN + 2)  // 时间戳位置
#define ARF_MsgBufN     (ARF_MsgN + 4)  // 总缓冲区长度（含 RSSI 和锁定标志）
```

消息缓冲区结构：
- `[0:31]` - 数据区（32 字节）
- `[32]` - RSSI 信号强度
- `[33]` - 锁定标志
- `[34:35]` - 时间戳（16 位）

## API 接口

### 系统初始化

```c
// 系统初始化（需要在 arf_Init 之前调用）
void arf_SysInit(void);

// 协议栈初始化
void arf_Init(void);
```

### 主循环

```c
// 主循环处理（在 while(1) 中调用）
void arf_proc(void);

// 完整的主循环封装
void arf_MainLoop(void);
```

### 数据收发

```c
// 启动接收
void arf_RxStart(void);

// 检查接收是否完成
uint8_t* arf_isRxFinish(void);

// 发送数据
int arf_TxSend(uint8_t buf[], int len);
```

### 时钟和延时

```c
// 获取 16 位时钟
uint16_t arf_get_tc16(void);

// 获取 32 位时钟
uint32_t arf_get_tc(void);

// 毫秒级延时
void arf_delayms(int tm_ms);
```

### 事件和任务

```c
// 注册任务
uint8_t arf_register(arf_task_fc taskf);

// 触发事件
void arf_event(uint8_t task_id, uint16_t evt);

// 延时触发事件
void arf_event_at(uint8_t task_id, uint16_t evt, int tc);
```

### 定时器

```c
// 设置定时器（最多 3 个）
void arf_set_timer(int tmr, int t_us, arf_proc_fc tmrf);
```

参数说明：
- `tmr`: 定时器编号（0-2）
- `t_us`: 定时周期（微秒），正数表示周期定时，负数表示单次定时
- `tmrf`: 定时回调函数

### LED 控制

```c
// LED 亮指定时间
void arf_led_on(int tm_ms);

// 设置 LED 模式
void arf_led_mod(int mod);

// LED 闪烁 ID
int arf_led_id(int id);
```

LED 模式：
- `ARF_LED_RX` (0x01) - 接收时闪烁
- `ARF_LED_TX` (0x02) - 发送时闪烁
- `ARF_LED_SEND` (0x04) - 发送完成后闪烁

### 回调函数

```c
// 设置处理函数
void arf_set_procfunc(arf_proc_fc procf);

// 设置新数据回调函数
void arf_set_newdatfunc(arf_newdat_fc newdatf);
```

## HAL 硬件抽象层（RISC-V 平台）

### 必需的 HAL 接口

如果使用 RISC-V 平台（如 CH32V208），您需要实现以下接口：

#### LED 控制

```c
void LED_on(void);   // 点亮 LED
void LED_off(void);  // 熄灭 LED
```

#### RF 底层接口

```c
// RF 配置
void RF_Config(rfConfig_t *config);

// RF 接收
void RF_Rx(uint8_t *buf, int len, uint8_t rxType, uint8_t txType);

// RF 发送
void RF_Tx(uint8_t *buf, int len, uint8_t txPower, uint8_t txLLEMode);

// RF 关闭
void RF_Shut(void);

// RF 角色初始化
void RF_RoleInit(void);
```

#### TMOS 接口（RTOS）

```c
// 获取系统时钟
uint32_t TMOS_GetSystemClock(void);

// 设置事件
void tmos_set_event(uint8_t task_id, uint16_t evt);

// 启动延时任务
void tmos_start_task(uint8_t task_id, uint16_t evt, int tc);

// 注册事件处理器
uint8_t TMOS_ProcessEventRegister(void (*func)(uint8_t, uint16_t));

// 系统处理
void TMOS_SystemProcess(void);

// 内存设置
void tmos_memset(void *dst, uint8_t val, uint32_t len);
```

## 使用示例

### 基本使用流程

```c
#include "aros_rf.h"

int main(void) {
    // 1. 系统初始化
    arf_SysInit();
    
    // 2. 协议栈初始化
    arf_Init();
    
    // 3. 设置回调函数（可选）
    arf_set_procfunc(my_proc_func);
    arf_set_newdatfunc(my_newdat_func);
    
    // 4. 设置定时器（可选）
    arf_set_timer(0, 1000000, my_timer_callback);  // 1 秒定时器
    
    // 5. 主循环
    arf_MainLoop();
    
    return 0;
}
```

### 数据收发示例

```c
void send_data_example(void) {
    uint8_t txbuf[ARF_MsgN];
    
    // 准备数据
    memset(txbuf, 0, ARF_MsgN);
    txbuf[0] = 0x01;  // 示例数据
    
    // 发送数据
    arf_TxSend(txbuf, ARF_MsgN);
}

void receive_data_example(void) {
    uint8_t *rxbuf;
    
    // 启动接收
    arf_RxStart();
    
    // 等待接收完成
    while ((rxbuf = arf_isRxFinish()) != NULL) {
        // 处理接收到的数据
        uint8_t rssi = arf_RSSI(rxbuf);
        uint16_t tc = arf_u16toi(rxbuf + ARF_MsgTC);
        
        // 处理数据...
    }
}
```

## 编译选项

### 推荐编译器选项

**C 编译器:**
```
-Wall -O2 -g
```

**预定义宏（根据平台）:**
```
ARF_RISCV     (RISC-V 平台)
ARF_WIN       (Windows 平台，可自动检测)
```

### 内存要求

- **Flash**: 约 8-12 KB（取决于配置）
- **RAM**: 约 2-4 KB（不包含应用缓冲区）

## Tag 节点 ID 设计说明

### `tg_id` 类型与节点数量

- 历史版本 `tag.c` 中曾使用 `static int16_t tg_id = 0;`，当前版本对外接口使用 `uint8_t tg_id`。
- 这两种类型的变化**不会**改变协议可用节点数量。
- 协议可用节点数由时隙与缓存设计决定，核心约束是：
  - `TG_N = 21`（节点索引 `0~20`）
  - `TG_MAX_ID = 20`
  - `recv_dat[TG_N][ARF_MsgN]` 的缓存边界检查
- 因此，当前协议的有效节点 ID 范围固定为 `0~20`，共 `21` 个节点。

### 为什么使用 `uint8_t`

- `uint8_t` 可避免负数 ID 输入带来的隐患，语义上更符合“节点编号”。
- 配合 `TG_MAX_ID` 的 sanitize 逻辑，可以稳定限制 ID 在合法范围内。

### 如果要扩容节点数量

- 不能仅修改 `tg_id` 类型。
- 需要同步调整：`TG_N`、`TG_MAX_ID`、层级参数（如 `TG_LV_N`）以及 `lv_slot[]` 时隙编排与缓存大小。

## 注意事项

1. **平台兼容性**: 确保根据目标平台定义正确的宏
2. **HAL 实现**: RISC-V 平台需要完整实现 HAL 接口
3. **时序精度**: 协议栈依赖精确的时序，确保系统时钟配置正确
4. **中断处理**: RF 回调函数中不要直接调用 RF API，使用事件机制
5. **内存对齐**: RISC-V 平台需要注意内存对齐（4 字节对齐）

## 技术支持

如有问题或建议，请联系开发团队。

## 版本历史

- **V1.0** (2025/11/12) - 初始版本
- **V1.1** (2026/01/12) - 添加标签应用示例

## 许可证

Copyright (c) 2025, Angran Inc. All rights reserved.

Angran: alive and robust
