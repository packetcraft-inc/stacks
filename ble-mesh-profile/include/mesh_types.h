/*************************************************************************************************/
/*!
 *  \file   mesh_types.h
 *
 *  \brief  Common type definitions.
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

/*! ***********************************************************************************************
 * @addtogroup MESH_CORE_API
 * @{
 *************************************************************************************************/

#ifndef MESH_TYPES_H
#define MESH_TYPES_H

#include "wsf_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief TRUE if the address is unassigned */
#define MESH_IS_ADDR_UNASSIGNED(addr)              ((addr) == MESH_ADDR_TYPE_UNASSIGNED)

/*! \brief TRUE if the address type is unicast */
#define MESH_IS_ADDR_UNICAST(addr)                 ((((addr) & MESH_ADDR_TYPE_UNICAST_MASK) == 0) \
                                                    && !MESH_IS_ADDR_UNASSIGNED(addr))

/*! \brief TRUE if the address is a fixed group address */
#define MESH_IS_ADDR_FIXED_GROUP(addr)             ((addr) > 0xFFFB)

/*! \brief TRUE if the address is a dynamically assigned group address */
#define MESH_IS_ADDR_DYN_GROUP(addr)               (((addr) >= 0xC000) && ((addr) < 0xFF00))

/*! \brief TRUE if address type is valid group */
#define MESH_IS_ADDR_GROUP(addr)                   (MESH_IS_ADDR_FIXED_GROUP(addr) ||\
                                                    MESH_IS_ADDR_DYN_GROUP(addr))

/*! \brief TRUE if the address is RFU address */
#define MESH_IS_ADDR_RFU(addr)                     (((addr) >= 0xFF00) && ((addr) < 0xFFFC))

/*! \brief Macro to extract the two MSbit's of a 16-bit Mesh address */
#define MESH_ADDR_EXTRACT_TWO_MSBITS(addr)         (((addr) & MESH_ADDR_TYPE_GROUP_VIRTUAL_MASK) \
                                                    >> 14)

/*! \brief TRUE if address type is virtual */
#define MESH_IS_ADDR_VIRTUAL(addr)                 (MESH_ADDR_EXTRACT_TWO_MSBITS(addr) \
                                                    == MESH_ADDR_TYPE_VIRTUAL_MSBITS_VALUE)

/*! Determines if a ::meshMsgOpcode_t message operation code has only one byte
 *  (opcode[0] == 0b0xxxxxxx)
 *
 *  \remarks Do not use directly. Use ::MESH_OPCODE_SIZE instead
 */
#define MESH_OPCODE_IS_SIZE_ONE(opcode)            (((opcode).opcodeBytes[0] & 0x80) == 0)

/*! Extract the two MSBits from a ::meshMsgOpcode_t message operation code that specify
 *  if its size is 2 or 3
 *
 *  \brief opcode[0] is 0b10xxxxxx if opcode size is 2 or 0b11xxxxxx if opcode size is 3
 *
 *  \remarks Do not use directly. Use ::MESH_OPCODE_SIZE instead
 */
#define MESH_OPCODE_MSBITS_TO_SIZE(opcode)         (((opcode).opcodeBytes[0] & 0xC0) >> 6)

/*! \brief Get application opcode size from a ::meshMsgOpcode_t variable */
#define MESH_OPCODE_SIZE(opcode)                   (MESH_OPCODE_IS_SIZE_ONE(opcode) ? 1 : \
                                                    (uint8_t) MESH_OPCODE_MSBITS_TO_SIZE(opcode))

/*! \brief Check if the application opcode is valid */
#define MESH_OPCODE_IS_VALID(opcode)               (!((opcode).opcodeBytes[0] == 0x7F))

/*! \brief Check if a Model Opcode belongs to a Vendor Model */
#define MESH_OPCODE_IS_VENDOR(opcode)              (MESH_OPCODE_SIZE(opcode) == 3)

/*! \brief Check if the Beacon state is valid */
#define MESH_BEACON_STATE_IS_VALID(beacon)         ((beacon == 0) || (beacon == 1))

/*! \brief Check if the TTL value is valid */
#define MESH_TTL_IS_VALID(ttl)                     (!(((ttl) >= 0x80) && ((ttl) < 0xFF)))

/*! \brief Check if the Sequence Number is in valid range */
#define MESH_SEQ_IS_VALID(seqNo)                   ((seqNo) <= MESH_SEQ_MAX_VAL)

/*! \brief Extracts company ID from vendor model ID */
#define MESH_VENDOR_MODEL_ID_TO_COMPANY_ID(modelId)  ((uint16_t)((modelId) >> 16))

/*! \brief Extracts model ID from a vendor model ID */
#define MESH_VENDOR_MODEL_ID_TO_MODEL_ID(modelId)    ((uint16_t)((modelId) & 0xFFFF))

/*! \brief Makes a vendor model ID from a 2-byte company ID and a 2-byte model ID */
#define MESH_VENDOR_MODEL_MK(compId, modelId)        (((meshVendorModelId_t)(compId) << 16) |\
                                                      (modelId))

/*! \brief Makes a vendor opcode from a 6-bit opcode and a 16-bit company ID */
#define MESH_VENDOR_OPCODE_MK(opcode, compId)        { { (opcode | 0xC0), (uint8_t)((compId) & 0xFF),\
                                                       (uint8_t)((compId) >> 16) } }

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Mesh element address type */
typedef uint16_t meshAddress_t;

/*! \brief SIG model identifier definition */
typedef uint16_t meshSigModelId_t;

/*! \brief Vendor model identifier definition */
typedef uint32_t meshVendorModelId_t;

/*! \brief Mesh Element identifier definition */
typedef uint8_t meshElementId_t;

/*! \brief Mesh SEQ number type */
typedef uint32_t meshSeqNumber_t;

/*! \brief Union of SIG and vendor model identifiers */
typedef union modelId_tag
{
  meshSigModelId_t    sigModelId;    /*!< SIG Model identifier */
  meshVendorModelId_t vendorModelId; /*!< Vendor Model identifier */
} modelId_t;

/*! \brief Message operation code structure */
typedef struct meshMsgOpcode_tag
{
  uint8_t opcodeBytes[3];  /*!< Opcode bytes */
} meshMsgOpcode_t;

/*! \brief Data type for storing the product information values */
typedef struct meshProdInfo_tag
{
  uint16_t companyId;  /*!< Company Identifier */
  uint16_t productId;  /*!< Product Identifier */
  uint16_t versionId;  /*!< Version Identifier */
} meshProdInfo_t;

/*! \brief Supported features bit field data type. See ::meshFeaturesBitMaskValues */
typedef uint16_t meshFeatures_t;

/*! Publish period number of steps. Publish period is number of steps * step resolution
 *  \see MESH_PUBLISH_PERIOD_NUM_STEPS_MAX
 *  \see MESH_PUBLISH_PERIOD_DISABLED_NUM_STEPS
 */
typedef uint8_t meshPublishPeriodNumSteps_t;

/*! \brief Publish Period Step Resolution. See ::meshPublishPeriodStepResValues for valid values */
typedef uint8_t meshPublishPeriodStepRes_t;

/*! \brief Publish security credentials. See ::meshPublishCredValues for valid values */
typedef uint8_t meshPublishFriendshipCred_t;

/*! \brief Publish retransmit count. See ::MESH_PUBLISH_RETRANS_COUNT_MAX */
typedef uint8_t meshPublishRetransCount_t;

/*! Number of 50 millisecond steps between retransmissions of published messages.
 *  See ::MESH_PUBLISH_RETRANS_INTVL_STEPS_MAX
 */
typedef uint8_t meshPublishRetransIntvlSteps_t;

/*! \brief Mesh Relay states data type. See ::meshRelayStatesValues */
typedef uint8_t meshRelayStates_t;

/*! \brief Mesh Secure Network Beacon states data type. See ::meshBeaconStatesValues */
typedef uint8_t meshBeaconStates_t;

/*! \brief Mesh GATT Proxy states data type. See ::meshGattProxyStatesValues */
typedef uint8_t meshGattProxyStates_t;

/*! \brief Mesh Node Identity states data type. See ::meshNodeIdentityStatesValues */
typedef uint8_t meshNodeIdentityStates_t;

/*! \brief Mesh Friend states data type. See ::meshFriendStatesValues */
typedef uint8_t meshFriendStates_t;

/*! \brief Mesh Low Power states data type. See ::meshLowPowerStatesValues */
typedef uint8_t meshLowPowerStates_t;

/*! \brief Mesh Key Refresh Phase states data type. See ::meshKeyRefreshStatesValues */
typedef uint8_t meshKeyRefreshStates_t;

/*! \brief Mesh Key Refresh Transition data types. See ::meshKeyRefreshTransValues */
typedef uint8_t meshKeyRefreshTrans_t;

/*! \brief Network Transmit state */
typedef struct meshNwkTransState_tag
{
  uint8_t                   transCount;              /*!< Number of transmissions for each Network
                                                      *   PDU
                                                      */
  uint8_t                   transIntervalSteps10Ms;  /*!< Number of 10-millisecond steps
                                                      *   between transmissions
                                                      */
} meshNwkTransState_t;

/*! \brief Relay Retransmit state */
typedef struct meshRelayRetransState_tag
{
  uint8_t                   retransCount;              /*!< Number of retransmission on advertising
                                                        *   bearer for each Network PDU
                                                        */
  uint8_t                   retransIntervalSteps10Ms;  /*!< Number of 10-millisecond steps
                                                        *   between retransmissions
                                                        */
} meshRelayRetransState_t;

/*! \brief Structure to store Model Publication state informations */
typedef struct meshModelPublicationParams_tag
{
  uint16_t                        publishAppKeyIndex;       /*!< Publish AppKey Index */
  meshPublishPeriodNumSteps_t     publishPeriodNumSteps;    /*!< Publish period number of steps */
  meshPublishPeriodStepRes_t      publishPeriodStepRes;     /*!< Publish period step resolution */
  meshPublishFriendshipCred_t     publishFriendshipCred;    /*!< Publish friendship security
                                                             *   material
                                                             */
  uint8_t                         publishTtl;               /*!< Publish TTL */
  meshPublishRetransCount_t       publishRetransCount;      /*!< Publish retransmit count */
  meshPublishRetransIntvlSteps_t  publishRetransSteps50Ms;  /*!< Publish 50 ms retransmit steps */
} meshModelPublicationParams_t;

/*! \brief NetKey index list for a specific node. */
typedef struct meshNetKeyList_tag
{
  uint8_t   netKeyCount;     /*!< Size of the pNetKeyIndexes array */
  uint16_t* pNetKeyIndexes;  /*!< Array of NetKey indexes */
} meshNetKeyList_t;

/*! \brief Key indexes for a NetKey and a bound AppKey */
typedef struct meshAppNetKeyBind_tag
{
  uint16_t netKeyIndex;  /*!< Associated NetKey index */
  uint16_t appKeyIndex;  /*!< AppKey index */
} meshAppNetKeyBind_t;

/*! \brief AppKey index list bound to a specific NetKey index */
typedef struct meshAppKeyList_tag
{
  uint16_t  netKeyIndex;     /*!< Associated NetKey index */
  uint8_t   appKeyCount;     /*!< Size of the pAppKeyIndexes array */
  uint16_t* pAppKeyIndexes;  /*!< Array of AppKey indexes */
} meshAppKeyList_t;

/*! \brief AppKey index list bound to a specific model */
typedef struct meshModelAppList_tag
{
  meshAddress_t         elemAddr;         /*!< Address of the element containing the model */
  modelId_t             modelId;          /*!< Model identifier */
  bool_t                isSig;            /*!< TRUE if model identifier is SIG, FALSE for vendor */
  uint8_t               appKeyCount;      /*!< Size of the pAppKeyIndexes array */
  uint16_t*             pAppKeyIndexes;   /*!< Array of AppKey indexes */
} meshModelAppList_t;

/*! \brief Composition Data */
typedef struct meshCompData_tag
{
  uint8_t  pageNumber;  /*!< Page number */
  uint16_t pageSize;    /*!< Size of the pPage array */
  uint8_t  *pPage;      /*!< Page in raw octet format (as received over the air) */
} meshCompData_t;

/*! \brief Heartbeat Publication state data */
typedef struct meshHbPub_tag
{
  meshAddress_t           dstAddr;      /*!< Destination address for heartbeat message */
  uint8_t                 countLog;     /*!< Number of heartbeat messages to be sent */
  uint8_t                 periodLog;    /*!< Period for sending heartbeat messages */
  uint8_t                 ttl;          /*!< TTL used when sending heartbeat messages */
  meshFeatures_t          features;     /*!< Bit field for features that trigger heartbeat messages */
  uint16_t                netKeyIndex;  /*!< Associated NetKey index */
} meshHbPub_t;

/*! \brief Heartbeat Subscription state data */
typedef struct meshHbSub_tag
{
  meshAddress_t           srcAddr;      /*!< Source address for heartbeat message */
  meshAddress_t           dstAddr;      /*!< Destination address for heartbeat message */
  uint8_t                 periodLog;    /*!< Period for sending heartbeat messages */
  uint8_t                 countLog;     /*!< Number of heartbeat messages to be sent */
  uint8_t                 minHops;      /*!< Min hops when receiving heartbeats */
  uint8_t                 maxHops;      /*!< Max hops when receiving heartbeats */
} meshHbSub_t;

/*! \brief Mesh Friendship RSSI factor, see ::meshFriendshipRssiFactorValues */
typedef uint8_t meshFriendshipRssiFactor_t;

/*! \brief Mesh Friendship Receive Window factor, see ::meshFriendshipRecvWinFactorValues */
typedef uint8_t meshFriendshipRecvWinFactor_t;

/*! \brief Mesh Friendship Min Queue size log, see ::meshFriendshipMinQueueSizeLogValues */
typedef uint8_t meshFriendshipMinQueueSizeLog_t;

/*! \brief Mesh Friendship Criteria structure */
typedef struct meshFriendshipCriteria_tag
{
  meshFriendshipRssiFactor_t        rssiFactor;              /*!< RSSI factor */
  meshFriendshipRecvWinFactor_t     recvWinFactor;           /*!< Receive Window factor */
  meshFriendshipMinQueueSizeLog_t   minQueueSizeLog;         /*!< Min Queue size log */
} meshFriendshipCriteria_t;

/*! \brief Mesh GATT Proxy PDU type. See ::meshGattProxyPduTypes */
typedef uint8_t meshGattProxyPduType_t;

#ifdef __cplusplus
}
#endif

#endif /* MESH_TYPES_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
