/*****************************************************************************
 *   az4186_temperature_compensation.c:  temperature compensation module file for az4186 csm board
 *
 *   Copyright(C) 2012,CSM GROUP
 *   All rights reserved.
 *
 *   History
 *   2012.11.23  ver 1.00    Preliminary version, first Release
 *
******************************************************************************/
#include "LPC23xx.h"                        // LPC23xx/24xx definitions
#include "type.h"
#include "target.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fpga_nxp.h"
#include "az4186_emc.h"
#include "irq.h"
#include "az4186_can.h"
#include "lcd.h"
#include "CO_driver.h"
#include "CANopen.h"
#ifdef def_canopen_enable
    #include "spiext.h"
#endif
#include "az4186_temperature_compensation.h"
#include "spiwin.h"

// Il grafico fornito dal cliente sembra fornire il coefficiente di temperatura in funzione della temperatura in �C
// La variazione di resistenza in funzione della temperatura pu� essere approssimata linearmente attorno alla temperatura ambiente:
// R=R0*(1+alfa*deltaT)
// dove:
// * R= resistenza alla temperatura attuale [ohm]
// * R0= resistenza alla temperatura ambiente 20�C [ohm]
// * alfa= coefficiente di temperatura della resistivit� [1/K]
// * deltaT=T-T0=delta di temperatura rispetto alla temperatura ambiente di 20�C [K]
// * il termine (1+alfa*deltaT) � detto anche fattore di temperatura [adimensionale]

// Per la lega indicata nel disegno NiFe52, in letteratura si trovano valori di alfa circa 0.04-0.043
// Nel disegno che ci hanno fornito, sono forniti i fattori di temperatura a 10, 15, 20, 30 e 40�C; 
// da questi dati se desume che alfa � supposto costante nell'intervallo considerato, e pari a 0.0041 [1/K]

// temperature first filter duration [seconds]
#define def_duration_first_filter_temperature_seconds 1
// temperature first filter duration [T]
// each sample should 
#define def_duration_first_filter_temperature_T ((def_duration_first_filter_temperature_seconds*1000)/def_canbus_sampling_period_ms)
// temperature second filter duration [s]
#define def_duration_second_filter_temperature_seconds (30)
// temperature second filter duration [taps]
#define def_duration_second_filter_temperature_taps (def_duration_second_filter_temperature_seconds/def_duration_first_filter_temperature_seconds)
// number of bits used to increase resolution on temperature averaging
#define def_numbit_increase_resolution_temp_average 4
// structure for handling the temperature filtering
typedef struct _TipoStructTemperatureFirstFilter{
	int i_temperature_binary;
	unsigned int ui_num_samples;
	int i_accum_samples;
	int i_average_samples;
	unsigned int ui_samples_counter;
	unsigned int ui_living_counter;
	unsigned int ui_timeout_counter;
	unsigned int ui_timeout_alarm;
	unsigned int ui_timeout_volatile_counter;
}TipoStructTemperatureFirstFilter;

// structure for handling the temperature filtering
typedef struct _TipoStructTemperatureSecondFilter{
	unsigned int i_samples[def_duration_second_filter_temperature_taps];
	unsigned int ui_num_samples;
	unsigned int ui_idx_samples;
	unsigned int i_averaged_sample;
	unsigned int ui_valid_measure;
}TipoStructTemperatureSecondFilter;

typedef struct _TipoStructTemperatureCompensationParameters{
	float f_alfa_factor_inv_kelvin; // temperature coefficient [1/K], e.g. for NiFe52 is about 0.0041 [1/K]
	unsigned int ui_alfa_factor_mantissa_16; // 16bit normalized mantissa temperature coefficient [1/K], e.g. for NiFe52 is about...
	unsigned int ui_alfa_factor_exp_16; // 16bit normalized exponent temperature coefficient [1/K], e.g. for NiFe52 is about...
	unsigned int ui_valid; // are the compensation parameters valid???
}TipoStructTemperatureCompensationParameters;


// struct for handling the long duration temperature filter
typedef enum{
	enum_status_handle_temperature_filter_idle=0,
	enum_status_handle_temperature_filter_init,
	enum_status_handle_temperature_filter_init_wait,
	enum_status_handle_temperature_filter_update_first_init,
	enum_status_handle_temperature_filter_update_first_wait,
	enum_status_handle_temperature_filter_update_first,
	enum_status_handle_temperature_filter_update_second,
	enum_status_handle_temperature_filter_numof
}enum_status_handle_temperature_filter;

// structure for handling the temperature filtering
typedef struct _TipoStructTemperatureFilter{
	TipoStructTemperatureFirstFilter first_filter;
	TipoStructTemperatureSecondFilter second_filter;
	TipoStructTemperatureCompensationParameters compensation_params;
	enum_status_handle_temperature_filter status;
	TipoStruct_timeout timeout;//timeout_init(&timeout);if (uc_timeout_active(&timeout, ul_ms_to_wait))
	int i_last_compensation_used_permille;
}TipoStructTemperatureFilter;
// the variable containing the temperature filter
static TipoStructTemperatureFilter temperature_filter;
unsigned int ui_temperature_alarm_subcode;
unsigned int ui_first_time_alarm_temperature_measure;

// returns 1 if the temperature reading is in alarm status
unsigned int ui_alarm_reading_temperature(void){
	return temperature_filter.first_filter.ui_timeout_alarm;
}

// return 1 if warm up has finished
unsigned int ui_temperature_compensation_warmup_finished(void){
	// if canopen is in alarm, no problem with the warmup
	if (temperature_filter.first_filter.ui_timeout_alarm){
		return 1;
	}
	return temperature_filter.second_filter.ui_valid_measure;
}

// update the first temperature filter
unsigned int ui_update_first_temperature_filter(void){
	TipoStructTemperatureFirstFilter *pff;
	unsigned int ui_retcode;
	unsigned long ul_canbus_living_counter;
// init	
	pff=&temperature_filter.first_filter;
	ui_retcode=0;
	ul_canbus_living_counter=ul_get_canbus_living_counter();
// update	
// is the canbus living? is the temperature valid?
	if (  ( pff->ui_living_counter==ul_canbus_living_counter       )
	    ||(!ui_get_canopen_temperature(&pff->i_temperature_binary))
	   ){
		// timeout???
		if (++pff->ui_timeout_volatile_counter>=20*(def_canbus_sampling_period_ms/PeriodoIntMs)){
			pff->ui_timeout_volatile_counter=0;
			pff->ui_timeout_counter++;
			pff->ui_timeout_alarm=1;
			// sottocodice di allarme, la ui_get_canopen_temperature lo ha gia' eventualmente impostato
			if (pff->ui_living_counter==ul_canbus_living_counter){
				ui_temperature_alarm_subcode=enumStr20_FaultTemp_Timeout;
			}
			// I visualize the alrm:
			// the first time
			// if a program using temperature compensation is active
			if ((ui_first_time_alarm_temperature_measure==0)||(PrgRunning&&(macroIsCodiceProdotto_TemperatureCompensated(&hprg.theRunning)))){
				// Allarme: TEMPERATURA
				alarms |= ALR_TEMPERATURE;
				// E' effettivamente un allarme valido.
				alr_status=alr_status_alr;
				ui_first_time_alarm_temperature_measure=1;
			}
		}
	}
	else{
		// no alarm timeout
		pff->ui_timeout_alarm=0;
        pff->ui_timeout_volatile_counter=0;
		// update living counter!
		pff->ui_living_counter=ul_canbus_living_counter;
		// increment number of samples successfully got
		pff->ui_samples_counter++;
		// increment actual number of samples
		pff->ui_num_samples++;
		// accumulate samples
		pff->i_accum_samples+=pff->i_temperature_binary;
		// if num samples limit reached, calculate average value
		if (pff->ui_num_samples>=def_duration_first_filter_temperature_T){
			// do average calculation
			pff->i_average_samples=pff->i_accum_samples/pff->ui_num_samples;
			// reset samples number
			pff->ui_num_samples=0;
			// reset samples accumulator
			pff->i_accum_samples=0;
			// yes, we get a new value
			ui_retcode=1;
		}
	}
	return ui_retcode;
}//void v_update_first_temperature_filter(void)

// init the temperature compensation ...
// check if enabled from machine configuration
void v_initialize_temperature_compensation(void){
	memset(&temperature_filter,0,sizeof(temperature_filter));
//#warning auto alfa initialization
//    v_set_alfa_factor(0.0041);
}//void v_initialize_temperature_compensation(void)

// status machine: handles temperature compensation
// it would be a good idea to call this routine in the timer0 interrupt routine
void v_handle_temperature_compensation(void){
	if (!macParms.ucAbilitaCompensazioneTemperatura
	    ||(!macroIsCodiceProdotto_TemperatureCompensated(&hprg.theRunning))
	   ){
		return;
	}
	switch(temperature_filter.status){
		case enum_status_handle_temperature_filter_idle:
		default:
            temperature_filter.status=enum_status_handle_temperature_filter_init;
			break;
		case enum_status_handle_temperature_filter_init:
			// initialize first and second filter
			memset(&temperature_filter.first_filter,0,sizeof(temperature_filter.first_filter));
			memset(&temperature_filter.second_filter,0,sizeof(temperature_filter.second_filter));
			// initialize timeout: wait some seconds before starting compensation calculation
			timeout_init(&temperature_filter.timeout);
			// change status
			temperature_filter.status=enum_status_handle_temperature_filter_init_wait;
			break;
		case enum_status_handle_temperature_filter_init_wait:
			// is the timeout triggered? then change status
			// wait some seconds until canbus intializes...
			if (uc_timeout_active(&temperature_filter.timeout, 20000)){
				temperature_filter.status=enum_status_handle_temperature_filter_update_first_init;
				break;
			}
			break;
		case enum_status_handle_temperature_filter_update_first_init:
			// start timeout wait new sample
			timeout_init(&temperature_filter.timeout);
			// change status
			temperature_filter.status=enum_status_handle_temperature_filter_update_first_wait;
			break;
		case enum_status_handle_temperature_filter_update_first_wait:
			// if timeout triggered, goto update status
			if (uc_timeout_active(&temperature_filter.timeout, PeriodoIntMs)){
				temperature_filter.status=enum_status_handle_temperature_filter_update_first;
				break;
			}
			break;
		case enum_status_handle_temperature_filter_update_first:
			// if I get a new sample, use it for average calculation
			if (ui_update_first_temperature_filter()){
				temperature_filter.status=enum_status_handle_temperature_filter_update_second;
				break;
			}
			// else wait for the next loop
			else{
				temperature_filter.status=enum_status_handle_temperature_filter_update_first_init;
			}
			break;
		case enum_status_handle_temperature_filter_update_second:
			{
				TipoStructTemperatureSecondFilter *psf;
				// first of all, prepare the new status
				temperature_filter.status=enum_status_handle_temperature_filter_update_first_init;
				// pointer to second filter structure
				psf=&temperature_filter.second_filter;
				// save the new sample
				psf->i_samples[psf->ui_idx_samples]=temperature_filter.first_filter.i_average_samples;
				if (++psf->ui_idx_samples>=sizeof(psf->i_samples)/sizeof(psf->i_samples[0])){
					psf->ui_idx_samples=0;
				}
				
				// update num of samples, if limit reached, calculate the second averaged value
				if (++psf->ui_num_samples>=sizeof(psf->i_samples)/sizeof(psf->i_samples[0])){
					unsigned int i;
					long ul;
					ul=0;
					i=0;
					while (i<psf->ui_num_samples){
						ul+=psf->i_samples[i++];
					}
					// calculate averaged value
					// aumentare la risoluzione con fattore di scala, ad esempio 4 bit *16 in modo da passare da 0.1 a 0.00625
					ul*=(1<<def_numbit_increase_resolution_temp_average);
					// calculate the average
					psf->i_averaged_sample=ul/(sizeof(psf->i_samples)/sizeof(psf->i_samples[0]));
					// reset number of samples to the maximum
					psf->ui_num_samples=sizeof(psf->i_samples)/sizeof(psf->i_samples[0]);
					// set a valid sample acquired!
					psf->ui_valid_measure=1;
				}
				break;
			}
	}
}//void v_handle_temperature_compensation(void)

// set alfa factor, the temperature coefficient alfa used in formula: R=R0*(1+alfa*(t-20�C))
// the alfa factor should be espressed in SI unit 1/K, typically 0.004 or so
void v_set_alfa_factor(float f_alfa_factor_inv_kelvin){
	unsigned int i;
	unsigned int ui_found;
	unsigned long ul_factor;
	unsigned long ul_exp;
	unsigned short int us_normalized_mantissa;
	temperature_filter.compensation_params.f_alfa_factor_inv_kelvin=f_alfa_factor_inv_kelvin;
	ul_factor=1;
	ul_exp=0;
	ui_found=0;
	// loop max 24 times...
	for (i=0;i<24;i++){
		us_normalized_mantissa=f_alfa_factor_inv_kelvin*ul_factor;
		if (us_normalized_mantissa&0x8000){
			ui_found=1;
			break;
		}
		ul_exp++;
		ul_factor*=2;
	}
	// if found, calculate mantissa and exponent approximating alfa with 16 bit mantissa resolution: alfa = mantissa/(2^exp)
	if (ui_found){
		temperature_filter.compensation_params.ui_alfa_factor_mantissa_16=us_normalized_mantissa;
		temperature_filter.compensation_params.ui_alfa_factor_exp_16=ul_exp;
		// the compensation parameters are valid!!!
		temperature_filter.compensation_params.ui_valid=1;
	}
	else{
		temperature_filter.compensation_params.ui_alfa_factor_mantissa_16=1;
		temperature_filter.compensation_params.ui_alfa_factor_exp_16=0;
		// the compensation parameters aren't valid!!!
		temperature_filter.compensation_params.ui_valid=0;
	}
}

// returns 1 if the temperature compensation parameters are valid, else returns 0
unsigned int ui_temperature_compensation_valid(void){
	return temperature_filter.compensation_params.ui_valid;
}

// calculates temperature compensation
// safe to call it also when temperature compensation is disabled (by machine parameters), it handles all the stuff
int i_ro_compensate_temperature(unsigned int ui_actual_ro){
	int i_compensated_ro;
	// if compensation not active or invalid, return same value as input
	if (  (!macParms.ucAbilitaCompensazioneTemperatura)
 	     ||(!macroIsCodiceProdotto_TemperatureCompensated(&hprg.theRunning))
	     ||temperature_filter.first_filter.ui_timeout_alarm
		 ||!temperature_filter.second_filter.ui_valid_measure
		 ||!temperature_filter.compensation_params.ui_valid
		 ){
		i_compensated_ro=ui_actual_ro;
	}
	else{
		long int i_delta_t;
		long int li_norm_factor;
		long int li_denominator;
		long long int lli_new_ro;
// R=R0*(1+alfa*deltat) --> R0=R/(1+alfa*deltat)
// R0=R/(1+(mantissa_alfa/(1<<exp_alfa))*(mantissa_deltat/(1<<exp_deltat)))
// R0=(R*(1<<(exp_alfa+exp_deltat)))/((1<<(exp_alfa+exp_deltat))+mantissa_alfa*mantissa_deltat)
// ro_0=(ro*(1<<(exp_alfa+exp_deltat)))/((1<<(exp_alfa+exp_deltat))+mantissa_alfa*mantissa_deltat)
		// delta T in 0.1�C units; 200-->20.0�C
		i_delta_t =temperature_filter.second_filter.i_averaged_sample;
		// subtract 20�C @ 0.1�C resolution and with scaling factor
		i_delta_t-=(200L*(1<<def_numbit_increase_resolution_temp_average));
		// normalization factor, 10*(1<<(exp_alfa+exp_deltat))
        // I take care also of the temperature resolution 0.1deg
		li_norm_factor=10*(1<<(def_numbit_increase_resolution_temp_average+temperature_filter.compensation_params.ui_alfa_factor_exp_16));
		// the scaled denominator
		li_denominator=li_norm_factor+temperature_filter.compensation_params.ui_alfa_factor_mantissa_16*i_delta_t;
		// calculate delta ro on 64 bit variable
		lli_new_ro  =ui_actual_ro;
		lli_new_ro *=li_norm_factor;
		lli_new_ro /=li_denominator;
		i_compensated_ro=lli_new_ro;
	}
	temperature_filter.i_last_compensation_used_permille=((i_compensated_ro-(int)ui_actual_ro)*1000)/(int)ui_actual_ro;
	return i_compensated_ro;
}//int i_ro_compensate_temperature(unsigned int ui_actual_ro)

// returns the last permille value[... -2000 ... -1000 ... 0 ...+1000 ... +2000 ...] of temperature compensation
// 0 means no compensation
// +5 means the nominal ro value was increased by 0.5%
// -7 means the nominal ro value was decreased by 0.7%
// useful to log or to verify if something wrong is happening
int i_get_last_temperature_compensation_permille_used(void){
	return temperature_filter.i_last_compensation_used_permille;
}

// returns 1 if the temperature comepnsation is valid and the reading is ok, else returns 0
// parameters: the last value of averaged temperature used for temperature compensation
unsigned int ui_get_averaged_compensation_temperature_celsius(float *pf_temperature_celsius){
	float f_temperature;
	if (  (!macParms.ucAbilitaCompensazioneTemperatura)
	     ||(!macroIsCodiceProdotto_TemperatureCompensated(&hprg.theRunning))
	     ||temperature_filter.first_filter.ui_timeout_alarm
		 ||!temperature_filter.second_filter.ui_valid_measure
		 ){
		return 0;
	}
	f_temperature=temperature_filter.second_filter.i_averaged_sample;
	f_temperature*=(1.0/(1<<def_numbit_increase_resolution_temp_average));	
	f_temperature*=0.1;
	*pf_temperature_celsius=f_temperature;
	return 1;
}

// returns the temperature from the normalized value saved in stats
float f_get_temperature_from_normalized_one(int i_normalized_one){
	float f_temperature;
	f_temperature=i_normalized_one;
	f_temperature*=(1.0/(1<<def_numbit_increase_resolution_temp_average));	
	f_temperature*=0.1;
	return f_temperature;
}
// returns alfa factor actually used [1/K] to compensate for resistence variation with the temperature
float f_get_alfa_factor(void){
	return temperature_filter.compensation_params.f_alfa_factor_inv_kelvin;
}

// gives some stats about temperature compensation:
// - actual temperature �C
// - actual compensation %
// - total number of timeouts/errors reading for temperature
// - total number of temperature samples acquired til now
void v_get_temperature_compensation_stats(TipoStructTempCompStats *p){
	// if no vALID SAMPLES TIL NOW, RESET ALL
	if (!temperature_filter.second_filter.ui_valid_measure){
		memset(p,0,sizeof(*p));
	}
	else{
		p->i_act_temperature_celsius_scaled=temperature_filter.second_filter.i_averaged_sample;
		p->i_act_compensation_permille	=i_get_last_temperature_compensation_permille_used();
		p->i_num_of_timeout			=temperature_filter.first_filter.ui_timeout_counter;
		p->i_num_samples_acquired	=temperature_filter.first_filter.ui_samples_counter;
	}
}//void v_get_temperature_compensation_stats(TipoStructTempCompStats *p)

// returns 1 if the temperature compensation is available, else returns 0
// f_res_in is the resistance value to compensate for
// in *pf_res_out is copied the compensated value for the resistence
unsigned int ui_get_temperature_compensated_resistance(float f_res_in,float *pf_res_out){
	float f_temp_celsius;
	float f_alfa;
	float f_resistenza_compensata;
	if (!ui_get_averaged_compensation_temperature_celsius(&f_temp_celsius)){
		return 0;
	}
	f_alfa=f_get_alfa_factor();
	f_resistenza_compensata=f_res_in/(1+f_alfa*(f_temp_celsius-20));
	*pf_res_out=f_resistenza_compensata;
	return 1;
}
