// Gui.c : interfaccia grafica verso display
//
// Area del touch screen
//  X=240, Y=240						X=0,Y=240
//
//
//
//
//
//
//
//
//
//
//  X=240, Y=0							X=0,Y=0
//


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "fpga_nxp.h"
#include "guideclar.h"
#include "gui.h"
#include "lcd.h"
#include "vdip.h"
#include "spicost.h"
#include "spityp.h"
#include "spiext.h"
#include "spiwin.h"
extern code unsigned char ucFirmwareVersion[];
extern void SD_PutChar( unsigned char bByte );
extern void SD_Init( void );
void vLinearizzaPosLcd(void);
// salva i coefficienti di calibrazione lcd in nand flash
void vSaveCoeffsLcdCalibration(void);
// carica i coefficienti di calibrazione di default
void vLoadCoeffsLcdCalibration(void);



// per stampare a video coordinate di touch
//#define defStampaTouchCoords


// dimensione dei font in pixel (prima la larghezza X, poi la ampiezza Y)
xdata unsigned char ucFontSizePixelXY[enumNumOfFonts][2]={
// larghezza/altezza font small enumFont_8x8
	{defWidthPixelFont_8x8,defHeightPixelFont_8x8},

// larghezza/altezza font medio enumFont_16x16
	{defWidthPixelFont_16x16,defHeightPixelFont_16x16 },

// larghezza/altezza font medio grande enumFont_24x24
	{defWidthPixelFont_24x24,defHeightPixelFont_24x24},

// larghezza/altezza font big enumFont_32x32
	{defWidthPixelFont_32x32,defHeightPixelFont_32x32},

// larghezza/altezza font very big enumFont_40x40
	{defWidthPixelFont_40x40,defHeightPixelFont_40x40}
    };


  



// primo elemento della coda del testo a display
xdata TipoStructTextOnDisplayQueue headTextOnDisplayQueue;
// definizione del cursore
TipoCursoreOnDisplay xdata LCD_Cursore;    
// impostazioni di default del testo
xdata TipoDefaultTextSettings defaultTextSettings;
// struttura con alcune info sulle immagini visualizzate a display lcd
xdata TipoStructInfoImmagine infoImmagine;
// num max di entries nella tabella degli id lcd
#define defMaxLcdId 32
typedef struct _TipoStructLcdIdTable{
	unsigned char ucId;
	TipoTestoOnDisplay xdata * pTesto;
}TipoStructLcdIdTable;
// tabella degli id dei testi lcd, usati per accedere rapidamente
// ai testi
TipoStructLcdIdTable tableLcdId[defMaxLcdId];

// stringa in xdata con 1 solo carattere dentro
unsigned char xdata uc1charString[2];
// stringa in xdata con 2 soli caratteri dentro
unsigned char xdata uc2charString[3];


// indica se calibrazione lcd in corso
xdata unsigned char ucCalibraLcdInCorso;
void vInitKbd(void);

typedef struct _TipoKbdInterface{
	unsigned char ucAkeyPressed,ucKeyPressed;
}TipoKbdInterface;

xdata TipoKbdInterface kbdInterface;

#if 0
	void vSignalNewKeyPressed(unsigned char ucKey){
		kbdInterface.ucAkeyPressed=1;
		kbdInterface.ucKeyPressed=ucKey;
	}
#endif
	
unsigned char ucNewKeyPressed(void){
	return kbdInterface.ucAkeyPressed;
}

unsigned char ucGetKeyPressed(void){
	kbdInterface.ucAkeyPressed=0;
	return kbdInterface.ucKeyPressed;
}



unsigned int uiPixelDistance(unsigned int p1, unsigned int p2){
	if (p1>=p2)
		return p1-p2;
	return p2-p1;	
}



// **********************************************************************************
// **********************************************************************************
//
// definizione di variabili per test di lettura/scrittura
//
// **********************************************************************************
// **********************************************************************************
//
// variabile per test touch screen
TipoTestoOnDisplay xdata testoTouch;
TipoTestoOnDisplay xdata testoPrintDefault;
// testo per eseguire verifiche display touch screren
TipoTestoOnDisplay xdata testoCallback;
// stringa per eseguire verifiche display touch screen
TipoDisplayChar xdata stringaTestTouch[]="0123456789 - *";
TipoDisplayChar xdata stringaTestoScrivi[defMaxCharTestoScrivi];
TipoTestoOnDisplay xdata testoScrivi;
xdata unsigned char idxStringaTestoScrivi;


// **********************************************************************************
// **********************************************************************************
//
// inizio codice...
//
// **********************************************************************************
// **********************************************************************************

// inizializzazione della coda che contiene la lista di tutti i testi che appaiono a display
void vInitTextOnDisplayQueue(void){
	headTextOnDisplayQueue.pNext=NULL;
	headTextOnDisplayQueue.pPrev=NULL;
	headTextOnDisplayQueue.pTesto=NULL;
}


// inizializzazione della tabella degli identificatori die testi che appaiono a display
void vInitLcdTable(void){
	unsigned char i;
	for (i=0;i<defMaxLcdId;i++){
		tableLcdId[i].ucId=0;
		tableLcdId[i].pTesto=NULL;
	}
}

// funzione che restituisce un nuovo lcd id, se disponibile
// 0: id non disponibile
// >0 id desiderato
unsigned char ucNextLcdId(TipoTestoOnDisplay xdata * pTesto){
	unsigned char i;
	for (i=1;i<defMaxLcdId;i++){
		if (!tableLcdId[i].ucId){
			tableLcdId[i].ucId=i;
			tableLcdId[i].pTesto=pTesto;
			return i;
		}
	}
	return 0;
}

// funzione che libera l'id lcd specificato
// 0: id non trovato
// 1: id trovato e liberato
unsigned char ucFreeLcdId(unsigned char ucId){
	unsigned char i;
	for (i=0;i<defMaxLcdId;i++){
		if (tableLcdId[i].ucId==ucId){
			tableLcdId[i].ucId=0;
			tableLcdId[i].pTesto=NULL;
			return 1;
		}
	}
	return 0;
}

#if 0
	// trova il testo che ha l'id indicato e restituisce puntatore alla struttura testo
	TipoTestoOnDisplay xdata * pFindId(unsigned char ucId){
		unsigned char i;
		for (i=0;i<defMaxLcdId;i++){
			if (tableLcdId[i].ucId==ucId){
				return tableLcdId[i].pTesto;
			}
		}
		return NULL;
	}
#endif

// funzione che cancella un elemento nella coda del testo a display
// Ingresso:
// - puntatore all'elemento da cancellare
// Uscita:
// 1 se tutto ok
// 0 se errore ( richiesto free della testa, errore in free...)
unsigned char ucDeleteTextOnDisplayQueue(TipoStructTextOnDisplayQueue xdata *p){
	// non posso cancellare la testa
	if (p==&headTextOnDisplayQueue)
		return 0;
	ucFreeLcdId(p->pTesto->ucId);
	// se esiste elemento successivo, gli assegno il puntatore prev
	if(p->pNext)
		p->pNext->pPrev=p->pPrev;
	// esiste sicuramente elemento precedente, gli assegno il puntatore next
	//  (infatti la coda ha una testa che nn puo' essere cancellata)
	p->pPrev->pNext=p->pNext;
	// libero la stringa di testo
	if (p->pTesto->pucDisplayString)
		free(p->pTesto->pucDisplayString);
	// libero la stringa di picture
	if (p->pTesto->pucPictureString)
		free(p->pTesto->pucPictureString);
	// libero la struttura testo
	if (p->pTesto)
		free(p->pTesto);
	// libero l'elemento della coda
	free(p);
	return 1;
}



// reset della struttura del testo a display
// ritorna 1 se tutto ok,
// ritorna 0 se errore nelle free
unsigned char ucResetTextOnDisplayQueue(void){
	while(1){
		// se nessun elemento in coda, esco
		if (!headTextOnDisplayQueue.pNext)
			break;
		if (!ucDeleteTextOnDisplayQueue(headTextOnDisplayQueue.pNext))
			return 0;
	}
	headTextOnDisplayQueue.pPrev=NULL;
	headTextOnDisplayQueue.pTesto=NULL;
	return 1;
}

// cancellazione di un testo a video...
// 1 se trovato e cancellato, 0 se non trovato o non cancellato
#if 0
	unsigned char ucDeleteTesto(TipoTestoOnDisplay xdata * pTesto){
	TipoStructTextOnDisplayQueue xdata * xdata p;
		// se ptr null, esco per evitare disastri...
		if (!pTesto||!pTesto->pTestoOnQueue)
			return 0;
		// chiamo la routine che si incarica di gestire la distruzione del testo		
		//if (0&&pTesto->pFunCallBackTouch)
		//	pTesto->pFunCallBackTouch(eventCallback_Destroyed,pTesto);
			
		// punto al primo elemento della coda=testa
		p=&headTextOnDisplayQueue;
		while(1){
			// se finita la coda, esco dal loop
			if (!p->pNext)
				break;
			//	passo al prossimo elemento
			p=p->pNext;
			// se testo coincide con quello da cancellare, chiamo la routine di cancellazione
			if (pTesto->pTestoOnQueue==p->pTesto){
				if (!ucDeleteTextOnDisplayQueue(p))
					return 0;
				return 1;	
			}				
		}
		// elemento non trovato, esco
		return 0;
	}
#endif

// solo per debug...
void myexit(unsigned int uiExitCode){
	switch(uiExitCode){
		case 1: // errore in malloc, memoria heap esaurita!
			break;
		default:
			break;
	}
	while(1){
	}
}


// routine che inserisce in coda display un elemento
// ingresso: struttura che deve essere ricopiata in coda
//           vengono ricopiate le stringhe di picture e di testo
// uscita: 0 se errore
//         1 se tutto ok
unsigned char ucInsertTextOnDisplayQueue(TipoTestoOnDisplay xdata * pTesto){
	TipoStructTextOnDisplayQueue xdata* p;
	unsigned char xdata ucSize;
	// alloco un nuovo elemento della coda
	p=(TipoStructTextOnDisplayQueue xdata*)malloc(sizeof(TipoStructTextOnDisplayQueue));
	if (!p){
		myexit(1);
		return 0;
	}	
	// inserisco l'elemento giusto dopo la testa
	p->pNext=headTextOnDisplayQueue.pNext;
	if (p->pNext)
		p->pNext->pPrev=p;
	headTextOnDisplayQueue.pNext=p;
	p->pPrev=&headTextOnDisplayQueue;
	// alloco il testo
	p->pTesto=(TipoTestoOnDisplay xdata *)malloc(sizeof(TipoTestoOnDisplay));
	if (!p->pTesto){
		myexit(1);
		return 0;
	}		
	// salvo ptr a struttura testo salvata in coda
	pTesto->pTestoOnQueue=p->pTesto;
	// ricopio la struttura passata
	memcpy(p->pTesto,pTesto,sizeof(TipoTestoOnDisplay));
	// copio la stringa col testo
	if (pTesto->pucDisplayString){
		ucSize=strlen((const char xdata *)pTesto->pucDisplayString)+1;
		p->pTesto->pucDisplayString=(TipoDisplayChar xdata *)malloc(ucSize);
		if (p->pTesto->pucDisplayString==NULL){
			myexit(1);
			return 0;
		}	
		strcpy((char *)p->pTesto->pucDisplayString,(const char *)pTesto->pucDisplayString);
	}
	// copio la picture
	if (pTesto->pucPictureString){
		if (!(p->pTesto->pucPictureString=
			   (TipoPictureChar xdata *)malloc(strlen((const char *)pTesto->pucPictureString)+1))
			){
			myexit(1);
			return 0;
		}	
		strcpy((char *)p->pTesto->pucPictureString,(const char *)pTesto->pucPictureString);
	}
	// salvo l'id
	p->pTesto->ucId=pTesto->ucId=ucNextLcdId(p->pTesto);
	return 1;
}



#if 0
	// funzione che esamina il testo di un elemento in coda display rimappando
	// i caratteri secondo questo schema 
	// caratteri non stampabili: ascii 0..31-->1 (carattere spazio)
	// caratteri normali: ascii 32..127-->1..96
	//  0..31 -->32-->32-32+1=1  i caratteri di controllo non sono stampabili
	//  32..127 -->32..127-->32-32+1..127-32+1-->1..96 i caratteri normali sono tutti ok
	//  127..191 -->32-->32-32+1-->1
	//  192..192+31=223 -->192-95+1..223-95+1-->97..128  caratteri accentati maiuscoli-->accentati minuscoli
	//  224..255 -->97..128  caratteri accentati maiuscoli ok
	void vRemapTextOnQueue(TipoTestoOnDisplay xdata *pTesto){
			register unsigned char ucTheChar;
			TipoDisplayChar xdata *pc;
			if (!pTesto->pucDisplayString)
				return;
			pc=pTesto->pucDisplayString;
			// loop di rimpiazzo caratteri
			while((ucTheChar=*pc)!=0){
				// caratteri "normali" e accentati minusc ok
				// metto prima questo test, perch� sar� quello pi� volte verificato...
				if (  (ucTheChar>=32)&&(ucTheChar<=127)	)
					*pc-=31;
				else if (ucTheChar>=224)
					*pc-=127;
				// accentati maiusc-->accentati minusc
				else if((ucTheChar>=192)&&(ucTheChar<=223)){
					*pc-=95; // passo a corrispondente accentato minuscolo
				}
				else
					*pc=defaultSubstitutionChar;	// altrimenti sostituisco con <spazio>
				pc++;
			}//while((ucTheChar=*pc)!=0)
	}//void vRemapTextOnQueue(TipoTestoOnDisplay *pTesto)
#else
	void vRemapTextOnQueue(void){
	}
#endif


// calcolo area occupata nel touch screen da una entit�
// Attenzione!
// Riga e colonna sono sempre riferiti ad 8x8
void vCalcAreaTouchScreenTesto(TipoTestoOnDisplay xdata *pTesto){
xdata int i;
	if (pTesto->colonna>defLcdWidthX_pixel)
		pTesto->structTouchArea.x1Pixel=0;
	else
		pTesto->structTouchArea.x1Pixel=defLcdWidthX_pixel-pTesto->colonna;
	
	i=strlen(pTesto->pucDisplayString);
	if (pTesto->structTouchArea.x1Pixel<i*ucFontSizePixelXY[pTesto->ucTipoFont][0])
		pTesto->structTouchArea.x2Pixel=0;
	else
		pTesto->structTouchArea.x2Pixel=pTesto->structTouchArea.x1Pixel-i*ucFontSizePixelXY[pTesto->ucTipoFont][0];

	if (pTesto->riga>defLcdWidthY_pixel)
		pTesto->structTouchArea.y1Pixel=0;
	else
		pTesto->structTouchArea.y1Pixel=defLcdWidthY_pixel-pTesto->riga;

	if (pTesto->structTouchArea.y1Pixel<ucFontSizePixelXY[pTesto->ucTipoFont][1])
		pTesto->structTouchArea.y2Pixel=0;
	else
		pTesto->structTouchArea.y2Pixel=pTesto->structTouchArea.y1Pixel-ucFontSizePixelXY[pTesto->ucTipoFont][1];

	if (pTesto->button.ucType){
		if (pTesto->structTouchArea.x1Pixel<defLcdWidthX_pixel-4)
			pTesto->structTouchArea.x1Pixel+=4;
		else
			pTesto->structTouchArea.x1Pixel=defLcdWidthX_pixel-1;
		if (pTesto->structTouchArea.x2Pixel>=4)
			pTesto->structTouchArea.x2Pixel-=4;
		else
			pTesto->structTouchArea.x2Pixel=0;
		if (pTesto->structTouchArea.y1Pixel<defLcdWidthY_pixel-4)
			pTesto->structTouchArea.y1Pixel+=4;
		else
			pTesto->structTouchArea.y1Pixel=defLcdWidthX_pixel-1;
		if (pTesto->structTouchArea.y2Pixel>=4)
			pTesto->structTouchArea.y2Pixel-=4;
		else
			pTesto->structTouchArea.y2Pixel=0;
	}
}

// routine che verifica se � stato selezionato un tasto touch screen valido
// confrontando la posizione in pixel con quella occupata dal testo...
// inoltre il touch deve essere abilitato...
unsigned char ucVerifyTouchOnTesto(TipoTestoOnDisplay xdata *pTesto,unsigned int x, unsigned int y){
	int iAux;
	if (   (pTesto->structTouchArea.ucEnable)
	     &&(x<=pTesto->structTouchArea.x1Pixel)
		 &&(x> pTesto->structTouchArea.x2Pixel)
		 &&(y<=pTesto->structTouchArea.y1Pixel)
		 &&(y> pTesto->structTouchArea.y2Pixel)		 
		){
		// trovo quale carattere del testo � stato selezionato
		iAux=pTesto->structTouchArea.x1Pixel;//-ucFontOffset[pTesto->ucTipoFont].ucOffsetX;
		iAux-=x;
		pTesto->ucCharOnTextSelected=iAux/ucFontSizePixelXY[pTesto->ucTipoFont][0];
		return 1;
	}	
	return 0;		 
}


// routine che posiziona a video un testo
// ritorna 0 se errore
//  altrimenti ritorna l'id del testo
unsigned char ucTextOnDisplay(TipoTestoOnDisplay xdata *pTesto){
	// verifica numero del font
	if (pTesto->ucTipoFont>=enumNumOfFonts)
		return 0;
	// verifica numero di riga/colonna
	if (pTesto->riga>=defLcdWidthY_pixel)
		return 0;
	if (pTesto->colonna>=defLcdWidthX_pixel)
		return 0;
	// colore di sfondo originale del testo
 	pTesto->structProperties.ucBackGroundColorSave=pTesto->structProperties.ucBackGroundColor;
   // colore di primo piano originale del testo
 	pTesto->structProperties.ucForeGroundColorSave=pTesto->structProperties.ucForeGroundColor;
	// impostazione area di touch
	vCalcAreaTouchScreenTesto(pTesto);
	
	// inserisco il testo in coda
	if (!ucInsertTextOnDisplayQueue(pTesto))
		return 0;			
	// chiamo la routine che si incarica di gestire la creazione del testo		
	//if (pTesto->pFunCallBackTouch)
	//	pTesto->pFunCallBackTouch(eventCallback_Created,pTesto);
        			
		
	// effettuo la validazione del testo inserito in coda
	// che � in ram e puo' essere modificato
	vRemapTextOnQueue();
	return pTesto->ucId;
}


// routine che posiziona a video un testo
// ritorna 0 se errore
//  altrimenti ritorna l'id del testo
unsigned char ucBitmap40x25OnDisplay(TipoTestoOnDisplay xdata *pTesto){
	// verifica numero del font
	if (pTesto->ucTipoFont>=enumNumOfFonts)
		return 0;
	// verifica numero di riga/colonna
	if (pTesto->riga>=defLcdWidthY_pixel)
		return 0;
	if (pTesto->colonna>=defLcdWidthX_pixel)
		return 0;
	// colore di sfondo originale del testo
 	pTesto->structProperties.ucBackGroundColorSave=pTesto->structProperties.ucBackGroundColor;
   // colore di primo piano originale del testo
 	pTesto->structProperties.ucForeGroundColorSave=pTesto->structProperties.ucForeGroundColor;
	// impostazione area di touch: 40x24 a partire dalle coordinate correnti
	if (pTesto->colonna>defLcdWidthX_pixel)
		pTesto->structTouchArea.x1Pixel=0;
	else
		pTesto->structTouchArea.x1Pixel=defLcdWidthX_pixel-pTesto->colonna;
	
	if (pTesto->structTouchArea.x1Pixel<40)
		pTesto->structTouchArea.x2Pixel=0;
	else
		pTesto->structTouchArea.x2Pixel=pTesto->structTouchArea.x1Pixel-40;

	if (pTesto->riga>defLcdWidthY_pixel)
		pTesto->structTouchArea.y1Pixel=0;
	else
		pTesto->structTouchArea.y1Pixel=defLcdWidthY_pixel-pTesto->riga;

	if (pTesto->structTouchArea.y1Pixel<24)
		pTesto->structTouchArea.y2Pixel=0;
	else
		pTesto->structTouchArea.y2Pixel=pTesto->structTouchArea.y1Pixel-24;

	
	// inserisco il testo in coda
	if (!ucInsertTextOnDisplayQueue(pTesto))
		return 0;			
	// chiamo la routine che si incarica di gestire la creazione del testo		
	//if (pTesto->pFunCallBackTouch)
	//	pTesto->pFunCallBackTouch(eventCallback_Created,pTesto);
        			
		
	// effettuo la validazione del testo inserito in coda
	// che � in ram e puo' essere modificato
	vRemapTextOnQueue();
	return pTesto->ucId;
}



// numero max di char 16x16 che ci stanno in un button
#define defMaxNumPixelPerTestoInButton (20U*16U)
// quante righe di bitmap sono necessarie per definire un button??? 3
#define defNumRowBitmapButton 3
// numero di bitmap per riga
#define defNumBitmapPerRow 64
// numero di pixel di cui si sposta in X il testo di un button premuto
#define defPressedButton_PixelOffsetX 2
// numero di pixel di cui si sposta in Y il testo di un button premuto
#define defPressedButton_PixelOffsetY 2
// numero di bitmap 8x8 lungo X per ogni button
const code unsigned int uiButtonNumBitmapX[10]={5,9,13,17,21,25,29,33,37,40};
// dato che posso tx la bitmap con un numero di ripetizioni pari max a 16, mi preparo un array
// che indica le ripetizioni da spedire per ogni riga bitmap
const code unsigned int uiButtonNumRepetitionsX[3]={
														/*{5,0,0},
														{9,0,0},
														{13,0,0},
														{16,1,0},
														{16,5,0},
														{16,9,0},
														{16,13,0},
														{16,16,1},
														{16,16,5},
														*/
														16,16,8
														};
// indirizzo di inizio dei vari button nella mappa nella memoria flash del display lcd
// � memorizzata come una matrice da 8x8 bitmaps, 64 bitmaps per riga
const code unsigned int uiButtonAddress[10]={0,64*3,64U*3*8U,27,192,192+21,384,384+29,576,768};

// indirizzo di inizio dei vari bitmap button
const code unsigned int uiButtonBitmapAddress[]={0};

// funzione che dato un testo a display lo traduce in mappa icone o bitmap
void vMapTesto(TipoTestoOnDisplay xdata *pTesto){
	TipoDisplayChar xdata * puc;
	xdata unsigned int i;
	xdata unsigned char ucNumPezziButton;
	xdata unsigned int uiLenTestoPixel;
	xdata unsigned char ucButtonShape;
	xdata int iLenTestoPixelResidua;
	// se nessuna stringa da mappare, esco
	if (!pTesto||!pTesto->pucDisplayString)
		return;
	pParamsIcon->uiYpixel=pTesto->riga;
	pParamsIcon->uiXpixel=pTesto->colonna;
	pParamsIcon->ucFont=pTesto->ucTipoFont; // font 8x8
	pParamsIcon->pucVettore=pTesto->pucDisplayString;
	pParamsIcon->ucIconColor=pTesto->structProperties.ucForeGroundColor; 
	pParamsIcon->ucBackgroundColor=pTesto->structProperties.ucBackGroundColor;
	// se bottone e trasparenza attivata...
	// oppure testo con propriet� di trasparenza...
	if (  ((pTesto->button.ucType>enumButton_None)
		    &&defIsButtonTransparent(pTesto->button.ucFlags[0])
		  )
		  ||(pTesto->structProperties.ucProperties&defBitTextProperties_Transparent)
		){
		pParamsIcon->ucTransparent=1;
	}
	else{
		// altrimenti niente effetto trasparenza...
		pParamsIcon->ucTransparent=0;
	}
	// se button di tipo bitmap...
	if (pTesto->button.ucType==enumButton_Bitmap){
		pBitmapLista->uiXpixel=pParamsIcon->uiXpixel;
		pBitmapLista->uiYpixel=pParamsIcon->uiYpixel;
		if (pTesto->button.ucBitmap>=enumBitmap_40x24_thirdrow){
			pBitmapLista->uiAddress=((unsigned int)64)*(18)+((unsigned int )5)*(pTesto->button.ucBitmap-enumBitmap_40x24_thirdrow);
		}
		else if (pTesto->button.ucBitmap>=enumBitmap_40x24_secondrow){
			pBitmapLista->uiAddress=((unsigned int)64)*(12)+((unsigned int )5)*(pTesto->button.ucBitmap-enumBitmap_40x24_secondrow);
		}
		else{
			pBitmapLista->uiAddress=((unsigned int)64)*(6)+((unsigned int )5)*pTesto->button.ucBitmap;
		}
		// bottone premuto???
		if (defIsButtonPressed(pTesto->button.ucFlags[0])){
			pBitmapLista->uiAddress+=64*3;
		}
		// adesso devo ciclare su tutti gli spezzettamenti del button
		pBitmapLista->ucRipY=0; // 2 ripetizioni in Y per avere bitmap 40x24
		// salvo il numero di ripetizioni
		pBitmapLista->ucRipX=4; // 4 ripetizioni in X per avere bitmap 40x24

		for (i=0;i<3;i++){
			// scrivo il button
			vWriteBMPelement(pBitmapLista);
			// passo alla riga successiva del bitmap
			pBitmapLista->uiAddress+=64;
			pBitmapLista->uiYpixel+=8;
		}
	}
	// se devo aggiungere il button attorno al testo...
	// ecco la mappatura delle bitmap dei bottoni
	// vengono memorizzati solo i bottoni per 2,4,6,8,10,12,14,16,18 caratteri
	// c'� un margine di 4 pixel attorno ai caratteri a sx, dx, sopra, sotto
	// il punto in alto a sinistra � l'origine
	// button da  2 caratteri:  5x3 bitmaps=(nchar*2+1)x3bitmaps, indirizzi   0, 64, 128 (40pixel x 24 pixel)
	// button da  4 caratteri:  9x3 bitmaps=(nchar*2+1)x3bitmaps, indirizzi   5, 64+5,  128+5
	// button da  6 caratteri: 13x3 bitmaps=(nchar*2+1)x3bitmaps, indirizzi  14, 64+14, 128+14
	// button da  8 caratteri: 17x3 bitmaps=(nchar*2+1)x3bitmaps, indirizzi  27, 64+27, 128+27
	// passiamo alla riga sotto del bitmap da 512 pixel...
	// button da 10 caratteri: 21x3 bitmaps=(nchar*2+1)x3bitmaps, indirizzi 192, 64+192, 128+192
	// button da 12 caratteri: 25x3 bitmaps=(nchar*2+1)x3bitmaps, indirizzi 192+21, 64+192+21, 128+192+21
	// passiamo alla riga sotto del bitmap da 512 pixel...
	// button da 14 caratteri: 29x3 bitmaps=(nchar*2+1)x3bitmaps, indirizzi 384, 64+384, 128+384
	// button da 16 caratteri: 33x3 bitmaps=(nchar*2+1)x3bitmaps, indirizzi 384+29, 64+384+29, 128+384+29
	// passiamo alla riga sotto del bitmap da 512 pixel...
	// button da 18 caratteri: 37x3 bitmaps=(nchar*2+1)x3bitmaps, indirizzi 576, 64+576, 128+576
	// passiamo alla riga sotto del bitmap da 512 pixel...
	// button da 20 caratteri: 40x3 bitmaps=(nchar*2)x3bitmaps, indirizzi 768, 64+768, 128+768
	else if (pTesto->button.ucType>enumButton_None){
		if (pTesto->button.ucType==enumButton_Static)
			ucButtonShape=1;
		else if (pTesto->button.ucType==enumButton_StaticAlarm)
			ucButtonShape=2;
		else
			ucButtonShape=0;
		// calcolo la lunghezza del bottone che deve contenere il testo
		// punto alla stringa col testo
		puc=pParamsIcon->pucVettore;
		// azzero la lunghezza del testo...
		uiLenTestoPixel=0;
		while(*puc){
			uiLenTestoPixel++;
			// tappo per evitare problemi con stringhe molto lunghe...
			if (uiLenTestoPixel>50){
				break;
			}
			puc++;
		}
		// trovo il numero di pixel occupati dal testo...
		uiLenTestoPixel*=defFontPixelSizeFromFontType(pTesto->ucTipoFont);
		// verifico se � possibile metterci un button attorno al testo
		// elimino il caso 0 pixel ed il caso in cui il testo sia troppo lungo...
		if (uiLenTestoPixel&&(uiLenTestoPixel<=defMaxNumPixelPerTestoInButton)){
			// trovo quale button devo usare: 
			// 1-32 pixel-->0, 33,64-->1  65,96-->2 etc
			//ucButtonType=((uiLenTestoPixel-1)/32);
			if (pTesto->ucTipoFont==enumFontBig){
				// stampo le 5 righe del button
				for (i=0;i<defNumRowBitmapButton+2;i++){
					if (pParamsIcon->uiXpixel>4)
						pBitmapLista->uiXpixel=pParamsIcon->uiXpixel-4;
					else
						pBitmapLista->uiXpixel=0;
					if (pParamsIcon->uiYpixel>4)
						pBitmapLista->uiYpixel=pParamsIcon->uiYpixel-4;
					else
						pBitmapLista->uiYpixel=0;
					pBitmapLista->uiYpixel+=i*8;
					// adesso devo ciclare su tutti gli spezzettamenti del button
					pBitmapLista->ucRipY=0;// sempre 0 ripetizioni in Y
					// indirizzo del button � quello base, pi� numero di righe*numero bitmap per riga
					if (i==0)
						pBitmapLista->uiAddress=uiButtonAddress[ucButtonShape];
					else if (i==4)
						pBitmapLista->uiAddress=uiButtonAddress[ucButtonShape]+defNumBitmapPerRow*2;
					else
						pBitmapLista->uiAddress=uiButtonAddress[ucButtonShape]+defNumBitmapPerRow*1;
					// quanti pixel devo rappresentare prima di mettere il tappo di chiusura del button?
					// devo rappresentare tutto il testo
					iLenTestoPixelResidua=uiLenTestoPixel;

					// ciclo per tutti gli spezzettamenti del button
					for (ucNumPezziButton=0;ucNumPezziButton<3;ucNumPezziButton++){
						// se finito gli spezzettamenti, esco dal loop
						if (!uiButtonNumRepetitionsX[ucNumPezziButton]){
							break;
						}
						// salvo il numero di ripetizioni=numero di bitmap-1
						pBitmapLista->ucRipX=uiButtonNumRepetitionsX[ucNumPezziButton]-1;
						// verifico se sono necessarie tutte le ripetizioni
						if (iLenTestoPixelResidua<((unsigned int)(pBitmapLista->ucRipX+1))*8U){
							if (iLenTestoPixelResidua<8)
								pBitmapLista->ucRipX=0;
							else
								pBitmapLista->ucRipX=iLenTestoPixelResidua/8-1;
						}
						// decremento la lunghezza residua in pixel del testo da rappresentare a video
						iLenTestoPixelResidua-=((int)(pBitmapLista->ucRipX+1))*8;
						// scrivo il button
						vWriteBMPelement(pBitmapLista);

						// mi sposto in X (ripetizioni_x+1) bitmap pi� in l�
						pBitmapLista->uiXpixel+=8*(((unsigned int )pBitmapLista->ucRipX)+1);
						// passo al prossimo bitmap
						pBitmapLista->uiAddress+=pBitmapLista->ucRipX+1;
						if (iLenTestoPixelResidua<=0)
							break;
					}// for degli spezzettamenti
					// se non ho spazio per la chiusura, me lo procuro
					if (pBitmapLista->uiXpixel>=defLcdWidthX_pixel-8){
						pBitmapLista->uiXpixel=defLcdWidthX_pixel-8;
					}
					// chiusura del button: solo 8 pixel
					pBitmapLista->ucRipX=0;
					// in una riga ci sono 320 pixel=40bitmap
					// la chiusura del button la trovo 39 bitmap oltre il primo
					// cio� (defLcdWidthX_pixel/8-1)
					pBitmapLista->uiAddress=uiButtonAddress[ucButtonShape]+(defLcdWidthX_pixel/8-1);
					if (i==0){
					}
					else if (i==4)
						pBitmapLista->uiAddress+=defNumBitmapPerRow*2;
					else
						pBitmapLista->uiAddress+=defNumBitmapPerRow*1;
					// scrivo la chiusura del button
					vWriteBMPelement(pBitmapLista);
				}// for delle righe button
			}
			// font normal
			else{
				// stampo le tre righe del button
				for (i=0;i<defNumRowBitmapButton;i++){
					// se font small, non serve la riga centrale...
					if ((pTesto->ucTipoFont==enumFontSmall)&&(i==1))
						continue;
					// servono 4 pixel di margine a sinistra del testo, ed 8 a destra
					if (pParamsIcon->uiXpixel>4)
						pBitmapLista->uiXpixel=pParamsIcon->uiXpixel-4;
					else
						pBitmapLista->uiXpixel=0;
					// in Y invece bastano 4 pixel di margine sopra
					if (pParamsIcon->uiYpixel>4)
						pBitmapLista->uiYpixel=pParamsIcon->uiYpixel-4;
					else
						pBitmapLista->uiYpixel=0;
					if ((pTesto->ucTipoFont==enumFontSmall)&&(i>1))
						pBitmapLista->uiYpixel+=(i-1)*8;
					else
						pBitmapLista->uiYpixel+=i*8;
					// adesso devo ciclare su tutti gli spezzettamenti del button
					pBitmapLista->ucRipY=0;// sempre 0 ripetizioni in Y
					// indirizzo del button � quello base, pi� numero di righe*numero bitmap per riga
					//pBitmapLista->uiAddress=uiButtonAddress[ucButtonType]+defNumBitmapPerRow*i;

					// per ora utilizzo sempre e solo il button numero 0...
					pBitmapLista->uiAddress=uiButtonAddress[ucButtonShape]+defNumBitmapPerRow*i;
					// quanti pixel devo rappresentare prima di mettere il tappo di chiusura del button?
					// devo rappresentare tutto il testo
					iLenTestoPixelResidua=uiLenTestoPixel;
					// ciclo per tutti gli spezzettamenti del button
					for (ucNumPezziButton=0;ucNumPezziButton<3;ucNumPezziButton++){
						// se finito gli spezzettamenti, esco dal loop
						if (!uiButtonNumRepetitionsX[ucNumPezziButton]){
							break;
						}
						// salvo il numero di ripetizioni=numero di bitmap-1
						pBitmapLista->ucRipX=uiButtonNumRepetitionsX[ucNumPezziButton]-1;
						// verifico se sono necessarie tutte le ripetizioni
						if (iLenTestoPixelResidua<((unsigned int)(pBitmapLista->ucRipX+1))*8U){
							if (iLenTestoPixelResidua<8)
								pBitmapLista->ucRipX=0;
							else
								pBitmapLista->ucRipX=iLenTestoPixelResidua/8-1;
						}
						// decremento la lunghezza residua in pixel del testo da rappresentare a video
						iLenTestoPixelResidua-=((int)(pBitmapLista->ucRipX+1))*8;
						// scrivo il button
						vWriteBMPelement(pBitmapLista);
						// mi sposto in X (ripetizioni_x+1) bitmap pi� in l�
						pBitmapLista->uiXpixel+=8*(((unsigned int )pBitmapLista->ucRipX)+1);
						// passo al prossimo bitmap
						pBitmapLista->uiAddress+=pBitmapLista->ucRipX+1;
						if (iLenTestoPixelResidua<=0)
							break;
					}// for degli spezzettamenti
					// se non ho spazio per la chiusura, me lo procuro
					if (pBitmapLista->uiXpixel>=defLcdWidthX_pixel-8){
						pBitmapLista->uiXpixel=defLcdWidthX_pixel-8;
					}
					// chiusura del button: solo 8 pixel
					pBitmapLista->ucRipX=0;
					// in una riga ci sono 320 pixel=40bitmap
					// la chiusura del button la trovo 39 bitmap oltre il primo
					// cio� (defLcdWidthX_pixel/8-1)
					pBitmapLista->uiAddress=uiButtonAddress[ucButtonShape]+(defLcdWidthX_pixel/8-1)+defNumBitmapPerRow*i;
					// scrivo la chiusura del button
					vWriteBMPelement(pBitmapLista);
				}// for delle righe button
			}
		}// se button � ok
	}// se button

	switch(pTesto->button.ucType){
		// un button di tipo bitmap non ha testo da stampare...
		case enumButton_Bitmap:
			break;
		case enumButton_None:
			//
			// stampo il testo dopo aver stampato il button, altrimenti il button me lo nasconde
			//
			vWriteIconElement(pParamsIcon);
			break;
		case enumButton_Normal:
		case enumButton_Static:
		case enumButton_StaticAlarm:
			// se bottone � premuto, visualizzo il testo spostato a destra basso di 2 pixel come se fosse premuto...
			if (defIsButtonPressed(pTesto->button.ucFlags[0])){
				pParamsIcon->uiYpixel+=defPressedButton_PixelOffsetY;
				pParamsIcon->uiXpixel+=defPressedButton_PixelOffsetX;
			}
			//
			// stampo il testo dopo aver stampato il button, altrimenti il button me lo nasconde
			//
			vWriteIconElement(pParamsIcon);
			break;
		// stampa del testo di un field button, cio� di un button in cui il testo
		// � diviso in campi che possono essere separatamente premuti
		// come ad es codice prodotto -->x.xxx aaaa y.yy b c
		// posso selezionare diametro filo, tipo materiale, diametro mandrino, numero fili, codice fornitore
		case enumButton_Field:
			// percorro tutti i fields del button 
			i=0; // qui metto l'indice corrente del carattere
			for (ucNumPezziButton=0;ucNumPezziButton<pTesto->button.ucNumOfButtonFields;ucNumPezziButton++){
				// stampo i caratteri che precedono il field corrente, trasparenti
				pParamsIcon->ucTransparent=1;
				// trovo la coordinata X [pixel]
				pParamsIcon->uiXpixel=pTesto->colonna+defFontPixelSizeFromFontType(pTesto->ucTipoFont)*i;
				// stampo il testo prima del field, passo il numero di chars da stampare
				pParamsIcon->pucVettore=&pTesto->pucDisplayString[i];
				vWriteIconElement_N(pParamsIcon, pTesto->button.ucCharStartField[ucNumPezziButton]-i);
				// indice carattere corrente: lo porto sull'indice del primo carattere del field corrente
				i=pTesto->button.ucCharStartField[ucNumPezziButton];
				// stampo il field corrente; se il field corrente � premuto, stampo in rosso!
				// sicuramente, trattandosi di un field, non sar� trasparente
				pParamsIcon->ucTransparent=0;
				// coordinata iniziale di stampa del testo...
				pParamsIcon->uiXpixel=pTesto->colonna+defFontPixelSizeFromFontType(pTesto->ucTipoFont)*i;
				// se field premuto... visualizzo back rosso
				if (defIsButtonPressed(pTesto->button.ucFlags[ucNumPezziButton])){
					// background color=rosso
					pParamsIcon->ucBackgroundColor=defLCD_Color_Red;
					// e lo visualizzo spostato a destra-basso di 2 pixel
					pParamsIcon->uiYpixel+=defPressedButton_PixelOffsetY;
					pParamsIcon->uiXpixel+=defPressedButton_PixelOffsetX;
				}
				// field non premuto, background color=normale
				else{
					pParamsIcon->ucBackgroundColor=pTesto->structProperties.ucBackGroundColor;
				}
				pParamsIcon->pucVettore=&pTesto->pucDisplayString[i];
				// stampo il field: passo il numero di caratteri del field
				vWriteIconElement_N(pParamsIcon, pTesto->button.ucLenField[ucNumPezziButton]);
				// reimposto la coordinata Y in modo corretto...
				// la coordinata X viene ricalcolata da zero
				if (defIsButtonPressed(pTesto->button.ucFlags[ucNumPezziButton])){
					pParamsIcon->uiYpixel-=defPressedButton_PixelOffsetY;
				}
				// passo al prossimo carattere, saltando tutti quelli del field attuale
				i+=pTesto->button.ucLenField[ucNumPezziButton];
			}
			// stampo l'ultima parte di testo non field eventualmente restante a destra
			// sar� sicuramente trasparente
			pParamsIcon->ucTransparent=1;
			// trovo la coordinata X [pixel]
			pParamsIcon->uiXpixel=pTesto->colonna+defFontPixelSizeFromFontType(pTesto->ucTipoFont)*i;
			// passo il puntatore al primo carattere successivo al field appena stampato
			pParamsIcon->pucVettore=&pTesto->pucDisplayString[i];
			// stampo l'ultima parte del testo...
			vWriteIconElement(pParamsIcon);
			break;
	}

}//void vMapTesto(TipoTestoOnDisplay *pTesto)



// procedura che esegue il rendering dei caratteri:
// percorre la lista dei testi a video e compone la matrice delle icone e delle bitmap
unsigned char ucRenderTextOnVideo(void){
static TipoStructTextOnDisplayQueue xdata * p;
	// punto al primo elemento della coda=testa
	p=&headTextOnDisplayQueue;
	// percorro tutti gli elementi di testo per stamparli a video
	while(1){
		if (!p->pNext)
			break;
		p=p->pNext;
		vMapTesto(p->pTesto);
		// se lista troppo piena, e ci sono ancora elementi in lista, la trasmetto e la reinizializzo
		if (p->pNext&&ucTroppoPienaListaLcd()){
			vCloseListDisplayElements();
			invio_lista();
			vInitListDisplayElements();
		}

	}
	// tutto ok
	return 1;
}// unsigned char ucRenderTextOnVideo(void)



// ============================================================================================
// ============================================================================================
// ============================================================================================
// ============================================================================================
//
// qui iniziano le funzioni che possono essere chiamate dall'utente
//
//
// ============================================================================================
// ============================================================================================
// ============================================================================================
// ============================================================================================



// funzione che permette di modificare un testo che appare a video
#if 0
	unsigned char ucChangeText(TipoTestoOnDisplay xdata*ptesto, unsigned char xdata *pstr){
		if (!ucDeleteTesto(ptesto))
			return 0;
		ptesto->pucDisplayString=pstr;
		// inserisco testo a video
		return ucTextOnDisplay(ptesto);
	}	
#endif
#if 0
	// funzione che permette di spostare un testo che appare a video
	unsigned char ucMoveText(TipoTestoOnDisplay xdata*ptesto, TipoPosizioneDisplayRigaFont riga, TipoPosizioneDisplayColonnaFont colonna){
		if (!ucDeleteTesto(ptesto))
			return 0;
		ptesto->riga=riga;
		ptesto->colonna=colonna;
		// inserisco testo a video
		return ucTextOnDisplay(ptesto);
	}	
#endif

// fa un bel clear screen
unsigned char ucClearScreen(void){
	// forzo un refresh dell'immagine di sfondo
	vForceRefreshImmagine();
	// reset mappa icone e bitmap
	vResetIconsAndBitmap();
   // inzializzazione dati cursore
	memset(&LCD_Cursore,0,sizeof(LCD_Cursore));
	// reset della coda del testo a video
	return ucResetTextOnDisplayQueue();
}

// inizializza immagine di sfondo
void vInitImmagine(void){
	infoImmagine.ucCurImage=0;
	infoImmagine.ucPrevImage=1;
}	

// forza un rinfresco dell'immagine a video
void vForceRefreshImmagine(void){
	infoImmagine.ucPrevImage=!infoImmagine.ucCurImage;
}	


// funzione di caricamento immagine; non viene visualizzata finch� non si chiama la rinfresca immagine
void vCaricaImmagine(unsigned char ucNumImmagine){
	infoImmagine.ucCurImage=ucNumImmagine;
}	

// restituisce 1 se immagine di sfondo cambiata
unsigned char ucImmagineCambiata(void){
	if (infoImmagine.ucCurImage!=infoImmagine.ucPrevImage)
		return 1;
	return 0;		
}

#if 0
	// rinfresca immagine a video e fa toggle del banco immagine
	void vRinfrescaImmagine(void){
		LCD_LoadImageOnBackgroundBank(infoImmagine.ucCurImage);
		infoImmagine.ucPrevImage=infoImmagine.ucCurImage;
	}	
#endif

// inizializzazione valori di default del testo: font e colore
void vResetTextSettings(void){
	defaultTextSettings.structProperties.ucBackGroundColor=defLCD_Color_DarkBlue;
	defaultTextSettings.structProperties.ucForeGroundColor=defLCD_Color_White;
	defaultTextSettings.ucTipoFont=enumFontMedium;
}	


// inizializzazione lcd
void vInitLcd(void){

	memset(&sLCD,0,sizeof(sLCD));
	sLCD.ucTest=' ';
	
	// azzero var che indica se calibrazione lcd in corso
	ucCalibraLcdInCorso=0;

	// inizializzo stringa xdata che contiene solo 1 carattere
	uc1charString[0]=' ';
	uc1charString[1]=0x00;

	// inizializzo stringa xdata che contiene solo 2 caratteri
	uc2charString[0]=' ';
	uc2charString[1]=' ';
	uc2charString[2]=0x00;


	// reset kbd interface
	memset(&kbdInterface,0,sizeof(kbdInterface));
	// inizializzazione tastiera
	vInitKbd();
	// inizializzazione tabella degli id dell'lcd
	vInitLcdTable();
	// reset del modulo lcd
	vLCD_Init();
	// reset segnale di interrupt dell'lcd
//#pragma message "\n***disabled reset int at startup***\n"
    
	//vResetIntLCD_spi();
	// inizializzazione immagine di sfondo
	vInitImmagine();
	// reset impostazioni del font
	vResetTextSettings();

	// inzializzazione dati cursore
	memset(&LCD_Cursore,0,sizeof(LCD_Cursore));
	// azzero testo touch
	memset(&testoTouch,0,sizeof(testoTouch));

	// azzeramento mappa icone e mappa bitmap
	vResetIconsAndBitmap();
	// inizializzazione coda gestione testo su display
	vInitTextOnDisplayQueue();
	// lettura a 8 bit dal touch...
	vSelect8bitTouch();
	lettura_versione_FW(&sLCD.FW1,&sLCD.FW2);

}//void vInitLcd(void)


void vCloseLCD(void){
	ucClearScreen();
}		

// tastiera definita dal bitmap nKdb_1.bmp, con tasti funzione F1..4
#define defUseKbd1

#ifdef defUseKbd1
		#define defMinX_Kbd 7
		#define defXdistanceBetweenBtns 10
		#define defYdistanceBetweenBtns 6
		#define defMaxX_Kbd 320
		#define defMinY_Kbd 4
		#define defMaxY_Kbd 200
		#define defYdistanceBetweenBtns 6
		code unsigned char ucTableCharsKbd[24][5]={	
			"Ent","F4","rg","0","lf","dn",
			"Bck","F3","3",	"2","1", "Up",
			"Clr","F2","6",	"5","4", ".",
			"Esc","F1","9",	"8","7", "+ -",
		};
		code unsigned char ucTableKeysKbd[24]={	
			 KEY_ENTER,	KEY_F4,	KEY_RG,		KEY_0,		KEY_LF,		KEY_DN,
			 KEY_F5,		KEY_F3,	KEY_3,		KEY_2,		KEY_1,		KEY_UP,
			 KEY_CLR,	KEY_F2,	KEY_6,		KEY_5,		KEY_4,		KEY_POINT,
			 KEY_ESC,	KEY_F1,	KEY_9,		KEY_8,		KEY_7,		KEY_SIGN
		};
		code unsigned char ucSizeXkbd[24]={	
			 60, 40, 40, 40, 40, 40,
			 60, 40, 40, 40, 40, 40,
			 60, 40, 40, 40, 40, 40,
			 60, 40, 40, 40, 40, 40
		};
		code unsigned char ucRadiusXok_kbd[24]={	
			 25, 18, 18, 18, 18, 18,
			 25, 18, 18, 18, 18, 18,
			 25, 18, 18, 18, 18, 18,
			 25, 18, 18, 18, 18, 18
		};
		code unsigned char ucSizeYkbd[24]={	
			 30, 30, 30, 30, 30, 30,
			 30, 30, 30, 30, 30, 30,
			 30, 30, 30, 30, 30, 30,
			 30, 30, 30, 30, 30, 30
		};
		code unsigned char ucRadiusYok_kbd[24]={	
			 13, 13, 13, 13, 13, 13,
			 13, 13, 13, 13, 13, 13,
			 13, 13, 13, 13, 13, 13,
			 13, 13, 13, 13, 13, 13
		};
		xdata unsigned int uiCoordXY_buttonCenter[24][2];
		xdata unsigned int uiCoordXY_ok_rect_button[24][4];

		void vInitKbd(void){
			xdata unsigned int uiActCoordY;
			xdata unsigned char ucPrevIdxButton,ucCurIdxButton;
			xdata unsigned char i,j;
			ucPrevIdxButton=0;
			ucCurIdxButton=0;
			for (j=0;j<4;j++){
				if (j==0)
					uiActCoordY=defMinY_Kbd+ucSizeYkbd[ucCurIdxButton]/2;
				else
					uiActCoordY+=ucSizeYkbd[ucCurIdxButton]+defYdistanceBetweenBtns;
				for (i=0;i<6;i++){
					if (i==0){
						uiCoordXY_buttonCenter[ucCurIdxButton][0]=defMinX_Kbd+ucSizeXkbd[ucCurIdxButton]/2;
						uiCoordXY_buttonCenter[ucCurIdxButton][1]=uiActCoordY;
					}
					else{
						uiCoordXY_buttonCenter[ucCurIdxButton][0]=
										 uiCoordXY_buttonCenter[ucPrevIdxButton][0]
										+ucSizeXkbd[ucPrevIdxButton]/2
										+defXdistanceBetweenBtns
										+ucSizeXkbd[ucCurIdxButton]/2;
						uiCoordXY_buttonCenter[ucCurIdxButton][1]=uiActCoordY;
					}
					// coord x minima rettangolo di confidenza...
					uiCoordXY_ok_rect_button[ucCurIdxButton][0]=uiCoordXY_buttonCenter[ucCurIdxButton][0]-ucRadiusXok_kbd[ucCurIdxButton];
					// coord x massima rettangolo di confidenza...
					uiCoordXY_ok_rect_button[ucCurIdxButton][1]=uiCoordXY_buttonCenter[ucCurIdxButton][0]+ucRadiusXok_kbd[ucCurIdxButton];
					// coord x massima rettangolo di confidenza...
					uiCoordXY_ok_rect_button[ucCurIdxButton][2]=uiCoordXY_buttonCenter[ucCurIdxButton][1]-ucRadiusYok_kbd[ucCurIdxButton];
					// coord y massima rettangolo di confidenza...
					uiCoordXY_ok_rect_button[ucCurIdxButton][3]=uiCoordXY_buttonCenter[ucCurIdxButton][1]+ucRadiusYok_kbd[ucCurIdxButton];
					ucPrevIdxButton=ucCurIdxButton;
					ucCurIdxButton++;
				}
			}
		}		


void vCtrlPressedButtons(void){
	TipoStructTextOnDisplayQueue xdata * xdata p;

	// punto al primo elemento della coda=testa
	p=&headTextOnDisplayQueue;
	// percorro tutti gli elementi di testo per stamparli a video
	while(1){
		// se coda finita, esco
		if (!p->pNext)
			break;
		// passo al prossimo elemento	
		p=p->pNext;
		if (   (p->pTesto->button.ucType)
			&& (ucVerifyTouchOnTesto(p->pTesto,LCD_Cursore.posX, LCD_Cursore.posY))
			){
			vSignalButtonPressed(p->pTesto->button.ucNum,p->pTesto->ucCharOnTextSelected);
			break;
		}	
	}
}


	// funzione che verifica se � stato premuto il touch screen
	unsigned char ucCtrlTouch(void){
		extern void vPrintTestChar(unsigned char uc);
		extern unsigned char ucTouchScreenPressed(void);
		extern void vClearButtonPressed(void);
		unsigned int xdata posX,posY;

		// se touch screen non premuto, esco con 0
		if (!ucTouchedScreen(&posX,&posY)){
			LCD_Cursore.ucTouchWasPressed=0;
			// reset indice del bottone premuto
			if (!ucTouchScreenPressed())
				vClearButtonPressed();
			return 0;
		}   	
		// disabilitazione autorepeat: prima della pressione, lcd deve essere nello stato NON premuto
		if (LCD_Cursore.ucTouchWasPressed)
			return 0;

		// salvo la coordinata del punto di pressione
		LCD_Cursore.posX=posX;
		LCD_Cursore.posY=posY;
		uiCursorePosXnoLin=LCD_Cursore.posX;
		uiCursorePosYnoLin=LCD_Cursore.posY;


		if (!ucCalibraLcdInCorso){
			// linearizzazione della posizione lcd!
			vLinearizzaPosLcd();	

			uiCursorePosX=LCD_Cursore.posX;
			uiCursorePosY=LCD_Cursore.posY;
#if 0
			pui=(unsigned int *)&uiCoordXY_ok_rect_button[0][0];
			for (i=0;i<24;i++){
				if (  
  					   (LCD_Cursore.posX>=pui[0])
					 &&(LCD_Cursore.posX<=pui[1])
  					 &&(LCD_Cursore.posY>=pui[2])
					 &&(LCD_Cursore.posY<=pui[3])
					){
		//				vPrintTestChar(ucTableCharsKbd0[i]);
						vSignalNewKeyPressed(ucTableKeysKbd[i]);
						LCD_Beep();
						break;
					}				
					// se click su una riga gi� superata, esco
  					if (LCD_Cursore.posY<pui[2])
  						break;
					pui+=4;
			}
#endif

		}
			
		// controllo se sono stati premuti pulsanti
		vCtrlPressedButtons();
		
		// indico touch premuto	
  		LCD_Cursore.ucTouchPressed=1;
		// salvo flag per aspettare rilascio del touch  	
  		LCD_Cursore.ucTouchWasPressed=1;

		// indico che nuovo tasto � a disposizione
  		return 1;
	}
#else

	// funzione che verifica se � stato premuto il touch screen
	unsigned char ucCtrlTouch(void){
		extern void vPrintTestChar(unsigned char uc);
		unsigned int xdata posX,posY,ucRiga,ucColonna;
		code unsigned int *xdata pui;
		register unsigned char i;
#if 0
		#define defMinX_Kbd_0 0
		#define defMaxX_Kbd_0 320
		#define defMinY_Kbd_0 0
		#define defMaxY_Kbd_0 200
		#define defSizeXbutton_Kbd_0 53
		#define defSizeYbutton_Kbd_0 46
		#define defMarginOkX_Kbd_0 5
		#define defMarginOkY_Kbd_0 5
		code unsigned char ucTableCharsKbd0[]={	
			'E','>','0','<','^',
			'C','3','2','1','v',
			'K','6','5','4','.',
			'X','9','8','7','+',
		};
		code unsigned char ucTableKeysKbd0[]={	
			 KEY_ENTER,	KEY_RG,		KEY_0,		KEY_LF,		KEY_DN,
			 KEY_CLR,	KEY_3,		KEY_2,		KEY_1,		KEY_UP,
			 100,			KEY_6,		KEY_5,		KEY_4,		KEY_POINT,
			 KEY_ESC,	KEY_9,		KEY_8,		KEY_7,		KEY_SIGN,
		};
		code unsigned int uiXYMinMaxTableCharsKbd0[]={	
			defMinX_Kbd_0+defMarginOkX_Kbd_0,defMinX_Kbd_0+2*defSizeXbutton_Kbd_0-defMarginOkX_Kbd_0,
			defMinY_Kbd_0+defMarginOkY_Kbd_0,defMinY_Kbd_0+defSizeYbutton_Kbd_0-defMarginOkY_Kbd_0,
			
			2*defSizeXbutton_Kbd_0+defMinX_Kbd_0+defMarginOkX_Kbd_0,defMinX_Kbd_0+3*defSizeXbutton_Kbd_0-defMarginOkX_Kbd_0,
			defMinY_Kbd_0+defMarginOkY_Kbd_0,defMinY_Kbd_0+defSizeYbutton_Kbd_0-defMarginOkY_Kbd_0,
			
			3*defSizeXbutton_Kbd_0+defMinX_Kbd_0+defMarginOkX_Kbd_0,defMinX_Kbd_0+4*defSizeXbutton_Kbd_0-defMarginOkX_Kbd_0,
			defMinY_Kbd_0+defMarginOkY_Kbd_0,defMinY_Kbd_0+defSizeYbutton_Kbd_0-defMarginOkY_Kbd_0,
			
			4*defSizeXbutton_Kbd_0+defMinX_Kbd_0+defMarginOkX_Kbd_0,defMinX_Kbd_0+5*defSizeXbutton_Kbd_0-defMarginOkX_Kbd_0,
			defMinY_Kbd_0+defMarginOkY_Kbd_0,defMinY_Kbd_0+defSizeYbutton_Kbd_0-defMarginOkY_Kbd_0,
			
			5*defSizeXbutton_Kbd_0+defMinX_Kbd_0+defMarginOkX_Kbd_0,defMinX_Kbd_0+6*defSizeXbutton_Kbd_0-defMarginOkX_Kbd_0,
			defMinY_Kbd_0+defMarginOkY_Kbd_0,defMinY_Kbd_0+defSizeYbutton_Kbd_0-defMarginOkY_Kbd_0,


			defMinX_Kbd_0+defMarginOkX_Kbd_0,defMinX_Kbd_0+2*defSizeXbutton_Kbd_0-defMarginOkX_Kbd_0,
			defSizeYbutton_Kbd_0+defMinY_Kbd_0+defMarginOkY_Kbd_0,defMinY_Kbd_0+2*defSizeYbutton_Kbd_0-defMarginOkY_Kbd_0,
			
			2*defSizeXbutton_Kbd_0+defMinX_Kbd_0+defMarginOkX_Kbd_0,defMinX_Kbd_0+3*defSizeXbutton_Kbd_0-defMarginOkX_Kbd_0,
			defSizeYbutton_Kbd_0+defMinY_Kbd_0+defMarginOkY_Kbd_0,defMinY_Kbd_0+2*defSizeYbutton_Kbd_0-defMarginOkY_Kbd_0,
			
			3*defSizeXbutton_Kbd_0+defMinX_Kbd_0+defMarginOkX_Kbd_0,defMinX_Kbd_0+4*defSizeXbutton_Kbd_0-defMarginOkX_Kbd_0,
			defSizeYbutton_Kbd_0+defMinY_Kbd_0+defMarginOkY_Kbd_0,defMinY_Kbd_0+2*defSizeYbutton_Kbd_0-defMarginOkY_Kbd_0,
			
			4*defSizeXbutton_Kbd_0+defMinX_Kbd_0+defMarginOkX_Kbd_0,defMinX_Kbd_0+5*defSizeXbutton_Kbd_0-defMarginOkX_Kbd_0,
			defSizeYbutton_Kbd_0+defMinY_Kbd_0+defMarginOkY_Kbd_0,defMinY_Kbd_0+2*defSizeYbutton_Kbd_0-defMarginOkY_Kbd_0,
			
			5*defSizeXbutton_Kbd_0+defMinX_Kbd_0+defMarginOkX_Kbd_0,defMinX_Kbd_0+6*defSizeXbutton_Kbd_0-defMarginOkX_Kbd_0,
			defSizeYbutton_Kbd_0+defMinY_Kbd_0+defMarginOkY_Kbd_0,defMinY_Kbd_0+2*defSizeYbutton_Kbd_0-defMarginOkY_Kbd_0,


			defMinX_Kbd_0+defMarginOkX_Kbd_0,defMinX_Kbd_0+2*defSizeXbutton_Kbd_0-defMarginOkX_Kbd_0,
			2*defSizeYbutton_Kbd_0+defMinY_Kbd_0+defMarginOkY_Kbd_0,defMinY_Kbd_0+3*defSizeYbutton_Kbd_0-defMarginOkY_Kbd_0,
			
			2*defSizeXbutton_Kbd_0+defMinX_Kbd_0+defMarginOkX_Kbd_0,defMinX_Kbd_0+3*defSizeXbutton_Kbd_0-defMarginOkX_Kbd_0,
			2*defSizeYbutton_Kbd_0+defMinY_Kbd_0+defMarginOkY_Kbd_0,defMinY_Kbd_0+3*defSizeYbutton_Kbd_0-defMarginOkY_Kbd_0,
			
			3*defSizeXbutton_Kbd_0+defMinX_Kbd_0+defMarginOkX_Kbd_0,defMinX_Kbd_0+4*defSizeXbutton_Kbd_0-defMarginOkX_Kbd_0,
			2*defSizeYbutton_Kbd_0+defMinY_Kbd_0+defMarginOkY_Kbd_0,defMinY_Kbd_0+3*defSizeYbutton_Kbd_0-defMarginOkY_Kbd_0,
			
			4*defSizeXbutton_Kbd_0+defMinX_Kbd_0+defMarginOkX_Kbd_0,defMinX_Kbd_0+5*defSizeXbutton_Kbd_0-defMarginOkX_Kbd_0,
			2*defSizeYbutton_Kbd_0+defMinY_Kbd_0+defMarginOkY_Kbd_0,defMinY_Kbd_0+3*defSizeYbutton_Kbd_0-defMarginOkY_Kbd_0,
			
			5*defSizeXbutton_Kbd_0+defMinX_Kbd_0+defMarginOkX_Kbd_0,defMinX_Kbd_0+6*defSizeXbutton_Kbd_0-defMarginOkX_Kbd_0,
			2*defSizeYbutton_Kbd_0+defMinY_Kbd_0+defMarginOkY_Kbd_0,defMinY_Kbd_0+3*defSizeYbutton_Kbd_0-defMarginOkY_Kbd_0,



			defMinX_Kbd_0+defMarginOkX_Kbd_0,defMinX_Kbd_0+2*defSizeXbutton_Kbd_0-defMarginOkX_Kbd_0,
			3*defSizeYbutton_Kbd_0+defMinY_Kbd_0+defMarginOkY_Kbd_0,defMinY_Kbd_0+4*defSizeYbutton_Kbd_0-defMarginOkY_Kbd_0,
			
			2*defSizeXbutton_Kbd_0+defMinX_Kbd_0+defMarginOkX_Kbd_0,defMinX_Kbd_0+3*defSizeXbutton_Kbd_0-defMarginOkX_Kbd_0,
			3*defSizeYbutton_Kbd_0+defMinY_Kbd_0+defMarginOkY_Kbd_0,defMinY_Kbd_0+4*defSizeYbutton_Kbd_0-defMarginOkY_Kbd_0,
			
			3*defSizeXbutton_Kbd_0+defMinX_Kbd_0+defMarginOkX_Kbd_0,defMinX_Kbd_0+4*defSizeXbutton_Kbd_0-defMarginOkX_Kbd_0,
			3*defSizeYbutton_Kbd_0+defMinY_Kbd_0+defMarginOkY_Kbd_0,defMinY_Kbd_0+4*defSizeYbutton_Kbd_0-defMarginOkY_Kbd_0,
			
			4*defSizeXbutton_Kbd_0+defMinX_Kbd_0+defMarginOkX_Kbd_0,defMinX_Kbd_0+5*defSizeXbutton_Kbd_0-defMarginOkX_Kbd_0,
			3*defSizeYbutton_Kbd_0+defMinY_Kbd_0+defMarginOkY_Kbd_0,defMinY_Kbd_0+4*defSizeYbutton_Kbd_0-defMarginOkY_Kbd_0,
			
			5*defSizeXbutton_Kbd_0+defMinX_Kbd_0+defMarginOkX_Kbd_0,defMinX_Kbd_0+6*defSizeXbutton_Kbd_0-defMarginOkX_Kbd_0,
			3*defSizeYbutton_Kbd_0+defMinY_Kbd_0+defMarginOkY_Kbd_0,defMinY_Kbd_0+4*defSizeYbutton_Kbd_0-defMarginOkY_Kbd_0,

			
		};
#endif
		
		// se touch screen non premuto, esco con 0
	   if (!ucTouchedScreen(&posX,&posY)){
   		LCD_Cursore.ucTouchWasPressed=0;
   		return 0;
		}   	
	// disabilitazione autorepeat: prima della pressione, lcd deve essere nello stato NON premuto
  		if (LCD_Cursore.ucTouchWasPressed)
  			return 0;

	// salvo la coordinata del punto di pressione
		LCD_Cursore.posX=posX;
		LCD_Cursore.posY=posY;


		if (!ucCalibraLcdInCorso){
			// linearizzazione della posizione lcd!
			vLinearizzaPosLcd();	
#if 0
			pui=uiXYMinMaxTableCharsKbd0;
			for (i=0;i<sizeof(uiXYMinMaxTableCharsKbd0)/(sizeof(uiXYMinMaxTableCharsKbd0[0])*4);i++){
				if (  
  					   (LCD_Cursore.posX>=pui[0])
					 &&(LCD_Cursore.posX<=pui[1])
  					 &&(LCD_Cursore.posY>=pui[2])
					 &&(LCD_Cursore.posY<=pui[3])
					){
						vSignalNewKeyPressed(ucTableKeysKbd0[i]);
						break;
					}				
					// se click su una riga gi� superata, esco
  					if (LCD_Cursore.posY<pui[2])
  						break;
					pui+=4;
			}
#endif
		}
			
		
	// indico touch premuto	
  		LCD_Cursore.ucTouchPressed=1;
	// salvo flag per aspettare rilascio del touch  	
  		LCD_Cursore.ucTouchWasPressed=1;

	// stampo a video le coordinate del punto di touch
	#ifdef defStampaTouchCoords
   		ucDeleteTesto(&testoTouch);
			// scrivo un testo COL FONT SMALL
			memset(&testoTouch,0,sizeof(testoTouch));
			sprintf(stringa,"X:%03i(%03i) Y:%03i(%03i)",(int)LCD_Cursore.posX,posX,(int)LCD_Cursore.posY,posY);
			testoTouch.pucDisplayString=stringa;
			testoTouch.riga=150;
			testoTouch.colonna=0;
			testoTouch.ucTipoFont=enumFontSmall;
	 		testoTouch.structProperties.ucBackGroundColor=defLCD_Color_DarkBlue;
			testoTouch.structProperties.ucForeGroundColor=defLCD_Color_Orange;
			// inserisco testo a video
			ucTextOnDisplay(&testoTouch);
	#endif   	


		// indico che nuovo tasto � a disposizione
  		return 1;
	}
#endif

// funzione che permette di selezionare il font del testo di default
void vSelectFont(enumFontType ucFont){
	defaultTextSettings.ucTipoFont=ucFont;
}

// funzione che permette di selezionare il colore di default del testo
void vSelectTextColor(unsigned char ucColor){
	defaultTextSettings.structProperties.ucForeGroundColor=ucColor;
}

// funzione che permette di selezionare il colore di default del background testo 
void vSelectBackColor(unsigned char ucColor){
	defaultTextSettings.structProperties.ucBackGroundColor=ucColor;
}

#if 0
	// funzione che stampa a video un testo, usando i valori di default per font e colori
	// questo testo NON puo' essere modificato n� spostato, viene cancellato dalla clearscreen
	unsigned char ucPrintStaticText(xdata char *stringa,TipoPosizioneDisplayRigaFont row, TipoPosizioneDisplayColonnaFont col){

		memset(&testoPrintDefault,0,sizeof(testoPrintDefault));
		testoPrintDefault.pucDisplayString=stringa;
		testoPrintDefault.riga=row;
		testoPrintDefault.colonna=col;
		testoPrintDefault.ucTipoFont=defaultTextSettings.ucTipoFont;
 		testoPrintDefault.structProperties.ucBackGroundColor=defaultTextSettings.structProperties.ucBackGroundColor;
		testoPrintDefault.structProperties.ucForeGroundColor=defaultTextSettings.structProperties.ucForeGroundColor;
		// inserisco testo a video
		return ucTextOnDisplay(&testoPrintDefault);
	}	
#endif


// funzione che stampa a video un testo, usando font medium e colori specificati
unsigned char ucPrintStaticText_MediumFont_Color(xdata char *stringa,
												 TipoPosizioneDisplayRigaFont row, 
												 TipoPosizioneDisplayColonnaFont col,
												 unsigned char ucForeColor,
												 unsigned char ucBackColor
												 ){

	memset(&testoPrintDefault,0,sizeof(testoPrintDefault));
	testoPrintDefault.pucDisplayString=stringa;
	testoPrintDefault.riga=row;
	testoPrintDefault.colonna=col;
	testoPrintDefault.ucTipoFont=enumFontMedium;
 	testoPrintDefault.structProperties.ucBackGroundColor=ucBackColor;
	testoPrintDefault.structProperties.ucForeGroundColor=ucForeColor;
	// inserisco testo a video
	return ucTextOnDisplay(&testoPrintDefault);
}	


// funzione che stampa a video un testo small per dare nomi a colonne tabella
unsigned char ucPrintSmallText_ColNames(xdata char *stringa,TipoPosizioneDisplayRigaFont row, TipoPosizioneDisplayColonnaFont col){

	memset(&testoPrintDefault,0,sizeof(testoPrintDefault));
	testoPrintDefault.pucDisplayString=stringa;
	testoPrintDefault.riga=row;
	testoPrintDefault.colonna=col;
	testoPrintDefault.ucTipoFont=enumFontSmall;
 	testoPrintDefault.structProperties.ucBackGroundColor=defLCD_Color_Cyan;//defLCD_Color_LightGreen;
	testoPrintDefault.structProperties.ucForeGroundColor=defLCD_Color_Black;
	// imposto la propriet� di trasparenza...
	// testoPrintDefault.structProperties.ucProperties|=defBitTextProperties_Transparent;
	// inserisco testo a video
	return ucTextOnDisplay(&testoPrintDefault);
}	

// funzione che stampa a video un testo small per dare nomi a colonne tabella
unsigned char ucPrintSmallText_ColNames_BackColor(xdata char *stringa,TipoPosizioneDisplayRigaFont row, TipoPosizioneDisplayColonnaFont col,unsigned char ucBackColor){

	memset(&testoPrintDefault,0,sizeof(testoPrintDefault));
	testoPrintDefault.pucDisplayString=stringa;
	testoPrintDefault.riga=row;
	testoPrintDefault.colonna=col;
	testoPrintDefault.ucTipoFont=enumFontSmall;
 	testoPrintDefault.structProperties.ucBackGroundColor=ucBackColor;//defLCD_Color_LightGreen;
	testoPrintDefault.structProperties.ucForeGroundColor=defLCD_Color_Black;
	// imposto la propriet� di trasparenza...
	// testoPrintDefault.structProperties.ucProperties|=defBitTextProperties_Transparent;
	// inserisco testo a video
	return ucTextOnDisplay(&testoPrintDefault);
}	



			

// funzione che stampa a video un testo
// ucCentered: maschera di bit composta come segue
// 1=ucCentered.0=1--> centrato
// 2=ucCentered.1=1--> non togliere blank a sinistra e destra della stringa, per default li toglie
// 3=ucCentered.2=1--> non mettere in uppercase
// 4=ucCentered.3=1--> usa pulsante rosso degli allarmi
unsigned char ucPrintTitleButton(	xdata char *stringa,
									TipoPosizioneDisplayRigaFont row, 
									TipoPosizioneDisplayColonnaFont col,
									enumFontType fontType,
									unsigned char ucNumButton,
									unsigned int uiColor,
									unsigned char ucCentered
								){
	unsigned char ucTheButtonIsPressed(unsigned char ucNumButton);
	static unsigned char xdata myTitleString[45];
	xdata unsigned char ucStatus;
	memset(&testoPrintDefault,0,sizeof(testoPrintDefault));
	if ((ucCentered&2)==0)
		vStrcpy_trimBlank(myTitleString,stringa,44);
	else
		mystrcpy(myTitleString,stringa,sizeof(myTitleString)-1);
	if ((ucCentered&4)==0)
		vStrUpperCase(myTitleString);
	testoPrintDefault.pucDisplayString=myTitleString;
	if (fontType>=enumNumOfFonts)
		testoPrintDefault.ucTipoFont=defaultTextSettings.ucTipoFont;
	else
		testoPrintDefault.ucTipoFont=fontType;
	testoPrintDefault.riga=row;
	if (ucCentered&8)
		testoPrintDefault.button.ucType=enumButton_StaticAlarm;
	else
		testoPrintDefault.button.ucType=enumButton_Static; // button di tipo static...
	// il testo deve comparire centrato...
	if (ucCentered&1)
		testoPrintDefault.colonna=(defLcdWidthX_pixel/2)-(strlen(myTitleString)*(int)defFontPixelSizeFromFontType(testoPrintDefault.ucTipoFont))/2;
	else
		testoPrintDefault.colonna=col;
	testoPrintDefault.button.ucNum=ucNumButton;
	if (ucCentered&8)
		testoPrintDefault.structProperties.ucForeGroundColor=defLCD_Color_Black;
	else
		testoPrintDefault.structProperties.ucForeGroundColor=defaultTextSettings.structProperties.ucForeGroundColor;
	ucStatus=ucTheButtonIsPressed(ucNumButton);
	// se bottone non premuto, uso colore sfondo default
	if (ucStatus==0){
		if (uiColor==0xFFFF){
   			// il button ha testo con sfondo trasparente
			defSetButtonTransparent(testoPrintDefault.button.ucFlags[0]);
 			testoPrintDefault.structProperties.ucBackGroundColor=defaultTextSettings.structProperties.ucBackGroundColor;
		}
		else{
			if ((uiColor&0xFF)==defLCD_Color_Yellow)
				testoPrintDefault.structProperties.ucForeGroundColor=defLCD_Color_Black;
			// se pressed, background del button � rosso
 			testoPrintDefault.structProperties.ucBackGroundColor=uiColor&0xFF;
		}
	}
	else{
		// se pressed, il button ha testo con sfondo visibile...
	    defSetButtonPressed(testoPrintDefault.button.ucFlags[0]);
		// se pressed, background del button � rosso
 		testoPrintDefault.structProperties.ucBackGroundColor=defLCD_Color_Red;
	}
	// il button NON ha l'area sensibile al touch!!!
    testoPrintDefault.structTouchArea.ucEnable=0;
	// inserisco testo a video
	return ucTextOnDisplay(&testoPrintDefault);
}	

// funzione che stampa a video un button bitmap
unsigned char ucPrintBitmapButton(	enumBitmap_40x24_Type bitmapType,
									TipoPosizioneDisplayRigaFont row, 
									TipoPosizioneDisplayColonnaFont col,
									unsigned char ucNumButton
								){
	unsigned char ucTheButtonIsPressed(unsigned char ucNumButton);
	// l'area equivalente e' di tre caratteri
	xdata static unsigned char stringButtonBitmap[]="111";
	xdata unsigned char ucStatus;
	memset(&testoPrintDefault,0,sizeof(testoPrintDefault));
	testoPrintDefault.riga=row;
	testoPrintDefault.button.ucType=enumButton_Bitmap; // button di tipo bitmap...
	testoPrintDefault.colonna=col;
	testoPrintDefault.pucDisplayString=stringButtonBitmap;
	testoPrintDefault.button.ucNum=ucNumButton;
	testoPrintDefault.ucTipoFont=enumFontMedium;
	testoPrintDefault.structProperties.ucForeGroundColor=defaultTextSettings.structProperties.ucForeGroundColor;
	ucStatus=ucTheButtonIsPressed(ucNumButton);
	// salvo il bitmap che il button deve rappresentare
	testoPrintDefault.button.ucBitmap=bitmapType;
	// se bottone non premuto, uso bitmap di default
	if (ucStatus==0){
		defSetButtonUnpressed(testoPrintDefault.button.ucFlags[0]);
	}
	// altrimenti devo andare 64 bitmap pi� in la'...
	else{
		defSetButtonPressed(testoPrintDefault.button.ucFlags[0]);
	}
	// il button ha l'area sensibile al touch!!!
    testoPrintDefault.structTouchArea.ucEnable=1;
	// inserisco testo a video
	return ucBitmap40x25OnDisplay(&testoPrintDefault);
	

}	


// funzione che stampa a video un testo, contornato da button usando i valori di default per font e colori
unsigned char ucPrintStaticButton(	xdata char *stringa,
									TipoPosizioneDisplayRigaFont row, 
									TipoPosizioneDisplayColonnaFont col,
									enumFontType fontType,
									unsigned char ucNumButton,
									unsigned int uiColor
								){
	unsigned char ucTheButtonIsPressed(unsigned char ucNumButton);
	xdata unsigned char ucStatus;
	memset(&testoPrintDefault,0,sizeof(testoPrintDefault));
	testoPrintDefault.pucDisplayString=stringa;
	testoPrintDefault.riga=row;
	testoPrintDefault.button.ucType=enumButton_Normal; // button di tipo 1...
	testoPrintDefault.colonna=col;
	testoPrintDefault.button.ucNum=ucNumButton;
	if (fontType>=enumNumOfFonts)
		testoPrintDefault.ucTipoFont=defaultTextSettings.ucTipoFont;
	else
		testoPrintDefault.ucTipoFont=fontType;
	testoPrintDefault.structProperties.ucForeGroundColor=defaultTextSettings.structProperties.ucForeGroundColor;
	ucStatus=ucTheButtonIsPressed(ucNumButton);
	// se bottone non premuto, uso colore sfondo default
	if (ucStatus==0){
		if (uiColor==0xFFFF){
   			// il button ha testo con sfondo trasparente
			defSetButtonTransparent(testoPrintDefault.button.ucFlags[0]);
 			testoPrintDefault.structProperties.ucBackGroundColor=defaultTextSettings.structProperties.ucBackGroundColor;
		}
		else{
			if (((uiColor&0xFF)==defLCD_Color_Yellow)||((uiColor&0xFF)==defLCD_Color_Gray)){
				testoPrintDefault.structProperties.ucForeGroundColor=defLCD_Color_Black;
			}
			// se pressed, background del button � rosso
 			testoPrintDefault.structProperties.ucBackGroundColor=uiColor&0xFF;
		}
	}
	else{
		// se pressed, il button ha testo con sfondo visibile...
	    defSetButtonPressed(testoPrintDefault.button.ucFlags[0]);
		// se pressed, background del button � rosso
 		testoPrintDefault.structProperties.ucBackGroundColor=defLCD_Color_Red;
	}
	// il button ha l'area sensibile al touch!!!
    testoPrintDefault.structTouchArea.ucEnable=1;
	// inserisco testo a video
	return ucTextOnDisplay(&testoPrintDefault);
}	


// ricerca quale campo del button � stato premuto...
// Restituisce:
// - 0xff se carattere premuto non appartiene a nessun field
// - 0..N<0xff indice del field cui appartiene il carattere premuto
unsigned char ucSearchFieldButtonPressed(	TipoTestoOnDisplay xdata *pTesto,
											unsigned char ucNumButton
										){
	// indice carattere premuto
	xdata unsigned char ucIdx;
	// serve per cercare il field
	xdata unsigned char i;
	// indice del carattere del button che � stato premuto...
	extern unsigned char ucIdxCharButtonPressed(unsigned char ucNumButton);
	// trovo l'indice del carattere premuto
	ucIdx=ucIdxCharButtonPressed(ucNumButton);
	// trovo a quale campo appartiene il carattere premuto
	for (i=0;i<defMaxButtonFields;i++){
		// il field ha indici: start, start+1,..., start+len-1
		if (  (ucIdx>=pTesto->button.ucCharStartField[i])
			&&(ucIdx< pTesto->button.ucCharStartField[i]+pTesto->button.ucLenField[i])
			){
			// restituisco l'indice del field cui appartiene il carattere premuto
			return i;
		}
	}
	// restituisco 0xff per indicare che il carattere premuto non appartiene a nessun field!!!!
	return 0xff;
}//unsigned char ucSearchFieldButtonPressed...



// permette di definire un button di tipo field, con vari campi al suo interno...
unsigned char ucPrintFieldButton(	xdata char *stringa,
									TipoPosizioneDisplayRigaFont row, 
									TipoPosizioneDisplayColonnaFont col,
									unsigned char ucNumButton,
									TipoButtonStruct xdata *pbutton,
									unsigned char ucBackGroundColor
								){
	unsigned char ucTheButtonIsPressed(unsigned char ucNumButton);
	xdata unsigned char ucIdx;
	memset(&testoPrintDefault,0,sizeof(testoPrintDefault));
	testoPrintDefault.pucDisplayString=stringa;
	testoPrintDefault.riga=row;
	testoPrintDefault.button.ucType=enumButton_Field; // button di tipo 1...
	testoPrintDefault.colonna=col;
	testoPrintDefault.button.ucNum=ucNumButton;
	// copio la struttura che contiene le info sui fields...
	memcpy(&(testoPrintDefault.button),pbutton,sizeof(testoPrintDefault.button));
	testoPrintDefault.ucTipoFont=defaultTextSettings.ucTipoFont;
	// sfondo sempre a default...
	testoPrintDefault.structProperties.ucBackGroundColor=ucBackGroundColor;
	if (ucBackGroundColor==defLCD_Color_Yellow)
		testoPrintDefault.structProperties.ucForeGroundColor=defLCD_Color_Black;
	else
		testoPrintDefault.structProperties.ucForeGroundColor=defaultTextSettings.structProperties.ucForeGroundColor;
	// se bottone non premuto, uso colore sfondo default
	if (ucTheButtonIsPressed(ucNumButton)){
		// trovo qual � il field che � stato premuto
		ucIdx=ucSearchFieldButtonPressed(&testoPrintDefault,ucNumButton);
		// salvo nel button l'info sul button premuto
		vSetFieldButtonPressed(ucNumButton, ucIdx);
		// se carattere appartiene ad un field...
		if (ucIdx!=0xff)
			// indico che quel field � premuto...
			defSetButtonPressed(testoPrintDefault.button.ucFlags[ucIdx]);
	}
	else
		// salvo nel button l'info sul button NON premuto
		vSetFieldButtonPressed(ucNumButton, 0xFF);
	// il button ha l'area sensibile al touch!!!
    testoPrintDefault.structTouchArea.ucEnable=1;
	// inserisco testo a video
	return ucTextOnDisplay(&testoPrintDefault);
}	





// funzione che stampa a video un testo, usando i valori di default per il cursore
unsigned char ucPrintCursorText(	char myChar,
									TipoPosizioneDisplayRigaFont row, 
									TipoPosizioneDisplayColonnaFont col,
									enumFontType fontType
									){
static unsigned char xdata cursorText[2];
	memset(&testoPrintDefault,0,sizeof(testoPrintDefault));
	//if (isalnum(myChar)||(myChar=='*'))
	if (myChar)
		cursorText[0]=myChar;
	else
		cursorText[0]=' ';
	cursorText[1]=0;
	testoPrintDefault.pucDisplayString=cursorText;
	testoPrintDefault.riga=row;
	testoPrintDefault.colonna=col;
	if (fontType>=enumNumOfFonts)
		testoPrintDefault.ucTipoFont=defaultTextSettings.ucTipoFont;
	else
		testoPrintDefault.ucTipoFont=fontType;
	//testoPrintDefault.structProperties.ucBackGroundColor=defLCD_Color_LightGreen;
 	testoPrintDefault.structProperties.ucBackGroundColor=defLCD_Color_Yellow;
	testoPrintDefault.structProperties.ucForeGroundColor=defLCD_Color_Black;
	// inserisco testo a video
	return ucTextOnDisplay(&testoPrintDefault);
}	


// funzione che stampa a video un testo, usando i valori di default per font e colori
// e inserendo abilitazione touch e funzione di callback								 
// quesot testo puo' essere modificato, spostato etc
unsigned char ucPrintTextTouch(TipoTestoOnDisplay xdata *ptesto,
										 xdata char *stringa, 
										 TipoPosizioneDisplayRigaFont row, 
										 TipoPosizioneDisplayColonnaFont col, 
										 unsigned char ucEnTouch
										 //,TipoFunCallBackTouch pFunCallBackTouch
										 ){
										 
	memset(ptesto,0,sizeof(*ptesto));
	ptesto->pucDisplayString=stringa;
	ptesto->riga=row;
	ptesto->colonna=col;
	ptesto->ucTipoFont=defaultTextSettings.ucTipoFont;
 	ptesto->structProperties.ucBackGroundColor=defaultTextSettings.structProperties.ucBackGroundColor;
	ptesto->structProperties.ucForeGroundColor=defaultTextSettings.structProperties.ucForeGroundColor;
    ptesto->structTouchArea.ucEnable=ucEnTouch;
	//ptesto->pFunCallBackTouch=pFunCallBackTouch;
	// inserisco testo a video
	return ucTextOnDisplay(ptesto);
}	
	
// funzione che controlla in background se � stato premuto lcd
// ritorna 0 se NON � stato premuto
// ritorna 1 se � stato premuto; prima di ritornare chiama un refresh lcd
unsigned char ucBackGroundCtrlLcd(void){
	// controllo del touch screen
	if (ucCtrlTouch()){
		//if (ucCalibraLcdInCorso){
		//	vRefreshLcd(enumRefreshLcd_Touched);
		//}
		return 1;
	}
	return 0;
}

// ============================================================================================
// ============================================================================================
// ============================================================================================
// ============================================================================================
//
// qui iniziano le funzioni di test della libreria
//
//
// ============================================================================================
// ============================================================================================
// ============================================================================================
// ============================================================================================




#if 0

// test di selezione carattere da touch screen
// su stringa composta da abcdefghij______
// decido quale lettera � stata selezionata
void vHandleCallbackTest(enumEventsCallback eventCallback,void *pTesto){
	switch(eventCallback){
		case eventCallback_Created:
			break;
		case eventCallback_Destroyed:
			break;
		case eventCallback_Touched:
			// cancella tutto
			if (stringaTestTouch[testoCallback.pTestoOnQueue->ucCharOnTextSelected]=='*'){
				memset(stringaTestoScrivi,' ',defMaxCharTestoScrivi-2);
				stringaTestoScrivi[defMaxCharTestoScrivi-1]=0x00;
				idxStringaTestoScrivi=0;
				ucChangeText(&testoScrivi,stringaTestoScrivi);
			}
			else if (stringaTestTouch[testoCallback.pTestoOnQueue->ucCharOnTextSelected]=='-'){
				if (idxStringaTestoScrivi>=1){
					stringaTestoScrivi[idxStringaTestoScrivi-1]=' ';
					idxStringaTestoScrivi--;
				}	
				else{	
					memset(stringaTestoScrivi,' ',defMaxCharTestoScrivi-2);
					stringaTestoScrivi[defMaxCharTestoScrivi-1]=0x00;
					idxStringaTestoScrivi=0;
				}	
				ucChangeText(&testoScrivi,stringaTestoScrivi);
			}
			else{
				if (idxStringaTestoScrivi<defMaxCharTestoScrivi-1){
					stringaTestoScrivi[idxStringaTestoScrivi++]=stringaTestTouch[testoCallback.pTestoOnQueue->ucCharOnTextSelected];
					ucChangeText(&testoScrivi,stringaTestoScrivi);
				}				
			}	
			break;
	}		
}

	// funzione di test lcd
	void vTestLcd(void){
		TipoTestoOnDisplay xdata testo;
		TipoDisplayChar xdata stringaMsg[]="ABCDE 1.2";
		unsigned char xdata errore;
		errore=0;

	#if 1	
		// scrivo un testo COL FONT BIG
		memset(&testo,0,sizeof(testo));
		testo.pucDisplayString=stringaMsg;
		testo.riga=10;
		testo.colonna=10;
 		testo.structProperties.ucBackGroundColor=defLCD_Color_Gray;
		testo.structProperties.ucForeGroundColor=defLCD_Color_Pink;
		testo.ucTipoFont=enumFontBig;
	   testo.structTouchArea.ucEnable=1;
		// inserisco testo a video
		ucTextOnDisplay(&testo);
		
		ucPrintStaticText(stringaMsg,32,32);
		
		// scrivo un testo COL FONT MEDIUM
		memset(&testo,0,sizeof(testo));
		testo.pucDisplayString=stringaMsg;
		testo.riga=100;
		testo.colonna=20;
		testo.ucTipoFont=enumFontMedium;
 		testo.structProperties.ucBackGroundColor=defLCD_Color_Gray;
		testo.structProperties.ucForeGroundColor=defLCD_Color_Maroon;
		testo.structProperties.ucFlashing=0;
	   testo.structTouchArea.ucEnable=1;
		// inserisco testo a video
		ucTextOnDisplay(&testo);
		
		// scrivo un testo COL FONT SMALL
		memset(&testo,0,sizeof(testo));
		testo.pucDisplayString=stringaMsg;
		testo.riga=200;
		testo.colonna=30;
		testo.ucTipoFont=enumFontSmall;
 		testo.structProperties.ucBackGroundColor=defLCD_Color_Gray;
		testo.structProperties.ucForeGroundColor=defLCD_Color_Pink;
		testo.structProperties.ucFlashing=0;
	   testo.structTouchArea.ucEnable=1;
		// inserisco testo a video
		ucTextOnDisplay(&testo);
		
		
		// test per callback col touch screen
		#ifdef defStampaTouchCoords
		memset(&testoCallback,0,sizeof(testoCallback));
		testoCallback.pucDisplayString=stringaTestTouch;
		testoCallback.riga=170;
		testoCallback.colonna=20;
		testoCallback.ucTipoFont=enumFontMedium;
		testoCallback.pFunCallBackTouch=vHandleCallbackTest;
 		testoCallback.structProperties.ucBackGroundColor=defLCD_Color_DarkBlue;
		testoCallback.structProperties.ucForeGroundColor=defLCD_Color_White;
		testoCallback.structProperties.ucFlashing=0;
	   testoCallback.structTouchArea.ucEnable=1;
		// inserisco testo a video
		ucTextOnDisplay(&testoCallback);
		
		memset(stringaTestoScrivi,' ',defMaxCharTestoScrivi-2);
		stringaTestoScrivi[defMaxCharTestoScrivi-1]=0x00;
		idxStringaTestoScrivi=0;
		// test per callback col touch screen
		memset(&testoScrivi,0,sizeof(testoScrivi));
		testoScrivi.pucDisplayString=stringaTestoScrivi;
		testoScrivi.riga=50;
		testoScrivi.colonna=20;
		testoScrivi.ucTipoFont=enumFontMedium;
 		testoScrivi.structProperties.ucBackGroundColor=defLCD_Color_DarkBlue;
		testoScrivi.structProperties.ucForeGroundColor=defLCD_Color_White;
		testoScrivi.structProperties.ucFlashing=0;
	   testoScrivi.structTouchArea.ucEnable=0;
		// inserisco testo a video
		ucTextOnDisplay(&testoScrivi);
	   #endif	
		
	#endif	

		// adesso faccio rendering del testo, cosi' determino la mappa icone, la mappa
		// bitmap etc
		vInitListDisplayElements();
		// adesso faccio rendering del testo, cosi' determino la mappa icone, la mappa
		// bitmap etc
		ucRenderTextOnVideo();	
		vCloseListDisplayElements();
		invio_lista();
		vRinfrescaImmagine();
		applicazione_lista();
		LCD_toggle_bank();
		if (ucCalibraLcdInCorso){
  			vRefreshLcd(enumRefreshLcd_Force_Refresh);
			vCalibraLcd();
			ucCalibraLcdInCorso=0;
		}

	//	if (!ucDeleteTextOnDisplayQueue(headTextOnDisplayQueue.pNext->pNext->pNext->pNext))
	//		errore++;

	//	if (!ucClearScreen())
	//		errore++;

	}//void vTestLcd(void)
#endif


// ============================================================================================
// ============================================================================================
// ============================================================================================
// ============================================================================================
//
// qui iniziano le funzioni di calibrazione lcd
//
//
// ============================================================================================
// ============================================================================================
// ============================================================================================
// ============================================================================================


	#define defScaleFactor (1.0)
	static xdata int iCalibraLcdIdeal[defNumColLcdCalibration*defNumRowLcdCalibration][2];
	static xdata int iCalibraLcdReal[defNumColLcdCalibration*defNumRowLcdCalibration][2];
	static TipoTestoOnDisplay xdata testoCalibraLcd;
	static xdata unsigned char stringCalibra[3]="X";

	// iCalibraLcdRealZone
	// array che contiene i punti di delimitazione delle varie zone della tabella di calibrazione
	// prima X, poi Y, a prtire dall'angolo sinistra alto del display (coordinate in pixel grandi), si scende in Y (X costante Y cala)
	// poi si riprende dall'alto e si sposta X a destra (X cala di tot, Y riparte da alto e ricala)
	// 
	//   P0           P4           P8           PC
	//    |         /  |         /  |         /  |
	//    |        /   |        /   |        /   |
	//   P1       /   P5       /   P9       /   PD
	//    |      /     |      /     |      /     |
	//    |     /      |     /      |     /      |
	//   P2    /      P6    /      PA    /      PE
	//    |   /        |   /        |   /        |
	//    |  /         |  /         |  /         |
	//   P3 /         P7 /         PB /         PF
	// 
	// 
	// 
	// fCoeffsCalibraLcd
	// array che contiene i coefficienti di calibrazione lcd, organizzati cos�:
	// i coefficienti sono sempre [a b c d] prima per il calcolo di X, poi per il calcolo di Y
	// nell'ordine:
	// quadrato Q1=P0P1P4P5, quadrato Q2=P1P2P5P6, quadrato Q3=P2P3P6P7, quadrato Q4=P4P5P8P9, quadrato Q5=P5P6P9PA,
	// quadrato Q6=P6P7PAPB, quadrato Q7=P8P9PCPD, quadrato Q8=P9PAPDPE, quadrato Q9PAPBPEPF, 
	//   P0-----------P4-----------P8-----------PC
	//    |    Q1      |    Q4      |    Q7      |
	//    |            |            |            |
	//   P1-----------P5-----------P9-----------PD
	//    |    Q2      |    Q5      |    Q8      |
	//    |            |            |            |
	//   P2-----------P6-----------PA-----------PE
	//    |    Q3      |    Q6      |    Q9      |
	//    |            |            |            |
	//   P3-----------P7-----------PB-----------PF
	// typedef struct _TipoStructLcdCalibration{
	//	int iCalibraLcdRealZone[defNumColLcdCalibration*defNumRowLcdCalibration][2];
	//	float fCoeffsCalibraLcd[(defNumColLcdCalibration-1)*(defNumRowLcdCalibration-1)*4*2];
	//}TipoStructLcdCalibration;
	xdata TipoStructLcdCalibration lcdCal;
#if 1
	code TipoStructLcdCalibration lcdCalDefault=
	{
		.uiCalibrationFormatVersion = 0,	// versione del formato...
		.ucCrc = 0,	// crc...
		.iCalibraLcdRealZone =
		{

					{270,200},{267,147},{270, 90},{271, 33},{193,199},{195,146},{195, 87},{195, 31},
					{116,201},{117,146},{116, 89},{117, 30},{43,199}, {40,145}, {41, 89}, {40, 30}
		},
		{
		   -0.001361,    0.310682,    0.304696,  -65.311539,    0.000019,   -0.018547,    0.127765,  -15.585415,
			0.000780,   -0.003509,   -0.152047,    1.684239,    0.000466,   -0.082605,   -0.073892,   13.633346,
			0.000246,    0.044506,   -0.048015,   -7.678679,   -0.000257,   -0.019707,    0.121587,   -3.371368,
			0.000260,   -0.012386,   -0.011565,   -2.311451,    0.000535,   -0.078056,    0.028892,   -4.218220,
			0.000228,   -0.007613,   -0.044415,    2.484574,   -0.000452,    0.065945,    0.105026,  -15.333828,
		   -0.000220,    0.032242,    0.042909,   -5.287276,    0.000693,   -0.034513,   -0.063621,   -0.484239,
			0.001049,   -0.113469,   -0.102832,    9.362617,   -0.000291,    0.028051,    0.124416,  -16.473883,
		   -0.000490,    0.110313,    0.038664,  -11.174402,   -0.000244,    0.021726,    0.081193,  -10.226192,
			0.000470,    0.024873,   -0.036863,   -4.452565,    0.000000,    0.000000,    0.016949,   -4.508475
		}
	};
#endif


	// funzione che calcola i coefficienti di calibrazione lcd
	// usando i dati memorizzati dalla vCalibraLcd
	void vCalcCoeffsLcdCalibration(void){
		extern void vSolve4x4System(float xdata *matrix_coeffs, float xdata *vector_data,float xdata *vector_result);


		// matrice ordinata per colonne; ogni RIGA deve contenere [xReale*yReale, xReale, yReale, 1]
		static xdata float fMatrix4x4[16];
		// vettore delle correzioni, definite come xIdeale-xReale per x, yIdeale-yReale per y
		static xdata float fVector4[4];
		// vettore dei coefficienti [a b c d], che permettono di calcolare il delta delta=a*xr*yr+b*xr+c*yr+d, xid=xr+delta
		static xdata float fResult4[4];
		unsigned char i,j;
		xdata int *xdata pBase,*xdata pIdeale;
		xdata float *xdata pCoeffsCalibraLcd;
#if 0		
		xdata unsigned char string_salva[30];
		xdata unsigned char * xdata pucBufAuxSave;
		xdata unsigned char ucBufAuxSave[400];
#endif		
		// copio le posizioni limite dei quadrati...
		memcpy(&(lcdCal.iCalibraLcdRealZone[0][0]),iCalibraLcdReal,sizeof(lcdCal.iCalibraLcdRealZone));
		pCoeffsCalibraLcd=lcdCal.fCoeffsCalibraLcd;
		// organizzo la prima matrice 4x4
		for (i=0;i<defNumColLcdCalibration-1;i++){	// loop che passa le colonne Q1 Q4 Q7
			for (j=0;j<defNumRowLcdCalibration-1;j++){ // loop che passa le righe Q1 Q2 Q3
				pIdeale=&iCalibraLcdIdeal[i*defNumRowLcdCalibration+j][0];
				// punto a P0/P1/P2/P4/P5/P6/P8/P9/PA
				pBase=&iCalibraLcdReal[i*defNumRowLcdCalibration+j][0];
				// P0.x=pBase[0] P0.y=pBase[1]
				// P1.x=pBase[1*2] P1.y=pBase[1*2+1]
				// P4.x=pBase[4*2] P4.y=pBase[4*2+1]
				// P5.x=pBase[5*2] P5.y=pBase[5*2+1]
				// occhio che matrice � organizzata a colonne, pertanto inserisco prima colonna che contiene gli xy
				// normalizzo dividendo per 256 in modo da ottenere coefficienti vicini ad 1
				fMatrix4x4[0]=pBase[0*2]*defScaleFactor;fMatrix4x4[0]*=pBase[0*2+1]*defScaleFactor;	// per evitare overflow...
				fMatrix4x4[1]=pBase[1*2]*defScaleFactor;fMatrix4x4[1]*=pBase[1*2+1]*defScaleFactor;
				fMatrix4x4[2]=pBase[4*2]*defScaleFactor;fMatrix4x4[2]*=pBase[4*2+1]*defScaleFactor;
				fMatrix4x4[3]=pBase[5*2]*defScaleFactor;fMatrix4x4[3]*=pBase[5*2+1]*defScaleFactor;
		
				// adesso inserisco le x				
				fMatrix4x4[4]=pBase[0*2]*defScaleFactor;
				fMatrix4x4[5]=pBase[1*2]*defScaleFactor;
				fMatrix4x4[6]=pBase[4*2]*defScaleFactor;
				fMatrix4x4[7]=pBase[5*2]*defScaleFactor;
						
				// adesso inserisco le y
				fMatrix4x4[8] =pBase[0*2+1]*defScaleFactor;
				fMatrix4x4[9] =pBase[1*2+1]*defScaleFactor;
				fMatrix4x4[10]=pBase[4*2+1]*defScaleFactor;
				fMatrix4x4[11]=pBase[5*2+1]*defScaleFactor;
						
				// adesso inserisco 1.0
				fMatrix4x4[12]=1.0;
				fMatrix4x4[13]=1.0;
				fMatrix4x4[14]=1.0;
				fMatrix4x4[15]=1.0;
						
				// preparo il vettore delle correzioni X=xIdeale-xReale
				fVector4[0]=(pIdeale[0]-pBase[0])*defScaleFactor;
				fVector4[1]=(pIdeale[1*2]-pBase[1*2])*defScaleFactor;
				fVector4[2]=(pIdeale[4*2]-pBase[4*2])*defScaleFactor;
				fVector4[3]=(pIdeale[5*2]-pBase[5*2])*defScaleFactor;
						
				// calcolo il vettore dei coefficienti
	 			vSolve4x4System(fMatrix4x4, fVector4,fResult4);
				memcpy(pCoeffsCalibraLcd,fResult4,sizeof(fResult4));
				pCoeffsCalibraLcd+=4;
						
				// preparo il vettore delle correzioni Y=yIdeale-yReale
				fVector4[0]=(pIdeale[1]-pBase[1])*defScaleFactor;
				fVector4[1]=(pIdeale[1*2+1]-pBase[1*2+1])*defScaleFactor;
				fVector4[2]=(pIdeale[4*2+1]-pBase[4*2+1])*defScaleFactor;
				fVector4[3]=(pIdeale[5*2+1]-pBase[5*2+1])*defScaleFactor;
						
				// calcolo il vettore dei coefficienti
	 			vSolve4x4System(fMatrix4x4, fVector4,fResult4);
				memcpy(pCoeffsCalibraLcd,fResult4,sizeof(fResult4));
				pCoeffsCalibraLcd+=4;
			}
		}
// no! salvo in flash!
#if 0
		// adesso ho la matrice di tutti i coefficienti di correzione... 		
		// LA BUTTO FUORI SULLA SERIALE
				#if 1
							pucBufAuxSave=ucBufAuxSave;
							vReceiveVdipString();
							// cancello coeff.txt
							strcpy(string_salva,"DLF COEFF.TXT\r");
							vSendVdipBinaryString(string_salva,strlen(string_salva));
							vReceiveVdipString();
							strcpy(string_salva,"OPW COEFF.TXT\r");
							vSendVdipBinaryString(string_salva,strlen(string_salva));
							vReceiveVdipString();
				#else
					   SD_Init();
					   SD_PutChar('C');
					   SD_PutChar('O');
					   SD_PutChar('E');
					   SD_PutChar('F');
					   SD_PutChar(0x0d);
					   SD_PutChar(0x0a);
				#endif
						for (i=0;i<sizeof(lcdCal.fCoeffsCalibraLcd)/sizeof(lcdCal.fCoeffsCalibraLcd[0]);i++){	
				#if 1
							*(float *)pucBufAuxSave=lcdCal.fCoeffsCalibraLcd[i];
							pucBufAuxSave+=sizeof(float);
				#else
							sprintf(string_salva,"%f,",lcdCal.fCoeffsCalibraLcd[i]);
							for (j=0;j<strlen(string_salva);j++)
							   SD_PutChar(string_salva[j]);
						   SD_PutChar(0x0d);
						   SD_PutChar(0x0a);
				#endif
						}	   
						// adesso ho la matrice CON I LIMITI DELLE ZONE DI CORREZIONE
						// LA BUTTO FUORI SULLA SERIALE
				#if 1
				#else
					   SD_PutChar('Z');
					   SD_PutChar('O');
					   SD_PutChar('N');
					   SD_PutChar('E');
					   SD_PutChar(0x0d);
					   SD_PutChar(0x0a);
				#endif
						for (i=0;i<sizeof(lcdCal.iCalibraLcdRealZone)/sizeof(lcdCal.iCalibraLcdRealZone[0]);i++){	
				#if 1
							*(int *)pucBufAuxSave=lcdCal.iCalibraLcdRealZone[i][0];
							pucBufAuxSave+=sizeof(int);
							*(int *)pucBufAuxSave=lcdCal.iCalibraLcdRealZone[i][1];
							pucBufAuxSave+=sizeof(int);

				#else
							sprintf(string_salva,"%i,",lcdCal.iCalibraLcdRealZone[i][0]);
							for (j=0;j<strlen(string_salva);j++)
							   SD_PutChar(string_salva[j]);
						   SD_PutChar(0x0d);
						   SD_PutChar(0x0a);
							sprintf(string_salva,"%i,",lcdCal.iCalibraLcdRealZone[i][1]);
							for (j=0;j<strlen(string_salva);j++)
							   SD_PutChar(string_salva[j]);
						   SD_PutChar(0x0d);
						   SD_PutChar(0x0a);
				#endif
						}	   
				#if 1
					sprintf(string_salva,"WRF %0i\r",(int)(pucBufAuxSave-ucBufAuxSave));
					vSendVdipBinaryString(string_salva,strlen(string_salva));
					vSendVdipBinaryString(ucBufAuxSave,(int)(pucBufAuxSave-ucBufAuxSave));
					vReceiveVdipString();

					strcpy(string_salva,"CLF COEFF.TXT\r");
					vSendVdipBinaryString(string_salva,strlen(string_salva));
					vReceiveVdipString();
				#endif
#endif
	}//void vCalcCoeffsLcdCalibration(void)

	unsigned char ucCalcCrcLcdCalibra(void){
		xdata unsigned char ucCrc;
		xdata unsigned char * xdata pc;
		ucCrc=0;
		pc=(unsigned char xdata *)&(lcdCal.ucCrc);
		// devo saltare il crc, altrimenti ogni volta il risultato cambia!!!
		pc++;
		while(pc<((unsigned char xdata *)&lcdCal)+sizeof(lcdCal))
			ucCrc^=*pc++;
		ucCrc=~ucCrc;
		return ucCrc;
	}
	

	// salva i coefficienti di calibrazione lcd in nand flash
	void vSaveCoeffsLcdCalibration(void){
		// imposto il formato della struttura delle info calibrazione lcd
		lcdCal.uiCalibrationFormatVersion=defActFormatCalibrationVersion;
		lcdCal.ucCrc=ucCalcCrcLcdCalibra();
		if (!ui_write_lcd_calibration_file((char *)&lcdCal,sizeof(lcdCal))){
		}
	}

	// carica i coefficienti di calibrazione di default
	void vLoadCoeffsLcdCalibration(void){
		if (  !ui_read_lcd_calibration_file((char *)&lcdCal,sizeof(lcdCal))
		    ||(lcdCal.uiCalibrationFormatVersion>defActFormatCalibrationVersion)
			||(lcdCal.ucCrc!=ucCalcCrcLcdCalibra())
			){
			memcpy(&lcdCal,&lcdCalDefault,sizeof(lcdCal));
			vSaveCoeffsLcdCalibration();
		}
	}


//#define def_use_new_routines_for_lcd_calibration	
#ifdef def_use_new_routines_for_lcd_calibration	
    void vPrintSmallTesto_Black_ChooseBackground(unsigned int uiRiga,unsigned int uiColonna, unsigned char xdata *pTesto, unsigned int ui_color_background){
        pParamsIcon->uiYpixel=uiRiga;
        pParamsIcon->uiXpixel=uiColonna;
        pParamsIcon->ucFont=0; // font 16x16
        pParamsIcon->pucVettore=pTesto;
        pParamsIcon->ucIconColor = defLCD_Color_Black; 
        pParamsIcon->ucBackgroundColor = ui_color_background;
        vWriteIconElement(pParamsIcon);
    }
#endif

	// calibrazione lcd: procede per colonne da sinistra a destra del display
	void vCalibraLcd(void){
	extern unsigned char ucHasBeenTouchedButton(unsigned char ucNumButton);
	unsigned int xdata i,j,row,col;
	unsigned int xdata precPosY,precPosX,actPosX,actPosY;
	extern void vResetWindows();
#define defNumButton_Calibra_Exit 1
#define defNumButton_Calibra_Info 2
#define defNumSamplesCalibraz 4
	unsigned char xdata ucExit_Calibration;
	unsigned int xdata uiPosizioni_CalibraTouch[2];
	unsigned char xdata ucActNumOfSamples_CalibraTouch;
	//unsigned int xdata ucCalcPosizioni_CalibraTouch;
	unsigned char xdata ucString_Calibra[24];
    unsigned char uc_the_background_color;
// a che distanza dal bordo dello schermo rappresento i crocini pi� esterni???
#define defMarginX_calibration 8
#define defMarginY_calibration 8
		// carico una immagine bianca...
		vCaricaImmagine(10);
    // forza lcd toggle...
        LCD_toggle_bank(); 
        actPosX = 0;
        actPosY = 0;
    
		precPosY=precPosX=0xfff;
		ucExit_Calibration=0;
		for (i=0;i<defNumColLcdCalibration;i++){
			for (j=0;j<defNumRowLcdCalibration;j++){
				// genero su colonne:8 109 210 311
				// genero su righe:8 82 156 230
				uiPosizioni_CalibraTouch[0]=0;
				uiPosizioni_CalibraTouch[1]=0;
				for (ucActNumOfSamples_CalibraTouch=0;ucActNumOfSamples_CalibraTouch<defNumSamplesCalibraz;ucActNumOfSamples_CalibraTouch++){
					ucClearScreen();
                    vSelectFont(enumFontSmall);
                    vSelectTextColor(defLCD_Color_Black);
                    if (ucActNumOfSamples_CalibraTouch&1)
                        uc_the_background_color=defLCD_Color_Yellow;
                    else
                        uc_the_background_color=defLCD_Color_Orange;
                    vSelectBackColor(uc_the_background_color);
					col=i*(defLcdWidthX_pixel-2*defMarginX_calibration)/(defNumColLcdCalibration-1)+defMarginX_calibration;
					row=j*(defLcdWidthY_pixel-2*defMarginY_calibration)/(defNumColLcdCalibration-1)+defMarginY_calibration;
#ifdef def_use_new_routines_for_lcd_calibration	
                    vPrintSmallTesto_Black_ChooseBackground(row,col, stringCalibra,(ucActNumOfSamples_CalibraTouch&1)?defLCD_Color_Yellow:defLCD_Color_Pink);
#else                
					ucPrintTextTouch(&testoCalibraLcd,stringCalibra, row,col,1);
#endif                
					if (row>100){
						mystrcpy(ucString_Calibra,"Exit Calibration",sizeof(ucString_Calibra)-1);
						ucPrintStaticButton(ucString_Calibra,20,10,enumFontSmall,defNumButton_Calibra_Exit,uc_the_background_color);
						sprintf(ucString_Calibra,"touch %i/%i",ucActNumOfSamples_CalibraTouch+1,defNumSamplesCalibraz);
						ucPrintStaticButton(ucString_Calibra,50,10,enumFontSmall,defNumButton_Calibra_Info,uc_the_background_color);
					}
					else{
						mystrcpy(ucString_Calibra,"Exit Calibration",sizeof(ucString_Calibra)-1);
						ucPrintStaticButton(ucString_Calibra,180,10,enumFontSmall,defNumButton_Calibra_Exit,uc_the_background_color);
						sprintf(ucString_Calibra,"touch %i/%i",ucActNumOfSamples_CalibraTouch+1,defNumSamplesCalibraz);
						ucPrintStaticButton(ucString_Calibra,210,10,enumFontSmall,defNumButton_Calibra_Info,uc_the_background_color);
					}
					// inserisco la posizione X centrale del testo
					iCalibraLcdIdeal[i*defNumRowLcdCalibration+j][0]=(testoCalibraLcd.structTouchArea.x1Pixel+testoCalibraLcd.structTouchArea.x2Pixel)/2;
					// poi inserisco la posizione Y centrale del testo
					iCalibraLcdIdeal[i*defNumRowLcdCalibration+j][1]=(testoCalibraLcd.structTouchArea.y1Pixel+testoCalibraLcd.structTouchArea.y2Pixel)/2;
                
					vInitListDisplayElements();
					// adesso faccio rendering del testo, cosi' determino la mappa icone, la mappa
					// bitmap etc
					ucRenderTextOnVideo();	
					vCloseListDisplayElements();
					invio_lista();
					LCD_LoadImageOnBackgroundBank(10);
					applicazione_lista();
					LCD_toggle_bank(); 
                
					LCD_Cursore.ucTouchPressed=0;
					while(!ucExit_Calibration){
						if (ucCtrlTouch()){
							vHandleButtons();
							if (ucHasBeenTouchedButton(defNumButton_Calibra_Exit)){
								ucExit_Calibration=1;
								break;
							}

							actPosX=LCD_Cursore.posX;
							actPosY=LCD_Cursore.posY;
							// filtro doppie pressioni
							//if ((uiPixelDistance(actPosX,precPosX)<20)&&(uiPixelDistance(actPosY,precPosY)<20)){
							//	continue;
							//}
							// la pressione deve essere valida, cio� nei pressi del punto indicato...
							if ( (uiPixelDistance(actPosY,testoCalibraLcd.structTouchArea.y1Pixel)<50)
							   &&(uiPixelDistance(actPosX,testoCalibraLcd.structTouchArea.x1Pixel)<50)
							   ){
								break;
							}
						}	
						
					}// while(!ucExit...
					uiPosizioni_CalibraTouch[0]+=actPosX;
					uiPosizioni_CalibraTouch[1]+=actPosY;
				}
				if (ucExit_Calibration){
					break;
				}
				else{
					actPosX=precPosX=uiPosizioni_CalibraTouch[0]/defNumSamplesCalibraz;
					actPosY=precPosY=uiPosizioni_CalibraTouch[1]/defNumSamplesCalibraz;
					//ucCalcPosizioni_CalibraTouch=0;
					LCD_Cursore.ucTouchPressed=0;
					// inserisco la posizione X di touch
					iCalibraLcdReal[i*defNumRowLcdCalibration+j][0]=precPosX=actPosX;
					// inserisco la posizione Y di touch
					iCalibraLcdReal[i*defNumRowLcdCalibration+j][1]=precPosY=actPosY;
					ucClearScreen();
				}
			}//for (j=0;j<defNumRowLcdCalibration;j++)
			if (ucExit_Calibration)
				break;
		}//for (i=0;i<defNumColLcdCalibration;i++)
		if (!ucExit_Calibration){
			vCalcCoeffsLcdCalibration();
			vSaveCoeffsLcdCalibration();
		}
		ucClearScreen();
		vResetTextSettings();
		vJumpToWindow(enumWinId_SelezionaTaratura);

		// provo ad eliminare il reset delle finestre...
		//vResetWindows();
	}



void vLinearizzaPosLcd(void){
//code float lcdCal.fCoeffsCalibraLcd[(defNumColLcdCalibration-1)*(defNumRowLcdCalibration-1)*4*2]=
//code int lcdCal.iCalibraLcdRealZone[16][2]
	xdata float * xdata pBase;
	xdata float fDelta;
	unsigned char ucIdxZone;
	ucIdxZone=0;
	// ricerca del quadrato che contiene la posizione corrente
	if ((LCD_Cursore.posX>=lcdCal.iCalibraLcdRealZone[5][0])&&(LCD_Cursore.posY>=lcdCal.iCalibraLcdRealZone[5][1])){
		ucIdxZone=0;
	}
	else if ((LCD_Cursore.posX<=lcdCal.iCalibraLcdRealZone[9][0])&&(LCD_Cursore.posY>=lcdCal.iCalibraLcdRealZone[9][1])){
		ucIdxZone=6;
	}
	else if ((LCD_Cursore.posX>=lcdCal.iCalibraLcdRealZone[6][0])&&(LCD_Cursore.posY<=lcdCal.iCalibraLcdRealZone[6][1])){
		ucIdxZone=2;
	}
	else if ((LCD_Cursore.posX<=lcdCal.iCalibraLcdRealZone[10][0])&&(LCD_Cursore.posY<=lcdCal.iCalibraLcdRealZone[10][1])){
		ucIdxZone=8;
	}
	else if (LCD_Cursore.posX>=lcdCal.iCalibraLcdRealZone[5][0]){
		ucIdxZone=1;
	}
	else if (LCD_Cursore.posX<=lcdCal.iCalibraLcdRealZone[10][0]){
		ucIdxZone=7;
	}
	else if (LCD_Cursore.posY>=lcdCal.iCalibraLcdRealZone[9][0]){
		ucIdxZone=3;
	}
	else if (LCD_Cursore.posY<=lcdCal.iCalibraLcdRealZone[6][0]){
		ucIdxZone=5;
	}
	else
		ucIdxZone=4;
		
	// calcolo delle correzioni		
	pBase=&lcdCal.fCoeffsCalibraLcd[ucIdxZone*4*2];
	fDelta=  (pBase[0]*LCD_Cursore.posX)*LCD_Cursore.posY*defScaleFactor*defScaleFactor
			 +pBase[1]*LCD_Cursore.posX*defScaleFactor+pBase[2]*LCD_Cursore.posY*defScaleFactor+pBase[3];
	LCD_Cursore.posX+=fDelta;
	
	pBase+=4;
	fDelta=  (pBase[0]*LCD_Cursore.posX)*LCD_Cursore.posY*defScaleFactor*defScaleFactor
			 +pBase[1]*LCD_Cursore.posX*defScaleFactor+pBase[2]*LCD_Cursore.posY*defScaleFactor+pBase[3];
	LCD_Cursore.posY+=fDelta;
}


void vEnableCalibraLcd(unsigned char ucEnable){
	ucCalibraLcdInCorso=ucEnable;
}

// durata visualizzazione splash screen [ms]
#define defTimeVisualizeSplashScreenMs 3500U

typedef struct _TipoSplashScreen{
	unsigned char ucVisualize;
	unsigned char ucSaveCntInterrupt;
	unsigned int uiTimeElapsedMs;
}TipoSplashScreen;
xdata TipoSplashScreen splashScreen;

void vVisualizeSplashScreen(void){
	splashScreen.ucVisualize=1;
	splashScreen.ucSaveCntInterrupt=ucCntInterrupt;
	splashScreen.uiTimeElapsedMs=0;

}
unsigned char ucVisualizeSplashScreen(void){
	xdata unsigned char ucActCntInterrupt;
	extern void vForceLcdRefresh(void);
	if (!splashScreen.ucVisualize){
		return 0;
	}
	ucActCntInterrupt=ucCntInterrupt;
	if (ucActCntInterrupt>splashScreen.ucSaveCntInterrupt)
		splashScreen.uiTimeElapsedMs+=(ucActCntInterrupt-splashScreen.ucSaveCntInterrupt)*PeriodoIntMs;
	else
		splashScreen.uiTimeElapsedMs+=((256U+ucActCntInterrupt)-splashScreen.ucSaveCntInterrupt)*PeriodoIntMs;
	splashScreen.ucSaveCntInterrupt=ucActCntInterrupt;
	if (splashScreen.uiTimeElapsedMs>defTimeVisualizeSplashScreenMs){
		splashScreen.ucVisualize=0;
		vForceLcdRefresh();
		return 0;
	}
	vForceLcdRefresh();
	return 1;
}//unsigned char ucVisualizeSplashScreen(void)

// aggiornamento dello screen lcd
// puo' essere chiamato col parametro:
// 0 per indicare che � scaduto il timeout di aggiornamento screen
// 1 per indicare che � stato premuto il touch
void vRefreshLcd(enumTipoEventoRefreshLcd ucRefreshLcd_Event){
	xdata unsigned char ucDoRefresh;
	xdata unsigned char ucOkPressed;
	TipoStructTextOnDisplayQueue xdata * xdata p;
	
	// se rinfresco causato da touch, devo comunque ridipingere il video	
	if (ucRefreshLcd_Event==enumRefreshLcd_Touched){
		ucDoRefresh=LCD_Cursore.ucTouchPressed;
	}
	else if(ucRefreshLcd_Event==enumRefreshLcd_Force_Refresh){
		ucDoRefresh=1;
	}
	// se scaduto timeout, devo vedere se ci sono testi lampeggianti
	else{
		ucDoRefresh=0;
	}		
	// se l'immagine � stata modificata, DEVO fare refresh
	if (ucImmagineCambiata()||ucCalibraLcdInCorso)		
		ucDoRefresh=1;
		
   // faccio toggle dello status del cursore
	LCD_Cursore.ucFlashStatus=!LCD_Cursore.ucFlashStatus;
	// punto al primo elemento della coda=testa
	p=&headTextOnDisplayQueue;
	// percorro tutti gli elementi di testo per stamparli a video
	while(1){
		// se coda finita, esco
		if (!p->pNext)
			break;
		// passo al prossimo elemento	
		p=p->pNext;
		// se premuto touch, devo impostare tutte le propriet� di flashing...
		if (LCD_Cursore.ucTouchPressed){
			// in questo test,
			// il testo va in modo flashing se viene toccato
			// normalmente deve essere la routine di callback che gestisce la cosa
			ucOkPressed=ucVerifyTouchOnTesto(p->pTesto,LCD_Cursore.posX, LCD_Cursore.posY);
			p->pTesto->structProperties.ucFlashing=ucOkPressed;
			if (ucOkPressed){
				if (p->pTesto->button.ucType){
					vSignalButtonPressed(p->pTesto->button.ucNum,p->pTesto->ucCharOnTextSelected);
					// se c'� la call back, la chiamo
    				//if (p->pTesto->pFunCallBackTouch){
    				//	p->pTesto->pFunCallBackTouch(eventCallback_Touched,p->pTesto);
    					// no flashing su callback...
    				//	p->pTesto->structProperties.ucFlashing=0;
    				//}
				}
			}
		}	
		// se il testo � in modo flashing
		if (p->pTesto->structProperties.ucFlashing){
			// se un elemento sta facendo flashing, devo rinfrescare il video...
			ucDoRefresh=1;
			if (LCD_Cursore.ucFlashStatus==0){
 				p->pTesto->structProperties.ucBackGroundColor=p->pTesto->structProperties.ucBackGroundColorSave;
		   	// colore di primo piano originale del testo
			 	p->pTesto->structProperties.ucForeGroundColor=p->pTesto->structProperties.ucForeGroundColorSave;
			}
			else{
 				p->pTesto->structProperties.ucForeGroundColor=(~p->pTesto->structProperties.ucForeGroundColorSave)&0x0F;
		   	// colore di primo piano originale del testo
			 	p->pTesto->structProperties.ucBackGroundColor=defLCD_Color_Yellow;
			}
		}
		else{
 				p->pTesto->structProperties.ucBackGroundColor=p->pTesto->structProperties.ucBackGroundColorSave;
		   	// colore di primo piano originale del testo
			 	p->pTesto->structProperties.ucForeGroundColor=p->pTesto->structProperties.ucForeGroundColorSave;
		}
	}
	if (ucDoRefresh){
		if (ucVisualizeSplashScreen()){
			ucClearScreen();
			memset(&testoPrintDefault,0,sizeof(testoPrintDefault));
#if 0
			mystrcpy((char*)stringaTestoScrivi,(char*)ucFirmwareVersion,sizeof(stringaTestoScrivi)-1);
			testoPrintDefault.pucDisplayString=stringaTestoScrivi;
			testoPrintDefault.riga=184;
			testoPrintDefault.colonna=90;
			testoPrintDefault.ucTipoFont=enumFontSmall;
 			testoPrintDefault.structProperties.ucBackGroundColor=(unsigned char)defLCD_Color_Trasparente;//defLCD_Color_LightGreen;
			testoPrintDefault.structProperties.ucForeGroundColor=defLCD_Color_Blue;
			ucTextOnDisplay(&testoPrintDefault);
#else
			mystrcpy((char*)stringaTestoScrivi,(char*)ucFirmwareVersion,sizeof(stringaTestoScrivi)-1);
			// la stringa versione non ci sta tutta su una riga, frammento su due righe
			// la prima riga contiene i primi 18 caratteri, la seconda riga contiene i successivi 20 caratteri
			if (strlen(stringaTestoScrivi)>18){
				// stampa dei primi 18 caratteri
				stringaTestoScrivi[18]=0;
				testoPrintDefault.pucDisplayString=stringaTestoScrivi;
				testoPrintDefault.riga=184-6;
				testoPrintDefault.colonna=90;
				testoPrintDefault.ucTipoFont=enumFontSmall;
 				testoPrintDefault.structProperties.ucBackGroundColor=(unsigned char)defLCD_Color_Trasparente;//defLCD_Color_LightGreen;
				testoPrintDefault.structProperties.ucForeGroundColor=defLCD_Color_Blue;
				ucTextOnDisplay(&testoPrintDefault);

				// stampa dei successivi 20 caratteri
				mystrcpy((char*)stringaTestoScrivi,(char*)ucFirmwareVersion+18,sizeof(stringaTestoScrivi)-1);
				stringaTestoScrivi[20]=0;
				testoPrintDefault.pucDisplayString=stringaTestoScrivi;
				testoPrintDefault.riga=184+4;
				testoPrintDefault.colonna=90;
				testoPrintDefault.ucTipoFont=enumFontSmall;
 				testoPrintDefault.structProperties.ucBackGroundColor=(unsigned char)defLCD_Color_Trasparente;//defLCD_Color_LightGreen;
				testoPrintDefault.structProperties.ucForeGroundColor=defLCD_Color_Blue;
				ucTextOnDisplay(&testoPrintDefault);

			}
			else{
				testoPrintDefault.pucDisplayString=stringaTestoScrivi;
				testoPrintDefault.riga=184;
				testoPrintDefault.colonna=90;
				testoPrintDefault.ucTipoFont=enumFontSmall;
 				testoPrintDefault.structProperties.ucBackGroundColor=(unsigned char)defLCD_Color_Trasparente;//defLCD_Color_LightGreen;
				testoPrintDefault.structProperties.ucForeGroundColor=defLCD_Color_Blue;
				ucTextOnDisplay(&testoPrintDefault);
			}
#endif

			vInitListDisplayElements();
			// adesso faccio rendering del testo, cosi' determino la mappa icone, la mappa
			// bitmap etc
			ucRenderTextOnVideo();	
			vCloseListDisplayElements();
			invio_lista();
			LCD_LoadImageOnBackgroundBank(18);
			applicazione_lista();
			LCD_toggle_bank();
		}
		else{
            // se sto calibrando lcd, aspetto prima di dare refresh allo schermo altrimenti lcd si impalla leggermente
            // if (ucCalibraLcdInCorso){
                // v_wait_microseconds(300000);
            // }
			LCD_Cursore.ucTouchPressed=0;
			vInitListDisplayElements();
			// adesso faccio rendering del testo, cosi' determino la mappa icone, la mappa
			// bitmap etc
			ucRenderTextOnVideo();	
			vCloseListDisplayElements();

			invio_lista();
			LCD_LoadImageOnBackgroundBank(13);
			applicazione_lista();
			LCD_toggle_bank();
			// calibrazione lcd in corso???
			if (ucCalibraLcdInCorso){
				vCalibraLcd();
				ucCalibraLcdInCorso=0;
			}
		}
	}	
}
















