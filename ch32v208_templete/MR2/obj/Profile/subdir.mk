################################################################################
# MRS Version: 2.3.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Profile/devinfoservice.c \
../Profile/gattprofile.c 

C_DEPS += \
./Profile/devinfoservice.d \
./Profile/gattprofile.d 

OBJS += \
./Profile/devinfoservice.o \
./Profile/gattprofile.o 

DIR_OBJS += \
./Profile/*.o \

DIR_DEPS += \
./Profile/*.d \

DIR_EXPANDS += \
./Profile/*.253r.expand \


# Each subdirectory must supply rules for building sources it contributes
Profile/%.o: ../Profile/%.c
	@	riscv-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -DCH32V20x_D8W -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/SDK/Debug" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/SDK/Core" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/APP/include" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/SDK/Peripheral/inc" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/HAL/include" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/LIB" -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/Profile/include" -std=gnu17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

