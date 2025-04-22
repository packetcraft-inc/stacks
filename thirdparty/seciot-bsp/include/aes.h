/**************************************************************************
 * Project                      : Mindgrove Silicon
 * Name of the file             : aes_driver.h
 * Brief Description of file    : Contains the header files for aes256 driver.
 * Name of Author               : Siddhanth Ramani
 * Email ID                     : siddhanth@mindgrovetech.in

 Copyright (C) 2021 Mindgrovetech Pvt Ltd. All rights reserved.
 ***************************************************************************/
/**
  @file aes_driver.h
  @brief Contains the header files for aes256 driver.
*/

#ifndef AES_DRIVER_INCLUDED
#define AES_DRIVER_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

// Includes
#include "io.h"

typedef enum{
  AES_ENCRYPT,
  AES_DECRYPT
}AES_EXECUTE_MODE;

typedef enum{
  AES_ECB,
  AES_CBC,
  AES_CFB,
  AES_OFB,
  AES_CTR
}AES_TYPE_MODE;


/** @fn void run_ascii_aes_singleshotram(unsigned char *aes_output
                                , unsigned char *input_text, unsigned char *key, unsigned char *iv
                                , long int input_len_bits , long int key_len_bits
                                , int mode, int encrypt_or_decrypt)
 * @brief The main function which runs the AES algorithm on H/W
 * @details This function does a couple of things. 
 * 1. It first creates pointers to AES registers.
 * 2. It then does some checks to ensure the data is as per required.
 * 3. It then sets the configuration reg as required - encrypt/decrypt, key length, mode.
 * 4. It then iterates across text blocks and calls the required functions which will run AES. 
 * The H/W takes care of passing one blocks output to the next block 
 * encryption/decryption calculation.
 * 5. It then continually accumulates the outputs.
 * 6. The final step is returning a pointer to an array of output bits.
 * Note that no resetting of configuration is necessary. The AES hardware resets all configs
 * every time the config registration is filled.
 * @param unsigned char *aes_output : pointer to where the aes output has to be stored
 * @param unsigned char *input_text : pointer to the message that has to be encoded
 * @param unsigned char *key : pointer to the key
 * @param unsigned char *iv : pointer to the iv
 * @param long input input_len_bits : length of message in bits
 * @param long int key_len_bits : length of key in bits
 * @param int mode : AES mode. (same as h/w) mode=0 -> ECB, mode=1 -> CBC, mode=2 -> CFB, mode=3 -> OFB, mode=4 -> CTR
 * @param int encrypt_or_decrypt : (same as h/w) =1 -> decrypt, =0 -> encrypt
 * @return Returns nothing. Changes aes_output inplace.
 */
void AES_Single_Run(unsigned char *aes_output
                                            , unsigned char *input_text, unsigned char *key, unsigned char *iv
                                            , long int input_len_bits , long int key_len_bits
                                            , int mode, int encrypt_or_decrypt);


/** @fn long int run_ascii_aes_multishotram(unsigned char *aes_output
                                , unsigned char *input_text
                                , unsigned char *key
                                , unsigned char *iv 
                                , long int key_len_bits 
                                , int mode
                                , int encrypt_or_decrypt
                                , long int iterated_length_bits)
 * @brief The main function which runs the AES algorithm on H/W
 * @details This function does a couple of things. 
 * 1. It first creates pointers to AES registers.
 * 2. It then does some checks to ensure the data is as per required.
 * 3. It then sets the configuration reg as required - encrypt/decrypt, key length, mode.
 * 4. It then iterates across text blocks and calls the required functions which will run AES. 
 * 5. It then fills the output in the address pointed to by aes_output (an output address
 * accessible and shared by the user).
 * 6. The final step is returning a pointer to an array of output bits. This helps the user 
 * keep track of how many bits have been encrypted/decrypted. It also helps us differentiate
 * the first block (iterated_length_bits=0) from the other blocks. This helps
 * us set the configurations only once, and let the AES hardware know that the other blocks
 * are a continuation of the same encryption/decryption sequence (Resetting configuration implies
 * new sequence of inputs to AES).
 * @param unsigned char *aes_output : pointer to where the aes output has to be stored
 * @param unsigned char *input_text : pointer to the message that has to be encoded
 * @param unsigned char *key : pointer to the key
 * @param unsigned char *iv : pointer to the iv
 * @param long int key_len_bits : length of key in bits
 * @param int mode : AES mode. (same as h/w) mode=0 -> ECB, mode=1 -> CBC, mode=2 -> CFB, mode=3 -> OFB, mode=4 -> CTR
 * @param int encrypt_or_decrypt : (same as h/w) =1 -> decrypt, =0 -> encrypt
 * @param long int iterated_length_bits : number of bits in message which have already been processed by the aes h/w
 * @return Returns the number of bits which have been processed by AES hardware. 
 */
long int AES_Multi_Run(unsigned char *aes_output
                                , unsigned char *input_text
                                , unsigned char *key
                                , unsigned char *iv 
                                , long int key_len_bits 
                                , int mode
                                , int encrypt_or_decrypt
                                , long int iterated_length_bits);


#ifdef __cplusplus
}
#endif

#endif
