/*****************************************************************************
 *   +--+
 *   | ++----+   
 *   +-++    |  
 *     |     |  
 *   +-+--+  |   
 *   | +--+--+  
 *   +----+    Copyright (c) 2009-13 Code Red Technologies Ltd.
 *
 * LPC23xx/LPC24xx Startup code for use with Red Suite
 *
 * Version : 130903
 *
 * Software License Agreement
 * 
 * The software is owned by Code Red Technologies and/or its suppliers, and is 
 * protected under applicable copyright laws.  All rights are reserved.  Any 
 * use in violation of the foregoing restrictions may subject the user to criminal 
 * sanctions under applicable laws, as well as to civil liability for the breach 
 * of the terms and conditions of this license.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 * USE OF THIS SOFTWARE FOR COMMERCIAL DEVELOPMENT AND/OR EDUCATION IS SUBJECT
 * TO A CURRENT END USER LICENSE AGREEMENT (COMMERCIAL OR EDUCATIONAL) WITH
 * CODE RED TECHNOLOGIES LTD. 
 *
 ******************************************************************************/

/*******************************************************************************
 * 
 * Conventions used in the assembler code:
 * 
 * Mnemonics are in upper case.
 * labels (prog or data) are in lower case.
 * Constants for initialization values and/or conditional assembly are
 * in upper case.
 *
 * The following C preprocessor 'switches' are used in this file to conditionally
 * assemble certain parts of the code:
 *
 * PLL_INIT    : Define this to initialise the PLL
 * VPB_INIT    : Define this to initialise the VPB divider
 * MAM_INIT    : Define this to initialise the MAM
 * MMAP_INIT   : Define this to initialise the MMAP
 * STACK_INIT  : Define this to initialise the STACK
 * USE_OLD_STYLE_DATA_BSS_INIT : Define this to build startup code with pre-3.6
 *               version of the Code Red tools
 *
 *******************************************************************************/

		.global main                    // int main(void)

#ifndef USE_OLD_STYLE_DATA_BSS_INIT
/*****************************************************************************
/ The following symbols are constructs generated by the linker, indicating
/ the location of various points in the "Global Section Table". This table is
/ created by the linker via the Code Red managed linker script mechanism. It
/ contains the load address, execution address and length of each RW data
/ section and the execution and length of each BSS (zero initialized) section.
/*****************************************************************************/
		.global __data_section_table
        .global __data_section_table_end
        .global __bss_section_table
        .global __bss_section_table_end
#else
/*****************************************************************************
/ The following symbols are constructs generated by the linker, indicating
/ the load address, execution address and length of the RW data section and
/ the execution and length of the BSS (zero initialized) section.
/ Note that these symbols are not normally used by the managed linker script
/ mechanism in Red Suite/LPCXpresso 3.6 (Windows) and LPCXpresso 3.8 (Linux).
/ They are provide here simply so this startup code can be used with earlier
/ versions of Red Suite which do not support the more advanced managed linker
/ script mechanism introduced in the above version. To enable their use,
/ define "USE_OLD_STYLE_DATA_BSS_INIT".
/*****************************************************************************/
		.global _etext
		.global _data
		.global _edata
		.global _bss
		.global _ebss;
#endif

        .global _vStackTop              // top of stack

		//	Stack Sizes - remember these are bytes not words

        .set  UND_STACK_SIZE, 0x00000010
        .set  SVC_STACK_SIZE, 0x00000010
        .set  ABT_STACK_SIZE, 0x00000010
        .set  FIQ_STACK_SIZE, 0x00000010
        .set  IRQ_STACK_SIZE, 0X00000100

        .set  USR_STACK_SIZE, 0x00000400 // Shared with SYS mode
        
        .set TOTAL_STACK_SIZE, UND_STACK_SIZE + SVC_STACK_SIZE + \
                               ABT_STACK_SIZE + FIQ_STACK_SIZE + \
                               IRQ_STACK_SIZE + USR_STACK_SIZE



		//	LPC23xx mode bit and interrupt flag definitions

        .set  MODE_USR, 0x10            // User Mode
        .set  MODE_FIQ, 0x11            // FIQ Mode
        .set  MODE_IRQ, 0x12            // IRQ Mode
        .set  MODE_SVC, 0x13            // Supervisor Mode
        .set  MODE_ABT, 0x17            // Abort Mode
        .set  MODE_UND, 0x1B            // Undefined Mode
        .set  MODE_SYS, 0x1F            // System Mode

        .set  I_BIT, 0x80               // when I bit is set, IRQ is disabled
        .set  F_BIT, 0x40               // when F bit is set, FIQ is disabled

        .text
        .code 32
        .align 2

		.section .isr_vector,"x"
        .global _boot
		.arm        
        .func   _boot
_boot:

//	Exception Processing Vectors

Vectors:
        B     _start                    // reset
        LDR   pc,_undf                  // undefined
        LDR   pc,_swi                   // SWI/SVC
        LDR   pc,_pabt                  // program abort
        LDR   pc,_dabt                  // data abort
        NOP                             // Reserved for the flash checksum
		LDR	  pc,[pc, #-0x0120]			// VICAddress (vectored interrupts 0-31)
        LDR   pc,_fiq                   // FIQ


_undf:  .word __undf                    // undefined
_swi:   .word _swi_handler				// SWI
_pabt:  .word __pabt					// program abort
_dabt:  .word __dabt					// data abort
_irq:   .word __irq						// CRT_IRQ_Handler			
_fiq:   .word __fiq						// FIQ

		// Some simple default handlers

__undf: B     .                         // undefined
__pabt: B     .                         // program abort
__dabt: B     .                         // data abort
__irq:  B     .                         // IRQ
__fiq:  B     .                         // FIQ

		// Default SWI handler. Will not get here if
		// semihosting is enabled.
_swi_handler:
		B	.
		
        .size _boot, . - _boot
        .endfunc

		// Setup the operating mode & stack.

        .global _start, start, _mainCRTStartup
        .arm        
        .func   _start
_start:
start:
_mainCRTStartup:
		.set	SYSCTRL,			0xE01FC000

#ifdef PLL_INIT
		
		.set	PLL0,				SYSCTRL
		.set	PLLCON_OFFSET,		0x80
		.set	PLLCFG_OFFSET,		0x84
		.set	PLLSTAT_OFFSET,		0x88
		.set	PLLFEED_OFFSET,		0x8C

		.set	CCLKCFG_OFFSET,     0x104
		.set	USBCLKCFG_OFFSET,   0x108
		.set	CLKSRCSEL_OFFSET,   0x10C
		.set	SCS_OFFSET,     	0x1A0
		.set	PCLKSEL0_OFFSET,    0x1A8
		.set	PCLKSEL1_OFFSET,    0x1AC

		.set	SET_OSCRANGE,		(1 << 4)    // Oscillator Range Select
		.set	SET_OSCENABLE,		(1 << 5)    // Oscillator Enable
		.set	MSK_OSCSTATUS,		(1 << 6)    // Oscillator Status
		
		.set	SET_PLLCON_ENABLE,	(1 << 0)    // PLL0 Enable
		.set	SET_PLLCON_CONNECT,	(1 << 1)    // PLL0 Connect	
			
		.set	MSK_PLLSTAT_MSEL,	(0x7FFF) 	// PLL0 MSEL
		.set	MSK_PLLSTAT_NSEL,	(0xFF << 16)// PLL0 NSEL
		.set	PLLSTAT_PLOCK,		(1 << 26)   // PLL0 Lock bit

		// 14:00 MSEL
		.set	SET_PLLCFG_MUL1,	0x00
		.set	SET_PLLCFG_MUL2,	0x01
		.set	SET_PLLCFG_MUL3,	0x02		
		.set	SET_PLLCFG_MUL4,	0x03
		.set	SET_PLLCFG_MUL5,	0x04
		.set	SET_PLLCFG_MUL6,	0x05
		.set	SET_PLLCFG_MUL7,	0x06
		.set	SET_PLLCFG_MUL8,	0x07		
		.set	SET_PLLCFG_MUL9,	0x08
		.set	SET_PLLCFG_MUL10,	0x09
		.set	SET_PLLCFG_MUL11,   0x0A
		.set	SET_PLLCFG_MUL12,   0x0B
		.set	SET_PLLCFG_MUL13,   0x0C
		.set	SET_PLLCFG_MUL14,   0x0D
		.set	SET_PLLCFG_MUL15,   0x0E
		.set	SET_PLLCFG_MUL16,   0x0F
		.set	SET_PLLCFG_MUL17,   0x10
		.set	SET_PLLCFG_MUL18,   0x11
		.set	SET_PLLCFG_MUL19,   0x12
		.set	SET_PLLCFG_MUL20,   0x13
		.set	SET_PLLCFG_MUL21,   0x14
		.set	SET_PLLCFG_MUL22,   0x15
		.set	SET_PLLCFG_MUL23,   0x16
		.set	SET_PLLCFG_MUL24,   0x17
		.set	SET_PLLCFG_MUL25,   0x18
		.set	SET_PLLCFG_MUL26,   0x19
		.set	SET_PLLCFG_MUL27,   0x1A
		.set	SET_PLLCFG_MUL28,   0x1B
		.set	SET_PLLCFG_MUL29,   0x1C
		.set	SET_PLLCFG_MUL30,   0x1D
		.set	SET_PLLCFG_MUL31,   0x1E
		.set	SET_PLLCFG_MUL32,   0x1F
		// 23:16 NSEL
		.set	SET_PLLCFG_DIV1,	(0x0 << 16)
		.set	SET_PLLCFG_DIV2,	(0x1 << 16)
		.set	SET_PLLCFG_DIV3,	(0x2 << 16)		
		.set	SET_PLLCFG_DIV4,	(0x3 << 16)
		.set	SET_PLLCFG_DIV5,	(0x4 << 16)
		.set	SET_PLLCFG_DIV6,	(0x5 << 16)
		.set	SET_PLLCFG_DIV7,	(0x6 << 16)						
		.set	SET_PLLCFG_DIV8,	(0x7 << 16)
		.set	SET_PLLCFG_DIV9,	(0x8 << 16)
		.set	SET_PLLCFG_DIV10,	(0x9 << 16)		
		.set	SET_PLLCFG_DIV11,	(0xA << 16)
		.set	SET_PLLCFG_DIV12,	(0xB << 16)
		.set	SET_PLLCFG_DIV13,	(0xC << 16)
		.set	SET_PLLCFG_DIV14,	(0xD << 16)
		.set	SET_PLLCFG_DIV15,	(0xE << 16)
		.set	SET_PLLCFG_DIV16,	(0xF << 16)

		.set 	SET_CPUCLK_DIV1,	0
		.set 	SET_CPUCLK_DIV2,	1
		.set 	SET_CPUCLK_DIV3,	2
		.set 	SET_CPUCLK_DIV4,	3
		.set 	SET_CPUCLK_DIV5,	4
		.set 	SET_CPUCLK_DIV6,	5
		
		//  Fcco = (2 * MSEL * Fin) / NSEL
		//  Fin: 12000000 Hz
		//
		//  PLL 48 MHz: Fcco: 288 MHz, MSEL: 12 (12x multiply), NSEL: 1 (1x divide)
		//  PLLCFG:  MSEL - 1	(accounted for in the macros setting)
		//           NSEL - 1  
		//	CCLKCFG: 5
 		.set	PLLCFG_INIT_VAL,	(SET_PLLCFG_MUL12 | SET_PLLCFG_DIV1)
		.set	PCLKSEL_INIT_VAL,	0x00000000
		.set	SET_SCS,			0x20		
		.set	SET_CLKSRCSEL,		0x01
		.set	USBCLKCFG_INIT_VAL,	0x05

		//	Setup the PLL
		LDR		r0,=PLL0
		MOV		r1,#0xAA
		MOV		r2,#0x55

		LDR     r3,=SET_SCS          		// use main oscillator
		STR     r3,[r0, #SCS_OFFSET]
                
1:		LDR     r3,[r0, #SCS_OFFSET]   		// wait until stable
        ANDS    r3,r3, #MSK_OSCSTATUS
        BEQ     1b

		LDR		r3,=SET_CLKSRCSEL			// PLL is clock source
		STR     r3,[r0, #CLKSRCSEL_OFFSET] 

		LDR		r3,=PLLCFG_INIT_VAL
		STR     r3,[r0, #PLLCFG_OFFSET] 
		STR     r1,[r0, #PLLFEED_OFFSET]
		STR     r2,[r0, #PLLFEED_OFFSET]
		
		LDR		r3,=SET_CPUCLK_DIV6			// clock divider
		STR     r3,[r0, #CCLKCFG_OFFSET]
		
		// Setup the USB clock divider
		LDR     r3, =USBCLKCFG_INIT_VAL
		STR     r3, [R0, #USBCLKCFG_OFFSET]

		LDR		r3,=SET_PLLCON_ENABLE
		STR     r3,[r0, #PLLCON_OFFSET]
		STR     r1,[r0, #PLLFEED_OFFSET]
		STR     r2,[r0, #PLLFEED_OFFSET]       
		
2:		LDR		r3,[r0, #PLLSTAT_OFFSET]	// wait until PLL0 lock
		ANDS	r3,r3, #PLLSTAT_PLOCK
		BEQ		2b

		// Now swap the cpu clock to the PLL 
		LDR		r3,=(SET_PLLCON_ENABLE | SET_PLLCON_CONNECT)
		STR		r3,[r0, #PLLCON_OFFSET]
		STR		r1,[r0, #PLLFEED_OFFSET]
		STR		r2,[r0, #PLLFEED_OFFSET]

		LDR		r3,=PCLKSEL_INIT_VAL		// peripheral bus same as PLL
		STR		r3,[r0, #PCLKSEL0_OFFSET]

#endif

#ifdef VPB_INIT

		.set	PCLKSEL0_OFFSET,		0x1A8
		.set	PCLKSEL1_OFFSET,		0x1AC
		
		.set	PCLKSEL_INIT_VAL,		0	// CLCK / 4
		.set	PCLKSEL1_INIT_VAL,		0	// CLCK / 4

		// Setup the peripheral bus clocks
 		LDR		r0,=SYSCTRL
		LDR     r3,=PCLKSEL_INIT_VAL
		STR     r3,[r0, #PCLKSEL0_OFFSET]
		LDR     r3,=PCLKSEL1_INIT_VAL
		STR     r3,[r0, #PCLKSEL1_OFFSET]
		
#endif

		// Setup the MEMMAP Register
		//	1 - Flash
		//	2 - RAM

#ifdef MMAP_INIT
		.set	MEMMAP_OFFSET,			0x040
		.set 	MMAP_FLASH_INIT_VAL,	1
		.set 	MMAP_RAM_INIT_VAL,		2

 		LDR		r0,=SYSCTRL
		LDR		r1,=MMAP_FLASH_INIT_VAL
		STR		r1,[r0, #MEMMAP_OFFSET]

#endif

#ifdef MAM_INIT

		// Setup the Memory Accelerator Block (MAM)
	
		.set	MAMCR_OFFSET,		0x0
		.set	MAMTIM_OFFSET,		0x4

		.set	SET_MAMCR_DISABLE,	0x0
		.set	SET_MAMCR_PARTIAL,	0x1
		.set	SET_MAMCR_FULL,		0x2

		// How many cycles for flash access

		.set	SET_MAMTIM_0CLK,	0x0
		.set	SET_MAMTIM_1CLK,	0x1
		.set	SET_MAMTIM_2CLK,	0x2
		.set	SET_MAMTIM_3CLK,	0x3
		.set	SET_MAMTIM_4CLK,	0x4
		.set	SET_MAMTIM_5CLK,	0x5
		.set	SET_MAMTIM_6CLK,	0x6
		.set	SET_MAMTIM_7CLK,	0x7
		
		.set	MAMCR_INIT_VAL,		SET_MAMCR_FULL		
		.set	MAMTIM_INIT_VAL,	SET_MAMTIM_3CLK
		
		LDR		r0,=SYSCTRL
		MOV		r1,#MAMTIM_INIT_VAL
		STR		r1,[r0, #MAMTIM_OFFSET]
		MOV		r1,#MAMCR_INIT_VAL
		STR		r1,[r0, #MAMCR_OFFSET]

#endif

		// Setup some stack space for each ARM operating mode

        LDR   r0,=_vStackTop
        MSR   CPSR_c,#MODE_UND|I_BIT|F_BIT	// Undefined Instruction Mode
        MOV   sp,r0
        SUB   r0,r0,#UND_STACK_SIZE
        MSR   CPSR_c,#MODE_ABT|I_BIT|F_BIT	// Abort Mode
        MOV   sp,r0
        SUB   r0,r0,#ABT_STACK_SIZE
        MSR   CPSR_c,#MODE_FIQ|I_BIT|F_BIT	// FIQ Mode
        MOV   sp,r0
        SUB   r0,r0,#FIQ_STACK_SIZE
        MSR   CPSR_c,#MODE_IRQ|I_BIT|F_BIT	// IRQ Mode
        MOV   sp,r0
        SUB   r0,r0,#IRQ_STACK_SIZE
        MSR   CPSR_c,#MODE_SVC|I_BIT|F_BIT	// Supervisor Mode
        MOV   sp,r0
        SUB   r0,r0,#SVC_STACK_SIZE
        MSR   CPSR_c,#MODE_SYS|I_BIT|F_BIT	// System Mode
        MOV   sp,r0

#ifndef USE_OLD_STYLE_DATA_BSS_INIT
/*
 * Copy RWdata initial values from flash to its execution
 * address in RAM
 */
		LDR	r4, =Ldata_start	// Load base address of data...
		LDR r4, [r4]			// ...from Global Section Table
		LDR r5, =Ldata_end		// Load end address of data...
		LDR r5, [r5]			//...from Global Section Table
start_data_init_loop:
		CMP	r4,r5				// Check to see if reached end of...
		BEQ	end_data_init_loop	// ...data entries in G.S.T.
		LDR r0, [r4],#4			// Load LoadAddr from G.S.T.
		LDR r1, [r4],#4			// Load ExeAddr from G.S.T.
		LDR r2, [r4],#4			// Load SectionLen from G.S.T.
		BL	data_init			// Call subroutine to do copy
		B start_data_init_loop	// Loop back for next entry in G.S.T.

end_data_init_loop:
/*
 * Clear .bss (zero'ed space)
 */
		LDR r5, =Lbss_end		// Load end address of BSS...
		LDR r5, [r5]			//...from Global Section Table
start_bss_init_loop:
		CMP	r4,r5				// Check to see if reached end of...
		BEQ	post_data_bss_init	// ...bss entries in G.S.T.
		LDR r0, [r4],#4			// Load ExeAddr from G.S.T.
		LDR r1, [r4],#4			// Load SectionLen from G.S.T.
		BL	bss_init			// Call subroutine to do zero'ing
		B	start_bss_init_loop	// Loop back for next entry in G.S.T.

Ldata_start:   .word __data_section_table
Ldata_end:     .word __data_section_table_end
Lbss_end:      .word __bss_section_table_end

/******************************************************************************
 * Functions to carry out the initialization of RW and BSS data sections. These
 * are written as separate functions to cope with MCUs with multiple banks of
 * memory.
 ******************************************************************************/
// void data_init(unsigned int romstart, unsigned int start, unsigned int len)
data_init:
		MOV	r12,#0
.di_loop:
		CMP	r12,r2
        LDRLO 	r3,[r0],#4
        STRLO 	r3,[r1],#4
		ADDLO	r12,r12,#4
		BLO	.di_loop
		BX LR

// void bss_init(unsigned int start, unsigned int len)
bss_init:
		MOV	r12,#0
		MOV r2, #0
.bi_loop:
		CMP	r12,r1
        STRLO 	r2,[r0],#4
		ADDLO	r12,r12,#4
		BLO	.bi_loop
		BX LR


/******************************************************************************
 * Back to main flow of Reset_Handler
 ******************************************************************************/
post_data_bss_init:

#else
	// Use Old Style Data and BSS section initialization.
	// This will only initialize a single RAM bank
	//
	// Copy initialized data to its execution address in RAM
        LDR   r1,=_etext                // -> ROM data start
        LDR   r2,=_data                 // -> data start
        LDR   r3,=_edata                // -> end of data
1:      CMP   r2,r3                     // check if data to move
        LDRLO r0,[r1],#4                // copy it
        STRLO r0,[r2],#4
        BLO   1b                        // loop until done

	//Clear .bss (zero'ed space)
        MOV   r0,#0                     // get a zero
        LDR   r1,=_bss                  // -> bss start
        LDR   r2,=_ebss                 // -> bss end
2:      CMP   r1,r2                     // check if data to clear
        STRLO r0,[r1],#4                // clear 4 bytes
        BLO   2b                        // loop until done
#endif

#ifdef STACK_INIT
		// Initialize the stack to known values to aid debugging. This is
		// definitely optional, but can help early debugging for stack
		// overflows. Also system optimization for measuring stack depth
		// used.

		.global _vStackTop
		
		MOV		r0,#0					// start of count
		LDR		r1,=_vStackTop			// start from top
                LDR r3,=TOTAL_STACK_SIZE
                SUB r2,r1,r3
3:
		CMP		r1,r2
		STRGT	r0,[r1,#-4]!				// walk down the stack
		ADD		r0,r0,#1
		BGT		3b

#endif
		
       //
       // Call C++ library initilisation, if present
       //
.cpp_init:
        LDR   r3, .def__libc_init_array  // if
        CMP   r3, #0
        BEQ   .skip_cpp_init
        BL    __libc_init_array
.skip_cpp_init:

/*
 * Call main program: main(0)
 */
        MOV   r0,#0                     // no arguments (argc = 0)
        MOV   r1,r0
        MOV   r2,r0
        MOV   fp,r0                     // null frame pointer
        MOV   r7,r0                     // null frame pointer for thumb


		// Change to system mode (IRQs enabled) before calling main application

        MSR   CPSR_c,#MODE_SYS|F_BIT	// System Mode

#ifdef __REDLIB__
        LDR r10, =__main
#else
        LDR r10, =main
#endif

        MOV   lr,pc
        BX    r10                       // enter main() - could be ARM or Thumb

        .size   _start, . - _start
        .endfunc

        .global _reset, reset
        .func   _reset
_reset:
reset:
exit:

        B     .                         // loop until reset

	.weak   __libc_init_array
.def__libc_init_array:
	.word   __libc_init_array

        .weak 	init_lpc3xxx			// void init_lpc31xx(void)
.def__init_lpc3xxx:
		.word	init_lpc3xxx


        .size _reset, . - _reset
        .endfunc

        .end
