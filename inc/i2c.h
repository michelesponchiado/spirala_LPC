/*****************************************************************************
 *   i2c.h:  Header file for NXP LPC23xx/24xx Family Microprocessors
 *
 *   Copyright(C) 2006, NXP Semiconductor
 *   All rights reserved.
 *
 *   History
 *   2006.07.19  ver 1.00    Prelimnary version, first Release
 *
******************************************************************************/
#ifndef __I2C_H 
#define __I2C_H

//#define defSimulaI2c
#ifdef defSimulaI2c
    #warning *** simulating i2c ***
#endif


#define BUFSIZE			0x40
#define MAX_TIMEOUT		0x00FFFFFF

#define I2CMASTER		0x01
#define I2CSLAVE		0x02

/* For more info, read Philips's SE95 datasheet */
#define LM75_ADDR		0x90
#define LM75_TEMP		0x00
#define LM75_CONFIG		0x01
#define LM75_THYST		0x02
#define LM75_TOS		0x03

#define RD_BIT			0x01

typedef enum{
	 enum_I2C_IRQ_IDLE=0
	,enum_I2C_IRQ_STARTED
	,enum_I2C_IRQ_RESTARTED
	,enum_I2C_IRQ_REPEATED_START
	,enum_I2C_IRQ_DATA_ACK
	,enum_I2C_IRQ_DATA_NACK
	,enum_I2C_IRQ_DATA_STOP_SENDING   
	,enum_I2C_IRQ_DATA_STOP_SENT
}enum_I2C_IRQ_status;

#define I2C_IDLE			0
#define I2C_STARTED			1
#define I2C_RESTARTED		2
#define I2C_REPEATED_START	3
#define DATA_ACK			4
#define DATA_NACK			5

#define I2CONSET_I2EN		0x00000040  /* I2C Control Set Register */
#define I2CONSET_AA			0x00000004
#define I2CONSET_SI			0x00000008
#define I2CONSET_STO		0x00000010
#define I2CONSET_STA		0x00000020

#define I2CONCLR_AAC		0x00000004  /* I2C Control clear Register */
#define I2CONCLR_SIC		0x00000008
#define I2CONCLR_STAC		0x00000020
#define I2CONCLR_I2ENC		0x00000040

#define I2DAT_I2C			0x00000000  /* I2C Data Reg */
#define I2ADR_I2C			0x00000000  /* I2C Slave Address Reg */
// frequenza massima ammessa del bus i2c, in kHz, 
#define defMaxI2cFrequency_kHz 400
// frequenza desiderata del bus i2c, in kHz, max defMaxI2cFrequency_kHz
#define defDesiredI2cFrequency_kHz 400
// valore minimo dell'impostazione del registro scl dell'i2c bus
#define defMinI2cSCL_setting 4

#define I2SCLH_SCLH			0x00000080  /* I2C SCL Duty Cycle High Reg */
#define I2SCLL_SCLL			0x00000080  /* I2C SCL Duty Cycle Low Reg */

extern void I2C0MasterHandler( void );
extern DWORD I2CInit( DWORD I2cMode );
extern DWORD I2CStart( void );
extern DWORD I2CStop( void );
extern DWORD I2CEngine( void );

extern volatile DWORD I2CCount;
extern volatile BYTE I2CMasterBuffer[BUFSIZE];
extern volatile BYTE I2CMasterBuffer_Rx[BUFSIZE];
extern volatile DWORD I2CCmd, I2CMasterState;
extern volatile DWORD I2CReadLength, I2CWriteLength;

// Numero max di parametri in read/write
#define defMaxI2c_params 12

/*****************************************************************************
** Function name:		I2CEngine_Start
**
** Descriptions:		
**				Starts I2C transaction
**
** parameters:			None
** Returned value:		true or false, return false only if the
**				start condition can never be generated and
**				timed out. 
** 
*****************************************************************************/
DWORD I2CEngine_Start( void ) ;


/*****************************************************************************
** Function name:		I2CEngine_isEnded
**
** Descriptions:		verifies if i2c session has terminated
**
** parameters:			None
** Returned value:		1--> ended, 0->not ended
** 
*****************************************************************************/
unsigned int I2CEngine_isEnded( void ) ;

// reset the i2c engine; call it if timeout or so occurs
void v_i2c_engine_reset(void);

void i2c_Master_Xmit(    unsigned char ucAddress
						,unsigned char *pucTxBuf
						,unsigned char ucNumBytesDataTx
						);

void i2c_Master_Rcv(     unsigned char ucAddress
						,unsigned char *pucRxBuf
						,unsigned char ucNumBytesDataRx
						);


#endif /* end __I2C_H */
/****************************************************************************
**                            End Of File
*****************************************************************************/
