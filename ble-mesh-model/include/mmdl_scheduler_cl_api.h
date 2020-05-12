/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Scheduler Client Model API.
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
 * \addtogroup ModelSchedulerCl Scheduler Client Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_SCHEDULER_CL_API_H
#define MMDL_SCHEDULER_CL_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mmdl_defs.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Scheduler Client Model Status event structure */
typedef struct mmdlSchedulerClStatusEvent_tag
{
  wsfMsgHdr_t           hdr;              /*!< WSF message header */
  meshElementId_t       elementId;        /*!< Element ID */
  meshAddress_t         serverAddr;       /*!< Server Address */
  uint16_t              schedulesBf;      /*!< Each bit of the Schedules field set to 1 identifies
                                           *   a corresponding entry of the Schedule Register
                                           */
} mmdlSchedulerClStatusEvent_t;

/*! \brief Scheduler Client Model Status event structure */
typedef struct mmdlSchedulerClActionStatusEvent_tag
{
  wsfMsgHdr_t                   hdr;              /*!< WSF message header */
  meshElementId_t               elementId;        /*!< Element ID */
  meshAddress_t                 serverAddr;       /*!< Server Address */
  uint8_t                       index;            /*!< Selected Schedule Register entry */
  mmdlSchedulerRegisterEntry_t  scheduleRegister; /*!< Current values of the entry in the Schedule
                                                   *   Register state
                                                   */
} mmdlSchedulerClActionStatusEvent_t;

/*! \brief Scheduler Client Model event callback parameters structure */
typedef union mmdlSchedulerClEvent_tag
{
  wsfMsgHdr_t                         hdr;                /*!< WSF message header */
  mmdlSchedulerClStatusEvent_t        statusEvent;        /*!< Status event. Valid for
                                                           *   ::MMDL_SCHEDULER_CL_STATUS_EVENT
                                                           */
  mmdlSchedulerClActionStatusEvent_t  actionStatusEvent;  /*!< Action status event. Valid for
                                                           *   ::MMDL_SCHEDULER_CL_ACTION_STATUS_EVENT
                                                           */
} mmdlSchedulerClEvent_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlSchedulerClHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t  mmdlSchedulerClRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Scheduler Client Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerClHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Scheduler Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerClHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Send a Scheduler Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                        uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Send a Scheduler Action Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] index        Entry index of the Scheduler Register state.
 *                          Values 0x10-0xFF are Prohibited.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerClActionGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                              uint16_t appKeyIndex, uint8_t index);

/*************************************************************************************************/
/*!
 *  \brief     Send a Scheduler Action Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] index        Entry index of the Scheduler Register state.
 *                          Values 0x10-0xFF are Prohibited.
 *  \param[in] pParam       Pointer to Scheduler Register State entry parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerClActionSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                              uint16_t appKeyIndex, uint8_t index,
                              const mmdlSchedulerRegisterEntry_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Send a Scheduler Action Set Unacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] index        Entry index of the Scheduler Register state.
 *                          Values 0x10-0xFF are Prohibited.
 *  \param[in] pParam       Pointer to Scheduler Register State entry parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerClActionSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                   uint16_t appKeyIndex, uint8_t index,
                                   const mmdlSchedulerRegisterEntry_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Install the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerClRegister(mmdlEventCback_t recvCback);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_SCHEDULER_CL_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
