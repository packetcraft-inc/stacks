/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Generic On Off Server Model API.
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
 * \addtogroup ModelGenOnOffSr Generic On Off Server Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_GEN_ONOFF_SR_API_H
#define MMDL_GEN_ONOFF_SR_API_H

#include "wsf_timer.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 Macros
**************************************************************************************************/

/*! \brief Number of stored states (Present + Target) */
#define MMDL_GEN_ONOFF_STATE_CNT             2

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Model On Off Server Status parameters structure */
typedef struct mmdlGenOnOffStatusParam_tag
{
  mmdlGenOnOffState_t   presentOnOff;       /*!< Present On Off State */
  mmdlGenOnOffState_t   targetOnOff;        /*!< Target On Off State */
  uint8_t               remainingTime;      /*!< Remaining time */
} mmdlGenOnOffStatusParam_t;

/*! \brief Generic OnOff Server Model State Update event structure */
typedef struct mmdlGenOnOffSrStateUpdate_tag
{
  wsfMsgHdr_t           hdr;                /*!< WSF message header */
  meshElementId_t       elemId;             /*!< Element identifier */
  mmdlStateUpdateSrc_t  stateUpdateSource;  /*!< Updated state source */
  mmdlGenOnOffState_t   state;              /*!< Updated state */
} mmdlGenOnOffSrStateUpdate_t;

/*! \brief Generic OnOff Server Model Current State event structure */
typedef struct mmdlGenOnOffSrCurrentState_tag
{
  wsfMsgHdr_t           hdr;                /*!< WSF message header */
  meshElementId_t       elemId;             /*!< Element identifier */
  mmdlGenOnOffState_t   state;              /*!< Updated state */
} mmdlGenOnOffSrCurrentState_t;

/*! \brief Generic OnOff Server Model event callback parameters structure */
typedef union mmdlGenOnOffSrEvent_tag
{
  wsfMsgHdr_t hdr;                                /*!< WSF message header */
  mmdlGenOnOffSrStateUpdate_t  statusEvent;       /*!< State updated event. Used for
                                                   *   ::MMDL_GEN_ONOFF_SR_STATE_UPDATE_EVENT.
                                                   */
  mmdlGenOnOffSrCurrentState_t currentStateEvent; /*!< Current state event. Sent after a Get request
                                                   *   from the upper layer. Used for
                                                   *   ::MMDL_GEN_ONOFF_SR_CURRENT_STATE_EVENT.
                                                   */
} mmdlGenOnOffSrEvent_t;


/*! \brief Model Generic OnOff Server descriptor definition */
typedef struct mmdlGenOnOffSrDesc_tag
{
  mmdlGenOnOffState_t       *pStoredStates;       /*!< Pointer to the structure that stores
                                                   *   current state and scene data. First
                                                   *   value is always the current one.
                                                   *   Second value is the target state.
                                                   *   Sequential values represent scene
                                                   *   values starting with scene index 0 and
                                                   *   ending with index :MMDL_NUM_OF_SCENES - 1.
                                                   *   Structure will store :MMDL_NUM_OF_SCENES +
                                                   *   MMDL_GEN_ONOFF_STATE_CNT states.
                                                   */
  mmdlNvmSaveHandler_t      fNvmSaveStates;       /*!< Pointer to function that saves
                                                   *   Model instance states in NVM
                                                   */
  wsfTimer_t                transitionTimer;      /*!< WSF Timer for delay and state transition */
  wsfTimer_t                msgRcvdTimer;         /*!< Timer to manage received logically group
                                                   * messages.
                                                   */
  uint32_t                  remainingTimeMs;      /*!< Time remaining until the current state is
                                                   *   replaced with the target state. If set to 0,
                                                   *   the target state is ignored. Unit is 1 ms.
                                                   */
  uint8_t                   delay5Ms;             /*!< Delay until the transition to the new state
                                                   *   begins. Unit is 5 ms.
                                                   */
  uint8_t                   transactionId;        /*!< Transaction Identifier used to logically group a
                                                   *   series of messages.
                                                   */
  meshAddress_t             srcAddr;              /*!< Source address of the logically grouped series of
                                                   *   messages.
                                                   */
  bool_t                    ackPending;           /*!< TRUE if an ACK is pending for the last received
                                                   *   message.
                                                   */
  bool_t                    ackForUnicast;        /*!< TRUE if the last message was received as a unicast,
                                                   *   FALSE otherwise.
                                                   */
  uint16_t                  ackAppKeyIndex;       /*!< AppKeyIndex used for the last received message.
                                                   */
  mmdlStateUpdateSrc_t      updateSource;         /*!< State update source. Cached for transitions.
                                                   */
} mmdlGenOnOffSrDesc_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlGenOnOffSrHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t mmdlGenOnOffSrRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Generic On Off Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlGenOnOffSrInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Generic On Off Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Generic On Off Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenOnOffSrHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Generic On Off Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenOnOffSrHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Publish a GenOnOff Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenOnOffSrPublish(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Set the local state.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] targetState  Target State for this transaction. See ::mmdlGenOnOffState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenOnOffSrSetState(meshElementId_t elementId, mmdlGenOnOffState_t targetState);

/*************************************************************************************************/
/*!
 *  \brief     Get the local state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenOnOffSrGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenOnOffSrRegister(mmdlEventCback_t recvCback);

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between the Generic OnPowerUp and the Generic OnOff state.
 *
 *  \param[in] onPowerUpElemId  Element identifier where the OnPowerUp state resides.
 *  \param[in] onOffElemId      Element identifier where the Generic OnOff state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenOnOffSrBind2OnPowerUp(meshElementId_t onPowerUpElemId, meshElementId_t onOffElemId);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_GEN_ONOFF_SR_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
