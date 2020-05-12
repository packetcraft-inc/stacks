/*************************************************************************************************/
/*!
 *  \file   mesh_cfg_mdl_sr_app_key.c
 *
 *  \brief  Model to AppKey messages implementation.
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
#include "mesh_access.h"

#include "mesh_utils.h"

#include "mesh_cfg_mdl_api.h"
#include "mesh_cfg_mdl_sr_api.h"
#include "mesh_cfg_mdl.h"
#include "mesh_cfg_mdl_sr.h"
#include "mesh_cfg_mdl_sr_main.h"

#include "mesh_cfg_mdl_messages.h"

#include <string.h>

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Config Model App Bind request.
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
void meshCfgMdlSrHandleModelAppBind(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                    uint8_t ttl, uint16_t netKeyIndex)
{
  meshModelId_t mdlId;
  uint16_t dummyNetKeyIndex;
  uint8_t rspMsgParam[CFG_MDL_MSG_MODEL_APP_STATUS_MAX_NUM_BYTES];
  meshElementId_t elemId;
  meshCfgMdlModelAppBindEvt_t evt =
  {
    .cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT,
    .cfgMdlHdr.hdr.param = MESH_CFG_MDL_APP_BIND_EVENT,
    .cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS
  };

  /* Validate length. */
  if (pMsgParam == NULL)
  {
    return;
  }
  else if (msgParamLen == CFG_MDL_MSG_MODEL_APP_BIND_NUM_BYTES(TRUE))
  {
    evt.isSig = TRUE;
  }
  else if (msgParamLen == CFG_MDL_MSG_MODEL_APP_BIND_NUM_BYTES(FALSE))
  {
    evt.isSig = FALSE;
  }
  else
  {
    return;
  }

  /* Unpack bind. */
  meshCfgMsgUnpackModelAppBind(pMsgParam, &evt.elemAddr, &evt.appKeyIndex, &evt.modelId.sigModelId,
                               &evt.modelId.vendorModelId, evt.isSig);

  /* Verify element address. */
  if (!MESH_IS_ADDR_UNICAST(evt.elemAddr))
  {
    return;
  }

  /* Get element id and also validate element address exists on node. */
  if (MeshLocalCfgGetElementIdFromAddr(evt.elemAddr, &elemId) != MESH_SUCCESS)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_ADDR;
  }
  /* Validate AppKey Index by reading the bound NetKey Index. */
  else if (MeshLocalCfgGetBoundNetKeyIndex(evt.appKeyIndex, &dummyNetKeyIndex) != MESH_SUCCESS)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_APPKEY_INDEX;
  }
  else if((elemId == 0) && (evt.isSig) &&
          ((evt.modelId.sigModelId == MESH_CFG_MDL_SR_MODEL_ID) ||
           (evt.modelId.sigModelId == MESH_CFG_MDL_CL_MODEL_ID)))
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_CANNOT_BIND;
  }
  else
  {
    /* Build model id structure for local config. */
    mdlId.isSigModel = evt.isSig;
    if (mdlId.isSigModel)
    {
      mdlId.modelId.sigModelId = evt.modelId.sigModelId;
    }
    else
    {
      mdlId.modelId.vendorModelId = evt.modelId.vendorModelId;
    }

    /* Check if model exists. */
    if (!MeshLocalCfgModelExists(elemId, (const meshModelId_t *)&mdlId))
    {
      rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_MODEL;
    }
    else
    {
      /* Bind AppKey to model. */
      switch (MeshLocalCfgBindAppKeyToModel(elemId, (const meshModelId_t *)&mdlId, evt.appKeyIndex))
      {
        case MESH_SUCCESS:
        /* Fall-through. */
        case MESH_LOCAL_CFG_ALREADY_EXIST:
          rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;
          break;
        case MESH_LOCAL_CFG_OUT_OF_MEMORY:
          rspMsgParam[0] = MESH_CFG_MDL_ERR_INSUFFICIENT_RESOURCES;
          break;
        default:
          rspMsgParam[0] = MESH_CFG_MDL_ERR_CANNOT_BIND;
          break;
      }
    }
  }

  /* Pack binding. */
  meshCfgMsgPackModelAppBind(&rspMsgParam[1], evt.elemAddr, evt.appKeyIndex, evt.modelId.sigModelId,
                             evt.modelId.vendorModelId, evt.isSig);

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_MODEL_APP_STATUS, rspMsgParam,
                      CFG_MDL_MSG_MODEL_APP_STATUS_NUM_BYTES(evt.isSig), src, ttl, netKeyIndex);

  /* On success, trigger user callback. */
  if (rspMsgParam[0] == MESH_CFG_MDL_SR_SUCCESS)
  {
    evt.cfgMdlHdr.peerAddress = src;
    meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)&evt);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Config Model App Unbind request.
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
void meshCfgMdlSrHandleModelAppUnbind(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                      uint8_t ttl, uint16_t netKeyIndex)
{
  uint8_t *pLabelUuid;
  meshModelId_t mdlId;
  uint16_t dummyKeyIndex;
  meshAddress_t pubAddr;
  meshLocalCfgRetVal_t retVal;
  uint8_t rspMsgParam[CFG_MDL_MSG_MODEL_APP_STATUS_MAX_NUM_BYTES];
  meshElementId_t elemId;
  bool_t triggerCback = FALSE;
  meshCfgMdlModelAppBindEvt_t evt =
  {
    .cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT,
    .cfgMdlHdr.hdr.param = MESH_CFG_MDL_APP_UNBIND_EVENT,
    .cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS
  };

  /* Validate length. */
  if (pMsgParam == NULL)
  {
    return;
  }
  else if (msgParamLen == CFG_MDL_MSG_MODEL_APP_UNBIND_NUM_BYTES(TRUE))
  {
    evt.isSig = TRUE;
  }
  else if (msgParamLen == CFG_MDL_MSG_MODEL_APP_UNBIND_NUM_BYTES(FALSE))
  {
    evt.isSig = FALSE;
  }
  else
  {
    return;
  }

  /* Unpack bind. */
  meshCfgMsgUnpackModelAppBind(pMsgParam, &evt.elemAddr, &evt.appKeyIndex, &evt.modelId.sigModelId,
                               &evt.modelId.vendorModelId, evt.isSig);

  /* Verify element address. */
  if (!MESH_IS_ADDR_UNICAST(evt.elemAddr))
  {
    return;
  }

  /* Get element id and also validate element address exists on node. */
  if (MeshLocalCfgGetElementIdFromAddr(evt.elemAddr, &elemId) != MESH_SUCCESS)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_ADDR;
  }
  /* Validate AppKey Index by reading the bound NetKey Index. */
  else if (MeshLocalCfgGetBoundNetKeyIndex(evt.appKeyIndex, &dummyKeyIndex) != MESH_SUCCESS)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_APPKEY_INDEX;
  }
  else
  {
    /* Build model id structure for local config. */
    mdlId.isSigModel = evt.isSig;
    if (mdlId.isSigModel)
    {
      mdlId.modelId.sigModelId = evt.modelId.sigModelId;
    }
    else
    {
      mdlId.modelId.vendorModelId = evt.modelId.vendorModelId;
    }

    /* Check if model exists. */
    if (!MeshLocalCfgModelExists(elemId, (const meshModelId_t *)&mdlId))
    {
      rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_MODEL;
    }
    /* Check if bind exists. */
    else if (!MeshLocalCfgValidateModelToAppKeyBind(elemId, &mdlId, evt.appKeyIndex))
    {
      rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;
    }
    else
    {
      rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;
      triggerCback = TRUE;

      /* Unbind AppKey from model. */
      MeshLocalCfgUnbindAppKeyFromModel(elemId, (const meshModelId_t *)&mdlId, evt.appKeyIndex);

      /* Check and disable publication if needed. */

      /* Read publish address. Always allowed. */
      retVal = MeshLocalCfgGetPublishAddress(elemId, (const meshModelId_t *)&mdlId, &pubAddr,
                                             &pLabelUuid);
      WSF_ASSERT(retVal == MESH_SUCCESS);

      /* Skip models that have publication disabled. */
      /* Note:  because mdlId has been confirmed to exist on elemId, pubAddr will always be
       * initialized by the above call.
       */
      /* coverity[uninit_use] */
      if (!MESH_IS_ADDR_UNASSIGNED(pubAddr))
      {
        /* Read publish AppKeyIndex. Always allowed if publication is enabled. */
        retVal = MeshLocalCfgGetPublishAppKeyIndex(elemId, (const meshModelId_t *)&mdlId,
                                                   &dummyKeyIndex);
        WSF_ASSERT(retVal == MESH_SUCCESS);

        /* Check if it matches the one from the bind. */
        if (dummyKeyIndex == evt.appKeyIndex)
        {
          /* Disable publishing. */
          (void) MeshLocalCfgSetPublishAddress(elemId, (const meshModelId_t *)&mdlId,
                                                 MESH_ADDR_TYPE_UNASSIGNED);
          /* Set Model Publication parameters to 0. */
          MeshLocalCfgMdlClearPublishAppKeyIndex(elemId, (const meshModelId_t *)&mdlId);
          retVal = MeshLocalCfgSetPublishFriendshipCredFlag(elemId, (const meshModelId_t *)&mdlId,
                                                            FALSE);
          WSF_ASSERT(retVal == MESH_SUCCESS);
          retVal = MeshLocalCfgSetPublishPeriod(elemId, (const meshModelId_t *)&mdlId, 0, 0);
          WSF_ASSERT(retVal == MESH_SUCCESS);
          retVal = MeshLocalCfgSetPublishRetransCount(elemId, (const meshModelId_t *)&mdlId, 0);
          WSF_ASSERT(retVal == MESH_SUCCESS);
          retVal = MeshLocalCfgSetPublishRetransIntvlSteps(elemId, (const meshModelId_t *)&mdlId, 0);
          WSF_ASSERT(retVal == MESH_SUCCESS);
          retVal = MeshLocalCfgSetPublishTtl(elemId, (const meshModelId_t *)&mdlId, 0);
          WSF_ASSERT(retVal == MESH_SUCCESS);

          /* Notify Access Layer that periodic publishing state has changed.*/
          MeshAccPeriodPubChanged(elemId, &mdlId);
        }
      }
    }
  }

  /* Pack binding. */
  meshCfgMsgPackModelAppBind(&rspMsgParam[1], evt.elemAddr, evt.appKeyIndex, evt.modelId.sigModelId,
                             evt.modelId.vendorModelId, evt.isSig);

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_MODEL_APP_STATUS, rspMsgParam,
                      CFG_MDL_MSG_MODEL_APP_STATUS_NUM_BYTES(evt.isSig), src, ttl, netKeyIndex);

  /* On success, trigger user callback. */
  if (triggerCback)
  {
    evt.cfgMdlHdr.peerAddress = src;
    meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)&evt);
  }
  (void)retVal;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Config SIG Model App Get request.
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
static void meshCfgMdlSrHandleModelAppGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                          uint8_t ttl, uint16_t netKeyIndex, bool_t isSig)
{
  uint8_t *pRspMsgParam, *pTemp;
  uint16_t rspMsgParamLen, keyIndex1, keyIndex2;
  meshAddress_t elemAddr;
  meshLocalCfgRetVal_t retVal;
  uint8_t count, indexer;
  meshElementId_t elemId;
  meshModelId_t mdlId;
  uint8_t rspMsgParamEmpty[CFG_MDL_MSG_MODEL_APP_LIST_EMPTY_MAX_NUM_BYTES];

  (void)msgParamLen;

  /* Point to empty list. */
  pRspMsgParam = rspMsgParamEmpty;
  /* Set parameters length to empty list size. */
  rspMsgParamLen = CFG_MDL_MSG_MODEL_APP_LIST_EMPTY_NUM_BYTES(isSig);

  /* Unpack element address. */
  BSTREAM_TO_UINT16(elemAddr, pMsgParam);

  /* Build local config model structure. */
  mdlId.isSigModel = isSig;

  /* Unpack model id.*/
  if (isSig)
  {
    BSTREAM_TO_UINT16(mdlId.modelId.sigModelId, pMsgParam);
  }
  else
  {
    BSTREAM_TO_VEND_MDL(mdlId.modelId.vendorModelId, pMsgParam);
  }

  /* Verify element address. */
  if (!MESH_IS_ADDR_UNICAST(elemAddr))
  {
    return;
  }

  /* Get element id and also validate element address exists on node. */
  if (MeshLocalCfgGetElementIdFromAddr(elemAddr, &elemId) != MESH_SUCCESS)
  {
    pRspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_ADDR;
  }
  /* Check if model exists. */
  else if (!MeshLocalCfgModelExists(elemId, (const meshModelId_t *)&mdlId))
  {
    pRspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_MODEL;
  }
  else
  {
    /* Get number of AppKeys bound to model. */
    count = MeshLocalCfgCountModelBoundAppKeys(elemId, (const meshModelId_t *)&mdlId);
    if (count != 0)
    {
      /* Attempt to allocate memory to store the response. */
      if((pRspMsgParam = WsfBufAlloc(CFG_MDL_MSG_MODEL_APP_LIST_NUM_BYTES(isSig, count)))
         == NULL)
      {
        /* No resources to complete request. */
        return;
      }
      else
      {
        rspMsgParamLen = CFG_MDL_MSG_MODEL_APP_LIST_NUM_BYTES(isSig, count);

        indexer = 0;
        pTemp = pRspMsgParam + CFG_MDL_MSG_MODEL_APP_LIST_EMPTY_NUM_BYTES(isSig);
        /* Attempt to read two key indexes at a time to follow encoding rules. */
        while (count > 1)
        {
          retVal = MeshLocalCfgGetNextModelBoundAppKey(elemId, (const meshModelId_t *)&mdlId,
                                                       &keyIndex1, &indexer);
          WSF_ASSERT(retVal == MESH_SUCCESS);
          retVal = MeshLocalCfgGetNextModelBoundAppKey(elemId, (const meshModelId_t *)&mdlId,
                                                       &keyIndex2, &indexer);
          WSF_ASSERT(retVal == MESH_SUCCESS);
          pTemp += meshCfgMsgPackTwoKeyIndex(pTemp, keyIndex1, keyIndex2);
          count -= 2;
        }
        /* If there is an odd number of AppKey Indexes, pack the last one. */
        if (count != 0)
        {
          retVal = MeshLocalCfgGetNextModelBoundAppKey(elemId, (const meshModelId_t *)&mdlId,
                                                       &keyIndex1, &indexer);
          WSF_ASSERT(retVal == MESH_SUCCESS);

          /* Note: a count greater than 0 implies that there is at least one appkey,
           * so keyIndex1 cannot be uninitialized after loading value above.
           */
          /* coverity[uninit_use_in_call] */
          (void)meshCfgMsgPackSingleKeyIndex(pTemp, keyIndex1);
        }
      }
    }
    /* Set success status. */
    pRspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;
  }

  pTemp = &pRspMsgParam[1];
  /* Pack element address. */
  UINT16_TO_BSTREAM(pTemp, elemAddr);

  /* Pack model id. */
  if (isSig)
  {
    UINT16_TO_BSTREAM(pTemp, mdlId.modelId.sigModelId);
  }
  else
  {
    VEND_MDL_TO_BSTREAM(pTemp, mdlId.modelId.vendorModelId);
  }

  /* Send response. */
  meshCfgMdlSrSendRsp(isSig ? MESH_CFG_MDL_SR_MODEL_APP_SIG_LIST : MESH_CFG_MDL_SR_MODEL_APP_VENDOR_LIST,
                      pRspMsgParam, rspMsgParamLen, src, ttl, netKeyIndex);

  /* Check if memory was allocated and free. */
  if (pRspMsgParam != rspMsgParamEmpty)
  {
    WsfBufFree(pRspMsgParam);
  }
  (void)retVal;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Config SIG Model App Get request.
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
void meshCfgMdlSrHandleModelAppSigGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                      uint8_t ttl, uint16_t netKeyIndex)
{
  /* Verify parameters. */
  if ((msgParamLen != CFG_MDL_MSG_MODEL_APP_GET_NUM_BYTES(TRUE)) || (pMsgParam == NULL))
  {
    return;
  }

  /* Call common handler. */
  meshCfgMdlSrHandleModelAppGet(pMsgParam, msgParamLen, src, ttl, netKeyIndex, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Config Vendor Model App Get request.
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
void meshCfgMdlSrHandleModelAppVendorGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                         uint8_t ttl, uint16_t netKeyIndex)
{
  /* Verify parameters. */
  if ((msgParamLen != CFG_MDL_MSG_MODEL_APP_GET_NUM_BYTES(FALSE)) || (pMsgParam == NULL))
  {
    return;
  }

  /* Call common handler. */
  meshCfgMdlSrHandleModelAppGet(pMsgParam, msgParamLen, src, ttl, netKeyIndex, FALSE);
}
