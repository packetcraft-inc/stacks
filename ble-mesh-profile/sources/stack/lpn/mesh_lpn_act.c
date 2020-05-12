/*************************************************************************************************/
/*!
 *  \file   mesh_lpn_act.c
 *
 *  \brief  Mesh LPN state machine actions.
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
#include "wsf_buf.h"
#include "wsf_os.h"
#include "wsf_timer.h"
#include "wsf_assert.h"
#include "sec_api.h"
#include "util/bstream.h"
#include "wsf_trace.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_error_codes.h"
#include "mesh_local_config.h"
#include "mesh_main.h"
#include "mesh_friendship_defs.h"
#include "mesh_lpn.h"
#include "mesh_lpn_api.h"
#include "mesh_lpn_main.h"
#include "mesh_network_mgmt.h"
#include "mesh_lower_transport.h"
#include "mesh_upper_transport.h"
#include "mesh_upper_transport_heartbeat.h"
#include "mesh_security_toolbox.h"
#include "mesh_security.h"
#include "mesh_utils.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Friend Offer Receive Delay */
#define MESH_LPN_FRIEND_OFFER_RECV_DELAY_MS         100

/*! Friend Offer Receive Window */
#define MESH_LPN_FRIEND_OFFER_RECV_WIN_MS           1000

/*! LPN TX offset for reseding Friend Poll PDU */
#define MESH_LPN_TX_JITTER                          10

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security friendship credentials derivation complete callback.
 *
 *  \param[in] friendAddress  The address of the friend node.
 *  \param[in] lpnAddress     The address of the low power node.
 *  \param[in] netKeyIndex    Global network key index.
 *  \param[in] isSuccess      TRUE if operation is successful.
 *  \param[in] pParam         Pointer to generic callback parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSecFriendCredDerivCback(meshAddress_t friendAddress, meshAddress_t lpnAddress,
                                        uint16_t netKeyIndex, bool_t isSuccess, void *pParam)
{
  wsfMsgHdr_t *pMsg;
  uint8_t ctxIdx;

  ctxIdx = meshLpnCtxIdxByNetKeyIndex(netKeyIndex);

  WSF_ASSERT(ctxIdx != MESH_LPN_INVALID_CTX_IDX);

  /* Terminate Friendship */
  if ((pMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t))) != NULL)
  {
    if (isSuccess)
    {
      pMsg->event = MESH_LPN_MSG_SEND_FRIEND_POLL;
      lpnCb.pLpnTbl[ctxIdx].friendAddr = friendAddress;
      lpnCb.pLpnTbl[ctxIdx].txRetryCount = MESH_LPN_TX_NUM_RETRIES;
    }
    else
    {
      pMsg->event = MESH_LPN_MSG_TERMINATE;
    }

    pMsg->param = ctxIdx;

    /* Send Message. */
    WsfMsgSend(meshCb.handlerId, pMsg);
  }

  (void)lpnAddress;
  (void)pParam;
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     No action.
 *
 *  \param[in] pLpnCtx  LPN context.
 *  \param[in] pMsg     State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshLpnActNone(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg)
{
  (void)pLpnCtx;
  (void)pMsg;
  MESH_TRACE_INFO0("MESH LPN: [ACT] No action on state change.");
}

/*************************************************************************************************/
/*!
 *  \brief     Terminate Friendship.
 *
 *  \param[in] pLpnCtx  LPN context.
 *  \param[in] pMsg     State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshLpnActTerminateFriendship(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg)
{
  void *pReq;
  uint8_t *pPdu;
  meshUtrCtlPduInfo_t ctlPduInfo;
  meshLpnFriendshipTerminatedEvt_t evtMsg;
  meshAddress_t elem0Addr;
  uint8_t ctlPdu[MESH_FRIEND_CLEAR_NUM_BYTES];
  uint8_t i;
  bool_t established = FALSE;

  /* Stop timers. */
  WsfTimerStop(&pLpnCtx->lpnTimer);
  WsfTimerStop(&pLpnCtx->pollTimer);

  /* Check if Friend Clear needs to be sent. */
  if ((pMsg->hdr.event == MESH_LPN_MSG_SEND_FRIEND_CLEAR) &&
      (pLpnCtx->friendAddr != MESH_ADDR_TYPE_UNASSIGNED))
  {
    /* Set primary element address as source address. */
    MeshLocalCfgGetAddrFromElementId(0, &(ctlPduInfo.src));

    /* Set destination address. */
    ctlPduInfo.dst = pLpnCtx->friendAddr;

    /* Set netKey index. */
    ctlPduInfo.netKeyIndex = pLpnCtx->netKeyIndex;

    /* Set TTL. */
    ctlPduInfo.ttl = 0;

    /* Set OpCode. */
    ctlPduInfo.opcode = MESH_UTR_CTL_FRIEND_CLEAR_OPCODE;

    /* Set ACK Required to false. */
    ctlPduInfo.ackRequired = FALSE;

    pPdu = ctlPdu;

    /* Set LPN Address and Counter. */
    UINT16_TO_BE_BSTREAM(pPdu, ctlPduInfo.src);
    UINT16_TO_BE_BUF(pPdu, pLpnCtx->lpnCounter);

    /* Add control PDU to PDU info. */
    ctlPduInfo.pCtlPdu = ctlPdu;
    ctlPduInfo.pduLen = sizeof(ctlPdu);

    /* Set priority to FALSE. */
    ctlPduInfo.prioritySend = FALSE;

    /* This message is sent with master credentials. */
    ctlPduInfo.friendLpnAddr = MESH_ADDR_TYPE_UNASSIGNED;

    ctlPduInfo.ifPassthr = TRUE;

    /* Send CTL PDU. */
    MeshUtrSendCtlPdu(&ctlPduInfo);
  }

  /* Clear credentials. */
  MeshLocalCfgGetAddrFromElementId(0, &elem0Addr);
  MeshSecRemoveFriendCred(pLpnCtx->friendAddr, elem0Addr, pLpnCtx->netKeyIndex);

  /* Clear established flag. */
  pLpnCtx->established = FALSE;

  while (!WsfQueueEmpty(&(pLpnCtx->subscrListQueue)))
  {
    pReq = WsfQueueDeq(&(pLpnCtx->subscrListQueue));

    WsfBufFree(pReq);
  }

  /* Dealloc context. */
  meshLpnCtxDealloc(pLpnCtx);

  /* Check if there's any other friendship established. */
  for (i = 0; i < lpnCb.maxNumFriendships; i++)
  {
    if (lpnCb.pLpnTbl[i].inUse && lpnCb.pLpnTbl[i].established)
    {
      established = TRUE;

      break;
    }
  }

  /* Check if feature is enabled. */
  if (!established && (MeshLocalCfgGetLowPowerState() == MESH_LOW_POWER_FEATURE_ENABLED))
  {
    /* Set Low Power feature disabled. */
    MeshLocalCfgSetLowPowerState(MESH_LOW_POWER_FEATURE_DISABLED);

    /* Signal Feature state changed. */
    MeshHbFeatureStateChanged(MESH_FEAT_LOW_POWER);
  }

  /* Notify upper layer that Friendship has been terminated. */
  evtMsg.hdr.event = MESH_LPN_EVENT;
  evtMsg.hdr.param = MESH_LPN_FRIENDSHIP_TERMINATED_EVENT;
  evtMsg.hdr.status = MESH_SUCCESS;
  evtMsg.netKeyIndex = pLpnCtx->netKeyIndex;
  lpnCb.lpnEvtNotifyCback((meshLpnEvt_t *)&evtMsg);

  (void)pMsg;
}

/*************************************************************************************************/
/*!
 *  \brief     Sends Friend Request PDU.
 *
 *  \param[in] pLpnCtx  LPN context.
 *  \param[in] pMsg     State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshLpnActSendFriendReq(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg)
{
  meshUtrCtlPduInfo_t ctlPduInfo;
  uint8_t ctlPdu[MESH_FRIEND_REQUEST_NUM_BYTES];
  uint32_t pollTimeout;

  /* Set primary element address as source address. */
  MeshLocalCfgGetAddrFromElementId(0, &ctlPduInfo.src);

  /* Set destination address. */
  ctlPduInfo.dst = MESH_ADDR_GROUP_FRIEND;

  /* Set netKey index. */
  ctlPduInfo.netKeyIndex = pLpnCtx->netKeyIndex;

  /* Set TTL. */
  ctlPduInfo.ttl = 0;

  /* Set OpCode. */
  ctlPduInfo.opcode = MESH_UTR_CTL_FRIEND_REQUEST_OPCODE;

  /* Set ACK Required to false. */
  ctlPduInfo.ackRequired = FALSE;

  /* Set Criteria value. */
  ctlPdu[MESH_FRIEND_REQUEST_CRITERIA_OFFSET] = 0;

  MESH_UTILS_BF_SET(ctlPdu[MESH_FRIEND_REQUEST_CRITERIA_OFFSET], pLpnCtx->criteria.rssiFactor,
                    MESH_FRIEND_REQUEST_RSSI_FACTOR_SHIFT, MESH_FRIEND_REQUEST_RSSI_FACTOR_SIZE);

  MESH_UTILS_BF_SET(ctlPdu[MESH_FRIEND_REQUEST_CRITERIA_OFFSET], pLpnCtx->criteria.recvWinFactor,
                    MESH_FRIEND_REQUEST_RECV_WIN_FACTOR_SHIFT,
                    MESH_FRIEND_REQUEST_RECV_WIN_FACTOR_SIZE);

  MESH_UTILS_BF_SET(ctlPdu[MESH_FRIEND_REQUEST_CRITERIA_OFFSET], pLpnCtx->criteria.minQueueSizeLog,
                    MESH_FRIEND_REQUEST_MIN_QUEUE_SIZE_SHIFT,
                    MESH_FRIEND_REQUEST_MIN_QUEUE_SIZE_SIZE);

  /* Set receive delay value. */
  ctlPdu[MESH_FRIEND_REQUEST_RECV_DELAY_OFFSET] = pLpnCtx->recvDelayMs;

  pollTimeout = (pLpnCtx->sleepDurationMs + (pLpnCtx->txRetryCount + 1) *
                 (pLpnCtx->recvDelayMs + MESH_FRIEND_RECV_WIN_MS_MAX)) /
                MESH_FRIEND_POLL_TIMEOUT_STEP_MS;

  /* Set poll timeout value. */
  UINT24_TO_BE_BUF(&ctlPdu[MESH_FRIEND_REQUEST_POLL_TIMEOUT_OFFSET], pollTimeout);

  /* Set previous address value. */
  UINT16_TO_BE_BUF(&ctlPdu[MESH_FRIEND_REQUEST_PREV_ADDR_OFFSET],
                   meshLpnHistorySearch(pLpnCtx->netKeyIndex));

  /* Set number of elements value. */
  ctlPdu[MESH_FRIEND_REQUEST_NUM_ELEMENTS_OFFSET] = pMeshConfig->elementArrayLen;

  /* Increment LPN counter. */
  lpnCb.lpnCounter++;

  /* Update LPN counter value. */
  pLpnCtx->lpnCounter = lpnCb.lpnCounter;

  /* Set LPN counter value. */
  UINT16_TO_BE_BUF(&ctlPdu[MESH_FRIEND_REQUEST_LPN_COUNTER_OFFSET], pLpnCtx->lpnCounter);

  /* Add control PDU to PDU info. */
  ctlPduInfo.pCtlPdu = ctlPdu;
  ctlPduInfo.pduLen = sizeof(ctlPdu);

  /* Set priority to FALSE. */
  ctlPduInfo.prioritySend = FALSE;

  /* This message is sent with master credentials. */
  ctlPduInfo.friendLpnAddr = MESH_ADDR_TYPE_UNASSIGNED;

  ctlPduInfo.ifPassthr = TRUE;

  /* Send CTL PDU. */
  MeshUtrSendCtlPdu(&ctlPduInfo);

  /* Set timer event value. */
  pLpnCtx->lpnTimer.msg.event = MESH_LPN_MSG_RECV_DELAY_TIMEOUT;

  /* Start receive delay timer */
  WsfTimerStartMs(&pLpnCtx->lpnTimer, MESH_LPN_FRIEND_OFFER_RECV_DELAY_MS);

  (void)pMsg;
}

/*************************************************************************************************/
/*!
 *  \brief     Waits Friend Offer PDU.
 *
 *  \param[in] pLpnCtx  LPN context.
 *  \param[in] pMsg     State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshLpnActWaitFriendOffer(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg)
{
  /* Set timer event value. */
  pLpnCtx->lpnTimer.msg.event = MESH_LPN_MSG_RECV_WIN_TIMEOUT;

  /* Start receive window timer */
  WsfTimerStartMs(&pLpnCtx->lpnTimer, MESH_LPN_FRIEND_OFFER_RECV_WIN_MS);

  (void)pMsg;
}

/*************************************************************************************************/
/*!
 *  \brief     Sends Friend Request PDU.
 *
 *  \param[in] pLpnCtx  LPN context.
 *  \param[in] pMsg     State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshLpnActResendFriendReq(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg)
{
  wsfMsgHdr_t *pReqMsg;

  if ((pReqMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t))) != NULL)
  {
    if (pLpnCtx->establishRetryCount)
    {
      pLpnCtx->establishRetryCount--;

      /* Re-send friend request */
      pReqMsg->event = MESH_LPN_MSG_SEND_FRIEND_REQ;
    }
    else
    {
      /* Terminate Friendship */
      pReqMsg->event = MESH_LPN_MSG_TERMINATE;
    }

    pReqMsg->param = pMsg->hdr.param;

    /* Send Message. */
    WsfMsgSend(meshCb.handlerId, pReqMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Friend Offer PDU.
 *
 *  \param[in] pLpnCtx  LPN context.
 *  \param[in] pMsg     State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshLpnActProcessFriendOffer(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg)
{
  meshSecFriendshipCred_t friendCred;

  if (((meshLpnFriendOffer_t *)pMsg)->queueSize >=
      (1 << pLpnCtx->criteria.minQueueSizeLog))
  {
    /* Accept Offer. */
    pLpnCtx->recvWinMs = ((meshLpnFriendOffer_t *)pMsg)->recvWinMs;

    friendCred.friendAddres = ((meshLpnFriendOffer_t *)pMsg)->friendAddr;
    friendCred.friendCounter = ((meshLpnFriendOffer_t *)pMsg)->friendCounter;
    friendCred.lpnCounter = pLpnCtx->lpnCounter;
    friendCred.netKeyIndex = pLpnCtx->netKeyIndex;
    MeshLocalCfgGetAddrFromElementId(0, &friendCred.lpnAddress);

    MeshSecAddFriendCred(&friendCred, meshSecFriendCredDerivCback, NULL);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends Friend Poll PDU.
 *
 *  \param[in] pLpnCtx  LPN context.
 *  \param[in] pMsg     State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshLpnActSendFriendPoll(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg)
{
  meshUtrCtlPduInfo_t ctlPduInfo;
  uint8_t ctlPdu[MESH_FRIEND_POLL_NUM_BYTES];

  /* Set primary element address as source address. */
  MeshLocalCfgGetAddrFromElementId(0, &ctlPduInfo.src);

  /* Set destination address. */
  ctlPduInfo.dst = pLpnCtx->friendAddr;

  /* Set netKey index. */
  ctlPduInfo.netKeyIndex = pLpnCtx->netKeyIndex;

  /* Set TTL. */
  ctlPduInfo.ttl = 0;

  /* Set OpCode. */
  ctlPduInfo.opcode = MESH_UTR_CTL_FRIEND_POLL_OPCODE;

  /* Set ACK Required to false. */
  ctlPduInfo.ackRequired = FALSE;

  /* Set FSN value. */
  ctlPdu[MESH_FRIEND_POLL_FSN_OFFSET] = (pLpnCtx->fsn & MESH_FRIEND_POLL_FSN_MASK);

  /* Add control PDU to PDU info. */
  ctlPduInfo.pCtlPdu = ctlPdu;
  ctlPduInfo.pduLen = sizeof(ctlPdu);

  /* Set priority to FALSE. */
  ctlPduInfo.prioritySend = FALSE;

  /* This message is sent with master credentials. */
  ctlPduInfo.friendLpnAddr = pLpnCtx->friendAddr;

  ctlPduInfo.ifPassthr = TRUE;

  /* Send CTL PDU. */
  MeshUtrSendCtlPdu(&ctlPduInfo);

  /* Set timer event value. */
  pLpnCtx->lpnTimer.msg.event = MESH_LPN_MSG_RECV_DELAY_TIMEOUT;

  /* Start receive delay timer */
  WsfTimerStartMs(&pLpnCtx->lpnTimer, pLpnCtx->recvDelayMs);

  (void)pMsg;
}

/*************************************************************************************************/
/*!
 *  \brief     Waits Friend Update PDU.
 *
 *  \param[in] pLpnCtx  LPN context.
 *  \param[in] pMsg     State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshLpnActWaitFriendUpdate(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg)
{
  /* Set timer event value. */
  pLpnCtx->lpnTimer.msg.event = MESH_LPN_MSG_RECV_WIN_TIMEOUT;

  /* Start receive window timer */
  WsfTimerStartMs(&pLpnCtx->lpnTimer, pLpnCtx->recvWinMs);

  (void)pMsg;
}

/*************************************************************************************************/
/*!
 *  \brief     Resends Friend Poll PDU.
 *
 *  \param[in] pLpnCtx  LPN context.
 *  \param[in] pMsg     State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshLpnActResendFriendPoll(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg)
{
  wsfMsgHdr_t *pReqMsg;

  if ((pReqMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t))) != NULL)
  {
    if (pLpnCtx->txRetryCount)
    {
      pLpnCtx->txRetryCount--;

      /* Re-send friend request */
      pReqMsg->event = MESH_LPN_MSG_SEND_FRIEND_POLL;
    }
    else
    {
      if (pLpnCtx->established)
      {
        /* Terminate Friendship */
        pReqMsg->event = MESH_LPN_MSG_TERMINATE;
      }
      else
      {
        if (pLpnCtx->establishRetryCount)
        {
          pLpnCtx->establishRetryCount--;

          /* Re-send friend request */
          pReqMsg->event = MESH_LPN_MSG_SEND_FRIEND_REQ;
        }
        else
        {
          /* Terminate Friendship */
          pReqMsg->event = MESH_LPN_MSG_TERMINATE;
        }
      }
    }

    pReqMsg->param = pMsg->hdr.param;

    /* Send Message. */
    WsfMsgSend(meshCb.handlerId, pReqMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles Friendship Established.
 *
 *  \param[in] pLpnCtx  LPN context.
 *  \param[in] pMsg     State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshLpnActFriendshipEstablished(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg)
{
  wsfMsgHdr_t *pReqMsg;
  meshLpnFriendshipEstablishedEvt_t evtMsg;
  meshAddress_t address;
  meshKeyRefreshStates_t refPhase;
  bool_t subscrAddressListNotEmpty = MeshLocalCfgSubscrAddressListIsNotEmpty();
  bool_t subscrVirtualAddrListNotEmpty = MeshLocalCfgSubscrVirtualAddrListIsNotEmpty();
  bool_t ivUpdate, keyRef;
  bool_t useNewKey = FALSE;

  /* Toggle FSN. */
  pLpnCtx->fsn ^= 1;

  pLpnCtx->established = TRUE;

  /* Extract Key Refresh and IV update flags. */
  keyRef =  MESH_UTILS_BITMASK_CHK(((meshLpnFriendUpdate_t *)pMsg)->flags,
                                   (1 << MESH_FRIEND_UPDATE_KEY_REFRESH_FLAG_SHIFT));
  ivUpdate =  MESH_UTILS_BITMASK_CHK(((meshLpnFriendUpdate_t *)pMsg)->flags,
                                     (1 << MESH_FRIEND_UPDATE_IV_UPDATE_FLAG_SHIFT));

  /* Get current Key Refresh Phase. */
  refPhase = MeshLocalCfgGetKeyRefreshPhaseState(pLpnCtx->netKeyIndex);

  /* Check if New Key should be used. */
  if (((refPhase == MESH_KEY_REFRESH_FIRST_PHASE) || (refPhase == MESH_KEY_REFRESH_SECOND_PHASE)) &&
      (keyRef == TRUE))
  {
    useNewKey = TRUE;
  }
  else if ((refPhase == MESH_KEY_REFRESH_SECOND_PHASE) && (keyRef == FALSE))
  {
    useNewKey = TRUE;
  }

  MeshNwkMgmtHandleBeaconData(pLpnCtx->netKeyIndex, useNewKey,
                              ((meshLpnFriendUpdate_t *)pMsg)->ivIndex, keyRef, ivUpdate);

  /* Check for subscribed addresses */
  if (subscrAddressListNotEmpty || subscrVirtualAddrListNotEmpty)
  {
    if ((pReqMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t))) != NULL)
    {
      pLpnCtx->subscrReq.nextAddressIdx = 0;
      pLpnCtx->subscrReq.nextVirtualAddrIdx = 0;
      pLpnCtx->subscrReq.addrListCount = 0;
      pLpnCtx->subscrReq.add = TRUE;

      while (MeshLocalCfgGetNextSubscrAddress(&address, &pLpnCtx->subscrReq.nextAddressIdx) == MESH_SUCCESS)
      {
        /* Check if address is group. */
        if (MESH_IS_ADDR_GROUP(address))
        {
          pLpnCtx->subscrReq.addrList[pLpnCtx->subscrReq.addrListCount] = address;

          if (++pLpnCtx->subscrReq.addrListCount == MESH_LPN_SUBSCR_LIST_REQ_MAX_ENTRIES)
          {
            break;
          }
        }
      }

      if (pLpnCtx->subscrReq.addrListCount < MESH_LPN_SUBSCR_LIST_REQ_MAX_ENTRIES)
      {
        while (MeshLocalCfgGetNextSubscrVirtualAddr(&address, &pLpnCtx->subscrReq.nextVirtualAddrIdx) == MESH_SUCCESS)
        {
          pLpnCtx->subscrReq.addrList[pLpnCtx->subscrReq.addrListCount] = address;

          if (++pLpnCtx->subscrReq.addrListCount == MESH_LPN_SUBSCR_LIST_REQ_MAX_ENTRIES)
          {
            break;
          }
        }
      }

      pReqMsg->event = MESH_LPN_MSG_SEND_FRIEND_SUBSCR_ADD_RM;
      pReqMsg->param = pMsg->hdr.param;

      WsfMsgSend(meshCb.handlerId, pReqMsg);
    }
  }
  else if (((meshLpnFriendUpdate_t *)pMsg)->md)
  {
    /* Re-send friend poll */
    pLpnCtx->lpnTimer.msg.event = MESH_LPN_MSG_SEND_FRIEND_POLL;
    pLpnCtx->lpnTimer.msg.param = pMsg->hdr.param;

    /* Start jitter timer */
    WsfTimerStartMs(&pLpnCtx->lpnTimer, MESH_LPN_TX_JITTER);
  }

  meshLpnHistoryAdd(pLpnCtx->netKeyIndex, pLpnCtx->friendAddr);

  pLpnCtx->txRetryCount = MESH_LPN_TX_NUM_RETRIES;

  if (MeshLocalCfgGetLowPowerState() == MESH_LOW_POWER_FEATURE_DISABLED)
  {
    /* Set Low Power feature enabled. */
    MeshLocalCfgSetLowPowerState(MESH_LOW_POWER_FEATURE_ENABLED);

    /* Signal Feature state changed. */
    MeshHbFeatureStateChanged(MESH_FEAT_LOW_POWER);
  }

  /* Set timer event value. */
  pLpnCtx->pollTimer.msg.event = MESH_LPN_MSG_POLL_TIMEOUT;

  /* Start poll timeout timer */
  WsfTimerStartMs(&pLpnCtx->pollTimer, pLpnCtx->sleepDurationMs);

  /* Notify upper layer that Friendship has been established. */
  evtMsg.hdr.event = MESH_LPN_EVENT;
  evtMsg.hdr.param = MESH_LPN_FRIENDSHIP_ESTABLISHED_EVENT;
  evtMsg.hdr.status = MESH_SUCCESS;
  evtMsg.netKeyIndex = pLpnCtx->netKeyIndex;
  lpnCb.lpnEvtNotifyCback((meshLpnEvt_t *)&evtMsg);
}

/*************************************************************************************************/
/*!
 *  \brief     Waits for a Friend message.
 *
 *  \param[in] pLpnCtx  LPN context.
 *  \param[in] pMsg     State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshLpnActWaitFriendMessage(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg)
{
  /* Set timer event value. */
  pLpnCtx->lpnTimer.msg.event = MESH_LPN_MSG_RECV_WIN_TIMEOUT;

  /* Start receive window timer */
  WsfTimerStartMs(&pLpnCtx->lpnTimer, pLpnCtx->recvWinMs);

  (void)pMsg;
}

/*************************************************************************************************/
/*!
 *  \brief     Process Friend Update PDU.
 *
 *  \param[in] pLpnCtx  LPN context.
 *  \param[in] pMsg     State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshLpnActProcessFriendUpdate(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg)
{
  meshLpnFriendSubscrEvent_t *ptr;
  meshKeyRefreshStates_t refPhase;
  wsfMsgHdr_t *pReqMsg;
  bool_t ivUpdate, keyRef;
  bool_t useNewKey = FALSE;

  /* Extract Key Refresh and IV update flags. */
  keyRef =  MESH_UTILS_BITMASK_CHK(((meshLpnFriendUpdate_t *)pMsg)->flags,
                                   (1 << MESH_FRIEND_UPDATE_KEY_REFRESH_FLAG_SHIFT));
  ivUpdate =  MESH_UTILS_BITMASK_CHK(((meshLpnFriendUpdate_t *)pMsg)->flags,
                                     (1 << MESH_FRIEND_UPDATE_IV_UPDATE_FLAG_SHIFT));

  /* Get current Key Refresh Phase. */
  refPhase = MeshLocalCfgGetKeyRefreshPhaseState(pLpnCtx->netKeyIndex);

  /* Check if New Key should be used. */
  if (((refPhase == MESH_KEY_REFRESH_FIRST_PHASE) || (refPhase == MESH_KEY_REFRESH_SECOND_PHASE)) &&
      (keyRef == TRUE))
  {
    useNewKey = TRUE;
  }
  else if ((refPhase == MESH_KEY_REFRESH_SECOND_PHASE) && (keyRef == FALSE))
  {
    useNewKey = TRUE;
  }

  MeshNwkMgmtHandleBeaconData(pLpnCtx->netKeyIndex, useNewKey,
                              ((meshLpnFriendUpdate_t *)pMsg)->ivIndex, keyRef, ivUpdate);

  if ((((meshLpnFriendUpdate_t *)pMsg)->md == 0) &&
      ((ptr = (meshLpnFriendSubscrEvent_t *)WsfQueueDeq(&pLpnCtx->subscrListQueue)) != NULL))
  {
    pLpnCtx->subscrReq.addrList[0] = ptr->address;
    pLpnCtx->subscrReq.add = ptr->add;
    pLpnCtx->subscrReq.addrListCount = 1;
    pLpnCtx->subscrReq.nextAddressIdx = pMeshConfig->pMemoryConfig->addrListMaxSize;
    pLpnCtx->subscrReq.nextVirtualAddrIdx = pMeshConfig->pMemoryConfig->virtualAddrListMaxSize;

    WsfBufFree(ptr);

    if ((pReqMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t))) != NULL)
    {
      pReqMsg->event = MESH_LPN_MSG_SEND_FRIEND_SUBSCR_ADD_RM;

      pLpnCtx->txRetryCount = MESH_LPN_TX_NUM_RETRIES;

      pReqMsg->param = pMsg->hdr.param;

      /* Send Message. */
      WsfMsgSend(meshCb.handlerId, pReqMsg);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Friend message.
 *
 *  \param[in] pLpnCtx  LPN context.
 *  \param[in] pMsg     State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshLpnActProcessFriendMessage(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg)
{
  WsfTimerStop(&pLpnCtx->lpnTimer);

  /* Set timer event value. */
  pLpnCtx->pollTimer.msg.event = MESH_LPN_MSG_POLL_TIMEOUT;

  /* Start poll timeout timer */
  WsfTimerStartMs(&pLpnCtx->pollTimer, pLpnCtx->sleepDurationMs);

  /* Reset Poll retry count value. */
  pLpnCtx->txRetryCount = MESH_LPN_TX_NUM_RETRIES;

  if (((meshLpnFriendRxPdu_t *)pMsg)->toggleFsn)
  {
    /* Toggle FSN. */
    pLpnCtx->fsn ^= 1;
  }

  if (((meshLpnFriendRxPdu_t *)pMsg)->md)
  {
    /* Re-send friend poll */
    pLpnCtx->lpnTimer.msg.event = MESH_LPN_MSG_SEND_FRIEND_POLL;
    pLpnCtx->lpnTimer.msg.param = pMsg->hdr.param;

    /* Start jitter timer */
    WsfTimerStartMs(&pLpnCtx->lpnTimer, MESH_LPN_TX_JITTER);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends Friend Subscription Add and Remove PDU's.
 *
 *  \param[in] pLpnCtx  LPN context.
 *  \param[in] pMsg     State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshLpnActSendFriendSubscrAddRm(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg)
{
  uint8_t *pPdu;
  meshUtrCtlPduInfo_t ctlPduInfo;
  uint8_t ctlPdu[MESH_FRIEND_SUBSCR_LIST_ADD_RM_MAX_NUM_BYTES];
  uint8_t i;

  /* Set primary element address as source address. */
  MeshLocalCfgGetAddrFromElementId(0, &ctlPduInfo.src);

  /* Set destination address. */
  ctlPduInfo.dst = pLpnCtx->friendAddr;

  /* Set netKey index. */
  ctlPduInfo.netKeyIndex = pLpnCtx->netKeyIndex;

  /* Set TTL. */
  ctlPduInfo.ttl = 0;

  /* Set OpCode. */
  ctlPduInfo.opcode = (pLpnCtx->subscrReq.add == TRUE) ? MESH_UTR_CTL_FRIEND_SUBSCR_LIST_ADD_OPCODE :
                                                         MESH_UTR_CTL_FRIEND_SUBSCR_LIST_RM_OPCODE;

  /* Set ACK Required to false. */
  ctlPduInfo.ackRequired = FALSE;

  /* Set Transaction Number value. */
  ctlPdu[MESH_FRIEND_SUBSCR_LIST_ADD_RM_TRAN_NUM_OFFSET] = pLpnCtx->tranNumber;

  pPdu = &ctlPdu[MESH_FRIEND_SUBSCR_LIST_ADD_RM_ADDR_LIST_START_OFFSET];

  /* Add addresses in CTL PDU message. */
  for (i = 0; i < pLpnCtx->subscrReq.addrListCount; i++)
  {
    UINT16_TO_BE_BSTREAM(pPdu, pLpnCtx->subscrReq.addrList[i]);
  }

  /* Add control PDU to PDU info. */
  ctlPduInfo.pCtlPdu = ctlPdu;
  ctlPduInfo.pduLen =
    MESH_FRIEND_SUBSCR_LIST_ADD_RM_NUM_BYTES(pLpnCtx->subscrReq.addrListCount);

  /* Set priority to FALSE. */
  ctlPduInfo.prioritySend = FALSE;

  /* This message is sent with friendship credentials. */
  ctlPduInfo.friendLpnAddr = pLpnCtx->friendAddr;

  ctlPduInfo.ifPassthr = TRUE;

  /* Send CTL PDU. */
  MeshUtrSendCtlPdu(&ctlPduInfo);

  /* Set timer event value. */
  pLpnCtx->lpnTimer.msg.event = MESH_LPN_MSG_RECV_DELAY_TIMEOUT;

  /* Start receive delay timer */
  WsfTimerStartMs(&pLpnCtx->lpnTimer, pLpnCtx->recvDelayMs);

  (void)pMsg;
}

/*************************************************************************************************/
/*!
 *  \brief     Resends Friend Subscription Add and Remove PDU's.
 *
 *  \param[in] pLpnCtx  LPN context.
 *  \param[in] pMsg     State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshLpnActResendFriendSubscrAddRm(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg)
{
  wsfMsgHdr_t *pReqMsg;

  if ((pReqMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t))) != NULL)
  {
    if (pLpnCtx->txRetryCount)
    {
      pLpnCtx->txRetryCount--;

      /* Re-send request */
      pReqMsg->event = MESH_LPN_MSG_SEND_FRIEND_SUBSCR_ADD_RM;
    }
    else
    {
      /* Terminate Friendship */
      pReqMsg->event = MESH_LPN_MSG_TERMINATE;
    }

    pReqMsg->param = pMsg->hdr.param;

    /* Send Message. */
    WsfMsgSend(meshCb.handlerId, pReqMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Waits Friend Subscription Confirm PDU.
 *
 *  \param[in] pLpnCtx  LPN context.
 *  \param[in] pMsg     State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshLpnActWaitFriendSubscrCnf(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg)
{
  /* Set timer event value. */
  pLpnCtx->lpnTimer.msg.event = MESH_LPN_MSG_RECV_WIN_TIMEOUT;

  /* Start receive window timer */
  WsfTimerStartMs(&pLpnCtx->lpnTimer, pLpnCtx->recvWinMs);

  (void)pMsg;
}

/*************************************************************************************************/
/*!
 *  \brief     Process Friend Subscription Confirm PDU.
 *
 *  \param[in] pLpnCtx  LPN context.
 *  \param[in] pMsg     State machine message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshLpnActProcessFriendSubscrCnf(meshLpnCtx_t *pLpnCtx, meshLpnSmMsg_t *pMsg)
{
  meshLpnFriendSubscrEvent_t *ptr;
  wsfMsgHdr_t *pReqMsg;
  meshAddress_t address;

  WsfTimerStop(&pLpnCtx->lpnTimer);

  /* Set timer event value. */
  pLpnCtx->pollTimer.msg.event = MESH_LPN_MSG_POLL_TIMEOUT;

  /* Start poll timeout timer */
  WsfTimerStartMs(&pLpnCtx->pollTimer, pLpnCtx->sleepDurationMs);

  /* Transaction confirmed. Remove. */
  if (((meshLpnFriendSubscrCnf_t *)pMsg)->tranNumber == pLpnCtx->tranNumber)
  {
    /* Increment Transaction Number. */
    pLpnCtx->tranNumber++;
    pLpnCtx->subscrReq.addrListCount = 0;
    pLpnCtx->subscrReq.add = TRUE;

    if ((pLpnCtx->subscrReq.nextAddressIdx != pMeshConfig->pMemoryConfig->addrListMaxSize) ||
        (pLpnCtx->subscrReq.nextVirtualAddrIdx != pMeshConfig->pMemoryConfig->virtualAddrListMaxSize))
    {
      if (pLpnCtx->subscrReq.nextAddressIdx != pMeshConfig->pMemoryConfig->addrListMaxSize)
      {
        while (MeshLocalCfgGetNextSubscrAddress(&address, &pLpnCtx->subscrReq.nextAddressIdx) == MESH_SUCCESS)
        {
          /* Check if address is group. */
          if (MESH_IS_ADDR_GROUP(address))
          {
            pLpnCtx->subscrReq.addrList[pLpnCtx->subscrReq.addrListCount] = address;

            if (++pLpnCtx->subscrReq.addrListCount == MESH_LPN_SUBSCR_LIST_REQ_MAX_ENTRIES)
            {
              break;
            }
          }
        }
      }

      if ((pLpnCtx->subscrReq.addrListCount < MESH_LPN_SUBSCR_LIST_REQ_MAX_ENTRIES) &&
          (pLpnCtx->subscrReq.nextVirtualAddrIdx != pMeshConfig->pMemoryConfig->virtualAddrListMaxSize))
      {
        while (MeshLocalCfgGetNextSubscrVirtualAddr(&address, &pLpnCtx->subscrReq.nextVirtualAddrIdx) == MESH_SUCCESS)
        {
          pLpnCtx->subscrReq.addrList[pLpnCtx->subscrReq.addrListCount] = address;

          if (++pLpnCtx->subscrReq.addrListCount == MESH_LPN_SUBSCR_LIST_REQ_MAX_ENTRIES)
          {
            break;
          }
        }
      }

      if ((pReqMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t))) != NULL)
      {
        /* Re-send request */
        pReqMsg->event = MESH_LPN_MSG_SEND_FRIEND_SUBSCR_ADD_RM;
        pReqMsg->param = pMsg->hdr.param;

        /* Send Message. */
        WsfMsgSend(meshCb.handlerId, pReqMsg);
      }
    }
    else if ((ptr = (meshLpnFriendSubscrEvent_t *) WsfQueueDeq(&pLpnCtx->subscrListQueue)) != NULL)
    {
      /* Note: ptr cannot be NULL here. */
      pLpnCtx->subscrReq.addrList[0] = ptr->address;
      pLpnCtx->subscrReq.add = ptr->add;
      pLpnCtx->subscrReq.addrListCount = 1;
      pLpnCtx->subscrReq.nextAddressIdx = pMeshConfig->pMemoryConfig->addrListMaxSize;
      pLpnCtx->subscrReq.nextVirtualAddrIdx = pMeshConfig->pMemoryConfig->virtualAddrListMaxSize;

      WsfBufFree(ptr);

      if ((pReqMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t))) != NULL)
      {
        /* Re-send request */
        pReqMsg->event = MESH_LPN_MSG_SEND_FRIEND_SUBSCR_ADD_RM;
        pReqMsg->param = pMsg->hdr.param;

        /* Send Message. */
        WsfMsgSend(meshCb.handlerId, pReqMsg);
      }
    }
    else
    {
      pLpnCtx->txRetryCount = MESH_LPN_TX_NUM_RETRIES;

      /* Re-send friend poll */
      pLpnCtx->lpnTimer.msg.event = MESH_LPN_MSG_SEND_FRIEND_POLL;
      pLpnCtx->lpnTimer.msg.param = pMsg->hdr.param;

      /* Start jitter timer */
      WsfTimerStartMs(&pLpnCtx->lpnTimer, MESH_LPN_TX_JITTER);
    }
  }
  else
  {
    if ((pReqMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t))) != NULL)
    {
      /* Re-send request */
      pReqMsg->event = MESH_LPN_MSG_RESEND_FRIEND_SUBSCR_ADD_RM;
      pReqMsg->param = pMsg->hdr.param;

      /* Send Message. */
      WsfMsgSend(meshCb.handlerId, pReqMsg);
    }
  }
}
