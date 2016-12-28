/*****************************************************************************
 *   file_list.c:  module handling a file like a double queue
 *
 *   Copyright(C) 2012, CSM Group
 *   All rights reserved.
 *
 *   History
 *   september 27 2012  ver 1.00    Preliminary version, first Release
 *
******************************************************************************/
#ifdef _MSC_VER
	#define FIL FILE
	#define FRESULT int
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <stddef.h>
	

#else
	#include "LPC23xx.h"                       /* LPC23xx/24xx definitions */
	#include "type.h"
	#include "irq.h"
	#include "mci.h"
	#include "dma.h"
	#include "string.h"
	#include "target.h"
	#include "rtc.h"
	#include "ff.h"
#endif

#include "file_list.h"

#ifdef _MSC_VER
	int f_write(FILE *f, char *buffer, unsigned int uisize, unsigned int *pbyteswritten){
			unsigned int byteswritten;
			byteswritten=fwrite(buffer,1,uisize,f);
			*pbyteswritten=byteswritten;
			return byteswritten==uisize?0:1;
	};
	int f_read(FILE *f, char *buffer, unsigned int uisize, unsigned int *pbytesread){
			unsigned int bytesread;
			bytesread=fread(buffer,1,uisize,f);
			*pbytesread=bytesread;
			return bytesread==uisize?0:1;
	};
	int f_lseek (FILE *f,unsigned long uloffset){
		return fseek(f, uloffset, SEEK_SET);
	};
	unsigned long f_size(FILE *f) {
		unsigned long ulpos,ulsize;
		ulpos=ftell(f);
		if( fseek(f, 0, SEEK_END) ){
		  return -1;
		}
		ulsize = ftell(f);
		if( fseek(f, ulpos, SEEK_SET) ){
		  return -1;
		}
		return ulsize;
	}


#endif

// actual file list version
#define def_file_list_version 1

// max num of char in key 
#define def_max_char_file_list_key 32

// the valid entry code
#define def_file_list_valid_entry_code 0xa55a3663
// the updating entry code
#define def_file_list_updating_entry_code 0x12345678
// the deleted entry code
#define def_file_list_deleted_entry_code 0xF0F0F0F0


// variables marking body busy/free
static const unsigned int ui_file_list_mark_body_busy=0xFFFFFFFF;
static const unsigned int ui_file_list_mark_body_free=0x00000000;
// this variable contains the offset of the last block found with file list find
static unsigned long ul_offset_last_element_found;



// creates a file list
// input:
//	* file name as string
//	* pointer to file pointer
//	* pointer to body info
// returns:
// 	* retcode indicating the operation status
// 	* it modifies pfil so it contains the file pointer if the file has been opened ok, 
#ifdef _MSC_VER
enum_create_file_list_retcode create_file_list(char *file_name,FIL **pfil,TipoStructBodyInfo *pBodyInfo);
#else
enum_create_file_list_retcode create_file_list(char *file_name,FIL *pfil,TipoStructBodyInfo *pBodyInfo);
#endif



// opens an existing file list
// input:
//	* file name as string
//	* pointer to file pointer
//	* pointer to body info
// returns:
// 	* retcode indicating the operation status
// 	* it modifies pfil so it contains the file pointer if the file has been opened ok, 
#ifdef _MSC_VER
enum_open_file_list_retcode open_file_list(char *file_name,FIL **pfil,TipoStructBodyInfo *pBodyInfo);
#else
enum_open_file_list_retcode open_file_list(char *file_name,FIL *pfil,TipoStructBodyInfo *pBodyInfo);
#endif

// creates a file list
// input:
//	* file name as string
//	* pointer to file pointer
//	* pointer to body info
// returns:
// 	* retcode indicating the operation status
// 	* it modifies pfil so it contains the file pointer if the file has been opened ok, 
#ifdef _MSC_VER
enum_create_file_list_retcode create_file_list(char *file_name,FIL **pfil,TipoStructBodyInfo *pBodyInfo);
#else
enum_create_file_list_retcode create_file_list(char *file_name,FIL *pfil,TipoStructBodyInfo *pBodyInfo);
#endif


// crc handling...
static unsigned long    crc_tab32[256];
static unsigned int     crc_tab32_init=0;
#define                 P_32        0x4C11DB7L

   /*******************************************************************\
    *                                                                   *
    *   static void init_crc32_tab( void );                             *
    *                                                                   *
    *   The function init_crc32_tab() is used  to  fill  the  array     *
    *   for calculation of the CRC-32 with values.                      *
    *                                                                   *
    \*******************************************************************/

static void init_crc32_tab( void ) {
    int i, j;
    unsigned long crc;

    for (i=0; i<256; i++) {
        crc = (unsigned long) i;
        for (j=0; j<8; j++) {
            if ( crc & 0x00000001L ) crc = ( crc >> 1 ) ^ P_32;
            else                     crc =   crc >> 1;
        }
        crc_tab32[i] = crc;
    }
    crc_tab32_init = 1;
}  /* init_crc32_tab */

// calculate crc of the body
unsigned int ui_file_list_crc(char *pBody, unsigned int ul_body_size_bytes){
	unsigned int i, crc_32;
	// initialize crc table???
	if (!crc_tab32_init)
		init_crc32_tab();
	// crc32 seed  value
	crc_32         = 0xffffffffL;
	// loop for crc32 calculation
	for (i=0;i<ul_body_size_bytes;i++){
		register unsigned char uc_idx;
		//long_c = 0x000000ffL & (unsigned long) (*pBody++);
		//tmp = crc_32 ^ long_c;
		uc_idx=((*pBody++)^crc_32);
		crc_32 = (crc_32 >> 8) ^ crc_tab32[ uc_idx ];
	}
	return crc_32;	
}// unsigned int ui_file_list_crc(char *pBody, unsigned int ul_body_size_bytes)

// get info about the body actually stored in the file
// just to execute a version check!
// returns 0 if error, 1 means all ok
unsigned int ui_get_header_body_size_file_list(char *pc_filename, TipoStructBodyInfo *pBody){
	FIL fil;
	FRESULT ff_return_code;
	TipoStructQueueElementHeader header;
	unsigned int bytes_read;
	unsigned int ui_retcode;
// initialize retcode to 0-->unknown body size...
	ui_retcode=0;
// always blank result structure	
	memset(pBody,0,sizeof(pBody));
	
	ff_return_code = f_open(&fil, pc_filename, FA_WRITE | FA_OPEN_EXISTING | FA_READ);
// error opening??? it returns error
	if (ff_return_code){
		goto end_of_procedure_no_close;
	}	
	// read the header
	ff_return_code = f_read(&fil, (char*)&header, sizeof(header), &bytes_read);	
	if (ff_return_code||(bytes_read!=sizeof(header))){
		goto end_of_procedure;
	}
	if (header.ui_valid_entry_0xa55a3663!=def_file_list_valid_entry_code){
		goto end_of_procedure;
    }
    // get body infos
    memcpy(pBody,&header.body_info,sizeof(header.body_info));
	// all ok!
	ui_retcode=1;
	
	// get finally the actual body size...
	
end_of_procedure:	
	f_close(&fil);
end_of_procedure_no_close:	
	return ui_retcode;
}



// opens an existing file list
// input:
//	* file name as string
//	* pointer to file pointer
//	* pointer to body info
// returns:
// 	* retcode indicating the operation status
// 	* it modifies pfil so it contains the file pointer if the file has been opened ok, 
#ifdef _MSC_VER
enum_open_file_list_retcode open_file_list(char *file_name,FIL **pfil,TipoStructBodyInfo *pBodyInfo){
#else
enum_open_file_list_retcode open_file_list(char *file_name,FIL *pfil,TipoStructBodyInfo *pBodyInfo){
#endif
	FRESULT ff_return_code;
	enum_create_file_list_retcode retcode;
	static TipoStructQueueElementHeader header;
	static TipoStructQueueElementFooter footer;
	unsigned int bytes_read,ui_check_file_list_mark_body_free;

	
	// initialize the retcode to OK
	retcode=enum_open_file_list_retcode_ok;
	
	// is the key size valid???
	if (pBodyInfo->ul_key_size_bytes>def_max_char_file_list_key){
		retcode=enum_open_file_list_retcode_invalid_key_size;
		goto end_of_procedure;
	}
	
	// try to open the file
#ifdef _MSC_VER
	fopen_s(pfil,file_name,"r+b");
	ff_return_code = *pfil?0:1;
#else
    ff_return_code = f_open(pfil, file_name, FA_WRITE | FA_OPEN_EXISTING | FA_READ);
#endif
	// error opening???
	if (ff_return_code){
		retcode=enum_open_file_list_retcode_unable_to_open;
		goto end_of_procedure;
	}
	
	// read the infos of the header
#ifdef _MSC_VER
	ff_return_code = f_read(*pfil, (char*)&header, sizeof(header), &bytes_read);
#else
	ff_return_code = f_read(pfil, (char*)&header, sizeof(header), &bytes_read);
#endif
	if (ff_return_code||(bytes_read!=sizeof(header))){
		retcode=enum_open_file_list_retcode_unable_to_write_header;
		goto end_of_procedure;
	}
	// initialize valid entry field
	if (header.ui_valid_entry_0xa55a3663!=def_file_list_valid_entry_code){
		retcode=enum_open_file_list_retcode_invalid_entry;
		goto end_of_procedure;
    }
    // verifies file version
	if (header.ui_version!=def_file_list_version){
    }
    // verifies body infos
    if (memcmp(&header.body_info,pBodyInfo,sizeof(header.body_info))){
		retcode=enum_open_file_list_retcode_invalid_body_info;
		goto end_of_procedure;
    }
    // check if body free            ???
#ifdef _MSC_VER
    ff_return_code = f_read(*pfil, (char*)&ui_check_file_list_mark_body_free, sizeof(ui_check_file_list_mark_body_free), &bytes_read);
#else
    ff_return_code = f_read(pfil, (char*)&ui_check_file_list_mark_body_free, sizeof(ui_check_file_list_mark_body_free), &bytes_read);
#endif
    if (ff_return_code||(bytes_read!=sizeof(ui_file_list_mark_body_free))){
        retcode=enum_open_file_list_retcode_unable_to_read_body;
        goto end_of_procedure;
    }        
// is the body free??
    if (ui_check_file_list_mark_body_free!=ui_file_list_mark_body_free){
        // retcode=enum_open_file_list_retcode_unfreed_body;
        // goto end_of_procedure;
    }
	// skip the body, then write down the footer
#ifdef _MSC_VER
	ff_return_code = f_lseek (*pfil,sizeof(header)+pBodyInfo->ul_body_size_bytes);
#else
	ff_return_code = f_lseek (pfil,sizeof(header)+pBodyInfo->ul_body_size_bytes);
#endif
	if (ff_return_code){
		retcode=enum_open_file_list_retcode_unable_to_seek;
		goto end_of_procedure;
	}
	// read the footer
	memset(&footer,0,sizeof(footer));
	
	// write down the infos of the footer
#ifdef _MSC_VER
	ff_return_code = f_read(*pfil, (char*)&footer, sizeof(footer), &bytes_read);
#else
	ff_return_code = f_read(pfil, (char*)&footer, sizeof(footer), &bytes_read);
#endif
	if (ff_return_code||(bytes_read!=sizeof(footer))){
		retcode=enum_open_file_list_retcode_unable_to_read_footer;
		goto end_of_procedure;
	}
	if (footer.ui_valid_entry_0xa55a3663!=def_file_list_valid_entry_code){
		retcode=enum_open_file_list_retcode_invalid_footer;
		goto end_of_procedure;
    }
// always execute a sync to flush buffers!
	f_sync(pfil);
end_of_procedure:
	return retcode;
}//enum_open_file_list_retcode open_file_list(char *file_name,FIL *pfil,TipoStructBodyInfo *pBodyInfo)


// create a file list
// input:
//	* file name as string
//	* pointer to file pointer
//	* pointer to body info
// returns:
// 	* retcode indicating the operation status
// 	* it modifies pfil so it contains the file pointer if the file has been opened ok, 
#ifdef _MSC_VER
enum_create_file_list_retcode create_file_list(char *file_name,FIL **pfil,TipoStructBodyInfo *pBodyInfo){
#else
enum_create_file_list_retcode create_file_list(char *file_name,FIL *pfil,TipoStructBodyInfo *pBodyInfo){
#endif
	FRESULT ff_return_code;
	enum_create_file_list_retcode retcode;
	static TipoStructQueueElementHeader header;
	static TipoStructQueueElementFooter footer;
	unsigned int bytes_written;

	
	// initialize the retcode to OK
	retcode=enum_create_file_list_retcode_ok;
	
	// is the key size valid???
	if (pBodyInfo->ul_key_size_bytes>def_max_char_file_list_key){
		retcode=enum_create_file_list_retcode_invalid_key_size;
		goto end_of_procedure;
	}
	
	// try to open the file
#ifdef _MSC_VER
	fopen_s(pfil,file_name,"w+b");
	ff_return_code = *pfil?0:1;
#else
    ff_return_code = f_open(pfil, file_name, FA_WRITE | FA_CREATE_ALWAYS | FA_READ);
#endif
	// error opening???
	if (ff_return_code){
		retcode=enum_create_file_list_retcode_unable_to_open;
		goto end_of_procedure;
	}
	// initialize valid entry field
	header.ui_valid_entry_0xa55a3663=def_file_list_valid_entry_code;
	header.ui_version=def_file_list_version;				// the block header version 
	memcpy(&header.body_info,pBodyInfo,sizeof(header.body_info));	// copy infos about the body
	
	// write down the infos of the header
#ifdef _MSC_VER
	ff_return_code = f_write(*pfil, (char*)&header, sizeof(header), &bytes_written);
#else
	ff_return_code = f_write(pfil, (char*)&header, sizeof(header), &bytes_written);
#endif
	if (ff_return_code||(bytes_written!=sizeof(header))){
		retcode=enum_create_file_list_retcode_unable_to_write_header;
		goto end_of_procedure;
	}
 // mark body free            
#ifdef _MSC_VER
    ff_return_code = f_write(*pfil, (char*)&ui_file_list_mark_body_free, sizeof(ui_file_list_mark_body_free), &bytes_written);
#else
    ff_return_code = f_write(pfil, (char*)&ui_file_list_mark_body_free, sizeof(ui_file_list_mark_body_free), &bytes_written);
#endif
    if (ff_return_code||(bytes_written!=sizeof(ui_file_list_mark_body_free))){
        retcode=enum_create_file_list_retcode_unable_to_write_body;
        goto end_of_procedure;
    }        
	// skip the body, then write down the footer
#ifdef _MSC_VER
	ff_return_code = f_lseek (*pfil,sizeof(header)+pBodyInfo->ul_body_size_bytes);
#else
	ff_return_code = f_lseek (pfil,sizeof(header)+pBodyInfo->ul_body_size_bytes);
#endif
	if (ff_return_code){
		retcode=enum_create_file_list_retcode_unable_to_seek;
		goto end_of_procedure;
	}
	// reset the footer
	memset(&footer,0,sizeof(footer));
	footer.ui_valid_entry_0xa55a3663=def_file_list_valid_entry_code;
	//footer.ul_next_block_offset=0;	// offset of the next block; 
	//footer.ul_prev_block_offset=0;	// offset of the prev block; 
	
	// write down the infos of the footer
#ifdef _MSC_VER
	ff_return_code = f_write(*pfil, (char*)&footer, sizeof(footer), &bytes_written);
#else
	ff_return_code = f_write(pfil, (char*)&footer, sizeof(footer), &bytes_written);
#endif
	if (ff_return_code||(bytes_written!=sizeof(footer))){
		retcode=enum_create_file_list_retcode_unable_to_write_footer;
		goto end_of_procedure;
	}
// always execute a sync to flush buffers!
	f_sync(pfil);
	
end_of_procedure:
	return retcode;
}//enum_create_file_list_retcode create_file_list(char *file_name,FIL *pfil,TipoStructBodyInfo *pBodyInfo)

// return codes from file list find
typedef enum{
	enum_find_file_list_retcode_ok=0,
	enum_find_file_list_retcode_key_already_exists,
	enum_find_file_list_retcode_unable_to_seek,
	enum_find_file_list_retcode_unable_to_read_footer,
	enum_find_file_list_retcode_invalid_footer,
	enum_find_file_list_retcode_invalid_key_size,
	enum_find_file_list_retcode_unable_to_read_key,
	enum_find_file_list_retcode_numof
}enum_find_file_list_retcode;

typedef struct _TipoStructFindInFileList{
	FIL *pfil;
	char *pBody;
	TipoStructBodyInfo *pBodyInfo;
	unsigned long ul_previous_block_offset,ul_next_block_offset;
}TipoStructFindInFileList;
//	unsigned long ul_body_size_bytes;		// the size of the body
//	unsigned long ul_key_offset_in_body_bytes;// the offset of the key in the body
//	unsigned long ul_key_size_bytes;			// the size of the key in bytes


	
// look for a key in the file list specified, the routine sets the offset to the previous and to the next element in list
enum_find_file_list_retcode file_list_find_element(TipoStructFindInFileList *pf){
	enum_find_file_list_retcode retcode;
	static TipoStructQueueElementFooter footer;
	unsigned int bytes_read;
	unsigned long ul_footer_offset,ul_key_size,ul_key_offset;
	char *pkey_to_find;
	static char key_buffer[def_max_char_file_list_key];
    FRESULT ff_return_code;
	
	// all ok by now
	retcode=enum_find_file_list_retcode_ok;

	// initialize key size
	ul_key_size=pf->pBodyInfo->ul_key_size_bytes;	
	// check key buffer size
	if (ul_key_size>sizeof(key_buffer)){
		retcode=enum_find_file_list_retcode_invalid_key_size;
		goto end_of_procedure;
	}
	// initialize footer offset from beginning of block
	ul_footer_offset=sizeof(TipoStructQueueElementHeader)+pf->pBodyInfo->ul_body_size_bytes;
	// initialize key offset from beginning of block
	ul_key_offset=sizeof(TipoStructQueueElementHeader)+pf->pBodyInfo->ul_key_offset_in_body_bytes;
	// get the pointer to the key of the element to insert
	pkey_to_find=&pf->pBody[pf->pBodyInfo->ul_key_offset_in_body_bytes];
	
	// seek to the footer of the head element
	ff_return_code = f_lseek (pf->pfil,ul_footer_offset);
	if (ff_return_code){
		retcode=enum_find_file_list_retcode_unable_to_seek;
		goto end_of_procedure;
	}
	// read footer of the head
	ff_return_code = f_read(pf->pfil, (char*)&footer, sizeof(footer), &bytes_read);
	if (ff_return_code||(bytes_read!=sizeof(footer))){
		retcode=enum_find_file_list_retcode_unable_to_read_footer;
		goto end_of_procedure;
	}
	// footer invalid???
	if (footer.ui_valid_entry_0xa55a3663!=def_file_list_valid_entry_code){
		retcode=enum_find_file_list_retcode_invalid_footer;
		goto end_of_procedure;
	}
	// offset of the previous block
	pf->ul_previous_block_offset=0;
	// offset of the next block
	pf->ul_next_block_offset=footer.ul_next_block_offset;
	// loop waiting to find the block position!
	while (pf->ul_next_block_offset){
		// seek to the key of the next block
		ff_return_code = f_lseek (pf->pfil,pf->ul_next_block_offset+ul_key_offset);
		// seek has completed ok?
		if (ff_return_code){
			retcode=enum_find_file_list_retcode_unable_to_seek;
			goto end_of_procedure;
		}
		// read key 
		ff_return_code = f_read(pf->pfil, (char*)&key_buffer, ul_key_size, &bytes_read);
		if (ff_return_code||(bytes_read!=ul_key_size)){
			retcode=enum_find_file_list_retcode_unable_to_read_key;
			goto end_of_procedure;
		}
		// compare the keys: key to find vs actual...
		{
			int memcmp_retcode;
            // do the compare: compare key to find with the key read from file
			memcmp_retcode=memcmp(pkey_to_find,key_buffer,ul_key_size);
			// is the key to find bigger than the actual one? if yes, continue looking for...
			if (memcmp_retcode>0){
				// move to the footer of the current block
				ff_return_code = f_lseek (pf->pfil,pf->ul_next_block_offset+ul_footer_offset);
				if (ff_return_code){
					retcode=enum_find_file_list_retcode_unable_to_seek;
					goto end_of_procedure;
				}
				// read footer
				ff_return_code = f_read(pf->pfil, (char*)&footer, sizeof(footer), &bytes_read);
				if (ff_return_code||(bytes_read!=sizeof(footer))){
					retcode=enum_find_file_list_retcode_unable_to_read_footer;
					goto end_of_procedure;
				}
				// footer invalid???
				if (footer.ui_valid_entry_0xa55a3663!=def_file_list_valid_entry_code){
					retcode=enum_find_file_list_retcode_invalid_footer;
					goto end_of_procedure;
				}
				// offset of the previous block becomes the actual next 
				pf->ul_previous_block_offset=pf->ul_next_block_offset;
				// offset of the next block is the offset in the footer
				pf->ul_next_block_offset=footer.ul_next_block_offset;
			}
			// is the key to find smaller than the actual one? if yes, we've found where to insert
			// the new block...
			else if (memcmp_retcode<0){
				break;
			}
			// if they are equal, duplicated key??? very strange!
			else{
				// pf->ul_next_block_offset points to the block found
				retcode=enum_find_file_list_retcode_key_already_exists;
                // save the offset of the last element found
                ul_offset_last_element_found=pf->ul_next_block_offset;
				goto end_of_procedure;
			}
		}
	}
// if the program runs here, if retcode is "enum_find_file_list_retcode_ok" I have found where to insert the block
// pf->ul_previous_block_offset and pf->ul_next_block_offset
// contain the offset of the previous and of the next block in list
end_of_procedure:
	return retcode;
}



// insert an element in the file list
enum_insert_file_list_retcode insert_file_list(FIL *pfil,char *pBody,TipoStructBodyInfo *pBodyInfo,unsigned long *pul_offset_block_inserted){
	enum_find_file_list_retcode find_file_list_retcode;
	enum_insert_file_list_retcode retcode;
	static TipoStructFindInFileList ff_list;
    FRESULT ff_return_code;
	
	// all ok by now
	retcode=enum_insert_file_list_retcode_ok;
	
	// initialize struct to find the key
	ff_list.pfil=pfil;
	ff_list.pBody=pBody;
	ff_list.pBodyInfo=pBodyInfo;
	ff_list.ul_previous_block_offset=0;
	ff_list.ul_next_block_offset=0;
	// find!
	find_file_list_retcode=file_list_find_element(&ff_list);
	// found?
	switch(find_file_list_retcode){
		case enum_find_file_list_retcode_ok:
            {
                unsigned int bytes_written,ui_crc;
                unsigned long ul_offset_new_block;
                TipoStructQueueElementHeader header;
                TipoStructQueueElementFooter footer;            
                // problema: devo assicurare la integrit� del file...
                // per farlo, metto nella head un marcatore che indica elaborazione in corso
                // head.modifying=1
                // append element, this->prev=prev, this->next=next
                // prev element->next= this
                // next element->prev=this
                // head.modifying=0
                // se all'avvio trovo head.modifying impostato, devo riscandire tutto il file...

// seek to the start of file, just to mark that we're starting a file list modification
                ff_return_code = f_lseek (ff_list.pfil,sizeof(TipoStructQueueElementHeader));
                // seek has completed ok?
                if (ff_return_code){
                    retcode=enum_insert_file_list_retcode_unable_to_insert;
                    goto end_of_procedure;
                }
// mark body busy            
                ff_return_code = f_write(ff_list.pfil, (char*)&ui_file_list_mark_body_busy, sizeof(ui_file_list_mark_body_busy), &bytes_written);
                if (ff_return_code||(bytes_written!=sizeof(ui_file_list_mark_body_busy))){
                    retcode=enum_insert_file_list_retcode_unable_to_insert;
                    goto end_of_procedure;
                }        
// append one element            
                // get the offset of the new block: append @ the end of file
                ul_offset_new_block=f_size(ff_list.pfil);
                if (pul_offset_block_inserted)
                    *pul_offset_block_inserted=ul_offset_new_block;
                // seek to the end of file
                ff_return_code = f_lseek (ff_list.pfil,ul_offset_new_block);
                // has seek completed ok?
                if (ff_return_code){
                    retcode=enum_insert_file_list_retcode_unable_to_insert;
                    goto end_of_procedure;
                }
// write down header
// initialize valid entry field to "updating"
                header.ui_valid_entry_0xa55a3663=def_file_list_updating_entry_code;
                header.ui_version=def_file_list_version;				// the block header version 
                memcpy(&header.body_info,pBodyInfo,sizeof(header.body_info));	// copy infos about the body                
                // write down the infos of the header
                ff_return_code = f_write(pfil, (char*)&header, sizeof(header), &bytes_written);
                if (ff_return_code||(bytes_written!=sizeof(header))){
                    retcode=enum_insert_file_list_retcode_unable_to_insert;
                    goto end_of_procedure;
                } 
// calculate crc            
				ui_crc=ui_file_list_crc(pBody, pBodyInfo->ul_body_size_bytes);
// write down the body            
                ff_return_code = f_write(ff_list.pfil, pBody, pBodyInfo->ul_body_size_bytes, &bytes_written);
                if (ff_return_code||(bytes_written!=pBodyInfo->ul_body_size_bytes)){
                    retcode=enum_insert_file_list_retcode_unable_to_insert;
                    goto end_of_procedure;
                } 
// write down the footer            
                // reset the footer
                memset(&footer,0,sizeof(footer));
                footer.ui_valid_entry_0xa55a3663=def_file_list_valid_entry_code;
                footer.ul_next_block_offset=ff_list.ul_next_block_offset;	// offset of the next block; 
                footer.ul_prev_block_offset=ff_list.ul_previous_block_offset;	// offset of the prev block; 
                footer.ui_body_crc=~ui_crc;		// the crc of the body is the 1-complement f the calculated crc
                
                // write down the infos of the footer
                ff_return_code = f_write(ff_list.pfil, (char*)&footer, sizeof(footer), &bytes_written);
                if (ff_return_code||(bytes_written!=sizeof(footer))){
                    retcode=enum_insert_file_list_retcode_unable_to_insert;
                    goto end_of_procedure;
                }
// update header setting valid entry field
                header.ui_valid_entry_0xa55a3663=def_file_list_valid_entry_code;
                // seek to the start of block
                ff_return_code = f_lseek (ff_list.pfil,ul_offset_new_block);
                // has seek completed ok?
                if (ff_return_code){
                    retcode=enum_insert_file_list_retcode_unable_to_insert;
                    goto end_of_procedure;
                }
                // write down the infos of the header
                ff_return_code = f_write(pfil, (char*)&header, sizeof(header), &bytes_written);
                if (ff_return_code||(bytes_written!=sizeof(header))){
                    retcode=enum_insert_file_list_retcode_unable_to_insert;
                    goto end_of_procedure;
                } 

// update previous block 
                // seek to the previous block, where the next block ptr field is
                ff_return_code = f_lseek (ff_list.pfil,
                                          ff_list.ul_previous_block_offset
                                         +sizeof(header)
                                         +pBodyInfo->ul_body_size_bytes
#ifndef _MSC_VER
                                         +__builtin_offsetof(TipoStructQueueElementFooter,ul_next_block_offset)
#else
                                         +(char*)&(footer.ul_next_block_offset)-(char*)&footer
#endif
                                        );
            
                // update next ptr of previous block
                ff_return_code = f_write(ff_list.pfil, (char*)&ul_offset_new_block, sizeof(ul_offset_new_block), &bytes_written);
                if (ff_return_code||(bytes_written!=sizeof(ul_offset_new_block))){
                    retcode=enum_insert_file_list_retcode_unable_to_insert;
                    goto end_of_procedure;
                }
// update next block 
                // seek to the next block, where the previous block ptr field is
                ff_return_code = f_lseek (ff_list.pfil,
                                          ff_list.ul_next_block_offset
                                         +sizeof(header)
                                         +pBodyInfo->ul_body_size_bytes
#ifndef _MSC_VER
                                         +__builtin_offsetof(TipoStructQueueElementFooter,ul_prev_block_offset)
#else
                                         +(char*)&(footer.ul_prev_block_offset)-(char*)&footer
#endif
                                        );
            
                // update previous ptr of the next block
                ff_return_code = f_write(ff_list.pfil, (char*)&ul_offset_new_block, sizeof(ul_offset_new_block), &bytes_written);
                if (ff_return_code||(bytes_written!=sizeof(ul_offset_new_block))){
                    retcode=enum_insert_file_list_retcode_unable_to_insert;
                    goto end_of_procedure;
                }
                
// seek to the start of file, just to mark that we've ended a file list modification
                ff_return_code = f_lseek (ff_list.pfil,sizeof(TipoStructQueueElementHeader));
                // seek has completed ok?
                if (ff_return_code){
                    retcode=enum_insert_file_list_retcode_unable_to_insert;
                    goto end_of_procedure;
                }                
 // header: writing finished
                ff_return_code = f_write(ff_list.pfil, (char*)&ui_file_list_mark_body_free, sizeof(ui_file_list_mark_body_free), &bytes_written);
                if (ff_return_code||(bytes_written!=sizeof(ui_file_list_mark_body_free))){
                    retcode=enum_insert_file_list_retcode_unable_to_insert;
                    goto end_of_procedure;
                }        
            
        
                break;
            }
		case enum_find_file_list_retcode_key_already_exists:
			retcode=enum_insert_file_list_retcode_already_exists;
			goto end_of_procedure;
			break;
		case enum_find_file_list_retcode_unable_to_seek:
		case enum_find_file_list_retcode_unable_to_read_footer:
		case enum_find_file_list_retcode_invalid_footer:
		case enum_find_file_list_retcode_invalid_key_size:
		case enum_find_file_list_retcode_unable_to_read_key:
		default:
			retcode=enum_insert_file_list_retcode_unable_to_insert;
			goto end_of_procedure;
			break;
	}
// always execute a sync to flush buffers!
	f_sync(ff_list.pfil);

end_of_procedure:	
	return retcode;
	
	
}//enum_insert_file_list_retcode insert_file_list(FIL *pfil,char *pBody, unsigned long *pul)





// return codes from file list deletion
typedef enum{
	enum_delete_file_list_retcode_ok=0,
	enum_delete_file_list_retcode_unable_to_delete,
	enum_delete_file_list_retcode_numof
}enum_delete_file_list_retcode;

// delete an element from the file list
enum_delete_file_list_retcode delete_file_list(FIL *pfil,char *pBody,TipoStructBodyInfo *pBodyInfo){
	enum_find_file_list_retcode find_file_list_retcode;
	enum_delete_file_list_retcode retcode;
	static TipoStructFindInFileList ff_list;
    FRESULT ff_return_code;
	
	// all ok by now
	retcode=enum_delete_file_list_retcode_ok;
	
	// initialize struct to find the key
	ff_list.pfil=pfil;
	ff_list.pBody=pBody;
	ff_list.pBodyInfo=pBodyInfo;
	ff_list.ul_previous_block_offset=0;
	ff_list.ul_next_block_offset=0;
	// find!
	find_file_list_retcode=file_list_find_element(&ff_list);
	// found?
	switch(find_file_list_retcode){
		case enum_find_file_list_retcode_key_already_exists:
			// found?
            {
                unsigned int bytes_written,bytes_read;
				unsigned long offset_previous,offset_next;
                TipoStructQueueElementHeader header;
                TipoStructQueueElementFooter footer;            
                // problema: devo assicurare la integrit� del file...
                // per farlo, metto nella head un marcatore che indica elaborazione in corso
                // head.modifying=1
                // ff_list.ul_next_block_offset points to the block to delete
				// offset_block_to_delete=ff_list.ul_next_block_offset
				// offset_previous=footer[offset_block_to_delete].ul_prev_block_offset
				// offset_next=footer[offset_block_to_delete].ul_next_block_offset
				// footer[offset_previous].next=offset_next
				// footer[offset_next].next=offset_previous
                // head.modifying=0
                // se all'avvio trovo head.modifying impostato, devo riscandire tutto il file...

// seek to the start of file, just to mark that we're starting a file list modification
                ff_return_code = f_lseek (ff_list.pfil,sizeof(TipoStructQueueElementHeader));
                // seek has completed ok?
                if (ff_return_code){
                    retcode=enum_delete_file_list_retcode_unable_to_delete;
                    goto end_of_procedure;
                }
// mark body busy            
                ff_return_code = f_write(ff_list.pfil, (char*)&ui_file_list_mark_body_busy, sizeof(ui_file_list_mark_body_busy), &bytes_written);
                if (ff_return_code||(bytes_written!=sizeof(ui_file_list_mark_body_busy))){
                    retcode=enum_delete_file_list_retcode_unable_to_delete;
                    goto end_of_procedure;
                }        
// get info about element to delete
                // seek to the header of block
                ff_return_code = f_lseek (ff_list.pfil,ff_list.ul_next_block_offset);
                // has seek completed ok?
                if (ff_return_code){
                    retcode=enum_delete_file_list_retcode_unable_to_delete;
                    goto end_of_procedure;
                }
				// read header
				ff_return_code = f_read(ff_list.pfil, (char*)&header, sizeof(header), &bytes_read);
                if (ff_return_code||(bytes_read!=sizeof(header))){
                    retcode=enum_delete_file_list_retcode_unable_to_delete;
                    goto end_of_procedure;
                }  
				// seek to the footer
				ff_return_code = f_lseek (ff_list.pfil,ff_list.ul_next_block_offset+sizeof(header)+pBodyInfo->ul_body_size_bytes);
                // has seek completed ok?
                if (ff_return_code){
                    retcode=enum_delete_file_list_retcode_unable_to_delete;
                    goto end_of_procedure;
                }
				// read footer
				ff_return_code = f_read(ff_list.pfil, (char*)&footer, sizeof(footer), &bytes_read);
                if (ff_return_code||(bytes_read!=sizeof(footer))){
                    retcode=enum_delete_file_list_retcode_unable_to_delete;
                    goto end_of_procedure;
                }   
				// get offset of next and prev block
				offset_previous=footer.ul_prev_block_offset;
				offset_next=footer.ul_next_block_offset;
				// mark block as deleted
// write down header marking it as deleted
                header.ui_valid_entry_0xa55a3663=def_file_list_deleted_entry_code;
 // seek to the header of block
                ff_return_code = f_lseek (ff_list.pfil,ff_list.ul_next_block_offset);
                // has seek completed ok?
                if (ff_return_code){
                    retcode=enum_delete_file_list_retcode_unable_to_delete;
                    goto end_of_procedure;
                }
                // write down the infos of the header
                ff_return_code = f_write(pfil, (char*)&header, sizeof(header), &bytes_written);
                if (ff_return_code||(bytes_written!=sizeof(header))){
                    retcode=enum_delete_file_list_retcode_unable_to_delete;
                    goto end_of_procedure;
                } 

// update previous block 
                // seek to the previous block, where the next block ptr field is
                ff_return_code = f_lseek (ff_list.pfil,
                                          offset_previous
                                         +sizeof(header)
                                         +pBodyInfo->ul_body_size_bytes
#ifndef _MSC_VER
                                         +__builtin_offsetof(TipoStructQueueElementFooter,ul_next_block_offset)
#else
                                         +(char*)&(footer.ul_next_block_offset)-(char*)&footer
#endif
                                        );
            
                // update next ptr of previous block
                ff_return_code = f_write(ff_list.pfil, (char*)&offset_next, sizeof(offset_next), &bytes_written);
                if (ff_return_code||(bytes_written!=sizeof(offset_next))){
                    retcode=enum_delete_file_list_retcode_unable_to_delete;
                    goto end_of_procedure;
                }
// update next block 
                // seek to the next block, where the previous block ptr field is
                ff_return_code = f_lseek (ff_list.pfil,
                                          offset_next
                                         +sizeof(header)
                                         +pBodyInfo->ul_body_size_bytes
#ifndef _MSC_VER
                                         +__builtin_offsetof(TipoStructQueueElementFooter,ul_prev_block_offset)
#else
                                         +(char*)&(footer.ul_prev_block_offset)-(char*)&footer
#endif
                                        );
            
                // update previous ptr of the next block
                ff_return_code = f_write(ff_list.pfil, (char*)&offset_previous, sizeof(offset_previous), &bytes_written);
                if (ff_return_code||(bytes_written!=sizeof(offset_previous))){
                    retcode=enum_delete_file_list_retcode_unable_to_delete;
                    goto end_of_procedure;
                }
                
// seek to the start of file, just to mark that we've ended a file list modification
                ff_return_code = f_lseek (ff_list.pfil,sizeof(TipoStructQueueElementHeader));
                // seek has completed ok?
                if (ff_return_code){
                    retcode=enum_delete_file_list_retcode_unable_to_delete;
                    goto end_of_procedure;
                }                
 // header: writing finished
                ff_return_code = f_write(ff_list.pfil, (char*)&ui_file_list_mark_body_free, sizeof(ui_file_list_mark_body_free), &bytes_written);
                if (ff_return_code||(bytes_written!=sizeof(ui_file_list_mark_body_free))){
                    retcode=enum_delete_file_list_retcode_unable_to_delete;
                    goto end_of_procedure;
                }        
            
        
                break;
            }
		case enum_find_file_list_retcode_unable_to_seek:
		case enum_find_file_list_retcode_unable_to_read_footer:
		case enum_find_file_list_retcode_invalid_footer:
		case enum_find_file_list_retcode_invalid_key_size:
		case enum_find_file_list_retcode_unable_to_read_key:
		case enum_find_file_list_retcode_ok:
		default:
			retcode=enum_delete_file_list_retcode_unable_to_delete;
			goto end_of_procedure;
			break;
	}
// always execute a sync to flush buffers!
	f_sync(ff_list.pfil);

end_of_procedure:	
	return retcode;
	
}//enum_delete_file_list_retcode delete_file_list(FIL *pfil,char *pBody)






// walking file list
enum_walk_file_list_retcode	walk_file_list(TipoStructQueueListWalk *walk){
	enum_walk_file_list_retcode retcode;
	FRESULT ff_return_code;
	unsigned int bytes_read;
	unsigned int ui_seek_body;
	unsigned int ui_seek_footer;
	unsigned int ui_read_footer;
	ui_seek_body=0;
	ui_seek_footer=0;
	ui_read_footer=0;
	// init retcode
	retcode=enum_walk_file_list_retcode_ok;
// handles operation BEFORE
	switch(walk->op){
		case enum_walk_file_list_init:
		default:
			walk->ul_current_block_offset=0;
			walk->ul_footer_offset=sizeof(TipoStructQueueElementHeader)+walk->pBodyInfo->ul_body_size_bytes;
			walk->ul_element_size=walk->ul_footer_offset+sizeof(TipoStructQueueElementFooter);
			ui_seek_footer=1;
			ui_read_footer=1;
			break;
		case enum_walk_file_list_next:
			walk->ul_current_block_offset=walk->footer.ul_next_block_offset;
			ui_seek_footer=1;
			ui_read_footer=1;
			break;
        case enum_walk_file_list_seek:
			ui_seek_footer=1;
			ui_read_footer=1;
			break;		
        case enum_walk_file_list_prev:
			walk->ul_current_block_offset=walk->footer.ul_prev_block_offset;
			ui_seek_footer=1;
			ui_read_footer=1;
			break;
		case enum_walk_file_list_get_body:
        case enum_walk_file_list_put_body:
			ui_seek_body=1;
			break;
	}
	if (ui_seek_body){
		// seek to the body of the current block
		ff_return_code = f_lseek (walk->pfil,walk->ul_current_block_offset+sizeof(TipoStructQueueElementHeader));
		// has seek completed ok?
		if (ff_return_code){
			retcode=enum_walk_file_list_retcode_seek_error;
			goto end_of_procedure;
		}
	}
	if (ui_seek_footer){
		// seek to the footer of the current block
		ff_return_code = f_lseek (walk->pfil,walk->ul_current_block_offset+walk->ul_footer_offset);
		// has seek completed ok?
		if (ff_return_code){
			retcode=enum_walk_file_list_retcode_seek_error;
			goto end_of_procedure;
		}
	}
	if (ui_read_footer){
		// read footer
		ff_return_code = f_read(walk->pfil, (char*)&walk->footer, sizeof(walk->footer), &bytes_read);
		if (ff_return_code||(bytes_read!=sizeof(walk->footer))){
			retcode=enum_walk_file_list_retcode_read_error;
			goto end_of_procedure;
		}
	}
// handles operation AFTER
	switch(walk->op){
		case enum_walk_file_list_init:
		default:
			// empty queue???
			if (walk->footer.ul_next_block_offset==0){
				retcode=enum_walk_file_list_retcode_end_of_list;
			}
			break;
		case enum_walk_file_list_next:
		case enum_walk_file_list_prev:
		case enum_walk_file_list_seek:
			if (walk->ul_current_block_offset==0){
				retcode=enum_walk_file_list_retcode_end_of_list;
			}
			break;
        case enum_walk_file_list_put_body:
            {
                unsigned int ui_crc;
                unsigned int bytes_written;
                // calculate crc            
                ui_crc=ui_file_list_crc((char*)walk->pBody, walk->pBodyInfo->ul_body_size_bytes);
                // write body
                ff_return_code = f_write(walk->pfil, (char*)walk->pBody, walk->pBodyInfo->ul_body_size_bytes, &bytes_written);
                if (ff_return_code||(bytes_written!=walk->pBodyInfo->ul_body_size_bytes)){
                    retcode=enum_walk_file_list_retcode_write_error;
                    goto end_of_procedure;
                }
                // read the footer, I am already in the correct seek position... the footer is just after the body
                ff_return_code = f_read(walk->pfil, (char*)&walk->footer, sizeof(walk->footer), &bytes_read);
                if (ff_return_code||(bytes_read!=sizeof(walk->footer))){
                    retcode=enum_walk_file_list_retcode_read_error;
                    goto end_of_procedure;
                }   
                // update crc field
                walk->footer.ui_body_crc=~ui_crc;		// the crc of the body is the 1-complement f the calculated crc
                // seek again to the beginning of the footer of the current block
                ff_return_code = f_lseek (walk->pfil,walk->ul_current_block_offset+walk->ul_footer_offset);
                // has seek completed ok?
                if (ff_return_code){
                    retcode=enum_walk_file_list_retcode_seek_error;
                    goto end_of_procedure;
                }            
                // write down the footer
                ff_return_code = f_write(walk->pfil, (char*)&walk->footer, sizeof(walk->footer), &bytes_written);
                if (ff_return_code||(bytes_written!=sizeof(walk->footer))){
                    retcode=enum_walk_file_list_retcode_write_error;
                    goto end_of_procedure;
                }   
                break;
            }
        
		case enum_walk_file_list_get_body:
            if (!walk->pBody){
				retcode=enum_walk_file_list_retcode_null_ptr_body_error;
				goto end_of_procedure;
            }
			// read body
			ff_return_code = f_read(walk->pfil, (char*)walk->pBody, walk->pBodyInfo->ul_body_size_bytes, &bytes_read);
			if (ff_return_code||(bytes_read!=walk->pBodyInfo->ul_body_size_bytes)){
				retcode=enum_walk_file_list_retcode_read_error;
				goto end_of_procedure;
			}
// crc verification
			{
				unsigned int ui_crc;
				TipoStructQueueElementFooter footer; 
				// calculate crc of the body
				ui_crc=ui_file_list_crc(walk->pBody, walk->pBodyInfo->ul_body_size_bytes);
				// read the footer
				ff_return_code = f_read(walk->pfil, (char*)&footer, sizeof(footer), &bytes_read);
                if (ff_return_code||(bytes_read!=sizeof(footer))){
                    retcode=enum_walk_file_list_retcode_read_error;
                    goto end_of_procedure;
                }   
				// check crc
				if (footer.ui_body_crc!=~ui_crc){
					retcode=enum_walk_file_list_retcode_crc_error;
                    goto end_of_procedure;
				}
			}
			break;
	}
// always execute a sync to flush buffers!
	f_sync(walk->pfil);

end_of_procedure:
	return retcode;
}//enum_walk_file_list_retcode	walk_file_list(enum_walk_file_list op)


int close_file_list(FIL *f){
	#ifndef _MSC_VER
		f_close(f);
	#else
		fclose(f);
	#endif
	return 0;
}//int close_file_list(FIL *f)


// file list registry single element
typedef struct _TipoStructFileListRegistry{
    const char *pc_filename;
    FIL f;
    TipoStructBodyInfo body_info;
    TipoStructQueueListWalk walk;
    unsigned long ul_offset_last_block_inserted; // offset of the last block inserted
    unsigned long ul_offset_last_block_found; // offset of the last block found
}TipoStructFileListRegistry;

// max entry in file list registry
#define def_max_num_entry_file_list_registry 16
// struct to handle file list registry
typedef struct _TipoStructHandleFileListRegistry{
    unsigned int ui_num_entries;
    TipoStructFileListRegistry list[def_max_num_entry_file_list_registry];
}TipoStructHandleFileListRegistry;

// variable to handle file list registry
TipoStructHandleFileListRegistry handle_file_list_registry;

// initialize file list registry
void v_init_file_list_registry(void){
    memset(&handle_file_list_registry,0,sizeof(handle_file_list_registry));
}

//
// registers a new entry in file list registry
// returns: 
//    enum_retcode_file_list_register_ok if ok, else error code
//    *pui_handle contains handle of the entry if ok,def_file_list_invalid_handle else
//
enum_retcode_file_list_register file_list_register(const char *pc_filename,TipoStructBodyInfo *p_body_info, unsigned int * pui_handle){
    TipoStructFileListRegistry *p;
    enum_retcode_file_list_register retcode;
//by now,all ok
    retcode=enum_retcode_file_list_register_ok;
// too many registered files?
    if (handle_file_list_registry.ui_num_entries>=sizeof(handle_file_list_registry.list)/sizeof(handle_file_list_registry.list[0])){
        retcode=enum_retcode_file_list_register_too_many_files;
        goto end_of_procedure;
    }
// get ptr to next free slot
    p=&handle_file_list_registry.list[handle_file_list_registry.ui_num_entries];
// copy slot infos
    memcpy(&(p->body_info),p_body_info,sizeof(p->body_info));
    p->pc_filename=pc_filename;
// save handle
    *pui_handle=handle_file_list_registry.ui_num_entries++;
// try to open in read mode the file list
    switch(open_file_list((char*)p->pc_filename,&p->f,&(p->body_info))){
        case enum_open_file_list_retcode_ok:
            break;
        default:           
            // now, try to open the file list in write mode...
            if (create_file_list((char*)p->pc_filename,&p->f,&(p->body_info))==enum_create_file_list_retcode_ok){
                break;
            }
            retcode=enum_retcode_file_list_register_unable_to_open_file;
            goto end_of_procedure;
    }//switch

end_of_procedure:
    // if not ok, returns invalid handle
    if (retcode!=enum_retcode_file_list_register_ok){
        *pui_handle=def_file_list_invalid_handle;
    }
    return retcode;
}//enum_retcode_file_list_register file_list_register(char *pc_filename,TipoStructBodyInfo *p_body_info)


// insert an element in the file list, given handle
enum_insert_reg_file_list_retcode insert_reg_file_list(unsigned int ui_handle,char *pBody){
    TipoStructFileListRegistry *p;
    enum_insert_reg_file_list_retcode retcode;
    // by now, retcode is ok
    retcode=enum_insert_reg_file_list_ok;
    // valid handle???
    if (ui_handle>=handle_file_list_registry.ui_num_entries){
        retcode=enum_insert_reg_file_list_invalid_handle;
        goto end_of_procedure;
    }
// get ptr to the free slot
    p=&handle_file_list_registry.list[ui_handle];
// insertion ok???
    switch(insert_file_list(&(p->f),pBody,&(p->body_info),&p->ul_offset_last_block_inserted)){
        case enum_insert_file_list_retcode_ok:
            break;
        case enum_insert_file_list_retcode_already_exists:
            retcode=enum_insert_reg_file_list_already_exists;
            p->ul_offset_last_block_found=ul_offset_last_element_found;
            goto end_of_procedure;
        case enum_insert_file_list_retcode_unable_to_insert:
        default:
            retcode=enum_insert_reg_file_list_unable_to_insert;
            goto end_of_procedure;
            
    
    }
end_of_procedure:
    return retcode;
}//enum_insert_reg_file_list_retcode insert_reg_file_list(unsigned int ui_handle,char *pBody)



// delete an element from the file list, given handle and body pointer
enum_delete_reg_file_list_retcode delete_reg_file_list(unsigned int ui_handle,char *pBody){
    TipoStructFileListRegistry *p;
    enum_delete_reg_file_list_retcode retcode;
    // by now, retcode is ok
    retcode=enum_delete_reg_file_list_ok;
    // valid handle???
    if (ui_handle>=handle_file_list_registry.ui_num_entries){
        retcode=enum_delete_reg_file_list_invalid_handle;
        goto end_of_procedure;
    }
// get ptr to the free
    p=&handle_file_list_registry.list[ui_handle];
// deletion ok???
    switch(delete_file_list(&(p->f),pBody,&(p->body_info))){
        case enum_delete_file_list_retcode_ok:
            break;
        case enum_delete_file_list_retcode_unable_to_delete:
        default:
            retcode=enum_delete_reg_file_list_unable_to_delete;
            goto end_of_procedure;
            
    
    }
end_of_procedure:
    return retcode;
}//enum_delete_reg_file_list_retcode delete_reg_file_list(unsigned int ui_handle,char *pBody)

// close all of the registered file list
void v_close_reg_file_list(void){
    unsigned int i;
    for (i=0;i<handle_file_list_registry.ui_num_entries;i++){
        close_file_list(&(handle_file_list_registry.list[i].f));
    }
    // give a reinit to the registry!
    v_init_file_list_registry();
}//void v_close_reg_file_list(void)

// get the identifier of the block involved in operations like (write, read, find)
// returns 0xFFFFFFF if invalid handle
unsigned long ul_reg_file_cur_id(unsigned int ui_handle, enum_reg_file_id_required reg_file_id_required){
    unsigned long ul;
    TipoStructFileListRegistry *p;    
    ul=0xFFFFFFFF;
// valid handle???
    if (ui_handle<handle_file_list_registry.ui_num_entries){
        p=&handle_file_list_registry.list[ui_handle];    
        switch(reg_file_id_required){
            case enum_reg_file_id_required_last_block_inserted:
                ul=p->ul_offset_last_block_inserted;
                break;
            case enum_reg_file_id_required_last_block_walked:
                ul=p->walk.ul_current_block_offset;
                break;
            case enum_reg_file_id_required_last_block_found:
                ul=p->ul_offset_last_block_found;
                break;
            default:
                break;            
        }
    }
    return ul;
}//unsigned long ul_reg_file_cur_id(unsigned int ui_handle)

// backup file list retcodes
typedef enum{
	enum_backup_file_list_retcode_ok=0,
	enum_backup_file_list_retcode_numof
}enum_backup_file_list_retcode;


// walks through the file...
enum_walk_reg_file_list_retcode walk_reg_file_list(unsigned int ui_handle, enum_walk_reg_file_list_op op,TipoUnionParamWalkReg u){
    enum_walk_reg_file_list_retcode retcode;
    enum_walk_file_list_retcode walk_retcode;
    TipoStructFileListRegistry *p;                
// init retcode to ok
    retcode=enum_walk_reg_file_list_retcode_ok;
// valid handle???
    if (ui_handle>=handle_file_list_registry.ui_num_entries){
        retcode=enum_walk_reg_file_list_retcode_invalid_handle;
        goto end_of_procedure;
    }
// init handles list pointer
    p=&handle_file_list_registry.list[ui_handle];    
// which op do you require?
    switch(op){
        case enum_walk_reg_file_list_init:
            {
                p->walk.pBody=u.pBody;
                p->walk.pBodyInfo=&(p->body_info);
                p->walk.pfil=&(p->f);
                p->walk.op=enum_walk_file_list_init;
                walk_retcode=walk_file_list(&p->walk);
                if (walk_retcode!=enum_walk_file_list_retcode_ok){
                    retcode=enum_walk_reg_file_list_retcode_error;
                    goto end_of_procedure;            
                }
                break;
            }
        case enum_walk_reg_file_list_next:
            p->walk.op=enum_walk_file_list_next;
            walk_retcode=walk_file_list(&p->walk);
            switch(walk_retcode){
                case enum_walk_file_list_retcode_ok:
                    break;
                case enum_walk_file_list_retcode_end_of_list:
                    retcode=enum_walk_reg_file_list_retcode_end_of_list;
                    break;  
                default:
                    retcode=enum_walk_reg_file_list_retcode_error;
                    goto end_of_procedure;                             
            }
            break;
        case enum_walk_reg_file_list_prev:
            p->walk.op=enum_walk_file_list_prev;
            walk_retcode=walk_file_list(&p->walk);
            switch(walk_retcode){
                case enum_walk_file_list_retcode_ok:
                    break;
                case enum_walk_file_list_retcode_end_of_list:
                    retcode=enum_walk_reg_file_list_retcode_end_of_list;
                    break;  
                default:
                    retcode=enum_walk_reg_file_list_retcode_error;
                    goto end_of_procedure;                             
            }
            break;          
        case enum_walk_reg_file_list_get_body:
			p->walk.op=enum_walk_file_list_get_body;
			// importante!!!
			p->walk.pBody=u.pBody;
            walk_retcode=walk_file_list(&p->walk);
            if (walk_retcode!=enum_walk_file_list_retcode_ok){
                retcode=enum_walk_reg_file_list_retcode_error;
                goto end_of_procedure;            
            }
            break;
        case enum_walk_reg_file_list_put_body:
			p->walk.op=enum_walk_file_list_put_body;
            walk_retcode=walk_file_list(&p->walk);
            if (walk_retcode!=enum_walk_file_list_retcode_ok){
                retcode=enum_walk_reg_file_list_retcode_error;
                goto end_of_procedure;            
            }
            break;
        case enum_walk_reg_file_list_seek:
			p->walk.op=enum_walk_file_list_seek;
            p->walk.ul_current_block_offset=u.ul_offset;
            walk_retcode=walk_file_list(&p->walk);
            switch(walk_retcode){
                case enum_walk_file_list_retcode_ok:
                    break;
                case enum_walk_file_list_retcode_end_of_list:
                    retcode=enum_walk_reg_file_list_retcode_end_of_list;
                    break;  
                default:
                    retcode=enum_walk_reg_file_list_retcode_error;
                    goto end_of_procedure;                             
            }
            break;        
		default:
		{
			break;
		}
    }

end_of_procedure:
    return retcode;
}//enum_walk_reg_file_list_retcode walk_reg_file_list(unsigned int ui_handle, enum_walk_reg_file_list_op op,char *pBody){



// file list test


#ifdef _MSC_VER

// list of bodies to insert
char *list_body_to_insert[]={
		 "TEST "
		,"PROVA"
		,"ABCDE"
		,"ZZZZZ"
		,"AAAAA"
	};

// test routine
int __cdecl v_test_file_list(void){
	FILE *f;
	char body[32];
	TipoStructBodyInfo bodyInfo;
	TipoStructQueueListWalk walk;

// body initialize
	memset(&body[0],0,sizeof(body));

// body info initialize
	bodyInfo.ul_body_size_bytes=sizeof(body);
	bodyInfo.ul_key_offset_in_body_bytes=0;
	bodyInfo.ul_key_size_bytes=20;

// create list
	{
		switch(create_file_list("file_list.bin",&f,&bodyInfo)){
			case enum_create_file_list_retcode_ok:
				break;
			default:
				return 1;
		}
	}

// insert elements in list
	{
		int i;
		for (i=0;i<sizeof(list_body_to_insert)/sizeof(list_body_to_insert[0]);i++){
			strncpy_s(body,sizeof(body)-1,list_body_to_insert[i],5);
			switch (insert_file_list(f,body,&bodyInfo,NULL)){
				case enum_insert_file_list_retcode_ok:
					break;
				default:
					return 2;
			}
		}
	}
	
// walking through the list	
	{
		enum_walk_file_list_retcode walk_retcode;

	// init walk structure
		walk.pBody=body;
		walk.pBodyInfo=&bodyInfo;
		walk.pfil=f;
	// walk init
		walk.op=enum_walk_file_list_init;
		while(1){
			walk_retcode=walk_file_list(&walk);
			switch(walk_retcode){
				case enum_walk_file_list_retcode_ok:
					if (walk.op==enum_walk_file_list_init){
						printf("list init successful\r\n");
					}
					else{
						walk.op=enum_walk_file_list_get_body;
						walk_retcode=walk_file_list(&walk);
						if (walk_retcode==enum_walk_file_list_retcode_ok){
							printf("\tbody:<%s>\r\n",walk.pBody);
						}
						else{
							printf("unable to get body\r\n");
							return 5;
						}
					}
					break;
				case enum_walk_file_list_retcode_end_of_list:
					printf("end of list\r\n");
					break;
				default:
					printf("walking error\r\n");
					return 4;
			}
			// end of list
			if (walk_retcode==enum_walk_file_list_retcode_end_of_list)
				break;
			// walk next
			walk.op=enum_walk_file_list_next;
		}
	
	}
	// element deletion
	{
		enum_delete_file_list_retcode delete_retcode;
		strncpy_s(body,sizeof(body)-1,list_body_to_insert[4],5);
		printf("deleting element <%s>\r\n",body);
		delete_retcode=delete_file_list(f,body,&bodyInfo);
		switch (delete_retcode){
			case enum_delete_file_list_retcode_ok:
				printf("deletion ok\r\n");
				break;
			case enum_delete_file_list_retcode_unable_to_delete:
			default:
				printf("error deleting\r\n");
				break;
		} 
	}

// walking through the list	
	{
		enum_walk_file_list_retcode walk_retcode;

	// init walk structure
		walk.pBody=body;
		walk.pBodyInfo=&bodyInfo;
		walk.pfil=f;
	// walk init
		walk.op=enum_walk_file_list_init;
		while(1){
			walk_retcode=walk_file_list(&walk);
			switch(walk_retcode){
				case enum_walk_file_list_retcode_ok:
					if (walk.op==enum_walk_file_list_init){
						printf("list init successful\r\n");
					}
					else{
						walk.op=enum_walk_file_list_get_body;
						walk_retcode=walk_file_list(&walk);
						if (walk_retcode==enum_walk_file_list_retcode_ok){
							printf("\tbody:<%s>\r\n",walk.pBody);
						}
						else{
							printf("unable to get body\r\n");
							return 5;
						}
					}
					break;
				case enum_walk_file_list_retcode_end_of_list:
					printf("end of list\r\n");
					break;
				default:
					printf("walking error\r\n");
					return 4;
			}
			// end of list
			if (walk_retcode==enum_walk_file_list_retcode_end_of_list)
				break;
			// walk next
			walk.op=enum_walk_file_list_next;
		}
	
	}
// close the list
	if (close_file_list(f)){
		return 3;
	}
// end of program, all ok
	printf("end of program, all ok\r\n");
	return 0;

}


void main(void){
	v_test_file_list();
}
#endif



#ifndef _MSC_VER

// list of bodies to insert
char *list_body_to_insert[]={
		 "TEST "
		,"PROVA"
		,"ABCDE"
		,"ZZZZZ"
		,"AAAAA"
	};

FIL f_test;
char body[32];
TipoStructBodyInfo bodyInfo;
TipoStructQueueListWalk walk;

// test routine
int v_test_file_list(void){

#if 0
        FRESULT rc;
        FIL myf;
        rc = f_open(&(myf), "HELLO_HOW_ARE_YOU.TXT", FA_WRITE | FA_CREATE_ALWAYS);
        if (rc) {
            goto end_of_procedure;
        }

        {
            char *p_text="Lorem ipsum dolor sit amet, consectetur adipiscing elit.\r\n";
            int bytes_written;
            rc = f_write(&(myf), p_text, strlen(p_text), &bytes_written);
        }
        if (rc) {
            
            goto end_of_procedure;
        }
        rc = f_close(&(myf));
        if (rc) {
            
            goto end_of_procedure;
        }
end_of_procedure:
#endif

// body initialize
	memset(&body[0],0,sizeof(body));

// body info initialize
	bodyInfo.ul_body_size_bytes=sizeof(body);
	bodyInfo.ul_key_offset_in_body_bytes=0;
	bodyInfo.ul_key_size_bytes=20;

// create list
	{
		switch(create_file_list("file_list.bin",&f_test,&bodyInfo)){
			case enum_create_file_list_retcode_ok:
				break;
			default:
				return 1;
		}
	}

// insert elements in list
	{
		int i;
		for (i=0;i<sizeof(list_body_to_insert)/sizeof(list_body_to_insert[0]);i++){
			strncpy(body,list_body_to_insert[i],sizeof(body)-1);
			switch (insert_file_list(&f_test,body,&bodyInfo,NULL)){
				case enum_insert_file_list_retcode_ok:
					break;
				default:
					return 2;
			}
		}
	}
	
// walking through the list	
	{
		enum_walk_file_list_retcode walk_retcode;

	// init walk structure
		walk.pBody=body;
		walk.pBodyInfo=&bodyInfo;
		walk.pfil=&f_test;
	// walk init
		walk.op=enum_walk_file_list_init;
		while(1){
			walk_retcode=walk_file_list(&walk);
			switch(walk_retcode){
				case enum_walk_file_list_retcode_ok:
					if (walk.op==enum_walk_file_list_init){
						//printf("list init successful\r\n");
					}
					else{
						walk.op=enum_walk_file_list_get_body;
						walk_retcode=walk_file_list(&walk);
						if (walk_retcode==enum_walk_file_list_retcode_ok){
							//printf("\tbody:<%s>\r\n",walk.pBody);
						}
						else{
							//printf("unable to get body\r\n");
							return 5;
						}
					}
					break;
				case enum_walk_file_list_retcode_end_of_list:
					//printf("end of list\r\n");
					break;
				default:
					//printf("walking error\r\n");
					return 4;
			}
			// end of list
			if (walk_retcode==enum_walk_file_list_retcode_end_of_list)
				break;
			// walk next
			walk.op=enum_walk_file_list_next;
		}
	
	}
	// element deletion
	{
		enum_delete_file_list_retcode delete_retcode;
		strncpy(body,list_body_to_insert[4],sizeof(body)-1);
		//printf("deleting element <%s>\r\n",body);
		delete_retcode=delete_file_list(&f_test,body,&bodyInfo);
		switch (delete_retcode){
			case enum_delete_file_list_retcode_ok:
				//printf("deletion ok\r\n");
				break;
			case enum_delete_file_list_retcode_unable_to_delete:
			default:
				//printf("error deleting\r\n");
				break;
		} 
	}


// walking through the list	
	{
		enum_walk_file_list_retcode walk_retcode;

	// init walk structure
		walk.pBody=body;
		walk.pBodyInfo=&bodyInfo;
		walk.pfil=&f_test;
	// walk init
		walk.op=enum_walk_file_list_init;
		while(1){
			walk_retcode=walk_file_list(&walk);
			switch(walk_retcode){
				case enum_walk_file_list_retcode_ok:
					if (walk.op==enum_walk_file_list_init){
						//printf("list init successful\r\n");
					}
					else{
						walk.op=enum_walk_file_list_get_body;
						walk_retcode=walk_file_list(&walk);
						if (walk_retcode==enum_walk_file_list_retcode_ok){
							//printf("\tbody:<%s>\r\n",walk.pBody);
						}
						else{
							//printf("unable to get body\r\n");
							return 5;
						}
					}
					break;
				case enum_walk_file_list_retcode_end_of_list:
					//printf("end of list\r\n");
					break;
				default:
					//printf("walking error\r\n");
					return 4;
			}
			// end of list
			if (walk_retcode==enum_walk_file_list_retcode_end_of_list)
				break;
			// walk next
			walk.op=enum_walk_file_list_next;
		}
	
	}
// close the list
	if (close_file_list(&f_test)){
		return 3;
	}
// end of program, all ok
//	printf("end of program, all ok\r\n");
	return 0;

}


#endif
