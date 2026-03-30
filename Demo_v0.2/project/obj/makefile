################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################
-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include utils/subdir.mk
-include sdk/USBLIB/USB-Driver/src/subdir.mk
-include sdk/USBLIB/CONFIG/subdir.mk
-include sdk/Startup/subdir.mk
-include sdk/Peripheral/src/subdir.mk
-include sdk/LIB/subdir.mk
-include sdk/HAL/subdir.mk
-include sdk/Debug/subdir.mk
-include sdk/Core/subdir.mk
-include lib/sht40/src/subdir.mk
-include lib/sht40/adapters/subdir.mk
-include lib/sc7a20/src/subdir.mk
-include lib/sc7a20/adapters/subdir.mk
-include lib/oled/subdir.mk
-include bsp/drivers/src/subdir.mk
-include bsp/bus/src/subdir.mk
-include bsp/subdir.mk
-include ble_profile/subdir.mk
-include app/tasks/subdir.mk
-include app/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
CH32V208GBU_Templete.hex \

SECONDARY_LIST += \
CH32V208GBU_Templete.lst \

SECONDARY_SIZE += \
CH32V208GBU_Templete.siz \


# All Target
all: 
	$(MAKE) --no-print-directory main-build 

main-build: CH32V208GBU_Templete.elf secondary-outputs

# Tool invocations
CH32V208GBU_Templete.elf: $(OBJS) $(USER_OBJS_ESCAPE)
	@	riscv32-wch-elf-g++ -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -gdwarf-4 -T "d:\Desktop\ch32\0.ch32v208_dome\Demo_v0.1\sdk\HAL\Link.ld" -nostartfiles -Xlinker --gc-sections -L"d:\Desktop\ch32\0.ch32v208_dome\Demo_v0.1\sdk\LIB" -Xlinker --print-memory-usage -Xlinker --print-memory-usage -Wl,-Map,"CH32V208GBU_Templete.map" --specs=nano.specs --specs=nosys.specs -o "CH32V208GBU_Templete.elf" $(OBJS) $(LIBS)
CH32V208GBU_Templete.hex: CH32V208GBU_Templete.elf
	@	riscv32-wch-elf-objcopy -O ihex "CH32V208GBU_Templete.elf" "CH32V208GBU_Templete.hex"
CH32V208GBU_Templete.lst: CH32V208GBU_Templete.elf
	@	riscv32-wch-elf-objdump --all-headers --demangle --disassemble -M xw "CH32V208GBU_Templete.elf" > "CH32V208GBU_Templete.lst"
CH32V208GBU_Templete.siz: CH32V208GBU_Templete.elf
	riscv32-wch-elf-size --format=berkeley "CH32V208GBU_Templete.elf"

# Other Targets
clean:
	-$(RM) $(DIR_OBJS) $(SECONDARY_FLASH)$(SECONDARY_LIST)$(SECONDARY_SIZE) CH32V208GBU_Templete.elf
	-$(RM) $(DIR_EXPANDS) $(CALLGRAPH_DOT)
	-$(RM) $(DIR_DEPS)
secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents

-include ../makefile.targets