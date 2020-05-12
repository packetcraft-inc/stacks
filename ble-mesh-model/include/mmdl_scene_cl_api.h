/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Scenes Client Model API.
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
 * \addtogroup ModelSceneCl Scenes Client Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_SCENE_CL_API_H
#define MMDL_SCENE_CL_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mmdl_defs.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/
/*! \brief Model Scene Client Recall parameters structure */
typedef struct mmdlSceneRecallParam_tag
{
  uint16_t             sceneNum;        /*!< Scene Number. Value 0 is prohibited */
  uint8_t              tid;             /*!< Transaction Identifier */
  uint8_t              transitionTime;  /*!< Transition time */
  uint8_t              delay;           /*!< Delay in steps of 5 ms  */
} mmdlSceneRecallParam_t;

/*! \brief Scene Client Model Status event structure */
typedef struct mmdlSceneClStatusEvent_tag
{
  wsfMsgHdr_t          hdr;             /*!< WSF message header */
  meshElementId_t      elementId;       /*!< Element ID */
  meshAddress_t        serverAddr;      /*!< Server Address */
  mmdlSceneStatus_t    status;          /*!< Status code */
  uint16_t             currentScene;    /*!< Current scene */
  uint16_t             targetScene;     /*!< Target scene */
  uint8_t              remainingTime;   /*!< Remaining time until the transition completes */
} mmdlSceneClStatusEvent_t;

/*! \brief Scene Client Model Register Status event structure */
typedef struct mmdlSceneClRegStatusEvent_tag
{
  wsfMsgHdr_t          hdr;             /*!< WSF message header */
  meshElementId_t      elementId;       /*!< Element ID */
  meshAddress_t        serverAddr;      /*!< Server Address */
  mmdlSceneStatus_t    status;          /*!< Status code */
  uint16_t             currentScene;    /*!< Current scene */
  uint8_t              scenesCount;     /*!< Scenes Count */
  uint16_t             scenes[1];       /*!< Scenes Array */
} mmdlSceneClRegStatusEvent_t;

/*! \brief Scene Client Model event callback parameters structure */
typedef union mmdlSceneClEvent_tag
{
  wsfMsgHdr_t                 hdr;            /*!< WSF message header */
  mmdlSceneClStatusEvent_t    statusEvent;    /*!< State updated event. Used for
                                               *   ::MMDL_SCENE_CL_STATUS_EVENT.
                                               */
  mmdlSceneClRegStatusEvent_t regStatusEvent; /*!< Scene register updated event. Used for
                                               *   ::MMDL_SCENE_CL_REG_STATUS_EVENT.
                                               */
} mmdlSceneClEvent_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlSceneClHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t  mmdlSceneClRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Scene Client Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneClHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Scene Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneClHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Send a Scene Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                    uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Scene Register Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneClRegisterGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Scene Store message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] sceneNumber  Scene Number. Value 0x0000 is prohibited.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneClStore(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                      uint16_t appKeyIndex, mmdlSceneNumber_t sceneNumber);

/*************************************************************************************************/
/*!
 *  \brief     Send a Scene Store Unacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] sceneNumber  Scene Number. Value 0x0000 is prohibited.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneClStoreNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                           uint16_t appKeyIndex, mmdlSceneNumber_t sceneNumber);

/*************************************************************************************************/
/*!
 *  \brief     Send a Scene Recall message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneClRecall(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                       uint16_t appKeyIndex, const mmdlSceneRecallParam_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Send a Scene Recall Unacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneClRecallNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            uint16_t appKeyIndex, const mmdlSceneRecallParam_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Send a Scene Delete message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] sceneNumber  Scene Number. Value 0x0000 is prohibited.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneClDelete(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                      uint16_t appKeyIndex, mmdlSceneNumber_t sceneNumber);

/*************************************************************************************************/
/*!
 *  \brief     Send a Scene Delete Unacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] sceneNumber  Scene Number. Value 0x0000 is prohibited.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneClDeleteNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                           uint16_t appKeyIndex, mmdlSceneNumber_t sceneNumber);

/*************************************************************************************************/
/*!
 *  \brief     Install the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneClRegister(mmdlEventCback_t recvCback);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_SCENE_CL_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
