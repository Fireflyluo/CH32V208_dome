-- ============================================================================
-- 项目构建配置文件 (xmake.lua)
-- 项目名称: CH32V208GBU_Templete
-- 功能描述: 基于CH32V208微控制器的TMOS多任务应用构建配置
-- 架构: RISC-V (RV32IMACXW)
-- 工具链: WCH RISC-V Embedded GCC
-- ============================================================================

--
set_project("CH32V208GBU_Templete")

-- 设置构建模式规则（调试/发布）
add_rules("mode.debug", "mode.release")

-- 默认构建模式设置为调试模式
set_config("mode", "debug")

-- ============================================================================
-- 构建选项配置
-- ============================================================================

-- WCH GCC工具链版本选择选项
option("wch_gcc_ver")
    set_default("15")                    -- 默认使用GCC 15版本
    set_showmenu(true)                   -- 在配置菜单中显示
    set_values("15", "12", "auto")       -- 支持的版本选项
    set_description("Select WCH RISC-V GCC toolchain version")  -- 选项描述
option_end()

-- 运行时日志输出控制选项
option("log_print")
    set_default("true")                  -- 默认启用日志输出
    set_showmenu(true)                   -- 在配置菜单中显示
    set_values("true", "false")          -- 开关选项
    set_description("Enable runtime LOG_PRINT outputs")  -- 选项描述
option_end()

-- 射频节点ID配置（0~20）
option("rf_tg_id")
    set_default("0")
    set_showmenu(true)
    set_description("RF tg_id for this firmware image (0..20)")
option_end()

-- ============================================================================
-- 工具链路径配置
-- ============================================================================

-- WCH MRS工具链根目录（根据实际安装路径修改）
local toolchain_root = "e:/APP/MRS2/MounRiver_Studio2/resources/app/resources/win32/components/WCH/Toolchain"
local selected_ver = get_config("wch_gcc_ver") or "15"      -- 获取用户选择的GCC版本
local log_print_cfg = tostring(get_config("log_print") or "true")  -- 获取日志输出配置
local rf_tg_id_cfg = tonumber(get_config("rf_tg_id") or "0") or 0
local toolchain_path = nil

if rf_tg_id_cfg < 0 then
    rf_tg_id_cfg = 0
elseif rf_tg_id_cfg > 20 then
    rf_tg_id_cfg = 20
end

-- 根据选择的版本确定工具链路径
if selected_ver == "12" then
    toolchain_path = toolchain_root .. "/RISC-V Embedded GCC12"
elseif selected_ver == "15" then
    toolchain_path = toolchain_root .. "/RISC-V Embedded GCC15"
else
    -- 自动检测模式：优先使用GCC15，如果不存在则使用GCC12
    local gcc15_path = toolchain_root .. "/RISC-V Embedded GCC15"
    local gcc12_path = toolchain_root .. "/RISC-V Embedded GCC12"
    if os.isdir(gcc15_path) then
        toolchain_path = gcc15_path
    else
        toolchain_path = gcc12_path
    end
end

-- 构建工具链二进制目录和交叉编译前缀
local toolchain_bin = toolchain_path .. "/bin"
local cross_prefix = toolchain_bin .. "/riscv32-wch-elf-"
-- 兼容不同版本的工具链命名（有些版本使用riscv-wch-elf-前缀）
if not os.isfile(cross_prefix .. "gcc.exe") then
    cross_prefix = toolchain_bin .. "/riscv-wch-elf-"
end

-- ============================================================================
-- 平台和架构配置
-- ============================================================================

-- 设置目标平台为交叉编译
set_plat("cross")
-- 设置目标架构为RISC-V
set_arch("riscv")

-- 保存SDK路径到配置中，供后续使用
set_config("sdk", toolchain_path)

-- 自动生成compile_commands.json文件，用于VSCode等编辑器的智能提示
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})

-- ============================================================================
-- 主要构建目标配置
-- ============================================================================

target("CH32V208GBU_Templete")
    -- 设置目标类型为可执行二进制文件
    set_kind("binary")
    -- 设置输出文件扩展名为.elf
    set_extension(".elf")
    
    -- ============================================================================
    -- 工具链配置
    -- ============================================================================

    -- 配置交叉编译工具链路径
    set_toolset("cxx", cross_prefix .. "g++.exe")    -- C++编译器
    set_toolset("as", cross_prefix .. "gcc.exe")     -- 汇编器
    set_toolset("ld", cross_prefix .. "gcc.exe")     -- 链接器
    set_toolset("ar", cross_prefix .. "ar.exe")      -- 归档工具
    set_toolset("ranlib", cross_prefix .. "ranlib.exe")  -- 索引生成工具
    
    -- ============================================================================
    -- 编译器架构选项
    -- ============================================================================

    -- 设置RISC-V架构和ABI选项
    add_cflags("-march=rv32imacxw", "-mabi=ilp32", {force = true})      -- C编译器标志
    add_asflags("-march=rv32imacxw", "-mabi=ilp32", {force = true})     -- 汇编器标志
    add_ldflags("-march=rv32imacxw", "-mabi=ilp32", {force = true})     -- 链接器标志
    
    -- ============================================================================
    -- 源文件列表
    -- ============================================================================

    -- 添加所有需要编译的源文件
    add_files(
        -- 应用层TMOS任务
        "app/tasks/tmos_led_task.c",
        "app/tasks/i2c_request_task.c",
        "app/tasks/sensor_task.c",
        "app/tasks/display_task.c",
        "app/tasks/serial_upload_task.c",
        "app/tasks/ad_hoc_task.c",
        "app/adapters/adhoc_link_aros.c",
        
        -- USB驱动库
        "sdk/USBLIB/USB-Driver/src/usb_core.c",
        "sdk/USBLIB/USB-Driver/src/usb_init.c",
        "sdk/USBLIB/USB-Driver/src/usb_int.c",
        "sdk/USBLIB/USB-Driver/src/usb_mem.c",
        "sdk/USBLIB/USB-Driver/src/usb_regs.c",
        "sdk/USBLIB/USB-Driver/src/usb_sil.c",
        
        -- USB配置文件
        "sdk/USBLIB/CONFIG/hw_config.c",
        "sdk/USBLIB/CONFIG/usb_desc.c",
        "sdk/USBLIB/CONFIG/usb_endp.c",
        "sdk/USBLIB/CONFIG/usb_istr.c",
        "sdk/USBLIB/CONFIG/usb_prop.c",
        "sdk/USBLIB/CONFIG/usb_pwr.c",
        
        -- 启动文件
        "sdk/Startup/startup_ch32v20x_D8W.S",
        
        -- 外设驱动库
        "sdk/Peripheral/src/ch32v20x_adc.c",
        "sdk/Peripheral/src/ch32v20x_bkp.c",
        "sdk/Peripheral/src/ch32v20x_can.c",
        "sdk/Peripheral/src/ch32v20x_crc.c",
        "sdk/Peripheral/src/ch32v20x_dbgmcu.c",
        "sdk/Peripheral/src/ch32v20x_dma.c",
        "sdk/Peripheral/src/ch32v20x_exti.c",
        "sdk/Peripheral/src/ch32v20x_flash.c",
        "sdk/Peripheral/src/ch32v20x_gpio.c",
        "sdk/Peripheral/src/ch32v20x_i2c.c",
        "sdk/Peripheral/src/ch32v20x_iwdg.c",
        "sdk/Peripheral/src/ch32v20x_misc.c",
        "sdk/Peripheral/src/ch32v20x_opa.c",
        "sdk/Peripheral/src/ch32v20x_pwr.c",
        "sdk/Peripheral/src/ch32v20x_rcc.c",
        "sdk/Peripheral/src/ch32v20x_rtc.c",
        "sdk/Peripheral/src/ch32v20x_spi.c",
        "sdk/Peripheral/src/ch32v20x_tim.c",
        "sdk/Peripheral/src/ch32v20x_usart.c",
        "sdk/Peripheral/src/ch32v20x_wwdg.c",
        
        -- BLE任务调度器（汇编实现）
        "sdk/LIB/ble_task_scheduler.S",
        
        -- 硬件抽象层(HAL)
        "sdk/HAL/MCU.c",
        "sdk/HAL/RTC.c",
        "sdk/HAL/SLEEP.c",
        
        -- 调试支持
        "sdk/Debug/debug.c",
        
        -- RISC-V核心支持
        "sdk/Core/core_riscv.c",
        
        -- 公共工具库
        "utils/ringbuffer.c",
        "utils/data_protocol.c",
        "utils/sw_timer.c",
        
        -- 传感器驱动库
        "lib/sht40/adapters/sht40_ch32_adapter.c",
        "lib/sht40/src/sht40_core.c",
        "lib/sht40/src/sht40_sync.c",
        "lib/sht40/src/sht40_async.c",
        "lib/sc7a20/adapters/sc7a20_ch32_adapter.c",
        "lib/sc7a20/src/sc7a20_core.c",
        "lib/sc7a20/src/sc7a20_sync.c",
        "lib/sc7a20/src/sc7a20_async.c",
        
        -- 碰撞位移算法库
        "lib/impact_displacement/src/impact_displacement.c",
        
        -- OLED显示屏驱动
        "lib/oled/OLED.c",
        "lib/oled/OLED_Data.c",
        "lib/AROS-RF-LIB/src/aros_rf.c",
        
        -- 板级支持包(BSP)
        "bsp/board.c",
        "bsp/drivers/src/drv_gpio.c",
        "bsp/drivers/src/drv_i2c.c",
        "bsp/drivers/src/drv_tim.c",
        "bsp/bus/src/i2c_bus_arbiter.c",
        "bsp/usb_cdc.c",
        
        -- BLE配置文件
        
        -- 应用主文件
        "app/ch32v20x_it.c",
        "app/main.c",
        "app/system_ch32v20x.c"
    )

    -- Ad-Hoc协议栈（默认主路径）
    add_files(
        "lib/Ad-Hoc-lib/src/adhoc_node.c",
        "lib/Ad-Hoc-lib/src/adhoc_frame.c",
        "lib/Ad-Hoc-lib/src/adhoc_crc8.c",
        "lib/Ad-Hoc-lib/src/adhoc_timing.c",
        "lib/Ad-Hoc-lib/src/adhoc_sm.c",
        "lib/Ad-Hoc-lib/src/adhoc_reply_list.c",
        "lib/Ad-Hoc-lib/src/adhoc_data_plane.c",
        "lib/Ad-Hoc-lib/port/ch32v208/adhoc_port_ch32.c"
    )
    
    -- ============================================================================
    -- 头文件包含路径
    -- ============================================================================

    -- 添加所有需要的头文件搜索路径
    add_includedirs(
        "sdk/Debug",
        "sdk/Core",
        "app/include",
        "app/adapters",
        "sdk/Peripheral/inc",
        "sdk/HAL/include",
        "sdk/LIB",
        "bsp",
        "bsp/drivers/inc",
        "bsp/bus/inc",
        "sdk/USBLIB/CONFIG",
        "sdk/USBLIB/USB-Driver/inc",
        "utils",
        "lib/sht40",
        "lib/sht40/inc",
        "lib/sht40/adapters",
        "lib/oled",
        "lib/sc7a20",
        "lib/sc7a20/inc",
        "lib/sc7a20/adapters",
        "lib/impact_displacement/inc",
        "lib/AROS-RF-LIB/include",
        "lib/Ad-Hoc-lib/include",
        "lib/Ad-Hoc-lib/port/ch32v208"
    )
    if os.isdir(path.join(os.scriptdir(), "test")) then
        add_includedirs("test")
    end
    
    -- 为汇编文件添加启动文件目录
    add_asflags("-I" .. os.scriptdir() .. "/sdk/Startup", {force = true})
    
    -- ============================================================================
    -- 预处理器宏定义
    -- ============================================================================

    -- 定义芯片型号
    add_defines("CH32V20x_D8W")
    -- 根据配置选项控制日志输出
    if log_print_cfg == "false" then
        add_defines("LOG_PRINT_ENABLE=0")
    else
        add_defines("LOG_PRINT_ENABLE=1")
    end
    add_defines("ARF_USE_EXTERNAL_MEM_BUF=1")
    add_defines("RF_TG_ID=" .. tostring(rf_tg_id_cfg))
    add_defines("ADHOC_ENABLE=1")
    
    -- ============================================================================
    -- C语言编译选项
    -- ============================================================================

    add_cflags(
        "-msmall-data-limit=8",          -- 小数据段限制
        "-msave-restore",                -- 使用save/restore指令优化
        "-fmessage-length=0",            -- 错误信息不换行
        "-fsigned-char",                 -- char默认为有符号
        "-ffunction-sections",           -- 每个函数放入独立段
        "-fdata-sections",               -- 每个数据放入独立段
        "-fno-common",                   -- 不使用公共符号
        "-Wall",                         -- 启用所有警告
        "-Wextra",                       -- 启用额外警告
        "-Wno-unused-parameter",         -- 忽略未使用参数警告
        "-Wunused",                      -- 未使用变量警告
        "-Wuninitialized",               -- 未初始化变量警告
        "-fmax-errors=20",               -- 最多显示20个错误
        "-std=gnu17",                    -- 使用GNU C17标准
        {force = true}
    )
    
    -- ============================================================================
    -- 调试/发布模式配置
    -- ============================================================================

    if is_mode("debug") then
        -- 调试模式：保留调试信息并启用基础优化，避免FLASH0溢出
        add_cflags("-g", "-Og", {force = true})
        add_defines("DEBUG=5")           -- 启用详细调试级别
    else
        -- 发布模式：启用优化
        add_cflags("-Os", {force = true})  -- 优化代码大小
    end
    
    -- ============================================================================
    -- 汇编器和C++编译选项
    -- ============================================================================

    -- 禁用自动忽略未知标志的策略
    set_policy("check.auto_ignore_flags", false)
    
    -- 汇编器标志
    add_asflags(
        "-march=rv32imacxw",
        "-mabi=ilp32",
        "-msmall-data-limit=8",
        "-msave-restore",
        "-fmessage-length=0",
        "-fsigned-char",
        "-ffunction-sections",
        "-fdata-sections",
        "-fno-common",
        "-g",
        "-x", "assembler-with-cpp",      -- 支持C预处理器
        {force = true}
    )
    
    -- C++编译标志
    add_cxxflags(
        "-march=rv32imacxw",
        "-mabi=ilp32",
        "-msmall-data-limit=8",
        "-msave-restore",
        "-fmessage-length=0",
        "-fsigned-char",
        "-ffunction-sections",
        "-fdata-sections",
        "-fno-common",
        "-Wall",
        "-Wuninitialized",
        "-g",
        "-std=gnu++11",
        "-fabi-version=0",               -- 兼容性ABI版本
        {force = true}
    )
    
    -- ============================================================================
    -- 链接器配置
    -- ============================================================================

    add_ldflags(
        "-T", "sdk/HAL/Link.ld",         -- 链接脚本
        "-L", "sdk/LIB",                 -- 库搜索路径
        "-nostartfiles",                 -- 不使用默认启动文件
        "-Wl,--gc-sections",             -- 移除未使用的段
        "-Wl,--print-memory-usage",      -- 打印内存使用情况
        "--specs=nano.specs",            -- 使用newlib-nano标准库
        "--specs=nosys.specs",           -- 不使用系统调用
        {force = true}
    )
    
    -- 动态生成链接器映射文件路径
    on_load(function (target)
        local map_file = path.translate(path.join(target:targetdir(), target:name() .. ".map"))
        target:add("ldflags", "-Wl,-Map," .. map_file, {force = true})
    end)
    
    -- ============================================================================
    -- 链接库配置
    -- ============================================================================

    add_linkdirs("sdk/LIB")              -- 库文件搜索路径
    add_links("wchble", "m", "c")        -- 链接的库文件（BLE库、数学库、C标准库）
    
    -- 设置最终输出文件名
    set_filename("CH32V208GBU_Templete.elf")
    
    -- ============================================================================
    -- 构建后处理
    -- ============================================================================

    -- 构建完成后执行后处理脚本（生成hex、bin等格式文件）
    after_build(function (target)
        import("scripts.post_build", {rootdir = os.scriptdir()})
        post_build.main(target, toolchain_path, cross_prefix)
    end)
