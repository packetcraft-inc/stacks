/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  HCI event module.
 *
 *  Copyright (c) 2009-2018 Arm Ltd.
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
 *  This module implements parsing and translation of HCI event data structures. This module
 *  contains separate implementations for dual chip and single chip.
 */
/*************************************************************************************************/

#include <string.h>
#include "wsf_types.h"
#include "wsf_buf.h"
#include "wsf_msg.h"
#include "wsf_trace.h"
#include "util/bstream.h"
#include "hci_api.h"
#include "hci_main.h"
#include "hci_evt.h"
#include "hci_cmd.h"
#include "hci_core.h"
#include "hci_core_ps.h"

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! LL event to HCI callback event lookup table. */
static const uint8_t hciEvtLookup[] =
{
  HCI_HW_ERROR_CBACK_EVT,                              /* LL_ERROR_IND */
  /* --- Core Spec 4.0 --- */
  HCI_RESET_SEQ_CMPL_CBACK_EVT,                        /* LL_RESET_CNF */
  HCI_LE_ADV_REPORT_CBACK_EVT,                         /* LL_ADV_REPORT_IND */
  HCI_LE_ADV_ENABLE_CMD_CMPL_CBACK_EVT,                /* LL_ADV_ENABLE_CNF */
  HCI_LE_SCAN_ENABLE_CMD_CMPL_CBACK_EVT,               /* LL_SCAN_ENABLE_CNF */
  HCI_LE_CONN_CMPL_CBACK_EVT,                          /* LL_CONN_IND */
  HCI_DISCONNECT_CMPL_CBACK_EVT,                       /* LL_DISCONNECT_IND */
  HCI_LE_CONN_UPDATE_CMPL_CBACK_EVT,                   /* LL_CONN_UPDATE_IND */
  HCI_LE_CREATE_CONN_CANCEL_CMD_CMPL_CBACK_EVT,        /* LL_CREATE_CONN_CANCEL_CNF */
  HCI_READ_REMOTE_VER_INFO_CMPL_CBACK_EVT,             /* LL_READ_REMOTE_VER_INFO_CNF */
  HCI_LE_READ_REMOTE_FEAT_CMPL_CBACK_EVT,              /* LL_READ_REMOTE_FEAT_CNF */
  HCI_ENC_CHANGE_CBACK_EVT,                            /* LL_ENC_CHANGE_IND */
  HCI_ENC_KEY_REFRESH_CMPL_CBACK_EVT,                  /* LL_ENC_KEY_REFRESH_IND */
  HCI_LE_LTK_REQ_CBACK_EVT,                            /* LL_LTK_REQ_IND */
  HCI_LE_LTK_REQ_NEG_REPL_CMD_CMPL_CBACK_EVT,          /* LL_LTK_REQ_NEG_REPLY_CNF */
  HCI_LE_LTK_REQ_REPL_CMD_CMPL_CBACK_EVT,              /* LL_LTK_REQ_REPLY_CNF */
  /* --- Core Spec 4.2 --- */
  HCI_LE_REM_CONN_PARAM_REQ_CBACK_EVT,                 /* LL_REM_CONN_PARAM_IND */
  HCI_AUTH_PAYLOAD_TO_EXPIRED_CBACK_EVT,               /* LL_AUTH_PAYLOAD_TIMEOUT_IND */
  HCI_LE_DATA_LEN_CHANGE_CBACK_EVT,                    /* LL_DATA_LEN_CHANGE_IND */
  HCI_LE_READ_LOCAL_P256_PUB_KEY_CMPL_CBACK_EVT,       /* LL_READ_LOCAL_P256_PUB_KEY_CMPL_IND */
  HCI_LE_GENERATE_DHKEY_CMPL_CBACK_EVT,                /* LL_GENERATE_DHKEY_CMPL_IND */
  0,                                                   /* LL_SCAN_REPORT_IND */
  /* --- Core Spec 5.0 --- */
  HCI_LE_PHY_UPDATE_CMPL_CBACK_EVT,                    /* LL_PHY_UPDATE_IND */
  HCI_LE_EXT_ADV_REPORT_CBACK_EVT,                     /* LL_EXT_ADV_REPORT_IND */
  HCI_LE_EXT_SCAN_ENABLE_CMD_CMPL_CBACK_EVT,           /* LL_EXT_SCAN_ENABLE_CNF */
  HCI_LE_SCAN_TIMEOUT_CBACK_EVT,                       /* LL_SCAN_TIMEOUT_IND */
  HCI_LE_SCAN_REQ_RCVD_CBACK_EVT,                      /* LL_SCAN_REQ_RCVD_IND */
  HCI_LE_EXT_ADV_ENABLE_CMD_CMPL_CBACK_EVT,            /* LL_EXT_ADV_ENABLE_CNF */
  HCI_LE_ADV_SET_TERM_CBACK_EVT,                       /* LL_ADV_SET_TERM_IND */
  HCI_LE_PER_ADV_ENABLE_CMD_CMPL_CBACK_EVT,            /* LL_PER_ADV_ENABLE_CNF */
  HCI_LE_PER_ADV_SYNC_EST_CBACK_EVT,                   /* LL_PER_ADV_SYNC_EST_IND */
  HCI_LE_PER_ADV_SYNC_LOST_CBACK_EVT,                  /* LL_PER_ADV_SYNC_LOST_IND */
  HCI_LE_PER_ADV_REPORT_CBACK_EVT,                     /* LL_PER_ADV_REPORT_IND */
  HCI_LE_CH_SEL_ALGO_CBACK_EVT,                        /* LL_CH_SEL_ALGO_IND */
  /* --- Core Spec 5.1 --- */
  0,                                                   /* LL_CONNLESS_IQ_REPORT_IND */
  0,                                                   /* LL_CONN_IQ_REPORT_IND */
  0,                                                   /* LL_CTE_REQ_FAILED_IND */
  HCI_LE_PER_SYNC_TRSF_RCVD_CBACK_EVT,                 /* LL_PER_SYNC_TRSF_RCVD_IND */
  /* --- Core Spec Milan --- */
  HCI_CIS_EST_CBACK_EVT,                               /* LL_CIS_EST_IND */
  HCI_CIS_REQ_CBACK_EVT,                               /* LL_CIS_REQ_IND */
  HCI_REQ_PEER_SCA_CBACK_EVT                           /* LL_REQ_PEER_SCA_IND */
};

/* Rand command read counter */
static uint8_t hciEvtRandCnt;

/*************************************************************************************************/
/*!
 *  \brief  Process received an internal HCI event.
 *
 *  \param  pEvt    Buffer containing HCI event.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtProcessIntEvt(hciEvt_t *pEvt)
{
  pEvt->hdr.event &= ~HCI_EVT_INT_TYPE;

  switch (pEvt->hdr.event)
  {
    case HCI_LE_RAND_CMD_CMPL_CBACK_EVT:
      if (hciCb.secCback)
      {
        hciCb.secCback(pEvt);
      }

      /* handle event during reset sequence */
      if (hciCb.resetting)
      {
        /* check if need to send another rand command */
        if (hciEvtRandCnt < (HCI_RESET_RAND_CNT-1))
        {
          hciEvtRandCnt++;
          HciLeRandCmd();
        }
        else
        {
          hciCb.resetting = FALSE;

          /* propagate reset */
          pEvt->hdr.event = HCI_RESET_SEQ_CMPL_CBACK_EVT;
          hciCb.evtCback(pEvt);
        }
      }
      break;

    case HCI_LE_GENERATE_DHKEY_CMPL_CBACK_EVT:
    case HCI_LE_ENCRYPT_CMD_CMPL_CBACK_EVT:
      if (hciCb.secCback)
      {
        hciCb.secCback(pEvt);
      }
      break;

    default:
      hciCb.evtCback(pEvt);
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Process received HCI events.
 *
 *  \param  pEvt    Buffer containing HCI event.
 *
 *  \return None.
 */
/*************************************************************************************************/
void hciEvtProcessMsg(uint8_t *pEvt)
{
  LlEvt_t *pMsg = (LlEvt_t *)pEvt;

  uint8_t event = pMsg->hdr.event;

  /* if event generated internally by HCI APIs */
  if (event & HCI_EVT_INT_TYPE)
  {
    hciEvtProcessIntEvt((hciEvt_t *)pMsg);
    return;
  }

  /* convert hci event code to internal event code and perform special handling */
  switch (event)
  {
    case LL_RESET_CNF:
      /* initialize rand command count */
      hciEvtRandCnt = 0;

      /* restore LL state */
      LlGetBdAddr(hciCoreCb.bdAddr);

      /* if LL Privacy is supported by Controller and included */
      if ((HciGetLeSupFeat() & HCI_LE_SUP_FEAT_PRIVACY) &&
          (hciLeSupFeatCfg & HCI_LE_SUP_FEAT_PRIVACY))
      {
        LlReadResolvingListSize(&hciCoreCb.resListSize);
      }
      else
      {
        hciCoreCb.resListSize = 0;
      }

      /* if LE Data Packet Length Extensions is supported by Controller and included */
      if ((HciGetLeSupFeat() & HCI_LE_SUP_FEAT_DATA_LEN_EXT) &&
          (hciLeSupFeatCfg & HCI_LE_SUP_FEAT_DATA_LEN_EXT))
      {
        uint16_t maxTxOctets;
        uint16_t maxTxTime;
        uint16_t maxRxOctets;
        uint16_t maxRxTime;

        LlReadMaximumDataLen(&maxTxOctets, &maxTxTime, &maxRxOctets, &maxRxTime);

        /* Use Controller's maximum supported payload octets and packet duration times
         * for transmission as Host's suggested values for maximum transmission number
         * of payload octets and maximum packet transmission time for new connections.
         */
        LlWriteDefaultDataLen(maxTxOctets, maxTxTime);
      }

      /* restore local version info */
      LlGetVersion(&hciCoreCb.locVerInfo.manufacturerName,
                   &hciCoreCb.locVerInfo.hciVersion, &hciCoreCb.locVerInfo.hciRevision);

      /* set LMP_Version */
      hciCoreCb.locVerInfo.lmpVersion = hciCoreCb.locVerInfo.hciVersion;

      /* set LMP_Subversion */
      hciCoreCb.locVerInfo.lmpSubversion = hciCoreCb.locVerInfo.hciRevision;

      /* reset internals */
      hciCoreCb.availBufs = hciCoreCb.numBufs;

      /* reset extended info */
      if (hciCoreCb.extResetSeq)
      {
        (*hciCoreCb.extResetSeq)(NULL, 0);
      }

      /* check if need to send first rand command */
      if (hciCb.secCback)
      {
        hciEvtRandCnt++;
        HciLeRandCmd();
      }
      else
      {
        hciCb.resetting = FALSE;

        /* propagate reset */
        pMsg->hdr.event = HCI_RESET_SEQ_CMPL_CBACK_EVT;
        hciCb.evtCback((hciEvt_t *)pMsg);
      }
      break;

    case LL_GENERATE_DHKEY_CMPL_IND:
    case LL_READ_LOCAL_P256_PUB_KEY_CMPL_IND:
      if (hciCb.secCback)
      {
        pMsg->hdr.event = hciEvtLookup[pMsg->hdr.event];
        hciCb.secCback((hciEvt_t *)pMsg);
      }
      break;

    case LL_CONN_IND:
      /* if connection created successfully */
      if (pMsg->hdr.status == LL_SUCCESS)
      {
        hciCoreConnOpen(pMsg->connInd.handle);
      }
      /* fall through */

    case LL_ERROR_IND:
    case LL_ADV_REPORT_IND:
    case LL_DISCONNECT_IND:
    case LL_CONN_UPDATE_IND:
    case LL_REM_CONN_PARAM_IND:
    case LL_CREATE_CONN_CANCEL_CNF:
    case LL_READ_REMOTE_VER_INFO_CNF:
    case LL_READ_REMOTE_FEAT_CNF:
    case LL_ENC_CHANGE_IND:
    case LL_ENC_KEY_REFRESH_IND:
    case LL_LTK_REQ_IND:
    case LL_LTK_REQ_NEG_REPLY_CNF:
    case LL_LTK_REQ_REPLY_CNF:
    case LL_AUTH_PAYLOAD_TIMEOUT_IND:
    case LL_DATA_LEN_CHANGE_IND:
    case LL_PHY_UPDATE_IND:
    case LL_EXT_ADV_REPORT_IND:
    case LL_SCAN_TIMEOUT_IND:
    case LL_SCAN_REQ_RCVD_IND:
    case LL_ADV_SET_TERM_IND:
    case LL_SCAN_ENABLE_CNF:
    case LL_ADV_ENABLE_CNF:
    case LL_EXT_SCAN_ENABLE_CNF:
    case LL_EXT_ADV_ENABLE_CNF:
    case LL_PER_ADV_ENABLE_CNF:
    case LL_PER_ADV_SYNC_ESTD_IND:
    case LL_PER_ADV_SYNC_LOST_IND:
    case LL_PER_ADV_REPORT_IND:
    case LL_CH_SEL_ALGO_IND:

      /* lookup HCI event callback code */
      pMsg->hdr.event = hciEvtLookup[pMsg->hdr.event];

      /* Note: HCI and LL event structures identical, no translation needed */
      hciCb.evtCback((hciEvt_t *)pMsg);

      if (event == LL_DISCONNECT_IND)
      {
        hciCoreConnClose(pMsg->disconnectInd.handle);
      }

      break;

    default:
      break;
  }
}
