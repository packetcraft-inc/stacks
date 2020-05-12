/*************************************************************************************************/
/*!
 *  \file   mesh_gatt_bearer.c
 *
 *  \brief  GATT bearer module implementation.
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

#include <string.h>
#include "wsf_types.h"
#include "wsf_buf.h"
#include "wsf_math.h"
#include "wsf_msg.h"
#include "wsf_cs.h"
#include "wsf_os.h"
#include "wsf_assert.h"
#include "wsf_trace.h"

#include "cfg_mesh_stack.h"
#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_error_codes.h"
#include "mesh_api.h"
#include "mesh_main.h"
#include "mesh_utils.h"
#include "mesh_bearer_defs.h"
#include "mesh_gatt_bearer.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Invalid GATT Bearer interface ID value */
#define MESH_GATT_INVALID_INTERFACE_ID      0xFF

/*! Defines the bit mask for a valid interface */
#define MESH_GATT_VALID_INTERFACE_MASK      0x0F

/*! Checks whether the interface id is a valid GATT interface id */
#define MESH_GATT_IS_VALID_INTERFACE_ID(id) \
        (MESH_UTILS_BITMASK_XCL(id, MESH_GATT_VALID_INTERFACE_MASK))

/*! Extracts the SAR value from the first byte of the Proxy PDU */
#define MESH_GATT_EXTRACT_SAR(byte)         (byte >> 6)

/*! Extracts the SAR value from the first byte of the Proxy PDU */
#define MESH_GATT_SET_SAR(byte, sar)        (byte = byte | (sar << 6))

/*! Extracts the PDU type from the first byte of the Proxy PDU */
#define MESH_GATT_EXTRACT_PDU_TYPE(byte)    (byte & 0x3F)

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh GATT Proxy WSF message events */
enum meshGattWsfMsgEvents
{
  /*! Broadcast Timer expired message event */
  MESH_GATT_MSG_PROXY_RECV_TMR_EXPIRED = MESH_GATT_PROXY_MSG_START /*!< Proxy receive timer
                                                                    *   expired
                                                                    */
};

/*! Structure containing information stored for each item in the queue */
typedef struct meshGattQueuedItem_tag
{
  void                   *pNext;      /*!< Next buffer in queue */
  const uint8_t          *pPdu;       /*!< Upper Layer PDU data */
  uint16_t               proxyPduLen; /*!< Gatt Proxy PDU length */
  uint16_t               pduOffset;   /*!< Offset of the bearer PDU inside the Upper Layer PDU */
  uint8_t                proxyHdr;    /*!< Proxy Header */
} meshGattQueuedItem_t;

/*! Definition of the GATT interface  */
typedef struct meshGattInterface_tag
{
  meshGattProxyConnId_t connId;           /*!< Unique identifier for the GATT connection */
  uint16_t              maxPduLen;        /*!< Maximum PDU that can be sent or received on the
                                           *   GATT connection
                                           */
  uint8_t               rxBrPduType;      /*!< Proxy PDU type of the received PDU */
  uint8_t               *pRxBrPdu;        /*!< Pointer to the received reassembled PDU */
  uint16_t              rxBrPduLen;       /*!< Length of the received reassembled PDU */
  wsfQueue_t            gattTxQueue;      /*!< Queue used by the GATT bearer to store information
                                           *   about the packets prepared by the upper layer to
                                           *   send over-the-air.
                                           */
  wsfTimer_t            recvTmr;          /*!< Proxy receive timeout timer */
  bool_t                gattIfBusy;       /*!< States whether the GATT interface is busy sending a
                                           *   packet over-the-air.
                                           */
  uint16_t              qHeadIdx;         /*!< Index of the queue head*/
  meshGattQueuedItem_t  qItems[MESH_GATT_QUEUE_SIZE]; /*!< FIFO queued items */
} meshGattInterface_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! MESH GATT BR control block */
static struct meshGattCb_tag
{
  meshGattProxyPduSendCback_t gattPduSendCback;               /*!< Send PDU to GATT module */
  meshGattRecvCback_t         gattPduRecvCback;               /*!< GATT Proxy PDU received callback
                                                               *   for bearer layer
                                                               */
  meshGattEventNotifyCback_t  gattBrNotifCback;               /*!< Event notification callback for
                                                               *   bearer layer
                                                               */
  meshGattInterface_t         gattInterfaces[MESH_GATT_MAX_CONNECTIONS];/*!< Array of GATT interfaces
                                                                         *   supported by the stack
                                                                         */
} gattBrCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Removes a GATT Proxy connection from the bearer.
 *
 *  \param[in] pGattIf  GATT interface.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshGattRemoveIf(meshGattInterface_t *pGattIf)
{
  meshGattQueuedItem_t  *pQueuedItem;
  meshGattBrPduStatus_t pduStatus;

  if (pGattIf->pRxBrPdu != NULL)
  {
    /* Clear pending transaction */
    WsfBufFree(pGattIf->pRxBrPdu);
    pGattIf->pRxBrPdu = NULL;
  }

  /* Reset Tx Queue */
  pGattIf->qHeadIdx = 0;
  while ((pQueuedItem = WsfQueueDeq(&pGattIf->gattTxQueue)) != NULL)
  {
    /* Send notification to Upper Layer only for complete or last segment */
    if ((MESH_GATT_EXTRACT_SAR(pQueuedItem->proxyHdr) == MESH_GATT_PROXY_PDU_SAR_COMPLETE_MSG) ||
        (MESH_GATT_EXTRACT_SAR(pQueuedItem->proxyHdr) == MESH_GATT_PROXY_PDU_SAR_LAST_SEG))
    {
      pduStatus.pPdu = (uint8_t *)pQueuedItem->pPdu;
      pduStatus.pduType = MESH_GATT_EXTRACT_PDU_TYPE(pQueuedItem->proxyHdr);
      pduStatus.eventType = MESH_GATT_PACKET_PROCESSED;
      gattBrCb.gattBrNotifCback(pGattIf->connId, (meshGattEvent_t *)&pduStatus);
    }
  }

  /* Reset information for the specified GATT interface */
  pGattIf->connId = MESH_GATT_INVALID_INTERFACE_ID;

  /* Stop timer. */
  WsfTimerStop(&(pGattIf->recvTmr));
}

/*************************************************************************************************/
/*!
 *  \brief     Closes a GATT Proxy connection due to an internal error.
 *
 *  \param[in] pGattIf  GATT interface.
 *
 *  \return    None.
 *
 *  \remarks   A connection closed event is received after calling this.
 */
/*************************************************************************************************/
static void meshGattCloseProxyConn(meshGattInterface_t *pGattIf)
{
  meshGattConnEvt_t evt =
  {
    .hdr.event = MESH_CORE_EVENT,
    .hdr.status = MESH_SUCCESS,
    .hdr.param = MESH_CORE_GATT_CONN_CLOSE_EVENT,
    .connId = pGattIf->connId };

  MESH_TRACE_INFO1("MESH GATT BR: Closing interface id %d", pGattIf->connId);

  /* Interface should have a valid value */
  WSF_ASSERT(pGattIf != NULL);

  /* Trigger generic callback. */
  meshCb.evtCback((meshEvt_t *)&evt);
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh GATT Proxy Timer callback. Maintains all active timers for GATT Proxy
 *             Rx transactions.
 *
 *  \param[in] fidx  Interface index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshGattProxyTmrCback(uint8_t ifIdx)
{
  /* Close interface */
  meshGattCloseProxyConn(&gattBrCb.gattInterfaces[ifIdx]);
}

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler callback.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshGattWsfMsgHandlerCback(wsfMsgHdr_t *pMsg)
{
  /* Check event type to handle timer expiration. */
  switch(pMsg->event)
  {
    case MESH_GATT_MSG_PROXY_RECV_TMR_EXPIRED:
      meshGattProxyTmrCback((uint8_t)(pMsg->param));
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Searches for a GATT connection ID interface in the GATT interfaces list.
 *
 *  \param[in] connId  Unique identifier for the connection on which the event was received.
 *                     Valid range is 0x00 to 0x1F.
 *
 *  \return    Pointer to GATT interface descriptor, or NULL.
 */
/*************************************************************************************************/
static meshGattInterface_t* meshGattGetInterfaceById(meshGattProxyConnId_t connId)
{
  /* Interface identifier is always valid */
  WSF_ASSERT(MESH_GATT_IS_VALID_INTERFACE_ID(connId));

  /* Search the array for a matching GATT connection ID */
  for (uint8_t idx = 0; idx < MESH_GATT_MAX_CONNECTIONS; idx++)
  {
    if (gattBrCb.gattInterfaces[idx].connId == connId)
    {
      return &gattBrCb.gattInterfaces[idx];
    }
  }

  /* No entry was found. Return invalid index */
  return NULL;
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a GATT Proxy PDU outside the Mesh Stack to be sent  over-the-air.
 *
 *  \param[in] pGattIf   GATT interface.
 *  \param[in] proxyHdr  Proxy Header.
 *  \param[in] pBrPdu    Pointer to a buffer containing a Mesh Bearer PDU.
 *  \param[in] pduLen    Size of the Mesh Bearer PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshGattTransmitPacket(meshGattInterface_t *pGattIf, uint8_t proxyHdr,
                                   const uint8_t *pBrPdu, uint16_t pduLen)
{
  meshGattProxyPduSendEvt_t evt;

  MESH_TRACE_INFO1("MESH GATT BR: Sending PDU of length %d", pduLen);

  /* Set event data */
  evt.hdr.event = MESH_GATT_PROXY_PDU_SEND;
  evt.connId = pGattIf->connId;
  evt.proxyPduLen = pduLen;
  evt.pProxyPdu = pBrPdu;

  /* Set proxy PDU header value */
  evt.proxyHdr = proxyHdr;

  /* Send PDU to GATT interface */
  gattBrCb.gattPduSendCback(&evt);

  /* Mark interface as busy */
  pGattIf->gattIfBusy = TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a GATT Proxy PDU outside the Mesh Stack to be sent  over-the-air.
 *
 *  \param[in] pGattIf   GATT interface.
 *  \param[in] proxyHdr  Proxy Header.
 *  \param[in] pBrPdu    Pointer to a buffer containing a Mesh Bearer PDU.
 *  \param[in] pduLen    Size of the Mesh Bearer PDU.
 *
 *  \return    TRUE if packet can be queued, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshGattQueuePacket(meshGattInterface_t *pGattIf, uint8_t proxyHdr,
                                  const uint8_t *pBrPdu, uint16_t pduLen)
{
  /* Check Queue availability */
  if (WsfQueueCount(&pGattIf->gattTxQueue) == MESH_GATT_QUEUE_SIZE)
  {
    MESH_TRACE_INFO0("MESH GATT BR: Queue Full");
    return FALSE;
  }

  MESH_TRACE_INFO1("MESH GATT BR: Queue PDU of length %d", pduLen);

  /* Copy in PDU data */
  pGattIf->qItems[pGattIf->qHeadIdx].pPdu = pBrPdu;
  pGattIf->qItems[pGattIf->qHeadIdx].proxyPduLen = pduLen;
  pGattIf->qItems[pGattIf->qHeadIdx].pduOffset = 0;
  pGattIf->qItems[pGattIf->qHeadIdx].proxyHdr = proxyHdr;

  /* Queue Item */
  WsfQueueEnq(&pGattIf->gattTxQueue, &pGattIf->qItems[pGattIf->qHeadIdx]);

  /* Move queue head index */
  pGattIf->qHeadIdx = (pGattIf->qHeadIdx + 1) % MESH_GATT_QUEUE_SIZE;

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Segments and queues a GATT Proxy PDU.
 *
 *  \param[in] pGattIf  GATT interface.
 *  \param[in] pduType  PDU type. See ::meshGattProxyPduType.
 *  \param[in] pBrPdu   Pointer to a buffer containing a Mesh Bearer PDU.
 *  \param[in] pduLen   Size of the Mesh Bearer PDU.
 *
 *  \return    TRUE if packet can be queued, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshGattQueueLargePacket(meshGattInterface_t *pGattIf, meshGattProxyPduType_t pduType,
                                       const uint8_t *pBrPdu, uint16_t pduLen)
{
  uint16_t pduOffset, proxyPduLen;
  uint16_t segNo;
  uint8_t proxyHdr;
  uint8_t numEntriesRequired;

  /* Calculate number of segments for this PDU */
  segNo = pduLen / (pGattIf->maxPduLen - 1);

  if (pduLen % (pGattIf->maxPduLen - 1) > 0)
  {
    segNo += 1;
  }

  /* Get number of entries required in the queue. */
  if (pGattIf->gattIfBusy)
  {
    /* All segments need queueing. */
    numEntriesRequired = (uint8_t)segNo;
  }
  else
  {
    /* All segments except the first need queueing. */
    numEntriesRequired = (uint8_t)(segNo - 1);
  }

  /* Check Queue availability */
  if (WsfQueueCount(&pGattIf->gattTxQueue) + numEntriesRequired > MESH_GATT_QUEUE_SIZE)
  {
    MESH_TRACE_INFO0("MESH GATT BR: Cannot Queue segments");
    return FALSE;
  }

  /* Start sending */
  pduOffset = 0;

  while (pduOffset < pduLen)
  {
    /* Set proxy PDU length. Take into account the header length */
    proxyPduLen = WSF_MIN(pGattIf->maxPduLen - 1, pduLen - pduOffset);

    /* Set proxy PDU header value */
    proxyHdr = pduType;

    if (pduOffset == 0)
    {
      /* This is the first segment */
      MESH_GATT_SET_SAR(proxyHdr, MESH_GATT_PROXY_PDU_SAR_FIRST_SEG);
    }
    else if(pduOffset + proxyPduLen >= pduLen)
    {
      /* This is the last segment */
      MESH_GATT_SET_SAR(proxyHdr, MESH_GATT_PROXY_PDU_SAR_LAST_SEG);
    }
    else
    {
      /* This is a continuation segment */
      MESH_GATT_SET_SAR(proxyHdr, MESH_GATT_PROXY_PDU_SAR_CONT_SEG);
    }

    if(!pGattIf->gattIfBusy)
    {
      /* Transmit first segment if interface is not busy */
      meshGattTransmitPacket(pGattIf, proxyHdr, pBrPdu, proxyPduLen);
    }
    else
    {
      /* Queue segment */
      MESH_TRACE_INFO1("MESH GATT BR: Queue PDU segment of length %d", proxyPduLen);

      /* Copy in PDU data */
      pGattIf->qItems[pGattIf->qHeadIdx].pPdu = pBrPdu;
      pGattIf->qItems[pGattIf->qHeadIdx].proxyPduLen = proxyPduLen;
      pGattIf->qItems[pGattIf->qHeadIdx].pduOffset = pduOffset;
      pGattIf->qItems[pGattIf->qHeadIdx].proxyHdr = proxyHdr;

      /* Queue Item */
      WsfQueueEnq(&pGattIf->gattTxQueue, &pGattIf->qItems[pGattIf->qHeadIdx]);

      /* Move queue head index */
      pGattIf->qHeadIdx = (pGattIf->qHeadIdx + 1) % MESH_GATT_QUEUE_SIZE;
    }

    /* Move to then next segment */
    pduOffset += proxyPduLen;
  }

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the maximum PDU length for a specified PDU type.
 *
 *  \param[in] pduType  Type of the Proxy PDU. See ::meshGattProxyPduTypes;
 *
 *  \return    Maximum PDU Length. The current spec limit is set by the max provisioning PDU length.
 */
/*************************************************************************************************/
static uint8_t meshGattGetMaxPduLen(meshGattProxyPduType_t pduType)
{
  uint8_t pduLen;

  switch (pduType)
  {
    case MESH_GATT_PROXY_PDU_TYPE_NETWORK_PDU:
      pduLen = MESH_NWK_MAX_PDU_LEN;
      break;

    case MESH_GATT_PROXY_PDU_TYPE_BEACON:
      pduLen = MESH_NWK_BEACON_NUM_BYTES;
      break;

    case MESH_GATT_PROXY_PDU_TYPE_PROVISIONING:
      pduLen = MESH_PRV_MAX_PDU_LEN;
      break;
    case MESH_GATT_PROXY_PDU_TYPE_CONFIGURATION:
      pduLen = MESH_PRV_MAX_PDU_LEN;
      break;

    default:
      pduLen = 0;
      break;
  }

  return pduLen;
}

/*************************************************************************************************/
/*!
 *  \brief    Starts a segmented RX transaction on the GATT interface.
 *
 *  \param[in] pGattIf      GATT interface.
 *  \param[in] pProxyPdu    Pointer to a buffer containing the GATT Proxy PDU.
 *  \param[in] proxyPduLen  Size of the buffer referenced by pProxyPdu.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshGattStartRxTransaction(meshGattInterface_t *pGattIf, const uint8_t *pProxyPdu,
                                       uint16_t proxyPduLen)
{
  meshGattProxyPduType_t pduType;

  /* Extract PDU type */
  pduType = MESH_GATT_EXTRACT_PDU_TYPE(pProxyPdu[0]);

  /* Received Start segment. Allocate buffer for the full PDU depending on PDU type. */
  pGattIf->pRxBrPdu = WsfBufAlloc(meshGattGetMaxPduLen(pduType));

  if (pGattIf->pRxBrPdu == NULL)
  {
    /* If first segment cannot be allocated, the rest will fail. So, disconnect early. */
    meshGattCloseProxyConn(pGattIf);
    return;
  }

  /* Set type, length and copy PDU contents. */
  pGattIf->rxBrPduType = pduType;
  pGattIf->rxBrPduLen = proxyPduLen - 1;
  memcpy(pGattIf->pRxBrPdu, &pProxyPdu[1], pGattIf->rxBrPduLen);

  /* Start timeout timer. */
  WsfTimerStartSec(&(pGattIf->recvTmr), MESH_GATT_PROXY_TIMEOUT_SEC);
}

/*************************************************************************************************/
/*!
 *  \brief    Ends a segmented RX transaction on the GATT interface.
 *
 *  \param[in] pGattIf      GATT interface.
 *  \param[in] pProxyPdu    Pointer to a buffer containing the GATT Proxy PDU.
 *  \param[in] proxyPduLen  Size of the buffer referenced by pProxyPdu.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshGattEndRxTransaction(meshGattInterface_t *pGattIf, const uint8_t *pProxyPdu,
                                     uint16_t proxyPduLen)
{
  /* Set length and copy PDU contents. */
  memcpy(&pGattIf->pRxBrPdu[pGattIf->rxBrPduLen], &pProxyPdu[1], proxyPduLen - 1);
  pGattIf->rxBrPduLen += (proxyPduLen - 1);

  /* Received full PDU */
  gattBrCb.gattPduRecvCback(pGattIf->connId,  pGattIf->rxBrPduType, pGattIf->pRxBrPdu,
                            pGattIf->rxBrPduLen);

  /* Free buffer */
  WsfBufFree(pGattIf->pRxBrPdu);
  pGattIf->pRxBrPdu = NULL;

  /* Stop timer. */
  WsfTimerStop(&(pGattIf->recvTmr));
}

/*************************************************************************************************/
/*!
 *  \brief     Empty notification callback to upper layer.
 *
 *  \param[in] ifId    Interface Id.
 *  \param[in] pEvent  Event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void gattBrEmptyNotifCback(meshGattProxyConnId_t connId, const meshGattEvent_t *pEvent)
{
  (void)connId;
  (void)pEvent;
  MESH_TRACE_ERR0("MESH GATT BR: Notif cback not registered ");
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh GATT Proxy PDU received callback function pointer type.
 *
 *  \param[in] connId   Unique identifier for the connection on which the PDU was received.
 *                      Valid range is 0x00 to 0x1F.
 *  \param[in] pduType  PDU type. See ::meshGattProxyPduType.
 *  \param[in] pBrPdu   Pointer to a buffer containing the GATT Proxy PDU.
 *  \param[in] pduLen   Size of the buffer referenced by pProxyPdu.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void gattBrEmptyPduRecvCback(meshGattProxyConnId_t connId, meshGattProxyPduType_t pduType,
                                    const uint8_t *pBrPdu, uint16_t pduLen)
{
  (void)connId;
  (void)pduType;
  (void)pBrPdu;
  (void)pduLen;
  MESH_TRACE_ERR0("MESH GATT BR: PDU receive cback not registered ");
}

/*************************************************************************************************/
/*!
 *  \brief     Empty PDU send callback to external module.
 *
 *  \param[in] pEvt  GATT Proxy PDU Send event type .
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void gattBrEmptyPduSendCback(meshGattProxyPduSendEvt_t *pEvt)
{
  (void)pEvt;
  MESH_TRACE_ERR0("MESH GATT BR: PDU send cback not registerd ");
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the MESH GATT BR layer.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshGattInit(void)
{
  MESH_TRACE_INFO0("MESH GATT BR: init");

  /* Set callbacks */
  gattBrCb.gattBrNotifCback = gattBrEmptyNotifCback;
  gattBrCb.gattPduRecvCback = gattBrEmptyPduRecvCback;
  gattBrCb.gattPduSendCback = gattBrEmptyPduSendCback;

  /* Initialize the interfaces array */
  for (uint8_t i = 0; i < MESH_GATT_MAX_CONNECTIONS; i++)
  {
    gattBrCb.gattInterfaces[i].connId = MESH_GATT_INVALID_INTERFACE_ID;
    gattBrCb.gattInterfaces[i].qHeadIdx = 0;

    /* Initialize timer */
    gattBrCb.gattInterfaces[i].recvTmr.msg.event = MESH_GATT_MSG_PROXY_RECV_TMR_EXPIRED;
    gattBrCb.gattInterfaces[i].recvTmr.msg.param = i;
    gattBrCb.gattInterfaces[i].recvTmr.handlerId = meshCb.handlerId;
  }

  /* Register WSF message handler callback. */
  meshCb.gattProxyMsgCback = meshGattWsfMsgHandlerCback;
}

/*************************************************************************************************/
/*!
 *  \brief     Register upper layer callbacks.
 *
 *  \param[in] recvCback   Callback to be invoked when a Mesh GATT PDU is received on a specific
 *                         GATT connection.
 *  \param[in] notifCback  Callback to be invoked when an event on a specific GATT connection
 *                         is triggered.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshGattRegister(meshGattRecvCback_t pduRecvCback, meshGattEventNotifyCback_t evtCback)
{
  if ((evtCback != NULL) && (pduRecvCback != NULL))
  {
    gattBrCb.gattBrNotifCback = evtCback;
    gattBrCb.gattPduRecvCback = pduRecvCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Register callback to send PDU to bearer module.
 *
 *  \param[in] cback  Callback to be invoked to send a GATT Proxy PDU outside the stack.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshGattRegisterPduSendCback(meshGattProxyPduSendCback_t cback)
{
  if (cback != NULL)
  {
    gattBrCb.gattPduSendCback = cback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Adds a new GATT Proxy connection into the bearer.
 *
 *  \param[in] connId       Unique identifier for the connection. Valid range 0x00 to 0x1F.
 *  \param[in] maxProxyPdu  Maximum size of the Proxy PDU, derived from GATT MTU.
 *
 *  \return    None.
 *
 *  \remarks   If GATT Proxy is supported and this the first connection, it also enables proxy.
 */
/*************************************************************************************************/
void MeshGattAddProxyConn(meshGattProxyConnId_t connId, uint16_t maxProxyPdu)
{
  meshGattEventType_t gattEvt;
  meshGattConnEvt_t evt =
  {
    .hdr.event = MESH_CORE_EVENT,
    .hdr.status = MESH_SUCCESS,
    .hdr.param = MESH_CORE_GATT_CONN_ADD_EVENT,
    .connId = connId
  };
  uint8_t emptyIdx;

  MESH_TRACE_INFO1("MESH GATT BR: adding connection id %d", connId);

  /* Interface Id should have a valid value */
  WSF_ASSERT(MESH_GATT_IS_VALID_INTERFACE_ID(connId));

  /* Check for duplicate GATT connection ID */
  if (meshGattGetInterfaceById(connId) != NULL)
  {
    MESH_TRACE_WARN1("MESH GATT BR: duplicate connection id %d", connId);

    /* Set event status to error. */
    evt.hdr.status = MESH_ALREADY_EXISTS;
  }
  else
  {
    /* Search for an empty entry */
    for (emptyIdx = 0; emptyIdx < MESH_GATT_MAX_CONNECTIONS; emptyIdx++)
    {
      if (gattBrCb.gattInterfaces[emptyIdx].connId == MESH_GATT_INVALID_INTERFACE_ID)
      {
        /* Empty entry found. Populate information */
        gattBrCb.gattInterfaces[emptyIdx].connId = connId;
        gattBrCb.gattInterfaces[emptyIdx].maxPduLen = maxProxyPdu;
        gattBrCb.gattInterfaces[emptyIdx].gattIfBusy = FALSE;

        /* Signal the upper layer that the interface was opened */
        gattEvt = MESH_GATT_PROXY_CONN_OPENED;
        gattBrCb.gattBrNotifCback(connId, (meshGattEvent_t *)&gattEvt);

        /* Entry found. Break search sequence */
        break;
      }
    }

    /* No empty interface was found */
    if (emptyIdx == MESH_GATT_MAX_CONNECTIONS)
    {
      /* Set event status to error. */
      evt.hdr.status = MESH_NO_RESOURCES;
    }
  }

  /* Trigger generic callback. */
  meshCb.evtCback((meshEvt_t *)&evt);
}

/*************************************************************************************************/
/*!
 *  \brief     Removes a GATT Proxy connection from the bearer.
 *
 *  \param[in] connId  Unique identifier for a connection. Valid range is 0x00 to 0x1F.
 *
 *  \return    None.
 *
 *  \remarks   A connection removed event is received after calling this.
 */
/*************************************************************************************************/
void MeshGattRemoveProxyConn(meshGattProxyConnId_t connId)
{
  meshGattInterface_t *pGattIf;
  meshGattEventType_t gattEvt;
  meshGattConnEvt_t evt =
  {
    .hdr.event = MESH_CORE_EVENT,
    .hdr.status = MESH_SUCCESS,
    .hdr.param = MESH_CORE_GATT_CONN_REMOVE_EVENT,
    .connId = connId
  };

  MESH_TRACE_INFO1("MESH GATT BR: removing conn id %d", connId);

  /* Interface Id should have a valid value */
  WSF_ASSERT(MESH_GATT_IS_VALID_INTERFACE_ID(connId));

  /* Get interface */
  pGattIf = meshGattGetInterfaceById(connId);

  /* If interface is not found, return error */
  if (pGattIf == NULL)
  {
    /* Set event status to error. */
    evt.hdr.status = MESH_INVALID_PARAM;
  }
  else
  {
    meshGattRemoveIf(pGattIf);

    /* Signal the upper layer that the interface was closed */
    gattEvt = MESH_GATT_PROXY_CONN_CLOSED;
    gattBrCb.gattBrNotifCback(connId, (meshGattEvent_t *)&gattEvt);
  }

  /* Trigger generic callback. */
  meshCb.evtCback((meshEvt_t *)&evt);
}

/*************************************************************************************************/
/*!
 *  \brief     Processes a GATT Proxy PDU received on a GATT interface.
 *
 *  \param[in] connId       Unique identifier for the connection on which the PDU was received.
 *                          Valid range is 0x00 to 0x1F.
 *  \param[in] pProxyPdu    Pointer to a buffer containing the GATT Proxy PDU.
 *  \param[in] proxyPduLen  Size of the buffer referenced by pProxyPdu.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshGattProcessPdu(meshGattProxyConnId_t connId, const uint8_t *pProxyPdu,
                        uint16_t proxyPduLen)
{
  uint8_t sar;
  meshGattProxyPduType_t  pduType;
  meshGattInterface_t *pGattIf;

  /* Interface Id should have a valid value. Pointer to PDU should not be NULL */
  WSF_ASSERT(MESH_GATT_IS_VALID_INTERFACE_ID(connId));
  WSF_ASSERT(pProxyPdu != NULL);

  MESH_TRACE_INFO1("MESH GATT BR: Receiving PDU of length %d", proxyPduLen);

  /* Get interaface. */
  pGattIf = meshGattGetInterfaceById(connId);

  if (pGattIf != NULL)
  {
    /* Check length of proxu PDU is greater than the header length (1o) */
    if (proxyPduLen <= 1)
    {
      meshGattCloseProxyConn(pGattIf);
      return;
    }

    /* Extract and validate header fields from PDU */
    sar = MESH_GATT_EXTRACT_SAR(pProxyPdu[0]);
    pduType = MESH_GATT_EXTRACT_PDU_TYPE(pProxyPdu[0]);

    /* Receiving an unexpected value of the SAR field triggers a disconnect. */
    if (sar > MESH_GATT_PROXY_PDU_SAR_LAST_SEG)
    {
      meshGattCloseProxyConn(pGattIf);
      return;
    }

    /* Ignore message when message type is set to RFU value. */
    if (pduType > MESH_GATT_PROXY_PDU_TYPE_PROVISIONING)
    {
      return;
    }

    if (pGattIf->pRxBrPdu == NULL)
    {
      /* No transaction is pending. Expecting first segment of a PDU or a full PDU. */
      switch (sar)
      {
        case MESH_GATT_PROXY_PDU_SAR_COMPLETE_MSG:
          /* Received full PDU. Remove header. Check maximum PDU Length.
           * Ignore Proxy header of 1 octet.
           */
          if ((proxyPduLen - 1) <= meshGattGetMaxPduLen(pduType))
          {
            gattBrCb.gattPduRecvCback(connId, pduType, &pProxyPdu[1], proxyPduLen - 1);
          }
          else
          {
            meshGattCloseProxyConn(pGattIf);
          }
          break;

        case MESH_GATT_PROXY_PDU_SAR_FIRST_SEG:
          /*  Reject first fragment equal to max length, as it will not be able to process
           *  a continuation fragment.
           */
          if ((proxyPduLen - 1) < meshGattGetMaxPduLen(pduType))
          {
            meshGattStartRxTransaction(pGattIf, pProxyPdu, proxyPduLen);
          }
          break;

        default:
          meshGattCloseProxyConn(pGattIf);
          break;
      }
    }
    else
    {
      /* Close connection if a different PDU type is received or receive buffer could overflow. */
      if ((pduType != pGattIf->rxBrPduType) ||
          (pGattIf->rxBrPduLen + proxyPduLen - 1 > meshGattGetMaxPduLen(pduType)))
      {
        meshGattCloseProxyConn(pGattIf);
        return;
      }

      /* Transaction is pending. Expecting continuation or last segment of a PDU */
      switch (sar)
      {
        case MESH_GATT_PROXY_PDU_SAR_CONT_SEG:
          /* Set length and copy PDU contents. */
          memcpy(&pGattIf->pRxBrPdu[pGattIf->rxBrPduLen], &pProxyPdu[1], proxyPduLen - 1);
          pGattIf->rxBrPduLen += (proxyPduLen - 1);
          break;

        case MESH_GATT_PROXY_PDU_SAR_LAST_SEG:
          meshGattEndRxTransaction(pGattIf, pProxyPdu, proxyPduLen);
          break;

        default:
          meshGattCloseProxyConn(pGattIf);
          break;
      }
    }
  }
}


/*************************************************************************************************/
/*!
 *  \brief     Asynchronously sends a Mesh Bearer PDU on a GATT Proxy interface.
 *
 *  \param[in] connId   Unique identifier for the connection on which the PDU was received.
 *                      Valid range is 0x00 to 0x1F.
 *  \param[in] pduType  PDU type. See ::meshGattProxyPduType.
 *  \param[in] pBrPdu   Pointer to a buffer containing a Mesh Bearer PDU.
 *  \param[in] pduLen   Size of the Mesh Bearer PDU.
 *
 *  \return    TRUE if message is sent or queued for later transmission, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshGattSendBrPdu(meshGattProxyConnId_t connId, meshGattProxyPduType_t pduType,
                         const uint8_t *pBrPdu, uint16_t pduLen)
{
  meshGattInterface_t *pGattIf;
  uint8_t proxyHdr;
  meshGattBrPduStatus_t pduStatus;

  /* Input parameters should have valid values */
  WSF_ASSERT(MESH_GATT_IS_VALID_INTERFACE_ID(connId));
  WSF_ASSERT(pBrPdu != NULL);
  WSF_ASSERT(pduLen != 0);
  WSF_ASSERT(pduType <= MESH_GATT_PROXY_PDU_TYPE_PROVISIONING);

  /* Get interface */
  pGattIf = meshGattGetInterfaceById(connId);

  /* Check if the interface is valid */
  if (pGattIf == NULL)
  {
    MESH_TRACE_ERR0("MESH GATT BR: Invalid Interface. ");
    return FALSE;
  }

  /* Check PDU length. Take into account the Proxy header length. */
  if ((pduLen + sizeof(uint8_t)) > pGattIf->maxPduLen)
  {
    /* Packet needs segmenting */
    return meshGattQueueLargePacket(pGattIf, pduType, pBrPdu, pduLen);
  }
  else
  {
    /* Single PDU. Check availability of interface*/
    if (!pGattIf->gattIfBusy)
    {
      /* Set proxy PDU header value */
      proxyHdr = pduType;
      MESH_GATT_SET_SAR(proxyHdr, MESH_GATT_PROXY_PDU_SAR_COMPLETE_MSG);

      /* Transmit packet */
      meshGattTransmitPacket(pGattIf, proxyHdr, pBrPdu, pduLen);

      /* Send notification to Upper Layer */
      pduStatus.pPdu = (uint8_t *)pBrPdu;
      pduStatus.pduType = MESH_GATT_EXTRACT_PDU_TYPE(proxyHdr);
      pduStatus.eventType = MESH_GATT_PACKET_PROCESSED;
      gattBrCb.gattBrNotifCback(pGattIf->connId, (meshGattEvent_t *)&pduStatus);
    }
    else
    {
      return meshGattQueuePacket(pGattIf, pduType, pBrPdu, pduLen);
    }
  }

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Closes a GATT Proxy connection.
 *
 *  \param[in] connId  GATT connection identifier.
 *
 *  \return    None.
 *
 *  \remarks   A connection closed event is received after calling this.
 */
/*************************************************************************************************/
void MeshGattCloseProxyConn(meshGattProxyConnId_t connId)
{
  meshGattInterface_t *pGattIf;

  WSF_ASSERT(MESH_GATT_IS_VALID_INTERFACE_ID(connId));

  /* Get interface */
  pGattIf = meshGattGetInterfaceById(connId);

  /* If interface is not found, return error */
  if (pGattIf != NULL)
  {
    meshGattCloseProxyConn(pGattIf);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Adds a new GATT Proxy connection into the bearer.
 *
 *  \param[in] connId  Unique identifier for the connection. Valid range 0x00 to 0x1F.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshGattSignalIfReady(meshGattProxyConnId_t connId)
{
  meshGattInterface_t *pGattIf;
  meshGattQueuedItem_t  *pQueuedItem;
  meshGattConnEvt_t evt =
  {
    .hdr.event = MESH_CORE_EVENT,
    .hdr.status = MESH_SUCCESS,
    .hdr.param = MESH_CORE_GATT_SIGNAL_IF_RDY_EVENT,
    .connId = connId
  };
  meshGattBrPduStatus_t pduStatus;

  MESH_TRACE_INFO1("MESH GATT BR: Signal interface ready id %d", connId);

  /* Interface Id should have a valid value */
  WSF_ASSERT(MESH_GATT_IS_VALID_INTERFACE_ID(connId));

  /* Get GATT connection ID */
  if ((pGattIf = meshGattGetInterfaceById(connId)) == NULL)
  {
    MESH_TRACE_ERR1("MESH GATT BR: Invalid interface id %d", connId);

    /* Set event status to error. */
    evt.hdr.status = MESH_INVALID_INTERFACE;
  }
  else
  {
    /* Mark interface as not busy */
    pGattIf->gattIfBusy = FALSE;

    if ((pQueuedItem = WsfQueueDeq(&pGattIf->gattTxQueue)) != NULL)
    {
      /* Send queued item */
      meshGattTransmitPacket(pGattIf, pQueuedItem->proxyHdr,
                             &pQueuedItem->pPdu[pQueuedItem->pduOffset], pQueuedItem->proxyPduLen);

      /* Send notification to Upper Layer only for complete or last segment */
      if ((MESH_GATT_EXTRACT_SAR(pQueuedItem->proxyHdr) == MESH_GATT_PROXY_PDU_SAR_COMPLETE_MSG) ||
          (MESH_GATT_EXTRACT_SAR(pQueuedItem->proxyHdr) == MESH_GATT_PROXY_PDU_SAR_LAST_SEG))
      {
        pduStatus.pPdu = (uint8_t *)pQueuedItem->pPdu;
        pduStatus.pduType = MESH_GATT_EXTRACT_PDU_TYPE(pQueuedItem->proxyHdr);
        pduStatus.eventType = MESH_GATT_PACKET_PROCESSED;
        gattBrCb.gattBrNotifCback(pGattIf->connId, (meshGattEvent_t *)&pduStatus);
      }
    }
  }

  /* Trigger generic callback. */
  meshCb.evtCback((meshEvt_t *)&evt);
}
