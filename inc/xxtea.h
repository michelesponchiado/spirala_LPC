typedef enum{
	enum_xxtea_encode_return_code_ok,
	enum_xxtea_encode_return_code_invalid_word_num,
	enum_xxtea_encode_return_code_numof
}enum_xxtea_encode_return_code;

typedef enum{
	enum_xxtea_decode_return_code_ok,
	enum_xxtea_decode_return_code_invalid_word_num,
	enum_xxtea_decode_return_code_numof
}enum_xxtea_decode_return_code;

//encode part
// v: vector to encode: the vector is encoded in place
// n: number of 32-bit words to encode; should be at least 2
// key: pointer to an array of 4 32-bit word containing the encoding key
enum_xxtea_encode_return_code xxtea_encode(unsigned int *v, unsigned int n, unsigned int const *key);
// decode part
// v: vector to decode: the vector is decoded in place
// n: number of 32-bit words to decode; should be at least 2
// key: pointer to an array of 4 32-bit word containing the decoding key
enum_xxtea_decode_return_code xxtea_decode(unsigned int *v, unsigned int n, unsigned int const *key);
