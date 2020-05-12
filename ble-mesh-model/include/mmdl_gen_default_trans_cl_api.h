/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Generic Default Transition Client Model API.
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
 * \addtogroup ModelGenTransitionCl Generic Default Transition Client Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_GEN_DEFAULT_TRANS_CL_API_H
#define MMDL_GEN_DEFAULT_TRANS_CL_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mmdl_defs.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Model Default Transition Set parameters structure */
typedef struct mmdlGenDefaultTransSetParam_tag
{
  mmdlGenDefaultTransState_t state;   /*!< New Default Transition State */
} mmdlGenDefaultTransSetParam_t;

/*! \brief Generic Default Transition Client Model Status event structure */
typedef struct mmdlGenDefaultTransClStatusEvent_tag
{
  wsfMsgHdr_t                 hdr;           /*!< WSF message header */
  meshElementId_t             elementId;     /*!< Element ID */
  meshAddress_t               serverAddr;    /*!< Server Address */
  mmdlGenDefaultTransState_t  state;         /*!< Received published state */
} mmdlGenDefaultTransClStatusEvent_t;

/*! \brief Generic Default Transition Client Model event callback parameters structure */
typedef union mmdlGenDefaultTransClEvent_tag
{
  wsfMsgHdr_t                        hdr;         /*!< WSF message header */
  mmdlGenDefaultTransClStatusEvent_t statusEvent; /*!< State updated event. Used for
                                                   *   ::MMDL_GEN_DEFAULT_TRANS_CL_STATUS_EVENT.
                                                   */
} mmdlGenDefaultTransClEvent_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlGenDefaultTransClHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t  mmdlGenDefaultTransClRcvdOpcodes[];

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
void MmdlGenDefaultTransClHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Default Transition Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenDefaultTransClHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Send a GenDefaultTransGet message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenDefaultTransClGet(meshElementId_t elementId, meshAddress_t serverAddr,  uint8_t ttl,
                              uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a GenDefaultTransSet message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenDefaultTransClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                              const mmdlGenDefaultTransSetParam_t *pSetParam, uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a GenDefaultTransSetUnacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] serverAddr   Element address of the server
 *  \param[in] ttl          TTL value as defined by the specification
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenDefaultTransClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                   const mmdlGenDefaultTransSetParam_t *pSetParam, uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Install the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenDefaultTransClRegister(mmdlEventCback_t recvCback);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_GEN_DEFAULT_TRANS_CL_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
