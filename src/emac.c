/*****************************************************************************
 *   emac.c:  Ethernet module file for NXP LPC230x Family Microprocessors
 *
 *   Copyright(C) 2006, NXP Semiconductor
 *   All rights reserved.
 *
 *   History
 *   2006.09.01  ver 1.00    Prelimnary version, first Release
 *
******************************************************************************/
#include "LPC23xx.h"                        /* LPC21xx definitions */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "type.h"
#include "target.h"
#include "irq.h"
#include <fpga_nxp.h>
#include <crc32.h>
#include <emac.h>
#include <spicost.h>

extern unsigned long get_ul_timer_interrupt_count(void);

typedef enum
{
	enum_emac_status_idle = 0,
	enum_emac_status_init,
	enum_emac_status_init_reset,
	enum_emac_status_wait_reset,
	enum_emac_status_init_autonegotiate,
	enum_emac_status_wait_autonegotiate,
	enum_emac_status_complete,
	enum_emac_status_OK,
	enum_emac_status_ERROR,
	enum_emac_status_ends,

	enum_emac_status_numof
}enum_emac_status;

typedef enum
{
	emac_error_type_none = 0,
	emac_error_type_timeout_reset,
	emac_error_type_PHY_PHYIDR1,
	emac_error_type_PHY_PHYIDR2,
	emac_error_type_timeout_auto_negotiate,
	emac_error_type_unknown,
	emac_error_type_numof,
}emac_error_type;

#define def_timeout_emac_reset_ms 1000
#define def_emac_timeout_auto_negotiate_ms 10000

typedef struct _type_emac_stati
{
	enum_emac_status status;
	emac_error_type error_type;
	unsigned int is_finished;
	unsigned int is_OK;
	unsigned int is_ERROR;
	unsigned int num_OK;
	unsigned int num_ERROR;
	unsigned int num_ERROR_by_type[emac_error_type_numof];

	unsigned long ul_prev_timer_interrupt_count;
	unsigned int timeout_ms;
	DWORD PHYType;
}type_emac_stati;

static type_emac_stati emac_stati;

static void init_emac_stati(void)
{
	memset(&emac_stati, 0, sizeof(emac_stati));
	emac_stati.status = enum_emac_status_init;
	emac_stati.PHYType = NATIONAL_PHY;
}
static void reinit_emac_stati(void)
{
	emac_stati.status = enum_emac_status_init;
	emac_stati.is_ERROR = 0;
	emac_stati.is_OK = 0;
	emac_stati.is_finished = 0;
}

typedef struct _type_emac
{
	volatile DWORD Duplex;
	volatile DWORD Speed;

	volatile DWORD RXOverrunCount;
	volatile DWORD RXErrorCount;

	volatile DWORD TXUnderrunCount;
	volatile DWORD TXErrorCount;
	volatile DWORD RxFinishedCount;
	volatile DWORD TxFinishedCount;
	volatile DWORD TxDoneCount;
	volatile DWORD RxDoneCount;

	volatile DWORD CurrentRxPtr;
	volatile DWORD ReceiveLength;
	volatile DWORD PacketReceived;

	unsigned int rx_produce_consume_indexes_wrong;
	unsigned int anl_par,anl_par_np;

	#if ENABLE_WOL
	volatile DWORD WOLCount;
	volatile DWORD WOLArrived;
	#endif
}type_emac;

static type_emac my_emac;

static void my_emac_init(void)
{
	memset(&my_emac, 0, sizeof(my_emac));
	my_emac.CurrentRxPtr = EMAC_RX_BUFFER_ADDR;
	my_emac.PacketReceived = FALSE;
}


/******************************************************************************
** Function name:		EMAC_TxEnable/EMAC_TxDisable
**
** Descriptions:		EMAC TX API modules
**
** parameters:			None
** Returned value:		None
** 
******************************************************************************/
void EMAC_TxEnable( void )
{
  MAC_COMMAND |= 0x02;
  return;
}

void EMAC_TxDisable( void )
{
  MAC_COMMAND &= ~0x02;
  return;
}

/******************************************************************************
** Function name:		EMAC_RxEnable/EMAC_RxDisable
**
** Descriptions:		EMAC RX API modules
**
** parameters:			None
** Returned value:		None
** 
******************************************************************************/
void EMAC_RxEnable( void )
{
  MAC_COMMAND |= 0x01;
  MAC_MAC1 |= 0x01;
  return;    
}

void EMAC_RxDisable( void )
{
  MAC_COMMAND &= ~0x01;
  MAC_MAC1 &= ~0x01;
  return;
}

/******************************************************************************
** Function name:		EMACHandler
**
** Descriptions:		EMAC interrupt handler
**
** parameters:			None
** Returned value:		None
** 
******************************************************************************/
void EMACHandler (void)  
{
  volatile DWORD IntStatus;
  IntStatus = MAC_INTSTATUS;    
  if ( IntStatus != 0 )	/* At least one interrupt */
  {
#if ENABLE_WOL
	if ( IntStatus & EMAC_INT_WOL )
	{
	  MAC_INTCLEAR = EMAC_INT_WOL;
	  my_emac.WOLCount++;
	  my_emac.WOLArrived = TRUE;
	  /* the packet will be lost, no need to anything else, bail out */
	  return;
	}
#endif
	if ( IntStatus & EMAC_INT_RXOVERRUN )
	{
	  MAC_INTCLEAR = EMAC_INT_RXOVERRUN;
	  my_emac.RXOverrunCount++;
	  return;
	}

	if ( IntStatus & EMAC_INT_RXERROR )
	{
	  MAC_INTCLEAR = EMAC_INT_RXERROR;
	  my_emac.RXErrorCount++;
	  return;
	}
	
	if ( IntStatus & EMAC_INT_RXFINISHED )
	{
	  MAC_INTCLEAR = EMAC_INT_RXFINISHED;
	  my_emac.RxFinishedCount++;
	  /* Below should never happen or RX is seriously wrong */
	  if ( MAC_RXPRODUCEINDEX != (MAC_RXCONSUMEINDEX - 1) )
	  {
		  my_emac.rx_produce_consume_indexes_wrong++;
	  }
	}

	if ( IntStatus & EMAC_INT_RXDONE )
	{
	  MAC_INTCLEAR = EMAC_INT_RXDONE;
	  my_emac.ReceiveLength = EMACReceive( );
	  my_emac.PacketReceived = TRUE;
	  my_emac.RxDoneCount++;
	}

	if ( IntStatus & EMAC_INT_TXUNDERRUN )
	{
	  MAC_INTCLEAR = EMAC_INT_TXUNDERRUN;
	  my_emac.TXUnderrunCount++;
	  return;
	}
	
	if ( IntStatus & EMAC_INT_TXERROR )
	{
	  MAC_INTCLEAR = EMAC_INT_TXERROR;
	  my_emac.TXErrorCount++;
	  return;
	}

	if ( IntStatus & EMAC_INT_TXFINISHED )
	{
	  MAC_INTCLEAR = EMAC_INT_TXFINISHED;
	  my_emac.TxFinishedCount++;
	}

	if ( IntStatus & EMAC_INT_TXDONE )
	{
	  MAC_INTCLEAR = EMAC_INT_TXDONE;
	  my_emac.TxDoneCount++;
	}
  }   
  return;
}

/*****************************************************************************
** Function name:		WritePHY
**
** Descriptions:		Write Data to the PHY port
**
** parameters:			PHY register, write data
** Returned value:		None
** 
*****************************************************************************/
void WritePHY( DWORD PHYReg, DWORD PHYData )
{
  MAC_MCMD = 0x0000;			/* write command */
  MAC_MADR = PHY_ADDR | PHYReg;	/* [12:8] == PHY addr, [4:0]=register addr */
  MAC_MWTD = PHYData;
  while ( MAC_MIND != 0 );
  return;
}

/*****************************************************************************
** Function name:		ReadPHY
**
** Descriptions:		Read data from the PHY port
**
** parameters:			PHY register
** Returned value:		PHY data
** 
*****************************************************************************/
DWORD ReadPHY( DWORD PHYReg )
{
  MAC_MCMD = 0x0001;			/* read command */
  MAC_MADR = PHY_ADDR | PHYReg;	/* [12:8] == PHY addr, [4:0]= register addr */
  while ( (MAC_MIND & 0x05) != 0 );
  MAC_MCMD = 0x0000;
  return( MAC_MRDD );
}

DWORD find_phy(void )
{
    int i;
    static int i_MAC_MRDD,iFound;
    i=1;
    iFound=0;
    while(i<32){
        MAC_MCMD = 0x0001;			/* read command */
        MAC_MADR = (i<<8);	/* [12:8] == PHY addr, [4:0]= register addr */
        while ( (MAC_MIND & 0x05) != 0 );
        MAC_MCMD = 0x0000;
        i_MAC_MRDD=( MAC_MRDD );
        if (i_MAC_MRDD!=0xffff){
            iFound++;
        }
        i++;
    }
    return iFound;
}



/*****************************************************************************
** Function name:		EMACTxDesciptorInit
**
** Descriptions:		initialize EMAC TX descriptor table
**
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void EMACTxDescriptorInit( void )
{
  DWORD i;
  DWORD *tx_desc_addr, *tx_status_addr;
   
  /*-----------------------------------------------------------------------------      
   * setup the Tx status,descriptor registers -- 
   * Note, the actual tx packet data is loaded into the ahb2_sram16k memory as part
   * of the simulation
   *----------------------------------------------------------------------------*/ 
  MAC_TXDESCRIPTOR = TX_DESCRIPTOR_ADDR;	/* Base addr of tx descriptor array */
  MAC_TXSTATUS = TX_STATUS_ADDR;		/* Base addr of tx status */
  MAC_TXDESCRIPTORNUM = EMAC_TX_DESCRIPTOR_COUNT - 1;	/* number of tx descriptors, 16 */

  for ( i = 0; i < EMAC_TX_DESCRIPTOR_COUNT; i++ )
  {
	tx_desc_addr = (DWORD *)(TX_DESCRIPTOR_ADDR + i * 8);	/* two words at a time, packet and control */
	*tx_desc_addr = (DWORD)(EMAC_TX_BUFFER_ADDR + i * EMAC_BLOCK_SIZE);
	*(tx_desc_addr+1) = (DWORD)(EMAC_TX_DESC_INT | (EMAC_BLOCK_SIZE - 1));	/* set size only */
  }
    
  for ( i = 0; i < EMAC_TX_DESCRIPTOR_COUNT; i++ )
  {
	tx_status_addr = (DWORD *)(TX_STATUS_ADDR + i * 4);	/* TX status, one word only, status info. */
	*tx_status_addr = (DWORD)0;		/* initially, set status info to 0 */
  }
  MAC_TXPRODUCEINDEX = 0x0;	/* TX descriptors point to zero */
  return;
}

/*****************************************************************************
** Function name:		EMACRxDesciptorInit
**
** Descriptions:		initialize EMAC RX descriptor table
**
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void EMACRxDescriptorInit( void )
{
  DWORD i;
  DWORD *rx_desc_addr, *rx_status_addr;
   
  /*-----------------------------------------------------------------------------      
   * setup the Rx status,descriptor registers -- 
   * Note, the actual rx packet data is loaded into the ahb2_sram16k memory as part
   * of the simulation
   *----------------------------------------------------------------------------*/ 
  MAC_RXDESCRIPTOR = RX_DESCRIPTOR_ADDR;	/* Base addr of rx descriptor array */
  MAC_RXSTATUS = RX_STATUS_ADDR;			/* Base addr of rx status */
  MAC_RXDESCRIPTORNUM = EMAC_RX_DESCRIPTOR_COUNT - 1;	/* number of rx descriptors, 16 */

  for ( i = 0; i < EMAC_RX_DESCRIPTOR_COUNT; i++ )
  {
	/* two words at a time, packet and control */
	rx_desc_addr = (DWORD *)(RX_DESCRIPTOR_ADDR + i * 8);
	*rx_desc_addr = (DWORD)(EMAC_RX_BUFFER_ADDR + i * EMAC_BLOCK_SIZE);
	*(rx_desc_addr+1) = (DWORD)(EMAC_RX_DESC_INT | (EMAC_BLOCK_SIZE - 1));	/* set size only */    
  }
 
  for ( i = 0; i < EMAC_RX_DESCRIPTOR_COUNT; i++ )
  {
	/* RX status, two words, status info. and status hash CRC. */
	rx_status_addr = (DWORD *)(RX_STATUS_ADDR + i * 8);	
	*rx_status_addr = (DWORD)0;	/* initially, set both status info and hash CRC to 0 */
	*(rx_status_addr+1) = (DWORD)0; 
  }
  MAC_RXCONSUMEINDEX = 0x0;	/* RX descriptor points to zero */
  return;
}

static void v_emac_reset_init()
{
	MAC_INTCLEAR = 0xFFFF;	/* clear all MAC interrupts */
	/* MAC interrupt related register setting */
	// if ( install_irq( EMAC_INT, (void *)EMACHandler, HIGHEST_PRIORITY ) == FALSE )
	// {
	// return (FALSE);
	// }
	VICIntSelect &= ~(1<<21); // ethernet int disable
	MAC_INTCLEAR = 0xFFFF;	/* clear all MAC interrupts */

	/* turn on the ethernet MAC clock in PCONP, bit 30 */
	DWORD regVal = PCONP;
	regVal |= PCONP_EMAC_CLOCK;
	PCONP = regVal;
	v_wait_microseconds(100);

	/*------------------------------------------------------------------------------
	* write to PINSEL2/3 to select the PHY functions on P1[17:0]
	*-----------------------------------------------------------------------------*/
	/* documentation needs to be updated */
	#if RMII
		/* P1.6, ENET-TX_CLK, has to be set for EMAC to address a BUG in the engineering
		version, even if this pin is not used for RMII interface. This bug has been fixed,
		and this port pin can be used as GPIO pin in the future release. */
		/* Unfortunately, this MCB2300 board still has the old eng. version LPC23xx chip
		on it. On the new rev.(xxAY, released on 06/22/2007), P1.6 should NOT be set.
		See errata for more details. */
		regVal = MAC_MODULEID;
		if ( regVal == PHILIPS_EMAC_MODULE_ID )
		{
			/* This is the rev."-" ID for the existing MCB2300 board,
			on rev. A, regVal should NOT equal to PHILIPS_EMAC_MODULE_ID,
			P1.6 should NOT be set. */
			PINSEL2 = 0x50151105;	/* selects P1[0,1,4,6,8,9,10,14,15] */
		}
		else
		{
			PINSEL2 = 0x50150105;	/* selects P1[0,1,4,8,9,10,14,15] */
		}
		PINSEL3 = 0x00000005;	/* selects P1[17:16] */
		// ENABLE PULLUP RESISTORS
		PINMODE3 &=~ 0x0000000F;
	#else					/* else RMII, then it's MII mode */
		PINSEL2 = 0x55555555;	/* selects P1[15:0] */
		PINSEL3 = 0x00000005;	/* selects P1[17:16] */
	#endif

	/*-----------------------------------------------------------------------------
	* write the MAC config registers
	*----------------------------------------------------------------------------*/
	MAC_MAC1 = 0xCF00;	/* [15],[14],[11:8] -> soft resets all MAC internal modules */
	MAC_COMMAND = 0x0038;	/* reset all datapaths and host registers */

	v_wait_microseconds(10);

	MAC_MAC1 = 0x0;		/* deassert all of the above soft resets in MAC1 */

	EMAC_TxDisable();
	EMAC_RxDisable();

	MAC_MAC2 = 0x00;		/* initialize MAC2 register to default value */

	/* Non back to back inter-packet gap */
	MAC_IPGR = 0x0012;	/* use the default value recommended in the users manual */

	MAC_CLRT = 0x370F;	/* Use the default value in the users manual */
	MAC_MAXF = 0x0600;	/* Use the default value in the users manual */

	/* MII configuration, I may just need to do either RMII or MII configuration, not both. */
	MAC_MCFG = 0x8018;	/* host clock divided by 20, no suppress preamble, no scan increment */
	//MAC_MCFG = 0x801C;	/* host clock divided by 28 (i.e. 60/28=2.15 OK), no suppress preamble, no scan increment */
	v_wait_microseconds(10);
	MAC_MCFG = 0x0018;	/* Apply a reset */
	//  MAC_MCFG = 0x001C;	/* Apply a reset */
	MAC_MCMD = 0;
	#if RMII
		/* RMII configuration */
		MAC_COMMAND |= 0x0200;
		/* PHY support: [8]=0 ->10 Mbps mode, =1 -> 100 Mbps mode */
		MAC_SUPP = 0x0100;	/* RMII setting, at power-on, default set to 100. */
	#else
		MAC_COMMAND &= ~0x0200;
		MAC_SUPP = 0x0000;
	#endif

	v_wait_microseconds(100);

	WritePHY( PHY_BMCR, BMCR_RESET );
	v_wait_microseconds(100);
}

/*****************************************************************************
** Function name:		EMACInit
**
** Descriptions:		initialize EMAC port
**
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
static void set_emac_stati_error(emac_error_type e)
{
	emac_stati.error_type = e;
	if (e != emac_error_type_none)
	{
		emac_stati.status = enum_emac_status_ERROR;
	}
}


static void emac_complete(void)
{
	DWORD regValue = ReadPHY( NSM_PHY_PHYSTS );
	/* Link established from here on */
	if ( regValue & 0x04 )
	{
		my_emac.Duplex = FULL_DUPLEX;
	}
	else
	{
		my_emac.Duplex = HALF_DUPLEX;
	}

	if ( regValue & 0x02 )
	{
		my_emac.Speed = SPEED_10;
	}
	else
	{
		my_emac.Speed = SPEED_100;
	}
	{
		my_emac.anl_par=ReadPHY(PHY_ANLPAR);
		my_emac.anl_par_np=ReadPHY(PHY_ANLPARNP);
	}

	/* write the station address registers */
	MAC_SA0 = EMAC_ADDR12;
	MAC_SA1 = EMAC_ADDR34;
	MAC_SA2 = EMAC_ADDR56;

	if ( (my_emac.Speed == SPEED_10) && (my_emac.Duplex == HALF_DUPLEX) )
	{
		MAC_MAC2 = 0x30;		/* half duplex, CRC and PAD enabled. */
		MAC_SUPP &= ~0x0100;	/* RMII Support Reg. speed is set to 10M */
		#if RMII
			MAC_COMMAND |= 0x0240;
		#else
			MAC_COMMAND |= 0x0040;	/* [10]-half duplex,[9]-MII mode,[6]-Pass runt
							frame, [5]-RxReset */
		#endif
		/* back to back int-packet gap */
		MAC_IPGT = 0x0012;		/* IPG setting in half duplex mode */
	}
	else if ( (my_emac.Speed == SPEED_100) && (my_emac.Duplex == HALF_DUPLEX) )
	{
		MAC_MAC2 = 0x30;		/* half duplex, CRC and PAD enabled. */
		MAC_SUPP |= 0x0100;		/* RMII Support Reg. speed is set to 100M */
		#if RMII
			MAC_COMMAND |= 0x0240;
		#else
			MAC_COMMAND |= 0x0040;	/* [10]-half duplex,[9]-MII mode,[6]-Pass runt frame,
			[5]-RxReset */
		#endif
		/* back to back int-packet gap */
		MAC_IPGT = 0x0012;		/* IPG setting in half duplex mode */
	}
	else if ( (my_emac.Speed == SPEED_10) && (my_emac.Duplex == FULL_DUPLEX) )
	{
		MAC_MAC2 = 0x31;		/* full duplex, CRC and PAD enabled. */
		MAC_SUPP &= ~0x0100;	/* RMII Support Reg. speed is set to 10M */
		#if RMII
			MAC_COMMAND |= 0x0640;
		#else
			MAC_COMMAND |= 0x0440;	/* [10]-full duplex,[9]-MII mode,[6]-Pass runt frame,
									[5]-RxReset */
		#endif
		/* back to back int-packet gap */
		MAC_IPGT = 0x0015;		/* IPG setting in full duplex mode */
	}
	else if ( (my_emac.Speed == SPEED_100) && (my_emac.Duplex == FULL_DUPLEX) )
							/* default setting, 100 BASE, FULL DUPLEX */
	{
		MAC_MAC2 = 0x31;		/* full duplex, CRC and PAD enabled. */
		MAC_SUPP |= 0x0100;		/* RMII Support Reg. speed is set to 100M */
		#if RMII
			MAC_COMMAND |= 0x0640;
		#else
			MAC_COMMAND |= 0x0440;	/* [10]-full duplex,[9]-MII mode,[6]-Pass runt frame,
									[5]-RxReset */
		#endif
		/* back to back int-packet gap */
		MAC_IPGT = 0x0015;	/* IPG setting in full duplex mode */
	}

	EMACTxDescriptorInit();
	EMACRxDescriptorInit();

	MAC_MAC1 |= 0x0002;		/* [1]-Pass All Rx Frame */
	MAC_COMMAND |=(1<<7); // receive all the packets

	/* Set up RX filter, accept broadcast and perfect station */
	#if ACCEPT_BROADCAST
		MAC_RXFILTERCTRL = 0x0022;	/* [1]-accept broadcast, [5]accept perfect */
	#else
		MAC_RXFILTERCTRL = 0x0020;	/* accept perfect match only */
	#endif

	#if MULTICAST_UNICAST
		MAC_RXFILTERCTRL |= 0x0005;
	#endif

	#if ENABLE_HASH
		MAC_RXFILTERCTRL |= 0x0018;
	#endif

	MAC_INTCLEAR = 0xFFFF;	/* clear all MAC interrupts */
	/* MAC interrupt related register setting */
	// if ( install_irq( EMAC_INT, (void *)EMACHandler, HIGHEST_PRIORITY ) == FALSE )
	// {
	// return (FALSE);
	// }
	VICIntSelect &= ~(1<<21); // ethernet int disable
	VICVectPriority21 = 1; // set priority for timer 0 to be quite high
	VICVectAddr21 = (unsigned long)EMACHandler;

	VICIntEnable |= (1<<21);

	#if ENABLE_WOL
		MAC_RXFILTERWOLCLR = 0xFFFF;/* set all bits to clear receive filter WOLs */
		MAC_RXFILTERCTRL |= 0x2000;	/* enable Rx Magic Packet and RxFilter Enable WOL */
		MAC_INTENABLE = 0x2000;	/* only enable WOL interrupt */
	#else
		MAC_INTENABLE = 0x00FF;	/* Enable all interrupts except SOFTINT and WOL */
	#endif
}


static void emac_init_autonegotiate(void)
{
	DWORD regValue = ReadPHY( NSM_PHY_PHYCR );
	WritePHY( NSM_PHY_PHYCR, regValue|0x8000 ); // enable auto mdix (auto cross mdi pairs)
	// read actual anar setup
	regValue = ReadPHY( PHY_ANAR);
	// enable only 10mbps mode (bit 5 and bit 6) and disable 100Mbps mode (bit 7 8 9)
	regValue |= (1<<5)|(1<<6)|(1<<7)|(1<<8)|(1<<9);
	//regValue &= ~((1<<7)|(1<<8)|(1<<9));
	WritePHY( PHY_ANAR, regValue );

	regValue = ReadPHY( PHY_BMCR );
	regValue=regValue|(BMCR_AN | BMCR_RE_AN);
	WritePHY( PHY_BMCR, regValue ); /* auto negotiation, restart AN */
}


typedef enum
{
	enum_EMACInit_retcode_completed_OK = 0,
	enum_EMACInit_retcode_running,
	enum_EMACInit_retcode_ERROR,
	enum_EMACInit_retcode_numof
}enum_EMACInit_retcode;

enum_EMACInit_retcode EMACInit( void )
{
	enum_EMACInit_retcode r = enum_EMACInit_retcode_running;
	unsigned int elapsed_ms = 0;
	unsigned long now = get_ul_timer_interrupt_count();
	if (now >= emac_stati.ul_prev_timer_interrupt_count)
	{
		elapsed_ms = (now - emac_stati.ul_prev_timer_interrupt_count) * PeriodoIntMs;
	}
	emac_stati.ul_prev_timer_interrupt_count = now;


	switch(emac_stati.status)
	{
		case enum_emac_status_idle:
		{
			break;
		}
		case enum_emac_status_init:
		{
			emac_stati.status = enum_emac_status_init_reset;
			break;
		}
		case enum_emac_status_init_reset:
		{
			emac_stati.is_OK = 0;
			emac_stati.is_ERROR = 0;
			emac_stati.is_finished = 0;
			set_emac_stati_error(emac_error_type_none);
			v_emac_reset_init();
			emac_stati.timeout_ms = 0;
			emac_stati.status = enum_emac_status_wait_reset;
			break;
		}
		case enum_emac_status_wait_reset:
		{
			DWORD regValue = ReadPHY( PHY_BMCR );
			if ( (regValue & BMCR_RESET) == 0x0000 )
			{
				v_wait_microseconds(100);
				//if ( PHYType == NATIONAL_PHY )
				// check PHY IDs to make sure the reset takes place and PHY is in its default state. See National PHY DP83848 Users Manual for more details
				regValue = ReadPHY( PHY_PHYIDR1 );
				if ( (regValue & 0x2000) != 0x2000 )
				{
					set_emac_stati_error(emac_error_type_PHY_PHYIDR1);
					break;
				}

				regValue = ReadPHY( PHY_PHYIDR2 );
				if ( (regValue & 0x5C90) != 0x5C90 )
				{
					set_emac_stati_error(emac_error_type_PHY_PHYIDR2);
					break;
				}
				emac_stati.status = enum_emac_status_init_autonegotiate;
			}
			else
			{
				emac_stati.timeout_ms += elapsed_ms;
				if (emac_stati.timeout_ms >= def_timeout_emac_reset_ms)
				{
					set_emac_stati_error(emac_error_type_timeout_reset);
					emac_stati.status = enum_emac_status_ERROR;
					break;
				}
			}
			break;
		}
		case enum_emac_status_init_autonegotiate:
		{
			emac_init_autonegotiate();

			emac_stati.timeout_ms = 0;
			emac_stati.status = enum_emac_status_wait_autonegotiate;
			break;
		}
		case enum_emac_status_wait_autonegotiate:
		{
			DWORD regValue = ReadPHY( NSM_PHY_PHYSTS );
			if ( (regValue & 0x0011) == 0x0011 )
			{
				emac_stati.status = enum_emac_status_complete;
			}
			else
			{
				emac_stati.timeout_ms += elapsed_ms;
				if (emac_stati.timeout_ms >= def_emac_timeout_auto_negotiate_ms)
				{
					set_emac_stati_error(emac_error_type_timeout_auto_negotiate);
					emac_stati.status = enum_emac_status_ERROR;
					break;
				}
			}

			break;
		}
		case enum_emac_status_complete:
		{
			emac_complete();
			emac_stati.status = enum_emac_status_OK;
			break;
		}
		case enum_emac_status_OK:
		{
			emac_stati.is_OK = 1;
			emac_stati.is_ERROR = 0;
			emac_stati.num_OK++;
			emac_stati.status = enum_emac_status_ends;
			break;
		}
		case enum_emac_status_ERROR:
		{
			emac_stati.is_OK = 0;
			emac_stati.is_ERROR = 1;
			emac_stati.num_ERROR++;
			emac_error_type e = emac_stati.error_type;
			if (e >= emac_error_type_numof)
			{
				e = emac_error_type_unknown;
			}
			emac_stati.num_ERROR_by_type[e]++;
			emac_stati.status = enum_emac_status_ends;
			break;
		}
		case enum_emac_status_ends:
		{
			emac_stati.is_finished = 1;
			break;
		}
		default:
		{
			break;
		}
	}

	if (!emac_stati.is_finished)
	{
		r = enum_EMACInit_retcode_running;
	}
	else
	{
		if (emac_stati.is_OK)
		{
			r = enum_EMACInit_retcode_completed_OK;
		}
		else
		{
			r = enum_EMACInit_retcode_ERROR;
		}
	}
	return r;

    
  /* On Keil and Internal Engineering board, National PHY is used.
     On IAR and Embedded Artists board, Micrel PHY is used. However,
	 on IAR board and Embedded Artists board, due to the different
	 I/O pin setting, the PHY addresses are different. On IAR board,
	 Internal Engineering board, and Keil board, the PHY address is
	 0x0001, on Embedded Artists board, the PHY address is 0x0000. 
	 See definition in mac.h */
/*
#if (KEIL_BOARD_LPC23XX || ENG_BOARD_LPC24XX)
  if ( PHYInit( NATIONAL_PHY ) == FALSE )
  {
	return ( FALSE );
  }
#else
#if (IAR_BOARD_LPC23XX || EA_BOARD_LPC24XX)
  if ( PHYInit( MICREL_PHY ) == FALSE )
  {
	return ( FALSE );
  }
#endif
#endif
*/
}

/*****************************************************************************
** Function name:		EMACReceiveFractions
**
** Descriptions:		Dealing with a fraction of EMAC packet
**
** parameters:			StartIndex and End Index
** Returned value:		packet length
** 
*****************************************************************************/
DWORD EMACReceiveFractions( DWORD StartIndex, DWORD EndIndex )
{
  DWORD i, RxLength = 0;
  DWORD RxSize;
  DWORD *rx_status_addr;

  for ( i = StartIndex; i < EndIndex; i++ )
  {
	/* Get RX status, two words, status info. and status hash CRC. */
	rx_status_addr = (DWORD *)(RX_STATUS_ADDR + StartIndex * 8);
	RxSize = (*rx_status_addr & DESC_SIZE_MASK) - 1;
	/* two words at a time, packet and control */
	my_emac.CurrentRxPtr += EMAC_BLOCK_SIZE;
	StartIndex++;
	/* last fragment of a frame */
	if ( *rx_status_addr & RX_DESC_STATUS_LAST ) 
	{
	  /* set INT bit and RX packet size */
	  MAC_RXCONSUMEINDEX = StartIndex;
	  RxLength += RxSize;
	  break; 
	}
	else	/* In the middle of the frame, the RxSize should be EMAC_BLOCK_SIZE */
			/* In the emac.h, the EMAC_BLOCK_SIZE has been set to the largest 
			ethernet packet length to simplify the process, so, it should not 
			come here in any case to deal with fragmentation. Otherwise, 
			fragmentation and repacking will be needed. */
	{
	  /* set INT bit and maximum block size */
	  MAC_RXCONSUMEINDEX = StartIndex;
	  /* wait until the whole block is received, size is EMAC_BLOCK_SIZE. */
	  while ( (*rx_status_addr & DESC_SIZE_MASK) != (EMAC_BLOCK_SIZE - 1));
	  RxLength += RxSize;
	}
  }
  return( RxLength );
}

/*****************************************************************************
** Function name:		EMACReceive
**
** Descriptions:		Receive a EMAC packet, called by ISR
**
** parameters:			buffer pointer
** Returned value:		packet length
** 
*****************************************************************************/
DWORD EMACReceive( void )
{
  DWORD RxProduceIndex, RxConsumeIndex;
  DWORD RxLength = 0;
  DWORD Counter = 0;

  /* the input parameter, EMCBuf, needs to be word aligned */
  RxProduceIndex = MAC_RXPRODUCEINDEX;
  RxConsumeIndex = MAC_RXCONSUMEINDEX;

  if ( RxProduceIndex == EMAC_RX_DESCRIPTOR_COUNT )
  {
	/* reach the limit, that probably should never happen */
	MAC_RXPRODUCEINDEX = 0;
	my_emac.CurrentRxPtr = EMAC_RX_BUFFER_ADDR;
  }

  /* a packet has arrived. */
  if ( RxProduceIndex != RxConsumeIndex )
  {
	if ( RxProduceIndex < RxConsumeIndex )	/* Wrapped around already */
	{
	  /* take care of unwrapped, RxConsumeIndex to EMAC_RX_DESCERIPTOR_COUNT */
	  RxLength += EMACReceiveFractions( RxConsumeIndex, EMAC_RX_DESCRIPTOR_COUNT );
	  Counter++;
	  my_emac.PacketReceived = TRUE;
	
	  /* then take care of wrapped, 0 to RxProduceIndex */
	  if ( RxProduceIndex > 0 )
	  {
		RxLength += EMACReceiveFractions( 0, RxProduceIndex );
		Counter++;
	  }
	}
	else					/* Normal process */
	{
	    RxLength += EMACReceiveFractions( RxConsumeIndex, RxProduceIndex );
		Counter++;	
	}
  }
  return( RxLength );
}

/*****************************************************************************
** Function name:		EMACSend
**
** Descriptions:		Send a EMAC packet
**
** parameters:			buffer pointer, buffer length
** Returned value:		true or false
** 
*****************************************************************************/
DWORD EMACSend( DWORD *EMACBuf, DWORD length )
{
  DWORD *tx_desc_addr;
  DWORD TxProduceIndex;
  DWORD TxConsumeIndex;
  DWORD i, templen;

  TxProduceIndex = MAC_TXPRODUCEINDEX;
  TxConsumeIndex = MAC_TXCONSUMEINDEX;

  if ( TxConsumeIndex != TxProduceIndex )
  {
	return ( FALSE );
  }

  if ( TxProduceIndex == EMAC_TX_DESCRIPTOR_COUNT )
  {
	/* reach the limit, that probably should never happen */
	/* To be tested */
	MAC_TXPRODUCEINDEX = 0;
  }

  if ( length > EMAC_BLOCK_SIZE )
  {
	templen = length;
	for ( i = 0; (DWORD)(length/EMAC_BLOCK_SIZE) + 1; i++ )
	{
	  templen = length - EMAC_BLOCK_SIZE;
	  /* two words at a time, packet and control */
	  tx_desc_addr = (DWORD *)(TX_DESCRIPTOR_ADDR + TxProduceIndex * 8);
	  /* descriptor status needs to be checked first */
	  if ( templen % EMAC_BLOCK_SIZE )
	  {
		/* full block */ 
		*tx_desc_addr = (DWORD)(EMACBuf + i * EMAC_BLOCK_SIZE);
		/* set TX descriptor control field */
		*(tx_desc_addr+1) = (DWORD)(EMAC_TX_DESC_INT | (EMAC_BLOCK_SIZE - 1));
		TxProduceIndex++;
		if ( TxProduceIndex == EMAC_TX_DESCRIPTOR_COUNT )
    	{
		  TxProduceIndex = 0;
		}
		MAC_TXPRODUCEINDEX = TxProduceIndex;	/* transmit now */
	  }
	  else
	  {
		/* last fragment */
		*tx_desc_addr = (DWORD)(EMACBuf + i * EMAC_BLOCK_SIZE);
		/* set TX descriptor control field */
		*(tx_desc_addr+1) = (DWORD)(EMAC_TX_DESC_INT | EMAC_TX_DESC_LAST | (templen -1) );
		TxProduceIndex++;		/* transmit now */
		if ( TxProduceIndex == EMAC_TX_DESCRIPTOR_COUNT )
    	{
		  TxProduceIndex = 0;
		}
		MAC_TXPRODUCEINDEX = TxProduceIndex;	/* transmit now */
		break;
	  }    
	}
  }
  else
  {
	tx_desc_addr = (DWORD *)(TX_DESCRIPTOR_ADDR + TxProduceIndex * 8);
	/* descriptor status needs to be checked first */
	*tx_desc_addr = (DWORD)(EMACBuf);
	/* set TX descriptor control field */
	*(tx_desc_addr+1) = (DWORD)(EMAC_TX_DESC_INT | EMAC_TX_DESC_LAST | (length -1));
	TxProduceIndex++;		/* transmit now */
	if ( TxProduceIndex == EMAC_TX_DESCRIPTOR_COUNT )
	{
	  TxProduceIndex = 0;
	}
	MAC_TXPRODUCEINDEX = TxProduceIndex;
  }
  return ( TRUE );
}

#define TX_PACKET_SIZE		114
const char c_test_body_message[]="CSM AZ4186 board ethernet message; we are sending packets through embedded interface; this is a test message we send to check if the transmission works";

#define my_ETH_ALEN 6		// number of bytes of the MAC address
#define my_ETH_P_IP 0x0800 	// an IP Packet ID

typedef struct my_ether_header
{
	uint8_t  ether_dhost[my_ETH_ALEN];	/* destination eth addr	*/
	uint8_t  ether_shost[my_ETH_ALEN];	/* source ether addr	*/
	uint16_t ether_type;		        /* packet type ID field	*/
} __attribute__ ((__packed__)) type_my_ether_header;


typedef struct my_iphdr
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	unsigned int ihl:4;
	unsigned int version:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
    unsigned int version:4;
    unsigned int ihl:4;
#else
# error	"Please fix <bits/endian.h>"
#endif
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
    /*The options start here. */
}__attribute__ ((__packed__)) type_my_iphdr;

typedef struct my_udphdr
{
	uint16_t source;
	uint16_t dest;
	uint16_t len;
	uint16_t check;
}__attribute__ ((__packed__)) type_my_udphdr;

#define def_max_len_my_udp_payload 512

#if def_max_len_my_udp_payload > EMAC_BLOCK_SIZE
	#error "too big packet size!"
#endif

typedef struct _my_ether_packet_udp
{
	type_my_ether_header eh;
	type_my_iphdr ip;
	type_my_udphdr udp;
	uint8_t payload[def_max_len_my_udp_payload];
}__attribute__ ((__packed__)) type_my_ether_packet_udp;
typedef struct _build_packet_udp
{
	union
	{
		type_my_ether_packet_udp udp;
		uint8_t buf[sizeof(type_my_ether_packet_udp)];
	};

	uint32_t tx_len;
}__attribute__ ((__packed__)) type_build_packet_udp;

#warning "handle source MAC Address"
static uint8_t src_MAC_ADDRESS[my_ETH_ALEN] =
{
	0x10, 0x1f, 0xe0, 0x12, 0x1d, 0x0c
};

uint8_t *my_MAC_address_bytes(void)
{
	return src_MAC_ADDRESS;
}
uint8_t my_MAC_address_length(void)
{
	return sizeof(src_MAC_ADDRESS) / sizeof(src_MAC_ADDRESS[0]);
}


#if TX_ONLY
	// half integer from native byte order to big endian byte order
	static uint16_t my_htons(uint16_t src)
	{
	#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		uint16_t r = ((src << 8) & 0xFF00) | ((src >> 8) & 0xFF);
	#else
		uint16_t r = src;
	#endif
		return r;
	}

	#warning "handle dst MAC Address"
	static uint8_t dst_MAC_ADDRESS[my_ETH_ALEN] =
	{
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};

	static unsigned short csum(unsigned short *buf, int nwords)
	{
		unsigned long sum;
		for(sum=0; nwords>0; nwords--)
			sum += *buf++;
		sum = (sum >> 16) + (sum &0xffff);
		sum += (sum >> 16);
		return (unsigned short)(~sum);
	}
	static void build_my_udp_packet(type_build_packet_udp *p)
	{
		uint32_t tx_len = 0;
		// ethernet header
		type_my_ether_header *peh = &p->udp.eh;
		{
			memcpy(&peh->ether_shost[0], src_MAC_ADDRESS, sizeof(peh->ether_shost));
			memcpy(&peh->ether_dhost[0], dst_MAC_ADDRESS, sizeof(peh->ether_shost));
			peh->ether_type = my_htons(my_ETH_P_IP);
			tx_len += sizeof(*peh);
		}
		// ip header
		type_my_iphdr *iph = &p->udp.ip;
		{
			/* IP Header */
			iph->ihl 		= 5;
			iph->version 	= 4;
			iph->tos 		= 16; 	// Low delay
			iph->id 		= my_htons(54321);
			iph->ttl 		= 255; 	// hops
			iph->protocol 	= 17; 	// UDP
			/* Source IP address, can be spoofed */
			//iph->saddr = inet_addr(inet_ntoa(((struct sockaddr_in *)&if_ip.ifr_addr)->sin_addr));
			iph->saddr = 0x00000000;
			/* Destination IP address */
			iph->daddr = 0xffffffff;
			tx_len += sizeof(*iph);
		}
		// udp header
		type_my_udphdr *udph = &p->udp.udp;
		{
			/* UDP Header */
			udph->source = my_htons(3118);
			udph->dest = my_htons(3119);
			udph->check = 0; // skip
			tx_len += sizeof(*udph);
		}
		{
			uint8_t * p_pay = &p->udp.payload[0];
			memcpy(p_pay, c_test_body_message, sizeof(p->udp.payload));
			tx_len += sizeof(p->udp.payload);
		}

		/* Length of UDP payload and header */
		udph->len = my_htons(tx_len - sizeof(type_my_ether_header) - sizeof(type_my_iphdr));
		/* Length of IP payload and header */
		iph->tot_len = my_htons(tx_len - sizeof(type_my_ether_header));
		/* Calculate IP checksum on completed header */
		iph->check = csum((unsigned short *)(p->buf + sizeof(type_my_ether_header)), sizeof(type_my_iphdr) / 2);
		p->tx_len = tx_len;
	}

	/******************************************************************************
	** Function name:		PacketGen
	**
	** Descriptions:		Create a perfect packet for TX
	**
	** parameters:			None
	** Returned value:		None
	**
	******************************************************************************/
	static void PacketGen( BYTE *txptr )
	{

	  int i;
	  DWORD crcValue;
	  DWORD BodyLength = TX_PACKET_SIZE - 14;

	  /* Dest address */
	  *(txptr+0) = EMAC_DST_ADDR56 & 0xFF;
	  *(txptr+1) = (EMAC_DST_ADDR56 >> 0x08) & 0xFF;
	  *(txptr+2) = EMAC_DST_ADDR34 & 0xFF;
	  *(txptr+3) = (EMAC_DST_ADDR34 >> 0x08) & 0xFF;
	  *(txptr+4) = EMAC_DST_ADDR12 & 0xFF;
	  *(txptr+5) = (EMAC_DST_ADDR12 >> 0x08) & 0xFF;

	  /* Src address */
	  *(txptr+6) = EMAC_ADDR56 & 0xFF;
	  *(txptr+7) = (EMAC_ADDR56 >> 0x08) & 0xFF;
	  *(txptr+8) = EMAC_ADDR34 & 0xFF;
	  *(txptr+9) = (EMAC_ADDR34 >> 0x08) & 0xFF;
	  *(txptr+10) = EMAC_ADDR12 & 0xFF;
	  *(txptr+11) = (EMAC_ADDR12 >> 0x08) & 0xFF;

	  /* Type or length, body length is TX_PACKET_SIZE - 14 bytes */
	  *(txptr+12) = BodyLength & 0xFF;
	  *(txptr+13) = (BodyLength >> 0x08) & 0xFF;

	  /* Skip the first 14 bytes for dst, src, and type/length */
	  for ( i=0; i < BodyLength; i++ )
	  {
		*(txptr+i+14) = c_test_body_message[i];
	  }
	  crcValue = crc32_bfr( txptr, TX_PACKET_SIZE );

	  *(txptr+TX_PACKET_SIZE) = (0xff & crcValue);
	  *(txptr+TX_PACKET_SIZE+1) = 0xff & (crcValue >> 8 );
	  *(txptr+TX_PACKET_SIZE+2) = 0xff & (crcValue >> 16);
	  *(txptr+TX_PACKET_SIZE+3) = 0xff & (crcValue >> 24);
	  return;
	}
	/*****************************************************************************
	** Function name:		AllPacketGen
	**
	** Descriptions:		Fill all the TX buffers based on the number
	**						of TX blocks.
	**
	** parameters:			None
	** Returned value:		None
	**
	*****************************************************************************/
	static void AllPacketGen ( void )
	{
	  DWORD i;
	  BYTE *txptr;

	  txptr = (BYTE *)EMAC_TX_BUFFER_ADDR;
	  for ( i = 0; i < EMAC_TX_BLOCK_NUM; i++ )
	  {
		//PacketGen( txptr );
		build_my_udp_packet((type_build_packet_udp *)txptr);
		txptr += EMAC_BLOCK_SIZE;
	  }
	  return;
	}
#endif


typedef struct _emac_stats
{
	uint32_t OK_num_receive;
	uint32_t OK_num_send;
	uint32_t ERR_receive_too_long;
	uint32_t ERR_send_len;
	uint32_t ERR_send;
}type_emac_stats;


typedef struct _type_emac_handle
{
	unsigned int init_OK;
	BYTE *txptr;
	BYTE *rxptr;
	unsigned int mac_intstatus,mac_status, mac_command, mac_mac1;
	unsigned int idx_buffer;
	unsigned long ul_prev_timer_interrupt_count;
	#if TX_ONLY
		unsigned int enable_TX;
		unsigned int timeout_TX_ms;
		unsigned int do_TX;
		DWORD j;
	#endif
	#if ENABLE_HASH
		long long dstAddr;
		DWORD gHashCrcValue;
	#endif
	#if ENABLE_WOL
		DWORD FirstTime_WOL = TRUE;
	#endif
#if UIP
	uint8_t ibuffer[EMAC_BLOCK_SIZE];
	uint32_t rx_length;
	unsigned int rx_idx_buffer;
	unsigned int rx_valid;
	unsigned int tx_idx_buffer;
	unsigned int tx_valid;
#endif

	type_emac_stats stats;
}type_emac_handle;
type_emac_handle emac_handle;

void az4186_emac_init(void)
{
	memset(&emac_handle, 0, sizeof(emac_handle));
	init_emac_stati();
	my_emac_init();
}


#include "uip.h"
unsigned int netdev_read(void)
{
	unsigned int r = 0;
	if (emac_handle.rx_valid)
	{
		uint32_t rx_length = emac_handle.rx_length;
		if (rx_length <= UIP_BUFSIZE)
		{
			r = rx_length;
			memcpy(uip_buf, emac_handle.ibuffer, rx_length);
		}
		emac_handle.rx_valid = 0;
	}
	return r;
}

void netdev_send(void)
{
	uint16_t len = uip_len;
	if (len > EMAC_BLOCK_SIZE)
	{
		emac_handle.stats.ERR_send_len++;
	}
	else
	{
		emac_handle.txptr = (BYTE *)EMAC_TX_BUFFER_ADDR;
		memcpy(emac_handle.txptr, uip_buf, len);
		if (!EMACSend( (DWORD *)emac_handle.txptr, len))
		{
			emac_handle.stats.ERR_send++;
		}
		else
		{
			emac_handle.stats.OK_num_send++;
		}
	}
}



void az4186_emac_loop(void)
{

	if (!emac_handle.init_OK)
	{
		switch (EMACInit())
		{
			case enum_EMACInit_retcode_completed_OK:
			{
				emac_handle.init_OK = 1;
		        emac_handle.txptr = (BYTE *)EMAC_TX_BUFFER_ADDR;
		        emac_handle.rxptr = (BYTE *)EMAC_RX_BUFFER_ADDR;
		        EMAC_RxEnable();
		        EMAC_TxEnable();
				#if TX_ONLY
					// pre-format the transmit packets
					AllPacketGen();
					emac_handle.enable_TX = 1;
				#endif
					break;
			}
			case enum_EMACInit_retcode_running:
			{
				break;
			}
			case enum_EMACInit_retcode_ERROR:
			default:
			{
				reinit_emac_stati();
				break;
			}
		}

		return;
	}

#if UIP
	if ( my_emac.PacketReceived == TRUE )
	{
		my_emac.PacketReceived = FALSE;
		if (my_emac.ReceiveLength <= sizeof(emac_handle.ibuffer))
		{
			emac_handle.rx_valid = 0;
				memcpy(emac_handle.ibuffer,emac_handle.rxptr,sizeof(emac_handle.ibuffer));
				emac_handle.rx_length = my_emac.ReceiveLength;
			emac_handle.rx_valid = 1;
			emac_handle.stats.OK_num_receive++;
		}
		else
		{
			emac_handle.stats.ERR_receive_too_long++;
		}
//	  EMACSend( (DWORD *)emac_handle.txptr, ReceiveLength - 2 );
//	  emac_handle.txptr += EMAC_BLOCK_SIZE;
		emac_handle.rx_idx_buffer++;
		if ( ++emac_handle.rx_idx_buffer >= EMAC_RX_BLOCK_NUM )
		{
			emac_handle.rx_idx_buffer = 0;
			emac_handle.rxptr = (BYTE *)EMAC_RX_BUFFER_ADDR;
		}
		else
		{
			emac_handle.rxptr += EMAC_BLOCK_SIZE;
		}
	}
#endif

#if TX_ONLY
	if (emac_handle.enable_TX)
	{
		if (emac_handle.do_TX)
		{
			emac_handle.txptr = (BYTE *)EMAC_TX_BUFFER_ADDR;
			unsigned int i;
			for ( i = 0; i < 1; i++ )
//			for ( i = 0; i < EMAC_TX_BLOCK_NUM; i++ )
			{
				// Including 4 bytes of checksum, TX_PACKET_SIZE include 12 bytes of SRC and DST, and 2 bytes length/type.
				EMACSend( (DWORD *)emac_handle.txptr, TX_PACKET_SIZE + 4 );
				emac_handle.txptr += EMAC_BLOCK_SIZE;
				v_wait_microseconds(10000);
			}
			emac_handle.do_TX = 0;
			emac_handle.timeout_TX_ms = 0;
			emac_handle.ul_prev_timer_interrupt_count = get_ul_timer_interrupt_count();
		}
		else
		{
			unsigned int elapsed_ms = 0;
			unsigned long now = get_ul_timer_interrupt_count();
			if (now >= emac_handle.ul_prev_timer_interrupt_count)
			{
				elapsed_ms = (now - emac_handle.ul_prev_timer_interrupt_count) * PeriodoIntMs;
			}
			emac_handle.ul_prev_timer_interrupt_count = now;
			emac_handle.timeout_TX_ms += elapsed_ms;
			if (emac_handle.timeout_TX_ms > 1000)
			{
				emac_handle.do_TX = 1;
			}
		}
	}
#endif										/* endif TX_ONLY */

	#if ENABLE_HASH
		dstAddr = 0x010203040506;
		gHashCrcValue = do_crc_behav( dstAddr );
		Set_HashValue( gHashCrcValue );
	#endif


	#if BOUNCE_RX

		#if ENABLE_WOL
		  INTWAKE = 0x10;			/* Ethernet(WOL) Wakeup from powerdown mode */
		  LED_Blink( 0x000000F0 );	/* Indicating system is in power down now. */
		  PCON = 0x02;				/* Power down first */
		#endif						/* endif ENABLE_WOL */

	  {
		  emac_handle.mac_intstatus=MAC_INTSTATUS;
		  emac_handle.mac_status=MAC_STATUS;
		  emac_handle.mac_command=MAC_COMMAND;
		  emac_handle.mac_mac1=MAC_MAC1;
		#if ENABLE_WOL
			if ( (WOLArrived == TRUE) && (FirstTime_WOL == TRUE) )	/* WOL interrupt has occured */
			{
			  WOLArrived = FALSE;
			  FirstTime_WOL = FALSE;
			  EMAC_RxDisable();
			  EMAC_TxDisable();
			  /* From power down to WOL, the PLL needs to be reconfigured,
			  otherwise, the CCLK will be generated from 4Mhz IRC instead
			  of main OSC 12Mhz */
			  ConfigurePLL();
			  LED_Blink( 0x0000000F );		/* indicating system is awake now. */

			  /* Calling EMACInit() is overkill which also initializes the PHY, the
			  main reason to do that is to make sure the descriptors and descriptor
			  status for both TX and RX are clean and ready to use. It won't go wrong. */
			  EMACInit();
			  MAC_RXFILTERWOLCLR = 0xFFFF;	/* set all bits to clear receive filter WOLs */
			  MAC_RXFILTERCTRL &= ~0x2000;	/* enable Rx Magic Packet and RxFilter Enable WOL */
			  MAC_INTENABLE = 0x00FF;			/* Enable all interrupts except SOFTINT and WOL */
			  EMAC_RxEnable();
			  EMAC_TxEnable();
			  PacketReceived = FALSE;
			}
		#endif										/* endif ENABLE_WOL */

		if ( PacketReceived == TRUE )
		{
		  PacketReceived = FALSE;
		  /* Reverse Source and Destination, then copy the body */
		  memcpy( (BYTE *)emac_handle.txptr, (BYTE *)(emac_handle.rxptr+6), 6);
		  memcpy( (BYTE *)(emac_handle.txptr+6), (BYTE *)emac_handle.rxptr, 6);
		  memcpy( (BYTE *)(emac_handle.txptr+12), (BYTE *)(emac_handle.rxptr+12), ReceiveLength );
		  {
			memcpy(emac_handle.ibuffer,emac_handle.rxptr,sizeof(emac_handle.ibuffer));
		  }
		  EMACSend( (DWORD *)emac_handle.txptr, ReceiveLength - 2 );
		  emac_handle.txptr += EMAC_BLOCK_SIZE;
		  emac_handle.rxptr += EMAC_BLOCK_SIZE;
		  emac_handle.idx_buffer++;
		  /* EMAC_TX_BLOCK_NUM and EMAC_RX_BLOCK_NUM should be the same */
		  if ( emac_handle.idx_buffer >= EMAC_TX_BLOCK_NUM )
		  {
			  emac_handle.idx_buffer = 0;
				emac_handle.txptr = (BYTE *)EMAC_TX_BUFFER_ADDR;
				emac_handle.rxptr = (BYTE *)EMAC_RX_BUFFER_ADDR;
		  }
		}
	  }
	#endif										/* endif BOUNCE_RX */

}
/*********************************************************************************
**                            End Of File
*********************************************************************************/
