/*************************************************************************************************/
/*!
 *  \file   mesh_proxy_cl.c
 *
 *  \brief  Mesh Proxy Client module implementation.
 *
 *  Copyright (c) 2010-2019 Arm Ltd. All Rights Reserved.
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
 *
 */
/*************************************************************************************************/

#include "wsf_types.h"
#include "wsf_msg.h"
#include "wsf_os.h"
#include "wsf_assert.h"
#include "wsf_buf.h"
#include "util/bstream.h"
#include "wsf_trace.h"

#include "mesh_defs.h"
#include "mesh_error_codes.h"
#include "mesh_types.h"
#include "mesh_utils.h"
#include "mesh_api.h"
#include "mesh_bearer.h"
#include "mesh_local_config.h"
#include "mesh_network_if.h"
#include "mesh_proxy_main.h"
#include "mesh_proxy_cl.h"
#include "mesh_main.h"

#include <string.h>
#include <stddef.h>

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Proxy PDU type definition */
typedef union
{
  meshProxyFilterType_t filterType;
  meshAddress_t         *pAddress;
} proxyPdu_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Mesh Proxy Client Control Block */
meshProxyClCb_t meshProxyClCb =
{
  meshProxyProcessMsgEmpty
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Handles incoming decrypted Proxy Configuration PDUs from the bearer.
 *
 *  \param[in] brIfId  Unique identifier of the interface.
 *  \param[in] pPdu    Pointer to the Proxy Configuration message PDU.
 *  \param[in] pduLen  Length of the Proxy Configuration message PDU.
 *
 *  \return    None.
 */
 /*************************************************************************************************/
static void meshProxyClPduRecvCback(meshBrInterfaceId_t brIfId, const uint8_t *pPdu, uint8_t pduLen)
{
  uint8_t   opcode;
  uint16_t  listSize;
  meshProxyFilterStatusEvt_t evt;

  /* Should never happen since proxy main module validates this. */
  WSF_ASSERT(brIfId != MESH_BR_INVALID_INTERFACE_ID);
  WSF_ASSERT(pPdu != NULL);

  /* Extract Opcode */
  opcode = *pPdu;

  if ((opcode == MESH_PROXY_OPCODE_FILTER_STATUS) &&
      (pduLen == MESH_PROXY_FILTER_STATUS_TYPE_LEN) &&
      (pPdu[MESH_PROXY_FILTER_TYPE_OFFSET] <= MESH_NWK_BLACK_LIST))
  {
    /* Extract list size */
    BYTES_BE_TO_UINT16(listSize, &pPdu[MESH_PROXY_LIST_SIZE_OFFSET]);

    /* Signal event to the application */
    evt.hdr.event = MESH_CORE_EVENT;
    evt.hdr.param = MESH_CORE_PROXY_FILTER_STATUS_EVENT;
    evt.hdr.status = MESH_SUCCESS;
    evt.filterType = pPdu[MESH_PROXY_FILTER_TYPE_OFFSET];
    evt.listSize = listSize;
    meshCb.evtCback((meshEvt_t*)&evt);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles incoming events from the bearer layer.
 *
 *  \param[in] brIfId        Unique identifier of the interface.
 *  \param[in] event         Type of event that is notified.
 *  \param[in] pEventParams  Event parameters associated to the event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshBrEventNotificationCback(meshBrInterfaceId_t brIfId, meshBrEvent_t event,
                                         const meshBrEventParams_t *pEventParams)
{
  /* Should never happen as these are validated by the bearer. */
  WSF_ASSERT(brIfId != MESH_BR_INVALID_INTERFACE_ID);
  WSF_ASSERT(pEventParams != NULL);

  /* Handle events by type. */
  switch (event)
  {
    /* Handle new interface open. */
    case MESH_BR_INTERFACE_OPENED_EVT:
      /* Set black filter type on proxy client. */
      MeshNwkIfSetFilterType(brIfId, MESH_NWK_BLACK_LIST);
      break;

    case MESH_BR_INTERFACE_PACKET_SENT_EVT:
      /* Should never happen as these are validated by the bearer. */
      WSF_ASSERT(pEventParams->brPduStatus.pPdu != NULL);

      /* Free meta associated to the buffer for PDU sent over-the-air */
      WsfBufFree(pEventParams->brPduStatus.pPdu - offsetof(meshProxyPduMeta_t, pdu));
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Allocates and sends a WSF message.
 *
 *  \param[in] connId       Unique identifier for the connection on which the PDU was received.
 *                          Valid range is 0x00 to 0x1F.
 *  \param[in] netKeyIndex  Global Network Key identifier.
 *  \param[in] opcode       Proxy opcode. See ::meshProxyOpcodes.
 *  \param[in] pProxyPdu    Pointer to the Proxy Configuration message PDU.
 *  \param[in] proxyPduLen  Length of the Proxy Configuration message PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshProxyClSendMsg(meshGattProxyConnId_t connId, uint16_t netKeyIndex, uint8_t opcode,
                               proxyPdu_t proxyPdu, uint16_t proxyPduLen)
{
  meshSendProxyConfig_t *pMsg;
  uint16_t addrIndex;
  uint8_t *pPdu;

  /* Allocate the Stack Message. */
  if ((pMsg = WsfMsgAlloc(sizeof(meshSendProxyConfig_t) + proxyPduLen)) != NULL)
  {
    /* Set event type. */
    pMsg->hdr.event = MESH_MSG_API_PROXY_CFG_REQ;

    /* Add Connection ID, opcode and netKeyIndex. */
    pMsg->connId = connId;
    pMsg->netKeyIndex = netKeyIndex;
    pMsg->opcode = opcode;

    /* Add Proxy PDU location address. */
    pMsg->pProxyPdu = (uint8_t *)((uint8_t *)pMsg + sizeof(meshSendProxyConfig_t));

    /* Add Proxy PDU length. */
    pMsg->proxyPduLen = proxyPduLen;

    switch (opcode)
    {
      case MESH_PROXY_OPCODE_SET_FILTER_TYPE:
        pMsg->pProxyPdu[0] = proxyPdu.filterType;
        break;
      case MESH_PROXY_OPCODE_ADD_ADDRESS:
      case MESH_PROXY_OPCODE_REMOVE_ADDRESS:
        pPdu = pMsg->pProxyPdu;

        /* Copy addresses in Big Endian */
        for (addrIndex = 0; addrIndex < proxyPduLen / sizeof(meshAddress_t); addrIndex++)
        {
          UINT16_TO_BE_BSTREAM(pPdu, proxyPdu.pAddress[addrIndex]);
        }
        break;
      default:
        break;
    }

    /* Send Message. */
    WsfMsgSend(meshCb.handlerId, pMsg);
    return;
  }
  else
  {
    MESH_TRACE_ERR0("MESH API: Proxy Config failed. Out of memory!");
    return;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles an API WSF message.
 *
 *  \param[in] pMsg  Pointer to API WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshProxyClProcessMsg(wsfMsgHdr_t *pMsg)
{
  meshSendProxyConfig_t *pParam = (meshSendProxyConfig_t *)pMsg;

  /* Send Configuration message received from the application. */
  meshProxySendConfigMessage(MESH_BR_CONN_ID_TO_BR_IF(pParam->connId), pParam->opcode,
                             pParam->pProxyPdu, (uint8_t) pParam->proxyPduLen);

}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Proxy Client functionality.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshProxyClInit(void)
{
  /* Register callbacks. */
  meshProxyRegister(meshBrEventNotificationCback, meshProxyClPduRecvCback);

  /* Enable the message handler. */
  meshProxyClCb.msgHandlerCback = meshProxyClProcessMsg;

  if (MeshLocalCfgGetGattProxyState() == MESH_GATT_PROXY_FEATURE_NOT_SUPPORTED)
  {
    MeshLocalCfgSetGattProxyState(MESH_GATT_PROXY_FEATURE_DISABLED);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Set Filter Type configuration message to a Proxy Server.
 *
 *  \param[in] connId       Unique identifier for the connection on which the PDU was received.
 *                          Valid range is 0x00 to 0x1F.
 *  \param[in] netKeyIndex  Global Network Key identifier.
 *  \param[in] filterType   Proxy filter type. See ::meshProxyFilterType.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshProxyClSetFilterType(meshGattProxyConnId_t connId, uint16_t netKeyIndex,
                              meshProxyFilterType_t filterType)
{
  if (meshCb.initialized == TRUE)
  {
    /* Check input parameters. */
    if (MESH_GATT_PROXY_CONN_ID_IS_VALID(connId) == FALSE)
    {
      MESH_TRACE_ERR0("MESH API: Set Filter Type failed, invalid conn ID!");
      return;
    }

    if (filterType > MESH_PROXY_BLACK_LIST)
    {
      MESH_TRACE_ERR0("MESH API: Set Filter Type failed, invalid filter type!");
      return;
    }

    /* Allocate and send WSF message to the stack. */
    proxyPdu_t temp;
    temp.filterType = filterType;
    meshProxyClSendMsg(connId, netKeyIndex, MESH_PROXY_OPCODE_SET_FILTER_TYPE, temp,
                       sizeof(meshProxyFilterType_t));
  }
  else
  {
    MESH_TRACE_ERR0("MESH API: Set Filter Type failed, Mesh Stack not initialized!");
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends an Add Addresses to Filter configuration message to a Proxy Server.
 *
 *  \param[in] connId       Unique identifier for the connection on which the PDU was received.
 *                          Valid range is 0x00 to 0x1F.
 *  \param[in] netKeyIndex  Global Network Key identifier.
 *  \param[in] pAddrArray   Pointer to a buffer containing the addresses.
 *  \param[in] listSize     Address list size.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshProxyClAddToFilter(meshGattProxyConnId_t connId, uint16_t netKeyIndex,
                            meshAddress_t *pAddrArray, uint16_t listSize)
{
  if (meshCb.initialized == TRUE)
  {
    /* Check input parameters. */
    if (MESH_GATT_PROXY_CONN_ID_IS_VALID(connId) == FALSE)
    {
      MESH_TRACE_ERR0("MESH API: Add to Filter failed, invalid conn ID!");
      return;
    }

    if ((listSize == 0) || (pAddrArray == NULL))
    {
      MESH_TRACE_ERR0("MESH API: Add to Filter failed, empty list!");
      return;
    }

    /* Allocate and send WSF message to the stack. */
    proxyPdu_t temp;
    temp.pAddress = pAddrArray;
    meshProxyClSendMsg(connId, netKeyIndex, MESH_PROXY_OPCODE_ADD_ADDRESS, temp ,
                       listSize * sizeof(meshAddress_t));
  }
  else
  {
    MESH_TRACE_ERR0("MESH API: Add to Filter failed, Mesh Stack not initialized!");
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Remove Addresses from Filter configuration message to a Proxy Server.
 *
 *  \param[in] connId       Unique identifier for the connection on which the PDU was received.
 *                          Valid range is 0x00 to 0x1F.
 *  \param[in] netKeyIndex  Global Network Key identifier.
 *  \param[in] pAddrArray   Pointer to a buffer containing the addresses.
 *  \param[in] listSize     Address list size.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshProxyClRemoveFromFilter(meshGattProxyConnId_t connId, uint16_t netKeyIndex,
                                 meshAddress_t *pAddrArray, uint16_t listSize)
{
  if (meshCb.initialized == TRUE)
  {
    /* Check input parameters. */
    if (MESH_GATT_PROXY_CONN_ID_IS_VALID(connId) == FALSE)
    {
      MESH_TRACE_ERR0("MESH API: Remove from Filter failed, invalid conn ID!");
      return;
    }

    if ((listSize == 0) || (pAddrArray == NULL))
    {
      MESH_TRACE_ERR0("MESH API: Remove from Filter failed, empty list!");
      return;
    }

    /* Allocate and send WSF message to the stack. */
    proxyPdu_t temp;
    temp.pAddress = pAddrArray;
    meshProxyClSendMsg(connId, netKeyIndex, MESH_PROXY_OPCODE_REMOVE_ADDRESS, temp,
                       listSize * sizeof(meshAddress_t));
  }
  else
  {
    MESH_TRACE_ERR0("MESH API: Remove from Filter failed, Mesh Stack not initialized!");
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Checks if the node supports Proxy Client.
 *
 *  \return TRUE if Proxy Client is supported, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshProxyClIsSupported(void)
{
  /* Callback is always empty if the feature is not enabled. */
  return meshProxyClCb.msgHandlerCback != meshProxyProcessMsgEmpty;
}
