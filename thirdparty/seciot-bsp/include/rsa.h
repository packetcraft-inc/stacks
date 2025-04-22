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

#ifndef RSA_DRIVER_INCLUDED
#define RSA_DRIVER_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

// Includes
#include "io.h"
// #include <tommath.h>

// Padding mode structure
typedef struct{
    int padding_mode;
    unsigned char * label;    
    __uint64_t label_length;
} struct_rsa_padding;


void RSA_Run(uint8_t *output,uint8_t *input,uint8_t *exp,uint8_t *mod,uint8_t* r2modn);

#ifdef __cplusplus
}
#endif

#endif