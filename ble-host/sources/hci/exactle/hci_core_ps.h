/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  HCI core platform-specific interfaces for for single-chip.
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
#ifndef HCI_CORE_PS_H
#define HCI_CORE_PS_H

#include "ll_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/* HCI event generated internally by HCI APIs */
#define HCI_EVT_INT_TYPE          (1 << 7)

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

void hciCoreNumCmplPkts(uint16_t handle, uint8_t numBufs);
bool_t hciCoreEvtProcessLlEvt(LlEvt_t *pEvt);
void hciCoreAclRecvPending(uint16_t handle, uint8_t numBufs);
void hciCoreEvtSendIntEvt(uint8_t *pEvt, uint8_t evtSize);

#ifdef __cplusplus
};
#endif

#endif /* HCI_CORE_PS_H */
