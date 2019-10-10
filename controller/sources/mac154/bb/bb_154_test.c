/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      802.15.4 baseband: Test.
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

#include "bb_api.h"
#include "bb_154_int.h"
#include "bb_154.h"
#include "bb_154_api_op.h"
#include "pal_bb.h"

#include "wsf_assert.h"
#include "wsf_buf.h"

#include <string.h>

/*************************************************************************************************/
/*!
 *  \brief      Complete a transmit.
 *
 *  \param      ack       TRUE if ACK status.
 *  \param      success   TRUE if operation succeeded.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154TestTxCback(bool_t ack, bool_t success)
{
  BbOpDesc_t * const pOp = BbGetCurrentBod();
  Bb154Data_t * const p154 = pOp->prot.p154;
  Bb154TestTx_t * const pTx = &p154->op.testTx;

  /* Successful acknowledgment. */
  bool_t bodComplete = !pTx->testCback(pOp, ack, success);
  if (bodComplete || !success)
  {
    Bb154GenCleanupOp(pOp, p154);
  }
  else
  {
    /* Transmit same packet. */
    pOp->due += pTx->pktInterUsec;

    /* Increment sequence number. */
    PAL_BB_154_TX_FRAME_PTR(p154->op.testTx.pTxDesc)[2]++;

    PalBb154Tx(p154->op.testTx.pTxDesc, 1, pOp->due, FALSE);
  }
}
/*************************************************************************************************/
/*!
 *  \brief      Transmit test transmit ISR callback.
 *
 *  \param      flags         Transmit flags.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154TestTxTxCback(uint8_t flags)
{
  if ((flags & PAL_BB_154_FLAG_RX_ACK_START) == 0)
  {
    bb154TestTxCback(FALSE, TRUE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Transmit test receive ISR callback.
 *
 *  \param      pBuf          Receive buffer.
 *  \param      len           PSDU length.
 *  \param      rssi          Received signal strength indicator.
 *  \param      timestamp     PSDU start timestamp.
 *  \param      flags         Receive flags.
 *
 *  \return     rxFlags       Always go idle
 */
/*************************************************************************************************/
static uint8_t bb154TestTxRxCback(uint8_t *pRxFrame, uint16_t len, int8_t rssi, uint32_t timestamp, uint8_t flags)
{
  /* Unused parameters */
  (void)len;
  (void)rssi;
  (void)timestamp;
  (void)flags;

  PalBb154Off();
  bb154TestTxCback(TRUE, TRUE);

  /* Reclaim frame buffer. */
  PalBb154ReclaimRxFrame(pRxFrame);
  return PAL_BB_154_RX_FLAG_GO_IDLE;
}

/*************************************************************************************************/
/*!
 *  \brief      Transmit test operation error ISR callback.
 *
 *  \param      status  Error status
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154TestTxErrCback(uint8_t status)
{
  switch (status)
  {
    /* ---------- rx failures ---------- */

    case BB_STATUS_FAILED:
    case BB_STATUS_ACK_TIMEOUT:
    case BB_STATUS_CRC_FAILED:
    case BB_STATUS_FRAME_FAILED:
      /* Fallthrough */
    case BB_STATUS_RX_TIMEOUT:
      bb154TestTxCback(TRUE, FALSE);
      break;

    /* ---------- tx failures ---------- */

    case BB_STATUS_TX_FAILED:
      bb154TestTxCback(FALSE, FALSE);
      break;

    /* ---------- unexpected failures ---------- */

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Execute test mode transmit BOD.
 *
 *  \param      pOp     Pointer to the BOD to execute.
 *  \param      p154    802.15.4 operation parameters.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154TestExecuteTxOp(BbOpDesc_t *pOp, Bb154Data_t *p154)
{
  /* Unused parameters */
  (void)pOp;

  p154->opParam.edCback  = NULL;
  p154->opParam.txCback  = bb154TestTxTxCback;
  p154->opParam.rxCback  = bb154TestTxRxCback;
  p154->opParam.errCback = bb154TestTxErrCback;

  /* Build receive buffer queue */
  PalBb154BuildRxBufQueue(p154->op.testTx.rxLen, PAL_BB_154_MIN_RX_BUF_CNT);

  PalBb154SetChannelParam(&p154->chan);
  PalBb154SetOpParams(&p154->opParam);

  /* Increment sequence number. */
  PAL_BB_154_TX_FRAME_PTR(p154->op.testTx.pTxDesc)[2]++;

  //PalBb154Tx(p154->op.testTx.pTxDesc, 1, pOp->due, FALSE);
  /* TEST ONLY */
  PalBb154Tx(p154->op.testTx.pTxDesc, 1, 0, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief      Complete a receive.
 *
 *  \param      ack       TRUE if ACK status.
 *  \param      success   TRUE if operation succeeded.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154TestRxCback(bool_t ack, bool_t success)
{
  BbOpDesc_t * const pOp = BbGetCurrentBod();
  Bb154Data_t * const p154 = pOp->prot.p154;
  Bb154TestRx_t * const pRx = &p154->op.testRx;

  /* Successful acknowledgment. */
  bool_t bodComplete = !pRx->testCback(pOp, ack, success);
  if (bodComplete || !success)
  {
    Bb154GenCleanupOp(pOp, p154);
  }
  else
  {
    /* set up automatically. */
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Receive test transmit ISR callback.
 *
 *  \param      flags         Transmit flags.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154TestRxTxCback(uint8_t flags)
{
  /* Unused parameters */
  (void)flags;

  bb154TestRxCback(TRUE, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief      Receive test receive ISR callback.
 *
 *  \param      pBuf          Receive buffer.
 *  \param      len           PSDU length.
 *  \param      rssi          Received signal strength indicator
 *  \param      timestamp     PSDU start timestamp.
 *  \param      flags         Receive flags.
 *
 *  \return     Can go idle - always TRUE.
 */
/*************************************************************************************************/
static uint8_t bb154TestRxRxCback(uint8_t *pRxFrame, uint16_t len, int8_t rssi, uint32_t timestamp, uint8_t flags)
{
  /* Unused parameters */
  (void)len;
  (void)rssi;
  (void)timestamp;

  if ((flags & PAL_BB_154_FLAG_TX_ACK_START) == 0)
  {
    bb154TestRxCback(FALSE, TRUE);
  }

  /* Recycle buffer. */
  PalBb154ReclaimRxFrame(pRxFrame);
  return PAL_BB_154_RX_FLAG_GO_IDLE;
}

/*************************************************************************************************/
/*!
 *  \brief      Receive test operation error ISR callback.
 *
 *  \param      status        Operation status.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154TestRxErrCback(uint8_t status)
{
  switch (status)
  {
    /* ---- receive failures ----- */

    case BB_STATUS_FAILED:
    case BB_STATUS_CRC_FAILED:
    case BB_STATUS_FRAME_FAILED:
      /* receive already restarted */
      bb154TestRxCback(FALSE, FALSE);
      break;

    /* ----- transmit failures ----- */

    case BB_STATUS_ACK_FAILED:
      bb154TestRxCback(TRUE, FALSE);
      break;

    /* ----- unexpected failures ----- */

    case BB_STATUS_RX_TIMEOUT:
    default:
      WSF_ASSERT(FALSE);
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Execute test mode receive BOD.
 *
 *  \param      pOp     Pointer to the BOD to execute.
 *  \param      p154    802.15.4 operation parameters.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154TestExecuteRxOp(BbOpDesc_t *pOp, Bb154Data_t *p154)
{
  p154->opParam.edCback  = NULL;
  p154->opParam.txCback  = bb154TestRxTxCback;
  p154->opParam.rxCback  = bb154TestRxRxCback;
  p154->opParam.errCback = bb154TestRxErrCback;

  /* Build receive buffer queue */
  PalBb154BuildRxBufQueue(p154->op.testRx.rxLen, PAL_BB_154_MIN_RX_BUF_CNT);

  PalBb154SetChannelParam(&p154->chan);
  PalBb154SetOpParams(&p154->opParam);

  PalBb154Rx(pOp->due, TRUE, 0);
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize for test operations.
 *
 *  \return     None.
 *
 *  Update the operation table with test operations routines.
 */
/*************************************************************************************************/
void Bb154TestInit(void)
{
  bb154RegisterOp(BB_154_OP_TEST_TX, bb154TestExecuteTxOp);
  bb154RegisterOp(BB_154_OP_TEST_RX, bb154TestExecuteRxOp);
}
