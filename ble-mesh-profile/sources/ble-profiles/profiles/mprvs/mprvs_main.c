/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Mesh Provisioning Service client.
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
#include "svc_mprvs.h"
#include "app_api.h"
#include "mesh_api.h"
#include "mprvs_api.h"

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
#include "mesh_test_api.h"
#include "mesh_error_codes.h"
#endif

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
} mprvsCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *
 *  \brief  Handle connection open.
 *
 *  \param  pMsg     Event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void mprvsConnOpen(dmEvt_t *pMsg)
{

}

/*************************************************************************************************/
/*!
 *
 *  \brief  Handle connection close.
 *
 *  \param  pMsg     Event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void mprvsConnClose(dmEvt_t *pMsg)
{
  /* Signal the Mesh Stack the connection ID is not available. */
  MeshRemoveGattProxyConn((meshGattProxyConnId_t)pMsg->connClose.hdr.param);
}

/*************************************************************************************************/
/*!
 *
 *  \brief  Handle an ATT handle value confirm.
 *
 *  \param  pMsg     Event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void mprvsHandleValueCnf(attEvt_t *pMsg)
{
  dmConnId_t connId = (dmConnId_t) pMsg->hdr.param;

  /* Signal GATT interface is ready to transmit packets */
  MeshSignalGattProxyIfRdy(connId);
}

/*************************************************************************************************/
/*!
 *
 *  \brief  Handle a change of the CCCD state.
 *
 *  \param  pMsg     Event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void mprvsHandleCccdStateChangeInd(attsCccEvt_t *pMsg)
{
  /* Handle Mesh Proxy Service Data out CCC */
  if (pMsg->idx == mprvsCb.dataOutCccIdx)
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
 *
 *  \brief  This function is called by the application when a message that requires
 *          processing by the Mesh Provisioning server is received.
 *
 *  \param  pMsg     Event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MprvsProcMsg(wsfMsgHdr_t *pMsg)
{
  switch(pMsg->event)
  {
    case DM_CONN_OPEN_IND:
      mprvsConnOpen((dmEvt_t *) pMsg);
      break;

    case DM_CONN_CLOSE_IND:
      mprvsConnClose((dmEvt_t *) pMsg);
      break;

    case ATTS_HANDLE_VALUE_CNF:
      mprvsHandleValueCnf((attEvt_t *) pMsg);
      break;

    case ATTS_CCC_STATE_IND:
      mprvsHandleCccdStateChangeInd((attsCccEvt_t *) pMsg);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *
 *  \brief  ATTS write callback for mesh provisioning service. Use this function as  a parameter
 *          to SvcMprvsRegister().
 *
 *  \return ATT status.
 */
/*************************************************************************************************/
uint8_t MprvsWriteCback(dmConnId_t connId, uint16_t handle, uint8_t operation, uint16_t offset,
                      uint16_t len, uint8_t *pValue, attsAttr_t *pAttr)
{
  if (EXTRACT_PDU_TYPE(pValue[0]) == MESH_GATT_PROXY_PDU_TYPE_PROVISIONING)
  {
    /* Received GATT Write on Data In. Send to Mesh Stack. */
    MeshProcessGattProxyPdu((meshGattProxyConnId_t)connId, pValue, len);
  }
  else
  {
#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
    meshTestMprvsWriteInvalidRcvdInd_t mprvsWriteInvalidRcvdInd;

    mprvsWriteInvalidRcvdInd.hdr.event = MESH_TEST_EVENT;
    mprvsWriteInvalidRcvdInd.hdr.param = MESH_TEST_MPRVS_WRITE_INVALID_RCVD_IND;
    mprvsWriteInvalidRcvdInd.hdr.status = MESH_SUCCESS;
    mprvsWriteInvalidRcvdInd.handle = handle;
    mprvsWriteInvalidRcvdInd.pValue = pValue;
    mprvsWriteInvalidRcvdInd.len = len;

    meshTestCb.testCback((meshTestEvt_t *)&mprvsWriteInvalidRcvdInd);
#endif

    return ATT_ERR_INVALID_PDU;
  }

  return ATT_SUCCESS;
}

/*************************************************************************************************/
/*!
 *
 *  \brief  Set the CCCD index used by the application for mesh provisioning service characteristics.
 *
 *  \param  dataOutCccIdx   Data Out CCCD index.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MprvsSetCccIdx(uint8_t dataOutCccIdx)
{
  mprvsCb.dataOutCccIdx = dataOutCccIdx;
}

/*************************************************************************************************/
/*!
 *  \brief  Send data on the Mesh Provisioning Server.
 *
 *  \param[in]  pEvt  GATT Proxy PDU send event.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MprvsSendDataOut(meshGattProxyPduSendEvt_t *pEvt)
{
  dmConnId_t connId = (dmConnId_t)pEvt->connId;
  uint8_t *pMsg;

  if (AttsCccEnabled(connId, mprvsCb.dataOutCccIdx))
  {
    /* Allocate ATT message. */
    pMsg = AttMsgAlloc(pEvt->proxyPduLen + sizeof(uint8_t), ATT_PDU_VALUE_NTF);

    if (pMsg != NULL)
    {
      /* Copy in Proxy PDU. */
      *pMsg = pEvt->proxyHdr;
      memcpy(&pMsg[1], pEvt->pProxyPdu, pEvt->proxyPduLen);

      /* Send notification using the local buffer. */
      AttsHandleValueNtfZeroCpy(connId, MPRVS_DOUT_HDL, pEvt->proxyPduLen + sizeof(uint8_t),
                                pMsg);
    }
  }
}
