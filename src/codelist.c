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
#include "linguedefault.h"
// definizione della variabile che contiene la lista dei codici prodotto
extern TipoStructLavori *pJobsSelected_Jobs,*pJobsRunning_Jobs;
extern TipoStructCodeJobList *pJobsSelected,*pJobsRunning;


extern unsigned char ucStartLavorazione_enabled;

unsigned char ucEnableStartLavorazione(unsigned char ucEnable);
unsigned char ucIsDisabledStartLavorazione(void);
// restituisce 1 se indice elemento attivo nella lista produzione � cambiato
unsigned char ucAlignJobsRunningAndSelected(void);
// rende valida la struttura nvram_struct.codelist, impostando crc e validkey, per avere un qlcs che ci permetta di dire
// se la struttura � valida o no...
void vValidateCodeList(void);
// restituisce 1 se la struttura his, contiene un codice prodotto valido...
unsigned char ucIsValidCodeList(void);

void vSetActiveProductCode(unsigned char ucIdxActiveElem);
// rinfresca tutti i link al codice prodotto corrente...
void vRefreshActiveProductCode(void);
// sostituisco nella joblist il codice programma attivo
// serve ad es quando viene cambiata la tolleranza nella pagina di setup produzione
void vUpdateActiveCodeInJobList(unsigned char *pucCodice);
// permette di verificare se esiste nella joblist un codice
// restituisce l'idx se joblist contiene il codice, 0xff altrimenti
unsigned char ucIdxCodeInJobList(unsigned char *pucCodice);
// permette di inserire nella joblist un nuovo codice
// restituisce 1 se joblist ha spazio per un nuovo codice
unsigned char ucInsertCodeInJobList(unsigned char *pucCodice,unsigned char ucManualJob);
// cancellazione di un codice dalla joblist, dato l'indice
unsigned char ucDeleteCodeInJobList_Idx(unsigned char ucIdxElem2delete);
unsigned char ucDeleteCodeInJobList(unsigned char *pucCodice);
// effettua il purge della coda dei job
unsigned char ucPurgeJobList(void);
// inizializzazione della lista codici
void vInitListaCodici(void);

typedef struct _TipoStructCalcLimitRpm{
	float fvelFiloMmin;
	float f_aux2;
	float f_limitFactor;
	float f_deltaUtente;
}TipoStructCalcLimitRpm;
xdat TipoStructCalcLimitRpm calcLimitRpm;


// limitazione numero di giri...
// per evitare che le frizioni debbano ruotare a velocit� troppo alta
// se numero di giri mandrino eccessivo
unsigned int uiLimitCommRpm(unsigned int uiRpm){
	extern xdata int iMaxIncrementVelRuote[2];
	/* Puntatore alla struttura programma per accedere comodamente ai dati. */
	PROGRAMMA * xdat pPro;
	unsigned char xdat n;
	long xdata myLong;
	
   // se numero di giri basso, non ha senso verificare saturazione!!!!
	if (uiRpm<100){
		for (n=0;n<NUM_FRIZIONI;n++){
			iMaxIncrementVelRuote[n]=100;
		}
		return uiRpm;
	}


	/* Punto all' entry dell' array dei programmi che contiene il programma in visualizzazione. */
	pPro=hprg.ptheAct;
	// imposto fattore di limitazione ad 1
	calcLimitRpm.f_limitFactor=1.0;
	// calcolo velocit� di lavoro in millimetri/minuto
	calcLimitRpm.fvelFiloMmin=uiRpm*(PI_GRECO*(pPro->diametro_filo+pPro->diametro_mandrino));
	for (n=0;n<NUM_FRIZIONI;n++){
		if (macParms.diam_frizioni[n]<defMarginRpmFrizioni)
			continue;
		// calcolo velocit� lavoro frizioni in rpm 
		pPro->vel_frizioni[n]=(DEFAULT_DELTA_RPM_FRIZ/100.0)*(calcLimitRpm.fvelFiloMmin/macParms.diam_frizioni[n])*(1.0/PI_GRECO);

	  /* Se i potenziometri sono abilitati, allora prendo per buono il valore dato da essi. */
		if (!(spiralatrice.CheckPotVel[n]||spiralatrice.AbilitaModificaPotenziometri))
			calcLimitRpm.f_deltaUtente=spiralatrice.pComm->vruote_comm[n];
		else
			/* Altrimenti uso il valore memorizzato nel programma. */
			calcLimitRpm.f_deltaUtente=pPro->vruote_prg[n];
		//f_deltaUtente=FS_ADC   ->200%
		//f_deltaUtente=FS_ADC/2 ->100%
		calcLimitRpm.f_deltaUtente=(calcLimitRpm.f_deltaUtente*2)/FS_ADC;

		// se una delle frizioni dovrebbe ruotare a velocit� troppo alta, devo limitare i giri mandrino
		// questo controllo lo faccio solo se sto regolando la velocit� mandrino, se sto regolando le ruote di contrasto non modifico
		// la velocit� mandrino
		if ((ucLinkEncoder==enumSpeedRegulation_Spindle)||(macParms.uc_1Encoder_0Potenziometri==0)){
			if (pPro->vel_frizioni[n]*calcLimitRpm.f_deltaUtente>macParms.rpm_frizioni[n]-defMarginRpmFrizioni){
				// trovo il rapporto di cui deve essere diminuita la velocit�
				calcLimitRpm.f_aux2=((float)(macParms.rpm_frizioni[n]-defMarginRpmFrizioni))/(pPro->vel_frizioni[n]*calcLimitRpm.f_deltaUtente);
				// imposto limite per velocit� frizioni...
				pPro->vel_frizioni[n]=(macParms.rpm_frizioni[n]-defMarginRpmFrizioni)/calcLimitRpm.f_deltaUtente;
				// imposto fattore di correzione al valore pi� limitante
				if (calcLimitRpm.f_aux2<calcLimitRpm.f_limitFactor)
					calcLimitRpm.f_limitFactor=calcLimitRpm.f_aux2;
			}
		}
		// se velocit� frizioni sufficientemente elevata, verifico qual � il max incremento che posso
		// dare: potr� essere positivo se siamo al di sotto della velocit� massimo, 
		// negativo se la stiamo superando
		if (pPro->vel_frizioni[n]*16>macParms.rpm_frizioni[n]){
			// calcolo il max incremento percentuale della velocit� delle ruote di contrasto...
			// uso un long per evitare problemi di saturazione della variabile intera...
			myLong=100L*(((float)(macParms.rpm_frizioni[n]-defMarginRpmFrizioni))/pPro->vel_frizioni[n]-1.0)+0.5;
			if (myLong>100)
				myLong=100;
			else if (myLong<-80)
				myLong=-80;
			iMaxIncrementVelRuote[n]=myLong;
		}
		// se velocit� frizioni bassa, allora il max increment � certamente il +100%
		else
			iMaxIncrementVelRuote[n]=100;
	}
	// se selezionato potenziometro mandrino e devo limitare la velocit�, procedo alla limitazione!!!!
	if ((ucLinkEncoder==enumSpeedRegulation_Spindle)||(macParms.uc_1Encoder_0Potenziometri==0)){
		if ((calcLimitRpm.f_limitFactor>0)&&(calcLimitRpm.f_limitFactor<1)){
			uiRpm*=calcLimitRpm.f_limitFactor;
			// imposto encoder mandrino in modo da limitare numero di giri...
			vSetHRE_potentiometer(enumSpeedRegulation_Spindle,uiRpm);
			// devo anche selezionare il potenziometro per posizionare correttamente la lettura
			vSelectHRE_potentiometer(enumSpeedRegulation_Spindle);
		}
	}
	return uiRpm;
}//unsigned int uiLimitCommRpm(unsigned int uiRpm)



// finestra di gestione della lista codici...
unsigned char ucHW_listaCodici(void){


	extern xdata TipoStructHandleInserimentoCodice his_privato;
	// codice che identifica il tipo di chiamata: inserisci codice prodotto, yes/no, etc
	#define Win_CODELIST_code 0


	#define defOffsetRow_listaLavori 46
	#define defCODELIST_rowTitle (8)
	#define defCODELIST_colTitle (8)
	// riga e colonna del button che contiene il primo codice...
	#define defCODELIST_row 44
	#define defCODELIST_col 0
	// offset tra le righe che contengono i codici
	#define defCODELIST_row_offset 26
	#define defCODELIST_rowUp (defLL_Codice_row)
	#define defCODELIST_rowNameCols (defLL_Codice_rowUp+28)
	#define defCODELIST_rowCodes (defLL_Codice_rowNameCols+14)
	#define defCODELIST_rowDn (defLL_Codice_rowCodes+defNumLavoriDaVisualizzare*42-4)

	#define defCODELIST_rowDelete (defLcdWidthY_pixel-32-8-8)
	#define defCODELIST_colDelete (defLcdWidthX_pixel-14*8-12)

	#define defCODELIST_rowView (defCODELIST_rowDelete)
	#define defCODELIST_colView (defLcdWidthX_pixel/2-24)

	#define defCODELIST_rowOk (defLcdWidthY_pixel-32-8)
	#define defCODELIST_colOk (defLcdWidthX_pixel-32*2-8)

	#define defCODELIST_rowPrevious (defLL_Codice_rowOk)
	#define defCODELIST_colPrevious (defLL_Codice_colOk-32*2-8)

	

	// pulsanti della finestra
	typedef enum {
			enumCODELIST_code_1,
			enumCODELIST_code_2,
			enumCODELIST_code_3,
			enumCODELIST_code_4,
			enumCODELIST_code_5,
			enumCODELIST_Delete,
			enumCODELIST_ViewCodice,
			enumCODELIST_Previous,
			enumCODELIST_Up,
			enumCODELIST_Cut,
			enumCODELIST_Esc,
			enumCODELIST_Dn,
			enumCODELIST_Ok,
			enumCODELIST_Title,
			enumCODELIST_NumOfButtons
		}enumButtons_CODELIST;


	xdata unsigned char i,idxParametro;
	xdata unsigned int uiColorButton;
	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			// prelevo l'indice del parametro...
			idxParametro=defGetWinParam(Win_CODELIST_code);
			// ritorno da visualizzazione codice???
			if (idxParametro==enumCODELIST_ViewCodice){
			}
			// CANCELLAZIONE CODICE???
			else if (idxParametro==enumCODELIST_Delete){
				// solo se risposto di s� cancello il codice...
				if (uiGetWindowParam(enumWinId_YesNo,defWinParam_YESNO_Answer)){
					ucDeleteCodeInJobList_Idx(nvram_struct.codelist.ucIdxActiveElem);
				}
			}
			// inserimento codice???
			else if ((idxParametro>=enumCODELIST_code_1)&&(idxParametro<=enumCODELIST_code_5)){
				// inserisco il nuovo codice in coda????
				// solo se confermato tramite ok e valido-->devo trovarlo diverso da zero!!!
				if (his_privato.ucCodice[0]){
					ucInsertCodeInJobList(his_privato.ucCodice,0);
				}
			}
			// faccio in modo di puntare sempre al codice prodotto corretto
			vRefreshActiveProductCode();
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			ucEnableStartLavorazione(1);
			for (i=0;i<enumCODELIST_NumOfButtons;i++)
				ucCreateTheButton(i); 
			// verifica per sicurezza
			if (nvram_struct.codelist.ucNumElem>defMaxCodeJobList){
				nvram_struct.codelist.ucNumElem=0;
			}
			if (nvram_struct.codelist.ucIdxActiveElem>=nvram_struct.codelist.ucNumElem)
				nvram_struct.codelist.ucIdxActiveElem=0;
			// trasferisco i dati da nvram_struct.commesse a lista lavori
			vTrasferisciDaCommesseAlistaLavori();

			vSetStatusCurrentWindow(enumWindowStatus_Active);
			// validazione lista codici??? la faccio da reset windows...
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enumCODELIST_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enumCODELIST_Delete:
							// non posso cancellare entry se � quella del prg running
							if (    (pJobsRunning==&nvram_struct.codelist.codeJobList[nvram_struct.codelist.ucIdxActiveElem])
							      &&(PrgRunning)
							   ){
								break;
							}
							// salvo il parametro che indica dove andr� copiata l'info
							defSetWinParam(Win_CODELIST_code,i);
							// chiedo conferma prima di cancellare l'entry corrente!
							vStringLangCopy(hw.ucStringNumKeypad_in,enumStr20_DeleteEntryCodeList);
							mystrcpy(hw.ucStringNumKeypad_out,his_privato.ucCodice,sizeof(hw.ucStringNumKeypad_out)-1);
							// impostazione del parametro 0 al valore 0--> visualizza pulsanti yes e no
							ucSetWindowParam(enumWinId_YesNo,def_WinYesNo_Param_TipoPulsante, def_WinYesNo_PUlsantiYesNo);
							// visualizzo yes/no...
							ucCallWindow(enumWinId_YesNo);

							break;

						case enumCODELIST_Cut:
							/* Indico di eseguire un taglio asincrono. */
							vDoTaglio();
							// non occorre rinfrescare la finestra
							break;
						// richiesto di modificare il codice prodotto!!!!
						case enumCODELIST_code_1:
						case enumCODELIST_code_2:
						case enumCODELIST_code_3:
						case enumCODELIST_code_4:
						case enumCODELIST_code_5:
							idxParametro=i-enumCODELIST_code_1;
							// se premuto button cui corrisponde un codice valido, allora lo faccio diventare attivo
							// se premuto button cui corrisponde un codice vuoto,  allora devo inserire un nuovo codice in lista
							if (idxParametro<nvram_struct.codelist.ucNumElem){
								nvram_struct.codelist.ucIdxActiveElem=idxParametro; // imposto indice dell'elemento attivo
								vSetActiveProductCode(nvram_struct.codelist.ucIdxActiveElem);
							}
							else{
								// salvo il parametro che indica dove andr� copiata l'info
								defSetWinParam(Win_CODELIST_code,i);
								// imposto il codice
								memcpy(&his_privato,&his,sizeof(his_privato));
								// impostazione del parametro della finestra
								ucSetWindowParam(enumWinId_CodiceProdotto,defIdxParam_ArchivioProdotti_showDeleteButton,0);
								// chiamo la funzione di impostazione codice prodotto
								ucCallWindow(enumWinId_CodiceProdotto);
							}
							// indico di rinfrescare la finestra
							return 2;
			
						case enumCODELIST_ViewCodice:
							// salvo il parametro che indica dove andr� copiata l'info
							defSetWinParam(Win_CODELIST_code,i);
							ucCallWindow(enumWinId_ViewCodiceProdotto);
							return 2;

						case enumCODELIST_Title:
						case enumCODELIST_Esc:
							vValidateCodeList();
							// faccio in modo di puntare sempre al codice prodotto corretto
							vRefreshActiveProductCode();
							// chiamo il main menu...
							vJumpToWindow(enumWinId_MainMenu);
							// indico di rinfrescare la finestra
							return 2;

						case enumCODELIST_Up:
							break;
						case enumCODELIST_Dn:
							break;
						case enumCODELIST_Previous:
							vValidateCodeList();
							vJumpToWindow(enumWinId_ListaLavori);
							// indico di rinfrescare la finestra
							return 2;

						case enumCODELIST_Ok:
							vValidateCodeList();
							vRefreshActiveProductCode();
							// solo se lista contiene almeno un codice, ha senso che io vada
							// nella finestra jobs
							if (nvram_struct.codelist.ucNumElem){
								// salto alla finestra di inserimento lotto di lavorazione
								vJumpToWindow(enumWinId_ListaLavori);
							}
							// indico di rinfrescare la finestra
							return 2;
						default:
							break;
					}
				}
			}
			// PRESENTAZIONE DELLA FINESTRA...


			//strcpy(hw.ucString,"Single job");
			// "rapido" portarlo su menu principale
			//vStringLangCopy(hw.ucString,enumStr20_SingleJob);
			//ucPrintStaticButton(hw.ucString,defLL_Codice_rowUp,defLL_Codice_col+64,enumFontMedium,enumLL_SingleJob,defLCD_Color_Trasparente);

			//vStringLangCopy(hw.ucString,enumStr20_L_Mode+(pJobsSelected_Jobs->ucWorkingMode_0L_1LR?1:0));
			//ucPrintStaticButton(hw.ucString,defLL_Codice_rowUp,defLL_Codice_col+64+6*16U+12U,enumFontMedium,enumLL_Mode,defLCD_Color_Trasparente);
			


			//strcpy(hw.ucString,"   Lista lavori   ");
			vStringLangCopy(hw.ucString,enumStr20_Lista_Codici);
			ucPrintTitleButton(hw.ucString,defCODELIST_rowTitle,defCODELIST_colTitle,enumFontMedium,enumCODELIST_Title,defLCD_Color_Trasparente,1);

			if (nvram_struct.codelist.ucNumElem){
				// non posso cancellare entry se � quella del prg running
				if (    (pJobsRunning==&nvram_struct.codelist.codeJobList[nvram_struct.codelist.ucIdxActiveElem])
				      &&(PrgRunning)
				   ){
					
				}
				else{
					vStringLangCopy(hw.ucString,enumStr20_DeleteCode);
					ucPrintStaticButton(hw.ucString,defCODELIST_rowDelete,defCODELIST_colDelete,enumFontSmall,enumCODELIST_Delete,defLCD_Color_Trasparente);
				}
				//vStringLangCopy(hw.ucString,enumStr20_ViewCode);
				//ucPrintStaticButton(hw.ucString,defCODELIST_rowView,defCODELIST_colView,enumFontSmall,enumCODELIST_ViewCodice,defLCD_Color_Trasparente);

			}

			// codice prodotto con intestazione
			// "FILO        LEGA      MANDRINO  NF  TOLL",
			vString40LangCopy(hw.ucString,enumStr40_codefields);
			ucPrintSmallText_ColNames(hw.ucString,defCODELIST_row-12,defCODELIST_col);
			// PRIMA DI STAMPARE IL CODICE, NE VISUALIZZO LE VARIE PARTI COME TESTO STATICO PER CAMPIZZARLO
			
			for (i=0;i<nvram_struct.codelist.ucNumElem;i++){
				mystrcpy(hw.ucString,nvram_struct.codelist.codeJobList[i].ucCodice,sizeof(hw.ucString)-1);
				if (i==nvram_struct.codelist.ucIdxActiveElem){
					if (nvram_struct.codelist.codeJobList[i].jobs.ucNumLavoriInLista==0)
						uiColorButton=defLCD_Color_Gray;
					else
						uiColorButton=defLCD_Color_Yellow;
				}
				else
					uiColorButton=defLCD_Color_Trasparente;
				ucPrintStaticButton(hw.ucString,defCODELIST_row+i*defCODELIST_row_offset,defCODELIST_col,enumFontMedium,enumCODELIST_code_1+i,uiColorButton);
			}
			if (nvram_struct.codelist.ucNumElem<defMaxCodeJobList){
				sprintf(hw.ucString," %-*.*s",19,19,pucStringLang(enumStr20_Aggiungi));
				ucPrintStaticButton(hw.ucString,defCODELIST_row+i*defCODELIST_row_offset,defCODELIST_col,enumFontMedium,enumCODELIST_code_1+i,defLCD_Color_Trasparente);
			}


			vAddStandardButtonsOk(enumCODELIST_Previous);

			return 1;
	}
}//lista lavori



static xdata unsigned char ucTheStringLang_Aux[41];
extern unsigned int ui_custom_string_read_error;

void vStringLangCopy(unsigned char *pdst,enumStringhe20char stringa){
	xdata unsigned long ulAux;
	ulAux=stringa;
	if (ulAux>=enumStr20_NumOf){
		strcpy(pdst,"   ");
		return;
	}
	switch(nvram_struct.actLanguage){
		case 2:
			pRead_custom_string20(ulAux,ucTheStringLang_Aux);
			if (!ui_custom_string_read_error){
				if ((ucTheStringLang_Aux[0]>=20)&&(ucTheStringLang_Aux[0]<=0x7f)){
					//strcpy(pdst,ucTheStringLang_Aux);
					mystrcpy(pdst,ucTheStringLang_Aux,20);

					break;
				}
			}
			// se stringa contiene caratteri non validi, metto in inglese!!!!
		case 1:
			//strcpy(pdst,&ucStringhe20char_english[ulAux][0]);
			mystrcpy((char*)pdst,(char*)&ucStringhe20char_english[ulAux][0],20);
			break;
		case 0:
		default:
			//strcpy(pdst,&ucStringhe20char_default[ulAux][0]);
			mystrcpy((char*)pdst,(char*)&ucStringhe20char_default[ulAux][0],20);
			break;
	}
	
}


unsigned char *pucStringLang(enumStringhe20char stringa){
	xdata unsigned long ulAux;
	ulAux=stringa;
	switch(nvram_struct.actLanguage){
		case 2:
			pRead_custom_string20(ulAux,ucTheStringLang_Aux);
			if (!ui_custom_string_read_error){			
				if ((ucTheStringLang_Aux[0]>=20)&&(ucTheStringLang_Aux[0]<=0x7f)){
					// metto sempre il tappo per sicurezza
					ucTheStringLang_Aux[20]=0;
					return &ucTheStringLang_Aux[0];
					break;
				}
			}
			// se stringa contiene caratteri non validi, metto in inglese!!!!
		case 0:
		default:
			mystrcpy((char*)ucTheStringLang_Aux,(char*)ucStringhe20char_default[ulAux],20);
			return &ucTheStringLang_Aux[0];
			break;
		case 1:
			mystrcpy((char*)ucTheStringLang_Aux,(char*)ucStringhe20char_english[ulAux],20);
			return &ucTheStringLang_Aux[0];
			break;
	}
	
}


void vString40LangCopy(unsigned char *pdst,enumStringhe40char stringa){
xdata unsigned long ulAux;
	ulAux=stringa;
	if (ulAux>=enumStr40_NumOf){
		strcpy(pdst,"   ");
		return;
	}
	switch(nvram_struct.actLanguage){
		case 2:
			// copio in ucTheStringLang_Aux, che puo' contenere bene fino a 40 caratteri...
			pRead_custom_string40(ulAux,ucTheStringLang_Aux);
			if (!ui_custom_string_read_error){			
				if ((ucTheStringLang_Aux[0]>=20)&&(ucTheStringLang_Aux[0]<=0x7f)){
					mystrcpy(pdst,ucTheStringLang_Aux,40);
					break;
				}
			}
			// se stringa contiene caratteri non validi, metto in inglese!!!!
		case 1:
			mystrcpy((char*)pdst,(char*)&ucStringhe40char_english[ulAux][0],40);
			break;
		case 0:
		default:
			mystrcpy((char*)pdst,(char*)&ucStringhe40char_default[ulAux][0],40);
			break;
	}
	
}
