################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/lib/sc7a20/adapters/mock_adapter.c \
d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/lib/sc7a20/adapters/sc7a20_ch32_adapter.c 

C_DEPS += \
./lib/sc7a20/adapters/mock_adapter.d \
./lib/sc7a20/adapters/sc7a20_ch32_adapter.d 

OBJS += \
./lib/sc7a20/adapters/mock_adapter.o \
./lib/sc7a20/adapters/sc7a20_ch32_adapter.o 

DIR_OBJS += \
./lib/sc7a20/adapters/*.o \

DIR_DEPS += \
./lib/sc7a20/adapters/*.d \

DIR_EXPANDS += \
./lib/sc7a20/adapters/*.271r.expand \


# Each subdirectory must supply rules for building sources it contributes
lib/sc7a20/adapters/mock_adapter.o: d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/lib/sc7a20/adapters/mock_adapter.c
	@	riscv32-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -gdwarf-4 -DCH32V20x_D8W -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/app/include" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/app/tasks" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/ble_profile/include" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/bsp" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/bsp/bus/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/bsp/drivers/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/lib" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/lib/oled" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/lib/sc7a20/adapters" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/lib/sc7a20/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/lib/sht40/adapters" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/lib/sht40/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/sdk/Core" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/sdk/Debug" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/sdk/HAL/include" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/sdk/Ld" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/sdk/LIB" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/sdk/Peripheral/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/sdk/USBLIB/CONFIG" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/sdk/USBLIB/USB-Driver/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/utils" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/test" -isystem"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/bsp" -std=gnu17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
lib/sc7a20/adapters/sc7a20_ch32_adapter.o: d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/lib/sc7a20/adapters/sc7a20_ch32_adapter.c
	@	riscv32-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -gdwarf-4 -DCH32V20x_D8W -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/app/include" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/app/tasks" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/ble_profile/include" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/bsp" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/bsp/bus/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/bsp/drivers/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/lib" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/lib/oled" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/lib/sc7a20/adapters" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/lib/sc7a20/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/lib/sht40/adapters" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/lib/sht40/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/sdk/Core" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/sdk/Debug" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/sdk/HAL/include" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/sdk/Ld" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/sdk/LIB" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/sdk/Peripheral/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/sdk/USBLIB/CONFIG" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/sdk/USBLIB/USB-Driver/inc" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/utils" -I"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/test" -isystem"d:/Desktop/ch32/0.ch32v208_dome/Demo_v0.1/bsp" -std=gnu17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

