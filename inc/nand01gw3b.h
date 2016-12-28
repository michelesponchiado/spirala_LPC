/*****************************************************************************

   Filename:    c2188.h
   Description: - Header File - Contains interface, return codes and macro for 
          the Software Drivers for  NAND flash 2112 byte/1056 word page family.

   Author: STMicroelectronics
   Copyright:(C)STMicroelectronics

   				                
 You have a license to reproduce, display, perform, produce derivative works of, 
 and distribute (in original or modified form) the Program, provided that you 
 explicitly agree to the following disclaimer:
   
   THIS PROGRAM IS PROVIDED "AS IT IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
   EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO, THE IMPLIED WARRANTY 
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK 
   AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE 
   PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, 
   REPAIR OR CORRECTION.
   
********************************************************************************

   Version History.

   Ver.		No	Date     Comments
   
   Alpha	0.1	10/2004  Initial Alpha Release of the driver
   Beta   	0.5	04/2005  Not all functions tested.
   Release	1.0 08/2005  First release
   Release  1.4 02/2008  Added Multiplane Device Support and CacheRead ONFI compliant

*******************************************************************************/   
                           
                          
                           

#ifndef _NAND2112_
#define _NAND2112_

/* Basic Data-type*/
typedef unsigned char        ubyte;
typedef signed char          byte;
typedef unsigned short       uword;
typedef signed short         word;
typedef unsigned long int    udword;
typedef signed long int      dword;

/* macro used to select the spare area in some 
   operations (PageProgram,PageRead, etc)   */ 
#define SPARE_LENGTH(x) (x|0x8000)   

/* macro used to select the main area in some 
   operations (PageProgram,PageRead, etc)  */ 
#define MAIN_LENGTH(x)  (x)         


 

//#define NAND512RW3B
//#define NAND512RW4B
#define NAND01GRW3B      
//#define NAND01GRW4B
//  #define NAND02GRW3B
//#define NAND02GRW4B
//#define NAND04GRW3B
//#define NAND04GRW4B
//#define NAND08GRW3B
//#define NAND08GRW4B

//#define DMA_ENABLE

//#define MULTIPLANE /*USE IF DEVICE SUPPORTS MULTIPLANE FUNCTION*/
#define ONFI	   /*USE IF DEVICE IS ONFI 1.0 COMPLIANT*/
   

#define NUM_PAGE_BLOCK           64             /* Number of pages for block*/

#ifdef NAND512RW3B
   #define FLASH_WIDTH           8              /* Flash data width */
   #define FLASH_SIZE            0x4000000      /* Flash size in byte */
   #define PAGE_SIZE             2112           /* Page size in byte */ 
   #define PAGE_DATA_SIZE     	 2048           /* Page data size in byte */
   #define PAGE_SPARE_SIZE		 64				/* Page spare size in bytes*/
   typedef ubyte dataWidth;                     /* Flash data type */
   #define SHIFT_A8				 1
   #define NUM_BLOCKS			 0x200          /* Number of blocks*/
#endif 

#ifdef NAND512RW4B
   #define FLASH_WIDTH           16             /* Flash data width */
   #define FLASH_SIZE            0x2000000      /* Flash size in words */
   #define PAGE_SIZE             1056           /* Page size in words */ 
   #define PAGE_DATA_SIZE     	 1024           /* Page data size in words */
   #define PAGE_SPARE_SIZE		 32				/* Page spare size in words*/
   typedef uword dataWidth;                     /* Flash data type */
   #define NUM_BLOCKS			 0x200          /* Number of blocks*/
   #define SHIFT_A8				 0
#endif 


#ifdef NAND01GRW3B
   #define FLASH_WIDTH           8              /* Flash data width */
   #define FLASH_SIZE            0x8000000      /* Flash size in byte */
   #define PAGE_SIZE             2112           /* Page size in byte */ 
   #define PAGE_DATA_SIZE     	 2048           /* Page data size in byte */
   #define PAGE_SPARE_SIZE		 64				/* Page spare size in bytes*/
   typedef ubyte dataWidth;                     /* Flash data type */
   #define NUM_BLOCKS			 0x400          /* Number of blocks*/
   #define SHIFT_A8				 1
#endif 

#ifdef NAND01GRW4B
   #define FLASH_WIDTH           16             /* Flash data width */
   #define FLASH_SIZE            0x4000000      /* Flash size in words */
   #define PAGE_SIZE             1056           /* Page size in words */ 
   #define PAGE_DATA_SIZE     	 1024           /* Page data size in words */
   #define PAGE_SPARE_SIZE		 32				/* Page spare size in words*/
   typedef uword dataWidth;                     /* Flash data type */
   #define NUM_BLOCKS			 0x400          /* Number of blocks*/
   #define SHIFT_A8				 0
#endif 

#ifdef NAND02GRW3B
   #define FLASH_WIDTH           8              /* Flash data width */
   #define FLASH_SIZE            0x10000000     /* Flash size in byte */
   #define PAGE_SIZE             2112           /* Page size in byte */ 
   #define PAGE_DATA_SIZE     	 2048           /* Page data size in byte */
   #define PAGE_SPARE_SIZE		 64				/* Page spare size in bytes*/
   typedef ubyte dataWidth;                     /* Flash data type */
   #define NUM_BLOCKS			 0x800 		    /* Number of blocks*/
   #define SHIFT_A8				 1
#endif 

#ifdef NAND02GRW4B
   #define FLASH_WIDTH           16             /* Flash data width */
   #define FLASH_SIZE            0x8000000      /* Flash size in words */
   #define PAGE_SIZE             1056           /* Page size in words */ 
   #define PAGE_DATA_SIZE     	 1024           /* Page data size in words */
   #define PAGE_SPARE_SIZE		 32				/* Page spare size in words*/
   typedef uword dataWidth;                     /* Flash data type */
   #define NUM_BLOCKS			 0x800          /* Number of blocks*/
   #define SHIFT_A8				 0
#endif 

#ifdef NAND04GRW3B
   #define FLASH_WIDTH           8              /* Flash data width */
   #define FLASH_SIZE            0x20000000     /* Flash size in byte */
   #define PAGE_SIZE             2112           /* Page size in byte */ 
   #define PAGE_DATA_SIZE     	 2048           /* Page data size in byte */
   #define PAGE_SPARE_SIZE		 64				/* Page spare size in bytes*/   
   typedef ubyte dataWidth;                     /* Flash data type */
   #define NUM_BLOCKS			 0x1000 		/* Number of blocks*/
   #define SHIFT_A8				 1
#endif 

#ifdef NAND04GRW4B
   #define FLASH_WIDTH           16             /* Flash data width */
   #define FLASH_SIZE            0x10000000     /* Flash size in words */
   #define PAGE_SIZE             1056           /* Page size in words */ 
   #define PAGE_DATA_SIZE     	 1024           /* Page data size in words */
   #define PAGE_SPARE_SIZE		 32				/* Page spare size in words*/
   typedef uword dataWidth;                     /* Flash data type */
   #define NUM_BLOCKS			 0x1000         /* Number of blocks*/
   #define SHIFT_A8				 0
#endif 

#ifdef NAND08GRW3B
   #define FLASH_WIDTH           8              /* Flash data width */
   #define FLASH_SIZE            0x40000000     /* Flash size in byte */
   #define PAGE_SIZE             2112           /* Page size in byte */ 
   #define PAGE_DATA_SIZE     	 2048           /* Page data size in byte */
   #define PAGE_SPARE_SIZE		 64				/* Page spare size in bytes*/   
   typedef ubyte dataWidth;                     /* Flash data type */
   #define NUM_BLOCKS			 0x2000 		/* Number of blocks*/
   #define SHIFT_A8				 1
#endif 

#ifdef NAND08GRW4B
   #define FLASH_WIDTH           16             /* Flash data width */
   #define FLASH_SIZE            0x20000000     /* Flash size in words */
   #define PAGE_SIZE             1056           /* Page size in words */ 
   #define PAGE_DATA_SIZE     	 1024           /* Page data size in words */
   #define PAGE_SPARE_SIZE		 32				/* Page spare size in words*/
   typedef uword dataWidth;                     /* Flash data type */
   #define NUM_BLOCKS			 0x2000         /* Number of blocks*/
   #define SHIFT_A8				 0
#endif 


#define CACHE_READ_PENDING       0x01
#define CACHE_READ_NOT_PENDING   0x00



/****************************************************************************
			Utility
****************************************************************************/                           
#define ADDRESS_2_BLOCK(Address)        (Address>>(16 + SHIFT_A8))  
#define BLOCK_SIZE 	                  (PAGE_DATA_SIZE*NUM_PAGE_BLOCK)
#define BLOCK_2_ADDRESS(Num_block)      ((udword) (Num_block<<(16 + SHIFT_A8)))
/**************  Utility ****************************************************/ 


/****************************************************************************
			Hardware dependent function
****************************************************************************/
void NAND_Open(void);
void NAND_CommandInput(ubyte ubCommand); /* Put a command on bus*/
void NAND_AddressInput(ubyte ubAddress); /* Put an address on bus*/
void NAND_DataInput(dataWidth ubData);   /* Write a data to flash*/
dataWidth NAND_DataOutput(void);         /* Read a data from the flash*/   
void NAND_Close(void);
void NAND_SetWriteProtect(void);         /* Set WP pint to Low*/
void NAND_UnsetWriteProtect(void);       /* Set WP pint to High*/
void NAND_WaitTime(ubyte nanoseconds);   /* Wait time */

/**************  Hardware dependent function ****************/                         
                                
                             




/****************************************************************************
			Return Codes
****************************************************************************/
typedef ubyte NAND_Ret;

#define NAND_PASS                0x00  /* the operation on the NAND was 
                                          successfully completed*/
#define NAND_FAIL                0x01  /* the operation on the nand failed */
#define NAND_FLASH_SIZE_OVERFLOW 0x02  /* the address is not within the device*/
#define NAND_PAGE_OVERFLOW       0x04  /* attempt to access more than one page*/
#define NAND_WRONG_ADDRESS       0x08  /* the address is not */
#define NAND_DIFFERENT_PAGES     0x10  /* page different in datainput command*/
#define NAND_WRITE_PROTECTED	 0x80  /* device is wite protected */
#define NAND_UNLOCKED_BLOCK		 0x09  /* block unlocked*/
#define NAND_LOCKED_BLOCK		 0x06  /* block locked*/
#define NAND_LOCK_DOWN		     0x05  /* block lock-down*/
#define CACHE_READ_NOT_POSSIBLE  0x07  /* required cache read dataoutput with  cache read not pending */
#define OPERATION_NOT_ALLOWED    0x55  /*Operation is not permitted on this device*/

					/*Multiplane Return Codes*/
#ifdef MULTIPLANE
#define NAND_FIRSTPLANE_FAIL	 0x11  /*the operation failed on first plane*/
#define NAND_SECONDPLANE_FAIL	 0X12  /*the operation failed on second plane*/
#define NAND_BOTHPLANE_FAIL		 0x13  /*the operation failed on both planes*/
#define NAND_EDC_OK				 0x20  /*no EDC error after copyback or multiplanecopyback */
#define NAND_EDC_INVALID		 0x21  /*EDC is invalid and cannot be computed*/
#define NAND_EDC_ERROR			 0x22  /*EDC error after copyback or multiplane copyback*/
#endif

/**************  Return Codes ***************/ 



/****************************************************************************
			Hardware independent function
****************************************************************************/
NAND_Ret NAND_BlockErase(udword udAddress);
NAND_Ret NAND_CacheProgram(udword udPageAddresses[],ubyte piecesNumber, dataWidth **Buffers, uword udlength[],ubyte* errorPage);
NAND_Ret NAND_CopyBack(udword udSourceAddr, udword udDestinationAddr,uword offsetInPage,uword uiNumByte2Copy, dataWidth xdata *pBytes2copy);
NAND_Ret NAND_PageRead(udword udAddresses, dataWidth xdata *Buffer,uword udLength) ;
NAND_Ret NAND_PageProgram(udword udAddresses, dataWidth xdata *Buffer, uword udlength);
void NAND_ReadElectronicSignature(dataWidth *ubBuffer);
void NAND_Reset(void);
NAND_Ret NAND_SpareProgram(udword udAddress, dataWidth *Buffer,udword udlength);
NAND_Ret NAND_SpareRead(udword udAddress, dataWidth *Buffer, udword udlength);
void NAND_Lock(void);
void NAND_LockDown(void);
NAND_Ret NAND_UnLock(udword startBlock,udword endBlock);
void NAND_UnlockDown(void);
NAND_Ret NAND_ReadBlockLockStatus(udword address);
ubyte NAND_ReadStatusRegister(void);
NAND_Ret NAND_CacheReadRandomDataOutput(udword udAddress,dataWidth* Buffer,udword length);
NAND_Ret NAND_CacheRead(udword udAddress, dataWidth *Buffer, udword length);
void NAND_Terminate_CacheRead(void);
NAND_Ret NAND_CacheReadDataOutput(dataWidth* Buffer,udword length);

/*Multiplane function*/
#ifdef MULTIPLANE 
NAND_Ret NAND_MultiplanePageProgram(udword *FPudAddresses, dataWidth **FPBuffer,ubyte FPnumOfChunks, uword *FPudlength,udword *SPudAddresses, dataWidth **SPBuffer,ubyte SPnumOfChunks, uword *SPudlength);
NAND_Ret NAND_MultiplaneBlockErase(udword FPudAddress,udword SPudAddress);
NAND_Ret NAND_MultiplaneCopyBack(udword FPudSourceAddr, udword FPudDestinationAddr,uword *FPoffsetInPage,uword *FPchunkSizes, uword FPnumOfChunks,dataWidth** FPBuffers,udword SPudSourceAddr, udword SPudDestinationAddr,uword *SPoffsetInPage,uword *SPchunkSizes, uword SPnumOfChunks,dataWidth** SPBuffers);
ubyte NAND_ReadStatusEnhanced(udword planeAddress);
ubyte NAND_ReadEDCStatusRegister(void); 
#endif





/**************  Hardware independent function ***************/




#endif

// definizioni per gestione ecc codes

typedef unsigned char u_char;
typedef unsigned char uint8_t;
typedef unsigned long int uint32_t;


// struttura di gestione errori ecc
typedef struct _TipoStructErrors{
	unsigned char ucBefore,ucAfter;
	unsigned char ucBit;
	unsigned int uiByteNumber;
}TipoStructErrors;

typedef struct _TipoParsNandCorrect{
	u_char xdata *dat;
	u_char xdata *read_ecc;
	u_char xdata *calc_ecc;
	TipoStructErrors xdata *perr;
}TipoParsNandCorrect;


extern int nand_correct_data(TipoParsNandCorrect xdata *p);


