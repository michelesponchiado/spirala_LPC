/*------------------------------------------------------------------------------
Title: upsd_init
Date: August, 2004

DK3400 Initialization Code
08/2004 Ver 2.0 - Initial Version

Copyright (c) 2004 STMicroelectronics Inc.

This code is used to initialize the uPSD3400 MCU.  The file "upsd3400_hardware.h"
is used to specify the hardware configurable items such as crystal speed, 
memory addresses, etc.  The user MUST EDIT the "upsd3400_hardware.h" to match 
their physical hardware configuration AND as also defined in PSDsoft.
<
LIMITATION OF LIABILITY:   NEITHER STMicroelectronics NOR ITS VENDORS OR 
AGENTS SHALL BE LIABLE FOR ANY LOSS OF PROFITS, LOSS OF USE, LOSS OF DATA,
INTERRUPTION OF BUSINESS, NOR FOR INDIRECT, SPECIAL, INCIDENTAL OR
CONSEQUENTIAL DAMAGES OF ANY KIND WHETHER UNDER THIS AGREEMENT OR
OTHERWISE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
------------------------------------------------------------------------------*/

#pragma nodebug
#ifndef ARM9platform

    #include "upsd3400.h"
    #include "upsd3400_hardware.h"


    void upsd_Init(void)
    {

    // Set up BUSCON control register based on MCU voltage and Frequency of Oscilator
    // Turbo uPSD specific initialization 
    // BUSCON.7 = 1, Prefetch Queue is Enabled 
    // BUSCON.6 = 1, Branch Cache is Enabled                                              
    // BUSCON.5, BUSCON.4 =  Xdata Write bus cycles (0 to 3 wait cycles)
    // BUSCON.3, BUSCON.2 =  Xdata Read bus cycles (0 to 3 wait cycles)
    // BUSCON.1, BUSCON.0 =  Code Fetch bus cycles (0 to 3 wait cycles)

      #if (uPSD_5V == 0)
          #if (FREQ_OSC > 24000)
             BUSCON = 0xD6;  // 3V MCU > 24MHz
          #else
             BUSCON = 0xC0;  // 3V MCU <= 24MHz
          #endif
      #elif (uPSD_5V == 1)
          #if (FREQ_OSC > 24000)
             //BUSCON = 0xC1;  // 5V MCU > 24MHz
             BUSCON = 0xC1;  // 5V MCU > 24MHz  7clocks in write and read
          #else
             BUSCON = 0xC0;  // 5V MCU <= 24MHz
          #endif
      #else
             BUSCON = 0xFF;  // Default to slowest bus cycles for unknown values  
      #endif

      // Other uPSD init items...

      IE |= 0xC0;   // Enable Global and Debug Interrupts...

      return;
    } 
#else
    void upsd_Init(void)
    {
    }
#endif
