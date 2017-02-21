/* Numero di default di secondi da aspettare prima dell' avvio. */
   #define DEFAULT_NSEC_AVVIO  ((char)3)
/* Valori di default al reset della ram */
   #define defPezziAvvisoDefault 10
   #define defSecLampSpegnDefault 15
   #define defNspirePretDefault 15
   #define defMmGiroDefault 426.5
   #define defImpGiroDefault 4000.0
   #define defRpmFrizioniDefault 300 /* Max rpm frizioni. */
   #define defRpmMandrinoDefault 6000   /* Max rpm mandrino. */
   #define defDurataAllDefault 350 /* Durata minima allarme per antirimbalzo. */
/* Distanza in mm tra l' asse del foro da cui esce la spirale e l' asse
   del coltello col coltello alla posizione pi� bassa. */
        #define AsseSpi_AsseColt_mm  22.9
/* Corsa del coltello dalla posizione bassa alla posizione alta in mm.
   (Default).
*/
        #define DefaultCorsaColtello         11
   #define defDiamTestIntrzDefault 0.55
   #define defIasse0Default  AsseSpi_AsseColt_mm 
   #define defIasse1Default  AsseSpi_AsseColt_mm+DefaultCorsaColtello
   #define defDiamFriz0Default 80
   #define defDiamFriz1Default 80
   #define defDiamColtelloDefault 60
   #define defPercMaxErrRoDefault 1
   #define defMaxBurstErrRoDefault 3




/* Valore minimo e massimo impulsi/giro encoder. */
#define MIN_IMP_GIRO_ENC   500
#define MAX_IMP_GIRO_ENC  3000

/* Valore minimo e massimo sviluppo ruota [mm]. */
#define MIN_SVIL_RUO      100.
#define MAX_SVIL_RUO     5000.

/* Valore minimo e massimo (in rpm) del numero di giri del mandrino. */
#define MIN_RPM_MANDRINO       10
#define MAX_RPM_MANDRINO     6000

/* Valore minimo e massimo (in rpm) del numero di giri delle frizioni. */
#define MIN_RPM_FRIZIONE        0
#define MAX_RPM_FRIZIONE      500

/* Valore minimo del diametro del filo che esclude il controllo
   del finecorsa filo spezzato nelle lavorazioni con due fili. */
#define MIN_DIAM_TEST_INTRZ   0.30

/* Valore minimo e massimo (in m/min) della velocit� di produzione. */
#define MIN_VEL_PROD  0.1
#define MAX_VEL_PROD 60.0

/* Valore minimo e massimo del numero di pezzi prima della fine del
   lotto raggiunto il quale di deve far lampeggiare la lampada
   di segnalazione. */
#define MIN_NPEZZI_AVVISO       0
#define MAX_NPEZZI_AVVISO      99

/* Valore minimo e massimo del numero di secondi per cui la lampada
   rimane accesa alla fine del lotto se funzionamento automatico. */
#define MIN_LAMP_SPEGN            0
#define MAX_LAMP_SPEGN           99

/* Valore minimo e massimo del numero di spire di pretaglio. */
#define MIN_NSPIRE_PRET           0
#define MAX_NSPIRE_PRET           99

/* Valore minimo e massimo del numero di fornitore. */
#define MIN_FORNI             0
#define MAX_FORNI             9

/* Valore minimo e massimo della tolleranza delle resistenze dai fornitori. */
#define MAX_VAR_RES          80.
#define MIN_VAR_RES           1.
/* Quando calcolo la portata da usare, per evitare di effettuare cambi
   portata maggioro la resistenza da misurare del valore
   MIN_VAR_RES_CALC_PORTATA
*/
#define MIN_VAR_RES_CALC_PORTATA  5.

#define MIN_ASSORB_RUOTA     -2.0
#define MAX_ASSORB_RUOTA      2.0

/* Valore minimo e massimo (in mm) del diametro del filo. */
#define MIN_DIAM_FILO        0.02
#define MAX_DIAM_FILO        5.0

/* Valore minimo e massimo (in mm) del diametro del mandrino. */
#define MIN_DIAM_MD          0.2
#define MAX_DIAM_MD          20

/* Numero minimo e massimo di fili che possono comporre la spira. */
#define MIN_NUM_FILI         1
#define MAX_NUM_FILI         4

// numero max di pezzi che possono essere inseriti in lotto
#define MIN_QUANTITA	     0
#define MAX_QUANTITA         99999

#define MIN_QUANTITA_PROVE   0
#define MAX_QUANTITA_PROVE   999

/* Valore minimo e massimo (in ohm) delle resistenze producibili. */
#define MIN_RES_CAN_MAKE    0.1
#define MAX_RES_CAN_MAKE 20000.


/* Valore minimo e massimo (in ohm/m) della resistenza specifica del filo. */
#define MIN_RESSPEC_FILO    0.01
#define MAX_RESSPEC_FILO  999.99

#define MIN_RESSPEC_FILO_OHM_FEET   0.01
#define MAX_RESSPEC_FILO_OHM_FEET  99.999


/* Valore minimo e massimo (in mm) del diametro delle frizioni. */
#define MIN_DIAM_FRIZIONE      0.0
#define MAX_DIAM_FRIZIONE    200.0


/* Valore minimo e massimo (in mm) del diametro del coltello. */
#define MIN_DIAM_COLTELLO    45.0
#define MAX_DIAM_COLTELLO    85.0


/* Valore minimo e massimo (in mm) dell' interasse coltello-mandrino. */
#define MIN_IASSE    1.00
#define MAX_IASSE   60.00

/* Valore minimo e massimo (in %) del delta % della velocit� delle frizioni
   rispetto al valore nominale.
   Posso andare dal 20% del valore nominale al 180% del valore nominale.
*/

#define MIN_DELTA_RPM_FRIZIONE  20
#define MAX_DELTA_RPM_FRIZIONE  180




/* Valore minimo e massimo del numero di frizioni. */
#define MIN_NUM_FRIZIONI     1
#define MAX_NUM_FRIZIONI     2

/* Valore minimo e massimo del delta sulla velocit� delle frizioni
   (unit� di misura=unit� di fondo scala adc). */
#define MAX_DELTA_VRUOTE   FS_ADC
#define MIN_DELTA_VRUOTE   0
/* Numero massimo di secondi di ritardo prima dell' avvio. */
#define MAX_SEC_AVVIO        50



/* Valore minimo e massimo (in %) dell' escursione dei potenziometri. */
#define MIN_ESC_POTENZIO        0
#define MAX_ESC_POTENZIO      100



/* Valore minimo e massimo del tempo di taglio. */
#define MIN_TIME_TAGLIO         0
#define MAX_TIME_TAGLIO       255

/* Valore minimo e massimo dell' allungamento del filo. */
#define MAX_ALLUNGAMENTO        5.0
#define MIN_ALLUNGAMENTO        1.0

/* Valore minimo e massimo della costante di taratura del mandrino. */
#define MIN_K_RPM_MANDRINO      0.5
#define MAX_K_RPM_MANDRINO      1.5

/* Valore minimo e massimo della costante di taratura della frizione. */
#define MIN_K_RPM_FRIZIONE      0.5
#define MAX_K_RPM_FRIZIONE      1.5

/* Valore minimo e massimo della durata di un allarme valido (ms). */
#define MIN_DURATA_ALL  100
#define MAX_DURATA_ALL  10000

/* Valore minimo e massimo della durata discesa coltello in posizione di distanziazione (ms). */
#define MIN_DURATA_DISCESA_DISTANZIATORE  200
#define DEFAULT_DURATA_DISCESA_DISTANZIATORE  800
#define MAX_DURATA_DISCESA_DISTANZIATORE  10000

// Numero di default di spire di compressione nella distanziazione di una resistenza
#define DEFAULT_N_SPIRE_COMPRESSIONE 1.0

// Valore massimo per ritardo e durata attivazione bandella
#define MAX_RITARDO_BANDELLA 9999
#define MAX_ATTIVAZIONE_BANDELLA 9999

// Valore massimo e minimo per offset coltello/distanziatore [mm]
#define MAX_OFFSET_COLTELLO_DISTANZIATORE_mm 200.0
#define MIN_OFFSET_COLTELLO_DISTANZIATORE_mm 0.0
