/*************************************************************************************************/
/*!
 *  \file   mesh_main.h
 *
 *  \brief  Main internal stack interface.
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
 *
 */
/*************************************************************************************************/

#ifndef MESH_MAIN_H
#define MESH_MAIN_H

#include "wsf_timer.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Mesh Timer tick for sending Mesh messages with random delay. */
#ifndef MESH_API_TMR_SEND_TICK_MS
#define MESH_API_TMR_SEND_TICK_MS (10)
#endif

/*! Mesh NVM dataset count */
#define MESH_NVM_DATASET_COUNT                                 12

/*! Internal NVM dataset IDs */
#define MESH_LOCAL_CFG_NVM_DATASET_ID                          0xC000
#define MESH_LOCAL_CFG_NVM_NET_KEY_DATASET_ID                  0xC001
#define MESH_LOCAL_CFG_NVM_APP_KEY_DATASET_ID                  0xC002
#define MESH_LOCAL_CFG_NVM_APP_KEY_BIND_DATASET_ID             0xC003
#define MESH_LOCAL_CFG_NVM_ADDRESS_DATASET_ID                  0xC004
#define MESH_LOCAL_CFG_NVM_VIRTUAL_ADDR_DATASET_ID             0xC005
#define MESH_LOCAL_CFG_NVM_SUBSCR_DATASET_ID                   0xC006
#define MESH_LOCAL_CFG_NVM_SEQ_NUMBER_DATASET_ID               0xC007
#define MESH_LOCAL_CFG_NVM_SEQ_NUMBER_THRESH_DATASET_ID        0xC008
#define MESH_LOCAL_CFG_NVM_MODEL_DATASET_ID                    0xC009
#define MESH_LOCAL_CFG_NVM_HB_DATASET_ID                       0xC00A
#define MESH_RP_NVM_LIST_DATASET_ID                            0xC00B

/*! WSF Message Event base */
#define MESH_STACK_MSG_START                                   0x00
#define MESH_FRIENDSHIP_MSG_START                              0x20
#define MESH_CFG_MDL_CL_MSG_START                              0x30
#define MESH_ACC_MSG_START                                     0x40
#define MESH_HB_MSG_START                                      0x50
#define MESH_SAR_RX_MSG_START                                  0x60
#define MESH_SAR_TX_MSG_START                                  0x70
#define MESH_NWK_MSG_START                                     0x80
#define MESH_NWK_MGMT_MSG_START                                0x90
#define MESH_NWK_BEACON_MSG_START                              0xA0
#define MESH_PRV_BEACON_MSG_START                              0xB0
#define MESH_PRV_BR_MSG_START                                  0xC0
#define MESH_GATT_PROXY_MSG_START                              0xD0
#define MESH_LOCAL_CFG_MSG_START                               0xE0

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Stack WSF messages from API */
enum meshWsfMsgEvents
{
  MESH_MSG_API_INIT = MESH_STACK_MSG_START,     /*!< Init event */
  MESH_MSG_API_RESET,                           /*!< Factory reset event */
  MESH_MSG_API_SEND_MSG,                        /*!< Mesh Stack Send Message */
  MESH_MSG_API_PUBLISH_MSG,                     /*!< Mesh Stack Publish Message */
  MESH_MSG_API_ADD_GATT_CONN,                   /*!< Mesh Stack Add Gatt Proxy Connection */
  MESH_MSG_API_REM_GATT_CONN,                   /*!< Mesh Stack Remove Gatt Proxy Connection */
  MESH_MSG_API_PROC_GATT_MSG,                   /*!< Mesh Stack Process Gatt Proxy PDU message */
  MESH_MSG_API_ADD_ADV_IF,                      /*!< Mesh Stack Add Advertising Interface */
  MESH_MSG_API_REM_ADV_IF,                      /*!< Mesh Stack Remove Advertising Interface */
  MESH_MSG_API_PROC_ADV_MSG,                    /*!< Mesh Stack Process Advertising PDU message */
  MESH_MSG_API_SGN_ADV_IF_RDY,                  /*!< Mesh Stack Signal Advertising Interface Ready */
  MESH_MSG_API_PROXY_CFG_REQ,                   /*!< Mesh Proxy Configuration request. */
  MESH_MSG_API_ATT_SET,                         /*!< Mesh Attention Timer Set request. */
  MESH_MSG_API_SGN_GATT_IF_RDY,                 /*!< Mesh Stack Signal GATT Proxy Interface Ready */
  MESH_MSG_API_SEND_DELAY_ELAPSED,              /*!< Mesh Stack Send Message Delay Timer elapsed */
};

/*! MeshSendMessage() API internal message type */
typedef struct meshSendMessage_tag
{
  wsfMsgHdr_t             hdr;             /*!< Header structure */
  wsfTimer_t              delayTmr;        /*!< Delay timer for of random delay send */
  meshMsgInfo_t           msgInfo;         /*!< Pointer to Mesh message identification information */
  uint8_t                 *pMsgParam;      /*!< Pointer to a Mesh message parameter list */
  uint16_t                msgParamLen;     /*!< Length of the message parameter list */
  uint16_t                netKeyIndex;     /*!< Global Network Key identifier. */
} meshSendMessage_t;

/*! MeshPublishMessage() API internal message type */
typedef struct meshPublishMessage_tag
{
  wsfMsgHdr_t      hdr;                 /*!< Header structure */
  meshPubMsgInfo_t pubMsgInfo;          /*!< Published message identification data */
  uint8_t          *pMsgParam;          /*!< Pointer to a Mesh message parameter list */
  uint16_t         msgParamLen;         /*!< Length of the message parameter list */
} meshPublishMessage_t;

/*! MeshAddGattProxyConnection() API internal message type */
typedef struct meshAddGattProxyConn_tag
{
  wsfMsgHdr_t                    hdr;          /*!< Header structure */
  meshGattProxyConnId_t          connId;       /*!< Unique identifier for the connection */
  uint16_t                       maxProxyPdu;  /*!< Maximum size of the Proxy PDU */
} meshAddGattProxyConn_t;

/*! MeshRemoveGattProxyConnection() API internal message type */
typedef struct meshRemoveGattProxyConn_tag
{
  wsfMsgHdr_t            hdr;      /*!< Header structure */
  meshGattProxyConnId_t  connId;   /*!< Unique identifier for the connection */
} meshRemoveGattProxyConn_t;

/*! MeshProcessGattProxyPdu() API internal message type */
typedef struct meshProcessGattProxyPdu_tag
{
  wsfMsgHdr_t            hdr;          /*!< Header structure */
  meshGattProxyConnId_t  connId;       /*!< Unique identifier for the connection */
  const uint8_t          *pProxyPdu;   /*!< Pointer to a buffer containing the GATT Proxy PDU */
  uint16_t               proxyPduLen;  /*!< Size of the buffer referenced by pProxyPdu */
} meshProcessGattProxyPdu_t;

/*! MeshSignalGattProxyIfReady() API internal message type */
typedef struct meshSignalGattProxyIfRdy_tag
{
  wsfMsgHdr_t            hdr;       /*!< Header structure */
  meshGattProxyConnId_t  connId;    /*!< Unique identifier for the connection */
} meshSignalGattProxyIfRdy_t;

/*! MeshSendProxyConfig() API internal message type */
typedef struct meshSendProxyConfig_tag
{
  wsfMsgHdr_t             hdr;          /*!< Header structure */
  meshGattProxyConnId_t   connId;       /*!< Unique identifier for the connection */
  uint16_t                netKeyIndex;  /*!< Global Network Key identifier. */
  uint8_t                 opcode;       /*!< Proxy Configuration Opcode */
  uint8_t                 *pProxyPdu;   /*!< Pointer to a buffer containing the GATT Proxy PDU */
  uint16_t                proxyPduLen;  /*!< Size of the buffer referenced by pProxyPdu */
} meshSendProxyConfig_t;

/*! MeshAdvAddInterface() API internal message type */
typedef struct meshAddAdvIf_tag
{
  wsfMsgHdr_t            hdr;      /*!< Header structure */
  meshAdvIfId_t          advIfId;  /*!< Unique identifier for the interface */
} meshAddAdvIf_t;

/*! MeshRemoveAdvInterface() API internal message type */
typedef struct meshRemoveAdvIf_tag
{
  wsfMsgHdr_t         hdr;       /*!< Header structure */
  meshAdvIfId_t       advIfId;   /*!< Unique identifier for the interface */
} meshRemoveAdvIf_t;

/*! MeshProcessAdvPdu() API internal message type */
typedef struct meshProcessAdvPdu_tag
{
  wsfMsgHdr_t        hdr;        /*!< Header structure */
  meshAdvIfId_t      advIfId;    /*!< Unique identifier for the interface */
  uint8_t            *pAdvPdu;   /*!< Pointer to a buffer containing the Advertising PDU */
  uint8_t            advPduLen;  /*!< Size of the advertising PDU */
} meshProcessAdvPdu_t;

/*! MeshSignalAdvInterfaceReady() API internal message type */
typedef struct meshSignalAdvIfRdy_tag
{
  wsfMsgHdr_t        hdr;       /*!< Header structure */
  meshAdvIfId_t      advIfId;   /*!< Unique identifier for the interface */
} meshSignalAdvIfRdy_t;

/*! MeshAttentionSet() API internal message type */
typedef struct meshAttentionSet_tag
{
  wsfMsgHdr_t        hdr;         /*!< Header structure */
  meshElementId_t    elemId;      /*!< Element identifier */
  uint8_t            attTimeSec;  /*!< Unique identifier for the interface */
} meshAttentionSet_t;

/*! Mesh WSF message handling function type */
typedef void (*meshWsfMsgHandlerCback_t)(wsfMsgHdr_t *pMsg);

/*! Mesh Stack control block */
typedef struct meshCb_tag
{
  meshCback_t                evtCback;            /*!< Mesh Stack event notification callback */
  meshWsfMsgHandlerCback_t   apiMsgCback;         /*!< Mesh API WSF message callback */
  meshWsfMsgHandlerCback_t   friendshipMsgCback;  /*!< Mesh Friendship WSF message callback */
  meshWsfMsgHandlerCback_t   cfgMdlClMsgCback;    /*!< Mesh Config Client WSF message callback */
  meshWsfMsgHandlerCback_t   accMsgCback;         /*!< Mesh Access Layer WSF message callback */
  meshWsfMsgHandlerCback_t   hbMsgCback;          /*!< Mesh Hearbeat WSF message callback */
  meshWsfMsgHandlerCback_t   sarRxMsgCback;       /*!< Mesh SAR Rx WSF message callback */
  meshWsfMsgHandlerCback_t   sarTxMsgCback;       /*!< Mesh SAR Tx WSF message callback */
  meshWsfMsgHandlerCback_t   nwkMsgCback;         /*!< Mesh Network WSF message callback */
  meshWsfMsgHandlerCback_t   nwkMgmtMsgCback;     /*!< Mesh Nwk Management WSF message callback */
  meshWsfMsgHandlerCback_t   nwkBeaconMsgCback;   /*!< Mesh Network Beacon WSF message callback */
  meshWsfMsgHandlerCback_t   prvBeaconMsgCback;   /*!< Mesh Provisioning Beacon WSF message
                                                   *   callback
                                                   */
  meshWsfMsgHandlerCback_t   prvBrMsgCback;       /*!< Mesh Provisioning Bearer WSF message callback */
  meshWsfMsgHandlerCback_t   gattProxyMsgCback;   /*!< Mesh GATT Proxy WSF message callback */
  meshWsfMsgHandlerCback_t   localCfgMsgCback;    /*!< Mesh Local Config WSF message callback */
  wsfHandlerId_t             handlerId;           /*!< WSF handler ID */
  bool_t                     initialized;         /*!< Mesh Stack initialization flag */
  bool_t                     proxyIsServer;       /*!< Node is Proxy Server flag */
  uint8_t                    *pMemBuff;           /*!< Pointer to memory buffer required by stack
                                                   */
  uint32_t                   memBuffSize;         /*!< Memory buffer size */
} meshCb_t;

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! Mesh control block */
extern meshCb_t meshCb;

#ifdef __cplusplus
}
#endif

#endif /* MESH_MAIN_H */
