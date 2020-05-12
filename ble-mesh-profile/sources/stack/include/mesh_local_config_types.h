/*************************************************************************************************/
/*!
*  \file   mesh_local_config_types.h
*
*  \brief  Local Configuration types.
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
 *  \addtogroup LOCAL_CONFIGURATION Local Configuration Types
 *  @{
 */
/*************************************************************************************************/

#ifndef MESH_LOCAL_CONFIG_TYPES_H
#define MESH_LOCAL_CONFIG_TYPES_H

#include "wsf_timer.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Data type for storing both SIG and Vendor model identifiers */
typedef struct meshModelId_tag
{
  bool_t    isSigModel; /*!< Determines if model type is SIG or Vendor */
  modelId_t modelId;    /*!< Model Id */
} meshModelId_t;

/*! Data type for Subscription list entries */
typedef struct meshLocalCfgModelSubscrListEntry_tag
{
  uint16_t    subscrAddressIndex;  /*!< Address index into one of the address lists */
  bool_t      subscrToLabelUuid;   /*!< Subscription Address is in Label UUID list */
} meshLocalCfgModelSubscrListEntry_t;

typedef struct meshLocalCfgModelSubscrListInfo_tag
{
  meshLocalCfgModelSubscrListEntry_t *pSubscrList;    /*!< Pointer to the start location of the
                                                       *   Subscription List in memory
                                                       */
  uint16_t                           subscrListSize;  /*!< Size of the Subscription List */
} meshLocalCfgModelSubscrListInfo_t;

/*! Structure to store Model Publication state informations */
typedef struct meshLocalCfgModelPublication_tag
{
  uint16_t                        publishAddressIndex;      /*!< Publish Address index in the
                                                             *   address list
                                                             */
  uint16_t                        publishAppKeyEntryIndex;  /*!< Publish AppKey index in the
                                                             *   AppKey list
                                                             */
  meshPublishPeriodNumSteps_t     publishPeriodNumSteps;    /*!< Publish period number of steps */
  meshPublishPeriodStepRes_t      publishPeriodStepRes;     /*!< Publish period step resolution */
  meshPublishFriendshipCred_t     publishFriendshipCred;    /*!< Publish friendship security
                                                             *   material
                                                             */
  uint8_t                         publishTtl;               /*!< Publish TTL */
  meshPublishRetransCount_t       publishRetransCount;      /*!< Publish retransmit count */
  meshPublishRetransIntvlSteps_t  publishRetransSteps50Ms;  /*!< Publish 50 ms retransmit steps */
  bool_t                          publishToLabelUuid;       /*!< Publish Address is virtual */
} meshLocalCfgModelPublication_t;

/*! Data type for Model instance list entries */
typedef struct meshLocalCfgModelEntry_tag
{
  meshModelId_t                    modelId;                /*!< Pointer to the Model structure */
  meshElementId_t                  elementId;              /*!< Element index in the Element array
                                                            */
  meshLocalCfgModelPublication_t   publicationState;       /*!< Model publication state */
  uint16_t                         subscrListStartIdx;     /*!< Start index in the Subscription list
                                                            *   for a specific model
                                                            */
  uint16_t                         appKeyBindListStartIdx; /*!< Start index in the AppKey Bind List
                                                            *   for a specific model
                                                            */
  uint8_t                          subscrListSize;         /*!< Subscription list size for the
                                                            *   model
                                                            */
  uint8_t                          appKeyBindListSize;     /*!< Size of AppKey Bind list for the
                                                            *   model
                                                            */
} meshLocalCfgModelEntry_t;

/*! Local structure to store Models informations */
typedef struct meshLocalCfgModelInfo_tag
{
  meshLocalCfgModelEntry_t *pModelArray;    /*!< Pointer to array describing models */
  uint16_t                 modelArraySize;  /*!< Size of the Models List */
} meshLocalCfgModelInfo_t;

/*! Local structure for attention timer */
typedef struct meshLocalCfgAttTmr_tag
{
  wsfTimer_t attTmr;          /*!< WSF timer */
  uint8_t    remainingSec;    /*!< Remaining seconds */
} meshLocalCfgAttTmr_t;

/*! Local structure to store Elements informations */
typedef struct meshLocalCfgElementInfo_tag
{
  const meshElement_t        *pElementArray;          /*!< Pointer to array describing elements
                                                       *   present in the node
                                                       */
  meshLocalCfgAttTmr_t       *pAttTmrArray;           /*!< Pointer to array attention timer
                                                       *   for each element
                                                       */
  meshSeqNumber_t            *pSeqNumberArray;        /*!< Pointer to SEQ number array for each
                                                       *   element
                                                       */
  meshSeqNumber_t            *pSeqNumberThreshArray;  /*!< Pointer to SEQ number threshold array for
                                                       *   each element
                                                       */
  uint8_t                    elementArrayLen;         /*!< Length of pElementArray and
                                                       *   pElementLocalCfgArray. Both have the
                                                       *   same length and indexes
                                                       */
} meshLocalCfgElementInfo_t;

/*! Data type for non-virtual address list entries */
typedef struct meshLocalCfgAddressListEntry_tag
{
  meshAddress_t  address;                /*!< Non-virtual address */
  uint16_t       referenceCountPublish;  /*!< Number of allocations of this address for
                                          *   publication
                                          */
  uint16_t       referenceCountSubscr;   /*!< Number of allocations of this address for
                                          *   subscription
                                          */
} meshLocalCfgAddressListEntry_t;

/*! Local structure to store Non-virtual Address informations */
typedef struct meshLocalCfgAddressListInfo_tag
{
  meshLocalCfgAddressListEntry_t *pAddressList;    /*!< Pointer to the start location of the
                                                    *   Non-virtual Address List in memory
                                                    */
  uint16_t                       addressListSize;  /*!< Size of the Address List */
} meshLocalCfgAddressListInfo_t;

/*! Data type for Virtual Address list entries */
typedef struct meshLocalCfgVirtualAddrListEntry_tag
{
  meshAddress_t  address;                           /*!< Virtual Address */
  uint8_t        labelUuid[MESH_LABEL_UUID_SIZE];   /*!< Label UUID */
  uint16_t       referenceCountPublish;             /*!< Number of allocations of this address
                                                     *  for publication
                                                     */
  uint16_t       referenceCountSubscr;              /*!< Number of allocations of this address
                                                     *  for subscription
                                                     */
} meshLocalCfgVirtualAddrListEntry_t;

/*! Local structure to store Label UUID informations */
typedef struct meshLocalCfgVirtualAddrListInfo_tag
{
  meshLocalCfgVirtualAddrListEntry_t *pVirtualAddrList;    /*!< Pointer to the start location of the
                                                            *   Virtual Address List in memory
                                                            */
  uint16_t                           virtualAddrListSize;  /*!< Size of the Virtual Address List */
} meshLocalCfgVirtualAddrListInfo_t;

/*! Local structure to store AppKey to Model ID bind informations */
typedef struct meshLocalCfgAppKeyBindListInfo_tag
{
  uint16_t *pAppKeyBindList;     /*!< Pointer to the start location of the AppKeyBind list
                                  *   in memory
                                  */
  uint16_t  appKeyBindListSize;  /*!< Size of the AppKeyBind List */
} meshLocalCfgAppKeyBindListInfo_t;

/*! Data type for AppKey list entries */
typedef struct meshLocalCfgAppKeyListEntry_tag
{
  uint16_t netKeyEntryIndex;              /*!< Index in the table of NetKeys for the NetKey binded
                                           *   to the AppKey
                                           */
  uint16_t appKeyIndex;                   /*!< AppKey Index to identify the AppKey in the list */
  uint8_t  appKeyOld[MESH_KEY_SIZE_128];  /*!< Old Application Key value */
  uint8_t  appKeyNew[MESH_KEY_SIZE_128];  /*!< New Application Key value */
  bool_t   newKeyAvailable;               /*!< Flag to signal that a new key is available */
} meshLocalCfgAppKeyListEntry_t;

/*! Local structure to store AppKey List related informations */
typedef struct meshLocalCfgAppKeyListInfo_tag
{
  meshLocalCfgAppKeyListEntry_t *pAppKeyList;    /*!< Pointer to the start location of the
                                                  *   AppKeyList in memory
                                                  */
  uint16_t                      appKeyListSize;  /*!< Size of the AppKey List */
} meshLocalCfgAppKeyListInfo_t;

/*! Data type for NetKey List entries */
typedef struct meshLocalCfgNetKeyListEntry_tag
{
  uint16_t                 netKeyIndex;                   /*!< NetKey Index to identify the NetKey
                                                           *   in the list
                                                           */
  uint8_t                  netKeyOld[MESH_KEY_SIZE_128];  /*!< Old Network Key value */
  uint8_t                  netKeyNew[MESH_KEY_SIZE_128];  /*!< New Network Key value */
  meshKeyRefreshStates_t   keyRefreshState;               /*!< Key Refresh Phase state value.
                                                           *   See::meshKeyRefreshStates_t
                                                           */
  bool_t                   newKeyAvailable;               /*!< Flag to signal that a new key is
                                                           *   available
                                                           */
} meshLocalCfgNetKeyListEntry_t;

/*! Data type for Node Identity State List entries. See::meshNodeIdentityStates_t */
typedef meshNodeIdentityStates_t meshLocalCfgNodeIdentityListEntry_t;

/*! Local structure to store NetKey List related informations */
typedef struct meshLocalCfgNetKeyListInfo_tag
{
  meshLocalCfgNetKeyListEntry_t        *pNetKeyList;       /*!< Pointer to the start location of
                                                            *   the NetKeyList in memory
                                                            */
  meshLocalCfgNodeIdentityListEntry_t  *pNodeIdentityList; /*!< Pointer to the start location of
                                                            *   the Node Identity State list in memory
                                                            */
  uint16_t                             netKeyListSize;     /*!< Size of the NetKey List */
} meshLocalCfgNetKeyListInfo_t;

/*! Mesh Local Config Friend Subscription notification event types enumeration */
enum meshLocalCfgFriendSubscrEventTypes
{
  MESH_LOCAL_CFG_FRIEND_SUBSCR_ADD,   /*!< Local Config subscription address add */
  MESH_LOCAL_CFG_FRIEND_SUBSCR_RM,    /*!< Local Config subscription address remove */
};

/*! Mesh Local Config Friend Subscription notification event type.
 *  See ::meshLocalCfgFriendSubscrEventTypes
 */
typedef uint8_t meshLocalCfgFriendSubscrEvent_t;

/*! Mesh Local Config Friend Subscription Event notification */
typedef struct meshLocalCfgFriendSubscrEventParams_tag
{
  meshAddress_t  address;        /*!< Address */
  uint16_t       idx;            /*!< Index in address list */
} meshLocalCfgFriendSubscrEventParams_t;

#ifdef __cplusplus
}
#endif

#endif /* MESH_LOCAL_CONFIG_TYPES_H */

/*************************************************************************************************/
/*!
 *  @}
 */
/*************************************************************************************************/
