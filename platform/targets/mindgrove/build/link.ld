/**
 * Project                           : Secure IoT SoC
 * Name of the file                  : link.ld
 * Brief Description of file         : This is a linker script to compile code for RAM based application
 * Name of Author                    : Vishwajith.N.S 
 * Email ID                          : vishwajith@mindgrovetech.in
 * 
 * 
 * 
 * @file link.ld
 * @author Vishwajith .N.S (vishwajith@mindgrovetech.in)
 * @brief This is a linker script to compile code for ram based application
 * @version 0.2
 * @date 2024-09-27
 * 
 * @copyright Copyright (c) Mindgrove Technologies Pvt. Ltd 2024. All rights reserved.
 * 
 */

/*************************************************************************************************
1.Text an readonly sections are placed in RAM.
2.sdata ,data ,bss ,sbss are placed in RAM(vma) which are copied from RAM(lma) in startup code.
3.Stack is started from end of RAM uptil 1KB.
4.After Stack end,Heap occupies memory region between end of tbss section and stack section. 
***************************************************************************************************/
OUTPUT_ARCH( "riscv" )
ENTRY(_start)

_STACK_SIZE = 4K;

MEMORY
{
  FLASH (rx)                 : ORIGIN = 0x90001000, LENGTH = 512M
  RAM (rwx)                  : ORIGIN = 0x80000000, LENGTH = 256K
  PSRAM(rwx)                 : ORIGIN = 0xB0000000, LENGTH = 512M
}

SECTIONS
{
  .text.init : 
  { 
    *(.text.init) 
  }>RAM
  
  . = ALIGN(8);
  
  .text : 
  { 
      *(.text)
    . = ALIGN(8);
    *(.text.*)
  }>RAM
  
  . = ALIGN(8);

  .rodata : 
  {
    __rodata_start = .;
    *(.rodata)
    *(.rodata.*)
    *(.gnu.linkonce.r.*)
    __rodata_end = .;
      . = ALIGN(8);
  }>RAM
  _la_ramcode =LOADADDR(.i2c_driver);
  .i2c_driver :
  {
    . = ALIGN(8);
    __ram_code_start = .;
    *(.i2c_driver)
    . = ALIGN(8);
    *(.i2c_driver.*)
    . = ALIGN(8);
    __ram_code_end = .;
  }>RAM AT> RAM
  _la_sdata =LOADADDR(.sdata);
  
  .sdata : 
  {
    __sdata_start = .;
    __global_pointer$ = . + 0x800;
    *(.srodata.cst16) *(.srodata.cst8) *(.srodata.cst4) *(.srodata.cst2) *(.srodata*)
    *(.sdata .sdata.* .gnu.linkonce.s.*)
    __sdata_end = .;
      . = ALIGN(8);
  }>RAM AT> RAM

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
  }>RAM AT> RAM
  
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