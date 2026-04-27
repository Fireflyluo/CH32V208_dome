# Ad-Hoc-lib 使用说明（v0.1）

本文档说明 `Ad-Hoc-lib` 在当前工程中的推荐接入方式，以及当前实现边界。

## 1. 设计目标

- 协议层独立于具体射频驱动。
- 链路层只负责“收一帧 / 发一帧 / 提供时间与随机数”。
- 任务层负责周期驱动、角色配置、观测与业务数据注入。

## 2. 典型接入结构

当前工程采用以下调用链：

`ad_hoc_task -> adhoc_node -> adhoc_sm / adhoc_data_plane -> adhoc_link_ops_t -> adhoc_link_aros -> AROS-RF-LIB`

其中：

- `adhoc_node` 是应用层唯一入口。
- `adhoc_sm` 负责组网类 `A` 帧。
- `adhoc_data_plane` 负责数据类 `D` 帧。
- `adhoc_link_ops_t` 是协议层与链路层的边界。

## 3. 初始化步骤

1. 准备 `adhoc_cfg_t`
   - `domain_id`：域号
   - `node_id`：本节点 ID
   - `gateway_no`：网关编号
    - `t1_us ~ t4_us`：基础时间参数
   - `retry_max`：未确认态/数据面重试上限
   - `network_window_us`：网关组网时间窗，当前默认建议 `30000000`
   - `regroup_interval_us`：重组网本地计时，`0` 表示关闭
2. 准备一块静态内存，大小取自 `adhoc_node_required_size()`
3. 准备链路操作表 `adhoc_link_ops_t`
4. 调用 `adhoc_node_init()`
5. 角色切换：
   - 网关：`adhoc_node_set_role(..., ADHOC_ROLE_GATEWAY)`
   - 信标：`adhoc_node_set_role(..., ADHOC_ROLE_BEACON)`

## 4. 周期驱动建议

每个任务周期建议按以下顺序执行：

1. 链路层轮询接收；若得到完整 `32B` 帧，则调用 `adhoc_node_on_rx()`
2. 以当前时间调用 `adhoc_node_poll(now_us)`
3. 若 `adhoc_node_fetch_tx()` 返回成功，则将该帧交给链路层发送
4. 信标业务需要上报时，调用 `adhoc_node_submit_data()`
5. 周期读取 `adhoc_node_fetch_data_tx_report()`，更新业务统计或日志
6. 若需要运行态观测，可周期调用 `adhoc_node_get_runtime_status()`，读取 `state/joined_level/upstream_no/upstream_last_seen_us/gateway_network_end_us/network_lock_end_us`

## 5. 当前已具备能力

- 固定 `32B` 帧编解码与 `CRC8`
- `T5/T6/n/m` 时序关系校验
- `ST1/U1/UN/C1/CN` 组网状态机
- 网关 `A V=0` 周期发射与 `T2` 收集窗口
- 网关组网时间窗控制：默认 `30s` 到期后停止 `A V=0`
- 组网确认列表（最多 `6` 条）
- 轻量邻居表 `Top-K Cache`（当前 `K=6`），用于多网关候选缓存与优选
- 信标侧“方向结束时刻继承”：首次观测某个上行方向时记录本地 `network_end_us`，确认后到期自动锁定停发
- 上级失效释放：若在组网窗口未结束前连续 `3*T5` 未再收到当前上级 `A` 帧，则显式回退 `ST1`
- `D` 帧、`ID+No` 去重、网关 ACK（最多 `4` 条）
- 数据面按 `source.node_id + No` 做去重/ACK 命中，不把转发中会变化的 `id_flag` 纳入唯一键
- `source_id_flag` 的最小转发约束：原发数据首跳改写为“再转发上级编号”，转发数据需命中当前可再转发上级编号
- 已支持“监听确认”：监听到同一 `source.node_id + No` 的上级转发/再转发后停止本条继续重发
- 转发 `D` 帧使用当前组网状态同步得到的 `joined_level/upstream_gateway_no/upstream_no`
- `upstream_no` 由状态机内的上级编号绑定表（`0..5`）分配，不再在数据面固定为 `0`
- 组网 `A` 帧确认按请求 `Flag(0..5)` 定向回填，`Flag=6` 请求仅忽略，未确认节点仅在命中自身且 `Flag=N` 一致时转确认
- 锁定后仅处理与当前 `upstream_gateway_no` 一致的下级组网请求
- 本地重组网计时接口 `regroup_interval_us`（当前工程默认关闭）
- 信标侧数据转发、重试与结果回传
- CH32V208 `port` 封装（时间/随机/临界区）

## 6. 当前未完成项

以下仍需按设计文档继续推进：

- 全网统一绝对结束时刻的显式传播（当前为“首次观测方向”的本地时间）
- 重组网全局对齐策略（当前仅有本地计时骨架）
- `docs/t07任务约束` 中“`A` 帧前 4B 注入 Epoch”方案与当前 `6` 条确认载荷布局冲突，当前仅保留为协议演进候选项
- `source_id_flag` 的多上级编号完整转发语义
- 多网关方向锁定释放闭环

因此，现阶段更适合把本库视作“最小可联调协议基线”，而不是最终完整协议实现。

## 7. 当前工程中的最小业务约定

在 `2.4G_RF` 工程当前主路径中：

- 只有信标角色主动调用 `adhoc_node_submit_data()`
- `source_id_flag` 当前固定为 `0`
- `upstream_no` 当前由状态机根据 `Top-K Cache + 绑定表(0..5)` 分配，应用层不再直接指定上级编号
- `user[18]` 由 `sensor_task` 快照编码生成
- `OLED/串口/LED` 统一读取 `ad_hoc_task` 状态进行观测

这部分是当前工程联调约定，不应替代协议规范本身。

## 8. 调试与板测建议

- 当前工程已将 `adhoc_node_get_runtime_status()` 同步到串口 `adhoc sm ...` 日志，建议优先观察：
  - `st/lvl/retry`：当前状态机状态、级别与重试次数
  - `up_gw/up_no/up_age`：已选方向与上级最近存活时间
  - `gw_start/gw_lock/gw_left`：网关组网窗口是否启动、是否关闭、剩余时间
  - `net_act/net_closed/net_left`：信标方向锁定是否激活、是否到期关闭、剩余时间
- 当前已完成的单板验证（`2026-04-26`）：
  - 条件：`1` 块板、网关角色、串口 `COM8/115200`
  - 现象：`gw_left` 倒计时约 `30s` 后归零，`gw_lock` 由 `0` 变 `1`
  - 现象：`gw_lock=1` 后 `tx` 与链路层发送请求计数停止增长
  - 结论：网关侧 `T07`“时间窗到期停发 `A V=0`”已经具备硬件证据
- 当前已完成的双板验证（`2026-04-26`）：
  - 条件：板1为网关、板2为 `RF_TG_ID=1` 信标，仅观测板2 `COM3/115200`
  - 现象：板2从 `ST1` 进入 `C1`（`st=3`、`lvl=1`、`up_id=1`、`net_act=1`）
  - 现象：`adhoc data ack` 从 `4` 增长到 `11`，说明最小入网与数据 ACK 闭环已跑通
  - 现象：随后板2又回退到 `ST1`（`up_id=0`、`net_act=0`）
  - 结论：可确认“最小入网 + 数据 ACK”主路径成立；回退与 `3*T5` 上级失效逻辑一致，但仍建议下一轮同步观察板1状态
- 仍待后续双板/多板验证：
  - 信标方向结束时刻到期后的锁定停发
  - 连续 `3*T5` 上级失效后的显式回退 `ST1` 直接触发原因（需与板1侧状态同步复核）
  - 多网关取舍后的方向锁定与释放
