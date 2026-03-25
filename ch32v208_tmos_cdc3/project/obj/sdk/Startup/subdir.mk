################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Startup/startup_ch32v20x_D8W.S 

S_UPPER_DEPS += \
./sdk/Startup/startup_ch32v20x_D8W.d 

OBJS += \
./sdk/Startup/startup_ch32v20x_D8W.o 

DIR_OBJS += \
./sdk/Startup/*.o \

DIR_DEPS += \
./sdk/Startup/*.d \

DIR_EXPANDS += \
./sdk/Startup/*.271r.expand \


# Each subdirectory must supply rules for building sources it contributes
sdk/Startup/startup_ch32v20x_D8W.o: d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Startup/startup_ch32v20x_D8W.S
	@	riscv32-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -gdwarf-4 -x assembler-with-cpp -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Startup" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

