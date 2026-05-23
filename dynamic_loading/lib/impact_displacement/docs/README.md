# IMPACT_DISPLACEMENT 文档索引（v0：忽略旋转验证）

本目录文档用于支撑 `lib/impact_displacement` 在 PC 端与 MCU 端的“事件窗位移估计”开发与验证。

## Current（当前使用）

- `IMPACT_DISPLACEMENT_ALGO_ANALYSIS.md`
  - 读者：固件/算法/联调
  - 说明：给出可落地的算法流程、输入输出契约与风险边界（v0 按“小旋转可忽略”假设验证）。

- `IMPACT_DISPLACEMENT_MATH_PRINCIPLES.md`
  - 读者：算法/评审
  - 说明：解释二重积分、基线去重力、终点速度约束的数学依据，并说明旋转导致误差的原理（作为风险背景）。

- `IMPACT_DISPLACEMENT_PC_VALIDATION.md`
  - 读者：PC 工具/算法/验证
  - 说明：PC 端合成数据集、回归用例与当前基线结果，用于保证离线算法可复现。

- `IMPACT_DISPLACEMENT_RF_EVENT_LINK_SPEC.md`
  - 读者：固件/RF/协议/联调
  - 说明：第二阶段“事件后上传（store-and-forward）”的最小链路规格：远端分片上传、网关重组、PC 转发。

- `IMPACT_DISPLACEMENT_TASK_IMPLEMENTATION.md`
  - 读者：项目执行/协作
  - 说明：单一工作计划与进度基线（已完成项、下一步里程碑、验收标准）。

## v0 验证假设（重要）

v0 阶段用于快速验证“短窗事件位移估计”链路，默认假设：
- 事件以平移为主，旋转角度较小，可近似忽略重力投影变化对结果的影响。
- 旋转相关只作为风险提示与信息性质量指标（如 `rotation_error_mg`、`IMPACT_DISP_QF_ROTATION_HIGH`），不作为当前功能前置条件。

当真实碰撞场景存在明显旋转时：
- `disp_mm` 可能出现显著误差，应结合 `confidence/quality_flags/rotation_error_mg` 解读。
- 若业务需要在强旋转下仍保持位移物理意义，需要引入陀螺仪/姿态估计（后续增强项）。

