/***************************************************************************
 * Project                          : shakti devt board
 * Name of the file                 : traps.h
 * Brief Description of file        : Header file for handling traps
 * Name of Author                   : Sathya Narayanan N
 * Email ID                         : sathya281@gmail.com

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
*******************************************************************************/
/**
 * @file traps.h
 * @brief Header file for handling traps
 * @details This is the header file for traps.c. Here the definitions for
 * handling traps are declared.
 */

#ifndef TRAPS_H
#define TRAPS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*
   Table 3.6 risc v priv spec v1.10 chapter 7

   -------------------------------------------------------------
   Interrupt  Exception Code   Description
   -------------------------------------------------------------
   1           0                User software interrupt
   1           1                Supervisor software interrupt
   1           2                Reserved
   1           3                Machine software interrupt
   1           4                User timer interrupt
   1           5                Supervisor timer interrupt
   1           6                Reserved
   1           7                Machine timer interrupt
   1           8                User external interrupt
   1           9                Supervisor external interrupt
   1           10               Reserved
   1           11               Machine external interrupt
   1           ≥12              Reserved
   0           0                Instruction address misaligned
   0           1                Instruction access fault
   0           2                Illegal instruction
   0           3                Breakpoint
   0           4                Load address misaligned
   0           5                Load access fault
   0           6                Store/AMO address misaligned
   0           7                Store/AMO access fault
   0           8                Environment call from U-mode
   0           9                Environment call from S-mode
   0           10               Reserved
   0           11               Environment call from M-mode
   0           12               Instruction page fault
   0           13               Load page fault
   0           14               Reserved
   0           15               Store/AMO page fault
   0           ≥16              Reserved
   ---------------------------------------------------------------

   Off the 32 entries in the table, some index positions reserved for future use
 */

#define MAX_MCAUSE_VALUE               32


/*  Interrupts */
#define MAX_INTERRUPT_VALUE            16

#define USER_SW_INTERRUPT               0
#define SUPER_SW_INTERRUPT              1
#define RESERVED_INTERRUPT0             2
#define MACH_SW_INTERRUPT               3
#define USER_TIMER_INTERRUPT            4
#define SUPER_TIMER_INTERRUPT           5
#define RESERVED_INTERRUPT1             6
#define MACH_TIMER_INTERRUPT            7
#define USER_EXT_INTERRUPT              8
#define SUPERVISOR_EXT_INTERRUPT        9
#define RESERVED_INTERRUPT2            10
#define MACH_EXTERNAL_INTERRUPT        11
#define RESERVED_INTERRUPT3            12
#define RESERVED_INTERRUPT4            13
#define RESERVED_INTERRUPT5            14
#define RESERVED_INTERRUPT6            15

/* Traps */
#define MAX_TRAP_VALUE                 16

typedef enum {
    INSTRUCTION_ADDRESS_MISALIGNED = 0,
    INSTRUCTION_ACCESS_FAULT,
    ILLEGAL_INSTRUCTION,
    BREAKPOINT,
    LOAD_ADDRESS_MISALIGNED,
    LOAD_ACCESS_FAULT,
    STORE_AMO_ADDRESS_MISALIGNED,
    STORE_AMO_ACCESS_FAULT,
    ENVIRONMENT_CALL_FROM_U_MODE,
    ENVIRONMENT_CALL_FROM_S_MODE,
    RESERVED_TRAP1,
    ENVIRONMENT_CALL_FROM_M_MODE,
    INSTRUCTION_PAGE_FAULT,
    LOAD_PAGE_FAULT,
    RESERVED_TRAP2,
    STORE_AMO_PAGE_FAULT
} MCAUSE_REASON;

/*
   Trap table -  Each entry in the table corresponds to a service routine for a Trap
 */
extern uint16_t interrupt_id;
typedef void (*mtrap_fptr_t) (uintptr_t fptr);
extern mtrap_fptr_t mcause_trap_table[MAX_TRAP_VALUE];
extern mtrap_fptr_t mcause_interrupt_table[MAX_INTERRUPT_VALUE];

void default_handler(uintptr_t cause);
unsigned int extract_ie_code(unsigned int num);
uintptr_t Trap_Handler(uintptr_t cause, uintptr_t epc);


#ifdef __cplusplus
}
#endif

#endif
