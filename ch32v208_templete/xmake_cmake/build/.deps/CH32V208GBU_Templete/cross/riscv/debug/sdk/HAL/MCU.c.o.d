{
    files = {
        [[sdk\HAL\MCU.c]]
    },
    depfiles_format = "gcc",
    depfiles = "MCU.o: sdk\\HAL\\MCU.c sdk\\HAL\\include/HAL.h sdk\\HAL\\include/config.h  sdk\\LIB/wchble.H sdk\\Peripheral\\inc/ch32v20x.h sdk\\Core/core_riscv.h  APP\\include/system_ch32v20x.h APP\\include/ch32v20x_conf.h  sdk\\Peripheral\\inc/ch32v20x_adc.h sdk\\Peripheral\\inc/ch32v20x.h  sdk\\Peripheral\\inc/ch32v20x_bkp.h sdk\\Peripheral\\inc/ch32v20x_can.h  sdk\\Peripheral\\inc/ch32v20x_crc.h sdk\\Peripheral\\inc/ch32v20x_dbgmcu.h  sdk\\Peripheral\\inc/ch32v20x_dma.h sdk\\Peripheral\\inc/ch32v20x_exti.h  sdk\\Peripheral\\inc/ch32v20x_flash.h sdk\\Peripheral\\inc/ch32v20x_gpio.h  sdk\\Peripheral\\inc/ch32v20x_i2c.h sdk\\Peripheral\\inc/ch32v20x_iwdg.h  sdk\\Peripheral\\inc/ch32v20x_pwr.h sdk\\Peripheral\\inc/ch32v20x_rcc.h  sdk\\Peripheral\\inc/ch32v20x_rtc.h sdk\\Peripheral\\inc/ch32v20x_spi.h  sdk\\Peripheral\\inc/ch32v20x_tim.h sdk\\Peripheral\\inc/ch32v20x_usart.h  sdk\\Peripheral\\inc/ch32v20x_wwdg.h APP\\include/ch32v20x_it.h  sdk\\Debug/debug.h sdk\\Peripheral\\inc/ch32v20x_misc.h  sdk\\HAL\\include/RTC.h sdk\\HAL\\include/SLEEP.h sdk\\HAL\\include/KEY.h  sdk\\HAL\\include/LED.h\
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