/*************************************************************************************************/
/*!
 *  \file   mesh_friend_data.c
 *
 *  \brief  Mesh Friend Data path implementation.
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
#include "mesh_sar_utils.h"
#include "mesh_sar_rx.h"
#include "mesh_upper_transport.h"

#include "mesh_friendship_defs.h"
#include "mesh_friend.h"
#include "mesh_friend_main.h"

#include "mesh_utils.h"
#include "util/bstream.h"
#include <string.h>

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Gets next LPN that is destination for a PDU.
 *
 *  \param[in]  ttl            TTL of the PDU.
 *  \param[in]  dst            Destination address of the PDU.
 *  \param[in]  netKeyIndex    Global NetKey identifier for the sub-net used to deliver the PDU.
 *  \param[out] pInOutIndexer  Pointer to indexer variable.
 *
 *  \return     Pointer to context if there is an LPN, NULL otherwise.
 *  \remarks    This function acts as an iterator.
 */
/*************************************************************************************************/
static meshFriendLpnCtx_t *meshFriendNextLpnDstForPdu(meshAddress_t dst, uint16_t netKeyIndex,
                                                      uint8_t *pInOutIndexer)
{
  meshFriendLpnCtx_t *pCtx;
  uint8_t idx, subIdx;

  WSF_ASSERT(pInOutIndexer != NULL);

  idx = *pInOutIndexer;

  /* Iterate through all the LPN contexts. */
  for(; idx < GET_MAX_NUM_CTX(); idx++)
  {
    /* Check for an established friendship on the sub-net received as parameter. */
    if((LPN_CTX_PTR(idx)->inUse) && (LPN_CTX_PTR(idx)->friendSmState == FRIEND_ST_ESTAB) &&
        (LPN_CTX_PTR(idx)->netKeyIndex == netKeyIndex))
    {
      pCtx = LPN_CTX_PTR(idx);

      /* Verify unicast destination. */
      if(MESH_IS_ADDR_UNICAST(dst))
      {
        if((dst >= pCtx->lpnAddr) && (dst < (pCtx->lpnAddr + pCtx->estabInfo.numElements)))
        {
          /* Forward indexer for next search. */
          *pInOutIndexer = idx + 1;
          return pCtx;
        }
      }
      /* Verify group or virtual destination. */
      else if(MESH_IS_ADDR_GROUP(dst) || MESH_IS_ADDR_VIRTUAL(dst))
      {
        /* Iterate through subscription list. */
        for(subIdx = 0; subIdx < GET_MAX_SUBSCR_LIST_SIZE(); subIdx++)
        {
          /* Skip empty entries. */
          if(MESH_IS_ADDR_UNASSIGNED(pCtx->pSubscrAddrList[subIdx]))
          {
            continue;
          }
          /* Check if address matches. */
          if(pCtx->pSubscrAddrList[subIdx] == dst)
          {
            /* Forward indexer for next search. */
            *pInOutIndexer = idx + 1;
            return pCtx;
          }
        }
      }
      else
      {
        /* Reset indexer. */
        *pInOutIndexer = 0;

        return NULL;
      }
    }
  }

  /* Reset indexer. */
  *pInOutIndexer = 0;

  return NULL;
}

/*************************************************************************************************/
/*!
 *  \brief     Checks if at least one LPN is destination for an incoming PDU.
 *
 *  \param[in] dst          Destination address of the received PDU.
 *  \param[in] netKeyIndex  Global NetKey identifier.
 *
 *  \return    TRUE if at least one LPN needs the PDU, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshFriendLpnDstCheckCback(meshAddress_t dst, uint16_t netKeyIndex)
{
  uint8_t indexer = 0;

  return (meshFriendNextLpnDstForPdu(dst, netKeyIndex, &indexer) != NULL);
}

/*************************************************************************************************/
/*!
 *  \brief     Friend data path add Network PDU.
 *
 *  \param[in] pNwkPduRxInfo  Pointer to Network PDU and information.
 *
 *  \return    TRUE if the PDU is accepted in a Friend Queue, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshFriendAddNwkPdu(const meshNwkPduRxInfo_t *pNwkPduRxInfo)
{
  meshFriendLpnCtx_t *pCtx;
  uint8_t indexer = 0;
  uint8_t hdrLen;
  bool_t pduAccept = FALSE;

  /* Apply TTL filtering. */
  if(pNwkPduRxInfo->ttl <= MESH_TX_TTL_FILTER_VALUE)
  {
    return FALSE;
  }

  /* Calculate LTR header length. */
  hdrLen = MESH_UTILS_BF_GET(pNwkPduRxInfo->pLtrPdu[0], MESH_SEG_SHIFT, MESH_SEG_SIZE) ?
           MESH_SEG_HEADER_LENGTH : 1;

  /* Validate length. */
  if((pNwkPduRxInfo->pduLen <= hdrLen) || (pNwkPduRxInfo->pduLen > MESH_FRIEND_QUEUE_MAX_LTR_PDU))
  {
    return FALSE;
  }

  /* Find LPN */
  while((pCtx = meshFriendNextLpnDstForPdu(pNwkPduRxInfo->dst,
                                           pNwkPduRxInfo->netKeyIndex,
                                           &indexer)) != NULL)
  {
    pduAccept = TRUE;

    /* Add to Queue. */
    meshFriendQueueAddPdu(pCtx, pNwkPduRxInfo->ctl, pNwkPduRxInfo->ttl - 1, pNwkPduRxInfo->seqNo,
                          pNwkPduRxInfo->src, pNwkPduRxInfo->dst,
                          pNwkPduRxInfo->ivIndex, pNwkPduRxInfo->pLtrPdu,
                          &(pNwkPduRxInfo->pLtrPdu[hdrLen]), pNwkPduRxInfo->pduLen - hdrLen);

    /* Stop searching for unicast dst. */
    if(MESH_IS_ADDR_UNICAST(pNwkPduRxInfo->dst))
    {
      break;
    }
  }

  /* Return check that there is at least one LPN accepting it. */
  return pduAccept;
}

/*************************************************************************************************/
/*!
 *  \brief     Friend data path add segmented LTR Access PDU.
 *
 *  \param[in] pCtx      Pointer to LPN context to extract queue.
 *  \param[in] pPduInfo  Pointer to Access PDU.
 *  \param[in] ivIndex   IV index;
 *
 *  \remarks   To optimize, this function will make sure there is sufficient room in the queue.
 */
/*************************************************************************************************/
static void meshFriendAddAccSegPdu(meshFriendLpnCtx_t *pCtx, const meshLtrAccPduInfo_t *pPduInfo,
                                   uint32_t ivIndex)
{
  uint8_t *pPdu;
  uint32_t seqNo;
  meshSarSegHdr_t segHdr = { {0, 0, 0, 0} };
  uint8_t segCount;
  uint8_t lastSegLength;
  uint8_t idx = 0, pduLen;

  /* Compute number of segments and last segment size. */
  meshSarComputeSegmentCountAndLastLength(pPduInfo->pduLen, MESH_LTR_MAX_SEG_UTR_ACC_PDU_LEN,
                                          &segCount, &lastSegLength);
  /* Check if there is space. */
  if(meshFriendQueueGetMaxFreeEntries(pCtx) < segCount)
  {
    return;
  }

  /* Prepare segment header. */
  meshSarInitSegHdrForAcc(&segHdr, pPduInfo->akf, pPduInfo->aid, pPduInfo->szMic,
                          (pPduInfo->seqNo & MESH_SEQ_ZERO_MASK), segCount - 1);

  /* Reuse first sequence number. */
  seqNo = pPduInfo->seqNo;

  /* Build one segment at a time. */
  for(idx = 0; idx < segCount; idx++)
  {
    /* Set the pointer to the correct position within the UTR PDU */
    pPdu = pPduInfo->pUtrAccPdu + idx * MESH_LTR_MAX_SEG_UTR_ACC_PDU_LEN;

    /* Set the pduLen to either the maximum length or to the last segment length */
    pduLen = (idx == segCount - 1) ? lastSegLength : MESH_LTR_MAX_SEG_UTR_ACC_PDU_LEN;

    /* Set the SegO field in the segmentation header */
    meshSarSetSegHdrSegO(&segHdr, idx);

    /* Add segment to queue. */
    meshFriendQueueAddPdu(pCtx, FALSE, pPduInfo->ttl, seqNo, pPduInfo->src, pPduInfo->dst,
                          ivIndex, segHdr.bytes, pPdu, pduLen);
    /* Set the next sequence number */
    if(MeshSeqGetNumber(pPduInfo->src, &seqNo, TRUE) != MESH_SUCCESS)
    {
      /* Abort. Out of sequence numbers. */
      return;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Friend data path add LTR Access PDU.
 *
 *  \param[in] pLtrAccInfo  Pointer to Network PDU and information.
 *
 *  \return    TRUE if the PDU is accepted in a Friend Queue, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshFriendAddLtrAccPdu(const meshLtrAccPduInfo_t *pLtrAccPduInfo)
{
  meshFriendLpnCtx_t *pCtx;
  uint32_t ivIndex;
  uint8_t indexer = 0;
  uint8_t unsegHdr = 0;
  bool_t pduAccept = FALSE;
  bool_t segTx = FALSE;
  bool_t ivUpdtInProgress;

  /* Check if segmentation is required. */
  if ((pLtrAccPduInfo->pduLen > MESH_LTR_MAX_UNSEG_UTR_ACC_PDU_LEN) ||
      (pLtrAccPduInfo->ackRequired == TRUE))
  {
    segTx = TRUE;
  }
  else
  {
    unsegHdr = 0;
    /* Set AKF field. */
    MESH_UTILS_BF_SET(unsegHdr, pLtrAccPduInfo->akf, MESH_AKF_SHIFT, MESH_AKF_SIZE);
    /* Set AID field. */
    MESH_UTILS_BF_SET(unsegHdr, pLtrAccPduInfo->aid, MESH_AID_SHIFT, MESH_AID_SIZE);
  }

  /* Read IV index. */
  ivIndex = MeshLocalCfgGetIvIndex(&ivUpdtInProgress);
  if (ivUpdtInProgress)
  {
    /* For IV update in progress procedures, the IV index must be decremented by 1.
     * Make sure IV index is not 0.
     */
    WSF_ASSERT(ivIndex != 0);
    if (ivIndex != 0)
    {
      --ivIndex;
    }
  }

  /* Find LPN. */
  while((pCtx = meshFriendNextLpnDstForPdu(pLtrAccPduInfo->dst,
                                           pLtrAccPduInfo->netKeyIndex,
                                           &indexer)) != NULL)
  {
    pduAccept = TRUE;

    /* Check if segmentation is required. */
    if (segTx)
    {
      /* Segment and add. */
      meshFriendAddAccSegPdu(pCtx, pLtrAccPduInfo, ivIndex);
    }
    else
    {
      /* Add to Queue. */
      meshFriendQueueAddPdu(pCtx, FALSE, pLtrAccPduInfo->ttl, pLtrAccPduInfo->seqNo,
                            pLtrAccPduInfo->src, pLtrAccPduInfo->dst, ivIndex, &unsegHdr,
                            pLtrAccPduInfo->pUtrAccPdu, (uint8_t)pLtrAccPduInfo->pduLen);
    }

    /* Stop searching for unicast dst. */
    if(MESH_IS_ADDR_UNICAST(pLtrAccPduInfo->dst))
    {
      break;
    }
  }

  /* Return check that there is at least one LPN accepting it. */
  return pduAccept;
}

/*************************************************************************************************/
/*!
 *  \brief     Friend data path add segmented LTR Control PDU.
 *
 *  \param[in] pCtx      Pointer to LPN context to extract queue.
 *  \param[in] pPduInfo  Pointer to Control PDU.
 *  \param[in] ivIndex   IV index;
 *
 *  \remarks   To optimize, this function will make sure there is sufficient room in the queue.
 */
/*************************************************************************************************/
static void meshFriendAddSegCtlPdu(meshFriendLpnCtx_t *pCtx, const meshLtrCtlPduInfo_t *pPduInfo,
                                   uint32_t ivIndex)
{
  uint8_t *pPdu;
  uint32_t seqNo;
  meshSarSegHdr_t segHdr = { {0, 0, 0, 0} };
  uint8_t segCount;
  uint8_t lastSegLength;
  uint8_t idx = 0, pduLen;

  /* Compute number of segments and last segment size. */
  meshSarComputeSegmentCountAndLastLength(pPduInfo->pduLen, MESH_LTR_MAX_SEG_UTR_CTL_PDU_LEN,
                                          &segCount, &lastSegLength);

  /* Check if there is space. */
  if(meshFriendQueueGetMaxFreeEntries(pCtx) < segCount)
  {
    return;
  }

  /* Prepare segment header. */
  meshSarInitSegHdrForCtl(&segHdr, pPduInfo->opcode, (pPduInfo->seqNo & MESH_SEQ_ZERO_MASK),
                          segCount - 1);


  /* Reuse first sequence number. */
  seqNo = pPduInfo->seqNo;

  /* Build one segment at a time. */
  for(idx = 0; idx < segCount; idx++)
  {
    /* Set the pointer to the correct position within the UTR PDU */
    pPdu = pPduInfo->pUtrCtlPdu + idx * MESH_LTR_MAX_SEG_UTR_CTL_PDU_LEN;

    /* Set the pduLen to either the maximum length or to the last segment length */
    pduLen = (idx == segCount - 1) ? lastSegLength : MESH_LTR_MAX_SEG_UTR_CTL_PDU_LEN;

    /* Set the SegO field in the segmentation header */
    meshSarSetSegHdrSegO(&segHdr, idx);

    /* Add segment to queue. */
    meshFriendQueueAddPdu(pCtx, TRUE, pPduInfo->ttl, seqNo, pPduInfo->src, pPduInfo->dst,
                          ivIndex, segHdr.bytes, pPdu, pduLen);

    /* Set the next sequence number */
    if(MeshSeqGetNumber(pPduInfo->src, &seqNo, TRUE) != MESH_SUCCESS)
    {
      /* Abort. Out of sequence numbers. */
      return;
    }
  }
}
/*************************************************************************************************/
/*!
 *  \brief     Friend data path add LTR Control PDU.
 *
 *  \param[in] pLtrAccInfo  Pointer to Network PDU and information.
 *
 *  \return    TRUE if the PDU is accepted in a Friend Queue, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshFriendAddLtrCtlPdu(const meshLtrCtlPduInfo_t *pLtrCtlPduInfo)
{
  meshFriendLpnCtx_t *pCtx;
  uint32_t ivIndex;
  uint8_t indexer = 0;
  uint8_t unsegHdr = 0;
  bool_t pduAccept = FALSE;
  bool_t segTx = FALSE;
  bool_t ivUpdtInProgress;

  /* Check if segmentation is required. */
  if ((pLtrCtlPduInfo->pduLen > MESH_LTR_MAX_UNSEG_UTR_CTL_PDU_LEN) ||
      (pLtrCtlPduInfo->ackRequired == TRUE))
  {
    segTx = TRUE;
  }
  else
  {
    unsegHdr = 0;
    /* Set OPCODE field. */
    MESH_UTILS_BF_SET(unsegHdr, pLtrCtlPduInfo->opcode, MESH_CTL_OPCODE_SHIFT,
                      MESH_CTL_OPCODE_SIZE);
  }

  /* Read IV index. */
  ivIndex = MeshLocalCfgGetIvIndex(&ivUpdtInProgress);
  if (ivUpdtInProgress)
  {
    /* For IV update in progress procedures, the IV index must be decremented by 1.
     * Make sure IV index is not 0.
     */
    WSF_ASSERT(ivIndex != 0);
    if (ivIndex != 0)
    {
      --ivIndex;
    }
  }

  /* Find LPN */
  while((pCtx = meshFriendNextLpnDstForPdu(pLtrCtlPduInfo->dst,
                                           pLtrCtlPduInfo->netKeyIndex,
                                           &indexer)) != NULL)
  {
    pduAccept = TRUE;

    /* Check if segmentation is required. */
    if (segTx)
    {
      /* Segment and add. */
      meshFriendAddSegCtlPdu(pCtx, pLtrCtlPduInfo, ivIndex);
    }
    else
    {
      /* Add to Queue. */
      meshFriendQueueAddPdu(pCtx, TRUE, pLtrCtlPduInfo->ttl, pLtrCtlPduInfo->seqNo,
                            pLtrCtlPduInfo->src, pLtrCtlPduInfo->dst, ivIndex, &unsegHdr,
                            pLtrCtlPduInfo->pUtrCtlPdu, (uint8_t)pLtrCtlPduInfo->pduLen);
    }

    /* Stop searching for unicast dst. */
    if(MESH_IS_ADDR_UNICAST(pLtrCtlPduInfo->dst))
    {
      break;
    }
  }

  /* Return check that there is at least one LPN accepting it. */
  return pduAccept;
}

/*************************************************************************************************/
/*!
 *  \brief     Friend Queue PDU add callback.
 *
 *  \param[in] pPduInfo  Pointer to PDU and information.
 *  \param[in] pduType   Type of the generic pPduInfo parameter. See ::meshFriendQueuePduTypes
 *
 *  \return    TRUE if the PDU is accepted in a Friend Queue, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t meshFriendQueuePduAddCback(const void *pPduInfo, meshFriendQueuePduType_t pduType)
{
  switch(pduType)
  {
    case MESH_FRIEND_QUEUE_NWK_PDU:
      return meshFriendAddNwkPdu((const meshNwkPduRxInfo_t *)pPduInfo);
    case MESH_FRIEND_QUEUE_LTR_ACC_PDU:
      return meshFriendAddLtrAccPdu((const meshLtrAccPduInfo_t *)pPduInfo);
    case MESH_FRIEND_QUEUE_LTR_CTL_PDU:
      /* Accept only ACK and Heartbeat messages. */
      if((((const meshLtrCtlPduInfo_t *)pPduInfo)->opcode != MESH_UTR_CTL_HB_OPCODE) &&
         (((const meshLtrCtlPduInfo_t *)pPduInfo)->opcode != MESH_SEG_ACK_OPCODE))
      {
        return FALSE;
      }
      return meshFriendAddLtrCtlPdu((const meshLtrCtlPduInfo_t *)pPduInfo);
  }
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh SAR Rx reassemble complete callback for Friend Queue.
 *
 *  \param[in] pduType        Type and format of the reassembled PDU.
 *  \param[in] pReasPduInfo   Pointer to reassembled PDU information.
 *  \param[in] pSegInfoArray  Additional information required to add segments in the Friend Queue.
 *  \param[in] ivIndex        IV index for the received segments.
 *  \param[in] seqZero        SeqZero field of the segments.
 *  \param[in] segN           Last segment number.
 *
 *  \return    None.
 *
 *  \see meshSarRxReassembledPduInfo_t
 */
/*************************************************************************************************/
void meshFriendQueueSarRxPduAddCback(meshSarRxPduType_t pduType,
                                     const meshSarRxReassembledPduInfo_t *pReasPduInfo,
                                     const meshSarRxSegInfoFriend_t *pSegInfoArray,
                                     uint32_t ivIndex, uint16_t seqZero, uint8_t segN)
{
  meshFriendLpnCtx_t *pCtx;
  uint8_t *pPdu;
  meshSarSegHdr_t segHdr = { {0, 0, 0, 0} };
  meshAddress_t src, dst;
  uint16_t netKeyIndex, pduLen;
  uint8_t segLen, lastSegLen;
  uint8_t idx;
  bool_t ctl;
  uint8_t ttl;
  uint8_t indexer = 0;

  /* Build header. Extract parameters needed by the Friend Queue. */
  if(pduType == MESH_SAR_RX_TYPE_ACCESS)
  {
    ctl = FALSE;
    ttl = pReasPduInfo->accPduInfo.ttl - 1;
    src = pReasPduInfo->accPduInfo.src;
    dst = pReasPduInfo->accPduInfo.dst;
    netKeyIndex = pReasPduInfo->accPduInfo.netKeyIndex;
    pPdu = pReasPduInfo->accPduInfo.pUtrAccPdu;
    pduLen = pReasPduInfo->accPduInfo.pduLen;

    /* Initialize segmentation header. */
    meshSarInitSegHdrForAcc(&segHdr, pReasPduInfo->accPduInfo.akf, pReasPduInfo->accPduInfo.aid,
                            pReasPduInfo->accPduInfo.szMic, seqZero, segN);

    /* Calculate segment length and last segment length. */
    segLen = MESH_LTR_MAX_SEG_UTR_ACC_PDU_LEN;
    lastSegLen = (uint8_t)(pduLen - ((pduLen / MESH_LTR_MAX_SEG_UTR_ACC_PDU_LEN) *
                                      MESH_LTR_MAX_SEG_UTR_ACC_PDU_LEN));
    if(lastSegLen == 0)
    {
      lastSegLen = segLen;
    }
  }
  else
  {
    ctl = TRUE;
    ttl = pReasPduInfo->ctlPduInfo.ttl - 1;
    src = pReasPduInfo->ctlPduInfo.src;
    dst = pReasPduInfo->ctlPduInfo.dst;
    netKeyIndex = pReasPduInfo->ctlPduInfo.netKeyIndex;
    pPdu = pReasPduInfo->ctlPduInfo.pUtrCtlPdu;
    pduLen = pReasPduInfo->ctlPduInfo.pduLen;

    /* Initialize segmentation header. */
    meshSarInitSegHdrForCtl(&segHdr, pReasPduInfo->ctlPduInfo.opcode, seqZero, segN);

    /* Calculate segment length and last segment length. */
    segLen = MESH_LTR_MAX_SEG_UTR_CTL_PDU_LEN;
    lastSegLen = (uint8_t)(pduLen - ((pduLen / MESH_LTR_MAX_SEG_UTR_CTL_PDU_LEN) *
                                      MESH_LTR_MAX_SEG_UTR_CTL_PDU_LEN));
    if(lastSegLen == 0)
    {
      lastSegLen = segLen;
    }
  }

  /* Find LPN */
  while((pCtx = meshFriendNextLpnDstForPdu(dst,netKeyIndex, &indexer)) != NULL)
  {
    /* Check if there is space in the Friend Queue. */
    if(meshFriendQueueGetMaxFreeEntries(pCtx) >= (segN + 1))
    {
      /* Reconstruct segments. */
      for(idx = 0; idx <= segN; idx++)
      {
        /* Set segO field. */
        meshSarSetSegHdrSegO(&segHdr, pSegInfoArray[idx].segO);

        /* Add to Queue. */
        meshFriendQueueAddPdu(pCtx, ctl, ttl, pSegInfoArray[idx].segSeqNo, src, dst,
                              ivIndex, segHdr.bytes, &(pPdu[pSegInfoArray[idx].offset]),
                              ((pSegInfoArray[idx].segO != segN) ? segLen : lastSegLen));
      }
    }

    /* Stop searching for unicast dst. */
    if(MESH_IS_ADDR_UNICAST(dst))
    {
      break;
    }
  }
}
