/*************************************************************************************************/
/*!
 *  \file   mesh_lpn_main.c
 *
 *  \brief  LPN feature implementation.
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
#include "wsf_msg.h"
#include "wsf_timer.h"
#include "wsf_math.h"
#include "wsf_buf.h"
#include "wsf_cs.h"
#include "util/bstream.h"
#include "wsf_trace.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_error_codes.h"
#include "mesh_api.h"
#include "mesh_main.h"
#include "mesh_utils.h"

#include "mesh_network.h"
#include "mesh_local_config_types.h"
#include "mesh_local_config.h"
#include "mesh_lower_transport.h"
#include "mesh_upper_transport.h"
#include "mesh_access.h"
#include "mesh_cfg_mdl_sr.h"

#include "mesh_friendship_defs.h"
#include "mesh_lpn_api.h"
#include "mesh_lpn_main.h"
#include "mesh_lpn.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Friend Update PDU more data flag offset */
#define MESH_LPN_LTR_FRIEND_UPDATE_MD_OFFSET          (MESH_FRIEND_UPDATE_MD_OFFSET + 1)

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! LPN Control block */
meshLpnCb_t lpnCb;

uint16_t meshLpnEvtCbackLen[] =
{
  sizeof(meshLpnFriendshipEstablishedEvt_t), /*!< MESH_LPN_FRIENDSHIP_ESTABLISHED_EVENT */
  sizeof(meshLpnFriendshipTerminatedEvt_t),  /*!< MESH_LPN_FRIENDSHIP_TERMINATED_EVENT */
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Computes memory requirements for LPN context table.
 *
 *  \return Required memory in bytes.
 */
/*************************************************************************************************/
static inline uint32_t meshLpnGetRequiredMemoryCtxTable(void)
{
  uint8_t numFriendships;

  numFriendships = WSF_MIN(pMeshConfig->pMemoryConfig->maxNumFriendships,
                           pMeshConfig->pMemoryConfig->netKeyListSize);

  return MESH_UTILS_ALIGN(sizeof(meshLpnCtx_t) * numFriendships);
}

/*************************************************************************************************/
/*!
 *  \brief  Computes memory requirements for LPN history.
 *
 *  \return Required memory in bytes.
 */
/*************************************************************************************************/
static inline uint32_t meshLpnGetRequiredMemoryHistory(void)
{
  return MESH_UTILS_ALIGN(sizeof(meshLpnFriendshipHistory_t) *
                          pMeshConfig->pMemoryConfig->netKeyListSize);
}

/*************************************************************************************************/
/*!
 *  \brief     LPN WSF message handler.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshLpnMsgCback(wsfMsgHdr_t *pMsg)
{
  meshLpnCtx_t *pLpnCtx;

  if ((pLpnCtx = meshLpnCtxByIdx((uint8_t)pMsg->param)) != NULL)
  {
    /* Execute state machine. */
    meshLpnSmExecute(pLpnCtx, (meshLpnSmMsg_t *)pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh LPN PDU received callback function.
 *
 *  \param[in] pNwkPduRxInfo  Network PDU RX info.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshLpnRxPduNotifyCback(meshNwkPduRxInfo_t *pNwkPduRxInfo)
{
  meshLpnFriendRxPdu_t *pMsg;
  uint8_t ctxIdx;
  uint8_t opcode;

  WSF_ASSERT(pNwkPduRxInfo != NULL);

  if ((ctxIdx = meshLpnCtxIdxByNetKeyIndex(pNwkPduRxInfo->netKeyIndex)) != MESH_LPN_INVALID_CTX_IDX)
  {
    if ((pMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t))) != NULL)
    {
      pMsg->hdr.event = MESH_LPN_MSG_FRIEND_MESSAGE;
      pMsg->hdr.param = ctxIdx;
      pMsg->md = TRUE;
      pMsg->toggleFsn = TRUE;

      if ((pNwkPduRxInfo->ctl == 1) &&
          (!MESH_UTILS_BITMASK_CHK(pNwkPduRxInfo->pLtrPdu[0], MESH_SEG_MASK)))
      {
        opcode = MESH_UTILS_BF_GET(pNwkPduRxInfo->pLtrPdu[0],MESH_CTL_OPCODE_SHIFT,
                                   MESH_CTL_OPCODE_SIZE);

        if (opcode == MESH_UTR_CTL_FRIEND_SUBSCR_LIST_CNF_OPCODE)
        {
          pMsg->toggleFsn = FALSE;
          pMsg->md = FALSE;
        }
        else if ((opcode == MESH_UTR_CTL_FRIEND_UPDATE_OPCODE) &&
                 (pNwkPduRxInfo->pLtrPdu[MESH_LPN_LTR_FRIEND_UPDATE_MD_OFFSET] == 0))
        {
          pMsg->md = FALSE;
        }
      }

      /* Send Message. */
      WsfMsgSend(meshCb.handlerId, pMsg);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh LPN PDU received filter callback function.
 *
 *  \param[in] netKeyIndex  NetKey index.
 *
 *  \return    TRUE if the PDU needs to be filtered, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshLpnRxPduFilterCback(uint16_t netKeyIndex)
{
  meshLpnCtx_t *pLpnCtx;

  if ((pLpnCtx = meshLpnCtxByNetKeyIndex(netKeyIndex)) != NULL)
  {
    if (pLpnCtx->established == TRUE)
    {
      return TRUE;
    }
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh LPN Control PDU received callback function.
 *
 *  \param[in] pCtlPduInfo  Pointer to the structure holding the received Control PDU and other
 *                          fields. See ::meshLtrCtlPduInfo_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshLpnCtlRecvCback(const meshLtrCtlPduInfo_t *pCtlPduInfo)
{
  wsfMsgHdr_t *pMsg;
  uint8_t     ctxIdx;

  WSF_ASSERT(pCtlPduInfo != NULL);

  if ((ctxIdx = meshLpnCtxIdxByNetKeyIndex(pCtlPduInfo->netKeyIndex)) != MESH_LPN_INVALID_CTX_IDX)
  {
    switch (pCtlPduInfo->opcode)
    {
      case MESH_UTR_CTL_FRIEND_UPDATE_OPCODE:
        if ((pCtlPduInfo->ttl == 0x00) &&
            (pCtlPduInfo->friendLpnAddr != MESH_ADDR_TYPE_UNASSIGNED) &&
            (pCtlPduInfo->friendLpnAddr == lpnCb.pLpnTbl[ctxIdx].friendAddr))
        {
          if ((pMsg = WsfMsgAlloc(sizeof(meshLpnFriendOffer_t))) != NULL)
          {
            pMsg->event = MESH_LPN_MSG_FRIEND_UPDATE;
            pMsg->param = ctxIdx;

            ((meshLpnFriendUpdate_t *)(pMsg))->flags =
                pCtlPduInfo->pUtrCtlPdu[MESH_FRIEND_UPDATE_FLAGS_OFFSET];
            BYTES_BE_TO_UINT32(((meshLpnFriendUpdate_t *)(pMsg))->ivIndex,
                               &pCtlPduInfo->pUtrCtlPdu[MESH_FRIEND_UPDATE_IVINDEX_OFFSET]);
            ((meshLpnFriendUpdate_t *)(pMsg))->md =
                pCtlPduInfo->pUtrCtlPdu[MESH_FRIEND_UPDATE_MD_OFFSET];

            /* Send Message. */
            WsfMsgSend(meshCb.handlerId, pMsg);
          }
        }
        break;
      case MESH_UTR_CTL_FRIEND_OFFER_OPCODE:
        if (MESH_FRIEND_RECV_WIN_VALID(pCtlPduInfo->pUtrCtlPdu[MESH_FRIEND_OFFER_RECV_WIN_OFFSET]) &&
            (pCtlPduInfo->ttl == 0x00) &&
            (pCtlPduInfo->friendLpnAddr == MESH_ADDR_TYPE_UNASSIGNED))
        {
          if ((pMsg = WsfMsgAlloc(sizeof(meshLpnFriendOffer_t))) != NULL)
          {
            pMsg->event = MESH_LPN_MSG_FRIEND_OFFER;
            pMsg->param = ctxIdx;

            ((meshLpnFriendOffer_t *)(pMsg))->friendAddr = pCtlPduInfo->src;
            ((meshLpnFriendOffer_t *)(pMsg))->recvWinMs =
                pCtlPduInfo->pUtrCtlPdu[MESH_FRIEND_OFFER_RECV_WIN_OFFSET];
            ((meshLpnFriendOffer_t *)(pMsg))->queueSize =
                pCtlPduInfo->pUtrCtlPdu[MESH_FRIEND_OFFER_QUEUE_SIZE_OFFSET];
            ((meshLpnFriendOffer_t *)(pMsg))->subscrListSize =
                pCtlPduInfo->pUtrCtlPdu[MESH_FRIEND_OFFER_SUBSCR_LIST_SIZE_OFFSET];
            ((meshLpnFriendOffer_t *)(pMsg))->rssi =
                pCtlPduInfo->pUtrCtlPdu[MESH_FRIEND_OFFER_RSSI_OFFSET];
            BYTES_BE_TO_UINT16(((meshLpnFriendOffer_t *)(pMsg))->friendCounter,
                               &pCtlPduInfo->pUtrCtlPdu[MESH_FRIEND_OFFER_FRIEND_COUNTER_OFFSET]);

            /* Send Message. */
            WsfMsgSend(meshCb.handlerId, pMsg);
          }
        }
        break;
      case MESH_UTR_CTL_FRIEND_SUBSCR_LIST_CNF_OPCODE:
        if ((pCtlPduInfo->ttl == 0x00) &&
            (pCtlPduInfo->friendLpnAddr != MESH_ADDR_TYPE_UNASSIGNED) &&
            (pCtlPduInfo->friendLpnAddr == lpnCb.pLpnTbl[ctxIdx].friendAddr))
        {
          if ((pMsg = WsfMsgAlloc(sizeof(meshLpnFriendSubscrCnf_t))) != NULL)
          {
            pMsg->event = MESH_LPN_MSG_FRIEND_SUBSCR_CNF;
            pMsg->param = ctxIdx;

            ((meshLpnFriendSubscrCnf_t *)(pMsg))->tranNumber =
                pCtlPduInfo->pUtrCtlPdu[MESH_FRIEND_SUBSCR_LIST_CNF_TRAN_NUM_OFFSET];

            /* Send Message. */
            WsfMsgSend(meshCb.handlerId, pMsg);
          }
        }
        break;
      default:
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Empty event notification callback.
 *
 *  \param[in] pEvent  Pointer to the event. See ::meshLpnEvt_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshLpnEvtNotifyEmptyCback(const meshLpnEvt_t *pEvent)
{
  (void)pEvent;

  MESH_TRACE_WARN0("MESH LPN: Event notification callback not installed!");
}

/*************************************************************************************************/
/*!
 *  \brief     Local Config Friend Subscription event notification callback.
 *
 *  \param[in] event         Local Config Friend Subscription event.
 *                           See ::meshLocalCfgFriendSubscrEvent_t
 *  \param[in] pEventParams  Local Config Friend Subscription event params.
 *                           See ::meshLocalCfgFriendSubscrEventParams_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshLpnFriendSubscrEventNotifyCback(meshLocalCfgFriendSubscrEvent_t event,
                                                const meshLocalCfgFriendSubscrEventParams_t *pEventParams)
{
  meshLpnFriendSubscrEvent_t *ptr;
  wsfMsgHdr_t *pMsg;
  uint8_t i;

  switch (event)
  {
    case MESH_LOCAL_CFG_FRIEND_SUBSCR_ADD:
    case MESH_LOCAL_CFG_FRIEND_SUBSCR_RM:
      for (i = 0; i < lpnCb.maxNumFriendships; i++)
      {
        if (lpnCb.pLpnTbl[i].inUse && lpnCb.pLpnTbl[i].established)
        {
          if ((ptr = (meshLpnFriendSubscrEvent_t *)WsfBufAlloc(sizeof(meshLpnFriendSubscrEvent_t))) != NULL)
          {

            ptr->add = (event == MESH_LOCAL_CFG_FRIEND_SUBSCR_ADD) ? TRUE : FALSE;
            ptr->address = pEventParams->address;
            ptr->entryIdx = (uint8_t)pEventParams->idx;

            WsfQueueEnq(&(lpnCb.pLpnTbl[i].subscrListQueue), ptr);

            /* Send a Friend Poll to trigger the Subscription Add afterwards */
            if ((pMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t))) != NULL)
            {
              pMsg->event = MESH_LPN_MSG_SEND_FRIEND_POLL;
              pMsg->param = i;

              /* Send Message. */
              WsfMsgSend(meshCb.handlerId, pMsg);
            }
          }
        }
      }
      break;
    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Callback definition for getting the Friend address for a subnet.
 *
 *  \param[in] netKeyIndex  Global identifier for the Network Key of the subnet.
 *
 *  \return    Friend address or MESH_ADDR_TYPE_UNASSIGNED if friendship is not established.
 */
/*************************************************************************************************/
static meshAddress_t meshLpnFriendAddrFromSubnetCback(uint16_t netKeyIndex)
{
  uint8_t ctxIdx;

  /* Validate NetKey Index. */
  if (MESH_KEY_REFRESH_PROHIBITED_START != MeshLocalCfgGetKeyRefreshPhaseState(netKeyIndex))
  {
    ctxIdx = meshLpnCtxIdxByNetKeyIndex(netKeyIndex);

    if (ctxIdx != MESH_LPN_INVALID_CTX_IDX)
    {
      if (lpnCb.pLpnTbl[ctxIdx].inUse && lpnCb.pLpnTbl[ctxIdx].established)
      {
        return lpnCb.pLpnTbl[ctxIdx].friendAddr;
      }
    }
  }

  return MESH_ADDR_TYPE_UNASSIGNED;
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
static void meshLpnNetKeyDelNotifyCback(uint16_t netKeyIndex)
{
  wsfMsgHdr_t *pMsg;
  uint8_t     ctxIdx;

  if ((ctxIdx = meshLpnCtxIdxByNetKeyIndex(netKeyIndex)) != MESH_LPN_INVALID_CTX_IDX)
  {
    if ((pMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t))) != NULL)
    {
      pMsg->event = MESH_LPN_MSG_TERMINATE;
      pMsg->param = ctxIdx;

      /* Send Message. */
      WsfMsgSend(meshCb.handlerId, pMsg);
    }
  }
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Allocates an LPN context.
 *
 *  \param[in] netKeyIndex  NetKey index.
 *
 *  \return    LPN context index or MESH_LPN_CTX_INVALID_IDX if failure.
 */
/*************************************************************************************************/
uint8_t meshLpnCtxAlloc(uint16_t netKeyIndex)
{
  uint8_t freeIdx = MESH_LPN_INVALID_CTX_IDX;
  uint8_t i;

  for (i = 0; i < lpnCb.maxNumFriendships; i++)
  {
    /* Search for the first free index. */
    if ((lpnCb.pLpnTbl[i].inUse == FALSE) &&
        (freeIdx == MESH_LPN_INVALID_CTX_IDX))
    {
      freeIdx = i;
    }
    /* Check if the NetKey Index is not already added. */
    else if (lpnCb.pLpnTbl[i].inUse == TRUE)
    {
      if (lpnCb.pLpnTbl[i].netKeyIndex == netKeyIndex)
      {
        return MESH_LPN_INVALID_CTX_IDX;
      }
    }
  }

  if (freeIdx != MESH_LPN_INVALID_CTX_IDX)
  {
    lpnCb.pLpnTbl[freeIdx].netKeyIndex = netKeyIndex;
    lpnCb.pLpnTbl[freeIdx].inUse = TRUE;
    lpnCb.pLpnTbl[freeIdx].established = FALSE;
    lpnCb.pLpnTbl[freeIdx].friendAddr = MESH_ADDR_TYPE_UNASSIGNED;
    lpnCb.pLpnTbl[freeIdx].fsn = 0;
    lpnCb.pLpnTbl[freeIdx].tranNumber = 0;
    lpnCb.pLpnTbl[freeIdx].lpnCounter = 0;
    lpnCb.pLpnTbl[freeIdx].recvWinMs = 0;
    lpnCb.pLpnTbl[freeIdx].state = 0;
    lpnCb.pLpnTbl[freeIdx].msgTimeout = 0;
    lpnCb.pLpnTbl[freeIdx].txRetryCount = MESH_LPN_TX_NUM_RETRIES;
    lpnCb.pLpnTbl[freeIdx].subscrReq.addrListCount = 0;
    lpnCb.pLpnTbl[freeIdx].subscrReq.nextAddressIdx = pMeshConfig->pMemoryConfig->addrListMaxSize;
    lpnCb.pLpnTbl[freeIdx].subscrReq.nextVirtualAddrIdx = pMeshConfig->pMemoryConfig->virtualAddrListMaxSize;

    MESH_TRACE_INFO1("MESH LPN: meshLpnCtxAlloc 0x%04x", lpnCb.pLpnTbl[i].netKeyIndex);

    return freeIdx;
  }

  return MESH_LPN_INVALID_CTX_IDX;
}

/*************************************************************************************************/
/*!
 *  \brief     Deallocates an LPN context.
 *
 *  \param[in] pLpnCtx  LPN context.
 *
 *  \return    None
 */
/*************************************************************************************************/
void meshLpnCtxDealloc(meshLpnCtx_t *pLpnCtx)
{
  MESH_TRACE_INFO1("MESH LPN: meshLpnFriendshipCtxDealloc 0x%04x", pLpnCtx->netKeyIndex);

  pLpnCtx->inUse = FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Finds an LPN context with matching NetKey index.
 *
 *  \param[in] netKeyIndex  NetKey index to find.
 *
 *  \return    Pointer to LPN context or NULL if failure.
 */
/*************************************************************************************************/
meshLpnCtx_t *meshLpnCtxByNetKeyIndex(uint16_t netKeyIndex)
{
  meshLpnCtx_t *pLpnCtx = lpnCb.pLpnTbl;
  uint8_t      i;

  for (i = 0; i < lpnCb.maxNumFriendships; i++, pLpnCtx++)
  {
    if (pLpnCtx->inUse && (pLpnCtx->netKeyIndex == netKeyIndex))
    {
      return pLpnCtx;
    }
  }

  MESH_TRACE_WARN1("MESH LPN: NetKey index not found 0x%04x", netKeyIndex);

  return NULL;
}

/*************************************************************************************************/
/*!
 *  \brief     Returns the LPN context for the given context index.
 *
 *  \param[in] ctxIdx  LPN context index.
 *
 *  \return    Pointer to LPN context or NULL if failure.
 */
/*************************************************************************************************/
meshLpnCtx_t * meshLpnCtxByIdx(uint8_t ctxIdx)
{
  WSF_ASSERT(ctxIdx < lpnCb.maxNumFriendships);

  if (lpnCb.pLpnTbl[ctxIdx].inUse)
  {
    return &lpnCb.pLpnTbl[ctxIdx];
  }

  return NULL;
}

/*************************************************************************************************/
/*!
 *  \brief     Finds an LPN context index with matching NetKey index.
 *
 *  \param[in] netKeyIndex  NetKey index to find.
 *
 *  \return    LPN context index or MESH_LPN_CTX_INVALID_IDX if failure.
 */
/*************************************************************************************************/
uint8_t meshLpnCtxIdxByNetKeyIndex(uint16_t netKeyIndex)
{
  uint8_t i;

  for (i = 0; i < lpnCb.maxNumFriendships; i++)
  {
    if (lpnCb.pLpnTbl[i].inUse && (lpnCb.pLpnTbl[i].netKeyIndex == netKeyIndex))
    {
      return i;
    }
  }

  MESH_TRACE_WARN1("MESH LPN: NetKey index not found 0x%04x", netKeyIndex);

  return MESH_LPN_INVALID_CTX_IDX;
}

/*************************************************************************************************/
/*!
 *  \brief     Adds an unicast address to LPN history.
 *
 *  \param[in] netKeyIndex  NetKey index.
 *  \param[in] addr         Unicast address.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshLpnHistoryAdd(uint16_t netKeyIndex, meshAddress_t addr)
{
  uint16_t freeIdx = (uint8_t)pMeshConfig->pMemoryConfig->netKeyListSize;
  uint16_t i;

  WSF_ASSERT(MESH_KEY_REFRESH_PROHIBITED_START != MeshLocalCfgGetKeyRefreshPhaseState(netKeyIndex));
  WSF_ASSERT(MESH_IS_ADDR_UNICAST(addr));

  for (i = 0; i < pMeshConfig->pMemoryConfig->netKeyListSize; i++)
  {
    if (lpnCb.pLpnHistory[i].netKeyIndex == netKeyIndex)
    {
      lpnCb.pLpnHistory[i].prevAddr = addr;
      return;
    }

    if ((lpnCb.pLpnHistory[i].netKeyIndex == MESH_LPN_INVALID_NET_KEY_INDEX) &&
        (freeIdx == pMeshConfig->pMemoryConfig->netKeyListSize))
    {
      freeIdx = i;
    }
  }

  WSF_ASSERT(freeIdx != pMeshConfig->pMemoryConfig->netKeyListSize);

  lpnCb.pLpnHistory[freeIdx].netKeyIndex = netKeyIndex;
  lpnCb.pLpnHistory[freeIdx].prevAddr = addr;
}

/*************************************************************************************************/
/*!
 *  \brief     Searches for an address to LPN history with matching NetKey index.
 *
 *  \param[in] netKeyIndex  NetKey index.
 *
 *  \return    Mesh unicast address or MESH_ADDR_TYPE_UNASSIGNED if not found.
 */
/*************************************************************************************************/
meshAddress_t meshLpnHistorySearch(uint16_t netKeyIndex)
{
  uint16_t i;

  for (i = 0; i < pMeshConfig->pMemoryConfig->netKeyListSize; i++)
  {
    if (lpnCb.pLpnHistory[i].netKeyIndex == netKeyIndex)
    {
      return lpnCb.pLpnHistory[i].prevAddr;
    }
  }

  return MESH_ADDR_TYPE_UNASSIGNED;
}

/*************************************************************************************************/
/*!
 *  \brief  Computes memory requirements for LPN feature.
 *
 *  \return Required memory in bytes.
 */
/*************************************************************************************************/
uint32_t MeshLpnGetRequiredMemory(void)
{
  return meshLpnGetRequiredMemoryCtxTable() + meshLpnGetRequiredMemoryHistory();
}

/*************************************************************************************************/
/*!
 *  \brief     Initialize Low Power Node memory requirements.
 *
 *  \param[in] pFreeMem     Free memory.
 *  \param[in] freeMemSize  Amount of free memory in bytes.
 *
 *  \return    Amount of free memory consumed.
 *
 *  This function initializes the Low Power Node memory requirements.
 *
 *  \note      This function must be called once after Mesh Stack initialization.
 */
/*************************************************************************************************/
uint32_t MeshLpnMemInit(uint8_t *pFreeMem, uint32_t freeMemSize)
{
  uint32_t reqMem = MeshLpnGetRequiredMemory();

  /* Insufficient memory or invalid configuration. */
  if ((reqMem > freeMemSize) || (pMeshConfig->pMemoryConfig->maxNumFriendships == 0))
  {
    WSF_ASSERT(FALSE);
    return 0;
  }

  /* Set pointer to context table. */
  lpnCb.pLpnTbl = (meshLpnCtx_t *)pFreeMem;

  /* Set pointer to history. */
  lpnCb.pLpnHistory = (meshLpnFriendshipHistory_t *)(pFreeMem +
                                                     meshLpnGetRequiredMemoryCtxTable());

  return reqMem;
}

/*************************************************************************************************/
/*!
 *  \brief     Initialize Low Power Node feature.
 *
 *  \return    None.
 *
 *  This function initializes the Low Power Node feature.
 *
 *  \note      This function and MeshFriendInit() are mutually exclusive.
 */
/*************************************************************************************************/
void MeshLpnInit(void)
{
  uint16_t i;

  /* Set maximum number of friendships. */
  lpnCb.maxNumFriendships = (uint8_t)WSF_MIN(pMeshConfig->pMemoryConfig->maxNumFriendships,
                                             pMeshConfig->pMemoryConfig->netKeyListSize);

  /* Initialize LPN counter. */
  lpnCb.lpnCounter = 0;

  /* Clear context table. */
  memset(lpnCb.pLpnTbl, 0, sizeof(meshLpnCtx_t) * lpnCb.maxNumFriendships);

  /* Set Low Power feature disabled. */
  MeshLocalCfgSetLowPowerState(MESH_LOW_POWER_FEATURE_DISABLED);

  /* Register LPN message callback */
  meshCb.friendshipMsgCback = meshLpnMsgCback;

  /* Initialize context table. */
  for (i = 0; i < lpnCb.maxNumFriendships; i++)
  {
    lpnCb.pLpnTbl[i].lpnTimer.handlerId = meshCb.handlerId;
    lpnCb.pLpnTbl[i].pollTimer.handlerId = meshCb.handlerId;

    WSF_QUEUE_INIT(&lpnCb.pLpnTbl[i].subscrListQueue);
  }

  /* Initialize history. */
  for (i = 0; i < pMeshConfig->pMemoryConfig->netKeyListSize; i++)
  {
    lpnCb.pLpnHistory[i].netKeyIndex = MESH_LPN_INVALID_NET_KEY_INDEX;
    lpnCb.pLpnHistory[i].prevAddr = MESH_ADDR_TYPE_UNASSIGNED;
  }

  /* Register LPN empty notification callback. */
  lpnCb.lpnEvtNotifyCback = meshLpnEvtNotifyEmptyCback;
  lpnCb.pSm = &meshLpnSmIf;

  /* Register LPN UTR callback. */
  MeshUtrRegisterFriendship(meshLpnCtlRecvCback);

  /* Register LPN NWK RX PDU notification callbacks. */
  MeshNwkRegisterLpn(meshLpnRxPduNotifyCback, meshLpnRxPduFilterCback);

  /* Register Local Config Subscription events notification callback. */
  MeshLocalCfgRegisterLpn(meshLpnFriendSubscrEventNotifyCback);

  /* Register LPN Access callback. */
  MeshAccRegisterLpn(meshLpnFriendAddrFromSubnetCback);

  /* Register Config server callback. */
  MeshCfgMdlSrRegisterFriendship(NULL, meshLpnNetKeyDelNotifyCback, NULL);
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the Mesh LPN events callback.
 *
 *  \param[in] eventCback  Callback function for LPN events.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLpnRegister(meshLpnEvtNotifyCback_t eventCback)
{
  if (eventCback != NULL)
  {
    lpnCb.lpnEvtNotifyCback = eventCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Tries to establish a Friendship based on specific criteria for a subnet.
 *
 *  \param[in] netKeyIndex          NetKey index.
 *  \param[in] pLpnCriteria         Pointer to LPN criteria structure.
 *  \param[in] sleepDurationMs      Sleep duration in milliseconds.
 *  \param[in] recvDelayMs          Receive Delay in milliseconds, see ::meshFriendshipRecvDelayValues.
 *  \param[in] establishRetryCount  Number of retries for Friendship establish procedures.
 *
 *  \return    TRUE if success, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshLpnEstablishFriendship(uint16_t netKeyIndex, meshFriendshipCriteria_t *pLpnCriteria,
                                  uint32_t sleepDurationMs, uint8_t recvDelayMs,
                                  uint8_t establishRetryCount)
{
  wsfMsgHdr_t *pMsg;
  uint8_t ctxIdx;
  uint32_t pollTimeoutMs;


  /* Roughly check that the sleep duration is in range. */
  if (!MESH_FRIEND_POLL_TIMEOUT_MS_VALID(sleepDurationMs))
  {
    return FALSE;
  }

  /* Compute the actual Poll Timeout in milliseconds taking into account the retransmissions. */
  pollTimeoutMs = sleepDurationMs + (MESH_LPN_TX_NUM_RETRIES + 1) *
                  (recvDelayMs + MESH_FRIEND_RECV_WIN_MS_MAX);

  /* Validate params. */
  if ((pLpnCriteria != NULL) &&
      MESH_FRIEND_RSSI_FACTOR_VALID(pLpnCriteria->rssiFactor) &&
      MESH_FRIEND_RECV_WIN_FACTOR_VALID(pLpnCriteria->recvWinFactor) &&
      MESH_FRIEND_MIN_QUEUE_SIZE_VALID(pLpnCriteria->minQueueSizeLog) &&
      MESH_FRIEND_RECV_DELAY_VALID(recvDelayMs) &&
      MESH_FRIEND_POLL_TIMEOUT_MS_VALID(pollTimeoutMs) &&
      (MESH_KEY_REFRESH_PROHIBITED_START != MeshLocalCfgGetKeyRefreshPhaseState(netKeyIndex)))
  {
    ctxIdx = meshLpnCtxAlloc(netKeyIndex);

    if (ctxIdx != MESH_LPN_INVALID_CTX_IDX)
    {
      lpnCb.pLpnTbl[ctxIdx].criteria.rssiFactor = pLpnCriteria->rssiFactor;
      lpnCb.pLpnTbl[ctxIdx].criteria.recvWinFactor = pLpnCriteria->recvWinFactor;
      lpnCb.pLpnTbl[ctxIdx].criteria.minQueueSizeLog = pLpnCriteria->minQueueSizeLog;
      lpnCb.pLpnTbl[ctxIdx].recvDelayMs = recvDelayMs;
      lpnCb.pLpnTbl[ctxIdx].establishRetryCount = establishRetryCount;
      lpnCb.pLpnTbl[ctxIdx].sleepDurationMs = sleepDurationMs;

      /* Allocate the message and additional size for message parameters. */
      if ((pMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t))) != NULL)
      {
        pMsg->event = MESH_LPN_MSG_ESTABLISH;
        pMsg->param = ctxIdx;

        /* Send Message. */
        WsfMsgSend(meshCb.handlerId, pMsg);

        return TRUE;
      }
    }
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Terminates an established Friendship for a subnet.
 *
 *  \param[in] netKeyIndex  NetKey index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshLpnTerminateFriendship(uint16_t netKeyIndex)
{
  wsfMsgHdr_t *pMsg;
  uint8_t ctxIdx;

  /* Validate NetKey Index. */
  if (MESH_KEY_REFRESH_PROHIBITED_START != MeshLocalCfgGetKeyRefreshPhaseState(netKeyIndex))
  {
    ctxIdx = meshLpnCtxIdxByNetKeyIndex(netKeyIndex);

    if (ctxIdx != MESH_LPN_INVALID_CTX_IDX)
    {
      /* Allocate the message and additional size for message parameters. */
      if ((pMsg = WsfMsgAlloc(sizeof(wsfMsgHdr_t))) != NULL)
      {
        pMsg->event = MESH_LPN_MSG_SEND_FRIEND_CLEAR;
        pMsg->param = ctxIdx;

        /* Send Message. */
        WsfMsgSend(meshCb.handlerId, pMsg);
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Returns the period in milliseconds until another scheduled action is performed by LPN.
 *
 *  \return Period value in milliseconds.
 */
/*************************************************************************************************/
uint32_t MeshLpnGetRemainingSleepPeriod(void)
{
  uint32_t period = 0xFFFFFFFF;
  uint8_t i;

  WSF_CS_INIT(cs);

  WSF_CS_ENTER(cs);

  for (i = 0; i < lpnCb.maxNumFriendships; i++)
  {
    if (lpnCb.pLpnTbl->inUse)
    {
      /* Check if Receive Delay timer is started. */
      if (lpnCb.pLpnTbl->lpnTimer.isStarted &&
          (lpnCb.pLpnTbl->lpnTimer.msg.event == MESH_LPN_MSG_RECV_DELAY_TIMEOUT))
      {
        period = WSF_MIN(period, lpnCb.pLpnTbl->lpnTimer.ticks);
      }
      /* If timer is started it means Receive Window is in progress. */
      else if (lpnCb.pLpnTbl->lpnTimer.isStarted)
      {
        WSF_CS_EXIT(cs);

        return 0;
      }

      /* Check if Poll timer is started. */
      if (lpnCb.pLpnTbl->pollTimer.isStarted)
      {
        period = WSF_MIN(period, lpnCb.pLpnTbl->pollTimer.ticks);
      }
    }
  }

  WSF_CS_EXIT(cs);

  if (period != 0xFFFFFFFF)
  {
    return (period * WSF_MS_PER_TICK);
  }

  return 0;
}

/*************************************************************************************************/
/*!
 *  \brief     Return size of a Mesh Low Power Node callback event.
 *
 *  \param[in] pMeshLpnEvt  Mesh Low Power Node callback event.
 *
 *  \return    Size of Mesh Low Power Node callback event.
 */
/*************************************************************************************************/
uint16_t MeshLpnSizeOfEvt(wsfMsgHdr_t *pMeshLpnEvt)
{
  /* If a valid event ID */
  if ((pMeshLpnEvt->event == MESH_LPN_EVENT) &&
      (pMeshLpnEvt->param < MESH_LPN_MAX_EVENT))
  {
    return meshLpnEvtCbackLen[pMeshLpnEvt->param];
  }

  return 0;
}

