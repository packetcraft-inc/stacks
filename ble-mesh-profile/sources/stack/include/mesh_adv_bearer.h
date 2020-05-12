/*************************************************************************************************/
/*!
 *  \file   mesh_adv_bearer.h
 *
 *  \brief  ADV bearer module interface.
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

/*************************************************************************************************/
/*!
 *  \addtogroup MESH_ADV_BEARER Mesh Advertising Bearer API
 *  @{
 */
/*************************************************************************************************/

#ifndef MESH_ADV_BEARER_H
#define MESH_ADV_BEARER_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Advertising Bearer notification event types enumeration */
enum meshAdvEventTypes
{
  MESH_ADV_INTERFACE_OPENED = 0x00,  /*!< Advertising interface opened */
  MESH_ADV_INTERFACE_CLOSED = 0x01,  /*!< Advertising interface closed */
  MESH_ADV_PACKET_PROCESSED = 0x02   /*!< Advertising packet processed by the lower layers.
                                      *   This means that either the packet has been sent
                                      *   over-the-air or that it was dropped as a consequence of
                                      *   a removed interface.
                                      */
};

/*! Mesh ADV type */
typedef uint8_t meshAdvType_t;

/*! Mesh Advertising Bearer notification event type. See ::meshAdvEventTypes */
typedef uint8_t meshAdvEvent_t;

/*! Mesh Advertising Bearer PDU status structure */
typedef struct meshAdvBrPduStatus_tag
{
  meshAdvType_t  adType; /*!< Advertising Type. */
  uint8_t        *pPdu;  /*!< Pointer to the sent PDU mentioned in the event */
} meshAdvBrPduStatus_t;

/*! Mesh Advertising Bearer Event notification union */
typedef union meshAdvBrEventParams_tag
{
  meshAdvBrPduStatus_t    brPduStatus;   /*!< PDU status structure. See ::meshAdvBrPduStatus_t */
} meshAdvBrEventParams_t;

/*************************************************************************************************/
/*!
 *  \brief     Mesh Advertising PDU received callback function pointer type.
 *
 *  \param[in] advIfId  Unique advertising interface identifier.
 *  \param[in] advType  ADV type received. See ::meshAdvType
 *  \param[in] pBrPdu   Pointer to the bearer PDU received.
 *  \param[in] pduLen   Size of the bearer PDU received.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshAdvRecvCback_t)(meshAdvIfId_t advIfId, meshAdvType_t advType,
                                   const uint8_t *pBrPdu, uint8_t pduLen);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Advertising Bearer event notification callback function pointer type.
 *
 *  \param[in] ifId          Advertising interface identifier.
 *  \param[in] event         Reason the callback is being invoked. See ::meshBrEvent_t
 *  \param[in] pEventParams  Pointer to the event parameters structure passed to the function.
 *                           See ::meshAdvBrEventParams_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshAdvEventNotifyCback_t)(meshAdvIfId_t ifId, meshAdvEvent_t event,
                                          const meshAdvBrEventParams_t *pEventParams);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Mesh ADV Bearer layer.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshAdvInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Register upper layer callbacks.
 *
 *  \param[in] recvCback   Callback to be invoked when a Mesh ADV PDU is received on a specific ADV
 *                         interface
 *  \param[in] notifCback  Callback to be invoked when an event on a specific advertising interface
 *                         is triggered
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAdvRegister(meshAdvRecvCback_t recvCback, meshAdvEventNotifyCback_t notifCback);

/*************************************************************************************************/
/*!
 *  \brief     Register callback to send PDU to bearer module.
 *
 *  \param[in] cback  Callback to be invoked to send a Mesh ADV PDU outside the stack.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAdvRegisterPduSendCback(meshAdvPduSendCback_t cback);

/*************************************************************************************************/
/*!
 *  \brief     Allocates a Mesh ADV bearer instance.
 *
 *  \param[in] advIfId          Unique identifier for the interface.
 *  \param[in] statusCback      Status callback for the advIfId interface.
 *  \param[in] advPduRecvCback  Callback invoked by the Mesh Stack to send an advertising PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAdvAddInterface(meshAdvIfId_t advIfId);

/*************************************************************************************************/
/*!
 *  \brief     Frees a Mesh ADV bearer instance.
 *
 *  \param[in] advIfId  Unique identifier for the interface.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAdvRemoveInterface(meshAdvIfId_t advIfId);

/*************************************************************************************************/
/*!
 *  \brief     Processes an ADV PDU received on an ADV instance.
 *
 *  \param[in] advIfId  Unique identifier for the interface.
 *  \param[in] pAdvPdu  Pointer to a buffer containing an ADV PDU.
 *  \param[in] pduLen   Size of the ADV PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAdvProcessPdu(meshAdvIfId_t advIfId, const uint8_t *pAdvPdu, uint8_t pduLen);

/*************************************************************************************************/
/*!
 *  \brief     Signals the Advertising Bearer that the interface is ready to transmit packets.
 *
 *  \param[in] advIfId  Unique identifier for the interface.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAdvSignalInterfaceReady(meshAdvIfId_t advIfId);

/*************************************************************************************************/
/*!
 *  \brief     Asynchronously sends a Mesh Bearer PDU on an ADV bearer instance.
 *
 *  \param[in] advIfId  Unique identifier for the interface.
 *  \param[in] advType  ADV type received. See ::meshAdvType
 *  \param[in] pBrPdu   Pointer to a buffer containing a Mesh Bearer PDU.
 *  \param[in] pduLen   Size of the Mesh ADV Bearer PDU.
 *
 *  \return    TRUE if message is sent or queued for later transmission, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshAdvSendBrPdu(meshAdvIfId_t advIfId, meshAdvType_t advType,
                                 const uint8_t *pBrPdu, uint8_t pduLen);

#ifdef __cplusplus
}
#endif

#endif /* MESH_ADV_BEARER_H */
