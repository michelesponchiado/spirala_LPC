/*******************************************************************************
********************************************************************************
**
** File Name
** ---------
**
** TM.H
**
********************************************************************************
********************************************************************************
**
** Description
** -----------
** Header file for the gereral timer functions
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
**    TM_SetTimer          -  Sets the timer timeout time
**    TM_StartTimer        -  Starts a CPU timer
**    TM_StopTimer         -  Stops the timer
**    TM_TimeOut           -  Check if the timer has timed out yet
**
**
********************************************************************************
********************************************************************************
**                                                                            **
** COPYRIGHT NOTIFICATION (c) 1995,96,97,98,99 HMS Fieldbus Systems AB.       **
**                                                                            **
** This program is the property of HMS Fieldbus Systems AB.                   **
** It may not be reproduced, distributed, or used without permission          **
** of an authorised company official.                                         **
**                                                                            **
********************************************************************************
********************************************************************************
**
** Company: HMS Fieldbus Systems AB
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
**
********************************************************************************
********************************************************************************
*/

#ifndef TM_H
#define TM_H


/*******************************************************************************
**
** Public Globals
**
********************************************************************************
*/

/*------------------------------------------------------------------------------
**
** TM_iTimeOutTime
**
**------------------------------------------------------------------------------
**
** Current timer timeout time
**
**------------------------------------------------------------------------------
*/

extern UINT16 TM_iTimeOutTime;


/*------------------------------------------------------------------------------
**
** TM_iResponseTime
**
**------------------------------------------------------------------------------
**
** Time from start timer to first set set timer. ( repsonse on Modbus )
**
**------------------------------------------------------------------------------
*/

extern UINT16 TM_iResponseTime;


/*******************************************************************************
**
** Public Services Definitions
**
********************************************************************************
*/

/*------------------------------------------------------------------------------
**
** TM_SetTimer()
**
** Sets the timer Timout time
**
**------------------------------------------------------------------------------
**
** Inputs:
**    iTime    - New timeout time in uSecs
**
** Outputs:
**    None
**
** Usage:
**    TM_SetTimer();
**
**------------------------------------------------------------------------------
*/

void TM_SetTimer( UINT16 iTime );


/*------------------------------------------------------------------------------
**
** TM_StartTimer()
**
** Starts the timer
**
**------------------------------------------------------------------------------
**
** Inputs:
**    None
**
** Outputs:
**    None
**
** Usage:
**    TM_StartTimer();
**
**------------------------------------------------------------------------------
*/

void TM_StartTimer( void );


/*------------------------------------------------------------------------------
**
** TM_StopTimer()
**
** Stops the timer
**
**------------------------------------------------------------------------------
**
** Inputs:
**    None
**
** Outputs:
**    None
**
** Usage:
**    TM_StopTimer();
**
**------------------------------------------------------------------------------
*/

void TM_StopTimer( void );


/*------------------------------------------------------------------------------
**
** TM_TimeOut()
**
** Check if the timer has timed out.
**
**------------------------------------------------------------------------------
**
** Inputs:
**    None
**
** Outputs:
**    Return      - TRUE = The timer has timed out
**                - FALSE = The timer has not timed out
**
** Usage:
**    if( TM_TimeOut )
**    {
**       .........
**
**------------------------------------------------------------------------------
*/

BOOL TM_TimeOut();


#endif  /* inclusion lock */

/*******************************************************************************
**
** End of TM.H
**
********************************************************************************
*/