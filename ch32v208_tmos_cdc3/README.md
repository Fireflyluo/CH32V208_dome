# CH32V208 TMOS CDC 工程说明

## 1. 项目简介
本工程基于 **CH32V208**，使用 **TMOS** 进行任务调度，当前已实现：
- SC7A20 三轴加速度采集
- SHT40 温湿度采集（两段式）
- OLED 显示任务
- USB CDC 串口上报任务
- I2C 总线仲裁与 IT/DMA 混合传输

（内部中文注释多数使用ai添加，可能存在错误）

主入口位于 [app/main.c](app/main.c)，初始化顺序如下：
1. `board_init()`
2. `WCHBLE_Init()`
3. `HAL_Init()`
4. `sensor_task_init()`
5. `display_task_init()`
6. `serial_upload_task_init()`
7. `while(1) { TMOS_SystemProcess(); }`

## 2. 当前任务与频率
### 2.1 传感器任务（sensor_task）
文件：`app/tasks/sensor_task.c`
- SC7A20：**10 Hz** 采样（100 ms）
- SHT40：**1 Hz** 采样（1000 ms）
- SHT40 采用两段式事件：
  - 第一步发送测量命令
  - 延时后第二步读取结果
- 使用快照结构共享数据，供显示任务与串口任务读取

### 2.2 显示任务（display_task）
文件：`app/tasks/display_task.c`
- OLED 刷新周期：**500 ms（2 Hz）**
- 显示内容：
  - AX/AY/AZ（mg）
  - 温度（C）
  - 湿度（%）

### 2.3 串口上报任务（serial_upload_task）
文件：`app/tasks/serial_upload_task.c`
- 上报周期：**1 s**
- 每秒打印最近一次：
  - 加速度数据
  - 温湿度数据
  - 统计信息（accel_hz/sht_hz/sht_ok/sht_err/ready）

## 3. I2C 与 OLED 说明
### 3.1 I2C 架构
- 底层驱动：`bsp/drivers/src/drv_i2c.c`
- 仲裁层：`bsp/bus/src/i2c_bus_arbiter.c`
- OLED、SC7A20、SHT40 都通过仲裁层提交 I2C 请求

### 3.2 OLED 寻址模式（可选）
文件：`lib/oled/OLED.h`
- `OLED_ADDR_MODE_HORIZONTAL` = `0`
- `OLED_ADDR_MODE_PAGE` = `2`
- 当前默认：
  - `#define OLED_ADDR_MODE OLED_ADDR_MODE_PAGE`

### 3.3 OLED 刷新稳定性处理
文件：`lib/oled/OLED.c`
- 局部刷新已支持两种寻址模式
- 刷新链路增加了失败检查：命令/数据发送失败会中止当前轮，避免页错位扩散

## 4. 构建与运行
## 4.1 构建
```bash
xmake -r
```

## 4.2 构建选项
- 关闭运行日志：
```bash
xmake f --log_print=false
xmake -r
```
- 打开运行日志：
```bash
xmake f --log_print=true
xmake -r
```

## 4.3 串口查看（USB CDC）
示例：
```bash
python scripts/serial_reader.py --port COM8 --baud 115200 --encoding utf-8
```
单次读取：
```bash
python scripts/serial_reader.py --port COM8 --baud 115200 --encoding utf-8 --once
```

## 5. 目录概览（当前）
- `app/`：应用入口与任务
- `bsp/`：板级初始化、驱动、I2C 仲裁、USB CDC
- `lib/oled/`：OLED 驱动与字库
- `lib/sc7a20/`：加速度计驱动
- `lib/sht40/`：温湿度驱动
- `sdk/`：CH32 SDK/HAL/外设库
- `utils/`：公共工具
- `scripts/`：构建与串口辅助脚本

## 6. 注意事项
- 本工程文本文件统一建议使用 **UTF-8**。
- 在使用终端工具或其他自动化工具前，必须先确保输入与输出均为 **UTF-8** 编码（包括命令输出、日志、脚本读写文件）。
- PowerShell 推荐先执行：`chcp 65001`、`[Console]::InputEncoding = [System.Text.UTF8Encoding]::new($false)`、`[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)`。
- 若终端显示乱码，请确认终端编码与文件编码一致。
- OLED 寻址模式切换后，建议重新上电并观察一段时间，确认显示稳定。

## 7. 维护建议
如需继续扩展，建议优先保持以下约定：
1. 新增 I2C 设备统一走仲裁层。
2. 任务间数据共享统一走快照接口。
3. 日志输出统一走 `LOG_PRINT` 宏，便于整体开关。
