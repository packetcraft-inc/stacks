/*************************************************************************************************/
/*!
 *  \file   mesh_bearer.c
 *
 *  \brief  Bearer module implementation.
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
#include "wsf_trace.h"
#include "wsf_assert.h"

#include "mesh_defs.h"
#include "mesh_network_beacon_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"
#include "mesh_utils.h"
#include "mesh_bearer_defs.h"
#include "mesh_adv_bearer.h"
#include "mesh_gatt_bearer.h"
#include "mesh_bearer.h"

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
#include "mesh_test_api.h"
#include "mesh_error_codes.h"
#endif

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Mesh Control Block */
static struct meshBrCb_tag
{
  meshBrEventNotifyCback_t    brNwkEventCback;           /*! Event notification callback for Network
                                                          */
  meshBrEventNotifyCback_t    brNwkBeaconEventCback;     /*!< Event notification callback for Secure
                                                          *   Network Beacon
                                                          */
  meshBrEventNotifyCback_t    brPbEventCback;            /*! Event notification callback for
                                                          *  Provisioning Bearer
                                                          */
  meshBrEventNotifyCback_t    brPbBeaconEventCback;      /*! Event notification callback for
                                                          *  Unprovisioned Device Beacon
                                                          */
  meshBrNwkPduRecvCback_t     brNwkPduRecvCback;         /*! Network PDU received callback */
  meshBrBeaconRecvCback_t     brNwkBeaconPduRecvCback;   /*! Secure Beacon PDU received callback */
  meshBrPbPduRecvCback_t      brPbPduRecvCback;          /*! Provisioning Bearer PDU received
                                                          *  callback
                                                          */
  meshBrBeaconRecvCback_t     brPbBeaconPduRecvCback;    /*! Unprovisioned Device Beacon PDU
                                                          *  received callback
                                                          */
  meshBrNwkPduRecvCback_t     brProxyMsgRecvCback;       /*! Proxy message received callback for
                                                          *  upper layer. Message is similar to a
                                                          *  NWK PDU.
                                                          */
  meshBrEventNotifyCback_t    brProxyEventCback;         /*! Event notification callback for upper
                                                          *  layer
                                                          */
} brCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Callback triggered when an advertising PDU is received on an advertising bearer
 *             interface.
 *
 *  \param[in] advIfId  Unique advertising interface identifier.
 *  \param[in] advType  ADV type received. See ::meshAdvType.
 *  \param[in] pBrPdu   Pointer to the bearer PDU received.
 *  \param[in] pduLen   Size of the bearer PDU received.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshBrProcessAdvPduCback(meshAdvIfId_t advIfId, meshAdvType_t  advType,
                                     const uint8_t *pBrPdu, uint8_t pduLen)
{
  /* Advertising interface occupies only the least significant nibble. It will be checked
   * by the Advertising Bearer
   */
  WSF_ASSERT(advIfId <= MESH_BR_INTERFACE_ID_INTERFACE_MASK);

  /* AD type should be Mesh Beacon or Mesh Packet*/
  WSF_ASSERT((advType == MESH_AD_TYPE_PACKET) || (advType == MESH_AD_TYPE_BEACON) ||
             (advType == MESH_AD_TYPE_PB));

  /* Check for valid input parameters */
  WSF_ASSERT(pBrPdu != NULL);
  WSF_ASSERT(pduLen != 0);

  /* Send PDU to specified bearer type interface */
  switch (advType)
  {
    case MESH_AD_TYPE_PACKET:
      /* Network PDU received. Call CB registered by upper layer */
      brCb.brNwkPduRecvCback(MESH_BR_ADV_IF_TO_BR_IF(advIfId), pBrPdu, pduLen);
      break;

    case MESH_AD_TYPE_BEACON:
      if (pBrPdu[0] == MESH_BEACON_TYPE_UNPROV)
      {
        /* Unprovisioned Device Beacon received. Call CB registered by upper layer */
        brCb.brPbBeaconPduRecvCback(MESH_BR_ADV_IF_TO_BR_IF(advIfId), pBrPdu, pduLen);
      }
      else if (pBrPdu[0] == MESH_BEACON_TYPE_SEC_NWK)
      {
        /* Secure Network Beacon received. Call CB registered by upper layer */
        brCb.brNwkBeaconPduRecvCback(MESH_BR_ADV_IF_TO_BR_IF(advIfId), pBrPdu, pduLen);
      }
      break;

    case MESH_AD_TYPE_PB:
      /* Generic Provisioning PDU received. Call CB registered by upper layer */
      brCb.brPbPduRecvCback(MESH_BR_ADV_IF_TO_BR_IF(advIfId), pBrPdu, pduLen);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Advertising bearer event notification callback.
 *
 *  \param[in] advIfId       Unique Mesh Advertising interface.
 *  \param[in] event         Reason the callback is being invoked. See ::meshAdvEvent.
 *  \param[in] pEventParams  Pointer to the event parameters structure passed to the function.
 *                           See ::meshAdvBrEventParams_t.
 *  \return    None.
 */
/*************************************************************************************************/
static void meshBrProcessAdvEventCback(meshAdvIfId_t advIfId, meshAdvEvent_t event,
                                       const meshAdvBrEventParams_t *pEventParams)
{
  meshBrEventParams_t brEventParams;

  /* Advertising interface occupies only the least significant nibble. It will be checked
   * by the Advertising Bearer
   */
  WSF_ASSERT(advIfId <= MESH_BR_INTERFACE_ID_INTERFACE_MASK);

  /* Event. Call Cback registered by upper layer */
  switch (event)
  {
    case MESH_ADV_INTERFACE_OPENED:
      MESH_TRACE_INFO0("MESH BEARER: advertising interface open");

      /* Event doesn't have any parameters. Assert if event is not NULL. */
      WSF_ASSERT(pEventParams == NULL);

      /* Translate the ADV interface opened event into a Bearer interface opened event */
      brEventParams.brConfig.bearerType = MESH_ADV_BEARER;
      brCb.brNwkEventCback(MESH_BR_ADV_IF_TO_BR_IF(advIfId), MESH_BR_INTERFACE_OPENED_EVT,
                           &brEventParams);
      brCb.brPbEventCback(MESH_BR_ADV_IF_TO_BR_IF(advIfId), MESH_BR_INTERFACE_OPENED_EVT,
                          &brEventParams);

      break;

    case MESH_ADV_INTERFACE_CLOSED:
      MESH_TRACE_INFO0("MESH BEARER: advertising interface closed");

      /* Event doesn't have any parameters. Assert if event is not NULL. */
      WSF_ASSERT(pEventParams == NULL);

      /* Translate the ADV interface closed event into a Bearer interface closed event */
      brEventParams.brConfig.bearerType = MESH_ADV_BEARER;
      brCb.brNwkEventCback(MESH_BR_ADV_IF_TO_BR_IF(advIfId), MESH_BR_INTERFACE_CLOSED_EVT,
                           &brEventParams);
      brCb.brPbEventCback(MESH_BR_ADV_IF_TO_BR_IF(advIfId), MESH_BR_INTERFACE_CLOSED_EVT,
                          &brEventParams);
      break;

    case MESH_ADV_PACKET_PROCESSED:
      /* Event has parameters. Assert if event is NULL. */
      WSF_ASSERT(pEventParams != NULL);

      /* Translate the ADV packet sent event into a Bearer packet sent event */
      brEventParams.brPduStatus.pPdu = pEventParams->brPduStatus.pPdu;
      if (pEventParams->brPduStatus.adType == MESH_AD_TYPE_PACKET)
      {
        brCb.brNwkEventCback(MESH_BR_ADV_IF_TO_BR_IF(advIfId), MESH_BR_INTERFACE_PACKET_SENT_EVT,
                             &brEventParams);
      }
      else if (pEventParams->brPduStatus.adType == MESH_AD_TYPE_PB)
      {
        brCb.brPbEventCback(MESH_BR_ADV_IF_TO_BR_IF(advIfId), MESH_BR_INTERFACE_PACKET_SENT_EVT,
                            &brEventParams);
      }
      else if (pEventParams->brPduStatus.adType == MESH_AD_TYPE_BEACON)
      {
        if (pEventParams->brPduStatus.pPdu[0] == MESH_BEACON_TYPE_UNPROV)
        {
          brCb.brPbBeaconEventCback(MESH_BR_ADV_IF_TO_BR_IF(advIfId),
                                    MESH_BR_INTERFACE_PACKET_SENT_EVT, &brEventParams);
        }
        else if (pEventParams->brPduStatus.pPdu[0] == MESH_BEACON_TYPE_SEC_NWK)
        {
          brCb.brNwkBeaconEventCback(MESH_BR_ADV_IF_TO_BR_IF(advIfId),
                                     MESH_BR_INTERFACE_PACKET_SENT_EVT, &brEventParams);
        }
      }
      break;

    default:
      /* Assert if received event is unknown */
      WSF_ASSERT(event <= MESH_ADV_PACKET_PROCESSED);
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh GATT Proxy PDU received callback function pointer type.
 *
 *  \param[in] connId       Unique identifier for the connection on which the PDU was received.
 *                          Valid range is 0x00 to 0x1F.
 *  \param[in] pduType      PDU type. See ::meshGattProxyPduType.
 *  \param[in] pProxyPdu    Pointer to a buffer containing the GATT Proxy PDU.
 *  \param[in] proxyPduLen  Size of the buffer referenced by pProxyPdu.
 *
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshBrProcessGattPduCback(meshGattProxyConnId_t connId, meshGattProxyPduType_t pduType,
                                      const uint8_t *pBrPdu, uint16_t brPduLen)
{
#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
    meshTestProxyCfgPduRcvdInd_t proxyPduRcvdInd;

  if (meshTestCb.listenMask & MESH_TEST_PROXY_LISTEN)
  {
    proxyPduRcvdInd.hdr.event = MESH_TEST_EVENT;
    proxyPduRcvdInd.hdr.param = MESH_TEST_PROXY_PDU_RCVD_IND;
    proxyPduRcvdInd.hdr.status = MESH_SUCCESS;
    proxyPduRcvdInd.pPdu = pBrPdu;
    proxyPduRcvdInd.pduLen = brPduLen;
    proxyPduRcvdInd.pduType = pduType;

    meshTestCb.testCback((meshTestEvt_t *)&proxyPduRcvdInd);
  }
#endif
  /* Send PDU to specified bearer type interface. GATT bearer takes care of the PDU length to be
   * in range
   */
  switch (pduType)
  {
    case MESH_GATT_PROXY_PDU_TYPE_NETWORK_PDU:
        /* Network PDU received. Call CB registered by upper layer.  */
        brCb.brNwkPduRecvCback(MESH_BR_CONN_ID_TO_BR_IF(connId), pBrPdu, (uint8_t)brPduLen);
      break;

    case MESH_GATT_PROXY_PDU_TYPE_BEACON:
      /* Beacon received. Call CB registered by upper layer */
      if (pBrPdu[0] == MESH_BEACON_TYPE_UNPROV)
      {
        brCb.brPbBeaconPduRecvCback(MESH_BR_CONN_ID_TO_BR_IF(connId), pBrPdu, (uint8_t)brPduLen);
      }
      else if (pBrPdu[0] == MESH_BEACON_TYPE_SEC_NWK)
      {
        brCb.brNwkBeaconPduRecvCback(MESH_BR_CONN_ID_TO_BR_IF(connId), pBrPdu, (uint8_t)brPduLen);
      }
      break;

    case MESH_GATT_PROXY_PDU_TYPE_PROVISIONING:
      /* Generic Provisioning PDU received. Call CB registered by upper layer */
      brCb.brPbPduRecvCback(MESH_BR_CONN_ID_TO_BR_IF(connId), pBrPdu, (uint8_t)brPduLen);
      break;

    case MESH_GATT_PROXY_PDU_TYPE_CONFIGURATION:
      /* Config Message received. Call CB registered by upper layer */
      brCb.brProxyMsgRecvCback(MESH_BR_CONN_ID_TO_BR_IF(connId), pBrPdu, (uint8_t)brPduLen);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh GATT Bearer event notification callback function pointer type.
 *
 *  \param[in] connId  Unique identifier for the connection on which the event was received.
 *                     Valid range is 0x00 to 0x1F.
 *  \param[in] pEvent  Pointer to Mesh GATT Bearer event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshBrProcessGattEventCback(meshGattProxyConnId_t connId, const meshGattEvent_t *pEvent)
{
  meshBrEventParams_t brEventParams;

  /* Interface occupies only the least significant nibble. It will be checked
   * by the GATT Bearer.
   */
  WSF_ASSERT(connId <= MESH_BR_INTERFACE_ID_INTERFACE_MASK);

  /* Event. Call Cback registered by upper layer */
  switch (pEvent->eventType)
  {
    case MESH_GATT_PROXY_CONN_OPENED:
      MESH_TRACE_INFO0("MESH BEARER: GATT connection open");

      /* Translate the ADV interface opened event into a Bearer interface opened event */
      brEventParams.brConfig.bearerType = MESH_GATT_BEARER;

      brCb.brNwkEventCback(MESH_BR_CONN_ID_TO_BR_IF(connId), MESH_BR_INTERFACE_OPENED_EVT,
                           &brEventParams);
      brCb.brPbEventCback(MESH_BR_CONN_ID_TO_BR_IF(connId), MESH_BR_INTERFACE_OPENED_EVT,
                          &brEventParams);
      brCb.brProxyEventCback(MESH_BR_CONN_ID_TO_BR_IF(connId), MESH_BR_INTERFACE_OPENED_EVT,
          &brEventParams);
      break;

    case MESH_GATT_PROXY_CONN_CLOSED:
      MESH_TRACE_INFO0("MESH BEARER: GATT connection closed");

      /* Translate the ADV interface closed event into a Bearer interface closed event */
      brEventParams.brConfig.bearerType = MESH_GATT_BEARER;
      brCb.brNwkEventCback(MESH_BR_CONN_ID_TO_BR_IF(connId), MESH_BR_INTERFACE_CLOSED_EVT,
                           &brEventParams);
      brCb.brPbEventCback(MESH_BR_CONN_ID_TO_BR_IF(connId), MESH_BR_INTERFACE_CLOSED_EVT,
                          &brEventParams);
      brCb.brProxyEventCback(MESH_BR_CONN_ID_TO_BR_IF(connId), MESH_BR_INTERFACE_CLOSED_EVT,
                             &brEventParams);
      break;
    case MESH_GATT_PACKET_PROCESSED:
      brEventParams.brPduStatus.bearerType = MESH_GATT_BEARER;
      brEventParams.brPduStatus.pPdu = pEvent->brPduStatus.pPdu;

      switch (pEvent->brPduStatus.  pduType)
      {
        case MESH_GATT_PROXY_PDU_TYPE_NETWORK_PDU:
          brCb.brNwkEventCback(MESH_BR_CONN_ID_TO_BR_IF(connId), MESH_BR_INTERFACE_PACKET_SENT_EVT,
                               &brEventParams);
          break;

        case MESH_GATT_PROXY_PDU_TYPE_BEACON:
          brCb.brNwkBeaconEventCback(MESH_BR_CONN_ID_TO_BR_IF(connId), MESH_BR_INTERFACE_PACKET_SENT_EVT,
                               &brEventParams);
          break;
        case MESH_GATT_PROXY_PDU_TYPE_CONFIGURATION:
          brCb.brProxyEventCback(MESH_BR_CONN_ID_TO_BR_IF(connId), MESH_BR_INTERFACE_PACKET_SENT_EVT,
                                 &brEventParams);
          break;
        case MESH_GATT_PROXY_PDU_TYPE_PROVISIONING:
          brCb.brPbEventCback(MESH_BR_CONN_ID_TO_BR_IF(connId), MESH_BR_INTERFACE_PACKET_SENT_EVT,
                              &brEventParams);
          break;
        default:
          break;
      }
      break;

    default:
      /* Assert if received event is unknown */
      WSF_ASSERT(pEvent->eventType <= MESH_GATT_PROXY_CONN_CLOSED);
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Advertising bearer event notification callback.
 *
 *  \param[in] advIfId       Unique Mesh Advertising interface.
 *  \param[in] event         Reason the callback is being invoked. See ::meshBrEvent_t.
 *  \param[in] pEventParams  Pointer to the event parameters structure passed to the function.
 *                           See ::meshBrEventParams_t.
 *  \return    None.
 */
/*************************************************************************************************/
static void brEmptyEvtCback(meshBrInterfaceId_t brInterfaceId, meshBrEvent_t event,
                            const meshBrEventParams_t *pEventParams)
{
  (void)brInterfaceId;
  (void)event;
  (void)pEventParams;
  MESH_TRACE_ERR0("MESH BEARER: Event callback not installed");
}

/*************************************************************************************************/
/*!
 *  \brief     Empty Network receive PDU callback.
 *
 *  \param[in] brIfId   Unique Mesh bearer interface.
 *  \param[in] pNwkPdu  Pointer to Network PDU.
 *  \param[in] pduLen   Network PDU length.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void brEmptyNwkPduCback(meshBrInterfaceId_t brIfId, const uint8_t *pNwkPdu, uint8_t pduLen)
{
  (void)brIfId;
  (void)pNwkPdu;
  (void)pduLen;
  MESH_TRACE_ERR0("MESH BEARER: Network PDU receive callback not installed");
}

/*************************************************************************************************/
/*!
 *  \brief     Empty Beacon receive callback.
 *
 *  \param[in] brIfId       Unique Mesh bearer interface.
 *  \param[in] pBeaconData  Pointer to Beacon data.
 *  \param[in] len          Beacon data length.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void brEmptyBeaconCback(meshBrInterfaceId_t brIfId, const uint8_t *pBeaconData, uint8_t len)
{
  (void)brIfId;
  (void)pBeaconData;
  (void)len;
  MESH_TRACE_ERR0("MESH BEARER: Beacon callback not installed");
}

/*************************************************************************************************/
/*!
 *  \brief     Empty Provisioning receive callback.
 *
 *  \param[in] brIfId     Unique Mesh bearer interface.
 *  \param[in] pPrvBrPdu  Pointer to Provisioning Bearer PDU.
 *  \param[in] len        Provisioning Bearer PDU length.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void brEmptyPbPduCback(meshBrInterfaceId_t brIfId, const uint8_t *pPrvBrPdu, uint8_t pduLen)
{
  (void)brIfId;
  (void)pPrvBrPdu;
  (void)pduLen;
  MESH_TRACE_ERR0("MESH BEARER: Provisioning Bearer PDU receive callback not installed");
}

/*************************************************************************************************/
/*!
 *  \brief     Empty receive Proxy PDU callback.
 *
 *  \param[in] brIfId   Unique Mesh bearer interface.
 *  \param[in] pNwkPdu  Pointer to Proxy PDU.
 *  \param[in] pduLen   Network PDU length.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void brEmptyRecvProxyPduCback(meshBrInterfaceId_t brIfId, const uint8_t *pNwkPdu,
                                     uint8_t pduLen)
{
  (void)brIfId;
  (void)pNwkPdu;
  (void)pduLen;
  MESH_TRACE_ERR0("MESH BEARER: Proxy PDU receive callback not installed");
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Mesh Bearer layer.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshBrInit(void)
{
  MESH_TRACE_INFO0("MESH BEARER: init");

  /* Set event callbacks */
  brCb.brNwkEventCback = brEmptyEvtCback;
  brCb.brNwkBeaconEventCback = brEmptyEvtCback;
  brCb.brPbEventCback = brEmptyEvtCback;
  brCb.brPbBeaconEventCback = brEmptyEvtCback;
  brCb.brPbBeaconPduRecvCback = brEmptyBeaconCback;
  brCb.brNwkPduRecvCback = brEmptyNwkPduCback;
  brCb.brNwkBeaconPduRecvCback = brEmptyBeaconCback;
  brCb.brPbPduRecvCback = brEmptyPbPduCback;
  brCb.brProxyEventCback = brEmptyEvtCback;
  brCb.brProxyMsgRecvCback = brEmptyRecvProxyPduCback;

  /* Initialize the ADV Bearer functionality */
  MeshAdvRegister(meshBrProcessAdvPduCback, meshBrProcessAdvEventCback);
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback functions for event notification and received Network PDUs on
 *             the bearer interface.
 *
 *  \param[in] eventCback       Event notification callback for the upper layer.
 *  \param[in] nwkPduRecvCback  Callback to be invoked when a Mesh Bearer PDU is received on
 *                              a specific bearer interface.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshBrRegisterNwk(meshBrEventNotifyCback_t eventCback, meshBrNwkPduRecvCback_t nwkPduRecvCback)
{
  /* Check for valid callbacks */
  if ((eventCback != NULL) && (nwkPduRecvCback != NULL))
  {
    brCb.brNwkEventCback = eventCback;
    brCb.brNwkPduRecvCback = nwkPduRecvCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback functions for event notification and received Secure Network
 *             Beacon PDUs on the bearer interface.
 *
 *  \param[in] eventCback       Event notification callback for the upper layer.
 *  \param[in] beaconRecvCback  Callback to be invoked when a Mesh Beacon PDU is received on
 *                              the bearer interface.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshBrRegisterNwkBeacon(meshBrEventNotifyCback_t eventCback,
                             meshBrBeaconRecvCback_t beaconRecvCback)
{
  /* Check for valid callback */
  if ((eventCback != NULL) && (beaconRecvCback != NULL))
  {
    brCb.brNwkBeaconEventCback = eventCback;
    brCb.brNwkBeaconPduRecvCback = beaconRecvCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback functions for event notification and received Provisioning
 *             PDUs on the bearer interface.
 *
 *  \param[in] eventCback      Event notification callback for the upper layer.
 *  \param[in] pbPduRecvCback  Callback to be invoked when a Mesh Bearer PDU is received on
 *                             a specific bearer interface.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshBrRegisterPb(meshBrEventNotifyCback_t eventCback, meshBrPbPduRecvCback_t pbPduRecvCback)
{
  /* Check for valid callbacks */
  if ((eventCback != NULL) && (pbPduRecvCback != NULL))
  {
    brCb.brPbEventCback = eventCback;
    brCb.brPbPduRecvCback = pbPduRecvCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback functions for event notification and received Unprovisioned
 *             Device Beacon PDUs on the bearer interface.
 *
 *  \param[in] eventCback            Event notification callback for the upper layer.
 *  \param[in] pbBeaconPduRecvCback  Callback to be invoked when a Mesh Bearer PDU is received on
 *                                   a specific bearer interface.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshBrRegisterPbBeacon(meshBrEventNotifyCback_t eventCback,
                            meshBrBeaconRecvCback_t pbBeaconPduRecvCback)
{
  /* Check for valid callback */
  if ((eventCback != NULL) && (pbBeaconPduRecvCback != NULL))
  {
    brCb.brPbBeaconEventCback = eventCback;
    brCb.brPbBeaconPduRecvCback = pbBeaconPduRecvCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback functions for event notification and received Proxy
 *             Configuration message on the bearer interface.
 *
 *  \param[in] eventCback    Event notification callback for the upper layer.
 *  \param[in] pduRecvCback  Callback to be invoked when a Mesh Bearer PDU is received on
 *                           a specific bearer interface.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshBrRegisterProxy(meshBrEventNotifyCback_t eventCback, meshBrNwkPduRecvCback_t pduRecvCback)
{
  /* Check for valid callbacks */
  if ((eventCback != NULL) && (pduRecvCback != NULL))
  {
    brCb.brProxyEventCback = eventCback;
    brCb.brProxyMsgRecvCback= pduRecvCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Asynchronously sends a Mesh Network PDU on a bearer interface.
 *
 *  \param[in] brIfId   Unique Mesh Bearer interface ID.
 *  \param[in] pNwkPdu  Pointer to a buffer containing a Mesh Network PDU.
 *  \param[in] pduLen   Size of the Mesh Network PDU.
 *
 *  \return    True if message is sent to the interface, False otherwise.
 *
 *  \note      A notification event will be sent with the transmission status when completed.
 *             See ::meshBrEvent_t and ::meshBrPduStatus_t
 */
/*************************************************************************************************/
bool_t MeshBrSendNwkPdu(meshBrInterfaceId_t brIfId, const uint8_t *pNwkPdu, uint8_t pduLen)
{
  bool_t ret;

  WSF_ASSERT(pNwkPdu != NULL);
  WSF_ASSERT(pduLen != 0);

  /* Check for valid input parameters */
  if (brIfId == MESH_BR_INVALID_INTERFACE_ID)
  {
    return FALSE;
  }

  /* Send PDU to specified bearer type interface */
  switch (MESH_BR_GET_BR_TYPE(brIfId))
  {
    case MESH_ADV_BEARER:
      MESH_TRACE_INFO0("MESH BEARER: Sending PDU to advertising interface");
      ret = MeshAdvSendBrPdu(MESH_BR_IF_TO_ADV_IF(brIfId), MESH_AD_TYPE_PACKET,
                                   pNwkPdu, pduLen);
      break;

    case MESH_GATT_BEARER:
      MESH_TRACE_INFO0("MESH BEARER: Sending PDU to GATT interface");
      ret = MeshGattSendBrPdu(MESH_BR_IF_TO_CONN_ID(brIfId), MESH_GATT_PROXY_PDU_TYPE_NETWORK_PDU,
                              pNwkPdu, pduLen);
      break;

    default:
      MESH_TRACE_ERR0("MESH BEARER: Sending PDU to invalid interface");
      ret = FALSE;
      break;
  }

  return ret;
}

/*************************************************************************************************/
/*!
 *  \brief     Asynchronously sends a Mesh Beacon PDU on a bearer interface.
 *
 *  \param[in] brIfId       Unique Mesh Bearer interface ID.
 *  \param[in] pBeaconData  Pointer to the Beacon Data payload.
 *  \param[in] dataLen      Size of the Beacon Data payload.
 *
 *  \return    True if message is sent to the interface, False otherwise.
 */
/*************************************************************************************************/
bool_t MeshBrSendBeaconPdu(meshBrInterfaceId_t brIfId, uint8_t *pBeaconData, uint8_t dataLen)
{
  bool_t ret;

  WSF_ASSERT(pBeaconData != NULL);
  WSF_ASSERT(dataLen != 0);

  /* Check for valid input parameters */
  if (brIfId == MESH_BR_INVALID_INTERFACE_ID)
  {
    MESH_TRACE_ERR0("MESH BEARER: Invalid parameters");
    return FALSE;
  }

  /* Send PDU to specified bearer type interface */
  switch (MESH_BR_GET_BR_TYPE(brIfId))
  {
    case MESH_ADV_BEARER:
      MESH_TRACE_INFO0("MESH BEARER: Sending beacon to advertising interface");
      ret = MeshAdvSendBrPdu(MESH_BR_IF_TO_ADV_IF(brIfId), MESH_AD_TYPE_BEACON, pBeaconData, dataLen);
      break;

    case MESH_GATT_BEARER:
      MESH_TRACE_INFO0("MESH BEARER: Sending beacon to GATT interface");
      ret = MeshGattSendBrPdu(MESH_BR_IF_TO_CONN_ID(brIfId), MESH_GATT_PROXY_PDU_TYPE_BEACON,
                              pBeaconData, dataLen);
      break;

    default:
      MESH_TRACE_ERR0("MESH BEARER: Sending PDU to invalid interface");
      ret = FALSE;
      break;
  }

  return ret;
}

/*************************************************************************************************/
/*!
 *  \brief     Asynchronously sends a Mesh Provisioning Bearer PDU on a bearer interface.
 *
 *  \param[in] brIfId   Unique Mesh Bearer interface ID.
 *  \param[in] pPrvPdu  Pointer to the Beacon Data payload.
 *  \param[in] pduLen   Size of the Beacon Data payload.
 *
 *  \return    True if message is sent to the interface, False otherwise.
 */
/*************************************************************************************************/
bool_t MeshBrSendPrvPdu(meshBrInterfaceId_t brIfId, const uint8_t *pPrvPdu, uint8_t pduLen)
{
  bool_t ret;

  WSF_ASSERT(pPrvPdu != NULL);
  WSF_ASSERT(pduLen != 0);

  /* Check for valid input parameters */
  if (brIfId == MESH_BR_INVALID_INTERFACE_ID)
  {
    MESH_TRACE_ERR0("MESH BEARER: Invalid parameters");
    return FALSE;
  }

  /* Send PDU to specified bearer type interface */
  switch (MESH_BR_GET_BR_TYPE(brIfId))
  {
    case MESH_ADV_BEARER:
      MESH_TRACE_INFO0("MESH BEARER: Sending Prv PDU to advertising interface");
      ret = MeshAdvSendBrPdu(MESH_BR_IF_TO_ADV_IF(brIfId), MESH_AD_TYPE_PB, pPrvPdu, pduLen);
      break;

    case MESH_GATT_BEARER:
      MESH_TRACE_INFO0("MESH BEARER: Sending Prv PDU to GATT interface");
      ret = MeshGattSendBrPdu(MESH_BR_IF_TO_CONN_ID(brIfId), MESH_GATT_PROXY_PDU_TYPE_PROVISIONING,
                              pPrvPdu, pduLen);
      break;

    default:
      MESH_TRACE_ERR0("MESH BEARER: Sending PDU to invalid interface");
      ret = FALSE;
      break;
  }

  return ret;
}

/*************************************************************************************************/
/*!
 *  \brief     Asynchronously sends a Proxy Configuration on a bearer interface.
 *
 *  \param[in] brIfId   Unique Mesh Bearer interface ID.
 *  \param[in] pCfgPdu  Pointer to the Proxy Configuration Data payload.
 *  \param[in] pduLen   Size of the Proxy Configuration Data payload.
 *
 *  \return    True if message is sent to the interface, False otherwise.
 */
/*************************************************************************************************/
bool_t MeshBrSendCfgPdu(meshBrInterfaceId_t brIfId, const uint8_t *pCfgPdu, uint8_t pduLen)
{
  WSF_ASSERT(pCfgPdu != NULL);
  WSF_ASSERT(pduLen != 0);

  /* Check for valid input parameters */
  if (brIfId == MESH_BR_INVALID_INTERFACE_ID)
  {
    MESH_TRACE_ERR0("MESH BEARER: Invalid parameters");
    return FALSE;
  }

  /* Send PDU to specified bearer type interface */
  if (MESH_BR_GET_BR_TYPE(brIfId) != MESH_GATT_BEARER)
  {
    MESH_TRACE_ERR0("MESH BEARER: Sending PDU to invalid interface");
    return FALSE;
  }

  MESH_TRACE_INFO0("MESH BEARER: Sending Config PDU to GATT interface");
  return MeshGattSendBrPdu(MESH_BR_IF_TO_CONN_ID(brIfId), MESH_GATT_PROXY_PDU_TYPE_CONFIGURATION,
                           pCfgPdu, pduLen);
}

/*************************************************************************************************/
/*!
 *  \brief     Closes the specified bearer interface.Works only for GATT interfaces.
 *
 *  \param[in] brIfId  Unique Mesh Bearer interface ID.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshBrCloseIf(meshBrInterfaceId_t brIfId)
{
  WSF_ASSERT(brIfId != MESH_BR_INVALID_INTERFACE_ID);

  if (MESH_BR_GET_BR_TYPE(brIfId) == MESH_GATT_BEARER)
  {
    /* Close GATT connection. */
    MeshGattCloseProxyConn(MESH_BR_IF_TO_CONN_ID(brIfId));
  }
}
/*************************************************************************************************/
/*!
 *  \brief  Initializes the GATT bearer functionality.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshBrEnableGatt(void)
{
  /* Initialize the GATT Bearer functionality */
  MeshGattInit();
  MeshGattRegister(meshBrProcessGattPduCback, meshBrProcessGattEventCback);
}
