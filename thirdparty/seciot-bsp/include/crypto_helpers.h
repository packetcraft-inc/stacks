#ifndef CRYPTO_HELPERS_INCLUDED
#define CRYPTO_HELPERS_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "io.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "crypto_defines.h"


// Check if calloc memory has been allocated
void check_calloc(unsigned char * ptr);


// Getting a non zero 8 bit random number
__uint8_t get_non_zero_rand_8();


// Getting a non zero 64 bit random number
__uint64_t get_non_zero_rand_64();


// Reverse a 64 bit register bytewise
__uint64_t reverse_64bit(__uint64_t input_number);


// Prints string given length with space after every 8 bytes
void test_print(short int strlength, unsigned char * aes_output);


// Prints string given length in bits
void print_test_string(unsigned char *p_to_string, int length_in_bits);


// Prints string given length in dword(64bits)
void print_uchar64(unsigned char *printname, unsigned char *printarg, int len_dword, bool mode);


// Prints string given length in bytes
void print_uchar8(unsigned char *printname, unsigned char *printarg, int len_bytes, bool mode);


// Converts ascii hex to binary hex
void convert_asciihex_to_binaryhex(unsigned char *ascii_hex_pointer, unsigned char *binary_hex_pointer);

#ifdef __cplusplus
}
#endif

#endif
