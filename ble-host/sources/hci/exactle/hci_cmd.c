/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  HCI command module.
 *
 *  Copyright (c) 2009-2019 Arm Ltd. All Rights Reserved.
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
 *
 *  This module builds and translates HCI command data structures. It also implements command
 *  flow control.
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
 *  \brief  HCI disconnect command.
 */
/*************************************************************************************************/
void HciDisconnectCmd(uint16_t handle, uint8_t reason)
{
  LlDisconnect(handle, reason);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE add device white list command.
 */
/*************************************************************************************************/
void HciLeAddDevWhiteListCmd(uint8_t addrType, uint8_t *pAddr)
{
  LlAddDeviceToWhitelist(addrType, pAddr);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE clear white list command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeClearWhiteListCmd(void)
{
  LlClearWhitelist();
}

/*************************************************************************************************/
/*!
 *  \brief  HCI connection update command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeConnUpdateCmd(uint16_t handle, hciConnSpec_t *pConnSpec)
{
  LlConnUpdate(handle, (LlConnSpec_t *)pConnSpec);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE random command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeRandCmd(void)
{
  hciLeRandCmdCmplEvt_t evt;

  evt.hdr.param = 0;
  evt.hdr.event = HCI_LE_RAND_CMD_CMPL_CBACK_EVT;
  evt.hdr.status = HCI_SUCCESS;

  evt.status = HCI_SUCCESS;

  LlGetRandNum(evt.randNum);

  hciCoreEvtSendIntEvt((uint8_t *)&evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read advertising TX power command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadAdvTXPowerCmd(void)
{
  /* unused */
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read buffer size command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadBufSizeCmd(void)
{
  /* unused */
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read channel map command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadChanMapCmd(uint16_t handle)
{
  hciReadChanMapCmdCmplEvt_t evt;

  evt.hdr.param = handle;
  evt.hdr.event = HCI_LE_READ_CHAN_MAP_CMD_CMPL_CBACK_EVT;
  evt.hdr.status = LlGetChannelMap(handle, evt.chanMap);

  evt.handle = handle;
  evt.status = evt.hdr.status;

  hciCoreEvtSendIntEvt((uint8_t *)&evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read local supported feautre command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadLocalSupFeatCmd(void)
{
  /* unused */
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read remote feature command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadRemoteFeatCmd(uint16_t handle)
{
  LlReadRemoteFeat(handle);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read supported states command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadSupStatesCmd(void)
{
  /* unused */
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read white list size command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadWhiteListSizeCmd(void)
{
  /* unused */
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE remove device white list command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeRemoveDevWhiteListCmd(uint8_t addrType, uint8_t *pAddr)
{
  LlRemoveDeviceFromWhitelist(addrType, pAddr);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE set advanced enable command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetAdvEnableCmd(uint8_t enable)
{
  LlAdvEnable(enable);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE set advertising data command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetAdvDataCmd(uint8_t len, uint8_t *pData)
{
  LlSetAdvData(len, pData);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE set advertising parameters command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetAdvParamCmd(uint16_t advIntervalMin, uint16_t advIntervalMax, uint8_t advType,
                         uint8_t ownAddrType, uint8_t peerAddrType, uint8_t *pPeerAddr,
                         uint8_t advChanMap, uint8_t advFiltPolicy)
{
  LlSetAdvParam(advIntervalMin,
                advIntervalMax,
                advType,
                ownAddrType,
                peerAddrType,
                pPeerAddr,
                advChanMap,
                advFiltPolicy);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE set event mask command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetEventMaskCmd(uint8_t *pLeEventMask)
{
  /* unused */
}

/*************************************************************************************************/
/*!
 *  \brief  HCI set host channel class command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetHostChanClassCmd(uint8_t *pChanMap)
{
  /* unused */
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE set random address command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetRandAddrCmd(uint8_t *pAddr)
{
  wsfMsgHdr_t evt;

  evt.param = 0;
  evt.event = HCI_LE_SET_RAND_ADDR_CMD_CMPL_CBACK_EVT;
  evt.status = LlSetRandAddr(pAddr);

  hciCoreEvtSendIntEvt((uint8_t *) &evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE set scan response data.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetScanRespDataCmd(uint8_t len, uint8_t *pData)
{
  LlSetScanRespData(len, pData);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI read BD address command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadBdAddrCmd(void)
{
  /* not used */
}

/*************************************************************************************************/
/*!
 *  \brief  HCI read buffer size command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadBufSizeCmd(void)
{
  /* not used */
}

/*************************************************************************************************/
/*!
 *  \brief  HCI read local supported feature command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadLocalSupFeatCmd(void)
{
  /* not used */
}

/*************************************************************************************************/
/*!
 *  \brief  HCI read local version info command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadLocalVerInfoCmd(void)
{
  /* not used */
}

/*************************************************************************************************/
/*!
 *  \brief  HCI read remote version info command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadRemoteVerInfoCmd(uint16_t handle)
{
  LlReadRemoteVerInfo(handle);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI read RSSI command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadRssiCmd(uint16_t handle)
{
  hciReadRssiCmdCmplEvt_t evt;

  evt.hdr.param = handle;
  evt.hdr.event = HCI_READ_RSSI_CMD_CMPL_CBACK_EVT;
  evt.hdr.status = LlGetRssi(handle, &evt.rssi);

  evt.handle = handle;
  evt.status = evt.hdr.status;

  hciCoreEvtSendIntEvt((uint8_t *)&evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
 *  \brief  HCI read Tx power level command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadTxPwrLvlCmd(uint16_t handle, uint8_t type)
{
  hciReadTxPwrLvlCmdCmplEvt_t evt;

  evt.hdr.param = handle;
  evt.hdr.event = HCI_READ_TX_PWR_LVL_CMD_CMPL_CBACK_EVT;
  evt.hdr.status = LlGetTxPowerLevel(handle, type, &evt.pwrLvl);

  evt.handle = handle;
  evt.status = evt.hdr.status;

  hciCoreEvtSendIntEvt((uint8_t *)&evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
 *  \brief  HCI reset command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciResetCmd(void)
{
  LlReset();
}

/*************************************************************************************************/
/*!
 *  \brief  HCI set event mask command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciSetEventMaskCmd(uint8_t *pEventMask)
{
  /* unused */
}

/*************************************************************************************************/
/*!
 *  \brief  HCI add device to resolving list command.
 *
 *  \param  peerAddrType        Peer identity address type.
 *  \param  pPeerIdentityAddr   Peer identity address.
 *  \param  pPeerIrk            Peer IRK.
 *  \param  pLocalIrk           Local IRK.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeAddDeviceToResolvingListCmd(uint8_t peerAddrType, const uint8_t *pPeerIdentityAddr,
                                      const uint8_t *pPeerIrk, const uint8_t *pLocalIrk)
{
  hciLeAddDevToResListCmdCmplEvt_t evt;

  evt.hdr.param = 0;
  evt.hdr.event = HCI_LE_ADD_DEV_TO_RES_LIST_CMD_CMPL_CBACK_EVT;
  evt.hdr.status = LlAddDeviceToResolvingList(peerAddrType, pPeerIdentityAddr, pPeerIrk, pLocalIrk);

  evt.status = evt.hdr.status;

  hciCoreEvtSendIntEvt((uint8_t *)&evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
 *  \brief  HCI remove device from resolving list command.
 *
 *  \param  peerAddrType        Peer identity address type.
 *  \param  pPeerIdentityAddr   Peer identity address.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeRemoveDeviceFromResolvingList(uint8_t peerAddrType, const uint8_t *pPeerIdentityAddr)
{
  hciLeRemDevFromResListCmdCmplEvt_t evt;

  evt.hdr.param = 0;
  evt.hdr.event = HCI_LE_REM_DEV_FROM_RES_LIST_CMD_CMPL_CBACK_EVT;
  evt.hdr.status = LlRemoveDeviceFromResolvingList(peerAddrType, pPeerIdentityAddr);

  evt.status = evt.hdr.status;

  hciCoreEvtSendIntEvt((uint8_t *)&evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
 *  \brief  HCI clear resolving list command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeClearResolvingList(void)
{
  hciLeClearResListCmdCmplEvt_t evt;

  evt.hdr.param = 0;
  evt.hdr.event = HCI_LE_CLEAR_RES_LIST_CMD_CMPL_CBACK_EVT;
  evt.hdr.status = LlClearResolvingList();

  evt.status = evt.hdr.status;

  hciCoreEvtSendIntEvt((uint8_t *)&evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
 *  \brief  HCI read resolving list command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadResolvingListSize(void)
{
  /* unused */
}

/*************************************************************************************************/
/*!
 *  \brief  HCI read peer resolvable address command.
 *
 *  \param  addrType        Peer identity address type.
 *  \param  pIdentityAddr   Peer identity address.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadPeerResolvableAddr(uint8_t addrType, const uint8_t *pIdentityAddr)
{
  hciLeReadPeerResAddrCmdCmplEvt_t evt;

  evt.hdr.param = 0;
  evt.hdr.event = HCI_LE_READ_PEER_RES_ADDR_CMD_CMPL_CBACK_EVT;
  evt.hdr.status = LlReadPeerResolvableAddr(addrType, pIdentityAddr, evt.peerRpa);

  evt.status = evt.hdr.status;

  hciCoreEvtSendIntEvt((uint8_t *)&evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
 *  \brief  HCI read local resolvable address command.
 *
 *  \param  addrType        Peer identity address type.
 *  \param  pIdentityAddr   Peer identity address.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadLocalResolvableAddr(uint8_t addrType, const uint8_t *pIdentityAddr)
{
  hciLeReadLocalResAddrCmdCmplEvt_t evt;

  evt.hdr.param = 0;
  evt.hdr.event = HCI_LE_READ_LOCAL_RES_ADDR_CMD_CMPL_CBACK_EVT;
  evt.hdr.status = LlReadLocalResolvableAddr(addrType, pIdentityAddr, evt.localRpa);

  evt.status = evt.hdr.status;

  hciCoreEvtSendIntEvt((uint8_t *)&evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
 *  \brief  HCI enable or disable address resolution command.
 *
 *  \param  enable          Set to TRUE to enable address resolution or FALSE to disable address
 *                          resolution.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetAddrResolutionEnable(uint8_t enable)
{
  hciLeSetAddrResEnableCmdCmplEvt_t evt;

  evt.hdr.param = 0;
  evt.hdr.event = HCI_LE_SET_ADDR_RES_ENABLE_CMD_CMPL_CBACK_EVT;
  evt.hdr.status = LlSetAddrResolutionEnable(enable);

  evt.status = evt.hdr.status;

  hciCoreEvtSendIntEvt((uint8_t *)&evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
 *  \brief  HCI set resolvable private address timeout command.
 *
 *  \param  rpaTimeout      Timeout measured in seconds.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetResolvablePrivateAddrTimeout(uint16_t rpaTimeout)
{
  LlSetResolvablePrivateAddrTimeout(rpaTimeout);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE set privacy mode command.
 *
 *  \param  peerAddrType    Peer identity address type.
 *  \param  pPeerAddr       Peer identity address.
 *  \param  mode            Privacy mode.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetPrivacyModeCmd(uint8_t addrType, uint8_t *pAddr, uint8_t mode)
{
  LlSetPrivacyMode(addrType, pAddr, mode);
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE request peer SCA command.
 *
 *  \param      handle    Connection handle.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeRequestPeerScaCmd(uint16_t handle)
{
  uint8_t status;

  status = LlRequestPeerSca(handle);
  (void)status;

  WSF_ASSERT(status == LL_SUCCESS);
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE set host feature command.
 *
 *  \param      bitNum    Bit position in the FeatureSet.
 *  \param      bitVal    Enable or disable feature.
 *
 *  \return     None.
 *
 *  \note Set or clear a bit in the feature controlled by the Host in the Link Layer FeatureSet
 *  stored in the Controller.
 */
/*************************************************************************************************/
void HciLeSetHostFeatureCmd(uint8_t bitNum, bool_t bitVal)
{ 
  uint8_t status;

  status = LlSetHostFeatures(bitNum, bitVal);
  (void)status;

  WSF_ASSERT(status == LL_SUCCESS);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI vencor specific command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciVendorSpecificCmd(uint16_t opcode, uint8_t len, uint8_t *pData)
{
  /* not used */
}

/*************************************************************************************************/
/*!
 *  \brief  HCI Remote Connection Parameter Request Reply.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeRemoteConnParamReqReply(uint16_t handle, uint16_t intervalMin, uint16_t intervalMax, uint16_t latency,
                                  uint16_t timeout, uint16_t minCeLen, uint16_t maxCeLen)
{
  LlConnSpec_t connSpec;

  connSpec.connIntervalMax = intervalMax;
  connSpec.connIntervalMin = intervalMin;
  connSpec.connLatency = latency;
  connSpec.maxCeLen = maxCeLen;
  connSpec.minCeLen = minCeLen;
  connSpec.supTimeout = timeout;

  LlRemoteConnParamReqReply(handle, &connSpec);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI Remote Connection Parameter Request Negative Reply.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeRemoteConnParamReqNegReply(uint16_t handle, uint8_t reason)
{
  LlRemoteConnParamReqNegReply(handle, reason);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE Set Data Length.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetDataLen(uint16_t handle, uint16_t txOctets, uint16_t txTime)
{
  LlSetDataLen(handle, txOctets, txTime);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE Read Default Data Length.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadDefDataLen(void)
{
  /* not used */
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE Write Default Data Length.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeWriteDefDataLen(uint16_t suggestedMaxTxOctets, uint16_t suggestedMaxTxTime)
{
  LlWriteDefaultDataLen(suggestedMaxTxOctets, suggestedMaxTxTime);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE Read Local P-256 Public Key.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadLocalP256PubKey(void)
{
  LlGenerateP256KeyPair();
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE Generate DH Key.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeGenerateDHKey(uint8_t *pPubKeyX, uint8_t *pPubKeyY)
{
  uint8_t status = LlGenerateDhKey(pPubKeyX, pPubKeyY);

  if (status != HCI_SUCCESS)
  {
    hciLeGenDhKeyEvt_t evt;

    evt.hdr.event = HCI_LE_GENERATE_DHKEY_CMPL_CBACK_EVT;
    evt.hdr.status = evt.status = status;

    hciCoreEvtSendIntEvt((uint8_t *)&evt, sizeof(evt));
  }
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE Generate DH Key Version 2.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeGenerateDHKeyV2(uint8_t *pPubKeyX, uint8_t *pPubKeyY, uint8_t keyType)
{
  uint8_t status = LlGenerateDhKeyV2(pPubKeyX, pPubKeyY, keyType);

  if (status != HCI_SUCCESS)
  {
    hciLeGenDhKeyEvt_t evt;

    evt.hdr.event = HCI_LE_GENERATE_DHKEY_CMPL_CBACK_EVT;
    evt.hdr.status = evt.status = status;

    hciCoreEvtSendIntEvt((uint8_t *)&evt, sizeof(evt));
  }
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE Read Maximum Data Length.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadMaxDataLen(void)
{
  /* not used */
}

/*************************************************************************************************/
/*!
 *  \brief  HCI read authenticated payload timeout command.
 *
 *  \param  handle    Connection handle.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadAuthPayloadTimeout(uint16_t handle)
{
  /* not used */
}

/*************************************************************************************************/
/*!
 *  \brief  HCI write authenticated payload timeout command.
 *
 *  \param  handle    Connection handle.
 *  \param  timeout   Timeout value.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciWriteAuthPayloadTimeout(uint16_t handle, uint16_t timeout)
{
  hciWriteAuthPayloadToCmdCmplEvt_t evt;

  evt.hdr.status = LlWriteAuthPayloadTimeout(handle, timeout);
  evt.hdr.event = HCI_WRITE_AUTH_PAYLOAD_TO_CMD_CMPL_CBACK_EVT;
  evt.handle = handle;

  evt.status = evt.hdr.status;

  hciCoreEvtSendIntEvt((uint8_t *)&evt, sizeof(evt));
}

/*************************************************************************************************/
/*!
 *  \brief  HCI set event page 2 mask command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciSetEventMaskPage2Cmd(uint8_t *pEventMask)
{
  /* unused */
}

