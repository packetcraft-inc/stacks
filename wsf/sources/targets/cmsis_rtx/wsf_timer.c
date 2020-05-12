/*************************************************************************************************/
/*!
 *  \file   wsf_timer.c
 *
 *  \brief  Timer service.
 *
 *  Copyright (c) 2009-2019 Arm Ltd. All Rights Reserved.
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

#include <string.h>
#include "wsf_queue.h"
#include "wsf_timer.h"

#include "wsf_assert.h"
#include "wsf_cs.h"
#include "wsf_trace.h"
#include "wsf_types.h"
#include "pal_rtc.h"
#include "pal_led.h"
#include "pal_sys.h"
#include "cmsis_os2.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

#if (WSF_MS_PER_TICK == 10)
/* convert seconds to timer ticks */
#define WSF_TIMER_SEC_TO_TICKS(sec)         (100 * (sec) + 1)

/* convert milliseconds to timer ticks */
/* Extra tick should be added to guarantee waiting time is longer than the specified ms. */
#define WSF_TIMER_MS_TO_TICKS(ms)           (((uint32_t)(((uint64_t)(ms) * (uint64_t)(419431)) >> 22)) + 1)

/*! \brief  WSF timer ticks per second. */
#define WSF_TIMER_TICKS_PER_SEC       (1000 / WSF_MS_PER_TICK)

#elif (WSF_MS_PER_TICK == 1)
/* convert seconds to timer ticks */
#define WSF_TIMER_SEC_TO_TICKS(sec)         (1000 * (sec) + 1)

/*! \brief Convert milliseconds to timer ticks. */
/*! \brief Extra tick should be added to guarantee waiting time is longer than the specified ms. */
#define WSF_TIMER_MS_TO_TICKS(ms)           ((uint64_t)(ms) + 1)

#define WSF_TIMER_TICKS_PER_SEC             (1000)

#else
#error "WSF_TIMER_MS_TO_TICKS() and WSF_TIMER_SEC_TO_TICKS not defined for WSF_MS_PER_TICK"
#endif

/*! \brief  Number of RTC ticks per WSF timer tick. */
#define WSF_TIMER_RTC_TICKS_PER_WSF_TICK    ((PAL_RTC_TICKS_PER_SEC + WSF_TIMER_TICKS_PER_SEC - 1) / (WSF_TIMER_TICKS_PER_SEC))

/*! \brief  Calculate number of elapsed WSF timer ticks. */
#define WSF_RTC_TICKS_TO_WSF(x)             ((x) / WSF_TIMER_RTC_TICKS_PER_WSF_TICK)

/*! \brief  Mask of seconds part in RTC ticks. */
#define WSF_TIMER_RTC_TICKS_SEC_MASK        (0x00FF8000)

/*! \brief  Addition of RTC ticks. */
#define WSF_TIMER_RTC_ADD_TICKS(x, y)       (((x) + (y)) & PAL_MAX_RTC_COUNTER_VAL)

/*! \brief  Subtraction of RTC ticks. */
#define WSF_TIMER_RTC_SUB_TICKS(x, y)       ((PAL_MAX_RTC_COUNTER_VAL + 1 + (x) - (y)) & PAL_MAX_RTC_COUNTER_VAL)

/*! \brief  Minimum RTC ticks required to go into sleep. */
#define WSF_TIMER_MIN_RTC_TICKS_FOR_SLEEP   (2)

/*!
 * \brief  Computing the difference between two RTC counter values.
 *
 * Calculate elapsed ticks since last WSF timer update, with remainder;
 * since the RTC timer is 24 bit set the 24th bit to handle any underflow.
 */
#define WSF_TIMER_RTC_ELAPSED_TICKS(x)      ((PAL_MAX_RTC_COUNTER_VAL + 1 + (x) - wsfTimerRtcLastTicks \
                                  + wsfTimerRtcRemainder) & PAL_MAX_RTC_COUNTER_VAL)

/*! \brief  Max number of timers. */
#define WSF_TIMER_MAX_NUMBER                16

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

wsfQueue_t  wsfTimerTimerQueue;     /*!< Timer queue */

/*! \brief  Last RTC value read. */
static uint32_t wsfTimerRtcLastTicks = 0;

/*! \brief  Remainder value. */
static uint32_t wsfTimerRtcRemainder = 0;

/*! \brief      Control block. */
static struct
{
  osTimerId_t        osTimerId;         /*!< Os timer ID. */
  bool_t             expStatus;         /*!< Timer expire status. */
  wsfTimer_t         *pWsfTimer;        /*!< WSF timer. */
} wsfTimerCb[WSF_TIMER_MAX_NUMBER];

/*************************************************************************************************/
/*!
 *  \brief  Timer callback.
 *
 *  \param  pTimer  Pointer to timer.
 */
/*************************************************************************************************/
static void wsfTimerCallback(void const *pTimer)
{
  wsfTimer_t *pWsfTimer = (wsfTimer_t *)pTimer;

  uint8_t timerIdx;

  for (timerIdx = 0; timerIdx < WSF_TIMER_MAX_NUMBER; timerIdx++)
  {
    if (wsfTimerCb[timerIdx].pWsfTimer == pWsfTimer)
    {
      break;
    }
  }

  if (timerIdx == WSF_TIMER_MAX_NUMBER)
  {
    return;
  }

  /* task schedule lock */
  WsfTaskLock();

  wsfTimerCb[timerIdx].expStatus = TRUE;
  pWsfTimer->ticks = 0;

  /* timer expired; set task for this timer as ready */
  WsfTaskSetReady(pWsfTimer->handlerId, WSF_TIMER_EVENT);

  /* task schedule unlock */
  WsfTaskUnlock();
}

/*************************************************************************************************/
/*!
 *  \brief  Start timer.
 *
 *  \param  pTimer  Pointer to timer.
 *  \param  ticks   Timer ticks until expiration.
 */
/*************************************************************************************************/
static void wsfTimerStart(wsfTimer_t *pTimer, wsfTimerTicks_t ticks)
{
  uint8_t timerIdx;

  for (timerIdx = 0; timerIdx < WSF_TIMER_MAX_NUMBER; timerIdx++)
  {
    if ((wsfTimerCb[timerIdx].pWsfTimer == pTimer) || (wsfTimerCb[timerIdx].pWsfTimer == NULL))
    {
      break;
    }
  }

  if (timerIdx == WSF_TIMER_MAX_NUMBER)
  {
    return;
  }

  if (wsfTimerCb[timerIdx].pWsfTimer == NULL)
  {
    osTimerId_t timerId;
    timerId = osTimerNew((osTimerFunc_t)&wsfTimerCallback, osTimerOnce, pTimer, NULL);
    wsfTimerCb[timerIdx].osTimerId = timerId;
    wsfTimerCb[timerIdx].pWsfTimer = pTimer;
  }

  if (wsfTimerCb[timerIdx].osTimerId == NULL)
  {
    return;
  }

  wsfTimerCb[timerIdx].pWsfTimer->isStarted = TRUE;
  wsfTimerCb[timerIdx].pWsfTimer->ticks = ticks;
  osTimerStart (wsfTimerCb[timerIdx].osTimerId, ticks);
}

/*************************************************************************************************/
/*!
 *  \brief  Convert WSF ticks into RTC ticks.
 *
 *  \param  wsfTicks   Timer ticks.
 *
 *  \return Number of RTC ticks
 */
/*************************************************************************************************/
static uint32_t wsfTimerTicksToRtc(wsfTimerTicks_t wsfTicks)
{
  uint32_t numSec = wsfTicks / WSF_TIMER_TICKS_PER_SEC;
  uint32_t remainder = wsfTicks - numSec * WSF_TIMER_TICKS_PER_SEC;

  return ((numSec * PAL_RTC_TICKS_PER_SEC) + (remainder * WSF_TIMER_RTC_TICKS_PER_WSF_TICK));
}

/*************************************************************************************************/
/*!
 *  \brief  Return the number of ticks until the next timer expiration.  Note that this
 *          function can return zero even if a timer is running, indicating a timer
 *          has expired but has not yet been serviced.
 *
 *  \return The number of ticks until the next timer expiration.
 */
/*************************************************************************************************/
static wsfTimerTicks_t wsfTimerNextExpiration(void)
{
  return osTimerGetTicks();
}

/*************************************************************************************************/
/*!
 *  \brief  Initialize the timer service.  This function should only be called once
 *          upon system initialization.
 */
/*************************************************************************************************/
void WsfTimerInit(void)
{
  wsfTimerRtcLastTicks = PalRtcCounterGet();
  wsfTimerRtcRemainder = 0;
  memset(wsfTimerCb, 0, sizeof(wsfTimerCb));
}

/*************************************************************************************************/
/*!
 *  \brief  Start a timer in units of seconds.
 *
 *  \param  pTimer  Pointer to timer.
 *  \param  sec     Seconds until expiration.
 */
/*************************************************************************************************/
void WsfTimerStartSec(wsfTimer_t *pTimer, wsfTimerTicks_t sec)
{
  WSF_TRACE_INFO2("WsfTimerStartSec pTimer:0x%x ticks:%u", (uint32_t)pTimer, WSF_TIMER_SEC_TO_TICKS(sec));

  /* insert timer into queue */
  wsfTimerStart(pTimer, WSF_TIMER_SEC_TO_TICKS(sec));
}

/*************************************************************************************************/
/*!
 *  \brief  Start a timer in units of milliseconds.
 *
 *  \param  pTimer  Pointer to timer.
 *  \param  ms      Milliseconds until expiration.
 */
/*************************************************************************************************/
void WsfTimerStartMs(wsfTimer_t *pTimer, wsfTimerTicks_t ms)
{
  WSF_TRACE_INFO2("WsfTimerStartMs pTimer:0x%x ticks:%u", (uint32_t)pTimer, WSF_TIMER_MS_TO_TICKS(ms));

  /* insert timer into queue */
  wsfTimerStart(pTimer, WSF_TIMER_MS_TO_TICKS(ms));
}

/*************************************************************************************************/
/*!
 *  \brief  Stop a timer.
 *
 *  \param  pTimer  Pointer to timer.
 */
/*************************************************************************************************/
void WsfTimerStop(wsfTimer_t *pTimer)
{
  uint8_t timerIdx;

  for (timerIdx = 0; timerIdx < WSF_TIMER_MAX_NUMBER; timerIdx++)
  {
    if (wsfTimerCb[timerIdx].pWsfTimer == pTimer)
    {
      break;
    }
  }

  if ((timerIdx == WSF_TIMER_MAX_NUMBER) || (wsfTimerCb[timerIdx].osTimerId == NULL))
  {
    return;
  }

  pTimer->isStarted = FALSE;
  pTimer->ticks = 0;
  osTimerStop(wsfTimerCb[timerIdx].osTimerId);
}

/*************************************************************************************************/
/*!
 *  \brief  Update the timer service with the number of elapsed ticks.
 *
 *  \param  ticks  Number of ticks since last update.
 */
/*************************************************************************************************/
void WsfTimerUpdate(wsfTimerTicks_t ticks)
{
  (void)ticks;
  /* this is not needed in RTOS. RTOS will update timer */
}

/*************************************************************************************************/
/*!
 *  \brief  Service expired timers for the given task.
 *
 *  \param  taskId      Task ID.
 *
 *  \return Pointer to timer or NULL.
 */
/*************************************************************************************************/
wsfTimer_t *WsfTimerServiceExpired(wsfTaskId_t taskId)
{
  wsfTimer_t  *pElem = NULL;

  /* Unused parameters */
  (void)taskId;

  WsfTaskLock();

  for (unsigned int i = 0; i < WSF_TIMER_MAX_NUMBER; i++)
  {
    if (wsfTimerCb[i].expStatus)
    {
      pElem = wsfTimerCb[i].pWsfTimer;
      wsfTimerCb[i].expStatus = FALSE;
      pElem->isStarted = FALSE;
      break;
    }
  }

  WsfTaskUnlock();

  return pElem;
}

/*************************************************************************************************/
/*!
 *  \brief  Function for checking if there is an active timer and if there is enough time to
 *          go to sleep and going to sleep.
 */
/*************************************************************************************************/
void WsfTimerSleep(void)
{
  wsfTimerTicks_t nextExpiration;

  /* If PAL system is busy, no need to sleep. */
  if (PalSysIsBusy())
  {
    return;
  }

  nextExpiration = wsfTimerNextExpiration();

  if (nextExpiration > 0)
  {
    uint32_t awake = wsfTimerTicksToRtc(nextExpiration);
    uint32_t rtcCurrentTicks = PalRtcCounterGet();
    uint32_t elapsed = WSF_TIMER_RTC_ELAPSED_TICKS(rtcCurrentTicks);

    /* if we have time to sleep before timer expiration */
    if ((awake - elapsed) > WSF_TIMER_MIN_RTC_TICKS_FOR_SLEEP)
    {
      uint32_t compareVal = (rtcCurrentTicks + awake - elapsed) & PAL_MAX_RTC_COUNTER_VAL;

      /* set RTC timer compare */
      PalRtcCompareSet(0, compareVal);

      /* enable RTC interrupt */
      PalRtcEnableCompareIrq(0);

      /* one final check for OS activity then enter sleep */
      PalEnterCs();
      if (wsfOsReadyToSleep() && (PalRtcCounterGet() != PalRtcCompareGet(0)))
      {
        PalLedOff(PAL_LED_ID_CPU_ACTIVE);
        PalSysSleep();
        PalLedOn(PAL_LED_ID_CPU_ACTIVE);
      }
      PalExitCs();
    }
    else
    {
      /* Not enough time to go to sleep. Let the system run till the pending timer expires. */
      LL_TRACE_WARN0("WsfTimerSleep, not enough time to sleep");
    }
  }
  else
  {
    /* disable RTC interrupt */
    PalRtcDisableCompareIrq(0);

    /* one final check for OS activity then enter sleep */
    PalEnterCs();
    if (wsfOsReadyToSleep())
    {
      PalLedOff(PAL_LED_ID_CPU_ACTIVE);
      PalSysSleep();
      PalLedOn(PAL_LED_ID_CPU_ACTIVE);
    }
    PalExitCs();
  }
}


/*************************************************************************************************/
/*!
 *  \brief  Function for updating WSF timer based on elapsed RTC ticks.
 */
/*************************************************************************************************/
void WsfTimerSleepUpdate(void)
{
}
