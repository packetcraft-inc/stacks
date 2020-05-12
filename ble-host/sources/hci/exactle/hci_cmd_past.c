/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  HCI Periodic Advertising Sync Transfer (PAST) command module.
 *
 *  Copyright (c) 2018 Arm Ltd. All Rights Reserved.
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
#include "wsf_msg.h"
#include "util/bstream.h"
#include "hci_cmd.h"
#include "hci_api.h"
#include "hci_main.h"
#include "hci_core_ps.h"

#include "ll_api.h"

/*************************************************************************************************/
/*!
 *  \brief      HCI LE set periodic advertising receive enable command.
 *
 *  \param      syncHandle   Periodic sync handle.
 *  \param      enable       TRUE to enable reports, FALSE to disable reports.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeSetPerAdvRcvEnableCmd(uint16_t syncHandle, uint8_t enable)
{
  LlSetPeriodicAdvRcvEnable(syncHandle, enable);
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE periodic advertising sync transfer command.
 *
 *  \param      connHandle   Connection handle.
 *  \param      serviceData  Service data provided by the host.
 *  \param      syncHandle   Periodic sync handle.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLePerAdvSyncTrsfCmd(uint16_t connHandle, uint16_t serviceData, uint16_t syncHandle)
{
  hciLePerAdvSyncTrsfCmdCmplEvt_t evt;

  evt.hdr.param = connHandle;
  evt.hdr.status = LlPeriodicAdvSyncTransfer(connHandle, serviceData, syncHandle);
  evt.hdr.event = HCI_LE_PER_ADV_SYNC_TRSF_CMD_CMPL_CBACK_EVT;

  evt.handle = connHandle;
  evt.status = evt.hdr.status;

  hciCoreEvtSendIntEvt((uint8_t *) &evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE set periodic advertising set info transfer command.
 *
 *  \param      connHandle   Connection handle.
 *  \param      serviceData  Service data provided by the host.
 *  \param      advHandle    Handle to identify an advertising set.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLePerAdvSetInfoTrsfCmd(uint16_t connHandle, uint16_t serviceData, uint8_t advHandle)
{
  hciLePerAdvSetInfoTrsfCmdCmplEvt_t evt;

  evt.hdr.param = connHandle;
  evt.hdr.status = LlPeriodicAdvSetInfoTransfer(connHandle, serviceData, advHandle);
  evt.hdr.event = HCI_LE_PER_ADV_SET_INFO_TRSF_CMD_CMPL_CBACK_EVT;

  evt.handle = connHandle;
  evt.status = evt.hdr.status;

  hciCoreEvtSendIntEvt((uint8_t *) &evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE set periodic advertising sync transfer parameters command.
 *
 *  \param      connHandle   Connection handle.
 *  \param      mode         Periodic sync advertising sync transfer mode.
 *  \param      skip         The number of periodic advertising packets that can be skipped after
 *                           a successful receive.
 *  \param      syncTimeout  Synchronization timeout for the periodic advertising.
 *  \param      cteType      Constant tone extension type(Used in AoD/AoA).
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeSetPerAdvSyncTrsfParamsCmd(uint16_t connHandle, uint8_t mode, uint16_t skip,
                                     uint16_t syncTimeout, uint8_t cteType)
{
  LlSetPeriodicAdvSyncTransParams(connHandle, mode, skip, syncTimeout, cteType);
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE set default periodic advertising sync transfer parameters command.
 *
 *  \param      mode         Periodic sync advertising sync transfer mode.
 *  \param      skip         The number of periodic advertising packets that can be skipped after
 *                           a successful receive.
 *  \param      syncTimeout  Synchronization timeout for the periodic advertising.
 *  \param      cteType      Constant tone extension type(Used in AoD/AoA).
 *
 *  \return     None.
*/
/*************************************************************************************************/
void HciLeSetDefaultPerAdvSyncTrsfParamsCmd(uint8_t mode, uint16_t skip, uint16_t syncTimeout,
                                           uint8_t cteType)
{
  LlSetDefaultPeriodicAdvSyncTransParams(mode, skip, syncTimeout, cteType);
}
