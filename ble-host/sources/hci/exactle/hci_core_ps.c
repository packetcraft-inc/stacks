/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  HCI core platform-specific module single-chip.
 *
 *  Copyright (c) 2009-2018 Arm Ltd. All Rights Reserved.
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
 *
 *  This module implements core platform-dependent HCI features for transmit data path, receive
 *  data path, the “optimization” API, and the main event handler. This module contains separate
 *  implementations for dual chip and single chip.
 */
/*************************************************************************************************/

#include <string.h>
#include "wsf_types.h"
#include "wsf_msg.h"
#include "wsf_trace.h"
#include "util/bda.h"
#include "util/bstream.h"
#include "hci_core.h"
#include "hci_tr.h"
#include "hci_cmd.h"
#include "hci_evt.h"
#include "hci_api.h"
#include "hci_main.h"
#include "hci_core_ps.h"

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/* HCI event structure length table, indexed by LL callback event value */
static const uint16_t hciEvtCbackLen[] =
{
  sizeof(LlHwErrorInd_t),                              /* LL_ERROR_IND */
  /* --- Core Spec 4.0 --- */
  sizeof(wsfMsgHdr_t),                                 /* LL_RESET_CNF */
  sizeof(LlAdvReportInd_t),                            /* LL_ADV_REPORT_IND */
  sizeof(wsfMsgHdr_t),                                 /* LL_ADV_ENABLE_CNF */
  sizeof(wsfMsgHdr_t),                                 /* LL_SCAN_ENABLE_CNF */
  sizeof(LlConnInd_t),                                 /* LL_CONN_IND */
  sizeof(LlDisconnectInd_t),                           /* LL_DISCONNECT_IND */
  sizeof(LlConnUpdateInd_t),                           /* LL_CONN_UPDATE_IND */
  sizeof(LlCreateConnCancelCnf_t),                     /* LL_CREATE_CONN_CANCEL_CNF */
  sizeof(LlReadRemoteVerInfoCnf_t),                    /* LL_READ_REMOTE_VER_INFO_CNF */
  sizeof(LlReadRemoteFeatCnf_t),                       /* LL_READ_REMOTE_FEAT_CNF */
  sizeof(LlEncChangeInd_t),                            /* LL_ENC_CHANGE_IND */
  sizeof(LlEncKeyRefreshInd_t),                        /* LL_ENC_KEY_REFRESH_IND */
  sizeof(LlLtkReqInd_t),                               /* LL_LTK_REQ_IND */
  sizeof(LlLtkReqNegReplyCnf_t),                       /* LL_LTK_REQ_NEG_REPLY_CNF */
  sizeof(LlLtkReqReplyCnf_t),                          /* LL_LTK_REQ_REPLY_CNF */
  /* --- Core Spec 4.2 --- */
  sizeof(LlRemConnParamInd_t),                         /* LL_REM_CONN_PARAM_IND */
  sizeof(LlAuthPayloadTimeoutInd_t),                   /* LL_AUTH_PAYLOAD_TIMEOUT_IND */
  sizeof(LlDataLenChangeInd_t),                        /* LL_DATA_LEN_CHANGE_IND */
  sizeof(LlReadLocalP256PubKeyInd_t),                  /* LL_READ_LOCAL_P256_PUB_KEY_CMPL_IND */
  sizeof(LlGenerateDhKeyInd_t),                        /* LL_GENERATE_DHKEY_CMPL_IND */
  sizeof(LlScanReportInd_t),                           /* LL_SCAN_REPORT_IND */
  /* --- Core Spec 5.0 --- */
  sizeof(LlPhyUpdateInd_t),                            /* LL_PHY_UPDATE_IND */
  sizeof(LlExtAdvReportInd_t),                         /* LL_EXT_ADV_REPORT_IND */
  sizeof(LlExtScanEnableCnf_t),                        /* LL_EXT_SCAN_ENABLE_CNF */
  sizeof(wsfMsgHdr_t),                                 /* LL_SCAN_TIMEOUT_IND */
  sizeof(LlScanReqRcvdInd_t),                          /* LL_SCAN_REQ_RCVD_IND */
  sizeof(LlExtAdvEnableCnf_t),                         /* LL_EXT_ADV_ENABLE_CNF */
  sizeof(LlAdvSetTermInd_t),                           /* LL_ADV_SET_TERM_IND */
  sizeof(LlPerAdvEnableCnf_t),                         /* LL_PER_ADV_ENABLE_CNF */
  sizeof(LlPerAdvSyncEstdCnf_t),                       /* LL_PER_ADV_SYNC_EST_IND */
  sizeof(LlPerAdvSyncLostInd_t),                       /* LL_PER_ADV_SYNC_LOST_IND */
  sizeof(LlPerAdvReportInd_t),                         /* LL_PER_ADV_REPORT_IND */
  sizeof(LlChSelInd_t),                                /* LL_CH_SEL_ALGO_IND */
  /* --- Core Spec 5.1 --- */
  sizeof(wsfMsgHdr_t),                                 /* LL_CONNLESS_IQ_REPORT_IND */
  sizeof(wsfMsgHdr_t),                                 /* LL_CONN_IQ_REPORT_IND */
  sizeof(wsfMsgHdr_t),                                 /* LL_CTE_REQ_FAILED_IND */
  sizeof(LlPerSyncTrsfRcvdInd_t),                      /* LL_PER_SYNC_TRSF_RCVD_IND */
  /* --- Core Spec Milan --- */
  sizeof(LlCisEstInd_t),                               /* LL_CIS_EST_IND */
  sizeof(LlCisReqInd_t),                               /* LL_CIS_REQ_IND */
  sizeof(LlCreateBigCnf_t),                            /* LL_CREATE_BIG_CNF */
  sizeof(LlTerminateBigInd_t),                         /* LL_TERM_BIG_IND */
  sizeof(LlBigTermSyncCnf_t),                          /* LL_BIG_TERM_SYNC_CNF */
  sizeof(LlBigSyncEstInd_t),                           /* LL_BIG_SYNC_EST_IND */
  sizeof(LlBigSyncLostInd_t),                          /* LL_BIG_SYNC_LOST_IND */
  sizeof(LlPeerScaCnf_t),                              /* LL_REQ_PEER_SCA_IND */
  sizeof(LlPowerReportInd_t),                          /* LL_TX_POWER_REPORTING_IND */
  sizeof(LlPathLossThresholdEvt_t),                    /* LL_PATH_LOSS_REPORTING_IND */
  sizeof(LlIsoEventCmplInd_t),                         /* LL_ISO_EVT_CMPL_IND */
  sizeof(LlBigInfoAdvRptInd_t)                         /* LL_BIG_INFO_ADV_REPORT_IND */
};

/*************************************************************************************************/
/*!
 *  \brief  Return size of an LL callback event.
 *
 *  \param  event   LL event.
 *
 *  \return Size of LL event.
 */
/*************************************************************************************************/
size_t hciCoreSizeOfEvt(uint8_t event)
{
  return (event > LL_BIG_INFO_ADV_REPORT_IND) ? sizeof(wsfMsgHdr_t) : hciEvtCbackLen[event];
}

/*************************************************************************************************/
/*!
 *  \brief  HCI core initialization.
 *
 *  \return None.
 */
/*************************************************************************************************/
void hciCoreInit(void)
{
  LlEvtRegister(hciCoreEvtProcessLlEvt);
  LlAclRegister(hciCoreNumCmplPkts, hciCoreAclRecvPending);

  /* synchronize with LL */
  hciCoreCb.numBufs = LlGetAclTxBufs();
  hciCoreCb.availBufs = LlGetAclTxBufs();
  hciCoreCb.bufSize = LlGetAclMaxSize();
}

/*************************************************************************************************/
/*!
 *  \brief  Process received HCI events.
 *
 *  \param  pEvt    Buffer containing LL event.
 *
 *  \return Always TRUE.
 */
/*************************************************************************************************/
bool_t hciCoreEvtProcessLlEvt(LlEvt_t *pEvt)
{
  LlEvt_t *pMsg;
  size_t  msgLen = hciCoreSizeOfEvt(pEvt->hdr.event);
  size_t  reportLen;

  /* find out report length */
  switch (pEvt->hdr.event)
  {
  case LL_ADV_REPORT_IND:
    reportLen = pEvt->advReportInd.len;
    break;

  case LL_EXT_ADV_REPORT_IND:
    reportLen = pEvt->extAdvReportInd.len;
    break;

  case LL_PER_ADV_REPORT_IND:
    reportLen = pEvt->perAdvReportInd.len;
    break;

  default:
    reportLen = 0;
    break;
  }

  if ((pMsg = WsfMsgAlloc(msgLen + reportLen)) != NULL)
  {
    /* copy event to message buffer */
    memcpy(pMsg, pEvt, msgLen);

    /* copy report to message buffer */
    switch (pEvt->hdr.event)
    {
    case LL_ADV_REPORT_IND:
      pMsg->advReportInd.pData = (uint8_t *) pMsg + msgLen;
      memcpy(pMsg->advReportInd.pData, pEvt->advReportInd.pData, reportLen);
      break;

    case LL_EXT_ADV_REPORT_IND:
      pMsg->extAdvReportInd.pData = (uint8_t *) pMsg + msgLen;
      memcpy((uint8_t *) pMsg->extAdvReportInd.pData, pEvt->extAdvReportInd.pData, reportLen);
      break;

    case LL_PER_ADV_REPORT_IND:
      pMsg->perAdvReportInd.pData = (uint8_t *) pMsg + msgLen;
      memcpy((uint8_t *) pMsg->perAdvReportInd.pData, pEvt->perAdvReportInd.pData, reportLen);
      break;

    default:
      break;
    }

    WsfMsgEnq(&hciCb.rxQueue, HCI_EVT_TYPE, pMsg);
    WsfSetEvent(hciCb.handlerId, HCI_EVT_RX);
  }

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief  LL ACL receive buffer pending handler.
 *
 *  \param  handle    Connection handle.
 *  \param  numBufs   Number of buffers completed.
 *
 *  \return None.
 */
/*************************************************************************************************/
void hciCoreAclRecvPending(uint16_t handle, uint8_t numBufs)
{
  uint8_t *pBuf;

  while ((pBuf = LlRecvAclData()) != NULL)
  {
    WsfMsgEnq(&hciCb.rxQueue, HCI_ACL_TYPE, pBuf);
    LlRecvAclDataComplete(1);
  }

  WsfSetEvent(hciCb.handlerId, HCI_EVT_RX);
}

/*************************************************************************************************/
/*!
 *  \brief  Handle an HCI Number of Completed Packets event.
 *
 *  \param  pMsg    Message containing the HCI Number of Completed Packets event.
 *
 *  \return None.
 */
/*************************************************************************************************/
void hciCoreNumCmplPkts(uint16_t handle, uint8_t numBufs)
{
  hciCoreConn_t   *pConn;

  if ((pConn = hciCoreConnByHandle(handle)) != NULL)
  {
    /* decrement outstanding buffer count to controller */
    pConn->outBufs -= (uint8_t) numBufs;

    /* decrement queued buffer count for this connection */
    pConn->queuedBufs -= (uint8_t) numBufs;

    /* call flow control callback */
    if (pConn->flowDisabled && pConn->queuedBufs <=  hciCoreCb.aclQueueLo)
    {
      pConn->flowDisabled = FALSE;
      (*hciCb.flowCback)(handle, FALSE);
    }

    /* service TX data path */
    hciCoreTxReady(numBufs);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Send an event generated internally by HCI APIs.
 *
 *  \param  pEvt    Buffer containing HCI event.
 *  \param  evtSize Size of HCI event.
 *
 *  \return None.
 */
/*************************************************************************************************/
void hciCoreEvtSendIntEvt(uint8_t *pEvt, uint8_t evtSize)
{
  hciEvt_t *pMsg;

  if ((pMsg = WsfMsgAlloc(evtSize)) != NULL)
  {
    /* copy event to message buffer */
    memcpy(pMsg, pEvt, evtSize);

    /* event generated internally by HCI APIs */
    pMsg->hdr.event |= HCI_EVT_INT_TYPE;

    WsfMsgEnq(&hciCb.rxQueue, HCI_EVT_TYPE, pMsg);
    WsfSetEvent(hciCb.handlerId, HCI_EVT_RX);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  WSF event handler for core HCI.
 *
 *  \param  event   WSF event mask.
 *  \param  pMsg    WSF message.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciCoreHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
  uint8_t         *pBuf;
  wsfHandlerId_t  handlerId;

  /* Handle events */
  if (event & HCI_EVT_RX)
  {
    /* Process rx queue */
    while ((pBuf = WsfMsgDeq(&hciCb.rxQueue, &handlerId)) != NULL)
    {
      /* Handle incoming HCI events */
      if (handlerId == HCI_EVT_TYPE)
      {
        /* Parse/process events */
        hciEvtProcessMsg(pBuf);

        /* Free buffer */
        WsfMsgFree(pBuf);
      }
      /* Handle ACL data */
      else if (handlerId == HCI_ACL_TYPE)
      {
        /* Reassemble */
        if ((pBuf = hciCoreAclReassembly(pBuf)) != NULL)
        {
          /* Call ACL callback; client will free buffer */
          hciCb.aclCback(pBuf);
        }
      }
      /* Handle ISO data */
      else
      {
        if (hciCb.isoCback)
        {
          /* Call ISO callback; client will free buffer */
          hciCb.isoCback(pBuf);
        }
        else
        {
          /* free buffer */
          WsfMsgFree(pBuf);
        }
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Return a pointer to the BD address of this device.
 *
 *  \return Pointer to the BD address.
 */
/*************************************************************************************************/
uint8_t *HciGetBdAddr(void)
{
  return hciCoreCb.bdAddr;
}

/*************************************************************************************************/
/*!
 *  \brief  Return the white list size.
 *
 *  \return White list size.
 */
/*************************************************************************************************/
uint8_t HciGetWhiteListSize(void)
{
  return LlGetWhitelistSize();
}

/*************************************************************************************************/
/*!
 *  \brief  Return the advertising transmit power.
 *
 *  \return Advertising transmit power.
 */
/*************************************************************************************************/
int8_t HciGetAdvTxPwr(void)
{
  int8_t advTxPwr = 0;
  LlGetAdvTxPower(&advTxPwr);
  return advTxPwr;
}

/*************************************************************************************************/
/*!
 *  \brief  Return the ACL buffer size supported by the controller.
 *
 *  \return ACL buffer size.
 */
/*************************************************************************************************/
uint16_t HciGetBufSize(void)
{
  return hciCoreCb.bufSize;
}

/*************************************************************************************************/
/*!
 *  \brief  Return the number of ACL buffers supported by the controller.
 *
 *  \return Number of ACL buffers.
 */
/*************************************************************************************************/
uint8_t HciGetNumBufs(void)
{
  return hciCoreCb.numBufs;
}

/*************************************************************************************************/
/*!
 *  \brief  Return the states supported by the controller.
 *
 *  \return Pointer to the supported states array.
 */
/*************************************************************************************************/
uint8_t *HciGetSupStates(void)
{
  static uint8_t supStates[8];

  LlGetSupStates(supStates);

  return supStates;
}

/*************************************************************************************************/
/*!
 *  \brief  Return the LE supported features supported by the controller.
 *
 *  \return Supported features.
 */
/*************************************************************************************************/
uint64_t HciGetLeSupFeat(void)
{
  uint64_t supFeat;
  uint8_t  feat[HCI_LE_STATES_LEN];

  LlGetFeatures(feat);

  BYTES_TO_UINT64(supFeat, feat);

  return supFeat;
}

/*************************************************************************************************/
/*!
 *  \brief  Return the LE supported features supported by the controller.
 *
 *  \return Supported features.
 */
/*************************************************************************************************/
uint32_t HciGetLeSupFeat32(void)
{
  uint32_t supFeat;
  uint8_t  feat[HCI_LE_STATES_LEN];

  LlGetFeatures(feat);

  BYTES_TO_UINT32(supFeat, feat);

  return supFeat;
}

/*************************************************************************************************/
/*!
 *  \brief  Get the maximum reassembled RX ACL packet length.
 *
 *  \return ACL packet length.
 */
/*************************************************************************************************/
uint16_t HciGetMaxRxAclLen(void)
{
  return hciCoreCb.maxRxAclLen;
}

/*************************************************************************************************/
/*!
 *  \brief  Return the resolving list size.
 *
 *  \return resolving list size.
 */
/*************************************************************************************************/
uint8_t HciGetResolvingListSize(void)
{
  return hciCoreCb.resListSize;
}

/*************************************************************************************************/
/*!
*  \brief  Whether LL Privacy is supported.
*
*  \return TRUE if LL Privacy is supported. FALSE, otherwise.
*/
/*************************************************************************************************/
bool_t HciLlPrivacySupported(void)
{
  return (hciCoreCb.resListSize > 0) ? TRUE : FALSE;
}

/*************************************************************************************************/
/*!
*  \brief  Get the maximum advertisement (or scan response) data length supported by the Controller.
*
*  \return Maximum advertisement data length.
*/
/*************************************************************************************************/
uint16_t HciGetMaxAdvDataLen(void)
{
  return hciCoreCb.maxAdvDataLen;
}

/*************************************************************************************************/
/*!
*  \brief  Get the maximum number of advertising sets supported by the Controller.
*
*  \return Maximum number of advertising sets.
*/
/*************************************************************************************************/
uint8_t HciGetNumSupAdvSets(void)
{
  return hciCoreCb.numSupAdvSets;
}

/*************************************************************************************************/
/*!
*  \brief  Whether LE Advertising Extensions is supported.
*
*  \return TRUE if LE Advertising Extensions is supported. FALSE, otherwise.
*/
/*************************************************************************************************/
bool_t HciLeAdvExtSupported(void)
{
  return (hciCoreCb.numSupAdvSets > 0) ? TRUE : FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief  Return the periodic advertising list size.
 *
 *  \return periodic advertising list size.
 */
/*************************************************************************************************/
uint8_t HciGetPerAdvListSize(void)
{
  return hciCoreCb.perAdvListSize;
}

/*************************************************************************************************/
/*!
 *  \brief  Return a pointer to the local version information.
 *
 *  \return Pointer to the local version information.
 */
/*************************************************************************************************/
hciLocalVerInfo_t *HciGetLocalVerInfo(void)
{
  return &hciCoreCb.locVerInfo;
}
