-- CH32V208GBU_Templete xmake build file
-- RISC-V RV32IMACXW Architecture

-- 设置项目名称
set_project("CH32V208GBU_Templete")

-- 添加构建模式规则
add_rules("mode.debug", "mode.release")

-- 设置默认构建模式为 debug
set_config("mode", "debug")

-- 设置工具链路径
local toolchain_path = "e:/APP/MRS2/MounRiver_Studio2/resources/app/resources/win32/components/WCH/Toolchain/RISC-V Embedded GCC12"
local toolchain_bin = toolchain_path .. "/bin"

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
    
    -- 设置工具链
    set_toolchains("cross", {cross = "riscv-wch-elf-"})
    
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
        "lib/sht40/sht40_hal.c",
        "lib/sc7a20htr/platform/platform.c",
        "lib/sc7a20htr/example/async_example.c",
        "lib/sc7a20htr/example/simple_loop_example.c",
        "lib/sc7a20htr/sc7a20_core.c",
        "lib/sc7a20htr/sc7a20_core_async.c",
        "lib/oled/OLED.c",
        "lib/oled/OLED_Data.c",
        "bsp/UART/UART.c",
        "bsp/board.c",
        "bsp/drv_gpio.c",
        "bsp/drv_i2c.c",
        "bsp/usb_cdc.c",
        "ble_profile/devinfoservice.c",
        "ble_profile/gattprofile.c",
        "app/ch32v20x_it.c",
        "app/main.c",
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
        "bsp/include",
        "bsp/UART",
        "sdk/USBLIB/CONFIG",
        "sdk/USBLIB/USB-Driver/inc",
        "utils",
        "lib/sht40",
        "lib/oled",
        "lib/sc7a20htr",
        "lib/sc7a20htr/inc",
        "lib/sc7a20htr/platform"
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
        add_defines("NDEBUG=5")
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
        target:add("ldflags", "-Wl,-Map," .. target:name() .. ".map", {force = true})
    end)
    
    -- 库文件
    add_linkdirs("sdk/LIB")
    add_links("wchble", "m", "c")
    
    -- 设置输出文件名
    set_filename("CH32V208GBU_Templete.elf")
    
    -- 构建后处理 - 调用外部脚本
    after_build(function (target)
        import("scripts.post_build", {rootdir = os.scriptdir()})
        post_build.main(target, toolchain_path)
    end)
