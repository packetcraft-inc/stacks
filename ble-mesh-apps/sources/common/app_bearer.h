/*************************************************************************************************/
/*!
 *  \file   app_bearer.h
 *
 *  \brief  Application Bearer module interface.
 *
 *  Copyright (c) 2010-2019 Arm Ltd. All Rights Reserved.
 *
 *  $$LICENSE$$
 *
 */
/*************************************************************************************************/

#ifndef APP_BEARER_H
#define APP_BEARER_H

#include "wsf_os.h"
#include "wsf_timer.h"
#include "dm_api.h"
#include "att_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Bearer advertising states. */
enum bearerSlots
{
  BR_ADV_SLOT,    /*! ADV bearer slot index */
  BR_GATT_SLOT,   /*! GATT bearer slot index */
  BR_NUM_SLOTS    /*! Number of bearer scheduler slots */
};

/*! Bearer advertising states. */
enum bearerAdvStateValues
{
  ADV_STOPPED,    /*!< Advertising stopped */
  ADV_STARTED,    /*!< Advertising started */
  ADV_STOP_REQ,   /*!< Advertising stop request was sent */
  ADV_START_REQ   /*!< Advertising start request was sent */
};

/*! Bearer scanning states */
enum bearerScanStateValues
{
  SCAN_STOPPED,   /*!< Scanning stopped */
  SCAN_STARTED,   /*!< Scanning started */
  SCAN_STOP_REQ,  /*!< Scan stop request was sent */
  SCAN_START_REQ  /*!< Scan start request was sent */
};

/**************************************************************************************************
  Data Types
**************************************************************************************************/

typedef void (*brStartFunc_t) (void);
typedef bool_t (*brStopFunc_t) (void);
typedef void (*appBearerCback_t)(uint8_t slot);
typedef void (*brDmEvtCback_t)(dmEvt_t *pDmEvt);

/*! Scheduler entry */
typedef struct
{
  brStartFunc_t          pStartFunc;           /*!< Bearer Start function */
  brStopFunc_t           pStopFunc;            /*!< Bearer Stop function */
  brDmEvtCback_t         dmCback;              /*!< Bearer DM event handler */
  uint32_t               minSlotTimeInMs;      /*!< Minimum scheduled time in ms */
  bool_t                 enabled;              /*!< Specifies if the slot is enabled */
} brSchedulerEntry_t;

/*! App Bearer control block */
typedef struct
{
  appBearerCback_t    pAppCback;               /*!< Application callback */
  brSchedulerEntry_t  schTable[BR_NUM_SLOTS];  /*!< Bearer slots table */
  uint8_t             runningSlot;             /*!< Running slot index */
  uint8_t             pendingSlot;             /*!< Pending slot index */
  uint8_t             advState;                /*!< Advertising state.
                                                *   See ::bearerAdvStateValues
                                                */
  uint8_t             scanState;               /*!< Scanning state.
                                                *   See ::bearerScanStateValues
                                                */
  wsfTimer_t          schedulerTimer;          /*!< Scheduler WSF timer */
} appBrCb_t;

/**************************************************************************************************
  Function Declarations
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
void AppBearerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback function to service the bearer scheduler events.
 *
 *  \param[in] cback  Application callback function.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void AppBearerRegister(appBearerCback_t cback);

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
                           brDmEvtCback_t dmCback, uint32_t minSlotTimeInMs);

/*************************************************************************************************/
/*!
 *  \brief     Enables the scheduled bearer slot.
 *
 *  \param[in] bearerSlot  Bearer Slot. See ::bearerSlots.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void AppBearerEnableSlot(uint8_t bearerSlot);

/*************************************************************************************************/
/*!
 *  \brief     Disables the scheduled bearer slot.
 *
 *  \param[in] bearerSlot  Bearer Slot. See ::bearerSlots.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void AppBearerDisableSlot(uint8_t bearerSlot);

/*************************************************************************************************/
/*!
 *  \brief  Scheduler Bearer timeout.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppBearerSchedulerTimeout(void);

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
void AppBearerProcDmMsg(dmEvt_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief  Gets the scanning state.
 *
 *  \return Scanning state value. See ::bearerScanStateValues.
 */
/*************************************************************************************************/
uint8_t AppBearerGetScanState(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets the scanning state.
 *
 *  \param[in] scanState  Scanning state value. See ::bearerScanStateValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void AppBearerSetScanState(uint8_t scanState);

/*************************************************************************************************/
/*!
 *  \brief  Gets the advertising state.
 *
 *  \return Advertising state value. See ::bearerAdvStateValues.
 */
/*************************************************************************************************/
uint8_t AppBearerGetAdvState(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets the advertising state.
 *
 *  \param[in] advState  Advertising state value. See ::bearerAdvStateValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void AppBearerSetAdvState(uint8_t advState);

#ifdef __cplusplus
};
#endif

#endif /* APP_BEARER_H */
