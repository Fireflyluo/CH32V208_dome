################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20/src/sc7a20_async.c \
d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20/src/sc7a20_core.c \
d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20/src/sc7a20_sync.c 

C_DEPS += \
./lib/sc7a20/src/sc7a20_async.d \
./lib/sc7a20/src/sc7a20_core.d \
./lib/sc7a20/src/sc7a20_sync.d 

OBJS += \
./lib/sc7a20/src/sc7a20_async.o \
./lib/sc7a20/src/sc7a20_core.o \
./lib/sc7a20/src/sc7a20_sync.o 

DIR_OBJS += \
./lib/sc7a20/src/*.o \

DIR_DEPS += \
./lib/sc7a20/src/*.d \

DIR_EXPANDS += \
./lib/sc7a20/src/*.271r.expand \


# Each subdirectory must supply rules for building sources it contributes
lib/sc7a20/src/sc7a20_async.o: d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20/src/sc7a20_async.c
	@	riscv32-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -gdwarf-4 -DCH32V20x_D8W -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Debug" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Core" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Peripheral/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/HAL/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/LIB" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/ble_profile/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/tasks" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/bsp/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/USBLIB/CONFIG" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/USBLIB/USB-Driver/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/utils" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sht40" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/oled" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20/inc" -isystem"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/bsp" -std=gnu17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
lib/sc7a20/src/sc7a20_core.o: d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20/src/sc7a20_core.c
	@	riscv32-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -gdwarf-4 -DCH32V20x_D8W -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Debug" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Core" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Peripheral/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/HAL/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/LIB" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/ble_profile/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/tasks" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/bsp/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/USBLIB/CONFIG" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/USBLIB/USB-Driver/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/utils" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sht40" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/oled" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20/inc" -isystem"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/bsp" -std=gnu17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
lib/sc7a20/src/sc7a20_sync.o: d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20/src/sc7a20_sync.c
	@	riscv32-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -gdwarf-4 -DCH32V20x_D8W -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Debug" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Core" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Peripheral/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/HAL/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/LIB" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/ble_profile/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/tasks" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/bsp/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/USBLIB/CONFIG" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/USBLIB/USB-Driver/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/utils" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sht40" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/oled" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20/inc" -isystem"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/bsp" -std=gnu17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

