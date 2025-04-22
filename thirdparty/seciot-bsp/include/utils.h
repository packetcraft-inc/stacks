/****************************************************************************
 * Project           		    : Mindgrove Silicon's Secure-IoT SoC
 * Name of the file	     	    : utils.h
 * Brief Description of file    : Header file for Basic Utilites
 * Name of Author    	        : Kapil Shyam. M
 * Email ID                     : kapil@mindgrovetech.in

 Copyright (C) 2024 Mindgrove Technologies Pvt Ltd. All rights reserved.

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
*******************************************************************************/
/**
 * @file utils.h
 * @brief header file for basic utilities
 * @details This is the header file for util.c
 */

#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stddef.h>
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

unsigned long ReadData(size_t *addr);
void WriteData(size_t *addr, unsigned long val);

#ifdef __cplusplus
}
#endif

#endif
