/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Generic Level Server Model API.
 *
 *  Copyright (c) 2010-2018 Arm Ltd. All Rights Reserved.
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

/*! ***********************************************************************************************
 * \addtogroup ModelGenLevelSr Generic Level Server Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_GEN_LEVEL_SR_API_H
#define MMDL_GEN_LEVEL_SR_API_H

#include "wsf_timer.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 Macros
**************************************************************************************************/

/*! \brief Number of stored states (Present + Target) */
#define MMDL_GEN_LEVEL_STATE_CNT             2

/*! \brief The Generic Move Set timer update interval in milliseconds */
#define MMDL_GEN_LEVEL_MOVE_UPDATE_INTERVAL  100

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Model Level Server Status parameters structure */
typedef struct mmdlGenLevelStatusParam_tag
{
  mmdlGenLevelState_t   presentLevel;       /*!< Present Level State */
  mmdlGenLevelState_t   targetLevel;        /*!< Target Level State */
  uint8_t               remainingTime;      /*!< Remaining time */
} mmdlGenLevelStatusParam_t;

/*! \brief Generic Level Server Model State Update event structure */
typedef struct mmdlGenLevelSrStateUpdate_tag
{
  wsfMsgHdr_t           hdr;                /*!< WSF message header */
  meshElementId_t       elemId;             /*!< Element identifier */
  mmdlStateUpdateSrc_t  stateUpdateSource;  /*!< Updated state source */
  mmdlGenLevelState_t   state;              /*!< Updated state */
} mmdlGenLevelSrStateUpdate_t;

/*! \brief Generic Level Server Model Current State event structure */
typedef struct mmdlGenLevelSrCurrentState_tag
{
  wsfMsgHdr_t           hdr;                /*!< WSF message header */
  meshElementId_t       elemId;             /*!< Element identifier */
  mmdlGenLevelState_t   state;              /*!< Updated state */
} mmdlGenLevelSrCurrentState_t;

/*! \brief Generic Level Server Model event callback parameters structure */
typedef union mmdlGenLevelSrEvent_tag
{
  wsfMsgHdr_t                  hdr;               /*!< WSF message header */
  mmdlGenLevelSrStateUpdate_t  statusEvent;       /*!< State updated event. Used for
                                                   *   ::MMDL_GEN_LEVEL_SR_STATE_UPDATE_EVENT.
                                                   */
  mmdlGenLevelSrCurrentState_t currentStateEvent; /*!< Current state event. Sent after a Get request
                                                   *   from the upper layer. Used for
                                                   *   ::MMDL_GEN_LEVEL_SR_CURRENT_STATE_EVENT.
                                                   */
} mmdlGenLevelSrEvent_t;

/*! \brief Model Generic Level Server descriptor definition */
typedef struct mmdlGenLevelSrDesc_tag
{
  mmdlGenLevelState_t   *pStoredStates;    /*!< Pointer to the structure that stores
                                            *   current state and scene data. First
                                            *   value is always the current one.
                                            *   Second value is the target state.
                                            *   Sequential values represent scene
                                            *   values starting with scene index 0 and
                                            *   ending with index :MMDL_NUM_OF_SCENES - 1.
                                            *   Structure will store :MMDL_NUM_OF_SCENES +
                                            *   MMDL_GEN_LEVEL_STATE_CNT states.
                                            */
  wsfTimer_t            transitionTimer;   /*!< WSF Timer for delay and state transition */
  wsfTimer_t            msgRcvdTimer;      /*!< Timer to manage received logically group
                                            * messages.
                                            */
  uint32_t              remainingTimeMs;   /*!< Time remaining until the current state is
                                            *   replaced with the target state. If set to 0,
                                            *   the target state is ignored. Unit is 1 ms.
                                            */
  int16_t               transitionStep;    /*!< Transition state update step */
  uint16_t              steps;             /*!< The number of transition steps */
  uint8_t               delay5Ms;          /*!< Delay until the transition to the new state
                                            *   begins. Unit is 5 ms.
                                            */
  bool_t                isMoveSet;         /*!< Flag to show if server is processing
                                            *   Move Set message.
                                            */
  mmdlGenLevelState_t   deltaLevelStep;    /*!< Delta Level step value to calculate move speed.
                                            *   value is only necessary if isMoveSet == TRUE.
                                            */
  uint8_t               transactionId;     /*!< Transaction Identifier used to logically group a
                                            *   series of messages.
                                            */
  meshAddress_t         srcAddr;           /*!< Source address of the logically grouped series of
                                            *   messages.
                                            */
  bool_t                ackPending;        /*!< TRUE if an ACK is pending for the last received
                                            *   message.
                                            */
  bool_t                ackForUnicast;     /*!< TRUE if the last message was received as a unicast,
                                            *   FALSE otherwise.
                                            */
  uint16_t              ackAppKeyIndex;    /*!< AppKeyIndex used for the last received message.
                                            */
  mmdlGenLevelState_t   initialState;      /*!< Initial state within a transaction.
                                            */
  mmdlStateUpdateSrc_t  updateSource;      /*!< State update source. Cached for transitions.
                                            */
} mmdlGenLevelSrDesc_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlGenLevelSrHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t mmdlGenLevelSrRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Generic Level Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlGenLevelSrInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Generic Level Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Generic On Off Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelSrHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Generic Level Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelSrHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Publish a GenLevel Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelSrPublish(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Set the Generic Level state of the element.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] targetState  Target State for this transaction. See ::mmdlGenLevelState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelSrSetState(meshElementId_t elementId, mmdlGenLevelState_t targetState);

/*************************************************************************************************/
/*!
 *  \brief     Get the Generic Level state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelSrGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelSrRegister(mmdlEventCback_t recvCback);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_GEN_LEVEL_SR_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
