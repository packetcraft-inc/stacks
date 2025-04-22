/**************************************************************************
 * Project                      : Mindgrove Silicon
 * Name of the file             : rsa_driver.h
 * Brief Description of file    : Contains the header files for rsa driver.
 * Name of Author               : Siddhanth Ramani
 * Email ID                     : siddhanth@mindgrovetech.in

 Copyright (C) 2021 Mindgrovetech Pvt Ltd. All rights reserved.
 ***************************************************************************/
/**
  @file rsa_driver.h
  @brief Contains the header files for rsa driver.
*/

#ifndef RSA_PADDING_DRIVER_INCLUDED
#define RSA_PADDING_DRIVER_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

// Includes
#include "io.h"
#include <tommath.h>
#include "rsa.h"
mp_int get_R2modN(unsigned char * char_N, long int mod_len);


__uint8_t * I20SP(__uint32_t x, __uint8_t x_len);


void MGF1(unsigned char * mask, unsigned char * mgf_seed, long int mgf_seed_len, long int mask_len);


void create_db(unsigned char *db, unsigned char * l_hash, unsigned char * input_text, long int m_len);


void pad_rsa_input_pkcs15(__uint64_t *load_input, unsigned char *input_text, long int input_len_bits, struct_rsa_padding rsa_padding);


void pad_rsa_input_oaep(__uint64_t *load_input, unsigned char *input_text
                        , long int input_len_bits, struct_rsa_padding rsa_padding);


void pad_rsa_input(__uint64_t *load_input, unsigned char *input_text, long int input_len_bits, struct_rsa_padding rsa_padding);


void convert_60_to_64(__uint64_t *R2modN_64, unsigned short int mod_len_bits);



void print_dp(mp_digit * dp);


long int get_char_length(unsigned char *char_array_pointer);


void decode_rsa_output_pkcs15(__uint64_t *rsa_output);


void decode_rsa_output_oaep(__uint64_t *rsa_output, struct_rsa_padding rsa_padding);


void decode_rsa_output(__uint64_t *rsa_output, struct_rsa_padding rsa_padding);


void testing_encoding_decoding_rev_message(__uint64_t * rsa_output, __uint64_t * load_input);

void decode_rsa_output_pkcs15_from_sign(__uint64_t *rsa_output,uint8_t *decode_output);


#ifdef __cplusplus
}
#endif

#endif
