/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Generic Battery Client Model API.
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
 * \addtogroup ModelGenBattery Generic Battery Client Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_GEN_BATTERY_CL_API_H
#define MMDL_GEN_BATTERY_CL_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mmdl_defs.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Generic Battery Client Model Status event structure */
typedef struct mmdlGenBatteryClStatusEvent_tag
{
  wsfMsgHdr_t          hdr;             /*!< WSF message header */
  meshElementId_t      elementId;       /*!< Element ID */
  meshAddress_t        serverAddr;      /*!< Server Address */
  uint8_t              state;           /*!< Received published state */
  uint32_t             timeToDischarge; /*!< Received published time to discharge state */
  uint32_t             timeToCharge;    /*!< Received published time to charge state */
  uint8_t              flags;           /*!< Received published flag state */
} mmdlGenBatteryClStatusEvent_t;

/*! \brief Generic Battery Client Model event callback parameters structure */
typedef union mmdlGenBatteryClEvent_tag
{
  wsfMsgHdr_t                   hdr;         /*!< WSF message header */
  mmdlGenBatteryClStatusEvent_t statusEvent; /*!< State updated event. Used for
                                              *   ::MMDL_GEN_BATTERY_CL_STATUS_EVENT.
                                              */
} mmdlGenBatteryClEvent_t;

/*************************************************************************************************/
/*!
 *  \brief     Model Battery Level Client received callback.
 *
 *  \param[in] pEvent  Pointer model event. See ::mmdlGenBatteryClEvent_t.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*mmdlGenBatteryClRecvCback_t)(const wsfMsgHdr_t *pEvent);

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlGenBatteryClHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t  mmdlGenBatteryClRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Battery Client Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenBatteryClHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Battery Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenBatteryClHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Send a GenBatteryGet message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenBatteryClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                         uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Install the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenBatteryClRegister(mmdlEventCback_t recvCback);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_GEN_BATTERY_CL_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
