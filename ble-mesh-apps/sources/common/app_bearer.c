/*************************************************************************************************/
/*!
 *  \file   app_bearer.c
 *
 *  \brief Application Bearer module
 *
 *  Copyright (c) 2010-2019 Arm Ltd. All Rights Reserved.
 *
 *  $$LICENSE$$
 */
/*************************************************************************************************/

#include <string.h>

#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_trace.h"
#include "wsf_timer.h"

#include "mesh_types.h"

#include "app_bearer.h"
#include "app_mesh_api.h"

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Bearer control block */
static appBrCb_t appBrCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Schedules the bearer on the specified slot immediately.
 *
 *  \param[in] slot  Bearer slot.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void brSchedule(uint8_t slot)
{
  WSF_ASSERT(appBrCb.schTable[slot].pStartFunc != NULL);

  if (appBrCb.schTable[slot].enabled)
  {
    appBrCb.runningSlot = slot;
    appBrCb.schTable[slot].pStartFunc();
    WsfTimerStartMs(&appBrCb.schedulerTimer,  appBrCb.schTable[slot].minSlotTimeInMs);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Runs the bearer scheduler.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void brRunScheduler(void)
{
  uint8_t nextSlot;

  if (appBrCb.runningSlot == BR_NUM_SLOTS)
  {
    return;
  }

  /* Move to the next slot. */
  nextSlot = (appBrCb.runningSlot + 1) % BR_NUM_SLOTS;

  /* Find next valid slot. */
  while (!appBrCb.schTable[nextSlot].enabled && (nextSlot != appBrCb.runningSlot))
  {
    /* Move to the next slot. */
    nextSlot = (nextSlot + 1) % BR_NUM_SLOTS;
  }

  WSF_ASSERT(appBrCb.schTable[appBrCb.runningSlot].pStopFunc != NULL);
  WSF_ASSERT(appBrCb.schTable[nextSlot].pStartFunc != NULL);

  /* Stop running slot if another needs to be scheduled or the slot was GATT server
   * For GATT server we need to change ADV data. */
  if ((nextSlot != appBrCb.runningSlot) ||
      ((appBrCb.runningSlot == BR_GATT_SLOT) && appBrCb.schTable[appBrCb.runningSlot].enabled))
  {
    /* Stop current slot. */
    if (appBrCb.schTable[appBrCb.runningSlot].pStopFunc())
    {
      /* Stop scheduler until bearer is stopped. */
      appBrCb.runningSlot = BR_NUM_SLOTS;

      /* Mark next pending next slot. */
      appBrCb.pendingSlot = nextSlot;

      /* Wait on bearer. */
      return;
    }
    else
    {
      /* No need to wait on bearer. Start next slot. */
      appBrCb.runningSlot = nextSlot;
      appBrCb.schTable[appBrCb.runningSlot].pStartFunc();
    }
  }
  else
  {
    /* No other slots found. If this slot is still available, continue. If the
     * current slot has been disabled, then stop scheduler.
     */
    if ((!appBrCb.schTable[nextSlot].enabled))
    {
      appBrCb.runningSlot = BR_NUM_SLOTS;
      appBrCb.schTable[nextSlot].pStopFunc();
      return;
    }
  }

  /* Start timer. */
  WsfTimerStartMs(&appBrCb.schedulerTimer,  appBrCb.schTable[appBrCb.runningSlot].minSlotTimeInMs);
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the bearer scheduler.
 *
 *  \param[in] handlerID  WSF handler ID for App.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void appBearerEmptyCback(uint8_t slot)
{
  (void)slot;
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the bearer scheduler.
 *
 *  \param[in] handlerID  WSF handler ID for App.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void AppBearerInit(wsfHandlerId_t handlerId)
{
  memset(appBrCb.schTable, 0, sizeof(brSchedulerEntry_t) * BR_NUM_SLOTS);
  appBrCb.runningSlot = BR_NUM_SLOTS;
  appBrCb.pendingSlot = BR_NUM_SLOTS;
  appBrCb.pAppCback = appBearerEmptyCback;

  /* Set timer parameters. */
  appBrCb.schedulerTimer.isStarted = FALSE;
  appBrCb.schedulerTimer.handlerId = handlerId;
  appBrCb.schedulerTimer.msg.event = APP_BR_TIMEOUT_EVT;
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback function to service the bearer scheduler events.
 *
 *  \param[in] cback  Application callback function.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void AppBearerRegister(appBearerCback_t cback)
{
  appBrCb.pAppCback = cback;
}

/*************************************************************************************************/
/*!
 *  \brief     Schedules the bearer slot.
 *
 *  \param[in] bearerSlot       Bearer Slot. See ::bearerSlots.
 *  \param[in] pStart           Bearer Start function.
 *  \param[in] pStop            Bearer Stop function.
 *  \param[in] dmCback          Bearer DM event handler.
 *  \param[in] minSlotTimeInMs  Minimum time in ms scheduled for the ADV bearer.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void AppBearerScheduleSlot(uint8_t bearerSlot, brStartFunc_t pStart, brStopFunc_t pStop,
                           brDmEvtCback_t dmCback, uint32_t minSlotTimeInMs)
{
  if (appBrCb.runningSlot == bearerSlot)
  {
    /* Slot is running. Cannot schedule. */
    return;
  }

  appBrCb.schTable[bearerSlot].pStartFunc = pStart;
  appBrCb.schTable[bearerSlot].pStopFunc = pStop;
  appBrCb.schTable[bearerSlot].dmCback = dmCback;
  appBrCb.schTable[bearerSlot].minSlotTimeInMs = minSlotTimeInMs;
  appBrCb.schTable[bearerSlot].enabled = FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Enables the scheduled bearer slot.
 *
 *  \param[in] bearerSlot  Bearer Slot. See ::bearerSlots.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void AppBearerEnableSlot(uint8_t bearerSlot)
{
  if (appBrCb.schTable[bearerSlot].enabled)
  {
    /* Slot is already enabled. */
    return;
  }

  /* Enable Slot */
  appBrCb.schTable[bearerSlot].enabled = TRUE;

  if (appBrCb.runningSlot == BR_NUM_SLOTS)
  {
    /* No slots scheduled. Run this slot. */
    brSchedule(bearerSlot);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Disables the scheduled bearer slot.
 *
 *  \param[in] bearerSlot  Bearer Slot. See ::bearerSlots.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void AppBearerDisableSlot(uint8_t bearerSlot)
{
  if (!appBrCb.schTable[bearerSlot].enabled)
  {
    /* Slot is already disabled. */
    return;
  }

  /* Disable slot. */
  appBrCb.schTable[bearerSlot].enabled = FALSE;

  /* If the running bearer is cancelled, stop it immediately, stop the timer and run the
   * scheduler again.
   */
  if (appBrCb.runningSlot == bearerSlot)
  {
    WsfTimerStop(&appBrCb.schedulerTimer);
    brRunScheduler();
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Scheduler Bearer timeout.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppBearerSchedulerTimeout(void)
{
  /* Send timeout event to application. */
  appBrCb.pAppCback(appBrCb.runningSlot);

  /* On timeout, re-run scheduler. */
  brRunScheduler();
}

/*************************************************************************************************/
/*!
 *  \brief     Process DM messages for a Mesh node. This function should be called
 *             from the application's event handler.
 *
 *  \param[in] pMsg  Pointer to DM callback event message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void AppBearerProcDmMsg(dmEvt_t *pMsg)
{
  /* Update advertising or scanning state. */
  switch (pMsg->hdr.event)
  {
    case DM_ADV_START_IND:
    case DM_ADV_SET_START_IND:
      if (pMsg->hdr.status == HCI_SUCCESS)
      {
        appBrCb.advState = ADV_STARTED;
      }
      else
      {
        appBrCb.advState = ADV_STOPPED;
      }
      break;

    case DM_ADV_STOP_IND:
    case DM_ADV_SET_STOP_IND:
      WSF_ASSERT((pMsg->hdr.status == HCI_SUCCESS) ||
                 (pMsg->hdr.status == HCI_ERR_LIMIT_REACHED) ||
                 (pMsg->hdr.status == HCI_ERR_ADV_TIMEOUT));

      appBrCb.advState = ADV_STOPPED;

      /* Check if a slot is pending the end of the previous one.*/
      if(appBrCb.pendingSlot != BR_NUM_SLOTS)
      {
        brSchedule(appBrCb.pendingSlot);
        appBrCb.pendingSlot = BR_NUM_SLOTS;
      }
      break;

    case DM_SCAN_START_IND:
    case DM_EXT_SCAN_START_IND:
      WSF_ASSERT(pMsg->hdr.status == HCI_SUCCESS);
      appBrCb.scanState = SCAN_STARTED;
      break;

    case DM_SCAN_STOP_IND:
    case DM_EXT_SCAN_STOP_IND:
      WSF_ASSERT(pMsg->hdr.status == HCI_SUCCESS);
      appBrCb.scanState = SCAN_STOPPED;

      /* Check if a slot is pending the end of the previous one.*/
      if(appBrCb.pendingSlot != BR_NUM_SLOTS)
      {
        brSchedule(appBrCb.pendingSlot);
        appBrCb.pendingSlot = BR_NUM_SLOTS;
      }
      break;

    case DM_CONN_OPEN_IND:
      if (pMsg->hdr.status == HCI_SUCCESS)
      {
        /* Advertising stops when connection is established.*/
        appBrCb.advState = ADV_STOPPED;
      }
      break;

    case DM_CONN_CLOSE_IND:
      WSF_ASSERT(pMsg->hdr.status == HCI_SUCCESS);

      if (appBrCb.runningSlot != BR_GATT_SLOT)
      {
        /* Signal GATT that the connection is terminated. */
        appBrCb.schTable[BR_GATT_SLOT].dmCback(pMsg);
      }
      break;

    case DM_RESET_CMPL_IND:
        appBrCb.advState = ADV_STOPPED;
        appBrCb.scanState = SCAN_STOPPED;
      break;

    case DM_SCAN_REPORT_IND:
      break;

    default:
      break;
  }

  /* Call the bearer's DM event handler. */
  if (appBrCb.runningSlot < BR_NUM_SLOTS)
  {
    appBrCb.schTable[appBrCb.runningSlot].dmCback(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the scanning state.
 *
 *  \return Scanning state value. See ::bearerScanStateValues.
 */
/*************************************************************************************************/
uint8_t AppBearerGetScanState(void)
{
  return appBrCb.scanState;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the scanning state.
 *
 *  \param[in] scanState  Scanning state value. See ::bearerScanStateValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void AppBearerSetScanState(uint8_t scanState)
{
  appBrCb.scanState = scanState;
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the advertising state.
 *
 *  \return Advertising state value. See ::bearerAdvStateValues.
 */
/*************************************************************************************************/
uint8_t AppBearerGetAdvState(void)
{
  return appBrCb.advState;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the advertising state.
 *
 *  \param[in] advState  Advertising state value. See ::bearerAdvStateValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void AppBearerSetAdvState(uint8_t advState)
{
  appBrCb.advState = advState;
}
