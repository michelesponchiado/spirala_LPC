/*-----------------------------------------------------------------------*/
/* Low level disk I/O module                                             */
/* (C) Copyright 2007,2008                                               */
/* Martin Thomas, Lorenz Aebi                                            */
/*-----------------------------------------------------------------------*/
/* This is a LPC23xx/24xx MCI disk I/O module                            */
/*-----------------------------------------------------------------------*/

// Add Multi Block Write by Lorenz Aebi 2008
// Bugfix from Marco Ziegert 20090823

// #define DISKIO_DEBUG
// #define DISKIO_DUMPSTAT
// #define DISKIO_DEBUGR
// #define DISKIO_DEBUGW

// if you don't want to do select at every disk read/write/sync, define the following
#define DISKIO_OPTIMIZE_SELECT

#include "LPC23xx.h"
#include "type.h"
#include "irq.h"
#include "target.h"
#include "mci.h"
#include "dma.h"
#include <string.h>
#include <sd_regs.h>

#include "diskio.h"
#include "spityp.h"
#include "spiext.h"

#ifdef __GNUC__
    //#include "monitor.h"
    #define myprintf xprintf
#else
    #include <stdio.h>
    #define myprintf printf
#endif

#ifdef __ARMCC_VERSION
    // RealView
    #define INLINE __inline
#else
    // gcc
    #define INLINE inline
#endif


#ifdef DISKIO_DEBUG
    #define diskio_printf myprintf
#else
    #define diskio_printf( fmt, args... )
#endif

#ifdef DISKIO_DEBUGR
#define diskio_printfr myprintf
#else
#define diskio_printfr( fmt, args... )
#endif

#ifdef DISKIO_DEBUGW
#define diskio_printfw myprintf
#else
// #define diskio_printfw( fmt, args... )
#define diskio_printfw( fmt, ... )
#endif


static volatile DWORD Timer1, Timer2;	/* 1000Hz decrement timer */

static volatile DSTATUS Stat = STA_NOINIT;	/* Disk status */

static UINT CardType;			/* b0:MMC, b1:SDC, b2:Block addressing */

//static BYTE csd[16]; /* cached csd */

//static BYTE sd_status[16]; /* cached 16 MS-Bytes from SD_STATUS (ACMD13) */

#define	DWORD_TO_BYTES(byte_ptr_,dw_val_)	\
do { \
	*(BYTE*)(byte_ptr_)     = (BYTE)((DWORD)(dw_val_)>>24); \
	*(BYTE*)((byte_ptr_)+1) = (BYTE)((DWORD)(dw_val_)>>16); \
	*(BYTE*)((byte_ptr_)+2) = (BYTE)((WORD)(dw_val_)>>8);   \
	*(BYTE*)((byte_ptr_)+3) = (BYTE)(dw_val_); \
} while (0)


extern volatile DWORD MCI_CardType;
extern volatile DWORD MCI_Block_End_Flag;

extern BYTE *WriteBlock, *ReadBlock;

#define CURR_STATE_IDLE     0
#define CURR_STATE_READY    1
#define CURR_STATE_IDENT    2
#define CURR_STATE_STBY     3
#define CURR_STATE_TRAN     4
#define CURR_STATE_DATA     5
#define CURR_STATE_RCV      6
#define CURR_STATE_PRG      7
#define CURR_STATE_DIS      8

#ifdef DISKIO_DUMPSTAT

    const char* states[] = {
        "idle", "ready", "ident", "stby", 
        "tran", "data", "rcv", "prg", "dis"
    };

    static volatile void dump_status( const char* info )
    {
        DWORD st;
        DWORD currst;

        st = MCI_Send_Status();
        if ( st == INVALID_RESPONSE ) {
            //diskio_printf("dump_status: Send_Status INVALID_RESPONSE\n");
            return;
        }

        //diskio_printf("------ Status register %s\n -----", info);
        //diskio_printf("AKE_SEQ_ERROR    : %s\n",  (st & (1 <<  3)) ? "error"     : "no error" );
        //diskio_printf("APP_CMD          : %s\n",  (st & (1 <<  5)) ? "enabled"   : "disabled" );
        //diskio_printf("READY_FOR_DATA   : %s\n",  (st & (1 <<  8)) ? "not ready" : "ready"    );

        currst = (st >> 9) & 0x0f;
        if ( currst > 8 ) {
            //diskio_printf("CURR_STATE       : reserved\n");
        }
        else {
            //diskio_printf("CURR_STATE       : %s   <---\n", states[currst]);
        }
        // TODO bits 13-18
        //diskio_printf("ERROR            : %s\n", (st & (1<<19)) ? "error"  : "no error" );
        //diskio_printf("CC_ERROR         : %s\n", (st & (1<<20)) ? "error"  : "no error" );
        //diskio_printf("CARD_ECC_FAILED  : %s\n", (st & (1<<21)) ? "failure": "success"  );
        //diskio_printf("ILLEGAL_COMMAND  : %s\n", (st & (1<<22)) ? "error"  : "no error" );
        //diskio_printf("COM_CRC_ERROR    : %s\n", (st & (1<<23)) ? "error"  : "no error" );
        //diskio_printf("LOCK_UNLOCK_FAIL : %s\n", (st & (1<<24)) ? "error"  : "no error" );
        //diskio_printf("CARD_IS_LOCKED   : %s\n", (st & (1<<25)) ? "locked" : "unlocked" );
        // TODO bits 26-28
        //diskio_printf("BLOCK_LEN_ERROR  : %s\n", (st & (1<<29)) ? "error"  : "no error" );
        //diskio_printf("ADDRESS_ERROR    : %s\n", (st & (1<<30)) ? "error"  : "no error" );
        //diskio_printf("OUT_OF_RANGE     : %s\n", (st & (1<<31)) ? "error"  : "no error" );
    }
#else 
    #define dump_status(s__)
#endif

static INLINE USHORT currstate_from_status( DWORD st )
{
	return ( (st >> 9 ) & 0xff );
}

static INLINE BYTE status_ready_bit( DWORD st )
{
	return ( st & ( 1<<8 ) ) ? 1 : 0;
}


typedef enum{
    enum_diag_alarm_error_none=0,
    enum_diag_alarm_error_read_block,
    enum_diag_alarm_error_write_block,
    enum_diag_alarm_timeout_read_block,
    enum_diag_alarm_timeout_write_block,
    enum_diag_alarm_timeout_select,
    enum_diag_alarm_numof
}enum_diag_alarm;

static void v_generate_diag_alarm(enum_diag_alarm the_alarm){
    if (the_alarm==enum_diag_alarm_error_none){
        // no allarme ERRORE SD CARD
        alarms &= ~ALR_SD_CARD;
    }
    else{
        // Allarme: ERRORE SD CARD
        alarms |= ALR_SD_CARD;
        // E' effettivamente un allarme valido.
        alr_status=alr_status_alr;
    }
}


// state to trans (SD Card State Diagram: V2.2 P4-7)
static void SELECT(void)
{
	DWORD currst, st;
    TipoStruct_timeout timeout_disk_select;

    timeout_init(&timeout_disk_select);
	do {
		// cmd7 until state is trans
		st = MCI_Send_Status();
		currst = currstate_from_status(st);
		if ( currst != CURR_STATE_TRAN ) {
			if ( MCI_Select_Card() == FALSE ) {
				//diskio_printf("SELECT Select_Card failed - retry\n");
			}
		}
        if (uc_timeout_active(&timeout_disk_select, 5000)){
            v_generate_diag_alarm(enum_diag_alarm_timeout_select);
            break;
        }
	} while (currst != CURR_STATE_TRAN );

}
#if 0
// state to stby
static void DESELECT(void)
{
	DWORD cardtype_save, st, currst;

	Timer2 = 500;
	do {
		st = MCI_Send_Status();
		
		currst = currstate_from_status(st);
	} while ( ( currst != CURR_STATE_TRAN ) && 
	          ( currst != CURR_STATE_STBY  ) && 
	          Timer2 );

	if ( Timer2 == 0 ) {
		//diskio_printf("DESELCT timeout waiting trans or stby\n");
	}
	else if ( currst == CURR_STATE_TRAN ) {
		cardtype_save = MCI_CardType;
		// CMD7: trans to stby (RCA 0) / de-select
		MCI_CardType = 0;
		if ( MCI_Select_Card() == FALSE ) {
			//diskio_printf("DESELECT failed\n");
		}
		MCI_CardType = cardtype_save;
	}
	else {
		//diskio_printf("DSELECT already in stby\n");
	}
	
	return;
}
#endif

// wait for stby or trans with timeout
static BYTE wait_ready( void ) 
{
	BYTE res;
	DWORD st;
	// DWORD currst;

	res = 0xaa;
	Timer2 = 500;	/* Wait for ready in timeout of 500ms */
	do {
		st = MCI_Send_Status();
		if ( st == INVALID_RESPONSE ) {
			//diskio_printf("wait_ready: Send_Status INVALID_RESPONSE\n");
		}
		else {
			// currst = currstate_from_status(st);
			// if ( currst == 3 /* stby */ || currst == 4 /* trans */) {
			if ( status_ready_bit( st ) ) {
				res = 0xff;
			}
		}
	}
	while ( (res != 0xFF) && Timer2 );

	if ( Timer2 == 0 ) {
		//diskio_printf("wait_ready: timeout stby or trans\n");
	}

	return res;
}

// well, no SPI used here but will keep the interface compatible with
// Chan's examples
static void release_spi(void)
{
	//DESELECT();
}

static void power_off (void)
{
	SELECT();
	wait_ready();
	release_spi();

	//MCI_Power_Off();

	Stat |= STA_NOINIT;		/* Set STA_NOINIT */
}

#if 0
// must be in trans state - only used during init
// reads the SD Status ( do not confuse with Status Register )
static DWORD MCI_get_sd_status(void)
{
	DWORD i;
#ifdef DISKIO_DEBUG
	DWORD j;
#endif

	if ( MCI_Set_BlockLen( BLOCK_LENGTH ) == FALSE )
	{
		//diskio_printf("get_sd_status: Set_BlockLen SD_STATUS failed\n");
		return FALSE;
	}
	
	if ( MCI_Sd_Status() == FALSE ) {
		//diskio_printf("get_sd_status: MCI_Sd_Status failed\n");
		return FALSE;
	}
	
	Timer2 = 2000;
	while ( MCI_Block_End_Flag == 1 && Timer2 ) { ; }
	if ( Timer2 == 0 ) {
		//diskio_printf("get_sd_status: timeout waiting for data");
		return FALSE;
	}

#ifdef DISKIO_DEBUG
	//diskio_printf("SD-Status (MS-Bytes):\n");
	j=0;
	// for ( i=0; i<64; i++ ) {
	for ( i=0; i<16; i++ ) {
		//diskio_printf("%02d:%02x ", i, ReadBlock[i] );
		if ( j++ == 10 ) {
			//diskio_printf("\n");
			j = 0;
		}
	}
	//diskio_printf("\n");

	//diskio_printf("DAT_BUS_WIDTH : %02x = ", ReadBlock[0]>>6 );
	switch ( ReadBlock[0]>>6 ) {
		case 0:
			//diskio_printf("1 (default)");
			break;
		case 1:
			//diskio_printf("reserved1");
			break;
		case 2:
			//diskio_printf("4-bit width");
			break;
		case 3:
			//diskio_printf("reserved2");
			break;
	}
	//diskio_printf("\n");
#endif

	for ( i=0; i<16; i++ ) {
		sd_status[i] = ReadBlock[i];
	}
	
	return TRUE;
}
#endif

unsigned int uiSD_Initialize(void){    
    unsigned int ulOk;
    #if MCI_DMA_ENABLED
      /* on DMA channel 0, source is memory, destination is MCI FIFO. */
      /* On DMA channel 1, source is MCI FIFO, destination is memory. */
      DMA_Init();
    #endif
    ulOk=1;
    
    /* For the SD card I tested, the minimum required block length is 512 */
    /* For MMC, the restriction is loose, due to the variety of SD and MMC
    card support, ideally, the driver should read CSD register to find the 
    right speed and block length for the card, and set them accordingly.
    In this driver example, it will support both MMC and SD cards, and it
    does read the information by send SEND_CSD to poll the card status,
    however, to simplify the example, it doesn't configure them accordingly
    based on the CSD register value. This is not intended to support all 
    the SD and MMC cards. */
    if (ulOk&&( MCI_Init() != TRUE ))
    {
        ulOk=0;
    }

    MCI_CardType = MCI_CardInit();

    if (ulOk&&( MCI_CardType == CARD_UNKNOWN ))
    {
        ulOk=0;
    }

    if (ulOk&&(MCI_Check_CID() != TRUE))
    {
        ulOk=0;
    }

    if (ulOk&&( MCI_Set_Address() != TRUE ))
    {
        ulOk=0;
    }

    if (ulOk&&( MCI_Send_CSD() != TRUE ))
    {
        ulOk=0;
    }

    if (ulOk&&( MCI_Select_Card() != TRUE ))
    {
        ulOk=0;
    }

    if (ulOk&&( MCI_CardType == SD_CARD ))
    {
        MCI_Set_MCIClock( NORMAL_RATE );
        if (SD_Set_BusWidth( SD_4_BIT ) != TRUE )
        {
            ulOk=0;
        }
    }

    if (ulOk&&( MCI_Set_BlockLen( BLOCK_LENGTH ) != TRUE ))
    {
        ulOk=0;
    }
    return ulOk;
}    


#if 0
static int MCI_disk_initialize(void)
{
	int err = 0, i;
	DWORD csddw[4];

	//diskio_printf("\n");
#if MCI_DMA_ENABLED
	/* on DMA channel 0, source is memory, destination is MCI FIFO. */
	/* On DMA channel 1, source is MCI FIFO, destination is memory. */
	DMA_Init();
	//diskio_printf("MCI DMA enabled\n");
#endif

	if ( MCI_Init() != TRUE )
	{
		//diskio_printf("MCI_Init failed\n");
		err++; /* fatal error */
	}
	
	if ( !err ) 
	{
		MCI_CardType = MCI_CardInit();
		if ( MCI_CardType == CARD_UNKNOWN )
		{
			//diskio_printf("unknown card\n");
			err++; /* fatal error */
		}
	}

	if ( err || MCI_Check_CID() == FALSE )
	{
		//diskio_printf("Check_CID failed\n");
		err++; /* fatal error */
	}
	
	if ( err || MCI_Set_Address() == FALSE )
	{
		//diskio_printf("Set_Address failed\n");
		err++; /* fatal error */
	}
	
	if ( err || MCI_Send_CSD(  ) == FALSE )
	{
		//diskio_printf("Send_CSD failed\n");
		err++;
	}
	else 
	{
		//diskio_printf("diskio: CSD %08x %08x %08x %08x\n",csddw[0], csddw[1], csddw[2], csddw[3]);
		for ( i=0; i<4; i++ ) {
			DWORD_TO_BYTES( &csd[i*4], csddw[i] );
		}
	}
	
	if ( err || MCI_Select_Card() == FALSE )
	{
		//diskio_printf("Select_Card failed\n");
		err++;
	}
	
	if ( !err ) 
	{
		if ( ( MCI_CardType == SD_CARD )  )
		{
			MCI_Set_MCIClock( NORMAL_RATE );
			if (SD_Set_BusWidth( SD_4_BIT ) != TRUE )
			{
				//diskio_printf("set 4 bit mode failed\n");
				err++;
			}
		}
	}
	
	
	if ( err || MCI_Set_BlockLen( BLOCK_LENGTH ) == FALSE )
	{
		//diskio_printf("Set_BlockLen failed\n");
		err++;
	}
	//diskio_printf("Set_BlockLen o.k.\n");

	//if (!err) diskio_printf("MMC_disk_initialize ok\n");

	return err;
}
#endif

/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */

DSTATUS disk_initialize (BYTE drv){
	if ( drv != 0 ) return STA_NOINIT;

	if ( !uiSD_Initialize() ) {
		// error occured 
		Stat |= STA_NOINIT;
		power_off();
	}
	else {
		Stat &= ~STA_NOINIT;
		// cardtype mci.h to chan cardtype - (maybe redundant)
		switch ( MCI_CardType ) {
            //case SDHC_CARD :
            //	CardType = 4;
            //	break;
            
            case MMC_CARD :
                CardType = 1;
                break;
            case SD_CARD :
                CardType = 2;
                break;
            default :
                CardType = 0;
        }
	}
	
	return Stat;
}



/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */

DSTATUS disk_status (BYTE drv){		        /* Physical drive number (0..) */
	if ( drv != 0 ) return STA_NOINIT;
	return Stat;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */

DRESULT disk_read (
	BYTE drv,		/* Physical drive number (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector number (LBA) */
	BYTE count		/* Sector count (1..255) */
)
{
    TipoStruct_timeout timeout_disk_read;

	// DRESULT res;
	//WORD k;

	if (drv || !count) return RES_PARERR;
	if (Stat & STA_NOINIT) return RES_NOTRDY;

	// done in MCI driver if (!(CardType & 4)) sector *= 512;	/* Convert to byte address if needed */

	/* Select the card and wait for ready */
	/* mthomas - not sure if this is needed for SD */
	//DESELECT();
	//if (wait_ready() != 0xFF) return (DRESULT)(0xFF);
#ifndef DISKIO_OPTIMIZE_SELECT    
	SELECT();
#endif    

	// TODO: Multi-block
	do {
		//diskio_printfr("Read_Block start...\n");
		if ( MCI_Read_Block( sector ) != TRUE ) {
			//diskio_printf("MCI_Read_Block failed\n");
            v_generate_diag_alarm(enum_diag_alarm_error_read_block);
			break;
		}
		else {
            timeout_init(&timeout_disk_read);
			
			//diskio_printfr("Read_Block wait...\n");
			/* When MCI_Block_End_Flag is clear, it indicates RX is done */
			while ( MCI_Block_End_Flag == 1 ) { 
                if (uc_timeout_active(&timeout_disk_read, 5000)){
                    v_generate_diag_alarm(enum_diag_alarm_timeout_read_block);
                    break;
                }
            }
			//diskio_printfr("Read_Block end wait\n");
			// laebi: memcpy is faster than the for-Loop
			// TODO: avoid copy
			memcpy(buff, &ReadBlock[0], 512);
			// mziegert 20090823: Bugfix, memcpy does not increase buff pointer
			buff = &buff[512];
			sector++;
		}
	} while ( --count );
	
	release_spi();

	return count ? RES_ERROR : RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */

DRESULT disk_write (
	BYTE drv,			/* Physical drive number (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector number (LBA) */
	BYTE count			/* Sector count (1..255) */
)
{
    TipoStruct_timeout timeout_disk_write;

	if (drv || !count)
		return RES_PARERR;
	if (Stat & STA_NOINIT)
		return RES_NOTRDY;
	if (Stat & STA_PROTECT)
		return RES_WRPRT;

	// done in mci-driver: if (!(CardType & 4)) sector *= 512;	/* Convert to byte address if needed */

	/* Select the card and wait for ready */
	/* mthomas - not sure if this is needed for SD */
	//DESELECT();
	//if (wait_ready() != 0xFF) return (DRESULT)0xFF;
#ifndef DISKIO_OPTIMIZE_SELECT    
	SELECT();
#endif
	//do {
	//diskio_printfw("Write_Block %d start...\n", sector );
	if (count > 1) {
		// Multi Block Write
		if (MCI_Write_Multiple_Block(sector, count, buff) != TRUE) {
			//diskio_printf("MCI_Write_Multiple_Block failed\n");
            v_generate_diag_alarm(enum_diag_alarm_error_write_block);
			return RES_ERROR;
			//break;
		} else {
            timeout_init(&timeout_disk_write);
        
			//diskio_printfw("Write_Block Multi wait...\n");
			/* When MCI_Block_End_Flag is clear, it indicates TX is done */
			while (MCI_Block_End_Flag == 1) {
                if (uc_timeout_active(&timeout_disk_write, 10000)){
                    v_generate_diag_alarm(enum_diag_alarm_timeout_write_block);
                    break;
                }
			}
			//diskio_printfw("Write_Block Multi end wait\n");
		}
	} else {
		// Single Block write
		// TODO: avoid copy
		memcpy(WriteBlock, buff, 512);
		if (MCI_Write_Block(sector) != TRUE) {
			//diskio_printf("MCI_Write_Block failed\n");
            v_generate_diag_alarm(enum_diag_alarm_error_write_block);
			return RES_ERROR;
			//break;
		} else {
            timeout_init(&timeout_disk_write);
			//diskio_printfw("Write_Block wait...\n");
			/* When MCI_Block_End_Flag is clear, it indicates TX is done */
			while (MCI_Block_End_Flag == 1) {
                if (uc_timeout_active(&timeout_disk_write, 15000)){
                    v_generate_diag_alarm(enum_diag_alarm_timeout_write_block);
                    break;
                }
			}
			//diskio_printfw("Write_Block end wait\n");
		}
	}

	// laebi: not necessary
	//release_spi();

	//return count ? RES_ERROR : RES_OK;
	return RES_OK;
}




/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */

DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;

	//DWORD csize;

	if (drv) return RES_PARERR;
	if (Stat & STA_NOINIT) return RES_NOTRDY;

	res = RES_ERROR;

	switch (ctrl) {

		case CTRL_SYNC :	/* Flush dirty buffer if present */
#ifndef DISKIO_OPTIMIZE_SELECT    
			SELECT();
#endif        
			if (wait_ready() == 0xFF) {
				res = RES_OK;
			}
			break;

		case GET_SECTOR_SIZE :	/* Get sectors on the disk (WORD) */
            *(DWORD*)buff=SD.ulSizeOfBlocks;          
			//*(WORD*)buff = 512;
			res = RES_OK;
			break;

		case GET_SECTOR_COUNT :	/* Get number of sectors on the disk (WORD) */
           *(DWORD*)buff =SD.ulNumberOfBlocks;
            #if 0
                if ((csd[0] >> 6) == 1) {	/* SDC ver 2.00 */
                    csize = csd[9] + ((WORD)csd[8] << 8) + 1;
                    *(DWORD*)buff = (DWORD)csize << 10;
                } 
                else {					/* MMC or SDC ver 1.XX */
                    n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
                    csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
                    *(DWORD*)buff = (DWORD)csize << (n - 9);
                }
            #endif
			res = RES_OK;
			break;

		
		case GET_BLOCK_SIZE :	/* Get erase block size in unit of sectors (DWORD) */
            *(DWORD*)buff=SD.ulSizeOfBlocks;          
			res = RES_OK;
            #if 0
                if (CardType & 4) {   /* SDC ver 2.00 - use cached  Read SD status */
                    // TODO - untested!
                    *(DWORD*)buff = 16UL << (sd_status[10] >> 4);
                    res = RES_OK;
                } 
                else {   /* SDC ver 1.XX or MMC */
                    if (CardType & 2) {			/* SDC ver 1.XX */
                        *(DWORD*)buff = (((csd[10] & 63) << 1) + ((WORD)(csd[11] & 128) >> 7) + 1) << ((csd[13] >> 6) - 1);
                    } else {					/* MMC */
                        *(DWORD*)buff = ((WORD)((csd[10] & 124) >> 2) + 1) * (((csd[11] & 3) << 3) + ((csd[11] & 224) >> 5) + 1);
                    }
                    res = RES_OK;
                }
            #endif
			break;

		default:
			res = RES_PARERR;
	}

	release_spi(); 

	return res;
}


/*-----------------------------------------------------------------------*/
/* Device Timer Interrupt Procedure  (Platform dependent)                */
/*-----------------------------------------------------------------------*/
/* This function must be called in period of 1ms                         */

void disk_timerproc (unsigned int uiPeriodTimerMs){
	DWORD n;

	n = Timer1;						/* 1000Hz/5 decrement timer */
	if (n>uiPeriodTimerMs) 
        Timer1 = n-uiPeriodTimerMs;
    else
        Timer1 =0;
    
	n = Timer2;
	if (n>uiPeriodTimerMs) 
        Timer2 = n-uiPeriodTimerMs;
    else
        Timer2 =0;
}

// array che contiene indicatori che dicono se alcuni files sono stati modificati...
unsigned char ucFlagsFileModified[enumNumOfFileTypes];


// mi dice se un certo tipo di file � stato modificato...
unsigned char ucHasBeenFile_Modified(enumFileTypes fileType, enumFlagsFileModified enumFlag){
	if (fileType<sizeof(ucFlagsFileModified)/sizeof(ucFlagsFileModified[0])){
		if (enumFlag==enumFlagsFileModified_All)
			return ucFlagsFileModified[fileType]?1:0;
		else
			return (ucFlagsFileModified[fileType]&(1<<enumFlag))?1:0;
	}
	return 0;
}

// set/reset che un certo tipo di file � stato modificato...
unsigned char ucSignalFile_Modified(enumFileTypes fileType, unsigned char ucSet1_Reset0, enumFlagsFileModified enumFlag){
	if (fileType<sizeof(ucFlagsFileModified)/sizeof(ucFlagsFileModified[0])){
		if (ucSet1_Reset0){
			if (enumFlag==enumFlagsFileModified_All)
				ucFlagsFileModified[fileType]=0xFF;
			else
				ucFlagsFileModified[fileType]|=1<<enumFlag;
		}
		else{
			if (enumFlag==enumFlagsFileModified_All)
				ucFlagsFileModified[fileType]=0x00;
			else
				ucFlagsFileModified[fileType]&=~(1<<enumFlag);
		}
	}
	return 1;
}


