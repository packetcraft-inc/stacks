/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  HCI PHY command module.
 *
 *  Copyright (c) 2016-2018 Arm Ltd. All Rights Reserved.
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
#include "wsf_msg.h"
#include "util/bstream.h"
#include "hci_cmd.h"
#include "hci_api.h"
#include "hci_main.h"
#include "hci_core_ps.h"

#include "ll_api.h"

/*************************************************************************************************/
/*!
*  \brief  HCI read PHY command.
*
*  \return None.
*/
/*************************************************************************************************/
void HciLeReadPhyCmd(uint16_t handle)
{
  hciLeReadPhyCmdCmplEvt_t evt;

  evt.hdr.param = handle;
  evt.hdr.event = HCI_LE_READ_PHY_CMD_CMPL_CBACK_EVT;
  evt.hdr.status = LlReadPhy(handle, &evt.txPhy, &evt.rxPhy);

  evt.handle = handle;
  evt.status = evt.hdr.status;

  hciCoreEvtSendIntEvt((uint8_t *)&evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
*  \brief  HCI set default PHY command.
*
*  \return None.
*/
/*************************************************************************************************/
void HciLeSetDefaultPhyCmd(uint8_t allPhys, uint8_t txPhys, uint8_t rxPhys)
{
  hciLeSetDefPhyCmdCmplEvt_t evt;

  evt.hdr.param = 0;
  evt.hdr.event = HCI_LE_SET_DEF_PHY_CMD_CMPL_CBACK_EVT;
  evt.hdr.status = LlSetDefaultPhy(allPhys, txPhys, rxPhys);

  evt.status = evt.hdr.status;

  hciCoreEvtSendIntEvt((uint8_t *)&evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
*  \brief  HCI set PHY command.
*
*  \return None.
*/
/*************************************************************************************************/
void HciLeSetPhyCmd(uint16_t handle, uint8_t allPhys, uint8_t txPhys, uint8_t rxPhys, uint16_t phyOptions)
{
  LlSetPhy(handle, allPhys, txPhys, rxPhys, phyOptions);
}
