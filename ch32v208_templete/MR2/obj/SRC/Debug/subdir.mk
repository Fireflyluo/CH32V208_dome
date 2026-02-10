################################################################################
# MRS Version: 2.3.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../SRC/Debug/debug.c 

C_DEPS += \
./SRC/Debug/debug.d 

OBJS += \
./SRC/Debug/debug.o 

DIR_OBJS += \
./SRC/Debug/*.o \

DIR_DEPS += \
./SRC/Debug/*.d \

DIR_EXPANDS += \
./SRC/Debug/*.253r.expand \


# Each subdirectory must supply rules for building sources it contributes
SRC/Debug/%.o: ../SRC/Debug/%.c
	@	riscv-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -DCH32V20x_D8W -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/SRC/Debug" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/SRC/Core" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/APP/include" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/SRC/Peripheral/inc" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/HAL/include" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/LIB" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/Profile/include" -std=gnu17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

