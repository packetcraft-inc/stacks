/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  L2CAP main module.
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
 */
/*************************************************************************************************/

#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_trace.h"
#include "wsf_msg.h"
#include "util/bstream.h"
#include "l2c_api.h"
#include "l2c_main.h"
#include "dm_api.h"
#include "wsf_buf.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/* Control block */
l2cCb_t l2cCb;

/*************************************************************************************************/
/*!
 *  \brief  Default callback function for unregistered CID.
 *
 *  \param  handle    The connection handle.
 *  \param  cid       The L2CAP connection ID.
 *  \param  len       The length of the L2CAP payload data in pPacket.
 *  \param  pPacket   A buffer containing the packet.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void l2cDefaultDataCidCback(uint16_t handle, uint16_t cid, uint16_t len, uint8_t *pPacket)
{
  L2C_TRACE_WARN1("unknown cid=0x%04x", cid);
}

/*************************************************************************************************/
/*!
 *  \brief  Default L2CAP control callback function.
 *
 *  \param  pMsg    Pointer to message structure.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void l2cDefaultCtrlCback(wsfMsgHdr_t *pMsg)
{
  return;
}

/*************************************************************************************************/
/*!
 *  \brief  Process received L2CAP signaling packets.
 *
 *  \param  handle    The connection handle.
 *  \param  len       The length of the L2CAP payload data in pPacket.
 *  \param  pPacket   A buffer containing the packet.
 *
 *  \return None.
 */
/*************************************************************************************************/
void l2cRxSignalingPkt(uint16_t handle, uint16_t len, uint8_t *pPacket)
{
  uint8_t role;
  dmConnId_t connId;

  if ((connId = DmConnIdByHandle(handle)) == DM_CONN_ID_NONE)
  {
    return;
  }

  role = DmConnRole(connId);

  if ((role == DM_ROLE_MASTER) && (l2cCb.masterRxSignalingPkt != NULL))
  {
    (*l2cCb.masterRxSignalingPkt)(handle, len, pPacket);
  }
  else if ((role == DM_ROLE_SLAVE) && (l2cCb.slaveRxSignalingPkt != NULL))
  {
    (*l2cCb.slaveRxSignalingPkt)(handle, len, pPacket);
  }
  else
  {
    L2C_TRACE_ERR1("Invalid role configuration: role=%d", role);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  HCI ACL data callback function.
 *
 *  \param  pPacket   A buffer containing an ACL packet.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void l2cHciAclCback(uint8_t *pPacket)
{
  uint16_t  handle;
  uint16_t  hciLen;
  uint16_t  cid;
  uint16_t  l2cLen;
  uint8_t   *p = pPacket;

  /* parse HCI handle and length */
  BSTREAM_TO_UINT16(handle, p);
  handle &= HCI_HANDLE_MASK;
  BSTREAM_TO_UINT16(hciLen, p);

  /* parse L2CAP length */
  if (hciLen >= L2C_HDR_LEN)
  {
    BSTREAM_TO_UINT16(l2cLen, p);
  }
  else
  {
    l2cLen = 0;
  }

  /* verify L2CAP length vs HCI length */
  if (hciLen == (l2cLen + L2C_HDR_LEN))
  {
    l2cCback_t *pL2cCback = l2cCb.l2cCbackQueue.pHead;

    /* parse CID */
    BSTREAM_TO_UINT16(cid, p);

    /* Search for a registered callback for this cid */
    while (pL2cCback)
    {
      if (pL2cCback->cid == cid)
      {
        break;
      }
      pL2cCback = pL2cCback->pNext;
    }

    if (pL2cCback)
    {
      /* A callback has been found, call it */
      pL2cCback->dataCback(handle, l2cLen, pPacket);
    }
    else
    {
      /* No callback is registered, call the generic callback function */
      (*l2cCb.l2cDataCidCback)(handle, cid, l2cLen, pPacket);
    }
  }
  /* else length mismatch */
  else
  {
    L2C_TRACE_WARN2("length mismatch: l2c=%u hci=%u", l2cLen, hciLen);
  }

  /* deallocate buffer */
  WsfMsgFree(pPacket);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI flow control callback function.
 *
 *  \param  handle        The connection handle.
 *  \param  flowDisabled  TRUE if data flow is disabled.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void l2cHciFlowCback(uint16_t handle, bool_t flowDisabled)
{
  wsfMsgHdr_t hdr;

  L2C_TRACE_INFO2("flowDisabled=%u handle=%u", flowDisabled, handle);

  /* get conn ID for handle */
  if ((hdr.param = DmConnIdByHandle(handle)) != DM_CONN_ID_NONE)
  {
    l2cCback_t* pL2cCback = l2cCb.l2cCbackQueue.pHead;
    /* execute higher layer flow control callbacks */
    while (pL2cCback)
    {
      if (pL2cCback->ctrlCback)
      {
        hdr.event = flowDisabled;
        pL2cCback->ctrlCback(&hdr);
      }

      pL2cCback = pL2cCback->pNext;
    }

    /* execute connection oriented channel flow control callback */
    hdr.event = flowDisabled;
    (*l2cCb.l2cCocCtrlCback)(&hdr);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Send a command reject message with reason "not understood".
 *
 *  \param  handle      The connection handle.
 *  \param  identifier  Identifier value in received message being rejected.
 *  \param  reason      Why request was rejected.
 *
 *  \return None.
 */
/*************************************************************************************************/
void l2cSendCmdReject(uint16_t handle, uint8_t identifier, uint16_t reason)
{
  uint8_t *pPacket;
  uint8_t *p;

  /* allocate msg buffer */
  if ((pPacket = l2cMsgAlloc(L2C_SIG_PKT_BASE_LEN + L2C_SIG_CMD_REJ_LEN)) != NULL)
  {
    /* build message */
    p = pPacket + L2C_PAYLOAD_START;
    UINT8_TO_BSTREAM(p, L2C_SIG_CMD_REJ);         /* command code */
    UINT8_TO_BSTREAM(p, identifier);              /* identifier */
    UINT16_TO_BSTREAM(p, L2C_SIG_CMD_REJ_LEN);    /* parameter length */
    UINT16_TO_BSTREAM(p, reason);                 /* reason */

    /* send packet */
    L2cDataReq(L2C_CID_LE_SIGNALING, handle, (L2C_SIG_HDR_LEN + L2C_SIG_CMD_REJ_LEN), pPacket);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Allocate an L2CAP data message buffer to be used for the L2CAP protocol messages.
 *
 *  \param  len   Message length in bytes.
 *
 *  \return Pointer to data message buffer or NULL if allocation failed.
 */
/*************************************************************************************************/
void *l2cMsgAlloc(uint16_t len)
{
  return WsfMsgDataAlloc(len, HCI_TX_DATA_TAILROOM);
}

/*************************************************************************************************/
/*!
 *  \brief  Initialize L2C subsystem.
 *
 *  \return None.
 */
/*************************************************************************************************/
void L2cInit(void)
{
  /* Initialize control block */
  WSF_QUEUE_INIT(&l2cCb.l2cCbackQueue);
  l2cCb.l2cCocCtrlCback = l2cDefaultCtrlCback;
  l2cCb.l2cDataCidCback = l2cDefaultDataCidCback;
  l2cCb.identifier = 1;
  L2cRegister(L2C_CID_LE_SIGNALING, l2cRxSignalingPkt, NULL);

  /* Register with HCI */
  HciAclRegister(l2cHciAclCback, l2cHciFlowCback);
}

/*************************************************************************************************/
/*!
 *  \brief  called by the L2C client, such as ATT or SMP, to register for the given CID.
 *
 *  \param  cid       channel identifier.
 *  \param  dataCback Callback function for L2CAP data received for this CID.
 *  \param  ctrlCback Callback function for control events for this CID.
 *
 *  \return None.
 */
/*************************************************************************************************/
void L2cRegister(uint16_t cid, l2cDataCback_t dataCback, l2cCtrlCback_t ctrlCback)
{
  l2cCback_t* pL2cCback = l2cCb.l2cCbackQueue.pHead;

  /* Check whether a callback structure for this cid is already registered */
  while (pL2cCback)
  {
    if (pL2cCback->cid == cid)
    {
      break;
    }
    pL2cCback = pL2cCback->pNext;
  }

  if (!pL2cCback)
  {
    /* No callback has been registered, allocate a new structure and add to list */
    pL2cCback = WsfBufAlloc(sizeof(l2cCback_t));
    pL2cCback->cid = cid;
    WsfQueueEnq(&l2cCb.l2cCbackQueue, pL2cCback);
  }

  /* Update the callbacks */
  pL2cCback->dataCback = dataCback;
  pL2cCback->ctrlCback = ctrlCback;
}

/*************************************************************************************************/
/*!
 *  \brief  Send an L2CAP data packet on the given CID.
 *
 *  \param  cid       The channel identifier.
 *  \param  handle    The connection handle.  The client receives this handle from DM.
 *  \param  len       The length of the payload data in pPacket.
 *  \param  pPacket   A buffer containing the packet.
 *
 *  \return None.
 */
/*************************************************************************************************/
void L2cDataReq(uint16_t cid, uint16_t handle, uint16_t len, uint8_t *pPacket)
{
  uint8_t *p = pPacket;

  /* Set HCI header */
  UINT16_TO_BSTREAM(p, handle);
  UINT16_TO_BSTREAM(p, (len + L2C_HDR_LEN));

  /* Set L2CAP header */
  UINT16_TO_BSTREAM(p, len);
  UINT16_TO_BSTREAM(p, cid);

  /* Send to HCI */
  HciSendAclData(pPacket);
}
