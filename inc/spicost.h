#ifndef _SPICOST
//#include "upsd3400.h"
//#include "upsd3400_hardware.h"
//#include "upsd3400_timer.h"
#define xdat xdata
#ifndef ARM9platform
    #define rom code
#else
    #define rom const
#endif
// lunghezza in mm del primo moncone da eliminare...
#define defLunghezzaMonconeEliminaPrimoPezzoMm 60.0

/* Numero di linee di cui si compone l' lcd. */
#define NUM_LINES_LCD 2
/* Numero di carattere di cui si compone una linea dell' lcd. */
#define LEN_LINES_LCD 20

#if 0
	/* Definizione dei bit di configurazione delle maschere per il controllo
	   dell' lcd. */
	/* Clear lcd e cancella memoria. */
	#define LCD_CLEAR_AND_MEM 	0x01
	/* Cursor home. */
	#define LCD_HOME 		0x02
	/* Set entry mode. */
	#define LCD_SET_ENTRY		0x04
	  #define CURSOR_2RIGHT         0x02
	  #define SCREEN_2RIGHT         0x01
	/* On/Off control. */
	#define LCD_ON_OFF_CTRL		0x08
	  #define LCD_ON	        0x04
	  #define LCD_CURSOR_ON	        0x02
	  #define LCD_CURSOR_BLINK      0x01
	/* Shift control. */
	#define LCD_SHIFT_CTRL		0x10
	/* Set configuration. */
	#define LCD_SET_CONFIG 		0x20
	  #define LCD_CFG_8BIT          0x10
	  #define LCD_CFG_NUM_LINES     ((NUM_LINES_LCD==2)? (0x08):(0x00))
	  #define LCD_CFG_DOTS          0x00
	/* Access character generator RAM. */
	#define LCD_CG_RAM 		0x40
	/* Access display data RAM. */
	#define LCD_DD_RAM 		0x80
	/* Codice che definisce l' indirizzo del primo carattere dell' lcd. */
	#define LCD_FRST_CHAR LCD_DD_RAM
	/* Codice che definisce l' indirizzo del primo carattere della seconda
	   linea dell' lcd. */
	#define LCD_CHARS_ROW 0x40
	/* Posizione del bit di busy nella lettura dello stato dell' lcd. */
	#define LCD_BUSY 0x80
	/* Non usata. */
	#define LCD_RS   0x2

	/* Indirizzo di I/O dell' lcd.
	   0 --> write config
	   1 --> write data
	   2 --> read  config
	   3 --> read  data
	*/
	#define ADDR_LCDDAT     0xFE00

	/* Non usato mai: non si sa cos' �. */
	#define ADDR_LCDHND     0x8010
#endif

#define lcdReady 1



#define ReverseChar(C)  (C)
#define DirectChar(C)  (C)
extern void clearLcd(void);
extern void fillSpace(void);
extern void initLcd(void);




/* Con questa define viene escuso quasi totalmente il menu
   utilit� e si aggiungono nel menu dei parametri
   macchina i parametri massima perc errore significativa in misura
   ro e numero max di errori consecutivi ammessi. */
/* #define defTest2fili */

/*#define WriteSpezzoni*/


#ifndef _CC51
   #define COM_SIMUL 2
   #define GAP_ENC 10
#endif

#define _SPICOST

#define PI_GRECO 3.141592

#define MAX_SPEZ_FILO 6

/* Stato dell' allarme: noallarme, antirimbalzo, allarmevero. */
#define alr_status_noalr 0
#define alr_status_rimb  1
#define alr_status_alr   2




enum languages {
   LANG_ITA,
   LANG_ENG,
   LANG_DEU,

   NUM_LANGUAGES
};

#define DIM_BUF_RX 100
#define DIM_BUF_TX 200

#define SYNC   22
#define EOT    4
#define CR     '\r'
#define LF     '\n'

#define LEN_BUF_7SEG 10

#define NUM_MATERIALI 20

#define NUM_PROGRAMMI 3

#define NUM_COMMESSE  5

/* Numero di ingressi analogici dall' adc.
   1 --> canale 0 (misura dinamica)
   2 --> canale 1 (misura statica)
   3 --> canale 2 (vel. produzione)
   4 --> canale 3 (assorbimento ruota 1)
   5 --> canale 4 (assorbimento ruota 2)
   6 --> canale 5 (velocit� friz 1)
   7 --> canale 6 (velocit� friz 2)
   8 --> libero
*/
#define NUM_INP_ANALOG 8
#define NUM_INP_DG 16
#define NUM_OUT_DG 8
#define NUM_BYTES_DG 3


/* Fault motore coltello. */
#define IDG_FILOAGGROV    0x1
/* Fault motore coltello. */
#define IDG_ELETTROM    0x4
/* Coltello nella posizione alta. */
#define IDG_TAGLIO_HI   0x8
/* Misura valida da torretta. */
#define IDG_MIS_VALID1  0x40
/* Start. */
#define IDG_START       0x100
/* Stop. */
#define IDG_STOP        0x200
/* Pulsante misura statica. */
#define IDG_MSTA        0x400
/* Interruzione filo 1. */
#define IDG_INTRZ1      0x800
/* Interruzione filo 2. */
#define IDG_INTRZ2      0x1000
/* Drive motore mandrino. */
#define IDG_DROK        0x2000
/* Fault frizione 1. */
#define IDG_FRIZ1       0x4000
/* Fault frizione 2. */
#define IDG_FRIZ2       0x8000


/* Attivazione misura statica; uscite 0x02 e 0x04 indicano la portata. */
#define ODG_MSTA        0x8
/* Motore mandrino. */
#define ODG_MOTORE      0x10
/* Coltello in posizione di taglio. */
#define ODG_TAGLIO      0x20
/* Coltello in posizione di pretaglio. */
#define ODG_PRETAGLIO   0x40
/* Accensione lampada gialla. */
#define ODG_LAMPADA     0x80
// Attivazione distanziatore/separatore
#define ODG_DISTANZIA   0x100
// Pilotaggio bandella. 
#define ODG_BANDELLA    0x200
/* Accensione lampada rossa. */
#define ODG_LAMPADA_RED 0x400


/* Numero di giri/minuto del mandrino che identifica un programma
   solo inserito, con i parametri non ancora memorizzati. */
#define RPM_MAND_PRG_EMPTY  mymax(MIN_RPM_MANDRINO,100)
/* Numero di misure effettuate per tarare i potenziometri delle misure
   statica e dinamica. */
#define NMISURE_TARATURA  64

/* Isteresi per l' abilitazione dei potenziometri (+/-10%) */
#define IsteresiUpPot  (11./10.)
#define IsteresiDnPot  (9./10.)

/* Incremento della velocit� delle frizioni rispetto a quella
   nominale per dare loro un p� di grip rispetto a mandrino/filo. */
#define DEFAULT_DELTA_RPM_FRIZ  ((char)105)

// margine rotazione frizioni rispetto al massimo numero di giri ammesso
// es la frizione pu� ruotare a max 1000 rpm teorici, con questo valore impongo
// un margine di sicurezza [rpm]
#define defMarginRpmFrizioni 5


/* Numero di millisecondi da aspettare prima di tagliare il primo pezzo.
   Attualmente si aspettano otto secondi.
*/
#define TaglioPezzoMs  (8L*1000L)


/* L' utente puo' sollevare il coltello al massimo di MaxMmAggPosPre
   dalla posizione ideale di pretaglio. */
#define MaxMmAggPosPre  2.0
/* Posizione del coltello all' inizio del programma in percentuale.
   Il 100% corrisponde al coltello alla massima altezza.
*/
#define PercPosInizialeColtello         0.95 
/*#define PercPosInizialeColtello         1*/
#define PercPosInizialeColtelloPre      0.65

/* Numero di dac che necessitano di rampa:
   motori mandrino e frizioni.
*/
#define NumDacRampa 3

/* Numero di gradini di rampa generati per variare le uscite dei dac.
   ATTUALMENTE
   Dato che si d� un gradino ogni 30 ms e i gradini sono 10,
   la rampa dura 0.3 s.
*/
#define NumGradiniRampaDAC  10


#define RefreshRampa     30 /* ms rinfresco gradino rampa. */

/* Numero di millisecondi del periodo di aggiornamento della
   velocita' di produzione.
   Attualmente viene scelto un valore pari a 200ms.
*/
#define RefreshVelocity  200

/* Definizione dei caratteri aumenta/diminuisci potenziometro. */
#define CarattereFrecciaDx '>'
#define CarattereAumenta   CarattereFrecciaDx
#define CarattereFrecciaSx '<'
#define CarattereDiminuisci   CarattereFrecciaSx

/* Maschera degli allarmi che causano una partenza lenta della prossima
   commessa. */
#define maskAlarmsSlowRpm (ALR_FILOAGGROV|ALR_INTRZ1|ALR_INTRZ2)



enum menuOperations {
   MNU_OP_PRINT,
   MNU_OP_UP,
   MNU_OP_DN,
   MNU_OP_RG,
   MNU_OP_LF,
   MNU_OP_ABANDON_EDIT,
   MNU_OP_CONFIRM_EDIT,
   MNU_OP_ENTER,
   MNU_OP_ESC,
   MNU_OP_F1,
   MNU_OP_F2,
   MNU_OP_F3,
   MNU_OP_F4,
   MNU_OP_F5,
   MNU_OP_F6,
   MNU_OP_UPDATE,
   MNU_OP_DOIT,

   NUM_MENU_OPERATIONS
};

enum menus {
   MENU_SEL_MENU,
   MENU_EDIT_COMM,
   MENU_SETUP,
   MENU_EDIT_PROG,
   MENU_UTILS,
   MENU_EDIT_PARS,
   MENU_TARATURA,
   MENU_PASSW,

   NUM_MENUS
};
enum SubmenusTaratura {
   SBM_TARAT_MIS_STAT1K,
   SBM_TARAT_MIS_STAT1OFF,
   SBM_TARAT_MIS_STAT1RC,
   SBM_TARAT_MIS_STAT2K,
   SBM_TARAT_MIS_STAT2OFF,
   SBM_TARAT_MIS_STAT2RC,
   SBM_TARAT_MIS_STAT3K,
   SBM_TARAT_MIS_STAT3OFF,
   SBM_TARAT_MIS_STAT3RC,
   SBM_TARAT_MIS_STAT4K,
   SBM_TARAT_MIS_STAT4OFF,
   SBM_TARAT_MIS_STAT4RC,
   SBM_TARAT_MIS_DYN1K,
   SBM_TARAT_MIS_DYN1OFF,
   SBM_TARAT_MIS_DYN1RC,
   SBM_TARAT_MIS_DYN2K,
   SBM_TARAT_MIS_DYN2OFF,
   SBM_TARAT_MIS_DYN2RC,
   SBM_TARAT_MIS_DYN3K,
   SBM_TARAT_MIS_DYN3OFF,
   SBM_TARAT_MIS_DYN3RC,
   SBM_TARAT_MIS_DYN4K,
   SBM_TARAT_MIS_DYN4OFF,
   SBM_TARAT_MIS_DYN4RC,
   NUM_SUBMENUS_TARATURA
};

enum subMenusParLav {
   NUM_SUBMENUS_PARLAV
};


enum subMenusEditPars {
   SBM_EP_SEL_FUN,
   SBM_EP_UM,
#if 1
   SBM_EP_IMP_ENC,
#endif
   SBM_EP_SVIL_RUO,
   SBM_EP_RPM_MAND,
   SBM_EP_RPM_FRIZ,
   SBM_EP_LAMP_ACCESA,
   SBM_EP_LAMP_SPEGN,
   SBM_EP_ASSE_MAN_COL,
#if 1
   SBM_EP_DURATA_ALL,
#endif
   SBM_EP_TEST_INTRZ,
#if 1
   SBM_EP_PERCRO,
   SBM_EP_MAXBURSTSERR,
#endif
   SBM_EP_FS_ASSORB,
   SBM_EP_VERS_DATA,
   #if 0==1
   SBM_EP_NSEC_AVVIO,
   #endif
   #ifdef WriteSpezzoni
   SBM_EP_TEST,
   #endif

   NUM_SUBMENUS_EDIT_PARS
};


enum subMenusEditProg {
   SBM_ER_CODICE_PRG,
   SBM_ER_VEL_PROD,
   SBM_ER_POS_PRETAGLIO,
   SBM_ER_ASSORB_RUOTA,
   SBM_ER_COEFF_CORR,
   SBM_ER_CANCELLA,

   SBM_ER_QUAN_PROVE,

   NUM_SUBMENUS_EDIT_PROG
};

enum subMenusEditComm {
   SBM_EC_CODICE_COM,
   SBM_EC_CODICE_PRG_COM,
   SBM_EC_QUAN,
   SBM_EC_RES,
   SBM_EC_AUTOMATICO,
#ifdef defVisEcDiamRuota
   SBM_EC_DIAM_RUOTA,
   SBM_EC_DIAM_COLT,
#endif
   SBM_EC_CANCELLA,

   SBM_EC_PARAM_PRG,

   NUM_SUBMENUS_EDIT_COMM


};


enum subMenusSetup {
   SBM_EB_VISUALIZZA,
   SBM_EB_CODICE_COM,
   SBM_EB_RESSPEC,
   SBM_TARAT_POS_LAMA,
   SBM_EB_POS_PRETAGLIO,
   SBM_EB_DIAM_RUOTA,
   SBM_EB_DIAM_COLT,
   SBM_EB_NSPIRE_PRETAG,
   SBM_EB_MAX_VAR_RES,
   SBM_EB_CODICE_FOR,
   NUM_SUBMENUS_BOB
};


enum subMenusUtils {
   SBM_UT_ALR,
#ifndef defTest2Fili
   SBM_UT_LANG,
#endif
   SBM_UT_PASS,
#ifndef defTest2Fili
   SBM_UT_SERI,
   SBM_UT_TIPO_SERI,
   SBM_UT_DATA_ORA,
   SBM_UT_INDIG,
   SBM_UT_OUTDIG,
   SBM_UT_INANA,
   SBM_UT_OUTANA,
#endif
   NUM_SUBMENUS_UTILS
};


enum {
   STR_IMP_GIRO,
   STR_MM_GIRO,
   STR_INCH_GIRO,
   STR_RPM,
   STR_METRI_MINUTO,
   STR_FEET_MINUTO,
   STR_AMPERE,
   STR_MM,
   STR_INCH,
   STR_INCH_FEET,
   STR_NUM,
   STR_OHM,
   STR_OHM_PER_METRO,
   STR_OHM_PER_FEET,
   STR_PERC,
   STR_SEC,
   STR_MSEC,

   STR_PEZZI,
   STR_MEMORIZZATO,
   STR_RILEVATO,
   STR_NULLA,
   STR_PORTATA,

   STR_SELEZIONE_MENU,

   STR_EDIT_PARAMETRI,
   STR_IMP_GIRO_ENCODER,
   STR_SVILUPPO_RUOTA_MIS,
   STR_UNITA_DI_MISURA,
   STR_RPM_MANDRINO,
   STR_RPM_FRIZIONE,
   STR_SEL_FUNZIONAMENTO,
   STR_CON_MIS_OHMICA,
   STR_CON_LUNGHEZZA,
   STR_LAMP_ACC,
   STR_PEZZI_PRIMA_FINE,
   STR_LAMP_SPEGN,
   STR_SEC_DOPO_FINE,
   STR_N_SPIRE_PRETAGLIO,
   STR_MAX_VARIAZIONI_RES,
   STR_FORNITORE,
   STR_MAX_VARIAZIONE,

   STR_PROG_DI_LAVORO,
   STR_CODICE_PROG,
   STR_VEL_PROD,
   STR_ASSORB_RUOTA,
   STR_QUOTA_TAGLIO,
   STR_COEFF_CORR,

   STR_PRG_COMMESSA,
   STR_COMMESSA,
   STR_QUANTITA,
   STR_PEZZI_PROVE,
   STR_RES_SPIRALE,
   STR_RES_SPEC_FILO,
   STR_DIAM_RUOTA_C,
   STR_LAMA_TAGLIO,
   STR_CAMBIO_COMMESSA,
   STR_CAMBIO_AUTOMATICO,
   STR_CAMBIO_NO,

   STR_VISUALIZZAZIONE,
   STR_COMM_PUNTO,
   STR_PEZZI_DA_FARE,
   STR_PEZZI_FATTI,

   STR_TARATURA,
   STR_TARATURA_MIS_STATICA,
   STR_TARATURA_MIS_DINAMICA,
   STR_FATTORE_K,
   STR_OFFSET,
   STR_RES_CAMPIONE,

   STR_UTILITA,
   STR_SEL_LINGUA,
   STR_IMPOSTA_PASSW,
   STR_IMPOSTA_SERIALE,
   STR_BAUD,
   STR_PARITA,
   STR_IMPOSTA_CALENDARIO,
   STR_GIORNO,
   STR_MESE,
   STR_ANNO,
   STR_ORA,
   STR_MINUTO,
   STR_ING_DIG,
   STR_DIG,
   STR_OUT_DIG,
   STR_ING,
   STR_ANALOGICO,
   STR_OUT,
   STR_ITA_LANG,
   STR_ENG_LANG,
   STR_DEU_LANG,
   STR_BAUD0,
   STR_BAUD1,
   STR_BAUD2,
   STR_BAUD3,
   STR_BAUD4,
   STR_PARITY_NO,
   STR_PARITY_ODD,
   STR_PARITY_EVEN,

   STR_SI,
   STR_NO,
   STR_PASSW,
   STR_XXXXX,
   STR_IASSE,
   STR_CANCELLO,
   STR_ALR,
   STR_ALR_FRENI1,
   STR_ALR_FRENI2,
   STR_ALR_FRIZ1,
   STR_ALR_FRIZ2,
   STR_ALR_INTRZ1,
   STR_ALR_INTRZ2,
   STR_ALR_DRMAN,
   STR_ALR_NOTMIS,
   STR_ALR_TIMMIS,
   STR_ALR_TOLMIS,
   STR_ALR_PORTAT,
   STR_ALR_DRCOL,
   STR_ALR_NOALR,
   STR_RES_BOB,
   STR_DURATA_ALL,
   STR_POS_LAMA,
   STR_NSEC_AVVIO,
   STR_TEST_INTRZ,
   STR_CODICE_FOR,
   STR_EDIT_PARLAV,
   STR_VERSIONE,
   STR_TIPO_SERIALE,
   STR_TIPO_SER_232,
   STR_TIPO_SER_485,
   STR_EP_FS_ASSORB,
	STR_ER_PERCRO,
	STR_ER_MAXBURST,
   STR_VERS_DATA,


   NUM_STR
};

enum ums {
   UM_MM,
   UM_IN,

   NUM_UM
};

#define KEY_0  0xb
#define KEY_1  0xf
#define KEY_2  0x13
#define KEY_3  0x23
#define KEY_4  0x1
#define KEY_5  0x5
#define KEY_6  0x9
#define KEY_7  0xd
#define KEY_8  0x11
#define KEY_9  0x21
#define KEY_CLR 0xa
#define KEY_DN 0xe
#define KEY_UP 0x20
#define KEY_RG 0x10
#define KEY_LF 0xc
#define KEY_ENTER 0x7
#define KEY_ESC 0x3
#define KEY_F1 0x24 /* e` 0 ma lo 0 crea problemi in gestione */
#define KEY_F2 0x4
#define KEY_F3 0x8
#define KEY_F4 0x2
#define KEY_F5 0x6
#define KEY_SIGN  0x22
#define KEY_POINT 0x12

#define KED_0  0
#define KED_1  1
#define KED_2  2
#define KED_3  3
#define KED_4  4
#define KED_5  5
#define KED_6  6
#define KED_7  7
#define KED_8  8
#define KED_9  9
#define KED_DN 10
#define KED_UP 11
#define KED_RG 12
#define KED_LF 13
#define KED_ENTER 14
#define KED_ESC   15
#define KED_F1    16
#define KED_F2    17
#define KED_F3    18
#define KED_F4    19
#define KED_F5    20
#define KED_SIGN  21
#define KED_POINT 22
#define KED_CLR   23

#define KB_DATA_AVA 0x80
#define KB_MASK_KEY 0x7f


#define FLG_PRINT_CLEAR  0x1
#define FLG_PRINT_STR    0x2
#define FLG_PRINT_ENUM   0x4

/* Maschere che permettono di identificare nel carattere macparms.flag
   se una particolare funzionalit� � on o off. */
#define FLG_PAR_MRES 0x1
#define FLG_PAR_MSTA 0x2
#define FLG_PAR_VQ   0x4
#define FLG_PAR_VV1  0x8
/* Flag che indica se la macchina � predisposta al
   funzionamento automatico o no. */
#define FLG_PAR_AUT  0x10
/* Questo flag � inutile, dato che si lavora con una sola torretta
   #define FLG_PAR_VV2  0x10
*/

#define FLG_SPEZ_INIT 0x1

enum modiMisura {
   MODE_LEN,
   MODE_LENR,

   NUM_MIS_MODES
};

/* TH1 = 256 - (K * OscFreq) / (384 * BaudRate) con OscFreq = 10MHz e SMOD = 0 => K = 1 (SMOD = 1 => K = 2) */

enum baudRates {
   BAUD_1200,
   BAUD_2400,
   BAUD_4800,
   BAUD_9600,
   BAUD_19200,

   NUM_BAUD_RATES
};

enum parities {
   PARITY_NO,
   PARITY_ODD,
   PARITY_EVEN,

   NUM_PARITIES
};


#define IMP_TO_MM(imp) ((float)imp * impToMm)
#define IMP_TO_UNIT(imp) ((float)imp * impToUnit)
#define UNIT_TO_IMP(val) (val / impToUnit)
#define MM_TO_OHM(i, mm) (mm * actRo[i] / 1000.)
#define DIG_TO_VOLT(n) ((float)n * VOLT_PER_DIG)

enum statiAvvolgimento {
   AVV_IDLE,
   AVV_WAIT_PRE,
   AVV_WAIT_TAG,
   AVV_WAIT_TAGLIO,

   NUM_STATI_AVVOLGIMENTO
};

enum statiMisura {
   MIS_OFF_LINE,
   MIS_COMM_MUX,
   MIS_START_MIS,
   MIS_TEST_VALID,
   MIS_CALC_RO,
   MIS_MEDIA_RO,
   MIS_WAIT_CHANGE,
   MIS_WAIT_INVALID,

   NUM_STATI_MISURA
};

// Periodo di tempo (in millisecondi) che intercorre fra due successive
//   interrupt per overflow del timer 0, usato per routine di interrupt di gestione misura ohmica. 
#define PeriodoIntMs 5L

// Periodo di tempo (in millisecondi) che intercorre fra due successive
//   interrupt per overflow del timer 1, usato per supervisione i2c bus. 
#define defPeriodoInterruptCtrlI2cbus_ms 1


#define mymax(A,B) (((A)>=(B))?(A):(B))
#define mymin(A,B) (((A)>=(B))?(B):(A))

#define SLEEP_TAGLIO mymax(10/PeriodoIntMs,1)
#define SLEEP_CHANGE mymax(30/PeriodoIntMs,1)
#define SLEEP_STABIL mymax(10/PeriodoIntMs,1)
#define SLEEP_VALID  mymax(10/PeriodoIntMs,1)
/* Numero di millisecondi da aspettare tra quando arriva il segnale di
   misura valida e quando tale misura e' stabile.
   Inoltre si attende anche di passare alla portata corretta.
*/
#define SLEEP_RAMPA_E_PORTATA mymax(30/PeriodoIntMs,1)
/* Numero di cicli di interrupt da aspettare tra due letture consecutive dall' adc. */
#define SLEEP_STABIL_NEXT mymax(1/PeriodoIntMs,0)
/* Durata massima del comando di taglio: attualmente 400 ms.
*/
#define SLEEP_MAX_TAGLIO mymin(mymax(200/PeriodoIntMs,1),255)
/* Durata del pretaglio nel caso di taglio manuale.
   Attualmente 15/100 s, vale a dire 150 ms. */
#define SLEEP_TAGLIO_ASINC mymax(150/PeriodoIntMs,1)
#define SLEEP_TAGLIO_PRIMO_PEZZO mymin((1000U/PeriodoIntMs),200)

#define INTERVAL_TEST_ALARMS mymax(40/PeriodoIntMs,1) /* slices=numero di cicli di interrupt */
#define INTERVAL_TEST_LAV mymax(40/PeriodoIntMs,1) /* slices  */
#define INTERVAL_TEST_MIS mymax(40/PeriodoIntMs,1) /* slices  */
#define INTERVAL_UPD_7SEG mymax(100/PeriodoIntMs,1) /* slices */


#define defNumBitAD7327 (13)
#define defMaxValueAD7327 ((1<<defNumBitAD7327)-1)
#define defHalfValueAD7327 ((1<<defNumBitAD7327)/2-1)


/* Valore massimo della misura effettuata dall' adc converter.
   Questo valore indica un overflow  (che il programma interpreta
   come un overrange se pu� cambiare scala).
   Dato che il ns convertitore � a 13 bit, pero' solo l'80% pu� essere usato
   80*8191/100--> 6552, questa � una buona soglia...
   tolgo 10 step per sicurezza...
   
*/
#define MIS_DIG_MAX ((80L*8191L)/100-10) 

/* Modifica del 02/02/1996: il numero delle torrette sar� sempre e solo 1.
   Non si vorr� mai pi� gestire la seconda torretta. */
#define MAX_TORRETTE 1
#define MIN_ANTI_PRETAGLIO 50 /* impulsi */
#define RITARDO_MISURA 1000 /* impulsi */

#define NUM_PORTATE 4
#define NUM_PORTATE_DINAMICHE 5
#define PORTATA1 10000
#define PORTATA2 1000
#define PORTATA3 100
#define PORTATA4 10
#define PORTATA5 1
#define FONDO_SCALA_OHM_MIN .5 /* moltiplicare per PORTATAX */

/* Termine additivo di arrotondamento utilizzato nelle conversioni da
   floating point a intero. */
#define ROUND 0.4999999

#define ADDR_ENC1       0xFF06
/* Questo encoder non viene utilizzato, perch� si usa una sola torretta
    #define ADDR_ENC2   0xFF04
*/
#define ADDR_INP_DIG    0x9000
#define ADDR_OUT_DIG    0x9002
// indirizzo del dac che imposta la portata dinamica
// il dac � interfacciato col micro tramite bus parallelo
// si usa il CS5, che viene generato dalla logica psd del upsd
#define ADDR_DACPOR1    0xB000

#define ADDR_ADC        0xFF40
#define ADDR_ADCSTA     0x800B
#define ADDR_KBDATA     0xFE80
#define ADDR_KBENOU     0x800E
#define ADDR_7SEGDT     0xFE40
#define ADDR_7SEGPT     0xFE80
/* Neanche questo dac non viene pi� usato, dato che esiste una sola torretta
    #define ADDR_DACPOR2        0xFFC0
*/


/* Questa define viene usata solo per inizializzare il valore
   di imptomm nel caso in cui il programma sia eseguito su pc, ma credo
   che in modo subdolo influenzi anche il calcolo degli anticipi nelle
   azioni di taglio e pretaglio. */
#define MM_PER_IMPULSO 0.1
/* Cosa significa questa costante?
 * E' la distanza percorsa dal filo per andare dalla ruota di misura al mandrino
 * Quando si hanno due fili, il secondo filo può arrivare con un percorso diverso
 * Ci sono al max due percorsi diversi del filo, sarebbe opportuno che fosse possibile indicare esplicitamente quale percorso fanno i fili, oppure si potrebbe autoselezionare in base al diametro
 * */
#define MM_RUOTA1_AVV 811.0

#define MM_RUOTA1_AVV 811.0

#define ALR_FILOAGGROV 0x1
#define ALR_FRENI2 0x2
#define ALR_FRIZ1  0x4
#define ALR_FRIZ2  0x8
#define ALR_INTRZ1 0x10
#define ALR_INTRZ2 0x20
#define ALR_DROK   0x40
#define ALR_NOTMIS 0x80
#define ALR_TIMMIS 0x100
#define ALR_TOLMIS 0x200
#define ALR_PORTAT 0x400
#define ALR_ELETTR 0x800
#define ALR_USB_DISK  0x1000
#define ALR_SD_CARD   0x2000
#define ALR_I2C_BUS   0x4000
#define ALR_TEMPERATURE      0x08000
#define ALR_WIRE_TANGLED     0x10000

/* Unit� di resistenza programma: rappresenta il valore normalizzato
   della resistenza da produrre. Inoltre nella routine
   di servizio dell' interruzione da timer 0 non si possono usare numeri
   float, per cui questa scappatoia � necessaria. */
#define TARGET_URP 10000
/* Coefficiente moltiplicativo della lunghezza che permette di ottenere una
   mantissa con molti bit significativi. Dato che la lunghezza della spira
   � dell' ordine del metro (i.e. 1000 mmm), moltiplicando per MAGN_LEN
   si ottiene 100 000 000, quindi tramite un intero lungo si riesce a
   rappresentare la lunghezza con una precisione del centesimo di millimetro.
*/
#define MAGN_LEN 100000UL
/* Fattore di conversione del valore letto da convertitore in tensione.
   Invece di usare un float, uso un intero moltiplicando il risultato
   per 100000 (i.e. MAGN_LEN). */
#define ADC_TO_TENS 1221 /* 5 * 100000 / 4096 (misura 12 bit -> ?Volts tramite prodotto) */
/* Da quanti urp � composto un ohm? Da 10000.
   Ogni urp vale quindi 1/10000 di ohm. Invece di usare questo fattore
   che � un numero floating point, uso come unit� di resistenza il microohm;
   in questo modo un urp vale 1/10000*1 000 000 microohm= 100 microohm.
   Questo coefficiente converte pertanto gli urp in microohm.
*/
#define RES_PER_URP 100  /* 1000000 / 10000 (microOhms per urp) */

#define MAX_ULONG 0xffffffff
#define FONDO_SCALA_TENSIONI 2 /* volts */

enum {
   EDIT_NUMBER,
   EDIT_STRING,
   EDIT_ENUM,
   EDIT_PICTURE,
   EDIT_PASSW
};

#define SEG7_COLS 4

#define SEG7_DT1 0x1
#define SEG7_DT2 0x2
#define SEG7_DT3 0x4
#define SEG7_DT4 0x8
#define SEG7_DS1 0x10
#define SEG7_DS2 0x20
#define SEG7_CS1 0x40
#define SEG7_CS2 0x80

#define LEN_PASSWORD 5

#define rxOpcode serRx.p[2]

#define OPC_MOD_CONT '0'
#define OPC_REQ_LCOM '1'

#define LEN_EXTRA_TEXT 4

#define CLK40

#ifdef CLK40
	#undef CLK12
	#undef CLK16
	#undef CLK10
   #define CLOCK 40. /* MHz */
   /* Numero di conteggi del timer prima della ricarica nella procedura
      timer(). */
   #define NumConteggiT0 120L

  	// Calculate timer rollover based on FREQ_OSC to be PeriodoIntMs periods (1000hz)
	#define defTimerRolloverValue (0x10000 - ( ((FREQ_OSC * 5L*PeriodoIntMs-NumConteggiT0) / 6L) - 17L)/10)

   #define VAL_TH0 ((defTimerRolloverValue)>>8)
   #define VAL_TL0 ((defTimerRolloverValue)&0x00FF)
   /* 65535 - 1000 = 64536 (0xFC17) + task switch + operazioni iniziali;
      con clock 10 MHz devo avere un interrupt ogni 1000
      conteggi per ottenere 1 ms  (con 12 MHz) */
   #define TH1_1200 204
   #define TH1_2400 230
   #define TH1_4800 243
   #define TH1_9600 249
   /* Valori per i registri rcap (uso timer 2 per generare baud rate). */
   #define RCAP2H_1200 0xFE
   #define RCAP2L_1200 0xC8

   #define RCAP2H_2400 0xFF
   #define RCAP2L_2400 0x64

   #define RCAP2H_4800 0xFF
   #define RCAP2L_4800 0xB2

   #define RCAP2H_9600 0xFF
   #define RCAP2L_9600 0xD9

   #define RCAP2H_19200 0xFF
   #define RCAP2L_19200 0xEC
#endif

#ifdef CLK12
   #define CLOCK 12. /* MHz */
   /* Numero di conteggi del timer prima della ricarica nella procedura
      timer(). */
   #define NumConteggiT0 120L
   #define VAL_TH0 ((0xFFFF-(PeriodoIntMs*1000L-NumConteggiT0))&0xFF00)>>8
   #define VAL_TL0 (0xFFFF-(PeriodoIntMs*1000L-NumConteggiT0))&0x00FF
   /* 65535 - 1000 = 64536 (0xFC17) + task switch + operazioni iniziali;
      con clock 10 MHz devo avere un interrupt ogni 1000
      conteggi per ottenere 1 ms  (con 12 MHz) */
   #define TH1_1200 204
   #define TH1_2400 230
   #define TH1_4800 243
   #define TH1_9600 249
   /* Valori per i registri rcap (uso timer 2 per generare baud rate). */
   #define RCAP2H_1200 0xFE
   #define RCAP2L_1200 0xC8

   #define RCAP2H_2400 0xFF
   #define RCAP2L_2400 0x64

   #define RCAP2H_4800 0xFF
   #define RCAP2L_4800 0xB2

   #define RCAP2H_9600 0xFF
   #define RCAP2L_9600 0xD9

   #define RCAP2H_19200 0xFF
   #define RCAP2L_19200 0xEC

#else
   #ifdef CLK16
      #define CLOCK 16. /* MHz */
      /* Numero di conteggi del timer prima della ricarica nella procedura
         timer(). */
      #define NumConteggiT0 90
      #define VAL_TH0 ((0xFFFF-(PeriodoIntMs*1000L-NumConteggiT0)*16/12)&0xFF00)>>8
      #define VAL_TL0 (0xFFFF-(PeriodoIntMs*1000L-NumConteggiT0)*16/12)&0x00FF

      #define TH1_1200 213
      #define TH1_2400 234
      #define TH1_4800 245
      #define TH1_9600 251

      /* Valori per i registri rcap (uso timer 2 per generare baud rate). */
      #define RCAP2H_1200 0xFE
      #define RCAP2L_1200 0x5F

      #define RCAP2H_2400 0xFF
      #define RCAP2L_2400 0x30

      #define RCAP2H_4800 0xFF
      #define RCAP2L_4800 0x98

      #define RCAP2H_9600 0xFF
      #define RCAP2L_9600 0xCC

      #define RCAP2H_19200 0xFF
      #define RCAP2L_19200 0xE6
   #endif
#endif



/* Valore nominale della coppia motrice delle frizioni (in %).
   Si usa il 50% solo perch� � il valore centrale fra 0 e 100%, quindi
   � agevole da modificare via potenziometro.
*/
#define VAL_NOMINALE_COPPIA 50

/* Numero di frizioni che agiscono sul sistema mandrino_filo. */
#define NUM_FRIZIONI         2

/* Numero di potenziometri per la regolazione fine dei parametri. */
#define NUM_POTENZIOMETRI    5



/*--Device--------------------------------------------------------------*/
/*                                                                      */
/* Convertitore ADC.                                                    */
/*                                                                      */
/*----------------------------------------------------------------------*/

/* Numero di bit usati dall' ADC nella conversione. */
#define N_BIT_ADC     13
/* Fondo scala dell' ADC. 
   FONDO SCALA VA DA 0 A 10 v NORMALMENTE
   1 BIT, QUELLO DEL SEGNO, � PERSO, ALMENO MI SEMBRA DALLE PROVE FATTE
   INOLTRE LA TENSIONE ARRIVA MAX A 5V, PERTANTO PERDO DUE BIT...
*/
#define FS_ADC        ((float)(1L<<(N_BIT_ADC-1)))
/* Fondo scala dell' ADC nelle misure bipolari. */
#define FS_ADC_BIPOLARI        (float)(1L<<(N_BIT_ADC-2))
/* Posizione del bit che indica conversione bipolare nell' ADC. */
#define BIT_BIP_ADC   0x08
/* Al massimo vengono assorbiti 3.8 Amp�re di corrente
   dai motori delle ruote di contrasto. */
#define FS_AMPERE_ASSORB 3.8

// FONDO SCALA DELLA PORTATA DINAMICA
// MASSIMO ARRIVO A 8V SU 10V DI FONDO SCALA
#define MAX_FS_ADC_DINAMICA  ((8L*8191L)/10)


/* Fondo scala massimo nella conversione da misura statica: non vengono
   usati tutti i bit a disposizione. */
//#define MAX_FS_ADC_STATICA  3276
// 80% del fondo scala...
// era cosi' #define MAX_FS_ADC_STATICA  ((80L*4095L)/100)
// attenzione! La portata della misura statica � stata raddoppiata a 10V
// pertanto il fondo scala � 80% di 10V=80%di 8191 step=13bit!
#define MAX_FS_ADC_STATICA  ((80L*8191L)*0.01)
#define FS_ADC_STATICA  (8191L)
// commuto quando passo sotto il tot% di portata
// praticamente la formula � questa
// portataAlta=10*portataBassa
// per evitare di correre tra le portate deve essere
// sogliaLow*PortataAlta<sogliaHi*PortataBassa
// sogliaLow*10*PortataBassa<sogliaHi*PortataBassa
// quindi sogliaLow<sogliaHi/10, mettiamo 50% di questo valore
// cambiato da 5% a 9.5%
#define MIN_FS_ADC_STATICA  ((MAX_FS_ADC_STATICA*95)/1000)
/* Numero di misure acquisite dall' ohmetro statico prima di
   visualizzare la misura. */
#if 0
	#ifdef CLK16
		#define N_SAMPLE_RES_STA 64
	#else
		#define N_SAMPLE_RES_STA 32
	#endif
#else
	#define N_SAMPLE_RES_STA 64
#endif


/* Numero di canali che possono essere tarati;
   il primo canale � quello dell' ohmetro sulla torretta per le misure
      dinamiche;
   il secondo canale � quello dell' ohmetro per le misure statiche
   ...
*/
#define NCANALI_TARABILI 2
#define N_CANALE_OHMETRO_STA  	0
#define N_CANALE_OHMETRO_DYN 	1
#define N_CANALE_V_MAN 		2
#define N_CANALE_ASSORB1  	3
#define N_CANALE_ASSORB2  	4
#define N_CANALE_V1  		5 /* Canale velocit� ruota 1. */
#define N_CANALE_V2  		6 /* Canale velocit� ruota 2. */

#define VOLT_PER_DIG  1.2207e-3 /* 5 / 4096 */

/*--Device--------------------------------------------------------------*/
/*                                                                      */
/* Encoder.                                                             */
/*                                                                      */
/*----------------------------------------------------------------------*/

/* Numero delle fasi dell' encoder: questo coefficiente va a moltiplicare
   il numero di impulsi/giro dell' encoder per ottenere il numero di effettivo
   di impulsi spediti in un giro.
   Questo fattore moltiplicativo viene usato solo durante la fase di visualiz-
   zazione/modifica dei parametri macchina.
*/
#define N_FASI_ENCODER 4


/* Fattore di conversione da pollici in millimetri. */
#define INCH_TO_MM 25.4
/* Fattore di conversione da millimetri in pollici. */
#define MM_TO_INCH (1/INCH_TO_MM)

/* Fattore di conversione da metri a feet.
   1 feet=0.3049 metri
   1 metro=3.28 feet
*/
#define metri2feet  3.28
#define feet2metri  (1/metri2feet)


/* Define che permettono di determinare/modificare lo stato della lampada.
   LampOnOff indica che la lampada sta lampeggiando per indicare la prossima
   fine del lotto.
   LampAccesaPeriodo indica che la lampada se ne sta accesa per il periodo
   di tempo indicato dall' utente, dopodich� si spegne.
   LampCount serve per determinare se la lampada � in uno dei due stati
   precedenti, nei quali si deve contare il tempo che scorre.
   LampAccesa indica che la lampada � accesa fino al prossimo start.
*/
#define LampSpenta              0x00
#define LampOnOff               0x01
#define LampAccesaPeriodo       0x02
#define LampCount               ((LampOnOff)|(LampAccesaPeriodo))
#define LampAccesa              0x04
/* Durata del semiperiodo del lampeggio della lampada, in numero di interruzioni. */
#define SemiperiodoLampeggioLampada (500L/PeriodoIntMs)


// macro che permette di aggiornare il valore in uscita verso il dac parallelo
// ad558; ha un tempo di accesso di almeno 200ns, mentre upsd non pu� fare meno di 175ns
// e per default fa 100ns, allora divido il clock per 4, quindi i miei 100ns diventano 400ns
// e riesco a gestire ad558 senza problemi
//		CCON0|=0X02;   abbasso la frequenza di clock di un fattore 4
//		CCON0&=~0X07;  ripristino il clock 
#ifdef ARM9platform

#endif

extern unsigned int ui_act_dac_port_value;


// numero max di percentuali di distanziazione per codice prodotto
#define defMaxNumOfPercDistanzia 20
//#define defMaxNumOfPercDistanzia 10


typedef struct _TipoStruct_timeout{
	unsigned char uc_cnt_interrupt;
	unsigned char uc_prev_cnt_interrupt;
	unsigned int ui_time_elapsed_cnt;
}TipoStruct_timeout;

// inizializzazione gestione timeout
void timeout_init(TipoStruct_timeout *p);

// verifica se timeout scaduto
unsigned char uc_timeout_active(TipoStruct_timeout *p, unsigned int ui_timeout_ms);





#endif /* _SPICOST */
