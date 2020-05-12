/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Generic Power On Off Client Model API.
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
 * \addtogroup ModelGenPowOnOffCl Generic Power OnOff Client Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_GEN_POWER_ONOFF_CL_API_H
#define MMDL_GEN_POWER_ONOFF_CL_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mmdl_defs.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Model Power OnOff Client Set parameters structure */
typedef struct mmdlGenPowOnOffSetParam_tag
{
  mmdlGenOnPowerUpState_t         state;         /*!< New OnPowerUp State */
} mmdlGenPowOnOffSetParam_t;

/*! \brief Generic Power OnOff Client Model Status event structure */
typedef struct mmdlGenPowOnOffClStatusEvent_tag
{
  wsfMsgHdr_t                     hdr;           /*!< WSF message header */
  meshElementId_t                 elementId;     /*!< Element ID */
  meshAddress_t                   serverAddr;    /*!< Server Address */
  mmdlGenOnPowerUpState_t         state;         /*!< Received published state */
  mmdlGenOnPowerUpState_t         targetState;   /*!< Received published target state */
} mmdlGenPowOnOffClStatusEvent_t;

/*! \brief Generic Power OnOff Client Model event callback parameters structure */
typedef union mmdlGenPowOnOffClEvent_tag
{
  wsfMsgHdr_t                     hdr;           /*!< WSF message header */
  mmdlGenPowOnOffClStatusEvent_t  statusEvent;   /*!< State updated event. Used for
                                                  *   ::MMDL_GEN_POWER_ONOFF_CL_STATUS_EVENT.
                                                  */
} mmdlGenPowOnOffClEvent_t;

/*************************************************************************************************/
/*!
 *  \brief     Model Generic Power OnOff Client received callback.
 *
 *  \param[in] pEvent  Pointer model event. See ::mmdlGenPowOnOffClEvent_t.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*mmdlGenPowOnOffClRecvCback_t)(const mmdlGenPowOnOffClEvent_t *pEvent);

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlGenPowOnOffClHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t  mmdlGenPowOnOffClRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Power OnOff Client Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffClHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Power OnOff Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffClHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Send a GenOnPowerUpGet message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffClGet(meshElementId_t elementId, meshAddress_t serverAddr,  uint8_t ttl,
                          uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a GenOnPowerUpSet message to the destination address.
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
void MmdlGenPowOnOffClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          const mmdlGenPowOnOffSetParam_t *pSetParam, uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a GenOnPowerUpSetUnacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                               const mmdlGenPowOnOffSetParam_t *pSetParam, uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Install the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffClRegister(mmdlEventCback_t recvCback);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_GEN_POWER_ONOFF_CL_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
