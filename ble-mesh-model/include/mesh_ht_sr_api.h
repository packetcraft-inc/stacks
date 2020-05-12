/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Health Server Model API.
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
 * \addtogroup MESH_HEALTH_SERVER Mesh Health Server Model API
 * @{
 *************************************************************************************************/

#ifndef MESH_HT_SR_API_H
#define MESH_HT_SR_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "wsf_timer.h"
#include "mesh_ht_mdl_api.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Maximum number of companies supported by an instance of Mesh Health Server */
#ifndef MESH_HT_SR_MAX_NUM_COMP
#define MESH_HT_SR_MAX_NUM_COMP       1
#endif

/*! Maximum number of fault identifiers that can be stored on an instance of Health Server */
#ifndef MESH_HT_SR_MAX_NUM_FAULTS
#define MESH_HT_SR_MAX_NUM_FAULTS     5
#endif

/*! \brief Number of supported opcodes for receiving Health Messages */
#define MESH_HT_SR_NUM_RECVD_OPCODES  11

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Structure that stores a Health Server Fault state. Configured by the caller. */
typedef struct meshHtSrFaultState_tag
{
  uint16_t            companyId;                                   /*!< 16-Bit SIG assigned company
                                                                    *   identifier
                                                                    */
  meshHtMdlTestId_t   testId;                                      /*!< Test ID for the most recently
                                                                    *   performed test
                                                                    */
  meshHtFaultId_t     regFaultIdArray[MESH_HT_SR_MAX_NUM_FAULTS];  /*!< Array for storing registered
                                                                    *   faults
                                                                    */
  meshHtFaultId_t     crtFaultIdArray[MESH_HT_SR_MAX_NUM_FAULTS];  /*!< Array for storing current
                                                                    *   faults
                                                                    */
} meshHtSrFaultState_t;

/*! \brief Structure that describes a Mesh Health Server instance */
typedef struct meshHtSrDescriptor_tag
{
  meshHtSrFaultState_t faultStateArray[MESH_HT_SR_MAX_NUM_COMP];   /*!< Fault state array */
  wsfTimer_t           fastPubTmr;                                 /*!< Fast publication timer */
  uint32_t             pubPeriodMs;                                /*!< Publication period */
  meshHtPeriod_t       fastPeriodDiv;                              /*!< Fast period divisor. Divides
                                                                    *   Health Publish Period
                                                                    *   by pow(2, fastPeriodDiv)
                                                                    */
  bool_t               fastPubOn;                                  /*!< TRUE if fast publishing
                                                                    *   using divisor is on
                                                                    */
} meshHtSrDescriptor_t;

/*! \brief Health Server Test Start event data structure */
typedef struct meshHtSrTestStartEvt_tag
{
  wsfMsgHdr_t          hdr;          /*!< Header */
  meshElementId_t      elemId;       /*!< Current element Identifier */
  meshAddress_t        htClAddr;     /*!< Address of the remote element containing
                                      *   an instance of Health Client
                                      */
  uint16_t             companyId;    /*!< Company identifier */
  meshHtMdlTestId_t    testId;       /*!< Identifier of test to start */
  uint16_t             appKeyIndex;  /*!< AppKey identifier when signaling test end */
  bool_t               useTtlZero;   /*!< TTL flag used when signaling test end */
  bool_t               unicastReq;   /*!< Unicast flag used when signaling test end */
  bool_t               notifTestEnd; /*!< TRUE if upper layer should signal test end */
} meshHtSrTestStartEvt_t;

/*! \brief Union of all Health Server model events */
typedef union meshHtSrEvt_tag
{
  wsfMsgHdr_t                   hdr;            /*!< Header */
  meshHtSrTestStartEvt_t        testStartEvt;   /*!< Valid for
                                                 *   ::MESH_HT_SR_TEST_START_EVENT
                                                 */
} meshHtSrEvt_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! \brief WSF handler ID for Health Server */
extern wsfHandlerId_t meshHtSrHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t meshHtSrRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Health Server Handler.
 *
 *  \param[in] handlerId  WSF handler ID of the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtSrHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Health Server model.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshHtSrInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback registered by the upper layer to receive messages
 *                        from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtSrRegister(mmdlEventCback_t recvCback);

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Health Server model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtSrHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Sets company ID of an entry in the Fault State array.
 *
 *  \param[in] elementId        Local element identifier for a Health Server model instance.
 *  \param[in] faultStateIndex  ID of the most recently performed self-test.
 *  \param[in] companyId        Company identifier (16-bit).
 *
 *  \remarks   Fault State Array is identified by the company ID. This means faultStateIndex should
 *             always be less than ::MESH_HT_SR_MAX_NUM_COMP.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtSrSetCompanyId(meshElementId_t elementId, uint8_t faultStateIndex, uint16_t companyId);

/*************************************************************************************************/
/*!
 *  \brief     Adds a fault ID to a fault state array based on company Identifier.
 *
 *  \param[in] elementId     Local element identifier for a Health Server model instance.
 *  \param[in] companyId     Company identifier (16-bit).
 *  \param[in] recentTestId  ID of the most recently performed self-test.
 *  \param[in] faultId       Identifier of the fault to be added.
 *
 *  \remarks   Call this function with ::MESH_HT_MODEL_FAULT_NO_FAULT fault identifier to update
 *             the most recent test id.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtSrAddFault(meshElementId_t elementId, uint16_t companyId,
                      meshHtMdlTestId_t recentTestId, meshHtFaultId_t faultId);

/*************************************************************************************************/
/*!
 *  \brief     Removes a fault ID from a fault state array based on company identifier.
 *
 *  \param[in] elementId     Local element identifier for a Health Server model instance.
 *  \param[in] companyId     Company identifier (16-bit).
 *  \param[in] recentTestId  ID of the most recently performed self-test.
 *  \param[in] faultId       Identifier of the fault to be removed.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtSrRemoveFault(meshElementId_t elementId, uint16_t companyId,
                         meshHtMdlTestId_t recentTestId, meshHtFaultId_t faultId);

/*************************************************************************************************/
/*!
 *  \brief     Removes all fault IDs from a fault state array based on company identifier.
 *
 *  \param[in] elementId     Local element identifier for a Health Server model instance.
 *  \param[in] companyId     Company identifier (16-bit).
 *  \param[in] recentTestId  ID of the most recently performed self-test.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtSrClearFaults(meshElementId_t elementId, uint16_t companyId,
                         meshHtMdlTestId_t recentTestId);

/*************************************************************************************************/
/*!
 *  \brief     Signals to a Health Client that a test has been performed.
 *
 *  \param[in] elementId     Health Server element identifier to determine instance.
 *  \param[in] companyId     Company identifier for determining fault state.
 *  \param[in] meshHtClAddr  Health Client address received in the ::MESH_HT_SR_TEST_START_EVENT.
 *  \param[in] appKeyIndex   AppKey identifier received in the ::MESH_HT_SR_TEST_START_EVENT.
 *  \param[in] useTtlZero    TTL flag received in the ::MESH_HT_SR_TEST_START_EVENT.
 *  \param[in] unicastReq    Unicast flag received in the ::MESH_HT_SR_TEST_START_EVENT.
 *
 *  \return    None.
 *
 *  \remarks   Upon receiving a ::MESH_HT_SR_TEST_START_EVENT event with the notifTestEnd parameter
 *             set to TRUE, the user shall call this API using the associated event parameters, but
 *             not before logging 0 or more faults so that the most recent test identifier is stored.
 *
 */
/*************************************************************************************************/
void MeshHtSrSignalTestEnd(meshElementId_t elementId, uint16_t companyId,
                           meshAddress_t meshHtClAddr, uint16_t appKeyIndex, bool_t useTtlZero,
                           bool_t unicastReq);

#ifdef __cplusplus
}
#endif

#endif /* MESH_HT_SR_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
