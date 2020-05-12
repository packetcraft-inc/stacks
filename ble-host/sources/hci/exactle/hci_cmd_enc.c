/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  HCI command module.
 *
 *  Copyright (c) 2009-2018 Arm Ltd. All Rights Reserved.
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

#include <string.h>
#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_queue.h"
#include "wsf_timer.h"
#include "wsf_msg.h"
#include "wsf_trace.h"
#include "util/bstream.h"
#include "util/calc128.h"
#include "hci_cmd.h"
#include "hci_tr.h"
#include "hci_api.h"
#include "hci_main.h"
#include "hci_core_ps.h"

#include "ll_api.h"

/*************************************************************************************************/
/*!
 *  \brief  HCI LE encrypt command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeEncryptCmd(uint8_t *pKey, uint8_t *pData)
{
  hciLeEncryptCmdCmplEvt_t evt;
  uint8_t status;
  (void)status;

  status = LlEncrypt(pKey, pData);
  WSF_ASSERT(status == LL_SUCCESS);
  evt.hdr.event = HCI_LE_ENCRYPT_CMD_CMPL_CBACK_EVT;
  evt.status = evt.hdr.status = status;
  Calc128Cpy(evt.data, pData);

  hciCoreEvtSendIntEvt((uint8_t *)&evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE long term key request negative reply command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeLtkReqNegReplCmd(uint16_t handle)
{
  LlLtkReqNegReply(handle);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE long term key request reply command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeLtkReqReplCmd(uint16_t handle, uint8_t *pKey)
{
  LlLtkReqReply(handle, pKey);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE start encryption command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeStartEncryptionCmd(uint16_t handle, uint8_t *pRand, uint16_t diversifier, uint8_t *pKey)
{
  LlStartEncryption(handle, pRand, diversifier, pKey);
}

