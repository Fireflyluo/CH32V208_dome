-- CH32V208 
set_project("CH32V208GBU_Templete")
set_version("1.0.0")

add_rules("mode.debug", "mode.release")

-- 设置交叉编译工具链和目标平台
set_plat("cross")
set_arch("arm")

-- 工具链路径
local TOOLCHAIN_FOLDER = "e:/APP/MRS2/MounRiver_Studio2/resources/app/resources/win32/components/WCH/Toolchain/RISC-V Embedded GCC12"
local TOOLCHAIN_PREFIX = "riscv-wch-elf-"

-- 设置工具
set_toolset("cc", TOOLCHAIN_FOLDER .. "/bin/riscv-wch-elf-gcc")
set_toolset("cxx", TOOLCHAIN_FOLDER .. "/bin/riscv-wch-elf-g++")
set_toolset("as", TOOLCHAIN_FOLDER .. "/bin/riscv-wch-elf-gcc")
set_toolset("ld", TOOLCHAIN_FOLDER .. "/bin/riscv-wch-elf-gcc")
set_toolset("ar", TOOLCHAIN_FOLDER .. "/bin/riscv-wch-elf-ar")
set_toolset("objcopy", TOOLCHAIN_FOLDER .. "/bin/riscv-wch-elf-objcopy")
set_toolset("objdump", TOOLCHAIN_FOLDER .. "/bin/riscv-wch-elf-objdump")
set_toolset("size", TOOLCHAIN_FOLDER .. "/bin/riscv-wch-elf-size")




-- 目标设置
target("CH32V208GBU_Templete")
    set_kind("binary")
    set_extension(".elf")
    
    -- 编译选项
    add_cflags(
        "-march=rv32imacxw",
        "-mabi=ilp32",
        "-msmall-data-limit=8",
        "-msave-restore",
        "-fmax-errors=20",
        "-Os",
        "-fmessage-length=0",
        "-fsigned-char",
        "-ffunction-sections",
        "-fdata-sections",
        "-fno-common",
        "-Wunused",
        "-Wuninitialized",
        "-g",
        "-std=gnu17"
    )
    
    -- C++选项
    add_cxxflags(
        "-march=rv32imacxw",
        "-mabi=ilp32",
        "-msmall-data-limit=8",
        "-msave-restore",
        "-fmax-errors=20",
        "-Os",
        "-fmessage-length=0",
        "-fsigned-char",
        "-ffunction-sections",
        "-fdata-sections",
        "-fno-common",
        "-Wunused",
        "-Wuninitialized",
        "-g",
        "-std=gnu++11",
        "-fabi-version=0"
    )
    
    -- ASM_FLAGS
    add_asflags(
        "-march=rv32imacxw",
        "-mabi=ilp32",
        "-msmall-data-limit=8",
        "-msave-restore",
        "-fmax-errors=20",
        "-Os",
        "-fmessage-length=0",
        "-fsigned-char",
        "-ffunction-sections",
        "-fdata-sections",
        "-fno-common",
        "-Wunused",
        "-Wuninitialized",
        "-g"
    )
    
    -- 为汇编文件添加 -x assembler-with-cpp
    on_load(function (target)
        target:add("asflags", "-x", "assembler-with-cpp", {force = true})
    end)
    

    -- 调试选项
    if is_mode("debug") then
        add_cflags("-g", "-O0", {force = true})
        add_defines("DEBUG=1")
    else
        add_cflags("-Os", {force = true})
        add_defines("NDEBUG=1")
    end

    -- 链接标志
    add_ldflags(
        "-march=rv32imacxw",
        "-mabi=ilp32",
        "-msmall-data-limit=8",
        "-msave-restore",
        "-fmax-errors=20",
        "-Os",
        "-fmessage-length=0",
        "-fsigned-char",
        "-ffunction-sections",
        "-fdata-sections",
        "-fno-common",
        "-Wunused",
        "-Wuninitialized",
        "-g",
        "-T", "./HAL/Link.ld",
        "-L./LIB",
        "-nostartfiles",
        "-Xlinker", "--gc-sections",
        "-Wl,-Map,CH32V208GBU_Templete.map",
        "--specs=nano.specs",
        "--specs=nosys.specs"
    )
    
    -- 包含路径
    add_includedirs(
        "SDK/Debug",
        "SDK/Core",
        "APP/include",
        "SDK/Peripheral/inc",
        "HAL/include",
        "LIB",
        "Profile/include",
        "SDK/Startup"
    )
    
    -- 宏定义
    add_defines("CH32V20x_D8W")
    
    -- 添加 -fmacro-prefix-map
    on_load(function (target)
        local source_dir = os.projectdir()
        target:add("cxflags", "-fmacro-prefix-map=" .. source_dir .. "=..", {force = true})
    end)
    
    -- 添加源文件列表
    add_files(
        "SDK/Startup/startup_ch32v20x_D8W.S",
        "SDK/Peripheral/src/ch32v20x_adc.c",
        "SDK/Peripheral/src/ch32v20x_bkp.c",
        "SDK/Peripheral/src/ch32v20x_can.c",
        "SDK/Peripheral/src/ch32v20x_crc.c",
        "SDK/Peripheral/src/ch32v20x_dbgmcu.c",
        "SDK/Peripheral/src/ch32v20x_dma.c",
        "SDK/Peripheral/src/ch32v20x_exti.c",
        "SDK/Peripheral/src/ch32v20x_flash.c",
        "SDK/Peripheral/src/ch32v20x_gpio.c",
        "SDK/Peripheral/src/ch32v20x_i2c.c",
        "SDK/Peripheral/src/ch32v20x_iwdg.c",
        "SDK/Peripheral/src/ch32v20x_misc.c",
        "SDK/Peripheral/src/ch32v20x_opa.c",
        "SDK/Peripheral/src/ch32v20x_pwr.c",
        "SDK/Peripheral/src/ch32v20x_rcc.c",
        "SDK/Peripheral/src/ch32v20x_rtc.c",
        "SDK/Peripheral/src/ch32v20x_spi.c",
        "SDK/Peripheral/src/ch32v20x_tim.c",
        "SDK/Peripheral/src/ch32v20x_usart.c",
        "SDK/Peripheral/src/ch32v20x_wwdg.c",
        "SDK/Debug/debug.c",
        "SDK/Core/core_riscv.c",
        "Profile/devinfoservice.c",
        "Profile/gattprofile.c",
        "LIB/ble_task_scheduler.S",
        "HAL/KEY.c",
        "HAL/LED.c",
        "HAL/MCU.c",
        "HAL/RTC.c",
        "HAL/SLEEP.c",
        "APP/ch32v20x_it.c",
        "APP/peripheral.c",
        "APP/peripheral_main.c",
        "APP/system_ch32v20x.c"
    )
    
    -- 链接库
    add_links("wchble")

    -- 编译后操作，输出到 build 目录
    after_build(function (target)
        local build_dir = target:targetdir()  -- 获取构建目录
        local elf_file = target:targetfile()  -- 获取 elf 文件路径
        local hex_file = path.join(build_dir, "CH32V208GBU_Templete.hex")
        local lst_file = path.join(build_dir, "CH32V208GBU_Templete.lst")
        
        print("生成文件到: " .. build_dir)


        
        -- 生成 hex 文件
        os.exec("%s -O ihex %s %s", 
            TOOLCHAIN_FOLDER .. "/bin/riscv-wch-elf-objcopy", 
            elf_file, 
            hex_file
        )
        -- 显示大小信息
        os.exec("%s --format=berkeley %s", 
            TOOLCHAIN_FOLDER .. "/bin/riscv-wch-elf-size", 
            elf_file
        )
    end)