################################################################################
# MRS Version: 2.3.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
../SRC/Startup/startup_ch32v20x_D8W.S 

S_UPPER_DEPS += \
./SRC/Startup/startup_ch32v20x_D8W.d 

OBJS += \
./SRC/Startup/startup_ch32v20x_D8W.o 

DIR_OBJS += \
./SRC/Startup/*.o \

DIR_DEPS += \
./SRC/Startup/*.d \

DIR_EXPANDS += \
./SRC/Startup/*.253r.expand \


# Each subdirectory must supply rules for building sources it contributes
SRC/Startup/%.o: ../SRC/Startup/%.S
	@	riscv-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -x assembler-with-cpp -I"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/SRC/Startup" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

