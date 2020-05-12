/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Scene Server Model API.
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
 * \addtogroup ModelSceneSr Scenes Server Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_SCENE_SR_API_H
#define MMDL_SCENE_SR_API_H

#include "wsf_timer.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 Macros
**************************************************************************************************/

/*! \brief Number of stored states */
#define MMDL_SCENE_STATE_CNT             1

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Model Scene Server descriptor definition */
typedef struct mmdlSceneSrDesc_tag
{
  mmdlSceneNumber_t *pStoredScenes;   /*!< Pointer to the structure that stores
                                       *   current scene and scene register.
                                       *   Structure will store ::MMDL_NUM_OF_SCENES +
                                       *   MMDL_SCENE_STATE_CNT states.
                                       */
  wsfTimer_t        transitionTimer;  /*!< WSF Timer for delay and state transition */
  wsfTimer_t        msgRcvdTimer;     /*!< Timer to manage received logically group messages */
  uint32_t          remainingTimeMs;  /*!< Time remaining until the current state is
                                       *   replaced with the target state. If set to 0,
                                       *   the target state is ignored. Unit is 1 ms.
                                       */
  uint8_t           targetSceneIdx;   /*!< Target scene index in scene register */
  uint8_t           delay5Ms;         /*!< Delay until the transition to the new state
                                       *   begins. Unit is 5 ms.
                                       */
  uint8_t           transactionId;    /*!< Transaction Identifier used to logically group a
                                       *   series of messages.
                                       */
  meshAddress_t     srcAddr;          /*!< Source address of the logically grouped series of
                                       *   messages.
                                       */
  bool_t            ackPending;       /*!< TRUE if an ACK is pending for the last received
                                       *   message.
                                       */
  bool_t            ackForUnicast;    /*!< TRUE if the delayed message was received as a unicast,
                                       *   FALSE otherwise.
                                       */
  mmdlSceneStatus_t delayedStatus;    /*!< Status of delayed command.*/
  uint16_t          ackAppKeyIndex;   /*!< AppKeyIndex used for the last received message. */
} mmdlSceneSrDesc_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlSceneSrHandlerId;

/** \name Supported opcodes
 *
 */
/**@{*/
extern const meshMsgOpcode_t mmdlSceneSrRcvdOpcodes[];
extern const meshMsgOpcode_t mmdlSceneSetupSrRcvdOpcodes[];
/**@}*/

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Scenes Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlSceneSrInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Scenes Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Scenes Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneSrHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Scenes Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneSrHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Scenes Setup Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneSetupSrHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Publish a Scene Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneSrPublish(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Publish a Scene Register Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneSrPublishRegister(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneSrRegister(mmdlEventCback_t recvCback);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_SCENE_SR_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
