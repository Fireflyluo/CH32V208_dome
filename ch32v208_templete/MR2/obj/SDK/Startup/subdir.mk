################################################################################
# MRS Version: 2.3.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
../SDK/Startup/startup_ch32v20x_D8W.S 

S_UPPER_DEPS += \
./SDK/Startup/startup_ch32v20x_D8W.d 

OBJS += \
./SDK/Startup/startup_ch32v20x_D8W.o 

DIR_OBJS += \
./SDK/Startup/*.o \

DIR_DEPS += \
./SDK/Startup/*.d \

DIR_EXPANDS += \
./SDK/Startup/*.253r.expand \


# Each subdirectory must supply rules for building sources it contributes
SDK/Startup/%.o: ../SDK/Startup/%.S
	@	riscv-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -x assembler-with-cpp -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/SDK/Startup" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

