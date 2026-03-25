# I2C 驱动功能分析与已知问题

## 1. 范围
- 核心文件：`bsp/inc/drv_i2c.h`、`bsp/src/drv_i2c.c`
- 关联使用：`bsp/board.c`、`lib/oled/*`、`lib/sc7a20/adapters/*`、`lib/sht40/adapters/*`
- 当前工程默认：`I2C1` 用 DMA 初始化，但适配层可按设备/事务显式调用 IT 或轮询接口。

## 2. 已实现功能
- 控制器：`I2C1`、`I2C2`
- 模式：`Polling`、`IT`、`DMA`
- API：
  - 原始读写：`bsp_i2c_write/read`
  - 寄存器读写：`bsp_i2c_write_register/read_register`
  - 字节读写：`bsp_i2c_write_byte/read_byte`
- 状态与错误管理：`State`、`ErrorCode`
- 回调：TX 完成、RX 完成、错误回调
- 恢复：软件脉冲恢复 / RCC 复位恢复，并通过 `InitCfg` 重建 I2C 配置

## 3. 驱动工作流程
### 3.1 初始化
- `bsp_i2c_init()` 完成 GPIO/I2C/NVIC 配置，并保存初始化参数到句柄。
- `bsp_i2c_dma_init()` 绑定 DMA 通道并打开 DMA 时钟。

### 3.2 轮询模式
- 阻塞式完成 `START -> ADDR -> DATA -> STOP`。
- 当总线主模式为 IT/DMA 时，轮询函数会临时关闭 `EVT/BUF` 中断，退出时恢复。

### 3.3 中断模式
- API 负责装载句柄并发起 START。
- `bsp_i2c_irq_handler()` 在 `SB/ADDR/TXE/RXNE/BTF` 事件里推进状态机。

### 3.4 DMA 模式
- API 装载 DMA、设置状态、发起 START。
- 地址阶段仍依赖 I2C 事件中断（`SB/ADDR`）进入主收发。
- DMA 完成由 DMA IRQ 收尾（STOP、ACK 恢复、状态回 IDLE）。

### 3.5 错误处理
- `i2c_error_handler()` 会统一清理：停 DMA、发 STOP、恢复 ACK、状态置 IDLE。
- 当前仅 `BERR/ARLO/TIMEOUT` 做重型恢复；`AF`（NACK）默认不重置总线。
- 重型恢复后通过 `i2c_reinit_from_handle()` 按保存参数重建 I2C。

## 4. 当前“模式共存”方式
- 模式配置是“按控制器”（例如 I2C1=DMA），不是“按设备”。
- 但适配层可显式调用 `*_it/*_dma/*_polling`，因此可实现“同总线不同设备不同模式”。
- 当前实际：
  - OLED：轮询 + IT
  - SC7A20：按总线模式分发
  - SHT40：适配层宏切换 IT/轮询

## 5. 已知问题
1. DMA 寄存器读默认不是 repeated-start。
- `bsp_i2c_read_register_dma()` 先轮询写寄存器，再独立 DMA 读。
- 对多数设备可用，但不是严格 repeated-start 事务。

2. 驱动层缺少显式总线互斥。
- 主要依赖 `State` 防冲突。
- 多模块并发请求时仍可能出现 `BUSY` 争用或时序抖动。

3. `bsp_i2c_get_state()` 在 DMA 下有副作用。
- 查询状态时可能触发 DMA 完成处理。
- 虽能兜底推进状态，但增加调试与推理复杂度。

4. 超时为循环计数，不是毫秒基准。
- `wait_flag/wait_event` 对 CPU 频率和优化等级敏感。

5. 适配层 `wait_idle` 为忙等。
- `sc7a20/sht40` 适配层都存在紧循环等待，错误重试时 CPU 占用偏高。

6. AF 策略需按设备场景校准。
- 当前 AF 不做重型恢复，避免过度复位。
- 某些设备上电窗口可能仍需上层补充恢复。

7. SC7A20 适配层会清掉寄存器 bit7。
- `reg = msgs[0].buf[0] & 0x7F` 可能丢失自动地址递增语义。

## 6. 建议优化
1. 增加统一总线锁或事务队列（跨适配层）。
2. 超时改为 tick 毫秒基准。
3. 为 DMA 寄存器读增加可选 repeated-start 实现。
4. 将“状态查询”和“DMA 兜底推进”拆成两个 API。
5. 适配层忙等中加入短延时/让出机制。
6. 对不同设备建立差异化错误恢复策略（尤其 AF）。

## 7. 结论
- 当前驱动功能完整，支持多模式和恢复机制。
- 近期问题主要来自“模式混用边界”和“设备差异化恢复策略”。
- 在同一总线实现 IT 与 DMA 共存是可行的，关键是：总线串行化、超时策略稳定、恢复策略按设备细化。
