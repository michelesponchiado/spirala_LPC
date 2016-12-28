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


// temp_1 <value> [unit] coeff_1 <value>
// name: temp_1 / coeff_1

static float f_temperature_compensation_t_celsius[2];

static float f_temperature_compensation_coeff[2];
                            
unsigned int ui_temperature_coefficient_per_million;


code TipoStructInfoParametro_Lingua if_TCP[]={
	{0,5, "NNNNN","%5.1f",-20,200,(unsigned char*)&f_temperature_compensation_t_celsius[0],1,1,enumStr20_temp_comp_set_T,   enumPT_float,enumUm_none,     enumStringType_none,1},//
	{0,5, "NNNNN","%5.1f",-20,200,(unsigned char*)&f_temperature_compensation_t_celsius[1],1,1,enumStr20_temp_comp_set_T,   enumPT_float,enumUm_none,     enumStringType_none,1},//

	{0,5, "NNNN","%4.2f",0,5,(unsigned char*)&f_temperature_compensation_coeff[0],1,1,enumStr20_temp_comp_set_K,   enumPT_float,enumUm_none,     enumStringType_none,1},//
	{0,5, "NNNN","%4.2f",0,5,(unsigned char*)&f_temperature_compensation_coeff[1],1,1,enumStr20_temp_comp_set_K,   enumPT_float,enumUm_none,     enumStringType_none,1},//
	
	{0,5, "nnnnn","%5i",0,99999,(unsigned char*)&ui_temperature_coefficient_per_million,1,1,enumStr20_temp_comp_set_alfa,   enumPT_unsignedInt,enumUm_none,     enumStringType_none,1},//
	
};

unsigned int ui_is_valid_temp_calibration_point_on_act_prg(unsigned int ui_point_idx){ 
	if (ui_point_idx>=sizeof(hprg.ptheAct->f_temperature_compensation_coeff)/sizeof(hprg.ptheAct->f_temperature_compensation_coeff[0])){
		return 0;
	}
	// if temperature coefficient too low, invalid
	if (hprg.ptheAct->f_temperature_compensation_coeff[ui_point_idx]<=0){
		return 0;
	}
	// if temperature less than 20 deg celsius, it is invalid
	if (hprg.ptheAct->f_temperature_compensation_t_celsius[ui_point_idx]<30){
		return 0;
	}
	return 1;
}
					
// altri pars di produzione...
unsigned char ucHW_temperature_compensation_params(void){
	#define defTCP_title_row 6
	#define defTCP_title_col 0

	#define defTCP_Offset_between_buttons 28

	#define defTCP_button_1_row (defTCP_title_row+24)
	#define defTCP_button_1_col 4



	#define defTCP_NumParametro 0

	// pulsanti della finestra
	typedef enum {
			enumTCP_temp_value_1=0,
			enumTCP_temp_value_2,
			enumTCP_coeff_value_1,
			enumTCP_coeff_value_2,
			enumTCP_alfa_value,
			
			enumTCP_temp_name_1,
			enumTCP_temp_um_1,
			enumTCP_temp_name_2,
			enumTCP_temp_um_2,
			enumTCP_coeff_name_1,
			enumTCP_coeff_name_2,
			enumTCP_alfa_name,
			enumTCP_alfa_um,
			enumTCP_Title,

			enumTCP_Sx,
			enumTCP_Up,
			enumTCP_Cut,
			enumTCP_Esc,
			enumTCP_Dn,
			enumTCP_Dx,


			enumTCP_NumOfButtons
		}enumButtons_TCP;
	unsigned char i;
	unsigned int ui_back_color;
	unsigned char idxParametro;
	extern unsigned char uc_calling_from_view_codice_prodotto;


	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			{
				unsigned int ui_recalc_alfa;
				ui_recalc_alfa=1;
				switch(defGetWinParam(defTCP_NumParametro)){
					case enumTCP_temp_value_1:
						hprg.ptheAct->f_temperature_compensation_t_celsius[0]=atof(hw.ucStringNumKeypad_out);
						break;
					case enumTCP_temp_value_2:
						hprg.ptheAct->f_temperature_compensation_t_celsius[1]=atof(hw.ucStringNumKeypad_out);
						break;
					case enumTCP_coeff_value_1:
						hprg.ptheAct->f_temperature_compensation_coeff[0]=atof(hw.ucStringNumKeypad_out);
						break;
					case enumTCP_coeff_value_2:
						hprg.ptheAct->f_temperature_compensation_coeff[1]=atof(hw.ucStringNumKeypad_out);
						break;
					case enumTCP_alfa_value:
						ui_recalc_alfa=0;
						hprg.ptheAct->ui_temperature_coefficient_per_million=atoi(hw.ucStringNumKeypad_out);
						break;
					default:
						ui_recalc_alfa=0;
						break;
				}
				if (ui_recalc_alfa){
					unsigned int ui_valid_1,ui_valid_2;
					ui_valid_1=ui_is_valid_temp_calibration_point_on_act_prg(0);
					ui_valid_2=ui_is_valid_temp_calibration_point_on_act_prg(1);
					if (ui_valid_1&&!ui_valid_2){
						hprg.ptheAct->ui_temperature_coefficient_per_million=1e6*(hprg.ptheAct->f_temperature_compensation_coeff[0]-1)/(hprg.ptheAct->f_temperature_compensation_t_celsius[0]-20);
					}
					else if (!ui_valid_1&&ui_valid_2){
						hprg.ptheAct->ui_temperature_coefficient_per_million=1e6*(hprg.ptheAct->f_temperature_compensation_coeff[1]-1)/(hprg.ptheAct->f_temperature_compensation_t_celsius[1]-20);
					}
					else if (ui_valid_1&&ui_valid_2){
						float alfa1,alfa2;
						alfa1=1e6*(hprg.ptheAct->f_temperature_compensation_coeff[0]-1)/(hprg.ptheAct->f_temperature_compensation_t_celsius[0]-20);
						alfa2=1e6*(hprg.ptheAct->f_temperature_compensation_coeff[1]-1)/(hprg.ptheAct->f_temperature_compensation_t_celsius[1]-20);
						hprg.ptheAct->ui_temperature_coefficient_per_million=(alfa1+alfa2)/2;
					}
				}
				// salvo il programma
				ucSaveActPrg();				
				vSetStatusCurrentWindow(enumWindowStatus_Active);
				return 2;
			}
		case enumWindowStatus_Initialize:
			i=0;
			while (i<enumTCP_NumOfButtons){
				ucCreateTheButton(i); 
				i++;
			}
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
			i=0;
			while (i<enumTCP_NumOfButtons){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enumTCP_Cut:
							vDoTaglio();
							// non occorre rinfrescare la finestra
							break;

						case enumTCP_Up:
						case enumTCP_Dn:
							if (uc_calling_from_view_codice_prodotto){
								ucReturnFromWindow();
								return 2;
							}
							break;
							
						case enumTCP_Esc:
						case enumTCP_Sx:
						case enumTCP_Dx:
							ucReturnFromWindow();
							return 2;
						case enumTCP_temp_value_1:
						case enumTCP_temp_value_2:
						case enumTCP_coeff_value_1:
						case enumTCP_coeff_value_2:
						case enumTCP_alfa_value:
                            
                            f_temperature_compensation_t_celsius[0]=hprg.ptheAct->f_temperature_compensation_t_celsius[0];
                            f_temperature_compensation_t_celsius[1]=hprg.ptheAct->f_temperature_compensation_t_celsius[1];

                            f_temperature_compensation_coeff[0]=hprg.ptheAct->f_temperature_compensation_coeff[0];
                            f_temperature_compensation_coeff[1]=hprg.ptheAct->f_temperature_compensation_coeff[1];
                            
                            ui_temperature_coefficient_per_million=hprg.ptheAct->ui_temperature_coefficient_per_million;
                        
							idxParametro=i;
							defSetWinParam(defTCP_NumParametro,idxParametro);
							paramsNumK.enumUm=if_TCP[idxParametro].enumUm;	
							// imposto limiti min e max
							paramsNumK.fMin=if_TCP[idxParametro].fMin;
							paramsNumK.fMax=if_TCP[idxParametro].fMax;
							paramsNumK.ucMaxNumChar=if_TCP[idxParametro].ucLenField;	
							// imposto stringa di picture
							mystrcpy((char*)paramsNumK.ucPicture,(char*)if_TCP[idxParametro].ucPictField,sizeof(paramsNumK.ucPicture)-1);
							// titolo che deve comparire...
							vStringLangCopy(paramsNumK.ucTitle,if_TCP[idxParametro].stringa);
							// copio la stringa con cui si inizializza il numeric keypad
							vGetValueParametro_spiwin2(hw.ucStringNumKeypad_in,idxParametro,&if_TCP[0]);
							// imposto abilitazione uso stringa picture
							paramsNumK.ucPictureEnable=if_TCP[idxParametro].ucPictureEnable;	// abilitazione stringa picture
							// imposto abilitazione uso min/max
							paramsNumK.ucMinMaxEnable=if_TCP[idxParametro].ucMinMaxEnable;	// abilitazione controllo valore minimo/massimo
							// chiamo il numeric keypad 
							ucCallWindow(enumWinId_Num_Keypad);
							// indico di rinfrescare la finestra
							return 2;
					}
				}
                i++;
			}

			//strcpy(hw.ucString,"   compensazione temperatura...  ");
			vStringLangCopy(hw.ucString,enumStr20_title_params_temperature_compensation);
			//ucPrintStaticButton(hw.ucString,defAPP_title_row,defAPP_title_col,enumFontMedium,enumAPP_Title,defLCD_Color_Green);
			ucPrintTitleButton(hw.ucString,defTCP_title_row,defTCP_title_col,enumFontSmall,enumTCP_Title,defLCD_Color_Trasparente,1);
// temperature 1			
			// if valid, the background is transparent, else it is grey
			ui_back_color=ui_is_valid_temp_calibration_point_on_act_prg(0)?defLCD_Color_Trasparente:defLCD_Color_Gray;	
			sprintf(hw.ucString,"  %-4.4s",pucStringLang(enumStr20_temperature_compensation_t1));	
			ucPrintTitleButton(hw.ucString	,defTCP_button_1_row+defTCP_Offset_between_buttons*0+4 ,defTCP_button_1_col ,enumFontSmall,enumTCP_temp_name_1,defLCD_Color_Trasparente,2);
			sprintf(hw.ucString,"%5.1f",hprg.ptheAct->f_temperature_compensation_t_celsius[0]);				
			ucPrintStaticButton(hw.ucString	,defTCP_button_1_row+defTCP_Offset_between_buttons*0+4 ,defTCP_button_1_col+8*6+16 ,enumFontMedium,enumTCP_temp_value_1,ui_back_color);
			strcpy(hw.ucString,"*C");				
			ucPrintTitleButton(hw.ucString	,defTCP_button_1_row+defTCP_Offset_between_buttons*0+4 ,defTCP_button_1_col+8*6+16+12*5+32 ,enumFontSmall,enumTCP_temp_um_1,defLCD_Color_Trasparente,2);
// temperature K 1			
			sprintf(hw.ucString,"  %-4.4s",pucStringLang(enumStr20_temperature_compensation_k1));				
			ucPrintTitleButton(hw.ucString	,defTCP_button_1_row+defTCP_Offset_between_buttons*1+4 ,defTCP_button_1_col ,enumFontSmall,enumTCP_coeff_name_1,defLCD_Color_Trasparente,2);
			sprintf(hw.ucString," %4.2f",hprg.ptheAct->f_temperature_compensation_coeff[0]);				
			ucPrintStaticButton(hw.ucString	,defTCP_button_1_row+defTCP_Offset_between_buttons*1+4 ,defTCP_button_1_col+8*6+16 ,enumFontMedium,enumTCP_coeff_value_1,ui_back_color);
// temperature 2			
			// if valid, the background is transparent, else it is grey
			ui_back_color=ui_is_valid_temp_calibration_point_on_act_prg(1)?defLCD_Color_Trasparente:defLCD_Color_Gray;	
			sprintf(hw.ucString,"  %-4.4s",pucStringLang(enumStr20_temperature_compensation_t2));	
			ucPrintTitleButton(hw.ucString	,defTCP_button_1_row+defTCP_Offset_between_buttons*2+4+8 ,defTCP_button_1_col ,enumFontSmall,enumTCP_temp_name_2,defLCD_Color_Trasparente,2);
			sprintf(hw.ucString,"%5.1f",hprg.ptheAct->f_temperature_compensation_t_celsius[1]);				
			ucPrintStaticButton(hw.ucString	,defTCP_button_1_row+defTCP_Offset_between_buttons*2+4+8 ,defTCP_button_1_col+8*6+16 ,enumFontMedium,enumTCP_temp_value_2,ui_back_color);
			strcpy(hw.ucString,"*C");				
			ucPrintTitleButton(hw.ucString	,defTCP_button_1_row+defTCP_Offset_between_buttons*2+4+8 ,defTCP_button_1_col+8*6+16+12*5+32 ,enumFontSmall,enumTCP_temp_um_2,defLCD_Color_Trasparente,2);
// temperature K 2			
			sprintf(hw.ucString,"  %-4.4s",pucStringLang(enumStr20_temperature_compensation_k2));				
			ucPrintTitleButton(hw.ucString	,defTCP_button_1_row+defTCP_Offset_between_buttons*3+4+8 ,defTCP_button_1_col ,enumFontSmall,enumTCP_coeff_name_2,defLCD_Color_Trasparente,2);
			sprintf(hw.ucString," %4.2f",hprg.ptheAct->f_temperature_compensation_coeff[1]);				
			ucPrintStaticButton(hw.ucString	,defTCP_button_1_row+defTCP_Offset_between_buttons*3+4+8 ,defTCP_button_1_col+8*6+16 ,enumFontMedium,enumTCP_coeff_value_2,ui_back_color);
			
// alfa		
			sprintf(hw.ucString,"%-6.6s",pucStringLang(enumStr20_temperature_compensation_alfa));				
			ucPrintTitleButton(hw.ucString	,defTCP_button_1_row+defTCP_Offset_between_buttons*4+4+16 ,defTCP_button_1_col ,enumFontSmall,enumTCP_alfa_name,defLCD_Color_Trasparente,2);
			sprintf(hw.ucString, "%5i",hprg.ptheAct->ui_temperature_coefficient_per_million);				
			ucPrintStaticButton(hw.ucString	,defTCP_button_1_row+defTCP_Offset_between_buttons*4+4+16 ,defTCP_button_1_col+8*6+16 ,enumFontMedium,enumTCP_alfa_value,defLCD_Color_Trasparente);
			strcpy(hw.ucString," 10^6/*C");				
			ucPrintTitleButton(hw.ucString	,defTCP_button_1_row+defTCP_Offset_between_buttons*4+4+16 ,defTCP_button_1_col+8*6+16+12*6+16 ,enumFontSmall,enumTCP_alfa_um,defLCD_Color_Trasparente,2);
			vAddStandardButtons(enumTCP_Sx);

			return 1;
         
	}
}//unsigned char ucHW_AltriParametriProduzione(void)
