#ifndef def_az4186_emc_h_included
#define def_az4186_emc_h_included

// external ram start address
#define def_ext_ram_start_address 0x81000000
// external ram size [bytes] = 32 kBytes
#define def_ext_ram_size_bytes (32*1024)

// modulo di gestione bus EMC
// nella scheda az4186 il bus EMC viene usato per comunicare copn fpga e con memoria ram esterna
// procedura di inizializzazione di bus di interfaccia esterna
// Abilita l'interfaccia EMC e imposta i wait states in modo da poter comunicare con fpga 
// Attualmente sono supportate solo le frequenze di bus 48MHz e 60MHz
extern void vEMC_az4186_fpga_init(void);

unsigned int ui_ext_ram_retention_test(void);
unsigned int ui_ext_ram_destructive_rw_test(void);
unsigned int ui_enable_external_ram(unsigned int ui_enable);

#endif //#ifndef def_az4186_emc_h_included


