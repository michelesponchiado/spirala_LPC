#ifndef def_files_h_included
#define def_files_h_included

// name of the file containing the custom strings*20
#define def_name_file_custom_string_20 "az4186_custom_str_20.txt"
// name of the file containing the custom strings*40
#define def_name_file_custom_string_40 "az4186_custom_str_40.txt"
// name if gthe file containing the private parameters
#define def_name_file_custom_private_params "az4186_private_params.txt"

// write down lcd calibration data
unsigned int ui_write_lcd_calibration_file( char *pbytes, unsigned int ui_num_bytes_2_write);
// read lcd calibration data
unsigned int ui_read_lcd_calibration_file( char *pbytes, unsigned int ui_num_bytes_2_read);

// write down mac params
unsigned int ui_write_macparams_file( char *pbytes, unsigned int ui_num_bytes_2_write);
// read mac params
unsigned int ui_read_macparams_file( char *pbytes, unsigned int ui_num_bytes_2_read);

// read a custom string*20
unsigned char *pRead_custom_string20(unsigned long ulNumStringa,unsigned char *p);
// write a custom string*20
unsigned char *pWrite_custom_string20(unsigned long ulNumStringa,unsigned char *p);
// read a custom string*40
unsigned char *pRead_custom_string40(unsigned long ulNumStringa,unsigned char *p);
// write a custom string*40
unsigned char *pWrite_custom_string40(unsigned long ulNumStringa,unsigned char *p);

// read a custom string*40
unsigned char *pRead_custom_string40(unsigned long ulNumStringa,unsigned char *p);
// write a custom string*40
unsigned char *pWrite_custom_string40(unsigned long ulNumStringa,unsigned char *p);


#endif //#ifndef def_files_h_included
