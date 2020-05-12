/*************************************************************************************************/
/*!
 *  \file   mesh_cfg_mdl_cl_main.c
 *
 *  \brief  Configuration Client module implementation.
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
#include "mesh_cfg_mdl_cl_api.h"

#include "mesh_cfg_mdl.h"
#include "mesh_cfg_mdl_cl.h"
#include "mesh_cfg_mdl_cl_main.h"
#include "mesh_cfg_mdl_messages.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/
/*! Map received status code to API error codes. */
#define CFG_CL_MAP_OTA_TO_ERR_CODE(status) \
   (status) = ((status) != MESH_CFG_MDL_CL_SUCCESS ? \
                  ((uint16_t)(status) + MESH_CFG_MDL_CL_REMOTE_ERROR_BASE > MESH_CFG_MDL_ERR_RFU_END ? \
                              MESH_CFG_MDL_ERR_RFU_END : \
                              ((status) + MESH_CFG_MDL_CL_REMOTE_ERROR_BASE)) \
                  : MESH_CFG_MDL_CL_SUCCESS)


/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Mesh Configuration Client operation response action table */
const meshCfgMdlClOpRspAct_t meshCfgMdlClOpRspActTbl[MESH_CFG_MDL_SR_MAX_OP] =
{
  meshCfgMdlClHandleBeaconStatus,
  meshCfgMdlClHandleCompDataStatus,
  meshCfgMdlClHandleDefaultTtlStatus,
  meshCfgHandleGattProxyStatus,
  meshCfgMdlClHandleRelayStatus,
  meshCfgMdlClHandleModelPubStatus,
  meshCfgMdlClHandleModelSubscrStatus,
  meshCfgMdlClHandleModelSubscrSigList,
  meshCfgMdlClHandleModelSubscrVendorList,
  meshCfgMdlClHandleNetKeyStatus,
  meshCfgMdlClHandleNetKeyList,
  meshCfgMdlClHandleAppKeyStatus,
  meshCfgMdlClHandleAppKeyList,
  meshCfgMdlClHandleNodeIdentityStatus,
  meshCfgMdlClHandleModelAppStatus,
  meshCfgMdlClHandleModelAppSigList,
  meshCfgMdlClHandleModelAppVendorList,
  meshCfgMdlClHandleNodeResetStatus,
  meshCfgMdlClHandleFriendStatus,
  meshCfgHandleKeyRefPhaseStatus,
  meshCfgMdlClHandleHbPubStatus,
  meshCfgMdlClHandleHbSubStatus,
  meshCfgMdlClHandleLpnPollTimeoutStatus,
  meshCfgMdlClHandleNwkTransStatus,
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Empty callback implementation for a completed procedure.
 *
 *  \param[in] pEvt  Pointer to Configuration Client event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlClEmptyCback(meshCfgMdlClEvt_t* pEvt)
{
  (void)pEvt;
  MESH_TRACE_ERR0("MESH CFG CL: User callback not registered!");
}

/*************************************************************************************************/
/*!
 *  \brief  Empty handler for an API WSF message.
 *
 *  \return None.
 */
/*************************************************************************************************/
void meshCfgMdlClEmptyHandler(wsfMsgHdr_t *pMsg)
{
  /* Free Request parameters. */
  WsfBufFree(((meshCfgMdlClOpReq_t *)pMsg)->pReqParam);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles an API Send WSF message.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshCfgMdlClApiSendMsgHandlerCback(wsfMsgHdr_t *pMsg)
{
  meshCfgMdlClOpReqParams_t *pReqParam = ((meshCfgMdlClOpReq_t *)pMsg)->pReqParam;

  /* Build Access Layer message information. */
  meshMsgInfo_t msgInfo =
  {
    .modelId.sigModelId = MESH_CFG_MDL_CL_MODEL_ID,
    .elementId = 0,
    .pDstLabelUuid = NULL,
    .appKeyIndex = MESH_APPKEY_INDEX_REMOTE_DEV_KEY,
    .ttl = MESH_USE_DEFAULT_TTL
  };

  /* Set opcode and destination address. */
  msgInfo.opcode  = meshCfgMdlClOpcodes[pReqParam->reqOpId];
  msgInfo.dstAddr = pReqParam->cfgMdlSrAddr;

  /* Check if address is local. */
  if (msgInfo.dstAddr == MESH_ADDR_TYPE_UNASSIGNED)
  {
    /* Read primary element address. */
    if (MeshLocalCfgGetAddrFromElementId(0, &msgInfo.dstAddr) != MESH_SUCCESS)
    {
      /* Local device is unprovisioned. */
      WsfBufFree(pReqParam);

      return;
    }
  }

  /* Configure timer for response. */
  pReqParam->rspTmr.msg.event = MESH_CFG_MDL_CL_MSG_RSP_TMR_EXPIRED;
  pReqParam->rspTmr.msg.param = meshCfgMdlClCb.rspTmrUidGen++;
  pReqParam->rspTmr.handlerId = meshCb.handlerId;

  /* Enqueue request parameters. */
  WsfQueueEnq(&(meshCfgMdlClCb.opQueue), pReqParam);

  /* Start operation timeout timer. */
  WsfTimerStartSec(&(pReqParam->rspTmr), meshCfgMdlClCb.opTimeoutSec);

  /* Send message. */
  MeshAccSendMessage((const meshMsgInfo_t *)&msgInfo, ((meshCfgMdlClOpReq_t *)pMsg)->pMsgParam,
                     ((meshCfgMdlClOpReq_t *)pMsg)->msgParamLen, pReqParam->cfgMdlSrNetKeyIndex, 0, 0);
}

/*************************************************************************************************/
/*!
 *  \brief     Operation timeout timer callback.
 *
 *  \param[in] tmrUid  Unique timer identifier
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshCfgMdlClRspTimeoutMsgHandlerCback(uint16_t tmrUid)
{
  meshCfgMdlClOpReqParams_t *pReqParams, *pPrevReqParams;
  meshCfgMdlHdr_t evt;

  /* Point to start of the queue */
  pReqParams = (meshCfgMdlClOpReqParams_t *)(meshCfgMdlClCb.opQueue.pHead);
  pPrevReqParams = NULL;

  /* Loop through queue. */
  while (pReqParams != NULL)
  {
    /* Check if timer UID matches. */
    if (pReqParams->rspTmr.msg.param == tmrUid)
    {
      /* Trigger user callback. */
      evt.hdr.event = MESH_CFG_MDL_CL_EVENT;
      evt.hdr.param = pReqParams->apiEvt;
      evt.hdr.status = MESH_CFG_MDL_CL_TIMEOUT;
      evt.peerAddress = pReqParams->cfgMdlSrAddr;

      meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

      /* Check if request is local. */
      if (!MESH_IS_ADDR_UNASSIGNED(pReqParams->cfgMdlSrAddr))
      {
        /* Free entry in the remote server database since timeout occured. */
        meshCfgMdlClRemFromSrDbSafe(pReqParams->cfgMdlSrAddr);
      }

      /* Remove from queue. */
      WsfQueueRemove(&(meshCfgMdlClCb.opQueue), pReqParams, pPrevReqParams);
      /* Free memory. */
      WsfBufFree(pReqParams);

      return;
    }
    /* Move to next request. */
    pPrevReqParams = pReqParams;
    pReqParams = (meshCfgMdlClOpReqParams_t *)(pReqParams->pNext);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles an API WSF message.
 *
 *  \param[in] pMsg  Pointer to Configuration Client API WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlClWsfMsgHandlerCback(wsfMsgHdr_t *pMsg)
{
  /* Check event type */
  switch(pMsg->event)
  {
    case MESH_CFG_MDL_CL_MSG_API_SEND:
      meshCfgMdlClApiSendMsgHandlerCback(pMsg);
      break;
    case MESH_CFG_MDL_CL_MSG_RSP_TMR_EXPIRED:
      meshCfgMdlClRspTimeoutMsgHandlerCback(pMsg->param);
      break;
    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Callback implementation for receiving Access Layer messages for this core model.
 *
 *  \param[in] opcodeIdx    Index of the opcode in the receive opcodes array registered.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] src          Address of the element originating the request.
 *  \param[in] elemId       Destination element identifier.
 *  \param[in] ttl          TTL of the received message.
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet on which the message
 *                          is received
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshCfgMdlClAccMsgRcvCback(uint8_t opcodeIdx, uint8_t *pMsgParam,
                                uint16_t msgParamLen, meshAddress_t src,
                                meshElementId_t elemId, uint8_t ttl,
                                uint16_t netKeyIndex)
{
  meshCfgMdlClOpReqParams_t *pReqParams, *pPrevReqParams;
  meshAddress_t elem0Addr;

  /* Read element 0 address. */
  MeshLocalCfgGetAddrFromElementId(0, &elem0Addr);

  /* Check if response is from local device. */
  if (src == elem0Addr)
  {
    /* Set source address of message to unassigned to match internal requests. */
    src = MESH_ADDR_TYPE_UNASSIGNED;
  }

  /* Point to start of the queue */
  pReqParams = (meshCfgMdlClOpReqParams_t *)(meshCfgMdlClCb.opQueue.pHead);
  pPrevReqParams = NULL;

  /* Loop through queue. */
  while (pReqParams != NULL)
  {
    /* Check received message opcode matches expected response opcode and validate additional
     * requirements.
     */
    if((opcodeIdx != pReqParams->rspOpId) || (src != pReqParams->cfgMdlSrAddr) ||
       (netKeyIndex != pReqParams->cfgMdlSrNetKeyIndex) || (elemId != 0))
    {
      /* Move to next request. */
      pPrevReqParams = pReqParams;
      pReqParams = (meshCfgMdlClOpReqParams_t *)(pReqParams->pNext);
    }
    else
    {
      /* Call corresponding action function in table and check if processed successfully. */
      if (!meshCfgMdlClOpRspActTbl[pReqParams->rspOpId](pReqParams, pMsgParam, msgParamLen))
      {
        /* Move to next request. */
        pPrevReqParams = pReqParams;
        pReqParams = (meshCfgMdlClOpReqParams_t *)(pReqParams->pNext);
      }
      else
      {
        /* Check if request is local. */
        if (!MESH_IS_ADDR_UNASSIGNED(pReqParams->cfgMdlSrAddr))
        {
          /* Free entry in the remote server database since request was handled. */
          meshCfgMdlClRemFromSrDbSafe(pReqParams->cfgMdlSrAddr);
        }

        /* Stop timer. */
        WsfTimerStop(&(pReqParams->rspTmr));

        /* Remove from queue. */
        WsfQueueRemove(&(meshCfgMdlClCb.opQueue), pReqParams, pPrevReqParams);

        /* Free memory. */
        WsfBufFree(pReqParams);

        return;
      }
    }
  }

  (void)ttl;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Beacon Status response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgMdlClHandleBeaconStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                      uint16_t msgParamLen)
{
  meshCfgMdlBeaconStateEvt_t evt;

  /* Validate parameters */
  if ((msgParamLen != CFG_MDL_MSG_BEACON_STATE_NUM_BYTES) || (pMsgParam == NULL))
  {
    return FALSE;
  }

  /* Unpack message. */
  evt.state = pMsgParam[0];

  /* Validate unpacked parameters. */
  if (!MESH_BEACON_STATE_IS_VALID(evt.state))
  {
    return FALSE;
  }

  /* Set event type and address. */
  evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_CL_EVENT;
  evt.cfgMdlHdr.hdr.param = pReqParam->apiEvt;
  evt.cfgMdlHdr.hdr.status = MESH_CFG_MDL_CL_SUCCESS;
  evt.cfgMdlHdr.peerAddress = pReqParam->cfgMdlSrAddr;

  /* Trigger user callback. */
  meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Composition Data Status response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgMdlClHandleCompDataStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                        uint16_t msgParamLen)
{
  meshCfgMdlCompDataEvt_t evt;

  /* Validate parameters */
  if ((msgParamLen < CFG_MDL_MSG_COMP_DATA_STATE_NUM_BYTES) || (pMsgParam == NULL))
  {
    return FALSE;
  }

  /* Unpack message. */
  meshCfgMsgUnpackCompData(pMsgParam, (msgParamLen - CFG_MDL_MSG_COMP_DATA_STATE_NUM_BYTES), &evt.data);

  /* Validate for Page 0 that length accomodates at least one empty element and the page header.
   */
  if((evt.data.pageNumber == 0) &&
     (evt.data.pageSize < CFG_MDL_MSG_COMP_DATA_PG0_EMPTY_NUM_BYTES +
                          CFG_MDL_MSG_COMP_DATA_PG0_ELEM_HDR_NUM_BYTES))
  {
    return FALSE;
  }

  /* Set event type and address. */
  evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_CL_EVENT;
  evt.cfgMdlHdr.hdr.param = pReqParam->apiEvt;
  evt.cfgMdlHdr.hdr.status = MESH_CFG_MDL_CL_SUCCESS;
  evt.cfgMdlHdr.peerAddress = pReqParam->cfgMdlSrAddr;

  /* Trigger user callback. */
  meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Default TTL Status response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgMdlClHandleDefaultTtlStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                          uint16_t msgParamLen)
{
  meshCfgMdlDefaultTtlStateEvt_t evt;

  /* Validate parameters */
  if ((msgParamLen != CFG_MDL_MSG_DEFAULT_TTL_STATE_NUM_BYTES) || (pMsgParam == NULL))
  {
    return FALSE;
  }

  /* Unpack message. */
  evt.ttl = pMsgParam[0];

  /* Validate unpacked parameters. */
  if (!MESH_TTL_IS_VALID(evt.ttl) || (evt.ttl == MESH_TX_TTL_FILTER_VALUE) ||
      (evt.ttl == MESH_USE_DEFAULT_TTL))
  {
    return FALSE;
  }

  /* Set event type and address. */
  evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_CL_EVENT;
  evt.cfgMdlHdr.hdr.param = pReqParam->apiEvt;
  evt.cfgMdlHdr.hdr.status = MESH_CFG_MDL_CL_SUCCESS;
  evt.cfgMdlHdr.peerAddress = pReqParam->cfgMdlSrAddr;

  /* Trigger user callback. */
  meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Gatt Proxy Status response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgHandleGattProxyStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                    uint16_t msgParamLen)
{
  meshCfgMdlGattProxyEvt_t evt;

  /* Validate parameters */
  if ((msgParamLen != CFG_MDL_MSG_GATT_PROXY_STATE_NUM_BYTES) || (pMsgParam == NULL))
  {
    return FALSE;
  }

  /* Unpack message. */
  evt.gattProxy = pMsgParam[0];

  /* Validate unpacked parameters. */
  if (evt.gattProxy >= MESH_GATT_PROXY_FEATURE_PROHIBITED_START)
  {
    return FALSE;
  }

  /* Set event type and address. */
  evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_CL_EVENT;
  evt.cfgMdlHdr.hdr.param = pReqParam->apiEvt;
  evt.cfgMdlHdr.hdr.status = MESH_CFG_MDL_CL_SUCCESS;
  evt.cfgMdlHdr.peerAddress = pReqParam->cfgMdlSrAddr;

  /* Trigger user callback. */
  meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Relay Status response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgMdlClHandleRelayStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                  uint16_t msgParamLen)
{
  meshCfgMdlRelayCompositeStateEvt_t evt;

  /* Validate parameters */
  if ((msgParamLen != CFG_MDL_MSG_RELAY_COMP_STATE_NUM_BYTES) || (pMsgParam == NULL))
  {
    return FALSE;
  }

  /* Unpack message. */
  meshCfgMsgUnpackRelay(pMsgParam, &evt.relayState, &evt.relayRetrans);

  /* Validate unpacked parameters. */
  if (evt.relayState >= MESH_RELAY_FEATURE_PROHIBITED_START)
  {
    return FALSE;
  }

  /* Set event type and address. */
  evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_CL_EVENT;
  evt.cfgMdlHdr.hdr.param = pReqParam->apiEvt;
  evt.cfgMdlHdr.hdr.status = MESH_CFG_MDL_CL_SUCCESS;
  evt.cfgMdlHdr.peerAddress = pReqParam->cfgMdlSrAddr;

  /* Trigger user callback. */
  meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Model Publication Status response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgMdlClHandleModelPubStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                        uint16_t msgParamLen)
{
  meshCfgMdlModelPubEvt_t evt;

  /* Validate parameters */
  if (pMsgParam == NULL)
  {
    return FALSE;
  }

  /* Validate length and determine model type. */
  if (msgParamLen != CFG_MDL_MSG_MODEL_PUB_STATUS_NUM_BYTES(TRUE))
  {
    if (msgParamLen != CFG_MDL_MSG_MODEL_PUB_STATUS_NUM_BYTES(FALSE))
    {
      return FALSE;
    }
    else
    {
      evt.isSig = FALSE;
    }
  }
  else
  {
    evt.isSig = TRUE;
  }

  /* Get OTA status. */
  BSTREAM_TO_UINT8(evt.cfgMdlHdr.hdr.status, pMsgParam);

  /* Map to client OTA codes. */
  CFG_CL_MAP_OTA_TO_ERR_CODE(evt.cfgMdlHdr.hdr.status);

  /* Get element address. */
  BSTREAM_TO_UINT16(evt.elemAddr, pMsgParam);

  /* Validate unpacked data. */
  if (!MESH_IS_ADDR_UNICAST(evt.elemAddr))
  {
    return FALSE;
  }

  /* Get publish address. */
  BSTREAM_TO_UINT16(evt.pubAddr, pMsgParam);

  /* Get publication parameters. */
  meshCfgMsgUnpackModelPubParam(pMsgParam, &(evt.pubParams), &(evt.modelId.sigModelId),
                                &(evt.modelId.vendorModelId), evt.isSig);

  /* Set event type and address. */
  evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_CL_EVENT;
  evt.cfgMdlHdr.hdr.param = pReqParam->apiEvt;
  evt.cfgMdlHdr.peerAddress = pReqParam->cfgMdlSrAddr;

  /* Trigger user callback. */
  meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Model Subscription Status response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgMdlClHandleModelSubscrStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                           uint16_t msgParamLen)
{
  meshCfgMdlModelSubscrChgEvt_t evt;

   /* Validate parameters */
  if (pMsgParam == NULL)
  {
    return FALSE;
  }

  /* Validate length and determine model type. */
  if (msgParamLen != CFG_MDL_MSG_MODEL_SUBSCR_STATUS_NUM_BYTES(TRUE))
  {
    if (msgParamLen != CFG_MDL_MSG_MODEL_SUBSCR_STATUS_NUM_BYTES(FALSE))
    {
      return FALSE;
    }
    else
    {
      evt.isSig = FALSE;
    }
  }
  else
  {
    evt.isSig = TRUE;
  }

  /* Get OTA status. */
  BSTREAM_TO_UINT8(evt.cfgMdlHdr.hdr.status, pMsgParam);

  /* Map to client OTA codes. */
  CFG_CL_MAP_OTA_TO_ERR_CODE(evt.cfgMdlHdr.hdr.status);

  /* Get element address. */
  BSTREAM_TO_UINT16(evt.elemAddr, pMsgParam);

  /* Validate unpacked data. */
  if (!MESH_IS_ADDR_UNICAST(evt.elemAddr))
  {
    return FALSE;
  }
  /* Get subscription address. */
  BSTREAM_TO_UINT16(evt.subscrAddr, pMsgParam);

  /* Get model id. */
  if (evt.isSig)
  {
    BSTREAM_TO_UINT16(evt.modelId.sigModelId, pMsgParam);
  }
  else
  {
    BSTREAM_TO_VEND_MDL(evt.modelId.vendorModelId, pMsgParam);
  }

  /* Set event type and address. */
  evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_CL_EVENT;
  evt.cfgMdlHdr.hdr.param = pReqParam->apiEvt;
  evt.cfgMdlHdr.peerAddress = pReqParam->cfgMdlSrAddr;

  /* Trigger user callback. */
  meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Common handler for the SIG/Vendor Model Subscription List response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] isSig        TRUE if model is SIG, FALSE otherwise.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshCfgMdlClHandleModelSubscrList(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                                uint16_t msgParamLen, bool_t isSig)
{
  uint8_t idx = 0;
  meshCfgMdlModelSubscrListEvt_t evt;

   /* Validate parameters */
  if (pMsgParam == NULL)
  {
    return FALSE;
  }

  /* Validate length and determine model type. */
  if (msgParamLen < CFG_MDL_MSG_MODEL_SUBSCR_LIST_EMPTY_NUM_BYTES(isSig))
  {
    return FALSE;
  }

  /* Get OTA status. */
  BSTREAM_TO_UINT8(evt.cfgMdlHdr.hdr.status, pMsgParam);

  /* Map to client OTA codes. */
  CFG_CL_MAP_OTA_TO_ERR_CODE(evt.cfgMdlHdr.hdr.status);

  /* Get element address. */
  BSTREAM_TO_UINT16(evt.elemAddr, pMsgParam);

  /* Validate unpacked data. */
  if (!MESH_IS_ADDR_UNICAST(evt.elemAddr))
  {
    return FALSE;
  }

  evt.isSig = isSig;

  /* Get model id. */
  if (isSig)
  {
    BSTREAM_TO_UINT16(evt.modelId.sigModelId, pMsgParam);
  }
  else
  {
    BSTREAM_TO_VEND_MDL(evt.modelId.vendorModelId, pMsgParam);
  }

  /* Get subscription list size. */

  /* Validate length. */
  if ((msgParamLen - CFG_MDL_MSG_MODEL_SUBSCR_LIST_EMPTY_NUM_BYTES(isSig)) & 0x01)
  {
    return FALSE;
  }

  /* Get number of addresses. */
  evt.subscrListSize = (uint8_t)((msgParamLen - CFG_MDL_MSG_MODEL_SUBSCR_LIST_EMPTY_NUM_BYTES(isSig))
                                  >> 1);

  /* Check if empty list or error code. */
  if ((evt.subscrListSize == 0) || (evt.cfgMdlHdr.hdr.status != MESH_CFG_MDL_CL_SUCCESS))
  {
    evt.pSubscrList = NULL;
    evt.subscrListSize = 0;
  }
  else
  {
    /* Allocate memory for the subscription list. */
    if ((evt.pSubscrList = WsfBufAlloc(evt.subscrListSize * sizeof(meshAddress_t))) != NULL)
    {
      for (idx = 0; idx < evt.subscrListSize; idx++)
      {
        BSTREAM_TO_UINT16(evt.pSubscrList[idx], pMsgParam);

        /* Validate unpacked addresses. */
        if(MESH_IS_ADDR_UNASSIGNED(evt.pSubscrList[idx]) ||
           MESH_IS_ADDR_UNICAST(evt.pSubscrList[idx]) ||
           evt.pSubscrList[idx] == MESH_ADDR_GROUP_ALL)
        {
          WsfBufFree(evt.pSubscrList);
          return FALSE;
        }
      }
    }
    else
    {
      /* Set status to out of resources and list size to 0. */
      evt.cfgMdlHdr.hdr.status = MESH_CFG_MDL_CL_OUT_OF_RESOURCES;
      evt.subscrListSize = 0;
      evt.pSubscrList = NULL;
    }
  }

  /* Set event type and address. */
  evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_CL_EVENT;
  evt.cfgMdlHdr.hdr.param = pReqParam->apiEvt;
  evt.cfgMdlHdr.peerAddress = pReqParam->cfgMdlSrAddr;

  /* Trigger user callback. */
  meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

  /* Free memory. */
  if (evt.pSubscrList)
  {
    WsfBufFree(evt.pSubscrList);
  }

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the SIG Model Subscription List response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgMdlClHandleModelSubscrSigList(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                            uint16_t msgParamLen)
{
  /* Call common handler. */
  return meshCfgMdlClHandleModelSubscrList(pReqParam, pMsgParam, msgParamLen, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Vendor Model Subscription List response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgMdlClHandleModelSubscrVendorList(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                               uint16_t msgParamLen)
{
  /* Call common handler. */
  return meshCfgMdlClHandleModelSubscrList(pReqParam, pMsgParam, msgParamLen, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the NetKey Status response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgMdlClHandleNetKeyStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                      uint16_t msgParamLen)
{
  meshCfgMdlNetKeyChgEvt_t evt;

  /* Validate parameters */
  if ((msgParamLen != CFG_MDL_MSG_NETKEY_STATUS_NUM_BYTES) || (pMsgParam == NULL))
  {
    return FALSE;
  }

  /* Get OTA status. */
  BSTREAM_TO_UINT8(evt.cfgMdlHdr.hdr.status, pMsgParam);

  /* Map to client OTA codes. */
  CFG_CL_MAP_OTA_TO_ERR_CODE(evt.cfgMdlHdr.hdr.status);

  /* Unpack NetKeyIndex. */
  meshCfgMsgUnpackSingleKeyIndex(pMsgParam, &evt.netKeyIndex);

  /* Set event type and address. */
  evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_CL_EVENT;
  evt.cfgMdlHdr.hdr.param = pReqParam->apiEvt;
  evt.cfgMdlHdr.peerAddress = pReqParam->cfgMdlSrAddr;

  /* Trigger user callback. */
  meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the NetKey List response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgMdlClHandleNetKeyList(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                    uint16_t msgParamLen)
{
  meshCfgMdlNetKeyListEvt_t evt;
  uint8_t idx;

  /* Validate parameters */
  if ((!CFG_MDL_MSG_NETKEY_LIST_SIZE_VALID(msgParamLen)) || (pMsgParam == NULL))
  {
    return FALSE;
  }

  /* Extract number of NetKeyIndexes. */
  evt.netKeyList.netKeyCount = (uint8_t)CFG_MDL_MSG_NETKEY_LIST_TO_NUM_NETKEY(msgParamLen);

  /* Attempt to allocate memory for the key list. */
  if ((evt.netKeyList.pNetKeyIndexes = (uint16_t *)WsfBufAlloc(evt.netKeyList.netKeyCount * sizeof(uint16_t)))
      != NULL)
  {
    /* Start unpacking in pairs. If number is even, stop before last. */
    for (idx = 0; idx < (evt.netKeyList.netKeyCount & (~0x01)); idx+=2)
    {
      pMsgParam += meshCfgMsgUnpackTwoKeyIndex(pMsgParam, (evt.netKeyList.pNetKeyIndexes + idx),
                                               (evt.netKeyList.pNetKeyIndexes + idx + 1));
    }
    /* Check if there is one more remaining. */
    if (evt.netKeyList.netKeyCount & 0x01)
    {
      (void)meshCfgMsgUnpackSingleKeyIndex(pMsgParam, (evt.netKeyList.pNetKeyIndexes + idx));
    }
    /* Set status to success. */
    evt.cfgMdlHdr.hdr.status = MESH_CFG_MDL_CL_SUCCESS;
  }
  else
  {
    /* Signal out of resources to unpack the NetKeyIndex list. */
    evt.cfgMdlHdr.hdr.status = MESH_CFG_MDL_CL_OUT_OF_RESOURCES;
  }
  /* Set event type and address. */
  evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_CL_EVENT;
  evt.cfgMdlHdr.hdr.param = pReqParam->apiEvt;
  evt.cfgMdlHdr.peerAddress = pReqParam->cfgMdlSrAddr;

  /* Trigger user callback. */
  meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

  /* Check if memory is allocated and free. */
  if (evt.netKeyList.pNetKeyIndexes)
  {
    WsfBufFree(evt.netKeyList.pNetKeyIndexes);
  }

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the AppKey Status response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgMdlClHandleAppKeyStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                      uint16_t msgParamLen)
{
  meshCfgMdlAppKeyChgEvt_t evt;

  /* Validate parameters */
  if ((msgParamLen != CFG_MDL_MSG_APPKEY_STATUS_NUM_BYTES) || (pMsgParam == NULL))
  {
    return FALSE;
  }

  /* Get OTA status. */
  BSTREAM_TO_UINT8(evt.cfgMdlHdr.hdr.status, pMsgParam);

  /* Map to client OTA codes. */
  CFG_CL_MAP_OTA_TO_ERR_CODE(evt.cfgMdlHdr.hdr.status);

  /* Unpack key bind. */
  (void)meshCfgMsgUnpackTwoKeyIndex(pMsgParam, &evt.bind.netKeyIndex, &evt.bind.appKeyIndex);

  /* Set event type and address. */
  evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_CL_EVENT;
  evt.cfgMdlHdr.hdr.param = pReqParam->apiEvt;
  evt.cfgMdlHdr.peerAddress = pReqParam->cfgMdlSrAddr;

  /* Trigger user callback. */
  meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the AppKey List response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgMdlClHandleAppKeyList(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                    uint16_t msgParamLen)
{
  meshCfgMdlAppKeyListEvt_t evt;
  uint8_t idx;

  /* Validate parameters */
  if ((!CFG_MDL_MSG_APPKEY_LIST_SIZE_VALID(msgParamLen)) || (pMsgParam == NULL))
  {
    return FALSE;
  }

  /* Get OTA status. */
  BSTREAM_TO_UINT8(evt.cfgMdlHdr.hdr.status, pMsgParam);

  /* Map to client OTA codes. */
  CFG_CL_MAP_OTA_TO_ERR_CODE(evt.cfgMdlHdr.hdr.status);

  /* Unpack NetKeyIndex. */
  pMsgParam += meshCfgMsgUnpackSingleKeyIndex(pMsgParam, &evt.appKeyList.netKeyIndex);

  /* Extract number of AppKeyIndexes. */
  evt.appKeyList.appKeyCount = (uint8_t)CFG_MDL_MSG_APPKEY_LIST_TO_NUM_APPKEY(msgParamLen);

  /* Check OTA status. */
  if ((evt.cfgMdlHdr.hdr.status != MESH_CFG_MDL_CL_SUCCESS) || (evt.appKeyList.appKeyCount == 0))
  {
    /* Set list to empty. */
    evt.appKeyList.appKeyCount = 0;
    evt.appKeyList.pAppKeyIndexes = NULL;
  }
  else
  {
    /* Attempt to allocate memory for the key list. */
    if ((evt.appKeyList.pAppKeyIndexes = (uint16_t *)WsfBufAlloc(evt.appKeyList.appKeyCount * sizeof(uint16_t)))
        != NULL)
    {
      /* Start unpacking in pairs. If number is even, stop before last. */
      for (idx = 0; idx < (evt.appKeyList.appKeyCount & (~0x01)); idx+=2)
      {
        pMsgParam += meshCfgMsgUnpackTwoKeyIndex(pMsgParam, (evt.appKeyList.pAppKeyIndexes + idx),
                                                 (evt.appKeyList.pAppKeyIndexes + idx + 1));
      }
      /* Check if there is one more remaining. */
      if (evt.appKeyList.appKeyCount & 0x01)
      {
        (void)meshCfgMsgUnpackSingleKeyIndex(pMsgParam, (evt.appKeyList.pAppKeyIndexes + idx));
      }
    }
    else
    {
      /* Signal out of resources to unpack the AppKeyIndex list. */
      evt.cfgMdlHdr.hdr.status = MESH_CFG_MDL_CL_OUT_OF_RESOURCES;
    }
  }

  /* Set event type and address. */
  evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_CL_EVENT;
  evt.cfgMdlHdr.hdr.param = pReqParam->apiEvt;
  evt.cfgMdlHdr.peerAddress = pReqParam->cfgMdlSrAddr;

  /* Trigger user callback. */
  meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

  /* Check if memory is allocated and free. */
  if (evt.appKeyList.pAppKeyIndexes)
  {
    WsfBufFree(evt.appKeyList.pAppKeyIndexes);
  }

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Node Identity Status response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgMdlClHandleNodeIdentityStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                            uint16_t msgParamLen)
{
  meshCfgMdlNodeIdentityEvt_t evt;
  /* Validate parameters */
  if ((msgParamLen != CFG_MDL_MSG_NODE_IDENTITY_STATUS_NUM_BYTES) || (pMsgParam == NULL))
  {
    return FALSE;
  }

  /* Get OTA status. */
  BSTREAM_TO_UINT8(evt.cfgMdlHdr.hdr.status, pMsgParam);

  /* Map to client OTA codes. */
  CFG_CL_MAP_OTA_TO_ERR_CODE(evt.cfgMdlHdr.hdr.status);

  /* Unpack NetKey Index. */
  pMsgParam += meshCfgMsgUnpackSingleKeyIndex(pMsgParam, &evt.netKeyIndex);

  /* Unpack state. */
  BSTREAM_TO_UINT8(evt.state, pMsgParam);

  /* Verify unpacked parameters. */
  if ((evt.state >= MESH_NODE_IDENTITY_PROHIBITED_START))
  {
    return FALSE;
  }

  /* Set event type and address. */
  evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_CL_EVENT;
  evt.cfgMdlHdr.hdr.param = pReqParam->apiEvt;
  evt.cfgMdlHdr.peerAddress = pReqParam->cfgMdlSrAddr;

  /* Trigger user callback. */
  meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Model App Status response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgMdlClHandleModelAppStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                        uint16_t msgParamLen)
{
  meshCfgMdlModelAppBindEvt_t evt;

  /* Validate parameters */
  if (pMsgParam == NULL)
  {
    return FALSE;
  }

  /* Validate length and determine model type. */
  if (msgParamLen != CFG_MDL_MSG_MODEL_APP_STATUS_NUM_BYTES(TRUE))
  {
    if (msgParamLen != CFG_MDL_MSG_MODEL_APP_STATUS_NUM_BYTES(FALSE))
    {
      return FALSE;
    }
    else
    {
      evt.isSig = FALSE;
    }
  }
  else
  {
    evt.isSig = TRUE;
  }

  /* Get OTA status. */
  BSTREAM_TO_UINT8(evt.cfgMdlHdr.hdr.status, pMsgParam);

  /* Map to client OTA codes. */
  CFG_CL_MAP_OTA_TO_ERR_CODE(evt.cfgMdlHdr.hdr.status);

  /* Unpack Model App Bind. */
  meshCfgMsgUnpackModelAppBind(pMsgParam, &evt.elemAddr, &evt.appKeyIndex, &evt.modelId.sigModelId,
                               &evt.modelId.vendorModelId, evt.isSig);

  /* Validate unpacked data. */
  if (!MESH_IS_ADDR_UNICAST(evt.elemAddr))
  {
    return FALSE;
  }

  /* Set event type and address. */
  evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_CL_EVENT;
  evt.cfgMdlHdr.hdr.param = pReqParam->apiEvt;
  evt.cfgMdlHdr.peerAddress = pReqParam->cfgMdlSrAddr;

  /* Trigger user callback. */
  meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Common handler for the SIG/Vendor Model App List response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *  \param[in] isSig        TRUE if message is Model App SIG List, FALSE otherwise.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshCfgMdlClHandleModelAppList(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                             uint16_t msgParamLen, bool_t isSig)
{
  meshCfgMdlModelAppListEvt_t evt;
  uint8_t idx;

  /* Validate parameters */
  if ((!CFG_MDL_MSG_MODEL_APP_LIST_SIZE_VALID(isSig, msgParamLen)) || (pMsgParam == NULL))
  {
    return FALSE;
  }

  /* Get OTA status. */
  BSTREAM_TO_UINT8(evt.cfgMdlHdr.hdr.status, pMsgParam);

  /* Map to client OTA codes. */
  CFG_CL_MAP_OTA_TO_ERR_CODE(evt.cfgMdlHdr.hdr.status);

  /* Unpack element address. */
  BSTREAM_TO_UINT16(evt.modelAppList.elemAddr, pMsgParam);

  /* Check address is unicast. */
  if (!MESH_IS_ADDR_UNICAST(evt.modelAppList.elemAddr))
  {
    return FALSE;
  }

  /* Set model type. */
  evt.modelAppList.isSig = isSig;

  /* Unpack model identifier. */
  if (isSig)
  {
    BSTREAM_TO_UINT16(evt.modelAppList.modelId.sigModelId, pMsgParam);
  }
  else
  {
    BSTREAM_TO_VEND_MDL(evt.modelAppList.modelId.vendorModelId, pMsgParam);
  }

  /* Extract number of AppKeyIndexes. */
  evt.modelAppList.appKeyCount =
    (uint8_t)CFG_MDL_MSG_MODEL_APP_LIST_TO_NUM_APPKEY(isSig, msgParamLen);

  /* Check OTA status. */
  if ((evt.cfgMdlHdr.hdr.status != MESH_CFG_MDL_CL_SUCCESS) || (evt.modelAppList.appKeyCount == 0))
  {
    /* Set list to empty. */
    evt.modelAppList.appKeyCount = 0;
    evt.modelAppList.pAppKeyIndexes = NULL;
  }
  else
  {
    /* Attempt to allocate memory for the key list. */
    if ((evt.modelAppList.pAppKeyIndexes =
         (uint16_t *)WsfBufAlloc(evt.modelAppList.appKeyCount * sizeof(uint16_t))) != NULL)
    {
      /* Start unpacking in pairs. If number is even, stop before last. */
      for (idx = 0; idx < (evt.modelAppList.appKeyCount & (~0x01)); idx+=2)
      {
        pMsgParam += meshCfgMsgUnpackTwoKeyIndex(pMsgParam, (evt.modelAppList.pAppKeyIndexes + idx),
                                                 (evt.modelAppList.pAppKeyIndexes + idx + 1));
      }
      /* Check if there is one more remaining. */
      if (evt.modelAppList.appKeyCount & 0x01)
      {
        (void)meshCfgMsgUnpackSingleKeyIndex(pMsgParam, (evt.modelAppList.pAppKeyIndexes + idx));
      }
    }
    else
    {
      /* Signal out of resources to unpack the AppKeyIndex list. */
      evt.cfgMdlHdr.hdr.status = MESH_CFG_MDL_CL_OUT_OF_RESOURCES;
    }
  }

  /* Set event type and address. */
  evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_CL_EVENT;
  evt.cfgMdlHdr.hdr.param = pReqParam->apiEvt;
  evt.cfgMdlHdr.peerAddress = pReqParam->cfgMdlSrAddr;

  /* Trigger user callback. */
  meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

  /* Check if memory is allocated and free. */
  if (evt.modelAppList.pAppKeyIndexes)
  {
    WsfBufFree(evt.modelAppList.pAppKeyIndexes);
  }

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the SIG Model App List response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgMdlClHandleModelAppSigList(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                         uint16_t msgParamLen)
{
  return meshCfgMdlClHandleModelAppList(pReqParam, pMsgParam, msgParamLen, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Vendor Model App List response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgMdlClHandleModelAppVendorList(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                            uint16_t msgParamLen)
{
  return meshCfgMdlClHandleModelAppList(pReqParam, pMsgParam, msgParamLen, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Node Reset Status response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgMdlClHandleNodeResetStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                         uint16_t msgParamLen)
{
  (void)pMsgParam;
  meshCfgMdlNodeResetStateEvt_t evt;

  /* Validate parameters */
  if ((msgParamLen != CFG_MDL_MSG_NODE_RESET_STATE_NUM_BYTES))
  {
    return FALSE;
  }

  /* Set event type and address. */
  evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_CL_EVENT;
  evt.cfgMdlHdr.hdr.param = pReqParam->apiEvt;
  evt.cfgMdlHdr.hdr.status = MESH_CFG_MDL_CL_SUCCESS;
  evt.cfgMdlHdr.peerAddress = pReqParam->cfgMdlSrAddr;

  /* Trigger user callback. */
  meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Friend Status response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgMdlClHandleFriendStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                      uint16_t msgParamLen)
{
  meshCfgMdlFriendEvt_t evt;

  /* Validate parameters */
  if ((msgParamLen != CFG_MDL_MSG_FRIEND_STATE_NUM_BYTES) || (pMsgParam == NULL))
  {
    return FALSE;
  }

  /* Unpack message. */
  evt.friendState = pMsgParam[0];

  /* Validate unpacked parameters. */
  if (evt.friendState >= MESH_FRIEND_FEATURE_PROHIBITED_START)
  {
    return FALSE;
  }

  /* Set event type and address. */
  evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_CL_EVENT;
  evt.cfgMdlHdr.hdr.param = pReqParam->apiEvt;
  evt.cfgMdlHdr.hdr.status = MESH_CFG_MDL_CL_SUCCESS;
  evt.cfgMdlHdr.peerAddress = pReqParam->cfgMdlSrAddr;

  /* Trigger user callback. */
  meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Key Refresh Phase Status response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgHandleKeyRefPhaseStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                      uint16_t msgParamLen)
{
  meshCfgMdlKeyRefPhaseEvt_t evt;

  /* Validate parameters */
  if ((msgParamLen != CFG_MDL_MSG_KEY_REF_PHASE_STATUS_NUM_BYTES) || (pMsgParam == NULL))
  {
    return FALSE;
  }

  /* Get OTA status. */
  BSTREAM_TO_UINT8(evt.cfgMdlHdr.hdr.status, pMsgParam);

  /* Map to client OTA codes. */
  CFG_CL_MAP_OTA_TO_ERR_CODE(evt.cfgMdlHdr.hdr.status);

  /* Unpack NetKeyIndex. */
  pMsgParam += meshCfgMsgUnpackSingleKeyIndex(pMsgParam, &evt.netKeyIndex);

  /* Unpack Key Refresh Phase State. */
  BSTREAM_TO_UINT8(evt.keyRefState, pMsgParam);

  /* Validate unpacked parameters. */
  if (evt.keyRefState >= MESH_KEY_REFRESH_PROHIBITED_START)
  {
    return FALSE;
  }

  /* Set event type and address. */
  evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_CL_EVENT;
  evt.cfgMdlHdr.hdr.param = pReqParam->apiEvt;
  evt.cfgMdlHdr.peerAddress = pReqParam->cfgMdlSrAddr;

  /* Trigger user callback. */
  meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Heartbeat Publication Status response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgMdlClHandleHbPubStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                     uint16_t msgParamLen)
{
  meshCfgMdlHbPubEvt_t evt;

  /* Validate parameters */
  if ((msgParamLen != CFG_MDL_MSG_HB_PUB_STATUS_NUM_BYTES) || (pMsgParam == NULL))
  {
    return FALSE;
  }

  /* Unpack status. */
  BSTREAM_TO_UINT8(evt.cfgMdlHdr.hdr.status, pMsgParam);

  /* Map to client OTA codes. */
  CFG_CL_MAP_OTA_TO_ERR_CODE(evt.cfgMdlHdr.hdr.status);

  /* Unpack Heartbeat Publication state. */
  meshCfgMsgUnpackHbPub(pMsgParam, &evt.hbPub);

  /* On success, validate fields. */
  if (evt.cfgMdlHdr.hdr.status == MESH_CFG_MDL_CL_SUCCESS)
  {
    /* Validate unpacked Heartbeat Publication data. */
    if (((evt.hbPub.countLog >= CFG_MDL_HB_PUB_COUNT_LOG_NOT_ALLOW_START) &&
        (evt.hbPub.countLog <= CFG_MDL_HB_PUB_COUNT_LOG_NOT_ALLOW_END)) ||
        (evt.hbPub.periodLog >= CFG_MDL_HB_PUB_PERIOD_LOG_NOT_ALLOW_START) ||
        (evt.hbPub.ttl >= CFG_MDL_HB_PUB_TTL_NOT_ALLOW_START) ||
        (MESH_IS_ADDR_VIRTUAL(evt.hbPub.dstAddr)) ||
        (evt.hbPub.netKeyIndex > MESH_NET_KEY_INDEX_MAX_VAL))
    {
      return FALSE;
    }
  }

  /* Clear RFU bits. */
  evt.hbPub.features &= (MESH_FEAT_RFU_START - 1);

  /* Set event type and address. */
  evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_CL_EVENT;
  evt.cfgMdlHdr.hdr.param = pReqParam->apiEvt;
  evt.cfgMdlHdr.peerAddress = pReqParam->cfgMdlSrAddr;

  /* Trigger user callback. */
  meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Heartbeat Subcription Status response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgMdlClHandleHbSubStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                     uint16_t msgParamLen)
{
  meshCfgMdlHbSubEvt_t evt;

  /* Validate parameters */
  if ((msgParamLen != CFG_MDL_MSG_HB_SUB_STATE_NUM_BYTES) || (pMsgParam == NULL))
  {
    return FALSE;
  }

  /* Unpack status. */
  BSTREAM_TO_UINT8(evt.cfgMdlHdr.hdr.status, pMsgParam);

  /* Map to client OTA codes. */
  CFG_CL_MAP_OTA_TO_ERR_CODE(evt.cfgMdlHdr.hdr.status);

  /* Unpack Heartbeat Subscription state. */
  meshCfgMsgUnpackHbSubState(pMsgParam, &evt.hbSub);

  if(evt.cfgMdlHdr.hdr.status == MESH_CFG_MDL_CL_SUCCESS)
  {
    /* Validate unpacked parameters. */
    if (((evt.hbSub.countLog >= CFG_MDL_HB_SUB_COUNT_LOG_NOT_ALLOW_START) &&
        (evt.hbSub.countLog <= CFG_MDL_HB_SUB_COUNT_LOG_NOT_ALLOW_END)) ||
        (evt.hbSub.periodLog >= CFG_MDL_HB_SUB_PERIOD_LOG_NOT_ALLOW_START) ||
        (evt.hbSub.minHops >= CFG_MDL_HB_SUB_MIN_HOPS_NOT_ALLOW_START) ||
        (evt.hbSub.maxHops >= CFG_MDL_HB_SUB_MAX_HOPS_NOT_ALLOW_START) ||
         MESH_IS_ADDR_VIRTUAL(evt.hbSub.dstAddr) ||
         MESH_IS_ADDR_VIRTUAL(evt.hbSub.srcAddr) ||
         MESH_IS_ADDR_GROUP(evt.hbSub.srcAddr))
    {
      return FALSE;
    }
  }

  /* Set event type and address. */
  evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_CL_EVENT;
  evt.cfgMdlHdr.hdr.param = pReqParam->apiEvt;
  evt.cfgMdlHdr.peerAddress = pReqParam->cfgMdlSrAddr;

  /* Trigger user callback. */
  meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Low Power Node PollTimeout Status response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgMdlClHandleLpnPollTimeoutStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                              uint16_t msgParamLen)
{
  meshCfgMdlLpnPollTimeoutEvt_t evt;

  /* Validate parameters */
  if ((msgParamLen != CFG_MDL_MSG_LPN_POLLTIMEOUT_STATUS_NUM_BYTES) || (pMsgParam == NULL))
  {
    return FALSE;
  }

  /* Unpack message. */
  meshCfgMsgUnpackLpnPollTimeout(pMsgParam, &evt.lpnAddr, &evt.pollTimeout100Ms);

  /* Validate unpacked data. */
  if (!MESH_IS_ADDR_UNICAST(evt.lpnAddr) ||
      !CFG_MDL_MSG_LPN_POLLTIMEOUT_VALID(evt.pollTimeout100Ms))
  {
    return FALSE;
  }

  /* Set event type and address. */
  evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_CL_EVENT;
  evt.cfgMdlHdr.hdr.param = pReqParam->apiEvt;
  evt.cfgMdlHdr.hdr.status = MESH_CFG_MDL_CL_SUCCESS;
  evt.cfgMdlHdr.peerAddress = pReqParam->cfgMdlSrAddr;

  /* Trigger user callback. */
  meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handler for the Network Transmit Status response.
 *
 *  \param[in] pReqParams   Pointer to parameters of the request matching this response.
 *  \param[in] pMsgParam    Pointer to raw message parameters.
 *  \param[in] msgParamLen  Length of the message parameters array.
 *
 *  \return    TRUE if the message is processed successfully, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshCfgMdlClHandleNwkTransStatus(meshCfgMdlClOpReqParams_t *pReqParam, uint8_t *pMsgParam,
                                        uint16_t msgParamLen)
{
  meshCfgMdlNwkTransStateEvt_t evt;

  /* Validate parameters */
  if ((msgParamLen != CFG_MDL_MSG_NWK_TRANS_STATE_NUM_BYTES) || (pMsgParam == NULL))
  {
    return FALSE;
  }

  /* Unpack message. */
  meshCfgMsgUnpackNwkTrans(pMsgParam, &evt.nwkTransState);

  /* Set event type and address. */
  evt.cfgMdlHdr.hdr.event = MESH_CFG_MDL_CL_EVENT;
  evt.cfgMdlHdr.hdr.param = pReqParam->apiEvt;
  evt.cfgMdlHdr.hdr.status = MESH_CFG_MDL_CL_SUCCESS;
  evt.cfgMdlHdr.peerAddress = pReqParam->cfgMdlSrAddr;

  /* Trigger user callback. */
  meshCfgMdlClCb.cback((meshCfgMdlClEvt_t *)&evt);

  return TRUE;
}
