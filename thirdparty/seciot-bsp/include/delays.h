/**
 * Project                           : Secure IoT SoC
 * Name of the file                  : delays.
 * Brief Description of file         : This is the header file for all types of delays from utilites .
 * Name of Author                    : Suneeth Damodharan
 * Email ID                          : suneeth@mindgrovetech.in
 * 
 * @file delays.h
 * @author suneeth@mingrovetech.in 
 * @brief This is the source file for all types of delays
 * @date 2024-08-13
 * 
 * @copyright Copyright (c) Mindgrove Technologies Pvt. Ltd 2024. All rights reserved.
 * 
 */

#ifndef DELAYS_H
#define DELAYS_H

#include "io.h"
#include <stdint.h>
#include "gptimer.h"
#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @fn uint64_t mcycles(void)
* @brief Used to get number of elapsed machine cycles since the start of the current applications.
* @details Copies the value of mcycle register to C variable
* @param void.
*/
static inline uint64_t mcycles(void);

/**
* @fn void delayms(long delay)
* @brief Used to create delay in milliseconds.
* @details Sends NOP instruction till the specified delay is achieved.
* @param delay Delay in milliseconds.

* @note DELAY_FREQ_BASE macro must be set to the used clock frequency.
*/
void delay_ms(uint32_t milliseconds);

/**
* @fn void delayus(long delay)
* @brief Used to create delay in microseconds.
* @details Sends NOP instruction till the specified delay is achieved.
* @param delay Delay in microseconds.

* @note DELAY_FREQ_BASE macro must be set to the used clock frequency.
*/
void delayus(long delay);


void delay_gptimer(uint32_t delay_time);

#ifdef __cplusplus
}
#endif

#endif
