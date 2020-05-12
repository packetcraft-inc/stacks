/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Generic Power Level Server Model API.
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
 * \addtogroup ModelGenPowerLevelSr Generic Power Level Server Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_GEN_POWER_LEVEL_SR_API_H
#define MMDL_GEN_POWER_LEVEL_SR_API_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 Macros
**************************************************************************************************/

/*! \brief Number of stored states (Present + Target + Last + Default + RangeMin + RangeMax) */
#define MMDL_GEN_POWER_LEVEL_STATE_CNT             6

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Model Power Last Server Status parameters structure */
typedef struct mmdlGenPowerLastStatusParam_tag
{
  mmdlGenPowerLevelState_t          last;              /*!< Present Power Last State */
} mmdlGenPowerLastStatusParam_t;

/*! \brief Model Power Default Server Status parameters structure */
typedef struct mmdlGenPowerDefaultStatusParam_tag
{
  mmdlGenPowerLevelState_t          state;             /*!< Present Power Default State */
} mmdlGenPowerDefaultStatusParam_t;

/*! \brief Model Power Range Server Status parameters structure */
typedef struct mmdlGenPowerRangeStatusParam_tag
{
  uint8_t                           statusCode;        /*!< Status Code */
  mmdlGenPowerLevelState_t          powerMin;          /*!< Minimum Power Range state */
  mmdlGenPowerLevelState_t          powerMax;          /*!< Maximum Power Range state */
} mmdlGenPowerRangeStatusParam_t;

/*! \brief Generic Power Level Server Model State Update event structure */
typedef struct mmdlGenPowerLevelSrStateUpdate_tag
{
  wsfMsgHdr_t                       hdr;               /*!< WSF message header */
  meshElementId_t                   elemId;            /*!< Element identifier */
  mmdlStateUpdateSrc_t              stateUpdateSource; /*!< Updated state source */
  mmdlGenPowerLevelState_t          state;             /*!< Updated state */
  uint32_t                          transitionMs;      /*!< Transition Time in millisecond steps */
  uint8_t                           delay5Ms;          /*!< Message execution delay in 5 ms steps */
} mmdlGenPowerLevelSrStateUpdate_t;

/*! \brief Generic Power Level Server Model Current State event structure */
typedef struct mmdlGenPowerLevelSrCurrentState_tag
{
  wsfMsgHdr_t                       hdr;               /*!< WSF message header */
  meshElementId_t                   elemId;            /*!< Element identifier */
  mmdlGenLevelState_t               state;             /*!< Updated state */
} mmdlGenPowerLevelSrCurrentState_t;

/*! \brief Generic Power Range Server Model State event structure */
typedef struct mmdlGenPowerLevelSrRangeState_tag
{
  wsfMsgHdr_t                       hdr;               /*!< WSF message header */
  meshElementId_t                   elemId;            /*!< Element identifier */
  mmdlGenLevelState_t               minState;          /*!< Minimum state */
  mmdlGenLevelState_t               maxState;          /*!< Maximum state */
} mmdlGenPowerLevelSrRangeState_t;

/*! \brief Generic Power Level Server Model event callback parameters structure */
typedef union mmdlGenPowerLevelSrEvent_tag
{
  wsfMsgHdr_t                       hdr;               /*!< WSF message header */
  mmdlGenPowerLevelSrStateUpdate_t  statusEvent;       /*!< State updated event. Used for
                                                        *   ::MMDL_GEN_POWER_LEVEL_SR_STATE_UPDATE_EVENT
                                                        */
  mmdlGenPowerLevelSrCurrentState_t currentStateEvent; /*!< Current state event. Sent after a Get request
                                                        *   from the upper layer. Used for
                                                        *   ::MMDL_GEN_POWER_LEVEL_SR_CURRENT_STATE_EVENT,
                                                        *   MMDL_GEN_POWER_LAST_SR_CURRENT_STATE_EVENT,
                                                        *   MMDL_GEN_POWER_DEFAULT_SR_CURRENT_STATE_EVENT,
                                                        *   MMDL_GEN_POWER_RANGE_SR_CURRENT_EVENT,
                                                        *   MMDL_GEN_POWER_DEFAULT_SR_STATE_UPDATE_EVENT
                                                        */
  mmdlGenPowerLevelSrRangeState_t   rangeStatusEvent;  /*!< State updated event. Used for
                                                        *   ::MMDL_GEN_POWER_RANGE_SR_STATE_UPDATE_EVENT,
                                                        */
} mmdlGenPowerLevelSrEvent_t;

/*! \brief Model Generic Power Level Server descriptor definition */
typedef struct mmdlGenPowerLevelSrDesc_tag
{
  mmdlGenPowerLevelState_t  *pStoredStates;       /*!< Pointer to the structure that stores
                                                   *   current states and scene data.
                                                   *   0: Current state,
                                                   *   1: Target state, 2: Last state,
                                                   *   3: Default state, 4: Min Range
                                                   *   5: Max Range, 6-: :MMDL_NUM_OF_SCENES states
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
  int16_t                   transitionStep;       /*!< Transition state update step */
  uint16_t                  steps;                /*!< The number of transition steps */
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
  mmdlGenPowerLevelState_t  initialState;         /*!< Initial state within a transaction.
                                                   */
  mmdlStateUpdateSrc_t      updateSource;         /*!< State update source. Cached for transitions.
                                                   */
} mmdlGenPowerLevelSrDesc_t;

/*************************************************************************************************/
/*!
 * \brief     Model Generic Power Level received callback
 *
 * \param[in] pEvent  Pointer to Generic Power Level Server Event
 *
 * \return    None.
 */
/*************************************************************************************************/
typedef void (*mmdlGenPowerLevelSrRecvCback_t)(const mmdlGenPowerLevelSrEvent_t *pEvent);

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlGenPowerLevelSrHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t mmdlGenPowerLevelSrRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Generic Power Level Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSrInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Generic Power Level Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Generic Power Level Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSrHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Generic Power Level Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSrHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Publish a GenPowerLevel Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSrPublish(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Set the Generic Power Actual state of the element.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] targetState  Target State for this transaction. See ::mmdlGenPowerLevelState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSrSetState(meshElementId_t elementId, mmdlGenPowerLevelState_t targetState);

/*************************************************************************************************/
/*!
 *  \brief     Get the Generic Power Actual state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSrGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Get the Generic Power Last state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLastSrGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Set the Generic Power Default state of the element.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] targetState  Target State for this transaction. See ::mmdlGenPowerLevelState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerDefaultSrSetState(meshElementId_t elementId, mmdlGenPowerLevelState_t targetState);

/*************************************************************************************************/
/*!
 *  \brief     Get the Generic Power Default state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerDefaultSrGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Get the Generic Power Range state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerRangeSrGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSrRegister(mmdlEventCback_t recvCback);

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Power Level Actual State and a Generic OnPowerUp state.
 *
 *  \param[in] onPowerUpElemId  Element identifier where the OnPowerUp state resides.
 *  \param[in] powElemId        Element identifier where the Power Level Actual state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSrBind2OnPowerUp(meshElementId_t onPowerUpElemId, meshElementId_t powElemId);

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Generic Power Actual State and a Generic Level state.
 *
 *  \param[in] gplElemId  Element identifier where the Generic Power Actual state resides.
 *  \param[in] glvElemId  Element identifier where the Generic Level state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSrBind2GenLevel(meshElementId_t gplElemId, meshElementId_t glvElemId);

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Generic Power Actual State and a Generic On Off state.
 *
 *  \param[in] gplElemId    Element identifier where the Generic Power Actual state resides.
 *  \param[in] onoffElemId  Element identifier where the On Off state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSrBind2GenOnOff(meshElementId_t gplElemId, meshElementId_t onoffElemId);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_GEN_POWER_LEVEL_SR_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
