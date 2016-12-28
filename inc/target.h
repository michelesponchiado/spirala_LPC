/*****************************************************************************
 *   target.h:  Header file for NXP LPC23xx/24xx Family Microprocessors
 *
 *   Copyright(C) 2006, NXP Semiconductor
 *   All rights reserved.
 *
 *   History
 *   2006.09.20  ver 1.00    Prelimnary version, first Release
 *
******************************************************************************/
#ifndef __TARGET_H 
#define __TARGET_H

#ifdef __cplusplus
   extern "C" {
#endif

/* Only choose one of them below, by default, it's Keil MCB2300 */
#define ENG_BOARD_LPC24XX				0
#define KEIL_BOARD_LPC23XX				1
#define EA_BOARD_LPC24XX				0
#define IAR_BOARD_LPC23XX				0

/* On EA and IAR boards, they use Micrel PHY.
   on ENG and KEIL boards, they use National PHY */
#define NATIONAL_PHY			1
#define MICREL_PHY				2

/* If USB device is used, CCO will be 288Mhz( divided by 6) or 384Mhz( divided by 8)
to get precise USB clock 48Mhz. If USB is not used, you set any clock you want
but make sure the divider of the CCO should be an even number. If you want to 
use USB, change "define USE_USB" from 0 to 1 */
 
#define	USE_USB					1


/* PLL Setting Table Matrix */
/* 	
	Main Osc.	CCLKCFG		Fcco		Fcclk 		M 	N
	12Mhz		29		300Mhz		10Mhz			24	1
	12Mhz		35		360Mhz		10Mhz			14	0					
	12Mhz		27		336Mhz		12Mhz			13	0		
	12Mhz		17		360Mhz		20Mhz			14	0
	12Mhz		13		336Mhz		24Mhz			13	0
	12Mhz		11		300Mhz		25Mhz			24	1   
	12Mhz		9		300Mhz		30Mhz			24	1
	12Mhz		11		360Mhz		30Mhz			14	0
	12Mhz		9		320Mhz		32Mhz			39	2
	12Mhz		9		350Mhz		35Mhz			174	11
	12Mhz		7		312Mhz		39Mhz			12	0 
	12Mhz		7		360Mhz		45Mhz			14	0  
	12Mhz		5		300Mhz		50Mhz			24	1
	12Mhz		5		312Mhz		52Mhz			12	0
	12Mhz		5		336Mhz		56Mhz			13	0				
	12Mhz		3		300Mhz		75Mhz			24	1
	12Mhz		3		312Mhz		78Mhz			12	0  
	12Mhz		3		320Mhz		80Mhz			39	2
	12Mhz		3		336Mhz		84Mhz			13	0 
*/

/* These are limited number of Fcco configuration for
USB communication as the CPU clock and USB clock shares
the same PLL. The USB clock needs to be multiple of
48Mhz. */
#if USE_USB		/* 1 is USB, 0 is non-USB related */  
	/* Fcck = 60Mhz, Fosc = 240Mhz, and USB 48Mhz */
    // fcc0=240MHz
// FIN the frequency of PLLCLKIN from the Clock Source Selection Multiplexer.
// FCCO the frequency of the SYSCLK (output of the PLL Current Controlled Oscillator)
// N PLL Pre-divider value from the NSEL bits in the PLLCFG register (PLLCFG NSEL field + 1). N is an integer from 1 through 32.
// M PLL Multiplier value from the MSEL bits in the PLLCFG register (PLLCFG MSEL field + 1). Not all potential values are supported. See below.
// FREF PLL internal reference frequency, FIN divided by N.    
//
// The PLL output frequency (when the PLL is both active and connected) is given by:
// FCCO = (2 × M × FIN) / N
// The PLL inputs and settings must meet the following:
// • FIN is in the range of 32 kHz to 50 MHz.
// • FCCO is in the range of 275 MHz to 550 MHz.
// The PLL equation can be solved for other PLL parameters:
// M = (FCCO × N) / (2 × FIN)
// N = (2 × M × FIN) / FCCO
// FIN = (FCCO × N) / (2 × M)
// 
// 
	#define PLL_MValue			5
	#define PLL_NValue			0
	#define CCLKDivValue		3
	#define USBCLKDivValue		4

	/* System configuration: Fosc, Fcclk, Fcco, Fpclk must be defined */
	/* PLL input Crystal frequence range 4KHz~20MHz. */
	#define Fosc	20000000
// pll output frequency
    #define Fcco    (2*(PLL_MValue+1)*Fosc)/(PLL_NValue+1)  //(2 × M × FIN) / N
//	#define Fcco	240000000

// System frequence,should be less than 80MHz. 
	#define Fcclk	(Fcco/(CCLKDivValue+1))
	//#define Fcclk	60000000
    
#else

	/* Fcck = 60Mhz, Fosc = 360Mhz, USB can't be divided into 48Mhz
	in this case, so USBCLKDivValue is not needed. */
	#define PLL_MValue			14
	#define PLL_NValue			0
	#define CCLKDivValue		5

	/* System configuration: Fosc, Fcclk, Fcco, Fpclk must be defined */
	/* PLL input Crystal frequence range 4KHz~20MHz. */
	#define Fosc	12000000
	/* System frequence,should be less than 72MHz. */
	#define Fcclk	60000000
	#define Fcco	360000000

#endif

#if Fosc!=20000000
    #error wrong frequency setting! only fosc has not the correct value
#endif    

/* APB clock frequence , must be 1/2/4 multiples of ( Fcclk/4 ). */
/* If USB is enabled, the minimum APB must be greater than 16Mhz */ 
#if USE_USB
	#define Fpclk	(Fcclk / 2)
#else
	#define Fpclk	(Fcclk / 2)
#endif

// PCLKSEL0:
// 1:0 PCLK_WDT Peripheral clock selection for WDT. 00
// 3:2 PCLK_TIMER0 Peripheral clock selection for TIMER0. 00
// 5:4 PCLK_TIMER1 Peripheral clock selection for TIMER1. 00
// 7:6 PCLK_UART0 Peripheral clock selection for UART0. 00
// 9:8 PCLK_UART1 Peripheral clock selection for UART1. 00
// 11:10 - Unused, always read as 0. 00
// 13:12 PCLK_PWM1 Peripheral clock selection for PWM1. 00
// 15:14 PCLK_I2C0 Peripheral clock selection for I2C0. 00
// 17:16 PCLK_SPI Peripheral clock selection for SPI. 00
// 19:18 PCLK_RTC[1] Peripheral clock selection for RTC. 00
// 21:20 PCLK_SSP1 Peripheral clock selection for SSP1. 00
// 23:22 PCLK_DAC Peripheral clock selection for DAC. 00
// 25:24 PCLK_ADC Peripheral clock selection for ADC. 00
// 27:26 PCLK_CAN1[2] Peripheral clock selection for CAN1. 00
// 29:28 PCLK_CAN2[2] Peripheral clock selection for CAN2. 00
// 31:30 PCLK_ACF[2]

// PCLKSEL1:
// 1:0 PCLK_BAT_RAM Peripheral clock selection for the battery supported RAM. 00
// 3:2 PCLK_GPIO Peripheral clock selection for GPIOs. 00
// 5:4 PCLK_PCB Peripheral clock selection for the Pin Connect block. 00
// 7:6 PCLK_I2C1 Peripheral clock selection for I2C1. 00
// 9:8 - Unused, always read as 0. 00
// 11:10 PCLK_SSP0 Peripheral clock selection for SSP0. 00
// 13:12 PCLK_TIMER2 Peripheral clock selection for TIMER2. 00
// 15:14 PCLK_TIMER3 Peripheral clock selection for TIMER3. 00
// 17:16 PCLK_UART2 Peripheral clock selection for UART2. 00
// 19:18 PCLK_UART3 Peripheral clock selection for UART3. 00
// 21:20 PCLK_I2C2 Peripheral clock selection for I2C2.
// 23:22 PCLK_I2S Peripheral clock selection for I2S. 00
// 25:24 PCLK_MCI Peripheral clock selection for MCI. 00
// 27:26 - Unused, always read as 0. 00
// 29:28 PCLK_SYSCON Peripheral clock selection for the System Control block. 00
// 31:30 - Unused, always read as 0.

// enumerazione di tutte le periferiche LPC
typedef enum{
// periferiche pertinenti a PCLKSEL0
	enumLPC_Peripheral_PCLK_WDT=0 ,
	enumLPC_Peripheral_PCLK_TIMER0 ,
	enumLPC_Peripheral_PCLK_TIMER1 ,
	enumLPC_Peripheral_PCLK_UART0 ,
	enumLPC_Peripheral_PCLK_UART1, 
	enumLPC_Peripheral_Unused_0,
	enumLPC_Peripheral_PCLK_PWM1 ,
	enumLPC_Peripheral_PCLK_I2C0 ,
	enumLPC_Peripheral_PCLK_SPI ,
	enumLPC_Peripheral_PCLK_RTC,
	enumLPC_Peripheral_PCLK_SSP1,
	enumLPC_Peripheral_PCLK_DAC ,
	enumLPC_Peripheral_PCLK_ADC ,
	enumLPC_Peripheral_PCLK_CAN1,
	enumLPC_Peripheral_PCLK_CAN2,
	enumLPC_Peripheral_PCLK_ACF,
// queste periferiche sono pertinenti a PCLKSEL1
	enumLPC_Peripheral_PCLK_BAT_RAM,
	enumLPC_Peripheral_PCLK_GPIO,
	enumLPC_Peripheral_PCLK_PCB,
	enumLPC_Peripheral_PCLK_I2C1,
	enumLPC_Peripheral_Unused_1,
	enumLPC_Peripheral_PCLK_SSP0,
	enumLPC_Peripheral_PCLK_TIMER2,
	enumLPC_Peripheral_PCLK_TIMER3,
	enumLPC_Peripheral_PCLK_UART2,
	enumLPC_Peripheral_PCLK_UART3,
	enumLPC_Peripheral_PCLK_I2C2,
	enumLPC_Peripheral_PCLK_I2S,
	enumLPC_Peripheral_PCLK_MCI,
	enumLPC_Peripheral_Unused_2,
	enumLPC_Peripheral_PCLK_SYSCON,
	enumLPC_Peripheral_Unused_3,
	enumLPC_Peripheral_numOf
}enumLPC_Peripheral;

// lettura del valore di pclk per la periferica
extern unsigned long ulGetPclk(enumLPC_Peripheral p);
// impostazione del valore di pclk per la periferica
// restituisce il valore di plck che è stato possibile impostare
extern unsigned long ulSetPclk(enumLPC_Peripheral p, unsigned long ulPclk_desired);

/******************************************************************************
** Function name:		TargetInit
**
** Descriptions:		Initialize the target board; it is called in a 
**				necessary place, change it as needed
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
extern void TargetInit(void);
extern void ConfigurePLL( void );
extern void TargetResetInit(void);

#ifdef __cplusplus
   }
#endif
 
#endif /* end __TARGET_H */
/******************************************************************************
**                            End Of File
******************************************************************************/
