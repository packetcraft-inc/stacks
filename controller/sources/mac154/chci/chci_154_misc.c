/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      802.15.4 controller HCI: Miscellaneous.
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
#include "mac_154_api.h"
#include "mac_154_int.h"
#include "sch_api.h"

#include <string.h>

/*************************************************************************************************/
/*!
 *  \brief      Miscellaneous commands handler.
 *
 *  \param      pHdr      Command header.
 *  \param      pBuf      Command buffer.
 *
 *  \return     TRUE if command handled.
 */
/*************************************************************************************************/
static uint8_t chci154ResetHandler(void *param)
{
  /* TODO: Not happy with the way initialization is scattered
   * but this achieves what we want.
   */
  Mac154DeInit();
  Mac154Init(FALSE);
  /* Signal boolean through whether param is NULL or not */
  if (param != NULL)
  {
    Mac154InitPIB();
  }
  Chci154MiscSendResetCfm(MAC_154_ENUM_SUCCESS);
  return 0x00;
}

/*************************************************************************************************/
/*!
 *  \brief      Miscellaneous commands handler.
 *
 *  \param      pHdr      Command header.
 *  \param      pBuf      Command buffer.
 *
 *  \return     TRUE if command handled.
 */
/*************************************************************************************************/
static bool_t chci154MiscCmdHandler(Chci154Hdr_t *pHdr, uint8_t *pBuf)
{
  Mac154Pib_t * const pPib = Mac154GetPIB();
  Mac154PhyPib_t * const pPhyPib = Mac154GetPhyPIB();

  switch (pHdr->code)
  {
    case CHCI_154_CMD_MLME_RESET_REQ:
    {
      uint8_t setDefaultPib;

      BSTREAM_TO_UINT8(setDefaultPib, pBuf);
      /* Stop Rx if running */
      Mac154ActionRx(MAC_154_RX_STOP);
      chci154ResetHandler(setDefaultPib ? &setDefaultPib : NULL);
      return TRUE;
    }

    case CHCI_154_CMD_MLME_RX_ENABLE_REQ:
    {
      uint32_t symDuration;

      /* Maintained as a flag in conjunction with rx on when idle
       * If either are asserted, then receive will go on. The rx enable flag will
       * be timer-bound and "click off" after the timeout period.
       */
      BSTREAM_TO_UINT24(symDuration, pBuf);
      Mac154StartRxEnableTimer(symDuration);
      Chci154MiscSendRxEnableCfm(MAC_154_ENUM_SUCCESS);
      return TRUE;
    }

    case CHCI_154_CMD_MLME_START_REQ:
    {
      uint8_t  coordRealignment;
      uint16_t panId;
      uint8_t  logChan;
      uint8_t  panCoord;

      /* Don't do anything if short address is unassigned (SR [124,5]) */
      if (pPib->shortAddr == MAC_154_UNASSIGNED_ADDR)
      {
        Chci154MiscSendStartCfm(MAC_154_ENUM_NO_SHORT_ADDRESS);
        return FALSE;
      }

      /* Extract common parameters */
      BSTREAM_TO_UINT8(coordRealignment, pBuf);
      BSTREAM_TO_UINT16(panId, pBuf);
      BSTREAM_TO_UINT8(logChan, pBuf);
      BSTREAM_TO_UINT8(panCoord, pBuf);
      pPhyPib->txPower = 0;
      /* TODO: Beacon security */

      /* Sanity check */
#ifdef CHCI_154_MLME_SAP_SANITY_CHECK
      /* TODO */
      if (1)
      {
        /* Send confirm with status error */
        return FALSE;
      }
#endif /* CHCI_154_MLME_SAP_SANITY_CHECK */

      if (coordRealignment)
      {
        if (pPhyPib->chan != PHY_154_INVALID_CHANNEL)
        {
          if (Mac154DataCoordRealignStart(panId, panCoord, logChan, 0) == MAC_154_ERROR)
          {
            Chci154MiscSendStartCfm(MAC_154_ENUM_TRANSACTION_OVERFLOW);
          }
        }
        else
        {
          /* Can't send if there is no channel set */
          Chci154MiscSendStartCfm(MAC_154_ENUM_INVALID_PARAMETER);
        }
      }
      else
      {
        pPib->panId = panId;
        pPib->deviceType = panCoord ? MAC_154_DEV_TYPE_PAN_COORD : MAC_154_DEV_TYPE_COORD;
        pPhyPib->chan = logChan;
        pPhyPib->txPower = 0;

        /* Just send confirm */
        Chci154MiscSendStartCfm(MAC_154_ENUM_SUCCESS);
      }
      return TRUE;
    }

#if MAC_154_OPT_ORPHAN

    case CHCI_154_CMD_MLME_ORPHAN_RSP:
    {
      uint64_t orphanAddr;
      uint8_t  *pOrphanAddrBuf;
      uint16_t shtAddr;
      uint8_t  assocMember;

      /* Extract common parameters */
      pOrphanAddrBuf = pBuf;
      BSTREAM_TO_UINT64(orphanAddr, pBuf);
      BSTREAM_TO_UINT16(shtAddr, pBuf);
      BSTREAM_TO_UINT8(assocMember, pBuf);
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

      if (Mac154DataOrphanRspStart(orphanAddr, shtAddr, assocMember) == MAC_154_ERROR)
      {
        Mac154Addr_t srcAddr;
        Mac154Addr_t dstAddr;

        /* Source address must be set correctly for NHLE to process. */
        srcAddr.addrMode = MAC_154_ADDR_MODE_EXTENDED;
        memcpy(srcAddr.addr, pOrphanAddrBuf, 8);
        dstAddr.addrMode = MAC_154_ADDR_MODE_NONE;

        Chci154DataSendCommStatusInd(&srcAddr, &dstAddr, MAC_154_ENUM_TRANSACTION_OVERFLOW);
      }
      return TRUE;
    }
#endif /* MAC_154_OPT_ORPHAN */

    default:
      break;
  }
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief      Send reset confirm.
 *
 *  \param      status  Reset status
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154MiscSendResetCfm(const uint8_t status)
{
  /* TODO security */
  uint8_t *pMsg;

  if ((pMsg = WsfMsgAlloc(CHCI_154_MSG_HDR_LEN + 1)) != NULL) /* TODO magic number */
  {
    uint8_t *pBuf = pMsg;

    /* Set header */
    UINT8_TO_BSTREAM (pBuf, CHCI_154_EVT_MLME_RESET_CFM);
    UINT16_TO_BSTREAM(pBuf, 1);

    /* Parameters */
    UINT8_TO_BSTREAM(pBuf, status);

    Chci154SendEvent(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Send rx enable confirm.
 *
 *  \param      status  Rx enable status
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154MiscSendRxEnableCfm(const uint8_t status)
{
  /* TODO security */
  uint8_t *pMsg;

  if ((pMsg = WsfMsgAlloc(CHCI_154_MSG_HDR_LEN + 1)) != NULL) /* TODO magic number */
  {
    uint8_t *pBuf = pMsg;

    /* Set header */
    UINT8_TO_BSTREAM (pBuf, CHCI_154_EVT_MLME_RX_ENABLE_CFM);
    UINT16_TO_BSTREAM(pBuf, 1);

    /* Parameters */
    UINT8_TO_BSTREAM(pBuf, status);

    Chci154SendEvent(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Send start confirm.
 *
 *  \param      status  Start status
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154MiscSendStartCfm(const uint8_t status)
{
  /* TODO security */
  uint8_t *pMsg;

  if ((pMsg = WsfMsgAlloc(CHCI_154_MSG_HDR_LEN + 1)) != NULL) /* TODO magic number */
  {
    uint8_t *pBuf = pMsg;

    /* Set header */
    UINT8_TO_BSTREAM (pBuf, CHCI_154_EVT_MLME_START_CFM);
    UINT16_TO_BSTREAM(pBuf, 1);

    /* Parameters */
    UINT8_TO_BSTREAM(pBuf, status);

    Chci154SendEvent(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize controller HCI for miscellaneous operations.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154MiscInit(void)
{
  Chci154RegisterCmdHandler(chci154MiscCmdHandler);
}
