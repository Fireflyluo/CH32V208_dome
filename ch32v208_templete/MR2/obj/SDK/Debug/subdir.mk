################################################################################
# MRS Version: 2.3.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../SDK/Debug/debug.c 

C_DEPS += \
./SDK/Debug/debug.d 

OBJS += \
./SDK/Debug/debug.o 

DIR_OBJS += \
./SDK/Debug/*.o \

DIR_DEPS += \
./SDK/Debug/*.d \

DIR_EXPANDS += \
./SDK/Debug/*.253r.expand \


# Each subdirectory must supply rules for building sources it contributes
SDK/Debug/%.o: ../SDK/Debug/%.c
	@	riscv-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -DCH32V20x_D8W -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/SDK/Debug" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/SDK/Core" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/APP/include" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/SDK/Peripheral/inc" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/HAL/include" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/LIB" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/Profile/include" -std=gnu17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

