################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/ch32v20x_it.c \
d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/main.c \
d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/peripheral.c \
d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/system_ch32v20x.c 

C_DEPS += \
./app/ch32v20x_it.d \
./app/main.d \
./app/peripheral.d \
./app/system_ch32v20x.d 

OBJS += \
./app/ch32v20x_it.o \
./app/main.o \
./app/peripheral.o \
./app/system_ch32v20x.o 

DIR_OBJS += \
./app/*.o \

DIR_DEPS += \
./app/*.d \

DIR_EXPANDS += \
./app/*.271r.expand \


# Each subdirectory must supply rules for building sources it contributes
app/ch32v20x_it.o: d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/ch32v20x_it.c
	@	riscv32-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -gdwarf-4 -DCH32V20x_D8W -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Debug" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Core" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Peripheral/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/HAL/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/LIB" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/ble_profile/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/tasks" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/bsp/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/USBLIB/CONFIG" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/USBLIB/USB-Driver/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/utils" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sht40" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/oled" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20/inc" -isystem"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/bsp" -std=gnu17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
app/main.o: d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/main.c
	@	riscv32-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -gdwarf-4 -DCH32V20x_D8W -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Debug" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Core" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Peripheral/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/HAL/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/LIB" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/ble_profile/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/tasks" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/bsp/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/USBLIB/CONFIG" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/USBLIB/USB-Driver/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/utils" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sht40" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/oled" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20/inc" -isystem"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/bsp" -std=gnu17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
app/peripheral.o: d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/peripheral.c
	@	riscv32-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -gdwarf-4 -DCH32V20x_D8W -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Debug" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Core" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Peripheral/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/HAL/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/LIB" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/ble_profile/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/tasks" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/bsp/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/USBLIB/CONFIG" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/USBLIB/USB-Driver/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/utils" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sht40" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/oled" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20/inc" -isystem"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/bsp" -std=gnu17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
app/system_ch32v20x.o: d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/system_ch32v20x.c
	@	riscv32-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -gdwarf-4 -DCH32V20x_D8W -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Debug" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Core" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/Peripheral/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/HAL/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/LIB" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/ble_profile/include" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/app/tasks" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/bsp/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/USBLIB/CONFIG" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/sdk/USBLIB/USB-Driver/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/utils" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sht40" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/oled" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20" -I"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/lib/sc7a20/inc" -isystem"d:/Desktop/ch32/0.ch32v208_dome/ch32v208_tmos_cdc3/bsp" -std=gnu17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

