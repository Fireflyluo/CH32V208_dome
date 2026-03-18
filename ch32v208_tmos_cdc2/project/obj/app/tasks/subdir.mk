################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/app/tasks/tmos_led_task.c 

C_DEPS += \
./app/tasks/tmos_led_task.d 

OBJS += \
./app/tasks/tmos_led_task.o 

DIR_OBJS += \
./app/tasks/*.o \

DIR_DEPS += \
./app/tasks/*.d \

DIR_EXPANDS += \
./app/tasks/*.271r.expand \


# Each subdirectory must supply rules for building sources it contributes
app/tasks/tmos_led_task.o: d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/app/tasks/tmos_led_task.c
	@	riscv32-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -gdwarf-4 -DCH32V20x_D8W -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/sdk/Debug" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/sdk/Core" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/app/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/sdk/Peripheral/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/sdk/HAL/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/sdk/LIB" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/ble_profile/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/app/tasks" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/bsp/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/bsp/UART" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/sdk/USBLIB/CONFIG" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/sdk/USBLIB/USB-Driver/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/utils" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/lib/sht40" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/lib/oled" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/lib/sc7a20htr/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/lib/sc7a20htr/platform" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/lib/sc7a20htr/example" -std=gnu17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

