/**************************************************************************
 * Project                      : Mindgrove Silicon
 * Name of the file             : sha256_driver.h
 * Brief Description of file    : Contains the header files for sha256 driver.
 * Name of Author               : Siddhanth Ramani
 * Email ID                     : siddhanth@mindgrovetech.in

 Copyright (C) 2021 Mindgrovetech Pvt. Ltd. All rights reserved.
 ***************************************************************************/
/**
  @file sha256_driver.h
  @brief Contains the header files for sha256 driver.
*/


#ifndef SHA256_DRIVER_INCLUDED
#define SHA256_DRIVER_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

// Includes
#include "io.h"

// Functions
static void input_text_to_sha(__uint64_t *input_text_address_64, char **final_sha_text_dict
                        , int block_message_length_bits, int sha_append_length_bits, int mode);


static void get_sha_append_bits_helper(unsigned char *sha_append_bits, int sha_append_length_bits, 
                                long int input_len_bits);


static unsigned char * get_sha_append_bits(long int input_len_bits, short int sha_append_length_bits);


static unsigned char * get_final_text_block(int final_block_message_length_bits, unsigned char * substring_input_text
                                    , int sha_append_length_bits, unsigned char * sha_append_bits);


static unsigned char *get_output(unsigned char * sha_output, volatile __uint64_t * output_reg_address_64);


void SHA256_Single_Run(unsigned char *sha_output, unsigned char *input_text, long int input_len_bits);


int SHA256_Multi_Run(unsigned char *sha_output 
                                        , unsigned char *input_text, long int input_len_bits
                                        , long int total_length
                                        , long int iterated_length_bits);



#ifdef __cplusplus
}
#endif

#endif
