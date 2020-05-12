/*************************************************************************************************/
/*!
 *  \file   mesh_friend_act.c
 *
 *  \brief  Mesh Friend state machine actions.
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

#include <string.h>

#include "wsf_types.h"
#include "wsf_msg.h"
#include "wsf_buf.h"
#include "wsf_os.h"
#include "wsf_assert.h"
#include "sec_api.h"
#include "util/bstream.h"
#include "wsf_trace.h"

#include "mesh_defs.h"
#include "mesh_error_codes.h"
#include "mesh_types.h"
#include "mesh_api.h"
#include "mesh_main.h"
#include "mesh_local_config_types.h"
#include "mesh_local_config.h"
#include "mesh_security.h"

#include "mesh_network.h"
#include "mesh_lower_transport.h"
#include "mesh_sar_rx.h"
#include "mesh_upper_transport.h"

#include "mesh_friend.h"
#include "mesh_friend_main.h"
#include "mesh_friendship_defs.h"
#include "mesh_utils.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Converts Poll Timeout in seconds */
#define POLL_TIMEOUT_TO_SEC(timeout) (((timeout) * MESH_FRIEND_POLL_TIMEOUT_STEP_MS) / 1000)

/*! Additional drift involved in calculating delays and receive window in Friendship protocol. */
#define FRIEND_TMR_DRIFT_MS          (2 * WSF_MS_PER_TICK)

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Sends Friend Offer PDU.
 *
 *  \param[in] pCtx  Pointer to context.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshFriendSendOffer(meshFriendLpnCtx_t *pCtx)
{
  uint8_t *ptr;
  meshUtrCtlPduInfo_t utrCtlPduInfo;
  uint8_t offerPdu[MESH_FRIEND_OFFER_NUM_BYTES];
  meshUtrRetVal_t retVal;

  /* Configure PDU information. */
  MeshLocalCfgGetAddrFromElementId(0, &(utrCtlPduInfo.src));
  utrCtlPduInfo.dst = pCtx->lpnAddr;
  utrCtlPduInfo.ttl = 0;
  utrCtlPduInfo.netKeyIndex = pCtx->netKeyIndex;
  utrCtlPduInfo.friendLpnAddr = MESH_ADDR_TYPE_UNASSIGNED;
  utrCtlPduInfo.ifPassthr = TRUE;
  utrCtlPduInfo.prioritySend = FALSE;
  utrCtlPduInfo.ackRequired = FALSE;
  utrCtlPduInfo.opcode = MESH_UTR_CTL_FRIEND_OFFER_OPCODE;
  utrCtlPduInfo.pCtlPdu = offerPdu;
  utrCtlPduInfo.pduLen = sizeof(offerPdu);

  /* Build Friend Offer PDU. */
  ptr = offerPdu;

  /* Set Receive Window. */
  UINT8_TO_BSTREAM(ptr, friendCb.recvWindow);

  /* Set queue size. */
  UINT8_TO_BSTREAM(ptr, GET_MAX_NUM_QUEUE_ENTRIES());

  /* Set Subscription list size. */
  UINT8_TO_BSTREAM(ptr, GET_MAX_SUBSCR_LIST_SIZE());

  /* Set RSSI measured on Friend Request. */
  UINT8_TO_BSTREAM(ptr, pCtx->estabInfo.reqRssi);

  /* Set friend counter. */
  UINT16_TO_BE_BUF(ptr, pCtx->estabInfo.friendCounter);

  /* Send PDU. */
  retVal = MeshUtrSendCtlPdu((const meshUtrCtlPduInfo_t *)&utrCtlPduInfo);

  WSF_ASSERT(retVal == MESH_SUCCESS);
  (void)retVal;
}

/*************************************************************************************************/
/*!
 *  \brief     Sends Friend Clear PDU.
 *
 *  \param[in] pCtx  Pointer to context.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshFriendSendClear(meshFriendLpnCtx_t *pCtx)
{
  meshUtrCtlPduInfo_t utrCtlPduInfo;
  uint8_t *pPdu;
  uint8_t clearPdu[MESH_FRIEND_CLEAR_NUM_BYTES];
  meshUtrRetVal_t retVal;

  /* Configure PDU information. */
  MeshLocalCfgGetAddrFromElementId(0, &(utrCtlPduInfo.src));

  /* Set previous Friend as destination. */
  utrCtlPduInfo.dst = pCtx->estabInfo.prevFriendAddr;
  utrCtlPduInfo.ttl = 0;
  utrCtlPduInfo.netKeyIndex = pCtx->netKeyIndex;
  /* Use master credentials. */
  utrCtlPduInfo.friendLpnAddr = MESH_ADDR_TYPE_UNASSIGNED;
  utrCtlPduInfo.ifPassthr = TRUE;
  utrCtlPduInfo.prioritySend = FALSE;
  utrCtlPduInfo.ackRequired = FALSE;
  utrCtlPduInfo.opcode = MESH_UTR_CTL_FRIEND_CLEAR_OPCODE;
  utrCtlPduInfo.pCtlPdu = clearPdu;
  utrCtlPduInfo.pduLen = sizeof(clearPdu);

  pPdu = clearPdu;

  /* Set LPN Address and Counter. */
  UINT16_TO_BE_BSTREAM(pPdu, pCtx->lpnAddr);
  UINT16_TO_BE_BUF(pPdu, pCtx->estabInfo.lpnCounter);

  /* Send PDU. */
  retVal = MeshUtrSendCtlPdu((const meshUtrCtlPduInfo_t *)&utrCtlPduInfo);

  WSF_ASSERT(retVal == MESH_SUCCESS);
  (void)retVal;
}

/*************************************************************************************************/
/*!
 *  \brief     Sends Friend Clear Confirm PDU.
 *
 *  \param[in] pCtx           Pointer to context.
 *  \param[in] dst            Destination for the PDU (new Friend).
 *  \param[in] newLpnCounter  LPN Counter of the new relationship.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshFriendSendClearCnf(meshFriendLpnCtx_t *pCtx, meshAddress_t dst,
                                   uint16_t newLpnCounter)
{
  meshUtrCtlPduInfo_t utrCtlPduInfo;
  uint8_t *pPdu;
  uint8_t clearCnfPdu[MESH_FRIEND_CLEAR_CNF_NUM_BYTES];
  meshUtrRetVal_t retVal;

  /* Configure PDU information. */
  MeshLocalCfgGetAddrFromElementId(0, &(utrCtlPduInfo.src));

  /* Set new Friend as destination. */
  utrCtlPduInfo.dst = dst;
  utrCtlPduInfo.ttl = 0;
  utrCtlPduInfo.netKeyIndex = pCtx->netKeyIndex;
  /* Use master credentials. */
  utrCtlPduInfo.friendLpnAddr = MESH_ADDR_TYPE_UNASSIGNED;
  utrCtlPduInfo.ifPassthr = TRUE;
  utrCtlPduInfo.prioritySend = FALSE;
  utrCtlPduInfo.ackRequired = FALSE;
  utrCtlPduInfo.opcode = MESH_UTR_CTL_FRIEND_CLEAR_CNF_OPCODE;
  utrCtlPduInfo.pCtlPdu = clearCnfPdu;
  utrCtlPduInfo.pduLen = sizeof(clearCnfPdu);

  pPdu = clearCnfPdu;

  /* Set LPN Address and Counter. */
  UINT16_TO_BE_BSTREAM(pPdu, pCtx->lpnAddr);
  UINT16_TO_BE_BUF(pPdu, newLpnCounter);

  /* Send PDU. */
  retVal = MeshUtrSendCtlPdu((const meshUtrCtlPduInfo_t *)&utrCtlPduInfo);

  WSF_ASSERT(retVal == MESH_SUCCESS);
  (void)retVal;
}


/*************************************************************************************************/
/*!
 *  \brief     Sends Friend Subscription List confirm PDU.
 *
 *  \param[in] pCtx  Pointer to context.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshFriendSendSubscrListCnf(meshFriendLpnCtx_t *pCtx)
{
  meshUtrCtlPduInfo_t utrCtlPduInfo;
  uint8_t cnfPdu[MESH_FRIEND_SUBSCR_LIST_CNF_NUM_BYTES];
  meshUtrRetVal_t retVal;

  /* Configure PDU information. */
  MeshLocalCfgGetAddrFromElementId(0, &(utrCtlPduInfo.src));
  utrCtlPduInfo.dst = pCtx->lpnAddr;
  utrCtlPduInfo.ttl = 0;
  utrCtlPduInfo.netKeyIndex = pCtx->netKeyIndex;
  utrCtlPduInfo.friendLpnAddr = pCtx->lpnAddr;
  utrCtlPduInfo.ifPassthr = TRUE;
  utrCtlPduInfo.prioritySend = TRUE;
  utrCtlPduInfo.ackRequired = FALSE;
  utrCtlPduInfo.opcode = MESH_UTR_CTL_FRIEND_SUBSCR_LIST_CNF_OPCODE;
  utrCtlPduInfo.pCtlPdu = cnfPdu;
  utrCtlPduInfo.pduLen = sizeof(cnfPdu);

  /* Set transaction number. */
  cnfPdu[MESH_FRIEND_SUBSCR_LIST_CNF_TRAN_NUM_OFFSET] = pCtx->transNum;

  /* Send PDU. */
  retVal = MeshUtrSendCtlPdu((const meshUtrCtlPduInfo_t *)&utrCtlPduInfo);

  WSF_ASSERT(retVal == MESH_SUCCESS);
  (void)retVal;
}

/*************************************************************************************************/
/*!
 *  \brief     Adds addresses to an LPN subscription list.
 *
 *  \param[in] pCtx            Pointer to context.
 *  \param[in] pSubscrListMsg  Pointer to Subscription List message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshFriendAddToSubscrList(meshFriendLpnCtx_t *pCtx,
                                      meshFriendSubscrList_t *pSubscrListMsg)
{
  uint8_t i,j, emptyIdx;

  /* Check if there are no addresses. This means same Transaction Number. */
  if(pSubscrListMsg->listSize == 0)
  {
    return;
  }

  /* Iterate through the received list. */
  for(i = 0; i < pSubscrListMsg->listSize; i++)
  {
    emptyIdx = GET_MAX_SUBSCR_LIST_SIZE();
    /* Iterate through the local list. */
    for(j = 0; j < GET_MAX_SUBSCR_LIST_SIZE(); j++)
    {
      /* Search for duplicates. */
      if(pSubscrListMsg->pSubscrList[i] == pCtx->pSubscrAddrList[j])
      {
        break;
      }

      /* Search for empty entries. */
      if((emptyIdx == GET_MAX_SUBSCR_LIST_SIZE()) &&
         (pCtx->pSubscrAddrList[j] == MESH_ADDR_TYPE_UNASSIGNED))
      {
        emptyIdx = j;
      }
    }
    /* Check if no duplicate found. */
    if(j == GET_MAX_SUBSCR_LIST_SIZE())
    {
      /* Check if there is room to add address. */
      if(emptyIdx != GET_MAX_SUBSCR_LIST_SIZE())
      {
        pCtx->pSubscrAddrList[emptyIdx] = pSubscrListMsg->pSubscrList[i];
      }
      else
      {
        /* No point continuing since the list is already full. */
        return;
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Removes addresses from an LPN subscription list.
 *
 *  \param[in] pCtx            Pointer to context.
 *  \param[in] pSubscrListMsg  Pointer to Subscription List message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshFriendRmFromSubscrList(meshFriendLpnCtx_t *pCtx,
                                       meshFriendSubscrList_t *pSubscrListMsg)
{
  uint8_t i,j;
  /* Check if there are no addresses. This means same Transaction Number. */
  if(pSubscrListMsg->listSize == 0)
  {
    return;
  }

  /* Iterate through the received list. */
  for(i = 0; i < pSubscrListMsg->listSize; i++)
  {
    /* Iterate through the local list. */
    for(j = 0; j < GET_MAX_SUBSCR_LIST_SIZE(); j++)
    {
      /* Check if addresses match. */
      if(pSubscrListMsg->pSubscrList[i] == pCtx->pSubscrAddrList[j])
      {
        /* Mark slot as free. */
        pCtx->pSubscrAddrList[j] = MESH_ADDR_TYPE_UNASSIGNED;
        break;
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security friendship credentials derivation complete callback.
 *
 *  \param[in] friendAddr   The address of the friend node.
 *  \param[in] lpnAddr      The address of the low power node.
 *  \param[in] netKeyIndex  Global network key index.
 *  \param[in] isSuccess    TRUE if operation is successful.
 *  \param[in] pParam       Pointer to generic callback parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshFriendCredDerivCompleteCback(meshAddress_t friendAddr, meshAddress_t lpnAddr,
                                             uint16_t netKeyIndex, bool_t isSuccess, void *pParam)
{
  meshFriendLpnCtx_t *pCtx = (meshFriendLpnCtx_t *)pParam;
  wsfMsgHdr_t *pMsg;

  WSF_ASSERT(pCtx->inUse);
  WSF_ASSERT(pCtx->lpnAddr == lpnAddr);
  WSF_ASSERT(pCtx->netKeyIndex == netKeyIndex);

  /* Allocate message with key derivation status. */
  if((pMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t))) != NULL)
  {
    pMsg->event = isSuccess ? MESH_FRIEND_MSG_KEY_DERIV_SUCCESS : MESH_FRIEND_MSG_KEY_DERIV_FAILED;
    pMsg->param = LPN_CTX_IDX(pCtx);

    WsfMsgSend(meshCb.handlerId, pMsg);
  }
  (void)friendAddr;
  (void)lpnAddr;
  (void)netKeyIndex;
}

/*************************************************************************************************/
/*!
 *  \brief     No action.
 *
 *  \param[in] pCtx  Pointer to LPN context entry.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshFriendActNone(meshFriendLpnCtx_t *pCtx, void *pMsg)
{
  (void)pCtx;
  (void)pMsg;
  MESH_TRACE_INFO0("MESH FRIEND: [ACT] No action on state change.");
}

/*************************************************************************************************/
/*!
 *  \brief     Free context.
 *
 *  \param[in] pCtx  Pointer to LPN context entry.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshFriendActDealloc(meshFriendLpnCtx_t *pCtx, void *pMsg)
{
  (void)pMsg;
  /* Reset entry. */
  meshFriendResetLpnCtx(LPN_CTX_IDX(pCtx));
}

/*************************************************************************************************/
/*!
 *  \brief     Prepares Key material.
 *
 *  \param[in] pCtx  Pointer to LPN context entry.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshFriendActPrepKeyMat(meshFriendLpnCtx_t *pCtx, void *pMsg)
{
  meshFriendReq_t *pMsgReq = (meshFriendReq_t *)pMsg;
  meshSecFriendshipCred_t friendCred;

  /* Configure address and NetKey Index. */
  pCtx->lpnAddr = pMsgReq->lpnAddr;
  pCtx->netKeyIndex = pMsgReq->netKeyIndex;

  /* Configure establishment info. */
  pCtx->estabInfo.friendCounter = friendCb.friendCounter++;
  pCtx->estabInfo.lpnCounter = pMsgReq->lpnCounter;
  pCtx->estabInfo.numElements = pMsgReq->numElements;
  pCtx->estabInfo.pollTimeout = pMsgReq->pollTimeout;
  pCtx->estabInfo.prevFriendAddr = pMsgReq->prevAddr;
  pCtx->estabInfo.recvDelay = pMsgReq->recvDelay;
  pCtx->estabInfo.reqRssi = pMsgReq->rssi;

  /* Prepare security material. */
  MeshLocalCfgGetAddrFromElementId(0, &(friendCred.friendAddres));
  friendCred.lpnAddress = pCtx->lpnAddr;
  friendCred.friendCounter = pCtx->estabInfo.friendCounter;
  friendCred.lpnCounter = pCtx->estabInfo.lpnCounter;
  friendCred.netKeyIndex = pCtx->netKeyIndex;

  /* Request material derivation. */
  if(MeshSecAddFriendCred((const meshSecFriendshipCred_t *) &friendCred,
                          meshFriendCredDerivCompleteCback, pCtx) != MESH_SUCCESS)
  {
    /* Simulate adding credentials failed. */
    if((pMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t))) != NULL)
    {
      ((wsfMsgHdr_t *)pMsg)->event = MESH_FRIEND_MSG_KEY_DERIV_FAILED;
      ((wsfMsgHdr_t *)pMsg)->param = LPN_CTX_IDX(pCtx);
      WsfMsgSend(meshCb.handlerId, pMsg);
    }
    return;
  }

  /* Start offer delay timer. Use the receive delay timer. */
  WsfTimerStartMs(&(pCtx->recvDelayTmr), pMsgReq->localDelay + FRIEND_TMR_DRIFT_MS);
}

/*************************************************************************************************/
/*!
 *  \brief     Sends Friend Offer as part of Friendship establishment.
 *
 *  \param[in] pCtx  Pointer to LPN context entry.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshFriendActSendOffer(meshFriendLpnCtx_t *pCtx, void *pMsg)
{
  WSF_ASSERT(((wsfMsgHdr_t *)pMsg)->event == MESH_FRIEND_MSG_RECV_DELAY);

  /* Send Friend offer. */
  meshFriendSendOffer(pCtx);

  /* Start establishment timer. */
  WsfTimerStartSec(&(pCtx->pollTmr), 1);

  (void)pMsg;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles procedures required at the start of a new friendship.
 *
 *  \param[in] pCtx  Pointer to LPN context entry.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshFriendActSetupFriendship(meshFriendLpnCtx_t *pCtx, void *pMsg)
{
  meshFriendPoll_t *pMsgPoll = (meshFriendPoll_t *)pMsg;
  meshAddress_t elem0Addr;

  WSF_ASSERT(pMsgPoll->hdr.event == MESH_FRIEND_MSG_POLL_RECV);

  /* Prepare receive delay timer to send update. */
  meshFriendActStartRecvDelay(pCtx, pMsg);

  /* Check if there was a previous friend. */
  if(((wsfMsgHdr_t *)pMsg)->event == MESH_FRIEND_MSG_POLL_RECV)
  {
    if(!MESH_IS_ADDR_UNASSIGNED(pCtx->estabInfo.prevFriendAddr))
    {
      /* Check if previous friend was not this node. */
      MeshLocalCfgGetAddrFromElementId(0, &elem0Addr);

      if(elem0Addr != pCtx->estabInfo.prevFriendAddr)
      {
        /* Configure repeat timer. Start from 1 second. */
        pCtx->clearPeriodTimeSec = 1;

        /* Check if procedure should start. */
        if(pCtx->clearPeriodTimeSec < 2 * POLL_TIMEOUT_TO_SEC(pCtx->estabInfo.pollTimeout))
        {
          WsfTimerStartSec(&(pCtx->clearPeriodTmr), pCtx->clearPeriodTimeSec);
        }

        /* Start sending Friend Clear messages. */
        meshFriendSendClear(pCtx);
      }
    }
  }

  (void)pMsgPoll;
}

/*************************************************************************************************/
/*!
 *  \brief     Starts receive delay timer.
 *
 *  \param[in] pCtx  Pointer to LPN context entry.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshFriendActStartRecvDelay(meshFriendLpnCtx_t *pCtx, void *pMsg)
{
  meshFriendPoll_t *pMsgPoll = (meshFriendPoll_t *)pMsg;
  uint8_t nextFsn;

  WSF_ASSERT(pMsgPoll->hdr.event == MESH_FRIEND_MSG_POLL_RECV);

  /* If first Poll, save FSN. */
  if(pCtx->crtNextFsn == FRIEND_CRT_NEXT_FSN_INIT_VAL)
  {
    pCtx->crtNextFsn = 0;

    /* Set current FSN. */
    MESH_UTILS_BF_SET(pCtx->crtNextFsn, pMsgPoll->fsn, FRIEND_FSN_CRT_SHIFT, 1);

    /* Set next FSN. */
    nextFsn = pMsgPoll->fsn;
  }
  else
  {
    /* Set next FSN. */
    nextFsn = pMsgPoll->fsn;
  }

  /* Set next FSN. */
  MESH_UTILS_BF_SET(pCtx->crtNextFsn, nextFsn, FRIEND_FSN_NEXT_SHIFT, 1);

  /* Start Receive Delay timer. */
  WsfTimerStartMs(&(pCtx->recvDelayTmr), pCtx->estabInfo.recvDelay + FRIEND_TMR_DRIFT_MS);

  /* Restart Poll Timer. */
  WsfTimerStartMs(&(pCtx->pollTmr),
                  (pCtx->estabInfo.pollTimeout * MESH_FRIEND_POLL_TIMEOUT_STEP_MS) +
                  FRIEND_TMR_DRIFT_MS);
}

/*************************************************************************************************/
/*!
 *  \brief     Sends Next PDU from the queue.
 *
 *  \param[in] pCtx  Pointer to LPN context entry.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshFriendActSendNextPdu(meshFriendLpnCtx_t *pCtx, void *pMsg)
{
  uint8_t crtFsn, nextFsn;

  crtFsn  = MESH_UTILS_BF_GET(pCtx->crtNextFsn, FRIEND_FSN_CRT_SHIFT, 1);
  nextFsn = MESH_UTILS_BF_GET(pCtx->crtNextFsn, FRIEND_FSN_NEXT_SHIFT, 1);

  /* Check PDU needs re-send. */
  if(crtFsn != nextFsn)
  {
    /* Last PDU sent is acknowledged. Time to remove it. */
    meshFriendQueueRmAckPendPdu(pCtx);

    /* Set current FSN. */
    MESH_UTILS_BF_SET(pCtx->crtNextFsn, nextFsn, FRIEND_FSN_CRT_SHIFT, 1);
  }

  /* Call function to send PDU. */
  meshFriendQueueSendNextPdu(pCtx);

  (void)pMsg;
}

/*************************************************************************************************/
/*!
 *  \brief     Sends Friend Subscription List Confirm PDU.
 *
 *  \param[in] pCtx  Pointer to LPN context entry.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshFriendActSendSubscrCnf(meshFriendLpnCtx_t *pCtx, void *pMsg)
{
  /* Send Subscription List Confirm. */
  meshFriendSendSubscrListCnf(pCtx);

  (void)pMsg;
}

/*************************************************************************************************/
/*!
 *  \brief     Terminates friendship.
 *
 *  \param[in] pCtx  Pointer to LPN context entry.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshFriendActTerminate(meshFriendLpnCtx_t *pCtx, void *pMsg)
{
  meshAddress_t elem0Addr;

  MeshLocalCfgGetAddrFromElementId(0, &elem0Addr);

  /* Optimize not to clean up material on derivation failed. */
  if(((wsfMsgHdr_t *)pMsg)->event != MESH_FRIEND_MSG_KEY_DERIV_FAILED)
  {
    /* Remove Friendship material. */
    (void)MeshSecRemoveFriendCred(elem0Addr, pCtx->lpnAddr, pCtx->netKeyIndex);
  }

  /* Check if termination is due to a Friend Clear message. */
  if(((wsfMsgHdr_t *)pMsg)->event == MESH_FRIEND_MSG_FRIEND_CLEAR_RECV)
  {
    /* Send Clear Confirm. */
    meshFriendSendClearCnf(pCtx, ((meshFriendClear_t *)pMsg)->friendAddr,
                           ((meshFriendClear_t *)pMsg)->lpnCounter);
  }

  /* Reset context. */
  meshFriendResetLpnCtx(LPN_CTX_IDX(pCtx));

  (void)pMsg;
}

/*************************************************************************************************/
/*!
 *  \brief     Notifies another friend that a friendship is over.
 *
 *  \param[in] pCtx  Pointer to LPN context entry.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshFriendActNotifyFriend(meshFriendLpnCtx_t *pCtx, void *pMsg)
{
  /* Configure repeat timer. Double the interval. */
  pCtx->clearPeriodTimeSec = (pCtx->clearPeriodTimeSec << 1);

  /* Check if procedure should continue. */
  if(pCtx->clearPeriodTimeSec < 2 * POLL_TIMEOUT_TO_SEC(pCtx->estabInfo.pollTimeout))
  {
    WsfTimerStartSec(&(pCtx->clearPeriodTmr), pCtx->clearPeriodTimeSec);
  }

  /* Send Friend Clear messages. */
  meshFriendSendClear(pCtx);

  (void)pMsg;
}

/*************************************************************************************************/
/*!
 *  \brief     Stops notifying another friend that a friendship is over.
 *
 *  \param[in] pCtx  Pointer to LPN context entry.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshFriendActStopNotifyFriend(meshFriendLpnCtx_t *pCtx, void *pMsg)
{
  meshFriendClearCnf_t *pMsgClrCnf = (meshFriendClearCnf_t *)pMsg;

  WSF_ASSERT(pMsgClrCnf->hdr.event == MESH_FRIEND_MSG_FRIEND_CLEAR_CNF_RECV);

  if(pMsgClrCnf->hdr.event == MESH_FRIEND_MSG_FRIEND_CLEAR_CNF_RECV)
  {
    /* Validate parameters. */
    if((pCtx->lpnAddr == pMsgClrCnf->lpnAddr) &&
       (pCtx->estabInfo.lpnCounter == pMsgClrCnf->lpnCounter) &&
       (pCtx->estabInfo.prevFriendAddr == pMsgClrCnf->friendAddr))
    {
      /* Stop period timer. */
      WsfTimerStop(&(pCtx->clearPeriodTmr));
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Updates subscription list for the LPN.
 *
 *  \param[in] pCtx  Pointer to LPN context entry.
 *  \param[in] pMsg  State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshFriendActUpdateSubscrList(meshFriendLpnCtx_t *pCtx, void *pMsg)
{
  meshFriendSubscrList_t *pSubscrListMsg = (meshFriendSubscrList_t *)pMsg;

  /* Start Subscription List Confirm Receive Delay timer. */
  WsfTimerStartMs(&(pCtx->subscrCnfRecvDelayTmr), pCtx->estabInfo.recvDelay + FRIEND_TMR_DRIFT_MS);

  /* Restart Poll Timer. */
  WsfTimerStartMs(&(pCtx->pollTmr),
                  (pCtx->estabInfo.pollTimeout * MESH_FRIEND_POLL_TIMEOUT_STEP_MS) +
                  FRIEND_TMR_DRIFT_MS);

  /* Update transaction number. */
  pCtx->transNum = pSubscrListMsg->transNum;

  /* Handle Subscription List. */
  if(pSubscrListMsg->hdr.event == MESH_FRIEND_MSG_SUBSCR_LIST_ADD)
  {
    meshFriendAddToSubscrList(pCtx, pSubscrListMsg);
  }
  else
  {
    meshFriendRmFromSubscrList(pCtx, pSubscrListMsg);
  }
}
