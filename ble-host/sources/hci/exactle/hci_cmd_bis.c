/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  HCI Broadcast Isochronous Stream (BIS) command module.
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
 */
 /*************************************************************************************************/

#include <string.h>
#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_queue.h"
#include "wsf_timer.h"
#include "wsf_msg.h"
#include "wsf_trace.h"
#include "util/bstream.h"
#include "hci_cmd.h"
#include "hci_tr.h"
#include "hci_api.h"
#include "hci_main.h"
#include "hci_core_ps.h"

#include "ll_api.h"

/*************************************************************************************************/
/*!
 *  \brief      HCI LE create BIG command.
 *
 *  \param      pCreateBis    Create BIG parameters.
 *
 *  \return     None.
 */
 /*************************************************************************************************/
void HciLeCreateBigCmd(HciCreateBig_t *pCreateBig)
{
  uint8_t status;

  if ((status = LlCreateBig((LlCreateBig_t *) pCreateBig)) != HCI_SUCCESS)
  {
    HciLeCreateBigCmplEvt_t evt;

    evt.hdr.param = evt.bigHandle = pCreateBig->bigHandle;
    evt.hdr.status = evt.status = status;
    evt.hdr.event = HCI_LE_CREATE_BIG_CMPL_CBACK_EVT;

    hciCoreEvtSendIntEvt((uint8_t *) &evt, sizeof(evt));
  }
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE terminate BIG command.
 *
 *  \param      bigHandle     Used to identify the BIG.
 *  \param      reason        Termination reason.
 *
 *  \return     None.
 */
 /*************************************************************************************************/
void HciTerminateBigCmd(uint8_t bigHandle, uint8_t reason)
{
  uint8_t status;

  if ((status = LlTerminateBig(bigHandle, reason)) != HCI_SUCCESS)
  {
    HciLeTerminateBigCmplEvt_t evt;

    evt.hdr.param = evt.bigHandle = bigHandle;
    evt.hdr.status = status;
    evt.hdr.event = HCI_LE_TERM_BIG_CMPL_CBACK_EVT;

    evt.reason = reason;

    hciCoreEvtSendIntEvt((uint8_t *) &evt, sizeof(evt));
  }
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE BIG create synccommand.
 *
 *  \param      pCreateSync     BIG Create Sync parameters.
 *
 *  \return     Status error code.
 */
 /*************************************************************************************************/
void HciLeBigCreateSyncCmd(HciBigCreateSync_t *pCreateSync)
{
  uint8_t status;

  if ((status = LlBigCreateSync((LlBigCreateSync_t *) pCreateSync)) != HCI_SUCCESS)
  {
    HciLeBigSyncEstEvt_t evt;

    evt.hdr.param = evt.bigHandle = pCreateSync->bigHandle;
    evt.hdr.status = evt.status = status;
    evt.hdr.event = HCI_LE_BIG_SYNC_EST_CBACK_EVT;

    hciCoreEvtSendIntEvt((uint8_t *) &evt, sizeof(evt));
  }
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE BIG terminate command.
 *
 *  \param      bigHandle     Used to identify the BIG.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeBigTerminateSync(uint8_t bigHandle)
{
  LlBigTerminateSync(bigHandle);
}
