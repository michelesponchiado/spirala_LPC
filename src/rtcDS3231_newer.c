#include "LPC23xx.h"                        /* LPC23xx/24xx definitions */
#include "stdlib.h"
#include "string.h"
#include "type.h"
#include "irq.h"
#include "i2c.h"
#include "i2c_master.h"
#include "rtcDS3231.h"


#define defNumBytesRTC_DS3231 0x12
#define defi2cAddr_RTC_DS3231 0xD0
unsigned char ucBytesRTC_DS3231[defNumBytesRTC_DS3231];
unsigned char i2c_master_xmit_buf[defMaxI2c_params];
unsigned char i2c_master_rcv_buf[defMaxI2c_params];
extern volatile unsigned char i2c_engine_started;
extern unsigned char uc_requesting_i2c_bus;


TipoStructRTC rtc;

//#define defSimulaRtc

#ifdef defSimulaRtc
#warning *** rtc simulation! ***
    void i2c_Master_Xmit(    unsigned char ucAddress
                            ,unsigned char *pucTxBuf
                            ,unsigned char ucNumBytesDataTx
                            ){
    }

    void i2c_Master_Rcv(     unsigned char ucAddress
                            ,unsigned char *pucRxBuf 
                            ,unsigned char ucNumBytesDataRx
                            ){
    }

    // routine per leggere il flag di oscillator stop flag e la temperatura
    void vReadStatusTempRTC_DS3231(void){
    }

    // routine per resettare oscillator stop flag
    void vResetOSF_RTC_DS3231(void){
    }



    void vReadTimeBytesRTC_DS3231(void){
    }//void vReadTimeBytesRTC_DS3231(void)


    void vWriteTimeBytesRTC_DS3231(void){
    }//void vWriteTimeBytesRTC_DS3231(void)



    // inizializzazione realtime clock
    void vInit_RtcDS3231(void){
    }

#else


void i2c_Master_Xmit(    unsigned char ucAddress
						,unsigned char *pucTxBuf
						,unsigned char ucNumBytesDataTx
						){
	if (ucNumBytesDataTx+1>BUFSIZE)
		return;
    I2CWriteLength = ucNumBytesDataTx;
    I2CReadLength = 0;
    I2CMasterBuffer[0] = ucAddress;
	memcpy((void*)&I2CMasterBuffer[1],pucTxBuf,ucNumBytesDataTx);
    I2CEngine(); 
}

void i2c_Master_Rcv(     unsigned char ucAddress
						,unsigned char *pucRxBuf 
						,unsigned char ucNumBytesDataRx
						){
	// sfortunatamente la realizzazione attuale prevede che il dato di ritorno sia copiato a partire
	// dal terzo elemento
	if (ucNumBytesDataRx>BUFSIZE)
		return;
	// no data bytes, only the slave address should be sent
    I2CWriteLength = 0;
    I2CReadLength = ucNumBytesDataRx;
	// imposto rd_bit!!!
    I2CMasterBuffer[0] = ucAddress|RD_BIT;
    I2CEngine(); 
	if (ucNumBytesDataRx)
		memcpy((char*)pucRxBuf,(char*)&I2CMasterBuffer[3],ucNumBytesDataRx);
    
	// if (ucNumBytesDataRx)
		// memcpy((char*)pucRxBuf,(char*)&I2CMasterBuffer_Rx[0],ucNumBytesDataRx);
}




// routine per leggere il flag di oscillator stop flag e la temperatura
void vReadStatusTempRTC_DS3231(void){

	if (ucWaitI2Cbus(20)){
		i2c_engine_started=1;
			// indirizzo registro control/status
			i2c_master_xmit_buf[0]=0x0f;
			i2c_Master_Xmit(defi2cAddr_RTC_DS3231,i2c_master_xmit_buf,1);
			i2c_Master_Rcv(defi2cAddr_RTC_DS3231,i2c_master_rcv_buf ,4);
		i2c_engine_started=0;
		// spedisco richiesta di accedere all'indirizzo 0x0f
		//i2c_return_code = upsd_i2c_Master_Xmit(defi2cAddr_RTC_DS3231,i2c_master_xmit_buf,1);
		// leggo 4 bytes
		//i2c_return_code = upsd_i2c_Master_Recv (defi2cAddr_RTC_DS3231,i2c_master_rcv_buf,0x4);
		vSignalI2Cbus();
	}

	// se il control/status reg ha osf=1, allora oscillator was stopped!
	if (i2c_master_rcv_buf[0]&0x80){
		rtc.ucErrorTheTimeIsNotValid=1;
	}
	else
		rtc.ucErrorTheTimeIsNotValid=0;
	// ecco gli 8 bit piu' significativi
	rtc.iTemperature=i2c_master_rcv_buf[2];
	// sign extension
	if (i2c_master_rcv_buf[2]&0x80)	
		rtc.iTemperature|=0xFF00;
	// occhio perché la temperatura è data con una risoluzione di 0.25 gradi centigradi
	// ecco i due bit meno significativi: 0 25 50 75
	rtc.ucTemperatureDec=(i2c_master_rcv_buf[3]>>6)*25;
	rtc.fTemperature=rtc.iTemperature+rtc.ucTemperatureDec*0.01;
}

// routine per resettare oscillator stop flag
void vResetOSF_RTC_DS3231(void){
	// leggo status control/register
	vReadStatusTempRTC_DS3231();

	if (ucWaitI2Cbus(1)){
		i2c_engine_started=1;
			// indirizzo della prima word trasmessa
			i2c_master_xmit_buf[0]=0x0F;
			// reset bit osf
			i2c_master_xmit_buf[1]=i2c_master_rcv_buf[0]&0x7F;
			i2c_Master_Xmit(defi2cAddr_RTC_DS3231,i2c_master_xmit_buf,2);
		i2c_engine_started=0;
		// spedisco richiesta di accedere all'indirizzo 0x0f
		//i2c_return_code = upsd_i2c_Master_Xmit(defi2cAddr_RTC_DS3231,i2c_master_xmit_buf,2);
		vSignalI2Cbus();
	}
}



void vReadTimeBytesRTC_DS3231(void){
    register unsigned char ucAux;

		if (ucWaitI2Cbus(30)){
			i2c_engine_started=1;
				i2c_master_xmit_buf[0]=0;
				//i2c_return_code = upsd_i2c_Master_Xmit(defi2cAddr_RTC_DS3231,i2c_master_xmit_buf,1);
				//i2c_return_code = upsd_i2c_Master_Recv (defi2cAddr_RTC_DS3231,i2c_master_rcv_buf,0x7);
				i2c_Master_Xmit(defi2cAddr_RTC_DS3231,i2c_master_xmit_buf,1);
				i2c_Master_Rcv(defi2cAddr_RTC_DS3231,i2c_master_rcv_buf ,7);
			i2c_engine_started=0;
			vSignalI2Cbus();
		}

        // trasformo in ore minuti secondi etc
        ucAux=i2c_master_rcv_buf[0];
        rtc.ucSecondi=(ucAux&0x0F)+(ucAux>>4)*10;
        ucAux=i2c_master_rcv_buf[1];
        rtc.ucMinuti=(ucAux&0x0F)+(ucAux>>4)*10;
        ucAux=i2c_master_rcv_buf[2];
        // bit che indica 12/24*
        if (ucAux&0x40){ // se vale 1, sono nel modo 12 ore
            rtc.ucOre=(ucAux&0x0F)+((ucAux>>4)&0x01)*10;;
            if (ucAux&0x20) // se PM, devo aggiungere 12 ore
                rtc.ucOre+=12;
        }
        else{
            rtc.ucOre=(ucAux&0x0F)+((ucAux>>4)&0x03)*10;;
        }
        rtc.ucDayOfWeek=i2c_master_rcv_buf[3];
        ucAux=i2c_master_rcv_buf[4];
        rtc.ucGiorno=(ucAux&0x0F)+(ucAux>>4)*10;
        ucAux=i2c_master_rcv_buf[5];
        rtc.ucMese=(ucAux&0x0F)+((ucAux>>4)&0x01)*10;
        rtc.ucSecolo=(ucAux&0x80)?1:0;
        ucAux=i2c_master_rcv_buf[6];
        rtc.ucAnno=(ucAux&0x0F)+(ucAux>>4)*10;
}//void vReadTimeBytesRTC_DS3231(void)


void vWriteTimeBytesRTC_DS3231(void){
    register unsigned char ucAux;

		if (ucWaitI2Cbus(20)){
			i2c_engine_started=1;
				// indirizzo della prima word trasmessa
				i2c_master_xmit_buf[0]=0;
				// trasformo in ore minuti secondi etc
				ucAux=(rtc.ucSecondi%10)|((rtc.ucSecondi/10)<<4);
				i2c_master_xmit_buf[1]=ucAux;
				
				ucAux=(rtc.ucMinuti%10)|((rtc.ucMinuti/10)<<4);
				i2c_master_xmit_buf[2]=ucAux;
				
				ucAux=(rtc.ucOre%10)|((rtc.ucOre/10)<<4);
				i2c_master_xmit_buf[3]=ucAux;
				
				i2c_master_xmit_buf[4]=rtc.ucDayOfWeek;
				
				ucAux=(rtc.ucGiorno%10)|((rtc.ucGiorno/10)<<4);
				i2c_master_xmit_buf[5]=ucAux;
				
				ucAux=(rtc.ucMese%10)|((rtc.ucMese/10)<<4)|((rtc.ucSecolo&0x01)<<7);
				i2c_master_xmit_buf[6]=ucAux;
				
				ucAux=(rtc.ucAnno%10)|((rtc.ucAnno/10)<<4);
				i2c_master_xmit_buf[7]=ucAux;
	//			i2c_return_code = upsd_i2c_Master_Xmit(defi2cAddr_RTC_DS3231,i2c_master_xmit_buf,0x8);          
	//            i2c_return_code = upsd_i2c_Master_Recv (defi2cAddr_RTC_DS3231,i2c_master_rcv_buf,0x7);

				i2c_Master_Xmit(defi2cAddr_RTC_DS3231,i2c_master_xmit_buf,8);
				i2c_Master_Rcv(defi2cAddr_RTC_DS3231,i2c_master_rcv_buf ,7);
			i2c_engine_started=0;

			vSignalI2Cbus();
		}

        // azzero flag che indica tempo non valido
        vResetOSF_RTC_DS3231();
//#endif        
}//void vWriteTimeBytesRTC_DS3231(void)



// inizializzazione realtime clock
void vInit_RtcDS3231(void){
	vReadTimeBytesRTC_DS3231();
	// azzero flag che indica tempo non valido
	vResetOSF_RTC_DS3231();

}
#endif