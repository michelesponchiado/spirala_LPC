/*****************************************************************************
 *   i2c.c:  I2C C file for NXP LPC23xx/24xx Family Microprocessors
 *
 *   Copyright(C) 2006, NXP Semiconductor
 *   All rights reserved.
 *
 *   History
 *   2006.07.19  ver 1.00    Prelimnary version, first Release
 *
*****************************************************************************/
#include "LPC23xx.h"                        /* LPC23xx/24xx definitions */
#include "type.h"
#include "irq.h"
#include "i2c.h"
#include "spityp.h"
#include "spiext.h"
#include "fpga_nxp.h"

volatile DWORD I2CMasterState = I2C_IDLE;
volatile DWORD I2CSlaveState = I2C_IDLE;

volatile DWORD I2CCmd;
volatile DWORD I2CMode;

volatile BYTE I2CMasterBuffer[BUFSIZE];
volatile BYTE I2CSlaveBuffer[BUFSIZE];
volatile DWORD I2CCount = 0;
volatile DWORD I2CReadLength;
volatile DWORD I2CWriteLength;

volatile DWORD RdIndex = 0;
volatile DWORD WrIndex = 0;

volatile unsigned char i2c_engine_started=0;
volatile unsigned char uc_i2c_device_signaling_nack_at_start;


/* 
From device to device, the I2C communication protocol may vary, 
in the example below, the protocol uses repeated start to read data from or 
write to the device:
For master read: the sequence is: STA,Addr(W),offset,RE-STA,Addr(r),data...STO 
for master write: the sequence is: STA,Addr(W),length,RE-STA,Addr(w),data...STO
Thus, in state 8, the address is always WRITE. in state 10, the address could 
be READ or WRITE depending on the I2CCmd.
*/   

#define def_i2c_patch

unsigned int ui_i2c_alarm_subcode;

unsigned int ui_get_i2c_alarm_subcode(void){
	return ui_i2c_alarm_subcode;
}
unsigned int ui_clear_i2c_alarm_subcode(void){
	ui_i2c_alarm_subcode=0;
	return ui_i2c_alarm_subcode;
}

typedef enum{
	enum_i2c_alarm_type_start=0,
	enum_i2c_alarm_type_stop,
	enum_i2c_alarm_type_timeout,
	enum_i2c_alarm_type_numof

}enum_i2c_alarm_type;

void v_generate_alarm_i2c_bus(enum_i2c_alarm_type alarm_type){
	// Allarme: ERRORE i2c bus
	alarms |= ALR_I2C_BUS;
	// E' effettivamente un allarme valido.
	alr_status=alr_status_alr;
	// or the mask of subcodes error
	ui_i2c_alarm_subcode|=(1<<alarm_type);
}


/*****************************************************************************
** Function name:		I2C0MasterHandler
**
** Descriptions:		I2C0 interrupt handler, deal with master mode
**				only.
**
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void I2C0MasterHandler(void)
{
  BYTE StatValue;

  /* this handler deals with master read and master write only */
  StatValue = I20STAT;
// I don't want to handle nested interrupt
//  IENABLE;				/* handles nested interrupt */	
  switch ( StatValue )
  {
#ifdef def_i2c_patch
// 10.5.1 State : 0x00
// Bus Error. Enter not addressed Slave mode and release bus.
// 1. Write 0x14 to I2CONSET to set the STO and AA bits.
// 2. Write 0x08 to I2CONCLR to clear the SI flag.
// 3. Exit
	case 0x00:
		I20CONSET = I2CONSET_AA|I2CONSET_STO;
		I20CONCLR = I2CONCLR_SIC;
        I2CMasterState = DATA_NACK;
		break;
#endif

  
	case 0x08:			/* A Start condition is issued. */
        I20DAT = I2CMasterBuffer[0];
        I20CONCLR = (I2CONCLR_SIC | I2CONCLR_STAC);
#ifdef def_i2c_patch
        I20CONSET = I2CONSET_AA;
#endif    
        I2CMasterState = I2C_STARTED;
        break;
	
	case 0x10:			/* A repeated started is issued */
        // send again the slave address+ read bit set
        I20DAT = I2CMasterBuffer[0]|RD_BIT;
        I20CONSET = I2CONSET_AA;
        I20CONCLR = (I2CONCLR_SIC | I2CONCLR_STAC);
        I2CMasterState = I2C_RESTARTED;
        break;
	
	case 0x18:			/* Regardless, it's a ACK */
        if ( I2CMasterState == I2C_STARTED )
        {
		  if (WrIndex<I2CWriteLength){
			I20DAT = I2CMasterBuffer[1+WrIndex];
			WrIndex++;
		  }
          I2CMasterState = DATA_ACK;
        }
#ifdef def_i2c_patch
        I20CONSET = I2CONSET_AA;
#endif     
        I20CONCLR = I2CONCLR_SIC;
        break;
    
#ifdef def_i2c_patch    
// 10.7.2 State : 0x20
// Slave Address + Write has been transmitted, NOT ACK has been received. A Stop
// condition will be transmitted.
// 1. Write 0x14 to I2CONSET to set the STO and AA bits.
// 2. Write 0x08 to I2CONCLR to clear the SI flag.
// 3. Exit	
	case 0x20:		
        I2CMasterState = DATA_NACK;
		I20CONSET = I2CONSET_AA|I2CONSET_STO;
		I20CONCLR = I2CONCLR_SIC;
        // set flag device is signaling nack at start...
        uc_i2c_device_signaling_nack_at_start=1;
		break;  
#endif    
#ifdef def_i2c_patch 
//State : 0x28
//Data has been transmitted, ACK has been received. If the transmitted data was the last
//data byte then transmit a Stop condition, otherwise transmit the next data byte.
//1. Decrement the Master data counter, skip to step 5 if not the last data byte.
//2. Write 0x14 to I2CONSET to set the STO and AA bits.
//3. Write 0x08 to I2CONCLR to clear the SI flag.
//4. Exit
//5. Load I2DAT with next data byte from Master Transmit buffer.
//6. Write 0x04 to I2CONSET to set the AA bit.
//7. Write 0x08 to I2CONCLR to clear the SI flag.
//8. Increment Master Transmit buffer pointer
//9. Exit	
	case 0x28:		
		// ho trasmesso tutti i caratteri???
        if ( WrIndex < I2CWriteLength )    
		{   
			I20DAT = I2CMasterBuffer[1+WrIndex]; 
            WrIndex++;
			I2CMasterState = enum_I2C_IRQ_DATA_ACK;
			I20CONSET = I2CONSET_AA;
			I20CONCLR = I2CONCLR_SIC;
		}
		else{
            I2CMasterState = DATA_NACK;
			I20CONSET = I2CONSET_AA|I2CONSET_STO;
			I20CONCLR = I2CONCLR_SIC;
		}
        break;
		
//State : 0x30
//Data has been transmitted, NOT ACK received. A Stop condition will be transmitted.
//1. Write 0x14 to I2CONSET to set the STO and AA bits.
//2. Write 0x08 to I2CONCLR to clear the SI flag.
//3. Exit
	case 0x30:
		I20CONSET = I2CONSET_AA|I2CONSET_STO;
		I20CONCLR = I2CONCLR_SIC;
        I2CMasterState = DATA_NACK;
        break;
    
#else
	case 0x28:	/* Data byte has been transmitted, regardless ACK or NACK */
	case 0x30:
        if ( WrIndex != I2CWriteLength )
        {   
          I20DAT = I2CMasterBuffer[1+WrIndex]; 
          WrIndex++;
          if ( WrIndex != I2CWriteLength )
          {   
            I2CMasterState = DATA_ACK;
            I20CONSET = I2CONSET_AA;          
          }
          else
          {
            I2CMasterState = DATA_NACK;
            if ( I2CReadLength != 0 )
            {
              I20CONSET = I2CONSET_STA|I2CONSET_I2EN;	/* Set Repeated-start flag */
              I2CMasterState = I2C_REPEATED_START;
            }
            else{
                I20CONSET = I2CONSET_AA;
            }
          }
      
        }
        else
        {
          if ( I2CReadLength != 0 )
          {
            I20CONSET = I2CONSET_STA|I2CONSET_I2EN;	/* Set Repeated-start flag */
            I2CMasterState = I2C_REPEATED_START;
          }
          else
          {
            I20CONSET = I2CONSET_AA|I2CONSET_STO;   
            I2CMasterState = DATA_NACK;
          }
        }
        I20CONCLR = I2CONCLR_SIC;
        break;
#endif

// 10.7.5 State : 0x38
// Arbitration has been lost during Slave Address + Write or data. The bus has been
// released and not addressed Slave mode is entered. A new Start condition will be
// transmitted when the bus is free again.
// 1. Write 0x24 to I2CONSET to set the STA and AA bits.
// 2. Write 0x08 to I2CONCLR to clear the SI flag.
// 3. Exit	
	case 0x38:		
		I20CONSET = I2CONSET_AA|I2CONSET_STA;
        I20CONCLR = I2CONCLR_SIC;	
		break;
    
	
	case 0x40:	/* Master Receive, SLA_R has been sent */
#ifdef def_i2c_patch        
        if ( I2CReadLength>1 )
        {   
  		  I20CONSET = I2CONSET_AA;	/* assert ACK after data is received */
		  I20CONCLR = I2CONCLR_SIC;
        }
        else
        {
		  // Michele: all'ultimo byte in ricezione devo rispondere con un NACK!!!
		  // errore nella libreria keil...
          I20CONCLR = (I2CONCLR_SIC | I2CONCLR_AAC);
        }    
#else    
        I20CONCLR = I2CONCLR_SIC;
#endif    
        break;
    
#ifdef def_i2c_patch        
// 10.8.2 State : 0x48
// Slave Address + Read has been transmitted, NOT ACK has been received. A Stop
// condition will be transmitted.
// 1. Write 0x14 to I2CONSET to set the STO and AA bits.
// 2. Write 0x08 to I2CONCLR to clear the SI flag.
// 3. Exit
	case 0x48:
		I20CONSET = I2CONSET_AA|I2CONSET_STO;
		I20CONCLR = I2CONCLR_SIC;
        break;  
#endif    


#ifdef def_i2c_patch  	
//State : 0x50
//Data has been received, ACK has been returned. Data will be read from I2DAT. Additional
//data will be received. If this is the last data byte then NOT ACK will be returned, otherwise
//ACK will be returned.
//1. Read data byte from I2DAT into Master Receive buffer.
//2. Decrement the Master data counter, skip to step 5 if not the last data byte.
//3. Write 0x0C to I2CONCLR to clear the SI flag and the AA bit.
//4. Exit
//5. Write 0x04 to I2CONSET to set the AA bit.
//6. Write 0x08 to I2CONCLR to clear the SI flag.
//7. Increment Master Receive buffer pointer
//8. Exit
	case 0x50:	
        I2CMasterBuffer[3+RdIndex] = I20DAT;
        RdIndex++;
        if ( RdIndex+1< I2CReadLength )        
		{   
			I20CONSET = I2CONSET_AA;	/* assert ACK after data is received */
			I20CONCLR = I2CONCLR_SIC;
		}
		else
		{
			// fine della ricezione!
			//hi2c.pServicingReq->ucRx_done=1;
			// Michele: all'ultimo byte in ricezione devo rispondere con un NACK!!!
			// errore nella libreria keil...
			I20CONCLR = (I2CONCLR_SIC | I2CONCLR_AAC);
		}
		break;


// State : 0x58
// Data has been received, NOT ACK has been returned. 
// Data will be read from I2DAT. A Stop condition will be transmitted.	
// 1. Read data byte from I2DAT into Master Receive buffer.
// 2. Write 0x14 to I2CONSET to set the STO and AA bits.
// 3. Write 0x08 to I2CONCLR to clear the SI flag.
// 4. Exit
	case 0x58:
        I2CMasterBuffer[3+RdIndex] = I20DAT;
        I2CMasterState = DATA_NACK;
		I20CONSET = I2CONSET_AA|I2CONSET_STO;
		I20CONCLR = I2CONCLR_SIC;
		break;
#else
	
	case 0x50:	/* Data byte has been received, regardless following ACK or NACK */
	case 0x58:
        I2CMasterBuffer[3+RdIndex] = I20DAT;
        RdIndex++;
        if ( RdIndex+1 != I2CReadLength )
        {   
          I2CMasterState = DATA_ACK;
		  I20CONSET = I2CONSET_AA|I2CONSET_I2EN;	/* assert ACK after data is received */
		  I20CONCLR = I2CONCLR_SIC;
        }
        else
        {
          RdIndex = 0;
          I2CMasterState = DATA_NACK;
		  // Michele: all'ultimo byte in ricezione devo rispondere con un NACK!!!
		  // errore nella libreria keil...
          I20CONCLR = (I2CONCLR_SIC | I2CONCLR_AAC);
        }
        break;
#endif	
#ifndef def_i2c_patch  	
    case 0x20:		/* regardless, it's a NACK */
	case 0x48:
        I20CONCLR = I2CONCLR_SIC;
        I2CMasterState = DATA_NACK;
        break;
#endif        
	
	default:
        I20CONCLR = I2CONCLR_SIC;	
        break;
  }
// Michele
    if (I2CMasterState == DATA_NACK )
    {
        int i;
        i=0;
        while(i<10){
            if ( (I20CONSET & I2CONSET_STO )==0){
                break;
            }
            v_wait_microseconds(5);
            i++;
        }
        i2c_engine_started=0;
    }

// I don't want to handle nested interrupt
//  IDISABLE;
// interrupt is already acnkowledged
//  VICVectAddr = 0;		/* Acknowledge Interrupt */
}

unsigned char uc_accessing_eeprom_private;
void v_signal_accessing_eeprom_i2c(unsigned char uc_accessing_eeprom){
    uc_accessing_eeprom_private=uc_accessing_eeprom;
}


/*****************************************************************************
** Function name:		I2CStart
**
** Descriptions:		Create I2C start condition, a timeout
**				value is set if the I2C never gets started,
**				and timed out. It's a fatal error. 
**
** parameters:			None
** Returned value:		true or false, return false if timed out
** 
*****************************************************************************/
DWORD I2CStart( void )
{
  DWORD retVal = FALSE;
  TipoStruct_timeout timeout_i2c;
  // force to idle state
  I2CMasterState = I2C_IDLE;

  /*--- Issue a start condition ---*/
  I20CONSET = I2CONSET_STA|I2CONSET_I2EN;	/* Set Start flag */
  timeout_init(&timeout_i2c);				
    
  /*--- Wait until START transmitted ---*/
  while( 1 )
  {
	// if at least the started state is ok, exit with ok
	if ( I2CMasterState >= I2C_STARTED )
	{
	  retVal = TRUE;
	  break;	
	}
	if (uc_timeout_active(&timeout_i2c, 500)){
		v_generate_alarm_i2c_bus(enum_i2c_alarm_type_start);
	    retVal = FALSE;
		break;
	}
  }
  return( retVal );
}

/*****************************************************************************
** Function name:		I2CStop
**
** Descriptions:		Set the I2C stop condition, if the routine
**				never exit, it's a fatal bus error.
**
** parameters:			None
** Returned value:		true or never return
** 
*****************************************************************************/
DWORD I2CStop( void )
{
  TipoStruct_timeout timeout_i2c;

  I20CONSET = I2CONSET_STO|I2CONSET_I2EN;      /* Set Stop flag */ 
  I20CONCLR = I2CONCLR_SIC;  /* Clear SI flag */ 
  timeout_init(&timeout_i2c);				
  /*--- Wait for STOP detected ---*/
  while( I20CONSET & I2CONSET_STO ){
	if (uc_timeout_active(&timeout_i2c, 20)){
		v_generate_alarm_i2c_bus(enum_i2c_alarm_type_stop);
		break;
	}
  }
  return TRUE;
}

/*****************************************************************************
** Function name:		I2CInit
**
** Descriptions:		Initialize I2C controller
**
** parameters:			I2c mode is either MASTER or SLAVE
** Returned value:		true or false, return false if the I2C
**				interrupt handler was not installed correctly
** 
*****************************************************************************/
DWORD I2CInit( DWORD I2cMode ) 
{
    unsigned long ul_pclk_sel;
    PCONP |= (1 << 19);
    ul_pclk_sel=PCLKSEL0;
    // reset i2c clock select bits
//#warning using 400kHz i2c bus clock frequency
    ul_pclk_sel&=~(3<<14);
    // enable 400kHz i2c bus clock
    ul_pclk_sel |=(2<<14);
    // enable 200kHz i2c bus clock
    //  ul_pclk_sel |=(1<<14);
    // enable 100kHz i2c bus clock
    //  ul_pclk_sel |=(0<<14);
    PCLKSEL0=ul_pclk_sel;
    {
        unsigned long ul;
	// select sda0 e scl0 for i2c bus
        ul=PINSEL1;
        // reset settings on P0.27=sda0, P0.28=scl0
        ul &=~( (3<<22)|(3<<24) );
        // set settings on P0.27=sda0=1, P0.28=scl0=1
        ul |= ( (1<<22)|(1<<24) );
        PINSEL1 = ul; // was 0x0140 0000;
    }
    /* function to 01 on both SDA and SCK. */
    /*--- Clear flags ---*/
    I20CONCLR = I2CONCLR_AAC | I2CONCLR_SIC | I2CONCLR_STAC | I2CONCLR_I2ENC;    
    i2c_engine_started=0;

    /*--- Reset registers ---*/
    // I20SCLL   = I2SCLL_SCLL;
    // I20SCLH   = I2SCLH_SCLH;
// force 400kHz frequency on i2c bus...    
    I20SCLL   = I2SCLL_SCLL/4;
    I20SCLH   = I2SCLH_SCLH/4;
    if ( I2cMode == I2CSLAVE )
    {
        I20ADR = LM75_ADDR;
    }    

    /* Install interrupt handler */	

    VICIntSelect &= ~(1<<9); // I2C0 selected as IRQ
    VICVectPriority9 = 15; // set priority for I2Co to be quite low
    VICVectAddr9 = (unsigned long)I2C0MasterHandler; 
    VICIntEnable |= (1<<9); // I2C0 interrupt enabled

    I20CONSET = I2CONSET_I2EN;
    return( TRUE );
}

/*****************************************************************************
** Function name:		I2CEngine
**
** Descriptions:		The routine to complete a I2C transaction
**				from start to stop. All the intermitten
**				steps are handled in the interrupt handler.
**				Before this routine is called, the read
**				length, write length, I2C master buffer,
**				and I2C command fields need to be filled.
**				see i2cmst.c for more details. 
**
** parameters:			None
** Returned value:		true or false, return false only if the
**				start condition can never be generated and
**				timed out. 
** 
*****************************************************************************/
DWORD I2CEngine( void ) 
{
    TipoStruct_timeout timeout_i2c;
    unsigned int ui_num_try_i2c_engine;
    ui_num_try_i2c_engine=10;
    while(ui_num_try_i2c_engine--){

        I2CMasterState = I2C_IDLE;
        RdIndex = 0;
        WrIndex = 0;
        uc_i2c_device_signaling_nack_at_start=0;
        if ( I2CStart() != TRUE )
        {
            I2CStop();
            return ( FALSE );
        }

        timeout_init(&timeout_i2c);				
        while ( 1 )
        {
            if (uc_timeout_active(&timeout_i2c, 100)){
                v_generate_alarm_i2c_bus(enum_i2c_alarm_type_timeout);
                break;
            }
            if ( I2CMasterState == DATA_NACK )
            {
                // michele!
                //I2CStop();
                break;
            }
        }    
        if (!uc_accessing_eeprom_private){
            break;
        }
        if (!uc_i2c_device_signaling_nack_at_start){
            break;
        }
        // wait some microseconds...
        v_wait_microseconds(1000);
    }
    return ( TRUE );      
}


/*****************************************************************************
** Function name:		I2CStart
**
** Descriptions:		Create I2C start condition, a timeout
**				value is set if the I2C never gets started,
**				and timed out. It's a fatal error. 
**
** parameters:			None
** Returned value:		true or false, return false if timed out
** 
*****************************************************************************/
DWORD I2CStart_noWait( void )
{

 
  /*--- First, clear all the flags ---*/
  I20CONCLR = I2CONCLR_AAC | I2CONCLR_SIC | I2CONCLR_STAC ;    
  /*--- Then, issue a start condition ---*/
  I20CONSET = I2CONSET_I2EN|I2CONSET_STA;	/* Set Start flag */
    
  /*--- do not Wait until START transmitted ---*/
  return 1;
}



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
DWORD I2CEngine_Start( void ) 
{
#ifdef defSimulaI2c
  return ( TRUE );      
#endif
    // non si dovrebbe mai verificare che d� start mentre i2c bus � impegnato...
    // per�...
    if (i2c_engine_started){
        I2CStop();
	}
    
    // imposto semaforo che indica i2c bus impegnato
    i2c_engine_started=1;
    RdIndex = 0;
    WrIndex = 0;    
    I2CMasterState = enum_I2C_IRQ_IDLE;
    I2CStart_noWait();
    

    return ( TRUE );      
}


// reset the i2c engine; call it if timeout or so occurs
void v_i2c_engine_reset(void){
    I2CStop();
    // reset semaforo che indica i2c bus impegnato
    i2c_engine_started=0;
    RdIndex = 0;
    WrIndex = 0;    
    I2CMasterState = enum_I2C_IRQ_IDLE;
}
/*****************************************************************************
** Function name:		I2CEngine_isEnded
**
** Descriptions:		verifies if i2c session has terminated
**
** parameters:			None
** Returned value:		1--> ended, 0->not ended
** 
*****************************************************************************/
unsigned int I2CEngine_isEnded( void ) 
{
#ifdef defSimulaI2c
  return ( TRUE );      
#endif
    if (!i2c_engine_started){
        return 1;
	}
// michele    
    if (0&& I2CMasterState == enum_I2C_IRQ_DATA_NACK )
    {
        I2CStop();
        i2c_engine_started=0;
        return 1;
    }
    return 0;      
}
// verifies if i2c bus should be unlocked...
void v_verify_unlock_i2c_bus(void){
	I2CEngine_isEnded();
}

/******************************************************************************
**                            End Of File
******************************************************************************/

