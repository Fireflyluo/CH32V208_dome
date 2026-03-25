################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/LIB/ble_task_scheduler.S 

S_UPPER_DEPS += \
./sdk/LIB/ble_task_scheduler.d 

OBJS += \
./sdk/LIB/ble_task_scheduler.o 

DIR_OBJS += \
./sdk/LIB/*.o \

DIR_DEPS += \
./sdk/LIB/*.d \

DIR_EXPANDS += \
./sdk/LIB/*.271r.expand \


# Each subdirectory must supply rules for building sources it contributes
sdk/LIB/ble_task_scheduler.o: d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/LIB/ble_task_scheduler.S
	@	riscv32-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -gdwarf-4 -x assembler-with-cpp -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Startup" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

