/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      802.15.4 baseband: Data.
 *
 *  Copyright (c) 2016-2018 Arm Ltd.
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
 */
/*************************************************************************************************/

#include "wsf_types.h"
#include "util/bstream.h"
#include "pal_bb.h"
#include "bb_api.h"
#include "bb_154_int.h"
#include "bb_154.h"
#include "bb_154_api_op.h"
#include "chci_154_int.h"
#include "mac_154_int.h"
#include "mac_154_defs.h"

#include "wsf_assert.h"
#include "wsf_buf.h"
#include "wsf_timer.h"

#include <string.h>

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief    BB assert enable. */
#ifndef BB_ASSERT_ENABLED
#define BB_ASSERT_ENABLED  TRUE
#endif

/*! \brief    BB assert. */
#if BB_ASSERT_ENABLED == TRUE
#if WSF_TOKEN_ENABLED == TRUE
#define BB_ASSERT(expr)      if (!(expr)) {WsfAssert(MODULE_ID, (uint16_t) __LINE__);}
#else
#define BB_ASSERT(expr)      if (!(expr)) {WsfAssert(__FILE__, (uint16_t) __LINE__);}
#endif
#else
#define BB_ASSERT(expr)
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief    Indirect queue entry. */
typedef struct bb154TxIndEntry
{
  struct bb154TxIndEntry *pNext;    /*!< Pointer to next entry in queue */
  PalBb154TxBufDesc_t    *pTxDesc;  /*!< Pointer to buffer to be queued. */
  wsfTimer_t             timer;     /*!< Transaction timer. */
} bb154TxIndEntry_t;

/*! \brief    Indirect queue. */
typedef struct bb154TxIndQueue
{
  wsfQueue_t entryQ;      /*!< Entry queue. */
  uint8_t    entryCount;  /*!< Number of entries. */
} bb154TxIndQueue_t;

/*! \brief    BB data control block. */
typedef struct bb154DataCtrlBlk
{
  /*!< Tx indirect queue. */
  struct
  {
    bb154TxIndQueue_t free;   /*!< Free buffer queue. */
    bb154TxIndQueue_t used;   /*!< Used buffer queue */
  } txIndirect;
} bb154DataCtrlBlk_t;

/*! \brief     Obtain the address of the bb154TxIndEntry_t from timer message.
 *
 * The message "param" element is used to hold the negative offset from the
 * address of the message itself to point to the enclosing bb154TxIndEntry_t
 * structure.
 */
#define TX_IND_ENTRY_FROM_MSG(pMsg) (bb154TxIndEntry_t *)((uint8_t *)(pMsg) - \
                                                          ((wsfMsgHdr_t *)(pMsg))->param)

/**************************************************************************************************
  Function Prototypes
**************************************************************************************************/

/**************************************************************************************************
  Data
**************************************************************************************************/

static bb154DataCtrlBlk_t bb154DataCb;

/**************************************************************************************************
  Subroutines
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      General transmit error handling. Used by poll and data tx.
 *
 *  \param      status        Incoming error status
 *  \param      pOpStatus     Status to set in the operation (passed by ref.)
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154GenTxErrCback(uint8_t status, uint8_t *pOpStatus)
{
  BbOpDesc_t * const pOp = BbGetCurrentBod();
  bool_t cleanup = TRUE;

  switch (status)
  {
    case BB_STATUS_TX_FAILED:
      /* TODO */
      cleanup = FALSE;
      break;

    case BB_STATUS_ACK_TIMEOUT:
      /* Ack timed out. after retries */
      *pOpStatus = MAC_154_ENUM_NO_ACK;
      break;

    case BB_STATUS_RX_TIMEOUT:
      /* General receive timeout */
      *pOpStatus = MAC_154_ENUM_NO_DATA;
      break;

    case BB_STATUS_TX_CCA_FAILED:
      *pOpStatus = MAC_154_ENUM_CHANNEL_ACCESS_FAILURE;
      break;

    default:
      break;
  }

  if (cleanup)
  {
    Bb154GenCleanupOp(pOp, pOp->prot.p154);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Get and dequeue a tx indirect packet.
 *
 *  \param      pAddr     Pointer to address to match
 *  \param      bufCnt    Total number of buffers remaining in queue (passed by reference)
 *
 *  \return     Pointer to dequeued buffer (the actual data).
 *
 *  Calling this routine will dequeue a Tx indirect frame from the Tx indirect queue
 */
/*************************************************************************************************/
static inline PalBb154TxBufDesc_t *bb154GetTxIndirectBuf(Mac154Addr_t *pAddr, uint8_t *pMatches)
{
  bb154TxIndEntry_t *pPrev = NULL;
  bb154TxIndEntry_t *pElem;
  bb154TxIndEntry_t *pFirstPrev = NULL;
  bb154TxIndEntry_t *pFirstElem = NULL;
  uint8_t matches = 0;

  for (pElem = (bb154TxIndEntry_t *)bb154DataCb.txIndirect.used.entryQ.pHead;
      pElem != NULL;
      pElem = pElem->pNext)
  {
    uint8_t *pTxFrame = PAL_BB_154_TX_FRAME_PTR(pElem->pTxDesc);
    uint8_t acc = 0x00;
    uint16_t fctl;
    Mac154Addr_t dstAddr;

    /* Get frame control and skip over sequence number fields */
    BSTREAM_TO_UINT16(fctl, pTxFrame);
    pTxFrame++;

    /* Get addresses */
    pTxFrame = Bb154GetAddrsFromFrame(pTxFrame, fctl, NULL, &dstAddr);

    /* Use bit ops for comparison */
    if (((pAddr->panId[0] ^ dstAddr.panId[0]) |
        (pAddr->panId[1] ^ dstAddr.panId[1]) |
        (pAddr->addrMode ^ dstAddr.addrMode)) == 0)
    {
      static const uint8_t amLenLUT[4] = {0,0,2,8};
      for (unsigned int i = 0; i < amLenLUT[pAddr->addrMode]; i++)
      {
        acc |= (pAddr->addr[i] ^ dstAddr.addr[i]);
      }
    }

    if (acc == 0x00)
    {
      matches++; /* Increment number of matches */
      if (pFirstElem == NULL)
      {
        /* Mark first occurrence */
        pFirstPrev = pPrev;
        pFirstElem = pElem;
      }
    }
    /* Set previous pointer to this one */
    pPrev = pElem;
  }

  *pMatches = matches;

  if (pFirstElem != NULL)
  {
    /* Remove from used queue */
    --bb154DataCb.txIndirect.used.entryCount;
    WsfQueueRemove(&bb154DataCb.txIndirect.used.entryQ, pFirstElem, pFirstPrev);

    /* Stop transaction persistence timer */
    WsfTimerStop(&pFirstElem->timer);

    /* Add back to free entry queue */
    WsfQueueEnq(&bb154DataCb.txIndirect.free.entryQ, pFirstElem);
    bb154DataCb.txIndirect.free.entryCount++;

    return pFirstElem->pTxDesc;
  }

  return NULL;
}

/**************************************************************************************************
  15.4 BB driver data transmit callbacks
**************************************************************************************************/

/*
 * Tx: bb154DataTxTxCback
 * Rx: bb154DataTxRxCback
 * Er: bb154DataTxErrCback
 */

/*************************************************************************************************/
/*!
 *  \brief      Data transmit receive complete callback
 *
 *  \param      pRxBuf        Receive buffer
 *  \param      len           Received frame length
 *  \param      rssi          Received signal strength indicator
 *  \param      timestamp     Timestamp of received frame
 *  \param      flags         Receive flags
 *
 *  \return     rxflags - can always go idle.
 *
 *  \note       Must use BB_154_DRV_BUFFER_PTR on pRxBuf to get frame
 */
/*************************************************************************************************/
static uint8_t bb154DataTxRxCback(uint8_t *pRxFrame, uint16_t len, int8_t rssi, uint32_t timestamp, uint8_t flags)
{
  BbOpDesc_t * const pOp = BbGetCurrentBod();
  Bb154Data_t * const p154 = pOp->prot.p154;
  Bb154DataTx_t * const pDataTx = &p154->op.dataTx;

  /* Unused parameters */
  (void)len;
  (void)rssi;
  (void)timestamp;

  /* Check it's an ack. and that sequence number matches that in Tx frame */
  if ((flags & PAL_BB_154_FLAG_RX_ACK_CMPL) &&
      (PAL_BB_154_TX_FRAME_PTR(pDataTx->pTxDesc)[2] == pRxFrame[2]))
  {
    /* Tx with ack. is complete, cleanup BOD */
    pDataTx->status = MAC_154_ENUM_SUCCESS;
  }
  else
  {
    /* Tx with ack. is complete, cleanup BOD */
    pDataTx->status = MAC_154_ENUM_NO_ACK;
  }
  pDataTx->timestamp = 0; /* TODO */

  /* Reclaim frame buffer */
  PalBb154ReclaimRxFrame(pRxFrame);
  Bb154GenCleanupOp(pOp, p154);

  /* If not ack. and no matching sequence number, then wait until another ack. or ack. timeout */
  return PAL_BB_154_RX_FLAG_GO_IDLE;
}

/*************************************************************************************************/
/*!
 *  \brief      Data transmit transmit complete callback
 *
 *  \param      flags Transmit flags
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154DataTxTxCback(uint8_t flags)
{
  BbOpDesc_t * const pOp = BbGetCurrentBod();
  Bb154Data_t * const p154 = pOp->prot.p154;

  if ((flags & PAL_BB_154_FLAG_RX_ACK_START) == 0)
  {
    /* Completed transmitting a non-ack. frame that was not expecting an ack. */
    /* If not expecting an ack., transmit is done, cleanup BOD */
    p154->op.dataTx.status = MAC_154_ENUM_SUCCESS;
    Bb154GenCleanupOp(pOp, p154);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Data transmit error callback
 *
 *  \param      flags Transmit flags
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154DataTxErrCback(uint8_t status)
{
  BbOpDesc_t * const pOp = BbGetCurrentBod();
  Bb154Data_t * const p154 = pOp->prot.p154;

  bb154GenTxErrCback(status, &p154->op.dataTx.status);
}

/**************************************************************************************************
  15.4 BB driver data receive callbacks
**************************************************************************************************/

/*
 * Note this is the most likely operation running for an 802.15.4 coordinator running ZigBee
 * or Thread, where Rx on when idle is TRUE.
 *
 * Tx: bb154DataRxTxCback
 * FP: bb154DataRxFPCback
 * Rx: bb154DataRxRxCback
 * Er: bb154DataRxErrCback
 */

/*************************************************************************************************/
/*!
 *  \brief      Frame pending check callback
 *
 *  \param      srcAddr       Source address
 *
 *  \return     bool_t        TRUE if there is at least one stored frame matching source address
 */
/*************************************************************************************************/
static bool_t bb154DataRxFPCback(uint8_t srcAddrMode, uint64_t srcAddr)
{
  if (bb154DataCb.txIndirect.used.entryCount)
  {
    bb154TxIndEntry_t *pElem;

    for (pElem = (bb154TxIndEntry_t *)bb154DataCb.txIndirect.used.entryQ.pHead;
        pElem != NULL;
        pElem = pElem->pNext)
    {
      uint8_t *pTxFrame = PAL_BB_154_TX_FRAME_PTR(pElem->pTxDesc);
      uint16_t fctl;

      /* Get frame control */
      BSTREAM_TO_UINT16(fctl, pTxFrame);
      pTxFrame += 3; /* Skip over sequence number and destination PAN ID */
      /* Get destination address. Note PAN ID matching not necessary - assume hardware filters correctly */
      switch (MAC_154_FC_DST_ADDR_MODE(fctl))
      {
        case MAC_154_ADDR_MODE_SHORT:
          {
            uint16_t dstAddr;
            BYTES_TO_UINT16(dstAddr, pTxFrame);
            if (srcAddr == (uint64_t)dstAddr)
            {
              return TRUE;
            }
          }
          break;

        case MAC_154_ADDR_MODE_EXTENDED:
          {
            uint64_t dstAddr;
            BYTES_TO_UINT64(dstAddr, pTxFrame);
            if (srcAddr == dstAddr)
            {
              return TRUE;
            }
          }
          break;

        case MAC_154_ADDR_MODE_NONE:
          /* Special case for frames to PAN coordinator */
          if (srcAddrMode == MAC_154_ADDR_MODE_NONE)
          {
            return TRUE;
          }

        default:
          /* Go to next in queue */
          break;
      }
    }
  }
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief      Data receive receive complete callback
 *
 *  \param      pRxFrane      Pointer to received frame
 *  \param      len           Received frame length
 *  \param      rssi          Received signal strength indicator
 *  \param      timestamp     Timestamp of received frame
 *  \param      flags         Receive flags
 *
 *  \return     rxFlags       Always go idle
 */
/*************************************************************************************************/
static uint8_t bb154DataRxRxCback(uint8_t *pRxFrame, uint16_t len, int8_t rssi, uint32_t timestamp, uint8_t flags)
{
  Mac154Pib_t * const pPib = Mac154GetPIB();
  BbOpDesc_t * const pOp = BbGetCurrentBod();
  Bb154Data_t * const p154 = pOp->prot.p154;
  Bb154DataRx_t * const pDataRx = &p154->op.dataRx;
  uint16_t fctl;
  uint8_t seq;
  uint8_t *pRxFrameStart = pRxFrame;
  bool_t rxFinished = FALSE;
  uint8_t rxFlags = PAL_BB_154_RX_FLAG_GO_IDLE;

  /* Store pointer to original rx buffer and length */
  pDataRx->pRxFrame = pRxFrame;
  pDataRx->rxLen = len;

  if (pPib->vsRawRx)
  {
    Mac154ExecuteRawFrameCback(len, pRxFrame, PalBb154RssiToLqi(rssi), timestamp);
    /* Done with the rx frame. */
    rxFinished = TRUE;
  }
  else if (pPib->promiscuousMode)
  {
    static const Mac154Addr_t zeroAddr = {0, {0}, {0}};
    /* Send as a raw data indication */
    Chci154DataRxSendInd(&zeroAddr, &zeroAddr, PalBb154RssiToLqi(rssi), 0, timestamp, len, pRxFrame);
    /* Done with the rx frame. */
    rxFinished = TRUE;
  }
  else
  {
    /* Get frame control and sequence number fields */
    BSTREAM_TO_UINT16(fctl, pRxFrame);
    BSTREAM_TO_UINT8(seq, pRxFrame);

    /*
     * Any frames which:
     * a) Don't require subsequent Tx
     * b) Are not soliciting an ack.
     * can be handled here.
     * Any frames soliciting an ack. must be processed in bb154DataRxTxCback()
     */
    switch (MAC_154_FC_FRAME_TYPE(fctl))
    {
      case MAC_154_FRAME_TYPE_DATA:
        {
          Mac154Addr_t srcAddr;
          Mac154Addr_t dstAddr;
          /* Get addresses */
          pRxFrame = Bb154GetAddrsFromFrame(pRxFrame, fctl, &srcAddr, &dstAddr);

          /* Check for legacy security */
          if (MAC_154_FC_LEGACY_SEC_TEST(fctl))
          {
            Chci154DataSendCommStatusInd(&srcAddr, &dstAddr, MAC_154_ENUM_UNSUPPORTED_LEGACY);
          }
          else
          {
            /* Send as a data indication */
            Chci154DataRxSendInd(&srcAddr, &dstAddr, PalBb154RssiToLqi(rssi), seq, timestamp, len - (pRxFrame - pRxFrameStart), pRxFrame);
          }
          /* Done with the rx frame. */
          rxFinished = TRUE;
        }
        break;

      case MAC_154_FRAME_TYPE_MAC_COMMAND:
        {
          uint8_t *pPayload = PalBb154GetPayloadPtr(pRxFrameStart, fctl);
          if (pPayload != NULL)
          {
            switch (*pPayload)
            {
              case MAC_154_CMD_FRAME_TYPE_BEACON_REQ:
                if (pPib->deviceType != MAC_154_DEV_TYPE_DEVICE)
                {
                  /* Broadcast - not soliciting an ack. */
                  pDataRx->pTxDesc = Bb154BuildBeacon();
                  PalBb154Off(); /* Cancel any Rx if in progress */
                  PAL_BB_154_TX_FRAME_PTR(pDataRx->pTxDesc)[2] = Mac154GetBSNIncr();
                  PalBb154Tx(pDataRx->pTxDesc, 1, 0, TRUE);
                }
                /* Done with the rx frame. */
                rxFinished = TRUE;
                break;

              case MAC_154_CMD_FRAME_TYPE_ORPHAN_NTF:
                {
                  Mac154Addr_t srcAddr;

                  /* Get source address */
                  (void)Bb154GetAddrsFromFrame(pRxFrame, fctl, &srcAddr, NULL);
                  Chci154AssocSendOrphanInd(srcAddr.addr);
                  /* Done with the rx frame. */
                  rxFinished = TRUE;
                }
                break;

              case MAC_154_CMD_FRAME_TYPE_ASSOC_REQ:
              case MAC_154_CMD_FRAME_TYPE_DATA_REQ:
              case MAC_154_CMD_FRAME_TYPE_DISASSOC_NTF:
                /* These are all processed after the ack. has been transmitted so hold onto the rx frame */
                break;

              default:
                /* Free the rx frame in any other case */
                rxFinished = TRUE;
                break;
            }
          }
        }
        break;

      case MAC_154_FRAME_TYPE_BEACON:
        {
          Mac154Addr_t srcAddr;
          uint16_t ss;
          uint8_t sduLen;

          /* Get source address */
          pRxFrame = Bb154GetAddrsFromFrame(pRxFrame, fctl, &srcAddr, NULL);
          /* Get Superframe specification */
          BSTREAM_TO_UINT16(ss, pRxFrame);
          /* TODO: Skip over GTS specification and pending address specification */
          pRxFrame += 2;

          sduLen = len - (pRxFrame - pRxFrameStart);
          if ((sduLen > 0) || (pPib->autoRequest == 0))
          {
            /* Send beacon notify */
            Mac154PanDescr_t panDescr;

            /* Structure copy address over */
            panDescr.coord = srcAddr;
            panDescr.logicalChan = p154->chan.channel;
            UINT16_TO_BUF(panDescr.superframeSpec, ss);
            panDescr.gtsPermit = FALSE; /* TODO: Assume no GTS */
            panDescr.linkQuality = PalBb154RssiToLqi(rssi);
            UINT24_TO_BUF(panDescr.timestamp, timestamp);
            /* TODO: Security of incoming frame */
            if (MAC_154_FC_LEGACY_SEC_TEST(fctl))
            {
              panDescr.securityFailure = MAC_154_ENUM_UNSUPPORTED_LEGACY;
            }
            else
            {
              panDescr.securityFailure = MAC_154_ENUM_SUCCESS;
            }

            /* Send beacon notify */
            Chci154ScanSendBeaconNotifyInd(seq, &panDescr, sduLen, pRxFrame);
          }
          rxFinished = TRUE;
        }
        break;

      case MAC_154_FRAME_TYPE_ACKNOWLEDGMENT:
        {
          if ((flags & PAL_BB_154_FLAG_RX_ACK_CMPL) &&
              (pDataRx->pTxDesc != NULL))
          {
            uint8_t *pTxFrame = PAL_BB_154_TX_FRAME_PTR(pDataRx->pTxDesc);

            /* Check sequence number matches that in ack. frame */
            if (pTxFrame[2] == seq)
            {
              Mac154HandleTxComplete(PAL_BB_154_TX_FRAME_PTR(pDataRx->pTxDesc), pDataRx->pTxDesc->handle, MAC_154_ENUM_SUCCESS);
            }

            /* Finished with tx buffer associated with this rx'ed ack. */
            WsfBufFree(pDataRx->pTxDesc);
            pDataRx->pTxDesc = NULL;

          }
          rxFinished = TRUE; /* Don't need ack. any more */
        }
        break;

      default:
        /* Free the rx frame in any other case */
        rxFinished = TRUE;
        break;
    } /* END switch (MAC_154_FC_FRAME_TYPE(fctl)) */
  }

  if (rxFinished)
  {
    /* No further use for received data; clear */
    pDataRx->pRxFrame = NULL;
    pDataRx->rxLen = 0;

    /* Reclaim frame buffer */
    PalBb154ReclaimRxFrame(pRxFrameStart);
  }

  return rxFlags;
}

/*************************************************************************************************/
/*!
 *  \brief      Data receive transmit complete callback
 *
 *  \param      flags Transmit flags
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154DataRxTxCback(uint8_t flags)
{
  BbOpDesc_t * const pOp = BbGetCurrentBod();
  Bb154Data_t * const p154 = pOp->prot.p154;
  Bb154DataRx_t * const pDataRx = &p154->op.dataRx;

  if (flags & PAL_BB_154_FLAG_TX_ACK_CMPL)
  {
    /**** Ack. processing ****/

    /* Tx'ed frame (ack.) has ended */
    /* Completed transmitting an ack. frame when receiving a frame */
    /* Process any frame received but not yet processed */
    if (pDataRx->pRxFrame != NULL)
    {
      uint8_t *pRxFrame = pDataRx->pRxFrame;
      uint16_t fctl;
      Mac154Addr_t srcAddr;

      /* Get frame control field and skip over sequence number */
      BSTREAM_TO_UINT16(fctl, pRxFrame);
      pRxFrame++;

      /* Get source addresses */
      pRxFrame = Bb154GetAddrsFromFrame(pRxFrame, fctl, &srcAddr, NULL);

      if (MAC_154_FC_FRAME_TYPE(fctl) == MAC_154_FRAME_TYPE_MAC_COMMAND)
      {
        switch (*pRxFrame)
        {
          case MAC_154_CMD_FRAME_TYPE_ASSOC_REQ:
            Chci154AssocSendAssocInd(srcAddr.addr, pRxFrame[1]);
            break;

          case MAC_154_CMD_FRAME_TYPE_DATA_REQ:
            if (bb154DataCb.txIndirect.used.entryCount)
            {
              uint8_t matches;

              /* Get source address */
              pDataRx->pTxDesc = bb154GetTxIndirectBuf(&srcAddr, &matches);
              if (pDataRx->pTxDesc != NULL)
              {
                uint8_t *pTxFrame = PAL_BB_154_TX_FRAME_PTR(pDataRx->pTxDesc);
                if (matches > 1)
                {
                  /* Indicate more frames pending */
                  *pTxFrame |= MAC_154_FC_FRAME_PENDING_MASK;
                }
                PalBb154Off(); /* Cancel any Rx if in progress */
                pTxFrame[2] = Mac154GetDSNIncr();
                PalBb154Tx(pDataRx->pTxDesc, 1, 0, TRUE);

                /* Indicate to higher layer */
                Chci154DataSendPollInd(&srcAddr, (uint8_t)(pDataRx->pTxDesc != NULL));
              }
            }
            break;

          case MAC_154_CMD_FRAME_TYPE_DISASSOC_NTF:
            Chci154AssocSendDisassocInd(srcAddr.addr, pRxFrame[1]);
            break;

         /*
          * TODO:
          * MAC_154_CMD_FRAME_TYPE_ASSOC_RSP
          * MAC_154_CMD_FRAME_TYPE_PANID_CNFL_NTF
          * MAC_154_CMD_FRAME_TYPE_COORD_REALIGN
          */

          /*
           * These are not expected at all:
           * MAC_154_CMD_FRAME_TYPE_GTS_REQ
           */
        default:
          break;
        } /* END switch (*pRxFrame) */
      } /* END if (MAC_154_FC_FRAME... */

      /* Recycle rx buffer associated with ack. */
      PalBb154ReclaimRxFrame(pDataRx->pRxFrame);

    } /* END if (pDataRx... */
  } /* END if (flags & BB_154_DRV */

  else if (flags & PAL_BB_154_FLAG_RX_ACK_START)
  {
    /**** Non ack. processing, ack. reqd. ****/

    /* Tx'ed frame (not ack.) with ack. requested has ended; rx ack. pending */
    /* Still need to hold onto frame until ack. rx'ed */
    if (pDataRx->pTxDesc != NULL)
    {

    }
  }
  else
  {
    /**** Non ack. processing, no ack. reqd. ****/

    /* Tx'ed frame (not ack.) with no ack. requested has ended */
    /* No need to hold onto frame */
    if (pDataRx->pTxDesc != NULL)
    {
      Mac154HandleTxComplete(PAL_BB_154_TX_FRAME_PTR(pDataRx->pTxDesc), pDataRx->pTxDesc->handle, MAC_154_ENUM_SUCCESS);
      /* Called if beacon sent in response to beacon request. */
      WsfBufFree(pDataRx->pTxDesc);
      pDataRx->pTxDesc = NULL;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Data receive error callback
 *
 *  \param      status  Error status
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154DataRxErrCback(uint8_t status)
{
  Bb154DataRx_t * const pDataRx = &BbGetCurrentBod()->prot.p154->op.dataRx;
  uint8_t macStatus = 0xFF;

  switch (status)
  {
    case BB_STATUS_RX_TIMEOUT:
      /* Cannot happen */
      break;

    case BB_STATUS_TX_CCA_FAILED:
      macStatus = MAC_154_ENUM_CHANNEL_ACCESS_FAILURE;
      break;
    case BB_STATUS_TX_FAILED:
      /* Note this failure is peculiar to Cordio h/w */
      macStatus = MAC_154_ENUM_TRANSACTION_OVERFLOW;
      break;
    case BB_STATUS_ACK_TIMEOUT:
      macStatus = MAC_154_ENUM_NO_ACK;
      break;

    default:
      break;
  }

  if ((macStatus != 0xFF) && (pDataRx->pTxDesc != NULL))
  {
    Mac154HandleTxComplete(PAL_BB_154_TX_FRAME_PTR(pDataRx->pTxDesc), pDataRx->pTxDesc->handle, macStatus);
    WsfBufFree(pDataRx->pTxDesc);
    pDataRx->pTxDesc = NULL;
  }
}

/**************************************************************************************************
  15.4 BB driver poll callbacks
**************************************************************************************************/

/*
 * Tx: bb154DataPollTxCback
 * Rx: bb154DataPollRxCback
 * Er: bb154DataPollErrCback
 */

/*************************************************************************************************/
/*!
 *  \brief      Data poll transmit complete callback
 *
 *  \param      flags Transmit flags
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154DataPollTxCback(uint8_t flags)
{
  BbOpDesc_t * const pOp = BbGetCurrentBod();
  Bb154Data_t * const p154 = pOp->prot.p154;

  /* This will get called twice in a poll:
   * 1) After the data request is sent. We are expecting an ack. in this case
   *    so no need to do anything until ack. received
   * 2) After an ack. is sent in response to data frame requesting it. In this
   *    case, we need to clean up.
   */
  if (flags & PAL_BB_154_FLAG_TX_ACK_CMPL)
  {
    /* Completed transmitting an ack. frame in response to data frame requesting it. */
    p154->op.poll.status = MAC_154_ENUM_SUCCESS;
    Bb154GenCleanupOp(pOp, p154);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Data poll receive complete callback
 *
 *  \param      pRxBuf        Receive buffer
 *  \param      len           Received frame length
 *  \param      rssi          Received signal strength indicator
 *  \param      timestamp     Timestamp of received frame
 *  \param      flags         Receive flags
 *
 *  \return     rxFlags       Always go idle and may indicate frame pending.
 *
 *  \note       Must use BB_154_DRV_BUFFER_PTR on pRxBuf to get frame
 */
/*************************************************************************************************/
static uint8_t bb154DataPollRxCback(uint8_t *pRxFrame, uint16_t len, int8_t rssi, uint32_t timestamp, uint8_t flags)
{
  /* TODO: Compare with bb_154_assoc.c:bb154AssocRxCback. It is likely these can be made common */

  Mac154Pib_t * const pPib = Mac154GetPIB();
  BbOpDesc_t * const pOp = BbGetCurrentBod();
  Bb154Data_t * const p154 = pOp->prot.p154;
  uint8_t *pRxFrameStart = pRxFrame;
  uint8_t rxFlags = PAL_BB_154_RX_FLAG_GO_IDLE;
  bool_t cleanup = FALSE;
  uint16_t fctl;
  uint8_t seq;
  volatile uint32_t currentTime;
  bool_t rxFinished = FALSE;
  bool_t canFreeTxDesc = TRUE; /* We generally can do this */
  Bb154Poll_t * const pPoll = &p154->op.poll;

  /* Calculated elapsed time */
  currentTime = PalBbGetCurrentTime(USE_RTC_BB_CLK);
  /* Reset snapshot */
  pPoll->snapshot = currentTime;

  /* Get frame control and sequence number */
  BSTREAM_TO_UINT16(fctl, pRxFrame);
  BSTREAM_TO_UINT8(seq, pRxFrame);

  /*
   * Any frames which:
   * a) Don't require subsequent Tx
   * b) Are not soliciting an ack.
   * can be handled here.
   * Any frames soliciting an ack. must be processed in bb154DataRxTxCback()
   */
  switch (MAC_154_FC_FRAME_TYPE(fctl))
  {
    case MAC_154_FRAME_TYPE_DATA:
      /* Save buffer pointer and length for subsequent MCPS-DATA.ind */
      pPoll->pRxFrame = pRxFrameStart;
      pPoll->rxLen = len;
      pPoll->timestamp = timestamp;
      pPoll->linkQuality = PalBb154RssiToLqi(rssi);
      /* rxFinished when MCPS-DATA.ind has been sent */
      if (MAC_154_FC_ACK_REQUEST(fctl) == 0)
      {
        /* Can only clean up if no ack. being sent */
        cleanup = TRUE;
      }
      break;

    case MAC_154_FRAME_TYPE_ACKNOWLEDGMENT:
      if ((flags & PAL_BB_154_FLAG_RX_ACK_CMPL) && (pPoll->pTxDesc != NULL))
      {
        /* Look for frame pending. If set, we need to stay on to receive the frame */
        /* Check sequence number matches that in Tx frame */
        if (PAL_BB_154_TX_FRAME_PTR(pPoll->pTxDesc)[2] == seq)
        {
          if (MAC_154_FC_FRAME_PENDING(fctl))
          {
            /* Restart Rx with macMaxFrameTotalWaitTime */
            PalBb154Rx(0, TRUE, PAL_BB_154_SYMB_TO_US(pPib->maxFrameTotalWaitTime));
            rxFlags &= ~PAL_BB_154_RX_FLAG_GO_IDLE;  /* No, because we're receiving next */
          }
          else
          {
            /* No pending data, cleanup BOD */
            pPoll->status = MAC_154_ENUM_NO_DATA;
            cleanup = TRUE;
          }
          rxFinished = TRUE;
        }
      }
      break;

    case MAC_154_FRAME_TYPE_MAC_COMMAND:
    default:
      /* Command frame or anything else treated as NO_DATA (SR [133,24])  */
      pPoll->status = MAC_154_ENUM_NO_DATA;
      rxFinished = TRUE;
      cleanup = TRUE;
      break;
  }

  if ((canFreeTxDesc) && (pPoll->pTxDesc != NULL))
  {
    /* Finished with tx buffer associated with this rx'ed ack. */
    WsfBufFree(pPoll->pTxDesc);
    pPoll->pTxDesc = NULL;
  }

  if (rxFinished)
  {
    /* Reclaim frame buffer */
    PalBb154ReclaimRxFrame(pRxFrameStart);
  }

  if (cleanup)
  {
    /* Finish the operation */
    Bb154GenCleanupOp(pOp, p154);
  }

  /* If not ack. and no matching sequence number, then wait until another ack. or ack. timeout */
  return rxFlags;
}

/*************************************************************************************************/
/*!
 *  \brief      Data transmit error callback
 *
 *  \param      status  Error status
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154DataPollErrCback(uint8_t status)
{
  BbOpDesc_t * const pOp = BbGetCurrentBod();
  Bb154Data_t * const p154 = pOp->prot.p154;

  bb154GenTxErrCback(status, &p154->op.poll.status);
}

/**************************************************************************************************
  BOD scheduler Execute callbacks via 15.4 BB driver
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Execute data transmit BOD.
 *
 *  \param      pOp    Pointer to the BOD to execute.
 *  \param      p154    802.15.4 operation parameters.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154DataTxExecuteOp(BbOpDesc_t *pOp, Bb154Data_t *p154)
{
  Bb154DataTx_t * const pDataTx = &p154->op.dataTx;

  /* Unused parameters */
  (void)pOp;

  p154->opParam.txCback  = bb154DataTxTxCback;
  p154->opParam.rxCback  = bb154DataTxRxCback;
  p154->opParam.errCback = bb154DataTxErrCback;

  /* Build receive buffer queue for ack. */
  PalBb154BuildRxBufQueue(p154->opParam.psduMaxLength, BB_154_DATA_TX_MIN_RX_BUF_CNT);

  PalBb154SetChannelParam(&p154->chan);
  PalBb154SetOpParams(&p154->opParam);

  if ((p154->opParam.flags & PAL_BB_154_FLAG_RAW) == 0)
  {
    PAL_BB_154_TX_FRAME_PTR(pDataTx->pTxDesc)[2] = Mac154GetDSNIncr();
  }
  PalBb154Tx(pDataTx->pTxDesc, 1, 0, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief      Execute data receive BOD.
 *
 *  \param      pOp     Pointer to the BOD to execute.
 *  \param      p154    802.15.4 operation parameters.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154DataRxExecuteOp(BbOpDesc_t *pOp, Bb154Data_t *p154)
{
  /* Unused parameters */
  (void)pOp;

  if (Mac154IsRxEnabled())
  {
    p154->opParam.txCback  = bb154DataRxTxCback;
    p154->opParam.fpCback  = bb154DataRxFPCback;
    p154->opParam.rxCback  = bb154DataRxRxCback;
    p154->opParam.errCback = bb154DataRxErrCback;

    /* Start baseband now (note different to other BbOps) */
    BbStart(BB_PROT_15P4);

    /* Build receive buffer queue */
    PalBb154BuildRxBufQueue(p154->opParam.psduMaxLength, BB_154_DATA_RX_MIN_RX_BUF_CNT);

    PalBb154SetChannelParam(&p154->chan);
    PalBb154SetOpParams(&p154->opParam);

    PalBb154Rx(pOp->due, TRUE, 0);
  }
  else
  {
    BbSetBodTerminateFlag();
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Execute data poll BOD.
 *
 *  \param      pOp     Pointer to the BOD to execute.
 *  \param      p154    802.15.4 operation parameters.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154DataPollExecuteOp(BbOpDesc_t *pOp, Bb154Data_t *p154)
{
  Bb154Poll_t * const pPoll = &p154->op.poll;

  /* Unused parameters */
  (void)pOp;

  p154->opParam.txCback  = bb154DataPollTxCback;
  p154->opParam.rxCback  = bb154DataPollRxCback;
  p154->opParam.errCback = bb154DataPollErrCback;

  /* Build receive buffer queue for ack. */
  PalBb154BuildRxBufQueue(p154->opParam.psduMaxLength, BB_154_DATA_TX_MIN_RX_BUF_CNT);

  PalBb154SetChannelParam(&p154->chan);
  PalBb154SetOpParams(&p154->opParam);

  PAL_BB_154_TX_FRAME_PTR(pPoll->pTxDesc)[2] = Mac154GetDSNIncr();
  PalBb154Tx(pPoll->pTxDesc, 1, 0, TRUE);
}

/**************************************************************************************************
  Indirect data handling
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Queue the Tx indirect packet.
 *
 *  \param      pTxDesc   Transmit descriptor.
 *
 *  \return     Total number of buffers queued.
 *
 *  Calling this routine will queue a Tx indirect frame to the Tx indirect queue
 */
/*************************************************************************************************/
uint8_t Bb154QueueTxIndirectBuf(PalBb154TxBufDesc_t *pTxDesc)
{
  if (pTxDesc != NULL)
  {
    bb154TxIndEntry_t *pTxIndEntry;

    /* Take a free entry */
    if ((pTxIndEntry = (bb154TxIndEntry_t *)WsfQueueDeq(&bb154DataCb.txIndirect.free.entryQ)) != NULL)
    {
      bb154DataCb.txIndirect.free.entryCount--;
      pTxIndEntry->pTxDesc = pTxDesc;
      WsfQueueEnq(&bb154DataCb.txIndirect.used.entryQ, pTxIndEntry);
      bb154DataCb.txIndirect.used.entryCount++;
      /* Store offset of timer message within bb154TxIndEntry_t for retrieval in the timeout callback */
      pTxIndEntry->timer.msg.param = (uint16_t)((uint8_t *)&pTxIndEntry->timer.msg - (uint8_t *)pTxIndEntry);
      /* Start transaction persistence timer */
      Mac154StartTransactionPersistenceTimer(&pTxIndEntry->timer);
    }
  }
  return bb154DataCb.txIndirect.used.entryCount;
}

/*************************************************************************************************/
/*!
 *  \brief      Purge a tx indirect packet.
 *
 *  \param      msduHandle    MSDU handle to purge
 *  \param      bufCnt        Total number of buffers remaining in queue (passed by reference)
 *
 *  \return     TRUE if purged, FALSE otherwise.
 *
 *  Calling this routine will purge a Tx indirect frame from the Tx indirect queue
 */
/*************************************************************************************************/
bool_t Bb154PurgeTxIndirectBuf(uint8_t msduHandle)
{
  bb154TxIndEntry_t *pPrev = NULL;
  bb154TxIndEntry_t *pElem;

  for (pElem = (bb154TxIndEntry_t *)bb154DataCb.txIndirect.used.entryQ.pHead;
      pElem != NULL;
      pElem = pElem->pNext)
  {
    if (pElem->pTxDesc->handle == msduHandle)
    {
      /* Remove from used queue */
      bb154DataCb.txIndirect.used.entryCount--;
      WsfQueueRemove(&bb154DataCb.txIndirect.used.entryQ, pElem, pPrev);

      /* Stop transaction persistence timer */
      WsfTimerStop(&pElem->timer);

      /* Add back to free entry queue */
      WsfQueueEnq(&bb154DataCb.txIndirect.free.entryQ, pElem);
      bb154DataCb.txIndirect.free.entryCount++;

      return TRUE;
    }
    /* Set previous pointer to this one */
    pPrev = pElem;
  }
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief      Handle transaction persistence timer timeout.
 *
 *  \param      pMsg    Pointer to the associated timer message
 *
 *  \return     Pointer to dequeued buffer (the actual data).
 *
 *  Calling this routine will dequeue a Tx indirect frame from the Tx indirect queue
 *
 *  \note       The timer message address has a fixed relationship to the structure it is
 *              enclosed in as it is never copied by virtue of using linked lists. Therefore
 *              the address of the enclosing structure can always be recovered using simple
 *              pointer arithmetic, although structure arithmetic can't be used due to padding
 *              rules.
 */
/*************************************************************************************************/
void Bb154HandleTPTTimeout(void *pMsg)
{
  /* Note structure arithmetic cannot be used easily due to padding */
  bb154TxIndEntry_t * const pTxIndEntry = TX_IND_ENTRY_FROM_MSG(pMsg);
  bb154TxIndEntry_t *pPrev = NULL;
  bb154TxIndEntry_t *pElem;

  for (pElem = (bb154TxIndEntry_t *)bb154DataCb.txIndirect.used.entryQ.pHead;
      pElem != NULL;
      pElem = pElem->pNext)
  {
    if (pElem == pTxIndEntry)
    {
      /* Remove from used queue */
      bb154DataCb.txIndirect.used.entryCount--;
      WsfQueueRemove(&bb154DataCb.txIndirect.used.entryQ, pElem, pPrev);

      /* Handle expiry */
      Mac154HandleTxComplete(PAL_BB_154_TX_FRAME_PTR(pElem->pTxDesc), pElem->pTxDesc->handle, MAC_154_ENUM_TRANSACTION_EXPIRED);

      /* Add back to free entry queue */
      WsfQueueEnq(&bb154DataCb.txIndirect.free.entryQ, pElem);
      bb154DataCb.txIndirect.free.entryCount++;
      return;
    }
  }
  /* Something wrong if we get here */
  BB_ASSERT(0);
}

/**************************************************************************************************
  Initialization
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Initialize for data baseband operations.
 *
 *  \return     None.
 *
 *  Update the operation table with data transmit operations
 */
/*************************************************************************************************/
void Bb154DataInit(void)
{
  int i;

  bb154RegisterOp(BB_154_OP_DATA_TX, bb154DataTxExecuteOp);
  bb154RegisterOp(BB_154_OP_DATA_RX, bb154DataRxExecuteOp);
  bb154RegisterOp(BB_154_OP_DATA_POLL, bb154DataPollExecuteOp);
  memset(&bb154DataCb, 0, sizeof(bb154DataCb));

  /* Load up free Tx indirect buffers */
  for (i = 0; i < BB_154_DATA_IND_BUF_CNT; i++)
  {
    bb154TxIndEntry_t *pTxIndEntry = WsfBufAlloc(sizeof(bb154TxIndEntry_t));
    if (pTxIndEntry != NULL)
    {
      WsfQueueEnq(&bb154DataCb.txIndirect.free.entryQ, pTxIndEntry);
      bb154DataCb.txIndirect.free.entryCount++;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief      De-initialize for data baseband operations.
 *
 *  \return     None.
 *
 *  Update the operation table with data transmit operations
 */
/*************************************************************************************************/
void Bb154DataDeInit(void)
{
  /* Clear out and free any queued buffers in used queue */
  while (bb154DataCb.txIndirect.used.entryQ.pHead != NULL)
  {
    WsfBufFree(WsfQueueDeq(&bb154DataCb.txIndirect.used.entryQ));
  }
  bb154DataCb.txIndirect.used.entryCount = 0;

  /* Clear out and free any queued buffers in free queue */
  while (bb154DataCb.txIndirect.free.entryQ.pHead != NULL)
  {
    WsfBufFree(WsfQueueDeq(&bb154DataCb.txIndirect.free.entryQ));
  }
  bb154DataCb.txIndirect.free.entryCount = 0;
}
