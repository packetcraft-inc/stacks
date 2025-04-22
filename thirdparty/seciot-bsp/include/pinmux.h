/****************************************************************************
 * Project               	: shakti devt board
 * Name of the file	        : pinmux.h
 * Brief Description of file    : store pinmux configurations
 * Name of Author    	        : Sathya Narayanan N
 * Email ID                     : sathya281@gmail.com
 
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
*****************************************************************************/
/**
 * @file pinmux.h
 * @brief store pinmux configuration
 */

#ifndef PINMUX_H
#define PINMUX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "io.h"
#include"secure_iot.h"
#include "log.h"

uint32_t PINMUX_EnableGPIO(uint8_t);
uint32_t PINMUX_EnablePWM(uint8_t);
uint32_t ENABLE_Pwm(uint8_t count, uint32_t enable_pwm[]);
uint32_t PINMUX_EnableAllPWM();
uint32_t PINMUS_EnableAllGPIO();

#ifdef __cplusplus
}
#endif

#endif
