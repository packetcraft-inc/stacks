/*************************************************************************************************/
/*!
 *  \file   mesh_api.h
 *
 *  \brief  Main stack API.
 *
 *  Copyright (c) 2010-2019 Arm Ltd. All Rights Reserved.
 *
 *  Copyright (c) 2019-2020 Packetcraft, Inc.
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
 * @addtogroup MESH_CORE_API Mesh Core API
 * @{
 *************************************************************************************************/

#ifndef MESH_API_H
#define MESH_API_H

#include "mesh_defs.h"
#include "mesh_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief Check if the Advertising Interface ID value is valid */
#define MESH_ADV_IF_ID_IS_VALID(advIfId)           ((advIfId) <= 0x1F)

/*! \brief Check if the Proxy Connection ID value is valid */
#define MESH_GATT_PROXY_CONN_ID_IS_VALID(connId)   ((connId) <= 0x1F)

/*! \brief Value to set in order to use the Default TTL */
#define MESH_USE_DEFAULT_TTL                       0xFF

/*! \brief Value returned by MeshGetRequiredMemory if the mesh configuration has invalid parameters. */
#define MESH_MEM_REQ_INVALID_CFG                   0xFFFFFFFF

/*! \brief Mesh callback events */
#define MESH_CBACK_START                           0xA0  /*! Mesh callback event starting value */

/*! \brief The model shares the subscription list from a root model */
#define MMDL_SUBSCR_LIST_SHARED                    0xFF

/*! \brief Mesh events */
enum meshEvents
{
  MESH_CORE_EVENT = MESH_CBACK_START,      /*!< Mesh Core event */
  MESH_CFG_MDL_CL_EVENT,                   /*!< Mesh Configuration Client event */
  MESH_CFG_MDL_SR_EVENT,                   /*!< Mesh Configuration Server event */
  MESH_LPN_EVENT,                          /*!< Mesh LPN event */
  MESH_PRV_CL_EVENT,                       /*!< Mesh Provisioning Client event */
  MESH_PRV_SR_EVENT,                       /*!< Mesh Provisioning Server event */
  MESH_TEST_EVENT                          /*!< Mesh Test event */
};

#define MESH_CBACK_END                             MESH_TEST_EVENT  /*! Mesh callback event ending value */

/*! \brief Mesh callback events */
enum meshEventValues
{
  MESH_CORE_RESET_EVENT,                   /*!< Mesh reset event */
  MESH_CORE_ERROR_EVENT,                   /*!< Mesh internal error event */
  MESH_CORE_SEND_MSG_EVENT,                /*!< Mesh send message event */
  MESH_CORE_PUBLISH_MSG_EVENT,             /*!< Mesh publish message event */
  MESH_CORE_GATT_CONN_ADD_EVENT,           /*!< Mesh add GATT proxy connection event */
  MESH_CORE_GATT_CONN_REMOVE_EVENT,        /*!< Mesh remove GATT proxy connection event */
  MESH_CORE_GATT_CONN_CLOSE_EVENT,         /*!< Mesh GATT proxy connection closed event */
  MESH_CORE_GATT_PROCESS_PROXY_PDU_EVENT,  /*!< Mesh process GATT proxy PDU event */
  MESH_CORE_GATT_SIGNAL_IF_RDY_EVENT,      /*!< Mesh signal GATT interface ready for TX event */
  MESH_CORE_ADV_IF_ADD_EVENT,              /*!< Mesh add ADV interface event */
  MESH_CORE_ADV_IF_REMOVE_EVENT,           /*!< Mesh remove ADV interface event */
  MESH_CORE_ADV_IF_CLOSE_EVENT,            /*!< Mesh ADV interface closed event */
  MESH_CORE_ADV_PROCESS_PDU_EVENT,         /*!< Mesh process ADV PDU event */
  MESH_CORE_ADV_SIGNAL_IF_RDY_EVENT,       /*!< Mesh signal ADV interface ready event */
  MESH_CORE_ATTENTION_SET_EVENT,           /*!< Mesh element attention set event */
  MESH_CORE_ATTENTION_CHG_EVENT,           /*!< Mesh element attention state changed event */
  MESH_CORE_NODE_STARTED_EVENT,            /*!< Mesh Node started */
  MESH_CORE_PROXY_SERVICE_DATA_EVENT,      /*!< Proxy Service Data for connectable advertising data event */
  MESH_CORE_PROXY_FILTER_STATUS_EVENT,     /*!< Mesh Proxy Configuration Filter Status  event */
  MESH_CORE_IV_UPDATED_EVENT,              /*!< Mesh IV updated */
  MESH_CORE_HB_INFO_EVENT                  /*!< Mesh Heartbeat information received */
};

/*! \brief Mesh callback events end */
#define MESH_CORE_MAX_EVENT                MESH_CORE_HB_INFO_EVENT  /*! Mesh callback event ending value */


/*! \brief Initializer of a message info for the specified model ID and opcode */
#define MESH_MSG_INFO(modelId, opcode)     {{(meshSigModelId_t)(modelId) }, \
                                            {{UINT16_TO_BE_BYTES(opcode)}}, 0xFF, NULL, \
                                            MESH_ADDR_TYPE_UNASSIGNED, 0xFF, 0xFF}

/*! \brief Initializer of a publish message info for the specified model ID and opcode */
#define MESH_PUB_MSG_INFO(modelId, opcode) {{{UINT16_TO_BE_BYTES(opcode)}}, 0xFF, \
                                            {(meshSigModelId_t)(modelId) }}

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Mesh Model event type enumeration */
enum meshModelEventValues
{
  MESH_MODEL_EVT_MSG_RECV = 0, /*!< Mesh Model message received event */
  MESH_MODEL_EVT_PERIODIC_PUB, /*!< Mesh Model periodic publish time expired event */
};

/*! \brief Mesh Model event for ::MESH_MODEL_EVT_MSG_RECV */
typedef struct meshModelMsgRecvEvt_tag
{
  wsfMsgHdr_t          hdr;               /*!< Header structure */
  meshElementId_t      elementId;         /*!< Identifier of the element which received the message */
  meshAddress_t        srcAddr;           /*!< Address off the element that sent the message */
  uint8_t              ttl;               /*!< TTL of the received message. */
  bool_t               recvOnUnicast;     /*!< Indicates if message was received on unicast */
  meshMsgOpcode_t      opCode;            /*!< Opcode of the message */
  uint8_t              *pMessageParams;   /*!< Pointer to the message parameters */
  uint16_t             messageParamsLen;  /*!< Message parameters data length */
  uint16_t             appKeyIndex;       /*!< Global Application Key identifier. */
  modelId_t            modelId;           /*!< Model identifier. */
} meshModelMsgRecvEvt_t;

/*! \brief Mesh Model event for ::MESH_MODEL_EVT_PERIODIC_PUB. */
typedef struct meshModelPeriodicPubEvt_tag
{
  wsfMsgHdr_t          hdr;               /*!< Header structure */
  meshElementId_t      elementId;         /*!< Identifier of the element which received the message
                                           */
  uint32_t             nextPubTimeMs;     /*!< Next publication time in ms */
  modelId_t            modelId;           /*!< Model identifier. */
  bool_t               isVendorModel;     /*!< Vendor Model identifier */
} meshModelPeriodicPubEvt_t;

/*! \brief Mesh Model event union. */
typedef union meshModelEvt_tag
{
  meshModelMsgRecvEvt_t     msgRecvEvt;     /*!< Mesh Model message received event */
  meshModelPeriodicPubEvt_t periodicPubEvt; /*!< Mesh Model periodic publish timer expired event */
} meshModelEvt_t;

/*! \brief Mesh model subscription list link */
typedef struct meshModelLink_tag
{
  modelId_t            rootModelId;           /*!< Identifier of the model which shares the subscription list */
  meshElementId_t      rootElementId;         /*!< Identifier of the element which shares the subscription list */
  bool_t               isSig;                 /*!< TRUE if model identifier is SIG, FALSE for vendor */
}meshModelLink_t;

/*! \brief Mesh SIG model definition */
typedef struct meshSigModel_tag
{
  void                   *pModelDescriptor;   /*!< Pointer to the model descriptor */
  wsfHandlerId_t         *pHandlerId;         /*!< Pointer to Model WSF handler ID */
  const meshMsgOpcode_t  *pRcvdOpcodeArray;   /*!< Pointer to array of supported received
                                               *   SIG opcodes
                                               */
 const  meshModelLink_t  *pModelLink;         /*!< Pointer to the model link descriptor */
  meshSigModelId_t       modelId;             /*!< Model ID, as assigned by the SIG */
  uint8_t                opcodeCount;         /*!< Number of SIG defined opcodes
                                               *   supported
                                               */
  uint8_t                subscrListSize;      /*!< Subscription list size */
  uint8_t                appKeyBindListSize;  /*!< AppKey to Model bind list size */
} meshSigModel_t;

/*! \brief Mesh Vendor model definition */
typedef struct meshVendorModel_tag
{
  void                      *pModelDescriptor;   /*!< Pointer to the model descriptor */
  wsfHandlerId_t            *pHandlerId;         /*!< Pointer to Model WSF handler ID */
  const meshMsgOpcode_t     *pRcvdOpcodeArray;   /*!< Pointer to array of supported received
                                                  *   SIG opcodes
                                                  */
  meshModelLink_t           *pModelLink;         /*!< Pointer to the model link descriptor */
  meshVendorModelId_t       modelId;             /*!< Model ID, as assigned by vendor */
  uint8_t                   opcodeCount;         /*!< Number of SIG defined opcodes supported */
  uint8_t                   subscrListSize;      /*!< Subscription list size */
  uint8_t                   appKeyBindListSize;  /*!< AppKey to Model bind list size */
} meshVendorModel_t;

/*!
 *  Mesh element definition
 *
 *  Usage:
 *  \code
 *  const meshSigModel_t elem0SigModels[2] = {{.modelId = 0xaaaa}, {.modelId = 0xbbbb}};
 *
 *  meshElement_t firstElement =
 *  {
 *    .numSigModels         = 2,
 *    .numVendorModels      = 0,
 *    .pSigModelArray       = elem0SigModels,
 *    .pVendorModedArray    = NULL
 *  };
 *  \endcode
 */
typedef struct meshElement_tag
{
  uint16_t                locationDescriptor;  /*!< Location descriptor as defined
                                                *   in the GATT Bluetooth Namespace
                                                */
  uint8_t                 numSigModels;        /*!< Number of SIG models in this element */
  uint8_t                 numVendorModels;     /*!< Number of Vendor models in this element */
  const meshSigModel_t    *pSigModelArray;     /*!< Pointer to array containing numSigModels
                                                *   number of SIG models in this element
                                                */
  const meshVendorModel_t *pVendorModelArray;  /*!< Pointer to array containing numVendorModels
                                                *   number of vendor models for this element
                                                */
} meshElement_t;

/*! \brief Bitfield data type for optional features that use configuration memory */
typedef uint8_t meshMemConfigOptFeat_t;

/*! \brief Mesh configuration memory descriptor. */
typedef struct meshMemoryConfig_tag
{
  uint16_t               addrListMaxSize;          /*!< Maximum number of non-virtual addresses
                                                    *   stored
                                                    */
  uint16_t               virtualAddrListMaxSize;   /*!< Maximum number of virtual addresses stored
                                                    */
  uint16_t               appKeyListSize;           /*!< Maximum number of AppKeys stored */
  uint16_t               netKeyListSize;           /*!< Maximum number of NetKeys stored */
  uint8_t                nwkCacheL1Size;           /*!< Maximum number of elements in Level 1
                                                    *   Network Cache
                                                    */
  uint16_t               rpListSize;               /*!< Maximum number of elements in Replay
                                                    *   Protection List
                                                    */
  uint8_t                nwkCacheL2Size;           /*!< Maximum number of elements in Level 2
                                                    *   Network
                                                    */
  uint8_t                nwkOutputFilterSize;      /*!< Maximum number of element addresses in the
                                                    *   output filter of a network interface
                                                    */
  uint8_t                sarRxTranHistorySize;     /*!< Maximum number of elements in SAR Rx
                                                    *   Transaction History
                                                    */
  uint8_t                sarRxTranInfoSize;        /*!< Maximum number of elements in SAR Rx
                                                    *   Transaction Info Table */
  uint8_t                sarTxMaxTransactions;     /*!< Maximum number of SAR TX transactions */
  uint16_t               cfgMdlClMaxSrSupported;   /*!< Maximum number of Configuration Servers
                                                    *   supported simultaneously by the Configuration
                                                    *   Client
                                                    */
  uint8_t                maxNumFriendships;        /*!< Maximum number of friendships this node can
                                                    *   establish
                                                    */
  uint8_t                maxNumFriendQueueEntries; /*!< Maximum number of entries for a
                                                    *   friend queue
                                                    */
  uint8_t                maxFriendSubscrListSize;  /*!< Maximum number of subscription addresses
                                                    *   for a friendship
                                                    */
} meshMemoryConfig_t;

/*! \brief Mesh NVM configuration descriptor. */
typedef struct meshNvmConfig_tag
{
  uint8_t    instanceId;     /*!< Mesh NVM instance ID */
  uint32_t   startAddress;      /*!< Start address in persistent memory */
  uint32_t   endAddress;        /*!< End address in persistent memory */
} meshNvmConfig_t;

/*! \brief Mesh Stack initial configuration structure */
typedef struct meshConfig_tag
{
  const meshElement_t *pElementArray;    /*!< Pointer to array describing elements
                                         *   present in the node
                                         */
  uint8_t             elementArrayLen;   /*!< Length of the element array */
  meshMemoryConfig_t  *pMemoryConfig;    /*!< Memory configuration for internal storage */
} meshConfig_t;

/*! Message identifier structure
 *
 *  \brief Contains information that identifies a Mesh message
 */
typedef struct meshMsgInfo_tag
{
  modelId_t           modelId;         /*!< Model identifier. */
  meshMsgOpcode_t     opcode;          /*!< Message operation code */
  meshElementId_t     elementId;       /*!< Identifier of the originating element */
  uint8_t             *pDstLabelUuid;  /*!< Label UUID pointer for destination virtual address */
  meshAddress_t       dstAddr;         /*!< Message destination address */
  uint16_t            appKeyIndex;     /*!< Global identifier of the Application Key. */
  uint8_t             ttl;             /*!< Initial TTL of the message, or ::MESH_USE_DEFAULT_TTL */
} meshMsgInfo_t;

/*! Published message identifier structure
 *
 *  \brief Contains information that identifies a published Mesh message
 */
typedef struct meshPubMsgInfo_tag
{
  meshMsgOpcode_t       opcode;              /*!< Message operation code */
  meshElementId_t       elementId;           /*!< Identifier of the originating element */
  modelId_t             modelId;             /*!< Model identifier. */
} meshPubMsgInfo_t;

/*! \brief Mesh GATT Proxy connection identifier */
typedef uint8_t  meshGattProxyConnId_t;

/*! \brief Mesh Proxy interface filter type. See ::meshProxyFilterType */
typedef uint8_t meshProxyFilterType_t;

/*! \brief Mesh Proxy Service identification type. See ::meshProxyIdType */
typedef uint8_t meshProxyIdType_t;

/*! \brief Mesh Advertising interface */
typedef uint8_t  meshAdvIfId_t;

/*! GATT Proxy connection event type for ::MESH_CORE_GATT_CONN_ADD_EVENT, ::MESH_CORE_GATT_CONN_REMOVE_EVENT,
 *  ::MESH_CORE_GATT_CONN_CLOSE_EVENT, ::MESH_CORE_GATT_PROCESS_PROXY_PDU_EVENT, ::MESH_CORE_GATT_SIGNAL_IF_RDY_EVENT
 *
 *  \brief GATT Proxy connection event type.
 */
typedef struct meshGattConnEvt_tag
{
  wsfMsgHdr_t             hdr;     /*!< Header structure */
  meshGattProxyConnId_t   connId;  /*!< Connection identifier */
} meshGattConnEvt_t;

/*! ADV interface event type for ::MESH_CORE_ADV_IF_ADD_EVENT, ::MESH_CORE_ADV_IF_REMOVE_EVENT,
 *  ::MESH_CORE_ADV_IF_CLOSE_EVENT, ::MESH_CORE_ADV_PROCESS_PDU_EVENT, ::MESH_CORE_ADV_SIGNAL_IF_RDY_EVENT
 *
 *  \brief ADV interface event type.
 */
typedef struct meshAdvIfEvt_tag
{
  wsfMsgHdr_t     hdr;    /*!< Header structure. Used with MESH_CORE_ADV_EVENT* events */
  meshAdvIfId_t   ifId;   /*!< ADV Interface identifier */
} meshAdvIfEvt_t;

/*! \brief Attention event type for ::MESH_CORE_ATTENTION_CHG_EVENT */
typedef struct meshAttentionEvt_tag
{
  wsfMsgHdr_t       hdr;          /*!< Header structure */
  meshElementId_t   elementId;    /*!< Element identifier */
  bool_t            attentionOn;  /*!< New attention state */
} meshAttentionEvt_t;

/*! \brief Mesh event type for ::MESH_CORE_NODE_STARTED_EVENT */
typedef struct meshNodeStartedEvt_tag
{
  wsfMsgHdr_t     hdr;       /*!< Header structure */
  meshAddress_t   address;   /*!< Primary element address */
  uint8_t         elemCnt;   /*!< Number of elements */
} meshNodeStartedEvt_t;

/*! \brief Proxy Service Data event type for ::MESH_CORE_PROXY_SERVICE_DATA_EVENT */
typedef struct meshProxyServiceDataEvt_tag
{
  wsfMsgHdr_t       hdr;                                                /*!< Header structure */
  uint8_t           serviceData[MESH_PROXY_NODE_ID_SERVICE_DATA_SIZE];  /*!< Service data */
  uint8_t           serviceDataLen;                                     /*!< Service data length*/
} meshProxyServiceDataEvt_t;

/*! \brief Proxy Filter Status event type for ::MESH_CORE_PROXY_FILTER_STATUS_EVENT */
typedef struct meshProxyFilterStatusEvt_tag
{
  wsfMsgHdr_t             hdr;        /*!< Header structure */
  meshGattProxyConnId_t   connId;     /*!< Connection identifier */
  meshProxyFilterType_t   filterType; /*!< Proxy filter type */
  uint16_t                listSize;   /*!< Proxy filter list size */
} meshProxyFilterStatusEvt_t;

/*! \brief Mesh IV updated event type for ::MESH_CORE_IV_UPDATED_EVENT */
typedef struct meshIvUpdtEvt_tag
{
  wsfMsgHdr_t       hdr;              /*!< Header structure */
  uint32_t          ivIndex;          /*!< IV index */
} meshIvUpdtEvt_t;

/*! \brief Received Heartbeat info event type for ::MESH_CORE_HB_INFO_EVENT */
typedef struct meshHbInfoEvt_tag
{
  wsfMsgHdr_t       hdr;              /*!< Header structure */
  meshAddress_t     src;              /*!< SRC address */
  meshFeatures_t    features;         /*!< Received features bitmask */
  uint8_t           hops;             /*!< Hops taken by the Heartbeat message to reach the node */
  uint8_t           minHops;          /*!< Computed Minimum Hops value */
  uint8_t           maxHops;          /*!< Computed Maximum Hops value */
} meshHbInfoEvt_t;

/*! \brief Union of all Mesh Stack events */
typedef union meshEvt_tag
{
  wsfMsgHdr_t                 hdr;         /*!< Generic WSF header. Used for the following events:
                                            *   ::MESH_CORE_RESET_EVENT
                                            *   ::MESH_CORE_ERROR_EVENT
                                            *   ::MESH_CORE_SEND_MSG_EVENT
                                            *   ::MESH_CORE_PUBLISH_MSG_EVENT
                                            */
  meshAdvIfEvt_t              advIf;       /*!< Advertising interface API event data.
                                            *   See ::meshAdvIfEvt_t
                                            */
  meshGattConnEvt_t           gattConn;    /*!< GATT connection API event data. See ::meshGattConnEvt_t */
  meshAttentionEvt_t          attention;   /*!< Attention event data. See ::meshAttentionEvt_t */
  meshNodeStartedEvt_t        nodeStarted; /*!< Node Started event data */
  meshProxyServiceDataEvt_t   serviceData; /*!< Proxy service data. See ::meshProxyServiceDataEvt_t */
  meshProxyFilterStatusEvt_t  filterStatus;/*!< Proxy filter status event. See ::meshProxyFilterStatusEvt_t*/
  meshIvUpdtEvt_t             ivUpdt;      /*!< IV updated event */
  meshHbInfoEvt_t             hbInfo;      /*!< Hearbeat information event. See::meshHbInfoEvt_t */
} meshEvt_t;

/*! \brief Mesh Stack event notification callback */
typedef void(*meshCback_t) (meshEvt_t *pEvt);

/*! \brief GATT proxy PDU send callback event */
enum MeshGattProxyPduSendEvent
{
  MESH_GATT_PROXY_PDU_SEND = 0, /*!< GATT Proxy PDU send */
};

/*! \brief GATT Proxy PDU received event type */
typedef struct meshGattProxyPduSendEvt_tag
{
  wsfMsgHdr_t            hdr;          /*!< Header structure */
  const uint8_t          *pProxyPdu;   /*!< Pointer to a buffer containing a Proxy PDU */
  uint16_t               proxyPduLen;  /*!< Length of the Proxy PDU (incl. header) message */
  uint8_t                proxyHdr;     /*!< Header value for a Proxy PDU */
  meshGattProxyConnId_t  connId;       /*!< Connection identifier */
} meshGattProxyPduSendEvt_t;

/*! \brief Advertising interface PDU send callback event */
enum meshAdvIfPduSendEvent
{
  MESH_CORE_ADV_PDU_SEND_EVENT = 0 /*!< ADV PDU send */
};

/*! \brief ADV interface PDU send event type */
typedef struct meshAdvPduSendEvt_tag
{
  wsfMsgHdr_t     hdr;        /*!< Header structure */
  meshAdvIfId_t   ifId;       /*!< ADV Interface identifier */
  uint8_t         adType;     /*!< AD type. */
  const uint8_t   *pAdvPdu;   /*!< Pointer to a buffer containing an ADV PDU */
  uint8_t         advPduLen;  /*!< Length of the advertising PDU */
} meshAdvPduSendEvt_t;


/*! \brief Provisioning data flags */
typedef uint8_t meshPrvFlags_t;

/*! \brief Provisioning data type */
typedef struct meshPrvData_tag
{
  uint8_t         *pDevKey;                   /*!< Pointer to DevKey array (not mandatory) */
  uint8_t         *pNetKey;                   /*!< Pointer to NetKey array */
  uint32_t        ivIndex;                    /*!< Current value of the IV Index */
  uint16_t        netKeyIndex;                /*!< Global identifier of the Network Key */
  meshAddress_t   primaryElementAddr;         /*!< Unicast address of the primary element */
  meshPrvFlags_t  flags;                      /*!< Provisioning flags bitmask */
} meshPrvData_t;

/*! \brief Callback invoked by the Mesh Stack when it needs to send a Proxy PDU */
typedef void (*meshGattProxyPduSendCback_t) (meshGattProxyPduSendEvt_t *pEvt);

/*! \brief Callback invoked by the Mesh Stack when it needs to send an advertising PDU */
typedef void (*meshAdvPduSendCback_t) (meshAdvPduSendEvt_t *pEvt);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! \brief Mesh configuration structure */
extern meshConfig_t *pMeshConfig;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Computes the required memory to be provided based on the given configuration.
 *
 *  \return Memory required value in bytes if success or ::MESH_MEM_REQ_INVALID_CFG in case fail.
 */
/*************************************************************************************************/
uint32_t MeshGetRequiredMemory(void);

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh Core Stack.
 *
 *  \param[in] pFreeMem     Pointer to free memory.
 *  \param[in] freeMemSize  Size of pFreeMem.
 *
 *  \return    Amount of free memory consumed.
 */
/*************************************************************************************************/
uint32_t MeshInit(uint8_t *pFreeMem, uint32_t freeMemSize);

/*************************************************************************************************/
/*!
*  \brief     Registers the Mesh Core Stack events callback.
*
*  \param[in] meshCback  Callback function for Mesh Stack events.
*
*  \return    None.
*/
/*************************************************************************************************/
void MeshRegister(meshCback_t meshCback);

/*************************************************************************************************/
/*!
 *  \brief  Resets the node to unprovisioned device state.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshFactoryReset(void);

/*************************************************************************************************/
/*!
 *  \brief  Checks if a device is provisioned.
 *
 *  \return TRUE if device is provisioned. FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshIsProvisioned(void);

/*************************************************************************************************/
/*!
 *  \brief  Starts a device as node. The device needs to be already provisioned.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshStartNode(void);

/*************************************************************************************************/
/*!
 *  \brief     Sends a Mesh message to a destination address.
 *
 *  \param[in] pMsgInfo       Pointer to a Mesh message information structure.
 *  \param[in] pMsgParam      Pointer to a Mesh message parameter list.
 *  \param[in] msgParamLen    Length of the message parameter list.
 *  \param[in] rndDelayMsMin  Minimum value for random send delay in ms. 0 for instant send
 *  \param[in] rndDelayMsMax  Maximum value for random send delay in ms. 0 for instant send.
 *
 *  \return    None.
 *
 */
/*************************************************************************************************/
void MeshSendMessage(const meshMsgInfo_t *pMsgInfo, const uint8_t *pMsgParam, uint16_t msgParamLen,
                    uint32_t rndDelayMsMin, uint32_t rndDelayMsMax);

/*************************************************************************************************/
/*!
 *  \brief     Publishes a Mesh Message based on internal Model Publication State configuration.
 *
 *  \param[in] pPubMsgInfo  Pointer to a Mesh publish message information structure.
 *  \param[in] pMsgParam    Pointer to a Mesh message parameter list.
 *  \param[in] msgParamLen  Length of the message parameter list.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPublishMessage(meshPubMsgInfo_t *pPubMsgInfo,const uint8_t *pMsgParam, uint16_t msgParamLen);

/*************************************************************************************************/
/*!
*  \brief  Initializes the GATT proxy functionality.
*
*  \return None.
*/
/*************************************************************************************************/
void MeshGattProxyInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Registers GATT proxy callback for PDU's sent by the stack to the bearer.
 *
 *  \param[in] cback  Callback used by the Mesh Stack to send a GATT Proxy PDU.
 *
 *  \return    None.
 *
 *  \remarks   If GATT Proxy is supported and this the first connection, it also enables proxy.
 */
/*************************************************************************************************/
void MeshRegisterGattProxyPduSendCback(meshGattProxyPduSendCback_t cback);

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
void MeshAddGattProxyConn(meshGattProxyConnId_t connId, uint16_t maxProxyPdu);

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
void MeshRemoveGattProxyConn(meshGattProxyConnId_t connId);

/*************************************************************************************************/
/*!
 *  \brief  Checks if GATT Proxy Feature is enabled.
 *
 *  \return TRUE if GATT Proxy Feature is enabled. FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshIsGattProxyEnabled(void);

/*************************************************************************************************/
/*!
 *  \brief     Sends a GATT Proxy PDU to the Mesh Stack for processing.
 *
 *  \param[in] connId       Unique identifier for the connection on which the PDU was received.
 *                          Valid range is 0x00 to 0x1F.
 *  \param[in] pProxyPdu    Pointer to a buffer containing the GATT Proxy PDU.
 *  \param[in] proxyPduLen  Size of the buffer referenced by pProxyPdu.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshProcessGattProxyPdu(meshGattProxyConnId_t connId, const uint8_t *pProxyPdu,
                             uint16_t proxyPduLen);

/*************************************************************************************************/
/*!
 *  \brief     Signals the Mesh Stack that the GATT Proxy interface is ready to transmit packets.
 *
 *  \param[in] connId  Unique identifier for the connection on which the PDU was received.
 *                     Valid range is 0x00 to 0x1F.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSignalGattProxyIfRdy(meshGattProxyConnId_t connId);

/*************************************************************************************************/
/*!
 *  \brief     Sends a Set Filter Type configuration message to a Proxy Server.
 *
 *  \param[in] connId       Unique identifier for the connection on which the PDU was received.
 *                          Valid range is 0x00 to 0x1F.
 *  \param[in] netKeyIndex  Global Network Key identifier.
 *  \param[in] filterType   Proxy filter type. See ::meshProxyFilterType.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshProxyClSetFilterType(meshGattProxyConnId_t connId, uint16_t netKeyIndex,
                              meshProxyFilterType_t filterType);

/*************************************************************************************************/
/*!
 *  \brief     Sends an Add Addresses to Filter configuration message to a Proxy Server.
 *
 *  \param[in] connId       Unique identifier for the connection on which the PDU was received.
 *                          Valid range is 0x00 to 0x1F.
 *  \param[in] netKeyIndex  Global Network Key identifier.
 *  \param[in] pAddrArray   Pointer to a buffer containing the addresses.
 *  \param[in] listSize     Address list size.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshProxyClAddToFilter(meshGattProxyConnId_t connId, uint16_t netKeyIndex,
                            meshAddress_t *pAddrArray, uint16_t listSize);

/*************************************************************************************************/
/*!
 *  \brief     Sends a Remove Addresses from Filter configuration message to a Proxy Server.
 *
 *  \param[in] connId       Unique identifier for the connection on which the PDU was received.
 *                          Valid range is 0x00 to 0x1F.
 *  \param[in] netKeyIndex  Global Network Key identifier.
 *  \param[in] pAddrArray   Pointer to a buffer containing the addresses.
 *  \param[in] listSize     Address list size.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshProxyClRemoveFromFilter(meshGattProxyConnId_t connId, uint16_t netKeyIndex,
                                 meshAddress_t *pAddrArray, uint16_t listSize);

/*************************************************************************************************/
/*!
 *  \brief     Registers advertising interface callback for PDU's sent by the stack to the bearer.
 *
 *  \param[in] cback  Callback used by the Mesh Stack to send an advertising PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshRegisterAdvIfPduSendCback(meshAdvPduSendCback_t cback);

/*************************************************************************************************/
/*!
 *  \brief     Adds a new advertising interface into the bearer.
 *
 *  \param[in] advIfId  Unique identifier for the interface. Valid range is 0x00 to 0x1F.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAddAdvIf(meshAdvIfId_t advIfId);

/*************************************************************************************************/
/*!
 *  \brief     Removes an advertising interface from the bearer.
 *
 *  \param[in] advIfId  Unique identifier for the interface. Valid range is 0x00 to 0x1F.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshRemoveAdvIf(meshAdvIfId_t advIfId);

/*************************************************************************************************/
/*!
 *  \brief     Sends an Advertising PDU to the Mesh Stack for processing.
 *
 *  \param[in] advIfId    Unique identifier for the interface on which the PDU was received.
 *                        Valid range is 0x00 to 0x1F.
 *  \param[in] pAdvPdu    Pointer to a buffer containing the Advertising PDU.
 *  \param[in] advPduLen  Size of the advertising PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshProcessAdvPdu(meshAdvIfId_t advIfId, const uint8_t *pAdvPdu, uint8_t advPduLen);

/*************************************************************************************************/
/*!
 *  \brief     Signals the Mesh Stack that the advertising interface is ready to transmit packets.
 *
 *  \param[in] advIfId  Unique identifier for the connection.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSignalAdvIfRdy(meshAdvIfId_t advIfId);

/*************************************************************************************************/
/*!
 *  \brief     Sets the provisioning and configuration data, either as a result of a completed
 *             Provisioning Procedure, or after reading the data from NVM if already provisioned.
 *
 *  \param[in] pPrvData  Pointer to a structure containing provisioning and configuration data.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLoadPrvData(const meshPrvData_t* pPrvData);

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Proxy Server functionality.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshProxySrInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Requests the Proxy service data from the Mesh stack. This is used by the application
 *             to send connectable advertising packets.
 *
 *  \param[in] netKeyIndex  Network key index.
 *  \param[in] idType       Identification type.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshProxySrGetServiceData(uint16_t netKeyIndex, meshProxyIdType_t idType);

/*************************************************************************************************/
/*!
 *  \brief     Requests the next available Proxy service data from the Mesh stack while cycling
 *             through the netkey indexes.This is used by the application to send connectable
 *             advertising packets.
 *
 *  \param[in] idType  Identification type.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshProxySrGetNextServiceData(meshProxyIdType_t idType);

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Proxy Client functionality.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshProxyClInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Sets attention timer for an element.
 *
 *  \param[in] elemId      Element identifier.
 *  \param[in] attTimeSec  Attention timer time in seconds. Set to 0 to disable.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAttentionSet(meshElementId_t elemId, uint8_t attTimeSec);

/*************************************************************************************************/
/*!
 *  \brief     Gets attention timer remaining time in seconds for an element.
 *
 *  \param[in] elemId  Element identifier.
 *
 *  \return    Current Attention Timer time in seconds.
 */
/*************************************************************************************************/
uint8_t MeshAttentionGet(meshElementId_t elemId);

/*************************************************************************************************/
/*!
 *  \brief     Return size of a Mesh callback event.
 *
 *  \param[in] pMeshEvt  Mesh callback event.
 *
 *  \return    Size of Mesh callback event.
 */
/*************************************************************************************************/
uint16_t MeshSizeOfEvt(meshEvt_t *pMeshEvt);

#ifdef __cplusplus
}
#endif

#endif /* MESH_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
