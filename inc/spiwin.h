// indico di generare src assembly
// #pragma SRC

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


void mystrcpy(char *dst, char *src, unsigned int uiMaxchar2copy);
void vForceRefreshLcd(void);
void vResetAllButtons(void);
unsigned char ucResetTheButton(unsigned char ucNumButton);
unsigned char ucAckButton(unsigned char ucNumButton);
unsigned char ucHasBeenPressedButton(unsigned char ucNumButton);
unsigned char ucCreateTheButton(unsigned char ucNumButton);
unsigned char ucTheButtonIsCreated(unsigned char ucNumButton);
unsigned char ucTheButtonIsPressed(unsigned char ucNumButton);
void vSignalButtonPressed(unsigned char ucNumButton,unsigned char ucNumCharPressed);
// funzione di gestione dei pulsanti
void vHandleButtons(void);
void vValidateTabFornitori(void);

extern data int iActCounts;
extern data int iActCounts2;


// numero del parametro della finestra yesno che contiene la risposta: 1 =yes, 0=no
#define defWinParam_YESNO_Answer 0



typedef enum {
	enumWinId_None							   =0,
	enumWinId_listaCodici						,
	enumWinId_CodiceProdotto		            ,
	enumWinId_ListaLavori		                ,
	enumWinId_ModificaLavoro		            ,
	enumWinId_inserisciOhmMfili		            ,
	enumWinId_setupProduzione		            ,
	enumWinId_Taratura_Misura_Dyn_Or_Stat		,
	enumWinId_Taratura_Portata_1_4			    ,
	enumWinId_Taratura_Portata					,
	enumWinId_ParametriMacchina			        ,
	enumWinId_Utilities     			        ,
	enumWinId_IngressiDigitali					,
	enumWinId_UsciteDigitali					,
	enumWinId_IngressiAnalogici					,
	enumWinId_UsciteAnalogiche					,
	enumWinId_IngressiEncoder					,
	enumWinId_AltriParametriProduzione			,
	enumWinId_Fornitori							,
	enumWinId_Num_Keypad						,  // keypad numerico
	enumWinId_YesNo						        ,  // richiesta conferma s�/no
	enumWinId_MainMenu							,
	enumWinId_Info								,
	enumWinId_LottoDiretto						,
	enumWinId_LogProduzione						,
	enumWinId_ViewCodiceProdotto				,
	enumWinId_CambioBoccola						,
	enumWinId_CalibraSensore					,
	enumWinId_SelezionaTaratura					,
	enumWinId_SaveParams						,
	enumWinId_DataOra							,
	enumWinId_LoadParsMac						,
	enumWinId_LoadCodiciProdotto				,
	enumWinId_LoadCustomLanguage				,
	enumWinId_ResetCodiciProdotto				,
	enumWinId_Distanziatore						,
	enumWinId_TaraDistanziatore					,
	enumWinId_Temperature_Compensation_Params	,
	enumWinId_spindle_grinding	,

	enumWinId_NumOf
}enumWindowsId;




//
// ---------------------------------------
//   gestione delle finestre...
// ---------------------------------------
//
extern xdat PROGRAMMA auxPrg;


// motivi per cui si � usciti dalla finestra
typedef enum {
		enumWindowExitCode_Ok,
		enumWindowExitCode_Escape,
		enumWindowExitCode_Keypad,
		enumWindowExitCode_NumOf
	}enumWindowExitCode;

// stati possibili della finestra: idle, attiva, aspetta ritorno da finestra chiamata...
typedef enum{
	enumWindowStatus_Null=0,
	enumWindowStatus_Initialize,
	enumWindowStatus_Active,
	enumWindowStatus_WaitingReturn,
	enumWindowStatus_ReturnFromCall,
	enumWindowStatus_DoSomeActivity_0,
	enumWindowStatus_DoSomeActivity_1,
	enumWindowStatus_DoSomeActivity_2,
	enumWindowStatus_NumOf
}enumWindowStatus;

// tipo parametro finestra
typedef unsigned int tipoParamWindow;
// numero max di parametri di una finestra
#define defMaxWindowParams 5

// struttura che definisce una schermata (window)
typedef struct _TipoStructWindow{
	enumWindowExitCode exitCode;  //codice di uscita dalla finestra
	enumWindowStatus status; // stato della finestra
	unsigned char ucRow; // riga corrente nella finestra
	tipoParamWindow uiParams[defMaxWindowParams];
}TipoStructWindow;

// identificatore di finestra: � un unsigned int
typedef unsigned int tipoIdWindow;

// numero max di finestre
#define defMaxWindow (enumWinId_NumOf+1)//64

#define defMaxCharStringLcd 42
#define defMaxCharKeyPad 42
typedef struct _TipoHandleWindow{
	// stack degli indici delle finestre attive
	tipoIdWindow uiStack[defMaxWindow];
	// stack pointer uiStack[uiSP] contiene indice della finestra attiva
	tipoIdWindow uiSP;
	// questo id � quello della finestra che ha fatto return
	tipoIdWindow uiIdReturner;
	// questo id � quello della finestra che mi ha chiamato
	tipoIdWindow uiIdCaller;
	// questo id � quello della finestra da cui si � fatto il salto
	tipoIdWindow uiIdJumper;
	TipoStructWindow win[defMaxWindow];
	unsigned char ucString[defMaxCharStringLcd];
	unsigned char ucStringNumKeypad_in[defMaxCharKeyPad];
	unsigned char ucStringNumKeypad_out[defMaxCharKeyPad];
}TipoHandleWindow;

// variabile che contiene le strutture per la gestione delle finestre...
extern xdata TipoHandleWindow hw;


// preleva l'id della finestra da cui � partito il salto
tipoIdWindow uiGetIdJumperWindow(void);

// preleva l'id della finestra che ha fatto il call
tipoIdWindow uiGetIdCallerWindow(void);


void vJumpToWindow(tipoIdWindow id);

// svuoto stack
// metto in prima posizione id finestra corrente...
void vEmptyStackCaller(void);

unsigned char ucCallWindow(tipoIdWindow id);

unsigned char ucReturnFromWindow();

// preleva l'id della finestra corrente
tipoIdWindow uiGetIdCurrentWindow(void);

// restituisce lo status della finestra corrente
enumWindowStatus uiGetStatusCurrentWindow(void);

// restituisce lo status della finestra corrente
void vSetStatusCurrentWindow(enumWindowStatus status);

// impostazione di un parametro di una finestra
unsigned char ucSetWindowParam(tipoIdWindow id,unsigned char ucParamIdx, tipoParamWindow uiParamValue);

// legge un parametro di una win
tipoParamWindow uiGetWindowParam(tipoIdWindow id,unsigned char ucParamIdx);

// legge un parametro della win attiva
tipoParamWindow uiGetActiveWindowParam(unsigned char ucParamIdx);

// scrive  un parametro della win attiva
void vSetActiveWindowParam(unsigned char ucParamIdx,tipoParamWindow uiParamValue);




// questa funzione usa come interfaccia la struttura scp
unsigned char ucSearchCodiceProgramma(void);

// max lunghezza stringa unit� di misura
#define defMaxCharUmNumK 14


typedef enum{
		enumUm_none=0,
		enumUm_imp_giro,
		enumUm_mm_giro,
		enumUm_inch_giro,
		enumUm_rpm,
		enumUm_metri_minuto,
		enumUm_feet_minuto,
		enumUm_ampere,
		enumUm_mm,
		enumUm_inch,
		enumUm_inch_feet,
		enumUm_num,
		enumUm_ohm,
		enumUm_ohm_per_metro,
		enumUm_ohm_per_feet,
		enumUm_percentuale,
		enumUm_secondi,
		enumUm_millisecondi,
		enumUm_Volt,
		enumUm_centesimisecondo,

		enumUm_password,
		enumUm_modify_password,

		enumUm_NumOf

	}enumUmType;


#define def_WinYesNo_Param_TipoPulsante 0
#define def_WinYesNo_PUlsantiYesNo 1
#define def_WinYesNo_PUlsanteOk 2


unsigned char ucIsUmSensitive(enumUmType um);

extern code unsigned char tableUm[enumUm_NumOf][defMaxCharUmNumK+1];

// max lunghezza stringa titolo numeric keypad
#define defMaxCharTitleNumK 21
// max lunghezza stringa di picture
#define defMaxCharPicture 21
// significato dei codici di picture
// 'n' --> 0..9
// '.' --> il punto decimale, fisso
// 'N' --> 0..9 oppure punto decimale
// 'x' --> qualsiasi carattere
// 'b' --> carattere blank, fisso
// 'a' --> carattere alfabetico a..z
// 'p' --> carattere alfanumerico e punto decimale a..z 0..9 '.'
// 'S' o 's' --> solo caratteri numerici '1' .. '9'
typedef struct _TipoStructParamsNumK{
		unsigned char ucMaxNumChar;		// numero max caratteri che possono essere inseriti
		unsigned char ucPictureEnable;	// abilitazione stringa picture
		unsigned char ucPicture[defMaxCharPicture]; // stringa di picture
		unsigned char ucMinMaxEnable;	// abilitazione controllo valore minimo/massimo
		unsigned char ucTitle[defMaxCharTitleNumK+1];
		float fMin,fMax;				// valore minimo e massimo ammesso
		unsigned char ucDotInserted;	// indica se dot gi� inserito
		enumUmType enumUm;	// unit� di misura da usare
		unsigned char ucNumLoopPassword;
		unsigned char ucEnableT9;
		unsigned char ucT9_enabled;
		unsigned char ucT9_numLoop;
		unsigned char ucT9_lastButton;
		unsigned char ucT9_found;
		unsigned char ucT9_lastCharIdx;
		unsigned char ucT9_pressedButton;
		unsigned char ucT9_candidate;
		unsigned long ulPasswordEntered;
	}TipoStructParamsNumK;
extern TipoStructParamsNumK xdata paramsNumK;

// controlla valori minimo e massimo, e verifica se sono ok
// restitusice:
// 0--> tutto ok,valore tra minimo e max
// 1--> valore saturato al minimo
// 2--> valore saturato al massimo
unsigned char ucCtrlMinMaxValue(xdata unsigned char *pValue);


// restituisce 1 se il carattere di picture NumK ammette le digit 0..9
unsigned char ucPictureNumK_DigitOk(unsigned char ucIdx);

// restituisce 1 se il carattere di picture NumK ammette il punto decimale
unsigned char ucPictureNumK_DecimalPointOk(unsigned char ucIdx);

void vCalcAnticipoPretaglio(void);
void vTrasferisciDaCommesseAlistaLavori(void);

// gestione numeric keypad
// se ritorna 2--> richiama ancora una volta la gestione finestre, c'� stato un cambio di finestra
unsigned char ucHW_Num_Keypad(void);


// variabili con le info sul codice prg corrente
extern xdata TipoStructHandleInserimentoCodice his;


// restituisce 1 se la struttura his, contiene un codice prodotto valido...
unsigned char ucIsValidHis(TipoStructHandleInserimentoCodice *phis);

// rende valida la struttura his, impostando crc e validkey, per avere un qlcs che ci permetta di dire
// se la struttura � valida o no...
void vValidateHis(TipoStructHandleInserimentoCodice *phis);

// struttura che contiene le info sui campi del codice prodotto
typedef struct _TipoStructInfoFieldCodice{
	unsigned char ucStartField; // indice del carattere di inizio del campo
	unsigned char ucLenField;   // lunghezza del campo
	unsigned char ucPictField[21]; // stringa di picture
	unsigned char ucFmtField[10]; // stringa di formato
	float fMin;						// valore minimo ammesso per il campo
	float fMax;						// valore massimo ammesso per il campo
	unsigned char xdata *pc;		// puntatore alla stringa src/dst delle modifiche
	unsigned char ucPictureEnable;	// abilitazione uso picture
	unsigned char ucMinMaxEnable;	// abilitazione uso min/max
	unsigned char ucTitleNumK[defMaxCharTitleNumK+1]; // stringa del titolo numeri keypad
	enumUmType enumUm;	// unit� di misura da rappresentare...
	unsigned char ucPictFieldInch[21]; // stringa di picture [inches]
	unsigned char ucFmtFieldInch[10]; // stringa di formato [inches]
}TipoStructInfoFieldCodice;

	#define enumStr20_Posizione			0
	#define enumStr20_Numero_pezzi		1
	#define enumStr20_Valore_ohmico		2
	#define enumStr20_Descrizione		3
	#define enumStr20_Modifica_Lavoro	4
	#define enumStr20_Lista_Lavori		5
	#define enumStr20_Inserisci_Lavoro	6
	#define enumStr20_SingleJob			7
	#define enumStr20_L_Mode			8
	#define enumStr20_LR_Mode			9
	#define enumStr20_Ohmm_filo1		10
	#define enumStr20_Ohmm_filo2		11
	#define enumStr20_Ohmm_filo3		12
	#define enumStr20_Res_Spec_Fili		13
	#define enumStr20_Filo				14
	#define enumStr20_diametro_filo		15
	#define enumStr20_tipo_filo			16
	#define enumStr20_diametro_mandrino	17
	#define enumStr20_numero_di_fili	18
	#define enumStr20_tolleranza		19
	#define enumStr20_Seleziona_Codice	20
	#define enumStr20_Maggiorazione		21
	#define enumStr20_Lotto				22
	#define enumStr_Spindle				23
	#define enumStr_Wheel_1				24
	#define enumStr_Wheel_2				25
	#define enumStr_Taglia				26
	#define enumStr_Mem					27
	#define enumStr_Lock				28
	#define enumStr_Locked				29
	#define enumStr_Unlock				30
	#define enumStr_Tara				31
	#define enumStr_Yes					32
	#define enumStr_No					33
	#define enumStr20_MainMenu			34
	#define enumStr20_ListaLavori		35
	#define enumStr20_Produzione		36
	#define enumStr20_CodiceProdotto	37
	#define enumStr20_Utilities			38
	#define enumStr20_ParametriMacchina	39
	#define enumStr20_TaraturaStrumenti	40
	#define enumStr20_Tarature			41
	#define enumStr20_Sel_Funzionamento	42
	#define enumStr20_Numero_max_fili   43
	#define enumStr20_Sel_Encoder		44
	#define enumStr20_Sel_Automatico	45
	#define enumStr20_Impulsi_giro_encoder	46
	#define enumStr20_Sviluppo_Ruota_Mis	47
	#define enumStr20_Unita_di_Misura	48
	#define enumStr20_Vel_Max_Mandrino	49
	#define enumStr20_Vel_Max_Ruota_Co_1	50
	#define enumStr20_Vel_Max_Ruota_Co_2	51
	#define enumStr20_Pz_prima_Fine			52
	#define enumStr20_Spegnimento_Lampada	53
	#define enumStr20_Interasse_1			54
	#define enumStr20_Interasse_2			55
	#define enumStr20_Durata_Min_Allarme	56
	#define enumStr20_DFilo_Esc_Interruz	57
	#define enumStr20_P_Misura_Scarta		58
	#define enumStr20_N_Misure_Scarte		59
	#define enumStr20_FS_Ass_Ruote_Contr	60
	#define enumStr20_Sel_Lingua			61
	#define enumStr20_Versione_CNC			62
	#define enumStr20_Versione_LCD			63
	#define enumStr20_Quinta_Portata		64
	#define enumStr20_Numero_pezzi_prodotti	65
	#define enumStr20_N_spire_pretaglio		66
	#define enumStr20_Diametro_coltello		67
	#define enumStr20_Diametro_ruota_1		68
	#define enumStr20_Diametro_ruota_2		69
	#define enumStr20_Pos_riposo_taglio		70
	#define enumStr20_Altri_Parametri		71
	#define enumStr20_TabellaFornitori		72
	#define enumStr20_toll					73
	#define enumStr20_wire_res_tolerance	74
	#define enumStr20_IngressiAnalogici		75
	#define enumStr20_UsciteAnalogiche		76
	#define enumStr20_IngressiDigitali		77
	#define enumStr20_UsciteDigitali		78
	#define enumStr20_IngressiEncoder		79
	#define enumStr20_StoreSetup			80
	#define enumStr20_MisuraStatica			81
	#define enumStr20_MisuraDinamica		82
	#define enumStr20_SelezionaPortata		83
	#define enumStr20_PortataStat			84
	#define enumStr20_PortataDyn			85
	#define enumStr20_AbilitaTaraturaR1		86
	#define enumStr20_AbilitaTaraturaR2		87
	#define enumStr20_Log					88
	#define enumStr20_SaveParams			89
	#define enumStr20_DataOra				90
	#define enumStr20_LOADPARMAC			91
	#define enumStr20_LoadCodiciProdotto	92
	#define enumStr20_LoadCustomLanguage	93
	#define enumStr20_serialNumber			94
	#define enumStr20_ResetCodiciProdotto	95
	#define enumStr20_CalibraLcd			96
	#define enumStr20_CambioBoccola			97
	#define enumStr20_StartCambioBoccola	98
	#define enumStr20_CalibraSensore		99
	#define enumStr20_SensoreTaglio			100
	#define enumStr20_MisuraStatica_Title	101
	#define enumStr20_CutGain				102
	#define enumStr20_CutDacValue			103
	#define enumStr20_CutPosTolerance		104
	#define enumStr20_CutPosition			105
	#define enumStr20_CutSlowDac			106
	#define enumStr20_cannot_delete			107
	#define enumStr20_prgrunning			108
	#define enumStr20_jobinlist				109
	#define enumStr20_ContrastoLcd			110
	#define enumStr20_Spire_pretag			111
	#define enumStr20_Toll_filo				112
	#define enumStr20_Diam_lama				113
	#define enumStr20_Diam_ruota_1			114
	#define enumStr20_Diam_ruota_2			115
	#define enumStr20_Tara_taglio			116
	#define enumStr20_TitoloAllarmi			117
	#define enumStr20_MeasuredTooBig		118
	#define enumStr20_MeasuredTooLittle		119
	#define enumStr20_Contati				120
	#define enumStr20_Attesi				121
	#define enumStr20_Massimo				122
	#define enumStr20_FirstEnum				123
	#define enumStr20_With_length           enumStr20_FirstEnum
    #define enumStr20_With_ohm				124
    #define enumStr20_mm					125
    #define enumStr20_inch					126
    #define enumStr20_LinguaItaliana		127
    #define enumStr20_LinguaInglese			128
    #define enumStr20_LinguaUlteriore		129
    #define enumStr20_Potenziometri			130
    #define enumStr20_Encoder				131
    #define enumStr20_Disabled				132
    #define enumStr20_Enabled				133
	#define enumStr20_FineCorsa				134
	#define enumStr20_Potenziometro			135
    #define enumStr20_Allarmi				136
    #define enumStr20_Filo_Aggrovigliato	enumStr20_Allarmi
    #define enumStr20_Fault_Freni_2			137
    #define enumStr20_Fault_Ruota_Cont_1	138
    #define enumStr20_Fault_Ruota_Cont_2	139
    #define enumStr20_Interruzione_Filo_1	140
    #define enumStr20_Interruzione_Filo_2	141
    #define enumStr20_Fault_Motore_Mandr	142
    #define enumStr20_Salto_Misura_Ohmica	143
    #define enumStr20_Tempo_Misura_Insuf	144
    #define enumStr20_Filo_Fuori_Toll		145
    #define enumStr20_Misura_Fuori_Portata	146
    #define enumStr20_Fault_Motore_Colt		147
    #define enumStr20_Allarme_13			148
    #define enumStr20_Allarme_14			149
    #define enumStr20_Allarme_15			150
    #define enumStr20_Allarme_16			151
	#define enumStr20_password				152
	#define enumStr20_retype_password		153
	#define enumStr20_ok_password			154
	#define enumStr20_bad_password			155
	#define enumStr20_Up					156
	#define enumStr20_Dn					157
	#define enumStr20_Esc					158
	#define enumStr20_Del					159
	#define enumStr20_DeleteCode			160
	#define enumStr20_ViewCode				161
	#define enumStr20_ViewCodiceProdotto	162
	#define enumStr20_FattoreCorrezione		163
	#define enumStr20_VelProduzione			164
	#define enumStr20_QuotaPenetrazioneTaglio	165
	#define enumStr20_Clr					166
	#define enumStr20_Ok					167
	#define enumStr20_FirstUm				168
	#define enumStr20_Um_none				enumStr20_FirstUm
	#define enumStr20_Um_imp_giro			169
	#define enumStr20_Um_mm_giro			170
	#define enumStr20_Um_inch_giro			171
	#define enumStr20_Um_rpm				172
	#define enumStr20_Um_metri_minuto		173
	#define enumStr20_Um_feet_minuto		174
	#define enumStr20_Um_ampere				175
	#define enumStr20_Um_mm					176
	#define enumStr20_Um_inch				177
	#define enumStr20_Um_inch_feet			178
	#define enumStr20_Um_num				179
	#define enumStr20_Um_ohm				180
	#define enumStr20_Um_ohm_per_metro		181
	#define enumStr20_Um_ohm_per_feet		182
	#define enumStr20_Um_percentuale		183
	#define enumStr20_Um_secondi			184
	#define enumStr20_Um_millisecondi		185
	#define enumStr20_Um_Volt				186
	#define enumStr20_Um_decimi_secondo		187
	#define enumStr20_Ohm					188
	#define enumStr20_SetpointValue			189
	#define enumStr20_MeasuredValue			190
	#define enumStr20_ResetAlarm			191
	#define enumStr20_RestoreProductCodes	192
	#define enumStr20_RestoreCodes			193
	#define enumStr20_RestoreCodesEndsOk	194
	#define enumStr20_ErrorOnProductCodes	195
	#define enumStr20_Reboot				196
	#define enumStr20_ClearAllProdCodes		197
	#define enumStr20_LoadingParams			198
	#define enumStr20_LoadLanguage			199
	#define enumStr20_LoadLanguageEndsOk	200
	#define enumStr20_Restoring				201
	#define enumStr20_RestoreParams			202
	#define enumStr20_RestoreParamsEndsOk	203
	#define enumStr20_Saving				204
	#define enumStr20_InitSave				205
	#define enumStr20_SaveEndsOk			206
	#define enumStr20_DoSave				207
	#define enumStr20_IDG_0					208
	#define enumStr20_IDG_1					209
	#define enumStr20_IDG_2					210
	#define enumStr20_IDG_3					211
	#define enumStr20_IDG_4					212
	#define enumStr20_IDG_5					213
	#define enumStr20_IDG_6					214
	#define enumStr20_IDG_7					215
	#define enumStr20_IDG_8					216
	#define enumStr20_IDG_9					217
	#define enumStr20_IDG_10				218
	#define enumStr20_IDG_11				219
	#define enumStr20_IDG_12				220
	#define enumStr20_IDG_13				221
	#define enumStr20_IDG_14				222
	#define enumStr20_IDG_15				223
	#define enumStr20_ODG_0					224
	#define enumStr20_ODG_1					225
	#define enumStr20_ODG_2					226
	#define enumStr20_ODG_3					227
	#define enumStr20_ODG_4					228
	#define enumStr20_ODG_5					229
	#define enumStr20_ODG_6					230
	#define enumStr20_ODG_7					231
	#define enumStr20_ODG_8					232
	#define enumStr20_ODG_9					233
	#define enumStr20_ODG_10				234
	#define enumStr20_ODG_11				235
	#define enumStr20_ODG_12				236
	#define enumStr20_ODG_13				237
	#define enumStr20_ODG_14				238
	#define enumStr20_ODG_15				239
	#define enumStr20_Lista_Codici			240
	#define enumStr20_DeleteEntryCodeList	241
	#define enumStr20_JobListFull			242
	#define enumStr20_Aggiungi				243
	#define enumStr20_DurataBandella		244
	#define enumStr20_RitardoBandella		245
	#define enumStr20_numero_di_fili_tolleranza_modo	246
	#define enumStr20_ResetRam				247
	#define enumStr20_MeasureOk				248
	#define enumStr20_Versione_Fpga			249
	#define enumStr20_Ethernet_MacAddress	250
	#define enumStr20_RemoteOperation		251
	#define enumStr20_AcceptRemoteControl	252
	#define enumStr20_LampadaAllarme		253
	#define enumStr20_PresenzaDistanziatore	254
	#define enumStr20_Distanza_Coltello_Distanziatore 255
	#define enumStr20_spacer_perc			256
	#define enumStr20_spacer_perc_ends		257
	#define enumStr20_TabellaDistanziatore	258
	#define enumStr20_Distanziatore_NonPresente		259
	#define enumStr20_Distanziatore_Pneumatico		260
	#define enumStr20_Distanziatore_Coltello		261
	#define enumStr20_DelayDistanziatoreDown		262
	#define enumStr20_DistanziatoreGoDown		263
	#define enumStr20_DistanziatorePosition		264
	#define enumStr20_coils						265
	#define enumStr20_CompensazioneTemperatura 266
	#define enumStr20_FaultTemp_ProbeOpen 267
	#define enumStr20_FaultTemp_ProbeUnderrange 268
	#define enumStr20_FaultTemp_ProbeOverrange 269
	#define enumStr20_FaultTemp_Timeout 270
	#define enumStr20_UtilizzaCompensazioneTemperatura 271
	#define enumStr20_Temperature_coefficient 272
	#define enumStr20_StandardValue_at_20C 273
	#define enumStr20_Temperature_coefficient_short 274
	#define enumStr20_title_params_temperature_compensation 275
	#define enumStr20_temperature_compensation_t1 276
	#define enumStr20_temperature_compensation_k1 277
	#define enumStr20_temperature_compensation_t2 278
	#define enumStr20_temperature_compensation_k2 279
	#define enumStr20_temperature_compensation_alfa 280
	#define enumStr20_temp_comp_set_T 281
	#define enumStr20_temp_comp_set_K 282
	#define enumStr20_temp_comp_set_alfa 283
	#define enumStr20_WaitWarmup 284
	#define enumStr20_Mode 285
	#define enumStr20_spindle_grinding 286
	#define enumStr20_spindle_grinding_run 287
	#define enumStr20_spindle_grinding_stop 288
	#define enumStr20_spindle_grinding_run_help 289
	#define enumStr20_spindle_grinding_stop_help 290
	#define enumStr20_reset_pieces_done 291
	#define enumStr20_alarm_wire_tangled_speed 292
	#define enumStr20_Enable_Check_Wire_tangled 293
	#define enumStr20_Speed_Check_Wire_tangled 294
	#define enumStr20_Duration_Check_Wire_tangled 295

	#define enumStr20_NumOf					296

typedef int enumStringhe20char;

typedef enum{
	enumStr40_Pos_NumPezzi_Ohm,
	enumStr40_TempoRes_PezziTot,
	enumStr40_MtMin_Rpm_Ass12,
	enumStr40_Rpm_Precut,
	enumStr40_Rpm_Precut_OhmM,
	enumStr40_Rpm_Precut_Ohm,
	enumStr40_Encoder,
	enumStr40_chvolt,
	enumStr40_codefields,
	enumStr40_WireLengthWorkingJob,
	enumStr40_WireLengthAllOrders,
	enumStr40_InnerBoxTandRTC,
	enumStr40_Rpm_Distanz,
	enumStr40_predist_offset,
	enumStr40_NumOf
}enumStringhe40char;


typedef struct _TipoStructInfoParametro_Lingua{
	unsigned char ucStartField; // indice del carattere di inizio del campo
	unsigned char ucLenField;   // lunghezza del campo
	unsigned char ucPictField[21]; // stringa di picture
	unsigned char ucFmtField[10];  // stringa di formato [mm]
	float fMin;						// valore minimo ammesso per il campo
	float fMax;						// valore massimo ammesso per il campo
	unsigned char xdata *pc;		// puntatore alla stringa src/dst delle modifiche
	unsigned char ucPictureEnable;	// abilitazione uso picture
	unsigned char ucMinMaxEnable;	// abilitazione uso min/max
	enumStringhe20char stringa;
	enumParameterType paramType;
	enumUmType enumUm;
	enumStringType enumIsAstring;
	float fConvUm_mm_toUm_inch;
	//unsigned char ucFmtField_inch[10]; // stringa di formato [inch]
	//unsigned char ucUm_mm[10];		// unit� di misura [mm]
	//unsigned char ucUm_inch[10];    // unit� di misura [inch]
}TipoStructInfoParametro_Lingua;

void vGetValueParametro_spiwin2(unsigned char *pString,unsigned char idxParametro,code TipoStructInfoParametro_Lingua *pif);

extern code unsigned char ucStringhe20char[20][21];

void vString40LangCopy(unsigned char *pdst,enumStringhe40char stringa);
void vStringLangCopy(unsigned char *pdst,enumStringhe20char stringa);
unsigned char *pucStringLang(enumStringhe20char stringa);

// struttura che contiene le info sui campi del codice prodotto, con gestione della lingua
typedef struct _TipoStructInfoFieldCodice_Lingua{
	unsigned char ucStartField; // indice del carattere di inizio del campo
	unsigned char ucLenField;   // lunghezza del campo
	unsigned char ucPictField[21]; // stringa di picture
	unsigned char ucFmtField[10]; // stringa di formato
	float fMin;						// valore minimo ammesso per il campo
	float fMax;						// valore massimo ammesso per il campo
	unsigned char xdata *pc;		// puntatore alla stringa src/dst delle modifiche
	unsigned char ucPictureEnable;	// abilitazione uso picture
	unsigned char ucMinMaxEnable;	// abilitazione uso min/max
	enumStringhe20char stringa; // stringa che contiene il titolo da visualizzare nel numeric keypad
	enumUmType enumUm;	// unit� di misura da rappresentare...
	unsigned char ucPictFieldInch[21]; // stringa di picture [inches]
	unsigned char ucFmtFieldInch[10]; // stringa di formato [inches]
}TipoStructInfoFieldCodice_Lingua;


extern code TipoStructInfoFieldCodice_Lingua ifc[5];


// finestra di gestione della selezione/inserimento codice prodotto
unsigned char ucHW_codiceProdotto(void);


// macro che permette di impostare la posizione cursore
#define defSetWinParam(param,value) vSetActiveWindowParam(param,value);
// macro che permette di leggere la posizione cursore
#define defGetWinParam(param) uiGetActiveWindowParam(param)

unsigned char ucIamInTaraturaDinamica(void);
unsigned char ucIamInTaraturaStatica(void);


#define defMaxCharLavoroString_Position 2
#define defMaxCharLavoroString_Numpezzi 5
#define defMaxCharLavoroString_Ohm 6
typedef struct _TipoPrivatoLavoro{
	TipoLavoro lavoro;
	unsigned char ucStringPosition[defMaxCharLavoroString_Position+1];
	unsigned char ucStringNumpezzi[defMaxCharLavoroString_Numpezzi+1];
	unsigned char ucStringOhm[defMaxCharLavoroString_Ohm+1];
	unsigned char ucStringDescrizione[defMaxCharDescrizioneLavoro+1];
}TipoPrivatoLavoro;
extern xdata TipoPrivatoLavoro privato_lavoro;

extern code TipoStructInfoFieldCodice_Lingua iml[4];


// numero di lavori da visualizzare in lista lavori
#define defNumLavoriDaVisualizzare 3


// sbianca il record relativo al lavoro con indice ucIdx
void vEmptyLavoroInLista(unsigned char ucIdx);
// finestra di gestione della modifica lavoro...
unsigned char ucHW_modificaLavoro(void);

// funzione per trasferire da commessa a lista pJobsSelected_Jobs->..
void vTrasferisciDaCommesseAlistaLavori(void);
void vTrasferisciDaListaLavoriAcommesse(void);
void vSearchFirstComm(void);

// finestra di gestione della selezione/inserimento lista pJobsSelected_Jobs->..
unsigned char ucHW_listaLavori(void);

#define defMaxCharOhmMFilo 7

// per inserimento ohm/m filo
extern code TipoStructInfoFieldCodice_Lingua im_ohmm_fili[3];

unsigned char ucHW_inserisciOhmMfili(void);

// calcola tempo residuo a finire commessa
void vCalcTempoResiduo(void);



// tipi di regolazione di velocit� possibili tramite encoder:
// spindle, wheel1, wheel2
typedef enum{
			enumSpeedRegulation_Spindle=0,
			enumSpeedRegulation_Wheel1,
			enumSpeedRegulation_Wheel2,
			enumSpeedRegulation_NumOf
		}enumSpeedRegulationTypes;

extern enumSpeedRegulationTypes xdata ucLinkEncoder;

// disabilitazione della gestione del potenziometro
void vDisableHRE_potentiometer(void);
// impostazione del valore del potenziometro simulato dall'encoder
void vSetHRE_potentiometer(enumSpeedRegulationTypes regulationType,int iValue);
// selezione dell'associazione tra encoder e potenziometro simulato
void vSelectHRE_potentiometer(enumSpeedRegulationTypes regulationType);
// restituisce l'indirizzamento attuale dell'encoder di regolazione velocit�: spindle, wheel1 o wheel2?
unsigned char ucGetSelectHRE_potentiometer(void);
// lettura del valore del potenziometro simulato dall'encoder
int iGetHRE_potentiometer(enumSpeedRegulationTypes regulationType);
// gestione del potenziometro simulato dall'encoder
void vHandleHRE(void);
// restituisce l'impostazione attuale dell'encoder di regolazione
// pu� indicare rpm se � selezionato il mandrino
// altrimenti indica la percentuale di maggiorazione della velocit� della ruota di contrasto 1/2
int iGetHRE_act_setting(void);
// restituisce l'impostazione attuale di uno degli encoder di regolazione
int iGetHRE_setting(enumSpeedRegulationTypes regulationType);

extern unsigned char ucHW_LogProduzione(void);
extern unsigned char ucHW_TaraDistanziatore(void);
extern unsigned char ucHW_listaCodici();
// puntatore alla lista lavori corrente
extern xdata TipoStructLavori *pJobsSelected_Jobs,*pJobsRunning_Jobs;
extern xdata TipoStructCodeJobList *pJobsSelected,*pJobsRunning;
unsigned char ucInsertCodeInJobList(unsigned char *pucCodice,unsigned char ucManualJob);
// permette di verificare se esiste nella joblist un codice
// restituisce l'idx se joblist contiene il codice, 0xff altrimenti
unsigned char ucIdxCodeInJobList(unsigned char *pucCodice);
// cancellazione di un codice dalla joblist
unsigned char ucDeleteCodeInJobList(unsigned char *pucCodice);
// effettua il purge della coda dei job
unsigned char ucPurgeJobList(void);
// rinfresca tutti i link al codice prodotto corrente...
void vRefreshActiveProductCode(void);
// sostituisco nella joblist il codice programma attivo
// serve ad es quando viene cambiata la tolleranza nella pagina di setup produzione
void vUpdateActiveCodeInJobList(unsigned char *pucCodice);

// indice del parametro della finestra archivio prodotti
// che indica se si deve visualizzare o no il button "delete codice"
#define defIdxParam_ArchivioProdotti_showDeleteButton 4
// indice del parametro della finestra archivio fornitori filo (tabella tolleranza filo)
// che indica se pu� modificare la tabella
#define defIdxParam_enumWinId_Fornitori_ModifyEnable 3
// indice del parametro della finestra archivio fornitori filo (tabella tolleranza filo)
// che contiene il filo scelto (0xFF=nessuno)
#define defIdxParam_enumWinId_Fornitori_Choice 4

// list the parameters we can input for a program 
typedef enum{
	enum_vcp_preset_param_coeff_corr=0,
	enum_vcp_preset_param_coeff_temperature,
	enum_vcp_preset_param_numof
}enum_vcp_preset_param;
// prepares for inputting parameter of a product code
void v_preset_for_program_param_input(enum_vcp_preset_param preset_param);
