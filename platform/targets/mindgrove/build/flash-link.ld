/**
 * Project                           : Secure IoT SoC
 * Name of the file                  : flash-linker.ld
 * Brief Description of file         : This is a linker script to compile code for flash based application
 * Name of Author                    : Vishwajith.N.S 
 * Email ID                          : vishwajith@mindgrovetech.in
 * 
 * 
 * 
 * @file flash-linker.ld
 * @author Vishwajith .N.S (vishwajith@mindgrovetech.in)
 * @brief This is a linker script to compile code for flash based application
 * @version 0.2
 * @date 2024-09-27
 * 
 * @copyright Copyright (c) Mindgrove Technologies Pvt. Ltd 2024. All rights reserved.
 * 
 */

/*************************************************************************************************
1.Text an readonly sections are placed in FLASH.
2.sdata ,data ,bss ,sbss are placed in RAM(vma) which are copied from FLASH(lma) in startup code.
3.Stack is started from end of RAM uptil 1KB.
4.After Stack end,Heap occupies memory region between end of tbss section and stack section. 
***************************************************************************************************/
OUTPUT_ARCH( "riscv" )
ENTRY(_start)

_STACK_SIZE = 32K;

MEMORY
{
  FLASH (rx)                 : ORIGIN = 0x90000200, LENGTH = 512M
  RAM (rwx)                  : ORIGIN = 0x80000000, LENGTH = 128K
  PSRAM(rwx)                 : ORIGIN = 0xB0000000, LENGTH = 512M
}

SECTIONS
{
  .text.init : 
  { 
    *(.text.init) 
  }>FLASH
  
  . = ALIGN(8);
  
  .text : 
  { 
    *(.text)
    . = ALIGN(8);
    *(.text.*)
  }>FLASH
  
  . = ALIGN(8);

  .rodata : 
  {
    __rodata_start = .;
    *(.rodata)
    *(.rodata.*)
    *(.gnu.linkonce.r.*)
    __rodata_end = .;
      . = ALIGN(8);
  }>FLASH

  _la_sdata =LOADADDR(.sdata);
  
  .sdata : 
  {
    __sdata_start = .;
    __global_pointer$ = . + 0x800;
    *(.srodata.cst16) *(.srodata.cst8) *(.srodata.cst4) *(.srodata.cst2) *(.srodata*)
    *(.sdata .sdata.* .gnu.linkonce.s.*)
    __sdata_end = .;
      . = ALIGN(8);
  }>RAM AT> FLASH
  
  _la_data =LOADADDR(.data);
  
  .data : 
  {
    __data_start = .;
    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d.*)
        _tls_data = .;
    *(.tdata.begin)
    *(.tdata)
    *(.tdata.end)
          *(.tbss)
      *(.tbss.end)
    _tls_end = .;
    __data_end = .;
  }>RAM AT> FLASH
  
  .sbss : 
  {
    . = ALIGN(8);
    __sbss_start = .;
    *(.sbss)
    *(.sbss.*)
    *(.gnu.linkonce.sb.*)
    __sbss_end = .;
  }>RAM

  .bss : 
  {
     . = ALIGN(8);
     __bss_start = .;
     *(.bss)
     *(.bss.*)
     *(.gnu.linkonce.b.*)
     *(COMMON)
     . = ALIGN(8);
      _end = .;
      __bss_end = .;
  . = ORIGIN(RAM)+LENGTH(RAM) - 8;
  }>RAM
  
 
  .stack : 
  {
    . = ALIGN(8);
   _stack = . ; 
   __stack_pointer$ = . ;
   _stack_end = . - _STACK_SIZE;
  }>RAM
  
  .heap : 
  {
    _HEAP_SIZE  = _stack_end - _end - 8;
    _heap_end = _stack_end - 8;
   _heap = _heap_end - _HEAP_SIZE ;
  }>RAM
}