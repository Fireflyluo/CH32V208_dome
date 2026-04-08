# CH32V208: 用 CherryUSB 替换 WCH 官方 USBLIB 的收益说明

这份文档面向项目维护与后续迭代，解释为什么在 CH32V208 上优先选择 CherryUSB（以 CDC 虚拟串口为落地目标）。

## 1. 当前项目落地到什么程度

- 设备侧采用 CherryUSB Device + CDC ACM，应用层集中在 `User/usb_cdc_app.c`。
- 构建链路切换为 `xmake`，产物输出在 `build/cross/riscv/debug/CH32V208GBU.elf`（同时生成 `.hex/.lst/.map`）。
- USBFS 低层初始化补齐板级必需项（PA11/PA12 IO 状态、内部上拉、SIE reset），在 `User/Main.c` 的 `usb_dc_low_level_init()`。

## 2. 为什么替换：核心收益

### 2.1 业务代码更稳定、可复用

CherryUSB 的“协议栈/类驱动/芯片端口”是分层的：

- 业务只关心 CDC 的收发回调，不用反复改寄存器细节。
- 换同系列 MCU 或换不同 USB IP 时，优先改端口层，CDC 业务层复用率更高。

### 2.2 功能扩展成本更低

从“单 CDC”走向“复合设备”时（比如 `CDC + HID`、`CDC + MSC`）：

- 通常只需要增加 descriptor 和 class 初始化，主体框架不变。
- 相比 USBLIB 的“按例程拼装”，更不容易出现散落在多个文件、难维护的状态机代码。

### 2.3 调试验证更可控

- 类驱动回调清晰，定位问题更像排查一条数据链路（Setup -> EP0 -> EPx 收发）。
- 便于加自检：例如本工程在 `User/usb_cdc_app.c` 增加了 `cdc_acm_poll()` 计数器，帮助判断是否发生 `RESET/CONFIGURED`。

### 2.4 构建与协作更顺畅

切到 `xmake` 后：

- 命令行可重复构建，减少 IDE 配置漂移带来的问题。
- 编译输出（map/lst）稳定可追溯，便于做体积与符号排查。

## 3. 需要正视的代价与风险

- 初期需要把“板级初始化 + 端口层”打通（一次性投入）。
- 某些 USB 主机/系统对 CDC 兼容性更严格，可能需要完善通知端点（中断 IN）行为、line coding 策略、以及长包/断连恢复策略（可迭代补齐）。

## 4. 适用结论

- 如果项目只做一次性 demo、功能非常固定：USBLIB 也能用。
- 只要存在持续迭代需求（复合设备、协议升级、多人协作、跨板复用）：CherryUSB 的长期收益更明显。

