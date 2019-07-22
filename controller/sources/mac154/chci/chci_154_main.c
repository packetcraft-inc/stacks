/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      802.15.4 controller HCI: main implementation file.
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
#include "wsf_assert.h"
#include "util/bstream.h"

#include "chci_api.h"
#include "chci_154_int.h"
#include "mac_154_api.h"
#include "mac_154_int.h"

#include <string.h>

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief      Task event types. */
enum
{
  CHCI_154_EVT_DATA_RCVD = (1 << 0),
  CHCI_154_EVT_CMD_RCVD  = (1 << 1),
  CHCI_154_EVT_SEND_CMPL = (1 << 2)
};

/*! \brief      Main control block of 802.15.4 CHCI. */
typedef struct
{
  wsfHandlerId_t  handlerId;          /*!< Task handler Id. */

  wsfQueue_t      dataFromMacQ;       /*!< Data MAC --> CHCI queue. */
  wsfQueue_t      dataToMacQ;         /*!< Data CHCI --> MAC queue. */
  wsfQueue_t      cmdQ;               /*!< Command queue. */
  wsfQueue_t      evtQ;               /*!< Event queue. */
  bool_t          trPending;          /*!< Transport pending. */
} chci154Cb_t;

/*! \brief      Number of registered command handlers. */
static uint8_t chci154CmdHdlrNum = 0;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! \brief      Main control block of 802.15.4 CHCI. */
static chci154Cb_t chci154Cb;

/*! \brief      Command handler table. */
static Chci154CmdHandler_t chci154CmdHdlrTbl[CHCI_154_CMD_HANDLER_NUM];

/*! \brief      Data handler (only one). */
static Chci154DataHandler_t chci154DataHdlr;

/*************************************************************************************************/
/*!
 *  \brief      Unpack command header into buffer.
 *
 *  \param      pHdr      Command header.
 *  \param      pBuf      Command buffer.
 *
 *  \return     Length of header.
 */
/*************************************************************************************************/
static uint8_t chci154UnpackHdr(Chci154Hdr_t *pHdr, uint8_t *pBuf)
{
  BSTREAM_TO_UINT8 (pHdr->code, pBuf);
  BSTREAM_TO_UINT16(pHdr->len,  pBuf);

  return CHCI_154_MSG_HDR_LEN;
}

/*************************************************************************************************/
/*!
 *  \brief      Invoke a command handler.
 *
 *  \param      pHdr  Command header.
 *  \param      pBuf  Command buffer.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static bool_t chci154InvokeCmdHandler(Chci154Hdr_t *pHdr, uint8_t *pBuf)
{
  unsigned int cmdHandlerIdx;

  for (cmdHandlerIdx = 0; cmdHandlerIdx < chci154CmdHdlrNum; cmdHandlerIdx++)
  {
    if (chci154CmdHdlrTbl[cmdHandlerIdx] && chci154CmdHdlrTbl[cmdHandlerIdx](pHdr, pBuf))
    {
      return TRUE;
    }
  }

  /* Unhandled command. */
  /* TODO unhandled command warning? */
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief      Invoke data handler.
 *
 *  \param      pHdr  Data header.
 *  \param      pBuf  Data buffer.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static bool_t chci154InvokeDataHandler(Chci154Hdr_t *pHdr, uint8_t *pBuf)
{
  if (chci154DataHdlr)
  {
    chci154DataHdlr(pHdr, pBuf);
    return TRUE;
  }
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief      802.15.4 controller HCI message dispatch handler.
 *
 *  \param      event       WSF event.
 *  \param      pMsg        WSF message.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void chci154Handler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
  Chci154Hdr_t hdr;

  /* Unused parameters */
  (void)pMsg;

  if (event & CHCI_154_EVT_DATA_RCVD)
  {
    uint8_t *pDataBuf;
    wsfHandlerId_t handlerId;

    while ((pDataBuf = WsfMsgDeq(&chci154Cb.dataToMacQ, &handlerId)) != NULL)
    {
      uint8_t *pPldBuf = pDataBuf + chci154UnpackHdr(&hdr, pDataBuf);
      /* TODO unhandled command warning? */
      (void)chci154InvokeDataHandler(&hdr, pPldBuf);
      WsfMsgFree(pDataBuf);
    }
  }

  if (event & CHCI_154_EVT_CMD_RCVD)
  {
    uint8_t *pCmdBuf;
    wsfHandlerId_t handlerId;

    while ((pCmdBuf = WsfMsgDeq(&chci154Cb.cmdQ, &handlerId)) != NULL)
    {
      uint8_t *pPldBuf = pCmdBuf + chci154UnpackHdr(&hdr, pCmdBuf);
      /* TODO unhandled command warning? */
      (void)chci154InvokeCmdHandler(&hdr, pPldBuf);
      WsfMsgFree(pCmdBuf);
    }
  }

  if (event & CHCI_154_EVT_SEND_CMPL)
  {
    chci154Cb.trPending = FALSE;
    ChciTrNeedsService(CHCI_TR_PROT_15P4);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Queue a received buffer for processing by the message handler.
 *
 *  \param  type        Type of message.
 *  \param  pBuf        Pointer to received message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void chci154Recv(uint8_t type, uint8_t *pBuf)
{
  switch (type)
  {
    case CHCI_TR_TYPE_DATA:
      WsfMsgEnq(&chci154Cb.dataToMacQ, 0, pBuf);
      WsfSetEvent(chci154Cb.handlerId, CHCI_154_EVT_DATA_RCVD);
      break;

    case CHCI_TR_TYPE_CMD:
      WsfMsgEnq(&chci154Cb.cmdQ, 0, pBuf);
      WsfSetEvent(chci154Cb.handlerId, CHCI_154_EVT_CMD_RCVD);
      break;

    default:
      WsfMsgFree(pBuf);
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Signal transport send completion.
 *
 *  \param  type    Type of message.
 *  \param  pBuf    Pointer to transmitted message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void chci154SendComplete(uint8_t type, uint8_t *pBuf)
{
  switch (type)
  {
    case CHCI_TR_TYPE_EVT:
    case CHCI_TR_TYPE_DATA:
      WsfMsgFree(pBuf);
      break;

    default:
      WSF_ASSERT(FALSE);
      break;
  }

  WsfSetEvent(chci154Cb.handlerId, CHCI_154_EVT_SEND_CMPL);
}

/*************************************************************************************************/
/*!
 *  \brief  Service CHCI for transport.
 *
 *  \param  pType   Storage for type of message.
 *  \param  pLen    Storage for length of message.
 *  \param  pBuf    Storage for pointer to transmitted message.
 *
 *  \return TRUE if message ready.
 */
/*************************************************************************************************/
static bool_t chci154Service(uint8_t *pType, uint16_t *pLen, uint8_t **pBuf)
{
  uint8_t  *pBufTemp;
  uint16_t len;

  if (!chci154Cb.trPending)
  {
    wsfHandlerId_t handlerId;

    if ((pBufTemp = WsfMsgDeq(&chci154Cb.evtQ, &handlerId)) != NULL)
    {
      BYTES_TO_UINT16(len, pBufTemp + 1);

      chci154Cb.trPending = TRUE;
      *pType = CHCI_TR_TYPE_EVT;
      *pLen  = len + CHCI_154_MSG_HDR_LEN;
      *pBuf  = pBufTemp;
      return TRUE;
    }
    else if ((pBufTemp = WsfMsgDeq(&chci154Cb.dataFromMacQ, &handlerId)) != NULL)
    {
      BYTES_TO_UINT16(len, pBufTemp + 1);

      chci154Cb.trPending = TRUE;
      *pType = CHCI_TR_TYPE_DATA;
      *pLen  = len + CHCI_154_MSG_HDR_LEN;
      *pBuf  = pBufTemp;
      return TRUE;
    }
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize 802.15.4 controller HCI handler.
 *
 *  \param      None.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154HandlerInit(void)
{
  ChciTrSetCbacks(CHCI_TR_PROT_15P4, chci154Recv, chci154SendComplete, chci154Service);

  memset(&chci154Cb, 0, sizeof(chci154Cb));
  chci154Cb.handlerId = WsfOsSetNextHandler(chci154Handler);
}

/*************************************************************************************************/
/*!
 *  \brief      Register a command handler.
 *
 *  \param      cmdHandler  Command handler.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154RegisterCmdHandler(Chci154CmdHandler_t cmdHandler)
{
  /* Check it is not already registered */
  for (unsigned int i = 0; i < CHCI_154_CMD_HANDLER_NUM; i++)
  {
    if (chci154CmdHdlrTbl[i] == cmdHandler)
    {
      /* Already there; return */
      return;
    }
  }

  WSF_ASSERT(chci154CmdHdlrNum < CHCI_154_CMD_HANDLER_NUM);

  if (chci154CmdHdlrNum < CHCI_154_CMD_HANDLER_NUM)
  {
    chci154CmdHdlrTbl[chci154CmdHdlrNum++] = cmdHandler;
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Register the data handler
 *
 *  \param      dataHandler  Data handler.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154RegisterDataHandler(Chci154DataHandler_t dataHandler)
{
  chci154DataHdlr = dataHandler;
}

/*************************************************************************************************/
/*!
 *  \brief      Send an event and service the event queue.
 *
 *  \param      pBuf    Buffer containing event to send or NULL to service the queue.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154SendEvent(uint8_t *pBuf)
{
  bool_t serviceChci = TRUE;

  if (pBuf != NULL)
  {
    if (!Mac154ExecuteEvtCback(pBuf))
    {
      WsfMsgEnq(&chci154Cb.evtQ, 0, pBuf);
    }
    else
    {
      /* Message fully handled by callback - free it */
      WsfMsgFree(pBuf);
      serviceChci = FALSE;
    }
  }

  if (serviceChci && !chci154Cb.trPending)
  {
    ChciTrNeedsService(CHCI_TR_PROT_15P4);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Send data and service the data queue.
 *
 *  \param      pBuf    Buffer containing data to send or NULL to service the queue.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void Chci154SendData(uint8_t *pBuf)
{
  bool_t serviceChci = TRUE;

  if (pBuf != NULL)
  {
    if (!Mac154ExecuteDataCback(pBuf))
    {
      WsfMsgEnq(&chci154Cb.dataFromMacQ, 0, pBuf);
    }
    else
    {
      /* Message fully handled by callback - free it */
      WsfMsgFree(pBuf);
      serviceChci = FALSE;
    }
  }

  if (serviceChci && !chci154Cb.trPending)
  {
    ChciTrNeedsService(CHCI_TR_PROT_15P4);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Invoke a command handler.
 *
 *  \param      pHdr  Command header.
 *  \param      pBuf  Command buffer.
 *
 *  \return     None.
 */
/*************************************************************************************************/
bool_t Mac154InvokeCmdHandler(Mac154Hdr_t *pHdr, uint8_t *pBuf)
{
  return chci154InvokeCmdHandler((Chci154Hdr_t *)pHdr, pBuf);
}

/*************************************************************************************************/
/*!
 *  \brief      Invoke data handler.
 *
 *  \param      pHdr  Data header.
 *  \param      pBuf  Data buffer.
 *
 *  \return     None.
 */
/*************************************************************************************************/
bool_t Mac154InvokeDataHandler(Mac154Hdr_t *pHdr, uint8_t *pBuf)
{
  return chci154InvokeDataHandler((Chci154Hdr_t *)pHdr, pBuf);
}

