// modulo di gestione a basso livello comunicazione con lcd

#ifndef ARM9platform
    #include "upsd3400.h"
    #include "upsd3400_hardware.h"
    #include "upsd3400_timer.h"
    #include "intrins.h"
    #include <alloc.h>
#endif    
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "fpga_nxp.h"
#include "lcd.h"


// ATTIVO reset spi lcd
#define defResetSpiLcd	{spi_lcd_reset_low=0;}
// DISATTIVO reset spi lcd
#define defEndResetSpiLcd	{spi_lcd_reset_low=1;}
// scrive 1 su P4.0 in modo che sia accessibile in lettura
#define defDoINTinput	{}
// test se pin INT da fpga attivo
#define defLcdINTactive	(spi_lcd_int_low==0)


typedef struct _TipoStructLCD{
	unsigned char ucVisualizeBank;
	unsigned char ucBackgroundBank;
}TipoStructLCD;

xdata TipoStructLCD structLCD;
xdata unsigned char ucTouchScreen_unPressed;
xdata TipoStructLCD_Lista LCD_Lista;
xdata TipoStructLCD_Lista * data pListLcd;
xdata TipoStructParams_Icon_Lista Params_Icon_Lista;
xdata TipoStructParams_Icon_Lista * xdata pParamsIcon;
xdata TipoStructParams_Bitmap_Lista Bitmap_Lista;
xdata TipoStructParams_Bitmap_Lista * xdata pBitmapLista;
xdata unsigned char ucRememberLcdIntActive;


// indica se � stato premuto il tasto di touch screen
unsigned char ucTouchScreenPressed(void){
	return !ucTouchScreen_unPressed;
}

#if 0
	// indica se � stato rilasciato il tasto di touch screen
	unsigned char ucTouchScreenUnPressed(void){
		return ucTouchScreen_unPressed;
	}
#endif


// inizializzazione lcd e spi per comunicazione verso display
void vLCD_Init(void){
// unsigned int i;
	// ATTIVO reset spi lcd
	defResetSpiLcd;
// piccolo ritardo per comandare reset sufficientemente lungo...
    v_wait_microseconds(10000);
    // for (i=0;i<1000;i++)
        // if (ucFpgaFirmwareVersion_Low==0xaa)
            // break;
	// al reset viene visualizzato il banco 1, e si scrive sul banco 0
	structLCD.ucVisualizeBank=1;
	structLCD.ucBackgroundBank=0;
	ucTouchScreen_unPressed=0;
	// DISATTIVO reset spi lcd
	defEndResetSpiLcd;

}



// funzione di test del touch screen
// restituisce 1 se touch screen � stato premuto
// e ritorna in posX posY la posizione del touch
// *pPosX � un intero, la posizione restituita varia tra 0 e 320
// *pPosY � un intero, la posizione restituita varia tra 0 e 240
unsigned char ucTouchedScreen(unsigned int xdata*pPosX,unsigned int xdata*pPosY){
	unsigned char xdata dummy,ucControlOldPosition;
	static unsigned int xdata oldPosX,oldpPosY,save[3];
#if 1	
	if (!defLcdINTactive&&!ucRememberLcdIntActive)
		return 0;//attesa discesa segnale dalla FPGA
	ucControlOldPosition=0;
	if (!defLcdINTactive){
		ucControlOldPosition=1;
	}		
	ucRememberLcdIntActive=0;
#else
	ucControlOldPosition=0;
	if (!defLcdINTactive)
		return 0;//attesa discesa segnale dalla FPGA
#endif	

	// lettura a 8 bit del touch
	LCD_SpiAccess(0x60,&dummy);		
	LCD_SpiAccess(0x00,&dummy);		
	save[1]=dummy;
	// 240/256 =15/16
	*pPosY=(save[1]*15)>>4;
	LCD_SpiAccess(0x00,&dummy);		
	save[0]=dummy;
	// 320/256 =20/16
	*pPosX=(save[0]*20)>>4;
	LCD_SpiAccess(0x6F,&dummy);//fa salire segnale di interrupt
	   
	if ((*pPosX==0)&&(*pPosY==0)){
		ucTouchScreen_unPressed=1;
		oldPosX=0;
		oldpPosY=0;
		return 0;
	}	
	ucTouchScreen_unPressed=0;
	if (ucControlOldPosition){
		if ((*pPosX==oldPosX)&&(*pPosY==oldpPosY)){
			return 0;
		}	
	}

	oldPosX=*pPosX;
	oldpPosY=*pPosY;

	return 1;
}//unsigned char ucTouchedScreen(unsigned int xdata*pPosX,unsigned int xdata*pPosY)
	
	
#if 0
	// funzione di test di scrittura icone a video
	void LCD_writeAnIconTest(){
	}
#endif

// reset del piedino INT dell'lcd
void vResetIntLCD_spi(void){
unsigned char xdata dummy;
	 LCD_SpiAccess(0x6F,&dummy);//fa salire segnale di interrupt
}	 



// libreria bertronic...............


//***Selezionare solo uno dei tre tipi di bus
//#define bus_8bit
#define bus_SPI
//#define bus_RS232

//***Selezionare una delle due risoluzioni
//#define VGA;
#define QVGA 



//carattere di acknowledge
unsigned char ACK_CHAR= 0xAA;
unsigned char xdata dummyLcd;



// spedizione di un byte via spi ad lcd
// ingresso:
//    dato da spedire ad lcd fff
//    puntatore a dato ricevuto da lcd
void LCD_SpiAccess(unsigned char data_to_be_sent, unsigned char xdata * pdata_read){
volatile unsigned char status;
		// trasmetto il byte
		spi_lcd_register=data_to_be_sent;
		// aspetto che tx/rx sia finita
		while(1){
			status=spi_lcd_busy;
			if ((status&0x01)==0)
				break;				
		}
		// leggo il byte ricevuto		
		*pdata_read=spi_lcd_register;
}

void TFT_write(unsigned char data_to_be_sent){
	LCD_SpiAccess(data_to_be_sent,&dummyLcd);
}

void TFT_read(unsigned char xdata * data_read)	{
	LCD_SpiAccess(0,data_read);
}

unsigned long ul_count_timeout_lcd_int_active;
void v_signal_timeout_wating_lcd_int(void){
    ul_count_timeout_lcd_int_active++;
}

// attesa che fpga abbia finito; nella demo board di ST si va a testare pin INT
// che � collegato a piedino INT di lcd
void LCD_wait_FPGA_done(void){
        #define def_max_wait_lcd_int_active_ms 1000
        #define def_max_wait_lcd_int_active_us (def_max_wait_lcd_int_active_ms*1000UL)
        #define def_pause_between_test_lcd_int_active_us (500UL)
        #define def_num_loop_test_lcd_int_active (def_max_wait_lcd_int_active_us/def_pause_between_test_lcd_int_active_us)

        #define def_max_wait_AA_from_lcd_ms 200
        #define def_max_wait_AA_from_lcd_us (def_max_wait_AA_from_lcd_ms*1000UL)
        #define def_pause_between_test_AA_from_lcd_us (500UL)
        #define def_num_loop_test_AA_from_lcd (def_max_wait_AA_from_lcd_us/def_pause_between_test_AA_from_lcd_us)

		unsigned char xdata dummy;
		register unsigned long uiNumLoop;
		dummy=0;
		uiNumLoop=0;
		while (!defLcdINTactive){
            // wait some us
            v_wait_microseconds(def_pause_between_test_lcd_int_active_us);
#pragma message "***sistemare numero di loop in lcd_wait_fpga_done ***"
			if (++uiNumLoop>=def_num_loop_test_lcd_int_active){
				// ATTIVO reset spi lcd
				//defResetSpiLcd;
				// DISATTIVO reset spi lcd
				//defEndResetSpiLcd;
                v_signal_timeout_wating_lcd_int();
				break;
			}				
		}//attesa discesa segnale dalla FPGA
		uiNumLoop=0;
		while ((dummy!=0xaa)&&((dummy!=0x55))) {
            // wait some us
            v_wait_microseconds(def_pause_between_test_AA_from_lcd_us);
			// Max 1000 loop per evitare stallo spi...
			if (++uiNumLoop>=def_num_loop_test_AA_from_lcd){
				// ATTIVO reset spi lcd
				//defResetSpiLcd;
				// DISATTIVO reset spi lcd
				//defEndResetSpiLcd;
                v_signal_timeout_wating_lcd_int();
				break;
			}				
			LCD_SpiAccess(0x00,&dummy);//attesa AA
		}			
		LCD_SpiAccess(0x6F,&dummy);//fa salire segnale di interrupt
}

void FPGA_done(void){
	LCD_wait_FPGA_done();
}

	
	
void Beep(void)
	{
	TFT_write(0x68);
	}	

// esegue beep di lcd
void LCD_Beep(void){
	Beep();
}	
	
	
#if 0
	// esegue un test del beep dell'lcd
	void vTest_LCD_Beep(void){
		register unsigned char i;
		for (i=0;i<1;i++)
			Beep();
	}
#endif

#if 0
	void DMA(unsigned int X_start,unsigned int X_stop,unsigned int Y_start,unsigned int Y_stop,unsigned char mod)
		{
		unsigned char dummy;
		unsigned int i;
		
		TFT_write(0x80);
		TFT_write((X_start>>2)&0xFF);
		TFT_write(((X_start&0x03)<<6)|(X_stop>>4));
		TFT_write(((X_stop&0x0F)<<4)|(Y_start>>5));
		TFT_write(((Y_start&0x1F)<<3)|(Y_stop>>6));
		TFT_write(((Y_stop&0x3F)<<2)|(mod&0x03));
		for (i=0;i<(X_stop-X_start+1)*(Y_stop-Y_start+1);i++){
			TFT_write(i);
			if(mod==0x00) TFT_write(i);
		}
		FPGA_done();
		}
#endif
	
#if 0
	void LCD_contrast(unsigned char contrasto){
		TFT_write(0x69);
		TFT_write(contrasto);
	}
#endif
	
	
void Change_bank(unsigned char write_bank,unsigned char read_bank)
	{
	TFT_write(0x66);
	TFT_write((write_bank<<4)|read_bank);
	}


// scambia banchi di visualizzazione e background per visualizzare
// l'immagine precedentemente caricata in ram
void LCD_toggle_bank(void){
register unsigned char ucAux;
	ucAux=structLCD.ucVisualizeBank;
	structLCD.ucVisualizeBank=structLCD.ucBackgroundBank;
	structLCD.ucBackgroundBank=ucAux;
	Change_bank(structLCD.ucVisualizeBank,structLCD.ucBackgroundBank);
}	

	
void Read_image_from_flash(unsigned char image_number)
	{
		TFT_write(0x67);		
		#ifdef QVGA
			switch (image_number)
			{
				case 1:	TFT_write(0x04);//address di lettura
				break;
				case 2:	TFT_write(0x07);//address di lettura
				break;
				case 3:	TFT_write(0x0A);//address di lettura
				break;
				case 4:	TFT_write(0x0D);//address di lettura
				break;
				case 5:	TFT_write(0x10);//address di lettura
				break;
				case 6:	TFT_write(0x13);//address di lettura
				break;	
				case 7:	TFT_write(0x16);//address di lettura
				break;
				case 8:	TFT_write(0x19);//address di lettura
				break;
				case 9:	TFT_write(0x1C);//address di lettura
				break;
				case 10:TFT_write(0x1F);//address di lettura
				break;
				case 11:TFT_write(0x22);//address di lettura
				break;
				case 12:TFT_write(0x25);//address di lettura
				break;
				case 13:TFT_write(0x28);//address di lettura
				break;	
				case 14:TFT_write(0x2B);//address di lettura
				break;
				case 15:TFT_write(0x2E);//address di lettura
				break;
				case 16:TFT_write(0x31);//address di lettura
				break;
				case 17:TFT_write(0x34);//address di lettura
				break;
				case 18:TFT_write(0x37);//address di lettura
				break;
			}	
		#endif
        ;
		#ifdef VGA
			switch (image_number)
			{
				case 1:	TFT_write(0x04);//address di lettura
				break;
				case 2:	TFT_write(0x0E);//address di lettura
				break;
				case 3:	TFT_write(0x18);//address di lettura
				break;
				case 4:	TFT_write(0x22);//address di lettura
				break;
				case 5:	TFT_write(0x2C);//address di lettura
				break;
				case 6:	TFT_write(0x36);//address di lettura
				break;	
				case 7:	TFT_write(0x40);//address di lettura
				break;
				case 8:	TFT_write(0x4A);//address di lettura
				break;
				case 9:	TFT_write(0x54);//address di lettura
				break;
				case 10:TFT_write(0x5E);//address di lettura
				break;
				case 11:TFT_write(0x68);//address di lettura
				break;
				case 12:TFT_write(0x72);//address di lettura
				break;
				case 13:TFT_write(0x80);//address di lettura
				break;
				case 14:TFT_write(0x8A);//address di lettura
				break;
				case 15:TFT_write(0x94);//address di lettura
				break;
				case 16:TFT_write(0x9E);//address di lettura
				break;
				case 17:TFT_write(0xA8);//address di lettura
				break;
				case 18:TFT_write(0xB2);//address di lettura
				break;
				case 19:TFT_write(0xBC);//address di lettura
				break;
				case 20:TFT_write(0xC6);//address di lettura
				break;
				case 21:TFT_write(0xD0);//address di lettura
				break;
				case 22:TFT_write(0xDA);//address di lettura	
				break;
				case 23:TFT_write(0xE4);//address di lettura
				break;
				case 24:TFT_write(0xEE);//address di lettura
				break;														
			}	
		#endif
        ;		
		FPGA_done();	
	}
// carica in ram di background immagine numero ucImageNumber 0..17
void LCD_LoadImageOnBackgroundBank(unsigned char ucImageNumber){
	Read_image_from_flash(ucImageNumber);
}

#if 0
	// visualizzazione immagine a display
	void LCD_VisualizeImage(unsigned char ucImageNumber){
		LCD_LoadImageOnBackgroundBank(ucImageNumber);
		LCD_toggle_bank();
	}
#endif

#if 0
	void Read_touch_value(unsigned int *x,unsigned int *y, unsigned char mod)
		{
		unsigned char xdata temp_x,temp_y;
		#ifdef bus_8bit	
			TFT_write(0x60);
			if (mod==0){
			TFT_read(&temp_y);
			*y=(int)temp_y;
			TFT_read(&temp_x);
			*x=(int)temp_x;
			} else {
			TFT_read(&temp_y);
			*y=(int)temp_y<<8;
			TFT_read(&temp_y);
			*y=*y&(int)temp_y;
			*y=*y>>4;
			TFT_read(&temp_x);
			*x=(int)temp_x<<8;
			TFT_read(&temp_x);
			*x=*x&(int)temp_x;
			*x=*x>>4;			
			}
			TFT_write(0x6F);				
		#endif;
		#ifdef bus_SPI
			TFT_write(0x60);
			if (mod==0){
				TFT_read(&temp_y);
				*y=(int)temp_y;
				TFT_read(&temp_x);
				*x=(int)temp_x;
			} 
			else {
				TFT_read(&temp_y);
				*y=(int)temp_y;
				*y=*y<<8;
				TFT_read(&temp_y);
				*y=*y|(int)temp_y;
				*y=*y>>4;
				TFT_read(&temp_x);
				*x=(int)temp_x;
				*x=*x<<8;
				TFT_read(&temp_x);
				*x=*x|(int)temp_x;
				*x=*x>>4;

			}
			TFT_write(0x6F);
		#endif
		//Per la RS232, i dati vengono inviati direttamente, senza bisogno del comando di lettura
	}	
#endif	
	




void vWriteBMPelement(TipoStructParams_Bitmap_Lista *pb){

	*(pListLcd->puclista++)=(((pb->ucRipX<<2)&0x3C)		|	((pb->ucRipY>>2)&0x03));
	*(pListLcd->puclista++)=(((pb->ucRipY<<6)&0xC0)		|	((pb->uiAddress>>5)&0x3F));
	*(pListLcd->puclista++)=(((pb->uiAddress<<3)&0xF8)	|	((pb->uiXpixel>>7)&0x07));
	*(pListLcd->puclista++)=(((pb->uiXpixel<<1)&0xFE)	|	((pb->uiYpixel>>8)&0x01));
	*(pListLcd->puclista++)=(pb->uiYpixel)&0xFF;
	pListLcd->uiLength+=5;
	pListLcd->lastTipo=enumElementoBitmap;

}

#include "MappaCaratteri.c"

void vWriteIconElement(TipoStructParams_Icon_Lista xdata *pp){
register unsigned char ucChar2send;
unsigned char xdata * data puc;
	puc=pp->pucVettore;
	ucChar2send=ucMapDisplayChar(*puc);
	if (!ucChar2send)
		return;
	ucChar2send-=0x20;

	*(pListLcd->puclista++)=(0x40|((pp->ucFont&0x07)<<3)|((ucChar2send>>5)&0x07));
	*(pListLcd->puclista++)=(((ucChar2send<<3)&0xF8)|((pp->uiXpixel>>7)&0x07));
	*(pListLcd->puclista++)=(((pp->uiXpixel<<1)&0xFE)|((pp->uiYpixel>>8)&0x01));
	*(pListLcd->puclista++)=(pp->uiYpixel)&0xFF;
	if (!pp->ucTransparent) 
		*(pListLcd->puclista++)=pp->ucBackgroundColor;
	else 
		*(pListLcd->puclista++)=pp->ucIconColor;
	*(pListLcd->puclista++)=pp->ucIconColor;
	pListLcd->uiLength+=6;
	if (puc[1])
		pListLcd->lastTipo=enumElementoNextIcona;
	else
		pListLcd->lastTipo=enumElementoIcona;
	while(*++puc){
		ucChar2send=ucMapDisplayChar(*puc)-0x20;
		*(pListLcd->puclista++)=(0x78|((ucChar2send>>5)&0x07));
		*(pListLcd->puclista++)=((ucChar2send<<3)&0xF8);
		pListLcd->uiLength+=2;
	}
}



// scrive un testo, per� max N caratteri
void vWriteIconElement_N(TipoStructParams_Icon_Lista xdata *pp, unsigned char ucN){
register unsigned char ucChar2send;
unsigned char xdata * data puc;
	puc=pp->pucVettore;
	ucChar2send=ucMapDisplayChar(*puc);
	if (!ucChar2send||!ucN)
		return;
	ucChar2send-=0x20;

	*(pListLcd->puclista++)=(0x40|((pp->ucFont&0x07)<<3)|((ucChar2send>>5)&0x07));
	*(pListLcd->puclista++)=(((ucChar2send<<3)&0xF8)|((pp->uiXpixel>>7)&0x07));
	*(pListLcd->puclista++)=(((pp->uiXpixel<<1)&0xFE)|((pp->uiYpixel>>8)&0x01));
	*(pListLcd->puclista++)=(pp->uiYpixel)&0xFF;
	if (!pp->ucTransparent) 
		*(pListLcd->puclista++)=pp->ucBackgroundColor;
	else 
		*(pListLcd->puclista++)=pp->ucIconColor;
	*(pListLcd->puclista++)=pp->ucIconColor;
	pListLcd->uiLength+=6;
	if (puc[1]&&(ucN>1))
		pListLcd->lastTipo=enumElementoNextIcona;
	else
		pListLcd->lastTipo=enumElementoIcona;
	while(*++puc&&--ucN){
		ucChar2send=ucMapDisplayChar(*puc)-0x20;
		*(pListLcd->puclista++)=(0x78|((ucChar2send>>5)&0x07));
		*(pListLcd->puclista++)=((ucChar2send<<3)&0xF8);
		pListLcd->uiLength+=2;
	}
}



void vInitListDisplayElements(void){
	pListLcd=&LCD_Lista;
	pListLcd->uiLength=0;
	pListLcd->puclista=pListLcd->ucLCD_Lista;
	pParamsIcon=&Params_Icon_Lista;
	pBitmapLista=&Bitmap_Lista;
    pListLcd->ui_valid=1;
}

unsigned char ucTroppoPienaListaLcd(void){
	if (LCD_Lista.uiLength+defLCD_MarginCharListaD>=sizeof(pListLcd->ucLCD_Lista)){
		return 1;
	}
	return 0;
}

// reset mappa icone e bitmap
void vResetIconsAndBitmap(void){
	vInitListDisplayElements();
}	


void vCloseListDisplayElements(void){
    switch(pListLcd->lastTipo){
        case enumElementoIcona:
            if (pListLcd->puclista-pListLcd->ucLCD_Lista>=6){
                pListLcd->puclista[-6]|=0x80;
            }
            else{
                pListLcd->ui_valid=0;
            }
            break;
        case enumElementoNextIcona:
            if (pListLcd->puclista-pListLcd->ucLCD_Lista>=2){
                pListLcd->puclista[-2]|=0x80;
            }
            else{
                pListLcd->ui_valid=0;
            }
            break;
        case enumElementoBitmap:
            if (pListLcd->puclista-pListLcd->ucLCD_Lista>=5){
                pListLcd->puclista[-5]|=0x80;
            }
            else{
                pListLcd->ui_valid=0;
            }
            break;
        default:
            pListLcd->ui_valid=0;
            break;
    }
}


void invio_lista(void){
	int i;
	register unsigned char *puc;
    if (LCD_Lista.ui_valid){
        TFT_write(0x84);//scrittura lista
        i=LCD_Lista.uiLength;
        puc=&LCD_Lista.ucLCD_Lista[0];
        while(i--)
            TFT_write(*puc++);		
        // forzo sempre una rilettura del touch...
        ucRememberLcdIntActive=1;
        FPGA_done();
    }
}

#if 0
	void vSelect12bitTouch(void){
		TFT_write(0x86);
	}
#endif

void vSelect8bitTouch(void){
	TFT_write(0x87);
}


void applicazione_lista(void)
{
	TFT_write(0x85);
	FPGA_done();
}

void lettura_versione_FW(unsigned char xdata *FW1,unsigned char xdata *FW2)
{
	TFT_write(0x6C);
	TFT_read(FW1);//prima lettura
	TFT_read(FW2);//seconda lettura
	TFT_write(0x6F);//fa salire segnale di interrupt
}


