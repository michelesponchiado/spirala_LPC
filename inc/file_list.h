#ifndef def_file_list_h_included
#define def_file_list_h_included

#include "ff.h"


// defines an invalid handle
#define def_file_list_invalid_handle 0xFFFFFFFF

// each file list element should be described by a the body info
typedef struct _TipoStructBodyInfo{
	unsigned long ul_body_size_bytes;		// the size of the body
	unsigned long ul_key_offset_in_body_bytes;// the offset of the key in the body
	unsigned long ul_key_size_bytes;			// the size of the key in bytes
}TipoStructBodyInfo;	
// the header of the element
typedef struct _TipoStructQueueElementHeader{
	unsigned int ui_valid_entry_0xa55a3663;	// it's a valid entry only if this field has the value 0xa55a3663
	unsigned int ui_version;				// the block header version 
	TipoStructBodyInfo body_info;			// infos about the body
}TipoStructQueueElementHeader;

// the footer of the element
typedef struct _TipoStructQueueElementFooter{
	unsigned int ui_valid_entry_0xa55a3663;	// it's a valid entry only if this field has the value 0xa55a3663
	unsigned long ul_next_block_offset;	// offset of the next block; 
	unsigned long ul_prev_block_offset;	// offset of the prev block; 
	unsigned int ui_body_crc;		// the crc of the body
}TipoStructQueueElementFooter;


// retcodes from file list registration
typedef enum{
    enum_retcode_file_list_register_ok,
    enum_retcode_file_list_register_too_many_files,
    enum_retcode_file_list_register_unable_to_open_file,
    enum_retcode_file_list_register_numof
}enum_retcode_file_list_register;

// initialize file list registry
void v_init_file_list_registry(void);

//
// registers a new entry in file list registry
// returns: 
//    enum_retcode_file_list_register_ok if ok, else error code
//    *pui_handle contains handle of the entry if ok,0xFFFFFFFF else
//
enum_retcode_file_list_register file_list_register(const char *pc_filename,TipoStructBodyInfo *p_body_info, unsigned int * pui_handle);

// return codes from file list insertion
typedef enum{
	enum_insert_reg_file_list_ok=0,
	enum_insert_reg_file_list_invalid_handle,
    enum_insert_reg_file_list_unable_to_insert,
    enum_insert_reg_file_list_already_exists,
	enum_insert_reg_file_list_retcode_numof
}enum_insert_reg_file_list_retcode;

// insert an element in the file list, given handle
enum_insert_reg_file_list_retcode insert_reg_file_list(unsigned int ui_handle,char *pBody);

// return codes from file list deletion
typedef enum{
	enum_delete_reg_file_list_ok=0,
	enum_delete_reg_file_list_invalid_handle,
    enum_delete_reg_file_list_unable_to_delete,
	enum_delete_reg_file_list_retcode_numof
}enum_delete_reg_file_list_retcode;

// delete an element from the file list, given handle
enum_delete_reg_file_list_retcode delete_reg_file_list(unsigned int ui_handle,char *pBody);

typedef enum{
    enum_reg_file_id_required_last_block_inserted=0,
    enum_reg_file_id_required_last_block_walked,
    enum_reg_file_id_required_last_block_found,
    enum_reg_file_id_required_last_block_op_numof
}enum_reg_file_id_required;

// get the identifier of the block involved in operations like (write, read, find)
// returns 0xFFFFFFF if invalid handle
unsigned long ul_reg_file_cur_id(unsigned int ui_handle, enum_reg_file_id_required reg_file_id_required);

//walk file list operations admitted
typedef enum{
	enum_walk_reg_file_list_init=0,
	enum_walk_reg_file_list_next,
	enum_walk_reg_file_list_prev,
	enum_walk_reg_file_list_get_body,
    enum_walk_reg_file_list_put_body,
	enum_walk_reg_file_list_seek,
	enum_walk_reg_file_list_numof
}enum_walk_reg_file_list_op;

// walk file list retcodes
typedef enum{
	enum_walk_reg_file_list_retcode_ok=0,
	enum_walk_reg_file_list_retcode_end_of_list,
	enum_walk_reg_file_list_retcode_invalid_handle,
	enum_walk_reg_file_list_retcode_error,
	enum_walk_reg_file_list_retcode_numof
}enum_walk_reg_file_list_retcode;

typedef union _TipoUnionParamWalkReg{
    char *pBody;
    unsigned long ul_offset;
}TipoUnionParamWalkReg;

// walks through the file...
enum_walk_reg_file_list_retcode walk_reg_file_list(unsigned int ui_handle, enum_walk_reg_file_list_op op,TipoUnionParamWalkReg u);

// close all of the registered file list
void v_close_reg_file_list(void);

// get info about the body actually stored in the file
// just to execute a version check!
// returns 0 if error, 1 means all ok
unsigned int ui_get_header_body_size_file_list(char *pc_filename, TipoStructBodyInfo *pBody);

// return codes from file list creation
typedef enum{
	enum_create_file_list_retcode_ok=0,
	enum_create_file_list_retcode_invalid_key_size,
	enum_create_file_list_retcode_unable_to_open,
	enum_create_file_list_retcode_unable_to_write_header,
    enum_create_file_list_retcode_unable_to_write_body,
	enum_create_file_list_retcode_unable_to_seek,
	enum_create_file_list_retcode_unable_to_write_footer,
	enum_create_file_list_retcode_numof
}enum_create_file_list_retcode;
enum_create_file_list_retcode create_file_list(char *file_name,FIL *pfil,TipoStructBodyInfo *pBodyInfo);


// return codes from file list open
typedef enum{
    enum_open_file_list_retcode_ok,
    enum_open_file_list_retcode_invalid_key_size,
    enum_open_file_list_retcode_unable_to_open,
    enum_open_file_list_retcode_unable_to_write_header,
    enum_open_file_list_retcode_invalid_entry,
    enum_open_file_list_retcode_invalid_body_info,
    enum_open_file_list_retcode_unable_to_read_body,
    enum_open_file_list_retcode_unable_to_seek,
	enum_open_file_list_retcode_unable_to_read_footer,
	enum_open_file_list_retcode_invalid_footer
}enum_open_file_list_retcode;
enum_open_file_list_retcode open_file_list(char *file_name,FIL *pfil,TipoStructBodyInfo *pBodyInfo);

// return codes from file list insertion
typedef enum{
	enum_insert_file_list_retcode_ok=0,
	enum_insert_file_list_retcode_already_exists,
	enum_insert_file_list_retcode_unable_to_insert,
	enum_insert_file_list_retcode_numof
}enum_insert_file_list_retcode;
// insert an element in the file list
enum_insert_file_list_retcode insert_file_list(FIL *pfil,char *pBody,TipoStructBodyInfo *pBodyInfo,unsigned long *pul_offset_block_inserted);

//walk file list operations admitted
typedef enum{
	enum_walk_file_list_init=0,
	enum_walk_file_list_next,
	enum_walk_file_list_prev,
	enum_walk_file_list_get_body,
    enum_walk_file_list_put_body,
	enum_walk_file_list_seek,
	enum_walk_file_list_numof
}enum_walk_file_list;

// walk file list retcodes
typedef enum{
	enum_walk_file_list_retcode_ok=0,
	enum_walk_file_list_retcode_end_of_list,
	enum_walk_file_list_retcode_seek_error,
	enum_walk_file_list_retcode_read_error,
    enum_walk_file_list_retcode_write_error,
	enum_walk_file_list_retcode_crc_error,
	enum_walk_file_list_retcode_null_ptr_body_error,
	enum_walk_file_list_retcode_numof
}enum_walk_file_list_retcode;


typedef struct _TipoStructQueueListWalk{
	FIL *pfil;
	char *pBody;
	TipoStructBodyInfo *pBodyInfo;
	unsigned long ul_current_block_offset;
	unsigned long ul_footer_offset;
	unsigned long ul_element_size;
	TipoStructQueueElementFooter footer;
	enum_walk_file_list op;
}TipoStructQueueListWalk;


// walking file list
enum_walk_file_list_retcode	walk_file_list(TipoStructQueueListWalk *walk);

// always returns 0
int close_file_list(FIL *f);

#endif //#ifndef def_file_list_h_included
