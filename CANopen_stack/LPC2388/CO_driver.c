/*******************************************************************************

   File - CO_driver.c
   CAN module object for LPC2388.

   Copyright (C) 2012 CSM Group

   License: GNU Lesser General Public License (LGPL).

   <http://canopennode.sourceforge.net>
*/
/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.


   Author: Michele Sponchiado

*******************************************************************************/

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
//#include "CO_Emergency.h"

#include <stdlib.h> // for malloc, free


typedef int (*type_can_callback)(enum_LPC_event lpc_event, const CO_CANrxMsg_t *msg);

int enable_CAN1callback,enable_CAN2callback;		

// tests if callback enabled on can channel
unsigned int ui_is_enabled_callback(enum_LPC_CAN_Ports can_ports){
	unsigned int retcode;
	retcode=1;
    if (can_ports==enum_LPC_CAN_Port_both){        
        if (!enable_CAN1callback||!enable_CAN2callback)
			retcode=0;
    }
    else if (can_ports==enum_LPC_CAN_Port_1){        
        if (!enable_CAN1callback)
			retcode=0;
    }
    else if (can_ports==enum_LPC_CAN_Port_2){        
        if (!enable_CAN2callback)
			retcode=0;
    }
	else{
		retcode=0;
	}
	return retcode;
}


int canRegisterCallback(enum_LPC_CAN_Ports can_ports, type_can_callback callback){
	int retcode;
	retcode=1;
    if (can_ports==enum_LPC_CAN_Port_both){        
        enable_CAN1callback = 1;		
        enable_CAN2callback = 1;		
    }
    else if (can_ports==enum_LPC_CAN_Port_1){        
        enable_CAN1callback = 1;		
    }
    else if (can_ports==enum_LPC_CAN_Port_2){        
        enable_CAN2callback = 1;		
    }
	else{
		retcode=0;
	}
	return retcode;
}

int  can_send(enum_LPC_CAN_Ports can_port,CO_CANtxArray_t *buffer){
	int i;
	unsigned long ul;
	unsigned long ul_rtr;
	if (can_port==enum_LPC_CAN_Port_1){
		// Transmit initial message on CAN 1
		// wait max 100ms
		i=1000;
		while(i){
			if ( (CAN1GSR & (1 << 3)) ){
				break;
			}
			v_wait_microseconds(100);
			i--;
		}
		if (!i)
			return 0;
		ul=buffer->DLC&0xf;
        ul_rtr=0;
        if (buffer->DLC&(1<<7)){
            ul_rtr=(1<<30);
        }
		MsgBuf_TX1.Frame = (ul<<16)|(ul_rtr); // 
		MsgBuf_TX1.MsgID = buffer->ident; // 
		memcpy(&MsgBuf_TX1.DataA,buffer->_data,4);
		memcpy(&MsgBuf_TX1.DataB,buffer->_data+4,4);
		if ( dw_can1_send_message( &MsgBuf_TX1 ) == FALSE ){
			return 0;
		}

	}
	else{
		// Transmit initial message on CAN 1
		// wait max 100ms
		i=1000;
		while(i){
			if ( (CAN2GSR & (1 << 3)) ){
				break;
			}
			v_wait_microseconds(100);
			i--;
		}
		if (!i)
			return 0;
		ul=buffer->DLC;
		MsgBuf_TX2.Frame = ul<<16; // 
		MsgBuf_TX2.MsgID = buffer->ident; // 
		memcpy(&MsgBuf_TX2.DataA,buffer->_data,4);
		memcpy(&MsgBuf_TX2.DataB,buffer->_data+4,4);
		if ( dw_can2_send_message( &MsgBuf_TX2 ) == FALSE ){
			return 0;
		}
	}
	return 1;
}



/******************************************************************************/
void memcpySwap2(UNSIGNED8* dest, UNSIGNED8* src){
	#ifdef BIG_ENDIAN
		*(dest++) = *(src+1);
		*(dest) = *(src);
	#endif
}

void memcpySwap4(UNSIGNED8* dest, UNSIGNED8* src){	
	#ifdef BIG_ENDIAN
		src += 3;
		*(dest++) = *(src--);
		*(dest++) = *(src--);
		*(dest++) = *(src--);
		*(dest) = *(src);
	#endif
}

 
/******************************************************************************/
void CO_CANsetConfigurationMode(UNSIGNED16 CANbaseAddress){
	switch(CANbaseAddress){
		case enum_LPC_CAN_Port_1:
			CAN1MOD = 1;
			break;
		case enum_LPC_CAN_Port_2:
			CAN2MOD = 1;	// Reset CAN
			break;
		default:
			break;
	}
//   canEnableRx(CANbaseAddress, FALSE);
}


/******************************************************************************/
void CO_CANsetNormalMode(UNSIGNED16 CANbaseAddress){
//   canEnableRx(CANbaseAddress, TRUE);
	switch(CANbaseAddress){
		case enum_LPC_CAN_Port_1:
			CAN1MOD = 0;
			break;
		case enum_LPC_CAN_Port_2:
			CAN2MOD = 0;	// Reset CAN
			break;
		default:
			break;
	}
}


/******************************************************************************/
INTEGER16 CO_CANmodule_init(
      CO_CANmodule_t     **ppCANmodule,
      UNSIGNED16           CANbaseAddress,
      UNSIGNED16           rxSize,
      UNSIGNED16           txSize,
      UNSIGNED16           CANbitRate)
{
   UNSIGNED16 i;
   enum_az4186_canbus_frequency az4186_canbus_frequency;

   //allocate memory if not already allocated
   if((*ppCANmodule) == NULL){
      if(((*ppCANmodule)          = (CO_CANmodule_t *) malloc(       sizeof(CO_CANmodule_t ))) == NULL){                                                                   return CO_ERROR_OUT_OF_MEMORY;}
      if(((*ppCANmodule)->rxArray = (CO_CANrxArray_t *)malloc(rxSize*sizeof(CO_CANrxArray_t))) == NULL){                               free(*ppCANmodule); *ppCANmodule=0; return CO_ERROR_OUT_OF_MEMORY;}
      if(((*ppCANmodule)->txArray = (CO_CANtxArray_t *)malloc(txSize*sizeof(CO_CANtxArray_t))) == NULL){free((*ppCANmodule)->rxArray); free(*ppCANmodule); *ppCANmodule=0; return CO_ERROR_OUT_OF_MEMORY;}
   }
   else if((*ppCANmodule)->rxArray == NULL || (*ppCANmodule)->txArray == NULL) return CO_ERROR_ILLEGAL_ARGUMENT;

   CO_CANmodule_t *CANmodule = *ppCANmodule; //pointer to (newly created) object

   //Configure object variables
   CANmodule->CANbaseAddress = CANbaseAddress;
   CANmodule->rxSize = rxSize;
   CANmodule->txSize = txSize;
   CANmodule->curentSyncTimeIsInsideWindow = 0;
   CANmodule->bufferInhibitFlag = 0;
   CANmodule->firstCANtxMessage = 1;
   CANmodule->error = 0;
   CANmodule->CANtxCount = 0;
   CANmodule->errOld = 0;
   CANmodule->EM = 0;
   i=0;
   while(i<rxSize){
      CANmodule->rxArray[i].ident = 0;
      CANmodule->rxArray[i].pFunct = 0;
	  i++;
   }
   i=0;
   while(i<txSize){
      CANmodule->txArray[i].bufferFull = 0;
	  i++;
   }

   switch(CANbitRate){
	case 1000:
		az4186_canbus_frequency=enum_az4186_canbus_frequency_1MHz;
		break;
	case 500:
		az4186_canbus_frequency=enum_az4186_canbus_frequency_500kHz;
		break;
	case 250:
		az4186_canbus_frequency=enum_az4186_canbus_frequency_250kHz;
		break;
	case 125:
		az4186_canbus_frequency=enum_az4186_canbus_frequency_125kHz;
		break;
	default:
		return CO_ERROR_ILLEGAL_BAUDRATE;
   }

	if (!ui_init_canbus(CANbaseAddress,az4186_canbus_frequency))
		if (!ui_init_canbus(CANbaseAddress,az4186_canbus_frequency))
			return CO_ERROR_ILLEGAL_ARGUMENT;

	// if you want t accept all of the messages, use
#if !ACCEPTANCE_FILTER_ENABLED
	CAN_set_acceptance_filter( ACCF_BYPASS );
#else
   //Set acceptance filters to accept all messages with standard identifier, also accept rtr
	CAN_set_acceptance_filter( ACCF_ON );
#endif

   //purge buffers
   //canEnableRx(CANbaseAddress, FALSE);
   //canPurgeRx(CANbaseAddress);
   //canPurgeTx(CANbaseAddress, FALSE);
    
   //register callback function
   if(CANbaseAddress == enum_LPC_CAN_Port_1)
      canRegisterCallback(CANbaseAddress, CAN1callback);
   else if(CANbaseAddress == enum_LPC_CAN_Port_2)
      canRegisterCallback(CANbaseAddress, CAN2callback);

   return CO_ERROR_NO;
}


/******************************************************************************/
void CO_CANmodule_delete(CO_CANmodule_t** ppCANmodule){
   if(*ppCANmodule){
//#warning realize can_deinit!
//      canDeinit((*ppCANmodule)->CANbaseAddress);
      free((*ppCANmodule)->txArray);
      free((*ppCANmodule)->rxArray);
      free(*ppCANmodule);
      *ppCANmodule = 0;
   }
}


/******************************************************************************/
// it seems that this function isn't used
//UNSIGNED16 CO_CANrxMsg_readIdent(CO_CANrxMsg_t *rxMsg){
//   return (UNSIGNED16) canDecodeId(rxMsg->ident);
//}


/******************************************************************************/
INTEGER16 CO_CANrxBufferInit( CO_CANmodule_t   *CANmodule,
                              UNSIGNED16        index,
                              UNSIGNED16        ident,
                              UNSIGNED16        mask,
                              UNSIGNED8         rtr,
                              void             *object,
                              INTEGER16       (*pFunct)(void *object, CO_CANrxMsg_t *message))
{
   CO_CANrxArray_t *rxBuffer;

   //safety
   if(!CANmodule || !object || !pFunct || index >= CANmodule->rxSize){
      return CO_ERROR_ILLEGAL_ARGUMENT;
   }


   //buffer, which will be configured
   rxBuffer = CANmodule->rxArray + index;

   //Configure object variables
   rxBuffer->object = object;
   rxBuffer->pFunct = pFunct;

   //Configure CAN identifier and CAN mask, bit aligned with CAN module.
   //No hardware filtering is used.
   //rxBuffer->ident = canEncodeId(ident, FALSE, rtr);
   //rxBuffer->mask = canEncodeId(mask, FALSE, FALSE);
   rxBuffer->ident = ident;
   rxBuffer->mask  = mask;

   return CO_ERROR_NO;
}


/******************************************************************************/
CO_CANtxArray_t *CO_CANtxBufferInit(
                              CO_CANmodule_t   *CANmodule,
                              UNSIGNED16        index,
                              UNSIGNED16        ident,
                              UNSIGNED8         rtr,
                              UNSIGNED8         noOfBytes,
                              UNSIGNED8         syncFlag)
{
   //safety
   if(!CANmodule || CANmodule->txSize <= index) return 0;

   //get specific buffer
   CO_CANtxArray_t *buffer = &CANmodule->txArray[index];

   //CAN identifier, bit aligned with CAN module registers
//   buffer->ident = canEncodeId(ident, FALSE, rtr);
   buffer->ident = ident;

   buffer->DLC = noOfBytes|((rtr?1:0)<<7);
   buffer->bufferFull = 0;
   buffer->syncFlag = syncFlag?1:0;

   return buffer;
}


/******************************************************************************/
INTEGER16 CO_CANsend(   CO_CANmodule_t   *CANmodule,
                        CO_CANtxArray_t  *buffer)
{

   //Was previous message sent or it is still waiting?
   if(buffer->bufferFull){
      if(!CANmodule->firstCANtxMessage)//don't set error, if bootup message is still on buffers
         {
            //CO_errorReport((CO_emergencyReport_t*)CANmodule->EM, ERROR_CAN_TX_OVERFLOW, 0);
         }
      return CO_ERROR_TX_OVERFLOW;
   }

   //messages with syncFlag set (synchronous PDOs) must be transmited inside preset time window
   if(CANmodule->curentSyncTimeIsInsideWindow && buffer->syncFlag && !(*CANmodule->curentSyncTimeIsInsideWindow)){
      //CO_errorReport((CO_emergencyReport_t*)CANmodule->EM, ERROR_TPDO_OUTSIDE_WINDOW, 0);
      return CO_ERROR_TX_PDO_WINDOW;
   }

   //send message
   int ok;
   ok = can_send(CANmodule->CANbaseAddress,buffer);
   //if no buffer is free, message will be sent by interrupt
   if(!ok){
      buffer->bufferFull = 1;
      CANmodule->CANtxCount++;
   }

#ifdef CO_LOG_CAN_MESSAGES
   void CO_logMessage(const CanMsg *msg);
   CO_logMessage((const CanMsg*) buffer);
#endif

   return CO_ERROR_NO;
}


/******************************************************************************/
void CO_CANclearPendingSyncPDOs(CO_CANmodule_t *CANmodule){

   if(CANmodule->bufferInhibitFlag){
      //canPurgeTx(CANmodule->CANbaseAddress, FALSE);
      //CO_errorReport((CO_emergencyReport_t*)CANmodule->EM, ERROR_TPDO_OUTSIDE_WINDOW, 1);
   }
}


/******************************************************************************/
void CO_CANverifyErrors(CO_CANmodule_t *CANmodule){
   //unsigned rxErrors, txErrors;
   //CO_emergencyReport_t* EM = (CO_emergencyReport_t*)CANmodule->EM;

   //canGetErrorCounters(CANmodule->CANbaseAddress, &rxErrors, &txErrors);
   //if(txErrors > 0xFFFF) txErrors = 0xFFFF;
   //if(rxErrors > 0xFF) rxErrors = 0xFF;
   //UNSIGNED32 err = (UNSIGNED32)txErrors << 16 | rxErrors << 8 | CANmodule->error;

   //if(CANmodule->errOld != err){
   //   CANmodule->errOld = err;

   //   if(txErrors >= 256){                               //bus off
   //      CO_errorReport(EM, ERROR_CAN_TX_BUS_OFF, err);
   //   }
   //   else{                                              //not bus off
   //      CO_errorReset(EM, ERROR_CAN_TX_BUS_OFF, err);

   //      if(rxErrors >= 96 || txErrors >= 96){           //bus warning
   //         CO_errorReport(EM, ERROR_CAN_BUS_WARNING, err);
   //      }

   //      if(rxErrors >= 128){                            //RX bus passive
   //         CO_errorReport(EM, ERROR_CAN_RX_BUS_PASSIVE, err);
   //      }
   //      else{
   //         CO_errorReset(EM, ERROR_CAN_RX_BUS_PASSIVE, err);
   //      }

   //      if(txErrors >= 128){                            //TX bus passive
   //         if(!CANmodule->firstCANtxMessage){
   //            CO_errorReport(EM, ERROR_CAN_TX_BUS_PASSIVE, err);
   //         }
   //      }
   //      else{
   //         INTEGER16 wasCleared;
   //         wasCleared =        CO_errorReset(EM, ERROR_CAN_TX_BUS_PASSIVE, err);
   //         if(wasCleared == 1) CO_errorReset(EM, ERROR_CAN_TX_OVERFLOW, err);
   //      }

   //      if(rxErrors < 96 && txErrors < 96){             //no error
   //         INTEGER16 wasCleared;
   //         wasCleared =        CO_errorReset(EM, ERROR_CAN_BUS_WARNING, err);
   //         if(wasCleared == 1) CO_errorReset(EM, ERROR_CAN_TX_OVERFLOW, err);
   //      }
   //   }

   //   if(CANmodule->error & 0x02){                       //CAN RX bus overflow
   //      CO_errorReport(EM, ERROR_CAN_RXB_OVERFLOW, err);
   //   }
   //}

}


/******************************************************************************/
unsigned long ul_numof_unmatched_msg;
int CO_CANinterrupt(CO_CANmodule_t *CANmodule, enum_LPC_event event, const CO_CANrxMsg_t *msg){

   //receive interrupt (New CAN messagge is available in RX FIFO buffer)
   if(event == enum_LPC_event_rx_full){
      CO_CANrxMsg_t *rcvMsg;     //pointer to received message in CAN module
      UNSIGNED16 index;          //index of received message
      CO_CANrxArray_t *msgBuff;  //receive message buffer from CO_CANmodule_t object.
      UNSIGNED8 msgMatched = 0;

      rcvMsg = (CO_CANrxMsg_t *) msg;  //structures are aligned
      //CAN module filters are not used, message with any standard 11-bit identifier
      //has been received. Search rxArray form CANmodule for the same CAN-ID.
      msgBuff = CANmodule->rxArray;
      for(index = 0; index < CANmodule->rxSize; index++){
         if(((rcvMsg->ident ^ msgBuff->ident) & msgBuff->mask) == 0){
            msgMatched = 1;
            break;
         }
         msgBuff++;
      }

      //Call specific function, which will process the message
      if(msgMatched){
        msgBuff->pFunct(msgBuff->object, rcvMsg);
      }
      else{
        ul_numof_unmatched_msg++;
      }

#ifdef CO_LOG_CAN_MESSAGES
      void CO_logMessage(const CanMsg *msg);
      CO_logMessage(msg);
#endif

      return 0;
   }

   //transmit interrupt (TX buffer is free)
   if(event == enum_LPC_event_tx_empty){
      //First CAN message (bootup) was sent successfully
      CANmodule->firstCANtxMessage = 0;
      //clear flag from previous message
      CANmodule->bufferInhibitFlag = 0;
      //Are there any new messages waiting to be send and buffer is free
      if(CANmodule->CANtxCount > 0){
         UNSIGNED16 index;          //index of transmitting message
         CANmodule->CANtxCount--;
         //search through whole array of pointers to transmit message buffers.
         for(index = 0; index < CANmodule->txSize; index++){
            //get specific buffer
            CO_CANtxArray_t *buffer = &CANmodule->txArray[index];
            //if message buffer is full, send it.
            if(buffer->bufferFull){
               //messages with syncFlag set (synchronous PDOs) must be transmited inside preset time window
               if(CANmodule->curentSyncTimeIsInsideWindow && buffer->syncFlag){
                  if(!(*CANmodule->curentSyncTimeIsInsideWindow)){
                     //CO_errorReport((CO_emergencyReport_t*)CANmodule->EM, ERROR_TPDO_OUTSIDE_WINDOW, 2);
                     //release buffer
                     buffer->bufferFull = 0;
                     //exit for loop
                     break;
                  }
                  CANmodule->bufferInhibitFlag = 1;
               }

               //Copy message to CAN buffer
               can_send(CANmodule->CANbaseAddress, buffer);

               //release buffer
               buffer->bufferFull = 0;
               //exit for loop
               break;
            }
         }//end of for loop
      }
      return 0;
   }

   // if(event == CAN_EVENT_BUS_OFF){
      // CANmodule->error |= 0x01;
      // return 0;
   // }

   // if(event == CAN_EVENT_OVERRUN){
      // CANmodule->error |= 0x02;
      // return 0;
   // }
   return 0;
}


