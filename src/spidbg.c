// indico di generare src assembly
// #pragma SRC

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
#ifdef def_canopen_enable
    #include "az4186_can.h"
#endif
#include "az4186_temperature_compensation.h"
#include "az4186_initHw.h"

unsigned int ui_super_user_password_cycle;
unsigned char uc_super_user_password[32+6];
void vSetNumK_to_ask_password_super_user(unsigned int ui_super_user_password_cycle);


extern float fCalcCoeffCorr(void);
extern char *pcGetAnybusMacAddress(void);
extern void vInitBandella(void);
extern unsigned long lastRoNonCompensato;
unsigned long xdata lastRoNonCompensatoMain[2];

extern code unsigned char ucFirmwareVersion[];

extern unsigned char ucHW_AltriParametriProduzione(void);

extern xdata float fArrayConvAD7327_Volts[8];

xdata unsigned char ucBloccaVisualizzazioneMisuraStatica;
xdata unsigned char ucHoldMisuraStatica;
xdata unsigned char ucMarkMemorizeSetup;
xdata unsigned char ucCountLoopBeforeSaveSetup;
extern unsigned char uc_use_hexadecimal_letters;



// almeno defCountLoopBeforeSaveSetup loop di refresh dello schermo dopo che rampa dac stabile prima di salvare il setup
#define defCountLoopBeforeSaveSetup 7


// per setup produzione
#define defMaxCharMaggiorazione 4
#define defMinMaggiorazione 0
#define defMaxMaggiorazione 1000
code TipoStructInfoFieldCodice_Lingua im_setup_prod[3]={
	{0,defMaxCharMaggiorazione, "nnnn","%4i",defMinMaggiorazione,defMaxMaggiorazione,NULL,    1,1,enumStr20_Maggiorazione,enumUm_none,"nnnn","%4i"},
	{0,1, "n","%c",0,9,NULL,    1,1,enumStr20_tolleranza,enumUm_none,"n","%c"}
};

enumSpeedRegulationTypes xdata ucLinkEncoder;
// massimo incremento percentuale della velocit� delle ruote di contrasto
xdata int iMaxIncrementVelRuote[2]={100,100};
extern unsigned int uiLimitCommRpm(unsigned int uiRpm);



// finestra di setup produzione...
unsigned char ucHW_setupProduzione(void){

	#define defPROD_title_row 12
	#define defPROD_title_col 0

	#define defPROD_fatti_row (defPROD_title_row+32+12)
	#define defPROD_fatti_col (140)

	#define defPROD_da_fare_row (defPROD_title_row+32+12)
	#define defPROD_da_fare_col (defLcdWidthX_pixel-4*16-12-24)

	#define defPROD_button_tempo_residuo_row defPROD_da_fare_row
	#define defPROD_button_tempo_residuo_col 0

	#define defPROD_valore_nominale_row defPROD_da_fare_row
	#define defPROD_valore_nominale_col (defPROD_button_tempo_residuo_col+7*16)

	#define defPROD_valori_nominali_row (defPROD_button_tempo_residuo_row+36)
	#define defPROD_valori_nominali_col (0)

	#define defPROD_valori_reali_row (defPROD_valori_nominali_row+24)
	#define defPROD_valori_reali_col (0)

	#define defPROD_ohmm_row (defPROD_valori_reali_row+32)
	#define defPROD_ohmm_col (0)

	#define defPROD_rpm_row (defPROD_valori_reali_row+32)
	#define defPROD_rpm_col (4)

	#define defPROD_encoder_row (defPROD_valori_reali_row+32)
	#define defPROD_encoder_col (0)

	#define defPROD_CUT_row (defPROD_ohmm_row+36)
	#define defPROD_CUT_col (defLcdWidthX_pixel-72)

	#define defPROD_button_mem_row (defLcdWidthY_pixel-24-24-8)
	#define defPROD_button_mem_col (4)

	#define defPROD_button_altri_row  (defPROD_encoder_row)
	#define defPROD_button_altri_col (defPROD_button_unlock_col+4*16+40)

	#define defPROD_button_unlock_row (defPROD_button_mem_row)
	#define defPROD_button_unlock_col (defPROD_button_mem_col+4*16+48-1)
	#define defPROD_f1_col defPROD_button_unlock_col

	#define defPROD_button_tara_row  (defPROD_button_mem_row)
	#define defPROD_button_tara_col (defPROD_button_unlock_col+4*16+48)
	#define defPROD_f2_col defPROD_button_tara_col


	#define defPROD_button_prev_row  (defPROD_encoder_row)
	#define defPROD_button_prev_col (defLcdWidthX_pixel-16*2-8)


	// pulsanti della finestra
	typedef enum {
			enumPROD_pezzi_da_fare=0,
			enumPROD_TolleranzaFilo,
			enumPROD_codiceprodotto,
			enumPROD_tempo_residuo,
			enumPROD_pezzi_fatti,
			enumPROD_valori_nominali,
			enumPROD_valori_reali,
			enumPROD_Ohmm,
			enumPROD_Pretaglio_Value,enumPROD_Encoder_Spindle=enumPROD_Pretaglio_Value,
			enumPROD_Pretaglio_Up,enumPROD_Encoder_F1=enumPROD_Pretaglio_Up,
			enumPROD_Pretaglio_Dn,enumPROD_Encoder_F2=enumPROD_Pretaglio_Dn,
			enumPROD_mem,
			enumPROD_lock_unlock,
			enumPROD_tara,
			enumPROD_valore_nominale_resistenza,
			enumPROD_Rpm,

			enumPROD_Sx,
			enumPROD_Up,
			enumPROD_Cut,
			enumPROD_Esc,
			enumPROD_Dn,
			enumPROD_Dx,

			enumPROD_Title,
			enumPROD_NumOfButtons
		}enumButtons_setupProduzione;
	unsigned char i,j;
    unsigned int ui;
	static PROGRAMMA * pPro;
	static COMMESSA * pComm_static;
	static float fAux[4];
	// versione 1.51
	static unsigned long ndo_prev;
	int iRpmMandrino;
	static unsigned char ucCharMuoviPotenziometri[3];
	static unsigned char ucPretaglioPressed,ucRegolazioniAttive,ucRemember2saveActPrg;
	static unsigned char ucCtrlProduzione_PrgWasRunning,ucPressChange_regola_distanzia;
	static TipoStruct_timeout toggle_visualize_temperature_timeout;
	static unsigned char uc_visualize_temperature;

	
	PROGRAMMA saveSetupPrg;

	// inizializzo puntatori a nvram_struct.commesse e programma corrente
	pPro=&(hprg.theRunning);
	pComm_static=&nvram_struct.commesse[spiralatrice.runningCommessa];


	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			// ogni 100ms rinfresco la pagina...
            // con arm vado a 100ms
			ucLcdRefreshTimeoutEnable(200);
			switch(defGetWinParam(defWinParam_PROD_NumCampo)){
				case enumPROD_pezzi_da_fare:
					nvram_struct.commesse[spiralatrice.runningCommessa].magg=atoi(hw.ucStringNumKeypad_out);
					if (nvram_struct.commesse[spiralatrice.runningCommessa].magg+nvram_struct.commesse[spiralatrice.runningCommessa].n>MAX_QUANTITA)
						nvram_struct.commesse[spiralatrice.runningCommessa].magg=MAX_QUANTITA-nvram_struct.commesse[spiralatrice.runningCommessa].n;
					// molto importante! aggiorno lista jobs con la maggiorazione corretta!!!
					vTrasferisciDaCommesseAlistaLavori();
					break;
				case enumPROD_TolleranzaFilo:
					// salvo tutti i pars del programma attuale
					memcpy(&saveSetupPrg,&(hprg.theRunning),sizeof(saveSetupPrg));
					// il risultato della scelta ce l'ho nel parametro defIdxParam_enumWinId_Fornitori_Choice
					// della finestra enumWinId_Fornitori
					i=uiGetWindowParam(enumWinId_Fornitori,defIdxParam_enumWinId_Fornitori_Choice);
					// se vale 0xFF, non � stato cambiato il valore!!
					if (i==0xFF){
						break;
					}
					if (i<0)
						i=0;
					else if(i>9)
						i=9;
					his.ucCodice[18]=i+'0';
					his.ucCodice[19]=' ';
					his.ucCodice[20]=0;
					// reset struttura di ricerca codice prodotto
					memset(&scp,0,sizeof(scp));
				    // indico di effettuare la ricerca esatta
					mystrcpy(scp.ucString,his.ucCodice,sizeof(scp.ucString)-1);
					scp.ucFindExactCode=1;
					if (!hprg.firstPrg)
						scp.uiStartRecord=0;
					else
						scp.uiStartRecord=hprg.firstPrg;
					scp.uiNumMaxRecord2Find=1;
					// se il codice esiste, lo imposto come codice corrente
					if (ucSearchCodiceProgramma()){
						uiSetActPrg(scp.uiLastRecordFound);
					}
					// altrimenti lo devo inserire, ereditando dal programma corrente tutti i pars di regolazione!!!!
					else{
						if (InsertPrg(his.ucCodice)){
							InsertTheProgram(his.ucCodice);
							#if 0
								hprg.theAct.rpm_mandrino=saveSetupPrg.rpm_mandrino;
								hprg.theAct.empty=saveSetupPrg.empty;
								hprg.theAct.vel_produzione=saveSetupPrg.vel_produzione;
								hprg.theAct.vel_prod_vera=saveSetupPrg.vel_prod_vera;
								i=0;
								while (i<NUM_FRIZIONI){
									hprg.theAct.assorb[i]=saveSetupPrg.assorb[i];
									hprg.theAct.vruote_prg[i]=saveSetupPrg.vruote_prg[i];
								i++;
								}
								hprg.theAct.pos_pretaglio=saveSetupPrg.pos_pretaglio;
								hprg.theAct.coeff_corr=saveSetupPrg.coeff_corr;
								// alfa factor, used in formula R=R0(@20�C)*(1+alfa*(t-20�C))
								// 0 means no resistance changes with temperature
								hprg.theAct.ui_temperature_coefficient_per_million=saveSetupPrg.ui_temperature_coefficient_per_million;
							#else
								{
									char c_save_codice_prg[sizeof(hprg.theAct.codicePrg)];
									unsigned char uc_save_fornitore;
									// salvo il codice programma
									memcpy(&c_save_codice_prg[0],&hprg.theAct.codicePrg[0],sizeof(c_save_codice_prg));
									// salvo il codice fornitore
									uc_save_fornitore=hprg.theAct.fornitore;
									// copio tutta la struttura, in modo da non lasciare campi non copiati...
									memcpy(&hprg.theAct,&saveSetupPrg,sizeof(hprg.theAct));
									// ripristino il codice programma
									memcpy(&hprg.theAct.codicePrg[0],&c_save_codice_prg[0],sizeof(hprg.theAct.codicePrg));
									// ripristino il codice fornitore
									hprg.theAct.fornitore=uc_save_fornitore;
								}
							#endif
							AdjustVelPeriferica(0);
						}
						// la funzione InsertPrg() imposta gi� il programma appena inserito come programma corrente
					}
					// effettuo la validazione dei parametri programma...
					ValidatePrg();
					// salvo il programma
					ucSaveActPrg();
					// aggiorno il numero di programma cui fa riferimento il mio lotto di nvram_struct.commesse
                    i=0;
					while (i<sizeof(nvram_struct.commesse)/sizeof(nvram_struct.commesse[0])){
						nvram_struct.commesse[i].numprogramma=uiGetActPrg();
                        i++;
                    }
					// sostituisco nella joblist il codice programma attivo
					vUpdateActiveCodeInJobList(his.ucCodice);

					// reinizializzo tutto per evitare sorprese...
					vSetStatusCurrentWindow(enumWindowStatus_Initialize);
					return 2;
				case enumPROD_mem:
					// l'utente ha risposto s�, memorizza???
					if (uiGetWindowParam(enumWinId_YesNo,defWinParam_YESNO_Answer)){
						if (!PrgRunning)
							break;
						ucMarkMemorizeSetup=1;
						// abilito il refresh automatico del display!
						ucLcdRefreshTimeoutEnable(200);
					}
					break;
				// correzione alfa factor
				case 1000:
					break;

			}

			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			// reset parametro che indica cambio programma...
			ucSetWindowParam(enumWinId_setupProduzione,defWinParam_PROD_AutomaticProgramChange,0);
            i=0;
			while(i<enumPROD_NumOfButtons){
				ucCreateTheButton(i);
                i++;
            }
			// validazione numero pezzi da fare e maggiorazione
			if (nvram_struct.commesse[spiralatrice.runningCommessa].n>MAX_QUANTITA)
				nvram_struct.commesse[spiralatrice.runningCommessa].n=MAX_QUANTITA;
			if (nvram_struct.commesse[spiralatrice.runningCommessa].magg+nvram_struct.commesse[spiralatrice.runningCommessa].n>MAX_QUANTITA)
				nvram_struct.commesse[spiralatrice.runningCommessa].magg=MAX_QUANTITA-nvram_struct.commesse[spiralatrice.runningCommessa].n;
			ucRemember2saveActPrg=0;
			ucCtrlProduzione_PrgWasRunning=PrgRunning;
			
			timeout_init(&toggle_visualize_temperature_timeout);
			uc_visualize_temperature=1;


			ucMarkMemorizeSetup=0;
			ucCountLoopBeforeSaveSetup=0;
			vRefreshActiveProductCode();
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			ucRegolazioniAttive=0;
			ucPressChange_regola_distanzia=0;
			ucEnableStartLavorazione(1);
			
			if (!PrgRunning){
				// voglio essere certo di centrare con act prg il programma che � selezionato!			
				// reset struttura di ricerca codice prodotto
				memset(&scp,0,sizeof(scp));
				// indico di effettuare la ricerca esatta
				mystrcpy(scp.ucString,his.ucCodice,sizeof(scp.ucString)-1);
				scp.ucFindExactCode=1;
				if (!hprg.firstPrg)
					scp.uiStartRecord=0;
				else
					scp.uiStartRecord=hprg.firstPrg;
				scp.uiNumMaxRecord2Find=1;
				// se il codice esiste, lo imposto come codice corrente
				if (ucSearchCodiceProgramma()){
					uiSetActPrg(scp.uiLastRecordFound);
					uiSetRunningPrg(scp.uiLastRecordFound);
				}
			
				spiralatrice.AbilitaModificaPotenziometri=1;
				vDisableHRE_potentiometer();
			}
			else{
				mystrcpy(his.ucCodice,pJobsRunning->ucCodice,sizeof(his.ucCodice)-1);
				if (!spiralatrice.AbilitaModificaPotenziometri)
					SetPotenziometri();
				// altrimenti disabilito la regolazione tramite encoder...
				else
					vDisableHRE_potentiometer();
			}
			ucLinkEncoder=ucGetSelectHRE_potentiometer();
			spiralatrice.OkAbilitaTaraturaStat=0;
			ucPretaglioPressed=0;
			spiralatrice.misStaOk=0;
			ucBloccaVisualizzazioneMisuraStatica=0;
			// imposto programma corrente=running
			uiSetActPrg(uiGetRunningPrg());
			// effettuo la validazione dei parametri programma...
			ValidatePrg();
			// Piloto il coltello.
			spiralatrice.SommaRaggiMenoInterasse=(spiralatrice.pPrgRun->diametro_filo+spiralatrice.pPrgRun->diametro_mandrino+macParms.diam_coltello)/2.-macParms.iasse[0];
			// aggiusto la posizione del coltello
			if (spiralatrice.SommaRaggiMenoInterasse>spiralatrice.pPrgRun->pos_pretaglio)
				spiralatrice.actPosDacColtello=spiralatrice.SommaRaggiMenoInterasse-spiralatrice.pPrgRun->pos_pretaglio;
			else
				spiralatrice.actPosDacColtello=PercPosInizialeColtelloPre*spiralatrice.CorsaColtello;
			// aggiorno la posizione del coltello
			vAggiornaPosizioneColtello();
			// aggiorno la posizione di spaziatura del coltello
			vAggiornaPosizioneSpaziaturaColtello();
			// ogni 100ms rinfresco la pagina...
			ucLcdRefreshTimeoutEnable(200);
			// just trying to align job running and job selected...
			ucAlignJobsRunningAndSelected();
			// salvo numero attuale di pezzi prodotti
			ndo_prev=nvram_struct.commesse[spiralatrice.runningCommessa].ndo;
			return 2;
		case enumWindowStatus_Active:
			// se c'� stato un cambio automatico di programma... reinizializzo la finestra
			// per evitare sorprese...
			if (uiGetWindowParam(enumWinId_setupProduzione,defWinParam_PROD_AutomaticProgramChange)){
				vSetStatusCurrentWindow(enumWindowStatus_Initialize);
				break;
			}
			{
				extern unsigned int ui_goto_temperature_compensation_params_window;
				if(ui_goto_temperature_compensation_params_window){
					ui_goto_temperature_compensation_params_window=0;
					defSetWinParam(defWinParam_PROD_NumCampo,1000);
					ucCallWindow(enumWinId_Temperature_Compensation_Params);
					break;
				}
			}
			
			pPro=&(hprg.theRunning);
			pComm_static=&nvram_struct.commesse[spiralatrice.runningCommessa];
			// GESTIONE DELLA PRESSIONE PULSANTI SOLAMENTE SE NON E' IN CORSO SALVATAGGIO DEL SETUP!
			if (!ucMarkMemorizeSetup){
                i=0;
                while (i<enumPROD_NumOfButtons){
                    if (ucHasBeenPressedButton(i)){
                        // indico qual � il campo che sto modificando
                        // in modo che in caso di return so da dove arrivo
                        defSetWinParam(defWinParam_PROD_NumCampo,i);
                        switch(i){
                            case enumPROD_Sx:
                                // devo salvare il programma attuale?
                                if (ucRemember2saveActPrg){
                                    ucSaveActPrg();
                                    ucRemember2saveActPrg=0;
                                }
                                ucLcdRefreshTimeoutEnable(0);
                                ucBloccaVisualizzazioneMisuraStatica=0;
                                // se programma attualmente caricato in esecuzione diverso da ultimo selezionato, tornando indietro
                                // con freccia a sinistra vado nella pagina di selezione codice...
                                // per evitare fraintendimenti
                                if (strcmp(pJobsRunning->ucCodice,nvram_struct.codelist.codeJobList[nvram_struct.codelist.ucIdxActiveElem].ucCodice)){
                                    vJumpToWindow(enumWinId_listaCodici);
                                }
                                else{
                                    if (nvram_struct.ucComingToControlloProduzioneFromJobs==0){
                                        // se prg running e provengo da lotto manuale, non posso andare a modificarlo!!!!
                                        if (!PrgRunning)
                                            vJumpToWindow(enumWinId_LottoDiretto);
                                    }
                                    else
                                        vJumpToWindow(enumWinId_inserisciOhmMfili);
                                }
                                return 2;
                            // se faccio click sul titolo, posso scegliere dove andare...
                            case enumPROD_Title:
                            case enumPROD_Esc:
                                ucBloccaVisualizzazioneMisuraStatica=0;
                                //sblocco le operazioni lama...
                                vUnlockOperazioneLama();
                                // alzo il coltello, se era gi�...
                                if (ucPretaglioPressed){
                                    vAttivaOperazioneLama(enumCiclicaLama_sali);
                                }							
                                ucLcdRefreshTimeoutEnable(0);
                                // chiamo il main menu...
                                vJumpToWindow(enumWinId_MainMenu);
                                // devo salvare il programma attuale?
                                if (ucRemember2saveActPrg){
                                    ucSaveActPrg();
                                    ucRemember2saveActPrg=0;
                                }
                                // indico di rinfrescare la finestra
                                return 2;
                            // selezionata tolleranza filo???
                            case enumPROD_TolleranzaFilo:
                                // indico che la tabella NON pu� essere modificata
                                ucSetWindowParam(enumWinId_Fornitori,defIdxParam_enumWinId_Fornitori_ModifyEnable,0);
                                // chiamo la finestra fornitori
                                ucCallWindow(enumWinId_Fornitori);
                                return 2;

                            // inserisco il numero di pezzi per la maggiorazione
                            case enumPROD_pezzi_da_fare:
                                j=i;
                                paramsNumK.ucMaxNumChar=im_setup_prod[j].ucLenField;
                                // imposto stringa di picture
                                mystrcpy((char*)paramsNumK.ucPicture,(char*)im_setup_prod[j].ucPictField,sizeof(paramsNumK.ucPicture)-1);
                                paramsNumK.enumUm=im_setup_prod[j].enumUm;

                                // imposto limiti min e max
                                paramsNumK.fMin=im_setup_prod[j].fMin;
                                paramsNumK.fMax=im_setup_prod[j].fMax;
                                //strcpy(paramsNumK.ucTitle,im_setup_prod[j].ucTitleNumK);
                                vStringLangCopy(paramsNumK.ucTitle,im_setup_prod[i].stringa);
                                if (i==enumPROD_pezzi_da_fare)
                                    // copio la stringa con cui si inizializza il numeric keypad
                                    sprintf(hw.ucStringNumKeypad_in,im_setup_prod[j].ucFmtField,nvram_struct.commesse[spiralatrice.runningCommessa].magg);
                                else  //enumPROD_TolleranzaFilo --> sempre fra '0' e '9'
                                    // copio la stringa con cui si inizializza il numeric keypad
                                    sprintf(hw.ucStringNumKeypad_in,im_setup_prod[j].ucFmtField,his.ucCodice[18]);
                                // imposto abilitazione uso stringa picture
                                paramsNumK.ucPictureEnable=im_setup_prod[j].ucPictureEnable;	// abilitazione stringa picture
                                // imposto abilitazione uso min/max
                                paramsNumK.ucMinMaxEnable=im_setup_prod[j].ucMinMaxEnable;	// abilitazione controllo valore minimo/massimo
                                ucLcdRefreshTimeoutEnable(0);
                                // chiamo il numeric keypad
                                ucCallWindow(enumWinId_Num_Keypad);
                                // indico di rinfrescare la finestra
                                return 2;
                            case enumPROD_lock_unlock:
                                spiralatrice.AbilitaModificaPotenziometri=!spiralatrice.AbilitaModificaPotenziometri;
                                // Se ho tolto il lucchetto ai potenziometri, determino
                                // la condizione sotto la quale si attivano.
                                if (!spiralatrice.AbilitaModificaPotenziometri){
                                    SetPotenziometri();
                                }
                                // altrimenti disabilito la regolazione tramite encoder...
                                else{
                                    // posso adesso andare veloce...
                                    spiralatrice.slowRpmMan=noSlowRpmMan;
                                    vDisableHRE_potentiometer();
                                }
                                break;
                            // accedo al menu di impostazione dei parametri: diametro coltello
                            // diametro ruota 1 e 2, tolleranza fili, numero di spire di pretaglio
                            case enumPROD_Dn:
                            case enumPROD_Dx:
                                // devo salvare il programma attuale?
                                if (ucRemember2saveActPrg){
                                    ucSaveActPrg();
                                    ucRemember2saveActPrg=0;
                                }
                                vUnlockOperazioneLama();
                                ucLcdRefreshTimeoutEnable(0);
                                ucCallWindow(enumWinId_AltriParametriProduzione);
                                return 2;
                            case enumPROD_Cut:
                                vDoTaglio();
                                // non occorre rinfrescare la finestra
                                break;
                            case enumPROD_tara:
                                // se commessa esaurita, non posso tarare...
                                if (nvram_struct.commesse[spiralatrice.runningCommessa].ndo>=nvram_struct.commesse[spiralatrice.runningCommessa].n+nvram_struct.commesse[spiralatrice.runningCommessa].magg){
                                    break;
                                }
                                if (spiralatrice.OkAbilitaTaraturaStat){
                                    spiralatrice.OkAbilitaTaraturaStat=0;
                                    spiralatrice.misStaOk=0;
                                }
                                else{
                                    // indico che sto effettuando una taratura...
                                    spiralatrice.OkAbilitaTaraturaStat=1;
                                    spiralatrice.OkMisStatica=1;
                                    spiralatrice.misStaOk=0;
                                    ucBloccaVisualizzazioneMisuraStatica=0;
                                }
                                break;
                            case enumPROD_Pretaglio_Up:
                                if (ucRegolazioniAttive){
                                    if (!PrgRunning){
                                        break;
                                    }
                                    ucLinkEncoder=enumSpeedRegulation_Wheel1;
                                    vSelectHRE_potentiometer(ucLinkEncoder);
                                }
                                else{
                                    // se prg running, posso aggiustare la posizione di pretaglio!
                                    // se prg non running, devo prima premere il pulsante per poter aggiustare!!!
                                    if (!PrgRunning&&!ucPretaglioPressed)
                                        break;
                                    if (ucPretaglioPressed==2){
                                        if (spiralatrice.f_pos_coltello_spaziatura_mm<spiralatrice.CorsaColtello){
                                            // Alzo il coltello di due bit per ottenere circa
                                            // 0.1 millimetri=2*11/256.
                                            spiralatrice.f_pos_coltello_spaziatura_mm+=2*spiralatrice.CorsaColtello/FS_DAC_MAX519;
                                            if (spiralatrice.f_pos_coltello_spaziatura_mm>spiralatrice.CorsaColtello)
                                                spiralatrice.f_pos_coltello_spaziatura_mm=spiralatrice.CorsaColtello;
                                            // Faccio ristampare il valore ed aggiornare la posizione
                                            // del coltello.
                                            hprg.ptheAct->f_coltello_pos_spaziatura_mm=spiralatrice.f_raggio_coltello_meno_interasse-spiralatrice.f_pos_coltello_spaziatura_mm;
                                            // non salvo subito il programma, altrimenti introduco una pausa...
                                            ucRemember2saveActPrg=1;
                                            // aggiorno i parametri di impostazione distanziazione...
                                            vAggiornaPosizioneSpaziaturaColtello();
                                            //ucSaveActPrg();
                                            if (!PrgRunning){
                                                // aggiorno dac che comanda la posizione del coltello
                                                vAttivaOperazioneLama(enumCiclicaLama_distanzia);
                                            }
                                        }
                                    }
                                    else{
                                        if (spiralatrice.actPosDacColtello<spiralatrice.CorsaColtello){
                                            // Alzo il coltello di due bit per ottenere circa
                                            // 0.1 millimetri=2*11/256.
                                            spiralatrice.actPosDacColtello+=2*spiralatrice.CorsaColtello/FS_DAC_MAX519;
                                            if (spiralatrice.actPosDacColtello>spiralatrice.CorsaColtello)
                                                spiralatrice.actPosDacColtello=spiralatrice.CorsaColtello;
                                            // Faccio ristampare il valore ed aggiornare la posizione
                                            // del coltello.
                                            hprg.ptheAct->pos_pretaglio=spiralatrice.SommaRaggiMenoInterasse-spiralatrice.actPosDacColtello;
                                            // non salvo subito il programma, altrimenti introduco una pausa...
                                            ucRemember2saveActPrg=1;
                                            //ucSaveActPrg();
                                            // aggiorno dac che comanda la posizione del coltello
                                            vAggiornaPosizioneColtello();
                                            if (!PrgRunning)
                                                vAttivaOperazioneLama(enumCiclicaLama_pretaglia);
                                        }
                                    }
                                }
                                break;
                            case enumPROD_Pretaglio_Dn:
                                if (ucRegolazioniAttive){
                                    if (!PrgRunning){
                                        break;
                                    }
                                    ucLinkEncoder=enumSpeedRegulation_Wheel2;
                                    vSelectHRE_potentiometer(ucLinkEncoder);
                                }
                                else{
                                    // se prg running, posso aggiustare la posizione di pretaglio!
                                    // se prg non running, devo prima premere il pulsante per poter aggiustare!!!
                                    if (!PrgRunning&&!ucPretaglioPressed)
                                        break;
                                    if (ucPretaglioPressed==2){
                                        if (spiralatrice.f_pos_coltello_spaziatura_mm>2*spiralatrice.CorsaColtello*(1.0/FS_DAC_MAX519)+0.01){
                                            // Abbasso il coltello di 2 bit per ottenere
                                            // 0.1 millimetri.
                                            spiralatrice.f_pos_coltello_spaziatura_mm-=2*spiralatrice.CorsaColtello*(1.0/FS_DAC_MAX519);
                                            // Faccio ristampare il valore ed aggiornare la posizione
                                            // del coltello.
                                            hprg.ptheAct->f_coltello_pos_spaziatura_mm=spiralatrice.f_raggio_coltello_meno_interasse-spiralatrice.f_pos_coltello_spaziatura_mm;
                                            // non salvo subito il programma, altrimenti introduco una pausa...
                                            ucRemember2saveActPrg=1;
                                            // aggiorno i parametri di impostazione distanziazione...
                                            vAggiornaPosizioneSpaziaturaColtello();
                                            //ucSaveActPrg();
                                            if (!PrgRunning){
                                                vAttivaOperazioneLama(enumCiclicaLama_distanzia);
                                            }
                                        }
                                    }
                                    else if (spiralatrice.actPosDacColtello>2*spiralatrice.CorsaColtello*(1.0/FS_DAC_MAX519)+0.01){
                                        // Abbasso il coltello di 2 bit per ottenere
                                        // 0.1 millimetri.
                                        spiralatrice.actPosDacColtello-=2*spiralatrice.CorsaColtello*(1.0/FS_DAC_MAX519);
                                        if (spiralatrice.actPosDacColtello>spiralatrice.CorsaColtello)
                                            spiralatrice.actPosDacColtello=spiralatrice.CorsaColtello;
                                        // Faccio ristampare il valore ed aggiornare la posizione
                                        // del coltello.
                                        hprg.ptheAct->pos_pretaglio=spiralatrice.SommaRaggiMenoInterasse-spiralatrice.actPosDacColtello;
                                        // non salvo subito il programma, altrimenti introduco una pausa...
                                        ucRemember2saveActPrg=1;
                                        //ucSaveActPrg();
                                        // aggiorno dac che comanda la posizione del coltello
                                        vAggiornaPosizioneColtello();
                                        if (!PrgRunning)
                                            vAttivaOperazioneLama(enumCiclicaLama_pretaglia);
                                    }
                                }
                                break;
                            case enumPROD_Pretaglio_Value:
                                if (ucRegolazioniAttive){
                                    if (!PrgRunning){
                                        break;
                                    }
                                    ucLinkEncoder=enumSpeedRegulation_Spindle;
                                    vSelectHRE_potentiometer(ucLinkEncoder);
                                }
                                else{
                                    ucPretaglioPressed=!ucPretaglioPressed;
                                    if (!ucPretaglioPressed){
                                        vUnlockOperazioneLama();
                                        // devo salvare il programma???
                                        if (ucRemember2saveActPrg){
											// if prg running make sure that the actual program points to the running one!
											if (PrgRunning){
												uiSetActPrg(uiGetRunningPrg());
											}
										
                                            ucSaveActPrg();
                                            ucRemember2saveActPrg=0;
                                        }
                                    }
                                    // se prg running, non comando mai il pretaglio da pulsante!!!!
                                    if (PrgRunning){
                                        break;
                                    }
                                    // solo se prg non running, mi porto alla posizione di pretaglio!!!
                                    if (!ucPretaglioPressed){
                                        vAttivaOperazioneLama(enumCiclicaLama_sali);
                                    }
                                    else{
                                        vAttivaOperazioneLama(enumCiclicaLama_pretaglia);
                                    }
                                    if (!criticLav&&!TaglioAsincrono){
                                        // Se comando di pretaglio attivo, lo disattivo
                                        if (outDigVariable&ODG_PRETAGLIO){
                                            outDigVariable&=~ODG_PRETAGLIO;
                                        }
                                        else{
                                            // Attivo il comando di pretaglio. */
                                            outDigVariable|=ODG_PRETAGLIO;
                                        }
                                    }
                                }
                                break;
                            case enumPROD_mem:
                                if (!PrgRunning)
                                    break;
                                ucLcdRefreshTimeoutEnable(0);
                                // chiedo conferma
                                //strcpy(hw.ucStringNumKeypad_in,"Memorizzo setup?");
                                vStringLangCopy(hw.ucStringNumKeypad_in,enumStr20_StoreSetup);
                                hw.ucStringNumKeypad_out[0]=0;
                                // impostazione del parametro 0 al valore 0--> visualizza pulsanti yes e no
                                ucSetWindowParam(enumWinId_YesNo,def_WinYesNo_Param_TipoPulsante, def_WinYesNo_PUlsantiYesNo);
                                // visualizzo yes/no...
                                ucCallWindow(enumWinId_YesNo);
                                return 2;

                        }// switch
                    }// if
                    i++;
                }// while
			}// if

			 pComm_static->assorb_frizioni[0]=(float)InputAnalog[N_CANALE_ASSORB1]/FS_ADC_BIPOLARI*macParms.fFSassorbRuote;
			 // 1.28 DEVO VISUALIZZARE ANCHE ASSORBIMENTI NEGATIVI
			 //if (pComm->assorb_frizioni[0]<0)
			 //	 pComm->assorb_frizioni[0]=0;
			 pComm_static->assorb_frizioni[1]=(float)InputAnalog[N_CANALE_ASSORB2]/FS_ADC_BIPOLARI*macParms.fFSassorbRuote;
			 // 1.28 DEVO VISUALIZZARE ANCHE ASSORBIMENTI NEGATIVI
			 //if (spiralatrice.pComm->assorb_frizioni[1]<0)
			 //	 spiralatrice.pComm->assorb_frizioni[1]=0;
			 if (macParms.uc_1Encoder_0Potenziometri==1){
                 long l_aux;
				 l_aux=macParms.rpm_mandrino;
				 l_aux=l_aux*iGetHRE_potentiometer(enumSpeedRegulation_Spindle);
				 l_aux=l_aux/FS_ADC;
				 pComm_static->rpm=l_aux;
                 fAux[0]=l_aux;
				 pComm_static->vruote_comm[0]=(unsigned int)iGetHRE_potentiometer(enumSpeedRegulation_Wheel1);
				 pComm_static->vruote_comm[1]=(unsigned int)iGetHRE_potentiometer(enumSpeedRegulation_Wheel2);

			 }
			 else{
				 long l_aux;
                 l_aux=macParms.rpm_mandrino;
				 l_aux=l_aux*InputAnalog[N_CANALE_V_MAN];
				 l_aux=l_aux/FS_ADC;
				 pComm_static->rpm=l_aux;
                 fAux[0]=l_aux;
				 pComm_static->vruote_comm[0]=(unsigned int)InputAnalog[N_CANALE_V1];
				 pComm_static->vruote_comm[1]=(unsigned int)InputAnalog[N_CANALE_V2];
			 }

			 // verifico se supero i limiti...
			 if (((long)pComm_static->vruote_comm[0]*2-FS_ADC)/FS_ADC*100>iMaxIncrementVelRuote[0]){
				pComm_static->vruote_comm[0]=FS_ADC/2+((long)iMaxIncrementVelRuote[0]*FS_ADC)/200;
				// imposto encoder ruota in modo da limitare numero di giri...
				vSetHRE_potentiometer(enumSpeedRegulation_Wheel1,pComm_static->vruote_comm[0]);
				if (ucLinkEncoder==enumSpeedRegulation_Wheel1)
					// devo anche selezionare il potenziometro per posizionare correttamente la lettura
					vSelectHRE_potentiometer(enumSpeedRegulation_Wheel1);
			 }
			 if (((long)pComm_static->vruote_comm[1]*2-FS_ADC)/FS_ADC*100>iMaxIncrementVelRuote[1]){
				pComm_static->vruote_comm[1]=FS_ADC/2+((long)iMaxIncrementVelRuote[1]*FS_ADC)/200;
				// imposto encoder ruota in modo da limitare numero di giri...
				vSetHRE_potentiometer(enumSpeedRegulation_Wheel2,pComm_static->vruote_comm[1]);
				if (ucLinkEncoder==enumSpeedRegulation_Wheel2)
					// devo anche selezionare il potenziometro per posizionare correttamente la lettura
					vSelectHRE_potentiometer(enumSpeedRegulation_Wheel2);
			 }


			 if (!spiralatrice.AbilitaModificaPotenziometri&&!PrgRunning){
				spiralatrice.AbilitaModificaPotenziometri=1;
				vDisableHRE_potentiometer();
			  }


		     // Aggiorno solo se la rampa � esaurita e programma running!!!!
		     // e soprattutto se non devo salvare il setup!!!

			 if (  PrgRunning
			     &&(spiralatrice.StatusRampaDac==0)
				  &&(!ucMarkMemorizeSetup)
			     ){

				 /* Se il potenziometro del mandrino deve essere abilitato... */
				 if (spiralatrice.CheckPotMan&&!spiralatrice.AbilitaModificaPotenziometri){
					if (spiralatrice.slowRpmMan==noSlowRpmMan){
						/* Se sto aspettando che l' utente ne aumenti il valore, faccio il controllo. */
						if (spiralatrice.CheckPotMan==CheckPotMaggioreUguale){
							if (pComm_static->rpm*IsteresiUpPot>=hprg.theRunning.rpm_mandrino){
								spiralatrice.CheckPotMan=0;
							}
						}
						else if (pComm_static->rpm*IsteresiDnPot<=hprg.theRunning.rpm_mandrino){
							spiralatrice.CheckPotMan=0;
						}
					}
					/* Se sto partendo lento, il termine di confronto deve essere il numero di giri basso. */
					else{
						if (spiralatrice.CheckPotMan==CheckPotMaggioreUguale){
							if (pComm_static->rpm*IsteresiUpPot>=RPM_MAND_PRG_EMPTY){
								spiralatrice.CheckPotMan=0;
							}
						}
						else if (pComm_static->rpm*IsteresiDnPot<=RPM_MAND_PRG_EMPTY){
							spiralatrice.CheckPotMan=0;
						}
					}
				 }

				 if (!spiralatrice.AbilitaModificaPotenziometri){
                    i=0;
					while (i<NUM_FRIZIONI){
						/* Se il potenziometro del mandrino deve essere abilitato... */
						if (spiralatrice.CheckPotVel[i]){
							/* Se sto aspettando che l' utente ne aumenti il valore, faccio il controllo. */
							if (spiralatrice.CheckPotVel[i]==CheckPotMaggioreUguale){
								if (pComm_static->vruote_comm[i]*IsteresiUpPot>=hprg.theRunning.vruote_prg[i]){
									spiralatrice.CheckPotVel[i]=0;
								}
							}
							else if (pComm_static->vruote_comm[i]*IsteresiDnPot<=hprg.theRunning.vruote_prg[i]){
								spiralatrice.CheckPotVel[i]=0;
							}
						}
                        i++;
					}
				 }
				 /* Indico di partire dal primo gradino della rampa. */
				 spiralatrice.GradinoRampaDAC=0;
				 /* Se quello che mi d� il potenziometro � oro colato, spedisco il comando al mandrino. */
				 if (!(spiralatrice.CheckPotMan||spiralatrice.AbilitaModificaPotenziometri)){
					/* Se stavo partendo da lento, una volta raggiunta la velocita' di programma lucchetto
						i potenziometri e mi fermo alla velocita' programmata.	*/
					if (spiralatrice.slowRpmMan!=noSlowRpmMan){
						/* Fine partenza da lento. */
						spiralatrice.slowRpmMan=noSlowRpmMan;
						/* Imposto l' indicatore che avvisa di fermarsi alla velocita' programmata
						   (se il programma non e' vuoto).
						*/
						if (!hprg.theRunning.empty){
							// se sono gi� alla v di regime, non devo fare clip
							if (pComm_static->rpm*IsteresiUpPot<hprg.theRunning.rpm_mandrino)
								spiralatrice.ucClipVelocity=1;
						}
					}
					if (  spiralatrice.ucClipVelocity&&(pComm_static->rpm*IsteresiUpPot>=hprg.theRunning.rpm_mandrino)){
						/* Resetto il flag di clip velocita'. */
						spiralatrice.ucClipVelocity=0;
						/* Imposto il dac al valore di programma. */
						spiralatrice.DacOutValue[0]=hprg.theRunning.rpm_mandrino;
						/* regolo le frizioni sulla velocita' di programma. */
						AdjustVelPeriferica(0);
						/* Lucchetto ai potenziometri. */
						spiralatrice.AbilitaModificaPotenziometri=1;
						vDisableHRE_potentiometer();
					}
					else{
						// limito il numero di giri... solo se necessario per�
						 pComm_static->rpm=uiLimitCommRpm(pComm_static->rpm);
					
						/* Calcolo in spiralatrice.DacOutValue[0] la velocit� angolare del motore. */
						spiralatrice.DacOutValue[0]=pComm_static->rpm;
						/* Ricalcolo la velocit� delle frizioni e in metri/minuto.
							Uso la velocita' del potenziometro
							per calcolare quella delle ruote.
						*/
						AdjustVelPeriferica(2);
					}
				 }
				 else if (spiralatrice.slowRpmMan==noSlowRpmMan){

					/* Calcolo in spiralatrice.DacOutValue[0] la velocit� angolare del motore. */
					spiralatrice.DacOutValue[0]=hprg.theRunning.rpm_mandrino;
					/* Correggo la velocita' delle ruote di contrasto. */
					AdjustVelPeriferica(0);
				 }
				 /* Partenza lenta. */
				 else {

					/* Calcolo in spiralatrice.DacOutValue[0] la velocit� angolare del motore. */
					spiralatrice.DacOutValue[0]=RPM_MAND_PRG_EMPTY;
				 }
				 /* Fisso il valore massimo degli rpm del mandrino. */
				 spiralatrice.DacOutMaxValue[0]=macParms.rpm_mandrino;
				 /* La coppia del mandrino non va controllata. */
				 spiralatrice.DacOutValue[1]=1;
				 spiralatrice.DacOutMaxValue[1]=1;

				 SetDacValue(addrDacMandrino);

				 /* Preparazione delle rampe dei dac delle ruote di contrasto. */
                 i=0;
				 while (i<NUM_FRIZIONI){
				  /* Velocit� in rpm della ruota 1 e 2. */
				  /* Se i potenziometri sono abilitati, allora prendo per buono il valore dato da essi. */
					if (!(spiralatrice.CheckPotVel[i]||spiralatrice.AbilitaModificaPotenziometri))
						spiralatrice.DacOutValue[0]=pComm_static->vruote_comm[i];
					else
						/* Altrimenti uso il valore memorizzato nel programma. */
						spiralatrice.DacOutValue[0]=hprg.theRunning.vruote_prg[i];
					/* Il valore della velocit� lo ottengo da quello nominale
						(che dipende dagli rpm del mandrino) modificandolo
						secondo quanto stabilito dal potenziometro. */
					spiralatrice.DacOutValue[0]=(((long)spiralatrice.DacOutValue[0])*2L*hprg.theRunning.vel_frizioni[i])/FS_ADC;
					/* Valore massimo della velocit� della routa. */
					spiralatrice.DacOutMaxValue[0]=macParms.rpm_frizioni[i];
					/* La coppia non viene gestita, � indifferente. */
					spiralatrice.DacOutValue[1]=1;
					spiralatrice.DacOutMaxValue[1]=1;
					/* Chiamo la routine con l' indirizzo corretto. */
					SetDacValue(addrDacFrizione1+i);
                    i++;
				}

				ucCharMuoviPotenziometri[0]=' ';
				ucCharMuoviPotenziometri[1]=' ';
				ucCharMuoviPotenziometri[2]=' ';
				if (spiralatrice.AbilitaModificaPotenziometri){
					/* Potenziometri col lucchetto. */
					//edit.actLcdStr[1][COL_TIT_COM_OHM+2]='L';
				}
				else{
					if (spiralatrice.CheckPotMan==CheckPotMaggioreUguale){
						/* Aumenta potenziometro. */
						ucCharMuoviPotenziometri[0]=CarattereAumenta;
					}
					else if (spiralatrice.CheckPotMan==CheckPotMinoreUguale){
						/* Aumenta potenziometro. */
						ucCharMuoviPotenziometri[0]=CarattereDiminuisci;
					}
					else
						ucCharMuoviPotenziometri[0]=' ';
                    i=0;
					while (i<NUM_FRIZIONI){
						if (spiralatrice.CheckPotVel[i]==CheckPotMaggioreUguale){
							/* Aumenta potenziometro. */
							ucCharMuoviPotenziometri[1+i]=CarattereAumenta;
						}
						else if (spiralatrice.CheckPotVel[i]==CheckPotMinoreUguale){
							/* Aumenta potenziometro. */
							ucCharMuoviPotenziometri[1+i]=CarattereDiminuisci;
						}
                        i++;
					}
				}

			}//if (spiralatrice.StatusRampaDac==0){

			// if (  spiralatrice.ucClipVelocity&&(pComm_static->rpm*IsteresiUpPot>=hprg.theRunning.rpm_mandrino)){
				// spiralatrice.ucClipVelocity=0;
			// }
			if (!PrgRunning){
				ucMarkMemorizeSetup=0;
			}
			// se programma viene stoppato, salvo regolazione lama di taglio, se necessario,
			// e tolgo stato di pressed a pulsante regolazione pretaglio
			if (!PrgRunning&&ucCtrlProduzione_PrgWasRunning){
				// devo salvare il programma???
				if (ucRemember2saveActPrg){
					ucSaveActPrg();
					ucRemember2saveActPrg=0;
				}
				if (ucPretaglioPressed){
					vAttivaOperazioneLama(enumCiclicaLama_sali);
					ucPretaglioPressed=0;
				}
			}
			ucCtrlProduzione_PrgWasRunning=PrgRunning;
			
			if (!ucMarkMemorizeSetup){
				ucCountLoopBeforeSaveSetup=0;
				if (!PrgRunning&&macroIsCodiceProdotto_TemperatureCompensated(&hprg.theRunning)&&!ui_temperature_compensation_warmup_finished()){
					sprintf(hw.ucString,"%-20.20s",pucStringLang(enumStr20_WaitWarmup));
					// 1.42 cambio lo sfondo in magenta per visualizzare meglio la stringa "salvataggio"
					ucPrintTitleButton(hw.ucString,defPROD_valori_nominali_row+8,defPROD_valori_nominali_col,enumFontMedium,enumPROD_pezzi_da_fare,defLCD_Color_Magenta,1);
				}
			}
			// devo memorizzare il setup????
			else {
				sprintf(hw.ucString,"%-20.20s",pucStringLang(enumStr20_Saving));
				// 1.42 cambio lo sfondo in magenta per visualizzare meglio la stringa "salvataggio"
				ucPrintTitleButton(hw.ucString,defPROD_valori_nominali_row+8,defPROD_valori_nominali_col,enumFontMedium,enumPROD_pezzi_da_fare,defLCD_Color_Magenta,1);

				// MEMORIZZO IL SETUP SOLO QUANDO SONO SICURO CHE LO STATO SIA STABILE!
				// Quindi un tot di secondi dopo che la rampa dac � esaurita!
				if (ucCountLoopBeforeSaveSetup<defCountLoopBeforeSaveSetup){
					// meorizzo il setup solo dopo che rampa dac stabile per almeno tot secondi...
					if (spiralatrice.StatusRampaDac==0)
						ucCountLoopBeforeSaveSetup++;
				}
				else{
					ucMarkMemorizeSetup=0;
					ucCountLoopBeforeSaveSetup=0;
					// Se il potenziometro del mandrino e' sbloccato ed abilitato, salvo il valore.
					if (!(spiralatrice.CheckPotMan||spiralatrice.AbilitaModificaPotenziometri))
						pPro->rpm_mandrino=pComm_static->rpm;
					// indico di ripetere la procedura di elimina primo pezzo, se prg era vuoto
					if (pPro->empty)
						spiralatrice.PrimoPezzoEliminato=0;
					// Salvo il valore "vero" della velocita' di produzione.
					pPro->vel_prod_vera=velMMin;
                    i=0;
					while (i<NUM_FRIZIONI){
						// Velocit� in rpm della ruota 1 e 2.
						// Se i potenziometri sono abilitati, allora prendo
						//	per buono il valore dato da essi.
						if (!(spiralatrice.CheckPotVel[i]||spiralatrice.AbilitaModificaPotenziometri)){
							pPro->vruote_prg[i]=pComm_static->vruote_comm[i];
						}
						pPro->assorb[i]=pComm_static->assorb_frizioni[i];
						// Altrimenti lascio il default, che corrisponde alla
						//		velocit� nominale calcolata.
                        i++;
					}
					AdjustVelPeriferica(0);
					// Se il programma necessitava di memorizzazione, adesso
					//	viene azzerato il contapezzi.
					if (pPro->empty){
						spiralatrice.NumPezziScarto=actNumPezzi;
						// Se la commessa era gia' stata lavorata
						// con un altro programma, ritorno al numero di
						// pezzi con cui ero partito, oppure torno a zero. */
						nvram_struct.commesse[spiralatrice.runningCommessa].ndo=spiralatrice.oldndo;
						// Il contapezzi vale anch' esso spiralatrice.oldndo. */
						actNumPezzi=spiralatrice.oldndo;
					}
					// Indico che adesso il programma contiene dei parametri
					//	validi. A questo punto verr� tagliato il pezzo
					//	in esecuzione e partir� la produzione normale.
					pPro->empty=0;
					// salvo il programma
					ucSaveRunningPrg();
					// Spengo la lampada. */
					TempoLampada=0;
					outDigVariable &= ~ODG_LAMPADA;
					LampStatus=LampSpenta;
					// Blocco i potenziometri. */
					spiralatrice.AbilitaModificaPotenziometri=1;
					vDisableHRE_potentiometer();


				}
			}



		     /* Se misura statica attiva, calcolo coefficiente di correzione. */
		     if (	spiralatrice.misStaOk
					// &&spiralatrice.OkMisStatica
					&&(spiralatrice.OkAbilitaTaraturaStat||spiralatrice.OkAbilitaTaraFineStat)
		        ){
				// se la taratura � finita, ed il pulsante � ancora premuto,
				// devo bloccare la visualizzazione della finestra della misura statica
				if (myInpDig & IDG_MSTA){
					ucBloccaVisualizzazioneMisuraStatica=1;
				}
				else
					ucBloccaVisualizzazioneMisuraStatica=0;
				// libero l'uscita di attivazione misura statica
				ucHoldMisuraStatica=0;

				//spiralatrice.OkMisStatica=0;
				if (spiralatrice.OkAbilitaTaraturaStat){
					spiralatrice.OkAbilitaTaraturaStat=0;
					// versione 0.5
					if (spiralatrice.resStaMediata<1e30){
						float f_resistance_temperature_compensated;
						// if available and enabled, use the temperature-compensated value for the resistance measured
						if (ui_get_temperature_compensated_resistance(spiralatrice.resStaMediata,&f_resistance_temperature_compensated)){
							hprg.theRunning.coeff_corr=hprg.theRunning.coeff_corr*(f_resistance_temperature_compensated/pComm_static->res);
						}
						else{
							hprg.theRunning.coeff_corr=hprg.theRunning.coeff_corr*(spiralatrice.resStaMediata/pComm_static->res);
						}
						// salvo il programma
						ucSaveRunningPrg();
					}

					/* Salvo il valore della resistenza mediata calcolata. */
					spiralatrice.oldresStaMediata=spiralatrice.resStaMediata;
				}
				else if (spiralatrice.OkAbilitaTaraFineStat){
					spiralatrice.OkAbilitaTaraFineStat=0;
					/* Uso il valore precedentemente salvato della resistenza mediata calcolata.	*/
					// versione 0.5
					if ((spiralatrice.oldresStaMediata<1e30)&&(spiralatrice.resStaMediata<1e30)){
						fResDeltaR=spiralatrice.resStaMediata-spiralatrice.oldresStaMediata;
						if (fResDeltaR<0)
							fResDeltaR=0;
					}
				}
				if (withMis){
					/* Il numero di impulsi di anticipo dell' azione di pretaglio
					   va scalato in base al fattore correttivo.
					*/
					//anticipoPretaglio/=fCalcCoeffCorr();
					// V1.29--> aniticipo di pretaglio lo ricalcolo da ZERO...
					vCalcAnticipoPretaglio();
					actCorrOhmica=fCalcCoeffCorr()*1024.0;
				}
				/* Se sono in modalit� L, ... */
				else {
					/* Calcolo il numero di impulsi che devo ricevere per poter dire
					di aver completato il pezzo. */
					targetLenImp = (long)((pComm_static->res/fCalcCoeffCorr())/pComm_static->resspec_spir *1000./ impToMm  + ROUND);
					if (anticipoPretaglio>targetLenImp)
						anticipoPretaglio=(unsigned short)(9*targetLenImp/10);
					preLenImp = targetLenImp - anticipoPretaglio; /* per far partire il comando di pretaglio */
				}
				spiralatrice.misStaOk=0;
		     }
			// se numero attuale di pezzi prodotti � cambiato, aggiorno lista jobs!
			 if (ndo_prev!=nvram_struct.commesse[spiralatrice.runningCommessa].ndo){
				vTrasferisciDaCommesseAlistaLavori();
			 }


			// "FILO        LEGA      MANDRINO  NF  TOLL",
			vString40LangCopy(hw.ucString,enumStr40_codefields);
			ucPrintSmallText_ColNames(hw.ucString,defPROD_title_row-12,defPROD_title_col);
			if (!PrgRunning){
				sprintf(hw.ucString,"%c",his.ucCodice[18]);
				ucPrintStaticButton(hw.ucString,defPROD_title_row,4+18U*16,enumFontMedium,enumPROD_TolleranzaFilo,defLCD_Color_Trasparente);
				sprintf(hw.ucString,"%18.18s",his.ucCodice);
				ucPrintTitleButton(hw.ucString,defPROD_title_row,4,enumFontMedium,enumPROD_Title,defLCD_Color_Trasparente,0);
			}
			else{
				sprintf(hw.ucString,"%19.19s",his.ucCodice);
				ucPrintTitleButton(hw.ucString,defPROD_title_row,defPROD_title_col,enumFontMedium,enumPROD_Title,defLCD_Color_Trasparente,1);
			}

			sprintf(hw.ucString,"%s: %20.20s",pucStringLang(enumStr20_Lotto),nvram_struct.commesse[spiralatrice.runningCommessa].commessa);
#if 0			
			// visualizzo anche la temperatura...
			if (macParms.ucAbilitaCompensazioneTemperatura&&macroIsCodiceProdotto_TemperatureCompensated(&hprg.theRunning)){
				int i_temperature_binary;
				float f_temperature_celsius;
				// prelevo la temperatura misurata dalla termocoppia collegata la modulo phoenix
				ui_get_canopen_temperature(&i_temperature_binary);
				f_temperature_celsius=i_temperature_binary*0.1;
				sprintf(hw.ucString,"%s - T:%-4.1f *C",hw.ucString,f_temperature_celsius);
			}
#endif			
			// this print was needed only to check  why the product code parameters was suddenly changing 'cause anybus was creating parameters...
			// sprintf(hw.ucString,"%s: %20.20s",pucStringLang(enumStr20_Lotto),hprg.theRunning.codicePrg);

			// formatto su 4a caratteri
			sprintf(hw.ucString,"%-40.40s",hw.ucString);
			ucPrintSmallText_ColNames(hw.ucString,defPROD_title_row+20,defPROD_title_col);


			//strcpy(hw.ucString,"tempo residuo       pezzi fatti/totale  ");
			vString40LangCopy(hw.ucString,enumStr40_TempoRes_PezziTot);
			ucPrintSmallText_ColNames(hw.ucString,defPROD_button_tempo_residuo_row-12,defPROD_button_tempo_residuo_col);
			// every 5 seconds, toggle between temperature and residual time
			if (uc_timeout_active(&toggle_visualize_temperature_timeout, 5000)){
				timeout_init(&toggle_visualize_temperature_timeout);
				uc_visualize_temperature=!uc_visualize_temperature;
			}
			// visualizzo anche la temperatura...
			if (  macParms.ucAbilitaCompensazioneTemperatura
			    &&macroIsCodiceProdotto_TemperatureCompensated(&hprg.theRunning)
				&&(uc_visualize_temperature||!PrgRunning)
				){
				int i_temperature_binary;
				float f_temperature_celsius;
				// prelevo la temperatura misurata dalla termocoppia collegata la modulo phoenix
				ui_get_canopen_temperature(&i_temperature_binary);
				f_temperature_celsius=i_temperature_binary*0.1;
				if (f_temperature_celsius<0){
					f_temperature_celsius=0.0;
				}
				else if (f_temperature_celsius>99){
					f_temperature_celsius=99;
				}
				sprintf(hw.ucString,"%-4.1f*C",f_temperature_celsius);
			
			}
			else{
				vCalcTempoResiduo();
				sprintf(hw.ucString,"%02uh%02um",TPR.uiOreResidue,TPR.uiMinutiResidui);
			}
			ucPrintTitleButton(hw.ucString,defPROD_button_tempo_residuo_row,defPROD_button_tempo_residuo_col,100,enumPROD_tempo_residuo,defLCD_Color_Trasparente,4);
#if 1
			switch(ucPretaglioPressed){
				case 0:
				default:
					ucPressChange_regola_distanzia=0;
					break;
				case 1:
					if (!macroIsCodiceProdotto_Distanziatore(&hprg.theRunning)){
						break;
					}
					if (us_is_button_pressed(enumPROD_Pretaglio_Value)){
						if (++ucPressChange_regola_distanzia>4){
							if (!PrgRunning){
								vAttivaOperazioneLama(enumCiclicaLama_distanzia);
								vLockOperazioneLama(enumCiclicaLama_distanzia);
							}
							ucPretaglioPressed=2;
							ucPressChange_regola_distanzia=0;
							break;
						}
					}
					else
						ucPressChange_regola_distanzia=0;
					break;
				case 2:
					break;
			}
#endif

			sprintf(hw.ucString," %-4.4i",
					nvram_struct.commesse[spiralatrice.runningCommessa].n+nvram_struct.commesse[spiralatrice.runningCommessa].magg
					);
			// se commessa terminata, visualizzo un testo statico!
			if (nvram_struct.commesse[spiralatrice.runningCommessa].ndo<nvram_struct.commesse[spiralatrice.runningCommessa].n+nvram_struct.commesse[spiralatrice.runningCommessa].magg)
				ucPrintStaticButton(hw.ucString,defPROD_da_fare_row,defPROD_da_fare_col,100,enumPROD_pezzi_da_fare,defLCD_Color_Trasparente);
			else
				ucPrintTitleButton(hw.ucString,defPROD_da_fare_row,defPROD_da_fare_col,100,enumPROD_pezzi_da_fare,defLCD_Color_Trasparente,2);

			sprintf(hw.ucString,"%-6.2f",nvram_struct.commesse[spiralatrice.runningCommessa].res);
			ucPrintTitleButton(hw.ucString,defPROD_valore_nominale_row,defPROD_valore_nominale_col,100,enumPROD_valore_nominale_resistenza,defLCD_Color_Trasparente,2);
			// se potenziometri sbloccati, visualizzo le varie info su rpm/assorbimenti ideali/reali
			if (!spiralatrice.AbilitaModificaPotenziometri){

				//strcpy(hw.ucString,"mt/min    rpm        assorb1    assorb2 ");
				vString40LangCopy(hw.ucString,enumStr40_MtMin_Rpm_Ass12);
                // replace m/min string with feet/min
                if (nvram_struct.actUM==UM_IN)
                {
                    unsigned char *puc_src;
                    unsigned char *puc_dst;
                    puc_src=pucStringLang(enumStr20_Um_feet_minuto);
                    puc_dst=hw.ucString;
                    while(*puc_src && *puc_dst)
                    {
                        *puc_dst++=*puc_src++;
                    }
                }
				ucPrintSmallText_ColNames(hw.ucString,defPROD_valori_nominali_row-12,defPROD_valori_nominali_col);
            
                fAux[0]=pPro->vel_prod_vera;
                // convert from m/min to feet/min
                if (nvram_struct.actUM==UM_IN)
                {
                    fAux[0]*=metri2feet;
                }
                    
				fAux[1]=pPro->rpm_mandrino;
				fAux[2]=pPro->assorb[0];
				fAux[3]=pPro->assorb[1];
				sprintf(hw.ucString,"%4.1f %4i %4.2f %4.2f ",
							fAux[0],
							(int)fAux[1],
							fAux[2],
							fAux[3]
							);
				// metto il tappo per evitare sforamenti nel caso di assorbimenti negativi...
				hw.ucString[20]=0;
				// se programma vuoto, lo visualizzo in grigio
				if (pPro->empty){
					ucPrintStaticButton(hw.ucString,defPROD_valori_nominali_row,defPROD_valori_nominali_col,100,enumPROD_valori_nominali,defLCD_Color_Gray);
				}
				// se invece contiene dati memorizzati, lo rappresento col fondo giallo
				else{
					ucPrintStaticButton(hw.ucString,defPROD_valori_nominali_row,defPROD_valori_nominali_col,100,enumPROD_valori_nominali,defLCD_Color_Yellow);
				}
				fAux[0]=velMMin;
                // convert from m/min to feet/min
                if (nvram_struct.actUM==UM_IN)
                {
                    fAux[0]*=metri2feet;
                }
            
				// se potenziometri sbloccati e prg running, visualizzo il numero di giri che arriva da potenziometro
				if ((PrgRunning)&&!spiralatrice.AbilitaModificaPotenziometri)
					iRpmMandrino=pComm_static->rpm;
				// se potenziometri Bloccati, visualizzo il numero di giri programmato, fisso
				else
					iRpmMandrino=pPro->rpm_mandrino;
				fAux[2]=pComm_static->assorb_frizioni[0];
				fAux[3]=pComm_static->assorb_frizioni[1];
				sprintf(hw.ucString,"%4.1f%c%4i%c%4.2f%c%4.2f",
							fAux[0],
							ucCharMuoviPotenziometri[0],
							iRpmMandrino,
							ucCharMuoviPotenziometri[1],
							fAux[2],
							ucCharMuoviPotenziometri[2],
							fAux[3]
							);
				// metto il tappo per evitare sforamenti nel caso di assorbimenti negativi...
				hw.ucString[20]=0;
				// se potenziometro=oro colato, nel senso che viene applicato subito a mandrino/ruote, allora visualizzo sfondo color oro
				if ((PrgRunning)&&!(spiralatrice.CheckPotMan||spiralatrice.AbilitaModificaPotenziometri))
					ucPrintStaticButton(hw.ucString,defPROD_valori_reali_row,defPROD_valori_reali_col,100,enumPROD_valori_reali,defLCD_Color_Yellow);
				// se potenziometri sbloccati, ma ancora non comandano mandrino/ruote metto il fondo in verde...
				else if ((PrgRunning)&&!spiralatrice.AbilitaModificaPotenziometri)
					ucPrintStaticButton(hw.ucString,defPROD_valori_reali_row,defPROD_valori_reali_col,100,enumPROD_valori_reali,defLCD_Color_Green);
				else
					ucPrintStaticButton(hw.ucString,defPROD_valori_reali_row,defPROD_valori_reali_col,100,enumPROD_valori_reali,defLCD_Color_Trasparente);
			}
			// se potenziometri bloccati, visualizzo in grande la v di lavoro e il numero di pezzi fatti
			else{
				if (nvram_struct.actUM==UM_IN)
					sprintf(hw.ucString,"      %-12.12s   ",pucStringLang(enumStr20_Um_feet_minuto));
				else
					sprintf(hw.ucString,"      %-12.12s   ",pucStringLang(enumStr20_Um_metri_minuto));
				sprintf(hw.ucString,"%s%-18.18s ",hw.ucString,pucStringLang(enumStr20_Numero_pezzi_prodotti));
				ucPrintSmallText_ColNames(hw.ucString,defPROD_valori_nominali_row-12,defPROD_valori_nominali_col);
                
				fAux[0]=velMMin;
                // convert from m/min to feet/min
                if (nvram_struct.actUM==UM_IN)
                {
                    fAux[0]*=metri2feet;
                }
                
				sprintf(hw.ucString,"%4.1f %4i ",fAux[0],nvram_struct.commesse[spiralatrice.runningCommessa].ndo);
				ucPrintStaticButton(hw.ucString,defPROD_valori_nominali_row,defPROD_valori_nominali_col,enumFontBig,enumPROD_valori_nominali,defLCD_Color_Trasparente);
			}
			ucRegolazioniAttive=!((spiralatrice.AbilitaModificaPotenziometri)||(macParms.uc_1Encoder_0Potenziometri==0));
			if (!ucRegolazioniAttive){
				//strcpy(hw.ucString,"ohm/m          pretaglio                ");
				pPro=&(hprg.theRunning);
				if (ucPretaglioPressed){
					if (ucPretaglioPressed==2){
						vString40LangCopy(hw.ucString,enumStr40_Rpm_Distanz);
						ucPrintSmallText_ColNames_BackColor(hw.ucString,defPROD_rpm_row-12,0,defLCD_Color_Magenta);
					}
					else{
						vString40LangCopy(hw.ucString,enumStr40_Rpm_Precut);
						ucPrintSmallText_ColNames(hw.ucString,defPROD_rpm_row-12,0);
					}

					//strcpy(hw.ucString,"UP");
					vStringLangCopy(hw.ucString,enumStr20_Up);
					ucPrintBitmapButton(enumBitmap_40x24_arrow_up,defPROD_ohmm_row-4,defPROD_ohmm_col+14*16U-1,enumPROD_Pretaglio_Up);

					//strcpy(hw.ucString,"DN");
					vStringLangCopy(hw.ucString,enumStr20_Dn);
					ucPrintBitmapButton(enumBitmap_40x24_arrow_dn,defPROD_ohmm_row-4,defPROD_ohmm_col+17*16U+4,enumPROD_Pretaglio_Dn);
				}
				else{
					if (PrgRunning){
						if (withMis){
							vString40LangCopy(hw.ucString,enumStr40_Rpm_Precut_OhmM);
                            // change the last string to ohm/feet
                            if (nvram_struct.actUM==UM_IN)
                            {
                                unsigned char *puc_src;
                                unsigned char *puc_dst;
                                puc_src=pucStringLang(enumStr20_Um_ohm_per_feet);
                                puc_dst=hw.ucString+strlen(hw.ucString)-1;
                                while((puc_dst>hw.ucString) &&  isspace(*puc_dst))
                                {
                                    puc_dst--;
                                }
                                while((puc_dst>hw.ucString) && !isspace(*puc_dst))
                                {
                                    puc_dst--;
                                }
                                // move to the first non space character of the last string
                                if (*puc_dst)
                                {
                                    puc_dst++;
                                }
                                // copy the correct unit name, ohm/feet
                                while(*puc_src && *puc_dst)
                                {
                                    *puc_dst++=*puc_src++;
                                }
                            }
                        
							ucPrintSmallText_ColNames(hw.ucString,defPROD_rpm_row-12,0);
							// leggo l'ultimo valore del ro non compensato!
							lastRoNonCompensatoMain[0]=lastRoNonCompensato;
							lastRoNonCompensatoMain[1]=lastRoNonCompensato;
							if (lastRoNonCompensatoMain[0]!=lastRoNonCompensatoMain[1])
								lastRoNonCompensatoMain[0]=lastRoNonCompensato;
							fAux[0]=((float)(lastRoNonCompensatoMain[0]))/(TARGET_URP * MAGN_LEN * impToMm / 1000. /targetRes );
                            // convert the ohm/m value to the appropriate unit
                            if (nvram_struct.actUM==UM_IN)
                            {
                                fAux[0]*=feet2metri;
                            }
						}
						else {
							vString40LangCopy(hw.ucString,enumStr40_Rpm_Precut_Ohm);
							ucPrintSmallText_ColNames(hw.ucString,defPROD_rpm_row-12,0);
							fAux[0]=(actLenImp*pComm_static->res) /targetLenImp;
						}
						if (fAux[0]<100.0)
							sprintf(hw.ucString,"%5.2f",fAux[0]);
						else if (fAux[0]<1000.0)
							sprintf(hw.ucString,"%5.1f",fAux[0]);
						else
							sprintf(hw.ucString,"%5i",(int)fAux[0]);
						ucPrintTitleButton(hw.ucString,defPROD_ohmm_row,defPROD_ohmm_col+14*16U+2,100,enumPROD_Ohmm,defLCD_Color_Trasparente,0);
					}
					else{
						vString40LangCopy(hw.ucString,enumStr40_Rpm_Precut);
						ucPrintSmallText_ColNames(hw.ucString,defPROD_rpm_row-12,0);
					}
				}
				if ((PrgRunning)&&!(spiralatrice.CheckPotMan||spiralatrice.AbilitaModificaPotenziometri))
					iRpmMandrino=pComm_static->rpm;
				// se potenziometri Bloccati, visualizzo il numero di giri programmato, fisso
				else if (spiralatrice.slowRpmMan==noSlowRpmMan)
					iRpmMandrino=pPro->rpm_mandrino;
				// se devo ancora agganciarmi coi potenziometri, visualizzo 100rpm
				else {
					iRpmMandrino=RPM_MAND_PRG_EMPTY;
				}
				sprintf(hw.ucString,"%5i",iRpmMandrino);
				ucPrintTitleButton(hw.ucString,defPROD_rpm_row,defPROD_rpm_col,100,enumPROD_Rpm,defLCD_Color_Trasparente,2);
				if (ucPretaglioPressed==2){
					if (nvram_struct.actUM==UM_IN)
						fAux[0]=MM_TO_INCH;
					else
						fAux[0]=1;
					sprintf(hw.ucString,"%+4.2f",pPro->f_coltello_pos_spaziatura_mm*fAux[0]);
				}
				else{
					if (pPro->pos_pretaglio>29)
						pPro->pos_pretaglio=29;
					else if (pPro->pos_pretaglio<-29)
						pPro->pos_pretaglio=-29;
					// visualizzo quota pretaglio in millesimi di pollice
					if (nvram_struct.actUM==UM_IN){
						int i_inch_mills;
						i_inch_mills=pPro->pos_pretaglio*(MM_TO_INCH*1000);
						if (i_inch_mills>999){
							i_inch_mills=999;
						}
						else if (i_inch_mills<-999){
							i_inch_mills=-999;
						}
						sprintf(hw.ucString,"%+i",i_inch_mills);
					}
					else{
						sprintf(hw.ucString,"%+4.2f",pPro->pos_pretaglio);
					}
				}
				// se pretaglio attivo, visualizzo sfondo giallo
				//if ((outDigVariable&ODG_PRETAGLIO)||ucPretaglioPressed)
				switch(ucPretaglioPressed){
					case 1:
					case 2:
						ucPrintStaticButton(hw.ucString,defPROD_ohmm_row,defPROD_button_unlock_col,100,enumPROD_Pretaglio_Value,(ucPretaglioPressed==1)?defLCD_Color_Yellow:defLCD_Color_Magenta);
						break;

					default:
						ucPrintStaticButton(hw.ucString,defPROD_ohmm_row,defPROD_button_unlock_col,100,enumPROD_Pretaglio_Value,defLCD_Color_Trasparente);
						break;
				}
			}
			else{
				vStringLangCopy(hw.ucString,enumStr_Spindle);
				// METTO IL VALORE INDICATO DAL POTENZIOMETRO DOPO LA STRINGA CHE IDENTIFICA L'INDIRIZZAMENTO
				//sprintf(hw.ucString,"%s%4i",hw.ucString,iGetHRE_setting(enumSpeedRegulation_Spindle));
				sprintf(hw.ucString,"%s%4i",hw.ucString,pComm_static->rpm);
				
				if (ucLinkEncoder==enumSpeedRegulation_Spindle)
					ui=defLCD_Color_Yellow;
				else
					ui=defLCD_Color_Trasparente;
				ucPrintStaticButton(hw.ucString,defPROD_encoder_row,defPROD_encoder_col,enumFontMedium,enumPROD_Encoder_Spindle,ui);

				//strcpy(hw.ucString,"WHEEL 1");
				vStringLangCopy(hw.ucString,enumStr_Wheel_1);
				// METTO IL VALORE INDICATO DAL POTENZIOMETRO DOPO LA STRINGA CHE IDENTIFICA L'INDIRIZZAMENTO
				if ((int)iMaxIncrementVelRuote[0]<iGetHRE_setting(enumSpeedRegulation_Wheel1))
					sprintf(hw.ucString,"%s%+2i%%",hw.ucString,(int)iMaxIncrementVelRuote[0]);
				else
					sprintf(hw.ucString,"%s%+2i%%",hw.ucString,iGetHRE_setting(enumSpeedRegulation_Wheel1));
				if (ucLinkEncoder==enumSpeedRegulation_Wheel1)
					ui=defLCD_Color_Yellow;
				else
					ui=defLCD_Color_Trasparente;
				ucPrintStaticButton(hw.ucString,defPROD_encoder_row,defPROD_f1_col,enumFontMedium,enumPROD_Encoder_F1,ui);

				//strcpy(hw.ucString,"WHEEL 2");
				vStringLangCopy(hw.ucString,enumStr_Wheel_2);
				// METTO IL VALORE INDICATO DAL POTENZIOMETRO DOPO LA STRINGA CHE IDENTIFICA L'INDIRIZZAMENTO
				if ((int)iMaxIncrementVelRuote[1]<iGetHRE_setting(enumSpeedRegulation_Wheel2))
					sprintf(hw.ucString,"%s%+2i%%",hw.ucString,(int)iMaxIncrementVelRuote[1]);
				else
					sprintf(hw.ucString,"%s%+2i%%",hw.ucString,iGetHRE_setting(enumSpeedRegulation_Wheel2));
				if (ucLinkEncoder==enumSpeedRegulation_Wheel2)
					ui=defLCD_Color_Yellow;
				else
					ui=defLCD_Color_Trasparente;
				ucPrintStaticButton(hw.ucString,defPROD_encoder_row,defPROD_f2_col,enumFontMedium,enumPROD_Encoder_F2,ui);

			}
			// routine che permette di aggiungere ad una finestra i bottoni standard sx, dx, up, dn, esc, cut
			vAddStandardButtons(enumPROD_Sx);

			//strcpy(hw.ucString,"                                taglio  ");
			//vString40LangCopy(hw.ucString,enumStr40_Taglio);
			//ucPrintSmallText_ColNames(hw.ucString,defPROD_CUT_row-12,defPROD_encoder_col);

			//strcpy(hw.ucString,"CUT!");
			//vStringLangCopy(hw.ucString,enumStr_Taglia);
			//ucPrintStaticButton(hw.ucString,defPROD_CUT_row,defPROD_CUT_col,100,enumPROD_Taglia,defLCD_Color_Trasparente);

			//strcpy(hw.ucString,"MEM");
			vStringLangCopy(hw.ucString,enumStr_Mem);
			ucPrintStaticButton(hw.ucString,defPROD_button_mem_row,defPROD_button_mem_col,100,enumPROD_mem,defLCD_Color_Trasparente);
			// se potenziometri sbloccati, li posso bloccare
			if (!spiralatrice.AbilitaModificaPotenziometri)
				//strcpy(hw.ucString,"LOCK");
				vStringLangCopy(hw.ucString,enumStr_Lock);
			// se potenziometri bloccati, li posso sbloccare
			else
				//strcpy(hw.ucString,"UNLK");
				vStringLangCopy(hw.ucString,enumStr_Unlock);
			ucPrintStaticButton(hw.ucString,defPROD_button_unlock_row,defPROD_button_unlock_col,100,enumPROD_lock_unlock,defLCD_Color_Trasparente);

			//strcpy(hw.ucString,"....");
			//ucPrintStaticButton(hw.ucString,defPROD_button_altri_row,defPROD_button_altri_col,100,enumPROD_altri,defLCD_Color_Trasparente);

			//strcpy(hw.ucString,"TARA");
			vStringLangCopy(hw.ucString,enumStr_Tara);
			// se sono in attesa di taratura, faccio diventare giallo il button...
			if (spiralatrice.OkAbilitaTaraturaStat)
				ucPrintStaticButton(hw.ucString,defPROD_button_tara_row,defPROD_button_tara_col,100,enumPROD_tara,defLCD_Color_Yellow);
			else
				ucPrintStaticButton(hw.ucString,defPROD_button_tara_row,defPROD_button_tara_col,100,enumPROD_tara,defLCD_Color_Trasparente);



			return 1;
	}
    return 1;
}//unsigned char ucHW_setupProduzione(void)




#define defMaxCharStringType 20

typedef enum {
	enumAllStr_Con_Lunghezza,
	enumAllStr_Con_Misura_Ohmica,
	enumAllStr_mm,
	enumAllStr_inch,
	enumAllStr_linguaDefault,
	enumAllStr_linguaEnglish,
	enumAllStr_linguaUlteriore,
	enumAllStr_Potenziometri,
	enumAllStr_Encoder,
	enumAllStr_Disabled,
	enumAllStr_Enabled,
	enumAllStr_FineCorsa,
	enumAllStr_Potenziometro,
	enumAllStr_NumOf
}enumListAllTheStrings;



typedef struct _TipoStructScegliStringa{
	enumStringType tipoScegliStringa;
	unsigned char xdata *pVariabile;
	unsigned char ucFirstVal;
	unsigned char ucLastVal;
	unsigned char ucFirstStringIdx;
	unsigned char ucLastStringIdx;
}TipoStructScegliStringa;

code TipoStructScegliStringa scegliStringa[enumStringType_NumOf]={
	{enumStringType_none,NULL,0,0,0,0},
	{enumStringType_SelezionaModoFunzionamento,&macParms.modo,MODE_LEN ,MODE_LENR,enumAllStr_Con_Lunghezza,enumAllStr_Con_Misura_Ohmica},
	{enumStringType_SelezionaUnitaMisura,(char*)&nvram_struct.actUM,UM_MM,UM_IN,enumAllStr_mm,enumAllStr_inch},
	{enumStringType_SelezionaLingua,(char*)&nvram_struct.actLanguage,0,2,enumAllStr_linguaDefault,enumAllStr_linguaUlteriore},
	{enumStringType_SelezionaEncoderPotenziometri,&macParms.uc_1Encoder_0Potenziometri,0,1,enumAllStr_Potenziometri,enumAllStr_Encoder},
	{enumStringType_SelezionaCommesseAutomatiche,&macParms.ucEnableCommesseAutomatiche,0,1,enumAllStr_Disabled,enumAllStr_Enabled},
	{enumStringType_SelezionaQuintaPortata,&macParms.ucAbilitaQuintaPortata,0,1,enumAllStr_Disabled,enumAllStr_Enabled},
	{enumStringType_UsaSensoreTaglio,&macParms.ucUsaSensoreTaglio,0,1,enumAllStr_FineCorsa,enumAllStr_Potenziometro},
	{enumStringType_AbilitaLampadaAllarme,&macParms.ucAbilitaLampadaAllarme,0,1,enumAllStr_Disabled,enumAllStr_Enabled},
	{enumStringType_SelezionaDistanziatore,&macParms.uc_distanziatore_type,0,enum_distanziatore_type_numOf-1,enumStr20_Distanziatore_NonPresente-enumStr20_FirstEnum,enumStr20_Distanziatore_Coltello-enumStr20_FirstEnum},
	{enumStringType_AbilitaCompensazioneTemperatura,&macParms.ucAbilitaCompensazioneTemperatura,0,1,enumAllStr_Disabled,enumAllStr_Enabled},
	{enumStringType_spindle_grinding_enable,&macParms.uc_enable_spindle_grinding,0,1,enumAllStr_Disabled,enumAllStr_Enabled},

};

// struttura che contiene le info sui campi del codice prodotto
typedef struct _TipoStructInfoParametro{
	unsigned char ucStartField; // indice del carattere di inizio del campo
	unsigned char ucLenField;   // lunghezza del campo
	unsigned char ucPictField[21]; // stringa di picture
	unsigned char ucFmtField[10];  // stringa di formato [mm]
	float fMin;						// valore minimo ammesso per il campo
	float fMax;						// valore massimo ammesso per il campo
	unsigned char xdata *pc;		// puntatore alla stringa src/dst delle modifiche
	unsigned char ucPictureEnable;	// abilitazione uso picture
	unsigned char ucMinMaxEnable;	// abilitazione uso min/max
	unsigned char ucTitleNumK[defMaxCharTitleNumK+1]; // stringa del titolo numeric keypad
	enumParameterType paramType;
	enumUmType enumUm;
	enumStringType enumIsAstring;
	float fConvUm_mm_toUm_inch;
	//unsigned char ucFmtField_inch[10]; // stringa di formato [inch]
	//unsigned char ucUm_mm[10];		// unit� di misura [mm]
	//unsigned char ucUm_inch[10];    // unit� di misura [inch]
}TipoStructInfoParametro;




typedef struct _TipoStructInfoParametriMacchina{
	float theFloat;
	unsigned int uiTheUInt;
	unsigned long ulTheULong;
	unsigned char ucIdxFirstParametro;
}TipoStructInfoParametriMacchina;
xdata TipoStructInfoParametriMacchina ipm;


typedef enum{
	enumList_PM_tipoControlloTaglio,
	enumList_PM_modo,
	enumList_PM_numfili,
	enumList_PM_encoder,
	enumList_PM_commesse_auto,
	enumList_PM_impgiro,
	enumList_PM_mmgiro,
	enumList_PM_um,
	enumList_PM_rpm_mandrino,
	enumList_PM_rpm_frizioni_0,
	enumList_PM_rpm_frizioni_1,
	enumList_PM_nPezziAvviso,
	enumList_PM_nSecLampSpegn,
	enumList_PM_iAsse_0,
	enumList_PM_iAsse_1,
	enumList_PM_durata_all,
	enumList_PM_diam_test_intrz,
	enumList_PM_perc_max_err_ro,
	enumList_PM_max_bad_spezz,
	enumList_PM_presenza_distanziatore,
	enumList_PM_offset_distanziatore,
	enumList_PM_fs_assorb_ruote,
	enumList_PM_durata_bandella,
	enumList_PM_ritardo_bandella,
	enumList_PM_abilita_lampada_allarme,
	enumList_PM_abilita_quinta_portata,
	enumList_PM_abilita_compensazione_temperatura,
	enumList_PM_spindle_grinding_enable,
	enumList_PM_lingua,
	enumList_PM_password,
	enumList_PM_serial_number,
	enumList_PM_versione_cnc,
	enumList_PM_versione_lcd,
	enumList_PM_versione_fpga,
	enumList_PM_ethernet_mac_address,

	enumList_PM_NumOf
}enumListParametriMacchina;

code unsigned char ucSuperUserPasswordRequired[]={
 0,//	{0,4, "n","%-1i"      ,0           ,0           ,(unsigned char xdata*)&macParms.ucUsaSensoreTaglio,1,1,enumStr20_SensoreTaglio,   enumPT_unsignedChar,enumUm_none,     enumStringType_UsaSensoreTaglio,1},//
 0,//{0,20, "n","%-1i"      ,0           ,0           ,(unsigned char xdata*)&macParms.modo				,   1,1, enumStr20_Sel_Funzionamento,    enumPT_unsignedChar,enumUm_none,    enumStringType_SelezionaModoFunzionamento,1}, //
 0,//{0,2, "nn","%-2i",MIN_NUM_FILI,MAX_NUM_FILI,(unsigned char xdata*)&macParms.ucMaxNumFili,           1,1,     enumStr20_Numero_max_fili,enumPT_unsignedChar, enumUm_none,enumStringType_none,1},
 0,//{0,4, "n","%-1i"      ,0           ,0           ,(unsigned char xdata*)&macParms.uc_1Encoder_0Potenziometri,1,1,enumStr20_Sel_Encoder,   enumPT_unsignedChar,enumUm_none,     enumStringType_SelezionaEncoderPotenziometri,1},//
 0,//{0,4, "n","%-1i"      ,0           ,0           ,(unsigned char xdata*)&macParms.ucEnableCommesseAutomatiche,1,1,enumStr20_Sel_Automatico,   enumPT_unsignedChar,enumUm_none,     enumStringType_SelezionaCommesseAutomatiche,1},//
 0,//{0,4, "nnnn","%4i",MIN_IMP_GIRO_ENC,MAX_IMP_GIRO_ENC,(unsigned char xdata*)&ipm.uiAuxImpGiro,           1,1, enumStr20_Impulsi_giro_encoder,enumPT_unsignedInt, enumUm_imp_giro,enumStringType_none,1},
 0,//{0,8, "NNNNNNNN","%-7.3f",MIN_SVIL_RUO,MAX_SVIL_RUO,(unsigned char xdata*)&macParms.mmGiro,             1,1,enumStr20_Sviluppo_Ruota_Mis,enumPT_float,       enumUm_mm,      enumStringType_none,MM_TO_INCH},//mm/inch
 0,//{0,4, "n","%-1i"      ,0           ,0           ,(unsigned char xdata*)&nvram_struct.actUM                       ,1,1,enumStr20_Unita_di_Misura,   enumPT_unsignedChar,enumUm_none,     enumStringType_SelezionaUnitaMisura,1},//
 0,//{0,5, "nnnnn","%-5i",MIN_RPM_MANDRINO,MAX_RPM_MANDRINO,(unsigned char xdata*)&macParms.rpm_mandrino,    1,1,enumStr20_Vel_Max_Mandrino,enumPT_unsignedInt, enumUm_rpm,     enumStringType_none,1},// rpm
 0,//{0,5, "nnnnn","%-5i",MIN_RPM_FRIZIONE,MAX_RPM_FRIZIONE,(unsigned char xdata*)&macParms.rpm_frizioni[0], 1,1,enumStr20_Vel_Max_Ruota_Co_1,enumPT_unsignedInt, enumUm_rpm,     enumStringType_none,1},// rpm
 0,//{0,5, "nnnnn","%-5i",MIN_RPM_FRIZIONE,MAX_RPM_FRIZIONE,(unsigned char xdata*)&macParms.rpm_frizioni[1], 1,1,enumStr20_Vel_Max_Ruota_Co_2,enumPT_unsignedInt, enumUm_rpm,     enumStringType_none,1},// rpm
 0,//{0,2, "nn","%-2i",MIN_NPEZZI_AVVISO,MAX_NPEZZI_AVVISO,(unsigned char xdata*)&macParms.nPezziAvviso,     1,1,enumStr20_Pz_prima_Fine,enumPT_unsignedChar,enumUm_num,     enumStringType_none,1}, //
 0,//{0,2, "nn","%-2i",MIN_LAMP_SPEGN,MAX_LAMP_SPEGN,(unsigned char xdata*)&macParms.nSecLampSpegn,          1,1,enumStr20_Spegnimento_Lampada,enumPT_unsignedChar,enumUm_secondi, enumStringType_none,1}, //secondi
 0,//{0,8, "NNNNNNNN","%-7.3f",MIN_IASSE,MAX_IASSE,(unsigned char xdata*)&macParms.iasse[0],                 1,1,enumStr20_Interasse_1,enumPT_float,       enumUm_mm,      enumStringType_none,MM_TO_INCH}, //mm-inch
 0,//{0,8, "NNNNNNNN","%-7.3f",MIN_IASSE,MAX_IASSE,(unsigned char xdata*)&macParms.iasse[1],                 1,1,enumStr20_Interasse_2,enumPT_float,       enumUm_mm,      enumStringType_none,MM_TO_INCH}, //mm-inch
 0,//{0,5, "nnnnn","%-5i",MIN_DURATA_ALL,MAX_DURATA_ALL,(unsigned char xdata*)&macParms.durata_all,          1,1,enumStr20_Durata_Min_Allarme,enumPT_unsignedInt, enumUm_millisecondi,enumStringType_none,1}, //ms
 0,//{0,6, "NNNNNN","%-5.3f",MIN_DIAM_TEST_INTRZ,100.0,(unsigned char xdata*)&macParms.DiamTestIntrz,        1,1,enumStr20_DFilo_Esc_Interruz,enumPT_float,       enumUm_mm,      enumStringType_none,MM_TO_INCH},//mm/inch
 0,//{0,3, "nnn","%-3i",0,100,(unsigned char xdata*)&macParms.ucPercMaxErrRo,                                1,1,enumStr20_P_Misura_Scarta,enumPT_unsignedChar,enumUm_percentuale,enumStringType_none,1}, //%
 0,//{0,3, "nnn","%-3i",0,100,(unsigned char xdata*)&macParms.ucMaxBadSpezzConsec,                           1,1,enumStr20_N_Misure_Scarte,enumPT_unsignedChar,enumUm_num,     enumStringType_none,1}, //
 1,//{0,4, "n","%-1i"      ,0           ,0           ,(unsigned char xdata*)&macParms.uc_distanziatore_type,1,1,enumStr20_PresenzaDistanziatore,   enumPT_unsignedChar,enumUm_none,     enumStringType_SelezionaCommesseAutomatiche,1},//
 0,//{0,5, "nnnnn","%-5i",MIN_DURATA_DISCESA_DISTANZIATORE,MAX_DURATA_DISCESA_DISTANZIATORE,(unsigned char xdata*)&macParms.usDelayDistanziaDownMs,          1,1,enumStr20_DelayDistanziatoreDown,enumPT_unsignedInt, enumUm_millisecondi,enumStringType_none,1}, //ms


0,//{0,6, "NNNNNN","%-5.3f",0,100.0,(unsigned char xdata*)&macParms.fFSassorbRuote,                         1,1,enumStr20_FS_Ass_Ruote_Contr,enumPT_float,       enumUm_ampere,  enumStringType_none,1},//amp�res
0,//{0,4, "nnnn","%-4u",0,MAX_ATTIVAZIONE_BANDELLA,(unsigned char xdata*)&macParms.uiDurataAttivazioneBandella_centesimi_secondo, 1,1,enumStr20_DurataBandella,enumPT_unsignedInt,enumUm_centesimisecondo,     enumStringType_none,1}, //
0,//{0,4, "nnnn","%-4u",0,MAX_RITARDO_BANDELLA,(unsigned char xdata*)&macParms.uiRitardoAttivazioneBandella_centesimi_secondo,1,1,enumStr20_RitardoBandella,enumPT_unsignedInt,enumUm_centesimisecondo,     enumStringType_none,1}, //
1,//{0,4, "n","%-1i"      ,0           ,0           ,(unsigned char xdata*)&macParms.ucAbilitaLampadaAllarme,1,1,enumStr20_LampadaAllarme,  enumPT_unsignedChar,enumUm_none,     enumStringType_AbilitaLampadaAllarme ,1}, //

1,//{0,4, "n","%-1i"      ,0           ,0           ,(unsigned char xdata*)&macParms.ucAbilitaQuintaPortata,1,1,enumStr20_Quinta_Portata,   enumPT_unsignedChar,enumUm_none,     enumStringType_SelezionaQuintaPortata,1},//
1,//{0,4, "n","%-1i"      ,0           ,0           ,(unsigned char xdata*)&macParms.ucAbilitaCompensazioneTemperatura,1,1,enumStr20_CompensazioneTemperatura,   enumPT_unsignedChar,enumUm_none,     enumStringType_AbilitaCompensazioneTemperatura,1},//
// spindle grinding
1, //	{0,4, "n","%-1i"      ,0           ,0           ,(unsigned char xdata*)&macParms.uc_enable_spindle_grinding,1,1,enumStr20_spindle_grinding,   enumPT_unsignedChar,enumUm_none,     enumStringType_spindle_grinding_enable,1},//
0,//{0,4, "n","%-1i"      ,0           ,0           ,(unsigned char xdata*)&nvram_struct.actLanguage                       ,1,1,enumStr20_Sel_Lingua,   enumPT_unsignedChar,enumUm_none,     enumStringType_SelezionaLingua,1},//
// password
0,//{0,5, defPictureString_Password,"%-5li",0,99999,(unsigned char xdata*)&macParms.password,          1,1,enumStr20_password,enumPT_unsignedLong, enumUm_modify_password,enumStringType_none,1}, //...
// box serial number!
1,//{0,7, "nnnnnnn","%-7li",0,9999999L,(unsigned char xdata*)&ulBoxSerialNumber,          1,1,enumStr20_serialNumber,enumPT_unsignedLong, enumUm_none,enumStringType_none,1}, //...

// serve per versione cnc
0,//{0,8, "nnnnnnnn","%-8i"      ,0           ,0           ,(unsigned char xdata*)&nvram_struct.actLanguage                       ,1,1,enumStr20_Versione_CNC,   enumPT_unsignedChar,enumUm_none,     enumStringType_none,1},//
// serve per versione lcd
0,//{0,8, "nnnnnnnn","%-8i"      ,0           ,0           ,(unsigned char xdata*)&nvram_struct.actLanguage                       ,1,1,enumStr20_Versione_LCD,   enumPT_unsignedChar,enumUm_none,     enumStringType_none,1},//
// serve per versione fpga
0,//{0,8, "nnnnnnnn","%-8i"      ,0           ,0           ,(unsigned char xdata*)&nvram_struct.actLanguage                       ,1,1,enumStr20_Versione_Fpga,   enumPT_unsignedChar,enumUm_none,     enumStringType_none,1},//
// serve per mac address
0,//{0,8, "nnnnnnnn","%-8i"      ,0           ,0           ,(unsigned char xdata*)&nvram_struct.actLanguage                       ,1,1,enumStr20_Ethernet_MacAddress,   enumPT_unsignedChar,enumUm_none,     enumStringType_none,1},//

};


code TipoStructInfoParametro_Lingua if_PM[]={
	{0,4, "n","%-1i"      ,0           ,0           ,(unsigned char xdata*)&macParms.ucUsaSensoreTaglio,1,1,enumStr20_SensoreTaglio,   enumPT_unsignedChar,enumUm_none,     enumStringType_UsaSensoreTaglio,1},//
	{0,20, "n","%-1i"      ,0           ,0           ,(unsigned char xdata*)&macParms.modo				,   1,1, enumStr20_Sel_Funzionamento,    enumPT_unsignedChar,enumUm_none,    enumStringType_SelezionaModoFunzionamento,1}, //
	{0,2, "nn","%-2i",MIN_NUM_FILI,MAX_NUM_FILI,(unsigned char xdata*)&macParms.ucMaxNumFili,           1,1,     enumStr20_Numero_max_fili,enumPT_unsignedChar, enumUm_none,enumStringType_none,1},
	{0,4, "n","%-1i"      ,0           ,0           ,(unsigned char xdata*)&macParms.uc_1Encoder_0Potenziometri,1,1,enumStr20_Sel_Encoder,   enumPT_unsignedChar,enumUm_none,     enumStringType_SelezionaEncoderPotenziometri,1},//
	{0,4, "n","%-1i"      ,0           ,0           ,(unsigned char xdata*)&macParms.ucEnableCommesseAutomatiche,1,1,enumStr20_Sel_Automatico,   enumPT_unsignedChar,enumUm_none,     enumStringType_SelezionaCommesseAutomatiche,1},//
	{0,4, "nnnn","%4i",MIN_IMP_GIRO_ENC,MAX_IMP_GIRO_ENC,(unsigned char xdata*)&(ipm.uiTheUInt),           1,1, enumStr20_Impulsi_giro_encoder,enumPT_unsignedInt, enumUm_imp_giro,enumStringType_none,1},
	{0,8, "NNNNNNNN","%-7.3f",MIN_SVIL_RUO,MAX_SVIL_RUO,(unsigned char xdata*)&(ipm.theFloat),             1,1,enumStr20_Sviluppo_Ruota_Mis,enumPT_float,       enumUm_mm,      enumStringType_none,MM_TO_INCH},//mm/inch
	{0,4, "n","%-1i"      ,0           ,0           ,(unsigned char xdata*)&nvram_struct.actUM                       ,1,1,enumStr20_Unita_di_Misura,   enumPT_unsignedChar,enumUm_none,     enumStringType_SelezionaUnitaMisura,1},//
	{0,5, "nnnnn","%-5i",MIN_RPM_MANDRINO,MAX_RPM_MANDRINO,(unsigned char xdata*)&macParms.rpm_mandrino,    1,1,enumStr20_Vel_Max_Mandrino,enumPT_unsignedInt, enumUm_rpm,     enumStringType_none,1},// rpm
	{0,5, "nnnnn","%-5i",MIN_RPM_FRIZIONE,MAX_RPM_FRIZIONE,(unsigned char xdata*)&macParms.rpm_frizioni[0], 1,1,enumStr20_Vel_Max_Ruota_Co_1,enumPT_unsignedInt, enumUm_rpm,     enumStringType_none,1},// rpm
	{0,5, "nnnnn","%-5i",MIN_RPM_FRIZIONE,MAX_RPM_FRIZIONE,(unsigned char xdata*)&macParms.rpm_frizioni[1], 1,1,enumStr20_Vel_Max_Ruota_Co_2,enumPT_unsignedInt, enumUm_rpm,     enumStringType_none,1},// rpm
	{0,2, "nn","%-2i",MIN_NPEZZI_AVVISO,MAX_NPEZZI_AVVISO,(unsigned char xdata*)&macParms.nPezziAvviso,     1,1,enumStr20_Pz_prima_Fine,enumPT_unsignedChar,enumUm_num,     enumStringType_none,1}, //
	{0,2, "nn","%-2i",MIN_LAMP_SPEGN,MAX_LAMP_SPEGN,(unsigned char xdata*)&macParms.nSecLampSpegn,          1,1,enumStr20_Spegnimento_Lampada,enumPT_unsignedChar,enumUm_secondi, enumStringType_none,1}, //secondi
	{0,8, "NNNNNNNN","%-7.3f",MIN_IASSE,MAX_IASSE,(unsigned char xdata*)&(ipm.theFloat),                 1,1,enumStr20_Interasse_1,enumPT_float,       enumUm_mm,      enumStringType_none,MM_TO_INCH}, //mm-inch
	{0,8, "NNNNNNNN","%-7.3f",MIN_IASSE,MAX_IASSE,(unsigned char xdata*)&(ipm.theFloat),                 1,1,enumStr20_Interasse_2,enumPT_float,       enumUm_mm,      enumStringType_none,MM_TO_INCH}, //mm-inch
	{0,5, "nnnnn","%-5i",MIN_DURATA_ALL,MAX_DURATA_ALL,(unsigned char xdata*)&macParms.durata_all,          1,1,enumStr20_Durata_Min_Allarme,enumPT_unsignedInt, enumUm_millisecondi,enumStringType_none,1}, //ms
	{0,6, "NNNNNN","%-5.3f",MIN_DIAM_TEST_INTRZ,100.0,(unsigned char xdata*)&(ipm.theFloat),        1,1,enumStr20_DFilo_Esc_Interruz,enumPT_float,       enumUm_mm,      enumStringType_none,MM_TO_INCH},//mm/inch
	{0,3, "nnn","%-3i",0,100,(unsigned char xdata*)&macParms.ucPercMaxErrRo,                                1,1,enumStr20_P_Misura_Scarta,enumPT_unsignedChar,enumUm_percentuale,enumStringType_none,1}, //%
	{0,3, "nnn","%-3i",0,100,(unsigned char xdata*)&macParms.ucMaxBadSpezzConsec,                           1,1,enumStr20_N_Misure_Scarte,enumPT_unsignedChar,enumUm_num,     enumStringType_none,1}, //
	{0,4, "n","%-1i"      ,0           ,0           ,(unsigned char xdata*)&macParms.uc_distanziatore_type,1,1,enumStr20_PresenzaDistanziatore,   enumPT_unsignedChar,enumUm_none,     enumStringType_SelezionaDistanziatore,1},//
	{0,5, "nnnnn","%-5i",MIN_DURATA_DISCESA_DISTANZIATORE,MAX_DURATA_DISCESA_DISTANZIATORE,(unsigned char xdata*)&macParms.usDelayDistanziaDownMs,          1,1,enumStr20_DelayDistanziatoreDown,enumPT_unsignedShort, enumUm_millisecondi,enumStringType_none,1}, //ms
	{0,6, "NNNNNN","%-5.3f",0,100.0,(unsigned char xdata*)&(ipm.theFloat),                         1,1,enumStr20_FS_Ass_Ruote_Contr,enumPT_float,       enumUm_ampere,  enumStringType_none,1},//amp�res
	{0,4, "nnnn","%-4u",0,MAX_ATTIVAZIONE_BANDELLA,(unsigned char xdata*)&(ipm.uiTheUInt), 1,1,enumStr20_DurataBandella,enumPT_unsignedInt,enumUm_centesimisecondo,     enumStringType_none,1}, //
	{0,4, "nnnn","%-4u",0,MAX_RITARDO_BANDELLA,(unsigned char xdata*)&(ipm.uiTheUInt),1,1,enumStr20_RitardoBandella,enumPT_unsignedInt,enumUm_centesimisecondo,     enumStringType_none,1}, //
	{0,4, "n","%-1i"      ,0           ,0           ,(unsigned char xdata*)&macParms.ucAbilitaLampadaAllarme,1,1,enumStr20_LampadaAllarme,  enumPT_unsignedChar,enumUm_none,     enumStringType_AbilitaLampadaAllarme ,1}, //

	{0,4, "n","%-1i"      ,0           ,0           ,(unsigned char xdata*)&macParms.ucAbilitaQuintaPortata,1,1,enumStr20_Quinta_Portata,   enumPT_unsignedChar,enumUm_none,     enumStringType_SelezionaQuintaPortata,1},//
	{0,4, "n","%-1i"      ,0           ,0           ,(unsigned char xdata*)&macParms.ucAbilitaCompensazioneTemperatura,1,1,enumStr20_CompensazioneTemperatura,   enumPT_unsignedChar,enumUm_none,     enumStringType_AbilitaCompensazioneTemperatura,1},//
// spindle grinding
	{0,4, "n","%-1i"      ,0           ,0           ,(unsigned char xdata*)&macParms.uc_enable_spindle_grinding,1,1,enumStr20_spindle_grinding,   enumPT_unsignedChar,enumUm_none,     enumStringType_spindle_grinding_enable,1},//



	{0,4, "n","%-1i"      ,0           ,0           ,(unsigned char xdata*)&nvram_struct.actLanguage                       ,1,1,enumStr20_Sel_Lingua,   enumPT_unsignedChar,enumUm_none,     enumStringType_SelezionaLingua,1},//
// password
	{0,5, defPictureString_Password,"%-5li",0,99999,(unsigned char xdata*)&(ipm.ulTheULong),          1,1,enumStr20_password,enumPT_unsignedLong, enumUm_modify_password,enumStringType_none,1}, //...
// box serial number!
	{0,7, "nnnnnnn","%-7li",0,9999999L,(unsigned char xdata*)&(ipm.ulTheULong),          1,1,enumStr20_serialNumber,enumPT_unsignedLong, enumUm_none,enumStringType_none,1}, //...

// serve per versione cnc
	{0,8, "nnnnnnnn","%-8i"      ,0           ,0           ,(unsigned char xdata*)&nvram_struct.actLanguage                       ,1,1,enumStr20_Versione_CNC,   enumPT_unsignedChar,enumUm_none,     enumStringType_none,1},//
// serve per versione lcd
	{0,8, "nnnnnnnn","%-8i"      ,0           ,0           ,(unsigned char xdata*)&nvram_struct.actLanguage                       ,1,1,enumStr20_Versione_LCD,   enumPT_unsignedChar,enumUm_none,     enumStringType_none,1},//
// serve per versione fpga
	{0,8, "nnnnnnnn","%-8i"      ,0           ,0           ,(unsigned char xdata*)&nvram_struct.actLanguage                       ,1,1,enumStr20_Versione_Fpga,   enumPT_unsignedChar,enumUm_none,     enumStringType_none,1},//
// serve per mac address
	{0,8, "nnnnnnnn","%-8i"      ,0           ,0           ,(unsigned char xdata*)&nvram_struct.actLanguage                       ,1,1,enumStr20_Ethernet_MacAddress,   enumPT_unsignedChar,enumUm_none,     enumStringType_none,1},//

};

#define defTotalNumParsMacchinaToVisualize (sizeof(if_PM)/sizeof(if_PM[0]))




void vGetValueParametro(unsigned char *pString,unsigned char idxParametro,code TipoStructInfoParametro_Lingua *pif){
	xdata float fConv;
	//sprintf(hw.ucString,pif[idxParametro].ucFmtField,pif[idxParametro].pc[0]);
	switch(pif[idxParametro].paramType){
		case enumPT_unsignedChar:
			sprintf(pString,pif[idxParametro].ucFmtField,*(unsigned char*)pif[idxParametro].pc);
			break;
		case enumPT_signedChar:
			sprintf(pString,pif[idxParametro].ucFmtField,*(signed char*)pif[idxParametro].pc);
			break;
		case enumPT_unsignedInt:
			sprintf(pString,pif[idxParametro].ucFmtField,*(unsigned int*)pif[idxParametro].pc);
			break;
		case enumPT_signedInt:
			sprintf(pString,pif[idxParametro].ucFmtField,*(signed int*)pif[idxParametro].pc);
			break;
		case enumPT_unsignedShort:
			sprintf(pString,pif[idxParametro].ucFmtField,*(unsigned short int*)pif[idxParametro].pc);
			break;
		case enumPT_signedShort:
			sprintf(pString,pif[idxParametro].ucFmtField,*(signed short int*)pif[idxParametro].pc);
			break;
		case enumPT_unsignedLong:
			sprintf(pString,pif[idxParametro].ucFmtField,*(unsigned long*)pif[idxParametro].pc);
			break;
		case enumPT_signedLong:
			sprintf(pString,pif[idxParametro].ucFmtField,*(signed long*)pif[idxParametro].pc);
			break;
		case enumPT_float:
			if ((nvram_struct.actUM==UM_IN)&&ucIsUmSensitive(pif[idxParametro].enumUm)){
				fConv=pif[idxParametro].fConvUm_mm_toUm_inch;
			}
			else
				fConv=1.0;
			sprintf(pString,pif[idxParametro].ucFmtField,(*(float*)pif[idxParametro].pc)*fConv);
			break;
		case enumPT_string:
			strncpy(pString,pif[idxParametro].pc,pif[idxParametro].ucLenField);
			pString[pif[idxParametro].ucLenField]=0;
			break;
		default:
		{
			break;
		}

	}
}

void vSetValueParametro(unsigned char *pString,unsigned char idxParametro,code TipoStructInfoParametro_Lingua *pif){
	xdata int i;
	unsigned int ui;
	xdata long theLong;
	unsigned long theULong;
	xdata float theFloat;
	signed short theShortSigned;
	unsigned short theShortUnsigned;
	switch(pif[idxParametro].paramType){
		case enumPT_unsignedChar:
			i=atoi(pString);
			*(unsigned char *)(pif[idxParametro].pc)=i&0xFF;
			break;
		case enumPT_signedChar:
			i=atoi(pString);
			*(signed char *)(pif[idxParametro].pc)=i&0xFF;
			break;
		case enumPT_unsignedInt:
			ui=atoi(pString);
			*(unsigned int *)(pif[idxParametro].pc)=ui;
			break;
		case enumPT_signedInt:
			i=atoi(pString);
			*(signed int *)(pif[idxParametro].pc)=i;
			break;
		case enumPT_unsignedLong:
			theULong=atol(pString);
			*(unsigned long *)(pif[idxParametro].pc)=theULong;
			break;
		case enumPT_signedLong:
			theLong=atol(pString);
			*(signed long *)(pif[idxParametro].pc)=theLong;
			break;
		case enumPT_signedShort:
			theShortSigned=atoi(pString);
			*(signed short *)(pif[idxParametro].pc)=theShortSigned;
			break;
		case enumPT_unsignedShort:
			theShortUnsigned=atoi(pString);
			*(unsigned short *)(pif[idxParametro].pc)=theShortUnsigned;
			break;
		case enumPT_float:
			theFloat=atof(pString);
			if ((nvram_struct.actUM==UM_IN)&&ucIsUmSensitive(pif[idxParametro].enumUm)){
				*(float *)(pif[idxParametro].pc)=theFloat/pif[idxParametro].fConvUm_mm_toUm_inch;
			}
			else
				*(float *)(pif[idxParametro].pc)=theFloat;

			break;
		case enumPT_string:
			strncpy(pif[idxParametro].pc,pString,pif[idxParametro].ucLenField);
			// tappo...
			pif[idxParametro].pc[pif[idxParametro].ucLenField]=0;
			break;
		default:
		{
			break;
		}

	}
}

void vRefresh_IPM_values(unsigned char idxParametro){
	switch(idxParametro){
		case enumList_PM_fs_assorb_ruote:
			ipm.theFloat=macParms.fFSassorbRuote;
			break;
		// se modificata durata attivazione bandella, reinizializzo bandella 
		// cosi' abilitazione/disabilitazione viene fatta al volo
		case enumList_PM_durata_bandella:
			ipm.uiTheUInt=macParms.uiDurataAttivazioneBandella_centesimi_secondo;
			break;
		case enumList_PM_ritardo_bandella:
			ipm.uiTheUInt=macParms.uiRitardoAttivazioneBandella_centesimi_secondo;
			break;

		case enumList_PM_password:
			ipm.ulTheULong=macParms.password;
			break;

		case enumList_PM_mmgiro:
			ipm.theFloat=macParms.mmGiro;
			break;
	    
		case enumList_PM_diam_test_intrz:
			ipm.theFloat=macParms.DiamTestIntrz;
			break;
		case enumList_PM_serial_number:
			ipm.ulTheULong=ulBoxSerialNumber;
			break;
		case enumList_PM_impgiro:
		   ipm.uiTheUInt= macParms.impGiro/N_FASI_ENCODER;
		   break;

		case enumList_PM_iAsse_0:
			ipm.theFloat=macParms.iasse[0];
			break;
		case enumList_PM_iAsse_1:
			ipm.theFloat=macParms.iasse[1];
			break;
	}
}


unsigned char ucHW_ParametriMacchina(void){
	//void vSaveMachineParsOnFile(void);
	void vJumpToBootMain(void);
	#define defPM_NumParametroMacchina 0
	// pulsanti della finestra
	typedef enum {
			enum_PM_Title =0,
			enum_PM_Button_0,
			enum_PM_Button_1,
			enum_PM_Button_2,
			enum_PM_Button_3,
			enum_PM_Button_4,
			enum_PM_Button_Desc_0,
			enum_PM_Button_Desc_1,
			enum_PM_Button_Desc_2,
			enum_PM_Button_Desc_3,
			enum_PM_Button_Desc_4,

			enum_PM_Sx,
			enum_PM_Up,
			enum_PM_Cut,
			enum_PM_Esc,
			enum_PM_Dn,
			enum_PM_Dx,

			enum_PM_NumOfButtons
		}enum_PM_Buttons;

	#define defParMac_button_Title_row 4

	#define defParMac_button_Esc_row (defLcdWidthY_pixel-32-4)
	#define defParMac_button_Esc_col (defLcdWidthX_pixel-32*3-8)

	#define defParMac_button_Title_col 6
	#define defParMac_button_ParDesc_row 46
	#define defParMac_button_Par_um_row (defParMac_button_ParDesc_row+12)
	#define defParMac_button_ParDesc_col 0
	#define defParMac_button_ParValue_row_normalfont (defParMac_button_ParDesc_row)
	#define defParMac_button_ParValue_row_smallfont (defParMac_button_ParDesc_row)
	#define defParMac_button_ParValue_col (defParMac_button_ParDesc_col+120)
	#define defPM_OffsetBetweenRows 34
	#define defPM_NumOfParsVisualized 5


	unsigned int xdata idxParametro,i;
	unsigned int xdata uiColonnaParametro;
	char * xdata pc;
	xdata enumStringType stringType;
	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			idxParametro=defGetWinParam(defPM_NumParametroMacchina);
			// visualizzata finestra password non corretta
			if (idxParametro==201){
				sLCD.ucPasswordChecked=0;
				// chiamo il main menu...
				vJumpToWindow(enumWinId_MainMenu);
				return 2;
			}
			// check password???
			else if (idxParametro==200){
				if (ucVerifyPasswordCorrect(hw.ucStringNumKeypad_out)){
					sLCD.ucPasswordChecked=1;
					vSetStatusCurrentWindow(enumWindowStatus_Active);
					return 2;
				}
				// visualizzo la finestra: password non corretta
				// numero msg da visualizzare: 1
				ucSetWindowParam(enumWinId_Info,0, 1);
				// messaggio da visualizzare: password bad
				ucSetWindowParam(enumWinId_Info,1, enumStr20_bad_password);
				// messaggio: password KO
				defSetWinParam(defPM_NumParametroMacchina,201);
			    // chiamo la finestra info
		        ucCallWindow(enumWinId_Info);
				return 2;
			}
			// check password???
			else if (idxParametro==400){
				strncat(uc_super_user_password,hw.ucStringNumKeypad_out,sizeof(uc_super_user_password)-1-strlen(uc_super_user_password));
				if (ui_super_user_password_cycle==3){
					if (ui_save_encrypted_param(uc_super_user_password)!=enum_save_encrypted_param_retcode_ok){
						// visualizzo la finestra: password non corretta
						// numero msg da visualizzare: 1
						ucSetWindowParam(enumWinId_Info,0, 1);
						// messaggio da visualizzare: password bad
						ucSetWindowParam(enumWinId_Info,1, enumStr20_bad_password);
						// messaggio: password KO
						defSetWinParam(defPM_NumParametroMacchina,201);
						// chiamo la finestra info
						ucCallWindow(enumWinId_Info);
					}
					else{
						vSetStatusCurrentWindow(enumWindowStatus_Active);
					}
				}
				else{
					ui_super_user_password_cycle++;
					uc_use_hexadecimal_letters=1;
					vSetNumK_to_ask_password_super_user(ui_super_user_password_cycle);
					defSetWinParam(defPM_NumParametroMacchina,400);
					// chiamo il numeric keypad
					ucCallWindow(enumWinId_Num_Keypad);
				
				}
				return 2;
			}
			vSetValueParametro(hw.ucStringNumKeypad_out,idxParametro,&if_PM[0]);
			switch(idxParametro){
				case enumList_PM_fs_assorb_ruote:
					macParms.fFSassorbRuote=ipm.theFloat;
					break;
				case enumList_PM_tipoControlloTaglio:
					if (macParms.ucUsaSensoreTaglio){
						vInitSensoreTaglio();
					}
					break;
			    // se modificata durata attivazione bandella, reinizializzo bandella 
			    // cosi' abilitazione/disabilitazione viene fatta al volo
				case enumList_PM_durata_bandella:
					macParms.uiDurataAttivazioneBandella_centesimi_secondo=ipm.uiTheUInt;
					vInitBandella();
					break;
				case enumList_PM_ritardo_bandella:
					macParms.uiRitardoAttivazioneBandella_centesimi_secondo=ipm.uiTheUInt;
					break;

				case enumList_PM_password:
					macParms.password=ipm.ulTheULong;
					break;

				case enumList_PM_mmgiro:
					macParms.mmGiro=ipm.theFloat;
					impToMm = macParms.mmGiro / (float)macParms.impGiro;
					break;
                
				case enumList_PM_diam_test_intrz:
					macParms.DiamTestIntrz=ipm.theFloat;
					break;
				case enumList_PM_serial_number:
					//ulBoxSerialNumber=ipm.ulTheULong;
					// scrittura del serial number del box...
					//pWrite_private_parameter(enum_private_param_box_serial_number,(unsigned char xdata *)&ulBoxSerialNumber);
					break;
				case enumList_PM_impgiro:
				   macParms.impGiro = ipm.uiTheUInt*N_FASI_ENCODER;
				   macParms.lenSemigiro = macParms.impGiro / 2;
				   impToMm = macParms.mmGiro / macParms.impGiro;
				   break;

				case enumList_PM_iAsse_0:
					macParms.iasse[0]=ipm.theFloat;
					if (macParms.iasse[1]>macParms.iasse[0]){
						spiralatrice.CorsaColtello=macParms.iasse[1]-macParms.iasse[0];
					}
					break;
				case enumList_PM_iAsse_1:
					macParms.iasse[1]=ipm.theFloat;
					if (macParms.iasse[1]>macParms.iasse[0]){
						spiralatrice.CorsaColtello=macParms.iasse[1]-macParms.iasse[0];
					}
					break;
			}
			ValidateMacParms();
			ucSaveMacParms=1;
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enum_PM_NumOfButtons;i++)
				ucCreateTheButton(i);
			ipm.ucIdxFirstParametro=0;
			ucEnableStartLavorazione(0);
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Active:
			// se ancora non � stata verificata la password, passo a chiederla...
			if (!sLCD.ucPasswordChecked){
				vSetNumK_to_ask_password();
				defSetWinParam(defPM_NumParametroMacchina,200);
				// chiamo il numeric keypad
				ucCallWindow(enumWinId_Num_Keypad);
				// indico di rinfrescare la finestra
				return 2;
			}
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enum_PM_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enum_PM_Button_0:
						case enum_PM_Button_1:
						case enum_PM_Button_2:
						case enum_PM_Button_3:
						case enum_PM_Button_4:
						case enum_PM_Button_Desc_0:
						case enum_PM_Button_Desc_1:
						case enum_PM_Button_Desc_2:
						case enum_PM_Button_Desc_3:
						case enum_PM_Button_Desc_4:
							if ((i>=enum_PM_Button_0)&&(i<=enum_PM_Button_4))
								idxParametro=ipm.ucIdxFirstParametro+i-enum_PM_Button_0;
							else if ((i>=enum_PM_Button_Desc_0)&&(i<=enum_PM_Button_Desc_4))
								idxParametro=ipm.ucIdxFirstParametro+i-enum_PM_Button_Desc_0;
							else
								break;
							// se premuto uno dei 2 ultimi pulsanti (versione fpga o mac address ethernet, non faccio nulla)
							if (idxParametro>=sizeof(if_PM)/sizeof(if_PM[0])-2){
								break;
							}
							// se premuto versione cnc e lcd, se start/stop premuti reboot)
							if (idxParametro>=sizeof(if_PM)/sizeof(if_PM[0])-4){

								// se start o stop non premuti, non chiamo aggiornamento!!!!
								if (  !(InpDig & IDG_START)
									||!(InpDig & IDG_STOP)
									) {
									// PER PROVA, DA QUI CHIAMAVO SALVA PARAMETRI MACCHINA...
									//vSaveMachineParsOnFile();
									break;
								}
								v_reset_az4186();
								break;
							}
							// richiesta modifica del box serial number
							// oppure modifica quinta portata
							// oppure abilita/disabilita distanziatore
							// oppure regolazione velocit�
							// --> allora serve abilitazione superuser...
							//if (if_PM[idxParametro].pc==(unsigned char xdata*)&ulBoxSerialNumber){
							if (ucSuperUserPasswordRequired[idxParametro]){
								if (!ucSuperUser_passwordChecked){
									uc_super_user_password[0]=0;
									uc_use_hexadecimal_letters=1;
									ui_super_user_password_cycle=0;
									vSetNumK_to_ask_password_super_user(ui_super_user_password_cycle);
									defSetWinParam(defPM_NumParametroMacchina,400);
									// chiamo il numeric keypad
									ucCallWindow(enumWinId_Num_Keypad);
									// indico di rinfrescare la finestra
									return 2;
								}
							}
							if (if_PM[idxParametro].enumIsAstring){
								stringType=if_PM[idxParametro].enumIsAstring;
								if (++(if_PM[idxParametro].pc[0])>scegliStringa[stringType].ucLastVal)
									if_PM[idxParametro].pc[0]=scegliStringa[stringType].ucFirstVal;
								ucSaveMacParms=1;
								break;
							}
							// rinfresco il valore contenuto nella struttura ipm...
							vRefresh_IPM_values(idxParametro);

							paramsNumK.enumUm=if_PM[idxParametro].enumUm;
							// imposto limiti min e max
							paramsNumK.fMin=if_PM[idxParametro].fMin;
							paramsNumK.fMax=if_PM[idxParametro].fMax;
							if ((nvram_struct.actUM==UM_IN)&&ucIsUmSensitive(if_PM[idxParametro].enumUm)){
								paramsNumK.fMin*=if_PM[idxParametro].fConvUm_mm_toUm_inch;
								paramsNumK.fMax*=if_PM[idxParametro].fConvUm_mm_toUm_inch;
								paramsNumK.enumUm++;
							}
							paramsNumK.ucMaxNumChar=if_PM[idxParametro].ucLenField;
							// imposto stringa di picture
							mystrcpy((char*)paramsNumK.ucPicture,(char*)if_PM[idxParametro].ucPictField,sizeof(paramsNumK.ucPicture)-1);
							// titolo che deve comparire...
							//strcpy(paramsNumK.ucTitle,if_PM[idxParametro].ucTitleNumK);
							vStringLangCopy(paramsNumK.ucTitle,if_PM[idxParametro].stringa);
							// copio la stringa con cui si inizializza il numeric keypad
							// sprintf(hw.ucStringNumKeypad_in,if_PM[idxParametro].ucFmtField,if_PM[idxParametro].pc);
							vGetValueParametro(hw.ucStringNumKeypad_in,idxParametro,&if_PM[0]);

							// imposto abilitazione uso stringa picture
							paramsNumK.ucPictureEnable=if_PM[idxParametro].ucPictureEnable;	// abilitazione stringa picture
							// imposto abilitazione uso min/max
							paramsNumK.ucMinMaxEnable=if_PM[idxParametro].ucMinMaxEnable;	// abilitazione controllo valore minimo/massimo
							defSetWinParam(defPM_NumParametroMacchina,idxParametro);
							// chiamo il numeric keypad
							ucCallWindow(enumWinId_Num_Keypad);
							// indico di rinfrescare la finestra
							return 2;
						case enum_PM_Title:
						case enum_PM_Esc:
						case enum_PM_Sx:
						case enum_PM_Dx:
							ucEnableStartLavorazione(1);
							vHandleEscFromMenu();
							// indico di rinfrescare la finestra
							return 2;

						case enum_PM_Up:
							if (ipm.ucIdxFirstParametro>0){
								if (ipm.ucIdxFirstParametro>defPM_NumOfParsVisualized)
									ipm.ucIdxFirstParametro-=defPM_NumOfParsVisualized;
								else
									ipm.ucIdxFirstParametro=0;
							}
							// roll over
							else{
								ipm.ucIdxFirstParametro=defTotalNumParsMacchinaToVisualize-defPM_NumOfParsVisualized;
							}
							break;
						case enum_PM_Dn:
							if (ipm.ucIdxFirstParametro+defPM_NumOfParsVisualized<defTotalNumParsMacchinaToVisualize){
								if (ipm.ucIdxFirstParametro+2*defPM_NumOfParsVisualized>defTotalNumParsMacchinaToVisualize)
									ipm.ucIdxFirstParametro=defTotalNumParsMacchinaToVisualize-defPM_NumOfParsVisualized;
								else
									ipm.ucIdxFirstParametro+=defPM_NumOfParsVisualized;
							}
							break;
					}
				}
			}
			//strcpy(hw.ucString,"PARAMETRI MACCHINA");
			vStringLangCopy(hw.ucString,enumStr20_ParametriMacchina);
			//ucPrintStaticButton(hw.ucString,defParMac_button_Title_row ,defParMac_button_Title_col,enumFontMedium,enum_PM_Title,defLCD_Color_LightGreen);
			ucPrintTitleButton(hw.ucString,defParMac_button_Title_row ,defParMac_button_Title_col,enumFontMedium,enum_PM_Title,defLCD_Color_Trasparente,1);

			for (i=0;i<defPM_NumOfParsVisualized;i++){
				idxParametro=ipm.ucIdxFirstParametro+i;
				// sto sempre attento a non andare out of bounds...
				if (idxParametro>=defTotalNumParsMacchinaToVisualize)
					break;
				vRefresh_IPM_values(idxParametro);

				if (idxParametro>=sizeof(if_PM)/sizeof(if_PM[0])-4){
					if (idxParametro==sizeof(if_PM)/sizeof(if_PM[0])-3)
						sprintf(hw.ucString,"%i.%i",(int)sLCD.FW1,(int)sLCD.FW2);
					else if (idxParametro==sizeof(if_PM)/sizeof(if_PM[0])-4)
						sprintf(hw.ucString,"%s ",ucFirmwareVersion);
					else if (idxParametro==sizeof(if_PM)/sizeof(if_PM[0])-2)
						sprintf(hw.ucString,"%i.%i ",ucFpgaFirmwareVersion_Hi,ucFpgaFirmwareVersion_Low);
					else if (idxParametro==sizeof(if_PM)/sizeof(if_PM[0])-1){
                        {
                            // risolve bug di visualizzazione non corretta in caso di mac address bytes con bit 7 impostato (es C0 diventava FFC0)
                            unsigned int ui_codes[6],i;
                            pc=pcGetAnybusMacAddress();
                            i=0;
                            while (i<6){
                                ui_codes[i]=pc[i]&0xff;
                                i++;
                            }
                            sprintf(hw.ucString,"%02X-%02X-%02X-%02X-%02X-%02X",ui_codes[0],ui_codes[1],ui_codes[2],ui_codes[3],ui_codes[4],ui_codes[5]);
                        }
                    
					}
				}
				else{
					if (if_PM[idxParametro].enumIsAstring){
						stringType=if_PM[idxParametro].enumIsAstring;
						if (  (if_PM[idxParametro].pc[0]>scegliStringa[stringType].ucLastVal)
							||(if_PM[idxParametro].pc[0]<scegliStringa[stringType].ucFirstVal)
							)
							if_PM[idxParametro].pc[0]=scegliStringa[stringType].ucFirstVal;
						//strcpy(hw.ucString,ucEnumStringTable[scegliStringa[stringType].ucFirstStringIdx+if_PM[idxParametro].pc[0]]);
						vStringLangCopy(hw.ucString,enumStr20_FirstEnum+scegliStringa[stringType].ucFirstStringIdx+if_PM[idxParametro].pc[0]);
					}
					else{
						vGetValueParametro(hw.ucString,idxParametro,&if_PM[0]);
						// trovo la colonna cui stampare il parametro, in modo che sia allineato a dx
						// per ora la formula � scritta di getto... numero di caratteri deve essere multiplo di 2,
						// 16 � la larghezza di ogni char nel font medium, 8 � il margine del button
						// con un ulteriore margine di 4...
					}
				}
				uiColonnaParametro=defLcdWidthX_pixel/2+8+4;
				if (strlen(hw.ucString)>16){
					hw.ucString[18]=0;
					ucPrintStaticButton(hw.ucString,defParMac_button_ParValue_row_smallfont+i*defPM_OffsetBetweenRows,uiColonnaParametro-3*8,enumFontSmall,enum_PM_Button_0+i,defLCD_Color_Trasparente);
				}
				else if (strlen(hw.ucString)>8){
					sprintf(hw.ucString,"%-16.16s",hw.ucString);
					ucPrintStaticButton(hw.ucString,defParMac_button_ParValue_row_smallfont+i*defPM_OffsetBetweenRows,uiColonnaParametro,enumFontSmall,enum_PM_Button_0+i,defLCD_Color_Trasparente);
				}
				else{
					sprintf(hw.ucString,"%-8.8s",hw.ucString);
					ucPrintStaticButton(hw.ucString,defParMac_button_ParValue_row_normalfont+i*defPM_OffsetBetweenRows,uiColonnaParametro,enumFontMedium,enum_PM_Button_0+i,defLCD_Color_Trasparente);
				}
				//strcpy(hw.ucString,if_PM[idxParametro].ucTitleNumK);
				vStringLangCopy(hw.ucString,if_PM[idxParametro].stringa);
				//ucPrintStaticButton(hw.ucString,defParMac_button_ParDesc_row+i*defPM_OffsetBetweenRows ,defParMac_button_ParDesc_col,enumFontSmall,enum_PM_Button_Desc_0+i,defLCD_Color_Trasparente);
				// stampo un button di tipo title...
			    ucPrintTitleButton(hw.ucString,defParMac_button_ParDesc_row+i*defPM_OffsetBetweenRows ,defParMac_button_ParDesc_col,enumFontSmall,enum_PM_Button_Desc_0+i,defLCD_Color_Trasparente,2);
				if (if_PM[idxParametro].enumUm){
					if ((if_PM[idxParametro].enumUm==enumUm_password)||(if_PM[idxParametro].enumUm==enumUm_modify_password))
						sprintf(hw.ucString,"%20.20s "," ");
					else if ((nvram_struct.actUM==UM_IN)&&ucIsUmSensitive(if_PM[idxParametro].enumUm)){
						sprintf(hw.ucString,"%20.20s ",pucStringLang(enumStr20_FirstUm+if_PM[idxParametro].enumUm+1));
					}
					else
						sprintf(hw.ucString,"%20.20s ",pucStringLang(enumStr20_FirstUm+if_PM[idxParametro].enumUm));
				}
				else
					strcpy(hw.ucString,"                     ");
				ucPrintSmallText_ColNames(hw.ucString,defParMac_button_Par_um_row+i*defPM_OffsetBetweenRows,0);
			}
			// routine che permette di aggiungere ad una finestra i bottoni standard sx, dx, up, dn, esc, cut
			vAddStandardButtons(enum_PM_Sx);

			return 1;
	}//switch(uiGetStatusCurrentWindow())

}//unsigned char ucHW_ParametriMacchina(void)

#define defMaxCharStringaFornitore 30
typedef char tipoStringaFornitore[defMaxCharStringaFornitore+1];

typedef enum{
		enumFORN_NumeroSpirePretaglio=0,
		enumFORN_TolleranzaFilo,
		enumFORN_DiametroLama,
		enumFORN_DiametroRuota1,
		enumFORN_DiametroRuota2,
		enumFORN_TaraturaPosizioneRiposoTaglio,
		enumFORN_NumOf
	}enumButtonsFornitori;


#define defNumEntriesFornitori (sizeof(nvram_struct.TabFornitori)/sizeof(nvram_struct.TabFornitori[0]))

typedef struct _TipoStructHandleFornitori{
	unsigned char ucFirstLine;
	float fPercentualeForni;
}TipoStructHandleFornitori;

xdata TipoStructHandleFornitori hforn;

code TipoStructInfoParametro_Lingua if_FORN[]={
	{0,4, "NNNN","%-4.1f",MIN_VAR_RES,MAX_VAR_RES,(unsigned char xdata*)&hforn.fPercentualeForni,           1,1,enumStr20_wire_res_tolerance,enumPT_float, enumUm_percentuale,enumStringType_none,1}
};


// FORNITORI...
unsigned char ucHW_Fornitori(void){
	#define defFORN_title_row 6
	#define defFORN_title_col 0

	#define defFORN_Offset_between_buttons 38

	#define defFORN_button_1_row 50
	#define defFORN_button_1_col 0


	#define defFORN_OK_row (defLcdWidthY_pixel-36)
	#define defFORN_OK_col (defLcdWidthX_pixel-32*3-8)

	#define defFORN_Up_row (defFORN_title_row+28)
	#define defFORN_Up_col (0)

	#define defFORN_Dn_row (defFORN_OK_row)
	#define defFORN_Dn_col (0)

	#define defNumRowMenuFORN 4

	#define defFORN_NumFornitore 0

	// pulsanti della finestra
	typedef enum {
			enumFORN_button_1=0,
			enumFORN_button_2,
			enumFORN_button_3,
			enumFORN_button_4,
			enumFORN_descbutton_1,
			enumFORN_descbutton_2,
			enumFORN_descbutton_3,
			enumFORN_descbutton_4,
			enumFORN_umbutton_1,
			enumFORN_umbutton_2,
			enumFORN_umbutton_3,
			enumFORN_umbutton_4,
			enumFORN_Title,

			enumFORN_Sx,
			enumFORN_Up,
			enumFORN_Cut,
			enumFORN_Esc,
			enumFORN_Dn,
			enumFORN_Dx,

			enumFORN_NumOfButtons
		}enumButtons_APP;
	xdata unsigned char i;
	xdata unsigned char idxParametro;

	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			idxParametro=defGetWinParam(defFORN_NumFornitore);
			if ((idxParametro>=0)&&(idxParametro<sizeof(nvram_struct.TabFornitori)/sizeof(nvram_struct.TabFornitori[0]))){
				nvram_struct.TabFornitori[idxParametro]=atof(hw.ucStringNumKeypad_out);
				vValidateTabFornitori();
			}
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enumFORN_NumOfButtons;i++)
				ucCreateTheButton(i);
			hforn.ucFirstLine=0;
			vValidateTabFornitori();
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			// indico che non � stata effettuata nessuna scelta del fornitore, al momento...
			defSetWinParam(defIdxParam_enumWinId_Fornitori_Choice,0xFF);
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enumFORN_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enumFORN_Up:
							if (hforn.ucFirstLine>=defNumRowMenuFORN)
								hforn.ucFirstLine-=defNumRowMenuFORN;
							else
								hforn.ucFirstLine=0;
							break;
						case enumFORN_Dn:
							if (hforn.ucFirstLine+defNumRowMenuFORN<defNumEntriesFornitori){
								hforn.ucFirstLine+=defNumRowMenuFORN;
							}
							else{
								hforn.ucFirstLine=defNumEntriesFornitori-1;
							}
							if (hforn.ucFirstLine+defNumRowMenuFORN>=defNumEntriesFornitori){
								hforn.ucFirstLine=defNumEntriesFornitori-defNumRowMenuFORN;
							}
							break;
						case enumFORN_Cut:
							/* Indico di eseguire un taglio asincrono. */
							vDoTaglio();
						    // non occorre rinfrescare la finestra
							break;
						case enumFORN_Title:
						case enumFORN_Esc:
							vHandleEscFromMenu();
							// indico di rinfrescare la finestra
							return 2;

						case enumFORN_Sx:
						case enumFORN_Dx:
							ucReturnFromWindow();
							return 2;
						case enumFORN_button_1:
						case enumFORN_button_2:
						case enumFORN_button_3:
						case enumFORN_button_4:
							idxParametro=i-enumFORN_button_1+hforn.ucFirstLine;
							// la tabella pu� essere modificata???
							// se non pu� essere modificata, vuol dire che posso solo scegliere la tolleranza
							if (!defGetWinParam(defIdxParam_enumWinId_Fornitori_ModifyEnable)){
								defSetWinParam(defIdxParam_enumWinId_Fornitori_Choice,idxParametro);
								ucReturnFromWindow();
							}
							else{
								defSetWinParam(defFORN_NumFornitore,idxParametro);
								paramsNumK.enumUm=if_FORN[0].enumUm;
								// imposto limiti min e max
								paramsNumK.fMin=if_FORN[0].fMin;
								paramsNumK.fMax=if_FORN[0].fMax;
								if ((nvram_struct.actUM==UM_IN)&&ucIsUmSensitive(if_FORN[0].enumUm)){
									paramsNumK.fMin*=if_FORN[0].fConvUm_mm_toUm_inch;
									paramsNumK.fMax*=if_FORN[0].fConvUm_mm_toUm_inch;
									paramsNumK.enumUm++;
								}
								paramsNumK.ucMaxNumChar=if_FORN[0].ucLenField;
								// imposto stringa di picture
								mystrcpy((char*)paramsNumK.ucPicture,(char*)if_FORN[0].ucPictField,sizeof(paramsNumK.ucPicture)-1);
								// titolo che deve comparire...
								//strcpy(paramsNumK.ucTitle,if_FORN[0].ucTitleNumK);
								vStringLangCopy(paramsNumK.ucTitle,enumStr20_wire_res_tolerance);
								// copio la stringa con cui si inizializza il numeric keypad
								sprintf(hw.ucStringNumKeypad_in,if_FORN[0].ucFmtField,nvram_struct.TabFornitori[idxParametro]);

								// imposto abilitazione uso stringa picture
								paramsNumK.ucPictureEnable=if_FORN[0].ucPictureEnable;	// abilitazione stringa picture
								// imposto abilitazione uso min/max
								paramsNumK.ucMinMaxEnable=if_FORN[0].ucMinMaxEnable;	// abilitazione controllo valore minimo/massimo
								// chiamo il numeric keypad
								ucCallWindow(enumWinId_Num_Keypad);
							}
							// indico di rinfrescare la finestra
							return 2;
					}
				}
			}



			//strcpy(hw.ucString," TABELLA FORNITORI ");
			vStringLangCopy(hw.ucString,enumStr20_TabellaFornitori);
			//ucPrintStaticButton(hw.ucString,defFORN_title_row,defFORN_title_col,enumFontMedium,enumFORN_Title,defLCD_Color_Green);
			ucPrintTitleButton(hw.ucString,defFORN_title_row,defFORN_title_col,enumFontMedium,enumFORN_Title,defLCD_Color_Trasparente,1);
			for (i=0;i<defNumRowMenuFORN;i++){
				idxParametro=i+hforn.ucFirstLine;
				if (idxParametro>=defNumEntriesFornitori)
					break;
				sprintf(hw.ucString,"%4.1f",nvram_struct.TabFornitori[idxParametro]);
				ucPrintStaticButton(hw.ucString,defFORN_button_1_row+defFORN_Offset_between_buttons*i ,defFORN_button_1_col+13U*16 ,enumFontMedium,enumFORN_button_1+i,defLCD_Color_Trasparente);
				sprintf(hw.ucString,"%-11.11s%1i",pucStringLang(enumStr20_toll),idxParametro);
				ucPrintTitleButton(hw.ucString,defFORN_button_1_row+defFORN_Offset_between_buttons*i ,0 ,enumFontMedium,enumFORN_descbutton_1+i,defLCD_Color_Trasparente,0);
				strcpy(hw.ucString,"%");
				ucPrintTitleButton(hw.ucString,defFORN_button_1_row+defFORN_Offset_between_buttons*i ,defLcdWidthX_pixel-24-12 ,enumFontMedium,enumFORN_umbutton_1+i,defLCD_Color_Trasparente,0);
			}

			vAddStandardButtons(enumFORN_Sx);


			return 1;
	}
}//unsigned char ucHW_Fornitori(void)



// funzione che dato un testo a display lo traduce in mappa icone o bitmap
void vPrintTesto(unsigned int uiRiga,unsigned int uiColonna, unsigned char xdata *pTesto){
	pParamsIcon->uiYpixel=uiRiga;
	pParamsIcon->uiXpixel=uiColonna;
	pParamsIcon->ucFont=1; // font 16x16
	pParamsIcon->pucVettore=pTesto;
	pParamsIcon->ucIconColor=defLCD_Color_White; 
	pParamsIcon->ucBackgroundColor=defLCD_Color_Blue;
	vWriteIconElement(pParamsIcon);
}


void vRefreshScreen(void){
	vCloseListDisplayElements();
	invio_lista();
	LCD_LoadImageOnBackgroundBank(13);
	applicazione_lista();
	LCD_toggle_bank();
	vInitListDisplayElements();
}


void vPrintMessage(char *s){
	vRefreshScreen();
	strcpy(hw.ucString,s);
	vPrintTesto(100,4,hw.ucString);
	vRefreshScreen();
}


void vResetRamAndReboot(void){
	// fermo tutte le interrupt
	vLCD_Init();
	vRefreshScreen();
	strcpy(hw.ucString,"WAIT RAM RESET...");
	vPrintTesto(100,4,hw.ucString);
	vRefreshScreen();
// reset variabili non volatili!	
	memset(&nvram_struct,0,sizeof(nvram_struct));
//	vInitDataAndBssSections();
	vLCD_Init();
	// pulisco lo schermo
	vRefreshScreen();
	strcpy(hw.ucString,"RAM RESET OK");
	vPrintTesto(100,4,hw.ucString);
	vRefreshScreen();
	// faccio tre beep lcd per avvisare che ha fatto reset
	LCD_Beep();
	v_wait_microseconds(300*1000);
	LCD_Beep();
	v_wait_microseconds(300*1000);
	LCD_Beep();
	strcpy(hw.ucString,"REBOOT...");
	vPrintTesto(100,4,hw.ucString);
	vRefreshScreen();
	v_wait_microseconds(300*1000);

    v_reset_az4186();
}


#define defMaxCharStringaUtilities 20
typedef char tipoStringaUtilities[defMaxCharStringaUtilities+1];

// per inserimento ohm valore di resistenza di taratura
code enumStringhe20char ucNomiButtonsUtilities[]={
	enumStr20_Tarature,
	enumStr20_ParametriMacchina,
	enumStr20_IngressiDigitali,
	enumStr20_UsciteDigitali,
	enumStr20_IngressiAnalogici,
	enumStr20_UsciteAnalogiche,
	enumStr20_IngressiEncoder,
	enumStr20_Log,
	enumStr20_SaveParams,
	enumStr20_LOADPARMAC,
	enumStr20_LoadCodiciProdotto,
	enumStr20_LoadCustomLanguage,
	enumStr20_ResetCodiciProdotto,
	enumStr20_DataOra,
	enumStr20_ResetRam
};

#define defNumEntriesUtilities (sizeof(ucNomiButtonsUtilities)/sizeof(ucNomiButtonsUtilities[0]))

typedef struct _TipoStructHandleUtilities{
	unsigned char ucFirstLine;
}TipoStructHandleUtilities;

xdata TipoStructHandleUtilities hu;



// utilit�...
unsigned char ucHW_Utilities(void){

	#define defUT_NumParametro 0

	#define defUT_title_row 6
	#define defUT_title_col 0

	#define defUT_Offset_between_buttons 30

	#define defUT_button_1_row 40
	#define defUT_button_1_col 0


	#define defUT_OK_row (defLcdWidthY_pixel-32-8)
	#define defUT_OK_col (defLcdWidthX_pixel-32*3-8)

	#define defUT_Up_row (defUT_title_row+28)
	#define defUT_Up_col (0)

	#define defUT_Dn_row (defUT_OK_row)
	#define defUT_Dn_col (0)

	#define defNumRowMenuUtils 6

	// pulsanti della finestra
	typedef enum {
			enumUT_button_1=0,
			enumUT_button_2,
			enumUT_button_3,
			enumUT_button_4,
			enumUT_button_5,
			enumUT_button_6,
			enumUT_Title,

			enumUT_Sx,
			enumUT_Up,
			enumUT_Cut,
			enumUT_Esc,
			enumUT_Dn,
			enumUT_Dx,

			enumUT_NumOfButtons
		}enumButtons_utilities;
	xdata unsigned char i;


	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			switch(defGetWinParam(defUT_NumParametro)){
#if 1
				// visualizzata finestra password non corretta
				case 201:
					sLCD.ucPasswordChecked=0;
					// chiamo il main menu...
					vJumpToWindow(enumWinId_MainMenu);
					return 2;
				// check password???
				case 200:
					if (ucVerifyPasswordCorrect(hw.ucStringNumKeypad_out)){
						sLCD.ucPasswordChecked=1;
						vResetRamAndReboot();
						return 2;
					}
					// visualizzo la finestra: password non corretta
					// numero msg da visualizzare: 1
					ucSetWindowParam(enumWinId_Info,0, 1);
					// messaggio da visualizzare: password bad
					ucSetWindowParam(enumWinId_Info,1, enumStr20_bad_password);
					// messaggio: password KO
					defSetWinParam(defUT_NumParametro,201);
					// chiamo la finestra info
					ucCallWindow(enumWinId_Info);
					return 2;
#endif
				default:
					break;
			}
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enumUT_NumOfButtons;i++)
				ucCreateTheButton(i);
			hu.ucFirstLine=0;
			// da pagina utilities non si pu� dare start alla lavorazione
			ucEnableStartLavorazione(0);
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enumUT_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enumUT_Cut:
							/* Indico di eseguire un taglio asincrono. */
							vDoTaglio();
							// non occorre rinfrescare la finestra
							break;
						case enumUT_Up:
							if (hu.ucFirstLine>=defNumRowMenuUtils)
								hu.ucFirstLine-=defNumRowMenuUtils;
							else
								hu.ucFirstLine=0;
							break;
						case enumUT_Dn:
							if (hu.ucFirstLine+defNumRowMenuUtils<defNumEntriesUtilities)
								hu.ucFirstLine+=defNumRowMenuUtils;
							//else
							//	hu.ucFirstLine=defNumEntriesUtilities-1;
							break;
						case enumUT_Esc:
						case enumUT_Sx:
						case enumUT_Dx:
							ucEnableStartLavorazione(1);
							vHandleEscFromMenu();
							return 2;
						case enumUT_button_1:
						case enumUT_button_2:
						case enumUT_button_3:
						case enumUT_button_4:
						case enumUT_button_5:
						case enumUT_button_6:
							switch(ucNomiButtonsUtilities[i-enumUT_button_1+hu.ucFirstLine]){
								case enumStr20_ResetRam:
                                    if (!PrgRunning)
                                    {
                                        vSetNumK_to_ask_password();
                                        defSetWinParam(defUT_NumParametro,200);
                                        // chiamo il numeric keypad
                                        ucCallWindow(enumWinId_Num_Keypad);
                                        // indico di rinfrescare la finestra
                                        return 2;
                                    }
                                    break;
								case enumStr20_IngressiDigitali:
									ucCallWindow(enumWinId_IngressiDigitali);
									return 2;
								case enumStr20_UsciteDigitali:
									ucCallWindow(enumWinId_UsciteDigitali);
									return 2;
								case enumStr20_IngressiAnalogici:
									ucCallWindow(enumWinId_IngressiAnalogici);
									return 2;
								case enumStr20_UsciteAnalogiche:
									ucCallWindow(enumWinId_UsciteAnalogiche);
									return 2;
								case enumStr20_Tarature:
									ucCallWindow(enumWinId_SelezionaTaratura);
									return 2;
								case enumStr20_ParametriMacchina:
									if (PrgRunning)
										break;
									ucCallWindow(enumWinId_ParametriMacchina);
									return 2;
								case enumStr20_IngressiEncoder:
									ucCallWindow(enumWinId_IngressiEncoder);
									return 2;
								case enumStr20_Log:
									ucCallWindow(enumWinId_LogProduzione);
									return 2;
								case enumStr20_SaveParams:
									if (PrgRunning)
										break;
									ucCallWindow(enumWinId_SaveParams);
									return 2;
								case enumStr20_DataOra:
									//if (PrgRunning)
									//	break;
									ucCallWindow(enumWinId_DataOra);
									return 2;
								case enumStr20_LOADPARMAC:
									if (PrgRunning)
										break;
									ucCallWindow(enumWinId_LoadParsMac);
									return 2;
								case enumStr20_LoadCodiciProdotto:
									if (PrgRunning)
										break;
									ucCallWindow(enumWinId_LoadCodiciProdotto);
									return 2;
								case enumStr20_LoadCustomLanguage:
									if (PrgRunning)
										break;
									ucCallWindow(enumWinId_LoadCustomLanguage);
									return 2;
								case enumStr20_ResetCodiciProdotto:
									if (PrgRunning)
										break;
									ucCallWindow(enumWinId_ResetCodiciProdotto);
									return 2;

							}
							break;
					}
				}
			}



			//strcpy(hw.ucString,"      Utilities     ");
			vStringLangCopy(hw.ucString,enumStr20_Utilities);
			//ucPrintStaticButton(hw.ucString,defUT_title_row,defUT_title_col,enumFontMedium,enumUT_Title,defLCD_Color_Green);
			ucPrintTitleButton(hw.ucString,defUT_title_row,defUT_title_col,enumFontMedium,enumUT_Title,defLCD_Color_Trasparente,1);
			for (i=0;i<defNumRowMenuUtils;i++){
				if (i+hu.ucFirstLine>=defNumEntriesUtilities)
					break;
				//strcpy(hw.ucString,ucNomiButtonsUtilities[i+hu.ucFirstLine]);
				//vStringLangCopy(hw.ucString,ucNomiButtonsUtilities[i+hu.ucFirstLine]);
				sprintf(hw.ucString,"%-20.20s",pucStringLang(ucNomiButtonsUtilities[i+hu.ucFirstLine]));
				ucPrintStaticButton(hw.ucString,defUT_button_1_row+defUT_Offset_between_buttons*i ,defUT_button_1_col ,enumFontMedium,enumUT_button_1+i,defLCD_Color_Trasparente);
			}
			vAddStandardButtons(enumUT_Sx);

			return 1;
	}
}//unsigned char ucHW_Utilities(void)

void vHandleButtonsRegulation(int iNumButtonRegSpindle){
#define defButtonsReg_row 190
#define defButtonsReg_col 0
	unsigned char uc_ramping;
    unsigned int ui;
	xdata long lValue;
	uc_ramping=spiralatrice.StatusRampaDac;
	vStringLangCopy(hw.ucString,enumStr_Spindle);
	// METTO IL VALORE INDICATO DAL POTENZIOMETRO DOPO LA STRINGA CHE IDENTIFICA L'INDIRIZZAMENTO
	sprintf(hw.ucString,"%s%4i",hw.ucString,iGetHRE_setting(enumSpeedRegulation_Spindle));
	if (ucLinkEncoder==enumSpeedRegulation_Spindle){
		if (!uc_ramping){
			/* Calcolo in spiralatrice.DacOutValue[0] la velocit� angolare del motore. */
			spiralatrice.DacOutValue[0]=iGetHRE_setting(enumSpeedRegulation_Spindle);
			/* Fisso il valore massimo degli rpm del mandrino. */
			spiralatrice.DacOutMaxValue[0]=macParms.rpm_mandrino;
			/* La coppia del mandrino non va controllata. */
			spiralatrice.DacOutValue[1]=1;
			spiralatrice.DacOutMaxValue[1]=1;
			SetDacValue(addrDacMandrino);
		}
		ui=defLCD_Color_Yellow;
	}
	else
		ui=defLCD_Color_Trasparente;
	ucPrintStaticButton(hw.ucString,defButtonsReg_row,defButtonsReg_col,enumFontMedium,iNumButtonRegSpindle,ui);

	//strcpy(hw.ucString,"WHEEL 1");
	vStringLangCopy(hw.ucString,enumStr_Wheel_1);
	// METTO IL VALORE INDICATO DAL POTENZIOMETRO DOPO LA STRINGA CHE IDENTIFICA L'INDIRIZZAMENTO
	lValue=iGetHRE_setting(enumSpeedRegulation_Wheel1);
	if (lValue>100){
		lValue=100;
	}
	else if (lValue<0){
		lValue=0;
	}
	sprintf(hw.ucString,"%s %03i",hw.ucString,(int)((lValue*macParms.rpm_frizioni[0])/100));
	if (ucLinkEncoder==enumSpeedRegulation_Wheel1){
		if (!uc_ramping){
			/* Calcolo in spiralatrice.DacOutValue[0] la velocit� angolare della ruota i. */
			spiralatrice.DacOutValue[0]=(lValue*macParms.rpm_frizioni[0])/100;
			/* Fisso il valore massimo degli rpm della ruota. */
			spiralatrice.DacOutMaxValue[0]=macParms.rpm_frizioni[0];
			/* La coppia della ruota non va controllata. */
			spiralatrice.DacOutValue[1]=1;
			spiralatrice.DacOutMaxValue[1]=1;

			SetDacValue(addrDacFrizione1+0);
		}
		ui=defLCD_Color_Yellow;
	}
	else
		ui=defLCD_Color_Trasparente;
	ucPrintStaticButton(hw.ucString,defButtonsReg_row,defButtonsReg_col+100,enumFontMedium,iNumButtonRegSpindle+1,ui);

	//strcpy(hw.ucString,"WHEEL 2");
	vStringLangCopy(hw.ucString,enumStr_Wheel_2);
	lValue=iGetHRE_setting(enumSpeedRegulation_Wheel2);
	if (lValue>100){
		lValue=100;
	}
	else if (lValue<0){
		lValue=0;
	}
	// METTO IL VALORE INDICATO DAL POTENZIOMETRO DOPO LA STRINGA CHE IDENTIFICA L'INDIRIZZAMENTO
	sprintf(hw.ucString,"%s %03i",hw.ucString,(int)((lValue*macParms.rpm_frizioni[1])/100));
	if (ucLinkEncoder==enumSpeedRegulation_Wheel2){
		if (!uc_ramping){
			/* Calcolo in spiralatrice.DacOutValue[0] la velocit� angolare della ruota i. */
			spiralatrice.DacOutValue[0]=(lValue*macParms.rpm_frizioni[1])/100;
			/* Fisso il valore massimo degli rpm della ruota. */
			spiralatrice.DacOutMaxValue[0]=macParms.rpm_frizioni[1];
			/* La coppia della ruota non va controllata. */
			spiralatrice.DacOutValue[1]=1;
			spiralatrice.DacOutMaxValue[1]=1;
			SetDacValue(addrDacFrizione1+1);
		}
		ui=defLCD_Color_Yellow;
	}
	else
		ui=defLCD_Color_Trasparente;
	ucPrintStaticButton(hw.ucString,defButtonsReg_row,defButtonsReg_col+218,enumFontMedium,iNumButtonRegSpindle+2,ui);

}

code unsigned char ucStringNamesInAnalogici[8][20]={
		"V_OHM_METER     ",
		"V_OHM_MEAS_WHEEL",
		"V_RPM           ",
		"V_LOAD1         ",
		"V_LOAD2         ",
		"V_WHEEL1        ",
		"V_WHEEL2        ",
		"V_CUT_POSITION  "
	};
// ingressi analogici...
unsigned char ucHW_InAnalog(void){
	#define defIA_title_row 2
	#define defIA_title_col 0

	#define defIA_Offset_between_buttons 24
	#define defIA_button_1_row 40
	#define defIA_button_1_col 0


	#define defIA_OK_row (defLcdWidthY_pixel-32-8)
	#define defIA_OK_col (defLcdWidthX_pixel-32*3-8)

	// pulsanti della finestra
	typedef enum {
			enumIA_button_1=0,
			enumIA_button_2,
			enumIA_button_3,
			enumIA_button_4,
			enumIA_button_5,
			enumIA_button_6,
			enumIA_button_7,
			enumIA_button_8,
		#ifdef def_canopen_enable
			enumIA_button_9,
		#endif
			enumIA_Sx,
			enumIA_Up,
			enumIA_Cut,
			enumIA_Esc,
			enumIA_Dn,
			enumIA_Dx,

			enumIA_Title,
			enumIA_NumOfButtons
		}enumButtons_IA_utilities;
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
			for (i=0;i<enumIA_NumOfButtons;i++)
				ucCreateTheButton(i);
			//sLCD.ucSuspendDigitalIoUpdate=1;
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			// ogni 400ms rinfresco gli ingressi digitali
			ucLcdRefreshTimeoutEnable(400);
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enumIA_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enumIA_Esc:
							sLCD.ucSuspendDigitalIoUpdate=0;
							// fine refresh lcd...
							ucLcdRefreshTimeoutEnable(0);
							vHandleEscFromMenu();
							// indico di rinfrescare la finestra
							return 2;

						case enumIA_Cut:
							/* Indico di eseguire un taglio asincrono. */
							vDoTaglio();
							// non occorre rinfrescare la finestra
							break;

						case enumIA_Sx:
						case enumIA_Dx:
							// fine refresh lcd...
							sLCD.ucSuspendDigitalIoUpdate=0;
							ucLcdRefreshTimeoutEnable(0);
							ucReturnFromWindow();
							return 2;

					}
				}
			}
#if 0
			if (!PrgRunning&&(outDigVariable&ODG_MOTORE)&&!StatusRampaDac){
				WriteOnDacByPot();
			}
#endif


			//strcpy(hw.ucString,"Ingressi analogici");
			vStringLangCopy(hw.ucString,enumStr20_IngressiAnalogici);
			//ucPrintStaticButton(hw.ucString,defIA_title_row,defIA_title_col,enumFontMedium,enumIA_Title,defLCD_Color_Green);
			ucPrintTitleButton(hw.ucString,defIA_title_row,defIA_title_col,enumFontMedium,enumIA_Title,defLCD_Color_Trasparente,1);

			sprintf(hw.ucString,"%-40.40s","                    STEP  VOLT");
			ucPrintSmallText_ColNames(hw.ucString,defIA_button_1_row-14,defIA_title_col);
			for (i=0;i<8;i++){
				sprintf(hw.ucString,"%-18.18s",ucStringNamesInAnalogici[i]);
				ucPrintSmallText_ColNames(hw.ucString,defIA_button_1_row+20*i,defIA_title_col);
				sprintf(hw.ucString,"%-4i %-6.3f",iArrayConvAD7327[i],fArrayConvAD7327_Volts[i]);
				ucPrintTitleButton(hw.ucString,defIA_button_1_row+20*i,defIA_title_col+18U*8+12,enumFontSmall,enumIA_button_1+i,defLCD_Color_Trasparente,2);
			}
			#ifdef def_canopen_enable
			{
				int i_temperature_binary,i_temp_of_cold_junction;
				// prelevo la temperatura misurata dalla termocoppia collegata la modulo phoenix
				ui_get_canopen_temperature(&i_temperature_binary);
				// prelevo anche la temeratura della cold junction(secondo PDI)
				ui_get_canopen_temperature_cold_junction(&i_temp_of_cold_junction);
				sprintf(hw.ucString,"%-18.18s","PROBE/COLDJ T*C");
				ucPrintSmallText_ColNames(hw.ucString,defIA_button_1_row+20*8,defIA_title_col);
				sprintf(hw.ucString,"%-5.1f / %-5.1f",(float)i_temperature_binary*0.1,(float)i_temp_of_cold_junction*0.1);
				{
					unsigned int ui_color_background;
					if (ui_alarm_reading_temperature()){
						ui_color_background=defLCD_Color_Yellow;
					}
					else{
						ui_color_background=defLCD_Color_Trasparente;
					}
					ucPrintTitleButton(hw.ucString,defIA_button_1_row+20*8,defIA_title_col+18U*8+12,enumFontSmall,enumIA_button_1+8,ui_color_background,2);
				}
			}
			#endif

			vAddStandardButtons(enumIA_Sx);

			return 1;
	}
}//unsigned char ucHW_InAnalog(void)



code unsigned char ucStringNamesOutAnalogici[8][20]={
		"V_SPINDLE       ",
		"V_CUT           ",
		"V_LOAD1         ",
		"V_NOT_USED      ",
		"V_LOAD2         ",
		"V_NOT_USED      ",
		"V_CUT_RESTE_POS ",
		"V_CUT_PRECUT_POS"
	};

// uscite analogiche...
unsigned char ucHW_OutAnalog(void){
	#define defOA_title_row 2
	#define defOA_title_col 0

	#define defOA_Offset_between_buttons 24
	#define defOA_button_1_row 40
	#define defOA_button_1_col 0


	#define defOA_OK_row (defLcdWidthY_pixel-32-8)
	#define defOA_OK_col (defLcdWidthX_pixel-32*3-8)

	// pulsanti della finestra
	typedef enum {
			enumOA_button_1=0,
			enumOA_button_2,
			enumOA_button_3,
			enumOA_button_4,
			enumOA_button_5,
			enumOA_button_6,
			enumOA_button_7,
			enumOA_button_8,

			enumOA_Sx,
			enumOA_Up,
			enumOA_Cut,
			enumOA_Esc,
			enumOA_Dn,
			enumOA_Dx,

			enumOA_Title,
			enumOA_NumOfButtons
		}enumButtons_OA_utilities;
	xdata unsigned char i;
	extern xdata unsigned char ucLastCommandTaglio;

	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enumOA_NumOfButtons;i++)
				ucCreateTheButton(i);
			//sLCD.ucSuspendDigitalIoUpdate=1;
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			// ogni 400ms rinfresco gli ingressi digitali
			ucLcdRefreshTimeoutEnable(400);
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enumOA_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enumOA_Esc:
							sLCD.ucSuspendDigitalIoUpdate=0;
							// fine refresh lcd...
							ucLcdRefreshTimeoutEnable(0);
							vHandleEscFromMenu();
							// indico di rinfrescare la finestra
							return 2;

						case enumOA_Sx:
						case enumOA_Dx:
							// fine refresh lcd...
							sLCD.ucSuspendDigitalIoUpdate=0;
							ucLcdRefreshTimeoutEnable(0);
							ucReturnFromWindow();
							return 2;

						case enumOA_Cut:
							/* Indico di eseguire un taglio asincrono. */
							vDoTaglio();
							// non occorre rinfrescare la finestra
							break;

					}
				}
			}
#if 0
			if (!PrgRunning&&(outDigVariable&ODG_MOTORE)&&!StatusRampaDac){
				WriteOnDacByPot();
		    }
#endif


			//strcpy(hw.ucString,"Uscite analogiche");
			vStringLangCopy(hw.ucString,enumStr20_UsciteAnalogiche);
			//ucPrintStaticButton(hw.ucString,defOA_title_row,defOA_title_col,enumFontMedium,enumOA_Title,defLCD_Color_Green);
			ucPrintTitleButton(hw.ucString,defOA_title_row,defOA_title_col,enumFontMedium,enumOA_Title,defLCD_Color_Trasparente,1);

			sprintf(hw.ucString,"%-40.40s","                    STEP  VOLT");
			ucPrintSmallText_ColNames(hw.ucString,defOA_button_1_row-14,defOA_title_col);

			for (i=0;i<8;i++){
				sprintf(hw.ucString,"%-18.18s",ucStringNamesOutAnalogici[i]);
				ucPrintSmallText_ColNames(hw.ucString,defOA_button_1_row+20*i,defOA_title_col);
				if (i==1)
					sprintf(hw.ucString,"%-4i %-6.3f",(int)ucLastCommandTaglio,ucLastCommandTaglio*(10.0/255));
				else
					sprintf(hw.ucString,"%-4i %-6.3f",(int)spiralatrice.DacOut[i],spiralatrice.DacOut[i]*(10.0/255));
				ucPrintTitleButton(hw.ucString,defOA_button_1_row+20*i,defOA_title_col+18U*8+12,enumFontSmall,enumOA_button_1+i,defLCD_Color_Trasparente,2);
			}

			vAddStandardButtons(enumOA_Sx);

			return 1;
	}
}//unsigned char ucHW_OutAnalog(void)




#if 0

code unsigned char ucStringhe_IDG[16][14]={
	"Bunched wire ",	// 1
	"Not used     ",	// 2
	"KnifeDrvFault",	// 4
	"Knife hi     ",    // 8
	"Not used     ",	// 10
	"Not used     ",	// 20
	"Measure valid",	// 40
	"Not used     ",	// 80
	"Start        ",	// 100
	"Stop         ",	// 200
	"Ohm test     ",	// 400
	"Wire 1 break ",	// 800
	"Wire 2 break ",	// 1000
	"Spindle fault",	// 2000
	"Wheel 1 fault",	// 4000
	"Wheel 2 fault"	    // 8000
};
#endif


// ingressi digitali...
unsigned char ucHW_Indig(void){
	#define defID_title_row 6
	#define defID_title_col 0

	#define defID_Offset_between_buttons 24
	#define defID_button_1_row 24
	#define defID_button_1_col 0


	#define defID_OK_row (defLcdWidthY_pixel-32-8)
	#define defID_OK_col (defLcdWidthX_pixel-32*3-8)

	#define defIdg_row (defID_title_row+14)
	#define defIdg_row_offset (24)
	#define defIdg_col 6
	#define defIdg_col2 (160)

	// pulsanti della finestra
	typedef enum {
			enumID_button_1=0,
			enumID_button_2,
			enumID_button_3,
			enumID_button_4,
			enumID_button_5,
			enumID_button_6,
			enumID_button_7,
			enumID_button_8,
			enumID_button_9,
			enumID_button_10,
			enumID_button_11,
			enumID_button_12,
			enumID_button_13,
			enumID_button_14,
			enumID_button_15,
			enumID_button_16,
			enumID_Title,

			enumID_Sx,
			enumID_Up,
			enumID_Cut,
			enumID_Esc,
			enumID_Dn,
			enumID_Dx,

			enumID_NumOfButtons
		}enumButtons_ID_utilities;
	xdata unsigned int i,uiAux;


	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enumID_NumOfButtons;i++)
				ucCreateTheButton(i);
			sLCD.ucSuspendDigitalIoUpdate=1;
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			// ogni 100ms rinfresco gli ingressi digitali
			ucLcdRefreshTimeoutEnable(200);
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enumID_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enumID_Up:
							ucReturnFromWindow();
							return 2;
						case enumID_Esc:
							sLCD.ucSuspendDigitalIoUpdate=0;
							// fine refresh lcd...
							ucLcdRefreshTimeoutEnable(0);
							vHandleEscFromMenu();
							// indico di rinfrescare la finestra
							return 2;

						case enumID_Sx:
						case enumID_Dx:
						case enumID_Dn:
							vJumpToWindow(enumWinId_UsciteDigitali);
							return 2;
						case enumID_Cut:
							/* Indico di eseguire un taglio asincrono. */
							vDoTaglio();
							// non occorre rinfrescare la finestra
							break;

					}
				}
			}


			//strcpy(hw.ucString,"Ingressi digitali");
			vStringLangCopy(hw.ucString,enumStr20_IngressiDigitali);
			ucPrintTitleButton(hw.ucString,defID_title_row,defID_title_col,enumFontSmall,enumID_Title,defLCD_Color_Trasparente,1);
			// 1.28 ingresso misura valida � attivo basso
			myInpDig=InpDig^IDG_MIS_VALID1;

			for (i=0;i<8;i++){
				//strcpy(hw.ucString,ucStringhe_IDG[i]);
				vStringLangCopy(hw.ucString,enumStr20_IDG_0+i);
				ucPrintSmallText_ColNames(hw.ucString,defIdg_row+6+defIdg_row_offset*i,defIdg_col);
				if (myInpDig&(1<<i))
					ucPrintBitmapButton(enumBitmap_40x24_ON,defIdg_row+defIdg_row_offset*i,defIdg_col+13*8,enumID_button_1+i);
				else
					ucPrintBitmapButton(enumBitmap_40x24_OFF,defIdg_row+defIdg_row_offset*i,defIdg_col+13*8,enumID_button_1+i);

				// strcpy(hw.ucString,ucStringhe_IDG[i+8]);
				vStringLangCopy(hw.ucString,enumStr20_IDG_0+i+8);
				ucPrintSmallText_ColNames(hw.ucString,defIdg_row+6+defIdg_row_offset*i,defIdg_col2);
				uiAux=i+8;
				if (myInpDig&(1<<uiAux))
					ucPrintBitmapButton(enumBitmap_40x24_ON,defIdg_row+defIdg_row_offset*i,defIdg_col2+13*8,enumID_button_1+uiAux);
				else
					ucPrintBitmapButton(enumBitmap_40x24_OFF,defIdg_row+defIdg_row_offset*i,defIdg_col2+13*8,enumID_button_1+uiAux);
			}
			vAddStandardButtons(enumID_Sx);
			// 1.28 ripristino lettura standard ingressi
			myInpDig=InpDig;

			return 1;
	}
}//unsigned char ucHW_Indig(void)



#if 0
code unsigned char ucStringhe_ODG[16][14]={
	"Range 0      ",	// 1
	"Range 1      ",	// 2
	"Range 2      ",	// 4
	"Ohm test     ",    // 8
	"Spindle motor",	// 10
	"Cut          ",	// 20
	"Precut       ",	// 40
	"Lamp         ",	// 80
	"Not used     ",	// 100
	"Not used     ",	// 200
	"Not used     ",	// 400
	"Not used     ",	// 800
	"Not used     ",	// 1000
	"Not used     ",	// 2000
	"Not used     ",	// 4000
	"Not used     "	    // 8000
};
#endif


// uscite digitali...
unsigned char ucHW_Outdig(void){
	#define defUD_title_row 6
	#define defUD_title_col 0

	#define defOdg_row (defID_title_row+14)
	#define defOdg_row_offset (24)
	#define defOdg_col 6
	#define defOdg_col2 (160)

	// pulsanti della finestra
	typedef enum {
			enumUD_button_1=0,
			enumUD_button_2,
			enumUD_button_3,
			enumUD_button_4,
			enumUD_button_5,
			enumUD_button_6,
			enumUD_button_7,
			enumUD_button_8,
			enumUD_button_9,
			enumUD_button_10,
			enumUD_button_11,
			enumUD_button_12,
			enumUD_button_13,
			enumUD_button_14,
			enumUD_button_15,
			enumUD_button_16,
			enumUD_button_spindle,
			enumUD_button_f1,
			enumUD_button_f2,
			enumUD_button_toggle,
			enumUD_Title,

			enumUD_Sx,
			enumUD_Up,
			enumUD_Cut,
			enumUD_Esc,
			enumUD_Dn,
			enumUD_Dx,

			enumUD_NumOfButtons
		}enumButtons_UD_utilities;
	xdata unsigned int i, uiAux;


	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enumUD_NumOfButtons;i++)
				ucCreateTheButton(i);
			sLCD.ucSuspendDigitalIoUpdate=1;
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			ucLinkEncoder=enumSpeedRegulation_Spindle;
			// versione 1.33 inizializzo il valore encoder!!
			// altrimenti rimango disallineato
			vSetHRE_potentiometer(enumSpeedRegulation_Spindle,0);
			vSetHRE_potentiometer(enumSpeedRegulation_Wheel1,FS_ADC/2);
			vSetHRE_potentiometer(enumSpeedRegulation_Wheel2,FS_ADC/2);
		    vSelectHRE_potentiometer(ucLinkEncoder);
			// ogni 100ms rinfresco le uscite digitali
			ucLcdRefreshTimeoutEnable(200);
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enumUD_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enumUD_button_1:
						case enumUD_button_2:
						case enumUD_button_3:
						case enumUD_button_4:
						case enumUD_button_5:
						case enumUD_button_6:
						case enumUD_button_7:
						case enumUD_button_8:
						case enumUD_button_9:
						case enumUD_button_10:
						case enumUD_button_11:
						case enumUD_button_12:
						case enumUD_button_13:
						case enumUD_button_14:
						case enumUD_button_15:
						case enumUD_button_16:
							if (PrgRunning)
								break;
							uiAux=i-enumUD_button_1;
							if (uiAux<8){
								uiAux=1<<uiAux;
								if (OutDig&uiAux)
									OutDig&=~uiAux;
								else{
									OutDig|=uiAux;
								}
								// se attivo/disattivo l'uscita motore, devo partire/finire con rpm=0
								if (uiAux==ODG_MOTORE){
									vDisableHRE_potentiometer();
									// versione 1.33 inizializzo il valore encoder!!
									// altrimenti rimango disallineato
									ucLinkEncoder=enumSpeedRegulation_Spindle;
									// versione 1.33 inizializzo il valore encoder!!
									// altrimenti rimango disallineato
									vSetHRE_potentiometer(enumSpeedRegulation_Spindle,0);
									vSetHRE_potentiometer(enumSpeedRegulation_Wheel1,FS_ADC/2);
									vSetHRE_potentiometer(enumSpeedRegulation_Wheel2,FS_ADC/2);
									vSelectHRE_potentiometer(ucLinkEncoder);
									// se motore viene fermato, reset completo
									if ((OutDig&ODG_MOTORE)==0)
										vDisableHRE_potentiometer();
									/* Calcolo in spiralatrice.DacOutValue[0] la velocit� angolare del motore. */
									spiralatrice.DacOutValue[0]=0;
									/* Fisso il valore massimo degli rpm del mandrino. */
									spiralatrice.DacOutMaxValue[0]=macParms.rpm_mandrino;
									/* La coppia del mandrino non va controllata. */
									spiralatrice.DacOutValue[1]=1;
									spiralatrice.DacOutMaxValue[1]=1;
									SetDacValue(addrDacMandrino);
									/* Fisso il valore massimo degli rpm della ruota. */
									spiralatrice.DacOutMaxValue[0]=macParms.rpm_frizioni[0];
									SetDacValue(addrDacFrizione1+0);
									/* Fisso il valore massimo degli rpm della ruota. */
									spiralatrice.DacOutMaxValue[0]=macParms.rpm_frizioni[1];
									SetDacValue(addrDacFrizione1+1);
								}
							}
							else{
								uiAux-=8;
								uiAux=1<<uiAux;
								if (Outputs_Hi&uiAux)
									Outputs_Hi&=~uiAux;
								else
									Outputs_Hi|=uiAux;
								uiAux=Outputs_Hi;
								// ricompongo la mia uscita outdigVariable...
								outDigVariable=(outDigVariable&0xFF)|(uiAux<<8);
							}
							break;

						case enumUD_Esc:
							// fine refresh lcd...
							sLCD.ucSuspendDigitalIoUpdate=0;
							ucLcdRefreshTimeoutEnable(0);
							vHandleEscFromMenu();
							// indico di rinfrescare la finestra
							return 2;

						case enumUD_Cut:
							/* Indico di eseguire un taglio asincrono. */
							vDoTaglio();
							// non occorre rinfrescare la finestra
							break;
						case enumUD_button_spindle:
							if (PrgRunning)
								break;
							ucLinkEncoder=enumSpeedRegulation_Spindle;
							vSelectHRE_potentiometer(ucLinkEncoder);
							break;
						case enumUD_button_f1:
							if (PrgRunning)
								break;
							ucLinkEncoder=enumSpeedRegulation_Wheel1;
							vSelectHRE_potentiometer(ucLinkEncoder);
							break;
						case enumUD_button_f2:
							if (PrgRunning)
								break;
							ucLinkEncoder=enumSpeedRegulation_Wheel2;
							vSelectHRE_potentiometer(ucLinkEncoder);
							break;

						case enumUD_Sx:
						case enumUD_Dx:
						case enumUD_Dn:
							vJumpToWindow(enumWinId_IngressiDigitali);
							return 2;
						case enumUD_Up:
							ucReturnFromWindow();
							return 2;


					}
				}
			}

			// Se non vi � programma in esecuzione ed il motore del mandrino � acceso, allora sto testando la
			// velocit� del mandrino. Inoltre effettuo l' aggiornamento solo se non vi � una rampa in esecuzione e se il valore
			// con cui devo aggiornare � diverso da quello gi� presente.
			if (!PrgRunning&&(outDigVariable&ODG_MOTORE)){
				if (macParms.uc_1Encoder_0Potenziometri==0){
					if (!spiralatrice.StatusRampaDac)
						WriteOnDacByPot();
				}
				else
					vHandleButtonsRegulation(enumUD_button_spindle);
			}



			//strcpy(hw.ucString,"Ingressi digitali");
			vStringLangCopy(hw.ucString,enumStr20_UsciteDigitali);
			ucPrintTitleButton(hw.ucString,defID_title_row,defID_title_col,enumFontSmall,enumUD_Title,defLCD_Color_Trasparente,1);

			for (i=0;i<8;i++){
				uiAux=Outputs_Hi;
				outDigVariable=OutDig|(uiAux<<8);
				// se motore acceso, non visualizzo ultima riga perch� ci sono i buttons della regolazione
				// spindle e frizioni...
				if ((outDigVariable&ODG_MOTORE)&&(i>=6))
					break;
				//strcpy(hw.ucString,ucStringhe_ODG[i]);
				vStringLangCopy(hw.ucString,enumStr20_ODG_0+i);
				ucPrintSmallText_ColNames(hw.ucString,defIdg_row+6+defIdg_row_offset*i,defIdg_col);
				if (outDigVariable&(1<<i))
					ucPrintBitmapButton(enumBitmap_40x24_ON,defIdg_row+defIdg_row_offset*i,defIdg_col+13*8,enumUD_button_1+i);
				else
					ucPrintBitmapButton(enumBitmap_40x24_OFF,defIdg_row+defIdg_row_offset*i,defIdg_col+13*8,enumUD_button_1+i);

				//strcpy(hw.ucString,ucStringhe_ODG[i+8]);
				vStringLangCopy(hw.ucString,enumStr20_ODG_0+i+8);
				ucPrintSmallText_ColNames(hw.ucString,defIdg_row+6+defIdg_row_offset*i,defIdg_col2);
				uiAux=i+8;
				if (Outputs_Hi&(1<<i))
					ucPrintBitmapButton(enumBitmap_40x24_ON,defIdg_row+defIdg_row_offset*i,defIdg_col2+13*8,enumUD_button_1+uiAux);
				else
					ucPrintBitmapButton(enumBitmap_40x24_OFF,defIdg_row+defIdg_row_offset*i,defIdg_col2+13*8,enumUD_button_1+uiAux);
			}

			vAddStandardButtons(enumUD_Sx);

			//ucPrintBitmapButton(enumBitmap_40x24_ESC,defLcdWidthY_pixel-24,defLcdWidthX_pixel-2-40-(15+40)*2,enumUD_Esc);



			return 1;
	}
}//unsigned char ucHW_Outdig(void)



// Ingressi Encoder...
unsigned char ucHW_IngressiEncoder(void){
	#define defIE_title_row 2
	#define defIE_title_col 0

	#define defIE_Offset_between_buttons (24+12)
	#define defIE_button_1_row 60
	#define defIE_button_1_col 0


	#define defIE_OK_row (defLcdWidthY_pixel-42)
	#define defIE_OK_col (defLcdWidthX_pixel-32*3-8)

	// pulsanti della finestra
	typedef enum {
			enumIE_encoderPosizione=0,
			enumIE_encoderVelocity,
			enumIE_Title,

			enumIE_Sx,
			enumIE_Up,
			enumIE_Cut,
			enumIE_Esc,
			enumIE_Dn,
			enumIE_Dx,

			enumIE_NumOfButtons
		}enumButtons_IE_utilities;
	xdata unsigned char i;
	static signed int iCounts_EncoderInputs;


	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enumIE_NumOfButtons;i++)
				ucCreateTheButton(i);
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			iCounts_EncoderInputs=iActCounts;
			// ogni 100ms rinfresco gli ingressi encoder
			ucLcdRefreshTimeoutEnable(200);
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enumIE_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enumIE_Esc:
							// fine refresh lcd...
							ucLcdRefreshTimeoutEnable(0);
							vHandleEscFromMenu();
							// indico di rinfrescare la finestra
							return 2;

						case enumIE_Sx:
						case enumIE_Dx:
							// fine refresh lcd...
							ucLcdRefreshTimeoutEnable(0);
							ucReturnFromWindow();
							return 2;
						case enumIE_encoderPosizione:
							iCounts_EncoderInputs=iActCounts;
							break;
						case enumIE_encoderVelocity:
							iAzzeraReadCounter_2();
							break;
						case enumIE_Cut:
							/* Indico di eseguire un taglio asincrono. */
							vDoTaglio();
							// non occorre rinfrescare la finestra
							break;


					}
				}
			}



			strcpy(hw.ucString,"Encoder inputs");
			//ucPrintStaticButton(hw.ucString,defIE_title_row,defIE_title_col,enumFontMedium,enumIE_Title,defLCD_Color_Green);
			ucPrintTitleButton(hw.ucString,defIE_title_row,defIE_title_col,enumFontMedium,enumIE_Title,defLCD_Color_Trasparente,1);

			sprintf(hw.ucString,"Meas. Wheel: %+5i",iActCounts-iCounts_EncoderInputs);
			ucPrintStaticButton(hw.ucString,defIE_button_1_row ,defIE_button_1_col ,enumFontMedium,enumIE_encoderPosizione,defLCD_Color_Yellow);
			sprintf(hw.ucString,"Speed ADJ  : %+5i",iActCounts2);
			ucPrintStaticButton(hw.ucString,defIE_button_1_row+defIE_Offset_between_buttons ,defIE_button_1_col ,enumFontMedium,enumIE_encoderVelocity,defLCD_Color_Yellow);

			vAddStandardButtons(enumIE_Sx);

			return 1;
	}
}//unsigned char ucHW_IngressiEncoder(void)













#define defMaxCharOhmResTaratura 7

// per inserimento ohm valore di resistenza di taratura
code TipoStructInfoFieldCodice im_ohm_resistenza_taratura[3]={
	{0,defMaxCharOhmResTaratura, "NNNNNN","%6.3f",0,100000.0,NULL,    1,1,"R taratura [ohm]",enumUm_ohm,"NNNNNNN","%7.3f"},
	{0,defMaxCharOhmResTaratura, "NNNNNN","%6.3f",0,100000.0,NULL,    1,1,"Offset taratura[ohm]",enumUm_ohm,"NNNNNNN","%7.3f"},
	{0,defMaxCharOhmResTaratura, "NNNNNN","%6.3f",0,10.0,NULL,        1,1,"K taratura [ohm]",enumUm_ohm,"NNNNNNN","%7.3f"},
};



// taratura portata n-esima
unsigned char ucHW_Taratura_Portata(void){
	#define defTP_title_row 6
	#define defTP_title_col 0

	#define defTP_abilita_taratura_R1_row 32
	#define defTP_abilita_taratura_R1_col 4

	#define defTP_valore_nominale_R1_row (defTP_abilita_taratura_R1_row+28)
	#define defTP_valore_nominale_R1_col 4

	#define defTP_abilita_taratura_R2_row (defTP_valore_nominale_R1_row+40)
	#define defTP_abilita_taratura_R2_col 4

	#define defTP_valore_nominale_R2_row (defTP_abilita_taratura_R2_row+28)
	#define defTP_valore_nominale_R2_col 4

	#define defTP_offset_row (defTP_valore_nominale_R2_row+36)
	#define defTP_offset_col 4
	#define defTP_K_row (defTP_offset_row+28)
	#define defTP_K_col 4

	#define defTP_OK_row (defLcdWidthY_pixel-42)
	#define defTP_OK_col (defLcdWidthX_pixel-32*3-8)

	#define defWinParam_TP_CanaleDiTaratura 0
	#define defWinParam_TP_PortataDiTaratura 1
	#define defWinParam_TP_NumCampoModificaNumK 2
	// pulsanti della finestra
	typedef enum {
			enumTP_abilita_taratura_R1=0,
			enumTP_valore_nominale_R1,
			enumTP_abilita_taratura_R2,
			enumTP_valore_nominale_R2,
			enumTP_valore_offset,
			enumTP_valore_K,
			enumTP_Title,

			enumTP_Sx,
			enumTP_Up,
			enumTP_Cut,
			enumTP_Esc,
			enumTP_Dn,
			enumTP_Dx,

			// enumTP_Esc,
			enumTP_NumOfButtons
		}enumButtons_taraturaPortate;
	xdata unsigned char i;
	code TipoStructInfoFieldCodice *pinfo;
	static xdata unsigned char ucRememberTaraturaInCorso;


	spiralatrice.actCanPortata=uiGetActiveWindowParam(0);
	actPortata=uiGetActiveWindowParam(1);
	if (spiralatrice.actCanPortata==N_CANALE_OHMETRO_DYN){
		spiralatrice.actTaraPortata=NUM_PORTATE_DINAMICHE-1-actPortata;
	}
	else
		spiralatrice.actTaraPortata=NUM_PORTATE-1-actPortata;
	pTar=&(macParms.tarParms[spiralatrice.actCanPortata][spiralatrice.actTaraPortata]);
	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			switch(defGetWinParam(defWinParam_TP_NumCampoModificaNumK)){
				case enumTP_valore_nominale_R1:
					pTar->p0=atof(hw.ucStringNumKeypad_out);
					ucSaveMacParms=1;
					break;
				case enumTP_valore_nominale_R2:
					pTar->p=atof(hw.ucStringNumKeypad_out);
					ucSaveMacParms=1;
					break;
				case enumTP_valore_offset:
					pTar->o=atof(hw.ucStringNumKeypad_out);
					ucSaveMacParms=1;
					break;
				case enumTP_valore_K:
					pTar->k=atof(hw.ucStringNumKeypad_out);
					if (spiralatrice.actCanPortata==N_CANALE_OHMETRO_DYN){
						pTar->ksh = pTar->k * 1024.0;
					}

					ucSaveMacParms=1;
					break;
			}
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enumTP_NumOfButtons;i++)
				ucCreateTheButton(i);
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			actPortata=uiGetActiveWindowParam(1);
			spiralatrice.actCanPortata=uiGetActiveWindowParam(0);
			ucRememberTaraturaInCorso=0;
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enumTP_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enumTP_Esc:
							// stop refresh
							ucLcdRefreshTimeoutEnable(0);
							vHandleEscFromMenu();
							// indico di rinfrescare la finestra
							return 2;

						case enumTP_Up:
							// stop refresh
							ucLcdRefreshTimeoutEnable(0);
							ucReturnFromWindow();
							return 2;
						case enumTP_abilita_taratura_R1:
							spiralatrice.TaraPortate=TaraOffset;
							actPortata=uiGetActiveWindowParam(1);
							if (spiralatrice.actCanPortata==N_CANALE_OHMETRO_DYN){
								spiralatrice.actTaraPortata=NUM_PORTATE_DINAMICHE-1-actPortata;
							}
							else
								spiralatrice.actTaraPortata=NUM_PORTATE-1-actPortata;
							spiralatrice.actCanPortata=uiGetActiveWindowParam(0);
							spiralatrice.ChangePortata=1;
							ucRememberTaraturaInCorso=1;
							break;
						case enumTP_abilita_taratura_R2:
							spiralatrice.TaraPortate=TaraGuadagno;
							actPortata=uiGetActiveWindowParam(1);
							if (spiralatrice.actCanPortata==N_CANALE_OHMETRO_DYN){
								spiralatrice.actTaraPortata=NUM_PORTATE_DINAMICHE-1-actPortata;
							}
							else
								spiralatrice.actTaraPortata=NUM_PORTATE-1-actPortata;
							spiralatrice.actCanPortata=uiGetActiveWindowParam(0);
							spiralatrice.ChangePortata=1;
							ucRememberTaraturaInCorso=1;
							break;
						case enumTP_valore_offset:
							pTar->o=0;
							pTar->osh=0;
							ucSaveMacParms=1;
							break;
						case enumTP_valore_K:
							pTar->k=1;
							pTar->ksh=1024;
							ucSaveMacParms=1;
							break;

						case enumTP_valore_nominale_R1:
						case enumTP_valore_nominale_R2:
							if (i==enumTP_valore_nominale_R1){
								pinfo=&im_ohm_resistenza_taratura[0];
								sprintf(hw.ucStringNumKeypad_in,pinfo->ucFmtField,pTar->p0);
							}
							else if (i==enumTP_valore_nominale_R2){
								pinfo=&im_ohm_resistenza_taratura[0];
								sprintf(hw.ucStringNumKeypad_in,pinfo->ucFmtField,pTar->p);
							}
							paramsNumK.ucMaxNumChar=pinfo->ucLenField;
							// imposto stringa di picture
							mystrcpy((char*)paramsNumK.ucPicture,(char*)pinfo->ucPictField,sizeof(paramsNumK.ucPicture)-1);
							paramsNumK.fMin=pinfo->fMin;
							paramsNumK.fMax=pinfo->fMax;
							paramsNumK.enumUm=pinfo->enumUm;

							mystrcpy((char*)paramsNumK.ucTitle,(char*)pinfo->ucTitleNumK,sizeof(paramsNumK.ucTitle)-1);
							// imposto abilitazione uso stringa picture
							paramsNumK.ucPictureEnable=pinfo->ucPictureEnable;	// abilitazione stringa picture
							// imposto abilitazione uso min/max
							paramsNumK.ucMinMaxEnable=pinfo->ucMinMaxEnable;	// abilitazione controllo valore minimo/massimo
							// indico qual � il campo che sto modificando
							defSetWinParam(defWinParam_TP_NumCampoModificaNumK,i);
							// chiamo il numeric keypad
							ucCallWindow(enumWinId_Num_Keypad);
							// indico di rinfrescare la finestra
							return 2;
					}
				}
			}

			switch (ucRememberTaraturaInCorso){
				case 0:
					break;
				case 1:
					// se taratura finita, avviso che � finita con finestra che rimane per 1.56secondi..
					if (spiralatrice.TaraPortate==TaraNulla){
						ucRememberTaraturaInCorso=2;
						// fra 1.5 secondi azzero scritta che riporta il valore letto...
						ucLcdRefreshTimeoutEnable(1500);
						vStringLangCopy(hw.ucString,enumStr20_MeasureOk);
						ucPrintStaticButton(hw.ucString,defTP_abilita_taratura_R2_row,0,enumFontBig,0,defLCD_Color_Red);
					}
					break;
				default:
					ucRememberTaraturaInCorso=0;
					// stop refresh
					ucLcdRefreshTimeoutEnable(0);
					break;

			}


			if (uiGetActiveWindowParam(0)==N_CANALE_OHMETRO_STA){
				sprintf(hw.ucString,"  %s %1i ",pucStringLang(enumStr20_MisuraStatica),uiGetActiveWindowParam(1)+1);
			}
			else{
				if (macParms.ucAbilitaQuintaPortata){
					sprintf(hw.ucString,"  %s %1i ",pucStringLang(enumStr20_MisuraDinamica),uiGetActiveWindowParam(1)+1);
				}
				else{
					sprintf(hw.ucString,"  %s %1i ",pucStringLang(enumStr20_MisuraDinamica),uiGetActiveWindowParam(1));
				}
			}
			//ucPrintStaticButton(hw.ucString,defTP_title_row,defTP_title_col,enumFontMedium,enumTP_Title,defLCD_Color_Green);
			ucPrintTitleButton(hw.ucString,defTP_title_row,defTP_title_col,enumFontMedium,enumTP_Title,defLCD_Color_Trasparente,1);

			//strcpy(hw.ucString,"ABILITA TARATURA R1 ");
			vStringLangCopy(hw.ucString,enumStr20_AbilitaTaraturaR1);
			ucPrintStaticButton(hw.ucString,defTP_abilita_taratura_R1_row,defTP_abilita_taratura_R1_col,enumFontMedium,enumTP_abilita_taratura_R1,(spiralatrice.TaraPortate==TaraOffset)?defLCD_Color_Yellow:defLCD_Color_Trasparente);

			sprintf(hw.ucString,"R1: %6.3f",pTar->p0);
			ucPrintStaticButton(hw.ucString,defTP_valore_nominale_R1_row,defTP_valore_nominale_R1_col,enumFontMedium,enumTP_valore_nominale_R1,defLCD_Color_Trasparente);

			//strcpy(hw.ucString,"ABILITA TARATURA R2 ");
			vStringLangCopy(hw.ucString,enumStr20_AbilitaTaraturaR2);
			ucPrintStaticButton(hw.ucString,defTP_abilita_taratura_R2_row,defTP_abilita_taratura_R2_col,enumFontMedium,enumTP_abilita_taratura_R2,defLCD_Color_Trasparente);

			sprintf(hw.ucString,"R2: %6.3f",pTar->p);
			ucPrintStaticButton(hw.ucString,defTP_valore_nominale_R2_row,defTP_valore_nominale_R2_col,enumFontMedium,enumTP_valore_nominale_R2,(spiralatrice.TaraPortate==TaraGuadagno)?defLCD_Color_Yellow:defLCD_Color_Trasparente);

			sprintf(hw.ucString,"offset: %+7.3f",pTar->o);
			ucPrintStaticButton(hw.ucString,defTP_offset_row,defTP_offset_col,enumFontMedium,enumTP_valore_offset,defLCD_Color_Trasparente);
			sprintf(hw.ucString,"K     : %+7.3f",pTar->k);
			ucPrintStaticButton(hw.ucString,defTP_K_row,defTP_K_col,enumFontMedium,enumTP_valore_K,defLCD_Color_Trasparente);


			vAddStandardButtons(enumTP_Sx);

			return 1;
	}
}//unsigned char ucHW_Taratura_Portata(void)





// se ritorna 2--> richiama ancora una volta la gestione finestre, c'� stato un cambio di finestra
unsigned char ucHW_Taratura_Portata_1_4(void){
	typedef enum {

			enumTARAS14_Portata0,
			enumTARAS14_Portata1,
			enumTARAS14_Portata2,
			enumTARAS14_Portata3,
			enumTARAS14_Portata4,
			enumTARAS14_Title,

			enumTARAS14_Sx,
			enumTARAS14_Up,
			enumTARAS14_Cut,
			enumTARAS14_Esc,
			enumTARAS14_Dn,
			enumTARAS14_Dx,
			enumTARAS14_NumOf
	}enumTARAS14_buttons;



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
			for (i=0;i<enumTARAS14_NumOf;i++)
				ucCreateTheButton(i);

			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Active:
			// quale portata � stata premuta???
			for (i=0;i<4;i++){
				if (ucHasBeenPressedButton(i)){
					ucSetWindowParam(enumWinId_Taratura_Portata,0, uiGetActiveWindowParam(0));
					if (uiGetActiveWindowParam(0)==N_CANALE_OHMETRO_DYN){
						// se non � abilitata la quinta portata, parto dalla prima con le tarature...
						if (!macParms.ucAbilitaQuintaPortata){
							ucSetWindowParam(enumWinId_Taratura_Portata,1, i+1);
						}
						else{
							ucSetWindowParam(enumWinId_Taratura_Portata,1, i);
						}
					}
					else
						ucSetWindowParam(enumWinId_Taratura_Portata,1, i);
					ucCallWindow(enumWinId_Taratura_Portata);
					return 2;
				}
			}
			if (ucHasBeenPressedButton(enumTARAS14_Portata4)){
				ucSetWindowParam(enumWinId_Taratura_Portata,0, uiGetActiveWindowParam(0));
				ucSetWindowParam(enumWinId_Taratura_Portata,1, enumTARAS14_Portata4);
				ucCallWindow(enumWinId_Taratura_Portata);
				return 2;
			}
			if (ucHasBeenPressedButton(enumTARAS14_Esc)){
				vHandleEscFromMenu();
				// indico di rinfrescare la finestra
				return 2;
			}
			if (ucHasBeenPressedButton(enumTARAS14_Up)){
				ucReturnFromWindow();
				return 2;
			}
			//strcpy(hw.ucString,"  Seleziona portata ");
			vStringLangCopy(hw.ucString,enumStr20_SelezionaPortata);
			ucPrintTitleButton(hw.ucString,6,0,enumFontMedium,enumTARAS14_Title,defLCD_Color_Trasparente,1);

			for (i=0;i<4;i++){
				if (uiGetActiveWindowParam(0)==N_CANALE_OHMETRO_STA)
					sprintf(hw.ucString,"%s %i",pucStringLang(enumStr20_PortataStat),i+1);
				else
					sprintf(hw.ucString,"%s %i",pucStringLang(enumStr20_PortataDyn),i+1);
				ucPrintStaticButton(hw.ucString,64+i*36,4,100,enumTARAS14_Portata0+i,defLCD_Color_Trasparente);
			}
			if (macParms.ucAbilitaQuintaPortata){
				if (uiGetActiveWindowParam(0)==N_CANALE_OHMETRO_DYN){
					sprintf(hw.ucString,"%s %i",pucStringLang(enumStr20_PortataDyn),5);
					ucPrintStaticButton(hw.ucString,64+4*36,4,100,enumTARAS14_Portata4,defLCD_Color_Trasparente);
				}
			}
			vAddStandardButtons(enumTARAS14_Sx);

			return 1;
	}
}

// se ritorna 2--> richiama ancora una volta la gestione finestre, c'� stato un cambio di finestra
unsigned char ucHW_Taratura_Misura_Dyn_Or_Stat(void){
	#define defTS_NumParametro 0

	typedef enum {

			enumTARAS_Title,
			enumTARAS_Dyn,
			enumTARAS_Stat,

			enumTARAS_Sx,
			enumTARAS_Up,
			enumTARAS_Cut,
			enumTARAS_Esc,
			enumTARAS_Dn,
			enumTARAS_Dx,
			enumTARAS_NumOf
	}enumTARAS_buttons;

	xdata unsigned int i;

	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			switch(defGetWinParam(defTS_NumParametro)){
				// visualizzata finestra password non corretta
				case 201:
					sLCD.ucPasswordChecked=0;
					// chiamo il main menu...
					vJumpToWindow(enumWinId_MainMenu);
					return 2;
				// check password???
				case 200:
					if (ucVerifyPasswordCorrect(hw.ucStringNumKeypad_out)){
						sLCD.ucPasswordChecked=1;
						vSetStatusCurrentWindow(enumWindowStatus_Active);
						return 2;
					}
					// visualizzo la finestra: password non corretta
					// numero msg da visualizzare: 1
					ucSetWindowParam(enumWinId_Info,0, 1);
					// messaggio da visualizzare: password bad
					ucSetWindowParam(enumWinId_Info,1, enumStr20_bad_password);
					// messaggio: password KO
					defSetWinParam(defTS_NumParametro,201);
					// chiamo la finestra info
					ucCallWindow(enumWinId_Info);
					return 2;
				default:
					vSetStatusCurrentWindow(enumWindowStatus_Active);
					break;
			}
			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enumTARAS_NumOf;i++)
				ucCreateTheButton(i);
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			ucEnableStartLavorazione(0);

			return 2;
		case enumWindowStatus_Active:
			// se ancora non � stata verificata la password, passo a chiederla...
			if (!sLCD.ucPasswordChecked){
				vSetNumK_to_ask_password();
				defSetWinParam(defTS_NumParametro,200);
				// chiamo il numeric keypad
				ucCallWindow(enumWinId_Num_Keypad);
				// indico di rinfrescare la finestra
				return 2;
			}
			for (i=0;i<enumTARAS_NumOf;i++){
				if (!ucHasBeenPressedButton(i)){
					continue;
				}
				switch(i){
					case enumTARAS_Esc:
						ucEnableStartLavorazione(1);
						vHandleEscFromMenu();
						// indico di rinfrescare la finestra
						return 2;
					case enumTARAS_Stat:
						ucSetWindowParam(enumWinId_Taratura_Portata_1_4,0,N_CANALE_OHMETRO_STA); // 0--> statica
						ucCallWindow(enumWinId_Taratura_Portata_1_4);
						return 2;
					case enumTARAS_Dyn:
						ucSetWindowParam(enumWinId_Taratura_Portata_1_4,0,N_CANALE_OHMETRO_DYN); // 1--> dinamica
						ucCallWindow(enumWinId_Taratura_Portata_1_4);
						return 2;
				}
			}

			//strcpy(hw.ucString," Taratura strumenti ");
			vStringLangCopy(hw.ucString,enumStr20_TaraturaStrumenti);
			ucPrintTitleButton(hw.ucString,6,0,enumFontMedium,enumTARAS_Title,defLCD_Color_Trasparente,1);
			//strcpy(hw.ucString,"Misura statica");
			sprintf(hw.ucString,"%-20.20s",pucStringLang(enumStr20_MisuraStatica));
			ucPrintStaticButton(hw.ucString,80-8,4,100,enumTARAS_Stat,defLCD_Color_Trasparente);
			//strcpy(hw.ucString,"Misura dinamica");
			sprintf(hw.ucString,"%-20.20s",pucStringLang(enumStr20_MisuraDinamica));
			ucPrintStaticButton(hw.ucString, 80+48+8,4,100,enumTARAS_Dyn,defLCD_Color_Trasparente);

			vAddStandardButtons(enumTARAS_Sx);

			return 1;
	}
}


unsigned char ucHW_SelezionaTaratura(void){
	#define defSELT_NumParametro 0

	typedef enum {

			enumSELT_Title,
			enumSELT_Strumenti,
			enumSELT_Lcd,
			enumSELT_CalibraPotenziometro,

			enumSELT_Sx,
			enumSELT_Up,
			enumSELT_Cut,
			enumSELT_Esc,
			enumSELT_Dn,
			enumSELT_Dx,
			enumSELT_NumOf
	}enumSELT_buttons;

	xdata unsigned int i,idxParametro;

	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			idxParametro=defGetWinParam(defSELT_NumParametro);
#if 1
			// visualizzata finestra password non corretta
			if (idxParametro==201){
				sLCD.ucPasswordChecked=0;
				defSetWinParam(defSELT_NumParametro,0);
				// chiamo il main menu...
				vJumpToWindow(enumWinId_MainMenu);
			}
			// check password???
			else if (idxParametro==200){
				if (ucVerifyPasswordCorrect(hw.ucStringNumKeypad_out)){
					defSetWinParam(defSELT_NumParametro,0);
					sLCD.ucPasswordChecked=1;
					vSetStatusCurrentWindow(enumWindowStatus_Active);
					return 2;
				}
				// visualizzo la finestra: password non corretta
				// numero msg da visualizzare: 1
				ucSetWindowParam(enumWinId_Info,0, 1);
				// messaggio da visualizzare: password bad
				ucSetWindowParam(enumWinId_Info,1, enumStr20_bad_password);
				// messaggio: password KO
				defSetWinParam(defSELT_NumParametro,201);
			    // chiamo la finestra info
		        ucCallWindow(enumWinId_Info);
				return 2;
			}
#endif
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enumSELT_NumOf;i++)
				ucCreateTheButton(i);
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			ucEnableStartLavorazione(0);
			defSetWinParam(defSELT_NumParametro,0);
			return 2;
		case enumWindowStatus_Active:
			// se ancora non � stata verificata la password, passo a chiederla...
			if (!sLCD.ucPasswordChecked){
				vSetNumK_to_ask_password();
				defSetWinParam(defSELT_NumParametro,200);
				// chiamo il numeric keypad
				ucCallWindow(enumWinId_Num_Keypad);
				// indico di rinfrescare la finestra
				return 2;
			}
			for (i=0;i<enumSELT_NumOf;i++){
				if (!ucHasBeenPressedButton(i)){
					continue;
				}
				switch(i){
					case enumSELT_Esc:
						ucEnableStartLavorazione(1);
						vHandleEscFromMenu();
						// indico di rinfrescare la finestra
						return 2;
					case enumSELT_Strumenti:
						if (PrgRunning)
							break;
						defSetWinParam(defSELT_NumParametro,0);
						ucCallWindow(enumWinId_Taratura_Misura_Dyn_Or_Stat);
						return 2;
					case enumSELT_Lcd:
						if (PrgRunning)
							break;
						defSetWinParam(defSELT_NumParametro,0);
						vEnableCalibraLcd(1);
						return 0;
					case enumSELT_CalibraPotenziometro:
						if (PrgRunning)
							break;
						defSetWinParam(defSELT_NumParametro,0);
						ucCallWindow(enumWinId_CalibraSensore);
						return 2;

				}
			}

			//strcpy(hw.ucString," Tarature");
			vStringLangCopy(hw.ucString,enumStr20_Tarature);
			ucPrintTitleButton(hw.ucString,6,0,enumFontMedium,enumSELT_Title,defLCD_Color_Trasparente,1);

			sprintf(hw.ucString,"%-20.20s",pucStringLang(enumStr20_TaraturaStrumenti));
			ucPrintStaticButton(hw.ucString,60,0,enumFontMedium,enumSELT_Strumenti,defLCD_Color_Trasparente);

			sprintf(hw.ucString,"%-20.20s",pucStringLang(enumStr20_CalibraLcd));
			ucPrintStaticButton(hw.ucString,60+40,0,enumFontMedium,enumSELT_Lcd,defLCD_Color_Trasparente);

			sprintf(hw.ucString,"%-20.20s",pucStringLang(enumStr20_CalibraSensore));
			ucPrintStaticButton(hw.ucString,60+2*40,0,enumFontMedium,enumSELT_CalibraPotenziometro,defLCD_Color_Trasparente);


			vAddStandardButtons(enumSELT_Sx);

			return 1;
	}
}

// finestra che fa vedere a video max tre righe di messaggi, poi in fondo il tasto OK
// il primo par della finestra dice il numero N di msg da visualizzare
// i successivi N parametri contengono il numero di stringa da visualizzare
unsigned char ucHW_Info(void){
	typedef enum {
			enumInfo_message1=0,
			enumInfo_message2,
			enumInfo_message3,
			enumInfo_message4,
			enumInfo_OK,
			enumInfo_NumOfButtons
		}enumButtons_Info;
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
			for (i=0;i<enumInfo_NumOfButtons;i++)
				ucCreateTheButton(i);
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Active:
			// premuto ok?
			if (ucHasBeenPressedButton(enumInfo_OK)){
				// return
				ucReturnFromWindow();
				// indico di rinfrescare la finestra
				return 2;
			}
			for (i=0;i<defGetWinParam(0);i++){
				if (i>=3)
					break;
				vStringLangCopy(hw.ucString,defGetWinParam(i+1));
				if (strlen(hw.ucString)>20){
					ucPrintTitleButton(hw.ucString,36+32U*(i),0,enumFontSmall,enumInfo_message1+i,defLCD_Color_Trasparente,1);
				}
				else{
					ucPrintTitleButton(hw.ucString,36+32U*(i),0,enumFontMedium,enumInfo_message1+i,defLCD_Color_Trasparente,1);
				}
			}
			vStringLangCopy(hw.ucString,enumStr20_Ok);
			ucPrintStaticButton(hw.ucString,defLcdWidthY_pixel-32-12,defLcdWidthX_pixel/2-32-8,enumFontBig,enumInfo_OK,defLCD_Color_Trasparente);
			return 1;
	}
}

float f_temperature_coefficient;
unsigned char uc_calling_from_view_codice_prodotto;
#if 1
code TipoStructInfoParametro_Lingua if_VCP[]={
	{0,5, "NNNNN","%5.3f",0.8,2,(unsigned char xdata*)&hprg.theAct.coeff_corr,           1,1,enumStr20_FattoreCorrezione,enumPT_float, enumUm_none,enumStringType_none,1},
	{0,5, "NNNNN","%5.3f",0.0,99999.0,(unsigned char xdata*)&f_temperature_coefficient,1,1,enumStr20_Temperature_coefficient,enumPT_float, enumUm_none,enumStringType_none,1},
//	{0,5, "NNNNN","%5.2f",MIN_VEL_PROD,MAX_VEL_PROD,(unsigned char xdata*)&hprg.theAct.vel_produzione,1,1,enumStr20_VelProduzione,enumPT_float, enumUm_metri_minuto,enumStringType_none,1},
//	{0,5, "nnnn","%4i",MIN_RPM_MANDRINO,MAX_RPM_MANDRINO,(unsigned char xdata*)&hprg.theAct.rpm_mandrino,1,1,enumStr20_Um_rpm,enumPT_unsignedInt, enumUm_rpm,enumStringType_none,1},
};

// prepares for inputting parameter of a product code
void v_preset_for_program_param_input(enum_vcp_preset_param preset_param){
	unsigned int idxParametro;
	idxParametro=preset_param;
//						defSetWinParam(defWinParam_VCP_NumCampo,i);
	paramsNumK.enumUm=if_VCP[idxParametro].enumUm;
	// imposto limiti min e max
	paramsNumK.fMin=if_VCP[idxParametro].fMin;
	paramsNumK.fMax=if_VCP[idxParametro].fMax;
	if ((nvram_struct.actUM==UM_IN)&&ucIsUmSensitive(if_VCP[idxParametro].enumUm)){
		paramsNumK.fMin*=if_VCP[idxParametro].fConvUm_mm_toUm_inch;
		paramsNumK.fMax*=if_VCP[idxParametro].fConvUm_mm_toUm_inch;
		paramsNumK.enumUm++;
	}
	paramsNumK.ucMaxNumChar=if_VCP[idxParametro].ucLenField;
	// imposto stringa di picture
	mystrcpy((char*)paramsNumK.ucPicture,(char*)if_VCP[idxParametro].ucPictField,sizeof(paramsNumK.ucPicture)-1);
	// titolo che deve comparire...
	//strcpy(paramsNumK.ucTitle,if_APP[idxParametro].ucTitleNumK);
	vStringLangCopy(paramsNumK.ucTitle,if_VCP[idxParametro].stringa);
	// copio la stringa con cui si inizializza il numeric keypad
	// sprintf(hw.ucStringNumKeypad_in,if_APP[idxParametro].ucFmtField,if_APP[idxParametro].pc);
	vGetValueParametro(hw.ucStringNumKeypad_in,idxParametro,&if_VCP[0]);

	// imposto abilitazione uso stringa picture
	paramsNumK.ucPictureEnable=if_VCP[idxParametro].ucPictureEnable;	// abilitazione stringa picture
	// imposto abilitazione uso min/max
	paramsNumK.ucMinMaxEnable=if_VCP[idxParametro].ucMinMaxEnable;	// abilitazione controllo valore minimo/massimo
	// chiamo il numeric keypad
	ucCallWindow(enumWinId_Num_Keypad);
}

// finestra che fa vedere a video i pars del codice prodotto corrente
unsigned char ucHW_ViewCodiceProdotto(void){
	#define defWinParam_VCP_NumCampo 0

	typedef enum {
			enumVCP_title=0,
			enumVCP_codice,
			enumVCP_coeff_corr,
			enumVCP_temperature_coefficient,
			
			enumVCP_vel_produzione,
			enumVCP_vel_produzione_rpm,
			enumVCP_quota_penetrazione,
			enumVCP_coeff_corr_desc,
			enumVCP_vel_produzione_desc,
			enumVCP_vel_produzione_rpm_desc,
			enumVCP_quota_penetrazione_desc,
			enumVCP_temperature_coefficient_desc,

			enumVCP_Sx,
			enumVCP_Up,
			enumVCP_Cut,
			enumVCP_Esc,
			enumVCP_Dn,
			enumVCP_Dx,

			enumVCP_NumOfButtons
		}enumButtons_VCP;

	#define defVCP_Title_row 6
	#define defVCP_Title_col 0

	#define defVCP_codice_row 38
	#define defVCP_codice_col 0

	#define defVCP_offset_between_row 36

	#define defNumCharVCP_Descrizione 18

	#define defVCP_coeff_desc_row (defVCP_codice_row+32)
	#define defVCP_coeff_desc_col 0
	#define defVCP_coeff_row defVCP_coeff_desc_row
	#define defVCP_coeff_col (defVCP_coeff_desc_col+defNumCharVCP_Descrizione*8+12)

	xdata unsigned char i,idxParametro;
	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			switch(defGetWinParam(defWinParam_VCP_NumCampo)){
				case enumVCP_coeff_corr:
					hprg.theAct.coeff_corr=atof(hw.ucStringNumKeypad_out);
					// salvo il programma
					ucSaveActPrg();
					if (PrgRunning){
						if (withMis){
							actCorrOhmica=fCalcCoeffCorr()*1024.0;
							//anticipoPretaglio/=fCalcCoeffCorr();
							// V1.29--> aniticipo di pretaglio lo ricalcolo da ZERO...
							vCalcAnticipoPretaglio();
						}
						// Se sono in modalit� L, ...
						else {
							// Calcolo il numero di impulsi che devo ricevere per poter dire di aver completato il pezzo.
							targetLenImp = (long)((nvram_struct.commesse[spiralatrice.runningCommessa].res/fCalcCoeffCorr())/nvram_struct.commesse[spiralatrice.runningCommessa].resspec_spir *1000./ impToMm  + ROUND);
							if (anticipoPretaglio>targetLenImp)
								anticipoPretaglio=(unsigned short)(9*targetLenImp/10);
							preLenImp = targetLenImp - anticipoPretaglio; /* per far partire il comando di pretaglio */
						}
					}
					break;
				case enumVCP_temperature_coefficient:
					break;
				case enumVCP_vel_produzione:
					// always refresh active product code...
					// vSetActiveNumberProductCode(&his);
				
					hprg.theAct.vel_produzione=atof(hw.ucStringNumKeypad_out);
					/* Aggiusto al velocit� del mandrino in rpm e delle frizioni. */
					AdjustVelPeriferica(1);
					// salvo il programma
					ucSaveActPrg();
					break;
				case enumVCP_vel_produzione_rpm:
					hprg.theAct.rpm_mandrino=atof(hw.ucStringNumKeypad_out);
					/* Aggiusto al velocit� del mandrino in rpm e delle frizioni. */
					AdjustVelPeriferica(0);
					// salvo il programma
					ucSaveActPrg();
					break;

			}
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enumVCP_NumOfButtons;i++)
				ucCreateTheButton(i);
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			uc_calling_from_view_codice_prodotto=1;
			// punto al programma attuale
			//hprg.ptheAct=&(hprg.theAct);
			return 2;
		case enumWindowStatus_Active:
			f_temperature_coefficient=(hprg.ptheAct->ui_temperature_coefficient_per_million*1e-6)*80.0+1.0;
			for (i=0;i<enumVCP_NumOfButtons;i++){
				if (!ucHasBeenPressedButton(i))
					continue;
				switch(i){
					case enumVCP_Esc:
						uc_calling_from_view_codice_prodotto=0;
						vHandleEscFromMenu();
						// indico di rinfrescare la finestra
						return 2;

					case enumVCP_Cut:
						/* Indico di eseguire un taglio asincrono. */
						vDoTaglio();
						// non occorre rinfrescare la finestra
						break;
					case enumVCP_Dn:
					case enumVCP_Up:
                        if (macParms.ucAbilitaCompensazioneTemperatura)
                        {
                            defSetWinParam(defWinParam_VCP_NumCampo,enumVCP_temperature_coefficient);
                            ucCallWindow(enumWinId_Temperature_Compensation_Params);
                            return 2;
                        }
                        break;
					case enumVCP_Dx:
					case enumVCP_Sx:
						uc_calling_from_view_codice_prodotto=0;
						// return
						ucReturnFromWindow();
						// indico di rinfrescare la finestra
						return 2;
					// non posso aggiustare la vel produzione da tastiera se programma running...
					case enumVCP_vel_produzione_rpm:
					case enumVCP_vel_produzione:
						// 1.28 non posso cambiare la vel di produzione o gli rpm...
						//if (PrgRunning)
						break;
					case enumVCP_temperature_coefficient:
						defSetWinParam(defWinParam_VCP_NumCampo,i);
						ucCallWindow(enumWinId_Temperature_Compensation_Params);
						break;
					case enumVCP_coeff_corr:
						idxParametro=i-enumVCP_coeff_corr;
						defSetWinParam(defWinParam_VCP_NumCampo,i);
						v_preset_for_program_param_input((enum_vcp_preset_param)idxParametro);
						// indico di rinfrescare la finestra
						return 2;


				}
			}
			vStringLangCopy(hw.ucString,enumStr20_ViewCodiceProdotto);
			ucPrintTitleButton(hw.ucString,defVCP_Title_row,defVCP_Title_col,enumFontMedium,enumVCP_title,defLCD_Color_Trasparente,1);

			vString40LangCopy(hw.ucString,enumStr40_codefields);
			ucPrintSmallText_ColNames(hw.ucString,defVCP_codice_row-12,defVCP_codice_col);
			// versione 1.49: problema della visualizzazione codici prodotto da archivio codici prodotto...
			// quando si va in "visualizza codici prodotto", e poi si seleziona un codice
			// e si preme "visualizza", visualizza il codice in produzione...		
			sprintf(hw.ucString,"%19.19s ",hprg.theAct.codicePrg);
			ucPrintTitleButton(hw.ucString,defVCP_codice_row,defVCP_codice_col,enumFontMedium,enumVCP_codice,defLCD_Color_Trasparente,1);
			sprintf(hw.ucString,"%-18.18s",pucStringLang(enumStr20_FattoreCorrezione));
			ucPrintTitleButton(hw.ucString,defVCP_coeff_desc_row,defVCP_coeff_desc_col,enumFontSmall,enumVCP_coeff_corr_desc,defLCD_Color_Trasparente,2);
			sprintf(hw.ucString," %5.3f",hprg.theAct.coeff_corr);
			ucPrintStaticButton(hw.ucString,defVCP_coeff_row,defVCP_coeff_col,enumFontMedium,enumVCP_coeff_corr,defLCD_Color_Trasparente);
			sprintf(hw.ucString,"%-19.19s"," ");
			ucPrintSmallText_ColNames(hw.ucString,defVCP_coeff_desc_row+12,defVCP_coeff_desc_col);

			sprintf(hw.ucString,"%-18.18s",pucStringLang(enumStr20_VelProduzione));
			ucPrintTitleButton(hw.ucString,defVCP_coeff_desc_row+defVCP_offset_between_row,defVCP_coeff_desc_col,enumFontSmall,enumVCP_vel_produzione_desc,defLCD_Color_Trasparente,2);
			sprintf(hw.ucString," %5.3f",hprg.theAct.vel_produzione);
			ucPrintStaticButton(hw.ucString,defVCP_coeff_row+defVCP_offset_between_row,defVCP_coeff_col,enumFontMedium,enumVCP_vel_produzione,defLCD_Color_Trasparente);
			if (nvram_struct.actUM==UM_IN)
				sprintf(hw.ucString,"%18.18s ",pucStringLang(enumStr20_Um_feet_minuto));
			else
				sprintf(hw.ucString,"%18.18s ",pucStringLang(enumStr20_Um_metri_minuto));
			ucPrintSmallText_ColNames(hw.ucString,defVCP_coeff_desc_row+defVCP_offset_between_row+12,defVCP_coeff_desc_col);

			sprintf(hw.ucString,"%-18.18s",pucStringLang(enumStr20_Um_rpm));
			ucPrintTitleButton(hw.ucString,defVCP_coeff_desc_row+2*defVCP_offset_between_row,defVCP_coeff_desc_col,enumFontSmall,enumVCP_vel_produzione_rpm_desc,defLCD_Color_Trasparente,2);
			sprintf(hw.ucString," %5i",hprg.theAct.rpm_mandrino);
			ucPrintStaticButton(hw.ucString,defVCP_coeff_row+2*defVCP_offset_between_row,defVCP_coeff_col,enumFontMedium,enumVCP_vel_produzione_rpm,defLCD_Color_Trasparente);
			sprintf(hw.ucString,"%18.18s ",pucStringLang(enumStr20_Um_rpm));
			ucPrintSmallText_ColNames(hw.ucString,defVCP_coeff_desc_row+2*defVCP_offset_between_row+12,defVCP_coeff_desc_col);


			sprintf(hw.ucString,"%-18.18s",pucStringLang(enumStr20_QuotaPenetrazioneTaglio));
			ucPrintTitleButton(hw.ucString,defVCP_coeff_desc_row+3*defVCP_offset_between_row,defVCP_coeff_desc_col,enumFontSmall,enumVCP_quota_penetrazione_desc,defLCD_Color_Trasparente,2);
			sprintf(hw.ucString,"%+5.3f",hprg.theAct.pos_pretaglio);
			ucPrintStaticButton(hw.ucString,defVCP_coeff_row+3*defVCP_offset_between_row,defVCP_coeff_col,enumFontMedium,enumVCP_quota_penetrazione,defLCD_Color_Trasparente);
			if (nvram_struct.actUM==UM_IN)
				sprintf(hw.ucString,"%18.18s ",pucStringLang(enumStr20_Um_inch));
			else
				sprintf(hw.ucString,"%18.18s ",pucStringLang(enumStr20_Um_mm));
			ucPrintSmallText_ColNames(hw.ucString,defVCP_coeff_desc_row+3*defVCP_offset_between_row+12,defVCP_coeff_desc_col);
			// routine che permette di aggiungere ad una finestra i bottoni standard sx, dx, up, dn, esc, cut
			vAddStandardButtons(enumVCP_Sx);


			return 1;
	}
}

#endif






extern xdata TipoStructHandleMeasure hM;

extern unsigned char ucNeedToClosePopupWindow(void);
// ritorna 1 se le finestre sottostanti NON devono essere rigenerate...
extern unsigned char ucHandlePopupWindow(void);


// usare questa define per avere una finestra che rappresenta le posizioni
// xy calibrate e normali del touch
//#define defVerifyLcdCalibration
xdata unsigned int uiCursorePosXnoLin, uiCursorePosYnoLin;
xdata unsigned int uiCursorePosX, uiCursorePosY;

unsigned char ucHandleWindows(void){

	xdata unsigned char ucCode,i;
	ucCode=0;
#ifdef defVerifyLcdCalibration
	if (uiCursorePosXnoLin||uiCursorePosYnoLin){
		sprintf(hw.ucString,"x:%03i xc:%03i",uiCursorePosXnoLin,uiCursorePosX);
		ucPrintStaticButton(hw.ucString,12,12,enumFontSmall,defMaxButtonsOnLcd-1,defLCD_Color_Yellow);
		sprintf(hw.ucString,"y:%03i yc:%03i",uiCursorePosYnoLin,uiCursorePosY);
		ucPrintStaticButton(hw.ucString,24,12,enumFontSmall,defMaxButtonsOnLcd-2,defLCD_Color_Yellow);
	}
#endif
	// salvo variabile che indica se siamo nella pagina di produzione
	if (uiGetIdCurrentWindow()==enumWinId_setupProduzione){
		// se programma running, e ci sono ancora pezzi da fare
		// indico che lavorazione in corso
		if (PrgRunning&&(nvram_struct.commesse[spiralatrice.runningCommessa].ndo<nvram_struct.commesse[spiralatrice.runningCommessa].n+nvram_struct.commesse[spiralatrice.runningCommessa].magg)){
			nvram_struct.ucIwasWorkingOnMenuProduzione_0xa5=0xa5;
		}
		// se commessa esaurita, azzero flag che indica che era in corso una lavorazione
		else if (nvram_struct.commesse[spiralatrice.runningCommessa].ndo>=nvram_struct.commesse[spiralatrice.runningCommessa].n+nvram_struct.commesse[spiralatrice.runningCommessa].magg)
		{
			nvram_struct.ucIwasWorkingOnMenuProduzione_0xa5=0;
		}
	}
	else
		nvram_struct.ucIwasWorkingOnMenuProduzione_0xa5=0;
	// se c'� una finestra di popup (es allarme)
	// che non vuole che le finestre sottostanti siano rigenerate, non lo faccio...
	if (!ucHandlePopupWindow()){
		// ripeto il loop max 5 volte, per dare modo alle finestre di essere rigenerate...
		for (i=0;i<5;i++){
			switch(uiGetIdCurrentWindow()){
				case enumWinId_listaCodici:
					ucCode=ucHW_listaCodici();
					break;
				case enumWinId_CodiceProdotto:
					ucCode=ucHW_codiceProdotto();
					break;
				case enumWinId_ListaLavori:
					ucCode=ucHW_listaLavori();
					break;
				case enumWinId_ModificaLavoro:
					ucCode=ucHW_modificaLavoro();
					break;
				case enumWinId_inserisciOhmMfili:
					ucCode=ucHW_inserisciOhmMfili();
					break;
				case enumWinId_setupProduzione:
					ucCode=ucHW_setupProduzione();
					break;
				case enumWinId_Taratura_Misura_Dyn_Or_Stat:
					ucCode=ucHW_Taratura_Misura_Dyn_Or_Stat();
					break;
				case enumWinId_Taratura_Portata_1_4:
					ucCode=ucHW_Taratura_Portata_1_4();
					break;
				case enumWinId_Taratura_Portata:
					ucCode=ucHW_Taratura_Portata();
					break;
				case enumWinId_Num_Keypad:
					ucCode=ucHW_Num_Keypad();
					break;
				case enumWinId_YesNo:
					ucCode=ucHW_YesNo();
					break;
				case enumWinId_MainMenu:
					ucCode=ucHW_MainMenu();
					break;
				case enumWinId_ParametriMacchina:
					ucCode=ucHW_ParametriMacchina();
					break;
				case enumWinId_Utilities:
					ucCode=ucHW_Utilities();
					break;
				case enumWinId_IngressiDigitali:
					ucCode=ucHW_Indig();
					break;
				case enumWinId_IngressiAnalogici:
					ucCode=ucHW_InAnalog();
					break;
				case enumWinId_UsciteAnalogiche:
					ucCode=ucHW_OutAnalog();
					break;
				case enumWinId_UsciteDigitali:
					ucCode=ucHW_Outdig();
					break;
				case enumWinId_IngressiEncoder:
					ucCode=ucHW_IngressiEncoder();
					break;
				case enumWinId_AltriParametriProduzione:
					ucCode=ucHW_AltriParametriProduzione();
					break;
				case enumWinId_Fornitori:
					ucCode=ucHW_Fornitori();
					break;
				case enumWinId_Info:
					ucCode=ucHW_Info();
					break;
				case enumWinId_LottoDiretto:
					ucCode=ucHW_inserisciLottoDiretto();
					break;
				case enumWinId_LogProduzione:
					ucCode=ucHW_LogProduzione();
					break;
				case enumWinId_TaraDistanziatore:
					ucCode=ucHW_TaraDistanziatore();
					break;
				case enumWinId_SaveParams:
					ucCode=ucHW_SaveParams();
					break;
				case enumWinId_DataOra:
					ucCode=ucHW_DataOra();
					break;
				case enumWinId_spindle_grinding:
					ucCode=ucHW_spindle_grinding();
					break;
				case enumWinId_ViewCodiceProdotto:
					ucCode=ucHW_ViewCodiceProdotto();
					break;
				case enumWinId_CambioBoccola:
					ucCode=ucHW_CambioBoccola();
					break;
				case enumWinId_CalibraSensore:
					ucCode=ucHW_CalibraSensore();
					break;
				case enumWinId_SelezionaTaratura:
					ucCode=ucHW_SelezionaTaratura();
					break;
				case enumWinId_LoadParsMac:
					ucCode=ucHW_LoadParsMac();
					break;
				case enumWinId_LoadCodiciProdotto:
					ucCode=ucHW_LoadCodiciProdotto();
					break;
				case enumWinId_LoadCustomLanguage:
					ucCode=ucHW_LoadCustomLanguage();
					break;
				case enumWinId_ResetCodiciProdotto:
					ucCode=ucHW_ResetCodiciProdotto();
					break;
				case enumWinId_Distanziatore:
					ucCode=ucHW_Distanziatore();
					break;
				case enumWinId_Temperature_Compensation_Params:
					ucCode=ucHW_temperature_compensation_params();
					break;
			}
			if (ucCode!=2)
				break;
		}
	}
	return ucCode;

}






