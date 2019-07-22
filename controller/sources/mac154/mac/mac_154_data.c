/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      802.15.4 MAC: Data.
 *
 *  Copyright (c) 2013-2018 Arm Ltd.
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
#include "wsf_buf.h"
#include "util/bstream.h"
#include "pal_bb.h"
#include "bb_154_int.h"
#include "bb_154.h"
#include "chci_154_int.h"
#include "mac_154_int.h"
#include "mac_154_defs.h"
#include "sch_api.h"
#include <string.h>

/**************************************************************************************************
  Macros
**************************************************************************************************/

#define ADDITIONAL_TESTER_FEATURES

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/**************************************************************************************************
  Function Prototypes
**************************************************************************************************/

/**************************************************************************************************
  Data
**************************************************************************************************/

/* TODO: Control block needed? Multiple instances? */

/**************************************************************************************************
  Subroutines
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Handle transmit complete.
 *
 *  \param      pTxFrame      Pointer to frame transmitted/failed
 *  \param      handle        Associated handle (if any).
 *  \param      status        Associated status.
 *
 *  \return     None.
 *
 *  Determines the correct indication/response to send based on packet transmitted/failed.
 */
/*************************************************************************************************/
bool_t Mac154HandleTxComplete(uint8_t *pTxFrame, uint8_t handle, uint8_t status)
{
  uint16_t fctl;
  Mac154Addr_t srcAddr;
  Mac154Addr_t dstAddr;
  bool_t bcastCoordRealign = FALSE;

  /* Get frame control and skip over sequence number fields */
  BSTREAM_TO_UINT16(fctl, pTxFrame);
  pTxFrame++;

  /* Get addresses */
  pTxFrame = Bb154GetAddrsFromFrame(pTxFrame, fctl, &srcAddr, &dstAddr);

  /* Figure out what SAP primitive to send based on dropped frame */
  switch (MAC_154_FC_FRAME_TYPE(fctl))
  {
    case MAC_154_FRAME_TYPE_DATA:
      /* Send a MCPS-DATA.cnf */
      Chci154DataTxSendCfm(handle, status, 0);
      break;

    case MAC_154_FRAME_TYPE_MAC_COMMAND:
      switch (*pTxFrame)
      {
        case MAC_154_CMD_FRAME_TYPE_ASSOC_RSP:
          Chci154DataSendCommStatusInd(&srcAddr, &dstAddr, status);
          break;

        case MAC_154_CMD_FRAME_TYPE_COORD_REALIGN:
          if ((dstAddr.addrMode == MAC_154_ADDR_MODE_SHORT) &&
              (status == MAC_154_ENUM_SUCCESS))
          {
            bcastCoordRealign =  TRUE;
          }
          else
          {
            /* Coordinator realignment due to orphan response */
            Chci154DataSendCommStatusInd(&srcAddr, &dstAddr, status);
          }

        case MAC_154_CMD_FRAME_TYPE_DISASSOC_NTF:
          /* TODO */
          break;

        /* No other MAC command frames can be sent indirect */
        default:
          break;
      }
      break;

#ifdef ADDITIONAL_TESTER_FEATURES
    /* This is used for testing only */
    case MAC_154_FRAME_TYPE_ILLEGAL4:
      if (Mac154GetPIB()->vsFctlOverride)
      {
        /* Send a MCPS-DATA.cnf */
        Chci154DataTxSendCfm(handle, status, 0);
      }
      break;
#endif

    /* Beacon and ack. frames cannot be indirect */
    default:
      break;
  }
  return bcastCoordRealign;
}

/**************************************************************************************************
  BOD cleanup
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Cleanup BOD at end of data transmit
 *
 *  \param      pOp     Pointer to the BOD to cleanup.
 *
 *  \return     None.
 *
 *  \note       Called from scheduler context, not ISR.
 */
/*************************************************************************************************/
static void mac154DataTxEndCback(BbOpDesc_t *pOp)
{
  Bb154Data_t * const p154 = pOp->prot.p154;
  Bb154DataTx_t * const pDataTx = &p154->op.dataTx;

  BbStop(BB_PROT_15P4);

  if (pDataTx->pTxDesc != NULL)
  {
    if (Mac154HandleTxComplete(PAL_BB_154_TX_FRAME_PTR(pDataTx->pTxDesc), pDataTx->pTxDesc->handle, pDataTx->status))
    {
      Mac154Pib_t * const pPib = Mac154GetPIB();
      Mac154PhyPib_t * const pPhyPib = Mac154GetPhyPIB();
      Bb154Start_t * const pStart = &p154->op.start;

      /* Was a successfully broadcast coord realignment sent due to start. Set PIB attributes */
      pPib->panId = pStart->panId;
      pPib->deviceType = pStart->panCoord ? MAC_154_DEV_TYPE_PAN_COORD : MAC_154_DEV_TYPE_COORD;
      pPhyPib->chan = pStart->logChan;
      pPhyPib->txPower = pStart->txPower;
    }
    WsfBufFree(pDataTx->pTxDesc);
  }

  WsfBufFree(p154);
  WsfBufFree(pOp);
}

/*************************************************************************************************/
/*!
 *  \brief      Cleanup BOD at end of data poll
 *
 *  \param      pOp     Pointer to the BOD to cleanup.
 *
 *  \return     None.
 *
 *  \note       Called from scheduler context, not ISR.
 */
/*************************************************************************************************/
static void mac154DataPollEndCback(BbOpDesc_t *pOp)
{
  Bb154Data_t * const p154 = pOp->prot.p154;
  Bb154Poll_t * const pPoll = &p154->op.poll;

  BbStop(BB_PROT_15P4);

  if (pPoll->pRxFrame != NULL)
  {
    uint8_t *pRxFrame = pPoll->pRxFrame;
    uint8_t *pRxFrameStart = pRxFrame;
    Mac154Addr_t srcAddr;
    Mac154Addr_t dstAddr;
    uint16_t fctl;
    uint8_t seq;
    uint8_t msduLength;

    /* Get frame control and sequence number */
    BSTREAM_TO_UINT16(fctl, pRxFrame);
    BSTREAM_TO_UINT8(seq, pRxFrame);

    /* Get addresses */
    pRxFrame = Bb154GetAddrsFromFrame(pRxFrame, fctl, &srcAddr, &dstAddr);

    /* Get MSDU length */
    msduLength = (uint8_t)pPoll->rxLen - (pRxFrame - pRxFrameStart);

    if (msduLength > 0)
    {
      Chci154DataSendPollCfm(pPoll->status);
      Chci154DataRxSendInd(&srcAddr, &dstAddr, pPoll->linkQuality, seq, pPoll->timestamp, msduLength, pRxFrame);
    }
    else
    {
      /* Don't send MCPS-DATA.ind and indicate NO_DATA (SR [133,24]) */
      Chci154DataSendPollCfm(MAC_154_ENUM_NO_DATA);
    }
    /* Recycle received buffer */
    PalBb154ReclaimRxFrame(pPoll->pRxFrame);
  }
  else
  {
    Chci154DataSendPollCfm(pPoll->status);
  }

  if (pPoll->pTxDesc != NULL)
  {
    WsfBufFree(pPoll->pTxDesc);
  }
  WsfBufFree(p154);
  WsfBufFree(pOp);
}

/*************************************************************************************************/
/*!
 *  \brief      Cleanup BOD at end of receive
 *
 *  \param      pOp     Pointer to the BOD to cleanup.
 *
 *  \return     None.
 *
 *  \note       Called from scheduler context, not ISR.
 */
/*************************************************************************************************/
static void mac154DataRxEndCback(BbOpDesc_t *pOp)
{
  if (Mac154IsRxEnabled())
  {
    SchInsertNextAvailable(pOp);
  }
  else
  {
    /* Stop 15.4 baseband operation */
    BbStop(BB_PROT_15P4);
    WsfBufFree(pOp->prot.p154);
    WsfBufFree(pOp);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Start data transmit operation
 *
 *  \param      p154          15.4 operation to (re)start
 *
 *  \return     Result code.
 */
/*************************************************************************************************/
static uint8_t mac154DataTxStartOp(void *pPendingTxHandle)
{
  BbOpDesc_t *pOp;
  Bb154Data_t * const p154 = (Bb154Data_t *)pPendingTxHandle;
  Mac154PhyPib_t * const pPhyPib = Mac154GetPhyPIB();

  /* Allocate storage for data transmit BOD */
  if ((pOp = WsfBufAlloc(sizeof(BbOpDesc_t))) == NULL)
  {
    WsfBufFree(p154);
    return MAC_154_ERROR;
  }
  memset(pOp, 0, sizeof(BbOpDesc_t));
  pOp->prot.p154 = p154;

  /* Initialize data BOD protocol */
  pOp->reschPolicy = BB_RESCH_MOVEABLE_PREFERRED;
  pOp->protId = BB_PROT_15P4;
  pOp->endCback = mac154DataTxEndCback;
  pOp->abortCback = mac154DataTxEndCback;

  /* Set the 802.15.4 operation type */
  p154->opType = BB_154_OP_DATA_TX;

  /* Set overall MAC state */
  Mac154SetState(MAC_154_STATE_TX);

  /* Claim baseband for 15.4 use */
  BbStart(BB_PROT_15P4);

  /* Initiate Tx Data */
  p154->chan.channel = pPhyPib->chan;
  p154->chan.txPower = pPhyPib->txPower;
  p154->op.dataTx.snapshot = PalBbGetCurrentTime(USE_RTC_BB_CLK);
  p154->op.dataTx.status = MAC_154_ENUM_TRANSACTION_OVERFLOW;/* Default status if aborted early */
  SchInsertNextAvailable(pOp);

  return MAC_154_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \brief      Start data poll operation
 *
 *  \param      p154          15.4 operation to (re)start
 *
 *  \return     Result code.
 */
/*************************************************************************************************/
static uint8_t mac154DataPollStartOp(void *pPendingTxHandle)
{
  BbOpDesc_t *pOp;
  Bb154Data_t * const p154 = (Bb154Data_t *)pPendingTxHandle;
  Mac154PhyPib_t * const pPhyPib = Mac154GetPhyPIB();

  /* Allocate storage for data transmit BOD */
  if ((pOp = WsfBufAlloc(sizeof(BbOpDesc_t))) == NULL)
  {
    WsfBufFree(p154);
    return MAC_154_ERROR;
  }
  memset(pOp, 0, sizeof(BbOpDesc_t));
  pOp->prot.p154 = p154;

  /* Initialize data BOD protocol */
  pOp->reschPolicy = BB_RESCH_MOVEABLE_PREFERRED;
  pOp->protId = BB_PROT_15P4;
  pOp->endCback = mac154DataPollEndCback;
  pOp->abortCback = mac154DataPollEndCback;

  /* Set the 802.15.4 operation type */
  p154->opType = BB_154_OP_DATA_POLL;

  /* Set overall MAC state */
  Mac154SetState(MAC_154_STATE_POLL);

  /* Claim baseband for 15.4 use */
  BbStart(BB_PROT_15P4);

  /* Initiate Poll Data */
  p154->chan.channel = pPhyPib->chan;
  p154->chan.txPower = pPhyPib->txPower;
  p154->op.poll.snapshot = PalBbGetCurrentTime(USE_RTC_BB_CLK);
  p154->op.poll.timestamp = 0;
  p154->op.poll.status = MAC_154_ENUM_TRANSACTION_OVERFLOW;/* Default status if aborted early */
  p154->op.poll.pRxFrame = NULL;
  SchInsertNextAvailable(pOp);

  return MAC_154_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \brief      Start data transmit.
 *
 *  \param      srcAddrMode   Source address mode
 *  \param      pDstAddr      Pointer to destination address
 *  \param      msduHandle    Handle (used for purge)
 *  \param      txOptions     Bitmap of transmit options
 *  \param      timestamp     Timestamp when to send it (Zigbee GP support)
 *  \param      msduLen       Length of MSDU
 *  \param      pMsdu         Pointer to MSDU
 *
 *  \return     Result code.
 */
/*************************************************************************************************/
uint8_t Mac154DataTxStart(uint8_t srcAddrMode,
                          Mac154Addr_t *pDstAddr,
                          uint8_t msduHandle,
                          uint8_t txOptions,
                          uint32_t timestamp,
                          uint8_t msduLen,
                          uint8_t *pMsdu)
{
  Bb154Data_t *p154;
  PalBb154TxBufDesc_t *pTxDesc;

  if (txOptions & MAC_154_MCPS_TX_OPT_INDIRECT)
  {
    if ((pTxDesc = Bb154BuildDataFrame(PHY_154_aMaxPHYPacketSize, srcAddrMode, pDstAddr, txOptions, msduLen, pMsdu)) == NULL)
    {
      return MAC_154_ERROR;
    }
    pTxDesc->handle = msduHandle;
    Bb154QueueTxIndirectBuf(pTxDesc);
    /* Transmission will be handled in Data Rx */
  }
  else
  {
    /* Allocate storage for data transmit BOD's 15.4 specific data */
    if ((p154 = WsfBufAlloc(sizeof(Bb154Data_t))) == NULL)
    {
      return MAC_154_ERROR;
    }
    memset(p154, 0, sizeof(Bb154Data_t));

    /* Set 802.15.4 operational parameters */
    p154->opParam.flags = PAL_BB_154_FLAG_TX_RX_AUTO_ACK;
    if (txOptions & MAC_154_MCPS_TX_OPT_VS_DISABLE_CCA)
    {
      p154->opParam.flags |= PAL_BB_154_FLAG_DIS_CCA;
    }
    p154->opParam.psduMaxLength = PHY_154_aMaxPHYPacketSize;

    if ((p154->op.dataTx.pTxDesc = Bb154BuildDataFrame(PHY_154_aMaxPHYPacketSize, srcAddrMode, pDstAddr, txOptions, msduLen, pMsdu)) == NULL)
    {
      WsfBufFree(p154);
      return MAC_154_ERROR;
    }

    /* Initialize remainder of operation data (TxFrame already done) */
    p154->op.dataTx.pTxDesc->handle = msduHandle;
    p154->op.dataTx.timestamp = timestamp;

    /* Start the baseband operation */
    return mac154DataTxStartOp(p154);
  }
  return MAC_154_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \brief      Start raw frame transmit.
 *
 *  \param      disableCCA    Disable CCA on transmission
 *  \param      msduLen       Length of MSDU
 *  \param      pMsdu         Pointer to MSDU
 *
 *  \return     Result code.
 */
/*************************************************************************************************/
uint8_t Mac154RawFrameTxStart(bool_t disableCCA, uint8_t mpduLen, uint8_t *pMpdu)
{
  Bb154Data_t *p154;

  /* Allocate storage for data transmit BOD's 15.4 specific data */
  if ((p154 = WsfBufAlloc(sizeof(Bb154Data_t))) == NULL)
  {
    return MAC_154_ERROR;
  }
  memset(p154, 0, sizeof(Bb154Data_t));

  /* Set 802.15.4 operational parameters */
  p154->opParam.flags = PAL_BB_154_FLAG_TX_RX_AUTO_ACK | PAL_BB_154_FLAG_RAW;
  if (disableCCA)
  {
    p154->opParam.flags |= PAL_BB_154_FLAG_DIS_CCA;
  }
  p154->opParam.psduMaxLength = PHY_154_aMaxPHYPacketSize;

  if ((p154->op.dataTx.pTxDesc = Bb154BuildRawFrame(PHY_154_aMaxPHYPacketSize, mpduLen, pMpdu)) == NULL)
  {
    WsfBufFree(p154);
    return MAC_154_ERROR;
  }

  p154->op.dataTx.timestamp = 0;

  /* Start the baseband operation */
  return mac154DataTxStartOp(p154);
}

/*************************************************************************************************/
/*!
 *  \brief      Start data poll.
 *
 *  \param      pCoordAddr    Pointer to coordinator address
 *
 *  \return     Result code.
 */
/*************************************************************************************************/
uint8_t Mac154DataPollStart(Mac154Addr_t *pCoordAddr)
{
  Bb154Data_t *p154;

  /* Allocate storage for data transmit BOD's 15.4 specific data */
  if ((p154 = WsfBufAlloc(sizeof(Bb154Data_t))) == NULL)
  {
    return MAC_154_ERROR;
  }
  memset(p154, 0, sizeof(Bb154Data_t));

  /* Set 802.15.4 operational parameters */
  p154->opParam.flags = PAL_BB_154_FLAG_TX_RX_AUTO_ACK;
  p154->opParam.psduMaxLength = PHY_154_aMaxPHYPacketSize;

  if ((p154->op.poll.pTxDesc = Bb154BuildDataReq(pCoordAddr, FALSE)) == NULL)
  {
    WsfBufFree(p154);
    return MAC_154_ERROR;
  }

  /* Start the baseband operation */
  return mac154DataPollStartOp(p154);
}

/*************************************************************************************************/
/*!
 *  \brief      Start data receive.
 *
 *  \param      None.
 *
 *  \return     Result code.
 */
/*************************************************************************************************/
uint8_t Mac154DataRxStart(void)
{
  Mac154PhyPib_t * const pPhyPib = Mac154GetPhyPIB();
  Mac154Pib_t * const pPib = Mac154GetPIB();
  BbOpDesc_t *pOp;
  Bb154Data_t *p154;

  /* Allocate storage for data receive BOD */
  if ((pOp = WsfBufAlloc(sizeof(BbOpDesc_t))) == NULL)
  {
    return MAC_154_ERROR;
  }
  memset(pOp, 0, sizeof(BbOpDesc_t));

  /* Allocate storage for data receive BOD's 15.4 specific data */
  if ((pOp->prot.p154 = WsfBufAlloc(sizeof(Bb154Data_t))) == NULL)
  {
    WsfBufFree(pOp);
    return MAC_154_ERROR;
  }

  p154 = pOp->prot.p154;
  memset(p154, 0, sizeof(Bb154Data_t));

  /* Set 802.15.4 operational parameters */
  p154->opParam.flags = PAL_BB_154_FLAG_TX_RX_AUTO_ACK;
  p154->opParam.psduMaxLength = PHY_154_aMaxPHYPacketSize;

  /* Initialize remainder of operation data */
  p154->op.dataRx.pTxDesc = NULL; /* Allocated on the fly */

  /* Initialize 15.4 BOD protocol */
  pOp->reschPolicy = BB_RESCH_BACKGROUND;
  pOp->protId = BB_PROT_15P4;
  pOp->minDurUsec = pPib->maxFrameTotalWaitTime;

  pOp->endCback = mac154DataRxEndCback;
  pOp->abortCback = mac154DataRxEndCback;

  /* Set the 802.15.4 operation type */
  p154->opType = BB_154_OP_DATA_RX;

  /* Set overall MAC state */
  Mac154SetState(MAC_154_STATE_RX);

  /* Initiate Rx Data */
  p154->chan.channel = pPhyPib->chan;
  p154->chan.txPower = pPhyPib->txPower;

  SchInsertNextAvailable(pOp);

  return MAC_154_SUCCESS;
}

#if MAC_154_OPT_ORPHAN

/*************************************************************************************************/
/*!
 *  \brief      Start orphan response.
 *
 *  \param      orphanAddr    Extended address of orphaned
 *  \param      shtAddr       Short address of orphaned device
 *  \param      assocMember   Whether orphaned device is associated through coordinator
 *
 *  \return     Result code.
 */
/*************************************************************************************************/
uint8_t Mac154DataOrphanRspStart(uint64_t orphanAddr, uint16_t shtAddr, uint8_t assocMember)
{
  if (assocMember)
  {
    Mac154Pib_t * const pPib = Mac154GetPIB();
    Bb154Data_t *p154;

    /* Allocate storage for data transmit BOD's 15.4 specific data */
    if ((p154 = WsfBufAlloc(sizeof(Bb154Data_t))) == NULL)
    {
      return MAC_154_ERROR;
    }
    memset(p154, 0, sizeof(Bb154Data_t));

    /* Set 802.15.4 operational parameters */
    p154->opParam.flags = PAL_BB_154_FLAG_TX_RX_AUTO_ACK;

    p154->opParam.psduMaxLength = PHY_154_aMaxPHYPacketSize;

    if ((p154->op.dataTx.pTxDesc = Bb154BuildCoordRealign(orphanAddr, pPib->panId, shtAddr)) == NULL)
    {
      WsfBufFree(p154);
      return MAC_154_ERROR;
    }

    /* Initialize remainder of operation data (TxFrame already done) */
    p154->op.dataTx.pTxDesc->handle = 0;
    p154->op.dataTx.timestamp = 0;

    /* Start the baseband operation */
    return mac154DataTxStartOp(p154);
  }
  return MAC_154_SUCCESS; /* Silently discard if assocMember == FALSE [105, 7]*/
}

#endif /* MAC_154_OPT_ORPHAN */

/*************************************************************************************************/
/*!
 *  \brief      Start coordinator realignment.
 *
 *  \param      panId     PAN ID start parameter
 *  \param      panCoord  PAN Coordinator start parameter
 *  \param      logChan   Logical channel start parameter
 *  \param      txPower   Transmit power start parameter
 *
 *  \return     Result code
 *
 *  \note       Starts a coordinator realignment frame due to a superframe configuration
 *              change from an MLME-START.req
 */
/*************************************************************************************************/
uint8_t Mac154DataCoordRealignStart(uint16_t panId, uint8_t panCoord, uint8_t logChan, uint8_t txPower)
{
  Bb154Data_t *p154;

  /* Allocate storage for data transmit BOD's 15.4 specific data */
  if ((p154 = WsfBufAlloc(sizeof(Bb154Data_t))) == NULL)
  {
    return MAC_154_ERROR;
  }
  memset(p154, 0, sizeof(Bb154Data_t));

  /* Set 802.15.4 operational parameters */
  p154->opParam.flags = PAL_BB_154_FLAG_TX_RX_AUTO_ACK;

  p154->opParam.psduMaxLength = PHY_154_aMaxPHYPacketSize;

  if ((p154->op.start.pTxDesc = Bb154BuildCoordRealign(0, panId, MAC_154_BROADCAST_ADDR)) == NULL)
  {
    WsfBufFree(p154);
    return MAC_154_ERROR;
  }

  /* Initialize remainder of operation data (TxFrame already done) */
  p154->op.start.pTxDesc->handle = 0;
  p154->op.start.timestamp = 0;

  p154->op.start.panId = panId;
  p154->op.start.panCoord = panCoord;
  p154->op.start.logChan = logChan;
  p154->op.start.txPower = txPower;

  /* Start the baseband operation */
  return mac154DataTxStartOp(p154);
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize MAC data
 *
 *  \param      None.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Mac154DataInit(void)
{
  /* Nothing to do - yet */
}
