/*************************************************************************************************/
/*!
 *  \file   mesh_cfg_mdl_sr_app_net_key.c
 *
 *  \brief  AppKey-NetKey messages implementation.
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

#include <string.h>
#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_buf.h"
#include "wsf_msg.h"
#include "wsf_os.h"
#include "wsf_cs.h"
#include "wsf_trace.h"

#include "mesh_defs.h"
#include "mesh_error_codes.h"
#include "mesh_api.h"
#include "mesh_handler.h"

#include "mesh_main.h"
#include "mesh_security.h"
#include "mesh_local_config_types.h"
#include "mesh_local_config.h"
#include "mesh_network_mgmt.h"

#include "mesh_lower_transport.h"
#include "mesh_upper_transport.h"
#include "mesh_upper_transport_heartbeat.h"
#include "mesh_access.h"

#include "mesh_utils.h"

#include "mesh_cfg_mdl_api.h"
#include "mesh_cfg_mdl_sr_api.h"
#include "mesh_cfg_mdl.h"
#include "mesh_cfg_mdl_sr.h"
#include "mesh_cfg_mdl_sr_main.h"

#include "mesh_cfg_mdl_messages.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Parameters required to respond to Config AppKey/NetKey Add/Update. */
typedef struct keyAddUpdtParams_tag
{
  uint16_t      netKeyIndex;      /*!< NetKeyIndex.*/
  uint16_t      appKeyIndex;      /*!< AppKeyIndex or invalid for Config NetKey messages. */
  meshAddress_t cfgMdlClAddr;     /*!< Address of the client sending the request. */
  uint16_t      recvNetKeyIndex;  /*!< NetKeyIndex of the network on which the request is received.
                                   */
  uint8_t       ttl;              /*!< TTL of the message containing the request. */
  bool_t        isUpdate;         /*!< TRUE if operation is update, FALSE if operation is add. */

} keyAddUpdtParams_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Temporary buffer. */
static uint8_t tempKey[MESH_KEY_SIZE_128];

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Manages clean-up in case an AppKey is deleted.
 *
 *  \param[in] appKeyIndex  Global key index for AppKey.
 *
 *  \return    None.
 *
 *  \remarks   This function searches for all models that publish using the AppKey and disables
 *             publication.
 */
/*************************************************************************************************/
static void appKeyDelCleanup(uint16_t appKeyIndex)
{
  uint8_t *pLabelUuid;
  meshAddress_t pubAddr = MESH_ADDR_TYPE_UNASSIGNED;
  uint16_t pubAppKeyIndex;
  meshElementId_t elemId;
  uint8_t sigIdx,vendorIdx,totalIdx;
  meshModelId_t mdlId;
  meshLocalCfgRetVal_t retVal;

  /* Iterate through all elements. */
  for (elemId = 0; elemId < pMeshConfig->elementArrayLen; elemId++)
  {
    totalIdx = 0;
    /* Parse all models at once. */
    while(totalIdx < (pMeshConfig->pElementArray[elemId].numSigModels +
                      pMeshConfig->pElementArray[elemId].numVendorModels))
    {
      /* Create model id structure used by local config. */
      if (totalIdx < pMeshConfig->pElementArray[elemId].numSigModels)
      {
        sigIdx = totalIdx;
        mdlId.isSigModel = TRUE;
        mdlId.modelId.sigModelId = pMeshConfig->pElementArray[elemId].pSigModelArray[sigIdx].modelId;
      }
      else
      {
        vendorIdx = totalIdx - pMeshConfig->pElementArray[elemId].numSigModels;
        mdlId.isSigModel = FALSE;
        mdlId.modelId.vendorModelId =
          pMeshConfig->pElementArray[elemId].pVendorModelArray[vendorIdx].modelId;
      }
      totalIdx++;

      /* Check if the model instance is bound to the AppKey. */
      if (MeshLocalCfgValidateModelToAppKeyBind(elemId, &mdlId, appKeyIndex))
      {
        /* Unbind. */
        MeshLocalCfgUnbindAppKeyFromModel(elemId, (const meshModelId_t *)&mdlId, appKeyIndex);
      }

      /* Read publish address. */
      retVal = MeshLocalCfgGetPublishAddress(elemId, (const meshModelId_t *)&mdlId, &pubAddr,
                                             &pLabelUuid);

      /* Skip models that have publication disabled. */
      if ((retVal != MESH_SUCCESS) || MESH_IS_ADDR_UNASSIGNED(pubAddr))
      {
        continue;
      }

      /* Read publish AppKeyIndex. Allowed if publication is enabled. */
      retVal = MeshLocalCfgGetPublishAppKeyIndex(elemId, (const meshModelId_t *)&mdlId,
                                                 &pubAppKeyIndex);

      /* Skip models that don't use this AppKeyIndex. */
      if ((retVal != MESH_SUCCESS) || (pubAppKeyIndex != appKeyIndex))
      {
        continue;
      }

      /* Disable publishing. */
      MeshLocalCfgSetPublishAddress(elemId, (const meshModelId_t *)&mdlId,
                                    MESH_ADDR_TYPE_UNASSIGNED);

      /* Set Model Publication parameters to 0. */
      MeshLocalCfgMdlClearPublishAppKeyIndex(elemId, (const meshModelId_t *)&mdlId);

      (void) MeshLocalCfgSetPublishFriendshipCredFlag(elemId, (const meshModelId_t *)&mdlId, FALSE);
      (void) MeshLocalCfgSetPublishPeriod(elemId, (const meshModelId_t *)&mdlId, 0, 0);
      (void) MeshLocalCfgSetPublishRetransCount(elemId, (const meshModelId_t *)&mdlId, 0);
      (void) MeshLocalCfgSetPublishRetransIntvlSteps(elemId, (const meshModelId_t *)&mdlId, 0);
      (void) MeshLocalCfgSetPublishTtl(elemId, (const meshModelId_t *)&mdlId, 0);

      /* Notify Access Layer that periodic publishing state has changed.*/
      MeshAccPeriodPubChanged(elemId, &mdlId);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Manages clean-up in case a NetKey is deleted.
 *
 *  \param[in] netKeyIndex  Global key index for NetKey.
 *
 *  \return    None.
 *
 *  \remarks   This function removes all bound AppKeys and disables Heartbeat if the NetKey is used.
 */
/*************************************************************************************************/
static void netKeyDelCleanup(uint16_t netKeyIndex)
{
  uint16_t appKeyIndex;
  uint16_t hbNetKeyIndex;
  uint16_t indexer = 0;

  /* Iterate through all the bound AppKeys. */
  while(MeshLocalCfgGetNextBoundAppKey(netKeyIndex, &appKeyIndex, &indexer)
        == MESH_SUCCESS)
  {
    /* Clean-up AppKey dependencies. */
    appKeyDelCleanup(appKeyIndex);

    /* Remove key material. */
    (void)MeshSecRemoveKeyMaterial(MESH_SEC_KEY_TYPE_APP, appKeyIndex, FALSE);

    /* Remove key. */
    (void)MeshLocalCfgRemoveAppKey(appKeyIndex, FALSE);
  }

  /* Start verifying Heartbeat Publication. */
  if((MeshLocalCfgGetHbPubNetKeyIndex(&hbNetKeyIndex) == MESH_SUCCESS) &&
      (hbNetKeyIndex == netKeyIndex))
  {
    /* Disable Heartbeat. */
    MeshLocalCfgSetHbPubDst(MESH_ADDR_TYPE_UNASSIGNED);
    MeshLocalCfgSetHbPubCountLog(0);
    MeshLocalCfgSetHbPubPeriodLog(0);
    MeshLocalCfgSetHbPubTtl(0);

    /* Notify Module. */
    MeshHbPublicationStateChanged();
  }

  /* Notify friendship about key removal. */
  meshCfgMdlSrCb.netKeyDelNotifyCback(netKeyIndex);
}

/*************************************************************************************************/
/*!
 *  \brief     NetKey material derivation complete callback.
 *
 *  \param[in] keyType     The type of the key.
 *  \param[in] keyIndex    Global key index for application and network key types.
 *  \param[in] keyUpdated  TRUE if key material was added for the new key of an existing key index.
 *  \param[in] isSuccess   TRUE if operation is successful.
 *  \param[in] pParam      Pointer to generic callback parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void netKeyMatCback(meshSecKeyType_t keyType, uint16_t keyIndex,
                           bool_t isSuccess, bool_t keyUpdated, void *pParam)
{
  keyAddUpdtParams_t *pRspParams = (keyAddUpdtParams_t *)pParam;
  meshCfgMdlNetKeyChgEvt_t evt =
  {
    .cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS
  };
  uint8_t rspMsgParam[CFG_MDL_MSG_NETKEY_STATUS_NUM_BYTES];

  WSF_ASSERT(keyUpdated == pRspParams->isUpdate);

  /* Check if key material is added or updated. */
  if (isSuccess)
  {
    WSF_ASSERT(keyType == MESH_SEC_KEY_TYPE_NWK);
    WSF_ASSERT(keyIndex == pRspParams->netKeyIndex);

    rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;
    /* If a key is updated, transition to first phase. */
    if (pRspParams->isUpdate)
    {
      WSF_ASSERT(MeshLocalCfgGetKeyRefreshPhaseState(pRspParams->netKeyIndex)
                 == MESH_KEY_REFRESH_NOT_ACTIVE);

      /* Call Network Management. */
      MeshNwkMgmtHandleKeyRefreshTrans(pRspParams->netKeyIndex, MESH_KEY_REFRESH_NOT_ACTIVE,
                                       MESH_KEY_REFRESH_FIRST_PHASE);
    }

    /* Configure user event. */
    evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT;
    evt.cfgMdlHdr.hdr.param = pRspParams->isUpdate ? MESH_CFG_MDL_NETKEY_UPDT_EVENT :
                                           MESH_CFG_MDL_NETKEY_ADD_EVENT;
    evt.cfgMdlHdr.peerAddress = pRspParams->cfgMdlClAddr;
    evt.netKeyIndex = pRspParams->netKeyIndex;
  }
  else
  {
    rspMsgParam[0] = pRspParams->isUpdate ? MESH_CFG_MDL_ERR_CANNOT_UPDATE :
                                            MESH_CFG_MDL_ERR_UNSPECIFIED;
    /* Do clean-up. */
    if (pRspParams->isUpdate)
    {
      /* Read old key. */
      (void) MeshLocalCfgGetNetKey(pRspParams->netKeyIndex, tempKey);
      /* Replace old key with new. */
      (void) MeshLocalCfgRemoveNetKey(pRspParams->netKeyIndex, TRUE);
      /* Set the old key back as updated key. */
      (void) MeshLocalCfgUpdateNetKey(pRspParams->netKeyIndex, (const uint8_t *)tempKey);
      /* Replace new with old. */
      (void) MeshLocalCfgRemoveNetKey(pRspParams->netKeyIndex, TRUE);
    }
    else
    {
      /* Remove key. */
      (void) MeshLocalCfgRemoveNetKey(pRspParams->netKeyIndex, FALSE);
    }
  }
  /* Pack NetKey Index. */
  (void) meshCfgMsgPackSingleKeyIndex(&rspMsgParam[1], pRspParams->netKeyIndex);

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_NETKEY_STATUS, rspMsgParam, sizeof(rspMsgParam),
                   pRspParams->cfgMdlClAddr, pRspParams->ttl, pRspParams->recvNetKeyIndex);

  /* Free memory. */
  WsfBufFree(pRspParams);

  if (rspMsgParam[0] == MESH_CFG_MDL_SR_SUCCESS)
  {
    /* Trigger user callback. */
    meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)&evt);
  }

  (void) keyType;
  (void) keyIndex;
  (void) keyUpdated;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Config NetKey Add request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleNetKeyAdd(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                 uint8_t ttl, uint16_t netKeyIndex)
{
  uint8_t *pKey;
  keyAddUpdtParams_t *pRspParams;
  uint16_t msgNetKeyIndex;
  meshSecRetVal_t retVal;
  uint8_t rspMsgParam[CFG_MDL_MSG_NETKEY_STATUS_NUM_BYTES];

  /* Validate length. */
  if ((msgParamLen != CFG_MDL_MSG_NETKEY_ADD_NUM_BYTES) || (pMsgParam == NULL))
  {
    return;
  }

  /* Unpack NetKeyIndex. */
  pMsgParam += meshCfgMsgUnpackSingleKeyIndex(pMsgParam, &msgNetKeyIndex);

  /* Point to NetKey. */
  pKey = pMsgParam;

  /* Try to read key identified by NetKeyIndex. */
  if (MeshLocalCfgGetNetKey(msgNetKeyIndex, tempKey) == MESH_SUCCESS)
  {
    /* Key exists, verify if the same. */
    if (memcmp(pKey, tempKey, MESH_KEY_SIZE_128) == 0)
    {
      rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;
    }
    else
    {
      rspMsgParam[0] = MESH_CFG_MDL_ERR_KEY_INDEX_EXISTS;
    }
  }
  else
  {
    /* Try to store it in local config. */
    if (MeshLocalCfgSetNetKey(msgNetKeyIndex, (const uint8_t *)pKey) != MESH_SUCCESS)
    {
      rspMsgParam[0] = MESH_CFG_MDL_ERR_INSUFFICIENT_RESOURCES;
    }
    else
    {
      /* Allocate memory to store response parameters at the end of the async key derivation. */
      if ((pRspParams = (keyAddUpdtParams_t *)WsfBufAlloc(sizeof(keyAddUpdtParams_t))) == NULL)
      {
        /* No resources to complete request. */
        return;
      }
      else
      {
        /* Configure parameters. */
        pRspParams->netKeyIndex = msgNetKeyIndex;
        pRspParams->appKeyIndex = 0xFFFF;
        pRspParams->cfgMdlClAddr = src;
        pRspParams->ttl = ttl;
        pRspParams->recvNetKeyIndex = netKeyIndex;
        pRspParams->isUpdate = FALSE;

        /* Call security to derive the key material. */
        retVal = MeshSecAddKeyMaterial(MESH_SEC_KEY_TYPE_NWK, msgNetKeyIndex, FALSE,
                                       netKeyMatCback,(void *)pRspParams);
        if (retVal != MESH_SUCCESS)
        {
          MESH_TRACE_WARN1("CFG SR: NetKey add derivation failed with code %x", retVal);

          /* Set error code. */
          rspMsgParam[0] = MESH_CFG_MDL_ERR_UNSPECIFIED;
          /* Free memory. */
          WsfBufFree(pRspParams);
          /* Remove key. */
          MeshLocalCfgRemoveNetKey(msgNetKeyIndex, FALSE);
        }
        else
        {
          /* Resume after derivation finishes. */
          return;
        }
      }
    }
  }
  /* Pack NetKeyIndex. */
  meshCfgMsgPackSingleKeyIndex(&rspMsgParam[1], msgNetKeyIndex);

  /* Send response for error cases or key already existing. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_NETKEY_STATUS, rspMsgParam, sizeof(rspMsgParam), src, ttl,
                      netKeyIndex);
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Config NetKey Update request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleNetKeyUpdt(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                  uint8_t ttl, uint16_t netKeyIndex)
{
  uint8_t *pKey;
  keyAddUpdtParams_t *pRspParams;
  uint16_t msgNetKeyIndex;
  meshSecRetVal_t retVal;
  uint8_t rspMsgParam[CFG_MDL_MSG_NETKEY_STATUS_NUM_BYTES];
  meshKeyRefreshStates_t keyRefreshState;

  /* Validate length. */
  if ((msgParamLen != CFG_MDL_MSG_NETKEY_UPDT_NUM_BYTES) || (pMsgParam == NULL))
  {
    return;
  }

  /* Unpack NetKeyIndex. */
  pMsgParam += meshCfgMsgUnpackSingleKeyIndex(pMsgParam, &msgNetKeyIndex);

  /* Point to NetKey. */
  pKey = pMsgParam;

  keyRefreshState = MeshLocalCfgGetKeyRefreshPhaseState(msgNetKeyIndex);

  /* Verify NetKeyIndex by reading key refresh state. */
  if (keyRefreshState >= MESH_KEY_REFRESH_PROHIBITED_START)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_NETKEY_INDEX;
  }
  /* Check if phase allows this message. */
  else if (keyRefreshState > MESH_KEY_REFRESH_FIRST_PHASE)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_CANNOT_UPDATE;
  }
  else
  {
    /* Try to read updated key identified by NetKeyIndex. */
    if (MeshLocalCfgGetUpdatedNetKey(msgNetKeyIndex, tempKey) == MESH_SUCCESS)
    {
      /* Key exists, verify if the same. */
      if (memcmp(pKey, tempKey, MESH_KEY_SIZE_128) == 0)
      {
        rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;
      }
      else
      {
        rspMsgParam[0] = MESH_CFG_MDL_ERR_CANNOT_UPDATE;
      }
    }
    else
    {
      retVal = (meshSecRetVal_t)MeshLocalCfgUpdateNetKey(msgNetKeyIndex, (const uint8_t *)pKey);
      /* Try to store it in local config. */
      if (retVal != MESH_SUCCESS)
      {
        MESH_TRACE_WARN1("CFG SR: NetKey update store failed with code %x", retVal);
        /* Should never happen. */
        rspMsgParam[0] = MESH_CFG_MDL_ERR_CANNOT_UPDATE;
      }
      else
      {
        /* Allocate memory to store response parameters at the end of the async key derivation. */
        if ((pRspParams = (keyAddUpdtParams_t *)WsfBufAlloc(sizeof(keyAddUpdtParams_t))) == NULL)
        {
          /* No resources to complete request. */
          return;
        }
        else
        {
          /* Configure parameters. */
          pRspParams->netKeyIndex = msgNetKeyIndex;
          pRspParams->appKeyIndex = 0xFFFF;
          pRspParams->cfgMdlClAddr = src;
          pRspParams->ttl = ttl;
          pRspParams->recvNetKeyIndex = netKeyIndex;
          pRspParams->isUpdate = TRUE;

          /* Call security to derive the key material. */
          retVal = MeshSecAddKeyMaterial(MESH_SEC_KEY_TYPE_NWK, msgNetKeyIndex, TRUE,
                                         netKeyMatCback,(void *)pRspParams);
          if (retVal != MESH_SUCCESS)
          {
            MESH_TRACE_WARN1("CFG SR: NetKey update derivation failed with code %x", retVal);

            /* Set error code. */
            rspMsgParam[0] = MESH_CFG_MDL_ERR_CANNOT_UPDATE;
            /* Free memory. */
            WsfBufFree(pRspParams);

            /* Read old key. */
            (void) MeshLocalCfgGetNetKey(msgNetKeyIndex, tempKey);
            /* Replace old with key. */
            MeshLocalCfgRemoveNetKey(msgNetKeyIndex, TRUE);
            /* Set the old key back as updated key. */
            MeshLocalCfgUpdateNetKey(msgNetKeyIndex, (const uint8_t *)tempKey);
            /* Replace new key with old. */
            MeshLocalCfgRemoveNetKey(msgNetKeyIndex, TRUE);
          }
          else
          {
            /* Resume after derivation finishes. */
            return;
          }
        }
      }
    }
  }

  /* Pack key index. */
  meshCfgMsgPackSingleKeyIndex(&rspMsgParam[1], msgNetKeyIndex);

  /* Send response for error cases or key already existing. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_NETKEY_STATUS, rspMsgParam, sizeof(rspMsgParam), src, ttl,
                      netKeyIndex);
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Config NetKey Delete request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleNetKeyDel(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                 uint8_t ttl, uint16_t netKeyIndex)
{
  meshCfgMdlNetKeyChgEvt_t evt =
  {
    .cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT,
    .cfgMdlHdr.hdr.param = MESH_CFG_MDL_NETKEY_DEL_EVENT,
    .cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS
  };
  uint8_t rspMsgParam[CFG_MDL_MSG_NETKEY_STATUS_NUM_BYTES];
  bool_t triggerCback = FALSE;

  /* Validate length. */
  if ((msgParamLen != CFG_MDL_MSG_NETKEY_DEL_NUM_BYTES) || (pMsgParam == NULL))
  {
    return;
  }

  /* Unpack NetKeyIndex. */
  (void)meshCfgMsgUnpackSingleKeyIndex(pMsgParam, &evt.netKeyIndex);

  /* Verify NetKeyIndex by reading key refresh state. */
  if (MeshLocalCfgGetKeyRefreshPhaseState(evt.netKeyIndex) >= MESH_KEY_REFRESH_PROHIBITED_START)
  {
    /* A non-existing key delete means success. */
    rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;
  }
  /* Verify that the NetKey to be deleted is not the one used to process this request. */
  else if (evt.netKeyIndex == netKeyIndex)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_CANNOT_REMOVE;
  }
  else
  {
    rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;

    /* Clean-up NetKey dependencies. */
    netKeyDelCleanup(evt.netKeyIndex);

    /* Remove key material. */
    MeshSecRemoveKeyMaterial(MESH_SEC_KEY_TYPE_NWK, evt.netKeyIndex, FALSE);

    /* Remove key. */
    MeshLocalCfgRemoveNetKey(evt.netKeyIndex, FALSE);

    /* Key really removed. Upper layer can be informed. */
    triggerCback = TRUE;
  }

  /* Pack NetKeyIndex. */
  meshCfgMsgPackSingleKeyIndex(&rspMsgParam[1], evt.netKeyIndex);

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_NETKEY_STATUS, rspMsgParam, sizeof(rspMsgParam), src, ttl,
                      netKeyIndex);

  if (triggerCback)
  {
    /* Set client address. */
    evt.cfgMdlHdr.peerAddress = src;
    /* Trigger user callback. */
    meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)&evt);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Config NetKey Get request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleNetKeyGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                 uint8_t ttl, uint16_t netKeyIndex)
{
  uint8_t *pRspMsgParam, *pTemp;
  uint16_t rspLen;
  uint16_t numKeyIndexes = 0;
  uint16_t keyIndex1, keyIndex2;
  uint16_t indexer;

  (void)pMsgParam;

  /* Validate length. */
  if (msgParamLen != CFG_MDL_MSG_NETKEY_GET_NUM_BYTES)
  {
    return;
  }
  /* Get number of NetKeys. */
  numKeyIndexes = MeshLocalCfgCountNetKeys();

  WSF_ASSERT(numKeyIndexes != 0);

  rspLen = CFG_MDL_MSG_NETKEY_LIST_NUM_BYTES(numKeyIndexes);

  /* Allocate memory for the response. */
  if ((pRspMsgParam = (uint8_t *)WsfBufAlloc(rspLen)) != NULL)
  {
    indexer = 0;
    pTemp = pRspMsgParam;
    /* Attempt to read two key indexes at a time to follow encoding rules. */
    while (numKeyIndexes > 1)
    {
      (void) MeshLocalCfgGetNextNetKeyIndex(&keyIndex1, &indexer);
      (void) MeshLocalCfgGetNextNetKeyIndex(&keyIndex2, &indexer);

      /* Note: at least 2 keys exist must exist so keyIndex 1 aand cannot be uninitialized. */
      /* coverity[uninit_use_in_call] */
      pTemp += meshCfgMsgPackTwoKeyIndex(pTemp, keyIndex1, keyIndex2);
      numKeyIndexes -= 2;
    }
    /* If there is an odd number of NetKey Indexes, pack the last one. */
    if (numKeyIndexes != 0)
    {
      (void) MeshLocalCfgGetNextNetKeyIndex(&keyIndex1, &indexer);

      /* Note: at least 1 key must exist there for keyIndex1 cannot be returned unitiliazed. */
      /* coverity[uninit_use_in_call] */
      meshCfgMsgPackSingleKeyIndex(pTemp, keyIndex1);
    }
    /* Send response. */
    meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_NETKEY_LIST, pRspMsgParam, rspLen, src, ttl, netKeyIndex);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     AppKey material derivation complete callback.
 *
 *  \param[in] keyType     The type of the key.
 *  \param[in] keyIndex    Global key index for application and network key types.
 *  \param[in] keyUpdated  TRUE if key material was added for the new key of an existing key index.
 *  \param[in] isSuccess   TRUE if operation is successful.
 *  \param[in] pParam      Pointer to generic callback parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void appKeyMatCback(meshSecKeyType_t keyType, uint16_t keyIndex,
                           bool_t isSuccess, bool_t keyUpdated, void *pParam)
{
  keyAddUpdtParams_t *pRspParams = (keyAddUpdtParams_t *)pParam;
  meshCfgMdlAppKeyChgEvt_t evt =
  {
    .cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS
  };
  uint8_t rspMsgParam[CFG_MDL_MSG_APPKEY_STATUS_NUM_BYTES];

  WSF_ASSERT(keyUpdated == pRspParams->isUpdate);

  /* Check if key material is added or updated. */
  if (isSuccess)
  {
    WSF_ASSERT(keyType == MESH_SEC_KEY_TYPE_APP);
    WSF_ASSERT(keyIndex == pRspParams->appKeyIndex);

    rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;
    /* If a new key is added, bind it to the NetKey. */
    if (!pRspParams->isUpdate)
    {
      MeshLocalCfgBindAppKeyToNetKey(pRspParams->appKeyIndex, pRspParams->netKeyIndex);
    }

    /* Configure user event. */
    evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT;
    evt.cfgMdlHdr.hdr.param = pRspParams->isUpdate ? MESH_CFG_MDL_APPKEY_UPDT_EVENT :
                                           MESH_CFG_MDL_APPKEY_ADD_EVENT;
    evt.cfgMdlHdr.peerAddress = pRspParams->cfgMdlClAddr;
    evt.bind.appKeyIndex = pRspParams->appKeyIndex;
    evt.bind.netKeyIndex = pRspParams->netKeyIndex;
  }
  else
  {
    rspMsgParam[0] = pRspParams->isUpdate ? MESH_CFG_MDL_ERR_CANNOT_UPDATE :
                                            MESH_CFG_MDL_ERR_UNSPECIFIED;
    /* Do clean-up. */
    if (pRspParams->isUpdate)
    {
      /* Read old key. */
      (void) MeshLocalCfgGetAppKey(pRspParams->appKeyIndex, tempKey);
      /* Replace old key with new. */
      MeshLocalCfgRemoveAppKey(pRspParams->appKeyIndex, TRUE);
      /* Set the old key back as updated key. */
      MeshLocalCfgUpdateAppKey(pRspParams->appKeyIndex, (const uint8_t *)tempKey);
      /* Replace new key with old. */
      MeshLocalCfgRemoveAppKey(pRspParams->appKeyIndex, TRUE);
    }
    else
    {
      /* Remove key. */
      MeshLocalCfgRemoveAppKey(pRspParams->appKeyIndex, FALSE);
    }
  }
  /* Pack key bind. */
  meshCfgMsgPackTwoKeyIndex(&rspMsgParam[1], pRspParams->netKeyIndex, pRspParams->appKeyIndex);

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_APPKEY_STATUS, rspMsgParam, sizeof(rspMsgParam),
                      pRspParams->cfgMdlClAddr, pRspParams->ttl, pRspParams->recvNetKeyIndex);

  /* Free memory. */
  WsfBufFree(pRspParams);

  if (rspMsgParam[0] == MESH_CFG_MDL_SR_SUCCESS)
  {
    /* Trigger user callback. */
    meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)&evt);
  }
  (void)keyType;
  (void)keyIndex;
  (void)keyUpdated;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Config AppKey Add request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleAppKeyAdd(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                 uint8_t ttl, uint16_t netKeyIndex)
{
  uint8_t *pKey;
  keyAddUpdtParams_t *pRspParams;
  meshAppNetKeyBind_t bind;
  meshSecRetVal_t retVal;
  uint8_t rspMsgParam[CFG_MDL_MSG_APPKEY_STATUS_NUM_BYTES];

  /* Validate length. */
  if ((msgParamLen != CFG_MDL_MSG_APPKEY_ADD_NUM_BYTES) || (pMsgParam == NULL))
  {
    return;
  }

  /* Unpack NetKeyIndex and AppKeyIndex. */
  pMsgParam += meshCfgMsgUnpackTwoKeyIndex(pMsgParam, &bind.netKeyIndex, &bind.appKeyIndex);

  /* Point to AppKey. */
  pKey = pMsgParam;

  /* Verify NetKeyIndex by reading key refresh state. */
  if (MeshLocalCfgGetKeyRefreshPhaseState(bind.netKeyIndex) >= MESH_KEY_REFRESH_PROHIBITED_START)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_NETKEY_INDEX;
  }
  else
  {
    /* Try to read key identified by AppKeyIndex. */
    if (MeshLocalCfgGetAppKey(bind.appKeyIndex, tempKey) == MESH_SUCCESS)
    {
      /* Validate that a bind exists. */
      if (!MeshLocalCfgValidateNetToAppKeyBind(bind.netKeyIndex, bind.appKeyIndex))
      {
        rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_NETKEY_INDEX;
      }
      /* Key exists, verify if the same. */
      else if (memcmp(pKey, tempKey, MESH_KEY_SIZE_128) == 0)
      {
        rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;
      }
      else
      {
        rspMsgParam[0] = MESH_CFG_MDL_ERR_KEY_INDEX_EXISTS;
      }
    }
    else
    {
      /* Try to store it in local config. */
      if (MeshLocalCfgSetAppKey(bind.appKeyIndex, (const uint8_t *)pKey) != MESH_SUCCESS)
      {
        rspMsgParam[0] = MESH_CFG_MDL_ERR_INSUFFICIENT_RESOURCES;
      }
      else
      {
        /* Allocate memory to store response parameters at the end of the async key derivation. */
        if ((pRspParams = (keyAddUpdtParams_t *)WsfBufAlloc(sizeof(keyAddUpdtParams_t))) == NULL)
        {
          /* No resources to complete request. */
          return;
        }
        else
        {
          /* Configure parameters. */
          pRspParams->netKeyIndex = bind.netKeyIndex;
          pRspParams->appKeyIndex = bind.appKeyIndex;
          pRspParams->cfgMdlClAddr = src;
          pRspParams->ttl = ttl;
          pRspParams->recvNetKeyIndex = netKeyIndex;
          pRspParams->isUpdate = FALSE;

          /* Call security to derive the key material. */
          retVal = MeshSecAddKeyMaterial(MESH_SEC_KEY_TYPE_APP, bind.appKeyIndex, FALSE,
                                         appKeyMatCback,(void *)pRspParams);
          if (retVal != MESH_SUCCESS)
          {
            MESH_TRACE_WARN1("CFG SR: AppKey add derivation failed with code %x", retVal);

            /* Set error code. */
            rspMsgParam[0] = MESH_CFG_MDL_ERR_UNSPECIFIED;
            /* Free memory. */
            WsfBufFree(pRspParams);
            /* Remove key. */
            MeshLocalCfgRemoveAppKey(bind.appKeyIndex, FALSE);
          }
          else
          {
            /* Resume after derivation finishes. */
            return;
          }
        }
      }
    }
  }

  /* Pack key bind. */
  meshCfgMsgPackTwoKeyIndex(&rspMsgParam[1], bind.netKeyIndex, bind.appKeyIndex);

  /* Send response for error cases or key already existing. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_APPKEY_STATUS, rspMsgParam, sizeof(rspMsgParam), src, ttl, netKeyIndex);
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Config AppKey Update request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleAppKeyUpdt(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                  uint8_t ttl, uint16_t netKeyIndex)
{
  uint8_t *pKey;
  keyAddUpdtParams_t *pRspParams;
  meshAppNetKeyBind_t bind;
  uint16_t boundNetKeyIndex;
  meshSecRetVal_t retVal;
  uint8_t rspMsgParam[CFG_MDL_MSG_APPKEY_STATUS_NUM_BYTES];
  meshKeyRefreshStates_t keyRefreshState;

  /* Validate length. */
  if ((msgParamLen != CFG_MDL_MSG_APPKEY_UPDT_NUM_BYTES) || (pMsgParam == NULL))
  {
    return;
  }

  /* Unpack NetKeyIndex and AppKeyIndex. */
  pMsgParam += meshCfgMsgUnpackTwoKeyIndex(pMsgParam, &bind.netKeyIndex, &bind.appKeyIndex);

  /* Point to AppKey. */
  pKey = pMsgParam;

  /* Verify NetKeyIndex by reading key refresh state. */
  keyRefreshState = MeshLocalCfgGetKeyRefreshPhaseState(bind.netKeyIndex);
  if (keyRefreshState >= MESH_KEY_REFRESH_PROHIBITED_START)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_NETKEY_INDEX;
  }
  /* Check if binding exists. */
  else if (MeshLocalCfgGetBoundNetKeyIndex(bind.appKeyIndex, &boundNetKeyIndex) != MESH_SUCCESS)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_APPKEY_INDEX;
  }
  /* Check if bound NetKeyIndex is the same as the one received. */
  else if (boundNetKeyIndex != bind.netKeyIndex)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_BINDING;
  }
  /* Check if Key Refresh State allows Update. */
  else if (keyRefreshState != MESH_KEY_REFRESH_FIRST_PHASE)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_CANNOT_UPDATE;
  }
  else
  {
    /* Try to read updated key identified by AppKeyIndex. */
    if (MeshLocalCfgGetUpdatedAppKey(bind.appKeyIndex, tempKey) == MESH_SUCCESS)
    {
      /* Key exists, verify if the same. */
      if (memcmp(pKey, tempKey, MESH_KEY_SIZE_128) == 0)
      {
        rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;
      }
      else
      {
        rspMsgParam[0] = MESH_CFG_MDL_ERR_CANNOT_UPDATE;
      }
    }
    else
    {
      retVal = (meshSecRetVal_t)MeshLocalCfgUpdateAppKey(bind.appKeyIndex, (const uint8_t *)pKey);
      /* Try to store it in local config. */
      if (retVal != MESH_SUCCESS)
      {
        MESH_TRACE_WARN1("CFG SR: AppKey update store failed with code %x", retVal);
        /* Should never happen. */
        rspMsgParam[0] = MESH_CFG_MDL_ERR_CANNOT_UPDATE;
      }
      else
      {
        /* Allocate memory to store response parameters at the end of the async key derivation. */
        if ((pRspParams = (keyAddUpdtParams_t *)WsfBufAlloc(sizeof(keyAddUpdtParams_t))) == NULL)
        {
          /* No resources to complete request. */
          return;
        }
        else
        {
          /* Configure parameters. */
          pRspParams->netKeyIndex = bind.netKeyIndex;
          pRspParams->appKeyIndex = bind.appKeyIndex;
          pRspParams->cfgMdlClAddr = src;
          pRspParams->ttl = ttl;
          pRspParams->recvNetKeyIndex = netKeyIndex;
          pRspParams->isUpdate = TRUE;

          /* Call security to derive the key material. */
          retVal = MeshSecAddKeyMaterial(MESH_SEC_KEY_TYPE_APP, bind.appKeyIndex, TRUE,
                                         appKeyMatCback,(void *)pRspParams);
          if (retVal != MESH_SUCCESS)
          {
            MESH_TRACE_WARN1("CFG SR: AppKey update derivation failed with code %x", retVal);

            /* Set error code. */
            rspMsgParam[0] = MESH_CFG_MDL_ERR_CANNOT_UPDATE;
            /* Free memory. */
            WsfBufFree(pRspParams);

            /* Read old key. */
            (void) MeshLocalCfgGetAppKey(pRspParams->appKeyIndex, tempKey);
            /* Replace old key with new. */
            MeshLocalCfgRemoveAppKey(pRspParams->appKeyIndex, TRUE);
            /* Set the old key back as updated key. */
            MeshLocalCfgUpdateAppKey(pRspParams->appKeyIndex, (const uint8_t *)tempKey);
            /* Replace new key with old. */
            MeshLocalCfgRemoveAppKey(pRspParams->appKeyIndex, TRUE);
          }
          else
          {
            /* Resume after derivation finishes. */
            return;
          }
        }
      }
    }
  }

  /* Pack key bind. */
  meshCfgMsgPackTwoKeyIndex(&rspMsgParam[1], bind.netKeyIndex, bind.appKeyIndex);

  /* Send response for error cases or key already existing. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_APPKEY_STATUS, rspMsgParam, sizeof(rspMsgParam), src, ttl, netKeyIndex);
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Config AppKey Delete request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleAppKeyDel(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                 uint8_t ttl, uint16_t netKeyIndex)
{
  uint16_t boundNetKeyIndex;
  meshCfgMdlAppKeyChgEvt_t evt =
  {
    .cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT,
    .cfgMdlHdr.hdr.param = MESH_CFG_MDL_APPKEY_DEL_EVENT,
    .cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS
  };
  uint8_t rspMsgParam[CFG_MDL_MSG_APPKEY_STATUS_NUM_BYTES];
  bool_t triggerCback = FALSE;

  /* Validate length. */
  if ((msgParamLen != CFG_MDL_MSG_APPKEY_DEL_NUM_BYTES) || (pMsgParam == NULL))
  {
    return;
  }

  /* Unpack NetKeyIndex and AppKeyIndex. */
  (void) meshCfgMsgUnpackTwoKeyIndex(pMsgParam, &evt.bind.netKeyIndex, &evt.bind.appKeyIndex);

  /* Verify NetKeyIndex by reading key refresh state. */
  if (MeshLocalCfgGetKeyRefreshPhaseState(evt.bind.netKeyIndex) >= MESH_KEY_REFRESH_PROHIBITED_START)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_NETKEY_INDEX;
  }
  /* Verify AppKeyIndex by reading the bound NetKeyIndex. */
  else if(MeshLocalCfgGetBoundNetKeyIndex(evt.bind.appKeyIndex, &boundNetKeyIndex) != MESH_SUCCESS)
  {
    /* AppKeyIndex not found means success for delete operation. */
    rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;
  }
  /* Verify that bound NetKeyIndex matches the one in the request. */
  else if(boundNetKeyIndex != evt.bind.netKeyIndex)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_BINDING;
  }
  else
  {
    rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;

    /* Clean-up AppKey dependencies. */
    appKeyDelCleanup(evt.bind.appKeyIndex);

    /* Remove key material. */
    MeshSecRemoveKeyMaterial(MESH_SEC_KEY_TYPE_APP, evt.bind.appKeyIndex, FALSE);

    /* Remove key. */
    MeshLocalCfgRemoveAppKey(evt.bind.appKeyIndex, FALSE);

    /* Key really removed. Upper layer can be informed. */
    triggerCback = TRUE;
  }

  /* Pack key bind. */
  meshCfgMsgPackTwoKeyIndex(&rspMsgParam[1], evt.bind.netKeyIndex, evt.bind.appKeyIndex);

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_APPKEY_STATUS, rspMsgParam, sizeof(rspMsgParam), src, ttl,
                      netKeyIndex);

  if (triggerCback)
  {
    /* Set client address. */
    evt.cfgMdlHdr.peerAddress = src;
    /* Trigger user callback. */
    meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)&evt);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for Config AppKey Get request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleAppKeyGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                 uint8_t ttl, uint16_t netKeyIndex)
{
  uint8_t *pRspMsgParam, *pTemp;
  uint8_t rspMsgParamEmpty[CFG_MDL_MSG_APPKEY_LIST_EMPTY_NUM_BYTES];
  uint16_t rspLen;
  uint16_t msgNetKeyIndex;
  uint16_t numKeyIndexes = 0;
  uint16_t keyIndex1, keyIndex2;
  uint16_t indexer;

  /* Validate length. */
  if ((msgParamLen != CFG_MDL_MSG_APPKEY_GET_NUM_BYTES) || (pMsgParam == NULL))
  {
    return;
  }

  /* Point to an empty list message. */
  pRspMsgParam = rspMsgParamEmpty;
  rspLen = sizeof(rspMsgParamEmpty);

  /* Unpack NetKeyIndex. */
  meshCfgMsgUnpackSingleKeyIndex(pMsgParam, &msgNetKeyIndex);

  /* Read Key Refresh Phase to determine if NetKey exists. */
  if (MeshLocalCfgGetKeyRefreshPhaseState(msgNetKeyIndex) >= MESH_KEY_REFRESH_PROHIBITED_START)
  {
    pRspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_NETKEY_INDEX;
  }
  else
  {
    /* Get number of bound AppKeys. */
    numKeyIndexes = MeshLocalCfgCountBoundAppKeys(msgNetKeyIndex);

    if (numKeyIndexes != 0)
    {
      rspLen = CFG_MDL_MSG_APPKEY_LIST_NUM_BYTES(numKeyIndexes);
      /* Allocate memory for the response. */
      if ((pRspMsgParam = (uint8_t *)WsfBufAlloc(rspLen)) == NULL)
      {
        /* No resources to complete request. */
        return;
      }
      else
      {
        indexer = 0;
        pTemp = pRspMsgParam + CFG_MDL_MSG_APPKEY_LIST_EMPTY_NUM_BYTES;
        /* Attempt to read two key indexes at a time to follow encoding rules. */
        while (numKeyIndexes > 1)
        {
          MeshLocalCfgGetNextBoundAppKey(msgNetKeyIndex, &keyIndex1, &indexer);
          MeshLocalCfgGetNextBoundAppKey(msgNetKeyIndex, &keyIndex2, &indexer);

          /* Note: number of key indexes already confirmed, keyIndex values cannot be
           * uninitialized.
           */
          /* coverity[uninit_use_in_call] */
          pTemp += meshCfgMsgPackTwoKeyIndex(pTemp, keyIndex1, keyIndex2);
          numKeyIndexes -= 2;
        }
        /* If there is an odd number of AppKey Indexes, pack the last one. */
        if (numKeyIndexes != 0)
        {
          MeshLocalCfgGetNextBoundAppKey(msgNetKeyIndex, &keyIndex1, &indexer);
          meshCfgMsgPackSingleKeyIndex(pTemp, keyIndex1);
        }
        /* Set status to success. */
        pRspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;
      }
    }
    else
    {
      pRspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;
    }
  }

  /* Pack NetKeyIndex. */
  meshCfgMsgPackSingleKeyIndex(&pRspMsgParam[1], msgNetKeyIndex);

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_APPKEY_LIST, pRspMsgParam, rspLen, src, ttl, netKeyIndex);
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Key Refresh Phase Get request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleKeyRefPhaseGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                      uint8_t ttl, uint16_t netKeyIndex)
{
  uint8_t *ptr;
  uint16_t msgNetKeyIndex;
  meshKeyRefreshStates_t phaseState;
  uint8_t rspMsgParam[CFG_MDL_MSG_KEY_REF_PHASE_STATUS_NUM_BYTES];

  /* Validate length. */
  if (msgParamLen != CFG_MDL_MSG_KEY_REF_PHASE_GET_NUM_BYTES)
  {
    return;
  }
  /* Unpack NetKeyIndex. */
  meshCfgMsgUnpackSingleKeyIndex(pMsgParam, &msgNetKeyIndex);

  /* Read from local config. */
  phaseState = MeshLocalCfgGetKeyRefreshPhaseState(msgNetKeyIndex);

  /* Set the status code to success or invalid NetKeyIndex. */
  rspMsgParam[0] = phaseState < MESH_KEY_REFRESH_PROHIBITED_START ?
                                MESH_CFG_MDL_SR_SUCCESS : MESH_CFG_MDL_ERR_INVALID_NETKEY_INDEX;

  ptr = &rspMsgParam[1];
  /* Pack NetKeyIndex and Key Refresh Phase state. */
  ptr += meshCfgMsgPackSingleKeyIndex(ptr, msgNetKeyIndex);

  if (rspMsgParam[0] != MESH_CFG_MDL_SR_SUCCESS)
  {
    phaseState = MESH_KEY_REFRESH_NOT_ACTIVE;
  }
  /* Pack Key Refresh Phase state. */
  UINT8_TO_BSTREAM(ptr, phaseState);

  /* Send KeyRefresh Phase Status. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_KEY_REF_PHASE_STATUS, rspMsgParam,
                      CFG_MDL_MSG_KEY_REF_PHASE_STATUS_NUM_BYTES, src, ttl, netKeyIndex);
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Key Refresh Phase Set request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlSrHandleKeyRefPhaseSet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                      uint8_t ttl, uint16_t netKeyIndex)
{
  uint8_t *ptr;
  uint8_t rspMsgParam[CFG_MDL_MSG_KEY_REF_PHASE_STATUS_NUM_BYTES];
  meshKeyRefreshStates_t oldState;
  meshKeyRefreshStates_t newState = MESH_KEY_REFRESH_NOT_ACTIVE;
  meshKeyRefreshStates_t transition;

  meshCfgMdlKeyRefPhaseEvt_t evt =
  {
    .cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT,
    .cfgMdlHdr.hdr.param = MESH_CFG_MDL_KEY_REF_PHASE_SET_EVENT,
    .cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS
  };

  /* Validate length. */
  if ((msgParamLen != CFG_MDL_MSG_KEY_REF_PHASE_SET_NUM_BYTES) || (pMsgParam == NULL))
  {
    return;
  }

  /* Unpack NetKeyIndex. */
  pMsgParam += meshCfgMsgUnpackSingleKeyIndex(pMsgParam, &evt.netKeyIndex);
  /* Unpack transition. */
  BSTREAM_TO_UINT8(transition, pMsgParam);

  /* Validate Key Refresh transition value. */
  if ((transition != MESH_KEY_REFRESH_TRANS02) && (transition != MESH_KEY_REFRESH_TRANS03))
  {
    return;
  }

  /* Read old state. */
  oldState = MeshLocalCfgGetKeyRefreshPhaseState(evt.netKeyIndex);

  if (oldState < MESH_KEY_REFRESH_PROHIBITED_START)
  {
    switch (transition)
    {
      case MESH_KEY_REFRESH_TRANS02:
        if ((oldState == MESH_KEY_REFRESH_FIRST_PHASE) ||
            (oldState == MESH_KEY_REFRESH_SECOND_PHASE))
        {
          newState = MESH_KEY_REFRESH_SECOND_PHASE;
        }
        else
        {
          /* Consider prohibited. */
          return;
        }
        break;
      case MESH_KEY_REFRESH_TRANS03:
        if (oldState == MESH_KEY_REFRESH_NOT_ACTIVE)
        {
          newState = oldState;
        }
        else if (oldState < MESH_KEY_REFRESH_THIRD_PHASE)
        {
          newState = MESH_KEY_REFRESH_NOT_ACTIVE;
        }
        else
        {
          /* Consider prohibited. */
          return;
        }
        break;

      /* coverity[dead_error_begin] */
      default:
        return;
    }

    /* Call Network Management. */
    MeshNwkMgmtHandleKeyRefreshTrans(netKeyIndex, oldState, newState);

    evt.keyRefState = newState;
    rspMsgParam[0]  = MESH_CFG_MDL_SR_SUCCESS;
  }
  else
  {
    evt.keyRefState = MESH_KEY_REFRESH_NOT_ACTIVE;
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_NETKEY_INDEX;
  }

  ptr = &rspMsgParam[1];

  /* Pack NetKeyIndex. */
  ptr += meshCfgMsgPackSingleKeyIndex(ptr, evt.netKeyIndex);

  /* Pack new state. */
  UINT8_TO_BSTREAM(ptr, evt.keyRefState);

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_KEY_REF_PHASE_STATUS, rspMsgParam,
                      CFG_MDL_MSG_KEY_REF_PHASE_STATUS_NUM_BYTES, src, ttl, netKeyIndex);

  if (rspMsgParam[0] == MESH_CFG_MDL_SR_SUCCESS)
  {
    /* Set event parameters. */
    evt.cfgMdlHdr.peerAddress = src;

    /* Call notification callback. */
    meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)&evt);
  }
}
