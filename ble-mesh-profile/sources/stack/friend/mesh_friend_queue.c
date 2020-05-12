/*************************************************************************************************/
/*!
 *  \file   mesh_friend_queue.c
 *
 *  \brief  Mesh Friend Queue implementation.
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
#include "wsf_trace.h"
#include "wsf_assert.h"
#include "wsf_msg.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_error_codes.h"
#include "mesh_api.h"
#include "mesh_main.h"

#include "mesh_local_config_types.h"
#include "mesh_local_config.h"
#include "mesh_seq_manager.h"

#include "mesh_network.h"
#include "mesh_lower_transport.h"
#include "mesh_sar_rx.h"
#include "mesh_upper_transport.h"

#include "mesh_friendship_defs.h"
#include "mesh_friend.h"
#include "mesh_friend_main.h"

#include "mesh_utils.h"
#include "util/bstream.h"
#include <string.h>

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Offset for MD field in the Friend Queue PDU for a Friend Update PDU. */
#define FRIEND_QUEUE_PDU_UPDATE_MD_OFFSET (MESH_FRIEND_UPDATE_MD_OFFSET + 1)

/*! Checks if PDU is a SAR ACK. */
#define FRIEND_QUEUE_PDU_IS_ACK(pEntry)  (((pEntry)->ctl) &&\
                                          ((pEntry)->ltrPdu[0] == MESH_SEG_ACK_OPCODE))

/*! Extracts SeqZero from a Friend Queue entry that is ACK. */
#define FRIEND_QUEUE_PDU_ACK_GET_SEQZERO(pPdu) (((uint16_t)(MESH_UTILS_BF_GET((pPdu)[0],\
                                                  MESH_SEQ_ZERO_H_SHIFT,\
                                                  MESH_SEQ_ZERO_H_SIZE)) <<\
                                                  MESH_SEQ_ZERO_L_SIZE) |\
                                                 (uint8_t)(MESH_UTILS_BF_GET((pPdu)[1],\
                                                                             MESH_SEQ_ZERO_L_SHIFT,\
                                                                             MESH_SEQ_ZERO_L_SIZE)))\

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Prepares the Friend Queue to accept a new ACK Control PDU .
 *
 *  \param[in] pCtx     Pointer to LPN context.
 *  \param[in] ivIndex  IV Index of the received PDU.
 *  \param[in] seqNo    Sequence Number of the received PDU.
 *  \param[in] src      Source address.
 *  \param[in] dst      Destination address.
 *  \param[in] seqZero  SeqZero field of the new ACK.
 *
 *  \return    None
 */
/*************************************************************************************************/
static void meshFriendQueuePrepNewAckAdd(meshFriendLpnCtx_t *pCtx, uint32_t ivIndex,
                                         meshSeqNumber_t seqNo, meshAddress_t src, meshAddress_t dst,
                                         uint16_t seqZero)
{
  meshFriendQueueEntry_t *pEntry, *pPrev, *pNext;
  uint16_t localSeqZero;

  /* Set previous to NULL. */
  pPrev = NULL;
  /* Point to start of the queue. */
  pEntry = (meshFriendQueueEntry_t *)(&(pCtx->pduQueue))->pHead;

  /* Find all entries which have ACK PDU flag and same addressing information. */
  while(pEntry != NULL)
  {
    pNext = (meshFriendQueueEntry_t *)(pEntry->pNext);
    /* Check for ACK PDU's with same source and destination. */
    if ((pEntry->flags & FRIEND_QUEUE_FLAG_ACK_PDU) && (pEntry->src == src) &&
        (pEntry->dst == dst))
    {
      /* Extract SeqZero. */
      localSeqZero = FRIEND_QUEUE_PDU_ACK_GET_SEQZERO(&(pEntry->ltrPdu[1]));

      /* Compare SeqZero fields. */
      if(localSeqZero == seqZero)
      {
        /* Check if newer IV index or Sequence Number. */
        if((pEntry->ivIndex < ivIndex) ||
           ((pEntry->ivIndex == ivIndex) && (pEntry->seqNo < seqNo)))
        {
          /* Invalidate entry by setting address to 0 */
          pEntry->flags = 0;

          /* Remove from queue */
          WsfQueueRemove(&(pCtx->pduQueue), pEntry, pPrev);

          /* Increment free count. */
          pCtx->pduQueueFreeCount++;
        }

        break;
      }
    }

    pPrev = pEntry;
    pEntry = pNext;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Discards oldest entry in the queue except Friend Updates.
 *
 *  \param[in] pCtx  Pointer to LPN context.
 *
 *  \return    TRUE if entry discarded, or FALSE if all messages are Friend Updates.
 */
/*************************************************************************************************/
static bool_t meshFriendQueueDiscardOldest(meshFriendLpnCtx_t *pCtx)
{
  meshFriendQueueEntry_t *pEntry, *pPrev, *pNext;
  uint8_t qCnt;

  /* Set previous to NULL. */
  pPrev = NULL;
  /* Point to start of the queue. */
  pEntry = (meshFriendQueueEntry_t *)(&(pCtx->pduQueue))->pHead;
  /* Get queue count. */
  qCnt = (uint8_t)WsfQueueCount((&(pCtx->pduQueue)));

  /* Find all entries which have flags different than Friend Update. */
  while(qCnt > 0)
  {
    pNext = (meshFriendQueueEntry_t *)(pEntry->pNext);
    if (!(pEntry->flags & FRIEND_QUEUE_FLAG_UPDT_PDU))
    {
      /* Invalidate entry by setting address to 0 */
      pEntry->flags = 0;

      /* Remove from queue */
      WsfQueueRemove(&(pCtx->pduQueue), pEntry, pPrev);

      /* Increment free count. */
      pCtx->pduQueueFreeCount++;

      return TRUE;
    }

    pPrev = pEntry;
    pEntry = pNext;
    qCnt--;
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Allocates a Friend queue entry.
 *
 *  \param[in] pCtx  Pointer to LPN context.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static meshFriendQueueEntry_t *meshFriendQueueAlloc(meshFriendLpnCtx_t *pCtx)
{
  uint8_t idx = 0;
  if(pCtx->pduQueueFreeCount > 0)
  {
    for(idx = 0; idx < GET_MAX_NUM_QUEUE_ENTRIES(); idx++)
    {
      if(pCtx->pQueuePool[idx].flags == FRIEND_QUEUE_FLAG_EMPTY)
      {
        pCtx->pduQueueFreeCount--;
        memset(pCtx->pQueuePool[idx].ltrPdu, 0, sizeof(pCtx->pQueuePool[idx].ltrPdu));
        return &(pCtx->pQueuePool[idx]);
      }
    }
  }

  return NULL;
}

/*************************************************************************************************/
/*!
 *  \brief     If the Friend Queue already contains a message with the same SEQ and SRC fields as
 *             in the received message, or if the SRC field of the received message is a unicast
 *             address of an element of the Low Power node, then the message shall not be stored
 *             in the Friend Queue.
 *
 *  \param[in] pCtx     Pointer to LPN context.
 *  \param[in] seqNo    Sequence Number of the received PDU.
 *  \param[in] src      Source address.
 *
 *  \return    TRUE if duplicate PDU was found. Else FALSE.
 */
/*************************************************************************************************/
static bool_t meshFriendQueueCheckForDuplicatePdu(meshFriendLpnCtx_t *pCtx, meshSeqNumber_t seqNo,
                                                  meshAddress_t src)
{
  meshFriendQueueEntry_t *pEntry;
  bool_t status = FALSE;

  /* Check if the src address is a unicast address of an element of the Low Power node */
  if (MESH_IS_ADDR_UNICAST(src) && (src >= pCtx->lpnAddr) &&
      (src < (pCtx->lpnAddr + pCtx->estabInfo.numElements)))
  {
    status = TRUE;
  }
  else
  {
    /* Point to start of the queue. */
    pEntry = (meshFriendQueueEntry_t *)(&(pCtx->pduQueue))->pHead;

    /* Iterate through queue. */
    while (pEntry != NULL)
    {
      /* Check for duplicate PDU. */
      if ((pEntry->seqNo == seqNo) && (pEntry->src == src))
      {
        status = TRUE;
        break;
      }
      pEntry = pEntry->pNext;
    }
  }
  return status;
}

/*************************************************************************************************/
/*!
 *  \brief     Adds a Friend Update message to the queue.
 *
 *  \param[in] pCtx  Pointer to LPN context.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshFriendQueueAddUpdate(meshFriendLpnCtx_t *pCtx)
{
  meshFriendQueueEntry_t *pEntry;
  uint8_t *ptr;
  uint32_t iv;
  meshAddress_t elem0Addr;
  bool_t ivUpdate;

  MeshLocalCfgGetAddrFromElementId(0, &elem0Addr);

  /* Try to allocate or try removing oldest and allocating again. */
  if (((pEntry = meshFriendQueueAlloc(pCtx)) == NULL) &&
      (!meshFriendQueueDiscardOldest(pCtx) ||
       ((pEntry = meshFriendQueueAlloc(pCtx)) == NULL)))
  {
    return;
  }

  /* Allocate sequence number. */
  if (MeshSeqGetNumber(elem0Addr, &(pEntry->seqNo), TRUE) != MESH_SUCCESS)
  {
    /* Increment free count since entry is not used. */
    pCtx->pduQueueFreeCount++;
    return;
  }

  /* Configure PDU information. */
  pEntry->src = elem0Addr;
  pEntry->dst = pCtx->lpnAddr;
  pEntry->ctl = 1;
  pEntry->ttl = 0;
  pEntry->flags = FRIEND_QUEUE_FLAG_UPDT_PDU;

  ptr = pEntry->ltrPdu;

  /* Set opcode with SEG cleared. */
  UINT8_TO_BSTREAM(ptr, MESH_UTR_CTL_FRIEND_UPDATE_OPCODE);

  /* Set Key Refresh bit. */
  if (MeshLocalCfgGetKeyRefreshPhaseState(pCtx->netKeyIndex) == MESH_KEY_REFRESH_SECOND_PHASE)
  {
    MESH_UTILS_BIT_SET(*ptr, MESH_FRIEND_UPDATE_KEY_REFRESH_FLAG_SHIFT);
  }
  else
  {
    MESH_UTILS_BIT_CLR(*ptr, MESH_FRIEND_UPDATE_KEY_REFRESH_FLAG_SHIFT);
  }

  /* Get IV index. */
  iv = MeshLocalCfgGetIvIndex(&ivUpdate);

  /* Set IV Update bit. */
  if (ivUpdate)
  {
     MESH_UTILS_BIT_SET(*ptr, MESH_FRIEND_UPDATE_IV_UPDATE_FLAG_SHIFT);
  }
  else
  {
    MESH_UTILS_BIT_CLR(*ptr, MESH_FRIEND_UPDATE_IV_UPDATE_FLAG_SHIFT);
  }

  /* Increment to get to next field. */
  ptr++;

  /* Set IV. */
  UINT32_TO_BE_BSTREAM(ptr, iv);

  /* Set MD to 0. */
  *(ptr)++ = 0;

  /* Compute length. */
  pEntry->ltrPduLen = (uint8_t)(ptr - &(pEntry->ltrPdu[0]));

  /* Enqueue. */
  WsfQueueEnq(&(pCtx->pduQueue), pEntry);
}

/*************************************************************************************************/
/*!
 *  \brief     Adds a PDU to the queue that can be a segment ACK or non Friend Update PDU.
 *
 *  \param[in] pCtx        Pointer to LPN context.
 *  \param[in] ctl         1 if message is Control, 0 - Access.
 *  \param[in] ttl         TTL.
 *  \param[in] seqNo       Sequence number.
 *  \param[in] src         Source address.
 *  \param[in] dst         Destination address.
 *  \param[in] ivIndex     IV index.
 *  \param[in] pLtrHdr     Pointer to LTR header.
 *  \param[in] pLtrUtrPdu  Pointer to the UTR PDU that fits in an LTR PDU.
 *  \param[in] pduLen      Length of the UTR PDU that fits in the LTR PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshFriendQueueAddPdu(meshFriendLpnCtx_t *pCtx, uint8_t ctl, uint8_t ttl, uint32_t seqNo,
                           meshAddress_t src, meshAddress_t dst, uint32_t ivIndex,
                           uint8_t *pLtrHdr, uint8_t *pLtrUtrPdu, uint8_t pduLen)
{
  meshFriendQueueEntry_t *pEntry;
  uint8_t *pPdu;
  uint16_t seqZero = 0;

  /* Check if ACK Control PDU. */

  if (!MESH_UTILS_BITMASK_CHK(pLtrHdr[0], MESH_SEG_MASK) &&
      MESH_UTILS_BF_GET(pLtrHdr[0], MESH_CTL_OPCODE_SHIFT,
                        MESH_CTL_OPCODE_SIZE) == MESH_SEG_ACK_OPCODE)
  {
    /* Extract seqZero from the new ACK PDU. */
    seqZero = FRIEND_QUEUE_PDU_ACK_GET_SEQZERO(pLtrUtrPdu);
    /* Manage ACK PDU's before adding new one in queue. */
    meshFriendQueuePrepNewAckAdd(pCtx, ivIndex, seqNo, src, dst, seqZero);
  }


  /* Try to allocate or try removing oldest and allocating again. */
  if (((pEntry = meshFriendQueueAlloc(pCtx)) == NULL) &&
      (!meshFriendQueueDiscardOldest(pCtx) ||
       ((pEntry = meshFriendQueueAlloc(pCtx)) == NULL)))
  {
    return;
  }

  /* Erata 11302: Check if PDU should be added to Friend queue */
  if (meshFriendQueueCheckForDuplicatePdu(pCtx, seqNo, src) == FALSE)
  {
    /* Configure entry. */
    pEntry->src = src;
    pEntry->dst = dst;
    pEntry->seqNo = seqNo;
    pEntry->ivIndex = ivIndex;
    pEntry->ctl = ctl;
    pEntry->ttl = ttl;

    /* Point to the start of the PDU. */
    pPdu = pEntry->ltrPdu;

    /* Copy LTR header. */
    if (MESH_UTILS_BF_GET(pLtrHdr[0], MESH_SEG_SHIFT, MESH_SEG_SIZE))
    {
      memcpy(pPdu, pLtrHdr, MESH_SEG_HEADER_LENGTH);
      pPdu += MESH_SEG_HEADER_LENGTH;
    }
    else
    {
      *(pPdu)++ = pLtrHdr[0];
    }

    /* Copy remaining bytes. */
    memcpy(pPdu, pLtrUtrPdu, pduLen);

    /* Compute length. */
    pEntry->ltrPduLen = (uint8_t)((pPdu - &(pEntry->ltrPdu[0])) + pduLen);

    /* Set flags. */
    if (FRIEND_QUEUE_PDU_IS_ACK(pEntry))
    {
      pEntry->flags = FRIEND_QUEUE_FLAG_ACK_PDU;
    }
    else
    {
      pEntry->flags = FRIEND_QUEUE_FLAG_DATA_PDU;
    }

    /* Enqueue. */
    WsfQueueEnq(&(pCtx->pduQueue), pEntry);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends oldest PDU from the queue. If the queue is empty, it sends a Friend Update
 *             with MD set to 0.
 *
 *  \param[in] pCtx  Pointer to LPN context.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshFriendQueueSendNextPdu(meshFriendLpnCtx_t *pCtx)
{
  meshFriendQueueEntry_t *pEntry;
  meshNwkPduTxInfo_t nwkPduTxInfo;
  meshNwkRetVal_t retVal = MESH_SUCCESS;

  /* If queue is empty, add an update message. */
  if(pCtx->pduQueueFreeCount == GET_MAX_NUM_QUEUE_ENTRIES())
  {
    meshFriendQueueAddUpdate(pCtx);
  }

  /* Point to start of the queue. */
  pEntry = (meshFriendQueueEntry_t *)(&(pCtx->pduQueue))->pHead;

  WSF_ASSERT((pEntry != NULL) && (pEntry->flags != FRIEND_QUEUE_FLAG_EMPTY));

  /* Configure TX. */
  /* Note: pEntry cannot be NULL */
  /* coverity[var_deref_op] */
  nwkPduTxInfo.src = pEntry->src;
  nwkPduTxInfo.dst = pEntry->dst;
  nwkPduTxInfo.ctl = pEntry->ctl;
  nwkPduTxInfo.ttl = pEntry->ttl;
  nwkPduTxInfo.seqNo = pEntry->seqNo;
  nwkPduTxInfo.netKeyIndex = pCtx->netKeyIndex;
  /* Send with priority. */
  nwkPduTxInfo.prioritySend = TRUE;
  /* Send with friendship credentials. */
  nwkPduTxInfo.friendLpnAddr = pCtx->lpnAddr;
  nwkPduTxInfo.ifPassthr = TRUE;

  /* Configure pointers to PDU. */
  nwkPduTxInfo.pLtrHdr = &(pEntry->ltrPdu[0]);
  /* LTR header can be 1 or 4 bytes depending on SAR. */
  nwkPduTxInfo.ltrHdrLen = MESH_UTILS_BF_GET(pEntry->ltrPdu[0], MESH_SEG_SHIFT, MESH_SEG_SIZE) ?
                           MESH_SEG_HEADER_LENGTH : 1;
  nwkPduTxInfo.pUtrPdu = &(pEntry->ltrPdu[nwkPduTxInfo.ltrHdrLen]);
  nwkPduTxInfo.utrPduLen = pEntry->ltrPduLen - nwkPduTxInfo.ltrHdrLen;

  /* Check if PDU is Friend Update and there is more data to be sent. */
  if(pEntry->flags & FRIEND_QUEUE_FLAG_UPDT_PDU)
  {
    if(pEntry->pNext != NULL)
    {
      pEntry->ltrPdu[FRIEND_QUEUE_PDU_UPDATE_MD_OFFSET] = 1;
    }
    else
    {
      pEntry->ltrPdu[FRIEND_QUEUE_PDU_UPDATE_MD_OFFSET] = 0;
    }
  }

  /* Send PDU. */
  retVal = MeshNwkSendLtrPdu((const meshNwkPduTxInfo_t *)&nwkPduTxInfo);
  WSF_ASSERT(retVal == MESH_SUCCESS);

  /* Mark entry as pending ACK. */
  pEntry->flags |= FRIEND_QUEUE_FLAG_ACK_PEND;

  (void)retVal;
}

/*************************************************************************************************/
/*!
 *  \brief     Removes oldest PDU from the queue as a result of it getting acknowledged.
 *
 *  \param[in] pCtx  Pointer to LPN context.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshFriendQueueRmAckPendPdu(meshFriendLpnCtx_t *pCtx)
{
  meshFriendQueueEntry_t *pEntry;

  /* Point to start of the queue. */
  pEntry = (meshFriendQueueEntry_t *)(&(pCtx->pduQueue))->pHead;

  /* Check if the queue is not empty and the ACK pending flag is set as it might have been removed
   * by another call to discard oldest to make room for newer messages.
   */
  if((pEntry != NULL) && (pEntry->flags & FRIEND_QUEUE_FLAG_ACK_PEND))
  {
    /* Remove from queue. */
    WsfQueueRemove(&(pCtx->pduQueue), pEntry, NULL);

    /* Reset flags. */
    pEntry->flags = FRIEND_QUEUE_FLAG_EMPTY;

    /* Increment free count. */
    pCtx->pduQueueFreeCount++;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Computes total number of free or freeable entries in the Friend Queue.
 *
 *  \param[in] pCtx  Pointer to LPN context.
 *
 *  \return    Total number of entries that can be used..
 */
/*************************************************************************************************/
uint8_t meshFriendQueueGetMaxFreeEntries(meshFriendLpnCtx_t *pCtx)
{
  uint8_t updtCnt = 0;
  meshFriendQueueEntry_t *pEntry;

  /* Point to start of the queue. */
  pEntry = (meshFriendQueueEntry_t *)(&(pCtx->pduQueue))->pHead;

  while(pEntry != NULL)
  {
    /* Count Friend Update messages. */
    if(pEntry->flags & FRIEND_QUEUE_FLAG_UPDT_PDU)
    {
      updtCnt++;
    }
    pEntry = (meshFriendQueueEntry_t *)(pEntry->pNext);
  }

  /* Only Friend Updates cannot be removed from the queue. */
  return GET_MAX_NUM_QUEUE_ENTRIES() - updtCnt;
}
