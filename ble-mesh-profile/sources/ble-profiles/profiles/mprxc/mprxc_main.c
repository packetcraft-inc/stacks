/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Mesh Proxy Service client.
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
#include "svc_ch.h"
#include "app_api.h"
#include "mesh_defs.h"
#include "mesh_api.h"
#include "mprxc_api.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Extracts the PDU type from the first byte of the Proxy PDU */
#define EXTRACT_PDU_TYPE(byte)    ((byte) & 0x3F)

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*!
 *  Mesh Proxy service
 */

/* Characteristics for discovery */

/*! Data In */
static const attcDiscChar_t mprxsDin =
{
  attMprxDinChUuid,
  ATTC_SET_REQUIRED
};

/*! Data Out */
static const attcDiscChar_t mprxsDout =
{
  attMprxDoutChUuid,
  ATTC_SET_REQUIRED
};

/*! Data Out CCC descriptor */
static const attcDiscChar_t mprxsDoutCcc =
{
  attCliChCfgUuid,
  ATTC_SET_REQUIRED | ATTC_SET_DESCRIPTOR
};

/*! List of characteristics to be discovered; order matches handle index enumeration  */
static const attcDiscChar_t *mprxsDiscCharList[] =
{
  &mprxsDin,                    /*! Data In */
  &mprxsDout,                   /*! Data Out */
  &mprxsDoutCcc,                /*! Data Out CCC descriptor */
};

/* sanity check:  make sure handle list length matches characteristic list length */
WSF_CT_ASSERT(MPRXC_MPRXS_HDL_LIST_LEN == ((sizeof(mprxsDiscCharList) / sizeof(attcDiscChar_t *))));


/* Control block */
static struct
{
  uint16_t       dataInHandle;              /* Data In Handle discovered by the client */
  uint16_t       dataOutHandle;             /* Data Out Handle discovered by the client */
} mprxcCb;

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
static void mprxcConnOpen(dmEvt_t *pMsg)
{
  mprxcCb.dataInHandle = ATT_HANDLE_NONE;
  mprxcCb.dataOutHandle = ATT_HANDLE_NONE;
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
static void mprxcConnClose(dmEvt_t *pMsg)
{
  /* Signal the Mesh Stack the connection ID is not available. */
  MeshRemoveGattProxyConn((meshGattProxyConnId_t)pMsg->connClose.hdr.param);
}

/*************************************************************************************************/
/*!
 *
 *  \brief  Handle an ATT Write confirm.
 *
 *  \param  pMsg     Event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void mprxcHandleWriteCnf(attEvt_t *pMsg)
{
  dmConnId_t connId = (dmConnId_t) pMsg->hdr.param;

  /* Signal GATT interface is ready to transmit packets */
  MeshSignalGattProxyIfRdy(connId);
}

/*************************************************************************************************/
/*!
 *
 *  \brief  Handle an ATT Notification.
 *
 *  \param  pMsg     Event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void mprxcHandleNotification(attEvt_t *pMsg)
{
  dmConnId_t connId = (dmConnId_t) pMsg->hdr.param;

  if (pMsg->handle == mprxcCb.dataOutHandle)
  {
    if (EXTRACT_PDU_TYPE(pMsg->pValue[0]) !=  MESH_GATT_PROXY_PDU_TYPE_PROVISIONING)
    {
      /* Received GATT Write on Data Out. Send to Mesh Stack. */
      MeshProcessGattProxyPdu((meshGattProxyConnId_t)connId, pMsg->pValue, pMsg->valueLen);
    }
  }
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \fn     MprxcMprxsDiscover
 *
 *  \brief  Perform service and characteristic discovery for Mesh Proxy service.
 *          Parameter pHdlList must point to an array of length MPRXC_MPRXS_HDL_LIST_LEN.
 *          If discovery is successful the handles of discovered characteristics and
 *          descriptors will be set in pHdlList.
 *
 *  \param  connId    Connection identifier.
 *  \param  pHdlList  Characteristic handle list.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MprxcMprxsDiscover(dmConnId_t connId, uint16_t *pHdlList)
{
  AppDiscFindService(connId, ATT_16_UUID_LEN, (uint8_t *) attMprxSvcUuid,
                     MPRXC_MPRXS_HDL_LIST_LEN, (attcDiscChar_t **) mprxsDiscCharList, pHdlList);
}

/*************************************************************************************************/
/*!
 *  \brief  Send data on the Mesh Proxy Client.
 *
 *  \param[in]  pEvt  GATT Proxy PDU send event.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MprxcSendDataIn(meshGattProxyPduSendEvt_t *pEvt)
{
  uint8_t   buf[ATT_DEFAULT_PAYLOAD_LEN];
  uint8_t   bufLen;

  if (mprxcCb.dataInHandle != ATT_HANDLE_NONE)
  {
    /* Copy in Proxy PDU. */
    bufLen = pEvt->proxyPduLen + sizeof(pEvt->proxyHdr);
    buf[0] = pEvt->proxyHdr;
    memcpy(&buf[1], pEvt->pProxyPdu, pEvt->proxyPduLen);

    AttcWriteCmd(pEvt->connId, mprxcCb.dataInHandle, bufLen, buf);
  }
}

/*************************************************************************************************/
/*!
 *  \fn     MprxcSetHandles
 *
 *  \brief  Set the handle used by the application for interacting with the Mesh
 *          Proxy service Data In characteristic.
 *
 *  \param  connId         Connection ID.
 *  \param  dataInHandle   Data In handled on the server discovered by the client.
 *  \param  dataOutHandle  Data Out handled on the server discovered by the client.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MprxcSetHandles(dmConnId_t connId, uint16_t dataInHandle, uint16_t dataOutHandle)
{
  mprxcCb.dataInHandle = dataInHandle;
  mprxcCb.dataOutHandle = dataOutHandle;

  /* Signal the Mesh Stack a new interface on the connection ID is available. */
  MeshAddGattProxyConn(connId, AttGetMtu((dmConnId_t) connId) - ATT_VALUE_NTF_LEN);
}

/*************************************************************************************************/
/*!
 *  \fn     MprxcProcMsg
 *
 *  \brief  This function is called by the application when a message that requires
 *          processing by the Mesh Proxy client is received.
 *
 *  \param  pMsg     Event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MprxcProcMsg(wsfMsgHdr_t *pMsg)
{
  switch(pMsg->event)
  {
    case DM_CONN_OPEN_IND:
      mprxcConnOpen((dmEvt_t *) pMsg);
      break;

    case DM_CONN_CLOSE_IND:
      mprxcConnClose((dmEvt_t *) pMsg);
      break;

    case ATTC_WRITE_CMD_RSP:
      mprxcHandleWriteCnf((attEvt_t *) pMsg);
      break;

    case ATTC_HANDLE_VALUE_NTF:
      mprxcHandleNotification((attEvt_t *) pMsg);
      break;

    default:
      break;
  }
}
