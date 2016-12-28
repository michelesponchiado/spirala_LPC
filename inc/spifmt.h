/*
   READ THIS FIRST, PLEASE!

   Questo file contiene tutti i formati di stampa su LCD dei parametri
   di controllo della spiralizzatrice, in modo che possano essere
   agevolmente modificati.
   Essi sono costituiti sempre da coppie stringa-formato+lunghezza campo edit
   La prima define stabilisce il formato di print su LCD della grandezza
   definita dal nome della stringa di formato.
   La seconda fornisce la lunghezza del campo su cui si pu• editare il
   parametro, i.e. il numero di caratteri modificabili/visualizzabili.
   ATTENZIONE!!!
   Se si modifica l' uno o l' altro, correggere l' altro/l' uno.
*/

#define LEN_EDIT_TEST_INTRZ 6
#define FMT_TEST_INTRZ      "%-5.3f"

#define FMT_IMP_GIRO_ENCODER  "%-4d"
#define LEN_EDIT_IMP_GIRO_ENC 5

#define FMT_SVIL_RUO       "%-7.3f"
#define LEN_EDIT_SVIL_RUO  8

#define LEN_EDIT_COD_FOR 2
#define FMT_COD_FOR "%-1.1d  "

#define FMT_FORNITORE       "%-1.1d  "
#define LEN_EDIT_FORNITORE   2

/* Parametro: giri/minuto del mandrino al 100%
   (menu parametri, submenu SBM_EP_RPM_MAND)  */
#define FMT_RPM_MANDRINO	"%-5u"
#define LEN_EDIT_RPM_MANDRINO 	5

/* Parametro: giri/minuto delle frizioni al 100%
   (menu parametri, submenu SBM_EP_RPM_FRIZ)  */
#define FMT_RPM_FRIZIONI 	"%-5u"
#define LEN_EDIT_RPM_FRIZ 	5

/* Parametro: numero di pezzi prima della fine del lotto
   raggiunto il quale si attiva la lampada di segnalazione fine lotto.
   (menu parametri, submenu SBM_EP_PEZZI_AVVISO)  */
#define FMT_NPEZZI_AVVISO	"%-2d"
#define LEN_EDIT_NPEZZI_AVVISO	2

/* Parametro: numero di secondi durante i quali la lampada deve
   restare accesa dopo la fine del lotto se il funzionamento Š automatico.
   (menu parametri, submenu SBM_EP_LAMP_SPEGN)  */
#define FMT_LAMP_SPEGN        "%-2d"
#define LEN_EDIT_LAMP_SPEGN   2

/* Parametro: numero di spire di pretaglio
   (menu parametri, submenu SBM_EP_NSPIRE_PRETAG)  */
#define FMT_NSPIRE_PRET        "%-2d"
#define LEN_EDIT_NSPIRE_PRET   2


#define FMT_MAXV            "%-4.1f"
#define LEN_EDIT_MAXV        4

#define COL_TIT_FORN         0
#define COL_FORN             5
#define COL_TIT_MAXV	     8
#define COL_MAXV	    14

#define FMT_VEL_PROD        "%-4.1f "
#define LEN_EDIT_VEL_PROD    4

#define FMT_RPM_VP          "%-4d "
#define LEN_EDIT_RPM_VP      4

#define COL_TIT_MM_VP        5
#define COL_MM_VP            0
#define COL_TIT_RPM_VP      17
#define COL_RPM_VP          12

#define COL_TIT_FEET_VP      6
#define COL_FEETM_VP         0
#define FMT_VEL_PROD_FEET      "%-5.1f "
#define LEN_EDIT_VEL_PROD_FEET    5


/* Formato del codice programma. */
#define FMT_CODICE_PRG      "%19s"
#define LEN_EDIT_CODICE_PRG   19

/* Formato del codice programma:
         x.xxx aaaa y.yy b kk
            x.xxx diametro filo in mm
            aaaa  tipo materiale
            y.yy  diametro mandrino mm
            b     numero di fili
            k    fornitore
       se u.m. =inch
         .xxxx aaaa .yyy b k
*/
/* Picture del codice programma se unit… di misura=mm. */
#define PICT_COD_PRG_MM     "n.nnn aaaa n.nn n n"
#define FMT_COD_EMPTY_MM    "0.000      0.00 1 0"
/* Indicazione dei campi da stampare in reverse se unit… di misura=mm. */
#define REVE_COD_PRG_MM     "1111100000011110001"
/* Picture del codice programma se unit… di misura=inch. */
#define PICT_COD_PRG_INCH   ".nnnn aaaa .nnn n n"
#define FMT_COD_EMPTY_INCH  ".0000      .000 1 0"
/* Indicazione dei campi da stampare in reverse se unit… di misura=inch. */
#define REVE_COD_PRG_INCH   "1111100000011110001"


#define COL_FPRG_DIAM_FILO_MM    0
#define COL_FPRG_DIAM_FILO_INCH  0
#define LEN_FPRG_DIAM_FILO_MM    4
#define LEN_FPRG_DIAM_FILO_INCH  4

#define COL_FPRG_DIAM_MAND_MM    11
#define COL_FPRG_DIAM_MAND_INCH  11
#define LEN_FPRG_DIAM_MAND_MM    4
#define LEN_FPRG_DIAM_MAND_INCH  4

#define COL_FPRG_NFILI_MM    	 16
#define COL_FPRG_NFILI_INCH  	 16
#define LEN_FPRG_NFILI_MM        1
#define LEN_FPRG_NFILI_INCH  	 1

#define COL_FPRG_FORNI_MM    	 18
#define COL_FPRG_FORNI_INCH  	 18
#define LEN_FPRG_FORNI_MM        2
#define LEN_FPRG_FORNI_INCH  	 2

#define COL_FPRG_TFILO_MM    	 6
#define COL_FPRG_TFILO_INCH  	 6
#define LEN_FPRG_TFILO_MM        4
#define LEN_FPRG_TFILO_INCH  	 4


#define FMT_ASSORB_RUOTA         "%+5.2f "
#define LEN_EDIT_ASSORB_RUOTA    5
#define COL_AMPERE_ASSORB_PRG    6

#define FMT_QUOTA_PRET_MM        "%+5.2f "
#define COL_MM_PRET              6
#define FMT_QUOTA_PRET_INCH      "%+5.3f "
#define COL_INCH_PRET            6

#define FMT_DIAM_FILO_MM         "%-5.3f "
#define LEN_EDIT_DIAM_FILO_MM    5
#define COL_TIT_MM_DF            6

#define FMT_DIAM_FILO_INCH       "%-5.4f "
#define LEN_EDIT_DIAM_FILO_INCH  5
#define COL_TIT_INCH_DF          6

#define FMT_TIPO_FILO            "%-4.4s"
#define LEN_EDIT_TIPO_FILO       4

#define FMT_DIAM_MAND_MM         "%-5.3f "
#define LEN_EDIT_DIAM_MAND_MM    5
#define COL_TIT_MM_MD            6
#define FMT_DIAM_MAND_INCH       "%-4.4f "
#define LEN_EDIT_DIAM_MAND_INCH  4
#define COL_TIT_INCH_MD          5

#define FMT_DIAM_SPIR_MM         "%-6.3f "
#define LEN_EDIT_DIAM_SPIR_MM    6
#define COL_TIT_MM_SP            7
#define FMT_DIAM_SPIR_INCH       "%-4.4f "
#define LEN_EDIT_DIAM_SPIR_INCH  4
#define COL_TIT_INCH_SP          5

#define FMT_NUM_FILI             "%1d      "
#define COL_TIT_NUM_FILI         6
#define LEN_EDIT_NUMFILI         1

#define FMT_COEFF_CORR           "%-6.3f    "
#define COL_TIT_OHM_CC		 7
#define LEN_EDIT_COEFFCORR       6

#define FMT_DELTAR               "%-6.2f    "
#define LEN_EDIT_DELTAR          6

#define FMT_CODICE_COMMESSA      "%-20s"
#define LEN_EDIT_CODICE_COMM     20


#define COL_NUM_COMM             14
#define FMT_NUM_COM              "%4d"
#define COL_PUNTO_COMM           18
#define FMT_PUNTO_COM            "%1.1d"
#define LEN_EDIT_COD_COMM        6


#define FMT_QUANTITA             "%-4d    "
#define LEN_EDIT_QUANTITA        4
#define COL_PEZZI_FATTI          5
#define FMT_PEZZI_FATTI          "(%-4d)"

#define FMT_QUANTITA_PROVE       "%-3d"
#define LEN_EDIT_QUANTITA_PROVE  3

#define FMT_RES                  "%-6.2f"
#define LEN_EDIT_RES		 6

#define FMT_RESSPEC_MM           "%-6.3f"
#define LEN_EDIT_RESSPEC_MM	 6
#define FMT_RESSPEC_FEET         "%-7.3f"
#define LEN_EDIT_RESSPEC_FEET	 7


#define FMT_DIAM_RC_MM		 "%-5.2f"
#define LEN_EDIT_DIAM_RC_MM      5
#define FMT_DIAM_RC_INCH         "%-5.3f"
#define LEN_EDIT_DIAM_RC_INCH    5

#define FMT_DIAM_CO_MM		 "%-5.2f"
#define LEN_EDIT_DIAM_CO_MM      5
#define FMT_DIAM_CO_INCH         "%-5.3f"
#define LEN_EDIT_DIAM_CO_INCH    5

#define COL_ASS_MEM              2
#define FMT_ASS_COMM             "%-5.2f "
#define COL_TIT_ASS_MEM          0
#define COL_TIT_AMP_MEM          8
#define COL_ASS_RIL              13
#define FMT_ASS_RIL              "%-5.2f "
#define COL_TIT_ASS_RIL          11
#define COL_TIT_AMP_RIL          19


#define COL_COM_RPM              0
#define FMT_COM_RPM              "%4u "
#define COL_TIT_COM_RPM          4
#define COL_COM_MEM              8
#define FMT_COM_MEM              "%+5.2f "
#define COL_COM_ASS              13
#define FMT_COM_ASS              "%+5.2f "
#define COL_TIT_COM_AMP          18
#define COL_COM_OHM              0
#define FMT_COM_OHM              "%-5.2f"
#define COL_TIT_COM_OHM          5

#define COL_TIT_VIS_COM          0
#define COL_VIS_COM              4
#define FMT_VIS_COM              "%4d.%1.1d  "
#define COL_TIT_VIS_PDF          11
#define COL_VIS_DF               16
#define FMT_VIS_DF               "%-4d"
#define COL_TIT_VIS_OHM          0
#define COL_VIS_OHM              4
#define FMT_VIS_OHM              "%-6.2f   "
#define COL_TIT_VIS_PFA          11
#define COL_VIS_FA               16
#define FMT_VIS_FA               "%-4d"

#define COL_TIT_TAR_P            18
#define COL_TIT_TAR_COSA          0
#define COL_TAR_K                15
#define FMT_TAR_K                "%5.3f"
#define COL_TAR_O                13
#define FMT_TAR_O                "%7.3f"
#define COL_TAR_RC               13
#define FMT_TAR_RC               "%7.3f"
#define LEN_EDIT_TARA_RES	  7

#define LEN_EDIT_PASSW           5

#define COL_TIT_UT_BAUD		 0
#define COL_UT_BAUD              5
#define COL_TIT_UT_PARITA       10
#define COL_UT_PARITA           17


#define COL_TIT_UT_GIORNO        2
#define COL_UT_GIORNO            0
#define FMT_UT_GIORNO            "%2.2d "
#define COL_TIT_UT_MESE          6
#define COL_UT_MESE              4
#define FMT_UT_MESE              "%2.2d "
#define COL_TIT_UT_ANNO          10
#define COL_UT_ANNO              8
#define FMT_UT_ANNO              "%2.2d "
#define COL_TIT_UT_ORA           14
#define COL_UT_ORA               12
#define FMT_UT_ORA               "%2.2d "
#define COL_TIT_UT_MINUTO        18
#define COL_UT_MINUTO            16
#define FMT_UT_MINUTO            "%2.2d "


#define COL_TIT_UT_INDIG	 0
#define COL_UT_INDIG             4
#define FMT_UT_INDIG             "%16s"

#define COL_TIT_UT_OUTDIG	 0
#define COL_UT_OUTDIG             4
#define FMT_UT_OUTDIG             "%-16s"

#define COL_TIT_UT_INANA	 0
#define COL_UT_INANA		 4
#define LEN_FIELD_INANA          4
#define FMT_UT_INANA             "%-4.4li"

#define COL_TIT_UT_OUTANA	 0
#define COL_UT_OUTANA		 4
#define LEN_FIELD_OUTANA          4
#define FMT_UT_OUTANA             "%-4.4u"

#define PICT_IN_ANA		"nnnnnnnnnnnnnnnn"
#define REVE_IN_ANA             "0000111100001111"

#define PICT_OUT_ANA		"nnnnnnnnnnnnnnnn"
#define REVE_OUT_ANA             "0000111100001111"

#define FMT_IASSE_INCH  "%-7.4f"
#define FMT_IASSE_MM    "%-7.4f"
#define LEN_EDIT_IASSE  6

#define FMT_DELTA_RPM_FRIZIONI      "%3d"
#define LEN_EDIT_DRPM_FRIZ 	     3


#define LEN_EDIT_SINO_CANC  4
#define COL_SINO_CANC	   17

#define COL_UT_ALR	    4

#define FMT_NSEC_AVVIO      "%-4d"

#define FMT_DURATA_ALL      "%-5d"
#define LEN_EDIT_DURATA_ALL    5


#define FMT_BURSTERR             "%-3d      "
#define LEN_EDIT_MAXBURST        3

#define FMT_PERCRO               "%-3d      "
#define LEN_EDIT_ER_PERCRO       3

#define LEN_EDIT_EP_FS_ASSORB    7
#define FMT_FS_ASSORB    	"%-5.3f"
