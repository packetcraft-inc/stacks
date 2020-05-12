/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Light HSL Server Model API.
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
 * \addtogroup ModelLightHst Light HSL Server Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_LIGHT_HSL_API_H
#define MMDL_LIGHT_HSL_API_H

#include "wsf_timer.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Union of HSL states */
typedef union mmdlLightHslStates_tag
{
  mmdlLightHslState_t      state;              /*!< State */
  mmdlLightHslRangeState_t rangeState;         /*!< Range state */
} mmdlLightHslStates_t;

/*! \brief Light HSL Server Model State Update event structure */
typedef struct mmdlLightHslSrStateUpdate_tag
{
  wsfMsgHdr_t                  hdr;             /*!< WSF message header */
  meshElementId_t              elemId;          /*!< Element identifier */
  mmdlLightHslStates_t         hslStates;       /*!< Updated states */
} mmdlLightHslSrStateUpdate_t;

/*! \brief Light HSL Server Model event callback parameters structure */
typedef union mmdlLightHslSrEvent_tag
{
  wsfMsgHdr_t                  hdr;             /*!< WSF message header */
  mmdlLightHslSrStateUpdate_t  statusEvent;     /*!< State updated event. Used for
                                                 *   ::MMDL_LIGHT_HSL_SR_STATE_UPDATE_EVENT.
                                                 */
} mmdlLightHslSrEvent_t;

/*! \brief Light HSL stored state definition */
typedef struct mmdlLightHslSrStoredState_tag
{
  mmdlLightHslState_t         present;                        /*!< Present state */
  mmdlLightHslState_t         target;                         /*!< Target state */
  mmdlLightHslState_t         hslScenes[MMDL_NUM_OF_SCENES];  /*!< Hue Scenes */
  uint16_t                    defaultHue;                     /*!< Default Hue value */
  uint16_t                    minHue;                         /*!< Minimum Hue value */
  uint16_t                    maxHue;                         /*!< Maximum Hue value */
  uint16_t                    defaultSat;                     /*!< Default Saturation value */
  uint16_t                    minSat;                         /*!< Minimum Saturation value */
  uint16_t                    maxSat;                         /*!< Maximum Saturation value */
}mmdlLightHslSrStoredState_t;

/*! \brief Light HSL transition step definition */
typedef struct mmdlLightHslTransStep_tag
{
  int16_t  ltness;       /*!< Lightness step value */
  int16_t  hue;          /*!< Hue step value */
  int16_t  saturation;   /*!< Saturation step value */
} mmdlLightHslTransStep_t;

/*! \brief Model Light HSL Server descriptor definition */
typedef struct mmdlLightHslSrDesc_tag
{
  mmdlLightHslSrStoredState_t *pStoredState;    /*!< Pointer to the structure that stores the Light
                                                 *   HSL state and scenes
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
  mmdlLightHslTransStep_t     transitionStep;   /*!< Transition state update step */
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
  bool_t                      ackForUnicast;    /*!< TRUE if the delayed message was received as a unicast,
                                                 *   FALSE otherwise.
                                                 */
  uint16_t                    ackAppKeyIndex;   /*!< AppKeyIndex used for the last received message. */
  mmdlStateUpdateSrc_t        updateSource;     /*!< State update source. Cached for transitions.
                                                 */
  meshElementId_t             hueElementId;     /*!< Element Id of the Hue element */
  meshElementId_t             satElementId;     /*!< Element Id of the Saturation element */
} mmdlLightHslSrDesc_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlLightHslSrHandlerId;

/** \name Supported opcodes
 *
 */
/**@{*/
extern const meshMsgOpcode_t mmdlLightHslSrRcvdOpcodes[];
extern const meshMsgOpcode_t mmdlLightHslSetupSrRcvdOpcodes[];
/**@}*/

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Light HSL Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlLightHslSrInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Light HSL Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Light HSL Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Light HSL Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Light HSL Setup Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSetupSrHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Publish a Light HSL Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrPublish(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Publish a Light HSL Target Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrPublishTarget(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrRegister(mmdlEventCback_t recvCback);

/*************************************************************************************************/
/*!
 *  \brief     Links the Main Element to the Sat and Hue elements.
 *
 *  \param[in] mainElementId  Identifier of the Main element
 *  \param[in] hueElementId   Identifier of the Hue element
 *  \param[in] satElementId   Identifier of the Sat element
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrLinkElements(meshElementId_t mainElementId, meshElementId_t hueElementId,
                                meshElementId_t satElementId);

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Light HSL State and a Generic OnPowerUp state.
 *
 *  \param[in] onPowerUpElemId  Element identifier where the OnPowerUp state resides.
 *  \param[in] hslElemId        Element identifier where the Light HSL state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrBind2OnPowerUp(meshElementId_t onPowerUpElemId, meshElementId_t hslElemId);

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Light Lightness Actual State and a Light HSL state.
 *             A bind between the Generic OnOff and Light HSL and Generic Level and Light HSL is
 *             created to support the lightness extension.
 *
 *  \param[in] ltElemId   Element identifier where the Light Lightness Actual state resides.
 *  \param[in] hslElemId  Element identifier where the Light HSL state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrBind2LtLtnessAct(meshElementId_t ltElemId, meshElementId_t hslElemId);

/*************************************************************************************************/
/*!
 *  \brief     Get the local Light HSL state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Set the local Light HSL state.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] pTargetState Target State for this transaction. See ::mmdlLightHslState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrSetState(meshElementId_t elementId,
                            mmdlLightHslState_t *pTargetState);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_LIGHT_HSL_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
