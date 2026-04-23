set_project("ch32v208_zig_c_mixed_tmos")
set_version("0.2.0")

add_rules("mode.debug", "mode.release")
set_config("mode", "debug")

-- Generate `.vscode/compile_commands.json` for clangd / VSCode.
add_rules("plugin.compile_commands.autoupdate", { outputdir = ".vscode" })

-- Target: CH32V208 (RISC-V)
set_plat("cross")
set_arch("riscv")

-- -----------------------------------------------------------------------------
-- Toolchain selection (follow parent projects style)
-- -----------------------------------------------------------------------------

option("wch_gcc_ver")
    set_default("15")
    set_showmenu(true)
    set_values("15", "12", "auto")
    set_description("Select WCH RISC-V GCC toolchain version")
option_end()

option("wch_toolchain_root")
    set_default("E:/APP/MRS2/MounRiver_Studio2/resources/app/resources/win32/components/WCH/Toolchain")
    set_showmenu(true)
    set_description("Root folder that contains 'RISC-V Embedded GCC12/15'")
option_end()

option("log_print")
    set_default("true")
    set_showmenu(true)
    set_values("true", "false")
    set_description("Enable runtime LOG_PRINT outputs (LOG_PRINT_ENABLE)")
option_end()

option("zig_exe")
    set_default("")
    set_showmenu(true)
    set_description("Path to zig.exe (leave empty to auto-detect tools/zig/**/zig.exe or zig on PATH)")
option_end()

local function _pick_wch_toolchain()
    local root = get_config("wch_toolchain_root")
    local ver = get_config("wch_gcc_ver") or "auto"

    local gcc15 = path.join(root, "RISC-V Embedded GCC15")
    local gcc12 = path.join(root, "RISC-V Embedded GCC12")
    local tc = nil
    if ver == "15" then
        tc = gcc15
    elseif ver == "12" then
        tc = gcc12
    else
        tc = os.isdir(gcc15) and gcc15 or gcc12
    end

    local bin = path.join(tc, "bin")
    local prefix = path.join(bin, "riscv32-wch-elf-")
    if not os.isfile(prefix .. "gcc.exe") then
        prefix = path.join(bin, "riscv-wch-elf-")
    end
    return tc, prefix
end

local function _find_zig()
    local cfg = get_config("zig_exe")
    if cfg and #cfg > 0 then
        if os.isfile(cfg) then
            return path.translate(cfg)
        end
        raise("zig_exe not found: " .. cfg)
    end

    local candidates = os.files(path.join(os.projectdir(), "tools", "zig", "**", "zig.exe"))
    if candidates and #candidates > 0 then
        return candidates[1]
    end
    return "zig"
end

-- -----------------------------------------------------------------------------
-- Firmware (C entry + WCH startup) -> calls into Zig library
-- -----------------------------------------------------------------------------

target("firmware")
    set_kind("binary")
    set_extension(".elf")

    local tc, prefix = _pick_wch_toolchain()

    -- Toolchain (C/ASM/LINK) from WCH GCC
    set_toolset("cc",     prefix .. "gcc.exe")
    set_toolset("cxx",    prefix .. "g++.exe")
    set_toolset("as",     prefix .. "gcc.exe")
    set_toolset("ld",     prefix .. "gcc.exe")
    set_toolset("ar",     prefix .. "ar.exe")
    set_toolset("ranlib", prefix .. "ranlib.exe")
    set_toolset("objcopy",prefix .. "objcopy.exe")
    set_toolset("objdump",prefix .. "objdump.exe")
    set_toolset("size",   prefix .. "size.exe")

    -- Be strict about flags: if a flag is invalid, do not auto-ignore it.
    set_policy("check.auto_ignore_flags", false)

    add_files(
        -- Startup + scheduler glue for WCH BLE/TMOS
        "sdk/Startup/startup_ch32v20x_D8W.S",
        "sdk/LIB/ble_task_scheduler.S",

        -- Core / debug / HAL
        "sdk/Core/core_riscv.c",
        "sdk/Debug/debug.c",
        "sdk/HAL/*.c",

        -- On-chip peripheral drivers
        "sdk/Peripheral/src/*.c",

        -- Board + app
        "bsp/*.c",
        "APP/*.c"
    )

    -- CPU/ABI (CH32V208 = rv32imacxw)
    add_cflags(
        "-march=rv32imacxw", "-mabi=ilp32",
        "-msmall-data-limit=8",
        "-msave-restore",
        "-fmessage-length=0",
        "-fsigned-char",
        "-ffunction-sections", "-fdata-sections",
        "-fno-common",
        "-Wall", "-Wextra", "-Wno-unused-parameter",
        "-Wunused", "-Wuninitialized",
        "-fmax-errors=20",
        "-std=gnu17",
        {force = true}
    )

    add_asflags(
        "-march=rv32imacxw", "-mabi=ilp32",
        "-msmall-data-limit=8",
        "-msave-restore",
        "-ffunction-sections", "-fdata-sections",
        "-fno-common",
        "-fmax-errors=20",
        "-fmessage-length=0",
        "-fsigned-char",
        "-g",
        {force = true}
    )

    -- For `.S` files: run through preprocessor
    on_load(function (target)
        target:add("asflags", "-x", "assembler-with-cpp", {force = true})
    end)

    if is_mode("debug") then
        add_cflags("-g", "-O0", {force = true})
        add_defines("DEBUG=5") -- DEBUG_UART2_IT
    else
        add_cflags("-Os", {force = true})
        add_defines("DEBUG=0", "NDEBUG=1")
    end

    add_includedirs(
        "sdk/Debug",
        "sdk/Core",
        "sdk/Peripheral/inc",
        "sdk/HAL/include",
        "sdk/LIB",
        "sdk/Startup",
        "APP/include",
        "bsp"
    )

    -- Chip type
    add_defines("CH32V20x_D8W")

    -- Optional log switch (used by some parent demos; harmless here)
    if tostring(get_config("log_print") or "true") == "false" then
        add_defines("LOG_PRINT_ENABLE=0")
    else
        add_defines("LOG_PRINT_ENABLE=1")
    end

    add_ldflags(
        "-nostartfiles",
        "-T", "sdk/HAL/Link.ld",
        "-L", "sdk/LIB",
        "-Xlinker", "--gc-sections",
        "-Wl,--print-memory-usage",
        "--specs=nano.specs",
        "--specs=nosys.specs",
        {force = true}
    )

    -- Map file (same style as Demo_v0.2)
    on_load(function (target)
        local map_file = path.translate(path.join(target:targetdir(), target:name() .. ".map"))
        target:add("ldflags", "-Wl,-Map," .. map_file, {force = true})
        target:add("asflags", "-I" .. os.projectdir() .. "/sdk/Startup", {force = true})
    end)

    -- Build Zig static lib and link it in (must appear before wchble so symbols resolve)
    local builddir = get_config("builddir") or "build"
    local zig_outdir = path.join(builddir, "zig", get_config("mode") or "debug")
    add_linkdirs(zig_outdir)
    add_links("zig_app", "wchble", "m", "c")

    before_build(function (target)
        local zig = _find_zig()
        os.mkdir(zig_outdir)

        local libpath = path.join(zig_outdir, "libzig_app.a")
        local hdrpath = path.join(zig_outdir, "zig_app.h")
        local cache_dir = path.join(zig_outdir, "cache")
        local global_cache_dir = path.join(zig_outdir, "gcache")
        os.mkdir(cache_dir)
        os.mkdir(global_cache_dir)

        local optimize = "ReleaseSmall"
        if is_mode("debug") then
            optimize = "Debug"
        end

        local argv = {
            "build-lib", path.join(os.projectdir(), "zig", "app.zig"),
            "-target", "riscv32-freestanding-none",
            "-mcpu=generic_rv32+m+a+c",
            "-O", optimize,
            "--cache-dir", cache_dir,
            "--global-cache-dir", global_cache_dir,
            "-static",
            "-femit-bin=" .. libpath,
            "-femit-h=" .. hdrpath
        }
        os.execv(zig, argv)
    end)

    on_clean(function (target)
        if os.isdir(zig_outdir) then
            os.rm(zig_outdir)
        end
    end)

    after_build(function (target)
        -- Emit BIN (convenient for some flashing tools), then run Demo_v0.2-style post build.
        local elf_file = target:targetfile()
        local output_dir = path.directory(elf_file)
        local bin_file = path.join(output_dir, target:name() .. ".bin")
        os.execv(target:tool("objcopy"), {"-O", "binary", elf_file, bin_file})

        local post_build = import("scripts.post_build", {rootdir = os.scriptdir()})
        post_build.main(target, tc, prefix)
    end)
