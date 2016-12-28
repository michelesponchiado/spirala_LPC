/*******************************************************************************
********************************************************************************
**
** File Name
** ---------
**
** ABIC.H
**
********************************************************************************
********************************************************************************
**
** Description
** -----------
** Header file for AnyBus-IC access functions.
**
**
********************************************************************************
********************************************************************************
**
** Services List
** -------------
**
** Public Services:
**
**    ABIC_AutoBaud()         - Sends messages to the AnyBus-IC to make it
**                              auto detect the baudrate
**    ABIC_NormalMode()       - Sets the AnyBus-IC in Normal Operational mode
**    ABIC_ReadOutData()      - Reads Output data FROM the AnyBus-IC
**    ABIC_WriteInData()      - Writes Input data TO the AnyBus-IC
**
**
********************************************************************************
********************************************************************************
**                                                                            **
** COPYRIGHT NOTIFICATION (c) 1995,96,97,98,99 HMS Industrial Networks AB.    **
**                                                                            **
** This program is the property of HMS Industrial Networks AB.                **
** It may not be reproduced, distributed, or used without permission          **
** of an authorised company official.                                         **
**                                                                            **
********************************************************************************
********************************************************************************
**
** Company: HMS Industrial Networks AB
**          Pilefeltsgatan 93-95
**          S-302 50  Halmstad
**          SWEDEN
**          Tel:     +46 (0)35 - 17 29 00
**          Fax:     +46 (0)35 - 17 29 09
**          e-mail:  info@hms.se
**
********************************************************************************
********************************************************************************
*/

/*******************************************************************************
********************************************************************************
**
** Change Log
** ----------
**
** Latest Revision:
**
**    Rev 0.10    13 jun 2002   Created
**    Rev 1.00    12 jul 2002   First Release
**    Rev 1.01    28 mar 2006   replacement of type bool by BOOL
**
********************************************************************************
********************************************************************************
*/

#ifndef ABIC_H
#define ABIC_H


/*******************************************************************************
**
** Public Services Definitions
**
********************************************************************************
*/

/*------------------------------------------------------------------------------
**
** ABIC_AutoBaud()
**
** The AnyBus-IC is able to auto detect the Baud rate on the
** SCI channel by measuring the bit time when the Modbus Master
** sends a special byte sequence. The byte sequence the master
** has to send is 0x01 0x03.
** By sending a the Modbus Command "Read Holding Register" to
** Modbus Node #1 the Modbus message will start with the needed
** Byte sequence. When the baud rate is detected the AnyBus will
** simply answer the request and when we receive a response
** we know that it has the right baud rate.
** This function does 10 attempts to make the AnyBus-IC auto detect
** the baud rate.
**
**------------------------------------------------------------------------------
**
** Inputs:
**    None
**
** Outputs:
**    Return      - TRUE = SUCCESS
**                - FALSE = FAILED
**
** Usage:
**   fResult = ABIC_AutoBaud();
**
**------------------------------------------------------------------------------
*/

BOOL ABIC_AutoBaud( void );


/*------------------------------------------------------------------------------
**
** ABIC_NormalMode()
**
** This function sends modbus command 0x06 ( Preset Single Register )
** to set Parameter #1 ( Modbus address 0x5001 ) to set the AnyBus-IC
** in Normal Operation Mode
**
**------------------------------------------------------------------------------
**
** Inputs:
**   None
**
** Outputs:
**   Return      - TRUE = SUCCESS
**               - FALSE = FAILED
**
** Usage:
**   fResult = ABIC_NormalMode();
**
**------------------------------------------------------------------------------
*/

//bool ABIC_NormalMode (void);
BOOL ABIC_NormalMode (void);		// BEG, 28.03.06

/*------------------------------------------------------------------------------
**
** ABIC_ReadOutData()
**
** The AnyBus-IC SCI Out Data Area contains IO data FROM the fieldbus or
** the AnyBus-IC SCC interface depending on configuration. This data
** can be read at address 0x1000 in the AnyBus-IC Modbus space.
** This function will send modbus command 0x04 ( Read Input Registers ) to
** read data in the SCI Out Data Area.
**
**------------------------------------------------------------------------------
**
** Inputs:
**    bOffset     - Address offset ( in 32-bit Words, Modbus uses word access )
**    bSize       - Amount of data to read ( in 32-bit Words )
**    pData       - Pointer to where to put the read data
**
** Outputs:
**   Return      - TRUE = SUCCESS
**               - FALSE = FAILED
**
** Usage:
**    ABIC_ReadOutData( 0, 2, abOutDataBuffer );
**
**------------------------------------------------------------------------------
*/

BOOL ABIC_ReadOutData( UINT8 bOffset, UINT8 bSize, UINT8* pData );


/*------------------------------------------------------------------------------
**
** ABIC_WriteInData()
**
** The AnyBus-IC SCI In Data Area contains IO data TO the fieldbus or
** the AnyBus-IC SCC interface depending on configuration. This data
** can be written at address 0x0000 in the AnyBus-IC Modbus space.
** This fuction will send Modbus Command 0x10 ( Preset Multiple Regisers )
** to write data in the SCI In Data Area.
**
**------------------------------------------------------------------------------
**
** Inputs:
**    bOffset     - Address offset ( in 32-bit Words, Modbus uses word access )
**    bSize       - Amount of data to write ( in 32-bit Words )
**    pData       - Pointer to the Data
**
** Outputs:
**   Return      - TRUE = SUCCESS
**               - FALSE = FAILED
**
** Usage:
**    ABIC_WriteData( 0, 2, abInDataBuffer );
**
**------------------------------------------------------------------------------
*/

BOOL ABIC_WriteInData( UINT8 bOffset, UINT8 bSize, UINT8* pData );


#endif  /* inclusion lock */

/*******************************************************************************
**
** End of ABIC.H
**
********************************************************************************
*/