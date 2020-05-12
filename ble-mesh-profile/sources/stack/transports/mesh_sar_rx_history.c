/*************************************************************************************************/
/*!
 *  \file   mesh_sar_rx_history.c
 *
 *  \brief  SAR Rx history module.
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
#include "wsf_assert.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"
#include "mesh_main.h"
#include "mesh_utils.h"

#include "mesh_sar_rx_history.h"

/**************************************************************************************************
  Macros
 **************************************************************************************************/

/*! SEQ Interval for one single SAR transaction */
#define SAR_RX_SEQ_INTEVAL      MESH_SEQ_ZERO_MASK

/*! Mask of least significant 3 bits of the IVI */
#define SAR_RX_IVI_MASK         0x00000007U

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! SAR Rx Transaction History Entry */
typedef struct meshSarRxHistoryEntry_tag
{
  void          *pNext;   /*!< Pointer to next entry */
  uint32_t      seqNo;    /*!< Sequence number of the first fragment of the Upper Transport PDU.
                           *   It is used to extract seqZero.
                           */
  meshAddress_t srcAddr;  /*!< Address of the element originating the message. */
  uint8_t       iviLsb;   /*!< Least significant byte of the 32-bit IV index value */
  uint8_t       segNo;    /*!< Number of segments expected for a completed transaction. If set
                           *   to 0, then the transaction wasn't completed. It is used
                           *   to send late acks for completed transactions.
                           */
  bool_t        obo;      /*!< OBO flag. */
} meshSarRxHistoryEntry_t;

/*! SAR Rx Transaction History */
typedef struct meshSarRxHistory_tag
{
  wsfQueue_t              usedHistQueue;  /*!< Transaction history queue of allocate entries */
  wsfQueue_t              freeHistQueue;  /*!< Transaction history queue of used entries */
  meshSarRxHistoryEntry_t *pTranHistory;  /*!< Transaction history memory pointer */
} meshSarRxHistory_t;

/*! SAR Rx Transaction History control block type */
typedef struct sarRxHistoryCb_tag
{
  meshSarRxHistory_t  history;      /*!< Transaction history */
  uint8_t             historySize;  /*!< Transaction history maximum size */
} sarRxHistoryCb_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Mesh SAR Rx Transaction History control block */
static sarRxHistoryCb_t sarRxHistoryCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Computes memory requirements based on configured size of SAR Rx Transaction History.
 *
 *  \param[in] historySize  SAR Rx Transaction History size.
 *
 *  \return    Required memory in bytes for SAR Rx Transaction History.
 */
/*************************************************************************************************/
static inline uint32_t meshSarRxHistoryGetRequiredMemory(uint8_t historySize)
{
  /* Compute required memory size for SAR Rx Transaction History */
  return  MESH_UTILS_ALIGN(sizeof(meshSarRxHistoryEntry_t) * historySize);
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Computes the required memory to be provided based on the given configuration.
 *
 *  \return Required memory in bytes.
 */
/*************************************************************************************************/
uint32_t MeshSarRxHistoryGetRequiredMemory(void)
{
  uint32_t reqMem;

  /* Compute the required memory. */
  reqMem = meshSarRxHistoryGetRequiredMemory(pMeshConfig->pMemoryConfig->sarRxTranHistorySize);

  return reqMem;
}

/*************************************************************************************************/
/*!
 *  \brief  Initializes the SAR Rx Transaction History table and allocates memory.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshSarRxHistoryInit(void)
{
  uint8_t *pMemBuff;
  uint32_t reqMem;

  pMemBuff = meshCb.pMemBuff;

  /* Save the pointer for the SAR Rx Transaction history */
  sarRxHistoryCb.history.pTranHistory = (meshSarRxHistoryEntry_t *)pMemBuff;

  /* Increment the memory buffer pointer. */
  reqMem = meshSarRxHistoryGetRequiredMemory(pMeshConfig->pMemoryConfig->sarRxTranHistorySize);
  pMemBuff += reqMem;

  /* Save the updated address. */
  meshCb.pMemBuff = pMemBuff;

  /* Subtract the reserved size from memory buffer size. */
  meshCb.memBuffSize -= reqMem;

  /* Set history size. */
  sarRxHistoryCb.historySize = pMeshConfig->pMemoryConfig->sarRxTranHistorySize;

  /* Reset SAR Rx Transaction history internals. */
  MeshSarRxHistoryReset();
}

/*************************************************************************************************/
/*!
 *  \brief     Adds the SAR Rx transaction parameters in the SAR Rx Transaction History table.
 *
 *  \param[in] srcAddr  Source address.
 *  \param[in] seqNo    Sequence number that is used on SAR TX for seqZero computation.
 *  \param[in] iviLsb   Least significant 2 bits of the IV Index for the received packet.
 *  \param[in] segN     SegN value for the transaction.
 *  \param[in] obo      TRUE if transaction was realized on behalf of a LPN node.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSarRxHistoryAdd(meshAddress_t srcAddr, uint32_t seqNo, uint8_t iviLsb, uint8_t segN,
                         bool_t obo)
{
  meshSarRxHistoryEntry_t *pEntry;

  /* Check if no empty entries left. */
  if(WsfQueueEmpty(&(sarRxHistoryCb.history.freeHistQueue)))
  {
    /* Get oldest from the used queue. */
    pEntry = (meshSarRxHistoryEntry_t *)WsfQueueDeq(&(sarRxHistoryCb.history.usedHistQueue));
  }
  else
  {
    /* Get one from the free queue. */
    pEntry = (meshSarRxHistoryEntry_t *)WsfQueueDeq(&(sarRxHistoryCb.history.freeHistQueue));
  }

  if (pEntry != NULL)
  {
    /* A new SAR Rx transaction has ended - add it to the queue. */
    pEntry->srcAddr  = srcAddr;
    pEntry->seqNo = seqNo;
    pEntry->segNo = segN + 1;
    pEntry->iviLsb = iviLsb;
    pEntry->obo = obo;

    /* Enqueue in the used queue. */
    WsfQueueEnq(&(sarRxHistoryCb.history.usedHistQueue),pEntry);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Checks if the SAR Rx Transaction History table contains a transaction that matches
 *              the parameters.
 *
 *  \param[in]  srcAddr      Source address.
 *  \param[in]  seqNo        Sequence number.
 *  \param[in]  seqZero      Seq Zero value.
 *  \param[in]  iviLsb       Least significant 2 bits of the IV Index for the received packet.
 *  \param[in]  segN         SegN value for the transaction.
 *  \param[out] pOutSendAck  Set to TRUE if we need to send ACK. Valid only for FALSE return value.
 *  \param[out] pOutObo      Set to TRUE if OBO flag should be set, FALSE otherwise.
 *
 *  \return     TRUE if transaction was not found, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshSarRxHistoryCheck(meshAddress_t srcAddr, uint32_t seqNo, uint16_t seqZero,
                             uint8_t iviLsb, uint8_t segN, bool_t *pOutSendAck, bool_t *pOutObo)
{
  meshSarRxHistoryEntry_t *pEntry;

  /* By default, we do not send any Ack */
  *pOutSendAck = FALSE;
  *pOutObo = FALSE;

  /* Check if history is empty. */
  if(WsfQueueEmpty(&(sarRxHistoryCb.history.usedHistQueue)))
  {
    return TRUE;
  }

  /* Point to start of the queue. */
  pEntry = (meshSarRxHistoryEntry_t *)(&(sarRxHistoryCb.history.usedHistQueue))->pHead;

  /* Search from head to tail to always check the latest entries first. */
  while (pEntry != NULL)
  {
    /* Find the same source address */
    if (pEntry->srcAddr == srcAddr)
    {
      if (iviLsb == pEntry->iviLsb)
      {
        /* Drop if it is a lower seqAuth = lower SEQ for same IVI */
        if (seqNo < pEntry->seqNo)
        {
          return FALSE;
        }

        /* Check if a segment belongs to a completed or canceled transaction. */
        if ((SAR_RX_SEQZERO(pEntry->seqNo) == seqZero) &&
            ((pEntry->seqNo + SAR_RX_SEQ_INTEVAL) > seqNo))
        {
          /* For a completed transaction (same SeqAuth) send it again. Maybe the last ACK
           * was missed by the remote element.
           */
          if ((segN + 1) == pEntry->segNo)
          {
            *pOutSendAck = TRUE;
            *pOutObo = pEntry->obo;
          }

          return FALSE;
        }
      }

      if (((iviLsb + 1) & 0x3) == pEntry->iviLsb)
      {
        return FALSE;
      }
    }
    /* Move to next entry */
    pEntry = (meshSarRxHistoryEntry_t *)(pEntry->pNext);
  }

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief  Resets the SAR Rx Transaction History table.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshSarRxHistoryReset(void)
{
  uint8_t idx;

  /* Reset memory */
  memset(sarRxHistoryCb.history.pTranHistory, 0,
         sizeof(meshSarRxHistoryEntry_t) * sarRxHistoryCb.historySize);

  /* Initialize queues. */
  WSF_QUEUE_INIT(&(sarRxHistoryCb.history.freeHistQueue));
  WSF_QUEUE_INIT(&(sarRxHistoryCb.history.usedHistQueue));

  /* Add all entries to the free queue. */
  for(idx = 0; idx < sarRxHistoryCb.historySize; idx++)
  {
    WsfQueueEnq(&(sarRxHistoryCb.history.freeHistQueue),
                &(sarRxHistoryCb.history.pTranHistory[idx]));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Clears History entries for source address with older SeqAuth.
 *
 *  \param[in] srcAddr  Source address.
 *  \param[in] seqZero  Seq Zero value.
 *  \param[in] iviLsb   Least significant 2 bits of the IV Index for the received packet.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSarRxHistoryCleanupOld(meshAddress_t srcAddr, uint16_t seqZero, uint8_t iviLsb)
{
  meshSarRxHistoryEntry_t *pEntry, *pPrev, *pNext;
  uint16_t count;
  bool_t entryRemoved = FALSE;

  if(WsfQueueEmpty(&(sarRxHistoryCb.history.usedHistQueue)))
  {
    return;
  }

  /* Set previous to NULL. */
  pPrev = NULL;
  /* Point to start of the queue. */
  pEntry = (meshSarRxHistoryEntry_t *)(&(sarRxHistoryCb.history.usedHistQueue))->pHead;
  /* Get queue count. */
  count = WsfQueueCount((&(sarRxHistoryCb.history.usedHistQueue)));

  /* Find all entries which have an IVI value lower than IVI - 1 */
  while(count > 0)
  {
    entryRemoved = FALSE;

    pNext = (meshSarRxHistoryEntry_t *)(pEntry->pNext);
    if (pEntry->srcAddr == srcAddr)
    {
      if((pEntry->iviLsb < iviLsb) ||
         ((pEntry->iviLsb == iviLsb) && (SAR_RX_SEQZERO(pEntry->seqNo) < seqZero)))
      {
        /* Invalidate history entry by setting address to 0 */
        pEntry->srcAddr = 0;

        /* Remove from used queue */
        WsfQueueRemove(&(sarRxHistoryCb.history.usedHistQueue), pEntry, pPrev);

        /* Add to free queue. */
        WsfQueueEnq(&(sarRxHistoryCb.history.freeHistQueue), pEntry);

        entryRemoved = TRUE;
      }
    }

    if (entryRemoved == FALSE)
    {
      pPrev = pEntry;
    }

    pEntry = pNext;
    count--;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Clears entries with lower IV values than new IV index.
 *
 *  \param[in] newIvIndex  New value for the IV index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshSarRxHistoryIviCleanup(uint32_t newIvIndex)
{
  meshSarRxHistoryEntry_t *pEntry, *pPrev, *pNext;
  uint16_t count;
  bool_t entryRemoved = FALSE;

  if(WsfQueueEmpty(&(sarRxHistoryCb.history.usedHistQueue)))
  {
    return;
  }

  /* Set previous to NULL. */
  pPrev = NULL;
  /* Point to start of the queue. */
  pEntry = (meshSarRxHistoryEntry_t *)(&(sarRxHistoryCb.history.usedHistQueue))->pHead;
  /* Get queue count. */
  count = WsfQueueCount((&(sarRxHistoryCb.history.usedHistQueue)));

  /* Find all entries which have an IVI value lower than IVI - 1 */
  while(count > 0)
  {
    entryRemoved = FALSE;

    pNext = (meshSarRxHistoryEntry_t *)(pEntry->pNext);
    if (pEntry->iviLsb + 1 < (uint8_t)(newIvIndex & SAR_RX_IVI_MASK))
    {
      /* Invalidate history entry by setting address to 0 */
      pEntry->srcAddr = 0;

      /* Remove from used queue */
      WsfQueueRemove(&(sarRxHistoryCb.history.usedHistQueue), pEntry, pPrev);

      /* Add to free queue. */
      WsfQueueEnq(&(sarRxHistoryCb.history.freeHistQueue), pEntry);

      entryRemoved = TRUE;
    }

    if (entryRemoved == FALSE)
    {
      pPrev = pEntry;
    }

    pEntry = pNext;
    count--;
  }
}
