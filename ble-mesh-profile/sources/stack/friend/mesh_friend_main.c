/*************************************************************************************************/
/*!
 *  \file   mesh_friend_main.c
 *
 *  \brief  Friend module implementation.
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

#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_msg.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_error_codes.h"
#include "mesh_api.h"
#include "mesh_main.h"

#include "mesh_local_config_types.h"
#include "mesh_local_config.h"

#include "mesh_network.h"
#include "mesh_network_mgmt.h"
#include "mesh_lower_transport.h"
#include "mesh_sar_rx.h"
#include "mesh_upper_transport.h"

#include "mesh_cfg_mdl_sr.h"

#include "mesh_friend_api.h"
#include "mesh_friend.h"
#include "mesh_friend_main.h"
#include "mesh_friendship_defs.h"

#include "mesh_utils.h"

#include "util/bstream.h"
#include <string.h>

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Mesh Friend control block. */
meshFriendCb_t friendCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Resets an LPN context.
 *
 *  \param[in] idx  Index of the context to be reset.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshFriendResetLpnCtx(uint8_t idx)
{
  /* Reset establishment information. */
  memset(&(LPN_CTX_PTR(idx)->estabInfo), 0, sizeof(meshFriendEstabInfo_t));

  /* Restore free count. */
  LPN_CTX_PTR(idx)->pduQueueFreeCount = GET_MAX_NUM_QUEUE_ENTRIES();

  /* Reset the queue. */
  memset(LPN_CTX_PTR(idx)->pQueuePool, 0,
         LPN_CTX_PTR(idx)->pduQueueFreeCount * sizeof(meshFriendQueueEntry_t));
  WSF_QUEUE_INIT(&(LPN_CTX_PTR(idx)->pduQueue));

  /* Reset subscription list. */
  memset(LPN_CTX_PTR(idx)->pSubscrAddrList, 0,
         GET_MAX_SUBSCR_LIST_SIZE() * sizeof(meshAddress_t));

  /* Reset address and NetKey Index. */
  LPN_CTX_PTR(idx)->lpnAddr = MESH_ADDR_TYPE_UNASSIGNED;
  LPN_CTX_PTR(idx)->netKeyIndex = 0xFFFF;
  LPN_CTX_PTR(idx)->inUse = FALSE;

  /* Reset FSN and Transaction Number by setting invalid value. */
  LPN_CTX_PTR(idx)->crtNextFsn = FRIEND_CRT_NEXT_FSN_INIT_VAL;
  /* Set to 0xFF so that first transaction is 0 and differs from stored. */
  LPN_CTX_PTR(idx)->transNum = FRIEND_SUBSCR_TRANS_NUM_INIT_VAL;
  /* Reset Clear Period Counter. */
  LPN_CTX_PTR(idx)->clearPeriodTimeSec = 0;

  /* Stop pending timers. */
  WsfTimerStop(&(LPN_CTX_PTR(idx)->pollTmr));
  WsfTimerStop(&(LPN_CTX_PTR(idx)->recvDelayTmr));
  WsfTimerStop(&(LPN_CTX_PTR(idx)->subscrCnfRecvDelayTmr));
  WsfTimerStop(&(LPN_CTX_PTR(idx)->clearPeriodTmr));
}

/*************************************************************************************************/
/*!
 *  \brief     Converts LPN address into the context using it.
 *
 *  \param[in] lpnAddr      Address of the LPN.
 *  \param[in] netKeyIndex  Global identifier of the NetKey.
 *
 *  \return    Pointer to context or NULL.
 */
/*************************************************************************************************/
static meshFriendLpnCtx_t *meshFriendLpnInfoToCtx(meshAddress_t lpnAddr, uint16_t netKeyIndex)
{
  uint8_t idx;
  for(idx = 0; idx < GET_MAX_NUM_CTX(); idx++)
  {
    if((LPN_CTX_PTR(idx)->inUse) &&
       (LPN_CTX_PTR(idx)->lpnAddr == lpnAddr) &&
       (LPN_CTX_PTR(idx)->netKeyIndex == netKeyIndex))
    {
      WSF_ASSERT(LPN_CTX_PTR(idx)->friendSmState > FRIEND_ST_IDLE);
      return LPN_CTX_PTR(idx);
    }
  }
  return NULL;
}

/*************************************************************************************************/
/*!
 *  \brief  Allocates empty context.
 *
 *  \return Pointer to context or NULL.
 */
/*************************************************************************************************/
static meshFriendLpnCtx_t *meshFriendGetUnusedCtx(void)
{
  uint8_t idx;
  for(idx = 0; idx < GET_MAX_NUM_CTX(); idx++)
  {
    if(!(LPN_CTX_PTR(idx)->inUse))
    {
      return LPN_CTX_PTR(idx);
    }
  }
  return NULL;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Friend Request PDU.
 *
 *  \param[in] pCtlPduInfo  Pointer to the structure holding the received Control PDU and other
 *                          fields.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshFriendHandleRequest(const meshLtrCtlPduInfo_t *pCtlPduInfo)
{
  meshFriendLpnCtx_t *pCtx;
  meshFriendReq_t *pMsg;
  uint8_t *pPdu;
  uint32_t pollTimeout;
  int32_t  calcDelay;
  uint16_t lpnCounter, prevAddr;
  uint8_t criteria, minQLog, rssiFact, recvWindFact, recvDelay, numElements;
  int8_t rssi = MESH_FRIEND_RSSI_UNAVAILBLE;

  /* Validate correct key credentials. */
  if(!MESH_IS_ADDR_UNASSIGNED(pCtlPduInfo->friendLpnAddr))
  {
    return;
  }

  /* Validate destination address. */
  if(pCtlPduInfo->dst != MESH_ADDR_GROUP_FRIEND)
  {
    return;
  }

  /* Validate TTL. */
  if(pCtlPduInfo->ttl != 0)
  {
    return;
  }

  /* Validate message length */
  if((pCtlPduInfo->pduLen != MESH_FRIEND_REQUEST_NUM_BYTES) || (pCtlPduInfo->pUtrCtlPdu == NULL))
  {
    return;
  }

  /* Search if context exists for the same LPN. */
  pCtx = meshFriendLpnInfoToCtx(pCtlPduInfo->src, pCtlPduInfo->netKeyIndex);

  if(pCtx == NULL)
  {
    /* Get unused context. */
    if((pCtx = meshFriendGetUnusedCtx()) == NULL)
    {
      /* Return if none found. */
      return;
    }
  }

  pPdu = pCtlPduInfo->pUtrCtlPdu;

  /* Extract criteria. */
  BSTREAM_TO_UINT8(criteria, pPdu);

  /* Get factors. */
  rssiFact = MESH_UTILS_BF_GET(criteria, MESH_FRIEND_REQUEST_RSSI_FACTOR_SHIFT,
                               MESH_FRIEND_REQUEST_RSSI_FACTOR_SIZE);
  recvWindFact = MESH_UTILS_BF_GET(criteria, MESH_FRIEND_REQUEST_RECV_WIN_FACTOR_SHIFT,
                                   MESH_FRIEND_REQUEST_RECV_WIN_FACTOR_SIZE);

  /* Get queue log field. */
  minQLog = MESH_UTILS_BF_GET(criteria, MESH_FRIEND_REQUEST_MIN_QUEUE_SIZE_SHIFT,
                              MESH_FRIEND_REQUEST_MIN_QUEUE_SIZE_SIZE);

  /* Extract Receive Delay. */
  BSTREAM_TO_UINT8(recvDelay, pPdu);

  /* Extract Poll Timeout. */
  BSTREAM_BE_TO_UINT24(pollTimeout, pPdu);

  /* Extract previous address. */
  BSTREAM_BE_TO_UINT16(prevAddr, pPdu);

  /* Extract number of elements */
  BSTREAM_TO_UINT8(numElements, pPdu);

  /* Extract LPN Counter. */
  BYTES_BE_TO_UINT16(lpnCounter, pPdu)

  /* Validate fields. */
  if(!MESH_FRIEND_RECV_DELAY_VALID(recvDelay) ||
     !MESH_FRIEND_POLL_TIMEOUT_MS_VALID(pollTimeout * MESH_FRIEND_POLL_TIMEOUT_STEP_MS) ||
     ((!MESH_IS_ADDR_UNASSIGNED(prevAddr) && !MESH_IS_ADDR_UNICAST(prevAddr))) ||
     (numElements == 0))
  {
    return;
  }

  /* Validate and check if queue size requirement is met. */
  if(minQLog == MESH_FRIEND_MIN_QUEUE_SIZE_PROHIBITED)
  {
    return;
  }

  /* Compare required queue size with existing. */
  if((1 << minQLog) > GET_MAX_NUM_QUEUE_ENTRIES())
  {
    /* If strict requirements need to be met, return. */;
  }

  /* Allocate message. */
  if((pMsg = WsfMsgAlloc(sizeof(meshFriendReq_t))) == NULL)
  {
    return;
  }

  /* Calculate local delay. Multiply by 10 to turn 0.5 in 5. */
  calcDelay = ((int32_t)(10 + (recvWindFact * 5)) * (int32_t)friendCb.recvWindow) -
              ((int32_t)(10 + (rssiFact * 5)) * (int32_t)rssi);

  /* Check if calculated delay is under the minimum value multiplied by 10. */
  if(calcDelay < (int32_t)(MESH_FRIEND_MIN_OFFER_DELAY_MS * 10))
  {
    calcDelay = (int32_t)MESH_FRIEND_MIN_OFFER_DELAY_MS;
  }
  else
  {
    /* Divide result by 10 to recover original value. */
    calcDelay /= 10;
  }

  /* Mark slot as in use. If it was already in use, there is no change. */
  pCtx->inUse = TRUE;

  /* Configure message. */
  pMsg->hdr.event = MESH_FRIEND_MSG_FRIEND_REQ_RECV;
  pMsg->hdr.param = LPN_CTX_IDX(pCtx);

  pMsg->localDelay = (uint32_t)calcDelay;
  pMsg->recvDelay = recvDelay;
  pMsg->pollTimeout = pollTimeout;
  pMsg->prevAddr = prevAddr;
  pMsg->numElements = numElements;
  pMsg->lpnCounter = lpnCounter;
  pMsg->netKeyIndex = pCtlPduInfo->netKeyIndex;
  pMsg->lpnAddr = pCtlPduInfo->src;
  pMsg->rssi = rssi;

  /* Send message. */
  WsfMsgSend(meshCb.handlerId, pMsg);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Friend Poll PDU.
 *
 *  \param[in] pCtlPduInfo  Pointer to the structure holding the received Control PDU and other
 *                          fields.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshFriendHandlePoll(const meshLtrCtlPduInfo_t *pCtlPduInfo)
{
  meshFriendLpnCtx_t *pCtx;
  meshFriendPoll_t *pMsg;
  meshAddress_t elem0Addr;

  /* Validate correct key credentials. */
  if((MESH_IS_ADDR_UNASSIGNED(pCtlPduInfo->friendLpnAddr) ||
      pCtlPduInfo->friendLpnAddr != pCtlPduInfo->src))
  {
    return;
  }

  /* Get element 0 address. */
  MeshLocalCfgGetAddrFromElementId(0, &elem0Addr);

  /* Validate destination address. */
  if(pCtlPduInfo->dst != elem0Addr)
  {
    return;
  }

  /* Validate TTL. */
  if(pCtlPduInfo->ttl != 0)
  {
    return;
  }

  /* Validate message length */
  if((pCtlPduInfo->pduLen != MESH_FRIEND_POLL_NUM_BYTES) || (pCtlPduInfo->pUtrCtlPdu == NULL))
  {
    return;
  }

  /* Validate prohibited bits. */
  if((pCtlPduInfo->pUtrCtlPdu[0] & ~MESH_FRIEND_POLL_FSN_MASK))
  {
    return;
  }

  /* Search if context exists for the same friendship. */
  pCtx = meshFriendLpnInfoToCtx(pCtlPduInfo->src, pCtlPduInfo->netKeyIndex);

  if(pCtx == NULL)
  {
    return;
  }

  /* Check for invalid FSN on first poll. */
  if((pCtx->friendSmState != FRIEND_ST_ESTAB) &&
     (pCtlPduInfo->pUtrCtlPdu[0] != 0))
  {
    return;
  }

  /* Allocate message. */
  if((pMsg = WsfMsgAlloc(sizeof(meshFriendPoll_t))) == NULL)
  {
    return;
  }

  /* Configure message. */
  pMsg->hdr.event = MESH_FRIEND_MSG_POLL_RECV;
  pMsg->hdr.param = LPN_CTX_IDX(pCtx);
  pMsg->lpnAddr = pCtlPduInfo->src;
  pMsg->fsn = (pCtlPduInfo->pUtrCtlPdu[0] & MESH_FRIEND_POLL_FSN_MASK);

  /* Send message. */
  WsfMsgSend(meshCb.handlerId, pMsg);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Friend Clear PDU.
 *
 *  \param[in] pCtlPduInfo  Pointer to the structure holding the received Control PDU and other
 *                          fields.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshFriendHandleClear(const meshLtrCtlPduInfo_t *pCtlPduInfo)
{
  meshFriendLpnCtx_t *pCtx;
  meshFriendClear_t *pMsg;
  uint8_t *pPdu;
  meshAddress_t elem0Addr, lpnAddr;
  uint16_t lpnCounter;

  /* Validate correct key credentials. */
  if(!MESH_IS_ADDR_UNASSIGNED(pCtlPduInfo->friendLpnAddr))
  {
    return;
  }

  /* Get element 0 address. */
  MeshLocalCfgGetAddrFromElementId(0, &elem0Addr);

  /* Validate destination address. */
  if(pCtlPduInfo->dst != elem0Addr)
  {
    return;
  }

  /* Validate message length */
  if((pCtlPduInfo->pduLen != MESH_FRIEND_CLEAR_NUM_BYTES) || (pCtlPduInfo->pUtrCtlPdu == NULL))
  {
    return;
  }

  pPdu = pCtlPduInfo->pUtrCtlPdu;

  /* Extract address and counter. */
  BSTREAM_BE_TO_UINT16(lpnAddr, pPdu);
  BYTES_BE_TO_UINT16(lpnCounter, pPdu);

  /* Get context based on LPN address. */
  pCtx = meshFriendLpnInfoToCtx(lpnAddr, pCtlPduInfo->netKeyIndex);

  /* Check if context exists and new counter is greater than old. */
  if((pCtx == NULL) ||
     (FRIEND_LPN_CTR_WRAP_DIFF(pCtx->estabInfo.lpnCounter, lpnCounter) >
        MESH_FRIEND_MAX_LPN_CNT_WRAP_DIFF))
  {
    return;
  }

  /* Allocate message. */
  if((pMsg = WsfMsgAlloc(sizeof(meshFriendClear_t))) == NULL)
  {
    return;
  }

  /* Configure message. */
  pMsg->hdr.event = MESH_FRIEND_MSG_FRIEND_CLEAR_RECV;
  pMsg->hdr.param = LPN_CTX_IDX(pCtx);

  pMsg->friendAddr = pCtlPduInfo->src;
  pMsg->lpnAddr = lpnAddr;
  pMsg->lpnCounter = lpnCounter;

  /* Send message. */
  WsfMsgSend(meshCb.handlerId, pMsg);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Friend Clear Confirm PDU.
 *
 *  \param[in] pCtlPduInfo  Pointer to the structure holding the received Control PDU and other
 *                          fields.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshFriendHandleClearCnf(const meshLtrCtlPduInfo_t *pCtlPduInfo)
{
  meshFriendLpnCtx_t *pCtx;
  meshFriendClearCnf_t *pMsg;
  uint8_t *pPdu;
  meshAddress_t elem0Addr, lpnAddr;
  uint16_t lpnCounter;

  /* Validate correct key credentials. */
  if(!MESH_IS_ADDR_UNASSIGNED(pCtlPduInfo->friendLpnAddr))
  {
    return;
  }

  /* Get element 0 address. */
  MeshLocalCfgGetAddrFromElementId(0, &elem0Addr);

  /* Validate destination address. */
  if(pCtlPduInfo->dst != elem0Addr)
  {
    return;
  }

  /* Validate message length */
  if((pCtlPduInfo->pduLen != MESH_FRIEND_CLEAR_CNF_NUM_BYTES) || (pCtlPduInfo->pUtrCtlPdu == NULL))
  {
    return;
  }

  pPdu = pCtlPduInfo->pUtrCtlPdu;

  /* Extract address and counter. */
  BSTREAM_BE_TO_UINT16(lpnAddr, pPdu);
  BYTES_BE_TO_UINT16(lpnCounter, pPdu);

  /* Get context based on LPN address. */
  pCtx = meshFriendLpnInfoToCtx(lpnAddr, pCtlPduInfo->netKeyIndex);

  /* Check if friendship still exists. */
  if(pCtx == NULL)
  {
    return;
  }

  /* Allocate message. */
  if((pMsg = WsfMsgAlloc(sizeof(meshFriendClearCnf_t))) == NULL)
  {
    return;
  }

  /* Configure message. */
  pMsg->hdr.event = MESH_FRIEND_MSG_FRIEND_CLEAR_CNF_RECV;
  pMsg->hdr.param = LPN_CTX_IDX(pCtx);

  pMsg->friendAddr = pCtlPduInfo->src;
  pMsg->lpnAddr = lpnAddr;
  pMsg->lpnCounter = lpnCounter;

  /* Send message. */
  WsfMsgSend(meshCb.handlerId, pMsg);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles Friend Subscription List Add and Remove PDUs.
 *
 *  \param[in] pCtlPduInfo  Pointer to the structure holding the received Control PDU and other
 *                          fields.
 *  \param[in] isAdd        TRUE if operation is add, FALSE if operation is remove.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshFriendHandleSubscrAddRm(const meshLtrCtlPduInfo_t *pCtlPduInfo, bool_t isAdd)
{
  meshFriendLpnCtx_t *pCtx;
  meshFriendSubscrList_t *pMsg;
  meshAddress_t addr;
  uint8_t *pPdu;
  meshAddress_t elem0Addr;
  uint8_t numAddr, idx = 0;

  /* Validate correct key credentials. */
  if((MESH_IS_ADDR_UNASSIGNED(pCtlPduInfo->friendLpnAddr) ||
      pCtlPduInfo->friendLpnAddr != pCtlPduInfo->src))
  {
    return;
  }

  /* Get element 0 address. */
  MeshLocalCfgGetAddrFromElementId(0, &elem0Addr);

  /* Validate destination address. */
  if(pCtlPduInfo->dst != elem0Addr)
  {
    return;
  }

  /* Validate TTL. */
  if(pCtlPduInfo->ttl != 0)
  {
    return;
  }

  /* Validate message length */
  if((!MESH_FRIEND_SUBSCR_LIST_VALID(pCtlPduInfo->pduLen)) || (pCtlPduInfo->pUtrCtlPdu == NULL))
  {
    return;
  }

  numAddr = (uint8_t)MESH_FRIEND_SUBSCR_LIST_ADD_RM_NUM_ADDR(pCtlPduInfo->pduLen);

  /* Check number of addresses. */
  if(numAddr == 0)
  {
    return;
  }

  /* Search if context exists for the same LPN. */
  pCtx = meshFriendLpnInfoToCtx(pCtlPduInfo->src, pCtlPduInfo->netKeyIndex);

  if(pCtx == NULL)
  {
    return;
  }

  /* Check if transaction number is different the same. */
  if(pCtx->transNum == pCtlPduInfo->pUtrCtlPdu[0])
  {
    /* Make sure not to allocate memory for the address list. */
    numAddr = 0;
  }

  /* Allocate message. */
  if((pMsg = WsfMsgAlloc(sizeof(meshFriendSubscrList_t) + numAddr*sizeof(meshAddress_t))) == NULL)
  {
    return;
  }
  /* Point list to end of message. */
  pMsg->pSubscrList = (meshAddress_t *)(((uint8_t *)pMsg) + sizeof(meshFriendSubscrList_t));

  /* Configure message. */
  pMsg->hdr.event = isAdd ? MESH_FRIEND_MSG_SUBSCR_LIST_ADD : MESH_FRIEND_MSG_SUBSCR_LIST_REM;
  pMsg->hdr.param = LPN_CTX_IDX(pCtx);
  pMsg->lpnAddr = pCtlPduInfo->src;
  pMsg->listSize = numAddr;

  pPdu = pCtlPduInfo->pUtrCtlPdu;

  /* Get transaction number. */
  BSTREAM_TO_UINT8(pMsg->transNum, pPdu);

  /* Parse address list. */
  while(numAddr > 0)
  {
    BSTREAM_BE_TO_UINT16(addr, pPdu);

    /* Validate address. */
    if(!MESH_IS_ADDR_GROUP(addr) && !MESH_IS_ADDR_VIRTUAL(addr))
    {
      WsfMsgFree(pMsg);
      return;
    }
    pMsg->pSubscrList[idx++] = addr;
    numAddr--;
  }

  /* Send message. */
  WsfMsgSend(meshCb.handlerId, pMsg);
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Friend Control PDU received callback function pointer type.
 *
 *  \param[in] pCtlPduInfo  Pointer to the structure holding the received Control PDU and other
 *                          fields. See ::meshLtrCtlPduInfo_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshFriendCtlRecvCback(const meshLtrCtlPduInfo_t *pCtlPduInfo)
{
  WSF_ASSERT(pCtlPduInfo != NULL);

  switch (pCtlPduInfo->opcode)
  {
    case MESH_UTR_CTL_FRIEND_REQUEST_OPCODE:
      meshFriendHandleRequest(pCtlPduInfo);
      break;
    case MESH_UTR_CTL_FRIEND_POLL_OPCODE:
      meshFriendHandlePoll(pCtlPduInfo);
      break;
    case MESH_UTR_CTL_FRIEND_CLEAR_OPCODE:
      meshFriendHandleClear(pCtlPduInfo);
      break;
    case MESH_UTR_CTL_FRIEND_CLEAR_CNF_OPCODE:
      meshFriendHandleClearCnf(pCtlPduInfo);
      break;
    case MESH_UTR_CTL_FRIEND_SUBSCR_LIST_ADD_OPCODE:
      meshFriendHandleSubscrAddRm(pCtlPduInfo, TRUE);
      break;
    case MESH_UTR_CTL_FRIEND_SUBSCR_LIST_RM_OPCODE:
      meshFriendHandleSubscrAddRm(pCtlPduInfo, FALSE);
      break;
    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler callback for Friend.
 *
 *  \param[in] pMsg  Friend message callback event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshFriendMsgCback(wsfMsgHdr_t *pMsg)
{
  uint8_t idx;
  switch (pMsg->event)
  {
    case MESH_FRIEND_MSG_STATE_ENABLED:
      /* Change state in place for all. */
      for(idx = 0; idx < GET_MAX_NUM_CTX(); idx++)
      {
        WSF_ASSERT(LPN_CTX_PTR(idx)->friendSmState == FRIEND_ST_IDLE);
        if(LPN_CTX_PTR(idx)->friendSmState == FRIEND_ST_IDLE)
        {
          meshFriendSmExecute(LPN_CTX_PTR(idx), (meshFriendSmMsg_t *)pMsg);
        }
      }
      break;
    case MESH_FRIEND_MSG_STATE_DISABLED:
      /* Execute SM on all contexts. */
      for(idx = 0; idx < GET_MAX_NUM_CTX(); idx++)
      {
         meshFriendSmExecute(LPN_CTX_PTR(idx), (meshFriendSmMsg_t *)pMsg);
      }
      break;
    case MESH_FRIEND_MSG_FRIEND_REQ_RECV:
    case MESH_FRIEND_MSG_POLL_RECV:
    case MESH_FRIEND_MSG_FRIEND_CLEAR_RECV:
    case MESH_FRIEND_MSG_FRIEND_CLEAR_CNF_RECV:
    case MESH_FRIEND_MSG_KEY_DERIV_SUCCESS:
    case MESH_FRIEND_MSG_KEY_DERIV_FAILED:
    case MESH_FRIEND_MSG_RECV_DELAY:
    case MESH_FRIEND_MSG_SUBSCR_CNF_DELAY:
    case MESH_FRIEND_MSG_CLEAR_SEND_TIMEOUT:
    case MESH_FRIEND_MSG_TIMEOUT:
    case MESH_FRIEND_MSG_SUBSCR_LIST_ADD:
    case MESH_FRIEND_MSG_SUBSCR_LIST_REM:
      WSF_ASSERT(pMsg->param < GET_MAX_NUM_CTX());
      WSF_ASSERT(LPN_CTX_PTR(pMsg->param)->inUse);
      meshFriendSmExecute(LPN_CTX_PTR(pMsg->param), (meshFriendSmMsg_t *)pMsg);
      break;
    case MESH_FRIEND_MSG_NETKEY_DEL:
      for(idx = 0; idx < GET_MAX_NUM_CTX(); idx++)
      {
        /* Run state machine for contexts in use with the same NetKey Index. */
        if((LPN_CTX_PTR(idx)->inUse) &&
           (LPN_CTX_PTR(idx)->netKeyIndex == ((meshFriendNetKeyDel_t *)pMsg)->netKeyIndex))
        {
          meshFriendSmExecute(LPN_CTX_PTR(idx), (meshFriendSmMsg_t *)pMsg);
        }
      }
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Callback implementation for friendship security updates.
 *
 *  \param[in] ivChg        IV update state changed.
 *  \param[in] keyChg       Key refresh state changed for the NetKey specified by netKeyIndex param.
 *  \param[in] netKeyIndex  NetKey Index. Valid if keyChg is TRUE.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshFriendSecChgCback(bool_t ivChg, bool_t keyChg, uint16_t netKeyIndex)
{
  uint8_t idx;

  /* Trap if not exclusive and at least one is TRUE. */
  WSF_ASSERT((ivChg ^ keyChg) == TRUE );

  /* Iterate through all the LPN contexts. */
  for(idx = 0; idx < GET_MAX_NUM_CTX(); idx++)
  {
    /* Check for an established friendship. */
    if((LPN_CTX_PTR(idx)->inUse) && (LPN_CTX_PTR(idx)->friendSmState == FRIEND_ST_ESTAB))
    {
      /* On key refresh state changed, ignore friendships on a different sub-net. */
      if(keyChg)
      {
        if(LPN_CTX_PTR(idx)->netKeyIndex != netKeyIndex)
        {
          continue;
        }
      }
      /* Add an update message. */
      meshFriendQueueAddUpdate(LPN_CTX_PTR(idx));
    }
  }
  (void)ivChg;
}

/*************************************************************************************************/
/*!
 *  \brief  Handles Friend state changed.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void meshFriendStateChgCback(void)
{
  meshFriendSmMsg_t    *pMsg;
  meshFriendStates_t   friendState;

  /* Read Friend State. */
  friendState = MeshLocalCfgGetFriendState();
  WSF_ASSERT(friendState != MESH_FRIEND_FEATURE_NOT_SUPPORTED);

  if(friendCb.state != friendState)
  {
    /* Allocate message. */
    if((pMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t))) == NULL)
    {
      return;
    }

    /* Configure message. */
    pMsg->hdr.event = (friendState == MESH_FRIEND_FEATURE_DISABLED) ?
                       MESH_FRIEND_MSG_STATE_DISABLED :
                       MESH_FRIEND_MSG_STATE_ENABLED;
    pMsg->hdr.param = 0;

    /* Send message. */
    WsfMsgSend(meshCb.handlerId, pMsg);

    /* Set internal state. */
    friendCb.state = friendState;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles NetKey deletion.
 *
 *  \param[in] netKeyIndex  NetKey index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshFriendNetKeyDelNotifyCback(uint16_t netKeyIndex)
{
  meshFriendNetKeyDel_t *pMsg;

  /* Allocate message. */
  if((pMsg = WsfMsgAlloc(sizeof(meshFriendNetKeyDel_t))) != NULL)
  {
    pMsg->hdr.event = MESH_FRIEND_MSG_NETKEY_DEL;
    pMsg->netKeyIndex = netKeyIndex;

    /* Send message. */
    WsfMsgSend(meshCb.handlerId, pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles Poll Timeout Timer get.
 *
 *  \param[in] lpnAddr      LPN address.
 *
 *  \return    Min Poll Timer Timeout value.
 */
/*************************************************************************************************/
static uint32_t meshFriendPollTimeoutGetCback(meshAddress_t lpnAddr)
{
  meshFriendLpnCtx_t *pCtx = friendCb.pLpnCtxTbl;
  uint32_t minTimeout = 0xFFFFFFFF;
  uint32_t idx;

  /* Errata 10087: For each Low Power node, the entry in the PollTimeout List holds the current
   * value of the PollTimeout timer. If there are multiple friendship relationships set up on
   * multiple subnets, the value held on the list is the minimum value of all PollTimeout timers
   * for all friendship relationships the Friend Node has established with the Low Power node.
   * The list is indexed by Low Power node primary element address.
   */
  for (idx = 0; idx < GET_MAX_NUM_CTX(); idx++)
  {
    if ((pCtx->inUse) && (pCtx->lpnAddr == lpnAddr) &&
        (pCtx->friendSmState == FRIEND_ST_ESTAB) &&
        (pCtx->estabInfo.pollTimeout < minTimeout))
    {
      minTimeout = pCtx->estabInfo.pollTimeout;
    }

    pCtx++;
  }

  /* Check if friendship exists. */
  if(minTimeout == 0xFFFFFFFF)
  {
    minTimeout = 0;
  }

  /* Return minimum Poll Timeout in units of 100 ms. */
  return minTimeout;
}

/*************************************************************************************************/
/*!
 *  \brief  Registers module callbacks for Friend.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void meshFriendRegisterCbacks(void)
{
  /* Setup WSF message handler. */
  meshCb.friendshipMsgCback = meshFriendMsgCback;

  /* Setup UTR CTL opcode handler. */
  MeshUtrRegisterFriendship(meshFriendCtlRecvCback);

  /* Setup Network Management notification callback. */
  MeshNwkMgmtRegisterFriendship(meshFriendSecChgCback);

  /* Setup Config Server notification callbacks. */
  MeshCfgMdlSrRegisterFriendship(meshFriendStateChgCback, meshFriendNetKeyDelNotifyCback,
                              meshFriendPollTimeoutGetCback);

  /* Setup Network Rx PDU checker. */
  MeshNwkRegisterFriend(meshFriendLpnDstCheckCback);

  /* Setup LTR Friend Queue add callback. */
  MeshLtrRegisterFriend(meshFriendQueuePduAddCback);

  /* Setup SAR Rx Friend Queue add callback. */
  MeshSarRxRegisterFriend((meshSarRxLpnDstCheckCback_t)meshFriendLpnDstCheckCback,
                          meshFriendQueueSarRxPduAddCback);
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Computes the required memory to be provided based on the given configuration.
 *
 *  \return Memory required value in bytes if success or ::MESH_MEM_REQ_INVALID_CFG in case fail.
 */
/*************************************************************************************************/
uint32_t MeshFriendGetRequiredMemory(void)
{
  uint32_t memCtx = 0;
  uint32_t memFriendQueue = 0;
  uint32_t memSubscrList = 0;

  /* Check number of friendships. */
  if((GET_MAX_NUM_CTX() == 0) || (GET_MAX_NUM_QUEUE_ENTRIES() == 0) ||
     (GET_MAX_SUBSCR_LIST_SIZE() == 0))
  {
    return MESH_MEM_REQ_INVALID_CFG;
  }

  /* Compute required memory for each component */
  memCtx = GET_MAX_NUM_CTX() * sizeof(meshFriendLpnCtx_t);

  memFriendQueue = GET_MAX_NUM_CTX() * GET_MAX_NUM_QUEUE_ENTRIES() *
                   sizeof(meshFriendQueueEntry_t);
  memSubscrList = GET_MAX_NUM_CTX() * GET_MAX_SUBSCR_LIST_SIZE() * sizeof(meshAddress_t);

  /* Return total while aligning each component. */
  return MESH_UTILS_ALIGN(memCtx) + MESH_UTILS_ALIGN(memFriendQueue) +
         MESH_UTILS_ALIGN(memSubscrList);
}


/*************************************************************************************************/
/*!
 *  \brief     Initialize Friend Node memory requirements.
 *
 *  \param[in] pFreeMem     Pointer to free memory.
 *  \param[in] freeMemSize  Size of pFreeMem.
 *
 *  \return    Amount of free memory consumed.
 *
 *  \note      This function must be called once after Mesh Stack initialization.
 */
/*************************************************************************************************/
uint32_t MeshFriendMemInit(uint8_t *pFreeMem, uint32_t freeMemSize)
{
  meshAddress_t          *pSubscrList;
  meshFriendQueueEntry_t *pQueueEntry;
  uint32_t               memCtx, memFriendQueue;
  uint8_t                idx;
  uint32_t               reqMem = MeshFriendGetRequiredMemory();

  /* Check if memory is sufficient. */
  if(reqMem > freeMemSize)
  {
    return 0;
  }

  /* Compute offset for the Friend Queue pool. */
  memCtx = GET_MAX_NUM_CTX() * sizeof(meshFriendLpnCtx_t);
  /* Compute offset for the Subscription List. */
  memFriendQueue = GET_MAX_NUM_CTX() * GET_MAX_NUM_QUEUE_ENTRIES() * sizeof(meshFriendQueueEntry_t);

  /* Reserve memory for the contexts. */
  friendCb.pLpnCtxTbl = (meshFriendLpnCtx_t *)pFreeMem;

  /* Reserve memory for the Friend Queue pool */
  pQueueEntry = (meshFriendQueueEntry_t *)(pFreeMem + MESH_UTILS_ALIGN(memCtx));
  /* Reserve memory for the Subscription Lists */
  pSubscrList = (meshAddress_t *)(((uint8_t *)pQueueEntry) + MESH_UTILS_ALIGN(memFriendQueue));

  /* Configure individual pools and subscription lists. Reset each context. */
  for(idx = 0; idx < GET_MAX_NUM_CTX(); idx++)
  {
    /* Point to start address. */
    LPN_CTX_PTR(idx)->pQueuePool      = pQueueEntry;
    LPN_CTX_PTR(idx)->pSubscrAddrList = pSubscrList;

    /* Advance pointers. */
    pQueueEntry += GET_MAX_NUM_QUEUE_ENTRIES();
    pSubscrList += GET_MAX_SUBSCR_LIST_SIZE();

    /* Assign handler id to the timers. */
    LPN_CTX_PTR(idx)->pollTmr.handlerId = meshCb.handlerId;
    LPN_CTX_PTR(idx)->recvDelayTmr.handlerId = meshCb.handlerId;
    LPN_CTX_PTR(idx)->subscrCnfRecvDelayTmr.handlerId = meshCb.handlerId;
    LPN_CTX_PTR(idx)->clearPeriodTmr.handlerId = meshCb.handlerId;
    /* Set timers event. */
    LPN_CTX_PTR(idx)->pollTmr.msg.event = MESH_FRIEND_MSG_TIMEOUT;
    LPN_CTX_PTR(idx)->recvDelayTmr.msg.event = MESH_FRIEND_MSG_RECV_DELAY;
    LPN_CTX_PTR(idx)->subscrCnfRecvDelayTmr.msg.event = MESH_FRIEND_MSG_SUBSCR_CNF_DELAY;
    LPN_CTX_PTR(idx)->clearPeriodTmr.msg.event = MESH_FRIEND_MSG_CLEAR_SEND_TIMEOUT;
    /* Set timers param. */
    LPN_CTX_PTR(idx)->pollTmr.msg.param = idx;
    LPN_CTX_PTR(idx)->recvDelayTmr.msg.param = idx;
    LPN_CTX_PTR(idx)->subscrCnfRecvDelayTmr.msg.param = idx;
    LPN_CTX_PTR(idx)->clearPeriodTmr.msg.param = idx;

    /* Reset state. */
    LPN_CTX_PTR(idx)->friendSmState = FRIEND_ST_IDLE;

    /* Reset context. */
    meshFriendResetLpnCtx(idx);
  }

  /* Return used memory */
  return reqMem;
}

/*************************************************************************************************/
/*!
 *  \brief     Initialize Friend Node feature.
 *
 *  \param[in] recvWinMs    Receive Window in milliseconds.
 *
 *  \return    None
 *
 *  \note      This function and MeshLpnInit() are mutually exclusive.
 */
/*************************************************************************************************/
void MeshFriendInit(uint8_t recvWinMs)
{
  meshFriendStates_t friendState;

  /* Check if Receive Window is valid. */
  if(!MESH_FRIEND_RECV_WIN_VALID(recvWinMs))
  {
    return;
  }

  /* Setup state machine interface. */
  friendCb.pSm = &meshFriendSrSmIf;

  /* Set Receive Window. */
  friendCb.recvWindow = recvWinMs;

  /* Set internal state to disabled. */
  friendCb.state = MESH_FRIEND_FEATURE_DISABLED;

  /* Register callbacks into layers and modules. */
  meshFriendRegisterCbacks();

  /* Get Friend state. */
  friendState = MeshLocalCfgGetFriendState();

  if(friendState == MESH_FRIEND_FEATURE_NOT_SUPPORTED)
  {
    MeshLocalCfgSetFriendState(MESH_FRIEND_FEATURE_DISABLED);
  }
  else if (friendState == MESH_FRIEND_FEATURE_ENABLED)
  {
    meshFriendStateChgCback();
  }
}
