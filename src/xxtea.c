// BTEA will encode or decode n words as a single block where n > 1

//    v is the n word data vector
//    k is the 4 word key
//    n is negative for decoding
//    if n is zero result is 1 and no coding or decoding takes place, otherwise the result is zero
//    assumes 32 bit 'long' and same endian coding and decoding

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <ctype.h>

#include "spiext.h"
#include "spiprot.h"
#include "spimsg.h"
#include "spipass.h"
#include "spifmt.h"
#include "max519.h"
#include "lcd.h"
#include "minmax.h"
#include "gui.h"
#include "spiwin.h"
#include "i2c_master.h"
#include "vdip.h"
#include "rtcDS3231.h"
#include "file_list.h"
#include "az4186_const.h"
#include "az4186_program_codes.h"
#include "az4186_commesse.h"
#include "az4186_spiralatrice.h"
#include "xxtea.h"


#define DELTA 0x9e3779b9
#define MX (((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (key[(p&3)^e] ^ z)))



//encode part
// v: vector to encode: the vector is encoded in place
// n: number of 32-bit words to encode; should be at least 2
// key: pointer to an array of 4 32-bit word containing the encoding key
enum_xxtea_encode_return_code xxtea_encode(unsigned int *v, unsigned int n, unsigned int const *key) {
	unsigned int y, z, sum;
	unsigned p, rounds, e;
	enum_xxtea_encode_return_code return_code;
	// by now, all ok
	return_code=enum_xxtea_encode_return_code_ok;
	if (n<=1){
		return_code=enum_xxtea_encode_return_code_invalid_word_num;
		goto end_of_procedure;
	}
	rounds = 6 + 52/n;
	sum = 0;
	z = v[n-1];
	do {
		sum += DELTA;
		e = (sum >> 2) & 3;
		for (p=0; p<n-1; p++) {
			y = v[p+1]; 
			z = v[p] += MX;
		}
		y = v[0];
		z = v[n-1] += MX;
	} while (--rounds);
end_of_procedure:
	return return_code;
}

// decode part
// v: vector to decode: the vector is decoded in place
// n: number of 32-bit words to decode; should be at least 2
// key: pointer to an array of 4 32-bit word containing the decoding key
enum_xxtea_decode_return_code xxtea_decode(unsigned int *v, unsigned int n, unsigned int const *key) {
	unsigned int y, z, sum;
	unsigned p, rounds, e;
	enum_xxtea_decode_return_code return_code;
	return_code=enum_xxtea_decode_return_code_ok;
	if (n<=1){
		return_code=enum_xxtea_decode_return_code_invalid_word_num;
		goto end_of_procedure;
	}
	rounds = 6 + 52/n;
	sum = rounds*DELTA;
	y = v[0];
	do {
		e = (sum >> 2) & 3;
		for (p=n-1; p>0; p--) {
			z = v[p-1];
			y = v[p] -= MX;
		}
		z = v[n-1];
		y = v[0] -= MX;
	} while ((sum -= DELTA) != 0);
end_of_procedure:
	return return_code;
}