/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      Stubbed Timer driver
 */
/*************************************************************************************************/

#include "pal_timer.h"

#include <stdio.h>

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

static struct
{
  PalTimerState_t    state;
  uint32_t           compareVal;
  PalTimerCompCback_t expCback;
} palTimerCb = {
  .state = PAL_TIMER_STATE_UNINIT,
  .compareVal = 0,
  .expCback = NULL
};

/**************************************************************************************************
  Stubbed Functions
**************************************************************************************************/

void PalTimerInit(PalTimerCompCback_t expCback)
{
  palTimerCb.state = PAL_TIMER_STATE_READY;
  palTimerCb.expCback = expCback;
}

void PalTimerDeInit(void)
{
  palTimerCb.state = PAL_TIMER_STATE_UNINIT;
  palTimerCb.expCback = NULL;
}

PalTimerState_t PalTimerGetState(void)
{
  return palTimerCb.state;
}

void PalTimerStart(uint32_t expTimeUsec)
{
  (void)expTimeUsec;
  palTimerCb.state = PAL_TIMER_STATE_BUSY;

  // Simulate immediate callback for testing
  if (palTimerCb.expCback != NULL)
  {
    palTimerCb.expCback();
  }

  palTimerCb.state = PAL_TIMER_STATE_READY;
}

void PalTimerStop(void)
{
  palTimerCb.state = PAL_TIMER_STATE_READY;
}

uint32_t PalTimerGetCurrentTime(void)
{
  // Stub return a dummy increasing value or fixed mock value
  static uint32_t dummyTime = 0;
  return dummyTime += 1000;  // e.g., simulate time progression
}
