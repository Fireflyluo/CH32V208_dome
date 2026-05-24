# 单槽位模块管理器说明

## 1. 为什么要加这一层

前面的 LED demo 和 `impact_displacement` 都已经证明了：

- 模块镜像可以放在 `FLASH1`
- 运行时可以搬到 `RAM_MODULE`
- 宿主可以通过 ABI 调用模块

但在没有管理器之前，每个 runtime 都要自己重复做这些事情：

- `module_loader_copy_from_flash`
- `module_loader_resolve_exports`
- `gp` 切换
- `init/deinit`
- 槽位占用判断

这会带来两个问题：

1. 代码重复，后面接新模块时还要再写一遍
2. 单槽位冲突没有统一语义，不同模块可能互相覆盖

所以当前增加了一层简单管理器：

- [app/include/module_manager.h](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/include/module_manager.h:1>)
- [app/module_manager.c](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/module_manager.c:1>)

## 2. 它的定位

它不是：

- 多模块并发运行器
- 动态链接器
- 热更新框架

它是：

- 单个 `RAM_MODULE` 槽位的统一协调层

也就是说，当前模型仍然只有一个可执行槽位。  
管理器做的是“谁能装进去、怎么安全调用、冲突时怎么返回”。

## 3. 当前提供了什么能力

当前接口在 [module_manager.h](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/include/module_manager.h:1>)。

核心能力有：

- `module_runtime_setup()`
  初始化某个业务自己的 runtime 容器和 host API
- `module_manager_load()`
  尝试把某个模块镜像装进当前唯一槽位
- `module_manager_call_init()`
  调模块 `init()`
- `module_manager_call_tick()`
  调模块 `tick()`
- `module_manager_call_command()`
  调模块 `call()`
- `module_manager_unload()`
  调模块 `deinit()` 并释放槽位
- `module_manager_active_owner()`
  查询当前槽位被谁占着
- `module_manager_active_program()`
  查询当前装进去的是哪个模块

## 4. 运行时对象是什么

每个业务模块各自维护一个 `module_runtime_t`：

```c
typedef struct
{
    const char *owner;
    const char *program_name;
    module_ctx_t ctx;
    module_host_api_t host;
    const module_exports_t *exports;
} module_runtime_t;
```

这里有一个关键点：

- `module_runtime_t` 是“业务侧的宿主容器”
- 真正的 `RAM_MODULE` 物理槽位仍然是全局唯一的

所以：

- `impact` 有自己的 runtime
- LED demo 也有自己的 runtime
- 但任何时刻只有一个 runtime 能真正成为 active owner

## 5. 管理器内部做了什么

### 5.1 统一装载

`module_manager_load()` 内部统一做：

1. 检查参数
2. 检查当前槽位是否已被别的 owner 占用
3. 如果同一个 runtime 正在切换到另一个程序，先 `unload`
4. 调 `module_loader_copy_from_flash()`
5. 调 `module_loader_resolve_exports()`
6. 检查 `ctx_size`
7. 记录新的 active runtime

### 5.2 统一 gp 切换

`module_manager_call_init()`、`module_manager_call_tick()`、`module_manager_call_command()` 内部都统一做：

1. 保存宿主 `gp`
2. 切到模块 `global_pointer`
3. 调模块入口
4. 恢复宿主 `gp`

这样后续再接新的复杂算法模块时，不需要在每个 runtime 里重复写一遍这套代码。

### 5.3 统一槽位冲突语义

如果当前槽位已经被别的 runtime 占用，`module_manager_load()` 会返回：

- `MODULE_MANAGER_BUSY`

宿主侧就可以根据这个统一结果做日志或降级处理，而不是互相覆盖。

## 6. 当前是怎么接入 impact 模块的

`impact_displacement` 现在的结构是：

- [app/impact_module_runtime.c](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/impact_module_runtime.c:1>)
- [app/tasks/sensor_task.c](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/tasks/sensor_task.c:1>)

其中 `impact_module_runtime.c` 只负责：

- 准备自己的 `module_runtime_t`
- 调用 `module_manager_load()`
- 把业务接口继续包装成 `impact_module_runtime_*`

真正的装载和 `gp` 切换已经交给管理器。

## 7. 当前是怎么接入 LED demo 的

LED demo 现在也已经改成走同一层管理器：

- [app/tasks/tmos_led_task.c](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/tasks/tmos_led_task.c:1>)

它通过：

- `module_manager_load()`
- `module_manager_call_init()`
- `module_manager_call_tick()`
- `module_manager_unload()`

来切换不同点灯模块。

如果以后你重新启用 LED demo，而此时 `impact` 已经占着槽位，LED 侧会统一拿到 `BUSY`，而不是悄悄覆盖掉算法模块。

## 8. 这层管理器暂时没做什么

当前没做的包括：

- 自动抢占切换
- 优先级仲裁
- 多槽位调度
- 同时驻留多个模块
- 模块引用计数
- 等待队列

也就是说，这还是一个“简单管理器”，不是完整调度器。

## 9. 后面还能怎么演进

如果后面继续推进，比较自然的演进顺序是：

1. 给 `module_manager` 增加显式的 `switch` / `takeover` 接口
2. 定义模块 owner 的优先级策略
3. 增加“谁在占用槽位、为什么占用”的诊断日志
4. 如果后续确实需要，再考虑多槽位或换出策略

对当前这个工程来说，这一层已经足够把“重复代码”变成“统一入口”，也足够把“单槽位冲突”从隐式行为变成显式状态。
