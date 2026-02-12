{
    files = {
        [[sdk\Core\core_riscv.c]]
    },
    depfiles_format = "gcc",
    depfiles = "core_riscv.o: sdk\\Core\\core_riscv.c\
",
    values = {
        "E:/APP/MRS2/MounRiver_Studio2/resources/app/resources/win32/components/WCH/Toolchain/RISC-V Embedded GCC12/bin/riscv-wch-elf-gcc",
        {
            [[-Isdk\Debug]],
            [[-Isdk\Core]],
            [[-Isdk\Startup]],
            [[-Isdk\Peripheral\inc]],
            [[-Isdk\HAL\include]],
            [[-Isdk\LIB]],
            [[-Ibsp\include]],
            [[-IAPP\include]],
            [[-IProfile\include]],
            [[-Itask\inc]],
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
            [[-fmacro-prefix-map=D:\Desktop\ch32\0.CH32V208_dome\ch32v208_templete\xmake_cmake=..]]
        }
    }
}