/*************************************************************************************************/
/*!
 *  \file   mesh_prv_br_main.h
 *
 *  \brief  Mesh Provisioning Bearer module interface.
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

#ifndef MESH_PRV_BR_MAIN_H
#define MESH_PRV_BR_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mesh_bearer.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Provisioning bearer types enumeration */
enum meshPrvType
{
  MESH_PRV_SERVER,  /*!< Provisioner Server */
  MESH_PRV_CLIENT   /*!< Provisioner Client */
};

/*! Mesh Provisioner type. See ::meshPrvType */
typedef uint8_t meshPrvType_t;

/*! Mesh Provisioning Bearer notification event types enumeration */
enum meshPrvBrEventTypes
{
  MESH_PRV_BR_LINK_OPENED,           /*!< Provisioning bearer link opened */
  MESH_PRV_BR_LINK_FAILED,           /*!< Provisioning bearer link failed to open */
  MESH_PRV_BR_LINK_CLOSED_BY_PEER,   /*!< Provisioning bearer link closed by peer */
  MESH_PRV_BR_SEND_TIMEOUT,          /*!< Provisioning bearer link closed on Tx transaction failure */
  MESH_PRV_BR_PDU_SENT,              /*!< Provisioning PDU was sent */
  MESH_PRV_BR_CONN_CLOSED,           /*!< Provisioning bearer GATT connection closed */
};

/*! Mesh Bearer notification event type. See ::meshPrvBrEventTypes */
typedef uint8_t meshPrvBrEvent_t;

/*! PB-ADV Link Close reason type. See ::meshPrvBrReasonTypes */
typedef uint8_t meshPrvBrReason_t;

/*! Mesh Bearer Event notification union */
typedef union meshPrvBrEventParams_tag
{
  uint8_t     linkCloseReason;        /*!< Reason for PB link closure */
  uint8_t     pduSentOpcode;          /*!< Opcode of the PDU that was sent */
} meshPrvBrEventParams_t;

/*************************************************************************************************/
/*!
 *  \brief     Mesh Provisioning Bearer PDU received function pointer type.
 *
 *  \param[in] pPdu    Pointer to the Provisioning Bearer PDU received.
 *  \param[in] pduLen  Size of the Provisioning Bearer PDU received.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshPrvBrPduRecvCback_t)(const uint8_t *pPrvPdu, uint8_t pduLen);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Provisioning Bearer event notification callback function pointer type.
 *
 *  \param[in] event         Reason the callback is being invoked. See ::meshPrvBrEvent_t
 *  \param[in] pEventParams  Pointer to the event parameters structure passed to the function.
 *                           See ::meshPrvBrEventParams_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshPrvBrEventNotifyCback_t) (meshPrvBrEvent_t event,
                                              const meshPrvBrEventParams_t *pEventParams);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Provisioning Bearer functionality.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshPrvBrInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callbacks.
 *
 *  \param[in] prvPduRecvCback      Callback to be invoked when a Provisioning PDU is received.
 *  \param[in] prvEventNotifyCback  Event notification callback for the upper layer.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvBrRegisterCback(meshPrvBrPduRecvCback_t prvPduRecvCback,
                            meshPrvBrEventNotifyCback_t prvEventNotifyCback);

/*************************************************************************************************/
/*!
 *  \brief     Enables PB-ADV Server functionality.
 *
 *  \param[in] advIfId         Advertising bearer interface ID.
 *  \param[in] beaconInterval  Unprovisioned Device beacon interval in ms.
 *  \param[in] pUuid           16 bytes of UUID data.
 *  \param[in] oobInfoSrc      OOB information indicating the availability of OOB data.
 *  \param[in] pUriData        Uniform Resource Identifier (URI) data.
 *  \param[in] uriLen          URI data length.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvBrEnablePbAdvServer(uint8_t advIfId, uint32_t periodInMs, const uint8_t *pUuid,
                               uint16_t oobInfoSrc, const uint8_t *pUriData, uint8_t uriLen);

/*************************************************************************************************/
/*!
 *  \brief     Enables PB-GATT Server functionality.
 *
 *  \param[in] connId  GATT bearer interface ID.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvBrEnablePbGattServer(uint8_t connId);

/*************************************************************************************************/
/*!
 *  \brief     Enables PB-ADV Client functionality.
 *
 *  \param[in] advIfId  Advertising bearer interface ID.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvBrEnablePbAdvClient(uint8_t advIfId);

/*************************************************************************************************/
/*!
 *  \brief     Enables PB-GATT Client functionality.
 *
 *  \param[in] connId  GATT bearer interface ID.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvBrEnablePbGattClient(uint8_t connId);

/*************************************************************************************************/
/*!
 *  \brief     Closes the provisioning link. Can be used by both Provisioning Client and Server.
 *
 *  \param[in] reason  Reason for closing the interface. See ::meshPrvBrReasonTypes.
 *
 *  \remarks   Calling this function will NOT generate the MESH_PRV_BR_LINK_CLOSED event,
 *             because the upper layer is already aware of the link closure.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvBrCloseLink(meshPrvBrReason_t reason);

/*************************************************************************************************/
/*!
 *  \brief   Closes the provisioning link without sending Link Close.
 *           Can be used by both Provisioning Client and Server.
 *
 *  \remarks Calling this function will NOT generate the MESH_PRV_BR_LINK_CLOSED event,
 *           because the upper layer is already aware of the link closure.
 *
 *  \return  None.
 */
/*************************************************************************************************/
void MeshPrvBrCloseLinkSilent(void);

/*************************************************************************************************/
/*!
 *  \brief     Opens a PB-ADV link with a Provisioning Server on the already enabled advertising
 *             interface. Stores device UUID and generates link ID. The Link Open message is sent
 *             after receiving an unprovisioned beacon with a matching UUID. Used only by a
 *             Provisioning Client.
 *
 *  \param[in] pUuid  Device UUID value of the Node.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvBrOpenPbAdvLink(const uint8_t *pUuid);

/*************************************************************************************************/
/*!
 *  \brief     Sends a Provisioning PDU on the already enabled Provisioning Bearer interface.
 *
 *  \param[in] pPrvPdu  Pointer to the Provisioning PDU.
 *  \param[in] pduLen   Provisioning PDU length.
 *
 *  \return    TRUE if the PDU can be send, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshPrvBrSendProvisioningPdu(uint8_t* pPrvPdu, uint8_t pduLen);

#ifdef __cplusplus
}
#endif

#endif /* MESH_PRV_BR_MAIN_H */
