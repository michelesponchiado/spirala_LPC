#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <ctype.h>

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
#include "i2c_master.h"
#include "fpga_nxp.h"

//#define def_small_code


xdata unsigned char ucStartLavorazione_enabled;
xdata unsigned char ucCountNumLoop2VisualizeMisuraStaticaIntaratura;
extern xdata unsigned char ucBloccaVisualizzazioneMisuraStatica;

xdata TipoStructAnalyzeInputEdge aie[enumInputEdge_numOf];
unsigned char ucIsLockedJob(unsigned char i);




// utilit� di calcolo resistenza di uno spezzone
float f_calc_res_spezzone(float f_res_spec_filo){
	return f_res_spec_filo*(macParms.mmGiro)/1000./2.;
}

/* Utilit� di calcolo della portata dinamica
	Parto dalla portata piu' bassa e salgo. Se vado fuori portata,
	la routine di servizio dell' interruzione cerca alla portata
	di numero inferiore (10 volte maggiore per�), per cui non
	corro rischi con valori a cavallo di due portate, se non quello
	di dover effettuare due cambi portata.
*/
unsigned short int us_calc_portata_dinamica_per_resistenza(float f_max_res_to_measure){
	xdata unsigned short idx_portata;
	/* Minimo fondo scala misura dinamica: 5 ohm/spezzone. */
	if (!macParms.ucAbilitaQuintaPortata)
		idx_portata=NUM_PORTATE_DINAMICHE-2;
	else
		idx_portata=NUM_PORTATE_DINAMICHE-1;
	/* Esco sicuramente dal ciclo con i=0, vale a dire la portata pi� alta. */
	while(idx_portata){
		/* Quando trovo che la resistenza dello spezzone diviene minore del fondo scala, 
			ho trovato la portata giusta.
		*/
		if (f_max_res_to_measure<portataDinamica[idx_portata]){
			break;
		}
		idx_portata--;
	}
	return idx_portata;
}



// utilit� di calcolo resistenza specifica filo
// se resistenze specifiche non inizializzate correttamente, restituisce 0
float f_calc_res_spec_filo(PROGRAMMA * pPro){
	xdata float f_aux;
	register unsigned char i;
	f_aux=0.0;
	for (i=0;i<pPro->num_fili;i++){
		if (nvram_struct.resspec[i]<=0){
			f_aux=0.0;
			break;
		}
		f_aux+=1/nvram_struct.resspec[i];
	}
	if (f_aux)
		f_aux=1/f_aux;
	return f_aux;
}


// main menu della spiralatrice...
unsigned char ucHW_MainMenu(void){

	#define defMAINMENU_button_Title_row 6
	#define defMAINMENU_button_Title_col ((defLcdWidthX_pixel-10*16)/2)

	#define defDistanceRowBetweenPuls_MainMenu 36

	#define defMAINMENU_button_CodiceProdotto_row (defMAINMENU_button_Title_row+32)
	#define defMAINMENU_button_CodiceProdotto_col 0


	#define defMAINMENU_button_Produzione_row (defMAINMENU_button_CodiceProdotto_row+defDistanceRowBetweenPuls_MainMenu)
	#define defMAINMENU_button_Produzione_col 0

	#define defMAINMENU_button_SingleJob_row (defMAINMENU_button_CodiceProdotto_row+defDistanceRowBetweenPuls_MainMenu*2)
	#define defMAINMENU_button_SingleJob_col 0

	#define defMAINMENU_button_ArchivioCodici_row (defMAINMENU_button_CodiceProdotto_row+defDistanceRowBetweenPuls_MainMenu*3)
	#define defMAINMENU_button_ArchivioCodici_col 0


	#define defMAINMENU_button_Utilities_row (defMAINMENU_button_CodiceProdotto_row+defDistanceRowBetweenPuls_MainMenu*4)
	#define defMAINMENU_button_Utilities_col 0


	#define defMAINMENU_button_Esc_row (defLcdWidthY_pixel-24)
	#define defMAINMENU_button_Esc_col (defLcdWidthX_pixel-4*16-8)


	// pulsanti della finestra
	typedef enum {
			enum_MAINMENU_Title=0,
			enum_MAINMENU_ListaDiProduzione,
			enum_MAINMENU_Produzione,
			enum_MAINMENU_SingleJob,
			enum_MAINMENU_ArchivioCodici,
			enum_MAINMENU_Utilities,

			enum_MAINMENU_Sx,
			enum_MAINMENU_Up,
			enum_MAINMENU_Cut,
			enum_MAINMENU_Esc,
			enum_MAINMENU_Dn,
			enum_MAINMENU_Dx,

			enum_MAINMENU_NumOfButtons
		}enum_MAINMENU_Buttons;
	xdata unsigned char i;		
	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enum_MAINMENU_NumOfButtons;i++)
				ucCreateTheButton(i); 
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			vRefreshActiveProductCode();
			ucEnableStartLavorazione(1);
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enum_MAINMENU_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enum_MAINMENU_ArchivioCodici:
							vEmptyStackCaller();
							// impostazione del parametro della finestra
							ucSetWindowParam(enumWinId_CodiceProdotto,defIdxParam_ArchivioProdotti_showDeleteButton,1);
							vJumpToWindow(enumWinId_CodiceProdotto);
							break;
						case enum_MAINMENU_ListaDiProduzione:
							vEmptyStackCaller();
							vJumpToWindow(enumWinId_listaCodici);
							break;
						case enum_MAINMENU_SingleJob:
							if (!PrgRunning){
								vEmptyStackCaller();
								vJumpToWindow(enumWinId_LottoDiretto);
							}
							break;
						case enum_MAINMENU_Utilities:
							vEmptyStackCaller();
							vJumpToWindow(enumWinId_Utilities);
							break;
						case enum_MAINMENU_Produzione:
							vEmptyStackCaller();
							vJumpToWindow(enumWinId_setupProduzione);
							break;
						case enum_MAINMENU_Esc:
							break;
					}
				}
			}

			//strcpy(hw.ucString,"MAIN MENU");
			vStringLangCopy(hw.ucString,enumStr20_MainMenu);
			//ucPrintStaticButton(hw.ucString,defMAINMENU_button_Title_row ,defMAINMENU_button_Title_col,enumFontMedium,enum_MAINMENU_Title,defLCD_Color_LightGreen);
			ucPrintTitleButton(hw.ucString,defMAINMENU_button_Title_row ,defMAINMENU_button_Title_col,enumFontMedium,enum_MAINMENU_Title,defLCD_Color_Trasparente,1);

			//strcpy(hw.ucString,"LISTA DI PRODUZIONE");
			sprintf(hw.ucString,"%-20.20s",pucStringLang(enumStr20_Lista_Codici));
			ucPrintStaticButton(hw.ucString,defMAINMENU_button_CodiceProdotto_row ,defMAINMENU_button_CodiceProdotto_col,enumFontMedium,enum_MAINMENU_ListaDiProduzione,defLCD_Color_Trasparente);

	
			//strcpy(hw.ucString,"PARAMETRI MACCHINA");
			sprintf(hw.ucString,"%-20.20s",pucStringLang(enumStr20_SingleJob));
			ucPrintStaticButton(hw.ucString,defMAINMENU_button_SingleJob_row ,defMAINMENU_button_SingleJob_col,enumFontMedium,enum_MAINMENU_SingleJob,defLCD_Color_Trasparente);


			//strcpy(hw.ucString,"ARCHIVIO CODICI      ");
			sprintf(hw.ucString,"%-20.20s",pucStringLang(enumStr20_CodiceProdotto));
			ucPrintStaticButton(hw.ucString,defMAINMENU_button_ArchivioCodici_row ,defMAINMENU_button_ArchivioCodici_col,enumFontMedium,enum_MAINMENU_ArchivioCodici,defLCD_Color_Trasparente);

			//strcpy(hw.ucString,"PRODUZIONE        ");
			sprintf(hw.ucString,"%-20.20s",pucStringLang(enumStr20_Produzione));
			ucPrintStaticButton(hw.ucString,defMAINMENU_button_Produzione_row ,defMAINMENU_button_Produzione_col,enumFontMedium,enum_MAINMENU_Produzione,defLCD_Color_Trasparente);


			//strcpy(hw.ucString,"UTILITIES         ");
			sprintf(hw.ucString,"%-20.20s",pucStringLang(enumStr20_Utilities));
			ucPrintStaticButton(hw.ucString,defMAINMENU_button_Utilities_row ,defMAINMENU_button_Utilities_col,enumFontMedium,enum_MAINMENU_Utilities,defLCD_Color_Trasparente);


			// routine che permette di aggiungere ad una finestra i bottoni standard sx, dx, up, dn, esc, cut
			vAddStandardButtons(enum_MAINMENU_Sx);


			return 1;
	}//switch(uiGetStatusCurrentWindow())
}//unsigned char ucHW_MainMenu(void)

// funzione yesno
unsigned char ucHW_YesNo(void){


	#define defYESNO_button_Title_0_row 32
	#define defYESNO_button_Title_0_col 0

	#define defYESNO_button_Title_1_row (defYESNO_button_Title_0_row+24)
	#define defYESNO_button_Title_1_col 0

	#define defYESNO_button_Title_2_row (defYESNO_button_Title_0_row+24*2)
	#define defYESNO_button_Title_2_col 0

	#define defYESNO_button_Title_3_row (defYESNO_button_Title_0_row+24*3)
	#define defYESNO_button_Title_3_col 0

	#define defYESNO_button_Answer_YES_row (defYESNO_button_Title_0_row+24*4)
	#define defYESNO_button_Answer_YES_col (32+164)

	#define defYESNO_button_Answer_NO_row defYESNO_button_Answer_YES_row
	#define defYESNO_button_Answer_NO_col (32)

	// pulsanti della finestra
	typedef enum {
			enum_YES_NO_Title_0=0,
			enum_YES_NO_Title_1,
			enum_YES_NO_Title_2,
			enum_YES_NO_Title_3,
			enum_YES_NO_Answer_YES,
			enum_YES_NO_Answer_NO,
			enum_YES_NO_NumOfButtons
		}enum_YES_NO_Buttons;
	xdata unsigned char i;		
	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enum_YES_NO_NumOfButtons;i++)
				ucCreateTheButton(i); 
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enum_YES_NO_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enum_YES_NO_Answer_YES:
							// indico che ho risposto yes
							defSetWinParam(defWinParam_YESNO_Answer,1);
							ucReturnFromWindow();
							return 2;
						case enum_YES_NO_Answer_NO:
							// indico che ho risposto no
							defSetWinParam(defWinParam_YESNO_Answer,0);
							ucReturnFromWindow();
							return 2;
					}
				}
			}

			if (strlen(hw.ucStringNumKeypad_in)>20){
				sprintf(hw.ucString,"%-20.20s",hw.ucStringNumKeypad_in);
				ucPrintTitleButton(hw.ucString,defYESNO_button_Title_0_row,defYESNO_button_Title_0_col,enumFontMedium,enum_YES_NO_Title_0,defLCD_Color_Trasparente,1);
				sprintf(hw.ucString,"%-20.20s",hw.ucStringNumKeypad_in+20);
				ucPrintTitleButton(hw.ucString,defYESNO_button_Title_1_row,defYESNO_button_Title_1_col,enumFontMedium,enum_YES_NO_Title_1,defLCD_Color_Trasparente,1);
			}
			else{
				sprintf(hw.ucString,"%-20.20s",hw.ucStringNumKeypad_in);
				ucPrintTitleButton(hw.ucString,defYESNO_button_Title_1_row,defYESNO_button_Title_1_col,enumFontMedium,enum_YES_NO_Title_1,defLCD_Color_Trasparente,1);
			}


			if (strlen(hw.ucStringNumKeypad_out)>20){
				sprintf(hw.ucString,"%-20.20s",hw.ucStringNumKeypad_out);
				ucPrintTitleButton(hw.ucString,defYESNO_button_Title_2_row,defYESNO_button_Title_0_col,enumFontMedium,enum_YES_NO_Title_2,defLCD_Color_Trasparente,1);
				sprintf(hw.ucString,"%-20.20s",hw.ucStringNumKeypad_out+20);
				ucPrintTitleButton(hw.ucString,defYESNO_button_Title_3_row,defYESNO_button_Title_1_col,enumFontMedium,enum_YES_NO_Title_3,defLCD_Color_Trasparente,1);
			}
			else{
				sprintf(hw.ucString,"%-20.20s",hw.ucStringNumKeypad_out);
				ucPrintTitleButton(hw.ucString,defYESNO_button_Title_2_row,defYESNO_button_Title_1_col,enumFontMedium,enum_YES_NO_Title_2,defLCD_Color_Trasparente,1);
			}

			if (defGetWinParam(def_WinYesNo_Param_TipoPulsante)==def_WinYesNo_PUlsantiYesNo){
				//strcpy(hw.ucString,"YES");
				vStringLangCopy(hw.ucString,enumStr_Yes);
				ucPrintStaticButton(hw.ucString,defYESNO_button_Answer_YES_row,defYESNO_button_Answer_YES_col,enumFontBig,enum_YES_NO_Answer_YES,defLCD_Color_Trasparente);
				//strcpy(hw.ucString,"NO");
				vStringLangCopy(hw.ucString,enumStr_No);
				ucPrintStaticButton(hw.ucString,defYESNO_button_Answer_NO_row,defYESNO_button_Answer_NO_col,enumFontBig,enum_YES_NO_Answer_NO,defLCD_Color_Trasparente);
			}
			else{
				vStringLangCopy(hw.ucString,enumStr20_Ok);
				ucPrintStaticButton(hw.ucString,defYESNO_button_Answer_NO_row,(defYESNO_button_Answer_NO_col+defYESNO_button_Answer_YES_col)/2,enumFontBig,enum_YES_NO_Answer_NO,defLCD_Color_Trasparente);
			}
			return 1;
	}//switch(uiGetStatusCurrentWindow())
}//unsigned char ucHW_YesNo(void)

/* Procedura di validazione dei parametri del programma attualmente
   in visualizzazione. */
void ValidatePrg(void){
 /* Puntatore alla struttura programma per accedere comodamente ai dati. */
 PROGRAMMA * xdat pPro;
 unsigned char xdat n;
 /* Punto all' entry dell' array dei programmi che contiene il programma
    in visualizzazione. */
 pPro=hprg.ptheAct;

 /* Dato che posso entrare in print usando le frecce
    sx e dx, devo validare sempre all' ingresso il
    numero di filo. */
 if (actFilo>=hprg.ptheAct->num_fili-1)
    actFilo=MIN_NUM_FILI-1;

 if (pPro->rpm_mandrino<MIN_RPM_MANDRINO)
	pPro->rpm_mandrino=MIN_RPM_MANDRINO;
 else if (pPro->rpm_mandrino>MAX_RPM_MANDRINO)
	pPro->rpm_mandrino=MAX_RPM_MANDRINO;

 /* Validazione del numero di fili. */
 if (pPro->num_fili>macParms.ucMaxNumFili)
     pPro->num_fili=macParms.ucMaxNumFili;

 if (pPro->num_fili<MIN_NUM_FILI)
     pPro->num_fili=MIN_NUM_FILI;

 /* Validazione  del diametro del filo. */
 if (pPro->diametro_filo>MAX_DIAM_FILO)
     pPro->diametro_filo=MAX_DIAM_FILO;

 if (pPro->diametro_filo<MIN_DIAM_FILO)
     pPro->diametro_filo=MIN_DIAM_FILO;

 /* Validazione della velocit� di produzione. */
 if (pPro->vel_produzione>MAX_VEL_PROD)
     pPro->vel_produzione=MAX_VEL_PROD;

 if (pPro->vel_produzione<MIN_VEL_PROD)
     pPro->vel_produzione=MIN_VEL_PROD;

 /* Validazione  del diametro del mandrino. */
 if (pPro->diametro_mandrino>MAX_DIAM_MD)
     pPro->diametro_mandrino=MAX_DIAM_MD;

 if (pPro->diametro_mandrino<MIN_DIAM_MD)
     pPro->diametro_mandrino=MIN_DIAM_MD;

 if (pPro->coeff_corr>10)
	pPro->coeff_corr=1;
 else if (pPro->coeff_corr<0.1)
	pPro->coeff_corr=1;

 /* Validazione  delle coppie delle frizioni. */
 for (n=0;n<NUM_FRIZIONI;n++){
     if (pPro->vruote_prg[n]>MAX_DELTA_VRUOTE)
	 pPro->vruote_prg[n]=MAX_DELTA_VRUOTE;
 #if MIN_DELTA_VRUOTE>0
     if (pPro->vruote_prg[n]<MIN_DELTA_VRUOTE)
		 pPro->vruote_prg[n]=MIN_DELTA_VRUOTE;
 #endif		 
 }

 for (n=0;n<MAX_NUM_FILI;n++){
     if (nvram_struct.resspec[n]<MIN_RESSPEC_FILO)
		 nvram_struct.resspec[n]=MIN_RESSPEC_FILO;
     if (nvram_struct.resspec[n]>MAX_RESSPEC_FILO)
		nvram_struct.resspec[n]=MAX_RESSPEC_FILO;
 }
}





unsigned char ucNeedToClosePopupWindow(void){
	switch(hM.ucActivated){
		case 0:
			return 0;
		case 1:
			if (!(OutDig & ODG_MSTA)&&!sLCD.ucAttivataMisuraDinamica){
				return 1;
			}
			return 0;
	}
	return 0;
}
xdata TipoStructHandleMeasure hM;
unsigned char ucNeedToClosePopupWindow(void);



// ritorna 1 se le finestre sottostanti NON devono essere rigenerate...
unsigned char ucHandlePopupWindow(void){
	// indica se comando remoto attivo
	extern unsigned char ucRemoteCommandRunning(void);
	unsigned int ui_num_decimali;

	static code unsigned char ucLivingCodes[2][3]={"* ","  "};
	// 
	static unsigned char remoteCommandStatus=0;

	typedef enum{
		enumButtonMeasureTitle=defMaxButtonsOnLcd-1,
		enumButtonMeasureValue=defMaxButtonsOnLcd-2,
		enumButtonMeasureScala=defMaxButtonsOnLcd-3,
		enumButtonMeasure_desc1=defMaxButtonsOnLcd-4,
		enumButtonMeasure_desc2=defMaxButtonsOnLcd-5,
		enumButtonMeasure_desc3=defMaxButtonsOnLcd-6,
		enumButtonMeasure_desc4=defMaxButtonsOnLcd-7,
		enumButtonMeasureNumOf
	}enumButtonMeasure;

	typedef enum{
		enumButtonAlarmTitle =defMaxButtonsOnLcd-1,
		enumButtonAlarmDesc  =defMaxButtonsOnLcd-2,
		enumButtonAlarmDesc_2=defMaxButtonsOnLcd-3,
		enumButtonAlarmDesc_3=defMaxButtonsOnLcd-4,
		enumButtonAlarmDesc_4=defMaxButtonsOnLcd-5,
		enumButtonAlarmOk    =defMaxButtonsOnLcd-6,
		enumButtonAlarmNumOf
	}enumButtonAlarm;


	xdata unsigned char i,fontType;
	xdata unsigned int colonna;
	xdata float fResistenza_Visualizzata,f_aux,f_res_spezzone;
	unsigned int ui_visualizza_misura_statica;
	#define defMeasure_Title_row (36)
	#define defMeasure_Title_col (64-12)
	#define defMeasure_Meas_row (defMeasure_Title_row+40)
	#define defMeasure_Meas_col 64
	#define defMeasure_Scala_row (defMeasure_Meas_row+40+16)
	#define defMeasure_Scala_col (64+28)

	#define defMeasure_title_alarm_row 12
	#define defMeasure_title_alarm_col 0

	#define defOffset_title_desc_alarm 54
	#define defOffset_desc_desc_alarm 28

	#define defMeasure_desc_alarm_row (defMeasure_title_alarm_row+defOffset_title_desc_alarm)
	#define defMeasure_desc_alarm_col 0
// la descrizione allarme 4 la visualizzo sopra la stringa di descrizione 2
// in questo modo vedo:
// * filo fuori tolleranza
// * misurato > set point
// * 7.2 > 6.5
// * +10.7%
	#define defMeasure_desc_2_alarm_row (defMeasure_desc_alarm_row+defOffset_desc_desc_alarm*2)
	#define defMeasure_desc_2_alarm_col 0

	#define defMeasure_desc_3_alarm_row (defMeasure_desc_alarm_row+defOffset_desc_desc_alarm*3)
	#define defMeasure_desc_3_alarm_col 0
	#define defMeasure_desc_4_alarm_row (defMeasure_desc_alarm_row+defOffset_desc_desc_alarm*1)
	#define defMeasure_desc_4_alarm_col 0
	
	#define defMeasure_ok_alarm_row (defMeasure_desc_alarm_row+4*defOffset_desc_desc_alarm)
	#define defMeasure_ok_alarm_col ((defLcdWidthX_pixel-32*3-4)/2)

	
#ifdef def_small_code
	return 0;
#else
	ui_num_decimali=0;
	ui_visualizza_misura_statica=0;
	fResistenza_Visualizzata = 0;
	// se c'� un allarme, lo visualizzo...
	if ((alr_status==alr_status_alr)||hM.ucAlarmStatus){
		for (i=0;i<16;i++){
			if (alarms&(1L<<i)){
				if (!hM.ucAlarmStatus){
					ucCreateTheButton(enumButtonAlarmTitle);
					ucCreateTheButton(enumButtonAlarmDesc);
					ucCreateTheButton(enumButtonAlarmOk);
					ucCreateTheButton(enumButtonAlarmDesc_2);
					spiralatrice.oldalarms=alarms;
					hM.ucAlarmStatus=i+1;
				}
				else if (ucHasBeenPressedButton(enumButtonAlarmOk)){
					extern unsigned int ui_clear_i2c_alarm_subcode(void);
					ui_clear_i2c_alarm_subcode();
					alarms = 0;
					/* Indico che l' allarme deve essere rivalutato. */
					alr_status=alr_status_noalr;
					/* Spengo la lampada gialla. */
					outDigVariable &= ~ODG_LAMPADA;
					/* La lampada � spenta. */
					LampStatus=LampSpenta;
					/* Spengo la lampada rossa. */
					outDigVariable &= ~ODG_LAMPADA_RED;

					hM.ucAlarmStatus=0;
					vForceLcdRefresh();
				}
				//strcpy(hM.ucTitle_alarm,"      Allarme!      ");
				vStringLangCopy(hM.ucDesc_alarm2,enumStr20_TitoloAllarmi);	
				vStrcpy_trimBlank(hM.ucTitle_alarm,hM.ucDesc_alarm2,sizeof(hM.ucTitle_alarm)-2);
				if (strlen(hM.ucTitle_alarm)<=10)
					ucPrintTitleButton(hM.ucTitle_alarm,defMeasure_title_alarm_row,defMeasure_title_alarm_col,enumFontBig,enumButtonAlarmTitle,defLCD_Color_Yellow,9);
				else
					ucPrintTitleButton(hM.ucTitle_alarm,defMeasure_title_alarm_row,defMeasure_title_alarm_col,enumFontBig,enumButtonAlarmTitle,defLCD_Color_Yellow,9);
				//strcpy(hM.ucDesc_alarm,ucTabellaMsgAllarmi[i]);
				vStringLangCopy(hM.ucDesc_alarm,enumStr20_Allarmi+i);
				ucPrintTitleButton(hM.ucDesc_alarm,defMeasure_desc_alarm_row,defMeasure_desc_alarm_col,enumFontMedium,enumButtonAlarmDesc,defLCD_Color_Trasparente,9);

				hM.ucDesc_alarm2[0]=0;
				hM.ucDesc_alarm3[0]=0;
				hM.ucDesc_alarm4[0]=0;

				switch(1L<<i){
					case ALR_TEMPERATURE: 
						{
							// add diagnostic info to temperature alarm
							extern unsigned int ui_temperature_alarm_subcode;
							if (ui_temperature_alarm_subcode){
								vStringLangCopy(hM.ucDesc_alarm2,ui_temperature_alarm_subcode);
							}
							break;
						}
					case ALR_TOLMIS:
						if (macParms.modo == MODE_LENR){
							vStringLangCopy(hM.ucDesc_alarm2,enumStr20_SetpointValue);
							vStringLangCopy(hM.ucDesc_alarm3,enumStr20_MeasuredValue);
 							if (sLCD.ulInfoAlarms[0]>sLCD.ulInfoAlarms[1])
                            {
                                float f1,f2;
                                f1=((float)(sLCD.ulInfoAlarms[0]))/(TARGET_URP * MAGN_LEN * impToMm / 1000. /targetRes );
                                f2=((float)(sLCD.ulInfoAlarms[3]))/(TARGET_URP * MAGN_LEN * impToMm / 1000. /targetRes );
                                if (nvram_struct.actUM==UM_IN)
                                {
                                    f1*=feet2metri;
                                    f2*=feet2metri;
                                }
								//strcpy(hM.ucDesc_alarm2,"MISURA>Massimo");
								sprintf(hM.ucDesc_alarm4,"%s > %s",hM.ucDesc_alarm3,hM.ucDesc_alarm2);
								// vStringLangCopy(hM.ucDesc_alarm2,enumStr20_MeasuredTooBig);	
								sprintf(hM.ucDesc_alarm2,"%5.2f > %5.2f",f1,f2);
								sprintf(hM.ucDesc_alarm3,"    +%i%%",(int)(((sLCD.ulInfoAlarms[0]-sLCD.ulInfoAlarms[3])*100L)/sLCD.ulInfoAlarms[3]));
							}
							else
                            {
                                float f1,f2;
								f1=((float)(sLCD.ulInfoAlarms[0]))/(TARGET_URP * MAGN_LEN * impToMm / 1000. /targetRes );
								f2=((float)(sLCD.ulInfoAlarms[3]))/(TARGET_URP * MAGN_LEN * impToMm / 1000. /targetRes );
                                if (nvram_struct.actUM==UM_IN)
                                {
                                    f1*=feet2metri;
                                    f2*=feet2metri;
                                }
								//strcpy(hM.ucDesc_alarm2,"misura<Minimo");
								//vStringLangCopy(hM.ucDesc_alarm2,enumStr20_MeasuredTooLittle);
								sprintf(hM.ucDesc_alarm4,"%s < %s",hM.ucDesc_alarm3,hM.ucDesc_alarm2);
								sprintf(hM.ucDesc_alarm2,"%5.2f < %5.2f",f1,f2);
								sprintf(hM.ucDesc_alarm3,"    -%i%%",(int)(((sLCD.ulInfoAlarms[3]-sLCD.ulInfoAlarms[0])*100L)/sLCD.ulInfoAlarms[3]));
							}
						}
						else{
							sprintf(hM.ucDesc_alarm2,"%s %li imp",pucStringLang(enumStr20_Contati),sLCD.ulInfoAlarms[0]);
							sprintf(hM.ucDesc_alarm3,"%s %li imp",pucStringLang(enumStr20_Attesi),sLCD.ulInfoAlarms[1]);
						}

						break;
					case ALR_NOTMIS:
						sprintf(hM.ucDesc_alarm2,"%s %li imp",pucStringLang(enumStr20_Contati),sLCD.ulInfoAlarms[0]);
						sprintf(hM.ucDesc_alarm3,"%s %li imp",pucStringLang(enumStr20_Massimo),sLCD.ulInfoAlarms[1]);
						break;
					case ALR_I2C_BUS:
						{
							extern unsigned int ui_get_i2c_alarm_subcode(void);
							sprintf(hM.ucDesc_alarm2,"subcode: %i",ui_get_i2c_alarm_subcode());
							break;
						}

				}
				if (hM.ucDesc_alarm4[0]){
					if (strlen(hM.ucDesc_alarm4)>20)
						fontType=enumFontSmall;
					else
						fontType=enumFontMedium;
					ucPrintTitleButton(hM.ucDesc_alarm4,defMeasure_desc_4_alarm_row,defMeasure_desc_4_alarm_col,fontType,enumButtonAlarmDesc_4,defLCD_Color_Trasparente,9);
				}
				if (hM.ucDesc_alarm2[0])
					ucPrintTitleButton(hM.ucDesc_alarm2,defMeasure_desc_2_alarm_row,defMeasure_desc_2_alarm_col,enumFontMedium,enumButtonAlarmDesc_2,defLCD_Color_Trasparente,9);

				if (hM.ucDesc_alarm3[0])
					ucPrintTitleButton(hM.ucDesc_alarm3,defMeasure_desc_3_alarm_row,defMeasure_desc_3_alarm_col,enumFontMedium,enumButtonAlarmDesc_3,defLCD_Color_Trasparente,9);
				//strcpy(hM.ucTitle,"Esc");
				// faccio apparire la stringa "reset alarm" centrata
				vStringLangCopy(hM.ucTitle,enumStr20_ResetAlarm);
				// NON PI� DI 10 CARATTERI, DATO CHE � IN FONT BIG...
				hM.ucTitle[10]=0;
				vStrUpperCase(hM.ucTitle);
				colonna=((defLcdWidthX_pixel-32*strlen(hM.ucTitle))/2);
				ucPrintStaticButton(hM.ucTitle,defMeasure_ok_alarm_row,colonna,enumFontBig,enumButtonAlarmOk,defLCD_Color_Yellow);
				
				// RomStringAt(STR_ALR_FRENI1+i,2,0);
				break;
			}
		}
		return 1;
	}

// se comando remoto attivo, devo presentare una finestra opportuna
	switch(remoteCommandStatus){
		case 0:
			if (ucRemoteCommandRunning()){
				remoteCommandStatus=1;
				// sospendo autorefresh lcd per evitare effetti orrendi a video
				ucLcdRefreshTimeoutEnable(0);
				ucCreateTheButton(enumButtonAlarmDesc_2);
				vStringLangCopy(hM.ucDesc_alarm2,enumStr20_RemoteOperation);
				ucPrintTitleButton(hM.ucDesc_alarm2,defMeasure_desc_2_alarm_row,defMeasure_desc_2_alarm_col,enumFontMedium,enumButtonAlarmDesc_2,defLCD_Color_Trasparente,9);
				vForceLcdRefresh();
				return 1;
			}
			if (ucRemoteCommandAsking()){
				remoteCommandStatus=2;
				// sospendo autorefresh lcd per evitare effetti orrendi a video
				ucLcdRefreshTimeoutEnable(0);
				// chiedo conferma accesso
				vStringLangCopy(hw.ucStringNumKeypad_in,enumStr20_AcceptRemoteControl);
				hw.ucStringNumKeypad_out[0]=0;
				// impostazione del parametro 0 al valore 0--> visualizza pulsanti yes e no
				ucSetWindowParam(enumWinId_YesNo,def_WinYesNo_Param_TipoPulsante, def_WinYesNo_PUlsantiYesNo);
				// visualizzo yes/no...
				ucCallWindow(enumWinId_YesNo);
				vForceLcdRefresh();
				break;
			}
			break;
		case 1:
		default:
			if (!ucRemoteCommandRunning()){
				remoteCommandStatus=0;
				// salto al main menu...
				vJumpToWindow(enumWinId_MainMenu);
				vForceLcdRefresh();
				return 0;
			}
			return 1;
		case 2:
			if (!ucRemoteCommandAsking()){
				remoteCommandStatus=0;
				vForceLcdRefresh();
				return 0;
			}
			// se finestra non � pi� si/no allora vedo cosa ha risposto utente
			if (uiGetIdCurrentWindow()==enumWinId_YesNo)
				break;
			// se ha risposto s�...
			if (uiGetWindowParam(enumWinId_YesNo,defWinParam_YESNO_Answer)){
				vRemoteCommandAskingReply(1);
				// se ha risposto s�, meglio fare refresh
				ucLcdRefreshTimeoutEnable(0);
				ucCreateTheButton(enumButtonAlarmDesc_2);
				vStringLangCopy(hM.ucDesc_alarm2,enumStr20_RemoteOperation);
				ucPrintTitleButton(hM.ucDesc_alarm2,defMeasure_desc_2_alarm_row,defMeasure_desc_2_alarm_col,enumFontMedium,enumButtonAlarmDesc_2,defLCD_Color_Trasparente,9);
				vForceLcdRefresh();
			}
			else{
				vRemoteCommandAskingReply(0);
			}
			// adesso o remote command � abilitato o � disabilitato, torno nello stato 0
			remoteCommandStatus=0;
			return 1;
	}

	// se sono in un menu diagnostica, oppure finita taratura misura statica, non faccio nulla
	if (sLCD.ucSuspendDigitalIoUpdate)
		return 0;
	// Se taratura misura statica attiva, e terminata, esco con 0 per andare alla finestra di controllo produzione!!!!
	if (		spiralatrice.misStaOk
			&&(spiralatrice.OkAbilitaTaraturaStat||spiralatrice.OkAbilitaTaraFineStat)
		){
		// dato che la taratura impiega pochissimo, uno non fa in tempo a vedere la misura che gi� � finita:
		// devo allora prolungare artificialmente la procedura in modo che possano essere fatti almeno un certo numero di refresh
		// faccio almeno 10 loop di refresh della misura
		// fatti questi 10 refresh, ritorno 0 per indicare che la taratura pu� essere acquisita
		if (ucCountNumLoop2VisualizeMisuraStaticaIntaratura<10)
			++ucCountNumLoop2VisualizeMisuraStaticaIntaratura;
		else
			return 0;
	}
	// quarta riga: resistenza misurata
	if (  (!(OutDig & ODG_MSTA)&&!sLCD.ucAttivataMisuraDinamica)
		// se visualizzazione misura statica bloccata perch� ho finito la taratura e sono tornato al menu produzione
		// non devo visualizzare la finestra di misura della resistenza, per cui ritorno 0
		||(ucBloccaVisualizzazioneMisuraStatica)	
		){
		// non faccio nulla, se pulsante misura statica non premuto...
		// e se non � attivata la misura dinamica...
		ucCreateTheButton(enumButtonMeasureTitle);
		ucCreateTheButton(enumButtonMeasureValue);
		ucCreateTheButton(enumButtonMeasureScala);
		hM.ucLiving=0;
		hM.ucActivated=0;
		ucCountNumLoop2VisualizeMisuraStaticaIntaratura=0;
		return 0;
	}
	// se sto aspettando che venga resettata l'uscita misura statica
	// devo visualizzare la finestra precedente...
	else if ((ucCountNumLoop2VisualizeMisuraStaticaIntaratura>=10)&&(OutDig & ODG_MSTA)){
		return 0;
	}
	else{
		if (sLCD.ucAttivataMisuraDinamica){
			hM.ucDesc_alarm[0]=0;
			hM.ucDesc_alarm2[0]=0;
			hM.ucDesc_alarm3[0]=0;
			hM.ucDesc_alarm4[0]=0;
			//strcpy(hM.ucTitle,"Misura Dinamica");
			vStringLangCopy(hM.ucTitle,enumStr20_MisuraDinamica);
			if ((spiralatrice.resStaMediata<0)||(spiralatrice.resStaMediata>25000)){
				sprintf(hM.ucMeasure,"------",spiralatrice.resStaMediata);
				strcpy(hM.ucScala,"ohm[1x]    ");
			}
			else {
				switch(actStaPortata){
					case 0:
					default:
						sprintf(hM.ucMeasure,"%6.0f",spiralatrice.resStaMediata);
						strcpy(hM.ucDesc_alarm,"ohm [1x]    ");
						break;
					case 1:
						sprintf(hM.ucMeasure,"%6.1f",spiralatrice.resStaMediata);
						strcpy(hM.ucDesc_alarm,"ohm [10x]    ");
						break;
					case 2:
						sprintf(hM.ucMeasure,"%6.2f",spiralatrice.resStaMediata);
						strcpy(hM.ucDesc_alarm,"ohm [100x]   ");
						break;
					case 3:
						sprintf(hM.ucMeasure,"%6.3f",spiralatrice.resStaMediata);
						strcpy(hM.ucDesc_alarm,"ohm [1000x]  ");
						break;
					case 4:
						sprintf(hM.ucMeasure,"%6.3f",spiralatrice.resStaMediata);
						strcpy(hM.ucDesc_alarm,"ohm [10000x]");
						break;
				}
				sprintf(hM.ucScala,"%s %05i",hM.ucDesc_alarm,(int)InputAnalog[N_CANALE_OHMETRO_DYN]);
			// se sono nel menu log produzione devi visualizzare anche:
//    - richiesta di Michele Gaiotto: da pagina log, premo start misura ohmica e lui misura lo spezzone del filo 
//		e restituisce ohm spezzone e ohm/m spezzone
//      se possibile visualizzare anche valore atteso e/o percentuale tolleranza
//      se misura dinamica non valida, visualizzare asterischi o cancelletti
				if (ucIamInLogProduzione()){
                    if (nvram_struct.actUM==UM_IN)                    
                    {
                        sprintf(hM.ucDesc_alarm ,"%6.3f",spiralatrice.resStaMediata*2000/macParms.mmGiro*feet2metri);
                    }
                    else
                    {
                        sprintf(hM.ucDesc_alarm ,"%6.3f",spiralatrice.resStaMediata*2000/macParms.mmGiro);
                    }
					f_aux=f_calc_res_spec_filo(&hprg.theRunning);
					if (f_aux){
						f_res_spezzone=f_calc_res_spezzone(f_aux);
						sprintf(hM.ucDesc_alarm2,"%6.2f",f_res_spezzone);
                        // convert to the appropriate unit: ohm/ft
                        if (nvram_struct.actUM==UM_IN)                    
                        {
                            sprintf(hM.ucDesc_alarm3,"%6.3f",f_aux*feet2metri);
                            sprintf(hM.ucDesc_alarm4,"%+4.1f%%",100.0*feet2metri*(spiralatrice.resStaMediata-f_res_spezzone)/f_res_spezzone);
                        }
                        else
                        {
                            sprintf(hM.ucDesc_alarm3,"%6.3f",f_aux);
                            sprintf(hM.ucDesc_alarm4,"%+4.1f%%",100.0*(spiralatrice.resStaMediata-f_res_spezzone)/f_res_spezzone);
                        }
					}
				}
			}
		}
		else {
			//strcpy(hM.ucTitle,"Misura Statica");
			ui_visualizza_misura_statica=1;
			vStringLangCopy(hM.ucTitle,enumStr20_MisuraStatica_Title);
			if ((spiralatrice.resStaMediata<0)||(spiralatrice.resStaMediata>25000)){
				sprintf(hM.ucMeasure,"------",spiralatrice.resStaMediata);
				sprintf(hM.ucScala,"ohm[1x]    ",spiralatrice.resStaMediata);
			}
			else{
				fResistenza_Visualizzata=spiralatrice.resStaMediata;
				if (fResistenza_Visualizzata>9999.9){
					fResistenza_Visualizzata=(int)fResistenza_Visualizzata;
				}
				switch(OutDig& 0x7){
					case 0:
					default:
						if (spiralatrice.resStaMediata>=10000){
							sprintf(hM.ucMeasure,"%6.0f",fResistenza_Visualizzata);
							ui_num_decimali=0;
						}
						else{
							sprintf(hM.ucMeasure,"%6.1f",fResistenza_Visualizzata);
							ui_num_decimali=1;
						}
						sprintf(hM.ucScala,"ohm [1x]    ");
						break;
					case 4:
						sprintf(hM.ucMeasure,"%6.1f",fResistenza_Visualizzata);
						ui_num_decimali=1;
						sprintf(hM.ucScala,"ohm [10x]   ");
						break;
					case 2:
						sprintf(hM.ucMeasure,"%6.2f",fResistenza_Visualizzata);
						ui_num_decimali=2;
						sprintf(hM.ucScala,"ohm [100x]  ");
						break;
					case 1:
						sprintf(hM.ucMeasure,"%6.3f",fResistenza_Visualizzata);
						ui_num_decimali=3;
						sprintf(hM.ucScala,"ohm [1000x] ");
						break;
				}
			}
		}
	}
	// animo la stringa per far capire che sta leggendo...
	hM.ucLiving++;
	hM.ucLiving&=0x1;
	strcat(hM.ucScala,ucLivingCodes[hM.ucLiving]);
	hM.ucActivated=1;

// nel caso io sia nel menu log produzione, e venga premuto il tasto misura statica, devo visualizzare info
//- valore di resistenza [ohm] misurato
//- valore di resistenza specifica [ohm/m] misurato
//- valore di resistenza [ohm] ideale
//- valore di resistenza specifica [ohm/m] ideale
//- differenza percentuale fra valore ideale e misurato
	if (ucIamInLogProduzione()&&hM.ucDesc_alarm[0]){	
		// non devo visualizzare "misura dinamica"!!!
		// ucPrintTitleButton(hM.ucTitle,defMeasure_Title_row-24,defMeasure_Title_col,enumFontMedium,enumButtonMeasureTitle,defLCD_Color_Yellow,1);
		sprintf(hw.ucStringNumKeypad_in,"     %-20.20s",pucStringLang(enumStr20_MeasuredValue));
		sprintf(hw.ucString,"%s%-15.15s",hw.ucStringNumKeypad_in,pucStringLang(enumStr20_SetpointValue));
		vStrUpperCase(hw.ucString);
		ucPrintSmallText_ColNames(hw.ucString,defMeasure_Title_row-16,0);

		// stampo subito la percentuale di errore, perch� dopo user� alarm4 per le unit� di misura...
		// lo sfondo lo metto:
		//    rosso se la tolleranza � superiore a quella ammessa, 
		//    giallo se � fra l'80% ed il 100% della massima ammessa
		//    verde se inferiore all'80% della massima ammessa
		f_aux=fabs(atof(hM.ucDesc_alarm4))/nvram_struct.TabFornitori[spiralatrice.pPrgRun->fornitore];
		if (f_aux>1)
			i=defLCD_Color_Red;
		else if (f_aux>=0.5)
			i=defLCD_Color_Yellow;
		else 
			i=defLCD_Color_Green;

		ucPrintTitleButton(hM.ucDesc_alarm4,defMeasure_Title_row+64*2,12,enumFontBig,enumButtonMeasure_desc4,i,2);

		// mi preparo la stringa "ohm"
		sprintf(hM.ucDesc_alarm4,"%s",pucStringLang(enumStr20_FirstUm+enumUm_ohm));
		// stampo ohm spezzone misurato
		ucPrintTitleButton(hM.ucMeasure,	defMeasure_Title_row		,12,enumFontBig,enumButtonMeasureValue,defLCD_Color_Yellow,2);
		// stampo ohm spezzone ideale
		ucPrintTitleButton(hM.ucDesc_alarm2,defMeasure_Title_row     ,12+6*32+8,enumFontMedium,enumButtonMeasure_desc2,defLCD_Color_Green,2);
		// stampo stringa unit� di misura "ohm"
		ucPrintTitleButton(hM.ucDesc_alarm4,defMeasure_Title_row+24  ,defLcdWidthX_pixel-12*8-12,enumFontSmall,enumButtonMeasure_desc2,defLCD_Color_Trasparente,0);
        // use the correct unit!
		if (nvram_struct.actUM==UM_IN)
        {
            // mi preparo la stringa "ohm/feet"
            sprintf(hM.ucDesc_alarm4,"%s",pucStringLang(enumStr20_FirstUm+enumUm_ohm_per_feet));
        }
        else
        {
            // mi preparo la stringa "ohm/metro"
            sprintf(hM.ucDesc_alarm4,"%s",pucStringLang(enumStr20_FirstUm+enumUm_ohm_per_metro));
        }

		// stampo ohm/metro spezzone misurato
		ucPrintTitleButton(hM.ucDesc_alarm ,defMeasure_Title_row+64*1,  12,enumFontBig,enumButtonMeasure_desc1,defLCD_Color_Yellow,2);
		// stampo ohm/metro spezzone ideale
		ucPrintTitleButton(hM.ucDesc_alarm3,defMeasure_Title_row+64*1,  12+6*32+8,enumFontMedium,enumButtonMeasure_desc3,defLCD_Color_Green,2);
		// stampo stringa unit� di misura "ohm/metro"
		ucPrintTitleButton(hM.ucDesc_alarm4,defMeasure_Title_row+64*1+24,defLcdWidthX_pixel-12*8-12,enumFontSmall,enumButtonMeasure_desc2,defLCD_Color_Trasparente,0);
		// solo per debug: stampo la scala...
		ucPrintTitleButton(hM.ucScala,defMeasure_Title_row+64*2+32+12,defMeasure_Scala_col,enumFontSmall,enumButtonMeasureScala,defLCD_Color_Blue,3);


	}
	else{
		float f_temp_celsius;
		if (ui_visualizza_misura_statica&&ui_get_averaged_compensation_temperature_celsius(&f_temp_celsius)){
			float f_alfa;
			float f_resistenza_compensata;
            float f_delta_perc;
			f_alfa=f_get_alfa_factor();
            f_delta_perc=f_alfa*(f_temp_celsius-20)*100;
			f_resistenza_compensata=fResistenza_Visualizzata/(1+f_alfa*(f_temp_celsius-20));
			sprintf(hM.ucDesc_alarm,"%6.*f",ui_num_decimali,f_resistenza_compensata);
			ucPrintTitleButton(hM.ucTitle,defMeasure_Title_row-24,defMeasure_Title_col,enumFontMedium,enumButtonMeasureTitle,defLCD_Color_Yellow,1);
			ucPrintTitleButton(hM.ucMeasure,defMeasure_Meas_row-32,defMeasure_Meas_col,enumFontBig,enumButtonMeasureValue,defLCD_Color_Yellow,1);
			snprintf(hM.ucDesc_alarm2,sizeof(hM.ucDesc_alarm2),"%s",pucStringLang(enumStr20_MeasuredValue));
			ucPrintTitleButton(hM.ucDesc_alarm2,defMeasure_Meas_row-32+40,defMeasure_Meas_col,enumFontMedium,enumButtonMeasureValue,defLCD_Color_Trasparente,3);
			
			ucPrintTitleButton(hM.ucDesc_alarm,defMeasure_Meas_row+40,defMeasure_Meas_col,enumFontBig,enumButtonMeasureValue,defLCD_Color_Green,1);
			snprintf(hM.ucDesc_alarm2,sizeof(hM.ucDesc_alarm2),"%s",pucStringLang(enumStr20_StandardValue_at_20C));
			ucPrintTitleButton(hM.ucDesc_alarm2,defMeasure_Meas_row+40+40,defMeasure_Meas_col,enumFontMedium,enumButtonMeasureValue,defLCD_Color_Trasparente,3);
            snprintf(hw.ucString,sizeof(hw.ucString),"T:%4.1f*C K=%7i delta=%+5.2f%%",f_temp_celsius,(unsigned int)(f_alfa*1e6),f_delta_perc);
            ucPrintSmallText_ColNames(hw.ucString,defMeasure_Meas_row+40+40+24,0);
			
			ucPrintTitleButton(hM.ucScala,defMeasure_Title_row+64*2+32+16,defMeasure_Scala_col,enumFontMedium,enumButtonMeasureScala,defLCD_Color_Blue,3);
		}
		else{
			ucPrintTitleButton(hM.ucTitle,defMeasure_Title_row,defMeasure_Title_col,enumFontMedium,enumButtonMeasureTitle,defLCD_Color_Yellow,1);
			ucPrintTitleButton(hM.ucMeasure,defMeasure_Meas_row,defMeasure_Meas_col,enumFontBig,enumButtonMeasureValue,defLCD_Color_Yellow,1);
			ucPrintTitleButton(hM.ucScala,defMeasure_Title_row+64*2+16,defMeasure_Scala_col,enumFontMedium,enumButtonMeasureScala,defLCD_Color_Blue,3);
		}
	}
	// ritorno 1 per indicare di non rappresentare a video anche lo sfondo..
	return 1;
#endif //#ifdef def_small_code
}



unsigned char ucEnableStartLavorazione(unsigned char ucEnable){
	ucStartLavorazione_enabled=ucEnable;
	return ucStartLavorazione_enabled;
}

unsigned char ucIsDisabledStartLavorazione(void){
	return ucStartLavorazione_enabled?0:1;
}
