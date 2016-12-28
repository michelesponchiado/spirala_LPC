#ifndef defGuiHIncluded
#define defGuiHIncluded


// utilizzare questa define per partire con la routine di calibrazione lcd...
//#define defCalibraLcd	


// definizione dei font
typedef enum{
// ****
// **** FONT PICCOLO
// ****
        enumFont_8x8=0,    // font  8(larghezza)x8(altezza)   --> icone	X
							//	font per scritte piccole					X
		enumFontSmall=enumFont_8x8,
							//
// ****
// **** FONT MEDIO
// ****
        enumFont_16x16,     // font 16(larghezza)x16(altezza)   --> icone	XX
							//												XX
							//		font per scritte dimensione media		XX
							//
		enumFontMedium=enumFont_16x16,

// ****
// **** FONT MEDIO GRANDE
// ****
        enumFont_24x24,     // font 24(larghezza)x24(altezza)   --> icone	XX
							//												XX
							//		font per scritte dimensione media		XX
							//
		enumFontMediumBig=enumFont_24x24,
							//
							//
// ****
// **** FONT GRANDE
// ****
        enumFont_32x32,     // font 32(larghezza)x32(altezza)   --> bitmap	XXXX
							//												XXXX
							//	font per scritte grandi						XXXX
							//												XXXX
		enumFontBig=enumFont_32x32,
							//
							//
// ****
// **** FONT MOLTO GRANDE
// ****
        enumFont_40x40,     // font 32(larghezza)x32(altezza)   --> bitmap	XXXX
							//												XXXX
							//	font per scritte grandi						XXXX
							//												XXXX
		enumFontVeryBig=enumFont_40x40,
							//
							//
        enumNumOfFonts
    }enumFontType;
    
    
// dimensione dei font in pixel (prima la larghezza X, poi la ampiezza Y)
extern xdata unsigned char ucFontSizePixelXY[enumNumOfFonts][2];
// dimensione dei font in righe/colonne da 8 pixel
extern xdata unsigned char ucFontSizeRowXY[enumNumOfFonts][2];

// macro che restituisce la dimensione di un font in pixel, dato il tipo di font
#define defFontPixelSizeFromFontType(fontType) ((fontType+1)*8U)


// struttura che contiene numero righe/colonne che ci stanno sull'intera area del display,
// in funzione del font
typedef struct _TipoStructFontsNumRowCols{
	unsigned char ucMaxNumRows;
	unsigned char ucMaxNumCols;
}TipoStructFontsNumRowCols;

// struttura che contiene numero righe/colonne che ci stanno sull'intera area del display,
// in funzione del font
typedef struct _TipoStructOffsetXYfont{
	unsigned char ucOffsetX;
	unsigned char ucOffsetY;
}TipoStructOffsetXYfont;

// struttura che contiene info su immagine correntemente visualizzata a display
typedef struct _TipoStructInfoImmagine{
	unsigned char ucCurImage;
	unsigned char ucPrevImage;
}TipoStructInfoImmagine;
    


// definizione dei codici dei colori
// BLACK=0 0 0 --> (332) 000 000 00 -->0000 0000=0x00
#define defLCD_Color_Black				0x00	 
// BLUE=0 0 139 --> (332) 000 000 10 -->0000 0010=0x02
#define defLCD_Color_DarkBlue			0x02
// green=0 128 0  --> (332) 000 100 00 -->0001 0000=0x10
#define defLCD_Color_Green				0x10
// cyan=0 255 255  --> (332) 000 111 11 -->0001 1111=0x1F
#define defLCD_Color_Cyan				0x1F
// dark green=0 100 0  --> (332) 000 010 00 -->0000 1000=0x08
#define defLCD_Color_DarkGreen		0x08
// BLUE=0 0 255 --> (332) 000 000 11 -->0000 0011=0x03
#define defLCD_Color_Blue			0x03
// lime=0 255 0--> (332) 000 111 00 -->0001 1100=0x1C
#define defLCD_Color_LightGreen		0x1C
// light blue=173 216 230--> (332) 101 110 11 -->1011 1011=0xBB
#define defLCD_Color_LightBlue		0xBB		
// red=255 0 0--> (332) 111 000 00 -->1110 0000=0xE0
#define defLCD_Color_Red			0xE0
// magenta=255 0 255--> (332) 111 000 11 -->1110 0011=0xE3
#define defLCD_Color_Magenta			0xe3
// orange=255 165 0--> (332) 111 101 00 -->1111 0100=0xF4
#define defLCD_Color_Orange			0xf4
// pink=255 192 203--> (332) 111 110 11 -->1111 1011=0xFB
#define defLCD_Color_Pink				0xfb
// maroon=128 0 0--> (332) 100 000 00 -->1000 0000=0x80
#define defLCD_Color_Maroon		0x80
// gray=128 128 128--> (332) 100 100 10 -->1001 0010=0x92
#define defLCD_Color_Gray	0x92

// YELLOW=255 255 0 --> (332) 111 111 00 -->1111 1100=0xFC
#define defLCD_Color_Yellow			0xFC
// WHITE=255 255 255 --> (332) 111 111 11 -->1111 1111=0xFF
#define defLCD_Color_White		    0xFF
// trasparenyte=0xFFFF
#define defLCD_Color_Trasparente	0xFFFF




typedef enum {
			eventCallback_Created=0,
			eventCallback_Touched=1,
			eventCallback_Destroyed=0xFF
		}enumEventsCallback;    
    
// definizione della stringa di picture (è una stringa costante, pertanto la metto in memoria codice)
typedef unsigned char TipoPictureChar;
// definzione del tipo: carattere della stringa a video
typedef unsigned char TipoDisplayChar;
// riga a cui comincia la visualizzazione del carattere
typedef unsigned int TipoPosizioneDisplayRigaFont;
// colonna a cui comincia la visualizzazione del carattere
typedef unsigned int TipoPosizioneDisplayColonnaFont;
// primo offset di visualizzazione del carattere nel display
typedef unsigned int TipoOffsetDisplay;
// primo parametro  della funzione di callback, da chiamare quando il touch segnala la pressione nell'area del testo
typedef enumEventsCallback TipoParam1CallBack;
// area di touch del testo; le posizioni sono in pixel
typedef struct _TipoStructTouchArea{
		// x varia fra 0 e defLcdWidthX_pixel=320
		// y varia fra 0 e defLcdWidthY_pixel=240
		unsigned int x1Pixel,y1Pixel,x2Pixel,y2Pixel;
      // indica se il touch screen è abilitato per il testo
      unsigned char ucEnable;
    }TipoStructTouchArea;

#define defBitTextProperties_Transparent 0x1
// proprietà del testo; lampeggio etc
typedef struct _TipoStructTextProperties{
        unsigned char ucFlashing;
        unsigned char ucFlashTime;
		// colore di sfondo del testo
        unsigned char ucBackGroundColor;
		// colore di primo piano del testo
        unsigned char ucForeGroundColor;
		// colore di sfondo originale del testo
        unsigned char ucBackGroundColorSave;
		// colore di primo piano originale del testo
        unsigned char ucForeGroundColorSave;
		// altre proprietà
        unsigned char ucProperties;
    }TipoStructTextProperties;
// funzione di callback, da chiamare quando il touch segnala la pressione nell'area del testo
// evento 0=creazione
// evento 0xFF= distruzione
// evento 1= touch
// descritti da tipo enumEventsCallback
// viene poi passato puntatore alla struttura testo, in modo che si possa
// assegnare la stessa callback a più testi e poi decidere quale ha ricevuto l'evento (gestione menu)
typedef void(*TipoFunCallBackTouch)(enumEventsCallback ucParam1,void *pTesto);


// tipi possibili di button
typedef enum{
				enumButton_None  =0,  //--> testo semplice senza button intorno
				enumButton_Normal=1,  //--> button normale, testo con un button attorno
				enumButton_Field =2,  //--> button diviso a campi...
				enumButton_Bitmap=3,  //--> button a bitmap, non ha testo all'interno
				enumButton_Static=4,  //--> button contorno ovale, non cliccabile...
				enumButton_StaticAlarm=5,  //--> button contorno ovale, non cliccabile, rosso
				enumButtonType_NumOf
			}enumButtonType;

// maschera che identifica il flag di un pulsante trasparente
#define defFlagButtonTransparent 0x1
// macro per individuare se button trasparente
#define defIsButtonTransparent(theFlag) ((theFlag)&defFlagButtonTransparent)
// macro per impostare button trasparente
#define defSetButtonTransparent(theFlag) {(theFlag)|=defFlagButtonTransparent;}

// maschera che identifica il flag di un pulsante premuto
#define defFlagButtonPressed 0x2
// macro per individuare se button premuto
#define defIsButtonPressed(theFlag) ((theFlag)&defFlagButtonPressed)
// macro per impostare button pressed
#define defSetButtonPressed(theFlag) {(theFlag)|=defFlagButtonPressed;}
// macro per resettare button pressed
#define defSetButtonUnpressed(theFlag) {(theFlag)&=~defFlagButtonPressed;}

// maschera che identifica il flag di un pulsante che ha il focus
#define defFlagButtonFocused 0x4
// macro per individuare se button ha focus
#define defIsButtonFocused(theFlag) ((theFlag)&defFlagButtonFocused)
// macro per impostare button focused
#define defSetButtonFocused(theFlag) {(theFlag)|=defFlagButtonFocused;}

typedef enum{
	enumBitmap_40x24_arrow_left,
	enumBitmap_40x24_arrow_right,
	enumBitmap_40x24_arrow_dn,
	enumBitmap_40x24_arrow_up,
	enumBitmap_40x24_ESC,
	enumBitmap_40x24_CUT,
	enumBitmap_40x24_OFF,
	enumBitmap_40x24_ON,
	enumBitmap_40x24_OK,
	enumBitmap_40x24_backspace,
	enumBitmap_40x24_dot,
	enumBitmap_40x24_cursorRight,
	enumBitmap_40x24_secondrow,
	enumBitmap_40x24_0=enumBitmap_40x24_secondrow,
	enumBitmap_40x24_1,
	enumBitmap_40x24_2,
	enumBitmap_40x24_3,
	enumBitmap_40x24_4,
	enumBitmap_40x24_5,
	enumBitmap_40x24_6,
	enumBitmap_40x24_7,
	enumBitmap_40x24_8,
	enumBitmap_40x24_9,
	enumBitmap_40x24_DnArrow,
	enumBitmap_40x24_UpArrow,

	enumBitmap_40x24_thirdrow,
	enumBitmap_40x24_abc=enumBitmap_40x24_thirdrow,
	enumBitmap_40x24_def,
	enumBitmap_40x24_ghi,
	enumBitmap_40x24_jkl,
	enumBitmap_40x24_mno,
	enumBitmap_40x24_pqrs,
	enumBitmap_40x24_tuv,
	enumBitmap_40x24_wxyz,
	enumBitmap_40x24_123,
	enumBitmap_40x24_space,
	enumBitmap_40x24_other_chars,
	enumBitmap_40x24_Type_NumOf
}enumBitmap_40x24_Type;


// numero max di campi in un button
#define defMaxButtonFields 5

// struttura che definisce alcune proprietà di un button
typedef struct _TipoButtonStruct{
	unsigned char ucType;   // tipo del bottone che deve contornare il testo; 0=nessuno
	unsigned char ucNum;    // numero del bottone associato al testo
	unsigned char ucBitmap; // bitmap associata al bottone
	// numero di fields
	unsigned char ucNumOfButtonFields;
	// caratteri di inizio dei fields
	unsigned char ucCharStartField[defMaxButtonFields];
	// lunghezza dei fields
	unsigned char ucLenField[defMaxButtonFields];
	// flag del button nel field
	unsigned char ucFlags[defMaxButtonFields];
}TipoButtonStruct;

// struttura con tutte le proprietà di un testo su display
typedef struct _TipoTestoOnDisplay{
        TipoDisplayChar xdata * pucDisplayString;
        TipoPictureChar xdata * pucPictureString;
        TipoPosizioneDisplayRigaFont riga;
        TipoPosizioneDisplayColonnaFont colonna;
        enumFontType ucTipoFont;
        unsigned char ucId;	// identificatore del testo a display
		TipoButtonStruct button;
        TipoStructTouchArea structTouchArea;
	    unsigned char ucCharOnTextSelected; // quale carattere nel testo è stato selezionato col touch?
		TipoStructTextProperties structProperties;
		struct _TipoTestoOnDisplay xdata *pTestoOnQueue;
    }TipoTestoOnDisplay;
    
// struttura che contiene le impostazioni di default del testo
typedef struct _TipoDefaultTextSettings{
	TipoStructTextProperties structProperties;
   enumFontType ucTipoFont;
}TipoDefaultTextSettings;
    
    
// struttura con tutte le proprietà del cursore
typedef struct _TipoCursoreOnDisplay{
		// puntatore al testo che contiene il cursore...
        TipoTestoOnDisplay xdata *pTestoCursore;
		// quale carattere del testo deve lampeggiare???
        unsigned char ucOffsetCursoreNelTesto;
        unsigned char ucEnabled;
        unsigned char ucFlashTime;
        unsigned char ucFlashStatus;
        unsigned char ucTouchPressed;
        unsigned char ucTouchWasPressed;
		unsigned int posX,posY;
    }TipoCursoreOnDisplay;
extern xdata unsigned int uiCursorePosXnoLin, uiCursorePosYnoLin;
extern xdata unsigned int uiCursorePosX, uiCursorePosY;


// elementi della coda delle strutture testo a display
typedef struct _TipoStructTextOnDisplayQueue{
	struct _TipoStructTextOnDisplayQueue xdata *pNext;
	struct _TipoStructTextOnDisplayQueue xdata *pPrev;
   TipoTestoOnDisplay xdata *pTesto;
}TipoStructTextOnDisplayQueue;
 
 
// Enumerazione dei possibili motivi per cui viene chiamato il rinfresco lcd
// è la descrizione del parametro della vRefreshLcd
typedef enum{enumRefreshLcd_Timeout=0,
				 enumRefreshLcd_Touched,
				 enumRefreshLcd_Force_Refresh,
				 
				 enumNumOfRefreshLcdEvents
				} enumTipoEventoRefreshLcd;

// inizializzazione lcd
void vInitLcd(void);
// test lcd
void vTestLcd(void);
// aggiornamento dello screen lcd
// puo' essere chiamato col parametro:
// 0 per indicare che è scaduto il timeout di aggiornamento screen
// 1 per indicare che è stato premuto il touch
void vRefreshLcd(enumTipoEventoRefreshLcd ucRefreshLcd_Event);
// funzione che controlla in background se è stato premuto lcd
// ritorna 0 se NON è stato premuto
// ritorna 1 se è stato premuto; prima di ritornare chiama un refresh lcd
unsigned char ucBackGroundCtrlLcd(void);

// inizializza immagine di sfondo
void vInitImmagine(void);
// funzione di caricamento immagine; non viene visualizzata finché non si chiama la rinfresca immagine
void vCaricaImmagine(unsigned char ucNumImmagine);
// forza un rinfresco dell'immagine a video
void vForceRefreshImmagine(void);
// restituisce 1 se immagine di sfondo cambiata
unsigned char ucImmagineCambiata(void);
// rinfresca immagine a video e fa toggle del banco immagine
void vRinfrescaImmagine(void);
// cancellazione di un testo a video...
// 1 se trovato e cancellato, 0 se non trovato o non cancellato
unsigned char ucDeleteTesto(TipoTestoOnDisplay xdata * pTesto);
// funzione che permette di modificare un testo che appare a video
unsigned char ucChangeText(TipoTestoOnDisplay xdata*ptesto, unsigned char xdata *pstr);
// funzione che permette di spostare un testo che appare a video
unsigned char ucMoveText(TipoTestoOnDisplay xdata*ptesto, TipoPosizioneDisplayRigaFont riga, TipoPosizioneDisplayColonnaFont colonna);


// inizializzazione valori di default del testo: font e colore
void vResetTextSettings(void);
// funzione che permette di selezionare il font del testo di default
void vSelectFont(enumFontType ucFont);
// funzione che permette di selezionare il colore di default del testo
void vSelectTextColor(unsigned char ucColor);
// funzione che permette di selezionare il colore di default del background testo 
void vSelectBackColor(unsigned char ucColor);
// funzione che stampa a video un testo, usando i valori di default per font e colori
// questo testo è statico e non puo' essere modificato, ma solo cancellato dalla clear screen
// restituisce l'id del testo
unsigned char ucPrintStaticText(xdata char *stringa,TipoPosizioneDisplayRigaFont row,TipoPosizioneDisplayColonnaFont col);
// funzione che stampa a video un testo, usando i valori di default per font e colori
// e inserendo abilitazione touch e funzione di callback
// restituisce l'id del testo e 
unsigned char ucPrintTextTouch(TipoTestoOnDisplay xdata *ptesto,
										 xdata char *stringa, 
										 TipoPosizioneDisplayRigaFont row, 
										 TipoPosizioneDisplayColonnaFont col, 
										 unsigned char ucEnTouch
										 //,TipoFunCallBackTouch pFunCallBackTouch
										 );


// fa un bel clear screen
unsigned char ucClearScreen(void);

// chiusura lcd 
void vCloseLCD(void);

// numero di righe/colonne nel font
extern xdata TipoStructFontsNumRowCols ucFontNumRowCols[enumNumOfFonts];
// offset font
extern xdata TipoStructOffsetXYfont ucFontOffset[enumNumOfFonts];
 
// variabile per test touch screen
extern TipoTestoOnDisplay xdata testoTouch;
extern TipoTestoOnDisplay xdata testoPrintDefault;
// testo per eseguire verifiche display touch screren
extern TipoTestoOnDisplay xdata testoCallback;
// stringa per eseguire verifiche display touch screen
extern TipoDisplayChar xdata stringaTestTouch[];
#define defMaxCharTestoScrivi 40
extern TipoDisplayChar xdata stringaTestoScrivi[];
extern TipoTestoOnDisplay xdata testoScrivi;
extern xdata unsigned char idxStringaTestoScrivi;
// funzione che stampa a video un testo, usando i valori di default per il cursore
unsigned char ucPrintCursorText(	char myChar,
									TipoPosizioneDisplayRigaFont row, 
									TipoPosizioneDisplayColonnaFont col,
									enumFontType fontType
									);

void vSignalNewKeyPressed(unsigned char ucKey);	
unsigned char ucNewKeyPressed(void);
unsigned char ucGetKeyPressed(void);
// funzione che stampa a video un testo, contornato da button usando i valori di default per font e colori
unsigned char ucPrintStaticButton(	xdata char *stringa,
									TipoPosizioneDisplayRigaFont row, 
									TipoPosizioneDisplayColonnaFont col,
									enumFontType fontType,
									unsigned char ucNumButton,
									unsigned int uiColorButton
								);
extern void vSignalButtonPressed(unsigned char ucNumButton,unsigned char ucNumCharPressed);

// permette di definire un button di tipo field, con vari campi al suo interno...
unsigned char ucPrintFieldButton(	xdata char *stringa,
									TipoPosizioneDisplayRigaFont row, 
									TipoPosizioneDisplayColonnaFont col,
									unsigned char ucNumButton,
									TipoButtonStruct xdata *pbutton,
									unsigned char ucBackGroundColor

								);


// indice del field cui appartiene il carattere del button che è stato premuto...
unsigned char ucFieldButtonPressed(unsigned char ucNumButton);

// impostazione dell' indice del field cui appartiene il carattere del button che è stato premuto...
void vSetFieldButtonPressed(unsigned char ucNumButton, unsigned char ucFieldPressed);

// funzione che stampa a video un testo small per dare nomi a colonne tabella
unsigned char ucPrintSmallText_ColNames(xdata char *stringa,TipoPosizioneDisplayRigaFont row, TipoPosizioneDisplayColonnaFont col);


// funzione che stampa a video un button bitmap
unsigned char ucPrintBitmapButton(	enumBitmap_40x24_Type bitmapType,
									TipoPosizioneDisplayRigaFont row, 
									TipoPosizioneDisplayColonnaFont col,
									unsigned char ucNumButton
								);

void vEnableCalibraLcd(unsigned char ucEnable);
#endif //#ifndef defGuiHIncluded

// funzione che stampa a video un testo, usando font medium e colori specificati
unsigned char ucPrintStaticText_MediumFont_Color(xdata char *stringa,
												 TipoPosizioneDisplayRigaFont row, 
												 TipoPosizioneDisplayColonnaFont col,
												 unsigned char ucForeColor,
												 unsigned char ucBackColor
												 );

unsigned char ucPrintSmallText_ColNames_BackColor(xdata char *stringa,TipoPosizioneDisplayRigaFont row, TipoPosizioneDisplayColonnaFont col,unsigned char ucBackColor);
