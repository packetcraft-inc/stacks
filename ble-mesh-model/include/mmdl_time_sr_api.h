/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Time Server Model API.
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
 * \addtogroup ModelTimeSr Time Server Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_TIME_SR_API_H
#define MMDL_TIME_SR_API_H

#include "wsf_timer.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Model Time Server Status parameters structure */
typedef struct mmdlTimeStatusParam_tag
{
  uint64_t     taiSeconds;         /*!< Current TAI time in seconds */
  uint8_t      subSecond;          /*!< Sub-second time */
  uint8_t      uncertainty;        /*!< Time Authority */
  uint8_t      timeAuthority;      /*!< TAI-UTC Delta */
  int16_t      taiUtcDelta;        /*!< Current difference between TAI and UTC in seconds */
  int8_t       timeZoneOffset;     /*!< Local Time Zone Offset in 15-minutes increments */
} mmdlTimeStatusParam_t;

/*! \brief Model Time Zone Server Status parameters structure */
typedef struct mmdlTimeZoneStatusParam_tag
{
  uint8_t      currentOffset;      /*!< Time Zone Offset Current */
  uint8_t      newOffset;          /*!< Time Zone Offset New */
  uint64_t     taiOfZoneChange;    /*!< TAI Seconds time of upcoming offset change */
} mmdlTimeZoneStatusParam_t;

/*! \brief Model Time Role Server Status parameters structure */
typedef struct mmdlTimeRoleStatusParam_tag
{
  uint8_t      timeRole;           /*!< Time Role */
} mmdlTimeRoleStatusParam_t;

/*! \brief Union of Time states */
typedef union
{
  mmdlTimeState_t        timeState;          /*!< Time state */
  mmdlTimeZoneState_t    timeZoneState;      /*!< Time Zone state */
  mmdlTimeDeltaState_t   timeDeltaState;     /*!< Time Delta state */
  mmdlTimeRoleState_t    timeRoleState;      /*!< Time Role state */
} mmdlTimeStates_t;

/*! \brief Time Server Model State Update event structure */
typedef struct mmdlTimeSrStateUpdate_tag
{
  wsfMsgHdr_t              hdr;                /*!< WSF message header */
  meshElementId_t          elemId;             /*!< Element identifier */
  mmdlStateUpdateSrc_t     stateUpdateSource;  /*!< Updated state source */
  mmdlTimeStates_t         state;              /*!< Update time state union */
} mmdlTimeSrStateUpdate_t;

/*! \brief Time Server Model Current State event structure */
typedef struct mmdlTimeSrCurrentState_tag
{
  wsfMsgHdr_t              hdr;                /*!< WSF message header */
  meshElementId_t          elemId;             /*!< Element identifier */
  mmdlTimeStates_t         state;              /*!< Update time state union */
} mmdlTimeSrCurrentState_t;

/*! \brief Time Server Model event callback parameters structure */
typedef union mmdlGenTimeSrEvent_tag
{
  wsfMsgHdr_t              hdr;               /*!< WSF message header */

  mmdlTimeSrStateUpdate_t  statusEvent;       /*!< State updated event. Used for
                                               *   ::MMDL_TIME_SR_STATE_UPDATE_EVENT.
                                               */
  mmdlTimeSrCurrentState_t currentStateEvent; /*!< Current state event. Sent after a Get request
                                               *   from the upper layer. Used for
                                               *   ::MMDL_TIME_SR_CURRENT_STATE_EVENT.
                                               */
} mmdlTimeSrEvent_t;

/*! \brief Model Time Server descriptor definition */
typedef struct mmdlTimeSrDesc_tag
{
  mmdlTimeState_t          storedTimeState;       /*!< Field that stores the Time state */
  mmdlTimeZoneState_t      storedTimeZoneState;   /*!< Field that stores the Time Zone state */
  mmdlTimeDeltaState_t     storedTimeDeltaState;  /*!< Field that stores the Time Delta state */
  mmdlTimeRoleState_t      storedTimeRoleState;   /*!< Field that stores the Time Zone state */
  mmdlTimeRoleState_t      initialState;          /*!< Initial state within a transaction. */
} mmdlTimeSrDesc_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlTimeSrHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t mmdlTimeSrRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Time Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlTimeSrInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Time Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Time Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSrHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Time Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSrHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Publish a Time Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSrPublish(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Set the Time state of the element.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] pTargetState  Target State for this transaction. See ::mmdlTimeState_t.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSrSetState(meshElementId_t elementId, mmdlTimeState_t *pTargetState);

/*************************************************************************************************/
/*!
 *  \brief     Get the Time state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSrGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Set the Time Zone Offset New state of the element.
 *
 *  \param[in] elementId     Identifier of the Element implementing the model.
 *  \param[in] pTargetState  Target State for this transaction. See ::mmdlTimeZoneState_t.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSrZoneSetState(meshElementId_t elementId, mmdlTimeZoneState_t *pTargetState);

/*************************************************************************************************/
/*!
 *  \brief     Get the Time Zone Offset Current state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSrZoneGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Set the TAI-UTC Delta New state of the element.
 *
 *  \param[in] elementId     Identifier of the Element implementing the model.
 *  \param[in] pTargetState  Target State for this transaction. See ::mmdlTimeDeltaState_t.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeDeltaSrSetState(meshElementId_t elementId, mmdlTimeDeltaState_t *pTargetState);

/*************************************************************************************************/
/*!
 *  \brief     Get the TAI-UTC Delta Current state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeDeltaSrGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSrRegister(mmdlEventCback_t recvCback);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_TIME_SR_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
