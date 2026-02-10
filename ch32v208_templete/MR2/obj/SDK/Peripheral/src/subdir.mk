################################################################################
# MRS Version: 2.3.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../SDK/Peripheral/src/ch32v20x_adc.c \
../SDK/Peripheral/src/ch32v20x_bkp.c \
../SDK/Peripheral/src/ch32v20x_can.c \
../SDK/Peripheral/src/ch32v20x_crc.c \
../SDK/Peripheral/src/ch32v20x_dbgmcu.c \
../SDK/Peripheral/src/ch32v20x_dma.c \
../SDK/Peripheral/src/ch32v20x_exti.c \
../SDK/Peripheral/src/ch32v20x_flash.c \
../SDK/Peripheral/src/ch32v20x_gpio.c \
../SDK/Peripheral/src/ch32v20x_i2c.c \
../SDK/Peripheral/src/ch32v20x_iwdg.c \
../SDK/Peripheral/src/ch32v20x_misc.c \
../SDK/Peripheral/src/ch32v20x_opa.c \
../SDK/Peripheral/src/ch32v20x_pwr.c \
../SDK/Peripheral/src/ch32v20x_rcc.c \
../SDK/Peripheral/src/ch32v20x_rtc.c \
../SDK/Peripheral/src/ch32v20x_spi.c \
../SDK/Peripheral/src/ch32v20x_tim.c \
../SDK/Peripheral/src/ch32v20x_usart.c \
../SDK/Peripheral/src/ch32v20x_wwdg.c 

C_DEPS += \
./SDK/Peripheral/src/ch32v20x_adc.d \
./SDK/Peripheral/src/ch32v20x_bkp.d \
./SDK/Peripheral/src/ch32v20x_can.d \
./SDK/Peripheral/src/ch32v20x_crc.d \
./SDK/Peripheral/src/ch32v20x_dbgmcu.d \
./SDK/Peripheral/src/ch32v20x_dma.d \
./SDK/Peripheral/src/ch32v20x_exti.d \
./SDK/Peripheral/src/ch32v20x_flash.d \
./SDK/Peripheral/src/ch32v20x_gpio.d \
./SDK/Peripheral/src/ch32v20x_i2c.d \
./SDK/Peripheral/src/ch32v20x_iwdg.d \
./SDK/Peripheral/src/ch32v20x_misc.d \
./SDK/Peripheral/src/ch32v20x_opa.d \
./SDK/Peripheral/src/ch32v20x_pwr.d \
./SDK/Peripheral/src/ch32v20x_rcc.d \
./SDK/Peripheral/src/ch32v20x_rtc.d \
./SDK/Peripheral/src/ch32v20x_spi.d \
./SDK/Peripheral/src/ch32v20x_tim.d \
./SDK/Peripheral/src/ch32v20x_usart.d \
./SDK/Peripheral/src/ch32v20x_wwdg.d 

OBJS += \
./SDK/Peripheral/src/ch32v20x_adc.o \
./SDK/Peripheral/src/ch32v20x_bkp.o \
./SDK/Peripheral/src/ch32v20x_can.o \
./SDK/Peripheral/src/ch32v20x_crc.o \
./SDK/Peripheral/src/ch32v20x_dbgmcu.o \
./SDK/Peripheral/src/ch32v20x_dma.o \
./SDK/Peripheral/src/ch32v20x_exti.o \
./SDK/Peripheral/src/ch32v20x_flash.o \
./SDK/Peripheral/src/ch32v20x_gpio.o \
./SDK/Peripheral/src/ch32v20x_i2c.o \
./SDK/Peripheral/src/ch32v20x_iwdg.o \
./SDK/Peripheral/src/ch32v20x_misc.o \
./SDK/Peripheral/src/ch32v20x_opa.o \
./SDK/Peripheral/src/ch32v20x_pwr.o \
./SDK/Peripheral/src/ch32v20x_rcc.o \
./SDK/Peripheral/src/ch32v20x_rtc.o \
./SDK/Peripheral/src/ch32v20x_spi.o \
./SDK/Peripheral/src/ch32v20x_tim.o \
./SDK/Peripheral/src/ch32v20x_usart.o \
./SDK/Peripheral/src/ch32v20x_wwdg.o 

DIR_OBJS += \
./SDK/Peripheral/src/*.o \

DIR_DEPS += \
./SDK/Peripheral/src/*.d \

DIR_EXPANDS += \
./SDK/Peripheral/src/*.253r.expand \


# Each subdirectory must supply rules for building sources it contributes
SDK/Peripheral/src/%.o: ../SDK/Peripheral/src/%.c
	@	riscv-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -DCH32V20x_D8W -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/SDK/Debug" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/SDK/Core" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/APP/include" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/SDK/Peripheral/inc" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/HAL/include" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/LIB" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/Profile/include" -std=gnu17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

