/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Generic Power OnOff Server Model API.
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
 * \addtogroup ModelGenPowerOnOffSr Generic Power OnOff Server Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_GEN_POWER_ONOFF_SR_API_H
#define MMDL_GEN_POWER_ONOFF_SR_API_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 Macros
**************************************************************************************************/

/*! \brief Number of stored states (Present + Target) */
#define MMDL_GEN_POWER_ONOFF_STATE_CNT             2

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Model Power OnOff Server Status parameters structure */
typedef struct mmdlGenPowOnOffStatusParam_tag
{
  mmdlGenOnPowerUpState_t         onPowerUp;          /*!< Value of the OnPowerUp State */
} mmdlGenPowOnOffStatusParam_t;

/*! \brief Generic Power OnOff Server Model State Update event structure */
typedef struct mmdlGenPowOnOffSrStateUpdate_tag
{
  wsfMsgHdr_t                     hdr;                /*!< WSF message header */
  meshElementId_t                 elemId;             /*!< Element identifier */
  mmdlStateUpdateSrc_t            stateUpdateSource;  /*!< Updated state source */
  mmdlGenOnPowerUpState_t         state;              /*!< Updated state */
} mmdlGenPowOnOffSrStateUpdate_t;

/*! \brief Generic Power OnOff Server Model Current State event structure */
typedef struct mmdlGenPowOnOffSrCurrentState_tag
{
  wsfMsgHdr_t                     hdr;                /*!< WSF message header */
  meshElementId_t                 elemId;             /*!< Element identifier */
  mmdlGenOnPowerUpState_t         state;              /*!< Updated state */
} mmdlGenPowOnOffSrCurrentState_t;

/*! \brief Generic Power OnOff Server Model event callback parameters structure */
typedef union mmdlGenPowOnOffSrEvent_tag
{
  wsfMsgHdr_t                     hdr;                /*!< WSF message header */
  mmdlGenPowOnOffSrStateUpdate_t  statusEvent;        /*!< State updated event. Used for
                                                       *   ::MMDL_GEN_POWER_ONOFF_SR_STATE_UPDATE_EVENT.
                                                       */
  mmdlGenPowOnOffSrCurrentState_t currentStateEvent;  /*!< Current state event. Sent after a Get
                                                       *   request from the upper layer. Used for
                                                       *   ::MMDL_GEN_POWER_ONOFF_SR_CURRENT_STATE_EVENT.
                                                       */
} mmdlGenPowOnOffSrEvent_t;

/*! \brief Model Power Generic OnOff Server descriptor definition */
typedef struct mmdlGenPowOnOffSrDesc_tag
{
  mmdlGenOnPowerUpState_t         *pStoredStates;     /*!< The structure that stores
                                                       *   current state.
                                                       */
  mmdlNvmSaveHandler_t            fNvmSaveStates;     /*!< Pointer to function that saves
                                                       *   Model instance states in NVM
                                                       */
} mmdlGenPowOnOffSrDesc_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlGenPowOnOffSrHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t mmdlGenPowOnOffSrRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Generic Power OnOff Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffSrInit(void);

/*************************************************************************************************/
/*!
 *  \brief  Execute the PowerUp procedure.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffOnPowerUp(void);

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Generic Power OnOff Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Generic Power OnOff Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffSrHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Generic Power OnOff Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffSrHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Publish a GenPowOnOff Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffSrPublish(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Get the Generic OnPowerUp state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffSrGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Set the Generic OnPowerUp state of the element.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] targetState  Target State for this transaction. See ::mmdlGenOnPowerUpState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffSrSetState(meshElementId_t elementId, mmdlGenOnPowerUpState_t targetState);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffSrRegister(mmdlEventCback_t recvCback);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_GEN_POWER_ONOFF_SR_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
