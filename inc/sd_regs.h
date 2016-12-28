// registro CSD dell'SD card
// pag 108 del documento "Part_1_Physical_Layer_Simplified_Specification_Ver_3.01_Final_100518.pdf"
typedef struct _Tipo_Struct_CSD{
	unsigned char always_1:1;				// 1b - [0:0]
	unsigned char CRC:7;					// xxxxxxxb R/W [7:1]

	unsigned char reserved_6:2;			// 00b R/W [9:8]
	unsigned char FILE_FORMAT:2;			// xxb R/W(1) [11:10]
	unsigned char TMP_WRITE_PROTECT:1;	// xb R/W [12:12]
	unsigned char PERM_WRITE_PROTECT:1;	// xb R/W(1) [13:13]
	unsigned char COPY:1;				// xb R/W(1) [14:14]
	unsigned char FILE_FORMAT_GRP:1;	// xb R/W(1) [15:15]

	unsigned char reserved_5:5;		// 00000b R [20:16]
	unsigned char WRITE_BL_PARTIAL:1;	// xb R [21:21]
	unsigned char WRITE_BL_LEN_1:2;		// xxxxb R [25:22]

	unsigned char WRITE_BL_LEN_2:2;		// xxxxb R [25:22]
	unsigned char R2W_FACTOR:3;		// xxxb R [28:26]
	unsigned char reserved_3:2;		// 00b R [30:29]
	unsigned char WP_GRP_ENABLE:1;	// xb R [31:31]

	unsigned char WP_GRP_SIZE_1:2;	// xxxxxxxb R [38:32]
	unsigned char WP_GRP_SIZE_2:5;	// xxxxxxxb R [38:32]
	unsigned char SECTOR_SIZE_1:1;		// xxxxxxxb R [45:39]

	unsigned char SECTOR_SIZE_2:6;		// xxxxxxxb R [45:39]
	unsigned char ERASE_BLK_EN:1;		// xb R [46:46]
	unsigned char C_SIZE_MULT_1:1;		// xxxb R [49:47]

	unsigned char C_SIZE_MULT_2:2;		// xxxb R [49:47]
	unsigned char VDD_W_CURR_MAX:3;	// xxxb R [52:50]
	unsigned char VDD_W_CURR_MIN_1:2;	// xxxb R [55:53]
	unsigned char VDD_W_CURR_MIN_2:1;	// xxxb R [55:53]

	unsigned char VDD_R_CURR_MAX:3;	// xxxb R [58:56]
	unsigned char VDD_R_CURR_MIN:3;	// xxxb R [61:59]
	unsigned char C_SIZE_1:2;			// xxxh R [73:62]

	unsigned char C_SIZE_2:8;			// xxxh R [73:62]
	unsigned char C_SIZE_3:2;			// xxxh R [73:62]

	unsigned char reserved_2:2;		// 00b R [75:74]
	unsigned char DSR_IMP:1;			// xb R [76:76]
	unsigned char READ_BLK_MISALIGN:1;	// xb R [77:77]
	unsigned char WRITE_BLK_MISALIGN:1;	// xb R [78:78]
	unsigned char READ_BL_PARTIAL:1;	// 1b R [79:79]

	unsigned char READ_BL_LEN:4;		// xh R [83:80]
	unsigned char CCC_1:4;				// 01x110110101b R [95:84]
	unsigned char CCC_2:8;				// 01x110110101b R [95:84]

	unsigned char TRAN_SPEED:8;		// 32h or 5Ah R [103:96]
	unsigned char NSAC:8;				// xxh R [111:104]
	unsigned char TAAC:8;				// xxh R [119:112]
	unsigned char reserved:6;			// 00 0000b R [125:120]
	unsigned char CSD_STRUCTURE:2;	// 00b R [127:126]
}Tipo_Struct_CSD;

typedef struct _Tipo_Struct_CSD_V2{
	unsigned char always_1:1;				// 1b - [0:0]
	unsigned char CRC:7;					// xxxxxxxb R/W [7:1]

	unsigned char reserved_6:2;			// 00b R/W [9:8]
	unsigned char FILE_FORMAT:2;			// xxb R/W(1) [11:10]
	unsigned char TMP_WRITE_PROTECT:1;	// xb R/W [12:12]
	unsigned char PERM_WRITE_PROTECT:1;	// xb R/W(1) [13:13]
	unsigned char COPY:1;				// xb R/W(1) [14:14]
	unsigned char FILE_FORMAT_GRP:1;	// xb R/W(1) [15:15]

	unsigned char reserved_5:5;		// 00000b R [20:16]
	unsigned char WRITE_BL_PARTIAL:1;	// xb R [21:21]
	unsigned char WRITE_BL_LEN_1:2;		// xxxxb R [25:22]

	unsigned char WRITE_BL_LEN_2:2;		// xxxxb R [25:22]
	unsigned char R2W_FACTOR:3;		// xxxb R [28:26]
	unsigned char reserved_3:2;		// 00b R [30:29]
	unsigned char WP_GRP_ENABLE:1;	// xb R [31:31]

	unsigned char WP_GRP_SIZE_1:2;	// xxxxxxxb R [38:32]
	unsigned char WP_GRP_SIZE_2:5;	// xxxxxxxb R [38:32]
	unsigned char SECTOR_SIZE_1:1;		// xxxxxxxb R [45:39]

	unsigned char SECTOR_SIZE_2:6;		// xxxxxxxb R [45:39]
	unsigned char ERASE_BLK_EN:1;		// xb R [46:46]
	unsigned char reserved__1:1;		    // xb R [47:47]
	
	unsigned char C_SIZE_MULT_1:8;		// xxxb R [55:48]
	unsigned char C_SIZE_MULT_2:8;		// xxxb R [63:56]

	unsigned char C_SIZE_MULT_3:6;		// xxxb R [69:64]
	unsigned char reserved__6:6;		    // xb R [75:70]
	unsigned char DSR_IMP:1;			// xb R [76:76]
	
	unsigned char READ_BLK_MISALIGN:1;	// xb R [77:77]
	unsigned char WRITE_BLK_MISALIGN:1;	// xb R [78:78]
	unsigned char READ_BL_PARTIAL:1;	// 1b R [79:79]

	unsigned char READ_BL_LEN:4;		// xh R [83:80]
	unsigned char CCC_1:4;				// 01x110110101b R [95:84]
	unsigned char CCC_2:8;				// 01x110110101b R [95:84]

	unsigned char TRAN_SPEED:8;		// 32h or 5Ah R [103:96]
	unsigned char NSAC:8;				// xxh R [111:104]
	unsigned char TAAC:8;				// xxh R [119:112]
	unsigned char reserved:6;			// 00 0000b R [125:120]
	unsigned char CSD_STRUCTURE:2;	// 00b R [127:126]
}Tipo_Struct_CSD_V2;



//memory capacity = BLOCKNR * BLOCK_LEN
//Where
//MULT		= 2^(C_SIZE_MULT+2)		, (C_SIZE_MULT < 8)
//BLOCKNR	= (C_SIZE+1) * MULT
//BLOCK_LEN = 2^(READ_BL_LEN)		, (READ_BL_LEN < 12)

typedef struct _Tipo_Struct_SD{
	unsigned long ulMemoryCapacity_Bytes;
	unsigned long ulMemoryCapacity_KBytes;
	unsigned long ulMemoryCapacity_MBytes;
	unsigned long ulNumberOfBlocks;
	unsigned long ulSizeOfBlocks;
	Tipo_Struct_CSD csd;

}Tipo_Struct_SD;

extern Tipo_Struct_SD SD;