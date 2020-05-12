/*************************************************************************************************/
/*!
 *  \file   mesh_bearer.h
 *
 *  \brief  Bearer module interface.
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
 *  \addtogroup MESH_BEARER Mesh Bearer API
 *  @{
 */
/*************************************************************************************************/

#ifndef MESH_BEARER_H
#define MESH_BEARER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "cfg_mesh_stack.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Maximum number of Mesh Bearer interfaces supported */
#define MESH_BR_MAX_INTERFACES                (MESH_ADV_MAX_INTERFACES + MESH_GATT_MAX_CONNECTIONS)

/*! Invalid Mesh Bearer interface ID value */
#define MESH_BR_INVALID_INTERFACE_ID          0xFF

/*! Defines the bit mask for bearer type inside the bearer interface identifier */
#define MESH_BR_INTERFACE_ID_TYPE_MASK        0xF0

/*! Defines the bit mask for interface identifier inside the bearer interface identifier */
#define MESH_BR_INTERFACE_ID_INTERFACE_MASK   0x0F

/*! Defines offset of the bearer type inside the bearer interface identifier */
#define MESH_BR_INTERFACE_ID_TYPE_OFFSET      5

/*! Extract bearer type from a bearer interface identifier. See ::meshBrInterfaceId_t */
#define MESH_BR_GET_BR_TYPE(brInterfaceId)   (MESH_UTILS_BF_GET(brInterfaceId, \
                                              MESH_BR_INTERFACE_ID_TYPE_OFFSET, \
                                              MESH_BR_INTERFACE_ID_TYPE_OFFSET))

/*! Create bearer interface identifier from ADV interface identifier. See ::meshBrInterfaceId_t */
#define MESH_BR_ADV_IF_TO_BR_IF(interfaceId) (MESH_UTILS_BITMASK_SET(interfaceId, \
                                              (MESH_ADV_BEARER << MESH_BR_INTERFACE_ID_TYPE_OFFSET)))

/*! Create ADV interface interface identifier from bearer identifier. See ::meshBrInterfaceId_t */
#define MESH_BR_IF_TO_ADV_IF(brInterfaceId)  (MESH_UTILS_BITMASK_CLR(brInterfaceId, \
                                              MESH_BR_INTERFACE_ID_TYPE_MASK))

/*! Create bearer interface identifier from GATT connection identifier. See ::meshBrInterfaceId_t */
#define MESH_BR_CONN_ID_TO_BR_IF(connId) (MESH_UTILS_BITMASK_SET(connId, \
                                          (MESH_GATT_BEARER << MESH_BR_INTERFACE_ID_TYPE_OFFSET)))

/*! Create GATT connection identifier from bearer identifier. See ::meshBrInterfaceId_t */
#define MESH_BR_IF_TO_CONN_ID(brInterfaceId)  (MESH_UTILS_BITMASK_CLR(brInterfaceId, \
                                               MESH_BR_INTERFACE_ID_TYPE_MASK))

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Bearer notification event types enumeration */
enum meshBrEventTypes
{
  MESH_BR_INTERFACE_OPENED_EVT                = 0x00,  /*!< Bearer interface opened */
  MESH_BR_INTERFACE_CLOSED_EVT                = 0x01,  /*!< Bearer interface closed */
  MESH_BR_INTERFACE_PACKET_SENT_EVT           = 0x02,  /*!< Reference of sent packet */
};

/*! Mesh Bearer interface types enumeration */
enum meshBrType
{
  MESH_ADV_BEARER         = 0x00,  /*!< Mesh Advertising Bearer */
  MESH_GATT_BEARER        = 0x01,  /*!< Mesh GATT Bearer */
  MESH_INVALID_BEARER     = 0x02   /*!< Mesh Invalid Bearer type */
};

/*! Unique Mesh Bearer interface ID. The identifier is described as follows:
 *  - Bits 3-0 : The unique interface identifier for a specific bearer
 *  - Bits 7-4 : The bearer type. See ::meshBrType
 */
typedef uint8_t meshBrInterfaceId_t;

/*! Mesh Bearer interface type. See ::meshBrType */
typedef uint8_t meshBrType_t;

/*! Mesh Bearer notification event type. See ::meshBrEventTypes */
typedef uint8_t meshBrEvent_t;

/*! Mesh Bearer configuration structure */
typedef struct meshBrConfig_tag
{
  meshBrType_t  bearerType;  /*!< Mesh Bearer interface type */
} meshBrConfig_t;

/*! Mesh Bearer PDU status structure */
typedef struct meshBrPduStatus_tag
{
  meshBrType_t  bearerType;  /*!< Mesh Bearer interface type */
  uint8_t       *pPdu;       /*!< Pointer to the delivered PDU */
} meshBrPduStatus_t;

/*! Mesh Bearer Event notification union */
typedef union meshBrEventParams_tag
{
  meshBrConfig_t          brConfig;           /*!< MESH_BR_INTERFACE_OPENED_EVT
                                               *   MESH_BR_INTERFACE_CLOSED_EVT
                                               */
  meshBrPduStatus_t       brPduStatus;        /*!< MESH_BR_INTERFACE_PACKET_SENT_EVT */
} meshBrEventParams_t;

/*************************************************************************************************/
/*!
 *  \brief     Mesh NWK PDU received on Bearer callback function pointer type.
 *
 *  \param[in] brInterfaceId  Unique Mesh Bearer interface ID.
 *  \param[in] pNwkPdu        Pointer to the Network PDU received.
 *  \param[in] pduLen         Size of the Network PDU received.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshBrNwkPduRecvCback_t)(meshBrInterfaceId_t brInterfaceId, const uint8_t *pNwkPdu,
                                        uint8_t pduLen);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Beacon PDU received callback function pointer type.
 *
 *  \param[in] brInterfaceId  Unique Mesh Bearer interface ID.
 *  \param[in] pBeaconData    Pointer to the Beacon Data payload.
 *  \param[in] dataLen        Size of the Beacon Data payload.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshBrBeaconRecvCback_t)(meshBrInterfaceId_t brInterfaceId,
                                        const uint8_t *pBeaconData, uint8_t dataLen);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Provisioning PDU received on Bearer callback function pointer type.
 *
 *  \param[in] brInterfaceId  Unique Mesh Bearer interface ID.
 *  \param[in] pPbPdu         Pointer to the Provisioning Bearer PDU received.
 *  \param[in] pduLen         Size of the Provisioning Bearer PDU received.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshBrPbPduRecvCback_t)(meshBrInterfaceId_t brInterfaceId, const uint8_t *pPbPdu,
                                       uint8_t pduLen);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Bearer event notification callback function pointer type.
 *
 *  \param[in] brInterfaceId  Unique Mesh Bearer interface ID.
 *  \param[in] event          Reason the callback is being invoked. See ::meshBrEvent_t
 *  \param[in] pEventParams   Pointer to the event parameters structure passed to the function.
 *                            See ::meshBrEventParams_t
 *
 *  \return    None.
 *
 *  \note      For the Mesh Bearer PDU transmission, the pEventParams contains the bearer PDU status
 *             structure. See ::meshBrPduStatus_t
 *             For interface specific events, the pEventParams contains the bearer configuration
 *             structure. See ::meshBrConfig_t
 *             For filtering specific events, the pEventParams contains the interface filter
 *             structure. See ::meshBrInterfaceFilter_t
 */
/*************************************************************************************************/
typedef void (*meshBrEventNotifyCback_t)(meshBrInterfaceId_t brInterfaceId, meshBrEvent_t event,
                                         const meshBrEventParams_t *pEventParams);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Mesh Bearer layer.
 *  \return None.
 */
/*************************************************************************************************/
void MeshBrInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback functions for event notification and received Network PDUs on
 *             the bearer interface.
 *
 *  \param[in] eventCback       Event notification callback for the upper layer.
 *  \param[in] nwkPduRecvCback  Callback to be invoked when a Mesh Bearer PDU is received on
 *                              a specific bearer interface.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshBrRegisterNwk(meshBrEventNotifyCback_t eventCback, meshBrNwkPduRecvCback_t nwkPduRecvCback);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback functions for event notification and received Secure Network
 *             Beacon PDUs on the bearer interface.
 *
 *  \param[in] eventCback       Event notification callback for the upper layer.
 *  \param[in] beaconRecvCback  Callback to be invoked when a Mesh Beacon PDU is received on
 *                              the bearer interface.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshBrRegisterNwkBeacon(meshBrEventNotifyCback_t eventCback,
                             meshBrBeaconRecvCback_t beaconRecvCback);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback functions for event notification and received Provisioning
 *             PDUs on the bearer interface.
 *
 *  \param[in] eventCback      Event notification callback for the upper layer.
 *  \param[in] pbPduRecvCback  Callback to be invoked when a Mesh Bearer PDU is received on
 *                             a specific bearer interface.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshBrRegisterPb(meshBrEventNotifyCback_t eventCback, meshBrPbPduRecvCback_t pbPduRecvCback);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback functions for event notification and received Unprovisioned
 *             Device Beacon PDUs on the bearer interface.
 *
 *  \param[in] eventCback        Event notification callback for the upper layer.
 *  \param[in] pbBcPduRecvCback  Callback to be invoked when a Mesh Bearer PDU is received on
 *                               a specific bearer interface.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshBrRegisterPbBeacon(meshBrEventNotifyCback_t eventCback,
                            meshBrBeaconRecvCback_t pbBcPduRecvCback);

/*************************************************************************************************/
/*!
 *  \brief     Asynchronously sends a Mesh Network PDU on a bearer interface.
 *
 *  \param[in] brIfId   Unique Mesh Bearer interface ID.
 *  \param[in] pNwkPdu  Pointer to a buffer containing a Mesh Network PDU.
 *  \param[in] pduLen   Size of the Mesh Network PDU.
 *
 *  \return    True if message is sent to the interface, False otherwise.
 *
 *  \note      A notification event will be sent with the transmission status when completed.
 *             See ::meshBrEvent_t and ::meshBrPduStatus_t
 */
/*************************************************************************************************/
bool_t MeshBrSendNwkPdu(meshBrInterfaceId_t brIfId, const uint8_t *pNwkPdu, uint8_t pduLen);

/*************************************************************************************************/
/*!
 *  \brief     Asynchronously sends a Mesh Beacon PDU on a bearer interface.
 *
 *  \param[in] brIfId       Unique Mesh Bearer interface ID.
 *  \param[in] pBeaconData  Pointer to the Beacon Data payload.
 *  \param[in] dataLen      Size of the Beacon Data payload.
 *
 *  \return    True if message is sent to the interface, False otherwise.
 */
/*************************************************************************************************/
bool_t MeshBrSendBeaconPdu(meshBrInterfaceId_t brIfId, uint8_t *pBeaconData, uint8_t dataLen);

/*************************************************************************************************/
/*!
 *  \brief     Asynchronously sends a Mesh Provisioning Bearer PDU on a bearer interface.
 *
 *  \param[in] brIfId   Unique Mesh Bearer interface ID.
 *  \param[in] pPrvPdu  Pointer to the Beacon Data payload.
 *  \param[in] pduLen   Size of the Beacon Data payload.
 *
 *  \return    True if message is sent to the interface, False otherwise.
 */
/*************************************************************************************************/
bool_t MeshBrSendPrvPdu(meshBrInterfaceId_t brIfId, const uint8_t *pPrvPdu, uint8_t pduLen);

/*************************************************************************************************/
/*!
 *  \brief     Asynchronously sends a Proxy Configuration on a bearer interface.
 *
 *  \param[in] brIfId   Unique Mesh Bearer interface ID.
 *  \param[in] pCfgPdu  Pointer to the Proxy Configuration Data payload.
 *  \param[in] pduLen   Size of the Proxy Configuration Data payload.
 *
 *  \return    True if message is sent to the interface, False otherwise.
 */
/*************************************************************************************************/
bool_t MeshBrSendCfgPdu(meshBrInterfaceId_t brIfId, const uint8_t *pCfgPdu, uint8_t pduLen);

/*************************************************************************************************/
/*!
 *  \brief     Closes the specified bearer interface. Works only for GATT interfaces.
 *
 *  \param[in] brIfId  Unique Mesh Bearer interface ID.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshBrCloseIf(meshBrInterfaceId_t brIfId);

/*************************************************************************************************/
/*!
 *  \brief  Initializes the GATT bearer functionality.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshBrEnableGatt(void);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback functions for event notification and received Proxy
 *             Configuration message on the bearer interface.
 *
 *  \param[in] eventCback    Event notification callback for the upper layer.
 *  \param[in] pduRecvCback  Callback to be invoked when a Mesh Bearer PDU is received on
 *                           a specific bearer interface.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshBrRegisterProxy(meshBrEventNotifyCback_t eventCback, meshBrNwkPduRecvCback_t pduRecvCback);

#ifdef __cplusplus
}
#endif

#endif /* MESH_BEARER_H */

/*************************************************************************************************/
/*!
 *  @}
 */
/*************************************************************************************************/
