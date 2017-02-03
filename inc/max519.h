/* File header del modulo di comunicazione col DAC MAXIM 519. */


/* Fondo scala del DAC MAXIM 519. */
#define FS_DAC_MAX519  0xFF
// num of shift to get full scale of MAX519
// e.g. executing def_num_bit_shift_FS_DAC_MAX519 right shift you divide by FS_DAC_MAX519
#define def_num_bit_shift_FS_DAC_MAX519  8

/* Indirizzi dei Dac di controllo del mandrino, della velocit� e della
   coppia delle frizioni, delle posizioni del coltello. */

// max519 numero 0 out0=velocit� Mandrino  out1=coppia mandrino 
#define addrDacMandrino 	0
// max519 numero 1 out0=velocit� ruota 1  out1=velocit� ruota 2 
#define addrDacFrizione1  	1
// nel dac 1, frizione 1 esce dall'uscita 0
#define addrOutInDacFrizione1 	0
#define addrDacFrizione2 	1
// nel dac 1, frizione 2 esce dall'uscita 1
#define addrOutInDacFrizione2 	1
// max519 numero 2 non usato
// max519 numero 3  : out0= posizione di riposo Coltello, out1=posizione di pretaglio Coltello. 
#define addrDacColtello		3

/* Routine di comunicazione col DAC.
   Ingresso:
     - addrMax519: indirizzo del DAC;
     - out0: valore di out0 (prima   uscita del DAC)
     - out1: valore di out1 (seconda uscita del DAC)
   Uscita: nulla.
   La routine spedisce al DAC di indirizzo specificato i due byte che
   definiscono i valori delle sue due uscite analogiche.
*/
extern void SendByteMax519(unsigned char addrMax519,
                    	   unsigned char out0,
                    	   unsigned char out1);


// Routine di spedizione valore al dac di comando posizione taglio/pretaglio, da usare in interrupt!!!!!
extern unsigned char ucSendCommandPosTaglioMax519_Interrupt(unsigned char out1);
extern unsigned char ucSendCommandPosTaglioMax519_Main(unsigned char out1);


/* Routine che porta un Dac in power down mode.
   Ingresso: l' indirizzo del Dac da portare in power down mode.
   Uscita: nulla.
   Quando un Dac � in pwer down, i suoi latch conservano il valore
   da portare in uscita, ma l' uscita � flottante.
*/
void PowerDownDac(unsigned char addrMax519);
