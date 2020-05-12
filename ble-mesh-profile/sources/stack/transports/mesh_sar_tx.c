/*************************************************************************************************/
/*!
 *  \file   mesh_sar_tx.c
 *
 *  \brief  SAR Tx implementation.
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
#include "wsf_msg.h"
#include "wsf_os.h"
#include "wsf_assert.h"
#include "wsf_buf.h"
#include "wsf_cs.h"
#include "wsf_trace.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_error_codes.h"
#include "mesh_api.h"
#include "mesh_main.h"

#include "mesh_seq_manager.h"
#include "mesh_network.h"
#include "mesh_network_mgmt.h"
#include "mesh_lower_transport.h"
#include "mesh_sar_tx.h"
#include "mesh_sar_utils.h"
#include "mesh_utils.h"
#include "mesh_api.h"
#include "mesh_main.h"
#include "mesh_seq_manager.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/
/*! Defines the unicast timer offset in ms */
#define MESH_SAR_TX_UNICAST_SEG_TMR_OFFSET_MS   200

/*! Defines the unicast segment transmission timer value in milliseconds based on PDU TTL */
#define MESH_SAR_TX_UNICAST_SEG_TMR_MS(ttl)     (MESH_SAR_TX_UNICAST_SEG_TMR_OFFSET_MS +\
                                                 (50 * (ttl)))

/*! Defines the multicast segment transmission timer value in milliseconds  */
#define MESH_SAR_TX_MULTICAST_SEG_TMR_MS        500

/*! Defines the number of segment transmissions for multicast destinations */
#define MESH_SAR_TX_MULTICAST_RETRANSMISSIONS   3

/*! Defines the number of unicast retransmissions */
#define MESH_SAR_TX_UNICAST_RETRANSMISSIONS     3

/*! Quick access to transaction table */
#define MESH_SAR_TX_TRAN_TABLE  (sarTxCb.pTranTable)

/*! Quick access to transaction table size */
#define MESH_SAR_TX_TRAN_TABLE_SIZE  (sarTxCb.maxTransactions)

/*! Creates the SAR Tx Block Mask with all fragments received */
#define MESH_SAR_TX_BLOCK_MASK(segCount)  (((segCount) == 32U) ? 0xFFFFFFFFU : (uint32_t)MESH_UTILS_BTMASK_MAKE(segCount))

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! SAR Tx WSF message events */
enum meshSarTxWsfMsgEvents
{
  MESH_SAR_TX_MSG_RETRY_TMR_EXPIRED = MESH_SAR_TX_MSG_START /*!< Retry timer expired event */
};

/*! States of SAR Tx Transaction */
enum meshSarTxTranStates
{
  SAR_TX_TRAN_INACTIVE,
  SAR_TX_TRAN_ACTIVE
};

/*! SAR Tx Transaction state data type */
typedef uint8_t meshSarTxTranState_t;

/*! Upper Transport layer and Lower Transport layer exchange Access packets in this format */
typedef struct meshSarTxTranInfo_tag
{
  meshSarTxTranState_t state;        /*!< Indicates transaction state.
                                      */
  void                 *pUtrBuffer;  /*!< UTR buffer; to be released when transaction is done.
                                      */
  uint8_t              *pUtrPdu;     /*!< Pointer to the UTR PDU; length is contained in the other fields:
                                      *    segCount, maxSegLength and lastSegLength.
                                      */
  meshNwkPduTxInfo_t  nwkPduInfo;    /*!< Network PDU info, common for all segments.
                                      *   The pUtrPdu and utrPduLen fields need to be updated
                                      *   before sending each segment to the Network Layer.
                                      *   The Network Layer sets the seqNo field.
                                      */
  meshSarSegHdr_t     segHdr;        /*!< The segmentation header, common for all segments.
                                      *   The segment index must be updated with meshSarSetSegHdrSegO
                                      *   before sending each segment to the Network Layer.
                                      */
  meshSarTxBlockAck_t blockAckMask;  /*!< Acknowledged blocks mask for the
                                      *   current SAR Tx transaction
                                      */
  uint16_t            seqZero;       /*!< Least significant bits of SeqAuth. It is used to
                                      *   identify the SAR Tx transaction
                                      */
  uint8_t             segCount;      /*!< Total number of segments (== SegN + 1)
                                      */
  uint8_t             maxSegLength;  /*!< Length of the segment for the SAR Tx transaction
                                      */
  uint8_t             lastSegLength; /*!< Length of the last segment for the SAR Tx transaction.
                                      *   Value is similar to segLength when the Lower Transport
                                      *   PDU length is a multiple of segNo.
                                      */
  wsfTimer_t          retryTmr;      /*!< Retransmission timer */
  uint8_t             retryCounter;  /*!< Remaining number of segment transmissions. */
} meshSarTxTranInfo_t;

/*! Mesh SAR Tx control block type definition */
typedef struct meshSarTxCb_t
{
  meshSarTxNotifyCback_t          notifyCback;        /*!< Transaction notification callback */
  meshSarTxTranInfo_t             *pTranTable;         /*!< Table containing info about SAR Tx
                                                         *   transactions
                                                         */
  uint8_t                         maxTransactions;    /*!< Transaction table size */
  bool_t                          rejectTran;         /*!< Reject new transactions */
  uint8_t                         activeTranCnt;      /*!< Active transactions count */
} meshSarTxCb_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Mesh SAR Tx control block */
static meshSarTxCb_t sarTxCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Computes total memory required by this module.
 *
 *  \param[in] maxTransactions  Maximum number of parallel transactions.
 *
 *  \return    Memory size (in octets), aligned to the architecture.
 */
/*************************************************************************************************/
static inline uint32_t meshSarTxLocalGetRequiredMemory(uint8_t maxTransactions)
{
  return MESH_UTILS_ALIGN(sizeof(meshSarTxTranInfo_t) * maxTransactions);
}

/*************************************************************************************************/
/*!
 *  \brief     Default SAR-Tx transaction notification callback.
 *
 *  \param[in] eventStatus  Event status.
 *  \param[in] dst          Destination identifying the transaction.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSarTxEmptyNotifyCback(meshSarTxEventStatus_t eventStatus,
                                      meshAddress_t dst)
{
  (void)eventStatus;
  (void)dst;

  MESH_TRACE_INFO0("MESH SAR Tx: Transaction notification callback not set!");
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes transaction table.
 *
 *  \param[in] reset  If TRUE, then existing upper layer buffers are freed.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSarTxInitTranDetails(bool_t reset)
{
  wsfMsgHdr_t msg = { .event = MESH_NWK_MGMT_MSG_IV_UPDT_ALLOWED };

  uint8_t index;

  for (index = 0; index < MESH_SAR_TX_TRAN_TABLE_SIZE; index++)
  {
    MESH_SAR_TX_TRAN_TABLE[index].state = SAR_TX_TRAN_INACTIVE;
    if (reset)
    {
      if(MESH_SAR_TX_TRAN_TABLE[index].pUtrBuffer != NULL)
      {
        WsfBufFree(MESH_SAR_TX_TRAN_TABLE[index].pUtrBuffer);
      }
      WsfTimerStop(&(MESH_SAR_TX_TRAN_TABLE[index].retryTmr));
    }
    MESH_SAR_TX_TRAN_TABLE[index].pUtrBuffer = NULL;
    MESH_SAR_TX_TRAN_TABLE[index].nwkPduInfo.ltrHdrLen = MESH_LTR_SEG_HDR_LEN;
    MESH_SAR_TX_TRAN_TABLE[index].nwkPduInfo.pLtrHdr = MESH_SAR_TX_TRAN_TABLE[index].segHdr.bytes;
    MESH_SAR_TX_TRAN_TABLE[index].retryTmr.msg.event = MESH_SAR_TX_MSG_RETRY_TMR_EXPIRED;
    MESH_SAR_TX_TRAN_TABLE[index].retryTmr.msg.param = index;
    MESH_SAR_TX_TRAN_TABLE[index].retryTmr.handlerId = meshCb.handlerId;
  }

  sarTxCb.activeTranCnt = 0;
  /* Notify Network Management to switch to new IV if needed. */
  meshCb.nwkMgmtMsgCback(&msg);
}

/*************************************************************************************************/
/*!
 *  \brief     Sends all unacknowledged segments for a transaction.
 *
 *  \param[in] tranIndex  Transaction index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSarTxSendUnackedSegments(uint8_t tranIndex)
{
  uint8_t i;
  for (i = 0; i < MESH_SAR_TX_TRAN_TABLE[tranIndex].segCount; i++)
    /* Iterate through the block ACK mask for each segment */
  {
    if (0 == (MESH_SAR_TX_TRAN_TABLE[tranIndex].blockAckMask & (1 << i)))
      /* This segment has not been acknowledged */
    {
      /* Set the pUtrPdu pointer to the correct position within the UTR PDU */
      MESH_SAR_TX_TRAN_TABLE[tranIndex].nwkPduInfo.pUtrPdu = MESH_SAR_TX_TRAN_TABLE[tranIndex].pUtrPdu
                                                + i * MESH_SAR_TX_TRAN_TABLE[tranIndex].maxSegLength;

      MESH_SAR_TX_TRAN_TABLE[tranIndex].nwkPduInfo.pLtrHdr =
        (uint8_t *)&(MESH_SAR_TX_TRAN_TABLE[tranIndex].segHdr);
      MESH_SAR_TX_TRAN_TABLE[tranIndex].nwkPduInfo.ltrHdrLen =
        sizeof(MESH_SAR_TX_TRAN_TABLE[tranIndex].segHdr);

      /* Set the utrPduLen to either the maximum length or to the last segment length */
      MESH_SAR_TX_TRAN_TABLE[tranIndex].nwkPduInfo.utrPduLen =
        (i == MESH_SAR_TX_TRAN_TABLE[tranIndex].segCount - 1) ?
        MESH_SAR_TX_TRAN_TABLE[tranIndex].lastSegLength :
        MESH_SAR_TX_TRAN_TABLE[tranIndex].maxSegLength;
      /* Set the SegO field in the segmentation header */
      meshSarSetSegHdrSegO(&MESH_SAR_TX_TRAN_TABLE[tranIndex].segHdr, i);

      /* Set the next sequence number */
      if (MeshSeqGetNumber(MESH_SAR_TX_TRAN_TABLE[tranIndex].nwkPduInfo.src,
                           &MESH_SAR_TX_TRAN_TABLE[tranIndex].nwkPduInfo.seqNo,
                           TRUE) != MESH_SUCCESS)
      {
        /* Abort. Out of sequence numbers. */
        return;
      }

      /* Send the PDU to the Network Layer */
      MeshNwkSendLtrPdu(&MESH_SAR_TX_TRAN_TABLE[tranIndex].nwkPduInfo);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Starts the timer for a transaction.
 *
 *  \param[in] tranIndex  Transaction index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSarTxStartTimerForTran(uint8_t tranIndex)
{
  if (MESH_IS_ADDR_GROUP(MESH_SAR_TX_TRAN_TABLE[tranIndex].nwkPduInfo.dst) ||
      MESH_IS_ADDR_VIRTUAL(MESH_SAR_TX_TRAN_TABLE[tranIndex].nwkPduInfo.dst))
  /* Multicast destination */
  {
    WsfTimerStartMs(&(MESH_SAR_TX_TRAN_TABLE[tranIndex].retryTmr), MESH_SAR_TX_MULTICAST_SEG_TMR_MS);
    MESH_SAR_TX_TRAN_TABLE[tranIndex].retryCounter = MESH_SAR_TX_MULTICAST_RETRANSMISSIONS;
  }
  else
  /* Unicast destination */
  {
    WsfTimerStartMs(&(MESH_SAR_TX_TRAN_TABLE[tranIndex].retryTmr),
                    MESH_SAR_TX_UNICAST_SEG_TMR_MS(MESH_SAR_TX_TRAN_TABLE[tranIndex].nwkPduInfo.ttl));
    MESH_SAR_TX_TRAN_TABLE[tranIndex].retryCounter = MESH_SAR_TX_UNICAST_RETRANSMISSIONS;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Stops the timer for a transaction.
 *
 *  \param[in] tranIndex  Transaction index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSarTxStopTimerForTran(uint8_t tranIndex)
{
  /* Stop WSF timer. */
  WsfTimerStop(&(MESH_SAR_TX_TRAN_TABLE[tranIndex].retryTmr));
}

/*************************************************************************************************/
/*!
 *  \brief     Starts a transaction.
 *
 *  \param[in] tranIndex  Transaction index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSarTxBeginTran(uint8_t tranIndex)
{
  wsfMsgHdr_t msg = { .event = MESH_NWK_MGMT_MSG_IV_UPDT_DISALLOWED };

  MESH_SAR_TX_TRAN_TABLE[tranIndex].state = SAR_TX_TRAN_ACTIVE;
  MESH_SAR_TX_TRAN_TABLE[tranIndex].blockAckMask = 0; //Perhaps this should be a memset with sizeof(struct)

  /* Increment active transactions count. */
  WSF_ASSERT(sarTxCb.activeTranCnt < sarTxCb.maxTransactions);
  if(sarTxCb.activeTranCnt < sarTxCb.maxTransactions)
  {
    sarTxCb.activeTranCnt++;

    /* If first transaction. */
    if(sarTxCb.activeTranCnt == 1)
    {
      /* Notify Network Management not to switch to new IV. */
      meshCb.nwkMgmtMsgCback(&msg);
    }
  }

  meshSarTxSendUnackedSegments(tranIndex);

  meshSarTxStartTimerForTran(tranIndex);
}

/*************************************************************************************************/
/*!
 *  \brief     Ends a transaction.
 *
 *  \param[in] tranIndex  Transaction index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSarTxEndTran(uint8_t tranIndex)
{
  wsfMsgHdr_t msg = { .event = MESH_NWK_MGMT_MSG_IV_UPDT_ALLOWED };

  /* Decrement active transactions count. */
  WSF_ASSERT(sarTxCb.activeTranCnt > 0);
  if(sarTxCb.activeTranCnt)
  {
    sarTxCb.activeTranCnt--;
  }

  meshSarTxStopTimerForTran(tranIndex);
  MESH_SAR_TX_TRAN_TABLE[tranIndex].state = SAR_TX_TRAN_INACTIVE;
  if (MESH_SAR_TX_TRAN_TABLE[tranIndex].pUtrBuffer != NULL)
  {
    WsfBufFree(MESH_SAR_TX_TRAN_TABLE[tranIndex].pUtrBuffer);
    MESH_SAR_TX_TRAN_TABLE[tranIndex].pUtrBuffer = NULL;
  }

  /* Notify Network Management to switch to new IV if needed. */
  if(sarTxCb.activeTranCnt == 0)
  {
    meshCb.nwkMgmtMsgCback(&msg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Retry timer callback.
 *
 *  \param[in] tranIndex  Transaction index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshSarTxRetryTmrCback(uint8_t tranIndex)
{
  /* Check if retries left. */
  if( MESH_SAR_TX_TRAN_TABLE[tranIndex].retryCounter == 0)
  {
    /* End transaction */
    meshSarTxEndTran(tranIndex);

    return;
  }

  /* Resend unacked segments */
  meshSarTxSendUnackedSegments(tranIndex);

  /* Decrement retry counter. */
  MESH_SAR_TX_TRAN_TABLE[tranIndex].retryCounter--;

  /* Re-arm timer. */
  if (MESH_IS_ADDR_UNICAST(MESH_SAR_TX_TRAN_TABLE[tranIndex].nwkPduInfo.dst))
  {
    WsfTimerStartMs(&(MESH_SAR_TX_TRAN_TABLE[tranIndex].retryTmr),
                    MESH_SAR_TX_UNICAST_SEG_TMR_MS(MESH_SAR_TX_TRAN_TABLE[tranIndex].nwkPduInfo.ttl));
  }
  else
  {
    WsfTimerStartMs(&(MESH_SAR_TX_TRAN_TABLE[tranIndex].retryTmr), MESH_SAR_TX_MULTICAST_SEG_TMR_MS);
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
static void meshSarTxWsfMsgHandlerCback(wsfMsgHdr_t *pMsg)
{
  switch(pMsg->event)
  {
    case MESH_SAR_TX_MSG_RETRY_TMR_EXPIRED:
      /* Invoke timer callback. */
      meshSarTxRetryTmrCback((uint8_t)(pMsg->param));
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
 *  \brief  Initialises the SAR Tx module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshSarTxInit(void)
{
  uint32_t reqMem;

  MESH_TRACE_INFO0("MESH SAR Tx: Init");

  /* Save the pointer for the SAR Rx Transaction Info table. */
  sarTxCb.pTranTable = (meshSarTxTranInfo_t*) meshCb.pMemBuff;

  /* Compute required memory */
  reqMem = meshSarTxLocalGetRequiredMemory(pMeshConfig->pMemoryConfig->sarTxMaxTransactions);

  /* Increment the memory buffer pointer. */
  meshCb.pMemBuff += reqMem;

  /* Subtract the reserved size from memory buffer size. */
  meshCb.memBuffSize -= reqMem;

  /* Set transaction info table size */
  sarTxCb.maxTransactions = pMeshConfig->pMemoryConfig->sarTxMaxTransactions;

  /* Store empty callback into local structure. */
  sarTxCb.notifyCback = meshSarTxEmptyNotifyCback;

  meshSarTxInitTranDetails(FALSE);

  /* Register WSF message handler. */
  meshCb.sarTxMsgCback = meshSarTxWsfMsgHandlerCback;

  /* Start by accepting transactions. */
  MeshSarTxAcceptIncoming();
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the notification callback for the upper layer.
 *
 *  \param[in] notifyCback  Callback invoked when a SAR Tx transaction completes successfully or
 *                          with an error.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSarTxRegister(meshSarTxNotifyCback_t notifyCallback)
{
  /* Validate input parameters. */
  if (notifyCallback == NULL)
  {
    MESH_TRACE_ERR0("MESH SAR Tx: Invalid callback register attempt!");
    return;
  }

  /* Store callback into local structure. */
  sarTxCb.notifyCback = notifyCallback;
}

/*************************************************************************************************/
/*!
 *  \brief  Resets all ongoing SAR Tx transactions.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshSarTxReset(void)
{
  meshSarTxInitTranDetails(TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief  Instructs SAR Tx to reject new transactions.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshSarTxRejectIncoming(void)
{
  sarTxCb.rejectTran = TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief  Instructs SAR Tx to accept new transactions.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshSarTxAcceptIncoming(void)
{
  sarTxCb.rejectTran = FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Creates a SAR Tx transaction for a Control Message that is received by the Lower
 *             Transport from the Upper Transport and requires segmentation.
 *
 *  \param[in] pLtrPduInfo  Pointer to the Lower Transport PDU Information structure.
 *
 *  \return    TRUE if transaction started, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshSarTxStartSegCtlTransaction(meshLtrCtlPduInfo_t *pLtrPduInfo)
{
  uint8_t index;

  /* Do not accept transactions if not allowed. */
  if (sarTxCb.rejectTran)
  {
    /* Release the UTR PDU buffer. */
    WsfBufFree(pLtrPduInfo);
    return FALSE;
  }

  /* Check if there are any slots left. */
  if(sarTxCb.activeTranCnt >= sarTxCb.maxTransactions)
  {
    /* Release the UTR PDU buffer. */
    WsfBufFree(pLtrPduInfo);
    return FALSE;
  }

  for (index = 0; index < MESH_SAR_TX_TRAN_TABLE_SIZE; index++)
  {
    if (MESH_SAR_TX_TRAN_TABLE[index].state == SAR_TX_TRAN_INACTIVE)
    {
      break;
    }
  }

  if (index == MESH_SAR_TX_TRAN_TABLE_SIZE)
  {
    WSF_ASSERT(index != MESH_SAR_TX_TRAN_TABLE_SIZE);

    /* Release the UTR PDU buffer. */
    WsfBufFree(pLtrPduInfo);
    return FALSE;
  }

  /* Parameter validation. */
  if (pLtrPduInfo->pduLen == 0 ||
    pLtrPduInfo->pduLen > MESH_LTR_MAX_UTR_PDU_LEN)
  {
    /* Release the UTR PDU buffer. */
    WsfBufFree(pLtrPduInfo);
    return FALSE;
  }

  MESH_SAR_TX_TRAN_TABLE[index].pUtrBuffer = pLtrPduInfo;
  MESH_SAR_TX_TRAN_TABLE[index].maxSegLength = MESH_LTR_MAX_SEG_UTR_CTL_PDU_LEN;
  MESH_SAR_TX_TRAN_TABLE[index].pUtrPdu = pLtrPduInfo->pUtrCtlPdu;

  /* Compute segment count and last length. */
  meshSarComputeSegmentCountAndLastLength(pLtrPduInfo->pduLen,
                                          MESH_LTR_MAX_SEG_UTR_CTL_PDU_LEN,
                                          &MESH_SAR_TX_TRAN_TABLE[index].segCount,
                                          &MESH_SAR_TX_TRAN_TABLE[index].lastSegLength);

  /* SeqZero is the 13 least significant bits of the sequence number. */
  MESH_SAR_TX_TRAN_TABLE[index].seqZero = pLtrPduInfo->seqNo & MESH_SEQ_ZERO_MASK;

  /* Initialize segmentation header. */
  meshSarInitSegHdrForCtl(&MESH_SAR_TX_TRAN_TABLE[index].segHdr,
                          pLtrPduInfo->opcode,
                          MESH_SAR_TX_TRAN_TABLE[index].seqZero,
                          MESH_SAR_TX_TRAN_TABLE[index].segCount - 1);

  /* Initialize PDU info. */
  meshNwkPduTxInfo_t* pInfo = &MESH_SAR_TX_TRAN_TABLE[index].nwkPduInfo;
  pInfo->ctl = 1;
  pInfo->dst = pLtrPduInfo->dst;
  pInfo->netKeyIndex = pLtrPduInfo->netKeyIndex;
  pInfo->src = pLtrPduInfo->src;
  pInfo->ttl = pLtrPduInfo->ttl;
  pInfo->prioritySend = FALSE;

  /* From this point on, the transaction is agnostic of PDU type (acc/ctl). */
  meshSarTxBeginTran(index);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Creates a SAR Tx transaction for an Access Message that is received by the Lower
 *             Transport from the Upper Transport and requires segmentation.
 *
 *  \param[in] pLtrPduInfo  Pointer to the Lower Transport PDU Information structure.
 *
 *  \return    TRUE if transaction started, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshSarTxStartSegAccTransaction(meshLtrAccPduInfo_t *pLtrPduInfo)
{
  uint8_t index;

  /* Do not accept transactions if not allowed. */
  if (sarTxCb.rejectTran)
  {
    /* Release the UTR PDU buffer. */
    WsfBufFree(pLtrPduInfo);
    return FALSE;
  }

  /* Check if there are any slots left. */
  if(sarTxCb.activeTranCnt >= sarTxCb.maxTransactions)
  {
    /* Release the UTR PDU buffer. */
    WsfBufFree(pLtrPduInfo);
    return FALSE;
  }

  for (index = 0; index < MESH_SAR_TX_TRAN_TABLE_SIZE; index++)
  {
    if (MESH_SAR_TX_TRAN_TABLE[index].state == SAR_TX_TRAN_INACTIVE)
    {
      break;
    }
  }

  if (index == MESH_SAR_TX_TRAN_TABLE_SIZE)
  {
    WSF_ASSERT(index != MESH_SAR_TX_TRAN_TABLE_SIZE);

    /* Release the UTR PDU buffer. */
    WsfBufFree(pLtrPduInfo);
    return FALSE;
  }

  /* Parameter validation. */
  if (pLtrPduInfo->pduLen == 0 ||
    pLtrPduInfo->pduLen > MESH_LTR_MAX_UTR_PDU_LEN)
  {
    /* Release the UTR PDU buffer. */
    WsfBufFree(pLtrPduInfo);
    return FALSE;
  }

  MESH_SAR_TX_TRAN_TABLE[index].pUtrBuffer = pLtrPduInfo;
  MESH_SAR_TX_TRAN_TABLE[index].maxSegLength = MESH_LTR_MAX_SEG_UTR_ACC_PDU_LEN;
  MESH_SAR_TX_TRAN_TABLE[index].pUtrPdu = pLtrPduInfo->pUtrAccPdu;

  /* Compute segment count and last length. */
  meshSarComputeSegmentCountAndLastLength(pLtrPduInfo->pduLen,
                                          MESH_LTR_MAX_SEG_UTR_ACC_PDU_LEN,
                                          &MESH_SAR_TX_TRAN_TABLE[index].segCount,
                                          &MESH_SAR_TX_TRAN_TABLE[index].lastSegLength);

  /* SeqZero is the 13 least significant bits of the sequence number. */
  MESH_SAR_TX_TRAN_TABLE[index].seqZero = pLtrPduInfo->seqNo & MESH_SEQ_ZERO_MASK;

  /* Initialize segmentation header. */
  meshSarInitSegHdrForAcc(&MESH_SAR_TX_TRAN_TABLE[index].segHdr,
                          pLtrPduInfo->akf,
                          pLtrPduInfo->aid,
                          pLtrPduInfo->szMic,
                          MESH_SAR_TX_TRAN_TABLE[index].seqZero,
                          MESH_SAR_TX_TRAN_TABLE[index].segCount - 1);

  /* Initialize PDU info. */
  meshNwkPduTxInfo_t* pInfo = &MESH_SAR_TX_TRAN_TABLE[index].nwkPduInfo;
  pInfo->ctl = 0;
  pInfo->dst = pLtrPduInfo->dst;
  pInfo->netKeyIndex = pLtrPduInfo->netKeyIndex;
  pInfo->src = pLtrPduInfo->src;
  pInfo->ttl = pLtrPduInfo->ttl;
  pInfo->prioritySend = FALSE;

  /* From this point on, the transaction is agnostic of PDU type (acc/ctl). */
  meshSarTxBeginTran(index);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Find an ongoing SAR Tx transaction and marks segments as acknowledged. If the
 *             segmented transaction is completed, all allocated memory is freed.
 *
 *  \param[in] remoteAddress  Mesh address of the remote device.
 *  \param[in] seqZero        Seq Zero value for the transaction.
 *  \param[in] oboFlag        On-behalf-of flag for this transaction.
 *  \param[in] blockAck       BlockAck field as received in the Segment Acknowledgement.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSarTxProcessBlockAck(meshAddress_t remoteAddress,
                              uint16_t seqZero,
                              bool_t oboFlag,
                              meshSarTxBlockAck_t blockAck)
{
  uint8_t index;
  for (index = 0; index < MESH_SAR_TX_TRAN_TABLE_SIZE; index++)
  {
    if (MESH_SAR_TX_TRAN_TABLE[index].state == SAR_TX_TRAN_ACTIVE
      && MESH_SAR_TX_TRAN_TABLE[index].seqZero == seqZero
      && (MESH_SAR_TX_TRAN_TABLE[index].nwkPduInfo.dst == remoteAddress
        || oboFlag))
    {
      if( blockAck > MESH_SAR_TX_TRAN_TABLE[index].blockAckMask)
      {
        if (oboFlag)
        {
          /* TBD if this triggers any local behavior. Probably not. */
        }

        /* Update block ACK. */
        MESH_SAR_TX_TRAN_TABLE[index].blockAckMask |= blockAck;

        /* Check if transaction complete. */
        if (MESH_SAR_TX_TRAN_TABLE[index].blockAckMask ==
            MESH_SAR_TX_BLOCK_MASK(MESH_SAR_TX_TRAN_TABLE[index].segCount))
        /* Transaction complete. */
        {
          meshSarTxEndTran(index);
        }
        else
        {
          /* Reset retransmission counter. */
          MESH_SAR_TX_TRAN_TABLE[index].retryCounter = MESH_SAR_TX_UNICAST_RETRANSMISSIONS;
          /* The timer will execute at some point and resend unAcked segments. */
        }

        /* No need to continue loop. */
        break;
      }
      /* If block ACK is set to 0, remote device cannot accept the transaction. Cancel it. */
      else if (blockAck == 0x00)
      {
        meshSarTxEndTran(index);
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Computes the required memory to be provided based on the given configuration.
 *
 *  \return Required memory in bytes or ::MESH_MEM_REQ_INVALID_CONFIG.
 */
/*************************************************************************************************/
uint32_t MeshSarTxGetRequiredMemory(void)
{
  if (pMeshConfig->pMemoryConfig->sarTxMaxTransactions == 0)
  {
    return MESH_MEM_REQ_INVALID_CFG;
  }

  return meshSarTxLocalGetRequiredMemory(pMeshConfig->pMemoryConfig->sarTxMaxTransactions);
}
