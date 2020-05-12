/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  HCI vendor specific AE functions for single-chip.
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
#include "util/bstream.h"
#include "hci_core.h"

/*************************************************************************************************/
/*!
 *  \brief  Implement the HCI extended reset sequence.
 *
 *  \param  pMsg    HCI event message from previous command in the sequence.
 *  \param  opcode  HCI event message opcode.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciCoreExtResetSequence(uint8_t *pMsg, uint16_t opcode)
{
  (void) pMsg;
  (void) opcode;

  /* if LE Extended Advertising is supported by Controller and included */
  if ((HciGetLeSupFeat() & HCI_LE_SUP_FEAT_LE_EXT_ADV) &&
      (hciLeSupFeatCfg & HCI_LE_SUP_FEAT_LE_EXT_ADV))
  {
    LlReadMaxAdvDataLen(&hciCoreCb.maxAdvDataLen);
    LlReadNumSupAdvSets(&hciCoreCb.numSupAdvSets);
  }
  else
  {
    hciCoreCb.maxAdvDataLen = 0;
    hciCoreCb.numSupAdvSets = 0;
  }

  /* if LE Periodic Advertising is supported by Controller and included */
  if ((HciGetLeSupFeat() & HCI_LE_SUP_FEAT_LE_PER_ADV) &&
      (hciLeSupFeatCfg & HCI_LE_SUP_FEAT_LE_PER_ADV))
  {
    LlReadPeriodicAdvListSize(&hciCoreCb.perAdvListSize);
  }
  else
  {
    hciCoreCb.perAdvListSize = 0;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Vendor-specific controller AE initialization function.
 *
 *  \param  param    Vendor-specific parameter.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciVsAeInit(uint8_t param)
{
  hciCoreCb.extResetSeq = hciCoreExtResetSequence;
}
