#ifndef defFpga_nxp_h_included
#define defFpga_nxp_h_included

// indirizzo base fpga
#define defNXP_AddressBaseFpga ((volatile unsigned char *)0x80000000)

// input digitali
#define InpDig				(*(defNXP_AddressBaseFpga+0x01)|((*(defNXP_AddressBaseFpga+0x00))<<8))

// output digitali
#define Outputs_Low			(*(defNXP_AddressBaseFpga+0x02))
#define Outputs_Hi			(*(defNXP_AddressBaseFpga+0x03))
#define OutDig				Outputs_Low

// indirizzi del display...
#define spi_lcd_register	(*(defNXP_AddressBaseFpga+0x04))
#define spi_lcd_busy		(*(defNXP_AddressBaseFpga+0x05))
#define spi_lcd_reset_low	(*(defNXP_AddressBaseFpga+0x06))
#define spi_lcd_int_low		(*(defNXP_AddressBaseFpga+0x07))

// a/d converter
#define ucLowByteAD7327			(*(defNXP_AddressBaseFpga+0x08))
#define ucHiByteAD7327			(*(defNXP_AddressBaseFpga+0x09))
#define ucReadBusy_WriteLatch   (*(defNXP_AddressBaseFpga+0x0a))

// debug registers
#define fpga_dbg_reg_lo		(*(defNXP_AddressBaseFpga+0x0B))
#define fpga_dbg_reg_hi		(*(defNXP_AddressBaseFpga+0x0C))

// indirizzi nand flash...
#define nand_data_reg		(*(defNXP_AddressBaseFpga+0x10))
#define nand_ctrl_reg		(*(defNXP_AddressBaseFpga+0x11))
#define nand_read_busy		(*(defNXP_AddressBaseFpga+0x12))

// counter 1 counts and latch command
#define ucCounter1_low			(*(defNXP_AddressBaseFpga+0x18))
#define ucCounter1_hi			(*(defNXP_AddressBaseFpga+0x19))
#define ucCounter1_latch_toggle (*(defNXP_AddressBaseFpga+0x1a))

// counter 2 trigger on c channel enable, counts, num of trigs found
#define uc_Counter2_TrigEnable			(*(defNXP_AddressBaseFpga+0x1b))
#define uc_Counter2_c_chan_latched_low  (*(defNXP_AddressBaseFpga+0x1c))
#define uc_Counter2_c_chan_latched_hi	(*(defNXP_AddressBaseFpga+0x1d))
#define uc_Counter2_c_chan_trig_num_of  (*(defNXP_AddressBaseFpga+0x1e))

//vdip
#define ucByteTxVdip_OldData	(*(defNXP_AddressBaseFpga+0x20))
#define ucByteRxVdip			(*(defNXP_AddressBaseFpga+0x21))
#define ucByteStatusVdip		(*(defNXP_AddressBaseFpga+0x22))
#define ucBusySpiVdip			(*(defNXP_AddressBaseFpga+0x23))
#define ucReset_Low_Vdip		(*(defNXP_AddressBaseFpga+0x24))
#define ucDataReq_Low_Vdip		(*(defNXP_AddressBaseFpga+0x25))

// enable external ram access through fpga    
#define ucEnable_ext_ram_0xa5   (*(defNXP_AddressBaseFpga+0x26))

// invert wheel 1 rotation when you write 0xA5 @ address 39=0x27  
#define uc_wheel_one_rotation_invert_0xA5   (*(defNXP_AddressBaseFpga+0x27))

// counter 2 counts and control for latching
#define ucCounter2_low			(*(defNXP_AddressBaseFpga+0x28))
#define ucCounter2_hi			(*(defNXP_AddressBaseFpga+0x29))
#define ucCounter2_latch_toggle (*(defNXP_AddressBaseFpga+0x2a))

// counter 3 counts and control for latching
#define ucCounter3_low          (*(defNXP_AddressBaseFpga+0x2b))
#define ucCounter3_hi           (*(defNXP_AddressBaseFpga+0x2c))
#define ucCounter3_latch_toggle (*(defNXP_AddressBaseFpga+0x2d))

// counter 3 trigger on c channel enable, counts, num of trigs found
#define uc_Counter3_TrigEnable			(*(defNXP_AddressBaseFpga+0x2e))
#define uc_Counter3_c_chan_latched_low  (*(defNXP_AddressBaseFpga+0x2f))
#define uc_Counter3_c_chan_latched_hi	(*(defNXP_AddressBaseFpga+0x30))
#define uc_Counter3_c_chan_trig_num_of  (*(defNXP_AddressBaseFpga+0x31))

// Anybus
#define ucUartAnybus_TxRx               (*(defNXP_AddressBaseFpga+0x32))
#define ucUartAnybus_Status             (*(defNXP_AddressBaseFpga+0x33))
#define ucUartAnybus_LatchInfos         (*(defNXP_AddressBaseFpga+0x34))
#define ucUartAnybus_NumBytesRx_Low     (*(defNXP_AddressBaseFpga+0x35))
#define ucUartAnybus_NumBytesRx_Hi      (*(defNXP_AddressBaseFpga+0x36))
#define ucUartAnybus_NumBytesCanTx_Low  (*(defNXP_AddressBaseFpga+0x37))
#define ucUartAnybus_NumBytesCanTx_Hi   (*(defNXP_AddressBaseFpga+0x38))
#define ucUartAnybus_NumBytesTotalTx    (*(defNXP_AddressBaseFpga+0x39))
#define ucUartAnybus_NumBytesTotalRx    (*(defNXP_AddressBaseFpga+0x3A))

// in queste locazioni di memoria posso trovare i miei canali a/d, latched, corrispondenti alla media presa su circa 3-4ms
// di letture circolari dei vari canali fatte ogni 2us circa
// in questo modo il rumore dovrebbe scendere di circa sqrt(128) volte, cioè 11 volte,
// quindi guadagniamo circa 3.5bit
// se ad es il rumore di lettura è di 30step (come nel caso del sensore di taglio), così dovremmo passare
// a circa 3 step, quindi solo gli ultimi due bit dovrebbero ballare...
#define ucChannel_AutoAD_0_low (*(defNXP_AddressBaseFpga+0x40))
#define ucChannel_AutoAD_0_hi  (*(defNXP_AddressBaseFpga+0x41))
#define ucChannel_AutoAD_1_low (*(defNXP_AddressBaseFpga+0x42))
#define ucChannel_AutoAD_1_hi  (*(defNXP_AddressBaseFpga+0x43))

#define ucChannel_AutoAD_2_low (*(defNXP_AddressBaseFpga+0x44))
#define ucChannel_AutoAD_2_hi  (*(defNXP_AddressBaseFpga+0x45))

#define ucChannel_AutoAD_3_low (*(defNXP_AddressBaseFpga+0x46))
#define ucChannel_AutoAD_3_hi  (*(defNXP_AddressBaseFpga+0x47))

#define ucChannel_AutoAD_4_low (*(defNXP_AddressBaseFpga+0x48))
#define ucChannel_AutoAD_4_hi  (*(defNXP_AddressBaseFpga+0x49))

#define ucChannel_AutoAD_5_low (*(defNXP_AddressBaseFpga+0x4a))
#define ucChannel_AutoAD_5_hi  (*(defNXP_AddressBaseFpga+0x4b))

#define ucChannel_AutoAD_6_low (*(defNXP_AddressBaseFpga+0x4c))
#define ucChannel_AutoAD_6_hi  (*(defNXP_AddressBaseFpga+0x4d))

#define ucChannel_AutoAD_7_low (*(defNXP_AddressBaseFpga+0x4e))
#define ucChannel_AutoAD_7_hi  (*(defNXP_AddressBaseFpga+0x4f))

// variabile fpga che permette di impostare acquisizione automatica canali convertitore a/d
#define ucEnableAutoAD    			(*(defNXP_AddressBaseFpga+0x50))
// variabile fpga che permette di fare il latch delle ultime conversioni disponibili...
// devo: impostare latch=0, attendere che rileggendo vada a zero, impostare ad 1 ad attendere che vada ad 1
#define ucLatchAutoAD				(*(defNXP_AddressBaseFpga+0x51))
// variabile fpga che CONTIENE IL NUMERO DI LETTURE DAL CONVERTITORE A/D
// su 8 bit, ricircola tra 0 e 255, usato nel modo autoAD
#define ucNumOfReadAutoAD			(*(defNXP_AddressBaseFpga+0x52))
// variabile fpga che contiene gli 8 bit bassi restituiti da ad7327 nell'ultima conversione in modo automatico
#define ucLastChanAutoAD_low		(*(defNXP_AddressBaseFpga+0x53))
// variabile fpga che contiene gli 8 bit bassi restituiti da ad7327 nell'ultima conversione in modo automatico
#define ucLastChanAutoAD_hi			(*(defNXP_AddressBaseFpga+0x54))
// variabile fpga che contiene gli 8 bit bassi restituiti da ad7327 nel filtro corrente
#define ucLastChanFltAutoAD_low		(*(defNXP_AddressBaseFpga+0x55))
// variabile fpga che contiene gli 8 bit bassi restituiti da ad7327 nel filtro corrente
#define ucLastChanFltAutoAD_hi		(*(defNXP_AddressBaseFpga+0x56))
// variabile fpga che contiene gli 8 bit bassi restituiti da ad7327 in ram
#define ucLastChanRamAutoAD_low		(*(defNXP_AddressBaseFpga+0x57))
// variabile fpga che contiene gli 8 bit bassi restituiti da ad7327 in ram
#define ucLastChanRamAutoAD_hi		(*(defNXP_AddressBaseFpga+0x58))
#define ucLastChanAccum6AutoAD_low	(*(defNXP_AddressBaseFpga+0x59))
#define ucLastChanAccum6AutoAD_hi	(*(defNXP_AddressBaseFpga+0x5a))
#define ucDebugAutoAD_0				(*(defNXP_AddressBaseFpga+0x5b))
#define ucAutoAD_NumOfLatch 		(*(defNXP_AddressBaseFpga+0x5c))

// fpga version
#define ucFpgaFirmwareVersion_Low	(*(defNXP_AddressBaseFpga+59))
#define ucFpgaFirmwareVersion_Hi	(*(defNXP_AddressBaseFpga+60))


// start address of battery ram...
#define defNXP_BatteryRam_StartAddress ((volatile unsigned int *)0xE0084000)

void v_wait_microseconds(unsigned long ul_wait_microseconds);

#endif //#ifndef defFpga_nxp_h_included
