{
    files = {
        [[SDK\Peripheral\src\ch32v20x_iwdg.c]]
    },
    depfiles_format = "gcc",
    depfiles = "ch32v20x_iwdg.o: SDK\\Peripheral\\src\\ch32v20x_iwdg.c  SDK\\Peripheral\\inc/ch32v20x_iwdg.h SDK\\Peripheral\\inc/ch32v20x.h  SDK\\Core/core_riscv.h APP\\include/system_ch32v20x.h  APP\\include/ch32v20x_conf.h SDK\\Peripheral\\inc/ch32v20x_adc.h  SDK\\Peripheral\\inc/ch32v20x_bkp.h SDK\\Peripheral\\inc/ch32v20x_can.h  SDK\\Peripheral\\inc/ch32v20x_crc.h SDK\\Peripheral\\inc/ch32v20x_dbgmcu.h  SDK\\Peripheral\\inc/ch32v20x_dma.h SDK\\Peripheral\\inc/ch32v20x_exti.h  SDK\\Peripheral\\inc/ch32v20x_flash.h SDK\\Peripheral\\inc/ch32v20x_gpio.h  SDK\\Peripheral\\inc/ch32v20x_i2c.h SDK\\Peripheral\\inc/ch32v20x_pwr.h  SDK\\Peripheral\\inc/ch32v20x_rcc.h SDK\\Peripheral\\inc/ch32v20x_rtc.h  SDK\\Peripheral\\inc/ch32v20x_spi.h SDK\\Peripheral\\inc/ch32v20x_tim.h  SDK\\Peripheral\\inc/ch32v20x_usart.h SDK\\Peripheral\\inc/ch32v20x_wwdg.h  APP\\include/ch32v20x_it.h SDK\\Debug/debug.h  SDK\\Peripheral\\inc/ch32v20x.h SDK\\Peripheral\\inc/ch32v20x_misc.h\
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