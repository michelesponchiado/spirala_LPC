#ifndef def_az4186_commesse_h_included
#define def_az4186_commesse_h_included

#include "az4186_const.h"

// numero max di commesse gestibili
#define NUM_COMMESSE  5
// numero max di caratteri del nome commessa
#define defMaxCharNomeCommessa 20

// Definizione della struttura: commessa. 
typedef struct _COMMESSA{
  char commessa[defMaxCharNomeCommessa+4]; 	// Nome della commessa. 
  unsigned long numcommessa;  				// Numero progressivo della commessa. 
  unsigned long ucWorkingMode_0L_1LR;      	// modo con cui deve essere eseguita la commessa: L o L+R???
  unsigned long int numprogramma; 			// Numero programma cui fa riferimento. 
  unsigned long n;           				// pezzi da produrre. 
  unsigned long ndo;         				// pezzi prodotti. 
  float res;                  				// Valore di resistenza da produrre. 
  float resspec_spir;         				// Resistenza specifica della spira. 
  float assorb_frizioni[NUM_FRIZIONI];  	// Corrente assorbita dalle ruote. 
// Aggiustamento della velocità delle ruote. Il valore centrale corrisponde
// al valore nominale; il potenziometro a zero indica velocità zero, il
// potenziometro al massimo indica valore doppio rispetto a quello nominale. 
  unsigned long vruote_comm[NUM_FRIZIONI];
  unsigned long rpm;                   		// Rpm correnti del mandrino. 
  unsigned long automatico; 				// Passaggio automatico alla commessa successiva. 
  unsigned long magg;        				// Magg. pezzi da produrre. 
  unsigned long defPortata; 				// Numero della portata da usare per il filo
											// del programma.
											// NUM_PORTATE-1  e' la piu' bassa.
											// 0 -  5
											// 5 - 50
											// 50-500
											// 500-5000
  unsigned long nextComm,prevComm; // Numero della prossima (+1) e della precedente (+1) commessa.
  unsigned long uiValidComm_0xa371; // codice che identifica una commessa valida
} COMMESSA; //typedef struct _COMMESSA


#endif //#ifndef def_az4186_commesse_h_included
