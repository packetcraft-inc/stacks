/*************************************************************************************************/
/*!
 *  \file   mesh_proxy_sr.c
 *
 *  \brief  Mesh Proxy Server module implementation.
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
#include "wsf_cs.h"
#include "wsf_assert.h"
#include "wsf_buf.h"
#include "util/bstream.h"
#include "sec_api.h"
#include "wsf_trace.h"

#include "mesh_error_codes.h"
#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"
#include "mesh_bearer.h"
#include "mesh_network_if.h"
#include "mesh_proxy_main.h"
#include "mesh_proxy_sr.h"
#include "mesh_main.h"
#include "mesh_network_beacon_defs.h"
#include "mesh_network_beacon.h"
#include "mesh_local_config.h"
#include "mesh_security.h"
#include "mesh_security_toolbox.h"

#include <string.h>
#include <stddef.h>

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Checks whether the length is valid for a Add/Remove Addresses to/from Filter message*/
#define MESH_PROXY_IS_VALID_LEN(pduLen)                     ((pduLen - sizeof(uint8_t)) % 2 == 0)

/*! Size of Proxy Filter Status configuration message */
#define MESH_PROXY_CFG_FILTER_STATUS_MSG_SIZE               3

/*! Position of Filter Type parameter in the Proxy Filter Status configuration message */
#define MESH_PROXY_CFG_FILTER_STATUS_TYPE_PARAM_OFFSET      0

/*! Position of Filter Size parameter in the Proxy Filter Status configuration message */
#define MESH_PROXY_CFG_FILTER_STATUS_SIZE_PARAM_OFFSET      1

/*! Size of Plain Text used for encrypting node identity */
#define MESH_PROXY_NODE_ID_PT_SIZE                          16

/*! Size of Padding in the Plain Text used for encrypting node identity */
#define MESH_PROXY_NODE_ID_PT_PADDING_SIZE                  6

/*! Size of Random in the Plain Text used for encrypting node identity */
#define MESH_PROXY_NODE_ID_PT_RANDOM_SIZE                   8

/*! Size of encrypting node identity - hash as defined by the spec */
#define MESH_PROXY_NODE_ID_HASH_SIZE                        8

/*! Offset of the encrypted node identity hash */
#define MESH_PROXY_NODE_ID_HASH_OFFSET                      8

/*! Size of the Encrypting node identity */
#define MESH_PROXY_ENC_NODE_ID_SIZE                         17

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Proxy Server Control Block */
typedef struct meshProxySrCb_tag
{
  meshBrInterfaceId_t brIfId;                               /*! Bearer interface */
  uint16_t            bcnNetKeyIndexer;                     /*! Network key indexer used to cycle
                                                             *  through all network keys when
                                                             *  sending secure beacons.*/
  bool_t              encInProgress;                        /*! Node Identifier encryption in
                                                             *  progress
                                                             *  */
  uint8_t             svcData[MESH_PROXY_NODE_ID_PT_SIZE];  /*! Service Data requested by the upper
                                                             *  layer.
                                                             */
  uint16_t            svcDataNetKeyIndexer;                 /*! Network key indexer used to cycle
                                                             *  through all network keys when
                                                             *  creating service data.*/
} meshProxySrCb_t;

/**************************************************************************************************
 Local Variables
 **************************************************************************************************/

/*! Mesh Proxy Server Control Block */
static meshProxySrCb_t meshProxySrCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Handles incoming decrypted Proxy Configuration PDUs from the bearer.
 *
 *  \param[in] brIfId   Unique identifier of the interface.
 *  \param[in] pPdu     Pointer to the Proxy Configuration message PDU.
 *  \param[in] pduLen   Length of the Proxy Configuration message PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshProxySrPduRecvCback(meshBrInterfaceId_t brIfId, const uint8_t *pPdu, uint8_t pduLen)
{
  uint8_t opcode;
  bool_t sendStatus = FALSE;
  meshNwkIf_t *pNwkIf;
  meshAddress_t address;
  uint8_t *pPduOffset;

  /* Should never happen since proxy main module validates this. */
  WSF_ASSERT(brIfId != MESH_BR_INVALID_INTERFACE_ID);
  WSF_ASSERT(pPdu != NULL);
  WSF_ASSERT(pduLen > 0);

  /* Extract Opcode */
  opcode = *pPdu;

  switch (opcode)
  {
    case MESH_PROXY_OPCODE_SET_FILTER_TYPE:
      /* Check valid length and valid filter type. */
      if (pduLen == MESH_PROXY_SET_FILTER_TYPE_LEN &&
          pPdu[MESH_PROXY_FILTER_TYPE_OFFSET] <= MESH_NWK_BLACK_LIST)
      {
        MeshNwkIfSetFilterType(brIfId, pPdu[MESH_PROXY_FILTER_TYPE_OFFSET]);
        sendStatus = TRUE;
      }
      break;

    case MESH_PROXY_OPCODE_ADD_ADDRESS:
      /* Check valid length */
      if (MESH_PROXY_IS_VALID_LEN(pduLen))
      {
        pPduOffset = (uint8_t *)pPdu + MESH_PROXY_ADDRESS_OFFSET;

        while (pPduOffset < pPdu + pduLen - MESH_PROXY_ADDRESS_OFFSET)
        {
          /* Extract and add address to filter. */
          BSTREAM_BE_TO_UINT16(address, pPduOffset);
          MeshNwkIfAddAddressToFilter(brIfId, address);
        }
        /* Send Filter Status message */
        sendStatus = TRUE;
      }
      break;

    case MESH_PROXY_OPCODE_REMOVE_ADDRESS:
      /* Check valid length */
      if (MESH_PROXY_IS_VALID_LEN(pduLen))
      {
        pPduOffset = (uint8_t *)pPdu + MESH_PROXY_ADDRESS_OFFSET;

        while (pPduOffset < pPdu + pduLen - MESH_PROXY_ADDRESS_OFFSET)
        {
          /* Extract and add address to filter. */
          BSTREAM_BE_TO_UINT16(address, pPduOffset);
          MeshNwkIfRemoveAddressFromFilter(brIfId, address);
        }
        /* Send Filter Status message */
        sendStatus = TRUE;
      }
      break;

    default:
      /* Ignore RFU */
      break;
  }

  if (sendStatus == TRUE)
  {
    if ((pNwkIf = MeshNwkIfGet(brIfId)) != NULL)
    {
      uint8_t pdu[MESH_PROXY_CFG_FILTER_STATUS_MSG_SIZE];
      pdu[MESH_PROXY_CFG_FILTER_STATUS_TYPE_PARAM_OFFSET] = pNwkIf->outputFilter.filterType;

      UINT16_TO_BE_BUF(&pdu[MESH_PROXY_CFG_FILTER_STATUS_SIZE_PARAM_OFFSET],
                       pNwkIf->outputFilter.filterSize);

      /* Send Filter Status message. */
      meshProxySendConfigMessage(brIfId, MESH_PROXY_OPCODE_FILTER_STATUS,
                                 pdu, MESH_PROXY_CFG_FILTER_STATUS_MSG_SIZE);
    }
  }
}

/*************************************************************************************************/
/*!
 * \brief     Beacon generate complete callback.
 *
 * \param[in] isSuccess    TRUE if beacon was generated successfully.
 * \param[in] netKeyIndex  Global identifier of the NetKey.
 * \param[in] pBeacon      Pointer to generated beacon, provided in the request.
 *
 * \return    None.
 */
/*************************************************************************************************/
static void meshBeaconGenCompleteCback(bool_t isSuccess, uint16_t netKeyIndex, uint8_t *pBeacon)
{
  uint8_t *pBeaconPdu;

  if (isSuccess)
  {
    MeshBrSendBeaconPdu(meshProxySrCb.brIfId, pBeacon, MESH_NWK_BEACON_NUM_BYTES);
  }
  else
  {
    WsfBufFree(pBeacon);
  }

  /* Get next subnet key index */
  if (MeshLocalCfgGetNextNetKeyIndex(&netKeyIndex, &meshProxySrCb.bcnNetKeyIndexer) == MESH_SUCCESS)
  {
    /* Allocate memory for the beacon PDU*/
    pBeaconPdu = WsfBufAlloc(MESH_NWK_BEACON_NUM_BYTES);

    if(pBeaconPdu != NULL)
    {
      /* Generate beacon on demand */
      MeshNwkBeaconGenOnDemand(netKeyIndex, pBeaconPdu, meshBeaconGenCompleteCback);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Send secure beacons to the Proxy Client.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void meshProxySrSendSecureBeacons(void)
{
  uint16_t  netKeyIndex = 0;
  uint8_t *pBeaconPdu;

  /* Initialize indexer for NetKeys */
  meshProxySrCb.bcnNetKeyIndexer = 0;

  /* Get subnet key index */
  (void) MeshLocalCfgGetNextNetKeyIndex(&netKeyIndex, &meshProxySrCb.bcnNetKeyIndexer);

  /* Allocate memory for the beacon PDU*/
  pBeaconPdu = WsfBufAlloc(MESH_NWK_BEACON_NUM_BYTES);

  if(pBeaconPdu != NULL)
  {
    /* Generate beacon on demand */
    MeshNwkBeaconGenOnDemand(netKeyIndex, pBeaconPdu, meshBeaconGenCompleteCback);
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
      /* Store bearer interface. */
      meshProxySrCb.brIfId = brIfId;

      /* Send Secure Beacons. */
      meshProxySrSendSecureBeacons();
      break;

    /* Handle interface closed. */
    case MESH_BR_INTERFACE_CLOSED_EVT:
      /* Reset bearer interface. */
      meshProxySrCb.brIfId = MESH_BR_INVALID_INTERFACE_ID;
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
 *  \brief     Encrypted Node Identity complete callback.
 *
 *  \param[in] pCipherTextBlock  Pointer to 128-bit AES result.
 *  \param[in] pParam            Generic parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshProxyNodeIdentityCompleteCback(const uint8_t *pCipherTextBlock, void *pParam)
{
  meshProxyServiceDataEvt_t evt;

  /* Mark encryption as finished. */
  meshProxySrCb.encInProgress = FALSE;

  /* Advertise with Node Identity */
  evt.hdr.event = MESH_CORE_EVENT;
  evt.hdr.param = MESH_CORE_PROXY_SERVICE_DATA_EVENT;
  evt.serviceData[0] = MESH_PROXY_NODE_IDENTITY_TYPE;
  evt.serviceDataLen = MESH_PROXY_ENC_NODE_ID_SIZE;

  /* Copy Hash */
  memcpy(&evt.serviceData[1], pCipherTextBlock + MESH_PROXY_NODE_ID_HASH_OFFSET,
        MESH_PROXY_NODE_ID_HASH_SIZE);

  /* Copy Random */
  memcpy(&evt.serviceData[1 + MESH_PROXY_NODE_ID_HASH_SIZE],
         &meshProxySrCb.svcData[MESH_PROXY_NODE_ID_PT_PADDING_SIZE],
         MESH_PROXY_NODE_ID_PT_RANDOM_SIZE);

  /* Send event to application. */
  meshCb.evtCback((meshEvt_t*)&evt);

  (void)pParam;
}

/*************************************************************************************************/
/*!
 *  \brief     Computes Node Identity.
 *
 *  \param[in] netKeyIndex  Netkey index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshProxyComputeNodeIdentity(uint16_t netKeyIndex)
{
  uint8_t *pIdentityKey;
  meshAddress_t addr;
  uint8_t *pData;

  /* Set Padding */
  memset(meshProxySrCb.svcData, 0, MESH_PROXY_NODE_ID_PT_PADDING_SIZE);

  /* Set Random */
  SecRand(&meshProxySrCb.svcData[MESH_PROXY_NODE_ID_PT_PADDING_SIZE], MESH_PROXY_NODE_ID_PT_RANDOM_SIZE);

  /* Set Address */
  MeshLocalCfgGetAddrFromElementId(0, &addr);
  pData = &meshProxySrCb.svcData[MESH_PROXY_NODE_ID_PT_SIZE - sizeof(meshAddress_t)];
  UINT16_TO_BE_BSTREAM(pData, addr);

  /* Get Identity key for the specified netKey */
  pIdentityKey = MeshSecNetKeyIndexToIdentityKey(netKeyIndex);

  /* Call AES */
  if ((pIdentityKey != NULL) &&
      (MeshSecToolAesEncrypt(pIdentityKey, meshProxySrCb.svcData, meshProxyNodeIdentityCompleteCback, NULL)
        == MESH_SUCCESS))
  {
    /* Mark encryption as in progress. */
    meshProxySrCb.encInProgress = TRUE;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Requests the Proxy service data from the Mesh stack. This is used by the application
 *             to send connectable advertising packets.
 *
 *  \param[in] netKeyIndex  Network key index.
 *  \param[in] idType       Identification type.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshProxySrCreateServiceData(uint16_t netKeyIndex, meshProxyIdType_t idType)
{
  meshGattProxyStates_t proxyState;
  meshNodeIdentityStates_t nodeIdentityState;
  meshProxyServiceDataEvt_t evt;
  uint8_t *pData;

  evt.hdr.event = MESH_CORE_EVENT;
  evt.hdr.param = MESH_CORE_PROXY_SERVICE_DATA_EVENT;
  evt.serviceDataLen = 0;

  /* Get Proxy State */
  proxyState = MeshLocalCfgGetGattProxyState();

  /* Get Node Identity State */
  nodeIdentityState = MeshLocalCfgGetNodeIdentityState(netKeyIndex);

  /* Proxy must be enabled if advertising with Network ID. */
  if ((idType == MESH_PROXY_NWK_ID_TYPE) && (proxyState != MESH_GATT_PROXY_FEATURE_ENABLED))
  {
    MESH_TRACE_ERR0("MESH PROXY SR: Proxy must be enabled if advertising with Network ID");

    /* Set status. */
    evt.hdr.status = MESH_INVALID_CONFIG;

    /* Send empty service data to application. */
    meshCb.evtCback((meshEvt_t*)&evt);
  }
  /* Node Identity must be running if advertising with Node ID. */
  else if ((idType == MESH_PROXY_NODE_IDENTITY_TYPE) && (nodeIdentityState != MESH_NODE_IDENTITY_RUNNING))
  {
    MESH_TRACE_ERR0("MESH PROXY SR: Node Identity must be running if advertising with Node ID");

    /* Set status. */
    evt.hdr.status = MESH_INVALID_CONFIG;

    /* Send empty service data to application. */
    meshCb.evtCback((meshEvt_t*)&evt);
  }

  switch (idType)
  {
    case MESH_PROXY_NWK_ID_TYPE:
      /* Advertise with Network ID */
      evt.serviceData[0] = MESH_PROXY_NWK_ID_TYPE;

      /* Get Network Key*/
      pData = MeshSecNetKeyIndexToNwkId(netKeyIndex);

      if (pData != NULL)
      {
        /* Copy Network Key in event payload. */
        memcpy(&evt.serviceData[1], pData, MESH_NWK_ID_NUM_BYTES);
        evt.serviceDataLen = MESH_PROXY_NWKID_SERVICE_DATA_SIZE;

        /* Set status. */
        evt.hdr.status = MESH_SUCCESS;

        /* Send event to application. */
        meshCb.evtCback((meshEvt_t*)&evt);
      }
      break;

    case MESH_PROXY_NODE_IDENTITY_TYPE:
      if (!meshProxySrCb.encInProgress)
      {
        /* Compute encrypted Node Identity only if there isn't another encryption ongoing.
         * The application should always wait for data.
         */
        meshProxyComputeNodeIdentity(netKeyIndex);
      }

      break;

    default:
      break;
  }
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Proxy Server functionality.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshProxySrInit(void)
{
  uint16_t keyIndex;
  uint16_t indexer = 0;

  /* Initialize control block. */
  meshProxySrCb.brIfId = MESH_BR_INVALID_INTERFACE_ID;
  meshProxySrCb.svcDataNetKeyIndexer = 0;
  meshProxySrCb.bcnNetKeyIndexer = 0;
  meshProxySrCb.encInProgress = FALSE;

  /* Register callbacks. */
  meshProxyRegister(meshBrEventNotificationCback, meshProxySrPduRecvCback);

  if (MeshLocalCfgGetGattProxyState() == MESH_GATT_PROXY_FEATURE_NOT_SUPPORTED)
  {
    MeshLocalCfgSetGattProxyState(MESH_GATT_PROXY_FEATURE_DISABLED);
  }

  /* Set Node Identity state to stopped for all subnets. */
  while (MeshLocalCfgGetNextNetKeyIndex(&keyIndex, &indexer) == MESH_SUCCESS)
  {
    MeshLocalCfgSetNodeIdentityState(keyIndex, MESH_NODE_IDENTITY_STOPPED);
  }

  /* Set Proxy Server flag. */
  meshCb.proxyIsServer = TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief  Disables the Proxy Server functionality.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshProxySrDisable(void)
{
  /* Close GATT interface */
  if (meshProxySrCb.brIfId != MESH_BR_INVALID_INTERFACE_ID)
  {
    MeshBrCloseIf(meshProxySrCb.brIfId);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Requests the Proxy service data from the Mesh stack. This is used by the application
 *             to send connectable advertising packets.
 *
 *  \param[in] netKeyIndex  Network key index.
 *  \param[in] idType       Identification type.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshProxySrGetServiceData(uint16_t netKeyIndex, meshProxyIdType_t idType)
{
  /* Initialize critical section. */
  WSF_CS_INIT(cs);
  /* Enter critical section. */
  WSF_CS_ENTER(cs);

  /* Create Service Data. */
  meshProxySrCreateServiceData(netKeyIndex, idType);

  /* Exit critical section. */
  WSF_CS_EXIT(cs);
}

/*************************************************************************************************/
/*!
 *  \brief     Requests the next available Proxy service data from the Mesh stack while cycling
 *             through the netkey indexes.This is used by the application to send connectable
 *             advertising packets.
 *
 *  \param[in] idType  Identification type.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshProxySrGetNextServiceData(meshProxyIdType_t idType)
{
  uint16_t  netKeyIndex = 0;

  /* Initialize critical section. */
  WSF_CS_INIT(cs);
  /* Enter critical section. */
  WSF_CS_ENTER(cs);

  /* Get subnet key index */
  if (MESH_SUCCESS != MeshLocalCfgGetNextNetKeyIndex(&netKeyIndex, &meshProxySrCb.svcDataNetKeyIndexer))
  {
    /* Reset indexer to 0 and get the net key. */
    meshProxySrCb.svcDataNetKeyIndexer = 0;

    if (MESH_SUCCESS != MeshLocalCfgGetNextNetKeyIndex(&netKeyIndex, &meshProxySrCb.svcDataNetKeyIndexer))
    {
      return;
    }
  }

  /* Create Service Data. */
  meshProxySrCreateServiceData(netKeyIndex, idType);

  /* Exit critical section. */
  WSF_CS_EXIT(cs);
}
