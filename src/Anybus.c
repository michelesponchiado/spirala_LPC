//
// Programma di gestione modulo Anybus
// - gestisce colloquio con modulo anybus tramite interfaccia uart bufferizzata fpga
//
//
//
//
//
//
//

#ifndef ARM9platform
    #include "upsd3400.h"
    #include "upsd3400_hardware.h"
    #include "upsd3400_timer.h"
    #include "intrins.h"
#endif
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <ctype.h>

//#include "LPC23xx.h"                        /* LPC23xx/24xx definitions */
#include "type.h"
#include "target.h"
#include "fpga_nxp.h"

#include "spiext.h"
#include "spiprot.h"
#include "spimsg.h"
#include "spipass.h"
#include "spifmt.h"
#include "max519.h"
#include "lcd.h"
#include "minmax.h"
#include "gui.h"
#include "spiwin.h"
#include "rtcDS3231.h"
#include "file_list.h"
#include "az4186_const.h"
#include "az4186_program_codes.h"
#include "az4186_commesse.h"
#include "az4186_spiralatrice.h"


extern void vValidateCodeList(void);
extern unsigned int uiGetNumValidationCodeList(void);
static unsigned int ui_modbus_error,ui_error_id;

unsigned short int us_swap_bytes(unsigned short int us_in){
	unsigned short int us_out;
	us_out=(us_in>>8)|((us_in&0xFF)<<8);
	return us_out;
}




//-- Modulo UART
//-- offset 50=0x32: tx/rx register
//-- offset 51=0x33: status register -->stato uart: 0=b7 | 0 | 0 | 0 | TxFull | TxEmpty | RxFull | RxEmpty=b0
//-- offset 52=0x34: latch per memorizzare info su code fifo
//-- offset 53=0x35: num bytes in fifo rx queue (low byte)
//-- offset 54=0x36: num bytes in fifo rx queue (hi  byte)
//-- offset 55=0x37: num bytes free in fifo tx queue (low byte)
//-- offset 56=0x38: num bytes free in fifo tx queue (hi  byte)

// maschere di selezione dei bit di stato dell'uart Anybus 
#define defUartAnybus_bitStatus_Rx_Empty 0x01
#define defUartAnybus_bitStatus_Rx_Full  0x02
#define defUartAnybus_bitStatus_Tx_Empty 0x04
#define defUartAnybus_bitStatus_Tx_Full  0x08

// dichiarazione della struttura che contiene lo stato dell'uart anybus
typedef struct _TipoStruct_UartAnybus_Status{
		unsigned char ucRx_Empty,ucRx_Full;
		unsigned char ucTx_Empty,ucTx_Full;
		unsigned int uiRx_NumBytesCanRead,uiTx_NumBytesCanWrite;
		unsigned char ucNumBytesTotalTx,ucNumBytesTotalRx;
		
}TipoStruct_UartAnybus_Status;

// numero max di caratteri che possono essere inviati alla fifo in "un giro" della macchina a stati di trasmissione
#define defUartAnybus_MaxChar2Tx 64
// potenza di due che definisce numero di elementi in coda rx uart anybus
// es se vale 4 la coda avra' dimensione 2**4=16
#define defUartAnybus_QueueSize2power 5
// numero max di caratteri nella coda uart anybus
#define defMaxBytesInUartAnybusQueue (1<<defUartAnybus_QueueSize2power)
// maschera per fare rapidamente il modulo degli indici nella coda rx uart anybus
// se ad es la coda ha 32 elementi, il modulo si ottiene facendo la AND con 32-1=31
#define defModulus_UartAnybusQueue ((defMaxBytesInUartAnybusQueue)-1)
// definizione del tipo: coda caratteri uart anybus
typedef struct _TipoStruct_UartAnybus_Queue{
		unsigned char ucQueue[defMaxBytesInUartAnybusQueue];
		unsigned int uiIdxWrite,uiIdxRead,uiNumElemInQueue;
	}TipoStruct_UartAnybus_Queue;
	
// definizione di un elemento della coda di post tx dell'uart anybus
typedef struct _TipoStruct_UartAnybus_PostTxCommand	{
	  unsigned char *pucChars2Tx; // puntatore ai caratteri da spedire
		unsigned int uiNumBytes2Tx; // numero totale di bytes da tx
		unsigned ucFinished;	  		// vale 1 se i bytes sono stati tutti trasmessi
	}TipoStruct_UartAnybus_PostTxCommand;
	
// enumerazione dei possibili stati di trasmissione
typedef enum{
		enumUartAnybus_TxStatus_idle=0,
		enumUartAnybus_TxStatus_sendBytes,
		enumUartAnybus_TxStatus_numOf
	}enumUartAnybus_TxStatus;
	
// dichiarazione della struttura che gestisce l'uart anybus
typedef struct _TipoStruct_UartAnybus{
	// definizione della variabile che contiene lo stato dell'uart anybus
	TipoStruct_UartAnybus_Status status;
	// comando di trasmissione
	TipoStruct_UartAnybus_PostTxCommand *ppost;
	// indica lo stato della trasmissione
	enumUartAnybus_TxStatus txStatus;
	// indica se un buffer in tx � disponibile
	unsigned char ucTxBufferAvalilable;
	// coda di ricezione
	TipoStruct_UartAnybus_Queue rxQueue;
}TipoStruct_UartAnybus;
	
// definizione della variabile che gestisce l'uart anybus
xdata TipoStruct_UartAnybus uartAnybus;

// inizializzazione uart
void vUartAnybus_Init(void){
	memset(&uartAnybus,0,sizeof(uartAnybus));
}//void vUartAnybus_Init(void)

// se la coda della seriale contiene almeno un carattere, lo restituisce 
// se seriale vuota, restituisce 0xFFFF
unsigned short int uiUartAnybus_getRxByte(void){
	unsigned short int uiReturnValue;
	register unsigned int uiAux;
	// se nessun elemento in coda, restituisce 0xFFFF
	if (uartAnybus.rxQueue.uiNumElemInQueue==0){
		return 0xFFFF;
	}
 	// salvo su registro veloce l'indice attuale
 	uiAux=uartAnybus.rxQueue.uiIdxRead;
	// salvo carattere
	uiReturnValue=uartAnybus.rxQueue.ucQueue[uiAux++];
	// avanzo l'indice
	uiAux&=defModulus_UartAnybusQueue;
	// aggiorno indice
	uartAnybus.rxQueue.uiIdxRead=uiAux;
	// decremento il numero di bytes in coda...
	uartAnybus.rxQueue.uiNumElemInQueue--;
	// restituisco carattere
	return uiReturnValue;
}//unsigned int uiUartAnybus_getRxByte(void)

// post di un comando di tramsissione
// restituisce 1 se il post � riuscito correttamente
// occhio perch� il buffer da tx deve essere mantenuto inalterato fino alla fine della tx.-..
unsigned char ucUartAnybus_PostTxCommand(TipoStruct_UartAnybus_PostTxCommand *ppost){
	// se trasmsissione non � idle, esci e ritorna 0
		if (uartAnybus.txStatus!=enumUartAnybus_TxStatus_idle){
				return 0;
		}
		// salvo struttura di spedizione
		uartAnybus.ppost=ppost;
		// indico che tx non � finita
		ppost->ucFinished=0;
		// indico che ho un buffer a disposizione!
		uartAnybus.ucTxBufferAvalilable=1;
	  return 1;
}

// funzione che rinfresca lo stato dell'uart Anybus
// riempie i campi della struttura uartAnybus_Status
// in modo che si possa sapere se ci sono caratteri in coda rx, quanti se ne possono trasmettere etc
void vUartAnybus_RefreshStatus(void){
	xdata unsigned char ucAux;
	register unsigned int uiAux;
	// leggo lo stato delle code fifo...
	ucUartAnybus_LatchInfos=1; // comando il latch delle info
	// leggo parte alta del dato
	uiAux=ucUartAnybus_NumBytesRx_Hi;
	// faccio or con parte bassa del dato
	uartAnybus.status.uiRx_NumBytesCanRead  =(uiAux<<8)|ucUartAnybus_NumBytesRx_Low;
	// leggo parte alta del dato
	uiAux=ucUartAnybus_NumBytesCanTx_Hi;
	// faccio or con parte bassa del dato
	uartAnybus.status.uiTx_NumBytesCanWrite =(uiAux<<8)|ucUartAnybus_NumBytesCanTx_Low;
	// leggo il byte di stato
	ucAux=ucUartAnybus_Status;
	// imposto i bit di stato delle code fifo
	uartAnybus.status.ucRx_Empty=ucAux&defUartAnybus_bitStatus_Rx_Empty;
	uartAnybus.status.ucRx_Full =ucAux&defUartAnybus_bitStatus_Rx_Full;
	uartAnybus.status.ucTx_Empty=ucAux&defUartAnybus_bitStatus_Tx_Empty;
	uartAnybus.status.ucTx_Full =ucAux&defUartAnybus_bitStatus_Tx_Full;
	uartAnybus.status.ucNumBytesTotalTx=ucUartAnybus_NumBytesTotalTx;
	uartAnybus.status.ucNumBytesTotalRx=ucUartAnybus_NumBytesTotalRx;
	
}//void vUartAnybus_RefreshStatus(void)

// programma di gestione uart anybus
// controlla se ci sono caratteri da ricevere/spedire
// 
void vUartAnybus_Handle(void){
	register unsigned int uiNumBytes,uiIdx;
	// rinfresco lo stato dell'uart
	vUartAnybus_RefreshStatus();
	// ricezione caratteri	
	// se fifo ha caratteri in coda e la coda di ricezione ha spazio...
	if (  uartAnybus.status.uiRx_NumBytesCanRead
		  &&uartAnybus.rxQueue.uiNumElemInQueue<sizeof(uartAnybus.rxQueue.ucQueue)
		 ){
		 	// numero di elementi che possono essere letti dalla fifo
		 	uiNumBytes=uartAnybus.status.uiRx_NumBytesCanRead;
		 	// verifico che ci stiano tutti in coda...
		 	if (uiNumBytes>sizeof(uartAnybus.rxQueue.ucQueue)-uartAnybus.rxQueue.uiNumElemInQueue){
		 		uiNumBytes=sizeof(uartAnybus.rxQueue.ucQueue)-uartAnybus.rxQueue.uiNumElemInQueue;
		 	}
		 	// salvo su registro veloce l'indice attuale
		 	uiIdx=uartAnybus.rxQueue.uiIdxWrite;
            // incremento il numero di bytes presenti nella coda rx
            uartAnybus.rxQueue.uiNumElemInQueue+=uiNumBytes;
		 	// loop di lettura caratteri da fifo uart a coda interna del micro
		 	while(uiNumBytes--){
		 		// salvo carattere
		 		uartAnybus.rxQueue.ucQueue[uiIdx++]=ucUartAnybus_TxRx;
		 		// avanzo l'indice
		 		uiIdx&=defModulus_UartAnybusQueue;
				// wait a little bit
				v_wait_microseconds(1);
				
		 	}
		 	// salvo il valore dell'indice
		 	uartAnybus.rxQueue.uiIdxWrite=uiIdx;	 		
	}//if (  uartAnybus.status.uiRx_NumBytesCanRead
	
	// gestione della procedura di tx
	switch(uartAnybus.txStatus){
		case enumUartAnybus_TxStatus_idle:
			// se NON abbiamo a disposizione un buffer da tx, restiamo in idle state!!!
			if (!uartAnybus.ucTxBufferAvalilable){
				break;
			}
			// fall through!!!
			uartAnybus.txStatus=enumUartAnybus_TxStatus_sendBytes;
			// passo diretto nella procedura di trasmissione!
		case enumUartAnybus_TxStatus_sendBytes:
			// se numero di bytes da tx nullo, fine tx
			if (uartAnybus.ppost->uiNumBytes2Tx==0){
				// fine tx
				if (uartAnybus.ppost){
					uartAnybus.ppost->ucFinished=1;
				}
				// buffer tx non + disponibile
				uartAnybus.ucTxBufferAvalilable=0;
				// torno nello stato idle
				uartAnybus.txStatus=enumUartAnybus_TxStatus_idle;
				break;
		  }
		  // trasmetto max "defUartAnybus_MaxChar2Tx" caratteri
		  uiNumBytes=uartAnybus.ppost->uiNumBytes2Tx;
		  if (uiNumBytes>defUartAnybus_MaxChar2Tx){
		  	uiNumBytes=defUartAnybus_MaxChar2Tx;
		  }
		  if (uiNumBytes>uartAnybus.status.uiTx_NumBytesCanWrite){
		  	uiNumBytes=uartAnybus.status.uiTx_NumBytesCanWrite;
		  }
		  uiIdx=0;
		  while (uiIdx<uiNumBytes){
		  	// inserisco byte in fifo...
		  	// incremento il puntatore ai caratteri da tx
		  	ucUartAnybus_TxRx=*(uartAnybus.ppost->pucChars2Tx++);
			uiIdx++;
			// wait a little bit
			v_wait_microseconds(1);
		  	
		  }
		  // modifico il numero di bytes da trasmettere
		  uartAnybus.ppost->uiNumBytes2Tx-=uiNumBytes;
			break;
		default:
			break;
	}
	
}//void vHandle_UartAnybus(void)

// definizione della struttura fragment protocol
typedef struct _TipoFragmentProtocol{
		unsigned char ucFragmentInProcessIndicator:1; // 1 se messaggio � frammento di un messaggio frammentato
		unsigned char ucLastFragmentIndicator:1; // 1 se questo � ultimo frammento di un messaggio frammentato
		unsigned char ucDummy:3;	// just 3 useless bits
		unsigned char ucFragmentSequenceNumber:3;	// numero di sequenza, che loopa fra 0 e 7
	}TipoFragmentProtocol;

// numero di bytes a disposizione per la parte dati di un messaggio hms
// max bytes nel fragment
#define defMaxDataBytes_HMSFragment (197)
// il messaggio deve contenere max defMaxDataBytes_HMSFragment bytes, cui va sottratta la parte di fragment protocol, class id, instance id, service code etc,
#define defMaxDataBytes_HMSObjectMessaging (defMaxDataBytes_HMSFragment-2-2-2-1)
// numero max di bytes nella query
#define defMaxDataBytes_HMSObjectMessaging_Query (defMaxDataBytes_HMSObjectMessaging)
// numero max di bytes nella response
#define defMaxDataBytes_HMSObjectMessaging_Response (defMaxDataBytes_HMSObjectMessaging-2)

// struttura dell' header di hms object message
typedef struct _TipoStruct_HMSObjectMessaging_Header{
	unsigned char ucFragmentByteCount; 	//fragment byte count: numero di bytes che seguono (escluso stuff byte)
	TipoFragmentProtocol ucFragmentProtocol; 	//fragment protocol
	unsigned short int  uiClassId; 						//class    id
	unsigned short int  uiInstanceId; 				//instance id
	unsigned short int  uiServiceCode; 				//service  code
}TipoStruct_HMSObjectMessaging_Header;

// struttura di un hms object message di tipo query: lo prendo come paradigma anche per la response
typedef struct _TipoStruct_HMSObjectMessaging{
	TipoStruct_HMSObjectMessaging_Header header;
	unsigned short int uiAttribute; // valore del campo attribute
	unsigned char ucData[defMaxDataBytes_HMSObjectMessaging_Query]; // parte dati del messaggio
// per semplicit� ometto di inserire lo stuff byte
//	unsigned char ucStuffByte; 				  //stuff byte (optional) se si spediscono sempre multipli di 2 bytes, questo stuff byte non viene usato mai
}TipoStruct_HMSObjectMessaging;

// struttura di un hms object message di tipo query/response
typedef struct _TipoStruct_HMSObjectMessaging_QueryResponse{
	TipoStruct_HMSObjectMessaging_Header header;
	unsigned short int uiErrorCode;
	unsigned char ucData[defMaxDataBytes_HMSObjectMessaging_Response]; // parte dati del messaggio
	unsigned char ucStuffByte; 				  //stuff byte (optional) se si spediscono sempre multipli di 2 bytes, questo stuff byte non viene usato mai
}TipoStruct_HMSObjectMessaging_Response;


// tipo di messaggi modbus supportati da modulo anybus
typedef enum{
	enumModbusFunctionCode_ReadRegisters							=0x03, // equivalente a 0x04
	enumModbusFunctionCode_WriteRegister 							=0x06,
	enumModbusFunctionCode_WriteMultipleRegisters 		=0x10,
//	enumModbusFunctionCode_ReadWriteMultipleRegisters =0x17,
	enumModbusFunctionCode_ObjectMessaging 						=0x5B
}enumModbusFunctionCode;

// corpo della read registers
typedef struct _TipoStructModbus_ReadRegisters{
		unsigned short int uiAddressOfFirstReg_msbFirst;
		unsigned short int uiNumOfReg_msbFirst;
}TipoStructModbus_ReadRegisters;

// corpo della read registers_response
typedef struct _TipoStructModbus_ReadRegisters_Response{
		unsigned char ucNumDataBytes;
		unsigned char ucDataBytes[256];
}TipoStructModbus_ReadRegisters_Response;

// corpo della write register
typedef struct _TipoStructModbus_WriteRegister{
		unsigned short int uiAddressOfFirstReg_msbFirst;
		unsigned short int uiValueToWrite_msbFirst;
}TipoStructModbus_WriteRegister;

// corpo della write register response
typedef struct _TipoStructModbus_WriteRegister_Response{
		unsigned short int uiAddressOfFirstReg_msbFirst;
		unsigned short int uiValueToWrite_msbFirst;
}TipoStructModbus_WriteRegister_Response;

// corpo della write multiple registers
typedef struct _TipoStructModbus_WriteMultipleRegisters{
		unsigned short int uiAddressOfFirstReg_msbFirst;
		unsigned short int uiNumOfReg2Write;
		unsigned char ucNumDataBytes;
		unsigned char ucDataBytes[256];
}TipoStructModbus_WriteMultipleRegisters;

// risposta alla write multiple registers
typedef struct _TipoStructModbus_WriteMultipleReg_Response{
		unsigned short int uiAddressOfFirstReg_msbFirst;
		unsigned short int uiNumOfRegWritten;
}TipoStructModbus_WriteMultipleReg_Response;

// messaggio modbus verso anybus
typedef struct _TipoStructModbusMessage{
	unsigned char ucSlaveAddress; 			// indirizzo dello slave hms, normalmente vale 1
	enumModbusFunctionCode ucFunctionCode;				// function code, deve essere 0x5B=91 per essere 'Object Messaging'
	union{
			TipoStructModbus_ReadRegisters rr;
			TipoStructModbus_WriteRegister wr;
			TipoStructModbus_WriteMultipleRegisters wmr;
			TipoStruct_HMSObjectMessaging om;
			TipoStructModbus_ReadRegisters_Response	rr_r;
			TipoStructModbus_WriteRegister_Response   wr_r;
			TipoStruct_HMSObjectMessaging_Response    om_r;
			TipoStructModbus_WriteMultipleReg_Response wmr_r;
		}u;
	// 16-bit modbus crc
	unsigned short int uiCrc;
	// numero di bytyes nel messaggio, crc compreso
	unsigned int uiNumBytesInMsg;
	unsigned char ucStuffBytePresent;      // indica se stuff byte presente...
}TipoStructModbusMessage;

// enumerazione stati gestione tx/rx messaggio
typedef enum{
		enumStatoTxRxModbusMessage_idle=0,
		enumStatoTxRxModbusMessage_init,
		enumStatoTxRxModbusMessage_tx,
		enumStatoTxRxModbusMessage_tx_wait,
		enumStatoTxRxModbusMessage_rx_wait,
		enumStatoTxRxModbusMessage_rx,
		enumStatoTxRxModbusMessage_rx_error,
		enumStatoTxRxModbusMessage_rx_purge,
		enumStatoTxRxModbusMessage_numOf
	}enumStatoTxRxModbusMessage;
	
// microstati di ricezione messaggio modbus...
typedef enum{
	enumStatoRxMsgModbus_Idle,
	enumStatoRxMsgModbus_SlaveAddress,
	enumStatoRxMsgModbus_FunctionCode,
	enumStatoRxMsgModbus_FC_RR_NumDataBytes,
	enumStatoRxMsgModbus_FC_RR_DataBytes,
	enumStatoRxMsgModbus_FC_WR_Address_Hi,
	enumStatoRxMsgModbus_FC_WR_Address_Lo,
	enumStatoRxMsgModbus_FC_WR_Value_Hi,
	enumStatoRxMsgModbus_FC_WR_Value_Lo,
	enumStatoRxMsgModbus_FC_OM_NumDataBytes,
	enumStatoRxMsgModbus_FC_OM_HeaderAndBody,
	
	enumStatoRxMsgModbus_FC_WMR_Address_Hi,
	enumStatoRxMsgModbus_FC_WMR_Address_Lo,
	enumStatoRxMsgModbus_FC_WMR_NumOf_HI,
	enumStatoRxMsgModbus_FC_WMR_NumOf_LO,
	
	
	enumStatoRxMsgModbus_CrcLo,
	enumStatoRxMsgModbus_CrcHi,
	enumStatoRxMsgModbus_Ok,
	enumStatoRxMsgModbus_Error
}enumStatoRxMsgModbus;

typedef enum {
		enumAnybus_Result_Idle=0,
		enumAnybus_Result_Transmitting,
		enumAnybus_Result_TxOk,
		enumAnybus_Result_TxKo
	}enumAnybus_Result;

	// b8..5 -> 0000 UINT, 0001 INT, 0010 STRING, 0011 STRING, 0100 FLOAT, 0101 BYTE ARRAY
typedef enum {
	enumApplicationParameter_dataType_UInt=0,
	enumApplicationParameter_dataType_Int =1,
// tipo 2 = bit string non gestito
//	enumApplicationParameter_dataType_bitString=2,
	enumApplicationParameter_dataType_String=3,
	enumApplicationParameter_dataType_Float=4,
	enumApplicationParameter_dataType_ByteArray=5
}enumApplicationParameter_dataType;
	
// definizione del tipo: application parameters
typedef struct _TipoStructAnybus_ApplicationParameters{
	// dimensione del parametro in numero di bytes
	// es char ->1byte
	// es int  ->2bytes
	// es float->4bytes
	// es string->1..32bytes (compreso NULL)
	unsigned int uiParameterSize;
	// b0 1->read access allowed, 0->read access not allowed
	unsigned char ucReadAccess;
	// b1 1->write access allowed, 0->write access not allowed
	unsigned char ucWriteAccess;
	// b8..5 -> 0000 UINT, 0001 INT, 0010 STRING, 0011 STRING, 0100 FLOAT, 0101 BYTE ARRAY
	enumApplicationParameter_dataType ucDataType;
	// b10..9 ->00 DEC, 01 HEX, 10 BIN, 11 DOTTED DECIMAL
	unsigned int uiMinValue;
	unsigned int uiMaxValue;
	unsigned int uiInitialValue;
	char cName[16];
	char cUnit[16];
	unsigned int *puiParamModbusAddress; // puntatore alla zona di memoria che conterr� indirizzo modbus di accesso al parametro
	// 1 se il parametro deve essere autorinfrescato
	unsigned char ucAutoRefresh;
	char *pcParam;
}TipoStructAnybus_ApplicationParameters;
	
// struttura gestione anybus
typedef struct _TipoHandleAnybus{
	// struttura che contiene il messaggio in trasmissione
	TipoStructModbusMessage modbusMessageTx;
	// struttura che contiene il messaggio in ricezione
	TipoStructModbusMessage modbusMessageRx;
	// post tx command
	TipoStruct_UartAnybus_PostTxCommand postTx;
	// stato ricezione messaggio
	enumStatoRxMsgModbus rxStato;
	// indice bytes ricevuti
	unsigned int uiIdxBytesRx;
	// puntatore molto utile con object messaging
	unsigned short int *pui;
	// 
	unsigned char *puc;
	unsigned char *puc_msgrx;
	// indice che specifica quale parametro si sta creando/usando
	unsigned int uiIdxParamAddress;
	// puntatore alla file instance
	unsigned int *puiFileInstance;
	// stato trasmissione/ricezione messaggi
	enumStatoTxRxModbusMessage stato;
	// esito della trasmissione del messaggio
	enumAnybus_Result result;
	// gestione timeout caratteri
	unsigned char ucCntInterrupt,ucOldCntInterrupt;
	unsigned int uiTimeElapsed,uiTimeMsgMs,uiTimeCharMs;
	TipoStructAnybus_ApplicationParameters *pApplicationParameter;
	
}TipoHandleAnybus;


unsigned short int uiModbus_Crc( unsigned char * pabMessage, unsigned int iLength )
{
   static code unsigned char abCrcHi[] =
   {
      0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
      0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
      0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
      0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
      0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
      0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
      0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
      0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
      0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
      0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
      0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
      0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
      0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
      0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
      0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
      0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
      0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
      0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
      0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
      0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
      0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
      0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
      0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
      0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
      0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
      0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
   };

   static code unsigned char abCrcLo[] =
   {
      0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
      0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
      0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
      0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
      0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
      0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
      0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
      0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
      0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
      0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
      0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
      0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
      0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
      0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
      0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
      0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
      0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
      0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
      0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
      0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
      0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
      0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
      0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
      0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
      0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
      0x43, 0x83, 0x41, 0x81, 0x80, 0x40
   };

   unsigned char bCrcHi = 0xFF;
   unsigned char bCrcLo = 0xFF;
   unsigned short int iIndex;

    while( iLength-- ){
        iIndex = bCrcHi ^ *pabMessage++;
        bCrcHi = bCrcLo ^ abCrcHi[iIndex];
        bCrcLo = abCrcLo[iIndex];
    }

    return( bCrcHi << 8 | bCrcLo );

}// end generate crc


unsigned char *puc_assign_us_swapped_bytes(unsigned short int us_in, unsigned char *p){
	// first, the high byte
	*p++=((us_in>>8)&0xFF);
	// then, the low byte
	*p++=(us_in&0xFF);
	return p;
}					


// gestione tx/rx messaggi anybus
xdata TipoHandleAnybus handleAnybusTxRx;

// permette di sapere se c'� una trasmissione in corso...
unsigned char ucAnybus_TxMessageBusy(void){
	if (handleAnybusTxRx.result==enumAnybus_Result_Transmitting)
		return 1;
	return 0;
}

// permette di sapere se c'� la trasmissione � finita ok...
unsigned char ucAnybus_TxMessageOk(void){
	if (handleAnybusTxRx.result==enumAnybus_Result_TxOk)
		return 1;
	return 0;
}


// inizializzazione anybus tx/rx
void vAnybusTxRx_Init(void){
	// reset uart anybus
	vUartAnybus_Init();
	memset(&handleAnybusTxRx,0,sizeof(handleAnybusTxRx));
}//void vAnybusTxRx_Init(void)


// timeout [ms] fra due caratteri anybus
#define defAnybus_CharTimeoutMs 200
// timeout [ms] ricezione messaggio anybus
#define defAnybus_MsgTimeoutMs 1000

// enumerazione dei parametri generali anybus
// li metto come define e non come enum perch� gli enum sono char e perderei la parte alta del valore enumerativo...
#define     enumAnybusGP_ModuleMode  	0x5001
#define 	enumAnybusGP_ModuleStatus	0x5002
#define 	enumAnybusGP_ModuleType		0x5003
#define 	enumAnybusGP_FieldBusType	0x5004
#define 	enumAnybusGP_LedState		0x5007
#define 	enumAnybusGP_ConfigBits		0x5008
#define 	enumAnybusGP_SwitchCoding	0x5009
#define 	enumAnybusGP_OfflineAction 0x500A
#define 	enumAnybusGP_IdleAction    0x500B
#define 	enumAnybusGP_InterruptConfig 0x500C
#define 	enumAnybusGP_InterruptCause  0x500D
#define 	enumAnybusGP_ModbusRTUAddress 0x5016
#define 	enumAnybusGP_ModbusCRCDisable 0x5017
// il mac address � contenuto in 6 bytes consecutivi
#define  enumAnybusGP_MAC_Address  	0x701B

typedef struct _TipoHma_ReadInitialParsTable{
	unsigned int uiAddress;
	unsigned int uiNumOf;
	unsigned int *pui;
}TipoHma_ReadInitialParsTable;


	
#if 0	
		1 0x5001 Module Mode 2 bytes - R/W
		2 0x5002 Module Status 2 bytes - R
		3 0x5003 Module Type 2 bytes - R
		4 0x5004 Fieldbus Type 2 bytes - R
		7 0x5007 LED State 2 bytes - R
		8 0x5008 Config Bits 2 bytes 0x0000 R/W
		9 0x5009 Switch Coding 1 byte Fieldbus dependant
		a. See separate fieldbus appendix
		R/W
		10 0x500A Offline Action 1 byte 0x00 R/W
		11 0x500B Idle Action 1 byte 0x00 R/W
		12 0x500C Interrupt Config 2 bytes 0x0001 R/W
		13 0x500D Interrupt Cause 2 bytes - R
		14 0x500E SCI Rate Config 1 byte 0x00 R/W
		15 0x500F SCI Rate Actual 1 byte - R
		16 0x5010 SCI Settings Config 1 byte 0x00 R/W
		17 0x5011 SCI Settings Actual 1 byte - R
		18 0x5012 MIF Rate Config 1 byte 0x04 R/W
		19 0x5013 MIF Rate Actual 1 byte - R
		20 0x5014 MIF Settings Config 1 byte 0x00 R/W
		21 0x5015 MIF Settings Actual 1 byte - R
		22 0x5016 Modbus RTU Address 1 byte 0x01 R/W
		23 0x5017 Modbus CRC Disable 1 byte 0x00 R/W
		27 0x501B... 0x5032 FB Fault Values 48 bytes 0x00 R/W	
#endif


// aggiornamento parametro user...
unsigned char ucPostAnybusMessage_updateParameter(TipoStructAnybus_ApplicationParameters *p){
	xdata unsigned int ui,i;
	if (handleAnybusTxRx.stato!=enumStatoTxRxModbusMessage_idle){
		return 0;
	}
	handleAnybusTxRx.stato=enumStatoTxRxModbusMessage_init;
	switch(p->ucDataType){
		case enumApplicationParameter_dataType_UInt:
		case enumApplicationParameter_dataType_Int:
			handleAnybusTxRx.modbusMessageTx.ucFunctionCode=enumModbusFunctionCode_WriteRegister;
			handleAnybusTxRx.modbusMessageTx.u.wr.uiValueToWrite_msbFirst=(*(unsigned int*)(p->pcParam));
			break;
		case enumApplicationParameter_dataType_String:
			//ui=strlen(p->pcParam)+1;
			ui=p->uiParameterSize;
			goto common_wmr;
		case enumApplicationParameter_dataType_Float:
			ui=sizeof(float);
			goto common_wmr;
		case enumApplicationParameter_dataType_ByteArray:
// patch!!! per ora imposto a 12 fisso il numero di bytes nella byte string...
			ui=12;
			goto common_wmr;
common_wmr:		
			handleAnybusTxRx.modbusMessageTx.ucFunctionCode=enumModbusFunctionCode_WriteMultipleRegisters;
			handleAnybusTxRx.modbusMessageTx.u.wmr.uiNumOfReg2Write=(ui/2);
			handleAnybusTxRx.modbusMessageTx.u.wmr.ucNumDataBytes=ui;
			i=0;
			while (i<ui){
				// swap the bytes, they will be reswapped by the send routine...
				if (i&1){
					handleAnybusTxRx.modbusMessageTx.u.wmr.ucDataBytes[i]=p->pcParam[i&~1];
				}
				else{
					handleAnybusTxRx.modbusMessageTx.u.wmr.ucDataBytes[i]=p->pcParam[i|1];
				}
				i++;
			}

			break;
	}
	handleAnybusTxRx.modbusMessageTx.u.wr.uiAddressOfFirstReg_msbFirst=(*(p->puiParamModbusAddress));
	handleAnybusTxRx.result=enumAnybus_Result_Transmitting;
	return 1;
}


// post di un messaggio anybus: general parameter write --> 
unsigned char ucPostAnybusMessage_generalParameterWrite(unsigned int generalParameter, unsigned int uiValue){
	if (handleAnybusTxRx.stato!=enumStatoTxRxModbusMessage_idle){
		return 0;
	}
	handleAnybusTxRx.stato=enumStatoTxRxModbusMessage_init;
	handleAnybusTxRx.modbusMessageTx.ucFunctionCode=enumModbusFunctionCode_WriteRegister;
	handleAnybusTxRx.modbusMessageTx.u.wr.uiAddressOfFirstReg_msbFirst=(generalParameter);
	handleAnybusTxRx.modbusMessageTx.u.wr.uiValueToWrite_msbFirst=(uiValue);
	handleAnybusTxRx.result=enumAnybus_Result_Transmitting;
	return 1;
}

// post di un messaggio anybus: general parameter read --> 
unsigned char ucPostAnybusMessage_generalParameterRead(unsigned int generalParameter, unsigned int uiNumOfReg){
	if (handleAnybusTxRx.stato!=enumStatoTxRxModbusMessage_idle){
		return 0;
	}
	handleAnybusTxRx.stato=enumStatoTxRxModbusMessage_init;
	handleAnybusTxRx.modbusMessageTx.ucFunctionCode=enumModbusFunctionCode_ReadRegisters;
	handleAnybusTxRx.modbusMessageTx.u.rr.uiAddressOfFirstReg_msbFirst=(generalParameter);
	handleAnybusTxRx.modbusMessageTx.u.rr.uiNumOfReg_msbFirst=(uiNumOfReg);
	handleAnybusTxRx.result=enumAnybus_Result_Transmitting;
	return 1;
}


// definizione delle varie classi di oggetti
#define defClassId_ApplicationParameter 0x0085
#define defClassId_FileObject 0x0086

// definizione dei vari tipi di servizi 
#define defServiceCode_Create 0x0005
#define defServiceCode_Create_Response (defServiceCode_Create+1)
#define defServiceCode_FileOpen 0x0080
#define defServiceCode_FileOpen_Response (defServiceCode_FileOpen+1)
#define defServiceCode_FileRead 0x00A0
#define defServiceCode_FileRead_Response (defServiceCode_FileRead+1)
#define defServiceCode_FileWrite 0x00A2
#define defServiceCode_FileWrite_Response (defServiceCode_FileWrite+1)
#define defServiceCode_FileClose 0x0082
#define defServiceCode_FileClose_Response (defServiceCode_FileClose+1)
#define defServiceCode_FileDelete 0x0084
#define defServiceCode_FileDelete_Response (defServiceCode_FileDelete+1)


unsigned char ucCreateApplicationParameter(TipoStructAnybus_ApplicationParameters *p){
	//unsigned short int * pui;
	unsigned char * puc;
	xdata float * xdata pf;
	if (handleAnybusTxRx.stato!=enumStatoTxRxModbusMessage_idle){
		return 0;
	}

	// salvo indice del parametro in esame
	handleAnybusTxRx.pApplicationParameter=p;

	handleAnybusTxRx.stato=enumStatoTxRxModbusMessage_init;
	handleAnybusTxRx.modbusMessageTx.ucFunctionCode=enumModbusFunctionCode_ObjectMessaging;
	handleAnybusTxRx.modbusMessageTx.u.om.header.ucFragmentByteCount=0x37;  //numero di bytes...
	*(char*)&(handleAnybusTxRx.modbusMessageTx.u.om.header.ucFragmentProtocol)=0x02;   //last fragment...
	handleAnybusTxRx.modbusMessageTx.u.om.header.uiClassId=(defClassId_ApplicationParameter);          //application parameter class
	handleAnybusTxRx.modbusMessageTx.u.om.header.uiInstanceId=(0x0000); 		//instance id
	handleAnybusTxRx.modbusMessageTx.u.om.header.uiServiceCode=(defServiceCode_Create); 	    //service  code=create
	handleAnybusTxRx.modbusMessageTx.u.om.uiAttribute=(0x0000); 				//attribute=0
	puc=(unsigned char *)(handleAnybusTxRx.modbusMessageTx.u.om.ucData);
	puc=puc_assign_us_swapped_bytes(p->uiParameterSize,puc);
	puc=puc_assign_us_swapped_bytes(0x0000,puc);
	puc=puc_assign_us_swapped_bytes((p->ucDataType<<5)|(p->ucReadAccess)|(p->ucWriteAccess<<1),puc);
	if (p->ucDataType==enumApplicationParameter_dataType_String){
		sprintf((char*)puc,"%*.*s",p->uiParameterSize,p->uiParameterSize," ");
		puc+=p->uiParameterSize;
		strcpy((char*)puc,p->cName);
		strcpy((char*)puc+16,p->cUnit);
		puc+=32;
		puc=puc_assign_us_swapped_bytes(0x0001,puc);
		handleAnybusTxRx.modbusMessageTx.u.om.header.ucFragmentByteCount=6+p->uiParameterSize+16+16+2+9;
	}
	else if (p->ucDataType==enumApplicationParameter_dataType_Float){
		//pui[1]=us_swap_bytes(0x0000); // descriptor msw--> the format is always decimal!
// float format is never used!!!!		
//	#error handle floating point value modbus format???	
		pf=(xdata float*)puc;
		*pf++=0.0; // minimum value
		*pf++=500.0; // maximum value
		*pf++=0.0; // initial value
		puc=(xdata unsigned char*)(pf);
		strcpy((char*)puc,p->cName);
		strcpy((char*)puc+16,p->cUnit);
		puc+=32;
		puc=puc_assign_us_swapped_bytes(0x0001,puc); // request modbus address
		handleAnybusTxRx.modbusMessageTx.u.om.header.ucFragmentByteCount=6+3*sizeof(float)+16+16+2+9;
	}
	else{
		puc=puc_assign_us_swapped_bytes(p->uiMinValue,puc);
		puc=puc_assign_us_swapped_bytes(p->uiMaxValue,puc);
		puc=puc_assign_us_swapped_bytes(p->uiInitialValue,puc);
		strcpy((char*)puc,p->cName);
		strcpy((char*)puc+16,p->cUnit);
		puc+=32;
		puc=puc_assign_us_swapped_bytes(0x0001,puc); // request modbus address
	}
	handleAnybusTxRx.result=enumAnybus_Result_Transmitting;
	return 1 ;
}

typedef enum {
	enumAnybusFileOpenMode_Read	  =0x00,
	enumAnybusFileOpenMode_Write  =0x01,
	enumAnybusFileOpenMode_Append =0x02
}enumAnybusFileOpenMode;

unsigned char ucAnybus_FileWrite(unsigned int uiFileInstance, unsigned char *pucFileBuffer , unsigned char ucNumByteFileBuffer){
	unsigned char * puc;
	if (handleAnybusTxRx.stato!=enumStatoTxRxModbusMessage_idle){
		return 0;
	}


	handleAnybusTxRx.stato=enumStatoTxRxModbusMessage_init;
	handleAnybusTxRx.modbusMessageTx.ucFunctionCode=enumModbusFunctionCode_ObjectMessaging;
	// il numero di byte lo forzo sempre ad essere pari per evitare il noioso byte stuffing
	handleAnybusTxRx.modbusMessageTx.u.om.header.ucFragmentByteCount=(9+2+ucNumByteFileBuffer);  //numero di bytes...
	*(char*)&(handleAnybusTxRx.modbusMessageTx.u.om.header.ucFragmentProtocol)=0x02;   //last fragment...
	handleAnybusTxRx.modbusMessageTx.u.om.header.uiClassId=(defClassId_FileObject);          //file class
	handleAnybusTxRx.modbusMessageTx.u.om.header.uiInstanceId=(uiFileInstance); 		//instance id
	handleAnybusTxRx.modbusMessageTx.u.om.header.uiServiceCode=(defServiceCode_FileWrite); 	    //service  code=file open
	handleAnybusTxRx.modbusMessageTx.u.om.uiAttribute=(0x0000); 				//attribute=0
	puc=(unsigned char*)(handleAnybusTxRx.modbusMessageTx.u.om.ucData);
	puc=puc_assign_us_swapped_bytes(ucNumByteFileBuffer,puc);
	// copio i caratteri da scrivere nel file
	while(ucNumByteFileBuffer--)
		*puc++=*pucFileBuffer++;
	// aggiungo un carattere per evitare problemi con lo stuffing
	puc[0]=0;
	handleAnybusTxRx.result=enumAnybus_Result_Transmitting;
	return 1 ;
}



unsigned char ucAnybus_FileRead(unsigned int uiFileInstance, unsigned char ucNumByteFileBuffer){
	unsigned char * puc;
	if (handleAnybusTxRx.stato!=enumStatoTxRxModbusMessage_idle){
		return 0;
	}

	handleAnybusTxRx.stato=enumStatoTxRxModbusMessage_init;
	handleAnybusTxRx.modbusMessageTx.ucFunctionCode=enumModbusFunctionCode_ObjectMessaging;
	handleAnybusTxRx.modbusMessageTx.u.om.header.ucFragmentByteCount=(9+2);  //numero di bytes... 9 (fisso) +2=numero di bytes richiesti
	*(char*)&(handleAnybusTxRx.modbusMessageTx.u.om.header.ucFragmentProtocol)=0x02;   //last fragment...
	handleAnybusTxRx.modbusMessageTx.u.om.header.uiClassId=(defClassId_FileObject);          //file class
	handleAnybusTxRx.modbusMessageTx.u.om.header.uiInstanceId=(uiFileInstance); 		//instance id
	handleAnybusTxRx.modbusMessageTx.u.om.header.uiServiceCode=(defServiceCode_FileRead); 	    //service  code=file read
	handleAnybusTxRx.modbusMessageTx.u.om.uiAttribute=(0x0000); 		
	// limito il numero max di bytes che possono essere richiesti...
	if (ucNumByteFileBuffer>defMaxDataBytes_HMSObjectMessaging_Query)
		ucNumByteFileBuffer=defMaxDataBytes_HMSObjectMessaging_Query;
	puc=(unsigned char*)(handleAnybusTxRx.modbusMessageTx.u.om.ucData);
	puc=puc_assign_us_swapped_bytes(ucNumByteFileBuffer,puc);
	// aggiungo un carattere per evitare problemi con lo stuffing
	puc[0]=0;
	handleAnybusTxRx.result=enumAnybus_Result_Transmitting;
	return 1 ;
}


unsigned char ucAnybus_FileClose(unsigned int uiFileInstance){
	unsigned char *puc;
	if (handleAnybusTxRx.stato!=enumStatoTxRxModbusMessage_idle){
		return 0;
	}


	handleAnybusTxRx.stato=enumStatoTxRxModbusMessage_init;
	handleAnybusTxRx.modbusMessageTx.ucFunctionCode=enumModbusFunctionCode_ObjectMessaging;
	// il numero di byte lo forzo sempre ad essere pari per evitare il noioso byte stuffing
	handleAnybusTxRx.modbusMessageTx.u.om.header.ucFragmentByteCount=(9+2);  //numero di bytes...
	*(char*)&(handleAnybusTxRx.modbusMessageTx.u.om.header.ucFragmentProtocol)=0x02;   //last fragment...
	handleAnybusTxRx.modbusMessageTx.u.om.header.uiClassId=(defClassId_FileObject);          //file class
	handleAnybusTxRx.modbusMessageTx.u.om.header.uiInstanceId=(0x0000); 		//instance id
	handleAnybusTxRx.modbusMessageTx.u.om.header.uiServiceCode=(defServiceCode_FileClose); 	    //service  code=file open
	handleAnybusTxRx.modbusMessageTx.u.om.uiAttribute=(0x0000); 				//attribute=0
	handleAnybusTxRx.result=enumAnybus_Result_Transmitting;
	puc=(unsigned char*)(handleAnybusTxRx.modbusMessageTx.u.om.ucData);
	puc=puc_assign_us_swapped_bytes(uiFileInstance,puc);
	return 1 ;
}


unsigned char ucAnybus_FileOpen(char *pFileName, enumAnybusFileOpenMode mode ,unsigned int *puiFileInstance){
	unsigned short int ui;
	unsigned char * puc;
	if (handleAnybusTxRx.stato!=enumStatoTxRxModbusMessage_idle){
		return 0;
	}

	// salvo indice del parametro in esame
	handleAnybusTxRx.puiFileInstance=puiFileInstance;
	// salvo la lunghezza del nome del file, compreso il null!!!!
	ui=strlen(pFileName)+1;

	handleAnybusTxRx.stato=enumStatoTxRxModbusMessage_init;
	handleAnybusTxRx.modbusMessageTx.ucFunctionCode=enumModbusFunctionCode_ObjectMessaging;
	// numero di bytes=
	//  9-->parte fissa
	//  1-->open mode
	//  n-->lunghezza filename+1 (DEVE ESSERE COMPRESO ANCHE IL NULL)
	handleAnybusTxRx.modbusMessageTx.u.om.header.ucFragmentByteCount=(9+1+ui);  //numero di bytes...
	*(char*)&(handleAnybusTxRx.modbusMessageTx.u.om.header.ucFragmentProtocol)=0x02;   //last fragment...
	handleAnybusTxRx.modbusMessageTx.u.om.header.uiClassId=(defClassId_FileObject);          //file class
	handleAnybusTxRx.modbusMessageTx.u.om.header.uiInstanceId=(0x0000); 		//instance id
	handleAnybusTxRx.modbusMessageTx.u.om.header.uiServiceCode=(defServiceCode_FileOpen); 	    //service  code=file open
	handleAnybusTxRx.modbusMessageTx.u.om.uiAttribute=(0x0000); 				//attribute=0
	puc=(unsigned char*)(handleAnybusTxRx.modbusMessageTx.u.om.ucData);
	*puc++=mode;
	// copio gli N caratteri del nome
	strcpy((char*)puc,pFileName);
	puc+=ui;
	// aggiungo carattere tappo che � necessario!!!
	puc[0]=0;
	handleAnybusTxRx.result=enumAnybus_Result_Transmitting;
	return 1 ;
}

unsigned char ucAnybus_FileDelete(char *pFileName){
	unsigned int ui;
	unsigned char * puc;
	if (handleAnybusTxRx.stato!=enumStatoTxRxModbusMessage_idle){
		return 0;
	}

	// salvo la lunghezza del nome del file, compreso il null!!!!
	ui=strlen(pFileName)+1;

	handleAnybusTxRx.stato=enumStatoTxRxModbusMessage_init;
	handleAnybusTxRx.modbusMessageTx.ucFunctionCode=enumModbusFunctionCode_ObjectMessaging;
	// numero di bytes=
	//  9-->parte fissa
	//  n-->lunghezza filename+1 (DEVE ESSERE COMPRESO ANCHE IL NULL)
	handleAnybusTxRx.modbusMessageTx.u.om.header.ucFragmentByteCount=(9+ui);  //numero di bytes...
	*(char*)&(handleAnybusTxRx.modbusMessageTx.u.om.header.ucFragmentProtocol)=0x02;   //last fragment...
	handleAnybusTxRx.modbusMessageTx.u.om.header.uiClassId=(defClassId_FileObject);          //file class
	handleAnybusTxRx.modbusMessageTx.u.om.header.uiInstanceId=(0x0000); 		//instance id
	handleAnybusTxRx.modbusMessageTx.u.om.header.uiServiceCode=(defServiceCode_FileDelete); 	    //service  code=file open
	handleAnybusTxRx.modbusMessageTx.u.om.uiAttribute=(0x0000); 				//attribute=0
	puc=(unsigned char*)(handleAnybusTxRx.modbusMessageTx.u.om.ucData);
	// copio gli N caratteri del nome
	strcpy((char*)puc,pFileName);
	puc+=ui;
	// aggiungo carattere tappo che � necessario!!!
	puc[0]=0;
	handleAnybusTxRx.result=enumAnybus_Result_Transmitting;
	return 1 ;
}


// reinizializza i timers di gestione timeout comunicazione modbus anybus
void vReinitTimers_AnybusTxRx(void){
	// inizializzo gestione timeout
	handleAnybusTxRx.uiTimeMsgMs=0;
	handleAnybusTxRx.uiTimeCharMs=0;
	handleAnybusTxRx.ucOldCntInterrupt=ucCntInterrupt;
	handleAnybusTxRx.ucCntInterrupt=handleAnybusTxRx.ucOldCntInterrupt;
}

// restituisce puntatore al primo byte della parte dati...
unsigned char *pucAnybus_GetReadRegMsgBody(void){
	return &(handleAnybusTxRx.modbusMessageRx.u.rr_r.ucDataBytes[0]);
}


unsigned char uc_message_to_anybus[255];

// gestione anybus tx/rx
void vAnybusTxRx_Handle(void){
	//unsigned char * puc;
	unsigned short int ui;
	unsigned char *p_msg_body;
	
	// gestisco anche uart anybus...
	vUartAnybus_Handle();
	// calcolo timeouts
	handleAnybusTxRx.ucCntInterrupt=ucCntInterrupt;
	if (handleAnybusTxRx.ucCntInterrupt!=handleAnybusTxRx.ucOldCntInterrupt){
		if (handleAnybusTxRx.ucCntInterrupt>handleAnybusTxRx.ucOldCntInterrupt){
			handleAnybusTxRx.uiTimeElapsed=(handleAnybusTxRx.ucCntInterrupt-handleAnybusTxRx.ucOldCntInterrupt);
		}
		else{
			handleAnybusTxRx.uiTimeElapsed=((((unsigned int)256)+handleAnybusTxRx.ucCntInterrupt)-handleAnybusTxRx.ucOldCntInterrupt);
		}
		handleAnybusTxRx.uiTimeElapsed*=PeriodoIntMs;
		handleAnybusTxRx.uiTimeMsgMs+=handleAnybusTxRx.uiTimeElapsed;
		handleAnybusTxRx.uiTimeCharMs+=handleAnybusTxRx.uiTimeElapsed;
		handleAnybusTxRx.ucOldCntInterrupt=ucCntInterrupt;
	}
	
	switch(handleAnybusTxRx.stato){
		case enumStatoTxRxModbusMessage_idle:
		default:
			break;
		case enumStatoTxRxModbusMessage_init:
			// imposto slave address al valore 1
			handleAnybusTxRx.modbusMessageTx.ucSlaveAddress=1;
			uc_message_to_anybus[0]=handleAnybusTxRx.modbusMessageTx.ucSlaveAddress;
			uc_message_to_anybus[1]=handleAnybusTxRx.modbusMessageTx.ucFunctionCode;
			p_msg_body=&uc_message_to_anybus[2];
			// a seconda del tipo di messaggio, calcolo il numero di bytes da spedire
			switch(handleAnybusTxRx.modbusMessageTx.ucFunctionCode){
				case enumModbusFunctionCode_ReadRegisters:
					// <slave addr> + <function code> + <corpo read registers>+<crc>
					//     1               1                    4                2
					handleAnybusTxRx.modbusMessageTx.uiNumBytesInMsg=2+sizeof(TipoStructModbus_ReadRegisters)+2;
					p_msg_body=puc_assign_us_swapped_bytes(handleAnybusTxRx.modbusMessageTx.u.rr.uiAddressOfFirstReg_msbFirst, 	p_msg_body);
					p_msg_body=puc_assign_us_swapped_bytes(handleAnybusTxRx.modbusMessageTx.u.rr.uiNumOfReg_msbFirst, 			p_msg_body);
					break;
				case enumModbusFunctionCode_WriteRegister:
					// <slave addr> + <function code> + <corpo write register>+<crc>
					//     1               1                    4                2
					handleAnybusTxRx.modbusMessageTx.uiNumBytesInMsg=2+sizeof(TipoStructModbus_WriteRegister)+2;
					p_msg_body=puc_assign_us_swapped_bytes(handleAnybusTxRx.modbusMessageTx.u.wr.uiAddressOfFirstReg_msbFirst, 	p_msg_body);
					p_msg_body=puc_assign_us_swapped_bytes(handleAnybusTxRx.modbusMessageTx.u.wr.uiValueToWrite_msbFirst, 		p_msg_body);
					break;
				case enumModbusFunctionCode_WriteMultipleRegisters:
					// <slave addr> + <function code> + <corpo write multiple register>+<crc>
					//     1               1                    n                2
					handleAnybusTxRx.modbusMessageTx.uiNumBytesInMsg=
						 2
						+sizeof(TipoStructModbus_WriteMultipleRegisters)
						-sizeof(handleAnybusTxRx.modbusMessageTx.u.wmr.ucDataBytes)
						+handleAnybusTxRx.modbusMessageTx.u.wmr.ucNumDataBytes
						+2;
					p_msg_body=puc_assign_us_swapped_bytes(handleAnybusTxRx.modbusMessageTx.u.wmr.uiAddressOfFirstReg_msbFirst, 	p_msg_body);
					p_msg_body=puc_assign_us_swapped_bytes(handleAnybusTxRx.modbusMessageTx.u.wmr.uiNumOfReg2Write, 		p_msg_body);
					*p_msg_body++=handleAnybusTxRx.modbusMessageTx.u.wmr.ucNumDataBytes;
					{
						unsigned int ui_idx_reg;
						unsigned char *mypuc;
						mypuc=handleAnybusTxRx.modbusMessageTx.u.wmr.ucDataBytes;
						ui_idx_reg=0;
						while(ui_idx_reg*2<handleAnybusTxRx.modbusMessageTx.u.wmr.ucNumDataBytes){
							*p_msg_body++=mypuc[1];
							*p_msg_body++=mypuc[0];
							mypuc+=2;
							ui_idx_reg++;
						}
					}
					break;
//				case enumModbusFunctionCode_ReadWriteMultipleRegisters:
					//break;
				case enumModbusFunctionCode_ObjectMessaging:
					// <slave addr> + <function code> + <corpo object messaging>+<crc>
					//     1               1                    4                2
				//	handleAnybusTxRx.modbusMessageTx.uiNumBytesInMsg=2+sizeof(TipoStructModbus_WriteMultipleRegisters)-sizeof()+2;
					handleAnybusTxRx.modbusMessageTx.uiNumBytesInMsg=
						 2
						+sizeof(TipoStruct_HMSObjectMessaging)
						-sizeof(handleAnybusTxRx.modbusMessageTx.u.om.ucData)
						+handleAnybusTxRx.modbusMessageTx.u.om.header.ucFragmentByteCount
						+2;
					// gestione stuff byte
					// se fragment byte count � pari, la parte dati contiene numero dispari di byte, ne devo aggiungere uno...
					if ((handleAnybusTxRx.modbusMessageTx.u.om.header.ucFragmentByteCount&1)==0){
						handleAnybusTxRx.modbusMessageTx.uiNumBytesInMsg++;
					}
					*p_msg_body++=handleAnybusTxRx.modbusMessageTx.u.om.header.ucFragmentByteCount;
					*p_msg_body++=*(unsigned char *)&handleAnybusTxRx.modbusMessageTx.u.om.header.ucFragmentProtocol;
					p_msg_body=puc_assign_us_swapped_bytes(handleAnybusTxRx.modbusMessageTx.u.om.header.uiClassId, 			p_msg_body);
					p_msg_body=puc_assign_us_swapped_bytes(handleAnybusTxRx.modbusMessageTx.u.om.header.uiInstanceId, 		p_msg_body);
					p_msg_body=puc_assign_us_swapped_bytes(handleAnybusTxRx.modbusMessageTx.u.om.header.uiServiceCode, 		p_msg_body);
					p_msg_body=puc_assign_us_swapped_bytes(handleAnybusTxRx.modbusMessageTx.u.om.uiAttribute, 				p_msg_body);
					{
						int i_num_bytes2_copy;
						unsigned char *mypuc;
						i_num_bytes2_copy=handleAnybusTxRx.modbusMessageTx.u.om.header.ucFragmentByteCount;
						// tolgo i 9 caratteri di parte fissa sempre presenti (classId etc)
						i_num_bytes2_copy-=9;
						// eventuale stuff byte...
						if ((handleAnybusTxRx.modbusMessageTx.u.om.header.ucFragmentByteCount&1)==0){
							i_num_bytes2_copy++;
						}
						mypuc=handleAnybusTxRx.modbusMessageTx.u.om.ucData;
						while(i_num_bytes2_copy>0){
							*p_msg_body++=*mypuc++;
							i_num_bytes2_copy--;
						}
					}
					
					break;
			}
			// calcolo il crc del messaggio, e lo inserisco dopo l'ultimo byte del messaggio
			ui=uiModbus_Crc( uc_message_to_anybus, p_msg_body-uc_message_to_anybus);
			*p_msg_body++=ui>>8;   // save high crc byte first
			*p_msg_body++=ui&0xFF; // save low  crc byte then
			handleAnybusTxRx.modbusMessageTx.uiNumBytesInMsg=p_msg_body-uc_message_to_anybus;
			
			// salvo il numero di bytes da tx
			handleAnybusTxRx.postTx.uiNumBytes2Tx=handleAnybusTxRx.modbusMessageTx.uiNumBytesInMsg;
			handleAnybusTxRx.postTx.pucChars2Tx=uc_message_to_anybus;
			handleAnybusTxRx.stato=enumStatoTxRxModbusMessage_tx;
			break;
						
		case enumStatoTxRxModbusMessage_tx:
			if (ucUartAnybus_PostTxCommand(&(handleAnybusTxRx.postTx))){
					handleAnybusTxRx.stato=enumStatoTxRxModbusMessage_tx_wait;
					break;
			}
			break;
		case enumStatoTxRxModbusMessage_tx_wait:
			// se trasmissione finita, passo in ricezione
			if (handleAnybusTxRx.postTx.ucFinished){
				vReinitTimers_AnybusTxRx();
				// cambio stato
				handleAnybusTxRx.stato=enumStatoTxRxModbusMessage_rx_wait;
				handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_SlaveAddress;
			}
			break;
		case enumStatoTxRxModbusMessage_rx_wait:
			// ricezione byte...
			ui=uiUartAnybus_getRxByte();
			// se nessun byte ricevuto, controllo timeout carattere
			if (ui&0xFF00){
				// se sono in error o timeout allora procedo col passaggio di stato anche se non ho ricevuto caratteri...
				if ( (handleAnybusTxRx.rxStato!=enumStatoRxMsgModbus_Ok)
				   &&(handleAnybusTxRx.rxStato!=enumStatoRxMsgModbus_Error)
				   ){
					// max TOTms per ricevere un carattere
					// se timeout esco!
					if (handleAnybusTxRx.uiTimeCharMs>defAnybus_CharTimeoutMs){
						ui_error_id=0;
						handleAnybusTxRx.stato=enumStatoTxRxModbusMessage_rx_error;
						break;
					}
					break;
				}
			}
			// rcevuto carattere, azzero timeout carattere
			else{
				// azzero timeout carattere
				handleAnybusTxRx.uiTimeCharMs=0;
			}
			// max TOT secondi per ricevere un messaggio
			if (handleAnybusTxRx.uiTimeMsgMs>defAnybus_MsgTimeoutMs){
				ui_error_id=1;
				handleAnybusTxRx.stato=enumStatoTxRxModbusMessage_rx_error;
				break;
			}
			// verifico qual � lo stato di ricezione carattere
			switch(handleAnybusTxRx.rxStato){
				case enumStatoRxMsgModbus_SlaveAddress:
					// azzero flag presenza stuff byte
					handleAnybusTxRx.modbusMessageRx.ucStuffBytePresent=0;
					handleAnybusTxRx.modbusMessageRx.ucSlaveAddress=ui;
					// se slave address non � uguale ad 1, errore!!!
					if (ui!=0x1){
						ui_modbus_error=1;
						handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_Error;
						break;
				  }
					handleAnybusTxRx.puc_msgrx=uc_message_to_anybus;
					*handleAnybusTxRx.puc_msgrx++=ui;
				  
				  // passo nello stato codice funzione
				  handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_FunctionCode;
					break;
				case enumStatoRxMsgModbus_FunctionCode:
					// salvo il function code
					handleAnybusTxRx.modbusMessageRx.ucFunctionCode=ui;
					*handleAnybusTxRx.puc_msgrx++=ui;
					
					// salvo il function code
					switch(ui){
						case enumModbusFunctionCode_ReadRegisters:
							handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_FC_RR_NumDataBytes;
							break;
						case enumModbusFunctionCode_WriteRegister:
							handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_FC_WR_Address_Hi;
							break;
						case enumModbusFunctionCode_WriteMultipleRegisters:
							handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_FC_WMR_Address_Hi;
							break;
						case enumModbusFunctionCode_ObjectMessaging:
							handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_FC_OM_NumDataBytes;
							break;
						default:
							ui_modbus_error=2;
							handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_Error;
							break;
					}
					break;
				case enumStatoRxMsgModbus_FC_OM_NumDataBytes:
					handleAnybusTxRx.modbusMessageRx.u.om_r.header.ucFragmentByteCount=ui;
					*handleAnybusTxRx.puc_msgrx++=ui;
					
					// se fragment byte count � pari allora c'� lo stuff byte!
					if ((ui&1)==0)
						handleAnybusTxRx.modbusMessageRx.ucStuffBytePresent=1;
					handleAnybusTxRx.uiIdxBytesRx=0;
					handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_FC_OM_HeaderAndBody;
					break;
				case enumStatoRxMsgModbus_FC_OM_HeaderAndBody:
					*handleAnybusTxRx.puc_msgrx++=ui;
				
					if (handleAnybusTxRx.uiIdxBytesRx==0){
						*(char*)&(handleAnybusTxRx.modbusMessageRx.u.om_r.header.ucFragmentProtocol)=ui;
						handleAnybusTxRx.puc=(unsigned char *)&handleAnybusTxRx.modbusMessageRx.u.om_r.header.uiClassId;
					}
					else if (handleAnybusTxRx.uiIdxBytesRx&1){
						handleAnybusTxRx.puc[1]=ui&0xFF;
					}
					else{
						handleAnybusTxRx.puc[0]=ui&0xFF;
						handleAnybusTxRx.puc+=2;
					}
					if (++handleAnybusTxRx.uiIdxBytesRx>=
							handleAnybusTxRx.modbusMessageRx.u.om_r.header.ucFragmentByteCount
						  +handleAnybusTxRx.modbusMessageRx.ucStuffBytePresent
						){
						// era stato richiesto il create di un parametro???
						// ne salvo il modbus address
						if (  (handleAnybusTxRx.modbusMessageRx.u.om.header.uiClassId==defClassId_ApplicationParameter)
							&&(handleAnybusTxRx.modbusMessageRx.u.om.header.uiServiceCode==defServiceCode_Create_Response)
						   ){
							handleAnybusTxRx.pui=&handleAnybusTxRx.modbusMessageRx.u.om_r.header.uiClassId;
							// copio l'indirizzo modbus nella zona indicata...
							handleAnybusTxRx.pApplicationParameter->puiParamModbusAddress[0]=handleAnybusTxRx.pui[6];
						}
						// richiesta operazione su file (open)???
						else if (	(handleAnybusTxRx.modbusMessageRx.u.om.header.uiClassId==defClassId_FileObject)
								  &&(handleAnybusTxRx.modbusMessageRx.u.om.header.uiServiceCode==defServiceCode_FileOpen_Response)
							){
							handleAnybusTxRx.pui=&handleAnybusTxRx.modbusMessageRx.u.om_r.header.uiClassId;
							// copio il file instance nella zona indicata...
							handleAnybusTxRx.puiFileInstance[0]=handleAnybusTxRx.pui[4];
						}
						handleAnybusTxRx.modbusMessageRx.uiNumBytesInMsg=
						// sommo address+function code+fragment byte+dait+bytes del crc
							 2 +1+handleAnybusTxRx.modbusMessageRx.u.om_r.header.ucFragmentByteCount+2;
						handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_CrcHi;
					}
					break;
				case enumStatoRxMsgModbus_FC_RR_NumDataBytes:
					*handleAnybusTxRx.puc_msgrx++=ui;				
					handleAnybusTxRx.modbusMessageRx.u.rr_r.ucNumDataBytes=ui;
					handleAnybusTxRx.uiIdxBytesRx=0;
					handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_FC_RR_DataBytes;
					break;
				case enumStatoRxMsgModbus_FC_RR_DataBytes:
					*handleAnybusTxRx.puc_msgrx++=ui;				
					handleAnybusTxRx.modbusMessageRx.u.rr_r.ucDataBytes[handleAnybusTxRx.uiIdxBytesRx++]=ui;
					if (handleAnybusTxRx.uiIdxBytesRx>=handleAnybusTxRx.modbusMessageRx.u.rr_r.ucNumDataBytes){
						handleAnybusTxRx.modbusMessageRx.uiNumBytesInMsg=
							 2 // modbus header
							+sizeof(TipoStructModbus_ReadRegisters_Response) // corpo del messaggio di risposta
							+2 // crc
							-sizeof(handleAnybusTxRx.modbusMessageRx.u.rr_r.ucDataBytes) // parte dati fissa
							+handleAnybusTxRx.modbusMessageRx.u.rr_r.ucNumDataBytes  // parte dati effettivamente giunta
							;
						handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_CrcHi;
						break;
					}
					break;
				case enumStatoRxMsgModbus_FC_WMR_Address_Hi:
					*handleAnybusTxRx.puc_msgrx++=ui;				
					handleAnybusTxRx.modbusMessageRx.u.wmr_r.uiAddressOfFirstReg_msbFirst=ui<<8;
					handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_FC_WMR_Address_Lo;
					break;
					
				case enumStatoRxMsgModbus_FC_WMR_Address_Lo:
					*handleAnybusTxRx.puc_msgrx++=ui;
					handleAnybusTxRx.modbusMessageRx.u.wmr_r.uiAddressOfFirstReg_msbFirst|=ui&0xff;
					// verifico che l'indirizzo sia corretto
					if (  handleAnybusTxRx.modbusMessageRx.u.wmr_r.uiAddressOfFirstReg_msbFirst
						   !=
						   (handleAnybusTxRx.modbusMessageTx.u.wmr.uiAddressOfFirstReg_msbFirst)
						 ){
						ui_modbus_error=3;
						 handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_Error;
						 break;
					}
					handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_FC_WMR_NumOf_HI;
					break;
				case enumStatoRxMsgModbus_FC_WMR_NumOf_HI:
					*handleAnybusTxRx.puc_msgrx++=ui;
					handleAnybusTxRx.modbusMessageRx.u.wmr_r.uiNumOfRegWritten=ui<<8;
					handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_FC_WMR_NumOf_LO;
					break;
				case enumStatoRxMsgModbus_FC_WMR_NumOf_LO:
					*handleAnybusTxRx.puc_msgrx++=ui;
					handleAnybusTxRx.modbusMessageRx.u.wmr_r.uiNumOfRegWritten|=ui&0xff;
					// verifico che il numero di regs sia corretto
					if (  handleAnybusTxRx.modbusMessageRx.u.wmr_r.uiNumOfRegWritten
						   !=
						   (handleAnybusTxRx.modbusMessageTx.u.wmr.uiNumOfReg2Write)
						 ){
						ui_modbus_error=4;
						 handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_Error;
						 break;
					}
					// sempre 8 bytes nella risposta alla write multiple registers
					handleAnybusTxRx.modbusMessageRx.uiNumBytesInMsg=8;
					handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_CrcHi;
					break;
					
				case enumStatoRxMsgModbus_FC_WR_Address_Hi:
					*handleAnybusTxRx.puc_msgrx++=ui;
					handleAnybusTxRx.modbusMessageRx.u.wr_r.uiAddressOfFirstReg_msbFirst=ui<<8;
					handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_FC_WR_Address_Lo;
					break;
				case enumStatoRxMsgModbus_FC_WR_Address_Lo:
					*handleAnybusTxRx.puc_msgrx++=ui;
					handleAnybusTxRx.modbusMessageRx.u.wr_r.uiAddressOfFirstReg_msbFirst|=ui&0xff;
					// verifico che l'indirizzo sia corretto
					if (  handleAnybusTxRx.modbusMessageRx.u.wr_r.uiAddressOfFirstReg_msbFirst
						   !=
						   (handleAnybusTxRx.modbusMessageTx.u.wr.uiAddressOfFirstReg_msbFirst)
						 ){
						ui_modbus_error=5;
						 handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_Error;
						 break;
					}
					handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_FC_WR_Value_Hi;
					break;
				case enumStatoRxMsgModbus_FC_WR_Value_Hi:
					*handleAnybusTxRx.puc_msgrx++=ui;
					handleAnybusTxRx.modbusMessageRx.u.wr_r.uiValueToWrite_msbFirst=ui<<8;
					handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_FC_WR_Value_Lo;
					break;
				case enumStatoRxMsgModbus_FC_WR_Value_Lo:
					*handleAnybusTxRx.puc_msgrx++=ui;
					handleAnybusTxRx.modbusMessageRx.u.wr_r.uiValueToWrite_msbFirst|=ui&0xff;
					// verifico che il valore sia stato scritto correttamente
					if (  handleAnybusTxRx.modbusMessageRx.u.wr_r.uiValueToWrite_msbFirst
						   !=
						   (handleAnybusTxRx.modbusMessageTx.u.wr.uiValueToWrite_msbFirst)
						 ){
						ui_modbus_error=6;
						 handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_Error;
						 break;
					}
					// la risposta ad una write register � sempre composta da 8 bytes
					handleAnybusTxRx.modbusMessageRx.uiNumBytesInMsg=8;
					handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_CrcHi;
					break;
				case enumStatoRxMsgModbus_CrcHi:
					handleAnybusTxRx.modbusMessageRx.uiCrc=ui<<8;
					handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_CrcLo;
					break;
				case enumStatoRxMsgModbus_CrcLo:
					handleAnybusTxRx.modbusMessageRx.uiCrc|=ui&0xFF;
					// calcolo il crc del msg giunto...
					ui=uiModbus_Crc(uc_message_to_anybus,handleAnybusTxRx.puc_msgrx-uc_message_to_anybus);
					if (handleAnybusTxRx.modbusMessageRx.uiCrc!=ui){
						ui_modbus_error=7;
							handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_Error;
							break;
					}
#if 0					
                    // swap the bytes in the body
                    if (handleAnybusTxRx.modbusMessageRx.ucFunctionCode==enumModbusFunctionCode_ObjectMessaging){
                        unsigned int ui_num_word_to_swap,ui_num_word_swapped;
						handleAnybusTxRx.puc=(unsigned char *)&handleAnybusTxRx.modbusMessageRx.u.om_r.header.uiClassId;
                        ui_num_word_to_swap=(handleAnybusTxRx.modbusMessageRx.u.om_r.header.ucFragmentByteCount-1)/2;
                        ui_num_word_swapped=0;
                        while(ui_num_word_swapped<ui_num_word_to_swap){
                            {
                                unsigned char uc;
                                uc = handleAnybusTxRx.puc[0];
                                handleAnybusTxRx.puc[0]=handleAnybusTxRx.puc[1];
                                handleAnybusTxRx.puc[1]=uc;
                                handleAnybusTxRx.puc+=2;
                            }
                            ui_num_word_swapped++;
                        }
					}
#endif					
					handleAnybusTxRx.rxStato=enumStatoRxMsgModbus_Ok;
					break;
				case enumStatoRxMsgModbus_Ok:
				  	handleAnybusTxRx.stato=enumStatoTxRxModbusMessage_rx;
					break;
				case enumStatoRxMsgModbus_Error:
					ui_error_id=2;
				  	handleAnybusTxRx.stato=enumStatoTxRxModbusMessage_rx_error;
					break;
				case enumStatoRxMsgModbus_Idle:
				default:
				{
					break;
				}
			}			
			break;
		case enumStatoTxRxModbusMessage_rx:
			// verifico se numero di bytes scritti ed il loro valore coincidono
			// segnalare che � giunto un messaggio valido di risposta
			handleAnybusTxRx.stato=enumStatoTxRxModbusMessage_idle;
			handleAnybusTxRx.result=enumAnybus_Result_TxOk;
			break;
		case enumStatoTxRxModbusMessage_rx_error:
			// segnalare che � NON giunto un messaggio valido di risposta
			// purgare la seriale...
			handleAnybusTxRx.stato=enumStatoTxRxModbusMessage_rx_purge;
			vReinitTimers_AnybusTxRx();
			break;
		case enumStatoTxRxModbusMessage_rx_purge:
			// aspetto 1 secondo per purgare l'uart...
			if (handleAnybusTxRx.uiTimeMsgMs>1000){		
				handleAnybusTxRx.stato=enumStatoTxRxModbusMessage_idle;
				// reinizializzo uart...
				vUartAnybus_Init();
				// do' iol via libera
				handleAnybusTxRx.result=enumAnybus_Result_TxKo;
			}
			break;
		}
}

// funzione che restituisce il codice di errore dell'ultima operazione file eseguita
static unsigned int uiFileOperationErrorCode(void){
	return handleAnybusTxRx.modbusMessageRx.u.om_r.uiErrorCode;
}
// funzione che restituisce il puntatore alla parte dati dell'ultima operazione file eseguita
static unsigned char *pucFileOperationData(void){
	return &(handleAnybusTxRx.modbusMessageRx.u.om_r.ucData[0]);
}


// numero minimo di messaggi anybus txrx ok per poter dire che autobaud � ok
#define defHMA_MinNumMsgOKTxRx 5
// numero minimo di messaggi di autobaud
#define defHMA_MinNumMsg_Autobaud (defHMA_MinNumMsgOKTxRx+20)
// numero max di application parameters handled...
#define defMaxNumAnybusApplParams 50

// definizione dei tipi dello stato invio messaggi anybus
typedef enum{
		enumStatusMainAnybus_InitAutobaud=0,
		enumStatusMainAnybus_DoAutobaud,
		enumStatusMainAnybus_LoopAutobaud,
		enumStatusMainAnybus_ResetModule,
		enumStatusMainAnybus_ResetModule_Wait,

		enumStatusMainAnybus_SetNormalMode,
		enumStatusMainAnybus_SetNormalMode_wait,
		enumStatusMainAnybus_ReadInitialParams,
		enumStatusMainAnybus_ReadInitialParams_wait,
		enumStatusMainAnybus_SetApplicationParam,
		enumStatusMainAnybus_SetApplicationParam_Wait,


		enumStatusMainAnybus_RegenProductCodes,
		enumStatusMainAnybus_RegenProductCodes_handle,
		enumStatusMainAnybus_RegenProductCodes_init,
		enumStatusMainAnybus_RegenProductCodes_init_wait,
		enumStatusMainAnybus_RegenProductCodes_fopen,
		enumStatusMainAnybus_RegenProductCodes_fopen_wait,
		enumStatusMainAnybus_RegenProductCodes_fprintf,
		enumStatusMainAnybus_RegenProductCodes_fprintf_wait,
		enumStatusMainAnybus_RegenProductCodes_fclose,
		enumStatusMainAnybus_RegenProductCodes_fclose_wait,
		enumStatusMainAnybus_RegenProductCodes_end,

		enumStatusMainAnybus_ReloadProductCodes,
		enumStatusMainAnybus_ReloadProductCodes_ctrlNeeded,
		enumStatusMainAnybus_ReloadProductCodes_start,
		enumStatusMainAnybus_ReloadProductCodes_waitYesNo,
		enumStatusMainAnybus_ReloadProductCodes_Yes,
		enumStatusMainAnybus_ReloadProductCodes_handle,
		enumStatusMainAnybus_ReloadProductCodes_init,
		enumStatusMainAnybus_ReloadProductCodes_fopen,
		enumStatusMainAnybus_ReloadProductCodes_fgets,
		enumStatusMainAnybus_ReloadProductCodes_fgets_do,
		enumStatusMainAnybus_ReloadProductCodes_fgets_wait,
		enumStatusMainAnybus_ReloadProductCodes_fclose,
		enumStatusMainAnybus_ReloadProductCodes_fclose_wait,
		enumStatusMainAnybus_ReloadProductCodes_fclose_delete,
		enumStatusMainAnybus_ReloadProductCodes_fclose_delete_wait,
		enumStatusMainAnybus_ReloadProductCodes_error,
		enumStatusMainAnybus_ReloadProductCodes_end,
		enumStatusMainAnybus_ReloadProductCodes_end_waitopen,
		enumStatusMainAnybus_ReloadProductCodes_end_writeresult,
		enumStatusMainAnybus_ReloadProductCodes_end_wait_writeresult,
		enumStatusMainAnybus_ReloadProductCodes_end_fclose,
		enumStatusMainAnybus_ReloadProductCodes_end_fclose_wait,
		enumStatusMainAnybus_ReloadProductCodes_end_all,





		enumStatusMainAnybus_Jobs_Refresh,
		enumStatusMainAnybus_Jobs_Refresh_wait_ctrl_if,
		enumStatusMainAnybus_Jobs_Refresh_wait_close_andgo,
		enumStatusMainAnybus_Jobs_Refresh_doit,
		enumStatusMainAnybus_Jobs_waitYesNo,
		enumStatusMainAnybus_Jobs_Refresh_delfile,
		enumStatusMainAnybus_Jobs_Refresh_delfile_wait,
		enumStatusMainAnybus_Jobs_Refresh_end,
		enumStatusMainAnybus_Jobs_Refresh_end_open_wait,
		enumStatusMainAnybus_Jobs_Refresh_end_writeresult,
		enumStatusMainAnybus_Jobs_Refresh_end_wait_writeresult,
		enumStatusMainAnybus_Jobs_Refresh_end_fclose,
		enumStatusMainAnybus_Jobs_Refresh_end_fclose_wait,
		enumStatusMainAnybus_Jobs_Refresh_end_all,


		enumStatusMainAnybus_Regen_FInit,
		enumStatusMainAnybus_Regen_FDelete_Wait,
		enumStatusMainAnybus_Regen_FOpen,
		enumStatusMainAnybus_Regen_FOpen_Wait,
		enumStatusMainAnybus_Regen_FWrite,
		enumStatusMainAnybus_Regen_FWrite_Wait,
		enumStatusMainAnybus_Regen_FClose,
		enumStatusMainAnybus_Regen_FClose_Wait,

		enumStatusMainAnybus_Refresh_FInit,
		enumStatusMainAnybus_Refresh_FOpen_Wait,
		enumStatusMainAnybus_Refresh_FRead,
		enumStatusMainAnybus_Refresh_FRead_Wait,
		enumStatusMainAnybus_Refresh_FClose,
		enumStatusMainAnybus_Refresh_FClose_Wait,

		enumStatusMainAnybus_RegenJoblist,
		enumStatusMainAnybus_RegenJoblist_end,



		enumStatusMainAnybus_LoopUpdateParams,
		enumStatusMainAnybus_LoopUpdateParams_Wait,

		enumStatusMainAnybus_SuspendAnybusOp,
		
	   enumStatusMainAnybus_NumOf
	}enumStatusMainAnybus;

typedef struct _TipoTimerHma{
	unsigned char ucPrevCntInterrupt,ucActCntInterrupt;
	unsigned long ulTickValue;
}TipoTimerHma;

// tipo di comandi che possono essere inviati dal pc al box
#define defPCRequest_productList 0x0001
#define defPCForce_productList 0x0002


// puntatore a funzione inizializzatore produttore dati per rigenerazione file
typedef unsigned char (*TipoFunzione_ucInitDataProducer)(void);
// puntatore a funzione produttore dati per rigenerazione file
typedef unsigned char (*TipoFunzione_ucDataProducer)(char *puc,unsigned char ucNumBytes2get);
// puntatore a funzione inizializzatore produttore dati per rinfresco file
typedef unsigned char (*TipoFunzione_ucInitDataConsumer)(void);
// puntatore a funzione produttore dati per rinfrescare file
typedef unsigned char (*TipoFunzione_ucDataConsumer)(char *puc,unsigned char ucNumBytes2put);

// struttura con tutte le info per impostare/eseguire sequenza di regen (riscrittura) file
typedef struct _TipoRegenSeq{
	char *pcName;
	TipoFunzione_ucInitDataProducer ucInitDataProducer;
	TipoFunzione_ucDataProducer ucDataProducer;
	enumStatusMainAnybus returnStatus;
}TipoRegenSeq;

// struttura con tutte le info per impostare/eseguire sequenza di refresh (rilettura) file
typedef struct _TipoRefreshSeq{
	char *pcName;
	TipoFunzione_ucInitDataConsumer ucInitDataConsumer;
	TipoFunzione_ucDataConsumer ucDataConsumer;
	enumStatusMainAnybus returnStatus;
}TipoRefreshSeq;

// funzione che sepcifica le condizioni sotto cui posso accettare controllo remoto
// attualmente:
// - NON accetto controllo remoto se:
//	* programma running
//	* sono in un menu in cui � disabilitato lo start lavorazione, ad es menu utilities
//  * se lista jobs non vuota!
unsigned char uiCanIAcceptRemoteControl(void){
	xdata unsigned int i;
	static unsigned int xdata uiHysteresisAcceptRemote=0;
	if (ucIsDisabledStartLavorazione()||(PrgRunning)){
		uiHysteresisAcceptRemote=0;
		return 0;
	}
	// verifico se esiste una joblist con lavori in lista
	// se esiste, non posso accettare controllo remoto
	i=0;
	while (i<nvram_struct.codelist.ucNumElem){
		if (nvram_struct.codelist.codeJobList[i].jobs.ucNumLavoriInLista){
			uiHysteresisAcceptRemote=0;
			return 0;
		}
		i++;
	}
	// quando la situazione � sbloccata, attendo ancora un certo numero di loops per dare modo
	// al micro di rigenerare i files se serve, es. dopo aver concluso programma, devo salvare gli archivi
	if (uiHysteresisAcceptRemote<100){
		uiHysteresisAcceptRemote++;
		return 0;
	}
	uiHysteresisAcceptRemote=100;
	// se finalmente arrivo qui, posso accettare jobs!!!
	return 1;
}//unsigned char uiCanIAcceptRemoteControl(void)


// numero di stringhe di log a disposizione del sistema az4181
#define defNumLogString_Az4181 3

// numero max di caratteri in una log string az4181: 128
#define defMaxCharLogString_Az4181 (128)

// numero max di caratteri in una log string anybus: 32 di cui 1 per� � il tappo
#define defMaxCharLogString_Anybus (32)

// numero di stringhe anybus necessarie per rappresentare una stringa di log az4181
#define defNumOfLogStringAnybusPerAz4181 (defMaxCharLogString_Az4181/defMaxCharLogString_Anybus)

// numero di stringhe di log anybus necessarie per rappresentare le stringhe di log az4181
#define defNumLogString_Anybus (defNumLogString_Az4181*defNumOfLogStringAnybusPerAz4181)

typedef struct _TipoStructHandleMainAnybus{
	enumStatusMainAnybus status;
	TipoTimerHma tick;
	unsigned int uiIdxParam;
	// array che accoglie gli indirizzi dei parametri modbus...
	unsigned int uiAnyBusApplParamsAddress[defMaxNumAnybusApplParams];
	unsigned int uiFileInstance;
	unsigned int uiFileInstance_codici;
	unsigned int uiFileInstance_jobs;
	unsigned int uiHeartBeat;
	unsigned int uiNumValidationCodeList;
	unsigned char ucFileBuffer[128]; 
	unsigned char ucNumByteFileBuffer;
	unsigned char *puc;
	unsigned char ucEsitoError;
	unsigned char LoadCodici_Fase;
	unsigned char ucRemoteCommand;
	unsigned char ucRemoteCommandReply,ucRemoteCommandAsking,ucRemoteCommandResponseYes;
	unsigned char semaphor;
	unsigned char resetdone;

	char logString[defMaxCharLogString_Az4181];
	char logString_hms[defNumLogString_Anybus][defMaxCharLogString_Anybus];

	char startString_0[defMaxCharLogString_Anybus];
	char startString_1[defMaxCharLogString_Anybus];
	char startString_2[defMaxCharLogString_Anybus];
	char startString_3[defMaxCharLogString_Anybus];
	char jobStatusString_0[defMaxCharLogString_Anybus];
	char jobStatusString_1[defMaxCharLogString_Anybus];
	char jobStatusString_2[defMaxCharLogString_Anybus];
	char jobStatusString_3[defMaxCharLogString_Anybus];
	char endString_0[defMaxCharLogString_Anybus];
	char endString_1[defMaxCharLogString_Anybus];
	char endString_2[defMaxCharLogString_Anybus];
	char endString_3[defMaxCharLogString_Anybus];


	unsigned char ucChanged_LogString[defMaxNumAnybusApplParams];
	unsigned char ucIdxlogString,ucIdxProductionLog;
	unsigned char anybusLogString_cIdChar;
	unsigned char ucDisable_ModuleNotResponding;
	unsigned int uiPrgRunning,uiRpmSpindle,uiRpmWheel1,uiRpmWheel2;
	char ucVelMMin[8];
	char ucAlarmString[21];
	char ucRemoteOk[8];


	unsigned char ucNumLoopAutoBaud,ucIdxReadPars;
	unsigned char ucAutobaud_NumMsgTxRxOk,ucAutobaud_NumMsgTxRxKo;
	unsigned char ucMacAddress[7];
	unsigned char ucMacAddress_String[22];
	// info per sequenza regen file
	TipoRegenSeq fileReGenSeq;
	// info per sequenza refresh file
	TipoRefreshSeq fileRefreshSeq;
}TipoStructHandleMainAnybus;


// struttura di gestione del modulo anybus
xdata TipoStructHandleMainAnybus hma;



// tabella dei parametri modbus da creare
code TipoStructAnybus_ApplicationParameters anybus_ApplParams[defMaxNumAnybusApplParams]={

// heartbeat--> indica che box � vivo
// passo tick + 1 che contiene parte bassa del tick...
	{sizeof(unsigned short int),1,1,enumApplicationParameter_dataType_UInt,0,0xFFFF,0x0000,"HeartBeat"      ,  "none",	&hma.uiAnyBusApplParamsAddress[0],1,((char*)&(hma.uiHeartBeat))},
	{sizeof(unsigned short int),1,1,enumApplicationParameter_dataType_UInt,0,0xFFFF,0x0000,"PrgRunning"      ,  "none",	&hma.uiAnyBusApplParamsAddress[1],1,((char*)&(hma.uiPrgRunning))},
	{sizeof(hma.ucVelMMin),1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"Speed"           ,  "m/min",	&hma.uiAnyBusApplParamsAddress[2],1,((char*)&(hma.ucVelMMin[0]))},
	{sizeof(unsigned short int),1,1,enumApplicationParameter_dataType_UInt,0,0xFFFF,0x0000,"Spindle"      ,  "rpm",	&hma.uiAnyBusApplParamsAddress[3],1,((char*)&(hma.uiRpmSpindle))},
	{sizeof(unsigned short int),1,1,enumApplicationParameter_dataType_UInt,0,0xFFFF,0x0000,"Wheel_1"      ,  "rpm",	&hma.uiAnyBusApplParamsAddress[4],1,((char*)&(hma.uiRpmWheel1))},
	{sizeof(unsigned short int),1,1,enumApplicationParameter_dataType_UInt,0,0xFFFF,0x0000,"Wheel_2"      ,  "rpm",	&hma.uiAnyBusApplParamsAddress[5],1,((char*)&(hma.uiRpmWheel2))},
	{20                ,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"Status"     ,  "string",	&hma.uiAnyBusApplParamsAddress[6],1,((char*)&(hma.ucAlarmString[0]))},
	{30                ,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"RemoteOk"    ,  "string",	&hma.uiAnyBusApplParamsAddress[7],1,((char*)&(hma.ucRemoteOk[0]))},

	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"StartString_0"      ,  "string",	&hma.uiAnyBusApplParamsAddress[8],1,((char*)hma.startString_0)},
	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"StartString_1"      ,  "string",	&hma.uiAnyBusApplParamsAddress[9],1,((char*)hma.startString_1)},
	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"StartString_2"      ,  "string",	&hma.uiAnyBusApplParamsAddress[10],1,((char*)hma.startString_2)},
	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"StartString_3"      ,  "string",	&hma.uiAnyBusApplParamsAddress[11],1,((char*)hma.startString_3)},

	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"JobString_0"      ,  "string",	&hma.uiAnyBusApplParamsAddress[12],1,((char*)hma.jobStatusString_0)},
	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"JobString_1"      ,  "string",	&hma.uiAnyBusApplParamsAddress[13],1,((char*)hma.jobStatusString_1)},
	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"JobString_2"      ,  "string",	&hma.uiAnyBusApplParamsAddress[14],1,((char*)hma.jobStatusString_2)},
	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"JobString_3"      ,  "string",	&hma.uiAnyBusApplParamsAddress[15],1,((char*)hma.jobStatusString_3)},

	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"EndString_0"      ,  "string",	&hma.uiAnyBusApplParamsAddress[16],1,((char*)hma.endString_0)},
	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"EndString_1"      ,  "string",	&hma.uiAnyBusApplParamsAddress[17],1,((char*)hma.endString_1)},
	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"EndString_2"      ,  "string",	&hma.uiAnyBusApplParamsAddress[18],1,((char*)hma.endString_2)},
	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"EndString_3"      ,  "string",	&hma.uiAnyBusApplParamsAddress[19],1,((char*)hma.endString_3)},

	{22,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"macAddress"      ,  "string",	&hma.uiAnyBusApplParamsAddress[20],1,((char*)hma.ucMacAddress_String)},


// log string az4181 #0
	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"LogString_0"       ,  "string",	&hma.uiAnyBusApplParamsAddress[21] ,1,((char*)hma.logString_hms[0] )},
	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"LogString_1"       ,  "string",	&hma.uiAnyBusApplParamsAddress[22] ,1,((char*)hma.logString_hms[1] )},
	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"LogString_2"       ,  "string",	&hma.uiAnyBusApplParamsAddress[23] ,1,((char*)hma.logString_hms[2] )},
	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"LogString_3"       ,  "string",	&hma.uiAnyBusApplParamsAddress[24] ,1,((char*)hma.logString_hms[3] )},
// log string az4181 #1
	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"LogString_4"       ,  "string",	&hma.uiAnyBusApplParamsAddress[25] ,1,((char*)hma.logString_hms[4] )},
	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"LogString_5"       ,  "string",	&hma.uiAnyBusApplParamsAddress[26] ,1,((char*)hma.logString_hms[5] )},
	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"LogString_6"       ,  "string",	&hma.uiAnyBusApplParamsAddress[27] ,1,((char*)hma.logString_hms[6] )},
	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"LogString_7"       ,  "string",	&hma.uiAnyBusApplParamsAddress[28] ,1,((char*)hma.logString_hms[7] )},
// log string az4181 #2
	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"LogString_8"       ,  "string",	&hma.uiAnyBusApplParamsAddress[29] ,1,((char*)hma.logString_hms[8] )},
	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"LogString_9"       ,  "string",	&hma.uiAnyBusApplParamsAddress[30],1,((char*)hma.logString_hms[9] )},
	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"LogString_10"      ,  "string",	&hma.uiAnyBusApplParamsAddress[31],1,((char*)hma.logString_hms[10])},
	{32,1,1,enumApplicationParameter_dataType_String,0,0xFFFF,0x0000,"LogString_11"      ,  "string",	&hma.uiAnyBusApplParamsAddress[32],1,((char*)hma.logString_hms[11])},



};

// funzione che comanda l'impostazione e l'avvio della procedura di rigenerazione (riscrittura) di un file
static unsigned char ucStartFileRegen(char *pcName,
							   TipoFunzione_ucInitDataProducer ucInitDataProducer, 
							   TipoFunzione_ucDataProducer ucDataProducer,
							   enumStatusMainAnybus returnStatus
							   ){
	hma.fileReGenSeq.pcName=pcName;
	hma.fileReGenSeq.ucInitDataProducer=ucInitDataProducer;
	hma.fileReGenSeq.ucDataProducer=ucDataProducer;
	hma.fileReGenSeq.returnStatus=returnStatus;
	hma.status=enumStatusMainAnybus_Regen_FInit;
	return 1;
}// unsigned char ucStartFileRegen(char *pc,...


// funzione che comanda l'impostazione e l'avvio della procedura di refresh (rilettura) di un file
static unsigned char ucStartFileRefresh(char *pcName,
							   TipoFunzione_ucInitDataConsumer ucInitDataConsumer, 
							   TipoFunzione_ucDataConsumer ucDataConsumer,
							   enumStatusMainAnybus returnStatus
							   ){
	hma.fileRefreshSeq.pcName=pcName;
	hma.fileRefreshSeq.ucInitDataConsumer=ucInitDataConsumer;
	hma.fileRefreshSeq.ucDataConsumer=ucDataConsumer;
	hma.fileRefreshSeq.returnStatus=returnStatus;
	hma.status=enumStatusMainAnybus_Refresh_FInit;
	return 1;
}// unsigned char ucStartFileRefresh(char *pc,...


// tabella dei parametri da leggere all'avvio del programma...
code TipoHma_ReadInitialParsTable hma_pars[]={
	{enumAnybusGP_MAC_Address,3,(unsigned int *)&(hma.ucMacAddress[0])}, // 3 registri=6bytes
	{0,0,NULL}
};

// restituisce mac address anybus
unsigned char *pcGetAnybusMacAddress(void){
	return &(hma.ucMacAddress[0]);
}

// void vUpdateAnybusLogString(void)
// aggiorna le stringhe di log anybus
// la stringa da inserire nel log deve essere scritta in hma.logString
static void vUpdateAnybusLogString(void){
	xdata unsigned char i,j,ucEndOfSrcString;
	// controllo dell'indice delle stringhe
	if (hma.ucIdxlogString>=sizeof(hma.logString_hms)/sizeof(hma.logString_hms[0])){
		hma.ucIdxlogString=0;
	}
	// reset delle stringhe destinate ad accogliere la nuova stringa di log...
	memset(&hma.logString_hms[hma.ucIdxlogString][0],0,defNumOfLogStringAnybusPerAz4181*sizeof(hma.logString_hms[0]));
	// inizializzazione id char che identifica sequenza di aggiornamento:
	// � un carattere che viene posto all'inizio della serie di n stringhe che compongono
	// la stringa di log
	// viene incrementato ad ogni scrittura di una sequenza di stringhe e serve per evitare
	// di prelevare stringhe di log in corso di aggiornamento
	hma.anybusLogString_cIdChar++;
	if (hma.anybusLogString_cIdChar<'A'){
		hma.anybusLogString_cIdChar='A';
	}
	else if (hma.anybusLogString_cIdChar>'Z'){
		hma.anybusLogString_cIdChar='Z';
	}
	// indico che src string non � finita
	ucEndOfSrcString=0;
	// loop di copia delle stringhe...
	i=0;
	while (i<defNumOfLogStringAnybusPerAz4181){
		if (hma.ucIdxlogString>=sizeof(hma.logString_hms)/sizeof(hma.logString_hms[0])){
			hma.ucIdxlogString=0;
		}
		// occhio che se src stringa al giro precedente era finita non devo andare a pescare caratteri dopo il tappo!!!!
		if (!ucEndOfSrcString){
			strncpy(&hma.logString_hms[hma.ucIdxlogString][0],
					hma.logString+i*(defMaxCharLogString_Anybus-2),
					sizeof(hma.logString_hms[0])-2
					);
		}
		j=strlen(&hma.logString_hms[hma.ucIdxlogString][0]);
		if (j<sizeof(hma.logString_hms[0])-2){
			// sbianco la parte rimanente di stringa
			memset(&hma.logString_hms[hma.ucIdxlogString][j],' ',sizeof(hma.logString_hms[0])-2-j);
			ucEndOfSrcString=1;
		}
		// alla fine della stringa metto sempre l'id char...
		hma.logString_hms[hma.ucIdxlogString][sizeof(hma.logString_hms[0])-2]=hma.anybusLogString_cIdChar;
		// metto per sicurezza il tappo...
		hma.logString_hms[hma.ucIdxlogString][sizeof(hma.logString_hms[0])-1]=0;
		// indico che stringa � cambiata...
		hma.ucChanged_LogString[hma.ucIdxlogString]=1;
		hma.ucIdxlogString++;
		i++;
	}//while (i=0;i<defNumOfLogStringAnybusPerAz4181;i++)
}//void vUpdateAnybusLogString(void)


// inizializzazione global Handle Module Anybus
void vHMA_init(void){
	extern void vInitStructSuspendAnybusOp(void);
	// inizializzazione anybus tx/rx
	vAnybusTxRx_Init();
	vInitStructSuspendAnybusOp();
	// reset struttura hma
	memset(&hma,0,sizeof(hma));
	// inizailizzo struttura log string changed in modo che le stringhe vengano rinfrescate all'avvio...
	memset(hma.ucChanged_LogString,1,sizeof(hma.ucChanged_LogString));

	// leggo date and time per inserirli nella stringa di log...
	vReadTimeBytesRTC_DS3231();
	memset(hma.logString,0,sizeof(hma.logString));
	sprintf(hma.logString,"CSM COIL MACHINE POWER UP AT %i-%02hi-%02hi %02hi:%02hi:%02hi\r\n",
					rtc.ucAnno+2000,
					rtc.ucMese,
					rtc.ucGiorno,
					rtc.ucOre,
					rtc.ucMinuti,
					rtc.ucSecondi
					);
	// inserisco la stringa di log anybus
	vUpdateAnybusLogString();
}


// numero amx caratteri stringa descrizione product code...
#define defMaxCharStringProductCode 512

typedef struct _TipoStructGetStringNextProductCode{
	int iNumCharInBuffer;
	char *pCurCharInBuffer;
	char s[defMaxCharStringProductCode+1];
	int iNumPrg,iNextNumPrg;
	int idx0,idx1,idx2;
	PROGRAMMA params;

}TipoStructGetStringNextProductCode;

xdata TipoStructGetStringNextProductCode gsnpc;

// funzione che salta tutti i blank puntati da pc, fino al primo non blank od eventualmente al tappo
xdata char *pcSkipBlank(xdata char *pc){
	while((*pc)&&isspace(*pc))
		pc++;
	return pc;
}

// funzione che trova il prossimo blank o tappo
xdata char *pcFindBlank(xdata char *pc){
	while((*pc)&&!isspace(*pc))
		pc++;
	return pc;
}


// funzione che salta tutti i caratteri fino a trovare il carattere specificato, salta anche quello
// e restituisce il ptr a carattere; se trova tappo si ferma
xdata char *pcFindChar(xdata char *pc, char c){
	while((*pc)&&(*pc!=c))
		pc++;
	// se trovato carattere indicato, passo al successivo
	if (*pc&&(*pc==c))
		pc++;
	return pc;
}

// funzione chiamata per inizializzare trasferimento di una joblist da pc verso box
unsigned char ucInitDataConsumer_productList(void){
	// reset struttura nvram_struct.codelist
	memset(&nvram_struct.codelist,0,sizeof(nvram_struct.codelist));
	// validazione struttura nvram_struct.codelist
	vValidateCodeList();
	// reset anche struttura gsnpc per gestione buffers che arrivano da pc
	memset(&gsnpc,0,sizeof(gsnpc));
	return 1;
}


// funzione chiamata per scaricare dati joblist da pc verso box
// problema: non arriva una riga per volta... ma n bytes alla volta...
// soluzione: la stringa gsnpc.s contiene 512 bytes, + che sufficienti a memorizzare una linea intera...
unsigned char ucDataConsumer_productList(char *puc,unsigned char ucNumBytes2put){
	void vVerifyInsertCodiceProdotto(xdata char *s);
	unsigned int uiMaxBytes2append;
	xdata char *pc;
	xdata char *pc2;
	xdata TipoLavoro *p;
	// appendo al buffer corrente il buffer attuale, stando attento a non sforare
	uiMaxBytes2append=sizeof(gsnpc.s)-1;// numero di bytes che possono essere appesi= dimensione stringa-1
	uiMaxBytes2append-=strlen(gsnpc.s);// tolgo numero di bytes gi� occupati
	// attenzione! se ne devo appendere pi� di quanti ce ne stanno, cosa faccio???
	// questo non dovrebbe mai capitare, comunque per semplicit� reset buffer corrente
	if (ucNumBytes2put>uiMaxBytes2append){
		goto errorDataConsumer;
	}
	// eseguo l'append
	strncat(gsnpc.s,puc,ucNumBytes2put);
	while(1){
		// verifico se esiste terminatore di linea, nel qual caso la esamino
		// come terminatore di linea considero sia \r\n che il solo \n
		if (!strstr(gsnpc.s,"\r\n")){
			if (!strchr(gsnpc.s,'\n')){
				return 0;
			}
		}
		// salto tutti i blanks
		pc=pcSkipBlank(gsnpc.s);
		// verifico su che tipo di riga mi trovo
		if (strncmp(pc,"Prow=",5)==0){
			gsnpc.idx2=0;
		}
		else{
			gsnpc.idx2=1;
		}
		switch(gsnpc.idx2){
			// attesa clausola "Prow=" etc
			case 0:

				pc=pcSkipBlank(pc);
				// se non trovo prow, errore
				if (strncmp(pc,"Prow=",5)!=0){
					goto errorDataConsumer;
				}
				pc=pcFindChar(pc,'=');
				pc=pcSkipBlank(pc);
				gsnpc.idx0=atoi(pc);
				// validazione idx0= indice product list
				if (gsnpc.idx0>=sizeof(nvram_struct.codelist.codeJobList)/sizeof(nvram_struct.codelist.codeJobList[0])){
					goto errorDataConsumer;
				}
				pc=pcFindChar(pc,',');

				pc=pcSkipBlank(pc);
				// se non trovo code, errore
				if (strncmp(pc,"code=",5)!=0){
					goto errorDataConsumer;
				}
				pc=pcFindChar(pc,'=');
				pc=pcSkipBlank(pc);
				uiMaxBytes2append=sizeof(nvram_struct.codelist.codeJobList[0].ucCodice)-1;
				pc2=pcFindChar(pc,',');
				if (pc2-1-pc<uiMaxBytes2append)
					uiMaxBytes2append=pc2-1-pc;
				nvram_struct.codelist.codeJobList[gsnpc.idx0].ucCodice[0]=0;
				strncpy((char*)nvram_struct.codelist.codeJobList[gsnpc.idx0].ucCodice,pc,uiMaxBytes2append);
				// verifico se il codice esiste: se non esiste, bisogna inserirlo in lista!
				vVerifyInsertCodiceProdotto((char*)nvram_struct.codelist.codeJobList[gsnpc.idx0].ucCodice);

				pc=pc2;

				pc=pcSkipBlank(pc);
				// se non trovo workMode, errore
				if (strncmp(pc,"workMode=",9)!=0){
					goto errorDataConsumer;
				}
				pc=pcFindChar(pc,'=');
				pc=pcSkipBlank(pc);
				if (strncmp(pc,"L+R",3)==0)
					nvram_struct.codelist.codeJobList[gsnpc.idx0].jobs.ucWorkingMode_0L_1LR=1;
				else
					nvram_struct.codelist.codeJobList[gsnpc.idx0].jobs.ucWorkingMode_0L_1LR=0;
				pc=pcFindChar(pc,',');
				pc=pcSkipBlank(pc);

				// se non trovo manual, errore
				if (strncmp(pc,"manual=",7)!=0){
					goto errorDataConsumer;
				}
				pc=pcFindChar(pc,'=');
				pc=pcSkipBlank(pc);
				if (strncmp(pc,"Y",1)==0)
					nvram_struct.codelist.codeJobList[gsnpc.idx0].ucManual=1;
				else
					nvram_struct.codelist.codeJobList[gsnpc.idx0].ucManual=0; 
				// salto il carattere "Y" o "N"
				pc=pcFindBlank(pc);
				// salto tutti i blank che seguono
				pc=pcSkipBlank(pc);
				nvram_struct.codelist.ucNumElem=nvram_struct.codelist.ucNumElem+1;				// numero di elementi in lista
				nvram_struct.codelist.ucIdxActiveElem=0;		    // indice dell'elemento attivo
				break;
			// attesa clausola "Job=" etc
			case 1:
				pc=pcSkipBlank(pc);
				// se non trovo Job, errore
				if (strncmp(pc,"Job=",4)!=0){
					goto errorDataConsumer;
				}
				pc=pcFindChar(pc,'=');
				pc=pcSkipBlank(pc);
				gsnpc.idx1=atoi(pc);
				// validazione idx0= indice product list
				if (gsnpc.idx1>=sizeof(nvram_struct.codelist.codeJobList[0].jobs.lista)/sizeof(nvram_struct.codelist.codeJobList[0].jobs.lista[0])){
					goto errorDataConsumer;
				}
				pc=pcFindChar(pc,',');


				p=&(nvram_struct.codelist.codeJobList[gsnpc.idx0].jobs.lista[gsnpc.idx1]);

				pc=pcSkipBlank(pc);
				// se non trovo n, errore
				if (strncmp(pc,"n=",2)!=0){
					goto errorDataConsumer;
				}
				pc=pcFindChar(pc,'=');
				pc=pcSkipBlank(pc);
				p->uiNumeroPezzi=atoi(pc);
				pc=pcFindChar(pc,',');


				pc=pcSkipBlank(pc);
				// se non trovo ohm, errore
				if (strncmp(pc,"ohm=",4)!=0){
					goto errorDataConsumer;
				}
				pc=pcFindChar(pc,'=');
				pc=pcSkipBlank(pc);
				p->fOhm=atof(pc);
				pc=pcFindChar(pc,',');

				pc=pcSkipBlank(pc);
				// se non trovo descr, errore
				if (strncmp(pc,"descr=",6)!=0){
					goto errorDataConsumer;
				}
				pc=pcFindChar(pc,'=');
				uiMaxBytes2append=sizeof(p->ucDescrizione)-1;
				pc2=pcFindChar(pc,',');
				if (pc2-1-pc<uiMaxBytes2append)
					uiMaxBytes2append=pc2-1-pc;
				strncpy((char*)p->ucDescrizione,pc,uiMaxBytes2append);
				pc=pc2;


				pc=pcSkipBlank(pc);
				// se non trovo ndone, errore
				if (strncmp(pc,"ndone=",6)!=0){
					goto errorDataConsumer;
				}
				pc=pcFindChar(pc,'=');
				pc=pcSkipBlank(pc);
				p->npezzifatti=atoi(pc);
				pc=pcFindChar(pc,',');
				pc=pcSkipBlank(pc);

				// se non trovo added, errore
				if (strncmp(pc,"added=",6)!=0){
					goto errorDataConsumer;
				}
				pc=pcFindChar(pc,'=');
				pc=pcSkipBlank(pc);
				p->magg=atoi(pc);
				// trovo il blank che segue
				pc=pcFindBlank(pc);
				// salto tutti i blank che seguono
				pc=pcSkipBlank(pc);
				gsnpc.idx1++;
	// salvo la nvram_struct.codelist...
				nvram_struct.codelist.codeJobList[gsnpc.idx0].jobs.ucNumLavoriInLista=gsnpc.idx1;

				break;

		}// switch
		vValidateCodeList();
		// travaso...
		strcpy(gsnpc.s,pc);
		// sbianco la parte che segue la fine della stringa
		memset(gsnpc.s+strlen(gsnpc.s),0,sizeof(gsnpc.s)-strlen(gsnpc.s));

	}// while
	return 1;

errorDataConsumer:
	memset(gsnpc.s,0,sizeof(gsnpc.s));
	return 0;
}

unsigned char ucInitDataProducer_productList(void){
	memset(&gsnpc,0,sizeof(gsnpc));
	return 1;
}

unsigned char ucDataProducer_productList(char *puc,unsigned char ucNumBytes2get){
	xdata char *pc, ucContinue;
	TipoLavoro *p;
	if (!gsnpc.iNumCharInBuffer){
		ucContinue=1;
		pc=gsnpc.s;
		while(ucContinue){
			if (gsnpc.idx0>=nvram_struct.codelist.ucNumElem)
				return 0;
			switch(gsnpc.idx2){
				case 0:
					pc+=sprintf(pc,"Prow=%i,",gsnpc.idx0);
					pc+=sprintf(pc,"code=%s,",nvram_struct.codelist.codeJobList[gsnpc.idx0].ucCodice);
					pc+=sprintf(pc,"workMode=%s,",nvram_struct.codelist.codeJobList[gsnpc.idx0].jobs.ucWorkingMode_0L_1LR?"L+R":"L");
					pc+=sprintf(pc,"manual=%c\r\n",nvram_struct.codelist.codeJobList[gsnpc.idx0].ucManual?'Y':'N');
					gsnpc.idx2=1;
					gsnpc.idx1=0;
					ucContinue=0;
					break;
				case 1:
				default:
					if (gsnpc.idx1>=nvram_struct.codelist.codeJobList[gsnpc.idx0].jobs.ucNumLavoriInLista){
						gsnpc.idx0++;
						gsnpc.idx2=0;
						gsnpc.idx1=0;
						ucContinue=1;
						break;
					}
					ucContinue=0;
					pc=gsnpc.s;
					p=&(nvram_struct.codelist.codeJobList[gsnpc.idx0].jobs.lista[gsnpc.idx1]);
					pc+=sprintf(pc,"Job=%i,",gsnpc.idx1);
					pc+=sprintf(pc,"n=%u,",p->uiNumeroPezzi);
					pc+=sprintf(pc,"ohm=%f,",p->fOhm);
					pc+=sprintf(pc,"descr=%s,",p->ucDescrizione);
					pc+=sprintf(pc,"ndone=%u,",p->npezzifatti);
					pc+=sprintf(pc,"added=%u\r\n",p->magg);
					gsnpc.idx1++;
					break;
			}
		}//while(ucContinue){
		gsnpc.iNumCharInBuffer=pc-gsnpc.s;
		gsnpc.pCurCharInBuffer=gsnpc.s;
	}//if (!gsnpc.iNumCharInBuffer){
	// indico quanti caratteri posso trasferire
	// se me ne richiedono pi� di quelli che ho, clippo
	if (ucNumBytes2get>gsnpc.iNumCharInBuffer)
		ucNumBytes2get=gsnpc.iNumCharInBuffer;
	// decremento il numero di chars a disposizione
	gsnpc.iNumCharInBuffer-=ucNumBytes2get;
	// salvo ptr ai dati da restituire
	strncpy(puc,gsnpc.pCurCharInBuffer,ucNumBytes2get);
	// metto il tappo...
	puc[ucNumBytes2get]=0;
	// aggiorno ptr ai prossimi dati
	gsnpc.pCurCharInBuffer+=ucNumBytes2get;
	return ucNumBytes2get;
}

static TipoUnionParamWalkReg u_anybus;
unsigned char ucInitDataProducer_productArchive(void){
	memset(&gsnpc,0,sizeof(gsnpc));
    u_anybus.pBody=(char *)(&gsnpc.params);
    // walk init
    walk_reg_file_list(   ui_handle_product_codes
                         ,enum_walk_reg_file_list_init
                         ,u_anybus
                      );
	return 1;
}

// restituisce n chars dall'archivio prodotti
unsigned char ucDataProducer_productArchive(char *puc,unsigned char ucNumBytes2get){
	xdata char *pc;
    enum_walk_reg_file_list_retcode walk_reg_file_list_retcode;
	if (!gsnpc.iNumCharInBuffer){
 // walk next
        walk_reg_file_list_retcode=walk_reg_file_list(   ui_handle_product_codes
                                                        ,enum_walk_reg_file_list_next
                                                        ,u_anybus
                                                    );     
        if (walk_reg_file_list_retcode==enum_walk_reg_file_list_retcode_ok){
            walk_reg_file_list_retcode=walk_reg_file_list(  ui_handle_product_codes
                                                            ,enum_walk_reg_file_list_get_body
                                                            ,u_anybus
                                                          );
            if (walk_reg_file_list_retcode!=enum_walk_reg_file_list_retcode_ok){
                return 0;
            }
        }
		gsnpc.iNextNumPrg=ul_reg_file_cur_id(ui_handle_product_codes, enum_reg_file_id_required_last_block_walked);;
		
		pc=gsnpc.s;
		pc+=sprintf(pc,"%s,",gsnpc.params.codicePrg);
		pc+=sprintf(pc,"%u,",gsnpc.params.nextPrg);
		pc+=sprintf(pc,"%u,",gsnpc.params.prevPrg);
		pc+=sprintf(pc,"%f,",gsnpc.params.vel_produzione);
		pc+=sprintf(pc,"%u,",gsnpc.params.rpm_mandrino);

		pc+=sprintf(pc,"%f,",gsnpc.params.assorb[0]);
		pc+=sprintf(pc,"%f,",gsnpc.params.assorb[1]);

		pc+=sprintf(pc,"%f,",gsnpc.params.pos_pretaglio);
		pc+=sprintf(pc,"%f,",gsnpc.params.diametro_filo);
		pc+=sprintf(pc,"%f,",gsnpc.params.diametro_mandrino);
		pc+=sprintf(pc,"%c,",gsnpc.params.num_fili+'0');
		pc+=sprintf(pc,"%f,",gsnpc.params.coeff_corr);
		pc+=sprintf(pc,"%u,",gsnpc.params.vruote_prg[0]);
		pc+=sprintf(pc,"%u,",gsnpc.params.vruote_prg[1]);

		pc+=sprintf(pc,"%f,",gsnpc.params.vel_frizioni[0]);
		pc+=sprintf(pc,"%f,",gsnpc.params.vel_frizioni[1]);

		pc+=sprintf(pc,"%c,",gsnpc.params.fornitore+'0');
		pc+=sprintf(pc,"%hu,",gsnpc.params.usValidProgram_A536);
		pc+=sprintf(pc,"%c,",gsnpc.params.empty+'0');
		pc+=sprintf(pc,"%f\r\n",gsnpc.params.vel_prod_vera);

		gsnpc.iNumCharInBuffer=pc-gsnpc.s;
		gsnpc.pCurCharInBuffer=gsnpc.s;
	}
	// indico quanti caratteri posso trasferire
	// se me ne richiedono pi� di quelli che ho, clippo
	if (ucNumBytes2get>gsnpc.iNumCharInBuffer)
		ucNumBytes2get=gsnpc.iNumCharInBuffer;
	// decremento il numero di chars a disposizione
	gsnpc.iNumCharInBuffer-=ucNumBytes2get;
	// salvo ptr ai dati da restituire
	strncpy(puc,gsnpc.pCurCharInBuffer,ucNumBytes2get);
	// metto il tappo...
	puc[ucNumBytes2get]=0;
	// aggiorno ptr ai prossimi dati
	gsnpc.pCurCharInBuffer+=ucNumBytes2get;
	return ucNumBytes2get;
}

//
// **********************************************************
// **********************************************************
// **********************************************************
// **********************************************************
//
// NOMI DEI FILE NEL WEB SERVER ANYBUS
//
// **********************************************************
// **********************************************************
// **********************************************************
// **********************************************************

//
// nome del file che elenca tutti i codici prodotto
//
#define defAZ4181_product_file_name "\\RAM\\CSM_product_codes.txt"
//
// nome del file che elenca tutti i codici prodotto scritti da pc perch� il box li importi
//
#define defAZ4181_product_file_name_pc "\\RAM\\CSM_product_codes_pc.txt"
//
// nome del file che contiene il risultato dell'importazione dei codici prodotto e parametri macchina
//
#define defAZ4181_product_file_name_result "\\RAM\\CSM_product_codes_result.txt"


//
// nome del file che elenca la product list=job list
//
#define defAZ4181_joblist_file_name "\\RAM\\CSM_job_list.txt"
//
// nome del file che elenca la product list=job list scritti da pc perch� il box li importi
//
#define defAZ4181_joblist_file_name_pc "\\RAM\\CSM_job_list_pc.txt"
//
// nome del file che contiene il risultato dell'importazione della joblist
//
#define defAZ4181_joblist_file_name_result "\\RAM\\CSM_job_list_result.txt"


void vUpdateAnybusStartString(void){
	memset(hma.startString_0,0,sizeof(hma.startString_0));
	memset(hma.startString_1,0,sizeof(hma.startString_1));
	memset(hma.startString_2,0,sizeof(hma.startString_2));
	memset(hma.startString_3,0,sizeof(hma.startString_3));

	sprintf(hma.startString_0,"%-30.30s",hma.logString);
	sprintf(hma.startString_1,"%-30.30s",hma.logString+30);
	sprintf(hma.startString_2,"%-30.30s",hma.logString+60);
	sprintf(hma.startString_3,"%-30.30s",hma.logString+90);
}

void vUpdateAnybusProductionString(void){
	memset(hma.jobStatusString_0,0,sizeof(hma.jobStatusString_0));
	memset(hma.jobStatusString_1,0,sizeof(hma.jobStatusString_1));
	memset(hma.jobStatusString_2,0,sizeof(hma.jobStatusString_2));
	memset(hma.jobStatusString_3,0,sizeof(hma.jobStatusString_3));
	sprintf(hma.jobStatusString_0,"%-30.30s",hma.logString);
	sprintf(hma.jobStatusString_1,"%-30.30s",hma.logString+30);
	sprintf(hma.jobStatusString_2,"%-30.30s",hma.logString+60);
	sprintf(hma.jobStatusString_3,"%-30.30s",hma.logString+90);
}

void vUpdateAnybusEndProductionString(void){
	memset(hma.endString_0,0,sizeof(hma.endString_0));
	memset(hma.endString_1,0,sizeof(hma.endString_1));
	memset(hma.endString_2,0,sizeof(hma.endString_2));
	memset(hma.endString_3,0,sizeof(hma.endString_3));
	sprintf(hma.endString_0,"%-30.30s",hma.logString);
	sprintf(hma.endString_1,"%-30.30s",hma.logString+30);
	sprintf(hma.endString_2,"%-30.30s",hma.logString+60);
	sprintf(hma.endString_3,"%-30.30s",hma.logString+90);
}

void vAnybusSprintProductionString(void){
	memset(hma.logString,0,sizeof(hma.logString));
	sprintf(hma.logString,
					"JOB %-20.20s,code=%-20.20s,n=%hu,ndone=%hu,ohm=%6.3f,ohm/m=%6.3f,added=%hu",
					spiralatrice.pCommRun->commessa,
					hprg.theRunning.codicePrg,
					spiralatrice.pCommRun->n,
					actNumPezzi,
					spiralatrice.pCommRun->res,
					spiralatrice.pCommRun->resspec_spir,
					spiralatrice.pCommRun->magg
				);
}


// inserimento nel log anybus delle righe di log produzione
void vLog_production_log(void){
	if (production_log.ucIdx==hma.ucIdxProductionLog){
		return;
	}
	if (--hma.ucIdxProductionLog>=sizeof(production_log.datas)/sizeof(production_log.datas[0]))
		hma.ucIdxProductionLog=sizeof(production_log.datas)/sizeof(production_log.datas[0])-1;
	strcpy(hma.logString,"PROD:");
	vPrepareDesc_ProductionLog(hma.logString+5,&production_log.datas[hma.ucIdxProductionLog],0,0xFF);
	// inserisco la stringa di log anybus
	vUpdateAnybusLogString();
	// solo se taglio, aggiorno event log
	if (production_log.datas[hma.ucIdxProductionLog].event_type==let_cut){
		// INSERISCO INFO SU PRODUCTION
		vAnybusSprintProductionString();
		vUpdateAnybusProductionString();
	}
}//void vLogProductionlog(void)


// inserimento nel log anybus della riga che indica inizio log produzione
void vLogStartProduzione(void){
	// pericoloso usare la sprintf!!!!!!
	// pero' in libreria non c'� la snprintf

	// leggo date and time per inserirli nella stringa di log...
	vReadTimeBytesRTC_DS3231();
	// INSERISCO DATA/ORA DI PARTENZA
	memset(hma.logString,0,sizeof(hma.logString));
	sprintf(hma.logString,"PRODUCTION STARTS AT %i-%02hi-%02hi %02hi:%02hi:%02hi",
					rtc.ucAnno+2000,
					rtc.ucMese,
					rtc.ucGiorno,
					rtc.ucOre,
					rtc.ucMinuti,
					rtc.ucSecondi
					);
	vUpdateAnybusStartString();

	// inserisco la stringa di log anybus
	vUpdateAnybusLogString();

	// INSERISCO INFO SU PRODUCTION
	vAnybusSprintProductionString();
	// inserisco la stringa di log anybus
	vUpdateAnybusLogString();
	vUpdateAnybusProductionString();
	// sbianco end strings...
	memset(hma.endString_0,0,sizeof(hma.endString_0));
	memset(hma.endString_1,0,sizeof(hma.endString_1));
	memset(hma.endString_2,0,sizeof(hma.endString_2));
	memset(hma.endString_3,0,sizeof(hma.endString_3));

}//void vLogStartProduzione(void)


// inserimento nel log anybus della riga che indica inizio stop produzione
void vLogStopProduzione(void){
	// pericoloso usare la sprintf!!!!!!
	// pero' in libreria non c'� la snprintf

	// INSERISCO INFO SU PRODUCTION
	vAnybusSprintProductionString();
	// inserisco la stringa di log anybus
	vUpdateAnybusLogString();
	vUpdateAnybusProductionString();

	// leggo date and time per inserirli nella stringa di log...
	vReadTimeBytesRTC_DS3231();
	memset(hma.logString,0,sizeof(hma.logString));
	// INSERISCO DATA/ORA DI STOP PRODUZIONE
	sprintf(hma.logString,"PRODUCTION %s AT %i-%02hi-%02hi %02hi:%02hi:%02hi",
		ucDigitalInput_PositiveEdge(enumInputEdge_stop)?"STOPS":"ALARM",
					rtc.ucAnno+2000,
					rtc.ucMese,
					rtc.ucGiorno,
					rtc.ucOre,
					rtc.ucMinuti,
					rtc.ucSecondi
					);
	// inserisco la stringa di log anybus
	vUpdateAnybusLogString();
	vUpdateAnybusEndProductionString();

}//void vLogStartProduzione(void)


// inserimento nel log anybus della riga che indica end produzione
void vLogEndProduzione(void){
	// pericoloso usare la sprintf!!!!!!
	// pero' in libreria non c'� la snprintf

	// INSERISCO INFO SU PRODUCTION
	vAnybusSprintProductionString();
	// inserisco la stringa di log anybus
	vUpdateAnybusLogString();
	vUpdateAnybusProductionString();

	// leggo date and time per inserirli nella stringa di log...
	vReadTimeBytesRTC_DS3231();
	// INSERISCO DATA/ORA DI STOP PRODUZIONE
	memset(hma.logString,0,sizeof(hma.logString));
	sprintf(hma.logString,"PRODUCTION ENDS AT %i-%02hi-%02hi %02hi:%02hi:%02hi",
					rtc.ucAnno+2000,
					rtc.ucMese,
					rtc.ucGiorno,
					rtc.ucOre,
					rtc.ucMinuti,
					rtc.ucSecondi
					);
	// inserisco la stringa di log anybus
	vUpdateAnybusLogString();
	vUpdateAnybusEndProductionString();


}//void vLogEndProduzione(void)


// inserimento nel log anybus della riga che indica allarme
void vLogAlarm(void){
	xdata unsigned char i;
	// cerco il codice di allarme
	i=0;
	while (i<16){
		if (alarms&(1L<<i)){
			break;
		}
		i++;
	}
	if (i>=16)
		return;

	// INSERISCO INFO SU PRODUCTION
	vAnybusSprintProductionString();
	// inserisco la stringa di log anybus
	vUpdateAnybusLogString();
	vUpdateAnybusProductionString();

	// leggo date and time per inserirli nella stringa di log...
	vReadTimeBytesRTC_DS3231();
	// INSERISCO DESCRIZIONE ALLARME CON DATA/ORA DI INIZIO ALLARME
	memset(hma.logString,0,sizeof(hma.logString));
	sprintf(hma.logString,"ALARM %s AT %i-%hi-%hi %hi:%hi:%hi",
					pucStringLang(enumStr20_Allarmi+i),
					rtc.ucAnno+2000,
					rtc.ucMese,
					rtc.ucGiorno,
					rtc.ucOre,
					rtc.ucMinuti,
					rtc.ucSecondi
					);
	// inserisco la stringa di log anybus
	vUpdateAnybusLogString();
	vUpdateAnybusEndProductionString();
}//void vLogAlarm(void)


// indica se comando remoto attivo
unsigned char ucRemoteCommandRunning(void){
	return hma.ucRemoteCommand;
}

// indica se richiesta comando remoto attiva
unsigned char ucRemoteCommandAsking(void){
	return hma.ucRemoteCommandAsking;
}

void vRemoteCommandsFlagsInit(void){
	hma.ucRemoteCommandReply=0;
	hma.ucRemoteCommandResponseYes=0;
	hma.ucRemoteCommand=0;
	hma.ucRemoteCommandAsking=0;
	hma.semaphor=0;
}

// indica richiesta comando remoto accettata/non accettata
void vRemoteCommandAskingSet(unsigned char ucSetAsking){
	vForceLcdRefresh();
	hma.ucRemoteCommandReply=0;
	hma.ucRemoteCommandResponseYes=0;
	hma.ucRemoteCommand=0;
	if (ucSetAsking){
		hma.ucRemoteCommandAsking=1;
	}
	else{
		hma.ucRemoteCommandAsking=0;
	}
}


// indica richiesta comando remoto accettata/non accettata
void vRemoteCommandAskingReply(unsigned char ucAccepted){
	vForceLcdRefresh();
	hma.ucRemoteCommandReply=1;
	if (ucAccepted){
		hma.ucRemoteCommandAsking=0;
		hma.ucRemoteCommand=1;
		hma.ucRemoteCommandResponseYes=1;
	}
	else{
		hma.ucRemoteCommandAsking=0;
		hma.ucRemoteCommand=0;
		hma.ucRemoteCommandResponseYes=0;
	}
}


typedef struct _TipoStructSignalSuspendAnybus{
	unsigned char ucRequestSuspend;
	unsigned char ucRequestSuspendAck;
}TipoStructSignalSuspendAnybus;
xdata TipoStructSignalSuspendAnybus signalSuspendAnybus;

void vInitStructSuspendAnybusOp(void){
	memset(&signalSuspendAnybus,0,sizeof(signalSuspendAnybus));
}

unsigned int uiHasBeenAckRequestSuspendAnybusOp(void){
    #ifdef defDisable_Anybus_Module
        hma.ucDisable_ModuleNotResponding=1;
    #endif
	if (hma.ucDisable_ModuleNotResponding){
		return 1;
	}
	if (signalSuspendAnybus.ucRequestSuspendAck){
		return 1;
	}
	return 0;
}//unsigned int uiHasBeenAckRequestSuspendAnybusOp(void)

unsigned int uiHasBeenRequestSuspendAnybusOp(void){
	if (signalSuspendAnybus.ucRequestSuspend)
		return 1;
	return 0;
}

unsigned int uiRequestSuspendAnybusOp(unsigned char ucRequest){
	if (ucRequest){
		signalSuspendAnybus.ucRequestSuspend=1;
	}
	else{
		signalSuspendAnybus.ucRequestSuspend=0;
		// reset anche dell'ack per non correre il rischio di trovarmelo impostato la prossima volta!!!!
		signalSuspendAnybus.ucRequestSuspendAck=0;
	}
	return signalSuspendAnybus.ucRequestSuspend;
}

unsigned int uiSignalSuspendAnybusOp(unsigned char ucSignal){
	if (ucSignal){
		signalSuspendAnybus.ucRequestSuspendAck=1;
	}
	else{
		signalSuspendAnybus.ucRequestSuspendAck=0;
	}
	return 1;
}


// gestione macchina a stati modulo anybus...
void vHMA_handle(void){
	xdata unsigned int uiNumBytes;
	xdata unsigned int uiNumBytesRead;
	xdata char *pc;
	xdata int i;

		// se gestione disabilitata perch� modulo non risponde (magari non � installato)
		// esco
		if (hma.ucDisable_ModuleNotResponding){
			return;
		}	

		hma.uiPrgRunning=PrgRunning;
		hma.uiRpmSpindle=(spiralatrice.DacOut[0]*(long)macParms.rpm_mandrino)/FS_DAC_MAX519;
		hma.uiRpmWheel1=(spiralatrice.DacOut[2]*(long)macParms.rpm_frizioni[0])/FS_DAC_MAX519;
		hma.uiRpmWheel2=(spiralatrice.DacOut[3]*(long)macParms.rpm_frizioni[1])/FS_DAC_MAX519;
		// formatto stringa vel m/min
		memset(hma.ucVelMMin,0,sizeof(hma.ucVelMMin));
		if ((velMMin<0)||(velMMin>100))
			strcpy(hma.ucVelMMin,"0.00 ");
		else
			sprintf(hma.ucVelMMin,"%-6.2f",velMMin);
		// faccio in modo che occupi esattamente il numero di chars richiesto
		sprintf(hma.ucVelMMin,"%-*.*s",sizeof(hma.ucVelMMin)-1,sizeof(hma.ucVelMMin)-1,hma.ucVelMMin);
#if 0
		// aggiungo blanks in coda...
		i=strlen(hma.ucVelMMin);
		if (i<sizeof(hma.ucVelMMin)-1)
			memset(hma.ucVelMMin+i,'*',sizeof(hma.ucVelMMin)-1-i);
#endif

		if ((alr_status==alr_status_alr)||hM.ucAlarmStatus){
			hma.ucAlarmString[0]=0;
			i=0;
			while (i<16){
				if (alarms&(1L<<i)){
					vStringLangCopy((unsigned char*)hma.ucAlarmString,enumStr20_Allarmi+i);
					break;
				}
				i++;
			}
		}
		else{
			if (PrgRunning)
				strncpy(hma.ucAlarmString,"RUNNING",sizeof(hma.ucAlarmString));
			else
				strncpy(hma.ucAlarmString,"HALT",sizeof(hma.ucAlarmString));
		}
		if (uiCanIAcceptRemoteControl()){
			strncpy(hma.ucRemoteOk,"READY",sizeof(hma.ucRemoteOk));
		}
		else{
			strncpy(hma.ucRemoteOk,"BUSY",sizeof(hma.ucRemoteOk));
		}


		// gestione comunicazione a basso livello via uart con modulo anybus
		vAnybusTxRx_Handle();

		// inserimento nel log anybus delle righe di log produzione
		vLog_production_log();

		// contatore per timeouts...
		hma.tick.ucActCntInterrupt=ucCntInterrupt;
		if (hma.tick.ucActCntInterrupt>=hma.tick.ucPrevCntInterrupt){
			hma.tick.ulTickValue+=hma.tick.ucActCntInterrupt-hma.tick.ucPrevCntInterrupt;
		}
		else
			hma.tick.ulTickValue+=(256U+hma.tick.ucActCntInterrupt)-hma.tick.ucPrevCntInterrupt;
		hma.tick.ucPrevCntInterrupt=hma.tick.ucActCntInterrupt;

		// incremento contatore di heartbeat che mi serve per fra vedere che comunicazione � attiva
		hma.uiHeartBeat++;


		switch(hma.status){
			case enumStatusMainAnybus_InitAutobaud:
				// segnalo a me stesso che il file codici prodotto � stato modificato, in modo che venga
				// spedito al pc all'avvio 
				ucSignalFile_Modified(enumProductFile, 1,enumFlagsFileModified_Anybus);
				hma.status=enumStatusMainAnybus_DoAutobaud;
				hma.ucNumLoopAutoBaud=0;
				break;
			case enumStatusMainAnybus_DoAutobaud:
				if (ucPostAnybusMessage_generalParameterRead(enumAnybusGP_ModuleMode, 1)){
					hma.status=enumStatusMainAnybus_LoopAutobaud;
					break;
				}
				break;


			case enumStatusMainAnybus_LoopAutobaud:
				if (!ucAnybus_TxMessageBusy()){
					// se messaggio tx ok
					if (ucAnybus_TxMessageOk()){
						if (++hma.ucAutobaud_NumMsgTxRxOk>=defHMA_MinNumMsgOKTxRx){
							if (hma.resetdone)
								hma.status=enumStatusMainAnybus_SetNormalMode;
							else
								hma.status=enumStatusMainAnybus_ResetModule;
							break;
						}
					}
					else{
						hma.ucAutobaud_NumMsgTxRxOk=0;
						hma.ucAutobaud_NumMsgTxRxKo++;
					}
				
					if (++hma.ucNumLoopAutoBaud>=defHMA_MinNumMsg_Autobaud){
						if (hma.resetdone)
							hma.status=enumStatusMainAnybus_SetNormalMode;
						else
							hma.status=enumStatusMainAnybus_ResetModule;
					}
					else
						hma.status=enumStatusMainAnybus_DoAutobaud;
					break;
				}
				break;
			case enumStatusMainAnybus_ResetModule:
				if (ucPostAnybusMessage_generalParameterWrite(enumAnybusGP_ModuleMode, 3)){
					hma.status=enumStatusMainAnybus_ResetModule_Wait;
					break;
				}
				break;
			case enumStatusMainAnybus_ResetModule_Wait:
				if (!ucAnybus_TxMessageBusy()){
					hma.resetdone=1;
					hma.status=enumStatusMainAnybus_InitAutobaud;
				}
				break;
			case enumStatusMainAnybus_SetNormalMode:
				if (ucPostAnybusMessage_generalParameterWrite(enumAnybusGP_ModuleMode, 1)){
					hma.status=enumStatusMainAnybus_SetNormalMode_wait;
					break;
				}
				break;
			case enumStatusMainAnybus_SetNormalMode_wait:
				if (!ucAnybus_TxMessageBusy()){
					hma.status=enumStatusMainAnybus_ReadInitialParams;
				}
				break;

				
//
// ****************************************
// ****************************************
//
// parametri anybus che vanno letti una sola volta all'avvio del programma
// es. mac address
//
// ****************************************
// ****************************************
//
			case enumStatusMainAnybus_ReadInitialParams:
				if (ucPostAnybusMessage_generalParameterRead(hma_pars[hma.ucIdxReadPars].uiAddress,hma_pars[hma.ucIdxReadPars].uiNumOf)){
					hma.status=enumStatusMainAnybus_ReadInitialParams_wait;
					break;
				}
				break;
			case enumStatusMainAnybus_ReadInitialParams_wait:
				if (!ucAnybus_TxMessageBusy()){
					if (!ucAnybus_TxMessageOk()){
						hma.ucAutobaud_NumMsgTxRxKo++;					
					}
					else{
						// copio tanti bytes quanti sono il numero di registri *2
						memcpy(hma_pars[hma.ucIdxReadPars].pui,pucAnybus_GetReadRegMsgBody(),hma_pars[hma.ucIdxReadPars].uiNumOf*2);
						hma.ucIdxReadPars++;
						if (hma_pars[hma.ucIdxReadPars].pui==NULL){
							// ATTENZIONE! QUI DECIDO SE LA COMUNICAZIONE CON ANYBUS DEVE CONTINUARE O NO
							// SE IL MAC ADDRESS E' VUOTO, NON HA SENSO PROSEGUIRE
							if (  (hma.ucMacAddress[0]==0)&&(hma.ucMacAddress[1]==0)
								&&(hma.ucMacAddress[2]==0)&&(hma.ucMacAddress[3]==0)
								&&(hma.ucMacAddress[4]==0)&&(hma.ucMacAddress[5]==0)
								&&(hma.ucMacAddress[6]==0)&&(hma.ucMacAddress[6]==0)
								){
									hma.ucDisable_ModuleNotResponding=1;
							}
                            {
                                // risolve bug di visualizzazione non corretta in caso di mac address bytes con bit 7 impostato (es C0 diventava FFC0)
                                unsigned int ui_codes[6],i;
                                pc=(char*)hma.ucMacAddress;
                                i=0;
                                while (i<6){
                                    ui_codes[i]=pc[i]&0xff;
                                    i++;
                                }
                                sprintf((char*)hma.ucMacAddress_String,"%02X-%02X-%02X-%02X-%02X-%02X",ui_codes[0],ui_codes[1],ui_codes[2],ui_codes[3],ui_codes[4],ui_codes[5]);
                            }
							hma.status=enumStatusMainAnybus_SetApplicationParam;
							hma.uiIdxParam=0;
							break;
						}
					}
				
					break;
				}
				break;


				
//
// ****************************************
// ****************************************
//
// creazione application parameters
//
// ****************************************
// ****************************************
//
			case enumStatusMainAnybus_SetApplicationParam:
				if (ucCreateApplicationParameter((TipoStructAnybus_ApplicationParameters *)&anybus_ApplParams[hma.uiIdxParam])){
					hma.status=enumStatusMainAnybus_SetApplicationParam_Wait;
					break;
				}
				break;
			case enumStatusMainAnybus_SetApplicationParam_Wait:
				if (!ucAnybus_TxMessageBusy()){
					if (!ucAnybus_TxMessageOk()){
						hma.ucAutobaud_NumMsgTxRxKo++;
					}
					if ((hma.uiIdxParam>=sizeof(hma.uiAnyBusApplParamsAddress)/sizeof(hma.uiAnyBusApplParamsAddress[0])-1)
                        ||(anybus_ApplParams[hma.uiIdxParam+1].uiParameterSize==0)
                       )
                        {
						hma.status=enumStatusMainAnybus_LoopUpdateParams;
						hma.uiIdxParam=0;
					}
					else{
						hma.status=enumStatusMainAnybus_SetApplicationParam;
						hma.uiIdxParam++;
					}
					break;
				}
				break;


//
// ****************************************
// ****************************************
//
// loop principale
//
// ****************************************
// ****************************************
//
			case enumStatusMainAnybus_SuspendAnybusOp:
				if (uiHasBeenRequestSuspendAnybusOp()){
					uiSignalSuspendAnybusOp(1);
					break;
				}
				uiSignalSuspendAnybusOp(0);
				hma.status=enumStatusMainAnybus_LoopUpdateParams;
				break;

			case enumStatusMainAnybus_LoopUpdateParams:
				if (uiHasBeenRequestSuspendAnybusOp()){
					hma.status=enumStatusMainAnybus_SuspendAnybusOp;
					break;
				}
				while(1){
					if (hma.uiIdxParam>=sizeof(hma.uiAnyBusApplParamsAddress)/sizeof(hma.uiAnyBusApplParamsAddress[0])-1){
						hma.uiIdxParam=0;
						hma.status=enumStatusMainAnybus_RegenProductCodes;
						break;
					}
					// rinfresco parametri in autorefresh
					if (anybus_ApplParams[hma.uiIdxParam].ucAutoRefresh){
						// se parametro � logstring, devo verificare se e' cambiato e non trasmetterlo a testa bassa
						if (strncmp(anybus_ApplParams[hma.uiIdxParam].cName,"LogString_",10)==0){
							// trovo indice della log string in esame
							uiNumBytes=atoi(anybus_ApplParams[hma.uiIdxParam].cName+10);
							// verifico per sicurezza di non sfondare l'array
							// e poi controllo se stringa � cambiata...
							if (  (uiNumBytes>=sizeof(hma.ucChanged_LogString)/sizeof(hma.ucChanged_LogString[0]))
								||!hma.ucChanged_LogString[uiNumBytes]){
								hma.uiIdxParam++;
								continue;
							}
							// reset flag stringa cambiata, adesso passo ad aggiornarla...
							hma.ucChanged_LogString[uiNumBytes]=0;
						}
						if (ucPostAnybusMessage_updateParameter((TipoStructAnybus_ApplicationParameters *)&anybus_ApplParams[hma.uiIdxParam])){
							// incremento indice per trovarmi pronto su prossimo parametro...
							hma.uiIdxParam++;
							hma.status=enumStatusMainAnybus_LoopUpdateParams_Wait;
						}
						break;
					}
					// se parametro non � in autorefresh, passo al prossimo
					else{
						hma.uiIdxParam++;
					}
				}
				break;

			case enumStatusMainAnybus_LoopUpdateParams_Wait:
				if (!ucAnybus_TxMessageBusy()){
					if (!ucAnybus_TxMessageOk()){
						hma.ucAutobaud_NumMsgTxRxKo++;
					}
					hma.status=enumStatusMainAnybus_LoopUpdateParams;
					break;
				}
				break;




#if 1

//
// ****************************************
// ****************************************
//
// sequenza di rigenerazione file = scrittura file su modulo anybus
//
// ****************************************
// ****************************************
//
			case enumStatusMainAnybus_Regen_FInit:
				// inizializzo procedura che mi deve fornire i dati...
				hma.fileReGenSeq.ucInitDataProducer();
				// cancello il file...
				if (ucAnybus_FileDelete(hma.fileReGenSeq.pcName))
					hma.status=enumStatusMainAnybus_Regen_FDelete_Wait;
				break;

			case enumStatusMainAnybus_Regen_FDelete_Wait:
				if (!ucAnybus_TxMessageBusy()){
					hma.status=enumStatusMainAnybus_Regen_FOpen;
				}
				break;
			case enumStatusMainAnybus_Regen_FOpen:
				if (ucAnybus_FileOpen(hma.fileReGenSeq.pcName, enumAnybusFileOpenMode_Write ,&(hma.uiFileInstance))){
					hma.status=enumStatusMainAnybus_Regen_FOpen_Wait;
					break;
				}
				break;
			case enumStatusMainAnybus_Regen_FOpen_Wait:
				if (!ucAnybus_TxMessageBusy()){
					if (ucAnybus_TxMessageOk()){
						hma.status=enumStatusMainAnybus_Regen_FWrite;
						break;
					}
					else{
						hma.status=enumStatusMainAnybus_Regen_FClose;
					}
				}
				break;
			case enumStatusMainAnybus_Regen_FWrite:
				// chiedo max 100 bytes
				hma.ucNumByteFileBuffer=hma.fileReGenSeq.ucDataProducer((char*)hma.ucFileBuffer,100);
				// se i bytes non ci sono, ho finito
				if (!hma.ucNumByteFileBuffer){
					hma.status=enumStatusMainAnybus_Regen_FClose;
					break;
				}
				if (ucAnybus_FileWrite(hma.uiFileInstance, hma.ucFileBuffer ,hma.ucNumByteFileBuffer)){
					hma.status=enumStatusMainAnybus_Regen_FWrite_Wait;
					break;
				}
				break;
			case enumStatusMainAnybus_Regen_FWrite_Wait:
				if (!ucAnybus_TxMessageBusy()){
					if (ucAnybus_TxMessageOk()){
						hma.status=enumStatusMainAnybus_Regen_FWrite;
						break;
					}
					else{
						hma.status=enumStatusMainAnybus_Regen_FClose;
					}
				}
				break;
			case enumStatusMainAnybus_Regen_FClose:
				if (ucAnybus_FileClose(hma.uiFileInstance)){
					hma.status=enumStatusMainAnybus_Regen_FClose_Wait;
					break;
				}
				break;
			case enumStatusMainAnybus_Regen_FClose_Wait:
				if (!ucAnybus_TxMessageBusy()){
					hma.status=hma.fileReGenSeq.returnStatus;
				}
				break;
//
// ****************************************
// ****************************************
//
// fine sequenza di rigenerazione file = scrittura file su modulo anybus
//
// ****************************************
// ****************************************
//


//
// ****************************************
// ****************************************
//
// sequenza di refresh file = lettura file da modulo anybus
//
// ****************************************
// ****************************************
//
			case enumStatusMainAnybus_Refresh_FInit:
				// inizializzo data consumer
				hma.fileRefreshSeq.ucInitDataConsumer();
				// apertura del file in lettura
				if (ucAnybus_FileOpen(hma.fileRefreshSeq.pcName, enumAnybusFileOpenMode_Read ,&(hma.uiFileInstance))){
					hma.status=enumStatusMainAnybus_Refresh_FOpen_Wait;
					break;
				}
				break;
			// attendo che file si apra
			case enumStatusMainAnybus_Refresh_FOpen_Wait:
				if (!ucAnybus_TxMessageBusy()){
					// se messaggio mi d� l'ok, procedo con la lettura
					if (ucAnybus_TxMessageOk()){
						hma.status=enumStatusMainAnybus_Refresh_FRead;
						break;
					}
					else{
						hma.ucEsitoError=1;
						hma.status=hma.fileRefreshSeq.returnStatus;
					}
				}
				break;
			case enumStatusMainAnybus_Refresh_FRead:
				// leggo max 128 bytes alla volta...
				// potrebbero starcene 197 ma non � il caso di questionare sulle performances...
				if (ucAnybus_FileRead(hma.uiFileInstance, 128)){
					hma.status=enumStatusMainAnybus_Refresh_FRead_Wait;
					break;
				}
				break;
			case enumStatusMainAnybus_Refresh_FRead_Wait:
				if (!ucAnybus_TxMessageBusy()){
					if (ucAnybus_TxMessageOk()){
						// se codice di errore diverso da zero... chiudo il file...				
   						if (uiFileOperationErrorCode()!=0){
							hma.ucEsitoError=1;
							hma.status=enumStatusMainAnybus_Refresh_FClose;
							break;
						}
						// ci sono bytes?
						// mi procuro il ptr alla parte dati
						// i primi due bytes sono la lunghezza (prima msB poi lsB)
						// poi segue lo stream dei bytes
						hma.puc=pucFileOperationData();
						// se nessun byte arrivato, fine file
						if (!(hma.puc[0]|hma.puc[1])){
							hma.status=enumStatusMainAnybus_Refresh_FClose;
							break;
						}
						else{
							unsigned int ui_num_bytes_to_swap;
							ui_num_bytes_to_swap=hma.puc[0]+256L*hma.puc[1];
							// i bytes sono disponibili a partire da hma.puc+2
							//la lunghezza si trova in hma.puc[0,1]
							//faccio swap dei bytes del body...
							{
								unsigned int i;
								unsigned char *puc,uc;
								puc=hma.puc+2;
                                i=0;
								while (i<ui_num_bytes_to_swap/2){
									uc=puc[0];
									puc[0]=puc[1];
									puc[1]=uc;
									puc+=2;
                                    i++;
								}
							}
							// lancio data consumer
							hma.fileRefreshSeq.ucDataConsumer((char*)hma.puc+2,ui_num_bytes_to_swap);
							hma.status=enumStatusMainAnybus_Refresh_FRead;
							break;
						}
						break;
					}
					else{
						hma.ucEsitoError=1;
						hma.status=enumStatusMainAnybus_Refresh_FClose;
					}
				}
				break;
			case enumStatusMainAnybus_Refresh_FClose:
				if (ucAnybus_FileClose(hma.uiFileInstance))
					hma.status=enumStatusMainAnybus_Refresh_FClose_Wait;
				break;
			case enumStatusMainAnybus_Refresh_FClose_Wait:
				if (!ucAnybus_TxMessageBusy()){
					hma.status=hma.fileRefreshSeq.returnStatus;
				}
				break;
//
// ****************************************
// ****************************************
//
// fine sequenza di refresh file = lettura file da modulo anybus
//
// ****************************************
// ****************************************
//
#endif



// verifico se il file dei codici prodotto+parametri deve essere rigenerato
			case enumStatusMainAnybus_RegenProductCodes:
				// se semaforo attivo o prgrunning, non rigenero files...
				if (hma.semaphor||PrgRunning){
					hma.status=enumStatusMainAnybus_RegenProductCodes_end;
					break;
				}

				if (ucHasBeenFile_Modified(enumProductFile,enumFlagsFileModified_Anybus)){
					// segnalo che il file codici prodotto non � pi� modificato
					ucSignalFile_Modified(enumProductFile, 0,enumFlagsFileModified_Anybus);
				}
				else if (ucHasBeenFile_Modified(enumMacParamsFile,enumFlagsFileModified_Anybus)){
					// segnalo che il file parametri macchina non � pi� modificato
					ucSignalFile_Modified(enumMacParamsFile, 0,enumFlagsFileModified_Anybus);
				}
				else{
					hma.status=enumStatusMainAnybus_RegenProductCodes_end;
					break;
				}
				// passo ad inizializzare la macchina a stati...
				vInitMachineParsOnFile_Stati();
				hma.status=enumStatusMainAnybus_RegenProductCodes_handle;
				break;

			case enumStatusMainAnybus_RegenProductCodes_handle:
				// chiamo la routine col parametro 1 per indicare di salvare su ethernet
				if (!ucSaveMachineParsOnFile_Stati(1)){
					hma.status=enumStatusMainAnybus_RegenProductCodes_end;
					break;
				}
				switch(spsi.op){
					case savePars_return_op_none:
						break;
					case savePars_return_op_init:
						hma.status=enumStatusMainAnybus_RegenProductCodes_init;
						break;
					case savePars_return_op_openfile:
						hma.status=enumStatusMainAnybus_RegenProductCodes_fopen;
						break;
					case savePars_return_op_printfile:
						hma.status=enumStatusMainAnybus_RegenProductCodes_fprintf;
						break;
					case savePars_return_op_closefile:
						hma.status=enumStatusMainAnybus_RegenProductCodes_fclose;
						break;
					default:
						break;
				}
				break;
			case enumStatusMainAnybus_RegenProductCodes_init:
				// cancello il file...
				if (ucAnybus_FileDelete(defAZ4181_product_file_name))
					hma.status=enumStatusMainAnybus_RegenProductCodes_init_wait;
				break;

			case enumStatusMainAnybus_RegenProductCodes_init_wait:
				if (!ucAnybus_TxMessageBusy()){
					hma.status=enumStatusMainAnybus_RegenProductCodes_handle;
				}
				break;

			case enumStatusMainAnybus_RegenProductCodes_fopen:
				if (ucAnybus_FileOpen(	defAZ4181_product_file_name, 
										enumAnybusFileOpenMode_Write,
										&(hma.uiFileInstance)
									  )){
					hma.status=enumStatusMainAnybus_RegenProductCodes_fopen_wait;
					break;
				}
				break;
			case enumStatusMainAnybus_RegenProductCodes_fopen_wait:
				if (!ucAnybus_TxMessageBusy()){
					if (ucAnybus_TxMessageOk()){
						hma.status=enumStatusMainAnybus_RegenProductCodes_handle;
						break;
					}
					else{
						hma.status=enumStatusMainAnybus_RegenProductCodes_end;
					}
				}
				break;

			case enumStatusMainAnybus_RegenProductCodes_fprintf:
				hma.ucNumByteFileBuffer=strlen(spsi.pc2save);
				if (ucAnybus_FileWrite(hma.uiFileInstance, (unsigned char*)spsi.pc2save ,hma.ucNumByteFileBuffer)){
					hma.status=enumStatusMainAnybus_RegenProductCodes_fprintf_wait;
					break;
				}
				break;
			case enumStatusMainAnybus_RegenProductCodes_fprintf_wait:
				if (!ucAnybus_TxMessageBusy()){
					// gestire msg di errore???
					if (!ucAnybus_TxMessageOk()){
					}
					hma.status=enumStatusMainAnybus_RegenProductCodes_handle;
				}
				break;
			case enumStatusMainAnybus_RegenProductCodes_fclose:
				if (ucAnybus_FileClose(hma.uiFileInstance)){
					hma.status=enumStatusMainAnybus_RegenProductCodes_fclose_wait;
					break;
				}
				break;
			case enumStatusMainAnybus_RegenProductCodes_fclose_wait:
				if (!ucAnybus_TxMessageBusy()){
					hma.status=enumStatusMainAnybus_RegenProductCodes_handle;
			    }
				break;

			case enumStatusMainAnybus_RegenProductCodes_end:
				// michele per test solo creazione file codici prodotto...
				//hma.status=enumStatusMainAnybus_ReloadProductCodes;
				hma.status=enumStatusMainAnybus_RegenJoblist;
				break;


//
// ****************************************
// ****************************************
//
// inizio sequenza di importazione file product codes e parametri macchina da pc
//
// ****************************************
// ****************************************
//

// verifico se il file dei codici prodotto deve essere caricato da pc
			case enumStatusMainAnybus_ReloadProductCodes:
				// sono in attesa yes/no???
				if (hma.semaphor==1){
					hma.status=enumStatusMainAnybus_ReloadProductCodes_waitYesNo;
					break;
				}
				// sono in attesa yes/no su semaforo aggiornamento joblist???
				else if (hma.semaphor==2){
					hma.status=enumStatusMainAnybus_LoopUpdateParams;
					break;
				}
				// se esiste file defAZ4181_product_file_name_pc
				// allora devo leggerlo e scaricarlo nel mio file codici prodotto+ parametri macchina
				if (ucAnybus_FileOpen(  defAZ4181_product_file_name_pc, 
										enumAnybusFileOpenMode_Read ,&(hma.uiFileInstance))
								      ){
					hma.status=enumStatusMainAnybus_ReloadProductCodes_ctrlNeeded;
					break;
				}
				break;

// verifica se apertura file file che contiene parametri+codici prodotto finisce correttamente
			case enumStatusMainAnybus_ReloadProductCodes_ctrlNeeded:
				if (!ucAnybus_TxMessageBusy()){
					// se messaggio mi d� l'ok, procedo con la lettura
					if (ucAnybus_TxMessageOk()){
						// se codice di errore=0 allora il file esiste
						if (uiFileOperationErrorCode()==0){
							if (hma.LoadCodici_Fase==0){
								vRemoteCommandsFlagsInit();
							}
							hma.status=enumStatusMainAnybus_ReloadProductCodes_start;
						}
						else
							hma.status=enumStatusMainAnybus_ReloadProductCodes_end_all;
						break;
					}
					// se invece il file non esiste, fine procedura...
					else{
						hma.status=enumStatusMainAnybus_ReloadProductCodes_end_all;
					}
				}
				break;


			case enumStatusMainAnybus_ReloadProductCodes_start:
				// se non posso accettare controllo remoto, errore
				if (!uiCanIAcceptRemoteControl()){
					hma.ucEsitoError=1;
					vRemoteCommandAskingSet(0);
					hma.status=enumStatusMainAnybus_ReloadProductCodes_fclose;
					break;
				}
				if (hma.LoadCodici_Fase==0){
					hma.status=enumStatusMainAnybus_ReloadProductCodes_waitYesNo;
					// salvo handle del file su cui lavoro
					hma.uiFileInstance_codici=hma.uiFileInstance;
					// indico richiesta comando remoto in corso
					vRemoteCommandAskingSet(1);
				}
				else{
					hma.status=enumStatusMainAnybus_ReloadProductCodes_Yes;
				}

				break;
			case enumStatusMainAnybus_ReloadProductCodes_waitYesNo:
				// se risposto di no...
				if (hma.ucRemoteCommandReply&&!hma.ucRemoteCommandResponseYes){
					hma.ucEsitoError=1;
					// indico che NON sono in attesa su yes no codici prodotto
					hma.semaphor=0;
					vRemoteCommandAskingSet(0);
					hma.status=enumStatusMainAnybus_ReloadProductCodes_fclose;
					break;
				}
				// se non ha ancora risposto ??? devo aspettare!
				if (!hma.ucRemoteCommandReply){
					// indico che sono in attesa su yes no codici prodotto
					hma.semaphor=1;
					hma.status=enumStatusMainAnybus_LoopUpdateParams;
					break;
				}
				// se arrivo qui, ha risposto s�
				hma.status=enumStatusMainAnybus_ReloadProductCodes_Yes;
				break;

			case enumStatusMainAnybus_ReloadProductCodes_Yes:
				// recupero handle del file su cui lavorare
				hma.uiFileInstance=hma.uiFileInstance_codici;
				hma.semaphor=0;
				// se sono qui, ha risposto s�
				// indico comando remoto in corso
				hma.ucRemoteCommand=1;
				hma.ucEsitoError=0;
				vForceLcdRefresh();
				// passo ad inizializzare la macchina a stati...
				vInitMachineParsOnFile_Stati();
				// indico esito ok
				hma.ucEsitoError=0;
				// reset struttura gsnpc
				memset(&gsnpc,0,sizeof(gsnpc));
				// indico che dato non ready
				spsi.ucDataReady=0;
				hma.status=enumStatusMainAnybus_ReloadProductCodes_handle;
				break;

			case enumStatusMainAnybus_ReloadProductCodes_handle:
				// fase 0: carica codici prodotto
				if (hma.LoadCodici_Fase==0){
					// chiamo la routine col parametro 1 per indicare di caricare da ethernet
					if (!ucLoadCodiciProdottoOnFile_Stati(1)){
						hma.status=enumStatusMainAnybus_ReloadProductCodes_end;
						break;
					}
				}
				// fase 1: carica parametri macchina
				else {
					// chiamo la routine col parametro 1 per indicare di caricare da ethernet
					if (!ucLoadParMacOnFile_Stati(1)){
						hma.status=enumStatusMainAnybus_ReloadProductCodes_end;
						break;
					}
				}
				switch(spsi.op){
					case savePars_return_op_none:
						break;
					case savePars_return_op_init:
						hma.status=enumStatusMainAnybus_ReloadProductCodes_init;
						break;
					case savePars_return_op_openfile:
						hma.status=enumStatusMainAnybus_ReloadProductCodes_fopen;
						break;
					case savePars_return_op_inputfile:
						hma.status=enumStatusMainAnybus_ReloadProductCodes_fgets;
						break;
					case savePars_return_op_closefile:
						hma.status=enumStatusMainAnybus_ReloadProductCodes_fclose;
						break;
					default:
						break;
				}
				break;
			case enumStatusMainAnybus_ReloadProductCodes_init:
				hma.status=enumStatusMainAnybus_ReloadProductCodes_handle;
				break;

			// il file � gi� aperto...
			case enumStatusMainAnybus_ReloadProductCodes_fopen:
				hma.status=enumStatusMainAnybus_ReloadProductCodes_handle;
				break;

			case enumStatusMainAnybus_ReloadProductCodes_fgets:
				// se c'� il terminatore di linea, allora posso passare il buffer
				// verifico sia terminatore \r\n che \n
				pc=(xdata char *)strstr(gsnpc.s,"\r\n");
				if (pc==NULL){
					pc=(xdata char *)strchr(gsnpc.s,'\n');
					if (pc==NULL){
						// leggi ancora perch� non hai trovato una linea completa!
						hma.status=enumStatusMainAnybus_ReloadProductCodes_fgets_do;
						break;
					}
				}
				// se arrivo qui, la mia linea � terminata da "\r\n" (OPPURE E' L'ULTIMA RIGA DEL FILE), 
				// devo eliminare tutti i \r\n
				// intanto metto tappo (funziona anche con ultima riga del file)
				*pc=0;
				// copio la porzione "buona" di stringa
				strcpy(spsi.pc2save,gsnpc.s);
				// salto tutti gli spaces successivi al \r\n o al \n
				// non ho paura di sforare perch� gsnpc.s ha sempre spazio a sufficienza per caratteri tappo alla fine
				pc=pcSkipBlank(pc+1);
				// copio la parte rimanente
				strcpy(gsnpc.s,pc);
				// indico che dato ready
				spsi.ucDataReady=1;
				// passo alla handle
				hma.status=enumStatusMainAnybus_ReloadProductCodes_handle;
				break;
			case enumStatusMainAnybus_ReloadProductCodes_fgets_do:
				// leggo max 128 bytes alla volta...
				// potrebbero starcene 197 ma non � il caso di questionare sulle performances...
				if (ucAnybus_FileRead(hma.uiFileInstance, 128))
					hma.status=enumStatusMainAnybus_ReloadProductCodes_fgets_wait;
				break;
			case enumStatusMainAnybus_ReloadProductCodes_fgets_wait:
				if (ucAnybus_TxMessageBusy()){
					break;
				}
				if (!ucAnybus_TxMessageOk()){
					hma.status=enumStatusMainAnybus_ReloadProductCodes_fclose;
					break;
				}
				// se codice di errore diverso da zero... chiudo il file...				
				if (uiFileOperationErrorCode()!=0){
					hma.status=enumStatusMainAnybus_ReloadProductCodes_fclose;
					break;
				}
				// ci sono bytes?
				// mi procuro il ptr alla parte dati
				// i primi due bytes sono la lunghezza (prima msB poi lsB)
				// poi segue lo stream dei bytes
				hma.puc=pucFileOperationData();
				// se nessun byte arrivato, fine file--> devo fare flush del buffer
				if (!(hma.puc[0]|hma.puc[1])){
					// appendo terminatore di linea per dire che questa � ultima linea del file
					strcat(gsnpc.s,"\r\n");
				}
				else{
					// i bytes sono disponibili a partire da hma.puc+2
					// la lunghezza si trova in hma.puc[0,1]
					// appendo a stringa esistente
					uiNumBytes=sizeof(gsnpc.s)-strlen(gsnpc.s)-1;
					uiNumBytesRead=hma.puc[0]*256+hma.puc[1];
					// non dovrebbe mai capitare di sforare...
					if (uiNumBytesRead>uiNumBytes)
						uiNumBytesRead=uiNumBytes;
                    //faccio swap dei bytes del body...
                    {
                        unsigned int i;
                        unsigned char *puc,uc;
                        puc=hma.puc+2;
                        i=0;
                        while (i<uiNumBytesRead/2){
                            uc=puc[0];
                            puc[0]=puc[1];
                            puc[1]=uc;
                            puc+=2;
                            i++;
                        }
                    }
					// appendo porzione di stringa giunta
					strncat(gsnpc.s,(char*)hma.puc+2,uiNumBytesRead);
				}
				hma.status=enumStatusMainAnybus_ReloadProductCodes_fgets;
				break;

			case enumStatusMainAnybus_ReloadProductCodes_fclose:
				if (ucAnybus_FileClose(hma.uiFileInstance))
					hma.status=enumStatusMainAnybus_ReloadProductCodes_fclose_wait;
				break;
			case enumStatusMainAnybus_ReloadProductCodes_fclose_wait:
				if (!ucAnybus_TxMessageBusy()){
					// se sono nella fase 0, passo alla fase 1
					if ((hma.LoadCodici_Fase==0)&&!hma.ucEsitoError){
						hma.LoadCodici_Fase=1;
						hma.status=enumStatusMainAnybus_ReloadProductCodes;
						break;
					}
					// reset fase!!!!!
					hma.LoadCodici_Fase=0;
					hma.status=enumStatusMainAnybus_ReloadProductCodes_fclose_delete;
			    }
				break;
			case enumStatusMainAnybus_ReloadProductCodes_fclose_delete:
				if (ucAnybus_FileDelete(defAZ4181_product_file_name_pc))
					hma.status=enumStatusMainAnybus_ReloadProductCodes_fclose_delete_wait;
				break;
			case enumStatusMainAnybus_ReloadProductCodes_fclose_delete_wait:
				if (!ucAnybus_TxMessageBusy()){
					hma.status=enumStatusMainAnybus_ReloadProductCodes_handle;
				}
				break;
			case enumStatusMainAnybus_ReloadProductCodes_error:
				// indico esito errore
				hma.ucEsitoError=1;
				hma.LoadCodici_Fase=0;
				hma.status=enumStatusMainAnybus_ReloadProductCodes_end;
				break;

			case enumStatusMainAnybus_ReloadProductCodes_end:

				if (ucAnybus_FileOpen(  defAZ4181_product_file_name_result, 
										enumAnybusFileOpenMode_Write ,&(hma.uiFileInstance))
								      ){
					hma.status=enumStatusMainAnybus_ReloadProductCodes_end_waitopen;
					break;
				}
				break;

			case enumStatusMainAnybus_ReloadProductCodes_end_waitopen:
				if (!ucAnybus_TxMessageBusy()){
					hma.status=enumStatusMainAnybus_ReloadProductCodes_end_writeresult;
				}
				break;
			case enumStatusMainAnybus_ReloadProductCodes_end_writeresult:
				if (hma.ucEsitoError){
					strcpy((char*)hma.ucFileBuffer,"result: ERROR\r\n");
				}
				else{
					strcpy((char*)hma.ucFileBuffer,"result: ok\r\n");
				}
				hma.ucNumByteFileBuffer=strlen((char*)hma.ucFileBuffer);
				if (ucAnybus_FileWrite(hma.uiFileInstance, hma.ucFileBuffer ,hma.ucNumByteFileBuffer)){
					hma.status=enumStatusMainAnybus_ReloadProductCodes_end_wait_writeresult;
					break;
				}
				break;

			case enumStatusMainAnybus_ReloadProductCodes_end_wait_writeresult:
				if (!ucAnybus_TxMessageBusy()){
					hma.status=enumStatusMainAnybus_ReloadProductCodes_end_fclose;
				}
				break;
			case enumStatusMainAnybus_ReloadProductCodes_end_fclose:
				if (ucAnybus_FileClose(hma.uiFileInstance))
					hma.status=enumStatusMainAnybus_ReloadProductCodes_end_fclose_wait;
				break;
			case enumStatusMainAnybus_ReloadProductCodes_end_fclose_wait:
				if (!ucAnybus_TxMessageBusy()){
					hma.status=enumStatusMainAnybus_ReloadProductCodes_end_all;
				}
				break;
			case enumStatusMainAnybus_ReloadProductCodes_end_all:
				// reset fase!!!!!
				hma.LoadCodici_Fase=0;
				// indico comando remoto NON in corso
				hma.ucRemoteCommand=0;
				vForceLcdRefresh();
				hma.status=enumStatusMainAnybus_LoopUpdateParams;
				break;



//
// ****************************************
// ****************************************
//
// inizio sequenza di importazione file joblist da pc
//
// ****************************************
// ****************************************
//

			case enumStatusMainAnybus_Jobs_Refresh:
				// sono in attesa yes/no???
				if (hma.semaphor==2){
					hma.status=enumStatusMainAnybus_Jobs_waitYesNo;
					break;
				}
				// sono in attesa yes/no su semaforo aggiornamento product list???
				else if (hma.semaphor==1){
					hma.status=enumStatusMainAnybus_ReloadProductCodes;
					break;
				}
				// se esiste file defAZ4181_productlist_pc_file_name
				// allora devo leggerlo e scaricarlo nella mia product list...
				if (ucAnybus_FileOpen(  defAZ4181_joblist_file_name_pc, 
										enumAnybusFileOpenMode_Read ,&(hma.uiFileInstance))
								      ){
				    vRemoteCommandsFlagsInit();
					hma.status=enumStatusMainAnybus_Jobs_Refresh_wait_ctrl_if;
					break;
				}
				break;

// verifica se apertura file che contiene nuova joblist finisce correttamente
			case enumStatusMainAnybus_Jobs_Refresh_wait_ctrl_if:
				if (!ucAnybus_TxMessageBusy()){
					// se messaggio mi d� l'ok, procedo con la lettura
					if (ucAnybus_TxMessageOk()){
						// se codice di errore=0 allora il file esiste
						if (uiFileOperationErrorCode()==0){
							if (ucAnybus_FileClose(hma.uiFileInstance))
								hma.status=enumStatusMainAnybus_Jobs_Refresh_wait_close_andgo;
						}
						else
							hma.status=enumStatusMainAnybus_Jobs_Refresh_end_all;
						break;
					}
					else{
						hma.status=enumStatusMainAnybus_Jobs_Refresh_end_all;
					}
				}
				break;

				break;
// aspetta chiusura file che contiene nuova joblist, poi procede ad importarla
			case enumStatusMainAnybus_Jobs_Refresh_wait_close_andgo:
				if (!ucAnybus_TxMessageBusy()){
					hma.status=enumStatusMainAnybus_Jobs_Refresh_doit;
				}
				break;


// importazione file che contiene nuova joblist
			case enumStatusMainAnybus_Jobs_Refresh_doit:
				// se non posso accettare controllo remoto, errore
				if (!uiCanIAcceptRemoteControl()){
					hma.ucEsitoError=1;
					vRemoteCommandAskingSet(0);
					hma.status=enumStatusMainAnybus_Jobs_Refresh_delfile;
					break;
				}
				hma.status=enumStatusMainAnybus_Jobs_waitYesNo;
				// salvo handle del file su cui devo lavorare...
				hma.uiFileInstance_jobs=hma.uiFileInstance;
				// indico richiesta comando remoto in corso
				vRemoteCommandAskingSet(1);
				break;


			case enumStatusMainAnybus_Jobs_waitYesNo:
				// se risposto di no...
				if (hma.ucRemoteCommandReply&&!hma.ucRemoteCommandResponseYes){
					hma.ucEsitoError=1;
					// indico che NON sono in attesa su yes no su jobs
					hma.semaphor=0;
					vRemoteCommandAskingSet(0);
					hma.status=enumStatusMainAnybus_Jobs_Refresh_delfile;
					break;
				}
				// se non ha ancora risposto ??? devo aspettare!
				if (!hma.ucRemoteCommandReply){
					// indico che sono in attesa su yes no jobs
					hma.semaphor=2;
					hma.status=enumStatusMainAnybus_ReloadProductCodes;
					break;
				}
				// recupero handle del file su cui devo lavorare...
				hma.uiFileInstance=hma.uiFileInstance_jobs;
				hma.semaphor=0;
				// se sono qui, ha risposto s�
				// indico comando remoto in corso
				hma.ucRemoteCommand=1;
				hma.ucEsitoError=0;
				vForceLcdRefresh();

				// comando avvio refresh file product archive
				ucStartFileRefresh(	defAZ4181_joblist_file_name_pc,
									ucInitDataConsumer_productList, 
									ucDataConsumer_productList,
									enumStatusMainAnybus_Jobs_Refresh_delfile
								);
				break;
// cancellazione file joblist presente su modulo anybus
			case enumStatusMainAnybus_Jobs_Refresh_delfile:
				if (ucAnybus_FileDelete(defAZ4181_joblist_file_name_pc))
					hma.status=enumStatusMainAnybus_Jobs_Refresh_delfile_wait;
				break;
// attesa cancellazione file joblist presente su modulo anybus
			case enumStatusMainAnybus_Jobs_Refresh_delfile_wait:
				if (!ucAnybus_TxMessageBusy()){
					hma.status=enumStatusMainAnybus_Jobs_Refresh_end;
				}
				break;

			case enumStatusMainAnybus_Jobs_Refresh_end:
				if (ucAnybus_FileOpen(  defAZ4181_joblist_file_name_result, 
										enumAnybusFileOpenMode_Write ,&(hma.uiFileInstance))
								      ){
					hma.status=enumStatusMainAnybus_Jobs_Refresh_end_open_wait;
					break;
				}
				break;

			case enumStatusMainAnybus_Jobs_Refresh_end_open_wait:
				if (!ucAnybus_TxMessageBusy()){
					hma.status=enumStatusMainAnybus_Jobs_Refresh_end_writeresult;
				}
				break;
			case enumStatusMainAnybus_Jobs_Refresh_end_writeresult:
				if (hma.ucEsitoError){
					strcpy((char*)hma.ucFileBuffer,"result: ERROR\r\n");
				}
				else{
					// trasferisco joblist
					vTrasferisciDaListaLavoriAcommesse();
					strcpy((char*)hma.ucFileBuffer,"result: ok\r\n");
				} 
				hma.ucNumByteFileBuffer=strlen((char*)hma.ucFileBuffer);
				if (ucAnybus_FileWrite(hma.uiFileInstance, hma.ucFileBuffer ,hma.ucNumByteFileBuffer)){
					hma.status=enumStatusMainAnybus_Jobs_Refresh_end_wait_writeresult;
					break;
				}
				break;

			case enumStatusMainAnybus_Jobs_Refresh_end_wait_writeresult:
				if (!ucAnybus_TxMessageBusy()){
					hma.status=enumStatusMainAnybus_Jobs_Refresh_end_fclose;
				}
				break;
			case enumStatusMainAnybus_Jobs_Refresh_end_fclose:
				if (ucAnybus_FileClose(hma.uiFileInstance))
					hma.status=enumStatusMainAnybus_Jobs_Refresh_end_fclose_wait;
				break;
			case enumStatusMainAnybus_Jobs_Refresh_end_fclose_wait:
				if (!ucAnybus_TxMessageBusy()){
					hma.status=enumStatusMainAnybus_Jobs_Refresh_end_all;
				}
				break;

				// fine importazione file joblist da pc
			case enumStatusMainAnybus_Jobs_Refresh_end_all:
				// indico comando remoto NON in corso
				hma.ucRemoteCommand=0;
				vForceLcdRefresh();
				hma.status=enumStatusMainAnybus_ReloadProductCodes;
				break;

//
// ****************************************
// ****************************************
//
// fine sequenza di importazione file joblist da pc
//
// ****************************************
// ****************************************
//

//
// ****************************************
// ****************************************
//
// inizio sequenza di esportazione file joblist su pc
//
// ****************************************
// ****************************************
//
			case enumStatusMainAnybus_RegenJoblist:
				// se semaforo attivo o prgrunning, non rigenero files...
				if (hma.semaphor||PrgRunning){
					hma.status=enumStatusMainAnybus_RegenJoblist_end;
					break;
				}
				if (hma.uiNumValidationCodeList==uiGetNumValidationCodeList()){
					hma.status=enumStatusMainAnybus_RegenJoblist_end;
					break;
				}
				else{
					// comando avvio rigenerazione file product list
					ucStartFileRegen(	defAZ4181_joblist_file_name,
										ucInitDataProducer_productList, 
										ucDataProducer_productList,
										enumStatusMainAnybus_RegenJoblist_end
									);
				}
				break;
			case enumStatusMainAnybus_RegenJoblist_end:
				hma.uiNumValidationCodeList=uiGetNumValidationCodeList();
				hma.status=enumStatusMainAnybus_Jobs_Refresh;
				break;

			default:
				break;
		}
}//void vHMA_handle(void)



