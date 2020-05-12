/*************************************************************************************************/
/*!
 *  \file   mesh_cfg_mdl_sr_pub_subscr.c
 *
 *  \brief  Publish-subscribe implementation.
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
#include "mesh_security_toolbox.h"
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
  Data Types
**************************************************************************************************/

/*! Structure used for storing Model Publication Virtual Set parameters */
typedef struct cfgMdlSrModelPubVirtSetParams_tag
{
  meshCfgMdlModelPubEvt_t      evt;            /*!< User event containing most of the parameters.
                                                *   of the request
                                                */
  uint16_t                     recvNetKeyIndex;/*!< Network Key identifier of the network on which
                                                *   the request is received
                                                */
  uint8_t                      recvTtl;        /*!< TTL of the request */
  meshElementId_t              elemId;         /*!< Element identifier of the element address from
                                                *   the request
                                                */
  uint8_t                      labelUuid[MESH_LABEL_UUID_SIZE]; /*!< Label UUID */
} cfgMdlSrModelPubVirtSetParams_t;

/*! Structure used for storing Model Subscription Virtual Address Add/Overwrite parameters */
typedef struct cfgMdlSrModelSubscrVirtAddOvrParams_tag
{
  meshCfgMdlModelSubscrChgEvt_t evt;            /*!< User event */
  uint16_t                      recvNetKeyIndex;/*!< Network Key identifier of the network on which
                                                 *   the request is received
                                                 */
  uint8_t                       recvTtl;        /*!< TTL of the request */
  meshElementId_t               elemId;         /*!< Element identifier of the element address from
                                                 *   the request
                                                 */
  bool_t                        overwrite;      /*!< TRUE if operation is overwrite, FALSE if add */
  uint8_t                       labelUuid[MESH_LABEL_UUID_SIZE]; /*!< Label UUID */
} cfgMdlSrModelSubscrVirtAddOvrParams_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Salt s1("vtad") */
static const uint8_t saltVtad[MESH_KEY_SIZE_128] =
{
  0xCE, 0xF7, 0xFA, 0x9D, 0xC4, 0x7B, 0xAF, 0x5D,
  0xAA, 0xEE, 0xD1, 0x94, 0x06, 0x09, 0x4F, 0x37
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Model Publication Get request.
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
void meshCfgMdlSrHandleModelPubGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                  uint8_t ttl, uint16_t netKeyIndex)
{
  uint8_t *ptr;
  meshAddress_t elemAddr, pubAddr;
  meshModelId_t mdlId;
  meshModelPublicationParams_t pubParams;
  meshElementId_t elemId;
  uint8_t rspMsgParam[CFG_MDL_MSG_MODEL_PUB_STATUS_MAX_NUM_BYTES];
  meshLocalCfgRetVal_t retVal = MESH_SUCCESS;
  /* Validate pointer. */
  if(pMsgParam == NULL)
  {
    return;
  }

  /* Initialize zero values used in error responses. */
  pubAddr = MESH_ADDR_TYPE_UNASSIGNED;
  mdlId.modelId.sigModelId = 0x0000;
  mdlId.modelId.vendorModelId = 0x00000000;
  memset(&pubParams, 0x00, sizeof(meshModelPublicationParams_t));

  /* Validate length and extract model type. */
  if (msgParamLen != CFG_MDL_MSG_MODEL_PUB_GET_NUM_BYTES(TRUE))
  {
    if (msgParamLen != CFG_MDL_MSG_MODEL_PUB_GET_NUM_BYTES(FALSE))
    {
      return;
    }
    else
    {
      mdlId.isSigModel = FALSE;
    }
  }
  else
  {
    mdlId.isSigModel = TRUE;
  }

  /* Unpack message. */
  meshCfgMsgUnpackModelPubGet(pMsgParam, &elemAddr, &(mdlId.modelId.sigModelId),
                              &(mdlId.modelId.vendorModelId), mdlId.isSigModel);

  /* Get element id. */
  if (MeshLocalCfgGetElementIdFromAddr(elemAddr, &elemId) != MESH_SUCCESS)
  {
    /* Set error status. */
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_ADDR;
  }
  /* Check if model exists. */
  else if (!MeshLocalCfgModelExists(elemId, (const meshModelId_t *) &mdlId))
  {
    /* Error means model not found on element. */
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_MODEL;
  }
  else
  {
    /* Read publish address. */
    (void)MeshLocalCfgGetPublishAddress(elemId, (const meshModelId_t *)&mdlId, &pubAddr, &ptr);

    /* Request is successful. */
    rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;

    /* Check if publishing is enabled. */
    if(!MESH_IS_ADDR_UNASSIGNED(pubAddr))
    {
      /* Read publication parameters which should be valid. */
      retVal = MeshLocalCfgGetPublishAppKeyIndex(elemId, (const meshModelId_t *)&mdlId,
                                                 &pubParams.publishAppKeyIndex);
      WSF_ASSERT(retVal == MESH_SUCCESS);

      /* Read publication parameters which should be valid. */
      retVal = MeshLocalCfgGetPublishTtl(elemId, (const meshModelId_t *)&mdlId,
                                         &pubParams.publishTtl);
      WSF_ASSERT(retVal == MESH_SUCCESS);

      retVal = MeshLocalCfgGetPublishFriendshipCredFlag(elemId, (const meshModelId_t *)&mdlId,
                                                        &pubParams.publishFriendshipCred);
      WSF_ASSERT(retVal == MESH_SUCCESS);

      retVal = MeshLocalCfgGetPublishPeriod(elemId, (const meshModelId_t *)&mdlId,
                                            &pubParams.publishPeriodNumSteps, &pubParams.publishPeriodStepRes);
      WSF_ASSERT(retVal == MESH_SUCCESS);

      retVal = MeshLocalCfgGetPublishRetransCount(elemId, (const meshModelId_t *)&mdlId,
                                                  &pubParams.publishRetransCount);
      WSF_ASSERT(retVal == MESH_SUCCESS);

      retVal = MeshLocalCfgGetPublishRetransIntvlSteps(elemId, (const meshModelId_t *)&mdlId,
                                                       &pubParams.publishRetransSteps50Ms);
      WSF_ASSERT(retVal == MESH_SUCCESS);
    }
    else
    {
      /* Clear publication parameters. */
      memset(&pubParams, 0, sizeof(pubParams));
    }
  }

  /* Check if error is encountered. */
  if (rspMsgParam[0] != MESH_CFG_MDL_SR_SUCCESS)
  {
    /* Clear publication parameters. */
    memset(&pubParams, 0, sizeof(pubParams));
    pubAddr = MESH_ADDR_TYPE_UNASSIGNED;
  }

  /* Prepare response */
  ptr = &rspMsgParam[1];

  /* Pack element address. */
  UINT16_TO_BSTREAM(ptr, elemAddr);

  /* Pack publish address. */
  UINT16_TO_BSTREAM(ptr, pubAddr);

  /* Pack parameters. */
  meshCfgMsgPackModelPubParam(ptr, &pubParams, mdlId.modelId.sigModelId,
                              mdlId.modelId.vendorModelId, mdlId.isSigModel);
  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_MODEL_PUB_STATUS, rspMsgParam,
                      CFG_MDL_MSG_MODEL_PUB_STATUS_NUM_BYTES(mdlId.isSigModel), src, ttl, netKeyIndex);

  /* In case WSF_ASSERT is disabled. */
  (void)retVal;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Model Publication Set request.
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
void meshCfgMdlSrHandleModelPubSet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                   uint8_t ttl, uint16_t netKeyIndex)
{
  uint8_t *ptr;
  meshCfgMdlModelPubEvt_t evt;
  meshModelId_t mdlId;
  uint16_t dummyNetKeyIndex;
  meshElementId_t elemId;
  uint8_t rspMsgParam[CFG_MDL_MSG_MODEL_PUB_STATUS_MAX_NUM_BYTES];
  meshLocalCfgRetVal_t retVal = MESH_SUCCESS;

  /* Validate pointer. */
  if(pMsgParam == NULL)
  {
    return;
  }

  /* Validate length and extract model type. */
  if (msgParamLen != CFG_MDL_MSG_MODEL_PUB_SET_NUM_BYTES(TRUE))
  {
    if (msgParamLen != CFG_MDL_MSG_MODEL_PUB_SET_NUM_BYTES(FALSE))
    {
      return;
    }
    else
    {
      mdlId.isSigModel = FALSE;
    }
  }
  else
  {
    mdlId.isSigModel = TRUE;
  }

  /* Unpack Model Publication Set parameters. */
  ptr = pMsgParam;

  /* Get element address. */
  BSTREAM_TO_UINT16(evt.elemAddr, ptr);
  /* Get publish address. */
  BSTREAM_TO_UINT16(evt.pubAddr, ptr);
  /* Get publication parameters. */
  meshCfgMsgUnpackModelPubParam(ptr, &evt.pubParams, &mdlId.modelId.sigModelId,
                                &mdlId.modelId.vendorModelId, mdlId.isSigModel);

  /* Set status to success. */
  rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;

  /* Validate publication parameters. */
  if(MESH_IS_ADDR_VIRTUAL(evt.pubAddr) || !MESH_TTL_IS_VALID(evt.pubParams.publishTtl) ||
     (evt.pubParams.publishAppKeyIndex > MESH_APP_KEY_INDEX_MAX_VAL))
  {
    /* Prohibited values */
    return;
  }

  /* Get element id. */
  if (MeshLocalCfgGetElementIdFromAddr(evt.elemAddr, &elemId) != MESH_SUCCESS)
  {
    /* Set error status. */
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_ADDR;
  }
  /* Check if model instance exists. */
  else if (!MeshLocalCfgModelExists(elemId, &mdlId))
  {
    /* Set error status. */
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_MODEL;
  }
  /* Check that AppKey exists on device by reading the bound NetKey. */
  else if (!MESH_IS_ADDR_UNASSIGNED(evt.pubAddr) &&
           MeshLocalCfgGetBoundNetKeyIndex(evt.pubParams.publishAppKeyIndex, &dummyNetKeyIndex)
           != MESH_SUCCESS)
  {
    /* AppKeyIndex is invalid. */
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_APPKEY_INDEX;
  }
  else if ((evt.pubParams.publishFriendshipCred) && (MeshLocalCfgGetLowPowerState() >= MESH_LOW_POWER_FEATURE_PROHIBITED_START))
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_FEATURE_NOT_SUPPORTED;
  }
  else
  {
    /* Set publish address. */
    retVal = MeshLocalCfgSetPublishAddress(elemId, &mdlId, evt.pubAddr);

    if(retVal == MESH_SUCCESS)
    {
      /* Set publication parameters only if publication is enabled. */
      if(MESH_IS_ADDR_UNASSIGNED(evt.pubAddr))
      {
        memset(&evt.pubParams, 0, sizeof(meshModelPublicationParams_t));

        /* Clear AppKey Index. */
        MeshLocalCfgMdlClearPublishAppKeyIndex(elemId, (const meshModelId_t *)&mdlId);
      }
      else
      {
        /* Set Model Publication parameters. */
        MeshLocalCfgSetPublishAppKeyIndex(elemId, (const meshModelId_t *)&mdlId,
                                          evt.pubParams.publishAppKeyIndex);
      }

      /* Set Model Publication parameters. */
      retVal = MeshLocalCfgSetPublishFriendshipCredFlag(elemId, (const meshModelId_t *)&mdlId,
                                                        evt.pubParams.publishFriendshipCred);
      WSF_ASSERT(retVal == MESH_SUCCESS);

      retVal = MeshLocalCfgSetPublishPeriod(elemId, (const meshModelId_t *)&mdlId,
                                            evt.pubParams.publishPeriodNumSteps,
                                            evt.pubParams.publishPeriodStepRes);
      WSF_ASSERT(retVal == MESH_SUCCESS);

      retVal = MeshLocalCfgSetPublishRetransCount(elemId, (const meshModelId_t *)&mdlId,
                                                  evt.pubParams.publishRetransCount);
      WSF_ASSERT(retVal == MESH_SUCCESS);

      retVal = MeshLocalCfgSetPublishRetransIntvlSteps(elemId, (const meshModelId_t *)&mdlId,
                                                       evt.pubParams.publishRetransSteps50Ms);
      WSF_ASSERT(retVal == MESH_SUCCESS);

      retVal = MeshLocalCfgSetPublishTtl(elemId, (const meshModelId_t *)&mdlId,
                                         evt.pubParams.publishTtl);
      WSF_ASSERT(retVal == MESH_SUCCESS);

      /* Notify Access Layer that periodic publishing state has changed.*/
      MeshAccPeriodPubChanged(elemId, &mdlId);
    }
    else
    {
      switch (retVal)
      {
        case MESH_LOCAL_CFG_OUT_OF_MEMORY:
          /* Set error to insufficient resources. */
          rspMsgParam[0] = MESH_CFG_MDL_ERR_INSUFFICIENT_RESOURCES;
          break;
        default:
          /* Set error to unspecified. */
          rspMsgParam[0] = MESH_CFG_MDL_ERR_UNSPECIFIED;
          break;
      }
    }
  }

  /* Clear publication parameters on error. */
  if (rspMsgParam[0] != MESH_CFG_MDL_SR_SUCCESS)
  {
    memset(&evt.pubParams, 0, sizeof(evt.pubParams));
    evt.pubAddr = MESH_ADDR_TYPE_UNASSIGNED;
  }

  /* Pack response. */
  ptr = &rspMsgParam[1];

  /* Pack element address. */
  UINT16_TO_BSTREAM(ptr, evt.elemAddr);

  /* Pack publish address. */
  UINT16_TO_BSTREAM(ptr, evt.pubAddr);

  /* Pack publication parameters. */
  meshCfgMsgPackModelPubParam(ptr, &evt.pubParams, mdlId.modelId.sigModelId,
                              mdlId.modelId.vendorModelId, mdlId.isSigModel);

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_MODEL_PUB_STATUS, rspMsgParam,
                      CFG_MDL_MSG_MODEL_PUB_STATUS_NUM_BYTES(mdlId.isSigModel), src, ttl, netKeyIndex);

  if (rspMsgParam[0] == MESH_CFG_MDL_SR_SUCCESS)
  {
    /* Set event parameters. */
    evt.cfgMdlHdr.hdr.event  = MESH_CFG_MDL_SR_EVENT;
    evt.cfgMdlHdr.hdr.param  = MESH_CFG_MDL_PUB_SET_EVENT;
    evt.cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS;

    /* Set client address. */
    evt.cfgMdlHdr.peerAddress = src;

    /* Set model id. */
    evt.isSig = mdlId.isSigModel;
    if (evt.isSig)
    {
      evt.modelId.sigModelId = mdlId.modelId.sigModelId;
    }
    else
    {
      evt.modelId.vendorModelId = mdlId.modelId.vendorModelId;
    }

    /* Call notification callback. */
    meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)&evt);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Stores Model Publication Virtual Set parameters in the local device.
 *
 *  \param[in] pVSetParams  Pointer to structure containing fields from the Model Publication
 *                          Virtual Set request.
 *  \param[in] virtAddr     Virtual Address used for setting publish address.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static uint8_t cfgMdlSrModelPubVirtSetStore(cfgMdlSrModelPubVirtSetParams_t *pVSetParams,
                                            meshAddress_t virtAddr)
{
  meshCfgMdlModelPubEvt_t *pEvt;
  uint8_t status = MESH_CFG_MDL_SR_SUCCESS;
  meshLocalCfgRetVal_t retVal;
  meshModelId_t mdlId;

  /* Extract event. */
  pEvt = &pVSetParams->evt;

  /* Create local config model id structure for accessing model publication information. */
  mdlId.isSigModel = pEvt->isSig;
  if (mdlId.isSigModel)
  {
    mdlId.modelId.sigModelId = pEvt->modelId.sigModelId;
  }
  else
  {
    mdlId.modelId.vendorModelId = pEvt->modelId.vendorModelId;
  }

  /* Set publish address. */
  retVal = MeshLocalCfgSetPublishVirtualAddr(pVSetParams->elemId,
                                             (const meshModelId_t *)&mdlId,
                                             pVSetParams->labelUuid, virtAddr);

  if (retVal == MESH_SUCCESS)
  {
    retVal = MeshLocalCfgSetPublishAppKeyIndex(pVSetParams->elemId,
                                               (const meshModelId_t *)&mdlId,
                                               pEvt->pubParams.publishAppKeyIndex);
    WSF_ASSERT(retVal == MESH_SUCCESS);

    retVal = MeshLocalCfgSetPublishFriendshipCredFlag(pVSetParams->elemId,
                                                      (const meshModelId_t *)&mdlId,
                                                      pEvt->pubParams.publishFriendshipCred);
    WSF_ASSERT(retVal == MESH_SUCCESS);

    retVal = MeshLocalCfgSetPublishPeriod(pVSetParams->elemId, (const meshModelId_t *)&mdlId,
                                          pEvt->pubParams.publishPeriodNumSteps,
                                          pEvt->pubParams.publishPeriodStepRes);
    WSF_ASSERT(retVal == MESH_SUCCESS);

    retVal = MeshLocalCfgSetPublishRetransCount(pVSetParams->elemId, (const meshModelId_t *)&mdlId,
                                                pEvt->pubParams.publishRetransCount);
    WSF_ASSERT(retVal == MESH_SUCCESS);

    retVal = MeshLocalCfgSetPublishRetransIntvlSteps(pVSetParams->elemId,
                                                     (const meshModelId_t *)&mdlId,
                                                     pEvt->pubParams.publishRetransSteps50Ms);
    WSF_ASSERT(retVal == MESH_SUCCESS);

    retVal = MeshLocalCfgSetPublishTtl(pVSetParams->elemId, (const meshModelId_t *)&mdlId,
                                       pEvt->pubParams.publishTtl);
    WSF_ASSERT(retVal == MESH_SUCCESS);

    /* Notify Access Layer that periodic publishing state has changed.*/
    MeshAccPeriodPubChanged(pVSetParams->elemId, &mdlId);
  }
  else
  {
    switch (retVal)
    {
      case MESH_LOCAL_CFG_OUT_OF_MEMORY:
        /* Set error to insufficient resources. */
        status = MESH_CFG_MDL_ERR_INSUFFICIENT_RESOURCES;
        break;
      default:
        /* Set error to unspecified. */
        status = MESH_CFG_MDL_ERR_UNSPECIFIED;
        break;
    }
  }

  return status;
}

/*************************************************************************************************/
/*!
 *  \brief     CMAC callback for generating virtual addresses for Model Publication Virtual Set.
 *
 *  \param[in] pCmacResult  AES-CMAC result.
 *  \param[in] pParam       Generic parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void cfgMdlSrModelPubLabelUuidCmacCback(const uint8_t *pCmacResult, void *pParam)
{
  uint8_t *ptr;
  cfgMdlSrModelPubVirtSetParams_t *pVSetParams = (cfgMdlSrModelPubVirtSetParams_t *)pParam;
  meshCfgMdlModelPubEvt_t *pEvt = &(pVSetParams->evt);
  uint8_t rspMsgParam[CFG_MDL_MSG_MODEL_PUB_STATUS_MAX_NUM_BYTES];

  /* Check if CMAC failed. */
  if (pCmacResult != NULL)
  {
    /* Move cursor to virtual address location. */
    ptr = (uint8_t *)&pCmacResult[MESH_SEC_TOOL_AES_BLOCK_SIZE - sizeof(meshAddress_t)];

    /* Extract last two bytes. */
    pEvt->pubAddr = ((*ptr) << 8) | (*(ptr + 1));

    /* Clear address type mask. */
    MESH_UTILS_BITMASK_CLR(pEvt->pubAddr, MESH_ADDR_TYPE_GROUP_VIRTUAL_MASK);

    /* Set address type as virtual. */
    pEvt->pubAddr |= (MESH_ADDR_TYPE_VIRTUAL_MSBITS_VALUE << MESH_ADDR_TYPE_SHIFT);

    /* Set all parameters. */
    rspMsgParam[0] = cfgMdlSrModelPubVirtSetStore(pVSetParams, pEvt->pubAddr);
  }
  else
  {
    /* CMAC failed, set unspecified error code. */
    rspMsgParam[0] = MESH_CFG_MDL_ERR_UNSPECIFIED;
  }

  /* Clear publication parameters on error. */
  if (rspMsgParam[0] != MESH_CFG_MDL_SR_SUCCESS)
  {
    memset(&(pEvt->pubParams), 0, sizeof(pEvt->pubParams));
    pEvt->pubAddr = MESH_ADDR_TYPE_UNASSIGNED;
  }

  /* Pack response. */
  ptr = &rspMsgParam[1];

  /* Pack element address. */
  UINT16_TO_BSTREAM(ptr, pEvt->elemAddr);

  /* Pack publish address. */
  UINT16_TO_BSTREAM(ptr, pEvt->pubAddr);

  /* Pack publication parameters. */
  meshCfgMsgPackModelPubParam(ptr, &(pEvt->pubParams), pEvt->modelId.sigModelId,
                              pEvt->modelId.vendorModelId,
                              pEvt->isSig);

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_MODEL_PUB_STATUS, rspMsgParam,
                      CFG_MDL_MSG_MODEL_PUB_STATUS_NUM_BYTES(pEvt->isSig),
                      pEvt->cfgMdlHdr.peerAddress, pVSetParams->recvTtl, pVSetParams->recvNetKeyIndex);

  if (rspMsgParam[0] == MESH_CFG_MDL_SR_SUCCESS)
  {
    /* Set event parameters. */
    pEvt->cfgMdlHdr.hdr.event  = MESH_CFG_MDL_SR_EVENT;
    pEvt->cfgMdlHdr.hdr.param  = MESH_CFG_MDL_PUB_VIRT_SET_EVENT;
    pEvt->cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS;

    /* Call notification callback. */
    meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)pEvt);
  }

  /* Free memory since a response is sent. */
  WsfBufFree(pVSetParams);
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Model Publication Virtual Set request.
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
void meshCfgMdlSrHandleModelPubVirtSet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                       uint8_t ttl, uint16_t netKeyIndex)
{
  uint8_t *ptr;
  cfgMdlSrModelPubVirtSetParams_t *pVSetParams;
  meshCfgMdlModelPubEvt_t *pEvt;
  uint16_t dummyNetKeyIndex;
  uint8_t rspMsgParam[CFG_MDL_MSG_MODEL_PUB_STATUS_MAX_NUM_BYTES];
  meshModelId_t mdlId;
  bool_t addrExists = FALSE;

  /* Validate pointer. */
  if(pMsgParam == NULL)
  {
    return;
  }

  /* Validate length and extract model type. */
  if (msgParamLen != CFG_MDL_MSG_MODEL_PUB_VIRT_SET_NUM_BYTES(TRUE))
  {
    if (msgParamLen != CFG_MDL_MSG_MODEL_PUB_VIRT_SET_NUM_BYTES(FALSE))
    {
      return;
    }
    else
    {
      mdlId.isSigModel = FALSE;
    }
  }
  else
  {
    mdlId.isSigModel = TRUE;
  }

  /* Allocate memory to handle the request. */
  if ((pVSetParams = WsfBufAlloc(sizeof(cfgMdlSrModelPubVirtSetParams_t))) == NULL)
  {
    return;
  }

  /* Get event from request. */
  pEvt = &(pVSetParams->evt);

  /* Set model type. */
  pEvt->isSig = mdlId.isSigModel;

  /* Unpack Model Publication Set parameters. */
  ptr = pMsgParam;

  /* Get element address. */
  BSTREAM_TO_UINT16(pEvt->elemAddr, ptr);

  /* Get Label UUID. */
  memcpy(pVSetParams->labelUuid, ptr, MESH_LABEL_UUID_SIZE);
  ptr += MESH_LABEL_UUID_SIZE;

  /* Unpack publication parameters. */
  meshCfgMsgUnpackModelPubParam(ptr, &(pEvt->pubParams),
                                &(pEvt->modelId.sigModelId),
                                &(pEvt->modelId.vendorModelId), pEvt->isSig);

  /* Validate publication parameters. */
 if(!MESH_TTL_IS_VALID(pEvt->pubParams.publishTtl) ||
    (pEvt->pubParams.publishAppKeyIndex > MESH_APP_KEY_INDEX_MAX_VAL))
  {
    /* Prohibited values. */
    return;
  }

  /* Set model id structure used by Local Config. */
  if (pEvt->isSig)
  {
    mdlId.modelId.sigModelId = pEvt->modelId.sigModelId;
  }
  else
  {
    mdlId.modelId.vendorModelId = pEvt->modelId.vendorModelId;
  }

  /* Check if virtual address already exists. */
  if (MeshLocalCfgGetVirtualAddrFromLabelUuid((const uint8_t *)ptr, &(pEvt->pubAddr)) == MESH_SUCCESS)
  {
    addrExists = TRUE;
  }

  /* Set response status to success. */
  rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;

  /* Get element id. */
  if (MeshLocalCfgGetElementIdFromAddr(pEvt->elemAddr, &(pVSetParams->elemId)) != MESH_SUCCESS)
  {
    /* Set error status. */
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_ADDR;
  }
  /* Check if model instance exists. */
  else if (!MeshLocalCfgModelExists(pVSetParams->elemId, &mdlId))
  {
    /* Set error status. */
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_MODEL;
  }
  /* Check that AppKey exists on device by reading the bound NetKey. */
  else if (MeshLocalCfgGetBoundNetKeyIndex(pEvt->pubParams.publishAppKeyIndex, &dummyNetKeyIndex)
           != MESH_SUCCESS)
  {
    /* AppKeyIndex is invalid. */
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_APPKEY_INDEX;
  }
  else if ((pEvt->pubParams.publishFriendshipCred) && (MeshLocalCfgGetLowPowerState() >= MESH_LOW_POWER_FEATURE_PROHIBITED_START))
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_FEATURE_NOT_SUPPORTED;
  }
  else
  {
    /* If address already exists, store new Model Publication state. */
    if (addrExists)
    {
      /* Store parameters in the node. */
      rspMsgParam[0] = cfgMdlSrModelPubVirtSetStore(pVSetParams, pEvt->pubAddr);
    }
    else
    {
      /* Set fields needed to send the Model Publication Status response. */
      pEvt->cfgMdlHdr.peerAddress = src;
      pVSetParams->recvTtl = ttl;
      pVSetParams->recvNetKeyIndex = netKeyIndex;

      /* Derive virtual address from label UUID. */
      if (MeshSecToolCmacCalculate((uint8_t *)saltVtad, pVSetParams->labelUuid, MESH_LABEL_UUID_SIZE,
                                   cfgMdlSrModelPubLabelUuidCmacCback, pVSetParams)
          == MESH_SUCCESS)
      {
        /* Resume execution after security finishes. */
        return;
      }
      else
      {
        /* No resources to calculate label UUID. */
        rspMsgParam[0] = MESH_CFG_MDL_ERR_INSUFFICIENT_RESOURCES;
      }
    }
  }

  /* Clear publication parameters on error. */
  if (rspMsgParam[0] != MESH_CFG_MDL_SR_SUCCESS)
  {
    memset(&(pEvt->pubParams), 0, sizeof(pEvt->pubParams));
    pEvt->pubAddr = MESH_ADDR_TYPE_UNASSIGNED;
  }

  /* Pack response. */
  ptr = &rspMsgParam[1];

  /* Pack element address. */
  UINT16_TO_BSTREAM(ptr, pEvt->elemAddr);

  /* Pack publish address. */
  UINT16_TO_BSTREAM(ptr, pEvt->pubAddr);

  /* Pack publication parameters. */
  meshCfgMsgPackModelPubParam(ptr, &pEvt->pubParams, pEvt->modelId.sigModelId,
                              pEvt->modelId.vendorModelId, pEvt->isSig);

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_MODEL_PUB_STATUS, rspMsgParam,
                      CFG_MDL_MSG_MODEL_PUB_STATUS_NUM_BYTES(pEvt->isSig), src, ttl, netKeyIndex);

  if (rspMsgParam[0] == MESH_CFG_MDL_SR_SUCCESS)
  {
    /* Set event parameters. */
    pEvt->cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT;
    pEvt->cfgMdlHdr.hdr.param = MESH_CFG_MDL_PUB_VIRT_SET_EVENT;
    pEvt->cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS;

    /* Call notification callback. */
    meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)pEvt);
  }

  /* Free memory since a response is sent. */
  WsfBufFree(pVSetParams);
}

/*************************************************************************************************/
/*!
 *  \brief     CMAC callback for generating virtual addresses for Model Subscription Virtual
 *             Add/Overwrite.
 *
 *  \param[in] pCmacResult  AES-CMAC result.
 *  \param[in] pParam       Generic parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void cfgMdlSrSubscrCmacCback(const uint8_t *pCmacResult, void *pParam)
{
  uint8_t *ptr;
  meshModelId_t mdlId;
  cfgMdlSrModelSubscrVirtAddOvrParams_t *pVAddOvr = (cfgMdlSrModelSubscrVirtAddOvrParams_t *)pParam;
  meshCfgMdlModelSubscrChgEvt_t *pEvt = &(pVAddOvr->evt);
  uint8_t rspMsgParam[CFG_MDL_MSG_MODEL_SUBSCR_STATUS_MAX_NUM_BYTES];

  /* Check if CMAC failed. */
  if (pCmacResult != NULL)
  {
    /* Move cursor to virtual address location. */
    ptr = (uint8_t *)&pCmacResult[MESH_SEC_TOOL_AES_BLOCK_SIZE - sizeof(meshAddress_t)];

    /* Extract last two bytes. */
    pEvt->subscrAddr = ((*ptr) << 8) | (*(ptr + 1));

    /* Clear address type mask. */
    MESH_UTILS_BITMASK_CLR(pEvt->subscrAddr, MESH_ADDR_TYPE_GROUP_VIRTUAL_MASK);

    /* Set address type as virtual. */
    pEvt->subscrAddr |= (MESH_ADDR_TYPE_VIRTUAL_MSBITS_VALUE << MESH_ADDR_TYPE_SHIFT);

    /* Set Local Config structure for accesing model information. */
    mdlId.isSigModel = pEvt->isSig;
    if (pEvt->isSig)
    {
      mdlId.modelId.sigModelId = pEvt->modelId.sigModelId;
    }
    else
    {
      mdlId.modelId.vendorModelId = pEvt->modelId.vendorModelId;
    }

    /* If overwrite clear the list first. */
    if (pVAddOvr->overwrite)
    {
      /* Clear subscription list. */
      (void)MeshLocalCfgRemoveAllFromSubscrList(pVAddOvr->elemId, (const meshModelId_t *)&mdlId);
    }

    /* Add to subscription list. */

    /* Map internal return values to OTA error codes. */
    switch (MeshLocalCfgAddVirtualAddrToSubscrList(pVAddOvr->elemId, (const meshModelId_t *)&mdlId,
                                                   pVAddOvr->labelUuid,
                                                   pEvt->subscrAddr))
    {
      case MESH_LOCAL_CFG_ALREADY_EXIST:
      case MESH_SUCCESS:
        rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;
        break;
      case MESH_LOCAL_CFG_OUT_OF_MEMORY:
        rspMsgParam[0] = MESH_CFG_MDL_ERR_INSUFFICIENT_RESOURCES;
        break;
      default:
        rspMsgParam[0] = MESH_CFG_MDL_ERR_UNSPECIFIED;
        break;
    }
  }
  else
  {
    /* CMAC failed, set unspecified error code. */
    rspMsgParam[0] = MESH_CFG_MDL_ERR_UNSPECIFIED;
    pEvt->subscrAddr = MESH_ADDR_TYPE_UNASSIGNED;
  }

  /* Pack response. */
  ptr = &rspMsgParam[1];

  /* Pack element address. */
  UINT16_TO_BSTREAM(ptr, pEvt->elemAddr);

  /* Pack subscription address. */
  UINT16_TO_BSTREAM(ptr, pEvt->subscrAddr);

  /* Pack model id. */
  if (pEvt->isSig)
  {
    UINT16_TO_BSTREAM(ptr, pEvt->modelId.sigModelId);
  }
  else
  {
    VEND_MDL_TO_BSTREAM(ptr, pEvt->modelId.vendorModelId);
  }

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_MODEL_SUBSCR_STATUS, rspMsgParam,
                      CFG_MDL_MSG_MODEL_SUBSCR_STATUS_NUM_BYTES(pEvt->isSig), pEvt->cfgMdlHdr.peerAddress,
                      pVAddOvr->recvTtl, pVAddOvr->recvNetKeyIndex);

  if (rspMsgParam[0] == MESH_CFG_MDL_SR_SUCCESS)
  {
    /* Set event parameters. */
    pEvt->cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT;
    pEvt->cfgMdlHdr.hdr.param = pVAddOvr->overwrite ? MESH_CFG_MDL_SUBSCR_VIRT_OVR_EVENT :
                                            MESH_CFG_MDL_SUBSCR_VIRT_ADD_EVENT;
    pEvt->cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS;

    /* Call notification callback. */
    meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)pEvt);
  }

  /* Free memory. */
  WsfBufFree(pVAddOvr);
}

/*************************************************************************************************/
/*!
 *  \brief     Common Handler for the Model Subscription Add/Overwrite request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *  \param[in] overwrite    TRUE if operation is Overwrite, FALSE if it is Add.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshCfgMdlSrHandleModelSubscrAddOvr(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                                uint8_t ttl, uint16_t netKeyIndex, bool_t overwrite)
{
  uint8_t *ptr;
  meshCfgMdlModelSubscrChgEvt_t evt;
  meshModelId_t mdlId;
  meshElementId_t elemId;
  uint8_t totalSubscrListSize;
  uint8_t rspMsgParam[CFG_MDL_MSG_MODEL_SUBSCR_STATUS_MAX_NUM_BYTES];
  meshLocalCfgRetVal_t retVal;

  /* Validate pointer. */
  if (pMsgParam == NULL)
  {
    return;
  }

  if (overwrite)
  {
    /* Validate length and extract model type. */
    if (msgParamLen != CFG_MDL_MSG_MODEL_SUBSCR_OVR_NUM_BYTES(TRUE))
    {
      if (msgParamLen != CFG_MDL_MSG_MODEL_SUBSCR_OVR_NUM_BYTES(FALSE))
      {
        return;
      }
      else
      {
        mdlId.isSigModel = FALSE;
      }
    }
    else
    {
      mdlId.isSigModel = TRUE;
    }
  }
  else
  {
    /* Validate length and extract model type. */
    if (msgParamLen != CFG_MDL_MSG_MODEL_SUBSCR_ADD_NUM_BYTES(TRUE))
    {
      if (msgParamLen != CFG_MDL_MSG_MODEL_SUBSCR_ADD_NUM_BYTES(FALSE))
      {
        return;
      }
      else
      {
        mdlId.isSigModel = FALSE;
      }
    }
    else
    {
      mdlId.isSigModel = TRUE;
    }
  }

  /* Unpack element address. */
  BSTREAM_TO_UINT16(evt.elemAddr, pMsgParam);

  /* Unpack subscription address. */
  BSTREAM_TO_UINT16(evt.subscrAddr, pMsgParam);

  /* Unpack model id. */
  if (mdlId.isSigModel)
  {
    BSTREAM_TO_UINT16(mdlId.modelId.sigModelId, pMsgParam);
  }
  else
  {
    BSTREAM_TO_VEND_MDL(mdlId.modelId.vendorModelId, pMsgParam);
  }

  /* Set status to success. */
  rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;

  /* Validate subscription address. */
  if(MESH_IS_ADDR_UNASSIGNED(evt.subscrAddr) ||
     MESH_IS_ADDR_VIRTUAL(evt.subscrAddr) ||
     (evt.subscrAddr == MESH_ADDR_GROUP_ALL))
  {
    return;
  }

  /* Verify element address. */
  if (MeshLocalCfgGetElementIdFromAddr(evt.elemAddr, &elemId) != MESH_SUCCESS)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_ADDR;
  }
  /* Check if model exists. */
  else if (!MeshLocalCfgModelExists(elemId, (const meshModelId_t *)&mdlId))
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_MODEL;
  }
  /* Check if model allows subscription. */
  else if(MeshLocalCfgGetSubscrListSize(elemId, (const meshModelId_t *)&mdlId, NULL,
                                        &totalSubscrListSize) != MESH_SUCCESS ||
          totalSubscrListSize == 0)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_NOT_SUBSCRIBE_MODEL;
  }
  else
  {
    if (overwrite)
    {
      /* Clear subscription list. */
      MeshLocalCfgRemoveAllFromSubscrList(elemId, (const meshModelId_t *)&mdlId);
    }

    /* Try to add to subscription list. */
    retVal = MeshLocalCfgAddAddressToSubscrList(elemId, &mdlId, evt.subscrAddr);

    switch (retVal)
    {
      case MESH_LOCAL_CFG_ALREADY_EXIST:
      case MESH_SUCCESS:
        break;
      case MESH_LOCAL_CFG_OUT_OF_MEMORY:
        if (!overwrite)
        {
          rspMsgParam[0] = MESH_CFG_MDL_ERR_INSUFFICIENT_RESOURCES;
        }
        else
        {
          /* Subscription list has 0 entries. */
          rspMsgParam[0] = MESH_CFG_MDL_ERR_NOT_SUBSCRIBE_MODEL;
        }
        break;
      default:
        rspMsgParam[0] = MESH_CFG_MDL_ERR_UNSPECIFIED;
        break;
    }
  }

  /* Copy model id. */
  evt.isSig = mdlId.isSigModel;
  if (evt.isSig)
  {
    evt.modelId.sigModelId = mdlId.modelId.sigModelId;
  }
  else
  {
    evt.modelId.vendorModelId = mdlId.modelId.vendorModelId;
  }

  /* Pack response. */
  ptr = &rspMsgParam[1];

  /* Pack element address. */
  UINT16_TO_BSTREAM(ptr, evt.elemAddr);

  /* Pack subscription address. */
  UINT16_TO_BSTREAM(ptr, evt.subscrAddr);

  /* Pack model id. */
  if (evt.isSig)
  {
    UINT16_TO_BSTREAM(ptr, evt.modelId.sigModelId);
  }
  else
  {
    VEND_MDL_TO_BSTREAM(ptr, evt.modelId.vendorModelId);
  }

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_MODEL_SUBSCR_STATUS, rspMsgParam,
                      CFG_MDL_MSG_MODEL_SUBSCR_STATUS_NUM_BYTES(evt.isSig), src, ttl, netKeyIndex);

  if (rspMsgParam[0] == MESH_CFG_MDL_SR_SUCCESS)
  {
    /* Set event parameters. */
    evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT;
    evt.cfgMdlHdr.hdr.param = overwrite ? MESH_CFG_MDL_SUBSCR_OVR_EVENT : MESH_CFG_MDL_SUBSCR_ADD_EVENT;
    evt.cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS;
    evt.cfgMdlHdr.peerAddress = src;

    /* Call notification callback. */
    meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)&evt);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Common handler for the Model Subscription Virtual Add/Overwrite requests.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received.
 *  \param[in] overwrite    TRUE if request is overwrite.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshCfgMdlSrHandleModelSubscrVirtAddOvr(uint8_t *pMsgParam, uint16_t msgParamLen,
                                                    meshAddress_t src, uint8_t ttl,
                                                    uint16_t netKeyIndex, bool_t overwrite)
{
  cfgMdlSrModelSubscrVirtAddOvrParams_t *pVAddOvr;
  uint8_t *pTemp;
  meshCfgMdlModelSubscrChgEvt_t evt;
  meshModelId_t mdlId;
  meshElementId_t elemId;
  uint8_t totalSubscrListSize;
  uint8_t rspMsgParam[CFG_MDL_MSG_MODEL_SUBSCR_STATUS_MAX_NUM_BYTES];
  meshLocalCfgRetVal_t retVal;

  /* Validate pointer. */
  if (pMsgParam == NULL)
  {
    return;
  }

  /* Initialize zero values used in error responses. */
  evt.subscrAddr = MESH_ADDR_TYPE_UNASSIGNED;

  /* Validate length and extract model type. */
  if (overwrite)
  {
    if (msgParamLen != CFG_MDL_MSG_MODEL_SUBSCR_VIRT_OVR_NUM_BYTES(TRUE))
    {
      if (msgParamLen != CFG_MDL_MSG_MODEL_SUBSCR_VIRT_OVR_NUM_BYTES(FALSE))
      {
        return;
      }
      else
      {
        mdlId.isSigModel = FALSE;
      }
    }
    else
    {
      mdlId.isSigModel = TRUE;
    }
  }
  else
  {
    if (msgParamLen != CFG_MDL_MSG_MODEL_SUBSCR_VIRT_ADD_NUM_BYTES(TRUE))
    {
      if (msgParamLen != CFG_MDL_MSG_MODEL_SUBSCR_VIRT_ADD_NUM_BYTES(FALSE))
      {
        return;
      }
      else
      {
        mdlId.isSigModel = FALSE;
      }
    }
    else
    {
      mdlId.isSigModel = TRUE;
    }

  }

  /* Unpack element address. */
  BSTREAM_TO_UINT16(evt.elemAddr, pMsgParam);

  /* Point to Label UUID. */
  pTemp = pMsgParam;
  pMsgParam += MESH_LABEL_UUID_SIZE;

  /* Unpack model id. */
  if (mdlId.isSigModel)
  {
    BSTREAM_TO_UINT16(mdlId.modelId.sigModelId, pMsgParam);
  }
  else
  {
    BSTREAM_TO_VEND_MDL(mdlId.modelId.vendorModelId, pMsgParam);
  }

  /* Copy model id. */
  evt.isSig = mdlId.isSigModel;
  if (evt.isSig)
  {
    evt.modelId.sigModelId = mdlId.modelId.sigModelId;
  }
  else
  {
    evt.modelId.vendorModelId = mdlId.modelId.vendorModelId;
  }

  /* Set status to success. */
  rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;

  /* Verify element address. */
  if (MeshLocalCfgGetElementIdFromAddr(evt.elemAddr, &elemId) != MESH_SUCCESS)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_ADDR;
  }
  /* Check if model exists. */
  else if (!MeshLocalCfgModelExists(elemId, (const meshModelId_t *)&mdlId))
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_MODEL;
  }
  /* Check if model allows subscription. */
  else if(MeshLocalCfgGetSubscrListSize(elemId, (const meshModelId_t *)&mdlId, NULL,
                                        &totalSubscrListSize) != MESH_SUCCESS ||
          totalSubscrListSize == 0)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_NOT_SUBSCRIBE_MODEL;
  }
  else
  {
    /* Read Virtual Address for the Label UUID to check if it already exists. */
    if(MeshLocalCfgGetVirtualAddrFromLabelUuid((const uint8_t *)pTemp, &evt.subscrAddr)
       == MESH_SUCCESS)
    {
      /* If overwrite clear the list first. */
      if (overwrite)
      {
        /* Clear subscription list. */
       (void)MeshLocalCfgRemoveAllFromSubscrList(elemId, (const meshModelId_t *)&mdlId);
      }

      /* Add to subscription list. */
      retVal = MeshLocalCfgAddVirtualAddrToSubscrList(elemId, (const meshModelId_t *)&mdlId,
                                                      (const uint8_t *)pTemp, evt.subscrAddr);
      /* Map internal return values to OTA error codes. */
      switch (retVal)
      {
        case MESH_LOCAL_CFG_ALREADY_EXIST:
        case MESH_SUCCESS:
          break;
        case MESH_LOCAL_CFG_OUT_OF_MEMORY:
          rspMsgParam[0] = MESH_CFG_MDL_ERR_INSUFFICIENT_RESOURCES;
          break;
        default:
          rspMsgParam[0] = MESH_CFG_MDL_ERR_UNSPECIFIED;
          break;
      }
    }
    else
    {
      /* Allocate memory to process the request. */
      if ((pVAddOvr = WsfBufAlloc(sizeof(cfgMdlSrModelSubscrVirtAddOvrParams_t))) == NULL)
      {
        /* No resources to complete request. */
        return;
      }
      else
      {
        /* Copy event. */
        pVAddOvr->evt = evt;

        /* Copy parameters needed for the response. */
        pVAddOvr->evt.cfgMdlHdr.peerAddress = src;
        pVAddOvr->elemId = elemId;
        pVAddOvr->recvNetKeyIndex = netKeyIndex;
        pVAddOvr->recvTtl = ttl;
        /* Copy Label UUID. */
        memcpy(pVAddOvr->labelUuid, pTemp, MESH_LABEL_UUID_SIZE);
        /* Set overwrite flag. */
        pVAddOvr->overwrite = overwrite;

        /* Call toolbox to derive virtual address from Label UUID. */
        if (MeshSecToolCmacCalculate((uint8_t *)saltVtad, pVAddOvr->labelUuid, MESH_LABEL_UUID_SIZE,
                                     cfgMdlSrSubscrCmacCback, (void *)pVAddOvr)
            != MESH_SUCCESS)
        {
          rspMsgParam[0] = MESH_CFG_MDL_ERR_UNSPECIFIED;
          /* Free memory. */
          WsfBufFree(pVAddOvr);
        }
        else
        {
          /* Resume execution after virtual address computation. */
          return;
        }
      }
    }
  }

  /* Pack response. */
  pTemp = &rspMsgParam[1];

  /* Pack element address. */
  UINT16_TO_BSTREAM(pTemp, evt.elemAddr);

  /* Pack subscription address. */
  UINT16_TO_BSTREAM(pTemp, evt.subscrAddr);

  /* Pack model id. */
  if (evt.isSig)
  {
    UINT16_TO_BSTREAM(pTemp, evt.modelId.sigModelId);
  }
  else
  {
    VEND_MDL_TO_BSTREAM(pTemp, evt.modelId.vendorModelId);
  }

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_MODEL_SUBSCR_STATUS, rspMsgParam,
                      CFG_MDL_MSG_MODEL_SUBSCR_STATUS_NUM_BYTES(evt.isSig), src, ttl, netKeyIndex);

  if (rspMsgParam[0] == MESH_CFG_MDL_SR_SUCCESS)
  {
    /* Set event parameters. */
    evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT;
    evt.cfgMdlHdr.hdr.param = overwrite ? MESH_CFG_MDL_SUBSCR_VIRT_OVR_EVENT :
                                MESH_CFG_MDL_SUBSCR_VIRT_ADD_EVENT;
    evt.cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS;
    evt.cfgMdlHdr.peerAddress = src;

    /* Call notification callback. */
    meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)&evt);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Model Subscription Add request.
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
void meshCfgMdlSrHandleModelSubscrAdd(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                      uint8_t ttl, uint16_t netKeyIndex)
{
  /* Call common handler. */
  meshCfgMdlSrHandleModelSubscrAddOvr(pMsgParam, msgParamLen, src, ttl, netKeyIndex, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Model Subscription Virtual Address Add request.
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
void meshCfgMdlSrHandleModelSubscrVirtAdd(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                          uint8_t ttl, uint16_t netKeyIndex)
{
  /* Call common handler. */
  meshCfgMdlSrHandleModelSubscrVirtAddOvr(pMsgParam, msgParamLen, src, ttl, netKeyIndex, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Model Subscription Delete request.
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
void meshCfgMdlSrHandleModelSubscrDel(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                      uint8_t ttl, uint16_t netKeyIndex)
{
  uint8_t *ptr;
  meshCfgMdlModelSubscrChgEvt_t evt;
  meshModelId_t mdlId;
  meshElementId_t elemId;
  uint8_t totalSubscrListSize;
  uint8_t rspMsgParam[CFG_MDL_MSG_MODEL_SUBSCR_STATUS_MAX_NUM_BYTES];

  /* Validate pointer. */
  if (pMsgParam == NULL)
  {
    return;
  }

  /* Validate length and extract model type. */
  if (msgParamLen != CFG_MDL_MSG_MODEL_SUBSCR_DEL_NUM_BYTES(TRUE))
  {
    if (msgParamLen != CFG_MDL_MSG_MODEL_SUBSCR_DEL_NUM_BYTES(FALSE))
    {
      return;
    }
    else
    {
      mdlId.isSigModel = FALSE;
    }
  }
  else
  {
    mdlId.isSigModel = TRUE;
  }

  /* Unpack element address. */
  BSTREAM_TO_UINT16(evt.elemAddr, pMsgParam);

  /* Unpack subscription address. */
  BSTREAM_TO_UINT16(evt.subscrAddr, pMsgParam);

  /* Unpack model id. */
  if (mdlId.isSigModel)
  {
    BSTREAM_TO_UINT16(mdlId.modelId.sigModelId, pMsgParam);
  }
  else
  {
    BSTREAM_TO_VEND_MDL(mdlId.modelId.vendorModelId, pMsgParam);
  }

  /* Set status to success. */
  rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;

  /* Validate subscription address. */
  if(MESH_IS_ADDR_UNASSIGNED(evt.subscrAddr) ||
     MESH_IS_ADDR_VIRTUAL(evt.subscrAddr) ||
     (evt.subscrAddr == MESH_ADDR_GROUP_ALL))
  {
    return;
  }

  /* Verify element address. */
  if (MeshLocalCfgGetElementIdFromAddr(evt.elemAddr, &elemId) != MESH_SUCCESS)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_ADDR;
  }
  /* Check if model exists. */
  else if (!MeshLocalCfgModelExists(elemId, (const meshModelId_t *)&mdlId))
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_MODEL;
  }
  /* Check if model allows subscription. */
  else if(MeshLocalCfgGetSubscrListSize(elemId, (const meshModelId_t *)&mdlId, NULL,
                                        &totalSubscrListSize) != MESH_SUCCESS ||
          totalSubscrListSize == 0)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_NOT_SUBSCRIBE_MODEL;
  }
  else
  {
    /* Try to remove from subscription list. */
    (void)MeshLocalCfgRemoveAddressFromSubscrList(elemId, (const meshModelId_t *)&mdlId,
                                                  evt.subscrAddr);
  }

  /* Copy model id. */
  evt.isSig = mdlId.isSigModel;
  if (evt.isSig)
  {
    evt.modelId.sigModelId = mdlId.modelId.sigModelId;
  }
  else
  {
    evt.modelId.vendorModelId = mdlId.modelId.vendorModelId;
  }

  /* Pack response. */
  ptr = &rspMsgParam[1];

  /* Pack element address. */
  UINT16_TO_BSTREAM(ptr, evt.elemAddr);

  /* Pack subscription address. */
  UINT16_TO_BSTREAM(ptr, evt.subscrAddr);

  /* Pack model id. */
  if (evt.isSig)
  {
    UINT16_TO_BSTREAM(ptr, evt.modelId.sigModelId);
  }
  else
  {
    VEND_MDL_TO_BSTREAM(ptr, evt.modelId.vendorModelId);
  }

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_MODEL_SUBSCR_STATUS, rspMsgParam,
                      CFG_MDL_MSG_MODEL_SUBSCR_STATUS_NUM_BYTES(evt.isSig), src, ttl, netKeyIndex);

  if (rspMsgParam[0] == MESH_CFG_MDL_SR_SUCCESS)
  {
    /* Set event parameters. */
    evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT;
    evt.cfgMdlHdr.hdr.param = MESH_CFG_MDL_SUBSCR_DEL_EVENT;
    evt.cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS;
    evt.cfgMdlHdr.peerAddress = src;

    /* Call notification callback. */
    meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)&evt);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Model Subscription Virtual Address Delete request.
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
void meshCfgMdlSrHandleModelSubscrVirtDel(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                          uint8_t ttl, uint16_t netKeyIndex)
{
  uint8_t *ptr;
  uint8_t *pLabelUuid;
  meshCfgMdlModelSubscrChgEvt_t evt;
  meshModelId_t mdlId;
  meshElementId_t elemId;
  uint8_t totalSubscrListSize;
  uint8_t rspMsgParam[CFG_MDL_MSG_MODEL_SUBSCR_STATUS_MAX_NUM_BYTES];

  /* Validate pointer. */
  if (pMsgParam == NULL)
  {
    return;
  }

  /* Validate length and extract model type. */
  if (msgParamLen != CFG_MDL_MSG_MODEL_SUBSCR_VIRT_DEL_NUM_BYTES(TRUE))
  {
    if (msgParamLen != CFG_MDL_MSG_MODEL_SUBSCR_VIRT_DEL_NUM_BYTES(FALSE))
    {
      return;
    }
    else
    {
      mdlId.isSigModel = FALSE;
    }
  }
  else
  {
    mdlId.isSigModel = TRUE;
  }
  /* Set subscription address to unassigned initially. */
  evt.subscrAddr = MESH_ADDR_TYPE_UNASSIGNED;

  /* Unpack element address. */
  BSTREAM_TO_UINT16(evt.elemAddr, pMsgParam);

  /* Point to Label UUID. */
  pLabelUuid = pMsgParam;
  /* Advance pointer. */
  pMsgParam += MESH_LABEL_UUID_SIZE;

  /* Unpack model id. */
  if (mdlId.isSigModel)
  {
    BSTREAM_TO_UINT16(mdlId.modelId.sigModelId, pMsgParam);
  }
  else
  {
    BSTREAM_TO_VEND_MDL(mdlId.modelId.vendorModelId, pMsgParam);
  }

  /* Set status to success. */
  rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;

  /* Verify element address. */
  if (MeshLocalCfgGetElementIdFromAddr(evt.elemAddr, &elemId) != MESH_SUCCESS)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_ADDR;
  }
  /* Check if model exists. */
  else if (!MeshLocalCfgModelExists(elemId, (const meshModelId_t *)&mdlId))
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_MODEL;
  }
  /* Check if model allows subscription. */
  else if(MeshLocalCfgGetSubscrListSize(elemId, (const meshModelId_t *)&mdlId, NULL,
                                        &totalSubscrListSize) != MESH_SUCCESS ||
          totalSubscrListSize == 0)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_NOT_SUBSCRIBE_MODEL;
  }
  /* Check if Label UUID exists. */
  else if(MeshLocalCfgGetVirtualAddrFromLabelUuid((const uint8_t *)pLabelUuid, &evt.subscrAddr) !=
          MESH_SUCCESS)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_ADDR;
    evt.subscrAddr = MESH_ADDR_TYPE_UNASSIGNED;
  }
  else
  {
    /* Try to remove from subscription list. */
    (void)MeshLocalCfgRemoveVirtualAddrFromSubscrList(elemId, (const meshModelId_t *)&mdlId,
                                                      (const uint8_t *)pLabelUuid);
  }

  /* Copy model id. */
  evt.isSig = mdlId.isSigModel;
  if (evt.isSig)
  {
    evt.modelId.sigModelId = mdlId.modelId.sigModelId;
  }
  else
  {
    evt.modelId.vendorModelId = mdlId.modelId.vendorModelId;
  }

  /* Pack response. */
  ptr = &rspMsgParam[1];

  /* Pack element address. */
  UINT16_TO_BSTREAM(ptr, evt.elemAddr);

  /* Pack subscription address. */
  UINT16_TO_BSTREAM(ptr, evt.subscrAddr);

  /* Pack model id. */
  if (evt.isSig)
  {
    UINT16_TO_BSTREAM(ptr, evt.modelId.sigModelId);
  }
  else
  {
    VEND_MDL_TO_BSTREAM(ptr, evt.modelId.vendorModelId);
  }

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_MODEL_SUBSCR_STATUS, rspMsgParam,
                      CFG_MDL_MSG_MODEL_SUBSCR_STATUS_NUM_BYTES(evt.isSig), src, ttl, netKeyIndex);

  if (rspMsgParam[0] == MESH_CFG_MDL_SR_SUCCESS)
  {
    /* Set event parameters. */
    evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT;
    evt.cfgMdlHdr.hdr.param = MESH_CFG_MDL_SUBSCR_VIRT_DEL_EVENT;
    evt.cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS;
    evt.cfgMdlHdr.peerAddress = src;

    /* Call notification callback. */
    meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)&evt);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Model Subscription Overwrite request.
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
void meshCfgMdlSrHandleModelSubscrOvr(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                      uint8_t ttl, uint16_t netKeyIndex)
{
  /* Call common handler. */
  meshCfgMdlSrHandleModelSubscrAddOvr(pMsgParam, msgParamLen, src, ttl, netKeyIndex, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Model Subscription Virtual Address Overwrite request.
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
void meshCfgMdlSrHandleModelSubscrVirtOvr(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                          uint8_t ttl, uint16_t netKeyIndex)
{
  /* Call common handler. */
  meshCfgMdlSrHandleModelSubscrVirtAddOvr(pMsgParam, msgParamLen, src, ttl, netKeyIndex, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Model Subscription Delete All request.
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
void meshCfgMdlSrHandleModelSubscrDelAll(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                         uint8_t ttl, uint16_t netKeyIndex)
{
  uint8_t *ptr;
  meshCfgMdlModelSubscrChgEvt_t evt;
  meshModelId_t mdlId;
  meshElementId_t elemId;
  uint8_t totalSubscrListSize;
  uint8_t rspMsgParam[CFG_MDL_MSG_MODEL_SUBSCR_STATUS_MAX_NUM_BYTES];

  /* Validate pointer. */
  if (pMsgParam == NULL)
  {
    return;
  }

  /* Set subscription address to unassigned. */
  evt.subscrAddr = MESH_ADDR_TYPE_UNASSIGNED;

  /* Validate length and extract model type. */
  if (msgParamLen != CFG_MDL_MSG_MODEL_SUBSCR_DEL_ALL_NUM_BYTES(TRUE))
  {
    if (msgParamLen != CFG_MDL_MSG_MODEL_SUBSCR_DEL_ALL_NUM_BYTES(FALSE))
    {
      return;
    }
    else
    {
      mdlId.isSigModel = FALSE;
    }
  }
  else
  {
    mdlId.isSigModel = TRUE;
  }

  /* Unpack element address. */
  BSTREAM_TO_UINT16(evt.elemAddr, pMsgParam);

  /* Unpack model id. */
  if (mdlId.isSigModel)
  {
    BSTREAM_TO_UINT16(mdlId.modelId.sigModelId, pMsgParam);
  }
  else
  {
    BSTREAM_TO_VEND_MDL(mdlId.modelId.vendorModelId, pMsgParam);
  }

  /* Set status to success. */
  rspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;

  /* Verify element address. */
  if (MeshLocalCfgGetElementIdFromAddr(evt.elemAddr, &elemId) != MESH_SUCCESS)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_ADDR;
  }
  /* Check if model exists. */
  else if (!MeshLocalCfgModelExists(elemId, (const meshModelId_t *)&mdlId))
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_MODEL;
  }
  /* Check if model allows subscription. */
  else if(MeshLocalCfgGetSubscrListSize(elemId, (const meshModelId_t *)&mdlId, NULL,
                                        &totalSubscrListSize) != MESH_SUCCESS ||
          totalSubscrListSize == 0)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_NOT_SUBSCRIBE_MODEL;
  }
  else
  {
    /* Clear subscription list. */
    MeshLocalCfgRemoveAllFromSubscrList(elemId, (const meshModelId_t *)&mdlId);
  }

  /* Copy model id. */
  evt.isSig = mdlId.isSigModel;
  if (evt.isSig)
  {
    evt.modelId.sigModelId = mdlId.modelId.sigModelId;
  }
  else
  {
    evt.modelId.vendorModelId = mdlId.modelId.vendorModelId;
  }

  /* Pack response. */
  ptr = &rspMsgParam[1];

  /* Pack element address. */
  UINT16_TO_BSTREAM(ptr, evt.elemAddr);

  /* Pack subscription address. */
  UINT16_TO_BSTREAM(ptr, evt.subscrAddr);

  /* Pack model id. */
  if (evt.isSig)
  {
    UINT16_TO_BSTREAM(ptr, evt.modelId.sigModelId);
  }
  else
  {
    VEND_MDL_TO_BSTREAM(ptr, evt.modelId.vendorModelId);
  }

  /* Send response. */
  meshCfgMdlSrSendRsp(MESH_CFG_MDL_SR_MODEL_SUBSCR_STATUS, rspMsgParam,
                      CFG_MDL_MSG_MODEL_SUBSCR_STATUS_NUM_BYTES(evt.isSig), src, ttl, netKeyIndex);

  if (rspMsgParam[0] == MESH_CFG_MDL_SR_SUCCESS)
  {
    /* Set event parameters. */
    evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_SR_EVENT;
    evt.cfgMdlHdr.hdr.param = MESH_CFG_MDL_SUBSCR_DEL_ALL_EVENT;
    evt.cfgMdlHdr.hdr.status = MESH_CFG_MDL_SR_SUCCESS;
    evt.cfgMdlHdr.peerAddress = src;

    /* Call notification callback. */
    meshCfgMdlSrCb.cback((meshCfgMdlSrEvt_t *)&evt);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Common handler for the SIG/Vendor Model Subscription Get request.
 *
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *  \param[in] isSig        TRUE if request is SIG Model Subscription Get, FALSE otherwise.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshCfgMdlSrHandleModelSubscrGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                             uint8_t ttl, uint16_t netKeyIndex, bool_t isSig)
{
  uint8_t *ptr, *pRspMsgParam;
  meshAddress_t elemAddr, subscrAddr;
  uint16_t rspMsgParamLen = 0;
  meshModelId_t mdlId;
  meshElementId_t elemId;
  uint8_t rspMsgParam[CFG_MDL_MSG_MODEL_SUBSCR_LIST_EMPTY_MAX_NUM_BYTES];
  uint8_t numAddr = 0, totalNumAddr = 0, idx = 0;

  /* Unpack element address. */
  BSTREAM_TO_UINT16(elemAddr, pMsgParam);

  /* Unpack model id. */
  if (isSig)
  {
    mdlId.isSigModel = TRUE;
    BSTREAM_TO_UINT16(mdlId.modelId.sigModelId, pMsgParam);
  }
  else
  {
    mdlId.isSigModel = FALSE;
    BSTREAM_TO_VEND_MDL(mdlId.modelId.vendorModelId, pMsgParam);
  }

  /* Set response message to empty list at first. */
  rspMsgParamLen = CFG_MDL_MSG_MODEL_SUBSCR_LIST_EMPTY_NUM_BYTES(isSig);
  pRspMsgParam = rspMsgParam;

  /* Get element id from address. */
  if (MeshLocalCfgGetElementIdFromAddr(elemAddr, &elemId) != MESH_SUCCESS)
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_ADDR;
  }
  /* Check if model exists. */
  else if (!MeshLocalCfgModelExists(elemId, (const meshModelId_t *)&mdlId))
  {
    rspMsgParam[0] = MESH_CFG_MDL_ERR_INVALID_MODEL;
  }
  else
  {
    /* Read number of addresses. */
    (void)MeshLocalCfgGetSubscrListSize(elemId, (const meshModelId_t *)&mdlId, &numAddr,
                                        &totalNumAddr);

    /* Check if this is a subscribe model. */
    if (totalNumAddr == 0)
    {
      rspMsgParam[0] = MESH_CFG_MDL_ERR_NOT_SUBSCRIBE_MODEL;
    }
    else
    {
      /* Make sure addresses don't exceed Access PDU. */
      if (numAddr > CFG_MDL_MSG_MODEL_SUBSCR_LIST_MAX_NUM_ADDR(isSig))
      {
        numAddr = CFG_MDL_MSG_MODEL_SUBSCR_LIST_MAX_NUM_ADDR(isSig);
      }

      /* Allocate memory. */
      if((pRspMsgParam = WsfBufAlloc(CFG_MDL_MSG_MODEL_SUBSCR_LIST_EMPTY_NUM_BYTES(isSig) +
                                     (numAddr * sizeof(meshAddress_t)))) == NULL)
      {
        /* No resources to complete request. */
        return;
      }
      else
      {
        /* Set success. */
        pRspMsgParam[0] = MESH_CFG_MDL_SR_SUCCESS;

        /* Calculate message length. */
        rspMsgParamLen = CFG_MDL_MSG_MODEL_SUBSCR_LIST_EMPTY_NUM_BYTES(isSig) +
                         (numAddr * sizeof(meshAddress_t));

        /* Start populating with addresses. */
        ptr = &pRspMsgParam[CFG_MDL_MSG_MODEL_SUBSCR_LIST_EMPTY_NUM_BYTES(isSig)];
        /* Iterate with indexer through subscription list. */
        while (MeshLocalCfgGetNextAddressFromSubscrList(elemId, (const meshModelId_t *)&mdlId,
                                                        &subscrAddr, &idx)
               == MESH_SUCCESS && numAddr != 0)
        {
          UINT16_TO_BSTREAM(ptr, subscrAddr);
          numAddr--;
        }

        WSF_ASSERT(numAddr == 0);
      }
    }
  }

  /* Pack element address and model id. */
  ptr = &pRspMsgParam[1];
  UINT16_TO_BSTREAM(ptr, elemAddr);
  if (isSig)
  {
    UINT16_TO_BSTREAM(ptr, mdlId.modelId.sigModelId);
  }
  else
  {
    VEND_MDL_TO_BSTREAM(ptr, mdlId.modelId.vendorModelId);
  }

  /* Send response. */
  meshCfgMdlSrSendRsp(isSig ? MESH_CFG_MDL_SR_MODEL_SUBSCR_SIG_LIST : MESH_CFG_MDL_SR_MODEL_SUBSCR_VENDOR_LIST,
                      pRspMsgParam, rspMsgParamLen, src, ttl, netKeyIndex);

  /* Check if message was allocated and free it. */
  if (pRspMsgParam != &rspMsgParam[0])
  {
    WsfBufFree(pRspMsgParam);
  }
  (void)msgParamLen;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the SIG Model Subscription Get request.
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
void meshCfgMdlSrHandleModelSubscrSigGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                         uint8_t ttl, uint16_t netKeyIndex)
{
  /* Validate message parameters. */
  if((pMsgParam == NULL) ||
     (msgParamLen != CFG_MDL_MSG_MODEL_SUBSCR_SIG_GET_NUM_BYTES))
  {
    return;
  }

  /* Call common handler. */
  meshCfgMdlSrHandleModelSubscrGet(pMsgParam, msgParamLen, src, ttl, netKeyIndex, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Vendor Model Subscription Get request.
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
void meshCfgMdlSrHandleModelSubscrVendorGet(uint8_t *pMsgParam, uint16_t msgParamLen, meshAddress_t src,
                                            uint8_t ttl, uint16_t netKeyIndex)
{
  /* Validate message parameters. */
  if((pMsgParam == NULL) ||
     (msgParamLen != CFG_MDL_MSG_MODEL_SUBSCR_VENDOR_GET_NUM_BYTES))
  {
    return;
  }

  /* Call common handler. */
  meshCfgMdlSrHandleModelSubscrGet(pMsgParam, msgParamLen, src, ttl, netKeyIndex, FALSE);
}
