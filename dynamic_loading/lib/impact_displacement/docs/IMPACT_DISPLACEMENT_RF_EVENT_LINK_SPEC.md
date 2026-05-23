# 碰撞事件 RF 回传最小规格（Windows 验证阶段）

## 1. 目标

本规格用于第二阶段验证：不要求实时计算，只要求把真实场景下的碰撞事件加速度样本稳定回传到 PC。

核心目标：
- 远端板完整采集事件窗（前窗+后窗）。
- 事件结束后分片上传到网关板。
- 网关板重组成功后转发到 PC。
- PC 离线算法读取同一事件原始样本进行计算。

## 2. 约束条件（基于当前工程）

- RF 业务数据区长度：`TG_MSG_DAT_N = 24B`
- AROS-RF 消息长度：`ARF_MsgN = 32B`（其中业务数据在 `MSG_DAT` 区）
- 现有 `rf_task` 已通过 `tg_set_newdatfunc()` 注入 24B 业务数据
- 第二阶段允许“事件后上传”，不要求实时流

因此本规格采用“短头 + 小块分片 + 重复发送”方案，不引入复杂双向 ACK。

## 3. 端到端链路

数据流：
1. 远端采集板：持续采样 -> 触发碰撞 -> 冻结事件窗缓存。
2. 远端采集板：事件结束后发送 `BEGIN -> DATA* -> END`。
3. 网关板：按 `node_id + event_seq` 重组，完成后转发给 PC。
4. PC：解析并保存事件样本，调用算法库离线计算。

可靠性策略：
- 不做实时 ACK，采用“整事件重复发送 `N_repeat` 次”提高到达率。
- 网关按事件号去重并按分片位图补全。

## 4. 事件数据模型

## 4.1 样本定义（用于 RF DATA 分片）

每个样本固定 6 字节：
- `ax_mg`：int16，单位 mg
- `ay_mg`：int16，单位 mg
- `az_mg`：int16，单位 mg

说明：
- 第二阶段先不传每点时间戳，默认固定采样率（由 BEGIN 帧给出）。
- 若后续需要抖动补偿，可在升级版本引入 `dt` 或时间戳差分。

## 4.2 事件元信息

- `event_id`：u32，事件唯一标识（本节点单调递增）
- `event_seq`：u8，短序号（随事件递增，允许回绕）
- `sample_rate_hz`：u16
- `total_samples`：u16
- `pre_samples`：u8
- `post_samples`：u8
- `chunk_count`：u8
- `event_crc32`：u32（对全部样本字节序列计算）

## 5. RF 业务载荷格式（24B）

通用字段（所有帧）：
- `B0`：`proto_magic`，固定 `0xD1`
- `B1`：`frame_type`
- `B2`：`event_seq`
- `B3`：`node_id`（建议用 `tg_id`）

帧类型定义：
- `0x01`：`BEGIN`
- `0x02`：`DATA`
- `0x03`：`END`

## 5.1 BEGIN 帧（24B）

字节布局：
- `B0..B3`：通用字段
- `B4..B7`：`event_id`（u32，小端）
- `B8..B9`：`sample_rate_hz`（u16）
- `B10..B11`：`total_samples`（u16）
- `B12`：`pre_samples`（u8）
- `B13`：`post_samples`（u8）
- `B14`：`chunk_count`（u8）
- `B15`：`sample_format`（u8，当前固定 `0x01=int16_xyz_mg`）
- `B16..B19`：`event_crc32`（u32）
- `B20`：`repeat_total`（u8，整事件重复次数）
- `B21`：`repeat_index`（u8，当前是第几轮）
- `B22..B23`：保留

## 5.2 DATA 帧（24B）

字节布局：
- `B0..B3`：通用字段
- `B4`：`chunk_idx`（u8，0 开始）
- `B5`：`samples_in_chunk`（u8，1..3）
- `B6`：`flags`（u8，bit0=含 clipped 样本）
- `B7`：保留
- `B8..B23`：样本数据区（最多 3 个样本）

样本数据区编码：
- 每个样本 6B：`ax_mg int16` + `ay_mg int16` + `az_mg int16`（小端）
- 若 `samples_in_chunk < 3`，剩余字节填 0

## 5.3 END 帧（24B）

字节布局：
- `B0..B3`：通用字段
- `B4`：`status`（u8，0=normal）
- `B5`：`sent_chunk_count`（u8，本轮发送的数据分片数）
- `B6`：`repeat_total`（u8）
- `B7`：`repeat_index`（u8）
- `B8..B11`：`event_id`（u32）
- `B12..B15`：`event_crc32`（u32）
- `B16..B23`：保留

## 6. 远端发送状态机（建议）

状态：
1. `IDLE`：等待触发
2. `CAPTURE`：记录事件窗
3. `READY`：完成封包准备（计算 CRC、chunk_count）
4. `TX_BEGIN`
5. `TX_DATA`（`chunk_idx = 0..chunk_count-1`）
6. `TX_END`
7. `TX_REPEAT_NEXT`（未达到 `repeat_total` 则回到 `TX_BEGIN`）
8. `DONE`：释放事件缓存，回 `IDLE`

关键参数建议：
- `repeat_total = 2~3`
- 两帧最小发送间隔由现有 `tg_step` 调度决定，不额外阻塞

## 7. 网关重组策略（建议）

键值：
- `session_key = node_id + event_seq`

重组上下文：
- `event_id`
- `total_samples`
- `chunk_count`
- `sample_rate_hz`
- `event_crc32`
- `bitmap[chunk_count]`
- `sample_buffer[total_samples]`
- `deadline_ms`

处理规则：
1. 收到 BEGIN：创建/刷新上下文。
2. 收到 DATA：
- 若上下文存在且 `chunk_idx < chunk_count`，写入对应位置。
- 已接收分片直接丢弃（去重）。
3. 收到 END：不立即判定成功，仅作为“本轮结束”信号。
4. 当位图满：
- 计算样本 CRC32 与 `event_crc32` 比较。
- 成功则标记事件完成并转发 PC。
- 失败则等待下一轮重复数据，直到超时。

超时建议：
- `deadline_ms = max(2000, 2 * event_duration_ms + 1000)`

## 8. 网关到 PC 转发格式（建议）

为减少协议转换复杂度，网关到 PC 推荐“镜像转发”：
- 保留 `BEGIN/DATA/END` 三类记录语义
- 使用 USB CDC 按行输出十六进制或二进制包

Windows 验证阶段建议优先用“文本十六进制行”便于调试：
- `BEGIN,<hex24>`
- `DATA,<hex24>`
- `END,<hex24>`

PC 工具负责：
- 按 `node_id + event_seq` 重组
- 校验 CRC
- 导出 `csv/bin`
- 调用算法库离线计算

## 9. 数据规模与耗时估算（验证阶段）

以 `fs=400Hz`、事件窗 `1s` 为例：
- `total_samples = 400`
- 每帧最多 3 样本 -> `chunk_count = ceil(400/3)=134`
- 单轮总帧数约 `136`（BEGIN+DATA+END）

这对“事件后上传”是可接受的。  
即使重复发送 2 轮，总量约 `272` 帧，仍可用于离线验证。

## 10. 验收标准（第二阶段）

- PC 能拿到完整事件样本并通过 CRC 校验。
- 事件丢包情况下，重复轮次可提升完整率。
- 算法离线可稳定输出位移与质量指标。
- 同一事件重复上传不会被网关重复入库（去重生效）。

## 11. 版本管理

协议版本：
- `proto_magic = 0xD1`
- `sample_format = 0x01`

建议将版本写入任务日志与算法结果文件头，便于联调追踪。

