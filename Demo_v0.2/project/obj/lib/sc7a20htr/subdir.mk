################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/lib/sc7a20htr/sc7a20_core.c \
d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/lib/sc7a20htr/sc7a20_core_async.c 

C_DEPS += \
./lib/sc7a20htr/sc7a20_core.d \
./lib/sc7a20htr/sc7a20_core_async.d 

OBJS += \
./lib/sc7a20htr/sc7a20_core.o \
./lib/sc7a20htr/sc7a20_core_async.o 

DIR_OBJS += \
./lib/sc7a20htr/*.o \

DIR_DEPS += \
./lib/sc7a20htr/*.d \

DIR_EXPANDS += \
./lib/sc7a20htr/*.271r.expand \


# Each subdirectory must supply rules for building sources it contributes
lib/sc7a20htr/sc7a20_core.o: d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/lib/sc7a20htr/sc7a20_core.c
	@	riscv32-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -gdwarf-4 -DCH32V20x_D8W -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/sdk/Debug" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/sdk/Core" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/app/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/sdk/Peripheral/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/sdk/HAL/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/sdk/LIB" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/ble_profile/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/app/tasks" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/bsp/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/bsp/UART" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/sdk/USBLIB/CONFIG" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/sdk/USBLIB/USB-Driver/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/utils" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/lib/sht40" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/lib/oled" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/lib/sc7a20htr/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/lib/sc7a20htr/platform" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/lib/sc7a20htr/example" -std=gnu17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
lib/sc7a20htr/sc7a20_core_async.o: d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/lib/sc7a20htr/sc7a20_core_async.c
	@	riscv32-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -gdwarf-4 -DCH32V20x_D8W -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/sdk/Debug" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/sdk/Core" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/app/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/sdk/Peripheral/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/sdk/HAL/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/sdk/LIB" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/ble_profile/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/app/tasks" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/bsp/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/bsp/UART" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/sdk/USBLIB/CONFIG" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/sdk/USBLIB/USB-Driver/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/utils" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/lib/sht40" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/lib/oled" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/lib/sc7a20htr/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/lib/sc7a20htr/platform" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc2/lib/sc7a20htr/example" -std=gnu17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

