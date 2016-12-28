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

// product codes filename
#define def_product_codes_filename "product_codes.dat"

// structure for product codes searching
TipoStructSearchCodiceProgramma scp;
// auxiliary product code structure
PROGRAMMA auxPrg;
// handle to product codes file
unsigned int ui_handle_product_codes;
TipoStructHandlePrg hprg;
TipoStruct_Spiralatrice spiralatrice;

// returns 0 if check was ok, 1 if version was upgraded, 2 if error while doing the check/upgrade
unsigned int ui_check_version_of_product_codes_file(void){
	TipoStructBodyInfo body_info_in_the_file;
	TipoStructBodyInfo body_info_I_was_expecting;
	unsigned int ui_retcode,ui_original_file_exists,ui_renamed_file_exists;
	PROGRAMMA body;
	FIL fil_original, fil_new;
	#define def_product_codes_filename_new "product_codes_new.dat"
	#define def_product_codes_filename_renamed "product_codes_ren.dat"
// init...	
	ui_retcode=0;
    body_info_I_was_expecting.ul_body_size_bytes=sizeof(PROGRAMMA);
    body_info_I_was_expecting.ul_key_offset_in_body_bytes=__builtin_offsetof(PROGRAMMA,codicePrg);
    body_info_I_was_expecting.ul_key_size_bytes=defMaxCharNomeCommessa;
// if renamed file exists but not the original one, then rename the renamed!
	// check if original file exists
	ui_original_file_exists=0;
	if (f_open(&fil_original, def_product_codes_filename, FA_WRITE | FA_OPEN_EXISTING | FA_READ)==0){
		f_close(&fil_original);
		ui_original_file_exists=1;
	}
	// check also if renamed file exists
	ui_renamed_file_exists=0;
	if (f_open(&fil_new, def_product_codes_filename_renamed, FA_WRITE | FA_OPEN_EXISTING | FA_READ)==0){
		f_close(&fil_new);
		ui_renamed_file_exists=1;
	}
	// if renamed exists but not the original, roll back!
	if (ui_renamed_file_exists&&!ui_original_file_exists){
		// roll back...
		f_rename (def_product_codes_filename_renamed,def_product_codes_filename);	
	}
	
// if I can't read the body file, big problems bro, I think the product codes file will be reinitialized...
	if (!ui_get_header_body_size_file_list(def_product_codes_filename, &body_info_in_the_file)){
		goto end_of_procedure;
	}
// if bodies are just equal, nothing to do!	
	if (memcmp(&body_info_in_the_file,&body_info_I_was_expecting,sizeof(body_info_I_was_expecting))==0){
		goto end_of_procedure;
	}
// if bodysize stored is bigger than the new one, very weird thing happened!	
	if (body_info_I_was_expecting.ul_body_size_bytes<body_info_in_the_file.ul_body_size_bytes){
		goto end_of_procedure;
	}
// else lot of things should happen...
// message: start  of procedure...
	vPrintTesto(100,4,"wait...");
	vRefreshScreen();

// open ORIGINAL file
	if (open_file_list(def_product_codes_filename,&fil_original,&body_info_in_the_file)!=enum_open_file_list_retcode_ok){
		goto end_of_procedure;
	}
// create the NEW file	
	if (create_file_list(def_product_codes_filename_new,&fil_new,&body_info_I_was_expecting) != enum_create_file_list_retcode_ok){
		goto end_of_procedure;
	}
// walking through the list	
	{
		enum_walk_file_list_retcode walk_retcode;
		unsigned int ui_heartbeat, ui_counter;
		TipoStructQueueListWalk walk;

	// init walk structure on the original file!
		walk.pBody=(char*)&body;
		walk.pBodyInfo=&body_info_in_the_file;
		walk.pfil=&fil_original;
		ui_heartbeat=0;
		ui_counter=0;
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
						// reset the whole body before read (the stored size is lower than what expected, it is better to blank)
						memset(&body,0,sizeof(body));
					
						walk.op=enum_walk_file_list_get_body;
						walk_retcode=walk_file_list(&walk);
						if (walk_retcode==enum_walk_file_list_retcode_ok){
							// I've got a body I can insert into the new file list!
							// but just upgrade the version number!
							body.ucVersionNumber=defLastPROGRAMMAversion;
							switch (insert_file_list(&fil_new,(char*)&body,&body_info_I_was_expecting,NULL)){
								case enum_insert_file_list_retcode_ok:
									break;
								default:
									ui_retcode=2;
									break;
							}
						}
						else{
							ui_retcode=2;
							//printf("unable to get body\r\n");
							break;
						}
						if (++ui_counter>=50){
							ui_counter=0;
							switch(ui_heartbeat&0x03){
								case 0:
									vPrintTesto(100,4,"wait... |");
									ui_heartbeat=0;
									break;
								case 1:
									vPrintTesto(100,4,"wait... /");
									break;
								case 2:
									vPrintTesto(100,4,"wait... -");
									break;
								case 3:
									vPrintTesto(100,4,"wait... \\");
									break;
								
							}
							ui_heartbeat++;
							vRefreshScreen();
						}
						
					}
					break;
				case enum_walk_file_list_retcode_end_of_list:
					//printf("end of list\r\n");
					break;
				default:
					ui_retcode=2;
					//printf("walking error\r\n");
					break;
			}
			// end of list
			if (walk_retcode==enum_walk_file_list_retcode_end_of_list){
				break;
			}
			// walk next
			walk.op=enum_walk_file_list_next;
		}
	
	}
	
// close both files
	close_file_list(&fil_original);
	close_file_list(&fil_new);
// if the walk was ok, proceeed with renaming	
	if (ui_retcode==0){
		// if renamed exists, delete it!
		if (ui_renamed_file_exists){
			// delete existing renamed file if it is exists!
			f_unlink (def_product_codes_filename_renamed);	
		}
	
		// now the "magick" :) rename the actual file... actual ->renamed
		if ( f_rename (	def_product_codes_filename,	def_product_codes_filename_renamed)	==FR_OK){
			// rename the new file... new->actual
			if ( f_rename (	def_product_codes_filename_new,	def_product_codes_filename)	==FR_OK){
				// all ok finally
				ui_retcode=1;
			}
			else{
				// roll back...
				f_rename (def_product_codes_filename_new,def_product_codes_filename);
			}
			
		}
	}

end_of_procedure:
	return ui_retcode;
}	

// registers a new entry in file list registry
unsigned int ui_register_product_codes(void){
    unsigned int retcode;
    TipoStructBodyInfo bodyInfo;
    enum_retcode_file_list_register retcode_file_list_register;

// init...
    retcode=0;
    bodyInfo.ul_body_size_bytes=sizeof(PROGRAMMA);
    bodyInfo.ul_key_offset_in_body_bytes=__builtin_offsetof(PROGRAMMA,codicePrg);
    bodyInfo.ul_key_size_bytes=defMaxCharNomeCommessa;
// very important routine!
// here I check the consistency of the version of the product codes file... and eventually I do the necessary adjustments...	
	ui_check_version_of_product_codes_file();
// try to register
    retcode_file_list_register=file_list_register(def_product_codes_filename,&bodyInfo,&ui_handle_product_codes);
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

// read the product code body, returns 1 if ok, 0 else
unsigned int ui_product_file_read_body(unsigned long ul_offset, char *pBody){
    TipoUnionParamWalkReg u;
    unsigned int retcode;
    enum_walk_reg_file_list_retcode walk_reg_file_list_retcode;
// init...
    retcode=0;

// walk init
    u.pBody=pBody;
    walk_reg_file_list_retcode=walk_reg_file_list(   ui_handle_product_codes
                                                    ,enum_walk_reg_file_list_init
                                                    ,u
                                                );
    if (walk_reg_file_list_retcode!=enum_walk_reg_file_list_retcode_ok){
        goto end_of_procedure;        
    }

// seek to the offset required
    u.ul_offset=ul_offset;
    walk_reg_file_list_retcode=walk_reg_file_list(   ui_handle_product_codes
                                                    ,enum_walk_reg_file_list_seek
                                                    ,u
                                                 );  
    if (walk_reg_file_list_retcode!=enum_walk_reg_file_list_retcode_ok){
        goto end_of_procedure;
    }

// reinit body pointer
    u.pBody=pBody;
// get body
    if (walk_reg_file_list(  ui_handle_product_codes
                            ,enum_walk_reg_file_list_get_body
                            ,u
                          )
                        !=enum_walk_reg_file_list_retcode_ok
        ){
        goto end_of_procedure;   
    }
// ok!
    retcode=1;
end_of_procedure:
    return retcode;
}//unsigned int ui_product_file_read_body(unsigned long ul_offset, char *pBody)


// write the product code body, returns 1 if ok, 0 else
unsigned int ui_product_file_write_body(unsigned long ul_offset, char *pBody){
    TipoUnionParamWalkReg u;
    unsigned int retcode;
    enum_walk_reg_file_list_retcode walk_reg_file_list_retcode;
// init...
    retcode=0;

// walk init
    u.pBody=pBody;
    walk_reg_file_list_retcode=walk_reg_file_list(   ui_handle_product_codes
                                                    ,enum_walk_reg_file_list_init
                                                    ,u
                                                );
    if (walk_reg_file_list_retcode!=enum_walk_reg_file_list_retcode_ok){
        goto end_of_procedure;        
    }

// seek to the offset required
    u.ul_offset=ul_offset;
    walk_reg_file_list_retcode=walk_reg_file_list(   ui_handle_product_codes
                                                    ,enum_walk_reg_file_list_seek
                                                    ,u
                                                 );  
    if (walk_reg_file_list_retcode!=enum_walk_reg_file_list_retcode_ok){
        goto end_of_procedure;
    }

// reinit body pointer
    u.pBody=pBody;
// put body
    if (walk_reg_file_list(  ui_handle_product_codes
                            ,enum_walk_reg_file_list_put_body
                            ,u
                          )
                        !=enum_walk_reg_file_list_retcode_ok
        ){
        goto end_of_procedure;   
    }
// ok!
    retcode=1;
end_of_procedure:
    return retcode;
}//unsigned int ui_product_file_write_body(unsigned long ul_offset, char *pBody)

// operation types when searching for next/prev product code
typedef enum{
	enum_move_prg_pointer_op_next=0,
	enum_move_prg_pointer_op_prev,
	enum_move_prg_pointer_op_numof
} enum_move_prg_pointer_op;

// retcodes when searching for next/prev product code
typedef enum{
	enum_move_prg_pointer_retcode_ok=0,
	enum_move_prg_pointer_retcode_invalid_op,
	enum_move_prg_pointer_retcode_err,
	enum_move_prg_pointer_retcode_numof
} enum_move_prg_pointer_retcode;

// look for next/prev product code
enum_move_prg_pointer_retcode move_prg_pointer_retcode(enum_walk_reg_file_list_op op, unsigned int *pui_top_of_list_passed){
	enum_move_prg_pointer_retcode retcode;
	unsigned int ui_top_of_list_passed;
    TipoUnionParamWalkReg u;

    memset(&u,0,sizeof(u));
	retcode=enum_move_prg_pointer_retcode_ok;
	ui_top_of_list_passed=0;
	
	if ((op!=enum_walk_reg_file_list_next)&&(op!=enum_walk_reg_file_list_prev)){
		retcode=enum_move_prg_pointer_retcode_invalid_op;
		goto end_of_procedure;
	}

    switch (walk_reg_file_list(ui_handle_product_codes,op,u)){
		case enum_walk_reg_file_list_retcode_ok:
			break;
		case enum_walk_reg_file_list_retcode_end_of_list:
			ui_top_of_list_passed=1;
			if (walk_reg_file_list(ui_handle_product_codes,op,u)!=enum_walk_reg_file_list_retcode_ok){
				retcode=enum_move_prg_pointer_retcode_err;
				goto end_of_procedure;
			}
			break;
		case enum_walk_reg_file_list_retcode_invalid_handle:
		case enum_walk_reg_file_list_retcode_error:
		default:
			retcode=enum_move_prg_pointer_retcode_err;
			goto end_of_procedure;
    }
end_of_procedure:
	*pui_top_of_list_passed=ui_top_of_list_passed;
	return retcode;
}//enum_move_prg_pointer_retcode move_prg_pointer_retcode(enum_walk_reg_file_list_op op)




// funzione che confronta due stringhe, la prima delle quali pu� contenere caratteri jolly
// 0=le stringhe sono uguali
// 1=la prima � maggiore della seconda
// 2=la seconda � maggiore della prima (se la prima � la stringa di ricerca, la ricerca � finita)
unsigned char ucCmpCodiciProgrammaJolly(unsigned char xdata *pJolly, unsigned char xdata *puc){
	unsigned char xdata i;
	// loop di confronto max 21-1 caratteri (non controllo il tappo)
	for (i=0;i<defMaxLengthStringSearchCodiceProgramma-1;i++){
		// se sono su un carattere jolly, ogni char � ok
		if (*pJolly!=defJollyCharSearchCodiceProgramma){
			// la stringa di ricerca � maggiore della stringa codice programma
			if (*pJolly>*puc){
				return 1;
			}
			// la stringa di ricerca � minore della stringa codice programma
			else if (*pJolly<*puc){
				return 2;
			}
		}
		pJolly++;
		puc++;
	}//for (i=0;i<defMaxLengthStringSearchCodiceProgramma;i++)
	// se arrivo qui, le stringhe sono uguali!
	return 0;
}//unsigned char ucCmpCodiciProgrammaJolly(unsigned char xdata *pJolly, unsigned char xdata *puc)



// questa funzione usa come interfaccia la struttura scp 
unsigned char ucSearchCodiceProgramma(void){
	unsigned long int ulMaxNumLoop;
    TipoUnionParamWalkReg u;
    enum_walk_reg_file_list_retcode walk_reg_file_list_retcode;

	ulMaxNumLoop=20000;
	// per ora, nessun record trovato
	scp.uiNumRecordFound=0;
	// record corrente di ricerca=record iniziale indicato
	scp.uiCurRecord=scp.uiStartRecord;
	// se mi vengono richiesti pi� codici di quelli che riesco a trovare, clippo
	if (scp.uiNumMaxRecord2Find>defMaxStringCodiceProgrammaCanFind)
		scp.uiNumMaxRecord2Find=defMaxStringCodiceProgrammaCanFind;
    
// init product walking    
    u.pBody=(char *)(&auxPrg);
    // walk init
    walk_reg_file_list_retcode=walk_reg_file_list(   ui_handle_product_codes
                                                    ,enum_walk_reg_file_list_init
                                                    ,u
                                                 ); 
    // seek to the record required
    if (scp.uiStartRecord){
        u.ul_offset=scp.uiStartRecord;
        walk_reg_file_list_retcode=walk_reg_file_list(   ui_handle_product_codes
                                                        ,enum_walk_reg_file_list_seek
                                                        ,u
                                                     );  
        if (walk_reg_file_list_retcode!=enum_walk_reg_file_list_retcode_ok){
            goto end_of_procedure;
        }
    }
// reinit body pointer
    u.pBody=(char *)(&auxPrg);
	// loop di ricerca dei record desiderati
	while(scp.uiNumRecordFound<scp.uiNumMaxRecord2Find){
        // just to avoid lockup, better if using a timeout...
		if (ulMaxNumLoop--==0)
			return 0;
        // get next or get prev block???
		if (scp.direction==enumDirectionForward_SCP){
            walk_reg_file_list_retcode=walk_reg_file_list(   ui_handle_product_codes
                                                            ,enum_walk_reg_file_list_next
                                                            ,u
                                                        );   
        }
        else{
            walk_reg_file_list_retcode=walk_reg_file_list(   ui_handle_product_codes
                                                            ,enum_walk_reg_file_list_prev
                                                            ,u
                                                        );   
        }
        // end of list???
        if (walk_reg_file_list_retcode==enum_walk_reg_file_list_retcode_end_of_list)
            break;
        // error???
        if (walk_reg_file_list_retcode!=enum_walk_reg_file_list_retcode_ok)
            break;
        // get offset of actual block walked
        scp.uiCurRecord=ul_reg_file_cur_id(ui_handle_product_codes, enum_reg_file_id_required_last_block_walked);
       
        // get body
        walk_reg_file_list_retcode=walk_reg_file_list(   ui_handle_product_codes
                                                        ,enum_walk_reg_file_list_get_body
                                                        ,u
                                                     );         
		// i programmi sono ordinati per codice, per cui se il codice programma che trovo � maggiore
		// di quello di ricerca, ho finito
		if (scp.ucFindExactCode){
			scp.ucCmpResult=strcmp(scp.ucString, auxPrg.codicePrg);
		}
		else{
			scp.ucCmpResult=ucCmpCodiciProgrammaJolly(scp.ucString, auxPrg.codicePrg);
			// se la stringa codice programma � maggiore di quella di ricerca, fine ricerca
			// (i codici programma sono memorizzati in ordine crescente...)
			if (scp.ucCmpResult==2){
				break;
			}
		}
// se le stringhe sono uguali, ho trovato quello che cercavo!
		if (scp.ucCmpResult==0){
			strcpy(scp.ucFoundString[scp.uiNumRecordFound],auxPrg.codicePrg);
			scp.uiNumRecordFoundString[scp.uiNumRecordFound]=scp.uiCurRecord;
			scp.uiLastRecordFound=scp.uiCurRecord;
			scp.uiNumRecordFound++;
			if (scp.ucFindExactCode){
				break;
			}
		}    
	}//while(scp.uiNumRecordFound<scp.uiNumMaxRecord2Find)
end_of_procedure:
	// restituisco il numero di record trovati...
	return scp.uiNumRecordFound;
}//unsigned char ucSearchCodiceProgramma(void)

unsigned int uiGetActPrg(void){
	return hprg.actPrg;
}

unsigned int uiGetRunningPrg(void){
	return hprg.runningPrg;
}


// set actual program offset...
unsigned int uiSetActPrg(tipoNumeroProgramma uiNewValueActPrg){
	hprg.actPrg=uiNewValueActPrg;
	if (hprg.actPrg==hprg.runningPrg){
		hprg.ptheAct=&(hprg.theRunning);
		memcpy(&hprg.theAct,&hprg.theRunning,sizeof(hprg.theAct));
	}
	else{
		hprg.ptheAct=&(hprg.theAct);
        if (!ui_product_file_read_body(uiNewValueActPrg, (char *)&(hprg.theAct))){
			//ucNumErrNandFlash++;
        }
	}
	return hprg.actPrg;
}//unsigned int uiSetActPrg(tipoNumeroProgramma uiNewValueActPrg)


// set the running prg...
unsigned int uiSetRunningPrg(tipoNumeroProgramma uiNewValueRunningPrg){
	hprg.runningPrg=uiNewValueRunningPrg;
    if (!ui_product_file_read_body(uiNewValueRunningPrg, (char *)&(hprg.theRunning))){
		//ucNumErrNandFlash++;
    }
	if (hprg.actPrg==hprg.runningPrg){
		hprg.ptheAct=&(hprg.theRunning);
		memcpy(&hprg.theAct,&hprg.theRunning,sizeof(hprg.theAct));
	}
	return hprg.runningPrg;
}//unsigned int uiSetRunningPrg(tipoNumeroProgramma uiNewValueRunningPrg)

// save the running prg
unsigned char ucSaveRunningPrg(void){
	hprg.theRunning.ucVersionCharacter='V'; // il carattere 'V'
	hprg.theRunning.ucVersionNumber=defLastPROGRAMMAversion; // il numero di versione del formato codici prodotto gestito
    if (ui_product_file_write_body(hprg.runningPrg,(unsigned char *)&(hprg.theRunning))){
		if (hprg.actPrg==hprg.runningPrg){
			hprg.ptheAct=&(hprg.theRunning);
			memcpy(&hprg.theAct,&hprg.theRunning,sizeof(hprg.theAct));
		}
		return 1;
	}
	else{
    }
	return 0;
}//unsigned char ucSaveRunningPrg(void)


// save actual program
unsigned char ucSaveActPrg(void){
	hprg.ptheAct->ucVersionCharacter='V'; // il carattere 'V'
	hprg.ptheAct->ucVersionNumber=defLastPROGRAMMAversion; // il numero di versione del formato codici prodotto gestito
    if (ui_product_file_write_body(hprg.actPrg,(unsigned char *)(hprg.ptheAct))){
		// corregge bug per cui una modifica al programma act non si ricopiava su prg running!
		if ((!PrgRunning)&&(hprg.actPrg==hprg.runningPrg)&&(hprg.ptheAct!=&hprg.theRunning)){
			memcpy(&hprg.theRunning,hprg.ptheAct,sizeof(hprg.theRunning));
		}
		return 1;
	}
	else{
		//ucNumErrNandFlash++;
    }
	return 0;
}//unsigned char ucSaveActPrg(void)


// add a default product code, used when the product codes list is emoty...
void v_add_default_product_code(void){
    enum_insert_reg_file_list_retcode insert_reg_file_list_retcode;
    PROGRAMMA private_prg;
    unsigned long ul_cur_offset;

// init variables...
    memset(&private_prg,0,sizeof(private_prg));

// init the program with default values...
    private_prg.num_fili=1;
    private_prg.diametro_filo=0.55;
    private_prg.diametro_mandrino=2.50;
    private_prg.vel_produzione=24.864916;
    private_prg.rpm_mandrino=2595;
    private_prg.assorb[0]=0.07;
    private_prg.assorb[1]=0.25;
    private_prg.pos_pretaglio=3.84;
    private_prg.coeff_corr=1.03;
    private_prg.vruote_prg[0]=2048;
    private_prg.vruote_prg[1]=2048;
    private_prg.vel_frizioni[0]=103.88;
    private_prg.vel_frizioni[1]=103.88;
    private_prg.fornitore=0;
    private_prg.vel_prod_vera=20.04;
    // imposto chiave in modo che indichi che il programma � valido...
    private_prg.usValidProgram_A536=0xA536;
    private_prg.ucVersionCharacter='V'; // il carattere 'V'
    private_prg.ucVersionNumber=defLastPROGRAMMAversion; // il numero di versione del formato codici prodotto gestito

// copy the key...
    memcpy(private_prg.codicePrg,"0.550      2.50 1 0 ",sizeof(private_prg.codicePrg));
// try to insert the program
    insert_reg_file_list_retcode=insert_reg_file_list(ui_handle_product_codes,(char *)(&private_prg));
// test retcode...
    switch(insert_reg_file_list_retcode){
        case enum_insert_reg_file_list_ok:
            ul_cur_offset=ul_reg_file_cur_id(ui_handle_product_codes, enum_reg_file_id_required_last_block_inserted);
            break;
        case enum_insert_reg_file_list_already_exists:
        default:
            ul_cur_offset=ul_reg_file_cur_id(ui_handle_product_codes, enum_reg_file_id_required_last_block_found);
            break;
    }
// set actual program offset
    uiSetActPrg(ul_cur_offset);
}//static void v_add_default_product_code(void)



// initialize the program list
void vInitProgramList(void){
    enum_walk_reg_file_list_retcode walk_reg_file_list_retcode;


    TipoUnionParamWalkReg u;

// no running programs at this moment
	hprg.runningPrg=0xFFFFFFFF;
//
// register the product codes file? no, already registered!
//
//    ui_register_product_codes();

// init product walking    
    u.pBody=(char *)(&auxPrg);
    // walk init
    walk_reg_file_list_retcode=walk_reg_file_list(   ui_handle_product_codes
                                                    ,enum_walk_reg_file_list_init
                                                    ,u
                                                 ); 
    // walk next, just to find the first program in list...
    walk_reg_file_list_retcode=walk_reg_file_list(   ui_handle_product_codes
                                                    ,enum_walk_reg_file_list_next
                                                    ,u
                                                );   
    // if the list is empty, add at least one simple entry
    if (walk_reg_file_list_retcode==enum_walk_reg_file_list_retcode_end_of_list){
        v_add_default_product_code();
    }
    else{
        // set actual program using the offset of last block walked
        uiSetActPrg(ul_reg_file_cur_id(ui_handle_product_codes, enum_reg_file_id_required_last_block_walked));
    }

}//void vInitProgramList(void)


// Inserisco il programma nella lista dei programmi, se non esiste gi�. 
// restituisce 1 se il codice e' stato inserito correttamente
// restituisce  se codice gi� esistente o se operazione non riuscita
unsigned char InsertPrg(char * p_key_string){
    enum_insert_reg_file_list_retcode insert_reg_file_list_retcode;
    PROGRAMMA private_prg;
    unsigned char uc_retcode;
    unsigned long ul_cur_offset;
// init...
    memset(&private_prg,0,sizeof(private_prg));
    uc_retcode=0;
// copy the key...
    memcpy(private_prg.codicePrg,p_key_string,sizeof(private_prg.codicePrg));
// try to insert the program
    insert_reg_file_list_retcode=insert_reg_file_list(ui_handle_product_codes,(char *)(&private_prg));
// test retcode...
    switch(insert_reg_file_list_retcode){
        case enum_insert_reg_file_list_ok:
            uc_retcode=1;
            ul_cur_offset=ul_reg_file_cur_id(ui_handle_product_codes, enum_reg_file_id_required_last_block_inserted);
            break;
        case enum_insert_reg_file_list_already_exists:
        default:
            ul_cur_offset=ul_reg_file_cur_id(ui_handle_product_codes, enum_reg_file_id_required_last_block_found);
            break;
    }
// set actual program offset
    uiSetActPrg(ul_cur_offset);
    return uc_retcode;
}//unsigned char InsertPrg(char * p_key_string)

// Cancellazione del programma:
// se � il primo della lista, modifica hprg.firstPrg
// se � l' unico della lista, hprg.firstPrg=0
void DeleteProgramma(tipoNumeroProgramma nPrg){
	tipoNumeroProgramma prev,next;
    TipoUnionParamWalkReg u;
    PROGRAMMA private_prg;
	//unsigned int i;
	unsigned int ui_deleting_first_program;
	unsigned int ui_deleting_last_program;
    enum_walk_reg_file_list_retcode walk_reg_file_list_retcode;
	
// init...	
	ui_deleting_first_program=0;
	ui_deleting_last_program=0;
	
// read the body to get the prg offset... and also to get the body key...
    if (!ui_product_file_read_body(nPrg, (char*)&private_prg)){
        goto end_of_procedure;
    }
// get next prg identifier
	if (move_prg_pointer_retcode(enum_walk_reg_file_list_next,&ui_deleting_last_program)!=enum_move_prg_pointer_retcode_ok){
        goto end_of_procedure;
	}
    // get offset of actual block walked
    next=ul_reg_file_cur_id(ui_handle_product_codes, enum_reg_file_id_required_last_block_walked);
// back to the original position
    u.ul_offset=nPrg;
    walk_reg_file_list_retcode=walk_reg_file_list(   ui_handle_product_codes
                                                    ,enum_walk_reg_file_list_seek
                                                    ,u
                                                 );  
    if (walk_reg_file_list_retcode!=enum_walk_reg_file_list_retcode_ok){
        goto end_of_procedure;
    }
// get prev ptr
	if (move_prg_pointer_retcode(enum_walk_reg_file_list_prev,&ui_deleting_first_program)!=enum_move_prg_pointer_retcode_ok){
        goto end_of_procedure;
	}
    // get offset of actual block walked
    prev=ul_reg_file_cur_id(ui_handle_product_codes, enum_reg_file_id_required_last_block_walked);
// finally, delete the damn program
	if (delete_reg_file_list(ui_handle_product_codes,(char*)&private_prg)!=enum_delete_reg_file_list_ok){
        goto end_of_procedure;
	}

	// if this is the only program in list... mark it!
	if (prev==next){
		hprg.firstPrg=0;
	}
	else if (ui_deleting_last_program){
		uiSetActPrg(prev);
	}
	else{
		uiSetActPrg(next);
	}
	// Tutte le nvram_struct.commesse che usano il programma cancellato, le imposto al programma corrente.
	if (spiralatrice.firstComm){
		unsigned long index;
		index=spiralatrice.firstComm-1;
		do{
			if (nvram_struct.commesse[index].numprogramma==nPrg)
				nvram_struct.commesse[index].numprogramma=uiGetActPrg();
			if (SearchNextCommessa(index)==index)
				break;
			index++;
		}while(1);
	}	
end_of_procedure:
	;
}//void DeleteProgramma(tipoNumeroProgramma nPrg)


// empty the product codes list...
void v_empty_product_codes_list(void){
// close the product file list
    v_close_reg_file_list();
// empty the product codes file...
    f_unlink(def_product_codes_filename);
//
// register the product codes file
//
    ui_register_product_codes();

// add at least one product code...
    v_add_default_product_code();

}


// test product codes procedures...
enum_test_product_codes_retcode test_product_codes(void){
    //TipoStructBodyInfo bodyInfo;
    enum_test_product_codes_retcode retcode;
    PROGRAMMA prg_test;
    FATFS fatfs;

// mount file system
	f_mount(0, &fatfs);		// Register volume work area (never fails) 

// initialize the registry system...
    v_init_file_list_registry();

// initialize variables and structs...
    {
        retcode=enum_test_product_codes_retcode_ok;
        ui_handle_product_codes=def_file_list_invalid_handle;
        memset(&prg_test,0,sizeof(prg_test));
    
    }
//
// register the product codes file
//
    ui_register_product_codes();

	
//
// insert some data
    {
        enum_insert_reg_file_list_retcode insert_reg_file_list_retcode;
        static const char *pc_codes[]={  "codice_000"
                                        ,"codice_001"
                                        ,"codice_010"
                                        ,"codice_002"
                                      };
        int i;
        for (i=0;i<sizeof(pc_codes)/sizeof(pc_codes[0]);i++){
            strncpy(prg_test.codicePrg,pc_codes[i],sizeof(prg_test.codicePrg)-1);
            insert_reg_file_list_retcode=insert_reg_file_list(ui_handle_product_codes,(char *)(&prg_test));
            switch(insert_reg_file_list_retcode){
                case enum_insert_reg_file_list_ok:
                case enum_insert_reg_file_list_already_exists:
                    break;
                default:
                    retcode=enum_test_product_codes_retcode_err;
                    goto end_of_procedure;                    
            }
        }
    
    }
// try insertprg
    InsertPrg("codice_015");
    InsertPrg("codice_004");
// list the products
    {
        TipoUnionParamWalkReg u;
        enum_walk_reg_file_list_retcode walk_reg_file_list_retcode;
        unsigned int ui_continue_loop;
    
        ui_continue_loop=1;
    
        u.pBody=(char *)(&prg_test);
        // walk init
        walk_reg_file_list_retcode=walk_reg_file_list(   ui_handle_product_codes
                                                        ,enum_walk_reg_file_list_init
                                                        ,u
                                                    );
        if (walk_reg_file_list_retcode!=enum_walk_reg_file_list_retcode_ok){
            retcode=enum_test_product_codes_retcode_err;
            goto end_of_procedure;        
        }
        // listing loop
        while(ui_continue_loop){
            // walk next
            walk_reg_file_list_retcode=walk_reg_file_list(   ui_handle_product_codes
                                                            ,enum_walk_reg_file_list_next
                                                            ,u
                                                        );   
            switch(walk_reg_file_list_retcode){
                case enum_walk_reg_file_list_retcode_ok:                     
                    if (walk_reg_file_list(  ui_handle_product_codes
                                            ,enum_walk_reg_file_list_get_body
                                            ,u
                                          )
                        !=enum_walk_reg_file_list_retcode_ok
                        ){
                            retcode=enum_test_product_codes_retcode_err;
                            goto end_of_procedure;   
                        }
                    break;
                case enum_walk_reg_file_list_retcode_end_of_list:
                    ui_continue_loop=0;
                    break;
                default:
                    break;
            }
        }
    }
// now, search for product codes...
    {
        // reset struttura di ricerca codice prodotto
        memset(&scp,0,sizeof(scp));
        strncpy(scp.ucString,"codice_   ",sizeof(scp.ucString)-1);
        // indico di effettuare la ricerca jolly
        scp.ucFindExactCode=0;
        scp.uiStartRecord=0;
        scp.uiNumMaxRecord2Find=10;
        // cerca i codici...
        ucSearchCodiceProgramma();
    }



// close
    v_close_reg_file_list();
//
end_of_procedure:
    return retcode;
}//enum_test_product_codes_retcode test_product_codes(void)
