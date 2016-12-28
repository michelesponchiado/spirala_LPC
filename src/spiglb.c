#include "spityp.h"
#include "spicost.h"
#include "spiext.h"
#include "fpga_nxp.h"
/* Definizione del tipo di memoria in cui devono essere allocate le variabili
   cui si dovrebbe accedere rapidamente. Se si usa un 8052, o comunque un
   micro che ha spazio per lo stack, allora ho memoria di tipo data a
   disposizione, altrimenti si usa xdat. */
#ifndef ARM9platform
    #ifdef C52
      #define dat_xdat xdata
    #else
      #define dat_xdat xdat
    #endif
#endif
SPEZ_FILO spezFilo;
SER_RX serRx;
SER_TX serTx;

/* gestione menu */
unsigned char actMenu;
unsigned char actSubmenu;
unsigned char actMenuBck;
unsigned char actSubmenuBck;
unsigned char lastSubmenu[NUM_MENUS];

// variabile che contiene tutte le informazioni non volatili della spiralatrice
TipoNonVolatileRamStruct nvram_struct __attribute__ ((section ("AZ4186_NVRAM_SECTION"))) ;

/* programmi */
#if 0
// unsigned char actPrg;
PROGRAMMA prgList[NUM_PROGRAMMI]
#ifndef _CC51
= {
   {
     "  ",                /* Codice programma. */
     0,                   /* Numero del programma successivo. */
     0,                   /* Numero del programma precedente. */
     10.0,                /* Velocit… di produzione in m/min. */
     2000.,               /* Giri/minuto mandrino. */
     {1.0,1.0},           /* Assorbimento delle frizioni in ampŠre. */
     0.0,                 /* Posizione di pretaglio. */
     0.45,                /* Diametro del filo/i in mm. */
     1.7,                 /* Diametro del mandrino in mm. */
     1,                   /* Numero di fili: min NUM_MIN_FILI max MAX_NUM_FILI. */
     0.0,                 /* Coeff. corr. */
     {100,100},           /* Coppia delle frizioni. */
     {120,120},           /* Rpm frizioni */
     0,                   /* Codice del fornitore. */
     0,                   /* Delta r. */
     0,                   /* Indicatore di programma a parametri vuoti. */
     5.0                  /* Velocita' di produzione "vera". */
   }
}
#endif
;
#endif


/* parametri */
MACPARMS macParms
#ifndef ARM9platform
    #ifndef _CC51
    = {
       4000,                /* Impgiro */
       424.75,              /* mm/giro */
       {174.,174.},         /* rpm_frizioni */
       6000,                /* rpm mandrino. */
       2000,                /* len semigiro */
       1,                   /* modo */
       3,                   /* flags */
       3,                   /* nPezzi avviso */
       10,                  /* nsec lampada accesa */
       10,                  /* Numero di spire di pretaglio. */
       {27.2,34.4},         /* Interasse coltello-mandrino, alla posizione pi—
                   bassa e pi— alta. */
       {85.3,80.},          /* Diametro delle frizioni in mm. */
       59.5,                /* Diametro del coltello in mm. */
       2                    /* Numero di secondi di attesa prima dell' avvio. */
       }
    #endif
#endif
 ;

/* Taratura dei canali analogici. Non Š unica per ogni canale,
   ma ne esiste una per ogni portata.
   Per ora Š prevista la taratura dei soli canali 0 (misura dinamica)
   ed 1 (misura statica).
*/
/* Variabile che ha il compito di puntare al membro dell' array taratura
   che viene utilizzato per la misura. */
TARATURA * xdat pTar;
unsigned char actCan;

/* seriale */
SERIALE serParms;
bit txNewPz;
bit txModCont;
bit txBusy;
bit txReqLocalCommessa;
bit rxHasMsg;
bit parIsOdd;
bit parErr;
bit isPressed;
bit criticLav;
/* Semaforo che regola l' accesso all' adc converter. */
bit criticMis;
/* Semaforo che regola l' accesso alle variabili gapTime e velocit…. */
bit updVel;
bit checkPassw;
bit checkedPassw;
bit myTI;
bit watchDog;
/*bit txTest;*/
/*  Bit che vale 1 se il lotto corrente Š stato terminato correttamente,
    cioŠ sono stati eseguiti tutti i pezzi richiesti. */
bit RunningPrgEndOk;
/*  Bit che vale 1 se il lotto corrente Š stato terminato correttamente,
    ed il prossimo programma pu• essere eseguito in automatico senza
    fermo macchina. */
bit NextPrgAutomatic;
/*  Bit che vale 1 se c' Š un programma in esecuzione. */
bit PrgRunning;
/*  Indicatore di esecuzione taglio asincrono. */
bit TaglioAsincrono;
/*  Indicatore di conteggio pezzi prodotti. */
bit BitContaPezzi;

/* mappa I/O digitali */
unsigned short myInpDig;
unsigned int data outDigVariable;
unsigned char data outDigMis;

/* variabili per la misura */
unsigned char misStatus;
unsigned char misStaStatus;
unsigned char avvStatus;
unsigned char backupAvvStatus;
unsigned long lastPzLenImp;
unsigned long targetLenImp;

/* Variabili che sarebbe bene mettere in data ram, se possibile. */
dat_xdat unsigned long actSpezInPez;  /* 4 byte */
dat_xdat short    int   gapImp;       /* 2 byte */
dat_xdat unsigned short newEnc;       /* 2 byte */
dat_xdat unsigned short oldEncLav;    /* 2 byte */
dat_xdat unsigned long actSpezAvv;    /* 4 byte */
dat_xdat unsigned long actLenImp;     /* 4 byte */
dat_xdat unsigned long resLastSpezUp; /* 4 byte */
/* Resistenza del filo. */
dat_xdat unsigned long actRes;        /* 4 byte */
dat_xdat unsigned long actRo;         /* 4 byte */
dat_xdat unsigned long preLenImp;     /* 4 byte */
dat_xdat unsigned short oldEncMis;    /* 2 byte */
dat_xdat unsigned long impFromMis;    /* 4 byte */

dat_xdat unsigned char cntAvv;        /* 1 byte */
dat_xdat unsigned char cntMis;        /* 1 byte */
dat_xdat unsigned char misStaSleep;   /* 1 byte */
dat_xdat unsigned char upd7segSleep;  /* 1 byte */
dat_xdat unsigned char avvSleep;      /* 1 byte */
dat_xdat unsigned char misSleep;      /* 1 byte */

			     /* Totale  46 byte */


unsigned long actCorrOhmica;
unsigned long nomRo;
float targetRes;
float impToMm;
float impToUnit;
unsigned short actNumPezzi;
// unsigned char uiGetRunningPrg();
unsigned char rdQueSpez;
unsigned char wrQueSpez;
unsigned short misDig;
float misStaVolt;
unsigned char actPortata;
unsigned char actStaPortata;
unsigned short portate[NUM_PORTATE_DINAMICHE] = {PORTATA1, PORTATA2, PORTATA3, PORTATA4, PORTATA5};
unsigned short lastPzRes;
float resSta;
/* URP = Unita` Resistenza Programma (rTarget => TARGET_URP) */
unsigned long urpToRes;
unsigned long minRo;
unsigned long maxRo;
unsigned short nSpezDelay;
unsigned short chunkAvv;
float mmRuotaAvv = MM_RUOTA1_AVV;
bit withMis;
unsigned char actPreTime; /* s/100 */
unsigned char actTagTime; /* s/100 */
#ifndef ARM9platform
#ifndef _CC51
  unsigned short deltaimpulsi;
#endif
#endif
/* Numero di filo attualmente selezionato: variabile usata nella fase
   di visualizzazione/modifica dei dati di programma. */
unsigned char actFilo;
/* Numero di frizione attualmente selezionato: variabile usata nella fase
   di visualizzazione/modifica dei dati di programma. */
unsigned char actFrizione;

#ifdef WriteSpezzoni
TipoInfotest  RspecTestSpez[MAXPEZZITEST][MAXSPEZZONITEST];
#endif

unsigned int NumSpezzone;


unsigned short encVal;


/* locazioni delle porte i/o */
#ifndef _CC51
unsigned short inpDigSim;
unsigned char outDigSim;
#endif


unsigned short* pInpDig =
#ifndef _CC51
&inpDigSim;
#else
(unsigned short*)ADDR_INP_DIG;
#endif



/* locazione dei DAC di misura dinamica */
#ifndef _CC51
unsigned short ADCSim;
unsigned short ADCStaSim;
unsigned char  ADCSim1[10];
unsigned char  DACPortSim;
unsigned short* pADCDin =
&ADCSim;
#endif

unsigned char* pADC =
#ifndef _CC51
ADCSim1;
#else
(unsigned char*)ADDR_ADC;
#endif

unsigned short* pADCSta =
#ifndef _CC51
&ADCStaSim;
#else
(unsigned short*)ADDR_ADCSTA;
#endif

unsigned char* pDACPort =
#ifndef _CC51
&DACPortSim;
#else
(unsigned char*)ADDR_DACPOR1;
#endif


unsigned short alarms;
xdat unsigned char alr_status;

/* gestione tastiera */
char lastKeyPressed;

#ifndef _CC51
unsigned char kbDataSim;
unsigned char kbEnabOut;
#endif

unsigned char* pKbData =
#ifndef _CC51
&kbDataSim;
#else
(unsigned char*)ADDR_KBDATA;
#endif

unsigned char* pKbEnOu =
#ifndef _CC51
&kbEnabOut;
#else
(unsigned char*)ADDR_KBENOU;
#endif


/* gestione 7seg */
#ifndef _CC51
unsigned char seg7DataSim[10];
unsigned char seg7ExtraSim;
#endif

unsigned char* p7segData =
#ifndef _CC51
seg7DataSim;
#else
(unsigned char*)ADDR_7SEGDT;
#endif

unsigned char seg7Point;
unsigned char* p7segPoint =
#ifndef _CC51
&seg7ExtraSim;
#else
(unsigned char*)ADDR_7SEGPT;
#endif

char str7seg[LEN_BUF_7SEG];
char* act7segChar;
unsigned char act7segCol;

bit isEdit;
xdata TipoStructEdit edit;


unsigned char xdat AlarmOnTimeout;

unsigned long nImpulsi = 0; /* velocita` in impulsi / millisecondo */
unsigned long newnImpulsi,oldnImpulsi; /* velocita` in impulsi / millisecondo */
float velMMin = 0.;       /* velocita` in metri/minuto */
unsigned long gapT = 0; /* tempo passato dall'ultimo passaggio in conteggi
			    di numero di interruzioni intrno */
unsigned long oldgapT,newgapT ;

unsigned long spazio = 0;
unsigned long tempo = 0;

unsigned char xdata password[LEN_PASSWORD+1];
unsigned char xdata newpassw[LEN_PASSWORD+1];


unsigned short anticipoPretaglio;
/* Stato della lampada: deve essere scrutato/modificato tramite le define
   Lamp... in spicost.h. */
unsigned char  LampStatus;
/* Contatore del tempo per il controllo della lampada. */
unsigned long  TempoLampada;

unsigned char rtcSec,rtcMin,rtcHou,rtcBck,rtcDay,rtcMon,rtcYea;

#ifndef _CC51
unsigned char   P0      ;
unsigned char   SP      ;
unsigned char   DPL     ;
unsigned char   DPH     ;
unsigned char   PCON    ;
unsigned char   TCON    ;
unsigned char   TMOD    ;
unsigned char   TL0     ;
unsigned char   TL1     ;
unsigned char   TH0     ;
unsigned char   TH1     ;
unsigned char   RCAP2H     ;
unsigned char   RCAP2L     ;
unsigned char      RCLK;
unsigned char      TCLK;
unsigned char      T2CON;
unsigned char      TR2;
unsigned char   P1      = 0x80;
unsigned char   SCON    ;
unsigned char   SBUF    ;
unsigned char   P2      ;
unsigned char   IE      ;
unsigned char   P3      ;
unsigned char   IP      ;
unsigned char   PSW     ;
unsigned char   ACC     ;
unsigned char   B       ;
bit     IT0     ;
bit     IE0     ;
bit     IT1     ;
bit     IE1     ;
bit     TR0     ;
bit     TF0     ;
bit     TR1     ;
bit     TF1     ;
bit     RI      ;
bit     TI      ;
bit     RB8     ;
bit     TB8     ;
bit     REN     ;
bit     SM2     ;
bit     SM1     ;
bit     SM0     ;
bit     EX0     ;
bit     ET0     ;
bit     EX1     ;
bit     ET1     ;
bit     ES      ;
bit     EA      ;
bit     RXD     ;
bit     TXD     ;
bit     INT0    ;
bit     INT1    ;
bit     T0      ;
bit     T1      ;
bit     WR      ;
bit     RD      ;
bit     PX0     ;
bit     PT0     ;
bit     PX1     ;
bit     PT1     ;
bit     PS      ;
bit     P       ;
bit     OV      ;
bit     RS0     ;
bit     RS1     ;
bit     F0      ;
bit     AC      ;
bit     CY      ;
#endif
