/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Mesh Proxy Service server.
 *
 *  Copyright (c) 2012-2019 Arm Ltd. All Rights Reserved.
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

#include <string.h>
#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_trace.h"
#include "svc_mprxs.h"
#include "app_api.h"
#include "mprxs_api.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Extracts the PDU type from the first byte of the Proxy PDU */
#define EXTRACT_PDU_TYPE(byte)    ((byte) & 0x3F)

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/* Control block */
static struct
{
  uint8_t       dataOutCccIdx;              /* Data Out CCCD index */
} mprxsCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \fn     mprxsConnOpen
 *
 *  \brief  Handle connection open.
 *
 *  \param  pMsg     Event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void mprxsConnOpen(dmEvt_t *pMsg)
{

}

/*************************************************************************************************/
/*!
 *  \fn     mprxsConnClose
 *
 *  \brief  Handle connection close.
 *
 *  \param  pMsg     Event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void mprxsConnClose(dmEvt_t *pMsg)
{
  /* Signal the Mesh Stack the connection ID is not available. */
  MeshRemoveGattProxyConn((meshGattProxyConnId_t)pMsg->connClose.hdr.param);
}

/*************************************************************************************************/
/*!
 *  \fn     mprxsHandleValueCnf
 *
 *  \brief  Handle an ATT handle value confirm.
 *
 *  \param  pMsg     Event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void mprxsHandleValueCnf(attEvt_t *pMsg)
{
  dmConnId_t connId = (dmConnId_t) pMsg->hdr.param;

  /* Signal GATT interface is ready to transmit packets */
  MeshSignalGattProxyIfRdy(connId);
}

/*************************************************************************************************/
/*!
 *  \fn     mprxsHandleCccdStateChangeInd
 *
 *  \brief  Handle a change of the CCCD state.
 *
 *  \param  pMsg     Event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void mprxsHandleCccdStateChangeInd(attsCccEvt_t *pMsg)
{
  /* Handle Mesh Proxy Service Data out CCC */
  if (pMsg->idx == mprxsCb.dataOutCccIdx)
  {
    if (pMsg->value == ATT_CLIENT_CFG_NOTIFY)
    {
      /* Signal the Mesh Stack a new interface on the connection ID is available. */
      MeshAddGattProxyConn((meshGattProxyConnId_t)pMsg->hdr.param,
                           AttGetMtu((dmConnId_t) pMsg->hdr.param) - ATT_VALUE_NTF_LEN);
    }
  }
}

/**************************************************************************************************
  GLobal Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \fn     MprxsProcMsg
 *
 *  \brief  This function is called by the application when a message that requires
 *          processing by the mesh proxy server is received.
 *
 *  \param  pMsg     Event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MprxsProcMsg(wsfMsgHdr_t *pMsg)
{
  switch(pMsg->event)
  {
    case DM_CONN_OPEN_IND:
      mprxsConnOpen((dmEvt_t *) pMsg);
      break;

    case DM_CONN_CLOSE_IND:
      mprxsConnClose((dmEvt_t *) pMsg);
      break;

    case ATTS_HANDLE_VALUE_CNF:
      mprxsHandleValueCnf((attEvt_t *) pMsg);
      break;

    case ATTS_CCC_STATE_IND:
      mprxsHandleCccdStateChangeInd((attsCccEvt_t *) pMsg);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \fn     MprxsWriteCback
 *
 *  \brief  ATTS write callback for mesh proxy service. Use this function as  a parameter
 *          to SvcMprxsRegister().
 *
 *  \return ATT status.
 */
/*************************************************************************************************/
uint8_t MprxsWriteCback(dmConnId_t connId, uint16_t handle, uint8_t operation, uint16_t offset,
                      uint16_t len, uint8_t *pValue, attsAttr_t *pAttr)
{
  if (EXTRACT_PDU_TYPE(pValue[0]) != MESH_GATT_PROXY_PDU_TYPE_PROVISIONING)
  {
    /* Received GATT Write on Data In. Send to Mesh Stack. */
    MeshProcessGattProxyPdu((meshGattProxyConnId_t)connId, pValue, len);
  }
  else
  {
    return ATT_ERR_INVALID_PDU;
  }

  return ATT_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \fn     MprxsSetCccIdx
 *
 *  \brief  Set the CCCD index used by the application for mesh proxy service characteristics.
 *
 *  \param  dataOutCccIdx   Data Out CCCD index.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MprxsSetCccIdx(uint8_t dataOutCccIdx)
{
  mprxsCb.dataOutCccIdx = dataOutCccIdx;
}

/*************************************************************************************************/
/*!
 *  \brief  Send data on the Mesh Proxy Server.
 *
 *  \param[in]  pEvt  GATT Proxy PDU send event.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MprxsSendDataOut(meshGattProxyPduSendEvt_t *pEvt)
{
  dmConnId_t connId = (dmConnId_t)pEvt->connId;
  uint8_t *pMsg;

  if (AttsCccEnabled(connId, mprxsCb.dataOutCccIdx))
  {
    /* Allocate ATT message. */
    pMsg = AttMsgAlloc(pEvt->proxyPduLen + sizeof(uint8_t), ATT_PDU_VALUE_NTF);

    if (pMsg != NULL)
    {
      /* Copy in Proxy PDU. */
      *pMsg = pEvt->proxyHdr;
      memcpy(&pMsg[1], pEvt->pProxyPdu, pEvt->proxyPduLen);

      /* Send notification using the local buffer. */
      AttsHandleValueNtfZeroCpy(connId, MPRXS_DOUT_HDL, pEvt->proxyPduLen + 1, pMsg);
    }
  }
}
