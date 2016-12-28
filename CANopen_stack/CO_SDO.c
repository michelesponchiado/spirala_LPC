/*******************************************************************************

   File - CO_SDO.c
   CANopen SDO server object (SDO slave).

   Copyright (C) 2004-2008 Janez Paternoster

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


   Author: Janez Paternoster

*******************************************************************************/


#include "CO_driver.h"
#include "CO_SDO.h"

#include <stdlib.h> // for malloc, free
#include "az4186_initHw.h"


/******************************************************************************/
INTEGER16 CO_SDO_receive(void *object, CO_CANrxMsg_t *msg){
   CO_SDO_t *SDO;

   SDO = (CO_SDO_t*)object;   //this is the correct pointer type of the first argument

   //verify message length
   if(msg->DLC != 8) return CO_ERROR_RX_MSG_LENGTH;

   //verify message overflow (previous message was not processed yet)
   if(SDO->CANrxNew) return CO_ERROR_RX_OVERFLOW;

   //copy data and set 'new message' flag
   SDO->CANrxData[0] = msg->_data[0];
   SDO->CANrxData[1] = msg->_data[1];
   SDO->CANrxData[2] = msg->_data[2];
   SDO->CANrxData[3] = msg->_data[3];
   SDO->CANrxData[4] = msg->_data[4];
   SDO->CANrxData[5] = msg->_data[5];
   SDO->CANrxData[6] = msg->_data[6];
   SDO->CANrxData[7] = msg->_data[7];

   SDO->CANrxNew = 1;

   return CO_ERROR_NO;
}


/******************************************************************************/
#ifndef CO_ODF_MASK_DEFAULT
UNSIGNED32 CO_ODF(   void       *object,
                     UNSIGNED16  index,
                     UNSIGNED8   subIndex,
                     UNSIGNED16 *pLength,
                     UNSIGNED16  attribute,
                     UNSIGNED8   dir,
                     void       *dataBuff,
                     const void *pData)
{
   #define CO_ODA_MEM_ROM          0x01   //same attribute is in CO_SDO.h file
   #define CO_ODA_MEM_RAM          0x02   //same attribute is in CO_SDO.h file
   #define CO_ODA_MEM_EEPROM       0x03   //same attribute is in CO_SDO.h file
   #define CO_ODA_MB_VALUE         0x80   //same attribute is in CO_SDO.h file

   UNSIGNED16 length = *pLength;

   if(dir==0){ //Reading Object Dictionary variable
#ifdef BIG_ENDIAN
      if(attribute & CO_ODA_MB_VALUE){
         UNSIGNED8 *dataBuffCopy = (UNSIGNED8*)dataBuff;
         UNSIGNED8 *pDataCopy = (UNSIGNED8*)pData + length;
         DISABLE_INTERRUPTS();
         while(length--) *(dataBuffCopy++) = *(--pDataCopy);
         ENABLE_INTERRUPTS();
      }
      else{
#endif
         UNSIGNED8 *dataBuffCopy = (UNSIGNED8*)dataBuff;
         UNSIGNED8 *pDataCopy = (UNSIGNED8*)pData;
         DISABLE_INTERRUPTS();
         while(length--) *(dataBuffCopy++) = *(pDataCopy++);
         ENABLE_INTERRUPTS();
#ifdef BIG_ENDIAN
      }
#endif
   }

   else{ //Writing Object Dictionary variable
#ifdef BIG_ENDIAN
      if(attribute & CO_ODA_MB_VALUE){
         UNSIGNED8 *dataBuffCopy = (UNSIGNED8*)dataBuff;
         UNSIGNED8 *pDataCopy = (UNSIGNED8*)pData + length;
         DISABLE_INTERRUPTS();
         while(length--) *(--pDataCopy) = *(dataBuffCopy++);
         ENABLE_INTERRUPTS();
      }
      else{
#endif
         UNSIGNED8 *dataBuffCopy = (UNSIGNED8*)dataBuff;
         UNSIGNED8 *pDataCopy = (UNSIGNED8*)pData;
         DISABLE_INTERRUPTS();
         while(length--) *(pDataCopy++) = *(dataBuffCopy++);
         ENABLE_INTERRUPTS();
#ifdef BIG_ENDIAN
      }
#endif
   }

   return 0L;
}
#endif


/******************************************************************************/
UNSIGNED32 CO_ODF_1200( void       *object,
                        UNSIGNED16  index,
                        UNSIGNED8   subIndex,
                        UNSIGNED16 *pLength,
                        UNSIGNED16  attribute,
                        UNSIGNED8   dir,
                        void       *dataBuff,
                        const void *pData)
{
   UNSIGNED8 *nodeId;
   UNSIGNED32 abortCode;

   nodeId = (UNSIGNED8*) object; //this is the correct pointer type of the first argument

   abortCode = CO_ODF(object, index, subIndex, pLength, attribute, dir, dataBuff, pData);

   if(subIndex>0){
      //add NodeId to the value, which will be sent by SDO server
      UNSIGNED32 val;
      memcpySwap4((UNSIGNED8*)&val, (UNSIGNED8*)dataBuff);
      val += *nodeId;
      memcpySwap4((UNSIGNED8*)dataBuff, (UNSIGNED8*)&val);
   }

   return abortCode;
}


/******************************************************************************/
void CO_OD_configureArgumentForODF( CO_SDO_t      *SDO,
                                    UNSIGNED16     index,
                                    void          *object)
{
   UNSIGNED16 indexOfFoundObject;

   if(CO_OD_find(SDO, index, &indexOfFoundObject))
      SDO->ObjectDictionaryPointers[indexOfFoundObject] = object;
}


/******************************************************************************/
INTEGER16 CO_SDO_init(
      CO_SDO_t                    **ppSDO,
      UNSIGNED16                    COB_IDClientToServer,
      UNSIGNED16                    COB_IDServerToClient,
      UNSIGNED16                    ObjDictIndex_SDOServerParameter,
const sCO_OD_object                *ObjectDictionary,
      UNSIGNED16                    ObjectDictionarySize,
      UNSIGNED8                     nodeId,
      CO_CANmodule_t *CANdevRx, UNSIGNED16 CANdevRxIdx,
      CO_CANmodule_t *CANdevTx, UNSIGNED16 CANdevTxIdx)
{
   CO_SDO_t *SDO;

   //allocate memory if not already allocated
   if((*ppSDO) == NULL){
      if(((*ppSDO)                           = (CO_SDO_t *) malloc(                     sizeof(CO_SDO_t))) == NULL){                          return CO_ERROR_OUT_OF_MEMORY;}
      if(((*ppSDO)->ObjectDictionaryPointers = (void*)      malloc(ObjectDictionarySize*sizeof(void*)))    == NULL){free(*ppSDO); *ppSDO = 0; return CO_ERROR_OUT_OF_MEMORY;}
   }
   else if((*ppSDO)->ObjectDictionaryPointers == NULL) return CO_ERROR_ILLEGAL_ARGUMENT;

   SDO = *ppSDO; //pointer to (newly created) object

   //Configure object variables
   SDO->state = 0;
   SDO->ObjectDictionary = ObjectDictionary;
   SDO->ObjectDictionarySize = ObjectDictionarySize;
   SDO->nodeId = nodeId;

   //Configure SDO server for first argument of CO_ODF_1200
   CO_OD_configureArgumentForODF(SDO, ObjDictIndex_SDOServerParameter, (void*)&SDO->nodeId);

   //configure SDO server CAN reception
   CO_CANrxBufferInit(     CANdevRx,               //CAN device
                           CANdevRxIdx,            //rx buffer index
                           COB_IDClientToServer,   //CAN identifier
                           0x7FF,                  //mask
                           0,                      //rtr
                           (void*)SDO,             //object passed to receive function
                           CO_SDO_receive);        //this function will process received message

   //configure SDO server CAN transmission
   SDO->CANdevTx = CANdevTx;
   SDO->CANtxBuff = CO_CANtxBufferInit(
                           CANdevTx,               //CAN device
                           CANdevTxIdx,            //index of specific buffer inside CAN module
                           COB_IDServerToClient,   //CAN identifier
                           0,                      //rtr
                           8,                      //number of data bytes
                           0);                     //synchronous message flag bit

   return CO_ERROR_NO;
}


/******************************************************************************/
void CO_SDO_delete(CO_SDO_t **ppSDO){
   if(*ppSDO){
      free((*ppSDO)->ObjectDictionaryPointers);
      free(*ppSDO);
      *ppSDO = 0;
   }
}


/******************************************************************************/
const sCO_OD_object* CO_OD_find( CO_SDO_t            *SDO,
                                 UNSIGNED16           index,
                                 UNSIGNED16          *indexOfFoundObject){
   //Fast search in ordered Object Dictionary. If indexes are mixed, this won't work.
   //If Object Dictionary has up to 2^N entries, then N is max number of loop passes.
   UNSIGNED16 cur, min, max;
   const sCO_OD_object* object;

   min = 0;
   max = SDO->ObjectDictionarySize - 1;
   while(min < max){
      cur = (min + max) / 2;
      object = &SDO->ObjectDictionary[cur];
      //Is object matched
      if(index == object->index){
         if(indexOfFoundObject) *indexOfFoundObject = cur;
         return object;
      }
      if(index < object->index){
         max = cur;
         if(max) max--;
      }
      else
         min = cur + 1;
   }

   if(min == max){
      object = &SDO->ObjectDictionary[min];
      //Is object matched
      if(index == object->index){
         if(indexOfFoundObject) *indexOfFoundObject = min;
         return object;
      }
   }

   return 0;  //object does not exist in OD
}


/******************************************************************************/
UNSIGNED16 CO_OD_getLength(const sCO_OD_object* object, UNSIGNED8 subIndex){
   if(object->length){//object type is var or array
      if(subIndex == 0 && object->maxSubIndex > 0) return 1;  //object type is array
      return object->length;
   }
   else if(object->pData == 0){//data type is domain
      return CO_OD_MAX_OBJECT_SIZE;
   }
   else{//object type is record
      return ((const CO_ODrecord_t*)(object->pData))[subIndex].length;
   }
}


/******************************************************************************/
UNSIGNED8 CO_OD_getAttribute(const sCO_OD_object* object, UNSIGNED8 subIndex){
   if(object->length){//object type is var or array
      UNSIGNED8 attr = object->attribute;
      if(subIndex == 0 && object->maxSubIndex > 0) return (attr&0x03) | 0x04;  //object type is array
      return attr;
   }
   else if(object->pData == 0){//data type is domain
      return object->attribute;
   }
   else{//object type is record
      return ((const CO_ODrecord_t*)(object->pData))[subIndex].attribute;
   }
}


/******************************************************************************/
const void* CO_OD_getDataPointer(const sCO_OD_object* object, UNSIGNED8 subIndex){
   if(object->pData == 0){             //data type is domain
      return 0;
   }
   else if(object->maxSubIndex == 0){  //Object Type is Var
      return object->pData;
   }
   else if(object->length){            //Object Type is Array
      if(subIndex==0){
         //this is the data, for the subIndex 0 in the array
         return (const void*) &object->maxSubIndex;
      }
      return (const void*)(((const INTEGER8*)object->pData) + ((subIndex-1) * object->length));
   }
   else{                               //Object Type is Record
      return ((const CO_ODrecord_t*)(object->pData))[subIndex].pData;
   }
}


/******************************************************************************/
void CO_SDO_abort(CO_SDO_t *SDO, UNSIGNED32 thecode){
   SDO->CANtxBuff->_data[0] = 0x80;
   SDO->CANtxBuff->_data[1] = SDO->index & 0xFF;
   SDO->CANtxBuff->_data[2] = (SDO->index>>8) & 0xFF;
   SDO->CANtxBuff->_data[3] = SDO->subIndex;
   memcpySwap4(&SDO->CANtxBuff->_data[4], (UNSIGNED8*)&thecode);
   SDO->state = 0;
   CO_CANsend(SDO->CANdevTx, SDO->CANtxBuff);
}

void CO_SDO_process( CO_SDO_t            *SDO,
                     UNSIGNED8            NMTisPreOrOperational,
                     UNSIGNED16           timeDifference_ms,
                     UNSIGNED16           SDOtimeoutTime)
{
   if(NMTisPreOrOperational){
      if(SDO->CANrxNew && !SDO->CANtxBuff->bufferFull){  //New SDO object has to be processed and SDO CAN send buffer is free
         //clear response buffer
         SDO->CANtxBuff->_data[0] = SDO->CANtxBuff->_data[1] = SDO->CANtxBuff->_data[2] = SDO->CANtxBuff->_data[3] = 0;
         SDO->CANtxBuff->_data[4] = SDO->CANtxBuff->_data[5] = SDO->CANtxBuff->_data[6] = SDO->CANtxBuff->_data[7] = 0;

         switch(SDO->CANrxData[0]>>5){  //Switch Client Command Specifier
            UNSIGNED32 abortCode;
            UNSIGNED8 attr;
            UNSIGNED16 len, i;

            case 1:   //Initiate SDO Download request
               SDO->index = (UNSIGNED16)(SDO->CANrxData[2]<<8) | SDO->CANrxData[1];
               SDO->subIndex = SDO->CANrxData[3];
               //find pointer to object dictionary object
               SDO->pODE = CO_OD_find(SDO, SDO->index, &SDO->indexOfFoundObject);
               if(!SDO->pODE){
                  CO_SDO_abort(SDO, 0x06020000L); //object does not exist in OD
                  break;
               }
               if(SDO->subIndex > SDO->pODE->maxSubIndex){
                  CO_SDO_abort(SDO, 0x06090011L); //Sub-index does not exist.
                  break;
               }
               SDO->dataLength = CO_OD_getLength(SDO->pODE, SDO->subIndex);
               attr = CO_OD_getAttribute(SDO->pODE, SDO->subIndex);
               //verify length
               if(SDO->dataLength > CO_OD_MAX_OBJECT_SIZE){
                  CO_SDO_abort(SDO, 0x06040047L);  //general internal incompatibility in the device
                  break;
               }
               if(!(attr&CO_ODA_WRITEABLE)){
                  CO_SDO_abort(SDO, 0x06010002L); //attempt to write a read-only object
                  break;
               }
               //default response
               SDO->CANtxBuff->_data[0] = 0x60;
               SDO->CANtxBuff->_data[1] = SDO->CANrxData[1];
               SDO->CANtxBuff->_data[2] = SDO->CANrxData[2];
               SDO->CANtxBuff->_data[3] = SDO->CANrxData[3];
               if(SDO->CANrxData[0] & 0x02){
                  //Expedited transfer
                  if(SDO->CANrxData[0] & 0x01)//is size indicated
                     len = 4 - ((SDO->CANrxData[0]>>2)&0x03);   //size
                  else len = 4;
                  //is domain data type
                  if(SDO->pODE->pData == 0){
                     SDO->dataLength = len;
                  }
                  //verify length
                  else if(SDO->dataLength != len){
                     CO_SDO_abort(SDO, 0x06070010L);  //Length of service parameter does not match
                     break;
                  }
                  //write to memory
                  abortCode = SDO->pODE->pFunct(SDO->ObjectDictionaryPointers[SDO->indexOfFoundObject],
                                                SDO->index,
                                                SDO->subIndex,
                                               &SDO->dataLength,
                                                attr,
                                                1,
                                                &SDO->CANrxData[4],
                                                CO_OD_getDataPointer(SDO->pODE, SDO->subIndex));
                  //send response and finish
                  if(abortCode){
                     CO_SDO_abort(SDO, abortCode);
                     break;
                  }
                  SDO->state = 0;
                  CO_CANsend(SDO->CANdevTx, SDO->CANtxBuff);
               }
               else{
#if CO_OD_MAX_OBJECT_SIZE == 0x04
                  CO_SDO_abort(SDO, 0x06010000L);  //unsupported access to an object
               }
               break;
#else
                  //segmented transfer
                  //verify length if size is indicated
                  if(SDO->CANrxData[0]&0x01){
                     UNSIGNED32 lenRx;
                     memcpySwap4((UNSIGNED8*)&lenRx, &SDO->CANrxData[4]);
                     //is domain data type
                     if(SDO->pODE->pData == 0){
                        SDO->dataLength = lenRx;
                     }
                     //verify length
                     else if(lenRx != SDO->dataLength){
                        CO_SDO_abort(SDO, 0x06070010L);  //Length of service parameter does not match
                        break;
                     }
                  }
                  SDO->bufferOffset = 0;
                  SDO->timeoutTimer = 0;
                  SDO->state = 0x02;
                  CO_CANsend(SDO->CANdevTx, SDO->CANtxBuff);
               }
               break;

            case 0:   //Download SDO segment
               if(!(SDO->state&0x02)){//download SDO segment was not initiated
                  CO_SDO_abort(SDO, 0x05040001L);//command specifier not valid
                  break;
               }
               //verify toggle bit
               if((SDO->CANrxData[0]&0x10) != (SDO->state&0x10)){
                  CO_SDO_abort(SDO, 0x05030000L);//toggle bit not alternated
                  break;
               }
               //get size
               len = 7 - ((SDO->CANrxData[0]>>1)&0x07);   //size
               //verify length
               if((SDO->bufferOffset + len) > SDO->dataLength || (SDO->bufferOffset + len) > CO_OD_MAX_OBJECT_SIZE){
                  CO_SDO_abort(SDO, 0x06070012L);  //Length of service parameter too high
                  break;
               }
               //copy data to buffer
               for(i=0; i<len; i++)
                  SDO->databuffer[SDO->bufferOffset + i] = SDO->CANrxData[i+1];
               SDO->bufferOffset += len;
               //write response data (partial)
               SDO->CANtxBuff->_data[0] = 0x20 | (SDO->state&0x10);
               //If no more segments to be downloaded, copy data to variable
               if(SDO->CANrxData[0] & 0x01){
                  //verify length
                  if(SDO->dataLength != SDO->bufferOffset){
                     CO_SDO_abort(SDO, 0x06070010L);  //Length of service parameter does not match
                     break;
                  }
                  abortCode = SDO->pODE->pFunct(SDO->ObjectDictionaryPointers[SDO->indexOfFoundObject],
                                                SDO->index,
                                                SDO->subIndex,
                                               &SDO->dataLength,
                                                CO_OD_getAttribute(SDO->pODE, SDO->subIndex),
                                                1,
                                                SDO->databuffer,
                                                CO_OD_getDataPointer(SDO->pODE, SDO->subIndex));
                  if(abortCode){
                     CO_SDO_abort(SDO, abortCode);
                     break;
                  }
                  SDO->state = 0;
               }
               else{
                  //reset timeout timer, alternate toggle bit
                  SDO->timeoutTimer = 0;
                  if(SDO->state&0x10) SDO->state &= 0xEF;
                  else SDO->state |= 0x10;
               }
               //download segment response
               CO_CANsend(SDO->CANdevTx, SDO->CANtxBuff);
               break;
#endif
            case 2:   //Initiate SDO Upload request
               SDO->index = (UNSIGNED16)SDO->CANrxData[2]<<8 | SDO->CANrxData[1];
               SDO->subIndex = SDO->CANrxData[3];
               //find pointer to object dictionary object
               SDO->pODE = CO_OD_find(SDO, SDO->index, &SDO->indexOfFoundObject);
               if(!SDO->pODE){
                  CO_SDO_abort(SDO, 0x06020000L); //object does not exist in OD
                  break;
               }
               if(SDO->subIndex > SDO->pODE->maxSubIndex){
                  CO_SDO_abort(SDO, 0x06090011L); //Sub-index does not exist.
                  break;
               }
               SDO->dataLength = CO_OD_getLength(SDO->pODE, SDO->subIndex);
               attr = CO_OD_getAttribute(SDO->pODE, SDO->subIndex);
               //verify length
               if(SDO->dataLength > CO_OD_MAX_OBJECT_SIZE){
                  CO_SDO_abort(SDO, 0x06040047L);  //general internal incompatibility in the device
                  break;
               }
               if(!(attr&CO_ODA_READABLE)){
                  CO_SDO_abort(SDO, 0x06010001L);  //attempt to read a write-only object
                  break;
               }
               abortCode = SDO->pODE->pFunct(SDO->ObjectDictionaryPointers[SDO->indexOfFoundObject],
                                             SDO->index,
                                             SDO->subIndex,
                                            &SDO->dataLength,
                                             attr,
                                             0,
                                             SDO->databuffer,
                                             CO_OD_getDataPointer(SDO->pODE, SDO->subIndex));
               if(abortCode){
                  CO_SDO_abort(SDO, abortCode);
                  break;
               }
               //default response
               SDO->CANtxBuff->_data[1] = SDO->CANrxData[1];
               SDO->CANtxBuff->_data[2] = SDO->CANrxData[2];
               SDO->CANtxBuff->_data[3] = SDO->CANrxData[3];
               if(SDO->dataLength == 0 || SDO->dataLength > CO_OD_MAX_OBJECT_SIZE){
                  CO_SDO_abort(SDO, 0x06040047L);  //general internal incompatibility in the device
                  break;
               }
               else if(SDO->dataLength <= 4){
                  //expedited transfer
                  for(i=0; i<SDO->dataLength; i++)
                     SDO->CANtxBuff->_data[4+i] = SDO->databuffer[i];
                  SDO->CANtxBuff->_data[0] = 0x43 | ((4-SDO->dataLength) << 2);
                  SDO->state = 0;
                  CO_CANsend(SDO->CANdevTx, SDO->CANtxBuff);
               }
#if CO_OD_MAX_OBJECT_SIZE == 0x04
               break;
#else
               else{
                  UNSIGNED32 len;
                  SDO->bufferOffset = 0;  //indicates pointer to next data to be send
                  SDO->timeoutTimer = 0;
                  SDO->state = 0x04;
                  SDO->CANtxBuff->_data[0] = 0x41;
                  len = SDO->dataLength;
                  memcpySwap4(&SDO->CANtxBuff->_data[4], (UNSIGNED8*)&len);
                  CO_CANsend(SDO->CANdevTx, SDO->CANtxBuff);
               }
               break;

            case 3:   //Upload SDO segment
               if(!(SDO->state&0x04)){//upload SDO segment was not initiated
                  CO_SDO_abort(SDO, 0x05040001L);//command specifier not valid
                  break;
               }
               //verify toggle bit
               if((SDO->CANrxData[0]&0x10) != (SDO->state&0x10)){
                  CO_SDO_abort(SDO, 0x05030000L);//toggle bit not alternated
                  break;
               }
               //calculate length to be sent
               len = SDO->dataLength - SDO->bufferOffset;
               if(len > 7) len = 7;
               //fill data bytes
               for(i=0; i<len; i++)
                  SDO->CANtxBuff->_data[i+1] = SDO->databuffer[SDO->bufferOffset+i];
               SDO->bufferOffset += len;
               SDO->CANtxBuff->_data[0] = 0x00 | (SDO->state&0x10) | ((7-len)<<1);
               //is end of transfer?
               if(SDO->bufferOffset==SDO->dataLength){
                  SDO->CANtxBuff->_data[0] |= 1;
                  SDO->state = 0;
               }
               else{
                  //reset timeout timer, alternate toggle bit
                  SDO->timeoutTimer = 0;
                  if(SDO->state&0x10) SDO->state &= 0xEF;
                  else SDO->state |= 0x10;
               }
               CO_CANsend(SDO->CANdevTx, SDO->CANtxBuff);
               break;
#endif

            case 4:   //Abort SDO transfer by client
               SDO->state = 0;
               break;

            default:
               CO_SDO_abort(SDO, 0x05040001L);//command specifier not valid
         }//end switch
         SDO->CANrxNew = 0;
      }//end process new SDO object

   //verify timeout of segmented transfer
#if CO_OD_MAX_OBJECT_SIZE > 0x04
      if(SDO->state){ //Segmented SDO transfer in progress
         if(SDO->timeoutTimer < SDOtimeoutTime) SDO->timeoutTimer += timeDifference_ms;
         if(SDO->timeoutTimer >= SDOtimeoutTime){
            CO_SDO_abort(SDO, 0x05040000L); //SDO protocol timed out
         }
      }
#endif

   }//end of (pre)operational state
   else{
      SDO->state = 0;
      SDO->CANrxNew = 0;
   }
}
