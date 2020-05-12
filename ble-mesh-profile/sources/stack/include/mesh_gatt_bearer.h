/*************************************************************************************************/
/*!
 *  \file   mesh_gatt_bearer.h
 *
 *  \brief  GATT bearer module interface.
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
 *  \addtogroup MESH_GATT_BEARER Mesh GATT Bearer API
 *  @{
 */
/*************************************************************************************************/

#ifndef MESH_GATT_BEARER_H
#define MESH_GATT_BEARER_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh GATT Bearer notification event types enumeration */
enum meshGattEventTypes
{
  MESH_GATT_PROXY_CONN_OPENED = 0x00,  /*!< GATT Proxy connection opened */
  MESH_GATT_PROXY_CONN_CLOSED = 0x01,  /*!< GATT Proxy connection closed */
  MESH_GATT_PACKET_PROCESSED  = 0x02   /*!< GATT packet processed by the lower layers.
                                        *   This means that either the packet has been sent
                                        *   over-the-air or that it was dropped as a consequence of
                                        *   a removed interface.
                                        */
};

/*! Mesh GATT Bearer notification event type. See ::meshGattEventTypes */
typedef uint8_t meshGattEventType_t;

/*! Mesh GATT Bearer PDU status structure */
typedef struct meshGattBrPduStatus_tag
{
  meshGattEventType_t     eventType;   /*!< Mesh GATT Bearer notification event type.*/
  meshGattProxyPduType_t  pduType;     /*!< GATT PDU type. See ::meshGattProxyPduType */
  uint8_t                 *pPdu;       /*!< Pointer to the sent PDU mentioned in the event */
} meshGattBrPduStatus_t;

/*! Mesh GATT Bearer Event notification union */
typedef union meshGattBrEventParams_tag
{
  meshGattEventType_t      eventType;     /*!< Mesh GATT Bearer notification event type.*/
  meshGattBrPduStatus_t    brPduStatus;   /*!< PDU status structure. See ::meshGattBrPduStatus_t */
} meshGattEvent_t;

/*************************************************************************************************/
/*!
 *  \brief     Mesh GATT Proxy PDU received callback function pointer type.
 *
 *  \param[in] connId       Unique identifier for the connection on which the PDU was received.
 *                          Valid range is 0x00 to 0x1F.
 *  \param[in] pduType      PDU type. See ::meshGattProxyPduType.
 *  \param[in] pProxyPdu    Pointer to a buffer containing the GATT Proxy PDU.
 *  \param[in] proxyPduLen  Size of the buffer referenced by pProxyPdu.
 *
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshGattRecvCback_t)(meshGattProxyConnId_t connId, meshGattProxyPduType_t pduType,
                                    const uint8_t *pProxyPdu, uint16_t proxyPduLen);

/*************************************************************************************************/
/*!
 *  \brief     Mesh GATT Bearer event notification callback function pointer type.
 *
 *  \param[in] connId  Unique identifier for the connection on which the event was received.
 *                     Valid range is 0x00 to 0x1F.
 *  \param[in] pEvent  Pointer to GATT Event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshGattEventNotifyCback_t)(meshGattProxyConnId_t connId,
                                           const meshGattEvent_t *pEvent);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Mesh GATT Bearer layer.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshGattInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Register upper layer callbacks.
 *
 *  \param[in] recvCback   Callback to be invoked when a Mesh GATT PDU is received on a specific
 *                         GATT connection.
 *  \param[in] notifCback  Callback to be invoked when an event on a specific GATT connection
 *                         is triggered.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshGattRegister(meshGattRecvCback_t recvCback, meshGattEventNotifyCback_t notifCback);

/*************************************************************************************************/
/*!
 *  \brief     Register callback to send PDU to bearer module.
 *
 *  \param[in] cback  Callback to be invoked to send a GATT Proxy PDU outside the stack.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshGattRegisterPduSendCback(meshGattProxyPduSendCback_t cback);

/*************************************************************************************************/
/*!
 *  \brief     Adds a new GATT Proxy connection into the bearer.
 *
 *  \param[in] connId       Unique identifier for the connection. Valid range 0x00 to 0x0F.
 *  \param[in] maxProxyPdu  Maximum size of the Proxy PDU, derived from GATT MTU.
 *
 *  \return    None.
 *
 *  \remarks   If GATT Proxy is supported and this the first connection, it also enables proxy.
 */
/*************************************************************************************************/
void MeshGattAddProxyConn(meshGattProxyConnId_t connId, uint16_t maxProxyPdu);

/*************************************************************************************************/
/*!
 *  \brief     Removes a GATT Proxy connection from the bearer.
 *
 *  \param[in] connId  Unique identifier for a connection. Valid range is 0x00 to 0x1F.
 *
 *  \return    None.
 *
 *  \remarks   A connection removed event is received after calling this.
 */
/*************************************************************************************************/
void MeshGattRemoveProxyConn(meshGattProxyConnId_t connId);

/*************************************************************************************************/
/*!
 *  \brief     Closes a GATT Proxy connection.
 *
 *  \param[in] connId  GATT connection identifier.
 *
 *  \return    None.
 *
 *  \remarks   A connection closed event is received after calling this.
 */
/*************************************************************************************************/
void MeshGattCloseProxyConn(meshGattProxyConnId_t connId);

/*************************************************************************************************/
/*!
 *  \brief     Processes a GATT Proxy PDU received on a GATT interface.
 *
 *  \param[in] connId       Unique identifier for the connection on which the PDU was received.
 *                          Valid range is 0x00 to 0x1F.
 *  \param[in] pProxyPdu    Pointer to a buffer containing the GATT Proxy PDU.
 *  \param[in] proxyPduLen  Size of the buffer referenced by pProxyPdu.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshGattProcessPdu(meshGattProxyConnId_t connId, const uint8_t *pProxyPdu,
                        uint16_t proxyPduLen);

/*************************************************************************************************/
/*!
 *  \brief     Asynchronously sends a Mesh Bearer PDU on a GATT Proxy interface.
 *
 *  \param[in] connId   Unique identifier for the connection on which the PDU was received.
 *                      Valid range is 0x00 to 0x1F.
 *  \param[in] pduType  PDU type. See ::meshGattProxyPduType.
 *  \param[in] pBrPdu   Pointer to a buffer containing a Mesh Bearer PDU.
 *  \param[in] pduLen   Size of the Mesh Bearer PDU.
 *
 *  \return    TRUE if message is sent or queued for later transmission, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshGattSendBrPdu(meshGattProxyConnId_t connId, meshGattProxyPduType_t pduType,
                         const uint8_t *pBrPdu, uint16_t pduLen);

/*************************************************************************************************/
/*!
 *  \brief     Adds a new GATT Proxy connection into the bearer.
 *
 *  \param[in] connId  Unique identifier for the connection. Valid range 0x00 to 0x1F.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshGattSignalIfReady(meshGattProxyConnId_t connId);

#ifdef __cplusplus
}
#endif

#endif /* MESH_GATT_BEARER_H */
