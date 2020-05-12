/*************************************************************************************************/
/*!
 *  \file   mesh_sar_rx.c
 *
 *  \brief  SAR Rx implementation.
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
#include "wsf_buf.h"
#include "wsf_os.h"
#include "wsf_assert.h"
#include "wsf_cs.h"
#include "wsf_trace.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_utils.h"
#include "mesh_error_codes.h"
#include "mesh_api.h"
#include "mesh_main.h"
#include "mesh_seq_manager.h"
#include "mesh_network.h"
#include "mesh_lower_transport.h"
#include "mesh_local_config_types.h"
#include "mesh_local_config.h"
#include "mesh_sar_rx.h"
#include "mesh_sar_rx_history.h"

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
#include "mesh_test_api.h"
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Pointer to ACC info for the transaction at the specified index */
#define SAR_RX_TRAN_ACC_INFO(tranIndex) \
        ((meshLtrAccPduInfo_t *)sarRxCb.pTranInfoTable[tranIndex].pLtrPduInfo)

/*! Pointer to CTL info for the transaction at the specified index */
#define SAR_RX_TRAN_CTL_INFO(tranIndex) \
        ((meshLtrCtlPduInfo_t *)sarRxCb.pTranInfoTable[tranIndex].pLtrPduInfo)

/*! Gets last entry in the Friend Segment Info array. */
#define SAR_RX_TRAN_LAST_FRIEND_SEG_INFO(tranIndex) \
        sarRxCb.pTranInfoTable[tranIndex].pFriendSegInfo[sarRxCb.pTranInfoTable[tranIndex].friendSegInfoIdx]

/*! Creates the SAR Rx Block Mask with all fragments received */
#define SAR_RX_BLOCK_MASK(segN)     (((segN) == 31U) ? 0xFFFFFFFFU : (uint32_t)MESH_UTILS_BTMASK_MAKE((segN) + 1))

/*! Extract least significant 2 bits to store in history */
#define SAR_RX_IVI_LSB(ivi)             (ivi & 0x00000003)

/*! Mesh SAR Rx Timer Tick value */
#define MESH_SAR_RX_TMR_TICK_TO_MS        50

/*! Mesh SAR Rx Incomplete Timeout value */
#define MESH_SAR_RX_INCOMPLETE_TIMEOUT_MS 10000

/*! Mesh SAR Rx Ack Timeout value based on TTL */
#define MESH_SAR_RX_ACK_TIMEOUT_MS(ttl)   (150 + 50 * ttl)

#define UINT32_TO_BE_BUF(p, n)   {(p)[0] = (uint8_t)(n >> 24); (p)[1] = (uint8_t)((n) >> 16); \
                                      (p)[2] = (uint8_t)((n) >> 8); (p)[3] = (uint8_t)((n));}

/*! Mesh SAR Rx WSF message events */
enum meshSarRxWsfMsgEvents
{
  MESH_SAR_RX_MSG_ACK_TMR_EXPIRED = MESH_SAR_RX_MSG_START, /*!< ACK timer expired */
  MESH_SAR_RX_MSG_INCOMP_TMR_EXPIRED,                      /*!< Incomplete timer expired */
};

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Definition of the acknowledged blocks mask. Bit i represents block i
 *  Value 0b1 means block is acknowledged.
 *  Value 0b0 means block is unacknowledged.
 */
typedef uint32_t meshSarRxBlockAck_t;

/*! Possible states of a reassemble transaction */
enum meshSarRxTranStateValues
{
  MESH_SAR_RX_TRAN_NOT_STARTED = 0x00, /*!< The transaction slot is empty */
  MESH_SAR_RX_TRAN_IN_PROGRESS = 0x01, /*!< The transaction is in progress */
  MESH_SAR_RX_TRAN_COMPLETE    = 0x02  /*!< The transaction is complete */
};

/*! Mesh SAR Rx transaction state data type. See ::meshSarRxTranStateValues */
typedef uint8_t meshSarRxTranState_t;

/*! Mesh SAR Rx reassemble transaction information */
typedef struct meshSarRxTranInfo_tag
{
  void                      *pLtrPduInfo;        /*!< Reassembled PDU formatted by the pduType
                                                  *   parameter.
                                                  */
  meshSarRxSegInfoFriend_t  *pFriendSegInfo;     /*!< Reassembled PDU information to reconstruct
                                                  *   original segments for the Friend Queue.
                                                  */
  wsfTimer_t                ackTmr;              /*!< ACK timer */
  wsfTimer_t                incompTmr;           /*!< Incomplete timer */
  uint8_t                   friendSegInfoIdx;    /*!< Friend segment info index. */
  meshSarRxTranState_t      state;               /*!< State of this transaction. Only
                                                  *   ::MESH_SAR_RX_TRAN_NOT_STARTED
                                                  *   and ::MESH_SAR_RX_TRAN_IN_PROGRESS
                                                  *   are used.
                                                  */
  meshAddress_t             srcAddr;             /*!< Address of the element originating
                                                  *   the PDU.
                                                  */
  uint16_t                  seqZero;             /*!< Last 13 bits of the first sequence number
                                                  *   used to identify the Upper Transport PDU
                                                  */
  meshSarRxBlockAck_t       blockAckMask;        /*!< Block acknowledgement bitmask */
  uint8_t                   segN;                /*!< Last segment number */
  uint8_t                   recvIvIndex;         /*!< Last bit of the 32-bit IV index value
                                                  *   for the received PDU
                                                  */
  meshSarRxPduType_t        pduType;             /*!< Type of the PDU reassembled */
  meshAddress_t             lpnAddress;          /*!< Low power node address. Unassigned if the
                                                  *   PDU must not reach the friend queue
                                                  */
} meshSarRxTranInfo_t;

/*! Mesh SAR Rx control block type definition */
typedef struct meshSarRxCb_t
{
  meshSarRxPduReassembledCback_t        pduReassembledCback;       /*!< PDU Reassemble callback */
  meshSarRxFriendPduReassembledCback_t  friendPduReassembledCback; /*!< Friend PDU Reassemble
                                                                    *   callback
                                                                    */
  meshSarRxLpnDstCheckCback_t           lpnDstCheckCback;          /*!< LPN destination check
                                                                    *   callback
                                                                    */
  meshSarRxTranInfo_t                   *pTranInfoTable;           /*!< Table containing info
                                                                    *   about SAR Rx transactions
                                                                    */
  uint8_t                               tranInfoSize;              /*!< Size of the transaction
                                                                    *   info table
                                                                    */
} meshSarRxCb_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Mesh SAR Rx control block */
static meshSarRxCb_t sarRxCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Computes memory requirements based on configured size of SAR Rx Transaction Table.
 *
 *  \param[in] tranSize  SAR Rx Transaction Info Table size.
 *
 *  \return    Required memory in bytes for SAR Rx Transaction Table.
 */
/*************************************************************************************************/
static inline uint32_t meshSarRxTranInfoGetRequiredMemory(uint8_t tranSize)
{
  /* Compute required memory size for SAR Rx Transaction Table */
  return MESH_UTILS_ALIGN(sizeof(meshSarRxTranInfo_t) * tranSize);
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh SAR Rx PDU reassembled empty callback.
 *
 *  \param[in] pduType       Type and format of the reassembled PDU.
 *  \param[in] pReasPduInfo  Pointer to reassembled PDU information.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSarRxEmptyPduReassembled(meshSarRxPduType_t pduType,
                                         const meshSarRxReassembledPduInfo_t *pReasPduInfo)
{
  MESH_TRACE_WARN0("MESH SAR RX: PDU Reassembled callback not set!");
  (void)pduType;
  (void)pReasPduInfo;
  return;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh SAR RX LPN destination empty callback.
 *
 *  \param[in] dst          Destination address of the received PDU.
 *  \param[in] netKeyIndex  Global NetKey identifier.
 *
 *  \return    TRUE if at least one LPN needs the PDU, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshSarRxEmptyLpnDstCheckCback(meshAddress_t dst, uint16_t netKeyIndex)
{
  (void)dst;
  (void)netKeyIndex;
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh SAR Rx empty reassemble complete callback for Friend Queue.
 *
 *  \param[in] pduType        Type and format of the reassembled PDU.
 *  \param[in] pReasPduInfo   Pointer to reassembled PDU information.
 *  \param[in] pSegInfoArray  Additional information required to add segments in the Friend Queue.
 *  \param[in] ivIndex        IV index of the received segments.
 *  \param[in] seqZero        SeqZero field of the segments.
 *  \param[in] segN           Last segment number.
 *
 *  \return    None.
 *
 *  \see meshSarRxReassembledPduInfo_t
 */
/*************************************************************************************************/
static void meshSarRxEmptyFriendPduReassembledCback(meshSarRxPduType_t pduType,
                                                    const meshSarRxReassembledPduInfo_t *pReasPduInfo,
                                                    const meshSarRxSegInfoFriend_t *pSegInfoArray,
                                                    uint32_t ivIndex, uint16_t seqZero, uint8_t segN)
{
  MESH_TRACE_WARN0("MESH SAR RX: Friend PDU Reassembled callback not set!");
  (void)pduType;
  (void)pReasPduInfo;
  (void)pSegInfoArray;
  (void)ivIndex;
  (void)seqZero;
  (void)segN;
  return;
}


/*************************************************************************************************/
/*!
 *  \brief     Reset SAR Rx Transaction.
 *
 *  \param[in] tranIndex  Index of the SAR Rx transaction in the transaction table.
 *  \param[in] obo        TRUE if transaction was On-Behalf-Of an LPN.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSarRxResetTransaction(uint8_t tranIndex, bool_t obo)
{
  if (sarRxCb.pTranInfoTable[tranIndex].state != MESH_SAR_RX_TRAN_COMPLETE)
  {
    /* Free allocated message only if transaction is not completed. When the transaction is
     * completed, the Upper Transport Layer shall free the message.
     */
    WsfBufFree(sarRxCb.pTranInfoTable[tranIndex].pLtrPduInfo);

    /* Reset SegN to use it in the history */
    sarRxCb.pTranInfoTable[tranIndex].segN = 0;
  }

  /* Add transaction to SAR cache */
  if (sarRxCb.pTranInfoTable[tranIndex].pduType == MESH_SAR_RX_TYPE_CTL)
  {
    MeshSarRxHistoryAdd(sarRxCb.pTranInfoTable[tranIndex].srcAddr,
                        SAR_RX_TRAN_CTL_INFO(tranIndex)->seqNo,
                        sarRxCb.pTranInfoTable[tranIndex].recvIvIndex,
                        sarRxCb.pTranInfoTable[tranIndex].segN,
                        obo);
  }
  else
  {
    MeshSarRxHistoryAdd(sarRxCb.pTranInfoTable[tranIndex].srcAddr,
                        SAR_RX_TRAN_ACC_INFO(tranIndex)->seqNo,
                        sarRxCb.pTranInfoTable[tranIndex].recvIvIndex,
                        sarRxCb.pTranInfoTable[tranIndex].segN,
                        obo);
  }

  /* Check if Friendship segment info array is allocated. */
  if(sarRxCb.pTranInfoTable[tranIndex].pFriendSegInfo != NULL)
  {
    /* Free memory */
    WsfBufFree(sarRxCb.pTranInfoTable[tranIndex].pFriendSegInfo);
  }

  /* Stop timers. */
  WsfTimerStop(&(sarRxCb.pTranInfoTable[tranIndex].ackTmr));
  WsfTimerStop(&(sarRxCb.pTranInfoTable[tranIndex].incompTmr));

  /* Clear entry */
  memset(&sarRxCb.pTranInfoTable[tranIndex], 0, sizeof(meshSarRxTranInfo_t));

  /* Configure timers. */
  sarRxCb.pTranInfoTable[tranIndex].ackTmr.msg.event = MESH_SAR_RX_MSG_ACK_TMR_EXPIRED;
  sarRxCb.pTranInfoTable[tranIndex].incompTmr.msg.event = MESH_SAR_RX_MSG_INCOMP_TMR_EXPIRED;
  sarRxCb.pTranInfoTable[tranIndex].ackTmr.msg.param = tranIndex;
  sarRxCb.pTranInfoTable[tranIndex].incompTmr.msg.param = tranIndex;
  sarRxCb.pTranInfoTable[tranIndex].ackTmr.handlerId = meshCb.handlerId;
  sarRxCb.pTranInfoTable[tranIndex].incompTmr.handlerId = meshCb.handlerId;

  /* Reset state */
  sarRxCb.pTranInfoTable[tranIndex].state = MESH_SAR_RX_TRAN_NOT_STARTED;
}

/*************************************************************************************************/
/*!
 *  \brief     Send ACK for the specified transaction ID.
 *
 *  \param[in] tranIndex  Index of the SAR Rx transaction in the transaction table.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSarRxSendAck(uint8_t tranIndex)
{
  meshNwkPduTxInfo_t nwkPduTxInfo;
  uint8_t ltrHdr = 0;  /* Segment ACK Lower Transport Control PDU header */
  uint8_t ackUtrPdu[MESH_SEG_ACK_PDU_LENGTH];   /* Segment ACK PDU */
  uint8_t pduOffset = 0;
  bool_t obo;

  /* SEG is set to 0 for Segment ACK messages */
  ltrHdr = MESH_SEG_ACK_OPCODE;

  /* If friendship segment information is present it means
   * that we need to send ACK on behalf of an LPN.
   */
  obo = sarRxCb.pTranInfoTable[tranIndex].pFriendSegInfo != NULL;

  /* Prepare the message for the Network Layer */
  nwkPduTxInfo.dst = sarRxCb.pTranInfoTable[tranIndex].srcAddr;

  if (sarRxCb.pTranInfoTable[tranIndex].pduType == MESH_SAR_RX_TYPE_CTL)
  {
    /* If responding On Behalf Of source address is Friend address. */
    if(obo)
    {
      MeshLocalCfgGetAddrFromElementId(0, &(nwkPduTxInfo.src));
    }
    else
    {
      nwkPduTxInfo.src = SAR_RX_TRAN_CTL_INFO(tranIndex)->dst;
    }
    nwkPduTxInfo.netKeyIndex = SAR_RX_TRAN_CTL_INFO(tranIndex)->netKeyIndex;
  }
  else
  {
    /* If responding On Behalf Of source address is Friend address. */
    if(obo)
    {
      MeshLocalCfgGetAddrFromElementId(0, &(nwkPduTxInfo.src));
    }
    else
    {
      nwkPduTxInfo.src = SAR_RX_TRAN_ACC_INFO(tranIndex)->dst;
    }
    nwkPduTxInfo.netKeyIndex = SAR_RX_TRAN_ACC_INFO(tranIndex)->netKeyIndex;
  }

  nwkPduTxInfo.ctl = MESH_SAR_RX_TYPE_CTL;
  nwkPduTxInfo.ttl = MeshLocalCfgGetDefaultTtl();

  /* Set the next sequence number */
  if (MeshSeqGetNumber(nwkPduTxInfo.src, &nwkPduTxInfo.seqNo, TRUE) != MESH_SUCCESS)
  {
    /* Abort. Out of sequence numbers. */
    return;
  }

  nwkPduTxInfo.pLtrHdr = &ltrHdr;
  nwkPduTxInfo.ltrHdrLen = 1;
  nwkPduTxInfo.pUtrPdu = ackUtrPdu;
  nwkPduTxInfo.utrPduLen = MESH_SEG_ACK_PDU_LENGTH;
  nwkPduTxInfo.prioritySend = FALSE;
  nwkPduTxInfo.friendLpnAddr = MESH_ADDR_TYPE_UNASSIGNED;
  nwkPduTxInfo.ifPassthr = FALSE;

  /* Prepare ACK UTR PDU. Start with SeqZero. RFU set to 0.
   * OBO depends on existing friendship segment info
   */
  ackUtrPdu[pduOffset]   = (obo << MESH_OBO_SHIFT) & MESH_OBO_MASK;
  ackUtrPdu[pduOffset++] |= (sarRxCb.pTranInfoTable[tranIndex].seqZero >>
                             (MESH_SEQ_ZERO_L_SIZE)) & MESH_SEQ_ZERO_H_MASK;
  ackUtrPdu[pduOffset++] = (sarRxCb.pTranInfoTable[tranIndex].seqZero << MESH_SEQ_ZERO_L_SHIFT) &
                            MESH_SEQ_ZERO_L_MASK;

  /* Set Block ACK Mask */
  UINT32_TO_BE_BUF(&ackUtrPdu[pduOffset], sarRxCb.pTranInfoTable[tranIndex].blockAckMask);

  /* Send the message to the network layer. */
  MeshNwkSendLtrPdu(&nwkPduTxInfo);
}

/*************************************************************************************************/
/*!
 *  \brief     Send ACK in response to the source address.
 *
 *  \param[in] pNwkPduInfo   Pointer to the Network PDU Information structure.
 *  \param[in] blockAckMask  Block Ack Mask Value.
 *  \param[in] obo           TRUE to set OBO flag to 1.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSarRxSendFastAck(const meshNwkPduRxInfo_t *pNwkPduInfo, uint32_t blockAckMask,
                                 bool_t obo)
{
  meshNwkPduTxInfo_t nwkPduTxInfo;
  uint8_t ltrHdr = 0;  /* Segment ACK Lower Transport Control PDU header */
  uint8_t ackUtrPdu[MESH_SEG_ACK_PDU_LENGTH];   /* Segment ACK PDU */
  uint8_t pduOffset = 0;

  /* SEG is set to 0 for Segment ACK messages */
  ltrHdr = MESH_SEG_ACK_OPCODE;

  /* Prepare the message for the Network Layer */
  /* If responding On Behalf Of source address is Friend address. */
  if(obo)
  {
    MeshLocalCfgGetAddrFromElementId(0, &(nwkPduTxInfo.src));
  }
  else
  {
    nwkPduTxInfo.src = pNwkPduInfo->dst;
  }
  nwkPduTxInfo.dst = pNwkPduInfo->src;
  nwkPduTxInfo.netKeyIndex = pNwkPduInfo->netKeyIndex;
  nwkPduTxInfo.ctl = MESH_SAR_RX_TYPE_CTL;
  nwkPduTxInfo.ttl = MeshLocalCfgGetDefaultTtl();

  /* Set the next sequence number */
  if (MeshSeqGetNumber(nwkPduTxInfo.src, &nwkPduTxInfo.seqNo, TRUE) != MESH_SUCCESS)
  {
    /* Abort. Out of sequence numbers. */
    return;
  }

  nwkPduTxInfo.pLtrHdr = &ltrHdr;
  nwkPduTxInfo.ltrHdrLen = 1;
  nwkPduTxInfo.pUtrPdu = ackUtrPdu;
  nwkPduTxInfo.utrPduLen = MESH_SEG_ACK_PDU_LENGTH;
  nwkPduTxInfo.prioritySend = FALSE;
  nwkPduTxInfo.friendLpnAddr = MESH_ADDR_TYPE_UNASSIGNED;
  nwkPduTxInfo.ifPassthr = FALSE;

  /* Prepare ACK UTR PDU. Start with SeqZero. RFU set to 0. OBO depends on friendship status. */
  ackUtrPdu[pduOffset]   = (obo << MESH_OBO_SHIFT) & MESH_OBO_MASK;
  ackUtrPdu[pduOffset++] |= pNwkPduInfo->pLtrPdu[MESH_SEQ_ZERO_H_PDU_OFFSET] & MESH_SEQ_ZERO_H_MASK;
  ackUtrPdu[pduOffset++] = pNwkPduInfo->pLtrPdu[MESH_SEQ_ZERO_L_PDU_OFFSET] & MESH_SEQ_ZERO_L_MASK;

  /* BlockAck field is set to blockAckMask */
  UINT32_TO_BE_BUF(&ackUtrPdu[pduOffset], blockAckMask);

  /* Send the message to the network layer. */
  MeshNwkSendLtrPdu(&nwkPduTxInfo);
}

/*************************************************************************************************/
/*!
 *  \brief     Finds an existing SAR transaction, or an empty entry in the transaction table if
 *             there is no match.
 *
 *  \param[in] srcAddr  Source address.
 *  \param[in] dstAddr  Destination address.
 *  \param[in] segN     Last segment number.
 *
 *  \return    Index in the SAR RX transaction table, or size of the table on error.
 */
/*************************************************************************************************/
static uint8_t meshSarRxGetTransactionIndex(meshAddress_t srcAddr, meshAddress_t dstAddr,
                                            uint8_t segN)
{
  meshAddress_t addr;
  uint8_t tranIndex, emptyEntryIndex;

  /* This stores the index of an empty entry in the transactions table */
  emptyEntryIndex = sarRxCb.tranInfoSize;

  /* Search for either the current transaction, or a new entry */
  for (tranIndex = 0; tranIndex < sarRxCb.tranInfoSize; tranIndex++)
  {
    if (sarRxCb.pTranInfoTable[tranIndex].state == MESH_SAR_RX_TRAN_IN_PROGRESS)
    {
      addr = (sarRxCb.pTranInfoTable[tranIndex].pduType == MESH_SAR_RX_TYPE_ACCESS) ?
             SAR_RX_TRAN_ACC_INFO(tranIndex)->dst : SAR_RX_TRAN_CTL_INFO(tranIndex)->dst;

      if ((sarRxCb.pTranInfoTable[tranIndex].srcAddr == srcAddr) && (addr == dstAddr) &&
          (sarRxCb.pTranInfoTable[tranIndex].segN == segN))
      {
          return tranIndex;
      }
    }

    if ((emptyEntryIndex == sarRxCb.tranInfoSize) &&
        (sarRxCb.pTranInfoTable[tranIndex].state == MESH_SAR_RX_TRAN_NOT_STARTED))
    {
      /* Found an empty entry */
      emptyEntryIndex = tranIndex;
    }
  }

  /* Return empty entry index or the total size as an invalid value */
  return emptyEntryIndex;
}

/*************************************************************************************************/
/*!
 *  \brief     Updates an entry associated to a transaction in the SAR Rx Transaction Info table.
 *             there is no match.
 *
 *  \param[in] pNwkPduInfo  Pointer to the Network PDU Information structure.
 *  \param[in] tranIndex    Entry index in the SAR Rx Transaction Info table.
 *
 *  \return    Block Ack Mask value for the specific transaction.
 */
/*************************************************************************************************/
static meshSarRxBlockAck_t meshSarRxAddUpdateTransaction(const meshNwkPduRxInfo_t *pNwkPduInfo,
                                                         uint8_t tranIndex)
{
  uint16_t seqZero;
  uint8_t segO, segN;
  uint8_t *pData;
  uint32_t segZeroSeqNo;
  bool_t tmrStarted = FALSE;

  /* Extract SeqZero field. */
  seqZero = (((uint16_t)(MESH_UTILS_BF_GET(pNwkPduInfo->pLtrPdu[MESH_SEQ_ZERO_H_PDU_OFFSET],
                                           MESH_SEQ_ZERO_H_SHIFT,
                                           MESH_SEQ_ZERO_H_SIZE)) << MESH_SEQ_ZERO_L_SIZE) |
             (uint8_t)(MESH_UTILS_BF_GET(pNwkPduInfo->pLtrPdu[MESH_SEQ_ZERO_L_PDU_OFFSET],
                                         MESH_SEQ_ZERO_L_SHIFT,
                                         MESH_SEQ_ZERO_L_SIZE)));

  /* Extract Seg0 field. */
  segO = (((pNwkPduInfo->pLtrPdu[MESH_SEG_ZERO_H_PDU_OFFSET]) & MESH_SEG_ZERO_H_MASK) <<
            MESH_SEG_ZERO_H_SHIFT) |
         (((pNwkPduInfo->pLtrPdu[MESH_SEG_ZERO_L_PDU_OFFSET]) & MESH_SEG_ZERO_L_MASK) >>
            MESH_SEG_ZERO_L_SHIFT) ;

  /* Extract SegN field. */
  segN = pNwkPduInfo->pLtrPdu[MESH_SEG_N_PDU_OFFSET] & MESH_SEG_N_MASK;

  /* Check if the transaction needs creating or updating */
  if (sarRxCb.pTranInfoTable[tranIndex].state == MESH_SAR_RX_TRAN_NOT_STARTED)
  {
    /* Create SEQ to be sent to UTR */
    segZeroSeqNo = (pNwkPduInfo->seqNo & (~MESH_SEQ_ZERO_MASK)) + seqZero;

    /* If reconstructed SEQ is bigger than SEQ, then it must have rolled over (on 13 bits)
     * during the transaction.
     */
    if (segZeroSeqNo > pNwkPduInfo->seqNo)
    {
      segZeroSeqNo -= (MESH_SEQ_ZERO_MASK + 1);
    }

    if (pNwkPduInfo->ctl == MESH_SAR_RX_TYPE_CTL)
    {
      /* This is a CTL message */
      sarRxCb.pTranInfoTable[tranIndex].pduType = MESH_SAR_RX_TYPE_CTL;

      /* Allocate buffer for UTR PDU */
      sarRxCb.pTranInfoTable[tranIndex].pLtrPduInfo = WsfBufAlloc(sizeof(meshLtrCtlPduInfo_t) +
                                                  (MESH_CTL_SEG_MAX_LENGTH * (segN + 1)));

      /* If no memory is available, return FALSE. */
      if (sarRxCb.pTranInfoTable[tranIndex].pLtrPduInfo == NULL)
      {
        return 0;
      }

      /* Set pointer to PDU */
      SAR_RX_TRAN_CTL_INFO(tranIndex)->pUtrCtlPdu = ((uint8_t *)SAR_RX_TRAN_CTL_INFO(tranIndex)) +
                                                    sizeof(meshLtrCtlPduInfo_t);

      /* Set pointer to the data region where the PDU must be appended */
      pData = ((uint8_t *)(SAR_RX_TRAN_CTL_INFO(tranIndex)->pUtrCtlPdu)) +
               (segO * MESH_CTL_SEG_MAX_LENGTH);

      /* Copy UTR segment data and update length */
      memcpy(pData, &pNwkPduInfo->pLtrPdu[MESH_SEG_DATA_PDU_OFFSET],
             pNwkPduInfo->pduLen - MESH_SEG_HEADER_LENGTH);
      SAR_RX_TRAN_CTL_INFO(tranIndex)->pduLen = (pNwkPduInfo->pduLen - MESH_SEG_HEADER_LENGTH);

      /* Copy UTR ACC packet information */
      SAR_RX_TRAN_CTL_INFO(tranIndex)->src = pNwkPduInfo->src;
      SAR_RX_TRAN_CTL_INFO(tranIndex)->dst = pNwkPduInfo->dst;
      SAR_RX_TRAN_CTL_INFO(tranIndex)->netKeyIndex = pNwkPduInfo->netKeyIndex;
      SAR_RX_TRAN_CTL_INFO(tranIndex)->friendLpnAddr = pNwkPduInfo->friendLpnAddr;
      SAR_RX_TRAN_CTL_INFO(tranIndex)->ttl = pNwkPduInfo->ttl;
      SAR_RX_TRAN_CTL_INFO(tranIndex)->seqNo = segZeroSeqNo;
      SAR_RX_TRAN_CTL_INFO(tranIndex)->gtSeqNo = pNwkPduInfo->seqNo;
      SAR_RX_TRAN_CTL_INFO(tranIndex)->opcode = pNwkPduInfo->pLtrPdu[MESH_SEG_OPCODE_PDU_OFFSET] &
                                                MESH_CTL_OPCODE_MASK;
    }
    else
    {
      /* This is an ACC message */
      sarRxCb.pTranInfoTable[tranIndex].pduType = MESH_SAR_RX_TYPE_ACCESS;

      /* Allocate memory for UTR PDU */
      sarRxCb.pTranInfoTable[tranIndex].pLtrPduInfo = WsfBufAlloc(sizeof(meshLtrAccPduInfo_t) +
                                                  (MESH_ACC_SEG_MAX_LENGTH * (segN + 1)));

      /* If no memory is available, return FALSE. */
      if (sarRxCb.pTranInfoTable[tranIndex].pLtrPduInfo == NULL)
      {
        return 0;
      }

      /* Set pointer to PDU */
      SAR_RX_TRAN_ACC_INFO(tranIndex)->pUtrAccPdu = ((uint8_t *)SAR_RX_TRAN_ACC_INFO(tranIndex)) +
                                                        sizeof(meshLtrAccPduInfo_t);

      /* Set pointer to the data region where the PDU must be appended */
      pData = ((uint8_t *)(SAR_RX_TRAN_ACC_INFO(tranIndex)->pUtrAccPdu)) +
              (segO * MESH_ACC_SEG_MAX_LENGTH);

      /* Copy UTR segment data and update length */
      memcpy(pData, &pNwkPduInfo->pLtrPdu[MESH_SEG_DATA_PDU_OFFSET],
             pNwkPduInfo->pduLen - MESH_SEG_HEADER_LENGTH);
      SAR_RX_TRAN_ACC_INFO(tranIndex)->pduLen = (pNwkPduInfo->pduLen - MESH_SEG_HEADER_LENGTH);

      /* Copy UTR ACC packet information */
      SAR_RX_TRAN_ACC_INFO(tranIndex)->src = pNwkPduInfo->src;
      SAR_RX_TRAN_ACC_INFO(tranIndex)->dst = pNwkPduInfo->dst;
      SAR_RX_TRAN_ACC_INFO(tranIndex)->netKeyIndex = pNwkPduInfo->netKeyIndex;
      SAR_RX_TRAN_ACC_INFO(tranIndex)->friendLpnAddr = pNwkPduInfo->friendLpnAddr;
      SAR_RX_TRAN_ACC_INFO(tranIndex)->ttl = pNwkPduInfo->ttl;
      SAR_RX_TRAN_ACC_INFO(tranIndex)->seqNo = segZeroSeqNo;
      SAR_RX_TRAN_ACC_INFO(tranIndex)->gtSeqNo = pNwkPduInfo->seqNo;
      SAR_RX_TRAN_ACC_INFO(tranIndex)->ivIndex = pNwkPduInfo->ivIndex;
      SAR_RX_TRAN_ACC_INFO(tranIndex)->szMic = pNwkPduInfo->pLtrPdu[1] & MESH_SZMIC_MASK;
      SAR_RX_TRAN_ACC_INFO(tranIndex)->aid = pNwkPduInfo->pLtrPdu[0] & MESH_AID_MASK;
      SAR_RX_TRAN_ACC_INFO(tranIndex)->akf = MESH_UTILS_BF_GET(pNwkPduInfo->pLtrPdu[0],
                                                             MESH_AKF_SHIFT, MESH_AKF_SIZE);
    }

    /* Initialize the transaction specific fields */
    sarRxCb.pTranInfoTable[tranIndex].srcAddr = pNwkPduInfo->src;
    sarRxCb.pTranInfoTable[tranIndex].seqZero = seqZero;
    sarRxCb.pTranInfoTable[tranIndex].segN = segN;
    sarRxCb.pTranInfoTable[tranIndex].recvIvIndex = SAR_RX_IVI_LSB(pNwkPduInfo->ivIndex);

    /* Check if there is (at least) one LPN destination for the PDU. */
    if(sarRxCb.lpnDstCheckCback(pNwkPduInfo->dst, pNwkPduInfo->netKeyIndex))
    {
      /* Check if Friend Queue maximum size can accommodate this transaction. */
      if((pMeshConfig->pMemoryConfig->maxNumFriendQueueEntries < (segN + 1)) ||
      /* Apply TTL filter rule for Friend Queue. */
         (pNwkPduInfo->ttl <= MESH_TX_TTL_FILTER_VALUE))

      {
        /* Check if the LPN was the only destination for the PDU. */
        if(MESH_IS_ADDR_UNICAST(pNwkPduInfo->dst))
        {
          /* No other elements needing this. So abort. */
          meshSarRxResetTransaction(tranIndex, TRUE);

          return 0;
        }
      }
      /* Allocate Friend Segment Info. */
      else if((sarRxCb.pTranInfoTable[tranIndex].pFriendSegInfo =
              (meshSarRxSegInfoFriend_t *)WsfBufAlloc((segN + 1) * sizeof(meshSarRxSegInfoFriend_t)))
               == NULL)
      {
        /* Check if the LPN was the only destination for the PDU. */
        if(MESH_IS_ADDR_UNICAST(pNwkPduInfo->dst))
        {
          /* No other elements needing this. So abort. */
          meshSarRxResetTransaction(tranIndex, TRUE);

          return 0;
        }
      }
    }

    /* Clear all transactions for the same source address with older SEQ Auth. */
    MeshSarRxHistoryCleanupOld(sarRxCb.pTranInfoTable[tranIndex].srcAddr,
                               sarRxCb.pTranInfoTable[tranIndex].seqZero,
                               sarRxCb.pTranInfoTable[tranIndex].recvIvIndex);

    /* Mark the transaction in progress */
    sarRxCb.pTranInfoTable[tranIndex].state = MESH_SAR_RX_TRAN_IN_PROGRESS;
  }
  else
  {
    /* The transaction is in progress */

    /* If no memory is available, return 0. */
    if (sarRxCb.pTranInfoTable[tranIndex].pLtrPduInfo == NULL)
    {
      return 0;
    }

    /* Verify segN, CTL are consistent */
    if ((segN != sarRxCb.pTranInfoTable[tranIndex].segN) ||
       (pNwkPduInfo->ctl != sarRxCb.pTranInfoTable[tranIndex].pduType))
    {
      return 0;
    }

    /* If we already received the segment, return with success */
    if (sarRxCb.pTranInfoTable[tranIndex].blockAckMask & (1 << segO))
    {
      /* Update timeout only if there is no pending ack for this transaction and the destination
       * was unicast
       */
      if (MESH_IS_ADDR_UNICAST(pNwkPduInfo->dst))
      {
        /* Read timer state under critical section. */
        WSF_CS_INIT(cs);
        WSF_CS_ENTER(cs);
        tmrStarted = sarRxCb.pTranInfoTable[tranIndex].ackTmr.isStarted;
        WSF_CS_EXIT(cs);

        if(!tmrStarted)
        {
          /* Use Default TTL to calculate timer period. */
          WsfTimerStartMs(&(sarRxCb.pTranInfoTable[tranIndex].ackTmr),
                          MESH_SAR_RX_ACK_TIMEOUT_MS(MeshLocalCfgGetDefaultTtl()));
        }
      }

      /* Update incomplete timeout for this transaction */
      WsfTimerStartMs(&(sarRxCb.pTranInfoTable[tranIndex].incompTmr), MESH_SAR_RX_INCOMPLETE_TIMEOUT_MS);

      return sarRxCb.pTranInfoTable[tranIndex].blockAckMask;
    }

    if (sarRxCb.pTranInfoTable[tranIndex].pduType == MESH_SAR_RX_TYPE_CTL)
    {
      /* This is a CTL message */

      /* Set pointer to the data region where the PDU must be appended */
      pData = ((uint8_t *)(SAR_RX_TRAN_CTL_INFO(tranIndex)->pUtrCtlPdu)) +
              (segO * MESH_CTL_SEG_MAX_LENGTH);

      /* Copy UTR segment data and update length */
      memcpy(pData, &pNwkPduInfo->pLtrPdu[MESH_SEG_DATA_PDU_OFFSET],
             pNwkPduInfo->pduLen - MESH_SEG_HEADER_LENGTH);
      SAR_RX_TRAN_CTL_INFO(tranIndex)->pduLen += (pNwkPduInfo->pduLen - MESH_SEG_HEADER_LENGTH);
      if (SAR_RX_TRAN_CTL_INFO(tranIndex)->gtSeqNo < pNwkPduInfo->seqNo)
      {
        SAR_RX_TRAN_CTL_INFO(tranIndex)->gtSeqNo = pNwkPduInfo->seqNo;
      }
    }
    else
    {
      /* This is an ACC message */

      /* Set pointer to the data region where the PDU must be appended */
      pData = (uint8_t *)(SAR_RX_TRAN_ACC_INFO(tranIndex)->pUtrAccPdu) +
              (segO * MESH_ACC_SEG_MAX_LENGTH);

      /* Copy UTR segment data and update length */
      memcpy(pData, &pNwkPduInfo->pLtrPdu[MESH_SEG_DATA_PDU_OFFSET],
             pNwkPduInfo->pduLen - MESH_SEG_HEADER_LENGTH);
      SAR_RX_TRAN_ACC_INFO(tranIndex)->pduLen += (pNwkPduInfo->pduLen - MESH_SEG_HEADER_LENGTH);
      if (SAR_RX_TRAN_ACC_INFO(tranIndex)->gtSeqNo < pNwkPduInfo->seqNo)
      {
        SAR_RX_TRAN_ACC_INFO(tranIndex)->gtSeqNo = pNwkPduInfo->seqNo;
      }
    }
  }

  /* Update the Block ACK Mask */
  sarRxCb.pTranInfoTable[tranIndex].blockAckMask |= (1 << segO);

  /* Set segment information if needed by Friend module. */
  if(sarRxCb.pTranInfoTable[tranIndex].pFriendSegInfo != NULL)
  {
    SAR_RX_TRAN_LAST_FRIEND_SEG_INFO(tranIndex).segO = segO;
    SAR_RX_TRAN_LAST_FRIEND_SEG_INFO(tranIndex).segSeqNo = pNwkPduInfo->seqNo;
    if(pNwkPduInfo->ctl)
    {
      /* Compute offset of the segment. */
      SAR_RX_TRAN_LAST_FRIEND_SEG_INFO(tranIndex).offset =
          (pData - SAR_RX_TRAN_CTL_INFO(tranIndex)->pUtrCtlPdu);
    }
    else
    {
      /* Compute offset of the segment. */
      SAR_RX_TRAN_LAST_FRIEND_SEG_INFO(tranIndex).offset =
          (pData - SAR_RX_TRAN_ACC_INFO(tranIndex)->pUtrAccPdu);
    }
    /* There should never be more than segN + 1 segments. */
    WSF_ASSERT(sarRxCb.pTranInfoTable[tranIndex].friendSegInfoIdx <= segN);
    /* Move to next position. */
    sarRxCb.pTranInfoTable[tranIndex].friendSegInfoIdx++;
  }
  /* Update timeout only if there is no pending ack for this transaction and the destination
   * was unicast
   */
  if (MESH_IS_ADDR_UNICAST(pNwkPduInfo->dst))
  {
    /* Read timer state under critical section. */
    WSF_CS_INIT(cs);
    WSF_CS_ENTER(cs);
    tmrStarted = sarRxCb.pTranInfoTable[tranIndex].ackTmr.isStarted;
    WSF_CS_EXIT(cs);

    if(!tmrStarted)
    {
      /* Use Default TTL to calculate timer period. */
      WsfTimerStartMs(&(sarRxCb.pTranInfoTable[tranIndex].ackTmr),
                      MESH_SAR_RX_ACK_TIMEOUT_MS(MeshLocalCfgGetDefaultTtl()));
    }
  }

  /* Update incomplete timeout for this transaction */
  WsfTimerStartMs(&(sarRxCb.pTranInfoTable[tranIndex].incompTmr), MESH_SAR_RX_INCOMPLETE_TIMEOUT_MS);

  return sarRxCb.pTranInfoTable[tranIndex].blockAckMask;
}

/*************************************************************************************************/
/*! \brief     Maintains acknowledgement timers for SAR Rx transactions.
 *
 *  param[in]  tranIndex Transaction index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSarRxAckTmrCback(uint8_t tranIndex)
{
  WSF_ASSERT(tranIndex < sarRxCb.tranInfoSize);
  WSF_ASSERT(sarRxCb.pTranInfoTable[tranIndex].state == MESH_SAR_RX_TRAN_IN_PROGRESS);

  /* Send ACK for this transaction */
  meshSarRxSendAck(tranIndex);
}

/*************************************************************************************************/
/*! \brief     Maintains incomplete timers for SAR Rx transactions.
 *
 *  param[in]  tranIndex Transaction index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSarRxIncompTmrCback(uint8_t tranIndex)
{

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
  meshTestSarRxTimeoutInd_t    rxTimeoutInd;
#endif

  WSF_ASSERT(tranIndex < sarRxCb.tranInfoSize);
  WSF_ASSERT(sarRxCb.pTranInfoTable[tranIndex].state == MESH_SAR_RX_TRAN_IN_PROGRESS);

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
  if (meshTestCb.listenMask & MESH_TEST_SAR_LISTEN)
  {
    rxTimeoutInd.hdr.event = MESH_TEST_EVENT;
    rxTimeoutInd.hdr.param = MESH_TEST_SAR_RX_TIMEOUT_IND;
    rxTimeoutInd.hdr.status = MESH_SUCCESS;
    rxTimeoutInd.srcAddr = sarRxCb.pTranInfoTable[tranIndex].srcAddr;

    meshTestCb.testCback((meshTestEvt_t *)&rxTimeoutInd);
  }
#endif

  /* Timeout transaction */
  meshSarRxResetTransaction(tranIndex,
                            (sarRxCb.pTranInfoTable[tranIndex].pFriendSegInfo != NULL));
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
static void meshSarRxWsfMsgHandlerCback(wsfMsgHdr_t *pMsg)
{
  switch(pMsg->event)
  {
    case MESH_SAR_RX_MSG_ACK_TMR_EXPIRED:
      meshSarRxAckTmrCback((uint8_t)(pMsg->param));
      break;
    case MESH_SAR_RX_MSG_INCOMP_TMR_EXPIRED:
      meshSarRxIncompTmrCback((uint8_t)(pMsg->param));
      break;
    default:
      break;
  }
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief   Initializes the table that contains all ongoing SAR Rx transactions.
 *
 *  \remarks This function also cleans the associated resources and stops the timers used by the
 *           module.
 *
 *  \return  None.
 */
/*************************************************************************************************/
void MeshSarRxInit(void)
{
  uint8_t *pMemBuff;
  uint32_t reqMem;
  uint8_t tranIndex;

  MESH_TRACE_INFO0("MESH SAR RX: Init");

  pMemBuff = meshCb.pMemBuff;

  /* Save the pointer for the SAR Rx Transaction Info table */
  sarRxCb.pTranInfoTable = (meshSarRxTranInfo_t *)pMemBuff;

  /* Increment the memory buffer pointer. */
  reqMem = meshSarRxTranInfoGetRequiredMemory(pMeshConfig->pMemoryConfig->sarRxTranInfoSize);
  pMemBuff += reqMem;

  /* Save the updated address. */
  meshCb.pMemBuff = pMemBuff;

  /* Subtract the reserved size from memory buffer size. */
  meshCb.memBuffSize -= reqMem;

  /* Set Transaction Info table size */
  sarRxCb.tranInfoSize = pMeshConfig->pMemoryConfig->sarRxTranInfoSize;

  /* Store empty callbacks into local structure. */
  sarRxCb.pduReassembledCback = meshSarRxEmptyPduReassembled;
  sarRxCb.lpnDstCheckCback = meshSarRxEmptyLpnDstCheckCback;
  sarRxCb.friendPduReassembledCback = meshSarRxEmptyFriendPduReassembledCback;

  /* Initialize SAR Rx Transaction History Table */
  MeshSarRxHistoryInit();

  /* Reset SAR Rx Transaction Table */
  for (tranIndex = 0; tranIndex < sarRxCb.tranInfoSize; tranIndex++)
  {
    memset(&sarRxCb.pTranInfoTable[tranIndex], 0, sizeof(meshSarRxTranInfo_t));

    /* Configure timers. */
    sarRxCb.pTranInfoTable[tranIndex].ackTmr.msg.event = MESH_SAR_RX_MSG_ACK_TMR_EXPIRED;
    sarRxCb.pTranInfoTable[tranIndex].incompTmr.msg.event = MESH_SAR_RX_MSG_INCOMP_TMR_EXPIRED;
    sarRxCb.pTranInfoTable[tranIndex].ackTmr.msg.param = tranIndex;
    sarRxCb.pTranInfoTable[tranIndex].incompTmr.msg.param = tranIndex;
    sarRxCb.pTranInfoTable[tranIndex].ackTmr.handlerId = meshCb.handlerId;
    sarRxCb.pTranInfoTable[tranIndex].incompTmr.handlerId = meshCb.handlerId;
  }

  /* Register WSF message handler. */
  meshCb.sarRxMsgCback = meshSarRxWsfMsgHandlerCback;
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the required callback used by the SAR Rx.
 *
 *  \param[in] pduReassembledCback  Callback invoked by the SAR Rx module when a complete PDU is
 *                                  reassembled. The PDU is formatted depending on the intended
 *                                  recipient.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSarRxRegister(meshSarRxPduReassembledCback_t pduReassembledCback)
{
  /* Validate input parameters */
  if (pduReassembledCback == NULL)
  {
    MESH_TRACE_ERR0("MESH SAR RX: Invalid callback registered!");
    return;
  }

  /* Store callback into local structure. */
  sarRxCb.pduReassembledCback = pduReassembledCback;
}

/*************************************************************************************************/
/*!
 *  \brief     Registers callbacks for checking and adding reassembled PDU's to Friend Queue.
 *
 *  \param[in] lpnDstCheckCback    Callback that checks if at least one LPN is destination for a
 *                                 PDU.
 *  \param[in] friendPduReasCback  Callback invoked by the SAR Rx module when a complete PDU is
 *                                 reassembled and destination is at least one LPN.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSarRxRegisterFriend(meshSarRxLpnDstCheckCback_t lpnDstCheckCback,
                             meshSarRxFriendPduReassembledCback_t friendPduReasCback)
{
  /* Store friendship callbacks. */
  if((lpnDstCheckCback != NULL) && (friendPduReasCback != NULL))
  {
    sarRxCb.lpnDstCheckCback = lpnDstCheckCback;
    sarRxCb.friendPduReassembledCback = friendPduReasCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Processes a segment contained in a Network PDU Info structure.
 *
 *  \param[in] pNwkPduInfo  Pointer to the Network PDU Information structure.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSarRxProcessSegment(const meshNwkPduRxInfo_t *pNwkPduInfo)
{
  uint16_t seqZero;
  uint8_t segO, segN, tranIndex;
  uint32_t blockAckMask;
  bool_t sendAck;
  bool_t obo;

  /* Validate input parameters */
  if (pNwkPduInfo == NULL)
  {
    WSF_ASSERT(pNwkPduInfo != NULL);
    return;
  }

  /* Extract SeqZero field. */
  seqZero = (((uint16_t)(MESH_UTILS_BF_GET(pNwkPduInfo->pLtrPdu[MESH_SEQ_ZERO_H_PDU_OFFSET],
                                           MESH_SEQ_ZERO_H_SHIFT,
                                           MESH_SEQ_ZERO_H_SIZE)) << MESH_SEQ_ZERO_L_SIZE) |
             (uint8_t)(MESH_UTILS_BF_GET(pNwkPduInfo->pLtrPdu[MESH_SEQ_ZERO_L_PDU_OFFSET],
                                         MESH_SEQ_ZERO_L_SHIFT,
                                         MESH_SEQ_ZERO_L_SIZE)));

  /* Extract Seg0 field. */
  segO = (((pNwkPduInfo->pLtrPdu[MESH_SEG_ZERO_H_PDU_OFFSET]) & MESH_SEG_ZERO_H_MASK) <<
          MESH_SEG_ZERO_H_SHIFT) |
          (((pNwkPduInfo->pLtrPdu[MESH_SEG_ZERO_L_PDU_OFFSET]) & MESH_SEG_ZERO_L_MASK) >>
          MESH_SEG_ZERO_L_SHIFT) ;

  /* Extract SegN field. */
  segN = pNwkPduInfo->pLtrPdu[MESH_SEG_N_PDU_OFFSET] & MESH_SEG_N_MASK;

  /* Validate SegO <= SegN */
  if (segO > segN)
  {
    WSF_ASSERT(segO <= segN);
    return;
  }

  /* Check SAR Rx Transaction Cache to see if we have an outdated segment. */
  if (!MeshSarRxHistoryCheck(pNwkPduInfo->src, pNwkPduInfo->seqNo, seqZero,
                             SAR_RX_IVI_LSB(pNwkPduInfo->ivIndex), segN, &sendAck, &obo))

  {
    MESH_TRACE_INFO0("MESH SAR RX: Duplicate or outdated segment!");

    /* If the message was unicast send an error ack or last ack message  */
    if (MESH_IS_ADDR_UNICAST(pNwkPduInfo->dst) && sendAck)
    {
      meshSarRxSendFastAck(pNwkPduInfo, SAR_RX_BLOCK_MASK(segN), obo);
    }

    return;
  }

  /* Get an entry to store/update transaction */
  tranIndex = meshSarRxGetTransactionIndex(pNwkPduInfo->src, pNwkPduInfo->dst, segN);

  if (tranIndex == sarRxCb.tranInfoSize)
  {
    MESH_TRACE_WARN0("MESH SAR RX: No more transaction slots!");

    /* If the message was unicast send an error ack message */
    if (MESH_IS_ADDR_UNICAST(pNwkPduInfo->dst))
    {
      meshSarRxSendFastAck(pNwkPduInfo, 0,
                           sarRxCb.lpnDstCheckCback(pNwkPduInfo->dst, pNwkPduInfo->netKeyIndex));
    }

    return;
  }

  if (sarRxCb.pTranInfoTable[tranIndex].state == MESH_SAR_RX_TRAN_IN_PROGRESS)
  {
    /* Check if received SEQ Auth is lower than the current one. */
    if ((sarRxCb.pTranInfoTable[tranIndex].recvIvIndex > SAR_RX_IVI_LSB(pNwkPduInfo->ivIndex)) ||
        ((sarRxCb.pTranInfoTable[tranIndex].recvIvIndex == SAR_RX_IVI_LSB(pNwkPduInfo->ivIndex)) &&
        (sarRxCb.pTranInfoTable[tranIndex].seqZero > seqZero)))
    {
      return;
    }

    /* Check if the new segment has a greater SEQ Auth. */
    if ((sarRxCb.pTranInfoTable[tranIndex].recvIvIndex < SAR_RX_IVI_LSB(pNwkPduInfo->ivIndex)) ||
        ((sarRxCb.pTranInfoTable[tranIndex].recvIvIndex == SAR_RX_IVI_LSB(pNwkPduInfo->ivIndex)) &&
         (sarRxCb.pTranInfoTable[tranIndex].seqZero < seqZero)))
    {
      meshSarRxResetTransaction(tranIndex,
                                (sarRxCb.pTranInfoTable[tranIndex].pFriendSegInfo != NULL));
    }
  }

  /* Store/update transaction at the found entry index */
  blockAckMask = meshSarRxAddUpdateTransaction(pNwkPduInfo, tranIndex);

  if (!blockAckMask)
  {
    MESH_TRACE_WARN0("MESH SAR RX: No more memory for transactions!");

    /* If the message was unicast send an error ack message */
    if (MESH_IS_ADDR_UNICAST(pNwkPduInfo->dst))
    {
      meshSarRxSendFastAck(pNwkPduInfo, 0,
                           sarRxCb.lpnDstCheckCback(pNwkPduInfo->dst, pNwkPduInfo->netKeyIndex));
    }

    return;
  }
  else
  {
    /* Transaction has been successfully updated */

    if (blockAckMask == SAR_RX_BLOCK_MASK(segN))
    {
      sarRxCb.pTranInfoTable[tranIndex].state = MESH_SAR_RX_TRAN_COMPLETE;

      /* If the message was unicast send an ack message */
      if (MESH_IS_ADDR_UNICAST(pNwkPduInfo->dst))
      {
        meshSarRxSendAck(tranIndex);
      }

      /* Received all blocks. Decide upon destination of reassembled PDU. */

      /* Check if Friend module requires this PDU. */
      if(sarRxCb.pTranInfoTable[tranIndex].pFriendSegInfo != NULL)
      {
        sarRxCb.friendPduReassembledCback(sarRxCb.pTranInfoTable[tranIndex].pduType,
                                          sarRxCb.pTranInfoTable[tranIndex].pLtrPduInfo,
                                          sarRxCb.pTranInfoTable[tranIndex].pFriendSegInfo,
                                          pNwkPduInfo->ivIndex,
                                          seqZero,
                                          sarRxCb.pTranInfoTable[tranIndex].segN);

        /* Check if destination is unicast. */
        if(MESH_IS_ADDR_UNICAST(pNwkPduInfo->dst))
        {
          /* Free memory since Friend copies PDU in the Friend Queue. */
          WsfBufFree(sarRxCb.pTranInfoTable[tranIndex].pLtrPduInfo);

          /* Reset SAR RX Transaction. No more recipients for the message. */
          meshSarRxResetTransaction(tranIndex, TRUE);

          return;
        }
      }
      /* Received all blocks. Send message to UTR. */
      sarRxCb.pduReassembledCback(sarRxCb.pTranInfoTable[tranIndex].pduType,
                                  sarRxCb.pTranInfoTable[tranIndex].pLtrPduInfo);

      /* Reset SAR RX Transaction. */
      meshSarRxResetTransaction(tranIndex, FALSE);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Computes the required memory to be provided based on the given configuration.
 *
 *  \return Required memory in bytes or 0 in case of fail.
 */
/*************************************************************************************************/
uint32_t MeshSarRxGetRequiredMemory(void)
{
  uint32_t reqMem = MESH_MEM_REQ_INVALID_CFG;

  if ((pMeshConfig->pMemoryConfig == NULL) ||
    (pMeshConfig->pMemoryConfig->sarRxTranHistorySize == 0) ||
    (pMeshConfig->pMemoryConfig->sarRxTranInfoSize == 0))
  {
    return reqMem;
  }

  /* Compute the required memory. */
  reqMem = meshSarRxTranInfoGetRequiredMemory(pMeshConfig->pMemoryConfig->sarRxTranInfoSize) +
    MeshSarRxHistoryGetRequiredMemory();

  return reqMem;
}
