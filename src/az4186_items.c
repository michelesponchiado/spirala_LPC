/*
 * az4186_items.c
 *
 *  Created on: Feb 14, 2017
 *      Author: michele
 */

#include "LPC23xx.h"                       /* LPC23xx/24xx definitions */
#include "type.h"
#include "irq.h"
#include "mci.h"
#include "dma.h"
#include "string.h"
#include "target.h"
//#include "rtc.h"
#include "ff.h"
#include "file_list.h"
#include "az4186_const.h"
#include "az4186_program_codes.h"
#include "az4186_commesse.h"
#include "az4186_spiralatrice.h"
#include "spiext.h"
#ifdef def_enable_items
#define def_items_filename "items.dat"
static unsigned int ui_items_handle;

typedef enum
{
	enum_item_save_retcode_OK,
	enum_item_save_retcode_unable_to_open,
	enum_item_save_retcode_unable_to_write,
	enum_item_save_retcode_unable_to_close,
	enum_item_save_retcode_numof
}enum_item_save_retcode;

typedef struct _type_struct_item
{
	char name[defMaxCharArticleName + 1];		// the item name
	char program_code[defMaxCharNomeCommessa+1];// the program code to use
	unsigned long num_pieces_to_make;			// the number of pieces to make
	float resistance_to_make_ohm;				// the resistance to make [ohm]
	unsigned long ucWorkingMode_0L_1LR;     	// the work mode
	unsigned char uc_description[defMaxCharDescrizioneLavoro+1];	// description
	float wire_specific_resistance[8];			// the specific resistance of the wires, max 8 wires handled
	char spare[64];
}type_struct_item;

// registers a new entry in file list registry
unsigned int ui_register_items(void){
    unsigned int retcode;
    TipoStructBodyInfo bodyInfo;
    enum_retcode_file_list_register retcode_file_list_register;

// init...
    retcode=0;
    bodyInfo.ul_body_size_bytes 			= sizeof(type_struct_item);
    bodyInfo.ul_key_offset_in_body_bytes 	=__builtin_offsetof(PROGRAMMA, name);
    bodyInfo.ul_key_size_bytes 				= defMaxCharArticleName;
// very important routine!
// here I check the consistency of the version of the product codes file... and eventually I do the necessary adjustments...
	ui_check_version_of_product_codes_file();
// try to register
    retcode_file_list_register = file_list_register(def_items_filename, &bodyInfo, &ui_items_handle);
    switch(retcode_file_list_register){
        case enum_retcode_file_list_register_ok:
            break;
        default:
            goto end_of_procedure;
    }
// check handle ok
    if (ui_handle_product_codes==def_file_list_invalid_handle){
        goto end_of_procedure;
    }
// returns ok
    retcode=1;
end_of_procedure:
    return retcode;
}//unsigned int ui_register_product_codes(void)

enum_item_save_retcode item_save(char *name)
{
	enum_item_save_retcode r = enum_item_save_retcode_OK;
	return r;
}
#endif
