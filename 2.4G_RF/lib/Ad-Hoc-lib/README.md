# Ad-Hoc-lib

`Ad-Hoc-lib` 是当前工程的新自组网协议库，实现位置固定在 `lib/Ad-Hoc-lib/`，与原 `AROS-RF-LIB` 保持解耦。

## 1. 定位

- 负责协议层：帧模型、CRC、时序、组网状态机、数据面与应用侧节点 API。
- 依赖抽象链路接口 `adhoc_link_ops_t`，不直接实现具体射频驱动。
- 面向静态内存环境，不使用动态分配。
- 当前默认由 `app/tasks/ad_hoc_task.c` 驱动，已成为工程主路径。

## 2. 当前模块

- `adhoc_api.h` + `adhoc_node.c`：节点生命周期、角色切换、收发入口、数据提交与发送结果回传。
- `adhoc_frame.h/.c`：固定 `32B` 协议帧、`A/D` 头字段、发送者/载荷 ID 编解码。
- `adhoc_crc8.h/.c`：固定参数 `CRC8` 校验。
- `adhoc_timing.h/.c`：`T5/T6/n/m` 关系校验、时隙奇偶规则。
- `adhoc_sm.h/.c`：`ST1/U1/UN/C1/CN` 组网状态机、网关 `A V=0` 发射与 `T2` 收集窗口。
- `adhoc_reply_list.h/.c`：组网确认列表，单帧最多 `6` 条，支持补 `0`。
- `adhoc_data_plane.h/.c`：`D` 帧、`ID+No` 去重、网关 `4` 条 ACK、信标转发队列与 TX report。
- `port/ch32v208/adhoc_port_ch32.h/.c`：CH32V208 端口封装，提供时间、TMOS `rand/srand` 与临界区钩子。

## 3. 最小接入流程

1. 准备配置 `adhoc_cfg_t`，填入 `domain_id/node_id/gateway_no/T1~T4/retry_max`。
2. 准备链路适配 `adhoc_link_ops_t`，至少实现 `init/start_rx/tx/poll_rx`。
3. 通过 `adhoc_node_required_size()` 获取节点所需静态内存大小。
4. 调用 `adhoc_node_init()` 初始化节点。
5. 通过 `adhoc_node_set_role()` 切换为 `ADHOC_ROLE_BEACON` 或 `ADHOC_ROLE_GATEWAY`。
6. 在主循环或任务周期内执行：
   - 收包后调用 `adhoc_node_on_rx()`
   - 周期调用 `adhoc_node_poll(now_us)`
   - 若 `adhoc_node_fetch_tx()` 成功，则交由链路层发送
7. 信标上报数据时调用 `adhoc_node_submit_data()`，再用 `adhoc_node_fetch_data_tx_report()` 拉取结果。

更完整的接入说明见 `lib/Ad-Hoc-lib/docs/USAGE.md`。

## 4. 当前实现边界

以下条目在文档中已明确为“规范要求”，但当前库内尚未完全实现：

- 信标侧已具备“首次观测方向 -> 本地方向结束时刻 -> 到期锁定停发”的轻量闭环，但“全网统一绝对结束时刻”的显式传播仍未完全收口。
- 组网完成后的重组网全局对齐策略仍未完全收口；当前仅提供本地计时接口。
- `docs/t07任务约束` 提出的“`A` 帧前 4B 注入 Epoch”方案已评估为与当前 `6` 条确认布局冲突，暂不直接并入主线。
- `source_id_flag` 已与 `upstream_no` 绑定表和轻量邻居表联动，但“最大上级编号 / 可再转发上级编号”的完整多编号语义尚未最终收口。
- 多网关锁定释放策略的最终闭环仍待补齐。

当前版本已经支持“监听确认”最小行为：节点监听到同一 `source.node_id + No` 的上级转发/再转发后，会停止本条继续重发。
当前版本的 `D` 帧转发路由上下文已由组网状态机提供，数据面不再自行硬编码上级编号或直接沿用收到报文的转发级别。
当前版本已加入上级编号绑定表（`0..5`）与轻量邻居表 `Top-K Cache`（`K=6`）用于分配并筛选 `upstream_no`。
当前版本组网确认采用“定向 `Flag(0..5)` 回填”：`Flag=6` 请求仅忽略不入确认队列，未确认节点仅在命中自身且 `Flag=N`（与本次请求一致）时转入确认态。
当前版本已实现网关侧 `30s` 组网时间窗控制；时间窗到期后停止 `A V=0` 发射并关闭 `T2` 收集窗口。
当前版本已实现信标侧“方向结束时刻继承”：在 `ST1` 首次观测某个上行方向时记录本地 `network_end_us`，确认后沿该结束时刻继续广播，到期进入锁定停发。
当前版本已实现轻量“上级失效释放”：若在组网窗口未结束前连续 `3*T5` 未再收到当前上级 `A` 帧，则显式回退 `ST1` 并释放方向锁定。
当前版本已提供 `adhoc_node_get_runtime_status()` 运行态快照接口；当前工程会把 `gw_left/gw_lock/up_age/net_left` 同步到 `ad_hoc_task` 并经串口 `adhoc sm` 输出，便于板测。
截至 `2026-04-26`，已在“单板 + 网关角色”条件下验证到 `gw_left -> 0`、`gw_lock -> 1` 与发包计数停增；信标/多网关相关项仍待后续多板验证。
截至 `2026-04-26`，已在“双板（网关 + 信标）”条件下验证到板2执行 `ST1 -> C1 -> ST1` 状态跃迁，并在 `C1` 期间看到数据 ACK 增长；这证明最小入网与数据 ACK 主路径已跑通。

当前版本适合作为最小联调基线，不应把以上条目误判为已完成。

## 5. 相关文档

- 协议设计基线：`docs/设计文档（初稿）.md`
- 任务推进与完成度：`docs/任务分解技术书.md`
- 验证入口：`docs/验证与回归清单.md`
- CH32V208 端口说明：`lib/Ad-Hoc-lib/port/ch32v208/README.md`

## 6. 构建

本库已接入工程默认构建路径：

```bash
xmake
```
