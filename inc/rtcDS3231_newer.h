

#define defNumBytesRTC_DS3231 0x12
#define defi2cAddr_RTC_DS3231 0xD0

extern unsigned char ucBytesRTC_DS3231[defNumBytesRTC_DS3231];
typedef struct _TipoStructRTC{
	unsigned char ucGiorno;
	unsigned char ucMese;
	unsigned char ucAnno;
	unsigned char ucOre;
	unsigned char ucMinuti;
	unsigned char ucSecondi;
	unsigned char ucSecolo;
	unsigned char ucDayOfWeek;
	unsigned char ucErrorTheTimeIsNotValid;
	int iTemperature;
	// parte decimale della temperatura
	unsigned char ucTemperatureDec;
	float fTemperature;
}TipoStructRTC;
extern TipoStructRTC rtc;
// routine per leggere il flag di oscillator stop flag e la temperatura
void vReadStatusTempRTC_DS3231(void);
// routine per resettare oscillator stop flag
void vResetOSF_RTC_DS3231(void);
void vReadTimeBytesRTC_DS3231(void);
void vWriteTimeBytesRTC_DS3231(void);
// inizializzazione realtime clock
void vInit_RtcDS3231(void);