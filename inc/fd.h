/****************************************************************************
*****************************************************************************
**
** File Name
** ---------
**
** FD.H
**
*****************************************************************************
*****************************************************************************
**
** Description
** -----------
**
** Fundamental Definitions.
**
** Fundamental definitions of data types and constants
** that have scope across the multiple components.
**
** (This header file has NO corresponding "C" files).
**
*****************************************************************************
*****************************************************************************
**                                                                         **
** COPYRIGHT NOTIFICATION (c) 1998, 99    HMS Industrial Networks AB.      **
**                                                                         **
** This program is the property of HMS Industrial Networks AB.             **
** It may not be reproduced, distributed, or used without permission       **
** of an authorised company official.                                      **
**                                                                         **
*****************************************************************************
*****************************************************************************
**
** Company: HMS Industrial Networks AB
**          Pilefeltsgatan 93-95
**          S-302 50  Halmstad
**          SWEDEN
**          Tel:     +46 (0)35 - 17 29 00
**          Fax:     +46 (0)35 - 17 29 09
**          e-mail:  info@hms.se
**
*****************************************************************************
*****************************************************************************
*/

#ifndef FD_H
#define FD_H



/****************************************************************************
**
** Symbolic Constants
**
*****************************************************************************
*/

/*---------------------------------------------------------------------------
**
** FALSE
** TRUE
**
** These are the symbolic constants for true and false used in boolean
** data type comparisons.
**
**---------------------------------------------------------------------------
*/

// #define FALSE     0
// #define TRUE      ( !FALSE )


/****************************************************************************
**
** Fundamental Typedefs
**
*****************************************************************************
*/

/*---------------------------------------------------------------------------
**
** BOOL
**
** SINT8
** SINT16
** SINT32
**
** UINT8
** UINT16
** UINT32
**
** The standard boolean data type.
** The standard signed 8, 16, and 32 bit data types (respectively).
** The standard unsigned 8, 16, and 32 bit data types (respectively).
**
**---------------------------------------------------------------------------
*/

typedef unsigned char   BOOL;

typedef signed char     SINT8;
typedef signed short    SINT16;
typedef signed long     SINT32;

typedef unsigned char   UINT8;
typedef unsigned short  UINT16;
typedef unsigned long   UINT32;


#endif  /* inclusion lock */

/****************************************************************************
**
** End of FD.H
**
*****************************************************************************
*/
