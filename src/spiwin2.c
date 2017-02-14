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
#include "i2c_master.h"

xdata TipoStructLavori *pJobsSelected_Jobs,*pJobsRunning_Jobs;
xdata TipoStructCodeJobList *pJobsSelected,*pJobsRunning;
extern void vInitializeHisFields(TipoStructHandleInserimentoCodice *phis);
extern void vSetActiveNumberProductCode(TipoStructHandleInserimentoCodice *phis);
xdata unsigned char uc_after_ohm_m_fili_jump_to_enumWinId_Distanziatore;

extern int mystricmp(char *s1,char *s2);
extern void vVerifyInsertCodiceProdotto(xdata char *s);
int mystricmp(char *s1,char *s2){
	while(*s1&&*s2&&(toupper(*s1)==toupper(*s2))){
		s1++;
		s2++;
	}
	return *s1-*s2;
}

void vVerifyInsertCodiceProdotto(xdata char *s){
	// reset struttura di ricerca codice prodotto
	memset(&scp,0,sizeof(scp));
    // indico di effettuare la ricerca esatta
	mystrcpy(scp.ucString,s,sizeof(scp.ucString)-1);
	scp.ucFindExactCode=1;
	if (!hprg.firstPrg)
		scp.uiStartRecord=0;
	else
		scp.uiStartRecord=hprg.firstPrg;
	scp.uiNumMaxRecord2Find=1;
	// se il codice esiste...
	if (ucSearchCodiceProgramma()){
		uiSetActPrg(scp.uiLastRecordFound);
	}
	// altrimenti lo devo inserire...
	else{
		if (InsertPrg(s))
			InsertTheProgram(s);
		// la funzione InsertPrg() imposta gi� il programma appena inserito come programma corrente
	}
	// effettuo la validazione dei parametri programma...
	ValidatePrg();
	// salvo il programma
	ucSaveActPrg();
}



// restituisce 1 se indice elemento attivo nella lista produzione � cambiato
unsigned char ucAlignJobsRunningAndSelected(void){
	static unsigned char ucOldPrgRunning=0;
	// se programma non � in esecuzione, aggiorno puntatore a job running 
	if (!PrgRunning){
		// se programma era in esecuzione ed ha ancora lavori in lista,
		// adesso che � terminato allineo job corrente con job che era running
		if (ucOldPrgRunning&&pJobsRunning_Jobs->ucNumLavoriInLista){
			pJobsSelected=pJobsRunning;
			pJobsSelected_Jobs=pJobsRunning_Jobs;
			// ritrovo l'indice dell'elemento attivo
			nvram_struct.codelist.ucIdxActiveElem=pJobsSelected-&nvram_struct.codelist.codeJobList[0];
			ucOldPrgRunning=PrgRunning;
			return 1;
		}
		// se invece il job non era running, riassegno
		else{
			pJobsRunning=pJobsSelected;
			pJobsRunning_Jobs=pJobsSelected_Jobs;
		}
	}
	ucOldPrgRunning=PrgRunning;
	return 0;
}

// numero di validazioni della nvram_struct.codelist
xdata unsigned int uiNumValidationCodeList;

unsigned int uiGetNumValidationCodeList(void){
	return uiNumValidationCodeList;
}//unsigned int uiGetNumValidationCodeList

void vCodelistTouched(void){
	uiNumValidationCodeList++;
}

// rende valida la struttura nvram_struct.codelist, impostando crc e validkey, per avere un qlcs che ci permetta di dire
// se la struttura � valida o no...
void vValidateCodeList(void){
	unsigned char xdata *xdata pc;
	unsigned int xdata i;
	unsigned int xdata uiCrc;
	vCodelistTouched();
	nvram_struct.codelist.uiValidKey_0xa536=0xa536;
	pc =(unsigned char xdata *)&nvram_struct.codelist;
	uiCrc=0;
	for (i=0;i<sizeof(nvram_struct.codelist)-sizeof(nvram_struct.codelist.uiCrc);i++){
		uiCrc^=*pc++;
	}
	uiCrc=~uiCrc;
	nvram_struct.codelist.uiCrc=uiCrc;
}//void vValidateCodeList(void)

// restituisce 1 se la struttura his, contiene un codice prodotto valido...
unsigned char ucIsValidCodeList(void){
	// verifico che questa variabile non abbia un valore errato!
	if (  (nvram_struct.codelist.ucIdxActiveElem>=nvram_struct.codelist.ucNumElem)
		 ||(nvram_struct.codelist.ucIdxActiveElem>=sizeof(nvram_struct.codelist.codeJobList)/sizeof(nvram_struct.codelist.codeJobList[0]))	
	   ){
		nvram_struct.codelist.ucIdxActiveElem=0;		
	}
	// non controllo il crc, ci sono troppi punti dove viene variata la lista lavori...
	if (nvram_struct.codelist.uiValidKey_0xa536!=0xa536)
		return 0;
#if 0
	unsigned char xdata *xdata pc;
	unsigned int xdata i;
	unsigned int xdata uiCrc;
	pc =(unsigned char xdata *)&nvram_struct.codelist;
	uiCrc=0;
	for (i=0;i<sizeof(nvram_struct.codelist)-sizeof(nvram_struct.codelist.uiCrc);i++){
		uiCrc^=*pc++;
	}
	uiCrc=~uiCrc;
	if (uiCrc!=nvram_struct.codelist.uiCrc){
		return 0;
	}
#endif
	return 1;
}//unsigned char ucIsValidCodeList(void)



void vSetActiveProductCode(unsigned char ucIdxActiveElem){
	xdata unsigned char ucNumLoop;

	for (ucNumLoop=0;ucNumLoop<2;ucNumLoop++){
		// copio nella struttura his il codice opportuno e lo inizializzo
		mystrcpy(his.ucCodice,nvram_struct.codelist.codeJobList[ucIdxActiveElem].ucCodice,sizeof(his.ucCodice)-1);
		// inizializzo i campi della struttura his
		vInitializeHisFields(&his);
		// imposto il codice attuale come codice prodotto attivo--> trova il numero del programma nella lista dei programmi!
		vSetActiveNumberProductCode(&his);

		// imposto la lista pJobsSelected_Jobs->..
		pJobsSelected=&nvram_struct.codelist.codeJobList[ucIdxActiveElem];
		pJobsSelected_Jobs=&nvram_struct.codelist.codeJobList[ucIdxActiveElem].jobs;
		// se codice non cambia, non rifaccio il loop
		if (!ucAlignJobsRunningAndSelected())
			break;
	}
	// trasferisco da lista lavori a nvram_struct.commesse...
	vTrasferisciDaListaLavoriAcommesse();
	vInizializzaCodiceProdotto();
}//void vSetActiveProductCode(unsigned char ucIdxActiveElem)

// rinfresca tutti i link al codice prodotto corrente...
void vRefreshActiveProductCode(void){
	vSetActiveProductCode(nvram_struct.codelist.ucIdxActiveElem);
}
// sostituisco nella joblist il codice programma attivo
// serve ad es quando viene cambiata la tolleranza nella pagina di setup produzione
void vUpdateActiveCodeInJobList(unsigned char *pucCodice){
	vCodelistTouched();
	// copio nella struttura il codice opportuno
	mystrcpy(nvram_struct.codelist.codeJobList[nvram_struct.codelist.ucIdxActiveElem].ucCodice,pucCodice,sizeof(nvram_struct.codelist.codeJobList[nvram_struct.codelist.ucIdxActiveElem].ucCodice)-1);
	vRefreshActiveProductCode();
}


// permette di verificare se esiste nella joblist un codice
// restituisce l'idx se joblist contiene il codice, 0xff altrimenti
unsigned char ucIdxCodeInJobList(unsigned char *pucCodice){
	unsigned char xdata i;
	for (i=0;i<nvram_struct.codelist.ucNumElem;i++){
		// verifico il codice i-esimo...
		if (strcmp(nvram_struct.codelist.codeJobList[i].ucCodice,pucCodice)==0)
			return i;
	}
	return 0xff;
}


// permette di inserire nella joblist un nuovo codice
// restituisce 1 se joblist ha spazio per un nuovo codice
unsigned char ucInsertCodeInJobList(unsigned char *pucCodice,unsigned char ucManualJob){
	uiNumValidationCodeList++;
	if (nvram_struct.codelist.ucNumElem>=defMaxCodeJobList)
		return 0;
	// copio il codice...
	mystrcpy(nvram_struct.codelist.codeJobList[nvram_struct.codelist.ucNumElem].ucCodice,pucCodice,sizeof(nvram_struct.codelist.codeJobList[nvram_struct.codelist.ucNumElem].ucCodice)-1);
	// azzero la lista lavori associata al codice
	memset(&nvram_struct.codelist.codeJobList[nvram_struct.codelist.ucNumElem].jobs,0,sizeof(nvram_struct.codelist.codeJobList[nvram_struct.codelist.ucNumElem].jobs));
	nvram_struct.codelist.codeJobList[nvram_struct.codelist.ucNumElem].ucManual=ucManualJob;
	// imposto elemento corrente come attivo
	nvram_struct.codelist.ucIdxActiveElem=nvram_struct.codelist.ucNumElem; // imposto indice dell'elemento attivo
	// incremento il numero di elementi
	nvram_struct.codelist.ucNumElem++;
	vSetActiveProductCode(nvram_struct.codelist.ucIdxActiveElem);
	return 1;
}



// cancellazione di un codice dalla joblist, dato l'indice
unsigned char ucDeleteCodeInJobList_Idx(unsigned char ucIdxElem2delete){
	unsigned char i;
	vCodelistTouched();
	if (ucIdxElem2delete==0xFF)
		return 0;
	if (ucIdxElem2delete>=nvram_struct.codelist.ucNumElem)
		return 0;
	// faccio scorrere tutto in su di un posto, se sono all'interno della lista
	if (ucIdxElem2delete<nvram_struct.codelist.ucNumElem-1){
		for (i=ucIdxElem2delete;i<nvram_struct.codelist.ucNumElem;i++){
			memcpy(&nvram_struct.codelist.codeJobList[i],&nvram_struct.codelist.codeJobList[i+1],sizeof(nvram_struct.codelist.codeJobList[0]));
		}
	}
	// decremento il numero di elementi in lista
	if (nvram_struct.codelist.ucNumElem){
		// reset ultimo elemento della lista
		memset(&nvram_struct.codelist.codeJobList[nvram_struct.codelist.ucNumElem-1],0,sizeof(nvram_struct.codelist.codeJobList[0]));
		nvram_struct.codelist.ucNumElem--;
	}
	// verifico indice elemento attivo... deve essere all'interno della lista!
	if (nvram_struct.codelist.ucIdxActiveElem>=nvram_struct.codelist.ucNumElem){
		// se sono fuori dalla lista, devo rientrare nella lista, mi posiziono all'ultimo elemento
		if (nvram_struct.codelist.ucNumElem)
			nvram_struct.codelist.ucIdxActiveElem=nvram_struct.codelist.ucNumElem-1;
		else
			nvram_struct.codelist.ucIdxActiveElem=0;
	}
	// imposto elemento attivo...
	vSetActiveProductCode(nvram_struct.codelist.ucIdxActiveElem);
    return 1;
}

unsigned char ucDeleteCodeInJobList(unsigned char *pucCodice){
	return ucDeleteCodeInJobList_Idx(ucIdxCodeInJobList(pucCodice));
}	

// elimina tutti i codici MANUALI dalla lista job!
unsigned char ucPurgeManualCodesInJobList(void){
	unsigned char xdata i;
	for (i=0;i<nvram_struct.codelist.ucNumElem;i++){
		if (nvram_struct.codelist.codeJobList[i].ucManual)
			ucDeleteCodeInJobList_Idx(i);
	}
	return 1;
}

// purge empty manual jobs from the list
unsigned char ucPurgeManualCodesEmptyFromJobList(void)
{
	unsigned char xdata i;
	for (i=0;i<nvram_struct.codelist.ucNumElem;i++)
	{
		TipoStructCodeJobList * p = &nvram_struct.codelist.codeJobList[i];
		if (p->ucManual && !p->jobs.ucNumLavoriInLista)
		{
			ucDeleteCodeInJobList_Idx(i);
		}
	}
	return 1;
}

TipoLavoro *pFindManualCodeInJobList(void){
	xdata unsigned char i;
	for (i=0;i<nvram_struct.codelist.ucNumElem;i++){
		if (nvram_struct.codelist.codeJobList[i].ucManual)
			return &nvram_struct.codelist.codeJobList[i].jobs.lista[0];
	}
	return &pJobsSelected_Jobs->lista[0];

}


// effettua il purge della coda dei job
unsigned char ucPurgeJobList(void){
	unsigned char xdata i,ucNumCodesDeleted;
	ucNumCodesDeleted=0;
	i=0;
	while(i<nvram_struct.codelist.ucNumElem){
		if (nvram_struct.codelist.codeJobList[i].jobs.ucNumLavoriInLista==0){
			ucDeleteCodeInJobList(nvram_struct.codelist.codeJobList[i].ucCodice);
			ucNumCodesDeleted++;
		}
		else
			i++;

	}
	return ucNumCodesDeleted;
}


// inizializzazione della lista codici
void vInitListaCodici(void){
	extern void vBuildHisCodeFromFields(TipoStructHandleInserimentoCodice *phis);

	// reset codice his, se non valido...
	if (!ucIsValidHis(&his)){
		vInizializzaCodiceProdotto();
		//memset(&his,0,sizeof(his));
		//strcpy(his.ucDiametroFilo,"0.75");
		//strcpy(his.ucTipoFilo,"8020");
		//strcpy(his.ucDiametroMandrino,"1.20");
		//strcpy(his.ucNumFili,"1");
		//strcpy(his.ucFornitore,"1");
		//vBuildHisCodeFromFields(&his);
	}
	// inizializzo i campi del codice prodotto di default
	vInitializeHisFields(&his);

	// se lista codici non valida...
	if (!ucIsValidCodeList()){
		// reset lista codici
		memset(&nvram_struct.codelist,0,sizeof(nvram_struct.codelist));
		// copio il codice...
		// mystrcpy(nvram_struct.codelist.codeJobList[0].ucCodice,his.ucCodice,sizeof(nvram_struct.codelist.codeJobList[0].ucCodice)-1);
		// numero di elementi=0
		nvram_struct.codelist.ucNumElem=0;
		// valido la lista codici...
		vValidateCodeList();
	}
	// attivo il codice attuale
	vSetActiveProductCode(nvram_struct.codelist.ucIdxActiveElem);
}





xdata unsigned char ucSaveCodice_his[defCodice_MaxChar+1];// codice prodotto

void vGetValueParametro_spiwin2(unsigned char *pString,unsigned char idxParametro,code TipoStructInfoParametro_Lingua *pif){
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

void vSetValueParametro_spiwin2(unsigned char *pString,unsigned char idxParametro,code TipoStructInfoParametro_Lingua *pif){
	xdata int i;
	xdata long theLong;
	xdata float theFloat;
	signed short theSignedShort;
	unsigned short theUnsignedShort;
	switch(pif[idxParametro].paramType){
		case enumPT_unsignedChar:
			i=atoi(pString);
			*(unsigned char *)pif[idxParametro].pc=i&0xFF;
			break;
		case enumPT_signedChar:
			i=atoi(pString);
			*(signed char *)pif[idxParametro].pc=i&0xFF;
			break;
		case enumPT_unsignedInt:
			i=atoi(pString);
			*(unsigned int *)pif[idxParametro].pc=i;
			break;
		case enumPT_signedInt:
			i=atoi(pString);
			*(signed int *)pif[idxParametro].pc=i;
			break;
		case enumPT_unsignedLong:
			theLong=atol(pString);
			*(unsigned long *)pif[idxParametro].pc=theLong;
			break;
		case enumPT_signedLong:
			theLong=atol(pString);
			*(signed long *)pif[idxParametro].pc=theLong;
			break;
		case enumPT_float:
			theFloat=atof(pString);
			if ((nvram_struct.actUM==UM_IN)&&ucIsUmSensitive(pif[idxParametro].enumUm)){
				*(float *)pif[idxParametro].pc=theFloat/pif[idxParametro].fConvUm_mm_toUm_inch;
			}
			else
				*(float *)pif[idxParametro].pc=theFloat;
			
			break;
		case enumPT_signedShort:
			theSignedShort=atoi(pString);
			*(signed short *)pif[idxParametro].pc=theSignedShort;
			break;
		case enumPT_unsignedShort:
			theUnsignedShort=atoi(pString);
			*(unsigned short *)pif[idxParametro].pc=theUnsignedShort;
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



// controlla valori minimo e massimo del numeric keypad, e verifica se sono ok
// restitusice:
// 0--> tutto ok,valore tra minimo e max
// 1--> valore saturato al minimo
// 2--> valore saturato al massimo
unsigned char ucCtrlMinMaxValue(xdata unsigned char *pValue){
	xdata float f;
	if (!paramsNumK.ucMinMaxEnable)
		return 0;
	f=atof(pValue);
	if (f<paramsNumK.fMin){
		if (!strchr(paramsNumK.ucPicture,'N')&&!strchr(paramsNumK.ucPicture,'.'))
			sprintf(pValue,"%-*i",paramsNumK.ucMaxNumChar,(int)paramsNumK.fMin);
		else
			sprintf(pValue,"%-*f",paramsNumK.ucMaxNumChar,paramsNumK.fMin);
		// metto sempre il tappo per sicurezza
		pValue[paramsNumK.ucMaxNumChar]=0;
		return 1;
	}
	else if (f>paramsNumK.fMax){
		if (!strchr(paramsNumK.ucPicture,'N')&&!strchr(paramsNumK.ucPicture,'.'))
			sprintf(pValue,"%-*i",paramsNumK.ucMaxNumChar,(int)paramsNumK.fMax);
		else
			sprintf(pValue,"%-*f",paramsNumK.ucMaxNumChar,paramsNumK.fMax);
		// metto sempre il tappo per sicurezza
		pValue[paramsNumK.ucMaxNumChar]=0;
		return 2;
	}
	else 
		return 0;
}// unsigned char ucCtrlMinMaxValue(xdata unsigned char *pValue)







//xdata TipoStructLavori lavori;

xdata TipoPrivatoLavoro privato_lavoro;

code TipoStructInfoFieldCodice_Lingua iml[4]={
	{0,defMaxCharLavoroString_Position, "nn","%-2i",1,defMaxLavoriInLista,&privato_lavoro.ucStringPosition[0],    1,1,enumStr20_Posizione,enumUm_num},
	{0,defMaxCharLavoroString_Numpezzi, "nnnnn","%-5i",1,MAX_QUANTITA,&privato_lavoro.ucStringNumpezzi[0],    1,1,enumStr20_Numero_pezzi,enumUm_num},
	{0,defMaxCharLavoroString_Ohm, "NNNNNN","%-6.2f",MIN_RES_CAN_MAKE,MAX_RES_CAN_MAKE,&privato_lavoro.ucStringOhm[0],    1,1,enumStr20_Valore_ohmico,enumUm_ohm},
	{0,defMaxCharDescrizioneLavoro, "xxxxxxxxxxxxxxxxxxxx","%-20s",0.1,999,&privato_lavoro.ucStringDescrizione[0],    1,0,enumStr20_Descrizione,enumUm_none}
};

// sbianca il record relativo al lavoro con indice ucIdx
void vEmptyLavoroInLista(unsigned char ucIdx){
	if (ucIdx>=sizeof(pJobsSelected_Jobs->lista)/sizeof(pJobsSelected_Jobs->lista[0]))
		return;
	pJobsSelected_Jobs->lista[ucIdx].uiNumeroPezzi=0;
	pJobsSelected_Jobs->lista[ucIdx].fOhm=0.0;
	pJobsSelected_Jobs->lista[ucIdx].ucDescrizione[0]=0;
	pJobsSelected_Jobs->lista[ucIdx].npezzifatti=0;
	pJobsSelected_Jobs->lista[ucIdx].magg=0;
	pJobsSelected_Jobs->lista[ucIdx].ucValidKey=defLavoroNonValidoKey;
	pJobsSelected_Jobs->lista[ucIdx].ucPosition=ucIdx+1;
	vCodelistTouched();

}


// finestra di gestione della modifica lavoro...
unsigned char ucHW_modificaLavoro(void){
	#define defWinParamNumLavoro 0
	#define defWinParamNumCampo 1
	#define defWinParamWinId 2

	#define defML_Codice_rowTitle 8
	#define defML_Codice_colTitle 28


	#define defML_Codice_rowNumPezzi (defML_Codice_rowTitle+48)
	#define defML_Codice_colNumPezzi  4

	#define defML_Codice_rowOhm (defML_Codice_rowNumPezzi)
	#define defML_Codice_colOhm  (defML_Codice_colNumPezzi+12U*16-12)

	#define defML_Codice_rowModoFunz (defML_Codice_rowNumPezzi+40)
	#define defML_Codice_colModoFunz 4

	#define defML_Codice_rowDesc (defML_Codice_rowNumPezzi+40*2)
	#define defML_Codice_colDesc  4

	#define defML_Codice_rowPos   (defML_Codice_rowNumPezzi+40*3)
	#define defML_Codice_colPos   4


	#define defML_Codice_rowOk (240-32-8)
	#define defML_Codice_rowEsc (defML_Codice_rowTitle)

    #define defML_OK_column 250

	// pulsanti della finestra
	typedef enum {
			enumML_posizione=0,
			enumML_numpezzi,
			enumML_ohm,
			enumML_descrizione,
			enumML_Ok,
			enumML_Del,
			enumML_Mode,
			enumML_Title,
			enumML_Distanziatore,
			enumML_CompensazioneTemperatura,

			enumML_Sx,
			enumML_Up,
			enumML_Cut,
			enumML_Esc,
			enumML_Dn,
			enumML_Dx,

			enumML_NumOfButtons
		}enumButtons_ModificaLavoro;


	unsigned char i,j,ucStartMove,ucEndMove;
    extern float f_temperature_coefficient;
    
	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			switch (defGetWinParam(defWinParamWinId)){
				case 0:
					// copio il valore inserito tramite numeric keypad nel campo di pertinenza
					// l'indice del parametro � stato salvato in Win_CP_field
					sprintf(iml[defGetWinParam(defWinParamNumCampo)].pc,"%-*s",paramsNumK.ucMaxNumChar,hw.ucStringNumKeypad_out);
					// metto il tappo per sicurezza...
					iml[defGetWinParam(defWinParamNumCampo)].pc[paramsNumK.ucMaxNumChar]=0;
					i=atoi(iml[0].pc);
					// se ho modificato il valore della posizione e la posizione 
					// � oltre l'ultimo lavoro, allora sto tentando di spostare in posizione non ammessa un lavoro esistente
					// e non lo permetto
					// io posso inserire lavori in ultima posizione, non copiare in ultima posizione un lavoro esistente		
					if (  (defGetWinParam(defWinParamNumCampo)==enumML_posizione) // modifico posizione
						&&(i>pJobsSelected_Jobs->ucNumLavoriInLista)							// oltre ultima
						&&(i!=defGetWinParam(defWinParamNumLavoro)+1)			// e non sto inserendo un nuovo lavoro
						)
						// allora riprendo il valore originale della posizione...
						sprintf(iml[0].pc,"%i",(int)(pJobsSelected_Jobs->lista[defGetWinParam(defWinParamNumLavoro)].ucPosition));
					break;
				case 20:
					// always refresh active product code...
					vSetActiveNumberProductCode(&his);				
					break;
			}
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enumML_NumOfButtons;i++)
				ucCreateTheButton(i); 
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			memcpy(&privato_lavoro.lavoro,&pJobsSelected_Jobs->lista[defGetWinParam(defWinParamNumLavoro)],sizeof(privato_lavoro.lavoro));
			sprintf(iml[0].pc,iml[0].ucFmtField,privato_lavoro.lavoro.ucPosition);
			sprintf(iml[1].pc,iml[1].ucFmtField,privato_lavoro.lavoro.uiNumeroPezzi);
			sprintf(iml[2].pc,iml[2].ucFmtField,privato_lavoro.lavoro.fOhm);
			sprintf(iml[3].pc,iml[3].ucFmtField,privato_lavoro.lavoro.ucDescrizione);

			return 2;
		case enumWindowStatus_Active:
			// always refresh active product code...
			vSetActiveNumberProductCode(&his);
			f_temperature_coefficient=(hprg.ptheAct->ui_temperature_coefficient_per_million*1e-6)*80.0+1.0;
		
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enumML_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enumML_Mode:
							if (pJobsSelected_Jobs->ucWorkingMode_0L_1LR)
								pJobsSelected_Jobs->ucWorkingMode_0L_1LR=0;
							// vado nel modo lenr solo se abilitato da parametro macchina
							else if (macParms.modo == MODE_LENR)
								pJobsSelected_Jobs->ucWorkingMode_0L_1LR=1;
							break;
						// richiesto di modificare il codice prodotto!!!!
						case enumML_posizione:
						case enumML_numpezzi:
						case enumML_ohm:
						case enumML_descrizione:
							// non posso cambiare posizione se non ho inserito!!!!!
						   if ( (i==enumML_posizione)
						       &&(privato_lavoro.lavoro.ucValidKey!=defLavoroValidoKey)
						       )
						       break;
							if (i==enumML_descrizione)
								paramsNumK.ucEnableT9=1;
							else
								paramsNumK.ucEnableT9=0;
							paramsNumK.ucMaxNumChar=iml[i].ucLenField;	
							// imposto stringa di picture
							mystrcpy((char*)paramsNumK.ucPicture,(char*)iml[i].ucPictField,sizeof(paramsNumK.ucPicture)-1);
							// imposto limiti min e max
							paramsNumK.fMin=iml[i].fMin;
							paramsNumK.fMax=iml[i].fMax;
							paramsNumK.enumUm=iml[i].enumUm;
							// copio il titolo, che dipende dalla lingua scelta...
							vStringLangCopy(paramsNumK.ucTitle,iml[i].stringa);
							// copio la stringa con cui si inizializza il numeric keypad
							mystrcpy(hw.ucStringNumKeypad_in,iml[i].pc,sizeof(hw.ucStringNumKeypad_in)-1);
							// imposto abilitazione uso stringa picture
							paramsNumK.ucPictureEnable=iml[i].ucPictureEnable;	// abilitazione stringa picture
							// imposto abilitazione uso min/max
							paramsNumK.ucMinMaxEnable=iml[i].ucMinMaxEnable;	// abilitazione controllo valore minimo/massimo
							// indico qual � il campo che sto modificando
							defSetWinParam(defWinParamNumCampo,i);
							// indico che sto chiamando la keyboard
							defSetWinParam(defWinParamWinId,0);
							// chiamo il numeric keypad 
							ucCallWindow(enumWinId_Num_Keypad);
							// indico di rinfrescare la finestra
							return 2;

						case enumML_Cut:
							/* Indico di eseguire un taglio asincrono. */
							vDoTaglio();
							// non occorre rinfrescare la finestra
							break;
						case enumML_CompensazioneTemperatura:
							defSetWinParam(defWinParamWinId,20);
							ucCallWindow(enumWinId_Temperature_Compensation_Params);
							// indico di rinfrescare la finestra
							return 2;
						case enumML_Distanziatore:
							// richiesto di modificare i parametri distanziatore...?
							if (privato_lavoro.lavoro.fOhm>MIN_RES_CAN_MAKE){
								if (macroIsStringaCodiceProdotto_Distanziatore(his.ucCodice)){
									// indico che NON sto chiamando la keyboard
									defSetWinParam(defWinParamWinId,1);
									// imposto flag che indica alla finestra ohm/m fili di passare poi al distanziatore
									uc_after_ohm_m_fili_jump_to_enumWinId_Distanziatore=0xa5;
									ucCallWindow(enumWinId_inserisciOhmMfili);							
								}
							}
							return 2;

						// modifica versione 2.06
						//  se si esce da menu "modifica lavoro" premendo esc, i dati vengono comunque salvati
						case enumML_Title:
						case enumML_Esc:
						case enumML_Ok:
						case enumML_Dx:
						case enumML_Sx:
							privato_lavoro.lavoro.ucPosition=atoi(iml[0].pc);
							if (privato_lavoro.lavoro.ucPosition>defMaxLavoriInLista)
								privato_lavoro.lavoro.ucPosition=defMaxLavoriInLista;
							// non posso spostarmi oltre l'ultimo record!!!
							if (privato_lavoro.lavoro.ucPosition>pJobsSelected_Jobs->ucNumLavoriInLista){
								privato_lavoro.lavoro.ucPosition=pJobsSelected_Jobs->ucNumLavoriInLista+1;
							}
							privato_lavoro.lavoro.uiNumeroPezzi=atoi(iml[1].pc);
							privato_lavoro.lavoro.fOhm=atof(iml[2].pc);
							// se num pezzi =0 od ohm=0
							if ((privato_lavoro.lavoro.uiNumeroPezzi==0)||(privato_lavoro.lavoro.fOhm==0)){
								vHandleEscFromMenu();
								// indico di rinfrescare la finestra
								return 2;
							}

							mystrcpy(privato_lavoro.lavoro.ucDescrizione,iml[3].pc,sizeof(privato_lavoro.lavoro.ucDescrizione)-1);
							// e' stata modificata la posizione del lavoro in lista???
							if (privato_lavoro.lavoro.ucPosition!=defGetWinParam(defWinParamNumLavoro)+1){
								vCodelistTouched();
							   // se qualcuno ha inserito e modificato la posizione nello stesso tempo...
								if (privato_lavoro.lavoro.ucValidKey!=defLavoroValidoKey){
									privato_lavoro.lavoro.ucValidKey=defLavoroValidoKey;
									// metto la posizione a default!!!!
									privato_lavoro.lavoro.ucPosition = defGetWinParam(defWinParamNumLavoro)+1;
								}
								// se la nuova posizione � all'interno di quelle gi� inserite...
								if (privato_lavoro.lavoro.ucPosition<=pJobsSelected_Jobs->ucNumLavoriInLista){
									// se posizione non cambiata, faccio solo una memcpy
									if (privato_lavoro.lavoro.ucPosition==pJobsSelected_Jobs->lista[defGetWinParam(defWinParamNumLavoro)].ucPosition){
										memcpy(&pJobsSelected_Jobs->lista[privato_lavoro.lavoro.ucPosition-1],&privato_lavoro.lavoro,sizeof(privato_lavoro.lavoro));
									}
									// devo spostare di posizione il lavoro
									// in basso od in alto???
									else if (privato_lavoro.lavoro.ucPosition>pJobsSelected_Jobs->lista[defGetWinParam(defWinParamNumLavoro)].ucPosition){
										// devo spostare il lavoro in basso
										// quindi devo spostare verso l'alto tutti quelli che vengono dopo...
										ucStartMove=pJobsSelected_Jobs->lista[defGetWinParam(defWinParamNumLavoro)].ucPosition;
										ucEndMove=privato_lavoro.lavoro.ucPosition;
										for (j=ucStartMove;j<ucEndMove;j++){
											memcpy(&pJobsSelected_Jobs->lista[j-1],&pJobsSelected_Jobs->lista[j],sizeof(privato_lavoro.lavoro));
											pJobsSelected_Jobs->lista[j-1].ucPosition--;
										}
										// copio il lavoro...
										memcpy(&pJobsSelected_Jobs->lista[j-1],&privato_lavoro.lavoro,sizeof(privato_lavoro.lavoro));
									}
									else{
										// devo spostare il lavoro in alto
										// quindi devo spostare verso il basso tutti quelli che vengono prima...
										ucEndMove=privato_lavoro.lavoro.ucPosition-1;
										ucStartMove=pJobsSelected_Jobs->lista[defGetWinParam(defWinParamNumLavoro)].ucPosition-1;
										for (j=ucStartMove;j>ucEndMove;j--){
											memcpy(&pJobsSelected_Jobs->lista[j],&pJobsSelected_Jobs->lista[j-1],sizeof(privato_lavoro.lavoro));
											pJobsSelected_Jobs->lista[j].ucPosition++;
										}
										// copio il lavoro...
										memcpy(&pJobsSelected_Jobs->lista[j],&privato_lavoro.lavoro,sizeof(privato_lavoro.lavoro));
									}
								}
								// inserita una posizione oltre il numero max record corrente
								// percio' la nuova posizione deve andare in append
								// no! non posso appendere!!!!--> gi� protetto nel momento in cui la return
								// verifica il valore della posizione...
								else{
									pJobsSelected_Jobs->ucNumLavoriInLista++;
									privato_lavoro.lavoro.ucPosition=pJobsSelected_Jobs->ucNumLavoriInLista;
									memcpy(&pJobsSelected_Jobs->lista[privato_lavoro.lavoro.ucPosition-1],&privato_lavoro.lavoro,sizeof(privato_lavoro.lavoro));
								}
							}
							// la posizione non � stata modificata...
							else{
								vCodelistTouched();
								privato_lavoro.lavoro.ucValidKey=defLavoroValidoKey;
								memcpy(&pJobsSelected_Jobs->lista[defGetWinParam(defWinParamNumLavoro)],&privato_lavoro.lavoro,sizeof(privato_lavoro.lavoro));
							}
							// se sono in append, incremento il numero di lavori in lista...
							if (privato_lavoro.lavoro.ucPosition>pJobsSelected_Jobs->ucNumLavoriInLista){
								pJobsSelected_Jobs->ucNumLavoriInLista++;
							    // faccio diventare il lavoro appena modificato il primo lavoro visualizzato.
								if (privato_lavoro.lavoro.ucPosition>=2)
									pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato=privato_lavoro.lavoro.ucPosition-2;
								else
									pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato=privato_lavoro.lavoro.ucPosition-1;
							}
							ucReturnFromWindow();
							return 2;
						case enumML_Del:
							if (    pJobsSelected_Jobs->ucNumLavoriInLista
								  &&(pJobsSelected_Jobs->lista[privato_lavoro.lavoro.ucPosition-1].ucValidKey==defLavoroValidoKey)
							    ){
								ucStartMove=pJobsSelected_Jobs->lista[defGetWinParam(defWinParamNumLavoro)].ucPosition;
								ucEndMove=pJobsSelected_Jobs->ucNumLavoriInLista-1;
								for (j=ucStartMove;j<=ucEndMove;j++){
									memcpy(&pJobsSelected_Jobs->lista[j-1],&pJobsSelected_Jobs->lista[j],sizeof(privato_lavoro.lavoro));
									pJobsSelected_Jobs->lista[j-1].ucPosition--;
								}
								// sbianco il lavoro...
								vEmptyLavoroInLista(ucEndMove);
								pJobsSelected_Jobs->ucNumLavoriInLista--;
								// faccio diventare il lavoro appena modificato il primo lavoro visualizzato.
								pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato=privato_lavoro.lavoro.ucPosition-1;
							}
							ucReturnFromWindow();
							return 2;

						default:
					 		break;
					}
				}
			}

			privato_lavoro.lavoro.ucPosition=atoi(iml[0].pc);
			privato_lavoro.lavoro.uiNumeroPezzi=atoi(iml[1].pc);
			privato_lavoro.lavoro.fOhm=atof(iml[2].pc);
			mystrcpy(privato_lavoro.lavoro.ucDescrizione,iml[3].pc,sizeof(privato_lavoro.lavoro.ucDescrizione)-1);



			//strcpy(hw.ucString,"Modifica lavoro");
			vStringLangCopy(hw.ucString,enumStr20_Modifica_Lavoro);
			//ucPrintStaticButton(hw.ucString,defML_Codice_rowTitle,defML_Codice_colTitle,enumFontSmall,enumML_Title,defLCD_Color_LightGreen);
			ucPrintTitleButton(hw.ucString,defML_Codice_rowTitle,defML_Codice_colTitle,enumFontMedium,enumML_Title,defLCD_Color_Trasparente,1);

			//strcpy(hw.ucString,"Numero pezzi");
			sprintf(hw.ucString,"%-25.25s",pucStringLang(enumStr20_Numero_pezzi));
			sprintf(hw.ucString,"%s%-15.15s",hw.ucString,pucStringLang(enumStr20_Ohm));
			vStrUpperCase(hw.ucString);
			ucPrintSmallText_ColNames(hw.ucString,defML_Codice_rowNumPezzi-14,0);
			sprintf(hw.ucString," %-7i",privato_lavoro.lavoro.uiNumeroPezzi);

			// non ha senso specificare accuratezza migliore dell'1 per diecimila, per cui
			// se raggiungo i 10000 ohm, non devo avere decimali
			if (privato_lavoro.lavoro.fOhm>=10000)
				privato_lavoro.lavoro.fOhm=(int)privato_lavoro.lavoro.fOhm;

			ucPrintStaticButton(hw.ucString,defML_Codice_rowNumPezzi,defML_Codice_colNumPezzi,enumFontMedium,enumML_numpezzi,defLCD_Color_Trasparente);
			if (privato_lavoro.lavoro.fOhm>=10000){
				sprintf(hw.ucString,"%-7.1i",(int)privato_lavoro.lavoro.fOhm);
			}
			else{
				sprintf(hw.ucString,"%-7.2f",privato_lavoro.lavoro.fOhm);
			}

			ucPrintStaticButton(hw.ucString,defML_Codice_rowOhm,defML_Codice_colOhm,enumFontMedium,enumML_ohm,defLCD_Color_Trasparente);

			// modo di funzionamento
			//sprintf(hw.ucString,"%-40.40s",pucStringLang(enumStr20_Sel_Funzionamento));
			sprintf(hw.ucString,"%-40.40s",pucStringLang(enumStr20_Mode));
			
			vStrUpperCase(hw.ucString);
			ucPrintSmallText_ColNames(hw.ucString,defML_Codice_rowModoFunz-14,0);
			if (pJobsSelected_Jobs->ucWorkingMode_0L_1LR){
				//vStringLangCopy(hw.ucString,enumStr20_With_ohm);
				sprintf(hw.ucString," %-4.4s",pucStringLang(enumStr20_LR_Mode));
				
			}
			else{
				//vStringLangCopy(hw.ucString,enumStr20_With_length);
				sprintf(hw.ucString," %-4.4s",pucStringLang(enumStr20_L_Mode));
			}
			ucPrintStaticButton(hw.ucString,defML_Codice_rowModoFunz,defML_Codice_colModoFunz,enumFontMedium,enumML_Mode,defLCD_Color_Trasparente);

			// abilitata compensazione temperatura???
			if (macroIsStringaCodiceProdotto_TemperatureCompensated(his.ucCodice)){
				sprintf(hw.ucString,"%-5.5s",pucStringLang(enumStr20_Temperature_coefficient_short));
				ucPrintStaticButton(hw.ucString,defML_Codice_rowModoFunz,defML_Codice_colModoFunz+7*16,enumFontMedium,enumML_CompensazioneTemperatura,defLCD_Color_Trasparente);
			}
			// richiesto di modificare i parametri distanziatore...?
			if (macroIsStringaCodiceProdotto_Distanziatore(his.ucCodice)){
				sprintf(hw.ucString,"%-5.5s",pucStringLang(enumStr20_PresenzaDistanziatore));
				ucPrintStaticButton(hw.ucString,defML_Codice_rowModoFunz,defML_Codice_colModoFunz+14*16+4,enumFontMedium,enumML_Distanziatore,defLCD_Color_Trasparente);
			}

			//strcpy(hw.ucString,"Lotto");
			sprintf(hw.ucString,"%-40.40s",pucStringLang(enumStr20_Lotto));
			vStrUpperCase(hw.ucString);
			ucPrintSmallText_ColNames(hw.ucString,defML_Codice_rowDesc-14,0);
			mystrcpy(hw.ucString,privato_lavoro.lavoro.ucDescrizione,sizeof(hw.ucString)-1);
			ucPrintStaticButton(hw.ucString,defML_Codice_rowDesc,defML_Codice_colDesc,enumFontMedium,enumML_descrizione,defLCD_Color_Trasparente);

			//strcpy(hw.ucString,"Posizione");
			sprintf(hw.ucString,"%-40.40s",pucStringLang(enumStr20_Posizione));
			vStrUpperCase(hw.ucString);
			ucPrintSmallText_ColNames(hw.ucString,defML_Codice_rowPos-14,0);
			sprintf(hw.ucString," %-7i",privato_lavoro.lavoro.ucPosition);
			ucPrintStaticButton(hw.ucString,defML_Codice_rowPos,defML_Codice_colPos,enumFontMedium,enumML_posizione,defLCD_Color_Trasparente);


			// se sto lavorando su un record "valido", visualizzo il pulsante DEL
			if ( (pJobsSelected_Jobs->lista[privato_lavoro.lavoro.ucPosition-1].ucValidKey==defLavoroValidoKey)
				// la posizione non deve essere stata modificata!!!! 1.33
				// altrimenti facendo cancella dopo aver  modificato la posizone succedono cose incredibili nella lista
				&&(privato_lavoro.lavoro.ucPosition==defGetWinParam(defWinParamNumLavoro)+1)
				){
				//strcpy(hw.ucString,"Cancella");
				vStringLangCopy(hw.ucString,enumStr20_Del);
				ucPrintStaticButton(hw.ucString,defML_Codice_rowPos,defLcdWidthX_pixel-136,enumFontMedium,enumML_Del,defLCD_Color_Trasparente);
			}
			#if 0
				strcpy(hw.ucString,">>");
				ucPrintStaticButton(hw.ucString,defML_Codice_rowOk,defML_OK_column,enumFontBig,enumML_Ok,defLCD_Color_Trasparente);

				//strcpy(hw.ucString,"Esc");
				vStringLangCopy(hw.ucString,enumStr20_Esc);
				ucPrintStaticButton(hw.ucString,defML_Codice_rowEsc,250-32,enumFontBig,enumML_Esc,defLCD_Color_Trasparente);
			#endif

			// routine che permette di aggiungere ad una finestra i bottoni standard sx, dx, up, dn, esc, cut
			vAddStandardButtonsOk(enumML_Sx);

			return 1;
	}
}//modifica lista lavori




// per inserimento ohm/m filo
code TipoStructInfoFieldCodice_Lingua im_ohmm_fili[3]={
	{0,defMaxCharOhmMFilo, "NNNNNN","%6.3f",MIN_RESSPEC_FILO,MAX_RESSPEC_FILO,NULL,    1,1,enumStr20_Ohmm_filo1,enumUm_ohm_per_metro,"NNNNNNN","%7.3f"},
	{0,defMaxCharOhmMFilo, "NNNNNN","%6.3f",MIN_RESSPEC_FILO,MAX_RESSPEC_FILO,NULL,    1,1,enumStr20_Ohmm_filo2,enumUm_ohm_per_metro,"NNNNNNN","%7.3f"},
	{0,defMaxCharOhmMFilo, "NNNNNN","%6.3f",MIN_RESSPEC_FILO,MAX_RESSPEC_FILO,NULL,    1,1,enumStr20_Ohmm_filo3,enumUm_ohm_per_metro,"NNNNNNN","%7.3f"},
};


// inserimento dei valori ohm/metro dei fili...
unsigned char ucHW_inserisciOhmMfili(void){
	#define defOMF_title_row 8
	#define defOMF_title_col 48

	#define defOMF_ohmm_filo_row 64
	#define defOMF_ohmm_filo_col 0

	#define defOMF_OK_row (defLcdWidthY_pixel-32-8)
	#define defOMF_OK_col (defLcdWidthX_pixel-32*2-8)


	#define defOMF_Esc_row defOMF_OK_row
	#define defOMF_Esc_col (defOMF_OK_col-32*2-8)

	#define defOMF_OffsetBetweenRows 40

	#define defWinParam_OMF_NumCampo 0
	// pulsanti della finestra
	typedef enum {
			enumOMF_ohmm_filo_1=0,
			enumOMF_ohmm_filo_2,
			enumOMF_ohmm_filo_3,
			enumOMF_Title,
			enumOMF_Home,

			enumOMF_ohmm__titolo_filo_1,
			enumOMF_ohmm__titolo_filo_2,
			enumOMF_ohmm__titolo_filo_3,

			enumOMF_ohmm_WireLengthWorkingJob,
			enumOMF_ohmm_WireLengthAllOrders,

			enumOMF_Sx,
			enumOMF_Up,
			enumOMF_Cut,
			enumOMF_Esc,
			enumOMF_Dn,
			enumOMF_Dx,


			enumOMF_NumOfButtons
		}enumButtons_inserisciOhmMfili;
	xdata unsigned char i,j;
	extern unsigned char ucCalcWireLengthMetersForJobList(xdata float *pfWireLengthMeters);
	xdata float fWireLengthMeters[2];
	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			nvram_struct.resspec[defGetWinParam(defWinParam_OMF_NumCampo)]=atof(hw.ucStringNumKeypad_out);
			if (nvram_struct.actUM==UM_IN){
				nvram_struct.resspec[defGetWinParam(defWinParam_OMF_NumCampo)]*=metri2feet;
			}
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enumOMF_NumOfButtons;i++)
				ucCreateTheButton(i); 
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enumOMF_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enumOMF_Title:
						case enumOMF_Home:
						case enumOMF_Esc:
							// da qui devo entrare nel menu di impostazione ohm/metro fili, e solo dopo devo entrare nel menu distanziatore
							if (uc_after_ohm_m_fili_jump_to_enumWinId_Distanziatore==0xa5){
								uc_after_ohm_m_fili_jump_to_enumWinId_Distanziatore=0;
								vJumpToWindow(enumWinId_Distanziatore);
							}
							else
								vHandleEscFromMenu();
							// indico di rinfrescare la finestra
							return 2;
						case enumOMF_Cut:
							/* Indico di eseguire un taglio asincrono. */
							vDoTaglio();
							// non occorre rinfrescare la finestra
							break;

						case enumOMF_Sx:
							if (uc_after_ohm_m_fili_jump_to_enumWinId_Distanziatore==0xa5){
								uc_after_ohm_m_fili_jump_to_enumWinId_Distanziatore=0;
								vJumpToWindow(enumWinId_Distanziatore);
							}
							else if (nvram_struct.ucComingToControlloProduzioneFromJobs==0)
								vJumpToWindow(enumWinId_LottoDiretto);
							else
								vJumpToWindow(enumWinId_ListaLavori);						
							// indico di rinfrescare la finestra
							return 2;
						case enumOMF_Dx:
							if (uc_after_ohm_m_fili_jump_to_enumWinId_Distanziatore==0xa5){
								uc_after_ohm_m_fili_jump_to_enumWinId_Distanziatore=0;
								vJumpToWindow(enumWinId_Distanziatore);
							}
							else
								vJumpToWindow(enumWinId_setupProduzione);
							// indico di rinfrescare la finestra
							return 2;
						case enumOMF_ohmm_filo_1:
						case enumOMF_ohmm_filo_2:
						case enumOMF_ohmm_filo_3:
							j=i-enumOMF_ohmm_filo_1;
							paramsNumK.ucMaxNumChar=im_ohmm_fili[j].ucLenField;	
							// imposto stringa di picture
							mystrcpy((char*)paramsNumK.ucPicture,(char*)im_ohmm_fili[j].ucPictField,sizeof(paramsNumK.ucPicture)-1);
							paramsNumK.fMin=im_ohmm_fili[j].fMin;
							paramsNumK.fMax=im_ohmm_fili[j].fMax;
							paramsNumK.enumUm=im_ohmm_fili[j].enumUm;

							// imposto limiti min e max
							if (nvram_struct.actUM==UM_IN){
                            // here the mm_to_inch constant was used, instead the meters to feet should be used instead
                            // but anyway we prefer to use two different limits for the low and high limits of the specific resistance
								paramsNumK.fMin=MIN_RESSPEC_FILO_OHM_FEET;
								paramsNumK.fMax=MAX_RESSPEC_FILO_OHM_FEET;
							}
							//strcpy(paramsNumK.ucTitle,im_ohmm_fili[j].stringa);
							vStringLangCopy(paramsNumK.ucTitle,im_ohmm_fili[i].stringa);
							// copio la stringa con cui si inizializza il numeric keypad
							if (nvram_struct.actUM==UM_IN){
								sprintf(hw.ucStringNumKeypad_in,im_ohmm_fili[j].ucFmtFieldInch,nvram_struct.resspec[j]*feet2metri);
							}
							else{
								sprintf(hw.ucStringNumKeypad_in,im_ohmm_fili[j].ucFmtField,nvram_struct.resspec[j]);
							}
							// imposto abilitazione uso stringa picture
							paramsNumK.ucPictureEnable=im_ohmm_fili[j].ucPictureEnable;	// abilitazione stringa picture
							// imposto abilitazione uso min/max
							paramsNumK.ucMinMaxEnable=im_ohmm_fili[j].ucMinMaxEnable;	// abilitazione controllo valore minimo/massimo
							// indico qual � il campo che sto modificando
							defSetWinParam(defWinParam_OMF_NumCampo,i);
							// chiamo il numeric keypad 
							ucCallWindow(enumWinId_Num_Keypad);
							// indico di rinfrescare la finestra
							return 2;
					}
				}
			}
			//strcpy(hw.ucString,"Res. spec. fili");
			vStringLangCopy(hw.ucString,enumStr20_Res_Spec_Fili);
            if (nvram_struct.actUM==UM_IN)
            {
            // if measure unit is inch and the default string is not correct, force the ft instead of meter
                if (strncmp(hw.ucString,"OHM/MT",5)==0)
                {
                    hw.ucString[4]='F';
                }
            }
			//ucPrintStaticButton(hw.ucString,defOMF_title_row,defOMF_title_col,enumFontMedium,enumOMF_Title,defLCD_Color_LightGreen);
			ucPrintTitleButton(hw.ucString,defOMF_title_row,defOMF_title_col,enumFontMedium,enumOMF_Title,defLCD_Color_Trasparente,1);
			for (i=0;(i<MAX_NUM_FILI)&&(i<atoi(his.ucNumFili));i++){
				sprintf(hw.ucString,"%s %1i:",pucStringLang(enumStr20_Filo),i+1);
				j=strlen(hw.ucString);
				ucPrintTitleButton(hw.ucString,defOMF_ohmm_filo_row+i*defOMF_OffsetBetweenRows,defOMF_ohmm_filo_col,enumFontMedium,enumOMF_ohmm__titolo_filo_1+i,defLCD_Color_Trasparente,0);

				if (nvram_struct.resspec[i]<MIN_RESSPEC_FILO)
					nvram_struct.resspec[i]=MIN_RESSPEC_FILO;
				else if (nvram_struct.resspec[i]>MAX_RESSPEC_FILO)
					nvram_struct.resspec[i]=MAX_RESSPEC_FILO;
				if (nvram_struct.actUM==UM_IN)
					sprintf(hw.ucString,"%6.3f",nvram_struct.resspec[i]*feet2metri);
				else
					sprintf(hw.ucString,"%6.3f",nvram_struct.resspec[i]);
				ucPrintStaticButton(hw.ucString,defOMF_ohmm_filo_row+i*defOMF_OffsetBetweenRows,defOMF_ohmm_filo_col+j*16+16,enumFontMedium,enumOMF_ohmm_filo_1+i,defLCD_Color_Trasparente);
				if (nvram_struct.actUM==UM_IN)
					sprintf(hw.ucString,"%s",pucStringLang(enumStr20_FirstUm+enumUm_ohm_per_feet));
				else
					sprintf(hw.ucString,"%s",pucStringLang(enumStr20_FirstUm+enumUm_ohm_per_metro));
				//ucPrintSmallText_ColNames(hw.ucString,defOMF_ohmm_filo_row+i*defOMF_OffsetBetweenRows-12,defOMF_ohmm_filo_col);
				ucPrintTitleButton(hw.ucString,defOMF_ohmm_filo_row+i*defOMF_OffsetBetweenRows+4,defOMF_ohmm_filo_col+j*16+16+6*16+12,enumFontSmall,enumOMF_ohmm__titolo_filo_1+i,defLCD_Color_Trasparente,0);
			}

			if (ucCalcWireLengthMetersForJobList(&fWireLengthMeters[0])){
				vString40LangCopy(hw.ucString,enumStr40_WireLengthWorkingJob);
				if (nvram_struct.actUM==UM_IN){
					sprintf(hw.ucString+strlen(hw.ucString),":%6.1fft",fWireLengthMeters[0]*metri2feet);
				}
				else{
					sprintf(hw.ucString+strlen(hw.ucString),":%6.1fmt",fWireLengthMeters[0]);
				}
				ucPrintTitleButton(hw.ucString,defOMF_ohmm_filo_row+(i-1)*defOMF_OffsetBetweenRows+4+14*0+36,10,enumFontSmall,enumOMF_ohmm_WireLengthWorkingJob,defLCD_Color_Trasparente,1);

				vString40LangCopy(hw.ucString,enumStr40_WireLengthAllOrders);
				if (nvram_struct.actUM==UM_IN){
					sprintf(hw.ucString+strlen(hw.ucString),":%6.1fft",fWireLengthMeters[1]*metri2feet);
				}
				else{
					sprintf(hw.ucString+strlen(hw.ucString),":%6.1fmt",fWireLengthMeters[1]);
				}
				ucPrintTitleButton(hw.ucString,defOMF_ohmm_filo_row+(i-1)*defOMF_OffsetBetweenRows+4+18*1+36,10,enumFontSmall,enumOMF_ohmm_WireLengthAllOrders,defLCD_Color_Trasparente,1);
			}

#if 0
			strcpy(hw.ucString,"<<");
			ucPrintStaticButton(hw.ucString,defOMF_Esc_row,defOMF_Esc_col,enumFontBig,enumOMF_Esc,defLCD_Color_Trasparente);
			//strcpy(hw.ucString,"Home");
			//ucPrintStaticButton(hw.ucString,defOMF_HOME_row,defOMF_HOME_col,enumFontBig,enumOMF_Home,defLCD_Color_Trasparente);
			strcpy(hw.ucString,">>");
			ucPrintStaticButton(hw.ucString,defOMF_OK_row,defOMF_OK_col,enumFontBig,enumOMF_OK,defLCD_Color_Trasparente);
#endif

			vAddStandardButtonsOk(enumOMF_Sx);


			return 1;
	}
}//unsigned char ucHW_inserisciOhmMfili(void)


// calcola tempo residuo a finire commessa 
void vCalcTempoResiduo(void){
	if (nvram_struct.commesse[spiralatrice.runningCommessa].n+nvram_struct.commesse[spiralatrice.runningCommessa].magg==0)
		return;
	if ((nvram_struct.commesse[spiralatrice.runningCommessa].ndo==0)||(!PrgRunning)){
		TPR.uiOreResidue=0;
		TPR.uiMinutiResidui=0;
		TPR.uiLastNdo=0xFFFF;
		return;
	}
	TPR.uiLastNdo=nvram_struct.commesse[spiralatrice.runningCommessa].ndo;
	TPR.ucLastCntInterrupt=ucCntInterrupt;
	TPR.ulTempoPerResistenzaCorrente_Main=TPR.ulTempoPerResistenzaCorrente;
	TPR.ulTempoPerResistenzaUltima_Main=TPR.ulTempoPerResistenzaUltima;
	// se intervenuta interrupt, ritento
	if (TPR.ucLastCntInterrupt!=ucCntInterrupt){
		TPR.ulTempoPerResistenzaCorrente_Main=TPR.ulTempoPerResistenzaCorrente;
		TPR.ulTempoPerResistenzaUltima_Main=TPR.ulTempoPerResistenzaUltima;
	}
		
	TPR.ulAux=nvram_struct.commesse[spiralatrice.runningCommessa].n+nvram_struct.commesse[spiralatrice.runningCommessa].magg;
	if (TPR.ulAux>=nvram_struct.commesse[spiralatrice.runningCommessa].ndo)
		TPR.ulAux-=nvram_struct.commesse[spiralatrice.runningCommessa].ndo;
	else
		TPR.ulAux=0;

	// devo aver fatto almeno una resistenza per visualizzare un valore valido del tempo stimato...
	if (TPR.uiNumPezziFatti<1){
		TPR.uiOreResidue=0;
		TPR.uiMinutiResidui=0;
	}
	else{
		TPR.ulSecondiResidui=((TPR.ulAux*TPR.ulTempoPerResistenzaUltima_Main-TPR.ulTempoPerResistenzaCorrente_Main)*PeriodoIntMs)/1000;
		TPR.uiOreResidue=TPR.ulSecondiResidui/3600;
		TPR.uiMinutiResidui=(TPR.ulSecondiResidui-TPR.uiOreResidue*3600+31)/60;
	}
	if (TPR.uiOreResidue>99)
		TPR.uiOreResidue=99;
	if (TPR.uiMinutiResidui>59)
		TPR.uiMinutiResidui=59;
}


#if 1

typedef enum{
	enum_campi_LD_numpezzi=0,
	enum_campi_LD_ohm,
	enum_campi_LD_descrizione,
	enum_campi_LD_diametrofilo,
	enum_campi_LD_diametromandrino,
	enum_campi_LD_numfili,
	enum_campi_LD_NumOf
}enum_campi_lottodiretto;

// per inserimento rapido lotto
code TipoStructInfoFieldCodice_Lingua im_lottorapido[6]={
	{0,defMaxCharLavoroString_Numpezzi, "nnnn","%-4i",1,MAX_QUANTITA,&privato_lavoro.ucStringNumpezzi[0],    1,1,enumStr20_Numero_pezzi,enumUm_num},
	{0,defMaxCharLavoroString_Ohm, "NNNNNN","%-6.2f",MIN_RES_CAN_MAKE,MAX_RES_CAN_MAKE,&privato_lavoro.ucStringOhm[0],    1,1,enumStr20_Valore_ohmico,enumUm_ohm},
	{0,defMaxCharDescrizioneLavoro, "xxxxxxxxxxxxxxxxxxxx","%-20s",0.1,999,&privato_lavoro.ucStringDescrizione[0],    1,0,enumStr20_Descrizione,enumUm_none},
	{0,5, "NNNNN","%-5.3f",MIN_DIAM_FILO,MAX_DIAM_FILO,&his.ucDiametroFilo[0],    1,1,enumStr20_diametro_filo,enumUm_mm,".nnnn","%-5.4f"},
//	{6,4, "xxxx", "%4s",   0,            0,            &his.ucTipoFilo[0],        0,0,enumStr20_tipo_filo,enumUm_none},
	{11,4,"NNNN", "%-4.2f",MIN_DIAM_MD,  MAX_DIAM_MD,  &his.ucDiametroMandrino[0],1,1,enumStr20_diametro_mandrino,enumUm_mm,".nnn","%-4.3f"},
	{16,2,"nn",   "%-2i",   MIN_NUM_FILI, MAX_NUM_FILI, &his.ucNumFili[0],         1,1,enumStr20_numero_di_fili,enumUm_num},
//	{18,2,"nn",   "%2s",   MIN_FORNI,    MAX_FORNI,    &his.ucFornitore[0],       1,1,enumStr20_tolleranza,enumUm_num}
};


// inserimento di un lotto direttamente...
unsigned char ucHW_inserisciLottoDiretto(void){
	#define defLD_title_row 6
	#define defLD_title_col 0

	#define defLD_NumPezzi_row 48
	#define defLD_NumPezzi_col 0

	#define defOffsetBetweenLines 44

	#define defLD_Ohm_row defLD_NumPezzi_row
	#define defLD_Ohm_col (defLcdWidthX_pixel/2+16)

	#define defLD_desc_row (defLD_NumPezzi_row+defOffsetBetweenLines)
	#define defLD_desc_col 0

	#define defLD_diamfilo_row (defLD_NumPezzi_row+defOffsetBetweenLines*2)
	#define defLD_diamfilo_col 0

	#define defLD_diammand_row defLD_diamfilo_row
	#define defLD_diammand_col (defLcdWidthX_pixel/2+16)

	#define defLD_numfili_row (defLD_NumPezzi_row+defOffsetBetweenLines*3)
	#define defLD_numfili_col 0
	#define defLD_fornitore_col (defLD_numfili_col+24+12+16)
	#define defLD_modo_col  (defLD_fornitore_col+24+24)

	#define defLD_next_row (defLcdWidthY_pixel-32-8)
	#define defLD_next_col (defLcdWidthX_pixel-32*2-8)



	#define defLD_OffsetBetweenRows 40

	#define defWinParam_LD_NumCampo 0
	#define defWinParam_LD_TipoCall 1

	#define defWinParam_LD_CallParam 0
	#define defWinParam_LD_CallYesNo 1
	#define defWinParam_LD_CallFornitori 2

	// pulsanti della finestra
	typedef enum {
			enumLD_numPezzi=0,
			enumLD_OhmPezzo,
			enumLD_Descrizione,
			enumLD_diamFilo,
			enumLD_diamMandrino,
			enumLD_numFili,
			enumLD_OhmM_filo_1,
			enumLD_OhmM_filo_2,
			enumLD_OhmM_filo_3,
			enumLD_Tolleranza,
			enumLD_Modo,
			enumLD_Title,

			enumLD_Sx,
			enumLD_Up,
			enumLD_Cut,
			enumLD_Esc,
			enumLD_Dn,
			enumLD_Dx,


			enumLD_NumOfButtons
		}enumButtons_lottoDiretto;
	xdata unsigned char i,j,ucSaveWorkingMode;
	static unsigned char ucCodiceIngresso_Manuale[defCodice_MaxChar+1];
	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			if (defGetWinParam(defWinParam_LD_TipoCall)==defWinParam_LD_CallFornitori){
				// il risultato della scelta ce l'ho nel parametro defIdxParam_enumWinId_Fornitori_Choice
				// della finestra enumWinId_Fornitori
				i=uiGetWindowParam(enumWinId_Fornitori,defIdxParam_enumWinId_Fornitori_Choice);
				// se vale 0xFF, non � stato cambiato il valore!!
				if (i!=0xFF){
					if (i<0)
						i=0;
					else if(i>9)
						i=9;
					his.ucFornitore[0]=i+'0';
					his.ucFornitore[1]=0;
				}
			}
			else if (defGetWinParam(defWinParam_LD_TipoCall)==defWinParam_LD_CallYesNo){
			}
			else{
				// copio il valore inserito tramite numeric keypad nel campo di pertinenza
				// l'indice del parametro � stato salvato in Win_CP_field
				sprintf(im_lottorapido[defGetWinParam(defWinParam_LD_NumCampo)].pc,"%-*s",paramsNumK.ucMaxNumChar,hw.ucStringNumKeypad_out);
				// metto il tappo per sicurezza...
				im_lottorapido[defGetWinParam(defWinParam_LD_NumCampo)].pc[paramsNumK.ucMaxNumChar]=0;
			}
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enumLD_NumOfButtons;i++)
				ucCreateTheButton(i); 
			// trasferisco dati da commessa a lista lavori
			vTrasferisciDaCommesseAlistaLavori();
			// prelevo il codice manuale se esiste, altrimenti il primo della lista correntemente selezionata
			memcpy(&privato_lavoro.lavoro,pFindManualCodeInJobList(),sizeof(privato_lavoro.lavoro));
			// if we are here coming from job mode, better to reset the number of pieces done
			// but if we already are not in job mode, i.e. we already are in manual mode, we do not reset the number of pieces done
			if (nvram_struct.ucComingToControlloProduzioneFromJobs)
			{
	            // azzero il numero di pezzi fatti!
	            privato_lavoro.lavoro.npezzifatti=0;
			}
			//sprintf(iml[0].pc,iml[0].ucFmtField,privato_lavoro.lavoro.ucPosition);
			sprintf(im_lottorapido[0].pc,im_lottorapido[0].ucFmtField,privato_lavoro.lavoro.uiNumeroPezzi);
			sprintf(im_lottorapido[1].pc,im_lottorapido[1].ucFmtField,privato_lavoro.lavoro.fOhm);
			sprintf(im_lottorapido[2].pc,im_lottorapido[2].ucFmtField,privato_lavoro.lavoro.ucDescrizione);
			// se numero fili = 0, recupera ultimo codice inserito manualmente
			// se numero fili = 0, imposta ad 1
			if (atoi(im_lottorapido[5].pc) == 0)
			{

				sprintf(im_lottorapido[5].pc, im_lottorapido[5].ucFmtField, 1);
			}
			// se fornitore non impostato, assegna 0
			if (!isdigit(his.ucFornitore[0]))
			{
				his.ucFornitore[0] = '0';
				his.ucFornitore[1] = 0x00;
			}
			nvram_struct.ucComingToControlloProduzioneFromJobs=0;
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			mystrcpy(ucCodiceIngresso_Manuale,his.ucCodice,sizeof(ucCodiceIngresso_Manuale)-1);
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enumLD_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enumLD_Sx:
							// vado al menu lista pJobsSelected_Jobs->..
							vJumpToWindow(enumWinId_ListaLavori);
							return 2;
						case enumLD_Esc:
							// chiamo il main menu...
							vJumpToWindow(enumWinId_MainMenu);
							// indico di rinfrescare la finestra
							return 2;
						case enumLD_Cut:
							/* Indico di eseguire un taglio asincrono. */
							vDoTaglio();
							// non occorre rinfrescare la finestra
							break;
						case enumLD_Modo:
							if (macParms.modo == MODE_LENR) {
								if (pJobsSelected_Jobs->ucWorkingMode_0L_1LR)
									pJobsSelected_Jobs->ucWorkingMode_0L_1LR=0;
								else
									pJobsSelected_Jobs->ucWorkingMode_0L_1LR=1;
							}
							else{
								pJobsSelected_Jobs->ucWorkingMode_0L_1LR=0;
							}						
							return 2;

						case enumLD_Tolleranza:
							defSetWinParam(defWinParam_LD_TipoCall,defWinParam_LD_CallFornitori);
							// indico che la tabella NON pu� essere modificata
							ucSetWindowParam(enumWinId_Fornitori,defIdxParam_enumWinId_Fornitori_ModifyEnable,0);
							// chiamo la finestra fornitori
							ucCallWindow(enumWinId_Fornitori);
							return 2;
						case enumLD_OhmM_filo_1:
						case enumLD_OhmM_filo_2:
						case enumLD_OhmM_filo_3:
						case enumLD_Dx:
							if (privato_lavoro.lavoro.ucValidKey!=defLavoroValidoKey){
								break;
							}

							// chiamo la procedura che si incarica di inserireil codice prodotto in lista, se non esiste
							vVerifyInsertCodiceProdotto(his.ucCodice);


							// salvo il codice attualmente impostato
							mystrcpy(ucSaveCodice_his,his.ucCodice,sizeof(ucSaveCodice_his)-1);
							// salvo il modo di lavoro selezionato
							ucSaveWorkingMode=pJobsSelected_Jobs->ucWorkingMode_0L_1LR;
							// vuoto dalla joblist tutti i i codici manuali
							ucPurgeManualCodesInJobList();
							// inserisco il codice nella joblist.. se non ci sta cosa faccio????
							if (ucInsertCodeInJobList(ucSaveCodice_his,1)
									// pulisco anche tutti i codici che non hanno jobs
								||	( ucPurgeJobList()	&& ucInsertCodeInJobList(ucSaveCodice_his,1))
								){
								// se codice variato, azzero la maggiorazione!!!
								if (strcmp(ucCodiceIngresso_Manuale,his.ucCodice)){
									privato_lavoro.lavoro.magg=0;
								}

								// salvo il lavoro modificato/inserito...
								memcpy(&pJobsSelected_Jobs->lista[0],&privato_lavoro.lavoro,sizeof(privato_lavoro.lavoro));
								// indico che c'� un solo lavoro in lista
								pJobsSelected_Jobs->lista[1].ucValidKey=defLavoroNonValidoKey;
								vCodelistTouched();

								// un solo lavoro in lista...
								pJobsSelected_Jobs->ucNumLavoriInLista=1;
								// reimposto il modo di lavoro...
								pJobsSelected_Jobs->ucWorkingMode_0L_1LR=ucSaveWorkingMode;

								// trasferisco da lista lavori a nvram_struct.commesse...
								vTrasferisciDaListaLavoriAcommesse();
								// indico che si arriva a finestra controllo produzione da lavorazione manuale
								if (i==enumLD_Dx){
									vJumpToWindow(enumWinId_setupProduzione);
								}
								else
									vJumpToWindow(enumWinId_inserisciOhmMfili);
							}
							else{
								vRefreshActiveProductCode();
								vStringLangCopy(hw.ucStringNumKeypad_in,enumStr20_JobListFull);
								hw.ucStringNumKeypad_out[0]=0;
								// indico che sto chiamando la yes/no
								defSetWinParam(defWinParam_LD_TipoCall,defWinParam_LD_CallYesNo);
								// impostazione del parametro 0 al valore 0--> visualizza pulsanti yes e no
								ucSetWindowParam(enumWinId_YesNo,def_WinYesNo_Param_TipoPulsante, def_WinYesNo_PUlsanteOk);
								// visualizzo ok...
								ucCallWindow(enumWinId_YesNo);
							}
							return 2;

						case enumLD_numPezzi:
						case enumLD_OhmPezzo:
						case enumLD_Descrizione:
						case enumLD_numFili:
						case enumLD_diamFilo:
						case enumLD_diamMandrino:
							j=i-enumLD_numPezzi;
							paramsNumK.ucMaxNumChar=im_lottorapido[j].ucLenField;	
							// imposto stringa di picture
							mystrcpy((char*)paramsNumK.ucPicture,(char*)im_lottorapido[j].ucPictField,sizeof(paramsNumK.ucPicture)-1);
							paramsNumK.fMin=im_lottorapido[j].fMin;
							paramsNumK.fMax=im_lottorapido[j].fMax;
							paramsNumK.enumUm=im_lottorapido[j].enumUm;
							// imposto limiti min e max nel caso in cui l'unit� di misura attuale sia inch
							if ((nvram_struct.actUM==UM_IN)&&(im_lottorapido[j].enumUm==enumUm_mm)){
								paramsNumK.fMin*=MM_TO_INCH;
								paramsNumK.fMax*=MM_TO_INCH;
                                paramsNumK.enumUm=enumUm_inch;
							}
							// nella descrizione posso inserire caratteri alfanumerici
							if (i==	enumLD_Descrizione)					
								paramsNumK.ucEnableT9=1;
							else
								paramsNumK.ucEnableT9=0;

							//strcpy(paramsNumK.ucTitle,im_ohmm_fili[j].stringa);
							// titolo del numeric keypad
							vStringLangCopy(paramsNumK.ucTitle,im_lottorapido[i].stringa);
							// copio la stringa con cui si inizializza il numeric keyupad
							mystrcpy(hw.ucStringNumKeypad_in,im_lottorapido[j].pc,sizeof(hw.ucStringNumKeypad_in)-1);
							// imposto abilitazione uso stringa picture
							paramsNumK.ucPictureEnable=im_lottorapido[j].ucPictureEnable;	// abilitazione stringa picture
							// imposto abilitazione uso min/max
							paramsNumK.ucMinMaxEnable=im_lottorapido[j].ucMinMaxEnable;	// abilitazione controllo valore minimo/massimo
							// indico qual � il campo che sto modificando
							defSetWinParam(defWinParam_LD_NumCampo,i);
							// indico che sto modificando un parametro
							defSetWinParam(defWinParam_LD_TipoCall,defWinParam_LD_CallParam);
							// chiamo il numeric keypad 
							ucCallWindow(enumWinId_Num_Keypad);
							// indico di rinfrescare la finestra
							return 2;
					}
				}
			}

			// riformatto diametro filo e diametro mandrino
			his.fAux=atof(his.ucDiametroFilo);
			if (nvram_struct.actUM==UM_IN)
			{
				if (his.fAux < MIN_DIAM_FILO * MM_TO_INCH)
				{
					his.fAux = MIN_DIAM_FILO * MM_TO_INCH;
				}
				sprintf(his.ucDiametroFilo,im_lottorapido[enum_campi_LD_diametrofilo].ucFmtFieldInch,his.fAux);
				// elimino lo zero iniziale, non trovo al momento altro modo!!!!!
				if (his.ucDiametroFilo[0]&&(his.ucDiametroFilo[0]!='.')){
					strcpy(his.ucDiametroFilo,his.ucDiametroFilo+1);
				}
			}
			else
			{
				if (his.fAux < MIN_DIAM_FILO)
				{
					his.fAux = MIN_DIAM_FILO;
				}
				sprintf(his.ucDiametroFilo,im_lottorapido[enum_campi_LD_diametrofilo].ucFmtField,his.fAux);
			}
			his.ucDiametroFilo[defCodice_MaxCharDiametroFilo]=0;

			his.fAux=atof(his.ucDiametroMandrino);
			// voglio evitare overflow... siccome la precisione decimale non viene mai sacrificata dal C
			// se viene inserito 20.0 la sprintf la fa diventare 20.00 e posso corrompere la mia stringa...
			if (his.fAux>=10){
				snprintf(his.ucDiametroMandrino, sizeof(his.ucDiametroMandrino), "%-4.1f",his.fAux);
			}
			else{
				if (nvram_struct.actUM==UM_IN){
					if (his.fAux < MIN_DIAM_MD * MM_TO_INCH)
					{
						his.fAux = MIN_DIAM_MD * MM_TO_INCH;
					}
					sprintf(his.ucDiametroMandrino,im_lottorapido[enum_campi_LD_diametromandrino].ucFmtFieldInch,his.fAux);
					// elimino lo zero iniziale, non trovo al momento altro modo!!!!!
					if (his.ucDiametroMandrino[0]&&(his.ucDiametroMandrino[0]!='.')){
						strcpy(his.ucDiametroMandrino,his.ucDiametroMandrino+1);
					}
				}
				else
				{
					if (his.fAux < MIN_DIAM_MD)
					{
						his.fAux = MIN_DIAM_MD;
					}

					snprintf(his.ucDiametroMandrino, sizeof(his.ucDiametroMandrino), im_lottorapido[enum_campi_LD_diametromandrino].ucFmtField,his.fAux);
				}
			}
			his.ucDiametroMandrino[defCodice_MaxCharDiametroMandrino]=0;

			// compongo il codice prodotto corrente...
			sprintf(his.ucCodice,"%s %s %s %-2s%-2s", his.ucDiametroFilo,
													his.ucTipoFilo,
													his.ucDiametroMandrino,
													his.ucNumFili,
													his.ucFornitore
					);
			// valido la struttura his appena composta...
			vValidateHis(&his);

			privato_lavoro.lavoro.uiNumeroPezzi=atoi(im_lottorapido[0].pc);
			privato_lavoro.lavoro.fOhm=atof(im_lottorapido[1].pc);
			mystrcpy(privato_lavoro.lavoro.ucDescrizione,im_lottorapido[2].pc,sizeof(privato_lavoro.lavoro.ucDescrizione)-1);
			if ( (  (privato_lavoro.lavoro.uiNumeroPezzi<=im_lottorapido[enum_campi_LD_numpezzi].fMax)
				  &&(privato_lavoro.lavoro.uiNumeroPezzi>=im_lottorapido[enum_campi_LD_numpezzi].fMin)
				 )
				&&(
				     (privato_lavoro.lavoro.fOhm<=im_lottorapido[enum_campi_LD_ohm].fMax)
				   &&(privato_lavoro.lavoro.fOhm>=im_lottorapido[enum_campi_LD_ohm].fMin)
				)
				){
				privato_lavoro.lavoro.ucValidKey=defLavoroValidoKey;
			}
			else
				privato_lavoro.lavoro.ucValidKey=defLavoroNonValidoKey;

			vStringLangCopy(hw.ucString,enumStr20_Inserisci_Lavoro);
			//ucPrintStaticButton(hw.ucString,defLD_title_row,defLD_title_col,enumFontMedium,enumLD_Title,defLCD_Color_Green);
			ucPrintTitleButton(hw.ucString,defLD_title_row,defLD_title_col,enumFontMedium,enumLD_Title,defLCD_Color_Trasparente,1);


			//strcpy(hw.ucString,"Numero pezzi");
			sprintf(hw.ucString,"%-22.22s",pucStringLang(enumStr20_Numero_pezzi));
			sprintf(hw.ucString,"%s%-18.18s",hw.ucString,pucStringLang(enumStr20_Ohm));
			vStrUpperCase(hw.ucString);
			ucPrintSmallText_ColNames(hw.ucString,defLD_NumPezzi_row-14,defLD_NumPezzi_col);

			sprintf(hw.ucString," %-7i",privato_lavoro.lavoro.uiNumeroPezzi);
			ucPrintStaticButton(hw.ucString,defLD_NumPezzi_row,defLD_NumPezzi_col,enumFontMedium,enumLD_numPezzi,defLCD_Color_Trasparente);

			//strcpy(hw.ucString,"Ohm");
			if (privato_lavoro.lavoro.fOhm>=10000)
				privato_lavoro.lavoro.fOhm=(int)privato_lavoro.lavoro.fOhm;
			if (privato_lavoro.lavoro.fOhm>=10000){
				sprintf(hw.ucString,"%-8.1i",(int)privato_lavoro.lavoro.fOhm);
			}
			else{
				sprintf(hw.ucString,"%-8.2f",privato_lavoro.lavoro.fOhm);
			}
			ucPrintStaticButton(hw.ucString,defLD_Ohm_row,defLD_Ohm_col,enumFontMedium,enumLD_OhmPezzo,defLCD_Color_Trasparente);

			//strcpy(hw.ucString,"Descrizione");
			sprintf(hw.ucString,"%-40.40s",pucStringLang(enumStr20_Descrizione));
			vStrUpperCase(hw.ucString);
			ucPrintSmallText_ColNames(hw.ucString,defLD_desc_row-14,defLD_desc_col);
			mystrcpy(hw.ucString,privato_lavoro.lavoro.ucDescrizione,sizeof(hw.ucString)-1);
			ucPrintStaticButton(hw.ucString,defLD_desc_row,defLD_desc_col,enumFontMedium,enumLD_Descrizione,defLCD_Color_Trasparente);

			sprintf(hw.ucString,"%-22.22s",pucStringLang(enumStr20_diametro_filo));
			sprintf(hw.ucString,"%s%-18.18s",hw.ucString,pucStringLang(enumStr20_diametro_mandrino));
			vStrUpperCase(hw.ucString);
			ucPrintSmallText_ColNames(hw.ucString,defLD_diamfilo_row-14,defLD_diamfilo_col);

			vStrcpy_trimBlank(hw.ucString,his.ucDiametroFilo,8);
			sprintf(hw.ucString,"%-8.8s",hw.ucString);
			ucPrintStaticButton(hw.ucString,defLD_diamfilo_row,defLD_diamfilo_col,enumFontMedium,enumLD_diamFilo,defLCD_Color_Trasparente);


			vStrcpy_trimBlank(hw.ucString,his.ucDiametroMandrino,8);
			sprintf(hw.ucString,"%-8.8s",hw.ucString);
			ucPrintStaticButton(hw.ucString,defLD_diammand_row,defLD_diammand_col,enumFontMedium,enumLD_diamMandrino,defLCD_Color_Trasparente);


			// numero di fili, tolleranza, modo
			sprintf(hw.ucString,"%-19.19s",pucStringLang(enumStr20_numero_di_fili_tolleranza_modo));
			vStrUpperCase(hw.ucString);
			ucPrintSmallText_ColNames(hw.ucString,defLD_numfili_row-14,defLD_numfili_col);
			// sprinto il numero di fili
			sprintf(hw.ucString,"%-1.1s",his.ucNumFili);
			ucPrintStaticButton(hw.ucString,defLD_numfili_row,defLD_numfili_col,enumFontMedium,enumLD_numFili,defLCD_Color_Trasparente);
			// sprinto il fornitore
			sprintf(hw.ucString,"%-1.1s",his.ucFornitore);
			ucPrintStaticButton(hw.ucString,defLD_numfili_row,defLD_fornitore_col,enumFontMedium,enumLD_Tolleranza,defLCD_Color_Trasparente);
			// sprinto il modo 
			vStringLangCopy(hw.ucString,enumStr20_L_Mode+(pJobsSelected_Jobs->ucWorkingMode_0L_1LR?1:0));
			ucPrintStaticButton(hw.ucString,defLD_numfili_row,defLD_modo_col,enumFontMedium,enumLD_Modo,defLCD_Color_Trasparente);

			for (i=0;i<atoi(his.ucNumFili);i++){
				sprintf(hw.ucString,"%-8.8s%1i",pucStringLang(enumStr20_Filo),i+1);
				vStrUpperCase(hw.ucString);
				ucPrintSmallText_ColNames(hw.ucString,defLD_numfili_row-14+i*16,defLD_modo_col+3*16+16+8);
				if (nvram_struct.actUM==UM_IN)
					sprintf(hw.ucString,"%6.3f",nvram_struct.resspec[i]*feet2metri);
				else
					sprintf(hw.ucString,"%6.3f",nvram_struct.resspec[i]);
				ucPrintStaticButton(hw.ucString,defLD_numfili_row-14+i*16,defLD_modo_col+3*16+16+8+10*8,enumFontSmall,enumLD_OhmM_filo_1+i,defLCD_Color_Trasparente);
			}
			// if not all of the parameters are valid, no OK button will be shown
			if (privato_lavoro.lavoro.ucValidKey !=defLavoroValidoKey)
			{
				vAddStandardButtons(enumLD_Sx);
			}
			else
			{
				vAddStandardButtonsOk(enumLD_Sx);
			}

			
			return 1;
	}
}//unsigned char ucHW_inserisciLottoDiretto(void)







code TipoStructInfoParametro_Lingua if_CALSENS[]={

	{0,4, "nnnn","%-4hi",0,127,(unsigned char xdata*)&macParms.ucGainDacColtello,           1,1,enumStr20_CutGain,enumPT_unsignedChar, enumUm_none,enumStringType_none,1},
	{0,5, "NNNNN","%-5.2f",-10,+10,(unsigned char xdata*)&sensoreTaglio.fGainDacColtelloVolt,           1,1,enumStr20_CutDacValue,enumPT_float, enumUm_Volt,enumStringType_none,1},
	{0,4, "nnnn","%-4i",0,4000,(unsigned char xdata*)&macParms.uiTolerancePosColtello_StepAD,  1,1,enumStr20_CutPosTolerance,enumPT_unsignedInt, enumUm_none,enumStringType_none,1},
	{0,5, "NNNNN","%-5.2f",0,15,(unsigned char xdata*)&sensoreTaglio.fPosPretaglio,  1,1,enumStr20_CutPosition,enumPT_float, enumUm_none,enumStringType_none,1},
	{0,5, "NNNNN","%-5.2f",0,5,(unsigned char xdata*)&sensoreTaglio.fSlowDacColtelloVolt,  1,1,enumStr20_CutSlowDac,enumPT_float, enumUm_Volt,enumStringType_none,1},
};

unsigned char ucHW_CalibraSensore(void){
	#define defCALSENS_NumParametroMacchina 0

	#define defCalSens_title_row 6
	#define defCalSens_title_col 0

	#define defCalSens_NumAltroParametro 0

	#define defCalSensPosition_row 48
	#define defCalSensPosition_row_offset 28
	#define defCalSensPosition_col 6

	// pulsanti della finestra
	typedef enum {
			enumCalSens_gain=0,
			enumCalSens_dac,
			enumCalSens_posTolerance_change,
			enumCalSens_posPretaglio,
			enumCalSens_slowdac_change,

			enumCalSens_posattuale,
			enumCalSens_posMinima,
			enumCalSens_posMassima,
			enumCalSens_posTolerance,
			enumCalSens_slowfast,
			enumCalSens_do_cut,
			enumCalSens_do_precut,
			enumCalSens_do_cal,
			enumCalSens_Title,
			enumCalSens_open_closed_loop,

			enumCalSens_Sx,
			enumCalSens_Up,
			enumCalSens_Cut,
			enumCalSens_Esc,
			enumCalSens_Dn,
			enumCalSens_Dx,


			enumCalSens_NumOfButtons
		}enumButtons_CalSens;
	xdata unsigned char i,ucColor;
	xdata unsigned char idxParametro;
	xdata static unsigned char ucSensorCycle_Active,ucDataDac,uc_0slow_1fast_dacColtello;
	xdata static float fColtello_Step2mm;
	xdata static unsigned int uiTimeCycleCalibrationCutSensor_Cnt_Act,
							  uiTimeCycleCalibrationCutSensor_Ms,
							  uiTimeCycleCalibrationCutSensor_Cnt;

	xdata float fAux;

/*
vAttivaOperazioneLama(enumCiclicaLama_taglia)
vAttivaOperazioneLama(enumCiclicaLama_pretaglia)
vAttivaOperazioneLama(enumCiclicaLama_scendi)
vAttivaOperazioneLama(enumCiclicaLama_lenta)
vAttivaOperazioneLama(enumCiclicaLama_veloce)


*/


	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			idxParametro=defGetWinParam(defCALSENS_NumParametroMacchina);
			// assegno la variabile solo se esiste...
			if (if_CALSENS[idxParametro].pc){
				vSetValueParametro_spiwin2(hw.ucStringNumKeypad_out,idxParametro,&if_CALSENS[0]);
			}
			switch(idxParametro){
				case enumCalSens_dac:
					sensoreTaglio.fGainDacColtelloVolt=atof(hw.ucStringNumKeypad_out);
					// comando di non controllare la posizione del coltello....
					sensoreTaglio.stato_taglio=enumOperazioneLama_Idle;
					ucDataDac=(sensoreTaglio.fGainDacColtelloVolt+10.0)*(255.0/20.0);
					ucSendCommandPosTaglioMax519_Main(ucDataDac);
					break;
				case enumCalSens_posPretaglio:
					sensoreTaglio.fPosPretaglio=atof(hw.ucStringNumKeypad_out);	
					sensoreTaglio.uiPosPretaglio=sensoreTaglio.fPosPretaglio/spiralatrice.CorsaColtello*(sensoreTaglio.uiMax_cur-sensoreTaglio.uiMin_cur)+macParms.uiMinPosColtello_StepAD;
					if (sensoreTaglio.uiPosPretaglio>=sensoreTaglio.uiMax_cur){
						sensoreTaglio.uiPosPretaglio=sensoreTaglio.uiMax_cur;
						sensoreTaglio.fPosPretaglio=((sensoreTaglio.uiPosPretaglio-macParms.uiMinPosColtello_StepAD)*spiralatrice.CorsaColtello)/(sensoreTaglio.uiMax_cur-sensoreTaglio.uiMin_cur);
					}
					break;
				case enumCalSens_slowdac_change:
					sensoreTaglio.fSlowDacColtelloVolt=atof(hw.ucStringNumKeypad_out);
					macParms.ucSlowDacColtello=(sensoreTaglio.fSlowDacColtelloVolt+10.0)*(255.0/20.0);
					if (macParms.ucSlowDacColtello>128)
						macParms.ucSlowDacColtello-=128;
					else
						macParms.ucSlowDacColtello=0;
					break;
				case enumCalSens_posTolerance_change:
					break;
				case enumCalSens_gain:
				default:
					break;


			}
			ValidateMacParms();
			ucSaveMacParms=1;
			vSetStatusCurrentWindow(enumWindowStatus_DoSomeActivity_0);
			return 2;
		case enumWindowStatus_DoSomeActivity_0:
			sensoreTaglio.fSlowDacColtelloVolt=(((float)macParms.ucSlowDacColtello+128)*(20.0/255.0)-10.0);
			sensoreTaglio.fGainDacColtelloVolt=(((signed int )ucLastCommandTaglio)*(20.0/255.0))-10.0;
			// reinizializzo le variabili di interesse...
			fColtello_Step2mm=spiralatrice.CorsaColtello/(sensoreTaglio.uiMax_cur-sensoreTaglio.uiMin_cur);
			sensoreTaglio.uiMin_cur=macParms.uiMinPosColtello_StepAD;
			sensoreTaglio.uiMax_cur=macParms.uiMaxPosColtello_StepAD;
			sensoreTaglio.iTolerance=macParms.uiTolerancePosColtello_StepAD;
			// valore massimo da dare al dac quando si � in modo slow
			sensoreTaglio.ucSaturaDac_value=macParms.ucSlowDacColtello;
			// guadagno per il controllo del coltello [0..255]
			sensoreTaglio.uiKp=macParms.ucGainDacColtello;
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enumCalSens_NumOfButtons;i++)
				ucCreateTheButton(i); 
			ucSensorCycle_Active=0;
			ucDataDac=128;
			uiTimeCycleCalibrationCutSensor_Cnt_Act=uiTimeCycleCalibrationCutSensor_Ms=uiTimeCycleCalibrationCutSensor_Cnt=0;
			ucSendCommandPosTaglioMax519_Main(ucDataDac);
			spiralatrice.CorsaColtello=macParms.iasse[1]-macParms.iasse[0];
			vAggiornaPosizioneColtello();
			sensoreTaglio.fPosPretaglio=((sensoreTaglio.uiPosPretaglio-macParms.uiMinPosColtello_StepAD)*spiralatrice.CorsaColtello)/(sensoreTaglio.uiMax_cur-sensoreTaglio.uiMin_cur);
			// indico di partire sempre lento a fare taglio/pretaglio??? no
			vAttivaOperazioneLama(enumCiclicaLama_veloce);
			// ogni 100ms rinfresco gli ingressi digitali
			ucLcdRefreshTimeoutEnable(100);

			uc_0slow_1fast_dacColtello=1;
			vSetStatusCurrentWindow(enumWindowStatus_DoSomeActivity_0);
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enumCalSens_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enumCalSens_slowfast:
							if (uc_0slow_1fast_dacColtello==0){
								uc_0slow_1fast_dacColtello=1;
								vAttivaOperazioneLama(enumCiclicaLama_veloce);
							}
							else{
								uc_0slow_1fast_dacColtello=0;
								vAttivaOperazioneLama(enumCiclicaLama_lenta);
							}
							break;

						//case enumCalSens_do_dn:
						//	vAttivaOperazioneLama(enumCiclicaLama_scendi_lenta);
						//	break;

						case enumCalSens_Cut:
						case enumCalSens_do_cut:
							if(uc_0slow_1fast_dacColtello==0)
								vAttivaOperazioneLama(enumCiclicaLama_lenta);
							else
								vAttivaOperazioneLama(enumCiclicaLama_veloce);
							vAttivaOperazioneLama(enumCiclicaLama_taglia);
							// non occorre rinfrescare la finestra
							break;

						case enumCalSens_do_precut:
							if(uc_0slow_1fast_dacColtello==0)
								vAttivaOperazioneLama(enumCiclicaLama_lenta);
							else
								vAttivaOperazioneLama(enumCiclicaLama_veloce);
							vAttivaOperazioneLama(enumCiclicaLama_pretaglia);
							break;

						case enumCalSens_Sx:
						case enumCalSens_Dx:
						case enumCalSens_Up:
						case enumCalSens_Esc:
							vAttivaOperazioneLama(enumCiclicaLama_veloce);
							vAttivaOperazioneLama(enumCiclicaLama_taglia);
							ucSendCommandPosTaglioMax519_Main(128);
							ucLcdRefreshTimeoutEnable(0);
							vHaltReadCycleSensoreTaglio();
							vHandleEscFromMenu();
							// indico di rinfrescare la finestra
							return 2;

						case enumCalSens_do_cal:
							ucSensorCycle_Active=1;
							// imposto dac coltello...
							uiTimeCycleCalibrationCutSensor_Ms=0;
							uiTimeCycleCalibrationCutSensor_Cnt=ucCntInterrupt;
							ucLcdRefreshTimeoutEnable(100);
							// dac negativo 6 step per calibrazione
							ucDataDac=128-6;
							vInitReadCycleSensoreTaglio();
							ucSendCommandPosTaglioMax519_Main(ucDataDac);
							break;

						case enumCalSens_dac:
						case enumCalSens_gain:
						case enumCalSens_posTolerance_change:
						case enumCalSens_posPretaglio:
						case enumCalSens_slowdac_change:
							defSetWinParam(defCALSENS_NumParametroMacchina,i);
							idxParametro=i-enumCalSens_gain;
							paramsNumK.enumUm=if_CALSENS[idxParametro].enumUm;	
							// imposto limiti min e max
							paramsNumK.fMin=if_CALSENS[idxParametro].fMin;
							paramsNumK.fMax=if_CALSENS[idxParametro].fMax;
							if ((nvram_struct.actUM==UM_IN)&&(ucIsUmSensitive(if_CALSENS[idxParametro].enumUm))){
								paramsNumK.fMin*=MM_TO_INCH;
								paramsNumK.fMax*=MM_TO_INCH;
								paramsNumK.enumUm++;
							}
						    // imposto stringa di picture
						    mystrcpy((char*)paramsNumK.ucPicture,(char*)if_CALSENS[idxParametro].ucPictField,sizeof(paramsNumK.ucPicture)-1);

							paramsNumK.ucMaxNumChar=if_CALSENS[idxParametro].ucLenField;	
							// titolo che deve comparire...
							mystrcpy(paramsNumK.ucTitle,pucStringLang(if_CALSENS[idxParametro].stringa),sizeof(paramsNumK.ucTitle)-1);
							// copio la stringa con cui si inizializza il numeric keypad
							if (if_CALSENS[idxParametro].pc)
								vGetValueParametro_spiwin2(hw.ucStringNumKeypad_in,idxParametro,&if_CALSENS[0]);
							else
								strcpy(hw.ucStringNumKeypad_in,"0");

							// imposto abilitazione uso stringa picture
							paramsNumK.ucPictureEnable=if_CALSENS[idxParametro].ucPictureEnable;	// abilitazione stringa picture
							// imposto abilitazione uso min/max
							paramsNumK.ucMinMaxEnable=if_CALSENS[idxParametro].ucMinMaxEnable;	// abilitazione controllo valore minimo/massimo
							// chiamo il numeric keypad 
							ucCallWindow(enumWinId_Num_Keypad);
							// indico di rinfrescare la finestra
							return 2;


					}
				}
			}

			if (ucSensorCycle_Active){
				uiTimeCycleCalibrationCutSensor_Cnt_Act=ucCntInterrupt;
				if (uiTimeCycleCalibrationCutSensor_Cnt_Act>uiTimeCycleCalibrationCutSensor_Cnt){
					uiTimeCycleCalibrationCutSensor_Ms+=(uiTimeCycleCalibrationCutSensor_Cnt_Act-uiTimeCycleCalibrationCutSensor_Cnt_Act)*PeriodoIntMs;
				}
				else{
					uiTimeCycleCalibrationCutSensor_Ms+=((256+uiTimeCycleCalibrationCutSensor_Cnt_Act)-uiTimeCycleCalibrationCutSensor_Cnt_Act)*PeriodoIntMs;
				}
				uiTimeCycleCalibrationCutSensor_Cnt=uiTimeCycleCalibrationCutSensor_Cnt_Act;
				if (uiTimeCycleCalibrationCutSensor_Ms>10000){
					ucSensorCycle_Active=0;
					vHaltReadCycleSensoreTaglio();
					ucDataDac=128;
					ucSendCommandPosTaglioMax519_Main(ucDataDac);
					macParms.uiMinPosColtello_StepAD=sensoreTaglio.uiMin_cur;
					macParms.uiMaxPosColtello_StepAD=sensoreTaglio.uiMax_cur;
					ValidateMacParms();
					ucSaveMacParms=1;
					// eseguo un taglio
					vAttivaOperazioneLama(enumCiclicaLama_taglia);
					// reinizializzo tutto...
					vSetStatusCurrentWindow(enumWindowStatus_DoSomeActivity_0);
				}

			}

			//strcpy(hw.ucString,"CALIBRA SENSORE");
			vStringLangCopy(hw.ucString,enumStr20_CalibraSensore);
			//ucPrintStaticButton(hw.ucString,defCalSens_title_row,defCalSens_title_col,enumFontMedium,enumCalSens_Title,defLCD_Color_Green);
			ucPrintTitleButton(hw.ucString,defCalSens_title_row,defCalSens_title_col,enumFontMedium,enumCalSens_Title,defLCD_Color_Trasparente,1);

			strcpy(hw.ucString,"POS STEP  VOLT   mm   ");
			ucPrintSmallText_ColNames(hw.ucString,defCalSensPosition_row-14,defCalSensPosition_col);

			strcpy(hw.ucString,"ACT");
			ucPrintSmallText_ColNames(hw.ucString,defCalSensPosition_row,defCalSensPosition_col);
			if (iArrayConvAD7327[7]<sensoreTaglio.uiMin_cur)
				fAux=0.0;
			else
				fAux=(iArrayConvAD7327[7]-sensoreTaglio.uiMin_cur)*fColtello_Step2mm;
			sprintf(hw.ucString,"%-4i %-6.3f %-5.2f",iArrayConvAD7327[7],fArrayConvAD7327_Volts[7],fAux);
			ucPrintTitleButton(hw.ucString,defCalSensPosition_row,defCalSensPosition_col+3*8+10,enumFontSmall,enumCalSens_Title,defLCD_Color_Trasparente,2);

			strcpy(hw.ucString,"MIN");
			ucPrintSmallText_ColNames(hw.ucString,defCalSensPosition_row+1*defCalSensPosition_row_offset,defCalSensPosition_col);
			sprintf(hw.ucString,"%-4i %-6.3f 0.0  ",sensoreTaglio.uiMin_cur,sensoreTaglio.uiMin_cur*10.0/defMaxValueAD7327);
			ucPrintTitleButton(hw.ucString,defCalSensPosition_row+1*defCalSensPosition_row_offset,defCalSensPosition_col+3*8+10,enumFontSmall,enumCalSens_posMinima,defLCD_Color_Trasparente,2);

			strcpy(hw.ucString,"MAX");
			ucPrintSmallText_ColNames(hw.ucString,defCalSensPosition_row+2*defCalSensPosition_row_offset,defCalSensPosition_col);
			sprintf(hw.ucString,"%-4i %-6.3f %-5.2f",sensoreTaglio.uiMax_cur,sensoreTaglio.uiMax_cur*10.0/defMaxValueAD7327,spiralatrice.CorsaColtello);
			ucPrintTitleButton(hw.ucString,defCalSensPosition_row+2*defCalSensPosition_row_offset,defCalSensPosition_col+3*8+10,enumFontSmall,enumCalSens_posMassima,defLCD_Color_Trasparente,2);

			strcpy(hw.ucString,"TOL");
			ucPrintStaticButton(hw.ucString,defCalSensPosition_row+3*defCalSensPosition_row_offset,defCalSensPosition_col,enumFontSmall,enumCalSens_posTolerance_change,defLCD_Color_Trasparente);
			fAux=macParms.uiTolerancePosColtello_StepAD*fColtello_Step2mm;
			sprintf(hw.ucString,"%-4i %-6.3f %-5.2f",
					macParms.uiTolerancePosColtello_StepAD,
					macParms.uiTolerancePosColtello_StepAD*10.0/defMaxValueAD7327,
					fAux);
			ucPrintTitleButton(hw.ucString,defCalSensPosition_row+3*defCalSensPosition_row_offset,defCalSensPosition_col+3*8+10,enumFontSmall,enumCalSens_posTolerance,defLCD_Color_Trasparente,2);

			sprintf(hw.ucString,"GAIN:%-4i",sensoreTaglio.uiKp);
			ucPrintStaticButton(hw.ucString,defCalSensPosition_row+4*defCalSensPosition_row_offset,defCalSensPosition_col,enumFontSmall,enumCalSens_gain,defLCD_Color_Trasparente);

			sensoreTaglio.fPosPretaglio=((sensoreTaglio.uiPosPretaglio-macParms.uiMinPosColtello_StepAD)*spiralatrice.CorsaColtello)/(sensoreTaglio.uiMax_cur-sensoreTaglio.uiMin_cur);
			if (nvram_struct.actUM==UM_IN){
				sprintf(hw.ucString,"PRECUT:%-5.2f",sensoreTaglio.fPosPretaglio*MM_TO_INCH);
			}
			else
				sprintf(hw.ucString,"PRECUT:%-5.2f",sensoreTaglio.fPosPretaglio);
			ucPrintStaticButton(hw.ucString,defCalSensPosition_row+4*defCalSensPosition_row_offset,defCalSensPosition_col+100,enumFontSmall,enumCalSens_posPretaglio,defLCD_Color_Trasparente);

			if (nvram_struct.actUM==UM_IN)
				sprintf(hw.ucString,"%-6.6s",pucStringLang(enumStr20_Um_inch));
			else
				sprintf(hw.ucString,"%-6.6s",pucStringLang(enumStr20_Um_mm));
		    ucPrintTitleButton(hw.ucString,defCalSensPosition_row+4*defCalSensPosition_row_offset,defCalSensPosition_col+100+12*8+12,enumFontSmall,enumCalSens_posPretaglio,defLCD_Color_Trasparente,4);

			sprintf(hw.ucString,"DAC:%-5.2fV",(((signed int )ucLastCommandTaglio)-128)*20.0/255.0);
			ucPrintStaticButton(hw.ucString,defCalSensPosition_row+5*defCalSensPosition_row_offset,defCalSensPosition_col,enumFontSmall,enumCalSens_dac,defLCD_Color_Trasparente);

			if (!ucSensorCycle_Active){
				strcpy(hw.ucString,"CALIBRATE  ");
				ucPrintStaticButton(hw.ucString,defCalSensPosition_row+5*defCalSensPosition_row_offset,defCalSensPosition_col+100,enumFontSmall,enumCalSens_do_cal,defLCD_Color_Trasparente);
			}
			else{
				strcpy(hw.ucString,"STOP CAL   ");
				ucPrintStaticButton(hw.ucString,defCalSensPosition_row+5*defCalSensPosition_row_offset,defCalSensPosition_col+100,enumFontSmall,enumCalSens_do_cal,defLCD_Color_Yellow);
			}

			sprintf(hw.ucString,"SLOW:%-5.2fV",sensoreTaglio.fSlowDacColtelloVolt);
			ucPrintStaticButton(hw.ucString,defCalSensPosition_row+5*defCalSensPosition_row_offset,320-8*11-12,enumFontSmall,enumCalSens_slowdac_change,defLCD_Color_Trasparente);


			if (!uc_0slow_1fast_dacColtello){
				strcpy(hw.ucString,"SLOW   ");
			}
			else{
				strcpy(hw.ucString,"FAST   ");
			}
			ucPrintStaticButton(hw.ucString,defCalSensPosition_row,320-7*8-12,enumFontSmall,enumCalSens_slowfast,defLCD_Color_Trasparente);


			if (  (sensoreTaglio.stato_taglio>=enumOperazioneLama_Taglio_Init)
				&&(sensoreTaglio.stato_taglio<=enumOperazioneLama_WaitTaglioHi_0)
				)
				ucColor=defLCD_Color_Yellow;
			else
				ucColor=(unsigned char)defLCD_Color_Trasparente;

			strcpy(hw.ucString,"CUT!   ");
			ucPrintStaticButton(hw.ucString,defCalSensPosition_row+1*defCalSensPosition_row_offset,320-7*8-12,enumFontSmall,enumCalSens_do_cut,ucColor);

			if (  (sensoreTaglio.stato_taglio>=enumOperazioneLama_Pretaglio_Init)
				&&(sensoreTaglio.stato_taglio<=enumOperazioneLama_Pretaglio)
				)
				ucColor=defLCD_Color_Yellow;
			else
				ucColor=(unsigned char)defLCD_Color_Trasparente;
			strcpy(hw.ucString,"PRECUT!");
			ucPrintStaticButton(hw.ucString,defCalSensPosition_row+2*defCalSensPosition_row_offset,320-7*8-12,enumFontSmall,enumCalSens_do_precut,ucColor);
			// print label open/closed loop
			if (sensoreTaglio.stato_taglio==enumOperazioneLama_Idle){
				strcpy(hw.ucString,"OPEN LOOP");
				ucPrintTitleButton(hw.ucString,defCalSensPosition_row+3*defCalSensPosition_row_offset,320-9*8-12,enumFontSmall,enumCalSens_open_closed_loop,defLCD_Color_Red,4);
			}
			else{
				strcpy(hw.ucString,"CLOSED LOOP");
				ucPrintTitleButton(hw.ucString,defCalSensPosition_row+3*defCalSensPosition_row_offset,320-10*8-12,enumFontSmall,enumCalSens_open_closed_loop,defLCD_Color_Green,4);
			}


			vAddStandardButtons(enumCalSens_Sx);

			return 1;
	}
}




#define defMaxCharStringaAltriParsProduzione 20
typedef char tipoStringaAltriParsProduzione[defMaxCharStringaAltriParsProduzione+1];

typedef enum{
		enumAPP_CambioBoccola=0,
		enumAPP_TaraturaDistanziatore,
		enumAPP_Log,
		enumAPP_spindle_grinding,
		enumAPP_NumeroSpirePretaglio,
		enumAPP_TolleranzaFilo,
		enumAPP_DiametroLama,
		enumAPP_DiametroRuota1,
		enumAPP_DiametroRuota2,
		enumAPP_NumOf,
		enumAPP_Disabled
	}enumAltriParsProduzione;

// 
code enumStringhe20char uiNomiButtonsAltriParsProd[]={
	enumStr20_CambioBoccola,
	enumStr20_TabellaDistanziatore,
	enumStr20_Log,
	enumStr20_spindle_grinding,
	enumStr20_Spire_pretag,
	enumStr20_Toll_filo,
	enumStr20_Diam_lama,
	enumStr20_Diam_ruota_1,
	enumStr20_Diam_ruota_2,
};

#define defNumEntriesAltriParsProd (sizeof(uiNomiButtonsAltriParsProd)/sizeof(uiNomiButtonsAltriParsProd[0]))

typedef struct _TipoStructHandleAltriParsProd{
	unsigned char ucFirstLine;
}TipoStructHandleAltriParsProd;

xdata TipoStructHandleAltriParsProd happ;



code TipoStructInfoParametro_Lingua if_APP[]={
// cambio boccola...
	{0,3, "nnn","%3hi",0,100,(unsigned char xdata*)&spiralatrice.actPosLama,1,1,enumStr20_Pos_riposo_taglio,   enumPT_unsignedChar,enumUm_none,     enumStringType_none,1},//
// setup distanziatore
	{0,3, "nnn","%3hi",0,100,(unsigned char xdata*)&spiralatrice.actPosLama,1,1,enumStr20_Pos_riposo_taglio,   enumPT_unsignedChar,enumUm_none,     enumStringType_none,1},//
// log produzione
	{0,3, "nnn","%3hi",0,100,(unsigned char xdata*)&spiralatrice.actPosLama,1,1,enumStr20_Pos_riposo_taglio,   enumPT_unsignedChar,enumUm_none,     enumStringType_none,1},//
// spindle grind
	{0,3, "nnn","%3hi",0,100,(unsigned char xdata*)&spiralatrice.actPosLama,1,1,enumStr20_Pos_riposo_taglio,   enumPT_unsignedChar,enumUm_none,     enumStringType_none,1},//
// spire di pretaglio
	{0,2, "nn","%2hi",MIN_NSPIRE_PRET,MAX_NSPIRE_PRET,(unsigned char xdata*)&macParms.nspire_pret,           1,1,enumStr20_N_spire_pretaglio,enumPT_unsignedChar, enumUm_num,enumStringType_none,1},
// tolleranza filo...
	{0,3, "nnn","%3hi",0,100,(unsigned char xdata*)&spiralatrice.actPosLama,1,1,enumStr20_Pos_riposo_taglio,   enumPT_unsignedChar,enumUm_percentuale,     enumStringType_none,1},//
// diametro lama di taglio
	{0,8, "NNNNNNNN","%-6.2f",MIN_DIAM_COLTELLO,MAX_DIAM_COLTELLO,(unsigned char xdata*)&macParms.diam_coltello,1,1,enumStr20_Diametro_coltello,   enumPT_float,enumUm_mm,     enumStringType_none,MM_TO_INCH},//
// diametro ruota 1
	{0,8, "NNNNNNNN","%-6.2f",MIN_DIAM_FRIZIONE,MAX_DIAM_FRIZIONE,(unsigned char xdata*)&macParms.diam_frizioni[0],1,1,enumStr20_Diametro_ruota_1,   enumPT_float,enumUm_mm,     enumStringType_none,MM_TO_INCH},//
// diametro ruota 2
	{0,8, "NNNNNNNN","%-6.2f",MIN_DIAM_FRIZIONE,MAX_DIAM_FRIZIONE,(unsigned char xdata*)&macParms.diam_frizioni[1],1,1,enumStr20_Diametro_ruota_2,   enumPT_float,enumUm_mm,     enumStringType_none,MM_TO_INCH},//
//	{0,3, "nnn","%3hi",0,100,(unsigned char xdata*)&spiralatrice.actPosLama,1,1,enumStr20_Pos_riposo_taglio,   enumPT_unsignedChar,enumUm_none,     enumStringType_none,1},//
};

typedef enum {
	enumCambioBoccola_idle,
	enumCambioBoccola_waitAbutton,
	enumCambioBoccola_waitPressedUpDnAndStart,
	enumCambioBoccola_pressedScendi,
	enumCambioBoccola_waitPressedScendiAndStart,
	enumCambioBoccola_waitDurationScendiAndStart,
	enumCambioBoccola_pressedSali,
	enumCambioBoccola_waitPressedSaliAndStart,
	enumCambioBoccola_waitDurationSaliAndStart,
	enumCambioBoccola_NumOf
}enumStatoCambioBoccola;


typedef struct _TipoHandleCambioBoccola{
	enumStatoCambioBoccola stato;
	unsigned char ucCnt;
	unsigned char ucPressedDn;
	unsigned char ucPressedUp;
}TipoHandleCambioBoccola;
xdata TipoHandleCambioBoccola handleCambioBoccola;


unsigned char ucHW_CambioBoccola(void){

	#define defNumLoopWait_CambioBoccola 0
// imposto 3 secondi di timeout per la procedura comando a due mani-->15 loop da 200ms
	#define defNumLoopTimeout_CambioBoccola 15

	#define defBOCC_title_row 6
	#define defBOCC_title_col 0

	#define defBOCC_start_change_row (defLcdWidthY_pixel-defBOCC_title_row-24-18-4)/2
	#define defBOCC_start_change_col (defLcdWidthX_pixel-6*36-40-16)

	#define defBOCC_up_row (defBOCC_start_change_row-24)
	#define defBOCC_up_col (defLcdWidthX_pixel-40-24)

	#define defBOCC_dn_row (defBOCC_start_change_row+24)
	#define defBOCC_dn_col (defLcdWidthX_pixel-40-24)

	// pulsanti della finestra
	typedef enum {
			enumBOCC_desc=0,
			enumBOCC_Title,
			enumBOCC_Up,
			enumBOCC_Dn,
			enumBOCC_Esc,


			enumBOCC_NumOfButtons
		}enumButtons_BOCC;

	xdata unsigned int i;

	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enumBOCC_NumOfButtons;i++)
				ucCreateTheButton(i); 
			ucEnableStartLavorazione(0);
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			nvram_struct.ucImAmImMenuBoccola_0x37=0x37;
			memset(&handleCambioBoccola,0,sizeof(handleCambioBoccola));
			ucLcdRefreshTimeoutEnable(400);
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enumBOCC_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enumBOCC_Esc:
							{
								unsigned int ui_lama_a_riposo;
								ui_lama_a_riposo=
									 (    (sensoreTaglio.uiLetturaSensoreTaglio>sensoreTaglio.uiPosRiposo)
									  &&((sensoreTaglio.uiLetturaSensoreTaglio-sensoreTaglio.uiPosRiposo)<100)
									 )
								   ||
									(    (sensoreTaglio.uiLetturaSensoreTaglio<sensoreTaglio.uiPosRiposo)
									  &&((sensoreTaglio.uiPosRiposo-sensoreTaglio.uiLetturaSensoreTaglio)<100)
									) ;
								if ((spiralatrice.actPosLama>=100)||(ui_lama_a_riposo)){
									// chiamo il main menu...
									ucEnableStartLavorazione(1);
									nvram_struct.ucImAmImMenuBoccola_0x37=0;
									ucLcdRefreshTimeoutEnable(0);
									vHandleEscFromMenu();
								}
							}
							// indico di rinfrescare la finestra
							return 2;

						case enumBOCC_Up:
							// se sono gi� up, non ripeto
							if(  (    (sensoreTaglio.uiLetturaSensoreTaglio>sensoreTaglio.uiPosRiposo)
							      &&((sensoreTaglio.uiLetturaSensoreTaglio-sensoreTaglio.uiPosRiposo)<100)
							     )
							   ||
							    (    (sensoreTaglio.uiLetturaSensoreTaglio<sensoreTaglio.uiPosRiposo)
							      &&((sensoreTaglio.uiPosRiposo-sensoreTaglio.uiLetturaSensoreTaglio)<100)
							    ) 
							 ){
								// risolve bug per cui se lama veniva riportata in alto da stop, non era pi� possibile uscire da menu boccola...
								spiralatrice.actPosLama=100;
								break;
							}
							handleCambioBoccola.ucPressedUp=1;
							// indico di rinfrescare la finestra
							break;
						case enumBOCC_Dn:
							handleCambioBoccola.ucPressedDn=1;
							// indico di rinfrescare la finestra
							break;
					}
				}
			}


			switch(handleCambioBoccola.stato){
				case enumCambioBoccola_idle:
					if (handleCambioBoccola.ucPressedUp){
						handleCambioBoccola.ucPressedUp=0;
					}
					if (handleCambioBoccola.ucPressedDn){
						handleCambioBoccola.ucPressedDn=0;
					}
					if (!ucTouchScreenPressed()&&!(InpDig & IDG_START))
						handleCambioBoccola.stato=enumCambioBoccola_waitAbutton;
					break;
				case enumCambioBoccola_waitAbutton:
					handleCambioBoccola.ucCnt=0;
					if (handleCambioBoccola.ucPressedUp){
						handleCambioBoccola.ucPressedUp=0;
						handleCambioBoccola.stato=enumCambioBoccola_pressedSali;
					}
					else if (handleCambioBoccola.ucPressedDn){
						handleCambioBoccola.ucPressedDn=0;
						handleCambioBoccola.stato=enumCambioBoccola_pressedScendi;
					}
					else if ((InpDig & IDG_START)){
						handleCambioBoccola.stato=enumCambioBoccola_waitPressedUpDnAndStart;
					}
					break;
				case enumCambioBoccola_waitPressedUpDnAndStart:
					if (!(InpDig & IDG_START)){
						handleCambioBoccola.stato=enumCambioBoccola_idle;
						break;
					}
					if (handleCambioBoccola.ucPressedUp){
						handleCambioBoccola.ucPressedUp=0;
						handleCambioBoccola.stato=enumCambioBoccola_waitDurationSaliAndStart;
						handleCambioBoccola.ucCnt=0;
						break;
					}
					else if (handleCambioBoccola.ucPressedDn){
						handleCambioBoccola.ucPressedDn=0;
						handleCambioBoccola.stato=enumCambioBoccola_waitDurationScendiAndStart;
						handleCambioBoccola.ucCnt=0;
						break;
					}
					if (++handleCambioBoccola.ucCnt>=defNumLoopTimeout_CambioBoccola){
						handleCambioBoccola.stato=enumCambioBoccola_idle;
						break;
					}
					break;
				case enumCambioBoccola_pressedScendi:
					ucEnableStartLavorazione(0);
					ucLcdRefreshTimeoutEnable(400);
					handleCambioBoccola.ucCnt=0;
					handleCambioBoccola.stato=enumCambioBoccola_waitPressedScendiAndStart;
					break;
				case enumCambioBoccola_waitPressedScendiAndStart:
					if (!ucTouchScreenPressed()||handleCambioBoccola.ucPressedUp){
						handleCambioBoccola.stato=enumCambioBoccola_idle;
						break;
					}
					else if ((InpDig & IDG_START)&&ucTouchScreenPressed()){
						handleCambioBoccola.stato=enumCambioBoccola_waitDurationScendiAndStart;
						handleCambioBoccola.ucCnt=0;
						break;
					}
					if (++handleCambioBoccola.ucCnt>=defNumLoopTimeout_CambioBoccola){
						handleCambioBoccola.stato=enumCambioBoccola_idle;
						break;
					}
					break;
				case enumCambioBoccola_waitDurationScendiAndStart:
					if (handleCambioBoccola.ucPressedUp){
						handleCambioBoccola.stato=enumCambioBoccola_idle;
						break;
					}
					if ((InpDig & IDG_START)&&ucTouchScreenPressed()){
						if (++handleCambioBoccola.ucCnt>=defNumLoopWait_CambioBoccola){
							spiralatrice.actPosLama=0;
							if (macParms.ucUsaSensoreTaglio)
								vAttivaOperazioneLama(enumCiclicaLama_scendi_veloce);
							else{
								// attivo l'operazione di taglio!
								outDigVariable|=ODG_TAGLIO;
								// Spedisco al dac il comando di posizionamento della lama. 
								// Vado a modificare la posizione di riposo della lama, non quella di pretaglio!!!
								spiralatrice.DacOut[addrDacColtello*2]=(long)spiralatrice.actPosLama*FS_DAC_MAX519/100;
								// Come posizione di pretaglio prendo quella attuale, per comodit� .
								spiralatrice.DacOut[addrDacColtello*2+1]=spiralatrice.actPosDacColtello*FS_DAC_MAX519/spiralatrice.CorsaColtello;
								SendByteMax519(addrDacColtello,
									spiralatrice.DacOut[addrDacColtello*2],
									spiralatrice.DacOut[addrDacColtello*2+1]);
							}
							handleCambioBoccola.stato=enumCambioBoccola_idle;
						}
					}
					else{
						handleCambioBoccola.ucCnt=0;
						handleCambioBoccola.stato=enumCambioBoccola_idle;
					}
					break;
				case enumCambioBoccola_pressedSali:
					ucEnableStartLavorazione(0);
					ucLcdRefreshTimeoutEnable(100);
					handleCambioBoccola.ucCnt=0;
					handleCambioBoccola.stato=enumCambioBoccola_waitPressedSaliAndStart;
					break;
				case enumCambioBoccola_waitPressedSaliAndStart:
					if (handleCambioBoccola.ucPressedDn){
						handleCambioBoccola.stato=enumCambioBoccola_idle;
						break;
					}
					if (!ucTouchScreenPressed()){
						handleCambioBoccola.stato=enumCambioBoccola_idle;
					}
					else if ((InpDig & IDG_START)&&ucTouchScreenPressed()){
						handleCambioBoccola.stato=enumCambioBoccola_waitDurationSaliAndStart;
						handleCambioBoccola.ucCnt=0;
						break;
					}
					if (++handleCambioBoccola.ucCnt>=defNumLoopTimeout_CambioBoccola){
						handleCambioBoccola.stato=enumCambioBoccola_idle;
						break;
					}
					break;
				case enumCambioBoccola_waitDurationSaliAndStart:
					if (handleCambioBoccola.ucPressedDn){
						handleCambioBoccola.stato=enumCambioBoccola_idle;
						break;
					}
					if ((InpDig & IDG_START)&&ucTouchScreenPressed()){
						if (++handleCambioBoccola.ucCnt>=defNumLoopWait_CambioBoccola){
							spiralatrice.actPosLama=100;
							if (macParms.ucUsaSensoreTaglio)
								vAttivaOperazioneLama(enumCiclicaLama_taglia);
							else{
								// disattivo l'operazione di taglio!
								outDigVariable&=~ODG_TAGLIO;

								// Spedisco al dac il comando di posizionamento della lama. 
								// Vado a modificare la posizione di riposo della lama, non quella di pretaglio!!!
								spiralatrice.DacOut[addrDacColtello*2]=(long)spiralatrice.actPosLama*FS_DAC_MAX519/100;
								// Come posizione di pretaglio prendo quella attuale, per comodit� .
								spiralatrice.DacOut[addrDacColtello*2+1]=spiralatrice.actPosDacColtello*FS_DAC_MAX519/spiralatrice.CorsaColtello;
								SendByteMax519(addrDacColtello,
									spiralatrice.DacOut[addrDacColtello*2],
									spiralatrice.DacOut[addrDacColtello*2+1]);
							}
							handleCambioBoccola.stato=enumCambioBoccola_idle;
						}
					}
					else{
						handleCambioBoccola.ucCnt=0;
						handleCambioBoccola.stato=enumCambioBoccola_idle;
					}
					break;
				default:
				{
					break;
				}
			}
		}

		//strcpy(hw.ucString,"   Altri parametri  ");
		vStringLangCopy(hw.ucString,enumStr20_CambioBoccola);
		//ucPrintStaticButton(hw.ucString,defAPP_title_row,defAPP_title_col,enumFontMedium,enumAPP_Title,defLCD_Color_Green);
		ucPrintTitleButton(hw.ucString,defBOCC_title_row,defBOCC_title_col,enumFontMedium,enumBOCC_Title,defLCD_Color_Trasparente,1);

		vStringLangCopy(hw.ucString,enumStr20_StartCambioBoccola);
		ucPrintTitleButton(hw.ucString,defBOCC_start_change_row,defBOCC_start_change_col,enumFontBig,enumBOCC_desc,defLCD_Color_Trasparente,0);

		ucPrintBitmapButton(enumBitmap_40x24_arrow_up,defBOCC_up_row,defBOCC_up_col,enumBOCC_Up);
		strcpy(hw.ucString," UP  ");
		ucPrintSmallText_ColNames(hw.ucString,defBOCC_up_row+24,defBOCC_up_col);

		ucPrintBitmapButton(enumBitmap_40x24_arrow_dn,defBOCC_dn_row,defBOCC_dn_col,enumBOCC_Dn);
		strcpy(hw.ucString," DN  ");
		ucPrintSmallText_ColNames(hw.ucString,defBOCC_dn_row+24,defBOCC_dn_col);

		ucPrintBitmapButton(enumBitmap_40x24_ESC,defLcdWidthY_pixel-24,(defLcdWidthX_pixel-40)/2,enumBOCC_Esc);
		return 1;

}






#endif

unsigned char ucHW_TaraDistanziatore(void){
	extern void vSaveMacParms(void);

	#define defTARADIST_title_row 6
	#define defTARADIST_title_col 0

	#define defTARADIST_Offset_between_buttons 36

	#define defTARADIST_button_1_row (defTARADIST_title_row+18+18)
	#define defTARADIST_button_1_col 0


	#define defTARADIST_OK_row (defLcdWidthY_pixel-42)
	#define defTARADIST_OK_col (defLcdWidthX_pixel-32*3-8)

	#define defTARADIST_Up_row (defTARADIST_title_row+28)
	#define defTARADIST_Up_col (0)

	#define defTARADIST_Dn_row (defTARADIST_OK_row)
	#define defTARADIST_Dn_col (0)

	#define defNumRowMenuAPP 6

	#define defTARADIST_NumParametro 0

	#define defTARADIST_up_row (defTARADIST_button_1_row+defTARADIST_Offset_between_buttons*0)
	#define defTARADIST_up_col (defLcdWidthX_pixel-40-24)
	#define defTARADIST_dn_row (defTARADIST_up_row+defTARADIST_Offset_between_buttons*2)
	#define defTARADIST_dn_col defTARADIST_up_col

	#define defTARADIST_value_row (defTARADIST_up_row+defTARADIST_Offset_between_buttons*1+8)
	#define defTARADIST_value_col (defTARADIST_up_col-3*16+8)

	#define defTARADIST_delay_row (defTARADIST_button_1_row+defTARADIST_Offset_between_buttons*3+12)

	// pulsanti della finestra
	typedef enum {
			enumTARADIST_button_ForceDn=0,
			enumTARADIST_button_Up,
			enumTARADIST_button_Dn,
			enumTARADIST_button_SpacerPos,
			enumTARADIST_button_SpacerPos_um,
			enumTARADIST_button_bar,
			enumTARADIST_descbutton_descent_time_value,
			enumTARADIST_descbutton_descent_time_um,
			enumTARADIST_descbutton_offset_value,
			enumTARADIST_descbutton_offset_value_um,
			enumTARADIST_Title,

			enumTARADIST_Sx,
			enumTARADIST_Up,
			enumTARADIST_Cut,
			enumTARADIST_Esc,
			enumTARADIST_Dn,
			enumTARADIST_Dx,


			enumTARADIST_NumOfButtons
		}enumTARADIST_buttons;
	xdata unsigned char i;
	xdata PROGRAMMA * xdata pPro;
	xdata unsigned char idxParametro;
	xdata static unsigned char ucDIST_Remember2saveActPrg;
	xdata static unsigned char ucDIST_Remember2saveMacParms;
	xdata float f_aux;


	// inizializzo puntatore a programma running
	pPro=&hprg.theRunning;


	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			idxParametro=defGetWinParam(defTARADIST_NumParametro);
			if (idxParametro==enumTARADIST_descbutton_descent_time_value){
				pPro->usDelayDistanziaDownMs=atoi(hw.ucStringNumKeypad_out);
				ucDIST_Remember2saveActPrg=1;
			}
			else if (idxParametro==enumTARADIST_descbutton_offset_value){
				pPro->f_coltello_offset_n_spire=atof(hw.ucStringNumKeypad_out);
				ucDIST_Remember2saveActPrg=1;
			}
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enumTARADIST_NumOfButtons;i++)
				ucCreateTheButton(i); 
			happ.ucFirstLine=0;
			ucDIST_Remember2saveActPrg=0;
			ucDIST_Remember2saveMacParms=0;
			vAggiornaPosizioneSpaziaturaColtello();
			ucLcdRefreshTimeoutEnable(400);
			hDist.ucForceDown=0;

			// inizializzo la struttura distConversion
			distConversion.pPro=pPro;
			distConversion.f_res_value_ohm=nvram_struct.commesse[spiralatrice.runningCommessa].res;
			f_aux=0.0;
			for (i=0;i<pPro->num_fili;i++)
				f_aux+=1/nvram_struct.resspec[i];
			distConversion.f_res_specifica_ohm_m=1/f_aux;
			// reimpio la struttura dist utilizzando i dati salvati in distConversion
			ucFillStructDist(&distConversion);

			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enumTARADIST_NumOfButtons;i++){
				if (!ucHasBeenPressedButton(i)){
					continue;
				}
				switch(i){
					case enumTARADIST_Esc:
					case enumTARADIST_Sx:
					case enumTARADIST_Dx:
						vUnlockOperazioneLama();
						if (hDist.ucForceDown){
							vAttivaOperazioneLama(enumCiclicaLama_sali);
							hDist.ucForceDown=0;
						}
						if (ucDIST_Remember2saveActPrg){
							// non serve, i valori vengono gi� variati dalla routine
							//ucFillProgramStructDist(&distConversion);
							ucSaveActPrg();
							ucDIST_Remember2saveActPrg=0;
						}
						if (ucDIST_Remember2saveMacParms){
							ucDIST_Remember2saveMacParms=0;
							vSaveMacParms();
						}
						ucLcdRefreshTimeoutEnable(0);
						vHandleEscFromMenu();
						break;
					case enumTARADIST_button_ForceDn:
						// toggle forcing down
						hDist.ucForceDown=!hDist.ucForceDown;
						//  forzata discesa del coltello, e deve rimanere gi�!
						if (hDist.ucForceDown){
							vAttivaOperazioneLama(enumCiclicaLama_distanzia);
							vLockOperazioneLama(enumCiclicaLama_distanzia);
						}
						// se viene comandata risalita...
						else{
							vUnlockOperazioneLama();
							vAttivaOperazioneLama(enumCiclicaLama_sali);
						}
						break;
					case enumTARADIST_button_Up:
						if (!hDist.ucForceDown)
							break;
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
							ucDIST_Remember2saveActPrg=1;
							//ucSaveActPrg();
							// aggiorno dac che comanda la posizione del coltello
							vAggiornaPosizioneSpaziaturaColtello();
							vAttivaOperazioneLama(enumCiclicaLama_distanzia);
						}
						break;
					case enumTARADIST_button_Dn:
						if (!hDist.ucForceDown)
							break;
						if (spiralatrice.f_pos_coltello_spaziatura_mm>2*spiralatrice.CorsaColtello*(1.0/FS_DAC_MAX519)+0.01){
							// Abbasso il coltello di 2 bit per ottenere
							// 0.1 millimetri.
							spiralatrice.f_pos_coltello_spaziatura_mm-=2*spiralatrice.CorsaColtello*(1.0/FS_DAC_MAX519);
							// Faccio ristampare il valore ed aggiornare la posizione
							// del coltello.
							hprg.ptheAct->f_coltello_pos_spaziatura_mm=spiralatrice.f_raggio_coltello_meno_interasse-spiralatrice.f_pos_coltello_spaziatura_mm;
							// non salvo subito il programma, altrimenti introduco una pausa...
							ucDIST_Remember2saveActPrg=1;
							//ucSaveActPrg();
							// aggiorno dac che comanda la posizione del coltello
							vAggiornaPosizioneSpaziaturaColtello();
							vAttivaOperazioneLama(enumCiclicaLama_distanzia);
						}
						break;

					case enumTARADIST_descbutton_descent_time_value:
						defSetWinParam(defTARADIST_NumParametro,enumTARADIST_descbutton_descent_time_value);
						paramsNumK.ucMaxNumChar=5;
						// imposto stringa di picture
						strcpy(paramsNumK.ucPicture,"nnnnn");
						paramsNumK.enumUm=enumUm_millisecondi;
						// imposto limiti min e max
						paramsNumK.fMin=0;
						paramsNumK.fMax=5000;
						//strcpy(paramsNumK.ucTitle,im_setup_prod[j].ucTitleNumK);
						memset(paramsNumK.ucTitle,0,sizeof(paramsNumK.ucTitle));				
						vStringLangCopy(paramsNumK.ucTitle,enumStr20_DelayDistanziatoreDown);	
						// copio la stringa con cui si inizializza il numeric keypad
						sprintf(hw.ucStringNumKeypad_in,"%5i",pPro->usDelayDistanziaDownMs);
						// imposto abilitazione uso stringa picture
						paramsNumK.ucPictureEnable=1;	// abilitazione stringa picture
						// imposto abilitazione uso min/max
						paramsNumK.ucMinMaxEnable=1;	// abilitazione controllo valore minimo/massimo
						ucLcdRefreshTimeoutEnable(0);
						// chiamo il numeric keypad
						ucCallWindow(enumWinId_Num_Keypad);
						// indico di rinfrescare la finestra
						return 2;


					case enumTARADIST_descbutton_offset_value:
						defSetWinParam(defTARADIST_NumParametro,enumTARADIST_descbutton_offset_value);
						paramsNumK.ucMaxNumChar=6;
						// imposto stringa di picture
						strcpy(paramsNumK.ucPicture,"NNNNNN");
						// imposto limiti min e max
						paramsNumK.fMin=MIN_OFFSET_COLTELLO_DISTANZIATORE_mm;
						paramsNumK.fMax=MAX_OFFSET_COLTELLO_DISTANZIATORE_mm;
						//strcpy(paramsNumK.ucTitle,im_setup_prod[j].ucTitleNumK);
						memset(paramsNumK.ucTitle,0,sizeof(paramsNumK.ucTitle));
						vStringLangCopy(paramsNumK.ucTitle,enumStr20_Distanza_Coltello_Distanziatore);	
						paramsNumK.enumUm=enumUm_num;			
						sprintf(hw.ucStringNumKeypad_in,"%6.3f",pPro->f_coltello_offset_n_spire);				
						// imposto abilitazione uso stringa picture
						paramsNumK.ucPictureEnable=1;	// abilitazione stringa picture
						// imposto abilitazione uso min/max
						paramsNumK.ucMinMaxEnable=1;	// abilitazione controllo valore minimo/massimo
						ucLcdRefreshTimeoutEnable(0);
						// chiamo il numeric keypad
						ucCallWindow(enumWinId_Num_Keypad);
						// indico di rinfrescare la finestra
						return 2;


				}//switch
			}//for (i=0;i<enumTARADIST_NumOfButtons;i++)

			/* Mi procuro un puntatore al record del programma in esecuzione. */
			spiralatrice.pPrgRun=&hprg.theRunning;

// titolo della finestra
			sprintf(hw.ucString,"%-20.20s",pucStringLang(enumStr20_TabellaDistanziatore));
			ucPrintTitleButton(hw.ucString,defTARADIST_title_row,defTARADIST_title_col,enumFontMedium,enumTARADIST_Title,defLCD_Color_Trasparente,1);

// pulsante UP
			// il tasto up lo visualizzo solo se lo spacer � forzato gi�
			if (hDist.ucForceDown){
				ucPrintBitmapButton(enumBitmap_40x24_arrow_up,defTARADIST_up_row,defTARADIST_up_col	,enumTARADIST_button_Up);
				strcpy(hw.ucString," UP  ");
				ucPrintSmallText_ColNames(hw.ucString,defTARADIST_up_row+24,defTARADIST_up_col);
			}

// pulsante "spacer dn"
			vStringLangCopy(hw.ucString,enumStr20_DistanziatoreGoDown);
			ucPrintStaticButton(hw.ucString,    defTARADIST_value_row ,defTARADIST_button_1_col      ,enumFontMedium,enumTARADIST_button_ForceDn,hDist.ucForceDown?defLCD_Color_Yellow:defLCD_Color_Trasparente);
			if (nvram_struct.actUM==UM_IN)
				f_aux=MM_TO_INCH;
			else
				f_aux=1;

// valore attuale della posizione di spaziatura
			sprintf(hw.ucString,"%+7.2f",hprg.ptheAct->f_coltello_pos_spaziatura_mm*f_aux);
			ucPrintTitleButton(hw.ucString,  defTARADIST_value_row ,defTARADIST_value_col    ,enumFontMedium,enumTARADIST_button_SpacerPos,defLCD_Color_Trasparente,0);


			// il tasto dn lo visualizzo solo se lo spacer � forzato gi�
			if (hDist.ucForceDown){
				ucPrintBitmapButton(enumBitmap_40x24_arrow_dn,defTARADIST_dn_row,defTARADIST_dn_col	,enumTARADIST_button_Dn);
				strcpy(hw.ucString," DN  ");
				ucPrintSmallText_ColNames(hw.ucString,defTARADIST_dn_row+24,defTARADIST_dn_col);
			}


			vString40LangCopy(hw.ucString,enumStr40_predist_offset);
			ucPrintSmallText_ColNames(hw.ucString,defTARADIST_delay_row-12,0);

			sprintf(hw.ucString,"%5i",pPro->usDelayDistanziaDownMs);				
			ucPrintStaticButton(hw.ucString , defTARADIST_delay_row ,defTARADIST_button_1_col      ,enumFontMedium,enumTARADIST_descbutton_descent_time_value,defLCD_Color_Trasparente);

			sprintf(hw.ucString,"%-20.20s",pucStringLang(enumStr20_Um_millisecondi));
			ucPrintTitleButton(hw.ucString,defTARADIST_delay_row+16+8 ,defTARADIST_button_1_col  ,enumFontSmall,enumTARADIST_descbutton_descent_time_um,defLCD_Color_Trasparente,0);


			sprintf(hw.ucString,"%6.1f",pPro->f_coltello_offset_n_spire);				
			ucPrintStaticButton(hw.ucString, defTARADIST_delay_row ,defLcdWidthX_pixel/2      ,enumFontMedium,enumTARADIST_descbutton_offset_value,defLCD_Color_Trasparente);

			// se uso il coltello, l'offset indica il numero di spire da aggiungere a quello di default
			// per compensare la compressione della spira
			sprintf(hw.ucString,"%-20.20s",pucStringLang(enumStr20_Um_num));			
			ucPrintTitleButton(hw.ucString,defTARADIST_delay_row+16+8 ,defLcdWidthX_pixel/2,enumFontSmall,enumTARADIST_descbutton_offset_value_um,defLCD_Color_Trasparente,0);

			vAddStandardButtons(enumTARADIST_Sx);

			return 1;
	}
}


// altri pars di produzione...
unsigned char ucHW_AltriParametriProduzione(void){
	#define defAPP_title_row 6
	#define defAPP_title_col 0

	#define defAPP_Offset_between_buttons 32

	#define defAPP_button_1_row (defAPP_title_row+18)
	#define defAPP_button_1_col 0


	#define defAPP_OK_row (defLcdWidthY_pixel-42)
	#define defAPP_OK_col (defLcdWidthX_pixel-32*3-8)

	#define defAPP_Up_row (defAPP_title_row+28)
	#define defAPP_Up_col (0)

	#define defAPP_Dn_row (defAPP_OK_row)
	#define defAPP_Dn_col (0)

	#define defNumRowMenuAPP 6

	#define defAPP_NumAltroParametro 0

	// pulsanti della finestra
	typedef enum {
			enumAPP_button_1=0,
			enumAPP_button_2,
			enumAPP_button_3,
			enumAPP_button_4,
			enumAPP_button_5,
			enumAPP_button_6,
			enumAPP_descbutton_1,
			enumAPP_descbutton_2,
			enumAPP_descbutton_3,
			enumAPP_descbutton_4,
			enumAPP_descbutton_5,
			enumAPP_descbutton_6,
			enumAPP_umbutton_1,
			enumAPP_umbutton_2,
			enumAPP_umbutton_3,
			enumAPP_umbutton_4,
			enumAPP_umbutton_5,
			enumAPP_Title,

			enumAPP_Sx,
			enumAPP_Up,
			enumAPP_Cut,
			enumAPP_Esc,
			enumAPP_Dn,
			enumAPP_Dx,


			enumAPP_NumOfButtons
		}enumButtons_APP;
	xdata unsigned char i;
	xdata PROGRAMMA * xdata pPro;
	xdata unsigned char idxParametro;
	static enumAltriParsProduzione buttons_ids[enumAPP_NumOf];
	static enumAltriParsProduzione cur_buttons_ids[defNumRowMenuAPP];


	// inizializzo puntatore a programma running
	pPro=&hprg.theRunning;


	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			idxParametro=defGetWinParam(defAPP_NumAltroParametro);
			if (  (idxParametro==enumAPP_TolleranzaFilo)
				||(idxParametro==enumAPP_CambioBoccola)
				||(idxParametro==enumAPP_Log)
				||(idxParametro==enumAPP_TaraturaDistanziatore)
				||(idxParametro==enumAPP_spindle_grinding)
				){
			}
			else{
				vSetValueParametro_spiwin2(hw.ucStringNumKeypad_out,idxParametro,&if_APP[0]);
				switch(idxParametro){
					case enumAPP_NumeroSpirePretaglio:
// Modifica Gennaio 1999: numero di spire di pretaglio puo' essere
// aggiustato anche con macchina in funzione.
						if (PrgRunning)
							vCalcAnticipoPretaglio();
						break;
					case enumAPP_TolleranzaFilo:
						break;
					case enumAPP_DiametroLama:				
                        // Chiamo la routine che si incarica di aggiornare la posizione del coltello. 
                        vAggiornaPosizioneColtello();
						break;
					case enumAPP_DiametroRuota1:
						break;
					case enumAPP_DiametroRuota2:
						break;
#if 0						
					case enumAPP_TaraturaPosizioneRiposoTaglio:
						 // Spedisco al dac il comando di posizionamento della lama. 
						 // Vado a modificare la posizione di riposo della lama, non quella di pretaglio!!!
						 spiralatrice.DacOut[addrDacColtello*2]=(long)spiralatrice.actPosLama*FS_DAC_MAX519/100;
						 // Come posizione di pretaglio prendo quella attuale, per comodit� .
  						 spiralatrice.DacOut[addrDacColtello*2+1]=spiralatrice.actPosDacColtello*FS_DAC_MAX519/spiralatrice.CorsaColtello;
						 SendByteMax519(addrDacColtello,
								spiralatrice.DacOut[addrDacColtello*2],
								spiralatrice.DacOut[addrDacColtello*2+1]);

						break;
#endif						
				}
				ValidateMacParms();
				ucSaveMacParms=1;
			}
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enumAPP_NumOfButtons;i++)
				ucCreateTheButton(i); 
			{
			    enumAltriParsProduzione act_param;
			    int button_idx;
			    for (button_idx=0;button_idx<sizeof(buttons_ids)/sizeof(buttons_ids[0]);button_idx++){
				buttons_ids[button_idx]=enumAPP_Disabled;
			    }
			    button_idx=0;
			    for (act_param=(enumAltriParsProduzione)0;act_param<enumAPP_NumOf;act_param++){
   				switch(act_param){
					case enumAPP_CambioBoccola:
					case enumAPP_NumeroSpirePretaglio:
					case enumAPP_TolleranzaFilo:
					case enumAPP_DiametroLama:
					case enumAPP_DiametroRuota1:
					case enumAPP_DiametroRuota2:
					case enumAPP_Log:
					{
					    buttons_ids[button_idx]=act_param;
					    button_idx++;
				            break;
					}
					case enumAPP_TaraturaDistanziatore:
					{
					    if (macParms.uc_distanziatore_type!=enum_distanziatore_type_none){
						buttons_ids[button_idx]=act_param;
						button_idx++;
					    }
					    break;
					}
					case enumAPP_spindle_grinding:
					{
					    if (macParms.uc_enable_spindle_grinding){
						buttons_ids[button_idx]=act_param;
						button_idx++;
					    }
					    break;
					}
					default:
					{
					    break;
					}
				}
			    }
			}
			happ.ucFirstLine=0;
			vValidateTabFornitori();
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enumAPP_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enumAPP_Cut:
							vDoTaglio();
							// non occorre rinfrescare la finestra
							break;

						case enumAPP_Up:
							// se premo up e sono alla prima pagina, torno a menu produzione
							if (happ.ucFirstLine==0){
								ucEnableStartLavorazione(1);
								ucReturnFromWindow();
								break;
							}

							if (happ.ucFirstLine>=defNumRowMenuAPP)
								happ.ucFirstLine-=defNumRowMenuAPP;
							else
								happ.ucFirstLine=0;
							break;
						case enumAPP_Dn:
							// #if (defNumEntriesAltriParsProd<=defNumRowMenuAPP)
								// if (defNumEntriesAltriParsProd<=defNumRowMenuAPP)
									// break;
							// #endif
							if (happ.ucFirstLine+defNumRowMenuAPP<defNumEntriesAltriParsProd){
								happ.ucFirstLine+=defNumRowMenuAPP;
							}
							else{
								happ.ucFirstLine=defNumEntriesAltriParsProd-1;
							}
							if (happ.ucFirstLine+defNumRowMenuAPP>=defNumEntriesAltriParsProd){
							#if 0
								if (defNumEntriesAltriParsProd>=defNumRowMenuAPP)
									happ.ucFirstLine=defNumEntriesAltriParsProd-defNumRowMenuAPP;
							#endif
							}
							break;
						case enumAPP_Esc:
							vHandleEscFromMenu();
							// indico di rinfrescare la finestra
							return 2;

						//case enumAPP_OK:
						case enumAPP_Sx:
						case enumAPP_Dx:
							ucEnableStartLavorazione(1);
							ucReturnFromWindow();
							return 2;
						case enumAPP_button_1:
						case enumAPP_button_2:
						case enumAPP_button_3:
						case enumAPP_button_4:
						case enumAPP_button_5:
						case enumAPP_button_6:
							if (i-enumAPP_button_1>=sizeof(cur_buttons_ids)/sizeof(cur_buttons_ids[0])){
							    break;
							}
							idxParametro=cur_buttons_ids[i-enumAPP_button_1];
							// con prg running non posso cambiare diametri ruote o lama
							if (   PrgRunning
								 &&(  (uiNomiButtonsAltriParsProd[idxParametro]==enumStr20_Diam_lama)
								    ||(uiNomiButtonsAltriParsProd[idxParametro]==enumStr20_Diam_ruota_1)
								    ||(uiNomiButtonsAltriParsProd[idxParametro]==enumStr20_Diam_ruota_2)
								    ||(uiNomiButtonsAltriParsProd[idxParametro]==enumStr20_CambioBoccola)
									)
								)
								break;

							defSetWinParam(defAPP_NumAltroParametro,idxParametro);
							if(idxParametro==enumAPP_TolleranzaFilo){
								// indico che la tabella pu� essere modificata
								ucSetWindowParam(enumWinId_Fornitori,defIdxParam_enumWinId_Fornitori_ModifyEnable,1);
								ucCallWindow(enumWinId_Fornitori);
								// indico di rinfrescare la finestra
								return 2;
							}
							// log produzione???
							else if (idxParametro==enumAPP_Log){
								ucCallWindow(enumWinId_LogProduzione);
								// indico di rinfrescare la finestra
								return 2;
							}
							else if (idxParametro==enumAPP_TaraturaDistanziatore){
								// entro nel menu spaziatore solo se lo spaziatore � abilitato!
								if (macParms.uc_distanziatore_type!=enum_distanziatore_type_none){
									ucCallWindow(enumWinId_TaraDistanziatore);
								}
								// indico di rinfrescare la finestra
								return 2;
							}
							else if(idxParametro==enumAPP_CambioBoccola){
								// protezione: non devo mai poter entrare in cambio boccola con prg running!!!
								if (PrgRunning)
									break;
								vJumpToWindow(enumWinId_CambioBoccola);
								return 2;
							}
							else if (idxParametro==enumAPP_spindle_grinding){
								// protezione: non devo mai poter entrare in rettifica mandrino con prg running!!!
								if (PrgRunning)
									break;
								if (macParms.uc_enable_spindle_grinding){
									ucCallWindow(enumWinId_spindle_grinding);
								}
								return 2;
							}
							else{
								paramsNumK.enumUm=if_APP[idxParametro].enumUm;	
								// imposto limiti min e max
								paramsNumK.fMin=if_APP[idxParametro].fMin;
								paramsNumK.fMax=if_APP[idxParametro].fMax;
								if ((nvram_struct.actUM==UM_IN)&&ucIsUmSensitive(if_APP[idxParametro].enumUm)){
									paramsNumK.fMin*=if_APP[idxParametro].fConvUm_mm_toUm_inch;
									paramsNumK.fMax*=if_APP[idxParametro].fConvUm_mm_toUm_inch;
									paramsNumK.enumUm++;
								}
								paramsNumK.ucMaxNumChar=if_APP[idxParametro].ucLenField;	
								// imposto stringa di picture
								mystrcpy((char*)paramsNumK.ucPicture,(char*)if_APP[idxParametro].ucPictField,sizeof(paramsNumK.ucPicture)-1);
								// titolo che deve comparire...
								//strcpy(paramsNumK.ucTitle,if_APP[idxParametro].ucTitleNumK);
								vStringLangCopy(paramsNumK.ucTitle,if_APP[idxParametro].stringa);
								// copio la stringa con cui si inizializza il numeric keypad
								// sprintf(hw.ucStringNumKeypad_in,if_APP[idxParametro].ucFmtField,if_APP[idxParametro].pc);
								vGetValueParametro_spiwin2(hw.ucStringNumKeypad_in,idxParametro,&if_APP[0]);

								// imposto abilitazione uso stringa picture
								paramsNumK.ucPictureEnable=if_APP[idxParametro].ucPictureEnable;	// abilitazione stringa picture
								// imposto abilitazione uso min/max
								paramsNumK.ucMinMaxEnable=if_APP[idxParametro].ucMinMaxEnable;	// abilitazione controllo valore minimo/massimo
								// chiamo il numeric keypad 
								ucCallWindow(enumWinId_Num_Keypad);
								// indico di rinfrescare la finestra
								return 2;
							}
							break;
					}
				}
			}

			/* Mi procuro un puntatore al record del programma in esecuzione. */
			spiralatrice.pPrgRun=&hprg.theRunning;

			//strcpy(hw.ucString,"   Altri parametri  ");
			vStringLangCopy(hw.ucString,enumStr20_Altri_Parametri);
			//ucPrintStaticButton(hw.ucString,defAPP_title_row,defAPP_title_col,enumFontMedium,enumAPP_Title,defLCD_Color_Green);
			ucPrintTitleButton(hw.ucString,defAPP_title_row,defAPP_title_col,enumFontSmall,enumAPP_Title,defLCD_Color_Trasparente,1);
			for (i=0;i<sizeof(cur_buttons_ids)/sizeof(cur_buttons_ids[0]);i++){
				cur_buttons_ids[i]=enumAPP_Disabled;
			}
			for (i=0;i<defNumRowMenuAPP;i++){
				if (i+happ.ucFirstLine>=sizeof(buttons_ids)/sizeof(buttons_ids[0])){
				    break;
				}
				idxParametro=buttons_ids[i+happ.ucFirstLine];
				if (i<sizeof(cur_buttons_ids)/sizeof(cur_buttons_ids[0])){
					cur_buttons_ids[i]=idxParametro;
				}
				if (idxParametro>=defNumEntriesAltriParsProd)
					break;
				sprintf(hw.ucString,"%-20.20s",pucStringLang(uiNomiButtonsAltriParsProd[idxParametro]));				
				if (   (idxParametro==enumAPP_CambioBoccola)
					 ||(idxParametro==enumAPP_Log)
					 ||(idxParametro==enumAPP_TaraturaDistanziatore)
					 ||(idxParametro==enumAPP_spindle_grinding)
					){
					ucPrintStaticButton(hw.ucString,defAPP_button_1_row+defAPP_Offset_between_buttons*i+4 ,defAPP_button_1_col ,enumFontMedium,enumAPP_button_1+i,defLCD_Color_Trasparente);
				}
				else{
					ucPrintTitleButton(hw.ucString,defAPP_button_1_row+defAPP_Offset_between_buttons*i+4 ,defAPP_button_1_col ,enumFontSmall,enumAPP_descbutton_1+i,defLCD_Color_Trasparente,2);
					switch(idxParametro){
						case enumAPP_NumeroSpirePretaglio:
							sprintf(hw.ucString,"%6u",macParms.nspire_pret);				
							break;
						case enumAPP_TolleranzaFilo:
							sprintf(hw.ucString,"%6.1f",nvram_struct.TabFornitori[pPro->fornitore]);				
							break;
						case enumAPP_DiametroLama:				
							if (nvram_struct.actUM==UM_IN){
								sprintf(hw.ucString,"%6.3f",macParms.diam_coltello*MM_TO_INCH);				
							}
							else{
								sprintf(hw.ucString,"%6.2f",macParms.diam_coltello);				
							}
							break;
						case enumAPP_DiametroRuota1:
							if (nvram_struct.actUM==UM_IN){
								sprintf(hw.ucString,"%6.3f",macParms.diam_frizioni[0]*MM_TO_INCH);
							}
							else{
								sprintf(hw.ucString,"%6.2f",macParms.diam_frizioni[0]);				
							}
							break;
						case enumAPP_DiametroRuota2:
							if (nvram_struct.actUM==UM_IN){
								sprintf(hw.ucString,"%6.3f",macParms.diam_frizioni[1]*MM_TO_INCH);
							}
							else{
								sprintf(hw.ucString,"%6.2f",macParms.diam_frizioni[1]);				
							}
							break;
#if 0							
						case enumAPP_TaraturaPosizioneRiposoTaglio:
							sprintf(hw.ucString,"%6i",(int)spiralatrice.actPosLama);				
							break;
#endif
					}
					ucPrintStaticButton(hw.ucString,defAPP_button_1_row+defAPP_Offset_between_buttons*i ,defAPP_button_1_col+8*21+8 ,enumFontMedium,enumAPP_button_1+i,defLCD_Color_Trasparente);
					// visualizzo unit� di misura, se esiste
					if (if_APP[idxParametro].enumUm!=enumUm_none){
						if ((nvram_struct.actUM==UM_IN)&&ucIsUmSensitive(if_APP[idxParametro].enumUm)){
							sprintf(hw.ucString,"%-8.8s",pucStringLang(if_APP[idxParametro].enumUm+enumStr20_FirstUm+1));				
						}
						else
							sprintf(hw.ucString,"%-8.8s",pucStringLang(if_APP[idxParametro].enumUm+enumStr20_FirstUm));				
						ucPrintTitleButton(hw.ucString,defAPP_button_1_row+defAPP_Offset_between_buttons*i+4 ,defAPP_button_1_col+21*8+8+6*16U+20 ,enumFontSmall,enumAPP_umbutton_1+i,defLCD_Color_Trasparente,0);
					}
				}
			}
			vAddStandardButtons(enumAPP_Sx);

			return 1;
	}
}//unsigned char ucHW_AltriParametriProduzione(void)



// Aggiorna la posizione di spaziatura del coltello
void vAggiornaPosizioneSpaziaturaColtello(void){
// minima posizione coltello basso per distanziazione
#define defMinKnifeDownPosition_mm 0.1
	// Calcolo la somma dei raggi del coltello, del filo e del mandrino meno l' interasse mandrino-coltello; questo valore mi e' utile
	// per tutti i calcoli. 
	spiralatrice.f_raggio_coltello_meno_interasse=macParms.diam_coltello/2.0-macParms.iasse[0];

	// calcolo la posizione del coltello al momento della spaziatura, in mm
	if (spiralatrice.f_raggio_coltello_meno_interasse>hprg.ptheAct->f_coltello_pos_spaziatura_mm+defMinKnifeDownPosition_mm){
		spiralatrice.f_pos_coltello_spaziatura_mm=spiralatrice.f_raggio_coltello_meno_interasse-hprg.ptheAct->f_coltello_pos_spaziatura_mm;
	}
	else
		spiralatrice.f_pos_coltello_spaziatura_mm=defMinKnifeDownPosition_mm;

	// calcolo la posizione del coltello al momento della spaziatura, in a/d step
	sensoreTaglio.ui_pos_coltello_spaziatura_ad_step=spiralatrice.f_pos_coltello_spaziatura_mm/spiralatrice.CorsaColtello*(sensoreTaglio.uiMax_cur-sensoreTaglio.uiMin_cur)+macParms.uiMinPosColtello_StepAD; 

}//void vAggiornaPosizioneSpaziaturaColtello(void)

