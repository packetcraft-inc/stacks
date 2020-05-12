/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  HCI Advertising Extensions (AE) command module for master.
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

#include "ll_api.h"

/*************************************************************************************************/
/*!
 *  \brief      HCI LE set extended scanning parameters command.
 *
 *  \param      ownAddrType     Address type used by this device.
 *  \param      scanFiltPolicy  Scan filter policy.
 *  \param      scanPhys        Scanning PHYs.
 *  \param      pScanParam      Scanning parameter array.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeSetExtScanParamCmd(uint8_t ownAddrType, uint8_t scanFiltPolicy, uint8_t scanPhys,
                             hciExtScanParam_t *pScanParam)
{
  uint8_t status;

  status = LlSetExtScanParam(ownAddrType, scanFiltPolicy, scanPhys, (LlExtScanParam_t *)pScanParam);
  (void)status;

  WSF_ASSERT(status == LL_SUCCESS);
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE extended scan enable command.
 *
 *  \param      enable          Set to TRUE to enable scanning, FALSE to disable scanning.
 *  \param      filterDup       Set to TRUE to filter duplicates.
 *  \param      duration        Duration.
 *  \param      period          Period.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeExtScanEnableCmd(uint8_t enable, uint8_t filterDup, uint16_t duration, uint16_t period)
{
  LlExtScanEnable(enable, filterDup, duration, period);
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE extended create connection command.
 *
 *  \param      pInitParam      Initiating parameters.
 *  \param      pScanParam      Initiating scan parameters.
 *  \param      pConnSpec       Connection specification.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeExtCreateConnCmd(hciExtInitParam_t *pInitParam, hciExtInitScanParam_t *pScanParam,
                           hciConnSpec_t *pConnSpec)
{
  uint8_t status;

  status = LlExtCreateConn((LlExtInitParam_t *)pInitParam, (LlExtInitScanParam_t *)pScanParam,
                           (LlConnSpec_t *)pConnSpec);
  (void)status;

  WSF_ASSERT(status == LL_SUCCESS);
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE periodic advertising create sync command.
 *
 *  \param      options         Options.
 *  \param      advSid          Advertising SID.
 *  \param      advAddrType     Advertiser address type.
 *  \param      pAdvAddr        Advertiser address.
 *  \param      skip            Number of periodic advertising packets that can be skipped after
 *                              successful receive.
 *  \param      syncTimeout     Synchronization timeout.
 *  \param      unused          Reserved for future use (must be zero).
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLePerAdvCreateSyncCmd(uint8_t options, uint8_t advSid, uint8_t advAddrType,
                              uint8_t *pAdvAddr, uint16_t skip, uint16_t syncTimeout, uint8_t unused)
{
  LlPerAdvCreateSyncCmd_t param;
  uint8_t status;

  param.options = options;
  param.advSID = advSid;
  param.advAddrType = advAddrType;
  param.pAdvAddr = pAdvAddr;
  param.skip = skip;
  param.syncTimeOut = syncTimeout;

  status = LlPeriodicAdvCreateSync(&param);
  (void)status;

  WSF_ASSERT(status == LL_SUCCESS);
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE periodic advertising create sync cancel command.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLePerAdvCreateSyncCancelCmd(void)
{
  uint8_t status;

  status = LlPeriodicAdvCreateSyncCancel();
  (void)status;

  WSF_ASSERT(status == LL_SUCCESS);
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE periodic advertising terminate sync command.
 *
 *  \param      syncHandle      Sync handle.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLePerAdvTerminateSyncCmd(uint16_t syncHandle)
{
  uint8_t status;

  status = LlPeriodicAdvTerminateSync(syncHandle);
  (void)status;

  WSF_ASSERT(status == LL_SUCCESS);
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE add device to periodic advertiser list command.
 *
 *  \param      advAddrType     Advertiser address type.
 *  \param      pAdvAddr        Advertiser address.
 *  \param      advSid          Advertising SID.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeAddDeviceToPerAdvListCmd(uint8_t advAddrType, uint8_t *pAdvAddr, uint8_t advSid)
{
  LlDevicePerAdvList_t param;
  uint8_t status;

  param.advAddrType = advAddrType;
  param.pAdvAddr = pAdvAddr;
  param.advSID = advSid;

  status = LlAddDeviceToPeriodicAdvList(&param);
  (void)status;

  WSF_ASSERT(status == LL_SUCCESS);
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE remove device from periodic advertiser list command.
 *
 *  \param      advAddrType     Advertiser address type.
 *  \param      pAdvAddr        Advertiser address.
 *  \param      advSid          Advertising SID.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeRemoveDeviceFromPerAdvListCmd(uint8_t advAddrType, uint8_t *pAdvAddr, uint8_t advSid)
{
  LlDevicePerAdvList_t param;
  uint8_t status;

  param.advAddrType = advAddrType;
  param.pAdvAddr = pAdvAddr;
  param.advSID = advSid;

  status = LlRemoveDeviceFromPeriodicAdvList(&param);
  (void)status;

  WSF_ASSERT(status == LL_SUCCESS);
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE clear periodic advertiser list command.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeClearPerAdvListCmd(void)
{
  LlClearPeriodicAdvList();
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE read periodic advertiser size command.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeReadPerAdvListSizeCmd(void)
{
  /* unused */
}
