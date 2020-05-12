/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Generic Battery Server Model API.
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
 * \addtogroup ModelGenBatterySr Generic Battery Server Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_GEN_BATTERY_SR_API_H
#define MMDL_GEN_BATTERY_SR_API_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 Macros
**************************************************************************************************/

/*! \brief Number of stored states (Present + Target) */
#define MMDL_GEN_BATTERY_STATE_CNT             2

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Model Battery Server Status parameters structure */
typedef struct mmdlGenBatteryStatusParam_tag
{
  uint8_t              state;           /*!< Received published state */
  uint32_t             timeToDischarge; /*!< Received published time to discharge state */
  uint32_t             timeToCharge;    /*!< Received published time to charge state */
  uint8_t              flags;           /*!< Receviced published flag state */
} mmdlGenBatteryStatusParam_t;

/*! \brief Generic Level Server Model State Update event structure */
typedef struct mmdlGenBatterySrStateUpdate_tag
{
  wsfMsgHdr_t           hdr;                /*!< WSF message header */
  meshElementId_t       elemId;             /*!< Element identifier */
  mmdlStateUpdateSrc_t  stateUpdateSource;  /*!< Updated state source */
  mmdlGenBatteryState_t state;              /*!< Updated state */
} mmdlGenBatterySrStateUpdate_t;

/*! \brief Generic Battery Server Model Current State event structure */
typedef struct mmdlGenBatterySrCurrentState_tag
{
  wsfMsgHdr_t           hdr;              /*!< WSF message header */
  meshElementId_t       elemId;           /*!< Element identifier */
  mmdlGenBatteryState_t state;            /*!< Updated state */
} mmdlGenBatterySrCurrentState_t;

/*! \brief Generic Battery Server Model event callback parameters structure */
typedef union mmdlGenBatterySrEvent_tag
{
  wsfMsgHdr_t                    hdr;               /*!< WSF message header */
  mmdlGenBatterySrStateUpdate_t  statusEvent;       /*!< State updated event. Used for
                                                     *   ::MMDL_GEN_BATTERY_SR_STATE_UPDATE_EVENT.
                                                     */
  mmdlGenBatterySrCurrentState_t currentStateEvent; /*!< Current state event. Sent after a Get request
                                                     *   from the upper layer. Used for
                                                     *   ::MMDL_GEN_BATTERY_SR_CURRENT_STATE_EVENT.
                                                     */
} mmdlGenBatterySrEvent_t;

/*! \brief Model Generic Battery Server descriptor definition */
typedef struct mmdlGenBatterySrDesc_tag
{
  mmdlGenBatteryState_t     *pStoredStates;       /*!< Pointer to the structure that stores
                                                   *   current state and scene data. First
                                                   *   value is always the current one.
                                                   *   Second value is the target state.
                                                   */
} mmdlGenBatterySrDesc_t;

/*!
 * \brief Model Generic Battery received callback
 *
 * \param pEvent  Pointer to Generic Battery Server Event
 */
typedef void (*mmdlGenBatterySrRecvCback_t)(const mmdlGenBatterySrEvent_t *pEvent);

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlGenBatterySrHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t mmdlGenBatterySrRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Generic Battery Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlGenBatterySrInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Generic Battery Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Generic Battery Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenBatterySrHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Generic Battery Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenBatterySrHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Publish a Generic Battery Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenBatterySrPublish(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Set the local Generic Battery state of the element.
 *
 *  \param[in] elementId     Identifier of the Element implementing the model.
 *  \param[in] pTargetState  Target State for this transaction. See ::mmdlGenBatteryState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenBatterySrSetState(meshElementId_t elementId,
                              const mmdlGenBatteryState_t *pTargetState);

/*************************************************************************************************/
/*!
 *  \brief     Get the local Generic Battery state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenBatterySrGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenBatterySrRegister(mmdlEventCback_t recvCback);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_GEN_BATTERY_SR_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
