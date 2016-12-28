#include "LPC23xx.h"                        /* LPC23xx/24xx definitions */
#include "type.h"
#include "target.h"
#include "fpga_nxp.h"
#include "az4186_dac.h"

// num of bits on lpc2388 dac
#define def_num_bits_fs_dac 10
// full scale of lpc2388 dac
#define def_fs_dac ((1<<def_num_bits_fs_dac)-1)
// bit that select between fast output update or low current output update
#define def_dac_output_update_fast (0<<16)
#define def_dac_output_update_low_current (1<<16)



static enum_select_dac_portata last_selected_portata;

// le portate si selezionano impostando 0%,25%,45%, 65% della portata max
static const unsigned int ui_dac_values[enum_select_dac_portata_numof]={
        ( 0L*def_fs_dac)/100
       ,(25L*def_fs_dac)/100
       ,(45L*def_fs_dac)/100
       ,(65L*def_fs_dac)/100
       ,(85L*def_fs_dac)/100
    };

// impostazione dac portate
void vUpdateDacPortValue(enum_select_dac_portata select_dac_portata){
    unsigned long ul_dac_value;
    if(select_dac_portata>=enum_select_dac_portata_numof){
        select_dac_portata=0;
    }
    last_selected_portata=select_dac_portata;
// set the dac value; see lpc23xx user manual page 604
    ul_dac_value=(ui_dac_values[select_dac_portata]<<6)|def_dac_output_update_low_current;
// write cmmand to dac, so the analog output will be updated
    DACR=ul_dac_value;
  
}//void vUpdateDacPortValue(enum_select_dac_portata select_dac_portata)


// impostazione immediata valore su dac lpc2388
void v_dac_set_output_value(unsigned int ui_value){
    unsigned long ul_dac_value;
// set the dac value
    ul_dac_value=ui_value;
    if (ul_dac_value>def_fs_dac)
        ul_dac_value=0;
// set the dac value; see lpc23xx user manual page 604
    ul_dac_value=(ui_value<<6)|def_dac_output_update_low_current;
// write cmmand to dac, so the analog output will be updated
    DACR=ul_dac_value;
}//void v_dac_set_output_value(unsigned int ui_value)

// endless dac test: outputs a sawtooth waveform with circa 100ms period
void vTestDac(void){
    #define def_sawtooth_period_ms 100
    unsigned int ui_value;
    ui_value=0;
    while(1){
        v_dac_set_output_value(ui_value);
        v_wait_microseconds((def_sawtooth_period_ms*1000)/def_fs_dac);
        if (++ui_value>def_fs_dac)
            ui_value=0;
    }
}

// impostazione dac portate
enum_select_dac_portata ui_read_dac_selected_portata(void){
    return last_selected_portata;
}


//
// Function name:		v_dac_init
//
// Descriptions:		initialize DAC channel
//
// parameters:			None
// Returned value:		None
// 
//
void v_dac_init( void )
{
    unsigned long ul_pinsel1;
    ul_pinsel1=PINSEL1;
    ul_pinsel1&=~((1<<20)|(1<<21));// reset b20_b21 pair
    ul_pinsel1|= ((0<<20)|(1<<21));// set b20_b21 pair to 10--> select pin p0.26 to dac output
    // setup the related pin to DAC output 
    PINSEL1 = ul_pinsel1;	// set p0.26 to DAC output
    last_selected_portata=enum_select_dac_portata_01;
    vUpdateDacPortValue(last_selected_portata);
}
