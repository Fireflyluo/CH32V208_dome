# CH32V208 Zig/C 混合工程（xmake + TMOS）

本工程用于把 CH32V208（RISC-V）固件按“C 做底层，Zig 做应用层”的方式组织起来：

- 构建系统仍使用 `xmake`
- C 负责启动文件、SDK/外设驱动、底层初始化与 `main()`
- Zig 负责应用层逻辑：编译为静态库 `libzig_app.a`，由 C 工程链接
- 入口从 C 进入，然后移交到 Zig（Zig 内部运行 TMOS 主循环）

工程参考 `Demo_v0.2` 的结构与“编译后脚本（post_build）”风格，但去掉了 `Demo_v0.2` 里的 USB、传感器、OLED、RF 等外部库，仅保留：

- 片上外设驱动（`sdk/Peripheral`）
- TMOS（实现位于 `sdk/LIB/libwchble.a`，这里把它当作 TMOS 底座使用）

## 目录结构

- `xmake.lua`：xmake 构建入口（WCH GCC + Zig 静态库 + 链接 + post_build）
- `APP/main.c`：C 入口，完成底层初始化后调用 `zig_app_main()`
- `APP/system_ch32v20x.c`：WCH 系统时钟/`SystemInit`
- `APP/ch32v20x_it.c`：最小中断（提供 `BB_IRQHandler`，`LLE_IRQHandler` 由汇编提供）
- `bsp/board.c`、`bsp/board.h`：板级初始化（时钟更新、Delay、printf 串口、PC9 LED）
- `zig/app.zig`：Zig 应用层（注册一个 TMOS task，每 500ms 翻转 LED）
- `sdk/*`：从 `Demo_v0.2` 引入的 SDK 子集
  - `sdk/Startup/startup_ch32v20x_D8W.S`
  - `sdk/HAL/Link.ld`（链接脚本，含 RAM/FLASH 配置）
  - `sdk/LIB/libwchble.a` + `sdk/LIB/ble_task_scheduler.S`（TMOS/LLE 相关）
  - `sdk/Peripheral/src/*.c`（片上外设驱动）
- `scripts/post_build.lua`：构建后输出（生成 `.hex/.lst` + 统计内存占用）

## 构建方式

### 1) 准备 Zig（可选）

如果机器上没有 `zig`，可以把 Zig 下载到工程内 `tools/zig/`：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\setup_zig.ps1 -Version 0.15.2
```

### 2) xmake 配置 + 编译

```powershell
xmake f -p cross -a riscv -m debug
xmake
```

如果你的 WCH 工具链不在默认 MRS2 安装目录，可以覆盖：

```powershell
xmake f --wch_toolchain_root="E:/APP/MRS2/MounRiver_Studio2/resources/app/resources/win32/components/WCH/Toolchain"
xmake
```

如果需要指定 Zig 路径：

```powershell
xmake f --zig_exe="D:/path/to/zig.exe"
xmake
```

也可以直接用脚本（内部就是跑 `xmake f` + `xmake`）：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1
```

## 输出产物

默认输出目录：`build/cross/riscv/debug/`

- `firmware.elf`
- `firmware.bin`
- `firmware.hex`
- `firmware.lst`
- `firmware.map`

构建结束会执行 `scripts/post_build.lua`，并在终端输出内存占用统计。

## C -> Zig 交接流程（运行时）

1. `sdk/Startup/startup_ch32v20x_D8W.S` 进入 `handle_reset`
2. 调用 `SystemInit()`（`APP/system_ch32v20x.c`）
3. 跳转到 `main()`（`APP/main.c`）
4. `board_init()` 做基础外设初始化（串口 + LED 等）
5. `WCHBLE_Init()` + `HAL_Init()`：用于初始化 TMOS 相关底座
6. 调用 Zig 导出符号 `zig_app_main()`（`zig/app.zig`）
7. Zig 注册一个 TMOS task，然后在 `while(1) TMOS_SystemProcess()` 中运行

## 链接脚本与 RAM/FLASH 配置

链接脚本：`sdk/HAL/Link.ld`

当前按 CH32V208 常见配置启用：

- `FLASH`：`ORIGIN = 0x00000000`，`LENGTH = 128K`
- `RAM`：`ORIGIN = 0x20000000`，`LENGTH = 64K`

如果你的芯片/工程需要切到 `144K/48K`、`160K/32K` 等组合，请在 `sdk/HAL/Link.ld` 的 `MEMORY { ... }` 中修改对应 `LENGTH`，并确保 post_build 输出的总量与实际一致。

## 说明

- CH32V208 是 **RISC-V**，本工程使用 WCH 的 `riscv*-wch-elf-gcc` 工具链，不是 `arm-none-eabi-gcc`。
- TMOS 实现来自 `sdk/LIB/libwchble.a`；工程不启用 `Demo_v0.2` 中的 USB/传感器/OLED/RF 等外部库。
