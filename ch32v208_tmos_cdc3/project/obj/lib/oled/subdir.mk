################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/oled/OLED.c \
d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/oled/OLED_Data.c 

C_DEPS += \
./lib/oled/OLED.d \
./lib/oled/OLED_Data.d 

OBJS += \
./lib/oled/OLED.o \
./lib/oled/OLED_Data.o 

DIR_OBJS += \
./lib/oled/*.o \

DIR_DEPS += \
./lib/oled/*.d \

DIR_EXPANDS += \
./lib/oled/*.271r.expand \


# Each subdirectory must supply rules for building sources it contributes
lib/oled/OLED.o: d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/oled/OLED.c
	@	riscv32-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -gdwarf-4 -DCH32V20x_D8W -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/tasks" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/ble_profile/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/bsp" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/bsp/bus/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/bsp/drivers/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/oled" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20/adapters" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sht40/adapters" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sht40/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Core" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Debug" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/HAL/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Ld" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/LIB" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Peripheral/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/USBLIB/CONFIG" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/USBLIB/USB-Driver/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/utils" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/test" -isystem"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/bsp" -std=gnu17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
lib/oled/OLED_Data.o: d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/oled/OLED_Data.c
	@	riscv32-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -gdwarf-4 -DCH32V20x_D8W -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/tasks" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/ble_profile/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/bsp" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/bsp/bus/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/bsp/drivers/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/oled" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20/adapters" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sht40/adapters" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sht40/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Core" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Debug" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/HAL/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Ld" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/LIB" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Peripheral/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/USBLIB/CONFIG" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/USBLIB/USB-Driver/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/utils" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/test" -isystem"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/bsp" -std=gnu17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

