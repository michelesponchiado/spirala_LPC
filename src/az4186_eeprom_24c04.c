/*****************************************************************************
 *   az4186_eeprom_24c04.c:  handles i2c eeprom 24c04
 *
 *   Copyright(C) 2012,CSM GROUP
 *   All rights reserved.
 *
 *   History
 *   2012.11.23  ver 1.00    Preliminary version, first Release
 *
******************************************************************************/
#include "LPC23xx.h"                        // LPC23xx/24xx definitions
#include "type.h"
#include "target.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "fpga_nxp.h"
#include "az4186_emc.h"
#include "irq.h"
#include "i2c.h"
#include "i2c_master.h"
#include "spicost.h"
#include "spiext.h"
#include "xxtea.h"

//#include "az4186_temperature_compensation.h"
// se 24c04 datasheet
#define defi2cAddr_24c04 0xa0
// 24c04 is a 4kbit wide eeprom
#define def_24c04_eeprom_size_kbit 4
#define def_24c04_eeprom_size_bit (def_24c04_eeprom_size_kbit*1024)
#define def_24c04_eeprom_size_bytes (def_24c04_eeprom_size_bit/8)

extern void v_signal_accessing_eeprom_i2c(unsigned char uc_accessing_eeprom);



extern int i2c_engine_started;

// data types we can store in eeprom
// actually only the box s/n can be stored
typedef enum{
	enum_eeprom_data_box_sn=0,
	enum_eeprom_data_spacer_type,
	enum_eeprom_data_alarm_lamp,
	enum_eeprom_data_fifth_range,
	enum_eeprom_data_temperature_compensation,
	enum_eeprom_data_spindle_grinding,
	enum_eeprom_data_numof
}enum_eeprom_data;
// this structure has all the info we need to store a datum in eeprom
typedef struct _TipoStructEepromDataInfo{
	enum_eeprom_data the_enum; 	// data type
	unsigned char uc_size;		// data size [bytes]
	unsigned char * p;			// pointer to the variable containing the value
}TipoStructEepromDataInfo;

// the box s/n
extern unsigned long ulBoxSerialNumber;

// the table with the data to save in eeprom...
const TipoStructEepromDataInfo eeprom_data_info[enum_eeprom_data_numof]={
     {enum_eeprom_data_box_sn,                  sizeof(ulBoxSerialNumber),                          (unsigned char *)&ulBoxSerialNumber}
    ,{enum_eeprom_data_spacer_type,             sizeof(macParms.uc_distanziatore_type),             (unsigned char *)&macParms.uc_distanziatore_type}
    ,{enum_eeprom_data_alarm_lamp,              sizeof(macParms.ucAbilitaLampadaAllarme),           (unsigned char *)&macParms.ucAbilitaLampadaAllarme}
    ,{enum_eeprom_data_fifth_range,             sizeof(macParms.ucAbilitaQuintaPortata),            (unsigned char *)&macParms.ucAbilitaQuintaPortata}
    ,{enum_eeprom_data_temperature_compensation,sizeof(macParms.ucAbilitaCompensazioneTemperatura), (unsigned char *)&macParms.ucAbilitaCompensazioneTemperatura}
    ,{enum_eeprom_data_spindle_grinding		,sizeof(macParms.uc_enable_spindle_grinding), (unsigned char *)&macParms.uc_enable_spindle_grinding}
};

// each datum is stored in eeprom like this:
// * an header containing:
// 		the datum id [1byte], 0xFF means free location
// 		the datum size in bytes [1byte], 
// 		the datum status [1byte], 0xFF means not valid for read 
// * the datum itself [x bytes]
// * the datum crc, from id (included) to crc (excluded), [1 byte] 
// 
// please note that in eeprom there are 2 consecutives copies of each datum
//
typedef struct _TipoStructEepromHeader{
	unsigned char uc_id;
	unsigned char uc_size;
	unsigned char uc_status;
}TipoStructEepromHeader;

// but anyway it still would exists also a backup copy for the datum

// the data structure of the eeprom would be:
//  0     eeprom signature: "az4186ok" 8 bytes
//  8     box serial number copy #1: 8 bytes=2header+4data+2crc
// 16     box serial number copy #2: 8 bytes=2header+4data+2crc
typedef struct _TipoStructEepromQueueElement{
	enum_eeprom_data the_eeprom_datum;
	unsigned char uc_datum[16];
}TipoStructEepromQueueElement;

// the eeprom handling stati
typedef enum{
	enum_eeprom_status_idle=0,
	enum_eeprom_status_init,
	enum_eeprom_status_initialized,
	enum_eeprom_status_unrecoverable_error,
	enum_eeprom_status_ok,
	enum_eeprom_status_numof
}enum_eeprom_status;

// define the eeprom command queue size
#define def_num_bit_size_eeprom_queue 2
#define def_size_eeprom_queue (1<<def_num_bit_size_eeprom_queue)
#define def_mask_size_eeprom_queue (def_size_eeprom_queue-1)
#define def_eeprom_signature_ok "az4186ok"

// the structure to handle the eeprom
typedef struct _TipoStructEepromQueue{
	TipoStructEepromQueueElement queue[def_size_eeprom_queue];
	unsigned short us_num_read;
	unsigned short us_idx_read;
	unsigned short us_num_write;
	unsigned short us_idx_write;
	enum_eeprom_status status;
	unsigned char i2c_master_xmit_buf[16];
	unsigned char signature[8+1];
	unsigned int errors;
}TipoStructEepromQueue;

// the variable to handle the eeprom
TipoStructEepromQueue eeprom_queue;

// returns the number of elements im eeprom queue
unsigned int ui_num_element_in_eeprom_queue(void){
	unsigned int ui_num_elem;
	if (eeprom_queue.us_num_write>=eeprom_queue.us_num_read){
		ui_num_elem=eeprom_queue.us_num_write-eeprom_queue.us_num_read;
	}
	else{
		ui_num_elem=(1<<(8*sizeof(eeprom_queue.us_num_write)));
		ui_num_elem+=eeprom_queue.us_num_write;
		ui_num_elem-=eeprom_queue.us_num_read;
	}
	return ui_num_elem;
}

// returns 1 if eeprom queue is full, else returns 0
unsigned int ui_is_full_eeprom_queue(void){
	unsigned int ui_the_queue_is_full;
	ui_the_queue_is_full=0;
	if (ui_num_element_in_eeprom_queue()>=sizeof(eeprom_queue.queue)/sizeof(eeprom_queue.queue[0])){
		ui_the_queue_is_full=1;
	}
	return ui_the_queue_is_full;
}

// returns 1 if eeprom queue is empty, else returns 0
unsigned int ui_is_empty_eeprom_queue(void){
	unsigned int ui_the_queue_is_empty;
	ui_the_queue_is_empty=0;
	if (ui_num_element_in_eeprom_queue()==0){
		ui_the_queue_is_empty=1;
	}
	return ui_the_queue_is_empty;
}

// this routine saves the datum in eeprom, it really posts the datum in a queue, then it will be saved..
unsigned int ui_save_eeprom(enum_eeprom_data the_eeprom_datum){
	TipoStructEepromQueueElement *p;
	unsigned int ui_idx;
	unsigned int ui_size_available,ui_size_needed;
	if (the_eeprom_datum>=enum_eeprom_data_numof)
		return 0;
	if (ui_is_full_eeprom_queue())
		return 0;
	ui_idx=eeprom_queue.us_idx_write;
	p=&eeprom_queue.queue[ui_idx];
	p->the_eeprom_datum=the_eeprom_datum;
	ui_size_needed=eeprom_data_info[the_eeprom_datum].uc_size;
	ui_size_available=sizeof(p->uc_datum);
	if (ui_size_needed>ui_size_available)
		return 0;
	memcpy(p->uc_datum,eeprom_data_info[the_eeprom_datum].p,ui_size_needed);
	eeprom_queue.us_idx_write=(eeprom_queue.us_idx_write+1)&def_mask_size_eeprom_queue;
	eeprom_queue.us_num_write++;
    return 1;
}

// set the current address for eeprom read
void v_eeprom_set_address(unsigned int ui_address){
// just do a dummy write at the specified address
	unsigned int ui_block_number;
	unsigned char uc_address;

	if (ucWaitI2Cbus(0)){
		i2c_engine_started=1;
        v_signal_accessing_eeprom_i2c(1);
			ui_block_number=(ui_address>=256)?2:0;
			uc_address=ui_address&0xff;		
			eeprom_queue.i2c_master_xmit_buf[0]=uc_address;
			i2c_Master_Xmit(defi2cAddr_24c04|ui_block_number,eeprom_queue.i2c_master_xmit_buf,1);
        v_signal_accessing_eeprom_i2c(0);
		i2c_engine_started=0;

		vSignalI2Cbus();
	}
}

// use this routine to read a series of bytes from eeprom
void v_eeprom_read_bytes(unsigned int ui_address,unsigned char uc_num_bytes,unsigned char *puc_bytes_read){
	// check if valid address
	if (ui_address>=def_24c04_eeprom_size_bytes){
		return;
	}
	v_eeprom_set_address(ui_address);
// read the bytes
	if (ucWaitI2Cbus(0)){
		i2c_engine_started=1;
			i2c_Master_Rcv(defi2cAddr_24c04,puc_bytes_read,uc_num_bytes);
		i2c_engine_started=0;

		vSignalI2Cbus();
	}
}

// use this routine to write a series of bytes from eeprom
void v_eeprom_write_bytes(unsigned int ui_address,unsigned char uc_num_bytes,unsigned char *puc_bytes_to_write){
	unsigned int ui_block_number;
	unsigned char uc_address;
	while(uc_num_bytes--){
		// check if valid address
		if (ui_address>=def_24c04_eeprom_size_bytes){
			break;
		}
		if (ucWaitI2Cbus(0)){
			i2c_engine_started=1;
            v_signal_accessing_eeprom_i2c(1);
				// the block number is the b1 bit
				ui_block_number=(ui_address>=256)?2:0;
				uc_address=ui_address&0xff;
				ui_address++;
				eeprom_queue.i2c_master_xmit_buf[0]=uc_address;
				eeprom_queue.i2c_master_xmit_buf[1]=*puc_bytes_to_write++;
				i2c_Master_Xmit(defi2cAddr_24c04|ui_block_number,eeprom_queue.i2c_master_xmit_buf,2);
            v_signal_accessing_eeprom_i2c(0);
			i2c_engine_started=0;

			vSignalI2Cbus();
			//wait at least 20ms, or wait until a dummy write returns an ack, I prefer to wait... it is more simple...
			{
				TipoStruct_timeout timeout;
				timeout_init(&timeout);
				while(1){
					if (uc_timeout_active(&timeout, 25)){
						break;
					}
				}

			}
		}
	}
}

// returns 1 if the eeprom signature is valid, else returns 0
unsigned int ui_eeprom_verify_signature(void){
	memset(eeprom_queue.signature,0,sizeof(eeprom_queue.signature));
	v_eeprom_read_bytes(0,sizeof(eeprom_queue.signature)-1,&eeprom_queue.signature[0]);
	if (strcmp((char*)eeprom_queue.signature,def_eeprom_signature_ok)){
		return 0;
	}
	return 1;
}

// write the eeprom signature
void v_eeprom_write_signature(void){
    memset(eeprom_queue.signature,0,sizeof(eeprom_queue.signature));
	strncpy((char*)eeprom_queue.signature,def_eeprom_signature_ok,sizeof(eeprom_queue.signature));
	v_eeprom_write_bytes(0,sizeof(eeprom_queue.signature)-1,&eeprom_queue.signature[0]);
}

// initialize eeprom module...
void v_init_eeprom(void){
	memset(&eeprom_queue,0,sizeof(eeprom_queue));
}

// this enum identifies the first and the second copy of a datum in eeprom
typedef enum{
	enum_select_eeprom_datum_copy_first=0,
	enum_select_eeprom_datum_copy_second,
	enum_select_eeprom_datum_copy_numof
}enum_select_eeprom_datum_copy;

// erase an eeprom datum
// Input:
// * the enum which identifies the datum to erase
// * the enum identifying which copy of the datun should be erased: the first or the second one?
void v_erase_eeprom_datum(enum_eeprom_data the_eeprom_datum,enum_select_eeprom_datum_copy select_eeprom_datum_copy){
	TipoStructEepromHeader header;
	enum_eeprom_data idx_datum;
	unsigned int i;
	unsigned int ui_address;
	unsigned char uc_crc,uc_bytes[16];
	if (the_eeprom_datum>=enum_eeprom_data_numof){
		return;	
	}
	header.uc_id=the_eeprom_datum;
	header.uc_size=eeprom_data_info[the_eeprom_datum].uc_size;
	header.uc_status=0xFF;
	idx_datum=0;
	ui_address=8;
	while(idx_datum<the_eeprom_datum){
		ui_address+=(4+eeprom_data_info[idx_datum].uc_size)*2;
		idx_datum++;
	}
	if (select_eeprom_datum_copy==enum_select_eeprom_datum_copy_second){
		ui_address+=(4+eeprom_data_info[the_eeprom_datum].uc_size);
	}
	uc_crc=0;
	uc_crc^=header.uc_id;
	uc_crc^=header.uc_size;
	uc_crc^=header.uc_status;
	v_eeprom_write_bytes(ui_address,3,(unsigned char *)&header);
	ui_address+=3;
	i=0;
	while((i<sizeof(uc_bytes))&&(i<header.uc_size)){
		uc_bytes[i]=0xff;
		uc_crc^=uc_bytes[i];
		i++;
	}
	v_eeprom_write_bytes(ui_address,i,(unsigned char *)&uc_bytes);
	ui_address+=i;
	uc_crc^=0xFF;	
	v_eeprom_write_bytes(ui_address,1,(unsigned char *)&uc_crc);
}

// save an eeprom datum
// Input:
// * the enum which identifies the datum to erase
// * the pointer to the source of the datum
// * the enum identifying which copy of the datun should be erased: the first or the second one?
void v_save_eeprom_datum(enum_eeprom_data the_eeprom_datum,unsigned char *puc_value,enum_select_eeprom_datum_copy select_eeprom_datum_copy){
	TipoStructEepromHeader header;
	enum_eeprom_data idx_datum;
	unsigned int i;
	unsigned int ui_address;
	unsigned char uc_crc,uc_bytes[16];
	if (the_eeprom_datum>=enum_eeprom_data_numof)
		return;	
	header.uc_id=the_eeprom_datum;
	header.uc_size=eeprom_data_info[the_eeprom_datum].uc_size;
	header.uc_status=0xA5;
	idx_datum=0;
	ui_address=8;
	while(idx_datum<the_eeprom_datum){
		ui_address+=(4+eeprom_data_info[idx_datum].uc_size)*2;
		idx_datum++;
	}
	if (select_eeprom_datum_copy==enum_select_eeprom_datum_copy_second){
		ui_address+=(4+eeprom_data_info[the_eeprom_datum].uc_size);
	}
	uc_crc=0;
	uc_crc^=header.uc_id;
	uc_crc^=header.uc_size;
	uc_crc^=header.uc_status;
	v_eeprom_write_bytes(ui_address,3,(unsigned char *)&header);
	ui_address+=3;
	i=0;
	while((i<sizeof(uc_bytes))&&(i<header.uc_size)){
		uc_bytes[i]=*puc_value++;
		uc_crc^=uc_bytes[i];
		i++;
	}
	v_eeprom_write_bytes(ui_address,i,(unsigned char *)&uc_bytes);
	ui_address+=i;
	uc_crc^=0xFF;
	v_eeprom_write_bytes(ui_address,1,(unsigned char *)&uc_crc);
}

// read an eeprom datum
// Input:
// * the enum which identifies the datum to erase
// * the pointer to the destination of read
// * the enum identifying which copy of the datun should be erased: the first or the second one?
unsigned int ui_read_eeprom_datum(enum_eeprom_data the_eeprom_datum,unsigned char *puc_value,enum_select_eeprom_datum_copy select_eeprom_datum_copy){
	TipoStructEepromHeader *pheader;
	enum_eeprom_data idx_datum;
	unsigned int i;
	unsigned int ui_address;
	unsigned char uc_crc,uc_bytes[32];
	if (the_eeprom_datum>=enum_eeprom_data_numof)
		return 0;	
	idx_datum=0;
	ui_address=8;
	while(idx_datum<the_eeprom_datum){
		ui_address+=(4+eeprom_data_info[idx_datum].uc_size)*2;
		idx_datum++;
	}
	if (select_eeprom_datum_copy==enum_select_eeprom_datum_copy_second){
		ui_address+=(4+eeprom_data_info[the_eeprom_datum].uc_size);
	}
	v_eeprom_read_bytes(ui_address,4+eeprom_data_info[the_eeprom_datum].uc_size,(unsigned char *)&uc_bytes);
	uc_crc=0;
	i=0;
	while(i<3+eeprom_data_info[the_eeprom_datum].uc_size){
		uc_crc^=uc_bytes[i];
		i++;
	}
	uc_crc^=0xff;
	if (uc_crc==uc_bytes[i]){
        pheader=(TipoStructEepromHeader *)uc_bytes;
        // verify if id and status are ok!
        if (  (pheader->uc_id!=idx_datum)
            ||(pheader->uc_status==0xff)
            ||(pheader->uc_size!=eeprom_data_info[the_eeprom_datum].uc_size)
           ){
            return 0;
        }
		memcpy(puc_value,uc_bytes+3,eeprom_data_info[the_eeprom_datum].uc_size);
		return 1;
	}
	else{
		return 0;
	}
}

// this function update in eeprom a datum, 
unsigned int ui_update_eeprom_datum(enum_eeprom_data the_eeprom_datum,unsigned char *puc_value){
	unsigned char uc_value[16];
	// if the first copy isn't initialized, do initialization, then erase the second copy
	if (!ui_read_eeprom_datum(the_eeprom_datum,uc_value,enum_select_eeprom_datum_copy_first)){
		v_save_eeprom_datum(the_eeprom_datum,puc_value,enum_select_eeprom_datum_copy_first);
		v_erase_eeprom_datum(the_eeprom_datum,enum_select_eeprom_datum_copy_second);
	}
	else{
		// if second copy isn't initialized, do initialization then erase the first copy
		if (!ui_read_eeprom_datum(the_eeprom_datum,uc_value,enum_select_eeprom_datum_copy_second)){
			v_save_eeprom_datum(the_eeprom_datum,puc_value,enum_select_eeprom_datum_copy_second);
			v_erase_eeprom_datum(the_eeprom_datum,enum_select_eeprom_datum_copy_first);
		}
		// if second copy is initialized, save on first copy
		else{
			v_save_eeprom_datum(the_eeprom_datum,puc_value,enum_select_eeprom_datum_copy_first);
		}
	}
    return 1;
}

// this function load from eeprom a datum, 
unsigned int ui_load_from_eeprom_datum(enum_eeprom_data the_eeprom_datum){
	unsigned char uc_value[16];
	// if the first copy isn't initialized, do initialization, then erase the second copy
	if (!ui_read_eeprom_datum(the_eeprom_datum,uc_value,enum_select_eeprom_datum_copy_first)){
		if (!ui_read_eeprom_datum(the_eeprom_datum,uc_value,enum_select_eeprom_datum_copy_second)){
            if (!ui_update_eeprom_datum(the_eeprom_datum,eeprom_data_info[the_eeprom_datum].p)){
                return 0;
            }
            // after refreshing the parameter value, now it should be ok!
            if (!ui_read_eeprom_datum(the_eeprom_datum,uc_value,enum_select_eeprom_datum_copy_first)){
                if (!ui_read_eeprom_datum(the_eeprom_datum,uc_value,enum_select_eeprom_datum_copy_second)){
                    return 0;
                }
            }
        }
    }
    // update the ram value
    memcpy(eeprom_data_info[the_eeprom_datum].p,uc_value,eeprom_data_info[the_eeprom_datum].uc_size);
    return 1;
}
unsigned int ui_load_from_eeprom_all_data(void){
    enum_eeprom_data the_eeprom_datum;
    unsigned int ui_retcode;
    ui_retcode=1;
    for(the_eeprom_datum=0;the_eeprom_datum<enum_eeprom_data_numof;the_eeprom_datum++){
        if (!ui_load_from_eeprom_datum(the_eeprom_datum)){
            ui_retcode=0;
        }
    }
    return ui_retcode;
}

unsigned int ui_save_to_eeprom_all_data(void){
    enum_eeprom_data the_eeprom_datum;
    unsigned int ui_retcode;
    ui_retcode=1;
    for(the_eeprom_datum=0;the_eeprom_datum<enum_eeprom_data_numof;the_eeprom_datum++){
        if (!ui_update_eeprom_datum(the_eeprom_datum,eeprom_data_info[the_eeprom_datum].p)){
            ui_retcode=0;
        }
    }
    return ui_retcode;
}


void v_format_eeprom(void){
    unsigned char c_reset_zero[16];
	enum_eeprom_data the_datum;
	v_eeprom_write_signature();
	the_datum=0;
    memset(c_reset_zero,0,sizeof(c_reset_zero));
	while(the_datum<enum_eeprom_data_numof){
		//v_erase_eeprom_datum(the_datum,enum_select_eeprom_datum_copy_first);
		//v_erase_eeprom_datum(the_datum,enum_select_eeprom_datum_copy_second);
        // reset to zero both copies of the parameter values
        v_save_eeprom_datum(the_datum,c_reset_zero,enum_select_eeprom_datum_copy_first);
        v_save_eeprom_datum(the_datum,c_reset_zero,enum_select_eeprom_datum_copy_second);
		the_datum++;
	}	
}

//#define def_disable_eeprom
#ifdef def_disable_eeprom
    #warning eeprom is disabled
#endif

// to encode/decode we use this 128bit key (16bytes=32 hex digits= 4 32 bit words):
// csmgroupbox#bbbb
// where:
//  csmgroupbox# is the 12-bytes string containing exactly the text shown
//  bbbb is the box serial number as a 32 bit value
//
// the decoded bytes should be in the form (16 bytes=32 hex digits=4 32-bit words):
// csm_bbbbppppvvvv
// where:
//  csm_ is the 4-bytes string containing exactly the text shown
//  bbbb is the box serial number as a 32 bit value, should be the same as our box
//  pppp is the parameter to set as a 32 bit value
//       0x0001-->spacer type
//       0x0002-->alarm lamp
//       0x0003-->fifth range
//       0x0004-->temperature compensation
//  vvvv is the value as a 32 bit value

#define def_xtea_key_header "csmgroupbox#"



enum_save_encrypted_param_retcode ui_save_encrypted_param(const char *hex_digit_string_to_decode){
    char string_to_decode[16+1];
    char c_string_decoded[16+1];
    unsigned int ui_decode[4];
    char c_key[16+1];
    unsigned int ui_key[4];
    unsigned char *puc;
    //enum_xxtea_decode_return_code xxtea_decode_return_code;
    int i;
    memset(string_to_decode,0,sizeof(string_to_decode));
    for (i=0;i<16;i++){
        char c1,c2;
        c1=toupper(*hex_digit_string_to_decode++);
        if (c1>='A') c1=c1-'A'+10; else c1=c1-'0';
        c2=*hex_digit_string_to_decode++;
        if (c2>='A') c2=c2-'A'+10; else c2=c2-'0';
        string_to_decode[i]=c1|(c2<<4);
    }
    memcpy(ui_decode,string_to_decode,sizeof(ui_decode));
    strcpy(c_key,def_xtea_key_header);
    memcpy(c_key+12,&ulBoxSerialNumber,4);
    memcpy(ui_key,c_key,sizeof(ui_key));
    xxtea_decode(ui_decode, 4, ui_key);
    memset(c_string_decoded,0,sizeof(c_string_decoded));
    memcpy(c_string_decoded,ui_decode,sizeof(ui_decode));
    if (strncmp(c_string_decoded,"csm_",4)){
        return enum_save_encrypted_param_retcode_err;
    }
    if (ui_decode[1]!=ulBoxSerialNumber){
        return enum_save_encrypted_param_retcode_invalid_box_number;
    }
    switch (ui_decode[2]){
        case 0x0000:
            ulBoxSerialNumber=ui_decode[3];
            puc=(unsigned char *)&ulBoxSerialNumber;
            ui_update_eeprom_datum(enum_eeprom_data_box_sn,puc);
            break;
        case 0x0001:
            puc=&macParms.uc_distanziatore_type;
            *puc=ui_decode[3];
            ui_update_eeprom_datum(enum_eeprom_data_spacer_type,puc);
            break;
        case 0x0002:
            puc=&macParms.ucAbilitaLampadaAllarme;
            *puc=ui_decode[3];
            ui_update_eeprom_datum(enum_eeprom_data_alarm_lamp,puc);
            break;
        case 0x0003:
            puc=&macParms.ucAbilitaQuintaPortata;
            *puc=ui_decode[3];
            ui_update_eeprom_datum(enum_eeprom_data_fifth_range,puc);
            break;
        case 0x0004:
            puc=&macParms.ucAbilitaCompensazioneTemperatura;
            *puc=ui_decode[3];
            ui_update_eeprom_datum(enum_eeprom_data_temperature_compensation,puc);
            break;
        case 0x0005:
            puc=&macParms.uc_enable_spindle_grinding;
            *puc=ui_decode[3];
            ui_update_eeprom_datum(enum_eeprom_data_spindle_grinding,puc);
            break;
        default:
            return enum_save_encrypted_param_retcode_invalid_param;            
    }

    return enum_save_encrypted_param_retcode_ok;
}

void v_handle_eeprom(void){
#ifdef def_disable_eeprom
    return;
#endif
	switch(eeprom_queue.status){
		case enum_eeprom_status_idle:
		default:
			eeprom_queue.status=enum_eeprom_status_init;
			break;
		case enum_eeprom_status_init:
			if (!ui_eeprom_verify_signature()){
				v_format_eeprom();
				if (!ui_eeprom_verify_signature()){
					eeprom_queue.errors++;
                    eeprom_queue.status=enum_eeprom_status_unrecoverable_error;
                    break;
				}
                if (!ui_save_to_eeprom_all_data()){
                    eeprom_queue.status=enum_eeprom_status_unrecoverable_error;
                    break;
                }
			}
			eeprom_queue.status=enum_eeprom_status_initialized;
			break;
		case enum_eeprom_status_initialized:
            if (!ui_load_from_eeprom_all_data()){
                eeprom_queue.status=enum_eeprom_status_unrecoverable_error;
                break;
            }
			eeprom_queue.status=enum_eeprom_status_ok;
			break;
        case enum_eeprom_status_ok:
            break;
        case enum_eeprom_status_unrecoverable_error:
            break;
	}
}


