/*
* @headerfile platform.h - Header file for Mindgrove Silicon Secure-IoT SoC

* Name of Author  : Kapil Shyam. M
* Email ID        : kapil@mindgrovetech.in

 Copyright (C) 2023 Mindgrove Technologies Private Limited. All rights reserved.

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
*/
/*********c-rv64imafdc*********
 SPI   * 4
 GPIO  * 32
 UART  * 3
 I2C   * 2
 PLIC  * 1
 CLINT * 1
 XADC  * 1
 DDR   : 256MB
 Boot
 PWM   : 8
 pinmux
************************************/
/**
 * @file platform.h
 * @brief Header file for the Platform Secure-IOT Processor
 * @details This is the header file for the platform device Secure-IOT Device. Here the definitions for 
 *  all the Peripherals and their addresses are declared.
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#include "pll.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 *@brief RISCV - C CLASS SOC Memory mapping
 */

#define RISCV_PLIC 1

/* Phase Locked Loop (PLL) */
#define PLL_BASE            0x00100000
#define PLL_AXI_LOCK_EN     PLL_BASE
#define PLL_CLOCK_SELECT    0x00100002

/*! Core Local Interruptor CLINT */
#define CLINT_BASE 0x02000000
#define MSIP       0x02000000
#define MTIME      0x0200BFF8
#define MTIMECMP   0x02004000
#define CLINT_DIVISOR   256

#define CLOCK_FREQUENCY_SIM 1000000000
#define CLOCK_FREQUENCY_ASIC 700000000
#define CLOCK_FREQUENCY 40000000
#define DELAY_FREQ_BASE CLOCK_FREQUENCY

  /* Clock Frequency */

// #ifdef FPGA
//   #define CLOCK_FREQUENCY    CLOCK_FREQUENCY_FPGA
// #elif ASIC
//   #define CLOCK_FREQUENCY    (uint64_t)(pll_clock_check() * 1000000)
// #endif
  
//#define CLOCK_FREQUENCY_SIM 1000000000
//#define CLOCK_FREQUENCY_ASIC 700000000
//#define CLOCK_FREQUENCY_FPGA 40000000
//#define DELAY_FREQ_BASE CLOCK_FREQUENCY_FPGA

/*!Debugger Offset */
#define DBG_MEM_START 0x00000010

/*!Tightly Coupled Memory(TCM) Offset. Size 256mB */
#define TCM_MEM_START 0x80000000 /*! DDR3 Memory Start address */
#define TCM_MEM_SIZE 0x10000000 /*! DDR3 Memory Size */

/*!Percentage of Free Memory to be used as stack (0-100). The remaining space will be used by heap */
#define STACK_PERCENTAGE 50 /*! DDR3 Memory Size */

/*!Percentage of Free Memory to be used as stack (0-100). The remaining space will be used by heap */
#define HEAP_PERCENTAGE 50 /*! DDR3 Memory Size */

/*!Pulse Width Modulation Start Offsets */
#define PWM_MAX_COUNT 8
#define PWM_BASE_ADDRESS 0x00030000 /*PWM Base address*/
#define PWM_END_ADDRESS 0x000307FF /*PWM End address*/

#define PWM_MODULE_OFFSET 0x00000100 /*Offset value to be incremented for each interface*/

/*!Quad Serial Peripheral Interface Offsets */

#define QSPI_MAX_COUNT 2

#define QSPI0_BASE_ADDRESS  0x00040000 /*! Standard Quad Serial Peripheral Interface Base address*/
#define QSPI0_END_ADDRESS   0x000400FF
#define QSPI1_BASE_ADDRESS  0x00040100
#define QSPI1_END_ADDRESS    0x000401FF /*! Standard Quad Serial Peripheral Interface End address*/

#define QSPI_OFFSET 0x100

/*!Serial Peripheral Interface Offsets */
#define SPI0_START 0x00020000 /* Serial Peripheral Interface 0 */
#define SPI1_START 0x00020100 /* Serial Peripheral Interface 1 */
#define SPI2_START 0x00020200 /* Serial Peripheral Interface 2 */
#define SPI3_START 0x00020300 /* Serial Peripheral Interface 3 */

/* Struct to access SSPI registers as 32 bit registers */

#define SSPI_MAX_COUNT  4 /*! Number of Standard SSPI used in the SOC */

#define SSPI0_BASE_ADDRESS  0x00020000 /*! Standard Serial Peripheral Interface 0 Base address*/
#define SSPI0_END_ADDRESS  0x000200FF /*! Standard Serial Peripheral Interface 0 Base address*/

#define SSPI1_BASE_ADDRESS  0x00020100 /*! Standard Serial Peripheral Interface 1 Base address*/
#define SSPI1_END_ADDRESS  0x000201FF /*! Standard Serial Peripheral Interface 1 Base address*/

#define SSPI2_BASE_ADDRESS  0x00020200 /*! Standard Serial Peripheral Interface 2 Base address*/
#define SSPI2_END_ADDRESS  0x000202FF /*! Standard Serial Peripheral Interface 2 Base address*/

#define SSPI3_BASE_ADDRESS  0x00020300 /*! Standard Serial Peripheral Interface 3 Base address*/
#define SSPI3_END_ADDRESS  0x000203FF /*! Standard Serial Peripheral Interface 3 Base address*/

#define SSPI_BASE_OFFSET 0x100


/*!Universal Synchronous Receiver Transmitter Interface Offsets */

#define MAX_UART_COUNT 3

#define UART0_START 0x00011300 /*! UART 0 */
#define UART0_END 0x00011340 /*! UART 0 */

#define UART1_START 0x00011400 /*! UART 1 */
#define UART1_END 0x00011440 /*! UART 1 */

#define UART2_START 0x00011500 /*! UART 2 */
#define UART2_END 0x00011540 /*! UART 2 */

#define UART_OFFSET 0x100

/*! Inter Integrated Circuit (I2C) Interface */

#define MAX_I2C_COUNT 2

#define I2C0_BASE 0x00044000 /*! I2C0 Start Address */
#define I2C0_END 0x000440FF /*! I2C0 End Address */

#define I2C1_BASE 0x00044100 /*! I2C1 Start Address */
#define I2C1_END 0x000441FF /*! I2C1 End Address */

#define I2C_OFFSET 0x100

/*! pinmux*/
#define PINMUX_START 0x40300 
#define PINMUX_CONFIGURE_REG 0x40300

/*! Programmable Logic Interrupt Interface */
#define PLIC_BASE_ADDRESS 0x0C000000 /*! PLIC Interface Start */
#define PLIC_END_ADDRESS 0x0FFFFFFF /*! PLIC Interface End */

#define GPIO_INTERRUPT_0      1	 /* GPIO 0  */
#define GPIO_INTERRUPT_1      2	 /* GPIO 1  */
#define GPIO_INTERRUPT_2      3	 /* GPIO 2  */
#define GPIO_INTERRUPT_3      4	 /* GPIO 3  */
#define GPIO_INTERRUPT_4      5	 /* GPIO 4  */
#define GPIO_INTERRUPT_5      6	 /* GPIO 5  */
#define GPIO_INTERRUPT_6      7	 /* GPIO 6  */
#define GPIO_INTERRUPT_7      8	 /* GPIO 7  */
#define GPIO_INTERRUPT_8      9	 /* GPIO 8  */
#define GPIO_INTERRUPT_9      10	 /* GPIO 9  */
#define GPIO_INTERRUPT_10     11	 /* GPIO 10 */
#define GPIO_INTERRUPT_11     12	 /* GPIO 11 */
#define GPIO_INTERRUPT_12     13	 /* GPIO 12 */
#define GPIO_INTERRUPT_13     14	 /* GPIO 13 */
#define GPIO_INTERRUPT_14     15	 /* GPIO 14 */
#define GPIO_INTERRUPT_15     16	 /* GPIO 15 */
#define GPIO_INTERRUPT_16     17	 /* GPIO 16  */
#define GPIO_INTERRUPT_17     18	 /* GPIO 17  */
#define GPIO_INTERRUPT_18     19	 /* GPIO 18  */
#define GPIO_INTERRUPT_19     20	 /* GPIO 19  */
#define GPIO_INTERRUPT_20     21	 /* GPIO 20  */
#define GPIO_INTERRUPT_21     22	 /* GPIO 21 */
#define GPIO_INTERRUPT_22     23	 /* GPIO 22 */
#define GPIO_INTERRUPT_23     24	 /* GPIO 23 */
#define GPIO_INTERRUPT_24     25  /* GPIO 24 */
#define GPIO_INTERRUPT_25     26  /* GPIO 25 */
#define GPIO_INTERRUPT_26     27  /* GPIO 26 */
#define GPIO_INTERRUPT_27     28  /* GPIO 27 */
#define GPIO_INTERRUPT_28     29  /* GPIO 28 */
#define GPIO_INTERRUPT_29     30  /* GPIO 29 */
#define GPIO_INTERRUPT_30     31  /* GPIO 30 */
#define GPIO_INTERRUPT_31     32  /* GPIO 31 */
#define PWM_INTERRUPT_7       33  /* PWM 7 */
#define PWM_INTERRUPT_6       34  /* PWM 6 */
#define PWM_INTERRUPT_5       35  /* PWM 5 */
#define PWM_INTERRUPT_4       36  /* PWM 4 */
#define PWM_INTERRUPT_3       37  /* PWM 3 */
#define PWM_INTERRUPT_2       38  /* PWM 2 */
#define PWM_INTERRUPT_1       39  /* PWM 1 */
#define PWM_INTERRUPT_0       40  /* PWM 0 */
#define GPTIMER_INTERRUPT_0   41  /* GPTIMER 0 */
#define GPTIMER_INTERRUPT_1   42  /* GPTIMER 1 */
#define GPTIMER_INTERRUPT_2   43  /* GPTIMER 2 */
#define GPTIMER_INTERRUPT_3   44  /* GPTIMER 3 */
#define I2C_INTERRUPT_0       45  /* I2C 0 */
#define I2C_INTERRUPT_1       46  /* I2C 1 */
#define UART_INTERRUPT_0      47  /* UART 0 */
#define UART_INTERRUPT_1      48  /* UART 1 */
#define UART_INTERRUPT_2      49  /* UART 2 */
#define QSPI_INTERRUPT_1      50  /* QSPI 1 */
#define QSPI_READY_1          51
#define QSPI_INTERRUPT_0      52  /* QSPI 0 */
#define QSPI_READY_0          53                    
#define SPI_INTERRUPT_3       54  /* SPI 3 */
#define SPI_INTERRUPT_2       55  /* SPI 2 */
#define SPI_INTERRUPT_1       56  /* SPI 1 */
#define SPI_INTERRUPT_0       57  /* SPI 0 */  
#define ADC_INTERRUPT         58  /* ADC  */

#define PLIC_MAX_INTERRUPT_SRC 58

/*! SHA256 Interface */
#define SHA256_BASE_ADDRESS 0x03000000 /*! SHA256 Interface Start */
#define SHA256_END_ADDRESS 0x030000FF /*! SHA256 Interface Start */

/*! AES Interface */
#define AES_BASE_ADDRESS 0x04000000 /*! AES Interface Start */
#define AES_END_ADDRESS 0x040000FF /*! AES Interface Start */

/*! RSA Interface */
#define RSA_BASE_ADDRESS 0x05000000 /*! RSA Interface Start */
#define RSA_END_ADDRESS 0x050000FF /*! RSA Interface Start */

/*!General Purpose Input / Output */
#define GPIO_START 0x00040200 //GPIO Start Address */
#define GPIO_OFFSET 0x08 /*!Generic offset used to access GPIO registers*/
#define PLIC_GPIO_OFFSET 6

/*
 * General Purpose IOs supported
 */

#define GPIO0  (1 <<  0)
#define GPIO1  (1 <<  1)
#define GPIO2  (1 <<  2)
#define GPIO3  (1 <<  3)
#define GPIO4  (1 <<  4)
#define GPIO5  (1 <<  5)
#define GPIO6  (1 <<  6)
#define GPIO7  (1 <<  7)
#define GPIO8  (1 <<  8)
#define GPIO9  (1 <<  9)
#define GPIO10 (1 << 10)
#define GPIO11 (1 << 11)
#define GPIO12 (1 << 12)
#define GPIO13 (1 << 13)
#define GPIO14 (1 << 14)
#define GPIO15 (1 << 15)
#define GPIO16 (1 << 16)
#define GPIO17 (1 << 17)
#define GPIO18 (1 << 18)
#define GPIO19 (1 << 19)
#define GPIO20 (1 << 20)
#define GPIO21 (1 << 21)
#define GPIO22 (1 << 22)
#define GPIO23 (1 << 23)
#define GPIO24 (1 << 24)
#define GPIO25 (1 << 25)
#define GPIO26 (1 << 26)
#define GPIO27 (1 << 27)
#define GPIO28 (1 << 28)
#define GPIO29 (1 << 29)
#define GPIO30 (1 << 30)
#define GPIO31 (1 << 31)
#define GPIO_COUNT  0x20

#ifdef __cplusplus
}
#endif

#endif
