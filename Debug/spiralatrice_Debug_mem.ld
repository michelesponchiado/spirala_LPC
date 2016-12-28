/*
 * GENERATED FILE - DO NOT EDIT
 * (c) Code Red Technologies Ltd, 2008-2016
 * (c) NXP Semiconductors 2013-2016
 * Linker script memory definitions
 * Created from LinkMemoryTemplate
 * By LPCXpresso v7.8.0 [Build 426] [2015-05-28]  on Wed Dec 28 18:08:39 CET 2016)
*/

MEMORY
{
  /* Define each memory region */
  MFlash512 (rx) : ORIGIN = 0x0, LENGTH = 0x7e000 /* 504K bytes */
  Ram64 (rwx) : ORIGIN = 0x40000000, LENGTH = 0x10000 /* 64K bytes */
  RAM_ETH_16 (rwx) : ORIGIN = 0x7fe00000, LENGTH = 0x4000 /* 16K bytes */
  RAM_USB_16 (rwx) : ORIGIN = 0x7fd00000, LENGTH = 0x4000 /* 16K bytes */
  RAM_RTC_2 (rwx) : ORIGIN = 0xe0084000, LENGTH = 0x800 /* 2K bytes */


}
  /* Define a symbol for the top of each memory region */
  __top_MFlash512 = 0x0 + 0x7e000;
  __top_Ram64 = 0x40000000 + 0x10000;
  __top_RAM_ETH_16 = 0x7fe00000 + 0x4000;
  __top_RAM_USB_16 = 0x7fd00000 + 0x4000;
  __top_RAM_RTC_2 = 0xe0084000 + 0x800;



