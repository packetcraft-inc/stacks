/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Linker script describing the memory layout for the nRF51822.
 *
 *  Copyright (c) 2019 Packetcraft, Inc.
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*************************************************************************************************/

OUTPUT_FORMAT ("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
ENTRY(Reset_Handler)
SEARCH_DIR(.)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 512K
  RAM   (rw) : ORIGIN = 0x20000000, LENGTH =  64K
}

STACK_SIZE = 1024;

/* Following linker definitions are available for use by application software.
 *   __text_start__
 *   __text_end__
 *   __data_start__
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __heap_start__
 *   __heap_end__
 *   __stack_limit__
 *   __stack_top__
 */

SECTIONS
{
  /********************/
  /*** Text Segment ***/
  /********************/

  . = ORIGIN(FLASH);
  __text_start__ = .;

  .text_vect :
  {
    KEEP(*(.isr_vector))
  } > FLASH

  .text :
  {
    *(.text*)

    KEEP(*(.init))
    KEEP(*(.fini))

    *(.rodata*)
  } > FLASH

  . = ALIGN(4);
  __text_end__ = .;

  /********************/
  /*** Data Segment ***/
  /********************/

  .data : AT (__text_end__)
  {
    __data_start__ = .;
    *(.data*)

    . = ALIGN(4);

    __data_end__ = .;
  } > RAM

  /*******************/
  /*** BSS Segment ***/
  /*******************/

  . = ALIGN(4);
  __bss_start__ = .;

  .bss :
  {
    *(.bss*)
    *(COMMON)
  } > RAM

  . = ALIGN(4);
  __bss_end__ = .;

  /*************/
  /*** Stack ***/
  /*************/

  __stack_top__ = ORIGIN(RAM) + LENGTH(RAM);
  __stack_limit__ = __stack_top__ - STACK_SIZE;

  /************/
  /*** Heap ***/
  /************/

  __heap_start__ = __bss_end__;
  __heap_end__ = __stack_limit__;

  ASSERT(__stack_limit__ >= __heap_end__, "RAM region overflowed")
}
