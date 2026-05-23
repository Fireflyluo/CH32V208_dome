#############################################################[Chinese Manual]###################################################################
# *****宸ョ▼璺緞涓嶈鍖呭惈涓枃瀛楃鍜岀┖鏍?鍚﹀垯鍙兘浼氭姤閿欙紒*****
# 绗竴姝ワ細 纭鏈哄櫒涓婂凡缁忓畨瑁呬簡MRS鐨勫伐鍏烽摼(鍋囪褰撳墠骞冲彴涓篖inux,宸ュ叿閾捐矾寰勪负"/path/to/toolchain/RISC-V Embedded GCC12",宸ュ叿閾剧浉鍏冲墠缂€涓?riscv-wch-elf-")
# 绗簩姝? 璁剧疆宸ュ叿閾捐矾寰?渚嬪淇敼鏇挎崲涓嬫柟鐨剆et(TOOLCHAIN_FOLDER "/path/to/toolchain/RISC-V Embedded GCC12")
# 绗笁姝? 璁剧疆宸ュ叿閾句腑鐩稿叧鍙墽琛屾枃浠剁殑鍓嶇紑,渚嬪淇敼鏇挎崲涓嬫柟鐨剆et(TOOLCHAIN_PREFIX "riscv-wch-elf-")
# 绗洓姝? 鎵ц cd your/path/of/project
# 绗簲姝? 鎵ц cmake -B build -G "Unix Makefiles"
# 绗叚姝? 鎵ц cmake --build build
# 娉ㄦ剰: 鍦╓indows骞冲彴杩愯,闇€瑕乤.瀹夎GNU MAKE骞跺皢鍏惰矾寰?bin鐩綍)娣诲姞鍒扮郴缁熺幆澧冨彉閲?涔嬪悗鎵цcmake -B build -G "Unix Makefiles"(鎺ㄨ崘)
#       鎴栬€卋.瀹夎MinGW,灏嗗叾璺緞(bin鐩綍)娣诲姞鍒扮郴缁熺幆澧冨彉閲?涔嬪悗鎵цcmake -B build -G "MinGW Makefiles"
# 濡傛湁鐩稿叧鐤戦棶,鍙€氳繃support@mounriver.com鑱旂郴鎴戜滑
#################################################################################################################################################


#############################################################[English Manual]###################################################################
# Step 1: Confirm that the MRS toolchain has been installed on the machine. Assuming the current platform is Linux, 
#         the toolchain path is "/path/to/toolchain/RISC-V Embedded GCC12", and the toolchain-related prefix is "riscv-wch-elf-"
# Step 2: Set the toolchain path, for example, modify and replace the following "set (TOOLCHAIN_FOLDER "/path/to/toolchain/RISC-V Embedded GCC12")"
# Step 3: Set the prefix of the executable files in the toolchain,for example, modify and replace the following "set(TOOLCHAIN_PREFIX "riscv-wch-elf-")"
# Step 4: Call "cd your/path/of/project"
# Step 5: Call "cmake -B build -G "Unix Makefiles""
# Step 6: Call "cmake --build build"
# Note:   To run on Windows, you need: a. Install GNU MAKE and add its path (the bin directory) to the system environment PATH. 
#         Then call the execution of "cmake -B build -G "Unix Makefiles""(Recommanded).
#         or b. Install MinGW and add its path (the bin directory) to the system environment PATH. 
#         Then call the execution of "cmake -B build -G "MinGW Makefiles"".
# If you have any questions, please contact us at support@mounriver.com
#################################################################################################################################################


# Set CMake minimum require
cmake_minimum_required(VERSION 3.16)

# Set CMake sysytem name
set(CMAKE_SYSTEM_NAME Generic)

# Enable clangd support: export compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# Skipping compiler detection and use custom toolchain
SET(CMAKE_C_COMPILER_WORKS TRUE)
SET(CMAKE_CXX_COMPILER_WORKS TRUE)
SET(CMAKE_ASM_COMPILER_WORKS TRUE)

# Use MRS toolchian to compile
set(TOOLCHAIN_FOLDER "e:/APP/MRS2/MounRiver_Studio2/resources/app/resources/win32/components/WCH/Toolchain/RISC-V Embedded GCC15")
set(TOOLCHAIN_PREFIX "riscv32-wch-elf-")
set(CMAKE_C_COMPILER "${TOOLCHAIN_FOLDER}/bin/riscv32-wch-elf-gcc.exe")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_FOLDER}/bin/riscv32-wch-elf-g++.exe")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_FOLDER}/bin/riscv32-wch-elf-gcc.exe")
set(CMAKE_OBJCOPY "${TOOLCHAIN_FOLDER}/bin/${TOOLCHAIN_PREFIX}objcopy.exe")
set(CMAKE_OBJDUMP "${TOOLCHAIN_FOLDER}/bin/riscv32-wch-elf-objdump.exe")
set(CMAKE_SIZE "${TOOLCHAIN_FOLDER}/bin/${TOOLCHAIN_PREFIX}size.exe")
set(CMAKE_AR "${TOOLCHAIN_FOLDER}/bin/${TOOLCHAIN_PREFIX}ar.exe")

set(PROJECT_NAME "CH32V208GBU_Templete")
set(TARGET_ARTIFACT "CH32V208GBU_Templete.elf")

set(CMAKE_C_OUTPUT_EXTENSION .o)
set(CMAKE_CXX_OUTPUT_EXTENSION .o)

project("${PROJECT_NAME}" LANGUAGES C ASM CXX)

# Use relative path
add_compile_options(-fmacro-prefix-map=${CMAKE_SOURCE_DIR}=..)


set(SRC_FILES 
"${CMAKE_CURRENT_SOURCE_DIR}/app/tasks/tmos_led_task.c"
"${CMAKE_CURRENT_SOURCE_DIR}/app/tasks/i2c_request_task.c"
"${CMAKE_CURRENT_SOURCE_DIR}/app/tasks/sensor_task.c"
"${CMAKE_CURRENT_SOURCE_DIR}/app/tasks/display_task.c"
"${CMAKE_CURRENT_SOURCE_DIR}/app/tasks/serial_upload_task.c"
"${CMAKE_CURRENT_SOURCE_DIR}/app/tasks/ad_hoc_task.c"
"${CMAKE_CURRENT_SOURCE_DIR}/app/adapters/adhoc_link_aros.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/USBLIB/USB-Driver/src/usb_core.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/USBLIB/USB-Driver/src/usb_init.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/USBLIB/USB-Driver/src/usb_int.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/USBLIB/USB-Driver/src/usb_mem.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/USBLIB/USB-Driver/src/usb_regs.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/USBLIB/USB-Driver/src/usb_sil.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/USBLIB/CONFIG/hw_config.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/USBLIB/CONFIG/usb_desc.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/USBLIB/CONFIG/usb_endp.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/USBLIB/CONFIG/usb_istr.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/USBLIB/CONFIG/usb_prop.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/USBLIB/CONFIG/usb_pwr.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Startup/startup_ch32v20x_D8W.S"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Peripheral/src/ch32v20x_adc.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Peripheral/src/ch32v20x_bkp.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Peripheral/src/ch32v20x_can.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Peripheral/src/ch32v20x_crc.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Peripheral/src/ch32v20x_dbgmcu.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Peripheral/src/ch32v20x_dma.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Peripheral/src/ch32v20x_exti.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Peripheral/src/ch32v20x_flash.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Peripheral/src/ch32v20x_gpio.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Peripheral/src/ch32v20x_i2c.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Peripheral/src/ch32v20x_iwdg.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Peripheral/src/ch32v20x_misc.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Peripheral/src/ch32v20x_opa.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Peripheral/src/ch32v20x_pwr.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Peripheral/src/ch32v20x_rcc.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Peripheral/src/ch32v20x_rtc.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Peripheral/src/ch32v20x_spi.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Peripheral/src/ch32v20x_tim.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Peripheral/src/ch32v20x_usart.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Peripheral/src/ch32v20x_wwdg.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/LIB/ble_task_scheduler.S"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/HAL/MCU.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/HAL/RTC.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/HAL/SLEEP.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Debug/debug.c"
"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Core/core_riscv.c"
"${CMAKE_CURRENT_SOURCE_DIR}/utils/ringbuffer.c"
"${CMAKE_CURRENT_SOURCE_DIR}/utils/data_protocol.c"
"${CMAKE_CURRENT_SOURCE_DIR}/utils/sw_timer.c"
"${CMAKE_CURRENT_SOURCE_DIR}/lib/sht40/adapters/sht40_ch32_adapter.c"
"${CMAKE_CURRENT_SOURCE_DIR}/lib/sht40/src/sht40_core.c"
"${CMAKE_CURRENT_SOURCE_DIR}/lib/sht40/src/sht40_sync.c"
"${CMAKE_CURRENT_SOURCE_DIR}/lib/sht40/src/sht40_async.c"
"${CMAKE_CURRENT_SOURCE_DIR}/lib/sc7a20/adapters/sc7a20_ch32_adapter.c"
"${CMAKE_CURRENT_SOURCE_DIR}/lib/sc7a20/src/sc7a20_core.c"
"${CMAKE_CURRENT_SOURCE_DIR}/lib/sc7a20/src/sc7a20_sync.c"
"${CMAKE_CURRENT_SOURCE_DIR}/lib/sc7a20/src/sc7a20_async.c"
"${CMAKE_CURRENT_SOURCE_DIR}/lib/impact_displacement/src/impact_displacement.c"
"${CMAKE_CURRENT_SOURCE_DIR}/lib/AROS-RF-LIB/src/aros_rf.c"
"${CMAKE_CURRENT_SOURCE_DIR}/lib/oled/OLED.c"
"${CMAKE_CURRENT_SOURCE_DIR}/lib/oled/OLED_Data.c"
"${CMAKE_CURRENT_SOURCE_DIR}/lib/Ad-Hoc-lib/src/adhoc_node.c"
"${CMAKE_CURRENT_SOURCE_DIR}/lib/Ad-Hoc-lib/src/adhoc_frame.c"
"${CMAKE_CURRENT_SOURCE_DIR}/lib/Ad-Hoc-lib/src/adhoc_crc8.c"
"${CMAKE_CURRENT_SOURCE_DIR}/lib/Ad-Hoc-lib/src/adhoc_timing.c"
"${CMAKE_CURRENT_SOURCE_DIR}/lib/Ad-Hoc-lib/src/adhoc_sm.c"
"${CMAKE_CURRENT_SOURCE_DIR}/lib/Ad-Hoc-lib/src/adhoc_reply_list.c"
"${CMAKE_CURRENT_SOURCE_DIR}/lib/Ad-Hoc-lib/src/adhoc_data_plane.c"
"${CMAKE_CURRENT_SOURCE_DIR}/lib/Ad-Hoc-lib/port/ch32v208/adhoc_port_ch32.c"
"${CMAKE_CURRENT_SOURCE_DIR}/bsp/board.c"
"${CMAKE_CURRENT_SOURCE_DIR}/bsp/drivers/src/drv_gpio.c"
"${CMAKE_CURRENT_SOURCE_DIR}/bsp/drivers/src/drv_i2c.c"
"${CMAKE_CURRENT_SOURCE_DIR}/bsp/drivers/src/drv_tim.c"
"${CMAKE_CURRENT_SOURCE_DIR}/bsp/drivers/src/drv_rtc.c"
"${CMAKE_CURRENT_SOURCE_DIR}/bsp/bus/src/i2c_bus_arbiter.c"
"${CMAKE_CURRENT_SOURCE_DIR}/bsp/usb_cdc.c"
"${CMAKE_CURRENT_SOURCE_DIR}/app/ch32v20x_it.c"
"${CMAKE_CURRENT_SOURCE_DIR}/app/main.c"
"${CMAKE_CURRENT_SOURCE_DIR}/app/system_ch32v20x.c"
)
set(CMAKE_C_FLAGS " -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I\"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Debug\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Core\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/app/include\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/app/adapters\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Peripheral/inc\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/sdk/HAL/include\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/sdk/LIB\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/app/tasks\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/bsp\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/bsp/drivers/inc\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/bsp/bus/inc\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/sdk/USBLIB/CONFIG\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/sdk/USBLIB/USB-Driver/inc\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/utils\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/lib/sht40\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/lib/sht40/inc\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/lib/sht40/adapters\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/lib/oled\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/lib/sc7a20\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/lib/sc7a20/inc\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/lib/sc7a20/adapters\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/lib/impact_displacement/inc\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/lib/AROS-RF-LIB/include\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/lib/Ad-Hoc-lib/include\" -I\"${CMAKE_CURRENT_SOURCE_DIR}/lib/Ad-Hoc-lib/port/ch32v208\" -DCH32V20x_D8W -DLOG_PRINT_ENABLE=1 -DARF_USE_EXTERNAL_MEM_BUF=1 -DRF_TG_ID=0 -DADHOC_TASK_ROLE_GATEWAY=0 -DADHOC_TASK_GATEWAY_NO=0 -DADHOC_ENABLE=1 -std=gnu17")
set(CMAKE_CXX_FLAGS " -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -std=gnu++11 -fabi-version=0")
set(CMAKE_ASM_FLAGS " -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I\"${CMAKE_CURRENT_SOURCE_DIR}/sdk/Startup\" -x assembler-with-cpp")

add_executable("${TARGET_ARTIFACT}" ${SRC_FILES})

set_target_properties("${TARGET_ARTIFACT}" PROPERTIES LINK_FLAGS " -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -T \"${CMAKE_CURRENT_SOURCE_DIR}/sdk/HAL/Link.ld\"  -L\"${CMAKE_CURRENT_SOURCE_DIR}/sdk/LIB\"  -nostartfiles -Xlinker --gc-sections -Xlinker --print-memory-usage -Wl,-Map,\"${CMAKE_BINARY_DIR}/CH32V208GBU_Templete.map\" --specs=nano.specs --specs=nosys.specs")

target_link_libraries(
"${TARGET_ARTIFACT}" 
wchble
m
)

add_custom_command(TARGET "${TARGET_ARTIFACT}" 
POST_BUILD 
COMMAND ${CMAKE_OBJCOPY} ARGS -O ihex  "${TARGET_ARTIFACT}" "${PROJECT_NAME}.hex"
COMMAND ${CMAKE_OBJDUMP} ARGS  --all-headers --demangle --disassemble -M xw "${TARGET_ARTIFACT}" > "${PROJECT_NAME}.lst"
COMMAND ${CMAKE_SIZE} ARGS  --format=berkeley "${TARGET_ARTIFACT}"
WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# Copy compile_commands.json to .vscode for clangd support
add_custom_command(TARGET "${TARGET_ARTIFACT}" POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_BINARY_DIR}/compile_commands.json"
    "${CMAKE_SOURCE_DIR}/.vscode/compile_commands.json"
    COMMENT "Copying compile_commands.json to .vscode for clangd"
)


