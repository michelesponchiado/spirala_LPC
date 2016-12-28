/****************************   Header File   **********************************

   Filename:    C2194.h
   Description: Contains interface, return codes and macro for the LLD (Low
                Level Driver) of the NAND flash (528 byte/264 word)
                small page family

   Author: STMicroelectronics
   Copyright:(C)STMicroelectronics
   Version:  Release 1.0
  
   You have a license to reproduce, display,perform,produce derivative works of,
   and distribute (in original or modified form) the Program, provided that you
   explicitly agree to the following disclaimer:

   THIS PROGRAM IS PROVIDED "AS IT IS" WITHOUT WARRANTY OF ANY KIND, EITHER
   EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO, THE IMPLIED WARRANTY
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK
   AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE
   PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,
   REPAIR OR CORRECTION.
********************************************************************************
********************************************************************************

   Version History.

 
   Ver.   Date        Comments

   0.1  12/11/2003  Initial Release of the software (Alpha).
   0.3  26/01/2004  First Release
   0.5  23/05/2004  Fixed bugs
   0.6  05/10/2004  Added extended signature and length in cache read
   0.7  27/10/2004  Added DMA management
   0.8  03/11/2004  Resolved RDO bug,Resolved RDI bug
   0.9  23/05/2005  new signature of CopyBack function - add 2 new macro
   1.0  09/08/2005  Release of the software (Full Version)
   1.2  09/02/2007  Updated address functions
   1.4  04/12/2007  bug fixed

*******************************************************************************/

                           
/*******************************************************************************
    ABOUT THE DRIVER:     

This file contains the interface, the return codes and the macro for the driver 
of the NAND flash 528 byte/264 word page family. All the 528 byte/264 word page
NAND devices are supported from this driver by setting correctly some defines. 
This driver is designed on two layer: basic function layer and NAND command
function layer. The interface between the NAND and the CPU depends strictly
on the NAND controller or on the particular connection used for handling
NAND signals (often a glue logic is used). Then, the driver needs some porting
operation to be used on a specific platform. All the porting operations is
realized by adapting the basic functions to specific hardware.
The following points describe the two layer:

  1 - Basic functions: these functions give an abstraction layer of the 
      socket layer.They are hardware dependent.
      
      Name                        |Description
      ---------------------------------------------------------------------
      void NAND_Open(...)         | Open a command session
      void NAND_CommandInput(...) | Issue a command to the flash
      void NAND_AddressInput(...) | Issue an address to the flash
      void NAND_DataInput(...)    | Issue a data to the flash
      ubyte NAND_DataOutput(...)  | Get a data from the flash
      void NAND_Close(void)       | Close a command session
      ubyte waitForReady()        | Wait for NAND ready
      _____________________________________________________________________
           
      
  2 - NAND functions: these functions are used to execute a NAND command. 
      They are hardware indipendent because of the basic function layer.
      
      Name                                   |Description
      ---------------------------------------------------------------------
      NAND_Ret NAND_BlockErase(...)          | Start a Blockerase command
      NAND_Ret NAND_CopyBack(...)            | Start a CopyBack command
      NAND_Ret NAND_PageRead(...)            | Start a PageRead command
      NAND_Ret NAND_PageProgram(...)         | Start a PageProgram command
      NAND_Ret NAND_SpareProgram(...)        | Start a SpareProgram command
      void NAND_ReadElectronicSignature(...) | Start a ReadElectronicSignature command
      ubyte NAND_ReadStatusRegister(...)     | Start a ReadStatus Register command
      void NAND_Reset(...)                   | Start a Reset command                      
      NAND_Ret NAND_SpareRead(...)           | Start a SpareRead command
      _____________________________________________________________________
   
      You can read the NAND Flash datasheet to understand the behavioral of
      these functions.
      
      Some functions return a code to indicate the result of the operation. The
      following codes are possbile:

      Codes                      |Description
      ---------------------------------------------------------------------------
      NAND_PASS                  | The operation are successfully executed
      NAND_FAIL                  | The operation are not successfully executed,
                                 | NAND device returned an internal error
      NAND_FLASH_SIZE_OVERFLOW   | The address is outside the NAND range
      NAND_PAGE_OVERFLOW         | Attempt to program a buffer larger than a page
      NAND_WRONG_ADDRESS         | The address is inside the NAND range but it
                                 | doesn't respect the address constraints
      ___________________________________________________________________________
  



The example showed above refers to an ARM7 MPU configured to map only one NAND
device at the address 0xCFFD0. Take in consideration that in this case (ARM7)
the base address for the device is chosen considering the glue logic used between
the MPU and the NAND device(the address pin of the MPU are used to set the logic!)
The driver support all the 512Mbit NAND devices by setting the following define:
       
   Example:
 ___________________________________________________________________________
   #define NAND512RW3A
          NAND512R(W)3A x8 device
          NAND512R(W)4A x16 device   

The driver support both DMA operations and Write protect pin controlling by
setting the following device:

      #define WRITE_PROTECT_ENABLE
      #define DMA_ENABLE
   
After you launch a NAND command you can wait the end of the operation by polling
the status register (In this case the operation is a blocking call).
The basic datatype of this driver refers to the standard STFL-I:

         typedef unsigned char ubyte;
         typedef signed char byte;
         typedef unsigned short uword;
         typedef signed short word;
         typedef unsigned int udword;
         typedef signed int dword;

*******************************************************************************/


#ifndef __NAND512__H__
#define __NAND512__H__

/* Basic Data-type*/
typedef unsigned char        ubyte;
typedef signed char          byte;
typedef unsigned short       uword;
typedef signed short         word;
typedef unsigned int         udword;
typedef signed int           dword;

// #define NAND512RW4A   /* device selected */

          /* List of small family device supported */
/* #define NAND512RW3A //x8 device            1.8V 0r 3.3V */
/* #define NAND512RW4A //x16 device           1.8V 0r 3.3V */
/* #define NAND256RW3A //x8 device            1.8V 0r 3.3V */
/* #define NAND256RW4A //x16 device           1.8V 0r 3.3V */
/* #define NAND128RW3A //x8 device            1.8V 0r 3.3V */
/* #define NAND128RW4A //x16 device           1.8V 0r 3.3V */
// #define NAND01GRW3A //x8 1Gbit             1.8V 0r 3.3V */
#define NAND01GRW3B  // x8 1gbit, page size 2048+64 (2k data), 64 pagine/blocco=> 128k/block* 1024 blocks=1gbit
/* #define NAND01GRW4A //x16 1Gbit            1.8V 0r 3.3V */


//#define WRITE_PROTECT_ENABLE
//#define DMA_ENABLE

/* to do DMA define */

#ifdef WRITE_PROTECT_ENABLE
  #define WRITE_PROTECT_BIT 0x0
#endif

#ifndef WRITE_PROTECT_ENABLE
  #define WRITE_PROTECT_BIT 0x8
#endif

#define NUM_PAGE_BLOCK           64        /* Number of pages contains in a */
                                           /* physical block               */




#ifdef NAND128RW3A
   #define FLASH_WIDTH           8              /* Flash data width */
   #define FLASH_SIZE            0x1000000      /* Flash size in byte */
   #define PAGE_SIZE             528            /* Page size in byte */
   #define PAGE_DATA_SIZE        512            /* Page data size in byte */
   #define PAGE_SPARE_SIZE       16             /* Spare page size in byte */
   #define NUM_BLOCKS		     1024           /* Number of blocks in device*/
   #define HALF_PAGE_POINTER     (ubyte) 0x01   /* Half page Pointer */
   #define SHIFT_A8				 1
   typedef ubyte dataWidth;                     /* Flash data type */
#endif 


#ifdef NAND128RW4A
   #define FLASH_WIDTH           16             /* Flash data width */
   #define FLASH_SIZE            0x800000       /* Flash size in word */
   #define PAGE_SIZE             264            /* Page size in word */
   #define PAGE_DATA_SIZE        256            /* Page data size in word */
   #define PAGE_SPARE_SIZE       8              /* Spare page size in word */
   #define NUM_BLOCKS		     1024           /* Number of blocks in device*/
   #define HALF_PAGE_POINTER     (ubyte) 0x00   /* Half page Pointer */
   #define SHIFT_A8				 0
   typedef uword dataWidth;                     /* Flash data type */
#endif 


#ifdef NAND256RW3A
   #define FLASH_WIDTH           8              /* Flash data width */
   #define FLASH_SIZE            0x2000000      /* Flash size in byte */
   #define PAGE_SIZE             528            /* Page size  in byte */
   #define PAGE_DATA_SIZE        512            /* Page data size  in byte */
   #define PAGE_SPARE_SIZE       16             /* Spare page size in byte */
   #define NUM_BLOCKS		     2048           /*number of blocks in device*/
   #define HALF_PAGE_POINTER     (ubyte) 0x01   /* Half page Pointer */
   #define SHIFT_A8				 1
   typedef ubyte dataWidth;                     /* Flash data type */
#endif 


#ifdef NAND256RW4A
   #define FLASH_WIDTH           16             /* Flash data width */
   #define FLASH_SIZE            0x1000000      /* Flash size in word */
   #define PAGE_SIZE             264            /* Page size in word */
   #define PAGE_DATA_SIZE        256            /* Page data size  in word */
   #define PAGE_SPARE_SIZE       8              /* Spare page size in word */
   #define NUM_BLOCKS		     2048           /* Number of blocks in device*/
   #define HALF_PAGE_POINTER     (ubyte) 0x00   /* Half page Pointer */
   #define SHIFT_A8				 0
   typedef uword dataWidth;                     /* Flash data type */
#endif 


#ifdef NAND512RW3A
   #define FLASH_WIDTH           8              /* Flash data width */
   #define FLASH_SIZE            0x4000000      /* Flash size in byte */
   #define PAGE_SIZE             528            /* Page size in byte */ 
   #define PAGE_DATA_SIZE     	 512            /* Page data size in byte */
   #define PAGE_SPARE_SIZE       16             /* Spare page size in byte */
   #define NUM_BLOCKS		     4096           /* Number of blocks in device*/
   #define HALF_PAGE_POINTER     (ubyte) 0x01   /* Half page Pointer */
   #define SHIFT_A8				 1
   typedef ubyte dataWidth;                     /* Flash data type */
#endif 


#ifdef NAND512RW4A
   #define FLASH_WIDTH           16             /* Flash data width */
   #define FLASH_SIZE            0x2000000      /* Flash size in word */
   #define PAGE_SIZE             264            /* Page size in word */ 
   #define PAGE_DATA_SIZE        256            /* Page data size in word */
   #define PAGE_SPARE_SIZE       8              /* Spare page size in word */
   #define NUM_BLOCKS		     4096           /* Number of blocks in device*/
   #define HALF_PAGE_POINTER     (ubyte) 0x00   /* Half page Pointer */
   #define SHIFT_A8		 0
   typedef uword dataWidth;                     /* Flash data type */
#endif 


#ifdef NAND01GRW3A
   #define FLASH_WIDTH           8               /* Flash data width */
   #define FLASH_SIZE            0x8000000       /* Flash size in byte */
   #define PAGE_SIZE             528             /* Page size in byte */
   #define PAGE_DATA_SIZE        512             /* Page data size in byte */
   #define PAGE_SPARE_SIZE       16              /* Spare page size in byte */
   #define NUM_BLOCKS		     8192            /* Number of blocks in device*/
   #define HALF_PAGE_POINTER     (ubyte) 0x01    /* Half page Pointer */
   #define SHIFT_A8		 1
   typedef ubyte dataWidth;                     /* Flash data type */
#endif 

#ifdef NAND01GRW3B
   #define FLASH_WIDTH           8               /* Flash data width */
   #define FLASH_SIZE            0x8000000       /* Flash size in byte */
   #define PAGE_SIZE            (2048+64)        /* Page size in byte */
   #define PAGE_DATA_SIZE        2048             /* Page data size in byte */
   #define PAGE_SPARE_SIZE       64              /* Spare page size in byte */
   #define NUM_BLOCKS		      1024            /* Number of blocks in device*/
   #define HALF_PAGE_POINTER     (ubyte) 0x04    /* Half page Pointer */
   #define SHIFT_A8		 0				// nessuno shift di a8 nell'inserire addresses
   typedef ubyte dataWidth;                     /* Flash data type */
#endif 


#ifdef NAND01GRW4A
   #define FLASH_WIDTH           16             /* Flash data width */
   #define FLASH_SIZE            0x4000000      /* Flash size in word */
   #define PAGE_SIZE             264            /* Page size in word */
   #define PAGE_DATA_SIZE        256            /* Page data size in word */
   #define NUM_BLOCKS		     8192           /* Number of blocks in device*/
   #define PAGE_SPARE_SIZE       8              /* Spare page size in word */
   #define HALF_PAGE_POINTER     (ubyte) 0x00   /* Half page Pointer */
   #define SHIFT_A8		 1
   typedef ubyte dataWidth;                     /* Flash data type */
#endif 






/*******************************************************************************
			                      Utility
*******************************************************************************/
#define ADDRESS_2_BLOCK(Address)        (Address >> (13+HALF_PAGE_POINTER)) 
#define BLOCK_SIZE 	                    (PAGE_DATA_SIZE*NUM_PAGE_BLOCK)
#define BLOCK_2_ADDRESS(Num_block)      ((udword) (Num_block* BLOCK_SIZE))
/****************************** Utility ***************************************/


/*******************************************************************************
                           HARDWARE DEPENDENT LAYER
                                Basic functions
*******************************************************************************/

void NAND_Open(void);
void NAND_CommandInput(ubyte ubCommand);    /* Put a command on bus */
void NAND_AddressInput(ubyte ubAddress);    /* Put an address on bus */
void NAND_DataInput(dataWidth ubData);      /* Write a data to flash */
dataWidth NAND_DataOutput(void);            /* Read a data from the flash */
void NAND_Close(void);                      /* Close the command after an */
                                            /* operation is completed. */

/*************************  HARDWARE DEPENDENT LAYER **************************/
                                


/*******************************************************************************
			                    Return Codes
*******************************************************************************/
typedef ubyte NAND_Ret;

#define NAND_PASS                  0x00 /* the operation on the NAND was
                                            successfully completed*/
#define NAND_FAIL                  0x01 /* the operation on the nand failed */
#define NAND_FLASH_SIZE_OVERFLOW   0x02 /* the address is not within the device*/
#define NAND_PAGE_OVERFLOW         0x04 /* attempt to access more than one page*/
#define NAND_WRONG_ADDRESS         0x08 /* the address is not */

/******************************  Return Codes *********************************/


/******************************************************************************
                           HARDWARE INDIPENDENT LAYER
                            Nand operation functions
******************************************************************************/                             
 
NAND_Ret NAND_BlockErase(udword udAddress);
          
NAND_Ret NAND_CopyBack(udword udSourceAddr,udword udDestinationAddr);

NAND_Ret NAND_PageRead(udword udAddress, dataWidth *Buffer, udword udlength);

NAND_Ret NAND_PageProgram(udword udAddress, dataWidth *Buffer, udword udlength);

NAND_Ret NAND_SpareProgram(udword udAddress, dataWidth *Buffer, udword udlength);

void NAND_ReadElectronicSignature(dataWidth *ubBuffer);

ubyte NAND_ReadStatusRegister(void);

void NAND_Reset(void);
                       
NAND_Ret NAND_SpareRead(udword udAddress, dataWidth *Buffer, udword udlength);

/************************ HARDWARE INDIPENDENT LAYER **************************/

#endif //__NAND512__H__






