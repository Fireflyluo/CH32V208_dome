################################################################################
# MRS Version: 2.3.0
# Automatically-generated file. Do not edit!
################################################################################
-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include SDK/Startup/subdir.mk
-include SDK/Peripheral/src/subdir.mk
-include SDK/Debug/subdir.mk
-include SDK/Core/subdir.mk
-include Profile/subdir.mk
-include LIB/subdir.mk
-include HAL/subdir.mk
-include APP/subdir.mk
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
	@	riscv-wch-elf-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -T "d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/HAL/Link.ld" -nostartfiles -Xlinker --gc-sections -L"d:/Desktop/ch32/0.CH32V208_demo/ch32v208_templete/MR2/LIB" -Xlinker --print-memory-usage -Wl,-Map,"CH32V208GBU_Templete.map" --specs=nano.specs --specs=nosys.specs -o "CH32V208GBU_Templete.elf" $(OBJS) $(LIBS)
CH32V208GBU_Templete.hex: CH32V208GBU_Templete.elf
	@	riscv-wch-elf-objcopy -O ihex "CH32V208GBU_Templete.elf" "CH32V208GBU_Templete.hex"
CH32V208GBU_Templete.lst: CH32V208GBU_Templete.elf
	@	riscv-wch-elf-objdump --all-headers --demangle --disassemble -M xw "CH32V208GBU_Templete.elf" > "CH32V208GBU_Templete.lst"
CH32V208GBU_Templete.siz: CH32V208GBU_Templete.elf
	riscv-wch-elf-size --format=berkeley "CH32V208GBU_Templete.elf"

# Other Targets
clean:
	-$(RM) $(DIR_OBJS) $(SECONDARY_FLASH)$(SECONDARY_LIST)$(SECONDARY_SIZE) CH32V208GBU_Templete.elf
	-$(RM) $(DIR_EXPANDS) $(CALLGRAPH_DOT)
	-$(RM) $(DIR_DEPS)
secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents

-include ../makefile.targets