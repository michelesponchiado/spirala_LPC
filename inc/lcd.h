// lcd.h
#ifndef defLcdH_Included
#define defLcdH_Included

#include "guideclar.h"
void LCD_SpiAccess(unsigned char data_to_be_sent, unsigned char xdata *data_read);
void LCD_wait_FPGA_done(void);
void LCD_toggle_bank(void);
void LCD_LoadImageOnBackgroundBank(unsigned char ucImageNumber);
unsigned char ucTouchedScreen(unsigned int xdata*pPosX,unsigned int xdata*pPosY);
 // inizializzazione lcd e spi per comunicazione verso display
void vLCD_Init(void);
// reset del piedino INT dell'lcd
void vResetIntLCD_spi(void);
// scrittura delle icone a video
void vLCD_Send_My_icon_map(void);
// scrittura delle bitmap a video
void vLCD_Send_My_bitmap_map(void);
// reset mappa icone e bitmap
void vResetIconsAndBitmap(void);
void LCD_Beep(void);
void vSelect12bitTouch(void);
void vSelect8bitTouch(void);


typedef enum{	enumElementoIcona=0,
				enumElementoNextIcona=1,
				enumElementoBitmap=2,
				enumNumTipiElementiInLista
			}enumTipoElementoInLista;

#define defLCD_MaxCharListaD (2048)
#define defLCD_MarginCharListaD (256)
typedef struct _TipoStructLCD_Lista{
	unsigned char ucLCD_Lista[defLCD_MaxCharListaD];
	unsigned char *puclista;
	unsigned int uiLength;
    unsigned int ui_valid;
	enumTipoElementoInLista lastTipo;
}TipoStructLCD_Lista;

unsigned char ucTroppoPienaListaLcd(void);

extern xdata TipoStructLCD_Lista LCD_Lista;
extern xdata TipoStructLCD_Lista * data pListLcd;



typedef struct _TipoStructParams_Icon_Lista{
	unsigned int  uiXpixel;
	unsigned char uiYpixel;
	unsigned char ucFont;
	unsigned char xdata *pucVettore;
	unsigned char ucIconColor; 
	unsigned char ucBackgroundColor;
	unsigned char ucTransparent;
}TipoStructParams_Icon_Lista;

extern xdata TipoStructParams_Icon_Lista Params_Icon_Lista;
extern xdata TipoStructParams_Icon_Lista * xdata pParamsIcon;


typedef struct _TipoStructParams_Bitmap_Lista{
	unsigned int  uiXpixel;
	unsigned char uiYpixel;
	unsigned char ucRipX,ucRipY;
	unsigned int uiAddress;
}TipoStructParams_Bitmap_Lista;

extern xdata TipoStructParams_Bitmap_Lista Bitmap_Lista;
extern xdata TipoStructParams_Bitmap_Lista * xdata pBitmapLista;

void vWriteBMPelement(TipoStructParams_Bitmap_Lista *pb);
void vWriteIconElement(TipoStructParams_Icon_Lista xdata *pp);
void vInitListDisplayElements(void);
// reset mappa icone e bitmap
void vResetIconsAndBitmap(void);
void vCloseListDisplayElements(void);
void invio_lista(void);
void applicazione_lista(void);
void lettura_versione_FW(unsigned char xdata *FW1,unsigned char xdata *FW2);

// scrive un testo, però max N caratteri
void vWriteIconElement_N(TipoStructParams_Icon_Lista xdata *pp, unsigned char ucN);

// indica se è stato rilasciato il tastod i touch screen
unsigned char ucTouchScreenUnPressed(void);
// indica se è stato premuto il tasto di touch screen
unsigned char ucTouchScreenPressed(void);




#endif //#ifndef defLcdH_Included
