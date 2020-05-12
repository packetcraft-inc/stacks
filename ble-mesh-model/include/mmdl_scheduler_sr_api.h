/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Scheduler Server Model API.
 *
 *  Copyright (c) 2010-2018 Arm Ltd. All Rights Reserved.
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

/*! ***********************************************************************************************
 * \addtogroup ModelSchedulerSr Scheduler Server Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_SCHEDULER_SR_API_H
#define MMDL_SCHEDULER_SR_API_H

#include "wsf_timer.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Scheduler Server Register State entry definition */
typedef struct mmdlSchedulerSrRegisterEntry_tag
{
  mmdlSchedulerRegisterEntry_t   regEntry;  /*!< Scheduler Register State entry definition */
  bool_t                         inUse;     /*!< In use flag */
} mmdlSchedulerSrRegisterEntry_t;

/*! \brief Model Scheduler Server descriptor definition */
typedef struct mmdlSchedulerSrDesc_tag
{
  mmdlSchedulerSrRegisterEntry_t registerState[MMDL_SCHEDULER_REGISTER_ENTRY_MAX + 1]; /*!< Register
                                                                                        *   state
                                                                                        */
} mmdlSchedulerSrDesc_t;

/*!
 *  \brief Scheduler Server Model Start Schedule event structure
 *  \note  Upon receiving this event the application having a reliable time source
 *         can start scheduling an action identified by the id field.
 */
typedef struct mmdlSchedulerSrStartScheduleEvent_tag
{
  wsfMsgHdr_t                          hdr;         /*!< WSF message header */
  meshElementId_t                      elementId;   /*!< Element ID */
  uint8_t                              id;          /*!< Identifier of the scheduled event */
  uint8_t                              year;        /*!< Last two digits for the Year of the event
                                                     *   or ::MMDL_SCHEDULER_REGISTER_YEAR_ALL
                                                     */
   mmdlSchedulerRegisterMonthBf_t      months;      /*!< Bit-field for months. A value of 1 for bit N
                                                     *   means event occurs in month N
                                                     */
   mmdlSchedulerRegisterDay_t          day;         /*!< Day of the event */
   mmdlSchedulerRegisterHour_t         hour;        /*!< Hour of the event */
   mmdlSchedulerRegisterMinute_t       minute;      /*!< Minute of the event */
   mmdlSchedulerRegisterSecond_t       second;      /*!< Second of the event */
   mmdlSchedulerRegisterDayOfWeekBf_t  daysOfWeek;  /*!< Bit-field for days of week. A value of 1 for bit N
                                                     *   means event occurs in day N
                                                     */
} mmdlSchedulerSrStartScheduleEvent_t;

/*! \brief Scheduler Server Model Stop Schedule event structure */
typedef struct mmdlSchedulerSrStopScheduleEvent_tag
{
  wsfMsgHdr_t                          hdr;         /*!< WSF message header */
  meshElementId_t                      elementId;   /*!< Element ID */
  uint8_t                              id;          /*!< Identifier of the scheduled entry */
} mmdlSchedulerSrStopScheduleEvent_t;

/*! \brief Scheduler Server Model event callback parameters structure */
typedef union mmdlSchedulerSrEvent_tag
{
  wsfMsgHdr_t                          hdr;                /*!< WSF message header */
  mmdlSchedulerSrStartScheduleEvent_t  schedStartEvent;    /*!< Start schedule event. Valid for
                                                            *   ::MMDL_SCHEDULER_SR_START_SCHEDULE_EVENT
                                                            */
  mmdlSchedulerSrStopScheduleEvent_t   schedStopEvent;     /*!< Stop schedule event. Valid for
                                                            *   ::MMDL_SCHEDULER_SR_STOP_SCHEDULE_EVENT
                                                            */
} mmdlSchedulerSrEvent_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlSchedulerSrHandlerId;

/** \name Supported opcodes
 *
 */
/**@{*/
extern const meshMsgOpcode_t mmdlSchedulerSrRcvdOpcodes[];
extern const meshMsgOpcode_t mmdlSchedulerSetupSrRcvdOpcodes[];
/**@}*/

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Scheduler Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlSchedulerSrInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Scheduler Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Scheduler Server Model.
 *
 *  \return    None.
 *
 *  \remarks   Scheduler Server and Setup Server will share the handler id.
 */
/*************************************************************************************************/
void MmdlSchedulerSrHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Scheduler Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerSrHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Scheduler Setup Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerSetupSrHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Publish a Scheduler Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerSrPublish(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerSrRegister(mmdlEventCback_t recvCback);

/*************************************************************************************************/
/*!
 *  \brief     Triggers the action associated to the scheduled event identified by the id field.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] id         Identifier of the scheduled event.
 *                        See ::mmdlSchedulerSrStartScheduleEvent_t
 *
 *  \return    None.
 *
 *  \remarks   Upon receiving a ::mmdlSchedulerSrStartScheduleEvent_t the application can start
 *             scheduling an event and call this function to perform the associated action. If the
 *             event is not periodical then ::MmdlSchedulerSrClearEvent must be called.
 */
/*************************************************************************************************/
void MmdlSchedulerSrTriggerEventAction(meshElementId_t elementId, uint8_t id);

/*************************************************************************************************/
/*!
 *  \brief     Clears the scheduled event identified by the id field.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] id         Identifier of the scheduled event.
 *                        See ::mmdlSchedulerSrStartScheduleEvent_t
 *
 *  \return    None.
 *
 *  \remarks   This function should be called if the application cannot schedule an event or if
 *             the event is not periodical and the scheduled time elapsed.
 */
/*************************************************************************************************/
void MmdlSchedulerSrClearEvent(meshElementId_t elementId, uint8_t id);

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Scheduler Register State and a Generic On Off state.
 *
 *  \param[in] schedElemId  Element identifier where the Scheduler Register state resides.
 *  \param[in] onoffElemId  Element identifier where the On Off state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerSrBind2GenOnOff(meshElementId_t schedElemId, meshElementId_t onoffElemId);

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Scheduler Register State and a Scene Register state.
 *
 *  \param[in] schedElemId  Element identifier where the Scheduler Register state resides.
 *  \param[in] sceneElemId  Element identifier where the Scene Register state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerSrBind2SceneReg(meshElementId_t schedElemId, meshElementId_t sceneElemId);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_SCHEDULER_SR_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
