
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
#include "i2c_master.h"
#include "vdip.h"
#include "rtcDS3231.h"
#include "file_list.h"
#include "az4186_const.h"
#include "az4186_program_codes.h"
#include "az4186_commesse.h"
#include "az4186_spiralatrice.h"
#include "az4186_initHw.h"



static void v_init_handling_encoder_grinding(void){
	vDisableHRE_potentiometer();
	ucLinkEncoder=enumSpeedRegulation_Spindle;
	// versione 1.33 inizializzo il valore encoder!!
	// altrimenti rimango disallineato
	vSetHRE_potentiometer(enumSpeedRegulation_Spindle,0);
	vSetHRE_potentiometer(enumSpeedRegulation_Wheel1,FS_ADC/2);
	vSetHRE_potentiometer(enumSpeedRegulation_Wheel2,FS_ADC/2);
	vSelectHRE_potentiometer(ucLinkEncoder);
}
void v_reset_spindle_and_wheel_dac(void){
	/* Calcolo in spiralatrice.DacOutValue[0] la velocit� angolare del motore. */
	spiralatrice.DacOutValue[0]=0;
	/* Fisso il valore massimo degli rpm del mandrino. */
	spiralatrice.DacOutMaxValue[0]=macParms.rpm_mandrino;
	/* La coppia del mandrino non va controllata. */
	spiralatrice.DacOutValue[1]=1;
	spiralatrice.DacOutMaxValue[1]=1;
	SetDacValue(addrDacMandrino);
	/* Fisso il valore massimo degli rpm della ruota. */
	spiralatrice.DacOutMaxValue[0]=macParms.rpm_frizioni[0];
	SetDacValue(addrDacFrizione1+0);
	/* Fisso il valore massimo degli rpm della ruota. */
	spiralatrice.DacOutMaxValue[0]=macParms.rpm_frizioni[1];
	SetDacValue(addrDacFrizione1+1);
}					     


// rettifica mandrino
unsigned char ucHW_spindle_grinding(void){

	#define defHWSG_button_Title_row 6
	#define defHWSG_button_Title_col ((defLcdWidthX_pixel-10*16)/2)

	#define defDistanceRowBetweenPuls_HWSG 64

	#define defHWSG_button_RunStop_row (defHWSG_button_Title_row+36)
	#define defHWSG_button_RunStop_col (defLcdWidthX_pixel/2-4*16)

	#define defHWSG_button_SpindleSpeed_row (defHWSG_button_RunStop_row+defDistanceRowBetweenPuls_HWSG)
	#define defHWSG_button_SpindleSpeed_col (defLcdWidthX_pixel/4-32)

	#define defHWSG_button_Help_row (defLcdWidthY_pixel-24-36)
	#define defHWSG_button_Help_col (defLcdWidthX_pixel/2-4*16)


	#define defHWSG_button_Esc_row (defLcdWidthY_pixel-24)
	#define defHWSG_button_Esc_col (defLcdWidthX_pixel-4*16-8)


	#define defWinDataOraModifyNumCampo 0
// stato run/stop	
	typedef enum{
		enum_grind_status_stop=0,
		enum_grind_status_run,
		enum_grind_status_numof
	}enum_grind_status;
	
// handle
	typedef struct _type_handle_grinding{
		enum_grind_status status;
		unsigned int ui_spindle_speed_perc;
	}type_handle_grinding;
	
	static type_handle_grinding handle_grinding;
	
	#define defHWSG_Password 0


	// pulsanti della finestra
	typedef enum {
			enum_HWSG_Title=0,
			enum_HWSG_RunStop,
			enum_HWSG_SpindleSpeed,
			enum_HWSG_Help,

			enum_HWSG_Sx,
			enum_HWSG_Up,
			enum_HWSG_Cut,
			enum_HWSG_Esc,
			enum_HWSG_Dn,
			enum_HWSG_Dx,

			enum_HWSG_NumOfButtons
		}enum_HWSG_Buttons;
    
	xdata unsigned char i;


	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
		{
			int idxParametro;
			idxParametro=defGetWinParam(defHWSG_Password);
			// visualizzata finestra password non corretta
			if (idxParametro==201){
				sLCD.ucPasswordChecked=0;
				// chiamo il main menu...
				vJumpToWindow(enumWinId_MainMenu);
			}
			// check password???
			else if (idxParametro==200){
				if (ucVerifyPasswordCorrect(hw.ucStringNumKeypad_out)){
					sLCD.ucPasswordChecked=1;
					vSetStatusCurrentWindow(enumWindowStatus_Active);
					defSetWinParam(defHWSG_Password,0);
					return 2;
				}
				// visualizzo la finestra: password non corretta
				// numero msg da visualizzare: 1
				ucSetWindowParam(enumWinId_Info,0, 1);
				// messaggio da visualizzare: password bad
				ucSetWindowParam(enumWinId_Info,1, enumStr20_bad_password);
				// messaggio: password KO
				defSetWinParam(defHWSG_Password,201);
				    // chiamo la finestra info
				ucCallWindow(enumWinId_Info);
				return 2;
			}
			defSetWinParam(defHWSG_Password,0);
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		}
		case enumWindowStatus_Initialize:
			i=0;
			while (i<enum_HWSG_NumOfButtons){
				ucCreateTheButton(i);
				i++;
			}
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			memset(&handle_grinding,0,sizeof(handle_grinding));
			// cannot start production here!
			ucEnableStartLavorazione(0);
			// init encoder handling
			v_init_handling_encoder_grinding();
			// ogni 200ms rinfresco la pagina...
			ucLcdRefreshTimeoutEnable(200);
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
			i=0;
			while (i<enum_HWSG_NumOfButtons){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						// no taglio asincrono
						case enum_HWSG_Cut:
						    // non occorre rinfrescare la finestra
							break;
					
						case enum_HWSG_Esc:
						{
							// only if in stop mode I can exit from the menu
							if (handle_grinding.status==enum_grind_status_stop){
								// reset the spindle wheel rotation
								ui_zeroOK_reset_rotation_wheel_inversion();
								// OK we can resume production
								ucEnableStartLavorazione(1);
								ucLcdRefreshTimeoutEnable(0);
								ucReturnFromWindow();
								return 2;
							}
							break;
						}
						default:
						{
						    break;
						}
					}
				}
				i++;
			}
			// handle status change
			switch (handle_grinding.status){
				case enum_grind_status_stop:
				default:
				{
					if (    ucDigitalInput_AtHiLevel(enumInputEdge_start)
					     && !ucDigitalInput_AtHiLevel(enumInputEdge_stop)
					     ){
						// run!
						handle_grinding.status=enum_grind_status_run;
						// TODO check return code...
						ui_zeroOK_set_rotation_wheel_inversion();
						v_init_handling_encoder_grinding();
						v_reset_spindle_and_wheel_dac();
						// activate digital output spindle 
						outDigVariable|=ODG_MOTORE;
						break;
					}
					break;
				}
				case enum_grind_status_run:
				{		
					if (    ucDigitalInput_PositiveEdge(enumInputEdge_stop)
					     || ucDigitalInput_AtHiLevel(enumInputEdge_stop)
					     ){	
						// stop!
						handle_grinding.status=enum_grind_status_stop;
						// reset digital output spindle 
						outDigVariable&=~ODG_MOTORE;						
						// TODO check return code...
						ui_zeroOK_reset_rotation_wheel_inversion();
						// reset spindle and wheel DAC
						v_reset_spindle_and_wheel_dac();
						break;
					}
					break;
					
				}				
			}
			{
				
				unsigned int ui_back_color;
				ui_back_color=defLCD_Color_Green;
				if (handle_grinding.status==enum_grind_status_run){
					ui_back_color=defLCD_Color_Yellow;
				}
				
				vStringLangCopy(hw.ucString,enumStr20_spindle_grinding);
				ucPrintTitleButton(hw.ucString,defHWSG_button_Title_row ,defHWSG_button_Title_col,enumFontMedium,enum_HWSG_Title,defLCD_Color_Trasparente,1);
				if (handle_grinding.status==enum_grind_status_run){
					
					if (!spiralatrice.StatusRampaDac){
						/* Calcolo in spiralatrice.DacOutValue[0] la velocit� angolare del motore. */
						spiralatrice.DacOutValue[0]=iGetHRE_setting(enumSpeedRegulation_Spindle);
						/* Fisso il valore massimo degli rpm del mandrino. */
						spiralatrice.DacOutMaxValue[0]=macParms.rpm_mandrino;
						handle_grinding.ui_spindle_speed_perc=(100*spiralatrice.DacOutValue[0])/spiralatrice.DacOutMaxValue[0];
						/* La coppia del mandrino non va controllata. */
						spiralatrice.DacOutValue[1]=1;
						spiralatrice.DacOutMaxValue[1]=1;
						SetDacValue(addrDacMandrino);
						
						/* Calcolo in spiralatrice.DacOutValue[0] la velocit� angolare della ruota i. */
						spiralatrice.DacOutValue[0]=(macParms.rpm_frizioni[0]*handle_grinding.ui_spindle_speed_perc)/100;
						/* Fisso il valore massimo degli rpm della ruota. */
						spiralatrice.DacOutMaxValue[0]=macParms.rpm_frizioni[0];
						/* La coppia della ruota non va controllata. */
						spiralatrice.DacOutValue[1]=1;
						spiralatrice.DacOutMaxValue[1]=1;

						SetDacValue(addrDacFrizione1+0);
						
					}
					
					vStringLangCopy(hw.ucString,enumStr20_spindle_grinding_run);
					ucPrintTitleButton(hw.ucString,defHWSG_button_RunStop_row ,defHWSG_button_RunStop_col,enumFontBig,enum_HWSG_RunStop,defLCD_Color_Red,1);
				}
				else{
					handle_grinding.ui_spindle_speed_perc=0;
					vStringLangCopy(hw.ucString,enumStr20_spindle_grinding_stop);
					ucPrintTitleButton(hw.ucString,defHWSG_button_RunStop_row ,defHWSG_button_RunStop_col,enumFontBig,enum_HWSG_RunStop,defLCD_Color_Green,1);
				}
				{
					unsigned int ui_numcharperc;
					enumFontType the_font;
					the_font=enumFontBig;
					if (handle_grinding.ui_spindle_speed_perc<10){
						ui_numcharperc=2;
					}
					else if (handle_grinding.ui_spindle_speed_perc<100){
						ui_numcharperc=3;
					}
					else{
						ui_numcharperc=4;
					}
					sprintf(hw.ucString,"%3i%%",handle_grinding.ui_spindle_speed_perc);
					{
						unsigned int ui_nc;
						ui_nc=defLcdWidthX_pixel/2-(ui_numcharperc*ucFontSizePixelXY[the_font][0])/2;
						ucPrintTitleButton(hw.ucString,defHWSG_button_SpindleSpeed_row ,ui_nc,the_font,enum_HWSG_SpindleSpeed,ui_back_color,0);
					}
				}
				if (handle_grinding.status==enum_grind_status_run){
					vStringLangCopy(hw.ucString,enumStr20_spindle_grinding_run_help);
				}
				else{
					vStringLangCopy(hw.ucString,enumStr20_spindle_grinding_stop_help);
				}
				ucPrintTitleButton(hw.ucString,defHWSG_button_Help_row ,defHWSG_button_Help_col,enumFontMedium,enum_HWSG_Help,ui_back_color,1);
			}
			// routine che permette di aggiungere ad una finestra i bottoni standard sx, dx, up, dn, esc, cut
			vAddStandardButtons(enum_HWSG_Sx);


			return 1;
	}//switch(uiGetStatusCurrentWindow())
}//unsigned char ucHW_spindle_grinding(void)



// serial number del box..
unsigned long xdata ulBoxSerialNumber;
unsigned char ucSuperUser_passwordChecked=0;
xdata unsigned char ucMachineParsStatiGranted;
extern void vVerifyInsertCodiceProdotto(xdata char *s);
// struttura che serve per poter usare la stessa routine per salvare su usb e su ethernet
xdata TipoStructSavePars_StatusInfo spsi;



unsigned char uc_format_str_len_mult_16(unsigned char xdata *src,unsigned int ui_max_len){
	unsigned char xdata *puc;
	unsigned int ui_len,ui_margin,ui_required,ui;
	// lunghezza stringa in ingresso
	ui_len=strlen(src);
	// se stringa gi� con lunghezza corretta, tutto ok
	if ((ui_len&0x0F)==0)
		return 0;
	// se stringa troppo corta, errore
	if (ui_len<2)
		return 0;
	// calcolo numero di caratteri di margine per allineamento lunghezza
	if (ui_max_len-1<ui_len)
		ui_margin=0;
	else
		ui_margin=ui_max_len-1-ui_len;
	// calcolo numero di caratteri richiesti per allineare lunghezza a multiplo di 16
	ui_required=0;
	ui=ui_len;
	while(ui&0x0F){
		ui++;
		ui_required++;
	}
	// se non c'� spazio, errore
	if (ui_required>ui_margin)
		return 0;
	// punto al acarattere '\r' che chiude la stringa
	puc=src+ui_len-2;
	// se non trovo \r\n, errore!
	if ((puc[0]!='\r')||(puc[1]!='\n'))
		return 0;
	// aggiungo spazi per allineare
	ui=ui_required;
	while(ui--){
		*puc++=' ';
	}
	*puc++='\r';
	*puc++='\n';
	*puc=0;
	// restituisco il numero di caratteri richiesti...
	return ui_required;
}




// dimensione del buffer di lettura da vdip, per sveltire le operazioni di RDF
#define def_num_bytes_to_read_from_vdip 32
// margine per il prompt... quando la rdf richiede un byte, ne vengono restituiti in realt� cinque in pi� perch� viene appeso D:\><cr>
#define def_margin_prompt_bytes_to_read_from_vdip 5
// struttura per gestire bufferizzazione lettura dati da vdip
typedef struct _TipoStructReadBytesFromVdip{
	unsigned char uc_buffer_bytes_from_vdip[def_num_bytes_to_read_from_vdip+def_margin_prompt_bytes_to_read_from_vdip+2];
	unsigned char uc_num_bytes_inbuffer;
	unsigned char uc_idx_next_byte_to_read;
}TipoStructReadBytesFromVdip;
// variabile che gestisce bufferizzazione lettura dati da vdip
xdata TipoStructReadBytesFromVdip read_vdip_bytes;

// byte che memorizza il prossimo byte nel buffer dei dati ricevuto da vdip con una operazione RDF
xdata unsigned char uc_the_byte_read_from_vdip;

// inizializzazione lettura dati da vdip
void v_init_read_bytes_from_vdip(void){
	memset(&read_vdip_bytes,0,sizeof(read_vdip_bytes));
}

// lettura di un byte da vdip, gestione bufferizzata
unsigned char uc_read_a_byte_from_vdip(unsigned int ui_max_bytes_to_read){
	xdata unsigned char uc_buffer_command_vdip[32];
	xdata unsigned char uc_num_bytes_to_read;
	xdata unsigned char uc_num_bytes_read,i;
	xdata unsigned char uc_byte_read;
	extern unsigned char uc_num_cr_found_by_receive_vdip;
	// se mi viene richiesto di leggere max 0 bytes, restituisco newline...
	if (!ui_max_bytes_to_read)
		return '\n';
	// se il mio buffer attualmente non contiene caratteri...
	if (read_vdip_bytes.uc_num_bytes_inbuffer==0){
		// reset buffer
		memset(&read_vdip_bytes.uc_buffer_bytes_from_vdip[0],0,sizeof(read_vdip_bytes.uc_buffer_bytes_from_vdip));
		// tento di leggere def_num_bytes_to_read_from_vdip bytes al colpo
		uc_num_bytes_to_read=def_num_bytes_to_read_from_vdip;
		// se ne posso leggere meno di quanto potrei, clippo il numero di bytes da leggere
		if (ui_max_bytes_to_read<uc_num_bytes_to_read){
			uc_num_bytes_to_read=ui_max_bytes_to_read;
		}
		// preparo il comando da spedire al vdip: RDF <numero bytes>
		sprintf(uc_buffer_command_vdip,"RDF %i",(int)uc_num_bytes_to_read);
		// invio il comando al vdip
		// questo comando restituir� il numero di bytes richiesti, eventualmente con un pad
		// per cui � importante che tutti i bytes vengano estratti dal vdip con la receive!
		vSendVdipString(uc_buffer_command_vdip);
		// reset numero di bytes correntemente letti
		uc_num_bytes_read=0;
		// incremento il numero di bytes attesi in ricezione della quantit� dovuta al prompt finale... D:\><cr>
		uc_num_bytes_to_read+=def_margin_prompt_bytes_to_read_from_vdip;
		// loop riempimento buffer
		while(uc_num_bytes_read<uc_num_bytes_to_read){
			// leggo la stringa ricevuta
			vReceiveVdipString();
			// devo tener conto anche del fatto che la routine di ricezione si "mangia" i carriage return!!!!
			if (uc_num_bytes_to_read>=uc_num_cr_found_by_receive_vdip)
				uc_num_bytes_to_read-=uc_num_cr_found_by_receive_vdip;
			else{
				break;
			}

			// copio nel mio buffer i bytes ricevuti
			// reset source index
			i=0;
			// il loop va fatto max fino a esaurimento caratteri richiesti...
			while(uc_num_bytes_read<uc_num_bytes_to_read){
				// salvo il byte
				read_vdip_bytes.uc_buffer_bytes_from_vdip[uc_num_bytes_read]=ucStringRx_Vdip[i];
				// se stringa corrente finita, esco dal loop di trasferimento
				// e passo a eseguire un'altra receive
				if (ucStringRx_Vdip[i]==0){
					break;
				}
				// increment source index
				i++;
				// increment destination index
				uc_num_bytes_read++;
			}//while(uc_num_bytes_read<uc_num_bytes_to_read){
		}//while(uc_num_bytes_read<uc_num_bytes_to_read){
		// adesso devo eliminare il prompt finale... lo cerco all'indietro
		for (i=1;i<10;i++){
			if (i>uc_num_bytes_read)
				break;
			if(strcmp(&read_vdip_bytes.uc_buffer_bytes_from_vdip[uc_num_bytes_read-i],"D:\\>")==0){
				uc_num_bytes_read-=i;
				read_vdip_bytes.uc_buffer_bytes_from_vdip[uc_num_bytes_read]=0;
				break;
			}
		}
		// reinit indexes
		read_vdip_bytes.uc_num_bytes_inbuffer=uc_num_bytes_read;
		read_vdip_bytes.uc_idx_next_byte_to_read=0;
	}
	// se ancora ho zero bytes, restituisco newline per indicare fine linea
	if (read_vdip_bytes.uc_num_bytes_inbuffer==0){
		uc_byte_read='\n';
	}
	else{
		// salvo il byte da restituire
		uc_byte_read=read_vdip_bytes.uc_buffer_bytes_from_vdip[read_vdip_bytes.uc_idx_next_byte_to_read];
		// incremento indice estrazione carattere
		read_vdip_bytes.uc_idx_next_byte_to_read++;
		// decremento il numero di caratteri a disposizione
		read_vdip_bytes.uc_num_bytes_inbuffer--;
	}
	// restituisco il byte letto
	return uc_byte_read;
}//unsigned char uc_read_a_byte_from_vdip(unsigned int ui_max_bytes_to_read)


//For a file time of 7th June 2007 at 14:24:51 the following calculations are made:
//Year (2007 � 1980 = 27) -->0x1B; Month (6) --> 0x6; Day (7) --> 0x7.
//Hour (14) --> 0xE; Minute (24) --> 0x18; Seconds (51 / 2) --> 0x19.
//File time = (0x1B << 25) | (0x6 << 21) | (7 << 16) | (0xE << 11) | (0x18 << 5) | (0x19)
//In binary (spaces between fields) = 0011011 0110 00111 01110 011000 11001b
//Finally, convert binary to hexadecimal = 0x36C77319.
static unsigned char xdata *pucGetDateTimeFtdiFormat(void){
	static unsigned char xdata ucFtdi_DateTime[11];
	unsigned long ulAux,ulAux2;
	// leggo date and time
	vReadTimeBytesRTC_DS3231();
	// compongo i 32 bit della  codifica
	ulAux2=(rtc.ucAnno);
	ulAux2+=(2000-1980);
	ulAux=(ulAux2&0x7F)<<25;

	ulAux2=rtc.ucMese;
	ulAux|=(ulAux2&0xF)<<21;

	ulAux2=rtc.ucGiorno;
	ulAux|=(ulAux2&0x1F)<<16;

	ulAux2=rtc.ucOre;
	ulAux|=(ulAux2&0x1F)<<11;

	ulAux2=rtc.ucMinuti;
	ulAux|=(ulAux2&0x3F)<<5;

// si deve scrivere il numero di secondi/2, non il numero di secondi!!!!
	ulAux2=rtc.ucSecondi/2;
	ulAux|=(ulAux2&0x1F);
	// preparo la stringa
	sprintf(ucFtdi_DateTime,"0x%08lX",ulAux);
	// restituisco la stringa
	return ucFtdi_DateTime;
}





extern unsigned char ucVerifyPasswordCorrect(unsigned char xdata *pnewpassword);
typedef enum{
	enumFP_unsigned_char=0,
	enumFP_unsigned_int,
	enumFP_signed_int,
	enumFP_unsigned_short,
	enumFP_signed_short,
	enumFP_float,
	enumFP_unsigned_long,
	enumFP_signed_long,
	enumFP_string,
	enumFP_unsigned_start_struct,
	enumFP_unsigned_end_struct,
	enumFP_start_programs,
	enumFP_label,
	enumFP_end_programs,
	enumFP_date_and_time,
	enumFP_end_file,
	enumFP_NumOf
}enumFormatiParametro;

typedef struct _TipoInfoFormatoParsMac{
	enumFormatiParametro fp;
	unsigned char ucNumOf;
	unsigned char ucName[20+1];
	unsigned char xdata *p;
}TipoInfoFormatoParsMac;

PROGRAMMA auxPrg_params;


static code unsigned char ucLivingSaveParamsChars[]={'-','\\','|','/'};


code TipoInfoFormatoParsMac formatoParMac[]={
	{enumFP_date_and_time,1,"[Date]",NULL},
	{enumFP_label,	1,			"[Params]",NULL},
	{enumFP_unsigned_long,	1,			"serialn",(unsigned char xdata *)&ulBoxSerialNumber},
	{enumFP_unsigned_int,	1,			"version",(unsigned char xdata *)&macParms.uiMacParmsFormatVersion},
	{enumFP_unsigned_char,	1,			"crc",(unsigned char xdata *)&macParms.ucCrc},
	{enumFP_unsigned_short,	1,			"impGiro",(unsigned char xdata *)&macParms.impGiro},
	{enumFP_float,			1,			"mmGiro",(unsigned char xdata *)&macParms.mmGiro},
	{enumFP_unsigned_int,	NUM_FRIZIONI,"rpmFriz",(unsigned char xdata *)&macParms.rpm_frizioni[0]},
	{enumFP_unsigned_int,	1,			"rpmMandr",(unsigned char xdata *)&macParms.rpm_mandrino},
	{enumFP_unsigned_short,	1,			"lenSemi",(unsigned char xdata *)&macParms.lenSemigiro},
	{enumFP_unsigned_char,	1,			"modo",(unsigned char xdata *)&macParms.modo},
	{enumFP_unsigned_char,	1,			"numfili",(unsigned char xdata *)&macParms.ucMaxNumFili},
	{enumFP_unsigned_char,	1,			"numPzAvv",(unsigned char xdata *)&macParms.nPezziAvviso},
	{enumFP_unsigned_char,	1,			"nSecLamp",(unsigned char xdata *)&macParms.nSecLampSpegn},
	{enumFP_unsigned_char,	1,			"nSpirPr",(unsigned char xdata *)&macParms.nspire_pret},
	{enumFP_float,	NUM_FRIZIONI,"iAsse",(unsigned char xdata *)&macParms.iasse[0]},
	{enumFP_float,	NUM_FRIZIONI,"dFriz",(unsigned char xdata *)&macParms.diam_frizioni[0]},
	{enumFP_float,	MAXFORNITORI,"Forni",(unsigned char xdata *)&nvram_struct.TabFornitori[0]},

	{enumFP_float,	1,"dCut",(unsigned char xdata *)&macParms.diam_coltello},
	{enumFP_unsigned_char,	1,"nSec",(unsigned char xdata *)&macParms.nsec},
	{enumFP_unsigned_int,	1,"dAll",(unsigned char xdata *)&macParms.durata_all},
	{enumFP_float,	1,"dTest",(unsigned char xdata *)&macParms.DiamTestIntrz},
	{enumFP_unsigned_char,	1,"percRo",(unsigned char xdata *)&macParms.ucPercMaxErrRo},

	{enumFP_unsigned_char,	1,"badSpz",(unsigned char xdata *)&macParms.ucMaxBadSpezzConsec},
	{enumFP_float,	1,"fsAbs",(unsigned char xdata *)&macParms.fFSassorbRuote},

	{enumFP_unsigned_start_struct,NCANALI_TARABILI*NUM_PORTATE_DINAMICHE,"tarature",(unsigned char xdata *)sizeof(macParms.tarParms[0][0])},
	{enumFP_float,	1,"k",(unsigned char xdata *)&macParms.tarParms[0][0].k},
	{enumFP_unsigned_short,	1,"ksh",(unsigned char xdata *)&macParms.tarParms[0][0].ksh},
	{enumFP_float,	1,"o",(unsigned char xdata *)&macParms.tarParms[0][0].o},
	{enumFP_unsigned_short,	1,"osh",(unsigned char xdata *)&macParms.tarParms[0][0].osh},
	{enumFP_float,	1,"p0",(unsigned char xdata *)&macParms.tarParms[0][0].p0},
	{enumFP_float,	1,"p",(unsigned char xdata *)&macParms.tarParms[0][0].p},
	{enumFP_float,	1,"m0",(unsigned char xdata *)&macParms.tarParms[0][0].m0},
	{enumFP_float,	1,"m0step",(unsigned char xdata *)&macParms.tarParms[0][0].m0step},
	{enumFP_float,	1,"m",(unsigned char xdata *)&macParms.tarParms[0][0].m},
	{enumFP_float,	1,"mstep",(unsigned char xdata *)&macParms.tarParms[0][0].mstep},
	{enumFP_unsigned_end_struct,NCANALI_TARABILI*NUM_PORTATE_DINAMICHE,"tarature",(unsigned char xdata *)&macParms.tarParms[0][0].p0},

	{enumFP_signed_long,1,"passw",(unsigned char xdata *)&macParms.password},

	{enumFP_unsigned_char,1,"encoder",(unsigned char xdata *)&macParms.uc_1Encoder_0Potenziometri},
	{enumFP_unsigned_char,1,"auto",(unsigned char xdata *)&macParms.ucEnableCommesseAutomatiche},
	{enumFP_unsigned_char,1,"quintaP",(unsigned char xdata *)&macParms.ucAbilitaQuintaPortata},
	{enumFP_unsigned_char,1,"temp_comp",(unsigned char xdata *)&macParms.ucAbilitaCompensazioneTemperatura},
	{enumFP_unsigned_char,1,"tipoCut",(unsigned char xdata *)&macParms.ucUsaSensoreTaglio},

	{enumFP_unsigned_int,1,"minCutAd",(unsigned char xdata *)&macParms.uiMinPosColtello_StepAD},
	{enumFP_unsigned_int,1,"maxCutAd",(unsigned char xdata *)&macParms.uiMaxPosColtello_StepAD},
	{enumFP_unsigned_int,1,"tolCutAd",(unsigned char xdata *)&macParms.uiTolerancePosColtello_StepAD},
	{enumFP_unsigned_char,1,"slowDac",(unsigned char xdata *)&macParms.ucSlowDacColtello},
	{enumFP_unsigned_char,1,"gainDac",(unsigned char xdata *)&macParms.ucGainDacColtello},

	{enumFP_unsigned_int,1,"hopperTime",(unsigned char xdata *)&macParms.uiDurataAttivazioneBandella_centesimi_secondo},
	{enumFP_unsigned_int,1,"hopperDelay",(unsigned char xdata *)&macParms.uiRitardoAttivazioneBandella_centesimi_secondo},
	{enumFP_unsigned_char,1,"enAlarmLamp",(unsigned char xdata *)&macParms.ucAbilitaLampadaAllarme},

	{enumFP_unsigned_char,1,"spacerType",(unsigned char xdata *)&macParms.uc_distanziatore_type},
	{enumFP_float,	1,"spacerOffset",(unsigned char xdata *)&macParms.fOffsetDistanziatore_ColtelloMm},
	{enumFP_unsigned_short,	1,"spacerDelay",(unsigned char xdata *)&macParms.usDelayDistanziaDownMs},
	{enumFP_unsigned_char,1,"grinding",(unsigned char xdata *)&macParms.uc_enable_spindle_grinding},

	{enumFP_label,	1,			"[End params]",NULL},

	{enumFP_label,	1,			"[Lcd cal]",NULL},
	{enumFP_unsigned_int,1,"lcdCalV",(unsigned char xdata *)&lcdCal.uiCalibrationFormatVersion},
	{enumFP_unsigned_char,1,"lcdCrc",(unsigned char xdata *)&lcdCal.ucCrc},
	{enumFP_signed_int,sizeof(lcdCal.iCalibraLcdRealZone)/sizeof(lcdCal.iCalibraLcdRealZone[0]),"lcdIrz",(unsigned char xdata *)&lcdCal.iCalibraLcdRealZone[0]},
	{enumFP_float,sizeof(lcdCal.fCoeffsCalibraLcd)/sizeof(lcdCal.fCoeffsCalibraLcd[0]),"lcdFcal",(unsigned char xdata *)&lcdCal.fCoeffsCalibraLcd[0]},
	{enumFP_label,	1,			"[End lcd cal]",NULL},

	{enumFP_start_programs,1,"[Program 0]",NULL},
	{enumFP_unsigned_char,1,"versC",(unsigned char xdata *)&auxPrg_params.ucVersionCharacter},
	{enumFP_unsigned_char,1,"versN",(unsigned char xdata *)&auxPrg_params.ucVersionNumber},
	{enumFP_string,defMaxCharNomeCommessa,"cP",(unsigned char xdata *)&auxPrg_params.codicePrg[0]},
	{enumFP_unsigned_int,1,"nextP",(unsigned char xdata *)&auxPrg_params.nextPrg},
	{enumFP_unsigned_int,1,"prevP",(unsigned char xdata *)&auxPrg_params.prevPrg},
	{enumFP_float,1,"speedP",(unsigned char xdata *)&auxPrg_params.vel_produzione},
	{enumFP_unsigned_int,1,"rpmP",(unsigned char xdata *)&auxPrg_params.rpm_mandrino},

	{enumFP_float,NUM_FRIZIONI,"adsP",(unsigned char xdata *)&auxPrg_params.assorb[0]},
	{enumFP_float,1,"pospP",(unsigned char xdata *)&auxPrg_params.pos_pretaglio},
	{enumFP_float,1,"dwireP",(unsigned char xdata *)&auxPrg_params.diametro_filo},
	{enumFP_float,1,"dspindP",(unsigned char xdata *)&auxPrg_params.diametro_mandrino},
	{enumFP_unsigned_char,1,"nwireP",(unsigned char xdata *)&auxPrg_params.num_fili},
	{enumFP_float,1,"coeffP",(unsigned char xdata *)&auxPrg_params.coeff_corr},
	{enumFP_unsigned_int,NUM_FRIZIONI,"dvelW",(unsigned char xdata *)&auxPrg_params.vruote_prg[0]},
	{enumFP_float,NUM_FRIZIONI,"velW",(unsigned char xdata *)&auxPrg_params.vel_frizioni[0]},
	{enumFP_unsigned_char,1,"tollP",(unsigned char xdata *)&auxPrg_params.fornitore},
	{enumFP_unsigned_short,1,"validP",(unsigned char xdata *)&auxPrg_params.usValidProgram_A536},
	{enumFP_unsigned_char,1,"emptyP",(unsigned char xdata *)&auxPrg_params.empty},
	{enumFP_float,1,"tspeedP",(unsigned char xdata *)&auxPrg_params.vel_prod_vera},
// parametri distanziatore...
	{enumFP_unsigned_short	,1							,"knifDy",(unsigned char xdata *)&auxPrg_params.usDelayDistanziaDownMs},	// tempo necessario al coltello per scendere in posizione di distanziazione
	{enumFP_unsigned_short	,	defMaxNumOfPercDistanzia,"spcDn",(unsigned char xdata *)&auxPrg_params.usPercDistanzia_Starts[0]},
	{enumFP_unsigned_short	,	defMaxNumOfPercDistanzia,"spcUp",(unsigned char xdata *)&auxPrg_params.usPercDistanzia_Ends[0]},
   {enumFP_float			,1							,"knifPn",(unsigned char xdata *)&auxPrg_params.f_coltello_pos_spaziatura_mm}, // coltello: Posizione di distanziazione. 
   {enumFP_float			,1							,"knifOf",(unsigned char xdata *)&auxPrg_params.f_coltello_offset_n_spire}, // coltello: numero di spire di cui si comprime la resistenza prima di iniziare a distanziarsi
	{enumFP_unsigned_short	,	defMaxNumOfPercDistanzia,"spcNC",(unsigned char xdata *)&auxPrg_params.usPercDistanzia_NumSpire[0]}, // numero di spire di distanziazione
	// spcTy indica quale tipo di impostazione dei parametri di distanziazione ha effettuato l'utente
	// 1-->utente ha specificato percentuale iniziale e numero di spire (la percentuale finale viene calcolata)
	// 2-->utente ha specificato numero di spire e percentuale finale (la percentuale iniziale viene calcolata)
	// 0, altri (valore di default) -->percentuale iniziale e finale (il numero di spire viene calcolato)
	{enumFP_unsigned_char	,	defMaxNumOfPercDistanzia,"spcTy",(unsigned char xdata *)&auxPrg_params.ucPercDistanzia_Type[0]},     
	// temperature factor [*10-6/�C], eg 0.0041 1/C-->4'100
//	unsigned int ui_temperature_coefficient_per_million;   
	// additive adjustment factor; the resistance is corrected by a factor +/ x %
	// eg this field is +2-->it means that the resistance value is corrected by a +2% 
	// eg this field is -3-->it means that the resistance value is corrected by a -3% 
//	int i_temperature_corrective_factor_percentual;   
	{enumFP_unsigned_int,1,"tempK",(unsigned char xdata *)&auxPrg_params.ui_temperature_coefficient_per_million},
	{enumFP_signed_int  ,1,"tempP",(unsigned char xdata *)&auxPrg_params.i_temperature_corrective_factor_percentual},
	{enumFP_float  ,sizeof(auxPrg_params.f_temperature_compensation_t_celsius)/sizeof(auxPrg_params.f_temperature_compensation_t_celsius[0]),"tempT",(unsigned char *)&auxPrg_params.f_temperature_compensation_t_celsius},
	{enumFP_float  ,sizeof(auxPrg_params.f_temperature_compensation_coeff)/sizeof(auxPrg_params.f_temperature_compensation_coeff[0]),"tempC",(unsigned char *)&auxPrg_params.f_temperature_compensation_coeff},

	{enumFP_end_programs,1,"[end program 0]",NULL},

	{enumFP_end_file,	1,			"[End file]",NULL},

};



typedef enum{
	enumSP_idle=0,
	enumSP_initVdip,
	enumSP_handleVdip,
	enumSP_openFile,
	enumSP_openFile_ctrlErr,
	enumSP_initLoop,
	enumSP_doLoop,
	enumSP_nextLoop,
	enumSP_end,
	enumSP_endOk,
	enumSP_numOf
}enumSaveParams_stato;

typedef struct _TipoHandleSaveParams{
	enumSaveParams_stato stato;
	int i,j,iNumOfPars,iStructSize,exitCode,iNumPrg;
	int iOldClock,iTimeElapsed,iActClock;
	unsigned char iContaStruct,idxStruct,iNumFieldsStruct,ucParValue[100],ucIamInStruct,myuc;
	unsigned char ucIamInPrograms;
	unsigned char xdata *p;
	unsigned char msg2vdip[128];
	unsigned int uiTotalNumOfLoops,uiNumLoop;
	unsigned int uiLenOfFile,idxStartParams,idxEndParams;
	float fEstimatedDonePerc;
	unsigned int idx_param_loop;
}TipoHandleSaveParams;
xdata TipoHandleSaveParams handleSaveParams;

// restituisce il codice di uscita della procedura di salva parametri:
// 0=tutto ok
// 1=error communicating with usb disk
// 2=usb disk command failed
// 3=error opening param file for writing
// 4=error writing on param file
int iSaveMachineParsOnFile_GetExitCode(void){
	return handleSaveParams.exitCode;
}

// ritorna la percentuale di salvataggio correntemente eseguita
float fSaveMachineParsOnFile_GetEstimatedDonePerc(void){
	return handleSaveParams.fEstimatedDonePerc;
}

// inizializzazione della macchina a stati che permette il salvataggio dei parametri della macchina...
void vInitMachineParsOnFile_Stati(void){
	memset(&handleSaveParams,0,sizeof(handleSaveParams));
	handleSaveParams.stato=enumSP_initVdip;
}

// stringa che utilizzo per memorizzare risultati intermedi...
xdata char ucStringAuxParams[128];

#define def_try_vdip_1024_wrf		
#ifdef def_try_vdip_1024_wrf	
//	#warning ******* PROVA OTTIMIZZAZIONE VDIP... *********

	typedef enum{
		enum_send_vdip_string_op_init=0,
		enum_send_vdip_string_op_add,
		enum_send_vdip_string_op_flush,
		enum_send_vdip_string_op_numof
	}enum_send_vdip_string_op;
	#define def_bufvdip_write_size 1024
	#define def_margin_bufvdip_write_size 32
	#define def_margin_for_null_char 2
	typedef struct _TipoStructBufferVdipWrite{
		char buffer[def_bufvdip_write_size+def_margin_bufvdip_write_size+def_margin_for_null_char];
		unsigned int ui_num_char_in_buffer;
	}TipoStructBufferVdipWrite;
	TipoStructBufferVdipWrite vdip_write;

	unsigned int ui_send_vdip_string(enum_send_vdip_string_op op,char *s){
		unsigned int ui_len_str_input;
		unsigned int ui_room_for_chars;
		unsigned int ui_num_char_2_copy;
		unsigned int ui_num_char_residual;
		unsigned int ui_send_to_vdip;
		unsigned int retcode;
	// init	
		retcode=1;
		ui_send_to_vdip=0;
		ui_num_char_2_copy = 0;
		ui_num_char_residual = 0;
		
		switch(op){
			case enum_send_vdip_string_op_init:
				memset(&vdip_write,0,sizeof(vdip_write));
				break;
			case enum_send_vdip_string_op_add:
				ui_len_str_input=strlen(s);
				if (ui_len_str_input>def_bufvdip_write_size){
					retcode=0;
					break;
				}
				if (vdip_write.ui_num_char_in_buffer<=def_bufvdip_write_size)
					ui_room_for_chars=def_bufvdip_write_size-vdip_write.ui_num_char_in_buffer;
				else
					ui_room_for_chars=0;
				// if there is enough room, copy to the buffer and exit ok
				if (ui_len_str_input<=ui_room_for_chars){
					memcpy(&vdip_write.buffer[vdip_write.ui_num_char_in_buffer+def_margin_bufvdip_write_size],s,ui_len_str_input);
					vdip_write.ui_num_char_in_buffer+=ui_len_str_input;
				}
				else{
					// copy the portion that fits into
					ui_num_char_2_copy=ui_room_for_chars;
					ui_num_char_residual=ui_len_str_input-ui_num_char_2_copy;
					memcpy(&vdip_write.buffer[vdip_write.ui_num_char_in_buffer+def_margin_bufvdip_write_size],s,ui_num_char_2_copy);
					vdip_write.ui_num_char_in_buffer=def_bufvdip_write_size;
					ui_send_to_vdip=1;
				}
				break;
			case enum_send_vdip_string_op_flush:
				if (vdip_write.ui_num_char_in_buffer){
					ui_send_to_vdip=1;
				}
				break;			
			default:
				retcode=0;
				break;
		}
		// should I send to vdip the buffer???
		if (ui_send_to_vdip){
			// sprintf the command
			sprintf(vdip_write.buffer,"WRF %i\r",vdip_write.ui_num_char_in_buffer);
			// just to be sure.. force the null char...
			vdip_write.buffer[vdip_write.ui_num_char_in_buffer+def_margin_bufvdip_write_size]=0;
			// now concatenate the buffer, it will be shifted..
			strcat(vdip_write.buffer,&vdip_write.buffer[def_margin_bufvdip_write_size]);
			vSendVdipString_NOCR(vdip_write.buffer);
			vReceiveVdipString();
			// reset the buffer
			memset(&vdip_write,0,sizeof(vdip_write));
			// se non ho ricevuto l'ok, aspetto un po' e poi riverifico se tutto ok o no
			if (ucStringRx_Vdip[0]==0){
				// aspetto ...
				handleSaveParams.iOldClock=ucCntInterrupt;
				handleSaveParams.iTimeElapsed=0;
				// aspetto 10 secondi e riprovo
				while(handleSaveParams.iTimeElapsed<10000){
					handleSaveParams.iActClock=ucCntInterrupt;
					if (handleSaveParams.iActClock>=handleSaveParams.iOldClock){
						handleSaveParams.iTimeElapsed+=(handleSaveParams.iActClock-handleSaveParams.iOldClock)*PeriodoIntMs;
					}
					else{
						handleSaveParams.iTimeElapsed+=((256+handleSaveParams.iActClock)-handleSaveParams.iOldClock)*PeriodoIntMs;
					}
					handleSaveParams.iOldClock=handleSaveParams.iActClock;
				}
				vReceiveVdipString();
			}
			if (ucStringRx_Vdip[0]==0){
				handleSaveParams.exitCode=err_vdip_no_response_on_writing;
				handleSaveParams.stato=enumSP_end;
				return 1;
			}
			else
				// se bad command, errore!!!
				for (handleSaveParams.j=0;ucStringRx_Vdip[handleSaveParams.j];handleSaveParams.j++){
					if (  (strncmp(ucStringRx_Vdip+handleSaveParams.j,"Bad" ,3)==0)
						 ||(strncmp(ucStringRx_Vdip+handleSaveParams.j,"File",4)==0)
						 ||(strncmp(ucStringRx_Vdip+handleSaveParams.j,"Disk",4)==0)
						 ||(strncmp(ucStringRx_Vdip+handleSaveParams.j,"Read",4)==0)
						 ||(strncmp(ucStringRx_Vdip+handleSaveParams.j,"Fail",4)==0)
					   ){
						handleSaveParams.exitCode=err_vdip_diskerr_writing;
						handleSaveParams.stato=enumSP_end;
					}
				}
			
		}
		switch(op){
			case enum_send_vdip_string_op_init:
				break;
			case enum_send_vdip_string_op_add:
				if (ui_send_to_vdip){
					// now copy the remaining portion from the beginning of the buffer
					memcpy(&vdip_write.buffer[def_margin_bufvdip_write_size],s+ui_num_char_2_copy,ui_num_char_residual);
					vdip_write.ui_num_char_in_buffer=ui_num_char_residual;
					ui_send_to_vdip=0;
				}
				break;
			case enum_send_vdip_string_op_flush:
				break;			
			default:
				retcode=0;
				break;
		}
		return retcode;
	}
#endif

static TipoUnionParamWalkReg u_save_pars;
typedef union
{
	unsigned char *puc;
	int i;
}type_union_unsignedchar_int;

// salvataggio di tutti i parametri macchina su file...
// ritorna 0 se gestione finita
// ucMode=0--> salva su usb
// ucMode=1--> salva su ethernet
unsigned char ucSaveMachineParsOnFile_Stati(unsigned char ucMode){
	xdata unsigned char i;
    enum_walk_reg_file_list_retcode walk_reg_file_list_retcode;
    unsigned int ui_continue_loop;


	spsi.op=savePars_return_op_none;
	spsi.pc2save=ucStringAuxParams;

	switch (handleSaveParams.stato){
		default:
			return 0;
		case enumSP_idle:
			return 0;
		case enumSP_initVdip:
			if (ucMode==0){
				uiRequestSuspendAnybusOp(1);
				if (!uiHasBeenAckRequestSuspendAnybusOp())
					break;
			}
			handleSaveParams.uiTotalNumOfLoops=0;
			handleSaveParams.uiNumLoop=0;
			handleSaveParams.fEstimatedDonePerc=0;
            handleSaveParams.i=0;
			while (handleSaveParams.i<sizeof(formatoParMac)/sizeof(formatoParMac[0])){
				if(formatoParMac[handleSaveParams.i].fp==enumFP_start_programs){
					break;
				}
				if (formatoParMac[handleSaveParams.i].fp==enumFP_unsigned_start_struct){
					handleSaveParams.ucIamInStruct=1;
					handleSaveParams.iContaStruct=formatoParMac[handleSaveParams.i].ucNumOf;
					handleSaveParams.iNumFieldsStruct=0;
				}
				else if (formatoParMac[handleSaveParams.i].fp==enumFP_unsigned_end_struct){
					handleSaveParams.ucIamInStruct=0;
					handleSaveParams.uiTotalNumOfLoops+=handleSaveParams.iNumFieldsStruct*handleSaveParams.iContaStruct;
				}
                else{
                    if (formatoParMac[handleSaveParams.i].fp==enumFP_string)
                        handleSaveParams.uiTotalNumOfLoops++;
                    else if (!handleSaveParams.ucIamInStruct)
                        handleSaveParams.uiTotalNumOfLoops+=formatoParMac[handleSaveParams.i].ucNumOf;
                    else
                        handleSaveParams.iNumFieldsStruct+=formatoParMac[handleSaveParams.i].ucNumOf;
                }
                handleSaveParams.i++;
			}
			handleSaveParams.iNumFieldsStruct=0;
			handleSaveParams.i++;
			while (handleSaveParams.i<sizeof(formatoParMac)/sizeof(formatoParMac[0])){
				if(formatoParMac[handleSaveParams.i].fp==enumFP_end_programs){
					break;
				}
				if (formatoParMac[handleSaveParams.i].fp==enumFP_string)
					handleSaveParams.iNumFieldsStruct++;
				else
					handleSaveParams.iNumFieldsStruct+=formatoParMac[handleSaveParams.i].ucNumOf;
                handleSaveParams.i++;
			}
			handleSaveParams.iContaStruct=0;
			// trovo il numero di programmi in lista
            {
            
                u_save_pars.pBody=(char *)(&auxPrg_params);
                // walk init
                walk_reg_file_list_retcode=walk_reg_file_list(   ui_handle_product_codes
                                                                ,enum_walk_reg_file_list_init
                                                                ,u_save_pars
                                                            );
                if (walk_reg_file_list_retcode==enum_walk_reg_file_list_retcode_ok){
                    ui_continue_loop=1;
                    // listing loop
                    while(ui_continue_loop){
                        // walk next
                        walk_reg_file_list_retcode=walk_reg_file_list(   ui_handle_product_codes
                                                                        ,enum_walk_reg_file_list_next
                                                                        ,u_save_pars
                                                                    );   
                        switch(walk_reg_file_list_retcode){
                            case enum_walk_reg_file_list_retcode_ok: 
                                // a valid product code has been found
                                handleSaveParams.iContaStruct++;
                                break;
                            case enum_walk_reg_file_list_retcode_end_of_list:
                            default:
                                // end of valid product codes
                                ui_continue_loop=0;
                                break;
                        }
                    }
                }
//end_of_procedure:;
            }
        
			// incremento il numero di record da salvare...
			handleSaveParams.uiTotalNumOfLoops+=handleSaveParams.iNumFieldsStruct*handleSaveParams.iContaStruct;
			handleSaveParams.exitCode=all_ok_vdip;
			handleSaveParams.i=0;
			if (ucMode==0){
				vInitVdip();
			}
			else{
				spsi.op=savePars_return_op_init;
			}
			handleSaveParams.stato=enumSP_handleVdip;
			break;
		case enumSP_handleVdip:
			if (ucMode==0){
				if (ucHandleVdipStati()){
					handleSaveParams.stato=enumSP_openFile;
				}
			}
			else{
				handleSaveParams.stato=enumSP_openFile;
			}
			break;
		case enumSP_openFile:
			if (ucMode==0){
				// cancello il file coi parametri... se esiste!!!!
				vSendVdipString("DIR az4181P.ini");
				vReceiveVdipString();// elimino la riga vuota
				vReceiveVdipString();// riga coi dati
				// solo se il file esiste, lo cancello
				for (i=0;ucStringRx_Vdip[i];i++){
					if (strncmp(ucStringRx_Vdip+i,"FAILED",6)==0){
						break;
					}
				}
				// se il file esiste, lo cancello per evitare problemi
				// sembra che il vdip non gestisca correttamente un file in scrittura, se il file gi� esiste...
				if (ucStringRx_Vdip[i]==0){
					vReceiveVdipString();// riga col prompt
					vSendVdipString("DLF az4181P.ini");
					vReceiveVdipString();
				}
				else{
					vReceiveVdipString();// riga col prompt
				}
				// adesso creo il file di salvataggio parametri...
				// apri il file spcificando date and time! nel formato ascii 0x12345678
				sprintf(ucStringAuxParams,"OPW az4181P.ini %s",pucGetDateTimeFtdiFormat());
				vSendVdipString(ucStringAuxParams);
				vReceiveVdipString();
				vSendVdipString("SEK 0");
				vReceiveVdipString();
			}
			else{
				spsi.op=savePars_return_op_openfile;
			}
			handleSaveParams.stato=enumSP_openFile_ctrlErr;
			break;
		case enumSP_openFile_ctrlErr:
			if (ucMode==0){
				if (ucStringRx_Vdip[0]==0){
					handleSaveParams.exitCode=err_vdip_openfile_for_writing;
					handleSaveParams.stato=enumSP_end;
					return 1;
				}
				else{
					// se bad command, errore!!!
					for (handleSaveParams.j=0;ucStringRx_Vdip[handleSaveParams.j];handleSaveParams.j++){
						if ( (strncmp(ucStringRx_Vdip+handleSaveParams.j,"File",4)==0)
							||(strncmp(ucStringRx_Vdip+handleSaveParams.j,"Disk",4)==0)
							||(strncmp(ucStringRx_Vdip+handleSaveParams.j,"Read",4)==0)
							||(strncmp(ucStringRx_Vdip+handleSaveParams.j,"Fail",4)==0)
							) {
							handleSaveParams.exitCode=err_vdip_diskerr_on_writing;
							handleSaveParams.stato=enumSP_end;
							return 1;
						}
					}
				}
			}
			handleSaveParams.stato=enumSP_initLoop;
			break;
		case enumSP_initLoop:
			handleSaveParams.ucIamInStruct=0;
			handleSaveParams.ucIamInPrograms=0;
			handleSaveParams.i=0;
			handleSaveParams.stato=enumSP_doLoop;
			handleSaveParams.idx_param_loop=0;
#ifdef def_try_vdip_1024_wrf		
			ui_send_vdip_string(enum_send_vdip_string_op_init,ucStringAuxParams);
#endif			
			break;
		case enumSP_doLoop:
			{
				unsigned int ui_switch_to_next_loop;
				ui_switch_to_next_loop=1;
				// modifica per essere certi di non sforare...
				if (handleSaveParams.i>=sizeof(formatoParMac)/sizeof(formatoParMac[0])){
					handleSaveParams.stato=enumSP_endOk;
					break;
				}
				if (formatoParMac[handleSaveParams.i].fp==enumFP_start_programs){
					handleSaveParams.uiNumLoop++;
					handleSaveParams.ucIamInStruct=0;
					if (!handleSaveParams.ucIamInPrograms){
						handleSaveParams.ucIamInPrograms=1;
						handleSaveParams.idxStruct=handleSaveParams.i;
						u_save_pars.pBody=(char *)(&auxPrg_params);
						// walk init
						walk_reg_file_list_retcode=walk_reg_file_list(   ui_handle_product_codes
																		,enum_walk_reg_file_list_init
																		,u_save_pars
																	);
						// if walk error or we're at end of list, skip until end of program clause or end of list
						if (walk_reg_file_list_retcode!=enum_walk_reg_file_list_retcode_ok){
							handleSaveParams.iNumPrg=0;
						}
						else{
							handleSaveParams.iNumPrg=1;
						}
					
					}
					if (handleSaveParams.iNumPrg){
						handleSaveParams.iNumPrg=0;
						u_save_pars.pBody=(char *)(&auxPrg_params);
						walk_reg_file_list_retcode=walk_reg_file_list(   ui_handle_product_codes
																		,enum_walk_reg_file_list_next
																		,u_save_pars
																	);                   

						if (walk_reg_file_list_retcode==enum_walk_reg_file_list_retcode_ok){
							if (walk_reg_file_list(  ui_handle_product_codes
													,enum_walk_reg_file_list_get_body
													,u_save_pars
												  )
								==enum_walk_reg_file_list_retcode_ok
								){
								handleSaveParams.iNumPrg=ul_reg_file_cur_id(ui_handle_product_codes, enum_reg_file_id_required_last_block_walked);
							}
						}
						// if walk error or we're at end of list, skip until end of program clause or end of list
						else{
							handleSaveParams.iNumPrg=0;
							// look for end of program structure
							while(  (handleSaveParams.i<sizeof(formatoParMac)/sizeof(formatoParMac[0]))
								  &&(formatoParMac[handleSaveParams.i].fp!=enumFP_end_programs)
								 ){
								 handleSaveParams.i++;
							}
							handleSaveParams.stato=enumSP_nextLoop;
							break;
						}
						if (handleSaveParams.iNumPrg){
							sprintf(ucStringAuxParams,"[Program %i]\r\n",(int)handleSaveParams.iNumPrg);
							//uc_format_str_len_mult_16(ucStringAuxParams,sizeof(ucStringAuxParams));
	#ifndef def_try_vdip_1024_wrf						
							sprintf(handleSaveParams.msg2vdip,"WRF %i\r%s",strlen(ucStringAuxParams),ucStringAuxParams);
							if (ucMode==0){
								vSendVdipString_NOCR(handleSaveParams.msg2vdip);
								vReceiveVdipString();
							}
							else{
								spsi.op=savePars_return_op_printfile;
							}
	#else
							if (ucMode==0){
								ui_send_vdip_string(enum_send_vdip_string_op_add,ucStringAuxParams);
							}
							else{
								sprintf(handleSaveParams.msg2vdip,"WRF %i\r%s",strlen(ucStringAuxParams),ucStringAuxParams);
								spsi.op=savePars_return_op_printfile;
							}
	#endif						
						}
						else{
							handleSaveParams.ucIamInPrograms=0;
						}

					}
					else
						handleSaveParams.ucIamInPrograms=0;
				}
				else if (formatoParMac[handleSaveParams.i].fp==enumFP_end_programs){
					handleSaveParams.uiNumLoop++;
					if (handleSaveParams.ucIamInPrograms){
						sprintf(ucStringAuxParams,"[end program %i]\r\n",(int)handleSaveParams.iNumPrg);
						//uc_format_str_len_mult_16(ucStringAuxParams,sizeof(ucStringAuxParams));
	#ifndef def_try_vdip_1024_wrf						
							sprintf(handleSaveParams.msg2vdip,"WRF %i\r%s",strlen(ucStringAuxParams),ucStringAuxParams);
							if (ucMode==0){
								vSendVdipString_NOCR(handleSaveParams.msg2vdip);
								vReceiveVdipString();
							}
							else{
								spsi.op=savePars_return_op_printfile;
							}
	#else
							if (ucMode==0){
								ui_send_vdip_string(enum_send_vdip_string_op_add,ucStringAuxParams);
							}
							else{
								sprintf(handleSaveParams.msg2vdip,"WRF %i\r%s",strlen(ucStringAuxParams),ucStringAuxParams);
								spsi.op=savePars_return_op_printfile;
							}
	#endif						
						
						
						handleSaveParams.i=handleSaveParams.idxStruct-1;
					}
				}
				else if (formatoParMac[handleSaveParams.i].fp==enumFP_label){
					handleSaveParams.uiNumLoop++;
					sprintf(ucStringAuxParams,"%s\r\n",formatoParMac[handleSaveParams.i].ucName);
					//uc_format_str_len_mult_16(ucStringAuxParams,sizeof(ucStringAuxParams));
	#ifndef def_try_vdip_1024_wrf						
					sprintf(handleSaveParams.msg2vdip,"WRF %i\r%s",strlen(ucStringAuxParams),ucStringAuxParams);
					if (ucMode==0){
						vSendVdipString_NOCR(handleSaveParams.msg2vdip);
						vReceiveVdipString();
					}
					else{
						spsi.op=savePars_return_op_printfile;
					}
	#else
					if (ucMode==0){
						ui_send_vdip_string(enum_send_vdip_string_op_add,ucStringAuxParams);
					}
					else{
						sprintf(handleSaveParams.msg2vdip,"WRF %i\r%s",strlen(ucStringAuxParams),ucStringAuxParams);
						spsi.op=savePars_return_op_printfile;
					}
	#endif						
					
				}
				// clausola di chiusura del file...
				else if (formatoParMac[handleSaveParams.i].fp==enumFP_end_file){
					handleSaveParams.uiNumLoop++;
					sprintf(ucStringAuxParams,"%s\r\n",formatoParMac[handleSaveParams.i].ucName);
					//uc_format_str_len_mult_16(ucStringAuxParams,sizeof(ucStringAuxParams));
	#ifndef def_try_vdip_1024_wrf						
					sprintf(handleSaveParams.msg2vdip,"WRF %i\r%s",strlen(ucStringAuxParams),ucStringAuxParams);
					if (ucMode==0){
						vSendVdipString_NOCR(handleSaveParams.msg2vdip);
						vReceiveVdipString();
					}
					else{
						spsi.op=savePars_return_op_printfile;
					}
	#else
					if (ucMode==0){
						ui_send_vdip_string(enum_send_vdip_string_op_add,ucStringAuxParams);
					}
					else{
						sprintf(handleSaveParams.msg2vdip,"WRF %i\r%s",strlen(ucStringAuxParams),ucStringAuxParams);
						spsi.op=savePars_return_op_printfile;
					}
	#endif						
					
					handleSaveParams.stato=enumSP_endOk;
					break;
				}
				else if (formatoParMac[handleSaveParams.i].fp==enumFP_unsigned_start_struct){
					handleSaveParams.uiNumLoop++;
					if (!handleSaveParams.ucIamInStruct){
						handleSaveParams.ucIamInStruct=1;
						handleSaveParams.iContaStruct=formatoParMac[handleSaveParams.i].ucNumOf;
						handleSaveParams.idxStruct=handleSaveParams.i+1;
						handleSaveParams.iNumFieldsStruct=0;

						type_union_unsignedchar_int u;
						u.puc = formatoParMac[handleSaveParams.i].p;
						handleSaveParams.iStructSize = u.i;
						//handleSaveParams.iStructSize=*(int*)&(formatoParMac[handleSaveParams.i].p);
					}
				}
				else if (formatoParMac[handleSaveParams.i].fp==enumFP_unsigned_end_struct){
					handleSaveParams.uiNumLoop++;
					if (handleSaveParams.ucIamInStruct){
						if (++handleSaveParams.iNumFieldsStruct>=handleSaveParams.iContaStruct)
							handleSaveParams.ucIamInStruct=0;
						else{
							// incremento di i-1 perch� i viene incrementato di 1 dal ciclo for...
							handleSaveParams.i=handleSaveParams.idxStruct-1;
						}
					}
				}
				else if (formatoParMac[handleSaveParams.i].fp==enumFP_date_and_time){
					vReadTimeBytesRTC_DS3231();
					sprintf(ucStringAuxParams,"[Date=%02i/%02i/%02i Time=%02i:%02i:%02i]\r\n",
							(int)(*(unsigned char*)(&rtc.ucGiorno)),
							(int)(*(unsigned char*)(&rtc.ucGiorno+1)),
							(int)(*(unsigned char*)(&rtc.ucGiorno+2)),
							(int)(*(unsigned char*)(&rtc.ucGiorno+3)),
							(int)(*(unsigned char*)(&rtc.ucGiorno+4)),
							(int)(*(unsigned char*)(&rtc.ucGiorno+5))
							);
					//uc_format_str_len_mult_16(ucStringAuxParams,sizeof(ucStringAuxParams));
	#ifndef def_try_vdip_1024_wrf						
					sprintf(handleSaveParams.msg2vdip,"WRF %i\r%s",strlen(ucStringAuxParams),ucStringAuxParams);
					if (ucMode==0){
						vSendVdipString_NOCR(handleSaveParams.msg2vdip);
						vReceiveVdipString();
					}
					else{
						spsi.op=savePars_return_op_printfile;
					}
	#else
					if (ucMode==0){
						ui_send_vdip_string(enum_send_vdip_string_op_add,ucStringAuxParams);
					}
					else{
						sprintf(handleSaveParams.msg2vdip,"WRF %i\r%s",strlen(ucStringAuxParams),ucStringAuxParams);
						spsi.op=savePars_return_op_printfile;
					}
	#endif						

				}
				else{
					unsigned int ui_idx_start_loop;
					unsigned int ui_idx_end_loop;
					// correzione bug per cui in modo ethernet salvava solo l'ultimo di una serie di parametri...
					if (ucMode==0){
						ui_idx_start_loop=0;
						ui_idx_end_loop=formatoParMac[handleSaveParams.i].ucNumOf;
					}
					// execute only a loop to send the string to the ethernet...
					else{
						ui_idx_start_loop=handleSaveParams.idx_param_loop++;
						ui_idx_end_loop=ui_idx_start_loop+1;
						if (handleSaveParams.idx_param_loop<formatoParMac[handleSaveParams.i].ucNumOf){
							ui_switch_to_next_loop=0;
						}
					}
					if ((ucMode==0)||(ui_idx_start_loop==0)){
						handleSaveParams.p=formatoParMac[handleSaveParams.i].p;
						if (handleSaveParams.ucIamInStruct)
							handleSaveParams.p+=handleSaveParams.iNumFieldsStruct*handleSaveParams.iStructSize;
					}
					
					
					
					
					for (handleSaveParams.iNumOfPars=ui_idx_start_loop;handleSaveParams.iNumOfPars<ui_idx_end_loop;handleSaveParams.iNumOfPars++){
						handleSaveParams.uiNumLoop++;
						// nel tipo stringa, il campo numof indica il numero di chars della stringa
						// e non il numero di pars...
						if ((formatoParMac[handleSaveParams.i].fp==enumFP_string)&&handleSaveParams.iNumOfPars>0)
							break;
						switch(formatoParMac[handleSaveParams.i].fp){
							case enumFP_string:
								sprintf(handleSaveParams.ucParValue,"%-*.*s",formatoParMac[handleSaveParams.i].ucNumOf,formatoParMac[handleSaveParams.i].ucNumOf,handleSaveParams.p);
								break;
							case enumFP_unsigned_char:
								handleSaveParams.myuc=*(unsigned char*)handleSaveParams.p;
								sprintf(handleSaveParams.ucParValue,"%i",(int)handleSaveParams.myuc);
								handleSaveParams.p+=sizeof(unsigned char);
								break;
							case enumFP_unsigned_int:
								sprintf(handleSaveParams.ucParValue,"%u",*(unsigned int*)handleSaveParams.p);
								handleSaveParams.p+=sizeof(unsigned int);
								break;
							case enumFP_unsigned_short:
								sprintf(handleSaveParams.ucParValue,"%hu",*(unsigned short int*)handleSaveParams.p);
								handleSaveParams.p+=sizeof(unsigned short int);
								break;
							case enumFP_signed_int:
								sprintf(handleSaveParams.ucParValue,"%i",*(int*)handleSaveParams.p);
								handleSaveParams.p+=sizeof(signed int);
								break;
							case enumFP_signed_short:
								sprintf(handleSaveParams.ucParValue,"%hi",*(short int*)handleSaveParams.p);
								handleSaveParams.p+=sizeof(signed short int);
								break;
							case enumFP_float:
								sprintf(handleSaveParams.ucParValue,"%f",*(float*)handleSaveParams.p);
								handleSaveParams.p+=sizeof(float);
								break;
							case enumFP_unsigned_long:
								sprintf(handleSaveParams.ucParValue,"%lu",*(unsigned long*)handleSaveParams.p);
								handleSaveParams.p+=sizeof(unsigned long);
								break;
							case enumFP_signed_long:
								sprintf(handleSaveParams.ucParValue,"%lu",*(signed long*)handleSaveParams.p);
								handleSaveParams.p+=sizeof(signed long);
								break;
							default:
								break;
						}
						// i parametri struttura sono nel formato <nomepar>_s<idx struttura>[_<idx parametro>]
						// es.: p0_s0
						// i parametri non struttura sono nel formato <nomepar>[_<idx parametro>]
						if (handleSaveParams.ucIamInStruct){
							if (formatoParMac[handleSaveParams.i].ucNumOf>1)
								sprintf(ucStringAuxParams,"%s_s%i_%i",formatoParMac[handleSaveParams.i].ucName,handleSaveParams.iNumFieldsStruct,(int)handleSaveParams.iNumOfPars);
							else
								sprintf(ucStringAuxParams,"%s_s%i",formatoParMac[handleSaveParams.i].ucName,handleSaveParams.iNumFieldsStruct);
						}
						else if (formatoParMac[handleSaveParams.i].ucNumOf>1)
							sprintf(ucStringAuxParams,"%s_%i",formatoParMac[handleSaveParams.i].ucName,(int)handleSaveParams.iNumOfPars);
						else
							sprintf(ucStringAuxParams,"%s",formatoParMac[handleSaveParams.i].ucName);
						// formatto in modo che il nome del parametro sia sempre contenuto in un campo da 12 caratteri
						// 8 del nome � 3 dell'indice +1 di riserva
						sprintf(ucStringAuxParams,"%-12.12s=%s\r\n",ucStringAuxParams,handleSaveParams.ucParValue);

						//uc_format_str_len_mult_16(ucStringAuxParams,sizeof(ucStringAuxParams));
	#ifndef def_try_vdip_1024_wrf						

						// formatto la stringa in modo che contenga 32 caratteri=potenza di due,
						// per evitare problemi alla WRF che sembra non funzionare correttamente quando deve scrivere
						// un numero di bytes che non � una potenza di due
						sprintf(handleSaveParams.msg2vdip,"WRF %i\r%s",strlen(ucStringAuxParams),ucStringAuxParams);

						if (ucMode==0){
							vSendVdipString_NOCR(handleSaveParams.msg2vdip);
							//vSendVdipBinaryString(ucStringAuxParams,strlen(ucStringAuxParams));
 
							//vReceiveVdipString_waitAlong();
							vReceiveVdipString();
							// se non ho ricevuto l'ok, aspetto un po' e poi riverifico se tutto ok o no
							if (ucStringRx_Vdip[0]==0){
								// aspetto ...
								handleSaveParams.iOldClock=ucCntInterrupt;
								handleSaveParams.iTimeElapsed=0;
								// aspetto 10 secondi e riprovo
								while(handleSaveParams.iTimeElapsed<10000){
									handleSaveParams.iActClock=ucCntInterrupt;
									if (handleSaveParams.iActClock>=handleSaveParams.iOldClock){
										handleSaveParams.iTimeElapsed+=(handleSaveParams.iActClock-handleSaveParams.iOldClock)*PeriodoIntMs;
									}
									else{
										handleSaveParams.iTimeElapsed+=((256+handleSaveParams.iActClock)-handleSaveParams.iOldClock)*PeriodoIntMs;
									}
									handleSaveParams.iOldClock=handleSaveParams.iActClock;
								}
								vReceiveVdipString();
							}
							if (ucStringRx_Vdip[0]==0){
								handleSaveParams.exitCode=err_vdip_no_response_on_writing;
								handleSaveParams.stato=enumSP_end;
								return 1;
							}
							else
								// se bad command, errore!!!
								for (handleSaveParams.j=0;ucStringRx_Vdip[handleSaveParams.j];handleSaveParams.j++){
									if (  (strncmp(ucStringRx_Vdip+handleSaveParams.j,"Bad" ,3)==0)
										 ||(strncmp(ucStringRx_Vdip+handleSaveParams.j,"File",4)==0)
										 ||(strncmp(ucStringRx_Vdip+handleSaveParams.j,"Disk",4)==0)
										 ||(strncmp(ucStringRx_Vdip+handleSaveParams.j,"Read",4)==0)
										 ||(strncmp(ucStringRx_Vdip+handleSaveParams.j,"Fail",4)==0)
									   ){
										handleSaveParams.exitCode=err_vdip_diskerr_writing;
										handleSaveParams.stato=enumSP_end;
										return 1;
									}
								}
						}
						else{
							spsi.op=savePars_return_op_printfile;
						}
						
	#else
							if (ucMode==0){
								ui_send_vdip_string(enum_send_vdip_string_op_add,ucStringAuxParams);
							}
							else{
								sprintf(handleSaveParams.msg2vdip,"WRF %i\r%s",strlen(ucStringAuxParams),ucStringAuxParams);
								spsi.op=savePars_return_op_printfile;
							}
	#endif						
						



					}
				}
				if ((ucMode==0)||ui_switch_to_next_loop){
					handleSaveParams.stato=enumSP_nextLoop;
				}
				break;
			}
		case enumSP_nextLoop:
			handleSaveParams.i++;
			handleSaveParams.idx_param_loop=0;
			handleSaveParams.fEstimatedDonePerc=(handleSaveParams.uiNumLoop*100.0)/handleSaveParams.uiTotalNumOfLoops;
			if (handleSaveParams.fEstimatedDonePerc<0)
				handleSaveParams.fEstimatedDonePerc=0;
			else if (handleSaveParams.fEstimatedDonePerc>100)
				handleSaveParams.fEstimatedDonePerc=100;

			if (handleSaveParams.i>=sizeof(formatoParMac)/sizeof(formatoParMac[0]))
				handleSaveParams.stato=enumSP_endOk;
			else
				handleSaveParams.stato=enumSP_doLoop;
			break;
		case enumSP_endOk:
			handleSaveParams.fEstimatedDonePerc=100;
			handleSaveParams.exitCode=all_ok_vdip;
			handleSaveParams.stato=enumSP_end;
#ifdef def_try_vdip_1024_wrf		
			ui_send_vdip_string(enum_send_vdip_string_op_flush,ucStringAuxParams);
#endif			
		
			break;
		case enumSP_end:
			if (ucMode==0){
				uiRequestSuspendAnybusOp(0);
				vSendVdipString("CLF az4181P.ini");
				vReceiveVdipString();
			}
			else{
				spsi.op=savePars_return_op_closefile;
			}
			handleSaveParams.stato=enumSP_idle;
			break;
	}
	return 1;


}

// recupero di tutti i codici prodotto salvati su chiavetta usb...
// ritorna 0 se gestione finita
// ucMode=0 --> carica da usb
// ucMode=1 --> carica da ethernet
unsigned char ucLoadCodiciProdottoOnFile_Stati(unsigned char ucMode){
xdata int i,len;
xdata unsigned char *puc;
xdata static unsigned char ucLoadMacFileStato;
xdata static unsigned int uiFileLength;

	spsi.op=savePars_return_op_none;
	spsi.pc2save=ucStringAuxParams;

	switch (handleSaveParams.stato){
		case enumSP_idle:
			return 0;
		case enumSP_initVdip:
			ucLoadMacFileStato=0;
			handleSaveParams.uiTotalNumOfLoops=1;
			handleSaveParams.uiNumLoop=0;
			handleSaveParams.fEstimatedDonePerc=0;

			handleSaveParams.exitCode=all_ok_vdip;
			handleSaveParams.i=0;
			if (ucMode==0){
				uiRequestSuspendAnybusOp(1);
				if (!uiHasBeenAckRequestSuspendAnybusOp())
					break;
				vInitVdip();
			}
			else{
				spsi.op=savePars_return_op_init;
			}
			handleSaveParams.stato=enumSP_handleVdip;
			break;
		case enumSP_handleVdip:
			if (ucMode==0){
				if (ucHandleVdipStati()){
					handleSaveParams.stato=enumSP_openFile;
				}
			}
			else{
				handleSaveParams.stato=enumSP_openFile;
		    }
			break;
		case enumSP_openFile:
			if (ucMode==0){
				vSendVdipString("DIR az4181P.ini");
				vReceiveVdipString();// elimino la riga vuota
				vReceiveVdipString();// riga coi dati
				// se il file non esiste, continuo col prossimo
				for (i=0;ucStringRx_Vdip[i];i++){
					if (strncmp(ucStringRx_Vdip+i,"FAILED",6)==0){
						handleSaveParams.exitCode=err_vdip_file_notexists;
						handleSaveParams.stato=enumSP_end;
						return 1;
					}
				}


			// determino la lunghezza del file
				ucStringAuxParams[0]='0';
				ucStringAuxParams[1]='x';
				handleSaveParams.iStructSize=2;
				// vado alla fine della stringa
				for (i=0;ucStringRx_Vdip[i];i++){
				}
				// arretro per trovare il carattere '$'
				while(i>=1){
					i--;
					if (ucStringRx_Vdip[i]=='$'){
						if (isxdigit(ucStringRx_Vdip[i+1])){
							ucStringAuxParams[handleSaveParams.iStructSize++]  =ucStringRx_Vdip[i+1];
							if (isxdigit(ucStringRx_Vdip[i+2])){
								ucStringAuxParams[handleSaveParams.iStructSize++]=ucStringRx_Vdip[i+2];
							}
						}
					}
				}
				ucStringAuxParams[handleSaveParams.iStructSize]=0;
				sscanf(ucStringAuxParams,"%X",&handleSaveParams.uiLenOfFile);
				if (handleSaveParams.uiLenOfFile==0){
					handleSaveParams.exitCode=err_vdip_file_is_empty;
					handleSaveParams.stato=enumSP_end;
					return 1;
				}

				uiFileLength=handleSaveParams.uiLenOfFile;
				vReceiveVdipString();// elimino il prompt
				// adesso apro il file di salvataggio parametri...
				vSendVdipString("OPR az4181P.ini");
				vReceiveVdipString();
			}
			else{
				spsi.op=savePars_return_op_openfile;
				spsi.ucDataReady=0;
			}
			handleSaveParams.stato=enumSP_openFile_ctrlErr;
			break;
		case enumSP_openFile_ctrlErr:
			if (ucMode==0){
				if (ucStringRx_Vdip[0]==0){
					handleSaveParams.exitCode=err_vdip_openfile_for_reading;
					handleSaveParams.stato=enumSP_end;
					return 1;
				}
				else{
					// se bad command, errore!!!
					for (handleSaveParams.j=0;ucStringRx_Vdip[handleSaveParams.j];handleSaveParams.j++){
						if ( (strncmp(ucStringRx_Vdip+handleSaveParams.j,"File",4)==0)
							||(strncmp(ucStringRx_Vdip+handleSaveParams.j,"Disk",4)==0)
							||(strncmp(ucStringRx_Vdip+handleSaveParams.j,"Read",4)==0)
							||(strncmp(ucStringRx_Vdip+handleSaveParams.j,"Fail",4)==0)
							) {
							handleSaveParams.exitCode=err_vdip_diskerr_while_reading;
							handleSaveParams.stato=enumSP_end;
							return 1;
						}
					}
				}
			}
			handleSaveParams.stato=enumSP_initLoop;
			break;
		case enumSP_initLoop:
			handleSaveParams.ucIamInStruct=0;
			handleSaveParams.ucIamInPrograms=0;
			handleSaveParams.i=0;
			// inizializzo la routine di lettura bytes da vdip
			v_init_read_bytes_from_vdip();
			handleSaveParams.stato=enumSP_doLoop;
			handleSaveParams.idxStartParams=0;
			handleSaveParams.idxEndParams=0;
			for (i=0;i<sizeof(formatoParMac)/sizeof(formatoParMac[0]);i++){
				if (!handleSaveParams.idxStartParams&&(strcmp(formatoParMac[i].ucName,"[Program 0]")==0)){
					handleSaveParams.idxStartParams=i+1;
					continue;
				}
				if (strcmp(formatoParMac[i].ucName,"[end program 0]")==0){
					handleSaveParams.idxEndParams=i-1;
					break;
				}
			}


			break;
		case enumSP_doLoop:
			if (ucMode==0){
				for (i=0;i<sizeof(ucStringAuxParams)-2;i++){
					if (handleSaveParams.uiLenOfFile==0){
						handleSaveParams.stato=enumSP_end;
						return 1;
					}
					uc_the_byte_read_from_vdip=uc_read_a_byte_from_vdip(handleSaveParams.uiLenOfFile);
					handleSaveParams.uiLenOfFile--;
					if (uc_the_byte_read_from_vdip=='\n'){
						ucStringAuxParams[i]=0;
						break;
					}
					ucStringAuxParams[i]=uc_the_byte_read_from_vdip;
				}
				handleSaveParams.fEstimatedDonePerc=100.0-(100.0*handleSaveParams.uiLenOfFile)/uiFileLength;
				if (handleSaveParams.fEstimatedDonePerc<0)
					handleSaveParams.fEstimatedDonePerc=0;
				else if (handleSaveParams.fEstimatedDonePerc>100)
					handleSaveParams.fEstimatedDonePerc=100;

				// se finito file, esco con tutto ok
				if (!i){
					handleSaveParams.stato=enumSP_endOk;
					return 1;
				}
				if (ucStringAuxParams[i-1]=='\r'){
					ucStringAuxParams[i-1]=0;
				}
			}
			else{
				if (!spsi.ucDataReady){
					spsi.op=savePars_return_op_inputfile;
					break;
				}
				else{
					spsi.ucDataReady=0;
					// se finito file, esco con tutto ok
					if (ucStringAuxParams[0]==0){
						handleSaveParams.stato=enumSP_endOk;
						return 1;
					}
				}
			}
			// devo trovare la clausola [Program
			if (ucLoadMacFileStato==0){
				if (strncmp(ucStringAuxParams,"[Program",8)==0){
					ucLoadMacFileStato=1;
					return 1;
				}
				// se non ho trovato la clausola, esco
				return 1;
			}
			// se trovo clausola fine file, tutto ok, esco e fine
			if (strncmp(ucStringAuxParams,"[End file",9)==0){
				handleSaveParams.stato=enumSP_endOk;
				return 1;
			}
			// se trovo [end program, azzero stato, salvo programma e aspetto prossimo
			if (strncmp(ucStringAuxParams,"[end program",12)==0){

				vVerifyInsertCodiceProdotto(auxPrg_params.codicePrg);

				// gli indici next/prev li recupero non da usb key, ma da struttura PROGRAMMA!!!
				auxPrg_params.nextPrg=hprg.theAct.nextPrg;
				auxPrg_params.prevPrg=hprg.theAct.prevPrg;
				// adesso riverso tutti i pars del programma sovrascrivendo quelli esistenti in macchina
				memcpy(&hprg.theAct,&auxPrg_params,sizeof(hprg.theAct));

				// effettuo la validazione dei parametri programma...
				ValidatePrg();
				// salvo il programma
				ucSaveActPrg();

				ucLoadMacFileStato=0;
				return 1;
			}

			i=0;
			handleSaveParams.j=0;
			// indice struttura
			handleSaveParams.idxStruct=0;
			// indice parametro
			handleSaveParams.iNumOfPars=0;
			// cerco: dove finisce il nome parametro, dove si trova l'indice struttura, dove si trova l'indice parametro
			// se esiste _s, � struttura
			// se esiste _<number>, � indice di parametro
			// identifico dove inizia il valore, cercando '='
			while(ucStringAuxParams[i]&&(ucStringAuxParams[i]!='=')){
				if (ucStringAuxParams[i]==' '){
					// fine del nome del parametro!
					if (!handleSaveParams.j)
						handleSaveParams.j=i;
				}
				else if (ucStringAuxParams[i]=='_'){
					// fine del nome del parametro!
					if (!handleSaveParams.j)
						handleSaveParams.j=i;
					handleSaveParams.iNumOfPars=atoi(ucStringAuxParams+i+1);
				}
				i++;
			}
			if (!ucStringAuxParams[i]){
				handleSaveParams.exitCode=err_vdip_bad_row_syntax;
				handleSaveParams.stato=enumSP_end;
				return 1;
			}
			if (!handleSaveParams.j)
				handleSaveParams.j=i;
			// salto il carattere '='
			i++;
			// puntatore alla stringa che contiene il valore...
			puc=ucStringAuxParams+i;

			handleSaveParams.ucIamInStruct=0;
			for (i=handleSaveParams.idxStartParams;i<=handleSaveParams.idxEndParams;i++){
				// prendo la lunghezza massima tra le due stringhe
				// in modo da evitare problemi con parametri che si chiamano es k e ksh
				if (handleSaveParams.j<strlen(formatoParMac[i].ucName))
					len=strlen(formatoParMac[i].ucName);
				else
					len=handleSaveParams.j;
				// faccio una strncmp prendendo la massima delle due lunghezze come termine di confronto...
				if (strncmp(formatoParMac[i].ucName,ucStringAuxParams,len)==0)
					break;
			}
			// parametro: NON trovato!!!
			if (i>handleSaveParams.idxEndParams){
				break;
			}
			// handleSaveParams.i=indice del parametro
			handleSaveParams.i=i;
			// VERIFICO DI NON SUPERARE INDICE MASSIMO...
			if (handleSaveParams.iNumOfPars<formatoParMac[handleSaveParams.i].ucNumOf){

				// puntatore alla zona di mem dove salvare il parametro
				handleSaveParams.p=formatoParMac[handleSaveParams.i].p;

				switch(formatoParMac[handleSaveParams.i].fp){
					case enumFP_string:
						strcpy(handleSaveParams.p,puc);
						break;
					case enumFP_unsigned_char:
						handleSaveParams.p+=sizeof(unsigned char)*handleSaveParams.iNumOfPars;
						*(unsigned char*)handleSaveParams.p=atoi(puc);
						break;
					case enumFP_unsigned_int:
						handleSaveParams.p+=sizeof(unsigned int)*handleSaveParams.iNumOfPars;
						*(unsigned int*)handleSaveParams.p=atoi(puc);
						break;
					case enumFP_unsigned_short:
						handleSaveParams.p+=sizeof(unsigned short)*handleSaveParams.iNumOfPars;
						*(unsigned short*)handleSaveParams.p=atoi(puc);
						break;
					case enumFP_signed_int:
						handleSaveParams.p+=sizeof(unsigned int)*handleSaveParams.iNumOfPars;
						*(unsigned int*)handleSaveParams.p=atoi(puc);
						break;
					case enumFP_signed_short:
						handleSaveParams.p+=sizeof(signed short)*handleSaveParams.iNumOfPars;
						*(signed short*)handleSaveParams.p=atoi(puc);
						break;
					case enumFP_float:
						handleSaveParams.p+=sizeof(float)*handleSaveParams.iNumOfPars;
						*(float*)handleSaveParams.p=atof(puc);
						break;
					case enumFP_unsigned_long:
					case enumFP_signed_long:
						handleSaveParams.p+=sizeof(unsigned long)*handleSaveParams.iNumOfPars;
						*(unsigned long*)handleSaveParams.p=atol(puc);
						break;
					default:
					{
						break;
					}
				}
			}

			break;
		case enumSP_endOk:
			handleSaveParams.fEstimatedDonePerc=100;
			handleSaveParams.exitCode=all_ok_vdip;
			handleSaveParams.stato=enumSP_end;
			break;
		case enumSP_end:
		default:
			if (ucMode==0){
				uiRequestSuspendAnybusOp(0);
				vSendVdipString("CLF az4181P.ini");
				vReceiveVdipString();
			}
			else{
				spsi.op=savePars_return_op_closefile;
			}
			handleSaveParams.stato=enumSP_idle;
			vRefreshActiveProductCode();
			break;


	}
	return 1;


}




// recupero di tutti i parametri macchina su file...
// ritorna 0 se gestione finita
// ingresso: 0-->carica da usb
//           1-->carica da ethernet
unsigned char ucLoadParMacOnFile_Stati(unsigned char ucMode){
xdata int i,len;
xdata unsigned char *puc;
xdata static unsigned char ucLoadMacFileStato;

	spsi.op=savePars_return_op_none;
	spsi.pc2save=ucStringAuxParams;
	switch (handleSaveParams.stato){
		case enumSP_idle:
			return 0;
		case enumSP_initVdip:
			ucLoadMacFileStato=0;
			handleSaveParams.uiTotalNumOfLoops=1;
			handleSaveParams.uiNumLoop=0;
			handleSaveParams.fEstimatedDonePerc=0;

			handleSaveParams.exitCode=all_ok_vdip;
			handleSaveParams.i=0;
			if (ucMode==0){
				uiRequestSuspendAnybusOp(1);
				if (!uiHasBeenAckRequestSuspendAnybusOp())
					break;
				vInitVdip();
			}
			else{
				spsi.op=savePars_return_op_init;
			}
			handleSaveParams.stato=enumSP_handleVdip;
			break;
		case enumSP_handleVdip:
			if (ucMode==0){
				if (ucHandleVdipStati()){
					handleSaveParams.stato=enumSP_openFile;
				}
			}
			else{
				handleSaveParams.stato=enumSP_openFile;
		    }
			break;
		case enumSP_openFile:
			if (ucMode==0){
				vSendVdipString("DIR az4181P.ini");
				vReceiveVdipString();// elimino la riga vuota
				vReceiveVdipString();// riga coi dati
				// se il file non esiste, continuo col prossimo
				for (i=0;ucStringRx_Vdip[i];i++){
					if (strncmp(ucStringRx_Vdip+i,"FAILED",6)==0){
						handleSaveParams.exitCode=err_vdip_file_notexists;
						handleSaveParams.stato=enumSP_end;
						return 1;
					}
				}


			// determino la lunghezza del file
				ucStringAuxParams[0]='0';
				ucStringAuxParams[1]='x';
				handleSaveParams.iStructSize=2;
				// vado alla fine della stringa
				for (i=0;ucStringRx_Vdip[i];i++){
				}
				// arretro per trovare il carattere '$'
				while(i>=1){
					i--;
					if (ucStringRx_Vdip[i]=='$'){
						if (isxdigit(ucStringRx_Vdip[i+1])){
							ucStringAuxParams[handleSaveParams.iStructSize++]  =ucStringRx_Vdip[i+1];
							if (isxdigit(ucStringRx_Vdip[i+2])){
								ucStringAuxParams[handleSaveParams.iStructSize++]=ucStringRx_Vdip[i+2];
							}
						}
					}
				}
				ucStringAuxParams[handleSaveParams.iStructSize]=0;
				sscanf(ucStringAuxParams,"%X",&handleSaveParams.uiLenOfFile);
				if (handleSaveParams.uiLenOfFile==0){
					handleSaveParams.exitCode=err_vdip_file_is_empty;
					handleSaveParams.stato=enumSP_end;
					return 1;
				}


				vReceiveVdipString();// elimino il prompt
				// adesso apro il file di salvataggio parametri...
				vSendVdipString("OPR az4181P.ini");
				vReceiveVdipString();
			}
			else{
				spsi.op=savePars_return_op_openfile;
				spsi.ucDataReady=0;
			}
			handleSaveParams.stato=enumSP_openFile_ctrlErr;
			break;
		case enumSP_openFile_ctrlErr:
			if (ucMode==0){
				if (ucStringRx_Vdip[0]==0){
					handleSaveParams.exitCode=err_vdip_openfile_for_reading;
					handleSaveParams.stato=enumSP_end;
					return 1;
				}
				else{
					// se bad command, errore!!!
					for (handleSaveParams.j=0;ucStringRx_Vdip[handleSaveParams.j];handleSaveParams.j++){
						if ( (strncmp(ucStringRx_Vdip+handleSaveParams.j,"File",4)==0)
							||(strncmp(ucStringRx_Vdip+handleSaveParams.j,"Disk",4)==0)
							||(strncmp(ucStringRx_Vdip+handleSaveParams.j,"Read",4)==0)
							||(strncmp(ucStringRx_Vdip+handleSaveParams.j,"Fail",4)==0)
							) {
							handleSaveParams.exitCode=err_vdip_diskerr_while_reading;
							handleSaveParams.stato=enumSP_end;
							return 1;
						}
					}
				}
			}
			handleSaveParams.stato=enumSP_initLoop;
			break;
		case enumSP_initLoop:
			handleSaveParams.ucIamInStruct=0;
			handleSaveParams.ucIamInPrograms=0;
			handleSaveParams.i=0;
			// inizializzo la routine di lettura bytes da vdip
			v_init_read_bytes_from_vdip();
			handleSaveParams.stato=enumSP_doLoop;
			handleSaveParams.idxStartParams=0;
			handleSaveParams.idxEndParams=0;
			for (i=0;i<sizeof(formatoParMac)/sizeof(formatoParMac[0]);i++){
				if (!handleSaveParams.idxStartParams&&(strcmp(formatoParMac[i].ucName,"[Params]")==0)){
					handleSaveParams.idxStartParams=i+1;
					continue;
				}
				if (strcmp(formatoParMac[i].ucName,"[End params]")==0){
					handleSaveParams.idxEndParams=i-1;
					break;
				}
			}


			break;
		case enumSP_doLoop:
			if (ucMode==0){
				// leggo una linea
				for (i=0;i<sizeof(ucStringAuxParams)-2;i++){
					if (handleSaveParams.uiLenOfFile==0){
						handleSaveParams.stato=enumSP_end;
						return 1;
					}
					uc_the_byte_read_from_vdip=uc_read_a_byte_from_vdip(handleSaveParams.uiLenOfFile);
					handleSaveParams.uiLenOfFile--;
					if (uc_the_byte_read_from_vdip=='\n'){
						ucStringAuxParams[i]=0;
						break;
					}
					ucStringAuxParams[i]=uc_the_byte_read_from_vdip;
				}
				// se finito file, esco con tutto ok
				if (!i){
					handleSaveParams.stato=enumSP_endOk;
					return 1;
				}
				if (ucStringAuxParams[i-1]=='\r'){
					ucStringAuxParams[i-1]=0;
				}
			}
			else{
				if (!spsi.ucDataReady){
					spsi.op=savePars_return_op_inputfile;
					break;
				}
				else{
					spsi.ucDataReady=0;
					// se finito file, esco con tutto ok
					if (ucStringAuxParams[0]==0){
						handleSaveParams.stato=enumSP_endOk;
						return 1;
					}
				}
			}
			// devo trovare la clausola "params"
			if (ucLoadMacFileStato==0){
				if (strncmp(ucStringAuxParams,"[Params]",8)==0){
					ucLoadMacFileStato=1;
					return 1;
				}
				// se non ho trovato la clausola, esco
				return 1;
			}
			// se trovo end params, fine procedura
			if (strncmp(ucStringAuxParams,"[End params]",12)==0){
				handleSaveParams.stato=enumSP_endOk;
				return 1;
			}
			// se trovo clausola fine file, tutto ok, esco e fine
			if (strncmp(ucStringAuxParams,"[End file",9)==0){
				handleSaveParams.stato=enumSP_endOk;
				return 1;
			}

			i=0;
			handleSaveParams.j=0;
			// indice struttura
			handleSaveParams.idxStruct=0;
			// indice parametro
			handleSaveParams.iNumOfPars=0;
			// cerco: dove finisce il nome parametro, dove si trova l'indice struttura, dove si trova l'indice parametro
			// se esiste _s, � struttura
			// se esiste _<number>, � indice di parametro
			// identifico dove inizia il valore, cercando '='
			while(ucStringAuxParams[i]&&(ucStringAuxParams[i]!='=')){
				if (ucStringAuxParams[i]==' '){
					// fine del nome del parametro!
					if (!handleSaveParams.j)
						handleSaveParams.j=i;
				}
				else if (ucStringAuxParams[i]=='_'){
					// fine del nome del parametro!
					if (!handleSaveParams.j)
						handleSaveParams.j=i;
					if (ucStringAuxParams[i+1]=='s'){
						handleSaveParams.idxStruct=atoi(ucStringAuxParams+i+2);
					}
					else
						handleSaveParams.iNumOfPars=atoi(ucStringAuxParams+i+1);
				}
				i++;
			}
			if (!ucStringAuxParams[i]){
				handleSaveParams.exitCode=err_vdip_bad_row_syntax;
				handleSaveParams.stato=enumSP_end;
				return 1;
			}
			if (!handleSaveParams.j)
				handleSaveParams.j=i;
			// salto il carattere '='
			i++;
			// puntatore alla stringa che contiene il valore...
			puc=ucStringAuxParams+i;

			handleSaveParams.ucIamInStruct=0;
			for (i=handleSaveParams.idxStartParams;i<=handleSaveParams.idxEndParams;i++){
				if (formatoParMac[i].fp==enumFP_unsigned_start_struct){
					if (!handleSaveParams.ucIamInStruct){
						handleSaveParams.ucIamInStruct=1;
						handleSaveParams.iContaStruct=formatoParMac[i].ucNumOf;
						type_union_unsignedchar_int u;
						u.puc = formatoParMac[i].p;
						handleSaveParams.iStructSize = u.i;
						//handleSaveParams.iStructSize=*(int*)&(formatoParMac[i].p);
					}
				}
				else if (formatoParMac[i].fp==enumFP_unsigned_end_struct){
					if (handleSaveParams.ucIamInStruct){
						handleSaveParams.ucIamInStruct=0;
					}
				}
				// prendo la lunghezza massima tra le due stringhe
				// in modo da evitare problemi con parametri che si chiamano es k e ksh
				if (handleSaveParams.j<strlen(formatoParMac[i].ucName))
					len=strlen(formatoParMac[i].ucName);
				else
					len=handleSaveParams.j;
				// faccio una strncmp prendendo la massima delle due lunghezze come termine di confronto...
				if (strncmp(formatoParMac[i].ucName,ucStringAuxParams,len)==0)
					break;
			}
			// parametro: NON trovato!!!
			if (i>handleSaveParams.idxEndParams){
				break;
			}
			// handleSaveParams.i=indice del parametro
			handleSaveParams.i=i;



			// puntatore alla zona di mem dove salvare il parametro
			handleSaveParams.p=formatoParMac[handleSaveParams.i].p;

			// se trovo il parametro "versione"...
			// verifico che non sia troppo aggiornata...
			if (handleSaveParams.p==(unsigned char xdata *)&macParms.uiMacParmsFormatVersion){
				// se versione troppo aggiornata, errore
				if (atoi(puc)>macParms.uiMacParmsFormatVersion){
					handleSaveParams.exitCode=err_vdip_bad_params_version;
					handleSaveParams.stato=enumSP_end;
					return 1;
				}
				// se versione ok, non aggiorno ovviamente il numero di versione, lascio il mio!!!
				return 1;
			}

			// non modifico ovviamente il serial number!!!
			// se serial number non coincide, ritorno errore? per ora no
			// box sn non pu� essere impostato importando file ini
			if (handleSaveParams.p==(unsigned char *)&ulBoxSerialNumber){
				return 1;
			}
			// distanziatore non pu� essere impostato importando file ini
			if (handleSaveParams.p==(unsigned char *)&macParms.uc_distanziatore_type){
				return 1;
			}
			// grinding non pu� essere impostato importando file ini
			if (handleSaveParams.p==(unsigned char *)&macParms.uc_enable_spindle_grinding){
				return 1;
			}
			// lampada allarme non pu� essere impostata importando file ini
			if (handleSaveParams.p==(unsigned char *)&macParms.ucAbilitaLampadaAllarme){
				return 1;
			}
			// abilita quinta portata non pu� essere impostata importando file ini
			if (handleSaveParams.p==(unsigned char *)&macParms.ucAbilitaQuintaPortata){
				return 1;
			}
			// abilita compensazione temperatura non pu� essere impostata importando file ini
			if (handleSaveParams.p==(unsigned char *)&macParms.ucAbilitaCompensazioneTemperatura){
				return 1;
			}
            // password non pu� essere impostata importando file ini
            if (handleSaveParams.p==(unsigned char *)&macParms.password)
            {
                return 1;
            }

			// se sono all'interno di una struttura, devo sommare la dimensione della struttura
			// moltiplicata per l'indice struttura
			if (handleSaveParams.ucIamInStruct)
				handleSaveParams.p+=handleSaveParams.idxStruct*handleSaveParams.iStructSize;
			switch(formatoParMac[handleSaveParams.i].fp){
				case enumFP_string:
					strcpy(handleSaveParams.p,puc);
					break;
				case enumFP_unsigned_char:
					handleSaveParams.p+=sizeof(unsigned char)*handleSaveParams.iNumOfPars;
					*(unsigned char*)handleSaveParams.p=atoi(puc);
					break;
				case enumFP_unsigned_int:
					handleSaveParams.p+=sizeof(unsigned int)*handleSaveParams.iNumOfPars;
					*(unsigned int*)handleSaveParams.p=atoi(puc);
					break;
				case enumFP_unsigned_short:
					handleSaveParams.p+=sizeof(unsigned short)*handleSaveParams.iNumOfPars;
					*(unsigned short*)handleSaveParams.p=atoi(puc);
					break;
				case enumFP_signed_int:
					handleSaveParams.p+=sizeof(unsigned int)*handleSaveParams.iNumOfPars;
					*(unsigned int*)handleSaveParams.p=atoi(puc);
					break;
				case enumFP_signed_short:
					handleSaveParams.p+=sizeof(signed short)*handleSaveParams.iNumOfPars;
					*(signed short*)handleSaveParams.p=atoi(puc);
					break;
				case enumFP_float:
					handleSaveParams.p+=sizeof(float)*handleSaveParams.iNumOfPars;
					*(float*)handleSaveParams.p=atof(puc);
					break;
				case enumFP_unsigned_long:
				case enumFP_signed_long:
					handleSaveParams.p+=sizeof(unsigned long)*handleSaveParams.iNumOfPars;
					*(unsigned long*)handleSaveParams.p=atol(puc);
					break;
				default:
					break;
			}

			break;
		case enumSP_endOk:
			handleSaveParams.exitCode=all_ok_vdip;
			handleSaveParams.stato=enumSP_end;
			break;
		case enumSP_end:
		default:
			if (ucMode==0){
				uiRequestSuspendAnybusOp(0);
				vSendVdipString("CLF az4181P.ini");
				vReceiveVdipString();
			}
			else{
				spsi.op=savePars_return_op_closefile;
			}
			handleSaveParams.stato=enumSP_idle;
			break;


	}
	return 1;

}



// recupero di tutti i parametri macchina su file...
// ritorna 0 se gestione finita
unsigned char ucLoadCustomLanguage_Stati(void){
xdata int i,stato_riga_custom_language;
xdata static unsigned int uiNumRecordString20,uiNumRecordString40;
//xdata unsigned char *pc;
	switch (handleSaveParams.stato){
		case enumSP_idle:
			return 0;
		case enumSP_initVdip:
			uiNumRecordString20=0;
			uiNumRecordString40=0;
			handleSaveParams.exitCode=all_ok_vdip;
			uiRequestSuspendAnybusOp(1);
			if (!uiHasBeenAckRequestSuspendAnybusOp())
				break;
			vInitVdip();
			handleSaveParams.stato=enumSP_handleVdip;
			break;
		case enumSP_handleVdip:
			if (ucHandleVdipStati()){
				handleSaveParams.stato=enumSP_openFile;
			}
			break;
		case enumSP_openFile:
			vSendVdipString("DIR az4181L.ini");
			vReceiveVdipString();// elimino la riga vuota
			vReceiveVdipString();// riga coi dati
			// se il file non esiste, continuo col prossimo
			for (i=0;ucStringRx_Vdip[i];i++){
				if (strncmp(ucStringRx_Vdip+i,"FAILED",6)==0){
					handleSaveParams.exitCode=err_vdip_file_notexists;
					handleSaveParams.stato=enumSP_end;
					return 1;
				}
			}


		// determino la lunghezza del file
			hw.ucString[0]='0';
			hw.ucString[1]='x';
			handleSaveParams.iStructSize=2;
			// vado alla fine della stringa
			for (i=0;ucStringRx_Vdip[i];i++){
			}
			// arretro per trovare il carattere '$'
			while(i>=1){
				i--;
				if (ucStringRx_Vdip[i]=='$'){
					if (isxdigit(ucStringRx_Vdip[i+1])){
						hw.ucString[handleSaveParams.iStructSize++]  =ucStringRx_Vdip[i+1];
						if (isxdigit(ucStringRx_Vdip[i+2])){
							hw.ucString[handleSaveParams.iStructSize++]=ucStringRx_Vdip[i+2];
						}
					}
				}
			}
			hw.ucString[handleSaveParams.iStructSize]=0;
			sscanf(hw.ucString,"%X",&handleSaveParams.uiLenOfFile);
			if (handleSaveParams.uiLenOfFile==0){
				handleSaveParams.exitCode=err_vdip_file_is_empty;
				handleSaveParams.stato=enumSP_end;
				return 1;
			}


			vReceiveVdipString();// elimino il prompt
			// adesso apro il file di salvataggio parametri...
			vSendVdipString("OPR az4181L.ini");
			vReceiveVdipString();
			handleSaveParams.stato=enumSP_openFile_ctrlErr;
			break;
		case enumSP_openFile_ctrlErr:
			if (ucStringRx_Vdip[0]==0){
				handleSaveParams.exitCode=err_vdip_openfile_for_reading;
				handleSaveParams.stato=enumSP_end;
				return 1;
			}
			else{
				// se bad command, errore!!!
				for (handleSaveParams.j=0;ucStringRx_Vdip[handleSaveParams.j];handleSaveParams.j++){
					if ( (strncmp(ucStringRx_Vdip+handleSaveParams.j,"File",4)==0)
						||(strncmp(ucStringRx_Vdip+handleSaveParams.j,"Disk",4)==0)
						||(strncmp(ucStringRx_Vdip+handleSaveParams.j,"Read",4)==0)
						||(strncmp(ucStringRx_Vdip+handleSaveParams.j,"Fail",4)==0)
						) {
						handleSaveParams.exitCode=err_vdip_diskerr_while_reading;
						handleSaveParams.stato=enumSP_end;
						return 1;
					}
				}
			}
			handleSaveParams.stato=enumSP_initLoop;
			break;
		case enumSP_initLoop:
			handleSaveParams.stato=enumSP_doLoop;
			// inizializzo la routine di lettura bytes da vdip
			v_init_read_bytes_from_vdip();
			break;
		case enumSP_doLoop:
			// leggo una linea, filtrando il primo ed il secondo doppio apice
			stato_riga_custom_language=0;
			hw.ucString[0]=0;
			handleSaveParams.j=0;
			for (i=0;i<100;i++){
				if (handleSaveParams.uiLenOfFile==0){
					handleSaveParams.stato=enumSP_endOk;
					return 1;
				}


				uc_the_byte_read_from_vdip=uc_read_a_byte_from_vdip(handleSaveParams.uiLenOfFile);
				handleSaveParams.uiLenOfFile--;
				if (uc_the_byte_read_from_vdip=='\n'){
					break;
				}


				switch(stato_riga_custom_language){
					case 0:
						if (uc_the_byte_read_from_vdip=='"'){
							stato_riga_custom_language=1;
							break;
						}
						break;
					case 1:
						if (uc_the_byte_read_from_vdip=='"'){
							stato_riga_custom_language=2;
							break;
						}
						if (handleSaveParams.j<sizeof(hw.ucString)-1){
							hw.ucString[handleSaveParams.j++]=uc_the_byte_read_from_vdip;
						}
						break;
					case 2:
					default:
						break;
				}
				// se sono arrivato alla fine del file, esco
				if (handleSaveParams.uiLenOfFile==0){
					break;
				}
			}
			// metto sempre e comunque il tappo alla stringa
			hw.ucString[handleSaveParams.j]=0;
			if (handleSaveParams.j==0){
				handleSaveParams.stato=enumSP_endOk;
				return 1;
			}
			if (strlen(hw.ucString)>40){
				handleSaveParams.exitCode=err_vdip_bad_language_row_format;
				handleSaveParams.stato=enumSP_end;
				return 1;
			}
			if (strlen(hw.ucString)>20){
				pWrite_custom_string40(uiNumRecordString40,hw.ucString);
				uiNumRecordString40++;

			}
			else {
				pWrite_custom_string20(uiNumRecordString20,hw.ucString);
				uiNumRecordString20++;
			}


			break;
		case enumSP_endOk:
			handleSaveParams.exitCode=all_ok_vdip;
			handleSaveParams.stato=enumSP_end;
			break;
		case enumSP_end:
		default:
			uiRequestSuspendAnybusOp(0);
			vSendVdipString("CLF az4181L.ini");
			vReceiveVdipString();
			handleSaveParams.stato=enumSP_idle;
			break;


	}
	return 1;


}






xdata static unsigned char ucIAmSavingMachineParametersOnUsb,ucLivingSaveParams,ucStatoSaving;


unsigned char ucHW_LoadCodiciProdotto(void){

	#define defLOADCODICE_button_Title_row 6
	#define defLOADCODICE_button_Title_col ((defLcdWidthX_pixel-10*16)/2)

	#define defDistanceRowBetweenPuls_LOADCODICE 40

	#define defLOADCODICE_button_Msg_row (defLOADCODICE_button_Title_row+64)
	#define defLOADCODICE_button_Msg_col 4

	#define defLOADCODICE_button_Status_row (defLOADCODICE_button_Msg_row+defDistanceRowBetweenPuls_LOADCODICE)
	#define defLOADCODICE_button_Status_col 4


	#define defLOADCODICE_button_Esc_row (defLcdWidthY_pixel-24)
	#define defLOADCODICE_button_Esc_col (defLcdWidthX_pixel-4*16-8)

	#define defLPM_Password 0


	// pulsanti della finestra
	typedef enum {
			enum_LOADCODICE_Title=0,
			enum_LOADCODICE_Status,
			enum_LOADCODICE_Msg,

			enum_LOADCODICE_Sx,
			enum_LOADCODICE_Up,
			enum_LOADCODICE_Cut,
			enum_LOADCODICE_Esc,
			enum_LOADCODICE_Dn,
			enum_LOADCODICE_Dx,

			enum_LOADCODICE_NumOfButtons
		}enum_LOADCODICE_Buttons;
	xdata unsigned char i;
	xdata int idxParametro;
	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:

			idxParametro=defGetWinParam(defLPM_Password);
			// visualizzata finestra password non corretta
			if (idxParametro==201){
				sLCD.ucPasswordChecked=0;
				// chiamo il main menu...
				vJumpToWindow(enumWinId_MainMenu);
			}
			// check password???
			else if (idxParametro==200){
				if (ucVerifyPasswordCorrect(hw.ucStringNumKeypad_out)){
					sLCD.ucPasswordChecked=1;
					vSetStatusCurrentWindow(enumWindowStatus_Active);
					defSetWinParam(defLPM_Password,0);
					return 2;
				}
				// visualizzo la finestra: password non corretta
				// numero msg da visualizzare: 1
				ucSetWindowParam(enumWinId_Info,0, 1);
				// messaggio da visualizzare: password bad
				ucSetWindowParam(enumWinId_Info,1, enumStr20_bad_password);
				// messaggio: password KO
				defSetWinParam(defLPM_Password,201);
			    // chiamo la finestra info
		        ucCallWindow(enumWinId_Info);
				return 2;
			}
			defSetWinParam(defLPM_Password,0);
			vSetStatusCurrentWindow(enumWindowStatus_Active);


			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enum_LOADCODICE_NumOfButtons;i++)
				ucCreateTheButton(i);
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			//vInitMachineParsOnFile_Stati();
			ucEnableStartLavorazione(0);
			ucIAmSavingMachineParametersOnUsb=0;
			ucLivingSaveParams=0;
			ucStatoSaving=0;
			ucMachineParsStatiGranted=0;
			// ogni 100ms rinfresco la pagina...
			ucLcdRefreshTimeoutEnable(100);
			defSetWinParam(defLPM_Password,0);

			return 2;
		case enumWindowStatus_Active:
			// se ancora non � stata verificata la password, passo a chiederla...
			if (!sLCD.ucPasswordChecked){
				vSetNumK_to_ask_password();
				defSetWinParam(defLPM_Password,200);
				// chiamo il numeric keypad
				ucCallWindow(enumWinId_Num_Keypad);
				// indico di rinfrescare la finestra
				return 2;
			}
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enum_LOADCODICE_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enum_LOADCODICE_Esc:
							if (ucStatoSaving!=1){
								ucLcdRefreshTimeoutEnable(0);
								ucEnableStartLavorazione(1);
								ucReturnFromWindow();
								uiRequestSuspendAnybusOp(0);
								return 2;
							}
							break;
						case enum_LOADCODICE_Msg:
							if (ucStatoSaving!=1){
								ucStatoSaving=1;
								ucMachineParsStatiGranted=0;
								ucIAmSavingMachineParametersOnUsb=1;
							}
							break;

					}
				}
			}

			//strcpy(hw.ucString,"MAIN MENU");
			vStringLangCopy(hw.ucString,enumStr20_LoadCodiciProdotto);
			//ucPrintStaticButton(hw.ucString,defLOADCODICE_button_Title_row ,defLOADCODICE_button_Title_col,enumFontMedium,enum_LOADCODICE_Title,defLCD_Color_LightGreen);
			ucPrintTitleButton(hw.ucString,defLOADCODICE_button_Title_row ,defLOADCODICE_button_Title_col,enumFontMedium,enum_LOADCODICE_Title,defLCD_Color_Trasparente,1);

			if (ucStatoSaving==1){
				if (!ucMachineParsStatiGranted){
					handleSaveParams.fEstimatedDonePerc=0;
					uiRequestSuspendAnybusOp(1);
					if (uiHasBeenAckRequestSuspendAnybusOp()){
						vInitMachineParsOnFile_Stati();
						ucMachineParsStatiGranted=1;
					}
				}
				else{
					for (i=0;i<3;i++){
						ucIAmSavingMachineParametersOnUsb=ucLoadCodiciProdottoOnFile_Stati(0);
						if (!ucIAmSavingMachineParametersOnUsb)
							break;
					}
					if (!ucIAmSavingMachineParametersOnUsb){
						ucStatoSaving=2;
						ucMachineParsStatiGranted=0;
					}
				}
				if (ucStatoSaving==1){
					ucLivingSaveParams++;
					ucLivingSaveParams&=0x3;

					//"RESTORING %3i%% %c",
					sprintf(hw.ucString,pucStringLang(enumStr20_RestoreProductCodes),(int)fSaveMachineParsOnFile_GetEstimatedDonePerc(),ucLivingSaveParamsChars[ucLivingSaveParams]);

					ucPrintTitleButton(hw.ucString,defLOADCODICE_button_Msg_row,defLOADCODICE_button_Msg_col,enumFontMedium,enum_LOADCODICE_Msg,defLCD_Color_Trasparente,1);
				}
			}
			if (ucStatoSaving!=1){
				//strcpy(hw.ucString,"   RESTORE CODES    ");
				vStringLangCopy(hw.ucString,enumStr20_RestoreCodes);

				ucPrintStaticButton(hw.ucString,defLOADCODICE_button_Msg_row ,defLOADCODICE_button_Msg_col,enumFontMedium,enum_LOADCODICE_Msg,defLCD_Color_Trasparente);
				if (ucStatoSaving!=0){
					if (iSaveMachineParsOnFile_GetExitCode()==all_ok_vdip){
						//strcpy(hw.ucString,"Restore ends OK");
						vStringLangCopy(hw.ucString,enumStr20_RestoreCodesEndsOk);
					}
					else{
						//sprintf(hw.ucString,"Error: %i",iSaveMachineParsOnFile_GetExitCode());
						sprintf(hw.ucString,pucStringLang(enumStr20_ErrorOnProductCodes),iSaveMachineParsOnFile_GetExitCode());
					}
					ucPrintTitleButton(hw.ucString,defLOADCODICE_button_Status_row,defLOADCODICE_button_Status_col,enumFontMedium,enum_LOADCODICE_Status,defLCD_Color_Yellow,1);
				}
			}



			// routine che permette di aggiungere ad una finestra i bottoni standard sx, dx, up, dn, esc, cut
			vAddStandardButtons(enum_LOADCODICE_Sx);


			return 1;
	}//switch(uiGetStatusCurrentWindow())

}



unsigned char ucHW_ResetCodiciProdotto(void){
	#define defResetProductCode_button_Title_row 6
	#define defResetProductCode_button_Title_col ((defLcdWidthX_pixel-10*16)/2)

	#define defDistanceRowBetweenPuls_ResetProductCode 40

	#define defResetProductCode_button_Msg_row (defResetProductCode_button_Title_row+64)
	#define defResetProductCode_button_Msg_col 4

	#define defResetProductCode_button_Status_row (defResetProductCode_button_Msg_row+defDistanceRowBetweenPuls_ResetProductCode)
	#define defResetProductCode_button_Status_col 4


	#define defResetProductCode_button_Esc_row (defLcdWidthY_pixel-24)
	#define defResetProductCode_button_Esc_col (defLcdWidthX_pixel-4*16-8)

	#define defLPM_Password 0


	// pulsanti della finestra
	typedef enum {
			enum_ResetProductCode_Title=0,
			enum_ResetProductCode_Status,
			enum_ResetProductCode_Msg,

			enum_ResetProductCode_Sx,
			enum_ResetProductCode_Up,
			enum_ResetProductCode_Cut,
			enum_ResetProductCode_Esc,
			enum_ResetProductCode_Dn,
			enum_ResetProductCode_Dx,

			enum_ResetProductCode_NumOfButtons
		}enum_ResetProductCode_Buttons;
	xdata unsigned char i;
	static xdata unsigned char ucDoReboot;
	xdata int idxParametro;
	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:

			idxParametro=defGetWinParam(defLPM_Password);
			// visualizzata finestra password non corretta
			if (idxParametro==201){
				sLCD.ucPasswordChecked=0;
				// chiamo il main menu...
				vJumpToWindow(enumWinId_MainMenu);
			}
			// check password???
			else if (idxParametro==200){
				if (ucVerifyPasswordCorrect(hw.ucStringNumKeypad_out)){
					sLCD.ucPasswordChecked=1;
					vSetStatusCurrentWindow(enumWindowStatus_Active);
					defSetWinParam(defLPM_Password,0);
					return 2;
				}
				// visualizzo la finestra: password non corretta
				// numero msg da visualizzare: 1
				ucSetWindowParam(enumWinId_Info,0, 1);
				// messaggio da visualizzare: password bad
				ucSetWindowParam(enumWinId_Info,1, enumStr20_bad_password);
				// messaggio: password KO
				defSetWinParam(defLPM_Password,201);
			    // chiamo la finestra info
		        ucCallWindow(enumWinId_Info);
				return 2;
			}
			else if (idxParametro==100){
				if (uiGetWindowParam(enumWinId_YesNo,defWinParam_YESNO_Answer)){
					ucDoReboot=1;
                    // empty the product codes list...
                    v_empty_product_codes_list();
                    // close the product file list
                    v_close_reg_file_list();
				}
			}

			defSetWinParam(defLPM_Password,0);
			vSetStatusCurrentWindow(enumWindowStatus_Active);


			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enum_ResetProductCode_NumOfButtons;i++)
				ucCreateTheButton(i);
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			//vInitMachineParsOnFile_Stati();
			ucEnableStartLavorazione(0);
			ucIAmSavingMachineParametersOnUsb=0;
			ucLivingSaveParams=0;
			ucStatoSaving=0;
			// ogni 100ms rinfresco la pagina...
			ucLcdRefreshTimeoutEnable(100);
			defSetWinParam(defLPM_Password,0);
			ucDoReboot=0;

			return 2;
		case enumWindowStatus_Active:
			// se ancora non � stata verificata la password, passo a chiederla...
			if (!sLCD.ucPasswordChecked){
				vSetNumK_to_ask_password();
				defSetWinParam(defLPM_Password,200);
				// chiamo il numeric keypad
				ucCallWindow(enumWinId_Num_Keypad);
				// indico di rinfrescare la finestra
				return 2;
			}
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enum_ResetProductCode_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enum_ResetProductCode_Esc:
							ucLcdRefreshTimeoutEnable(0);
							ucEnableStartLavorazione(1);
							ucReturnFromWindow();
							return 2;
						case enum_ResetProductCode_Msg:
							vStringLangCopy(hw.ucStringNumKeypad_in,enumStr20_DeleteCode);
							defSetWinParam(defLPM_Password,100);
							hw.ucStringNumKeypad_out[0]=0;
							// impostazione del parametro 0 al valore 0--> visualizza pulsanti yes e no
							ucSetWindowParam(enumWinId_YesNo,def_WinYesNo_Param_TipoPulsante, def_WinYesNo_PUlsantiYesNo);
							// visualizzo yes/no...
							ucCallWindow(enumWinId_YesNo);
							return 2;

					}
				}
			}

			//strcpy(hw.ucString,"MAIN MENU");
			vStringLangCopy(hw.ucString,enumStr20_ResetCodiciProdotto);
			//ucPrintStaticButton(hw.ucString,defResetProductCode_button_Title_row ,defResetProductCode_button_Title_col,enumFontMedium,enum_ResetProductCode_Title,defLCD_Color_LightGreen);
			ucPrintTitleButton(hw.ucString,defResetProductCode_button_Title_row ,defResetProductCode_button_Title_col,enumFontMedium,enum_ResetProductCode_Title,defLCD_Color_Trasparente,1);
			if (ucDoReboot){
				if (++ucDoReboot==30){
                    v_reset_az4186();
				}
				vStringLangCopy(hw.ucString,enumStr20_Reboot);
				//strcpy(hw.ucString,"REBOOT...");
				ucPrintTitleButton(hw.ucString,defResetProductCode_button_Msg_row ,defResetProductCode_button_Msg_col,enumFontMedium,enum_ResetProductCode_Msg,defLCD_Color_Trasparente,1);
			}
			else{
				vStringLangCopy(hw.ucString,enumStr20_ClearAllProdCodes);
				//strcpy(hw.ucString,"CLEAR ALL PROD.CODES");
				ucPrintStaticButton(hw.ucString,defResetProductCode_button_Msg_row ,defResetProductCode_button_Msg_col,enumFontMedium,enum_ResetProductCode_Msg,defLCD_Color_Trasparente);
			}

			// routine che permette di aggiungere ad una finestra i bottoni standard sx, dx, up, dn, esc, cut
			vAddStandardButtons(enum_ResetProductCode_Sx);


			return 1;

	}//switch(uiGetStatusCurrentWindow())
}



unsigned char ucHW_LoadCustomLanguage(void){

	#define defLoadCustomLang_button_Title_row 6
	#define defLoadCustomLang_button_Title_col ((defLcdWidthX_pixel-10*16)/2)

	#define defDistanceRowBetweenPuls_LoadCustomLang 40

	#define defLoadCustomLang_button_Msg_row (defLoadCustomLang_button_Title_row+64)
	#define defLoadCustomLang_button_Msg_col 4

	#define defLoadCustomLang_button_Status_row (defLoadCustomLang_button_Msg_row+defDistanceRowBetweenPuls_LoadCustomLang)
	#define defLoadCustomLang_button_Status_col 4


	#define defLoadCustomLang_button_Esc_row (defLcdWidthY_pixel-24)
	#define defLoadCustomLang_button_Esc_col (defLcdWidthX_pixel-4*16-8)

	#define defLPM_Password 0


	// pulsanti della finestra
	typedef enum {
			enum_LoadCustomLang_Title=0,
			enum_LoadCustomLang_Status,
			enum_LoadCustomLang_Msg,

			enum_LoadCustomLang_Sx,
			enum_LoadCustomLang_Up,
			enum_LoadCustomLang_Cut,
			enum_LoadCustomLang_Esc,
			enum_LoadCustomLang_Dn,
			enum_LoadCustomLang_Dx,

			enum_LoadCustomLang_NumOfButtons
		}enum_LoadCustomLang_Buttons;
	xdata unsigned char i;
	xdata int idxParametro;
	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:

			idxParametro=defGetWinParam(defLPM_Password);
			// visualizzata finestra password non corretta
			if (idxParametro==201){
				sLCD.ucPasswordChecked=0;
				// chiamo il main menu...
				vJumpToWindow(enumWinId_MainMenu);
			}
			// check password???
			else if (idxParametro==200){
				if (ucVerifyPasswordCorrect(hw.ucStringNumKeypad_out)){
					sLCD.ucPasswordChecked=1;
					vSetStatusCurrentWindow(enumWindowStatus_Active);
					defSetWinParam(defLPM_Password,0);
					return 2;
				}
				// visualizzo la finestra: password non corretta
				// numero msg da visualizzare: 1
				ucSetWindowParam(enumWinId_Info,0, 1);
				// messaggio da visualizzare: password bad
				ucSetWindowParam(enumWinId_Info,1, enumStr20_bad_password);
				// messaggio: password KO
				defSetWinParam(defLPM_Password,201);
			    // chiamo la finestra info
		        ucCallWindow(enumWinId_Info);
				return 2;
			}
			defSetWinParam(defLPM_Password,0);
			vSetStatusCurrentWindow(enumWindowStatus_Active);


			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enum_LoadCustomLang_NumOfButtons;i++)
				ucCreateTheButton(i);
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			//vInitMachineParsOnFile_Stati();
			ucEnableStartLavorazione(0);
			ucIAmSavingMachineParametersOnUsb=0;
			ucLivingSaveParams=0;
			ucStatoSaving=0;
			ucMachineParsStatiGranted=0;
			// ogni 100ms rinfresco la pagina...
			ucLcdRefreshTimeoutEnable(100);
			defSetWinParam(defLPM_Password,0);

			return 2;
		case enumWindowStatus_Active:
			// se ancora non � stata verificata la password, passo a chiederla...
			if (!sLCD.ucPasswordChecked){
				vSetNumK_to_ask_password();
				defSetWinParam(defLPM_Password,200);
				// chiamo il numeric keypad
				ucCallWindow(enumWinId_Num_Keypad);
				// indico di rinfrescare la finestra
				return 2;
			}
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enum_LoadCustomLang_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enum_LoadCustomLang_Esc:
							if (ucStatoSaving!=1){
								ucLcdRefreshTimeoutEnable(0);
								ucEnableStartLavorazione(1);
								ucReturnFromWindow();
								uiRequestSuspendAnybusOp(0);
								return 2;
							}
							break;
						case enum_LoadCustomLang_Msg:
							if (ucStatoSaving!=1){
								ucStatoSaving=1;
								ucMachineParsStatiGranted=0;
								ucIAmSavingMachineParametersOnUsb=1;
							}
							break;

					}
				}
			}

			//strcpy(hw.ucString,"MAIN MENU");
			vStringLangCopy(hw.ucString,enumStr20_LoadCustomLanguage);
			//ucPrintStaticButton(hw.ucString,defLoadCustomLang_button_Title_row ,defLoadCustomLang_button_Title_col,enumFontMedium,enum_LoadCustomLang_Title,defLCD_Color_LightGreen);
			ucPrintTitleButton(hw.ucString,defLoadCustomLang_button_Title_row ,defLoadCustomLang_button_Title_col,enumFontMedium,enum_LoadCustomLang_Title,defLCD_Color_Trasparente,1);

			if (ucStatoSaving==1){
				if (!ucMachineParsStatiGranted){
					handleSaveParams.fEstimatedDonePerc=0;
					uiRequestSuspendAnybusOp(1);
					if (uiHasBeenAckRequestSuspendAnybusOp()){
						vInitMachineParsOnFile_Stati();
						ucMachineParsStatiGranted=1;
					}
				}
				else{
					for (i=0;i<3;i++){
						ucIAmSavingMachineParametersOnUsb=ucLoadCustomLanguage_Stati();
						if (!ucIAmSavingMachineParametersOnUsb)
							break;
					}
					if (!ucIAmSavingMachineParametersOnUsb){
						ucStatoSaving=2;
						ucMachineParsStatiGranted=0;
					}
				}
				if (ucStatoSaving==1){
					ucLivingSaveParams++;
					ucLivingSaveParams&=0x3;

					//sprintf(hw.ucString,"LOADING %c",ucLivingSaveParamsChars[ucLivingSaveParams]);
					sprintf(hw.ucString,pucStringLang(enumStr20_LoadingParams),ucLivingSaveParamsChars[ucLivingSaveParams]);
					ucPrintTitleButton(hw.ucString,defLoadCustomLang_button_Msg_row,defLoadCustomLang_button_Msg_col,enumFontMedium,enum_LoadCustomLang_Msg,defLCD_Color_Trasparente,1);
				}
			}
			if (ucStatoSaving!=1){
				vStringLangCopy(hw.ucString,enumStr20_LoadLanguage);
				//strcpy(hw.ucString,"    LOAD LANGUAGE   ");
				ucPrintStaticButton(hw.ucString,defLoadCustomLang_button_Msg_row ,defLoadCustomLang_button_Msg_col,enumFontMedium,enum_LoadCustomLang_Msg,defLCD_Color_Trasparente);
				if (ucStatoSaving!=0){
					if (iSaveMachineParsOnFile_GetExitCode()==all_ok_vdip){
						vStringLangCopy(hw.ucString,enumStr20_LoadLanguageEndsOk);
						//strcpy(hw.ucString,"Load ends OK");
					}
					else{
						sprintf(hw.ucString,pucStringLang(enumStr20_ErrorOnProductCodes),iSaveMachineParsOnFile_GetExitCode());
					}
					ucPrintTitleButton(hw.ucString,defLoadCustomLang_button_Status_row,defLoadCustomLang_button_Status_col,enumFontMedium,enum_LoadCustomLang_Status,defLCD_Color_Yellow,1);
				}
			}



			// routine che permette di aggiungere ad una finestra i bottoni standard sx, dx, up, dn, esc, cut
			vAddStandardButtons(enum_LoadCustomLang_Sx);


			return 1;
	}//switch(uiGetStatusCurrentWindow())

}












unsigned char ucHW_LoadParsMac(void){

	#define defLOADPARMAC_button_Title_row 6
	#define defLOADPARMAC_button_Title_col ((defLcdWidthX_pixel-10*16)/2)

	#define defDistanceRowBetweenPuls_LOADPARMAC 40

	#define defLOADPARMAC_button_Msg_row (defLOADPARMAC_button_Title_row+64)
	#define defLOADPARMAC_button_Msg_col 4

	#define defLOADPARMAC_button_Status_row (defLOADPARMAC_button_Msg_row+defDistanceRowBetweenPuls_LOADPARMAC)
	#define defLOADPARMAC_button_Status_col 4


	#define defLOADPARMAC_button_Esc_row (defLcdWidthY_pixel-24)
	#define defLOADPARMAC_button_Esc_col (defLcdWidthX_pixel-4*16-8)

	#define defLPM_Password 0


	// pulsanti della finestra
	typedef enum {
			enum_LOADPARMAC_Title=0,
			enum_LOADPARMAC_Status,
			enum_LOADPARMAC_Msg,

			enum_LOADPARMAC_Sx,
			enum_LOADPARMAC_Up,
			enum_LOADPARMAC_Cut,
			enum_LOADPARMAC_Esc,
			enum_LOADPARMAC_Dn,
			enum_LOADPARMAC_Dx,

			enum_LOADPARMAC_NumOfButtons
		}enum_LOADPARMAC_Buttons;
	xdata unsigned char i;
	xdata int idxParametro;
	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:

			idxParametro=defGetWinParam(defLPM_Password);
			// visualizzata finestra password non corretta
			if (idxParametro==201){
				sLCD.ucPasswordChecked=0;
				// chiamo il main menu...
				vJumpToWindow(enumWinId_MainMenu);
			}
			// check password???
			else if (idxParametro==200){
				if (ucVerifyPasswordCorrect(hw.ucStringNumKeypad_out)){
					sLCD.ucPasswordChecked=1;
					vSetStatusCurrentWindow(enumWindowStatus_Active);
					defSetWinParam(defLPM_Password,0);
					return 2;
				}
				// visualizzo la finestra: password non corretta
				// numero msg da visualizzare: 1
				ucSetWindowParam(enumWinId_Info,0, 1);
				// messaggio da visualizzare: password bad
				ucSetWindowParam(enumWinId_Info,1, enumStr20_bad_password);
				// messaggio: password KO
				defSetWinParam(defLPM_Password,201);
			    // chiamo la finestra info
		        ucCallWindow(enumWinId_Info);
				return 2;
			}
			defSetWinParam(defLPM_Password,0);
			vSetStatusCurrentWindow(enumWindowStatus_Active);


			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enum_LOADPARMAC_NumOfButtons;i++)
				ucCreateTheButton(i);
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			//vInitMachineParsOnFile_Stati();
			ucEnableStartLavorazione(0);
			ucIAmSavingMachineParametersOnUsb=0;
			ucLivingSaveParams=0;
			ucStatoSaving=0;
			ucMachineParsStatiGranted=0;
			// ogni 100ms rinfresco la pagina...
			ucLcdRefreshTimeoutEnable(100);
			defSetWinParam(defLPM_Password,0);

			return 2;
		case enumWindowStatus_Active:
			// se ancora non � stata verificata la password, passo a chiederla...
			if (!sLCD.ucPasswordChecked){
				vSetNumK_to_ask_password();
				defSetWinParam(defLPM_Password,200);
				// chiamo il numeric keypad
				ucCallWindow(enumWinId_Num_Keypad);
				// indico di rinfrescare la finestra
				return 2;
			}
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enum_LOADPARMAC_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enum_LOADPARMAC_Esc:
							if (ucStatoSaving!=1){
								ucLcdRefreshTimeoutEnable(0);
								ucEnableStartLavorazione(1);
								ucReturnFromWindow();
								uiRequestSuspendAnybusOp(0);
								return 2;
							}
							break;
						case enum_LOADPARMAC_Msg:
							if (ucStatoSaving!=1){
								ucStatoSaving=1;
								ucMachineParsStatiGranted=0;
								ucIAmSavingMachineParametersOnUsb=1;
							}
							break;

					}
				}
			}

			//strcpy(hw.ucString,"MAIN MENU");
			vStringLangCopy(hw.ucString,enumStr20_LOADPARMAC);
			//ucPrintStaticButton(hw.ucString,defLOADPARMAC_button_Title_row ,defLOADPARMAC_button_Title_col,enumFontMedium,enum_LOADPARMAC_Title,defLCD_Color_LightGreen);
			ucPrintTitleButton(hw.ucString,defLOADPARMAC_button_Title_row ,defLOADPARMAC_button_Title_col,enumFontMedium,enum_LOADPARMAC_Title,defLCD_Color_Trasparente,1);

			if (ucStatoSaving==1){
				if (!ucMachineParsStatiGranted){
					handleSaveParams.fEstimatedDonePerc=0;
					uiRequestSuspendAnybusOp(1);
					if (uiHasBeenAckRequestSuspendAnybusOp()){
						vInitMachineParsOnFile_Stati();
						ucMachineParsStatiGranted=1;
					}
				}
				else{
					for (i=0;i<3;i++){
						ucIAmSavingMachineParametersOnUsb=ucLoadParMacOnFile_Stati(0);
						if (!ucIAmSavingMachineParametersOnUsb)
							break;
					}
					if (!ucIAmSavingMachineParametersOnUsb){
						// se i parametri sono stati caricati correttamente, allora
						// li salvo in flash
						if (iSaveMachineParsOnFile_GetExitCode()==all_ok_vdip){
							ValidateMacParms();
							ucSaveMacParms=1;
						}
						// se i parametri NON sono stati caricati correttamente, allora
						// li recupero dalla flash
						else{
							vLoadMacParms();
						}
						ucStatoSaving=2;
						ucMachineParsStatiGranted=0;
					}
				}
				if (ucStatoSaving==1){
					ucLivingSaveParams++;
					ucLivingSaveParams&=0x3;

					//sprintf(hw.ucString,"RESTORING %c",ucLivingSaveParamsChars[ucLivingSaveParams]);
					sprintf(hw.ucString,pucStringLang(enumStr20_Restoring),ucLivingSaveParamsChars[ucLivingSaveParams]);
					ucPrintTitleButton(hw.ucString,defLOADPARMAC_button_Msg_row,defLOADPARMAC_button_Msg_col,enumFontMedium,enum_LOADPARMAC_Msg,defLCD_Color_Trasparente,1);
				}
			}
			if (ucStatoSaving!=1){
				//strcpy(hw.ucString,"   RESTORE PARAMS   ");
				vStringLangCopy(hw.ucString,enumStr20_RestoreParams);

				ucPrintStaticButton(hw.ucString,defLOADPARMAC_button_Msg_row ,defLOADPARMAC_button_Msg_col,enumFontMedium,enum_LOADPARMAC_Msg,defLCD_Color_Trasparente);
				if (ucStatoSaving!=0){
					if (iSaveMachineParsOnFile_GetExitCode()==all_ok_vdip){
						//strcpy(hw.ucString,"Restore ends OK");
						vStringLangCopy(hw.ucString,enumStr20_RestoreParamsEndsOk);
					}
					else{
						sprintf(hw.ucString,pucStringLang(enumStr20_ErrorOnProductCodes),iSaveMachineParsOnFile_GetExitCode());
					}
					ucPrintTitleButton(hw.ucString,defLOADPARMAC_button_Status_row,defLOADPARMAC_button_Status_col,enumFontMedium,enum_LOADPARMAC_Status,defLCD_Color_Yellow,1);
				}
			}



			// routine che permette di aggiungere ad una finestra i bottoni standard sx, dx, up, dn, esc, cut
			vAddStandardButtons(enum_LOADPARMAC_Sx);


			return 1;
	}//switch(uiGetStatusCurrentWindow())

}
















// salvataggio dei parametri della spiralatrice...
unsigned char ucHW_SaveParams(void){

	#define defSAVEPARAMS_button_Title_row 6
	#define defSAVEPARAMS_button_Title_col ((defLcdWidthX_pixel-10*16)/2)

	#define defDistanceRowBetweenPuls_SAVEPARAMS 40

	#define defSAVEPARAMS_button_Msg_row (defSAVEPARAMS_button_Title_row+64)
	#define defSAVEPARAMS_button_Msg_col 4

	#define defSAVEPARAMS_button_Status_row (defSAVEPARAMS_button_Msg_row+defDistanceRowBetweenPuls_SAVEPARAMS)
	#define defSAVEPARAMS_button_Status_col 4


	#define defSAVEPARAMS_button_Esc_row (defLcdWidthY_pixel-24)
	#define defSAVEPARAMS_button_Esc_col (defLcdWidthX_pixel-4*16-8)


	// pulsanti della finestra
	typedef enum {
			enum_SAVEPARAMS_Title=0,
			enum_SAVEPARAMS_Status,
			enum_SAVEPARAMS_Msg,

			enum_SAVEPARAMS_Sx,
			enum_SAVEPARAMS_Up,
			enum_SAVEPARAMS_Cut,
			enum_SAVEPARAMS_Esc,
			enum_SAVEPARAMS_Dn,
			enum_SAVEPARAMS_Dx,

			enum_SAVEPARAMS_NumOfButtons
		}enum_SAVEPARAMS_Buttons;
	xdata unsigned char i,j,len;
	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
            i=0;
			while(i<enum_SAVEPARAMS_NumOfButtons){
				ucCreateTheButton(i);
                i++;
            }
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			ucEnableStartLavorazione(0);
			ucStatoSaving=0;
			ucMachineParsStatiGranted=0;
			ucLivingSaveParams=0;
			// ogni 100ms rinfresco la pagina...
			ucLcdRefreshTimeoutEnable(100);

			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
            i=0;
			while(i<enum_SAVEPARAMS_NumOfButtons){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enum_SAVEPARAMS_Esc:
							// non si pu� fermare un salvataggio in corso!
							if (ucStatoSaving==1)
								break;
							ucLcdRefreshTimeoutEnable(0);
							ucEnableStartLavorazione(1);
							ucReturnFromWindow();
							uiRequestSuspendAnybusOp(0);
							return 2;
						case enum_SAVEPARAMS_Msg:
							// inizializzo macchina a stati per salvataggio parametri
							// se non � gi� in corso!
							if (ucStatoSaving!=1){
								ucStatoSaving=1;
								ucMachineParsStatiGranted=0;
								ucIAmSavingMachineParametersOnUsb=1;
							}
							break;

					}
				}
                i++;
			}

			//strcpy(hw.ucString,"MAIN MENU");
			vStringLangCopy(hw.ucString,enumStr20_SaveParams);
			//ucPrintStaticButton(hw.ucString,defSAVEPARAMS_button_Title_row ,defSAVEPARAMS_button_Title_col,enumFontMedium,enum_SAVEPARAMS_Title,defLCD_Color_LightGreen);
			ucPrintTitleButton(hw.ucString,defSAVEPARAMS_button_Title_row ,defSAVEPARAMS_button_Title_col,enumFontMedium,enum_SAVEPARAMS_Title,defLCD_Color_Trasparente,1);

			if (ucStatoSaving==1){
				if (!ucMachineParsStatiGranted){
					handleSaveParams.fEstimatedDonePerc=0;
					uiRequestSuspendAnybusOp(1);
					if (uiHasBeenAckRequestSuspendAnybusOp()){
						vInitMachineParsOnFile_Stati();
						ucMachineParsStatiGranted=1;
					}
				}
				else{
					if (ucIAmSavingMachineParametersOnUsb){
						// eseguo un po' di loop...
                        i=0;
						while (i<4){
							ucIAmSavingMachineParametersOnUsb=ucSaveMachineParsOnFile_Stati(0);
							if (!ucIAmSavingMachineParametersOnUsb)
								break;
                            i++;
						}
					}
					if (!ucIAmSavingMachineParametersOnUsb){
						ucStatoSaving=2;
						ucMachineParsStatiGranted=0;
					}
				}
				if (ucStatoSaving==1){
					ucLivingSaveParams++;
					ucLivingSaveParams&=0x3;
					if (fSaveMachineParsOnFile_GetEstimatedDonePerc()>0.5)
						vStringLangCopy(hw.ucString,enumStr20_Saving);
						//sprintf(hw.ucString,"Saving... ");
					else
						//sprintf(hw.ucString,"Init save... ");
						vStringLangCopy(hw.ucString,enumStr20_InitSave);
					sprintf(hw.ucString,"%s%c",hw.ucString,ucLivingSaveParamsChars[ucLivingSaveParams]);
					ucPrintTitleButton(hw.ucString,defSAVEPARAMS_button_Msg_row,defSAVEPARAMS_button_Msg_col,enumFontMedium,enum_SAVEPARAMS_Msg,defLCD_Color_Trasparente,1);
					sprintf(hw.ucString,"%i%%",(int)fSaveMachineParsOnFile_GetEstimatedDonePerc());
					len=strlen(hw.ucString);
					// numero di caratteri occupati dal fondo giallo
					i=(fSaveMachineParsOnFile_GetEstimatedDonePerc()/100)*20;
					// sempre numero pari di caratteri per evitare jitter nella posizione della cifra delle percentuali
					if (i&1)
						i++;
					// faccio in modo di centrare la scritta nello schermo...
					if (len<i){
						j=(i-len)/2;
						hw.ucString[j]=0;
						while(j){
							hw.ucString[j-1]=' ';
							j--;
						}
						j=(i-len)/2;
						sprintf(hw.ucString+j,"%i%%",(int)fSaveMachineParsOnFile_GetEstimatedDonePerc());
						if (i>j+len){
							i=i-j-len;
							j=strlen(hw.ucString);
							while(i--){
								hw.ucString[j++]=' ';
							}
							hw.ucString[j]=0;
						}
					}
					ucPrintTitleButton(hw.ucString,defSAVEPARAMS_button_Status_row,defSAVEPARAMS_button_Status_col,enumFontMedium,enum_SAVEPARAMS_Msg,defLCD_Color_Yellow,3);
				}
			}
			if (ucStatoSaving!=1){
				if (ucStatoSaving!=0){
					if (iSaveMachineParsOnFile_GetExitCode()==0){
						//strcpy(hw.ucString,"Save ends OK");
						vStringLangCopy(hw.ucString,enumStr20_SaveEndsOk);
					}
					else{
						sprintf(hw.ucString,pucStringLang(enumStr20_ErrorOnProductCodes),iSaveMachineParsOnFile_GetExitCode());
					}
					ucPrintTitleButton(hw.ucString,defSAVEPARAMS_button_Msg_row,defSAVEPARAMS_button_Msg_col,enumFontMedium,enum_SAVEPARAMS_Status,defLCD_Color_Yellow,1);
				}
				//strcpy(hw.ucString,"       Do save      ");
				vStringLangCopy(hw.ucString,enumStr20_DoSave);
				ucPrintStaticButton(hw.ucString,defSAVEPARAMS_button_Status_row,defSAVEPARAMS_button_Status_col,enumFontMedium,enum_SAVEPARAMS_Msg,defLCD_Color_Trasparente);
			}

			// routine che permette di aggiungere ad una finestra i bottoni standard sx, dx, up, dn, esc, cut
			vAddStandardButtons(enum_SAVEPARAMS_Sx);


			return 1;
	}//switch(uiGetStatusCurrentWindow())
}//unsigned char ucHW_SaveParams(void)



// reset delle variabili interessate nella costruzione delle finestre...
void vResetWindows(void){
extern void vInitInputEdge(void);
extern void vInitializeHisFields(TipoStructHandleInserimentoCodice *phis);
extern void vHRE_init(void);
extern void vInitPosizioneLamaTaglio();
extern void vInitListaCodici(void);
extern void vInitBandella(void);
	vInitInputEdge();
	ucHoldMisuraStatica=0;
	// lettura del serial number del box...
	pRead_private_parameter(enum_private_param_box_serial_number,(unsigned char xdata *)&ulBoxSerialNumber);
	ucSuperUser_passwordChecked=0;
	vInitBandella();
	// inizializzo parametro numeric keypad
	memset(&paramsNumK,0,sizeof(paramsNumK));
	// ne approfitto per azzerare il log...
	memset(&production_log,0,sizeof(production_log));
	vInitSensoreTaglio();
	ucEnableStartLavorazione(1);
	vHRE_init();
	vValidateTabFornitori();
    vEnableCalibraLcd(0);
	spiralatrice.PrimoPezzoEliminato=0;
	ucEliminaPrimoPezzo=0;
	vInitListaCodici();


	memset(&hw,0,sizeof(hw));
	memset(&hM,0,sizeof(hM));
	strcpy(hw.ucStringNumKeypad_out,"0.0");
	// password non � ancora stata verificata...
	sLCD.ucPasswordChecked=0;
	// se ero nel menu boccola, devo tornarci...
	if (nvram_struct.ucImAmImMenuBoccola_0x37==0x37){
		vJumpToWindow(enumWinId_CambioBoccola);
	}
	// se prima di spegnere stavo lavorando, devo ritornare nel menu produzione...
	else if (nvram_struct.ucIwasWorkingOnMenuProduzione_0xa5==0xa5){
		vJumpToWindow(enumWinId_setupProduzione);
	}
	else if (pJobsSelected_Jobs->ucNumLavoriInLista)
		vJumpToWindow(enumWinId_ListaLavori);
	else
		vJumpToWindow(enumWinId_listaCodici);
	vForceLcdRefresh();
	vInitPosizioneLamaTaglio();
	vRefreshActiveProductCode();
}


// salvataggio dei parametri della spiralatrice...
unsigned char ucHW_DataOra(void){

	#define defDATAORA_button_Title_row 6
	#define defDATAORA_button_Title_col ((defLcdWidthX_pixel-10*16)/2)

	#define defDistanceRowBetweenPuls_DATAORA 48

	#define defDATAORA_button_Anno_row (defDATAORA_button_Title_row+48)
	#define defDATAORA_button_Anno_col (defLcdWidthX_pixel/2-4*16)

	#define defDATAORA_button_Ora_row (defDATAORA_button_Anno_row+defDistanceRowBetweenPuls_DATAORA)
	#define defDATAORA_button_Ora_col (defLcdWidthX_pixel/2-4*16)

	#define defDATAORA_button_Temp_row (defDATAORA_button_Anno_row+2*defDistanceRowBetweenPuls_DATAORA)
	#define defDATAORA_button_Temp_col (defLcdWidthX_pixel/2-34/2*8)


	#define defDATAORA_button_Esc_row (defLcdWidthY_pixel-24)
	#define defDATAORA_button_Esc_col (defLcdWidthX_pixel-4*16-8)


	#define defWinDataOraModifyNumCampo 0


	// pulsanti della finestra
	typedef enum {
			enum_DATAORA_Title=0,
			enum_DATAORA_Giorno,
			enum_DATAORA_Mese,
			enum_DATAORA_Anno,
			enum_DATAORA_Ora,
			enum_DATAORA_Minuto,
			enum_DATAORA_Secondo,
			enum_DATAORA_BckAnno,
			enum_DATAORA_BckOra,
			enum_DATAORA_Temperatura,
			enum_DATAORA_RtcOK,

			enum_DATAORA_Sx,
			enum_DATAORA_Up,
			enum_DATAORA_Cut,
			enum_DATAORA_Esc,
			enum_DATAORA_Dn,
			enum_DATAORA_Dx,

			enum_DATAORA_NumOfButtons
		}enum_DATAORA_Buttons;
	xdata unsigned char i,idxParametro,j;
	xdata int iMyInt;
	typedef struct _TipoStructRtcNumK{
		float fMin,fMax;
		unsigned char title[21];
	}TipoStructRtcNumK;
	static code TipoStructRtcNumK rtcNumK[]={
		{1,31,"Day"},
		{1,12,"Month"},
		{0,99,"Year"},
		{0,59,"Hour"},
		{0,59,"Min"},
		{0,59,"Sec"}
	};

	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			iMyInt=atoi(hw.ucStringNumKeypad_out);
			idxParametro=defGetWinParam(defWinDataOraModifyNumCampo);
			if ((idxParametro>=0)&&(idxParametro<=5)){
				*(unsigned char*)(&rtc.ucGiorno+idxParametro)=(unsigned char)(iMyInt&0xFF);
				vWriteTimeBytesRTC_DS3231();
			}

			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			for (i=0;i<enum_DATAORA_NumOfButtons;i++)
				ucCreateTheButton(i);
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			// ogni 200ms rinfresco la pagina...
			ucLcdRefreshTimeoutEnable(200);
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enum_DATAORA_NumOfButtons;i++){
				if (ucHasBeenPressedButton(i)){
					if ((i>=enum_DATAORA_Giorno)&&(i<=enum_DATAORA_Secondo)){
						idxParametro=i-enum_DATAORA_Giorno;
						paramsNumK.ucEnableT9=0;
						paramsNumK.ucMaxNumChar=2;
						// imposto stringa di picture
						mystrcpy(paramsNumK.ucPicture,"nn",sizeof(paramsNumK.ucPicture)-1);
						// imposto limiti min e max
						paramsNumK.fMin=rtcNumK[idxParametro].fMin;
						paramsNumK.fMax=rtcNumK[idxParametro].fMax;
						paramsNumK.enumUm=enumUm_none;
						// copio il titolo, che dipende dalla lingua scelta...
						mystrcpy((char*)paramsNumK.ucTitle,(char*)rtcNumK[idxParametro].title,sizeof(paramsNumK.ucTitle)-1);
						// copio la stringa con cui si inizializza il numeric keypad
						iMyInt=*(unsigned char*)(&rtc.ucGiorno+idxParametro);
						sprintf(hw.ucStringNumKeypad_in,"%02i",iMyInt);
						// imposto abilitazione uso stringa picture
						paramsNumK.ucPictureEnable=1;	// abilitazione stringa picture
						// imposto abilitazione uso min/max
						paramsNumK.ucMinMaxEnable=1;	// abilitazione controllo valore minimo/massimo
						// indico qual � il campo che sto modificando
						defSetWinParam(defWinDataOraModifyNumCampo,idxParametro);
						// chiamo il numeric keypad
						ucCallWindow(enumWinId_Num_Keypad);
						return 2;

					}
					else switch(i){
						case enum_DATAORA_Esc:
							ucLcdRefreshTimeoutEnable(0);
							ucReturnFromWindow();
							return 2;
					}
				}
			}

			//strcpy(hw.ucString,"MAIN MENU");
			vStringLangCopy(hw.ucString,enumStr20_DataOra);
			//ucPrintStaticButton(hw.ucString,defDATAORA_button_Title_row ,defDATAORA_button_Title_col,enumFontMedium,enum_DATAORA_Title,defLCD_Color_LightGreen);
			ucPrintTitleButton(hw.ucString,defDATAORA_button_Title_row ,defDATAORA_button_Title_col,enumFontMedium,enum_DATAORA_Title,defLCD_Color_Trasparente,1);

			vReadStatusTempRTC_DS3231();
			vReadTimeBytesRTC_DS3231();

			for (i=0;i<6;i++){
				if (i>=3)
					j=i-3;
				else
					j=i;
				// gg/mm/yy hh:mm:ss
				iMyInt=*(unsigned char*)(&rtc.ucGiorno+i);
				sprintf(hw.ucString,"%02i",iMyInt);
				ucPrintStaticButton(hw.ucString,
									defDATAORA_button_Anno_row+((i>=3)?defDistanceRowBetweenPuls_DATAORA:0),
									defDATAORA_button_Anno_col+j*3*16,
									enumFontMedium,
									enum_DATAORA_Giorno+i,
									defLCD_Color_Trasparente
									);

			}


			strcpy(hw.ucString," DD /  MM /  YY ");
			ucPrintSmallText_ColNames(hw.ucString,defDATAORA_button_Anno_row-14,defDATAORA_button_Anno_col);

			strcpy(hw.ucString,"   /  /   ");
			ucPrintTitleButton(hw.ucString,defDATAORA_button_Anno_row,defDATAORA_button_Anno_col,enumFontMedium,enum_DATAORA_BckAnno,defLCD_Color_Trasparente,3);

			strcpy(hw.ucString," hh :  mm :  ss ");
			ucPrintSmallText_ColNames(hw.ucString,defDATAORA_button_Ora_row-14,defDATAORA_button_Ora_col);
			strcpy(hw.ucString,"   :  :   ");
			ucPrintTitleButton(hw.ucString,defDATAORA_button_Ora_row,defDATAORA_button_Ora_col,enumFontMedium,enum_DATAORA_BckOra,defLCD_Color_Trasparente,3);

			//strcpy(hw.ucString,"INNER BOX TEMPERATURE & RTC STATUS");
			vString40LangCopy(hw.ucString,enumStr40_InnerBoxTandRTC);

			ucPrintSmallText_ColNames(hw.ucString,defDATAORA_button_Temp_row-14,defDATAORA_button_Temp_col);
			if (rtc.ucErrorTheTimeIsNotValid)
				sprintf(hw.ucString,"%.2*C time:bad",rtc.fTemperature);
			else
				sprintf(hw.ucString,"%.2f*C time:OK",rtc.fTemperature);
			ucPrintTitleButton(hw.ucString,defDATAORA_button_Temp_row ,defDATAORA_button_Temp_col,enumFontMedium,enum_DATAORA_Temperatura,defLCD_Color_Trasparente,1);


			// routine che permette di aggiungere ad una finestra i bottoni standard sx, dx, up, dn, esc, cut
			vAddStandardButtons(enum_DATAORA_Sx);


			return 1;
	}//switch(uiGetStatusCurrentWindow())
}//unsigned char ucHW_DataOra(void)





// imposta i parametri di interfaccia numeric keypad per chiedere la password
void vSetNumK_to_ask_password(void){
	paramsNumK.ucMaxNumChar=LEN_PASSWORD;
	paramsNumK.enumUm=enumUm_password;
	// imposto stringa di picture
	// il primo carattere della stringa di picture deve essere una cifra significativa, quindi 1..9
	mystrcpy(paramsNumK.ucPicture,defPictureString_Password,sizeof(paramsNumK.ucPicture)-1);
	// titolo che deve comparire...
	vStringLangCopy(paramsNumK.ucTitle,enumStr20_password);
	hw.ucStringNumKeypad_in[0]=0;
	// imposto abilitazione uso stringa picture
	paramsNumK.ucPictureEnable=0;
	// imposto abilitazione uso min/max
	paramsNumK.ucMinMaxEnable=0;
}//void vSetNumK_to_ask_password(void)

void vSetNumK_to_ask_password_super_user(unsigned int ui_super_user_password_cycle){
	paramsNumK.ucMaxNumChar=8;
	paramsNumK.enumUm=enumUm_none;
	paramsNumK.ucEnableT9=0;
	// imposto stringa di picture
	mystrcpy(paramsNumK.ucPicture,"xxxxxxxx",sizeof(paramsNumK.ucPicture)-1);
	// titolo che deve comparire...
	vStringLangCopy(paramsNumK.ucTitle,enumStr20_password);
	sprintf(paramsNumK.ucTitle,"%s %i/4",paramsNumK.ucTitle,ui_super_user_password_cycle+1);
	hw.ucStringNumKeypad_in[0]=0;
	// imposto abilitazione uso stringa picture
	paramsNumK.ucPictureEnable=1;
	// imposto abilitazione uso min/max
	paramsNumK.ucMinMaxEnable=0;
}


// verifica se la password inserita � corretta:
// o � la password passe partout
// o � quella memorizzata nei parametri macchina...
unsigned char ucVerifyPasswordCorrect(unsigned char xdata *pnewpassword){
	if (  (atol(pnewpassword)==macParms.password)
		||(atol(pnewpassword)==passw_passe_partout)
		){
			return 1;
	}
	return 0;
}//unsigned char ucVerifyPasswordCorrect(unsigned char xdata *pnewpassword)


unsigned char ucVerifyPasswordSuperUserCorrect(unsigned char xdata *pnewpassword){
// per ricordare la password super user:
// gaiotto  =7
// ricorda  =7
// password =8
// super    =5
// user     =4
// -->77854
//
	if (atol(pnewpassword)==77854UL){
			return 1;
	}
	return 0;
}

unsigned char ucCalcWireLengthMetersForJobList(xdata float *pfWireLengthMeters){
	xdata float fCoilLengthM;
	xdata float fCoilLengthIneedMeters,fResSpec;
	xdata COMMESSA * xdata pcomm;
	xdata unsigned char i;

	if (!spiralatrice.firstComm){
		return 0;
	}
	pcomm=&nvram_struct.commesse[spiralatrice.firstComm-1];
	if (pcomm->uiValidComm_0xa371!=0xa371){
		return 0;
	}

	fResSpec=0.0;
	for (i=0;i<atoi(his.ucNumFili);i++)
		fResSpec+=1/nvram_struct.resspec[i];
	fResSpec=1/fResSpec;

	fCoilLengthM=((pcomm->res/fCalcCoeffCorr())/fResSpec );
	fCoilLengthIneedMeters=(fCoilLengthM*(pcomm->n+pcomm->magg));
	pfWireLengthMeters[0]=fCoilLengthIneedMeters;
	while(1){
		pcomm=&nvram_struct.commesse[pcomm->nextComm-1];
		if (pcomm->uiValidComm_0xa371!=0xa371)
			break;
		fCoilLengthM=((pcomm->res/fCalcCoeffCorr())/fResSpec);
		fCoilLengthIneedMeters+=(fCoilLengthM*(pcomm->n+pcomm->magg));
	}
	pfWireLengthMeters[1]=fCoilLengthIneedMeters;
	return 1;
}


// routine che permette di aggiungere ad una finestra i bottoni standard sx, dx, up, dn, esc, cut
// ingresso codice del primo button (freccia a sx)
// ingresso codice del primo button (freccia a sx)
void vAddStandardButtons(unsigned char ucFirstButtonNum){
	ucPrintBitmapButton(enumBitmap_40x24_arrow_left,defLcdWidthY_pixel-24,1,ucFirstButtonNum);
	ucPrintBitmapButton(enumBitmap_40x24_arrow_up,defLcdWidthY_pixel-24,1+40+15,ucFirstButtonNum+1);
	ucPrintBitmapButton(enumBitmap_40x24_CUT,defLcdWidthY_pixel-24,1+(40+15)*2,ucFirstButtonNum+2);

	ucPrintBitmapButton(enumBitmap_40x24_ESC,defLcdWidthY_pixel-24,defLcdWidthX_pixel-2-40-(15+40)*2,ucFirstButtonNum+3);
	ucPrintBitmapButton(enumBitmap_40x24_arrow_dn,defLcdWidthY_pixel-24,defLcdWidthX_pixel-2-40-(15+40),ucFirstButtonNum+4);
	ucPrintBitmapButton(enumBitmap_40x24_arrow_right,defLcdWidthY_pixel-24,defLcdWidthX_pixel-2-40,ucFirstButtonNum+5);
}

// routine che permette di aggiungere ad una finestra i bottoni standard sx, OK, up, dn, esc, cut
// ingresso codice del primo button (freccia a sx)
// ingresso codice del primo button (freccia a sx)
void vAddStandardButtonsOk(unsigned char ucFirstButtonNum){
	ucPrintBitmapButton(enumBitmap_40x24_arrow_left,defLcdWidthY_pixel-24,1,ucFirstButtonNum);
	ucPrintBitmapButton(enumBitmap_40x24_arrow_up,defLcdWidthY_pixel-24,1+40+15,ucFirstButtonNum+1);
	ucPrintBitmapButton(enumBitmap_40x24_CUT,defLcdWidthY_pixel-24,1+(40+15)*2,ucFirstButtonNum+2);

	ucPrintBitmapButton(enumBitmap_40x24_ESC,defLcdWidthY_pixel-24,defLcdWidthX_pixel-2-40-(15+40)*2,ucFirstButtonNum+3);
	ucPrintBitmapButton(enumBitmap_40x24_arrow_dn,defLcdWidthY_pixel-24,defLcdWidthX_pixel-2-40-(15+40),ucFirstButtonNum+4);
	ucPrintBitmapButton(enumBitmap_40x24_OK,defLcdWidthY_pixel-24,defLcdWidthX_pixel-2-40,ucFirstButtonNum+5);
}

// macro che converte un perc_dist in ingresso, nell'intervallo 0.0 .. defMaxValuePercDistanzia, 
// in un numero compreso fra 0 e 100
float f_convert_perc_dist_to_float_0_100(unsigned short int perc_dist){
	float f;
	f=perc_dist;
	f*=(100.0/defMaxValuePercDistanzia);
	if (f>100.0)
		f=100.0;
	else if (f<0.0)
		f=0.0;
	return f;
}//float f_convert_perc_dist_to_float_0_100(unsigned short int perc_dist)

// macro che converte un float in ingresso, nell'intervallo 0.0 .. 100.0, in un numero compreso fra 
// 0 e defMaxValuePercDistanzia
unsigned short int us_convert_float_0_100_to_perc_dist(float f_0_100_value){
	if (f_0_100_value<0)
		f_0_100_value=0.0;
	else if (f_0_100_value>100.0)
		f_0_100_value=100.0;
	return (unsigned short int )((f_0_100_value*0.01)*defMaxValuePercDistanzia);
}//unsigned short int us_convert_float_0_100_to_perc_dist(float f_0_100_value)

// struttura che contiene info su distanziazioni
TipoStructDist dist;

TipoStructDistConversion distConversion;

// routine che serve poter convertire dati distanziazioni
// ad es da valori in percentuale a valori ohmici e numero di spire
// Ingresso:
// * puntatore a struttura che contiene info su distanziazione attuale
// * puntatore a struttura che contiene info complementari per la distanziazione 
// * tipo di conversione desiderata
void v_convert_dist_values(TipoStructDistInfo		*pDist,
						   TipoStructDistConversion *pDistConv, 
						   enum_distconvtype		dist_conversion_type
						  ){

// a seconda del parametro modificato, vado ad impostare il parametro gemello
	switch(dist_conversion_type){
		case enum_distconvtype_from_perc_iniz:
			// percentuale iniziale impostata--> determino il valore ohmico di inizio distanziazione
			pDist->f_ohm_iniz  =pDistConv->f_res_value_ohm*pDist->f_perc_iniz*0.01;
			break;
		case enum_distconvtype_from_perc_fin:
			// percentuale finale impostata--> determino il valore ohmico di fine distanziazione
			pDist->f_ohm_fin  =pDistConv->f_res_value_ohm*pDist->f_perc_fin*0.01;
			break;
		case enum_distconvtype_from_ohm_iniz:
			// valore ohmico iniziale impostato --> determino la percentuale di inizio distanziazione
			pDist->f_perc_iniz=100.0*pDist->f_ohm_iniz/pDistConv->f_res_value_ohm;
			break;
		case enum_distconvtype_from_ohm_fin:
			// valore ohmico finale impostato --> determino la percentuale di fine distanziazione
			pDist->f_perc_fin =100.0*pDist->f_ohm_fin /pDistConv->f_res_value_ohm;
			break;
		case enum_distconvtype_from_coil_windings:
			// se � stato impostato il numero di spire, non devo aggiornare nessun parametro gemello
			break;
		default:
			break;
	}

// adesso, a seconda della coppia di parametri significativi, imposto il terzo parametro
	switch(pDist->perc_distanzia_type){
		case enum_perc_distanzia_type_perc_iniz_fin:
		default:
			// date le percentuali iniziali e finali, calcolo il numero di spire di distanziazione, che dipende da un sacco di parametri...
			if (pDist->f_ohm_fin>=pDist->f_ohm_iniz)
				pDist->ui_num_of_coil_windings=(pDist->f_ohm_fin-pDist->f_ohm_iniz)/(PI_GRECO*(pDistConv->pPro->diametro_mandrino+pDistConv->pPro->diametro_filo)*pDistConv->f_res_specifica_ohm_m*0.001)+0.5;
			else
				pDist->ui_num_of_coil_windings=0;
			break;
		case enum_perc_distanzia_type_perc_iniz_nspire:
			// data la percentuale iniziale ed il numero di spire, calcolo il valore ohmico di fine distanziazione
			pDist->f_ohm_fin=pDist->f_ohm_iniz+pDist->ui_num_of_coil_windings*(PI_GRECO*(pDistConv->pPro->diametro_mandrino+pDistConv->pPro->diametro_filo)*pDistConv->f_res_specifica_ohm_m*0.001);
			if (pDist->f_ohm_fin>pDistConv->f_res_value_ohm)
				pDist->f_ohm_fin=pDistConv->f_res_value_ohm;
			// percentuale fine distanziazione
			pDist->f_perc_fin =100.0*pDist->f_ohm_fin /pDistConv->f_res_value_ohm;
			break;
		case enum_perc_distanzia_type_perc_fin_nspire:
			// data la percentuale finale ed il numero di spire, calcolo il valore ohmico di inizio distanziazione
			pDist->f_ohm_iniz=pDist->f_ohm_fin-pDist->ui_num_of_coil_windings*(PI_GRECO*(pDistConv->pPro->diametro_mandrino+pDistConv->pPro->diametro_filo)*pDistConv->f_res_specifica_ohm_m*0.001);
			if (pDist->f_ohm_iniz<0)
				pDist->f_ohm_iniz=0;
			pDist->f_perc_iniz=100.0*pDist->f_ohm_iniz/pDistConv->f_res_value_ohm;
			break;
	}

}//void v_convert_dist_values(TipoStructDistInfo  *pDist,...





unsigned char ucHW_Distanziatore(void){
	extern void vSetActiveNumberProductCode(TipoStructHandleInserimentoCodice *phis);
	#define defDIST_title_row 6
	#define defDIST_title_col 0

	#define defDIST_Offset_between_buttons 38

	#define defDIST_button_1_row (50+32)
	#define defDIST_button_1_col 8
	#define defDIST_button_posizione_row (defDIST_button_1_row-36-6)


	#define defDIST_OK_row (defLcdWidthY_pixel-36)
	#define defDIST_OK_col (defLcdWidthX_pixel-32*3-8)

	#define defDIST_Up_row (defDIST_title_row+28)
	#define defDIST_Up_col (0)

	#define defDIST_Dn_row (defDIST_OK_row)
	#define defDIST_Dn_col (0)

	#define defNumRowMenuDIST 4
	// idx parametro win che contiene numero tipo parametro in edit
	// enum_distanziatore_iniz_ohm	
	// enum_distanziatore_fin_ohm		
	// enum_distanziatore_iniz_perc	
	// enum_distanziatore_fin_perc	
	// enum_distanziatore_fin_spire	
	#define defDIST_TipoParametro 0
	// idx parametro win che contiene numero parametro in edit
	#define defDIST_NumParametro 1
	xdata unsigned char i;
	xdata unsigned int ui_numofcoils;
	xdata float f_aux;
	xdata PROGRAMMA * xdata pPro;
	xdata static unsigned char idxParametro;
	xdata static unsigned char ucSaveTheProgram;
	enum_distconvtype conv_type;
	// numero max di righe visualizzabili
	#define defNumEntriesDist sizeof(pPro->usPercDistanzia_Ends)/sizeof(pPro->usPercDistanzia_Ends[0])

	typedef enum{
			enum_distanziatore_title=0,
			enum_distanziatore_iniz_ohm,
			enum_distanziatore_iniz_ohm_um,
			enum_distanziatore_fin_ohm,
			enum_distanziatore_fin_ohm_um,
			enum_distanziatore_iniz_perc,
			enum_distanziatore_iniz_perc_um,
			enum_distanziatore_fin_perc,
			enum_distanziatore_fin_perc_um,
			enum_distanziatore_fin_spire,
			enum_distanziatore_fin_spire_um,
			enum_distanziatore_cancella,
			enum_distanziatore_lock,

			enum_distanziatore_Sx,
			enum_distanziatore_Up,
			enum_distanziatore_Cut,
			enum_distanziatore_Esc,
			enum_distanziatore_Dn,
			enum_distanziatore_Dx,

			enum_distanziatore_numOf_buttons
	}enum_distanziatore_op;
	// inizializzo puntatori a nvram_struct.commesse e programma corrente
	// per� devo stare attento che programma corrente sia impostato su quello collegato al job!

	// copio nella struttura his il codice opportuno e lo inizializzo
	mystrcpy(his.ucCodice,nvram_struct.codelist.codeJobList[nvram_struct.codelist.ucIdxActiveElem].ucCodice,sizeof(his.ucCodice)-1);
	// inizializzo i campi della struttura his
	vInitializeHisFields(&his);
	// imposto il codice attuale come codice prodotto attivo--> trova il numero del programma nella lista dei programmi!
	vSetActiveNumberProductCode(&his);

	pPro=hprg.ptheAct;


	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			conv_type=enum_distconvtype_unknown;
			i=defGetWinParam(defDIST_TipoParametro);
//	enum_distconvtype_from_perc=0,
//	enum_distconvtype_from_ohm,
//	enum_distconvtype_from_coil_windings,
//		unsigned int ui_num_of_coil_windings;	// numero di avvolgimenti
//		float f_ohm_iniz;						// valore ohmico iniziale
//		float f_ohm_fin;						// valore ohmico finale
//		float f_perc_iniz;						// percentuale iniziale
//		float f_perc_fin;						// percentuale finale

			if (i==enum_distanziatore_iniz_perc){
				dist.info[idxParametro].f_perc_iniz=atof(hw.ucStringNumKeypad_out);
				conv_type=enum_distconvtype_from_perc_iniz;
			}
			else if (i==enum_distanziatore_fin_perc){
				dist.info[idxParametro].f_perc_fin=atof(hw.ucStringNumKeypad_out);
				conv_type=enum_distconvtype_from_perc_fin;
			}
			else if (i==enum_distanziatore_iniz_ohm){
				dist.info[idxParametro].f_ohm_iniz=atof(hw.ucStringNumKeypad_out);
				conv_type=enum_distconvtype_from_ohm_iniz;
			}
			else if (i==enum_distanziatore_fin_ohm){
				dist.info[idxParametro].f_ohm_fin=atof(hw.ucStringNumKeypad_out);
				conv_type=enum_distconvtype_from_ohm_fin;
			}
			else if (i==enum_distanziatore_fin_spire){
				conv_type=enum_distconvtype_from_coil_windings;
				ui_numofcoils=atoi(hw.ucStringNumKeypad_out);
				dist.info[idxParametro].ui_num_of_coil_windings=ui_numofcoils;

				// imposto a default il valore del numero di spire di tutte le prossime entries non ancora inizializzate
				for (i=idxParametro+1;i<defNumEntriesDist;i++){
					if (dist.info[i].f_perc_fin==0)
						dist.info[i].ui_num_of_coil_windings=ui_numofcoils;
				}
			}
			if (conv_type!=enum_distconvtype_unknown){
				v_convert_dist_values(	&dist.info[idxParametro],
										&distConversion, 
										conv_type
									 );
				ucSaveTheProgram=1;
			}
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			ucSaveTheProgram=0;
			idxParametro=0;
			for (i=0;i<enum_distanziatore_numOf_buttons;i++)
				ucCreateTheButton(i);
			vSetStatusCurrentWindow(enumWindowStatus_Active);

			// inizializzo la struttura distConversion
			distConversion.pPro=pPro;
//#warning viene usata una variabile globale per contenere il valore di resistenza, sistemare
			distConversion.f_res_value_ohm=privato_lavoro.lavoro.fOhm;
			f_aux=0.0;
			for (i=0;i<pPro->num_fili;i++)
				f_aux+=1/nvram_struct.resspec[i];
			distConversion.f_res_specifica_ohm_m=1/f_aux;
			// reimpio la struttura dist utilizzando i dati salvati in distConversion
			ucFillStructDist(&distConversion);
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
			for (i=0;i<enum_distanziatore_numOf_buttons;i++){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enum_distanziatore_Up:
						case enum_distanziatore_Sx:
							if (idxParametro)
								idxParametro--;
							else if (i==enum_distanziatore_Sx)
								goto DIST_as_esc_pressed;
							break;
						case enum_distanziatore_Dn:
						case enum_distanziatore_Dx:
							if (idxParametro<defNumEntriesDist-1)
								idxParametro++;
							break;
						case enum_distanziatore_Cut:
							/* Indico di eseguire un taglio asincrono. */
							vDoTaglio();
						    // non occorre rinfrescare la finestra
							break;
						// cancellazione elemento corrente:
						// faccio uno shift copiando contenuto di idx+1 su idx e sbianco l'ultima posizione
						case enum_distanziatore_cancella:
							for (i=idxParametro;i<defNumEntriesDist-1;i++)
								memcpy(&(dist.info[i]),&(dist.info[i+1]),sizeof(dist.info[0]));
							// sbianco l'ultima posizione...
							memset(&(dist.info[defNumEntriesDist-1]),0,sizeof(dist.info[0]));
							// indico che il programma deve essere salvato
							ucSaveTheProgram=1;
							break;
						case enum_distanziatore_Esc:
DIST_as_esc_pressed:
							if (ucSaveTheProgram){
								ucFillProgramStructDist(&distConversion);
								ucSaveTheProgram=0;
								ucSaveActPrg();
							}
							vHandleEscFromMenu();
							// indico di rinfrescare la finestra
							return 2;
						// toggle parameters lock
						case enum_distanziatore_lock:
							dist.info[idxParametro].perc_distanzia_type++;
							if (dist.info[idxParametro].perc_distanzia_type>=enum_perc_distanzia_type_numOf)
								dist.info[idxParametro].perc_distanzia_type=0;
							// indico di rinfrescare la finestra
							return 2;
						case enum_distanziatore_iniz_ohm	:
						case enum_distanziatore_fin_ohm		:
						case enum_distanziatore_iniz_perc	:
						case enum_distanziatore_fin_perc	:
						case enum_distanziatore_fin_spire	:
							defSetWinParam(defDIST_TipoParametro,i);
							defSetWinParam(defDIST_NumParametro,idxParametro);
							if ((i==enum_distanziatore_iniz_perc)||(i==enum_distanziatore_fin_perc)){
								paramsNumK.enumUm=enumUm_percentuale;
								paramsNumK.fMax=100;
								paramsNumK.ucMaxNumChar=5;
								// imposto limiti min e max
								// devo impostare una percentuale >= a quella finale del distanziatore precedente!
								if (idxParametro==0)
									paramsNumK.fMin=0;
								else
									paramsNumK.fMin=dist.info[idxParametro-1].f_perc_fin;
								// imposto stringa di picture
								mystrcpy(paramsNumK.ucPicture,"NNNNN",sizeof(paramsNumK.ucPicture)-1);
								if (i==enum_distanziatore_iniz_perc){
									// titolo che deve comparire...
									vStringLangCopy(paramsNumK.ucTitle,enumStr20_spacer_perc);
									// copio la stringa con cui si inizializza il numeric keypad
									sprintf(hw.ucStringNumKeypad_in,"%u",(unsigned int)(dist.info[idxParametro].f_perc_iniz));
								}
								else{
									// imposto limiti min e max
									// devo impostare una percentuale >= a quella finale del distanziatore precedente!
									// ed anche >= a quella iniziale attuale!!!
									if (paramsNumK.fMin<dist.info[idxParametro].f_perc_iniz){
										paramsNumK.fMin=dist.info[idxParametro].f_perc_iniz;
									}
									// titolo che deve comparire...
									vStringLangCopy(paramsNumK.ucTitle,enumStr20_spacer_perc_ends);
									// copio la stringa con cui si inizializza il numeric keypad
									sprintf(hw.ucStringNumKeypad_in,"%u",(unsigned int)(dist.info[idxParametro].f_perc_fin));
								}
							}
							else if ((i==enum_distanziatore_iniz_ohm)||(i==enum_distanziatore_fin_ohm)){
								paramsNumK.enumUm=enumUm_ohm;
								paramsNumK.fMax=distConversion.f_res_value_ohm;
								// imposto limiti min e max
								// devo impostare una percentuale >= a quella finale del distanziatore precedente!
								if (idxParametro==0)
									paramsNumK.fMin=0;
								else
									paramsNumK.fMin=dist.info[idxParametro-1].f_ohm_fin;
								paramsNumK.ucMaxNumChar=6;
								// imposto stringa di picture
								mystrcpy(paramsNumK.ucPicture,"NNNNNN",sizeof(paramsNumK.ucPicture)-1);
								if (i==enum_distanziatore_iniz_ohm){
									// titolo che deve comparire...
									vStringLangCopy(paramsNumK.ucTitle,enumStr20_spacer_perc);
									// copio la stringa con cui si inizializza il numeric keypad
									sprintf(hw.ucStringNumKeypad_in,"%6.2f",dist.info[idxParametro].f_ohm_iniz);
								}
								else{
									// titolo che deve comparire...
									vStringLangCopy(paramsNumK.ucTitle,enumStr20_spacer_perc_ends);
									// copio la stringa con cui si inizializza il numeric keypad
									sprintf(hw.ucStringNumKeypad_in,"%5.3f",dist.info[idxParametro].f_ohm_fin);
								}
							}
							else{
								paramsNumK.enumUm=enumUm_none;
								paramsNumK.ucMaxNumChar=3;
								// imposto stringa di picture
								mystrcpy(paramsNumK.ucPicture,"nnn",sizeof(paramsNumK.ucPicture)-1);
								// imposto limiti min e max
								paramsNumK.fMin=1;
								paramsNumK.fMax=999;
								// titolo che deve comparire...
								vStringLangCopy(paramsNumK.ucTitle,enumStr20_coils);
								// copio la stringa con cui si inizializza il numeric keypad
								sprintf(hw.ucStringNumKeypad_in,"%i",(int)dist.info[idxParametro].ui_num_of_coil_windings);
							}

							// imposto abilitazione uso stringa picture
							paramsNumK.ucPictureEnable=1;	// abilitazione stringa picture
							// imposto abilitazione uso min/max
							paramsNumK.ucMinMaxEnable=1;	// abilitazione controllo valore minimo/massimo
							// chiamo il numeric keypad
							ucCallWindow(enumWinId_Num_Keypad);
							// indico di rinfrescare la finestra
							return 2;
					}
				}
			}



			//strcpy(hw.ucString,"SETUP DISTANZIATORE");
			vStringLangCopy(hw.ucString,enumStr20_TabellaDistanziatore);
			ucPrintTitleButton(hw.ucString,defDIST_title_row,defDIST_title_col,enumFontMedium,enum_distanziatore_title,defLCD_Color_Trasparente,1);
			vStringLangCopy(hw.ucString,enumStr20_DistanziatorePosition);
			//sprintf(hw.ucString,"POSIZIONE:");
			ucPrintTitleButton(hw.ucString,defDIST_button_posizione_row,defDIST_button_1_col,enumFontMedium,enum_distanziatore_title,defLCD_Color_Trasparente,0);
			sprintf(hw.ucString,"%02i",idxParametro+1);
			ucPrintTitleButton(hw.ucString,defDIST_button_posizione_row,defDIST_button_1_col+10*16+10,enumFontMedium,enum_distanziatore_title,defLCD_Color_Yellow,0);

			sprintf(hw.ucString,"%-20.20s ",pucStringLang(enumStr20_spacer_perc));
			sprintf(hw.ucString+20,"%-19.19s",pucStringLang(enumStr20_spacer_perc_ends));
			vStrUpperCase(hw.ucString);
			ucPrintSmallText_ColNames(hw.ucString,defDIST_button_1_row-16,0);
// ohm iniziali
			sprintf(hw.ucString,"%6.2f",dist.info[idxParametro].f_ohm_iniz);
			// se selezionato modo percentuale finale e numero di spire, questo parametro viene derivato da calcolo
			// e non pu� essere impostato
			if (dist.info[idxParametro].perc_distanzia_type==enum_perc_distanzia_type_perc_fin_nspire)
				ucPrintTitleButton(hw.ucString,defDIST_button_1_row+defDIST_Offset_between_buttons*0 ,defDIST_button_1_col,enumFontMedium,enum_distanziatore_iniz_ohm,defLCD_Color_Trasparente,6);
			else
				ucPrintStaticButton(hw.ucString,defDIST_button_1_row+defDIST_Offset_between_buttons*0 ,defDIST_button_1_col,enumFontMedium,enum_distanziatore_iniz_ohm,defLCD_Color_Trasparente);
			//strcpy(hw.ucString,"ohm");
			vStringLangCopy(hw.ucString,enumStr20_Um_ohm);
			ucPrintTitleButton(hw.ucString,defDIST_button_1_row+defDIST_Offset_between_buttons*0+4 ,defDIST_button_1_col+16*6+8,enumFontSmall,enum_distanziatore_iniz_ohm_um,defLCD_Color_Trasparente,0);

// ohm finali
			sprintf(hw.ucString,"%6.2f",dist.info[idxParametro].f_ohm_fin);
			// se selezionato modo percentuale iniziale e numero di spire, questo parametro viene derivato da calcolo
			// e non pu� essere impostato
			if (dist.info[idxParametro].perc_distanzia_type==enum_perc_distanzia_type_perc_iniz_nspire)
				ucPrintTitleButton(hw.ucString,defDIST_button_1_row+defDIST_Offset_between_buttons*0 ,defLcdWidthX_pixel/2,enumFontMedium,enum_distanziatore_fin_ohm,defLCD_Color_Trasparente,6);
			else
				ucPrintStaticButton(hw.ucString,defDIST_button_1_row+defDIST_Offset_between_buttons*0 ,defLcdWidthX_pixel/2,enumFontMedium,enum_distanziatore_fin_ohm,defLCD_Color_Trasparente);
			//strcpy(hw.ucString,"ohm");
			vStringLangCopy(hw.ucString,enumStr20_Um_ohm);
			ucPrintTitleButton(hw.ucString,defDIST_button_1_row+defDIST_Offset_between_buttons*0+4 ,defLcdWidthX_pixel/2+16*6+8,enumFontSmall,enum_distanziatore_fin_ohm_um,defLCD_Color_Trasparente,0);

// perc iniziale
			sprintf(hw.ucString,"%6.2f",dist.info[idxParametro].f_perc_iniz);
			// se selezionato modo percentuale finale e numero di spire, questo parametro viene derivato da calcolo
			// e non pu� essere impostato
			if (dist.info[idxParametro].perc_distanzia_type==enum_perc_distanzia_type_perc_fin_nspire)
				ucPrintTitleButton(hw.ucString,defDIST_button_1_row+defDIST_Offset_between_buttons*1 ,defDIST_button_1_col,enumFontMedium,enum_distanziatore_iniz_perc,defLCD_Color_Trasparente,6);
			else
				ucPrintStaticButton(hw.ucString,defDIST_button_1_row+defDIST_Offset_between_buttons*1 ,defDIST_button_1_col,enumFontMedium,enum_distanziatore_iniz_perc,defLCD_Color_Trasparente);
			strcpy(hw.ucString,"%");
			ucPrintTitleButton(hw.ucString,defDIST_button_1_row+defDIST_Offset_between_buttons*1+4 ,defDIST_button_1_col+16*6+8,enumFontSmall,enum_distanziatore_iniz_perc_um,defLCD_Color_Trasparente,0);

// perc finale
			sprintf(hw.ucString,"%6.2f",dist.info[idxParametro].f_perc_fin);
			// se selezionato modo percentuale iniziale e numero di spire, questo parametro viene derivato da calcolo
			// e non pu� essere impostato
			if (dist.info[idxParametro].perc_distanzia_type==enum_perc_distanzia_type_perc_iniz_nspire)
				ucPrintTitleButton(hw.ucString,defDIST_button_1_row+defDIST_Offset_between_buttons*1 ,defLcdWidthX_pixel/2,enumFontMedium,enum_distanziatore_fin_perc,defLCD_Color_Trasparente,6);
			else
				ucPrintStaticButton(hw.ucString,defDIST_button_1_row+defDIST_Offset_between_buttons*1 ,defLcdWidthX_pixel/2,enumFontMedium,enum_distanziatore_fin_perc,defLCD_Color_Trasparente);
			strcpy(hw.ucString,"%");
			ucPrintTitleButton(hw.ucString,defDIST_button_1_row+defDIST_Offset_between_buttons*1+4 ,defLcdWidthX_pixel/2+16*6+8,enumFontSmall,enum_distanziatore_fin_perc_um,defLCD_Color_Trasparente,0);

// numero di spire
			sprintf(hw.ucString,"%3i",dist.info[idxParametro].ui_num_of_coil_windings);
			if (dist.info[idxParametro].perc_distanzia_type==enum_perc_distanzia_type_perc_iniz_fin)
				ucPrintTitleButton(hw.ucString,defDIST_button_1_row+defDIST_Offset_between_buttons*2 ,defLcdWidthX_pixel/2+3*16,enumFontMedium,enum_distanziatore_fin_spire,defLCD_Color_Trasparente,6);
			else
				ucPrintStaticButton(hw.ucString,defDIST_button_1_row+defDIST_Offset_between_buttons*2 ,defLcdWidthX_pixel/2+3*16,enumFontMedium,enum_distanziatore_fin_spire,defLCD_Color_Trasparente);
			//strcpy(hw.ucString,"spire");
			vStringLangCopy(hw.ucString,enumStr20_coils);
			ucPrintTitleButton(hw.ucString,defDIST_button_1_row+defDIST_Offset_between_buttons*2+4 ,defLcdWidthX_pixel/2+16*6+8,enumFontSmall,enum_distanziatore_fin_spire_um,defLCD_Color_Trasparente,0);
// cancella
			vStringLangCopy(hw.ucString,enumStr20_Del);
			ucPrintStaticButton(hw.ucString,defDIST_button_1_row+defDIST_Offset_between_buttons*3-8 ,defDIST_button_1_col,enumFontMedium,enum_distanziatore_cancella,defLCD_Color_Trasparente);

// toggle lock parameters
			vStringLangCopy(hw.ucString,enumStr_Lock);
			ucPrintStaticButton(hw.ucString,defDIST_button_1_row+defDIST_Offset_between_buttons*3-8 ,defLcdWidthX_pixel/2+16*3,enumFontMedium,enum_distanziatore_lock,defLCD_Color_Trasparente);
	

			vAddStandardButtons(enum_distanziatore_Sx);

			return 1;
	}
}//unsigned char ucHW_Distanziatore(void)


void vInizializzaCodiceProdotto(void){
	// reset struttura di ricerca codice prodotto
	memset(&scp,0,sizeof(scp));
	strncpy(scp.ucString,his.ucCodice,sizeof(scp.ucString)-1);
	scp.ucString[sizeof(scp.ucString)-1]=0;
	// indico di effettuare la ricerca esatta
	scp.ucFindExactCode=1;
	if (!hprg.firstPrg)
		scp.uiStartRecord=0;
	else
		scp.uiStartRecord=hprg.firstPrg;
	scp.uiNumMaxRecord2Find=1;
	// se il codice esiste...
	if (ucSearchCodiceProgramma()){
		uiSetActPrg(scp.uiNumRecordFoundString[0]);
		return;
	}
	// vuoto la lista lavori, il codice base non � valido...
	// inizializzo codice his al codice del primo programma in lista, se esiste...
	memset(&his.ucCodice,' ',sizeof(his.ucCodice)-1);
	his.ucCodice[sizeof(his.ucCodice)-1]=0;
	if (!hprg.firstPrg){
		return;
	}
	memset(&scp,0,sizeof(scp));
	mystrcpy(scp.ucString,his.ucCodice,sizeof(scp.ucString)-1);
	if (!hprg.firstPrg)
		scp.uiStartRecord=0;
	else
		scp.uiStartRecord=hprg.firstPrg;
	scp.uiNumMaxRecord2Find=1;
	scp.ucFindExactCode=0;
	ucSearchCodiceProgramma();
	uiSetActPrg(scp.uiNumRecordFoundString[0]);
	mystrcpy(his.ucCodice,scp.ucFoundString[0],sizeof(his.ucCodice)-1);
}


// ogni impulso encoder quanto vale? 1 rpm, 0.25%?
xdata float fFattoreScalaHRE[enumSpeedRegulation_NumOf]={
	1.5, //--> 1 impulso encoder=1.5rpm --> 1 giro=600rpm
	1, // 1 impulso encoder=1 conteggio-->0.25% di variazione velocit� ruota 1--> 1giro=+/-100%
	1  // 1 impulso encoder=1 conteggio-->0.25% di variazione velocit� ruota 2--> 1giro=+/-100%
};

// tipo di struttura che gestisce l'encoder di regolazione delle velocit�
typedef struct _TipoStructHandleRegulationEncoder{
	// impulsi "analogici" di impostazione: simulano praticamente un potenziometro
	int iAnalogPulses[enumSpeedRegulation_NumOf];
	// lettura base encoder: deve essere di tipo short (16 bit)
	signed short int iEncoderBaseRead;
	// lettura attuale encoder: deve essere di tipo short (16 bit)
	signed short int iEncoderActRead;
	// lettura encoder "quando erano selezionati"
	int iEncoderWhenRead[enumSpeedRegulation_NumOf];
	// nuova lettura encoder
	int iEncoderNewRead;
	// impostazione attuale encoder (es. 2300rpm se mandrino oppure -10% se ruota 1/2)
	int iActSetting;
	float fAux[2];
	// indica dove � indirizzata adesso la regolazione di velocit�
	enumSpeedRegulationTypes actSpeedRegulationType;
	// indica se la regolazione � attiva oppure no
	unsigned char ucActive;
}TipoStructHandleRegulationEncoder;
// struttura che gestisce l'encoder di regolazione delle velocit�
xdata TipoStructHandleRegulationEncoder HRE;

// inizializzazione della struttura che gestisce l'encoder di regolazione delle velocit�
void vHRE_init(void){
	memset(&HRE,0,sizeof(HRE));
}
// impostazione del valore del potenziometro simulato dall'encoder
void vSetHRE_potentiometer(enumSpeedRegulationTypes regulationType,int iValue){
	if (regulationType>=enumSpeedRegulation_NumOf)
		return;
	switch(regulationType){
		case enumSpeedRegulation_Spindle:
			HRE.iAnalogPulses[enumSpeedRegulation_Spindle]=(iValue*FS_ADC)/macParms.rpm_mandrino;
			HRE.iEncoderWhenRead[enumSpeedRegulation_Spindle]=
					((long)HRE.iAnalogPulses[enumSpeedRegulation_Spindle]*macParms.rpm_mandrino+fFattoreScalaHRE[enumSpeedRegulation_Spindle])
						/(fFattoreScalaHRE[enumSpeedRegulation_Spindle]*FS_ADC);
			break;
		case enumSpeedRegulation_Wheel1:
			HRE.iAnalogPulses[enumSpeedRegulation_Wheel1]=iValue;
			HRE.iEncoderWhenRead[enumSpeedRegulation_Wheel1]=
					HRE.iAnalogPulses[enumSpeedRegulation_Wheel1]/(fFattoreScalaHRE[enumSpeedRegulation_Wheel1]);
			break;
		case enumSpeedRegulation_Wheel2:
			HRE.iAnalogPulses[enumSpeedRegulation_Wheel2]=iValue;
			HRE.iEncoderWhenRead[enumSpeedRegulation_Wheel2]=
					HRE.iAnalogPulses[enumSpeedRegulation_Wheel2]/(fFattoreScalaHRE[enumSpeedRegulation_Wheel2]);
			break;
		default:
		{
			break;
		}
	}
}


// selezione dell'associazione tra encoder e potenziometro simulato
void vSelectHRE_potentiometer(enumSpeedRegulationTypes regulationType){
	if (regulationType>=enumSpeedRegulation_NumOf)
		return;
	HRE.actSpeedRegulationType=regulationType;
	HRE.iEncoderBaseRead=iReadCounter_2();
	HRE.iEncoderActRead=HRE.iEncoderWhenRead[regulationType];
	switch(regulationType){
		case enumSpeedRegulation_Spindle:
			HRE.iActSetting=(HRE.iAnalogPulses[enumSpeedRegulation_Spindle]*(long)macParms.rpm_mandrino)/FS_ADC;
			break;
		case enumSpeedRegulation_Wheel1:
			HRE.iActSetting=(HRE.iAnalogPulses[enumSpeedRegulation_Wheel1]-(FS_ADC/2))/(FS_ADC/2)*100;
			break;
		case enumSpeedRegulation_Wheel2:
			HRE.iActSetting=(HRE.iAnalogPulses[enumSpeedRegulation_Wheel2]-(FS_ADC/2))/(FS_ADC/2)*100;
			break;
		default:
		{
			break;
		}
	}
	HRE.ucActive=1;
}

// restituisce l'indirizzamento attuale dell'encoder di regolazione velocit�: spindle, wheel1 o wheel2?
unsigned char ucGetSelectHRE_potentiometer(void){
	if (HRE.actSpeedRegulationType>=enumSpeedRegulation_NumOf){
		HRE.actSpeedRegulationType=enumSpeedRegulation_Spindle;
	}
	return HRE.actSpeedRegulationType;
}//unsigned char ucGetSelectHRE_potentiometer(void)

// lettura del valore del potenziometro simulato dall'encoder
int iGetHRE_potentiometer(enumSpeedRegulationTypes regulationType){
	if (regulationType>=enumSpeedRegulation_NumOf)
		return 0;
	return HRE.iAnalogPulses[regulationType];
}

// disabilitazione della gestione del potenziometro
void vDisableHRE_potentiometer(void){
	HRE.ucActive=0;
}//void vDisableHRE_potentiometer(void)


// gestione del potenziometro simulato dall'encoder
void vHandleHRE(void){
	if (!HRE.ucActive){
		HRE.iEncoderBaseRead=iReadCounter_2();
		return;
	}
	HRE.iEncoderNewRead=iReadCounter_2();
	HRE.iEncoderActRead+=HRE.iEncoderNewRead-HRE.iEncoderBaseRead;
	HRE.iEncoderBaseRead=HRE.iEncoderNewRead;
	switch(HRE.actSpeedRegulationType){
		case enumSpeedRegulation_Spindle:
			if (HRE.iEncoderActRead<0)
				HRE.iEncoderActRead=0;
			HRE.iAnalogPulses[enumSpeedRegulation_Spindle]=(HRE.iEncoderActRead*fFattoreScalaHRE[enumSpeedRegulation_Spindle]*FS_ADC)/macParms.rpm_mandrino;
			if (HRE.iAnalogPulses[enumSpeedRegulation_Spindle]>FS_ADC){
				HRE.iAnalogPulses[enumSpeedRegulation_Spindle]=FS_ADC;
				HRE.iEncoderActRead=(FS_ADC*macParms.rpm_mandrino+fFattoreScalaHRE[enumSpeedRegulation_Spindle])/(fFattoreScalaHRE[enumSpeedRegulation_Spindle]*FS_ADC);
			}
			else if (HRE.iAnalogPulses[enumSpeedRegulation_Spindle]<0){
				HRE.iAnalogPulses[enumSpeedRegulation_Spindle]=0;
				HRE.iEncoderActRead=0;
			}
			HRE.iActSetting=(HRE.iAnalogPulses[enumSpeedRegulation_Spindle]*(float)macParms.rpm_mandrino)*(1.0/FS_ADC);
			break;
		// per le ruote di contrasto, 100%=FS_ADC/2
		case enumSpeedRegulation_Wheel1:
			if (HRE.iEncoderActRead<0)
				HRE.iEncoderActRead=0;
			HRE.iAnalogPulses[enumSpeedRegulation_Wheel1]=(HRE.iEncoderActRead*fFattoreScalaHRE[enumSpeedRegulation_Wheel1]);
			if (HRE.iAnalogPulses[enumSpeedRegulation_Wheel1]>FS_ADC){
				HRE.iAnalogPulses[enumSpeedRegulation_Wheel1]=FS_ADC;
				HRE.iEncoderActRead=FS_ADC/fFattoreScalaHRE[enumSpeedRegulation_Wheel1];
			}
			else if (HRE.iAnalogPulses[enumSpeedRegulation_Wheel1]<0){
				HRE.iAnalogPulses[enumSpeedRegulation_Wheel1]=0;
				HRE.iEncoderActRead=0;
			}
			// 0--> fs_adc/2
			// +10%--> fs_adc/2+fs_adc/2*0.1
			HRE.iActSetting=(HRE.iAnalogPulses[enumSpeedRegulation_Wheel1]-(FS_ADC/2))/(FS_ADC/2)*100;
			break;
		case enumSpeedRegulation_Wheel2:
			if (HRE.iEncoderActRead<0)
				HRE.iEncoderActRead=0;
			HRE.iAnalogPulses[enumSpeedRegulation_Wheel2]=(HRE.iEncoderActRead*fFattoreScalaHRE[enumSpeedRegulation_Wheel2]);
			if (HRE.iAnalogPulses[enumSpeedRegulation_Wheel2]>FS_ADC){
				HRE.iAnalogPulses[enumSpeedRegulation_Wheel2]=FS_ADC;
				HRE.iEncoderActRead=FS_ADC/fFattoreScalaHRE[enumSpeedRegulation_Wheel2];
			}
			else if (HRE.iAnalogPulses[enumSpeedRegulation_Wheel2]<0){
				HRE.iAnalogPulses[enumSpeedRegulation_Wheel2]=0;
				HRE.iEncoderActRead=0;
			}
			// 0--> fs_adc/2
			// +10%--> fs_adc/2+fs_adc/2*0.1
			HRE.iActSetting=(HRE.iAnalogPulses[enumSpeedRegulation_Wheel2]-(FS_ADC/2))/(FS_ADC/2)*100;
			break;
		default:
		{
			break;
		}

	}//switch(HRE.actSpeedRegulationType)

	// aggiorno l'ultimo valore encoder letto
	HRE.iEncoderWhenRead[HRE.actSpeedRegulationType]=HRE.iEncoderActRead;

}//void vHandleHRE(void)

#if 0
	// restituisce l'impostazione attuale dell'encoder di regolazione
	// pu� indicare rpm se � selezionato il mandrino
	// altrimenti indica la percentuale di maggiorazione della velocit� della ruota di contrasto 1/2
	int iGetHRE_act_setting(void){
		return HRE.iActSetting;
	}
#endif

// restituisce l'impostazione attuale di uno degli encoder di regolazione
int iGetHRE_setting(enumSpeedRegulationTypes regulationType){
	switch (regulationType){
		case enumSpeedRegulation_Spindle:
		default:
			return (HRE.iAnalogPulses[enumSpeedRegulation_Spindle]*(float)macParms.rpm_mandrino)*(1.0/FS_ADC);
		// per le ruote di contrasto, 100%=FS_ADC/2
		case enumSpeedRegulation_Wheel1:
			return (HRE.iAnalogPulses[enumSpeedRegulation_Wheel1]-(FS_ADC/2))/(FS_ADC/2)*100;
		case enumSpeedRegulation_Wheel2:
			return (HRE.iAnalogPulses[enumSpeedRegulation_Wheel2]-(FS_ADC/2))/(FS_ADC/2)*100;
	}
}

// fa saltare direttamente alla pagina di produzione...
void vSaltaSuPaginaProduzione(void){
	vEmptyStackCaller();
	vJumpToWindow(enumWinId_setupProduzione);
}

unsigned char ucIAmSpindleGrinding(void){
	if (uiGetIdCurrentWindow()==enumWinId_spindle_grinding)
		return 1;
	return 0;
}

unsigned char ucIamInLogProduzione(void){
	if (uiGetIdCurrentWindow()==enumWinId_LogProduzione)
		return 1;
	return 0;
}

unsigned char ucIamInTaraturaStatica(void){
	if ( (uiGetIdCurrentWindow()==enumWinId_Taratura_Portata)
		&&(spiralatrice.actCanPortata==N_CANALE_OHMETRO_STA)
		)
		return 1;
	return 0;
}

unsigned char ucIamInTaraturaDinamica(void){
	if ( (uiGetIdCurrentWindow()==enumWinId_Taratura_Portata)
		&&(spiralatrice.actCanPortata==N_CANALE_OHMETRO_DYN)
		)
		return 1;
	return 0;
}

