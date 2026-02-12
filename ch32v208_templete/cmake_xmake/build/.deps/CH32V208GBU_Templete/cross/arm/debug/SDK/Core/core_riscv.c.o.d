{
    files = {
        [[SDK\Core\core_riscv.c]]
    },
    depfiles_format = "gcc",
    depfiles = "core_riscv.o: SDK\\Core\\core_riscv.c\
",
    values = {
        "e:/APP/MRS2/MounRiver_Studio2/resources/app/resources/win32/components/WCH/Toolchain/RISC-V Embedded GCC12/bin/riscv-wch-elf-gcc",
        {
            [[-ISDK\Debug]],
            [[-ISDK\Core]],
            [[-IAPP\include]],
            [[-ISDK\Peripheral\inc]],
            [[-IHAL\include]],
            "-ILIB",
            [[-IProfile\include]],
            [[-ISDK\Startup]],
            "-DDEBUG=1",
            "-DCH32V20x_D8W",
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
            "-std=gnu17",
            "-O0",
            [[-fmacro-prefix-map=D:\Desktop\ch32\0.CH32V208_dome\ch32v208_templete\Cmake=..]]
        }
    }
}