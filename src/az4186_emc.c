// modulo di gestione bus EMC
// nella scheda az4186 il bus EMC viene usato per comunicare copn fpga e con memoria ram esterna
#include <stdlib.h>
//#include <stdio.h>
#include <string.h>
//#include <malloc.h>
#include <math.h> 
#include <ctype.h>

#include "LPC23xx.h"                        /* LPC23xx/24xx definitions */
#include "type.h"
#include "target.h"
#include "fpga_nxp.h"
#include "az4186_emc.h"

// only for diagnostic purposes...
extern void v_print_msg(unsigned int row,char *s);


// se la frequenza di funzionamento del micro supera i 48MHz
// � necessario rallentare il funzionamento del bus verso la memoria esterna
#if Fcclk==48000000
	#pragma message "\n ****** \n ****** \nSystem clock frequency is 48Mhz\n ****** \n ****** \n"
// frequenza clock "lenta": basta wait states minori

// EMC #0
	#define def_EMC_STA_WAITWEN0 0x3
	#define def_EMC_STA_WAITOEN0 0x0
	#define def_EMC_STA_WAITRD0  0x8
	#define def_EMC_STA_WAITWR0  0x8
	#define def_EMC_STA_WAITTURN0 0x1f
	#define def_EMC_STA_WAITPAGE0 0x1f

// EMC #1
	#define def_EMC_STA_WAITWEN1 0x3
	#define def_EMC_STA_WAITOEN1 0x0
	#define def_EMC_STA_WAITRD1  0x8
	#define def_EMC_STA_WAITWR1  0x8
	#define def_EMC_STA_WAITTURN1 0x1f
	#define def_EMC_STA_WAITPAGE1 0x1f

#elif Fcclk==60000000 
	#pragma message "System clock frequency is 60MHz"
// frequenza clock "veloce": servono wait states maggiori
 
// EMC #0
// attenzione! se si aumenta il tempo before write, si deve aumentare anche il temp di write!
// con queste impostazioni funziona correttamente, verificato ad oscilloscopio che
// un accesso in r/w occupa circa 100ns
	// #define def_EMC_STA_WAITWEN0  0x1 //0x5
	// #define def_EMC_STA_WAITOEN0  0x1 //0x0
	// #define def_EMC_STA_WAITRD0   (0x04+def_EMC_STA_WAITOEN0) //0x10
	// #define def_EMC_STA_WAITWR0   (0x04+def_EMC_STA_WAITWEN0) //0x10
	// #define def_EMC_STA_WAITTURN0 0x04
	// #define def_EMC_STA_WAITPAGE0 0x04

//EMC #1
	// #define def_EMC_STA_WAITWEN1 0x1
	// #define def_EMC_STA_WAITOEN1 0x1
	// #define def_EMC_STA_WAITRD1  (0x04+def_EMC_STA_WAITOEN1)
	// #define def_EMC_STA_WAITWR1  (0x04+def_EMC_STA_WAITWEN1)
	// #define def_EMC_STA_WAITTURN1 0x4
	// #define def_EMC_STA_WAITPAGE1 0x4



//	#define def_EMC_STA_WAITWEN0  0x1 //0x5
//	#define def_EMC_STA_WAITOEN0  0x1 //0x0
	#define def_EMC_STA_WAITWEN0  0x0 //0x5
	#define def_EMC_STA_WAITOEN0  0x0 //0x0
//	#define def_EMC_STA_WAITRD0   (0x04+def_EMC_STA_WAITOEN0) //0x10
//	#define def_EMC_STA_WAITWR0   (0x04+def_EMC_STA_WAITWEN0) //0x10
// importante! con questo setup (0x06+...)non vi sono flicker nella lettura fpga; il valore 0x04 � troppo basso!
	#define def_EMC_STA_WAITRD0   (0x10+def_EMC_STA_WAITOEN0) //0x10
	#define def_EMC_STA_WAITWR0   (0x10+def_EMC_STA_WAITWEN0) //0x10
    
	#define def_EMC_STA_WAITTURN0 0x04
	#define def_EMC_STA_WAITPAGE0 0x04

// EMC #1
	#define def_EMC_STA_WAITWEN1 0x5
	#define def_EMC_STA_WAITOEN1 0x1
	#define def_EMC_STA_WAITRD1  (0x08+def_EMC_STA_WAITOEN1)
	#define def_EMC_STA_WAITWR1  (0x08+def_EMC_STA_WAITWEN1)
	#define def_EMC_STA_WAITTURN1 0x1F
	#define def_EMC_STA_WAITPAGE1 0x1F

#else

	// altre frequenze di clock non sono al momento ammesse
	#error EMC bus: unhandled clock frequency, only 60MHz and 48MHz accepted

#endif //#if Fcclk==


// enable/disable external ram access through fpga
unsigned int ui_enable_external_ram(unsigned int ui_enable){
    if (ui_enable){
        // enable external ram access through fpga    
        ucEnable_ext_ram_0xa5=0xa5;
        if (ucEnable_ext_ram_0xa5!=1){
            return 0;
        }
    }
    else{
        // disable external ram access through fpga    
        ucEnable_ext_ram_0xa5=0x00;
        if (ucEnable_ext_ram_0xa5!=0){
            return 0;
        }
    }
    return 1;
}


// procedura di inizializzazione di bus di interfaccia esterna
// Abilita l'interfaccia EMC e imposta i wait states in modo da poter comunicare con fpga 
// Attualmente sono supportate solo le frequenze di bus 48MHz e 60MHz
void vEMC_az4186_fpga_init(void){

#ifdef def_test_ab6_ab7_pins
    // test ab6 and ab7: are they working??????
    {
        unsigned int ui_get_register_value;
    // reset address bus pins to i/o function
        PINSEL8 = 0x0;

    // set as output p0[19] p0[20] e p0[22]=clk cmd e data0
        FIO4DIR =0xffffffff;
  
    // set the mask so operations with FIOxSET and FIOxCLR are permitted
        FIO4MASK=0;
        ui_get_register_value=FIO4MASK;        
    // set/clr/set the ab2
        FIO4SET =1<<2;
        FIO4SET &=~(1<<2);
        FIO4CLR =1<<2;
        FIO4CLR &=~(1<<2);
        FIO4SET =1<<2;
        FIO4SET &=~(1<<2);

    // set/clr/set the ab6
        FIO4SET =1<<6;
        FIO4SET &=~(1<<6);
        FIO4CLR =1<<6;
        FIO4CLR &=~(1<<6);
        FIO4SET =1<<6;
        FIO4SET &=~(1<<6);
    // set/clr/set the ab7
        FIO4SET =1<<7;
        FIO4SET &=~(1<<7);
        FIO4CLR =1<<7;
        FIO4CLR &=~(1<<7);
        FIO4SET =1<<7;
        FIO4SET &=~(1<<7);
    }
#endif


	// Initialize EMC for fpga az4186
	EMC_CTRL = 0x00000000;   // disable emc!!!
    // importantissimo (vedi http://tech.groups.yahoo.com/group/lpc2000/message/43796)
//Mark,

// I have suffered for two weeks with the memory controller on the LPC2388, and a
// kind person on this forum put me out of my misery. Now I can hopefully return
// the favour.

// Regardless of how well you read the LPC2388 documentation, you will not find a
// description of a register called EMC_DYN_CTRL (address 0xFFE08020). Set it to
// 0x00000000 and the memory controller will start functioning as described.

// Although the 2388 only supports DRAM, not SRAM, this register still exists and
// by default is set to enable dynamic refresh function (see the manual for the
// LPC2478)! Apparently this fact will be incorporated in a future revision of the
// 2388 manual.

// After that, the rest should come easy by following the docs.

// Hope it helps,

// Bruce.

    EMC_DYN_CTRL=0;
    SCS|=(1<<2); // disables burst
//	EMC_CTRL = 0x00000001; // enable emc   

	PCONP  |= 0x00000800;		// Turn On EMC PCLK 
    {
        unsigned long ul;
        ul=PINSEL6;
        // reset settings on P3.0, P3.1,...,P3.7=>d0,d1,...,d7
        ul &=~( (3<<0)|(3<<2)|(3<<4)|(3<<6)|(3<<8)|(3<<10)|(3<<12)|(3<<14));
        // set settings on P3.0, P3.1,...,P3.7=>d0,d1,...,d7 set to 01 each pair of bits
        ul |= ( (1<<0)|(1<<2)|(1<<4)|(1<<6)|(1<<8)|(1<<10)|(1<<12)|(1<<14));
        PINSEL6 = ul; // was 0x00005555;
    }
	// select a15..0 for emc bus
	PINSEL8 = 0x55555555;// this pinsel is completely voted to address bus so it is not necessary set7reset bits individually
    {
        unsigned long ul;
	// select cs0 cs1 rd wr for emc bus
        ul=PINSEL9;
        // reset settings on P4.24=OE*, P4.25=BLS0*, P4.30=CS0*, P4.31=CS1*
        ul &=~( (3<<16)|(3<<18)|(3<<28)|(3<<30));
        // set settings on P4.24=OE*=1, P4.25=BLS0*=2, P4.30=CS0*=1, P4.31=CS1*=1
        ul |= ( (1<<16)|(2<<18)|(1<<28)|(1<<30));
        PINSEL9 = ul; // was 0x50090000;
    }

	//  EMC_STA_WAITWEN0  = 0x4;
	//  EMC_STA_WAITRD0   = 0x8;
	//  EMC_STA_WAITWR0   = 0x8;
	// 488-> 375 ns read, 250ns write

	//  EMC_STA_WAITWeEN0  = 0x3;
	//  EMC_STA_WAITRD0   = 0x8;
	//  EMC_STA_WAITWR0   = 0x8;
	// 388-> 375 ns read, 200ns write

	//  EMC_STA_WAITWEN0  = 0x3;
	//  EMC_STA_WAITRD0   = 0x6;
	//  EMC_STA_WAITWR0   = 0x8;
	// 368-> NON funziona!!!!

// ***********************************************************
// ***********************************************************
//
// inizializzazione external memory interface #0
//
// ***********************************************************
// ***********************************************************

	EMC_STA_CFG0      = 0x00000000;
    EMC_DYN_CFG0=0;
	// con frequenza 48MHz
	// non scendere al di sotto di questo valore -->0x3 altrimenti non funziona la lettura/scrittura da fpga
	// a causa di problemi nel bus nxp
	EMC_STA_WAITWEN0  = def_EMC_STA_WAITWEN0;

	// mettere un valore di pausa molto basso, idealmente zero, tra cs e oe in modo  
	// che se dopo cs non trovo OE allora certamente � una write
	// e riesco a lavorare anche con chip release B che ha un bug per cui segnale di write non funziona correttamente
	EMC_STA_WAITOEN0  = def_EMC_STA_WAITOEN0;
	// con frequenza 48MHz
	// non scendere al di sotto di questo valore -->0x8 altrimenti non funziona la lettura/scrittura da fpga
	// a causa di problemi nel bus nxp
	EMC_STA_WAITRD0   = def_EMC_STA_WAITRD0;

	// con frequenza 48MHz
	// non scendere al di sotto di questo valore -->0x8 altrimenti non funziona la lettura/scrittura da fpga
	// a causa di problemi nel bus nxp
	EMC_STA_WAITWR0   = def_EMC_STA_WAITWR0;

	EMC_STA_WAITTURN0 = def_EMC_STA_WAITTURN0;
	EMC_STA_WAITPAGE0 = def_EMC_STA_WAITPAGE0;

// ***********************************************************
// ***********************************************************
//
// inizializzazione external memory interface #1
//
// ***********************************************************
// ***********************************************************

	EMC_STA_CFG1      = 0x00000000;
    EMC_DYN_CFG1=0;
    
	// con frequenza 48MHz
	// non scendere al di sotto di questo valore -->0x3 altrimenti non funziona la lettura/scrittura da fpga
	// a causa di problemi nel bus nxp
	EMC_STA_WAITWEN1  = def_EMC_STA_WAITWEN1;

	// mettere un valore di pausa molto basso, idealmente zero, tra cs e oe in modo  
	// che se dopo cs non trovo OE allora certamente � una write
	// e riesco a lavorare anche con chip release B che ha un bug per cui segnale di write non funziona correttamente
	EMC_STA_WAITOEN1  = def_EMC_STA_WAITOEN1;
	// con frequenza 48MHz
	// non scendere al di sotto di questo valore -->0x8 altrimenti non funziona la lettura/scrittura da fpga
	// a causa di problemi nel bus nxp
	EMC_STA_WAITRD1   = def_EMC_STA_WAITRD1;

	// con frequenza 48MHz
	// non scendere al di sotto di questo valore -->0x8 altrimenti non funziona la lettura/scrittura da fpga
	// a causa di problemi nel bus nxp
	EMC_STA_WAITWR1   = def_EMC_STA_WAITWR1;

	EMC_STA_WAITTURN1 = def_EMC_STA_WAITTURN1;
	EMC_STA_WAITPAGE1 = def_EMC_STA_WAITPAGE1;

	EMC_CTRL = 0x00000001; // enable emc   
}//void vEMC_az4181_fpga_init(void)




// executes a destructive r/w test on external ram and and returns the number of errors found
// returns 0--> all ok, else the number of errors found
unsigned int ui_ext_ram_destructive_rw_test(void){
    //unsigned int *pui;
    //unsigned short int *pus;
    unsigned char *puc;
    unsigned int i;
    unsigned char c_read,c_written;
    unsigned int  ui_an_error_found,ui_num_error_found;
    #define def_ui_seed_test 0x1234321
    #define def_us_seed_test 0x3210987
    #define def_uc_seed_test 0xabbaccc
    char msg[64];
    unsigned int row;

// reser num of errors found
    ui_an_error_found=0;
    ui_num_error_found=0;
    row=0;

// on emc, only bit access is permitted
#if 0
        //
        // int test
        //

        // write
            pui=(unsigned int *)def_ext_ram_start_address;
            srand(def_ui_seed_test);
            i=0;
            w=0;
            while (i<def_ext_ram_size_bytes/sizeof(unsigned int)){
                //w=rand();
                w=w+1;
                *pui++=w;
                i++;
            }
        // read
            pui=(unsigned int *)def_ext_ram_start_address;
            srand(def_ui_seed_test);
            i=0;
            w=0;
            while (i<def_ext_ram_size_bytes/sizeof(unsigned int)){
                //w=rand();
                w=w+1;
                r=*pui++;
                if (r!=w){
                    ui_an_error_found=1;
                    ui_num_error_found++;
                }
                i++;
            }

        //
        // short int test
        //

        // write
            pus=(unsigned short int *)def_ext_ram_start_address;
            srand(def_us_seed_test);
            i=0;
            while(i<def_ext_ram_size_bytes/sizeof(unsigned short int)){
                w=rand()&0xFFFF;
                *pus++=w;
                i++;
            }
        // read
            pus=(unsigned short int *)def_ext_ram_start_address;
            srand(def_us_seed_test);
            i=0;
            while (i<def_ext_ram_size_bytes/sizeof(unsigned short int)){
                w=rand()&0xFFFF;
                r=*pus++;
                if (r!=w){
                    ui_an_error_found=1;
                    ui_num_error_found++;
                }
                i++;
            }
#endif
//
// char test
//
    if (!ui_enable_external_ram(1)){
        return 1;
    }
    {
        // check data bus, write and check 1,2,4,8,...
        i=0;
        puc=(unsigned char *)def_ext_ram_start_address;
        while (i<8){
            c_written=1<<i;
            *puc=c_written;
            c_read=*puc;
            // error setting bit D<i>
            if (c_read!=c_written){
                ui_an_error_found=1;
                ui_num_error_found++;
                memset(msg,0,sizeof(msg));
                strcpy(msg,"Databus err on D");
                msg[strlen(msg)]=i+'0';
                v_print_msg(row++,msg);
            }
            i++;
        }
        i=0;
        // check data bus, write and check 11111110 11111101 11111011 etc
        puc=(unsigned char *)def_ext_ram_start_address;
        while (i<8){
            c_written=~(1<<i);
            *puc=c_written;
            c_read=*puc;
            // error resetting bit D<i>
            if (c_read!=c_written){
                ui_an_error_found=1;
                ui_num_error_found++;
                memset(msg,0,sizeof(msg));
                strcpy(msg,"Databus err on D");
                msg[strlen(msg)]=i+'0';
                v_print_msg(row++,msg);
            }
            i++;
        }
    }
    {
		unsigned int ui_num_loop_check_data_address;
		memset(msg,0,sizeof(msg));
		strcpy(msg,"data/addr test");
		v_print_msg(row++,msg);
		// loop n times looking for errors on data and address
		for (ui_num_loop_check_data_address=0;ui_num_loop_check_data_address<100;ui_num_loop_check_data_address++) {
			// check address bus, write and check @address 1,2,4,8,...
			i=0;
			srand(def_uc_seed_test);
			puc=(unsigned char *)def_ext_ram_start_address;
			while (i<14){
				c_written=rand()&0xFF;
				puc[1<<i]=c_written;
				// if the bit is not forced to 1, the writing goes to address 0, so change the zero value...
				puc[0]=c_written+1;
				c_read=puc[1<<i];
				// error while reading from address A<i>
				if (c_read!=c_written){
					ui_an_error_found=1;
					ui_num_error_found++;
					memset(msg,0,sizeof(msg));
					strcpy(msg,"Addrbus err on A");
					msg[strlen(msg)]=i+'0';
					v_print_msg(row++,msg);
				}
				i++;
			}
			// check address bus, write and check @address 0x3FFE,etc
			i=0;
			// another random number initialization is not necessary
			//srand(def_uc_seed_test);
			puc=(unsigned char *)def_ext_ram_start_address;
			while (i<14){
				c_written=rand()&0xFF;
				puc[(~(1<<i))&((1<<15)-1)]=c_written;
				// if the bit is not forced to 1, the writing goes to address 0x3FFF, so change the 0x3FFFF value...
				puc[(1<<15)-1]=c_written+1;
				c_read=puc[(~(1<<i))&((1<<15)-1)];
				// error while reading from address A<i>
				if (c_read!=c_written){
					ui_an_error_found=1;
					ui_num_error_found++;
					memset(msg,0,sizeof(msg));
					strcpy(msg,"Addrbus err on A");
					msg[strlen(msg)]=i+'0';
					v_print_msg(row++,msg);
				}
				i++;
			}
		}
    }
    // if no errors coming from previous routines
    if (!ui_an_error_found){
		unsigned int ui_num_loop_check_rw;
		memset(msg,0,sizeof(msg));
		strcpy(msg,"RW test");
		v_print_msg(row++,msg);

		// loop n times looking for errors on data and address
		for (ui_num_loop_check_rw=0;ui_num_loop_check_rw<100;ui_num_loop_check_rw++) {
		
		// write
			puc=(unsigned char *)def_ext_ram_start_address;
			i=0;
			while (i<def_ext_ram_size_bytes/sizeof(unsigned char)){
				c_written=i+17;
				*puc++=c_written;
				i++;
			}
		// read
			puc=(unsigned char *)def_ext_ram_start_address;
			i=0;
			while (i<def_ext_ram_size_bytes/sizeof(unsigned char)){
				c_written=i+17;
				c_read=*puc++;
				if (c_read!=c_written){
					ui_an_error_found=1;
					ui_num_error_found++;
					memset(msg,0,sizeof(msg));
					strcpy(msg,"RW error");
					v_print_msg(row++,msg);
					if (ui_num_error_found>10)
						break;
				}
				i++;
			}
		// write random pattern
			puc=(unsigned char *)def_ext_ram_start_address;
			srand(def_uc_seed_test);
			i=0;
			while (i<def_ext_ram_size_bytes/sizeof(unsigned char)){
				c_written=rand()&0xFF;
				*puc++=c_written;
				i++;
			}
		// read random pattern
			puc=(unsigned char *)def_ext_ram_start_address;
			srand(def_uc_seed_test);
			i=0;
			while (i<def_ext_ram_size_bytes/sizeof(unsigned char)){
				c_written=rand()&0xFF;
				c_read=*puc++;
				if (c_read!=c_written){
					ui_an_error_found=1;
					ui_num_error_found++;
					memset(msg,0,sizeof(msg));
					strcpy(msg,"RW error");
					v_print_msg(row++,msg);
					if (ui_num_error_found>10)
						break;
				}
				i++;
			}
		}
    }
    if (ui_num_error_found)
        return ui_num_error_found;
    else if (ui_an_error_found)
        return 1;
    else
        return 0;
}//unsigned int ui_ext_ram_destructive_rw_test(void)


// retention test

unsigned int ui_ext_ram_retention_test(void){
    #define def_uc_seed_test_retention 0x12345678
    char msg[64];
    unsigned char *puc,c_written,c_read;
    unsigned int i,row,ui_an_error_found,ui_num_error_found; 
    unsigned int retcode;

// init
    row=0;
    ui_an_error_found=0;
    ui_num_error_found=0;
    retcode=0;

// print hello nsg
    memset(msg,0,sizeof(msg));
    strcpy(msg,"Retention test");
    v_print_msg(row++,msg);

    // verify if ram contains valid data
    // read random pattern
    puc=(unsigned char *)def_ext_ram_start_address;
    srand(def_uc_seed_test_retention);
    i=0;
    while (i<def_ext_ram_size_bytes/sizeof(unsigned char)){
        c_written=rand()&0xFF;
        c_read=*puc++;
        if (c_read!=c_written){
            ui_an_error_found=1;
            ui_num_error_found++;
            memset(msg,0,sizeof(msg));
            strcpy(msg,"Retention error");
            v_print_msg(row++,msg);
            break;
        }
        i++;
    }
    // reinit the retention data
    if (ui_an_error_found){
        // write pseudo-random pattern
        puc=(unsigned char *)def_ext_ram_start_address;
        srand(def_uc_seed_test_retention);
        i=0;
        while (i<def_ext_ram_size_bytes/sizeof(unsigned char)){
            c_written=rand()&0xFF;
            *puc++=c_written;
            i++;
        }    
        memset(msg,0,sizeof(msg));
        strcpy(msg,"Retention reinitialized!");
        v_print_msg(row++,msg);
        retcode=1;
    }
    else{
        memset(msg,0,sizeof(msg));
        strcpy(msg,"Retention ok !!!");
        v_print_msg(row++,msg);    
    }
    return retcode;

}//unsigned int ui_ext_ram_retention_test(void)
