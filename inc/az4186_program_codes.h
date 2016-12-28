#ifndef def_az4186_program_codes_h_included
#define def_az4186_program_codes_h_included

#include "az4186_const.h"
#include "az4186_commesse.h"

// handle to product codes file
extern unsigned int ui_handle_product_codes;

// numero dell'ultima versione gestita 
//#define defLastPROGRAMMAversion 1  // base version
#define defLastPROGRAMMAversion 2  // added temp_comp_t1,temp_comp_t2,temp_comp_ct1,temp_comp_ct2

// numero max di percentuali di distanziazione per codice prodotto
#define defMaxNumOfPercDistanzia 20
// type describing the program offset (or number)
typedef unsigned long tipoNumeroProgramma;

/* Definizione della struttura: programma di produzione. */
typedef struct _PROGRAMMA {
	unsigned char ucVersionCharacter; // il carattere 'V'
	unsigned char ucVersionNumber; // il numero di versione 1..255
	/* Nome del programma.
	  Campizzato in:
	   se u.m. =mm
		 x.xxxaaaay.yyz.zzbkk
		x.xxx diametro filo in mm
		aaaa  tipo materiale
			y.yy  diametro mandrino mm
		z.zz  diametro spirale mm
		b     numero di fili
		kk    fornitore
	   se u.m. =inch
	 .xxxxaaaa.yyy.zzzbkk
		.xxxx diametro filo in inch
		aaaa  tipo materiale
		.yyy  diametro mandrino inch
		.zzz  diametro spirale inch
		b     numero di fili
		kk    fornitore
	*/
	char codicePrg[defMaxCharNomeCommessa+1];
	tipoNumeroProgramma nextPrg;      /* Numero del programma con codice
				  immediatamente superiore.
				  Se vale zero, � l' ultimo programma.
				   */
	tipoNumeroProgramma prevPrg;      /* Numero del programma precedente. */
	float vel_produzione;       /* Velocit� di produzione in m/min. */
	unsigned int rpm_mandrino;  /* Giri/minuto mandrino. */
	float assorb[NUM_FRIZIONI]; /* Assorbimento delle frizioni in amp�re. */
	float pos_pretaglio; /* Posizione di pretaglio. */
	float diametro_filo; /* Diametro del filo/i in mm. */
	float diametro_mandrino;    /* Diametro del mandrino in mm. */
	unsigned char num_fili;    /* Numero di fili: min NUM_MIN_FILI max MAX_NUM_FILI. */
	float coeff_corr;

	/* Delta di velocit� delle ruote.
	velocit�_ruote=v0_ruota*(delta), 0%<=delta<=200%
	delta viene impostato dall' utente via potenziometro. (default 100%)
	*/
	unsigned int vruote_prg[NUM_FRIZIONI];
	/*
	Velocit� (in rpm) delle frizioni: il suo valore � dato da:
	v_frizione=v0_frizione+ delta_v
	- v0_frizione= vel_angolare_motore*(raggio_mandrino+raggio_filo)*spinta
	  v0_frizione � in questo modo uguale (a meno del fattore di spinta)
	  alla velocit� periferica del sistema mandrino+filo
	  spinta � un fattore (definito in spicost.h) leggermente maggiore di 1
	  in modo da assicurare che la frizione si mantenga in spinta
	  rispetto al filo.
	- delta_v � una variazione che l' utente pu� imporre tramite
	  potenziometro in modo indipendente da v0_frizione.
	*/
	float vel_frizioni[NUM_FRIZIONI];
	unsigned char fornitore; /* Codice del fornitore. */
	// intero a 16 bit che contiene chiave che indica che prg � valido...
	unsigned short usValidProgram_A536;
	unsigned char empty;        /* Programma vuoto. */
	float vel_prod_vera; /* valore "vero" della velocita' di produzione. */
	unsigned short int usDelayDistanziaDownMs;	// tempo necessario al coltello per scendere in posizione di distanziazione
	unsigned short int usPercDistanzia_Starts[defMaxNumOfPercDistanzia];	// percentuale di valore ohmico a cui inizia  l' attacco del distanziatore 0%=0x0000..100%=0xFFFF
	unsigned short int usPercDistanzia_Ends[defMaxNumOfPercDistanzia];	// percentuale di valore ohmico a cui finisce l' attacco del distanziatore 0%=0x0000..100%=0xFFFF
	float f_coltello_pos_spaziatura_mm; // coltello: Posizione di distanziazione. 
	float f_coltello_offset_n_spire; // coltello: numero di spire di cui si comprime la resistenza prima di iniziare a distanziarsi
	// numero di spire di distanziazione
	unsigned short int usPercDistanzia_NumSpire[defMaxNumOfPercDistanzia];	
	// questo array, contiene l'info che qualifica i valori di distanziazione inseriti dall'utente
	// 1-->utente ha specificato percentuale iniziale e numero di spire (la percentuale finale viene calcolata)
	// 2-->utente ha specificato numero di spire e percentuale finale (la percentuale iniziale viene calcolata)
	// 0, altri (valore di default) -->percentuale iniziale e finale (il numero di spire viene calcolato)
	unsigned char ucPercDistanzia_Type[defMaxNumOfPercDistanzia];
	// temperature factor [*10-6/�C], eg 0.0041 1/C-->4'100, default is 0
	unsigned int ui_temperature_coefficient_per_million;   
	// additive adjustment factor; the resistance is corrected by a factor +/ x %
	// eg this field is +2-->it means that the resistance value is corrected by a +2% 
	// eg this field is -3-->it means that the resistance value is corrected by a -3% 
	int i_temperature_corrective_factor_percentual;   
	// temperature [celsius] at which the temperature coefficient is set (eg 100-->100�C)
	// 0--> default value
	float f_temperature_compensation_t_celsius[2];
	// factor of change of resistance at temperature set, eg 1.33
	// 1.0--> no variation in resistance (default), 0-->invalid
	float f_temperature_compensation_coeff[2];
} PROGRAMMA;


// structure for programs (i.e. product codes) handling
typedef struct _TipoStructHandlePrg{
	tipoNumeroProgramma actPrg;
	tipoNumeroProgramma runningPrg;
	tipoNumeroProgramma firstPrg;
	PROGRAMMA theRunning;
	PROGRAMMA *ptheAct;
	PROGRAMMA theAct;
}TipoStructHandlePrg;


// funzione di ricerca nei codici programma di un codice che pu� contenere caratteri jolly
// ad esempio 1.240 ???? 2.20 ? ?
// In ingresso va passato il numero max N_max_corr di corrispondenze da cercare
// restituisce il numero di corrispondenze trovate
// 0 --> nessuna corrispondenza
// 1 --> una corrispondenza
// 2 --> due corrispondenze
// ...
// N_max_corr --> almeno N_max_corr corrispondenze trovate

// numero max di caratteri nella stringa di ricerca codice programma
#define defMaxLengthStringSearchCodiceProgramma 21
// numero max di stringhe codice programma che la procedura pu� restituire
#define defMaxStringCodiceProgrammaCanFind 4
// caratteri jolly nella ricerca di codice programma
#define defJollyCharSearchCodiceProgramma ' '
// direzione di ricerca del codice programma
typedef enum {  enumDirectionForward_SCP=0,
				enumDirectionBackward_SCP,
				enumDirection_SCP_NumOf
			 }  enumDirection_SearchCodiceProgramma;

typedef struct _TipoStructSearchCodiceProgramma{
	// stringa da ricercare
	unsigned char ucString[defMaxLengthStringSearchCodiceProgramma];
	// stringhe trovate e restituite
	unsigned char ucFoundString[defMaxStringCodiceProgrammaCanFind][defMaxLengthStringSearchCodiceProgramma];
	// numero di record dei codici programma
	unsigned long uiNumRecordFoundString[defMaxStringCodiceProgrammaCanFind];
	// record iniziale da cui cercare: 0= primo
	unsigned long uiStartRecord;
	// record corrente in cui creco
	unsigned long uiCurRecord;
	// numero dell'ultimo record trovato
	unsigned long uiLastRecordFound;
	// numero di record trovati: 0= nessuno
	unsigned int uiNumRecordFound;
	// numero max di record da trovare
	unsigned int uiNumMaxRecord2Find;
	// direzione di ricerca
	enumDirection_SearchCodiceProgramma direction;
	// risultato del confronto tra stringa di ricerca e codice programma:
	// 0=le stringhe sono uguali
	// 1=la prima � maggiore della seconda
	// 2=la seconda � maggiore della prima (se la prima � la stringa di ricerca, la ricerca � finita)
	unsigned char ucCmpResult;
	// impostare ad 1 se si vuole il codice esatto, altrimenti la ricerca viene fatta sfruttando i caratteri jolly
	unsigned char ucFindExactCode;
}TipoStructSearchCodiceProgramma;
extern TipoStructSearchCodiceProgramma scp;


// retcodes from test product codes
typedef enum{
    enum_test_product_codes_retcode_ok=0,
    enum_test_product_codes_retcode_err,
    enum_test_product_codes_retcode_numof
}enum_test_product_codes_retcode;

// test product codes insertion etc
enum_test_product_codes_retcode test_product_codes(void);

// aggiunge un prodotto codice di default
void v_add_default_product_code(void);
// empty the product codes list... just creates a list with only the default product code inside
void v_empty_product_codes_list(void);
unsigned int ui_register_product_codes(void);


#endif //#ifndef def_az4186_program_codes_h_included
