#include "LPC23xx.h"                        /* LPC23xx/24xx definitions */
#include <stdlib.h>
#include "type.h"
#include "target.h"
#include "fpga_nxp.h"
#include "gui.h"
#include <string.h>
#include "spiext.h"


// only for diagnostic purposes...
extern void v_print_msg(unsigned int row,char *s);
extern void vRefreshScreen(void);

// use this define to enable vdip diagnostic messages...
//#define def_enable_vdip_diag

#ifdef def_enable_vdip_diag
    char row_vdip_status[2][64];
    unsigned int ui_enable_vdip_diagnostic_messages;

    void v_refresh_vdip_status_video(unsigned int row, char *s){
        if (row>1){
            return;
        }
        strncpy(&row_vdip_status[row][0],s,sizeof(row_vdip_status[0])-1);
        v_print_msg(0,row_vdip_status[0]);
        v_print_msg(1,row_vdip_status[1]);
        vRefreshScreen();
    }
#endif

unsigned char ucVdip_just2wait;
unsigned char ucMaxLoopW=0;
unsigned int uiMaxLoopR=0;
// stringa che contiene il messaggio in arrivo da vdip
unsigned char ucStringRx_Vdip[128+32];
xdata unsigned char uc_vdip_short_pause;

void vReceiveVdipString(void);

void vVdip_pause(void){
register unsigned int i;
	for (i=0;i<180;i++)
		ucVdip_just2wait++;
}

void vVdip_very_big_pause(void){
register unsigned int i,j;
	// aspetta almeno 50ms
	for (j=0;j<10;j++){
		i=ucCntInterrupt;
		while(i==ucCntInterrupt);
	}

}

void vVdip_5ms_pause(void){
register unsigned int i,j;
	// aspetta almeno 5ms
	for (j=0;j<2;j++){
		i=ucCntInterrupt;
		while(i==ucCntInterrupt);
	}

}

unsigned char uiVdipHasData(void){
register unsigned char uiTimeout;
unsigned int xdata i;
	for (uiTimeout=0;uiTimeout<20;uiTimeout++){
		while(ucBusySpiVdip);
		for (i=0;i<500;i++){
			ucByteStatusVdip=0;
			while(ucBusySpiVdip);
			// leggo lo status per verificare se comando � stato accettato
			if (ucByteTxVdip_OldData==0)
				break;
		   // pausa fra i comandi al vdip...
			vVdip_pause();
		}				
		if ((ucByteStatusVdip&0x02)==0){
			if (uiTimeout>uiMaxLoopR)
				uiMaxLoopR=uiTimeout;
			return 1;
		}	
	   // pausa fra i comandi al vdip...
		vVdip_pause();
	}
	return 0;
}

#if 0
	void vSendVdipBinaryString(unsigned char xdata *puc,int iNumChars){
	unsigned int xdata i;
		// per vedere se vdip � pronto a rx comandi...
		if (uiVdipHasData()){
			vReceiveVdipString();	
		}
		while(iNumChars--){
			while(ucBusySpiVdip);
			for (i=0;i<800;i++){
				ucByteTxVdip_OldData=*puc;
				while(ucBusySpiVdip);
				// leggo lo status per verificare se comando � stato accettato
				if (ucByteTxVdip_OldData==0)
					break;
				// pausa fra i comandi al vdip...
				vVdip_pause();
			}
			puc++;
			// pausa fra i comandi al vdip...
			vVdip_pause();
		}
	}
#endif

void vSendVdipChar(unsigned char uc){
	xdata unsigned int i;
	while(ucBusySpiVdip);
	for (i=0;i<500;i++){
		ucByteTxVdip_OldData=uc;
		while(ucBusySpiVdip);
		// leggo lo status per verificare se comando � stato accettato
		if (ucByteTxVdip_OldData==0)
			break;
		// pausa fra i comandi al vdip...
		vVdip_pause();
	}

}

void vSendVdipString(unsigned char  *msg){
unsigned int xdata i;
#ifdef def_enable_vdip_diag
    v_refresh_vdip_status_video(0,msg);
#endif

	// per vedere se vdip � pronto a rx comandi...
	if (uiVdipHasData()){
		vReceiveVdipString();	
	}
	// mi sincronizzo in modo da evitare per quanto possibile di incappare in interrupts...
	i=ucCntInterrupt;
	while(i==ucCntInterrupt);

	while(*msg){
		while(ucBusySpiVdip);
		for (i=0;i<500;i++){
			ucByteTxVdip_OldData=*msg;
			while(ucBusySpiVdip);
			// leggo lo status per verificare se comando � stato accettato
			if (ucByteTxVdip_OldData==0)
				break;
			// pausa fra i comandi al vdip...
			vVdip_pause();
		}
		if (i>=500){
			break;
		}
		msg++;
		// pausa fra i comandi al vdip...
		vVdip_pause();
	
	}		
	while(ucBusySpiVdip);
	for (i=0;i<500;i++){
		ucByteTxVdip_OldData=0x0D;
		while(ucBusySpiVdip);
		// leggo lo status per verificare se comando � stato accettato
		if (ucByteTxVdip_OldData==0)
			break;
		// pausa fra i comandi al vdip...
		vVdip_pause();
	}
	//for (uiContaLungaPausa=0;uiContaLungaPausa<500;uiContaLungaPausa++)
	vVdip_pause();
}

#define def_try_optimize_write_nocr_vdip

void vSendVdipString_NOCR(unsigned char  *msg){
unsigned long i;
unsigned int ui_num_char_tx;
unsigned int ui_ok;
	// per vedere se vdip � pronto a rx comandi...
	if (uiVdipHasData()){
		vReceiveVdipString();	
	}
	// mi sincronizzo in modo da evitare per quanto possibile di incappare in interrupts...
	i=ucCntInterrupt;
	while(i==ucCntInterrupt);

	ui_num_char_tx=0;

	while(*msg){
		while(ucBusySpiVdip);
#ifdef def_try_optimize_write_nocr_vdip
        i=0;
        ui_ok=0;
        while(i<25000){
            i++;
			ucByteTxVdip_OldData=*msg;
			while(ucBusySpiVdip);
			// leggo lo status per verificare se comando � stato accettato
			if (ucByteTxVdip_OldData==0){
                ui_ok=1;
				break;
            }
            // wait about 200us
			v_wait_microseconds(200);
        }
		if (!ui_ok){
			break;
		}
#else
		for (i=0;i<500;i++){
			ucByteTxVdip_OldData=*msg;
			while(ucBusySpiVdip);
			// leggo lo status per verificare se comando � stato accettato
			if (ucByteTxVdip_OldData==0)
				break;
			vVdip_5ms_pause();
		}
		if (i>=500){
			break;
		}
#endif    
		ui_num_char_tx++;
		msg++;
		// pausa fra i comandi al vdip...
		vVdip_pause();
	
	}		
	while(ucBusySpiVdip);
	//for (uiContaLungaPausa=0;uiContaLungaPausa<500;uiContaLungaPausa++)
	vVdip_pause();
}


unsigned char ucReadVdipByte(unsigned char *pc){
// lettura di un byte da vdip			
	if (!uiVdipHasData())
		return 0;
	while(ucBusySpiVdip);
	ucByteRxVdip=0;// start lettura dato da vdip
	while(ucBusySpiVdip);
	*pc=ucByteRxVdip;
    return 1;
}

#if 0
	// riceve una stringa binaria composta da almeno uiNumBytes2Receive, seguita dal prompt
	void vReceiveVdipBinaryString(unsigned int uiNumBytes2Receive){
	unsigned int i,j;
		memset(ucStringRx_Vdip,0,sizeof(ucStringRx_Vdip));
	// lettura di un byte da vdip
		for (i=0;i<sizeof(ucStringRx_Vdip)-2;i++){
			for (j=0;j<100;j++){
				if (ucReadVdipByte(&ucStringRx_Vdip[i])){
					break;
				}
				// aspetto 50ms
				vVdip_very_big_pause();
			}
			if (j>=100){
				break;
			}
			if (ucByteTxVdip_OldData)
				break;
			// solo se ho esaurito i bytes binari, controllo se ho ricevuto un carriage return
			if (!uiNumBytes2Receive){
				if (ucStringRx_Vdip[i]==0x0d)
					break;
			}
			else
				uiNumBytes2Receive--;
			
		}	
		ucStringRx_Vdip[i]=0;
	}
#endif

xdata unsigned char uc_num_cr_found_by_receive_vdip;
#define def_try_optimize_receive_string_vdip

void vReceiveVdipString(void){
unsigned int i;
unsigned long j;
	memset(ucStringRx_Vdip,0,sizeof(ucStringRx_Vdip));
	uc_num_cr_found_by_receive_vdip=0;
// lettura di un byte da vdip
	for (i=0;i<sizeof(ucStringRx_Vdip)-2;i++){
#ifdef def_try_optimize_receive_string_vdip
	if (uc_vdip_short_pause){
		for (j=0;j<100;j++){
			if (ucReadVdipByte(&ucStringRx_Vdip[i])){
				break;
			}
			vVdip_pause();
		}
	}
	else{
		#define def_total_time_to_wait_s (5)
		#define def_total_time_to_wait_ms (def_total_time_to_wait_s*1000UL)
		#define def_total_time_to_wait_us (def_total_time_to_wait_ms*1000UL)
		#define def_readbyte_period_us (200)
		#define def_num_loop_to_wait_byte (def_total_time_to_wait_us/def_readbyte_period_us)
		for (j=0;j<def_num_loop_to_wait_byte;j++){
			if (ucReadVdipByte(&ucStringRx_Vdip[i])){
				break;
			}
            // wait about def_readbyte_period_us
			v_wait_microseconds(def_readbyte_period_us);
		}
	}
#else	
		for (j=0;j<100;j++){
			if (ucReadVdipByte(&ucStringRx_Vdip[i])){
				break;
			}
			if (uc_vdip_short_pause){
				vVdip_pause();
			}
			else{
				// aspetto 50ms
				vVdip_very_big_pause();
			}
		}
#endif		
		if (j>=100){
			break;
		}
		if (ucByteTxVdip_OldData)
			break;
		if (ucStringRx_Vdip[i]==0x0d){
			uc_num_cr_found_by_receive_vdip++;
			break;
		}
	}	
	// reset del flag short pause!!!!
	uc_vdip_short_pause=0;
	ucStringRx_Vdip[i]=0;
#ifdef def_enable_vdip_diag
    v_refresh_vdip_status_video(1,ucStringRx_Vdip);
#endif

}
typedef enum{
	enumStatoVdip_Idle=0,
	enumStatoVdip_Init,
	enumStatoVdip_DoReset,
	enumStatoVdip_EndOfReset,
	enumStatoVdip_PurgeMsg,
	enumStatoVdip_Toggle_eE,
	enumStatoVdip_FWV,
	enumStatoVdip_IPA,
	enumStatoVdip_endOfInitialization,
	enumStatoVdip_NumOf
}enumStatiVdip;

typedef struct _TipoHandleVdip{
	 enumStatiVdip statoVdip;
	unsigned char ucPrecCntInterrupt;
	unsigned char ucActCntInterrupt;
	unsigned int uiMsElapsed;
	unsigned int uiMsWait;
	unsigned int uiConta;
	unsigned int uiNumRetrySync;
	unsigned int uiNumRetryPurge;
	unsigned char ucTimerInitialzed;
}TipoHandleVdip;

// numero max di tentativi di risincronizzazione con vdip
#define def_max_retry_sync 10

TipoHandleVdip handleVdip;

void vInitVdip(void){
	memset(&handleVdip,0,sizeof(handleVdip));
	handleVdip.statoVdip=enumStatoVdip_Init;

}

typedef struct _TipoVdipInfo{
    char firmware_version_main[32];
    char firmware_version_rprg[32];
    unsigned char uc_iid_info[14][36];
}TipoVdipInfo;
TipoVdipInfo vdip_info;

unsigned char ucHandleVdipStati(void){
	xdata unsigned int i;
	if (handleVdip.uiMsWait){
		if (!handleVdip.ucTimerInitialzed){
			handleVdip.ucTimerInitialzed=1;
			handleVdip.ucPrecCntInterrupt=ucCntInterrupt;
			handleVdip.uiMsElapsed=0;
			return 0;
		}
		else{
			handleVdip.ucActCntInterrupt=ucCntInterrupt;
			if (handleVdip.ucActCntInterrupt>=handleVdip.ucPrecCntInterrupt){
				handleVdip.uiMsElapsed+=(handleVdip.ucActCntInterrupt-handleVdip.ucPrecCntInterrupt)*PeriodoIntMs;
			}
			else{
				handleVdip.uiMsElapsed+=((256U+handleVdip.ucActCntInterrupt)-handleVdip.ucPrecCntInterrupt)*PeriodoIntMs;
			}
			handleVdip.ucPrecCntInterrupt=handleVdip.ucActCntInterrupt;
			if (handleVdip.uiMsElapsed<handleVdip.uiMsWait){
				return 0;
			}
			handleVdip.uiMsWait=0;
			handleVdip.ucTimerInitialzed=0;
		}
		
	}
	switch(handleVdip.statoVdip){
		case enumStatoVdip_Idle:
		default:
		{
			break;
		}
		case enumStatoVdip_Init:
			handleVdip.statoVdip=enumStatoVdip_DoReset;
			handleVdip.uiMsWait=0;
#ifdef def_enable_vdip_diag
            ui_enable_vdip_diagnostic_messages=1;
#endif        
			break;
		case enumStatoVdip_DoReset:
			ucReset_Low_Vdip=0;
			ucDataReq_Low_Vdip=1;
			handleVdip.statoVdip=enumStatoVdip_EndOfReset;
			handleVdip.uiMsWait=20;
			break;
		case enumStatoVdip_EndOfReset:
			ucReset_Low_Vdip=1;
			ucDataReq_Low_Vdip=1;
			handleVdip.uiMsWait=20;
			handleVdip.uiConta=0;
			handleVdip.statoVdip=enumStatoVdip_PurgeMsg;
			break;
		case enumStatoVdip_PurgeMsg:
			uc_vdip_short_pause=1;
			vReceiveVdipString();
			// se ricevo qlcs, purgo
			if (ucStringRx_Vdip[0]&&++handleVdip.uiConta<20){
				memset(ucStringRx_Vdip,0,sizeof(ucStringRx_Vdip));
				handleVdip.uiMsWait=100;
				handleVdip.uiNumRetryPurge=0;
			}
			// se non ricevo nulla, incremento contatore timeout
			// se supero timeout, passa a toggle Ee
			else if (++handleVdip.uiNumRetryPurge>=3){
				handleVdip.uiConta=0;
				handleVdip.uiNumRetryPurge=0;
				handleVdip.statoVdip=enumStatoVdip_Toggle_eE;
				handleVdip.uiMsWait=0;
			}
			// altrimenti aspetto 300ms
			else {
				handleVdip.uiMsWait=300;
			}
			break;
		case enumStatoVdip_Toggle_eE:
			if (++handleVdip.uiConta>=10){
				handleVdip.uiConta=0;
				handleVdip.statoVdip=enumStatoVdip_FWV;
				handleVdip.uiMsWait=0;
			}
			else{
				// chiedo info sul drive inserito
				// ma solo dopo che sono quasi sicuramente sincronizzato
				if (handleVdip.uiConta==5){
					vSendVdipString("IDD");
				}
				else if (handleVdip.uiConta&1)
					vSendVdipString("E");
				else
					vSendVdipString("e");
				if (handleVdip.uiConta==5){
					memset(&vdip_info.uc_iid_info[0][0],0,sizeof(vdip_info.uc_iid_info));
                    i=0;
					while(i<sizeof(vdip_info.uc_iid_info)/sizeof(vdip_info.uc_iid_info[0])){
						vReceiveVdipString();
						strncpy(&vdip_info.uc_iid_info[i][0],ucStringRx_Vdip,sizeof(vdip_info.uc_iid_info[0])-1);
                        i++;
					}
					// se ricevo una stringa sbagliata, ricomincio!
					// alla posizione 1 devo trovare "USB VID"
					// inoltre alla posizione 8 il numero di bytes per settore deve essere 512 cio� 0x200
					if (   strncmp(vdip_info.uc_iid_info[1],"USB VID",7)
						||(atoi(&vdip_info.uc_iid_info[8][17])!=200)
						){
						if (handleVdip.uiNumRetrySync<def_max_retry_sync){
							handleVdip.uiNumRetrySync++;
							handleVdip.statoVdip=enumStatoVdip_PurgeMsg;
							break;
						}
						// se troppi tentativi di sincronismo, errore usb disk!!!
						else{
							alarms|=ALR_USB_DISK;
#ifdef def_enable_vdip_diag
            ui_enable_vdip_diagnostic_messages=0;
#endif        
						}
					}
				}
				else{
					vReceiveVdipString();
				
					if (handleVdip.uiConta&1){
						if (ucStringRx_Vdip[0]!='E'){
							if (handleVdip.uiNumRetrySync<def_max_retry_sync){
								handleVdip.uiNumRetrySync++;
								handleVdip.statoVdip=enumStatoVdip_PurgeMsg;
								break;
							}
							// se troppi tentativi di sincronismo, errore usb disk!!!
							else{
								alarms|=ALR_USB_DISK;
							}
						}
					}
					else {
						if (ucStringRx_Vdip[0]!='e'){
							if (handleVdip.uiNumRetrySync<def_max_retry_sync){
								handleVdip.uiNumRetrySync++;
								handleVdip.statoVdip=enumStatoVdip_PurgeMsg;
								break;
							}
							// se troppi tentativi di sincronismo, errore usb disk!!!
							else{
								alarms|=ALR_USB_DISK;
#ifdef def_enable_vdip_diag
            ui_enable_vdip_diagnostic_messages=0;
#endif        
							}
						}
					}
					handleVdip.uiMsWait=100;
				}
			}
			break;
		case enumStatoVdip_FWV:
			vSendVdipString("FWV");
			vReceiveVdipString();
			vReceiveVdipString();
            // save vdip information... that is firmware version or so
            strncpy(&vdip_info.firmware_version_main[0],ucStringRx_Vdip,sizeof(vdip_info.firmware_version_main)-1);
			vReceiveVdipString();
            strncpy(&vdip_info.firmware_version_rprg[0],ucStringRx_Vdip,sizeof(vdip_info.firmware_version_rprg)-1);
			vReceiveVdipString();
			handleVdip.statoVdip=enumStatoVdip_IPA;
			handleVdip.uiMsWait=0;
			break;
		case enumStatoVdip_IPA:
			// passo in modo ascii
			vSendVdipString("IPA");
			vReceiveVdipString();
			handleVdip.statoVdip=enumStatoVdip_endOfInitialization;
			handleVdip.uiMsWait=100;
			break;
		case enumStatoVdip_endOfInitialization:
#ifdef def_enable_vdip_diag
            ui_enable_vdip_diagnostic_messages=0;
#endif        
            
			return 1;
	}
	return 0;
}

#if 0
	// programma di test comunicazione con modulo vdip
	void main_vdip(void){
	xdata unsigned char i;
	xdata unsigned int j;
	xdata unsigned long iNumRead,iNumErr;

		ucReset_Low_Vdip=0;
		ucDataReq_Low_Vdip=1;
		vVdip_pause();
		ucReset_Low_Vdip=1;
		 
		ucDataReq_Low_Vdip=1;
		vVdip_pause();
		
		
		iNumRead=0;
		iNumErr=0;
		
		
	// attendo un bel po' per permettere che si stabilizzi il tutto
		j=0;
		while(j++<20){
			iNumRead=0;
			while(++iNumRead<10000);
		}
		
		j=0;
		while(j++<10){
			vReceiveVdipString();
			memset(ucStringRx_Vdip,0,sizeof(ucStringRx_Vdip));
		}
		
		
		
		for (i=0;i<15;i++){
			iNumRead++;
			vSendVdipString("E");
			vReceiveVdipString();
			if (ucStringRx_Vdip[0]!='E'){
				iNumErr++;
			}
			vSendVdipString("e");
			vReceiveVdipString();
			if (ucStringRx_Vdip[0]!='e'){
				iNumErr++;
			}

		}		
	// provo il comando FWV...
		for (i=0;i<15;i++){
			iNumRead++;
			memset(ucStringRx_Vdip,0,sizeof(ucStringRx_Vdip));
		   vSendVdipString("FWV");
    		vReceiveVdipString();
    		if (   (ucStringRx_Vdip[1]!='M')
				  ||(ucStringRx_Vdip[2]!='A')    	
				  ||(ucStringRx_Vdip[3]!='I')    	
				  ||(ucStringRx_Vdip[4]!='N')    	
				)  
				iNumErr++;

		}	
		
		
		// passo in modo ascii
		vSendVdipString("IPA");
		vReceiveVdipString();
		
	#if 0	

		vSendVdipString("DIR DELME.TXT");
		vReceiveVdipString();
		
	// chiudo il file aghi.txt
		vSendVdipString("DLF DELME.TXT");
		vReceiveVdipString();

	// apro in scrittura il file delme.txt
		vSendVdipString("OPW DELME.TXT");
		vReceiveVdipString();
		
	// SCRIVO 20 bytes	
		vSendVdipString("WRF 20");
		vSendVdipString("VENTI BYTES CON CR.");
		vReceiveVdipString();
	// SCRIVO 20 bytes	
		vSendVdipString("WRF 10");
		vSendVdipString("DIECI BYT");
		vReceiveVdipString();
		
	// chiudo il file aghi.txt
		vSendVdipString("CLF DELME.TXT");
		vReceiveVdipString();

		vSendVdipString("DIR DELME.TXT");
		vReceiveVdipString();
		
		
	// apro in lettura il file DELME.txt
		vSendVdipString("OPR DELME.TXT");
		vReceiveVdipString();
		
	// leggo 60 bytes	
		vSendVdipString("RDF 30");
		vReceiveVdipString();
	// chiudo il file DELME.txt
		vSendVdipString("CLF DELME.TXT");
		vReceiveVdipString();
		

				vSendVdipString("OPW COEFF.TXT");
				vReceiveVdipString();
				vSendVdipString("WRF 6");
				vSendVdipString("COEF ");
				vReceiveVdipString();
		for (i=0;i<100;i++)			{
				vSendVdipString("WRF 12");
				vSendVdipString("dodici cha\n");
				vReceiveVdipString();
		}			
				

		vSendVdipString("CLF COEFF.TXT");
		vReceiveVdipString();
		
		
		vSendVdipString("FWV");
		vReceiveVdipString();
		
	#endif
	}
#endif
