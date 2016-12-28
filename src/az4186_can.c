/*****************************************************************************
 *   az4186_can.c:  can module file for az4186 csm board
 *
 *   Copyright(C) 2012,CSM GROUP
 *   All rights reserved.
 *
 *   History
 *   2012.11.23  ver 1.00    Prelimnary version, first Release
 *
******************************************************************************/
#include "LPC23xx.h"                        /* LPC23xx/24xx definitions */
#include "type.h"
#include "target.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fpga_nxp.h"
#include "az4186_emc.h"
#include "irq.h"
#include "az4186_can.h"
#include "lcd.h"
#include "CO_driver.h"
#include "CANopen.h"
#ifdef def_canopen_enable
    #include "spiext.h"
#endif
#include "spiwin.h"

// for debug...
extern void v_print_msg_forebackcolor(unsigned int row,char *s,unsigned int ui_fore_color,unsigned int ui_back_color);

typedef enum{
  enum_can_tc_type_K=0,          //    0 0000 TC type K
  enum_can_tc_type_J=1,          //    1 0001 TC type J
  enum_can_tc_type_E=2,          //    2 0010 TC type E
  enum_can_tc_type_R=3,          //    3 0011 TC type R
  enum_can_tc_type_S=4,          //    4 0100 TC type S
  enum_can_tc_type_T=5,          //    5 0101 TC type T
  enum_can_tc_type_B=6,          //    6 0110 TC type B
  enum_can_tc_type_N=7,          //    7 0111 TC type N
  enum_can_tc_type_U=8,          //    8 1000 TC type U
  enum_can_tc_type_L=9,          //    9 1001 TC type L
  enum_can_tc_type_C=10,         //    10 1010 TC type C
  enum_can_tc_type_W=11,         //    11 1011 TC type W
  enum_can_tc_type_HK=12,        //    12 1100 TC type HK
  enum_can_tc_type_ColdJ=13,     //    13 1101 Cold junction (CJ)
  enum_can_tc_type_Voltage=14,   //    14 1110 U: Voltage (-15 mV ... 15 1111 Reserved
  enum_can_tc_type_numof

}enum_can_tc_types;

// Receive Queue: one queue for each CAN port
CAN_MSG MsgBuf_RX1, MsgBuf_RX2;
CAN_MSG MsgBuf_TX1, MsgBuf_TX2; // TX Buffers for CAN message
volatile DWORD CAN1RxDone, CAN2RxDone;

DWORD CANStatus;
DWORD CAN1RxCount = 0, CAN2RxCount = 0;
DWORD CAN1ErrCount = 0, CAN2ErrCount = 0;
typedef struct _CanRegs{
    unsigned long rfs;
    unsigned char dlc;
}CanRegs;
CanRegs canregs[2];
CO_CANrxMsg_t canopen_rx_msg[2];

/******************************************************************************
** Function name:		CAN_ISR_Rx1
**
** Descriptions:		CAN Rx1 interrupt handler
**
** parameters:			None
** Returned value:		None
** 
******************************************************************************/
void CAN_ISR_Rx1( void )
{
	DWORD * pDest;

	// initialize destination pointer
	pDest = (DWORD *)&MsgBuf_RX1;
	*pDest = CAN1RFS;  // Frame

	pDest++;
	*pDest = CAN1RID; // ID		//change by gongjun

	pDest++;
	*pDest = CAN1RDA; // Data A

	pDest++;
	*pDest = CAN1RDB; // Data B7

    canregs[0].rfs=CAN1RFS;
// save number of bytes received on can1 port
    canregs[0].dlc=(canregs[0].rfs>>16)&0x0F;
// callback enabled?
    if (ui_is_enabled_callback(enum_LPC_CAN_Port_1)){
    // now it's time to call the callback
        canopen_rx_msg[0].ident=MsgBuf_RX1.MsgID;
        canopen_rx_msg[0].DLC=canregs[0].dlc;
        memcpy(canopen_rx_msg[0]._data  ,&MsgBuf_RX1.DataA,4);
        memcpy(canopen_rx_msg[0]._data+4,&MsgBuf_RX1.DataB,4);
        CAN1callback(enum_LPC_event_rx_full, &canopen_rx_msg[0]);
    }

	CAN1RxDone = TRUE;
	CAN1CMR = 0x04; // release receive buffer
}


/******************************************************************************
** Function name:		CAN_ISR_Rx2
**
** Descriptions:		CAN Rx2 interrupt handler
**
** parameters:			None
** Returned value:		None
** 
******************************************************************************/
void CAN_ISR_Rx2( void )
{
	DWORD *pDest;

	// initialize destination pointer
	pDest = (DWORD *)&MsgBuf_RX2;
	*pDest = CAN2RFS;  // Frame

	pDest++;
	*pDest = CAN2RID; // ID

	pDest++;
	*pDest = CAN2RDA; // Data A

	pDest++;
	*pDest = CAN2RDB; // Data B
    canregs[1].rfs=CAN2RFS;
// save number of bytes received on can2 port
    canregs[1].dlc=(canregs[1].rfs>>16)&0x0F;
// callback enabled?
    if (ui_is_enabled_callback(enum_LPC_CAN_Port_2)){
    // now it's time to call the callback
        canopen_rx_msg[1].ident=MsgBuf_RX2.MsgID;
        canopen_rx_msg[1].DLC=canregs[1].dlc;
        memcpy(canopen_rx_msg[1]._data  ,&MsgBuf_RX2.DataA,4);
        memcpy(canopen_rx_msg[1]._data+4,&MsgBuf_RX2.DataB,4);
        CAN2callback(enum_LPC_event_rx_full, &canopen_rx_msg[1]);
    }

	CAN2RxDone = TRUE;
	CAN2CMR = 0x04; // release receive buffer
}



/*****************************************************************************
** Function name:		CAN_irq_handler
**
** Descriptions:		CAN interrupt handler
**
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void CAN_irq_handler(void)
{		

	CANStatus = CAN_RX_SR;
	if ( CANStatus & (1 << 8) )
	{
		CAN1RxCount++;
		CAN_ISR_Rx1();
	}
	if ( CANStatus & (1 << 9) )
	{
		CAN2RxCount++;
		CAN_ISR_Rx2();
	}
	if ( CAN1GSR & (1 << 6 ) )
	{
		// The error count includes both TX and RX 
		CAN1ErrCount = (CAN1GSR >> 16 );
	}
	if ( CAN2GSR & (1 << 6 ) )
	{
		// The error count includes both TX and RX 
		CAN2ErrCount = (CAN2GSR >> 16 );
	}
//	IDISABLE;
}


// can bus module clock can_clk frequency derives from fcclk
// can_clk_in=fcclk/clk_can_divisor
// where clk_can_divisor is in {1,2,4,6}, depending upon pclk_can1/pclk_can2/pclk_acf
// pclk_can1/pclk_can2/pclk_acf  clk_can_divisor
//   0								4
//   1								1
//   2								2
//   3								6
//

// now, the can_clk_in frequency can be lowered to obtain the effective bit sampling frequency
// can_clk_sampling=can_clk_in*(1+BRP=canxbtr(9..0))
//
// now, after the sync, we should wait can_tseg1_time before sampling, then sample for 1 sample time, then wait can_tseg1_time until next sync
// can_tseg1_time=(1+canxbtr(19..16))
// can_tseg2_time=(1+canxbtr(22..20))
// the recommended value for tseg2 is
//  Set TSEG2 equal to SJW. 
//
// can_bit_frequency=can_clk_sampling/(can_tseg1_time+1+can_tseg2_time+1+1) (the last '1' is the sync period)
//
// we can also define how many bits to use to sample the bit: we can choose between once and 3 times
// SAM=canxbtr(23)=1--> samples 3 times, 0--> samples once
// but as you can see in http://www.vector.com/portal/medien/vector_cantech/Sponsorship/EcoCar/AN-AND-1-106_Basic_CAN_Bit_Timing.pdf
// 	*** Use single sample mode.
//
// about the resync jump width, RJW = (1+canxbtr(15..14))
//  ***	It is recommended that a large Resynchronization Jump Width be used, the maximum value is 3+1=4
//
// now suppose you want to go @1MHz bit frequency, with fcclk=60MHz
// we can choose a can_clk_sampling 12 times higher, so can_clk_sampling=12MHz (but we can also choose 20MHz or 40MHz)
// then (can_tseg1_time+1+can_tseg2_time+1+1)=12/1=12, so 
// can_tseg1_time+can_tseg2_time=12-3=9
// now we can set tseg1 and tseg2 where to sample the bit, i.e. we can choose eg tseg1=9, tseg2=0, so the bit will be sample @10/12=83% of bit period
// about this, please note that: 
//   Application Note AN-AND-1-106
// 		http://www.vector.com/portal/medien/vector_cantech/Sponsorship/EcoCar/AN-AND-1-106_Basic_CAN_Bit_Timing.pdf
//     	*** Use sample points of 80% to 90%. ***
// 			Arbitrary values like 50% or 75% all seem to work fine in the lab on the bench top with short wires, but when the
//  		module is placed into its real system environment, the effects of longer wires and increased capacitance may
//    		cause communication errors that result from an inadequate sample point.

// so given the needed can bus frequency, it is easy to determine the values for tseg1 and tseg2 so that 0.8*(tseg1+tseg2+3)<=(tseg1+2)<=0.9*(tseg1+tseg2+3)
// basically, the setup tseg1=14, tseg2=3 would work fine --> (2+tseg1)/(3+tseg1+tseg2)=16/20=80%
// and please note that 1+tseg1+1+tseg2+1 gives 20
// then RJW=SJW=3<=tseg2=3 ok
// and SAM=0
// so finally we can set brp
// from the oscillator (eg 60Mhz) we should obtain 20 time quantum and a final frequency of let's say 1MHz
// 1*20=20MHz --> we can divide the frequency by 1 (pclk) then by 3 (brp+1), so we can choose BRP=2



// the frequencies, in Hz
unsigned long ul_can_frequency_Hz[enum_az4186_canbus_frequency_numof]={
	1000000,
	500000,
	250000,
	125000
};

// initialize and sets the canbus frequency needed
// returns 0 if the frequency can't be set or initialization fails
// return 1 if the frequency has been set ok and initialization is ok
unsigned int ui_init_canbus(enum_LPC_CAN_Ports can_ports,enum_az4186_canbus_frequency az4186_canbus_frequency){
// is the frequence a valid one?
	if (az4186_canbus_frequency>=enum_az4186_canbus_frequency_numof){
		return 0;
	}
    if (  (can_ports!=enum_LPC_CAN_Port_both)
        &&(can_ports!=enum_LPC_CAN_Port_1)
        &&(can_ports!=enum_LPC_CAN_Port_2)
       ){
		return 0;
    }

// enable canbus clock		
	PCONP |= ((1 << 13) | (1 << 14));	
// set canbus clock divider to 1 on CAN1 CAN2 and ACCEPTANCE FILTER
	{
		unsigned long ul;
		ul=PCLKSEL0;
		// reset settings on PCLKSEL0.27..26=PCLK_CAN1, PCLKSEL0.29..28=PCLK_CAN2, PCLKSEL0.31..30=PCLK_ACF
		ul &=~( (3<<26)|(3<<28)|(3<<30)); 	// 
		// set settings on PCLKSEL0.27..26=PCLK_CAN1=1-->DIV BY 1, PCLKSEL0.29..28=PCLK_CAN2=1-->DIV BY 1, PCLKSEL0.31..30=PCLK_ACF=1-->DIV BY 1
		ul |= ( (1<<26)|(1<<28)|(1<<30));	// 
		PCLKSEL0 = ul; 
	}	
// enable canbus pins	
	{
		unsigned long ul;
		ul=PINSEL0;
		// reset settings on P0.0=rd1, P0.1=td1, P0.4=rd2, P0.5=td2
		ul &=~( (3<<10)|(3<<8)|(3<<2)|(3<<0)); 	// 0x00000F0F
		// set settings on P0.0=1=rd1, P0.1=1=td1, P0.4=2=rd2, P0.5=2=td2
		ul |= ( (2<<10)|(2<<8)|(1<<2)|(1<<0));	// 0x0000A05
		PINSEL0 = ul; 
	}
    if (can_ports==enum_LPC_CAN_Port_both){
    // performs reset to initialize registers
        CAN1MOD = CAN2MOD = 1;	// Reset CAN
        CAN1IER = CAN2IER = 0;	// Disable Receive Interrupt
        CAN1GSR = CAN2GSR = 0;	// Reset error counter: CANxMOD is in reset
    }
    else if (can_ports==enum_LPC_CAN_Port_1){
    // performs reset to initialize registers
        CAN1MOD  = 1;	// Reset CAN
        CAN1IER  = 0;	// Disable Receive Interrupt
        CAN1GSR  = 0;	// Reset error counter: CANxMOD is in reset
    }
    else if (can_ports==enum_LPC_CAN_Port_2){
    // performs reset to initialize registers
        CAN2MOD  = 1;	// Reset CAN
        CAN2IER  = 0;	// Disable Receive Interrupt
        CAN2GSR  = 0;	// Reset error counter: CANxMOD is in reset
    }
	
// now I can set the canbus register for frequency setting
	{
		unsigned long ul_tseg1;
		unsigned long ul_tseg2;
		unsigned long ul_sjw;
		unsigned long ul_sam;
		unsigned long ul_brp;
		unsigned long ul_needed_frequency_Hz;
		unsigned long ul_canxbtr_value;
		// for these setting, see previous notes and 
			// 		http://www.vector.com/portal/medien/vector_cantech/Sponsorship/EcoCar/AN-AND-1-106_Basic_CAN_Bit_Timing.pdf
		ul_tseg1=14;
		ul_tseg2=3;
		ul_sjw=3;	// bigger sjw ever
		ul_sam=0; 	//just one sampling
		ul_needed_frequency_Hz=ul_can_frequency_Hz[az4186_canbus_frequency];
		// calculate brp value, see previous notes
		ul_brp=Fcclk/(ul_needed_frequency_Hz*(ul_tseg1+1+ul_tseg2+1+1));
		// remember the +1 offset
		if (ul_brp)
			ul_brp--;
		// check if valid value obtained
		if (ul_brp>((1<<10)-1))
			return 0;
		// now obtain the register value
		ul_canxbtr_value=	  ( (ul_brp  &0x3ff) <<  0)
							| ( (ul_sjw  &0x003) << 14)
							| ( (ul_tseg1&0x00f) << 16)
							| ( (ul_tseg2&0x007) << 20)
							| ( (ul_sam  &0x001) << 22) ;
		// set same values to both CAN channels
        if (can_ports==enum_LPC_CAN_Port_both){        
            CAN1BTR=ul_canxbtr_value;
            CAN2BTR=ul_canxbtr_value;
        }
        else if (can_ports==enum_LPC_CAN_Port_1){        
            CAN1BTR=ul_canxbtr_value;
        }
        else if (can_ports==enum_LPC_CAN_Port_2){        
            CAN2BTR=ul_canxbtr_value;
        }
	}
// back to normal operation mode...	
    if (can_ports==enum_LPC_CAN_Port_both){        
        CAN1MOD = CAN2MOD = 0x0;	// CAN in normal operation mode
    }
    else if (can_ports==enum_LPC_CAN_Port_1){        
        CAN1MOD = 0x0;	// CAN in normal operation mode
    }
    else if (can_ports==enum_LPC_CAN_Port_2){        
        CAN2MOD = 0x0;	// CAN in normal operation mode
    }

// Install CAN interrupt handler
    VICIntSelect 		&= ~(1<<CAN_INT); 		// CAN is a IRQ
    VICVectPriority23 	= HIGHEST_PRIORITY ; 	// set priority for IRQ to be quite high
    VICVectAddr23 		= (unsigned long)CAN_irq_handler;
    VICIntEnable 		|= 1<<CAN_INT; 			// CAN interrupt enabled

// so finally enable rx ints	
    if (can_ports==enum_LPC_CAN_Port_both){        
        CAN1IER = CAN2IER = 0x01;		// Enable receive interrupts
    }
    else if (can_ports==enum_LPC_CAN_Port_1){        
        CAN1IER = 0x01;		// Enable receive interrupts
    }
    else if (can_ports==enum_LPC_CAN_Port_2){        
        CAN2IER = 0x01;		// Enable receive interrupts
    }

// that's fine, returns ok	
	return 1;
}// unsigned int ui_init_canbus(enum_LPC_CAN_Ports can_ports,enum_az4186_canbus_frequency az4186_canbus_frequency)


/******************************************************************************
** Function name:		CAN_set_acceptance_filter_lookup
**
** Descriptions:		set  acceptance filter lookup table
**
** parameters:			none
** Returned value:		nope
** 
******************************************************************************/
void CAN_set_acceptance_filter_lookup( void )
{
	DWORD address = 0;
	DWORD i;
	DWORD ID_high, ID_low;

	// Set explicit standard Frame  
	CAN_SFF_SA = address;
    i=0;
	while( i < ACCF_IDEN_NUM  )
	{
		ID_low = (i << 29) | (EXP_STD_ID << 16);
		ID_high = ((i+1) << 13) | (EXP_STD_ID << 0);
		*((volatile DWORD *)(CAN_MEM_BASE + address)) = ID_low | ID_high;
		address += 4; 
        i += 2;
	}

	// Set group standard Frame 
	CAN_SFF_GRP_SA = address;
    i=0;
	while ( i < ACCF_IDEN_NUM )
	{
		ID_low = (i << 29) | (GRP_STD_ID << 16);
		ID_high = ((i+1) << 13) | (GRP_STD_ID << 0);
		*((volatile DWORD *)(CAN_MEM_BASE + address)) = ID_low | ID_high;
		address += 4; 
        i += 2;
	}

	// Set explicit extended Frame 
	CAN_EFF_SA = address;
    i=0;
	while ( i < ACCF_IDEN_NUM  )
	{
		ID_low = (i << 29) | (EXP_EXT_ID << 0);
		*((volatile DWORD *)(CAN_MEM_BASE + address)) = ID_low;
		address += 4; 
        i++;
	}

	// Set group extended Frame 
	CAN_EFF_GRP_SA = address;
    i=0;
	while ( i < ACCF_IDEN_NUM  )
	{
        ID_low = (i << 29) | (GRP_EXT_ID << 0);
        *((volatile DWORD *)(CAN_MEM_BASE + address)) = ID_low;
        address += 4; 
        i++;
	}

	// Set End of Table 
	CAN_EOT = address;

}//CAN_set_acceptance_filter_lookup


/******************************************************************************
** Function name:		CAN_set_acceptance_filter
**
** Descriptions:		Set acceptance filter and SRAM associated with	
**
** parameters:			ACMF mode
** Returned value:		None
**
** 
******************************************************************************/
void CAN_set_acceptance_filter(DWORD acceptance_filter_mode){
	switch ( acceptance_filter_mode )
	{
		case ACCF_OFF:
			CAN_AFMR = acceptance_filter_mode;
			CAN1MOD = CAN2MOD = 1;	// Reset CAN
			CAN1IER = CAN2IER = 0;	// Disable Receive Interrupt
			CAN1GSR = CAN2GSR = 0;	// Reset error counter when CANxMOD is in reset
			break;

		case ACCF_BYPASS:
			CAN_AFMR = acceptance_filter_mode;
			break;

		case ACCF_ON:
		case ACCF_FULLCAN:
			CAN_AFMR = ACCF_OFF;
			CAN_set_acceptance_filter_lookup();
			CAN_AFMR = acceptance_filter_mode;
			break;

		default:
			break;
	}
}



/******************************************************************************
** Function name:		dw_can1_send_message
**
** Descriptions:		Send message block to CAN1	
**
** parameters:			pointer to the CAN message
** Returned value:		if message buffer is available, and message can be sent successfully, returns TRUE,
**						otherwise, return FALSE.
** 
******************************************************************************/
DWORD dw_can1_send_message( CAN_MSG *pTxBuf )
{
	DWORD CANStatus;

	CANStatus = CAN1SR;
	if ( CANStatus & 0x00000004 )
	{
		CAN1TFI1 = pTxBuf->Frame & 0xC00F0000;
		CAN1TID1 = pTxBuf->MsgID;
		CAN1TDA1 = pTxBuf->DataA;
		CAN1TDB1 = pTxBuf->DataB;
		CAN1CMR = 0x21;
		return ( TRUE );
	}
	else if ( CANStatus & 0x00000400 )
	{
		CAN1TFI2 = pTxBuf->Frame & 0xC00F0000;
		CAN1TID2 = pTxBuf->MsgID;
		CAN1TDA2 = pTxBuf->DataA;
		CAN1TDB2 = pTxBuf->DataB;
		CAN1CMR = 0x41;
		return ( TRUE );
	}
	else if ( CANStatus & 0x00040000 )
	{	
		CAN1TFI3 = pTxBuf->Frame & 0xC00F0000;
		CAN1TID3 = pTxBuf->MsgID;
		CAN1TDA3 = pTxBuf->DataA;
		CAN1TDB3 = pTxBuf->DataB;
		CAN1CMR = 0x81;
		return ( TRUE );
	}
	return ( FALSE );
}//dw_can1_send_message


/******************************************************************************
** Function name:		dw_can2_send_message
**
** Descriptions:		Send message block to CAN2
**
** parameters:			pointer to the CAN message
** Returned value:		if message buffer is available, and message can be sent successfully, returns TRUE,
**						otherwise, return FALSE.
** 
******************************************************************************/
DWORD dw_can2_send_message( CAN_MSG *pTxBuf )
{
	DWORD CANStatus;

	CANStatus = CAN2SR;
	if ( CANStatus & 0x00000004 )
	{
		CAN2TFI1 = pTxBuf->Frame & 0xC00F0000;
		CAN2TID1 = pTxBuf->MsgID;
		CAN2TDA1 = pTxBuf->DataA;
		CAN2TDB1 = pTxBuf->DataB;
		CAN2CMR = 0x21;
		return ( TRUE );
	}
	else if ( CANStatus & 0x00000400 )
	{
		CAN2TFI2 = pTxBuf->Frame & 0xC00F0000;
		CAN2TID2 = pTxBuf->MsgID;
		CAN2TDA2 = pTxBuf->DataA;
		CAN2TDB2 = pTxBuf->DataB;
		CAN1CMR = 0x41;
		return ( TRUE );
	}
	else if ( CANStatus & 0x00040000 )
	{	
		CAN2TFI3 = pTxBuf->Frame & 0xC00F0000;
		CAN2TID3 = pTxBuf->MsgID;
		CAN2TDA3 = pTxBuf->DataA;
		CAN2TDB3 = pTxBuf->DataB;
		CAN2CMR = 0x81;
		return ( TRUE );
	}
	return ( FALSE );
}//dw_can2_send_message

//
//
//
// execute a can test... details will follow
//
//
//
enum_can_test_retcode ui_can_test(void){
    typedef struct _TipoInfoCanTest{
        unsigned long ul_num_msg_tx;
        unsigned long ul_num_msg_tx_ok;
        unsigned long ul_num_msg_tx_ko;
        unsigned long ul_num_msg_tx_timeout;
        unsigned long ul_num_msg_rx;
        unsigned long ul_num_msg_rx_ok;
        unsigned long ul_num_msg_rx_ko;
        unsigned long ul_num_msg_rx_timeout;
        unsigned long ul_refresh_screen;
        unsigned long ul_num_err;
        unsigned long ul_num_err_prev;
    }TipoInfoCanTest;
#if ACCEPTANCE_FILTER_ENABLED
    unsigned long ul_can_data_pattern[4][2]={
        {0xa55aa5a5,~0xa55aa5a5},
        {0x36636363,~0x36636363},
        {0x12488421,~0x12488421},
        {0xF0F0F0F0,~0xF0F0F0F0},
    };
#endif
    //unsigned long i;
    TipoInfoCanTest info_can_test;

	enum_can_test_retcode retcode;
// init
	retcode=enum_can_test_retcode_ok;
    memset(&info_can_test,0,sizeof(info_can_test));
// try to init canbus...
	if (!ui_init_canbus(enum_LPC_CAN_Port_both,enum_az4186_canbus_frequency_1MHz)){
		retcode=enum_can_test_retcode_ko;
		goto end_of_procedure;
	}
	
#if !ACCEPTANCE_FILTER_ENABLED
	// Initialize MsgBuf
	MsgBuf_TX1.Frame = 0x80080000; // 29-bit, no RTR, DLC is 8 bytes
	MsgBuf_TX1.MsgID = 0x00012345; // CAN ID
	MsgBuf_TX1.DataA = 0x3C3C3C3C;
	MsgBuf_TX1.DataB = 0xC3C3C3C3;

	MsgBuf_RX2.Frame = 0x0;
	MsgBuf_RX2.MsgID = 0x0;
	MsgBuf_RX2.DataA = 0x0;
	MsgBuf_RX2.DataB = 0x0;
	CAN_set_acceptance_filter( ACCF_BYPASS );

	// Test bypass 
	while ( 1 )
	{
		// Transmit initial message on CAN 1
		while ( !(CAN1GSR & (1 << 3)) ){
            //ul_num_msg_tx_timeout
        
        }
		if ( dw_can1_send_message( &MsgBuf_TX1 ) == FALSE ){
			continue;
		}
		if ( CAN2RxDone == TRUE ){
			CAN2RxDone = FALSE;
			if ( MsgBuf_RX2.Frame &   (1 << 10) ){
				 MsgBuf_RX2.Frame &= ~(1 << 10 );
			}
			if ( 	  ( MsgBuf_TX1.Frame != MsgBuf_RX2.Frame ) 
					||( MsgBuf_TX1.MsgID != MsgBuf_RX2.MsgID ) 
					||( MsgBuf_TX1.DataA != MsgBuf_RX2.DataA ) 
					||( MsgBuf_TX1.DataB != MsgBuf_RX2.DataB ) 
				){
                retcode=enum_can_test_retcode_ko;
                goto end_of_procedure;
				// while ( 1 ){
				// }
			}
			// Everything is correct, reset buffer
			MsgBuf_RX2.Frame = 0x0;
			MsgBuf_RX2.MsgID = 0x0;
			MsgBuf_RX2.DataA = 0x0;
			MsgBuf_RX2.DataB = 0x0;
		} // Message on CAN 2 received
	}
#else
	// Test Acceptance Filter 
	// Even though the filter RAM is set for all type of identifiers, the test module tests explicit standard identifier only 
	MsgBuf_TX1.Frame = 0x00080000; // 11-bit, no RTR, DLC is 8 bytes
	MsgBuf_TX1.MsgID = EXP_STD_ID; // Explicit Standard ID
	MsgBuf_TX1.DataA = 0x55AA55AA;
	MsgBuf_TX1.DataB = 0xAA55AA55;

	MsgBuf_RX2.Frame = 0x0;
	MsgBuf_RX2.MsgID = 0x0;
	MsgBuf_RX2.DataA = 0x0;
	MsgBuf_RX2.DataB = 0x0;
	CAN_set_acceptance_filter( ACCF_ON );
    v_print_msg(0,"Executing can test...!");
    vRefreshScreen();


	while ( 1 )
	{
        if (  (++info_can_test.ul_refresh_screen>=257)
            ||(info_can_test.ul_num_err!=info_can_test.ul_num_err_prev)
            ){
            char msg[64];
            unsigned int ui_fore_color,ui_back_color;
            if (info_can_test.ul_num_err!=info_can_test.ul_num_err_prev){
                ui_fore_color=defLCD_Color_Black;
                ui_back_color=defLCD_Color_Yellow;
            }
            else{
                ui_fore_color=defLCD_Color_White;
                ui_back_color=defLCD_Color_Green;
            }
            info_can_test.ul_refresh_screen=0;
            info_can_test.ul_num_err_prev=info_can_test.ul_num_err;
            v_print_msg(0,"Executing can test...!");
            sprintf(msg,"tx:%li ok:%li KO:%li tout:%li",info_can_test.ul_num_msg_tx,info_can_test.ul_num_msg_tx_ok,info_can_test.ul_num_msg_tx_ko,info_can_test.ul_num_msg_tx_timeout);
            v_print_msg_forebackcolor(1,msg,ui_fore_color,ui_back_color);     
            sprintf(msg," %08lX %08lX %08lX %08lX",MsgBuf_TX1.Frame,MsgBuf_TX1.MsgID,MsgBuf_TX1.DataA,MsgBuf_TX1.DataB);
            v_print_msg_forebackcolor(2,msg,ui_fore_color,ui_back_color);           
            sprintf(msg,"rx:%li ok:%li KO:%li tout:%li",info_can_test.ul_num_msg_rx,info_can_test.ul_num_msg_rx_ok,info_can_test.ul_num_msg_rx_ko,info_can_test.ul_num_msg_rx_timeout);
            v_print_msg_forebackcolor(3,msg,ui_fore_color,ui_back_color);           
            sprintf(msg," %08lX %08lX %08lX %08lX",MsgBuf_RX2.Frame,MsgBuf_RX2.MsgID,MsgBuf_RX2.DataA,MsgBuf_RX2.DataB);
            v_print_msg_forebackcolor(4,msg,ui_fore_color,ui_back_color);           
            vRefreshScreen();
        }
		// Transmit initial message on CAN 1
        // wait max 100ms
        i=1000;
        while(i){
            v_wait_microseconds(100);
            if ( (CAN1GSR & (1 << 3)) ){
                break;
            }
            i--;
        }
        if (!i){
            info_can_test.ul_num_msg_tx_timeout++;
            info_can_test.ul_num_err++;
            continue;
        }
        // set new values for A and B data
        MsgBuf_TX1.DataA=ul_can_data_pattern[info_can_test.ul_num_msg_tx&3][0];
        MsgBuf_TX1.DataB=ul_can_data_pattern[info_can_test.ul_num_msg_tx&3][1];
    
        info_can_test.ul_num_msg_tx++;
		if ( dw_can1_send_message( &MsgBuf_TX1 ) == FALSE ){
            info_can_test.ul_num_msg_tx_ko++;
            info_can_test.ul_num_err++;
			continue;
		}
        info_can_test.ul_num_msg_tx_ok++;
    // wait max 100ms
        i=1000;
        while(i){
            v_wait_microseconds(100);
            if ( CAN2RxDone == TRUE ){
                break;
            }
            i--;
        }
        if (!i){
            info_can_test.ul_num_msg_rx_timeout++;
            info_can_test.ul_num_err++;
            continue;
        }
    
		// please note: FULLCAN identifier will NOT be received as it's not set in the acceptance filter. 
        info_can_test.ul_num_msg_rx++;

        CAN2RxDone = FALSE;
        // The frame field is not checked, as ID index varies based on the	entries set in the filter RAM. 
        if (  ( MsgBuf_TX1.MsgID != MsgBuf_RX2.MsgID ) 
            ||( MsgBuf_TX1.DataA != MsgBuf_RX2.DataA ) 
            ||( MsgBuf_TX1.DataB != MsgBuf_RX2.DataB ) 
            ){
            info_can_test.ul_num_msg_tx_ko++;
            info_can_test.ul_num_err++;
            if (info_can_test.ul_num_msg_tx_ko>100){
                retcode=enum_can_test_retcode_ko;
                goto end_of_procedure;
            }
            // while ( 1 ){
            // }
        }
        else{
            // Everything is correct, reset buffer? no, leave untouched
            // MsgBuf_RX2.Frame = 0x0;
            // MsgBuf_RX2.MsgID = 0x0;
            // MsgBuf_RX2.DataA = 0x0;
            // MsgBuf_RX2.DataB = 0x0;
            info_can_test.ul_num_msg_rx_ok++;
        }
	}
#endif	
	
end_of_procedure:	
	return retcode;
}


#ifdef def_enable_canopen_test
    typedef struct _TipoStruct_canopen_timer0{
        unsigned long ul_period_ms;
        unsigned long ul_clock;
        unsigned long vicenable;
    }TipoStruct_canopen_timer0;

    volatile TipoStruct_canopen_timer0 canopen_timer0;

    CO_t *CO;        //pointer to CANopen object

    /* CAN callback function ******************************************************/
    int CAN1callback(enum_LPC_event event, const CO_CANrxMsg_t *msg){
       return CO_CANinterrupt(CO->CANmodule[0], event, msg);
    }

    int CAN2callback(enum_LPC_event event, const CO_CANrxMsg_t *msg){
       return CO_CANinterrupt(CO->CANmodule[1], event, msg);
    }

static void v_canopen_wait_ms(unsigned long ul_ms_to_wait){
        #ifdef def_canopen_enable
        TipoStruct_timeout timeout;
        timeout_init(&timeout);
        while(1){
            if (uc_timeout_active(&timeout, ul_ms_to_wait)){
                break;
            }
        }
        #else
            unsigned long ul_clock;
            unsigned long ul_num_loop_to_wait;
            ul_num_loop_to_wait=ul_ms_to_wait/canopen_timer0.ul_period_ms;
            if (ul_num_loop_to_wait==0)
                ul_num_loop_to_wait=1;
            while(ul_num_loop_to_wait--){
                // wait at least one timer tick
                ul_clock=canopen_timer0.ul_clock;
                while(canopen_timer0.ul_clock==ul_clock){};
            }
        #endif
    }


    // define rtcode from canopen write parameter routines
    typedef enum{
        enum_canopen_write_parameter_retcode_ok=0,
        enum_canopen_write_parameter_retcode_bad_node_id,
        enum_canopen_write_parameter_retcode_ko_initiate,
        enum_canopen_write_parameter_retcode_ko_download,
        enum_canopen_write_parameter_retcode_ko_abort,
        enum_canopen_write_parameter_retcode_wait,
        enum_canopen_write_parameter_retcode_numof
    }enum_canopen_write_parameter_retcode;
    // the structure to fill to write a canopen parameter
    typedef struct _TipoStructWriteCanopenParameter{
        unsigned int ui_sdo_server_node_id;     // write destination node id
        unsigned int ui_idx;                    // parameter index
        unsigned int ui_subidx;                 // parameter sub_index
        unsigned int num_of_bytes_to_send;      // num of bytes to send
        unsigned char _data[8];                 // array containing the bytes to send
    }TipoStructWriteCanopenParameter;
    // here all of the info about results of parameter writing
    typedef struct _TipoStructWriteCanopenParameterResult{
        UNSIGNED32 SDOabortCode; // abort code
    }TipoStructWriteCanopenParameterResult;
    // define the variables
    TipoStructWriteCanopenParameter s_write_canopen_parameter;
    TipoStructWriteCanopenParameterResult s_write_canopen_parameter_result;
	TipoStruct_timeout timeout_write_canopen_parameter;


    // write a canopen parameter, using the client access to server dictionary
    enum_canopen_write_parameter_retcode ui_write_canopen_parameter(TipoStructWriteCanopenParameter *ps){
        enum_canopen_write_parameter_retcode retcode;
        //int download_retcode;
    // init
        // retcode --> ok
        retcode=enum_canopen_write_parameter_retcode_ok;
        // reset abort code
        s_write_canopen_parameter_result.SDOabortCode = 0;
        // initialize sdo client
        CO_SDOclient_t *SDO_C = CO->SDOclient;
        // setup sdo client, autoinitialize fields using node id info
        if (CO_SDOclient_setup(SDO_C, 0, 0, ps->ui_sdo_server_node_id)!=0){
            retcode=enum_canopen_write_parameter_retcode_bad_node_id;
            goto end_of_procedure;
        }
        // starts parameter download
        if (CO_SDOclientDownloadInitiate(SDO_C, ps->ui_idx, ps->ui_subidx, ps->_data, ps->num_of_bytes_to_send)!=0){
            // error!
            retcode=enum_canopen_write_parameter_retcode_ko_initiate;
            goto end_of_procedure;
        }
        timeout_init(&timeout_write_canopen_parameter);
		
#if 0		
        #define def_ms_between_check 10
        #define def_timeout_write_parameter_ms 500
        // wait end of parameter download
        do{
            v_canopen_wait_ms(def_ms_between_check);
            download_retcode = CO_SDOclientDownload(SDO_C, def_ms_between_check, def_timeout_write_parameter_ms,  &s_write_canopen_parameter_result.SDOabortCode);
        }while(download_retcode > 0);
        // check if error on parameter download
        if (download_retcode<0){
            // error!
            retcode=enum_canopen_write_parameter_retcode_ko_download;
            goto end_of_procedure;
        }
    //disable sdo client
        CO_SDOclient_setup(SDO_C, 0, 0, 0);
#endif		
    end_of_procedure:
        return retcode;
    }
	
	
	enum_canopen_write_parameter_retcode ui_check_end_of_parameter_write(void){
        #define def_ms_between_check 10
        #define def_timeout_write_parameter_ms 1500
        enum_canopen_write_parameter_retcode retcode;
        int download_retcode;
    // init
        // retcode --> ok
        retcode=enum_canopen_write_parameter_retcode_ok;
		// a minimum time should be elapsed before check
		if (!uc_timeout_active(&timeout_write_canopen_parameter, def_ms_between_check)){
			return enum_canopen_write_parameter_retcode_wait;
		}
        timeout_init(&timeout_write_canopen_parameter);
		
        // reset abort code
        s_write_canopen_parameter_result.SDOabortCode = 0;
        // initialize sdo client
        CO_SDOclient_t *SDO_C = CO->SDOclient;
        // wait end of parameter download
		download_retcode = CO_SDOclientDownload(SDO_C, def_ms_between_check, def_timeout_write_parameter_ms,  &s_write_canopen_parameter_result.SDOabortCode);
        if (download_retcode>0){
			return enum_canopen_write_parameter_retcode_wait;
		}
        // check if error on parameter download
        if (download_retcode<0){
            // error!
            retcode=enum_canopen_write_parameter_retcode_ko_download;
        }
    //disable sdo client
        CO_SDOclient_setup(SDO_C, 0, 0, 0);
        return retcode;
	}
	
	
    // force analog input refresh from phoenix il-can-bk-tc
    enum_canopen_write_parameter_retcode ui_set_canopen_analog_input_refresh_period_ms(unsigned int ui_refresh_period_ms,unsigned int ui_sdo_server_node_id){
        // all'indirizzo 0x1801 si trova il tpdo2 communication parameter della centralina phoenix IL-CAN-BK-TC
        // dove si possono impostare le propriet� del tpdo dei primi due ingressi analogici collegati alla centralina
        // al sottoindice 5 si pu� specificare l'event timer, cio� ogni quanti ms la centralina deve fornire in uscita il dato analogico
        // per default l'event timer � 0 e perci� il dato non viene fornito in uscita...
        // il valore rappresenta il periodo di rinfresco in millisecondi e deve essere specificato come unsigned16, cio� a due bytes

        // set parameters write fields
        s_write_canopen_parameter.ui_sdo_server_node_id=ui_sdo_server_node_id;
        s_write_canopen_parameter.ui_idx=0x1801;
        s_write_canopen_parameter.ui_subidx=5;
        s_write_canopen_parameter.num_of_bytes_to_send=2;
        s_write_canopen_parameter._data[0]=ui_refresh_period_ms&0xff;         //every xxx ms a new analog value is sent, low byte
        s_write_canopen_parameter._data[1]=(ui_refresh_period_ms>>8)&0xff;    //every xxx ms a new analog value is sent, high byte
        return ui_write_canopen_parameter(&s_write_canopen_parameter);
    }//enum_canopen_write_parameter_retcode ui_set_canopen_analog_input_refresh_period_ms(unsigned int ui_refresh_period_ms)

    enum_canopen_write_parameter_retcode ui_set_canopen_analog_input_type(unsigned int ui_range,unsigned int ui_sdo_server_node_id){
// Send 8011hex to Analog Input Range Object, Index 2400hex, Subindex xxx using a
// Service Data Object (SDO) message. This message can be sent using a configuration
// tool such as the CANopen Configuration Studio�. Once the range is configured for a
// new value, the bus coupler will retain that sending in non-volatile flash memory.
        // set parameters write fields
        s_write_canopen_parameter.ui_sdo_server_node_id=ui_sdo_server_node_id;
        s_write_canopen_parameter.ui_idx=0x2400;
        s_write_canopen_parameter.ui_subidx=1; // analog input idx: 1 is the first one
        s_write_canopen_parameter.num_of_bytes_to_send=2;
        s_write_canopen_parameter._data[0]=ui_range&0xff;         //update analog input configuration
        s_write_canopen_parameter._data[1]=(ui_range>>8)&0xff;    //every xxx ms a new analog value is sent, high byte
        return ui_write_canopen_parameter(&s_write_canopen_parameter);
    }
    enum_canopen_write_parameter_retcode ui_set_canopen_analog_input_type_2(unsigned int ui_range,unsigned int ui_sdo_server_node_id){
// Send 8011hex to Analog Input Range Object, Index 2400hex, Subindex xxx using a
// Service Data Object (SDO) message. This message can be sent using a configuration
// tool such as the CANopen Configuration Studio�. Once the range is configured for a
// new value, the bus coupler will retain that sending in non-volatile flash memory.
        // set parameters write fields
        s_write_canopen_parameter.ui_sdo_server_node_id=ui_sdo_server_node_id;
        s_write_canopen_parameter.ui_idx=0x2400;
        s_write_canopen_parameter.ui_subidx=2; // analog input idx: 1 is the first one
        s_write_canopen_parameter.num_of_bytes_to_send=2;
        s_write_canopen_parameter._data[0]=ui_range&0xff;         //update analog input configuration
        s_write_canopen_parameter._data[1]=(ui_range>>8)&0xff;    //every xxx ms a new analog value is sent, high byte
        return ui_write_canopen_parameter(&s_write_canopen_parameter);
    }

    void v_request_data(void){
        CO_CANtxArray_t *CAN_txBuff;
           CAN_txBuff = CO_CANtxBufferInit(       //return pointer to 8-byte CAN data buffer, which should be populated
                                 CO->CANmodule[0], //pointer to CAN module used for sending this message
                                 0,                //index of specific buffer inside CAN module (CO_TXCAN_NMT has value of 0)
                                 0x80,            //0x281->request pdo2
                                 0,                //rtr
                                 0,               //number of data bytes
                                 0);               //synchronous message flag bit

           CO_CANsend(CO->CANmodule[0], CAN_txBuff); //0 = success
    }

    void v_canopen_nodes_operational(void){
        CO_CANtxArray_t *CAN_txBuff;


// Initialization

// Sample trace of communications between a master and two pressure transducer slaves configured for id 1 and node ID 2.
// CAN ID 	DATA LENGTH 	DATA 	Description
// 0x0 	2 	01 00 	Master puts the bus into operational mode
// 0x80 	0 		Master sends a SYNC message, which triggers devices to send data
// 0x181 	4 	CD 82 01 00 	Node at ID 1 (CID-0x180), reading pressure of 0x0182CD (99021) pascals
// 0x182 	4 	E5 83 01 00 	Node at ID 2 (CID-0x181), reading pressure of 0x0183E5 (99301) pascals    
    
    
       CAN_txBuff = CO_CANtxBufferInit(       //return pointer to 8-byte CAN data buffer, which should be populated
                             CO->CANmodule[0], //pointer to CAN module used for sending this message
                             0,                //index of specific buffer inside CAN module (CO_TXCAN_NMT has value of 0)
                             0x00,            //CAN identifier of the master of the bus -->0!
                             0,                //rtr
                             2,          //number of data bytes
                             0);               //synchronous message flag bit

       CAN_txBuff->_data[0] =CO_NMT_ENTER_OPERATIONAL;
       CAN_txBuff->_data[1] =0; //all nodes operational
       CO_CANsend(CO->CANmodule[0], CAN_txBuff); //0 = success
// now request data from modules (send a sync, just to try      
       CAN_txBuff = CO_CANtxBufferInit(       //return pointer to 8-byte CAN data buffer, which should be populated
                             CO->CANmodule[0], //pointer to CAN module used for sending this message
                             0,                //index of specific buffer inside CAN module (CO_TXCAN_NMT has value of 0)
                             0x80,            //CAN identifier of the master of the bus -->0!
                             0,                //rtr
                             0,          //number of data bytes
                             0);               //synchronous message flag bit

       CO_CANsend(CO->CANmodule[0], CAN_txBuff); //0 = success
    }

    static void IRQ_Handler_canopen_timer0(void){
        // watch dog feed
        WDFEED=0XAA;
        WDFEED=0X55;
        canopen_timer0.ul_clock++;
        canopen_timer0.vicenable=VICIntEnable;

        CO_process_RPDO(CO);


        CO_process_TPDO(CO);

        // clear interrupt flag
        T0IR = 0x1;         
    }// IRQ_Handler_timer0

    // inizializzazione timer 0--> interrupt every ui_interrupt_period_ms ms
    void v_canopen_init_timer0(unsigned int ui_interrupt_period_ms){


        T0TCR = 2; // reset counters
        T0TCR = 0; // disable timer

        T0PR = 100;
        canopen_timer0.ul_period_ms=ui_interrupt_period_ms;
        //T0MR0 = 1000;
        // uso un peripheral clock source pari a Fpclk/2... quindi devo mettere un fattore *2
        T0MR0 = 2*((ui_interrupt_period_ms) * (Fpclk /(100*1000)))-1;
        T0MCR = 0x3;
      
        VICIntSelect &= ~(0x10); // Timer 0 selected as IRQ
        VICVectPriority4 = 1; // set priority for timer 0 to be quite high
        VICVectAddr4 = (unsigned long)IRQ_Handler_canopen_timer0;

        T0TCR = 1;
        VICIntEnable |= 0x10; // Timer 0 interrupt enabled

    }
    unsigned long ul_timer_difference_init(unsigned long *pul_prev_clock){
        *pul_prev_clock=canopen_timer0.ul_clock;
        return 0;
    }
    unsigned long ul_timer_difference_update_ms(unsigned long *pul_prev_clock){
        unsigned long ul_act_clock;
        unsigned long ul_time_elapsed_ms;
        
        ul_act_clock=canopen_timer0.ul_clock;
        if (*pul_prev_clock>ul_act_clock){
            ul_time_elapsed_ms=0;
            *pul_prev_clock=ul_act_clock;
        }
        else{
            ul_time_elapsed_ms=ul_act_clock-*pul_prev_clock;
        }
        return ul_time_elapsed_ms;
    }
    unsigned int ui_get_canopen_temperature(int *pi_temperature_value_binary){
        short int i_16;
        unsigned int ui_temperature_valid;
		extern unsigned int ui_temperature_alarm_subcode;
        i_16=CO_OD_RAM.readAnalogueInput16Bit[1];
        ui_temperature_valid=0;
        switch(*(unsigned short int*)&i_16){
            case 0x8002:
                //sprintf(c_buffer,"ai0: open circuit");
				ui_temperature_alarm_subcode=enumStr20_FaultTemp_ProbeOpen;
                break;
            case 0x8001:
                //sprintf(c_buffer,"ai0: overrange");
				ui_temperature_alarm_subcode=enumStr20_FaultTemp_ProbeOverrange;
                break;
            case 0x8080:
                //sprintf(c_buffer,"ai0: underrange");
				ui_temperature_alarm_subcode=enumStr20_FaultTemp_ProbeUnderrange;
                break;
            default:
                ui_temperature_valid=1;
				//ui_temperature_alarm_subcode=0;
                break;
        }
        *pi_temperature_value_binary=i_16;
        return ui_temperature_valid;
    }
	// returns the temperature of the cold junction (analog input 0, first pdi word)
    unsigned int ui_get_canopen_temperature_cold_junction(int *pi_temperature_value_binary){
        short int i_16;
        unsigned int ui_temperature_valid;
		extern unsigned int ui_temperature_alarm_subcode;
        i_16=CO_OD_RAM.readAnalogueInput16Bit[0];
        ui_temperature_valid=0;
        switch(*(unsigned short int*)&i_16){
            case 0x8002:
                //sprintf(c_buffer,"ai0: open circuit");
                break;
            case 0x8001:
                //sprintf(c_buffer,"ai0: overrange");
                break;
            case 0x8080:
                //sprintf(c_buffer,"ai0: underrange");
                break;
            default:
                ui_temperature_valid=1;
				//ui_temperature_alarm_subcode=0;
                break;
        }
        *pi_temperature_value_binary=i_16;
        return ui_temperature_valid;
    }
	// return a value which is incremented at each sucessfully received rpdo canbus frame
	unsigned long ul_get_canbus_living_counter(void){
		extern unsigned long ul_num_of_rpdo_rx;
		return ul_num_of_rpdo_rx;
	}

    void v_update_screen(void){
        // only for diagnostic purposes...
        extern void v_print_msg(unsigned int row,char *s);
        extern void vRefreshScreen(void);
        char c_buffer[64];
        float f_temperature;
        {
            unsigned char *p;
            extern unsigned long ul_num_of_rpdo_rx;
            unsigned short int ui_16;
        
        
            // ui_16=CO_OD_RAM.writeOutput8Bit[3];     
            // ui_16<<=8;
            // ui_16|=CO_OD_RAM.writeOutput8Bit[2]; 
        
            ui_16=CO_OD_RAM.readAnalogueInput16Bit[1];
        
            switch(ui_16){
                case 0x8002:
                    sprintf(c_buffer,"ai0: open circuit");
                    break;
                case 0x8001:
                    sprintf(c_buffer,"ai0: overrange");
                    break;
                case 0x8080:
                    sprintf(c_buffer,"ai0: underrange");
                    break;
                default:
                    f_temperature=(ui_16);
                    f_temperature*=0.1;
                    sprintf(c_buffer,"ai0 temp.:%-6.2f deg Celsius",f_temperature);
                    break;
            }
             v_print_msg(0,c_buffer);
        

            // ui_16=CO_OD_RAM.writeOutput8Bit[1];     
            // ui_16<<=8;
            // ui_16|=CO_OD_RAM.writeOutput8Bit[0];     
            ui_16=CO_OD_RAM.readAnalogueInput16Bit[0];
        
            switch(ui_16){
                case 0x8002:
                    sprintf(c_buffer,"ai1: open circuit");
                    break;
                case 0x8001:
                    sprintf(c_buffer,"ai1: overrange");
                    break;
                case 0x8080:
                    sprintf(c_buffer,"ai1: underrange");
                    break;
                default:
                    f_temperature=(ui_16);
                    f_temperature*=0.1;
                    sprintf(c_buffer,"ai1 temp.:%-6.2f deg Celsius",f_temperature);
                    break;
            }
            v_print_msg(1,c_buffer);
            p=&CO->RPDO[0]->CANrxData[0];
            sprintf(c_buffer,"RPDO0.: %02X %02X %02X %02X"
                                ,(unsigned int)p[0],(unsigned int)p[1],(unsigned int)p[2],(unsigned int)p[3]
                   );
            v_print_msg(2,c_buffer);
        

            f_temperature=(MsgBuf_RX1.DataA>>16);
            f_temperature*=0.1;
            sprintf(c_buffer,"Raw rx temp.:%-6.2f deg Celsius (%X)",f_temperature,(MsgBuf_RX1.DataA>>16));
            v_print_msg(3,c_buffer);
        
            sprintf(c_buffer,"Num rx: %lu",ul_num_of_rpdo_rx);
            v_print_msg(4,c_buffer);
            sprintf(c_buffer,"co_od_ram_ai0*: %04X",(unsigned int)(CO_OD_RAM.readAnalogueInput16Bit[0]));
            v_print_msg(5,c_buffer);
            sprintf(c_buffer,"co_od_ram_ai1*: %04X",(unsigned int)(CO_OD_RAM.readAnalogueInput16Bit[1]));
            v_print_msg(6,c_buffer);
        }
        vRefreshScreen();            
    }

    // address as from canopen dipswitch address
    #define def_canopen_phoenix_dipswitch_address 1

    // how many ms should I wait before going to operational state?
    #define def_wait_before_operational_ms 1000
    // how many ms to wait after operational state before setting parameters?
    #define def_wait_before_set_params_ms 10000
    // canopen initialization sequence stati
    typedef enum{
        enum_canopen_initialization_sequence_idle=0,
        enum_canopen_initialization_sequence_wait_before_set_params,
        enum_canopen_initialization_sequence_set_params,
        enum_canopen_initialization_sequence_wait_param_0,
        enum_canopen_initialization_sequence_wait_param_1,
        enum_canopen_initialization_sequence_wait_param_2,
        enum_canopen_initialization_sequence_wait_before_operational,
        enum_canopen_initialization_sequence_operational_mode,
        enum_canopen_initialization_sequence_ends,
        enum_canopen_initialization_sequence_numof
    }enum_canopen_initialization_sequence;
    // structure to handle canopen initialization
    typedef struct _TipoStructHandleCanopenInitSeq{
        enum_canopen_initialization_sequence status;
        unsigned long ul_clock;
        #ifdef def_canopen_enable
            TipoStruct_timeout timeout;
        #endif
    
    }TipoStructHandleCanopenInitSeq;
    // define the variable to handle canopen initialization
    TipoStructHandleCanopenInitSeq canopen_init_seq;

    // initialize the structure needed to handle the canopen init sequence
    void v_init_canopen_init_seq(void){
        memset(&canopen_init_seq,0,sizeof(canopen_init_seq));
    }
    // handle canopen init sequence
    void v_handle_canopen_init_seq(void){
        switch(canopen_init_seq.status){
            case enum_canopen_initialization_sequence_idle:
            default:
                #ifdef def_canopen_enable
                    timeout_init(&canopen_init_seq.timeout);
                #else
                    ul_timer_difference_init(&canopen_init_seq.ul_clock);
                #endif
                canopen_init_seq.status=enum_canopen_initialization_sequence_wait_before_set_params;
                break;
            case enum_canopen_initialization_sequence_wait_before_set_params:
            #ifdef def_canopen_enable
                if (uc_timeout_active(&canopen_init_seq.timeout, def_wait_before_set_params_ms)){
            #else
                if (ul_timer_difference_update_ms(&canopen_init_seq.ul_clock)>def_wait_before_set_params_ms){
            #endif
                    canopen_init_seq.status=enum_canopen_initialization_sequence_set_params;
                    break;
                }
                break;
            case enum_canopen_initialization_sequence_set_params:
                ui_set_canopen_analog_input_refresh_period_ms(def_canbus_sampling_period_ms,def_canopen_phoenix_dipswitch_address);
				canopen_init_seq.status=enum_canopen_initialization_sequence_wait_param_0;
				break;
			case enum_canopen_initialization_sequence_wait_param_0:
				if (ui_check_end_of_parameter_write()==enum_canopen_write_parameter_retcode_wait){
					break;
				}
                // 0x8000 -> request configuration
            
                //    0 0000 TC type K
                //    1 0001 TC type J
                //    2 0010 TC type E
                //    3 0011 TC type R
                //    4 0100 TC type S
                //    5 0101 TC type T
                //    6 0110 TC type B
                //    7 0111 TC type N
                //    8 1000 TC type U
                //    9 1001 TC type L
                //    10 1010 TC type C
                //    11 1011 TC type W
                //    12 1100 TC type HK
                //    13 1101 Cold junction (CJ)
                //    14 1110 U: Voltage (-15 mV ... 15 1111 Reserved
                // format 1, 0.1�C, internal cold junction
				// bit 10..8: 
				//	000 -->internal cold junction active
				//	001 -->internal cold junction inactive
                ui_set_canopen_analog_input_type(0x8000|0x0d|(0<<8),def_canopen_phoenix_dipswitch_address);
				canopen_init_seq.status=enum_canopen_initialization_sequence_wait_param_1;
				break;
			case enum_canopen_initialization_sequence_wait_param_1:
				if (ui_check_end_of_parameter_write()==enum_canopen_write_parameter_retcode_wait){
					break;
				}
				// per configurare il primo analog input channel , si deve impostare la seconda pdo e viceversa...
				// impostando 1<<8 provo a disabilitare la compensazione del giunto freddo per verificare la lettura tramite simulatore di termocoppia
                ui_set_canopen_analog_input_type_2(0x8000|0x01|(0<<8),def_canopen_phoenix_dipswitch_address);
                //#warning set T type thermocouple
                //ui_set_canopen_analog_input_type_2(0x8000|enum_can_tc_type_T|(0<<8),def_canopen_phoenix_dipswitch_address);
				canopen_init_seq.status=enum_canopen_initialization_sequence_wait_param_2;
				break;
			case enum_canopen_initialization_sequence_wait_param_2:
				if (ui_check_end_of_parameter_write()==enum_canopen_write_parameter_retcode_wait){
					break;
				}
                canopen_init_seq.status=enum_canopen_initialization_sequence_wait_before_operational;
                #ifdef def_canopen_enable
                    timeout_init(&canopen_init_seq.timeout);
                #endif
                break;
            case enum_canopen_initialization_sequence_wait_before_operational:
            #ifdef def_canopen_enable
                if (uc_timeout_active(&canopen_init_seq.timeout, def_wait_before_operational_ms)){
            #else                
                if (ul_timer_difference_update_ms(&canopen_init_seq.ul_clock)>def_wait_before_operational_ms){
            #endif
                    canopen_init_seq.status=enum_canopen_initialization_sequence_operational_mode;
                    break;
                }
                break;
            case enum_canopen_initialization_sequence_operational_mode:
                v_canopen_nodes_operational();
                ul_timer_difference_init(&canopen_init_seq.ul_clock);
                canopen_init_seq.status=enum_canopen_initialization_sequence_ends;
                break;
            case enum_canopen_initialization_sequence_ends:
                break;
        }
    }
    // retcodes from canopen_init routine
    typedef enum{
        enum_canopen_init_retcode_ok=0,
        enum_canopen_init_retcode_ko,
        enum_canopen_init_retcode_numof
    }enum_canopen_init_retcode;

    //canopen_init
    enum_canopen_init_retcode canopen_init(void){
        enum_canopen_init_retcode retcode;
    // init
        retcode=enum_canopen_init_retcode_ok;
        CO=NULL;
    // goto configuration mode
        CO_CANsetConfigurationMode(ADDR_CAN1);
    //initialize CANopen
        if (CO_init(&CO)){
            retcode=enum_canopen_init_retcode_ko;
            goto end_of_procedure;
        }
    //start CAN
        CO_CANsetNormalMode(ADDR_CAN1);
    end_of_procedure: 
        return retcode;
    }
    UNSIGNED32 CO_ODF_1010( void       *object,
                        UNSIGNED16  index,
                        UNSIGNED8   subIndex,
                        UNSIGNED16 *pLength,
                        UNSIGNED16  attribute,
                        UNSIGNED8   dir,
                        void       *dataBuff,
                        const void *pData)
    {
        return 0;
    }
    UNSIGNED32 CO_ODF_1011( void       *object,
                        UNSIGNED16  index,
                        UNSIGNED8   subIndex,
                        UNSIGNED16 *pLength,
                        UNSIGNED16  attribute,
                        UNSIGNED8   dir,
                        void       *dataBuff,
                        const void *pData)
    {
        return 0;
    }
    UNSIGNED32 ODF_testDomain( void       *object,
                        UNSIGNED16  index,
                        UNSIGNED8   subIndex,
                        UNSIGNED16 *pLength,
                        UNSIGNED16  attribute,
                        UNSIGNED8   dir,
                        void       *dataBuff,
                        const void *pData)
    {
        return 0;
    }


    // returns 1 if canopen init is ok, else 0
    unsigned int ui_canopen_init(void){
        unsigned int retcode;
        // initialization
        retcode=1;
        // init canopen initialization sequence
        v_init_canopen_init_seq();    
        // open the canopen
        if (canopen_init()!=enum_canopen_init_retcode_ok){
            retcode=0;
        }
        return retcode;
    }
    // returns 1 if canopen process runs ok, else 0
    unsigned int ui_canopen_process(unsigned long ul_time_elapsed_from_lastcall_ms){
        unsigned int retcode;
        // initialization
        retcode=1;
        // handle canopen init sequence
        v_handle_canopen_init_seq();
        // CANopen process
        if (CO_process(CO, ul_time_elapsed_from_lastcall_ms)){
            retcode=0;
        }
        return retcode;
    }
    void v_process_canopen_rpdo(void)
    {
        CO_process_RPDO(CO);
    }

    void v_process_canopen_tpdo(void)
    {
        CO_process_TPDO(CO);
    }


    // canopen test...
    unsigned int ui_canopen_test(void){
        //unsigned int err;
        unsigned int retcode;
        unsigned int ui_end_of_canopen_loop;
        unsigned long ul_clock;
    // initialization
        retcode=0;
        // init canopen initialization sequence
        v_init_canopen_init_seq();    
        // open the canopen
        if (canopen_init()!=enum_canopen_init_retcode_ok){
            retcode=-1;
            goto end_of_procedure;
        }
        // initialize timer to 1ms
        v_canopen_init_timer0(1);
        // set some variables..
        ui_end_of_canopen_loop=0;
        // base time...
        ul_timer_difference_init(&ul_clock);
        //canopen process loop
        while(!ui_end_of_canopen_loop){
            // handle canopen init sequence
            v_handle_canopen_init_seq();
            // CANopen process
            ui_end_of_canopen_loop = CO_process(CO, ul_timer_difference_update_ms(&ul_clock));
            // if not error, wait some ms
            if (!ui_end_of_canopen_loop){
                v_canopen_wait_ms(50);
                v_update_screen();
            }
        }
    end_of_procedure:
        return retcode;

    }//unsigned int ui_canopen_test(void)
#endif
