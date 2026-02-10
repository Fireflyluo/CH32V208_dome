################################################################################
# MRS Version: 2.3.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../APP/ch32v20x_it.c \
../APP/peripheral.c \
../APP/peripheral_main.c \
../APP/system_ch32v20x.c 

C_DEPS += \
./APP/ch32v20x_it.d \
./APP/peripheral.d \
./APP/peripheral_main.d \
./APP/system_ch32v20x.d 

OBJS += \
./APP/ch32v20x_it.o \
./APP/peripheral.o \
./APP/peripheral_main.o \
./APP/system_ch32v20x.o 

DIR_OBJS += \
./APP/*.o \

DIR_DEPS += \
./APP/*.d \

DIR_EXPANDS += \
./APP/*.253r.expand \


# Each subdirectory must supply rules for building sources it contributes
APP/%.o: ../APP/%.c
	@	riscv-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -DCH32V20x_D8W -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/SDK/Debug" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/SDK/Core" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/APP/include" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/SDK/Peripheral/inc" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/HAL/include" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/LIB" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/Profile/include" -std=gnu17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

