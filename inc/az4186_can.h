#ifndef def_az4186_can_h_included
#define def_az4186_can_h_included
/*****************************************************************************
 *   can.h:  Header file for NXP LPC230x Family Microprocessors
 *
 *   Copyright(C) 2006, NXP Semiconductor
 *   All rights reserved.
 *
 *   History
 *   2006.09.20  ver 1.00    Preliminary version, first Release
 *
******************************************************************************/ 

#define ACCEPTANCE_FILTER_ENABLED	0

#define CAN_MEM_BASE		0xE0038000

#define MAX_PORTS	2		/* Number of CAN port on the chip */	

// can bus sampling period [ms]
#define def_canbus_sampling_period_ms 50	


/**
 * Bit Timing Values for 16MHz clk frequency
 */
#define BITRATE100K16MHZ          0x001C0009
#define BITRATE125K16MHZ          0x001C0007
#define BITRATE250K16MHZ          0x001C0003
#define BITRATE500K16MHZ          0x001C0001
#define BITRATE1000K16MHZ         0x001C0000

/*
 * Bit Timing Values for 24MHz clk frequency
 */
#define BITRATE100K24MHZ          0x001C000E
#define BITRATE125K24MHZ          0x001C000B
#define BITRATE250K24MHZ          0x001C0005
#define BITRATE500K24MHZ          0x001C0002
#define BITRATE1000K24MHZ         0x00090001

/*
 * Bit Timing Values for 48MHz clk frequency
 */
#define BITRATE100K48MHZ          0x001C001D
#define BITRATE125K48MHZ          0x001C0017
#define BITRATE250K48MHZ          0x001C000B
#define BITRATE500K48MHZ          0x001C0005
#define BITRATE1000K48MHZ         0x001C0002

/*
 * Bit Timing Values for 60MHz clk frequency
 */
#define BITRATE100K60MHZ          0x00090031
#define BITRATE125K60MHZ          0x00090027
#define BITRATE250K60MHZ          0x00090013
#define BITRATE500K60MHZ          0x00090009
#define BITRATE1000K60MHZ         0x00090004

/*
 * Bit Timing Values for 28.8MHz pclk frequency, 1/2 of 576.Mhz CCLK
 */
#define BITRATE100K28_8MHZ        0x00090017

/* When Fcclk is 50Mhz and 60Mhz and APBDIV is 4,
so Fpclk is 12.5Mhz and 15Mhz respectively. 
when Fpclk is 12.5Mhz, QUANTA is 10 and sample point is 90% 
when Fpclk is 15Mhz, QUANTA is 10 and sample point is 90% */

/* Common CAN bit rates for 12.5Mhz(50Mhz CCLK) clock frequency */
#define BITRATE125K12_5MHZ		0x00070009
#define BITRATE250K12_5MHZ		0x00070004

/**
 * Bit Timing Values for 15MHz(60Mhz CCLK) clk frequency
 */
#define BITRATE100K15MHZ		0x0007000E
#define BITRATE125K15MHZ		0x0007000B
#define BITRATE250K15MHZ		0x00070005
#define BITRATE500K15MHZ		0x00070002

/* Acceptance filter mode in AFMR register */
#define ACCF_OFF				0x01
#define ACCF_BYPASS				0x02
#define ACCF_ON					0x00
#define ACCF_FULLCAN			0x04

/* This number applies to all FULLCAN IDs, explicit STD IDs, group STD IDs, 
explicit EXT IDs, and group EXT IDs. */ 
#define ACCF_IDEN_NUM			4

/* Identifiers for FULLCAN, EXP STD, GRP STD, EXP EXT, GRP EXT */
#define FULLCAN_ID				0x100
#define EXP_STD_ID				0x100
#define GRP_STD_ID				0x200
#define EXP_EXT_ID				0x100000
#define GRP_EXT_ID				0x200000

// Type definition to hold a CAN message
typedef struct
{
	DWORD Frame; 	// Bits 16..19: DLC - Data Length Counter
					// Bit 30: Set if this is a RTR message
					// Bit 31: Set if this is a 29-bit ID message
	DWORD MsgID;	// CAN Message ID (11-bit or 29-bit)
	DWORD DataA;	// CAN Message Data Bytes 0-3
	DWORD DataB;	// CAN Message Data Bytes 4-7
} CAN_MSG;


typedef enum{
	enum_can_test_retcode_ok=0,
	enum_can_test_retcode_ko,
	enum_can_test_retcode_numof
}enum_can_test_retcode;

//
//
//
// execute a can test... details will follow
//
//
//
enum_can_test_retcode ui_can_test(void);


/**************************************************************************
PUBLIC FUNCTIONS
***************************************************************************/
typedef enum{
	enum_az4186_canbus_frequency_1MHz=0,
	enum_az4186_canbus_frequency_500kHz,
	enum_az4186_canbus_frequency_250kHz,
	enum_az4186_canbus_frequency_125kHz,
	enum_az4186_canbus_frequency_numof
}enum_az4186_canbus_frequency;

typedef enum{
   enum_LPC_CAN_Port_1=0,
   enum_LPC_CAN_Port_2,
   enum_LPC_CAN_Port_both,
   enum_LPC_CAN_Port_numof=2
}enum_LPC_CAN_Ports;

typedef enum{
    enum_LPC_event_tx_empty=0,
    enum_LPC_event_rx_full,
    enum_LPC_event_numof
}enum_LPC_event;

extern CAN_MSG MsgBuf_RX1, MsgBuf_RX2;
extern CAN_MSG MsgBuf_TX1, MsgBuf_TX2; // TX Buffers for CAN message
extern volatile DWORD CAN1RxDone, CAN2RxDone;


// initialize and sets the canbus frequency needed
// returns 0 if the frequency can't be set or initialization fails
// return 1 if the frequency has been set ok and initialization is ok
unsigned int ui_init_canbus(enum_LPC_CAN_Ports can_ports,enum_az4186_canbus_frequency az4186_canbus_frequency);

void CAN_set_acceptance_filter_lookup( void );
void CAN_set_acceptance_filter( DWORD ACCFMode );

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
DWORD dw_can1_send_message( CAN_MSG *pTxBuf );
DWORD dw_can2_send_message( CAN_MSG *pTxBuf );
//DWORD CAN2_SendMessage( CAN_MSG* pTXBuf );

#define def_enable_canopen_test

#ifdef def_enable_canopen_test
    unsigned int ui_canopen_test(void);
#endif    


// returns 1 if canopen init is ok, else 0
unsigned int ui_canopen_init(void);
// returns 1 if canopen process runs ok, else 0
unsigned int ui_canopen_process(unsigned long ul_time_elapsed_from_lastcall_ms);

void v_process_canopen_rpdo(void);

void v_process_canopen_tpdo(void);
unsigned int ui_get_canopen_temperature(int *pi_temperature_value_binary);
// returns the temperature of the cold junction (analog input 0, first pdi word)
unsigned int ui_get_canopen_temperature_cold_junction(int *pi_temperature_value_binary);
    
// return a value which is incremented at each sucessfully received rpdo canbus frame
unsigned long ul_get_canbus_living_counter(void);

/******************************************************************************
**                            End Of File
******************************************************************************/

#endif //#ifndef def_az4186_can_h_included
