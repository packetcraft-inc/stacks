/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Health Client Model API.
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
 *
 */
/*************************************************************************************************/

/*! ***********************************************************************************************
 * \addtogroup MESH_HEALTH_ClIENT Mesh Health Client Model API
 * @{
 *************************************************************************************************/

#ifndef MESH_HT_CL_API_H
#define MESH_HT_CL_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mesh_ht_mdl_api.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief Number of supported opcodes for receiving Health Messages */
#define MESH_HT_CL_NUM_RECVD_OPCODES  4

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Mesh Health Client Status values */
enum meshHtClStatusValues
{
  MESH_HT_CL_SUCCESS         = 0x00,  /*!< Procedure returned with success */
};

/*! \brief Mesh Health Client Health Fault Status event parameter */
typedef struct meshHtClFaultStatus_tag
{
  meshHtMdlTestId_t   testId;           /*!< Test identifier (8 bit) */
  uint16_t            companyId;        /*!< Company identifier 16-bit */
  meshHtFaultId_t     *pFaultIdArray;   /*!< Pointer to a fault identifier array */
  uint8_t             faultIdArrayLen;  /*!< Length of the array referenced by pFaultIdArray */
} meshHtClFaultStatus_t;

/*! \brief Data structure for ::MESH_HT_CL_ATTENTION_STATUS_EVENT */
typedef struct meshHtClAttentionStatusEvt_tag
{
  wsfMsgHdr_t          hdr;           /*!< Header */
  meshElementId_t      elemId;        /*!< Current element Identifier */
  meshAddress_t        htSrElemAddr;  /*!< Address of the remote element containing
                                       *   an instance of Health Server
                                       */
  meshHtAttTimer_t     attTimerState; /*!< Attention Timer state. */
} meshHtClAttentionStatusEvt_t;

/*! \brief Data structure for ::MESH_HT_CL_PERIOD_STATUS_EVENT */
typedef struct meshHtClPeriodStatusEvt_tag
{
  wsfMsgHdr_t          hdr;           /*!< Header */
  meshElementId_t      elemId;        /*!< Current element Identifier */
  meshAddress_t        htSrElemAddr;  /*!< Address of the remote element containing
                                       *   an instance of Health Server
                                       */
  meshHtPeriod_t       periodDivisor; /*!< Fast Period Divisor */
} meshHtClPeriodStatusEvt_t;

/*! \brief Data structure for ::MESH_HT_CL_CURRENT_STATUS_EVENT and ::MESH_HT_CL_FAULT_STATUS_EVENT */
typedef struct meshHtClFaultStatusEvt_tag
{
  wsfMsgHdr_t             hdr;           /*!< Header */
  meshElementId_t         elemId;        /*!< Current element Identifier */
  meshAddress_t           htSrElemAddr;  /*!< Address of the remote element containing
                                          *   an instance of Health Server
                                          */
  meshHtClFaultStatus_t   healthStatus;  /*!< Current health Status */
} meshHtClFaultStatusEvt_t;

/*! \brief Union of all Health Client model events */
typedef union meshHtClEvt_tag
{
  wsfMsgHdr_t                   hdr;                 /*!< Common header */
  meshHtClFaultStatusEvt_t      currentStatusEvt;    /*!< Current status event. Used for
                                                      *   ::MESH_HT_CL_CURRENT_STATUS_EVENT.
                                                      */
  meshHtClFaultStatusEvt_t      faultStatusEvt;      /*!< Fault status event.  Used for
                                                      *   ::MESH_HT_CL_FAULT_STATUS_EVENT.
                                                      */
  meshHtClPeriodStatusEvt_t     periodStatusEvt;     /*!< Period status event.  Used for
                                                      *   ::MESH_HT_CL_PERIOD_STATUS_EVENT.
                                                      */
  meshHtClAttentionStatusEvt_t  attentionStatusEvt;  /*!< Attention status event.  Used for
                                                      *   ::MESH_HT_CL_ATTENTION_STATUS_EVENT.
                                                      */
} meshHtClEvt_t;

/*************************************************************************************************/
/*!
 *  \brief     Procedure callback triggered after a Health Client model procedure is completed or
 *             an unrequested Health Current Status message is received by a Health Server model.
 *
 *  \param[in] pEvt  Pointer to structure containing the Health Client Model Event.
 *
 *  \return    None.
 *
 *  \see meshHtClEvent_t
 */
/*************************************************************************************************/
typedef void (*meshHtClCback_t)(const meshHtClEvt_t *pEvt);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! \brief WSF handler ID for Health Client */
extern wsfHandlerId_t meshHtClHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t meshHtClRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Health Client Handler.
 *
 *  \param[in] handlerId  WSF handler ID of the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtClHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Health Client model.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshHtClInit(void);

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Health Client model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtClHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback that is triggered when a message is received for this model.
 *
 *  \param[in] evtCback  Callback registered by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtClRegister(mmdlEventCback_t evtCback);

/*************************************************************************************************/
/*!
 *  \brief     Gets the current Registered Fault state identified by the company ID.
 *
 *  \param[in] elementId     Local element identifier for a Health Client model instance.
 *  \param[in] htSrElemAddr  Address of the element containing an instance of Health Server model.
 *  \param[in] appKeyIndex   AppKey Index.
 *  \param[in] ttl           TTL or ::MESH_USE_DEFAULT_TTL.
 *  \param[in] companyId     16-bit Bluetooth assigned Company Identifier.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtClFaultGet(meshElementId_t elementId, meshAddress_t htSrElemAddr, uint16_t appKeyIndex,
                      uint8_t ttl, uint16_t companyId);

/*************************************************************************************************/
/*!
 *  \brief     Clears the current Registered Fault state identified by the company ID.
 *
 *  \param[in] elementId     Local element identifier for a Health Client model instance.
 *  \param[in] htSrElemAddr  Address of the element containing an instance of Health Server model.
 *  \param[in] appKeyIndex   AppKey Index.
 *  \param[in] ttl           TTL or ::MESH_USE_DEFAULT_TTL.
 *  \param[in] companyId     16-bit Bluetooth assigned Company Identifier.
 *  \param[in] ackRequired   TRUE if request type is acknowledged.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtClFaultClear(meshElementId_t elementId, meshAddress_t htSrElemAddr,
                        uint16_t appKeyIndex, uint8_t ttl, uint16_t companyId, bool_t ackRequired);

/*************************************************************************************************/
/*!
 *  \brief     Invokes a self-test procedure on an element implementing a Health Server.
 *
 *  \param[in] elementId     Local element identifier for a Health Client model instance.
 *  \param[in] htSrElemAddr  Address of the element containing an instance of Health Server model.
 *  \param[in] appKeyIndex   AppKey Index.
 *  \param[in] ttl           TTL or ::MESH_USE_DEFAULT_TTL.
 *  \param[in] testId        Identifier of a specific test to be performed.
 *  \param[in] companyId     16-bit Bluetooth assigned Company Identifier.
 *  \param[in] ackRequired   TRUE if request type is acknowledged.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtClFaultTest(meshElementId_t elementId, meshAddress_t htSrElemAddr, uint16_t appKeyIndex,
                       uint8_t ttl, meshHtMdlTestId_t testId, uint16_t companyId,
                       bool_t ackRequired);

/*************************************************************************************************/
/*!
 *  \brief     Gets the current Health Period state of an element.
 *
 *  \param[in] elementId     Local element identifier for a Health Client model instance.
 *  \param[in] htSrElemAddr  Address of the element containing an instance of Health Server model.
 *  \param[in] appKeyIndex   AppKey Index.
 *  \param[in] ttl           TTL or ::MESH_USE_DEFAULT_TTL.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtClPeriodGet(meshElementId_t elementId, meshAddress_t htSrElemAddr, uint16_t appKeyIndex,
                       uint8_t ttl);

/*************************************************************************************************/
/*!
 *  \brief     Sets the current Health Period state of an element.
 *
 *  \param[in] elementId     Local element identifier for a Health Client model instance.
 *  \param[in] htSrElemAddr  Address of the element containing an instance of Health Server model.
 *  \param[in] appKeyIndex   AppKey Index.
 *  \param[in] ttl           TTL or ::MESH_USE_DEFAULT_TTL.
 *  \param[in] periodState   Health period divisor state.
 *  \param[in] ackRequired   TRUE if request type is acknowledged.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtClPeriodSet(meshElementId_t elementId, meshAddress_t htSrElemAddr, uint16_t appKeyIndex,
                       uint8_t ttl, meshHtPeriod_t periodState, bool_t ackRequired);

/*************************************************************************************************/
/*!
 *  \brief     Gets the current Attention Timer state of an element.
 *
 *  \param[in] elementId     Local element identifier for a Health Client model instance.
 *  \param[in] htSrElemAddr  Address of the element containing an instance of Health Server model.
 *  \param[in] appKeyIndex   AppKey Index.
 *  \param[in] ttl           TTL or ::MESH_USE_DEFAULT_TTL.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtClAttentionGet(meshElementId_t elementId, meshAddress_t htSrElemAddr,
                          uint16_t appKeyIndex, uint8_t ttl);

/*************************************************************************************************/
/*!
 *  \brief     Sets the current Attention Timer state of an element.
 *
 *  \param[in] elementId      Local element identifier for a Health Client model instance.
 *  \param[in] htSrElemAddr   Address of the element containing an instance of Health Server model.
 *  \param[in] appKeyIndex    AppKey Index.
 *  \param[in] ttl            TTL or ::MESH_USE_DEFAULT_TTL.
 *  \param[in] attTimerState  Attention timer state.
 *  \param[in] ackRequired    TRUE if request type is acknowledged.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtClAttentionSet(meshElementId_t elementId, meshAddress_t htSrElemAddr,
                          uint16_t appKeyIndex, uint8_t ttl, meshHtAttTimer_t  attTimerState,
                          bool_t ackRequired);

#ifdef __cplusplus
}
#endif

#endif /* MESH_HT_CL_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
