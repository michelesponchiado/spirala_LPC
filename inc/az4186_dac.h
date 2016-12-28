#ifndef def_az4186_dac_h_included
#define def_az4186_dac_h_included

// enum to select dac range
typedef enum {
    enum_select_dac_portata_01=0,
    enum_select_dac_portata_1   ,
    enum_select_dac_portata_10  ,
    enum_select_dac_portata_100 ,
    enum_select_dac_portata_1000,
    enum_select_dac_portata_numof
}enum_select_dac_portata;


// impostazione dac portate
void vUpdateDacPortValue(enum_select_dac_portata select_dac_portata);

// lettura attuale impostazione dac portate
enum_select_dac_portata ui_read_dac_selected_portata(void);

// endless dac test: outputs a sawtooth waveform with circa 100ms period
void vTestDac(void);

//
// Function name:		v_dac_init
//
// Descriptions:		initialize DAC channel
//
// parameters:			None
// Returned value:		None
// 
//
void v_dac_init( void );

#endif //#ifndef def_az4186_dac_h_included