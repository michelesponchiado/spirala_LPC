#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <ctype.h>

#include "LPC23xx.h"                        /* LPC23xx/24xx definitions */
#include "type.h"
#include "target.h"
#include "fpga_nxp.h"

#include "spityp.h"
#include "spicost.h"
#include "spiext.h"
#include "spifmt.h"
#include "az4186_dac.h"
#ifdef def_canopen_enable
    #include "az4186_can.h"
#endif
#include "az4186_temperature_compensation.h"


unsigned char outDigMis_Hi;
unsigned char uc_requesting_i2c_bus;
// variables for temperature compensation
int i_compensated_ro;
volatile unsigned long ul_counter_interrupt_timer0;
float f_delay_wait_spacer_in_precut_position_pulses;

typedef enum{
	enum_status_wait_one_coil_idle=0,
	enum_status_wait_one_coil_wait,
	enum_status_wait_one_coil_ok,
	enum_status_wait_one_coil_numof
}enum_status_wait_one_coil;

typedef struct _TipoHandleWaitOneCoil{
	enum_status_wait_one_coil status;
	unsigned int base_len;
	unsigned int ui_base_timeout;
}TipoHandleWaitOneCoil;
TipoHandleWaitOneCoil handleWaitOneCoil;

void v_init_handle_wait_one_coil(void){
	memset(&handleWaitOneCoil,0,sizeof(handleWaitOneCoil));
}
unsigned int ui_handle_wait_one_coil_ok(void){
	if (handleWaitOneCoil.status==enum_status_wait_one_coil_ok){
		return 1;
	}
	return 0;
}
void v_handle_wait_one_coil(void){
	switch(handleWaitOneCoil.status){
		case enum_status_wait_one_coil_idle:
		default:
			handleWaitOneCoil.status=enum_status_wait_one_coil_wait;
			handleWaitOneCoil.base_len=actLenImp;
			handleWaitOneCoil.ui_base_timeout=0;
			break;
		case enum_status_wait_one_coil_wait:
			// timeout??? 500ms timeout by now
			if (++handleWaitOneCoil.ui_base_timeout*PeriodoIntMs>=500){
				handleWaitOneCoil.status=enum_status_wait_one_coil_ok;
				break;
			}
			if (handleWaitOneCoil.base_len>actLenImp){
				handleWaitOneCoil.status=enum_status_wait_one_coil_ok;
				break;
			}
			if (actLenImp-handleWaitOneCoil.base_len>=f_delay_wait_spacer_in_precut_position_pulses){
				handleWaitOneCoil.status=enum_status_wait_one_coil_ok;
				break;
			}
			break;
		case enum_status_wait_one_coil_ok:
			break;
	}
}



void timeout_init(TipoStruct_timeout *p){
	p->uc_cnt_interrupt=ucCntInterrupt;
	p->uc_prev_cnt_interrupt=p->uc_cnt_interrupt;
	p->ui_time_elapsed_cnt=0;
}//void timeout_init(TipoStruct_timeout *p)

unsigned char uc_timeout_active(TipoStruct_timeout *p, unsigned int ui_timeout_ms){
	// verifico se il tempo trascorso � eccessivo
	p->uc_cnt_interrupt=ucCntInterrupt;
    // se non � cambiata la time unit, timeout non scaduto....
	if (p->uc_prev_cnt_interrupt==p->uc_cnt_interrupt){
        return 0;
    }
	// gestisci ricircolo
	if (p->uc_prev_cnt_interrupt>p->uc_cnt_interrupt){
		p->ui_time_elapsed_cnt+=256;
		p->ui_time_elapsed_cnt-=(p->uc_prev_cnt_interrupt-p->uc_cnt_interrupt)&0xFF;
	}
	else{
		p->ui_time_elapsed_cnt+=(p->uc_cnt_interrupt-p->uc_prev_cnt_interrupt)&0xFF;
	}
	p->uc_prev_cnt_interrupt=p->uc_cnt_interrupt;
	// timeout scaduto???
	if (p->ui_time_elapsed_cnt*PeriodoIntMs>=ui_timeout_ms){
		return 1;
	}
	// timeout non scaduto
	return 0;
}//unsigned char uc_timeout_active(TipoStruct_timeout *p, unsigned int ui_timeout_ms)



void mystrcpy(char *dst, char *src, unsigned int uiMaxchar2copy){
	while(*src&&uiMaxchar2copy--){
		*dst++=*src++;
	}
	*dst=0;
}

// reinizializzazione dello stato di pilotaggio della bandella, da chiamare in interrupt!!!!
// se chiamo una funzione che sta in un altro banco mentre sono in interrupt, succedono dei casini incredbili!!!!!
void vCommandSwitchBandella_Interrupt(void){
extern xdata unsigned char ucReinitStatusBandella;
	ucReinitStatusBandella=1;
}


// massima variazione nella lettura encoder tra due successive interrupt
#define defMaxDeltaEncoder 10000
// timeout operazione lama di taglio: max 5secondi per eseguire taglio/pretaglio
#define defTimeoutOperazioneLamaMs 5000U
// solo per simulazione, accorcio timeout a 250ms
//#define defTimeoutOperazioneLamaMs 250U


// Variabili che sarebbe bene mettere in data ram, se possibile.
data unsigned long roPrec;          /* 4 byte */
data unsigned char numSampleRo;
data unsigned long AccumRo;
data unsigned long ro;
data unsigned long testRo;
unsigned long testRo_verify_tolerance;
extern xdat unsigned long ulMaxTrueRo;
extern xdat unsigned long ulMinTrueRo;
extern xdat unsigned char ucMaxBadSpezzConsec;
extern xdat unsigned char ucActBadSpezzConsec;

extern xdat unsigned char runningCommessa;
extern unsigned char ucSendCommandPosTaglioMax519_Interrupt(unsigned char out1);

// array che contiene le conversioni dai canali a/d, lette in interrupt, che fpga media su 128 elementi...
xdata unsigned int uiResultConvAd_Interrupt[8];
unsigned char uc_reinit_ro_with_temperature_compensation;
// set to 1 when a precut command should be enabled at end of separation
unsigned int ui_requested_precut_after_spacing;


// usare questa define per abilitare il debug della lettura dei canali ad da fpga
//#define defDebugLetturaAdFromFpga

#ifdef defDebugLetturaAdFromFpga
	xdata unsigned char ucNumOfReadAutoAD_var;
	xdata unsigned char ucLastChanAutoAD_low_var;
	xdata unsigned char ucLastChanAutoAD_hi_var;
	xdata unsigned char ucLastChanFltAutoAD_low_var;
	xdata unsigned char ucLastChanFltAutoAD_hi_var;
	xdata unsigned char ucLastChanRamAutoAD_low_var;
	xdata unsigned char ucLastChanRamAutoAD_hi_var;
	xdata unsigned char ucLastChanAccum6AutoAD_low_var;
	xdata unsigned char ucLastChanAccum6AutoAD_hi_var;
	xdata unsigned char ucAutoAD_NumOfLatch_var;
#endif
	xdata unsigned char ucAutoAD_NumOfLatch_var;

#ifdef WriteSpezzoni
	xdat TipoInfotest *pInfoTestMis;
#endif

unsigned long nimpu;
unsigned char actDynPortata;
unsigned char CounterMisDig;
unsigned long lastRo,lastRoNonCompensato;

TipoStructTempoPerResistenza xdata TPR;

TARATURA *pTaratura;
// contatore che viene incrementato ad ogni interrupt
volatile unsigned char ucCntInterrupt;
// contatore che viene incrementato ad ogni interrupt
volatile unsigned int uiCntInterrupt;
// valore misura dinamica letto durante interrupt
unsigned int data uiLetturaMisuraDinamica_Interrupt;
// contatore che indica quante volte la misura dinamica � stata letta in interrupt
unsigned char data uiNumLettureMisuraDinamica_Interrupt;

// log della produzione
TipoStructLog production_log;
// puntatore per semplificare la memorizzazione del log della produzione...
TipoRecordLog *plog;


// per simulare l'encoder...
//#define defSimulaEncoder
#ifdef defSimulaEncoder
	#warning
    #warning ******************************
    #warning ******************************
    #warning INGRESSO ENCODER PER MISURA DINAMICA SIMULATO
    #warning ******************************
    #warning ******************************
	#warning
#endif

// test simulazione ingresso misura dinamica
//#define defSimulaIngressoMisuraDinamica
#ifdef defSimulaIngressoMisuraDinamica
	#warning
    #warning ******************************
    #warning ******************************
    #warning INGRESSO MISURA DINAMICA SIMULATO
    #warning ******************************
    #warning ******************************
	#warning
#endif

xdata unsigned long ulContaMezzoGiro;
unsigned char xdata ucMisuraDinamicaValidaSimulazione;

#ifdef defSimulaIngressoMisuraDinamica
	// la misura dinamica � valida circa 20�/180� quindi circa 14 volte su 128
	#define defMisuraDinamicaValida (ucMisuraDinamicaValidaSimulazione)
	#define defMisuraDinamicaNonValida (!ucMisuraDinamicaValidaSimulazione)
#else
	#define defMisuraDinamicaNonValida (InpDig & IDG_MIS_VALID1)
	#define defMisuraDinamicaValida (!defMisuraDinamicaNonValida)
#endif //#ifdef defSimulaIngressoMisuraDinamica




xdata int iArrayConvAD7327[8];
xdata float fArrayConvAD7327_Volts[8];

typedef struct _TipoStructRangeAD7327{
	float fMinVolt;		// valore minimo del range [V]
	float fMaxVolt;		// valore massimo del range [V]
	unsigned int uiVinAB;	// bit VinA_VinB
	float fm;
}TipoStructRangeAD7327;
typedef enum {
				enumRangeAD7372_bipolar10V,
				enumRangeAD7372_bipolar5V,
				enumRangeAD7372_bipolar2_5V,
				enumRangeAD7372_unipolar10V,
				enumRangeAD7372_numOf
			 }enumRangeAD7327;

code TipoStructRangeAD7327 rangeAD7327[]=
		{
			{ -10, +10,0,20.0/defMaxValueAD7327},
			{  -5,  +5,1,10.0/defMaxValueAD7327},
			{-2.5,+2.5,2, 5.0/defMaxValueAD7327},
			{   0, +10,3,10.0/defMaxValueAD7327}
		};

typedef enum {
				enumChannelsAD7372_MisuraStatica=0,
				enumChannelsAD7372_MisuraDinamica=1,
				enumChannelsAD7372_Mandrino=4,
				enumChannelsAD7372_Assorbimento1=5,
				enumChannelsAD7372_Assorbimento2=7,
				enumChannelsAD7372_Ruota1=6,
				enumChannelsAD7372_Ruota2=3,
				enumChannelsAD7372_Pretaglio=2,
				enumChannelsAD7372_numOf
			 }enumChannelsAD7327;

typedef enum {
				enumADmeasure_MisuraStatica=0,
				enumADmeasure_MisuraDinamica,
				enumADmeasure_Mandrino,
				enumADmeasure_Assorbimento1,
				enumADmeasure_Assorbimento2,
				enumADmeasure_Ruota1,
				enumADmeasure_Ruota2,
				enumADmeasure_Pretaglio,
				enumADmeasure_numOf
			 }enumADmeasure;

// range dei vari canali a/d
code enumRangeAD7327 rangesADmeasure[8]={
	enumRangeAD7372_unipolar10V,	//enumChannelsAD7372_MisuraStatica
	enumRangeAD7372_unipolar10V,	//enumADmeasure_MisuraDinamica
	enumRangeAD7372_unipolar10V,	//enumADmeasure_Mandrino
	enumRangeAD7372_bipolar5V,	//enumADmeasure_Assorbimento1
	enumRangeAD7372_bipolar5V,	//enumADmeasure_Assorbimento2
	enumRangeAD7372_unipolar10V,	//enumADmeasure_Ruota1
	enumRangeAD7372_unipolar10V,	//enumADmeasure_Ruota2
	enumRangeAD7372_unipolar10V		//enumADmeasure_Pretaglio
};

// tabella che riceve in ingresso un canale enumADmeasure
// e restituisce l'indice nell'array dei canali a/d
code unsigned char ucTableADmeas2ADch[8]={
	enumChannelsAD7372_MisuraStatica,
	enumChannelsAD7372_MisuraDinamica,
	enumChannelsAD7372_Mandrino,
	enumChannelsAD7372_Assorbimento1,
	enumChannelsAD7372_Assorbimento2,
	enumChannelsAD7372_Ruota1,
	enumChannelsAD7372_Ruota2,
	enumChannelsAD7372_Pretaglio
};

// tabella che riceve in ingresso un canale a/d
// e restituisce l'indice nell'array enumADmeasure
code unsigned char ucTableADch2ADmeas[8]={
	enumADmeasure_MisuraStatica,
	enumADmeasure_MisuraDinamica,
	enumADmeasure_Pretaglio,
	enumADmeasure_Ruota2,
	enumADmeasure_Mandrino,
	enumADmeasure_Assorbimento1,
	enumADmeasure_Ruota1,
	enumADmeasure_Assorbimento2,
};


void vInitAD7327(void){
	xdata unsigned int uiCode2Program;
		// disabilito acquisizione automatica...
		ucEnableAutoAD=0;
		// attendo che passi un po' di tempo per resettare in fpga la gestione ad7327...
		v_wait_microseconds(500);


	// attendo spi libera...
		while(ucReadBusy_WriteLatch){
		}
	// stop the sequence...
		ucLowByteAD7327=0x00;
		ucHiByteAD7327=0x80;
		// attendi fine operazione...
		while(ucReadBusy_WriteLatch){
		}
	// scrivi 0xA000 --> imposta range canali 0..3 a +/-10v
	// scrivi 0xAAA0 --> imposta range canali 0..3 a +/-5v
	//  15     14      13     12    11    10    9     8     7     6     5    4 3 2 1 0
	// write regsel1 regsel2 vin0a vin0b vin1a vin1b vin2a vin2b vin3a vin3b 0 0 0 0 0
		uiCode2Program=  0xA000 // write=1, regsel=01 --> 1010
						|(rangeAD7327[rangesADmeasure[ucTableADch2ADmeas[0]]].uiVinAB<<11)
						|(rangeAD7327[rangesADmeasure[ucTableADch2ADmeas[1]]].uiVinAB<<9)
						|(rangeAD7327[rangesADmeasure[ucTableADch2ADmeas[2]]].uiVinAB<<7)
						|(rangeAD7327[rangesADmeasure[ucTableADch2ADmeas[3]]].uiVinAB<<5)
						;
		ucLowByteAD7327=uiCode2Program&0xFF;
		ucHiByteAD7327=uiCode2Program>>8;
		// attendi fine operazione...
		while(ucReadBusy_WriteLatch){
		}
	// scrivi 0xC000 --> imposta range canali 4..7 a +/-10v
	// scrivi 0xCAA0 --> imposta range canali 4..7 a +/-5v
	//  15     14      13     12    11    10    9     8     7     6     5    4 3 2 1 0
	// write regsel1 regsel2 vin4a vin4b vin5a vin5b vin6a vin6b vin7a vin7b 0 0 0 0 0
		uiCode2Program=  0xC000		// write=1, regsel=10 -->1100
						|(rangeAD7327[rangesADmeasure[ucTableADch2ADmeas[4]]].uiVinAB<<11)
						|(rangeAD7327[rangesADmeasure[ucTableADch2ADmeas[5]]].uiVinAB<<9)
						|(rangeAD7327[rangesADmeasure[ucTableADch2ADmeas[6]]].uiVinAB<<7)
						|(rangeAD7327[rangesADmeasure[ucTableADch2ADmeas[7]]].uiVinAB<<5)
						;
		ucLowByteAD7327=uiCode2Program&0xFF;
		ucHiByteAD7327=uiCode2Program>>8;
		// attendi fine operazione...
		while(ucReadBusy_WriteLatch){
		}
	// scrivo sul sequence register, anche se non usato, che tutti i canali sono abilitati...
		ucLowByteAD7327=0xE0;
		ucHiByteAD7327=0xFF;
		// attendi fine operazione...
		while(ucReadBusy_WriteLatch){
		}

	// scrivi 0x8418 --> imposta lettura da canali 0..1
	// scrivi 0x9c18 --> imposta lettura da canali 0..7
	// scrivi 0x9c38 --> imposta lettura da canali 0..7 in modo binario secco
	// ctrl register
	// write regsel1 regsel2 add2 add1 add0 mode1 mode0 pm1 pm0 coding ref seq1 seq2 zero 0
	// write=1
	// regsel1 e 2=0 per selezionare ctrl register
	// add2..0 -->111 per convertire canali da 0 a 7
	// mode1..0 -->00 per avere canali single-ended
	// pm1..0 -->00 per avere full power
	// coding=1--> binario NON complemento a 2
	// ref=1 per usare internal reference
	// seq1..2 -->10 per avere conversione da 0..add2..0-->0..7
	// seq1..2 -->01 per avere conversione specificata da sequence register
	// zero-->0
	//  15     14      13     12  |  11    10    9     8  |   7     6     5    4  |  3    2   1  0
	// write regsel1 regsel2 add2 | add1  add0 mode1 mode0|  pm1   pm0  coding ref| seq1 seq2 0  0
	//  1     0        0      1   |  1      1    0    0   |   0     0     1     1 |  1    0   0  0
	//           9                         c                              3                 8
		// se si usa sequence register...
	//  1     0        0      1   |  1      1    0    0   |   0     0     1     1 |  0    1   0  0
	//           9                         c                              3                 4
		// se si usa c2...
	//  1     0        0      1   |  1      1    0    0   |   0     0     0     1 |  0    1   0  0
	//           9                         c                              1                 4
		ucLowByteAD7327=0x34;
		ucHiByteAD7327=0x9c;
		// attendi fine operazione...
		while(ucReadBusy_WriteLatch){
		}
		// riabilito acquisizione automatica...
		ucEnableAutoAD=1;
		// attendo che passi un po' di tempo per resettare in fpga la gestione ad7327...
		v_wait_microseconds(500);
}


xdata TipoSensoreTaglio sensoreTaglio;
#if 0
	unsigned int uiGetSensoreTaglio_NumRead(void){
		return sensoreTaglio.uiNumRead;
	}

	unsigned int uiGetSensoreTaglio_Min(void){
		return sensoreTaglio.uiMin_cur;
	}

	unsigned int uiGetSensoreTaglio_Max(void){
		return sensoreTaglio.uiMax_cur;
	}
#endif

void vHaltReadCycleSensoreTaglio(void){
	sensoreTaglio.ucEnableTest=0;
	sensoreTaglio.stato=enumStatiAnalizzaSensTaglio_Idle;
}

void vInitSensoreTaglio(void){
	memset(&sensoreTaglio,0,sizeof(sensoreTaglio));
	sensoreTaglio.ucSaturaDac_value=macParms.ucSlowDacColtello;
	sensoreTaglio.uiMin_cur=macParms.uiMinPosColtello_StepAD;
	sensoreTaglio.uiMax_cur=macParms.uiMaxPosColtello_StepAD;
	sensoreTaglio.uiPosRiposo=(sensoreTaglio.uiMax_cur*95L)/100;
	sensoreTaglio.uiPosRiposo2=(sensoreTaglio.uiMax_cur*97L)/100;
	sensoreTaglio.iTolerance=macParms.uiTolerancePosColtello_StepAD;
	// guadagno per il controllo del coltello [0..255]
	sensoreTaglio.uiKp=macParms.ucGainDacColtello;
}

void vInitReadCycleSensoreTaglio(void){
	sensoreTaglio.uiMin_cur=0x7fff;
	sensoreTaglio.uiMax_cur=0x0;
	sensoreTaglio.uiNumRead=0;
	sensoreTaglio.ucEnableTest=1;
	sensoreTaglio.ucFilter=0;
	sensoreTaglio.stato=enumStatiAnalizzaSensTaglio_WaitMin;
	// comando di non controllare la posizione del coltello....
	sensoreTaglio.stato_taglio=enumOperazioneLama_Idle;
}


// lucchetta la lama ad accettare solo un certo tipo di operazioni
// serve es per portare la lama in posizione di distanziazione spire e lasciarla l� per fare il setup
void vLockOperazioneLama(enumCiclicaLama op){
	sensoreTaglio.locked=1;
	sensoreTaglio.onlyCycleAdmitted=op;
}

// toglie il lock alle operazioni della lama
void vUnlockOperazioneLama(void){
	sensoreTaglio.locked=0;
}

// attiva una operazione lama
void vAttivaOperazioneLama(enumCiclicaLama op) {
	if (!macParms.ucUsaSensoreTaglio)
		return;

	// se operazioni lucchettate, e l'operazione non e' una di quelle ammesse, esco
	if (  (sensoreTaglio.locked)
		&&(op!=sensoreTaglio.onlyCycleAdmitted)
		)
		return;

	// reset flag che indica che l'ultima operazione comandata sulla lama era del distanziatore
	sensoreTaglio.ucLastOpWasOnDistanziatore=0;

	switch(op){
		case enumCiclicaLama_taglia:
            ui_requested_precut_after_spacing=0;
			sensoreTaglio.stato_taglio=enumOperazioneLama_Taglio_Init;
			break;
		case enumCiclicaLama_pretaglia:
			// versione 2.06: la distanziazione ha la priorit� rispetto al pretaglio
			// per cui se viene richiesto un pretaglio con distanziazione attiva, non faccio il pretaglio
			if (hDist.ucDistanziaInCorso&&hDist.uiEnabled_interrupt){
				sensoreTaglio.ucLastOpWasOnDistanziatore=1;
                ui_requested_precut_after_spacing=1;
				break;
            }
			// fall through!!!
			// il pretaglio normale � meno prioritario rispetto al distanziatore
		case enumCiclicaLama_pretaglia_distanziatore:
			sensoreTaglio.stato_taglio=enumOperazioneLama_Pretaglio_Init;
			break;
		case enumCiclicaLama_distanzia:
			// set del flag che indica che l'ultima operazione comandata sulla lama era del distanziatore
			sensoreTaglio.ucLastOpWasOnDistanziatore=1;
			sensoreTaglio.stato_taglio=enumOperazioneLama_Distanzia_Init;
			break;
		case enumCiclicaLama_scendi_lenta:
			sensoreTaglio.stato_taglio=enumOperazioneLama_DownSlow_Init;
			break;
		case enumCiclicaLama_scendi_veloce:
			sensoreTaglio.stato_taglio=enumOperazioneLama_DownFast_Init;
			break;
		case enumCiclicaLama_sali:
			ui_requested_precut_after_spacing=0;	
			sensoreTaglio.stato_taglio=enumOperazioneLama_UpFast_Init;
			break;
		case enumCiclicaLama_lenta:
			sensoreTaglio.ucSaturaDac=1;
			break;
		case enumCiclicaLama_veloce:
			sensoreTaglio.ucSaturaDac=0;
			break;
		default:
			break;
	}
}

// indica se � attiva una operazione della lama (taglio, pretaglio, discesa)
// ho finito l'operazione se:
// - sono in idle state
// - sono in loop di background
// - sto tagliando ed ho finito la fase veloce
unsigned char ucIsAttivaOperazioneLama(void){
	if (   (sensoreTaglio.stato_taglio==enumOperazioneLama_Idle)
		|| (sensoreTaglio.stato_taglio==enumOperazioneLama_BackgroundCtrl)
		|| (sensoreTaglio.stato_taglio==enumOperazioneLama_WaitTaglioHi_1)
		|| (sensoreTaglio.stato_taglio==enumOperazioneLama_Taglio_Distanzia)
		|| (sensoreTaglio.stato_taglio==enumOperazioneLama_Distanzia)
		)
		return 0;

	return 1;
}

#if 1
static unsigned char ucNumErrors_LatchFpga=0;
	// funzione che legge i valori della conversione da fpga
	// utilizzando la modalit� automatica di acquisizione, � possibile ottenere valori molto pi� accurati
	unsigned char ucRefreshAdchannels_Interrupt(void){
		xdata unsigned int ui;
		static data unsigned int uiAux;
        static unsigned long ul_count_loop_ok;
        static unsigned long ul_count_loop_auto_latch_disabled;
		// imposto latch a zero
		ucLatchAutoAD=0;
		ui=0;
        if (ucNumErrors_LatchFpga==0){
            ul_count_loop_ok++;
        }
        else{
            ul_count_loop_ok=0;
        }
        if (!ucEnableAutoAD){
            ul_count_loop_auto_latch_disabled++;
        }
		// attendo che anche fpga mi dica latch a zero
		// in teoria timeout non dovrebbe mai scadere, entro 2 o 3 microsecondi dovrebbe prenderlo
		while(ucLatchAutoAD){
			if (++ui>1000){
				ucNumErrors_LatchFpga++;
				return 0;
			}
		}
		// comando di fare latch
		ucLatchAutoAD=1;
		// aspetto finch� fpga mi dice che ha fatto latch
		// in teoria timeout non dovrebbe mai scadere, entro 2 o 3 microsecondi dovrebbe prenderlo
		ui=0;
		while(!ucLatchAutoAD){
			if (++ui>1000){
				ucNumErrors_LatchFpga++;
				return 0;
			}
		}
		//rimetto sempre a zero il latch!
		// in modo che al prossimo giro lo trovo gi� pronto...
		ucLatchAutoAD=0;
		// leggo il numero di latch effettuati
		ucAutoAD_NumOfLatch_var=ucAutoAD_NumOfLatch;
		// incremento il numero di letture effettuate in interrupt
		uiNumLettureMisuraDinamica_Interrupt++;

		//
		// lettura canale 0
		//
		uiAux=ucChannel_AutoAD_0_hi;
		uiAux=(uiAux<<8)|ucChannel_AutoAD_0_low;
		uiResultConvAd_Interrupt[0]=uiAux;

		//
		// lettura canale 1
		//
		uiAux=ucChannel_AutoAD_1_hi;
		uiAux=(uiAux<<8)|ucChannel_AutoAD_1_low;
		uiResultConvAd_Interrupt[1]=uiAux;

		//
		// lettura canale 2
		//
		uiAux=ucChannel_AutoAD_2_hi;
		uiAux=(uiAux<<8)|ucChannel_AutoAD_2_low;
		uiResultConvAd_Interrupt[2]=uiAux;

		//
		// lettura canale 3
		//
		uiAux=ucChannel_AutoAD_3_hi;
		uiAux=(uiAux<<8)|ucChannel_AutoAD_3_low;
		uiResultConvAd_Interrupt[3]=uiAux;

		//
		// lettura canale 4
		//
		uiAux=ucChannel_AutoAD_4_hi;
		uiAux=(uiAux<<8)|ucChannel_AutoAD_4_low;
		uiResultConvAd_Interrupt[4]=uiAux;

		//
		// lettura canale 5
		//
		uiAux=ucChannel_AutoAD_5_hi;
		uiAux=(uiAux<<8)|ucChannel_AutoAD_5_low;
		uiResultConvAd_Interrupt[5]=uiAux;

		//
		// lettura canale 6
		//
		uiAux=ucChannel_AutoAD_6_hi;
		uiAux=(uiAux<<8)|ucChannel_AutoAD_6_low;
		uiResultConvAd_Interrupt[6]=uiAux;

		//
		// lettura canale 7
		//
		uiAux=ucChannel_AutoAD_7_hi;
		uiAux=(uiAux<<8)|ucChannel_AutoAD_7_low;
		uiResultConvAd_Interrupt[7]=uiAux;

		// salvo la lettura della misura dinamica in una variabile ad hoc...
		uiLetturaMisuraDinamica_Interrupt=uiResultConvAd_Interrupt[enumChannelsAD7372_MisuraDinamica]&0x1FFF;
		// salvo la conversione dal canale del sensore di taglio una variabile ad hoc
		sensoreTaglio.uiLetturaSensoreTaglio=uiResultConvAd_Interrupt[enumChannelsAD7372_Pretaglio]&0x1FFF;;

		// tutta questa parte serve solo per debug
#ifdef defDebugLetturaAdFromFpga
		ucNumOfReadAutoAD_var=ucNumOfReadAutoAD;
		ucLastChanAutoAD_low_var=ucLastChanAutoAD_low;
		ucLastChanAutoAD_hi_var=ucLastChanAutoAD_hi;

		ucLastChanFltAutoAD_low_var=ucLastChanFltAutoAD_low;
		ucLastChanFltAutoAD_hi_var=ucLastChanFltAutoAD_hi;
		ucLastChanRamAutoAD_low_var=ucLastChanRamAutoAD_low;
		ucLastChanRamAutoAD_hi_var=ucLastChanRamAutoAD_hi;

		ucLastChanAccum6AutoAD_low_var=ucLastChanAccum6AutoAD_low;
		ucLastChanAccum6AutoAD_hi_var=ucLastChanAccum6AutoAD_hi;

		ucNumChannel=ucLastChanFltAutoAD_hi_var>>5;
		if (ucNumChannel==0){
			ucNumRow++;
			if (ucNumRow>=120){
				if (ucNumRow>=128){
					ucNumRow=0;
				}
			}
		}

		// solo per debug...faccio partire un'altra serie di acquisizioni
		// ucDebugAutoAD_0=1;
#endif
		return 1;
	}


	void vReadAD7327(void){
		xdata unsigned char i,iTheChannel,numLetture;
		xdata unsigned char ucChannel;
		volatile xdata unsigned char uiPrecNumLettureMisuraDinamica_Interrupt;
		xdata unsigned int uiResultConvAd[8];
		data unsigned int uiConvAD7327;

		// salvo il numero di accessi al conv a/d in interrupt
		uiPrecNumLettureMisuraDinamica_Interrupt=uiNumLettureMisuraDinamica_Interrupt;
		// metto un loop esterno da due cicli in modo che se subenmtra una interruzione ripeto il ciclo e
		// stavolta avr� le letture buone
		numLetture=0;
		while (numLetture<2){
			// leggo la conversione che interrupt ha copiato in uiResultConvAd_Interrupt
			memcpy(&uiResultConvAd[0],&uiResultConvAd_Interrupt[0],sizeof(uiResultConvAd));
			// converto i risultati delle conversioni (se non � subentrata un'interruzione che potrebbe
			// aver invalidato le mie operazioni...)
			// verifico che il numero di accessi al conv a/d in interrupt non sia cambiato...
			if (uiPrecNumLettureMisuraDinamica_Interrupt==uiNumLettureMisuraDinamica_Interrupt){
				// adesso dentro uiResultConvAd ho le 8 conversioni valide...
				i=0;
				while (i<8){
					uiConvAD7327=uiResultConvAd[i];
					ucChannel=i;
					// trovo lenum del segnale associato a questo canale a/d
					iTheChannel=ucTableADch2ADmeas[ucChannel];
					// attenzione! uso nel convertitore a/d la conversione in binario diretto
					// conversione unipolare:
					// 0--> 0
					// 0x1fff --> +fs
					if (rangesADmeasure[iTheChannel]==enumRangeAD7372_unipolar10V){
						iArrayConvAD7327[iTheChannel]=uiConvAD7327&0x1FFF;
						fArrayConvAD7327_Volts[iTheChannel]=(iArrayConvAD7327[iTheChannel]-0)*rangeAD7327[enumRangeAD7372_unipolar10V].fm+rangeAD7327[enumRangeAD7372_unipolar10V].fMinVolt;
					}
					// conversione bipolare:
					// 0--> -fs/2
					// 0x1fff --> +fs/2
					else if (rangesADmeasure[iTheChannel]==enumRangeAD7372_bipolar2_5V){
						// trovo il valore
						iArrayConvAD7327[iTheChannel]=(uiConvAD7327&0x1FFF);
						// ci tolgo met� scala
						iArrayConvAD7327[iTheChannel]-=defHalfValueAD7327;
						// uso la formula y=(x-x0)*m+q
						fArrayConvAD7327_Volts[iTheChannel]=(iArrayConvAD7327[iTheChannel]+defHalfValueAD7327)*rangeAD7327[enumRangeAD7372_bipolar2_5V].fm+rangeAD7327[enumRangeAD7372_bipolar2_5V].fMinVolt;
					}
					else{
						// trovo il valore
						iArrayConvAD7327[iTheChannel]=(uiConvAD7327&0x1FFF);
						// ci tolgo met� scala
						iArrayConvAD7327[iTheChannel]-=defHalfValueAD7327;
						fArrayConvAD7327_Volts[iTheChannel]=(iArrayConvAD7327[iTheChannel]+defHalfValueAD7327)*rangeAD7327[rangesADmeasure[iTheChannel]].fm+rangeAD7327[rangesADmeasure[iTheChannel]].fMinVolt;
					}
					i++;
				}//while (i<8)
				break; // esco dal loop esterni, ho acquisito le letture a/d
			}//if (uiPrecNumLettureMisuraDinamica_Interrupt==uiNumLettureMisuraDinamica_Interrupt)
			numLetture++;
		}//while (numLetture<2)
	}//void vReadAD7327(void)

#else
	// routine per leggere il valore della misura dinamica dalla routine
	// di interrupt
	unsigned char ucReadAD7327_MisuraDinamicaInterrupt(void){
		data unsigned char i;
		// incremento il numero di letture effettuate in interrupt
		uiNumLettureMisuraDinamica_Interrupt++;

		// attendi fine lettura eventualmente in corso...
		while(ucReadBusy_WriteLatch){
		}
		i=0;
		// spazzolo tutti ed 8 i canali...
		while (i<8){
		// scrivi 0x0000 --> comanda lettura da canale corrente
		    ucLowByteAD7327=0x00;
			ucHiByteAD7327=0x00;
		// attendi fine lettura
			while(ucReadBusy_WriteLatch){
			}
		// latch del valore della lettura
			ucReadBusy_WriteLatch=0;
		// leggo se il canale � quello che mi interessa (enumChannelsAD7372_MisuraDinamica)
			if (((ucHiByteAD7327>>5)&0x07)==enumChannelsAD7372_MisuraDinamica){
				uiConvAD7327=ucHiByteAD7327;
				uiConvAD7327=(uiConvAD7327<<8)|ucLowByteAD7327;
				//ucChannel=enumChannelsAD7372_MisuraDinamica;
			// se canale � quello della misura dinamica...
			// leggo il valore convertito
				// salvo il valore letto dentro ad una variabile ad hoc
				uiLetturaMisuraDinamica_Interrupt=uiConvAD7327&0x1FFF;
				// posso uscire dalla funzione con ok, tanto la misura la ho fatta...
				return 1;
			}
			i++;
		}
		return 0;
	}//unsigned char ucReadAD7327_MisuraDinamicaInterrupt(void)



	void vReadAD7327(void){
		xdata unsigned char i,iTheChannel,numLetture;
		xdata unsigned char ucChannel;
		volatile xdata unsigned char uiPrecNumLettureMisuraDinamica_Interrupt;
		xdata unsigned int uiResultConvAd[8];
		// salvo il numero di accessi al conv a/d in interrupt
		uiPrecNumLettureMisuraDinamica_Interrupt=uiNumLettureMisuraDinamica_Interrupt;
		// metto un loop esterno da due cicli in modo che se subentra una interruzione ripeto il ciclo e
		// stavolta avr� le letture buone
		numLetture=0;
		while (numLetture<2){
			// in questo primo loop
			// accumulo i risultati delle conversioni
			i=0;
			while (i<8){
			// attendi fine lettura
				while(ucReadBusy_WriteLatch){
				}
			// scrivi 0x0000 --> comanda lettura
				ucLowByteAD7327=0x00;
				ucHiByteAD7327=0x00;
			// attendi fine lettura
				while(ucReadBusy_WriteLatch){
				}
			// latch
				ucReadBusy_WriteLatch=0;
			// leggo risultato
				uiConvAD7327=ucHiByteAD7327;
				uiResultConvAd[i]=(uiConvAD7327<<8)|ucLowByteAD7327;
				i++;
			}
			// in questo secondo loop
			// converto i risultati delle conversioni (se non � subentrata un'interruzione che potrebbe
			// aver invalidato le mie operazioni...)
			// verifico che il numero di accessi al conv a/d in interrupt non sia cambiato...
			if (uiPrecNumLettureMisuraDinamica_Interrupt==uiNumLettureMisuraDinamica_Interrupt){
				i=0;
				while (i<8){
					uiConvAD7327=uiResultConvAd[i];
					ucChannel=(uiConvAD7327>>13)&0x07;
					// trovo l'enum del segnale associato a questo canale a/d
					iTheChannel=ucTableADch2ADmeas[ucChannel];
					// attenzione! uso nel convertitore a/d la conversione in binario diretto
					// conversione unipolare:
					// 0--> 0
					// 0x1fff --> +fs
					if (rangesADmeasure[iTheChannel]==enumRangeAD7372_unipolar10V){
						iArrayConvAD7327[iTheChannel]=uiConvAD7327&0x1FFF;
						fArrayConvAD7327_Volts[iTheChannel]=(iArrayConvAD7327[iTheChannel]-0)*rangeAD7327[enumRangeAD7372_unipolar10V].fm+rangeAD7327[enumRangeAD7372_unipolar10V].fMinVolt;
					}
					// conversione bipolare:
					// 0--> -fs/2
					// 0x1fff --> +fs/2
					else if (rangesADmeasure[iTheChannel]==enumRangeAD7372_bipolar2_5V){
						// trovo il valore
						iArrayConvAD7327[iTheChannel]=(uiConvAD7327&0x1FFF);
						// ci tolgo met� scala
						iArrayConvAD7327[iTheChannel]-=defHalfValueAD7327;
						// uso la formula y=(x-x0)*m+q
						fArrayConvAD7327_Volts[iTheChannel]=(iArrayConvAD7327[iTheChannel]+defHalfValueAD7327)*rangeAD7327[enumRangeAD7372_bipolar2_5V].fm+rangeAD7327[enumRangeAD7372_bipolar2_5V].fMinVolt;
					}
					else{
						// trovo il valore
						iArrayConvAD7327[iTheChannel]=(uiConvAD7327&0x1FFF);
						// ci tolgo met� scala
						iArrayConvAD7327[iTheChannel]-=defHalfValueAD7327;
						fArrayConvAD7327_Volts[iTheChannel]=(iArrayConvAD7327[iTheChannel]+defHalfValueAD7327)*rangeAD7327[rangesADmeasure[iTheChannel]].fm+rangeAD7327[rangesADmeasure[iTheChannel]].fMinVolt;
					}
					i++;
				}//while (i=0;i<8;i++){
				break; // esco dal loop esterni, ho acquisito le letture a/d
			}//if (uiPrecNumLettureMisuraDinamica_Interrupt==uiNumLettureMisuraDinamica_Interrupt)
			numLetture++;
		}//while (numLetture=0;numLetture<2;numLetture++)
	}//void vReadAD7327(void)

#endif


data int iActCounts;
data int iActCounts2;
xdata int iOffsetCounts2;

int iReadCounter(void){
register unsigned char ucWaitToggle;
register unsigned char ucNumwait;
	// memorizzo lo stato del registro di toggle (0 od 1);
	// � un registro che si inverte ad ogni acquisizione del valore del contatore
	ucWaitToggle=ucCounter1_latch_toggle;
	// taccio partire la conversione del contatore
	ucCounter1_latch_toggle=0;
	// azzero numero loop di attesa
	ucNumwait=0;
	// aspetto (max 3 loops) che il bit faccia il toggle, cio� che fpga acquisisca nuovo valore contatore
	while((ucCounter1_latch_toggle==ucWaitToggle)&&(++ucNumwait<3));
	// leggo parte alta dei 16 bit di conteggio
	iActCounts=ucCounter1_hi;
	iActCounts<<=8;
	// faccio or con parte bassa del conteggio
	iActCounts|=ucCounter1_low;
	// restituisco i 16 bit del conteggio
	return iActCounts;
}//int iReadCounter(void)


int iInitReadCounter_2(void){
	iOffsetCounts2=0;
	iReadCounter_2();
	iAzzeraReadCounter_2();
	iReadCounter_2();
	return 1;
}
int iAzzeraReadCounter_2(void){
	iOffsetCounts2+=iActCounts2;
	return 1;
}


void vInitPosizioneLamaTaglio(void){
	if (!macParms.ucUsaSensoreTaglio){
		return;
	}

	vAttivaOperazioneLama(enumCiclicaLama_lenta);
	vAttivaOperazioneLama(enumCiclicaLama_taglia);
	sensoreTaglio.ucWaitInizializzazioneLama=1;
	sensoreTaglio.ucGoToFastMode=1;
//	while(ucIsAttivaOperazioneLama());
//	vAttivaOperazioneLama(enumCiclicaLama_veloce);
}

typedef struct _TipoStructSpeedTargetUrp{
	unsigned long ul_act_speed;
	unsigned long ul_flt_speed;
	unsigned int  ui_accum_gap_imp;
	unsigned char uc_count_num_samples;
}TipoStructSpeedTargetUrp;
// periodo di campionamento della velocit� di produzione in target_urp per period
// uso 20ms per avere sufficiente definizione anche nel caso di velocit� basse
// con unavelocit� di produzione di 30m/min
// impGiro     =4000
// mmGiro      =426.500000
// 30m/min=30'000mm/min=500mm/s=1.17 giri/s=4689 imp/s=5imp/ms=25imp/5ms
// quindi andando a 1m/min mi aspetto di trovare 0imp/5ms, ma mi pu� andare anche abbastanza bene
// perch� 1m/min � una velocit� bassa per cui la compensazione del ritardo coltello potrebbe non avere molto senso
// posso allora usare un periodo pari a 4*5ms cos� ho una discreta risoluzione anche a 1m/min
#define defPeriodSamplingSpeedTargetUrp (20/PeriodoIntMs)
// fattore di riduzione usato nel calcolo della speed target urp per evitare di incorrere in overflow
#define defReductionFactorSpeedTargetUrp 16
xdata TipoStructSpeedTargetUrp stu;

// inizializzazione del calcolo della velocit� in target urp per T
void vSpeedTargetUrp_init(void){
	memset(&stu,0,sizeof(stu));
}

// aggiornamento del calcolo della velocit� in target urp per T
// ingresso: valore attuale e precedente della lettura encoder, come interi senza segno a 16bit
void vSpeedTargetUrp_update(unsigned short int us_new_encoder_value,unsigned short int us_prev_encoder_value){
	xdata unsigned short int us_delta_imp;
	// verifico variazione lettura encoder per calcolare un delta impulsi corretto
	if (us_new_encoder_value<us_prev_encoder_value){
		us_delta_imp=(0xFFFF-(us_prev_encoder_value-us_new_encoder_value))+1;
	}
	else{
		us_delta_imp=us_new_encoder_value-us_prev_encoder_value;
	}
	// verifico che non vi siano stati overflow!!!
	if (us_delta_imp>defMaxDeltaEncoder)
		us_delta_imp=0;

	// incremento accumulatore del numero di impulsi arrivati
	stu.ui_accum_gap_imp+=us_delta_imp;
	if (++stu.uc_count_num_samples>=defPeriodSamplingSpeedTargetUrp){
		// se lavoro con misura ohmica, devo restituire il numero di urp per periodo di campionamento
		// purtroppo mi serve anche un fattore di normalizzazione per evitare di sforare nell'accumulo dei vari ro
		// se invece lavoro in lunghezza, allora computo solo il numero di impulsi encoder arrivati
		if (withMis)
			stu.ul_act_speed=(unsigned long)((stu.ui_accum_gap_imp* actRo ) / defReductionFactorSpeedTargetUrp);
		else
			stu.ul_act_speed=(unsigned long)(stu.ui_accum_gap_imp);
		// usando pesi 100/28/128 quanto tempo ci metto a dimenticare un campione vecchio?
		// (100/128)^n=0.1 -->n*ln(100/128)=ln(0.1)-->n=ln(0.1)/ln(100/128)=9 periodi da 5ms
		// --> 45ms per portare al 10% la memoria di un campione vecchio
		// credo possa essere un filtro valido
		stu.ul_flt_speed=(100*stu.ul_flt_speed+28*stu.ul_act_speed)/128;
		stu.ui_accum_gap_imp=0;
		stu.uc_count_num_samples=0;
	}
}//void vSpeedTargetUrp_update(unsigned int gapImp)

// restituisce il valore stimato della velocit� in target urp per T
// attenzione! se lavoriamo con misura ohmica (withmis=1) la velocit� restituita, per essere convertita in urp/T, 
//      deve essere divisa per (MAGN_LEN*defPeriodSamplingSpeedTargetUrp/defReductionFactorSpeedTargetUrp)
// se lavoriamo senza misura ohmica (withMis=0) allora non ci sono fattori da applicare
unsigned long ulSpeedTargetUrp_get(void){
	return stu.ul_flt_speed;
}// end of --> unsigned long ulSpeedTargetUrp_get(void)

// normalizzazione intera di un valore moltiplicato per una percentuale di distanziazione
long l_multiply_long_perc_dist(long theLong, unsigned short int us_perc_dist){
#if 1
	xdata long long_hi,long_lo;
	// per fare la moltiplicazione fra un long ed un intero a 16 bit
	// moltiplico separatamente la parte alta e bassa del long
	// scarto la parte bassa del long basso e sommo i due risultati
	// poi shifto opportunamente per tener conto del fattore di scala dell'intero
	long_hi=theLong>>16;	// parte alta
	long_lo=theLong&0xFFFF; //parte bassa
	long_hi*=us_perc_dist;	// moltiplico la parte alta
	long_lo*=us_perc_dist;	// moltiplico la parte bassa
	long_lo>>=16;			// elimino la parte bassa bassa
	long_hi+=long_lo;		// sommo i due long
	long_hi<<=(16-defMaxNumShiftPercDistanzia);	// aggiusto il fattore di scala, che � minore di 16
	return long_hi;
#else
	register unsigned char ucnumshift;
	// azzero il numero di shift necessari a normalizzare il numero in ingresso
	ucnumshift=0;
	// normalizzo per evitare overlfow, evito anche di andare a toccare il bit 31 del numero
	// perch� non cambi segno
	while ( (theLong>defMaxIntegerIcanMultiplyByPercDistanzia)&&(ucnumshift<25)){
		ucnumshift++;
		theLong>>=1;
	}
	// eseguo la moltiplicazione senza timore di overflow, poi divido per il numero di shift introdotto dalla perc
	theLong=(theLong*us_perc_dist)>>(defMaxNumShiftPercDistanzia);
	// ripristino lo shift iniziale
	if (ucnumshift)
		theLong<<=ucnumshift;
	// restituisco il valore trovato...
	return theLong;
#endif
}//long l_multiply_long_perc_dist(unsigned long theLong, unsigned short int us_perc_dist)

typedef enum{
	enum_reset_dist_reset_numero_distanziazioni=0,
	enum_reset_dist_increment_numero_distanziazioni,
	enum_reset_dist_numof
} enum_reset_dist_type;

void v_reset_stato_distanziatore(enum_reset_dist_type reset_dist_type){
	// indico fine della zona critica
	hDist.ucRefresh = 0;
	// resetto il flag di distanziazione in corso 
	hDist.ucDistanziaInCorso=0;
	// reset od incremento del numero di distanziazioni???
	switch(reset_dist_type){
		case enum_reset_dist_reset_numero_distanziazioni:
		default:
			hDist.actNumDistanzia=0;
			break;
		case enum_reset_dist_increment_numero_distanziazioni:
			hDist.actNumDistanzia++;
			break;
	}
	// passo nello stato coltello idle
	hDist.stato=enum_coltello_distanzia_stato_idle;
}


int iReadCounter_2(void){
register unsigned char ucWaitToggle;
register unsigned char ucNumwait;

extern unsigned char ucSendCommandPosTaglioMax519_Main(unsigned char uc);
	// memorizzo lo stato del registro di toggle (0 od 1);
	// � un registro che si inverte ad ogni acquisizione del valore del contatore
	ucWaitToggle=ucCounter2_latch_toggle;
	// taccio partire la conversione del contatore
	ucCounter2_latch_toggle=0;
	// azzero numero loop di attesa
	ucNumwait=0;
	// aspetto (max 3 loops) che il bit faccia il toggle, cio� che fpga acquisisca nuovo valore contatore
	while((ucCounter2_latch_toggle==ucWaitToggle)&&(++ucNumwait<100)){
		v_wait_microseconds(10);
	}
	// leggo parte alta dei 16 bit di conteggio
	iActCounts2=ucCounter2_hi;
	iActCounts2<<=8;
	// faccio or con parte bassa del conteggio
	iActCounts2|=ucCounter2_low;
	iActCounts2-=iOffsetCounts2;
	// restituisco i 16 bit del conteggio
	return iActCounts2;
}//int iReadCounter_2(void)


// con che frequenza controllo le distanziazioni? ogni 4 cicli di interrupt, 4*5=20ms
#define def_do_distanz_ctrl ((ucCntInterrupt&3)==3)

	// gestione del taglio asincrono nel caso si stia usando il distanziatore
	void vHandleTaglioAsincrono_Distanziatore(void){

		// se distanziatore � il coltello allora devo contare anche il primo pezzo, altrimenti no
		if (macParms.uc_distanziatore_type!=enum_distanziatore_type_coltello)
			BitContaPezzi=0;

		if (  (macParms.uc_distanziatore_type==enum_distanziatore_type_coltello)
		    ||(hDist.anticipoDistanz==0)
			){
			if (withMis){
				resLastSpezUp=0;
				actRes      = 0;
			}
			else{
				actLenImp=0;
			}
		}
		else{
			if (withMis){
				resLastSpezUp=TARGET_URP-(((hDist.anticipoDistanz*hDist.actRoDist)/nomRo)%TARGET_URP);
				actRes      = resLastSpezUp;
			}
			else{
				actLenImp=targetLenImp-((hDist.anticipoDistanz*actCorrOhmica)/1000)%targetLenImp;
			}
		}
		hDist.actResDistValue=0;
		// Disattivo il comando di distanziazione. 
		outDigMis_Hi &= ~(ODG_DISTANZIA>>8);
		Outputs_Hi = outDigMis_Hi;

		// dopo un taglio asincrono, devo resettare lo stato del distanziatore...
		v_reset_stato_distanziatore(enum_reset_dist_reset_numero_distanziazioni);

	}//void vHandleTaglioAsincrono_Distanziatore(void)
	
// test per verificare se la prima distanziazione va fatta al volo, gi� durante il taglio e quindi la ciclica di taglio deve essere modificata
	unsigned int ui_separate_coil_during_cut(void){
		if (hDist.uiEnabled_interrupt&&(hprg.theRunning.usPercDistanzia_Starts[0]==0)){
			return 1;
		}
		return 0;
	}
	//
	// *****************************************
	// *****************************************
	//
	// gestione del distanziatore nella routine di interrupt...
	//
	// *****************************************
	// *****************************************
	//
	void vHandleDistanziatore_Interrupt(void){
	   xdata static long lFinalResValue;
// fino a qui OK
//	   return;
		if (!hDist.uiEnabled_interrupt)
		   return;
	   if (withMis)
			lFinalResValue=TARGET_URP;
	   else
			lFinalResValue=targetLenImp;
	
		if (withMis){
			if (actRes<=TARGET_URP)
			   hDist.actResDistValue=actRes;
			else
			   hDist.actResDistValue=TARGET_URP;
			// al valore della resistenza attualmente prodotta, sommo la parte di resistenza compresa
			// fra distanziatore e coltello
			hDist.actResDistValue+=(hDist.anticipoDistanz*hDist.actRoDist)/nomRo;
			// Mi serve una formula per tener conto del tempo di discesa del coltello/distanziatore 
			// in posizione di distanziazione
			// Dato che il tempo di discesa � un parametro, devo conoscere la velocit� di produzione 
			// in target_urp/ms per poter raggiungere la qta corretta
			// a 30m/min posso aspettarmi circa 
			// impGiro     =4000
			// mmGiro      =426.500000
			// 30m/min=30'000mm/min=500mm/s=1.17 giri/s=4689 imp/s=5imp/ms=25imp/5ms
			// quindi andando a 1m/min mi aspetto di trovare 0imp/5ms, ma mi pu� andare anche abbastanza bene
			// perch� 1m/min � una velocit� bassa per cui la compensazione del ritardo coltello potrebbe non avere molto senso
			// posso allora usare un periodo pari a 4*5ms cos� ho una discreta risoluzione anche a 1m/min
			// adesso cerco di stimare quanta resistenza viene prodotta nel tempo di discesa coltello, per compensarne l'effetto
			hDist.deltaDistValue=(ulSpeedTargetUrp_get()*hprg.theRunning.usDelayDistanziaDownMs)/((MAGN_LEN*PeriodoIntMs*defPeriodSamplingSpeedTargetUrp)/defReductionFactorSpeedTargetUrp);
			// tolgo la resistenza eventualmente in piu' 
			hDist.actResDistValue=hDist.actResDistValue%TARGET_URP;
		}
		else{
			if (actLenImp<=targetLenImp)
			   hDist.actResDistValue=actLenImp;
			else
			   hDist.actResDistValue=targetLenImp;
			// sommo resistenza prodotta tra distanziatore e coltello
			hDist.actResDistValue+=(hDist.anticipoDistanz*actCorrOhmica)>>10;
			// calcolo anche la resistenza prodotta durante la discesa del coltello
			hDist.deltaDistValue=(ulSpeedTargetUrp_get()*hprg.theRunning.usDelayDistanziaDownMs)/(PeriodoIntMs*defPeriodSamplingSpeedTargetUrp);
			// tolgo la resistenza in piu' 
			hDist.actResDistValue=hDist.actResDistValue%targetLenImp;
		}
// fino a qui tutto ok
//	   return;


		// se sto iniziando una nuova resistenza (valore precedente superiore a quello attuale+mezza resistenza)
		// allora reinizializzo i miei contatori
		// attenzione: alla prima resistenza, potrebbe esserci il moncone da tagliare, per cui se il primo pezzo non � stato eliminato
		// devo aspettare prima di iniziare i miei calcoli
		// se sto lavorando col distanziatore esterno (non uso la lama) allora la gestione � diversa, viene ignorato il primo moncone (per ora)
		if(   (hDist.precResDistValue>lFinalResValue/2+hDist.actResDistValue)
			&&(spiralatrice.PrimoPezzoEliminato)
			// non devo distanziare i pezzi successivi all' ultimo 
			&&(actNumPezzi <nvram_struct.commesse[spiralatrice.runningCommessa].n+nvram_struct.commesse[spiralatrice.runningCommessa].magg-1)
	      ){
			// abilito le distanziazioni 
	        hDist.ucStartDistanzia=1;
			// Disattivo il comando di distanziazione. 
			outDigMis_Hi &= ~(ODG_DISTANZIA>>8);
			Outputs_Hi = outDigMis_Hi;
			if (sensoreTaglio.ucLastOpWasOnDistanziatore&&!ui_separate_coil_during_cut()){
				if (macParms.uc_distanziatore_type==enum_distanziatore_type_coltello){
					vAttivaOperazioneLama(enumCiclicaLama_sali);
				}
			}
			// reset dello stato del distanziatore
			v_reset_stato_distanziatore(enum_reset_dist_reset_numero_distanziazioni);

			// salvo il valore attuale della resistenza prodotta sul distanziatore 
			hDist.precResDistValue=hDist.actResDistValue;

		}
		// altrimenti salvo valore precedente della resistenza, ma solo se 
		// il valore attuale � superiore al precedente
		else if (hDist.actResDistValue>hDist.precResDistValue)
			// salvo il valore attuale della resistenza prodotta sul distanziatore 
			hDist.precResDistValue=hDist.actResDistValue;

		// in che stato si trova il distanziatore???
		switch(hDist.stato){
			// in attesa di iniziare una procedura di distanziazione
			case enum_coltello_distanzia_stato_idle:
			default:
				// indico fine della zona critica
				hDist.ucRefresh = 0;
				// devo abilitare la distanziazione?
				if (   (hDist.actNumDistanzia<hDist.runPrgNumDistanzia)
					 && hDist.ucStartDistanzia
					// nell'abbassamento del coltello tengo anche conto del tempo di discesa, mentre nel sollevarlo non ne tengo conto,
					// suppongo che sia immediato...
					 &&(hDist.actResDistValue+hDist.deltaDistValue+hDist.res_spire_compressione_urp_imp>=l_multiply_long_perc_dist(lFinalResValue, hprg.theRunning.usPercDistanzia_Starts[hDist.actNumDistanzia]))
				   ){
					hDist.uc_clocks[0]=ucCntInterrupt;
					hDist.ucDistanziaInCorso=1;
					// passo nello stato per cui abbasso il coltello il posizione di pretaglio o il separatore

					// Attivazione fisica del comando di abbassamento separatore. 
					outDigMis_Hi |= (ODG_DISTANZIA>>8);
					Outputs_Hi = outDigMis_Hi;
					// passo nello stato coltello in posizione di pretaglio
					hDist.stato=enum_coltello_distanzia_stato_do_pretaglio;
					// se distanziatore=coltello, attivo la ciclica di discesa coltello in posizione di pretaglio!
					if (macParms.uc_distanziatore_type==enum_distanziatore_type_coltello){
						// se la spira va distanziata gi� dalle prime spire, � la ciclica di taglio che si cura di gestire il posizionamento
						if ((hDist.actNumDistanzia==0)&&ui_separate_coil_during_cut()){
						}
						else{
							// devo dare la precedenza alle operazioni di pretaglio/taglio rispetto a quelle del distanziatore!!!!!!
							if (avvStatus!=AVV_WAIT_TAGLIO){
								vAttivaOperazioneLama(enumCiclicaLama_pretaglia_distanziatore);
								// se il pretaglio era gi� attivo prima dell'inizio della distanziazione, chiedo di ritornare in pretaglio alla fine della distanziazione
								if (avvStatus==AVV_WAIT_TAG){
									ui_requested_precut_after_spacing=1;
								}
							}
						}
					}

				}
				break;
			case enum_coltello_distanzia_stato_do_pretaglio:
				// indico che il loop va eseguito con una certa frequenza
				hDist.ucRefresh = 1;
				// se raggiungo la percentuale impostata allora scendo in posizione di distanziazione
				// aggiungo 70ms fissi per tener conto della durata discesa coltello: da sistemare!!!!
#warning sistemare i 70ms fissi attuali...
				if ( (   hDist.actResDistValue
						+(ulSpeedTargetUrp_get()*70)/((MAGN_LEN*PeriodoIntMs*defPeriodSamplingSpeedTargetUrp)/defReductionFactorSpeedTargetUrp)
						+hDist.res_spire_compressione_urp_imp
						>=l_multiply_long_perc_dist(lFinalResValue, hprg.theRunning.usPercDistanzia_Starts[hDist.actNumDistanzia])
					  )
					|| ((hDist.actNumDistanzia==0)&&ui_separate_coil_during_cut())
					){
					hDist.uc_clocks[1]=ucCntInterrupt;
					hDist.stato=enum_coltello_distanzia_stato_do_distanzia;
					// valore ohmico alla fine della distanziazione
					hDist.deltaDistValue=l_multiply_long_perc_dist(lFinalResValue, hprg.theRunning.usPercDistanzia_Ends[hDist.actNumDistanzia]);
					// calcolo il max tempo per il quale il coltello dovrebbe rimanere gi�
					// def_timeout_spaziatore_factor volte il tempo previsto per la realizzazione della distanziazione
					hDist.ulAux=ulSpeedTargetUrp_get();
					// reset timeout
					hDist.usTimeout_NumT=0;
					// se velocit� attuale>0
					if (hDist.ulAux){
						if (withMis){
							hDist.usTimeout_NumT=(def_timeout_spaziatore_factor*(hDist.deltaDistValue-hDist.actResDistValue)*((MAGN_LEN*defPeriodSamplingSpeedTargetUrp)/defReductionFactorSpeedTargetUrp))/hDist.ulAux;
						}
						else{
							hDist.usTimeout_NumT=(def_timeout_spaziatore_factor*defPeriodSamplingSpeedTargetUrp*(hDist.deltaDistValue-hDist.actResDistValue))/hDist.ulAux;
						}
					}
					// se minore del minimo, clippo
					if (hDist.usTimeout_NumT<def_timeout_spaziatore_factor*defMinTimeoutSpaziatoreDown_NumT){
						hDist.usTimeout_NumT=def_timeout_spaziatore_factor*defMinTimeoutSpaziatoreDown_NumT;
					}
					// se distanziatore=coltello, attivo la ciclica di discesa coltello in posizione di pretaglio!
					if (macParms.uc_distanziatore_type==enum_distanziatore_type_coltello){
						// se la spira va distanziata gi� dalle prime spire, � la ciclica di taglio che si cura di gestire il posizionamento
						if ((hDist.actNumDistanzia==0)&&ui_separate_coil_during_cut()){
						}
						else{
							// devo dare la precedenza alle operazioni di pretaglio/taglio rispetto a quelle del distanziatore!!!!!!
							if (avvStatus!=AVV_WAIT_TAGLIO)
								vAttivaOperazioneLama(enumCiclicaLama_distanzia);
						}
					}
				}
				break;
			case enum_coltello_distanzia_stato_do_distanzia:
				// indico che il loop va eseguito con una certa frequenza
				hDist.ucRefresh = 1;
				// decremento il timer timeout
				if (hDist.usTimeout_NumT)
					hDist.usTimeout_NumT--;
				// se � scaduto il timeout
				// oppure ho raggiunto/superato la percentuale finale della resistenza,
				// alzo il distanziatore/coltello
				if (  (!hDist.usTimeout_NumT)
					||(hDist.actResDistValue>=l_multiply_long_perc_dist(lFinalResValue, hprg.theRunning.usPercDistanzia_Ends[hDist.actNumDistanzia]))
				   ){
				   // solo per poter mettere il breakpoint per intercettare i timeout distanziazione...
					if (hDist.usTimeout_NumT==0){
						hDist.uc_clocks[2]=ucCntInterrupt;
					}
					// disattivo il comando di distanziazione
					outDigMis_Hi &= ~(ODG_DISTANZIA>>8);
					Outputs_Hi = outDigMis_Hi;
					// se distanziatore=coltello, attivo la ciclica di risalita coltello in posizione di riposo
					// se non � in corso un taglio o pretaglio, faccio salire il distanziatore
					// quindi controllo se l'ultima operaz comandata sul coltello era per distanziare le spire
					if (sensoreTaglio.ucLastOpWasOnDistanziatore){
						if (macParms.uc_distanziatore_type==enum_distanziatore_type_coltello){
							// devo dare la precedenza alle operazioni di taglio rispetto a quelle del distanziatore!!!!!!
							if (avvStatus!=AVV_WAIT_TAGLIO){
                                if (ui_requested_precut_after_spacing){
                                    //sensoreTaglio.ui_on_the_wrong_cut_side=1;
                                    sensoreTaglio.stato_taglio=enumOperazioneLama_Pretaglio_Init;
                                }
                                else{
                                    vAttivaOperazioneLama(enumCiclicaLama_sali);
                                }
								ui_requested_precut_after_spacing=0;
                            }
						}
					}
					// reset dello stato, per� con incremento del numero di distanziazioni
					v_reset_stato_distanziatore(enum_reset_dist_increment_numero_distanziazioni);
				}
				break;
		}

	}//void vHandleDistanziatore_Interrupt(void)

void vRefreshInputEdge(enumInputEdge ie,unsigned int uiNewValue){
	xdata TipoStructAnalyzeInputEdge *paie;
	paie=&aie[ie];
	paie->ucPreviousValue=paie->ucActualValue;
	paie->ucActualValue=uiNewValue?1:0;
	if (paie->ucActualValue&&!paie->ucPreviousValue){
		paie->ucPositiveEdge=1;
	}
	if (!paie->ucActualValue&&paie->ucPreviousValue){
		paie->ucNegativeEdge=1;
	}
}

void v_reset_cutter_position(void){
	sensoreTaglio.ui_on_the_wrong_cut_side=0;
	ui_requested_precut_after_spacing=0;
}		



data unsigned char uc_in_timer0_interrupt=0;
xdata unsigned int ui_cnt_in_timer0_interrupt=0;



// routine di gestione del timer a tot ms
//__attribute__ ((interrupt ("IRQ"))) void IRQ_Handler_timer0(void){
void IRQ_Handler_timer0(void){
//	static data unsigned char i;
// solo per debug; imposto q15 all'inizio di ogni interrupt, lo resetto alla fine
/*
#warning 
#warning *************
#warning debug output hi @ 0x80 per tempistica routine di interrupt
#warning *************
#warning 
*/
	Outputs_Hi|=0x80;
    if (uc_in_timer0_interrupt){
		ui_cnt_in_timer0_interrupt++;
		goto really_the_end_timer0;
	}
	uc_in_timer0_interrupt=1;
//#error abilitare interrupt da timer1=i2c bus

// watch dog feed
    WDFEED=0XAA;
    WDFEED=0X55;

// incremento di 1 il contatore di interrupt
	ucCntInterrupt++;
    ul_counter_interrupt_timer0++;

	if (cntAvv == 100)
		cntAvv = 0;

	if (cntMis == 100)
		cntMis = 0;

	if (misStaSleep)
		--misStaSleep;

	if (upd7segSleep)
		--upd7segSleep;

	if (avvSleep)
		--avvSleep;

	if (misSleep)
		--misSleep;

#ifdef defSimulaEncoder
	if (OutDig & ODG_MOTORE)
		encVal += 5;
#else
	encVal = iReadCounter();
#endif

	// reimpostazione timer
#ifndef ARM9platform
	TL0 = VAL_TL0;
	TH0 = VAL_TH0;
#endif
	// se sono ancora in fase di boot, esco subito
	if (ucBooting){
		goto endOfTimerProcedure;
	}
#ifdef def_canopen_enable
    v_process_canopen_rpdo();
#endif

	
	vRefreshInputEdge(enumInputEdge_stop,(InpDig & IDG_STOP));
	vRefreshInputEdge(enumInputEdge_start,(InpDig & IDG_START));
	

	++cntAvv;
	++cntMis;

	// Salvo in outdigmis lo stato del port B dell' 8255.
	outDigMis=OutDig;

	// Resetto le uscite di taglio e pretaglio per sicurezza: se del caso, le setto di volta in volta.
	outDigMis&=~(ODG_TAGLIO|ODG_PRETAGLIO);
	// se motore fermo, azzero variabile che indica tempo occorrente epr resistenza corrente!!!!
	if (outDigMis&(ODG_MOTORE))
		// incremento il tempo impiegato per produrre la resistenza corrente
		TPR.ulTempoPerResistenzaCorrente++;
	else
		TPR.ulTempoPerResistenzaCorrente=0;

	// refresh della lettura dei canali a/d
	ucRefreshAdchannels_Interrupt();

	// Se velocit� troppo elevata: ruota ferma oppure qualcosa di strano: cancello tutto ed esco.
    // Se overflow nel conteggio da encoder, riporto tutto a zero.
	if ((encVal>newEnc)&&(encVal-newEnc>defMaxDeltaEncoder)){
SortByErrorEncoder:
		newEnc =oldEncMis = oldEncLav=encVal;
		goto endOfTimerProcedure;
	}
	// calcolo la velocit� attuale di produzione in target_urp / periodo di interrupt
	vSpeedTargetUrp_update(encVal,newEnc);
	// handle temperature compensation
	v_handle_temperature_compensation();
	

	// devo trovare il minimo ed il massimo delle conversioni dal canale a/d del sensore di taglio
	if ((!PrgRunning)&&sensoreTaglio.ucEnableTest){
			if (sensoreTaglio.uiNumRead<60000)
				sensoreTaglio.uiNumRead++;
			switch (sensoreTaglio.stato){
				case enumStatiAnalizzaSensTaglio_Idle:
				default:
				{
					break;
				}
				case enumStatiAnalizzaSensTaglio_WaitMin:
					if (sensoreTaglio.uiMin_cur>sensoreTaglio.uiLetturaSensoreTaglio){
						sensoreTaglio.uiMin_cur=sensoreTaglio.uiLetturaSensoreTaglio;
						sensoreTaglio.ucFilter=0;
					}
					else if (sensoreTaglio.uiLetturaSensoreTaglio>sensoreTaglio.uiMax_cur-300){
						if (++sensoreTaglio.ucFilter>=3){
							sensoreTaglio.stato=enumStatiAnalizzaSensTaglio_WaitMax;
							sensoreTaglio.ucFilter=0;
						}
					}
					else{
						sensoreTaglio.ucFilter=0;
					}
					break;
				case enumStatiAnalizzaSensTaglio_WaitMax:
					if (sensoreTaglio.uiLetturaSensoreTaglio>sensoreTaglio.uiMax_cur){
						sensoreTaglio.uiMax_cur=sensoreTaglio.uiLetturaSensoreTaglio;
						sensoreTaglio.uiPosRiposo=(sensoreTaglio.uiMax_cur*95L)/100;
						sensoreTaglio.uiPosRiposo2=(sensoreTaglio.uiMax_cur*97L)/100;
						sensoreTaglio.ucFilter=0;
					}
					else if (sensoreTaglio.uiLetturaSensoreTaglio<sensoreTaglio.uiMin_cur+300){
						if (++sensoreTaglio.ucFilter>=3){
							sensoreTaglio.stato=enumStatiAnalizzaSensTaglio_WaitMin;
							sensoreTaglio.ucFilter=0;
						}
					}
					else{
						sensoreTaglio.ucFilter=0;
					}
					break;
			}
	}//if ( (!PrgRunning&&sensoreTaglio.ucEnableTest)

	if(sensoreTaglio.stato_taglio){
		switch(sensoreTaglio.stato_taglio){
			default:
			{
				break;
			}
			case enumOperazioneLama_Idle:
				break;
			case enumOperazioneLama_WaitTaglioOk:
				break;
			case enumOperazioneLama_BackgroundCtrl:

				// errore negativo=dac negativo=coltello scende
				// perci� se posizione coltello ideale<posizione coltello reale
				// errore � negativo, coltello scende...  quindi la posizione reale cala -->OK
				sensoreTaglio.iError=(signed int)sensoreTaglio.uiSetPoint;
				sensoreTaglio.iError-=(signed int)sensoreTaglio.uiLetturaSensoreTaglio;
				// waiting for blade initialization???
				if (sensoreTaglio.ucWaitInizializzazioneLama){
					sensoreTaglio.ucWaitInizializzazioneLama=0;
				}
				// se errore minore della tolleranza *2 (per avere un po' d'isteresi)
				// allora non comando dac
				if (sensoreTaglio.ucGoToFastMode&&(abs(sensoreTaglio.iError)<20*sensoreTaglio.iTolerance)){
					sensoreTaglio.ucGoToFastMode=0;
					sensoreTaglio.ucSaturaDac=0;
				}
				if (abs(sensoreTaglio.iError)<2*sensoreTaglio.iTolerance){
					sensoreTaglio.ucSaturaDac=0;
					break;
				}
				// se errore grande, cerco di posizionarmi lentamente
				else if (abs(sensoreTaglio.iError)>500){
					sensoreTaglio.ucGoToFastMode=1;
					sensoreTaglio.ucSaturaDac=1;
				}
				// se mi trovo sulla parte di ciclo di taglio che deve essere pilotata in modo contrario a quello usuale per far risalire il coltello...
				if (sensoreTaglio.ui_on_the_wrong_cut_side){
					sensoreTaglio.iError*=-1;
				}
				
				sensoreTaglio.lAux=sensoreTaglio.iError;
				sensoreTaglio.lAux*=sensoreTaglio.uiKp;
				sensoreTaglio.lAux/=512;
				sensoreTaglio.iDacValue=sensoreTaglio.lAux;
				sensoreTaglio.ucUpdateDac=1;
				break;
			case enumOperazioneLama_DownSlow_Init:
				sensoreTaglio.uiTimeout=0;
				sensoreTaglio.stato_taglio=enumOperazioneLama_DownSlow;
				sensoreTaglio.uiSetPoint=sensoreTaglio.uiMin_cur+100;
				break;

			case enumOperazioneLama_UpFast_Init:
				sensoreTaglio.uiTimeout=0;
				sensoreTaglio.stato_taglio=enumOperazioneLama_UpFast;
				sensoreTaglio.uiSetPoint=sensoreTaglio.uiPosRiposo;
				break;
			case enumOperazioneLama_UpFast:
				// max 10 secondi timeout
				if (++sensoreTaglio.uiTimeout>=(defTimeoutOperazioneLamaMs/PeriodoIntMs)){
					//sensoreTaglio.ui_on_the_wrong_cut_side=0;
					if (ucSendCommandPosTaglioMax519_Interrupt(128))
						sensoreTaglio.stato_taglio=enumOperazioneLama_BackgroundCtrl;
					break;
				}
				if (sensoreTaglio.uiLetturaSensoreTaglio>sensoreTaglio.uiSetPoint){
					//sensoreTaglio.ui_on_the_wrong_cut_side=0;
					if (ucSendCommandPosTaglioMax519_Interrupt(128))
						sensoreTaglio.stato_taglio=enumOperazioneLama_BackgroundCtrl;
					break;
				}
				// increase the tolerance used to switch to background state
				else if (sensoreTaglio.uiSetPoint-sensoreTaglio.uiLetturaSensoreTaglio<15*sensoreTaglio.iTolerance+50){
					//sensoreTaglio.ui_on_the_wrong_cut_side=0;
					if (ucSendCommandPosTaglioMax519_Interrupt(128))
						sensoreTaglio.stato_taglio=enumOperazioneLama_BackgroundCtrl;
					break;
				}
				sensoreTaglio.iError=(signed int)sensoreTaglio.uiSetPoint;
				sensoreTaglio.iError-=(signed int)sensoreTaglio.uiLetturaSensoreTaglio;
				// se mi trovo sulla parte di ciclo di taglio che deve essere pilotata in modo contrario a quello usuale per far risalire il coltello...
				if (sensoreTaglio.ui_on_the_wrong_cut_side){
					sensoreTaglio.iError*=-1;
				}
				
				sensoreTaglio.lAux=sensoreTaglio.iError;
				sensoreTaglio.lAux*=sensoreTaglio.uiKp;
				sensoreTaglio.lAux/=512;
				sensoreTaglio.iDacValue=sensoreTaglio.lAux;
				sensoreTaglio.ucUpdateDac=1;
				break;

			case enumOperazioneLama_DownFast_Init:
				sensoreTaglio.uiTimeout=0;
				sensoreTaglio.stato_taglio=enumOperazioneLama_DownFast;
				sensoreTaglio.uiSetPoint=sensoreTaglio.uiMin_cur+100;
				break;
			case enumOperazioneLama_DownSlow:
			case enumOperazioneLama_DownFast:
				// max 10 secondi timeout
				if (++sensoreTaglio.uiTimeout>=(defTimeoutOperazioneLamaMs/PeriodoIntMs)){
					if (ucSendCommandPosTaglioMax519_Interrupt(128))
						sensoreTaglio.stato_taglio=enumOperazioneLama_BackgroundCtrl;
					break;
				}
				if (sensoreTaglio.uiLetturaSensoreTaglio<sensoreTaglio.uiMin_cur){
					if (ucSendCommandPosTaglioMax519_Interrupt(128))
						sensoreTaglio.stato_taglio=enumOperazioneLama_BackgroundCtrl;
					break;
				}
				else if (sensoreTaglio.uiLetturaSensoreTaglio-sensoreTaglio.uiMin_cur<15*sensoreTaglio.iTolerance+50){
					if (ucSendCommandPosTaglioMax519_Interrupt(128))
						sensoreTaglio.stato_taglio=enumOperazioneLama_BackgroundCtrl;
					break;
				}
				else {
					sensoreTaglio.iError=(signed int)sensoreTaglio.uiSetPoint;
					sensoreTaglio.iError-=(signed int)sensoreTaglio.uiLetturaSensoreTaglio;
					// se mi trovo sulla parte di ciclo di taglio che deve essere pilotata in modo contrario a quello usuale per far risalire il coltello...
					if (sensoreTaglio.ui_on_the_wrong_cut_side){
						sensoreTaglio.iError*=-1;
					}
					sensoreTaglio.lAux=sensoreTaglio.iError;
					sensoreTaglio.lAux*=sensoreTaglio.uiKp;
					sensoreTaglio.lAux/=512;
					sensoreTaglio.iDacValue=sensoreTaglio.lAux;
					if (sensoreTaglio.stato_taglio==enumOperazioneLama_DownSlow){
						if (sensoreTaglio.iDacValue>sensoreTaglio.ucSaturaDac_value)
							sensoreTaglio.iDacValue=sensoreTaglio.ucSaturaDac_value;
						else if (sensoreTaglio.iDacValue<-(signed int)sensoreTaglio.ucSaturaDac_value)
							sensoreTaglio.iDacValue=-(signed int)sensoreTaglio.ucSaturaDac_value;
					}
					sensoreTaglio.ucUpdateDac=1;
				}
				break;
			case enumOperazioneLama_Distanzia_Init:
				sensoreTaglio.uiTimeout=0;
				sensoreTaglio.stato_taglio=enumOperazioneLama_Distanzia;
				sensoreTaglio.uiSetPoint=sensoreTaglio.ui_pos_coltello_spaziatura_ad_step;
				sensoreTaglio.uiCurTimeDistanziaNumT=0;
				break;
			case enumOperazioneLama_Distanzia:
				sensoreTaglio.uiCurTimeDistanziaNumT++;
				// max 10 secondi timeout
				if (++sensoreTaglio.uiTimeout>=(defTimeoutOperazioneLamaMs/PeriodoIntMs)){
					sensoreTaglio.uiLastTimeDistanziaNumT=sensoreTaglio.uiCurTimeDistanziaNumT;
					if (ucSendCommandPosTaglioMax519_Interrupt(128)){
						sensoreTaglio.stato_taglio=enumOperazioneLama_BackgroundCtrl;
					}
					break;
				}
				// se il sensore ha lettura inferiore al set point
				// (coltello pi� basso di quanto voluto)
				// devo comunque controllare il distanziatore, perch� ad es nella fase di setup
				// pu� essere che debba regolare in su/gi�...
				// pertanto sospendo il controllo solo se sono nella tolleranza specificata 15*iTolerance
				if (abs((signed int)sensoreTaglio.uiLetturaSensoreTaglio-(signed int)sensoreTaglio.ui_pos_coltello_spaziatura_ad_step)<15*sensoreTaglio.iTolerance){
					sensoreTaglio.uiLastTimeDistanziaNumT=sensoreTaglio.uiCurTimeDistanziaNumT;
					if (ucSendCommandPosTaglioMax519_Interrupt(128)){
						sensoreTaglio.stato_taglio=enumOperazioneLama_BackgroundCtrl;
					}
					break;
				}
				else {
					sensoreTaglio.iError=(signed int)sensoreTaglio.ui_pos_coltello_spaziatura_ad_step;
					sensoreTaglio.iError-=(signed int)sensoreTaglio.uiLetturaSensoreTaglio;
					// se mi trovo sulla parte di ciclo di taglio che deve essere pilotata in modo contrario a quello usuale per far risalire il coltello...
					if (sensoreTaglio.ui_on_the_wrong_cut_side){
						sensoreTaglio.iError*=-1;
					}
					
					sensoreTaglio.lAux=sensoreTaglio.iError;
					sensoreTaglio.lAux*=sensoreTaglio.uiKp;
					sensoreTaglio.lAux/=512;
					sensoreTaglio.iDacValue=sensoreTaglio.lAux;
					sensoreTaglio.ucUpdateDac=1;
				}
				break;

				

			case enumOperazioneLama_Pretaglio_Init:
				sensoreTaglio.uiTimeout=0;
				sensoreTaglio.stato_taglio=enumOperazioneLama_Pretaglio;
				sensoreTaglio.uiSetPoint=sensoreTaglio.uiPosPretaglio;
				break;
			case enumOperazioneLama_Pretaglio:
				// max 10 secondi timeout
				if (++sensoreTaglio.uiTimeout>=(defTimeoutOperazioneLamaMs/PeriodoIntMs)){
					//sensoreTaglio.ui_on_the_wrong_cut_side=0;
					if (ucSendCommandPosTaglioMax519_Interrupt(128))
						sensoreTaglio.stato_taglio=enumOperazioneLama_BackgroundCtrl;
					break;
				}
				if (sensoreTaglio.uiLetturaSensoreTaglio<sensoreTaglio.uiPosPretaglio){
					//sensoreTaglio.ui_on_the_wrong_cut_side=0;
					if (ucSendCommandPosTaglioMax519_Interrupt(128))
						sensoreTaglio.stato_taglio=enumOperazioneLama_BackgroundCtrl;
					break;
				}
				else if (sensoreTaglio.uiLetturaSensoreTaglio-sensoreTaglio.uiPosPretaglio<15*sensoreTaglio.iTolerance){
					//sensoreTaglio.ui_on_the_wrong_cut_side=0;
					if (ucSendCommandPosTaglioMax519_Interrupt(128))
						sensoreTaglio.stato_taglio=enumOperazioneLama_BackgroundCtrl;
					break;
				}
				else {
					sensoreTaglio.iError=(signed int)sensoreTaglio.uiPosPretaglio;
					sensoreTaglio.iError-=(signed int)sensoreTaglio.uiLetturaSensoreTaglio;
					// se mi trovo sulla parte di ciclo di taglio che deve essere pilotata in modo contrario a quello usuale per far risalire il coltello...
					if (sensoreTaglio.ui_on_the_wrong_cut_side){
						sensoreTaglio.iError*=-1;
					}
					sensoreTaglio.lAux=sensoreTaglio.iError;
					sensoreTaglio.lAux*=sensoreTaglio.uiKp;
					sensoreTaglio.lAux/=512;
					sensoreTaglio.iDacValue=sensoreTaglio.lAux;
					sensoreTaglio.ucUpdateDac=1;
				}
				break;
			case enumOperazioneLama_Taglio_Init:
				sensoreTaglio.uiTimeout=0;
				sensoreTaglio.stato_taglio=enumOperazioneLama_Taglio;
				sensoreTaglio.uiSetPoint=sensoreTaglio.uiPosRiposo;
				break;
			case enumOperazioneLama_Taglio:
				sensoreTaglio.stato_taglio=enumOperazioneLama_WaitTaglioLow;
				// comando dac rapido gi�
				sensoreTaglio.iDacValue=-127;
				sensoreTaglio.ucUpdateDac=1;
				break;
			case enumOperazioneLama_WaitTaglioLow:
				// max 10 secondi timeout
				if (++sensoreTaglio.uiTimeout>=(defTimeoutOperazioneLamaMs/PeriodoIntMs)){
					if (ucSendCommandPosTaglioMax519_Interrupt(128))
						sensoreTaglio.stato_taglio=enumOperazioneLama_BackgroundCtrl;
					break;
				}
				// comando dac rapido basso -127--> scende alla max velocit�
				sensoreTaglio.iDacValue=-127;
				if (sensoreTaglio.ui_on_the_wrong_cut_side){
					sensoreTaglio.iDacValue*=-1;
				}
				sensoreTaglio.ucUpdateDac=1;
				if (sensoreTaglio.uiLetturaSensoreTaglio<sensoreTaglio.uiMin_cur+3*sensoreTaglio.iTolerance+200){
					if (ui_separate_coil_during_cut()){
						sensoreTaglio.ucLastOpWasOnDistanziatore=1;
						// marco che mi trovo sulla parte di ciclo di taglio che deve essere pilotata in modo contrario a quello usuale per far risalire il coltello...
						// test michele...
						//sensoreTaglio.ui_on_the_wrong_cut_side=1;
						sensoreTaglio.ui_on_the_wrong_cut_side=!sensoreTaglio.ui_on_the_wrong_cut_side;
						sensoreTaglio.stato_taglio=enumOperazioneLama_Taglio_Distanzia;
						v_init_handle_wait_one_coil();						
					}
					else{
						sensoreTaglio.stato_taglio=enumOperazioneLama_WaitTaglioHi_0;
					}
				}
				break;
			// if after cut we should space coils, this state handles the case
			case enumOperazioneLama_Taglio_Distanzia:
				// max 10 secondi timeout
				if (++sensoreTaglio.uiTimeout>=(defTimeoutOperazioneLamaMs/PeriodoIntMs)){
					//sensoreTaglio.ui_on_the_wrong_cut_side=0;
					if (ucSendCommandPosTaglioMax519_Interrupt(128))
						sensoreTaglio.stato_taglio=enumOperazioneLama_BackgroundCtrl;
					break;
				}				
				// spacing position reached?-->precut position
				if (sensoreTaglio.uiLetturaSensoreTaglio>sensoreTaglio.uiPosPretaglio){
					v_handle_wait_one_coil();
					if (ui_handle_wait_one_coil_ok()){					
						//sensoreTaglio.ui_on_the_wrong_cut_side=1;
						// no dac saturation! sensoreTaglio.ucSaturaDac=1;
						sensoreTaglio.stato_taglio=enumOperazioneLama_Distanzia_Init;
						break;
					}
				}
				else if (sensoreTaglio.uiPosPretaglio-sensoreTaglio.uiLetturaSensoreTaglio<200){
					//sensoreTaglio.ui_on_the_wrong_cut_side=1;
					v_handle_wait_one_coil();
					if (ui_handle_wait_one_coil_ok()){					
						// no dac saturation! sensoreTaglio.ucSaturaDac=1;
						sensoreTaglio.stato_taglio=enumOperazioneLama_Distanzia_Init;
						break;
					}
				}
				// faccio in modo di avere errore negativo per comandare dac negativo cos� coltello "continua il giro" dalla parte giusta
				#if 0
					sensoreTaglio.iError=(signed int)sensoreTaglio.uiLetturaSensoreTaglio;
					sensoreTaglio.iError-=(signed int)sensoreTaglio.uiPosPretaglio;
				#else
					// test michele...
					sensoreTaglio.iError=(signed int)sensoreTaglio.uiPosPretaglio;
					sensoreTaglio.iError-=(signed int)sensoreTaglio.uiLetturaSensoreTaglio;
					if (sensoreTaglio.ui_on_the_wrong_cut_side){
						sensoreTaglio.iError*=-1;
					}
				#endif
				
				sensoreTaglio.lAux=sensoreTaglio.iError;
				sensoreTaglio.lAux*=sensoreTaglio.uiKp;
				sensoreTaglio.lAux/=512;
				sensoreTaglio.iDacValue=sensoreTaglio.lAux;
				sensoreTaglio.ucUpdateDac=1;
				break;
				
			case enumOperazioneLama_WaitTaglioHi_0:
			case enumOperazioneLama_WaitTaglioHi_1:
				// max 10 secondi timeout
				if (++sensoreTaglio.uiTimeout>=(defTimeoutOperazioneLamaMs/PeriodoIntMs)){
					if (ucSendCommandPosTaglioMax519_Interrupt(128))
						sensoreTaglio.stato_taglio=enumOperazioneLama_BackgroundCtrl;
					break;
				}
				if (sensoreTaglio.stato_taglio==enumOperazioneLama_WaitTaglioHi_0){
					
					//if (sensoreTaglio.uiLetturaSensoreTaglio>sensoreTaglio.uiPosRiposo2+100){
					// diminuisco la soglia per evitare problemi!!! altrimenti il coltello "fa il giro" pi� volte, se la soglia non � corretta...
					// 3%del valore massimo sono circa 130 step, quindi imporre una soglia di 100 � troppo...
					if (sensoreTaglio.uiLetturaSensoreTaglio>sensoreTaglio.uiPosRiposo2+70){
						if (sensoreTaglio.ui_on_the_wrong_cut_side){
							sensoreTaglio.ui_on_the_wrong_cut_side=0;
							sensoreTaglio.stato_taglio=enumOperazioneLama_BackgroundCtrl;
							break;
						}
						sensoreTaglio.stato_taglio=enumOperazioneLama_WaitTaglioHi_1;
						break;
					}
					// faccio in modo di avere errore negativo per comandare dac negativo cos� coltello scende
					sensoreTaglio.iError=(signed int)sensoreTaglio.uiLetturaSensoreTaglio;
					sensoreTaglio.iError-=(signed int)sensoreTaglio.uiPosRiposo2;
				}
				else{
					// faccio il giro e supero pos riposo2 (97%) per almeno 10 step fissi verso il basso
					// a questo punto passo in idle state e mi porto verso il punto di riposo
					// if (sensoreTaglio.uiLetturaSensoreTaglio<sensoreTaglio.uiPosRiposo2-10){
					//if (sensoreTaglio.uiLetturaSensoreTaglio<sensoreTaglio.uiPosRiposo2+80){
					// diminuisco la soglia per evitare problemi...
					if (sensoreTaglio.uiLetturaSensoreTaglio<sensoreTaglio.uiPosRiposo2+50){
						sensoreTaglio.ui_on_the_wrong_cut_side=0;
						sensoreTaglio.stato_taglio=enumOperazioneLama_BackgroundCtrl;
						break;
					}
					sensoreTaglio.iError=(signed int)sensoreTaglio.uiPosRiposo;
					sensoreTaglio.iError-=(signed int)sensoreTaglio.uiLetturaSensoreTaglio;
				}
				// se mi trovo sulla parte di ciclo di taglio che deve essere pilotata in modo contrario a quello usuale per far risalire il coltello...
				if (sensoreTaglio.ui_on_the_wrong_cut_side){
					sensoreTaglio.iError*=-1;
				}
				
				sensoreTaglio.lAux=sensoreTaglio.iError;
				sensoreTaglio.lAux*=sensoreTaglio.uiKp;
				sensoreTaglio.lAux/=512;
				sensoreTaglio.iDacValue=sensoreTaglio.lAux;
				// cerco di andare veloce in queste fasi
				if (sensoreTaglio.iDacValue>-35)
					sensoreTaglio.iDacValue=-35;
				sensoreTaglio.ucUpdateDac=1;
				break;
		}
		if (sensoreTaglio.ucUpdateDac){
			sensoreTaglio.ucUpdateDac=0;
			if (sensoreTaglio.ucSaturaDac){
				if (sensoreTaglio.iDacValue>sensoreTaglio.ucSaturaDac_value)
					sensoreTaglio.iDacValue=sensoreTaglio.ucSaturaDac_value;
				else if (sensoreTaglio.iDacValue<-(signed int)sensoreTaglio.ucSaturaDac_value)
					sensoreTaglio.iDacValue=-(signed int)sensoreTaglio.ucSaturaDac_value;
			}
			else{
				if (sensoreTaglio.iDacValue>127)
					sensoreTaglio.iDacValue=127;
				else if (sensoreTaglio.iDacValue<-127)
					sensoreTaglio.iDacValue=-127;
			}
			sensoreTaglio.ucDacValue=sensoreTaglio.iDacValue+128;
			ucSendCommandPosTaglioMax519_Interrupt(sensoreTaglio.ucDacValue);
		}
	}


	// Se overflow nel conteggio da encoder, ripristino la situazione.
	if (encVal<newEnc){
		newEnc = 0xFFFF-newEnc+encVal+1;
		// Se il numero di conteggi � troppo elevato, considero	che vi sia stato un errore.
		if (newEnc>10000)
			goto SortByErrorEncoder; // Esco riportando tutto alla situazione attuale.
		oldEncMis = oldEncLav=0;
	}
	else{
		// Se posso aggiornare velocit� e gapTime, lo faccio.
		// GapTime contiene il numero di conteggi effettuati tra un interrupt	 e l' altro.
		if (updVel) {
			// Dato che la routine di interrupt entra in azione 1 volta ogni millisecondo,
			//	incremento una variabile che indica il numero
			// di interruzioni.
			gapT++;
			// Tengo conto del numero di giri dell' encoder.
			nImpulsi += encVal-newEnc;
		}
		newEnc = encVal;
	}

#ifdef defSimulaIngressoMisuraDinamica
	ulContaMezzoGiro+=newEnc - oldEncLav;
	if (ulContaMezzoGiro>(macParms.lenSemigiro*150L)/100)
		ulContaMezzoGiro-=macParms.lenSemigiro;
	ucMisuraDinamicaValidaSimulazione=((ulContaMezzoGiro > (macParms.lenSemigiro*84L)/100 )&&(ulContaMezzoGiro  < (macParms.lenSemigiro*116L)/100 ));
#endif

	if (withMis) {
// se devo usare distanziatore, meglio tenere frequentemente sott'occhio
// le quote, per non sbagliare il punto di discesa...
		if (    criticLav
			|| (!(cntAvv % INTERVAL_TEST_LAV))
			|| TaglioAsincrono
			|| (defMisuraDinamicaValida)
			||((hDist.uiEnabled_interrupt)&&def_do_distanz_ctrl)
			|| hDist.ucRefresh
		    ) {
			if ((avvStatus == AVV_IDLE)&&!TaglioAsincrono){
				++cntAvv;
				goto endOfTimerProcedure;
			}

		actSpezInPez += gapImp = newEnc - oldEncLav;
		actSpezAvv += gapImp;
		oldEncLav = newEnc;

		actLenImp += gapImp;

		if (!(criticLav||TaglioAsincrono)) {
			if (spezFilo.flags & FLG_SPEZ_INIT) {
				while (actSpezAvv >= macParms.lenSemigiro) {
					// cambia lo spezzone attuale
					actSpezInPez -= (unsigned long)(actSpezAvv - macParms.lenSemigiro);
					resLastSpezUp = (unsigned long)(resLastSpezUp+(unsigned long)((actSpezInPez* actRo ) / MAGN_LEN));
					rdQueSpez = (rdQueSpez + 1) % MAX_SPEZ_FILO;
					actSpezInPez = actSpezAvv -= macParms.lenSemigiro;
					actRo = spezFilo.ro[rdQueSpez];
                    hDist.actRoDist=(120*hDist.actRoDist+8*actRo)>>7;
				}
			}

			actRes = resLastSpezUp + (actSpezInPez * actRo) / MAGN_LEN;

			if ((actRes == 0)&&!TaglioAsincrono) {
				++cntAvv;
				goto endOfTimerProcedure;
			}
			// Calcolo in preLenImp la stima del numero di impulsi mancanti alla
			// fine della resistenza.
			// Se ho oltrepassato il valore desiderato di resistenza,
			// indico che devo partire col pretaglio-taglio.
			// [actRo]=URP*MAGN_LEN/Impulsi
			if (actRes>=TARGET_URP)
				// Se la resistenza ha valore maggiore o uguale a quello nominale, devo partire col pretaglio.
				preLenImp=0;
			else
				// Altrimenti faccio una stima del numero di impulsi che mancano alla fine della resistenza.
				preLenImp =((((long)TARGET_URP - (long)actRes) * MAGN_LEN) / (long)actRo);

		}
		// Se sto aspettando di tagliare, aggiorno il valore della resistenza.
		else{
			// Non posso fare actRes+= .. gapImp* perch� se gapImp
			// � troppo basso, sommo ad actRes un valore che � zero, per cui
			// non raggiungo mai TARGET_URP. Questo capita se devo
			// produrre resistenze di valore elevato (1000 ohm)
			// con velocit� di produzione normali (2500-3000 rpm).
			// Uso preLenImp per contare gli impulsi che arrivano dopo il pretaglio.
			preLenImp+=gapImp;
			// Vedo se la resistenza prodotta � aumentata di almeno un' unit�
			nimpu=(unsigned long)(preLenImp* actRo) / MAGN_LEN;
			// Se � il caso, aumento la resistenza prodotta e sottraggo il numero di impulsi considerati da preLenImp.
			if (nimpu){
				actRes+=nimpu;
				preLenImp-=(nimpu*MAGN_LEN)/actRo;
			}
		}
		vHandleDistanziatore_Interrupt();

		switch (avvStatus) {
			case AVV_WAIT_PRE:
				// Entro nella fase di pretaglio se ho raggiunto il numero di impulsi dell' anticipo di pretaglio
				// e se ho eliminato il primo moncone
				// o se � attivo il taglio asincrono.
				if ( (  (preLenImp <= anticipoPretaglio)
					  && PrgRunning
					  &&(spiralatrice.PrimoPezzoEliminato||hprg.theRunning.empty)
					 )
					 ||
					 TaglioAsincrono
					){
					if (TaglioAsincrono){
						v_reset_cutter_position();
					}
					// Attivazione del comando anche se si usa regolazione analogica
					OutDig = outDigMis |= ODG_PRETAGLIO;
					if (!macParms.ucUsaSensoreTaglio){
					}
					else{
						vAttivaOperazioneLama(enumCiclicaLama_pretaglia);
					}
					avvStatus = AVV_WAIT_TAG;
					// Armo un timer, che uso solo nel caso di taglio manuale.
					if (ucEliminaPrimoPezzo){
						avvSleep=SLEEP_TAGLIO_PRIMO_PEZZO;
					}
					else{
						avvSleep=SLEEP_TAGLIO_ASINC;
					}

					criticLav = 1;
					// Uso preLenImp per contare gli impulsi che arrivano
					// dopo il pretaglio; pertanto adesso devo partire
					// certamente da zero.
					preLenImp=0;
				}
				break;

			case AVV_WAIT_TAG:
				// Dato che sono in posizione di pretaglio, setto per sicurezza l' uscita dell' 8255.
				OutDig=outDigMis |= ODG_PRETAGLIO;
				if (!macParms.ucUsaSensoreTaglio){
				}
				// Entro nella fase di taglio se la resistenza che sto producendo ha raggiunto o superato quella desiderata.
				if ( (   PrgRunning
                      &&!TaglioAsincrono
					  &&(spiralatrice.PrimoPezzoEliminato||hprg.theRunning.empty)
					  &&((actRes >= TARGET_URP)||(actLenImp>=targetLenImp))
					 )
					// Se taglio manuale, entro solo dopo che � passato il tempo di pretaglio.
					||(TaglioAsincrono&&!avvSleep)
					) {
					// Se arrivo qui, devo impartire il comando di taglio e disabilitare quello di pretaglio.
					outDigMis |= ODG_TAGLIO; /* Attivo il comando di taglio. */
					outDigMis &= ~ODG_PRETAGLIO;/* Disattivo il comando di pretaglio. */
					/* Attivazione fisica del comando */
					OutDig = outDigMis;
					if (macParms.ucUsaSensoreTaglio){
						vAttivaOperazioneLama(enumCiclicaLama_taglia);
					}
					else{
					}
					avvStatus = AVV_WAIT_TAGLIO;
					avvSleep=(unsigned short)SLEEP_MAX_TAGLIO;
				}
				break;

			case AVV_WAIT_TAGLIO:
				// Dato che sono in posizione di taglio, setto per sicurezza l' uscita dell' 8255.
				OutDig=outDigMis |= ODG_TAGLIO;
				if (macParms.ucUsaSensoreTaglio&&ucIsAttivaOperazioneLama()){
					break;
				}
				else{
				}
				// Modifica del 14/03/1996: si disattiva il comando di taglio quando:
				// - � passato il tempo di taglio;
				// - oppure l' ingresso digitale IDG_TAGLIO_HI va ad 1
				//    (ad indicare che il coltello ha raggiunto la posizione alta.)
				// Segnalibro: IDG_TAGLIO_HI_USATO
				// Metto anche un timeout per disabilitare il comando di taglio.
				if (    macParms.ucUsaSensoreTaglio
					||(!macParms.ucUsaSensoreTaglio
					   && ((InpDig&IDG_TAGLIO_HI)||!avvSleep)
					   )
				    ){
					// se c'era una distanziazione in corso, devo resettarne lo stato...
					if (hDist.ucDistanziaInCorso){
						v_reset_stato_distanziatore(enum_reset_dist_reset_numero_distanziazioni);
					}
                    // reinizializzo lo stato di pilotaggio bandella
                    vCommandSwitchBandella_Interrupt();
                
					criticLav = 0;
					outDigMis &= ~ODG_TAGLIO; // Disattivo il comando di taglio.
					// Attivazione fisica del comando */
					OutDig = outDigMis;
					// Indico che mancano ancora molti impulsi alla fine del pezzo.
					preLenImp=0xFFFFFFL;
					if (ucEliminaPrimoPezzo||TaglioAsincrono){
						ucEliminaPrimoPezzo=0;
						spiralatrice.PrimoPezzoEliminato=1;
					}

					// Se ho attuato un taglio asincrono, non eseguo il conteggio del pezzo.
					if (TaglioAsincrono){
						TPR.ulTempoPerResistenzaCorrente=0;
						// Porto a zero tutti i parametri relativi al pezzo correntemente in produzione.
						// Lo faccio comunque, anche se un programma non � in esecuzione.
						actLenImp = 0;
						oldEncLav = oldEncMis = newEnc = iReadCounter();
						actSpezAvv=0;
						actSpezInPez = 0;

						// se effettuo un taglio asincrono, devo aggiustare opportunamente le variabili del distanziatore...
						// faccio in modo che il distanziatore ricominci con resistenza nulla
						// quindi sul coltello ci sar� un mozzicone di resistenza
						if (macroIsCodiceProdotto_Distanziatore(&hprg.theRunning)){
							vHandleTaglioAsincrono_Distanziatore();
						}
						else{
							resLastSpezUp= 0;
							actRes      = 0;
						}
						
						impFromMis = 0;
						TaglioAsincrono=0;
						// Rimetto lo stato dell' avvolgitore in attesa di pretaglio.
						if (PrgRunning)
							avvStatus = AVV_WAIT_PRE;
						else
							avvStatus = AVV_IDLE;
						break;
					}

					// salvo il tempo impiegato a produrre la res corrente
					TPR.ulTempoPerResistenzaUltima=TPR.ulTempoPerResistenzaCorrente;
					TPR.ulTempoPerResistenzaCorrente=0;
					TPR.uiNumPezziFatti++;

					lastPzLenImp = actLenImp;
					lastPzRes =(unsigned int) actRes;
					txNewPz = 1;

					// salvo nel log l'evento, andando all'indietro con l'indice
					if (--production_log.ucIdx>=sizeof(production_log.datas)/sizeof(production_log.datas[0]))
						production_log.ucIdx=sizeof(production_log.datas)/sizeof(production_log.datas[0])-1;
					plog=&production_log.datas[production_log.ucIdx];
					plog->event_type=let_cut;
					plog->u.cut.ulCutImp=actRes;
					plog->u.cut.ulExpectedImp=TARGET_URP;
					plog->misDig=misDig;

					// Se il numero di impulsi e' troppo grande, errore.
					if (actLenImp>=targetLenImp){
						sLCD.ulInfoAlarms[0]=actLenImp;
						sLCD.ulInfoAlarms[1]=targetLenImp;
						// Allarme: misura fuori tolleranza
						alarms |= ALR_TOLMIS;
						// E' effettivamente un allarme valido.
						alr_status=alr_status_alr;
						avvStatus = AVV_IDLE;
						misStatus = MIS_OFF_LINE;
						break;
					}
					// Conto un pezzo in pi� solo se il bit contapezzi vale 1.
					actNumPezzi+=BitContaPezzi;
					/* sicuramente il prossimo va contato */
					BitContaPezzi=1;
					
					//if (BitContaPezzi)

					// Se il lotto � finito, lo segnalo al programma principale che decider� se proseguire
					// col nuovo lotto o no.
					if (actNumPezzi >= nvram_struct.commesse[spiralatrice.runningCommessa].n+nvram_struct.commesse[spiralatrice.runningCommessa].magg) {
						// La routine si mette in IDLE state, in attesa della decisione del main.
						avvStatus = AVV_IDLE;
						misStatus = MIS_OFF_LINE;
						// lotto finito--> non segnalo errore in caso di tempo misura insufficiente
						// perch� potrei essere a cavallo di un periodo di misura...
						AlarmOnTimeout=0;
						// Aggiorno il valore della resistenza del pezzo.
						actRes = resLastSpezUp + (actSpezInPez * actRo) / MAGN_LEN;
						// Segnalo che il programma corrente � terminato correttamente.
						RunningPrgEndOk=1;
					}
					else {
						actLenImp = 0;
						actSpezAvv=0;
						actSpezInPez = 0;
						resLastSpezUp = 0;
						actRes = 0;
						avvStatus = AVV_WAIT_PRE;
					}

					break;
				}/* Fine if. */
			}/* Fine switch avvStatus. */
		}
		if (   (PrgRunning)
			&& (
			 (!(cntMis % INTERVAL_TEST_MIS))
			  || (misStatus != MIS_OFF_LINE)
			  || (defMisuraDinamicaValida)
			 )
		   ) {
			if (!(cntMis % INTERVAL_TEST_MIS)){
				gapImp = newEnc - oldEncMis;
				oldEncMis = newEnc;
				impFromMis += gapImp;
				// Se la torretta ha fatto un semigiro abbondante senza
				// che sia stato possibile aggiornare la misura resistiva,
				// ne prolungo per continuit� il valore.
				// Lo faccio solo se non sto effettuando una misura.
				// CREDO CHE SE NON RIESCE A FARE UNA MISURA DOVREBBE DARE
				// ALLARME SEMPRE E COMUNQUE!!!!
			}
			switch (misStatus) {
				case MIS_OFF_LINE:
					// Se l' ingresso IDG_MIS_VALID1 va basso, posso effettuare una misura resistiva.
					if (defMisuraDinamicaValida&& (!alarms)) {
						// Setto la portata adeguata.
						actDynPortata = nvram_struct.commesse[spiralatrice.runningCommessa].defPortata;
						pTaratura=&(macParms.tarParms[N_CANALE_OHMETRO_DYN][actDynPortata]);
						vUpdateDacPortValue((enum_select_dac_portata)actDynPortata);
						// Porto a zero il numero di impulsi giunti dopo l' ultima misura.
						impFromMis = 0;
						misStatus = MIS_COMM_MUX;
						// Attendo che si esaurisca la rampa del segnale, e nel
						// frattempo si effettua anche il cambio portata.
						misSleep = SLEEP_RAMPA_E_PORTATA;
						// Indico che sto effettuando le letture dall' adc.
						criticMis = 1;
						numSampleRo=0;
						AccumRo=0;
						ucActBadSpezzConsec=0;
					}
					else {
						if (impFromMis > macParms.lenSemigiro + RITARDO_MISURA) {
							sLCD.ulInfoAlarms[0]=impFromMis;
							sLCD.ulInfoAlarms[1]=macParms.lenSemigiro + RITARDO_MISURA;

							// allarme, misura non triggerata oppure la
							// ruota ha girato troppo dall'ultima misura
							// effettuata (velocita` eccessiva)
							alarms |= ALR_NOTMIS;
							/* E' effettivamente un allarme valido. */
							alr_status=alr_status_alr;
							avvStatus = AVV_IDLE;
							misStatus = MIS_OFF_LINE;
						}
					}
					break;

				case MIS_COMM_MUX:
					if (misSleep)
						break;
					misDig=0;

					// solo se la misura dinamica � ancora valida ha senso che io faccia conti...
					if (defMisuraDinamicaNonValida) {
					}
					// arrivo qui se misura valida...
					else{
						misDig=uiLetturaMisuraDinamica_Interrupt;
						testRo=misDig;
						// se coefficienti di taratura validi...
						if (pTaratura->ksh){
							// Tolgo l' offset dello shunt.
							testRo -= pTaratura->osh;
							// Moltiplico per il fattore correttivo.
							testRo *= pTaratura->ksh;
							// Elimino il fattore di scala usato per avere maggior precisione.
							//testRo /= 1024;
							testRo >>=10;
						}
						// devo ottenere il numero di ohm/impulso
						// allora:
						// il convertitore fornisce a seconda delle portate
						// 5V->5ohm, 5V=50ohm, 5V=500ohm, 5V=5000ohm
						// nella prima portata ho	1 ohm/volt,
						// nella seconda			10   ohm/volt
						// nella terza				100  ohm/volt
						// nella quarta				1000 ohm/volt

						// il fondo scala in step del conv a/d � 4095
						// nel caso precedente era a 12 bit fs 5V,
						// adesso � 13 bit fs 10V, quindi ho sempre 5/4095 V/step

						// la lunghezza dello spezzone � mezzo giro di ruota=2000impulsi

						// siccome devo ottenere la resistenza specifica ohm/impulso
						// ohm/impulso=Valore in step*(volt/step)*(ohm/volt)/lunghezza spezzone in impulsi
						// volt/step=5/4095
						// ohm/volt=1,10,100,1000
						// lunghezza spezzone in impulsi=2000
						// quindi
						//                             5                       1
						// ohm/impulso=Valore in step*---- * (1,10,100,1000)* ----
						//                            4095                    2000
						// ohm/impulso=Valore in step*(0.1,1,10,100,1000)*(5/4095*1/2000)
						// ohm/impulso=Valore in step*(0.1,1,10,100,1000)*(5/4095*1/2000)
						// ohm/impulso=Valore in step*(0.1,1,10,100,1000)*(1/1638000)
						// io per� in interrupt lavoro in microohm/impulso
						// pertanto
						// microohm/impulso=Valore in step*(0.1,1,10,100,1000)*(1'000'000/1'638'000)
						// per fare questo conto senza perdere cifre decimali uso questo sistema
						//
						// 1'000'000/1'638'000 = 1'000'000/(16380 *100)=10'000/16380
						// 10'000/16380=circa 10'000/16384=10'000/(128*128)
						// 10'000/(128*128)=(1000/128)*(10/128)
						// (1000/128)=500/64= *500 e poi >>6
						// il fattore (10/128) lo uso quando considero la portata
						// infine dobbiamo ricordare che uso un fattore di amplificazione
						// per non perdere cifre significative, e questo fattore
						// � pari a MAGN_LEN=100'000
						// Per tener conto dello spessore dei contatti (0.857 mm)
						// attenzione qui viene usato il famoso MAGN_LEN=100'000

						// ATTENZIONE: IL FONDO SCALA NON E' PIU' 5V, MA 8 VOLT
						// PERTANTO IL NUMERO DI OHM/V NON � PIU' 1 10 100 1000
						// MA 0.625 6.25 62.5 625
						// PERTANTO NE TENGO CONTO DIMINUENDO IL FATTORE MOLTIPLICATIVO
						// 100857*0.625=63036 CIRCA
 						// testRo = testRo * 63036L;

						// elimino la compensazione dello spessore dei contatti!!!
 						testRo = testRo * 62500L;
						// ADESSO moltiplico per (1000/128)=500/64= *500 e poi >>6
						// (prima parte del famoso fattore 1'000'000/1'638'000)
						testRo >>= 6; // / 64   500/64=1000/128
						testRo *= 500; // *500
						// portata zero, devo moltiplicare per 1 e dividere per 128
						// faccio diviso 128 direttamente
						// per questioni numeriche, per non perdere cifre significative
						// infine trasformo da resistenza[ohm] a unit� di resistenza programma 10'000=1ohm
						if (actDynPortata == 4) { // 1
							testRo >>= 7; // / 128
							testRo /= urpToRes;
						}
						// prima portata, devo moltiplicare per 10 e dividere per 128
						// faccio prima diviso 8, poi per 10, poi diviso 16
						// per questioni numeriche, per non perdere cifre significative
						// infine trasformo da resistenza[ohm] a unit� di resistenza programma 10'000=1ohm
						else if (actDynPortata == 3) { // 10
							testRo >>= 3; // / 8
							testRo *= 10;
							testRo >>= 4; // / 16
							testRo /= urpToRes;
						}
						// seconda portata, devo moltiplicare per 100 e dividere per 128
						else if (actDynPortata == 2) { // 100
							testRo /= urpToRes;
							testRo *= 100;
							testRo >>= 7; // / 128
						}
						// terza portata, devo moltiplicare per 1000 e dividere per 128
						else if (actDynPortata == 1) { // 1000 */
							testRo /= urpToRes;
							testRo >>= 3; // / 8
							testRo *= 1000;
							testRo >>= 4; // / 16
						}
						// quarta portata, devo moltiplicare per 10000 e dividere per 128
						else {  // 10000
							testRo /= urpToRes;
							if (testRo & 0xfffc0000L) {
								testRo *= 100;
								testRo >>= 5; // / 32
								testRo *= 100;
								testRo >>= 2; // / 4
							}
							else {
								testRo *= 10000;
								testRo >>= 7; // / 128
							}
						}
						// applico la compensazione di temperatura prima di verificare la tolleranza......
						if (macroIsCodiceProdotto_TemperatureCompensated(&hprg.theRunning)){
							testRo_verify_tolerance=i_ro_compensate_temperature(testRo);
						}				
						else{
							testRo_verify_tolerance=testRo;
						}
						
						// se la misura di ro sembra falsa (anche tenendo conto della compensazione di temperatura...)...
						if((testRo_verify_tolerance>ulMaxTrueRo)||(testRo_verify_tolerance<ulMinTrueRo)){
							// salvo nel log l'evento, andando all'indietro con l'indice
							if (--production_log.ucIdx>=sizeof(production_log.datas)/sizeof(production_log.datas[0]))
								production_log.ucIdx=sizeof(production_log.datas)/sizeof(production_log.datas[0])-1;
							plog=&production_log.datas[production_log.ucIdx];
							plog->event_type=let_bad_measure;
							plog->u.bad_measure.ulRo=testRo_verify_tolerance;
							plog->u.bad_measure.ulNomRo=nomRo;
							plog->misDig=misDig;

							if(++ucActBadSpezzConsec>ucMaxBadSpezzConsec){
								sLCD.ulInfoAlarms[0]=testRo_verify_tolerance;
								sLCD.ulInfoAlarms[1]=ulMaxTrueRo;
								sLCD.ulInfoAlarms[2]=ulMinTrueRo;
								sLCD.ulInfoAlarms[3]=nomRo;
								alarms |= ALR_TOLMIS;
								/* E' effettivamente un allarme valido. */
								alr_status=alr_status_alr;
								avvStatus = AVV_IDLE;
								misStatus = MIS_OFF_LINE;
								break;
							}
						}
						else{
							// Azzero il numero attuale di spezzoni bad consecutivi.
							ucActBadSpezzConsec=0;
						}
					}


					// Se � scaduto il tempo di misura valida...
					if (defMisuraDinamicaNonValida) {
						// Indico che ho terminato di leggere dall' adc.
						criticMis = 0;
						// Se non ho acquisito sufficienti campioni:
						// allarme se timeout abilitato
						// altrimenti ignoro la misura (capita se e' la prima misura di una commessa in automatico)
						if (numSampleRo==0){
							// Se non devo segnalare timeout...
							if (AlarmOnTimeout==0){
								// Indico che d' ora in poi lo debbo fare...
								AlarmOnTimeout=1;
								// Mi porto in stato off_line, in attesa della prossima misura.
								misStatus = MIS_OFF_LINE;
								break;
							}
							// allarme: il periodo di misura valida e` terminato
							// senza lasciare il tempo di effettuarla
							alarms |= ALR_TIMMIS;
							// E' effettivamente un allarme valido.
							alr_status=alr_status_alr;
							avvStatus = AVV_IDLE;
							misStatus = MIS_OFF_LINE;
						}
						else{
							ro=AccumRo/numSampleRo;
							misStatus=MIS_MEDIA_RO;
						}
						break;
					}
					// Se fuori portata, cambio portata.
					if (misDig >= MIS_DIG_MAX) {
						if (actDynPortata) {
							// comando fisico
							vUpdateDacPortValue((enum_select_dac_portata)--actDynPortata);
							pTaratura--;
							misSleep = SLEEP_CHANGE;
							misStatus = MIS_WAIT_CHANGE;
						}
						// Se non ho portate a disposizione, allarme.
						else {
							// Indico che ho terminato di leggere dall' adc.
							criticMis = 0;
							// allarme: anche la portata massima conduce al fondo scala di misura
							alarms |= ALR_PORTAT;
							// E' effettivamente un allarme valido.
							alr_status=alr_status_alr;
							avvStatus = AVV_IDLE;
							misStatus = MIS_OFF_LINE;
						}
						break;
					}

					if (ucActBadSpezzConsec==0){
						numSampleRo++;
						AccumRo+=misDig;
					}
					if (numSampleRo==16){
						// Indico che ho terminato di leggere dall' adc.
						criticMis = 0;
						misStatus=MIS_MEDIA_RO;
						ro=AccumRo>>4;
					}
					// Se ho spazio per ulteriori campioni, riparto con la conversione tra qualche millisecondo.
					else{
						misStatus = MIS_COMM_MUX;
						misSleep=SLEEP_STABIL_NEXT;
					}

					break;

				case MIS_MEDIA_RO:

					// Applicazione taratura alla misura effettuata. Segnalibro: TARATURA_CANALE_DINAMICO
					if (pTaratura->ksh){
						// Tolgo l' offset dello shunt.
						ro -= pTaratura->osh;
						// Moltiplico per il fattore correttivo.
						ro *= pTaratura->ksh;
						// Elimino il fattore di scala usato per avere maggior precisione.
						// ro /= 1024;
						ro >>=10;
					}
					// Per tener conto dello spessore dei contatti (0.857 mm)
					// ATTENZIONE: IL FONDO SCALA NON E' PIU' 5V, MA 8 VOLT
					// PERTANTO IL NUMERO DI OHM/V NON � PIU' 1 10 100 1000
					// MA 0.625 6.25 62.5 625
					// PERTANTO NE TENGO CONTO DIMINUENDO IL FATTORE MOLTIPLICATIVO
					// 100857*0.625=63036 CIRCA
					//ro = ro * 100857L; // 3
					// ro = ro * 63036L; // 3
					// elimino la compensazione dello spessore dei contatti!!!
					ro = ro * 62500L;


					ro >>= 6; // / 64
					// misura relativa a 2000 passi * 50 = misura su 1e5 passi
					ro *= 500;
					if (actDynPortata == 4) { // 1
						ro >>= 7; // / 128
						ro /= urpToRes;
					}
					else if (actDynPortata == 3) { // 10
						ro >>= 3; // / 8
						ro *= 10;
						ro >>= 4; // / 16
						ro /= urpToRes;
					}
					else if (actDynPortata == 2) { // 100
						ro /= urpToRes;
						ro *= 100;
						ro >>= 7; // / 128
					}
					else if (actDynPortata == 1) { // 1000
						ro /= urpToRes;
						ro >>= 3; // / 8
						ro *= 1000;
						ro >>= 4; // / 16
					}
					else {  // 10000
						ro /= urpToRes;
						if (ro & 0xfffc0000L) {
							ro *= 100;
							ro >>= 5; // / 32
							ro *= 100;
							ro >>= 2; // / 4
						}
						else {
							ro *= 10000;
							ro >>= 7; // / 128
						}
					}

					// Ulteriore filtraggio del valore di ro.
					// filtro al 60% precedente, 40% attuale
					// 60% di 128= 77
					// perch� vado meglio a dividere per 128 che per 100
					if (uc_reinit_ro_with_temperature_compensation){
						uc_reinit_ro_with_temperature_compensation=0;
						// I get the fiorst measured value as the first good value... then filtering proceed as usual...
						if (macroIsCodiceProdotto_TemperatureCompensated(&hprg.theRunning)){
							roPrec=ro;
						}
					}
					ro=(77*roPrec+(128-77)*ro)>>7;
					roPrec=ro;
					lastRoNonCompensato=ro;
					
					if (macroIsCodiceProdotto_TemperatureCompensated(&hprg.theRunning)){
						i_compensated_ro=i_ro_compensate_temperature(ro);
						ro=i_compensated_ro;
					}						

					// salvo nel log l'evento, andando all'indietro con l'indice
					if (--production_log.ucIdx>=sizeof(production_log.datas)/sizeof(production_log.datas[0]))
						production_log.ucIdx=sizeof(production_log.datas)/sizeof(production_log.datas[0])-1;
					plog=&production_log.datas[production_log.ucIdx];
					
					// applico la compensazione di temperatura...
					if (macroIsCodiceProdotto_TemperatureCompensated(&hprg.theRunning)){
						v_get_temperature_compensation_stats(&(plog->u.newSpezzone.temperature_compensation_stats));
					}						
					
					plog->event_type=let_newspezzone;
					plog->u.newSpezzone.ulRo=ro;
					plog->u.newSpezzone.ulNomRo=nomRo;
					plog->misDig=misDig;
					
					if (ro < minRo || ro > maxRo) {
						plog->event_type=let_spezzone_alarm;
						sLCD.ulInfoAlarms[0]=ro;
						sLCD.ulInfoAlarms[1]=minRo;
						sLCD.ulInfoAlarms[2]=maxRo;
						sLCD.ulInfoAlarms[3]=nomRo;
						// Allarme: misura fuori tolleranza
						alarms |= ALR_TOLMIS;
						// E' effettivamente un allarme valido.
						alr_status=alr_status_alr;
						avvStatus = AVV_IDLE;
						misStatus = MIS_OFF_LINE;
						break;
					}
					else {
						//ro=(ro*actCorrOhmica)/1024;
						ro=(ro*actCorrOhmica)>>10;
						// Scrivo nel buffer degli spezzoni la resistivit� dello spezzone attuale.
						spezFilo.ro[wrQueSpez++] = lastRo = ro;
						wrQueSpez %= MAX_SPEZ_FILO;
					}

					AccumRo=0;
					numSampleRo=0;
					if (!(spezFilo.flags & FLG_SPEZ_INIT)) {
						unsigned char j;
						actRo = ro;
                        j=0;
						while(j < nSpezDelay-1) {
							spezFilo.ro[wrQueSpez++] = ro;
							wrQueSpez %= MAX_SPEZ_FILO;
                            j++;
						}
						actSpezAvv  = chunkAvv;
						spezFilo.flags |= FLG_SPEZ_INIT;
					}

					misStatus = MIS_WAIT_INVALID;
					break;

				// Wait cambio di portata.
				case MIS_WAIT_CHANGE:
					if (!misSleep) {
						misStatus = MIS_COMM_MUX;
					}
					break;

				// Wait misura non valida.
				case MIS_WAIT_INVALID:
					if (defMisuraDinamicaNonValida) {
						// Mi rimetto con la portata pi� alta, che assicura la corrente minima sulle torrette.
						vUpdateDacPortValue((enum_select_dac_portata)0);
						misStatus = MIS_OFF_LINE;
					}
					break;
			}
		}
	}
	else {
		if (  !(cntAvv % INTERVAL_TEST_LAV) 
			|| criticLav
			|| TaglioAsincrono
// se devo usare distanziatore, meglio tenere frequentemente sott'occhio
// le quote, per non sbagliare il punto di discesa...
			||((hDist.uiEnabled_interrupt)&&def_do_distanz_ctrl)
			|| hDist.ucRefresh
			 ) {
			if ((avvStatus == AVV_IDLE)&&!TaglioAsincrono){
				++cntAvv;
				goto endOfTimerProcedure;
			}

			actLenImp += gapImp = newEnc - oldEncLav;
			oldEncLav = newEnc;
				// gestisco il distanziatore...
				vHandleDistanziatore_Interrupt();

			switch (avvStatus) {
				case AVV_WAIT_PRE:
					// Entro nella fase di pretaglio se ho raggiunto il numero
					// di impulsi dell' anticipo di pretaglio o se � attivo
					// il taglio asincrono.
					if ( (  (actLenImp >= preLenImp)
						   &&PrgRunning
						   &&(spiralatrice.PrimoPezzoEliminato||hprg.theRunning.empty)
						  )
						  ||TaglioAsincrono
						) {
						if (TaglioAsincrono){
							v_reset_cutter_position();
						}
						
						// Attivazione fisica del comando.
						OutDig = outDigMis |= ODG_PRETAGLIO;
						if (!macParms.ucUsaSensoreTaglio){
						}
						else{
							vAttivaOperazioneLama(enumCiclicaLama_pretaglia);
						}

						// Armo un timer, che uso solo nel caso di taglio manuale.
						if (ucEliminaPrimoPezzo){
							avvSleep=SLEEP_TAGLIO_PRIMO_PEZZO;
						}
						else
							avvSleep=SLEEP_TAGLIO_ASINC;

						avvStatus = AVV_WAIT_TAG;
						criticLav = 1;
					}
					break;

				case AVV_WAIT_TAG:
					// Dato che sono in posizione di pretaglio, setto
					// per sicurezza l' uscita dell' 8255.
					OutDig=outDigMis |= ODG_PRETAGLIO;
					if (!macParms.ucUsaSensoreTaglio){
					}
					// Entro nella fase di taglio se ho raggiunto il numero
					// di impulsi dell' anticipo di taglio o se � attivo
					// il taglio asincrono.
					if (   ((actLenImp >= targetLenImp)
						   &&PrgRunning
						   &&(spiralatrice.PrimoPezzoEliminato||hprg.theRunning.empty)
						   )
						   // Se taglio manuale, entro solo dopo che � passato il tempo di pretaglio.
						 ||(TaglioAsincrono&&!avvSleep)
						) {
						// Se arrivo qui, devo impartire il comando di taglio
						// e disabilitare quello di pretaglio.
						outDigMis |= ODG_TAGLIO;	// Attivo il comando di taglio.
						outDigMis &= ~ODG_PRETAGLIO;// Disattivo il comando di pretaglio.
						// Attivazione fisica del comando
						OutDig = outDigMis;
						if (macParms.ucUsaSensoreTaglio){
							vAttivaOperazioneLama(enumCiclicaLama_taglia);
						}
						else{

						}
						avvStatus = AVV_WAIT_TAGLIO;
						avvSleep=(unsigned short)SLEEP_MAX_TAGLIO;
					}
					break;

				case AVV_WAIT_TAGLIO:
					// Dato che sono in posizione di taglio, setto per sicurezza l' uscita dell' 8255.
					OutDig=outDigMis |= ODG_TAGLIO;
					if (macParms.ucUsaSensoreTaglio&&ucIsAttivaOperazioneLama()){
						break;
					}
					// Modifica del 14/03/1996: si disattiva il comando di taglio quando:
					// - � passato il tempo di taglio;
					// - oppure l' ingresso digitale IDG_TAGLIO_HI va ad 1
					//    (ad indicare che il coltello ha raggiunto la posizione alta.)
					// Segnalibro: IDG_TAGLIO_HI_USATO
					// Metto anche un timeout per disabilitare il comando di taglio.
					if (    macParms.ucUsaSensoreTaglio
						||(!macParms.ucUsaSensoreTaglio && ((InpDig&IDG_TAGLIO_HI)||!avvSleep) )
						){
						if (hDist.ucDistanziaInCorso){
							v_reset_stato_distanziatore(enum_reset_dist_reset_numero_distanziazioni);
						}
						criticLav = 0;
						outDigMis &= ~ODG_TAGLIO; // Disattivo il comando di taglio.
						// Attivazione fisica del comando
						OutDig = outDigMis;
						if (ucEliminaPrimoPezzo||TaglioAsincrono){
							ucEliminaPrimoPezzo=0;
							spiralatrice.PrimoPezzoEliminato=1;
						}
						// reinizializzo lo stato di pilotaggio bandella
						vCommandSwitchBandella_Interrupt();
						

						// Se ho attuato un taglio asincrono, non eseguo il conteggio del pezzo.
						if (TaglioAsincrono){
							TPR.ulTempoPerResistenzaCorrente=0;
							// Porto a zero tutti i parametri relativi al pezzo
							// correntemente in produzione.
							// Lo faccio comunque, anche se un programma non � in esecuzione.
							if (macroIsCodiceProdotto_Distanziatore(&hprg.theRunning)){
									vHandleTaglioAsincrono_Distanziatore();
								}
								else{
									actLenImp = 0;
								}
							oldEncLav = oldEncMis = newEnc = iReadCounter();
							actSpezInPez = 0;
							resLastSpezUp= 0;
							actRes      = 0;
							impFromMis = 0;
							TaglioAsincrono=0;
							// Rimetto lo stato dell' avvolgitore in attesa di pretaglio.
							if (PrgRunning)
								avvStatus = AVV_WAIT_PRE;
							else
								avvStatus = AVV_IDLE;
							break;
						}

						// salvo il tempo impiegato a produrre la res corrente
						TPR.ulTempoPerResistenzaUltima=TPR.ulTempoPerResistenzaCorrente;
						TPR.ulTempoPerResistenzaCorrente=0;
						TPR.uiNumPezziFatti++;

						lastPzLenImp = actLenImp;
						txNewPz = 1;

						// Conto un pezzo in piu' solo se il bit contapezzi vale 1.
						actNumPezzi+=BitContaPezzi;
						/* sicuramente il prossimo va contato */
						BitContaPezzi=1;
						//if (BitContaPezzi)
#if 0
						// salvo nel log l'evento, andando all'indietro con l'indice
						if (--production_log.ucIdx>=sizeof(production_log.datas)/sizeof(production_log.datas[0]))
							production_log.ucIdx=sizeof(production_log.datas)/sizeof(production_log.datas[0])-1;
						plog=&production_log.datas[production_log.ucIdx];
						plog->event_type=let_cut;
						plog->u.cut.ulCutImp=TARGET_URP;
						plog->u.cut.ulExpectedImp=TARGET_URP;
						plog->misDig=0;
#endif						


						// Se il lotto � finito, lo segnalo al programma principale che decider� se proseguire
						// col nuovo lotto o no.
						if (actNumPezzi >= nvram_struct.commesse[spiralatrice.runningCommessa].n+nvram_struct.commesse[spiralatrice.runningCommessa].magg) {
							// La routine si mette in IDLE state, in attesa della decisione del main.
							avvStatus = AVV_IDLE;
							// Segnalo che il programma corrente � terminato correttamente.
							RunningPrgEndOk=1;
						}
						else {
							actLenImp = 0;
							avvStatus = AVV_WAIT_PRE;
						}
					}
					break;
			}// switch
		}// if
	}// else

#ifdef def_canopen_enable
    v_process_canopen_tpdo();
#endif

endOfTimerProcedure:
	uc_in_timer0_interrupt=0;

really_the_end_timer0:
	uc_requesting_i2c_bus=0;

// solo per debug; imposto q15 all'inizio di ogni interrupt, lo restto alla fine
	/*
#warning 
#warning *************
#warning debug output hi @ 0x80 per tempistica routine di interrupt
#warning *************
#warning 
*/
    Outputs_Hi&=~0x80;

    T0IR = 0x1;         // clear interrupt flag
    //VICVectAddr = 0x00; // reset the vic

}// timer




// inizializzazione timer 0
void initTimer(void){
	unsigned long ulFpclk_timer0_Hz;
    #define def_T0PR 99

    cntAvv = cntMis = 0;
    criticLav = criticMis = 0;
    numSampleRo=0;
    AccumRo=0;
	vSpeedTargetUrp_init();

	// Prelevo il valore del peripheral clock di timer_0 [Hz]
	ulFpclk_timer0_Hz=ulGetPclk(enumLPC_Peripheral_PCLK_TIMER0);

    T0TCR = 2; // reset counters
    T0TCR = 0; // disable timer

    T0PR = def_T0PR;
	// set timer 0 match register 0 so that it can generate an interrupt every PeriodoIntMs ms
//	T0MR0 =PeriodoIntMs*(ulFpclk_timer0_Hz/(1000*(T0PR+1)));
    T0MR0 = (PeriodoIntMs * (ulFpclk_timer0_Hz /((def_T0PR+1)*1000)))-1;
    T0MCR = 0x3;				// when timer0 counter eaches timer0 match register 0, generates a cpu interrupt and reset timer0 counter
  
    VICIntSelect &= ~(0x10);	// Timer 0 selected as IRQ
    VICVectPriority4 = 1;		// set priority for timer 0 to be quite high
    VICVectAddr4 = (unsigned long)IRQ_Handler_timer0;

    T0TCR = 1;				// enable timer0
    VICIntEnable |= 0x10;	// Timer 0 interrupt enabled

}//void initTimer(void)


