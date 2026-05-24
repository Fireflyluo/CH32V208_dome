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

local default_adhoc_repo_dir = ""

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

-- UART调试串口DMA乒乓缓冲模式
option("debug_uart_dma")
    set_default("false")
    set_showmenu(true)
    set_values("true", "false")
    set_description("Enable USART3 DMA ping-pong mode for debug UART")
option_end()

-- 射频节点ID配置（0~20）
option("rf_tg_id")
    set_default("0")
    set_showmenu(true)
    set_description("RF tg_id for this firmware image (0..20)")
option_end()

option("adhoc_role")
    set_default("bcn")
    set_showmenu(true)
    set_values("gw", "bcn")
    set_description("Ad-Hoc role for this firmware image (gw or bcn)")
option_end()

option("adhoc_gw_no")
    set_default("0")
    set_showmenu(true)
    set_description("Gateway number for gw role (0..7)")
option_end()

option("adhoc_repo_dir")
    set_default(default_adhoc_repo_dir)
    set_showmenu(true)
    set_description("Local xrepo repository for Ad-Hoc-lib; empty means use vendored lib/Ad-Hoc-lib")
option_end()

-- ============================================================================
-- 工具链路径配置
-- ============================================================================

-- WCH MRS工具链根目录（根据实际安装路径修改）
local toolchain_root = "e:/APP/MRS2/MounRiver_Studio2/resources/app/resources/win32/components/WCH/Toolchain"
local selected_ver = get_config("wch_gcc_ver") or "15"      -- 获取用户选择的GCC版本
local log_print_cfg = tostring(get_config("log_print") or "true")  -- 获取日志输出配置
local rf_tg_id_cfg = tonumber(get_config("rf_tg_id") or "0") or 0
local adhoc_role_cfg = tostring(get_config("adhoc_role") or "bcn")
local adhoc_gw_no_cfg = tonumber(get_config("adhoc_gw_no") or "0") or 0
local adhoc_repo_dir_cfg = get_config("adhoc_repo_dir") or default_adhoc_repo_dir
	local debug_uart_dma_cfg = tostring(get_config("debug_uart_dma") or "false")
local toolchain_path = nil
local use_adhoc_package_repo = false
local adhoc_package_source_dir = ""

if type(adhoc_repo_dir_cfg) == "string" then
    adhoc_repo_dir_cfg = adhoc_repo_dir_cfg:gsub("\\", "/")
end

if adhoc_repo_dir_cfg ~= "" then
    local adhoc_repo_recipe = path.join(adhoc_repo_dir_cfg, "packages/a/adhoc-lib/xmake.lua")
    if not os.isfile(adhoc_repo_recipe) then
        raise("Ad-Hoc-lib xrepo recipe not found: %s", path.translate(adhoc_repo_recipe))
    end
    adhoc_package_source_dir = path.directory(adhoc_repo_dir_cfg)
    adhoc_package_source_dir = path.join(adhoc_package_source_dir, "Lib/Ad-Hoc-lib")
    if not os.isfile(path.join(adhoc_package_source_dir, "xmake.lua")) then
        raise("Ad-Hoc-lib source xmake.lua not found: %s", path.translate(path.join(adhoc_package_source_dir, "xmake.lua")))
    end
    use_adhoc_package_repo = true
end

if rf_tg_id_cfg < 0 then
    rf_tg_id_cfg = 0
elseif rf_tg_id_cfg > 20 then
    rf_tg_id_cfg = 20
end

adhoc_role_cfg = string.lower(adhoc_role_cfg)
if adhoc_role_cfg ~= "gw" and adhoc_role_cfg ~= "bcn" then
    adhoc_role_cfg = "bcn"
end

if adhoc_gw_no_cfg < 0 then
    adhoc_gw_no_cfg = 0
elseif adhoc_gw_no_cfg > 7 then
    adhoc_gw_no_cfg = 7
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
local adhoc_package_common_flags = {
    "-march=rv32imacxw",
    "-mabi=ilp32",
    "-msmall-data-limit=8",
    "-msave-restore",
    "-fmessage-length=0",
    "-fsigned-char"
}

-- ============================================================================
-- 平台和架构配置
-- ============================================================================

-- 设置目标平台为交叉编译
set_plat("cross")
-- 设置目标架构为RISC-V
set_arch("riscv")

-- 为当前工程下所有目标统一补充 RISC-V 架构/ABI 标志，
-- 以便外部 includes() 进来的协议库 target 与主固件保持一致。
add_cflags("-march=rv32imacxw", "-mabi=ilp32", {force = true})
add_asflags("-march=rv32imacxw", "-mabi=ilp32", {force = true})
add_cxxflags("-march=rv32imacxw", "-mabi=ilp32", {force = true})
add_ldflags("-march=rv32imacxw", "-mabi=ilp32", {force = true})
add_cflags(
    "-msmall-data-limit=8",
    "-msave-restore",
    "-fmessage-length=0",
    "-fsigned-char",
    "-ffunction-sections",
    "-fdata-sections",
    "-fno-common",
    {force = true}
)
add_asflags(
    "-msmall-data-limit=8",
    "-msave-restore",
    "-fmessage-length=0",
    "-fsigned-char",
    "-ffunction-sections",
    "-fdata-sections",
    "-fno-common",
    {force = true}
)
add_cxxflags(
    "-msmall-data-limit=8",
    "-msave-restore",
    "-fmessage-length=0",
    "-fsigned-char",
    "-ffunction-sections",
    "-fdata-sections",
    "-fno-common",
    {force = true}
)

if is_mode("debug") then
    add_cflags("-g", "-Og", {force = true})
    add_cxxflags("-g", "-Og", {force = true})
else
    add_cflags("-Os", {force = true})
    add_cxxflags("-Os", {force = true})
end

-- 保存SDK路径到配置中，供后续使用
set_config("sdk", toolchain_path)

-- 生成compile_commands.json文件，用于VSCode的智能提示
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})

if use_adhoc_package_repo then
    add_repositories("firefly-embedded-libs " .. adhoc_repo_dir_cfg)
    add_requireconfs("adhoc-lib", {
        configs = {
            pic = false,
            cflags = table.concat(adhoc_package_common_flags, " "),
            cxflags = table.concat(adhoc_package_common_flags, " "),
            asflags = table.concat(adhoc_package_common_flags, " "),
            ldflags = "-march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore"
        }
    })
    add_requires("adhoc-lib")

    target("adhoc-port-ch32v208")
        set_kind("static")
        set_group("libs")
        set_default(false)

        add_files(path.join(adhoc_package_source_dir, "port/ch32v208/adhoc_port_ch32.c"))

        add_defines("CH32V20x_D8W")
        add_includedirs("lib/AROS-RF-LIB/include", "sdk/Peripheral/inc", "sdk/Core", "sdk/Debug", "app/include", "modules/include")
        add_includedirs(path.join(adhoc_package_source_dir, "port/ch32v208"), {public = true})
end

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

    set_toolset("cxx", cross_prefix .. "g++.exe")
    set_toolset("as", cross_prefix .. "gcc.exe")
    set_toolset("ld", cross_prefix .. "gcc.exe")
    set_toolset("ar", cross_prefix .. "ar.exe")
    set_toolset("ranlib", cross_prefix .. "ranlib.exe")
    
    -- ============================================================================
    -- 工具链配置
    -- ============================================================================

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
        "app/impact_module_runtime.c",
        "app/module_manager.c",
        "app/module_manager_selftest.c",
        
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
        
        -- OLED显示屏驱动
        "lib/oled/OLED.c",
        "lib/oled/OLED_Data.c",
        "lib/AROS-RF-LIB/src/aros_rf.c",
        
        -- 板级支持包(BSP)
        "bsp/board.c",
        "bsp/drivers/src/drv_gpio.c",
        "bsp/drivers/src/drv_i2c.c",
        "bsp/drivers/src/drv_tim.c",
        "bsp/drivers/src/drv_rtc.c",
        "bsp/bus/src/i2c_bus_arbiter.c",
        "bsp/usb_cdc.c",
        
        -- BLE配置文件
        
        -- 应用主文件
        "app/led_module_programs.c",
        "app/impact_module_programs.c",
        "app/module_loader.c",
        "app/ch32v20x_it.c",
        "app/main.c",
        "app/system_ch32v20x.c"
    )

    if use_adhoc_package_repo then
        add_packages("adhoc-lib")
        add_deps("adhoc-port-ch32v208")
    else
        -- Ad-Hoc协议栈（仓库内副本回退路径）
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
    end
    
    -- ============================================================================
    -- 头文件包含路径
    -- ============================================================================

    -- 添加所有需要的头文件搜索路径
    add_includedirs(
        "sdk/Debug",
        "sdk/Core",
        "app/include",
        "modules/include",
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
        "lib/AROS-RF-LIB/include"
    )
    if use_adhoc_package_repo then
        add_includedirs(path.join(adhoc_package_source_dir, "port/ch32v208"))
    else
        add_includedirs("lib/Ad-Hoc-lib/include", "lib/Ad-Hoc-lib/port/ch32v208")
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
    add_defines("CLK_OSC32K=0")
    add_defines("ARF_USE_EXTERNAL_MEM_BUF=1")
    add_defines("RF_TG_ID=" .. tostring(rf_tg_id_cfg))
    add_defines("ADHOC_TASK_ROLE_GATEWAY=" .. (adhoc_role_cfg == "gw" and "1" or "0"))
    add_defines("ADHOC_TASK_GATEWAY_NO=" .. tostring(adhoc_gw_no_cfg))
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
        if debug_uart_dma_cfg == "true" then
            add_defines("DEBUG=8")           -- USART2 DMA 乒乓缓冲模式
        else
            add_defines("DEBUG=5")           -- USART2 中断模式（默认）
        end
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
    before_build(function (_)
        os.execv("python", {"scripts/build_modules.py"})
    end)

    after_build(function (target)
        import("scripts.post_build", {rootdir = os.scriptdir()})
        post_build.main(target, toolchain_path, cross_prefix)
    end)
