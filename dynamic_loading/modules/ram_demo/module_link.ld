ENTRY(module_get_exports)

MEMORY
{
    RAM_MODULE (rxw) : ORIGIN = 0x2000C000, LENGTH = 16K
}

SECTIONS
{
    .text :
    {
        . = ALIGN(4);
        KEEP(*(.text.module_get_exports))
        *(.text .text.*)
        *(.rodata .rodata.*)
        PROVIDE(__global_pointer$ = . + 0x800);
        *(.sdata .sdata.*)
        *(.data .data.*)
        *(.sbss .sbss.*)
        *(.bss .bss.* COMMON)
        . = ALIGN(4);
    } >RAM_MODULE

    /DISCARD/ :
    {
        *(.eh_frame*)
        *(.comment)
        *(.note*)
        *(.riscv.attributes)
        *(.init_array*)
        *(.fini_array*)
    }
}
