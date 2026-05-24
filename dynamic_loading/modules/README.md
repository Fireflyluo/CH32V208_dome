# modules 使用说明

## 1. 这套机制是干什么的

这套 `modules/` 机制的目标不是做通用动态链接器，而是做一套固定地址的 RAM 装载模块系统。

它解决的是下面这类需求：

- 把某段关键算法或协议子模块存放在 `FLASH1`
- 运行时再复制到 `RAM_MODULE` 执行
- 宿主程序只通过一层稳定 ABI 和模块交互
- 为后续做加密、完整性校验、按需切换模块打基础

当前工程里已经跑通的两类模块是：

- LED demo 模块：证明“从 `FLASH1` 搬到 `RAM` 并执行”可行
- `impact_displacement` 模块：证明“真实算法库”也可以按这种方式装载运行

## 2. 它更适合什么，不适合什么

更适合：

- 纯算法模块
- 轻量状态机模块
- 帧编解码、滤波、判定逻辑
- 对底层外设依赖较少，只需要少量宿主回调的代码

不适合直接搬进去的类型：

- 强依赖中断向量的模块
- 强依赖 TMOS 任务生命周期的模块
- 直接操作大量寄存器的模块
- 到处访问宿主全局变量的模块
- 需要多个大模块同时常驻 RAM 的场景

一句话判断：

- 如果代码天然像“函数库”或“算法库”，适合
- 如果代码天然像“一个子系统”或“半个固件”，不适合直接搬

## 3. 当前目录结构

- `modules/include/module_abi.h`
  模块 ABI 定义，宿主和模块都依赖它
- `modules/include/module_image.h`
  模块镜像头定义，供构建脚本和 loader 共用
- `modules/include/led_module_programs.h`
  LED demo 的模块描述表头文件
- `modules/include/impact_module_programs.h`
  `impact_displacement` 的模块描述表头文件
- `modules/include/impact_module_api.h`
  `impact_displacement` 模块的命令式 API
- `app/include/module_manager.h`
  单槽位模块管理器接口
- `app/module_manager.c`
  单槽位模块管理器实现
- `modules/led_*`
  三个 LED 示例模块
- `modules/impact_displacement/`
  `impact_displacement` 装载模块实现
- `modules/ram_demo/module_link.ld`
  模块固定地址链接脚本
- `scripts/build_modules.py`
  模块构建与镜像生成脚本

## 4. 当前整体流程

当前工作流如下：

1. 在 `modules/` 下写模块源码
2. 模块通过 `module_abi.h` 暴露导出表
3. `xmake -r` 前会自动运行 `scripts/build_modules.py`
4. 脚本把模块编译成固定地址 ELF
5. 再 `objcopy` 成纯二进制 payload
6. 再加上镜像头和 CRC32
7. 再生成 `app/*_module_programs.c`
8. 主固件把这些镜像链接到 `FLASH1`
9. 运行时 loader 校验镜像
10. 校验通过后复制到 `RAM_MODULE`
11. 执行 `fence.i`
12. `module_manager` 解析 `module_get_exports()` 并协调当前槽位所有者
13. 宿主通过 runtime wrapper 调用模块

## 5. 当前宿主侧已经做了什么

如果你想复用这套机制，宿主至少要有下面这些部分：

### 5.1 链接脚本预留槽位

当前在 [sdk/HAL/Link.ld](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/sdk/HAL/Link.ld:17>) 里做了两件事：

- 把 RAM 分成 `RAM_APP` 和 `RAM_MODULE`
- 预留一个固定运行槽位给动态模块

当前配置是：

- `RAM_MODULE = 16KB`
- 起始地址是 `0x2000C000`

### 5.2 运行时加载器

[app/module_loader.c](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/module_loader.c:1>) 负责：

- 校验模块来源是否在 `FLASH1`
- 校验镜像头和 CRC32
- 拷贝 payload 到 `RAM_MODULE`
- 执行 `fence.i`
- 解析导出表

### 5.3 宿主 API 适配

宿主需要决定模块能调用什么能力。

当前 ABI 里的 `module_host_api_t` 很小，只提供了：

- `set_output`
- `log_text`
- `user`

LED demo 用的是 [app/tasks/tmos_led_task.c](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/tasks/tmos_led_task.c:1>)。

`impact_displacement` 用的是 [app/module_manager.c](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/module_manager.c:1>)、[app/impact_module_runtime.c](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/impact_module_runtime.c:1>) 和 [app/tasks/sensor_task.c](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/tasks/sensor_task.c:1>)。

## 6. 模块作者需要做什么

如果你要新建一个模块，最少需要做这些：

1. 在 `modules/` 下新建目录
2. 写 `module_entry.c`
3. 包含 `module_abi.h`
4. 导出一个 `module_exports_t`
5. 实现 `module_get_exports()`
6. 在 `scripts/build_modules.py` 里把它加入模块清单

最小示例：

```c
#include "module_abi.h"

extern char __global_pointer$[];

static void module_init(module_ctx_t *ctx, const module_host_api_t *host)
{
    (void)ctx;
    if (host != 0 && host->log_text != 0)
    {
        host->log_text(host->user, "my_algo init");
    }
}

static void module_tick(module_ctx_t *ctx, const module_host_api_t *host, uint32_t tick)
{
    (void)host;
    ctx->words[0] = tick;
}

static const module_exports_t g_module_exports = {
    .magic = MODULE_ABI_MAGIC,
    .abi_version = MODULE_ABI_VERSION,
    .name = "my_algo",
    .global_pointer = (uintptr_t)__global_pointer$,
    .ctx_size = sizeof(module_ctx_t),
    .init = module_init,
    .tick = module_tick,
    .call = 0,
    .deinit = 0,
};

__attribute__((section(".text.module_get_exports")))
const module_exports_t *module_get_exports(void)
{
    return &g_module_exports;
}
```

## 7. 两种推荐模块形态

### 7.1 `tick` 型模块

适合：

- LED 演示
- 简单周期逻辑
- 很小的状态机

特点：

- 宿主周期性调用 `tick()`
- 模块状态放在 `module_ctx_t`
- 返回结果一般通过 `ctx` 或宿主回调表达

### 7.2 `call` 型模块

适合：

- 算法库
- 编解码库
- 明确的命令式 API

特点：

- 宿主按命令调用 `call()`
- 适合 `configure/reset/process/get_result` 这类接口
- `impact_displacement` 就是这种形态

`impact_displacement` 的命令定义在 [modules/include/impact_module_api.h](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/modules/include/impact_module_api.h:1>)，例如：

- `IMPACT_MODULE_CMD_GET_DEFAULT_CFG`
- `IMPACT_MODULE_CMD_CONFIGURE`
- `IMPACT_MODULE_CMD_RESET`
- `IMPACT_MODULE_CMD_BEGIN_EVENT`
- `IMPACT_MODULE_CMD_SET_BASELINE`
- `IMPACT_MODULE_CMD_FEED_SAMPLE`
- `IMPACT_MODULE_CMD_END_EVENT`

## 8. 使用这套机制时宿主需要改什么

如果你要把一个真实算法改成装载模块，宿主通常要改这几类东西：

### 8.1 把算法源码从主固件静态链接里拿出去

例如这次 `impact_displacement`：

- 原来主固件直接编 `lib/impact_displacement/src/impact_displacement.c`
- 现在改成模块构建脚本单独编
- 宿主只保留一层 runtime wrapper

### 8.2 新增一个运行时包装层

例如：

- [app/impact_module_runtime.c](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/impact_module_runtime.c:1>)
- [app/include/impact_module_runtime.h](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/include/impact_module_runtime.h:1>)

它负责：

- 触发装载
- 缓存模块导出表
- 负责 `gp` 切换
- 把宿主函数调用转成模块 `call()`

### 8.3 把原来直接调用算法库的地方改成调用 runtime wrapper

例如这次是在 [app/tasks/sensor_task.c](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/tasks/sensor_task.c:1>) 里把：

- `impact_disp_init`
- `impact_disp_reset`
- `impact_disp_begin_event`
- `impact_disp_set_baseline_mg`
- `impact_disp_feed_sample`
- `impact_disp_end_event`

改成了 `impact_module_runtime_*`

### 8.4 处理单槽冲突

当前是单个 `RAM_MODULE` 槽位，所以同一时刻只能装一个模块。

这意味着：

- LED demo 和 `impact_displacement` 不能同时都用同一个槽位
- 当前工程里为了避免冲突，已经先停掉了 LED 动态模块任务

如果你后面要多模块切换，要做一个统一的模块管理器，而不是让多个任务各自直接占槽。

## 9. 使用时最重要的注意事项

### 9.1 这不是 PIC

当前模块不是位置无关代码。

它能跑的前提是：

- 编译地址固定
- 运行地址也固定

所以不要把同一个模块随便搬到别的 RAM 地址直接运行。

### 9.2 一定要执行 `fence.i`

因为你是先把“指令”写进 RAM，再从 RAM 取指执行。

如果不做 `fence.i`，CPU 可能取到旧指令视图。

### 9.3 RISC-V 的 `gp` 必须处理

这是这次从 LED demo 升级到真实算法模块时最关键的坑。

复杂模块一旦：

- 带小数据区
- 带运行库
- 带常量/静态对象

就很可能依赖自己的 `gp`。

当前 ABI 里专门加了：

- `module_exports_t.global_pointer`

宿主在调用模块入口前要切换 `gp`，返回后再恢复。

如果不做这一步，模块非常容易直接跑飞或复位。

### 9.4 尽量少依赖 `libc`

能不用 `memcpy/memset/printf` 就不用。

原因不是“完全不能用”，而是：

- 会带来额外体积
- 会引入更多运行时依赖
- 更容易触发 `gp`、小数据区、运行库初始化相关问题

这次 `impact_displacement` 模块为了稳定，已经把若干 `memset` 改成了模块内部的简单清零函数。

### 9.5 模块内部状态尽量自包含

推荐：

- 状态全部放模块自己的上下文里
- 宿主只传命令和输入数据

不推荐：

- 模块直接读宿主全局变量
- 模块偷偷依赖外部静态状态

### 9.6 先模块化，再加密

如果后面要做保护，不要一开始就把“模块化”和“加密”一起做。

推荐顺序：

1. 明文模块先跑通
2. ABI 先稳定
3. 校验先稳定
4. 最后再加密

## 10. 当前已经实现了什么

当前仓库里，这套机制已经实现了：

- 固定 `RAM_MODULE` 槽位
- `FLASH1` 模块存储区
- 模块镜像头 + CRC32 运行时校验
- `fence.i`
- 模块导出表解析
- `gp` 切换
- LED demo 多模块切换
- `impact_displacement` 真实算法模块装载执行

## 11. 当前还没实现什么

还没做的包括：

- 多模块同时常驻
- 真正的 ELF 重定位
- 通用动态链接
- 安全签名认证
- 模块加密
- 自动模块仲裁器

所以当前它更准确的定位是：

- 固定装载地址
- 单槽位
- 明确 ABI
- 运行时校验
- 可切换的 RAM 模块系统

## 12. 推荐落地顺序

如果你后面继续往下做，建议顺序是：

1. 先做纯算法模块
2. 再做轻量状态机模块
3. 再补统一模块管理器
4. 再做镜像加密/认证
5. 最后再考虑更大的协议子模块

对这个工程来说，下一类很适合继续尝试的模块是：

- 滤波算法
- 帧编解码
- 局部判定逻辑
- `Ad-Hoc-lib` 中边界清晰的纯算法/纯处理子模块
