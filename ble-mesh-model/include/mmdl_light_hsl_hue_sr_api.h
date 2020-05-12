/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Light HSL Hue Server Model API.
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

#ifndef MMDL_LIGHT_HSL_HUE_API_H
#define MMDL_LIGHT_HSL_HUE_API_H

#include "wsf_timer.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Light HSL Hue Server Model State Update event structure */
typedef struct mmdlLightHslHueSrStateUpdate_tag
{
  wsfMsgHdr_t                  hdr;              /*!< WSF message header */
  meshElementId_t              elemId;           /*!< Element identifier */
  uint16_t                     state;            /*!< Updated state */
} mmdlLightHslHueSrStateUpdate_t;

/*! \brief Light HSL Hue Server Model event callback parameters structure */
typedef union mmdlLightHslHueSrEvent_tag
{
  wsfMsgHdr_t                     hdr;           /*!< WSF message header */
  mmdlLightHslHueSrStateUpdate_t  statusEvent;   /*!< State updated event. Used for
                                                  *   ::MMDL_LIGHT_HSL_HUE_SR_STATE_UPDATE_EVENT.
                                                  */
} mmdlLightHslHueSrEvent_t;

/*! \brief Light HSL Hue stored state definition */
typedef struct mmdlLightHslHuesStoredState_tag
{
  uint16_t                     presentHue;       /*!< Present state */
  uint16_t                     targetHue;        /*!< Target state */
}mmdlLightHslHueStoredState_t;

/*! \brief Model Light HSL Hue Server descriptor definition */
typedef struct mmdlLightHslHueSrDesc_tag
{
  mmdlLightHslHueStoredState_t *pStoredState;    /*!< Pointer to the structure that stores the Light
                                                  *   HSL state and scenes
                                                  */
  wsfTimer_t                   transitionTimer;  /*!< WSF Timer for delay and state transition */
  wsfTimer_t                   msgRcvdTimer;     /*!< Timer to manage received logically group messages */
  uint32_t                     remainingTimeMs;  /*!< Time remaining until the current state is
                                                  *   replaced with the target state. If set to 0,
                                                  *   the target state is ignored. Unit is 1 ms.
                                                  */
  int16_t                      transitionStep;   /*!< Transition state update step */
  uint16_t                     steps;            /*!< The number of transition steps */
  uint8_t                      delay5Ms;         /*!< Delay until the transition to the new state
                                                  *   begins. Unit is 5 ms.
                                                  */
  uint8_t                      transactionId;    /*!< Transaction Identifier used to logically group a
                                                  *   series of messages.
                                                  */
  meshAddress_t                srcAddr;          /*!< Source address of the logically grouped series of
                                                  *   messages.
                                                  */
  bool_t                       ackPending;       /*!< TRUE if an ACK is pending for the last received
                                                  *   message.
                                                  */
  bool_t                       ackForUnicast;    /*!< TRUE if the delayed message was received as a
                                                  *   unicast, FALSE otherwise.
                                                  */
  uint16_t                     ackAppKeyIndex;   /*!< AppKeyIndex used for the last received message. */
  meshElementId_t              mainElementId;    /*!< Element Id of the Main element that uses
                                                  *   this Hue instance.
                                                  */
  mmdlStateUpdateSrc_t         updateSource;     /*!< State update source. Cached for transitions.
                                                  */
} mmdlLightHslHueSrDesc_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlLightHslHueSrHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t mmdlLightHslHueSrRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Light HSL Hue Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlLightHslHueSrInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Light HSL Hue Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Light HSL Hue Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslHueSrHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Light HSL Hue Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslHueSrHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Publish a Light HSL Hue Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslHueSrPublish(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslHueSrRegister(mmdlEventCback_t recvCback);

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Light HSL Hue State and a Generic Level state.
 *
 *  \param[in] hueElemId  Element identifier where the Light HSL Hue state resides.
 *  \param[in] glvElemId  Element identifier where the Generic Level state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslHueSrBind2GenLevel(meshElementId_t hueElemId, meshElementId_t glvElemId);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_LIGHT_HSL_HUE_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
