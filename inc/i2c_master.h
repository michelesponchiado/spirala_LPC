#define I2C_ADDR 0xD0

#define buf_len 16
#define defI2cbusFrequencyKhz 400


/* I tre bit più significativi dell' indirizzo di un DAC 519 della Maxim
   sono sempre 010; questa maschera serve per impostarli come desiderato. */
#define FactoryMaskBit519 0x40
/* Maschera che permette di determinare la configurazione dei bit del byte
   di comando del 519. */
#define CommandMaskBit519 0x00
/* Bit di power down mode: settato a 1 porta Dac in stato a basso consumo. */
#define BitPowerDownMode  0x08
/* Uscita 0 del DAC. */
#define DacOut0           0x00
/* Uscita 1 del DAC. */
#define DacOut1           0x01

extern volatile unsigned char ucSemaphore_I2Cbus;

// routine che aspetta i2c bus
// da chiamare solo nel background loop
// e non nelle routine di interrupt
// altrimenti potrebbe dilatare eccessivamente i tempi!!!
// parametro in ingresso: timeout [ms]
// se timeout vale 0, la routine deve attendere per sempre, finché i2c bus non si libera
// se timeout vale 1, la routine deve verificare se i2cbus è libero, altrimenti uscire subito
// altrimenti la routine attende almeno il numero di ms specificato che i2cbus si liberi
// ritorna 0 se i2cbus non si è liberato entro timeout
// ritorna 1 se i2cbus libero
unsigned char ucWaitI2Cbus(unsigned int ui_timeout_ms);

void vSignalI2Cbus(void);



// routine di attesa che i2c sia libero ed abbia terminato la tx...
extern unsigned char ucWait_i2c_tx_operation(void);


extern unsigned char ucLastCommandMandrino;
extern unsigned char ucLastCommandTaglio;


/* Routine di comunicazione col DAC.
   Ingresso:
     - addrMax519: indirizzo del DAC;
     - out0: valore di out0 (prima   uscita del DAC)
     - out1: valore di out1 (seconda uscita del DAC)
   Uscita: nulla.
   La routine spedisce al DAC di indirizzo specificato i due byte che
   definiscono i valori delle sue due uscite analogiche.
*/
void SendByteMax519(unsigned char addrMax519,
                    unsigned char out0,
                    unsigned char out1);

/* Routine di comunicazione col DAC, da usare in interrupt!!!!!
   Ingresso:
     - addrMax519: indirizzo del DAC;
     - out0: valore di out0 (prima   uscita del DAC)
     - out1: valore di out1 (seconda uscita del DAC)
   Uscita: nulla.
   La routine spedisce al DAC di indirizzo specificato i due byte che
   definiscono i valori delle sue due uscite analogiche.
*/
unsigned char ucSendCommandPosTaglioMax519_Interrupt(unsigned char out1);
unsigned char ucSendCommandPosTaglioMax519_Main(unsigned char out1);
void vInitI2c(void);
