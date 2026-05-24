# impact_displacement 模块适配说明

## 1. 这份文档解决什么问题

这份文档专门解释一件事：

- 如果手里已经有一个“纯算法库”
- 想把它做成 `FLASH1 -> RAM_MODULE` 的可装载模块
- 宿主只在运行时加载并调用它

那么应该怎么改。

`impact_displacement` 是当前仓库里第一个完整的算法模块例子。它比 LED demo 更接近真实使用场景，因为它已经包含：

- 独立算法实现
- 模块私有状态
- 命令式 ABI
- 运行时镜像校验
- `gp` 切换

## 2. 先判断这个算法适不适合模块化

`impact_displacement` 适合做装载模块，核心原因是它天然像一个“函数库”：

- 输入是采样数据和配置
- 输出是位移结果和诊断信息
- 算法本身不直接访问 I2C、GPIO、中断、TMOS
- 算法状态可以放进自己的上下文里

这类代码适合做模块。

反过来，如果一段代码强依赖下面这些东西，就不适合直接照搬：

- 中断向量
- TMOS 任务注册和生命周期
- 大量外设寄存器直接访问
- 宿主里的全局变量
- 多个大模块同时常驻 RAM

## 3. 这次到底把什么搬进模块了

这次不是把整个 `sensor_task` 搬进模块，而是只搬了算法核心。

仍然留在宿主侧的内容：

- 传感器初始化与采样
- FIFO / 环形缓冲管理
- 触发状态机
- 基线窗口统计
- 结果日志打印
- TMOS 调度

搬进模块的内容：

- `impact_disp_get_version`
- `impact_disp_get_default_cfg`
- `impact_disp_init`
- `impact_disp_reset`
- `impact_disp_begin_event`
- `impact_disp_set_baseline_mg`
- `impact_disp_feed_sample`
- `impact_disp_end_event`

也就是说，宿主负责“什么时候算、拿哪些样本算”，模块负责“怎么把这些样本算成结果”。

## 4. 模块侧要做什么

模块侧入口在 [modules/impact_displacement/module_entry.c](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/modules/impact_displacement/module_entry.c:1>)。

### 4.1 定义模块私有状态

先定义一个只属于模块自己的状态结构：

```c
typedef struct
{
    impact_disp_cfg_t cfg;
    impact_disp_ctx_t algo;
    impact_module_diag_t last_diag;
    uint8_t configured;
    uint8_t reserved[3];
} impact_module_state_t;
```

这里做的是两件事：

- 把原算法库需要的上下文 `impact_disp_ctx_t` 收进模块
- 把模块运行时需要缓存的配置和诊断也一起收进模块

宿主不会直接操作这块结构体，只会给模块一块 `module_ctx_t` 大小的内存。

### 4.2 写一个命令分发层

算法库原本是直接函数调用。模块化之后，需要把它变成一个统一入口：

```c
static int32_t module_call(module_ctx_t *ctx,
                           const module_host_api_t *host,
                           uint32_t command,
                           const void *input,
                           void *output)
```

然后在内部用 `switch(command)` 去转发：

- `IMPACT_MODULE_CMD_GET_DEFAULT_CFG` -> `impact_disp_get_default_cfg`
- `IMPACT_MODULE_CMD_CONFIGURE` -> `impact_disp_init`
- `IMPACT_MODULE_CMD_RESET` -> `impact_disp_reset`
- `IMPACT_MODULE_CMD_BEGIN_EVENT` -> `impact_disp_begin_event`
- `IMPACT_MODULE_CMD_SET_BASELINE` -> `impact_disp_set_baseline_mg`
- `IMPACT_MODULE_CMD_FEED_SAMPLE` -> `impact_disp_feed_sample`
- `IMPACT_MODULE_CMD_END_EVENT` -> `impact_disp_end_event`

这一步是适配的核心。

以后如果再接一个新算法模块，通常也要先做这个“原始算法 API -> 模块命令 ABI”的映射。

### 4.3 导出模块元信息

导出表定义在同一个文件里：

```c
static const module_exports_t g_module_exports = {
    .magic = MODULE_ABI_MAGIC,
    .abi_version = MODULE_ABI_VERSION,
    .name = "impact_displacement",
    .global_pointer = (uintptr_t)__global_pointer$,
    .ctx_size = sizeof(impact_module_state_t),
    .init = 0,
    .tick = 0,
    .call = module_call,
    .deinit = module_deinit,
};
```

这里最重要的两个字段是：

- `global_pointer`
- `ctx_size`

`global_pointer` 让宿主知道切到哪个 `gp` 才能正确访问模块自己的小数据区。  
`ctx_size` 让宿主知道自己的 `module_ctx_t` 预留空间够不够。

最后通过 [module_get_exports()](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/modules/impact_displacement/module_entry.c:214>) 把这张表暴露给宿主。

## 5. 构建链路怎么变

构建脚本在 [scripts/build_modules.py](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/scripts/build_modules.py:23>)。

`impact` 这一组现在是：

```python
{
    "name": "impact_displacement",
    "sources": [
        ROOT / "modules" / "impact_displacement" / "module_entry.c",
        ROOT / "lib" / "impact_displacement" / "src" / "impact_displacement.c",
    ],
    "includes": [
        ROOT / "modules" / "include",
        ROOT / "lib" / "impact_displacement" / "inc",
    ],
}
```

也就是说：

- 模块入口 wrapper 要一起编
- 真正的算法实现也要一起编进模块镜像

然后脚本会：

1. 用 [modules/ram_demo/module_link.ld](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/modules/ram_demo/module_link.ld:1>) 固定链接到 `RAM_MODULE`
2. 生成 payload
3. 加镜像头和 `CRC32`
4. 生成 [app/impact_module_programs.c](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/impact_module_programs.c:1>)
5. 把镜像放进 `.flash1_module`

这意味着主固件最终只看到一个 `const uint8_t blob[]`，看不到模块内部源码结构。

## 6. 宿主侧要做什么

宿主侧新增了一层运行时包装：[app/impact_module_runtime.c](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/impact_module_runtime.c:1>)。

它的职责不是“做算法”，而是“把模块当算法来用”。

### 6.1 装载模块

初始化流程在 [impact_module_runtime_init()](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/impact_module_runtime.c:51>)：

1. 从 `g_impact_module_programs[0]` 取模块镜像
2. 调 `module_loader_copy_from_flash()`
3. 调 `module_loader_resolve_exports()`
4. 检查 `ctx_size`
5. 可选调用 `init`
6. 调一次 `GET_VERSION` 做 smoke test

这样，宿主对上层业务暴露的就只剩一个“模块已可用”的状态。

### 6.2 切换 gp 后再调用模块

真正的调用统一封装在 [impact_call()](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/impact_module_runtime.c:14>)。

它会：

1. 保存宿主当前 `gp`
2. 读取模块导出表里的 `global_pointer`
3. 切到模块 `gp`
4. 调模块 `call()`
5. 恢复宿主 `gp`

这一点对复杂算法模块是必须的。

原因是 `impact_displacement` 已经不再是 LED demo 那种纯寄存器代码。它会访问：

- `.sdata`
- `.data`
- `.bss`
- `libm/libgcc` 相关辅助符号

如果不切 `gp`，这些访问就可能落到宿主的地址语义上，结果通常是：

- 读错全局数据
- 跑飞
- 复位

## 7. 通用 loader 在做什么

loader 在 [app/module_loader.c](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/module_loader.c:1>)。

它负责三件事：

### 7.1 校验镜像

先检查 [module_image.h](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/modules/include/module_image.h:1>) 里定义的镜像头：

- `magic`
- `version`
- `header_size`
- `payload_size`
- `payload_crc32`
- `load_offset`

再重新计算 payload 的 `CRC32`。

只有通过后，才允许复制到 RAM。

### 7.2 复制到 RAM_MODULE

真正的复制发生在 [module_loader_copy_from_flash()](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/module_loader.c:154>)。

它会把 payload 从 `FLASH1` 复制到 `RAM_MODULE` 槽位。

### 7.3 执行 fence.i

复制后会执行：

```c
__asm volatile("fence.i");
```

原因是 CPU 之后要从这块 RAM 里取指执行新代码。  
`fence.i` 是让 CPU 丢掉旧的取指视图，看到刚写进去的新指令。

## 8. 业务代码里怎么替换

真正消费这个模块的是 [app/tasks/sensor_task.c](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/tasks/sensor_task.c:25>)。

改造方式不是重写整个任务，而是把原来直接调用算法库的地方，改成调 runtime wrapper。

初始化阶段：

- `impact_module_runtime_init()`
- `impact_module_runtime_get_default_cfg(&s_impact_cfg)`
- 宿主按项目需要覆写采样率、时间窗等参数
- `impact_module_runtime_configure(&s_impact_cfg)`

事件处理阶段：

- `impact_module_runtime_reset()`
- `impact_module_runtime_begin_event(event_id)`
- `impact_module_runtime_set_baseline(...)`
- 循环 `impact_module_runtime_feed_sample(...)`
- `impact_module_runtime_end_event(&response)`

这是一条非常重要的边界：

- 事件窗口仍然由宿主决定
- 基线均值仍然由宿主统计
- 样本顺序仍然由宿主提供
- 模块只负责对给定样本序列做运算

## 9. 这次实现说明了什么

`impact_displacement` 这个例子说明，这套机制已经不只是“点灯 PoC”了，而是可以承接真实算法库。

当前已经被证明可行的点：

- 算法库可以单独编成模块镜像
- 镜像可以放在 `FLASH1`
- 运行时可以校验后装入 `RAM_MODULE`
- 宿主可以通过统一 ABI 调算法
- 复杂模块可以靠 `gp` 切换正常运行

当前边界也很明确：

- 还是固定装载地址，不是 PIC
- 还是单槽位，不支持多个大模块同时常驻
- 现在是完整性校验，不是认证签名

## 10. 新增一个算法模块时的最小清单

如果你后面要再接一个新的算法模块，最小步骤就是：

1. 判断它是否适合做“宿主驱动、模块计算”
2. 在 `modules/include/` 下定义命令接口和输入输出结构
3. 写一个 `module_entry.c`，把原始算法 API 映射成 `call()`
4. 把算法状态收进模块私有 state
5. 在 `scripts/build_modules.py` 里把 wrapper 和算法源码一起加入模块构建
6. 新增一个 `*_module_runtime.c` 封装装载、导出表解析和 `gp` 切换
7. 在业务代码里把原来的直接库调用替换成 `*_module_runtime_*`

如果一段代码做完这 7 步后仍然很难塞进去，通常说明它并不是一个适合先模块化的算法边界，而更像一个子系统。
