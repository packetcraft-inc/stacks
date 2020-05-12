/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Light Lightness Server Model API.
 *
 *  Copyright (c) 2010-2019 Arm Ltd. All Rights Reserved.
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
 * \addtogroup ModelLightLightnessSr Light Lightness Server Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_LIGHT_LIGHTNESS_SR_API_H
#define MMDL_LIGHT_LIGHTNESS_SR_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mmdl_lightlightness_defs.h"

/**************************************************************************************************
 Macros
**************************************************************************************************/

/*! \brief Number of stored states (Actual + Linear + Target + Last + Default + RangeMin + RangeMax) */
#define MMDL_LIGHT_LIGHTNESS_STATE_CNT             7

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Union of Light Lightness states */
typedef union
{
  mmdlLightLightnessState_t       state;              /*!< State */
  mmdlLightLightnessRangeState_t  rangeState;         /*!< Range state */
} mmdlLightLightnessStates_t;

/*! \brief Light Lightness Server Model State Update event structure */
typedef struct mmdlLightLightnessSrStateUpdate_tag
{
  wsfMsgHdr_t                       hdr;                /*!< WSF message header */
  meshElementId_t                   elemId;             /*!< Element identifier */
  mmdlStateUpdateSrc_t              stateUpdateSource;  /*!< Updated state source */
  mmdlLightLightnessStates_t        lightnessState;     /*!< Updated state union */
} mmdlLightLightnessSrStateUpdate_t;

/*! \brief Light Lightness Server Model Current State event structure */
typedef struct mmdlLightLightnessSrCurrentState_tag
{
  wsfMsgHdr_t                       hdr;                /*!< WSF message header */
  meshElementId_t                   elemId;             /*!< Element identifier */
  mmdlLightLightnessStates_t        lightnessState;     /*!< Current state union */
} mmdlLightLightnessSrCurrentState_t;

/*! \brief Light Lightness Server Model event callback parameters structure */
typedef union mmdlLightLightnessSrEvent_tag
{
  wsfMsgHdr_t                        hdr;               /*!< WSF message header */
  mmdlLightLightnessSrStateUpdate_t  statusEvent;       /*!< State updated event. Used for
                                                         *   ::MMDL_LIGHT_LIGHTNESS_SR_STATE_UPDATE_EVENT.
                                                         */
  mmdlLightLightnessSrCurrentState_t currentStateEvent; /*!< Current state event. Sent after a Get request
                                                         *   from the upper layer. Used for
                                                         *   ::MMDL_LIGHT_LIGHTNESS_SR_CURRENT_STATE_EVENT.
                                                         */
} mmdlLightLightnessSrEvent_t;

/*! \brief Light Lightness Server descriptor definition */
typedef struct mmdlLightLightnessSrDesc_tag
{
  mmdlLightLightnessState_t *pStoredStates;   /*!< Pointer to the structure that stores
                                               *   current state and scene data. First
                                               *   value is always the current one.
                                               *   Second value is the target state.
                                               *   Sequential values represent scene
                                               *   values starting with scene index 0 and
                                               *   ending with index :MMDL_NUM_OF_SCENES - 1.
                                               *   Structure will store :MMDL_NUM_OF_SCENES + 2
                                               *   states.
                                               */
  mmdlNvmSaveHandler_t      fNvmSaveStates;   /*!< Pointer to function that saves
                                               *   Model instance states in NVM
                                               */
  wsfTimer_t                transitionTimer;  /*!< WSF Timer for delay and state transition */
  wsfTimer_t                msgRcvdTimer;     /*!< Timer to manage received logically group
                                               *   messages.
                                               */
  uint32_t                  remainingTimeMs;  /*!< Time remaining until the current state is
                                               *   replaced with the target state. If set to 0,
                                               *   the target state is ignored. Unit is 1 ms.
                                               */
  int16_t                   transitionStep;   /*!< Transition state update step */
  uint16_t                  steps;            /*!< The number of transition steps */
  uint8_t                   delay5Ms;         /*!< Delay until the transition to the new state
                                               *   begins. Unit is 5 ms.
                                               */
  uint8_t                   transactionId;    /*!< Transaction Identifier used to logically group a
                                               *   series of messages.
                                               */
  meshAddress_t             srcAddr;          /*!< Source address of the logically grouped series
                                               *   of messages.
                                               */
  bool_t                    ackPending;       /*!< TRUE if an ACK is pending for the last received
                                               *   message.
                                               */
  bool_t                    ackForUnicast;    /*!< TRUE if the last message was received as a
                                               *   unicast, FALSE otherwise.
                                               */
  uint16_t                  ackAppKeyIndex;   /*!< AppKeyIndex used for the last received message. */
  uint8_t                   transitionType;   /*!< Transition type - Actual or Linear */
  mmdlStateUpdateSrc_t      updateSource;     /*!< State update source. Cached for transitions.
                                               */
} mmdlLightLightnessSrDesc_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlLightLightnessSrHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t mmdlLightLightnessSrRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Light Lightness Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Light Lightness Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Light Lightness Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Light Lightness Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Publish a Light Lightness Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrPublish(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Publish a Light Lightness Linear Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrPublishLinear(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Get the local Light Lightness Actual state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Set the local Light Lightness Actual state.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] targetState  Target State for this transaction. See ::mmdlLightLightnessState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrSetState(meshElementId_t elementId,
                                  mmdlLightLightnessState_t targetState);

/*************************************************************************************************/
/*!
 *  \brief     Get the local Light Lightness Linear state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessLinearSrGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Set the Light Lightness Linear state.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] targetState  Target State for this transaction. See ::mmdlLightLightnessState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessLinearSrSetState(meshElementId_t elementId,
                                        mmdlLightLightnessState_t targetState);

/*************************************************************************************************/
/*!
 *  \brief     Get the Light Lightness Last state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessLastSrGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Get the local Light Lightness Default state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessDefaultSrGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Get the Light Lightness Range state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessRangeSrGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrRegister(mmdlEventCback_t recvCback);

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Light Lightness Actual State and a Generic OnPowerUp state.
 *
 *  \param[in] ltElemId         Element identifier where the Light Lightness Actual state resides.
 *  \param[in] onPowerUpElemId  Element identifier where the OnPowerUp state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrBind2OnPowerUp(meshElementId_t onPowerUpElemId, meshElementId_t ltElemId);

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Light Lightness Actual State and a Generic Level state.
 *
 *  \param[in] ltElemId   Element identifier where the Light Lightness Actual state resides.
 *  \param[in] glvElemId  Element identifier where the Generic Level state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrBind2GenLevel(meshElementId_t ltElemId, meshElementId_t glvElemId);

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Light Lightness Actual State and a  Generic On Off state.
 *
 *  \param[in] ltElemId     Element identifier where the Light Lightness Actual state resides.
 *  \param[in] onoffElemId  Element identifier where the On Off state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrBind2OnOff(meshElementId_t ltElemId, meshElementId_t onoffElemId);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_LIGHT_LIGHTNESS_SR_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
