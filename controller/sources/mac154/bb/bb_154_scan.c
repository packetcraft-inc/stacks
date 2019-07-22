/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      802.15.4 baseband: Scan.
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
#include "mac_154_int.h"
#include "chci_154_int.h"

#include "wsf_assert.h"
#include "wsf_buf.h"

#include <string.h>

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief      Number of times to perform test mode ED scan. */
#define BB_154_ED_SCAN_TEST_MODE_NUM   10

/**************************************************************************************************
  Function Prototypes
**************************************************************************************************/

#ifdef USE_GUARD_TIMER

/*************************************************************************************************/
/*!
 *  \brief      Guard timer cleanup
 *
 *  \param      param   Opaque pointer to the BOD ending.
 *
 *  \return     None.
 *
 *  \note       Called from scheduler context, not ISR.
 */
/*************************************************************************************************/
static void bb154ScanGuardTimerCback(void *param)
{
  BbOpDesc_t *pOp = (BbOpDesc_t *)param;
  Bb154Data_t * const p154 = pOp->prot.p154;

  /* Terminate the scan */
  pOp->prot.p154->op.scan.terminate = TRUE;
  Bb154GenCleanupOp(pOp, p154);
}
#endif


/*************************************************************************************************/
/*!
 *  \brief      Process timeout at end of ISR
 *
 *  \param      pOp             Pointer to the BOD.
 *  \param      guardTimeSymb   Additional guard time (in symbols).
 *
 *  \return     TRUE if timed out, FALSE otherwise
 */
/*************************************************************************************************/
static bool_t bb154ProcessTimeout(BbOpDesc_t * const pOp, uint32_t guardTimeSymb)
{
  Bb154Data_t * const p154 = pOp->prot.p154;
  Bb154Scan_t * const pScan = &p154->op.scan;
  volatile uint32_t currentTime;

  /* Calculated elapsed time */
  currentTime = PalBbGetCurrentTime(USE_RTC_BB_CLK);
  pScan->elapsed = currentTime - pScan->snapshot; /* Note subtraction will work even if counter has wrapped */
  /* Reset snapshot */
  pScan->snapshot = currentTime;

  /* Take off elapsed time (in symbols) from remaining scan duration */
  if (pScan->remaining < PAL_BB_154_US_TO_SYMB(pScan->elapsed))
  {
    pScan->remaining = PAL_BB_154_US_TO_SYMB(pScan->elapsed);
  }
  pScan->remaining -= PAL_BB_154_US_TO_SYMB(pScan->elapsed);
  if (pScan->remaining <= guardTimeSymb)
  {
    /* Scan duration timed out. */
    /* Notch out channel just done */
    pScan->channels &= ~(1 << pScan->channel);

    /* Finished scanning one channel, cleanup */
    Bb154GenCleanupOp(pOp, p154);
    /* Note: Sending confirm will be handled in the BOD complete handler */
    return TRUE;
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief      Receive complete callback
 *
 *  \param      pRxBuf        Receive buffer
 *  \param      len           Received frame length
 *  \param      rssi          Received signal strength indicator
 *  \param      timestamp     Timestamp of received frame
 *  \param      flags         Receive flags
 *
 *  \return     rxFlags       Always go idle
 *
 *  \note       Must use BB_154_DRV_BUFFER_PTR on pRxBuf to get frame
 */
/*************************************************************************************************/
static uint8_t bb154ScanActvPasvRxCback(uint8_t *pRxFrame, uint16_t len, int8_t rssi, uint32_t timestamp, uint8_t flags)
{
  Mac154Pib_t * const pPib = Mac154GetPIB();
  BbOpDesc_t * const pOp = BbGetCurrentBod();
  Bb154Data_t * const p154 = pOp->prot.p154;
  Bb154Scan_t * const pScan = &p154->op.scan;
  Mac154PanDescr_t *pPanDescr = NULL;
  Mac154Addr_t srcAddr;
  Mac154Addr_t dstAddr;
  uint16_t fctl;
  uint8_t hdrLen;
  uint8_t sduLen;
  uint8_t bsn;
  uint16_t ss;
  bool_t sendInd = FALSE;
  bool_t terminate = FALSE;
  uint8_t *pRxFrameStart = pRxFrame;

  /* Unused parameters */
  (void)flags;

  /* Sanity check - we are only looking for a beacon */
  if (MAC_154_FC_FRAME_TYPE(pRxFrame[0]) == MAC_154_FRAME_TYPE_BEACON)
  {
    /* Get frame control */
    BSTREAM_TO_UINT16(fctl, pRxFrame);
    /* Get sequence number */
    BSTREAM_TO_UINT8(bsn, pRxFrame);
    pRxFrame = Bb154GetAddrsFromFrame(pRxFrame, fctl, &srcAddr, &dstAddr);
    /* Get Superframe specification */
    BSTREAM_TO_UINT16(ss, pRxFrame);
    /* Get GTS specification. TODO: Should be 0, assume it is for now */
    pRxFrame++;
    /* Get pending address specification: TODO: Should be 0, assume it is for now */
    pRxFrame++;
    /* Calculate header length */
    hdrLen = pRxFrame - pRxFrameStart;

    sduLen = len - hdrLen;
    if ((sduLen > 0) || !pPib->autoRequest)
    {
      /* There is a payload or we send anyway */
      sendInd = TRUE;
    }

    pPanDescr = &pScan->results.panDescr[pScan->listSize];

    /* TODO: Check security of incoming frame */
    pPanDescr->securityFailure = FALSE;

    /* Structure copy address over */
    pPanDescr->coord = srcAddr;
    pPanDescr->logicalChan = pScan->channel;
    UINT16_TO_BUF(pPanDescr->superframeSpec, ss);
    pPanDescr->gtsPermit = FALSE; /* TODO: Assume no GTS */
    pPanDescr->linkQuality = PalBb154RssiToLqi(rssi);
    UINT24_TO_BUF(pPanDescr->timestamp, timestamp);
    /* TODO: Security of incoming frame */

    if (pPib->autoRequest)
    {
      /* Keep PAN descriptor in list, increment list size */
      terminate = (++pScan->listSize == MAC_154_SCAN_MAX_PD_ENTRIES);
    }

    if (sendInd)
    {
      Chci154ScanSendBeaconNotifyInd(bsn, pPanDescr, sduLen, pRxFrame);
    }
  }

  /* Reclaim frame buffer */
  PalBb154ReclaimRxFrame(pRxFrameStart);

  if (terminate)
  {
    /* Finished scanning, cleanup. Ending scan will send the confirm */
    Bb154GenCleanupOp(pOp, p154);
  }
  else if (!bb154ProcessTimeout(pOp, BB_154_ED_DURATION_SYMB)) /* TODO: Correct guard time */
  {
    /* Issue next receive command */
    PalBb154Off(); /* TODO: May no longer be necessary due to checking scan mode in IsRxEnabled() */
    PalBb154Rx(0, TRUE, PAL_BB_154_SYMB_TO_US(pScan->remaining));
  }
  return PAL_BB_154_RX_FLAG_GO_IDLE;
}

/*************************************************************************************************/
/*!
 *  \brief      Active/orphan scan error callback
 *
 *  \param      status  Error status
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154ScanActvOrphErrCback(uint8_t status)
{
  BbOpDesc_t * const pOp = BbGetCurrentBod();
  Bb154Data_t * const p154 = pOp->prot.p154;
  Bb154Scan_t * const pScan = &p154->op.scan;

  switch (status)
  {
    case BB_STATUS_TX_FAILED:
    case BB_STATUS_TX_CCA_FAILED:
      /* Leave current channel as unscanned by not notching it out  */
      /* Finished scanning one channel, cleanup */
      Bb154GenCleanupOp(pOp, p154);
      break;

    case BB_STATUS_RX_TIMEOUT:
      /* Scan duration timed out. */
      /* Notch out channel just done */
      pScan->channels &= ~(1 << pScan->channel);
      /* Finished scanning one channel, cleanup */
      Bb154GenCleanupOp(pOp, p154);
      break;

    default:
      break;
  }
}

/**************************************************************************************************
  15.4 BB driver ED scan callbacks
**************************************************************************************************/

/*
 * ED: bb154ScanEdCompleteCback
 * Rx: N/A? May need handler if enabling Rx on ED scan
 * Tx: N/A
 * Er: bb154ScanEdErrCback
 */

/*************************************************************************************************/
/*!
 *  \brief      Energy detect scan ED complete callback
 *
 *  \param      energyDetect  Energy detect value
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154ScanEdCompleteCback(int8_t rssi)
{
  BbOpDesc_t * const pOp = BbGetCurrentBod();
  Bb154Data_t * const p154 = pOp->prot.p154;
  Bb154Scan_t * const pScan = &p154->op.scan;
  uint8_t energyDetect = PalBb154RssiToLqi(rssi);

  if (pScan->testMode != 0)
  {
    if (pScan->testMode == BB_154_ED_SCAN_TEST_MODE_CCA)
    {
      /* Results are CCA (1 = clear, 0 = not clear) */
      pScan->results.edList[pScan->listSize++] = (rssi <= PAL_BB_154_ED_THRESHOLD) ? 1 : 0;
    }
    else
    {
      /* Assume results are LQI */
      pScan->results.edList[pScan->listSize++] = energyDetect;
    }

    if (pScan->listSize < BB_154_ED_SCAN_TEST_MODE_NUM)
    {
      /* Issue next energy detect command */
      PalBb154Ed(0, TRUE);
    }
    else
    {
      /* Notch out channel just done */
      pScan->channels &= ~(1 << pScan->channel);

      /* Finished scanning one channel, cleanup */
      Bb154GenCleanupOp(pOp, p154);
    }
  }
  else
  {
    /* Collect result and record the maximum */
    if (energyDetect > pScan->results.edList[pScan->listSize])
    {
      pScan->results.edList[pScan->listSize] = energyDetect;
    }

    if (!bb154ProcessTimeout(pOp, BB_154_ED_DURATION_SYMB))
    {
      /* Issue next energy detect command */
      PalBb154Ed(0, TRUE);
    }
    else
    {
      /* We have recorded a value, increment list size */
      pScan->listSize++;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Energy detect scan error callback
 *
 *  \param      status  Error status
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154ScanEdErrCback(uint8_t status)
{
  /* Unused parameters */
  (void)status;

  /* TODO */
#if 0
  BbOpDesc_t * const pOp = BbGetCurrentBod();
  Bb154Data_t * const p154 = pOp->prot.p154;
  Bb154Scan_t * const pScan = &p154->op.scan;
#endif
}

/**************************************************************************************************
  15.4 BB driver active scan callbacks
**************************************************************************************************/

/*
 * ED: N/A
 * Rx: bb154ScanActvPasvRxCback
 * Tx: bb154ScanActiveTxCback
 * Er: bb154ScanActvOrphErrCback
 */

/*************************************************************************************************/
/*!
 *  \brief      Active scan transmit complete callback
 *
 *  \param      flags Transmit flags
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154ScanActiveTxCback(uint8_t flags)
{
  BbOpDesc_t * const pOp = BbGetCurrentBod();
  Bb154Data_t * const p154 = pOp->prot.p154;
  Bb154Scan_t * const pScan = &p154->op.scan;

  /* Unused parameters */
  (void)flags;

  /* Note: Should always return FALSE; using bb154ProcessTimeout as a convenience */
  if (!bb154ProcessTimeout(pOp, BB_154_ED_DURATION_SYMB)) /* TODO: Correct guard time */
  {
    /* Issue next receive command */
    PalBb154Rx(0, TRUE, PAL_BB_154_SYMB_TO_US(pScan->remaining));
  }
}

/**************************************************************************************************
  15.4 BB driver passive scan callbacks
**************************************************************************************************/

/*
 * ED: N/A
 * Rx: bb154ScanActvPasvRxCback
 * Tx: N/A
 * Er: bb154ScanPassiveErrCback
 */

/*************************************************************************************************/
/*!
 *  \brief      Passive scan error callback
 *
 *  \param      status  Error status
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154ScanPassiveErrCback(uint8_t status)
{
  BbOpDesc_t * const pOp = BbGetCurrentBod();
  Bb154Data_t * const p154 = pOp->prot.p154;
  Bb154Scan_t * const pScan = &p154->op.scan;

  switch (status)
  {
    case BB_STATUS_RX_TIMEOUT:
      /* Scan duration timed out. */

      /* Notch out channel just done and increment list size */
      pScan->channels &= ~(1 << pScan->channel);

      /* Finished scanning one channel, cleanup */
      Bb154GenCleanupOp(pOp, p154);
      break;

    default:
      break;
  }
}

#if MAC_154_OPT_ORPHAN

/**************************************************************************************************
  15.4 BB driver orphan scan callbacks
**************************************************************************************************/

/*
 * ED: N/A
 * Rx: bb154ScanOrphanRxCback
 * Tx: bb154ScanOrphanTxCback
 * Er: bb154ScanActvOrphErrCback
 */

/*************************************************************************************************/
/*!
 *  \brief      Receive complete callback
 *
 *  \param      pRxBuf        Receive buffer
 *  \param      len           Received frame length
 *  \param      rssi          Received signal strength indicator
 *  \param      timestamp     Timestamp of received frame
 *  \param      flags         Receive flags
 *
 *  \return     rxFlags       Always go idle
 *
 *  \note       Must use BB_154_DRV_BUFFER_PTR on pRxBuf to get frame
 */
/*************************************************************************************************/
static uint8_t bb154ScanOrphanRxCback(uint8_t *pRxFrame, uint16_t len, int8_t rssi, uint32_t timestamp, uint8_t flags)
{
  BbOpDesc_t * const pOp = BbGetCurrentBod();
  Bb154Data_t * const p154 = pOp->prot.p154;
  Bb154Scan_t * const pScan = &p154->op.scan;
  Mac154Addr_t srcAddr;
  Mac154Addr_t dstAddr;
  uint16_t fctl;
  uint8_t cmd;
  uint8_t *pRxFrameStart = pRxFrame;

  /* Unused parameters */
  (void)len;
  (void)rssi;
  (void)timestamp;
  (void)flags;

  /* Sanity check - we are only looking for a coordinator realignment requesting an ack. */
  if ((MAC_154_FC_FRAME_TYPE(pRxFrame[0]) == MAC_154_FRAME_TYPE_MAC_COMMAND) &&
       MAC_154_FC_ACK_REQUEST(pRxFrame[0]))
  {
    /* Get frame control */
    BSTREAM_TO_UINT16(fctl, pRxFrame);
    /* Skip sequence number */
    pRxFrame++;
    pRxFrame = Bb154GetAddrsFromFrame(pRxFrame, fctl, &srcAddr, &dstAddr);
    /* Get command type */
    BSTREAM_TO_UINT8(cmd, pRxFrame);
    if (cmd == MAC_154_CMD_FRAME_TYPE_COORD_REALIGN)
    {
      /* Get PAN ID */
      BSTREAM_TO_UINT16(pScan->results.coordRealign.panId, pRxFrame);
      /* Get coord short address */
      BSTREAM_TO_UINT16(pScan->results.coordRealign.coordShtAddr, pRxFrame);
      /* Get logical channel */
      BSTREAM_TO_UINT8(pScan->results.coordRealign.logChan, pRxFrame);
      /* Get short address */
      BSTREAM_TO_UINT16(pScan->results.coordRealign.shtAddr, pRxFrame);

      /* Force scan to terminate when ack. received */
      pScan->terminate = TRUE;
    }
  }

  /* Reclaim frame buffer and just carry on */
  PalBb154ReclaimRxFrame(pRxFrameStart);
  return PAL_BB_154_RX_FLAG_GO_IDLE;
}

/*************************************************************************************************/
/*!
 *  \brief      Orphan scan transmit complete callback
 *
 *  \param      flags Transmit flags
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154ScanOrphanTxCback(uint8_t flags)
{
  Mac154Pib_t * const pPib = Mac154GetPIB();
  Mac154PhyPib_t * const pPhyPib = Mac154GetPhyPIB();
  BbOpDesc_t * const pOp = BbGetCurrentBod();
  Bb154Data_t * const p154 = pOp->prot.p154;
  Bb154Scan_t * const pScan = &p154->op.scan;

  if (flags & PAL_BB_154_FLAG_TX_ACK_CMPL)
  {
    /* Set coordinator realignment parameters now if valid */
    pPib->panId = pScan->results.coordRealign.panId;
    pPib->coordShortAddr = pScan->results.coordRealign.coordShtAddr;
    pPib->shortAddr = pScan->results.coordRealign.shtAddr;
    pPhyPib->chan = pScan->results.coordRealign.logChan;

    /* TODO: Actually change channels now or wait until next op? Next op should sort it out */

    /* Clear any remaining channels */
    pScan->channels = 0;
    /* Finished scanning, cleanup */
    Bb154GenCleanupOp(pOp, p154);
  }
  else
  {
    /* Issue next receive command with timeout to wait for coordinator realignment */
    PalBb154Rx(0, TRUE, pScan->remaining); /* Will be set to macResponseWaitTime */
  }
}

#endif /* MAC_154_OPT_ORPHAN */

/**************************************************************************************************
  BOD scheduler Execute callbacks via 15.4 BB driver
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Execute energy detect scan BOD.
 *
 *  \param      pOp     Pointer to the BOD to execute.
 *  \param      p154    802.15.4 operation parameters.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154ScanEdExecuteOp(BbOpDesc_t *pOp, Bb154Data_t *p154)
{
  p154->opParam.edCback  = bb154ScanEdCompleteCback;
  p154->opParam.txCback  = NULL;
  p154->opParam.rxCback  = NULL;
  p154->opParam.errCback = bb154ScanEdErrCback;

  /* Unused parameters */
  (void)pOp;

  PalBb154SetChannelParam(&p154->chan);
  PalBb154SetOpParams(&p154->opParam);

  /* BOD timing is provided by decrementing a counter after each ED scan result */
  PalBb154Ed(0, TRUE); /* Start now */
}

/*************************************************************************************************/
/*!
 *  \brief      Execute active scan BOD.
 *
 *  \param      pOp     Pointer to the BOD to execute.
 *  \param      p154    802.15.4 operation parameters.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154ScanActiveExecuteOp(BbOpDesc_t *pOp, Bb154Data_t *p154)
{
  Bb154Scan_t * const pScan = &p154->op.scan;

  /* Unused parameters */
  (void)pOp;

  p154->opParam.edCback  = NULL;
  p154->opParam.txCback  = bb154ScanActiveTxCback;
  p154->opParam.rxCback  = bb154ScanActvPasvRxCback;
  p154->opParam.errCback = bb154ScanActvOrphErrCback;

#ifdef USE_GUARD_TIMER
  /* Start guard timer - belt 'n' braces */
  Mac154StartParamTimer(&p154->guardTimer,
                        bb154ScanGuardTimerCback,
                        pOp,
                        PAL_BB_154_SYMB_TO_MS(pScan->duration + 800)); /* TODO: Arbitrary overrun */
#endif
  /* Build receive buffer queue */
  PalBb154BuildRxBufQueue(p154->opParam.psduMaxLength, BB_154_SCAN_MIN_RX_BUF_CNT);

  PalBb154SetChannelParam(&p154->chan);
  PalBb154SetOpParams(&p154->opParam);

  if (pScan->pTxDesc != NULL)
  {
    PAL_BB_154_TX_FRAME_PTR(pScan->pTxDesc)[2] = Mac154GetDSNIncr();
    PalBb154Tx(pScan->pTxDesc, 1, 0, TRUE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Execute passive scan BOD.
 *
 *  \param      pOp     Pointer to the BOD to execute.
 *  \param      p154    802.15.4 operation parameters.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154ScanPassiveExecuteOp(BbOpDesc_t *pOp, Bb154Data_t *p154)
{
  Bb154Scan_t * const pScan = &p154->op.scan;

  /* Unused parameters */
  (void)pOp;

  p154->opParam.edCback  = NULL;
  p154->opParam.txCback  = NULL;
  p154->opParam.rxCback  = bb154ScanActvPasvRxCback;
  p154->opParam.errCback = bb154ScanPassiveErrCback;

  /* Build receive buffer queue */
  PalBb154BuildRxBufQueue(p154->opParam.psduMaxLength, BB_154_SCAN_MIN_RX_BUF_CNT);

  PalBb154SetChannelParam(&p154->chan);
  PalBb154SetOpParams(&p154->opParam);

  /* BOD timing is provided using Rx timeout, diminishing on each Rx frame */
  PalBb154Rx(0, TRUE, PAL_BB_154_SYMB_TO_US(pScan->remaining));
}

#if MAC_154_OPT_ORPHAN

/*************************************************************************************************/
/*!
 *  \brief      Execute orphan scan BOD.
 *
 *  \param      pOp     Pointer to the BOD to execute.
 *  \param      p154    802.15.4 operation parameters.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154ScanOrphanExecuteOp(BbOpDesc_t *pOp, Bb154Data_t *p154)
{
  Bb154Scan_t * const pScan = &p154->op.scan;

  /* Unused parameters */
  (void)pOp;

  p154->opParam.edCback  = NULL;
  p154->opParam.txCback  = bb154ScanOrphanTxCback;
  p154->opParam.rxCback  = bb154ScanOrphanRxCback;
  p154->opParam.errCback = bb154ScanActvOrphErrCback;

  /* Build receive buffer queue */
  PalBb154BuildRxBufQueue(p154->opParam.psduMaxLength, BB_154_SCAN_MIN_RX_BUF_CNT);

  PalBb154SetChannelParam(&p154->chan);
  PalBb154SetOpParams(&p154->opParam);

  if (pScan->pTxDesc != NULL)
  {
    PAL_BB_154_TX_FRAME_PTR(pScan->pTxDesc)[2] = Mac154GetDSNIncr();
    PalBb154Tx(pScan->pTxDesc, 1, 0, TRUE);
  }
}

#endif /* MAC_154_OPT_ORPHAN */

/*************************************************************************************************/
/*!
 *  \brief      Initialize for scan operations.
 *
 *  Update the operation table with scan operations routines according to scan type
 *
 *  \param      scanType  Scan type
 *
 *  \return     None.
 *
 */
/*************************************************************************************************/
void Bb154ScanInit(uint8_t scanType)
{
  switch (scanType)
  {
    case MAC_154_MLME_SCAN_TYPE_ENERGY_DETECT:
      /* Register execute and cleanup routines for ED scan with 15.4 BB driver */
      bb154RegisterOp(BB_154_OP_SCAN, bb154ScanEdExecuteOp);
      break;
    case MAC_154_MLME_SCAN_TYPE_ACTIVE:
      /* Register execute and cleanup routines for active scan with 15.4 BB driver */
      bb154RegisterOp(BB_154_OP_SCAN, bb154ScanActiveExecuteOp);
      break;
    case MAC_154_MLME_SCAN_TYPE_PASSIVE:
      /* Register execute and cleanup routines for passive scan with 15.4 BB driver */
      bb154RegisterOp(BB_154_OP_SCAN, bb154ScanPassiveExecuteOp);
      break;
#if MAC_154_OPT_ORPHAN
    case MAC_154_MLME_SCAN_TYPE_ORPHAN:
      /* Register execute and cleanup routines for active scan with 15.4 BB driver */
      bb154RegisterOp(BB_154_OP_SCAN, bb154ScanOrphanExecuteOp);
      break;
#endif /* MAC_154_OPT_ORPHAN */
    default:
      break;
  }
}
