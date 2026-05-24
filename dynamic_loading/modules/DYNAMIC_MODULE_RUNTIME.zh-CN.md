# CH32V208 动态模块运行原理

## 1. 本方案的准确定位

这套方案不是：

- 通用动态链接器
- 任意地址可运行的 PIC
- 带重定位器的 ELF loader

这套方案实际是：

- 固定地址编译
- 固定地址装载
- 运行时复制到 RAM
- 再通过导出表调用

所以它更准确的名字应该是：

“固定装载地址的 RAM 模块系统”

## 2. 当前运行路径

当前模块执行路径如下：

1. 模块单独编译
2. 使用独立链接脚本，固定链接到 `RAM_MODULE`
3. 生成纯二进制 payload
4. 加上镜像头和 CRC32
5. 作为数组链接进主固件的 `FLASH1`
6. 运行时由 loader 校验镜像
7. 复制到 `RAM_MODULE`
8. 执行 `fence.i`
9. 解析 `module_get_exports()`
10. 宿主通过 `init/tick/call/deinit` 调用模块

当前相关文件：

- [sdk/HAL/Link.ld](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/sdk/HAL/Link.ld:1>)
- [modules/ram_demo/module_link.ld](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/modules/ram_demo/module_link.ld:1>)
- [app/module_loader.c](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/module_loader.c:1>)
- [scripts/build_modules.py](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/scripts/build_modules.py:1>)

## 3. 为什么最开始 LED demo 很容易成功

LED demo 之所以容易成功，是因为它几乎满足“最理想模块”的全部条件：

- 代码很小
- 不需要复杂数据结构
- 几乎没有运行库依赖
- 只有简单的 `tick` 逻辑
- 只通过宿主回调控制一个输出

所以第一阶段 PoC 证明的是：

- 固定槽位可行
- `FLASH1 -> RAM_MODULE` 可行
- `fence.i` 后跳转执行可行

但它并不能自动证明“复杂算法模块也一定没坑”。

## 4. 为什么 `impact_displacement` 比 LED 难很多

`impact_displacement` 这种真实算法模块，会比 LED demo 多出几类问题：

### 4.1 体积问题

它不再是几个指令，而是一个真实算法库：

- 有更多函数
- 有更多常量
- 会带来更多代码尺寸

这次实测模块 payload 约 `13.2KB`，因此：

- 原来 `8KB` 槽位不够
- 这次已经把 `RAM_MODULE` 扩到了 `16KB`

### 4.2 运行库依赖

算法库里会自然出现：

- `sqrtf`
- `sinf`
- 浮点辅助函数
- `memset` 之类的工具函数

这会引入：

- `libm`
- `libgcc`
- 小数据区访问
- 更多内部状态和符号

### 4.3 `gp` 问题

这是最关键的一点。

在 RISC-V 上，模块和宿主可能拥有不同的：

- `__global_pointer$`

如果模块内部使用自己的小数据区，而宿主直接用宿主 `gp` 去调用模块，结果通常就是：

- 读错全局/静态数据
- 跳飞
- 直接复位

所以当前 ABI 已经加了：

- `module_exports_t.global_pointer`

宿主在调用模块入口前必须：

1. 保存宿主 `gp`
2. 切换到模块 `gp`
3. 调用模块
4. 恢复宿主 `gp`

当前实现位置：

- [modules/include/module_abi.h](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/modules/include/module_abi.h:1>)
- [app/module_loader.c](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/module_loader.c:1>)
- [app/impact_module_runtime.c](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/impact_module_runtime.c:1>)

## 5. 为什么要有 `fence.i`

`fence.i` 是 RISC-V 的指令同步指令。

在这里的作用是：

- 先把模块字节复制到 RAM
- 再告诉 CPU：后续从这块 RAM 取指时，要看到刚刚写进去的新代码

不做这一步，CPU 可能还会按旧的取指状态执行。

当前实现位置：

- [app/module_loader.c](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/app/module_loader.c:36>)

## 6. 为什么模块镜像要做头部和 CRC32

当前模块镜像不是裸 payload，而是：

- `module_image_header_t`
- `payload`

头部定义在：

- [modules/include/module_image.h](</d:/Desktop/ch32/0.ch32v208_dome/dynamic_loading/modules/include/module_image.h:1>)

当前校验字段：

- `magic`
- `version`
- `header_size`
- `payload_size`
- `payload_crc32`
- `load_offset`

运行时校验顺序：

1. 检查镜像是否位于 `FLASH1`
2. 检查 `magic/version`
3. 检查头大小和 payload 大小
4. 重新计算 payload CRC32
5. 校验通过才允许复制到 `RAM_MODULE`

它当前提供的是：

- 完整性校验

它当前还不是：

- 带密钥认证
- 数字签名
- 加密保护

## 7. 为什么要区分 `tick` 和 `call`

一开始 ABI 只有：

- `init`
- `tick`
- `deinit`

这对 LED demo 足够，但对真实算法不够自然。

例如 `impact_displacement` 更像下面这种命令式接口：

- `get_default_cfg`
- `configure`
- `reset`
- `begin_event`
- `set_baseline`
- `feed_sample`
- `end_event`

如果强行塞进 `tick`，会带来：

- 宿主和模块语义混乱
- 参数打包很丑
- 状态难维护

所以当前 ABI 已经扩成：

- `init`
- `tick`
- `call`
- `deinit`

这样：

- 简单状态机继续用 `tick`
- 算法库走 `call`

## 8. 当前单槽位模型意味着什么

当前 `RAM_MODULE` 只有一个槽位。

这意味着：

- 同一时刻只能装一个模块
- 模块之间不能并行常驻
- 谁先装进去，谁就占住这块 RAM

所以这次为了让 `impact_displacement` 跑通，主程序里先停掉了 LED 动态模块任务。

这不是 bug，而是当前模型的设计边界。

如果后面需要：

- LED 模块
- 滤波模块
- 协议模块

按需切换，就要再加一层模块管理器，负责：

- 谁来装载
- 谁来卸载
- 什么时候切换
- 切换前后谁负责清理状态

## 9. 宿主侵入点到底有哪些

当前要把一个真实模块接进来，宿主通常要改这几类点：

### 9.1 构建层

- 从主固件 `add_files()` 里移除该算法源码
- 改为在 `scripts/build_modules.py` 里单独编译成模块

### 9.2 运行时包装层

- 新增一个 `*_module_runtime.c`
- 负责装载、缓存导出表、切 `gp`、封装 `call`

### 9.3 业务调用层

- 把原来直接调用算法库的地方
- 改成调用 `*_module_runtime_*`

### 9.4 资源冲突层

- 如果当前已经有别的动态模块占用单槽位
- 需要决定是停用、切换，还是做统一管理

## 10. 当前实现已经证明了什么

这次 `impact_displacement` 模块化已经证明了这些点：

- 不是只有“几字节 demo 指令”能装到 RAM
- 真实算法库也能按这个模型装载
- 带镜像头和 CRC32 的运行时校验可用
- 带 `gp` 切换的宿主调用链可用
- `sensor_task -> runtime wrapper -> RAM module` 这条路径已在板子上实际跑通

## 11. 当前还没有做什么

当前还没做：

- 模块加密
- 签名认证
- 通用重定位
- 多槽位并行
- 自动模块仲裁
- 模块热切换回收策略

所以对它的预期应该是：

- 一套稳定的固定地址装载运行机制

而不是：

- 一套完整通用的动态装载操作系统能力

## 12. 后续演进建议

建议按下面顺序继续：

1. 继续把更多纯算法做成 `call` 型模块
2. 总结一套统一模块管理器
3. 给镜像头增加更强的认证信息
4. 再考虑把关键库做加密存储
5. 最后再碰更复杂的协议子系统

对这个工程来说，最合适的下一批候选是：

- 滤波算法
- 帧编解码
- 纯判定逻辑
- `Ad-Hoc-lib` 里边界清晰的纯处理子模块
