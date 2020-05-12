/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  HCI Connected Isochronous Stream (CIS) command module.
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
 *  \brief      HCI LE set CIG parameters command.
 *
 *  \param      pCigParam    CIG parameters.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeSetCigParamsCmd(HciCisCigParams_t *pCigParam)
{
  hciLeSetCigParamsCmdCmplEvt_t evt;
  uint16_t cisHandle[LL_MAX_CIS] = {0};
  
  evt.hdr.param = pCigParam->cigId;
  evt.hdr.status = LlSetCigParams((LlCisCigParams_t *) pCigParam, cisHandle);
  evt.hdr.event = HCI_LE_SET_CIG_PARAMS_CMD_CMPL_CBACK_EVT;

  evt.status = evt.hdr.status;
  evt.cigId = pCigParam->cigId;
  evt.numCis = pCigParam->numCis;

  memcpy(evt.cisHandle, cisHandle, pCigParam->numCis * sizeof(uint16_t));

  hciCoreEvtSendIntEvt((uint8_t *) &evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE create CIS command.
 *
 *  \param      numCis            Nunber of CISes.
 *  \param      pCreateCisParam   Parameters for creating connected isochronous stream.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeCreateCisCmd(uint8_t numCis, HciCisCreateCisParams_t *pCreateCisParam)
{
  uint8_t status;

  status = LlCreateCis(numCis, (LlCisCreateCisParams_t *) pCreateCisParam);
  (void)status;

  WSF_ASSERT(status == LL_SUCCESS);
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE remove CIG command.
 *
 *  \param      cigId        Identifer of a CIG.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeRemoveCigCmd(uint8_t cigId)
{
  hciLeRemoveCigCmdCmplEvt_t evt;

  evt.hdr.param = cigId;
  evt.hdr.status = LlRemoveCig(cigId);
  evt.hdr.event = HCI_LE_REMOVE_CIG_CMD_CMPL_CBACK_EVT;

  evt.status = evt.hdr.status;
  evt.cigId = cigId;

  hciCoreEvtSendIntEvt((uint8_t *) &evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE accept CIS request command.
 *
 *  \param      handle       Connection handle of the CIS to be accepted.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeAcceptCisReqCmd(uint16_t handle)
{
  uint8_t status;

  status = LlAcceptCisReq(handle);
  (void)status;

  WSF_ASSERT(status == LL_SUCCESS);
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE reject CIS request command.
 *
 *  \param      handle       Connection handle of the CIS to be rejected.
 *  \param      reason       Reason the CIS request was rejected.
 *
 *  \return     None.
*/
/*************************************************************************************************/
void HciLeRejectCisReqCmd(uint16_t handle, uint8_t reason)
{
  uint8_t status;

  status = LlRejectCisReq(handle, reason);
  (void)status;

  WSF_ASSERT(status == LL_SUCCESS);
}
