/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  802.15.4 MAC: Scan.
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

#define NEW_STYLE

/**************************************************************************************************
  Macros
**************************************************************************************************/

/**************************************************************************************************
  Data Types
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
 *  \brief      Set the next channel to scan.
 *
 *  \param      pOp   Pointer to the current BOD.
 *
 *  \return     None.
 *
 *  This should only be called if it is sure that there is at least one channel left to scan
 *  in pScan->channels.
 */
/*************************************************************************************************/
static void mac154ScanSetNextChannel(BbOpDesc_t *pOp)
{
  Bb154Data_t * const p154 = pOp->prot.p154;
  Bb154Scan_t * const pScan = &p154->op.scan;

  /* Increment channel according to bitmap (see note above) */
  while (((1 << pScan->channel) & pScan->channels) == 0)
  {
    pScan->channel++;
  }

  /* Set the BOD duration */
  pOp->minDurUsec = pOp->maxDurUsec = PAL_BB_154_SYMB_TO_US(pScan->duration);

  /* Set channel, snapshot baseband timer and invoke BOD */
  pOp->prot.p154->chan.channel = pScan->channel;
  pOp->prot.p154->chan.txPower = 0;
  pScan->snapshot = PalBbGetCurrentTime(USE_RTC_BB_CLK);
  SchInsertNextAvailable(pOp);
}

/**************************************************************************************************
  BOD cleanup
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Cleanup BOD at end of channel
 *
 *  \param      pOp    Pointer to the BOD ending.
 *
 *  \return     None.
 *
 *  \note       Called from scheduler context, not ISR.
 */
/*************************************************************************************************/
static void mac154ScanEndCback(BbOpDesc_t *pOp)
{
  Bb154Data_t * const p154 = pOp->prot.p154;
  Bb154Scan_t * const pScan = &p154->op.scan;
  static const uint8_t listSizeLUT[MAC_154_MLME_NUM_SCAN_TYPE] =
    { MAC_154_SCAN_MAX_ED_ENTRIES,
      MAC_154_SCAN_MAX_PD_ENTRIES,
      MAC_154_SCAN_MAX_PD_ENTRIES };

  if (pScan->terminate ||
      (pScan->channels == 0) ||
      (pScan->channel == PHY_154_LAST_CHANNEL) ||
      ((pScan->type != MAC_154_MLME_SCAN_TYPE_ORPHAN) && (pScan->listSize == listSizeLUT[pScan->type])))
  {
    /* No more channels. Stop BB, send confirm and release BOD memory */
    BbStop(BB_PROT_15P4);
    Chci154ScanSendCfm(pScan->channels, pScan->type, pScan->listSize, &pScan->results, 0);
    if (pScan->pTxDesc != NULL)
    {
      WsfBufFree(pScan->pTxDesc);
    }
    WsfBufFree(p154);
    WsfBufFree(pOp);

    /* Indicate we are no longer scanning */
    Mac154SetState(MAC_154_STATE_IDLE);
  }
  else
  {
    /* Move on to next channel */
    pScan->channel++;
    mac154ScanSetNextChannel(pOp);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Start scan operation
 *
 *  \param      pPendingTxHandle  Opaque handle to 15.4 operation to (re)start
 *
 *  \return     Status code.
 */
/*************************************************************************************************/
static uint8_t mac154ScanStartOp(void *pPendingTxHandle)
{
  BbOpDesc_t *pOp;
  Bb154Data_t * const p154 = (Bb154Data_t *)pPendingTxHandle;
  Bb154Scan_t *pScan = &p154->op.scan;

  /* Allocate storage for data transmit BOD */
  if ((pOp = WsfBufAlloc(sizeof(BbOpDesc_t))) == NULL)
  {
    WsfBufFree(p154);
    return MAC_154_ERROR;
  }
  memset(pOp, 0, sizeof(BbOpDesc_t));
  pOp->prot.p154 = p154;

  /* Initialize scan BOD protocol */
  pOp->reschPolicy = BB_RESCH_MOVEABLE_PREFERRED;
  pOp->protId = BB_PROT_15P4;
  pOp->endCback = mac154ScanEndCback;
  pOp->abortCback = mac154ScanEndCback;

  /* Set the 802.15.4 operation type */
  p154->opType = BB_154_OP_SCAN;
  Mac154SetState(MAC_154_STATE_SCAN);

  /* Set 802.15.4 operational parameters */
  p154->opParam.flags = PAL_BB_154_FLAG_TX_RX_AUTO_ACK;
  p154->opParam.psduMaxLength = PHY_154_aMaxPHYPacketSize;

  /* Set scan type for baseband operation */
  Bb154ScanInit(pScan->type);

  /* Claim baseband for 15.4 use */
  BbStart(BB_PROT_15P4);

  /* Kick off scanning */
  mac154ScanSetNextChannel(pOp);
  return MAC_154_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \brief      Start scan.
 *
 *  \param      scanType      Type of scan (ED, active, passive)
 *  \param      scanChannels  Bitmap of channels to scan
 *  \param      scanDuration  Scan duration (exponential)
 *  \param      testMode      ED/CCA test mode - 10 scans in the list
 *
 *  \return     Result code
 */
/*************************************************************************************************/
uint8_t Mac154ScanStart(uint8_t scanType, uint32_t scanChannels, uint8_t scanDuration, uint8_t testMode)
{
  Bb154Data_t *p154;
  Bb154Scan_t *pScan;

  /* Allocate storage for data transmit BOD's 15.4 specific data */
  if ((p154 = WsfBufAlloc(sizeof(Bb154Data_t))) != NULL)
  {
    memset(p154, 0, sizeof(Bb154Data_t));
  }
  else
  {
    return MAC_154_ERROR;
  }

  pScan = &p154->op.scan;

  /* Initialize scan parameters */
  pScan->listSize = 0;
  pScan->channels = scanChannels;
  pScan->type = scanType;
  pScan->testMode = testMode;
  /* Calculate Scan duration in symbols */
  pScan->duration = BB_154_BASE_SUPERFRAME_DURATION_SYMB * ((1 << scanDuration) + 1);

  /* Select the first valid channel */
  pScan->channel = PHY_154_FIRST_CHANNEL;

  /* Default to scan duration */
  pScan->remaining = pScan->duration;

  switch (pScan->type)
  {
    case MAC_154_MLME_SCAN_TYPE_ACTIVE:
      /* Build beacon request to Tx if active scan */
      if ((pScan->pTxDesc = Bb154ScanBuildBeaconReq()) == NULL)
      {
        WsfBufFree(p154);
        return MAC_154_ERROR;
      }
      break;
#ifdef MAC_154_OPT_ORPHAN
    case MAC_154_MLME_SCAN_TYPE_ORPHAN:
      {
        Mac154Pib_t * const pPib = Mac154GetPIB();
        /* Build orphan notification to Tx if orphan scan */
        if ((pScan->pTxDesc = Bb154ScanBuildOrphanNtf()) == NULL)
        {
          WsfBufFree(p154);
          return MAC_154_ERROR;
        }
        /* Remaining time for rx is simply response wait time */
        pScan->remaining = PAL_BB_154_SYMB_TO_US(pPib->responseWaitTime);
      }
      break;
#endif /* MAC_154_OPT_ORPHAN */
    default:
      break;
  }

  /* Start the baseband operation */
  return mac154ScanStartOp(p154);
}

/*************************************************************************************************/
/*!
 *  \brief      Start single channel ED scan.
 *
 *  \param      channel         Channel to scan
 *  \param      scanDurationMs  Scan duration in ms
 *
 *  \return     Result code
 *
 *  \note       This function is required for OpenThread ED scan.
 */
/*************************************************************************************************/
uint8_t Mac154SingleChanEDScanStart(uint8_t channel, uint32_t scanDurationMs)
{
  Bb154Data_t *p154;
  Bb154Scan_t *pScan;

  /* Allocate storage for data transmit BOD's 15.4 specific data */
  if ((p154 = WsfBufAlloc(sizeof(Bb154Data_t))) != NULL)
  {
    memset(p154, 0, sizeof(Bb154Data_t));
  }
  else
  {
    return MAC_154_ERROR;
  }

  pScan = &p154->op.scan;

  /* Initialize scan parameters */
  pScan->listSize = 0;
  pScan->channels = (uint32_t)(1 << channel); /* Single channel */
  pScan->type = MAC_154_MLME_SCAN_TYPE_ENERGY_DETECT;
  /* Calculate Scan duration in symbols */
  pScan->duration = scanDurationMs * 1000;

  /* Select the first valid channel */
  pScan->channel = PHY_154_FIRST_CHANNEL;

  /* Default to scan duration */
  pScan->remaining = pScan->duration;

  /* Start the baseband operation */
  return mac154ScanStartOp(p154);
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize MAC scan
 *
 *  \param      None.
 *
 *  \return     None.
 *
 *  Initializes MAC scan control block
 */
/*************************************************************************************************/
void Mac154ScanInit(void)
{
  /* Nothing to do - yet */
}

