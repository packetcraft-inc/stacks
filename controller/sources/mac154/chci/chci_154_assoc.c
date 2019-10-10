/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      802.15.4 controller HCI: Association.
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
 *  \brief      Association command handler.
 *
 *  \param      pHdr      Command header.
 *  \param      pBuf      Command buffer.
 *
 *  \return     TRUE if command handled.
 */
/*************************************************************************************************/
static bool_t chci154AssocCmdHandler(Chci154Hdr_t *pHdr, uint8_t *pBuf)
{
  Mac154Pib_t * const pPib = Mac154GetPIB();
  Mac154PhyPib_t * const pPhyPib = Mac154GetPhyPIB();

  switch (pHdr->code)
  {
    case CHCI_154_CMD_MLME_ASSOC_REQ:
    {
      uint8_t logChan;
      Mac154Addr_t coordAddr;
      uint8_t capInfo;

      /* Extract common parameters */
      BSTREAM_TO_UINT8(logChan, pBuf);
      /* chanPage elided in command */
      BSTREAM_TO_UINT8(coordAddr.addrMode, pBuf);
      memcpy(coordAddr.panId, pBuf, 2);
      pBuf += 2;
      memcpy(coordAddr.addr, pBuf, 8);
      pBuf += 8;
      BSTREAM_TO_UINT8(capInfo, pBuf);
      /* TODO: Security */

      /* Sanity check */
#ifdef CHCI_154_MLME_SAP_SANITY_CHECK
      /* TODO */
      if (1)
      {
        /* Send confirm with status error */
        return FALSE;
      }
#endif /* CHCI_154_MLME_SAP_SANITY_CHECK */

      /* Set PIB attributes SR [179,29] */
      BYTES_TO_UINT16(pPib->panId, coordAddr.panId);
      if (coordAddr.addrMode == MAC_154_ADDR_MODE_SHORT)
      {
        BYTES_TO_UINT16(pPib->coordShortAddr, coordAddr.addr);
      }
      else
      {
        BYTES_TO_UINT64(pPib->coordExtAddr, coordAddr.addr);
      }
      pPhyPib->chan = logChan;
      pPhyPib->txPower = 0;

      if (Mac154AssocReqStart(&coordAddr, capInfo) == MAC_154_ERROR)
      {
        Chci154AssocSendAssocCfm(MAC_154_UNASSIGNED_ADDR, MAC_154_ENUM_TRANSACTION_OVERFLOW);
      }

      return TRUE; /* Handled */
    }

    case CHCI_154_CMD_MLME_ASSOC_RSP:
    {
      uint64a_t deviceAddr;
      uint16a_t assocShtAddr;
      uint8_t status;

      /* Extract common parameters */
      memcpy(deviceAddr, pBuf, 8);
      pBuf += 8;
      memcpy(assocShtAddr, pBuf, 2);
      pBuf += 2;
      BSTREAM_TO_UINT8(status, pBuf);

      /* Sanity check */
#ifdef CHCI_154_MLME_SAP_SANITY_CHECK
      /* TODO */
      if (1)
      {
        /* Send confirm with status error */
        return FALSE;
      }
#endif /* CHCI_154_MLME_SAP_SANITY_CHECK */

      if (Mac154AssocRspStart(deviceAddr, assocShtAddr, status) == MAC_154_ERROR)
      {
        Mac154Addr_t srcAddr;
        Mac154Addr_t dstAddr;

        /* Source address must be set correctly for NHLE to process. */
        srcAddr.addrMode = MAC_154_ADDR_MODE_EXTENDED;
        memcpy(srcAddr.addr, deviceAddr, 8);
        dstAddr.addrMode = MAC_154_ADDR_MODE_NONE;

        Chci154DataSendCommStatusInd(&srcAddr, &dstAddr, MAC_154_ENUM_TRANSACTION_OVERFLOW);
      }

      return TRUE; /* Handled */
    }

#if MAC_154_OPT_DISASSOC

    case CHCI_154_CMD_MLME_DISASSOC_REQ:
    {
      Mac154Addr_t deviceAddr;
      uint8_t reason;
      uint8_t txIndirect;
      bool_t  toCoord = FALSE;

      /* Extract common parameters */
      BSTREAM_TO_UINT8(deviceAddr.addrMode, pBuf);
      memcpy(deviceAddr.panId, pBuf, 2);
      pBuf += 2;
      memcpy(deviceAddr.addr, pBuf, 8);
      pBuf += 8;
      BSTREAM_TO_UINT8(reason, pBuf);
      BSTREAM_TO_UINT8(txIndirect, pBuf);
      /* TODO: Security */

      /* Sanity check */
#ifdef CHCI_154_MLME_SAP_SANITY_CHECK
      /* TODO */
      if (1)
      {
        /* Send confirm with status error */
        return FALSE;
      }
#endif /* CHCI_154_MLME_SAP_SANITY_CHECK */

      toCoord = Mac154AssocDisassocToCoord(&deviceAddr);

      if (!((pPib->deviceType == MAC_154_DEV_TYPE_DEVICE) && toCoord))
      {
        Chci154AssocSendDisassocCfm(&deviceAddr, MAC_154_ENUM_INVALID_PARAMETER);
      }
      else if (Mac154AssocDisassocStart(&deviceAddr, reason, txIndirect, toCoord) == MAC_154_ERROR)
      {
        Chci154AssocSendDisassocCfm(&deviceAddr, MAC_154_ENUM_TRANSACTION_OVERFLOW);
      }

      return TRUE; /* Handled */
    }

#endif /* MAC_154_OPT_DISASSOC */

    default:
      break;
  }
  return FALSE; /* Not handled */
}

/*************************************************************************************************/
/*!
 *  \brief      Send associate confirm.
 *
 *  \param      assocShtAddr  Associated short address
 *  \param      status        Association status
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154AssocSendAssocCfm(uint16_t assocShtAddr, uint8_t status)
{
  /* TODO security */
  uint8_t *pMsg;

  if (status != MAC_154_ENUM_SUCCESS)
  {
    /* pPib->shortAddr should remain as MAC_154_UNASSIGNED_ADDR */
    Mac154GetPIB()->panId = MAC_154_UNASSIGNED_PAN_ID; /* SR [181,6] */
  }

  if ((pMsg = WsfMsgAlloc(CHCI_154_MSG_HDR_LEN + 2 + 1)) != NULL) /* TODO magic number */
  {
    uint8_t *pBuf = pMsg;

    /* Set header */
    UINT8_TO_BSTREAM(pBuf, CHCI_154_EVT_MLME_ASSOC_CFM);
    UINT16_TO_BSTREAM(pBuf, 2 + 1);

    /* Parameters */
    UINT16_TO_BSTREAM(pBuf, assocShtAddr);
    UINT8_TO_BSTREAM(pBuf, status);

    Chci154SendEvent(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Send associate indication.
 *
 *  \param      deviceAddr  Device address
 *  \param      capInfo     Capability information
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154AssocSendAssocInd(uint64a_t deviceAddr, uint8_t capInfo)
{
  /* TODO security */
  uint8_t *pMsg;

  if ((pMsg = WsfMsgAlloc(CHCI_154_MSG_HDR_LEN + 8 + 1)) != NULL) /* TODO magic number */
  {
    uint8_t *pBuf = pMsg;

    /* Set header */
    UINT8_TO_BSTREAM (pBuf, CHCI_154_EVT_MLME_ASSOC_IND);
    UINT16_TO_BSTREAM(pBuf, 8 + 1);

    /* Parameters */
    memcpy(pBuf, deviceAddr, 8);
    pBuf += 8;
    UINT8_TO_BSTREAM(pBuf, capInfo);

    Chci154SendEvent(pMsg);
  }
}

#if MAC_154_OPT_DISASSOC

/*************************************************************************************************/
/*!
 *  \brief      Send disassociate indication.
 *
 *  \param      deviceAddr    Address of device disassociating
 *  \param      reason        Disassociation reason
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154AssocSendDisassocInd(uint64a_t deviceAddr, uint8_t reason)
{
  /* TODO security */
  uint8_t *pMsg;

  if ((pMsg = WsfMsgAlloc(CHCI_154_MSG_HDR_LEN + 8 + 1)) != NULL) /* TODO magic number */
  {
    uint8_t *pBuf = pMsg;

    /* Set header */
    UINT8_TO_BSTREAM (pBuf, CHCI_154_EVT_MLME_DISASSOC_IND);
    UINT16_TO_BSTREAM(pBuf, 8 + 1);

    /* Parameters */
    memcpy(pBuf, deviceAddr, 8);
    pBuf += 8;
    UINT8_TO_BSTREAM(pBuf, reason);

    Chci154SendEvent(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Send disassociate confirm.
 *
 *  \param      deviceAddr  Address disassoc. notification was sent to
 *  \param      status      Disassociation status
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154AssocSendDisassocCfm(Mac154Addr_t *pDeviceAddr, uint8_t status)
{
  /* TODO security */
  uint8_t *pMsg;

  if ((pMsg = WsfMsgAlloc(CHCI_154_MSG_HDR_LEN + 1 + 2 + 8 + 1)) != NULL) /* TODO magic number */
  {
    uint8_t *pBuf = pMsg;

    /* Set header */
    UINT8_TO_BSTREAM (pBuf, CHCI_154_EVT_MLME_DISASSOC_CFM);
    UINT16_TO_BSTREAM(pBuf, 1 + 2 + 8 + 1);

    /* Parameters */
    UINT8_TO_BSTREAM(pBuf, pDeviceAddr->addrMode);
    memcpy(pBuf, pDeviceAddr->panId, 2);
    pBuf += 2;
    memcpy(pBuf, pDeviceAddr->addr, 8);
    pBuf += 8;
    UINT8_TO_BSTREAM(pBuf, status);

    Chci154SendEvent(pMsg);
  }
}

#endif /* MAC_154_OPT_DISASSOC */

#if MAC_154_OPT_ORPHAN

/*************************************************************************************************/
/*!
 *  \brief      Send orphan indication.
 *
 *  \param      orphanAddr    Address of orphaned device
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154AssocSendOrphanInd(uint64a_t orphanAddr)
{
  /* TODO security */
  uint8_t *pMsg;

  if ((pMsg = WsfMsgAlloc(CHCI_154_MSG_HDR_LEN + 8)) != NULL) /* TODO magic number */
  {
    uint8_t *pBuf = pMsg;

    /* Set header */
    UINT8_TO_BSTREAM (pBuf, CHCI_154_EVT_MLME_ORPHAN_IND);
    UINT16_TO_BSTREAM(pBuf, 8);

    /* Parameters */
    memcpy(pBuf, orphanAddr, 8);

    Chci154SendEvent(pMsg);
  }
}

#endif /* MAC_154_OPT_ORPHAN */

/*************************************************************************************************/
/*!
 *  \brief      Initialize controller HCI for association operations.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154AssocInit(void)
{
  Chci154RegisterCmdHandler(chci154AssocCmdHandler);
}
