/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Light CTL Server Model API.
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
 * \addtogroup ModelLightHst Light CTL Server Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_LIGHT_CTL_API_H
#define MMDL_LIGHT_CTL_API_H

#include "wsf_timer.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "mmdl_defs.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Union of CTL states */
typedef union mmdlLightCtlStates_tag
{
  mmdlLightCtlState_t      state;              /*!< State */
  mmdlLightCtlRangeState_t rangeState;         /*!< Range state */
} mmdlLightCtlStates_t;

/*! \brief Light CTL Server Model State Update event structure */
typedef struct mmdlLightCtlSrStateUpdate_tag
{
  wsfMsgHdr_t                  hdr;             /*!< WSF message header */
  meshElementId_t              elemId;          /*!< Element identifier */
  mmdlLightCtlStates_t         ctlStates;       /*!< Updated states */
} mmdlLightCtlSrStateUpdate_t;

/*! \brief Light CTL Server Model event callback parameters structure */
typedef union mmdlLightCtlSrEvent_tag
{
  wsfMsgHdr_t                  hdr;             /*!< WSF message header */
  mmdlLightCtlSrStateUpdate_t  statusEvent;     /*!< State updated event. Used for
                                                 *   ::MMDL_LIGHT_CTL_SR_STATE_UPDATE_EVENT.
                                                 */
} mmdlLightCtlSrEvent_t;

/*! \brief Light CTL stored state definition */
typedef struct mmdlLightCtlSrStoredState_tag
{
  mmdlLightCtlState_t         present;                        /*!< Present state */
  mmdlLightCtlState_t         target;                         /*!< Target state */
  mmdlLightCtlState_t         ctlScenes[MMDL_NUM_OF_SCENES];  /*!< Temperature Scenes */
  uint16_t                    minTemperature;                 /*!< Minimum Temperature value */
  uint16_t                    maxTemperature;                 /*!< Maximum Temperature value */
  uint16_t                    defaultTemperature;             /*!< Default Temperature value */
  uint16_t                    defaultDeltaUV;                 /*!< Default Delta UV value */
}mmdlLightCtlSrStoredState_t;

/*! \brief Light CTL transition step definition */
typedef struct mmdlLightCtlTransStep_tag
{
  int16_t  ltness;       /*!< Lightness step value */
  int16_t  temperature;  /*!< Temperature step value */
  int16_t  deltaUV;      /*!< Delta UV step value */
} mmdlLightCtlTransStep_t;

/*! \brief Model Light CTL Server descriptor definition */
typedef struct mmdlLightCtlSrDesc_tag
{
  mmdlLightCtlSrStoredState_t *pStoredState;    /*!< Pointer to the structure that stores the Light
                                                 *   CTL state and scenes
                                                 */
  mmdlNvmSaveHandler_t        fNvmSaveStates;   /*!< Pointer to function that saves
                                                 *   Model instance states in NVM
                                                 */
  wsfTimer_t                  transitionTimer;  /*!< WSF Timer for delay and state transition */
  wsfTimer_t                  msgRcvdTimer;     /*!< Timer to manage received logically group messages */
  uint32_t                    remainingTimeMs;  /*!< Time remaining until the current state is
                                                 *   replaced with the target state. If set to 0,
                                                 *   the target state is ignored. Unit is 1 ms.
                                                 */
  mmdlLightCtlTransStep_t     transitionStep;   /*!< Transition state update step */
  uint16_t                    steps;            /*!< The number of transition steps */
  uint8_t                     delay5Ms;         /*!< Delay until the transition to the new state
                                                 *   begins. Unit is 5 ms.
                                                 */
  uint8_t                     transactionId;    /*!< Transaction Identifier used to logically group a
                                                 *   series of messages.
                                                 */
  meshAddress_t               srcAddr;          /*!< Source address of the logically grouped series of
                                                 *   messages.
                                                 */
  bool_t                      ackPending;       /*!< TRUE if an ACK is pending for the last received
                                                 *   message.
                                                 */
  bool_t                      ackForUnicast;    /*!< TRUE if the delayed message was received as a
                                                 *   unicast, FALSE otherwise.
                                                 */
  uint16_t                    ackAppKeyIndex;   /*!< AppKeyIndex used for the last received message. */
  mmdlStateUpdateSrc_t        updateSource;     /*!< State update source. Cached for transitions.
                                                 */
  meshElementId_t             mainElementId;    /*!< Element Id of the Main element */
  meshElementId_t             tempElementId;    /*!< Element Id of the Temperature element */
  meshElementId_t             deltaUVElementId; /*!< Element Id of the DeltaUVuration element */
} mmdlLightCtlSrDesc_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlLightCtlSrHandlerId;

/** \name Supported opcodes
 *
 */
/**@{*/
extern const meshMsgOpcode_t mmdlLightCtlSrRcvdOpcodes[];
extern const meshMsgOpcode_t mmdlLightCtlSetupSrRcvdOpcodes[];
/**@}*/

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Light CTL Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Light CTL Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Light CTL Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Light CTL Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Light CTL Setup Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSetupSrHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Publish a Light CTL Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrPublish(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrRegister(mmdlEventCback_t recvCback);

/*************************************************************************************************/
/*!
 *  \brief     Links the Main Element to the DeltaUV and Temperature elements.
 *
 *  \param[in] mainElementId  Identifier of the Main element
 *  \param[in] tempElementId  Identifier of the Temperature element
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrLinkElements(meshElementId_t mainElementId, meshElementId_t tempElementId);

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Light CTL State and a Generic OnPowerUp state.
 *
 *  \param[in] onPowerUpElemId  Element identifier where the OnPowerUp state resides.
 *  \param[in] ctlElemId        Element identifier where the Light CTL state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrBind2OnPowerUp(meshElementId_t onPowerUpElemId, meshElementId_t ctlElemId);

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Light Lightness Actual State and a Light CTL state.
 *             A bind between the Generic OnOff and Light CTL and Generic Level and Light CTL is
 *             created to support the lightness extension.
 *
 *  \param[in] ltElemId   Element identifier where the Light Lightness Actual state resides.
 *  \param[in] ctlElemId  Element identifier where the Light CTL state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrBind2LtLtnessAct(meshElementId_t ltElemId, meshElementId_t ctlElemId);

/*************************************************************************************************/
/*!
 *  \brief     Get the local Light CTL state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Set the local Light CTL state.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] pTargetState Target State for this transaction. See ::mmdlLightCtlState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrSetState(meshElementId_t elementId, mmdlLightCtlState_t *pTargetState);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_LIGHT_CTL_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
