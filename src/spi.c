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

#include "spiext.h"
#include "spiprot.h"
#include "spimsg.h"
#include "spipass.h"
#include "spifmt.h"
#include "max519.h"
#include "lcd.h"
#include "minmax.h"
#include "gui.h"
#include "spiwin.h"
#include "ff.h"

#include "file_list.h"
#include "az4186_const.h"
#include "az4186_program_codes.h"
#include "az4186_commesse.h"
#include "az4186_spiralatrice.h"
#include "az4186_files.h"
#include "az4186_dac.h"
#include "rtcDS3231.h"
#ifdef def_canopen_enable
    #include "az4186_can.h"
#endif
#include "az4186_temperature_compensation.h"

#ifdef def_use_emac
    #include "az4186_emac.h"
#endif

xdat TipoStructHandleDistanziatore hDist;

void vInitAD7327(void);
void vReadAD7327(void);
xdata float fMeanValueInputAnalogOnTaratura;
xdata unsigned char uiCountToVerifyHasBeenGetNewSample;
extern unsigned char data uiNumLettureMisuraDinamica_Interrupt;

#ifndef defDisable_Anybus_Module
	// modulo anybus: inizializzazione e gestione...
	extern void vHMA_init(void);
	extern void vHMA_handle(void);
#endif
// carica i coefficienti di calibrazione di default
void vLoadCoeffsLcdCalibration(void);

rom char ** msgNow;

// the fat of the filesystem...
FATFS fatfs;


/* Durata massima dell' allarme dal motore del mandrino all' accensione;
   quando la macchina viene accesa, si ignora l' allarme che proviene
   dal motore del mandrino. Successivi allarmi vengono invece tenuti
   in debito conto. */
#define durata_all_drmand  10000L

// usare questa define per ignorare tutti gli alarms...
#define defIgnoreAllAlarms 0



typedef struct{
      unsigned char rtcDay;
      unsigned char rtcMon;
      unsigned char rtcYea;
      unsigned char rtcHou;
      unsigned char rtcMin;
      unsigned int pezzi,scarti;
   }TipoSaveDataOra;
xdat TipoSaveDataOra SaveDate;




/* Indica se la macchina e' appena stata accesa. */
xdat unsigned char AppenaAcceso;
xdat unsigned char actNumInAna;
xdat unsigned char ucActResCampioneSel;
xdat unsigned long ulMaxTrueRo;
xdat unsigned long ulMinTrueRo;
xdat unsigned char ucMaxBadSpezzConsec;
xdat unsigned char ucActBadSpezzConsec;
xdat unsigned char ucSaveMacParms;
xdata float portataDinamica[5];
xdata unsigned int ui_act_dac_port_value;


/* Valore della resistenza usata per la taratura
   in una sequenza di nvram_struct.commesse in automatico.
   Il fattore di correzione e' dato dal rapporto
   fra la resistenza prodotta sull' ohmetro dinamico
   e la resistenza ideale. Se la resistenza prodotta sull' ohmetro
   dinamico e' legata a quella ideale da un fattore di allungamento
   e da un offset dovuto alle spire prese nei morsetti,
   esso si puo' scrivere come:

   fc1=k*(r1+dr)/r1
   con:
     fc1=fattore correzione 1
     k=fattore di allungamento
     r1=resistenza da produrre
     dr=resistenza nei morsetti dell' ohmetro statico

   analogamente:
   fc2=k*(r2+dr)/r2

   da cui
   fc2=fc1*(r2+dr)/(r1+dr)*r1/r2

   Pertanto devo memorizzare r1 e dr, vale a dire la resistenza ideale
   usata in taratura e la resistenza delle spire nei morsetti

*/
xdat float fResTaraturaFine;
/* Valore del delta R di resistenza nella misurazione
   fatta prendendo la spirale alle estremita'.
*/
xdat float fResDeltaR;

#ifdef WriteSpezzoni
	xdat TipoInfotest *pInfoTest;
#endif

xdata TipoStruct_Spiralatrice spiralatrice;


typedef struct _TipoSmartMenu{
			  unsigned char line1Str;
			  const char *formatStr;
			  unsigned char line2str;
			  unsigned char line;
			  float fMin;
			  float fMax;
			  unsigned char typeOfEdit;
			  unsigned char lenOfEdit;
			  unsigned char ucEnablePassword;
			  unsigned char ucIsInt;
	}TipoSmartMenu;

#define defSmartMenuEcQuan 		0
#define defSmartMenuEcRes 		1
#define defSmartMenuErQuanProve 	2
#define defSmartMenuErCoeffCorr 	3
#define defSmartMenuEpFsAssorb 		4
#define defSmartMenuEpDurataAll 	5
#define defSmartMenuEpRpmMand   	6
#define defSmartMenuEpLampAccesa	7
#define defSmartMenuEpLampSpenta	8
#define defSmartMenuEpRpmFriz		9
#define defSmartMenuEpImpEnc           10
#define defSmartMenuErAssRuota         11
#define defSmartMenuEbCodFor           12
#define defSmartMenuEpPercRo				13
#define defSmartMenuEpMaxBurst			14

rom TipoSmartMenu smartMenu[]={
  {
  	STR_QUANTITA,
	FMT_QUANTITA,
	STR_PEZZI,
	2,
	MIN_QUANTITA,
	MAX_QUANTITA,
	EDIT_NUMBER,
	LEN_EDIT_QUANTITA,
        PSW_EC_QUAN,
        1

  },

  {
	STR_RES_SPIRALE,
	FMT_RES,
	STR_OHM,
	2,
	MIN_RES_CAN_MAKE,
	MAX_RES_CAN_MAKE,
	EDIT_NUMBER,
	LEN_EDIT_RES,
        PSW_EC_RES,
        0
  },
  {
	STR_PEZZI_PROVE,
	FMT_QUANTITA_PROVE,
	STR_PEZZI,
        2,
	MIN_QUANTITA_PROVE,
	MAX_QUANTITA_PROVE,
	EDIT_NUMBER,
	LEN_EDIT_QUANTITA_PROVE,
        PSW_ER_QUAN_PROVE,
        1
  },
  {
	STR_COEFF_CORR,
	FMT_COEFF_CORR,
	0xFF,
        20,
	0.8,
	2,
	EDIT_NUMBER,
	LEN_EDIT_COEFFCORR,
        PSW_ER_COEFFCORR,
        0
  },
  {
	STR_EP_FS_ASSORB,
	FMT_FS_ASSORB,
	STR_AMPERE,
        2,
	0,
	1e5,
	EDIT_NUMBER,
	LEN_EDIT_EP_FS_ASSORB,
        1,
        0
  },
  {
  	STR_DURATA_ALL,
	FMT_DURATA_ALL,
	STR_MSEC,
	2,
	MIN_DURATA_ALL,
	MAX_DURATA_ALL,
	EDIT_NUMBER,
	LEN_EDIT_DURATA_ALL,
        PSW_EP_DURATAALL,
        1

  },
  {
  	STR_RPM_MANDRINO,
	FMT_RPM_MANDRINO,
	STR_RPM,
	2,
	MIN_RPM_MANDRINO,
	MAX_RPM_MANDRINO,
	EDIT_NUMBER,
	LEN_EDIT_RPM_MANDRINO,
        PSW_EP_RPM_MAND,
        1

  },
  {
  	STR_LAMP_ACC,
	FMT_NPEZZI_AVVISO,
	STR_PEZZI_PRIMA_FINE,
	2,
	MIN_NPEZZI_AVVISO,
	MAX_NPEZZI_AVVISO,
	EDIT_NUMBER,
	LEN_EDIT_NPEZZI_AVVISO,
        PSW_EP_PEZZI_AVVISO,
        1

  },
  {
  	STR_LAMP_SPEGN,
	FMT_LAMP_SPEGN,
	STR_SEC_DOPO_FINE,
	2,
	MIN_LAMP_SPEGN,
	MAX_LAMP_SPEGN,
	EDIT_NUMBER,
	LEN_EDIT_LAMP_SPEGN,
        PSW_EP_LAMP_SPEGN,
        1

  },
  {
  	STR_RPM_FRIZIONE,
	FMT_RPM_FRIZIONI,
	STR_RPM,
	2,
	MIN_RPM_FRIZIONE,
	MAX_RPM_FRIZIONE,
	EDIT_NUMBER,
	LEN_EDIT_RPM_FRIZ,
        PSW_EP_RPM_FRIZ,
        1

  },
  {
  	STR_IMP_GIRO_ENCODER,
	FMT_IMP_GIRO_ENCODER,
	STR_IMP_GIRO,
	2,
	MIN_IMP_GIRO_ENC,
	MAX_IMP_GIRO_ENC,
	EDIT_NUMBER,
	LEN_EDIT_IMP_GIRO_ENC,
        PSW_EP_IMP_ENC,
        1

  },
  {
  	STR_ASSORB_RUOTA,
	FMT_ASSORB_RUOTA,
	0xFF,
	20,
	MIN_ASSORB_RUOTA,
	MAX_ASSORB_RUOTA,
	EDIT_NUMBER,
	LEN_EDIT_ASSORB_RUOTA,
        PSW_ER_ASSORB_RUOTA,
        0

  },
  {
  	STR_CODICE_FOR,
	FMT_COD_FOR,
	0xFF,
	20,
	MIN_FORNI,
	MAX_FORNI,
	EDIT_NUMBER,
	LEN_EDIT_COD_FOR,
        0,
        1
  },
  {
  	STR_ER_PERCRO,
	FMT_PERCRO,
	STR_PERC,
	2,
	0,
	100,
	EDIT_NUMBER,
	LEN_EDIT_ER_PERCRO,
        1,
        1
  },
  {
  	STR_ER_MAXBURST,
	FMT_BURSTERR,
	0xff,
	20,
	0,
	100,
	EDIT_NUMBER,
	LEN_EDIT_MAXBURST,
        1,
        1
  },


};




void TxCommessa(unsigned int code);
void DatiInizioCommessa(void);
unsigned char SearchNextCommessa(unsigned char from);




// aggiornamento dei valori di taratura...
// da chiamare quando procedura di taratura finisce
void vUpdateValoriTaratura(void){
	pTar=&(macParms.tarParms[spiralatrice.actCanPortata][spiralatrice.actTaraPortata]);
	/* Se sto tarando l' offset... */
	if (spiralatrice.TaraPortate==TaraOffset){
		if (spiralatrice.actCanPortata==N_CANALE_OHMETRO_DYN){
			pTar->m0step=fMeanValueInputAnalogOnTaratura;
			pTar->m0 =FONDO_SCALA_OHM_MIN * ((float)portate[spiralatrice.actTaraPortata] / MAX_FS_ADC_DINAMICA) * fMeanValueInputAnalogOnTaratura;
			if ((pTar->mstep-pTar->m0step)>=1){
				pTar->k = (pTar->p-pTar->p0)/(pTar->m-pTar->m0) ;
				pTar->o   =pTar->m0    -pTar->p0/pTar->k;
				pTar->osh =pTar->m0step-pTar->p0*((pTar->mstep-pTar->m0step)/(pTar->p-pTar->p0));
				pTar->ksh =pTar->k * 1024.0;
			}
		}
		else{
			pTar->m0step=fMeanValueInputAnalogOnTaratura;
			pTar->m0 =((float)fMeanValueInputAnalogOnTaratura / MAX_FS_ADC_STATICA)* 2. * (float)portate[spiralatrice.actTaraPortata];
			if ((pTar->mstep-pTar->m0step)>=1){
				pTar->k = (pTar->p-pTar->p0)/(pTar->m-pTar->m0) ;
				pTar->o   =pTar->m0    -pTar->p0/pTar->k;
				//pTar->osh =pTar->m0step-pTar->p0*(pTar->mstep-pTar->m0step)/(pTar->p-pTar->p0);
			}
		}
		ucSaveMacParms=1;
	}
	/* Se sto tarando il guadagno... */
	else if (spiralatrice.TaraPortate==TaraGuadagno){
		if (spiralatrice.actCanPortata==N_CANALE_OHMETRO_DYN){
			pTar->mstep=fMeanValueInputAnalogOnTaratura;
			pTar->m =FONDO_SCALA_OHM_MIN * ((float)portate[spiralatrice.actTaraPortata] / MAX_FS_ADC_DINAMICA) * fMeanValueInputAnalogOnTaratura;
			if ((pTar->mstep-pTar->m0step)>=1){
				pTar->k = (pTar->p-pTar->p0)/(pTar->m-pTar->m0) ;
				pTar->o   =pTar->m0    -pTar->p0/pTar->k;
				pTar->osh =pTar->m0step-pTar->p0*((pTar->mstep-pTar->m0step)/(pTar->p-pTar->p0));
				pTar->ksh = pTar->k * 1024.0;
			}
		}
		else{
			pTar->mstep=fMeanValueInputAnalogOnTaratura;
			pTar->m =((float)fMeanValueInputAnalogOnTaratura / MAX_FS_ADC_STATICA)* 2. * (float)portate[spiralatrice.actTaraPortata];
			if ((pTar->mstep-pTar->m0step)>=1){
				pTar->k = (pTar->p-pTar->p0)/(pTar->m-pTar->m0) ;
				pTar->o   =pTar->m0    -pTar->p0/pTar->k;
				//pTar->osh =pTar->m0step-pTar->p0*(pTar->mstep-pTar->m0step)/(pTar->p-pTar->p0);
			}
		}
		ucSaveMacParms=1;
	}
	if (spiralatrice.TaraPortate!=TaraNulla){
		spiralatrice.TaraPortate=TaraNulla;
		if (spiralatrice.actCanPortata==N_CANALE_OHMETRO_DYN){
			actPortata=NUM_PORTATE_DINAMICHE-1-spiralatrice.actTaraPortata;
		}
		else{
			actPortata=NUM_PORTATE-1-spiralatrice.actTaraPortata;
		}
	}
}//void vUpdateValoriTaratura(void)








#if 0
	unsigned char ucSmartMenu(unsigned char op,unsigned char ucNumSmartMenu,float fValue){

		xdat unsigned char ucRet;
		rom TipoSmartMenu *p;
		ucRet=0;
		p=&smartMenu[ucNumSmartMenu];
		switch (op) {
			case MNU_OP_PRINT:
				edit.tipoPrint=PrintWithoutPicture;
				startPrintLcd(FLG_PRINT_CLEAR, p->line1Str,  0, 0, 0, 0);
				PrintGeneric:
				if (p->ucIsInt)
					At2(p->formatStr, (int)fValue);
				else
					At2(p->formatStr, fValue);
				if (p->line<3)
					AtRightLine(p->line2str,p->line);
				break;


			case MNU_OP_ENTER:
				if (isEdit) {
					isEdit = 0;
					*edit.actEditChar = 0;
					edit.tmpfSmartMenu = myatof(edit.firstEditChar);
					if ((edit.tmpfSmartMenu >= p->fMin) && (edit.tmpfSmartMenu <= p->fMax)) {
						ucRet=1;
					}
					else
						edit.tmpfSmartMenu=fValue;
					if (p->ucIsInt)
						At2(p->formatStr, (int)edit.tmpfSmartMenu);
					else
						At2(p->formatStr, edit.tmpfSmartMenu);
					AtRightLine(p->line2str,p->line);
				}
				else {
					isEdit = 1;
					if (p->ucEnablePassword)
						checkPassw = 1;
					edit.typEdit = p->typeOfEdit;
					edit.lenEdit = p->lenOfEdit;
				}
				break;

			case MNU_OP_ESC:
				if (isEdit) {
					isEdit = 0;
					goto PrintGeneric;
				}
				break;

		}
		return ucRet;
	}//unsigned char ucSmartMenu(unsigned char op,unsigned char ucNumSmartMenu,float fValue)
#endif





/* Funzione che calcola il coefficiente di correzione
   Il fattore di correzione e' dato dal rapporto
   fra la resistenza prodotta sull' ohmetro dinamico
   e la resistenza ideale. Se la resistenza prodotta sull' ohmetro
   dinamico e' legata a quella ideale da un fattore di allungamento
   e da un offset dovuto alle spire prese nei morsetti,
   esso si puo' scrivere come:

   fc1=k*(r1+dr)/r1
   con:
     fc1=fattore correzione 1
     k=fattore di allungamento
     r1=resistenza da produrre
     dr=resistenza nei morsetti dell' ohmetro statico

   analogamente:
   fc2=k*(r2+dr)/r2

   da cui
   fc2=fc1*(r2+dr)/(r1+dr)*r1/r2

   Pertanto devo memorizzare r1 e dr, vale a dire la resistenza ideale
   usata in taratura e la resistenza delle spire nei morsetti.
   Se la commessa non e' automatica, uso il fattore di correzione
   del programma e buona notte.
*/
float fCalcCoeffCorr(void){
	if (!NextPrgAutomatic)
		return hprg.theRunning.coeff_corr;
	return hprg.theRunning.coeff_corr+fResDeltaR/fResTaraturaFine-fResDeltaR/nvram_struct.commesse[spiralatrice.runningCommessa].res;
}

/* Funzione che calcola l' anticipo dell' azione di pretaglio */
void vCalcAnticipoPretaglio(void){
	xdata float fCoeffCorr;
	extern float f_delay_wait_spacer_in_precut_position_pulses;
	// calcolo il coefficiente di correzione
	fCoeffCorr=fCalcCoeffCorr();

	/* Calcolo del numero di impulsi di anticipo di pretaglio. */
	// correzione V1.29 --> tolgo il fattore 2 dal numero di spire di pretaglio!
	// perch� il numero di spire effettivo � circa 1/3 di quello desiderato????
	anticipoPretaglio=(float)macParms.nspire_pret*PI_GRECO*(spiralatrice.pPrgRun->diametro_mandrino+spiralatrice.pPrgRun->diametro_filo)/impToMm;
	/* Per tener conto del tempo di discesa del coltello
		e del ritardo di campionamento, aumento l' anticipo di pretaglio
		di un certo addendo (in numero di impulsi).
	*/
	anticipoPretaglio=anticipoPretaglio+50;


	// versione 1.36
	// perch� devo dividere per il coefficiente di correzione???
	// se sono in modo L serve solo per compensare effetti di allungamento
	// se sono in modo L+R, il numero di impulsi � gi� "compensato" in base alla resistenza effettiva della spirale...
	// quindi divido solo se sono in modo L
	if (!withMis){
		anticipoPretaglio/=fCoeffCorr;
	}

	// divido anche per il numero di fili
	if (spiralatrice.pPrgRun->num_fili>1)
		anticipoPretaglio/=spiralatrice.pPrgRun->num_fili;
		
	if (macParms.nspire_pret){
		// just wait 1 coil+0% before returning in spacing position
		f_delay_wait_spacer_in_precut_position_pulses=1.0*anticipoPretaglio/macParms.nspire_pret;
	}
	else{
		f_delay_wait_spacer_in_precut_position_pulses=100;
	}

	// quanti impulsi � lungo on moncone da 60mm?
	// divido 60 per diametro filo e trovo il numero di spire
	// moltiplico per pi greco e per il diametro (mandrino+filo)
	// infine per fattore conversione da mm a impulsi e ottengo
	// il numero di impulsi oltre il quale tagliare
	tagliaMoncone_Imp=(defLunghezzaMonconeEliminaPrimoPezzoMm/spiralatrice.pPrgRun->diametro_filo)*PI_GRECO*(spiralatrice.pPrgRun->diametro_mandrino+spiralatrice.pPrgRun->diametro_filo)/impToMm;

	// versione 1.36
	// perch� devo dividere per il coefficiente di correzione???
	// se sono in modo L serve solo per compensare effetti di allungamento
	// se sono in modo L+R, il numero di impulsi � gi� "compensato" in base alla resistenza effettiva della spirale...
	// quindi divido solo se sono in modo L
	if (!withMis){
		tagliaMoncone_Imp/=fCoeffCorr;
	}

	// divido anche per il numero di fili
	if (spiralatrice.pPrgRun->num_fili>1)
		tagliaMoncone_Imp/=spiralatrice.pPrgRun->num_fili;
}

void menuOperation(unsigned char op){
	//if ((op==MNU_OP_PRINT)||(op==MNU_OP_UPDATE))
		vForceLcdRefresh();
}

// Update 8-bit CRC value
//  using polynomial  X^8 + X^5 + X^4 + 1
#define POLYVAL 0x8C
void update_crc(unsigned char ucNewByte, unsigned char *pucCrc){
	xdata unsigned char c, i;
	c = *pucCrc;
	for (i = 0; i < 8; i++) {
		if ((c ^ ucNewByte) & 1) {
			c = (c >> 1 ) ^ POLYVAL;
		}
		else {
			c >>= 1;
		}
		ucNewByte >>= 1;
	}
	*pucCrc = c;
}

// calcola crc della struttura parametri macchina
// ritorna 1 se crc calcolato correttamente
//		in qst caso *pucCrc contiene il crc calcolato
// ritorna 0 se crc non puo' essere calcolato correttamente (es versione parametri non corretta)
//
unsigned char ucCalcCrcMacParams(unsigned char *pucCrc){
	xdata unsigned char ucCrc;
	xdata unsigned char * xdata pc;
	xdata unsigned int uiSizeMacParmsSaved;
	// gestione delle versioni dei parametri:
	// carico solo il numero di bytes specifico della versione dei parametri
	// se versione 0, uso la dimensione della struttura della versione 0
	// se versione 1, uso la dimensione della struttura della versione 1
	// se versione 2, uso la dimensione della struttura della versione 1, ma correggo l'errore nel calcolo crc!!!!
	// se versione 3, uso la dimensione della struttura della versione 3
	if (macParms.uiMacParmsFormatVersion>defMacParmsFormatVersion){
		return 0;
	}
	// se versione da 2 in poi, uso un calcolo crc pi� sofisticato per non permettere errori
	else if (macParms.uiMacParmsFormatVersion>=2){
		switch(macParms.uiMacParmsFormatVersion){
			case 2:
				uiSizeMacParmsSaved=sizeof(MACPARMS_V2);
				break;
			case 3:
				uiSizeMacParmsSaved=sizeof(MACPARMS_V3);
				break;
			case 4:
				uiSizeMacParmsSaved=sizeof(MACPARMS_V4);
				break;
			default:
			case 5:
				uiSizeMacParmsSaved=sizeof(macParms);
				break;
		}
		pc=(unsigned char xdata *)&(macParms.ucCrc);
		// devo saltare il crc, altrimenti ogni volta il risultato cambia!!!
		pc++;
		// inizializzo il valore del crc
		*pucCrc=0;
		// uso un crc polinomiale invece del precedente pessimo xor-based!!!!
		while(pc<((unsigned char xdata *)&macParms)+uiSizeMacParmsSaved){
			update_crc(*pc, pucCrc);
			pc++;
		}
	}
	else{
		ucCrc=0;
		switch(macParms.uiMacParmsFormatVersion){
			case 0:
				uiSizeMacParmsSaved=sizeof(MACPARMS_V0);
				break;
			case 1:
			case 2:
				uiSizeMacParmsSaved=sizeof(MACPARMS_V2);
				break;
			case 3:
				uiSizeMacParmsSaved=sizeof(MACPARMS_V3);
				break;
			case 4:
			default:
				uiSizeMacParmsSaved=sizeof(macParms);
				break;
		}
		pc=(unsigned char xdata *)&(macParms.ucCrc);
		// devo saltare il crc, altrimenti ogni volta il risultato cambia!!!
		pc++;
		while(pc<((unsigned char xdata *)&macParms)+uiSizeMacParmsSaved){
			ucCrc^=*pc++;
		}
		ucCrc=~ucCrc;
		*pucCrc=ucCrc;
	}
	return 1;
}
// salva i parametri macchina
void vSaveMacParms(void){
	// salvo versione del formato dei parametri macchina
	macParms.uiMacParmsFormatVersion=defMacParmsFormatVersion;
	// calcolo il crc
	ucCalcCrcMacParams(&macParms.ucCrc);
	// salvo la struttura in flash
    ui_write_macparams_file((unsigned char xdata *)&macParms, sizeof(macParms));
}
//#define def_test_macpars_save

#ifdef def_test_macpars_save

    #warning possibly macparams destructive test enabled!!!!    

    void v_test_macpars_file_save(void){
        MACPARMS macs_aux;
        int i,j;
        char *pc;
        static unsigned int error_sd_test;
        ui_read_macparams_file((unsigned char xdata *)&macs_aux, sizeof(macParms));
        if (memcmp(&macs_aux,&macParms,sizeof(macs_aux))){
            error_sd_test=1;
        }
        i=0;
        while(i<30){
            pc=(char*)&macs_aux;
            srand(333);
            j=0;
            while(j<sizeof(macs_aux)){
               *pc++=(rand()&0xff);
                j++;
            }
            ui_write_macparams_file((unsigned char xdata *)&macs_aux, sizeof(macParms));
            ui_read_macparams_file((unsigned char xdata *)&macs_aux, sizeof(macParms));
            pc=(char*)&macs_aux;
            srand(333);
            j=0;
            while(j<sizeof(macs_aux)){
                if (((*pc++)&0xff)!=(rand()&0xff)){
                    error_sd_test++;
                }
                j++;
            }
            i++;
        }
        ui_write_macparams_file((unsigned char xdata *)&macParms, sizeof(macParms));
        if (error_sd_test)
            while(1);
    }
#endif

// carica i parametri macchina
void vLoadMacParms(void){
	extern void vMacParmsAtDefaultValue(void);
	xdata unsigned char ucCrc;
	xdata unsigned int uiSizeMacParmsSaved;

// read mac params
    ui_read_macparams_file((unsigned char xdata *)&macParms, sizeof(macParms));
	// azzero tutti i parametri che "fuoriescono" dall'ambito della versione attuale dei parametri
	// se ad es i pars sono salvati nella versione 2, e quella attuale � la versione 3,
	// devo azzerare i pars aggiunti in modo da non trovarli abilitati...
	if (macParms.uiMacParmsFormatVersion!=defMacParmsFormatVersion){
		switch(macParms.uiMacParmsFormatVersion){
			case 0:
				uiSizeMacParmsSaved=sizeof(MACPARMS_V0);
				break;
			case 1:
			case 2:
				uiSizeMacParmsSaved=sizeof(MACPARMS_V2);
				break;
			case 3:
			default:
				uiSizeMacParmsSaved=sizeof(macParms);
				break;
		}
		// reset a 0 dei parametri aggiunti, se ce ne sono...
		if (uiSizeMacParmsSaved<sizeof(macParms)){
            char *p;
            unsigned int ui_size_block_to_reset;
            p=(char*)&macParms;
            p+=uiSizeMacParmsSaved;
            ui_size_block_to_reset=sizeof(macParms)-uiSizeMacParmsSaved;
            // proceed with reset!
			memset(p,0,ui_size_block_to_reset);
		}
	}

	if (  !ucCalcCrcMacParams(&ucCrc)
		 ||(macParms.ucCrc!=ucCrc)
		){
		vMacParmsAtDefaultValue();
		ui_write_macparams_file((unsigned char xdata *)&macParms, sizeof(macParms));
	}
#ifdef def_test_macpars_save
    v_test_macpars_file_save();
#endif
}

// verifica se i pars macchina devono essere salvati in nand flash...
void vCtrlSaveMacParms(void){
	if (ucSaveMacParms){
		ucSaveMacParms=0;
		vSaveMacParms();
	}
}//void vCtrlSaveMacParms(void)




// riempie la struttura dist coi parametri memorizzati nel programma...
// se il programma non � uno di quelli col distanziatore abilitato, ritorna 0
// se trasferimento ok, ritorna 1
// Ingresso:
// * puntatore al programma
// * valore ohmico da produrre
// * resistenza specifica del filo (per computare n spire...)
unsigned char ucFillStructDist(TipoStructDistConversion xdata *pDistConv){
	unsigned char uc;
	xdata unsigned int ui_numofcoils;
	TipoStructDistInfo xdata *pDistInfo;
	// se codice programma non � quello di una programma con distanziatore, restituisco 0
	if (!macroIsCodiceProdotto_Distanziatore(pDistConv->pPro)){
		return 0;
	}
	// sbianco la struttura dist
	memset(&dist,0,sizeof(dist));
	// imposto delay coltello basso
 	dist.usDelayDistanziaDownMs=pDistConv->pPro->usDelayDistanziaDownMs;

	// riempio la mia struttura
	pDistInfo=&(dist.info[0]);
	ui_numofcoils=0;
	for (uc=0;uc<sizeof(dist.info)/sizeof(dist.info[0]);uc++,pDistInfo++){
		if (pDistConv->pPro->usPercDistanzia_Ends[uc]==0){
			break;
		}
		switch(pDistConv->pPro->ucPercDistanzia_Type[uc]){
			case enum_perc_distanzia_type_perc_iniz_fin:
			default:
				pDistInfo->perc_distanzia_type=enum_perc_distanzia_type_perc_iniz_fin;
				// percentuale iniziale 0_100
				pDistInfo->f_perc_iniz=f_convert_perc_dist_to_float_0_100(pDistConv->pPro->usPercDistanzia_Starts[uc]);
				// valore ohmico inizio distanziazione
				pDistInfo->f_ohm_iniz =pDistConv->f_res_value_ohm*pDistInfo->f_perc_iniz*0.01;
				// percentuale finale 0_100
				pDistInfo->f_perc_fin =f_convert_perc_dist_to_float_0_100(pDistConv->pPro->usPercDistanzia_Ends[uc]);
				// valore ohmico fine distanziazione
				pDistInfo->f_ohm_fin  =pDistConv->f_res_value_ohm*pDistInfo->f_perc_fin*0.01;
				// numero di spire di distanziazione: dipende da un sacco di parametri...
				if (pDistInfo->f_ohm_fin<pDistInfo->f_ohm_iniz)
					pDistInfo->ui_num_of_coil_windings=ui_numofcoils;
				else{
					ui_numofcoils=(pDistInfo->f_ohm_fin-pDistInfo->f_ohm_iniz)/(PI_GRECO*(pDistConv->pPro->diametro_mandrino+pDistConv->pPro->diametro_filo)*pDistConv->f_res_specifica_ohm_m*0.001)+0.5;
					pDistInfo->ui_num_of_coil_windings=ui_numofcoils;
				}
				break;
			case enum_perc_distanzia_type_perc_iniz_nspire:
				pDistInfo->perc_distanzia_type=enum_perc_distanzia_type_perc_iniz_nspire;
				// percentuale iniziale 0_100
				pDistInfo->f_perc_iniz=f_convert_perc_dist_to_float_0_100(pDistConv->pPro->usPercDistanzia_Starts[uc]);
				// valore ohmico inizio distanziazione
				pDistInfo->f_ohm_iniz =pDistConv->f_res_value_ohm*pDistInfo->f_perc_iniz*0.01;
				// numero di spire
				pDistInfo->ui_num_of_coil_windings=pDistConv->pPro->usPercDistanzia_NumSpire[uc];
				ui_numofcoils=pDistInfo->ui_num_of_coil_windings;
				// percentuale finale 0_100 la devo ricavare...
				pDistInfo->f_ohm_fin=pDistInfo->f_ohm_iniz+pDistInfo->ui_num_of_coil_windings*(PI_GRECO*(pDistConv->pPro->diametro_mandrino+pDistConv->pPro->diametro_filo)*pDistConv->f_res_specifica_ohm_m*0.001);
				if (pDistInfo->f_ohm_fin>pDistConv->f_res_value_ohm)
					pDistInfo->f_ohm_fin=pDistConv->f_res_value_ohm;
				pDistInfo->f_perc_fin=pDistInfo->f_ohm_fin/(pDistConv->f_res_value_ohm*0.01);
				break;
			case enum_perc_distanzia_type_perc_fin_nspire:
				pDistInfo->perc_distanzia_type=enum_perc_distanzia_type_perc_fin_nspire;
				// percentuale finale 0_100
				pDistInfo->f_perc_fin =f_convert_perc_dist_to_float_0_100(pDistConv->pPro->usPercDistanzia_Ends[uc]);
				// valore ohmico fine distanziazione
				pDistInfo->f_ohm_fin  =pDistConv->f_res_value_ohm*pDistInfo->f_perc_fin*0.01;
				// numero di spire
				pDistInfo->ui_num_of_coil_windings=pDistConv->pPro->usPercDistanzia_NumSpire[uc];
				ui_numofcoils=pDistInfo->ui_num_of_coil_windings;
				// percentuale iniziale 0_100 la devo ricavare...
				pDistInfo->f_ohm_iniz=pDistInfo->f_ohm_fin-pDistInfo->ui_num_of_coil_windings*(PI_GRECO*(pDistConv->pPro->diametro_mandrino+pDistConv->pPro->diametro_filo)*pDistConv->f_res_specifica_ohm_m*0.001);
				if (pDistInfo->f_ohm_iniz<0)
					pDistInfo->f_ohm_iniz=0;
				pDistInfo->f_perc_iniz=pDistInfo->f_ohm_iniz/(pDistConv->f_res_value_ohm*0.01);
				break;
		}
	}
	dist.uc_num_valid_entries=uc;			// numero di entries valide
	// imposto a default il valore del numero di spire di tutte le prossime entries non ancora inizializzate
	for (;uc<sizeof(dist.info)/sizeof(dist.info[0]);uc++){
		dist.info[uc].ui_num_of_coil_windings=ui_numofcoils;
	}
return 1;

}//end of--> unsigned char ucFillStructDist(TipoStructDistConversion xdata *pDistConv)

// riempie la struttura programma coi parametri memorizzati nella struttura dist...
// Ingresso:
// * puntatore alla struttura distConv che contiene numerose info utili alle conversioni...
//
// Uscita:
// 1 se tutto ok
// 0 altrimenti--> es se le percentuali non sono crescenti, restituisco 0 e sbianco le percentuali da quella non crescente in poi
unsigned char ucFillProgramStructDist(TipoStructDistConversion xdata *pDistConv){
	xdata unsigned char uc;
	xdata unsigned char uc_ret_value;
	TipoStructDistInfo xdata *pDistInfo;
	uc_ret_value=1;
	// se codice programma non � quello di una programma con distanziatore, restituisco 0
	if (!macroIsCodiceProdotto_Distanziatore(pDistConv->pPro)){
		return 0;
	}
	// sbianco la struttura dist contenuta nel programma 
	memset(&pDistConv->pPro->usPercDistanzia_Starts[0]	,0,sizeof(pDistConv->pPro->usPercDistanzia_Starts)	);
	memset(&pDistConv->pPro->usPercDistanzia_Ends[0]	,0,sizeof(pDistConv->pPro->usPercDistanzia_Ends)	);
	memset(&pDistConv->pPro->usPercDistanzia_NumSpire[0],0,sizeof(pDistConv->pPro->usPercDistanzia_NumSpire));
	memset(&pDistConv->pPro->ucPercDistanzia_Type[0]	,0,sizeof(pDistConv->pPro->ucPercDistanzia_Type)	);
	// salvo delay coltello basso
	pDistConv->pPro->usDelayDistanziaDownMs=dist.usDelayDistanziaDownMs;

	// salvo le varie percentuali di distanziazione
	pDistInfo=&(dist.info[0]);
	for (uc=0;uc<sizeof(pDistConv->pPro->usPercDistanzia_Starts)/sizeof(pDistConv->pPro->usPercDistanzia_Starts[0]);uc++,pDistInfo++){
		pDistConv->pPro->usPercDistanzia_Starts[uc]=us_convert_float_0_100_to_perc_dist(pDistInfo->f_perc_iniz);
		pDistConv->pPro->usPercDistanzia_Ends[uc]  =us_convert_float_0_100_to_perc_dist(pDistInfo->f_perc_fin);
		pDistConv->pPro->usPercDistanzia_NumSpire[uc]=pDistInfo->ui_num_of_coil_windings;
		pDistConv->pPro->ucPercDistanzia_Type[uc]=pDistInfo->perc_distanzia_type;
		// percentuale di start e di end devono essere minori del massimo ammesso
		if (  (pDistConv->pPro->usPercDistanzia_Starts[uc]>=defMaxValuePercDistanzia)
		    &&(pDistConv->pPro->usPercDistanzia_Ends  [uc]> defMaxValuePercDistanzia)
		   ){
			uc_ret_value=0;
			break;
		}
		// percentuale di end deve essere maggiore di quella di start (oppure devono essere entrambe nulle)
		if (pDistConv->pPro->usPercDistanzia_Starts[uc]>pDistConv->pPro->usPercDistanzia_Ends[uc]){
			uc_ret_value=0;
			break;
		}
		// verifico se ci sono errori nell'impostazione delle percentuali
		if (uc){
			// verifico se le percentuali sono decrescenti o no, cio� se la percentuale iniziale di questa fase 
			// � o no superiore a quella finale della precedente
			if (pDistConv->pPro->usPercDistanzia_Starts[uc]<pDistConv->pPro->usPercDistanzia_Ends[uc-1]){
				uc_ret_value=0;
				break;
			}
		}
	}
	// errore nelle impostazioni??? allora sbianco la cella attuale (quelle successive sono certamente gi� state sbiancate)
	if (uc_ret_value==0){
		pDistConv->pPro->usPercDistanzia_Starts[uc]=0;
		pDistConv->pPro->usPercDistanzia_Ends[uc]=0;
		pDistConv->pPro->usPercDistanzia_NumSpire[uc]=0;
		pDistConv->pPro->ucPercDistanzia_Type[uc]=0;
	}
	return uc_ret_value;
}//end of--> unsigned char ucFillProgramStructDist(TipoStructDistConversion xdata *pDistConv)


// performs lot of initializations...
void init(void){
    extern void vResetWindows();
    void vResetRamAndReboot(void);
	// inserisco almeno qlch programma in lista...
	// per evitare situazioni degeneri...
	//xdata PROGRAMMA myTestPrg;
	unsigned char i;
	//xdata unsigned char ucRet;
	// funzione di apertura della nand flash...
	extern void vOpenNand(unsigned char ucFormatFlash);

// init rtc
	vInit_RtcDS3231();

    // if (examineBootReset[0].ucBootResetNecessary){
	// reset completo della ram...
		// examineBootReset[0].ucBootResetNecessary=0;
		// vResetRamAndReboot();
	// }

// mount file system
	f_mount(0, &fatfs);		// Register volume work area (never fails) 

// load mac parameters
	vLoadMacParms();

// initialize the registry system...
    v_init_file_list_registry();
//
// register the product codes file
//
    ui_register_product_codes();
    // unneeded intialization (bss is reset to zero)
	// ucSaveMacParms=0;

// load lcd calibration file...
	vLoadCoeffsLcdCalibration();
// initialize temperatire compensation
	v_initialize_temperature_compensation();

// setup dynamic ranges
	portataDinamica[0]=5000.0;
	portataDinamica[1]=500.0;
	portataDinamica[2]=50.0;
	portataDinamica[3]=5.0;
	portataDinamica[4]=0.5;
	// devo azzerare questa variabile!!!!
	spiralatrice.GradinoRampaDAC=0;


	#ifdef defTestPortataDinamica
		actTestPortVal=0;
	#endif

	memset(&production_log,0,sizeof(production_log));
	memset(&hDist,0,sizeof(hDist));
	actNumInAna=0;
	spiralatrice.spaziorv=0;
	spiralatrice.temporv=0;
	spiralatrice.NumPezziScarto=0;
	// Resetto l' indicatore di macchina in passaggio automatico da un programma al successivo.
	NextPrgAutomatic=0;

	if (nvram_struct.actLanguage>2)
		nvram_struct.actLanguage=0;
	//inizializzo il campo di edit...
	memset(&edit,0,sizeof(edit));
	//memset(&prgList,0,sizeof(prgList));
	// azzero le nvram_struct.commesse??? no!
	// memset(&nvram_struct.commesse,0,sizeof(nvram_struct.commesse));
	spiralatrice.actCommessa=0;
	actPortata=0;
	vInitProgramList();
	spiralatrice.LastNumCommessa=0;
	vSearchFirstComm();
	sLCD.ucAttivataMisuraDinamica=0;
	spiralatrice.pComm=NULL;
	spiralatrice.pCommRun=NULL;
	spiralatrice.pPrg=hprg.ptheAct;
	spiralatrice.pPrgRun=NULL;
	memset(&TPR,0,sizeof(TPR));
	ucLcdRefreshTimeoutReset();
	iInitReadCounter_2();

	if (spiralatrice.firstComm){
		/* Comincio dal menu setup. */
		actMenu        = MENU_SETUP;
		actSubmenu     = SBM_EB_CODICE_COM;
	}
	/* Se nessuna commessa presente, comincio dal menu nvram_struct.commesse. */
	else{
		/* Comincio dal menu nvram_struct.commesse. */
		actMenu        = MENU_SEL_MENU;
		actSubmenu     = MENU_EDIT_COMM;
	}
    i=0;
	while(i < NUM_MENUS){
		lastSubmenu[i] = 0;
        i++;
    }


	edit.actLcdChar = 0;
	act7segChar     = 0;
	myInpDig        = 0;
	outDigVariable  = 0;
	seg7Point       = 0xf8;


	misStatus = MIS_OFF_LINE;
	backupAvvStatus = avvStatus = AVV_IDLE;
	avvSleep = 0;
	encVal = iReadCounter();
	newEnc =oldEncMis = oldEncLav=encVal;
	gapImp = 0;
	actLenImp = 0;
	lastPzLenImp = 0;
	preLenImp = targetLenImp = 0;
	actSpezAvv = 0;
	actSpezInPez = 0;
	actRo = 0;
	resLastSpezUp = 0;
	actRes = 0;
	targetRes = 0.;
	actFilo = 0;
	actFrizione = MIN_NUM_FRIZIONI-1;
	misDig = 0;
	misStaVolt = 0.;
	actPortata = 0;
	vUpdateDacPortValue((enum_select_dac_portata)0);
	misSleep = 0;
	misStaSleep = 0;
	lastPzRes = 0;
	actPreTime = 0;
	actTagTime = 0;
	if (macParms.impGiro)
		impToMm = macParms.mmGiro / (float)macParms.impGiro;

	impFromMis = 0;
	resSta = 2e30;
	urpToRes = 0;
	minRo = maxRo = 0;
	nSpezDelay = 0;
	chunkAvv = 0;
	withMis = 0;
	rdQueSpez = 0;
	wrQueSpez = 0;
	spezFilo.flags = 0;
	anticipoPretaglio = MIN_ANTI_PRETAGLIO / 10;
	/* Pongo a zero l' indicatore di programma terminato correttamente. */
	RunningPrgEndOk=0;
	/* Allarme su Timeout da torretta attivo. */
	AlarmOnTimeout=1;
	/* Pongo a zero l' indicatore di entra in automatico col prossimo
	programma. */
	NextPrgAutomatic=0;
	/* Pongo a zero programma in esecuzione. */
	PrgRunning=0;
	/* Parto col programma legato alla commessa attuale. */
	vInitActPrg();
	/* Pongo a zero l' indicatore di esecuzione taglio asincrono. */
	TaglioAsincrono=0;
	spazio = tempo  = 0;
	spiralatrice.GradinoRampaDAC=NumGradiniRampaDAC;
    i=0;
	while(i<NumDacRampa*2){
		spiralatrice.DacOut[i]=0;
		spiralatrice.DacOutOld[i]=0;
        i++;
	}
	// Azzero le uscite dei dac motore e ruote di contrasto.
	// occhio che il dac del coltello va impostato a 128 perch� � bipolare
	SendByteMax519(addrDacMandrino,0,128);
	SendByteMax519(addrDacFrizione1,0,0);

	isEdit = 0;
	#ifdef okEditServe
		edit.okEdit = 0;
	#endif
	edit.actEditChar = 0;
	edit.firstEditChar = 0;
	updVel  = 1;
	checkedPassw = checkPassw = 0;
	spiralatrice.OkAbilitaTaraturaStat=0;
	spiralatrice.OkAbilitaTaraFineStat=0;
	spiralatrice.actFornitore=0;
	spiralatrice.StartPortata=0;

	edit.tipoPrint=PrintWithoutPicture;
	/* Stringa di picture. */
	edit.actStrPicture[0]=0x00;
	/* Stringa di reverse. */
	edit.actStrReverse[0]=0x00;
	/* Indicatore di taratura in corso. */
	spiralatrice.TaraPortate=TaraNulla;
	/* Numero del prossimo canale ADC da leggere per aggiornare i valori. */
	spiralatrice.numCanaleUpdate=0;
	misStaStatus =MIS_OFF_LINE;
	/* Azzero media e numero di campioni dei canali di lettura della corrente
	assorbita dalle ruote di contrasto. */
	spiralatrice.auxMediaCanaleAssorb=0;
	spiralatrice.nMisAssorb=0;
	spiralatrice.CorsaColtello=macParms.iasse[1]-macParms.iasse[0];

	txNewPz = 0;
	txModCont = 0;
	txBusy = 0;
	txReqLocalCommessa = 0;
	rxHasMsg = 0;

	isPressed      = 0;

	/* Pongo a zero l' indicatore di misura statica valida. */
	spiralatrice.misStaOk=0;
	velMMin=0.;

	nImpulsi=0;
	newnImpulsi=0;
	newgapT=0;
	oldnImpulsi=0;
	oldgapT=0;
	gapT=0;

	/* Indico che i pezzi prodotti vanno contati. */
	BitContaPezzi=1;

	vAggiornaPosizioneColtello();
    
    
	vInitAD7327();

	//initSerial();

	/* Disattivo tutte le uscite dell' 8255. */
	OutDig= outDigVariable = 0;

	actStaPortata = NUM_PORTATE - 1;
	actPortata=0;


	lastKeyPressed = 0;
	spiralatrice.misUpdateStatus=MIS_OFF_LINE;
	spiralatrice.misUpdateSleep=0;
	/* Indico che non vi sono allarmi. */
	alr_status=alr_status_noalr;

	/* Non vi e' nessun allarme per cui la commessa debba partire
	  con basso numero di giri mandrino. */
	spiralatrice.ucAlarmSlowRpm=0;
	/* La prima partenza deve sempre essere lenta. */
	spiralatrice.slowRpmMan=slowRpmManCom;

	/* La lampada � spenta. */
	LampStatus=LampSpenta;
	/* Contatore del tempo per il controllo della lampada. */
	TempoLampada=0;
	spiralatrice.TempoRefreshRampa=0;
	/* Pongo la variabile che indica la posizione di taratura della lama
	  al 100%. */
	spiralatrice.actPosLama=100;
	/* La macchina e' appena stata accesa.
	  Pongo la variabile a 100 perch�, per determinare se la macchina �
	  accesa, controllo l' ingresso di stop: se esso va a zero, � stata
	  accesa la marcia ausiliari, per cui gli allarmi che arrivano
	  sono "validi":. Per evitare rimbalzi, stop deve rimanere basso
	  per 100 cicli prima che io consideri di essere nella condizione
	  di macchina non appena accesa (100 cicli vale circa 500 ms)
	*/
	AppenaAcceso=100;
	/* Pongo a infinito il valore della resistenza mediata
	  misurata dall' ohmetro statico. */
	spiralatrice.resStaMediata=2e30;
	/* Pongo a zero buffer per calcolo resistenza statica mediata e
	  numero di campioni acquisiti. */
	spiralatrice.resStaMediataAux=0.0;
	spiralatrice.nSampleResSta=0;

	/* Valido i parametri macchina, per sicurezza. */
	ValidateMacParms();

	// reset delle finestre
	vResetWindows();

#ifndef defDisable_Anybus_Module
	// reset del modulo anybus...	
	vHMA_init();
#endif


	//....
	// initLcd();
	menuOperation(MNU_OP_PRINT);
	// indico che ho finito la fase di boot
	ucBooting=0;
    
}

void request_spindle_stop(void)
{
	type_stop_spindle * p = &spiralatrice.stop_spindle;
	p->num_req++;
}

static void update_spindle_stop(void)
{
	type_stop_spindle * p = &spiralatrice.stop_spindle;
	if (p->num_req != p->num_ack)
	{
	   p->num_ack = p->num_req;
	   p->status = enum_stop_spindle_status_init;
	}
	switch(spiralatrice.stop_spindle.status)
	{
		case enum_stop_spindle_status_idle:
		default:
		{
			break;
		}
		case enum_stop_spindle_status_init:
		{
			p->status = enum_stop_spindle_status_slow_down;
			unsigned int stop_ramp_max_length_ms = p->stop_ramp_max_length_ms;
			if ((stop_ramp_max_length_ms < def_min_stop_ramp_max_length_ms) || (stop_ramp_max_length_ms > def_max_stop_ramp_max_length_ms))
			{
				stop_ramp_max_length_ms = default_stop_ramp_max_length_ms;
			}
			unsigned int max_periods_to_stop = stop_ramp_max_length_ms / def_period_update_stop_ramp_ms;
			if (!max_periods_to_stop)
			{
				max_periods_to_stop = 1;
			}
			unsigned int spindle_dac_start_value = spiralatrice.DacOut[0];
			unsigned int n_periods_to_stop = (max_periods_to_stop * spindle_dac_start_value) >> def_num_bit_shift_FS_DAC_MAX519;
			if (!n_periods_to_stop)
			{
				n_periods_to_stop = 1;
			}
			p->final_num_periods             = n_periods_to_stop;
			p->current_num_periods           = 0;
			p->ul_prev_timer_interrupt_count = get_ul_timer_interrupt_count();
			int decrease_step_per_period     = spindle_dac_start_value / n_periods_to_stop;
			if (!decrease_step_per_period)
			{
			 decrease_step_per_period = 1;
			}
			p->num_step_decrease_per_period[0] = decrease_step_per_period;
			{
				unsigned int idxDAC;
				for (idxDAC = 1; idxDAC <= 2; idxDAC++)
				{
					unsigned int dac_start_value = spiralatrice.DacOut[2 * idxDAC];
					decrease_step_per_period = dac_start_value / n_periods_to_stop;
					if (!decrease_step_per_period)
					{
						decrease_step_per_period = 1;
					}
					p->num_step_decrease_per_period[idxDAC] = decrease_step_per_period;
				}
			}
		break;
		}
		case enum_stop_spindle_status_slow_down:
		{
			unsigned int elapsed_ms = def_period_update_stop_ramp_ms;
			unsigned long now = get_ul_timer_interrupt_count();
			if (now >= p->ul_prev_timer_interrupt_count)
			{
				elapsed_ms = (now - p->ul_prev_timer_interrupt_count) * PeriodoIntMs;
			}
			if (elapsed_ms >= def_period_update_stop_ramp_ms)
			{
				p->ul_prev_timer_interrupt_count = now;
				// spindle DAC update
				{
					unsigned int spindle_dac_value = spiralatrice.DacOut[0];
					if (spindle_dac_value >= p->num_step_decrease_per_period[0])
					{
						spindle_dac_value -= p->num_step_decrease_per_period[0];
					}
					else
					{
						spindle_dac_value = 0;
					}
					spiralatrice.DacOut[0] = spindle_dac_value;
					p->ucDacOutValues[0] = spindle_dac_value;
					// send the command to the DACs (ucDacOutValues[1] is the current driving to the spindle and it wont be changed here)
					SendByteMax519(addrDacMandrino, p->ucDacOutValues[0], spiralatrice.DacOut[1]);
				}
				// wheels DAC update
				{
					unsigned int idxDAC;
					for (idxDAC = 1; idxDAC <= 2; idxDAC++)
					{
						unsigned int dac_value = spiralatrice.DacOut[2 * idxDAC];
						unsigned int decrease_step =  p->num_step_decrease_per_period[idxDAC];
						if (dac_value >= decrease_step)
						{
							dac_value -= decrease_step;
						}
						else
						{
							dac_value = 0;
						}
						spiralatrice.DacOut[2 * idxDAC] = dac_value;
						p->ucDacOutValues[2 * idxDAC] = dac_value;
					}
					// send the command to the wheels
					SendByteMax519(addrDacFrizione1, p->ucDacOutValues[2], p->ucDacOutValues[4]);
				}
				if (++p->current_num_periods >= p->final_num_periods)
				{
					p->status = enum_stop_spindle_status_ends;
				}
			}
			break;
		}
		case enum_stop_spindle_status_ends:
		{
			p->status = enum_stop_spindle_status_idle;

			/* Indico che nessuna rampa deve essere aggiornata. */
			spiralatrice.StatusRampaDac=0;
			spiralatrice.GradinoRampaDAC=NumGradiniRampaDAC;
			// Se il motore e' acceso, lo spengo azzerando i DAC del mandrino e delle frizioni.
			unsigned int i=0;
			while (i < NumDacRampa * 2)
			{
				spiralatrice.DacOut[i]   = 0;
				spiralatrice.DacOutOld[i]= 0;
				i++;
			}
			SendByteMax519(addrDacMandrino, 0, 0);
			SendByteMax519(addrDacFrizione1, 0, 0);
			// Spengo il motore.
			outDigVariable &= ~ODG_MOTORE;
			break;
		}
	}
}

static void check_tangled_wire(void)
{
#define def_min_speed_check_wire_tangle_mmin 0.1
#define def_min_time_validate_after_lock_ms 2000
	type_check_tangled_wire * p = &spiralatrice.check_tangled_wire;
	if (!PrgRunning)
	{
		p->status = enum_status_check_tangled_wire_idle;
	}
	else switch(p->status)
	{
		case enum_status_check_tangled_wire_idle:
		default:
		{
			type_check_wire_tangled_params * p_params = &macParms.check_wire_tangled_params;
			// if program not running, or the program is empty, or check disabled (percentage is 0), or an alarm is present, better not to check
			if (!PrgRunning || hprg.theRunning.empty || (!p_params->enable) || alarms)
			{
				break;
			}
			// copy back the setup values, they are already validated
			p->min_duration_ms 	= p_params->min_duration_ms;
			p->speed_percentage = p_params->speed_percentage;
			p->status = enum_status_check_tangled_wire_wait_lock;
			p->vel_prod_to_check = -1.0;
			unsigned long now = get_ul_timer_interrupt_count();
			p->ul_prev_timer_interrupt_count = now;
			break;
		}
		case enum_status_check_tangled_wire_wait_lock:
		{
			float fv = velMMin;
			if ( (fv < def_min_speed_check_wire_tangle_mmin) || (!(spiralatrice.CheckPotMan||spiralatrice.AbilitaModificaPotenziometri)))
			{
				p->cur_duration_ms = 0;
				p->status = enum_status_check_tangled_wire_wait_lock;
				break;
			}
			else
			{
				unsigned int elapsed_ms = 0;
				unsigned long now = get_ul_timer_interrupt_count();
				if (now >= p->ul_prev_timer_interrupt_count)
				{
					elapsed_ms = (now - p->ul_prev_timer_interrupt_count) * PeriodoIntMs;
				}
				p->ul_prev_timer_interrupt_count = now;
				p->cur_duration_ms += elapsed_ms;
				if (p->cur_duration_ms >= def_min_time_validate_after_lock_ms)
				{
					p->vel_prod_to_check = fv;
					p->inv_vel_prod_to_check = 1.0 / fv;
					p->status = enum_status_check_tangled_wire_check;
				}

			}
			break;
		}
		case enum_status_check_tangled_wire_check:
		case enum_status_check_tangled_wire_validate:
		{
			// if we are going very slow, maybe the wheels are not gripping so it is better not to check
			if (velMMin < def_min_speed_check_wire_tangle_mmin)
			{
				p->status = enum_status_check_tangled_wire_wait_lock;
				p->cur_duration_ms = 0;
				break;
			}
			unsigned int elapsed_ms = def_period_check_tangled_wire_ms;
			unsigned long now = get_ul_timer_interrupt_count();
			if (now >= p->ul_prev_timer_interrupt_count)
			{
				elapsed_ms = (now - p->ul_prev_timer_interrupt_count) * PeriodoIntMs;
			}
			if (elapsed_ms >= def_period_check_tangled_wire_ms)
			{
				p->ul_prev_timer_interrupt_count = now;

				unsigned int delta_speed_0_100 = 100 * fabs((velMMin - p->vel_prod_to_check) * p->inv_vel_prod_to_check);
				if (delta_speed_0_100 > p->speed_percentage)
				{
					if (p->status == enum_status_check_tangled_wire_check)
					{
						p->cur_duration_ms = 0;
						p->status = enum_status_check_tangled_wire_validate;
					}
					else
					{
						p->cur_duration_ms += elapsed_ms;
						if (p->cur_duration_ms >= p->min_duration_ms)
						{
							p->status = enum_status_check_tangled_wire_alarm;
						}
					}
				}
				else
				{
					if (p->status != enum_status_check_tangled_wire_check)
					{
						p->status = enum_status_check_tangled_wire_check;
					}
				}
			}
			break;
		}
		case enum_status_check_tangled_wire_alarm:
		{
			// emit alarm wire tangled
			alarms    |= ALR_WIRE_TANGLED;
			alr_status = alr_status_alr;
			p->status = enum_status_check_tangled_wire_idle;
			break;
		}
	}
}





// aggiorna display
void upd7Seg(void){

	unsigned int xdat i,j,DacOutIndex,aux;
	unsigned char xdata ucDacOutValues[8];
	if (!act7segChar && !upd7segSleep) {
		// Se premuta prova statica e non sono nel menu diagnostica, sottomenu
		// in/out ana/dig, visualizzo la misura sul display 7 segmenti.
		if(  (myInpDig & IDG_MSTA)
#ifndef defTest2fili
			&& (!sLCD.ucSuspendDigitalIoUpdate)
			//&&(  (actMenu!=MENU_UTILS)
			//	||((actSubmenu!=SBM_UT_INDIG)&&(actSubmenu!=SBM_UT_OUTDIG))
			//   )
#endif
		  ){
			/* display valore di resStaMediata */
			if (spiralatrice.resStaMediata > 1e30) {
				/* Correzione: visualizzazione --- in caso di fuori
				   portata.
				   Alfabeto del display:
				   '0' '1' ... '9' '-' 'E' 'H' 'L' 'P'
				*/
				str7seg[0] = '0'+10;
				str7seg[1] = '0'+10;
				str7seg[2] = '0'+10;
				str7seg[3] = 0;

				/* Se fuori portata, accende il solo LED
				   dei kohm.
				*/
				seg7Point &= ~0x8; /* accende led Kohm */
				seg7Point |=  0x10;/* spegne  led  ohm */
			}
			else if (spiralatrice.resStaMediata > 1999.4) {
				sprintf(str7seg, "%-4.3f", spiralatrice.resStaMediata / 1000.);
				seg7Point |= 0x10; /* spegne led ohm */
				seg7Point &= ~0x8; /* accende led Kohm */
			}
			else  {
				sprintf(str7seg, "%-4.3f", spiralatrice.resStaMediata);
				seg7Point |= 0x8;
				seg7Point &= ~0x10;
			}
		}
		/* Se prg in esecuzione, visualizza velocita' in metri al minuto. */
		else if (PrgRunning){
			/* Spegnimento dei led. */
			seg7Point |= 0xf8;
			/* Se l' unita' di misura e' inch/feet,
				visualizzo la velocita' di produzione
				in feet/minuto. */
			if (nvram_struct.actUM==UM_IN)
				sprintf(str7seg, "%4.1f", velMMin*metri2feet);
			else
				sprintf(str7seg, "%4.1f", velMMin);
		}
		else {
			/* Spegnimento dei led. */
			seg7Point |= 0xf8;
			/* Visualizzazione --- in caso di misura non attiva.
				Alfabeto del display:
			    '0' '1' ... '9' '-' 'E' 'H' 'L' 'P'
			*/
			str7seg[0] = '0'+10;
			str7seg[1] = '0'+10;
			str7seg[2] = '0'+10;
			str7seg[3] = 0;
		}

		act7segChar = str7seg;
		act7segCol = 0;
		seg7Point &= 0xf8;
		upd7segSleep = INTERVAL_UPD_7SEG;
	}
   // Se non � passato il tempo di ricalcolo della velocit� di produzione, acquisisco nuovi dati.
   if (tempo*PeriodoIntMs < mymin(RefreshVelocity,RefreshRampa)) {
	  /* Lettura del tempo trascorso. */
	  /* Metto a rosso il semaforo che indica la possibilita' di aggiornare
	     il tempo trascorso. */
	updVel=0;

	newgapT=gapT;
	newnImpulsi+=nImpulsi;
	nImpulsi=0;

	/* Metto a verde il semaforo che indica la possibilita' di aggiornare
	 il tempo trascorso. */
	updVel=1;
	/* Inserisco dentro alla variabile spazio il numero di
	 impulsi conteggiati. */
	if (newnImpulsi>=oldnImpulsi){
	  spazio += newnImpulsi-oldnImpulsi;
	}
	else{
	  spazio += (0xFFFFFFFFUL-oldnImpulsi)+newnImpulsi+1;
	}
	oldnImpulsi=newnImpulsi;

	/* Inserisco dentro alla variabile tempo (e TempoLampada) il numero
		di conteggi di interrupt effettuati.
		Un conteggio avviene ogni PeriodoIntMs millisecondi.
	*/
	if (newgapT>=oldgapT){
		tempo += newgapT-oldgapT;
		spiralatrice.TempoRefreshRampa+=newgapT-oldgapT;
		TempoLampada+=newgapT-oldgapT;
		spiralatrice.TempoTaglioPezzo+=newgapT-oldgapT;
		/* Se sto valutando se un allarme � tale o � un rimbalzo,
		incremento il timer della durata dell' allarme. */
		if (alr_status==alr_status_rimb)
			spiralatrice.TempoAllarme+=newgapT-oldgapT;
	}
	else{
		tempo += (0xFFFFFFFFUL-oldgapT)+newgapT+1;
		spiralatrice.TempoRefreshRampa+=(0xFFFFFFFFUL-oldgapT)+newgapT+1;;
		TempoLampada+=(0xFFFFFFFFUL-oldgapT)+newgapT+1;
		spiralatrice.TempoTaglioPezzo+=(0xFFFFFFFFUL-oldgapT)+newgapT+1;
		/* Se sto valutando se un allarme � tale o � un rimbalzo,
		incremento il timer della durata dell' allarme. */
		if (alr_status==alr_status_rimb)
			spiralatrice.TempoAllarme+=(0xFFFFFFFFUL-oldgapT)+newgapT+1;
	}
	oldgapT=newgapT;
   }
   else  {
		/*
			Calcolo della velocit� in metri al minuto.
			spazio dice il numero di impulsi arrivati
			tempo*PeriodoIntMs dice quanto tempo � trascorso in millisecondi
			spazio/tempo � numero di impulsi/millisecondo
			moltiplicando per imptomm ottengo mm/ms==m/s
			moltiplicando per 60 ottengo m/min
		*/
		spiralatrice.spaziorv+=spazio;
		spiralatrice.temporv+=tempo;
		if (spiralatrice.temporv*PeriodoIntMs >= RefreshVelocity){
			if 	(OutDig & ODG_MOTORE)
				velMMin = 0.4*velMMin+0.6*(float)spiralatrice.spaziorv * 60. * impToMm/ (spiralatrice.temporv*PeriodoIntMs);
			else
				velMMin = 0.0;
			spiralatrice.spaziorv=0;
			spiralatrice.temporv=0;
		}

		spazio = tempo  = 0;


   }
   update_spindle_stop();

   check_tangled_wire();

	/* Ne approfitto anche per impartire un nuovo gradino alla
		rampa dei DAC, se � il caso.
	*/
	if (spiralatrice.TempoRefreshRampa*PeriodoIntMs>=RefreshRampa)
	{
		unsigned int ui_inc_step_rampa;
		ui_inc_step_rampa=(spiralatrice.TempoRefreshRampa*PeriodoIntMs+RefreshRampa/2)/RefreshRampa;
		// try to compensate for some delay in ramp generation (due to the motherfucking display fpga)
		if (ui_inc_step_rampa>3)
		{
			ui_inc_step_rampa=3;
		}
		else if (ui_inc_step_rampa<1)
		{
			ui_inc_step_rampa=1;
		}
		spiralatrice.TempoRefreshRampa=0;
		if (spiralatrice.GradinoRampaDAC<NumGradiniRampaDAC){
			/* Passo al gradino successivo. */
			spiralatrice.GradinoRampaDAC+=ui_inc_step_rampa;
			if (spiralatrice.GradinoRampaDAC>NumGradiniRampaDAC)
				spiralatrice.GradinoRampaDAC=NumGradiniRampaDAC;
			// if a friction is driven the other one should be driven too, to avoid false calculations on friction output values
			if (spiralatrice.StatusRampaDac&((1<<1)|(1<<2))){
				spiralatrice.StatusRampaDac|=((1<<1)|(1<<2));
			}
			/* Scorro i primi tre dac (mandrino, ruota 1 e ruota 2). */
			for (i=0;i<3;i++){
				/* Se se ne deve aggiornare la rampa... */
				if (spiralatrice.StatusRampaDac&(1<<i)){
					/* Calcolo l' offset delle uscite dei dac nell' array spiralatrice.DacOutValue. */
					DacOutIndex=i*2;
					/* Calcolo i valori delle due uscite. */
					for (j=0;j<2;j++){
						if (spiralatrice.DacOut[DacOutIndex]>spiralatrice.DacOutOld[DacOutIndex]){
							aux=spiralatrice.DacOut[DacOutIndex]-spiralatrice.DacOutOld[DacOutIndex];
							spiralatrice.DacOutValue_dacstep[j]=spiralatrice.DacOutOld[DacOutIndex]+(aux*spiralatrice.GradinoRampaDAC)/NumGradiniRampaDAC;
						}
						else{
							aux=spiralatrice.DacOutOld[DacOutIndex]-spiralatrice.DacOut[DacOutIndex];
							spiralatrice.DacOutValue_dacstep[j]=spiralatrice.DacOutOld[DacOutIndex]-(aux*spiralatrice.GradinoRampaDAC)/NumGradiniRampaDAC;
						}
						ucDacOutValues[DacOutIndex]=spiralatrice.DacOutValue_dacstep[j];
						/* Passo al prossimo elemento degli array delle uscite. */
						DacOutIndex++;
					}
					/* Spedisco al dac i valori delle uscite appena calcolate. */
					// non posso fare cos� perch� stata spostata l'uscita della velocit� della ruota 2
					// SendByteMax519(i, spiralatrice.DacOutValue[0], spiralatrice.DacOutValue[1]);
				}
			}
			// se richiesto, aggiorno rampa mandrino
			if (spiralatrice.StatusRampaDac&(1<<0)){
				SendByteMax519(addrDacMandrino,ucDacOutValues[0],ucDacOutValues[1]);
			}
			// se richiesto, aggiorno rampa ruote di contrasto
			// attenzione perch� ruota contrasto2 portata subito dopo ruota contrasto1
			if (spiralatrice.StatusRampaDac&((1<<1)|(1<<2))){
				SendByteMax519(addrDacFrizione1,ucDacOutValues[2],ucDacOutValues[4]);
			}
		}
		/* Se la rampa � esaurita, cancello i flag che indicano su quali
		   DAC andava fatta. */
		else{
			spiralatrice.StatusRampaDac=0;
			for (i=0;i<NumDacRampa*2;i++)
				spiralatrice.DacOutOld[i]=spiralatrice.DacOut[i];
		}
	}
	
#if 0
   if (act7segChar) {
      if (act7segCol == 0 && *act7segChar == '0')
	 ++act7segChar;

      if (act7segCol == 0 && *act7segChar != '1')
	 p7segData[act7segCol++] = 5; /* prima cifra vuota */

      if (*act7segChar == '.') {
	 /* attiva punto corrispondente ad act7segCol */
	 seg7Point |= (0x8 >> act7segCol);
	 ++act7segChar;
      }

      p7segData[act7segCol] = (unsigned char)(*act7segChar - '0');

      ++act7segChar;
      if (++act7segCol >= SEG7_COLS)
	 act7segChar = 0;
   }

   *p7segPoint = seg7Point;
#endif
}





void updLcd(void){
	xdata unsigned char i;
	for (i=0;i<20;i++){
		if (edit.actLcdChar) {
			if (!*edit.actLcdChar) {
				if (++edit.actRowLcd >= NUM_LINES_LCD) {
					edit.actRowLcd  = 0;
					edit.actLcdChar = 0;
					if (isEdit && checkPassw && !checkedPassw)
						startEdit();
					if (!isEdit && checkPassw) {
						if (checkedPassw) {
							menuOperation(MNU_OP_ENTER);
							startEdit();
						}
						checkPassw = 0;
					}
					return;
				}
				else
					edit.actLcdChar = edit.actLcdStr[edit.actRowLcd];
				edit.actColLcd = 0;
			}

			if (*edit.actLcdChar) {
				if (  (edit.tipoPrint==PrintWithPicture)
					&&(edit.actStrReverse[edit.actColLcd]=='1')
					&&((edit.actRowLcd+1)&edit.LineaPicture)
					){
					*edit.actLcdChar=ReverseChar(*edit.actLcdChar);
				}
				vSetTextRigaColonna(edit.actRowLcd,edit.actColLcd++);
				vPrintDirectChar(*edit.actLcdChar++);

			}
		}
		else
			break;
	}
}


/* Funzione che aggiorna gli ingressi:
   - legge il tasto premuto, se � il caso;
   - aggiorna gli ingressi digitali;
*/
void updInp(void){
	xdat unsigned int ritardoReadADC;
	myInpDig = InpDig;

	// LA CONDIZIONE DI MACCHINA APPENA ACCESA SI ESAURISCE QUANDO l' ingresso di stop va a zero,
	// vale a dire quando si attiva la marcia	ausiliari.
	if (AppenaAcceso&&((myInpDig&IDG_STOP)==0))
		AppenaAcceso--;

	// Se sono nella pagina diagnostica digitale, sottomenu in/out ana/dig, nessun allarme va controllato.
#ifndef defTest2fili
	#if defIgnoreAllAlarms==0
	if(
			(!defIgnoreAllAlarms)
		&&(!sLCD.ucSuspendDigitalIoUpdate
		  //(actMenu!=MENU_UTILS)
	      //||(actSubmenu!=SBM_UT_INDIG)
	     )
	 ){
	 #else
	 	if (0){
	 #endif
#else
	if (!defIgnoreAllAlarms){
#endif
	#if defIgnoreAllAlarms==0
		if (alr_status!=alr_status_alr)
			alarms=0;
		if (myInpDig & IDG_ELETTROM)
			alarms |= ALR_ELETTR;
		if ((myInpDig & IDG_FILOAGGROV)&&(PrgRunning)){
			// devo rinfrescare lcd...
			if (alr_status!=alr_status_alr)
				vForceLcdRefresh();
			// Non e' prevista isteresi sull' allarme filo aggrovigliato, per cui passo subito allo stato di allarme presente.
			alr_status=alr_status_alr;
			alarms |= ALR_FILOAGGROV;
		}
		if (myInpDig & IDG_FRIZ1)
			alarms |= ALR_FRIZ1 ;
		if (myInpDig & IDG_FRIZ2)
			alarms |= ALR_FRIZ2 ;
		if (  (PrgRunning)
			&&!((hprg.theRunning.num_fili>=2)&&(hprg.theRunning.diametro_filo>=macParms.DiamTestIntrz))
			){
			if (myInpDig & IDG_INTRZ1)
				alarms |= ALR_INTRZ1;
			if ((hprg.theRunning.num_fili>=2)&&(myInpDig & IDG_INTRZ2))
				alarms |= ALR_INTRZ2;
		}
		if (myInpDig & IDG_DROK)
			alarms |= ALR_DROK;
		// Se vi � un allarme, prima di segnalarlo lo valido, nel senso
		// che deve rimanere attivo un certo numero di ms prima di
		// segnalarlo come valido.
		if (alarms){
			// Se non vi era allarme in precedenza, parto col timer ed
			// indico che sto valutando se si tratta di un rimbalzo.
			if (alr_status==alr_status_noalr){
				// Indico che sto valutando se si tratta di un rimbalzo.
				alr_status=alr_status_rimb;
				// Porto a zero il conteggio del cronometro allarme.
				spiralatrice.TempoAllarme=0L;
				// Per il momento non vi � ancora allarme.
				spiralatrice.oldalarms=0;
			}
			// Se stavo valutando se era un rimbalzo ed � scaduto il tempo
			// di validazione, allora � effettivamente un allarme.
			else if (  (alr_status==alr_status_rimb)
					  &&(  (!AppenaAcceso&&spiralatrice.TempoAllarme*PeriodoIntMs>macParms.durata_all)
					     ||((alarms&ALR_DROK)&&AppenaAcceso&&(spiralatrice.TempoAllarme*PeriodoIntMs>durata_all_drmand))
					    )
				    ){
				// E' effettivamente un allarme valido.
				alr_status=alr_status_alr;
				// devo rinfrescare lcd...
				vForceLcdRefresh();

			}
		}
		// Se non vi sono allarmi, porto lo stato degli allarmi a nessun allarme presente.
		else{
			spiralatrice.oldalarms=0;
			alr_status=alr_status_noalr;
		}
		// Se vi e' effettivamente un allarme valido, controllo se e' uno di quelli che devono causare una partenza
		// "lenta" della prossima commessa.
		if (  (alr_status==alr_status_alr)
			 &&(alarms&maskAlarmsSlowRpm)
			){
			spiralatrice.ucAlarmSlowRpm=1;
		}
		#endif
	}

	if (ucNewKeyPressed()) {
		lastKeyPressed  = ucGetKeyPressed();
		if (lastKeyPressed == 0)
			lastKeyPressed = KEY_F1;
		isPressed = 1;
	}
	else {
		isPressed = 0;
	}



   /* Leggo ciclicamente il contenuto dell' adc e piazzo i valori
      letti nell' array InputAnalog[].
      Se sto tarando gli ohmetri, seleziono la portata adatta
      prima di leggere dall' adc.
      Se sto leggendo dai canali 3 e 4 (assorbimento di corrente delle
      ruote di contrasto), devo attivare il bit di conversione bipolare
      dell' adc.
   */

   /*
      Se programma con misura ohmica in esecuzione e la torretta
      indica che la misura � valida, non leggo dall' adc.
      Inoltre non leggo se misura statica in corso.
   */
   if (   !(myInpDig & IDG_MSTA)
		&&!(PrgRunning&&withMis&&criticMis)
		&&(!misStaSleep)
      ){
		spiralatrice.misUpdateStatus=MIS_COMM_MUX;
		if ((spiralatrice.TaraPortate!=TaraNulla)&&(spiralatrice.ChangePortata)){
			/* actPortata==0 ==> portata mimima, corrente massima
			actPortata==NUM_PORTATE-1 ==> portata massima, corrente minima
			*/
			spiralatrice.ChangePortata=0;
			/* Porto a zero il numero di conteggi di misure effettuate. */
			spiralatrice.nMisAssorb=0;
			/* Annullo anche la media delle misure. */
			spiralatrice.auxMediaCanaleAssorb=0;
			if (spiralatrice.actCanPortata==N_CANALE_OHMETRO_STA){
				outDigVariable &= 0xfff8;
				outDigVariable |= ODG_MSTA;
				outDigVariable |= ((0x8 >> (spiralatrice.actTaraPortata)) & 0x7);
				misStaSleep= 2*SLEEP_CHANGE;
				spiralatrice.numCanaleUpdate=N_CANALE_OHMETRO_STA;
			}
			else{
				vUpdateDacPortValue((enum_select_dac_portata)spiralatrice.actTaraPortata);
				misStaSleep= 2*SLEEP_CHANGE;
				spiralatrice.numCanaleUpdate=N_CANALE_OHMETRO_DYN;
			}
			goto NonLeggereAdcUpdate;
		}

		// aspetto che venga rinfrescata la lettura...
		if (uiCountToVerifyHasBeenGetNewSample==uiNumLettureMisuraDinamica_Interrupt)
			goto NonLeggereAdcUpdate;
		uiCountToVerifyHasBeenGetNewSample=uiNumLettureMisuraDinamica_Interrupt;
		//vTestAD7327();
		/* Lettura misura che aggiorna i valori analogici. */
		spiralatrice.misUpdate =iArrayConvAD7327[spiralatrice.numCanaleUpdate];
		/* A 16 Mhz, devo inserire una pausa tra una lettura e l' altra se
			non � in esecuzione un programma, altrimenti le letture sono
			troppo ravvicinate.
		*/
		if (!PrgRunning)
			for (ritardoReadADC=0;ritardoReadADC<100;ritardoReadADC++);
		/* Controllo per sicurezza che non sia subentrata un' interruzione... */
		if (!(PrgRunning&&withMis&&criticMis)){
			if (  (spiralatrice.numCanaleUpdate==N_CANALE_ASSORB1)
				||(spiralatrice.numCanaleUpdate==N_CANALE_ASSORB2)
				){
				/* Misura bipolare: 0 � al centro del range di conversione.
				   La misura e' espressa in C2.
				*/
				/* Misura negativa, estendo il bit del segno. */
				// if (spiralatrice.misUpdate&0x800)
				//	spiralatrice.misUpdate=(spiralatrice.misUpdate|0xFFFFF000L);
				/* Dato che la misura della corrente assorbita balla,
				   effettuo una media su una serie di misure. */
				/* Incremento il numero di misure effettuate dal canale. */
				spiralatrice.nMisAssorb++;
				/* Aggiungo il valore letto alla media. */
				spiralatrice.auxMediaCanaleAssorb+=spiralatrice.misUpdate;
				/* Se ho raggiunto il numero di campioni prestabilito... */
				if (spiralatrice.nMisAssorb==N_SAMPLE_CAN_ASSORB){
					/* Faccio la media dei campioni. */
					InputAnalog[spiralatrice.numCanaleUpdate]=spiralatrice.auxMediaCanaleAssorb/N_SAMPLE_CAN_ASSORB;
					/* Azzero media e numero di campioni. */
					spiralatrice.auxMediaCanaleAssorb=0;
					spiralatrice.nMisAssorb=0;
					/* Passo al canale successivo. */
					spiralatrice.numCanaleUpdate=(spiralatrice.numCanaleUpdate+1)%NUM_INP_ANALOG;
				}
			}
			 /* Se sto tarando, accumulo misure... */
			 else if (  (spiralatrice.TaraPortate!=TaraNulla)
				  &&(spiralatrice.actCanPortata==spiralatrice.numCanaleUpdate)
				 ){
				  ++spiralatrice.nMisAssorb;
				  /* Aggiungo il valore letto alla media. */
				  spiralatrice.auxMediaCanaleAssorb+=spiralatrice.misUpdate;
				  /* Se ho effettuato sufficienti misure, faccio la media... */
				  if (spiralatrice.nMisAssorb==NMISURE_TARATURA){
					 /* Faccio la media dei campioni. */
					 fMeanValueInputAnalogOnTaratura=((float)spiralatrice.auxMediaCanaleAssorb)/NMISURE_TARATURA;
					 InputAnalog[spiralatrice.numCanaleUpdate]=fMeanValueInputAnalogOnTaratura;
					 /* Azzero media e numero di campioni. */
					 spiralatrice.auxMediaCanaleAssorb=0;
					 spiralatrice.nMisAssorb=0;
					 /* Passo al canale successivo. */
					 spiralatrice.numCanaleUpdate=(spiralatrice.numCanaleUpdate+1)%NUM_INP_ANALOG;
				  }
			 }
			 else{
				InputAnalog[spiralatrice.numCanaleUpdate]=spiralatrice.misUpdate;
				/* Passo al canale successivo. */
				spiralatrice.numCanaleUpdate=(spiralatrice.numCanaleUpdate+1)%NUM_INP_ANALOG;
			 }
			 spiralatrice.misUpdateStatus=MIS_OFF_LINE;
			 /* Se sono tornato al canale 2, e se il display non � in corso
				di aggiornamento, indico di effettuare un update dei valori.
			 */
			 if (spiralatrice.numCanaleUpdate==2){
				if (spiralatrice.TaraPortate!=TaraNulla){
				   menuOperation(MNU_OP_UPDATE);
				   vUpdateValoriTaratura();
				   vForceLcdRefresh();
				   /* Se stavo tarando, ristabilisco la portata massima. */
				   if (spiralatrice.actCanPortata==N_CANALE_OHMETRO_STA){
					  outDigVariable &= ~ODG_MSTA;
					  outDigVariable &= 0xfff8;
				   }
				   else
						/* Se stavo tarando, ristabilisco la portata massima. */
						vUpdateDacPortValue((enum_select_dac_portata)0);
				}
				else if ((!edit.actLcdChar)&&(!isEdit)){
				//   menuOperation(MNU_OP_UPDATE);
				}
			 }
		}
   } /* end if (!(PrgRunning&&withMis&&!(InpDig & IDG_MIS_VALID1)))  */
NonLeggereAdcUpdate:;
}







// collaudo az4181
typedef enum{
	enumStatoBandella_Idle=0,
	enumStatoBandella_InitWaitDelay,
	enumStatoBandella_WaitDelay,
	enumStatoBandella_InitWaitDuration,
	enumStatoBandella_WaitDuration,
	enumStatoBandella_numof
}enumTipoStatoBandella;
typedef struct _TipoStructHandleBandella{
	enumTipoStatoBandella stato;
	unsigned long ulCounterMs;
	unsigned char ucEnabled;
	unsigned char ucPreviousCnt;
	unsigned char ucActCnt;
	unsigned int uiElapsedTimeMs;
}TipoStructHandleBandella;

xdata TipoStructHandleBandella handleBandella;
xdata unsigned char ucReinitStatusBandella;

// funzione per impostare un bit della parte alta dei digtal outputs
// passare come parametro la maschera definita dalle defines odg_...
void vSetBitOutputsHi(unsigned int uiMaskOutputsHi){
    T0TCR = 0; // disable timer
		Outputs_Hi|=(uiMaskOutputsHi>>8);
		// 1.42: imposto anche valore della variabile outDigVariable
		// visto che uscite sono comandate nel loop main...
		outDigVariable|=uiMaskOutputsHi;
    T0TCR = 1; // enable timer
}


// funzione per resettare un bit della parte alta dei digital outputs
// passare come parametro la maschera definita dalle defines odg_...
void vResetBitOutputsHi(unsigned int uiMaskOutputsHi){
    T0TCR = 0; // disable timer
		Outputs_Hi&=~(uiMaskOutputsHi>>8);
		// 1.42: imposto anche valore della variabile outDigVariable
		// visto che uscite sono comandate nel loop main...
		outDigVariable&=~uiMaskOutputsHi;
    T0TCR = 1; // enable timer
}


void vInitBandella(void){
	ucReinitStatusBandella=0;
	memset(&handleBandella,0,sizeof(handleBandella));
	// durata di attivazione=0 o >=32000 -->bandella disabilitata
	// ritardo di attivazione=0 o >=32000 -->bandella disabilitata
	if (macParms.uiDurataAttivazioneBandella_centesimi_secondo==0x0000){
		handleBandella.ucEnabled=0;
	}
	else{
		handleBandella.ucEnabled=1;
	}
	if (handleBandella.ucEnabled){
		// reset uscita bandella
		vResetBitOutputsHi(ODG_BANDELLA);
	}
}

// reinizializzazione dello stato di pilotaggio della bandella
void vCommandSwitchBandella(void){
	handleBandella.stato=enumStatoBandella_InitWaitDelay;
}



void vHandleBandella(void){
	if (!handleBandella.ucEnabled)
		return;
	if (ucReinitStatusBandella){
		vCommandSwitchBandella();
		ucReinitStatusBandella=0;

	}
	handleBandella.ucActCnt=ucCntInterrupt;
	if (handleBandella.ucActCnt>=handleBandella.ucPreviousCnt)
		handleBandella.uiElapsedTimeMs=handleBandella.ucActCnt-handleBandella.ucPreviousCnt;
	else{
		handleBandella.uiElapsedTimeMs=(256U-handleBandella.ucPreviousCnt)+handleBandella.ucActCnt;
	}
	handleBandella.uiElapsedTimeMs*=PeriodoIntMs;
	handleBandella.ucPreviousCnt=handleBandella.ucActCnt;
	switch(handleBandella.stato){
		case enumStatoBandella_Idle:
		default:
			break;
		case enumStatoBandella_InitWaitDelay:
			handleBandella.ulCounterMs=0;
			handleBandella.ucPreviousCnt=ucCntInterrupt;
			handleBandella.stato=enumStatoBandella_WaitDelay;
			// reset uscita bandella
			vResetBitOutputsHi(ODG_BANDELLA);
			break;
		case enumStatoBandella_WaitDelay:
			handleBandella.ulCounterMs+=handleBandella.uiElapsedTimeMs;
			if (handleBandella.ulCounterMs>=macParms.uiRitardoAttivazioneBandella_centesimi_secondo*10L){
				handleBandella.stato=enumStatoBandella_InitWaitDuration;
			}
			break;
		case enumStatoBandella_InitWaitDuration:
			handleBandella.ulCounterMs=0;
			handleBandella.ucPreviousCnt=ucCntInterrupt;
			handleBandella.stato=enumStatoBandella_WaitDuration;
			// set uscita bandella
			vSetBitOutputsHi(ODG_BANDELLA);
			break;
		case enumStatoBandella_WaitDuration:
			handleBandella.ulCounterMs+=handleBandella.uiElapsedTimeMs;
			if (handleBandella.ulCounterMs>=macParms.uiDurataAttivazioneBandella_centesimi_secondo*10L){
				handleBandella.stato=enumStatoBandella_Idle;
				// reset uscita bandella
				vResetBitOutputsHi(ODG_BANDELLA);
			}
			break;
	}
}



/* Routine che aggiorna le uscite; tutte le altre routine scrivono i valori
   su delle variabili di appoggio; questa routine li trasferisce
   ai dispositivi.
*/

void updOut(void){
	// aggiorno la bandella
	vHandleBandella();
   /* Output dei valori digitali verso il port B dell' 8255.
      Facendo la or delle due azioni di spi e di spimis non posso
      cancellare un' azione di taglio o di pretaglio impostata
      da spimis.
      In altre parole, se vi e' un programma in esecuzione oppure e' in
      corso un taglio manuale, la responsabilita' del controllo del coltello
      e' demandata in toto alla routine di servizio dell' interrupt.
   */
   if (PrgRunning||TaglioAsincrono){
       OutDig     = ((outDigVariable&0xFF)&~(ODG_TAGLIO|ODG_PRETAGLIO))|(outDigMis&(ODG_PRETAGLIO|ODG_TAGLIO));
	   Outputs_Hi = ((outDigVariable&~ODG_DISTANZIA)>>8)|(outDigMis_Hi&(ODG_DISTANZIA>>8));
   }
   else{
		OutDig     = (outDigVariable&0xFF);
		Outputs_Hi = outDigVariable>>8;
   }
   /* Aggiornamento del display Lcd. */
   if (edit.actLcdChar)
      updLcd();
   /* Aggiornamento del display a 7 segmenti. */
   upd7Seg();
   if (TaglioAsincrono){
       if (avvStatus == AVV_IDLE)
		avvStatus = AVV_WAIT_PRE;
   }

   /* 7/95; se il motore e` fermo (ciclo terminato o stoppato) assicuro
      la corrente minima sulla torretta */
	if (!(outDigVariable & ODG_MOTORE)) {
		if (  (spiralatrice.misUpdateStatus == MIS_OFF_LINE)
			&&(spiralatrice.TaraPortate==TaraNulla)
			&&(!sLCD.ucAttivataMisuraDinamica)
			){
			if (ui_read_dac_selected_portata()!=(enum_select_dac_portata)0)
				vUpdateDacPortValue((enum_select_dac_portata)0);
		}
	}
#if 0
#ifdef _CC51
   if (watchDog = !watchDog) {
#pragma asm
      setb   P1.6
#pragma endasm
   }
   else {
#pragma asm
      clr   P1.6
#pragma endasm
   }
#endif
#endif
}


void TxCommessa(unsigned int uiTheCode){
	uiTheCode=0;
#if 0
xdat unsigned int PezziFatti;
xdat float PezziScarto;
xdat char sData1[25],sData2[25],s3[50],s4[100];
      leggiDataOra();
      sprintf(sData1,"%02u:%02u %02u/%02u/%02u\n\r",
	       (unsigned int)SaveDate.rtcHou,
	       (unsigned int)SaveDate.rtcMin,
	       (unsigned int)SaveDate.rtcDay,
	       (unsigned int)SaveDate.rtcMon,
	       (unsigned int)SaveDate.rtcYea
	     );
      sprintf(sData2,"%02u:%02u %02u/%02u/%02u\n\r",
	       (unsigned int)rtcHou,
	       (unsigned int)rtcMin,
	       (unsigned int)rtcDay,
	       (unsigned int)rtcMon,
	       (unsigned int)rtcYea
	      );
      PezziFatti=actNumPezzi+spiralatrice.NumPezziScarto-SaveDate.pezzi;
      if (PezziFatti){
	  PezziScarto=(float)((signed int)spiralatrice.NumPezziScarto-(signed int)SaveDate.scarti+(signed int)nvram_struct.commesse[spiralatrice.runningCommessa].magg);
	  PezziScarto=PezziScarto*100./PezziFatti;
      }
      else
	  PezziScarto=0.0;
      sprintf(s3,"%u\n\r%6.3f\n\r%6.3f\n\r%6.3f\n\r%6.3f\n\r",
	       (unsigned int)PezziFatti,
	       (float)PezziScarto,
	       (float)velMMin,
	       (float)nvram_struct.commesse[spiralatrice.runningCommessa].res,
	       (float)nvram_struct.resspec[0]
	      );
      sprintf(s4, "%u.%u\n\r%s\n\r%s\n\r%s%6.3f\n\r",
	       (unsigned int)nvram_struct.commesse[spiralatrice.runningCommessa].numcommessa,
	       (unsigned int)nvram_struct.commesse[spiralatrice.runningCommessa].subnum,
	       nvram_struct.commesse[spiralatrice.runningCommessa].commessa,
	       hprg.theRunning.codicePrg,
	       s3,
	       /* Se 1 solo filo, trasmetto 0 come res. spec secondo filo. */
	       (float)nvram_struct.resspec[1]*(hprg.theRunning.num_fili-1)
	     );
      sprintf(serTx.p, "\n\r%s%s%s%u\n\r\n\r%c%c",
	       s4,
	       sData1,
	       sData2,
	       code,
	       EOT,
	       EOT
	      );
      txReqLocalCommessa = 0;
      goTx();
#endif
}

#if 0
	static void updSer(){
	   #if 0==1
	   /* ATTENZIONE !!! */
	   /* ATTENZIONE !!! */
	   /* ATTENZIONE !!! */
	   /* Ogni messaggio in trasmissione va chiuso col carattere eot. */
	   /* Ogni messaggio in trasmissione va chiuso col carattere eot. */
	   /* Ogni messaggio in trasmissione va chiuso col carattere eot. */
	   /* Ogni messaggio in trasmissione va chiuso col carattere eot. */
	   /* Ogni messaggio in trasmissione va chiuso col carattere eot. */
	   if (rxHasMsg) {
		  /* interpreta il messaggio remoto e risponde */

		  switch (rxOpcode) {
		 case OPC_REQ_LCOM:
			txReqLocalCommessa = 1;
			mymemcpy(serTx.p, serRx.p, LEN_EXTRA_TEXT);
			goTx();
			break;

		 case OPC_MOD_CONT:
			txModCont = !txModCont;
			mymemcpy(serTx.p, serRx.p, LEN_EXTRA_TEXT + 1);
			goTx();
			break;
		  }

		  serRx.i = 0;
		  rxHasMsg = 0;
		  REN = 1;

	   }
	   if (txModCont && txNewPz && !txBusy) {
		  sprintf(serTx.p, "L = %-8.1f %s / R = %-6.1f ohm\r\n", (float)lastPzLenImp * impToMm,  nvram_struct.actUM == UM_IN ? "in" : "mm",
			  withMis ? (targetRes / TARGET_URP * (float)lastPzRes) : ((float)lastPzLenImp * impToMm / 1000. * nvram_struct.commesse[spiralatrice.runningCommessa].resspec_spir));
		  txNewPz = 0;
		  goTx();

	   }

	   if (txReqLocalCommessa && !txBusy) {
		  sprintf(serTx.p, "%s : R = %-6.1fohm  / Q = %-5d\r\n",
			   nvram_struct.commesse[spiralatrice.runningCommessa].numcommessa,
			   nvram_struct.commesse[spiralatrice.runningCommessa].res,
			   nvram_struct.commesse[spiralatrice.runningCommessa].ndo
			  );

		  txReqLocalCommessa = 0;
		  goTx();

	   }
	   serialTx();
	   #endif
	}
#endif



// routine di attesa avvio fpga
void vWaitStartupFpga(void){
	// numero di ms di attesa avvio fpga
	#define defWaitStartupFpgaMs 600
	// contatore che viene incrementato ad ogni interrupt
	extern volatile unsigned char  ucCntInterrupt;
	volatile unsigned char xdata ucSaveCntInterrupt;
	xdata unsigned int uiNumLoop;
	// attendo semplicemente che si avvii fpga, per evitare di inizializzare lcd prima che fpga sia partito
	for (uiNumLoop=0;uiNumLoop<(defWaitStartupFpgaMs/PeriodoIntMs);uiNumLoop++){
		ucSaveCntInterrupt=ucCntInterrupt;
		while(ucSaveCntInterrupt==ucCntInterrupt);
	}

}



void spi_main(void){
	void vOpenNand(unsigned char ucFormatFlash);
	void vHandleButtons(void);
	unsigned char ucContaLoopMain;
	void vRefreshLcdStrings(void);
	extern unsigned char ucNeedToRefreshLcd(void);
	// verifies if i2c bus should be unlocked...
	void v_verify_unlock_i2c_bus(void);
	//unsigned char ucTouchPressed;
	void v_handle_eeprom(void);
	
//	extern void main_anybus(void);


	// aspetto che fpga si avvii--> no! si attende gi� nel main...
	//vWaitStartupFpga();
	// inizializzazione lcd
	vInitLcd();
	// beeep lcd per avvisare che � pronto
	LCD_Beep();


	// inizializzazione di tutte le vars del modulo
	init();
	// indico di visualizzare lo splash screen per tot secondi
	vVisualizeSplashScreen();
	
	
	// chiamo il programma di test modulo anybus
//	main_anybus();

	// loop esecuzione programma
	for (;;) {
		updInp();
		inpToOut();
		updOut();
		// verifico se touch pressed
		ucBackGroundCtrlLcd();
		vHandleButtons();
		if (  // ucTouchPressed
			 //||((ucContaLoopMain&0x0F)==0)
			 //||
			 ucNeedToRefreshLcd()
			){
			ucContaLoopMain=0;
			ucClearScreen();
			vRefreshLcdStrings();
			vRefreshLcd(enumRefreshLcd_Timeout);
			vHandleBackgroundNandFlash();
		}
		ucContaLoopMain++;
		// rinfresco lettura a/d...
		vReadAD7327();
		// verifico se max parms vanno salvati...
		vCtrlSaveMacParms();
		// lettura contatore numero 2
		iReadCounter_2();
		// gestione dell'encoder di regolazione delle velocit�
		vHandleHRE();
		// verifies if i2c bus should be unlocked...
		v_verify_unlock_i2c_bus();
		// handles eeprom r/w
		v_handle_eeprom();
#ifdef def_canopen_enable
        {
            static unsigned char uc_elapsed_time_initialized=0;
            static unsigned char uc_last_clock_canopen;
            unsigned long ul_canopen_time_elapsed_ms;
            unsigned char uc_act_time;
            if (!uc_elapsed_time_initialized){
                uc_elapsed_time_initialized=1;
                uc_last_clock_canopen=ucCntInterrupt;
            }
            uc_act_time=ucCntInterrupt;
            if (uc_act_time>=uc_last_clock_canopen){
                ul_canopen_time_elapsed_ms=uc_act_time-uc_last_clock_canopen;
            }
            else{
                ul_canopen_time_elapsed_ms=(256UL+uc_act_time)-uc_last_clock_canopen;
            }
            uc_last_clock_canopen=uc_act_time;
            ul_canopen_time_elapsed_ms*=PeriodoIntMs;
            if (ui_canopen_process(ul_canopen_time_elapsed_ms)){
                //canopen process ends...
            }
        }
#endif
    
#ifndef defDisable_Anybus_Module
		// gestione comunicazione modulo anybus...
		vHMA_handle();
		vHMA_handle();
		vHMA_handle();
		vHMA_handle();
		vHMA_handle();
#endif
#ifdef def_use_emac
	az4186_emac_loop();
#endif

	} /* for(;;) */

}//void spi_main(void)


// routine che prepara nella stringa pDst
// la descrizione formattata della compensazione di temperatura dell'evento di log produzione puntato da pRL
unsigned int uiPrepareDescTempComp_ProductionLog(char *pDst,TipoRecordLog *pRL, unsigned int ui_max_bytes){
	unsigned int ui_retcode;
	ui_retcode=0;
	if (!macParms.ucAbilitaCompensazioneTemperatura
	    ||(!macroIsCodiceProdotto_TemperatureCompensated(&hprg.theRunning))
	   ){
		return 0;
	}
	
	switch(pRL->event_type){
		case let_newspezzone:
			{
				TipoStructTempCompStats *p;
				p=&(pRL->u.newSpezzone.temperature_compensation_stats);
				// check if compensation running fine looking at the number of samples acquired...
				if (p->i_num_samples_acquired){
					ui_retcode=1;
					// visualizzo parametri della compensazione di temperatura associata al chunk
					snprintf(pDst,ui_max_bytes,"T=%4.1f*C DeltaR=%+3.1f%% #=%i, Tout=%i",
							f_get_temperature_from_normalized_one(p->i_act_temperature_celsius_scaled),
							p->i_act_compensation_permille*0.1,
							p->i_num_samples_acquired,
							p->i_num_of_timeout
							);
				}
				break;
			}
		case let_cut:
			break;
		case let_bad_measure:
			break;
		case let_spezzone_alarm:
			break;
		default:
			break;
	}
	return ui_retcode;
}


// routine che prepara nella stringa pDst
// la descrizione formattata dell'evento di log produzione puntato da pRL
// se idx=0xFF allora NON vengono usati i parametri ucActSpez e idx
// se idx!=0xFF allora ucActSpez indica se questa � la riga corrente, mentre idx � l'indice riga nella finestra
void vPrepareDesc_ProductionLog(xdata char *pDst,TipoRecordLog xdata *pRL,unsigned char ucActSpez,unsigned char idx){
	xdata float fDeltaRo,fDeltaRo2;
	xdata unsigned char ucPrefixString[12];
	float f_ohm_m,f_ohm_feet;
	// devo rappresentare spezzone attuale e indice riga nella finestra???
	if (idx!=0xFF)
		sprintf((char*)ucPrefixString," %c%2i",ucActSpez,(int)idx);
	else
		ucPrefixString[0]=0;

	switch(pRL->event_type){
		case let_numof:
		default:
		{
			break;
		}
		case let_newspezzone:
			if (pRL->u.newSpezzone.ulNomRo)
				fDeltaRo=100.0/(signed long)pRL->u.newSpezzone.ulNomRo;
			else
				fDeltaRo=1;
			if (pRL->u.newSpezzone.ulRo>pRL->u.newSpezzone.ulNomRo)
				fDeltaRo2=pRL->u.newSpezzone.ulRo-pRL->u.newSpezzone.ulNomRo;
			else{
				fDeltaRo2=pRL->u.newSpezzone.ulNomRo-pRL->u.newSpezzone.ulRo;
				fDeltaRo2*=-1;
			}
			if (pRL->u.newSpezzone.ulNomRo)
				fDeltaRo=fDeltaRo2*fDeltaRo;
			else
				fDeltaRo=0;
			f_ohm_m=(((float)(pRL->u.newSpezzone.ulRo))/(TARGET_URP * MAGN_LEN * impToMm / 1000. /targetRes ));
			f_ohm_feet=f_ohm_m*feet2metri;
			// visualizzo parametri del chunk
			sprintf(pDst,"%s chk %5.3fohm %5.3f%s %+3.1f%%%4i",
					ucPrefixString,
					(pRL->u.newSpezzone.ulRo*(float)macParms.lenSemigiro*(float)targetRes)/((float)TARGET_URP*(float)MAGN_LEN),
					(nvram_struct.actUM==UM_IN)?f_ohm_feet:f_ohm_m,
					(nvram_struct.actUM==UM_IN)?"ohm/feet":"ohm/m",
					fDeltaRo,
					(int)pRL->misDig
					);
			break;
		case let_cut:
			sprintf(pDst,"%s cut at %5.3f ohm",
					ucPrefixString,
					(float)((pRL->u.cut.ulCutImp)*targetRes/pRL->u.cut.ulExpectedImp)
					);
			break;
		case let_bad_measure:
			if (pRL->u.bad_measure.ulNomRo)
				fDeltaRo=100.0/(signed long)pRL->u.bad_measure.ulNomRo;
			else
				fDeltaRo=1;
			if (pRL->u.bad_measure.ulRo>pRL->u.bad_measure.ulNomRo)
				fDeltaRo2=pRL->u.bad_measure.ulRo-pRL->u.bad_measure.ulNomRo;
			else{
				fDeltaRo2=pRL->u.bad_measure.ulNomRo-pRL->u.bad_measure.ulRo;
				fDeltaRo2*=-1;
			}
			if (pRL->u.bad_measure.ulNomRo)
				fDeltaRo=fDeltaRo2*fDeltaRo;
			else
				fDeltaRo=0;
			f_ohm_m=(((float)(pRL->u.bad_measure.ulRo))/(TARGET_URP * MAGN_LEN * impToMm / 1000. /targetRes ));
			f_ohm_feet=f_ohm_m*feet2metri;
				
			sprintf(pDst,"%s badMeas %5.3fohm %5.3f%s %+3.1f%%",
					ucPrefixString,
					(pRL->u.newSpezzone.ulRo*(float)macParms.lenSemigiro*(float)targetRes)/((float)TARGET_URP*(float)MAGN_LEN),
					(nvram_struct.actUM==UM_IN)?f_ohm_feet:f_ohm_m,
					(nvram_struct.actUM==UM_IN)?"ohm/feet":"ohm/m",
					fDeltaRo
					);
			break;
		case let_spezzone_alarm:
			if (pRL->u.spezzone_alarm.ulNomRo)
				fDeltaRo=100.0/(signed long)pRL->u.spezzone_alarm.ulNomRo;
			else
				fDeltaRo=1;
			if (pRL->u.spezzone_alarm.ulRo>pRL->u.spezzone_alarm.ulNomRo)
				fDeltaRo2=pRL->u.spezzone_alarm.ulRo-pRL->u.spezzone_alarm.ulNomRo;
			else{
				fDeltaRo2=pRL->u.spezzone_alarm.ulNomRo-pRL->u.spezzone_alarm.ulRo;
				fDeltaRo2*=-1;
			}
			if (pRL->u.spezzone_alarm.ulNomRo)
				fDeltaRo=fDeltaRo2*fDeltaRo;
			else
				fDeltaRo=0;
			f_ohm_m=(((float)(pRL->u.spezzone_alarm.ulRo))/(TARGET_URP * MAGN_LEN * impToMm / 1000. /targetRes ));
			f_ohm_feet=f_ohm_m*feet2metri;
			sprintf(pDst,"%s badChunk %5.3fohm %5.3f%s %+3.1f%%",
					ucPrefixString,
					(pRL->u.newSpezzone.ulRo*(float)macParms.lenSemigiro*(float)targetRes)/((float)TARGET_URP*(float)MAGN_LEN),
					(nvram_struct.actUM==UM_IN)?f_ohm_feet:f_ohm_m,
					(nvram_struct.actUM==UM_IN)?"ohm/feet":"ohm/m",
					fDeltaRo
					);
			break;
	}
}



// finestra di visualizzazione log della produzione
unsigned char ucHW_LogProduzione(void){
	// parametro che al ritorno dal numeric keypad indica qual � il parametro che � stato modificato
	#define Win_CP_field 0
	#define Win_CP_doSearch 1



	#define defLP_Title_row (4)
	#define defLP_Title_col (0)

	#define defNumRigheLogDaVisualizzare 9

	#define defLP_LogDatas_row (32)
	#define defOffsetRow_LP 18
	#define defLP_LogDatas_col (0)

	#define defLP_Block_row (defLP_LogDatas_row+defOffsetRow_LP*defNumRigheLogDaVisualizzare+8)
	#define defLP_Block_col (0)

#if 0
	#define defLP_Codice_rowOk (defLcdWidthY_pixel-32-8)
	#define defLP_Codice_colOk (defLcdWidthX_pixel-32*2-8)
#endif


	// pulsanti della finestra
	typedef enum {
			enumLP_riga_1=0,
			enumLP_riga_2,
			enumLP_riga_3,
			enumLP_riga_4,
			enumLP_riga_5,
			enumLP_block,

			enumLP_Sx,
			enumLP_Up,
			enumLP_Cut,
			enumLP_Esc,
			enumLP_Dn,
			enumLP_Dx,

			enumLP_Title,
			enumLP_NumOfButtons
		}enumButtons_LogProduzione;


	xdata unsigned char i,j,ucActSpez;
	//xdata signed long lDeltaRo;
	TipoRecordLog xdata RL;
	xdata static unsigned char ucAutoScrollLogChunk,ucIdxFirstRow_LP;
	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			ucIdxFirstRow_LP=0;
			ucAutoScrollLogChunk=1;
			// ogni 100ms rinfresco la pagina...
            //ncon arm vado a 100ms
			ucLcdRefreshTimeoutEnable(100);

			// trasferisco i dati da nvram_struct.commesse a lista lavori
			vTrasferisciDaCommesseAlistaLavori();
			for (i=0;i<enumLP_NumOfButtons;i++)
				ucCreateTheButton(i);
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			// impostazione variabili locali della finestra...
			// defSetWinParam(Win_CP_doSearch,0);
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enumLP_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enumLP_Cut:
							/* Indico di eseguire un taglio asincrono. */
							vDoTaglio();
							// non occorre rinfrescare la finestra
							break;

						case enumLP_Title:
						case enumLP_Esc:
							ucLcdRefreshTimeoutEnable(0);
							vHandleEscFromMenu();
							// indico di rinfrescare la finestra
							return 2;
						// richiesto di inserire/modificare un lavoro in lista
						case enumLP_riga_1:
						case enumLP_riga_2:
						case enumLP_riga_3:
						case enumLP_riga_4:
						case enumLP_riga_5:
							break;
						case enumLP_Up:
							if (ucAutoScrollLogChunk){
								ucAutoScrollLogChunk=0;
								ucIdxFirstRow_LP=production_log.ucIdx;
							}
							if (ucIdxFirstRow_LP==0)
								ucIdxFirstRow_LP=sizeof(production_log.datas)/sizeof(production_log.datas[0])-1;
							else
								ucIdxFirstRow_LP--;

							break;
						case enumLP_Dn:
							if (ucAutoScrollLogChunk){
								ucAutoScrollLogChunk=0;
								ucIdxFirstRow_LP=production_log.ucIdx;
							}
							if (ucIdxFirstRow_LP<sizeof(production_log.datas)/sizeof(production_log.datas[0])-1)
								ucIdxFirstRow_LP++;
							else
								ucIdxFirstRow_LP=0;

							break;
						case enumLP_block:
							ucAutoScrollLogChunk=!ucAutoScrollLogChunk;
							if (!ucAutoScrollLogChunk)
								ucIdxFirstRow_LP=production_log.ucIdx;
							break;
						case enumLP_Dx:
						case enumLP_Sx:
							ucLcdRefreshTimeoutEnable(0);
							// ritorno alla finestra chiamante
							ucReturnFromWindow();
							return 2;
						default:
							break;
					}
				}
			}
			// PRESENTAZIONE DELLA FINESTRA...
			sprintf((char*)hw.ucString,"LOG: %6.2f ohm", (actRes*(float)targetRes)/TARGET_URP);
			ucPrintTitleButton((char*)hw.ucString,defLP_Title_row,defLP_Title_col,enumFontMedium,enumLP_Title,defLCD_Color_Yellow,1);

			if (ucAutoScrollLogChunk)
				sprintf((char*)hw.ucString," block");
			else
				sprintf((char*)hw.ucString," scroll");
			ucPrintStaticButton((char*)hw.ucString,defLP_Block_row,defLP_Block_col,enumFontSmall,enumLP_block,defLCD_Color_Trasparente);


			if (ucAutoScrollLogChunk)
				j=production_log.ucIdx;
			else
				j=ucIdxFirstRow_LP;
			for (i=0;i<defNumRigheLogDaVisualizzare;i++,j++){
				if (j>=sizeof(production_log.datas)/sizeof(production_log.datas[0]))
					j=0;
				memcpy(&RL,&production_log.datas[j],sizeof(RL));
				if (j==production_log.ucIdx)
					ucActSpez='*';
				else
					ucActSpez=' ';
				// chiamo la routine che prepara in hw.ucString la descrizione formattata dell'evento di log produzione
				// contenuto in RL
				vPrepareDesc_ProductionLog((char*)hw.ucString,&RL,ucActSpez,j);

				switch(RL.event_type){
					case let_numof:
					default:
					{
						break;
					}
					case let_newspezzone:
						ucPrintTitleButton((char*)hw.ucString,defLP_LogDatas_row+defOffsetRow_LP*i,defLP_LogDatas_col,enumFontSmall,enumLP_riga_1,defLCD_Color_Trasparente,6);
						break;
					case let_cut:
						ucPrintTitleButton((char*)hw.ucString,defLP_LogDatas_row+defOffsetRow_LP*i,defLP_LogDatas_col,enumFontSmall,enumLP_riga_1,defLCD_Color_Yellow,6);
						break;
					case let_bad_measure:
						ucPrintTitleButton((char*)hw.ucString,defLP_LogDatas_row+defOffsetRow_LP*i,defLP_LogDatas_col,enumFontSmall,enumLP_riga_1,defLCD_Color_Red,6);
						break;
					case let_spezzone_alarm:
						ucPrintTitleButton((char*)hw.ucString,defLP_LogDatas_row+defOffsetRow_LP*i,defLP_LogDatas_col,enumFontSmall,enumLP_riga_1,defLCD_Color_Red,6);
						break;
				}
				// add info about temperature compensation... if there is room and compensation is enabled
				if ((i<defNumRigheLogDaVisualizzare-1)&&uiPrepareDescTempComp_ProductionLog((char*)hw.ucString,&RL,sizeof(hw.ucString))){
					i++;
					ucPrintTitleButton((char*)hw.ucString,defLP_LogDatas_row+defOffsetRow_LP*i,defLP_LogDatas_col,enumFontSmall,enumLP_riga_1,defLCD_Color_Green,6);
				}
			}
			vAddStandardButtons(enumLP_Sx);

			//strcpy(hw.ucString,"Home");
			//ucPrintStaticButton(hw.ucString,defLL_Codice_rowEsc,defLL_Codice_colEsc,enumFontBig,enumLL_Esc,defLCD_Color_Trasparente);
			return 1;
	}
}//ucHW_LogProduzione

xdata TipoStructStringheLCD sLCD;

#if 0
	void vForceRefreshLcd(void){
		sLCD.ucForceARefresh=1;
	}
#endif

void vForceLcdRefresh(void){
	sLCD.ucNeedRefresh=1;
}


/*                                         */
/*                                         */
/* Funzioni di gestione editing.           */
/*                                         */
/*                                         */
// aggiorno stringhe lcd
#if 0
	void vUpdateStringsOnLcd(void){
	register unsigned char i;
		strncpy(&sLCD.ucString[0][0],&edit.actLcdStr[0][0],defMaxChrStringLcd);
		for (i=0;i<defMaxChrStringLcd;i++)
			if (sLCD.ucString[0][i]==0)
				break;
		for (;i<defMaxChrStringLcd;i++)
			 sLCD.ucString[0][i]=' ';
		sLCD.ucString[0][defMaxChrStringLcd]=0;
		strncpy(&sLCD.ucString[1][0],&edit.actLcdStr[1][0],defMaxChrStringLcd);
		for (i=0;i<defMaxChrStringLcd;i++)
			if (sLCD.ucString[1][i]==0)
				break;
		for (;i<defMaxChrStringLcd;i++)
			 sLCD.ucString[1][i]=' ';
		sLCD.ucString[1][defMaxChrStringLcd]=0;
		if (sLCD.ucCursorOn){
			if (edit.firstEditChar >= &edit.actLcdStr[1][0]){
				sLCD.ucCursorCol=edit.actEditChar - &edit.actLcdStr[1][0];
				sLCD.ucCursorRow=1;
			}
			else{
				sLCD.ucCursorCol=edit.actEditChar - &edit.actLcdStr[0][0];
				sLCD.ucCursorRow=0;
			}
		}
	}
#endif


typedef struct _TipoStructLcdRefreshTimeout{
	unsigned char ucEnable;
	unsigned int uiMilliseconds;
	unsigned char ucLastCnt;
	unsigned int uiCntPast;
	unsigned int uiActCnt;
}TipoStructLcdRefreshTimeout;
TipoStructLcdRefreshTimeout lcdRefreshTimeout;

unsigned char ucLcdRefreshTimeoutReset(void){
	memset(&lcdRefreshTimeout,0,sizeof(lcdRefreshTimeout));
	return 0;
}

unsigned char ucLcdRefreshTimeoutExpired(void){
	if (!lcdRefreshTimeout.ucEnable)
		return 0;
	lcdRefreshTimeout.uiActCnt=ucCntInterrupt;
	if (lcdRefreshTimeout.uiActCnt<lcdRefreshTimeout.ucLastCnt){
		lcdRefreshTimeout.uiActCnt+=256;
	}
	lcdRefreshTimeout.uiCntPast+=lcdRefreshTimeout.uiActCnt-lcdRefreshTimeout.ucLastCnt;
	if (lcdRefreshTimeout.uiCntPast*PeriodoIntMs>=lcdRefreshTimeout.uiMilliseconds){
		lcdRefreshTimeout.ucLastCnt=ucCntInterrupt;
		lcdRefreshTimeout.uiCntPast=0;
		return 1;
	}
	lcdRefreshTimeout.ucLastCnt=(lcdRefreshTimeout.uiActCnt)&0xFF;
	return 0;
}

unsigned char ucLcdRefreshTimeoutEnable(unsigned int uiMilliseconds){
	if (!uiMilliseconds)
		lcdRefreshTimeout.ucEnable=0;
	else{
		lcdRefreshTimeout.uiMilliseconds=uiMilliseconds;
		lcdRefreshTimeout.ucEnable=1;
		lcdRefreshTimeout.ucLastCnt=ucCntInterrupt;
		lcdRefreshTimeout.uiCntPast=0;
	}
	return 0;
}
unsigned int uiLcdRefreshTimeout_GetAct(void){
	if (!lcdRefreshTimeout.ucEnable)
		return 0;
	return lcdRefreshTimeout.uiMilliseconds;
}

#define defRow_1_Pixel  4
#define defRow_2_Pixel 24
#define defRow_3_Pixel 48
#define defRow_4_Pixel 68
#define defRow_Buttonstest_Pixel 68

// ritorna 1 se lcd deve essere rinfrescato
// (per ora se una delle stringhe sulle prime due righe � variata)
// o anche se misura ohmica variata!
unsigned char ucNeedToRefreshLcd(void){
//xdata static unsigned char ucContaRefresh;
		// salvo le stringhe che vado a stampare a video
	if (
#if 0
			strcmp(sLCD.ucSaveString[0],sLCD.ucString[0])
		||strcmp(sLCD.ucSaveString[1],sLCD.ucString[1])
		||(resStaMediata!=sLCD.fPrintRes)
		||((velMMin!=sLCD.fPrintVel)&&(++ucContaRefresh>=0x0F))
		||
#endif
		  (sLCD.ucNeedRefresh)
		||ucNeedToClosePopupWindow()
		||(sLCD.ucOldCursorRow!=sLCD.ucCursorRow)
		||(sLCD.ucOldCursorCol!=sLCD.ucCursorCol)
		||(ucLcdRefreshTimeoutExpired())
		){
		//ucContaRefresh=0;
		return 1;
	}
	return 0;
}//unsigned char ucNeedToRefreshLcd(void)

// stati possibili del button
typedef enum {
			enumBtnStatus_NeverInit=0,
			enumBtnStatus_Idle,
			enumBtnStatus_InitPressed,
			enumBtnStatus_Pressed,
			enumBtnStatus_Released,
			enumBtnStatus_NumOf
		}enumButtonStatus;

// struttura gestione buttons
typedef struct _TipoStructHandleButton{
	// stato del button
	enumButtonStatus stato;
	// timeout del button: serve per sapere quando passare di stato
	unsigned int uiTimeElapsedMs;
	// riferimento clock, per sapere quanti ms sono passati
	unsigned char ucPrecCntInterrupt;
	// flag per sapere se � stato premuto
	unsigned char ucFlagPressed;
	// indice del carattere del button che � stato premuto...
	unsigned char ucIdxCharPressed;
	// campo del button che � stato premuto...
	unsigned char ucFieldPressed;
}TipoStructHandleButton;

// struttura per la gestione buttons
xdata TipoStructHandleButton buttons[defMaxButtonsOnLcd];
// durata animazione button
#define defButtonAnimationTimeMs 100U

#if 0
	void vResetAllButtons(void){
		memset(&buttons[0],0,sizeof(buttons));
	}
#endif

unsigned char ucResetTheButton(unsigned char ucNumButton){
	if (ucNumButton>=defMaxButtonsOnLcd)
		return 0;
	memset(&buttons[ucNumButton],0,sizeof(buttons[0]));
	return 1;
}

#if 0
	unsigned char ucAckButton(unsigned char ucNumButton){
		if (ucNumButton>=defMaxButtonsOnLcd)
			return 0;
		buttons[ucNumButton].ucFlagPressed=0;
		return 1;
	}
#endif

unsigned char ucHasBeenPressedButton(unsigned char ucNumButton){
	if (ucNumButton>=defMaxButtonsOnLcd)
		return 0;
	if (buttons[ucNumButton].ucFlagPressed){
		buttons[ucNumButton].ucFlagPressed=0;
		return 1;
	}
	return 0;
}

unsigned char ucHasBeenTouchedButton(unsigned char ucNumButton){
	if (ucNumButton>=defMaxButtonsOnLcd)
		return 0;
	if (   (buttons[ucNumButton].stato==enumBtnStatus_InitPressed)
		 ||(buttons[ucNumButton].stato==enumBtnStatus_Pressed)
		 ||(buttons[ucNumButton].stato==enumBtnStatus_Released)
		 ){
		return 1;
	}
	return 0;
}


unsigned char ucCreateTheButton(unsigned char ucNumButton){
	if (!ucResetTheButton(ucNumButton))
		return 0;
	buttons[ucNumButton].stato=enumBtnStatus_Idle;
	return 1;
}

// indice del carattere del button che � stato premuto...
unsigned char ucIdxCharButtonPressed(unsigned char ucNumButton){
	return buttons[ucNumButton].ucIdxCharPressed;
}

// indice del field cui appartiene il carattere del button che � stato premuto...
unsigned char ucFieldButtonPressed(unsigned char ucNumButton){
	return buttons[ucNumButton].ucFieldPressed;
}

// impostazione dell' indice del field cui appartiene il carattere del button che � stato premuto...
void vSetFieldButtonPressed(unsigned char ucNumButton, unsigned char ucFieldPressed){
	buttons[ucNumButton].ucFieldPressed=ucFieldPressed;
}

#if 0
	unsigned char ucTheButtonIsCreated(unsigned char ucNumButton){
		if (ucNumButton>=defMaxButtonsOnLcd)
			return 1;
		if (buttons[ucNumButton].stato==enumBtnStatus_NeverInit)
			return 0;
		return 1;
	}
#endif

unsigned char ucTheButtonIsPressed(unsigned char ucNumButton){
	if (ucNumButton>=defMaxButtonsOnLcd)
		return 0;
	if (buttons[ucNumButton].stato==enumBtnStatus_Pressed)
		return 1;
	return 0;
}

unsigned short int button_touched;

unsigned short int us_is_button_pressed(unsigned char ucNumButton){
	if (button_touched==ucNumButton)
		return 1;
	return 0;
}

void vClearButtonPressed(void){
	button_touched=0xFFFF;
}



void vSignalButtonPressed(unsigned char ucNumButton,unsigned char ucNumCharPressed){
	if (ucNumButton>=defMaxButtonsOnLcd)
		return;
	button_touched=ucNumButton;
	switch(buttons[ucNumButton].stato){
		case enumBtnStatus_NeverInit:
		default:
			break;
		case enumBtnStatus_Idle:
			buttons[ucNumButton].stato=enumBtnStatus_InitPressed;
			buttons[ucNumButton].ucIdxCharPressed=ucNumCharPressed;
			break;
		case enumBtnStatus_Pressed:
		case enumBtnStatus_Released:
			break;
	}
}//void vSignalButtonPressed(unsigned char ucNumButton,unsigned char ucNumCharPressed)


// funzione di gestione dei pulsanti
void vHandleButtons(void){
	unsigned char xdata ucNumButton;
	for (ucNumButton=0;ucNumButton<defMaxButtonsOnLcd;ucNumButton++){
		switch(buttons[ucNumButton].stato){
			case enumBtnStatus_NeverInit:
			case enumBtnStatus_Idle:
			default:
				break;
			case enumBtnStatus_InitPressed:
				buttons[ucNumButton].stato=enumBtnStatus_Pressed;
				buttons[ucNumButton].uiTimeElapsedMs=0;
				// riferimento clock, per sapere quanti ms sono passati
				buttons[ucNumButton].ucPrecCntInterrupt=ucCntInterrupt;
				sLCD.ucNeedRefresh=1;
				break;
			case enumBtnStatus_Pressed:
			case enumBtnStatus_Released:
				if (ucCntInterrupt>=buttons[ucNumButton].ucPrecCntInterrupt)
					buttons[ucNumButton].uiTimeElapsedMs+=ucCntInterrupt-buttons[ucNumButton].ucPrecCntInterrupt;
				else{
					buttons[ucNumButton].uiTimeElapsedMs+=256;
					buttons[ucNumButton].uiTimeElapsedMs-=buttons[ucNumButton].ucPrecCntInterrupt-ucCntInterrupt;
				}
				if (buttons[ucNumButton].uiTimeElapsedMs*PeriodoIntMs>=defButtonAnimationTimeMs){
					sLCD.ucNeedRefresh=1;
					if (buttons[ucNumButton].stato==enumBtnStatus_Pressed){
						buttons[ucNumButton].stato=enumBtnStatus_Released;
						// indico che il button � stato pressed...
						buttons[ucNumButton].ucFlagPressed=1;
					}
					else{
						buttons[ucNumButton].stato=enumBtnStatus_Idle;
					}
					buttons[ucNumButton].uiTimeElapsedMs=0;
					// riferimento clock, per sapere quanti ms sono passati
					buttons[ucNumButton].ucPrecCntInterrupt=ucCntInterrupt;
				}
				// riferimento clock, per sapere quanti ms sono passati
				buttons[ucNumButton].ucPrecCntInterrupt=ucCntInterrupt;
				break;
		}
	}
}//void vHandleButtons(void)

//
// ---------------------------------------
//   fine gestione delle finestre...
// ---------------------------------------
//

void vRefreshLcdStrings(void){
extern unsigned char ucHandleWindows(void);
	// unsigned int uiAmplificazioneMisuraStatica;
	sLCD.ucForceARefresh=0;

	//sLCD.ucString[0][0]=sLCD.ucTest;
	if (sLCD.ucCursorOn){
		if (sLCD.ucToggle)
			ucPrintCursorText('*',sLCD.ucCursorRow*20+4, sLCD.ucCursorCol*16+1,(enumFontType)100);
		else{
			ucPrintCursorText(sLCD.ucString[sLCD.ucCursorRow][sLCD.ucCursorCol],sLCD.ucCursorRow*20+4, sLCD.ucCursorCol*16+1,(enumFontType)100);
		}
		if (++sLCD.ucToggle2>=5){
			sLCD.ucToggle2=0;
			sLCD.ucToggle=!sLCD.ucToggle;
		}
	}

	ucHandleWindows();

#if 0
	// salvo le stringhe che vado a stampare a video
	strcpy(sLCD.ucSaveString[0],sLCD.ucString[0]);
	strcpy(sLCD.ucSaveString[1],sLCD.ucString[1]);
	ucPrintStaticText(sLCD.ucString[0],defRow_1_Pixel,1);
	ucPrintStaticText(sLCD.ucString[1],defRow_2_Pixel,1);
// terza riga: velocit� in m/min
	sprintf(sLCD.ucString[2],"%6.3f m/min   ",(float)velMMin);
	for (i=0;i<defMaxChrStringLcd;i++)
		if (sLCD.ucString[2][i]==0)
			break;
	for (;i<defMaxChrStringLcd;i++)
		 sLCD.ucString[2][i]=' ';
	sLCD.ucString[2][defMaxChrStringLcd]=0;

	ucPrintStaticText(sLCD.ucString[2],defRow_3_Pixel,1);


// quarta riga: resistenza misurata
	if (   (spiralatrice.resStaMediata<0)||(spiralatrice.resStaMediata>25000)
		 ||(!(OutDig & ODG_MSTA)&&!sLCD.ucAttivataMisuraDinamica)
	   )
		sprintf(sLCD.ucString[3],"------ ohm      ");
	else{
		if (sLCD.ucAttivataMisuraDinamica){
			switch(actStaPortata){
				case 0:
				default:
					sprintf(sLCD.ucString[3],"%6.0f ohm [1x]   ",spiralatrice.resStaMediata);
					break;
				case 1:
					sprintf(sLCD.ucString[3],"%6.1f ohm [10x]  ",spiralatrice.resStaMediata);
					break;
				case 2:
					sprintf(sLCD.ucString[3],"%6.2f ohm [100x] ",spiralatrice.resStaMediata);
					break;
				case 3:
					sprintf(sLCD.ucString[3],"%6.3f ohm [1000x]",spiralatrice.resStaMediata);
					break;
			}
		}
		else switch(OutDig& 0x7){
			case 0:
			default:
				uiAmplificazioneMisuraStatica=1;
				sprintf(sLCD.ucString[3],"%6.0f ohm [1x]   ",spiralatrice.resStaMediata);
				break;
			case 4:
				uiAmplificazioneMisuraStatica=10;
				sprintf(sLCD.ucString[3],"%6.1f ohm [10x]  ",spiralatrice.resStaMediata);
				break;
			case 2:
				uiAmplificazioneMisuraStatica=100;
				sprintf(sLCD.ucString[3],"%6.2f ohm [100x] ",spiralatrice.resStaMediata);
				break;
			case 1:
				uiAmplificazioneMisuraStatica=1000;
				sprintf(sLCD.ucString[3],"%6.3f ohm [1000x]",spiralatrice.resStaMediata);
				break;
		}
	}
	for (i=0;i<defMaxChrStringLcd;i++)
		if (sLCD.ucString[3][i]==0)
			break;
	for (;i<defMaxChrStringLcd;i++)
		 sLCD.ucString[3][i]=' ';
	sLCD.ucString[3][defMaxChrStringLcd]=0;

	ucPrintStaticText(sLCD.ucString[3],defRow_4_Pixel,1);

#endif

	sLCD.fPrintRes=spiralatrice.resStaMediata;
	sLCD.fPrintVel=velMMin;
	sLCD.ucNeedRefresh=0;
	sLCD.ucOldCursorRow=sLCD.ucCursorRow;
	sLCD.ucOldCursorCol=sLCD.ucCursorCol;
	sLCD.ucForceARefresh=0;
	if (sLCD.ucForceARefresh)
		sLCD.ucNeedRefresh=1;
}
// posizionamento del testo alla linea, colonna indicata
void vSetTextRigaColonna(unsigned char ucRiga0_1,unsigned char ucColonna){
	//sLCD.ucNeedRefresh=1;
	sLCD.ucActRow=ucRiga0_1;
	if (ucColonna<sizeof(sLCD.ucString[0])-1)
		sLCD.ucActCol=ucColonna;
	sLCD.ucCursorRow=sLCD.ucActRow;
	sLCD.ucCursorCol=sLCD.ucActCol;
}
// cursore...
#if 0
	void vSetCursorOff(void){
		sLCD.ucCursorOn=0;
		sLCD.ucToggle=0;
		sLCD.ucToggle2=0;
		//sLCD.ucNeedRefresh=1;
		sLCD.ucOldCursorRow=0xFF;
		sLCD.ucOldCursorCol=0xFF;
	}
#endif
// cursore...
void vSetCursorOn(void){
	sLCD.ucCursorRow=sLCD.ucActRow;
	sLCD.ucCursorOn=1;
	//sLCD.ucNeedRefresh=1;
}

// stampa una stringa alla riga/colonna corrente
// es  " " o "*"
void vPrintDirectString(code char * pc){
	strncpy(&sLCD.ucString[sLCD.ucActRow&1][sLCD.ucActCol],pc,defMaxChrStringLcd);
	sLCD.ucString[sLCD.ucActRow&1][defMaxChrStringLcd]=0;
//	ucPrintStaticText(sLCD.ucString[sLCD.ucActRow&1],sLCD.ucActRow*16,sLCD.ucActCol*16);
}
void vPrintDirectChar(unsigned char uc){
	//sLCD.ucNeedRefresh=1;
	if (sLCD.ucActCol<defMaxChrStringLcd){
		sLCD.ucString[sLCD.ucActRow&1][sLCD.ucActCol]=uc;
		sLCD.ucCursorRow=sLCD.ucActRow&1;
		sLCD.ucCursorCol=sLCD.ucActCol;
		if (sLCD.ucCursorCol<19)
			sLCD.ucCursorCol++;
	}
//	ucPrintStaticText(sLCD.ucString[sLCD.ucActRow&1],sLCD.ucActRow*16,sLCD.ucActCol*16);
}

#if 0
	void vPrintTestChar(unsigned char uc){
		sLCD.ucTest=uc;
	}

	// clear lcd...
	void vClearLcd(void){
	register unsigned char i;
		sLCD.ucCursorRow=0;
		sLCD.ucCursorCol=0;
		sLCD.ucCursorOn=0;
		sLCD.ucActRow=0;
		sLCD.ucActCol=0;
		for (i=0;i<defMaxChrStringLcd;i++)
			 sLCD.ucString[0][i]=' ';
		sLCD.ucString[0][i]=0;
		for (i=0;i<defMaxChrStringLcd;i++)
			 sLCD.ucString[1][i]=' ';
		sLCD.ucString[1][i]=0;
	}
#endif

#if 0
void leggiDataOra(void){
}

void scriviDataOra(void){
}
#endif


void vTestAD7327(void);



#if 0
	void fillSpace(void){
	   xdat char *p = &edit.actLcdStr[1][0];

	   xdat unsigned char n;

	   n=LEN_LINES_LCD-1;

	   while (*p && n){
		  ++p;
		  n--;
	   }

	   while (n--)
		  *p++ = ' ';
	}
#endif


// static void doEdit(unsigned char key){
// funzione che aggiorna la stringa da rappresentare a video, a seconda del tasto key premuto...
#if 0
	void doEdit(unsigned char key){
		// Se oltrepasso i limiti dell' edit, non devo permettere la scrittura del carattere.
		if (  (edit.actEditChar >= edit.firstEditChar + edit.lenEdit)
			&&(key !=KED_LF)
			)
			return;
		// edit di un tipo enumerativo, niente da fare...
		if (edit.typEdit == EDIT_ENUM)
			return;

		// se tasto premuto � numerico...
		if (key >= KED_0 && key <= KED_9) {
			// se tipo=password, assegno tasto alla password
			if (edit.typEdit == EDIT_PASSW){
				//newpassw[(unsigned char)(edit.actEditChar - edit.firstEditChar)] = key+'0';
				}
			// altrimenti assegno tasto
			else
				*edit.actEditChar = '0'+key;
			// se sto editando stringa con picture...
			if(  (edit.typEdit == EDIT_PICTURE)
				&&(edit.actStrReverse[(unsigned char)(edit.actEditChar - edit.firstEditChar)]=='1')
			   )
				*edit.actEditChar=ReverseChar(*edit.actEditChar);
			// avanzo col cursore
			edit.actEditChar++;
			// aggiorno stringhe lcd
			vUpdateStringsOnLcd();
			// se edit con picture...
			if(  (edit.typEdit == EDIT_PICTURE)
				&&(edit.actStrPicture[edit.actEditChar - edit.firstEditChar]!='a')
				&&(edit.actStrPicture[edit.actEditChar - edit.firstEditChar]!='n')
				&&(edit.actStrPicture[edit.actEditChar - edit.firstEditChar]!='\0')
				){
				*edit.actEditChar=edit.actStrPicture[edit.actEditChar - edit.firstEditChar];
				edit.actEditChar++;
				// aggiorno stringhe lcd
				vUpdateStringsOnLcd();
			}
		}
		// se invece tasto � up/dn/./clr
		if (key == KED_UP || key == KED_DN || key == KED_POINT || key == KED_CLR) {
			// se sto editando un numero, accetto solo il punto
			if (   (edit.typEdit == EDIT_NUMBER)
				 ||(  (edit.typEdit == EDIT_PICTURE)
					&&(edit.actStrPicture[(unsigned char)(edit.actEditChar - edit.firstEditChar)]!='a')
					&&(edit.actStrPicture[(unsigned char)(edit.actEditChar - edit.firstEditChar)]!='n')
					&&(edit.actStrPicture[edit.actEditChar - edit.firstEditChar]!='\0')
					)
				){
				*edit.actEditChar = '.';
			}
			else if (edit.typEdit != EDIT_PASSW) {
				char s = *edit.actEditChar;
				// Nell' editing di lettere, ammetto solo numeri, lettere e carattere spazio.
				// I valori vengono proposti in modo circolare.
				if (key == KED_UP){
					// cerco il prossimo carattere alfanumerico o blank
					do{
						s++;
						s&=0x7F;
					}while (!(isalnum(s)||(s==' ')));
				}
				else if (key == KED_DN){
					do{
						s--;
						s&=0x7F;
					}while (!(isalnum(s)||(s==' ')));
				}
				else if (key == KED_POINT)
					s = '.';
				else if (key == KED_CLR)
					s = ' ';
				if (isascii(s))
					*edit.actEditChar = s;
				key = KED_LF;
			}
			if(  (edit.typEdit == EDIT_PICTURE)
				&&(edit.actStrReverse[(unsigned char)(edit.actEditChar - edit.firstEditChar)]=='1')
				)
				*edit.actEditChar=ReverseChar(*edit.actEditChar);
			edit.actEditChar++;
			// aggiorno stringhe lcd
			vUpdateStringsOnLcd();
			if(  (edit.typEdit == EDIT_PICTURE)
				&&(edit.actStrPicture[(unsigned char)(edit.actEditChar - edit.firstEditChar)]!='a')
				&&(edit.actStrPicture[(unsigned char)(edit.actEditChar - edit.firstEditChar)]!='n')
				){
				*edit.actEditChar=edit.actStrPicture[edit.actEditChar - edit.firstEditChar];
				// avanzo di due spazi
				edit.actEditChar+=2;
				// aggiorno stringhe lcd
				vUpdateStringsOnLcd();
			}
		}

		if (  (key==KED_LF)&&(edit.actEditChar>edit.firstEditChar)) {
			--edit.actEditChar;
			if(   (edit.typEdit == EDIT_PICTURE)
				&&(edit.actStrPicture[(unsigned char)(edit.actEditChar - edit.firstEditChar)]!='a')
				&&(edit.actStrPicture[(unsigned char)(edit.actEditChar - edit.firstEditChar)]!='n')
			  ){
				if (edit.actEditChar>edit.firstEditChar)
					--edit.actEditChar;
				else
					++edit.actEditChar;
			}
			// mi devo posizionare sulla seconda linea, colonna tot
			vSetTextRigaColonna(1,edit.actColLcdVal + (edit.actEditChar - edit.firstEditChar));
		}
		else if ( (key==KED_RG)&& (edit.actEditChar + 1 <= edit.firstEditChar + edit.lenEdit)){
			++edit.actEditChar;
			if(  (edit.typEdit == EDIT_PICTURE)
				&&(edit.actStrPicture[(unsigned char)(edit.actEditChar - edit.firstEditChar)]!='a')
				&&(edit.actStrPicture[(unsigned char)(edit.actEditChar - edit.firstEditChar)]!='n')
				){
				if (edit.actEditChar + 1 <= edit.firstEditChar + edit.lenEdit)
					++edit.actEditChar;
				else
					--edit.actEditChar;
			}
			// mi devo posizionare sulla seconda linea, colonna tot
			vSetTextRigaColonna(1,edit.actColLcdVal + (edit.actEditChar - edit.firstEditChar));
	   }
	}//static void doEdit(unsigned char key){
#endif

#if 0
	// routine che serve ad inizializzare puntatori per la stampa di un valore sulla seconda riga del display lcd
	void prtVal(void){
	   edit.actLcdChar = edit.firstEditChar;
	   edit.actRowLcd = 1;
	   edit.actColLcd = edit.actColLcdVal;
	}
#endif

#if 0
	void stopEdit(void){
		edit.actEditChar = 0;
		if (edit.typEdit == EDIT_ENUM) {
			// mi devo posizionare sulla seconda linea, colonna tot
			vSetTextRigaColonna(1,edit.actColLcdVal);
			// stampo un carattere blank???
			vPrintDirectString(" ");
		}
		else {
			// display on, cursor off, blink off
			vSetCursorOff();
		}
		prtVal();
	}//static void stopEdit(void)
#endif


void startEdit(void){
#ifdef okEditServe
	edit.okEdit = 0;
#endif
	edit.firstEditChar = edit.actEditChar = &edit.actLcdStr[1][edit.actColLcdVal];
	// mi devo posizionare sulla seconda linea, colonna tot
	vSetTextRigaColonna(1,edit.actColLcdVal);
	if(  (edit.typEdit == EDIT_PICTURE)
	  &&(edit.actStrPicture[(unsigned char)(edit.actEditChar - edit.firstEditChar)]!='a')
	  &&(edit.actStrPicture[(unsigned char)(edit.actEditChar - edit.firstEditChar)]!='n')
	 ){
		++edit.actEditChar;
	}
	if (edit.typEdit == EDIT_ENUM) {
		vSetTextRigaColonna(1,edit.actColLcdVal);
		vPrintDirectString((char*)"*");
	}
	else {
		vSetTextRigaColonna(1,edit.actColLcdVal+edit.actEditChar-edit.firstEditChar);
		// display on, cursor on, blink off
		vSetCursorOn();
	}
}//static void startEdit(void)


#if 0
	void startPrintLcd(	unsigned char flags,
								unsigned char line1Str, unsigned char line2Str,
								unsigned char index1,   unsigned char index2,
								unsigned char subIndex1
							  ){
		xdat unsigned char ch;
		rom char * xdat src;
		xdat char * xdat dst;

		if (flags & FLG_PRINT_CLEAR) {
			vClearLcd();
			edit.actLcdStr[0][0] = edit.actLcdStr[1][0] = 0;
		}

		if (index1) {
			if (subIndex1)
				sprintf(edit.actLcdStr[0], "%d.%d ", index1, subIndex1);
			else
				sprintf(edit.actLcdStr[0], "%d ", index1);
		}

		if (index2)
			sprintf(edit.actLcdStr[1], "%d ", index2);

		dst = edit.actLcdStr[0];
		for (ch = 0; ch < LEN_LINES_LCD && *dst; ch++)
			++dst;
		for (src = msgNow[line1Str]; ch < LEN_LINES_LCD && *src; ch++)
			*dst++ = *src++;
		for (; ch < LEN_LINES_LCD; ch++)
			*dst++ = ' ';

		*dst='\0';

		dst = edit.actLcdStr[1];
		ch = 0;

		if (line2Str) {
			for (; ch < LEN_LINES_LCD && *dst; ch++)
				++dst;
			for (src = msgNow[line2Str]; ch < LEN_LINES_LCD && *src; ch++) {
				*dst++ = *src++;
			}
			*dst='\0';
		}
		if (flags & FLG_PRINT_ENUM) {
			*dst++ = ' ';
			edit.actColLcdVal = ++ch;
		}
		// La scrittura pu� cominciare dal primo carattere della seconda linea.
		edit.actColLcdVal = ch;

		for (*dst++ = 0, ++ch; ch < LEN_LINES_LCD; ch++)
			*dst++ = ' ';

		*dst='\0';

		edit.actRowLcd = edit.actColLcd = 0;
		edit.actLcdChar = edit.actLcdStr[0];
	}//static void startPrintLcd(	unsigned char flags, ...
#endif


#if 0
	/* Routine che permette di inserire una stringa allineata a destra
	   nella linea line dell' lcd. */
	void AtRightLine(unsigned char line2str, unsigned char line){
		unsigned char xdat n;
		rom char * xdat src;
		xdat char * xdat dst;

		n = strlen(edit.actLcdStr[line-1]);
		dst=&edit.actLcdStr[line-1][n];
		while (n++<LEN_LINES_LCD)
			*dst++=' ';

		src=msgNow[line2str];
		n=0;
		while (*src++)
			n++;
		src=msgNow[line2str];

		dst=&edit.actLcdStr[line-1][LEN_LINES_LCD-n];

		while (n--)
			*dst++ = *src++;
	}//static void AtRightLine(unsigned char line2str, unsigned char line)
#endif

#if 0
	// Routine che permette di inserire un numero allineato a destra
	//   nella linea line dell' lcd.
	//   Il numero deve essere compreso fra 0 e 99.
	void AtRightLineNumber(unsigned char number, unsigned char line){
		unsigned char xdat n;
		xdat char * xdat dst;

		n = strlen(edit.actLcdStr[line-1]);
		dst=&edit.actLcdStr[line-1][n];
		while (n++<LEN_LINES_LCD)
			*dst++=' ';

		if (number>9){
			edit.actLcdStr[line-1][LEN_LINES_LCD-2]=number/10+'0';
			edit.actLcdStr[line-1][LEN_LINES_LCD-1]=number%10+'0';
		}
		else
			edit.actLcdStr[line-1][LEN_LINES_LCD-1]=number+'0';
	}//static void AtRightLineNumber(unsigned char number, unsigned char line)
#endif

#if 0
	/* Copia di una stringa rom sul display. */
	void RomStringAt(unsigned char enum_str,unsigned char line,unsigned char col){
		rom char * xdat src;
		xdat char * xdat dst;

		src=msgNow[enum_str];

		dst=&edit.actLcdStr[line-1][col];

		while (*src)
			*dst++ = *src++;
	}//static void RomStringAt(unsigned char enum_str,unsigned char line,unsigned char col)
#endif
#if 0
	// Funzione che paragona due password e verifica che siano uguali.
	//   Ritorna 1 se sono uguali, 0 altrimenti.
	bit cmpPassw(unsigned char xdat *pnewpassw,unsigned char xdat *ppassword){
		// Finch� le stringhe sono uguali, e non arrivo alla fine, continuo.
		while((*pnewpassw==*ppassword)&&(*pnewpassw)){
			pnewpassw++;
			ppassword++;
		}
		// Le stringhe sono uguali se sono arrivato al tappo di entrambe.
		return ((!*ppassword)&&(!*pnewpassw));
	}//static bit cmpPassw(char xdat *newpassw,char xdat *password)
#endif
#if 0
	// Posiziona un simbolo che indica a quale campo ci si riferisce.
	void SegnaCampo(unsigned char col,unsigned char line,unsigned char Dx0Sx1){
		backCampoChar=edit.actLcdStr[line][col];
		backCampoCol=col;
		if (Dx0Sx1==0)
			edit.actLcdStr[line-1][col]='>';
		else
			edit.actLcdStr[line-1][col]='<';
	}//static void SegnaCampo(unsigned char col,unsigned char line,unsigned char Dx0Sx1)
#endif




/* Routine che inserisce un programma vuoto in memoria, dato il codice
   puntato da pFirstChar. */
void InsertTheProgram(char xdat* pFirstChar){
	char xdat *xdat s;
	unsigned int xdat i;
	s=pFirstChar;
	spiralatrice.pPrg=hprg.ptheAct;
	for (i=0;*s;i++)
		spiralatrice.pPrg->codicePrg[i]=*s++;
	spiralatrice.pPrg->codicePrg[i]=0x00;
	if (nvram_struct.actUM==UM_IN){
		spiralatrice.pPrg->diametro_filo=mynatof(pFirstChar+COL_FPRG_DIAM_FILO_INCH,LEN_FPRG_DIAM_FILO_INCH)*INCH_TO_MM;
		spiralatrice.pPrg->diametro_mandrino=mynatof(pFirstChar+COL_FPRG_DIAM_MAND_INCH,LEN_FPRG_DIAM_MAND_INCH)*INCH_TO_MM;
		spiralatrice.pPrg->num_fili=mynatoi(pFirstChar+COL_FPRG_NFILI_INCH,LEN_FPRG_NFILI_INCH);
		spiralatrice.pPrg->fornitore=mynatoi(pFirstChar+COL_FPRG_FORNI_INCH,LEN_FPRG_FORNI_INCH);
	}
	else{
		spiralatrice.pPrg->diametro_filo=mynatof(pFirstChar+COL_FPRG_DIAM_FILO_MM,LEN_FPRG_DIAM_FILO_MM);
		spiralatrice.pPrg->diametro_mandrino=mynatof(pFirstChar+COL_FPRG_DIAM_MAND_MM,LEN_FPRG_DIAM_MAND_MM);
		spiralatrice.pPrg->num_fili=mynatoi(pFirstChar+COL_FPRG_NFILI_MM,LEN_FPRG_NFILI_MM);
		spiralatrice.pPrg->fornitore=mynatoi(pFirstChar+COL_FPRG_FORNI_MM,LEN_FPRG_FORNI_MM);
	}
	if (spiralatrice.pPrg->num_fili>macParms.ucMaxNumFili)
		spiralatrice.pPrg->num_fili=1;
	else if (spiralatrice.pPrg->num_fili<1)
		spiralatrice.pPrg->num_fili=1;
	spiralatrice.pPrg->rpm_mandrino=RPM_MAND_PRG_EMPTY;
	spiralatrice.pPrg->empty=1;
	spiralatrice.pPrg->vel_produzione=5.0;
	spiralatrice.pPrg->vel_prod_vera=5.0;
	AdjustVelPeriferica(0);
	spiralatrice.pPrg->assorb[0]=0;
	spiralatrice.pPrg->assorb[1]=0;
	/* Posiziono il delta a 0, vale a dire a met� della corsa del potenziometro. In questo modo l' utente,
		per modificare il valore, deve partire dal centro della scala.
	*/

	// versione 1.32
	// posiziono velocit� W1 a -5%
	// posiziono velocit� W2 a -10%
	spiralatrice.pPrg->vruote_prg[0]=0.95*FS_ADC/2;
	spiralatrice.pPrg->vruote_prg[1]=0.90*FS_ADC/2;

	spiralatrice.pPrg->pos_pretaglio=0; /* Posizione di pretaglio. */
	spiralatrice.pPrg->coeff_corr=1.0;
	/* Azzero le percentuali di distanziazione */
	for (i=0;i<defMaxNumOfPercDistanzia;i++){
		spiralatrice.pPrg->usPercDistanzia_Starts[i]=0;
		spiralatrice.pPrg->usPercDistanzia_Ends[i]=0;
	}
	hDist.actPercDist=0;
	// inizializzo il ritardo del distanziatore gi�
	spiralatrice.pPrg->usDelayDistanziaDownMs=macParms.usDelayDistanziaDownMs;
	spiralatrice.pPrg->f_coltello_pos_spaziatura_mm=0; // Posizione del coltello per la spaziatura. 
	spiralatrice.pPrg->f_coltello_offset_n_spire=DEFAULT_N_SPIRE_COMPRESSIONE; // NUMERO DI DEFAULT DI SPIRE DI COMPRESSIONE
	// se reference temperature to default value=0�C
	spiralatrice.pPrg->f_temperature_compensation_t_celsius[0]=0;
	spiralatrice.pPrg->f_temperature_compensation_t_celsius[1]=0;
	// factor of change of resistance at temperature set, eg 1.33
	// 1.0--> no variation in resistance 
	// 0.0--> calibration point not used (default)
	// these values ar defined as R(@some t)/R(@20�C)
	spiralatrice.pPrg->f_temperature_compensation_coeff[0]=0.0;
	spiralatrice.pPrg->f_temperature_compensation_coeff[1]=0.0;
	// alfa factor, used in formula R=R0(@20�C)*(1+alfa*(t-20�C))
	// 0 means no resistance changes with temperature
	spiralatrice.pPrg->ui_temperature_coefficient_per_million=0;
	// another corrective coefficient until now not used
	spiralatrice.pPrg->i_temperature_corrective_factor_percentual=0;

	// imposto chiave in modo che indichi che il programma � valido...
	spiralatrice.pPrg->usValidProgram_A536=0xA536;

	ucSaveActPrg();

}//void InsertTheProgram(char xdat* pFirstChar)


xdata unsigned char ucNumErrNandFlash;

/* Routine che inizializza il valore di uiGetActPrg() in funzione di quello
   di actCommessa.
*/
void vInitActPrg(void){
	ucNumErrNandFlash=0;
   if (spiralatrice.firstComm)
		uiSetActPrg(nvram_struct.commesse[spiralatrice.actCommessa].numprogramma);
   else if (hprg.firstPrg)
		uiSetActPrg(hprg.firstPrg-1);
   else
   	uiSetActPrg(0);
}



void vHandleBackgroundNandFlash(void){
}


void vValidateTabFornitori(void){
	xdata unsigned char i;
	for (i=0;i<sizeof(nvram_struct.TabFornitori)/sizeof(nvram_struct.TabFornitori[0]);i++)
	   if ((nvram_struct.TabFornitori[i]<MIN_VAR_RES) || (nvram_struct.TabFornitori[i]>MAX_VAR_RES))
			nvram_struct.TabFornitori[i]=MAX_VAR_RES;
}


void vSearchFirstComm(void){
	if (!spiralatrice.firstComm)
		return;
	
	if (spiralatrice.firstComm>sizeof(nvram_struct.commesse)/sizeof(nvram_struct.commesse[0])){
		spiralatrice.firstComm=0;
		return;
	}
	if (nvram_struct.commesse[spiralatrice.firstComm-1].uiValidComm_0xa371!=0xa371){
		spiralatrice.firstComm=0;
		return;
	}
}
/* Conversione ascii-integer di n caratteri. */
unsigned short mynatoi(char* str, unsigned char n){
	xdat unsigned short res;

	for (res = 0; *str >= '0' && *str <= '9' &&n--; str++)
		res = 10 * res + *str - '0';

	return res;
}


/* Routine che aggiusta la velocit� di produzione e
   delle frizioni 1 e 2.
   Deve esssere chiamata quando vengono variati i parametri:
   - velocit� di produzione ;
   - diametro del mandrino;
   - diametro del filo;
   Ingresso: 1 se variata velocit� in metri/minuto, 0 altrimenti.
	     2 se variato rpm e si deve usare l' indicazione del
	     potenziometro per calcolare la velocita' delle ruote.
*/
void AdjustVelPeriferica(unsigned char vmmChanged){
	/* Puntatore alla struttura programma per accedere comodamente ai dati. */
	PROGRAMMA * xdat pPro;
	unsigned char xdat n, ucRedoCalcs,ucCntRedoCalc;
	xdat float f_aux;
	xdat float f_aux2;


	/* Punto all' entry dell' array dei programmi che contiene il programma
	in visualizzazione. */
	pPro=hprg.ptheAct;
	/* Per il calcolo della velocit�, assumo come diametro di rotazione
	quello del mandrino+quello del filo. */
	if (pPro->diametro_filo+pPro->diametro_mandrino>0.001){
		if (!vmmChanged)
			pPro->vel_produzione=pPro->rpm_mandrino*(PI_GRECO*(pPro->diametro_filo+pPro->diametro_mandrino))/1000.;
		else
			pPro->rpm_mandrino=pPro->vel_produzione*1000./(PI_GRECO*(pPro->diametro_filo+pPro->diametro_mandrino));
	}


	// inserisco un controllo per limitare la velocit� del mandrino ed evitare che le frizioni vadano in saturazione
	// se la velocit� di lavoro delle frzioni raggiunge il valore max (-tolleranza) limito a valore max (-2*tolleranza)
	// in modo da evitare che la macchina lavori "male", con le frizioni troppo lente rispetto al mandrino

	// ripeto max 2 volte il loop di ricalcolo della velocit� mandrino per non saturare le frizioni
	for (ucCntRedoCalc=0;ucCntRedoCalc<2;ucCntRedoCalc++){
		ucRedoCalcs=0;
		/* La velocit� in giri/minuto delle frizioni si ottiene:
			vel_angolare_mandrino=vel_produzione/raggio_mandrino
			(il motore deve ruotare con la velocit� angolare del bordo del filo
			 su cui fa attrito, per cui nel calcolo della sua velocit� angolare
			 non credo si debba tener conto dello spessore del filo...)
			vel_mandrino_e_filo=vel_angolare_mandrino*(raggio_mandrino+raggio_filo)
			vel_frizione=vel_mandrino_e_filo+x%
			vel_angolare_frizione=vel_frizione/raggio_frizione
			vel_angolare_nominale=vel_frizione/raggio_frizione
		*/
		/* Se sto partendo da fermo, le frizioni devono girare sulla
		velocita' del mandrino, che e' quella bassa di default.
		*/
		if(spiralatrice.slowRpmMan!=noSlowRpmMan){
			f_aux=RPM_MAND_PRG_EMPTY*(PI_GRECO*(pPro->diametro_filo+pPro->diametro_mandrino))/1000.;
		}
		else{
			if (vmmChanged==2){
				f_aux=nvram_struct.commesse[spiralatrice.actCommessa].rpm*(PI_GRECO*(pPro->diametro_filo+pPro->diametro_mandrino))/1000.;
			}
			else{
				f_aux=pPro->vel_produzione;
			}
		}
		
		for (n=0;n<NUM_FRIZIONI;n++){
			if (!macParms.diam_frizioni[n])
				continue;
			f_aux2=macParms.diam_frizioni[n];
			f_aux2=f_aux/f_aux2*(1.0/PI_GRECO);
			pPro->vel_frizioni[n]=(1000.*(DEFAULT_DELTA_RPM_FRIZ/100.0))*f_aux2;
// Michele: escludo questo controllo perch� non funziona
#if 0
			// se una delle frizioni dovrebbe ruotare a velocit� troppo alta, devo limitare i giri mandrino
			if (pPro->vel_frizioni[n]>macParms.rpm_frizioni[n]-defMarginRpmFrizioni){
				// indico che il loop deve essere ripetuto
				ucRedoCalcs=1;
				// imposto il valore finale in modo da evitare instabilit�
				f_aux2=(macParms.rpm_frizioni[n]-defMarginRpmFrizioni-1);
				// trovo il rapporto di cui deve essere diminuita la velocit�
				f_aux2/=pPro->vel_frizioni[n];
				// imposto la nuova velocit� della frizione
				pPro->vel_frizioni[n]=macParms.rpm_frizioni[n]-defMarginRpmFrizioni-1;
				if(spiralatrice.slowRpmMan!=noSlowRpmMan){
				}
				else{
					// adesso reimposto le velocit� della commessa/programma
					if (vmmChanged==2){
						// abbasso la velocit� di lavoro della commessa
						nvram_struct.commesse[spiralatrice.actCommessa].rpm*=f_aux2;
					}
					else{
						// abbasso velocit� di produzione e velocit� mandrino...
						pPro->vel_produzione*=f_aux2;
						pPro->rpm_mandrino*=f_aux2;
					}
				}//if(spiralatrice.slowRpmMan!=noSlowRpmMan) ... else
			}//if (pPro->vel_frizioni[n]>macParms.rpm_frizioni[n]-defMarginRpmFrizioni)
#endif
		}//for (n=0;n<NUM_FRIZIONI;n++)
		// se nessuna delle frizioni � stata saturata (c�pita nella maggior parte dei casi), esco dal loop
		if (!ucRedoCalcs)
			break;
	}// while(1)
}//void AdjustVelPeriferica(unsigned char vmmChanged)


/* Funzione che, se chiamata, permette di aggiornare la posizione
   di pretaglio e di riposo della lama di taglio. */
void vAggiornaPosizioneColtello(void){
  /* Calcolo la somma dei raggi del coltello, del filo e del mandrino
     meno l' interasse mandrino-coltello; questo valore mi e' utile
     per tutti i calcoli. */
  spiralatrice.SommaRaggiMenoInterasse=(hprg.ptheAct->diametro_filo+
			   hprg.ptheAct->diametro_mandrino+
			   macParms.diam_coltello
			  )/2.
			  -macParms.iasse[0];

  if (spiralatrice.SommaRaggiMenoInterasse>hprg.ptheAct->pos_pretaglio){
     spiralatrice.actPosDacColtello=spiralatrice.SommaRaggiMenoInterasse-hprg.ptheAct->pos_pretaglio;
  }
  else
     spiralatrice.actPosDacColtello=PercPosInizialeColtelloPre*spiralatrice.CorsaColtello;

  sensoreTaglio.uiPosPretaglio=spiralatrice.actPosDacColtello/spiralatrice.CorsaColtello*(sensoreTaglio.uiMax_cur-sensoreTaglio.uiMin_cur)+macParms.uiMinPosColtello_StepAD;
  

  /* Invio il comando di posizionamento del coltello. */
  spiralatrice.DacOut[addrDacColtello*2]=PercPosInizialeColtello*FS_DAC_MAX519;
  spiralatrice.DacOut[addrDacColtello*2+1]=spiralatrice.actPosDacColtello*FS_DAC_MAX519/spiralatrice.CorsaColtello;
  SendByteMax519(addrDacColtello,
	         spiralatrice.DacOut[addrDacColtello*2],
	         spiralatrice.DacOut[addrDacColtello*2+1]);
}
/* Funzione che regola le uscite dei dac in base ai valori letti dai potenziometri. Viene usata in diagnostica
   per pilotare mandrino e ruote di contrasto. */
void WriteOnDacByPot(void){
   xdat unsigned char i;
   /* Calcolo in spiralatrice.DacOutValue[0] la velocit� angolare del motore. */
   spiralatrice.DacOutValue[0]=(long)InputAnalog[N_CANALE_V_MAN]*macParms.rpm_mandrino/FS_ADC;
   /* Fisso il valore massimo degli rpm del mandrino. */
   spiralatrice.DacOutMaxValue[0]=macParms.rpm_mandrino;
   /* La coppia del mandrino non va controllata. */
   spiralatrice.DacOutValue[1]=1;
   spiralatrice.DacOutMaxValue[1]=1;

   SetDacValue(addrDacMandrino);
   for (i=0;i<NUM_FRIZIONI;i++){
      /* Calcolo in spiralatrice.DacOutValue[0] la velocit� angolare della ruota i. */
      spiralatrice.DacOutValue[0]=(long)InputAnalog[N_CANALE_V1+i]*macParms.rpm_frizioni[i]/FS_ADC;
      /* Fisso il valore massimo degli rpm della ruota. */
      spiralatrice.DacOutMaxValue[0]=macParms.rpm_frizioni[i];
      /* La coppia della ruota non va controllata. */
      spiralatrice.DacOutValue[1]=1;
      spiralatrice.DacOutMaxValue[1]=1;

      SetDacValue(addrDacFrizione1+i);
   }
}//static void WriteOnDacByPot(void)


/* Se entro nel menu dei parametri di impostazione, determino
    come devono essere mossi i potenziometri. */
void SetPotenziometri(void){
unsigned char xdat i;
    spiralatrice.pComm=&nvram_struct.commesse[spiralatrice.actCommessa];
	// se devo gestire la regolazione velocit� tramite encoder...
	if (macParms.uc_1Encoder_0Potenziometri==1){
		// devo: indirizzare verso mandrino la regolazione
		// impostare vruote_comm[0/1] al valore adatto per poter essere "preso" subito
		// impostare spiralatrice.pComm->rpm al valore adatto per poter essere "preso" subito
		if (spiralatrice.slowRpmMan==noSlowRpmMan)
			vSetHRE_potentiometer(enumSpeedRegulation_Spindle,hprg.ptheAct->rpm_mandrino);
		else
			vSetHRE_potentiometer(enumSpeedRegulation_Spindle,RPM_MAND_PRG_EMPTY);
		vSetHRE_potentiometer(enumSpeedRegulation_Wheel1,hprg.ptheAct->vruote_prg[0]);
		vSetHRE_potentiometer(enumSpeedRegulation_Wheel2,hprg.ptheAct->vruote_prg[1]);
		vSelectHRE_potentiometer(enumSpeedRegulation_Spindle);
		spiralatrice.pComm->vruote_comm[0]=iGetHRE_potentiometer(enumSpeedRegulation_Wheel1);
		spiralatrice.pComm->vruote_comm[1]=iGetHRE_potentiometer(enumSpeedRegulation_Wheel2);
		spiralatrice.pComm->rpm=(long)(iGetHRE_potentiometer(enumSpeedRegulation_Spindle))*macParms.rpm_mandrino/FS_ADC;
	}
	// gestione regolazione velocit� con potenziometri
	else{
		spiralatrice.pComm->vruote_comm[0]=(unsigned int)InputAnalog[N_CANALE_V1];
		spiralatrice.pComm->vruote_comm[1]=(unsigned int)InputAnalog[N_CANALE_V2];
		spiralatrice.pComm->rpm=(long)InputAnalog[N_CANALE_V_MAN]*macParms.rpm_mandrino/FS_ADC;
	}
    if (spiralatrice.slowRpmMan==noSlowRpmMan){
        if (spiralatrice.pComm->rpm*IsteresiUpPot<hprg.ptheAct->rpm_mandrino)
			spiralatrice.CheckPotMan=CheckPotMaggioreUguale;
        else if (spiralatrice.pComm->rpm*IsteresiDnPot>hprg.ptheAct->rpm_mandrino)
			spiralatrice.CheckPotMan=CheckPotMinoreUguale;
        else
			spiralatrice.CheckPotMan=0;
    }
    else{
        if (spiralatrice.pComm->rpm*IsteresiUpPot<RPM_MAND_PRG_EMPTY)
			spiralatrice.CheckPotMan=CheckPotMaggioreUguale;
        else if (spiralatrice.pComm->rpm*IsteresiDnPot>RPM_MAND_PRG_EMPTY)
			spiralatrice.CheckPotMan=CheckPotMinoreUguale;
        else
			spiralatrice.CheckPotMan=0;
    }

    for (i=0;i<NUM_FRIZIONI;i++){
		if (spiralatrice.pComm->vruote_comm[i]*IsteresiUpPot<hprg.ptheAct->vruote_prg[i])
			spiralatrice.CheckPotVel[i]=CheckPotMaggioreUguale;
		else if (spiralatrice.pComm->vruote_comm[i]*IsteresiDnPot>hprg.ptheAct->vruote_prg[i])
			spiralatrice.CheckPotVel[i]=CheckPotMinoreUguale;
		else
			spiralatrice.CheckPotVel[i]=0;
    }
}


void vMacParmsAtDefaultValue(void){
	unsigned int i;
    memset(&macParms,0,sizeof(macParms));

    macParms.uiMacParmsFormatVersion=defMacParmsFormatVersion;


	macParms.impGiro=defImpGiroDefault;
	macParms.mmGiro=defMmGiroDefault;
	macParms.rpm_frizioni[0]=defRpmFrizioniDefault;
	macParms.rpm_frizioni[1]=defRpmFrizioniDefault; /* Max rpm frizioni. */
	macParms.rpm_mandrino=defRpmMandrinoDefault;
	macParms.lenSemigiro = macParms.impGiro / 2;
	impToMm = macParms.mmGiro / (float)macParms.impGiro;
	macParms.modo=MODE_LENR;
    macParms.ucMaxNumFili=MAX_NUM_FILI;
	/* Numero di pezzi prima della fine del lotto raggiunti i quali
	   si deve far lampeggiare la lampada di segnalazione lotto
	   in esaurimento. */
	macParms.nPezziAvviso=defPezziAvvisoDefault;
	macParms.nSecLampSpegn=defSecLampSpegnDefault;
	macParms.nspire_pret=defNspirePretDefault;  /* Numero di spire di pretaglio. */
	macParms.iasse[0]=defIasse0Default;  /* Distanza d' interasse mandrino coltello. */
	macParms.iasse[1]=defIasse1Default;
	macParms.diam_frizioni[0]=defDiamFriz0Default;  /* Diametro delle frizioni in mm. */
	macParms.diam_frizioni[1]=defDiamFriz1Default;  /* Diametro delle frizioni in mm. */
	macParms.diam_coltello=defDiamColtelloDefault;  /* Diametro del coltello in mm. */
	macParms.nsec=DEFAULT_NSEC_AVVIO;
	macParms.durata_all=defDurataAllDefault;
	macParms.DiamTestIntrz=defDiamTestIntrzDefault; // Per diametri filo >= a quanto indicato qui, con due fili non si testa interruzione filo. 
	macParms.ucPercMaxErrRo=defPercMaxErrRoDefault;
    // Num max di spezzoni consecutivi con misura fuori tolleranza ammessi. 
	macParms.ucMaxBadSpezzConsec=defMaxBurstErrRoDefault;
	macParms.fFSassorbRuote=FS_AMPERE_ASSORB;

	for (i=0;i<NCANALI_TARABILI;i++){
		actPortata = 0;
		while (actPortata<NUM_PORTATE_DINAMICHE) {
			macParms.tarParms[i][actPortata].k=1.;
			macParms.tarParms[i][actPortata].ksh=1024;
			macParms.tarParms[i][actPortata].o=0.;
			macParms.tarParms[i][actPortata].osh=0;
			macParms.tarParms[i][actPortata].p=1.0;
			macParms.tarParms[i][actPortata].p0=0.0;
			macParms.tarParms[i][actPortata].m=1.0;
			macParms.tarParms[i][actPortata].m0=1.0;
			macParms.tarParms[i][actPortata].mstep=1.0;
			macParms.tarParms[i][actPortata].m0step=1.0;
			actPortata++;
		}
	}
	actPortata=0;
    macParms.password=0x31313131;
    macParms.uc_1Encoder_0Potenziometri=0;
	// abilitazione a passaggio automatico da una comemssa alla successiva
	macParms.ucEnableCommesseAutomatiche=0;
	// char che indica se � abilitata la quinta portata
	macParms.ucAbilitaQuintaPortata=0;
	// un char per indicare se pilotaggio taglio/pretaglio usa sensore o no
	macParms.ucUsaSensoreTaglio=0;
	// posizione minima e massima del coltello, tolleranza nel posizionamento del coltello; [step a/d]
	macParms.uiMinPosColtello_StepAD=8000;
    macParms.uiMaxPosColtello_StepAD=2500;
    macParms.uiTolerancePosColtello_StepAD=300;
	// valore massimo da dare al dac quando si � in modo slow
	macParms.ucSlowDacColtello=10;
	// guadagno per il controllo del coltello [0..255]
	macParms.ucGainDacColtello=5;
	macParms.uiRitardoAttivazioneBandella_centesimi_secondo=20;
	macParms.uiDurataAttivazioneBandella_centesimi_secondo=100;
	// abilitazione gestione lampada allarme
	macParms.ucAbilitaLampadaAllarme=0;
    // presenza distanziatore: 1--> presente, else non presente
	macParms.uc_distanziatore_type=0;
    // spindle grinding non abilitato
	macParms.uc_enable_spindle_grinding=0;
    // distanza tra distanziatore e coltello [mm]
	macParms.fOffsetDistanziatore_ColtelloMm=0.0;
    // tempo che il distanziatore impiega a scendere in posizione bassa
    // questo valore viene assegnato ad ogni programma che usa il distanziatore
	macParms.usDelayDistanziaDownMs=800;

	set_check_tangled_wire_params_default();
    
    
// here the default setting ends and we calculate the crc    
    // recalc the crc...
    ucCalcCrcMacParams(&macParms.ucCrc);

}//void vMacParmsAtDefaultValue(void)


/* Procedura di validazione dei parametri macchina; viene chiamata ogni volta
   che si entra nel menu parametri macchina. */
void ValidateMacParms(void){
    xdat unsigned int n;

	if (macParms.uiMaxPosColtello_StepAD>8000)
		macParms.uiMaxPosColtello_StepAD=8000;
	else if (macParms.uiMaxPosColtello_StepAD<2500)
		macParms.uiMaxPosColtello_StepAD=2500;
	if (macParms.uiMinPosColtello_StepAD>2500)
		macParms.uiMinPosColtello_StepAD=2500;

	if (macParms.uiTolerancePosColtello_StepAD>8000)
		macParms.uiTolerancePosColtello_StepAD=300;

	// valore massimo da dare al dac quando si � in modo slow
	if (macParms.ucSlowDacColtello>30){
		macParms.ucSlowDacColtello=30;
	}
	// guadagno per il controllo del coltello [0..255]
	if (macParms.ucGainDacColtello<5)
		macParms.ucGainDacColtello=5;


    if (spiralatrice.actFornitore>MAXFORNITORI)
		spiralatrice.actFornitore=MAXFORNITORI;
	if (macParms.ucMaxNumFili<MIN_NUM_FILI)
		macParms.ucMaxNumFili=MIN_NUM_FILI;
	else if (macParms.ucMaxNumFili>MAX_NUM_FILI)
		macParms.ucMaxNumFili=MAX_NUM_FILI;


    if ((macParms.iasse[0]<MIN_IASSE)||(macParms.iasse[0]>MAX_IASSE))
		macParms.iasse[0]=AsseSpi_AsseColt_mm;
    if ((macParms.iasse[1]<MIN_IASSE)||(macParms.iasse[1]>MAX_IASSE)){
		macParms.iasse[1]=AsseSpi_AsseColt_mm+DefaultCorsaColtello;
		spiralatrice.CorsaColtello=DefaultCorsaColtello;
    }
    if (macParms.rpm_mandrino<MIN_RPM_MANDRINO)
		macParms.rpm_mandrino=MIN_RPM_MANDRINO;
    else if (macParms.rpm_mandrino>MAX_RPM_MANDRINO)
		macParms.rpm_mandrino=MAX_RPM_MANDRINO;

    /* Validazione del modo. Se non � un modo valido, uso per default il modo R.
    */
    if ((macParms.modo!=MODE_LEN)&&(macParms.modo!=MODE_LENR))
		macParms.modo=MODE_LENR;

    if (macParms.nPezziAvviso>MAX_NPEZZI_AVVISO)
		macParms.nPezziAvviso=MAX_NPEZZI_AVVISO;

    if (macParms.DiamTestIntrz<MIN_DIAM_TEST_INTRZ)
		macParms.DiamTestIntrz=MIN_DIAM_TEST_INTRZ;

    if (macParms.nSecLampSpegn>MAX_LAMP_SPEGN)
		macParms.nSecLampSpegn=MAX_LAMP_SPEGN;
    if (macParms.durata_all<MIN_DURATA_ALL)
		macParms.durata_all=MIN_DURATA_ALL;
    else if (macParms.durata_all>MAX_DURATA_ALL)
		macParms.durata_all=MAX_DURATA_ALL;

    for (n=0;n<MAX_NUM_FILI;n++){
		if (nvram_struct.resspec[n]<MIN_RESSPEC_FILO)
			nvram_struct.resspec[n]=MIN_RESSPEC_FILO;
		if (nvram_struct.resspec[n]>MAX_RESSPEC_FILO)
			nvram_struct.resspec[n]=MAX_RESSPEC_FILO;
    }
	if (macParms.uiRitardoAttivazioneBandella_centesimi_secondo>MAX_RITARDO_BANDELLA)
		macParms.uiRitardoAttivazioneBandella_centesimi_secondo=0;
	if (macParms.uiDurataAttivazioneBandella_centesimi_secondo>MAX_ATTIVAZIONE_BANDELLA)
		macParms.uiDurataAttivazioneBandella_centesimi_secondo=0;
		// presenza distanziatore: 
		// enum_distanziatore_type_none=0,		// nessun distanziatore presente
		// enum_distanziatore_type_pneumatico,	// distanziatore pneumatico
		// enum_distanziatore_type_coltello,	// coltello usato anche come distanziatore
		// enum_distanziatore_type_numOf

		if (macParms.uc_distanziatore_type>=enum_distanziatore_type_numOf)
			macParms.uc_distanziatore_type=enum_distanziatore_type_none;
		// distanza tra distanziatore e coltello [mm]
		if (macParms.fOffsetDistanziatore_ColtelloMm<MIN_OFFSET_COLTELLO_DISTANZIATORE_mm){
			macParms.fOffsetDistanziatore_ColtelloMm=MIN_OFFSET_COLTELLO_DISTANZIATORE_mm;
		}
		else if (macParms.fOffsetDistanziatore_ColtelloMm>MAX_OFFSET_COLTELLO_DISTANZIATORE_mm){
			macParms.fOffsetDistanziatore_ColtelloMm=MAX_OFFSET_COLTELLO_DISTANZIATORE_mm;
		}
		if (  (macParms.usDelayDistanziaDownMs<MIN_DURATA_DISCESA_DISTANZIATORE)
			||(macParms.usDelayDistanziaDownMs>MAX_DURATA_DISCESA_DISTANZIATORE)
			){
			macParms.usDelayDistanziaDownMs=DEFAULT_DURATA_DISCESA_DISTANZIATORE;
		}

}//void ValidateMacParms(void)



