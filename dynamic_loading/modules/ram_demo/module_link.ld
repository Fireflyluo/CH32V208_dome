ENTRY(module_get_exports)

MEMORY
{
    RAM_MODULE (rxw) : ORIGIN = 0x2000E000, LENGTH = 8K
}

SECTIONS
{
    .text :
    {
        . = ALIGN(4);
        KEEP(*(.text.module_get_exports))
        *(.text .text.*)
        *(.rodata .rodata.*)
        . = ALIGN(4);
    } >RAM_MODULE

    /DISCARD/ :
    {
        *(.data .data.*)
        *(.bss .bss.*)
        *(.sdata .sdata.*)
        *(.sbss .sbss.*)
        *(.eh_frame*)
        *(.comment)
        *(.note*)
        *(.riscv.attributes)
        *(.init_array*)
        *(.fini_array*)
    }
}
