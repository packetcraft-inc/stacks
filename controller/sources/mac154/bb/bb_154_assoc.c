/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      802.15.4 baseband: Association.
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
#include "mac_154_int.h"
#include "chci_154_int.h"

#include "wsf_assert.h"
#include "wsf_buf.h"

#include <string.h>

/**************************************************************************************************
  Function Prototypes
**************************************************************************************************/

/**************************************************************************************************
  15.4 BB driver association callbacks
**************************************************************************************************/

/*
 * ED: N/A
 * Rx: bb154AssocRxCback
 * Tx: bb154AssocTxCback
 * Er: bb154AssocErrCback
 */

/*************************************************************************************************/
/*!
 *  \brief      Receive complete callback
 *
 *  \param      pRxFrame      Received frame
 *  \param      len           Received frame length
 *  \param      rssi          Received signal strength indicator.
 *  \param      timestamp     Timestamp of received frame
 *  \param      flags         Receive flags
 *
 *  \return     rxFlags       May go idle
 *
 *  \note       Must use BB_154_DRV_BUFFER_PTR on pRxBuf to get frame
 */
/*************************************************************************************************/
static uint8_t bb154AssocRxCback(uint8_t *pRxFrame, uint16_t len, int8_t rssi, uint32_t timestamp, uint8_t flags)
{
  BbOpDesc_t * const pOp = BbGetCurrentBod();
  Bb154Data_t * const p154 = pOp->prot.p154;
  uint8_t rxFlags = PAL_BB_154_RX_FLAG_GO_IDLE;
  bool_t rxFinished = FALSE;
  bool_t cleanup = FALSE;
  uint16_t fctl;
  uint8_t seq;
  bool_t canFreeTxDesc = TRUE; /* We generally can do this */
  Bb154Assoc_t * const pAssoc = &p154->op.assoc;

  /* Unused parameters */
  (void)rssi;
  (void)timestamp;

  /* Store pointer to rx buffer and length */
  pAssoc->pRxFrame = pRxFrame;
  pAssoc->rxLen = len;

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
      /* Not expecting a data frame */
      rxFinished = TRUE;
      if (MAC_154_FC_ACK_REQUEST(fctl) == 0)
      {
        /* Can only clean up if no ack. being sent */
        cleanup = TRUE;
      }
      break;

    case MAC_154_FRAME_TYPE_MAC_COMMAND:
      /* We would be expecting an assoc rsp. No processing until the ack. */
      /* Do nothing as we are short on time for tx'ing ack! */
      break;

    case MAC_154_FRAME_TYPE_ACKNOWLEDGMENT:
      if ((flags & PAL_BB_154_FLAG_RX_ACK_CMPL) && (pAssoc->pTxDesc != NULL))
      {
        Mac154Pib_t * const pPib = Mac154GetPIB();

        /* Check sequence number matches that in ack. frame */
        if (PAL_BB_154_TX_FRAME_PTR(pAssoc->pTxDesc)[2] == seq)
        {
          /* Check the original frame that solicited the ack. */
          switch (pAssoc->cmd)
          {
            case MAC_154_CMD_FRAME_TYPE_ASSOC_REQ:
              {
                uint32_t due;

                /* Assoc. req operation - next, Tx data request macResponseWaitTime after */

                /* Free existing buffer */
                WsfBufFree(pAssoc->pTxDesc);

                /* Build the poll */
                /* MAC command: Data request */
                pAssoc->cmd = MAC_154_CMD_FRAME_TYPE_DATA_REQ;

                /* Build data request, forcing source address to be extended */
                if ((pAssoc->pTxDesc = Bb154BuildDataReq(&((Bb154AssocReq_t *)pAssoc)->coordAddr, TRUE)) != NULL)
                {
                  due = PalBbGetCurrentTime(USE_RTC_BB_CLK) + PAL_BB_154_SYMB_TO_US(pPib->responseWaitTime);
                  PAL_BB_154_TX_FRAME_PTR(pAssoc->pTxDesc)[2] = Mac154GetDSNIncr();
                  PalBb154Tx(pAssoc->pTxDesc, 1, due, FALSE);
                  canFreeTxDesc = FALSE; /* No; pAssoc->pTxDesc is still active */
                  rxFlags &= ~PAL_BB_154_RX_FLAG_GO_IDLE; /* No, because we're transmitting next */
                  /* TODO: Error handling in case of alloc failure */
                }
              }
              break;

            case MAC_154_CMD_FRAME_TYPE_DATA_REQ:
              /* Restart Rx with macMaxFrameTotalWaitTime */
              PalBb154Rx(0, TRUE, PAL_BB_154_SYMB_TO_US(pPib->maxFrameTotalWaitTime));
              rxFlags &= ~PAL_BB_154_RX_FLAG_GO_IDLE; /* No, because we're transmitting next */
              break;

            case MAC_154_CMD_FRAME_TYPE_DISASSOC_NTF:
              cleanup = TRUE;
              pAssoc->status = MAC_154_ENUM_SUCCESS;
              break;

            default:
              /* Shouldn't really get here but play it safe if we do */
              cleanup = TRUE;
              pAssoc->status = MAC_154_ENUM_NO_DATA;
          }
        }
      }
      rxFinished = TRUE;  /* Don't need ack. any more */
      break;

    default:
      /* Not expecting any other frame type at this point */
      rxFinished = TRUE;
      cleanup = TRUE;
      break;
  }

  if ((canFreeTxDesc) && (pAssoc->pTxDesc != NULL))
  {
    /* Finished with tx buffer associated with this rx'ed ack. */
    WsfBufFree(pAssoc->pTxDesc);
    pAssoc->pTxDesc = NULL;
  }

  if (rxFinished)
  {
    /* Recycle received buffer */
    PalBb154ReclaimRxFrame(pAssoc->pRxFrame);

    /* No further use for received data; clear */
    pAssoc->pRxFrame = NULL;
    pAssoc->rxLen = 0;
  }

  if (cleanup)
  {
    /* Finish the operation */
    Bb154GenCleanupOp(pOp, p154);
  }

  return rxFlags;
}

/*************************************************************************************************/
/*!
 *  \brief      Association transmit complete callback
 *
 *  \param      flags Transmit flags
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154AssocTxCback(uint8_t flags)
{
  Mac154Pib_t * const pPib = Mac154GetPIB();
  BbOpDesc_t * const pOp = BbGetCurrentBod();
  Bb154Data_t * const p154 = pOp->prot.p154;
  Bb154Assoc_t * const pAssoc = &p154->op.assoc;

  if (flags & PAL_BB_154_FLAG_TX_ACK_CMPL)
  {
    /**** Ack. processing ****/

    /* Tx'ed frame (ack.) has ended */
    /* Completed transmitting an ack. frame when receiving a frame */
    /* Process any frame received but not yet processed */
    if (pAssoc->pRxFrame != NULL)
    {
      uint8_t *pRxFrame = pAssoc->pRxFrame;
      uint16_t fctl;
      Mac154Addr_t srcAddr;
      Mac154Addr_t dstAddr;

      /* Get frame control field and skip over sequence number */
      BSTREAM_TO_UINT16(fctl, pRxFrame);
      pRxFrame++;

      /* Get addresses */
      pRxFrame = Bb154GetAddrsFromFrame(pRxFrame, fctl, &srcAddr, &dstAddr);

      if (MAC_154_FC_FRAME_TYPE(fctl) == MAC_154_FRAME_TYPE_MAC_COMMAND)
      {
        uint8_t cmd;

        BSTREAM_TO_UINT8(cmd, pRxFrame);
        switch (cmd)
        {
          case MAC_154_CMD_FRAME_TYPE_ASSOC_RSP:
            {
              if (MAC_154_FC_LEGACY_SEC_TEST(fctl))
              {
                pAssoc->status = MAC_154_ENUM_UNSUPPORTED_LEGACY;
              }
              else
              {
                uint16_t shtAddr;
                /*
                 * Note we can't check coordinator address as it will always use extended address
                 * for the association response and it may have used short address in beacon
                 */
                BSTREAM_TO_UINT16(shtAddr, pRxFrame);
                BSTREAM_TO_UINT8(pAssoc->status, pRxFrame); /* Confirm status comes from response (SR [80,5]) */
                if (pAssoc->status == MAC_154_ASSOC_STATUS_SUCCESSFUL)
                {
                  pPib->shortAddr = shtAddr; /* SR [181,1] */
                  BYTES_TO_UINT64(pPib->coordExtAddr, srcAddr.addr); /* SR [181,4] */
                }
              }
              /* Setting PAN ID back to unassigned if not successful is handled in Chci154AssocSendAssocCfm() */
            }
            break;

          default:
            pAssoc->status = MAC_154_ENUM_NO_DATA;
            break;
        }
      }

      /* Recycle rx buffer associated with ack. */
      PalBb154ReclaimRxFrame(pAssoc->pRxFrame);

      /* Finish the operation */
      Bb154GenCleanupOp(pOp, p154);
    }
  }
  else if (flags & PAL_BB_154_FLAG_RX_ACK_START)
  {
    /**** Non ack. processing, ack. reqd. ****/

    /* Tx'ed frame (not ack.) with ack. requested has ended; rx ack. pending */
    /* Still need to hold onto frame until ack. rx'ed */
  }
  else
  {
    /**** Non ack. processing, no ack. reqd. ****/

    /* Tx'ed frame (not ack.) with no ack. requested has ended */
    /* No need to hold onto frame */
    if (pAssoc->pTxDesc != NULL)
    {
      /* Called if beacon sent in response to beacon request. */
      WsfBufFree(pAssoc->pTxDesc);
      pAssoc->pTxDesc = NULL;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Association error callback
 *
 *  \param      status  Error status
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154AssocErrCback(uint8_t status)
{
  BbOpDesc_t * const pOp = BbGetCurrentBod();
  Bb154Data_t * const p154 = pOp->prot.p154;
  Bb154Assoc_t * const pAssoc = &p154->op.assoc;

  switch (status)
  {
    case BB_STATUS_TX_FAILED:
    case BB_STATUS_TX_CCA_FAILED:
      pAssoc->status = MAC_154_ENUM_CHANNEL_ACCESS_FAILURE;
      break;

    case BB_STATUS_RX_TIMEOUT:
      pAssoc->status = MAC_154_ENUM_NO_DATA;
      break;

    case BB_STATUS_ACK_TIMEOUT:
      pAssoc->status = MAC_154_ENUM_NO_ACK;
      break;

    default:
      break;
  }
  /* Clean up whatever the reason */
  Bb154GenCleanupOp(pOp, p154);
}

/**************************************************************************************************
  BOD scheduler Execute callbacks via 15.4 BB driver
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Execute association request scan BOD.
 *
 *  \param      pOp    Pointer to the BOD to execute.
 *  \param      p154    802.15.4 operation parameters.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154AssocExecuteOp(BbOpDesc_t *pOp, Bb154Data_t *p154)
{
  Bb154Assoc_t * const pAssoc = &p154->op.assoc;

  /* Unused parameters */
  (void)pOp;

  p154->opParam.txCback  = bb154AssocTxCback;
  p154->opParam.rxCback  = bb154AssocRxCback;
  p154->opParam.errCback = bb154AssocErrCback;

  /* Build receive buffer queue */
  PalBb154BuildRxBufQueue(p154->opParam.psduMaxLength, BB_154_ASSOC_MIN_RX_BUF_CNT);

  PalBb154SetChannelParam(&p154->chan);
  PalBb154SetOpParams(&p154->opParam);

  if (pAssoc->pTxDesc != NULL)
  {
    PAL_BB_154_TX_FRAME_PTR(pAssoc->pTxDesc)[2] = Mac154GetDSNIncr();
    PalBb154Tx(pAssoc->pTxDesc, 1, 0, TRUE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize for associate request operations.
 *
 *  \return     None.
 *
 *  Update the operation table with associate request operations
 */
/*************************************************************************************************/
void Bb154AssocInit(void)
{
  bb154RegisterOp(BB_154_OP_ASSOC, bb154AssocExecuteOp);
}
