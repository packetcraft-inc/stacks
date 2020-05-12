/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Time Setup Server Model API.
 *
 *  Copyright (c) 2010-2019 Arm Ltd. All Rights Reserved.
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

#ifndef MMDL_TIMESETUP_SR_API_H
#define MMDL_TIMESETUP_SR_API_H

#include "wsf_timer.h"
#include "mmdl_time_sr_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlTimeSetupSrHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t mmdlTimeSetupSrRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Time Setup Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlTimeSetupSrInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Time Setup Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Generic On Off Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSetupSrHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Time Setup Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSetupSrHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Set the Time state of the element.
 *
 *  \param[in] elementId     Identifier of the Element implementing the model.
 *  \param[in] pTargetState  Target State for this transaction. See ::mmdlGenLevelState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSetupSrSetState(meshElementId_t elementId, mmdlTimeState_t *pTargetState);

/*************************************************************************************************/
/*!
 *  \brief     Get the Time state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSetupSrGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Get the Time Zone Offset Current state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSetupSrZoneGetState(meshElementId_t elementId);

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
void MmdlTimeSetupSrZoneSetState(meshElementId_t elementId, mmdlTimeZoneState_t *pTargetState);

/*************************************************************************************************/
/*!
 *  \brief     Set the TAI-UTC Delta New state of the element.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model.
 *  \param[in] pTargetState  Target State for this transaction. See ::mmdlTimeDeltaState_t.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSetupSrDeltaSetState(meshElementId_t elementId, mmdlTimeDeltaState_t *pTargetState);

/*************************************************************************************************/
/*!
 *  \brief     Get the TAI-UTC Delta Current state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSetupSrDeltaGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Set the Time Role state of the element.
 *
 *  \param[in] elementId     Identifier of the Element implementing the model.
 *  \param[in] pTargetState  Target State for this transaction. See ::mmdlTimeRoleState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSetupSrRoleSetState(meshElementId_t elementId, mmdlTimeRoleState_t *pTargetState);

/*************************************************************************************************/
/*!
 *  \brief     Get the Time Role state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSetupSrRoleGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSetupSrRegister(mmdlEventCback_t recvCback);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_TIMESETUP_SR_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
