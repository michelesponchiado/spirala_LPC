#include "LPC23xx.h"                       /* LPC23xx/24xx definitions */
#include <stdio.h>
#include <stdlib.h>
#include "type.h"
#include "irq.h"
#include "mci.h"
#include "dma.h"
#include "string.h"
#include "target.h"
#include "rtcDS3231.h"
#include "fat_time.h"

DWORD get_fattime (void){
	DWORD result;
    extern void vReadTimeBytesRTC_DS3231(void);
//#warning disabled rtc for sd testing, i2c wont work, I dunno why
    vReadTimeBytesRTC_DS3231();

	result = (
		((( rtc.ucSecondi/2 )   &0x1F) << 0UL  ) |
		((( rtc.ucMinuti )      &0x3F) << 5UL  ) |
		((( rtc.ucOre )         &0x1F) << 11UL ) |
		((( rtc.ucGiorno )      &0x1F) << 16UL ) |
		((( rtc.ucMese )        &0x0F) << 21UL ) |
		((( rtc.ucAnno+2000-1980 )   &0x7F) << 25UL )  // watch! year is 1980-based on fat representation, while rtc uses 2'000 as base...
	);

	return result;
}//DWORD get_fattime (void)
