/*************************************************************************************************/
/*!
 *  \file   mesh_ht_sr_states.c
 *
 *  \brief  Implementation of the Health Server model states handling.
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
#include "wsf_timer.h"
#include "wsf_os.h"
#include "wsf_cs.h"
#include "wsf_buf.h"
#include "wsf_trace.h"
#include "wsf_assert.h"
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"

#include "mmdl_defs.h"
#include "mmdl_types.h"
#include "mmdl_common.h"
#include "mesh_ht_mdl_api.h"
#include "mesh_ht_sr_api.h"
#include "mesh_ht_sr_main.h"

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Computes number of faults in the fault array.
 *
 *  \param[in] pFaultArray  Pointer to fault array.
 *
 *  \return    Number of faults.
 */
/*************************************************************************************************/
uint8_t htSrGetNumFaults(uint8_t *pFaultArray)
{
  uint8_t idx = 0;
  uint8_t cnt = 0;

  for(idx = 0; idx < MESH_HT_SR_MAX_NUM_FAULTS; idx++)
  {
    if(pFaultArray[idx] != MESH_HT_MODEL_FAULT_NO_FAULT)
    {
      cnt++;
    }
  }

  return cnt;
}

/*************************************************************************************************/
/*!
 *  \brief      Searches for the Health Server model instance descriptor on the specified element.
 *
 *  \param[in]  elementId  Identifier of the Element implementing the model.
 *  \param[out] ppOutDesc  Double pointer to the descriptor.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void meshHtSrGetDesc(meshElementId_t elementId, meshHtSrDescriptor_t **ppOutDesc)
{
  uint8_t modelIdx;

  *ppOutDesc = NULL;

  /* Look for the model instance. */
  for (modelIdx = 0; modelIdx < pMeshConfig->pElementArray[elementId].numSigModels; modelIdx ++)
  {
    if (pMeshConfig->pElementArray[elementId].pSigModelArray[modelIdx].modelId ==
        MESH_HT_SR_MDL_ID)
    {
      /* Matching model ID on elementId */
      *ppOutDesc = pMeshConfig->pElementArray[elementId].pSigModelArray[modelIdx].pModelDescriptor;
      break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publishes Current Health Status of an element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshHtSrPublishCrtHt(meshElementId_t elementId)
{
  uint8_t *pRspMsgParam, *pTemp;
  meshHtSrDescriptor_t *pDesc;
  uint16_t rspMsgParamLen, tempLen;
  uint8_t cidx, fidx;
  uint8_t rspMsgParamNoFault[sizeof(meshHtMdlTestId_t) + sizeof(uint16_t)];
  meshPubMsgInfo_t pubMsgInfo =
  {
    .modelId.sigModelId = MESH_HT_SR_MDL_ID,
    .opcode = { MESH_HT_CRT_STATUS_OPCODE }
  };

  pubMsgInfo.elementId = elementId;

  /* Get descriptor. */
  meshHtSrGetDesc(elementId, &pDesc);

  if(pDesc == NULL)
  {
    return;
  }

  /* Search matching company ID. */
  for(cidx = 0; cidx < MESH_HT_SR_MAX_NUM_COMP; cidx++)
  {
    /* Get response length of status. */
    rspMsgParamLen = sizeof(rspMsgParamNoFault) +
                     htSrGetNumFaults(pDesc->faultStateArray[cidx].crtFaultIdArray);

    /* Configure response parameters based on number of faults. */
    if(rspMsgParamLen == sizeof(rspMsgParamNoFault))
    {
      pRspMsgParam = rspMsgParamNoFault;
    }
    else
    {
      /* Check response size and truncate if needed. */
      if(rspMsgParamLen + MESH_OPCODE_SIZE(pubMsgInfo.opcode) > MESH_ACC_MAX_PDU_SIZE)
      {
        rspMsgParamLen = MESH_ACC_MAX_PDU_SIZE - MESH_OPCODE_SIZE(pubMsgInfo.opcode);
      }

      /* Allocate memory for response. */
      if((pRspMsgParam = (uint8_t *)WsfBufAlloc(rspMsgParamLen)) == NULL)
      {
        continue;
      }
    }

    /* Prepare for packing. */
    pTemp = pRspMsgParam;
    tempLen = rspMsgParamLen - sizeof(rspMsgParamNoFault);

    /* Pack test ID and company ID. */
    UINT8_TO_BSTREAM(pTemp, pDesc->faultStateArray[cidx].testId);
    UINT16_TO_BSTREAM(pTemp, pDesc->faultStateArray[cidx].companyId);

    /* Search registered faults and pack them. */
    for(fidx = 0; fidx < MESH_HT_SR_MAX_NUM_FAULTS; fidx++)
    {
      /* If no more room available, break search. */
      if(tempLen == 0)
      {
        break;
      }

      if(pDesc->faultStateArray[cidx].crtFaultIdArray[fidx] != MESH_HT_MODEL_FAULT_NO_FAULT)
      {
        /* Pack fault. */
        UINT8_TO_BSTREAM(pTemp, pDesc->faultStateArray[cidx].crtFaultIdArray[fidx]);
        tempLen--;
      }
    }
    /* Send message */
    MeshPublishMessage(&pubMsgInfo, pRspMsgParam, rspMsgParamLen);

    /* Verify if memory is allocated and free it. */
    if(pRspMsgParam != rspMsgParamNoFault)
    {
      WsfBufFree(pRspMsgParam);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Mesh Health Status message to the specified destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] dstAddr      Element address of the destination.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pOpcode      Pointer to opcode.
 *  \param[in] pMsgParam    Pointer to message parameters.
 *  \param[in] msgParamLen  Message parameters length.
 *  \param[in] ttl          TTL.
 *  \param[in] unicastRsp   TRUE if response is sent for a request sent to an unicast address.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshHtSrSendStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                               uint16_t appKeyIndex, meshMsgOpcode_t *pOpcode, uint8_t *pMsgParam,
                               uint16_t msgParamLen, uint8_t ttl, bool_t unicastRsp)
{
  WSF_ASSERT(pOpcode != NULL);
  WSF_ASSERT(pMsgParam != NULL);

  meshMsgInfo_t msgInfo;

  /* Fill in the msg info parameters. */
  msgInfo.modelId.sigModelId = MESH_HT_SR_MDL_ID;
  msgInfo.opcode = *pOpcode;
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = dstAddr;
  msgInfo.pDstLabelUuid = NULL;
  msgInfo.ttl = ttl;
  msgInfo.appKeyIndex = appKeyIndex;


  /* Send message to the Mesh Core instantly. */
  MeshSendMessage(&msgInfo, (const uint8_t *)pMsgParam, msgParamLen,
                  MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                  MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(unicastRsp));
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Mesh Health Fault Status message to the specified destination address.
 *
 *  \param[in] companyId    Company identifier.
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] dstAddr      Element address of the destination.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] recvTtl      TTL of the received request generating this status.
 *  \param[in] unicastRsp   TRUE if status is sent for a request sent to an unicast address.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshHtSrSendFaultStatus(uint16_t companyId, meshElementId_t elementId,
                             meshAddress_t dstAddr, uint16_t appKeyIndex, uint8_t recvTtl,
                             bool_t unicastRsp)
{
  uint8_t *pRspMsgParam, *pTemp;
  meshHtSrDescriptor_t *pDesc;
  uint16_t rspMsgParamLen, tempLen;
  uint8_t cidx, fidx;
  uint8_t rspMsgParamNoFault[sizeof(meshHtMdlTestId_t) + sizeof(companyId)];
  meshMsgOpcode_t opcode = { MESH_HT_FAULT_STATUS_OPCODE };

  /* Get descriptor. */
  meshHtSrGetDesc(elementId, &pDesc);

  if(pDesc == NULL)
  {
    return;
  }

  /* Search matching company ID. */
  for(cidx = 0; cidx < MESH_HT_SR_MAX_NUM_COMP; cidx++)
  {
    if(pDesc->faultStateArray[cidx].companyId == companyId)
    {
      /* Get response length of status. */
      rspMsgParamLen = sizeof(rspMsgParamNoFault) +
                       htSrGetNumFaults(pDesc->faultStateArray[cidx].regFaultIdArray);

      /* Configure response parameters based on number of faults. */
      if(rspMsgParamLen == sizeof(rspMsgParamNoFault))
      {
        pRspMsgParam = rspMsgParamNoFault;
      }
      else
      {
        /* Check response size and truncate if needed. */
        if(rspMsgParamLen + MESH_OPCODE_SIZE(opcode) > MESH_ACC_MAX_PDU_SIZE)
        {
          rspMsgParamLen = MESH_ACC_MAX_PDU_SIZE - MESH_OPCODE_SIZE(opcode);
        }

        /* Allocate memory for response. */
        if((pRspMsgParam = (uint8_t *)WsfBufAlloc(rspMsgParamLen)) == NULL)
        {
          return;
        }
      }

      /* Prepare for packing. */
      pTemp = pRspMsgParam;
      tempLen = rspMsgParamLen - sizeof(rspMsgParamNoFault);

      /* Pack test ID and company ID. */
      UINT8_TO_BSTREAM(pTemp, pDesc->faultStateArray[cidx].testId);
      UINT16_TO_BSTREAM(pTemp, companyId);

      /* Search registered faults and pack them. */
      for(fidx = 0; fidx < MESH_HT_SR_MAX_NUM_FAULTS; fidx++)
      {
        /* If no more room available, break search. */
        if(tempLen == 0)
        {
          break;
        }

        if(pDesc->faultStateArray[cidx].regFaultIdArray[fidx] != MESH_HT_MODEL_FAULT_NO_FAULT)
        {
          /* Pack fault. */
          UINT8_TO_BSTREAM(pTemp, pDesc->faultStateArray[cidx].regFaultIdArray[fidx]);
          tempLen--;
        }
      }
      /* Send message */
      meshHtSrSendStatus(elementId, dstAddr, appKeyIndex, &opcode, pRspMsgParam, rspMsgParamLen,
                         recvTtl == 0 ? 0 : MESH_USE_DEFAULT_TTL, unicastRsp);

      /* Verify if memory is allocated and free it. */
      if(pRspMsgParam != rspMsgParamNoFault)
      {
        WsfBufFree(pRspMsgParam);
      }

      return;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles Health Fault Get operation.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshHtSrHandleFaultGet(const meshModelMsgRecvEvt_t *pMsg)
{
  meshHtSrDescriptor_t *pDesc;
  uint16_t companyId;
  uint8_t cidx;

  /* Validate message parameters length. */
  if(pMsg->messageParamsLen != sizeof(companyId))
  {
    return;
  }

  /* Get descriptor. */
  meshHtSrGetDesc(pMsg->elementId, &pDesc);

  if(pDesc == NULL)
  {
    return;
  }

  /* Extract company ID. */
  BYTES_TO_UINT16(companyId, pMsg->pMessageParams);

  /* Search for matching company ID. */
  for(cidx = 0; cidx < MESH_HT_SR_MAX_NUM_COMP; cidx++)
  {
    if(pDesc->faultStateArray[cidx].companyId == companyId)
    {
      /* Send status. */
      meshHtSrSendFaultStatus(companyId, pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                              pMsg->ttl, pMsg->recvOnUnicast);
      return;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles Health Fault Clear and Clear Unacknowledged operations.
 *
 *  \param[in] pMsg    Received model message.
 *  \param[in] ackReq  TRUE if message is acknowledged, FALSE otherwise.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshHtSrHandleFaultClearAll(const meshModelMsgRecvEvt_t *pMsg, bool_t ackReq)
{
  meshHtSrDescriptor_t *pDesc;
  uint16_t companyId;
  uint8_t cidx;

  /* Validate message parameters length. */
  if(pMsg->messageParamsLen != sizeof(companyId))
  {
    return;
  }

  /* Get descriptor. */
  meshHtSrGetDesc(pMsg->elementId, &pDesc);

  if(pDesc == NULL)
  {
    return;
  }

  /* Extract company ID. */
  BYTES_TO_UINT16(companyId, pMsg->pMessageParams);

  /* Search for matching company ID. */
  for(cidx = 0; cidx < MESH_HT_SR_MAX_NUM_COMP; cidx++)
  {
    if(pDesc->faultStateArray[cidx].companyId == companyId)
    {
      WsfTimerStop(&(pDesc->fastPubTmr));

      pDesc->fastPubOn = FALSE;

      /* Reset registered fault array. */
      memset(pDesc->faultStateArray[cidx].regFaultIdArray, MESH_HT_MODEL_FAULT_NO_FAULT,
             MESH_HT_SR_MAX_NUM_FAULTS);

      /* Send status if needed. */
      if(ackReq)
      {
        /* Send status. */
        meshHtSrSendFaultStatus(companyId, pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                                pMsg->ttl, pMsg->recvOnUnicast);
      }

      return;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles Health Fault Clear Unacknowledged operation.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshHtSrHandleFaultClearUnack(const meshModelMsgRecvEvt_t *pMsg)
{
  meshHtSrHandleFaultClearAll(pMsg, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles Health Fault Clear operation.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshHtSrHandleFaultClear(const meshModelMsgRecvEvt_t *pMsg)
{
  meshHtSrHandleFaultClearAll(pMsg, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles Health Fault Test and Test Unacknowledged operations.
 *
 *  \param[in] pMsg    Received model message.
 *  \param[in] ackReq  TRUE if message is acknowledged, FALSE otherwise.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshHtSrHandleFaultTestAll(const meshModelMsgRecvEvt_t *pMsg, bool_t ackReq)
{
  meshHtSrDescriptor_t *pDesc;
  uint8_t *pTemp;
  meshHtSrTestStartEvt_t evt =
  {
    .hdr.event = MESH_HT_SR_EVENT,
    .hdr.param = MESH_HT_SR_TEST_START_EVENT,
    .hdr.status = MMDL_SUCCESS
  };
  uint8_t cidx;

  /* Validate message parameters length. */
  if(pMsg->messageParamsLen != (sizeof(uint16_t) + sizeof(meshHtMdlTestId_t)))
  {
    return;
  }

  /* Get descriptor. */
  meshHtSrGetDesc(pMsg->elementId, &pDesc);

  if(pDesc == NULL)
  {
    return;
  }

  pTemp = pMsg->pMessageParams;

  /* Extract test id. */
  BSTREAM_TO_UINT8(evt.testId, pTemp);

  /* Extract company ID. */
  BSTREAM_TO_UINT16(evt.companyId, pTemp);

  /* Search for matching company ID. */
  for(cidx = 0; cidx < MESH_HT_SR_MAX_NUM_COMP; cidx++)
  {
    if(pDesc->faultStateArray[cidx].companyId == evt.companyId)
    {
      /* Configure event. */
      evt.elemId = pMsg->elementId;
      evt.htClAddr = pMsg->srcAddr;
      evt.useTtlZero = (pMsg->ttl == 0);
      evt.unicastReq = pMsg->recvOnUnicast;
      evt.appKeyIndex = pMsg->appKeyIndex;

      evt.notifTestEnd = ackReq;

      /* Trigger event callback. */
      htSrCb.recvCback((wsfMsgHdr_t*)&evt);

      return;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles Health Fault Test operation.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshHtSrHandleFaultTest(const meshModelMsgRecvEvt_t *pMsg)
{
  meshHtSrHandleFaultTestAll(pMsg, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles Health Fault Test Unacknowledged operation.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshHtSrHandleFaultTestUnack(const meshModelMsgRecvEvt_t *pMsg)
{
  meshHtSrHandleFaultTestAll(pMsg, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Mesh Health Period Status message to the specified destination address.
 *
 *  \param[in] period       Value of the Fast Period Divisor.
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] dstAddr      Element address of the destination.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] recvTtl      TTL of the received request generating this status.
 *  \param[in] unicastRsp   TRUE if status is sent for a request sent to an unicast address.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshHtSrSendPeriodStatus(meshHtPeriod_t period, meshElementId_t elementId,
                              meshAddress_t dstAddr, uint16_t appKeyIndex,
                              uint8_t recvTtl, bool_t unicastRsp)
{
  meshMsgOpcode_t opcode = { MESH_HT_PERIOD_STATUS_OPCODE };

  /* Send status message. */
  meshHtSrSendStatus(elementId, dstAddr, appKeyIndex, &opcode, &period, 1,
                     recvTtl == 0 ? 0 : MESH_USE_DEFAULT_TTL, unicastRsp);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles Health Period Get operation.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshHtSrHandlePeriodGet(const meshModelMsgRecvEvt_t *pMsg)
{
  meshHtSrDescriptor_t *pDesc;

  /* Validate message parameters length. */
  if(pMsg->messageParamsLen != 0)
  {
    return;
  }

  /* Get descriptor. */
  meshHtSrGetDesc(pMsg->elementId, &pDesc);

  /* Validate descriptor and state existing. */
  if(pDesc == NULL)
  {
    return;
  }

  /* Send status. */
  meshHtSrSendPeriodStatus(pDesc->fastPeriodDiv, pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                           pMsg->ttl, pMsg->recvOnUnicast);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles Health Period Set and Set Unacknowledged operations.
 *
 *  \param[in] pMsg    Received model message.
 *  \param[in] ackReq  TRUE if message is acknowledged, FALSE otherwise.
 *  \return    None.
 */
/*************************************************************************************************/
static void meshHtSrHandlePeriodSetAll(const meshModelMsgRecvEvt_t *pMsg, bool_t ackReq)
{
  meshHtSrDescriptor_t *pDesc;

  /* Validate message parameters length. */
  if((pMsg->messageParamsLen != 1) || (pMsg->pMessageParams == NULL))
  {
    return;
  }

  /* Get descriptor. */
  meshHtSrGetDesc(pMsg->elementId, &pDesc);

  /* Validate descriptor and state existing. */
  if(pDesc == NULL)
  {
    return;
  }

  /* Validate state */
  if(pMsg->pMessageParams[0] > MESH_HT_PERIOD_MAX_VALUE)
  {
    return;
  }

  /* Set state. */
  if(pDesc->fastPeriodDiv != pMsg->pMessageParams[0])
  {
    pDesc->fastPeriodDiv = pMsg->pMessageParams[0];
    if((pDesc->fastPubOn) && (pDesc->fastPeriodDiv != 0) && (pDesc->pubPeriodMs != 0))
    {
      /* Publish Current Health Status. */
      meshHtSrPublishCrtHt(pMsg->elementId);

      /* Start timer. */
      WsfTimerStartMs(&(pDesc->fastPubTmr), FAST_PUB_TIME(pDesc));
    }
    else
    {
      WsfTimerStop(&(pDesc->fastPubTmr));
    }
  }

  if(ackReq)
  {
    /* Send status if needed. */
    meshHtSrSendPeriodStatus(pDesc->fastPeriodDiv, pMsg->elementId, pMsg->srcAddr,
                             pMsg->appKeyIndex, pMsg->ttl, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles Health Period Set operation.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshHtSrHandlePeriodSetUnack(const meshModelMsgRecvEvt_t *pMsg)
{
  meshHtSrHandlePeriodSetAll(pMsg, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles Health Period Set operation.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshHtSrHandlePeriodSet(const meshModelMsgRecvEvt_t *pMsg)
{
  meshHtSrHandlePeriodSetAll(pMsg, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Mesh Health Attention Status message to the specified destination address.
 *
 *  \param[in] attTimerSec  Attention Timer value in seconds.
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] dstAddr      Element address of the destination.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] recvTtl      TTL of the received request generating this status.
 *  \param[in] unicastRsp   TRUE if status is sent for a request sent to an unicast address.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshHtSrSendAttentionStatus(uint8_t attTimerSec, meshElementId_t elementId,
                                 meshAddress_t dstAddr, uint16_t appKeyIndex,
                                 uint8_t recvTtl, bool_t unicastRsp)
{
  meshMsgOpcode_t opcode = { MESH_HT_ATTENTION_STATUS_OPCODE };

  /* Send status message. */
  meshHtSrSendStatus(elementId, dstAddr, appKeyIndex, &opcode, &attTimerSec, 1,
                     recvTtl == 0 ? 0 : MESH_USE_DEFAULT_TTL, unicastRsp);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles Health Attention Get operation.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshHtSrHandleAttentionGet(const meshModelMsgRecvEvt_t *pMsg)
{
  meshHtSrDescriptor_t *pDesc;
  uint8_t attTimerSec;
  /* Validate message parameters length. */
  if(pMsg->messageParamsLen != 0)
  {
    return;
  }

  /* Get descriptor. */
  meshHtSrGetDesc(pMsg->elementId, &pDesc);

  /* Validate descriptor and state existing. */
  if(pDesc == NULL)
  {
    return;
  }

  /* Read attention timer state. */
  attTimerSec = MeshAttentionGet(pMsg->elementId);

  /* Send status. */
  meshHtSrSendAttentionStatus(attTimerSec, pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                              pMsg->ttl, pMsg->recvOnUnicast);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles Health Attention Set and Set Unacknowledged operations.
 *
 *  \param[in] pMsg    Received model message.
 *  \param[in] ackReq  TRUE if message is acknowledged, FALSE otherwise.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshHtSrHandleAttentionSetAll(const meshModelMsgRecvEvt_t *pMsg, bool_t ackReq)
{
  meshHtSrDescriptor_t *pDesc;

  /* Validate message parameters length. */
  if((pMsg->messageParamsLen != 1) || (pMsg->pMessageParams == NULL))
  {
    return;
  }

  /* Get descriptor. */
  meshHtSrGetDesc(pMsg->elementId, &pDesc);

  /* Validate descriptor and state existing. */
  if(pDesc == NULL)
  {
    return;
  }

  /* Set state. */
  MeshAttentionSet(pMsg->elementId, pMsg->pMessageParams[0]);

  if(ackReq)
  {
    /* Send status if needed. */
    meshHtSrSendAttentionStatus(pMsg->pMessageParams[0], pMsg->elementId, pMsg->srcAddr,
                                pMsg->appKeyIndex, pMsg->ttl, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles Health Attention Set operation.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshHtSrHandleAttentionSet(const meshModelMsgRecvEvt_t *pMsg)
{
  meshHtSrHandleAttentionSetAll(pMsg, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles Health Attention Set Unacknowledged operation.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshHtSrHandleAttentionSetUnack(const meshModelMsgRecvEvt_t *pMsg)
{
  meshHtSrHandleAttentionSetAll(pMsg, FALSE);
}
