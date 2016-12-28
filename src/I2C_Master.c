/*------------------------------------------------------------------------------
i2c_master.c

Version:
November 30, 2004 Version 2.0 - Updated for DK3300 ELCD
April 2004 Version 0.2 - Initial Version

Dependencies:
upsd3300_lcd    - LCD driver.
upsd3300_i2c    - I2C driver.
upsd3300_timer  - Timer driver.


Description:  Demo for I2C Master
The main function of this code is to demonstrate the use of the I2C driver in
Master mode.  As a master, the uPSD device writes data to a slave device over
the I2C bus and reads it back.  It compares the received data with the sent data
and confirms with a message on the LCD if there is a match.  

It is intended that the I2C slave device this master communicates with is a uPSD
device programmed with the I2C Slave demo code.  The SCL and SDA lines of the
master and slave devices must be connected together and a pull-up is on each 
signal.  The two devices must also have their GNDs tied together.  Using two 
DK3300 boards, no pull-ups are required but the two signals (SCL, SDA) plus GND
must be connected.


Copyright (c) 2004 STMicroelectrons Inc.

This example demo code is provided as is and has no warranty,
implied or otherwise.  You are free to use/modify any of the provided
code at your own risk in your applications with the expressed limitation
of liability (see below) so long as your product using the code contains
at least one uPSD product (device).

LIMITATION OF LIABILITY:   NEITHER STMicroelectronics NOR ITS VENDORS OR 
AGENTS SHALL BE LIABLE FOR ANY LOSS OF PROFITS, LOSS OF USE, LOSS OF DATA,
INTERRUPTION OF BUSINESS, NOR FOR INDIRECT, SPECIAL, INCIDENTAL OR
CONSEQUENTIAL DAMAGES OF ANY KIND WHETHER UNDER THIS AGREEMENT OR
OTHERWISE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
------------------------------------------------------------------------------*/

#include "LPC23xx.h"                        /* LPC23xx/24xx definitions */
#include "type.h"
#include "i2c.h"
#include "i2c_master.h"
#include "spicost.h"
#include "fpga_nxp.h"

unsigned char k, i2c_return_code;
unsigned char ucLastCommandMandrino;
unsigned char ucLastCommandTaglio;



#ifdef defSimulaI2c
#warning *** i2c_master i2c simulation ***
    // routine che aspetta i2c bus
    // da chiamare solo nel background loop
    // e non nelle routine di interrupt
    // altrimenti potrebbe dilatare eccessivamente i tempi!!!
    unsigned char ucWaitI2Cbus(unsigned int ui_timeout_ms){
        return 1;
    }//unsigned char ucWaitI2Cbus(void)


    void vSignalI2Cbus(void){
    }//void vSignalI2Cbus(void)



    //
    //    Routine di comunicazione col DAC.
    //    Ingresso:
    //     - addrMax519: indirizzo del DAC;
    //     - out0: valore di out0 (prima   uscita del DAC)
    //     - out1: valore di out1 (seconda uscita del DAC)
    //    Uscita: nulla.
    //    La routine spedisce al DAC di indirizzo specificato i due byte che
    //    definiscono i valori delle sue due uscite analogiche.
    //
    void SendByteMax519(unsigned char addrMax519,
                        unsigned char out0,
                        unsigned char out1){
    }//void SendByteMax519(unsigned char addrMax519,...


    /* Routine di comunicazione col DAC, da usare in interrupt!!!!!
       Ingresso:
         - addrMax519: indirizzo del DAC;
         - out0: valore di out0 (prima   uscita del DAC)
         - out1: valore di out1 (seconda uscita del DAC)
       Uscita: nulla.
       La routine spedisce al DAC di indirizzo specificato i due byte che
       definiscono i valori delle sue due uscite analogiche.
    */
    unsigned char ucSendCommandPosTaglioMax519_Interrupt(unsigned char out1){

        return 1;
    }

    // spedizione comando di taglio max519
    unsigned char ucSendCommandPosTaglioMax519_Main(unsigned char out1){
        return 1;
    }



    // Inizializzazione i2c bus
    void vInitI2c(void){	
    }//void vInitI2c(void)


#else
    // it counts the number of i2c bus access failures
    static unsigned char uc_cnt_i2c_not_free;
    // max 16 i2c bus access fail before i2c bus reset
    #define def_max_cnt_i2c_not_free 16

    // routine che aspetta i2c bus
    // da chiamare solo nel background loop
    // e non nelle routine di interrupt
    // altrimenti potrebbe dilatare eccessivamente i tempi!!!
    // parametro in ingresso: timeout [ms]
    // se timeout vale 0, la routine deve attendere per sempre, finch� i2c bus non si libera
    // se timeout vale 1, la routine deve verificare se i2cbus � libero, altrimenti uscire subito
    // altrimenti la routine attende almeno il numero di ms specificato che i2cbus si liberi
    // ritorna 0 se i2cbus non si � liberato entro timeout
    // ritorna 1 se i2cbus libero
    unsigned char ucWaitI2Cbus(unsigned int ui_timeout_ms){
        unsigned char uc_retcode;
		extern unsigned char uc_requesting_i2c_bus;
		uc_requesting_i2c_bus=1;
    // init, retcode ok
        uc_retcode=1;
    // endless wait for i2c bus?
        if (ui_timeout_ms==0){
            while(!I2CEngine_isEnded());
        }
    // just test if i2cbus is free?
        else if (ui_timeout_ms==1){
            if (!I2CEngine_isEnded())
                uc_retcode=0;
        }
    // else wait until i2c bus free or timeout
        else{
            TipoStruct_timeout timeout_wait_i2c;
            unsigned int ui_timeout_wait;
            timeout_init(&timeout_wait_i2c);
            ui_timeout_wait=0;
            while(1){
                if (I2CEngine_isEnded())
                    break;
                // is the timeout routine enabled???
                if ((VICIntEnable&0x10)==0){
                    v_wait_microseconds(100);
                    if (++ui_timeout_wait*100>ui_timeout_ms*1000){
                        uc_retcode=0;
                        break;
                    }
                }
                else if (uc_timeout_active(&timeout_wait_i2c, ui_timeout_ms)){
                    if (!I2CEngine_isEnded())
                        uc_retcode=0;
                    break;
                }
            }
        }
        // i2c bus free ok?
        if (uc_retcode){
            uc_cnt_i2c_not_free=0;
        }
        // if too many timeouts, signal error!
        else{
            if (++uc_cnt_i2c_not_free>=def_max_cnt_i2c_not_free){
                v_i2c_engine_reset();
                uc_cnt_i2c_not_free=0;
            }
        }
        return uc_retcode;
    }//unsigned char ucWaitI2Cbus(unsigned int ui_timeout_ms)


    void vSignalI2Cbus(void){
    }//void vSignalI2Cbus(void)



    //
    //    Routine di comunicazione col DAC.
    //    Ingresso:
    //     - addrMax519: indirizzo del DAC;
    //     - out0: valore di out0 (prima   uscita del DAC)
    //     - out1: valore di out1 (seconda uscita del DAC)
    //    Uscita: nulla.
    //    La routine spedisce al DAC di indirizzo specificato i due byte che
    //    definiscono i valori delle sue due uscite analogiche.
    //
    void SendByteMax519(unsigned char addrMax519,
                        unsigned char out0,
                        unsigned char out1){
//#warning wait i2c bus very long!!!                        
//#warning trying 20ms timeout on dac settings to avoid timeout i2c and alarm i2c when doing start
        if (ucWaitI2Cbus(20)){
            if (addrMax519==0){
              ucLastCommandMandrino=out0;
              out1=ucLastCommandTaglio;
            }
            I2CWriteLength  = 4; //// numero di bytes di dati da tx, escludendo il primo byte
            I2CReadLength   = 0;
            I2CMasterBuffer[0]  = FactoryMaskBit519 | (addrMax519<<1);
            I2CMasterBuffer[1]  = CommandMaskBit519| DacOut0;
            I2CMasterBuffer[2]  = out0;
            I2CMasterBuffer[3]  = CommandMaskBit519| DacOut1;
            I2CMasterBuffer[4]  = out1;	
//#warning wait i2c bus very long!!!                        
            //I2CEngine(); 
            I2CEngine_Start(); 
            vSignalI2Cbus();
        }
    }//void SendByteMax519(unsigned char addrMax519,...


    /* Routine di comunicazione col DAC, da usare in interrupt!!!!!
       Ingresso:
         - addrMax519: indirizzo del DAC;
         - out0: valore di out0 (prima   uscita del DAC)
         - out1: valore di out1 (seconda uscita del DAC)
       Uscita: nulla.
       La routine spedisce al DAC di indirizzo specificato i due byte che
       definiscono i valori delle sue due uscite analogiche.
    */
    unsigned char ucSendCommandPosTaglioMax519_Interrupt(unsigned char out1){
		extern volatile unsigned char i2c_engine_started;
        extern unsigned char uc_requesting_i2c_bus;
//        return 1;
        // salvo il valore che deve essere impostato
        ucLastCommandTaglio=out1;
        // se semaforo non mi d� come disponibile la risorsa, esco subito
		if (i2c_engine_started||uc_requesting_i2c_bus){
        //if (!I2CEngine_isEnded()){
            return 0;
        }
        // dato che sono in interrupt, posso gestire queste risorse come credo
        // e non dovrei creare deadlock
        I2CWriteLength  = 4; // numero di bytes di dati da tx, escludendo il primo byte
        I2CReadLength   = 0;
        I2CMasterBuffer[0]  = FactoryMaskBit519 | (0<<1);
        I2CMasterBuffer[1]  = CommandMaskBit519| DacOut0;
        I2CMasterBuffer[2]  = ucLastCommandMandrino;
        I2CMasterBuffer[3]  = CommandMaskBit519| DacOut1;
        I2CMasterBuffer[4]  = out1;	
        I2CEngine_Start(); 

        return 1;
    }

    // spedizione comando di taglio max519
    unsigned char ucSendCommandPosTaglioMax519_Main(unsigned char out1){
//#warning trying 20ms timeout on dac settings to avoid timeout i2c and alarm i2c when doing start
        if (ucWaitI2Cbus(20)){
            ucLastCommandTaglio=out1;

            I2CWriteLength  = 4;  // numero di bytes di dati da tx, escludendo il primo byte
            I2CReadLength   = 0;
            I2CMasterBuffer[0]  = FactoryMaskBit519 | (0<<1);
            I2CMasterBuffer[1]  = CommandMaskBit519| DacOut0;
            I2CMasterBuffer[2]  = ucLastCommandMandrino;
            I2CMasterBuffer[3]  = CommandMaskBit519| DacOut1;
            I2CMasterBuffer[4]  = out1;	
            I2CEngine_Start(); 

            vSignalI2Cbus();
        }
        return 1;
    }



    // Inizializzazione i2c bus
    void vInitI2c(void){	
        vSignalI2Cbus();
        ucLastCommandTaglio=0;
        // inizializzazione a basso livello i2c bus
        I2CInit( (DWORD)I2CMASTER ) ;
    }//void vInitI2c(void)
#endif

#if 0
	void mainI2c(void){	
		
		
		i2c_master_xmit_buf[0]=0; 			// indirizzo zero
		
		upsd_i2c_init (defI2cbusFrequencyKhz,I2C_ADDR);						// I2C initialize
		
		vReadTimeBytesRTC_DS3231();
		// solo per resettare osf
		vWriteTimeBytesRTC_DS3231();
		
		while(1){
		#if 0
			SendByteMax519(0,k,k+100);
			SendByteMax519(1,k,k+100);
			SendByteMax519(2,k,k+100);
			SendByteMax519(3,k,k+100);
		#else
			SendByteMax519(0,0,0);
			SendByteMax519(1,0,0);
			SendByteMax519(2,0,0);
			SendByteMax519(3,0,0);
		#endif		
			k++;
			vReadTimeBytesRTC_DS3231();
			vReadStatusTempRTC_DS3231();
			// i2c_return_code = upsd_i2c_Master_Xmit(I2C_ADDR,i2c_master_xmit_buf,1);
			// i2c_return_code = upsd_i2c_Master_Recv (I2C_ADDR,i2c_master_rcv_buf,2);
		
		}
	}
#endif
