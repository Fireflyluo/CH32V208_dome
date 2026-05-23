# Ad-Hoc-lib 配置说明

本文档说明 `Ad-Hoc-lib` 的统一配置入口、参数含义和调参建议，便于在不同工程复用同一协议库。

## 1. 配置分层（先看这个）

`Ad-Hoc-lib` 配置分两层：

1. 运行时配置（实例级，`adhoc_cfg_t`）  
   适合“每块板/每个节点不一样”的参数：
   - 身份：`domain_id` / `node_id` / `gateway_no`
   - 时序：`t1_us` / `t2_us` / `t3_us` / `t4_us`
   - 策略：`retry_max` / `network_window_us` / `regroup_interval_us`

2. 编译期配置（库级，`adhoc_config.h`）  
   适合“库内部策略和容量”参数：
   - 队列容量、缓存容量
   - 默认阈值（如默认窗口、默认重试）
   - 测试开关、存储段属性

建议原则：**业务策略优先放运行时，库资源与默认值放编译期**。

## 2. 配置入口与覆盖方式

- 统一入口：`include/adhoc_config.h`
- 覆盖方式：
  - 直接传宏：`-DADHOC_CONFIG_...=...`
  - 指定用户头：`-DADHOC_CONFIG_USER_HEADER="adhoc_user_config.h"`

示例（xmake）：

```lua
add_cflags(
    "-DADHOC_CONFIG_SM_DEFAULT_RETRY_MAX=20",
    "-DADHOC_CONFIG_DATA_TX_QUEUE_CAPACITY=24",
    "-DADHOC_CONFIG_USER_HEADER=\\\"adhoc_user_config.h\\\""
)
```

## 3. 运行时参数总览（`adhoc_cfg_t`）

| 参数 | 单位 | 作用 | 常用值建议 |
| --- | --- | --- | --- |
| `domain_id` | - | 组网域号（不同域互不通信） | 同一网络保持一致 |
| `node_id` | - | 节点唯一 ID | 保证全网唯一 |
| `gateway_no` | - | 网关编号（0~7） | 单网关用 `0` |
| `t1_us` | μs | 时隙宽度 | `2500` |
| `t2_us` | μs | 网关收集窗口 | `57500` |
| `t3_us` | μs | 侦听时长 | `62500` |
| `t4_us` | μs | 休眠时长 | `1937500` |
| `retry_max` | 次 | 入网/数据重试上限 | `30` |
| `network_window_us` | μs | 组网窗口 | `30000000`（30s） |
| `regroup_interval_us` | μs | 重组网周期，`0`=关闭 | 调试期建议先 `0` |

## 4. 编译期可配置项总览（`ADHOC_CONFIG_*`）

### 4.1 时序基础

| 宏 | 默认值 | 中文说明 |
| --- | --- | --- |
| `ADHOC_CONFIG_TMOS_TICK_US` | `625` | TMOS 基础 tick，`T1` 需是其整数倍 |
| `ADHOC_CONFIG_TIMING_M_RECOMMENDED_MIN` | `10` | `m_wait` 推荐最小值（告警参考） |
| `ADHOC_CONFIG_TIMING_M_RECOMMENDED_MAX` | `1000` | `m_wait` 推荐最大值（告警参考） |

### 4.2 状态机（SM）

| 宏 | 默认值 | 中文说明 |
| --- | --- | --- |
| `ADHOC_CONFIG_SM_GATEWAY_ID_MAX` | `1000000` | 网关 ID 上界（含） |
| `ADHOC_CONFIG_SM_BEACON_ID_MIN` | `1000001` | 信标 ID 下界（含） |
| `ADHOC_CONFIG_SM_DEFAULT_RETRY_MAX` | `30` | 未显式配置时的默认重试上限 |
| `ADHOC_CONFIG_SM_DEFAULT_NETWORK_WINDOW_US` | `30000000` | 未显式配置时默认组网窗口（30s） |
| `ADHOC_CONFIG_SM_RSSI_STRONG_MIN_DBM` | `-65` | 强信号门限 |
| `ADHOC_CONFIG_SM_RSSI_MEDIUM_MIN_DBM` | `-80` | 中信号门限（低于此归弱） |
| `ADHOC_CONFIG_SM_JOIN_LEVEL_MAX` | `254` | 最大允许组网层级 |
| `ADHOC_CONFIG_SM_UPSTREAM_BINDING_CAPACITY` | `6` | 上级绑定表容量（与编号空间匹配） |
| `ADHOC_CONFIG_SM_NEIGHBOR_CACHE_CAPACITY` | `6` | 邻居缓存 Top-K 容量 |
| `ADHOC_CONFIG_SM_UPSTREAM_LOSS_CYCLES` | `3` | 上级失效判定周期数（`3*T5`） |
| `ADHOC_CONFIG_TEST_DROP_JOIN_CONFIRM` | `0` | 测试注入开关（丢确认） |

### 4.3 组网确认队列

| 宏 | 默认值 | 中文说明 |
| --- | --- | --- |
| `ADHOC_CONFIG_REPLY_LIST_CAPACITY` | `24` | 待确认下级记录容量 |
| `ADHOC_CONFIG_REPLY_MAX_PER_FRAME` | `5` | 单帧最多确认条目数（当前协议模型） |
| `ADHOC_CONFIG_SM_DOWNSTREAM_BINDING_CAPACITY` | `24` | 下级绑定关系容量 |

### 4.4 数据面

| 宏 | 默认值 | 中文说明 |
| --- | --- | --- |
| `ADHOC_CONFIG_DATA_DEDUP_CAPACITY` | `64` | 去重表容量 |
| `ADHOC_CONFIG_DATA_ACK_QUEUE_CAPACITY` | `32` | 待发送 ACK 队列容量 |
| `ADHOC_CONFIG_DATA_TX_QUEUE_CAPACITY` | `16` | 待发送数据队列容量 |
| `ADHOC_CONFIG_DATA_TX_REPORT_QUEUE_CAPACITY` | `16` | 发送结果上报队列容量 |
| `ADHOC_CONFIG_DATA_TX_RETRY_MAX` | `30` | 数据面默认重试上限 |
| `ADHOC_CONFIG_DATA_DEFAULT_DEDUP_WINDOW_MS` | `6h` | 默认去重窗口 |
| `ADHOC_CONFIG_DATA_GATEWAY_ID_MAX` | `1000000` | 数据面网关 ID 上界 |

### 4.5 CRC8 存储段

| 宏 | 默认值 | 中文说明 |
| --- | --- | --- |
| `ADHOC_CONFIG_CRC8_TABLE_STORAGE` | 平台相关 | RISC-V GCC 默认放 `.flash1_rodata`，其他平台无段属性 |

## 5. 约束与调参建议

1. `adhoc_config.h` 内含基础边界检查（如容量必须 `>0`），非法配置会直接编译报错。  
2. 与帧格式强耦合的常量不建议改成可配置项，避免协议兼容性漂移。  
3. 先调运行时参数（`adhoc_cfg_t`），再调编译期容量（`ADHOC_CONFIG_*`）。  
4. 若出现“窗口期抖动回退”，优先检查 `T2/T5` 收发节拍与链路 busy，再考虑增大 `retry_max` 或缓存容量。  

## 6. 最小迁移步骤

1. 保持 `adhoc_cfg_t` 初始化路径不变；  
2. 把构建脚本散落宏收敛到 `ADHOC_CONFIG_*`；  
3. 先单板验证，再做双板/多板联调。  
