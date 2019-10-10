/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      802.15.4 baseband: Main.
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
#include "bb_154.h"
#include "bb_154_api_op.h"
#include "pal_bb.h"

#include "wsf_assert.h"
#include "wsf_buf.h"

#include <string.h>

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

bb154CtrlBlk_t bb154Cb;               /*!< BB 802.15.4 control block. */

/**************************************************************************************************
  Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Cleanup BOD.
 *
 *  \param      pOp    Pointer to the BOD to cleanup.
 *  \param      p154   15.4 operation parameters.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Bb154GenCleanupOp(BbOpDesc_t *pOp, Bb154Data_t *p154)
{
  /* Unusued parameters */
  (void)p154;

#ifdef USE_GUARD_TIMER
  /* Stop guard timer */
  WsfTimerStop(&p154->guardTimer.timer);
#endif

  /* Force driver to off state */
  PalBb154Off();
  PalBb154ClearRxBufs();
  PalBb154ResetChannelParam();

  if (pOp->reschPolicy != BB_RESCH_BACKGROUND)
  {
    /* Invokes end callback in scheduler handler */
    BbTerminateBod();
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Start BB processing of 802.15.4 protocol.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154Start154(void)
{
  PalBb154Enable();
}

/*************************************************************************************************/
/*!
 *  \brief      Stop BB processing of 802.15.4 protocol.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154Stop154(void)
{
  /* Turn off first before disabling */
  PalBb154Off();
  PalBb154Disable();
}

/*************************************************************************************************/
/*!
 *  \brief      Execute operation.
 *
 *  \param      pOp     Pointer to the BOD being executed.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154ExecOp(BbOpDesc_t *pOp)
{
  Bb154Data_t * const p154 = pOp->prot.p154;

  WSF_ASSERT(p154->opType < BB_154_OP_NUM);

  if (bb154Cb.opCbacks[p154->opType].execOpCback)
  {
    bb154Cb.opCbacks[p154->opType].execOpCback(pOp, p154);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Cancel operation.
 *
 *  \param      pOp     Pointer to the BOD being canceled.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void bb154CancelOp(BbOpDesc_t *pOp)
{
  Bb154Data_t * const p154 = pOp->prot.p154;

  WSF_ASSERT(p154->opType < BB_154_OP_NUM);

  /* No need for a specific per-operation cleanup. */
  Bb154GenCleanupOp(pOp, p154);
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize the 802.15.4 BB.
 *
 *  \return     None.
 *
 *  Initialize baseband resources.
 */
/*************************************************************************************************/
void Bb154Init(void)
{
  PalBb154Init();
  BbRegisterProt(BB_PROT_15P4, bb154ExecOp, bb154CancelOp, bb154Start154, bb154Stop154);

  memset(&bb154Cb, 0, sizeof(bb154Cb));
}

/*************************************************************************************************/
/*!
 *  \brief      Check if 15.4 Rx is in progress
 *
 *  \return     BbOpDesc_t*   Pointer to BOD if in progress.
 */
/*************************************************************************************************/
BbOpDesc_t *Bb154RxInProgress(void)
{
  BbOpDesc_t *pOp;

  /* Check we are actually receiving first */
  if (((pOp = BbGetCurrentBod()) != NULL) &&
      (pOp->protId == BB_PROT_15P4) &&
      (pOp->prot.p154->opType == BB_154_OP_DATA_RX))
  {
    return pOp;
  }
  return NULL;
}

/*************************************************************************************************/
/*!
 *  \brief      Register operation handlers.
 *
 *  \param      opType          Operation type.
 *  \param      execOpCback     Execute operation callback.
 *  \param      cancelOpCback   Cancel operation callback.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void bb154RegisterOp(uint8_t opType, bb154ExecOpFn_t execOpCback)
{
  bb154Cb.opCbacks[opType].execOpCback   = execOpCback;
}
