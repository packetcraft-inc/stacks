/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Generic Level Client Model API.
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
 * \addtogroup ModelGenLevelCl Generic Level Client Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_GEN_LEVEL_CL_API_H
#define MMDL_GEN_LEVEL_CL_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mmdl_defs.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/
/*! \brief Model Level Client Set parameters structure */
typedef struct mmdlGenLevelSetParam_tag
{
  mmdlGenLevelState_t state;          /*!< New Level State */
  uint8_t             tid;            /*!< Transaction Identifier */
  uint8_t             transitionTime; /*!< Transition time */
  uint8_t             delay;          /*!< Delay in steps of 5 ms  */
} mmdlGenLevelSetParam_t;

/*! \brief Model Level Client Delta Set parameters structure */
typedef struct mmdlGenDeltaSetParam_tag
{
  mmdlGenDelta_t      delta;          /*!< Delta change */
  uint8_t             tid;            /*!< Transaction Identifier */
  uint8_t             transitionTime; /*!< Transition time */
  uint8_t             delay;          /*!< Delay in steps of 5 ms  */
} mmdlGenDeltaSetParam_t;

/*! \brief Generic Level Client Model Status event structure */
typedef struct mmdlGenLevelClStatusEvent_tag
{
  wsfMsgHdr_t          hdr;           /*!< WSF message header */
  meshElementId_t      elementId;     /*!< Element ID */
  meshAddress_t        serverAddr;    /*!< Server Address */
  mmdlGenLevelState_t  state;         /*!< Received published state */
  mmdlGenLevelState_t  targetState;   /*!< Received published target state */
  uint8_t              remainingTime; /*!< Remaining time until the transition completes */
} mmdlGenLevelClStatusEvent_t;

/*! \brief Generic Level Client Model event callback parameters structure */
typedef union mmdlGenLevelClEvent_tag
{
  wsfMsgHdr_t                 hdr;          /*!< WSF message header */
  mmdlGenLevelClStatusEvent_t statusEvent;  /*!< State updated event. Used for
                                             *   ::MMDL_GEN_LEVEL_CL_STATUS_EVENT.
                                             */
} mmdlGenLevelClEvent_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlGenLevelClHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t  mmdlGenLevelClRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Level Client Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelClHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Level Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelClHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Send a GenLevelGet message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelClGet(meshElementId_t elementId, meshAddress_t serverAddr,  uint8_t ttl,
                       uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a GenLevelSet message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                       const mmdlGenLevelSetParam_t *pSetParam, uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a GenLevelSetUnacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR
 *  \param[in] ttl          TTL value as defined by the specification
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            const mmdlGenLevelSetParam_t *pSetParam, uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
*  \brief     Send a GenDeltaSet message to the destination address.
*
*  \param[in] elementId    Identifier of the Element implementing the model.
*  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
*  \param[in] ttl          TTL value as defined by the specification.
*  \param[in] pSetParam    Pointer to structure containing the set parameters.
*  \param[in] appKeyIndex  Application Key Index.
*
*  \return    None.
*/
/*************************************************************************************************/
void MmdlGenDeltaClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                       const mmdlGenDeltaSetParam_t *pSetParam, uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
*  \brief     Send a GenDeltaSetUnacknowledged message to the destination address.
*
*  \param[in] elementId    Identifier of the Element implementing the model
*  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR
*  \param[in] ttl          TTL value as defined by the specification
*  \param[in] pSetParam    Pointer to structure containing the set parameters.
*  \param[in] appKeyIndex  Application Key Index.
*
*  \return    None.
*/
/*************************************************************************************************/
void MmdlGenDeltaClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            const mmdlGenDeltaSetParam_t *pSetParam, uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
*  \brief     Send a GenMoveSet message to the destination address.
*
*  \param[in] elementId    Identifier of the Element implementing the model.
*  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
*  \param[in] ttl          TTL value as defined by the specification.
*  \param[in] pSetParam    Pointer to structure containing the set parameters.
*  \param[in] appKeyIndex  Application Key Index.
*
*  \return    None.
*/
/*************************************************************************************************/
void MmdlGenMoveClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                      const mmdlGenLevelSetParam_t *pSetParam, uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
*  \brief     Send a GenMoveSetUnacknowledged message to the destination address.
*
*  \param[in] elementId    Identifier of the Element implementing the model
*  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR
*  \param[in] ttl          TTL value as defined by the specification
*  \param[in] pSetParam    Pointer to structure containing the set parameters.
*  \param[in] appKeyIndex  Application Key Index.
*
*  \return    None.
*/
/*************************************************************************************************/
void MmdlGenMoveClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                           const mmdlGenLevelSetParam_t *pSetParam, uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Install the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelClRegister(mmdlEventCback_t recvCback);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_GEN_LEVEL_CL_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
