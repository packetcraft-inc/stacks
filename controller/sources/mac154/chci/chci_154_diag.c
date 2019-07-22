/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      802.15.4 controller HCI: Diagostics.
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

#define MAX_LEN 128

uint8_t Chci154DiagOn = 0;

/*************************************************************************************************/
/*!
 *  \brief      Diagnostic command handler.
 *
 *  \param      pHdr      Command header.
 *  \param      pBuf      Command buffer.
 *
 *  \return     TRUE if command handled.
 */
/*************************************************************************************************/

static bool_t chci154DiagCmdHandler(Chci154Hdr_t *pHdr, uint8_t *pBuf)
{
  switch (pHdr->code)
  {
    case CHCI_154_CMD_VS_DIAG_CFG_REQ:
    {
      /* Extract common parameters */
      BSTREAM_TO_UINT8(Chci154DiagOn, pBuf);
      return TRUE;
    }

    default:
      break;
  }
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief      Send diagnostics indication. Will be picked up by test app.
 *
 *  \param      pString   Pointer to string to send.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154DiagSendInd(const char *pStr, uint32_t len)
{
  uint8_t *pMsg;

  if (len > MAX_LEN-1)
  {
    len = MAX_LEN-1;
  }

  if ((pMsg = WsfMsgAlloc(CHCI_154_MSG_HDR_LEN + len)) != NULL)
  {
    uint8_t *pBuf = pMsg;

    /* Set header */
    UINT8_TO_BSTREAM (pBuf, CHCI_154_EVT_VS_DIAG_IND);
    UINT16_TO_BSTREAM(pBuf, len);

    strncpy((char *)pBuf, pStr, len);

    /* Always null terminate */
    pMsg[CHCI_154_MSG_HDR_LEN + len] = 0;

    Chci154SendEvent(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize controller HCI for association operations.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154DiagInit(void)
{
  Chci154RegisterCmdHandler(chci154DiagCmdHandler);
}
