set_project("CH32V208GBU_cherryusb")
set_version("0.1.0")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})

set_plat("cross")
set_arch("riscv")
set_allowedplats("cross")
set_allowedarchs("riscv")
set_languages("gnu17")

option("wch_gcc_ver")
    set_default("auto")
    set_showmenu(true)
    set_values("auto", "12", "15")
    set_description("Select WCH RISC-V GCC toolchain version")
option_end()

option("wch_toolchain_root")
    set_default("E:/APP/MRS2/MounRiver_Studio2/resources/app/resources/win32/components/WCH/Toolchain")
    set_showmenu(true)
    set_description("WCH toolchain root directory")
option_end()

local function detect_wch_toolchain()
    local root = get_config("wch_toolchain_root")
    local selected_ver = get_config("wch_gcc_ver") or "auto"
    local bins = {}

    local function add_bin(ver)
        table.insert(bins, path.join(root, "RISC-V Embedded GCC" .. ver, "bin"))
    end

    if selected_ver == "12" then
        add_bin("12")
    elseif selected_ver == "15" then
        add_bin("15")
    else
        add_bin("12")
        add_bin("15")
    end

    for _, bin in ipairs(bins) do
        for _, prefix in ipairs({"riscv-wch-elf-", "riscv32-wch-elf-"}) do
            local gcc = path.join(bin, prefix .. "gcc.exe")
            if os.isfile(gcc) then
                return bin, prefix
            end
        end
    end

    raise("Cannot find WCH GCC toolchain in: %s", root)
end

target("CH32V208GBU")
    set_kind("binary")
    set_extension(".elf")
    set_filename("CH32V208GBU.elf")
    set_targetdir("build/$(plat)/$(arch)/$(mode)")

    on_load(function (target)
        local bin, prefix = detect_wch_toolchain()

        target:data_set("wch_bin", bin)
        target:data_set("wch_prefix", prefix)

        target:set("toolset", "cc", path.join(bin, prefix .. "gcc.exe"))
        target:set("toolset", "as", path.join(bin, prefix .. "gcc.exe"))
        target:set("toolset", "ld", path.join(bin, prefix .. "gcc.exe"))
        target:set("toolset", "ar", path.join(bin, prefix .. "ar.exe"))
        target:set("toolset", "ranlib", path.join(bin, prefix .. "ranlib.exe"))

        local mapfile = path.join(target:targetdir(), "CH32V208GBU.map")
        target:add("ldflags", "-Wl,-Map," .. mapfile, {force = true})
    end)

    add_files(
        "User/Main.c",
        "User/ch32v20x_it.c",
        "User/system_ch32v20x.c",
        "User/usb_cdc_app.c",

        "sdk/Core/core_riscv.c",
        "sdk/Debug/debug.c",
        "sdk/Peripheral/src/*.c",
        "sdk/Startup/startup_ch32v20x_D8W.S",

        "cherryUSB/core/usbd_core.c",
        "cherryUSB/class/cdc/usbd_cdc_acm.c",
        "cherryUSB/port/fsdev/usb_dc_fsdev.c"
    )

    add_includedirs(
        "sdk/Core",
        "sdk/Debug",
        "sdk/Peripheral/inc",
        "User",
        "cherryUSB",
        "cherryUSB/class/cdc",
        "cherryUSB/common",
        "cherryUSB/core",
        "cherryUSB/port/fsdev"
    )

    add_defines("CH32V20x_D8W", "DEBUG=2", "USB_BASE=0x40005C00")

    add_cflags(
        "-march=rv32imacxw",
        "-mabi=ilp32",
        "-msmall-data-limit=8",
        "-msave-restore",
        "-fmessage-length=0",
        "-fsigned-char",
        "-ffunction-sections",
        "-fdata-sections",
        "-fno-common",
        "-Wunused",
        "-Wuninitialized",
        "-fmax-errors=20",
        {force = true}
    )

    add_asflags(
        "-x", "assembler-with-cpp",
        "-march=rv32imacxw",
        "-mabi=ilp32",
        "-msmall-data-limit=8",
        "-msave-restore",
        "-fmessage-length=0",
        "-fsigned-char",
        "-ffunction-sections",
        "-fdata-sections",
        "-fno-common",
        "-fmax-errors=20",
        "-I" .. path.join(os.scriptdir(), "sdk/Startup"),
        {force = true}
    )

    add_ldflags(
        "-march=rv32imacxw",
        "-mabi=ilp32",
        "-msmall-data-limit=8",
        "-msave-restore",
        "-fmessage-length=0",
        "-fsigned-char",
        "-ffunction-sections",
        "-fdata-sections",
        "-fno-common",
        "-fmax-errors=20",
        "-T", "sdk/Ld/Link.ld",
        "-nostartfiles",
        "-Wl,--gc-sections",
        "--specs=nano.specs",
        "--specs=nosys.specs",
        {force = true}
    )

    if is_mode("debug") then
        add_cflags("-g", "-O0", {force = true})
        add_asflags("-g", {force = true})
    else
        add_cflags("-Os", {force = true})
    end

    after_build(function (target)
        local bin = target:data("wch_bin")
        local prefix = target:data("wch_prefix")
        local objcopy = path.join(bin, prefix .. "objcopy.exe")
        local objdump = path.join(bin, prefix .. "objdump.exe")
        local size = path.join(bin, prefix .. "size.exe")
        local elf = target:targetfile()
        local outdir = target:targetdir()
        local hex = path.join(outdir, "CH32V208GBU.hex")
        local lst = path.join(outdir, "CH32V208GBU.lst")

        os.exec("\"%s\" -O ihex \"%s\" \"%s\"", objcopy, elf, hex)
        local dump = os.iorunv(objdump, {"--all-headers", "--demangle", "--disassemble", "-M", "xw", elf})
        io.writefile(lst, dump)
        os.exec("\"%s\" --format=berkeley \"%s\"", size, elf)
    end)
