-- CH32V208GBU_Templete xmake build file
-- RISC-V RV32IMACXW Architecture

-- 设置项目名称
set_project("CH32V208GBU_Templete")

-- 添加构建模式规则
add_rules("mode.debug", "mode.release")

-- 设置默认构建模式为 debug
set_config("mode", "debug")

-- 工具链可手动切换:
--   xmake f --wch_gcc_ver=15   (默认)
--   xmake f --wch_gcc_ver=12
--   xmake f --wch_gcc_ver=auto (优先15, 回退12)
option("wch_gcc_ver")
    set_default("15")
    set_showmenu(true)
    set_values("15", "12", "auto")
    set_description("Select WCH RISC-V GCC toolchain version")
option_end()

local toolchain_root = "e:/APP/MRS2/MounRiver_Studio2/resources/app/resources/win32/components/WCH/Toolchain"
local selected_ver = get_config("wch_gcc_ver") or "15"
local toolchain_path = nil

if selected_ver == "12" then
    toolchain_path = toolchain_root .. "/RISC-V Embedded GCC12"
elseif selected_ver == "15" then
    toolchain_path = toolchain_root .. "/RISC-V Embedded GCC15"
else
    local gcc15_path = toolchain_root .. "/RISC-V Embedded GCC15"
    local gcc12_path = toolchain_root .. "/RISC-V Embedded GCC12"
    if os.isdir(gcc15_path) then
        toolchain_path = gcc15_path
    else
        toolchain_path = gcc12_path
    end
end

local toolchain_bin = toolchain_path .. "/bin"
local cross_prefix = toolchain_bin .. "/riscv32-wch-elf-"
if not os.isfile(cross_prefix .. "gcc.exe") then
    cross_prefix = toolchain_bin .. "/riscv-wch-elf-"
end

-- 设置平台和架构
set_plat("cross")
set_arch("riscv")

-- 设置SDK路径
set_config("sdk", toolchain_path)

-- 自动生成 VSCode 的 compile_commands.json 文件
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})

-- 目标配置
target("CH32V208GBU_Templete")
    set_kind("binary")
    set_extension(".elf")
    
    -- 显式指定工具，避免 xmake 内置 cross 检查在部分环境下误判
    set_toolset("cc", cross_prefix .. "gcc.exe")
    set_toolset("cxx", cross_prefix .. "g++.exe")
    set_toolset("as", cross_prefix .. "gcc.exe")
    set_toolset("ld", cross_prefix .. "gcc.exe")
    set_toolset("ar", cross_prefix .. "ar.exe")
    set_toolset("ranlib", cross_prefix .. "ranlib.exe")
    
    -- 设置架构和ABI (RISC-V RV32IMACXW)
    add_cflags("-march=rv32imacxw", "-mabi=ilp32", {force = true})
    add_asflags("-march=rv32imacxw", "-mabi=ilp32", {force = true})
    add_ldflags("-march=rv32imacxw", "-mabi=ilp32", {force = true})
    
    -- 源文件
    add_files(
        "app/tasks/tmos_led_task.c",
        "sdk/USBLIB/USB-Driver/src/usb_core.c",
        "sdk/USBLIB/USB-Driver/src/usb_init.c",
        "sdk/USBLIB/USB-Driver/src/usb_int.c",
        "sdk/USBLIB/USB-Driver/src/usb_mem.c",
        "sdk/USBLIB/USB-Driver/src/usb_regs.c",
        "sdk/USBLIB/USB-Driver/src/usb_sil.c",
        "sdk/USBLIB/CONFIG/hw_config.c",
        "sdk/USBLIB/CONFIG/usb_desc.c",
        "sdk/USBLIB/CONFIG/usb_endp.c",
        "sdk/USBLIB/CONFIG/usb_istr.c",
        "sdk/USBLIB/CONFIG/usb_prop.c",
        "sdk/USBLIB/CONFIG/usb_pwr.c",
        "sdk/Startup/startup_ch32v20x_D8W.S",
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
        "sdk/LIB/ble_task_scheduler.S",
        "sdk/HAL/KEY.c",
        "sdk/HAL/LED.c",
        "sdk/HAL/MCU.c",
        "sdk/HAL/RTC.c",
        "sdk/HAL/SLEEP.c",
        "sdk/Debug/debug.c",
        "sdk/Core/core_riscv.c",
        "utils/ringbuffer.c",
        "utils/sw_timer.c",
        "lib/sht40/adapters/sht40_ch32_adapter.c",
        "lib/sht40/src/sht40_core.c",
        "lib/sht40/src/sht40_sync.c",
        "lib/sht40/src/sht40_async.c",
        "lib/sc7a20/adapters/sc7a20_ch32_adapter.c",
        "lib/sc7a20/src/sc7a20_core.c",
        "lib/sc7a20/src/sc7a20_sync.c",
        "lib/sc7a20/src/sc7a20_async.c",
        "lib/oled/OLED.c",
        "lib/oled/OLED_Data.c",
        "bsp/board.c",
        "bsp/drivers/src/drv_gpio.c",
        "bsp/drivers/src/drv_i2c.c",
        "bsp/drivers/src/drv_tim.c",
        "bsp/bus/src/i2c_bus_arbiter.c",
        "bsp/usb_cdc.c",
        "ble_profile/devinfoservice.c",
        "ble_profile/gattprofile.c",
        "app/ch32v20x_it.c",
        "app/main.c",
        "test/main_runtime_test.c",
        "app/peripheral.c",
        "app/system_ch32v20x.c"
    )
    
    -- 包含路径
    add_includedirs(
        "sdk/Debug",
        "sdk/Core",
        "app/include",
        "sdk/Peripheral/inc",
        "sdk/HAL/include",
        "sdk/LIB",
        "ble_profile/include",
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
        "test"
    )
    
    -- 汇编包含路径
    add_asflags("-I" .. os.scriptdir() .. "/sdk/Startup", {force = true})
    
    -- 宏定义
    add_defines("CH32V20x_D8W")
    
    -- 通用编译选项
    add_cflags(
        "-msmall-data-limit=8",
        "-msave-restore",
        "-fmessage-length=0",
        "-fsigned-char",
        "-ffunction-sections",
        "-fdata-sections",
        "-fno-common",
        "-Wall",
        "-Wextra",
        "-Wno-unused-parameter",
        "-Wunused",
        "-Wuninitialized",
        "-fmax-errors=20",
        "-std=gnu17",
        {force = true}
    )
    
    -- 调试/发布模式选项
    if is_mode("debug") then
        add_cflags("-g", "-O0", {force = true})
        add_defines("DEBUG=5")
    else
        add_cflags("-Os", {force = true})
    end
    
    -- 汇编标志 (避免 -MMD 标志问题)
    set_policy("check.auto_ignore_flags", false)
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
        "-x", "assembler-with-cpp",
        {force = true}
    )
    
    -- C++ 标志
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
        "-fabi-version=0",
        {force = true}
    )
    
    -- 链接选项
    add_ldflags(
        "-T", "sdk/HAL/Link.ld",
        "-L", "sdk/LIB",
        "-nostartfiles",
        "-Wl,--gc-sections",
        "-Wl,--print-memory-usage",
        "--specs=nano.specs",
        "--specs=nosys.specs",
        {force = true}
    )
    
    -- 在 on_load 中设置需要目标名称的链接选项
    on_load(function (target)
        local map_file = path.translate(path.join(target:targetdir(), target:name() .. ".map"))
        target:add("ldflags", "-Wl,-Map," .. map_file, {force = true})
    end)
    
    -- 库文件
    add_linkdirs("sdk/LIB")
    add_links("wchble", "m", "c")
    
    -- 设置输出文件名
    set_filename("CH32V208GBU_Templete.elf")
    
    -- 构建后处理 - 调用外部脚本
    after_build(function (target)
        import("scripts.post_build", {rootdir = os.scriptdir()})
        post_build.main(target, toolchain_path, cross_prefix)
    end)
