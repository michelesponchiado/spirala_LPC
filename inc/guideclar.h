#ifndef defGuiDeclarH_Included
#define defGuiDeclarH_Included

#include "gui.h"
    
// larghezza/altezza font small enumFont_8x8
#define defWidthPixelFont_8x8 8
#define defHeightPixelFont_8x8 8

// larghezza/altezza font medio enumFont_16x16
#define defWidthPixelFont_16x16 16
#define defHeightPixelFont_16x16 16

// larghezza/altezza font medio grande enumFont_24x24
#define defWidthPixelFont_24x24 24
#define defHeightPixelFont_24x24 24

// larghezza/altezza font big enumFont_32x32
#define defWidthPixelFont_32x32 32
#define defHeightPixelFont_32x32 32

// larghezza/altezza font very big enumFont_40x40
#define defWidthPixelFont_40x40 40
#define defHeightPixelFont_40x40 40

    

// larghezza lcd, in pixel
#define defLcdWidthX_pixel    320
// altezza lcd, in pixel
#define defLcdWidthY_pixel    240

// numero di pixel X a disposizione delle bitmap
#define defLcdBitmapWidthX_pixel    defLcdWidthX_pixel
// numero di pixel Y a disposizione delle bitmap
#define defLcdBitmapWidthY_pixel    defLcdWidthY_pixel
// offset X della posizione della prima bitmap di testo
#define defLcdBitmapOffsetX_pixel   0
// offset Y della posizione della prima bitmap di testo
#define defLcdBitmapOffsetY_pixel   0
// larghezza X di una bitmap [pixel]
#define defBitmapWidthX             16
// larghezza y di una bitmap [pixel]
#define defBitmapWidthY             16
// numero di bitmap in una riga    320/16-->20
#define defLcdBitmapInRow    (defLcdBitmapWidthX_pixel/defBitmapWidthX)
// numero di bitmap in una colonna    240/16-->15
#define defLcdBitmapInCol    (defLcdBitmapWidthY_pixel/defBitmapWidthY)
// numero totale di bitmap nello schermo
#define defNumLcdBitmap (defLcdBitmapInRow*defLcdBitmapInCol)


// numero di pixel X a disposizione delle icone
//#define defLcdIconsWidthX_pixel     288
// numero di pixel Y a disposizione delle icone
//#define defLcdIconsWidthY_pixel     224
// offset X della posizione della prima icona di testo
#define defLcdIconsOffsetX_pixel     16
// offset Y della posizione della prima icona di testo
#define defLcdIconsOffsetY_pixel      8
// larghezza X di una icona [pixel]
#define defIconsWidthX                8
// larghezza Y di una icona [pixel]
#define defIconsWidthY                8
// numero di icone in una riga    288/8-->36
#define defLcdIconsInRow    (defLcdIconsWidthX_pixel/defIconsWidthX)
// numero di icone in una colonna 224/8-->28
//#define defLcdIconsInCol    (defLcdIconsWidthY_pixel/defIconsWidthY)
// numero totale di icone nello schermo
#define defNumLcdIcons (defLcdIconsInRow*defLcdIconsInCol)


// larghezza/altezza font small enumFont_8x16 in numero di icone
// 8/8=1
#define defWidthX_IconFont_8x16 (defWidthPixelFont_8x16/defIconsWidthX)
#define defWidthX_IconFont_Small defWidthX_IconFont_8x16 
// 16/8=2
#define defWidthY_IconFont_8x16 (defHeightPixelFont_8x16/defIconsWidthY)
#define defWidthY_IconFont_Small defWidthY_IconFont_8x16 

// larghezza/altezza font medio enumFont_16x24
#define defWidthPixelFont_16x24 16
#define defHeightPixelFont_16x24 24

// larghezza/altezza font big enumFont_32x32
#define defWidthPixelFont_32x32 32
#define defHeightPixelFont_32x32 32

// larghezza/altezza font mediuml enumFont_16x24 in numero di icone
// 16/8=2
#define defWidthX_IconFont_16x24 (defWidthPixelFont_16x24/defIconsWidthX)
#define defWidthX_IconFont_Medium defWidthX_IconFont_16x24
// 24/8=3
#define defWidthY_IconFont_16x24 (defHeightPixelFont_16x24/defIconsWidthY)
#define defWidthY_IconFont_Medium defWidthY_IconFont_16x24

// Valore massimo restituito dal touch
//#define defMaxValueFromTouch 240
// Fattori di scala per passare da pixel a touch screen in X
// da 320 devo passare a 240, meglio fare da 32 a 24
// quindi uso i due fattori di scala
#define defScaleValuePixelToTouchDiv 24
#define defScaleValuePixelToTouchMul 32


// carattere usato per sostituire caratteri non validi nelle mappe
// viene scelto il carattere 0--> che corrisponde a spazio
#define defaultSubstitutionChar 0

// numero di caratteri del font small
#define defNumCharsSmallFont 128
// numero di caratteri del font medio
#define defNumCharsMediumFont 128

// array che contiene la mappa delle icone
// ogni elemento viene descritto da tre bytes
//extern xdata unsigned char ucIconMap[defNumLcdIcons][3];
// array che contiene la mappa delle bitmap
// ogni elemento viene descritto da due bytes
extern xdata unsigned char ucBitmapMap[defNumLcdBitmap][2];



#endif //#ifndef defGuiDeclarH_Included

