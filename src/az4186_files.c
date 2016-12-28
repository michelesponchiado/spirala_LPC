#include "LPC23xx.h"                       /* LPC23xx/24xx definitions */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
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
#include "az4186_files.h"
#include "spityp.h"
#include "spiext.h"


// the name of lcd calibration file
#define def_file_name_lcd_calibration "lcd_calibration.bin"
// the name of mac params file
#define def_file_name_mac_params "mac_params.bin"
// backup directory
#define def_backup_dir "/Backup"
// backup suffix
#define def_backup_suffix "_bak"
// max number of backup files..
#define def_max_backup_files 5
// indice base di backup: primo backup ha indice 0 o 1?
#define def_idx_base_backup 1
// maximum index 
#define def_max_backup_idx 999


// write down single obj file
static unsigned int ui_write_single_object_file(char *pc_filename,char *pbytes, unsigned int ui_num_bytes_2_write){
	FIL fOut;
	unsigned int ui_num_bytes_written;
	unsigned int ui_retcode;
// init
	ui_retcode=0;

// backup the lcd calibration?	
// open for writing	
	if (f_open(&fOut,pc_filename,FA_WRITE | FA_CREATE_ALWAYS | FA_READ)){
		goto end_of_procedure;
	}
	// write down bytes
	f_write(&fOut,pbytes,ui_num_bytes_2_write,&ui_num_bytes_written);
// writing ok?	
	if (ui_num_bytes_written!=ui_num_bytes_2_write){
		goto end_of_procedure;
	}
	// all ok, probably
	ui_retcode=1;
end_of_procedure:	
	f_close(&fOut);
	return ui_retcode;
}//static unsigned int ui_write_single_object_file(char *pc_filename,char *pbytes, unsigned int ui_num_bytes_2_write)

// read single obj file
unsigned int ui_read_single_object_file(char *pc_filename,char *pbytes, unsigned int ui_num_bytes_2_read){
	FIL fIn;
	unsigned int ui_num_bytes_read;
	unsigned int ui_retcode;
// init
	ui_retcode=0;
// open for read	
	if (f_open(&fIn,pc_filename, FA_OPEN_EXISTING | FA_READ)){
		goto end_of_procedure;
	}
	// read down bytes
	f_read(&fIn,pbytes,ui_num_bytes_2_read,&ui_num_bytes_read);
// read ok?	
	if (ui_num_bytes_read!=ui_num_bytes_2_read){
		goto end_of_procedure;
	}
	// all ok, probably
	ui_retcode=1;
end_of_procedure:	
	f_close(&fIn);
	return ui_retcode;
}//unsigned int ui_read_single_object_file(char *pc_filename,char *pbytes, unsigned int ui_num_bytes_2_read)

#define defMaxCharDrvName 1
#define defMaxCharDirName 128
#define defMaxCharFileName 256
#define defMaxCharFileExt 12

unsigned int fileSystem_uiEqualJollyFileName(char *pcFileName,char *pcJollyFileName){
	typedef enum {
					enumStatoJollyCtrlName=0,
					enumStatoJollyCtrlExt,
					enumStatoJollyCtrl_numof
				}enumTipoStatoJollyCtrl;
	enumTipoStatoJollyCtrl statoJolly;

	char *pName,*pJolly;
	pName=pcFileName;
	pJolly=pcJollyFileName;
	statoJolly=enumStatoJollyCtrlName;
	while(1){
		// troppi caratteri esaminati??? esco e ritorno 0
		if (pJolly-pcJollyFileName>(defMaxCharFileName+defMaxCharFileExt+1+1))
			break;
		switch(*pJolly){
			// *-->se sono sull'estensione, tutto ok sono uguali
			//     se sono sul filename, il filename � ok devo passare all'estensione
			case '*':
				// se estensione uguale, i files hanno lo stesso nome
				if(statoJolly==enumStatoJollyCtrlExt){
					return 1;
				}
				// passo a verificare l'estensione
				pJolly++;
				// proseguo finch� trovo il carattere '.' o fine stringa
				while(*pJolly&&(*pJolly!='.')){
					pJolly++;
				}
				// se ho trovato l'estensione la salto e passo al prossimo carattere
				if (*pJolly=='.'){
					pJolly++;
				}
				// cerco l'estensione anche nel nome file
				while(*pName&&(*pName!='.')){
					pName++;
				}
				// se ho trovato l'estensione pasos al carattere successivo
				if (*pName=='.'){
					pName++;
				}
				// mi metto nello stato di controllo estensione				
				statoJolly=enumStatoJollyCtrlExt;
				break;
			// ?-->devo trovare un carattere valido
			case '?':
				// devo trovare un carattere valido nel nome del file!!!
				if ((*pName=='.')||(!*pName)){
					return 0;
				}
				pName++;
				pJolly++;
				break;
			// '\0'-->fine stringa jolly
			case 0:
				// se filename non � finito, sono diversi
				if (*pName)
					return 0;
				return 1;
			// '.'-->passaggio da nome ad estensione
			case '.':
				if (*pName!='.'){
					return 0;
				}
				pName++;
				pJolly++;
				// mi metto nello stato di controllo estensione				
				statoJolly=enumStatoJollyCtrlExt;
				break;
			// carattere "normale"
			default:
				// se i due caratteri non sono uguali, i nomi sono diversi
				if (toupper(*pName)!=toupper(*pJolly)){
					return 0;
				}
				pName++;
				pJolly++;
				break;
		}
		
	}
	return 0;
}

// copy f_src to f_dst, returns 1 if copy ok, else returns 0
unsigned int ui_file_copy(FIL *pf_dst, FIL *pf_src){
	char cbuffer[256];
	unsigned int ui_num_bytes_read, retcode,ui_num_bytes_written;
	retcode=1;
	while(!f_eof(pf_src)){
		f_read(pf_src,cbuffer,sizeof(cbuffer),&ui_num_bytes_read);
		if (!ui_num_bytes_read){
			retcode=0;
			break;
		}
		f_write(pf_dst,cbuffer,ui_num_bytes_read,&ui_num_bytes_written);
        if (ui_num_bytes_written!=ui_num_bytes_read){
			retcode=0;
			break;
		}
	}
	return retcode;
}//unsigned int ui_file_copy(FIL *pf_dst, FIL *pf_src)	

// copy f_src to f_dst, returns 1 if copy ok, else returns 0
unsigned int ui_f_copy(char *dst, char *src){
	FIL fin, fout;
	unsigned int retcode;
	
	retcode=0;
	
	if (f_open(&fin,src, FA_OPEN_EXISTING | FA_READ))
		goto end_of_procedure;
	if (f_open(&fout,dst,FA_WRITE | FA_CREATE_ALWAYS | FA_READ))
		goto end_of_procedure;
	if (!ui_file_copy(&fout,&fin))
		goto end_of_procedure;
	retcode=1;
end_of_procedure:
	f_close(&fout);
    f_close(&fin);
	return retcode;
}//unsigned int ui_f_copy(char *dst, char *src)			

// operations we can do on directory explore
typedef enum{
	enum_explore_dir_op_open=0,
	enum_explore_dir_op_close,
	enum_explore_dir_op_find,
	enum_explore_dir_op_numof
}	enum_explore_dir_op;

// operations result from directory explore
typedef enum{
	enum_explore_dir_result_ok=0,
	enum_explore_dir_result_err,
	enum_explore_dir_result_complete,
	enum_explore_dir_result_numof
}enum_explore_dir_result;
	
// explore directory
enum_explore_dir_result ui_explore_dir(enum_explore_dir_op op,char *pc_dir_name, char **ppc_file_found_name){
	enum_explore_dir_result retcode;
	typedef struct _tipo_struct_explore_dir{
		DIR dirFFind;
		unsigned int ui_dir_opened;
		FILINFO fno;
	}tipo_struct_explore_dir;
	static tipo_struct_explore_dir explore_dir;
	int res;
// init
	retcode=enum_explore_dir_result_ok;
	switch(op){
		case enum_explore_dir_op_open:
			if (f_opendir(&explore_dir.dirFFind, def_backup_dir));
				retcode=enum_explore_dir_result_err;	
			explore_dir.ui_dir_opened=1;
			break;
		case enum_explore_dir_op_close:
			explore_dir.ui_dir_opened=0;
			break;
		case enum_explore_dir_op_find:
			if (!explore_dir.ui_dir_opened){
				retcode=enum_explore_dir_result_err;
			}
			res = f_readdir(&explore_dir.dirFFind, &explore_dir.fno);
			if (res != FR_OK || explore_dir.fno.fname[0] == 0){ 
				break;
			}
#if _USE_LFN
			*ppc_file_found_name = *explore_dir.fno.lfname ? explore_dir.fno.lfname : explore_dir.fno.fname;
#else
			*ppc_file_found_name = explore_dir.fno.fname;
#endif			
			break;
		default:
			retcode=enum_explore_dir_result_err;
			break;			
	}
	return retcode;
}//enum_explore_dir_result ui_explore_dir(enum_explore_dir_op op,char *pc_dir_name, char *pc_file_found_name)

typedef struct _TipoInfoBackup{
		unsigned int ui_min;
		unsigned int ui_max;
		unsigned int ui_num;
		unsigned int ui_last[def_max_backup_files];
		unsigned int ui_next;
	}TipoInfoBackup;
TipoInfoBackup info_backup;

// void v_find_info_backup(char * pc_backup_name)
void v_find_info_backup(char * pc_backup_name){
	unsigned int ui_directory_fully_explored;
	char *found_filename;
    unsigned int len_suffix;

// set suffix len
	len_suffix=strlen(def_backup_suffix);
	memset(&info_backup,0,sizeof(info_backup));
	info_backup.ui_min=0xffffffff;	
	
// open backup directory... look for next backup index
	if (ui_explore_dir(enum_explore_dir_op_open,def_backup_dir,NULL)!=enum_explore_dir_result_ok)
		goto end_of_procedure;
// look into the dir
	ui_directory_fully_explored=0;
	while (!ui_directory_fully_explored) {
		// too many files found? exit from the loop
		if (info_backup.ui_num>def_max_backup_files){
			break;
		}
		// find the next directory element
		switch (ui_explore_dir(enum_explore_dir_op_find,NULL,&found_filename)){
			case enum_explore_dir_result_ok:
				break;
			case enum_explore_dir_result_complete:
				ui_directory_fully_explored=1;
				break;
			default:
				goto end_of_procedure;
		}
		// end of directory
		if (ui_directory_fully_explored)
			break;
		
		// verifico se il nome del file trovato � tra quelli indicati in maschera
		if (!fileSystem_uiEqualJollyFileName(found_filename,pc_backup_name)){
			continue;
		}
		// at least a file has been found: increment num of files found
		info_backup.ui_num++;
		// get the progressive id
		{
			unsigned int ui_act_id;
			char *pc;
			// locate the suffix
			pc=strstr(found_filename,def_backup_suffix);
			// not found?
			if (!pc)
				continue;
			// get current backup id
			ui_act_id=atoi(pc+len_suffix);
			// less than current minimum or bigger than current maximum? save
			if (ui_act_id>info_backup.ui_max)
				info_backup.ui_max=ui_act_id;
			else if (ui_act_id<info_backup.ui_min)
				info_backup.ui_min=ui_act_id;
			// insert in list if bigger n
			{
				int i,imin,idxmin;
				// is ghe current id bigger than the ones in list?
				for (i=0;i<def_max_backup_files;i++){
					if (ui_act_id>info_backup.ui_last[i]){
						break;
					}
				}
				// get index of minimum element in list
				imin=0xffffffff;
				idxmin=0xffffffff;
				for (i=0;i<def_max_backup_files;i++){
					if (info_backup.ui_last[i]<imin){
						idxmin=i;
						imin=info_backup.ui_last[i];
					}
				}
				// minimum found?
				if (idxmin!=0xffffffff){
					// save index
					info_backup.ui_last[idxmin]=ui_act_id;
				}
			}
		}
	}
end_of_procedure:	
	// end of exploration...
	ui_explore_dir(enum_explore_dir_op_close,NULL,NULL);	
}//void v_find_info_backup(char * pc_backup_name)
	
// restore...
// locate the maximum index backup file
// copy the found file to the default one
unsigned int ui_restore_file(char *pc_filename){
	unsigned int retcode;
    char *pc_backup_name;
    #define def_size_malloc_string 256
	retcode=0;
	pc_backup_name=malloc(def_size_malloc_string);
	if (!pc_backup_name)
		goto end_of_procedure;
// creates backup name mask (eg filename-->filename_bak000 etc)
	snprintf(pc_backup_name,def_size_malloc_string,"%s%s*",pc_filename,def_backup_suffix);
// find backup info...	
	v_find_info_backup(pc_backup_name);
// no backup files available?	
	if (info_backup.ui_num==0){
		goto end_of_procedure;
	}
// get destination filename
	snprintf(pc_backup_name,def_size_malloc_string,"%s/%s%s%i",def_backup_dir,pc_filename,def_backup_suffix,info_backup.ui_max);	
// delete original file	
	f_unlink(pc_filename);
// copy backup file to needed file
	if (!ui_f_copy(pc_filename,pc_backup_name)){
		goto end_of_procedure;
	}
// restore ok	
	retcode=1;
end_of_procedure:	
	if (pc_backup_name)
		free(pc_backup_name);
	return retcode;
}

// backup a file...
// procedure:
// get the file backup name with the biggest number in the backup dir
// when maximum number of files reached in the backup dir, eliminate the files with the lowest number
// if maximum number is too big, reset numbers...
unsigned int ui_backup_file(char *pc_filename){
	char *pc_backup_name;
	unsigned int retcode;
	char *found_filename;
	#define def_size_malloc_string 256
	
	unsigned int len_suffix;
    unsigned int ui_directory_fully_explored;
	
// init...
	retcode=0;
	
	len_suffix=strlen(def_backup_suffix);
// alloc space for name
	pc_backup_name=malloc(def_size_malloc_string);
	if (!pc_backup_name)
		goto end_of_procedure;
// creates backup name mask (eg filename-->filename_bak000 etc)
	snprintf(pc_backup_name,def_size_malloc_string,"%s%s*",pc_filename,def_backup_suffix);
// find backup info...	
	v_find_info_backup(pc_backup_name);
	// possible cases:
	// no backup files
	// some backup files
	// too many backup files
	// too big last index
	
	// no backup files
	if (info_backup.ui_num==0){
		// start with index base
		info_backup.ui_next=def_idx_base_backup;
	}
	else{
		// too big index?
		if (info_backup.ui_max>=def_max_backup_idx){
			int i,imin,idxmin;
			FIL fout,fin;
			// initialize backup index
			idxmin=def_idx_base_backup;
			// loop to free indexes
			for (i=0;i<def_max_backup_files;i++){
				// get destination filename
				snprintf(pc_backup_name,def_size_malloc_string,"%s/%s%s%i",def_backup_dir,pc_filename,def_backup_suffix,i+def_idx_base_backup);
				// delete file (if exists)
				f_unlink(pc_backup_name);
				// open file for output
				if (f_open(&fout,pc_backup_name,FA_WRITE | FA_CREATE_ALWAYS | FA_READ)) 
                    continue;
				// find minimum nonnull index
				// get index of minimum element
				imin=0xffffffff;
				idxmin=0xffffffff;
				for (i=0;i<def_max_backup_files;i++){
					// skip nonnull elements
					if (!info_backup.ui_last[i])
						continue;
					// locate minimum index
					if (info_backup.ui_last[i]<imin){
						idxmin=i;
						imin=info_backup.ui_last[i];
					}
				}
				// if minimum found
				if (idxmin!=0xffffffff){
					// get name of src file
					snprintf(pc_backup_name,def_size_malloc_string,"%s/%s%s%i",def_backup_dir,pc_filename,def_backup_suffix,idxmin);
					// open the source file
					if (f_open(&fin,pc_backup_name, FA_OPEN_EXISTING | FA_READ)==0){
						unsigned int ui_copy_ok;
						// copy to fout fin
						ui_copy_ok=ui_file_copy(&fout,&fin);
						// close fin
						f_close(&fin);
						// delete file...
						if (ui_copy_ok)
							f_unlink(pc_backup_name);
					}
					// reset the element saved
					info_backup.ui_last[idxmin]=0;
				}
				// close the output file
				f_close(&fout);
			}
			// get next index: set to base +1 to make it simple
			info_backup.ui_next=def_idx_base_backup+1;
			// cleanup all of the other files????
			// creates backup name mask (eg filename-->filename_bak000 etc)
			snprintf(pc_backup_name,def_size_malloc_string,"%s%s*",pc_filename,def_backup_suffix);
			// loop for cleanup from other unneeded files
			ui_directory_fully_explored=0;
			while(!ui_directory_fully_explored){
				if (ui_explore_dir(enum_explore_dir_op_open,def_backup_dir,NULL)!=enum_explore_dir_result_ok)
					goto end_of_procedure;					
				// look into the dir
				for (;;) {
			// find the next directory element
					switch (ui_explore_dir(enum_explore_dir_op_find,NULL,&found_filename)){
						case enum_explore_dir_result_ok:
							break;
						case enum_explore_dir_result_complete:
							ui_directory_fully_explored=1;
							break;
						default:
							goto end_of_procedure;
					}
					// end of directory
					if (ui_directory_fully_explored)
						break;					
					// verifico se il nome del file trovato � tra quelli indicati in maschera
					if (!fileSystem_uiEqualJollyFileName(found_filename,pc_backup_name)){
						continue;
					}
					// get the progressive id
					{
						unsigned int ui_act_id;
						char *pc;
						// locate the suffix
						pc=strstr(found_filename,def_backup_suffix);
						// not found?
						if (!pc)
							continue;
						// get current backup id
						ui_act_id=atoi(pc+len_suffix);	
						// if found file has an id equal or bigger than the next one, cleanup 
						if (ui_act_id>=info_backup.ui_next){
							// get full pathname
							snprintf(pc_backup_name,def_size_malloc_string,"%s/%s",def_backup_dir,found_filename);
							// delete the file
							f_unlink(found_filename);
							// reread the directory and repeat loop
							break;
						}
					}
				}
			}
		}
		else{
			// next index=maxmimum plus 1
			info_backup.ui_next=info_backup.ui_max+1;
		}
		// too many backup files?
		if (info_backup.ui_num>=def_max_backup_files){
			// get the name of the file with minimum index
			snprintf(pc_backup_name,def_size_malloc_string,"%s/%s%s%i",def_backup_dir,pc_filename,def_backup_suffix,info_backup.ui_min);
			// delete the file
			f_unlink(pc_backup_name);
		}
	}
	// finally, backup current file...
	{
		FIL fout,fin;
        int res_fin,res_fout;
		res_fin=f_open(&fin,pc_filename, FA_OPEN_EXISTING | FA_READ);
		// get the name of the backup file
		snprintf(pc_backup_name,def_size_malloc_string,"%s/%s%s%i",def_backup_dir,pc_filename,def_backup_suffix,info_backup.ui_next);
		res_fout=f_open(&fout,pc_backup_name,FA_WRITE | FA_CREATE_ALWAYS | FA_READ);
		if ((res_fout==0) && (res_fin==0)){
			ui_file_copy(&fout,&fin);
		}
		f_close(&fin);
        f_close(&fout);
	}
	
	
	retcode=1;
// end...	
end_of_procedure:
	if (pc_backup_name)
		free(pc_backup_name);
	return retcode;
}//unsigned int ui_backup_file(char *pc_filename)


// write down lcd calibration data
unsigned int ui_write_lcd_calibration_file( char *pbytes, unsigned int ui_num_bytes_2_write){
    // segnalo a tutti che il file � stato modificato...
    ucSignalFile_Modified(enumLcdCalibrationFile,1,enumFlagsFileModified_All);

    return ui_write_single_object_file(def_file_name_lcd_calibration,pbytes,ui_num_bytes_2_write);
}

// read lcd calibration data
unsigned int ui_read_lcd_calibration_file( char *pbytes, unsigned int ui_num_bytes_2_read){
    return ui_read_single_object_file(def_file_name_lcd_calibration,pbytes,ui_num_bytes_2_read);
}


// write down mac params
unsigned int ui_write_macparams_file( char *pbytes, unsigned int ui_num_bytes_2_write){
    // segnalo a tutti che il file � stato modificato...
    ucSignalFile_Modified(enumMacParamsFile,1,enumFlagsFileModified_All);

    return ui_write_single_object_file(def_file_name_mac_params,pbytes,ui_num_bytes_2_write);
}

// read mac params
unsigned int ui_read_macparams_file( char *pbytes, unsigned int ui_num_bytes_2_read){
    return ui_read_single_object_file(def_file_name_mac_params,pbytes,ui_num_bytes_2_read);
}

// funzione di lettura di un parametro privato
unsigned char *pRead_private_parameter(unsigned long ul_num_param,unsigned char *p){
	FIL fIn;
	unsigned int ui_num_bytes_read;
    unsigned int ui_num_bytes_2_read;
    unsigned int ul_seek_offset;
// init
    ui_num_bytes_2_read=sizeof(unsigned long);
    ul_seek_offset=ul_num_param*sizeof(unsigned long);

// open for read	
	if (f_open(&fIn,def_name_file_custom_private_params, FA_OPEN_EXISTING | FA_READ)){
		goto end_of_procedure;
	}
// seek
	if (f_lseek (&fIn,ul_seek_offset)){
		goto end_of_procedure;
	}

	// read down bytes
	f_read(&fIn,p,ui_num_bytes_2_read,&ui_num_bytes_read);
// read ok?	
	if (ui_num_bytes_read!=ui_num_bytes_2_read){
		goto end_of_procedure;
	}
end_of_procedure:	
	f_close(&fIn);
    // returns always p
	return p;
}


// write a custom string*20
unsigned char *pWrite_private_parameter(unsigned long ul_num_param,unsigned char *p){
	FIL fOut;
	unsigned int ui_num_bytes_written;
    unsigned int ui_num_bytes_2_write;
    unsigned int ul_seek_offset;
// init
    ui_num_bytes_2_write=sizeof(unsigned long);
    ul_seek_offset=ul_num_param*sizeof(unsigned long);

// open for read/writing	
	if (f_open(&fOut,def_name_file_custom_private_params,FA_WRITE | FA_OPEN_EXISTING | FA_READ)){
        // if doesn't exists, create always!
        if (f_open(&fOut,def_name_file_custom_private_params,FA_WRITE | FA_CREATE_ALWAYS | FA_READ)){
            goto end_of_procedure;
        }
	}
// seek
	if (f_lseek (&fOut,ul_seek_offset)){
		goto end_of_procedure;
	}
	// write down bytes
	f_write(&fOut,p,ui_num_bytes_2_write,&ui_num_bytes_written);
// writing ok?	
	if (ui_num_bytes_written!=ui_num_bytes_2_write){
		goto end_of_procedure;
	}
	// all ok, probably
end_of_procedure:	
	f_close(&fOut);
    // returns always p
	return p;
}


unsigned int ui_custom_string_read_error;

// read a custom string*20
unsigned char *pRead_custom_string20(unsigned long ulNumStringa,unsigned char *p){
	FIL fIn;
	unsigned int ui_num_bytes_read;
    unsigned int ui_num_bytes_2_read;
    unsigned int ul_seek_offset;
// init
    ui_num_bytes_2_read=20;
    ul_seek_offset=ulNumStringa*20;
	ui_custom_string_read_error=0;

// open for read	
	if (f_open(&fIn,def_name_file_custom_string_20, FA_OPEN_EXISTING | FA_READ)){
		ui_custom_string_read_error=1;
		goto end_of_procedure;
	}
// seek
	if (f_lseek (&fIn,ul_seek_offset)){
		ui_custom_string_read_error=1;
		goto end_of_procedure;
	}

	// read down bytes
	f_read(&fIn,p,ui_num_bytes_2_read,&ui_num_bytes_read);
// read ok?	
	if (ui_num_bytes_read!=ui_num_bytes_2_read){
		ui_custom_string_read_error=1;
		goto end_of_procedure;
	}
	// all ok, probably
end_of_procedure:	
	f_close(&fIn);
    // returns always p
	return p;
}

// write a custom string*20
unsigned char *pWrite_custom_string20(unsigned long ulNumStringa,unsigned char *p){
	FIL fOut;
	unsigned int ui_num_bytes_written;
    unsigned int ui_num_bytes_2_write;
    unsigned int ul_seek_offset;
// init
    ui_num_bytes_2_write=20;
    ul_seek_offset=ulNumStringa*20;

// open for read/writing	
	if (f_open(&fOut,def_name_file_custom_string_20,FA_WRITE | FA_OPEN_EXISTING | FA_READ)){
        // if doesn't exists, create always!
        if (f_open(&fOut,def_name_file_custom_string_20,FA_WRITE | FA_CREATE_ALWAYS | FA_READ)){
            goto end_of_procedure;
        }
	}
// seek
	if (f_lseek (&fOut,ul_seek_offset)){
		goto end_of_procedure;
	}
	// write down bytes
	f_write(&fOut,p,ui_num_bytes_2_write,&ui_num_bytes_written);
// writing ok?	
	if (ui_num_bytes_written!=ui_num_bytes_2_write){
		goto end_of_procedure;
	}
	// all ok, probably
end_of_procedure:	
	f_close(&fOut);
    // returns always p
	return p;
}

// read a custom string*40
unsigned char *pRead_custom_string40(unsigned long ulNumStringa,unsigned char *p){
	FIL fIn;
	unsigned int ui_num_bytes_read;
    unsigned int ui_num_bytes_2_read;
    unsigned int ul_seek_offset;
// init
    ui_num_bytes_2_read=40;
    ul_seek_offset=ulNumStringa*40;

// open for read	
	if (f_open(&fIn,def_name_file_custom_string_40, FA_OPEN_EXISTING | FA_READ)){
		ui_custom_string_read_error=1;
		goto end_of_procedure;
	}
// seek
	if (f_lseek (&fIn,ul_seek_offset)){
		ui_custom_string_read_error=1;
		goto end_of_procedure;
	}

	// read down bytes
	f_read(&fIn,p,ui_num_bytes_2_read,&ui_num_bytes_read);
// read ok?	
	if (ui_num_bytes_read!=ui_num_bytes_2_read){
		ui_custom_string_read_error=1;
		goto end_of_procedure;
	}
	// all ok, probably
end_of_procedure:	
	f_close(&fIn);
    // returns always p
	return p;
}

// write a custom string*40
unsigned char *pWrite_custom_string40(unsigned long ulNumStringa,unsigned char *p){
	FIL fOut;
	unsigned int ui_num_bytes_written;
    unsigned int ui_num_bytes_2_write;
    unsigned int ul_seek_offset;
// init
    ui_num_bytes_2_write=40;
    ul_seek_offset=ulNumStringa*40;


// backup the lcd calibration?	
// open for read/writing	
	if (f_open(&fOut,def_name_file_custom_string_40,FA_WRITE | FA_OPEN_EXISTING | FA_READ)){
        // if doesn't exists, create always!
        if (f_open(&fOut,def_name_file_custom_string_40,FA_WRITE | FA_CREATE_ALWAYS | FA_READ)){
            goto end_of_procedure;
        }
	}
// seek
	if (f_lseek (&fOut,ul_seek_offset)){
		goto end_of_procedure;
	}
	// write down bytes
	f_write(&fOut,p,ui_num_bytes_2_write,&ui_num_bytes_written);
// writing ok?	
	if (ui_num_bytes_written!=ui_num_bytes_2_write){
		goto end_of_procedure;
	}
	// all ok, probably
end_of_procedure:	
	f_close(&fOut);
    // returns always p
	return p;
}


// test di backup/restore
unsigned int ui_test_backup_restore(void){
	#define def_backup_test_filename "test.txt"
	FIL f_test;
	char cbuffer[256];
	unsigned int ui_error_restored_file;
    FATFS fatfs;

// mount file system
	f_mount(0, &fatfs);		// Register volume work area (never fails) 
// create the file	
    f_open(&f_test, def_backup_test_filename, FA_WRITE | FA_CREATE_ALWAYS | FA_READ);
	{
		int i;
        unsigned int ui_num_bytes_written;
        i=0;
		while (i<sizeof(cbuffer)){
			cbuffer[i]='0'+(i&0x0F);
            i++;
		}
        f_write(&f_test,cbuffer,sizeof(cbuffer),&ui_num_bytes_written);
		if (ui_num_bytes_written!=sizeof(cbuffer))
            return 0;
	}
	f_close(&f_test);
// test backup	
	if (!ui_backup_file(def_backup_test_filename)){
		return 0;
	}
// test restore procedure	
	{
		// empty the original file...
		f_unlink(def_backup_test_filename);
		// restore the file
		if (!ui_restore_file(def_backup_test_filename)){
			return 0;
		}	
		// read the file
		f_open(&f_test,def_backup_test_filename,FA_OPEN_EXISTING | FA_READ);
		{
			int i;
            unsigned int ui_num_bytes_read;
			ui_error_restored_file=0;
            f_read(&f_test,cbuffer,sizeof(cbuffer),&ui_num_bytes_read);
			if (ui_num_bytes_read!=sizeof(cbuffer))
                return 0;
            i=0;
			while (i<sizeof(cbuffer)){
				if (cbuffer[i]!='0'+(i&0x0F)){
					ui_error_restored_file++;
				}
                i++;
			}
            if (ui_error_restored_file)
                return 0;
		}
		f_close(&f_test);	
	}
	return 1;
}
