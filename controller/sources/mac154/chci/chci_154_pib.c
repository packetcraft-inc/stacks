/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      802.15.4 controller HCI: PIB get and set.
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
#include "bb_154.h"

#include <string.h>

/**************************************************************************************************
  Type Definitions
**************************************************************************************************/

typedef enum
{
  PIB_PHY,
  PIB_VS,
  PIB_MAC
} chciPidAttrGrp;

/*************************************************************************************************/
/*!
 *  \brief      Send GET/SET confirm.
 *
 *  \param      cfmType     GET or SET confirm.
 *  \param      attrEnum    Attribute enumeration.
 *  \param      attrIdx     Attribute index.
 *  \param      attrLen     Length of attribute buffer.
 *  \param      pAttr       Pointer to attribute buffer.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void chci154PibCfm(uint8_t status, uint8_t cfmType, uint8_t attrEnum, uint8_t attrIdx, uint8_t attrLen, uint8_t *pAttr)
{
  uint8_t *pMsg;

  if ((pMsg = WsfMsgAlloc(CHCI_154_MSG_HDR_LEN + 3 + attrLen)) != NULL) /* TODO magic number */
  {
    uint8_t *pBuf = pMsg;

    /* Set header */
    UINT8_TO_BSTREAM (pBuf, cfmType);
    UINT16_TO_BSTREAM(pBuf, 3 + attrLen); /* TODO magic number */

    UINT8_TO_BSTREAM (pBuf, status);
    UINT8_TO_BSTREAM (pBuf, attrEnum);
    UINT8_TO_BSTREAM (pBuf, attrIdx);
    if (attrLen > 0)
    {
      memcpy(pBuf, pAttr, attrLen);
    }

    Chci154SendEvent(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      PIB command handler.
 *
 *  \param      pHdr      Command header.
 *  \param      pBuf      Command buffer.
 *
 *  \return     TRUE if command handled.
 */
/*************************************************************************************************/
static bool_t chci154PibCmdHandler(Chci154Hdr_t *pHdr, uint8_t *pBuf)
{
  uint8_t type;
  uint8_t attrEnum;
  uint8_t attrIdx;
  uint8_t attrLen;
  uint8_t *pAttr;
  chciPidAttrGrp attrGrp;
  Mac154Pib_t * const pPib = Mac154GetPIB();

  switch (pHdr->code)
  {
    case CHCI_154_CMD_MLME_GET_REQ:
    case CHCI_154_CMD_MLME_SET_REQ:
      /* Extract common parameters */
      BSTREAM_TO_UINT8(attrEnum, pBuf);
      BSTREAM_TO_UINT8(attrIdx, pBuf);

      /* Check attribute range */
      if ((attrEnum >= MAC_154_PHY_PIB_ENUM_MIN) && (attrEnum <= MAC_154_PHY_PIB_ENUM_MAX))
      {
        attrGrp = PIB_PHY;
      }
      else if ((attrEnum >= MAC_154_PIB_VS_ENUM_MIN) && (attrEnum <= MAC_154_PIB_VS_ENUM_MAX))
      {
        attrGrp = PIB_VS;
      }
      else if ((attrEnum >= MAC_154_PIB_ENUM_MIN) && (attrEnum <= MAC_154_PIB_ENUM_MAX))
      {
        attrGrp = PIB_MAC;
      }
      else
      {
        chci154PibCfm(MAC_154_ENUM_INVALID_PARAMETER,
                      (pHdr->code == CHCI_154_CMD_MLME_GET_REQ) ? CHCI_154_EVT_MLME_GET_CFM : CHCI_154_EVT_MLME_SET_CFM,
                      attrEnum,
                      attrIdx,
                      0,
                      NULL);
        return TRUE;
      }

      if (pHdr->code == CHCI_154_CMD_MLME_GET_REQ)
      {
        /* Get the PIB value */
        switch (attrGrp)
        {
          case PIB_PHY:
            pAttr = Mac154PhyPibGetAttr(attrEnum, &attrLen);
            break;

          case PIB_VS:
            pAttr = Mac154PibGetVsAttr(attrEnum, &attrLen);
            break;

          case PIB_MAC:
            pAttr = Mac154PibGetAttr(attrEnum, &attrLen);
            if (attrEnum == MAC_154_PIB_ENUM_BEACON_PAYLOAD)
            {
              /* Limit the beacon payload's length */
              attrLen = pPib->beaconPayloadLength;
            }
            break;
        }
        type = CHCI_154_EVT_MLME_GET_CFM;
      }
      else /* CHCI_154_CMD_MLME_SET_REQ */
      {
        attrLen = pHdr->len - 2; /* TODO magic number */
        /* Set the PIB value */
        switch (attrGrp)
        {
          case PIB_PHY:
            Mac154PhyPibSetAttr(attrEnum, attrLen, pBuf);
            break;

          case PIB_VS:
            Mac154PibSetVsAttr(attrEnum, attrLen, pBuf);
            break;

          case PIB_MAC:
            if (attrEnum == MAC_154_PIB_ENUM_BEACON_PAYLOAD_LENGTH)
            {
              uint8_t bcnPydLen;

              /* Ensure beacon payload length cannot go beyond maximum */
              bcnPydLen = *pBuf;
              if (bcnPydLen > MAC_154_aMaxBeaconPayloadLength)
              {
                bcnPydLen = MAC_154_aMaxBeaconPayloadLength;
              }
              pPib->beaconPayloadLength = bcnPydLen;
            }
            else if (attrEnum == MAC_154_PIB_ENUM_BEACON_PAYLOAD)
            {
              /* Ensure beacon payload length cannot go beyond maximum */
              if (attrLen > pPib->beaconPayloadLength)
              {
                attrLen = pPib->beaconPayloadLength;
              }
              memcpy(pPib->beaconPayload, pBuf, attrLen);
            }
            else if (attrEnum == MAC_154_PIB_ENUM_PROMISCUOUS_MODE)
            {
              uint8_t nextProm = *pBuf;
              uint8_t flags = Mac154AssessRxEnable(MAC_154_RX_ASSESS_PROM, (bool_t)nextProm);
              pPib->promiscuousMode = nextProm;
              Mac154ActionRx(flags);
            }
            else if (attrEnum == MAC_154_PIB_ENUM_RX_ON_WHEN_IDLE)
            {
              uint8_t nextRXWI = *pBuf;
              uint8_t flags = Mac154AssessRxEnable(MAC_154_RX_ASSESS_RXWI, (bool_t)nextRXWI);
              pPib->rxOnWhenIdle = nextRXWI;
              Mac154ActionRx(flags);
            }
            else
            {
              Mac154PibSetAttr(attrEnum, attrLen, pBuf);
            }
            break;
        }
        type = CHCI_154_EVT_MLME_SET_CFM;
        attrLen = 0;
        pAttr = NULL;
      }
      chci154PibCfm(MAC_154_ENUM_SUCCESS, type, attrEnum, attrIdx, attrLen, pAttr);
      return TRUE;

    default:
      break;
  }
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize controller HCI for PIB operations.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154PibInit(void)
{
  Chci154RegisterCmdHandler(chci154PibCmdHandler);
}
