/*************************************************************************************************/
/*!
 *  \file   mesh_access_main.c
 *
 *  \brief  Access module implementation.
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
#include "wsf_buf.h"
#include "wsf_cs.h"
#include "wsf_msg.h"
#include "wsf_os.h"
#include "wsf_assert.h"
#include "wsf_trace.h"
#include "sec_api.h"

#include "mesh_defs.h"
#include "mesh_error_codes.h"
#include "mesh_api.h"
#include "mesh_main.h"

#include "mesh_seq_manager.h"
#include "mesh_local_config_types.h"
#include "mesh_local_config.h"
#include "mesh_lower_transport.h"
#include "mesh_upper_transport.h"
#include "mesh_utils.h"
#include "mesh_security.h"
#include "mesh_cfg_mdl.h"
#include "mesh_cfg_mdl_sr_api.h"
#include "mesh_cfg_mdl_cl_api.h"
#include "mesh_access.h"
#include "mesh_access_main.h"
#include "mesh_lpn_api.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Publish retransmit steps to time in milliseconds */
#define RETRANS_STEPS_TO_MS_TIME(steps) ((uint16_t)((steps) + 1) * 50)

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Format for storing Access Layer PDU and information used on publishing when retransmit count
 *  is not zero
 */
typedef struct meshAccPduPubTxInfo_tag
{
  void                            *pNext;                   /*!< Pointer to next element to make
                                                             *   this structure enqueable
                                                             */
  meshUtrAccPduTxInfo_t           utrAccPduTxInfo;          /*!< Upper Transport Access PDU and
                                                             *   additional information
                                                             */
  meshPublishRetransCount_t       publishRetransCount;      /*!< Publish retransmit count */
  meshPublishRetransIntvlSteps_t  publishRetransSteps50Ms;  /*!< Publish 50 ms retransmit steps */
  wsfTimer_t                      retransTmr;               /*!< Retransmission timer */
} meshAccPduPubTxInfo_t;

/*! Structure storing Access Layer message information required by models */
typedef struct meshAccToMdlMsgInfo_tag
{
  const uint8_t   *pDstLabelUuid;  /*!< Pointer to label UUID for virtual destination address */
  uint8_t         *pMsgParam;      /*!< Pointer to messsage parameters */
  uint16_t        msgParamLen;     /*!< Length of the message parameters */
  meshMsgOpcode_t opcode;          /*!< Message opcode */
  uint8_t         ttl;             /*!< Message TTL */
  meshAddress_t   src;             /*!< Source address */
  meshAddress_t   dst;             /*!< Destination address */
  uint16_t        appKeyIndex;     /*!< Global Application Key identifier */
  uint16_t        netKeyIndex;     /*!< Global Network Key identifier */
  bool_t          recvOnUnicast;   /*!< Indicates if initial destination address was unicast */
} meshAccToMdlMsgInfo_t;

/**************************************************************************************************
  Function Prototypes
**************************************************************************************************/

static void meshAccCancelPendingRetrans(meshAddress_t src, const uint8_t *pAccPduOpcode, 
                                        uint8_t accPduOpcodeLen);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Mesh Access control block */
meshAccCb_t meshAccCb = { 0 };

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Allocates an unique identifier for each Send Message API with random delay
 *          non-zero.
 *
 *  \return Unique identifier.
 */
/*************************************************************************************************/
static uint16_t meshAccSendMsgTmrIdAlloc(void)
{
  static uint16_t uid = 0;
  uint16_t tempUid = 0;

  WsfTaskLock();
  tempUid = uid++;
  WsfTaskUnlock();

  return tempUid;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Stack empty event handler.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshEmptyAccPpMsgHandler(wsfMsgHdr_t *pMsg)
{
  (void)pMsg;
  return;
}

/*************************************************************************************************/
/*!
 *  \brief     Empty implementation for periodic publishing notification.
 *
 *  \param[in] elemId    Element identifier.
 *  \param[in] pModelId  Pointer to a model identifier.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshAccEmptyPpChangedCback(meshElementId_t elemId, meshModelId_t *pModelId)
{
  (void)elemId;
  (void)pModelId;
  MESH_TRACE_WARN0("MESH ACC: Optional feature not initialized. ");
}

/*************************************************************************************************/
/*!
 *  \brief     Empty implementation for getting the Friend address for a subnet.
 *
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet.
 *
 *  \return    MESH_ADDR_TYPE_UNASSIGNED.
 */
/*************************************************************************************************/
static meshAddress_t meshAccEmptyFriendAddrFromSubnetCback(uint16_t netKeyIndex)
{
  (void)netKeyIndex;

  return MESH_ADDR_TYPE_UNASSIGNED;
}

/*************************************************************************************************/
/*!
 *  \brief     Allocate, build and send a WSF message received event to a model instance.
 *
 *  \param[in] pAccToMdlMsgInfo  Pointer to structure identifying message and information to be sent
 *                               to Mesh models.
 *  \param[in] elemId            Identifier of the element containing the model.
 *  \param[in] pHandlerId        Pointer to the Mesh model WSF handler ID.
 *  \param[in] pModelId          Pointer to generic model identifier structure.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshAccSendWsfMsgRecvEvt(meshAccToMdlMsgInfo_t *pAccToMdlMsgInfo,
                                     meshElementId_t elemId, wsfHandlerId_t *pHandlerId,
                                     meshModelId_t *pModelId)
{
  meshModelMsgRecvEvt_t *pEvt;

  /* Allocate WSF message. */
  if((pEvt = (meshModelMsgRecvEvt_t *)WsfMsgAlloc(sizeof(meshModelMsgRecvEvt_t) +
                                                  pAccToMdlMsgInfo->msgParamLen))
      != NULL)
  {
    /* Configure event type. */
    pEvt->hdr.event = MESH_MODEL_EVT_MSG_RECV;

    /* Configure event parameters. */
    pEvt->elementId   = elemId;
    pEvt->srcAddr     = pAccToMdlMsgInfo->src;
    pEvt->ttl         = pAccToMdlMsgInfo->ttl;
    pEvt->appKeyIndex = pAccToMdlMsgInfo->appKeyIndex;

    /* Set opcode. */
    pEvt->opCode.opcodeBytes[0] = pAccToMdlMsgInfo->opcode.opcodeBytes[0];
    if (MESH_OPCODE_SIZE(pEvt->opCode) > 1)
    {
      pEvt->opCode.opcodeBytes[1] = pAccToMdlMsgInfo->opcode.opcodeBytes[1];
    }
    if (MESH_OPCODE_SIZE(pEvt->opCode) > 2)
    {
      pEvt->opCode.opcodeBytes[2] = pAccToMdlMsgInfo->opcode.opcodeBytes[2];
    }

    /* Set model ID. */
    if (pModelId->isSigModel)
    {
      pEvt->modelId.sigModelId = pModelId->modelId.sigModelId;
    }
    else
    {
      pEvt->modelId.vendorModelId = pModelId->modelId.vendorModelId;
    }

    pEvt->messageParamsLen = pAccToMdlMsgInfo->msgParamLen;
    pEvt->recvOnUnicast = pAccToMdlMsgInfo->recvOnUnicast;

    /* Point to message parameters memory at the end of the structure. */
    pEvt->pMessageParams = ((uint8_t *)pEvt) + sizeof(meshModelMsgRecvEvt_t);

    /* Copy message parameters. */
    memcpy(pEvt->pMessageParams, pAccToMdlMsgInfo->pMsgParam, pEvt->messageParamsLen);

    /* Send message. */
    WsfMsgSend(*pHandlerId, (void *)pEvt);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Transforms a fixed group address into either primary element address or unassigned.
 *
 *  \param[in] dst  Destination address to be converted.
 *
 *  \return    Primary element address or the Unassigned Address.
 */
/*************************************************************************************************/
static meshAddress_t meshAccFixedGroupToUnicast(meshAddress_t dst)
{
  meshAddress_t elem0Addr = MESH_ADDR_TYPE_UNASSIGNED;

  if (MeshLocalCfgGetAddrFromElementId(0, &elem0Addr) == MESH_SUCCESS)
  {
    switch (dst)
    {
      case MESH_ADDR_GROUP_PROXY:
        if (MeshLocalCfgGetGattProxyState() == MESH_GATT_PROXY_FEATURE_ENABLED)
        {
          return elem0Addr;
        }
        break;
      case MESH_ADDR_GROUP_FRIEND:
        if (MeshLocalCfgGetFriendState() == MESH_FRIEND_FEATURE_ENABLED)
        {
          return elem0Addr;
        }
        break;
      case MESH_ADDR_GROUP_RELAY:
        if (MeshLocalCfgGetRelayState() == MESH_RELAY_FEATURE_ENABLED)
        {
          return elem0Addr;
        }
        break;
      case MESH_ADDR_GROUP_ALL:
        return elem0Addr;
      default:
        break;
    }
  }
  return MESH_ADDR_TYPE_UNASSIGNED;
}

/*************************************************************************************************/
/*!
 *  \brief     Checks if a Mesh message must be sent to a core model and sends it.
 *
 *  \param[in] pAccToMdlMsgInfo  Pointer to structure identifying message and information to be sent
 *                               to Mesh models.
 *  \param[in] elemId            Local element identifier.
 *
 *  \return    None.
 *
 */
/*************************************************************************************************/
static void meshAccSendMsgToCoreMdl(meshAccToMdlMsgInfo_t *pAccToMdlMsgInfo, meshElementId_t elemId)
{
  meshAccCoreMdl_t *pCoreMdl;
  uint8_t opIdx;

  /* Point to start of the queue. */
  pCoreMdl = (meshAccCoreMdl_t *)(&(meshAccCb.coreMdlQueue))->pHead;

  while (pCoreMdl != NULL)
  {
    /* Check if registered element matches destination element. */
    /* Note: pointer cannot be NULL */
    /* coverity[dereference] */
    if (pCoreMdl->elemId == elemId)
    {
      /* Iterate through the opcode list. */
      for (opIdx = 0; opIdx < pCoreMdl->opcodeArrayLen; opIdx++)
      {
        /* Check if opcodes don't match. */
        if ((pAccToMdlMsgInfo->opcode.opcodeBytes[0] != pCoreMdl->pOpcodeArray[opIdx].opcodeBytes[0]))
        {
          continue;
        }
        if ((MESH_OPCODE_SIZE(pAccToMdlMsgInfo->opcode) > 1) &&
            ((pAccToMdlMsgInfo->opcode.opcodeBytes[1] != pCoreMdl->pOpcodeArray[opIdx].opcodeBytes[1])))
        {
          continue;
        }
        if ((MESH_OPCODE_SIZE(pAccToMdlMsgInfo->opcode) > 2) &&
            ((pAccToMdlMsgInfo->opcode.opcodeBytes[2] != pCoreMdl->pOpcodeArray[opIdx].opcodeBytes[2])))
        {
          continue;
        }
        /* Invoke callback. */
        pCoreMdl->msgRecvCback(opIdx, pAccToMdlMsgInfo->pMsgParam,
                          pAccToMdlMsgInfo->msgParamLen, pAccToMdlMsgInfo->src, elemId,
                          pAccToMdlMsgInfo->ttl, pAccToMdlMsgInfo->netKeyIndex);
        return;
      }
    }
    /* Move to next entry */
    pCoreMdl = pCoreMdl->pNext;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Internal function used to validate and send an Access Message to Mesh models with
 *             based on instance identifiers (model index and element index).
 *
 *  \param[in] pAccToMdlMsgInfo  Pointer to structure identifying message and information to be sent
 *                               to Mesh models.
 *  \param[in] dstElemId         Identifier of the local element matching address with the
 *                               destination address.
 *  \param[in] modelIdx          Index in the list of SIG or vendor models for an element.
 *  \param[in] isSig             TRUE if the list to search is SIG models, FALSE otherwise.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshAccSendMsgToModelInstance(meshAccToMdlMsgInfo_t *pAccToMdlMsgInfo,
                                          meshElementId_t dstElemId,
                                          uint8_t modelIdx,
                                          bool_t isSig)
{
  const meshMsgOpcode_t *pOpcodeArray;
  wsfHandlerId_t *pHandlerId;
  meshModelId_t mdlId;
  uint8_t opcodeArrayLen;
  uint8_t jj;

  mdlId.isSigModel = isSig;

  /* Get opcode list. */
  if (mdlId.isSigModel)
  {
    /* Save opcode list. */
    pOpcodeArray   = SIG_MODEL_INSTANCE(dstElemId, modelIdx).pRcvdOpcodeArray;
    opcodeArrayLen = SIG_MODEL_INSTANCE(dstElemId, modelIdx).opcodeCount;
    /* Build generic model Id. */
    mdlId.modelId.sigModelId = SIG_MODEL_INSTANCE(dstElemId, modelIdx).modelId;
    /* Save handler Id. */
    pHandlerId = SIG_MODEL_INSTANCE(dstElemId, modelIdx).pHandlerId;
  }
  else
  {
    /* Save opcode list. */
    pOpcodeArray   = VENDOR_MODEL_INSTANCE(dstElemId, modelIdx).pRcvdOpcodeArray;
    opcodeArrayLen = VENDOR_MODEL_INSTANCE(dstElemId, modelIdx).opcodeCount;
    /* Build generic model Id. */
    mdlId.modelId.vendorModelId = VENDOR_MODEL_INSTANCE(dstElemId, modelIdx).modelId;
    /* Save handler Id. */
    pHandlerId = VENDOR_MODEL_INSTANCE(dstElemId, modelIdx).pHandlerId;
  }

  if ((pOpcodeArray != NULL) && (pHandlerId != NULL))
  {
    /* Iterate through the opcode list. */
    for (jj = 0; jj < opcodeArrayLen; jj++)
    {
      /* Check if opcodes don't match. */
      if ((pAccToMdlMsgInfo->opcode.opcodeBytes[0] != pOpcodeArray[jj].opcodeBytes[0]))
      {
        continue;
      }
      if ((MESH_OPCODE_SIZE(pAccToMdlMsgInfo->opcode) > 1) &&
          ((pAccToMdlMsgInfo->opcode.opcodeBytes[1] != pOpcodeArray[jj].opcodeBytes[1])))
      {
        continue;
      }
      if ((MESH_OPCODE_SIZE(pAccToMdlMsgInfo->opcode) > 2) &&
          ((pAccToMdlMsgInfo->opcode.opcodeBytes[2] != pOpcodeArray[jj].opcodeBytes[2])))
      {
        continue;
      }
      /* Validate that correct Application Key is used. */
      if (MeshLocalCfgValidateModelToAppKeyBind(dstElemId, &mdlId, pAccToMdlMsgInfo->appKeyIndex))
      {
        /* Send WSF message. */
        meshAccSendWsfMsgRecvEvt(pAccToMdlMsgInfo, dstElemId, pHandlerId, &mdlId);
      }
      /* There should be no duplicates of opcodes for the same model and element. */
      break;
    }
  }
  else
  {
    if(pHandlerId == NULL)
    {
      MESH_TRACE_ERR1("MESH ACC: WSF Handler not installed for model %d ",
                       (mdlId.isSigModel ? mdlId.modelId.sigModelId :
                                           mdlId.modelId.vendorModelId));
    }

    if(pOpcodeArray == NULL)
    {
      MESH_TRACE_ERR1("MESH ACC: Opcode list NULL for model %d ",
                      (mdlId.isSigModel ? mdlId.modelId.sigModelId :
                                          mdlId.modelId.vendorModelId));
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Internal function used to validate and send an Access Message to Mesh models with
 *             unicast destination criteria.
 *
 *  \param[in] pAccToMdlMsgInfo  Pointer to structure identifying message and information to be sent
 *                               to Mesh models.
 *  \param[in] dstElemId         Identifier of the local element matching address with the
 *                               destination address.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshAccSendMsgToModelUnicast(meshAccToMdlMsgInfo_t *pAccToMdlMsgInfo,
                                         meshElementId_t dstElemId)
{
  uint8_t mdlInstanceIdx;
  /* Check if model is SIG or vendor based on opcode. */
  bool_t isSigModel = !MESH_OPCODE_IS_VENDOR(pAccToMdlMsgInfo->opcode);
  uint8_t numModels = isSigModel ? pMeshConfig->pElementArray[dstElemId].numSigModels :
                                   pMeshConfig->pElementArray[dstElemId].numVendorModels;

  /* Try to send message to core models only if they are using device keys. */
  if ((pAccToMdlMsgInfo->appKeyIndex == MESH_APPKEY_INDEX_LOCAL_DEV_KEY) ||
      (pAccToMdlMsgInfo->appKeyIndex == MESH_APPKEY_INDEX_REMOTE_DEV_KEY))
  {
    /* Messages should be received on unicast with device keys. */
    if (pAccToMdlMsgInfo->recvOnUnicast)
    {
      meshAccSendMsgToCoreMdl(pAccToMdlMsgInfo, dstElemId);
    }
    return;
  }

  /* Search standalone model list. */
  for (mdlInstanceIdx = 0; mdlInstanceIdx < numModels; mdlInstanceIdx++)
  {
    meshAccSendMsgToModelInstance(pAccToMdlMsgInfo, dstElemId, mdlInstanceIdx, isSigModel);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Internal function used to validate and send an Access Message to Mesh models with
 *             group or virtual destination address criteria.
 *
 *  \param[in] pAccToMdlMsgInfo  Pointer to structure identifying message and information to be sent
 *                               to Mesh models.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshAccSendMsgToModelMulticast(meshAccToMdlMsgInfo_t *pAccToMdlMsgInfo)
{
  uint8_t elemId, modelIdx;
  meshModelId_t mdlId;
  uint8_t numModels;
  mdlId.isSigModel = !MESH_OPCODE_IS_VENDOR(pAccToMdlMsgInfo->opcode);

  for (elemId = 0; elemId < pMeshConfig->elementArrayLen; elemId++)
  {
    numModels = mdlId.isSigModel ? pMeshConfig->pElementArray[elemId].numSigModels :
                                   pMeshConfig->pElementArray[elemId].numVendorModels;
    for (modelIdx = 0; modelIdx < numModels; modelIdx++)
    {
      /* Build generic model id. */
      if (mdlId.isSigModel)
      {
        mdlId.modelId.sigModelId = SIG_MODEL_INSTANCE(elemId,modelIdx).modelId;
      }
      else
      {
        mdlId.modelId.vendorModelId = VENDOR_MODEL_INSTANCE(elemId, modelIdx).modelId;
      }

      /* Check if model instance is subscribed to destination. */
      if (MeshLocalCfgFindAddrInModelSubscrList(elemId, &mdlId, pAccToMdlMsgInfo->dst,
                                                pAccToMdlMsgInfo->pDstLabelUuid))
      {
        /* Try to send message to model instance. */
        meshAccSendMsgToModelInstance(pAccToMdlMsgInfo, elemId, modelIdx, mdlId.isSigModel);
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Checks and loopbacks a mesh message sent to a multicast destination.
 *
 *  \param[in] pMsgInfo     Pointer to a Mesh message information structure. See ::meshMsgInfo_t
 *  \param[in] pMsgParam    Pointer to a Mesh message parameter list.
 *  \param[in] msgParamLen  Length of the message parameter list.
 *  \param[in] netKeyIndex  Global Network Key identifier.
 *  \param[in] src          Address of the originating element.
 *
 *  \return    TRUE if multicast message is also loopback.
 */
/*************************************************************************************************/
static bool_t meshAccCheckAndLoopbackMsgMulticast(const meshMsgInfo_t *pMsgInfo,
                                                  const uint8_t *pMsgParam, uint16_t msgParamLen,
                                                  uint16_t netKeyIndex, meshAddress_t src)
{
  meshAccToMdlMsgInfo_t accToMdlMsgInfo;
  bool_t sendToUnicast = FALSE;

  /* Check if address is virtual or group. */
  if (MESH_IS_ADDR_GROUP(pMsgInfo->dstAddr) || MESH_IS_ADDR_VIRTUAL(pMsgInfo->dstAddr))
  {
    /* Try to loopback fixed group addresses and subscribed addresses. */
    if (MESH_IS_ADDR_FIXED_GROUP(pMsgInfo->dstAddr) ||
        MeshLocalCfgFindSubscrAddr(pMsgInfo->dstAddr))
    {
      /* Convert to unicast address. */
      if (MESH_IS_ADDR_FIXED_GROUP(pMsgInfo->dstAddr))
      {
        /* Find element address from fixed group address. */
        accToMdlMsgInfo.dst = meshAccFixedGroupToUnicast(pMsgInfo->dstAddr);

        /* If conversion failed, stop execution. */
        if (accToMdlMsgInfo.dst == MESH_ADDR_TYPE_UNASSIGNED)
        {
          return FALSE;
        }
        sendToUnicast = TRUE;
      }
      else
      {
        accToMdlMsgInfo.dst = pMsgInfo->dstAddr;
        sendToUnicast = FALSE;
      }

      /* Configure PDU information for reception. */
      accToMdlMsgInfo.pMsgParam = (uint8_t *)pMsgParam;
      accToMdlMsgInfo.msgParamLen = msgParamLen;
      accToMdlMsgInfo.src = src;
      accToMdlMsgInfo.ttl = pMsgInfo->ttl;
      accToMdlMsgInfo.appKeyIndex = pMsgInfo->appKeyIndex;
      accToMdlMsgInfo.netKeyIndex = netKeyIndex;
      accToMdlMsgInfo.opcode = pMsgInfo->opcode;
      accToMdlMsgInfo.recvOnUnicast = FALSE;
      accToMdlMsgInfo.pDstLabelUuid = MESH_IS_ADDR_VIRTUAL(pMsgInfo->dstAddr) ?
                                      pMsgInfo->pDstLabelUuid : NULL ;
      if (sendToUnicast)
      {
        /* Try to loop message back to primary element. */
        meshAccSendMsgToModelUnicast(&accToMdlMsgInfo, 0);
      }
      else
      {
        /* Try to loop message back to models subscribed to the destination. */
        meshAccSendMsgToModelMulticast(&accToMdlMsgInfo);
      }

      return TRUE;
    }
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Checks and loopbacks a mesh message sent to a unicast destination.
 *
 *  \param[in] pMsgInfo     Pointer to a Mesh message information structure. See ::meshMsgInfo_t
 *  \param[in] pMsgParam    Pointer to a Mesh message parameter list.
 *  \param[in] msgParamLen  Length of the message parameter list.
 *  \param[in] netKeyIndex  Global Network Key identifier.
 *  \param[in] src          Address of the originating element.
 *
 *  \return    TRUE if message is unicast loopback, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshAccCheckAndLoopbackMsgUnicast(const meshMsgInfo_t *pMsgInfo,
                                                const uint8_t *pMsgParam, uint16_t msgParamLen,
                                                uint16_t netKeyIndex, meshAddress_t src)
{
  meshAccToMdlMsgInfo_t accToMdlMsgInfo;
  meshElementId_t dstElemId;

  if ((MeshLocalCfgGetElementIdFromAddr(pMsgInfo->dstAddr, &dstElemId) == MESH_SUCCESS) &&
      (dstElemId < pMeshConfig->elementArrayLen))
  {
    /* Configure PDU information for reception. */
    accToMdlMsgInfo.pMsgParam = (uint8_t *)pMsgParam;
    accToMdlMsgInfo.msgParamLen = msgParamLen;
    accToMdlMsgInfo.src = src;
    accToMdlMsgInfo.dst = pMsgInfo->dstAddr;
    accToMdlMsgInfo.ttl = pMsgInfo->ttl;
    accToMdlMsgInfo.appKeyIndex = pMsgInfo->appKeyIndex;
    accToMdlMsgInfo.netKeyIndex = netKeyIndex;
    accToMdlMsgInfo.opcode = pMsgInfo->opcode;
    accToMdlMsgInfo.recvOnUnicast = TRUE;

    /* Loop message back. */
    meshAccSendMsgToModelUnicast(&accToMdlMsgInfo, dstElemId);

    return TRUE;
  }
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Internal function used to send a Mesh Message to Upper Transport PDU on a specific
 *             subnet or on loopback interface.
 *
 *  \param[in] pAllocPduInfo          Pointer to allocated Upper Transport PDU and information or
 *                                    NULL when local (static) memory should be used.
 *  \param[in] pMsgInfo               Pointer to a Mesh message information structure.
 *                                    See ::meshMsgInfo_t
 *  \param[in] pMsgParam              Pointer to a Mesh message parameter list.
 *  \param[in] msgParamLen            Length of the message parameter list.
 *  \param[in] netKeyIndex            Global Network Key identifier.
 *  \param[in] publishFriendshipCred  Publish security credentials.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshAccSendMsg(meshUtrAccPduTxInfo_t *pAllocPduInfo, const meshMsgInfo_t *pMsgInfo,
                           const uint8_t *pMsgParam, uint16_t msgParamLen, uint16_t netKeyIndex,
                           meshPublishFriendshipCred_t publishFriendshipCred)
{
  meshUtrAccPduTxInfo_t localPduTxInfo;
  meshUtrAccPduTxInfo_t *pUtrAccPduTxInfo;
  meshAddress_t src;

  /* Read source address from element id. */
  if (MeshLocalCfgGetAddrFromElementId(pMsgInfo->elementId, &src) != MESH_SUCCESS)
  {
    MESH_TRACE_ERR0("MESH ACC: Send/Publish message failed, source address not found");
    return;
  }

  /* Check if destination is unicast. */
  if (MESH_IS_ADDR_UNICAST(pMsgInfo->dstAddr))
  {
    /* Check if destination is local and loopback. */
    if (meshAccCheckAndLoopbackMsgUnicast(pMsgInfo, pMsgParam, msgParamLen, netKeyIndex, src))
    {
      return;
    }
  }

  /* Check if requester provided memory for Upper Transport PDU information. */
  if (pAllocPduInfo == NULL)
  {
    pUtrAccPduTxInfo = &localPduTxInfo;
  }
  else
  {
    pUtrAccPduTxInfo = pAllocPduInfo;
  }

  /* Set source. */
  pUtrAccPduTxInfo->src = src;
  /* Set destination. */
  pUtrAccPduTxInfo->dst = pMsgInfo->dstAddr;
  /* Set pointer to label UUID. */
  pUtrAccPduTxInfo->pDstLabelUuid = pMsgInfo->pDstLabelUuid;
  /* Set TTL. */
  pUtrAccPduTxInfo->ttl = pMsgInfo->ttl;

  pUtrAccPduTxInfo->ackRequired = FALSE;

  /* Instruct Upper transport to use either Application or Device Keys. */
  pUtrAccPduTxInfo->devKeyUse = ((pMsgInfo->appKeyIndex == MESH_APPKEY_INDEX_LOCAL_DEV_KEY) ||
                                 (pMsgInfo->appKeyIndex == MESH_APPKEY_INDEX_REMOTE_DEV_KEY));

  /* Set pointer to opcode bytes. */
  pUtrAccPduTxInfo->pAccPduOpcode   = pMsgInfo->opcode.opcodeBytes;
  /* Set opcode size. */
  pUtrAccPduTxInfo->accPduOpcodeLen = MESH_OPCODE_SIZE(pMsgInfo->opcode);
  /* Set pointer to message parameters. */
  pUtrAccPduTxInfo->pAccPduParam    = pMsgParam;
  /* Set length of message parameters. */
  pUtrAccPduTxInfo->accPduParamLen  = msgParamLen;
  /* Configure Key Indexes to be used. */
  pUtrAccPduTxInfo->appKeyIndex = pMsgInfo->appKeyIndex;
  pUtrAccPduTxInfo->netKeyIndex = netKeyIndex;
  pUtrAccPduTxInfo->friendLpnAddr = MESH_ADDR_TYPE_UNASSIGNED;

  /* Check if Friendship credentials should be used. */
  if (publishFriendshipCred == MESH_PUBLISH_FRIEND_SECURITY)
  {
    pUtrAccPduTxInfo->friendLpnAddr = meshAccCb.friendAddrFromSubnetCback(netKeyIndex);
  }

  /* Errata 10578: all pending retransmissions of the previous message published by the
   * model instance shall be canceled.
   */
  meshAccCancelPendingRetrans(src, pUtrAccPduTxInfo->pAccPduOpcode, pUtrAccPduTxInfo->accPduOpcodeLen);

  /* Send PDU information to Upper Transport. */
  MeshUtrSendAccPdu(pUtrAccPduTxInfo);

  /* Check and publish multicast message on loopback. */
  (void)meshAccCheckAndLoopbackMsgMulticast(pMsgInfo, pMsgParam, msgParamLen, netKeyIndex, src);
}

/*************************************************************************************************/
/*!
 *  \brief     Timer callback for retransmitting published messages.
 *
 *  \param[in] tmrUid  Timer unique identifier.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshAccPubRetransTmrCback(uint16_t tmrUid)
{
  meshAccPduPubTxInfo_t *ptr, *prev;

  /* Point to start of the queue. */
  ptr = (meshAccPduPubTxInfo_t *)(meshAccCb.pubRetransQueue.pHead);
  prev = NULL;

  /* Parse the queue. */
  while (ptr != NULL)
  {
    /* Check if timer identifier matches. */
    if (ptr->retransTmr.msg.param == tmrUid)
    {
      if (ptr->publishRetransCount > 0)
      {
        /* Decrement retransmit count. */
        --(ptr->publishRetransCount);

        /* Retransmit. */
        MeshUtrSendAccPdu(&(ptr->utrAccPduTxInfo));

        /* Check if another retransmission is required. */
        if (ptr->publishRetransCount != 0)
        {
          /* Restart timer. */
          WsfTimerStartMs(&(ptr->retransTmr), RETRANS_STEPS_TO_MS_TIME(ptr->publishRetransSteps50Ms));
        }
        else
        {
          /* Remove from queue. */
          WsfQueueRemove(&(meshAccCb.pubRetransQueue), ptr, prev);

          /* Free the allocated memory. */
          WsfBufFree(ptr);
        }
      }

      return;
    }
    /* Move to next entry. */
    prev = ptr;
    ptr = (meshAccPduPubTxInfo_t *)(ptr->pNext);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Cancel all pending retransmissions for a model instance.
 *
 *  \param[in] src              The source address
 *  \param[in] pAccPduOpcode    Pointer to the opcode
 *  \param[in] accPduOpcodeLen  The length of the opcode
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshAccCancelPendingRetrans(meshAddress_t src, const uint8_t *pAccPduOpcode, 
                                        uint8_t accPduOpcodeLen)
{
  meshAccPduPubTxInfo_t *ptr, *prev, *next;

  /* Point to start of the queue. */
  ptr = (meshAccPduPubTxInfo_t *)(meshAccCb.pubRetransQueue.pHead);
  prev = NULL;

  /* Parse the queue. */
  while (ptr != NULL)
  {
    next = (meshAccPduPubTxInfo_t *)(ptr->pNext);

    /* Check if the retransmission is done by the same model instance
     * (same source address and opcode)
     */
    if ((ptr->utrAccPduTxInfo.src == src) &&
        (ptr->utrAccPduTxInfo.accPduOpcodeLen == accPduOpcodeLen) &&
        (memcmp(ptr->utrAccPduTxInfo.pAccPduOpcode, pAccPduOpcode, accPduOpcodeLen) == 0))
    {
      /* Remove from queue. */
      WsfQueueRemove(&(meshAccCb.pubRetransQueue), ptr, prev);

      /* Stop timer. */
      WsfTimerStop(&(ptr->retransTmr));

      /* Free the allocated memory. */
      WsfBufFree(ptr);
    }
    else
    {
      prev = ptr;
    }

    /* Move to next entry. */
    ptr = next;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Timer callback for delaying access messages.
 *
 *  \param[in] tmrUid  Timer unique identifier.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshAccMsgDelayTmrCback(uint16_t tmrUid)
{
  meshSendMessage_t *pMsg;
  wsfHandlerId_t handlerId;

  /* Get queue count. */
  uint16_t count = WsfQueueCount(&(meshAccCb.msgSendQueue));

  /* Iterate through queue. */
  while (count > 0)
  {
    /* Dequeue message. */
    pMsg = WsfMsgDeq(&(meshAccCb.msgSendQueue), &handlerId);

    /* Check if timer expired. */
    /* coverity[dereference] */
    if (pMsg->delayTmr.msg.param == tmrUid)
    {
      /* Call internal handler. */
      meshAccSendMsg(NULL, &pMsg->msgInfo, pMsg->pMsgParam, pMsg->msgParamLen, pMsg->netKeyIndex,
                     MESH_PUBLISH_MASTER_SECURITY);

      /* Free queue pointer. */
      WsfMsgFree(pMsg);

      return;
    }
    /* Enqueue message back. */
    WsfMsgEnq(&(meshAccCb.msgSendQueue), handlerId, (void *)pMsg);
    count--;
  }
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
static void meshAccWsfMsgHandlerCback(wsfMsgHdr_t *pMsg)
{
  /* Check event type to handle timer expiration. */
  switch(pMsg->event)
  {
    case MESH_ACC_MSG_RETRANS_TMR_EXPIRED:
      meshAccPubRetransTmrCback(pMsg->param);
      break;

    case MESH_ACC_MSG_DELAY_TMR_EXPIRED:
      meshAccMsgDelayTmrCback(pMsg->param);
      break;

    default:
      /* Route default events to periodic publishing. */
      meshAccCb.ppWsfMsgCback(pMsg);
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Upper Transport PDU received callback.
 *
 *  \param[in] pAccPduInfo  Pointer to Upper Transport PDU and information.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshUtrAccRecvCback(const meshUtrAccPduRxInfo_t *pAccPduInfo)
{
  meshAccToMdlMsgInfo_t accToMdlMsgInfo;

  uint8_t idx = 0;
  meshElementId_t dstElemId;

  WSF_ASSERT(pAccPduInfo != NULL);

  if ((pAccPduInfo->pAccPdu != NULL) && (pAccPduInfo->pduLen > 0) &&
      (pAccPduInfo->pduLen < MESH_ACC_MAX_PDU_SIZE))
  {
    /* Copy first byte of the opcode. */
    accToMdlMsgInfo.opcode.opcodeBytes[0] = pAccPduInfo->pAccPdu[0];

    /* Copy remaining bytes. */
    for (idx = 1; idx < MESH_OPCODE_SIZE(accToMdlMsgInfo.opcode); idx++)
    {
      accToMdlMsgInfo.opcode.opcodeBytes[idx] = pAccPduInfo->pAccPdu[idx];
    }

    /* Make sure PDU length has at least opcode number of bytes. */
    if (pAccPduInfo->pduLen >= idx)
    {
      /* Copy addressing info. */
      accToMdlMsgInfo.src = pAccPduInfo->src;
      accToMdlMsgInfo.dst = pAccPduInfo->dst;
      accToMdlMsgInfo.pDstLabelUuid = pAccPduInfo->pDstLabelUuid;

      /* Copy additional identification data. */
      accToMdlMsgInfo.appKeyIndex = pAccPduInfo->appKeyIndex;
      accToMdlMsgInfo.netKeyIndex = pAccPduInfo->netKeyIndex;
      accToMdlMsgInfo.ttl = pAccPduInfo->ttl;
      accToMdlMsgInfo.recvOnUnicast = TRUE;

      /* Point to message parameters and set length. */
      accToMdlMsgInfo.pMsgParam = (uint8_t *)(&(pAccPduInfo->pAccPdu[idx]));
      accToMdlMsgInfo.msgParamLen = pAccPduInfo->pduLen - idx;

      /* Start validating conditions for standalone models. */

      /* Verify address type. */
      if (MESH_IS_ADDR_UNICAST(accToMdlMsgInfo.dst) ||
          MESH_IS_ADDR_FIXED_GROUP(accToMdlMsgInfo.dst))
      {
        /* Validate fixed group addresses. */
        if (MESH_IS_ADDR_FIXED_GROUP(accToMdlMsgInfo.dst))
        {
          /* Find element address from fixed group address. */
          accToMdlMsgInfo.dst = meshAccFixedGroupToUnicast(accToMdlMsgInfo.dst);

          /* If conversion failed, stop execution. */
          if (accToMdlMsgInfo.dst == MESH_ADDR_TYPE_UNASSIGNED)
          {
            return;
          }
          dstElemId = 0;
          /* Clear received on unicast flag. */
          accToMdlMsgInfo.recvOnUnicast = FALSE;
        }
        else
        {
          /* Get element id based on destination address. */
          if (MeshLocalCfgGetElementIdFromAddr(accToMdlMsgInfo.dst, &dstElemId) != MESH_SUCCESS)
          {
            return;
          }
          WSF_ASSERT(dstElemId < pMeshConfig->elementArrayLen);
        }
        /* Send message to model with unicat destination. */
        meshAccSendMsgToModelUnicast(&accToMdlMsgInfo, dstElemId);
      }
      else if (MESH_IS_ADDR_DYN_GROUP(accToMdlMsgInfo.dst) || MESH_IS_ADDR_VIRTUAL(accToMdlMsgInfo.dst))
      {
        if (MeshLocalCfgFindSubscrAddr(accToMdlMsgInfo.dst))
        {
          /* Clear received on unicast flag. */
          accToMdlMsgInfo.recvOnUnicast = FALSE;

          /* Send message to model with group or virtual destination. */
          meshAccSendMsgToModelMulticast(&accToMdlMsgInfo);
        }
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Upper Transport event callback.
 *
 *  \param[in] event        Type of event.
 *  \param[in] pEventParam  Parameters associated to the event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshUtrEventNotifyCback(meshUtrEvent_t event, void *pEventParam)
{
  (void)event;
  (void)pEventParam;
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Mesh Access layer.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshAccInit(void)
{
  MESH_TRACE_INFO0("MESH ACCESS: init");

  /* Initialize publication queue. */
  while (!WsfQueueEmpty(&(meshAccCb.pubRetransQueue)))
  {
    WsfBufFree(WsfQueueDeq(&(meshAccCb.pubRetransQueue)));
  }
  WSF_QUEUE_INIT(&(meshAccCb.pubRetransQueue));
  WSF_QUEUE_INIT(&(meshAccCb.coreMdlQueue));
  WSF_QUEUE_INIT(&(meshAccCb.msgSendQueue));

  /* Uninstall optional feature. */
  meshAccCb.ppChangedCback = meshAccEmptyPpChangedCback;
  meshAccCb.ppWsfMsgCback = meshEmptyAccPpMsgHandler;
  meshAccCb.friendAddrFromSubnetCback = meshAccEmptyFriendAddrFromSubnetCback;

  /* Register WSF message handler callback. */
  meshCb.accMsgCback = meshAccWsfMsgHandlerCback;

  /* Register with Upper Transport. */
  MeshUtrRegister(meshUtrAccRecvCback, meshUtrEventNotifyCback);
}

/*************************************************************************************************/
/*!
 *  \brief     Registers callback for LPN.
 *
 *  \param[in] cback  Callback to get the Friend address from subnet.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAccRegisterLpn(meshAccFriendAddrFromSubnetCback_t cback)
{
  /* Store LPN callback. */
  if (cback != NULL)
  {
    meshAccCb.friendAddrFromSubnetCback = cback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the Access Layer information to multiplex received messages to core models.
 *
 *  \param[in] pCoreMdl  Pointer to linked list element containing information used by the Access
 *                       Layer to multiplex messages on the RX data path.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshAccRegisterCoreModel(meshAccCoreMdl_t *pCoreMdl)
{
  WSF_ASSERT(pCoreMdl != NULL);
  WSF_ASSERT(pCoreMdl->msgRecvCback != NULL);
  WSF_ASSERT(pCoreMdl->pOpcodeArray != NULL);

  WsfQueueEnq(&(meshAccCb.coreMdlQueue), (void *)pCoreMdl);
}

/*************************************************************************************************/
/*!
 *  \brief      Gets number of core models contained by an element.
 *
 *  \param[in]  elemId         Element identifier.
 *  \param[out] pOutNumSig     Pointer to memory where number of SIG models is stored.
 *  \param[out] pOutNumVendor  Pointer to memory where number of Vendor models is stored.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void MeshAccGetNumCoreModels(uint8_t elemId, uint8_t *pOutNumSig, uint8_t *pOutNumVendor)
{
  meshAccCoreMdl_t *pCoreMdl;

  WSF_ASSERT(pOutNumSig != NULL);
  WSF_ASSERT(pOutNumVendor != NULL);

  *pOutNumSig = 0;
  *pOutNumVendor = 0;

  /* Point to start of the queue. */
  pCoreMdl = (meshAccCoreMdl_t *)(&(meshAccCb.coreMdlQueue))->pHead;

  while (pCoreMdl != NULL)
  {
    /* Check if element id matches. */
    /* coverity[dereference] */
    if (pCoreMdl->elemId == elemId)
    {
      if (pCoreMdl->mdlId.isSigModel)
      {
        (*pOutNumSig)++;
      }
      else
      {
        (*pOutNumVendor)++;
      }
    }

    /* Move to next entry */
    pCoreMdl = pCoreMdl->pNext;
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Gets core SIG models identifiers of models contained by an element.
 *
 *  \param[in]  elemId          Element identifier.
 *  \param[out] pOutMdlIdArray  Pointer to buffer store SIG model identifiers of core models.
 *  \param[in]  arraySize       Size of the array provided.
 *
 *  \return     Number of core SIG model identifiers contained by the element.
 */
/*************************************************************************************************/
uint8_t MeshAccGetCoreSigModelsIds(uint8_t elemId, meshSigModelId_t *pOutSigMdlIdArray,
                                   uint8_t arraySize)
{
  meshAccCoreMdl_t *pCoreMdl;
  uint8_t cnt = 0;

  WSF_ASSERT(pOutSigMdlIdArray != NULL);

  /* Point to start of the queue. */
  pCoreMdl = (meshAccCoreMdl_t *)(&(meshAccCb.coreMdlQueue))->pHead;

  /* Get queue element. */
  while (pCoreMdl != NULL)
  {

    /* Check if element id matches. */
    /* Note: pCoreMdl cannot be NULL */
    /* coverity[dereference] */
    if ((pCoreMdl->elemId == elemId) && (pCoreMdl->mdlId.isSigModel))
    {
      cnt++;
      if (arraySize > 0)
      {
        *pOutSigMdlIdArray = pCoreMdl->mdlId.modelId.sigModelId;
        pOutSigMdlIdArray++;
        arraySize--;
      }
    }

    pCoreMdl = pCoreMdl->pNext;
  }

  return cnt;
}

/*************************************************************************************************/
/*!
 *  \brief     Allocate and build an WSF message for delaying or sending an Access message.
 *
 *  \param[in] pMsgInfo       Pointer to a Mesh message information structure. See ::meshMsgId_t
 *  \param[in] pMsgParam      Pointer to a Mesh message parameter list.
 *  \param[in] msgParamLen    Length of the message parameter list.
 *  \param[in] netKeyIndex    Global Network Key identifier.
 *
 *  \return    Pointer to meshSendMessage_t.
 */
/*************************************************************************************************/
meshSendMessage_t *MeshAccAllocMsg(const meshMsgInfo_t *pMsgInfo, const uint8_t *pMsgParam,
                                   uint16_t msgParamLen, uint16_t netKeyIndex)
{
  uint16_t apiMsgLen;
  meshSendMessage_t *pMsg = NULL;

  /* Calculate API msg length. */
  apiMsgLen = sizeof(meshSendMessage_t) + sizeof(meshMsgInfo_t) + msgParamLen;
  apiMsgLen += (pMsgInfo->pDstLabelUuid != NULL ? MESH_LABEL_UUID_SIZE : 0);

  /* Allocate the Stack Message and additional size for message parameters. */
  if ((pMsg = WsfMsgAlloc(apiMsgLen)) != NULL)
  {
    /* Copy message information. */
    memcpy(&(pMsg->msgInfo), pMsgInfo, sizeof(meshMsgInfo_t));

    if (msgParamLen > 0)
    {
      /* Set message parameters address after the message structure. */
      pMsg->pMsgParam = (uint8_t *)((uint8_t *)pMsg + sizeof(meshSendMessage_t));

      /* Copy the message parameters at the end of the event structure and identifier. */
      memcpy(pMsg->pMsgParam, pMsgParam, msgParamLen);
    }
    else
    {
      /* Message does not have any parameters. */
      pMsg->pMsgParam = NULL;
    }

    /* Add Label UUID message parameters location address. */
    if (pMsgInfo->pDstLabelUuid != NULL)
    {
      pMsg->msgInfo.pDstLabelUuid = (uint8_t *)((uint8_t *)pMsg + sizeof(meshSendMessage_t)
                                                 + msgParamLen);

      /* Copy the Label UUID at the calculated address. */
      memcpy(pMsg->msgInfo.pDstLabelUuid, pMsgInfo->pDstLabelUuid, MESH_LABEL_UUID_SIZE);
    }

    /* Add message parameters length. */
    pMsg->msgParamLen = msgParamLen;

    /* Add NetKey Index. */
    pMsg->netKeyIndex = netKeyIndex;
  }

  return pMsg;
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Mesh message to a destination address with a random delay.
 *
 *  \param[in] pMsgInfo       Pointer to a Mesh message information structure. See ::meshMsgId_t
 *  \param[in] pMsgParam      Pointer to a Mesh message parameter list.
 *  \param[in] msgParamLen    Length of the message parameter list.
 *  \param[in] netKeyIndex    Global Network Key identifier.
 *  \param[in] rndDelayMsMin  Minimum value for random send delay in ms. 0 for instant send.
 *  \param[in] rndDelayMsMax  Maximum value for random send delay in ms. 0 for instant send.
 *
 *  \return    None.
 *
 *  \remarks   pMsgInfo->appKeyIndex can also be ::MESH_APPKEY_INDEX_LOCAL_DEV_KEY or
 *             ::MESH_APPKEY_INDEX_REMOTE_DEV_KEY for local or remote Device keys.
 */
/*************************************************************************************************/
void MeshAccSendMessage(const meshMsgInfo_t *pMsgInfo, const uint8_t *pMsgParam,
                        uint16_t msgParamLen, uint16_t netKeyIndex, uint32_t rndDelayMsMin,
                        uint32_t rndDelayMsMax)
{
  meshSendMessage_t *pMsg;
  uint32_t delayTimeMs;

  if ((rndDelayMsMin == 0) && (rndDelayMsMax == 0))
  {
    /* Call internal handler. */
    meshAccSendMsg(NULL, pMsgInfo, pMsgParam, msgParamLen, netKeyIndex,
                   MESH_PUBLISH_MASTER_SECURITY);
  }
  else
  {
    /* Allocate the Stack Message and additional size for message parameters. */
    if ((pMsg = MeshAccAllocMsg(pMsgInfo, pMsgParam, msgParamLen, netKeyIndex)) != NULL)
    {
      /* Copy message information. */
      memcpy(&(pMsg->msgInfo), pMsgInfo, sizeof(meshMsgInfo_t));

      /* Read random number. */
      SecRand((uint8_t *)&(delayTimeMs), sizeof(delayTimeMs));

      /* Send random number in range. */
      delayTimeMs = (delayTimeMs % (rndDelayMsMax + 1 - rndDelayMsMin)) + rndDelayMsMin;

      /* Configure timer. */
      pMsg->delayTmr.msg.event = MESH_ACC_MSG_DELAY_TMR_EXPIRED;
      pMsg->delayTmr.msg.param = meshAccSendMsgTmrIdAlloc();
      pMsg->delayTmr.handlerId = meshCb.handlerId;

      /* Enqueue message. */
      WsfMsgEnq(&(meshAccCb.msgSendQueue), meshCb.handlerId, (void *)pMsg);

      /* Start timer. */
      WsfTimerStartMs(&(pMsg->delayTmr), delayTimeMs);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publishes a Mesh Message based on internal Model Publication State configuration.
 *
 *  \param[in] pPubMsgInfo  Pointer to a published Mesh message information structure.
 *  \param[in] pMsgParam    Pointer to a Mesh message parameter list.
 *  \param[in] msgParamLen  Length of the message parameter list.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAccPublishMessage(const meshPubMsgInfo_t *pPubMsgInfo, const uint8_t *pMsgParam,
                           uint16_t msgParamLen)
{
  uint8_t               *pLabelUuid;
  meshAccPduPubTxInfo_t *pAccPduPubInfo;
  meshMsgInfo_t msgInfo;
  meshModelId_t mdlId;
  meshAddress_t dstAddr;
  meshElementId_t dstElemId;
  uint16_t pubNetKeyIndex = 0;
  uint16_t pubAppKeyIndex = 0;
  meshPublishRetransCount_t pubRetransCount = 0;
  meshPublishRetransIntvlSteps_t  pubRetransSteps50Ms = 0;
  meshPublishFriendshipCred_t pubFriendshipCred = MESH_PUBLISH_MASTER_SECURITY;
  uint8_t pubTtl = 0;

  /* Build generic model identifier. */
  mdlId.isSigModel = !MESH_OPCODE_IS_VENDOR(pPubMsgInfo->opcode);
  if (mdlId.isSigModel)
  {
    mdlId.modelId.sigModelId = pPubMsgInfo->modelId.sigModelId;
  }
  else
  {
    mdlId.modelId.vendorModelId = pPubMsgInfo->modelId.vendorModelId;
  }

  /* Read publication destination. */
  if (MeshLocalCfgGetPublishAddress(pPubMsgInfo->elementId, &mdlId, &dstAddr, &pLabelUuid)
      != MESH_SUCCESS)
  {
    MESH_TRACE_ERR0("MESH ACC: Publication failed, publication address not found ");
    return;
  }

  if (dstAddr == MESH_ADDR_TYPE_UNASSIGNED)
  {
    MESH_TRACE_WARN0("MESH ACC: Publication is disabled ");
    return;
  }

  /* Get publication Application Key. */
  if (MeshLocalCfgGetPublishAppKeyIndex(pPubMsgInfo->elementId, &mdlId, &pubAppKeyIndex) != MESH_SUCCESS)
  {
    MESH_TRACE_ERR0("MESH ACC: Publication failed, AppKey index not found ");
    return;
  }

  /* Get publication TTL. */
  if (MeshLocalCfgGetPublishTtl(pPubMsgInfo->elementId, &mdlId, &pubTtl) != MESH_SUCCESS)
  {
    MESH_TRACE_ERR0("MESH ACC: Publication failed, publish TTL not found ");
    return;
  }

  /* Validate publication TTL. */
  if (!MESH_TTL_IS_VALID(pubTtl))
  {
    MESH_TRACE_ERR0("MESH ACC: Publication failed, publish TTL invalid ");
    return;
  }

  /* Validate AppKey Index. */
  if (!MeshLocalCfgValidateModelToAppKeyBind(pPubMsgInfo->elementId, &mdlId, pubAppKeyIndex))
  {
    MESH_TRACE_ERR0("MESH ACC: Send message failed, AppKey not bound to model instance !");
    return;
  }

  /* Get Bound NetKey Index. */
  if (MeshLocalCfgGetBoundNetKeyIndex(pubAppKeyIndex, &pubNetKeyIndex) != MESH_SUCCESS)
  {
    MESH_TRACE_ERR0("MESH ACC: Publication failed, NetKey not bound to AppKey !");
    return;
  }

  /* Read retransmission params. */
  if (MeshLocalCfgGetPublishRetransIntvlSteps(pPubMsgInfo->elementId, &mdlId, &pubRetransSteps50Ms)
      != MESH_SUCCESS)
  {
    MESH_TRACE_ERR0("MESH ACC: Publication failed, cannot read retransmit interval !");
    return;
  }

  if (MeshLocalCfgGetPublishRetransCount(pPubMsgInfo->elementId, &mdlId, &pubRetransCount)
      != MESH_SUCCESS)
  {
    MESH_TRACE_ERR0("MESH ACC: Publication failed, cannot read retransmit count !");
    return;
  }

  /* Construct a Mesh message identifier to use the message sending implementation. */
  msgInfo.elementId     = pPubMsgInfo->elementId;
  msgInfo.dstAddr       = dstAddr;
  msgInfo.pDstLabelUuid = pLabelUuid;
  msgInfo.appKeyIndex   = pubAppKeyIndex;
  msgInfo.ttl           = pubTtl;
  if (mdlId.isSigModel)
  {
    msgInfo.modelId.sigModelId = pPubMsgInfo->modelId.sigModelId;
  }
  else
  {
    msgInfo.modelId.vendorModelId = pPubMsgInfo->modelId.vendorModelId;
  }
  msgInfo.opcode = pPubMsgInfo->opcode;

  /* Check if publication destination is unicast. */
  if (MESH_IS_ADDR_UNICAST(dstAddr))
  {
    /* If destination is local unicast. */
    if ((MeshLocalCfgGetElementIdFromAddr(dstAddr, &dstElemId) == MESH_SUCCESS) &&
        (dstElemId < pMeshConfig->elementArrayLen))
    {
      /* Set retransmission to 0 since there is no need to retransmit locally. */
      pubRetransCount = 0;
    }
  }

  /* Check Publish Credential Flag. */
  MeshLocalCfgGetPublishFriendshipCredFlag(pPubMsgInfo->elementId, &mdlId, &pubFriendshipCred);

  /* Check if retransmissions are needed. */
  if (pubRetransCount != 0)
  {
    /* Allocate memory for retransmissions. */
    if ((pAccPduPubInfo =
         WsfBufAlloc(sizeof(meshAccPduPubTxInfo_t) + MESH_OPCODE_SIZE(msgInfo.opcode) + msgParamLen))
        == NULL)
    {
      return;
    }

    /* Trigger internal request with allocated memory for retransmissions. This also configures
     * the Upper Transport PDU information but points to local message parameters and opcode.
     */
    meshAccSendMsg(&(pAccPduPubInfo->utrAccPduTxInfo), &msgInfo, pMsgParam, msgParamLen,
                   pubNetKeyIndex, pubFriendshipCred);

    /* Configure retransmission parameters. */
    pAccPduPubInfo->publishRetransCount = pubRetransCount;
    pAccPduPubInfo->publishRetransSteps50Ms = pubRetransSteps50Ms;

    /* Modify Upper Transport PDU to point to allocated memory. */
    pAccPduPubInfo->utrAccPduTxInfo.pAccPduOpcode = ((uint8_t *)pAccPduPubInfo) +
                                                    sizeof(meshAccPduPubTxInfo_t);
    pAccPduPubInfo->utrAccPduTxInfo.pAccPduParam = pAccPduPubInfo->utrAccPduTxInfo.pAccPduOpcode +
                                                   pAccPduPubInfo->utrAccPduTxInfo.accPduOpcodeLen;

    /* Copy opcode at the end of the structure. */
    memcpy((uint8_t *)pAccPduPubInfo->utrAccPduTxInfo.pAccPduOpcode, &(msgInfo.opcode),
           pAccPduPubInfo->utrAccPduTxInfo.accPduOpcodeLen);
    /* Copy message params at the end of opcode. */
    memcpy((uint8_t *)pAccPduPubInfo->utrAccPduTxInfo.pAccPduParam, pMsgParam, msgParamLen);

    /* Enqueue publication info and PDU. */
    WsfQueueEnq(&(meshAccCb.pubRetransQueue), pAccPduPubInfo);

    /* Configure timer. */
    pAccPduPubInfo->retransTmr.msg.event = MESH_ACC_MSG_RETRANS_TMR_EXPIRED;
    pAccPduPubInfo->retransTmr.msg.param = meshAccCb.tmrUidGen++;
    pAccPduPubInfo->retransTmr.handlerId = meshCb.handlerId;

    /* Start timer. */
    WsfTimerStartMs(&(pAccPduPubInfo->retransTmr), RETRANS_STEPS_TO_MS_TIME(pubRetransSteps50Ms));
  }
  else
  {
    /* Trigger internal request with no allocated memory since it is not needed for retransmissions.
     */
    meshAccSendMsg(NULL, &msgInfo, pMsgParam, msgParamLen, pubNetKeyIndex, pubFriendshipCred);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Informs the Access Layer of the periodic publishing value of a model instance.
 *
 *  \param[in] elemId    Element identifier.
 *  \param[in] pModelId  Pointer to a model identifier.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshAccPeriodPubChanged(meshElementId_t elemId, meshModelId_t *pModelId)
{
  /* Check if feature is installed. */
  meshAccCb.ppChangedCback(elemId, pModelId);
}
