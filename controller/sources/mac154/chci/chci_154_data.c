/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      802.15.4 controller HCI: Test.
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
#include "bb_154_int.h"

#include <string.h>

/*************************************************************************************************/
/*!
 *  \brief      Send data transmit confirm.
 *
 *  \param      msduHandle    Handle of corresponding MCPS-DATA.req
 *  \param      status        Purge status
 *  \param      timestamp     Timestamp of MSDU
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154DataTxSendCfm(const uint8_t msduHandle, const uint8_t status, const uint32_t timestamp)
{
  uint8_t *pMsg;

  if ((pMsg = WsfMsgAlloc(CHCI_154_MSG_HDR_LEN + sizeof(Mac154DataTxCfm_t))) != NULL)
  {
    uint8_t *pBuf = pMsg;

    /* Set header */
    UINT8_TO_BSTREAM (pBuf, CHCI_154_EVT_MCPS_DATA_CFM);
    UINT16_TO_BSTREAM(pBuf, sizeof(Mac154DataTxCfm_t));

    /* Fill message body */
    UINT8_TO_BSTREAM (pBuf, msduHandle);
    UINT8_TO_BSTREAM (pBuf, status);
    UINT24_TO_BSTREAM (pBuf, timestamp);

    Chci154SendEvent(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Send data poll confirm.
 *
 *  \param      status    Poll status
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154DataSendPollCfm(const uint8_t status)
{
  uint8_t *pMsg;

  if ((pMsg = WsfMsgAlloc(CHCI_154_MSG_HDR_LEN + sizeof(Mac154DataPollCfm_t))) != NULL)
  {
    uint8_t *pBuf = pMsg;

    /* Set header */
    UINT8_TO_BSTREAM (pBuf, CHCI_154_EVT_MLME_POLL_CFM);
    UINT16_TO_BSTREAM(pBuf, sizeof(Mac154DataPollCfm_t));

    /* Fill message body */
    UINT8_TO_BSTREAM (pBuf, status);

    Chci154SendEvent(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Send data poll indication.
 *
 *  \param      srcAddr         Source address
 *  \param      dataFrameSent   Poll status
 *
 *  \return     None.
 *
 *  \note       This is a vendor-specific extension
 */
/*************************************************************************************************/
void Chci154DataSendPollInd(const Mac154Addr_t *srcAddr, const uint8_t dataFrameSent)
{
  uint8_t *pMsg;

  if ((pMsg = WsfMsgAlloc(CHCI_154_MSG_HDR_LEN + sizeof(Mac154DataPollInd_t))) != NULL)
  {
    uint8_t *pBuf = pMsg;

    /* Set header */
    UINT8_TO_BSTREAM (pBuf, CHCI_154_EVT_MLME_POLL_IND);
    UINT16_TO_BSTREAM(pBuf, sizeof(Mac154DataPollInd_t));

    /* Fill message body */
    memcpy(pBuf, srcAddr, MAC_154_SIZEOF_ADDR_T);
    pBuf += MAC_154_SIZEOF_ADDR_T;
    UINT8_TO_BSTREAM (pBuf, dataFrameSent);

    Chci154SendEvent(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Send data purge confirm.
 *
 *  \param      msduHandle    Handle of corresponding MCPS-DATA.req
 *  \param      status        Purge status
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154DataSendPurgeCfm(const uint8_t msduHandle, const uint8_t status)
{
  uint8_t *pMsg;

  if ((pMsg = WsfMsgAlloc(CHCI_154_MSG_HDR_LEN + sizeof(Mac154DataPurgeCfm_t))) != NULL)
  {
    uint8_t *pBuf = pMsg;

    /* Set header */
    UINT8_TO_BSTREAM (pBuf, CHCI_154_EVT_MCPS_PURGE_CFM);
    UINT16_TO_BSTREAM(pBuf, sizeof(Mac154DataPurgeCfm_t));

    /* Fill message body */
    UINT8_TO_BSTREAM (pBuf, msduHandle);
    UINT8_TO_BSTREAM (pBuf, status);

    Chci154SendEvent(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Send data indication.
 *
 *  \param      srcAddr           Source address
 *  \param      dstAddr           Destination address
 *  \param      mpduLinkQuality   Link quality
 *  \param      dsn               Data sequence number
 *  \param      timestamp         Timestamp
 *  \param      msduLength        MAC SDU length
 *  \param      pMsdu             Pointer to MAC SDU
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154DataRxSendInd(const Mac154Addr_t *srcAddr,
                          const Mac154Addr_t *dstAddr,
                          const uint8_t mpduLinkQuality,
                          const uint8_t dsn,
                          const uint32_t timestamp,
                          const uint8_t msduLength,
                          const uint8_t *pMsdu)
{
  uint8_t *pMsg;

  if ((pMsg = WsfMsgAlloc(CHCI_154_MSG_HDR_LEN +
                          2 * MAC_154_SIZEOF_ADDR_T + 6 + /* Size of body excluding MSDU */
                          msduLength)) != NULL)
  {
    uint8_t *pBuf = pMsg;

    /* Set header */
    UINT8_TO_BSTREAM (pBuf, CHCI_154_DATA_MCPS_DATA_IND);
    UINT16_TO_BSTREAM(pBuf, 2 * MAC_154_SIZEOF_ADDR_T + 6 + msduLength);

    /* Fill message body */
    memcpy(pBuf, srcAddr, MAC_154_SIZEOF_ADDR_T);
    pBuf += MAC_154_SIZEOF_ADDR_T;
    memcpy(pBuf, dstAddr, MAC_154_SIZEOF_ADDR_T);
    pBuf += MAC_154_SIZEOF_ADDR_T;
    UINT8_TO_BSTREAM (pBuf, mpduLinkQuality);
    UINT8_TO_BSTREAM (pBuf, dsn);
    UINT24_TO_BSTREAM (pBuf, timestamp);
    UINT8_TO_BSTREAM (pBuf, msduLength);
    memcpy(pBuf, pMsdu, msduLength);
    /* TODO: Security */
    Chci154SendData(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Send comm status indication.
 *
 *  \param      srcAddr   Source address
 *  \param      dstAddr   Destination address
 *  \param      status    Status
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154DataSendCommStatusInd(const Mac154Addr_t *srcAddr,
                                  const Mac154Addr_t *dstAddr,
                                  const uint8_t status)
{
  uint8_t *pMsg;

  if ((pMsg = WsfMsgAlloc(CHCI_154_MSG_HDR_LEN +
                          2 * MAC_154_SIZEOF_ADDR_T + 1)) != NULL)
  {
    uint8_t *pBuf = pMsg;

    /* Set header */
    UINT8_TO_BSTREAM (pBuf, CHCI_154_EVT_MLME_COMM_STATUS_IND);
    UINT16_TO_BSTREAM(pBuf, 2 * MAC_154_SIZEOF_ADDR_T + 1);

    /* Fill message body */
    memcpy(pBuf, srcAddr, MAC_154_SIZEOF_ADDR_T);
    pBuf += MAC_154_SIZEOF_ADDR_T;
    memcpy(pBuf, dstAddr, MAC_154_SIZEOF_ADDR_T);
    pBuf += MAC_154_SIZEOF_ADDR_T;
    UINT8_TO_BSTREAM (pBuf, status);
    /* TODO: Security */
    Chci154SendEvent(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Data command handler.
 *
 *  \param      pHdr      Command header.
 *  \param      pBuf      Command buffer.
 *
 *  \return     TRUE if command handled.
 */
/*************************************************************************************************/

static bool_t chci154DataCmdHandler(Chci154Hdr_t *pHdr, uint8_t *pBuf)
{
  uint8_t msduHandle;
  uint8_t status;

  switch (pHdr->code)
  {
    case CHCI_154_CMD_MLME_POLL_REQ:
    {
      Mac154Addr_t coordAddr;

      /* Extract common parameters */
      BSTREAM_TO_UINT8(coordAddr.addrMode, pBuf);
      memcpy(coordAddr.panId, pBuf, 2);
      pBuf += 2;
      memcpy(coordAddr.addr, pBuf, 8);

      (void)Mac154DataPollStart(&coordAddr);
      return TRUE;
    }
    case CHCI_154_CMD_MCPS_PURGE_REQ:
    {
      BSTREAM_TO_UINT8(msduHandle, pBuf);
      status = Bb154PurgeTxIndirectBuf(msduHandle) ? MAC_154_ENUM_SUCCESS : MAC_154_ENUM_INVALID_HANDLE;
      Chci154DataSendPurgeCfm(msduHandle, status);
      return TRUE;
    }
  }
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief      Data handler.
 *
 *  \param      pHdr      Data header.
 *  \param      pBuf      Data buffer.
 *
 *  \return     None.
 *
 *  Handles MCPS-DATA.req calls.
 */
/*************************************************************************************************/

static void chci154DataHandler(Chci154Hdr_t *pHdr, uint8_t *pBuf)
{
  uint8_t srcAddrMode;
  Mac154Addr_t dstAddr;
  uint8_t msduHandle;
  uint8_t txOptions;
  uint8_t msduLen;
  uint32_t timestamp;

  /* Extract common parameters */
  BSTREAM_TO_UINT8(srcAddrMode, pBuf);
  BSTREAM_TO_UINT8(dstAddr.addrMode, pBuf);
  memcpy(dstAddr.panId, pBuf, 2);
  pBuf += 2;
  memcpy(dstAddr.addr, pBuf, 8);
  pBuf += 8;
  BSTREAM_TO_UINT8(msduHandle, pBuf);
  BSTREAM_TO_UINT8(txOptions, pBuf);
  /* TODO: Security */
  BSTREAM_TO_UINT8(msduLen, pBuf);
  /* Check for additional optional timestamp parameter */
  if ((pHdr->len - msduLen) == 18)
  {
    BSTREAM_TO_UINT24(timestamp, pBuf);
  }
  else
  {
    timestamp = 0;
  }

  if (Mac154DataTxStart(srcAddrMode, &dstAddr, msduHandle, txOptions, timestamp, msduLen, pBuf) == MAC_154_ERROR)
  {
    Chci154DataTxSendCfm(msduHandle, MAC_154_ENUM_TRANSACTION_OVERFLOW, 0);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize controller HCI for data operations.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154DataInit(void)
{
  Chci154RegisterCmdHandler(chci154DataCmdHandler);
  Chci154RegisterDataHandler(chci154DataHandler);
}
