/*
 * GENERATED FILE - DO NOT EDIT
 * (c) Code Red Technologies Ltd, 2008-13
 * (c) NXP Semiconductors 2013-2016
 * Generated linker script file for LPC2388
 * Created from generic_c.ld (7.8.0 ())
 * By LPCXpresso v7.8.0 [Build 426] [2015-05-28]  on Wed Dec 28 18:08:39 CET 2016
 */


INCLUDE "my_spiralatrice_Debug_lib.ld"
INCLUDE "my_spiralatrice_Debug_mem.ld"


/*
the user mode stack is an exception because we want it at the end of the RAM.
therefore, we just check against a minimum.
The value below is the minimum memory required for ALL THE STACKS.
There will be a link error if there is not this amount of RAM free at the end.
*/
_Minimum_Stack_Size = 0xC00 ;
/*	0x1000 = heap memory =4096 bytes
	Nella versione upsd avevo riservato 3500 bytes, quindi 4096 dovrebbero essere sufficienti
*/
_Minimum_Heap_Size  = 0x2000 ;


ENTRY(_start)

SECTIONS
{
PROVIDE( _start = 0x00000000); 




/* the user firmware version string should be at a fixed address    */
	user_fw_version 0x8000 :{
	} >MFlash512
	
/* the main entry point should be at a fixed address    */
	MAIN_ENTRY_POINT 0x8100 :
    {
        KEEP(*(.MAIN_ENTRY_POINT))            /* main entry point */
        KEEP(*(.text.main))            /* main entry point */
    } >MFlash512
    /* MAIN TEXT SECTION */  

    
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
	/* This is used by the startup in order to initialize the .data section */
   	_sidata = _etext;
        
    
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
	.data : AT( _sidata)
	{
	   . = ALIGN(4) ;
	
	   FILL(0xff)
	   _data = . ;
        /* This is used by the startup in order to initialize the .data secion */
        _sdata = . ;
	   
	   *(vtable)
	   *(.ramfunc*)
	   *(.data*)
	   . = ALIGN(4) ;
	   _edata = . ;
	} > Ram64 

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
        /* This is used by the startup in order to initialize the .bss secion */
        _sbss = .;
        
        _lib_bss_start = .;
			*libc.a:*(.bss)
			*libgcc.a:*(.bss)
			*std_sbrk.lib:*(.bss)
			*target.o(.bss)
			*codelist.o(.bss)
			*Anybus.o(.bss)
			*rtcDS3231.o(.bss)
			*spiwin2.o(.bss)
			*vdip.o(.bss)
        _lib_bss_end = .;
                
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
    
    /* This is the user stack section 
    This is just to check that there is enough RAM left for the User mode stack
    It should generate an error if it's full.
     */
    /* This is the user heap section */
    ._usrheap :
    {
	    . = ALIGN(4);
        _susrheap = . ;
        
        . = . + _Minimum_Heap_Size ;
        
	    . = ALIGN(4);
        _eusrheap = . ;
    } >Ram64
    ._usrstack :
    {
	    . = ALIGN(4);
        _susrstack = . ;
        
        . = . + _Minimum_Stack_Size ;
        
	    . = ALIGN(4);
        _eusrstack = . ;
        _estack = . ;
    } >Ram64
    
    
    ._AZ4186_NVRAM (NOLOAD):  
    {
        *(AZ4186_NVRAM_SECTION) ;
    } >AZ4186_NVRAM
        
    
    /*PROVIDE(_pvHeapStart = DEFINED(__user_heap_base) ? __user_heap_base : .);*/
    PROVIDE(_pvHeapStart = _susrheap);
    /* PROVIDE(_vStackTop = DEFINED(__user_stack_top) ? __user_stack_top : __top_Ram64 - 0); */
    /*PROVIDE(_vStackTop = __user_stack_top);*/
     PROVIDE(_vStackTop = __top_Ram64);
}
