/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      802.15.4 controller HCI: Scan.
 *
 *  Copyright (c) 2016-2018 Arm Ltd.
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

#include "chci_154_int.h"
#include "mac_154_int.h"

#include <string.h>

/*************************************************************************************************/
/*!
 *  \brief      Scan command handler.
 *
 *  \param      pHdr      Command header.
 *  \param      pBuf      Command buffer.
 *
 *  \return     TRUE if command handled.
 */
/*************************************************************************************************/

static bool_t chci154ScanCmdHandler(Chci154Hdr_t *pHdr, uint8_t *pBuf)
{
  switch (pHdr->code)
  {
    case CHCI_154_CMD_MLME_SCAN_REQ:
    {
      uint8_t scanType;
      uint32_t scanChannels;
      uint8_t scanDuration;

      /* Extract common parameters */
      BSTREAM_TO_UINT8(scanType, pBuf);
      BSTREAM_TO_UINT32(scanChannels, pBuf);
      BSTREAM_TO_UINT8(scanDuration, pBuf);

      if (Mac154GetState() != MAC_154_STATE_SCAN)
      {
        uint8_t testMode = 0;

#ifdef CHCI_154_MLME_SAP_SANITY_CHECK
        if ((scanType >= NUM_MAC_MLME_SCAN_TYPE) ||
            (scanDuration > 14) ||
            (scanChannels == 0))
        {
          /* Send confirm with status error */
          Chci154ScanSendCfm(scanChannels, scanType, 0, NULL, MAC_154_ENUM_INVALID_PARAMETER);
          return TRUE; /* Handled */
        }
#endif /* CHCI_154_MLME_SAP_SANITY_CHECK */

        /* The bottom (normally unused) 2 bits are used to specify ED/CCA test mode */
        if ((scanChannels & MAC_154_ED_SCAN_TEST_MODE_MASK) != 0)
        {
          /* Special ED/CCA test mode */
          testMode = (uint8_t)scanChannels;

          /* Remove test mode from scan channels */
          scanChannels &= ~MAC_154_ED_SCAN_TEST_MODE_MASK;
        }

        if (Mac154ScanStart(scanType, scanChannels, scanDuration, testMode) == MAC_154_ERROR)
        {
          Chci154ScanSendCfm(scanChannels, scanType, 0, NULL, MAC_154_ENUM_TRANSACTION_OVERFLOW);
        }
      }
      else
      {
        Chci154ScanSendCfm(scanChannels, scanType, 0, NULL, MAC_154_ENUM_SCAN_IN_PROGRESS);
      }
      return TRUE; /* Handled */
    }

    default:
      break;
  }
  return FALSE; /* Not handled */
}

/*************************************************************************************************/
/*!
 *  \brief      Send beacon notify indication
 *
 *  \param      bsn         Beacon sequence number.
 *  \param      pPanDescr   Pointer to PAN descriptor.
 *  \param      sduLength   SDU (beacon payload) length.
 *  \param      pSdu        Pointer to SDU (beacon payload).
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154ScanSendBeaconNotifyInd(uint8_t bsn, Mac154PanDescr_t *pPanDescr, uint8_t sduLength, uint8_t *pSdu)
{
  uint8_t *pMsg;

  if ((pMsg = WsfMsgAlloc(CHCI_154_MSG_HDR_LEN +
                          1 + /* BSN */
                          sizeof(Mac154PanDescr_t) +
                          1 + /* Beacon payload length */
                          sduLength)) != NULL)
  {
    uint8_t *pBuf = pMsg;

    /* Set header */
    UINT8_TO_BSTREAM (pBuf, CHCI_154_EVT_MLME_BEACON_NOTIFY_IND);
    UINT16_TO_BSTREAM(pBuf, 1 + sizeof(Mac154PanDescr_t) + 1 + sduLength);

    UINT8_TO_BSTREAM (pBuf, bsn);
    memcpy(pBuf, pPanDescr, sizeof(Mac154PanDescr_t));
    pBuf += sizeof(Mac154PanDescr_t);

    UINT8_TO_BSTREAM (pBuf, sduLength);
    memcpy(pBuf, pSdu, sduLength);

    Chci154SendEvent(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Send scan confirm.
 *
 *  \param      channels    Unscanned channels.
 *  \param      type        Scan type.
 *  \param      listSize    Results list size.
 *  \param      pScanList   Pointer to scan list.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154ScanSendCfm(uint32_t channels, uint8_t type, uint8_t listSize, Mac154ScanResults_t *pScanResults, uint8_t statusOverride)
{
  static const uint8_t listSizeFactor[MAC_154_MLME_NUM_SCAN_TYPE] = {1, sizeof(Mac154PanDescr_t), sizeof(Mac154PanDescr_t)};
  uint8_t *pMsg;
  uint8_t msgListSize;
  uint8_t status = MAC_154_ENUM_SUCCESS;

  msgListSize = listSize * listSizeFactor[type];
  if ((pMsg = WsfMsgAlloc(CHCI_154_MSG_HDR_LEN + 7 + msgListSize)) != NULL) /* TODO magic number */
  {
    uint8_t *pBuf = pMsg;
    int i;

    /* Set header */
    UINT8_TO_BSTREAM (pBuf, CHCI_154_EVT_MLME_SCAN_CFM);
    UINT16_TO_BSTREAM(pBuf, 7 + msgListSize);

    /* Work out status */
    if ((listSize == 0) && (pScanResults == NULL))
    {
      /* Early error */
      status = statusOverride;
    }
    else if ((type == MAC_154_MLME_SCAN_TYPE_ACTIVE) ||
             (type == MAC_154_MLME_SCAN_TYPE_PASSIVE))
    {
      if ((listSize == 0) && (pScanResults->panDescr[0].logicalChan == 0))
      {
        /* Note: Checking pScanResults->panDescr[0].logicalChan tells us if we
         * received a beacon or not. If it is 0, we received no beacons (it would be
         * 11-27 otherwise).
         */
        status = MAC_154_ENUM_NO_BEACON;
      }
      else if (listSize == MAC_154_SCAN_MAX_PD_ENTRIES)
      {
        status = MAC_154_ENUM_LIMIT_REACHED;
      }
    }

    /* TODO: Unscanned channels */
    UINT8_TO_BSTREAM (pBuf, status);
    UINT8_TO_BSTREAM (pBuf, type);
    UINT32_TO_BSTREAM (pBuf, channels);
    UINT8_TO_BSTREAM (pBuf, listSize);

    for (i = 0; i < (int)listSize; i++)
    {
      switch (type)
      {
        case MAC_154_MLME_SCAN_TYPE_ENERGY_DETECT:
          /* TODO: These results are in dBm at the moment - convert? */
          UINT8_TO_BSTREAM (pBuf, (uint8_t)pScanResults->edList[i]);
        break;
        case MAC_154_MLME_SCAN_TYPE_ACTIVE:
        case MAC_154_MLME_SCAN_TYPE_PASSIVE:
          memcpy(pBuf, &pScanResults->panDescr[i], sizeof(Mac154PanDescr_t));
          pBuf += sizeof(Mac154PanDescr_t);
        break;
      }
    }

    Chci154SendEvent(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize controller HCI for scan operations.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154ScanInit(void)
{
  Chci154RegisterCmdHandler(chci154ScanCmdHandler);
}
