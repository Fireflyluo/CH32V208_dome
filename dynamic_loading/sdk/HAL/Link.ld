ENTRY( _start )

__stack_size = 2048;

PROVIDE( _stack_size = __stack_size );

/*
 * CH32V208 CodeFlash layout (common on CH32V208 BLE parts):
 * - 0x08000000..0x0801FFFF (128KB)  : zero-wait execute region
 * - 0x08020000..            : non-zero-wait region
 *
 * This project also uses the tail of flash for BLE SNV storage via BLE_SNV_ADDR.
 * Keep linker allocations below that address to avoid corrupting SNV pages.
 */
__flash_origin      = 0x00000000;
__flash_snv_start   = 0x00077C00; /* 0x08077C00 - reserved for BLE SNV */
__ram_origin        = 0x20000000;
__ram_total_size    = 0x00010000; /* 64KB */
__module_ram_size   = 0x00002000; /* 8KB reserved for one loadable RAM module */
__module_ram_origin = __ram_origin + (__ram_total_size - __module_ram_size);

MEMORY
{  
/* CH32V20x_D6 - CH32V203F6-CH32V203G6-CH32V203C6 */
/*
	FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 32K
	RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 10K
*/ 

/* CH32V20x_D6 - CH32V203K8-CH32V203C8-CH32V203G8-CH32V203F8 */
/*
	FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 64K
	RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 20K
*/ 
  
/* CH32V20x_D8 - CH32V203RB
   CH32V20x_D8W - CH32V208x
   FLASH + RAM supports the following configuration
   FLASH-128K + RAM-64K
   FLASH-144K + RAM-48K
   FLASH-160K + RAM-32K
*/
	FLASH0 (rx) : ORIGIN = __flash_origin, LENGTH = 128K
	FLASH1 (rx) : ORIGIN = 0x00020000, LENGTH = 351K
	FLASH_SNV (rw) : ORIGIN = __flash_snv_start, LENGTH = 1K
	RAM_APP (xrw)    : ORIGIN = __ram_origin, LENGTH = __ram_total_size - __module_ram_size
	RAM_MODULE (xrw) : ORIGIN = __module_ram_origin, LENGTH = __module_ram_size

}


SECTIONS
{

	.init :
	{
		_sinit = .;
		. = ALIGN(4);
		KEEP(*(SORT_NONE(.init)))
		. = ALIGN(4);
		_einit = .;
	} >FLASH0 AT>FLASH0

  .vector :
  {
      *(.vector);
	  . = ALIGN(64);
		KEEP(*(SORT_NONE(.handle_reset)))
  } >FLASH0 AT>FLASH0
  
     .highcode : 
    {
        . = ALIGN(4);
        *(.highcode);
        *(.highcode.*);
		. = ALIGN(4); 
    } >FLASH0 AT>FLASH0

	.flash1_text :
	{
		. = ALIGN(4);
		*(.flash1_text .flash1_text.*)
		. = ALIGN(4);
	} >FLASH1 AT>FLASH1

	.flash1_rodata :
	{
		. = ALIGN(4);
		*(.flash1_rodata .flash1_rodata.*)
		. = ALIGN(4);
	} >FLASH1 AT>FLASH1

	.flash1_module :
	{
		. = ALIGN(16);
		__module_flash_start__ = .;
		KEEP(*(.flash1_module .flash1_module.*))
		. = ALIGN(16);
		__module_flash_end__ = .;
	} >FLASH1 AT>FLASH1
    
	.text :
	{
		. = ALIGN(4);

        EXCLUDE_FILE (*wchble.a) *(.text .text*)
       
		*(.text)
		*(.text.*)
		*(.rodata)
		*(.rodata*)
  		*(.sdata2.*)
		*(.glue_7)
		*(.glue_7t)
		*(.gnu.linkonce.t.*)
		. = ALIGN(4);
	} >FLASH0 AT>FLASH0 

	.fini :
	{
		KEEP(*(SORT_NONE(.fini)))
		. = ALIGN(4);
	} >FLASH0 AT>FLASH0

	PROVIDE( _etext = . );
	PROVIDE( _eitcm = . );	

	.preinit_array  :
	{
	  PROVIDE_HIDDEN (__preinit_array_start = .);
	  KEEP (*(.preinit_array))
	  PROVIDE_HIDDEN (__preinit_array_end = .);
	} >FLASH0 AT>FLASH0 
	
	.init_array     :
	{
	  PROVIDE_HIDDEN (__init_array_start = .);
	  KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
	  KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
	  PROVIDE_HIDDEN (__init_array_end = .);
	} >FLASH0 AT>FLASH0 
	
	.fini_array     :
	{
	  PROVIDE_HIDDEN (__fini_array_start = .);
	  KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
	  KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
	  PROVIDE_HIDDEN (__fini_array_end = .);
	} >FLASH0 AT>FLASH0 
	
	.ctors          :
	{
	  /* gcc uses crtbegin.o to find the start of
	     the constructors, so we make sure it is
	     first.  Because this is a wildcard, it
	     doesn't matter if the user does not
	     actually link against crtbegin.o; the
	     linker won't look for a file to match a
	     wildcard.  The wildcard also means that it
	     doesn't matter which directory crtbegin.o
	     is in.  */
	  KEEP (*crtbegin.o(.ctors))
	  KEEP (*crtbegin?.o(.ctors))
	  /* We don't want to include the .ctor section from
	     the crtend.o file until after the sorted ctors.
	     The .ctor section from the crtend file contains the
	     end of ctors marker and it must be last */
	  KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
	  KEEP (*(SORT(.ctors.*)))
	  KEEP (*(.ctors))
	} >FLASH0 AT>FLASH0 
	
	.dtors          :
	{
	  KEEP (*crtbegin.o(.dtors))
	  KEEP (*crtbegin?.o(.dtors))
	  KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
	  KEEP (*(SORT(.dtors.*)))
	  KEEP (*(.dtors))
	} >FLASH0 AT>FLASH0 

	.dalign :
	{
		. = ALIGN(4);
		PROVIDE(_data_vma = .);
	} >RAM_APP AT>FLASH0	

	.dlalign :
	{
		. = ALIGN(4); 
		PROVIDE(_data_lma = .);
	} >FLASH0 AT>FLASH0

	.data :
	{
    	*(.gnu.linkonce.r.*)
    	*(.data .data.*)
    	*(.gnu.linkonce.d.*)
		. = ALIGN(8);
    	PROVIDE( __global_pointer$ = . + 0x800 );
    	*(.sdata .sdata.*)
    	*(.gnu.linkonce.s.*)
    	. = ALIGN(8);
    	*(.srodata.cst16)
    	*(.srodata.cst8)
    	*(.srodata.cst4)
    	*(.srodata.cst2)
    	*(.srodata .srodata.*)
    	. = ALIGN(4);
		PROVIDE( _edata = .);
	} >RAM_APP AT>FLASH0

	.bss :
	{
		. = ALIGN(4);
		PROVIDE( _sbss = .);
  	    *(.sbss*)
        *(.gnu.linkonce.sb.*)
		*(.bss*)
     	*(.gnu.linkonce.b.*)		
		*(COMMON*)
		. = ALIGN(4);
		PROVIDE( _ebss = .);
	} >RAM_APP AT>FLASH0

	.module_slot (NOLOAD) :
	{
		. = ALIGN(16);
		__module_ram_start__ = .;
		. = . + LENGTH(RAM_MODULE);
		. = ALIGN(16);
		__module_ram_end__ = .;
	} >RAM_MODULE

	PROVIDE(__module_ram_size__ = LENGTH(RAM_MODULE));

	PROVIDE( _end = _ebss);
	PROVIDE( end = . );

    .stack ORIGIN(RAM_APP) + LENGTH(RAM_APP) - __stack_size :
    {
        PROVIDE( _heap_end = . );   
        . = ALIGN(4);
        PROVIDE(_susrstack = . );
        . = . + __stack_size;
        PROVIDE( _eusrstack = .);
    } >RAM_APP

	/*.stack ORIGIN(RAM)+LENGTH(RAM) :
	{
        PROVIDE( _heap_end = . );   
		. = ALIGN(4);
		PROVIDE(_eusrstack = . );
	} >RAM  */
}
