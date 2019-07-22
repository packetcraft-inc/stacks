/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  System hooks.
 *
 *  Copyright (c) 2013-2019 Arm Ltd.
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

#include "nrf.h"
#include "pal_bb.h"
#include "pal_rtc.h"
#include "pal_sys.h"
#include "pal_led.h"

#include <string.h>

/**************************************************************************************************
  Macros
**************************************************************************************************/

#ifdef __GNUC__

/*! \brief      Stack initial values. */
#define INIT_STACK_VAL    0xAFAFAFAF

/*! \brief      Starting memory location of free memory. */
#define FREE_MEM_START          ((uint8_t *)&__heap_start__)

/*! \brief      Total size in bytes of free memory. */
#define FREE_MEM_SIZE           ((uint32_t)&__heap_end__ - (uint32_t)&__heap_start__)

extern uint8_t *SystemHeapStart;
extern uint32_t SystemHeapSize;

extern unsigned long __text_end__;
extern unsigned long __data_start__;
extern unsigned long __data_end__;
extern unsigned long __bss_start__;
extern unsigned long __bss_end__;
extern unsigned long __stack_top__;
extern unsigned long __stack_limit__;
extern unsigned long __heap_end__;
extern unsigned long __heap_start__;

#else

/*! \brief      Starting memory location of free memory. */
#define FREE_MEM_START          ((uint8_t *)palSysFreeMem)

/*! \brief      Total size in bytes of free memory. */
#define FREE_MEM_SIZE           (1024 * 196)

#endif

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

#ifdef __GNUC__

uint8_t *SystemHeapStart;
uint32_t SystemHeapSize;

#else

/*! \brief      Free memory for pool buffers (align to word boundary). */
uint32_t palSysFreeMem[FREE_MEM_SIZE/sizeof(uint32_t)];

uint8_t *SystemHeapStart = (uint8_t *) palSysFreeMem;
uint32_t SystemHeapSize = FREE_MEM_SIZE;

#endif

/*! \brief      Number of assertions. */
static uint32_t palSysAssertCount;

/*! \brief      Trap enabled flag. */
static volatile bool_t PalSysAssertTrapEnable;

static uint32_t palSysBusyCount;

/**************************************************************************************************
  Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Enter a critical section.
 *
 *  \return None.
 */
/*************************************************************************************************/
static inline void palEnterCs(void)
{
  #ifdef __IAR_SYSTEMS_ICC__
      __disable_interrupt();
  #endif
  #ifdef __GNUC__
      __asm volatile ("cpsid i");
  #endif
  #ifdef __CC_ARM
      __disable_irq();
  #endif
}

/*************************************************************************************************/
/*!
 *  \brief  Exit a critical section.
 *
 *  \return None.
 */
/*************************************************************************************************/
static inline void palExitCs(void)
{
  #ifdef __IAR_SYSTEMS_ICC__
      __enable_interrupt();
  #endif
  #ifdef __GNUC__
      __asm volatile ("cpsie i");
  #endif
  #ifdef __CC_ARM
      __enable_irq();
  #endif
}

/*************************************************************************************************/
/*!
 *  \brief      Common platform initialization.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void PalSysInit(void)
{
  /* enable Flash cache */
  NRF_NVMC->ICACHECNF |= (NVMC_ICACHECNF_CACHEEN_Enabled << NVMC_ICACHECNF_CACHEEN_Pos);

  /* switch to more accurate 16 MHz crystal oscillator (system starts up using 16MHz RC oscillator) */
  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART    = 1;
  while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0)
  {
  }

  /* configure low-frequency clock */
  NRF_CLOCK->LFCLKSRC             = (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos);
  NRF_CLOCK->EVENTS_LFCLKSTARTED  = 0;
  NRF_CLOCK->TASKS_LFCLKSTART     = 1;
  while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0)
  {
  }
  NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;

  /* cycle radio peripheral power to guarantee known radio state */
  NRF_RADIO->POWER = 0;
  NRF_RADIO->POWER = 1;

  palSysAssertCount = 0;
  PalSysAssertTrapEnable = TRUE;
  palSysBusyCount = 0;

  PalRtcInit();
  PalLedInit();
  PalLedOff(PAL_LED_ID_ERROR);
  PalLedOn(PAL_LED_ID_CPU_ACTIVE);

#ifdef DEBUG
  /* Reset free memory. */
  memset(SystemHeapStart, 0, SystemHeapSize);
#endif
}

/*************************************************************************************************/
/*!
 *  \brief      System fault trap.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void PalSysAssertTrap(void)
{
  PalLedOn(PAL_LED_ID_ERROR);

  palSysAssertCount++;

  while (PalSysAssertTrapEnable);
}

/*************************************************************************************************/
/*!
 *  \brief      Set system trap.
 *
 *  \param      enable    Enable assert trap or not.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void PalSysSetTrap(bool_t enable)
{
  PalSysAssertTrapEnable = enable;
}

/*************************************************************************************************/
/*!
 *  \brief      Get assert count.
 *
 *  \return     None.
 */
/*************************************************************************************************/
uint32_t PalSysGetAssertCount(void)
{
  return palSysAssertCount;
}

/*************************************************************************************************/
/*!
 *  \brief      Count stack usage.
 *
 *  \return     Number of bytes used by the stack.
 */
/*************************************************************************************************/
uint32_t PalSysGetStackUsage(void)
{
#ifdef __GNUC__
  unsigned long *pUnused = &__stack_limit__;

  while (pUnused < &__stack_top__)
  {
    if (*pUnused != INIT_STACK_VAL)
    {
      break;
    }

    pUnused++;
  }

  return (uint32_t)(&__stack_top__ - pUnused) * sizeof(*pUnused);
#else
  /* Not available; stub routine. */
  return 0;
#endif
}

/*************************************************************************************************/
/*!
 *  \brief      System sleep.
 *
 *  \return     none.
 */
/*************************************************************************************************/
void PalSysSleep(void)
{
  #ifdef __IAR_SYSTEMS_ICC__
          __wait_for_interrupt();
  #endif
  #ifdef __GNUC__
          __asm volatile ("wfi");
  #endif
  #ifdef __CC_ARM
          __wfi();
  #endif
}

/*************************************************************************************************/
/*!
 *  \brief      Check if system is busy.
 *
 *  \return     True if system is busy.
 */
/*************************************************************************************************/
bool_t PalSysIsBusy(void)
{
  bool_t sysIsBusy = FALSE;
  palEnterCs();
  sysIsBusy = ((palSysBusyCount == 0) ? FALSE : TRUE);
  palExitCs();
  return sysIsBusy;
}

/*************************************************************************************************/
/*!
 *  \brief      Set system busy.
 *
 *  \return     none.
 */
/*************************************************************************************************/
void PalSysSetBusy(void)
{
  palEnterCs();
  palSysBusyCount++;
  palExitCs();
}

/*************************************************************************************************/
/*!
 *  \brief      Set system idle.
 *
 *  \return     none.
 */
/*************************************************************************************************/
void PalSysSetIdle(void)
{
  palEnterCs();
  if (palSysBusyCount)
  {
    palSysBusyCount--;
  }
  palExitCs();
}
