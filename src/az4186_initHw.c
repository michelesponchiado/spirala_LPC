// modulo di gestione inizializzazione hardware, con utilit� sulle interrupt etc
#include <stdlib.h>
#include <string.h>
//#include <malloc.h>
#include <math.h> 
#include <ctype.h>

#include "LPC23xx.h"                        /* LPC23xx/24xx definitions */
#include "type.h"
#include "target.h"
#include "fpga_nxp.h"
#include "az4186_emc.h"
#include "az4186_dac.h"

// variabile su cui salvo il valore del vic
static unsigned int my_saved_vic,my_actual_vic;
// serve a calcolare il tempo trascorso
// viene incrementato ad ogni timer0 interrupt
extern volatile unsigned char ucCntTimerInterrupt;
// serve a calcolare il tempo trascorso
// viene incrementato ad ogni timer0 interrupt
// questa per� � una variabile intera...
extern volatile unsigned int uiCntTimerInterrupt;

unsigned disableIRQ(void){
	my_saved_vic=VICIntEnable;
    VICIntEnClr=VICIntEnable;
    VICIntEnClr=0;
    my_actual_vic=VICIntEnable;
    return my_saved_vic;
}

unsigned restoreIRQ(unsigned vic){
    VICIntEnClr=0;
    VICIntEnable=vic;
	my_saved_vic=VICIntEnable;
    my_actual_vic=VICIntEnable;
    return vic;
}

unsigned enableIRQ(void){
    VICIntEnClr=0;
    VICIntEnable=my_saved_vic;
    my_actual_vic=VICIntEnable;
    return VICIntEnable;
}

// inizializzazione dati e bss
void vInitDataAndBssSections(void){
// Michele: pu� essere che bss vada azzerata altrimenti non funziona il programma spiralatrice???
// e' fortemente consigliato, all'avvio del sistema, azzerare completamente la bss
// quindi NON definire la prossima define a meno che non si voglia incorrere in grossi rischi
// #define defDontResetWholeBss


// inizializzazione zona data:
// serve per inizializzare correttamente libreria sprintf altrimenti non funziona niente!!!

// accedo alle variabili definite in link time...
// e che definiscono la partenza della zona di flash da scaricare in ram
// e l'inizio e la fine della zona ram...
extern int _sidata;
extern int _sdata;
extern int _edata;
register unsigned int *pStartInitData_Read=(unsigned int *)&_sidata;
register unsigned int *pStartInitData_Write=(unsigned int *)&_sdata;
register unsigned int *pEndInitData_Write=(unsigned int *)&_edata;
    while(pStartInitData_Write<pEndInitData_Write){
        *pStartInitData_Write++=*pStartInitData_Read++;
    }
#ifdef defDontResetWholeBss
    // reset only the bss coming from the c library
	// theoretically unnecessary
    extern int _lib_bss_start;
    extern int _lib_bss_end;
    register unsigned int *pStartLibBss_Write=(unsigned int *)&_lib_bss_start;
    register unsigned int *pEndLibBss_Write=(unsigned int *)&_lib_bss_end;
    while(pStartLibBss_Write<pEndLibBss_Write){
        *pStartLibBss_Write++=0;
    }
#else    
    // reset bss to zero!!!
    extern int _sbss;
    extern int _ebss;
    register unsigned int *pStartBss_Write=(unsigned int *)&_sbss;
    register unsigned int *pEndBss_Write=(unsigned int *)&_ebss;
        while(pStartBss_Write<pEndBss_Write){
            *pStartBss_Write++=0;
        } 
#endif
}//void vInitDataAndBssSections(void)

// wait at least ul_wait_microseconds
void v_wait_microseconds(unsigned long ul_wait_microseconds){

    while(ul_wait_microseconds--){
        int i;
#if Fcclk==60000000
        i=10;
        while(i--)
#else
        i=10;
        while(i--)
#endif
            asm("nop");
    }
}//void v_wait_microseconds(unsigned long ul_wait_microseconds)


// inizializza l'hardware della scheda az4186
void vAZ4186_InitHwStuff(void){
    // set target configuration
    TargetResetInit();
    // initialize emc interface
    vEMC_az4186_fpga_init();
    // Make timer operate at correct speed
    // select cclk/2 as clock source for timer0
    PCLKSEL0 &= ~(0x3 << 2);
    PCLKSEL0 |= (0x1 << 2);
    PINSEL10 = 0;
    VICIntEnClr = 0xffffffff;
    VICVectAddr = 0;
    VICIntSelect = 0;   
    // init data and bss sections
    vInitDataAndBssSections();
// disable watch dog
//#pragma message "\n***watch dog is disabled***\n"
    WDMOD=0X00;
// watch dog feed
    WDFEED=0XAA;
    WDFEED=0X55;
//#pragma message "\n*************\nimpostare valore corretto del watch dog!!!\n**************\n"
	WDTC=0xffff;
    // inizializzazione dac
    v_dac_init();
}//void vInitHwStuff(void)

// use this routine to reset az4186 board
void v_reset_az4186(void){
    // disable ints from timer0, which feeds the watch dog...
    VICIntEnable&=~0x10;
//disableIRQ();
// enable watch dog
    WDMOD=0X03;
// low watch dog feed, 0xff is the minimum value which can be set
	WDTC=0xff;
    while(1){
        WDFEED=0xAA; // the write sequence 0xaa, x00 calls for an immediate reset!
        WDFEED=0x00;
        v_wait_microseconds(100);
    }
}
