/*************************************************************************************************/
/*!
 *  \file   mesh_network_beacon.c
 *
 *  \brief  Secure Network Beacon module implementation.
 *
 *  Copyright (c) 2010-2019 Arm Ltd. All Rights Reserved.
 *
 *  Copyright (c) 2019-2020 Packetcraft, Inc.
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
#include "wsf_cs.h"
#include "wsf_buf.h"
#include "wsf_msg.h"
#include "wsf_queue.h"
#include "wsf_assert.h"
#include "wsf_trace.h"
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_network_beacon_defs.h"
#include "mesh_types.h"
#include "mesh_error_codes.h"
#include "mesh_api.h"
#include "mesh_main.h"

#include "mesh_local_config_types.h"
#include "mesh_local_config.h"
#include "mesh_security.h"
#include "mesh_utils.h"

#include "mesh_adv_bearer.h"
#include "mesh_gatt_bearer.h"
#include "mesh_bearer.h"
#include "mesh_proxy_cl.h"
#include "mesh_network_if.h"
#include "mesh_network_main.h"
#include "mesh_network_mgmt.h"
#include "mesh_network_beacon.h"

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
#include "mesh_test_api.h"
#endif

#include <string.h>

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Decides if beacon authentication should be computed using new key. */
#define BEACON_AUTH_WITH_NEW_KEY(netKeyIndex)  (MeshLocalCfgGetKeyRefreshPhaseState(netKeyIndex) ==\
                                                MESH_KEY_REFRESH_SECOND_PHASE)

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Network Beacons WSF message events */
enum meshNwkBeaconWsfMsgEvents
{
  /*! Broadcast Timer expired message event */
  MESH_NWK_BEACON_MSG_BCAST_TMR_EXPIRED = MESH_NWK_BEACON_MSG_START /*!< Beacon timer expired */
};

/*! Generic function pointer. */
typedef void (*meshNwkBeaconGenericCback_t) (void);

struct meshNwkBeaconMeta_tag;

/*! Internal beacon generate complete callback. */
typedef void (*meshBeaconGenInternalCback_t)(bool_t isSuccess, uint16_t netKeyIndex,
                                             struct meshNwkBeaconMeta_tag *pNwkBeaconMeta);

/*! Network beacon callbacks. */
typedef union meshNwkBeaconCbs_tag
{
  meshBeaconGenOnDemandCback_t  onDemandCback; /*!< On-demand generate complete callback. */
  meshBeaconGenInternalCback_t  compCback;     /*!< Internal generate complete callback. */
} meshNwkBeaconCbs_t;

/*! Beacon and meta information. */
typedef struct meshNwkBeaconMeta_tag
{
  void                            *pNext;      /*!< Pointer to next beacon generate request. */
  uint8_t                         *pBeacon;    /*!< Pointer to beacon buffer. */
  meshNwkBeaconCbs_t              cbacks;      /*!< Generate complete callback union. */
  uint16_t                        netKeyIndex; /*!< NetKey Index. */
  bool_t                          onDemand;    /*!< TRUE if beacon was generated with on demand
                                                *   API
                                                */
  uint8_t refCount;                            /*!< Number of references of the beacon in the
                                                *   bearer queues.
                                                */
} meshNwkBeaconMeta_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Secure Network Beacons control block */
static struct meshNwkBeaconCb_tag
{
  wsfQueue_t              txBeaconQueue;               /*!< Queue of beacons pending to be
                                                        *   ACK'ed by the bearer.
                                                        */
  wsfQueue_t              ackBeaconsQueue;             /*!< Queue of beacons pending to be
                                                        *   ACK'ed by the bearer.
                                                        */
  wsfQueue_t              rxBeaconQueue;               /*!< Queue of receveid beacons to be
                                                        *   authenticated
                                                        */
  wsfTimer_t              bcastTmr;                    /*!< Broadcast timer */
  uint16_t                bcastNetKeyIndexer;          /*!< Indexer used to parse NetKey
                                                        *   Indexes on periodic broadcast
                                                        */
  uint16_t                trigNetKeyIndexer;           /*!< Indexer used to parse NetKey
                                                        *   Indexes on triggered broadcast
                                                        */
  bool_t                  bcastOn;                     /*!< TRUE if beacon broadcasting is in
                                                        *   progress
                                                        */
  bool_t                  genInProgr;                  /*!< TRUE if generation in progress. */
  bool_t                  authInProgr;                 /*!< TRUE if authentication in progress.
                                                        */
} meshNwkBeaconCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

static void meshNwkBeaconResumeAuth(void);
static bool_t meshNwkBeaconGenNext(uint16_t *pIndexer, meshBeaconGenInternalCback_t cback);

/*************************************************************************************************/
/*!
 *  \brief     Security Beacon authentication callback implementation.
 *
 *  \param[in] isSuccess      TRUE if operation completed successfully.
 *  \param[in] newKeyUsed     TRUE if the new key was used to authenticate.
 *  \param[in] pSecNwkBeacon  Pointer to buffer where the Secure Network Beacon is stored.
 *  \param[in] netKeyIndex    Global network key index associated to the key that successfully
 *                            processed the received Secure Network Beacon.
 *  \param[in] pParam         Pointer to generic callback parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void secAuthCback(bool_t isSuccess, bool_t newKeyUsed, uint8_t *pSecNwkBeacon,
                         uint16_t netKeyIndex, void *pParam)
{
  uint32_t rxIv;
  bool_t ivUpdate, keyRef;

  if (isSuccess)
  {
    /* Extract Key Refresh and IV update flags. */
    keyRef =  MESH_UTILS_BITMASK_CHK(pSecNwkBeacon[MESH_NWK_BEACON_FLAGS_BYTE_POS],
                                     (1 << MESH_NWK_BEACON_KEY_REF_FLAG_SHIFT));
    ivUpdate =  MESH_UTILS_BITMASK_CHK(pSecNwkBeacon[MESH_NWK_BEACON_FLAGS_BYTE_POS],
                                       (1 << MESH_NWK_BEACON_IV_UPDT_FLAG_SHIFT));

    /* Extract IV index. */
    BYTES_BE_TO_UINT32(rxIv, &pSecNwkBeacon[MESH_NWK_BEACON_IV_START_BYTE]);

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
    if (meshTestCb.listenMask & MESH_TEST_NWK_LISTEN)
    {
      meshTestSecNwkBeaconRcvdInd_t secNwkBeaconInd;
      uint8_t *pNwkId;

      secNwkBeaconInd.hdr.event = MESH_TEST_EVENT;
      secNwkBeaconInd.hdr.param = MESH_TEST_SEC_NWK_BEACON_RCVD_IND;
      secNwkBeaconInd.hdr.status = MESH_SUCCESS;
      secNwkBeaconInd.ivUpdate = ivUpdate;
      secNwkBeaconInd.keyRefresh = keyRef;
      secNwkBeaconInd.ivi = rxIv;
      pNwkId = MeshSecNetKeyIndexToNwkId(netKeyIndex);

      if (pNwkId != NULL)
      {
        memcpy(secNwkBeaconInd.networkId, pNwkId, MESH_NWK_ID_NUM_BYTES);
      }
      else
      {
        memset(secNwkBeaconInd.networkId, 0, MESH_NWK_ID_NUM_BYTES);
      }

      meshTestCb.testCback((meshTestEvt_t *)&secNwkBeaconInd);
    }
#endif
    /* Call Network Management. */
    MeshNwkMgmtHandleBeaconData(netKeyIndex, newKeyUsed, rxIv, keyRef, ivUpdate);
  }

  /* Free memory. */
  WsfBufFree(pParam);

  /* Mark in progress as FALSE. */
  meshNwkBeaconCb.authInProgr = FALSE;

  /* Resume authentication of received beacons. */
  meshNwkBeaconResumeAuth();
}

/*************************************************************************************************/
/*!
 *  \brief  Resumes authentication of received beacons.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void meshNwkBeaconResumeAuth(void)
{
  meshNwkBeaconMeta_t *pBeaconMeta;
  /* If another authentication is in progress, resume after it finishes. */
  if (meshNwkBeaconCb.authInProgr)
  {
    return;
  }

  /* If there is nothing to process, return. */
  if (WsfQueueEmpty(&meshNwkBeaconCb.rxBeaconQueue))
  {
    return;
  }

  /* Iterate through stored beacons. */
  while((pBeaconMeta = (meshNwkBeaconMeta_t *)WsfQueueDeq(&meshNwkBeaconCb.rxBeaconQueue)) != NULL)
  {
    /* Request authentication. */
    if (MeshSecBeaconAuthenticate(pBeaconMeta->pBeacon, secAuthCback, pBeaconMeta) != MESH_SUCCESS)
    {
      /* On fail, free memory. */
      WsfBufFree(pBeaconMeta);
    }
    else
    {
      meshNwkBeaconCb.authInProgr = TRUE;
      return;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Filters beacons that have invalid IV index before authenticating them.
 *
 *  \param[in] pSecNwkBeacon Pointer to Secure Network Beacon.
 *
 *  \return    TRUE if beacon is filtered out, FALSE if it should be processed.
 */
/*************************************************************************************************/
static inline bool_t meshNwkBeaconFilterInvalidRxIv(const uint8_t *pSecNwkBeacon)
{
  uint32_t rxIv, localIv;

  /* Read local IV index. */
  localIv = MeshLocalCfgGetIvIndex(NULL);

  /* Extract IV index. */
  BYTES_BE_TO_UINT32(rxIv, &pSecNwkBeacon[MESH_NWK_BEACON_IV_START_BYTE]);

  /* Filter beacon when received IV index is smaller or when it exceeds "48 weeks" limit. */
  return ((rxIv < localIv) || (rxIv > localIv + MESH_NWK_BEACON_MAX_IV_DIFF));
}

/*************************************************************************************************/
/*!
 *  \brief     Handles incoming Secure Network Beacon PDU's from the bearer.
 *
 *  \param[in] brIfId       Unique identifier of the interface.
 *  \param[in] pBeaconData  Pointer to the beacon PDU.
 *  \param[in] pduLen       Length of the beacon PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkBeaconPduRecvCback(meshBrInterfaceId_t brIfId,
                                      const uint8_t *pBeaconData, uint8_t dataLen)
{
  meshNwkBeaconMeta_t *pBeaconMeta;

  if (dataLen != MESH_NWK_BEACON_NUM_BYTES)
  {
    return;
  }

  /* Don't accept more than defined limit. */
  if (WsfQueueCount(&meshNwkBeaconCb.rxBeaconQueue) >=
      MESH_NWK_BEACON_RX_QUEUE_LIMIT)
  {
    return;
  }

  /* Apply rules for received IV index. */
  if (meshNwkBeaconFilterInvalidRxIv(pBeaconData))
  {
    return;
  }

  /* Allocate beacon payload. */
  if ((pBeaconMeta =
       (meshNwkBeaconMeta_t *)WsfBufAlloc(sizeof(meshNwkBeaconMeta_t) + MESH_NWK_BEACON_NUM_BYTES))
       == NULL)
  {
    return;
  }

  /* Set pointer to beacon. */
  pBeaconMeta->pBeacon = ((uint8_t*)pBeaconMeta) + sizeof(meshNwkBeaconMeta_t);

  /* Copy beacon. */
  memcpy(pBeaconMeta->pBeacon, pBeaconData, MESH_NWK_BEACON_NUM_BYTES);

  /* Enqueue for authentication. */
  WsfQueueEnq(&meshNwkBeaconCb.rxBeaconQueue, pBeaconMeta);

  /* Trigger authentication start. */
  meshNwkBeaconResumeAuth();

  (void)brIfId;
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
static void meshNwkBeaconEvtCback(meshBrInterfaceId_t brIfId, meshBrEvent_t event,
                                  const meshBrEventParams_t *pEventParams)
{
  meshNwkBeaconMeta_t *pMeta, *pPrev, *pNext;

  /* Handle only delivery confirmations. */
  if(event == MESH_BR_INTERFACE_PACKET_SENT_EVT)
  {
    /* Set previous to NULL. */
    pPrev = NULL;
    /* Point to start of the queue. */
    pMeta = (meshNwkBeaconMeta_t *)(&(meshNwkBeaconCb.ackBeaconsQueue))->pHead;

    while (pMeta != NULL)
    {
      /* Get pointer to next entry. */
      pNext = pMeta->pNext;
      /* Check if sent PDU is contained by this meta. */
      /* Note: pMeta cannot be NULL */
      /* coverity[dereference] */
      if (pMeta->pBeacon == pEventParams->brPduStatus.pPdu)
      {
        /* Decrement reference count. */
        if (pMeta->refCount)
        {
          (pMeta->refCount)--;
        }
      }

      /* Check if beacon and meta still need to be stored. */
      /* Note: pMeta cannot be NULL */
      /* coverity[dereference] */
      if (pMeta->refCount == 0)
      {
        /* Remove from queue. pPrev remains the same. */
        WsfQueueRemove(&(meshNwkBeaconCb.ackBeaconsQueue), pMeta, pPrev);
        /* Free memory. */
        WsfBufFree(pMeta);
      }
      else
      {
        /* Update pointer to previous element. */
        pPrev = pMeta;
      }

      /* Move to next entry. */
      pMeta = pNext;
    }
  }

  (void)brIfId;
}

/*************************************************************************************************/
/*!
 *  \brief     Configures beacon fields based on subnet information.
 *
 *  \param[in] netKeyIndex  Global identifier of the NetKey associated to a subnet.
 *  \param[in] pBeaconData  Pointer to the network PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static inline bool_t meshNwkBeaconConfig(uint16_t netKeyIndex, uint8_t *pBeacon)
{
  uint8_t  *pOffset;
  uint8_t  flags = 0;
  uint32_t ivIndex;
  uint8_t  ivUpdate;
  meshKeyRefreshStates_t keyRef;

    /* Read Key Refresh state. */
  keyRef = MeshLocalCfgGetKeyRefreshPhaseState(netKeyIndex);

  /* Check Key Refresh Phase State to see if NetKey Index is valid. */
  if (keyRef >= MESH_KEY_REFRESH_PROHIBITED_START)
  {
    return FALSE;
  }

  /* Read IV. */
  ivIndex = MeshLocalCfgGetIvIndex(&ivUpdate);

  pOffset = pBeacon;

  /* Set beacon type. */
  UINT8_TO_BSTREAM(pOffset, MESH_BEACON_TYPE_SEC_NWK);

  /* Set beacon flags. */
  flags = (ivUpdate ? (1 << MESH_NWK_BEACON_IV_UPDT_FLAG_SHIFT) : 0);
  flags |=
    (keyRef == MESH_KEY_REFRESH_SECOND_PHASE ? (1 << MESH_NWK_BEACON_KEY_REF_FLAG_SHIFT) : 0);

  UINT8_TO_BSTREAM(pOffset, flags);

  /* Skip Network ID since security sets it. */
  pOffset += MESH_NWK_ID_NUM_BYTES;

  /* Set IV index. */
  UINT32_TO_BE_BSTREAM(pOffset, ivIndex);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Allocates a beacon meta.
 *
 *  \param[in] pBeacon      Pointer to beacon buffer for on demand API, NULL otherwise.
 *  \param[in] netKeyIndex  Global Network Key.
 *  \param[in] cback        Pointer to the complete callback.
 *
 *  \return    None.
*/
 /*************************************************************************************************/
static meshNwkBeaconMeta_t *meshNwkBeaconAllocateBeaconMeta(uint8_t *pBeacon, uint16_t netKeyIndex,
                                                            meshNwkBeaconGenericCback_t cback)
{
  meshNwkBeaconMeta_t *pBeaconMeta;

  /* Allocate beacon meta with beacon room if not on demand. */
  if (pBeacon != NULL)
  {
    if ((pBeaconMeta = WsfBufAlloc(sizeof(meshNwkBeaconMeta_t))) == NULL)
    {
      return NULL;
    }
    pBeaconMeta->pBeacon = pBeacon;
    /* Set complete callback. */
    pBeaconMeta->cbacks.onDemandCback = (meshBeaconGenOnDemandCback_t)cback;
  }
  else
  {
    /* Allocate beacon and meta. */
    if ((pBeaconMeta = WsfBufAlloc(sizeof(meshNwkBeaconMeta_t) + MESH_NWK_BEACON_NUM_BYTES))
         == NULL)
    {
      /* Allocation failed. */
      return NULL;
    }
    else
    {
      /* Set pointer to beacon. */
      pBeaconMeta->pBeacon = ((uint8_t *)pBeaconMeta) + sizeof(meshNwkBeaconMeta_t);
      /* Set complete callback. */
      pBeaconMeta->cbacks.compCback = (meshBeaconGenInternalCback_t)cback;
    }
  }

  /* Set beacon fields. */
  if (!meshNwkBeaconConfig(netKeyIndex, pBeaconMeta->pBeacon))
  {
    /* Free memory. */
    WsfBufFree(pBeaconMeta);

    return NULL;
  }

  /* Set on demand flag. */
  pBeaconMeta->onDemand = (pBeacon != NULL);
  /* Reset ref count. */
  pBeaconMeta->refCount = 0;

  /* Set NetKey Index. */
  pBeaconMeta->netKeyIndex = netKeyIndex;

  return pBeaconMeta;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Secure Network Beacon authentication calculated callback.
 *
 *  \param[in] isSuccess    TRUE if operation completed successfully.
 *  \param[in] pBeacon      Pointer to buffer where the Secure Network Beacon is stored.
 *  \param[in] netKeyIndex  Global network key index used to compute the authentication value.
 *  \param[in] pParam       Pointer to generic callback parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void secGenCback(bool_t isSuccess, uint8_t *pBeacon, uint16_t netKeyIndex, void *pParam)
{
  /* Recover request. */
  meshNwkBeaconMeta_t *pBeaconMeta = (meshNwkBeaconMeta_t *)pParam;

  WSF_ASSERT(pBeaconMeta != NULL);
  /* Should point at the same memory buffer. */
  WSF_ASSERT(pBeaconMeta->pBeacon == pBeacon);

  /* Check if this was on demand. */
  if (pBeaconMeta->onDemand)
  {
    /* Invoke callback. */
    pBeaconMeta->cbacks.onDemandCback(isSuccess, netKeyIndex, pBeacon);

    /* Free message as it was needed only for the on demand API. */
    WsfBufFree(pBeaconMeta);
  }
  else
  {
    /* Invoke callback. It will handle freeing on its own. */
    pBeaconMeta->cbacks.compCback(isSuccess, netKeyIndex, pBeaconMeta);
  }

  /* Iterate through remaining. */
  while ((pBeaconMeta = WsfQueueDeq(&meshNwkBeaconCb.txBeaconQueue)) != NULL)
  {
    /* Check if security accepts new request. */
    if (MeshSecBeaconComputeAuth(pBeaconMeta->pBeacon,
                                 pBeaconMeta->netKeyIndex, BEACON_AUTH_WITH_NEW_KEY(netKeyIndex),
                                 secGenCback, pBeaconMeta)
        == MESH_SUCCESS)
    {
      /* Set generation in progress flag. */
      meshNwkBeaconCb.genInProgr = TRUE;
      /* Accepted for processing. Return. */
      return;
    }
    else
    {
      /* Check if this was on demand. */
      if (pBeaconMeta->onDemand)
      {
        /* Invoke callback. */
        pBeaconMeta->cbacks.onDemandCback(FALSE, pBeaconMeta->netKeyIndex, pBeaconMeta->pBeacon);

        /* Free message as it was needed only for the on demand API. */
        WsfBufFree(pBeaconMeta);
      }
      else
      {
        /* Invoke callback. It will handle freeing on its own. */
        pBeaconMeta->cbacks.compCback(FALSE, pBeaconMeta->netKeyIndex, pBeaconMeta);
      }
    }
  }
  /* Clear in progress flag. */
  meshNwkBeaconCb.genInProgr = FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Manages beacon sending.
 *
 *  \param[in] pBeaconMeta  Pointer to beacon and meta information.
 *  \param[in] sendOnAdv    TRUE if beacon should be send on ADV.
 *  \param[in] sendOnGatt   TRUE if beacon should be send on GATT.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkBeaconManageSend(meshNwkBeaconMeta_t *pBeaconMeta, bool_t sendOnAdv,
                                    bool_t sendOnGatt)
{
  uint8_t idx;

  /* Send on all advertising interfaces. */
  for (idx = 0; idx < MESH_BR_MAX_INTERFACES; idx++)
  {
    /* Send on ADV if instructed. */
    if((sendOnAdv) && (nwkIfCb.interfaces[idx].brIfType == MESH_ADV_BEARER) &&
       MeshBrSendBeaconPdu(nwkIfCb.interfaces[idx].brIfId,
                           ((meshNwkBeaconMeta_t *)pBeaconMeta)->pBeacon,
                           MESH_NWK_BEACON_NUM_BYTES))
    {
      ((meshNwkBeaconMeta_t *)pBeaconMeta)->refCount++;
    }

    /* Send on GATT if instructed. */
    if((sendOnGatt) && (nwkIfCb.interfaces[idx].brIfType == MESH_GATT_BEARER) &&
        MeshBrSendBeaconPdu(nwkIfCb.interfaces[idx].brIfId,
                            ((meshNwkBeaconMeta_t *)pBeaconMeta)->pBeacon,
                            MESH_NWK_BEACON_NUM_BYTES))
     {
       ((meshNwkBeaconMeta_t *)pBeaconMeta)->refCount++;
     }
  }

  /* Check if bearer accepted the PDU on at least one interface. */
  if (((meshNwkBeaconMeta_t *)pBeaconMeta)->refCount != 0)
  {
    /* Enqueue until bearer confirms sending. */
    WsfQueueEnq(&meshNwkBeaconCb.ackBeaconsQueue, pBeaconMeta);
  }
  else
  {
    /* Free memory. */
    WsfBufFree(pBeaconMeta);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Beacon generate complete callback used by the trigger API for a single NetKey.
 *
 *  \param[in] isSuccess    TRUE if beacon was generated successfully.
 *  \param[in] netKeyIndex  Global identifier of the NetKey.
 *  \param[in] pBeaconMeta  Pointer to beacon and meta information.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkBeaconTrigSingleCback(bool_t isSuccess, uint16_t netKeyIndex,
                                         meshNwkBeaconMeta_t *pBeaconMeta)
{
  /* On success, send on all interfaces. */
  if (isSuccess)
  {
    meshNwkBeaconManageSend(pBeaconMeta, TRUE, TRUE);
  }
  else
  {
    /* Free memory. */
    WsfBufFree(pBeaconMeta);
  }
  (void)netKeyIndex;
}

/*************************************************************************************************/
/*!
 *  \brief     Beacon generate complete callback used by the trigger API for all NetKeys.
 *
 *  \param[in] isSuccess    TRUE if beacon was generated successfully.
 *  \param[in] netKeyIndex  Global identifier of the NetKey.
 *  \param[in] pBeaconMeta  Pointer to beacon and meta information.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkBeaconTrigAllCback(bool_t isSuccess, uint16_t netKeyIndex,
                                      meshNwkBeaconMeta_t *pBeaconMeta)
{
  /* On success, send on all interfaces. */
  if (isSuccess)
  {
    meshNwkBeaconManageSend(pBeaconMeta, TRUE, TRUE);
  }
  else
  {
    /* Free memory. */
    WsfBufFree(pBeaconMeta);
  }

  /* Try to generate next beacon. */
  (void)meshNwkBeaconGenNext(&meshNwkBeaconCb.trigNetKeyIndexer, meshNwkBeaconTrigAllCback);
  (void)netKeyIndex;
}

/*************************************************************************************************/
/*!
 *  \brief     Beacon generate complete callback used for periodic broadcasting.
 *
 *  \param[in] isSuccess    TRUE if beacon was generated successfully.
 *  \param[in] netKeyIndex  Global identifier of the NetKey.
 *  \param[in] pBeaconMeta  Pointer to beacon and meta information.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkBeaconBcastGenComplCback(bool_t isSuccess, uint16_t netKeyIndex,
                                            meshNwkBeaconMeta_t *pBeaconMeta)
{
  /* Check if state changed to not broadcasting until beacon was generated. */
  if (!meshNwkBeaconCb.bcastOn)
  {
    /* Free memory. */
    WsfBufFree(pBeaconMeta);

    /* Don't continue sending to bearer. */
    return;
  }

  if (isSuccess)
  {
    meshNwkBeaconManageSend(pBeaconMeta, TRUE, FALSE);
  }
  else
  {
    /* Free memory. */
    WsfBufFree(pBeaconMeta);
  }

  /* Try to generate next beacon. */
  if (!meshNwkBeaconGenNext(&meshNwkBeaconCb.bcastNetKeyIndexer, meshNwkBeaconBcastGenComplCback))
  {
    /* End of NetKey (sub-net) list reached. Start timer to restart beacon generation. */
    WsfTimerStartSec(&(meshNwkBeaconCb.bcastTmr), MESH_NWK_BEACON_INTVL_SEC);
  }
  (void)netKeyIndex;
}

/*************************************************************************************************/
/*!
 *  \brief     Resumes beacon generation based on a NetKey list indexer.
 *
 *  \param[in] pIndexer  Pointer to indexer variable used for iterating in the NetKey List.
 *  \param[in] cback     Callback to be called at the end of generation.
 *
 *  \return    TRUE if new generation starts, FALSE if end of NetKey list reached.
 */
/*************************************************************************************************/
static bool_t meshNwkBeaconGenNext(uint16_t *pIndexer, meshBeaconGenInternalCback_t cback)
{
  meshNwkBeaconMeta_t *pBeaconMeta;
  uint16_t netKeyIndex;

  /* Find next NetKey Index. */
  while (MeshLocalCfgGetNextNetKeyIndex(&netKeyIndex, pIndexer)
         == MESH_SUCCESS)
  {
    /* Allocate beacon and meta. */
    if ((pBeaconMeta =
         meshNwkBeaconAllocateBeaconMeta(NULL, netKeyIndex, (meshNwkBeaconGenericCback_t)cback))
         == NULL)
    {
      /* Skip to next. */
      continue;
    }

    /* Call security to compute authentication. */
    if (meshNwkBeaconCb.genInProgr)
    {
      /* Enqueue in beacon generation queue. */
      WsfQueueEnq(&(meshNwkBeaconCb.txBeaconQueue), pBeaconMeta);
      return TRUE;
    }

    /* Call security to compute authentication. */
    if(MeshSecBeaconComputeAuth(pBeaconMeta->pBeacon, netKeyIndex,
                                BEACON_AUTH_WITH_NEW_KEY(netKeyIndex), secGenCback,
                                pBeaconMeta)
       != MESH_SUCCESS)
    {
      /* Free memory. */
      WsfBufFree(pBeaconMeta);

      /* Skip to next. */
      continue;
    }

    /* Set generation in progress flag. */
    meshNwkBeaconCb.genInProgr = TRUE;
    return TRUE;
  }

  /* No more keys found. Signal end. */
  return FALSE;
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
static void meshNwkBeaconWsfMsgHandlerCback(wsfMsgHdr_t *pMsg)
{
  /* Check event type to handle timer expiration. */
  switch(pMsg->event)
  {
    case MESH_NWK_BEACON_MSG_BCAST_TMR_EXPIRED:
      /* Reset indexer. */
      meshNwkBeaconCb.bcastNetKeyIndexer = 0;

      /* Restart generating. */
      if (!meshNwkBeaconGenNext(&meshNwkBeaconCb.bcastNetKeyIndexer, meshNwkBeaconBcastGenComplCback))
      {
        /* Error encountered probably. Start timer to restart beacon generation. */
        WsfTimerStartSec(&(meshNwkBeaconCb.bcastTmr), MESH_NWK_BEACON_INTVL_SEC);
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
 *  \brief  Initializes the Secure Network Beacon module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshNwkBeaconInit(void)
{
  /* Initialize control block. */
  WSF_QUEUE_INIT(&meshNwkBeaconCb.ackBeaconsQueue);
  WSF_QUEUE_INIT(&meshNwkBeaconCb.rxBeaconQueue);
  WSF_QUEUE_INIT(&meshNwkBeaconCb.txBeaconQueue);

  /* Register bearer callbacks. */
  MeshBrRegisterNwkBeacon(meshNwkBeaconEvtCback, meshNwkBeaconPduRecvCback);

  /* Register WSF message callback. */
  meshCb.nwkBeaconMsgCback = meshNwkBeaconWsfMsgHandlerCback;

  /* Set broadcast flag to FALSE. */
  meshNwkBeaconCb.bcastOn = FALSE;

  /* Reset security flags. */
  meshNwkBeaconCb.genInProgr = FALSE;
  meshNwkBeaconCb.authInProgr = FALSE;

  /* Reset indexer. */
  meshNwkBeaconCb.bcastNetKeyIndexer = 0;

  /* Configure timer. */
  meshNwkBeaconCb.bcastTmr.msg.event = MESH_NWK_BEACON_MSG_BCAST_TMR_EXPIRED;
  meshNwkBeaconCb.bcastTmr.handlerId = meshCb.handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief  Informs the module that the Beacon state has changed.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshNwkBeaconHandleStateChanged(void)
{
  bool_t bcast;
  meshBeaconStates_t state = MeshLocalCfgGetBeaconState();

  WSF_ASSERT(state != MESH_BEACON_PROHIBITED_START);

  /* Check if broadcasting is enabled and if node is not Proxy Client. */
  bcast = ((state == MESH_BEACON_BROADCASTING) && (!MeshProxyClIsSupported()));

  /* Check if same state. */
  if (bcast == meshNwkBeaconCb.bcastOn)
  {
    return;
  }

  /* Signal state. */
  meshNwkBeaconCb.bcastOn = bcast;

  if (bcast)
  {
    /* Reset indexer. */
    meshNwkBeaconCb.bcastNetKeyIndexer = 0;
    /* Start generating. */
    if (!meshNwkBeaconGenNext(&meshNwkBeaconCb.bcastNetKeyIndexer, meshNwkBeaconBcastGenComplCback))
    {
      /* Error encountered probably. Start timer to restart beacon generation. */
      WsfTimerStartSec(&(meshNwkBeaconCb.bcastTmr), MESH_NWK_BEACON_INTVL_SEC);
    }
  }
  else
  {
    /* Stop timer. */
    WsfTimerStop(&(meshNwkBeaconCb.bcastTmr));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends beacons on all available interfaces for one or all NetKeys as a result of a
 *             trigger.
 *
 *  \param[in] netKeyIndex  Index of the NetKey that triggered the beacon sending or
 *                          MESH_NWK_BEACON_SEND_ON_ALL_NETKEYS.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshNwkBeaconTriggerSend(uint16_t netKeyIndex)
{
  meshNwkBeaconMeta_t *pBeaconMeta;
  meshBeaconGenInternalCback_t cback;

  /* Validate parameters. */
  if ((netKeyIndex > MESH_NET_KEY_INDEX_MAX_VAL) && (netKeyIndex != MESH_NWK_BEACON_SEND_ON_ALL_NETKEYS))
  {
    return;
  }

  /* Check if should trigger sending for all. */
  if (netKeyIndex == MESH_NWK_BEACON_SEND_ON_ALL_NETKEYS)
  {
    /* Reset triggered indexer. */
    meshNwkBeaconCb.trigNetKeyIndexer = 0;

    /* Get first netKeyIndex. It should never fail as at least one NetKey exists on device. */
    (void)MeshLocalCfgGetNextNetKeyIndex(&netKeyIndex, &meshNwkBeaconCb.trigNetKeyIndexer);

    /* Assign callback that will manage future requests. */
    cback = meshNwkBeaconTrigAllCback;
  }
  else
  {
    /* Assign callback that will manage this single request. */
    cback = meshNwkBeaconTrigSingleCback;
  }

  /* Allocate beacon and meta. */
  if ((pBeaconMeta =
      meshNwkBeaconAllocateBeaconMeta(NULL, netKeyIndex, (meshNwkBeaconGenericCback_t)cback))
      == NULL)
  {
    return;
  }

  /* Check if security can authenticate the beacon now. */
  if (meshNwkBeaconCb.genInProgr)
  {
    /* Enqueue in beacon generation queue. */
    WsfQueueEnq(&(meshNwkBeaconCb.txBeaconQueue), pBeaconMeta);
    return;
  }

  /* Call security to compute authentication. */
  if(MeshSecBeaconComputeAuth(pBeaconMeta->pBeacon, netKeyIndex,
                              BEACON_AUTH_WITH_NEW_KEY(netKeyIndex), secGenCback,
                              pBeaconMeta)
     != MESH_SUCCESS)
  {
    /* Free memory. */
    WsfBufFree(pBeaconMeta);

    /* Nothing to do. Add a trace message. */
    MESH_TRACE_INFO0("MESH NWK BC: Security cannot authenticate triggered beacon ");

    return;
  }

  /* Signal in progress. */
  meshNwkBeaconCb.genInProgr = TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Generates a Secure Network Beacon for a given subnet.
 *
 *  \param[in] netKeyIndex  Global identifier of the NetKey associated to a subnet.
 *  \param[in] pBuf         Pointer to buffer where to store the beacon.
 *  \param[in] cback        Beacon generation complete callback.
 *
 *  \return    TRUE if beacon generation starts, FALSE otherwise.
 *
 *  \remarks   pBuf must point to a buffer of at least ::MESH_NWK_BEACON_NUM_BYTES bytes
 *
 */
/*************************************************************************************************/
bool_t MeshNwkBeaconGenOnDemand(uint16_t netKeyIndex, uint8_t *pBuf,
                                meshBeaconGenOnDemandCback_t cback)
{
  meshNwkBeaconMeta_t *pBeaconMeta;

  /* Validate parameters. */
  if ((pBuf == NULL) || (cback == NULL) || (netKeyIndex > MESH_NET_KEY_INDEX_MAX_VAL))
  {
    return FALSE;
  }

  /* Allocate beacon and meta. */
  if ((pBeaconMeta =
       meshNwkBeaconAllocateBeaconMeta(pBuf, netKeyIndex, (meshNwkBeaconGenericCback_t)cback))
       == NULL)
  {
    return FALSE;
  }

  /* Check if security can authenticate the beacon now. */
  if (meshNwkBeaconCb.genInProgr)
  {
    /* Enqueue in beacon generation queue. */
    WsfQueueEnq(&(meshNwkBeaconCb.txBeaconQueue), pBeaconMeta);
    return TRUE;
  }

  /* Call security to compute authentication. */
  if(MeshSecBeaconComputeAuth(pBeaconMeta->pBeacon, netKeyIndex,
                              BEACON_AUTH_WITH_NEW_KEY(netKeyIndex), secGenCback,
                              pBeaconMeta)
     != MESH_SUCCESS)
  {
    /* Free memory. */
    WsfBufFree(pBeaconMeta);

    /* Nothing to do. Exit with error. */
    return FALSE;
  }

  /* Signal in progress. */
  meshNwkBeaconCb.genInProgr = TRUE;

  return TRUE;
}
