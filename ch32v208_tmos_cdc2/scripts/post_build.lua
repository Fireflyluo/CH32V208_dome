-- scripts/post_build.lua
-- CH32V208 构建后处理脚本 - EIDE风格输出
-- 
-- 内存占比统计原理：
-- 1. 链接器通过 -Wl,--print-memory-usage 选项读取 Link.ld 中的 MEMORY 定义
-- 2. 使用 riscv-wch-elf-size -A 解析 ELF 文件各段(.text/.data/.bss等)大小
-- 3. 计算 FLASH = Code + RO Data + RW Data
-- 4. 计算 RAM   = RW Data + ZI Data

-- 从链接脚本解析内存配置
local function parse_memory_from_ldscript(ldscript_path)
    local ram_total = 64 * 1024      -- 默认值
    local rom_total = 128 * 1024     -- 默认值
    
    if not os.isfile(ldscript_path) then
        return rom_total, ram_total
    end
    
    local content = io.readfile(ldscript_path)
    if not content then
        return rom_total, ram_total
    end
    
    -- 移除 C 风格的多行注释 /* ... */
    content = content:gsub("/%*.-%*/", "")
    
    -- 查找 FLASH 和 RAM 的 LENGTH 定义 (非注释行)
    -- 匹配格式: FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 128K
    -- 行首可能是空白字符或制表符
    local flash_match = content:match("\n%s*FLASH%s*%([^)]*%)%s*:%s*ORIGIN%s*=%s*[^,]+,%s*LENGTH%s*=%s*(%d+%s*[KM]B?)")
    local ram_match = content:match("\n%s*RAM%s*%([^)]*%)%s*:%s*ORIGIN%s*=%s*[^,]+,%s*LENGTH%s*=%s*(%d+%s*[KM]B?)")
    
    -- 解析大小值 (支持 K, KB, M, MB 后缀)
    local function parse_size(size_str)
        if not size_str then return nil end
        local num, unit = size_str:match("(%d+)%s*([KM]B?)")
        if num then
            num = tonumber(num)
            if unit:sub(1,1) == "K" then
                return num * 1024
            elseif unit:sub(1,1) == "M" then
                return num * 1024 * 1024
            end
        end
        return tonumber(size_str)
    end
    
    local flash_size = parse_size(flash_match)
    local ram_size = parse_size(ram_match)
    
    if flash_size then rom_total = flash_size end
    if ram_size then ram_total = ram_size end
    
    return rom_total, ram_total
end

function main(target, toolchain_path)
    local toolchain_bin = toolchain_path .. "/bin"
    local elf_file = target:targetfile()
    local target_name = target:name()
    local output_dir = path.directory(elf_file)
    local hex_file = path.join(output_dir, target_name .. ".hex")
    local lst_file = path.join(output_dir, target_name .. ".lst")
    
    -- 从链接脚本自动获取内存配置
    -- 尝试多个可能的链接脚本路径 (xmake 使用的是 sdk/HAL/Link.ld)
    local ldscript_paths = {
        path.join(output_dir, "..", "sdk", "HAL", "Link.ld"),
        path.join(output_dir, "..", "..", "sdk", "HAL", "Link.ld"),
        "sdk/HAL/Link.ld",
        path.join(os.projectdir(), "sdk", "HAL", "Link.ld"),
        -- 兼容旧路径
        path.join(output_dir, "..", "sdk", "Ld", "Link.ld"),
        path.join(os.projectdir(), "sdk", "Ld", "Link.ld"),
    }
    
    local rom_total, ram_total
    for _, ld_path in ipairs(ldscript_paths) do
        rom_total, ram_total = parse_memory_from_ldscript(ld_path)
        if rom_total ~= 128 * 1024 or ram_total ~= 64 * 1024 then
            -- 成功从链接脚本读取到非默认值
            break
        end
    end
    
    -- 定义彩色输出函数
    local function info(msg)
        cprint("${cyan}[ 信息 ]${clear} %s", msg)
    end
    
    local function success(msg)
        cprint("${bright green}[ 成功 ]${clear} %s", msg)
    end
    
    local function tool(msg)
        cprint("${magenta}[ 工具 ]${clear} %s", msg)
    end
    
    local function done_msg(msg)
        cprint("${bright green}[ 完成 ]${clear} %s", msg)
    end
    
    -- 打印开始信息
    local now = os.date("%Y-%m-%d %H:%M:%S")
    cprint("")
    info("开始构建后处理于 " .. now)
    cprint("")
    
    -- 显示工具链信息
    tool("RISC-V WCH Embedded GCC")
    cprint("")
    
    -- 统计源文件
    local function show_file_statistics()
        info("源文件统计")
        cprint("")
        
        local c_files = 0
        local cpp_files = 0
        local asm_files = 0
        local lib_files = 0
        local total = 0
        
        for _, sourcefile in ipairs(target:sourcefiles()) do
            local ext = path.extension(sourcefile)
            if ext == ".c" then
                c_files = c_files + 1
            elseif ext == ".cpp" or ext == ".cc" or ext == ".cxx" then
                cpp_files = cpp_files + 1
            elseif ext == ".s" or ext == ".S" or ext == ".asm" then
                asm_files = asm_files + 1
            elseif ext == ".lib" or ext == ".a" then
                lib_files = lib_files + 1
            end
            total = total + 1
        end
        
        -- 打印表格
        cprint("+---------+-----------+-----------+---------------+--------+")
        cprint("| C 文件  | C++ 文件  | 汇编文件  | 库/对象文件   |  总计  |")
        cprint("+---------+-----------+-----------+---------------+--------+")
        cprint("| %-7d | %-9d | %-9d | %-13d | %-6d |", c_files, cpp_files, asm_files, lib_files, total)
        cprint("+---------+-----------+-----------+---------------+--------+")
        cprint("")
    end
    
    -- 显示详细的内存使用情况 - EIDE风格
    local function show_memory_usage_eide()
        info("开始链接分析 ...")
        cprint("")
        
        -- 获取详细段信息
        local size_result = os.iorunv(toolchain_bin .. "/riscv-wch-elf-size.exe", {"-A", elf_file})
        
        -- 解析各个段的大小
        local text_size = 0
        local rodata_size = 0
        local data_size = 0
        local bss_size = 0
        
        for line in size_result:gmatch("[^\r\n]+") do
            local section, size = line:match("^(%S+)%s+(%d+)")
            if section and size then
                size = tonumber(size)
                if section == ".text" or section == ".isr_vector" or section == ".init" or section == ".fini" then
                    text_size = text_size + size
                elseif section == ".rodata" or section == ".srodata" then
                    rodata_size = rodata_size + size
                elseif section == ".data" or section == ".sdata" or section == ".init_array" or section == ".fini_array" then
                    data_size = data_size + size
                elseif section == ".bss" or section == ".sbss" then
                    bss_size = bss_size + size
                end
            end
        end
        
        -- 计算总计
        local code = text_size
        local ro_data = rodata_size
        local rw_data = data_size
        local zi_data = bss_size
        
        local total_ro = code + ro_data
        local total_rw = rw_data + zi_data
        local total_rom = code + ro_data + rw_data
        
        -- 打印程序大小 - EIDE风格
        cprint("Program Size: Code=%d RO-data=%d RW-data=%d ZI-data=%d", 
               code, ro_data, rw_data, zi_data)
        cprint("")
        cprint("Total RO  Size (Code + RO Data)          %8d (%8.2f kB)", total_ro, total_ro / 1024)
        cprint("Total RW  Size (RW Data + ZI Data)       %8d (%8.2f kB)", total_rw, total_rw / 1024)
        cprint("Total ROM Size (Code + RO Data + RW Data)%8d (%8.2f kB)", total_rom, total_rom / 1024)
        cprint("")
        
        -- 打印内存使用进度条
        local function draw_bar(percent, width)
            local filled = math.floor(percent * width / 100)
            local empty = width - filled
            local bar = ""
            for i = 1, filled do
                bar = bar .. "#"
            end
            for i = 1, empty do
                bar = bar .. " "
            end
            return bar
        end
        
        local ram_percent = (total_rw / ram_total) * 100
        local rom_percent = (total_rom / rom_total) * 100
        
        -- 选择颜色
        local ram_color = ram_percent > 80 and "${red}" or (ram_percent > 50 and "${yellow}" or "${green}")
        local rom_color = rom_percent > 80 and "${red}" or (rom_percent > 50 and "${yellow}" or "${green}")
        
        cprint("Total Memory Usage:")
        cprint("")
        cprint("  RAM: " .. ram_color .. "[" .. draw_bar(ram_percent, 20) .. "]${clear} %5.1f%%  %6.1fKB/%6.1fKB", 
               ram_percent, total_rw / 1024, ram_total / 1024)
        cprint("  ROM: " .. rom_color .. "[" .. draw_bar(rom_percent, 20) .. "]${clear} %5.1f%%  %6.1fKB/%6.1fKB", 
               rom_percent, total_rom / 1024, rom_total / 1024)
        cprint("")
        
        -- Section 内存使用详情
        cprint("Section Memory Usage:")
        cprint("")
        cprint("  Flash (0x08000000): " .. rom_color .. "[" .. draw_bar(rom_percent, 20) .. "]${clear} %5.1f%%  %6.1fKB/%6.1fKB", 
               rom_percent, total_rom / 1024, rom_total / 1024)
        cprint("    - .text/.isr_vector: %6d bytes", code)
        cprint("    - .rodata:           %6d bytes", ro_data)
        cprint("    - .data:             %6d bytes", rw_data)
        cprint("")
        cprint("  RAM (0x20000000):   " .. ram_color .. "[" .. draw_bar(ram_percent, 20) .. "]${clear} %5.1f%%  %6.1fKB/%6.1fKB", 
               ram_percent, total_rw / 1024, ram_total / 1024)
        cprint("    - .data:             %6d bytes", rw_data)
        cprint("    - .bss:              %6d bytes", zi_data)
        cprint("")
    end
    
    -- 生成输出文件
    local function generate_output_files()
        info("开始生成输出文件 ...")
        cprint("")
        
        -- 生成 hex 文件
        os.execv(toolchain_bin .. "/riscv-wch-elf-objcopy.exe", {"-O", "ihex", elf_file, hex_file})
        if os.isfile(hex_file) then
            success("生成 HEX 文件              [完成]")
            cprint("        文件路径: \"%s\"", hex_file)
        end
        cprint("")
        
        -- 生成 lst 文件
        local lst_content = os.iorunv(toolchain_bin .. "/riscv-wch-elf-objdump.exe", 
            {"--all-headers", "--demangle", "--disassemble", "-M", "xw", elf_file})
        io.writefile(lst_file, lst_content)
        if os.isfile(lst_file) then
            success("生成 LST 文件              [完成]")
            cprint("        文件路径: \"%s\"", lst_file)
        end
        cprint("")
    end
    
    -- 执行输出函数
    show_file_statistics()
    show_memory_usage_eide()
    generate_output_files()
    
    -- 显示完成信息
    done_msg("构建成功完成!")
    cprint("")
end
