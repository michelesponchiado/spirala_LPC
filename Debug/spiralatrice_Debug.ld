/*
 * GENERATED FILE - DO NOT EDIT
 * (c) Code Red Technologies Ltd, 2008-13
 * (c) NXP Semiconductors 2013-2016
 * Generated linker script file for LPC2388
 * Created from generic_c.ld (7.8.0 ())
 * By LPCXpresso v7.8.0 [Build 426] [2015-05-28]  on Wed Dec 28 18:08:39 CET 2016
 */


INCLUDE "spiralatrice_Debug_lib.ld"
INCLUDE "spiralatrice_Debug_mem.ld"

ENTRY(_start)

SECTIONS
{

    /* MAIN TEXT SECTION */    
    .text : ALIGN(4)
    {
        FILL(0xff)
        __vectors_start__ = ABSOLUTE(.) ;
        KEEP(*(.isr_vector))
        
        /* Global Section Table */
        . = ALIGN(4) ;
        __section_table_start = .;
        __data_section_table = .;
        LONG(LOADADDR(.data));
        LONG(    ADDR(.data));
        LONG(  SIZEOF(.data));
        LONG(LOADADDR(.data_RAM2));
        LONG(    ADDR(.data_RAM2));
        LONG(  SIZEOF(.data_RAM2));
        LONG(LOADADDR(.data_RAM3));
        LONG(    ADDR(.data_RAM3));
        LONG(  SIZEOF(.data_RAM3));
        LONG(LOADADDR(.data_RAM4));
        LONG(    ADDR(.data_RAM4));
        LONG(  SIZEOF(.data_RAM4));
        __data_section_table_end = .;
        __bss_section_table = .;
        LONG(    ADDR(.bss));
        LONG(  SIZEOF(.bss));
        LONG(    ADDR(.bss_RAM2));
        LONG(  SIZEOF(.bss_RAM2));
        LONG(    ADDR(.bss_RAM3));
        LONG(  SIZEOF(.bss_RAM3));
        LONG(    ADDR(.bss_RAM4));
        LONG(  SIZEOF(.bss_RAM4));
        __bss_section_table_end = .;
        __section_table_end = . ;
        /* End of Global Section Table */
        

        *(.after_vectors*)
        
        /* Code Read Protect data */
        . = 0x000001FC ;
        PROVIDE(__CRP_WORD_START__ = .) ;
        KEEP(*(.crp))
        PROVIDE(__CRP_WORD_END__ = .) ;
        ASSERT(!(__CRP_WORD_START__ == __CRP_WORD_END__), "Linker CRP Enabled, but no CRP_WORD provided within application");
        /* End of Code Read Protect */
        
    } >MFlash512
    
    .text : ALIGN(4)    
    {
         *(.text*)
        *(.rodata .rodata.* .constdata .constdata.*)
        . = ALIGN(4);
        
    } > MFlash512

    /*
     * for exception handling/unwind - some Newlib functions (in common
     * with C++ and STDC++) use this. 
     */
    .ARM.extab : ALIGN(4)
    {
    	*(.ARM.extab* .gnu.linkonce.armextab.*)
    } > MFlash512
    __exidx_start = .;
    
    .ARM.exidx : ALIGN(4)
    {
    	*(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > MFlash512
    __exidx_end = .;
    
    _etext = .;
        
    
    /* DATA section for RAM_ETH_16 */
    .data_RAM2 : ALIGN(4)
    {
       FILL(0xff)
       PROVIDE(__start_data_RAM2 = .) ;
       *(.ramfunc.$RAM2)
       *(.ramfunc.$RAM_ETH_16)
    	*(.data.$RAM2*)
    	*(.data.$RAM_ETH_16*)
       . = ALIGN(4) ;
       PROVIDE(__end_data_RAM2 = .) ;
    } > RAM_ETH_16 AT>MFlash512
    
    /* DATA section for RAM_USB_16 */
    .data_RAM3 : ALIGN(4)
    {
       FILL(0xff)
       PROVIDE(__start_data_RAM3 = .) ;
       *(.ramfunc.$RAM3)
       *(.ramfunc.$RAM_USB_16)
    	*(.data.$RAM3*)
    	*(.data.$RAM_USB_16*)
       . = ALIGN(4) ;
       PROVIDE(__end_data_RAM3 = .) ;
    } > RAM_USB_16 AT>MFlash512
    
    /* DATA section for RAM_RTC_2 */
    .data_RAM4 : ALIGN(4)
    {
       FILL(0xff)
       PROVIDE(__start_data_RAM4 = .) ;
       *(.ramfunc.$RAM4)
       *(.ramfunc.$RAM_RTC_2)
    	*(.data.$RAM4*)
    	*(.data.$RAM_RTC_2*)
       . = ALIGN(4) ;
       PROVIDE(__end_data_RAM4 = .) ;
    } > RAM_RTC_2 AT>MFlash512
    
    /* MAIN DATA SECTION */
    

    .uninit_RESERVED : ALIGN(4)
    {
        KEEP(*(.bss.$RESERVED*))
        . = ALIGN(4) ;
        _end_uninit_RESERVED = .;
    } > Ram64

	
	/* Main DATA section (Ram64) */
	.data : ALIGN(4)
	{
	   FILL(0xff)
	   _data = . ;
	   *(vtable)
	   *(.ramfunc*)
	   *(.data*)
	   . = ALIGN(4) ;
	   _edata = . ;
	} > Ram64 AT>MFlash512

    /* BSS section for RAM_ETH_16 */
    .bss_RAM2 : ALIGN(4)
    {
       PROVIDE(__start_bss_RAM2 = .) ;
    	*(.bss.$RAM2*)
    	*(.bss.$RAM_ETH_16*)
       . = ALIGN(4) ;
       PROVIDE(__end_bss_RAM2 = .) ;
    } > RAM_ETH_16
    /* BSS section for RAM_USB_16 */
    .bss_RAM3 : ALIGN(4)
    {
       PROVIDE(__start_bss_RAM3 = .) ;
    	*(.bss.$RAM3*)
    	*(.bss.$RAM_USB_16*)
       . = ALIGN(4) ;
       PROVIDE(__end_bss_RAM3 = .) ;
    } > RAM_USB_16
    /* BSS section for RAM_RTC_2 */
    .bss_RAM4 : ALIGN(4)
    {
       PROVIDE(__start_bss_RAM4 = .) ;
    	*(.bss.$RAM4*)
    	*(.bss.$RAM_RTC_2*)
       . = ALIGN(4) ;
       PROVIDE(__end_bss_RAM4 = .) ;
    } > RAM_RTC_2

    /* MAIN BSS SECTION */
    .bss : ALIGN(4)
    {
        _bss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4) ;
        _ebss = .;
        PROVIDE(end = .);
    } > Ram64
        
    /* NOINIT section for RAM_ETH_16 */
    .noinit_RAM2 (NOLOAD) : ALIGN(4)
    {
    	*(.noinit.$RAM2*)
    	*(.noinit.$RAM_ETH_16*)
       . = ALIGN(4) ;
    } > RAM_ETH_16 
    /* NOINIT section for RAM_USB_16 */
    .noinit_RAM3 (NOLOAD) : ALIGN(4)
    {
    	*(.noinit.$RAM3*)
    	*(.noinit.$RAM_USB_16*)
       . = ALIGN(4) ;
    } > RAM_USB_16 
    /* NOINIT section for RAM_RTC_2 */
    .noinit_RAM4 (NOLOAD) : ALIGN(4)
    {
    	*(.noinit.$RAM4*)
    	*(.noinit.$RAM_RTC_2*)
       . = ALIGN(4) ;
    } > RAM_RTC_2 
    
    /* DEFAULT NOINIT SECTION */
    .noinit (NOLOAD): ALIGN(4)
    {
        _noinit = .;
        *(.noinit*) 
         . = ALIGN(4) ;
        _end_noinit = .;
    } > Ram64
    
    PROVIDE(_pvHeapStart = DEFINED(__user_heap_base) ? __user_heap_base : .);
    PROVIDE(_vStackTop = DEFINED(__user_stack_top) ? __user_stack_top : __top_Ram64 - 0);
}
