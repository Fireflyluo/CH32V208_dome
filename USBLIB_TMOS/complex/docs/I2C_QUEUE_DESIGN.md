# I2C 总线队列化实现原理（面向当前工程）

## 1. 背景与目标
当前工程同一条 I2C 总线同时服务 OLED（大量刷新）、SC7A20（周期采样）、SHT40（周期测量）。
当多个模块几乎同时发起事务时，容易出现：
- `BUSY` 返回导致上层重试风暴
- 一个设备事务未完成时另一个设备插入，造成时序异常
- 错误恢复后状态不一致，出现“后续全错”的连锁问题

队列化目标：
- 同一总线事务严格串行化
- 上层从“直接抢总线”改为“提交请求”
- 支持优先级、超时、取消、统一错误恢复

## 2. 总体架构
建议新增总线仲裁层（Bus Arbiter），位于“设备适配层”与“drv_i2c”之间。

调用关系：
`SC7A20/SHT40/OLED adapter -> i2c_bus_arbiter -> drv_i2c`

核心原则：
- 一个总线（如 I2C1）一个队列
- 一个时刻仅有一个 in-flight 事务
- 所有模式（IT/DMA/Polling）都通过同一入口排队

## 3. 核心数据结构（建议）

```c
// 事务方向/类型
typedef enum {
    I2C_REQ_WRITE,
    I2C_REQ_READ,
    I2C_REQ_WRITE_REG,
    I2C_REQ_READ_REG,
} i2c_req_type_t;

// 可选优先级
typedef enum {
    I2C_PRIO_HIGH = 0,
    I2C_PRIO_NORMAL = 1,
    I2C_PRIO_LOW = 2,
} i2c_req_prio_t;

// 请求完成回调
typedef void (*i2c_req_done_cb_t)(void *user, int status);

typedef struct {
    uint32_t id;
    i2c_num_t bus;
    i2c_req_type_t type;
    uint8_t dev_addr;
    uint8_t reg;
    uint8_t *buf;
    uint16_t len;
    uint8_t mode_hint;      // IT/DMA/POLLING（可选）
    i2c_req_prio_t prio;
    uint32_t enqueue_tick;
    uint32_t timeout_ms;
    i2c_req_done_cb_t done;
    void *user;
} i2c_req_t;
```

实现可先用环形缓冲区：
- `queue[QUEUE_SIZE]`
- `head/tail/count`
- `current_req`（当前执行）

## 4. 状态机

### 4.1 Arbiter 状态
- `IDLE`：无事务执行
- `RUNNING`：有事务执行中
- `RECOVERING`：总线恢复中

### 4.2 执行流程
1. 上层 `submit(req)` 入队
2. 若状态 `IDLE`，立即 `start_next()`
3. `start_next()` 取队首，按 `mode_hint` 调 `bsp_i2c_*`
4. 周期 `poll()` 或中断回调检查完成：
   - 成功：回调 `done(0)`，出队，继续下一条
   - 超时/错误：统一 `recover`，回调错误，继续下一条

## 5. 与现有驱动的对接策略

### 5.1 发起执行
根据请求类型映射到现有 API：
- `WRITE_REG + DMA` -> `bsp_i2c_write_register_dma`
- `READ_REG + IT` -> `bsp_i2c_read_register_it`
- `READ_REG + DMA` -> `bsp_i2c_read_register_dma`
- 其他同理

### 5.2 完成判定
优先复用现有：
- `bsp_i2c_get_state(bus) == I2C_STATE_IDLE`
- `bsp_i2c_get_error(bus)` 判定结果

### 5.3 错误恢复
统一在 arbiter 内执行：
- `bsp_i2c_recover(bus)`
- 清理当前请求
- 记录错误计数（可用于降级策略）

## 6. 优先级与公平性
建议最小实现：
- 先 FIFO，避免复杂度
- 后续加“高优先级插队”但限制连续插队次数，避免低优先级饿死

实践建议：
- SC7A20 采样：`HIGH`
- SHT40 采样：`NORMAL`
- OLED 刷屏块：`LOW`

## 7. OLED 大流量的配合策略
队列化后建议把 OLED 刷屏拆片：
- 一页/半页为一个请求
- 每片完成后允许高优先级请求插入

效果：
- OLED 仍可 DMA 提速
- 传感器采样延迟显著降低
- 避免单次超长 DMA 占满总线

## 8. 取消与超时

### 8.1 取消
- 支持 `cancel_by_owner(owner_id)` 或 `cancel_by_dev(dev_addr)`
- 对未执行请求：直接出队回调 `-ECANCELED`
- 对执行中请求：标记取消，完成后不再继续后续链

### 8.2 超时
- 每个请求携带 `timeout_ms`
- 超时后统一 recover 并上报 `-ETIMEDOUT`

## 9. 线程/中断安全
当前工程若主循环 + 中断并发访问队列，需最小临界区保护：
- 入队/出队/head-tail 更新时关中断或使用轻量锁
- 避免在中断里做复杂逻辑，建议中断只置标志，主循环 `poll()` 完成收尾

## 10. 分阶段落地计划（推荐）

### 阶段 A：最小可用
- 单总线 FIFO 队列
- 仅支持 submit + poll
- 成功/失败回调
- 统一 recover

### 阶段 B：性能与鲁棒性
- 模式提示（IT/DMA/POLLING）
- 每请求超时
- 统计与诊断（队列长度、超时计数、平均等待）

### 阶段 C：体验优化
- 优先级队列
- OLED 分片与可抢占
- 取消接口

## 11. 验收标准
- 长时间运行无 `BUSY` 风暴
- 传感器数据连续稳定（无大面积 0 或卡死）
- OLED 刷屏期间传感器仍按期更新
- 出现错误后可自动恢复并继续处理后续队列

## 13. 阶段 C 当前实现状态（已落地）
- 已实现优先级队列（`HIGH > NORMAL > LOW`）
- 已实现取消接口：
  - `i2c_bus_cancel_by_dev(bus, dev_addr)`
  - `i2c_bus_cancel_by_owner(bus, owner_id)`
- 设备默认优先级与 owner：
  - SC7A20：`HIGH`，`owner_id=1`
  - SHT40：`NORMAL`，`owner_id=2`
  - OLED：配置/小写入 `NORMAL`，大块 DMA 刷屏 `LOW`，`owner_id=3`

## 14. 在线调试命令（通过 CDC 接收）
当前 `main.c` 已增加命令解析，支持：
- `i2c_stats show`
- `i2c_stats reset`
- `i2c_cancel owner <id>`
- `i2c_cancel dev <addr>`

示例：
- `i2c_cancel owner 3`
- `i2c_cancel dev 0x3c`

返回格式示例：
- `CMD OK: cancel owner 3 -> 2`
- `CMD OK: cancel dev 0x3C -> 1`

## 12. 对当前代码的改动边界建议
- 新增：`bsp/inc/i2c_bus_arbiter.h`、`bsp/src/i2c_bus_arbiter.c`
- 设备适配层（SC7A20/SHT40/OLED）改为提交请求，不直接调用 `bsp_i2c_*`
- `drv_i2c.c` 尽量少改，保持底层稳定

---

这份文档是“实现原理 + 可落地设计”。
如果你确认方向，我下一步可以按“阶段 A”直接给出最小可运行代码框架。
