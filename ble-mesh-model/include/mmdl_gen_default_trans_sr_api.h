/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Generic Default Transition Server Model API.
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
 * \addtogroup ModelGenDefaultTransSr Generic Default Transition Server Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_GEN_DEFAULT_TRANS_SR_API_H
#define MMDL_GEN_DEFAULT_TRANS_SR_API_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 Macros
**************************************************************************************************/

/*! \brief Number of stored states (Present + Target) */
#define MMDL_GEN_DEFAULT_TRANS_STATE_CNT             2

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Model Default Transition Server Status parameters structure */
typedef struct mmdlGenDefaultTransStatusParam_tag
{
  mmdlGenDefaultTransState_t  transitionTime;     /*!< Present Transition Time State */
} mmdlGenDefaultTransStatusParam_t;

/*! \brief Generic Default Transition Server Model State Update event structure */
typedef struct mmdlGenDefaultTransSrStateUpdate_tag
{
  wsfMsgHdr_t                 hdr;                /*!< WSF message header */
  meshElementId_t             elemId;             /*!< Element identifier */
  mmdlStateUpdateSrc_t        stateUpdateSource;  /*!< Updated state source */
  mmdlGenDefaultTransState_t  state;              /*!< Updated state */
} mmdlGenDefaultTransSrStateUpdate_t;

/*! \brief Generic Default Transition Server Model Current State event structure */
typedef struct mmdlGenDefaultTransSrCurrentState_tag
{
  wsfMsgHdr_t                 hdr;                /*!< WSF message header */
  meshElementId_t             elemId;             /*!< Element identifier */
  mmdlGenDefaultTransState_t  state;              /*!< Updated state */
} mmdlGenDefaultTransSrCurrentState_t;

/*! \brief Generic Default Transition Server Model event callback parameters structure */
typedef union mmdlGenDefaultTransSrEvent_tag
{
  wsfMsgHdr_t                         hdr;                 /*!< WSF message header */
  mmdlGenDefaultTransSrStateUpdate_t  statusEvent;         /*!< State updated event. Used for
                                                            *   ::MMDL_GEN_DEFAULT_TRANS_SR_STATE_UPDATE_EVENT.
                                                            */
  mmdlGenDefaultTransSrCurrentState_t currentStateEvent;   /*!< Current state event. Sent after
                                                            * a Get request from the upper layer.
                                                            * Used for
                                                            * ::MMDL_GEN_DEFAULT_TRANS_SR_CURRENT_STATE_EVENT.
                                                            */
} mmdlGenDefaultTransSrEvent_t;

/*! \brief Model Generic Default Transition Server descriptor definition */
typedef struct mmdlGenDefaultTransSrDesc_tag
{
  mmdlGenDefaultTransState_t   *pStoredStates;    /*!< Pointer to the structure that stores
                                                   *   current state and scene data. First
                                                   *   value is always the current one.
                                                   *   Second value is the target state.
                                                   */
} mmdlGenDefaultTransSrDesc_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlGenDefaultTransSrHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t mmdlGenDefaultTransSrRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Generic Default Transition Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlGenDefaultTransSrInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Generic Default Transition Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Generic Default Transition Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenDefaultTransSrHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Generic Default Transition Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenDefaultTransSrHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Publish a Gen Default Transition Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenDefaultTransSrPublish(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Set the local state.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] targetState  Target State for this transaction. See ::mmdlGenDefaultTransState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenDefaultTransSrSetState(meshElementId_t elementId, mmdlGenDefaultTransState_t targetState);

/*************************************************************************************************/
/*!
 *  \brief     Get the local state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenDefaultTransSrGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenDefaultTransSrRegister(mmdlEventCback_t recvCback);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_GEN_DEFAULT_TRANS_SR_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
