{
    files = {
        [[sdk\Startup\startup_ch32v20x_D8W.S]]
    },
    depfiles_format = "gcc",
    depfiles = "build\\.objs\\CH32V208GBU_Templete\\cross\\riscv\\debug\\sdk\\Startup\\startup_ch32v20x_D8W.S.o:  sdk\\Startup\\startup_ch32v20x_D8W.S\
",
    values = {
        "E:/APP/MRS2/MounRiver_Studio2/resources/app/resources/win32/components/WCH/Toolchain/RISC-V Embedded GCC12/bin/riscv-wch-elf-gcc",
        {
            "-O0",
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
            "-g"
        }
    }
}