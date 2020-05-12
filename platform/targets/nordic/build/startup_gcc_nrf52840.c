/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  nRF52840 system startup for GCC compiler directives.
 *
 *  Copyright (c) 2019-2020 Packetcraft, Inc.
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

#include "pal_led.h"
#include "pal_sys.h"
#include "nrf.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief      Stack initial values. */
#define INIT_STACK_VAL    0xAFAFAFAF

/*! Linker memory section definition. */
#define SECTION(x)  __attribute__ ((section(x)))

/*! Weak symbol reference. */
#define WEAK        __attribute__ ((weak))

/**************************************************************************************************
  Functions
**************************************************************************************************/

extern int main(void);
extern void SystemInit(void);
static void SystemDefaultHandler(void);

/* Core vectors. */
void WEAK Reset_Handler(void);
void WEAK NMI_Handler(void);
void WEAK HardFault_Handler(void);
void WEAK MemoryManagement_Handler(void);
void WEAK BusFault_Handler(void);
void WEAK UsageFault_Handler(void);
void WEAK SVC_Handler(void);
void WEAK DebugMon_Handler(void);
void WEAK PendSV_Handler(void);
void WEAK SysTick_Handler(void);
void WEAK POWER_CLOCK_IRQHandler(void);
void WEAK RADIO_IRQHandler(void);
void WEAK UARTE0_UART0_IRQHandler(void);
void WEAK SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQHandler(void);
void WEAK SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQHandler(void);
void WEAK NFCT_IRQHandler(void);
void WEAK GPIOTE_IRQHandler(void);
void WEAK SAADC_IRQHandler(void);
void WEAK TIMER0_IRQHandler(void);
void WEAK TIMER1_IRQHandler(void);
void WEAK TIMER2_IRQHandler(void);
void WEAK RTC0_IRQHandler(void);
void WEAK TEMP_IRQHandler(void);
void WEAK RNG_IRQHandler(void);
void WEAK ECB_IRQHandler(void);
void WEAK CCM_AAR_IRQHandler(void);
void WEAK WDT_IRQHandler(void);
void WEAK RTC1_IRQHandler(void);
void WEAK QDEC_IRQHandler(void);
void WEAK COMP_LPCOMP_IRQHandler(void);
void WEAK SWI0_EGU0_IRQHandler(void);
void WEAK SWI1_EGU1_IRQHandler(void);
void WEAK SWI2_EGU2_IRQHandler(void);
void WEAK SWI3_EGU3_IRQHandler(void);
void WEAK SWI4_EGU4_IRQHandler(void);
void WEAK SWI5_EGU5_IRQHandler(void);
void WEAK TIMER3_IRQHandler(void);
void WEAK TIMER4_IRQHandler(void);
void WEAK PWM0_IRQHandler(void);
void WEAK PDM_IRQHandler(void);
void WEAK MWU_IRQHandler(void);
void WEAK PWM1_IRQHandler(void);
void WEAK PWM2_IRQHandler(void);
void WEAK SPIM2_SPIS2_SPI2_IRQHandler(void);
void WEAK RTC2_IRQHandler(void);
void WEAK I2S_IRQHandler(void);
void WEAK FPU_IRQHandler(void);
void WEAK USBD_IRQHandler(void);
void WEAK UARTE1_IRQHandler(void);
void WEAK QSPI_IRQHandler(void);
void WEAK CRYPTOCELL_IRQHandler(void);
void WEAK PWM3_IRQHandler(void);
void WEAK SPIM3_IRQHandler(void);

/* Assign default weak references. Override these values by defining a new function with the same name. */
#pragma weak NMI_Handler                                    = SystemDefaultHandler
#pragma weak HardFault_Handler                              = SystemDefaultHandler
#pragma weak MemoryManagement_Handler                       = SystemDefaultHandler
#pragma weak BusFault_Handler                               = SystemDefaultHandler
#pragma weak UsageFault_Handler                             = SystemDefaultHandler
#pragma weak SVC_Handler                                    = SystemDefaultHandler
#pragma weak DebugMon_Handler                               = SystemDefaultHandler
#pragma weak PendSV_Handler                                 = SystemDefaultHandler
#pragma weak SysTick_Handler                                = SystemDefaultHandler
#pragma weak POWER_CLOCK_IRQHandler                         = SystemDefaultHandler
#pragma weak RADIO_IRQHandler                               = SystemDefaultHandler
#pragma weak nrfx_uart_0_irq_handler                        = SystemDefaultHandler
#pragma weak SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQHandler   = SystemDefaultHandler
#pragma weak SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQHandler   = SystemDefaultHandler
#pragma weak NFCT_IRQHandler                                = SystemDefaultHandler
#pragma weak GPIOTE_IRQHandler                              = SystemDefaultHandler
#pragma weak SAADC_IRQHandler                               = SystemDefaultHandler
#pragma weak TIMER0_IRQHandler                              = SystemDefaultHandler
#pragma weak TIMER1_IRQHandler                              = SystemDefaultHandler
#pragma weak TIMER2_IRQHandler                              = SystemDefaultHandler
#pragma weak RTC0_IRQHandler                                = SystemDefaultHandler
#pragma weak TEMP_IRQHandler                                = SystemDefaultHandler
#pragma weak RNG_IRQHandler                                 = SystemDefaultHandler
#pragma weak ECB_IRQHandler                                 = SystemDefaultHandler
#pragma weak CCM_AAR_IRQHandler                             = SystemDefaultHandler
#pragma weak WDT_IRQHandler                                 = SystemDefaultHandler
#pragma weak RTC1_IRQHandler                                = SystemDefaultHandler
#pragma weak QDEC_IRQHandler                                = SystemDefaultHandler
#pragma weak COMP_LPCOMP_IRQHandler                         = SystemDefaultHandler
#pragma weak SWI0_EGU0_IRQHandler                           = SystemDefaultHandler
#pragma weak SWI1_EGU1_IRQHandler                           = SystemDefaultHandler
#pragma weak SWI2_EGU2_IRQHandler                           = SystemDefaultHandler
#pragma weak SWI3_EGU3_IRQHandler                           = SystemDefaultHandler
#pragma weak SWI4_EGU4_IRQHandler                           = SystemDefaultHandler
#pragma weak SWI5_EGU5_IRQHandler                           = SystemDefaultHandler
#pragma weak TIMER3_IRQHandler                              = SystemDefaultHandler
#pragma weak TIMER4_IRQHandler                              = SystemDefaultHandler
#pragma weak PWM0_IRQHandler                                = SystemDefaultHandler
#pragma weak PDM_IRQHandler                                 = SystemDefaultHandler
#pragma weak MWU_IRQHandler                                 = SystemDefaultHandler
#pragma weak PWM1_IRQHandler                                = SystemDefaultHandler
#pragma weak PWM2_IRQHandler                                = SystemDefaultHandler
#pragma weak SPIM2_SPIS2_SPI2_IRQHandler                    = SystemDefaultHandler
#pragma weak RTC2_IRQHandler                                = SystemDefaultHandler
#pragma weak I2S_IRQHandler                                 = SystemDefaultHandler
#pragma weak FPU_IRQHandler                                 = SystemDefaultHandler
#pragma weak USBD_IRQHandler                                = SystemDefaultHandler
#pragma weak UARTE1_IRQHandler                              = SystemDefaultHandler
#pragma weak QSPI_IRQHandler                                = SystemDefaultHandler
#pragma weak CRYPTOCELL_IRQHandler                          = SystemDefaultHandler
#pragma weak PWM3_IRQHandler                                = SystemDefaultHandler
#pragma weak SPIM3_IRQHandler                               = SystemDefaultHandler

/**************************************************************************************************
  Global variables
**************************************************************************************************/

/* Defined by linker. */
extern unsigned long __text_end__;
extern unsigned long __data_start__;
extern unsigned long __data_end__;
extern unsigned long __bss_start__;
extern unsigned long __bss_end__;
extern unsigned long __stack_top__;
extern unsigned long __stack_limit__;
extern unsigned long __heap_end__;
extern unsigned long __heap_start__;

/*! Core vector table (must be placed at address VTOR, i.e. 0x00000000). */
SECTION(".isr_vector")
void (* const systemVectors[256])(void) =
{
  (void (*)(void))(&__stack_top__),             /*  0: The initial stack pointer */
  Reset_Handler,                                /*  1: The reset handler */
  NMI_Handler,                                  /*  2: The NMI handler */
  HardFault_Handler,                            /*  3: The hard fault handler */
  MemoryManagement_Handler,                     /*  4: The MPU fault handler */
  BusFault_Handler,                             /*  5: The bus fault handler */
  UsageFault_Handler,                           /*  6: The usage fault handler */
  0,                                            /*  7: Reserved */
  0,                                            /*  8: Reserved */
  0,                                            /*  9: Reserved */
  0,                                            /* 10: Reserved */
  SVC_Handler,                                  /* 11: SVCall handler */
  DebugMon_Handler,                             /* 12: Debug monitor handler */
  0,                                            /* 13: Reserved */
  PendSV_Handler,                               /* 14: The PendSV handler */
  SysTick_Handler,                              /* 15: The SysTick handler */

  /* External interrupts */
  POWER_CLOCK_IRQHandler,                       /* 16: POWER_CLOCK */
  RADIO_IRQHandler,                             /* 17: RADIO */
  UARTE0_UART0_IRQHandler,                      /* 18: UART0 */
  SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQHandler, /* 19: SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0 */
  SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQHandler, /* 20: SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1 */
  NFCT_IRQHandler,                              /* 21: NFCT */
  GPIOTE_IRQHandler,                            /* 22: GPIOTE */
  SAADC_IRQHandler,                             /* 23: SAADC */
  TIMER0_IRQHandler,                            /* 24: TIMER0 */
  TIMER1_IRQHandler,                            /* 25: TIMER1 */
  TIMER2_IRQHandler,                            /* 26: TIMER2 */
  RTC0_IRQHandler,                              /* 27: RTC0 */
  TEMP_IRQHandler,                              /* 28: TEMP */
  RNG_IRQHandler,                               /* 29: RNG */
  ECB_IRQHandler,                               /* 30: ECB */
  CCM_AAR_IRQHandler,                           /* 31: CCM_AAR */
  WDT_IRQHandler,                               /* 32: WDT */
  RTC1_IRQHandler,                              /* 33: RTC1 */
  QDEC_IRQHandler,                              /* 34: QDEC */
  COMP_LPCOMP_IRQHandler,                       /* 35: COMP_LPCOMP */
  SWI0_EGU0_IRQHandler,                         /* 36: SWI0_EGU0 */
  SWI1_EGU1_IRQHandler,                         /* 37: SWI1_EGU1 */
  SWI2_EGU2_IRQHandler,                         /* 38: SWI2_EGU2 */
  SWI3_EGU3_IRQHandler,                         /* 39: SWI3_EGU3 */
  SWI4_EGU4_IRQHandler,                         /* 40: SWI4_EGU4 */
  SWI5_EGU5_IRQHandler,                         /* 41: SWI5_EGU5 */
  TIMER3_IRQHandler,                            /* 42: TIMER3 */
  TIMER4_IRQHandler,                            /* 43: TIMER4 */
  PWM0_IRQHandler,                              /* 44: PWM0 */
  PDM_IRQHandler,                               /* 45: PDM */
  0,                                            /* 46: Reserved */
  0,                                            /* 47: Reserved */
  MWU_IRQHandler,                               /* 48: MWU */
  PWM1_IRQHandler,                              /* 49: PWM1 */
  PWM2_IRQHandler,                              /* 50: PWM2 */
  SPIM2_SPIS2_SPI2_IRQHandler,                  /* 51: SPIM2_SPIS2_SPI2 */
  RTC2_IRQHandler,                              /* 52: RTC2 */
  I2S_IRQHandler,                               /* 53: I2S */
  FPU_IRQHandler,                               /* 54: FPU */
  USBD_IRQHandler,                              /* 55: USBD */
  UARTE1_IRQHandler,                            /* 56: UARTE1 */
  QSPI_IRQHandler,                              /* 57: QSPI */
  CRYPTOCELL_IRQHandler,                        /* 58: CRYPTOCELL */
  0               ,                             /* 59: Reserved */
  0,                                            /* 60: Reserved */
  PWM3_IRQHandler,                              /* 61: PWM3 */
  0,                                            /* 62: Reserved */
  SPIM3_IRQHandler                              /* 63: SPIM3 */
                                                /* 64..127: Reserved */
};

/*************************************************************************************************/
/*!
 *  \brief      Reset handler.
 *
 *  \param      None.
 */
/*************************************************************************************************/
void Reset_Handler(void)
{
  unsigned long *pSrc, *pDst;

  /* Setup VTOR with address of the vector table.
   * Needed when the vector table is not located at address 0x00000000 
   */
  SCB->VTOR = (uint32_t)systemVectors;

  /* Copy data segment initializers from flash to SRAM. */
  pSrc = &__text_end__;
  pDst = &__data_start__;
  while (pDst < &__data_end__)
  {
    *pDst++ = *pSrc++;
  }

  /* Fill BSS segment with zeros. */
  pDst = &__bss_start__;
  while (pDst < &__bss_end__)
  {
    *pDst++ = 0;
  }

  /* Fill stack segment. */
  pDst = &__stack_limit__;
  while (pDst < &__stack_top__)
  {
    *pDst++ = INIT_STACK_VAL;
  }

  extern uint8_t *SystemHeapStart;
  extern uint32_t SystemHeapSize;

  /* Initialize heap variables. */
  SystemHeapStart = (uint8_t *)&__heap_start__;
  SystemHeapSize  = (uint8_t *)&__heap_end__ - (uint8_t *)&__heap_start__;

  /* Core initialization. */
  SystemInit();
  PalSysInit();

  /* Application entry. */
  main();

  /* Invoke semihosting exit on main() return. */
  __asm volatile ("mov    r0,#0x18");
  __asm volatile ("bkpt   0xAB");
}

/*************************************************************************************************/
/*!
 *  \brief      Default vector handler.
 *
 *  \param      None.
 */
/*************************************************************************************************/
static void SystemDefaultHandler(void)
{
  volatile unsigned int forever = 1;
  PalLedOn(PAL_LED_ID_ERROR);
  while (forever);
}
