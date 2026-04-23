# IMPACT_DISPLACEMENT PC 阶段验证说明

## 1. 目标

在 MCU 落地前，先在 Windows 上完成算法正确性与鲁棒性验证，确保：
- 理想碰撞输入下，位移积分结果正确。
- 非理想输入（噪声、交叉轴耦合、轻微姿态变化）下，算法输出稳定且可解释。

## 2. 数据集

路径：`lib/impact_displacement/test/data/`

- `ideal_collision_event.csv`
  - 400Hz，160 点。
  - `ax` 为对称正负脉冲，`ay=0`，`az=1000mg`。
  - 用于验证基础积分链路和零速度约束修正。

- `nonideal_collision_event.csv`
  - 400Hz，220 点。
  - 包含静态偏置、确定性噪声、交叉轴耦合与小幅重力投影变化。
  - 用于验证工程场景下的稳健性和质量评分行为。

- `generate_synthetic_events.py`
  - 生成上述两份 CSV，保证样本可重复。

## 3. 运行方式

1. 生成数据（可选）

```powershell
python lib/impact_displacement/test/data/generate_synthetic_events.py
```

2. 单场景 smoke（默认 ideal）

```powershell
powershell -ExecutionPolicy Bypass -File scripts/run_impact_displacement_windows_smoke.ps1
```

3. 指定 CSV 运行 smoke（示例：nonideal）

```powershell
build/impact_displacement_windows_smoke.exe lib/impact_displacement/test/data/nonideal_collision_event.csv 60
```

4. 双场景回归

```powershell
powershell -ExecutionPolicy Bypass -File scripts/run_impact_displacement_windows_regression.ps1
```

## 4. 当前基线结果（2026-04-20）
- ideal:
  - `dx=10.003mm`, `dy=0.000mm`, `dz=0.000mm`, `disp=10.003mm`
  - `confidence=100`, `flags=0x00000000`

- nonideal:
  - `dx=12.871mm`, `dy=1.541mm`, `dz=8.084mm`, `disp=15.277mm`
  - `confidence=100`, `flags=0x00000000`

说明：
- nonideal 的 `dz` 不为 0 是预期行为，来源是样本中注入的轻微重力投影变化。
- 两组结果可作为后续调参、重构、移植后的对照基线。

## 5. MCU 移植验证结果（2026-04-21）

### 5.1 板端集成状态
- 采样链路：TMOS 周期任务读出加速度（当前任务周期 `3ms`），SC7A20 ODR 配置 `400Hz`。
- 算法集成：板端已完成事件触发、事件窗提取、位移计算与日志输出。
- 串口验证：可稳定触发并输出 `disp_mm/confidence/quality_flags`。

### 5.2 本轮稳定性修正
- 时间戳：改为定时器微秒时间 `drv_tim_get_time_us()`，避免原先按“每秒统计窗口计数”生成时间戳导致的回绕问题。
- 环形窗：修复了环形缓冲区跨界时直接传递连续指针的问题，改为按逻辑顺序流式喂样本。
- 状态机：修复了事件处理阶段 `memset` 上下文导致 `cfg` 丢失的问题，改为初始化一次、每事件 `reset + begin/feed/end`。

### 5.3 典型现象
- 在桌面水平平移约 50cm 的样例中，关闭 EMA 时 `disp_mm` 会被小倾角导致的重力泄漏显著高估（约 1100mm 量级）。
- 开启 EMA（`tau=500ms`）后，`disp_mm` 回到约 514mm 量级，更接近预期位移。

### 5.4 I2C 竞争缓解观测（2026-04-21）
- 已应用显示侧缓解策略：OLED 改为“仅新帧触发刷新”，渲染频率降至 1Hz。
- 串口观测新增 `uplink i2c ...` 统计后，连续观测到：
  - `q=0/1`（队列深度低）
  - `to=0 rec=0`（无超时、无恢复）
  - `aw=0ms ae=1ms`（平均等待/执行较低）
- 结论：I2C 总线竞争已明显缓解，`accel_hz` 仍有波动主要与 TMOS 调度和 10ms 高频串口日志负载相关，而非 I2C 超时竞争。

### 5.5 板端算法回归实测（2026-04-21，COM8）
- 过程：按 `xmake -> openocd program verify -> 串口35s` 执行，抓取日志 `build/serial_impact_validation_20260421_1749.log`。
- 结果：
  - 成功出现多次 `IMPACT triggered`，并出现 5 条 `IMPACT result`（6 次触发中 5 次完成结果输出）。
  - 典型输出：
    - `IMPACT result: event_id=1 ... conf=75% flags=0x00000008`
    - `IMPACT result: event_id=2 ... conf=65% flags=0x0000000A`
    - `IMPACT result: event_id=3 ... conf=100% flags=0x00000000`
  - 质量标志可解释：
    - `0x00000008` -> `IMPACT_DISP_QF_NO_RELEASE`
    - `0x0000000A` -> `IMPACT_DISP_QF_NO_RELEASE | IMPACT_DISP_QF_DT_GAP`
- 结论：
  - 事件检测、切窗、积分与评分主链路已在 MCU 端跑通。
  - 当时串口 `printf` 浮点格式未生效，`disp/dx/dy/dz` 显示为空（`disp=mm dx= dy= dz=`）；后续已修复为定点输出。

### 5.6 日志格式修复后复测（2026-04-21，COM8）
- 修复内容：`IMPACT result` 由浮点 `%f` 改为“毫米*10”定点打印，避免依赖 C 库浮点 `printf` 支持。
- 验证日志：`build/serial_impact_validation_20260421_1754_fixfloat.log`
- 本轮实测样例：
  - `event_id=1 disp=33.0mm dx=-8.6 dy=3.8 dz=31.6 conf=65% flags=0x0000000A`
  - `event_id=2 disp=19.5mm dx=12.0 dy=15.2 dz=-2.0 conf=65% flags=0x0000000A`
  - `event_id=3 disp=16.7mm dx=-7.7 dy=8.7 dz=-11.9 conf=65% flags=0x0000000A`
  - `event_id=4 disp=27.0mm dx=-9.8 dy=20.2 dz=14.9 conf=65% flags=0x0000000A`
- 结论：板端算法输出数值可读，后续可继续做“已知位移工况”标定与参数收敛。

### 5.7 真实工况：约 50cm 平移（2026-04-21，COM8）
- 验证日志：`build/serial_impact_validation_20260421_50cm_run1.log`
- 关键结果：
  - `IMPACT result: event_id=4 disp=352.0mm dx=119.4 dy=-294.0 dz=-152.4 conf=65% flags=0x0000000A`
- 解读：
  - 本次“约 50cm 平移”已成功触发板端事件并输出位移量级结果（约 35.2cm）。
  - `flags=0x0000000A`（`NO_RELEASE | DT_GAP`）说明事件结束判据与采样节拍仍有改进空间，当前结果可用于趋势验证，不建议作为最终标定值。

### 5.8 动态重力估计开启后复测（2026-04-21，COM8）
- 配置确认：板端 `gravity_ema_tau_ms = 500ms`（动态重力估计开启）。
- 验证日志：`build/serial_impact_validation_20260421_50cm_run3_ema500.log`
- 事件结果（快速起停动作）：
  - `event_id=1 disp=303.0mm dx=-61.0 dy=-296.4 dz=-16.2 conf=65% flags=0x0000000A`
  - `event_id=2 disp=284.6mm dx=-100.3 dy=113.2 dz=241.0 conf=65% flags=0x0000000A`
  - `event_id=3 disp=48.6mm ... conf=65% flags=0x0000000A`
  - `event_id=4 disp=9.3mm ... conf=65% flags=0x0000000A`
- 解读：
  - 在“约 50cm 平移”工况下，主事件位移量级约 `0.28m ~ 0.30m`，相比目标仍偏低。
  - 多事件并发说明当前触发/释放门限对“平移+手持扰动”较敏感，需在下一步通过门限与事件筛选策略收敛。

### 5.9 参数联调复测（2026-04-21，COM4）
- 调参内容（实验态）：
  - `gravity_ema_tau_ms: 500 -> 1200`
  - `max_dt_ms: 默认 -> 80`
  - 串口上传节拍：`SERIAL_UPLOAD_MS: 10 -> 30`
- 验证日志：`build/serial_impact_validation_20260421_50cm_run4_tuned.log`
- 事件结果：
  - `event_id=1 disp=591.5mm dx=77.2 dy=-568.0 dz=-145.9 conf=65% flags=0x0000000A`
  - `event_id=2 disp=17.4mm dx=11.9 dy=12.1 dz=3.9 conf=75% flags=0x00000008`
- 观测：
  - 本轮离散度较大（同一轮内出现 `591.5mm` 与 `17.4mm` 两个量级）。
  - 串口统计显示 I2C 仲裁错误仍存在，且本次窗口内 `err/to/rec` 增量高于 run3。
- 结论（阶段性）：
  - 仅凭本轮样本不能证明 `tau=1200ms` 比 `tau=500ms` 更优。
  - 继续使用 `tau=500ms` 作为当前对比基线更稳妥，后续在“动作模板固定”的条件下再做参数收敛（每组参数至少 5 次同工况样本）。

### 5.10 释放条件调整复测（2026-04-21，COM8）
- 调参内容：
  - `release_count_min: 8 -> 24`（延长释放判定）
  - 触发阈值保持 `trigger_thr=800mg`
- 验证日志：`build/serial_impact_validation_20260421_181956_fb50cm_rel24.log`
- 结果：
  - 仅触发 1 个事件：`disp=9.3mm`
- 解读：
  - 事件碎片化有所收敛，但由于触发阈值过高，正常平移工况难以触发主事件。
  - 同日志统计显示 `|norm-1000|` 最大约 `370mg`，低于 `800mg` 触发阈值，证明触发门限与工况不匹配。

### 5.11 触发阈值下调复测（2026-04-21，COM8）
- 调参内容：
  - `trigger_thr: 800mg -> 180mg`
  - `release_count_min=24` 保持不变
- 验证日志：`build/serial_impact_validation_20260421_182217_fb50cm_rel24_trig180.log`
- 事件结果：
  - 主量级事件：
    - `event_id=1 disp=548.3mm ... flags=0x0000000A`
    - `event_id=4 disp=332.1mm ... flags=0x0000000A`
  - 其余小事件：
    - `event_id=2/3/5/6/7/8`，`disp` 约 `5.6mm ~ 37.7mm`
- 观测：
  - 能稳定触发到接近目标量级的主事件（约 0.33m~0.55m）。
  - 仍存在尾部碎片事件，说明仅调阈值还不够，需要增加事件级“去碎片化”策略。
- 结论（阶段性）：
  - `trigger_thr=180mg` 明显优于 `800mg` 的“漏检”状态。
  - 下一步应在维持该阈值的前提下，引入“冷却时间/最小峰值/最小位移”之一来抑制碎片事件。

### 5.12 冷却时间抑制连发复测（2026-04-21，COM8）
- 调参内容：
  - 保持 `trigger_thr=180mg`、`release_count_min=24`
  - 新增事件冷却：`cooldown=800ms`（事件结束后禁止再次触发）
- 验证日志：`build/serial_impact_validation_20260421_182930_fb50cm_cooldown800.log`
- 事件结果：
  - `event_id=1 disp=1.3mm ...`
  - `event_id=2 disp=187.1mm ...`
  - `event_id=3 disp=0.6mm ...`
  - `event_id=4 disp=377.3mm ...`
- 对比（上一轮 `trigger=180/release=24`）：
  - 事件数：`8 -> 4`
  - 主事件（百毫米量级）仍可保留，碎片事件明显减少。
- 结论（阶段性）：
  - 冷却策略有效，已完成第一步“去碎片化”。
  - 仍建议叠加“最小位移或最小峰值筛选”进一步清理 `1~10mm` 级小事件。

## 6. 真实数据样例（2026-04-20，串口直出 uplink）

说明：当前阶段为了快速验证算法，允许不走协议，直接在串口输出三轴加速度，由 PC 采集为 CSV，并离线计算位移。

采集脚本：

```powershell
python scripts/capture_uplink_accel_to_csv.py --port COM8 --out build/uplink_accel_capture_com8_xxx.csv
```

样例文件（本仓库工作区 `build/` 下，属于验证产物）：
- `build/uplink_accel_event_window_com8_translate50cm_uart_run1_peakpair.csv`

对比运行（100Hz，前窗 30 点）：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/run_impact_displacement_windows_cli.ps1 `
  -Csv build/uplink_accel_event_window_com8_translate50cm_uart_run1_peakpair.csv `
  -Rate 100 -Pre 30 -ReleaseThr 80 -ReleaseCount 8 -MaxDtMs 200 -GravEmaMs 0

powershell -ExecutionPolicy Bypass -File scripts/run_impact_displacement_windows_cli.ps1 `
  -Csv build/uplink_accel_event_window_com8_translate50cm_uart_run1_peakpair.csv `
  -Rate 100 -Pre 30 -ReleaseThr 80 -ReleaseCount 8 -MaxDtMs 200 -GravEmaMs 500
```

现象记录：
- 在该“桌面水平平移约 50cm”的样例中，关闭 EMA 时 `disp_mm` 会被小倾角导致的重力泄漏显著高估（约 1100mm 量级）。
- 开启 EMA（`tau=500ms`）后，`disp_mm` 回到约 514mm 量级，更接近预期位移。

