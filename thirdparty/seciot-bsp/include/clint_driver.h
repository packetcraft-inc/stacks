/***************************************************************************
* Project           		:  shakti devt board
* Name of the file	     	:  clint_driver.h
* Brief Description of file     :  Header file for clint.
* Name of Author    	        :  Sathya Narayanan N
* Email ID                      :  sathya281@gmail.com

Copyright (C) 2019  IIT Madras. All rights reserved.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
***************************************************************************/
/**
 * @file clint_driver.h
 * @brief  Header file for clint
 * @details This is the header file for clint_driver.c
 */

#ifndef CLIC_DRIVER_H
#define CLIC_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "traps.h"

#define MSIP       0x02000000
#define MTIME      0x0200BFF8
#define MTIMECMP   0x02004000
#define CLINT_DIVISOR   256

extern volatile uint64_t* mtime;
extern volatile uint64_t* mtimecmp;

/* function prototype */
uint64_t Get_MTIME(void);
inline void Config_Counter(uint64_t value);
inline void CLINT_Timer(uint64_t timer_value);
inline void CLINT_Handler(uintptr_t int_id, uintptr_t epc);

#ifdef __cplusplus
}
#endif

#endif
