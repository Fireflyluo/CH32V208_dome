# Ad-Hoc-lib

`Ad-Hoc-lib` 是本工程的自组网协议库。  
默认保留仓库内副本 `lib/Ad-Hoc-lib/` 作为参考/回退路径；当 `xmake` 配置了外部 `adhoc_repo_dir` 时，工程会优先通过本地 `xrepo` 包仓引入协议核心。  
库与底层 RF 驱动解耦，协议逻辑不放在 `AROS-RF-LIB` 内。

## 1. 库定位

- 提供协议层能力：帧模型、时序、组网状态机、数据转发与确认。
- 通过 `adhoc_link_ops_t` 对接任意链路实现。
- 静态内存模型，无动态分配。
- 适合作为“可复用协议内核”被多个工程接入。

## 2. 快速入口

- 对外 API：`include/adhoc_api.h`
- 最小接入：`docs/USAGE.md`
- 协议语义：`docs/protocol-design.md`
- 软件实现：`docs/software-architecture.md`
- 统一配置：`docs/configuration.md`
- 软件设计基线：`docs/软件设计文档.md`
- 平台移植：`docs/porting-guide.md`
- 文档导航：`docs/README.md`
- Windows 仿真：`test/README.md`

## 3. 目录结构

- `include/`：公开头文件
- `src/`：协议实现
- `docs/`：协议、实现、移植、使用文档
- `port/ch32v208/`：CH32V208 平台参考实现
- `test/`：Windows 多线程仿真与回归场景

## 4. 当前工程参考

- 协议任务接入示例：`app/tasks/ad_hoc_task.c`
- 链路适配示例：`app/adapters/adhoc_link_aros.c`
- CH32 平台端口：`port/ch32v208/`

## 5. 构建（在本仓库）

```bash
xmake
```
协议本身（Ad-Hoc）：约 ROM 13.3KB、RAM 14B。