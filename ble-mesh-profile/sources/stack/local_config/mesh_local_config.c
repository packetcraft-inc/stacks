/*************************************************************************************************/
/*!
 *  \file   mesh_local_config.c
 *
 *  \brief  Local Configuration implementation.
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

#include <string.h>

#include "wsf_types.h"
#include "wsf_msg.h"
#include "wsf_os.h"
#include "wsf_assert.h"
#include "wsf_nvm.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"
#include "mesh_main.h"

#include "mesh_error_codes.h"
#include "mesh_seq_manager.h"
#include "mesh_proxy_sr.h"

#include "mesh_local_config_types.h"
#include "mesh_local_config.h"
#include "mesh_utils.h"

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
#include "mesh_test_api.h"
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Sequence number increment for saving in NVM */
#ifndef MESH_SEQ_NUMBER_NVM_INC
#define MESH_SEQ_NUMBER_NVM_INC                          1000
#endif

/*! Invalid index in Address Lists, Mesh AppKey or NetKey lists */
#define MESH_INVALID_ENTRY_INDEX                         0xFFFF

/*! Invalid Mesh AppKey or NetKey value */
#define MESH_KEY_INVALID_INDEX                           0xFFFF

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Local Config WSF message events */
enum meshLocalCfgWsfMsgEvents
{
  MESH_LOCAL_CFG_MSG_ATT_TMR_EXPIRED = MESH_LOCAL_CFG_MSG_START, /*!< Attention timer expired */
};

/*! Mesh Local Config Local structure */
static struct meshLocalCfgLocalInfo_tag
{
  uint32_t               ivIndex;                           /*!< IV index */
  meshAddress_t          address;                           /*!< 16-bit element address */
  meshProdInfo_t         prodInfo;                          /*!< Product Info */
  uint8_t                deviceKey[MESH_KEY_SIZE_128];      /*!< Device Key */
  uint8_t                defaultTtl;                        /*!< Default TTL value */
  meshRelayStates_t      relayState;                        /*!< Relay state
                                                             *   \see meshRelayStates_t
                                                             */
  meshBeaconStates_t     beaconState;                       /*!< Beacon state
                                                             *   \see meshBeaconStates_t
                                                             */
  meshGattProxyStates_t  gattProxyState;                    /*!< GATT Proxy state
                                                             *   \see meshGattProxyStates_t
                                                             */
  meshFriendStates_t     friendState;                       /*!< Friend state
                                                             *   \see meshFriendStates_t
                                                             */
  meshLowPowerStates_t   lowPowerState;                     /*!< Low Power state
                                                             *   \see meshLowPowerStates_t
                                                             */
  uint8_t                nwkTransCount;                     /*!< Network Transmission Count */
  uint8_t                nwkIntvlSteps;                     /*!< Network Interval Steps */
  uint8_t                relayRetransCount;                 /*!< Relay Retransmission Count */
  uint8_t                relayRetransIntvlSteps;            /*!< Relay Retransmission Interval
                                                             *   Steps
                                                             */
  bool_t                 ivUpdtInProg;                      /*!< IV Index update in progress
                                                             *   flag
                                                             */
} localCfg;

/*! Mesh Local Config Heartbeat Local structure */
static struct meshLocalCfgHbLocalInfo_tag
{
  uint16_t       pubDstAddressIndex;  /*!< Publication Destination Address index in list */
  uint16_t       subSrcAddressIndex;  /*!< Subscription Source Address index in list */
  uint16_t       subDstAddressIndex;  /*!< Subscription Destination Address index in list */
  meshFeatures_t pubFeatures;         /*!< Publication Features */
  uint16_t       pubNetKeyEntryIndex; /*!< Publication NetKey entry index in NetKey list */
  uint8_t        pubCountLog;         /*!< Publication Count Log */
  uint8_t        subCountLog;         /*!< Subscription Count Log */
  uint8_t        pubPeriodLog;        /*!< Publication Period Log */
  uint8_t        subPeriodLog;        /*!< Subscription Period Log */
  uint8_t        pubTtl;              /*!< Publication TTL */
  uint8_t        subMinHops;          /*!< Subscription Minimum Hops */
  uint8_t        subMaxHops;          /*!< Subscription Maximum Hops */
} localCfgHb;

/*! Local Config NetKey List Local structure */
static meshLocalCfgNetKeyListInfo_t localCfgNetKeyList;

/*! Local Config AppKey List Local structure */
static meshLocalCfgAppKeyListInfo_t localCfgAppKeyList;

/*! Local Config AppKeyBind List Local structure */
static meshLocalCfgAppKeyBindListInfo_t localCfgAppKeyBindList;

/*! Local Config Virtual Address List Local structure */
static meshLocalCfgVirtualAddrListInfo_t localCfgVirtualAddrList;

/*! Local Config Non-virtual Address List Local structure */
static meshLocalCfgAddressListInfo_t localCfgAddressList;

/*! Local Config Elements List Local structure */
static meshLocalCfgElementInfo_t localCfgElement;

/*! Local Config Models List Local structure */
static meshLocalCfgModelInfo_t localCfgModel;

/*! Local Config Subscription List Local structure */
static meshLocalCfgModelSubscrListInfo_t localCfgSubscrList;

/*! Mesh Local Config control block */
static struct meshLocalCfgCb_tag
{
  meshLocalCfgFriendSubscrEventNotifyCback_t  friendSubscrEventCback; /*!< Send subscription event */
} localCfgCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Computes memory requirements based on configured number of elements.
 *
 *  \param[in] numElements  Number of elements.
 *
 *  \return    Required memory in bytes for Attention Timer array.
 */
/*************************************************************************************************/
static inline uint16_t meshLocalCfgGetRequiredMemoryAttTmrArray(uint16_t numElements)
{
  return  MESH_UTILS_ALIGN(sizeof(meshLocalCfgAttTmr_t) * numElements);
}

/*************************************************************************************************/
/*!
 *  \brief     Computes memory requirements for SEQ number array based on configured number of
 *             elements.
 *
 *  \param[in] numElements  Number of elements.
 *
 *  \return    Required memory in bytes for Element array.
 */
/*************************************************************************************************/
static inline uint16_t meshLocalCfgGetRequiredMemorySeqNumberArray(uint16_t numElements)
{
  return  MESH_UTILS_ALIGN(sizeof(meshSeqNumber_t) * numElements);
}

/*************************************************************************************************/
/*!
 *  \brief     Computes memory requirements based on configured number of models.
 *
 *  \param[in] numModels  Number of models.
 *
 *  \return    Required memory in bytes for Model array.
 */
/*************************************************************************************************/
static inline uint16_t meshLocalCfgGetRequiredMemoryModelArray(uint16_t numModels)
{
  return  MESH_UTILS_ALIGN(sizeof(meshLocalCfgModelEntry_t) * numModels);
}

/*************************************************************************************************/
/*!
 *  \brief     Computes memory requirements based on configured Subscription List size.
 *
 *  \param[in] subscrListSize  Subscription List size.
 *
 *  \return    Required memory in bytes for Subscription List.
 */
/*************************************************************************************************/
static inline uint16_t meshLocalCfgGetRequiredMemorySubscrList(uint16_t subscrListSize)
{
  return  MESH_UTILS_ALIGN(sizeof(meshLocalCfgModelSubscrListEntry_t) * subscrListSize);
}

/*************************************************************************************************/
/*!
 *  \brief     Computes memory requirements based on configured AppKey Bind List size.
 *
 *  \param[in] appKeyBindListSize  AppKey Bind List size.
 *
 *  \return    Required memory in bytes for AppKey Bind List.
 */
/*************************************************************************************************/
static inline uint16_t meshLocalCfgGetRequiredMemoryAppKeyBindList(uint16_t appKeyBindListSize)
{
  return  MESH_UTILS_ALIGN(sizeof(uint16_t) * appKeyBindListSize);
}

/*************************************************************************************************/
/*!
 *  \brief     Computes memory requirements based on configured Non-virtual Address List size.
 *
 *  \param[in] addressListSize  Non-virtual Address List size.
 *
 *  \return    Required memory in bytes for Non-virtual Address List.
 */
/*************************************************************************************************/
static inline uint16_t meshLocalCfgGetRequiredMemoryAddressList(uint16_t addressListSize)
{
  return  MESH_UTILS_ALIGN(sizeof(meshLocalCfgAddressListEntry_t) * addressListSize);
}

/*************************************************************************************************/
/*!
 *  \brief     Computes memory requirements based on configured Virtual Address List size.
 *
 *  \param[in] virtualAddrListSize  Virtual Address List size.
 *
 *  \return    Required memory in bytes for Virtual Address List.
 */
/*************************************************************************************************/
static inline uint16_t meshLocalCfgGetRequiredMemoryVirtualAddrList(uint16_t virtualAddrListSize)
{
  return  MESH_UTILS_ALIGN(sizeof(meshLocalCfgVirtualAddrListEntry_t) * virtualAddrListSize);
}

/*************************************************************************************************/
/*!
 *  \brief     Computes memory requirements based on configured AppKey List size.
 *
 *  \param[in] appKeyListSize  AppKey List size.
 *
 *  \return    Required memory in bytes for AppKey List.
 */
/*************************************************************************************************/
static inline uint16_t meshLocalCfgGetRequiredMemoryAppKeyList(uint16_t appKeyListSize)
{
  return  MESH_UTILS_ALIGN(sizeof(meshLocalCfgAppKeyListEntry_t) * appKeyListSize);
}

/*************************************************************************************************/
/*!
 *  \brief     Computes memory requirements based on configured NetKey List size.
 *
 *  \param[in] netKeyListSize  NetKey List size.
 *
 *  \return    Required memory in bytes for NetKey List.
 */
/*************************************************************************************************/
static inline uint16_t meshLocalCfgGetRequiredMemoryNetKeyList(uint16_t netKeyListSize)
{
  return  MESH_UTILS_ALIGN(sizeof(meshLocalCfgNetKeyListEntry_t) * netKeyListSize);
}

/*************************************************************************************************/
/*!
 *  \brief     Computes memory requirements based on configured Node Identity List size.
 *
 *  \param[in] netKeyListSize  NetKey List size.
 *
 *  \return    Required memory in bytes for Node Identity List.
 */
/*************************************************************************************************/
static inline uint16_t meshLocalCfgGetRequiredMemoryNodeIdentityList(uint16_t netKeyListSize)
{
  return  MESH_UTILS_ALIGN(sizeof(meshLocalCfgNodeIdentityListEntry_t) * netKeyListSize);
}

/*************************************************************************************************/
/*!
 *  \brief  Computes total number of models instances based on initial configuration.
 *
 *  \return Total number of model instances in the configuration.
 */
/*************************************************************************************************/
static uint16_t meshLocalCfgGetTotalNumModels(void)
{
  uint16_t numModels = 0;
  uint8_t i;

  /* Search through element array. */
  for (i = 0; i < pMeshConfig->elementArrayLen; i++)
  {
    /* Sum up the number of models for each element. */
    numModels += pMeshConfig->pElementArray[i].numSigModels +
                 pMeshConfig->pElementArray[i].numVendorModels;
  }
  return numModels;
}

/*************************************************************************************************/
/*!
 *  \brief  Computes total Subscription List size based on inital configuration.
 *
 *  \return Total Subscription List size.
 */
/*************************************************************************************************/
static uint16_t meshLocalCfgGetTotalSubscrListSize(void)
{
  uint16_t subscrListSize = 0;
  uint16_t size;
  uint16_t j;
  uint8_t i;

  /* Search through element array. */
  for (i = 0; i < pMeshConfig->elementArrayLen; i++)
  {
    /* Sum up the Subscription Lists sizes for each models instance in elements. */
    for (j = 0; j < pMeshConfig->pElementArray[i].numSigModels; j++)
    {
      size = pMeshConfig->pElementArray[i].pSigModelArray[j].subscrListSize;

      if (size != MMDL_SUBSCR_LIST_SHARED)
      {
        subscrListSize += size;
      }
    }
    for (j = 0; j < pMeshConfig->pElementArray[i].numVendorModels; j++)
    {
      size = pMeshConfig->pElementArray[i].pVendorModelArray[j].subscrListSize;

      if (size != MMDL_SUBSCR_LIST_SHARED)
      {
        subscrListSize += size;
      }
    }
  }
  return subscrListSize;
}

/*************************************************************************************************/
/*!
 *  \brief  Computes total AppKey Bind List size based on inital configuration.
 *
 *  \return Total AppKey Bind List size.
 */
/*************************************************************************************************/
static uint16_t meshLocalCfgGetTotalAppKeyBindListSize(void)
{
  uint16_t appKeyBindListSize = 0;
  uint16_t j;
  uint8_t i;

  /* Search through element array. */
  for (i = 0; i < pMeshConfig->elementArrayLen; i++)
  {
    /* Sum up the AppKey Bind Lists sizes for each models instance in elements. */
    for (j = 0; j < pMeshConfig->pElementArray[i].numSigModels; j++)
    {
      appKeyBindListSize += pMeshConfig->pElementArray[i].pSigModelArray[j].appKeyBindListSize;
    }
    for (j = 0; j < pMeshConfig->pElementArray[i].numVendorModels; j++)
    {
      appKeyBindListSize += pMeshConfig->pElementArray[i].pVendorModelArray[j].appKeyBindListSize;
    }
  }
  return appKeyBindListSize;
}

/*************************************************************************************************/
/*!
 *  \brief     Searches for a Model Instance into the local Model array based on Element ID and
 *             Model ID.
 *
 *  \param[in] elementId  Local element identifier.
 *  \param[in] pModelId   Pointer to generic model identifier structure.
 *
 *  \return    Model entry index or MESH_INVALID_ENTRY_INDEX if model not found.
 */
/*************************************************************************************************/
static uint16_t meshLocalCfgSearchModel(meshElementId_t elementId, const meshModelId_t *pModelId)
{
  uint16_t i;

  /* Search for Model into the local Model array. */
  for (i = 0; i < localCfgModel.modelArraySize; i++)
  {
    /* Check if both Model ID and Element ID match. */
    if (localCfgModel.pModelArray[i].elementId == elementId)
    {
      if (pModelId->isSigModel == localCfgModel.pModelArray[i].modelId.isSigModel)
      {
        if ((pModelId->isSigModel == TRUE) &&
            (pModelId->modelId.sigModelId ==
              localCfgModel.pModelArray[i].modelId.modelId.sigModelId))
        {
          return i;
        }
        else if ((pModelId->isSigModel == FALSE) &&
                 (pModelId->modelId.vendorModelId ==
                   localCfgModel.pModelArray[i].modelId.modelId.vendorModelId))
        {
          return i;
        }
      }
    }
  }
  return MESH_INVALID_ENTRY_INDEX;
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the address entry index in the address list.
 *
 *  \param[in] address     Address to be searched for.
 *  \param[in] pLabelUuid  Pointer to coresponding Label UUID if address is Virtual.
 *
 *  \return    Address entry index or MESH_INVALID_ENTRY_INDEX if address not found.
 */
/*************************************************************************************************/
static uint16_t meshLocalCfgGetAddressEntryIndex(meshAddress_t address, const uint8_t *pLabelUuid)
{
  uint16_t i;

  if (!MESH_IS_ADDR_UNASSIGNED(address))
  {
    /* Check if Virtual Address*/
    if (MESH_IS_ADDR_VIRTUAL(address))
    {
      /* If address is virtual, Label UUID is mandatory. */
      if (pLabelUuid == NULL)
      {
        WSF_ASSERT(pLabelUuid != NULL);
        return MESH_INVALID_ENTRY_INDEX;
      }

      /* Search Virtual Address list. */
      for (i = 0; i < localCfgVirtualAddrList.virtualAddrListSize; i++)
      {
        /* Virtual Address found. */
        if (localCfgVirtualAddrList.pVirtualAddrList[i].address == address)
        {
          /* Check if both Label UUIDs match. */
          if (memcmp(pLabelUuid, &localCfgVirtualAddrList.pVirtualAddrList[i].labelUuid,
                     MESH_LABEL_UUID_SIZE) == 0x00)
          {
            return i;
          }
        }
      }
    }
    else
    {
      /* Search Non-virtual Address list. */
      for (i = 0; i < localCfgAddressList.addressListSize; i++)
      {
        /* Address found. */
        if (localCfgAddressList.pAddressList[i].address == address)
        {
          return i;
        }
      }
    }
  }
  return MESH_INVALID_ENTRY_INDEX;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the address into one of the address lists.
 *
 *  \param[in] address        Address to be added.
 *  \param[in] pLabelUuid     Pointer to coresponding Label UUID if address is Virtual.
 *  \param[in] isPublishAddr  TRUE if the address is used for Publish.
 *                            FALSE if the address is used for Subscription.
 *
 *  \return    Address entry index if successfully added or MESH_INVALID_ENTRY_INDEX if it failed.
 *
 *  \remarks   This function does not check if the address already exists. Just adds an address to
 *             an empty position in list.
 */
/*************************************************************************************************/
static uint16_t meshLocalCfgSetAddress(meshAddress_t address, const uint8_t *pLabelUuid,
                                       bool_t isPublishAddr)
{
  meshLocalCfgFriendSubscrEventParams_t subscrEventParam;
  uint16_t i;

  if (!MESH_IS_ADDR_UNASSIGNED(address))
  {
    /* Check for address type. */
    if (MESH_IS_ADDR_VIRTUAL(address))
    {
      /* If address is virtual, Label UUID is mandatory. */
      if (pLabelUuid == NULL)
      {
        WSF_ASSERT(pLabelUuid != NULL);
        return MESH_INVALID_ENTRY_INDEX;
      }
      /* Search through all Virtual Address list. */
      for (i = 0; i < localCfgVirtualAddrList.virtualAddrListSize; i++)
      {
        /* If address is set on UNASSIGNED then the location is empty. */
        if (localCfgVirtualAddrList.pVirtualAddrList[i].address == MESH_ADDR_TYPE_UNASSIGNED)
        {
          /* Set address and Label UUID for that location. */
          localCfgVirtualAddrList.pVirtualAddrList[i].address = address;
          memcpy(localCfgVirtualAddrList.pVirtualAddrList[i].labelUuid, pLabelUuid,
                 MESH_LABEL_UUID_SIZE);
          /* Increment reference count based on address type. */
          if (isPublishAddr == TRUE)
          {
            localCfgVirtualAddrList.pVirtualAddrList[i].referenceCountPublish++;
          }
          else
          {
            /* Invoke callback if the address was added to Subscription List. */
            if (localCfgVirtualAddrList.pVirtualAddrList[i].referenceCountSubscr == 0)
            {
              subscrEventParam.address = address;
              subscrEventParam.idx = i;

              localCfgCb.friendSubscrEventCback(MESH_LOCAL_CFG_FRIEND_SUBSCR_ADD, &subscrEventParam);
            }

            localCfgVirtualAddrList.pVirtualAddrList[i].referenceCountSubscr++;
          }

          /* Update Virtual Address entry in NVM. */
          WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_VIRTUAL_ADDR_DATASET_ID, (uint8_t *)localCfgVirtualAddrList.pVirtualAddrList,
                                     sizeof(meshLocalCfgVirtualAddrListEntry_t) * localCfgVirtualAddrList.virtualAddrListSize, NULL);

          return i;
        }
      }
    }
    else
    {
      /* Search through all Non-virtual Address list. */
      for (i = 0; i < localCfgAddressList.addressListSize; i++)
      {
        /* If address is set on UNASSIGNED then the location is empty. */
        if (localCfgAddressList.pAddressList[i].address == MESH_ADDR_TYPE_UNASSIGNED)
        {
          /* Set address for that location. */
          localCfgAddressList.pAddressList[i].address = address;
          /* Increment reference count based on addres type. */
          if (isPublishAddr == TRUE)
          {
            localCfgAddressList.pAddressList[i].referenceCountPublish++;
          }
          else
          {
            /* Invoke callback if the address was added to Subscription List. */
            if ((localCfgAddressList.pAddressList[i].referenceCountSubscr == 0) &&
                (MESH_IS_ADDR_GROUP(address)))
            {
              subscrEventParam.address = address;
              subscrEventParam.idx = i;

              localCfgCb.friendSubscrEventCback(MESH_LOCAL_CFG_FRIEND_SUBSCR_ADD, &subscrEventParam);
            }

            localCfgAddressList.pAddressList[i].referenceCountSubscr++;
          }

          /* Update Address entry in NVM. */
          WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_ADDRESS_DATASET_ID, (uint8_t *)localCfgAddressList.pAddressList,
                                     sizeof(meshLocalCfgAddressListEntry_t) * localCfgAddressList.addressListSize, NULL);

          return i;
        }
      }
    }
  }
  return MESH_INVALID_ENTRY_INDEX;
}

/*************************************************************************************************/
/*!
 *  \brief     Removes an address from Address Lists based on Address Type and Address entry index.
 *
 *  \param[in] addrEntryIdx   Address entry index in address list.
 *  \param[in] isVirtualAddr  TRUE if address is virtual.
 *                            FALSE if addres is non-virtual.
 *  \param[in] isPublishAddr  TRUE if the address is used for Publish.
 *                            FALSE if the address is used for Subscription.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshLocalCfgRemoveAddress(uint16_t addrEntryIdx, bool_t isVirtualAddr,
                                      bool_t isPublishAddr)
{
  meshLocalCfgFriendSubscrEventParams_t subscrEventParam;

  if (isVirtualAddr == TRUE)
  {
    /* Check if address entry index does not exceed the Virtual Address list size. */
    if (addrEntryIdx < localCfgVirtualAddrList.virtualAddrListSize)
    {
      if (isPublishAddr == TRUE)
      {
        /* Check if Publish Reference count is not 0. */
        if (localCfgVirtualAddrList.pVirtualAddrList[addrEntryIdx].referenceCountPublish > 0)
        {
          localCfgVirtualAddrList.pVirtualAddrList[addrEntryIdx].referenceCountPublish--;
        }
      }
      else
      {
        /* Check if Subscription Reference count is not 0. */
        if (localCfgVirtualAddrList.pVirtualAddrList[addrEntryIdx].referenceCountSubscr > 0)
        {
          localCfgVirtualAddrList.pVirtualAddrList[addrEntryIdx].referenceCountSubscr--;

          /* Invoke callback if address was removed from subscription list. */
          if (localCfgVirtualAddrList.pVirtualAddrList[addrEntryIdx].referenceCountSubscr == 0)
          {
            subscrEventParam.address = localCfgVirtualAddrList.pVirtualAddrList[addrEntryIdx].address;
            subscrEventParam.idx = addrEntryIdx;

            localCfgCb.friendSubscrEventCback(MESH_LOCAL_CFG_FRIEND_SUBSCR_RM, &subscrEventParam);
          }
        }
      }
      /* Check if both counts are 0 to remove the entry. */
      if ((localCfgVirtualAddrList.pVirtualAddrList[addrEntryIdx].referenceCountPublish == 0) &&
          (localCfgVirtualAddrList.pVirtualAddrList[addrEntryIdx].referenceCountSubscr == 0))
      {
        localCfgVirtualAddrList.pVirtualAddrList[addrEntryIdx].address = MESH_ADDR_TYPE_UNASSIGNED;
      }
    }

    /* Update Virtual Address entry in NVM. */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_VIRTUAL_ADDR_DATASET_ID, (uint8_t *)localCfgVirtualAddrList.pVirtualAddrList,
                               sizeof(meshLocalCfgVirtualAddrListEntry_t) * localCfgVirtualAddrList.virtualAddrListSize, NULL);
  }
  else
  {
    /* Check if address entry index does not exceed the Non-virtual Address list size. */
    if (addrEntryIdx < localCfgAddressList.addressListSize)
    {
      if (isPublishAddr == TRUE)
      {
        /* Check if Publish Reference count is not 0. */
        if (localCfgAddressList.pAddressList[addrEntryIdx].referenceCountPublish > 0)
        {
          localCfgAddressList.pAddressList[addrEntryIdx].referenceCountPublish--;
        }
      }
      else
      {
        /* Check if Subscription Reference count is not 0. */
        if (localCfgAddressList.pAddressList[addrEntryIdx].referenceCountSubscr > 0)
        {
          localCfgAddressList.pAddressList[addrEntryIdx].referenceCountSubscr--;

          /* Invoke callback if address was removed from subscription list. */
          if ((localCfgAddressList.pAddressList[addrEntryIdx].referenceCountSubscr == 0) &&
              (MESH_IS_ADDR_GROUP(localCfgAddressList.pAddressList[addrEntryIdx].address)))
          {
            subscrEventParam.address = localCfgAddressList.pAddressList[addrEntryIdx].address;
            subscrEventParam.idx = addrEntryIdx;

            localCfgCb.friendSubscrEventCback(MESH_LOCAL_CFG_FRIEND_SUBSCR_RM, &subscrEventParam);
          }
        }
      }
      /* Check if both counts are 0 to remove the entry. */
      if ((localCfgAddressList.pAddressList[addrEntryIdx].referenceCountPublish == 0) &&
          (localCfgAddressList.pAddressList[addrEntryIdx].referenceCountSubscr == 0))
      {
        localCfgAddressList.pAddressList[addrEntryIdx].address = MESH_ADDR_TYPE_UNASSIGNED;
      }
    }

    /* Update Address entry in NVM. */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_ADDRESS_DATASET_ID, (uint8_t *)localCfgAddressList.pAddressList,
                               sizeof(meshLocalCfgAddressListEntry_t) * localCfgAddressList.addressListSize, NULL);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the key entry index in the key list.
 *
 *  \param[in] keyIndex  Key Index to be searched for.
 *  \param[in] isNetKey  TRUE if the key is NetKey.
 *                       FALSE if the key is AppKey.
 *
 *  \return    Key entry index or MESH_INVALID_ENTRY_INDEX if key not found.
 */
/*************************************************************************************************/
static uint16_t meshLocalGetKeyEntryIndex(uint16_t keyIndex, bool_t isNetKey)
{
  uint16_t i;

  /* Check if KeyIndex is valid. */
  if (keyIndex != MESH_KEY_INVALID_INDEX)
  {
    /* Check for NetKey Index. */
    if (isNetKey == TRUE)
    {
      /* Search through NetKey list. */
      for (i = 0; i < localCfgNetKeyList.netKeyListSize; i++)
      {
        /* If NetKey Index found, return NetKey entry index. */
        if (keyIndex == localCfgNetKeyList.pNetKeyList[i].netKeyIndex)
        {
          return i;
        }
      }
    }
    /* Check for AppKey Index. */
    else
    {
      /* Search through AppKey list. */
      for (i = 0; i < localCfgAppKeyList.appKeyListSize; i++)
      {
        /* If AppKey Index found, return AppKey entry index. */
        if (keyIndex == localCfgAppKeyList.pAppKeyList[i].appKeyIndex)
        {
          return i;
        }
      }
    }
  }
  return MESH_INVALID_ENTRY_INDEX;
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the free entry index in the key list.
 *
 *  \param[in] isNetKey  TRUE if searching in NetKey list.
 *                       FALSE if searching in AppKey list.
 *
 *  \return    Free entry index or MESH_INVALID_ENTRY_INDEX if no free entry found.
 */
/*************************************************************************************************/
static uint16_t meshLocalGetKeyFreeEntryIndex(bool_t isNetKey)
{
  uint16_t i;

  if (isNetKey == TRUE)
  {
    /* Search through NetKey list. */
    for (i = 0; i < localCfgNetKeyList.netKeyListSize; i++)
    {
      /* If empty entry found, return entry index. */
      if (MESH_KEY_INVALID_INDEX == localCfgNetKeyList.pNetKeyList[i].netKeyIndex)
      {
        return i;
      }
    }
  }
  else
  {
    /* Search through AppKey list. */
    for (i = 0; i < localCfgAppKeyList.appKeyListSize; i++)
    {
      /* If empty entry found, return entry index. */
      if (MESH_KEY_INVALID_INDEX == localCfgAppKeyList.pAppKeyList[i].appKeyIndex)
      {
        return i;
      }
    }
  }
  return MESH_INVALID_ENTRY_INDEX;
}

/*************************************************************************************************/
/*!
 *  \brief     Timer callback for the Attention Timer state.
 *             Triggers every second when the Attention Timer state is on.
 *
 *  \param[in] timerId  Timer identifier for the Attention Timer.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshLocalCfgAttentionTimerCback(uint8_t timerId)
{
  meshAttentionEvt_t evt;

  /* Check range. */
  if(timerId < localCfgElement.elementArrayLen)
  {
    /* Check if attention is on for the current element */
    if (localCfgElement.pAttTmrArray[timerId].remainingSec > 0)
    {
      /* Decrement the Attention Timer state */
      localCfgElement.pAttTmrArray[timerId].remainingSec--;

      /* Check if attention has expired for this element */
      if (localCfgElement.pAttTmrArray[timerId].remainingSec == 0)
      {
        /* Signal event to the application */
        evt.hdr.event = MESH_CORE_EVENT;
        evt.hdr.param = MESH_CORE_ATTENTION_CHG_EVENT;
        evt.elementId = timerId;
        evt.attentionOn = FALSE;
        meshCb.evtCback((meshEvt_t*)&evt);
      }
      else
      {
        /* Restart timer */
        WsfTimerStartSec(&(localCfgElement.pAttTmrArray[timerId].attTmr), 1);
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Signals that attention has been turned on for an element
 *             and ensures attention timer is running.
 *
 *  \param[in] elementId  Local element identifier.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshLocalCfgStartAttention(meshElementId_t elementId)
{
  meshAttentionEvt_t evt;

  /* Signal event to the application */
  evt.hdr.event = MESH_CORE_EVENT;
  evt.hdr.param = MESH_CORE_ATTENTION_CHG_EVENT;
  evt.elementId = elementId;
  evt.attentionOn = TRUE;

  meshCb.evtCback((meshEvt_t*)&evt);

  /* Start WSF timer at 1 second. */
  WsfTimerStartSec(&(localCfgElement.pAttTmrArray[elementId].attTmr), 1);

}

/*************************************************************************************************/
/*!
 *  \brief     Signals that attention has been turned off for an element
 *             and stops attention timer if needed.
 *
 *  \param[in] elementId  Local element identifier.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshLocalCfgStopAttention(meshElementId_t elementId)
{
  meshAttentionEvt_t evt;

  /* Signal event to the application */
  evt.hdr.event = MESH_CORE_EVENT;
  evt.hdr.param = MESH_CORE_ATTENTION_CHG_EVENT;
  evt.elementId = elementId;
  evt.attentionOn = FALSE;

  meshCb.evtCback((meshEvt_t*)&evt);

  /* Stop timer */
  WsfTimerStop(&(localCfgElement.pAttTmrArray[elementId].attTmr));
}

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler callback.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshLocalCfgWsfHandlerCback(wsfMsgHdr_t *pMsg)
{
  /* Check event type. */
  switch(pMsg->event)
  {
    case MESH_LOCAL_CFG_MSG_ATT_TMR_EXPIRED:
      /* Call timer callback to handle expiration. */
      meshLocalCfgAttentionTimerCback((uint8_t)(pMsg->param));
      break;
    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Empty event notification callback.
 *
 *  \param[in] event         Event.
 *  \param[in] pEventParams  Pointer to event parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshLocalCfgFriendSubscrEventNotifyCback(meshLocalCfgFriendSubscrEvent_t event,
                                                     const meshLocalCfgFriendSubscrEventParams_t *pEventParams)
{
  (void)event;
  (void)pEventParams;
}

/*************************************************************************************************/
/*!
 *  \brief     Get the local configuration of a model.
 *
 *  \param[in] elementId  Identifier of the element
 *  \param[in] modelId    SIG model Id / Vendor model Id union
 *  \param[in] isSig      TRUE if this is a SIG model, else FALSE
 *
 *  \return    If the model exists, returns a pointer to the model's local config structure.
 *             Else NULL
 */
/*************************************************************************************************/
static meshLocalCfgModelEntry_t* meshLocalCfgGetMdlCfg(meshElementId_t elementId,
                                                       modelId_t modelId, bool_t isSig)
{
  uint16_t i;
  meshModelId_t model;
  meshLocalCfgModelEntry_t *p = NULL;

  model.modelId = modelId;
  model.isSigModel = isSig;

  i = meshLocalCfgSearchModel(elementId, &model);

  if (i != MESH_INVALID_ENTRY_INDEX)
  {
    p = &localCfgModel.pModelArray[i];
  }

  return p;
}

/*************************************************************************************************/
/*!
 *  \brief     Initialize the local model configuration structure
 *
 *  \param[in] id                 Identifier of the element
 *  \param[in] idx                Pointer to the index of localCfgModel.pModelArray[]
 *  \param[in] appKeyBindListIdx  Pointer to the app key bind list index
 *  \param[in] subscrListIdx      Pointer to the subscription list index
 *  \param[in] sharedSubscrList   TRUE if only models tht use a shared subscription list should be
 *                                initialized. Else FALSE
 *
 *  \return    None
 */
/*************************************************************************************************/
static void meshLocalCfgInitModels(meshElementId_t id, uint16_t * pIdx,
                                   uint16_t * pAppKeyBindListIdx, uint16_t * pSubscrListIdx,
                                   bool_t sharedSubscrList)
{
  uint32_t j;
  const meshSigModel_t *pSigModel;
  const meshVendorModel_t *pVendModel;
  const meshLocalCfgModelEntry_t* pLinkCfg;
  meshLocalCfgModelEntry_t * pLocalMdlCfg;

  for (j = 0; j < pMeshConfig->pElementArray[id].numSigModels; j++)
  {
    /* Set local pointers */
    pSigModel = &pMeshConfig->pElementArray[id].pSigModelArray[j];
    pLocalMdlCfg = &localCfgModel.pModelArray[*pIdx];

    /* Filter only models that corresponds to the input parameters */
    if (((sharedSubscrList == TRUE) && (pSigModel->subscrListSize == MMDL_SUBSCR_LIST_SHARED)) ||
        ((sharedSubscrList == FALSE) && (pSigModel->subscrListSize != MMDL_SUBSCR_LIST_SHARED)))
    {
      pLocalMdlCfg->elementId = id;
      pLocalMdlCfg->modelId.modelId.sigModelId = pSigModel->modelId;
      pLocalMdlCfg->modelId.isSigModel = TRUE;
      pLocalMdlCfg->appKeyBindListStartIdx = *pAppKeyBindListIdx;
      pLocalMdlCfg->appKeyBindListSize = pSigModel->appKeyBindListSize;
      *pAppKeyBindListIdx += pLocalMdlCfg->appKeyBindListSize;
      pLocalMdlCfg->publicationState.publishAddressIndex = MESH_INVALID_ENTRY_INDEX;
      pLocalMdlCfg->publicationState.publishAppKeyEntryIndex = MESH_INVALID_ENTRY_INDEX;

      if (pSigModel->subscrListSize == MMDL_SUBSCR_LIST_SHARED)
      {
        /* Initialize models that use a shared subscription list */
        pLocalMdlCfg->subscrListSize = 0;
        pLocalMdlCfg->subscrListStartIdx = 0;

        WSF_ASSERT(pSigModel->pModelLink != NULL);

        if (pSigModel->pModelLink != NULL)
        {
          pLinkCfg = meshLocalCfgGetMdlCfg(pSigModel->pModelLink->rootElementId,
                                           pSigModel->pModelLink->rootModelId,
                                           pSigModel->pModelLink->isSig);

          WSF_ASSERT(pLinkCfg != NULL);

          if (pLinkCfg != NULL)
          {
            pLocalMdlCfg->subscrListSize = pLinkCfg->subscrListSize;
            pLocalMdlCfg->subscrListStartIdx = pLinkCfg->subscrListStartIdx;
          }
        }
      }
      else
      {
        /* Initialize root models that use a static subscription list */
        pLocalMdlCfg->subscrListSize = pSigModel->subscrListSize;
        pLocalMdlCfg->subscrListStartIdx = *pSubscrListIdx;
        *pSubscrListIdx += pLocalMdlCfg->subscrListSize;
      }

      (*pIdx)++;
    }
  }

  for (j = 0; j < pMeshConfig->pElementArray[id].numVendorModels; j++)
  {
    /* Set local pointers */
    pVendModel = &pMeshConfig->pElementArray[id].pVendorModelArray[j];
    pLocalMdlCfg = &localCfgModel.pModelArray[*pIdx];

    /* Filter only models that corresponds to the input parameters */
    if (((sharedSubscrList == TRUE) && (pVendModel->subscrListSize == MMDL_SUBSCR_LIST_SHARED)) ||
        ((sharedSubscrList == FALSE) && (pVendModel->subscrListSize != MMDL_SUBSCR_LIST_SHARED)))
    {
      pLocalMdlCfg->elementId = id;
      pLocalMdlCfg->modelId.modelId.vendorModelId = pVendModel->modelId;
      pLocalMdlCfg->modelId.isSigModel = FALSE;
      pLocalMdlCfg->appKeyBindListStartIdx = *pAppKeyBindListIdx;
      pLocalMdlCfg->appKeyBindListSize = pVendModel->appKeyBindListSize;
      *pAppKeyBindListIdx += pLocalMdlCfg->appKeyBindListSize;
      pLocalMdlCfg->publicationState.publishAddressIndex = MESH_INVALID_ENTRY_INDEX;
      pLocalMdlCfg->publicationState.publishAppKeyEntryIndex = MESH_INVALID_ENTRY_INDEX;

      if (pVendModel->subscrListSize == MMDL_SUBSCR_LIST_SHARED)
      {
        /* Initialize models that use a shared subscription list */
        pLocalMdlCfg->subscrListSize = 0;
        pLocalMdlCfg->subscrListStartIdx = 0;

        WSF_ASSERT(pVendModel->pModelLink != NULL);

        if (pVendModel->pModelLink != NULL)
        {
          pLinkCfg = meshLocalCfgGetMdlCfg(pVendModel->pModelLink->rootElementId,
                                           pVendModel->pModelLink->rootModelId,
                                           pVendModel->pModelLink->isSig);

          WSF_ASSERT(pLinkCfg != NULL);

          if (pLinkCfg != NULL)
          {
            pLocalMdlCfg->subscrListSize = pLinkCfg->subscrListSize;
            pLocalMdlCfg->subscrListStartIdx = pLinkCfg->subscrListStartIdx;
          }
        }
      }
      else
      {
        pLocalMdlCfg->subscrListSize = pVendModel->subscrListSize;
        pLocalMdlCfg->subscrListStartIdx = *pSubscrListIdx;
        *pSubscrListIdx += pLocalMdlCfg->subscrListSize;
      }

      (*pIdx)++;
    }
  }
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Computes the required memory to be provided based on the given configuration.
 *
 *  \return Required memory in bytes or ::MESH_MEM_REQ_INVALID_CFG in case of fail.
 */
/*************************************************************************************************/
uint32_t MeshLocalCfgGetRequiredMemory(void)
{
  uint32_t reqMem = MESH_MEM_REQ_INVALID_CFG;

  if ((pMeshConfig->pMemoryConfig == NULL) || (pMeshConfig->pElementArray == NULL))
  {
    return reqMem;
  }

  /* Compute required memory in bytes. */
  reqMem =
      meshLocalCfgGetRequiredMemoryAttTmrArray(pMeshConfig->elementArrayLen) +
      meshLocalCfgGetRequiredMemorySeqNumberArray(pMeshConfig->elementArrayLen) +
      meshLocalCfgGetRequiredMemorySeqNumberArray(pMeshConfig->elementArrayLen) +
      meshLocalCfgGetRequiredMemoryModelArray(meshLocalCfgGetTotalNumModels()) +
      meshLocalCfgGetRequiredMemorySubscrList(meshLocalCfgGetTotalSubscrListSize()) +
      meshLocalCfgGetRequiredMemoryAppKeyBindList(meshLocalCfgGetTotalAppKeyBindListSize()) +
      meshLocalCfgGetRequiredMemoryAddressList(pMeshConfig->pMemoryConfig->addrListMaxSize) +
      meshLocalCfgGetRequiredMemoryVirtualAddrList(pMeshConfig->pMemoryConfig->virtualAddrListMaxSize) +
      meshLocalCfgGetRequiredMemoryAppKeyList(pMeshConfig->pMemoryConfig->appKeyListSize) +
      meshLocalCfgGetRequiredMemoryNetKeyList(pMeshConfig->pMemoryConfig->netKeyListSize) +
      meshLocalCfgGetRequiredMemoryNodeIdentityList(pMeshConfig->pMemoryConfig->netKeyListSize);

  return reqMem;
}

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Local Configuration module and allocates memory.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshLocalCfgInit(void)
{
  uint32_t tempVal;
  uint16_t appKeyBindListStartIdx = 0;
  uint16_t subscrListStartIdx = 0;
  uint16_t i, k;
  uint8_t *pMemBuff;
  bool_t retVal;

  pMemBuff = meshCb.pMemBuff;

  /* Initialize Local Config local structure */
  memset(&localCfg, 0, sizeof(localCfg));

  /* Elements Initialization */
  /* Save the pointer for Element Array. */
  localCfgElement.pElementArray = pMeshConfig->pElementArray;
  /* Save the pointer to Local Config Element array. */
  localCfgElement.pAttTmrArray = (meshLocalCfgAttTmr_t *)pMemBuff;
  /* Save the Element Array size. */
  localCfgElement.elementArrayLen = pMeshConfig->elementArrayLen;
  /* Increment the memory buffer pointer. */
  tempVal = meshLocalCfgGetRequiredMemoryAttTmrArray(pMeshConfig->elementArrayLen);
  pMemBuff += tempVal;
  /* SEQ Number Array Initialization */
  /* Save the pointer to Local Config SEQ number array. */
  localCfgElement.pSeqNumberArray = (meshSeqNumber_t *)pMemBuff;
  /* Increment the memory buffer pointer. */
  tempVal = meshLocalCfgGetRequiredMemorySeqNumberArray(pMeshConfig->elementArrayLen);
  pMemBuff += tempVal;
  /* Initialize Local Config SEQ number array. */
  memset(localCfgElement.pSeqNumberArray, 0,
         sizeof(meshSeqNumber_t) * localCfgElement.elementArrayLen);
  /* SEQ Number Threshold Array Initialization */
  /* Save the pointer to Local Config SEQ number threshold array. */
  localCfgElement.pSeqNumberThreshArray = (meshSeqNumber_t *)pMemBuff;
  /* Increment the memory buffer pointer. */
  tempVal = meshLocalCfgGetRequiredMemorySeqNumberArray(pMeshConfig->elementArrayLen);
  pMemBuff += tempVal;
  /* Initialize Local Config SEQ number thresh array. */
  memset(localCfgElement.pSeqNumberThreshArray, 0,
         sizeof(meshSeqNumber_t) * localCfgElement.elementArrayLen);
  /* Address List Initialization. */
  /* Save the pointer for Address List. */
  localCfgAddressList.pAddressList = (meshLocalCfgAddressListEntry_t *)pMemBuff;
  /* Save the Address List size. */
  localCfgAddressList.addressListSize = pMeshConfig->pMemoryConfig->addrListMaxSize;
  /* Increment the memory buffer pointer. */
  tempVal = meshLocalCfgGetRequiredMemoryAddressList(pMeshConfig->pMemoryConfig->addrListMaxSize);
  pMemBuff += tempVal;
  /* Initialize the Address List. */
  for (i = 0; i < localCfgAddressList.addressListSize; i++)
  {
    localCfgAddressList.pAddressList[i].address = MESH_ADDR_TYPE_UNASSIGNED;
    localCfgAddressList.pAddressList[i].referenceCountPublish = 0;
    localCfgAddressList.pAddressList[i].referenceCountSubscr = 0;
  }

  /* Virtual Address List Initialization. */
  /* Save the pointer for Virtual Address List. */
  localCfgVirtualAddrList.pVirtualAddrList = (meshLocalCfgVirtualAddrListEntry_t *)pMemBuff;
  /* Save the Virtual Address List size. */
  localCfgVirtualAddrList.virtualAddrListSize = pMeshConfig->pMemoryConfig->virtualAddrListMaxSize;
  /* Increment the memory buffer pointer. */
  tempVal = meshLocalCfgGetRequiredMemoryVirtualAddrList(pMeshConfig->pMemoryConfig->virtualAddrListMaxSize);
  pMemBuff += tempVal;
  /* Initialize the Virtual Address List. */
  for (i = 0; i < localCfgAddressList.addressListSize; i++)
  {
    localCfgVirtualAddrList.pVirtualAddrList[i].address = MESH_ADDR_TYPE_UNASSIGNED;
    localCfgVirtualAddrList.pVirtualAddrList[i].referenceCountPublish = 0;
    localCfgVirtualAddrList.pVirtualAddrList[i].referenceCountSubscr = 0;
  }

  /* AppKey List Initialization. */
  /* Save the pointer for AppKey List. */
  localCfgAppKeyList.pAppKeyList = (meshLocalCfgAppKeyListEntry_t *)pMemBuff;
  /* Save the AppKey List size. */
  localCfgAppKeyList.appKeyListSize = pMeshConfig->pMemoryConfig->appKeyListSize;
  /* Increment the memory buffer pointer. */
  tempVal = meshLocalCfgGetRequiredMemoryAppKeyList(pMeshConfig->pMemoryConfig->appKeyListSize);
  pMemBuff += tempVal;
  /* Initialize the AppKey List. */
  for (i = 0; i < localCfgAppKeyList.appKeyListSize; i++)
  {
    localCfgAppKeyList.pAppKeyList[i].appKeyIndex = MESH_KEY_INVALID_INDEX;
    localCfgAppKeyList.pAppKeyList[i].netKeyEntryIndex = MESH_INVALID_ENTRY_INDEX;
  }
  /* NetKey List and Node Identity List Initialization. */
  /* Save the pointer for NetKey List. */
  localCfgNetKeyList.pNetKeyList = (meshLocalCfgNetKeyListEntry_t *)pMemBuff;
  /* Save the NetKey List size. */
  localCfgNetKeyList.netKeyListSize = pMeshConfig->pMemoryConfig->netKeyListSize;
  /* Increment the memory buffer pointer. */
  tempVal = meshLocalCfgGetRequiredMemoryNetKeyList(pMeshConfig->pMemoryConfig->netKeyListSize);
  pMemBuff += tempVal;
  /* Save the pointer for Node Identity List. */
  localCfgNetKeyList.pNodeIdentityList = (meshLocalCfgNodeIdentityListEntry_t *)pMemBuff;
  /* Increment the memory buffer pointer. */
  tempVal = meshLocalCfgGetRequiredMemoryNodeIdentityList(pMeshConfig->pMemoryConfig->netKeyListSize);
  pMemBuff += tempVal;
  /* Initialize the NetKey List and Node Identity List. */
  for (i = 0; i < localCfgNetKeyList.netKeyListSize; i++)
  {
    localCfgNetKeyList.pNetKeyList[i].netKeyIndex = MESH_KEY_INVALID_INDEX;
    localCfgNetKeyList.pNodeIdentityList[i] = MESH_NODE_IDENTITY_NOT_SUPPORTED;
  }
  /* AppKey Bind List Initialization. */
  /* Save the pointer for AppKeyBind List. */
  localCfgAppKeyBindList.pAppKeyBindList = (uint16_t *)pMemBuff;
  /* Save the AppKeyBind List size. */
  localCfgAppKeyBindList.appKeyBindListSize = meshLocalCfgGetTotalAppKeyBindListSize();
  /* Increment the memory buffer pointer. */
  tempVal = meshLocalCfgGetRequiredMemoryAppKeyBindList(localCfgAppKeyBindList.appKeyBindListSize);
  pMemBuff += tempVal;
  /* Initialize the AppKeyBind List. */
  for (i = 0; i < localCfgAppKeyBindList.appKeyBindListSize; i++)
  {
    localCfgAppKeyBindList.pAppKeyBindList[i] = MESH_KEY_INVALID_INDEX;
  }
  /* Subscription List Initialization. */
  /* Save the pointer for Subscription List. */
  localCfgSubscrList.pSubscrList = (meshLocalCfgModelSubscrListEntry_t *)pMemBuff;
  /* Save the Subscription List size. */
  localCfgSubscrList.subscrListSize = meshLocalCfgGetTotalSubscrListSize();
  /* Initialize the Subscription List. */
  for (i = 0; i < localCfgSubscrList.subscrListSize; i++)
  {
    localCfgSubscrList.pSubscrList[i].subscrAddressIndex = MESH_INVALID_ENTRY_INDEX;
  }
  /* Increment the memory buffer pointer. */
  tempVal = meshLocalCfgGetRequiredMemorySubscrList(localCfgSubscrList.subscrListSize);
  pMemBuff += tempVal;

  /* Model array Initialization. */
  /* Save the pointer for Model array. */
  localCfgModel.pModelArray = (meshLocalCfgModelEntry_t *)pMemBuff;
  /* Save the Model array size. */
  localCfgModel.modelArraySize = meshLocalCfgGetTotalNumModels();
  /* Increment the memory buffer pointer. */
  tempVal = meshLocalCfgGetRequiredMemoryModelArray(localCfgModel.modelArraySize);
  pMemBuff += tempVal;
  /* Initialize the Model array. */
  memset(localCfgModel.pModelArray, 0, sizeof(meshLocalCfgModelEntry_t) * localCfgModel.modelArraySize);

  /* Forward memory pointer. */
  meshCb.memBuffSize -= (pMemBuff - meshCb.pMemBuff);
  meshCb.pMemBuff = pMemBuff;

  k = 0;

  /* Populate local model configuration with root models (that use a static subscription list) */
  for (i = 0; i < pMeshConfig->elementArrayLen; i++)
  {
    meshLocalCfgInitModels((meshElementId_t)i, &k, &appKeyBindListStartIdx, &subscrListStartIdx, FALSE);
  }

  /* Populate local model configuration with models that use a shared subscription list */
  for (i = 0; i < pMeshConfig->elementArrayLen; i++)
  {
    meshLocalCfgInitModels((meshElementId_t)i, &k, &appKeyBindListStartIdx, &subscrListStartIdx, TRUE);
  }

  /* Initialize Heartbeat local structure. */
  memset(&localCfgHb, 0, sizeof(localCfgHb));
  localCfgHb.pubDstAddressIndex = MESH_INVALID_ENTRY_INDEX;
  localCfgHb.subSrcAddressIndex = MESH_INVALID_ENTRY_INDEX;
  localCfgHb.subDstAddressIndex = MESH_INVALID_ENTRY_INDEX;
  localCfgHb.pubNetKeyEntryIndex = MESH_INVALID_ENTRY_INDEX;

  /* Initialize Attention Timer array. */
  for(i = 0; i < pMeshConfig->elementArrayLen; i++)
  {
    localCfgElement.pAttTmrArray[i].attTmr.msg.event = MESH_LOCAL_CFG_MSG_ATT_TMR_EXPIRED;
    localCfgElement.pAttTmrArray[i].attTmr.msg.param = i;
    localCfgElement.pAttTmrArray[i].attTmr.handlerId = meshCb.handlerId;
    localCfgElement.pAttTmrArray[i].remainingSec = 0;
  }

  localCfg.defaultTtl = 10;

  /* Set company ID to unused. */
  localCfg.prodInfo.companyId = 0xFFFF;

  /* Set GATT Proxy state to unsupported. */
  localCfg.gattProxyState = MESH_GATT_PROXY_FEATURE_NOT_SUPPORTED;

  /* Set Friend state to unsupported. */
  localCfg.friendState = MESH_FRIEND_FEATURE_NOT_SUPPORTED;

  retVal = WsfNvmReadData((uint64_t)MESH_LOCAL_CFG_NVM_DATASET_ID, (uint8_t *)&localCfg,
                             sizeof(localCfg), NULL);

  retVal = WsfNvmReadData((uint64_t)MESH_LOCAL_CFG_NVM_NET_KEY_DATASET_ID, (uint8_t *)localCfgNetKeyList.pNetKeyList,
                             sizeof(meshLocalCfgNetKeyListEntry_t) * localCfgNetKeyList.netKeyListSize, NULL);

  retVal = WsfNvmReadData((uint64_t)MESH_LOCAL_CFG_NVM_APP_KEY_DATASET_ID, (uint8_t *)localCfgAppKeyList.pAppKeyList,
                             sizeof(meshLocalCfgAppKeyListEntry_t) * localCfgAppKeyList.appKeyListSize, NULL);

  retVal = WsfNvmReadData((uint64_t)MESH_LOCAL_CFG_NVM_APP_KEY_BIND_DATASET_ID, (uint8_t *)localCfgAppKeyBindList.pAppKeyBindList,
                             sizeof(uint16_t) * localCfgAppKeyBindList.appKeyBindListSize, NULL);

  retVal = WsfNvmReadData((uint64_t)MESH_LOCAL_CFG_NVM_ADDRESS_DATASET_ID, (uint8_t *)localCfgAddressList.pAddressList,
                             sizeof(meshLocalCfgAddressListEntry_t) * localCfgAddressList.addressListSize, NULL);

  retVal = WsfNvmReadData((uint64_t)MESH_LOCAL_CFG_NVM_VIRTUAL_ADDR_DATASET_ID, (uint8_t *)localCfgVirtualAddrList.pVirtualAddrList,
                             sizeof(meshLocalCfgVirtualAddrListEntry_t) * localCfgVirtualAddrList.virtualAddrListSize, NULL);

  retVal = WsfNvmReadData((uint64_t)MESH_LOCAL_CFG_NVM_SUBSCR_DATASET_ID, (uint8_t *)localCfgSubscrList.pSubscrList,
                             sizeof(meshLocalCfgModelSubscrListEntry_t) * localCfgSubscrList.subscrListSize, NULL);

  retVal = WsfNvmReadData((uint64_t)MESH_LOCAL_CFG_NVM_SEQ_NUMBER_DATASET_ID, (uint8_t *)localCfgElement.pSeqNumberArray,
                             sizeof(meshSeqNumber_t) * localCfgElement.elementArrayLen, NULL);


  retVal = WsfNvmReadData((uint64_t)MESH_LOCAL_CFG_NVM_SEQ_NUMBER_THRESH_DATASET_ID, (uint8_t *)localCfgElement.pSeqNumberThreshArray,
                             sizeof(meshSeqNumber_t) * localCfgElement.elementArrayLen, NULL);

  retVal = WsfNvmReadData((uint64_t)MESH_LOCAL_CFG_NVM_MODEL_DATASET_ID, (uint8_t *)localCfgModel.pModelArray,
                             sizeof(meshLocalCfgModelEntry_t) * localCfgModel.modelArraySize, NULL);

  retVal = WsfNvmReadData((uint64_t)MESH_LOCAL_CFG_NVM_HB_DATASET_ID, (uint8_t *)&localCfgHb, sizeof(localCfgHb), NULL);

  /* Register friendship callback. */
  localCfgCb.friendSubscrEventCback = meshLocalCfgFriendSubscrEventNotifyCback;

  /* Register WSF message handler. */
  meshCb.localCfgMsgCback = meshLocalCfgWsfHandlerCback;

  /* Suppress compile warnings. */
  (void)retVal;
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the LPN events callback.
 *
 *  \param[in] friendSubscrEventCback  Friend subscription event notification callback.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgRegisterLpn(meshLocalCfgFriendSubscrEventNotifyCback_t friendSubscrEventCback)
{
  if (friendSubscrEventCback != NULL)
  {
    localCfgCb.friendSubscrEventCback = friendSubscrEventCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the address for the primary node.
 *
 *  \param[in] address  Primary element address.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetPrimaryNodeAddress(meshAddress_t address)
{
    if (MESH_IS_ADDR_UNICAST(address))
    {
        localCfg.address = address;

        WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_DATASET_ID, (uint8_t *)&localCfg, sizeof(localCfg), NULL);

        return MESH_SUCCESS;
    }

    return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets a pointer to a constant local Mesh element based on element address.
 *
 *  \param[in]  elementAddress  Local element address.
 *  \param[out] ppOutElement    Pointer to store address of a constant local element or
 *                              NULL if element not found.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetElementFromAddress(meshAddress_t elementAddress,
                                                       const meshElement_t **ppOutElement)
{
  WSF_ASSERT(ppOutElement != NULL);

  /* Check for element address in range. */
  if ((elementAddress < localCfg.address) ||
      (elementAddress >= (localCfg.address + localCfgElement.elementArrayLen)))
  {
    return MESH_LOCAL_CFG_INVALID_PARAMS;
  }

  /* Element address was found. */
  *ppOutElement = &localCfgElement.pElementArray[elementAddress - localCfg.address];
  return MESH_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets a pointer to a constant local Mesh element based on element identifier in
 *              list.
 *
 *  \param[in]  elementId     Local element identifier.
 *  \param[out] ppOutElement  Pointer to store address of a constant local element or
 *                            NULL if element not found.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetElementFromId(meshElementId_t elementId,
                                                  const meshElement_t **ppOutElement)
{
  WSF_ASSERT(ppOutElement != NULL);

  /* Check if the element ID does not exceed the element array size. */
  if (elementId < localCfgElement.elementArrayLen)
  {
    *ppOutElement = &localCfgElement.pElementArray[elementId];
    return MESH_SUCCESS;
  }
  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets the Mesh Address from an element identifier.
 *
 *  \param[in]  elementId    Local element identifier.
 *  \param[out] pOutAddress  Pointer to store the address on success.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetAddrFromElementId(meshElementId_t elementId,
                                                      meshAddress_t *pOutAddress)
{
  WSF_ASSERT(pOutAddress != NULL);

  *pOutAddress = MESH_ADDR_TYPE_UNASSIGNED;

  /* Check if the element ID does not exceed the element array size. */
  if ((elementId < localCfgElement.elementArrayLen) && (MESH_IS_ADDR_UNICAST(localCfg.address)))
  {
    *pOutAddress = localCfg.address + elementId;
    return MESH_SUCCESS;
  }
  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets the Mesh element identifier from an unicast address.
 *
 *  \param[in]  elementAddress  Local element identifier.
 *  \param[out] pOutElementId   Pointer to memory where to store the id on success.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetElementIdFromAddr(meshAddress_t elementAddress,
                                                      meshElementId_t *pOutElementId)
{
  WSF_ASSERT(pOutElementId != NULL);

  /* Check for element address in range. */
  if ((elementAddress < localCfg.address) ||
      (elementAddress >= (localCfg.address + localCfgElement.elementArrayLen)))
  {
    return MESH_LOCAL_CFG_INVALID_PARAMS;
  }

  /* Element address was found. */
  *pOutElementId = (meshElementId_t)(elementAddress - localCfg.address);
  return MESH_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets generated Virtual Address associated to a Label UUID.
 *
 *  \param[in]  pLabelUuid        Pointer to the Label UUID location.
 *  \param[out] pPOutVirtualAddr  Pointer to store the virtual address or MESH_ADDR_TYPE_UNASSIGNED
 *                                if Label UUID not found.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetVirtualAddrFromLabelUuid(const uint8_t *pLabelUuid,
                                                             meshAddress_t *pOutVirtualAddr)
{
  uint16_t addrIdx;

  WSF_ASSERT(pLabelUuid != NULL);
  WSF_ASSERT(pOutVirtualAddr != NULL);

  /* Search through Virtual Address list. */
  for (addrIdx = 0; addrIdx < localCfgVirtualAddrList.virtualAddrListSize; addrIdx++)
  {
    /* Check if the address is not Unassigned.*/
    if (localCfgVirtualAddrList.pVirtualAddrList->address != MESH_ADDR_TYPE_UNASSIGNED)
    {
      /* Check if both Label UUIDs match. */
      if (memcmp(pLabelUuid, localCfgVirtualAddrList.pVirtualAddrList->labelUuid,
                 MESH_LABEL_UUID_SIZE) == 0x00)
      {
        *pOutVirtualAddr = localCfgVirtualAddrList.pVirtualAddrList->address;
        return MESH_SUCCESS;
      }
    }
  }
  return MESH_LOCAL_CFG_NOT_FOUND;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets product information.
 *
 *  \param[in] pProdInfo  Pointer to product information structure. See ::meshProdInfo_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetProductInformation(const meshProdInfo_t *pProdInfo)
{
  WSF_ASSERT(pProdInfo != NULL);

  localCfg.prodInfo.companyId = pProdInfo->companyId;
  localCfg.prodInfo.productId = pProdInfo->productId;
  localCfg.prodInfo.versionId = pProdInfo->versionId;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets product information.
 *
 *  \param[out] pOutProdInfo  Pointer to memory where to store the product information
 *                            (CID/PID/VID). See ::meshProdInfo_t
 *
 *  \return     None.
 */
/*************************************************************************************************/
void MeshLocalCfgGetProductInformation(meshProdInfo_t *pOutProdInfo)
{
  WSF_ASSERT(pOutProdInfo != NULL);

  pOutProdInfo->companyId = localCfg.prodInfo.companyId;
  pOutProdInfo->productId = localCfg.prodInfo.productId;
  pOutProdInfo->versionId = localCfg.prodInfo.versionId;
}

/*************************************************************************************************/
/*!
 *  \brief  Gets supported features from the stack.
 *
 *  \return Bit field feature support value. See ::meshFeatures_t.
 */
/*************************************************************************************************/
meshFeatures_t MeshLocalCfgGetSupportedFeatures(void)
{
  meshFeatures_t        features       = 0;
  meshRelayStates_t     relayState     = MeshLocalCfgGetRelayState();
  meshGattProxyStates_t gattProxyState = MeshLocalCfgGetGattProxyState();
  meshFriendStates_t    friendState    = MeshLocalCfgGetFriendState();
  meshLowPowerStates_t  lowPowerState  = MeshLocalCfgGetLowPowerState();

  /* Check for Relay feature. */
  if (relayState == MESH_RELAY_FEATURE_ENABLED)
  {
    features |= MESH_FEAT_RELAY;
  }

  /* Check for Proxy feature. */
  if (gattProxyState == MESH_GATT_PROXY_FEATURE_ENABLED)
  {
    features |= MESH_FEAT_PROXY;
  }

  /* Check for Friend feature. */
  if (friendState == MESH_FRIEND_FEATURE_ENABLED)
  {
    features |= MESH_FEAT_FRIEND;
  }

  /* Check for Low Power feature. */
  if (lowPowerState == MESH_LOW_POWER_FEATURE_ENABLED)
  {
    features |= MESH_FEAT_LOW_POWER;
  }

  return features;
}

/*************************************************************************************************/
/*!
 *  \brief     Determines if model instance exists on node.
 *
 *  \param[in] elementId  Local element identifier.
 *  \param[in] pModelId   Pointer to generic model identifier structure.
 *
 *  \return    TRUE if element contains an instance of the model.
 */
/*************************************************************************************************/
bool_t MeshLocalCfgModelExists(meshElementId_t elementId, const meshModelId_t *pModelId)
{
  uint16_t modelIdx;

  WSF_ASSERT(pModelId != NULL);

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);

  return (modelIdx != MESH_INVALID_ENTRY_INDEX);
}

/*************************************************************************************************/
/*!
 *  \brief     Sets publish non-virtual address based on a model ID.
 *
 *  \param[in] elementId       Local element identifier.
 *  \param[in] pModelId        Pointer to generic model identifier structure.
 *  \param[in] publishAddress  Publish address.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetPublishAddress(meshElementId_t elementId,
                                                   const meshModelId_t *pModelId,
                                                   meshAddress_t publishAddress)
{
  uint16_t modelIdx;
  uint16_t addrIdx;
  uint16_t newAddrIdx;
  bool_t   publishToLabelUuid;

  WSF_ASSERT(pModelId != NULL);
  WSF_ASSERT(!MESH_IS_ADDR_VIRTUAL(publishAddress));

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    addrIdx = localCfgModel.pModelArray[modelIdx].publicationState.publishAddressIndex;
    publishToLabelUuid =
      localCfgModel.pModelArray[modelIdx].publicationState.publishToLabelUuid;
    /* Check if address is UNASSIGNED. */
    if (MESH_IS_ADDR_UNASSIGNED(publishAddress))
    {
      /* Remove address from Model Publication state. */
      if (addrIdx != MESH_INVALID_ENTRY_INDEX)
      {
        meshLocalCfgRemoveAddress(addrIdx, publishToLabelUuid, TRUE);

        localCfgModel.pModelArray[modelIdx].publicationState.publishAddressIndex =
          MESH_INVALID_ENTRY_INDEX;
        localCfgModel.pModelArray[modelIdx].publicationState.publishToLabelUuid = FALSE;

        /* Update Model entry in NVM. */
        WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_MODEL_DATASET_ID, (uint8_t *)localCfgModel.pModelArray,
                                   sizeof(meshLocalCfgModelEntry_t) * localCfgModel.modelArraySize, NULL);
      }

      return MESH_SUCCESS;
    }
    else
    {
      /* Add address to Model Publication state. */
      /* Search for address. */
      newAddrIdx = meshLocalCfgGetAddressEntryIndex(publishAddress, NULL);
      /* Check if the new address and the old address are the same. */
      if ((newAddrIdx == addrIdx) && (newAddrIdx != MESH_INVALID_ENTRY_INDEX))
      {
        return MESH_SUCCESS;
      }

      /* Check index and add the new address if not found as it may fail with out of memory. */
      if (newAddrIdx == MESH_INVALID_ENTRY_INDEX)
      {
        /* Set Address in Address List. */
        newAddrIdx = meshLocalCfgSetAddress(publishAddress, NULL, TRUE);
      }
      else
      {
        /* Increment publication count. */
        localCfgAddressList.pAddressList[newAddrIdx].referenceCountPublish++;

        /* Update Address entry in NVM. */
        WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_ADDRESS_DATASET_ID, (uint8_t *)localCfgAddressList.pAddressList,
                                   sizeof(meshLocalCfgAddressListEntry_t) * localCfgAddressList.addressListSize, NULL);
      }

      /* Check again if Address List is full. */
      if (newAddrIdx == MESH_INVALID_ENTRY_INDEX)
      {
        return MESH_LOCAL_CFG_OUT_OF_MEMORY;
      }

      /* Another publish address is set, first remove that one. */
      if (addrIdx != MESH_INVALID_ENTRY_INDEX)
      {
        meshLocalCfgRemoveAddress(addrIdx, publishToLabelUuid, TRUE);
      }

      /* Add Index to module. */
      localCfgModel.pModelArray[modelIdx].publicationState.publishAddressIndex =
        newAddrIdx;
      localCfgModel.pModelArray[modelIdx].publicationState.publishToLabelUuid = FALSE;

      /* Update Model entry in NVM. */
      WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_MODEL_DATASET_ID, (uint8_t *)localCfgModel.pModelArray,
                                 sizeof(meshLocalCfgModelEntry_t) * localCfgModel.modelArraySize, NULL);

      return MESH_SUCCESS;
    }
  }

  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets publish address based on a model ID.
 *
 *  \param[in]  elementId           Local element identifier.
 *  \param[in]  pModelId            Pointer to generic model identifier structure.
 *  \param[out] pOutPublishAddress  Pointer to memory where to store the publish address.
 *  \param[out] ppOutLabelUuid      Pointer to memory where to store the memory address of the Label
 *                                  UUID if address is virtual.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetPublishAddress(meshElementId_t elementId,
                                                   const meshModelId_t *pModelId,
                                                   meshAddress_t *pOutPublishAddress,
                                                   uint8_t **ppOutLabelUuid)
{
  uint16_t modelIdx;
  uint16_t addrIdx;

  WSF_ASSERT(pModelId != NULL);
  WSF_ASSERT(pOutPublishAddress != NULL);
  WSF_ASSERT(ppOutLabelUuid != NULL);

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    addrIdx = localCfgModel.pModelArray[modelIdx].publicationState.publishAddressIndex;

    if (addrIdx != MESH_INVALID_ENTRY_INDEX)
    {
      if (localCfgModel.pModelArray[modelIdx].publicationState.publishToLabelUuid == FALSE)
      {
        *pOutPublishAddress = localCfgAddressList.pAddressList[addrIdx].address;
        *ppOutLabelUuid     = NULL;
      }
      else
      {
        *pOutPublishAddress = localCfgVirtualAddrList.pVirtualAddrList[addrIdx].address;
        *ppOutLabelUuid     = localCfgVirtualAddrList.pVirtualAddrList[addrIdx].labelUuid;
      }
      return MESH_SUCCESS;
    }
    else
    {
      /* Publish Address is unset. Return unassigned address. */
      *pOutPublishAddress = MESH_ADDR_TYPE_UNASSIGNED;
      *ppOutLabelUuid     = NULL;
      return MESH_SUCCESS;
    }
  }
  return MESH_LOCAL_CFG_NOT_FOUND;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets publish virtual address based on a model ID.
 *
 *  \param[in] elementId    Local element identifier.
 *  \param[in] pModelId     Pointer to generic model identifier structure.
 *  \param[in] pLabelUuid   Pointer to Label UUID.
 *  \param[in] virtualAddr  Virtual address for the Label UUID.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetPublishVirtualAddr(meshElementId_t elementId,
                                                       const meshModelId_t *pModelId,
                                                       const uint8_t *pLabelUuid,
                                                       meshAddress_t virtualAddr)
{
  uint16_t modelIdx;
  uint16_t addrIdx;
  uint16_t newAddrIdx;
  bool_t   publishToLabelUuid;

  WSF_ASSERT(pModelId != NULL);
  WSF_ASSERT(pLabelUuid != NULL);
  WSF_ASSERT(MESH_IS_ADDR_VIRTUAL(virtualAddr) || MESH_IS_ADDR_UNASSIGNED(virtualAddr));

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    addrIdx = localCfgModel.pModelArray[modelIdx].publicationState.publishAddressIndex;
    publishToLabelUuid =
      localCfgModel.pModelArray[modelIdx].publicationState.publishToLabelUuid;

    /* Check if address is UNASSIGNED. */
    if (MESH_IS_ADDR_UNASSIGNED(virtualAddr))
    {
      /* Remove address from Model Publication state. */
      if (addrIdx != MESH_INVALID_ENTRY_INDEX)
      {
        meshLocalCfgRemoveAddress(addrIdx, publishToLabelUuid, TRUE);

        localCfgModel.pModelArray[modelIdx].publicationState.publishAddressIndex =
          MESH_INVALID_ENTRY_INDEX;
        localCfgModel.pModelArray[modelIdx].publicationState.publishToLabelUuid = FALSE;

        /* Update Model entry in NVM. */
        WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_MODEL_DATASET_ID, (uint8_t *)localCfgModel.pModelArray,
                                   sizeof(meshLocalCfgModelEntry_t) * localCfgModel.modelArraySize, NULL);
      }

      return MESH_SUCCESS;
    }
    else
    {
      /* Search for address. */
      newAddrIdx = meshLocalCfgGetAddressEntryIndex(virtualAddr, pLabelUuid);
      /* Check if the new address and the old address are the same. */
      if ((newAddrIdx == addrIdx) && (newAddrIdx != MESH_INVALID_ENTRY_INDEX))
      {
        return MESH_SUCCESS;
      }
      /* Check index and add the new address if not found as it may fail with out of memory. */
      if (newAddrIdx == MESH_INVALID_ENTRY_INDEX)
      {
        /* Set Address in Address List. */
        newAddrIdx = meshLocalCfgSetAddress(virtualAddr, pLabelUuid, TRUE);
      }
      else
      {
        /* Increment publication count. */
        localCfgVirtualAddrList.pVirtualAddrList[newAddrIdx].referenceCountPublish++;

        /* Update Address entry in NVM. */
        WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_VIRTUAL_ADDR_DATASET_ID, (uint8_t *)localCfgVirtualAddrList.pVirtualAddrList,
                                   sizeof(meshLocalCfgVirtualAddrListEntry_t) * localCfgVirtualAddrList.virtualAddrListSize, NULL);
      }

      /* Check again if Address List is full. */
      if (newAddrIdx == MESH_INVALID_ENTRY_INDEX)
      {
        return MESH_LOCAL_CFG_OUT_OF_MEMORY;
      }

      /* Another publish address is set, first remove that one. */
      if (addrIdx != MESH_INVALID_ENTRY_INDEX)
      {
        meshLocalCfgRemoveAddress(addrIdx, publishToLabelUuid, TRUE);
      }

      /* Add Index to module. */
      localCfgModel.pModelArray[modelIdx].publicationState.publishAddressIndex =
        newAddrIdx;
      localCfgModel.pModelArray[modelIdx].publicationState.publishToLabelUuid = TRUE;

      /* Update Model entry in NVM. */
      WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_MODEL_DATASET_ID, (uint8_t *)localCfgModel.pModelArray,
                                 sizeof(meshLocalCfgModelEntry_t) * localCfgModel.modelArraySize, NULL);

      return MESH_SUCCESS;
    }
  }

  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets model publish period in number of steps and step resolution.
 *
 *  \param[in] elementId       Local element identifier.
 *  \param[in] pModelId        Pointer to generic model identifier structure.
 *  \param[in] numberOfSteps   Number of steps. See ::meshPublishPeriodNumSteps_t
 *  \param[in] stepResolution  The resolution of the number of steps field.
 *                             See ::meshPublishPeriodStepRes_t
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetPublishPeriod(meshElementId_t elementId,
                                                  const meshModelId_t *pModelId,
                                                  meshPublishPeriodNumSteps_t numberOfSteps,
                                                  meshPublishPeriodStepRes_t stepResolution)
{
  uint16_t modelIdx;

  WSF_ASSERT(pModelId != NULL);

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    localCfgModel.pModelArray[modelIdx].publicationState.publishPeriodNumSteps = numberOfSteps;
    localCfgModel.pModelArray[modelIdx].publicationState.publishPeriodStepRes = stepResolution;

    /* Update Model entry in NVM. */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_MODEL_DATASET_ID, (uint8_t *)localCfgModel.pModelArray,
                               sizeof(meshLocalCfgModelEntry_t) * localCfgModel.modelArraySize, NULL);

    return MESH_SUCCESS;
  }
  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets model publish period in number of steps and step resolution.
 *
 *  \param[in]  elementId           Local element identifier.
 *  \param[in]  pModelId            Pointer to generic model identifier structure.
 *  \param[out] pOutNumberOfSteps   Pointer to store the number of steps.
 *                                  See ::meshPublishPeriodNumSteps_t
 *  \param[out] pOutStepResolution  Pointer to store the resolution of the number of steps field.
 *                                  See :: meshPublishPeriodStepRes_t
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetPublishPeriod(meshElementId_t elementId,
                                                  const meshModelId_t *pModelId,
                                                  meshPublishPeriodNumSteps_t *pOutNumberOfSteps,
                                                  meshPublishPeriodStepRes_t *pOutStepResolution)
{
  uint16_t modelIdx;

  WSF_ASSERT(pModelId != NULL);
  WSF_ASSERT(pOutNumberOfSteps != NULL);
  WSF_ASSERT(pOutStepResolution != NULL);

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
    /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    *pOutNumberOfSteps =
      localCfgModel.pModelArray[modelIdx].publicationState.publishPeriodNumSteps;
    *pOutStepResolution =
      localCfgModel.pModelArray[modelIdx].publicationState.publishPeriodStepRes;

    return MESH_SUCCESS;
  }
  return MESH_LOCAL_CFG_NOT_FOUND;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets model publish AppKey Index.
 *
 *  \param[in] elementId    Local element identifier.
 *  \param[in] pModelId     Pointer to generic model identifier structure.
 *  \param[in] appKeyIndex  AppKey Index for model publication.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetPublishAppKeyIndex(meshElementId_t elementId,
                                                       const meshModelId_t *pModelId,
                                                       uint16_t appKeyIndex)
{
  uint16_t modelIdx;
  uint16_t appKeyIdx;

  WSF_ASSERT(pModelId != NULL);

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    /* Search for AppKeyIndex in list. */
    appKeyIdx = meshLocalGetKeyEntryIndex(appKeyIndex, FALSE);
    /* AppKeyIndex found. */
    if (appKeyIdx != MESH_INVALID_ENTRY_INDEX)
    {
      localCfgModel.pModelArray[modelIdx].publicationState.publishAppKeyEntryIndex =
        appKeyIdx;

      /* Update Model entry in NVM. */
      WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_MODEL_DATASET_ID, (uint8_t *)localCfgModel.pModelArray,
                                 sizeof(meshLocalCfgModelEntry_t) * localCfgModel.modelArraySize, NULL);

      return MESH_SUCCESS;
    }
  }
  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief     Un-sets model publish AppKey Index.
 *
 *  \param[in] elementId  Local element identifier.
 *  \param[in] pModelId   Pointer to generic model identifier structure.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgMdlClearPublishAppKeyIndex(meshElementId_t elementId, const meshModelId_t *pModelId)
{
  uint16_t modelIdx;

  WSF_ASSERT(pModelId != NULL);

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    localCfgModel.pModelArray[modelIdx].publicationState.publishAppKeyEntryIndex
      = MESH_INVALID_ENTRY_INDEX;
    /* Update Model entry in NVM. */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_MODEL_DATASET_ID, (uint8_t *)localCfgModel.pModelArray,
                               sizeof(meshLocalCfgModelEntry_t) * localCfgModel.modelArraySize, NULL);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Gets model publish AppKey Index.
 *
 *  \param[in]  elementId        Local element identifier.
 *  \param[in]  pModelId         Pointer to generic model identifier structure.
 *  \param[out] pOutAppKeyIndex  Pointer to memory where to store publish AppKeyIndex.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetPublishAppKeyIndex(meshElementId_t elementId,
                                                       const meshModelId_t *pModelId,
                                                       uint16_t *pOutAppKeyIndex)
{
  uint16_t modelIdx;
  uint16_t appKeyIdx;
  WSF_ASSERT(pModelId != NULL);
  WSF_ASSERT(pOutAppKeyIndex != NULL);

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    appKeyIdx = localCfgModel.pModelArray[modelIdx].publicationState.publishAppKeyEntryIndex;
    if (appKeyIdx != MESH_INVALID_ENTRY_INDEX)
    {
      /* Store AppKeyIndex. */
      *pOutAppKeyIndex = localCfgAppKeyList.pAppKeyList[appKeyIdx].appKeyIndex;
      return MESH_SUCCESS;
    }
  }
  return MESH_LOCAL_CFG_NOT_FOUND;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets model publish friendship security credential flag.
 *
 *  \param[in] elementId           Local element identifier.
 *  \param[in] pModelId            Pointer to generic model identifier structure.
 *  \param[in] friendshipCredFlag  Publish friendship security credential flag.
 *                                 See ::meshPublishFriendshipCred_t
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetPublishFriendshipCredFlag(meshElementId_t elementId,
                                                              const meshModelId_t *pModelId,
                                                              meshPublishFriendshipCred_t friendshipCredFlag)
{
  uint16_t modelIdx;

  WSF_ASSERT(pModelId != NULL);

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    localCfgModel.pModelArray[modelIdx].publicationState.publishFriendshipCred = friendshipCredFlag;

    /* Update Model entry in NVM. */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_MODEL_DATASET_ID, (uint8_t *)localCfgModel.pModelArray,
                               sizeof(meshLocalCfgModelEntry_t) * localCfgModel.modelArraySize, NULL);

    return MESH_SUCCESS;
  }
  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets model publish friendship security credential flag.
 *
 *  \param[in]  elementId               Local element identifier.
 *  \param[in]  pModelId                Pointer to generic model identifier structure.
 *  \param[out] pOutFriendshipCredFlag  Pointer to memory where to store publish friendship credential
 *                                      flag. See ::meshPublishFriendshipCred_t
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetPublishFriendshipCredFlag(meshElementId_t elementId,
                                                              const meshModelId_t *pModelId,
                                                              meshPublishFriendshipCred_t *pOutFriendshipCredFlag)
{
  uint16_t modelIdx;

  WSF_ASSERT(pModelId != NULL);
  WSF_ASSERT(pOutFriendshipCredFlag != NULL);

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    *pOutFriendshipCredFlag =
      localCfgModel.pModelArray[modelIdx].publicationState.publishFriendshipCred;
    return MESH_SUCCESS;
  }
  return MESH_LOCAL_CFG_NOT_FOUND;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets model publish TTL.
 *
 *  \param[in] elementId   Local element identifier.
 *  \param[in] pModelId    Pointer to generic model identifier structure.
 *  \param[in] publishTtl  Publish TTL value.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetPublishTtl(meshElementId_t elementId,
                                               const meshModelId_t *pModelId,
                                               uint8_t publishTtl)
{
  uint16_t modelIdx;

  WSF_ASSERT(pModelId != NULL);

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    localCfgModel.pModelArray[modelIdx].publicationState.publishTtl = publishTtl;

    /* Update Model entry in NVM. */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_MODEL_DATASET_ID, (uint8_t *)localCfgModel.pModelArray,
                               sizeof(meshLocalCfgModelEntry_t) * localCfgModel.modelArraySize, NULL);

    return MESH_SUCCESS;
  }
  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets model publish TTL.
 *
 *  \param[in]  elementId       Local element identifier.
 *  \param[in]  pModelId        Pointer to generic model identifier structure.
 *  \param[out] pOutPublishTtl  Pointer to memory where to store publish TTL.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetPublishTtl(meshElementId_t elementId,
                                               const meshModelId_t *pModelId,
                                               uint8_t *pOutPublishTtl)
{
  uint16_t modelIdx;

  WSF_ASSERT(pModelId != NULL);
  WSF_ASSERT(pOutPublishTtl != NULL);

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    *pOutPublishTtl = localCfgModel.pModelArray[modelIdx].publicationState.publishTtl;
    return MESH_SUCCESS;
  }
  return MESH_LOCAL_CFG_NOT_FOUND;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets model publish retransmit count.
 *
 *  \param[in] elementId     Local element identifier.
 *  \param[in] pModelId      Pointer to generic model identifier structure
 *  \param[in] retransCount  Publish retransmit count value. See ::meshPublishRetransCount_t
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetPublishRetransCount(meshElementId_t elementId,
                                                        const meshModelId_t *pModelId,
                                                        meshPublishRetransCount_t retransCount)
{
  uint16_t modelIdx;

  WSF_ASSERT(pModelId != NULL);

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    localCfgModel.pModelArray[modelIdx].publicationState.publishRetransCount = retransCount;

    /* Update Model entry in NVM. */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_MODEL_DATASET_ID, (uint8_t *)localCfgModel.pModelArray,
                               sizeof(meshLocalCfgModelEntry_t) * localCfgModel.modelArraySize, NULL);

    return MESH_SUCCESS;
  }

  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets model publish retransmit count.
 *
 *  \param[in]  elementId         Local element identifier.
 *  \param[in]  pModelId          Pointer to generic model identifier structure.
 *  \param[out] pOutRetransCount  Point to publish retransmit count value.
 *                                See ::meshPublishRetransCount_t
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetPublishRetransCount(meshElementId_t elementId,
                                                        const meshModelId_t *pModelId,
                                                        meshPublishRetransCount_t *pOutRetransCount)
{
  uint16_t modelIdx;

  WSF_ASSERT(pModelId != NULL);
  WSF_ASSERT(pOutRetransCount != NULL);

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    *pOutRetransCount =
      localCfgModel.pModelArray[modelIdx].publicationState.publishRetransCount;
    return MESH_SUCCESS;
  }

  return MESH_LOCAL_CFG_NOT_FOUND;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets model publish retransmission interval steps.
 *
 *  \param[in] elementId     Local element identifier.
 *  \param[in] pModelId      Pointer to generic model identifier structure.
 *  \param[in] retransSteps  Publish retransmit interval steps.
 *                           See ::meshPublishRetransIntvlSteps_t
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 *
 *  \remarks   The retransmission interval is calculated using the formula
 *             retransmitInterval = (publishRetransSteps + 1) * 50.
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetPublishRetransIntvlSteps(meshElementId_t elementId,
                                                             const meshModelId_t *pModelId,
                                                             meshPublishRetransIntvlSteps_t retransSteps)
{
  uint16_t modelIdx;

  WSF_ASSERT(pModelId != NULL);

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    localCfgModel.pModelArray[modelIdx].publicationState.publishRetransSteps50Ms =
      retransSteps;

    /* Update Model entry in NVM. */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_MODEL_DATASET_ID, (uint8_t *)localCfgModel.pModelArray,
                               sizeof(meshLocalCfgModelEntry_t) * localCfgModel.modelArraySize, NULL);

    return MESH_SUCCESS;
  }

  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets model publish retransmission interval steps.
 *
 *  \param[in]  elementId         Local element identifier.
 *  \param[in]  pModelId          Pointer to generic model identifier structure.
 *  \param[out] pOutRetransSteps  Pointer to publish retransmit interval steps value.
 *                                See ::meshPublishRetransIntvlSteps_t
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 *
 *  \remarks    The retransmission interval is calculated using the formula
 *              retransmitInterval = (publishRetransSteps + 1) * 50.
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetPublishRetransIntvlSteps(meshElementId_t elementId,
                                                             const meshModelId_t *pModelId,
                                                             meshPublishRetransIntvlSteps_t *pOutRetransSteps)
{
  uint16_t modelIdx;

  WSF_ASSERT(pModelId != NULL);
  WSF_ASSERT(pOutRetransSteps != NULL);

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);

  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    *pOutRetransSteps =
      localCfgModel.pModelArray[modelIdx].publicationState.publishRetransSteps50Ms;
    return MESH_SUCCESS;
  }

  return MESH_LOCAL_CFG_NOT_FOUND;
}

/*************************************************************************************************/
/*!
 *  \brief     Adds non-virtual address to the subscription list for an element.
 *
 *  \param[in] elementId     Local element identifier.
 *  \param[in] pModelId      Pointer to generic model identifier structure.
 *  \param[in] subscAddress  Subscription address.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgAddAddressToSubscrList(meshElementId_t elementId,
                                                        const meshModelId_t *pModelId,
                                                        meshAddress_t subscrAddress)
{
  uint16_t modelIdx;
  uint16_t subscrListOffset;
  uint16_t newAddrIdx;
  uint16_t freeIdx = MESH_INVALID_ENTRY_INDEX;
  uint16_t subscrIdx;

  WSF_ASSERT(pModelId != NULL);
  WSF_ASSERT((!MESH_IS_ADDR_UNASSIGNED(subscrAddress)) && (!MESH_IS_ADDR_VIRTUAL(subscrAddress)));

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    /* Search for address. */
    newAddrIdx = meshLocalCfgGetAddressEntryIndex(subscrAddress, NULL);

    /* Get the Model Subscription List offset. */
    subscrListOffset = localCfgModel.pModelArray[modelIdx].subscrListStartIdx;

    if (newAddrIdx == MESH_INVALID_ENTRY_INDEX)
    {
      /* Check for an empty slot. */
      for (subscrIdx = subscrListOffset;
           subscrIdx < subscrListOffset + localCfgModel.pModelArray[modelIdx].subscrListSize;
           subscrIdx++)
      {
        /* Empty slot found. */
        if (localCfgSubscrList.pSubscrList[subscrIdx].subscrAddressIndex == MESH_INVALID_ENTRY_INDEX)
        {
          /* Set Address in Address List. */
          newAddrIdx = meshLocalCfgSetAddress(subscrAddress, NULL, FALSE);

          /* Check again if Address List is full. */
          if (newAddrIdx == MESH_INVALID_ENTRY_INDEX)
          {
            return MESH_LOCAL_CFG_OUT_OF_MEMORY;
          }

          localCfgSubscrList.pSubscrList[subscrIdx].subscrAddressIndex = newAddrIdx;
          localCfgSubscrList.pSubscrList[subscrIdx].subscrToLabelUuid = FALSE;

          /* Update Subscription List entry in NVM. */
          WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_SUBSCR_DATASET_ID, (uint8_t *)localCfgSubscrList.pSubscrList,
                                       sizeof(meshLocalCfgModelSubscrListEntry_t) * localCfgSubscrList.subscrListSize, NULL);

          return MESH_SUCCESS;
        }
      }
      /* Reached the maximum list size. */
      return MESH_LOCAL_CFG_OUT_OF_MEMORY;
    }
    else
    {
      /* Address already in list, check if subscribed to this model. */
      for (subscrIdx = subscrListOffset;
           subscrIdx < subscrListOffset + localCfgModel.pModelArray[modelIdx].subscrListSize;
           subscrIdx++)
      {
        /* Address already subscribed. */
        if (localCfgSubscrList.pSubscrList[subscrIdx].subscrAddressIndex == newAddrIdx)
        {
          return MESH_LOCAL_CFG_ALREADY_EXIST;
        }
        /* Store the free index. */
        if ((localCfgSubscrList.pSubscrList[subscrIdx].subscrAddressIndex == MESH_INVALID_ENTRY_INDEX) &&
            (freeIdx == MESH_INVALID_ENTRY_INDEX))
        {
          freeIdx = subscrIdx;
        }
      }
      /* Reached the end of the search but the address was not found, check for free entry. */
      if ((freeIdx != MESH_INVALID_ENTRY_INDEX) &&
          (subscrIdx == subscrListOffset + localCfgModel.pModelArray[modelIdx].subscrListSize))
      {
        /* Increment subscription count. */
        localCfgAddressList.pAddressList[newAddrIdx].referenceCountSubscr++;

        /* Update Address entry in NVM. */
        WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_ADDRESS_DATASET_ID, (uint8_t *)localCfgAddressList.pAddressList,
                                   sizeof(meshLocalCfgAddressListEntry_t) * localCfgAddressList.addressListSize, NULL);

        localCfgSubscrList.pSubscrList[freeIdx].subscrAddressIndex = newAddrIdx;
        localCfgSubscrList.pSubscrList[freeIdx].subscrToLabelUuid = FALSE;

        /* Update Subscription List entry in NVM. */
        WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_SUBSCR_DATASET_ID, (uint8_t *)localCfgSubscrList.pSubscrList,
                                     sizeof(meshLocalCfgModelSubscrListEntry_t) * localCfgSubscrList.subscrListSize, NULL);

        return MESH_SUCCESS;
      }
      /* Check if it reached the maximum list size. */
      if (subscrIdx == subscrListOffset + localCfgModel.pModelArray[modelIdx].subscrListSize)
      {
        return MESH_LOCAL_CFG_OUT_OF_MEMORY;
      }
    }
  }

  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief         Gets address entry from a specific index in the subscription list for an element.
 *
 *  \param[in]     elementId         Local element identifier.
 *  \param[in]     pModelId          Pointer to generic model identifier structure.
 *  \param[out]    pOutSubscAddress  Pointer to the memory where to store the subscription address.
 *  \param[in,out] pInOutStartIndex  For input the index from where to start the search.
 *                                   For output the index where the search stopped.
 *
 *  \return        Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetNextAddressFromSubscrList(meshElementId_t elementId,
                                                              const meshModelId_t *pModelId,
                                                              meshAddress_t *pOutSubscAddress,
                                                              uint8_t *pInOutStartIndex)
{
  static uint16_t modelIdx = MESH_INVALID_ENTRY_INDEX;
  uint16_t subscrListOffset;
  uint16_t addrIdx;

  WSF_ASSERT(pModelId != NULL);
  WSF_ASSERT(pOutSubscAddress != NULL);
  WSF_ASSERT(pInOutStartIndex != NULL);

  /* Restart search when indexer is 0. */
  if (*pInOutStartIndex == 0)
  {
    /* Search for model in list. */
    modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
  }
  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    /* Search the subscription list starting from a specific position. */
    for ( ; *pInOutStartIndex < localCfgModel.pModelArray[modelIdx].subscrListSize;
         (*pInOutStartIndex)++)
    {
      /* Get the Model Subscription List offset. */
      subscrListOffset = localCfgModel.pModelArray[modelIdx].subscrListStartIdx;

      addrIdx =
        localCfgSubscrList.pSubscrList[subscrListOffset + *pInOutStartIndex].subscrAddressIndex;
      /* Check for valid entry. */
      if (addrIdx != MESH_INVALID_ENTRY_INDEX)
      {
        /* Check if subscription address is Virtual or Non-virtual. */
        if (localCfgSubscrList.pSubscrList[subscrListOffset + *pInOutStartIndex].subscrToLabelUuid
            == FALSE)
        {
          *pOutSubscAddress = localCfgAddressList.pAddressList[addrIdx].address;
          (*pInOutStartIndex)++;
        }
        else
        {
          *pOutSubscAddress = localCfgVirtualAddrList.pVirtualAddrList[addrIdx].address;
          (*pInOutStartIndex)++;
        }
        return MESH_SUCCESS;
      }
    }
    /* Start index exceeded or no more valid address found. */
    *pOutSubscAddress = MESH_ADDR_TYPE_UNASSIGNED;
    *pInOutStartIndex = 0x00;
    return MESH_LOCAL_CFG_NOT_FOUND;
  }

  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief     Removes an address from the subscription list for an element.
 *
 *  \param[in] elementId  Local element identifier.
 *  \param[in] pModelId   Pointer to generic model identifier structure.
 *  \param[in] address    Address to be removed.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgRemoveAddressFromSubscrList(meshElementId_t elementId,
                                                             const meshModelId_t *pModelId,
                                                             meshAddress_t address)
{
  uint16_t modelIdx;
  uint16_t addrIdx;
  uint16_t subscrListOffset;
  uint16_t subscrIdx;

  WSF_ASSERT(pModelId != NULL);
  WSF_ASSERT((!MESH_IS_ADDR_UNASSIGNED(address)) && (!MESH_IS_ADDR_VIRTUAL(address)));

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    /* Search for address. */
    addrIdx = meshLocalCfgGetAddressEntryIndex(address, NULL);

    if (addrIdx != MESH_INVALID_ENTRY_INDEX)
    {
      /* Get the Model Subscription List offset. */
      subscrListOffset = localCfgModel.pModelArray[modelIdx].subscrListStartIdx;

      /* Check for index in subscription list. */
      for (subscrIdx = subscrListOffset;
           subscrIdx < subscrListOffset + localCfgModel.pModelArray[modelIdx].subscrListSize;
           subscrIdx++)
      {
        if ((addrIdx == localCfgSubscrList.pSubscrList[subscrIdx].subscrAddressIndex) &&
            (localCfgSubscrList.pSubscrList[subscrIdx].subscrToLabelUuid == FALSE))
        {
          /* Address index found. Remove. */
          meshLocalCfgRemoveAddress(addrIdx, FALSE, FALSE);

          localCfgSubscrList.pSubscrList[subscrIdx].subscrAddressIndex = MESH_INVALID_ENTRY_INDEX;
          localCfgSubscrList.pSubscrList[subscrIdx].subscrToLabelUuid = FALSE;

          /* Update Subscription List entry in NVM. */
          WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_SUBSCR_DATASET_ID, (uint8_t *)localCfgSubscrList.pSubscrList,
                                       sizeof(meshLocalCfgModelSubscrListEntry_t) * localCfgSubscrList.subscrListSize, NULL);

          return MESH_SUCCESS;
        }
      }
    }
  }
  return MESH_LOCAL_CFG_NOT_FOUND;
}

/*************************************************************************************************/
/*!
 *  \brief     Adds multiple virtual address entries to the subscription list for an element.
 *
 *  \param[in] elementId    Local element identifier.
 *  \param[in] pModelId     Pointer to generic model identifier structure.
 *  \param[in] pLabelUuid   Pointer to Label UUID.
 *  \param[in] virtualAddr  Virtual address for the Label UUID.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgAddVirtualAddrToSubscrList(meshElementId_t elementId,
                                                            const meshModelId_t *pModelId,
                                                            const uint8_t *pLabelUuid,
                                                            meshAddress_t virtualAddr)
{
  uint16_t modelIdx;
  uint16_t newAddrIdx;
  uint16_t freeIdx = MESH_INVALID_ENTRY_INDEX;
  uint16_t subscrListOffset;
  uint16_t subscrIdx;

  WSF_ASSERT(pModelId != NULL);
  WSF_ASSERT(pLabelUuid != NULL);
  WSF_ASSERT(MESH_IS_ADDR_VIRTUAL(virtualAddr));

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    /* Search for address. */
    newAddrIdx = meshLocalCfgGetAddressEntryIndex(virtualAddr, pLabelUuid);

    /* Get the Model Subscription List offset. */
    subscrListOffset = localCfgModel.pModelArray[modelIdx].subscrListStartIdx;

    if (newAddrIdx == MESH_INVALID_ENTRY_INDEX)
    {
      /* Check for an empty slot. */
      for (subscrIdx = subscrListOffset;
           subscrIdx < subscrListOffset + localCfgModel.pModelArray[modelIdx].subscrListSize;
           subscrIdx++)
      {
        /* Empty slot found. */
        if (localCfgSubscrList.pSubscrList[subscrIdx].subscrAddressIndex == MESH_INVALID_ENTRY_INDEX)
        {
          /* Set Virtual Address in Virtual Address List. */
          newAddrIdx = meshLocalCfgSetAddress(virtualAddr, pLabelUuid, FALSE);

          /* Check again if Address List is full. */
          if (newAddrIdx == MESH_INVALID_ENTRY_INDEX)
          {
            return MESH_LOCAL_CFG_OUT_OF_MEMORY;
          }

          localCfgSubscrList.pSubscrList[subscrIdx].subscrAddressIndex = newAddrIdx;
          localCfgSubscrList.pSubscrList[subscrIdx].subscrToLabelUuid = TRUE;

          /* Update Subscription List entry in NVM. */
          WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_SUBSCR_DATASET_ID, (uint8_t *)localCfgSubscrList.pSubscrList,
                                       sizeof(meshLocalCfgModelSubscrListEntry_t) * localCfgSubscrList.subscrListSize, NULL);

          return MESH_SUCCESS;
        }
      }
      /* Reached the maximum list size. */
      return MESH_LOCAL_CFG_OUT_OF_MEMORY;
    }
    else
    {
      /* Address already in list, check if subscribed to this model. */
      for (subscrIdx = subscrListOffset;
           subscrIdx < subscrListOffset + localCfgModel.pModelArray[modelIdx].subscrListSize;
           subscrIdx++)
      {
        /* Address already subscribed. */
        if (localCfgSubscrList.pSubscrList[subscrIdx].subscrAddressIndex == newAddrIdx)
        {
          return MESH_LOCAL_CFG_ALREADY_EXIST;
        }
        /* Store the free index. */
        if ((localCfgSubscrList.pSubscrList[subscrIdx].subscrAddressIndex == MESH_INVALID_ENTRY_INDEX) &&
            (freeIdx == MESH_INVALID_ENTRY_INDEX))
        {
          freeIdx = subscrIdx;
        }
      }
      /* Reached the end of the search but the address was not found, check for free entry. */
      if ((freeIdx != MESH_INVALID_ENTRY_INDEX) &&
          (subscrIdx == subscrListOffset + localCfgModel.pModelArray[modelIdx].subscrListSize))
      {
        /* Increment subscription count. */
        localCfgVirtualAddrList.pVirtualAddrList[newAddrIdx].referenceCountSubscr++;

        /* Update Address entry in NVM. */
        WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_VIRTUAL_ADDR_DATASET_ID, (uint8_t *)localCfgVirtualAddrList.pVirtualAddrList,
                                   sizeof(meshLocalCfgVirtualAddrListEntry_t) * localCfgVirtualAddrList.virtualAddrListSize, NULL);

        localCfgSubscrList.pSubscrList[freeIdx].subscrAddressIndex = newAddrIdx;
        localCfgSubscrList.pSubscrList[freeIdx].subscrToLabelUuid = TRUE;

        /* Update Subscription List entry in NVM. */
        WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_SUBSCR_DATASET_ID, (uint8_t *)localCfgSubscrList.pSubscrList,
                                     sizeof(meshLocalCfgModelSubscrListEntry_t) * localCfgSubscrList.subscrListSize, NULL);

        return MESH_SUCCESS;
      }
      /* Check if it reached the maximum list size. */
      if (subscrIdx == subscrListOffset + localCfgModel.pModelArray[modelIdx].subscrListSize)
      {
        return MESH_LOCAL_CFG_OUT_OF_MEMORY;
      }
    }
  }

  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief     Removes an Label UUID from the subscription list for an element.
 *
 *  \param[in] elementId   Local element identifier.
 *  \param[in] pModelId    Pointer to generic model identifier structure.
 *  \param[in] pLabelUuid  Pointer to the Label UUID to be removed.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgRemoveVirtualAddrFromSubscrList(meshElementId_t elementId,
                                                                 const meshModelId_t *pModelId,
                                                                 const uint8_t *pLabelUuid)
{
  uint16_t modelIdx;
  uint16_t addrIdx;
  meshAddress_t virtualAddr;
  uint16_t subscrListOffset;
  uint16_t subscrIdx;

  WSF_ASSERT(pModelId != NULL);
  WSF_ASSERT(pLabelUuid != NULL);

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    if (MeshLocalCfgGetVirtualAddrFromLabelUuid(pLabelUuid, &virtualAddr) == MESH_SUCCESS)
    {
      /* Search for address. */
      addrIdx = meshLocalCfgGetAddressEntryIndex(virtualAddr, pLabelUuid);

      if (addrIdx != MESH_INVALID_ENTRY_INDEX)
      {
        /* Get the Model Subscription List offset. */
        subscrListOffset = localCfgModel.pModelArray[modelIdx].subscrListStartIdx;

        /* Check for index in subscription list. */
        for (subscrIdx = subscrListOffset;
             subscrIdx < subscrListOffset + localCfgModel.pModelArray[modelIdx].subscrListSize;
             subscrIdx++)
        {
          if ((addrIdx == localCfgSubscrList.pSubscrList[subscrIdx].subscrAddressIndex) &&
              (localCfgSubscrList.pSubscrList[subscrIdx].subscrToLabelUuid == TRUE))
          {
            /* Address index found. Remove. */
            meshLocalCfgRemoveAddress(addrIdx, TRUE, FALSE);

            localCfgSubscrList.pSubscrList[subscrIdx].subscrAddressIndex = MESH_INVALID_ENTRY_INDEX;
            localCfgSubscrList.pSubscrList[subscrIdx].subscrToLabelUuid = FALSE;

            /* Update Subscription List entry in NVM. */
            WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_SUBSCR_DATASET_ID, (uint8_t *)localCfgSubscrList.pSubscrList,
                                         sizeof(meshLocalCfgModelSubscrListEntry_t) * localCfgSubscrList.subscrListSize, NULL);

            return MESH_SUCCESS;
          }
        }
      }
    }
  }
  return MESH_LOCAL_CFG_NOT_FOUND;
}

/*************************************************************************************************/
/*!
 *  \brief     Removes all entries from the subscription list for an element.
 *
 *  \param[in] elementId  Local element identifier.
 *  \param[in] pModelId   Pointer to generic model identifier structure.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgRemoveAllFromSubscrList(meshElementId_t elementId,
                                                         const meshModelId_t *pModelId)
{
  uint16_t modelIdx;
  uint16_t addrIdx;
  uint16_t subscrListOffset;
  uint16_t subscrIdx;
  bool_t subscrToLabelUuid;

  WSF_ASSERT(pModelId != NULL);

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    /* Get the Model Subscription List offset. */
    subscrListOffset = localCfgModel.pModelArray[modelIdx].subscrListStartIdx;

    /* Search through Model's subscription list. */
    for (subscrIdx = subscrListOffset;
         subscrIdx < subscrListOffset + localCfgModel.pModelArray[modelIdx].subscrListSize;
         subscrIdx++)
    {
      addrIdx = localCfgSubscrList.pSubscrList[subscrIdx].subscrAddressIndex;
      subscrToLabelUuid = localCfgSubscrList.pSubscrList[subscrIdx].subscrToLabelUuid;

      if (addrIdx != MESH_INVALID_ENTRY_INDEX)
      {
        /* If valid subscription address index is found, remove address. */
        meshLocalCfgRemoveAddress(addrIdx, subscrToLabelUuid, FALSE);

        localCfgSubscrList.pSubscrList[subscrIdx].subscrAddressIndex = MESH_INVALID_ENTRY_INDEX;
        localCfgSubscrList.pSubscrList[subscrIdx].subscrToLabelUuid = FALSE;
      }
    }

    /* Update Subscription List in NVM. Sync as the Subscription list can be too large to send a
     * WSF message.
     */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_SUBSCR_DATASET_ID, (uint8_t *)localCfgSubscrList.pSubscrList,
                                 sizeof(meshLocalCfgModelSubscrListEntry_t) * localCfgSubscrList.subscrListSize, NULL);

    return MESH_SUCCESS;
  }

  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief     Checks if a model instance is subscribed to an address.
 *
 *  \param[in] elementId   Local element identifier.
 *  \param[in] pModelId    Pointer to generic model identifier structure.
 *  \param[in] subscrAddr  Subscription address.
 *  \param[in] pLabelUuid  Pointer to Label UUID for virtual subscription addresses.
 *
 *  \return    TRUE if address is found, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshLocalCfgFindAddrInModelSubscrList(meshElementId_t elementId,
                                             const meshModelId_t *pModelId,
                                             meshAddress_t subscrAddr,
                                             const uint8_t *pLabelUuid)
{
  uint16_t modelIdx;
  uint16_t addrIdx;
  uint16_t subscrListOffset;
  uint16_t subscrIdx;

  WSF_ASSERT(pModelId != NULL);

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);

  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    /* Get the Model Subscription List offset. */
    subscrListOffset = localCfgModel.pModelArray[modelIdx].subscrListStartIdx;

    /* Search through Model's subscription list. */
    for (subscrIdx = subscrListOffset;
         subscrIdx < subscrListOffset + localCfgModel.pModelArray[modelIdx].subscrListSize;
         subscrIdx++)
    {
      /* Get entry index in address list. */
      addrIdx = localCfgSubscrList.pSubscrList[subscrIdx].subscrAddressIndex;

      if (addrIdx == MESH_INVALID_ENTRY_INDEX)
      {
        continue;
      }

      /* Check if subscription address is virtual. */
      if (MESH_IS_ADDR_VIRTUAL(subscrAddr))
      {
        if (pLabelUuid != NULL)
        {
          /* Check if verified address is virtual. */
          if (localCfgSubscrList.pSubscrList[subscrIdx].subscrToLabelUuid)
          {
            /* Check for a match. */
            if ((localCfgVirtualAddrList.pVirtualAddrList[addrIdx].address == subscrAddr) &&
                (memcmp(localCfgVirtualAddrList.pVirtualAddrList[addrIdx].labelUuid, pLabelUuid,
                        MESH_LABEL_UUID_SIZE) == 0x00))
            {
              WSF_ASSERT(localCfgVirtualAddrList.pVirtualAddrList[addrIdx].referenceCountSubscr > 0);
              return TRUE;
            }
          }
        }
      }
      else
      {
        /* Check if verified address is non-virtual. */
        if (!localCfgSubscrList.pSubscrList[subscrIdx].subscrToLabelUuid)
        {
          if (localCfgAddressList.pAddressList[addrIdx].address == subscrAddr)
          {
            WSF_ASSERT(localCfgAddressList.pAddressList[addrIdx].referenceCountSubscr > 0);
            return TRUE;
          }
        }
      }
    }
  }
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets the subscription list available size and maximum size for an model instance.
 *
 *  \param[in]  elementId          Local element identifier.
 *  \param[in]  pModelId           Pointer to generic model identifier structure.
 *  \param[out] pOutNumAddr        Pointer to store the number of addresses in subscription list
 *                                 for the given element.
 *  \param[out] pOutTotalSize      Pointer to store the maximum size of the subscription list for
 *                                 the given element.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetSubscrListSize(meshElementId_t elementId,
                                                   const meshModelId_t *pModelId,
                                                   uint8_t *pOutNumAddr,
                                                   uint8_t *pOutTotalSize)
{
  uint16_t modelIdx;
  uint16_t addrIdx;
  uint16_t subscrListOffset;
  uint16_t subscrIdx;
  uint8_t countSize = 0;

  WSF_ASSERT(pModelId != NULL);

  /* Check if at least one output parameter is not NULL */
  if((pOutNumAddr != NULL) || (pOutTotalSize != NULL))
  {
    /* Search for model in list. */
    modelIdx = meshLocalCfgSearchModel(elementId, pModelId);

    /* Model found. */
    if (modelIdx != MESH_INVALID_ENTRY_INDEX)
    {
      /* Get the Model Subscription List offset. */
      subscrListOffset = localCfgModel.pModelArray[modelIdx].subscrListStartIdx;

      /* Search through Model's subscription list. */
      for (subscrIdx = subscrListOffset;
           subscrIdx < subscrListOffset + localCfgModel.pModelArray[modelIdx].subscrListSize;
           subscrIdx++)
      {
        addrIdx = localCfgSubscrList.pSubscrList[subscrIdx].subscrAddressIndex;

        if (addrIdx != MESH_INVALID_ENTRY_INDEX)
        {
          /* If valid address entry index found, increment count. */
          countSize++;
        }
      }

      if (pOutNumAddr != NULL)
      {
        *pOutNumAddr = countSize;
      }

      if (pOutTotalSize != NULL)
      {
        *pOutTotalSize = localCfgModel.pModelArray[modelIdx].subscrListSize;
      }
      return MESH_SUCCESS;
    }
  }
  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief     Searches for an address in the subscription lists.
 *
 *  \param[in] subscrAddr  Subscription Address to be searched.
 *
 *  \return    TRUE if the address is in any subscription list or FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshLocalCfgFindSubscrAddr(meshAddress_t subscrAddr)
{
  uint16_t addrIdx;

  /* Check if address is Virtual or Non-virtual. */
  if (!MESH_IS_ADDR_VIRTUAL(subscrAddr))
  {
    /* Search through non-virtual address list. */
    for (addrIdx = 0; addrIdx < localCfgAddressList.addressListSize; addrIdx++)
    {
      /* Check if subscription addresses match and if reference count is different than 0. */
      if ((localCfgAddressList.pAddressList[addrIdx].address == subscrAddr) &&
          (localCfgAddressList.pAddressList[addrIdx].referenceCountSubscr != 0))
      {
        return TRUE;
      }
    }
  }
  else
  {
    /* Search through virtual address list. */
    for (addrIdx = 0; addrIdx < localCfgVirtualAddrList.virtualAddrListSize; addrIdx++)
    {
      /* Check if subscription addresses match and if reference count is different than 0. */
      if ((localCfgVirtualAddrList.pVirtualAddrList[addrIdx].address == subscrAddr) &&
          (localCfgVirtualAddrList.pVirtualAddrList[addrIdx].referenceCountSubscr != 0))
      {
        return TRUE;
      }
    }
  }
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief  Checks if Subscription Address List is not empty.
 *
 *  \return TRUE if not empty, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshLocalCfgSubscrAddressListIsNotEmpty(void)
{
  uint16_t i;

  for (i = 0; i < localCfgAddressList.addressListSize; i++)
  {
    /* Check if entry is valid. */
    if ((localCfgAddressList.pAddressList[i].address != MESH_ADDR_TYPE_UNASSIGNED) &&
        (localCfgAddressList.pAddressList[i].referenceCountSubscr != 0))
    {
      return TRUE;
    }
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief  Checks if Subscription Virtual Address List is not empty.
 *
 *  \return TRUE if not empty, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshLocalCfgSubscrVirtualAddrListIsNotEmpty(void)
{
  uint16_t i;

  for (i = 0; i < localCfgVirtualAddrList.virtualAddrListSize; i++)
  {
    /* Check if entry is valid. */
    if ((localCfgVirtualAddrList.pVirtualAddrList[i].address != MESH_ADDR_TYPE_UNASSIGNED) &&
        (localCfgVirtualAddrList.pVirtualAddrList[i].referenceCountSubscr != 0))
    {
      return TRUE;
    }
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief         Gets subscription address entry from a specific index in the address list.
 *
 *  \param[out]    pOutSubscAddress  Pointer to the memory where to store the subscription address.
 *  \param[in,out] pInOutStartIndex  For input the index from where to start the search.
 *                                   For output the index where the search stopped.
 *
 *  \return        Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetNextSubscrAddress(meshAddress_t *pOutSubscAddress,
                                                      uint16_t *pInOutStartIndex)
{
  WSF_ASSERT(pOutSubscAddress != NULL);
  WSF_ASSERT(pInOutStartIndex);

  /* Iterate through the list. */
  for (; *pInOutStartIndex < localCfgAddressList.addressListSize; (*pInOutStartIndex)++)
  {
    /* Check if entry is valid. */
    if ((localCfgAddressList.pAddressList[*pInOutStartIndex].address != MESH_ADDR_TYPE_UNASSIGNED) &&
        (localCfgAddressList.pAddressList[*pInOutStartIndex].referenceCountSubscr != 0))
    {
      /* Store Address Index. */
      *pOutSubscAddress = localCfgAddressList.pAddressList[*pInOutStartIndex].address;
      /* Increment for future search. */
      (*pInOutStartIndex)++;
      return MESH_SUCCESS;
    }
  }

  return MESH_LOCAL_CFG_NOT_FOUND;
}

/*************************************************************************************************/
/*!
 *  \brief         Gets virtual subscription address entry from a specific index in the virtual
 *                 address list.
 *
 *  \param[out]    pOutSubscAddress  Pointer to the memory where to store the subscription address.
 *  \param[in,out] pInOutStartIndex  For input the index from where to start the search.
 *                                   For output the index where the search stopped.
 *
 *  \return        Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetNextSubscrVirtualAddr(meshAddress_t *pOutSubscAddress,
                                                          uint16_t *pInOutStartIndex)
{
  WSF_ASSERT(pOutSubscAddress != NULL);
  WSF_ASSERT(pInOutStartIndex);

  /* Iterate through the list. */
  for (; *pInOutStartIndex < localCfgVirtualAddrList.virtualAddrListSize; (*pInOutStartIndex)++)
  {
    /* Check if entry is valid. */
    if ((localCfgVirtualAddrList.pVirtualAddrList[*pInOutStartIndex].address != MESH_ADDR_TYPE_UNASSIGNED) &&
        (localCfgVirtualAddrList.pVirtualAddrList[*pInOutStartIndex].referenceCountSubscr != 0))
    {
      /* Store Address Index. */
      *pOutSubscAddress = localCfgVirtualAddrList.pVirtualAddrList[*pInOutStartIndex].address;
      /* Increment for future search. */
      (*pInOutStartIndex)++;
      return MESH_SUCCESS;
    }
  }

  return MESH_LOCAL_CFG_NOT_FOUND;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Device key.
 *
 *  \param[in] pDevKey  Pointer to the Device Key.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetDevKey(const uint8_t *pDevKey)
{
  WSF_ASSERT(pDevKey != NULL);

  memcpy(localCfg.deviceKey, pDevKey, MESH_KEY_SIZE_128);

  /* Update Local Cfg structure in NVM. */
  WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_DATASET_ID, (uint8_t *)&localCfg, sizeof(localCfg), NULL);
}

/*************************************************************************************************/
/*!
 *  \brief      Gets the Device key.
 *
 *  \param[out] pOutDevKey  Pointer to store the Device Key.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void MeshLocalCfgGetDevKey(uint8_t *pOutDevKey)
{
  WSF_ASSERT(pOutDevKey != NULL);

  memcpy(pOutDevKey, localCfg.deviceKey, MESH_KEY_SIZE_128);
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Network key and key index.
 *
 *  \param[in] netKeyIndex  Network key index.
 *  \param[in] pNetKey      Pointer to the Network Key.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetNetKey(uint16_t netKeyIndex, const uint8_t *pNetKey)
{
  uint16_t netKeyIdx;

  WSF_ASSERT(pNetKey != NULL);

  /* Check if the netKeyIndex is already in the list. */
  netKeyIdx = meshLocalGetKeyEntryIndex(netKeyIndex, TRUE);

  if (netKeyIdx != MESH_INVALID_ENTRY_INDEX)
  {
    return MESH_LOCAL_CFG_ALREADY_EXIST;
  }

  /* NetKey Index not found, check for first empty location. */
  netKeyIdx = meshLocalGetKeyFreeEntryIndex(TRUE);

  if (netKeyIdx != MESH_INVALID_ENTRY_INDEX)
  {
    /* If free entry found, update netKey entry. */
    memset(localCfgNetKeyList.pNetKeyList[netKeyIdx].netKeyNew, 0, MESH_KEY_SIZE_128);
    memcpy(localCfgNetKeyList.pNetKeyList[netKeyIdx].netKeyOld, pNetKey, MESH_KEY_SIZE_128);
    localCfgNetKeyList.pNetKeyList[netKeyIdx].newKeyAvailable = FALSE;
    localCfgNetKeyList.pNetKeyList[netKeyIdx].netKeyIndex = netKeyIndex;
    localCfgNetKeyList.pNetKeyList[netKeyIdx].keyRefreshState = MESH_KEY_REFRESH_NOT_ACTIVE;
    localCfgNetKeyList.pNodeIdentityList[netKeyIdx] = MESH_NODE_IDENTITY_STOPPED;

    /* Update NetKey list in NVM. */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_NET_KEY_DATASET_ID, (uint8_t *)localCfgNetKeyList.pNetKeyList,
                                 sizeof(meshLocalCfgNetKeyListEntry_t) * localCfgNetKeyList.netKeyListSize, NULL);

    return MESH_SUCCESS;
  }
  return MESH_LOCAL_CFG_OUT_OF_MEMORY;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the new Network key for a key index.
 *
 *  \param[in] netKeyIndex  Network key index.
 *  \param[in] pNetKey      Pointer to the Network Key.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgUpdateNetKey(uint16_t netKeyIndex, const uint8_t *pNetKey)
{
  uint16_t netKeyIdx;

  WSF_ASSERT(pNetKey != NULL);

  netKeyIdx = meshLocalGetKeyEntryIndex(netKeyIndex, TRUE);
  /* Check if the netKeyIndex is already in the list. */
  if (netKeyIdx != MESH_INVALID_ENTRY_INDEX)
  {
    /* Check if NetKey New flag is set and return error. */
    if (localCfgNetKeyList.pNetKeyList[netKeyIdx].newKeyAvailable == FALSE)
    {
      memcpy(localCfgNetKeyList.pNetKeyList[netKeyIdx].netKeyNew, pNetKey,
             MESH_KEY_SIZE_128);

      localCfgNetKeyList.pNetKeyList[netKeyIdx].newKeyAvailable = TRUE;

      /* Update NetKey list in NVM. */
      WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_NET_KEY_DATASET_ID, (uint8_t *)localCfgNetKeyList.pNetKeyList,
                      sizeof(meshLocalCfgNetKeyListEntry_t) * localCfgNetKeyList.netKeyListSize, NULL);

      return MESH_SUCCESS;
    }
    else
    {
      return MESH_LOCAL_CFG_ALREADY_EXIST;
    }
  }
  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief     Removes the Network key for a key index.
 *
 *  \param[in] netKeyIndex       Network key index.
 *  \param[in] removeOldKeyOnly  If TRUE, NetKey Old is replaced by NetKey New
 *                               If FALSE, the NetKey is removed from the NetKey List.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgRemoveNetKey(uint16_t netKeyIndex, bool_t removeOldKeyOnly)
{
  uint16_t netKeyIdx;
  uint16_t appKeyIdx;

  netKeyIdx = meshLocalGetKeyEntryIndex(netKeyIndex, TRUE);
  /* Check if the netKeyIndex is in the list. */
  if (netKeyIdx != MESH_INVALID_ENTRY_INDEX)
  {
    /* Check if only the old key should be removed. */
    if (removeOldKeyOnly)
    {
      WSF_ASSERT(localCfgNetKeyList.pNetKeyList[netKeyIdx].newKeyAvailable);

      if (localCfgNetKeyList.pNetKeyList[netKeyIdx].newKeyAvailable == TRUE)
      {
        /* Replace the old Key with the new Key. */
        memcpy(localCfgNetKeyList.pNetKeyList[netKeyIdx].netKeyOld,
               localCfgNetKeyList.pNetKeyList[netKeyIdx].netKeyNew,
               MESH_KEY_SIZE_128);
        localCfgNetKeyList.pNetKeyList[netKeyIdx].newKeyAvailable = FALSE;
      }
    }
    else
    {
      /* Check if the key is binded to an appKey and remove the entry from that list also. */
      for (appKeyIdx = 0; appKeyIdx < localCfgAppKeyList.appKeyListSize; appKeyIdx++)
      {
        if (netKeyIdx == localCfgAppKeyList.pAppKeyList[appKeyIdx].netKeyEntryIndex)
        {
          localCfgAppKeyList.pAppKeyList[appKeyIdx].netKeyEntryIndex = MESH_INVALID_ENTRY_INDEX;
        }
      }

      /* Remove the key entry from NetKey list. */
      localCfgNetKeyList.pNetKeyList[netKeyIdx].netKeyIndex = MESH_KEY_INVALID_INDEX;
      localCfgNetKeyList.pNetKeyList[netKeyIdx].newKeyAvailable = FALSE;
    }

    /* Update NetKey list in NVM. */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_NET_KEY_DATASET_ID, (uint8_t *)localCfgNetKeyList.pNetKeyList,
                                 sizeof(meshLocalCfgNetKeyListEntry_t) * localCfgNetKeyList.netKeyListSize, NULL);

    return MESH_SUCCESS;
  }
  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets the Network key for a key index.
 *
 *  \param[in]  netKeyIndex  Network key index.
 *  \param[out] pOutNetKey   Pointer to store the Network key.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetNetKey(uint16_t netKeyIndex, uint8_t *pOutNetKey)
{
  uint16_t netKeyIdx;

  WSF_ASSERT(pOutNetKey != NULL);

  netKeyIdx = meshLocalGetKeyEntryIndex(netKeyIndex, TRUE);
  /* Check if the netKeyIndex is in the list. */
  if (netKeyIdx != MESH_INVALID_ENTRY_INDEX)
  {
    memcpy(pOutNetKey, localCfgNetKeyList.pNetKeyList[netKeyIdx].netKeyOld, MESH_KEY_SIZE_128);
    return MESH_SUCCESS;
  }
  return MESH_LOCAL_CFG_NOT_FOUND;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets the updated Network key for a key index.
 *
 *  \param[in]  netKeyIndex  Network key index.
 *  \param[out] pOutNetKey   Pointer to store the updated Network key.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetUpdatedNetKey(uint16_t netKeyIndex, uint8_t *pOutNetKey)
{
  uint16_t netKeyIdx;

  WSF_ASSERT(pOutNetKey != NULL);

  netKeyIdx = meshLocalGetKeyEntryIndex(netKeyIndex, TRUE);
  /* Check if the netKeyIndex is in the list. */
  if ((netKeyIdx != MESH_INVALID_ENTRY_INDEX) &&
      (localCfgNetKeyList.pNetKeyList[netKeyIdx].newKeyAvailable == TRUE))
  {
    memcpy(pOutNetKey, localCfgNetKeyList.pNetKeyList[netKeyIdx].netKeyNew, MESH_KEY_SIZE_128);
    return MESH_SUCCESS;
  }
  return MESH_LOCAL_CFG_NOT_FOUND;
}

/*************************************************************************************************/
/*!
 *  \brief  Counts number of NetKeys on the node.
 *
 *  \return Number of NetKeys on the node.
 */
/*************************************************************************************************/
uint16_t MeshLocalCfgCountNetKeys(void)
{
  uint16_t netKeyIdx;
  uint16_t count = 0;

  for (netKeyIdx = 0; netKeyIdx < localCfgNetKeyList.netKeyListSize; netKeyIdx++)
  {
    if (localCfgNetKeyList.pNetKeyList[netKeyIdx].netKeyIndex != MESH_KEY_INVALID_INDEX)
    {
      count++;
    }
  }
  return count;
}

/*************************************************************************************************/
/*!
 *  \brief         Gets next NetKey Index.
 *
 *  \param[out]    pOutNetKeyIndex  Pointer to variable where the next NetKey Index is stored.
 *  \param[in,out] pInOutIndex      Pointer to an indexing variable. Must be initialized with 0 on
 *                                  to restart search.
 *
 *  \return        Success or error reason. \see meshLocalCfgRetVal_t
 *
 *  \remarks       This function behaves as an iterator, fetching one key index at a time.
 *
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetNextNetKeyIndex(uint16_t *pOutNetKeyIndex,
                                                    uint16_t *pInOutIndex)
{
  WSF_ASSERT(pOutNetKeyIndex != NULL);
  WSF_ASSERT(pInOutIndex);

  /* Iterate through the list. */
  for (; *pInOutIndex < localCfgNetKeyList.netKeyListSize; (*pInOutIndex)++)
  {
    /* Check if entry is valid. */
    if (localCfgNetKeyList.pNetKeyList[*pInOutIndex].netKeyIndex != MESH_KEY_INVALID_INDEX)
    {
      /* Store NetKey Index. */
      *pOutNetKeyIndex = localCfgNetKeyList.pNetKeyList[*pInOutIndex].netKeyIndex;
      /* Increment for future search. */
      (*pInOutIndex)++;
      return MESH_SUCCESS;
    }
  }

  return MESH_LOCAL_CFG_NOT_FOUND;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Application key and key index.
 *
 *  \param[in] appKeyIndex  Application key index.
 *  \param[in] pAppKey      Pointer to the Application Key.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetAppKey(uint16_t appKeyIndex, const uint8_t *pAppKey)
{
  uint16_t appKeyIdx;

  WSF_ASSERT(pAppKey != NULL);

  /* Check if the appKeyIndex is already in the list. */
  appKeyIdx = meshLocalGetKeyEntryIndex(appKeyIndex, FALSE);

  if (appKeyIdx != MESH_INVALID_ENTRY_INDEX)
  {
    return MESH_LOCAL_CFG_ALREADY_EXIST;
  }
  /* AppKeyIndex not found, check for first empty location. */
  appKeyIdx = meshLocalGetKeyFreeEntryIndex(FALSE);

  if (appKeyIdx != MESH_INVALID_ENTRY_INDEX)
  {
    /* If free entry found, update appKey entry. */
    memset(localCfgAppKeyList.pAppKeyList[appKeyIdx].appKeyNew, 0, MESH_KEY_SIZE_128);
    memcpy(localCfgAppKeyList.pAppKeyList[appKeyIdx].appKeyOld, pAppKey, MESH_KEY_SIZE_128);
    localCfgAppKeyList.pAppKeyList[appKeyIdx].newKeyAvailable = FALSE;
    localCfgAppKeyList.pAppKeyList[appKeyIdx].appKeyIndex = appKeyIndex;
    localCfgAppKeyList.pAppKeyList[appKeyIdx].netKeyEntryIndex = MESH_INVALID_ENTRY_INDEX;

    /* Update AppKey list in NVM. */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_APP_KEY_DATASET_ID, (uint8_t *)localCfgAppKeyList.pAppKeyList,
                                 sizeof(meshLocalCfgAppKeyListEntry_t) * localCfgAppKeyList.appKeyListSize, NULL);

    return MESH_SUCCESS;
  }

  return MESH_LOCAL_CFG_OUT_OF_MEMORY;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the new Application key for a key index.
 *
 *  \param[in] appKeyIndex  Application key index.
 *  \param[in] pAppKey      Pointer to the Application Key.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgUpdateAppKey(uint16_t appKeyIndex, const uint8_t *pAppKey)
{
  uint16_t appKeyIdx;

  WSF_ASSERT(pAppKey != NULL);

  appKeyIdx = meshLocalGetKeyEntryIndex(appKeyIndex, FALSE);
  /* Check if the appKeyIndex is in the list. */
  if (appKeyIdx != MESH_INVALID_ENTRY_INDEX)
  {
    /* Check if AppKey New flag is set and return error. */
    if (localCfgAppKeyList.pAppKeyList[appKeyIdx].newKeyAvailable == FALSE)
    {
      memcpy(localCfgAppKeyList.pAppKeyList[appKeyIdx].appKeyNew, pAppKey, MESH_KEY_SIZE_128);
      localCfgAppKeyList.pAppKeyList[appKeyIdx].newKeyAvailable = TRUE;

      /* Update AppKey list in NVM. */
      WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_APP_KEY_DATASET_ID, (uint8_t *)localCfgAppKeyList.pAppKeyList,
                                   sizeof(meshLocalCfgAppKeyListEntry_t) * localCfgAppKeyList.appKeyListSize, NULL);

      return MESH_SUCCESS;
    }
    else
    {
      return MESH_LOCAL_CFG_ALREADY_EXIST;
    }
  }
  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief     Removes the Application key for a key index.
 *
 *  \param[in] appKeyIndex       Application key index.
 *  \param[in] removeOldKeyOnly  If TRUE, AppKey Old is replaced by AppKey New
 *                               If FALSE, the AppKey is removed from the AppKey List.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgRemoveAppKey(uint16_t appKeyIndex, bool_t removeOldKeyOnly)
{
  uint16_t appKeyIdx;

  appKeyIdx = meshLocalGetKeyEntryIndex(appKeyIndex, FALSE);
  /* Check if the appKeyIndex is in the list. */
  if (appKeyIdx != MESH_INVALID_ENTRY_INDEX)
  {
    /* Check if only the AppKey old needs to be removed. */
    if (removeOldKeyOnly == TRUE)
    {
      if (localCfgAppKeyList.pAppKeyList[appKeyIdx].newKeyAvailable == TRUE)
      {
        /* Replace the old Key with the new Key. */
        memcpy(localCfgAppKeyList.pAppKeyList[appKeyIdx].appKeyOld,
               localCfgAppKeyList.pAppKeyList[appKeyIdx].appKeyNew,
               MESH_KEY_SIZE_128);
        localCfgAppKeyList.pAppKeyList[appKeyIdx].newKeyAvailable = FALSE;
      }
    }
    else
    {
      /* Remove the key entry from AppKey list. */
      localCfgAppKeyList.pAppKeyList[appKeyIdx].appKeyIndex = MESH_KEY_INVALID_INDEX;
      localCfgAppKeyList.pAppKeyList[appKeyIdx].netKeyEntryIndex = MESH_INVALID_ENTRY_INDEX;
      localCfgAppKeyList.pAppKeyList[appKeyIdx].newKeyAvailable = FALSE;
    }

    /* Update AppKey list in NVM. */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_APP_KEY_DATASET_ID, (uint8_t *)localCfgAppKeyList.pAppKeyList,
                                 sizeof(meshLocalCfgAppKeyListEntry_t) * localCfgAppKeyList.appKeyListSize, NULL);

    return MESH_SUCCESS;
  }
  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief         Gets next AppKey Index.
 *
 *  \param[out]    pOutAppKeyIndex  Pointer to variable where the next AppKey Index is stored.
 *  \param[in,out] pInOutIndex      Pointer to an indexing variable. Must be initialized with 0 on
 *                                  to restart search.
 *
 *  \return        Success or error reason. \see meshLocalCfgRetVal_t
 *
 *  \remarks       This function behaves as an iterator, fetching one key index at a time.
 *
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetNextAppKeyIndex(uint16_t *pOutAppKeyIndex,
                                                    uint16_t *pInOutIndex)
{
  WSF_ASSERT(pOutAppKeyIndex != NULL);
  WSF_ASSERT(pInOutIndex);

  /* Iterate through the list. */
  for (; *pInOutIndex < localCfgAppKeyList.appKeyListSize; (*pInOutIndex)++)
  {
    /* Check if entry is valid. */
    if (localCfgAppKeyList.pAppKeyList[*pInOutIndex].appKeyIndex != MESH_KEY_INVALID_INDEX)
    {
      /* Store AppKey Index. */
      *pOutAppKeyIndex = localCfgAppKeyList.pAppKeyList[*pInOutIndex].appKeyIndex;
      /* Increment for future search. */
      (*pInOutIndex)++;
      return MESH_SUCCESS;
    }
  }

  return MESH_LOCAL_CFG_NOT_FOUND;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets the Application key for a key index.
 *
 *  \param[in]  appKeyIndex  Application key index.
 *  \param[out] pOutAppKey   Pointer to store the Application key.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 *
 *  \remarks    Any of the output parameters can be omitted. Set NULL if not requested.
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetAppKey(uint16_t appKeyIndex, uint8_t *pOutAppKey)
{
  uint16_t appKeyIdx;

  WSF_ASSERT(pOutAppKey != NULL);

  appKeyIdx = meshLocalGetKeyEntryIndex(appKeyIndex, FALSE);
  /* Check if the appKeyIndex is in the list. */
  if (appKeyIdx != MESH_INVALID_ENTRY_INDEX)
  {
    memcpy(pOutAppKey, localCfgAppKeyList.pAppKeyList[appKeyIdx].appKeyOld,
           MESH_KEY_SIZE_128);
    return MESH_SUCCESS;
  }
  return MESH_LOCAL_CFG_NOT_FOUND;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets the updated Application key for a key index.
 *
 *  \param[in]  appKeyIndex  Application key index.
 *  \param[out] pOutAppKey   Pointer to store the updated Application key.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 *
 *  \remarks    Any of the output parameters can be omitted. Set NULL if not requested.
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetUpdatedAppKey(uint16_t appKeyIndex, uint8_t *pOutAppKey)
{
  uint16_t appKeyIdx;

  /* Check if at least one output pointer is not NULL. */
  WSF_ASSERT(pOutAppKey != NULL);

  appKeyIdx = meshLocalGetKeyEntryIndex(appKeyIndex, FALSE);
  /* Check if the appKeyIndex is in the list. */
  if ((appKeyIdx != MESH_INVALID_ENTRY_INDEX) &&
      (localCfgAppKeyList.pAppKeyList[appKeyIdx].newKeyAvailable == TRUE))
  {
    memcpy(pOutAppKey, localCfgAppKeyList.pAppKeyList[appKeyIdx].appKeyNew,
           MESH_KEY_SIZE_128);
    return MESH_SUCCESS;
  }
  return MESH_LOCAL_CFG_NOT_FOUND;
}

/*************************************************************************************************/
/*!
 *  \brief     Binds an Application Key index to a Model ID.
 *
 *  \param[in] elementId    Local element identifier.
 *  \param[in] pModelId     Pointer to generic model identifier structure.
 *  \param[in] appKeyIndex  Application key index.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgBindAppKeyToModel(meshElementId_t elementId,
                                                   const meshModelId_t *pModelId,
                                                   uint16_t appKeyIndex)
{
  uint16_t modelIdx;
  uint16_t appKeyListOffset;
  uint16_t appKeyIdx;
  uint16_t freeIdx = MESH_INVALID_ENTRY_INDEX;
  uint16_t keyBindIdx;

  WSF_ASSERT(pModelId != NULL);

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    /* Search for AppKeyIndex. */
    appKeyIdx = meshLocalGetKeyEntryIndex(appKeyIndex, FALSE);

    /* Get the Model AppKey Bind List offset. */
    appKeyListOffset = localCfgModel.pModelArray[modelIdx].appKeyBindListStartIdx;

    if (appKeyIdx != MESH_INVALID_ENTRY_INDEX)
    {
      /* AppKeyIndex found in list, bind to model. */
      for (keyBindIdx = appKeyListOffset;
           keyBindIdx < appKeyListOffset + localCfgModel.pModelArray[modelIdx].appKeyBindListSize;
           keyBindIdx++)
      {
        /* AppKeyIndex found.*/
        if (appKeyIdx == localCfgAppKeyBindList.pAppKeyBindList[keyBindIdx])
        {
          return MESH_LOCAL_CFG_ALREADY_EXIST;
        }
        /* Store the first free entry index, if found. */
        if ((freeIdx == MESH_INVALID_ENTRY_INDEX) &&
            (localCfgAppKeyBindList.pAppKeyBindList[keyBindIdx] == MESH_INVALID_ENTRY_INDEX))
        {
          freeIdx = keyBindIdx;
        }
      }
      /* If free entry index is available, store the appKeyIdx. */
      if (freeIdx != MESH_INVALID_ENTRY_INDEX)
      {
        localCfgAppKeyBindList.pAppKeyBindList[freeIdx] = appKeyIdx;

        /* Update AppKey Bind list in NVM. */
        WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_APP_KEY_BIND_DATASET_ID, (uint8_t *)localCfgAppKeyBindList.pAppKeyBindList,
                                     sizeof(uint16_t) * localCfgAppKeyBindList.appKeyBindListSize, NULL);

        return MESH_SUCCESS;
      }
      else
      {
        return MESH_LOCAL_CFG_OUT_OF_MEMORY;
      }
    }
  }
  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief     Unbinds an Application key index from a Model ID.
 *
 *  \param[in] elementId    Local element identifier.
 *  \param[in] pModelId     Pointer to generic model identifier structure.
 *  \param[in] appKeyIndex  Application key index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgUnbindAppKeyFromModel(meshElementId_t elementId, const meshModelId_t *pModelId,
                                       uint16_t appKeyIndex)
{
  uint16_t modelIdx;
  uint16_t appKeyListOffset;
  uint16_t appKeyIdx;
  uint16_t keyBindIdx;

  WSF_ASSERT(pModelId != NULL);

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
    /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    appKeyIdx = meshLocalGetKeyEntryIndex(appKeyIndex, FALSE);

    /* Get the Model AppKey Bind List offset. */
    appKeyListOffset = localCfgModel.pModelArray[modelIdx].appKeyBindListStartIdx;

    /* Check if AppKey entry index is valid. */
    if (appKeyIdx != MESH_INVALID_ENTRY_INDEX)
    {
      /* Search through Model's AppKey Bind list. */
      for (keyBindIdx = appKeyListOffset;
           keyBindIdx < appKeyListOffset + localCfgModel.pModelArray[modelIdx].appKeyBindListSize;
           keyBindIdx++)
      {
        /* If entry index is found, unbind. */
        if (localCfgAppKeyBindList.pAppKeyBindList[keyBindIdx] == appKeyIdx)
        {
          localCfgAppKeyBindList.pAppKeyBindList[keyBindIdx] = MESH_INVALID_ENTRY_INDEX;

          /* Update AppKey Bind list in NVM. */
          WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_APP_KEY_BIND_DATASET_ID, (uint8_t *)localCfgAppKeyBindList.pAppKeyBindList,
                                       sizeof(uint16_t) * localCfgAppKeyBindList.appKeyBindListSize, NULL);
        }
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Validates that an Application Key is bound to a Model instance.
 *
 *  \param[in] elementId    Local element identifier.
 *  \param[in] pModelId     Pointer to generic model identifier structure.
 *  \param[in] appKeyIndex  Application key index.
 *
 *  \return    TRUE if bind exists, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshLocalCfgValidateModelToAppKeyBind(meshElementId_t elementId, meshModelId_t *pModelId,
                                             uint16_t appKeyIndex)
{
  uint16_t modelIdx;
  uint16_t appKeyListOffset;
  uint16_t appKeyIdx;
  uint16_t keyIdx;

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
  /* Search for the AppKey Index in list. */
  appKeyIdx = meshLocalGetKeyEntryIndex(appKeyIndex, FALSE);

    /* Get the Model AppKey Bind List offset. */
  appKeyListOffset = localCfgModel.pModelArray[modelIdx].appKeyBindListStartIdx;

  /* Model found. */
  if ((modelIdx != MESH_INVALID_ENTRY_INDEX) && (appKeyIdx != MESH_INVALID_ENTRY_INDEX))
  {
    for (keyIdx = appKeyListOffset;
         keyIdx < appKeyListOffset + localCfgModel.pModelArray[modelIdx].appKeyBindListSize;
         keyIdx++)
    {
      if (localCfgAppKeyBindList.pAppKeyBindList[keyIdx] == appKeyIdx)
      {
        return TRUE;
      }
    }
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Counts number of Application Keys bound to a Model instance.
 *
 *  \param[in] elementId  Local element identifier.
 *  \param[in] pModelId   Pointer to generic model identifier structure.
 *
 *  \return    TRUE if bind exists, FALSE otherwise.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgCountModelBoundAppKeys(meshElementId_t elementId,
                                           const meshModelId_t *pModelId)
{
  uint16_t modelIdx;
  uint16_t appKeyListOffset;
  uint16_t keyIdx;
  uint8_t  count = 0;

  WSF_ASSERT(pModelId != NULL);

  /* Search for model in list. */
  modelIdx = meshLocalCfgSearchModel(elementId, pModelId);

  /* Model found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    /* Get the Model AppKey Bind List offset. */
    appKeyListOffset = localCfgModel.pModelArray[modelIdx].appKeyBindListStartIdx;

    for (keyIdx = appKeyListOffset;
         keyIdx < appKeyListOffset + localCfgModel.pModelArray[modelIdx].appKeyBindListSize;
         keyIdx++)
    {
      if (localCfgAppKeyBindList.pAppKeyBindList[keyIdx] != MESH_INVALID_ENTRY_INDEX)
      {
        count++;
      }
    }
  }

  return count;
}

/*************************************************************************************************/
/*!
 *  \brief         Gets next AppKey Index bound to a model instance.
 *
 *  \param[in]     elementId        Local element identifier.
 *  \param[in]     pModelId         Pointer to generic model identifier structure.
 *  \param[out]    pOutAppKeyIndex  Pointer to variable where the next AppKey Index is stored.
 *  \param[in,out] pInOutIndex      Pointer to an indexing variable. Must be intialized with 0 on
 *                                   to restart search.
 *
 *  \return        Success or error reason. \see meshLocalCfgRetVal_t
 *
 *  \remarks       This function behaves like an iterator. Fetching one key index at a time.
 *
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetNextModelBoundAppKey(meshElementId_t elementId,
                                                         const meshModelId_t *pModelId,
                                                         uint16_t *pOutAppKeyIndex,
                                                         uint8_t *pInOutIndex)
{
  static uint16_t modelIdx = MESH_INVALID_ENTRY_INDEX;
  uint16_t appKeyIdx;
  uint16_t appKeyListOffset;

  WSF_ASSERT(pOutAppKeyIndex != NULL);
  WSF_ASSERT(pInOutIndex != NULL);

  /* Restart search when indexer is 0. */
  if (*pInOutIndex == 0)
  {
    /* Search for model in list. */
    modelIdx = meshLocalCfgSearchModel(elementId, pModelId);
  }

  /* Check if model is found. */
  if (modelIdx != MESH_INVALID_ENTRY_INDEX)
  {
    /* Get the Model AppKey Bind List offset. */
    appKeyListOffset = localCfgModel.pModelArray[modelIdx].appKeyBindListStartIdx;

    /* Resume iteration. */
    for (; *pInOutIndex < localCfgModel.pModelArray[modelIdx].appKeyBindListSize; (*pInOutIndex)++)
    {
      appKeyIdx = localCfgAppKeyBindList.pAppKeyBindList[*pInOutIndex + appKeyListOffset];
      if (appKeyIdx != MESH_INVALID_ENTRY_INDEX)
      {
        /* Store AppKey Index. */
        *pOutAppKeyIndex = localCfgAppKeyList.pAppKeyList[appKeyIdx].appKeyIndex;
        /* Increment for future search. */
        (*pInOutIndex)++;

        return MESH_SUCCESS;
      }
    }
    return MESH_LOCAL_CFG_NOT_FOUND;
  }
  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief     Binds an Application key index to a Network key index.
 *
 *  \param[in] appKeyIndex  Application key index.
 *  \param[in] netKeyIndex  Network key index.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgBindAppKeyToNetKey(uint16_t appKeyIndex, uint16_t netKeyIndex)
{
  uint16_t netKeyIdx;
  uint16_t appKeyIdx;
  /* Search for NetKey Index. */
  netKeyIdx = meshLocalGetKeyEntryIndex(netKeyIndex, TRUE);
  /* Check if NetKey Index found. */
  if (netKeyIdx != MESH_INVALID_ENTRY_INDEX)
  {
    /* Search for AppKey Index. */
    appKeyIdx = meshLocalGetKeyEntryIndex(appKeyIndex, FALSE);
    /* Check if AppKey Index found. */
    if (appKeyIdx != MESH_INVALID_ENTRY_INDEX)
    {
      /* Bind AppKey to NetKey. */
      localCfgAppKeyList.pAppKeyList[appKeyIdx].netKeyEntryIndex = netKeyIdx;

      /* Update AppKey list in NVM. */
      WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_APP_KEY_DATASET_ID, (uint8_t *)localCfgAppKeyList.pAppKeyList,
                                   sizeof(meshLocalCfgAppKeyListEntry_t) * localCfgAppKeyList.appKeyListSize, NULL);

      return MESH_SUCCESS;
    }
  }
  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief     Unbinds an Application key index from a Network key index.
 *
 *  \param[in] appKeyIndex  Application key index.
 *  \param[in] netKeyIndex  Network key index.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgUnbindAppKeyToNetKey(uint16_t appKeyIndex, uint16_t netKeyIndex)
{
  uint16_t appKeyIdx;
  uint16_t netKeyIdx;
  /* Search through AppKey list. */
  for (appKeyIdx = 0; appKeyIdx < localCfgAppKeyList.appKeyListSize; appKeyIdx++)
  {
    netKeyIdx = localCfgAppKeyList.pAppKeyList[appKeyIdx].netKeyEntryIndex;
    if (netKeyIdx < localCfgNetKeyList.netKeyListSize)
    {
      /* Check if AppKey index and NetKeyIndex are binded. */
      if ((localCfgAppKeyList.pAppKeyList[appKeyIdx].appKeyIndex == appKeyIndex) &&
          (localCfgNetKeyList.pNetKeyList[netKeyIdx].netKeyIndex == netKeyIndex))
      {
        /* Unbind NetKey index from AppKey index. */
        localCfgAppKeyList.pAppKeyList[appKeyIdx].netKeyEntryIndex = MESH_INVALID_ENTRY_INDEX;

        /* Update AppKey list in NVM. */
        WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_APP_KEY_DATASET_ID, (uint8_t *)localCfgAppKeyList.pAppKeyList,
                                     sizeof(meshLocalCfgAppKeyListEntry_t) * localCfgAppKeyList.appKeyListSize, NULL);

        return MESH_SUCCESS;
      }
    }
  }
  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief     Validates if a Network-Application Key binding exists.
 *
 *  \param[in] netKeyIndex  Network Key index.
 *  \param[in] appKeyIndex  Application Key index.
 *
 *  \return    TRUE if bind exists. FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshLocalCfgValidateNetToAppKeyBind(uint16_t netKeyIndex, uint16_t appKeyIndex)
{
  uint16_t appKeyIdx;
  uint16_t netKeyIdx;

  /* Search for AppKey Index. */
  appKeyIdx = meshLocalGetKeyEntryIndex(appKeyIndex, FALSE);
  /* Check if AppKey Index found. */
  if (appKeyIdx != MESH_INVALID_ENTRY_INDEX)
  {
    /* Search for NetKey Index. */
    netKeyIdx = meshLocalGetKeyEntryIndex(netKeyIndex, TRUE);
    /* Check if NetKey Index found. */
    if (netKeyIdx != MESH_INVALID_ENTRY_INDEX)
    {
      /* Check if bind exist. */
      if (localCfgAppKeyList.pAppKeyList[appKeyIdx].netKeyEntryIndex == netKeyIdx)
      {
        return TRUE;
      }
    }
  }
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets NetKey Index of Network Key bound to Application Key identified by appKeyIndex.
 *
 *  \param[in]  appKeyIndex      Application Key index.
 *  \param[out] pOutNetKeyIndex  Pointer to memory where the NetKey Index is stored if successful.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetBoundNetKeyIndex(uint16_t appKeyIndex,
                                                     uint16_t *pOutNetKeyIndex)
{
  uint16_t appKeyIdx;
  uint16_t netKeyIdx;

  WSF_ASSERT(pOutNetKeyIndex != NULL);

  /* Search for AppKey Index. */
  appKeyIdx = meshLocalGetKeyEntryIndex(appKeyIndex, FALSE);
  /* Check if AppKey Index found. */
  if (appKeyIdx != MESH_INVALID_ENTRY_INDEX)
  {
    /*  Get NetKey entry idx */
    netKeyIdx = localCfgAppKeyList.pAppKeyList[appKeyIdx].netKeyEntryIndex;

    if (netKeyIdx != MESH_INVALID_ENTRY_INDEX)
    {
      *pOutNetKeyIndex = localCfgNetKeyList.pNetKeyList[netKeyIdx].netKeyIndex;
      return MESH_SUCCESS;
    }
  }
  return MESH_LOCAL_CFG_NOT_FOUND;
}

/*************************************************************************************************/
/*!
 *  \brief     Counts number of AppKeys bound to a NetKey.
 *
 *  \param[in] netKeyIndex  Network Key index.
 *
 *  \return    Number of bound AppKeys or 0 on error.
 */
/*************************************************************************************************/
uint16_t MeshLocalCfgCountBoundAppKeys(uint16_t netKeyIndex)
{
  uint16_t appKeyIdx;
  uint16_t netKeyIdx;
  uint16_t count = 0;

  /* Search for NetKey Index. */
  netKeyIdx = meshLocalGetKeyEntryIndex(netKeyIndex, TRUE);
  /* Check if NetKey Index found. */
  if (netKeyIdx != MESH_INVALID_ENTRY_INDEX)
  {
    for (appKeyIdx = 0; appKeyIdx < localCfgAppKeyList.appKeyListSize; appKeyIdx++)
    {
      if (localCfgAppKeyList.pAppKeyList[appKeyIdx].netKeyEntryIndex == netKeyIdx)
      {
        count++;
      }
    }
  }
  return count;
}

/*************************************************************************************************/
/*!
 *  \brief         Gets next AppKey Index bound to a NetKey Index.
 *
 *  \param[in]     netKeyIndex      Network Key index.
 *  \param[out]    pOutAppKeyIndex  Pointer to variable where the next AppKey Index is stored.
 *  \param[in,out] pInOutIndex      Pointer to an indexing variable. Must be intialized with 0 on
 *                                  to restart search.
 *
 *  \return        Success or error reason. \see meshLocalCfgRetVal_t
 *
 *  \remarks       This function behaves like an iterator. Fetching one key index at a time.
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetNextBoundAppKey(uint16_t netKeyIndex, uint16_t *pOutAppKeyIndex,
                                                    uint16_t *pInOutIndex)
{
  static uint16_t netKeyIdx = MESH_INVALID_ENTRY_INDEX;

  WSF_ASSERT(pOutAppKeyIndex != NULL);
  WSF_ASSERT(pInOutIndex != NULL);

  /* Restart search when indexer is 0. */
  if (*pInOutIndex == 0)
  {
    /* Search for NetKey Index. */
    netKeyIdx = meshLocalGetKeyEntryIndex(netKeyIndex, TRUE);
  }
  /* Check if NetKey Index found. */
  if (netKeyIdx != MESH_INVALID_ENTRY_INDEX)
  {
    /* Resume iteration. */
    for (; *pInOutIndex < localCfgAppKeyList.appKeyListSize; (*pInOutIndex)++)
    {
      if (localCfgAppKeyList.pAppKeyList[*pInOutIndex].netKeyEntryIndex == netKeyIdx)
      {
        /* Store AppKey Index. */
        *pOutAppKeyIndex = localCfgAppKeyList.pAppKeyList[*pInOutIndex].appKeyIndex;
        /* Increment for future search. */
        (*pInOutIndex)++;

        return MESH_SUCCESS;
      }
    }
    return MESH_LOCAL_CFG_NOT_FOUND;
  }
  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets a read only copy of the Virtual Address Table.
 *
 *  \param[out] ppVtadList  Pointer to memory where the address of the virtual address information
 *                          structure is stored.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void MeshLocalCfgGetVtadList(const meshLocalCfgVirtualAddrListInfo_t **ppVtadList)
{
  WSF_ASSERT(ppVtadList != NULL);

  *ppVtadList = &localCfgVirtualAddrList;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Default TTL value.
 *
 *  \param[in] defaultTtl  Default TTL value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetDefaultTtl(uint8_t defaultTtl)
{
  localCfg.defaultTtl = defaultTtl;

  /* Update Local Cfg structure in NVM. */
  WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_DATASET_ID, (uint8_t *)&localCfg, sizeof(localCfg), NULL);
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the Default TTL value.
 *
 *  \return Default TTL value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetDefaultTtl(void)
{
  return localCfg.defaultTtl;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Relay state value.
 *
 *  \param[in] relayState  Relay state value. See ::meshRelayStates_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetRelayState(meshRelayStates_t relayState)
{
  if (relayState < MESH_RELAY_FEATURE_PROHIBITED_START)
  {
    localCfg.relayState = relayState;

    /* Update Local Cfg structure in NVM. */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_DATASET_ID, (uint8_t *)&localCfg, sizeof(localCfg), NULL);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the Relay state value.
 *
 *  \return Relay state value. See ::meshRelayStates_t.
 */
/*************************************************************************************************/
meshRelayStates_t MeshLocalCfgGetRelayState(void)
{
  return localCfg.relayState;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Attention Timer value.
 *
 *  \param[in] elementId  Local element identifier.
 *  \param[in] timerVal   Attention Timer value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetAttentionTimer(meshElementId_t elementId, uint8_t timerVal)
{
  if (elementId < localCfgElement.elementArrayLen)
  {
    localCfgElement.pAttTmrArray[elementId].remainingSec = timerVal;

    if (timerVal == 0)
    {
      /* Attention timer is being stopped for an element */
      meshLocalCfgStopAttention(elementId);
    }
    else
    {
      /* Attention timer is being started for an element */
      meshLocalCfgStartAttention(elementId);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the Attention Timer value.
 *
 *  \param[in] elementId  Local element identifier.
 *
 *  \return    Attention Timer value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetAttentionTimer(meshElementId_t elementId)
{
  if (elementId < localCfgElement.elementArrayLen)
  {
    return localCfgElement.pAttTmrArray[elementId].remainingSec;
  }
  return 0;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Secure Network Beacon state value.
 *
 *  \param[in] beaconState  Secure Network Beacon state value. See ::meshBeaconStates_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetBeaconState(meshBeaconStates_t beaconState)
{
  if (beaconState < MESH_BEACON_PROHIBITED_START)
  {
    localCfg.beaconState = beaconState;

    /* Update Local Cfg structure in NVM. */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_DATASET_ID, (uint8_t *)&localCfg, sizeof(localCfg), NULL);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the Secure Network Beacon state value.
 *
 *  \return Secure Network Beacon state value. See ::meshBeaconStates_t.
 */
/*************************************************************************************************/
meshBeaconStates_t MeshLocalCfgGetBeaconState(void)
{
  return localCfg.beaconState;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the GATT Proxy state value.
 *
 *  \param[in] gattProxyState  GATT Proxy state value. See ::meshGattProxyStates_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetGattProxyState(meshGattProxyStates_t gattProxyState)
{
  if (gattProxyState < MESH_GATT_PROXY_FEATURE_PROHIBITED_START)
  {
    localCfg.gattProxyState = gattProxyState;

    /* Update Local Cfg structure in NVM. */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_DATASET_ID, (uint8_t *)&localCfg, sizeof(localCfg), NULL);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the GATT Proxy state value.
 *
 *  \return GATT Proxy state value. See ::meshGattProxyStates_t.
 */
/*************************************************************************************************/
meshGattProxyStates_t MeshLocalCfgGetGattProxyState(void)
{
  return localCfg.gattProxyState;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Node Identity state value.
 *
 *  \param[in] netKeyIndex        NetKey Index to identify the subnet for the Node Identity.
 *  \param[in] nodeIdentityState  Node Identity state value. See ::meshNodeIdentityStates_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetNodeIdentityState(uint16_t netKeyIndex, meshNodeIdentityStates_t nodeIdentityState)
{
  uint16_t netKeyIdx;

  WSF_ASSERT(nodeIdentityState < MESH_NODE_IDENTITY_PROHIBITED_START);

  /* Search for NetKey Index. */
  netKeyIdx = meshLocalGetKeyEntryIndex(netKeyIndex, TRUE);
  /* Check if NetKey Index found. */
  if (netKeyIdx != MESH_INVALID_ENTRY_INDEX)
  {
    localCfgNetKeyList.pNodeIdentityList[netKeyIdx] = nodeIdentityState;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the Node Identity state value.
 *
 *  \param[in] netKeyIndex  NetKey Index to identify the subnet for the Node Identity.
 *
 *  \return    Node Identity state value for the given NetKey Index or
 *             ::MESH_NODE_IDENTITY_PROHIBITED_START if NetKey Index is invalid.
 */
/*************************************************************************************************/
meshNodeIdentityStates_t MeshLocalCfgGetNodeIdentityState(uint16_t netKeyIndex)
{
  uint16_t netKeyIdx;

  /* Search for NetKey Index. */
  netKeyIdx = meshLocalGetKeyEntryIndex(netKeyIndex, TRUE);
  /* Check if NetKey Index found. */
  if (netKeyIdx != MESH_INVALID_ENTRY_INDEX)
  {
    return localCfgNetKeyList.pNodeIdentityList[netKeyIdx];
  }

  return MESH_NODE_IDENTITY_PROHIBITED_START;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Friend state value.
 *
 *  \param[in] friendState  Friend state value. See ::meshFriendStates_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetFriendState(meshFriendStates_t friendState)
{
  if (friendState < MESH_FRIEND_FEATURE_PROHIBITED_START)
  {
    localCfg.friendState = friendState;

    /* Update Local Cfg structure in NVM. */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_DATASET_ID, (uint8_t *)&localCfg, sizeof(localCfg), NULL);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the Friend state value.
 *
 *  \return Friend state value. See ::meshFriendStates_t.
 */
/*************************************************************************************************/
meshFriendStates_t MeshLocalCfgGetFriendState(void)
{
  return localCfg.friendState;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Low Power state value.
 *
 *  \param[in] lowPowerState  Low Power state value. See ::meshLowPowerStates_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetLowPowerState(meshLowPowerStates_t lowPowerState)
{
  if (lowPowerState < MESH_LOW_POWER_FEATURE_PROHIBITED_START)
  {
    localCfg.lowPowerState = lowPowerState;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the Low Power state value.
 *
 *  \return Low Power state value. See ::meshLowPowerStates_t.
 */
/*************************************************************************************************/
meshLowPowerStates_t MeshLocalCfgGetLowPowerState(void)
{
  return localCfg.lowPowerState;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Key Refresh Phase state value.
 *
 *  \param[in] netKeyIndex      NetKey Index to identify the NetKey in the list.
 *  \param[in] keyRefreshState  Key Refresh Phase state value. See ::meshKeyRefreshStates_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetKeyRefreshState(uint16_t netKeyIndex, meshKeyRefreshStates_t keyRefreshState)
{
  uint16_t netKeyIdx;

  WSF_ASSERT(keyRefreshState < MESH_KEY_REFRESH_PROHIBITED_START);

  /* Search for NetKey Index. */
  netKeyIdx = meshLocalGetKeyEntryIndex(netKeyIndex, TRUE);
  /* Check if NetKey Index found. */
  if (netKeyIdx != MESH_INVALID_ENTRY_INDEX)
  {
    localCfgNetKeyList.pNetKeyList[netKeyIdx].keyRefreshState = keyRefreshState;

    /* Update NetKey list in NVM. */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_NET_KEY_DATASET_ID, (uint8_t *)localCfgNetKeyList.pNetKeyList,
                                 sizeof(meshLocalCfgNetKeyListEntry_t) * localCfgNetKeyList.netKeyListSize, NULL);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the Key Refresh Phase state value.
 *
 *  \param[in] netKeyIndex  NetKey Index to identify the NetKey in the list.
 *
 *  \return    Key Refresh Phase state value for the given NetKey Index.
 */
/*************************************************************************************************/
meshKeyRefreshStates_t MeshLocalCfgGetKeyRefreshPhaseState(uint16_t netKeyIndex)
{
  uint16_t netKeyIdx;

  /* Search for NetKey Index. */
  netKeyIdx = meshLocalGetKeyEntryIndex(netKeyIndex, TRUE);
  /* Check if NetKey Index found. */
  if (netKeyIdx != MESH_INVALID_ENTRY_INDEX)
  {
    return localCfgNetKeyList.pNetKeyList[netKeyIdx].keyRefreshState;
  }
  /* Return prohibited value if key not found. */
  return MESH_KEY_REFRESH_PROHIBITED_START;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Publication destination address.
 *
 *  \param[in] dstAddress  Destination address for Heartbeat messages.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetHbPubDst(meshAddress_t dstAddress)
{
  uint16_t addrIdx;
  uint16_t newAddrIdx;

  WSF_ASSERT((MESH_IS_ADDR_UNASSIGNED(dstAddress) || MESH_IS_ADDR_UNICAST(dstAddress) ||
              MESH_IS_ADDR_GROUP(dstAddress)));

  addrIdx = localCfgHb.pubDstAddressIndex;

  /* Check if address is UNASSIGNED. */
  if (MESH_IS_ADDR_UNASSIGNED(dstAddress))
  {
    /* Check if Address entry index is valid. */
    if (addrIdx != MESH_INVALID_ENTRY_INDEX)
    {
      meshLocalCfgRemoveAddress(addrIdx, FALSE, TRUE);
      localCfgHb.pubDstAddressIndex = MESH_INVALID_ENTRY_INDEX;

      /* Update Heartbeat structure in NVM. */
      WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_HB_DATASET_ID, (uint8_t *)&localCfgHb, sizeof(localCfgHb), NULL);

      return MESH_SUCCESS;
    }
  }
  else
  {
    /* Search for address. */
    newAddrIdx = meshLocalCfgGetAddressEntryIndex(dstAddress, NULL);
    /* Check if the new address and the old address are the same. */
    if ((newAddrIdx == addrIdx) && (newAddrIdx != MESH_INVALID_ENTRY_INDEX))
    {
      return MESH_LOCAL_CFG_ALREADY_EXIST;
    }
    /* Check index and add the new address if not found as it may fail with out of memory. */
    if (newAddrIdx == MESH_INVALID_ENTRY_INDEX)
    {
      /* Set Address in Address List. */
      newAddrIdx = meshLocalCfgSetAddress(dstAddress, NULL, TRUE);
    }
    /* Check again if Address List is full. */
    if (newAddrIdx == MESH_INVALID_ENTRY_INDEX)
    {
      return MESH_LOCAL_CFG_OUT_OF_MEMORY;
    }
    /* Another publish address is set, first remove that one. */
    if (addrIdx != MESH_INVALID_ENTRY_INDEX)
    {
      meshLocalCfgRemoveAddress(addrIdx, FALSE, TRUE);
    }
    localCfgHb.pubDstAddressIndex = newAddrIdx;

    /* Update Heartbeat structure in NVM. */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_HB_DATASET_ID, (uint8_t *)&localCfgHb, sizeof(localCfgHb), NULL);

    return MESH_SUCCESS;
  }
  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the Heartbeat Publication destination address.
 *
 *  \return Heartbeat Publication destination address.
 */
/*************************************************************************************************/
meshAddress_t MeshLocalCfgGetHbPubDst(void)
{
  uint16_t addrIdx;

  addrIdx = localCfgHb.pubDstAddressIndex;
  /* Check if Address entry index is valid. */
  if (addrIdx != MESH_INVALID_ENTRY_INDEX)
  {
    return localCfgAddressList.pAddressList[addrIdx].address;
  }
  return MESH_ADDR_TYPE_UNASSIGNED;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Publication Count Log state which controls the number of
 *             periodical Heartbeat transport control messages to be sent.
 *
 *  \param[in] countLog  Heartbeat Publication Count Log value to be set.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetHbPubCountLog(uint8_t countLog)
{
  localCfgHb.pubCountLog = countLog;

  /* Update Heartbeat structure in NVM. */
  WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_HB_DATASET_ID, (uint8_t *)&localCfgHb, sizeof(localCfgHb), NULL);
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the Heartbeat Publication Count Log value.
 *
 *  \return Heartbeat Publication Count Log value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetHbPubCountLog(void)
{
  return localCfgHb.pubCountLog;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Publication Period Log state which controls the cadence of
 *             periodical Heartbeat transport control messages to be sent.
 *
 *  \param[in] periodLog  Heartbeat Publication Period Log value to be set.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetHbPubPeriodLog(uint8_t periodLog)
{
  localCfgHb.pubPeriodLog = periodLog;

  /* Update Heartbeat structure in NVM. */
  WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_HB_DATASET_ID, (uint8_t *)&localCfgHb, sizeof(localCfgHb), NULL);
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the Heartbeat Publication Period Log value.
 *
 *  \return Heartbeat Publication Period Log value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetHbPubPeriodLog(void)
{
  return localCfgHb.pubPeriodLog;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Publication TTL used when sending Heartbeat messages.
 *
 *  \param[in] pubTtl  Heartbeat Publication TTL value to be set.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetHbPubTtl(uint8_t pubTtl)
{
  localCfgHb.pubTtl = pubTtl;

  /* Update Heartbeat structure in NVM. */
  WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_HB_DATASET_ID, (uint8_t *)&localCfgHb, sizeof(localCfgHb), NULL);
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the Heartbeat Publication TTL value.
 *
 *  \return Heartbeat Publication TTL value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetHbPubTtl(void)
{
  return localCfgHb.pubTtl;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Publication Features that trigger sending Heartbeat messages.
 *
 *  \param[in] pubFeatures  Heartbeat Publication Features to be set.
 *
 *  \return    None.
*/
/*************************************************************************************************/
void MeshLocalCfgSetHbPubFeatures(meshFeatures_t pubFeatures)
{
  if (pubFeatures < MESH_FEAT_RFU_START)
  {
    localCfgHb.pubFeatures = pubFeatures;

    /* Update Heartbeat structure in NVM. */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_HB_DATASET_ID, (uint8_t *)&localCfgHb, sizeof(localCfgHb), NULL);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the Heartbeat Publication Features set.
 *
 *  \return Heartbeat Publication Features.
 */
/*************************************************************************************************/
meshFeatures_t MeshLocalCfgGetHbPubFeatures(void)
{
  return localCfgHb.pubFeatures;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Publication NetKey Index used when sending Heartbeat messages.
 *
 *  \param[in] netKeyIndex  Heartbeat Publication NetKey Index value to be set.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetHbPubNetKeyIndex(uint16_t netKeyIndex)
{
  uint16_t netKeyIdx;

  /* Search for NetKey Index. */
  netKeyIdx = meshLocalGetKeyEntryIndex(netKeyIndex, TRUE);
  /* Check if NetKey Index found. */
  if (netKeyIdx != MESH_INVALID_ENTRY_INDEX)
  {
    localCfgHb.pubNetKeyEntryIndex = netKeyIdx;

    /* Update Heartbeat structure in NVM. */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_HB_DATASET_ID, (uint8_t *)&localCfgHb, sizeof(localCfgHb), NULL);

    return MESH_SUCCESS;
  }
  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets the Heartbeat Publication NetKey Index value.
 *
 *  \param[out] pOutNetKeyIndex  Pointer to store the Heartbeat Publication NetKey Index value.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetHbPubNetKeyIndex(uint16_t *pOutNetKeyIndex)
{
  uint16_t netKeyIdx;

  WSF_ASSERT(pOutNetKeyIndex != NULL);

  netKeyIdx = localCfgHb.pubNetKeyEntryIndex;
  /* Check if NetKey entry index is valid. */
  if (netKeyIdx < localCfgNetKeyList.netKeyListSize)
  {
    *pOutNetKeyIndex = localCfgNetKeyList.pNetKeyList[netKeyIdx].netKeyIndex;
    return MESH_SUCCESS;
  }
  return MESH_LOCAL_CFG_NOT_FOUND;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Subscription source address.
 *
 *  \param[in] srcAddress  Source address for Heartbeat messages.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetHbSubSrc(meshAddress_t srcAddress)
{
  uint16_t addrIdx;
  uint16_t newAddrIdx;

  WSF_ASSERT(MESH_IS_ADDR_UNASSIGNED(srcAddress) || MESH_IS_ADDR_UNICAST(srcAddress));

  addrIdx = localCfgHb.subSrcAddressIndex;

  /* Check if address is UNASSIGNED. */
  if (MESH_IS_ADDR_UNASSIGNED(srcAddress))
  {
    /* Check if Address entry index is valid. */
    if (addrIdx != MESH_INVALID_ENTRY_INDEX)
    {
      meshLocalCfgRemoveAddress(addrIdx, FALSE, FALSE);
      localCfgHb.subSrcAddressIndex = MESH_INVALID_ENTRY_INDEX;

      /* Update Heartbeat structure in NVM. */
      WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_HB_DATASET_ID, (uint8_t *)&localCfgHb, sizeof(localCfgHb), NULL);

      return MESH_SUCCESS;
    }
  }
  else
  {
    /* Search for address. */
    newAddrIdx = meshLocalCfgGetAddressEntryIndex(srcAddress, NULL);
    /* Check if the new address and the old address are the same. */
    if ((newAddrIdx == addrIdx) && (newAddrIdx != MESH_INVALID_ENTRY_INDEX))
    {
      return MESH_LOCAL_CFG_ALREADY_EXIST;
    }
    /* Check index and add the new address if not found as it may fail with out of memory. */
    if (newAddrIdx == MESH_INVALID_ENTRY_INDEX)
    {
      /* Set Address in Address List. */
      newAddrIdx = meshLocalCfgSetAddress(srcAddress, NULL, FALSE);
    }

    /* Check again if Address List is full. */
    if (newAddrIdx == MESH_INVALID_ENTRY_INDEX)
    {
      return MESH_LOCAL_CFG_OUT_OF_MEMORY;
    }

    /* Another publish address is set, first remove that one. */
    if (addrIdx != MESH_INVALID_ENTRY_INDEX)
    {
      meshLocalCfgRemoveAddress(addrIdx, FALSE, FALSE);
    }
    localCfgHb.subSrcAddressIndex = newAddrIdx;

    /* Update Heartbeat structure in NVM. */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_HB_DATASET_ID, (uint8_t *)&localCfgHb, sizeof(localCfgHb), NULL);

    return MESH_SUCCESS;
  }
  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the Heartbeat Subscription source address.
 *
 *  \return Heartbeat Subscription source address.
 */
/*************************************************************************************************/
meshAddress_t MeshLocalCfgGetHbSubSrc(void)
{
  uint16_t addrIdx;

  addrIdx = localCfgHb.subSrcAddressIndex;
  /* Check if Address entry index is valid. */
  if (addrIdx != MESH_INVALID_ENTRY_INDEX)
  {
    return localCfgAddressList.pAddressList[addrIdx].address;
  }
  return MESH_ADDR_TYPE_UNASSIGNED;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Subscription destination address.
 *
 *  \param[in] dstAddress  Destination address for Heartbeat messages.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetHbSubDst(meshAddress_t dstAddress)
{
  uint16_t addrIdx;
  uint16_t newAddrIdx;

  WSF_ASSERT(MESH_IS_ADDR_UNASSIGNED(dstAddress) || MESH_IS_ADDR_UNICAST(dstAddress) ||
             MESH_IS_ADDR_GROUP(dstAddress));

  addrIdx = localCfgHb.subDstAddressIndex;

  /* Check if address is UNASSIGNED. */
  if (MESH_IS_ADDR_UNASSIGNED(dstAddress))
  {
    /* Check if Address entry index is valid. */
    if (addrIdx != MESH_INVALID_ENTRY_INDEX)
    {
      meshLocalCfgRemoveAddress(addrIdx, FALSE, FALSE);
      localCfgHb.subDstAddressIndex = MESH_INVALID_ENTRY_INDEX;

      /* Update Heartbeat structure in NVM. */
      WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_HB_DATASET_ID, (uint8_t *)&localCfgHb, sizeof(localCfgHb), NULL);

      return MESH_SUCCESS;
    }
  }
  else
  {
    /* Search for address. */
    newAddrIdx = meshLocalCfgGetAddressEntryIndex(dstAddress, NULL);
    /* Check if the new address and the old address are the same. */
    if ((newAddrIdx == addrIdx) && (newAddrIdx != MESH_INVALID_ENTRY_INDEX))
    {
      return MESH_LOCAL_CFG_ALREADY_EXIST;
    }
    /* Check index and add the new address if not found as it may fail with out of memory. */
    if (newAddrIdx == MESH_INVALID_ENTRY_INDEX)
    {
      /* Set Address in Address List. */
      newAddrIdx = meshLocalCfgSetAddress(dstAddress, NULL, FALSE);
    }
    /* Check again if Address List is full. */
    if (newAddrIdx == MESH_INVALID_ENTRY_INDEX)
    {
      return MESH_LOCAL_CFG_OUT_OF_MEMORY;
    }
    /* Another publish address is set, first remove that one. */
    if (addrIdx != MESH_INVALID_ENTRY_INDEX)
    {
      meshLocalCfgRemoveAddress(addrIdx, FALSE, FALSE);
    }
    localCfgHb.subDstAddressIndex = newAddrIdx;

    /* Update Heartbeat structure in NVM. */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_HB_DATASET_ID, (uint8_t *)&localCfgHb, sizeof(localCfgHb), NULL);

    return MESH_SUCCESS;
  }
  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the Heartbeat Subscription destination address.
 *
 *  \return Heartbeat Subscription destination address.
 */
/*************************************************************************************************/
meshAddress_t MeshLocalCfgGetHbSubDst(void)
{
  uint16_t addrIdx;

  addrIdx = localCfgHb.subDstAddressIndex;
  /* Check if address entry index is valid. */
  if (addrIdx != MESH_INVALID_ENTRY_INDEX)
  {
    return localCfgAddressList.pAddressList[addrIdx].address;
  }
  return MESH_ADDR_TYPE_UNASSIGNED;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Subscription Count Log value.
 *
 *  \param[in] countLog  Heartbeat Subscription Count Log value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetHbSubCountLog(uint8_t countLog)
{
  localCfgHb.subCountLog = countLog;

  /* Update Heartbeat structure in NVM. */
  WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_HB_DATASET_ID, (uint8_t *)&localCfgHb, sizeof(localCfgHb), NULL);
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the Heartbeat Subscription Count Log value.
 *
 *  \return Heartbeat Subscription Count Log value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetHbSubCountLog(void)
{
  return localCfgHb.subCountLog;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Subscription Period Log state which controls the period for
 *             processing periodical Heartbeat transport control messages.
 *
 *  \param[in] periodLog  Heartbeat Subscription Period Log value to be set.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetHbSubPeriodLog(uint8_t periodLog)
{
  localCfgHb.subPeriodLog = periodLog;

  /* Update Heartbeat structure in NVM. */
  WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_HB_DATASET_ID, (uint8_t *)&localCfgHb, sizeof(localCfgHb), NULL);
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the Heartbeat Subscription Period Log value.
 *
 *  \return Heartbeat Subscription Period Log value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetHbSubPeriodLog(void)
{
  return localCfgHb.subPeriodLog;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Subscription Min Hops value which determines the minimum hops
 *             registered when receiving Heartbeat messages since receiving the most recent.
 *
 *  \param[in] minHops  Heartbeat Subscription Min Hops value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetHbSubMinHops(uint8_t minHops)
{
  localCfgHb.subMinHops = minHops;

  /* Update Heartbeat structure in NVM. */
  WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_HB_DATASET_ID, (uint8_t *)&localCfgHb, sizeof(localCfgHb), NULL);
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the Heartbeat Subscription Min Hops value.
 *
 *  \return Heartbeat Subscription Min Hops value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetHbSubMinHops(void)
{
  return localCfgHb.subMinHops;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Heartbeat Subscription Max Hops value which determines the maximum hops
 *             registered when receiving Heartbeat messages since receiving the most recent
 *             Config Heartbeat Subscription Set message.
 *
 *  \param[in] maxHops  Heartbeat Subscription Max Hops value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetHbSubMaxHops(uint8_t maxHops)
{
  localCfgHb.subMaxHops = maxHops;

  /* Update Heartbeat structure in NVM. */
  WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_HB_DATASET_ID, (uint8_t *)&localCfgHb, sizeof(localCfgHb), NULL);
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the Heartbeat Subscription Max Hops value which determines the maximum hops
 *          registered when receiving Heartbeat messages since receiving the most recent
 *          Config Heartbeat Subscription Set message.
 *
 *  \return Heartbeat Subscription Max Hops value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetHbSubMaxHops(void)
{
  return localCfgHb.subMaxHops;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Network Transmit Count value which controls the number of message
 *             transmissions of the Network PDU originating from the node.
 *
 *  \param[in] transCount  Network Transmit Count value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetNwkTransmitCount(uint8_t transCount)
{
  localCfg.nwkTransCount = transCount;

  /* Update Heartbeat structure in NVM. */
  WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_DATASET_ID, (uint8_t *)&localCfg, sizeof(localCfg), NULL);
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the Network Transmit Count value.
 *
 *  \return Network Transmit Count value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetNwkTransmitCount(void)
{
  return localCfg.nwkTransCount;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Network Transmit Interval Steps value representing the number of 10ms
 *             steps that controls the interval between transmissions of Network PDUs originating
 *             from the node.
 *
 *  \param[in] intvlSteps  Network Transmit Interval Steps value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetNwkTransmitIntvlSteps(uint8_t intvlSteps)
{
  localCfg.nwkIntvlSteps = intvlSteps;

  /* Update Heartbeat structure in NVM. */
  WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_DATASET_ID, (uint8_t *)&localCfg, sizeof(localCfg), NULL);
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the Network Transmit Interval Steps value.
 *
 *  \return Network Transmit Interval Steps value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetNwkTransmitIntvlSteps(void)
{
  return localCfg.nwkIntvlSteps;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Relay Retransmit Count value which controls the number of retransmissions
 *             of the Network PDU relayed by the node.
 *
 *  \param[in] retransCount  Relay Retransmit Count value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetRelayRetransmitCount(uint8_t retransCount)
{
  localCfg.relayRetransCount = retransCount;

  /* Update Heartbeat structure in NVM. */
  WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_DATASET_ID, (uint8_t *)&localCfg, sizeof(localCfg), NULL);
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the Relay Retransmit Count value.
 *
 *  \return Relay Retransmit Count value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetRelayRetransmitCount(void)
{
  return localCfg.relayRetransCount;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the Relay Retransmit Interval Steps value representing the number of 10ms
 *             steps that controls the interval between retransmissions of Network PDUs relayed
 *             by the node.
 *
 *  \param[in] intvlSteps  Relay Retransmit Interval Steps value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetRelayRetransmitIntvlSteps(uint8_t intvlSteps)
{
  localCfg.relayRetransIntvlSteps = intvlSteps;

  /* Update Heartbeat structure in NVM. */
  WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_DATASET_ID, (uint8_t *)&localCfg, sizeof(localCfg), NULL);
}

/*************************************************************************************************/
/*!
 *  \brief  Gets the Relay Retransmit Interval Steps value.
 *
 *  \return Relay Retransmit Interval Steps value.
 */
/*************************************************************************************************/
uint8_t MeshLocalCfgGetRelayRetransmitIntvlSteps(void)
{
  return localCfg.relayRetransIntvlSteps;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the SEQ number value.
 *
 *  \param[in] elementId  Local element identifier.
 *  \param[in] seqNumber  SEQ number value.
 *
 *  \return    Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgSetSeqNumber(meshElementId_t elementId, meshSeqNumber_t seqNumber)
{
  if ((elementId < localCfgElement.elementArrayLen) && (seqNumber <= MESH_SEQ_MAX_VAL))
  {
    /* Set SEQ number threshold value. */
    MeshLocalCfgSetSeqNumberThresh(elementId, seqNumber);

    /* Set the current value. */
    localCfgElement.pSeqNumberArray[elementId] = seqNumber;

    return MESH_SUCCESS;
  }
  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets the SEQ number value.
 *
 *  \param[in]  elementId      Local element identifier.
 *  \param[out] pOutSeqNumber  Pointer to store the SEQ number value.
 *
 *  \return     Success or error reason. \see meshLocalCfgRetVal_t
 */
/*************************************************************************************************/
meshLocalCfgRetVal_t MeshLocalCfgGetSeqNumber(meshElementId_t elementId, meshSeqNumber_t *pOutSeqNumber)
{
  WSF_ASSERT(pOutSeqNumber != NULL);

  if (elementId < localCfgElement.elementArrayLen)
  {
    *pOutSeqNumber = localCfgElement.pSeqNumberArray[elementId];
    return MESH_SUCCESS;
  }
  return MESH_LOCAL_CFG_INVALID_PARAMS;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the SEQ number threshold value.
 *
 *  \param[in] elementId  Local element identifier.
 *  \param[in] seqNumber  SEQ number threshold value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetSeqNumberThresh(meshElementId_t elementId, meshSeqNumber_t seqNumber)
{
  WSF_ASSERT((elementId < localCfgElement.elementArrayLen) && (seqNumber <= MESH_SEQ_MAX_VAL));

  if ((seqNumber % MESH_SEQ_NUMBER_NVM_INC) == 0)
  {
    localCfgElement.pSeqNumberThreshArray[elementId] =
      ((seqNumber / MESH_SEQ_NUMBER_NVM_INC) + 1) * MESH_SEQ_NUMBER_NVM_INC;

    /* Save the next SEQ number threshold value to NVM. */
    WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_SEQ_NUMBER_THRESH_DATASET_ID, (uint8_t *)localCfgElement.pSeqNumberThreshArray,
                                 sizeof(meshSeqNumber_t) * localCfgElement.elementArrayLen, NULL);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the SEQ number Threshold value.
 *
 *  \param[in] elementId Local element identifier.
 *
 *  \return    SEQ number threshold value.
 */
/*************************************************************************************************/
meshSeqNumber_t MeshLocalCfgGetSeqNumberThresh(meshElementId_t elementId)
{
  if (elementId < localCfgElement.elementArrayLen)
  {
    return localCfgElement.pSeqNumberThreshArray[elementId];
  }
  return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      Gets the IV Index value and IV Index update in progress flag.
 *
 *  \param[out] pIvUpdtInProg  Pointer to store TRUE if IV update is in progress or FALSE otherwise.
 *
 *  \return     IV index value.
 */
/*************************************************************************************************/
uint32_t MeshLocalCfgGetIvIndex(bool_t *pIvUpdtInProg)
{
  if (pIvUpdtInProg != NULL)
  {
    *pIvUpdtInProg = localCfg.ivUpdtInProg;
  }
  return localCfg.ivIndex;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the IV index value.
 *
 *  \param[in] ivIndex  IV index value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetIvIndex(uint32_t ivIndex)
{
  meshIvUpdtEvt_t evt;

  localCfg.ivIndex = ivIndex;

  /* Update Local Cfg structure in NVM. */
  WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_DATASET_ID, (uint8_t *)&localCfg, sizeof(localCfg), NULL);

  /* Signal event to the application. */
  evt.hdr.event = MESH_CORE_EVENT;
  evt.hdr.param = MESH_CORE_IV_UPDATED_EVENT;
  evt.hdr.status = MESH_SUCCESS;
  evt.ivIndex = ivIndex;

  meshCb.evtCback((meshEvt_t*)&evt);
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the IV Index update in progress flag.
 *
 *  \param[in] ivUpdtInProg  IV Index update in progress flag value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLocalCfgSetIvUpdateInProgress(bool_t ivUpdtInProg)
{
  localCfg.ivUpdtInProg = (ivUpdtInProg != FALSE) ? TRUE : FALSE;

  /* Update Local Cfg structure in NVM. */
  WsfNvmWriteData((uint64_t)MESH_LOCAL_CFG_NVM_DATASET_ID, (uint8_t *)&localCfg, sizeof(localCfg), NULL);
}

/*************************************************************************************************/
/*!
 *  \brief  Erase configuration.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshLocalCfgEraseNvm(void)
{
  WsfNvmEraseData((uint64_t)MESH_LOCAL_CFG_NVM_DATASET_ID, NULL);
  WsfNvmEraseData((uint64_t)MESH_LOCAL_CFG_NVM_NET_KEY_DATASET_ID, NULL);
  WsfNvmEraseData((uint64_t)MESH_LOCAL_CFG_NVM_APP_KEY_DATASET_ID, NULL);
  WsfNvmEraseData((uint64_t)MESH_LOCAL_CFG_NVM_APP_KEY_BIND_DATASET_ID, NULL);
  WsfNvmEraseData((uint64_t)MESH_LOCAL_CFG_NVM_ADDRESS_DATASET_ID, NULL);
  WsfNvmEraseData((uint64_t)MESH_LOCAL_CFG_NVM_VIRTUAL_ADDR_DATASET_ID, NULL);
  WsfNvmEraseData((uint64_t)MESH_LOCAL_CFG_NVM_SUBSCR_DATASET_ID, NULL);
  WsfNvmEraseData((uint64_t)MESH_LOCAL_CFG_NVM_SEQ_NUMBER_DATASET_ID, NULL);
  WsfNvmEraseData((uint64_t)MESH_LOCAL_CFG_NVM_SEQ_NUMBER_THRESH_DATASET_ID, NULL);
  WsfNvmEraseData((uint64_t)MESH_LOCAL_CFG_NVM_MODEL_DATASET_ID, NULL);
  WsfNvmEraseData((uint64_t)MESH_LOCAL_CFG_NVM_HB_DATASET_ID, NULL);
}

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
/*************************************************************************************************/
/*!
 *  \brief     Alters the NetKey list size in Local Config for Mesh Test.
 *
 *  \param[in] listSize  NetKey list size.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshTestLocalCfgAlterNetKeyListSize(uint16_t listSize)
{
  localCfgNetKeyList.netKeyListSize = listSize;
}
#endif
