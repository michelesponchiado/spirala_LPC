/*****************************************************************************
 *   az4186_temperature_compensation.h:  header for temperature compensation module file for az4186 csm board
 *
 *   Copyright(C) 2012,CSM GROUP
 *   All rights reserved.
 *
 *   History
 *   2012.11.23  ver 1.00    Preliminary version, first Release
 *
******************************************************************************/
#ifndef az4186_temperature_compensation_h_included
#define az4186_temperature_compensation_h_included
// Il grafico fornito dal cliente sembra fornire il coefficiente di temperatura in funzione della temperatura in °C
// La variazione di resistenza in funzione della temperatura può essere approssimata linearmente attorno alla temperatura ambiente:
// R=R0*(1+alfa*deltaT)
// dove:
// * R= resistenza alla temperatura attuale [ohm]
// * R0= resistenza alla temperatura ambiente 20°C [ohm]
// * alfa= coefficiente di temperatura della resistività [1/K]
// * deltaT=T-T0=delta di temperatura rispetto alla temperatura ambiente di 20°C [K]
// * il termine (1+alfa*deltaT) è detto anche fattore di temperatura [adimensionale]

// Per la lega indicata nel disegno NiFe52, in letteratura si trovano valori di alfa circa 0.04-0.043
// Nel disegno che ci hanno fornito, sono forniti i fattori di temperatura a 10, 15, 20, 30 e 40°C; 
// da questi dati se desume che alfa è supposto costante nell'intervallo considerato, e pari a 0.0041 [1/K]


// init the temperature compensation ...
// check if enabled from machine configuration
void v_initialize_temperature_compensation(void);

// status machine: handles temperature compensation
// it would be a good idea to call this routine in the timer0 interrupt routine
void v_handle_temperature_compensation(void);

// set alfa factor, the temperature coefficient alfa used in formula: R[ohm]=R0[ohm @20°C]*(1+alfa[1/K]*(actual_temperature[°C]-20°C))
void v_set_alfa_factor(float f_alfa_factor_inv_kelvin);

// calculates temperature compensation
// safe to call it also when temperature compensation is disabled (by machine parameters), it handles all the stuff
int i_ro_compensate_temperature(unsigned int ui_actual_ro);

// returns the last permille value[... -2000 ... -1000 ... 0 ...+1000 ... +2000 ...] of temperature compensation
// 0 means no compensation
// +5 means the nominal ro value was increased by 0.5%
// -7 means the nominal ro value was decreased by 0.7%
// useful to log or to verify if something wrong is happening
int i_get_last_temperature_compensation_permille_used(void);

// returns 1 if the temperature comepnsation is valid and the reading is ok, else returns 0
// parameters: the last value of averaged temperature used for temperature compensation
unsigned int ui_get_averaged_compensation_temperature_celsius(float *pf_temperature_celsius);

// returns alfa factor actually used [1/K] to compensate for resistence variation with the temperature
float f_get_alfa_factor(void);

// stats about temperature compensation
typedef struct _TipoStructTempCompStats{
	int i_act_temperature_celsius_scaled;
	int i_act_compensation_permille;
	int i_num_of_timeout;
	int i_num_samples_acquired;
}TipoStructTempCompStats;
// gives some stats about temperature compensation:
// - actual temperature °C
// - actual compensation %
// - total number of timeouts/errors reading for temperature
// - total number of temperature samples acquired til now
void v_get_temperature_compensation_stats(TipoStructTempCompStats *p);
// returns the temperature from the normalized value saved in stats
float f_get_temperature_from_normalized_one(int i_normalized_one);
// returns 1 if the temperature compensation is available, else returns 0
// f_res_in is the resistance value to compensate for
// in *pf_res_out is copied the compensated value for the resistence
unsigned int ui_get_temperature_compensated_resistance(float f_res_in,float *pf_res_out);
// returns 1 if the temperature reading is in alarm status
unsigned int ui_alarm_reading_temperature(void);
// return 1 if warm up has finished
unsigned int ui_temperature_compensation_warmup_finished(void);
// returns 1 if the temperature compensation parameters are valid, else returns 0
unsigned int ui_temperature_compensation_valid(void);

#endif //#ifndef az4186_temperature_compensation_h_included
