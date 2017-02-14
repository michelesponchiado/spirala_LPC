#include "spityp.h"
#include "spicost.h"
#include "az4186_const.h"
#include "az4186_program_codes.h"
#include "az4186_commesse.h"
#include "az4186_spiralatrice.h"
#include "az4186_files.h"

#include "az4186_temperature_compensation.h"


// per simulare l'encoder...
//#define defSimulaEncoder

/* Definizione del tipo di memoria in cui devono essere allocate le variabili
   cui si dovrebbe accedere rapidamente. Se si usa un 8052, o comunque un
   micro che ha spazio per lo stack, allora ho memoria di tipo data a
   disposizione, altrimenti si usa xdat. */
   

#ifdef C52
  #define dat_xdat xdata
#else
  #define dat_xdat xdata
#endif

extern  unsigned char   actSubmenu;
extern  unsigned char lastSubmenu[NUM_MENUS];
extern  unsigned char   actMenu;
extern  unsigned char actMenuBck;
extern  unsigned char actSubmenuBck;
extern   bit   isEdit;
extern   bit updVel;
extern   bit checkPassw;
extern   bit checkedPassw;
extern   bit myTI;
extern   bit watchDog;
extern   bit txTest;
/*  Bit che vale 1 se il lotto corrente � stato terminato correttamente,
    cio� sono stati eseguiti tutti i pezzi richiesti. */
extern   bit RunningPrgEndOk;
/*  Bit che vale 1 se il lotto corrente � stato terminato correttamente,
    ed il prossimo programma pu� essere eseguito in automatico senza
    fermo macchina. */
extern   bit NextPrgAutomatic;
/*  Bit che vale 1 se c' � un programma in esecuzione. */
extern   bit PrgRunning;
/*  Indicatore di esecuzione taglio asincrono. */
extern   bit TaglioAsincrono;
/*  Indicatore di conteggio pezzi prodotti. */
extern   bit BitContaPezzi;

extern  SPEZ_FILO       spezFilo;
extern  SER_RX  serRx;
extern  SER_TX  serTx;

extern  char lastKeyPressed;

//extern  PROGRAMMA prgList[NUM_PROGRAMMI];
extern  COMMESSA commesse[NUM_COMMESSE];
extern unsigned int data outDigVariable;


extern  unsigned char actPrg;
extern  unsigned char runningPrg;

extern  MACPARMS macParms;

extern unsigned char actCan;
/* Variabile che ha il compito di puntare al membro dell' array taratura
   che viene utilizzato per la misura. */
extern TARATURA * xdata pTar;

extern   SERIALE serParms;
extern   bit txNewPz;
extern   bit txModCont;
extern   bit txBusy;
extern   bit txReqLocalCommessa;
extern   bit rxHasMsg;
extern   bit parIsOdd;
extern   bit parErr;
extern   bit isPressed;
extern   bit criticLav;
extern   bit criticMis;

extern   unsigned short myInpDig;
extern   unsigned char data outDig;
extern   unsigned char data outDigMis;

extern   unsigned char misStatus;
extern   unsigned char misStaStatus;
extern   unsigned char avvStatus;
extern   unsigned char backupAvvStatus;
extern   unsigned long lastPzLenImp;
extern   unsigned long targetLenImp;
extern   unsigned long actCorrOhmica;
extern   unsigned long nomRo;
extern   float targetRes;
extern   float impToMm;
extern   float impToUnit;
extern   unsigned short actNumPezzi;
extern   unsigned char rdQueSpez;
extern   unsigned char wrQueSpez;
extern   unsigned short misDig;
extern   float misStaVolt;
extern   unsigned char actPortata;
extern   unsigned char actStaPortata;
extern   unsigned short portate[NUM_PORTATE_DINAMICHE];
extern   unsigned short lastPzRes;
extern   float resSta;

/* Variabili che sarebbe bene mettere in data ram, se possibile. */
extern dat_xdat unsigned long actSpezInPez;  /* 4 byte */
extern dat_xdat short    int   gapImp;       /* 2 byte */
extern dat_xdat unsigned short newEnc;       /* 2 byte */
extern dat_xdat unsigned short oldEncLav;    /* 2 byte */
extern dat_xdat unsigned long actSpezAvv;    /* 4 byte */
extern dat_xdat unsigned long actLenImp;     /* 4 byte */
extern dat_xdat unsigned long resLastSpezUp; /* 4 byte */
/* Resistenza del filo. */
extern dat_xdat unsigned long actRes;        /* 4 byte */
extern dat_xdat unsigned long actRo;         /* 4 byte */
extern dat_xdat unsigned long preLenImp;     /* 4 byte */
extern dat_xdat unsigned short oldEncMis;    /* 2 byte */
extern dat_xdat unsigned long impFromMis;    /* 4 byte */

extern dat_xdat unsigned char cntAvv;        /* 1 byte */
extern dat_xdat unsigned char cntMis;        /* 1 byte */
extern dat_xdat unsigned char misStaSleep;   /* 1 byte */
extern dat_xdat unsigned char upd7segSleep;  /* 1 byte */
extern dat_xdat unsigned char avvSleep;      /* 1 byte */
extern dat_xdat unsigned char misSleep;      /* 1 byte */

		                    /* Totale  46 byte */



extern   unsigned long urpToRes;
extern   unsigned long minRo,maxRo;
extern   unsigned short nSpezDelay;
extern   unsigned short chunkAvv;
extern   float mmRuotaAvv;
extern   bit withMis;
extern   unsigned char actPreTime; /* s/100 */
extern   unsigned char actTagTime; /* s/100 */
#ifndef _CC51
  extern   unsigned int deltaimpulsi;
#endif

/* Numero di filo attualemnte selezionato: variabile usata nella fase
   di visualizzazione/modifica dei dati di programma. */
extern   unsigned char actFilo;
/* Numero di frizione attualmente selezionato: variabile usata nella fase
   di visualizzazione/modifica dei dati di programma. */
extern   unsigned char actFrizione;


#ifndef _CC51
extern unsigned short encValSim;
#else
extern unsigned short encVal1;
extern unsigned short encVal2;
#endif
extern unsigned short encVal;
extern unsigned short* pEncVal;

#ifndef _CC51
extern unsigned short inpDigSim;
#endif

extern unsigned short* pInpDig;

#ifndef _CC51
extern unsigned char outDigSim;
#endif
extern unsigned char* pOutDig;


#ifndef _CC51
extern unsigned short ADCSim;
extern unsigned short ADCStaSim;
extern unsigned char  ADCSim1[10];
extern unsigned char  DACPortSim;
extern unsigned short* pADCDin;
#endif
extern unsigned char* pADC;

extern unsigned short* pADCSta;



extern uint32_t alarms;
extern xdat unsigned char alr_status;

#ifndef _CC51
extern unsigned char kbDataSim;
extern unsigned char kbEnabOut;
#endif
extern unsigned char* pKbData;
extern unsigned char* pKbEnOu;


#ifndef _CC51
extern unsigned char seg7DataSim[10];
extern unsigned char seg7ExtraSim;
#endif
extern unsigned char* p7segData;
extern unsigned char seg7Point;
extern unsigned char* p7segPoint;

extern char str7seg[LEN_BUF_7SEG];
extern char* act7segChar;
extern unsigned char act7segCol;

extern bit isEdit;

typedef struct _TipoStructEdit{
	xdata char* actEditChar;
	xdata char * firstEditChar;
	unsigned char typEdit;
	unsigned char lenEdit;
	unsigned char actEnumEdit;
	#ifdef okEditServe
	   unsigned char okEdit;
	#endif
	/* Variabile che indica il tipo di print da usare: con o senza
	   picture/reverse. */
	unsigned char tipoPrint;
	/* Linea su cui � attivo il picture. */
	unsigned char LineaPicture;
	/* Stringa di picture. */
	char actStrPicture[LEN_LINES_LCD+1];
	/* Stringa di reverse. */
	char actStrReverse[LEN_LINES_LCD+1];

	/* Array dei caratteri da scrivere sull' lcd. */
	char actLcdStr[NUM_LINES_LCD][LEN_LINES_LCD+20];
	/* Riga attuale dell' lcd. */
	unsigned char actRowLcd;
	/* Colonna attuale dell' lcd. */
	unsigned char actColLcd;
	/* Puntatore al carattere corrente dell' lcd. */
	char xdata*	actLcdChar;
	/* Puntatore alla colonna su cui si trova il valore attuale. */
	unsigned char actColLcdVal;
	float tmpfSmartMenu;


}TipoStructEdit;

extern xdata TipoStructEdit edit;



//extern unsigned char xdata newpassw[LEN_PASSWORD+1];

extern unsigned char xdat AlarmOnTimeout;

		                    /* Totale 5 byte */

extern unsigned long  nImpulsi; /* velocita` in impulsi / millisecondo */
extern unsigned long newnImpulsi,oldnImpulsi; /* velocita` in impulsi / millisecondo */
extern float velMMin;       /* velocita` in metri/minuto */
extern unsigned long gapT; /* tempo passato dall'ultimo passaggio in conteggi
			    di numero di interruzioni intrno */
extern unsigned long oldgapT,newgapT ;

extern unsigned long spazio;
extern unsigned long tempo;

extern unsigned short anticipoPretaglio;
/* Stato della lampada: deve essere scrutato/modificato tramite le define
   Lamp... in spicost.h. */
extern unsigned char  LampStatus;
/* Contatore del tempo per il controllo della lampada. */
extern unsigned long  TempoLampada;
extern unsigned int NumSpezzone;
extern float allung;

extern unsigned char rtcSec,rtcMin,rtcHou,rtcBck,rtcDay,rtcMon,rtcYea;


float myatof(char *p);

/* Conversione ascii- floating di n caratteri. */
float mynatof(char *p, unsigned char n);

/* Conversione ascii-integer di n caratteri. */
unsigned short mynatoi(char* str, unsigned char n);

unsigned short myatoi(char* str);

void mymemcpy(char* d, char* s, unsigned char n);


/* Procedura di validazione dei parametri macchina; viene chiamata ogni volta
   che si entra nel menu parametri macchina. */
void ValidateMacParms(void);

/* Procedura di validazione dei parametri del programma attualmente
   in visualizzazione. */
void ValidatePrg(void);

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
void AdjustVelPeriferica(unsigned char vmmChanged);

/* Funzione che, se chiamata, permette di aggiornare la posizione
   di pretaglio e di riposo della lama di taglio. */
void vAggiornaPosizioneColtello(void);


/* Se entro nel menu dei parametri di impostazione, determino
    come devono essere mossi i potenziometri. */
void SetPotenziometri(void);



/* Cancellazione del programma:
   se � il primo della lista, modifica firstPrg
   se � l' unico della lista, firstPrg=0
*/
void DeleteProgramma(tipoNumeroProgramma nPrg);

/* Inserisco il programma nella lista dei programmi, se
   non esiste gi�. */
unsigned char InsertPrg(char * pFirstChar);


/* Routine che inserisce un programma vuoto in memoria, dato il codice
   puntato da pFirstChar. */
void InsertTheProgram(char xdat* pFirstChar);



//
//
//  routines di gestione della commessa
//
//
unsigned char SearchNextCommessa(unsigned char from);



/* Routine che sposta la commessa actCommessa in modo che il suo
   codice_subnum diventi numcommessa-subnum.
   Ritorna 1 se lo spostamento � possibile, 0 altrimenti.
*/
unsigned char ModifyCommCode(unsigned int numcommessa, unsigned char subnum);

/* Routine che cancella la commessa indicata.
   La struttura della commessa viene sbiancata.
   Una commessa vuota pu� essere riconosciuta dal fatto che il suo
   numero (campo numcommessa) vale 0.
*/
void DeleteCommessa(unsigned int nCommessa);

/* Programma di ricerca prossima commessa in lista.
   Ingresso: numero nella tabella delle commesse della commessa attuale.
   Se la commessa attuale e' l' unica in lista ne restituisce il numero.
*/
unsigned char SearchNextCommessa(unsigned char from);

/* Programma di ricerca precedente commessa in lista.
   Ingresso: numero nella tabella delle commesse della commessa attuale.
   Se la commessa attuale e' l' unica in lista ne restituisce il numero.
*/
unsigned char SearchPrevCommessa(unsigned char from);

/* Routine che inizializza il valore di uiGetActPrg() in funzione di quello
   di actCommessa.
*/
extern void vInitActPrg(void);

/* Funzione che serve per creare la rampa di aggiornamento uscite dac.
   I valori delle uscite vanno scritti nell' array DacOutValue[];
   i valori MASSIMI delle uscite vanno scritti nell' array DacOutMaxValue[];
   la funzione provvede ad effettuare il clipping al fondo scala dell' adc.
   Ingresso: l' indirizzo del dac cui spedire il dato.
   Il valore in uscita dal dac viene aggiornato solo se il valore attuale
   � diverso da quello proposto!
   Uscita: nulla.
*/

extern void SetDacValue(unsigned char DacOutIndex);

/* Funzione che regola le uscite dei dac in base ai valori letti dai potenziometri. Viene usata in diagnostica
   per pilotare mandrino e ruote di contrasto. */
extern void WriteOnDacByPot(void);

extern TipoStructHandlePrg xdat hprg;
unsigned int uiGetActPrg(void);
unsigned int uiGetRunningPrg(void);
unsigned int uiSetActPrg(tipoNumeroProgramma uiNewValueActPrg);
unsigned int uiSetRunningPrg(tipoNumeroProgramma uiNewValueRunningPrg);
void vInitProgramList(void);
void vHandleBackgroundNandFlash(void);
unsigned char ucSaveRunningPrg(void);
unsigned char ucSaveActPrg(void);

/* Variabili e define per gestione potenziometri. */
#define CheckPotMaggioreUguale 1
#define CheckPotMinoreUguale   2
/* Array dei valori letti dall' adc. */
extern xdat long int InputAnalog[NUM_INP_ANALOG];
/* Tipo della variabile che indica se vi deve essere una partenza "lenta"
   del programma: il valore della variabile ne specifica il motivo.
*/
/* Vale 1 se vi e' stato un allarme per cui la commessa deve
   partire con basso numero di rpm
*/
extern unsigned char xdat actIasse;

void doEdit(unsigned char key);
// routine che serve ad inizializzare puntatori per la stampa di un valore sulla seconda riga del display lcd
void prtVal(void);
void stopEdit(void);
// clear lcd...
void vClearLcd(void);


void startEdit(void);

void startPrintLcd(	unsigned char flags, 
							unsigned char line1Str, unsigned char line2Str, 
							unsigned char index1,   unsigned char index2, 
							unsigned char subIndex1
						  );
/* Routine che permette di inserire una stringa allineata a destra
   nella linea line dell' lcd. */
void AtRightLine(unsigned char line2str, unsigned char line);
// Routine che permette di inserire un numero allineato a destra
//   nella linea line dell' lcd.
//   Il numero deve essere compreso fra 0 e 99.
void AtRightLineNumber(unsigned char number, unsigned char line);
/* Copia di una stringa rom sul display. */
void RomStringAt(unsigned char enum_str,unsigned char line,unsigned char col);
// Funzione che paragona due password e verifica che siano uguali.
//   Ritorna 1 se sono uguali, 0 altrimenti.
bit cmpPassw(unsigned char xdat *newpassw,unsigned char xdat *password);
// Posiziona un simbolo che indica a quale campo ci si riferisce. 
void SegnaCampo(unsigned char col,unsigned char line,unsigned char Dx0Sx1);
extern unsigned char xdat backCampoChar;
extern unsigned char xdat backCampoCol;
extern unsigned char xdat backCampoLine;
/* Define che permette di stampare una stringa all' inizio della seconda
   linea del display specificandone il formato e il valore da stampare. */
#define At2(F,N) sprintf(&edit.actLcdStr[1][edit.actColLcdVal],F,N)

#define At2Adjust(F,N) sprintf(firstAdjChar,F,N)

/* Define che permette di stampare una stringa all' inizio del campo di edit
   della seconda linea del display specificandone il formato e il valore
   da stampare. */
#define AtEditChar(F,N) sprintf(edit.firstEditChar,F,N)
/* Macro che permette di spegnere il cursore. */
#define CursorOff   vSetCursorOff();
extern int iArrayConvAD7327[];


void vUpdateStringsOnLcd(void);
// posizionamento del testo alla linea, colonna indicata
void vSetTextRigaColonna(unsigned char ucRiga0_1,unsigned char ucColonna);
// cursore...
void vSetCursorOff(void);
// cursore...
void vSetCursorOn(void);

// stampa una stringa alla riga/colonna corrente
// es  " " o "*"
void vPrintDirectString(const char * pc);
void vPrintDirectChar(unsigned char uc);

void leggiDataOra(void);
void scriviDataOra(void);


void vTestAD7327(void);
void strromcat(unsigned char i);
void inpToOut(void);

/* Indica se la macchina e' appena stata accesa. */
extern xdat unsigned char AppenaAcceso;


extern xdat unsigned long ulMaxTrueRo;
extern xdat unsigned long ulMinTrueRo;
extern xdat unsigned char ucMaxBadSpezzConsec;
extern xdat unsigned char ucActBadSpezzConsec;


/* Valore della resistenza usata per la taratura
   in una sequenza di commesse in automatico.
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
extern xdat float fResTaraturaFine;
/* Valore del delta R di resistenza nella misurazione
   fatta prendendo la spirale alle estremita'.
*/
extern xdat float fResDeltaR;

typedef enum{
			let_newspezzone=0,
			let_cut,
			let_bad_measure,
			let_spezzone_alarm,
			let_numof
		}enumLogEventType;

typedef struct _TipoStructLog_newSpezzone{
	unsigned long ulRo;
	unsigned long ulNomRo;
	TipoStructTempCompStats temperature_compensation_stats;
}TipoStructLog_newSpezzone;

typedef struct _TipoStructLog_Cut{
	unsigned long ulCutImp;
	unsigned long ulExpectedImp;
}TipoStructLog_Cut;

typedef struct _TipoStructLog_Bad_Measure{
	unsigned long ulRo;
	unsigned long ulNomRo;
}TipoStructLog_Bad_Measure;

typedef struct _TipoStructLog_Spezzone_Alarm{
	unsigned long ulRo;
	unsigned long ulNomRo;
}TipoStructLog_Spezzone_Alarm;

typedef struct _TipoRecordLog{
	enumLogEventType event_type;
	unsigned short misDig;
	union{
		TipoStructLog_newSpezzone newSpezzone;
		TipoStructLog_Cut	cut;
		TipoStructLog_Bad_Measure bad_measure;
		TipoStructLog_Spezzone_Alarm spezzone_alarm;
	}u;
}TipoRecordLog;

// numero max di record del file di log
#define defMaxRecordLog 50
typedef struct _TipoStructLog{
	TipoRecordLog datas[defMaxRecordLog];
	unsigned char ucIdx;
}TipoStructLog;

extern xdata TipoStructLog production_log;
extern xdata TipoRecordLog *plog;

extern xdat unsigned long TempoRefreshRampa;
extern xdat unsigned long TempoTaglioPezzo;
extern xdat unsigned long spaziorv;
extern xdat unsigned long temporv;
extern xdat unsigned int NumPezziScarto;
/* Numero di pezzi fatti prima della partenza di una commessa. */
extern xdat unsigned int oldndo;

/* Indicatore del fatto che il primo pezzo prodotto dalla macchina
   debba essere eliminato. */
extern xdat unsigned char PrimoPezzoEliminato;

/* Vale 1 se i potenziometri hanno il lucchetto, 0 altrimenti. */
extern xdat unsigned char AbilitaModificaPotenziometri;

/* Variabili per il test della macchina. */
extern xdat unsigned char actSpezzoneTest;
extern xdat unsigned char actPezzoTest;

/* Backup valore degli allarmi. */
extern xdat uint32_t oldalarms;
/* Timer della durata dell' allarme. */
extern xdat unsigned long TempoAllarme;
extern xdat unsigned char nextRunCommessa;
extern xdat unsigned char nextRunPrg;
extern xdat unsigned char OkMisStatica;
/* Indica se la velocita' di una commessa deve essere clippata al valore
   massimo indicato a programma. */
extern xdat unsigned char ucClipVelocity;
/* Valore mediato della misura dall' ohmetro statico. */
extern xdat float resStaMediata,resStaMediataAux;
extern xdat float oldresStaMediata;
extern xdat unsigned int nSampleResSta;

/* Numero massimo di fornitori. */
#define MAXFORNITORI MAX_FORNI+1
/* Tipo: variazione della resistenza specifica del filo del fornitore. */
typedef float TipoVarFornitore;

// numero max di caratteri della descrizione di un lavoro
#define defMaxCharDescrizioneLavoro 20
// codice che identifica un record lavoro valido
#define defLavoroValidoKey 0xa5
// codice che identifica un record lavoro NON valido
#define defLavoroNonValidoKey 0x33

// struttura che definisce un lavoro
typedef struct _TipoLavoro{
	unsigned long ucPosition;  // posizione nell'array
	unsigned long ucValidKey;  // chiave che identifica che il record � valido...
	unsigned long uiNumeroPezzi; // numero di pezzi da produrre
	float fOhm;					// valore ohmico da produrre
	unsigned char ucDescrizione[defMaxCharDescrizioneLavoro+1];	// descrizione
	unsigned long npezzifatti;
	unsigned long magg;
}TipoLavoro;

// numero max di lavori in lista
#define defMaxLavoriInLista 20
//#define defMaxLavoriInLista 4
typedef struct _TipoStructLavori{
	// lista dei lavori da eseguire
	TipoLavoro lista[defMaxLavoriInLista];
	TipoLavoro appoggio;
	unsigned long ucWorkingMode_0L_1LR;
	unsigned long ucNumLavoriInLista;
	unsigned long ucNumFirstLavoroVisualizzato;
	unsigned long ucAuxString[12];
}TipoStructLavori;


// definizione dell'elemento base della struttura che contiene la lista dei codici prodotto
typedef struct _TipoStructCodeJobList{
	unsigned long ucManual;			// vale 1 se codice proviene da inserimento manuale (diretto), 0 altrimenti
	TipoStructLavori jobs;			//lista lavori associata al codice
	unsigned char ucCodice[defCodice_MaxChar+4];// codice prodotto
}TipoStructCodeJobList;

extern unsigned char ucSaveCodice_his[defCodice_MaxChar+1];// codice prodotto

// numero max di codici in lista
#define defMaxCodeJobList 5
// definizione della struttura che contiene la lista dei codici prodotto
typedef struct _TipoStructCodeList{
	unsigned long ucNumElem;				// numero di elementi in lista
	unsigned long ucIdxActiveElem;		    // indice dell'elemento attivo
	TipoStructCodeJobList codeJobList[defMaxCodeJobList];
	unsigned long uiValidKey_0xa536;			// chiave per identificare che la struttura � valida (per inizializzazione da versioni precedenti)
	unsigned long uiCrc;						// crc per verificare che la struttura sia corretta
}TipoStructCodeList;


// struttura che contiene tutte le informazioni non volatili della spiralatrice
typedef struct _TipoNonVolatileRamStruct{
	TipoVarFornitore TabFornitori[MAXFORNITORI];
    unsigned char actLanguage; 
    unsigned char actUM; 
	unsigned char ucImAmImMenuBoccola_0x37;
	unsigned char ucIwasWorkingOnMenuProduzione_0xa5;

	// Resistenza specifica delle due bobine di filo. 
	float resspec[MAX_NUM_FILI]; 
    // Commesse
    COMMESSA commesse[NUM_COMMESSE];
	// lavorazioni...
    TipoStructCodeList codelist;
	unsigned char ucComingToControlloProduzioneFromJobs;
}TipoNonVolatileRamStruct;

// variabile che contiene tutte le informazioni non volatili della spiralatrice
extern TipoNonVolatileRamStruct nvram_struct;

/* Codice corrente del fornitore. */
extern xdat unsigned char actFornitore;

/* Campo corrente nella finestra. */
extern unsigned char xdat actField;
/* Interasse nella finestra. */
extern unsigned char xdat actIasse;
/* Salvataggio del carattere occupato dal segnacampo. */
extern unsigned char xdat backCampoChar;
extern unsigned char xdat backCampoCol;
extern unsigned char xdat OkAbilitaTaraturaStat;
/* Indica se e' abilitata la taratura "fine" (i.e. prendendo le estremita'
   della spirale) mediante ohmetro statico.
*/
extern unsigned char xdat OkAbilitaTaraFineStat;
/* Indicatori che stabiliscono se la print a video va fatta
   usando l' indicazione sul diretto/reverse e sul formato dei caratteri. */
#define PrintWithoutPicture  0
#define PrintWithPicture     1

/* Backup menu e sottomenu. */
extern unsigned char xdat actMenuBck1;
extern unsigned char xdat actSubmenuBck1;
extern unsigned char xdat actMenuBckPassw;
extern unsigned char xdat actSubmenuBckPassw;
/* Backup menu per allarmi. */
extern unsigned char xdat actMenuBckAlr;
extern unsigned char xdat actSubmenuBckAlr;

/* Flag di modifica codice commessa. */
extern unsigned char xdat isModifyCodComm;


/* Valore del prossimo canale da leggere per aggiornare gli
   ingressi analogici. */
extern xdat unsigned char numCanaleUpdate;
extern xdat unsigned char misUpdateStatus;
extern xdat unsigned char misUpdateSleep;
/* Valore della misura letta dall' adc per agg.re gli ingressi analogici. */
extern xdat unsigned long  misUpdate;
/* Numero di misure effettuate dai canali che danno l' assorbimento
   delle ruote di contrasto. */
extern xdat char nMisAssorb;
/* Appoggio per il calcolo della media degli assorbimenti
   delle ruote di contrasto. */
extern xdat long int auxMediaCanaleAssorb;
/* Numero di campioni che vengono prelevati per effettuare la media degli
   assorbimenti delle ruote di contrasto. */
#define N_SAMPLE_CAN_ASSORB  16

/* Indicatore di taratura in corso. */
#define TaraOffset      1
#define TaraGuadagno    2
#define TaraNulla       0
extern xdat unsigned char TaraPortate;
extern xdat unsigned char ChangePortata;
extern xdat unsigned char actTaraPortata;
extern xdat unsigned char actCanPortata;
extern xdat unsigned char actPosLama;


void menuOperation(unsigned char op);
void TxCommessa(unsigned int code);
void DatiInizioCommessa(void);
unsigned char SearchNextCommessa(unsigned char from);
extern xdat unsigned char StartPortata;
// contatore che viene incrementato ad ogni interrupt
extern volatile unsigned char xdata ucCntInterrupt;


#define defMaxChrStringLcd 20
typedef struct _TipoStructStringheLCD{
	unsigned char ucActRow;
	unsigned char ucActCol;
	unsigned char ucCursorOn;
	unsigned char ucCursorRow,ucCursorCol;
	unsigned char ucForceARefresh;
	unsigned char ucOldCursorRow,ucOldCursorCol;
	char ucString[15][defMaxChrStringLcd+5];
	char ucSaveString[15][defMaxChrStringLcd+5];
	unsigned char ucTest,ucNeedRefresh;
	unsigned char ucToggle,ucToggle2;
	float fPrintRes,fPrintVel;
	unsigned char ucSuspendDigitalIoUpdate;
	unsigned char ucAttivataMisuraDinamica;
	unsigned long ulInfoAlarms[4];
	// versione del firmware lcd
	unsigned char FW1,FW2;
	unsigned char ucPasswordChecked;
}TipoStructStringheLCD;

extern xdata TipoStructStringheLCD sLCD;
extern long xdata tagliaMoncone_Imp;

extern xdat unsigned char ucSaveMacParms;
extern xdat PROGRAMMA auxPrg;
extern int iReadCounter_2(void);
extern int iReadCounter(void);

// aggiornamento dei valori di taratura...
// da chiamare quando procedura di taratura finisce
void vUpdateValoriTaratura(void);

unsigned char ucLcdRefreshTimeoutReset(void);
unsigned char ucLcdRefreshTimeoutExpired(void);
unsigned char ucLcdRefreshTimeoutEnable(unsigned int uiMilliseconds);
unsigned int uiLcdRefreshTimeout_GetAct(void);


int iInitReadCounter_2(void);
int iAzzeraReadCounter_2(void);

// imposta i parametri di interfaccia numeric keypad per chiedere la password
void vSetNumK_to_ask_password(void);
// Password passe_partout. 
#define passw_passe_partout 93629L

unsigned char ucHW_inserisciLottoDiretto(void);

// mette in uppercase la stringa
void vStrUpperCase(unsigned char xdata *pdst);
// copia stringa al max n caratteri aggiungendo blank a destra se servono
void vStrcpy_padBlank(unsigned char xdata *pdst,unsigned char xdata *psrc,unsigned char ucNumChar);

// riempie la stringa di n caratteri blank
void vStr_fillBlank(unsigned char xdata *pdst,unsigned char ucNumChar);


// copia stringa al max n caratteri togliendo blank a destra e sinsitra
void vStrcpy_trimBlank(unsigned char xdata *pdst,unsigned char xdata *psrc,unsigned char ucNumChar);



// funzione che stampa a video un testo
// ucCentered.0=1--> centrato
// ucCentered.1=1--> non togliere blank a sinistra e destra della stringa, per default li toglie
// ucCentered.2=1--> non mettere in uppercase
// ucCentered.3=1--> usa pulsante rosso degli allarmi
unsigned char ucPrintTitleButton(	xdata char *stringa,
									TipoPosizioneDisplayRigaFont row, 
									TipoPosizioneDisplayColonnaFont col,
									enumFontType fontType,
									unsigned char ucNumButton,
									unsigned int uiColor,
									unsigned char ucCentered
								);

// routine che permette di aggiungere ad una finestra i bottoni standard sx, dx, up, dn, esc, cut
void vAddStandardButtons(unsigned char ucFirstButtonNum);

unsigned char ucEnableStartLavorazione(unsigned char ucEnable);
unsigned char ucIsDisabledStartLavorazione(void);
unsigned char ucIsEnabledStartLavorazione(void);

unsigned char ucHW_CambioBoccola(void);
unsigned char ucHW_CalibraSensore(void);


typedef enum{
	enumPT_unsignedChar=0,
	enumPT_signedChar,
	enumPT_unsignedInt,
	enumPT_signedInt,
	enumPT_unsignedLong,
	enumPT_signedLong,
	enumPT_float,
	enumPT_string,
	enumPT_unsignedShort,
	enumPT_signedShort,
	enumPT_NumOf
}enumParameterType;

typedef enum{
	enumStringType_none=0,
	enumStringType_SelezionaModoFunzionamento,
	enumStringType_SelezionaUnitaMisura,
	enumStringType_SelezionaLingua,
	enumStringType_SelezionaEncoderPotenziometri,
	enumStringType_SelezionaCommesseAutomatiche,
	enumStringType_SelezionaQuintaPortata,
	enumStringType_UsaSensoreTaglio,
	enumStringType_AbilitaLampadaAllarme,
	enumStringType_SelezionaDistanziatore,
	enumStringType_AbilitaCompensazioneTemperatura,
	enumStringType_spindle_grinding_enable,
	enumStringType_check_wire_tangled_enable,
	enumStringType_NumOf

}enumStringType;

typedef enum{
	enumStatiAnalizzaSensTaglio_Idle=0,
	enumStatiAnalizzaSensTaglio_WaitMin,
	enumStatiAnalizzaSensTaglio_WaitMax,
	enumStatiAnalizzaSensTaglio_NumOf
}enumStatoAnalizzaSensTaglio;

typedef enum {
	enumOperazioneLama_Idle=0,
	enumOperazioneLama_Pretaglio_Init,
	enumOperazioneLama_Pretaglio,
	enumOperazioneLama_Taglio_Init,
	enumOperazioneLama_Taglio,
	enumOperazioneLama_WaitTaglioLow,
	enumOperazioneLama_WaitTaglioHi_0,
	enumOperazioneLama_WaitTaglioHi_1,
	enumOperazioneLama_WaitTaglioOk,
	enumOperazioneLama_DownSlow_Init,
	enumOperazioneLama_DownSlow,
	enumOperazioneLama_DownFast_Init,
	enumOperazioneLama_DownFast,
	enumOperazioneLama_UpFast_Init,
	enumOperazioneLama_UpFast,
	enumOperazioneLama_BackgroundCtrl,
	enumOperazioneLama_Distanzia_Init,
	enumOperazioneLama_Distanzia,
	enumOperazioneLama_Taglio_Distanzia,

	enumOperazioneLama_NumOf
}enumOperazioneLama;


typedef enum{
	enumCiclicaLama_taglia,
	enumCiclicaLama_pretaglia,
	enumCiclicaLama_scendi_lenta,
	enumCiclicaLama_scendi_veloce,
	enumCiclicaLama_sali,
	enumCiclicaLama_lenta,
	enumCiclicaLama_veloce,
	enumCiclicaLama_distanzia,
	enumCiclicaLama_distanzia_forced,
	enumCiclicaLama_pretaglia_distanziatore,
	enumCiclicaLama_numOf
}enumCiclicaLama;


void vAttivaOperazioneLama(enumCiclicaLama op);
extern unsigned char ucHW_LogProduzione(void);
extern void vHandleEscFromMenu(void);
unsigned char ucIsEmptyStackCaller(void);


typedef struct _TipoSensoreTaglio{
	int uiMin_cur;
	int uiMax_cur;
	int uiPosRiposo;
	int uiPosRiposo2;
	int iTolerance;
	int uiPosPretaglio;
	float fPosPretaglio;
	float fSlowDacColtelloVolt,fGainDacColtelloVolt;
	unsigned int uiTimeout;
	int uiLetturaSensoreTaglio;
	unsigned int uiNumRead;
	unsigned int uiAlto;
	unsigned int uiPretaglio;
	unsigned int uiSetPoint;
	unsigned int uiKp;
	signed long lAux;
	signed int iError;

	unsigned char ucEnableTest;
	unsigned char ucFilter;
	unsigned char ucSaturaDac;
	unsigned char ucSaturaDac_value;
	unsigned char ucUpdateDac;
	signed int iDacValue;
	unsigned char ucDacValue;
	unsigned char ucGoToFastMode;
	unsigned char ucWaitInizializzazioneLama;
	enumStatoAnalizzaSensTaglio stato;
	enumOperazioneLama stato_taglio;
	unsigned int ui_pos_coltello_spaziatura_ad_step;
	unsigned char locked;
	enumCiclicaLama onlyCycleAdmitted;
	unsigned char ucLastOpWasOnDistanziatore;
	unsigned int uiLastTimeDistanziaNumT,uiCurTimeDistanziaNumT;
	unsigned int ui_on_the_wrong_cut_side;
}TipoSensoreTaglio;
extern xdata TipoSensoreTaglio sensoreTaglio;


unsigned int uiGetSensoreTaglio_NumRead(void);
unsigned int uiGetSensoreTaglio_Min(void);
unsigned int uiGetSensoreTaglio_Max(void);
void vHaltReadCycleSensoreTaglio(void);
void vInitReadCycleSensoreTaglio(void);
void vInitSensoreTaglio(void);
void vLockOperazioneLama(enumCiclicaLama op);
void vUnlockOperazioneLama(void);

extern xdata float fArrayConvAD7327_Volts[8];
extern data unsigned char outDigMis_Hi;


//extern xdata unsigned char Outputs_Hi ;
extern void vVisualizeSplashScreen(void);
extern unsigned char ucVisualizeSplashScreen(void);
extern void vDoTaglio(void);
extern unsigned char ucHW_YesNo(void);
extern unsigned char ucHW_MainMenu(void);
extern unsigned char ucHW_SaveParams(void);
// inizializzazione della macchina a stati che permette il salvataggio dei parametri della macchina...
extern void vInitMachineParsOnFile_Stati(void);

// salvataggio di tutti i parametri macchina su file...
// ritorna 0 se gestione finita
// ucMode=0--> usb
// ucMode=1--> ethernet
extern unsigned char ucSaveMachineParsOnFile_Stati(unsigned char ucMode);

// restituisce il codice di uscita della procedura di salva parametri: 
// 0=tutto ok
// 1=error communicating with usb disk
// 2=usb disk command failed
// 3=error opening param file for writing
// 4=error writing on param file
int iSaveMachineParsOnFile_GetExitCode(void);
// ritorna la percentuale di salvataggio correntemente eseguita
float fSaveMachineParsOnFile_GetEstimatedDonePerc(void);

#define defMaxCharTitle 20
#define defMaxCharMeasure 10
#define defMaxCharScala 30

#define defMaxCharTitleAlarm 20
#define defMaxCharDescAlarm 40

typedef struct _TipoStructHandleMeasure{
	unsigned char ucTitle[defMaxCharTitle+1];
	unsigned char ucMeasure[defMaxCharMeasure+1];
	unsigned char ucScala[defMaxCharScala+1];
	unsigned char ucLiving;
	unsigned char ucActivated;
	unsigned char ucTitle_alarm[defMaxCharTitleAlarm+1];
	unsigned char ucDesc_alarm[defMaxCharDescAlarm+1];
	unsigned char ucDesc_alarm2[defMaxCharDescAlarm+1];
	unsigned char ucDesc_alarm3[defMaxCharDescAlarm+1];
	unsigned char ucDesc_alarm4[defMaxCharDescAlarm+1];
	unsigned char ucAlarmStatus;
}TipoStructHandleMeasure;

extern xdata TipoStructHandleMeasure hM;
extern void vInizializzaCodiceProdotto(void);
extern unsigned char ucHW_LoadParsMac(void);
extern unsigned char ucHW_DataOra(void);
extern unsigned char ucHW_spindle_grinding(void);

typedef enum{
	all_ok_vdip						=0,
	err_vdip_openfile_for_writing	=1,
	err_vdip_diskerr_on_writing		=2,
	err_vdip_no_response_on_writing	=3,
	err_vdip_diskerr_writing		=4,
	err_vdip_file_notexists			=5,
	err_vdip_file_is_empty			=6,
	err_vdip_openfile_for_reading	=7,
	err_vdip_diskerr_while_reading	=8,
	err_vdip_bad_row_syntax			=9,
	err_vdip_bad_params_version		=10,
	err_vdip_bad_language_row_format=11,
	err_vdip_bad_serial_number		=12,
	err_vdip_num_of
	}enum_VdipDisk_err_codes;

extern void vLoadMacParms(void);
extern unsigned char ucHW_LoadCodiciProdotto(void);
extern unsigned char ucHW_LoadCustomLanguage(void);
extern unsigned char ucHW_ResetCodiciProdotto(void);
extern unsigned char ucHW_Distanziatore(void);

unsigned char xdata *pRead_custom_string20(unsigned long ulNumStringa,unsigned char xdata *p);
// funzione di scrittura di una stringa custom da 20 caratteri
unsigned char xdata *pWrite_custom_string20(unsigned long ulNumStringa,unsigned char xdata *p);
// funzione di lettura di una stringa custom da 40 caratteri
unsigned char xdata *pRead_custom_string40(unsigned long ulNumStringa,unsigned char xdata *p);
// funzione di scrittura di una stringa custom da 40 caratteri
unsigned char xdata *pWrite_custom_string40(unsigned long ulNumStringa,unsigned char xdata *p);
// funzione di lettura di un parametro privato
unsigned char xdata *pRead_private_parameter(unsigned long ulNumStringa,unsigned char xdata *p);
// funzione di scrittura di un parametro privato
unsigned char xdata *pWrite_private_parameter(unsigned long ulNumStringa,unsigned char xdata *p);
// serial number del box..
extern unsigned long xdata ulBoxSerialNumber;
extern unsigned char ucSuperUser_passwordChecked;
typedef enum{
	enum_private_param_box_serial_number,
	enum_private_param_numOf
}enum_private_parameters;

unsigned char ucIamInTaraturaDinamica(void);
unsigned char ucIamInTaraturaStatica(void);
// fa saltare direttamente alla pagina di produzione...
void vSaltaSuPaginaProduzione(void);
void vInizializzaCodiceProdotto(void);
unsigned char ucVerifyPasswordCorrect(unsigned char xdata *pnewpassword);

unsigned char ucVerifyPasswordSuperUserCorrect(unsigned char xdata *pnewpassword);
extern float fCalcCoeffCorr(void);
extern xdata unsigned char ucHoldMisuraStatica;
void vAddStandardButtonsOk(unsigned char ucFirstButtonNum);
unsigned char ucAlignJobsRunningAndSelected(void);
// funzione per impostare un bit della parte alta dei digtal outputs
// passare come parametro la maschera definita dalle defines odg_...
void vSetBitOutputsHi(unsigned int uiMaskOutputsHi);
// funzione per impostare un bit della parte bassa dei digtal outputs
// passare come parametro la maschera definita dalle defines odg_...
void vResetBitOutputsHi(unsigned int uiMaskOutputsHi);

// stringa di picture della password
// primo carattere deve essere 1..9
// poi devono seguire 4 caratteri non numerici
#define defPictureString_Password "snnnn"

typedef enum{
	enumFlagsFileModified_Anybus=0,
	enumFlagsFileModified_All,
	enumFlagsFileModified_NumOf
}enumFlagsFileModified;


// mi dice se un certo tipo di file � stato modificato...
unsigned char ucHasBeenFile_Modified(enumFileTypes fileType, enumFlagsFileModified enumFlag);
// set/reset che un certo tipo di file � stato modificato...
unsigned char ucSignalFile_Modified(enumFileTypes fileType, unsigned char ucSet1_Reset0, enumFlagsFileModified enumFlag);
typedef enum{
	savePars_return_op_none=0,
	savePars_return_op_init,
	savePars_return_op_openfile,
	savePars_return_op_printfile,
	savePars_return_op_inputfile,
	savePars_return_op_closefile,
	savePars_return_op_numOf
}savePars_return_op;
typedef struct _TipoStructSavePars_StatusInfo{
	savePars_return_op op;
	char *pc2save;
	unsigned char ucDataReady;
}TipoStructSavePars_StatusInfo;
extern xdata TipoStructSavePars_StatusInfo spsi;
// recupero di tutti i codici prodotto salvati su chiavetta usb...
// ritorna 0 se gestione finita
// ucMode=0 --> carica da usb
// ucMode=1 --> carica da ethernet
unsigned char ucLoadCodiciProdottoOnFile_Stati(unsigned char ucMode);
void vCodelistTouched(void);
void vLogStopProduzione(void);
void vLogAlarm(void);
/* Routine che controlla
   - l' immissione di tasti;
   - start/stop della macchina;
   - misura statica e taratura dell' ohmetro della misura statica
*/

typedef struct _TipoStructAnalyzeInputEdge{
	unsigned char ucActualValue;
	unsigned char ucPreviousValue;
	unsigned char ucPositiveEdge;
	unsigned char ucNegativeEdge;
}TipoStructAnalyzeInputEdge;

typedef enum{
	enumInputEdge_stop=0,
	enumInputEdge_start,
	enumInputEdge_numOf
}enumInputEdge;

extern xdata TipoStructAnalyzeInputEdge aie[enumInputEdge_numOf];
unsigned char ucDigitalInput_PositiveEdge(enumInputEdge ie);
unsigned char ucDigitalInput_AtHiLevel(enumInputEdge ie);
unsigned char ucDigitalInput_AtLowLevel(enumInputEdge ie);
unsigned char ucDigitalInput_NegativeEdge(enumInputEdge ie);
void vRefreshInputEdge(enumInputEdge ie,unsigned int uiNewValue);
unsigned long get_ul_timer_interrupt_count(void);

// routine che prepara nella stringa pDst
// la descrizione formattata dell'evento di log produzione puntato da pRL
// se idx=0xFF allora NON vengono usati i parametri ucActSpez e idx
// se idx!=0xFF allora ucActSpez indica se questa � la riga corrente, mentre idx � l'indice riga nella finestra
void vPrepareDesc_ProductionLog(xdata char *pDst,TipoRecordLog xdata *pRL,unsigned char ucActSpez,unsigned char idx);
// indica se richiesta comando remoto attiva
unsigned char ucRemoteCommandAsking(void);

// indica richiesta comando remoto accettata/non accettata
void vRemoteCommandAskingReply(unsigned char ucAccepted);
// recupero di tutti i parametri macchina su file...
// ritorna 0 se gestione finita
// ingresso: 0-->carica da usb
//           1-->carica da ethernet
unsigned char ucLoadParMacOnFile_Stati(unsigned char ucMode);

// PARAMETRI DELLA FINESTRA SETUP PRODUZIONE

// parametro in cui � contenuto il numero del button che chiama la keypad...
#define defWinParam_PROD_NumCampo 0
// parametro che indica se c'� stato un cambio programma...
#define defWinParam_PROD_AutomaticProgramChange 1
// primitive per richiedere sospensione operazioni anybus...
// in modo da evitare conflitti tra operazioni anybus e salvataggio/ripristino codici prodotto e parametri da usb
unsigned int uiHasBeenAckRequestSuspendAnybusOp(void);
unsigned int uiRequestSuspendAnybusOp(unsigned char ucRequest);
// Aggiorna la posizione di spaziatura del coltello
void vAggiornaPosizioneSpaziaturaColtello(void);



// normalizzazione intera di un valore moltiplicato per una percentuale di distanziazione
long l_multiply_long_perc_dist(long theLong, unsigned short int us_perc_dist);
// macro che converte un perc_dist in ingresso, nell'intervallo 0.0 .. defMaxValuePercDistanzia, 
// in un numero compreso fra 0 e 100
float f_convert_perc_dist_to_float_0_100(unsigned short int perc_dist);
// macro che converte un float in ingresso, nell'intervallo 0.0 .. 100.0, in un numero compreso fra 
// 0 e defMaxValuePercDistanzia
unsigned short int us_convert_float_0_100_to_perc_dist(float f_0_100_value);

// spindle the spindle to ramp down until it stops
void request_spindle_stop(void);

// struttura che permette di contenere i valori temporanei memorizzati nel menu 
// (numero di spire, valore ohmico iniziale e finale)
typedef struct _TipoStructDistInfo{
		enum_perc_distanzia_type perc_distanzia_type;
		unsigned int ui_num_of_coil_windings;	// numero di avvolgimenti
		float f_ohm_iniz;						// valore ohmico iniziale
		float f_ohm_fin;						// valore ohmico finale
		float f_perc_iniz;						// percentuale iniziale
		float f_perc_fin;						// percentuale finale
}TipoStructDistInfo;

typedef struct _TipoStructDist{
		TipoStructDistInfo info[defMaxNumOfPercDistanzia]; // info sulle varie distanziazioni
		unsigned char uc_num_valid_entries;			// numero di entries valide
		unsigned short int usDelayDistanziaDownMs;	// tempo necessario al coltello per scendere in posizione di distanziazione
}TipoStructDist;

// struttura che contiene info su distanziazioni
extern TipoStructDist dist;

// enumerazione delle possibili conversioni dei dati del distanziatore
// * da percentuale a valore ohmico e numero di spire
// * da valore ohmico a percentuale e numero di spire
// * da numero di avvolgimenti a valore ohmico (finale) e percentuale (finale)
typedef enum {
	enum_distconvtype_unknown=0,
	enum_distconvtype_from_perc_iniz,
	enum_distconvtype_from_perc_fin,
	enum_distconvtype_from_ohm_iniz,
	enum_distconvtype_from_ohm_fin,
	enum_distconvtype_from_coil_windings,
	enum_distconvtype_num_of
}enum_distconvtype;

// struttura che contiene info ausiliarie per effettuare le conversioni
typedef struct _TipoStructDistConversion{
	xdata PROGRAMMA * pPro;
	float f_res_value_ohm;
	float f_res_specifica_ohm_m;
}TipoStructDistConversion;
extern TipoStructDistConversion distConversion;

// routine che serve poter convertire dati distanziazioni
// ad es da valori in percentuale a valori ohmici e numero di spire
// Ingresso:
// * puntatore a struttura che contiene info su distanziazione attuale
// * puntatore a struttura che contiene info complementari per la distanziazione 
// * tipo di conversione desiderata
void v_convert_dist_values(TipoStructDistInfo  *pDist,
						   TipoStructDistConversion  *pDistConv, 
						   enum_distconvtype dist_conversion_type
						   );

// riempie la struttura dist coi parametri memorizzati nel programma...
// se il programma non � uno di quelli col distanziatore abilitato, ritorna 0
// se trasferimento ok, ritorna 1
// Ingresso:
// * puntatore al programma
// * valore ohmico da produrre
// * resistenza specifica del filo (per computare n spire...)
unsigned char ucFillStructDist(TipoStructDistConversion xdata *pDistConv);
// riempie la struttura dist coi parametri memorizzati nel programma...
// se il programma non � uno di quelli col distanziatore abilitato, ritorna 0
// se trasferimento ok, ritorna 1
// Ingresso:
// * puntatore al programma
// * valore ohmico da produrre
// * resistenza specifica del filo (per computare n spire...)
unsigned char ucFillProgramStructDist(TipoStructDistConversion xdata *pDistConv);
unsigned short int us_is_button_pressed(unsigned char ucNumButton);
extern unsigned char ucIamInLogProduzione(void);
extern unsigned char ucIAmSpindleGrinding(void);
// utilit� di calcolo resistenza specifica filo
// se resistenze specifiche non inizializzate correttamente, restituisce 0
float f_calc_res_spec_filo(PROGRAMMA * pPro);
// utilit� di calcolo resistenza di uno spezzone
float f_calc_res_spezzone(float f_res_spec_filo);
/* Utilit� di calcolo della portata dinamica
	Parto dalla portata piu' bassa e salgo. Se vado fuori portata,
	la routine di servizio dell' interruzione cerca alla portata
	di numero inferiore (10 volte maggiore per�), per cui non
	corro rischi con valori a cavallo di due portate, se non quello
	di dover effettuare due cambi portata.
*/
unsigned short int us_calc_portata_dinamica_per_resistenza(float f_max_res_to_measure);
extern void vInitializeHisFields(TipoStructHandleInserimentoCodice *phis);

typedef enum{
    enum_save_encrypted_param_retcode_ok=0,
    enum_save_encrypted_param_retcode_err,
    enum_save_encrypted_param_retcode_invalid_box_number,
    enum_save_encrypted_param_retcode_invalid_param,
    enum_save_encrypted_param_retcode_numof
}enum_save_encrypted_param_retcode;
enum_save_encrypted_param_retcode ui_save_encrypted_param(const char *hex_digit_string_to_decode);

// resets the rotation wheel 1 inversion
// returns 0 if the rotation was reset OK
// returns 1 if cannot reset the rotation inversion due to invalid fpga version
// returns 2 if cannot reset the rotation inversion because the fpga does not set the pin
unsigned int ui_zeroOK_reset_rotation_wheel_inversion(void);

// sets the rotation wheel 1 inversion
// returns 0 if the rotation was inverted OK
// returns 1 if cannot reset the rotation inversion due to invalid fpga version
// returns 2 if cannot reset the rotation inversion because the fpga does not set the pin
unsigned int ui_zeroOK_set_rotation_wheel_inversion(void);


extern unsigned char ucHW_temperature_compensation_params(void);
void vPrintTesto(unsigned int uiRiga,unsigned int uiColonna, unsigned char xdata *pTesto);
void vRefreshScreen(void);
void v_reset_cutter_position(void);
void vAZ4186_InitHwStuff(void);
