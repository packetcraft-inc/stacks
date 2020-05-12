/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Vendor Test Client Model API.
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
 * \addtogroup ModelVendorTestCl Vendor Test Client Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_VENDOR_TEST_CL_API_H
#define MMDL_VENDOR_TEST_CL_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mmdl_defs.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Vendor Test Client Model Event Status */
enum mmdlVendorTestClEventStatus
{
  MMDL_VENDOR_TEST_CL_SUCCESS         = 0x00, /*!< Success event status */
};

/*! \brief Vendor Test Client Model Event Type Enum */
enum vendorTestClEventType
{
  MMDL_VENDOR_TEST_CL_STATUS_EVENT    = 0x00, /*!< Vendor Test Status Event */
};

/*! \brief Vendor Test Client Model Status event structure */
typedef struct mmdlVendorTestClStatusEvent_tag
{
  wsfMsgHdr_t          hdr;               /*!< WSF message header */
  meshElementId_t      elementId;         /*!< Element ID */
  meshAddress_t        serverAddr;        /*!< Server Address */
  uint8_t              ttl;               /*!< TTL of the received message. */
  uint8_t              *pMsgParams;       /*!< Received published state */
  uint16_t             messageParamsLen;  /*!< Message parameters data length */
} mmdlVendorTestClStatusEvent_t;

/*! \brief Vendor Test Client Model event callback parameters structure */
typedef union mmdlVendorTestClEvent_tag
{
  wsfMsgHdr_t                   hdr;         /*!< WSF message header */
  mmdlVendorTestClStatusEvent_t statusEvent; /*!< State updated event. Used for
                                              *   ::MMDL_VENDOR_TEST_CL_STATUS_EVENT.
                                              */
} mmdlVendorTestClEvent_t;

/*************************************************************************************************/
/*!
 *  \brief     Model Vendor Test Client received callback.
 *
 *  \param[in] pEvent  Pointer model event. See ::mmdlVendorTestClEvent_t.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*mmdlVendorTestClRecvCback_t)(const mmdlVendorTestClEvent_t *pEvent);

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlVendorTestClHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t  mmdlVendorTestClRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for On Off Client Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlVendorTestClHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for On Off Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlVendorTestClHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Install the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlVendorTestClRegister(mmdlVendorTestClRecvCback_t recvCback);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_VENDOR_TEST_CL_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
