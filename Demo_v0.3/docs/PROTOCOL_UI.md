# 协议与 OLED UI 速查

## 1. OLED UI 行定义（6x8 小字）
当前显示任务文件：`app/tasks/display_task.c`

1. `TG/RF/A/S`
- `TG`：当前设备 `tg_id`
- `RF`：RF 同步标志（0/1）
- `A/S`：`AHz/SHz` 采样统计频率

2. `RFTX/RFRX`
- RF 发包/收包累计计数

3. `A:x,y,z`
- 三轴加速度（单位：mg）
4. `T/H`
- `T`：温度（摄氏度）
- `H`：湿度（%）

5. `TX/SQ/Q`
- `TX`：最近发送包类型（`q/n/m/i/h`）
- `SQ`：最近发送包序号（0~63）
- `Q`：参数确认包累计计数

6. `UP`
- 上传状态与样本分片进度
- `O`：超阈值标志（0/1）

7. `TH/A`
- `TH`：阈值区间（低阈值-高阈值）
- `A`：当前加速度12bit值

8. `N/M/I/H`
- 协议上报包累计计数

### 1.1 UI 字符串模板（逐行）
```text
1. TG:%u RF:%u A:%u S:%u
2. RFTX:%lu RFRX:%lu
3. A:%ld,%ld,%ld
4. T:%ld.%02ld H:%ld%%
5. TX:%c SQ:%u Q:%u
6. UP:%u %u/%u O:%u
7. TH:%u-%u A:%u
8. N:%u M:%u I:%u H:%u
```

### 1.2 UI 显示示例（逐行）
```text
1. TG:1 RF:1 A:10 S:1
2. RFTX:209 RFRX:172
3. A:947,7,240
4. T:33.12 H:23%
5. TX:h SQ:12 Q:34
6. UP:1 8/64 O:0
7. TH:500-1500 A:923
8. N:20 M:5 I:12 H:18
```

---

## 2. 协议消息行为
当前协议任务文件：`app/tasks/protocol_task.c`

- 主机下发：
  - `P`：设置参数
  - `S`：查询
  - `E`：重复查询（请求重发）

- 设备回包：
  - `q`：参数确认
  - `n`：当前无数据
  - `m`：数据尚在准备中
  - `i`：长消息中间片
  - `h`：长消息最后片

- `E` 语义：
  - 重发最近一次上报帧（`n/m/i/h`）
  - 消息号保持不变

---

## 3. 参数生效范围（当前实现）
- `T2`：映射为协议样本采集间隔（带范围保护）
- `H/L`：映射为加速度12bit阈值
- 参数更新后会清空上传缓存，避免旧参数数据混入新会话

---

## 4. 串口联调建议
1. 上电后先发 `P`，确认收到 `q`
2. 连续发 `S`：预期看到 `n/m/i/h` 中的一种或多种
3. 收到 `i/h/n/m` 后立刻发 `E`：预期重发同一帧

---

## 5. 主机自动联调脚本
脚本：`scripts/protocol_host_tester.py`

用途：
- 发送 `P` 参数帧并等待 `q`
- 连续发送 `S` 查询并打印 `n/m/i/h`
- 发送 `E` 验证重发行为

示例：
```bash
python scripts/protocol_host_tester.py --port COM4 --send-params --query-count 6 --send-repeat
```

常用参数：
- `--port`：串口号，默认 `COM4`
- `--baud`：波特率，默认 `115200`
- `--timeout`：单帧接收超时（秒）
- `--query-count`：`S` 查询次数
- `--query-interval`：`S` 帧之间的间隔（秒）
