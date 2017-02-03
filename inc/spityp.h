#ifndef _SPITYP

#define _SPITYP
#include <stdint.h>
#include "spicost.h"
#include "lcd.h"
#include "minmax.h"
#include "gui.h"


#ifndef _CC51
//#define rom
#define data
//#define xdat
//#define xdata
#define register
#define bit int
#endif

typedef struct _SPEZ_FILO {
   unsigned char flags;
   unsigned long ro[MAX_SPEZ_FILO];
} SPEZ_FILO;

typedef struct _SER_RX {
   unsigned char i;
   char p[DIM_BUF_RX];
} SER_RX;

typedef struct _SER_TX {
   unsigned char i;
   char p[DIM_BUF_TX];
} SER_TX;




// enumerazione info che qualifica i valori di distanziazione inseriti dall'utente
typedef enum {
	enum_perc_distanzia_type_perc_iniz_fin=0, 
	enum_perc_distanzia_type_default=enum_perc_distanzia_type_perc_iniz_fin, // valore di default= perc iniziale e finale
	enum_perc_distanzia_type_perc_iniz_nspire,
	enum_perc_distanzia_type_perc_fin_nspire,
	enum_perc_distanzia_type_numOf
}enum_perc_distanzia_type;


// la percentuale di distanziazione deve avere un valore massimo che renda facile
// convertirla in un numero fra 0 ed 1, cos� usiamo un valore massimo che sia un certo numero di shift di 1
// numero max di shift della percentuale di distanziazione
#define defMaxNumShiftPercDistanzia (15)
// valore massimo della percentuale di distanziazione: 1<<15=0x8000
#define defMaxValuePercDistanzia (1U<<defMaxNumShiftPercDistanzia)
// massimo intero che posso moltiplicare per una perc distanzia senza causare overflow
#define defMaxIntegerIcanMultiplyByPercDistanzia ((1L<<(31-defMaxNumShiftPercDistanzia))-1)


typedef enum{
		enumProductFile=0,
		enumMacParamsFile,
		enumLcdCalibrationFile,
		enumString20_custom_File,
		enumString40_custom_File,
		enumPrivate_data_File,

		enumNumOfFileTypes
	}enumFileTypes;

unsigned char ucFlashFS_fRead(	enumFileTypes fileType,
								unsigned long ulIdxElem,
								unsigned char xdata *puc
							  );

unsigned char ucFlashFS_fWrite(	enumFileTypes fileType,
								unsigned long ulIdxElem,
								unsigned char xdata *puc
							  );

// funzione che ritorna il numero massimo di elementi nella flash
// a seconda del tipo di file (prodotti, parametri macchina, calibrazione lcd)
unsigned long ulMaxNumElemInNandFlashFile(enumFileTypes theFileType);

/* Definizione del tipo che permette di definire la taratura di un canale
   analogico.
   La lettura da adc viene corretta attraverso la formula:
   valore_corretto=k*valore_reale+offset
   k e offset sono numeri reali
*/
typedef struct _TARATURA {
   // Guadagno come floating point e come intero. 
   float k;
   unsigned short int ksh;
   // Offset come floating point e come intero. 
   float o;
   signed short int osh;
   // Primo punto di misura. Il primo punto non � sempre 0.  
   float p0;
   // Secondo punto di misura. 
   float p;
   // Valore misurato in corrispondenza del punto di misura p0.  
   float m0;
   // Valore misurato in corrispondenza del punto di misura p0, in step a/d.  
   float m0step;
   // Valore misurato in corrispondenza del punto di misura p.  
   float m;
   // Valore misurato in corrispondenza del punto di misura p., in step a/d  
   float mstep;
} TARATURA;


// versione attuale del file di definizione dei pars macchina
#define defMacParmsFormatVersion 5
// sono stati aggiunti i due parametri di gestione della bandella
// la versione 2 � uguale alla versione 1, per� nella versione 1 c'era un errore
// nel calcolo del crc!!!!
// nella versione 3 aggiungo parametro abilitazione lampada segnalazione allarme

// parametri macchina versione 0
typedef struct _MACPARMS_V0 {
	unsigned int uiMacParmsFormatVersion; // versione del formato di calibrazione: si parte da 0
	unsigned char ucCrc; // crc: xor invertito di tutti i char che seguono...
	/* Numero di impulsi/giro, considerandoli gi� moltiplicati per
	   il numero delle fasi. 
	*/
	unsigned short impGiro;
	float mmGiro;
	unsigned int rpm_frizioni[NUM_FRIZIONI]; /* Max rpm frizioni. */
	unsigned int rpm_mandrino;               /* Max rpm mandrino. */
	unsigned short lenSemigiro;
	unsigned char modo;         /* Modalit� di produzione: L o L+R. */
	unsigned char ucMaxNumFili;  // numero massimo di fili: 1 2 o 3???
	/* Numero di pezzi prima della fine del lotto raggiunti i quali
	   si deve far lampeggiare la lampada di segnalazione lotto
	   in esaurimento. 
	*/
	unsigned char nPezziAvviso;
	unsigned char nSecLampSpegn;
	unsigned char nspire_pret;  /* Numero di spire di pretaglio. */
	float iasse[NUM_FRIZIONI];  /* Distanza d' interasse mandrino coltello. */
	float diam_frizioni[NUM_FRIZIONI];  /* Diametro delle frizioni in mm. */
	float diam_coltello;  /* Diametro del coltello in mm. */
	unsigned char nsec;   /* Numero di secondi da attendere prima di
						partire col prg. */
	unsigned int durata_all; /* Numero minimo di millisecondi che deve durare
						   un allarme per essere considerato tale. */
	float DiamTestIntrz; /* Per diametri filo >= a quanto indicato qui,
					   con due fili non si testa interruzione filo. */
 	/* Percentuale massima di errore accettabile in ro:
    	se superiore, si tratta di una falsa misura.
	*/
	unsigned char ucPercMaxErrRo;
	/* Num max di spezzoni consecutivi con misura fuori tolleranza ammessi. */
	unsigned char ucMaxBadSpezzConsec;
	float fFSassorbRuote; /* fondo scala assorbimento delle ruote di contrasto */
	TARATURA tarParms[NCANALI_TARABILI][NUM_PORTATE];
} MACPARMS_V0;

// parametri macchina versione 1 e 2
typedef struct _MACPARMS_V2 {
	unsigned int uiMacParmsFormatVersion; // versione del formato di calibrazione: si parte da 0
	unsigned char ucCrc; // crc: xor invertito di tutti i char che seguono...
	/* Numero di impulsi/giro, considerandoli gia' moltiplicati per il numero delle fasi. 	*/
	unsigned short impGiro;
	float mmGiro;
	unsigned int rpm_frizioni[NUM_FRIZIONI]; /* Max rpm frizioni. */
	unsigned int rpm_mandrino;               /* Max rpm mandrino. */
	unsigned short lenSemigiro;
	unsigned char modo;         /* Modalit� di produzione: L o L+R. */
	unsigned char ucMaxNumFili;  // numero massimo di fili: 1 2 o 3???
	/* Numero di pezzi prima della fine del lotto raggiunti i quali
	   si deve far lampeggiare la lampada di segnalazione lotto
	   in esaurimento. 
	*/
	unsigned char nPezziAvviso;
	unsigned char nSecLampSpegn;
	unsigned char nspire_pret;  /* Numero di spire di pretaglio. */
	float iasse[NUM_FRIZIONI];  /* Distanza d' interasse mandrino coltello. */
	float diam_frizioni[NUM_FRIZIONI];  /* Diametro delle frizioni in mm. */
	float diam_coltello;  /* Diametro del coltello in mm. */
	unsigned char nsec;   /* Numero di secondi da attendere prima di
						partire col prg. */
	unsigned int durata_all; /* Numero minimo di millisecondi che deve durare
						   un allarme per essere considerato tale. */
	float DiamTestIntrz; /* Per diametri filo >= a quanto indicato qui,
					   con due fili non si testa interruzione filo. */
 	/* Percentuale massima di errore accettabile in ro:
    	se superiore, si tratta di una falsa misura.
	*/
	unsigned char ucPercMaxErrRo;
	/* Num max di spezzoni consecutivi con misura fuori tolleranza ammessi. */
	unsigned char ucMaxBadSpezzConsec;
	float fFSassorbRuote; /* fondo scala assorbimento delle ruote di contrasto */
	TARATURA tarParms[NCANALI_TARABILI][NUM_PORTATE_DINAMICHE];
	unsigned long password;
	// regolazione tramite encoder (1) o potenziometri (0)?
	unsigned char uc_1Encoder_0Potenziometri;
	// abilitazione a passaggio automatico da una comemssa alla successiva
	unsigned char ucEnableCommesseAutomatiche;
	// char che indica se � abilitata la quinta portata
	unsigned char ucAbilitaQuintaPortata;
	// un char per indicare se pilotaggio taglio/pretaglio usa sensore o no
	unsigned char ucUsaSensoreTaglio;
	// posizione minima e massima del coltello, tolleranza nel posizionamento del coltello; [step a/d]
	unsigned int uiMinPosColtello_StepAD,uiMaxPosColtello_StepAD,uiTolerancePosColtello_StepAD;
	// valore massimo da dare al dac quando si � in modo slow
	unsigned char ucSlowDacColtello;
	// guadagno per il controllo del coltello [0..255]
	unsigned char ucGainDacColtello;
	unsigned int uiRitardoAttivazioneBandella_centesimi_secondo;
	unsigned int uiDurataAttivazioneBandella_centesimi_secondo;
	// spazio per altri pars, per non dover aggiungere 1000 tipi di parametri macchina
	// finito!!
} MACPARMS_V2;

// parametri macchina versione 3
typedef struct _MACPARMS_V3 {
	unsigned int uiMacParmsFormatVersion; // versione del formato di calibrazione: si parte da 0
	unsigned char ucCrc; // crc: xor invertito di tutti i char che seguono...
	/* Numero di impulsi/giro, considerandoli gia' moltiplicati per il numero delle fasi. 	*/
	unsigned short impGiro;
	float mmGiro;
	unsigned int rpm_frizioni[NUM_FRIZIONI]; /* Max rpm frizioni. */
	unsigned int rpm_mandrino;               /* Max rpm mandrino. */
	unsigned short lenSemigiro;
	unsigned char modo;         /* Modalit� di produzione: L o L+R. */
	unsigned char ucMaxNumFili;  // numero massimo di fili: 1 2 o 3???
	/* Numero di pezzi prima della fine del lotto raggiunti i quali
	   si deve far lampeggiare la lampada di segnalazione lotto
	   in esaurimento. 
	*/
	unsigned char nPezziAvviso;
	unsigned char nSecLampSpegn;
	unsigned char nspire_pret;  /* Numero di spire di pretaglio. */
	float iasse[NUM_FRIZIONI];  /* Distanza d' interasse mandrino coltello. */
	float diam_frizioni[NUM_FRIZIONI];  /* Diametro delle frizioni in mm. */
	float diam_coltello;  /* Diametro del coltello in mm. */
	unsigned char nsec;   /* Numero di secondi da attendere prima di
						partire col prg. */
	unsigned int durata_all; /* Numero minimo di millisecondi che deve durare
						   un allarme per essere considerato tale. */
	float DiamTestIntrz; /* Per diametri filo >= a quanto indicato qui,
					   con due fili non si testa interruzione filo. */
 	/* Percentuale massima di errore accettabile in ro:
    	se superiore, si tratta di una falsa misura.
	*/
	unsigned char ucPercMaxErrRo;
	/* Num max di spezzoni consecutivi con misura fuori tolleranza ammessi. */
	unsigned char ucMaxBadSpezzConsec;
	float fFSassorbRuote; /* fondo scala assorbimento delle ruote di contrasto */
	TARATURA tarParms[NCANALI_TARABILI][NUM_PORTATE_DINAMICHE];
	unsigned long password;
	// regolazione tramite encoder (1) o potenziometri (0)?
	unsigned char uc_1Encoder_0Potenziometri;
	// abilitazione a passaggio automatico da una comemssa alla successiva
	unsigned char ucEnableCommesseAutomatiche;
	// char che indica se � abilitata la quinta portata
	unsigned char ucAbilitaQuintaPortata;
	// un char per indicare se pilotaggio taglio/pretaglio usa sensore o no
	unsigned char ucUsaSensoreTaglio;
	// posizione minima e massima del coltello, tolleranza nel posizionamento del coltello; [step a/d]
	unsigned int uiMinPosColtello_StepAD,uiMaxPosColtello_StepAD,uiTolerancePosColtello_StepAD;
	// valore massimo da dare al dac quando si � in modo slow
	unsigned char ucSlowDacColtello;
	// guadagno per il controllo del coltello [0..255]
	unsigned char ucGainDacColtello;
	unsigned int uiRitardoAttivazioneBandella_centesimi_secondo;
	unsigned int uiDurataAttivazioneBandella_centesimi_secondo;

// da qui iniziano i parametri della versione 3...

	// abilitazione gestione lampada allarme
	unsigned char ucAbilitaLampadaAllarme;
	// presenza distanziatore: 1--> presente, else non presente
	unsigned char uc_distanziatore_type;
	// distanza tra distanziatore e coltello [mm]
	float fOffsetDistanziatore_ColtelloMm;
	// tempo che il distanziatore impiega a scendere in posizione bassa
	// questo valore viene assegnato ad ogni programma che usa il distanziatore
	unsigned short int usDelayDistanziaDownMs;
	// spazio per altri parametri: rimangono 6chars
	unsigned char ucOtherParams[6];

} MACPARMS_V3;

// parametri macchina versione 4
typedef struct _MACPARMS_V4 {
	unsigned int uiMacParmsFormatVersion; // versione del formato di calibrazione: si parte da 0
	unsigned char ucCrc; // crc: xor invertito di tutti i char che seguono...
	/* Numero di impulsi/giro, considerandoli gia' moltiplicati per il numero delle fasi. 	*/
	unsigned short impGiro;
	float mmGiro;
	unsigned int rpm_frizioni[NUM_FRIZIONI]; /* Max rpm frizioni. */
	unsigned int rpm_mandrino;               /* Max rpm mandrino. */
	unsigned short lenSemigiro;
	unsigned char modo;         /* Modalit� di produzione: L o L+R. */
	unsigned char ucMaxNumFili;  // numero massimo di fili: 1 2 o 3???
	/* Numero di pezzi prima della fine del lotto raggiunti i quali
	   si deve far lampeggiare la lampada di segnalazione lotto
	   in esaurimento. 
	*/
	unsigned char nPezziAvviso;
	unsigned char nSecLampSpegn;
	unsigned char nspire_pret;  /* Numero di spire di pretaglio. */
	float iasse[NUM_FRIZIONI];  /* Distanza d' interasse mandrino coltello. */
	float diam_frizioni[NUM_FRIZIONI];  /* Diametro delle frizioni in mm. */
	float diam_coltello;  /* Diametro del coltello in mm. */
	unsigned char nsec;   /* Numero di secondi da attendere prima di
						partire col prg. */
	unsigned int durata_all; /* Numero minimo di millisecondi che deve durare
						   un allarme per essere considerato tale. */
	float DiamTestIntrz; /* Per diametri filo >= a quanto indicato qui,
					   con due fili non si testa interruzione filo. */
 	/* Percentuale massima di errore accettabile in ro:
    	se superiore, si tratta di una falsa misura.
	*/
	unsigned char ucPercMaxErrRo;
	/* Num max di spezzoni consecutivi con misura fuori tolleranza ammessi. */
	unsigned char ucMaxBadSpezzConsec;
	float fFSassorbRuote; /* fondo scala assorbimento delle ruote di contrasto */
	TARATURA tarParms[NCANALI_TARABILI][NUM_PORTATE_DINAMICHE];
	unsigned long password;
	// regolazione tramite encoder (1) o potenziometri (0)?
	unsigned char uc_1Encoder_0Potenziometri;
	// abilitazione a passaggio automatico da una comemssa alla successiva
	unsigned char ucEnableCommesseAutomatiche;
	// char che indica se � abilitata la quinta portata
	unsigned char ucAbilitaQuintaPortata;
	// un char per indicare se pilotaggio taglio/pretaglio usa sensore o no
	unsigned char ucUsaSensoreTaglio;
	// posizione minima e massima del coltello, tolleranza nel posizionamento del coltello; [step a/d]
	unsigned int uiMinPosColtello_StepAD,uiMaxPosColtello_StepAD,uiTolerancePosColtello_StepAD;
	// valore massimo da dare al dac quando si � in modo slow
	unsigned char ucSlowDacColtello;
	// guadagno per il controllo del coltello [0..255]
	unsigned char ucGainDacColtello;
	unsigned int uiRitardoAttivazioneBandella_centesimi_secondo;
	unsigned int uiDurataAttivazioneBandella_centesimi_secondo;

// da qui iniziano i parametri della versione 3...

	// abilitazione gestione lampada allarme
	unsigned char ucAbilitaLampadaAllarme;
	// presenza distanziatore: 1--> presente, else non presente
	unsigned char uc_distanziatore_type;
	// distanza tra distanziatore e coltello [mm]
	float fOffsetDistanziatore_ColtelloMm;
	// tempo che il distanziatore impiega a scendere in posizione bassa
	// questo valore viene assegnato ad ogni programma che usa il distanziatore
	unsigned short int usDelayDistanziaDownMs;
	// 
	unsigned char ucAbilitaCompensazioneTemperatura;

} MACPARMS_V4;



// parametri macchina versione 5
typedef struct _MACPARMS {
	unsigned int uiMacParmsFormatVersion; // versione del formato di calibrazione: si parte da 0
	unsigned char ucCrc; // crc: xor invertito di tutti i char che seguono...
	/* Numero di impulsi/giro, considerandoli gia' moltiplicati per il numero delle fasi. 	*/
	unsigned short impGiro;
	float mmGiro;
	unsigned int rpm_frizioni[NUM_FRIZIONI]; /* Max rpm frizioni. */
	unsigned int rpm_mandrino;               /* Max rpm mandrino. */
	unsigned short lenSemigiro;
	unsigned char modo;         /* Modalit� di produzione: L o L+R. */
	unsigned char ucMaxNumFili;  // numero massimo di fili: 1 2 o 3???
	/* Numero di pezzi prima della fine del lotto raggiunti i quali
	   si deve far lampeggiare la lampada di segnalazione lotto
	   in esaurimento. 
	*/
	unsigned char nPezziAvviso;
	unsigned char nSecLampSpegn;
	unsigned char nspire_pret;  /* Numero di spire di pretaglio. */
	float iasse[NUM_FRIZIONI];  /* Distanza d' interasse mandrino coltello. */
	float diam_frizioni[NUM_FRIZIONI];  /* Diametro delle frizioni in mm. */
	float diam_coltello;  /* Diametro del coltello in mm. */
	unsigned char nsec;   /* Numero di secondi da attendere prima di
						partire col prg. */
	unsigned int durata_all; /* Numero minimo di millisecondi che deve durare
						   un allarme per essere considerato tale. */
	float DiamTestIntrz; /* Per diametri filo >= a quanto indicato qui,
					   con due fili non si testa interruzione filo. */
 	/* Percentuale massima di errore accettabile in ro:
    	se superiore, si tratta di una falsa misura.
	*/
	unsigned char ucPercMaxErrRo;
	/* Num max di spezzoni consecutivi con misura fuori tolleranza ammessi. */
	unsigned char ucMaxBadSpezzConsec;
	float fFSassorbRuote; /* fondo scala assorbimento delle ruote di contrasto */
	TARATURA tarParms[NCANALI_TARABILI][NUM_PORTATE_DINAMICHE];
	unsigned long password;
	// regolazione tramite encoder (1) o potenziometri (0)?
	unsigned char uc_1Encoder_0Potenziometri;
	// abilitazione a passaggio automatico da una comemssa alla successiva
	unsigned char ucEnableCommesseAutomatiche;
	// char che indica se � abilitata la quinta portata
	unsigned char ucAbilitaQuintaPortata;
	// un char per indicare se pilotaggio taglio/pretaglio usa sensore o no
	unsigned char ucUsaSensoreTaglio;
	// posizione minima e massima del coltello, tolleranza nel posizionamento del coltello; [step a/d]
	unsigned int uiMinPosColtello_StepAD,uiMaxPosColtello_StepAD,uiTolerancePosColtello_StepAD;
	// valore massimo da dare al dac quando si � in modo slow
	unsigned char ucSlowDacColtello;
	// guadagno per il controllo del coltello [0..255]
	unsigned char ucGainDacColtello;
	unsigned int uiRitardoAttivazioneBandella_centesimi_secondo;
	unsigned int uiDurataAttivazioneBandella_centesimi_secondo;

// da qui iniziano i parametri della versione 3...

	// abilitazione gestione lampada allarme
	unsigned char ucAbilitaLampadaAllarme;
	// presenza distanziatore: 1--> presente, else non presente
	unsigned char uc_distanziatore_type;
	// distanza tra distanziatore e coltello [mm]
	float fOffsetDistanziatore_ColtelloMm;
	// tempo che il distanziatore impiega a scendere in posizione bassa
	// questo valore viene assegnato ad ogni programma che usa il distanziatore
	unsigned short int usDelayDistanziaDownMs;
	// 
	unsigned char ucAbilitaCompensazioneTemperatura;
	unsigned char uc_enable_spindle_grinding;

} MACPARMS;

// enumerazione dei tipi di distanziatore ammessi
typedef enum{
	enum_distanziatore_type_none=0,		// nessun distanziatore presente
	enum_distanziatore_type_pneumatico,	// distanziatore pneumatico
	enum_distanziatore_type_coltello,	// coltello usato anche come distanziatore
	enum_distanziatore_type_numOf
}enum_distanziatore_type;



typedef struct _SERIALE {
   unsigned char baud;
   unsigned char par;
   unsigned char Rs485;
} SERIALE;



/* Definizione di tipo usata per test. */
typedef struct StructTipoInfotest{
	   unsigned long ro;
	   unsigned long nimp;
	   unsigned int ns;
	}TipoInfotest;


// numero righe per calibrazione lcd
#define defNumColLcdCalibration 4
// numero colonne per calibrazione lcd
#define defNumRowLcdCalibration 4
// iCalibraLcdRealZone
// array che contiene i punti di delimitazione delle varie zone della tabella di calibrazione
// prima X, poi Y, a prtire dall'angolo sinistra alto del display (coordinate in pixel grandi), si scende in Y (X costante Y cala)
// poi si riprende dall'alto e si sposta X a destra (X cala di tot, Y riparte da alto e ricala)
// 
//   P0           P4           P8           PC
//    |         /  |         /  |         /  |
//    |        /   |        /   |        /   |
//   P1       /   P5       /   P9       /   PD
//    |      /     |      /     |      /     |
//    |     /      |     /      |     /      |
//   P2    /      P6    /      PA    /      PE
//    |   /        |   /        |   /        |
//    |  /         |  /         |  /         |
//   P3 /         P7 /         PB /         PF
// 
// 
// 
// fCoeffsCalibraLcd
// array che contiene i coefficienti di calibrazione lcd, organizzati cos�:
// i coefficienti sono sempre [a b c d] prima per il calcolo di X, poi per il calcolo di Y
// nell'ordine:
// quadrato Q1=P0P1P4P5, quadrato Q2=P1P2P5P6, quadrato Q3=P2P3P6P7, quadrato Q4=P4P5P8P9, quadrato Q5=P5P6P9PA,
// quadrato Q6=P6P7PAPB, quadrato Q7=P8P9PCPD, quadrato Q8=P9PAPDPE, quadrato Q9PAPBPEPF, 
//   P0-----------P4-----------P8-----------PC
//    |    Q1      |    Q4      |    Q7      |
//    |            |            |            |
//   P1-----------P5-----------P9-----------PD
//    |    Q2      |    Q5      |    Q8      |
//    |            |            |            |
//   P2-----------P6-----------PA-----------PE
//    |    Q3      |    Q6      |    Q9      |
//    |            |            |            |
//   P3-----------P7-----------PB-----------PF

// versione attuale della struttura di salvataggio info calibra lcd
#define defActFormatCalibrationVersion 0
typedef struct _TipoStructLcdCalibration{
	unsigned int uiCalibrationFormatVersion; // versione del formato di calibrazione: si parte da 0
	unsigned char ucCrc; // crc: xor invertito di tutti i char che seguono...
	int iCalibraLcdRealZone[defNumColLcdCalibration*defNumRowLcdCalibration][2];
	float fCoeffsCalibraLcd[(defNumColLcdCalibration-1)*(defNumRowLcdCalibration-1)*4*2];
}TipoStructLcdCalibration;
extern xdata TipoStructLcdCalibration lcdCal;
// valori di default per la tabella di calibrazione...
extern const TipoStructLcdCalibration lcdCalDefault;

// struttura con info sul reset al boot
// se premo start e stop --> reset solo delle commesse
// se premo start e prova ophmica --> reset delle commesse e della memoria flash
typedef struct _TipoExamineBootReset{
	unsigned char ucBootResetExamined;
	unsigned char ucBootResetNecessary;
	unsigned char ucSaveCntInterrupt;
	unsigned int uiCntLoop;
}TipoExamineBootReset;
extern xdata TipoExamineBootReset examineBootReset[2];
extern xdata unsigned char ucBooting;
extern xdata float portataDinamica[5];



//
// --------------------------------------------------------------------
// --------------------------------------------------------------------
//
//
// definizione della struttura che contiene il codice corrente...
//
// --------------------------------------------------------------------
// --------------------------------------------------------------------
//
//
#define defCodice_MaxCharDiametroFilo 5
#define defCodice_MaxCharTipoFilo 4
#define defCodice_MaxCharDiametroMandrino 4
#define defCodice_MaxCharNumFili 2
#define defCodice_MaxCharFornitore 2
#define defCodice_MaxChar 20

typedef struct _TipoStructHandleInserimentoCodice{
	float fAux;
	unsigned int uiValidKey_0xa536;
	unsigned int uiCrc;
	unsigned char ucDiametroFilo[defCodice_MaxCharDiametroFilo+3];
	unsigned char ucTipoFilo[defCodice_MaxCharTipoFilo+1];
	unsigned char ucDiametroMandrino[defCodice_MaxCharDiametroMandrino+3];
	unsigned char ucNumFili[defCodice_MaxCharNumFili+1];
	unsigned char ucFornitore[defCodice_MaxCharFornitore+1];
	unsigned char ucCodice[defCodice_MaxChar+1];
	unsigned char ucCodiceIngresso[defCodice_MaxChar+1];
	TipoButtonStruct button;
}TipoStructHandleInserimentoCodice;
// variabili con le info sul codice prg corrente
extern xdata TipoStructHandleInserimentoCodice his;
// numero max di buttons che si possono rappresentare su una schermata lcd
#define defMaxButtonsOnLcd 32
void vForceLcdRefresh(void);
unsigned char ucNeedToClosePopupWindow(void);
void vSearchFirstComm(void);
typedef struct _TipoStructTempoPerResistenza{
	unsigned long ulTempoPerResistenzaCorrente;
	unsigned long ulTempoPerResistenzaUltima;
	unsigned long ulTempoPerResistenzaCorrente_Main;
	unsigned long ulTempoPerResistenzaUltima_Main;
	unsigned int uiNumPezziFatti;
	unsigned int uiOreResidue;
	unsigned int uiMinutiResidui;
	unsigned long ulSecondiResidui;
	unsigned long ulAux;
	unsigned int uiLastNdo;
	unsigned char ucLastCntInterrupt;
}TipoStructTempoPerResistenza;

extern TipoStructTempoPerResistenza xdata TPR;
extern unsigned long lastRo;
extern unsigned char xdata ucEliminaPrimoPezzo;

	// minimo tempo ammesso per timeout spaziatore gi� [ms]
	// occhio che questo tempo viene moltiplicato per def_timeout_spaziatore_factor come maergine di sicurezza
	#define defMinTimeoutSpaziatoreDown_millisecondi 750
	// minimo tempo ammesso per timeout spaziatore gi� [numero di periodi interrupt=5ms]
	#define defMinTimeoutSpaziatoreDown_NumT (defMinTimeoutSpaziatoreDown_millisecondi/PeriodoIntMs)
	// il timeout distanziatore gi� quante volte deve essere rispetto il timeout minimo? impostare >1, ad esempio 2
	#define def_timeout_spaziatore_factor 2

	// stati possibili del coltello in relazione alla distanziazione
	typedef enum {
		enum_coltello_distanzia_stato_idle=0,
		enum_coltello_distanzia_stato_do_pretaglio,
		enum_coltello_distanzia_stato_do_distanzia,
		enum_coltello_distanzia_stato_num_of
	} enum_coltello_distanzia_stato;

	typedef struct _TipoStructHandleDistanziatore{
		enum_coltello_distanzia_stato stato;
		/* Attuale indice percentuale distanziazione */
		unsigned char actPercDist;
		unsigned char uc_clocks[4];
		long oldResDistValue;
		long actResDistValue;
		long precResDistValue;
		long deltaDistValue;
		// resistenza di N spire, per tener conto della compressione meccanica della spira devo scendere due spire prima di quanto sarebbe necessario
		long res_spire_compressione_urp_imp;
		unsigned long ulAux;
		unsigned short int usTimeout_NumT;
		/* vale 1 se e' in corso una distanziazione */
		unsigned char ucDistanziaInCorso;
		unsigned char ucRefresh;
		/* indica il numero di distanziazioni effettuate */
		unsigned char actNumDistanzia;
		/* indica il numero di distanziazioni nel programma in esecuzione */
		unsigned char runPrgNumDistanzia;
		/* vale 1 quando si possono iniziare le distanziazioni */
		unsigned char ucStartDistanzia;
		/* anticipo azione distanziatore in impulsi encoder o urp, a seconda del tipo di misura */
		long anticipoDistanz;
		unsigned long actRoDist;
		float fLenFiloDistColtMm;
		unsigned char ucForceDown;
		unsigned char uiEnabled_interrupt;
	}TipoStructHandleDistanziatore;
	extern xdat TipoStructHandleDistanziatore hDist;
	
	// posizione nel codice prodotto del carattere che identifica un codice che attiva il separatore
	// idx=15-->il carattere viene stampato immediatamente prima del numero di fili
	#define defDistanz_IdxSpecialCharInCode 15
	// carattere che identifica un codice prodotto che attiva il separatore
	#define defDistanz_SpecialCharInCode 'S'
	// macro che permette di identificare se codice prodotto attuale prevede l'attivazione del distanziatore
	#define macroIsCodiceProdotto_Distanziatore(pPrg)	(((pPrg)->codicePrg[defDistanz_IdxSpecialCharInCode]==defDistanz_SpecialCharInCode)?1:0)
	// macro che permette di identificare se stringa codice prodotto attuale prevede l'attivazione del distanziatore
	#define macroIsStringaCodiceProdotto_Distanziatore(theString)	((theString[defDistanz_IdxSpecialCharInCode]==defDistanz_SpecialCharInCode)?1:0)
	// macro che permette di abilitare in una stringa codice prodotto l'attivazione del distanziatore
	#define macroEnableStringCodiceProdotto_Distanziatore(theString)	(theString[defDistanz_IdxSpecialCharInCode]=defDistanz_SpecialCharInCode)
	// macro che permette di disabilitare in una stringa codice prodotto l'attivazione del distanziatore
	#define macroDisableStringCodiceProdotto_Distanziatore(theString)	(theString[defDistanz_IdxSpecialCharInCode]=' ')

// base dei tempi [ms] usata dal parametro che indica la durata di discesa del distanziatore
#define defTimeBaseDistanziaMs 100L
// define da usare per convertire un tempo di discesa distanziatore in numero di tick interrupt
#define defConvertTimeDistanziaToInterruptTick(a) ((((long)(a))*defTimeBaseDistanziaMs)/PeriodoIntMs)
// normalizzazione intera di un valore moltiplicato per una percentuale di distanziazione
long l_multiply_long_perc_dist(long theLong, unsigned short int us_perc_dist);

	// posizione nel codice prodotto del carattere che identifica un codice che attiva la compensazione di temperatura
	// idx=5-->il carattere viene stampato immediatamente dopo il diametro del filo e prima della lega
	#define defTempComp_IdxSpecialCharInCode 5
	// carattere che identifica un codice prodotto che utilizza la compensazione di temperatura
	#define defTempComp_SpecialCharInCode 'T'
	// macro che permette di identificare se codice prodotto attuale prevede la compensazione di temperatura
	#define macroIsCodiceProdotto_TemperatureCompensated(pPrg)	(((pPrg)->codicePrg[defTempComp_IdxSpecialCharInCode]==defTempComp_SpecialCharInCode)?1:0)
	// macro che permette di identificare se stringa codice prodotto attuale prevede la compensazione di temperatura
	#define macroIsStringaCodiceProdotto_TemperatureCompensated(theString)	((theString[defTempComp_IdxSpecialCharInCode]==defTempComp_SpecialCharInCode)?1:0)
	// macro che permette di abilitare in una stringa codice prodotto la compensazione di temperatura
	#define macroEnableStringCodiceProdotto_TemperatureCompensated(theString)	(theString[defTempComp_IdxSpecialCharInCode]=defTempComp_SpecialCharInCode)
	// macro che permette di disabilitare in una stringa codice prodotto la compensazione di temperatura
	#define macroDisableStringCodiceProdotto_TemperatureCompensated(theString)	(theString[defTempComp_IdxSpecialCharInCode]=' ')


#endif /* _SPITYP */
