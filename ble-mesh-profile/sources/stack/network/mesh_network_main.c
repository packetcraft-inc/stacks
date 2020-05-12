/*************************************************************************************************/
/*!
 *  \file   mesh_network.c
 *
 *  \brief  Network implementation main module.
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

#include <stdio.h>
#include <string.h>

#include "wsf_types.h"
#include "wsf_buf.h"
#include "wsf_cs.h"
#include "wsf_queue.h"
#include "wsf_timer.h"
#include "wsf_msg.h"
#include "wsf_os.h"
#include "wsf_assert.h"
#include "wsf_trace.h"
#include "sec_api.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_error_codes.h"

#include "mesh_api.h"
#include "mesh_main.h"
#include "mesh_seq_manager.h"
#include "mesh_local_config_types.h"
#include "mesh_local_config.h"
#include "mesh_adv_bearer.h"
#include "mesh_gatt_bearer.h"
#include "mesh_bearer.h"
#include "mesh_network.h"
#include "mesh_network_if.h"
#include "mesh_replay_protection.h"

#include "mesh_security.h"
#include "mesh_utils.h"

#include "mesh_network_main.h"

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
#include "mesh_test_api.h"
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Mesh Lower Transport PDU first byte position */
#define MESH_NWK_LTR_PDU_BYTE_OFFSET      (MESH_DST_ADDR_POS + sizeof(meshAddress_t))

/*! Network PDU Tag Mask for sending only on Advertising bearer */
#define MESH_NWK_PDU_TAG_MASK_ADV_ONLY    (MESH_NWK_TAG_SEND_ON_ADV_IF |\
                                           MESH_NWK_TAG_RLY_ON_ADV_IF  |\
                                           MESH_NWK_TAG_FWD_ON_ALL_IF  |\
                                           MESH_NWK_TAG_FWD_ON_ALL_IF_EXCEPT)

/*! Network PDU Tag Mask for sending only on GATT bearer */
#define MESH_NWK_PDU_TAG_MASK_GATT_ONLY   (MESH_NWK_TAG_SEND_ON_GATT_IF |\
                                           MESH_NWK_TAG_FWD_ON_ALL_IF   |\
                                           MESH_NWK_TAG_FWD_ON_ALL_IF_EXCEPT)

/*! Network PDU Tag Mask for PDU's sent only once */
#define MESH_NWK_PDU_TAG_MASK_NO_RETRANS  (MESH_NWK_TAG_FWD_ON_ALL_IF_EXCEPT |\
                                           MESH_NWK_TAG_FWD_ON_ALL_IF        |\
                                           MESH_NWK_TAG_SEND_ON_GATT_IF)

/*! Mesh Network random delay maximum value. */
#define MESH_NWK_RND_DELAY_MAX_MS         20

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Network WSF message events */
enum meshNwkWsfMsgEvents
{
  MESH_NWK_MSG_RETRANS_TMR_EXPIRED = MESH_NWK_MSG_START /*!< Retransmission timer expired */
};

/*! Network PDU bitmask tag to instruct the layer on how to send a PDU on the interfaces */
enum meshNwkPduTagBitMaskValues
{
  MESH_NWK_TAG_SEND_ON_ADV_IF       = (1 << 0),  /*!< Used for originating a Network PDU on an ADV
                                                  *   bearer
                                                  */
  MESH_NWK_TAG_SEND_ON_GATT_IF      = (1 << 1),  /*!< Used for originating a Network PDU on a GATT
                                                  *   bearer
                                                  */
  MESH_NWK_TAG_RLY_ON_ADV_IF        = (1 << 2),  /*!< Used for relaying a Network PDU
                                                  *   on all advertising bearers (relay)
                                                  */
  MESH_NWK_TAG_FWD_ON_ALL_IF        = (1 << 3),  /*!< Used for forwarding a Network PDU received
                                                  *   on ADV bearer to all bearers (proxy)
                                                  */
  MESH_NWK_TAG_FWD_ON_ALL_IF_EXCEPT = (1 << 4),  /*!< Used for forwarding a Network PDU received
                                                  *   on GATT bearer to all bearers except the
                                                  *   bearer on which it was received
                                                  */
};

/*! Bitfield tags associated to a Network PDU. See ::meshNwkPduTagBitMaskValues */
typedef uint8_t meshNwkPduBitfieldTag_t;

/*! Network PDU and meta information */
typedef struct meshNwkPduMeta_tag
{
  struct meshNwkPduMeta_t *pNext;          /*!< Pointer to next element for queuing */
  uint32_t                ivIndex;         /*!< IV Index used for security operations */
  wsfTimer_t              retransTmr;      /*!< Re-transmission timer */
  meshAddress_t           dstAddr;         /*!< Destination address. Used for filtering */
  meshAddress_t           friendLpnAddr;   /*!< Friend or LPN address to identify credentials
                                            *   used on encrypt
                                            */
  uint16_t                netKeyIndex;     /*!< Network Key (sub-net) Index used for security */
  meshNwkPduBitfieldTag_t nwkPduTag;       /*!< Tag to instruct the Network layer on how to send
                                            *   the PDU
                                            */
  meshBrInterfaceId_t     rcvdBrIfId;      /*!< Interface on which the PDU is received
                                            *   Invalid value if PDU is not relayed
                                            */
  uint16_t                pduRetransTime;  /*!< Time in milliseconds until next retransmission.
                                            *   Used for PDU's send on advertising interfaces
                                            */
  uint8_t                 pduRetransCount; /*!< Remaining retransmissions count (network transmit
                                            *   state or relay retransmit state)
                                            */
  uint8_t                 pduRefCount;     /*!< PDU reference count. Used to keep count
                                            *   of number of references to the network PDU
                                            *   sent to the bearer on unreliable interfaces
                                            */
  uint8_t                 pduLen;          /*!< Length of the network PDU */
  uint8_t                 nwkPdu[1];       /*!< First byte of the network PDU. Used to wrap on
                                            *   on large buffer. Must always be the last member
                                            *   of the structure
                                            */
} meshNwkPduMeta_t;

/*! Mesh Network control block */
typedef struct meshNwkCb_tag
{
  meshNwkRecvCback_t             nwkToLtrPduRecvCback;          /*!< Called to deliver PDU to
                                                                 *   LTR
                                                                 */
  meshNwkEventNotifyCback_t      nwkToLtrEventCback;            /*!< Called to deliver events to
                                                                 *   LTR
                                                                 */
  meshNwkFriendRxPduCheckCback_t lpnDstCheckCback;              /*!< Called to verify if an LPN
                                                                 *   needs an incoming PDU
                                                                 */
  meshNwkLpnRxPduNotifyCback_t   lpnRxPduNotifyCback;           /*!< Notify LPN if a message was
                                                                 *   received from Friend
                                                                 */
  meshNwkLpnRxPduFilterCback_t   lpnRxPduFilterCback;           /*!< Checks if a received PDU needs
                                                                 *   to be filtered
                                                                 */
  wsfQueue_t                     txPduQueue;                    /*!< Tx PDU queue */
  wsfQueue_t                     txSecQueue;                    /*!< Tx security queue */
  wsfQueue_t                     rxSecQueue;                    /*!< Tx security queue */
  bool_t                         nwkEncryptInProgress;          /*!< Flag used to signal encrypt is
                                                                 *   in progress;
                                                                 */
  bool_t                         nwkDecryptInProgress;          /*!< Flag used to signal decrypt is
                                                                 *   in progress;
                                                                 */
  uint16_t                       tmrUidGen;                     /*!< Unique timer identifier
                                                                 *   generator for transmission
                                                                 *   timers
                                                                 */
} meshNwkCb_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Mesh Network control block */
static meshNwkCb_t nwkCb = { 0 };

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Mesh Network empty receive callback.
 *
 *  \param[in] pNwkPduRxInfo  Mesh Network Rx PDU info structure.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkEmptyRecvCback(meshNwkPduRxInfo_t *pNwkPduRxInfo)
{
  (void)pNwkPduRxInfo;
  MESH_TRACE_WARN0("MESH NWK: Receive callback not set!");
  return;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Network empty event notification callback.
 *
 *  \param[in] event        Mesh Network Rx PDU info structure.
 *  \param[in] pEventParam  Pointer to the event parameter passed to the function.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkEmptyEventNotifyCback(meshNwkEvent_t event, void *pEventParam)
{
  (void)event;
  (void)pEventParam;
  MESH_TRACE_WARN0("MESH NWK: Notification callback not set!");
  return;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Network empty LPN destination check callback.
 *
 *  \param[in] dst          Destination address.
 *  \param[in] netKeyIndex  Global NetKey (sub-net) identifier.
 *
 *  \return    TRUE if at least one LPN is destination for this PDU.
 */
/*************************************************************************************************/
static bool_t meshNwkEmptyFriendLpnDstCheckCback(meshAddress_t dst, uint16_t netKeyIndex)
{
  (void)dst;
  (void)netKeyIndex;
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh LPN PDU received callback function pointer type.
 *
 *  \param[in] pNwkPduRxInfo  Network PDU RX info.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkEmptyLpnRxPduNotifyCback(meshNwkPduRxInfo_t *pNwkPduRxInfo)
{
  (void)pNwkPduRxInfo;
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh LPN PDU received filter callback function pointer type.
 *
 *  \param[in] netKeyIndex  NetKey index.
 *
 *  \return    TRUE if the PDU needs to be filtered, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshNwkEmptyLpnRxPduFilterCback(uint16_t netKeyIndex)
{
  (void)netKeyIndex;

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Reads (re)transmission count and/or time in milliseconds if the tag determines the
 *             parameters are needed.
 *
 *  \param[in] nwkPduTag            Tag associated to a Network PDU.
 *  \param[in] pOutRetransmitCount  Pointer to store number of retransmissions required.
 *  \param[in] pOutRetransmitTime   Pointer to store retransmission interval in milliseconds.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkReadTransmissionParams(meshNwkPduBitfieldTag_t nwkPduTag,
                                          uint8_t *pOutRetransmitCount,
                                          uint16_t *pOutRetransmitTime)
{
  uint8_t retransIntvlSteps10Milis = 0;

  WSF_ASSERT(pOutRetransmitCount != NULL);
  WSF_ASSERT(pOutRetransmitTime != NULL);

  /* If the PDU is sent by this node or relayed update retransmission count and time. */
  if (nwkPduTag & MESH_NWK_TAG_SEND_ON_ADV_IF)
  {
    /* Read Network Transmit state. */

    /* Read transmit count. */
    *pOutRetransmitCount = MeshLocalCfgGetNwkTransmitCount();

    /* If count is zero, time is set to zero as well. */
    if(*pOutRetransmitCount == 0)
    {
      *pOutRetransmitTime = 0;
    }
    else
    {
      /* Read transmit interval steps. */
      retransIntvlSteps10Milis = MeshLocalCfgGetNwkTransmitIntvlSteps();

      /* Casting to uint16_t ensures no rollover happens when adding 1. */
      *pOutRetransmitTime = ((uint16_t)retransIntvlSteps10Milis + 1) * 10;
    }
  }
  else if(nwkPduTag & MESH_NWK_TAG_RLY_ON_ADV_IF)
  {
    /* Read Relay Retransmit state */

    /* Read retransmit count. */
    *pOutRetransmitCount = MeshLocalCfgGetRelayRetransmitCount();

    /* If count is zero, time is set to zero as well. */
    if(*pOutRetransmitCount == 0)
    {
      *pOutRetransmitTime = 0;
    }
    else
    {
      /* Read retransmit interval steps. */
      retransIntvlSteps10Milis = MeshLocalCfgGetRelayRetransmitIntvlSteps();

      /* Calculate retransmission time in milliseconds. */
      *pOutRetransmitTime = ((uint16_t)retransIntvlSteps10Milis + 1) * 10;
    }
  }
  else
  {
    /* Reset parameters if tag does not require retransmissions. */
    *pOutRetransmitCount = 0;
    *pOutRetransmitTime = 0;
  }
}

/*************************************************************************************************/
/*! \brief     Mesh Network PDU send management function.
 *
 *  \param[in] pNwkPduMeta  Pointer to a Network PDU and meta information.
 *
 *  \remarks   This function sends references to a Network PDU to the bearer based on how the PDU
 *             is tagged and the result of the filters. It also clears "send once" tags after
 *             first transmission on each interface.
 */
/*************************************************************************************************/
static void meshNetworkManagePduSend(meshNwkPduMeta_t *pNwkPduMeta)
{
  uint8_t idx = 0;

  /* Go through all the interfaces. */
  for (idx = 0; idx < MESH_BR_MAX_INTERFACES; idx++)
  {
    /* Take decisions only on valid interfaces. */
    if (nwkIfCb.interfaces[idx].brIfId == MESH_BR_INVALID_INTERFACE_ID)
    {
      continue;
    }
    /* Take decisions based on bearer type. */
    if (nwkIfCb.interfaces[idx].brIfType == MESH_ADV_BEARER)
    {
      /* Do not send to this interface if the PDU is not tagged for Advertising bearers. */
      if (!(pNwkPduMeta->nwkPduTag & MESH_NWK_PDU_TAG_MASK_ADV_ONLY))
      {
        continue;
      }
    }
    else
    {
      /* Do not send to this interface if the PDU is not tagged for GATT bearers. */
      if (!(pNwkPduMeta->nwkPduTag & MESH_NWK_PDU_TAG_MASK_GATT_ONLY))
      {
        continue;
      }
      /* Do not send to this interface if the PDU was received on it. */
      if (((pNwkPduMeta->nwkPduTag) & MESH_NWK_TAG_FWD_ON_ALL_IF_EXCEPT))
      {
        if (pNwkPduMeta->rcvdBrIfId == nwkIfCb.interfaces[idx].brIfId)
        {
          continue;
        }
      }
    }
    /* Run the PDU through the interface filter. */
    if (meshNwkIfFilterOutMsg(&(nwkIfCb.interfaces[idx].outputFilter), pNwkPduMeta->dstAddr))
    {
      continue;
    }

    /* Send a PDU reference to the bearer. */
    if (MeshBrSendNwkPdu(nwkIfCb.interfaces[idx].brIfId,
                         pNwkPduMeta->nwkPdu,
                         pNwkPduMeta->pduLen))
    {
      /* Increment reference count. */
      ++(pNwkPduMeta->pduRefCount);
    }
  }

  /* Clear tags that are valid for sending once to avoid retransmissions on timer expiration. */
  MESH_UTILS_BITMASK_CLR((pNwkPduMeta->nwkPduTag), MESH_NWK_PDU_TAG_MASK_NO_RETRANS);

  return;
}

/*************************************************************************************************/
/*! \brief     Mesh Network Transmission Timer callback.
 *
 *  \param[in] uid  Unique timer identifier.
 *
 *  \return    None.
 *
 *  \remarks   Handling timer expiration assumes decrementing per PDU remaining time until
 *             retransmission, re-arming of the timer and calling the PDU sending management
 *             function.
 */
/*************************************************************************************************/
static void meshNwkTmrCback(uint16_t uid)
{
  meshNwkPduMeta_t *pNwkPduMeta  = NULL, *pPrevMeta = NULL;

  /* If the Tx queue is empty, return. */
  if (WsfQueueEmpty(&(nwkCb.txPduQueue)))
  {
    return;
  }

  /* Point to start of the queue. */
  pNwkPduMeta = (meshNwkPduMeta_t *)(&(nwkCb.txPduQueue))->pHead;
  pPrevMeta = NULL;

  /* Search for matching queue entry. */
  while(pNwkPduMeta != NULL)
  {
    /* Check if timer identifier matches expired timer. */
    if(pNwkPduMeta->retransTmr.msg.param == uid)
    {
      break;
    }
    pPrevMeta = pNwkPduMeta;
    pNwkPduMeta = (meshNwkPduMeta_t *)(pNwkPduMeta->pNext);
  }

  /* This should never happen, but guard anyway. */
  if(pNwkPduMeta == NULL)
  {
    return;
  }

  /* Timer only handles PDU's originating on this node or relayed PDU's. */
  if ((pNwkPduMeta->nwkPduTag & MESH_NWK_TAG_SEND_ON_ADV_IF) ||
      (pNwkPduMeta->nwkPduTag & MESH_NWK_TAG_RLY_ON_ADV_IF))
  {
    /* Validate other tags should not exist. */
    WSF_ASSERT(MESH_UTILS_BITMASK_XCL((pNwkPduMeta->nwkPduTag),
                                      (MESH_NWK_TAG_SEND_ON_ADV_IF | MESH_NWK_TAG_RLY_ON_ADV_IF)));
    /* Validate mutual exclusion between send and relay. */
    WSF_ASSERT(!MESH_UTILS_BITMASK_CHK((pNwkPduMeta->nwkPduTag),
                                         (MESH_NWK_TAG_SEND_ON_ADV_IF |
                                          MESH_NWK_TAG_RLY_ON_ADV_IF)));

    /* Network PDU transmission time management. */
    /* If additional retransmissions are required, timer must be restarted. */
    if (pNwkPduMeta->pduRetransCount > 0)
    {
      /* Decrement number of retransmissions. */
      --(pNwkPduMeta->pduRetransCount);

      /* Check if timer not already restarted and should be restarted. */
      if (pNwkPduMeta->pduRetransCount != 0)
      {
        /* Re-arm transmission timer. */
        WsfTimerStartMs(&(pNwkPduMeta->retransTmr), pNwkPduMeta->pduRetransTime);
      }
      else
      {
        /* Clear retransmission time. */
        pNwkPduMeta->pduRetransTime = 0;
      }
    }
    /* Prepare and send references of the PDU to the bearer. */
    meshNetworkManagePduSend(pNwkPduMeta);
  }

  /* Determine if the PDU should be freed. */
  if ((pNwkPduMeta->pduRefCount == 0) && (pNwkPduMeta->pduRetransTime == 0) &&
      (pNwkPduMeta->pduRetransCount == 0))
  {
    /* Remove from queue. */
    WsfQueueRemove(&(nwkCb.txPduQueue), pNwkPduMeta, pPrevMeta);

    /* No reason to keep the PDU, free memory. */
    WsfBufFree(pNwkPduMeta);
  }
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
static void meshNwkWsfMsgHandlerCback(wsfMsgHdr_t *pMsg)
{
  /* Check event type to handle timer expiration. */
  switch(pMsg->event)
  {
    case MESH_NWK_MSG_RETRANS_TMR_EXPIRED:
      /* Invoke timer callback. */
      meshNwkTmrCback(pMsg->param);
      break;
    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Configures Network Encryption parameters and sends a request to the Security Module.
 *
 *  \param[in] pNwkPduMeta  Pointer to structure containing network PDU and meta information.
 *  \param[in] secCback     Security module callback to be called at the end of encryption.
 *
 *  \return    Success or error reason. See ::meshReturnValues.
 */
/*************************************************************************************************/
static meshNwkRetVal_t meshNwkEncryptRequest(meshNwkPduMeta_t *pNwkPduMeta,
                                             meshSecNwkEncObfCback_t secCback)
{
  meshSecNwkEncObfParams_t encParams;

  /* Extract CTL. */
  uint8_t ctl = MESH_UTILS_BF_GET(pNwkPduMeta->nwkPdu[MESH_CTL_TTL_POS],
                                  MESH_CTL_SHIFT,
                                  MESH_CTL_SIZE);

  /* Compute NetMic size. */
  uint8_t netMicSize = (ctl != 0) ? MESH_NETMIC_SIZE_CTL_PDU : MESH_NETMIC_SIZE_ACC_PDU;

  /* Set pointer to PDU at the beginning of the Network PDU. */
  encParams.pNwkPduNoMic = (uint8_t *)(pNwkPduMeta->nwkPdu);

  /* Set size of network PDU excluding NetMic. */
  encParams.nwkPduNoMicSize = pNwkPduMeta->pduLen - netMicSize;

  /* Set NetMic size. */
  encParams.netMicSize = netMicSize;

  /* Set pointer to NetMic at the end of the Network PDU. */
  encParams.pNwkPduNetMic = ((pNwkPduMeta->nwkPdu) + (encParams.nwkPduNoMicSize));

  /* Set pointer to encrypted and obfuscated Network PDU same as input pointer. */
  encParams.pObfEncNwkPduNoMic = (uint8_t *)(pNwkPduMeta->nwkPdu);

  /* Set NetKey Index. */
  encParams.netKeyIndex = pNwkPduMeta->netKeyIndex;

  /* Set friendship credentials address to unassigned. */
  encParams.friendOrLpnAddress = pNwkPduMeta->friendLpnAddr;

  /* Set IV Index. */
  encParams.ivIndex = pNwkPduMeta->ivIndex;

  /* Call to security to encrypt the PDU. */
  return (meshNwkRetVal_t)MeshSecNwkEncObf(FALSE, &encParams, secCback, (void *)pNwkPduMeta);
}

/*************************************************************************************************/
/*! \brief     Mesh Security Network PDU encryption and obfuscation complete callback.
 *
 *  \param[in] isSuccess           TRUE if operation completed successfully.
 *  \param[in] isProxyConfig       TRUE if Network PDU is a Proxy Configuration Message.
 *  \param[in] pObfEncNwkPduNoMic  Pointer to buffer where the encrypted and obfuscated
 *                                 network PDU is stored.
 *  \param[in] nwkPduNoMicSize     Size of the network PDU excluding NetMIC.
 *  \param[in] pNwkPduNetMic       Pointer to buffer where the calculated NetMIC is stored.
 *  \param[in] netMicSize          Size of the calculated NetMIC.
 *  \param[in] pParam              Pointer to generic callback parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkEncObfCompleteCback(bool_t isSuccess, bool_t isProxyConfig,
                                       uint8_t *pObfEncNwkPduNoMic, uint8_t nwkPduNoMicSize,
                                       uint8_t *pNwkPduNetMic, uint8_t  netMicSize, void *pParam)
{
  uint8_t rndDelay = 0;

  (void)isProxyConfig;
  (void)pObfEncNwkPduNoMic;
  (void)nwkPduNoMicSize;
  (void)pNwkPduNetMic;
  (void)netMicSize;

  meshNwkPduMeta_t *pNwkPduMeta = (meshNwkPduMeta_t *)pParam;

  if (isSuccess == FALSE)
  {
    /* Silently abort as there is nothing to do. */
    WsfBufFree(pNwkPduMeta);
  }
  else
  {
    /* Send the PDU to bearer interfaces if not relay only is required. */
    if (pNwkPduMeta->nwkPduTag != MESH_NWK_TAG_RLY_ON_ADV_IF)
    {
      meshNetworkManagePduSend(pNwkPduMeta);

      /* If the PDU should be discarded, free it. */
      if ((pNwkPduMeta->pduRefCount == 0) &&
          (pNwkPduMeta->pduRetransCount == 0) &&
          (pNwkPduMeta->pduRetransTime == 0))
      {
        /* No reason to keep the PDU, free memory. */
        WsfBufFree(pNwkPduMeta);
      }
      else
      {
        /* Enqueue the PDU in the Network queue. At least the reference count is not zero,
         * so the PDU must be stored
         */
        WsfQueueEnq(&(nwkCb.txPduQueue), pNwkPduMeta);

        /* Check if the PDU must be retransmitted on ADV bearers. */
        if ((pNwkPduMeta->nwkPduTag & MESH_NWK_TAG_SEND_ON_ADV_IF) ||
            (pNwkPduMeta->nwkPduTag & MESH_NWK_TAG_RLY_ON_ADV_IF))
        {
          if (pNwkPduMeta->pduRetransTime)
          {
            /* Start transmission timer. */
            WsfTimerStartMs(&(pNwkPduMeta->retransTmr), pNwkPduMeta->pduRetransTime);
          }
        }
      }
    }
    /* If only relay is required. */
    else
    {
      /* Enqueue the PDU in the Network queue. Wait for Relay random delay timer expiration. */
      WsfQueueEnq(&(nwkCb.txPduQueue), pNwkPduMeta);

      /* Start a small random timer and postpone Tx. */
      SecRand(&rndDelay, sizeof(rndDelay));
      WsfTimerStartMs(&(pNwkPduMeta->retransTmr), (rndDelay % MESH_NWK_RND_DELAY_MAX_MS) + 1);

      /* Increment count so that delay timer expiration is not considered retransmission. */
      ++(pNwkPduMeta->pduRetransCount);
    }
  }
  /* Clear encrypt flag. */
  nwkCb.nwkEncryptInProgress = FALSE;

  /* Resume encryption if pending PDU's. */
  pNwkPduMeta = WsfQueueDeq(&(nwkCb.txSecQueue));

  while (pNwkPduMeta != NULL)
  {
    /* Set encrypt in progress flag. */
    nwkCb.nwkEncryptInProgress = TRUE;

    /* Request encryption. */
    if (meshNwkEncryptRequest(pNwkPduMeta, meshNwkEncObfCompleteCback) != MESH_SUCCESS)
    {
      /* Free the queue element. */
      WsfBufFree(pNwkPduMeta);

      /* Clear encrypt flag. */
      nwkCb.nwkEncryptInProgress = FALSE;
    }
    else
    {
      /* Break loop. */
      break;
    }

    /* Get next element. */
    pNwkPduMeta = WsfQueueDeq(&(nwkCb.txSecQueue));
  }

  return;
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the meta information for a Network PDU.
 *
 *  \param[in] pNwkPduMeta  Pointer to structure containing pointer to
 *                          Network PDU and meta information.
 *  \param[in] brIfId       Interface on which the PDU was received. Relevant in case
 *                          it is a relayed PDU or a Proxy Network PDU. NULL otherwise.
 *  \param[in] nwkPduTag    Bitfield value describing how the PDU should be sent.
 *  \param[in] dstAddr      Destination address used in filtering.
 *  \param[in] ifPassthr    Friendship pass-through flag for Network interface.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkSetMetaInfo(meshNwkPduMeta_t *pNwkPduMeta, meshBrInterfaceId_t brIfId,
                               meshNwkPduBitfieldTag_t nwkPduTag, meshAddress_t dstAddr,
                               bool_t ifPassthr)
{
  /* Set received interface (invalid if it is not a relayed PDU). */
  if (brIfId == MESH_BR_INVALID_INTERFACE_ID)
  {
    pNwkPduMeta->rcvdBrIfId = MESH_BR_INVALID_INTERFACE_ID;
    /* Validate that the interface is not NULL for a PDU received over a GATT bearer while
     * Proxy is active.
     */
    WSF_ASSERT((nwkPduTag & MESH_NWK_TAG_FWD_ON_ALL_IF_EXCEPT) == 0);
  }
  else
  {
    pNwkPduMeta->rcvdBrIfId = brIfId;
  }

  /* Bypass retransmissions of network PDU's. */
  if (ifPassthr)
  {
    pNwkPduMeta->pduRetransCount = 0;
    pNwkPduMeta->pduRetransTime = 0;
  }
  else
  {
    /* Read transmission parameters. */
    meshNwkReadTransmissionParams(nwkPduTag,
                                  &(pNwkPduMeta->pduRetransCount),
                                  &(pNwkPduMeta->pduRetransTime));
  }

  /* Reset reference count since the PDU is not sent yet through any bearer. */
  pNwkPduMeta->pduRefCount = 0;

  /* Tag the PDU with information on how it needs to be sent. */
  pNwkPduMeta->nwkPduTag = nwkPduTag;

  /* Set destination address used in filtering. */
  pNwkPduMeta->dstAddr = dstAddr;

  /* Configure transmission timer. */
  pNwkPduMeta->retransTmr.msg.event = MESH_NWK_MSG_RETRANS_TMR_EXPIRED;
  pNwkPduMeta->retransTmr.msg.param = nwkCb.tmrUidGen++;
  pNwkPduMeta->retransTmr.handlerId = meshCb.handlerId;

  return;
}

/*************************************************************************************************/
/*!
 *  \brief     Prepares given transport PDU to be sent by network layer.
 *
 *  \param[in] pNwkPduTxInfo  Pointer to a buffer containing a Lower Transport PDU and additional
 *                            fields. See ::meshNwkPduTxInfo_t
 *  \param[in] nwkPduTag      Bitfield value describing how the PDU should be sent.
 *
 *  \return    Success or error. See ::meshReturnValues
 */
/*************************************************************************************************/
static meshNwkRetVal_t meshNwkSendLtrPduInternal(const meshNwkPduTxInfo_t *pNwkPduTxInfo,
                                                 meshNwkPduBitfieldTag_t nwkPduTag)
{
  uint8_t *pHdr          = NULL;
  uint8_t *pDataPtr      = NULL;
  meshNwkRetVal_t retVal = MESH_SUCCESS;
  bool_t  ivUpdtInProgress = FALSE;

  /* Compute Nwk PDU len. */
  uint8_t pduLen = pNwkPduTxInfo->ltrHdrLen + pNwkPduTxInfo->utrPduLen + MESH_NWK_HEADER_LEN;

  /* Compute NetMic size. */
  uint8_t netMicSize = (pNwkPduTxInfo->ctl != 0) ? MESH_NETMIC_SIZE_CTL_PDU :
                                                   MESH_NETMIC_SIZE_ACC_PDU;

  /* Add NetMic size to PDU length. */
  pduLen += netMicSize;

  /* Allocate memory. PDU meta already contains first byte of the Network PDU. Subtract 1. */
  meshNwkPduMeta_t *pNwkPduMeta = WsfBufAlloc(sizeof(meshNwkPduMeta_t) - 1 + pduLen);

  /* If no memory is available, return proper error code. */
  if (pNwkPduMeta == NULL)
  {
    return MESH_OUT_OF_MEMORY;
  }

  /* Set PDU length. */
  pNwkPduMeta->pduLen = pduLen;

  /* Set pointer to header of network PDU. */
  pHdr = (uint8_t *)(pNwkPduMeta->nwkPdu);

  /* Pack Network PDU header with 0 for IVI and NID since security will set those fields. */
  MeshNwkPackHeader(pNwkPduTxInfo, pHdr, 0, 0);

  /* Set data pointer after Network PDU Header. */
  pDataPtr = (uint8_t *)(pNwkPduMeta->nwkPdu) + MESH_NWK_HEADER_LEN;

  /* Copy Lower Transport PDU header. */
  memcpy((void *)(pDataPtr), (void *)pNwkPduTxInfo->pLtrHdr, pNwkPduTxInfo->ltrHdrLen);

  /* Set data pointer after the Lower Transport PDU Header. */
  pDataPtr = (uint8_t *)(pNwkPduMeta->nwkPdu) + MESH_NWK_HEADER_LEN + pNwkPduTxInfo->ltrHdrLen;

  /* Copy Upper Transport PDU. */
  memcpy((void *)(pDataPtr), (void *)pNwkPduTxInfo->pUtrPdu, pNwkPduTxInfo->utrPduLen);

  /* Set PDU meta information such as transmission params, received interface,
   * PDU tag, filter destination address.
   */
  meshNwkSetMetaInfo(pNwkPduMeta, MESH_BR_INVALID_INTERFACE_ID, nwkPduTag, pNwkPduTxInfo->dst,
                     pNwkPduTxInfo->ifPassthr);

  /* Set NetKey index. */
  pNwkPduMeta->netKeyIndex = pNwkPduTxInfo->netKeyIndex;

  /* Set friend or LPN address to identify security material. */
  pNwkPduMeta->friendLpnAddr = pNwkPduTxInfo->friendLpnAddr;

  /* Read IV index. */
  pNwkPduMeta->ivIndex = MeshLocalCfgGetIvIndex(&ivUpdtInProgress);

  if (ivUpdtInProgress)
  {
    /* For IV update in progress procedures, the IV index must be decremented by 1.
     * Make sure IV index is not 0.
     */
    WSF_ASSERT(pNwkPduMeta->ivIndex != 0);
    if (pNwkPduMeta->ivIndex != 0)
    {
      --(pNwkPduMeta->ivIndex);
    }
  }

  /* Set IV index. */

  /* Check if another encryption is in progress. */
  if (nwkCb.nwkEncryptInProgress)
  {
    if(pNwkPduTxInfo->prioritySend)
    {
      /* Push in front of the encrypt queue. */
      WsfQueuePush(&(nwkCb.txSecQueue), (void *)pNwkPduMeta);
    }
    else
    {
      /* Enqueue the PDU in the TX encrypt queue. */
      WsfQueueEnq(&(nwkCb.txSecQueue), (void *)pNwkPduMeta);
    }
  }
  else
  {
    /* Set encrypt in progress flag. */
    nwkCb.nwkEncryptInProgress = TRUE;

    /* Prepare encrypt request. */
    retVal = meshNwkEncryptRequest(pNwkPduMeta, meshNwkEncObfCompleteCback);

    /* Check return value. */
    if (retVal != MESH_SUCCESS)
    {
      /* Free memory. */
      WsfBufFree(pNwkPduMeta);

      /* Clear encrypt in progress flag. */
      nwkCb.nwkEncryptInProgress = FALSE;
    }
  }

  return retVal;
}

/*************************************************************************************************/
/*!
 *  \brief     Extracts required information from a decrypted Network PDU.
 *
 *  \param[in] pNwkPdu        Pointer a decrypted Network PDU.
 *  \param[in] pduLen         Network PDU length (includes NetMic size).
 *  \param[in] pNwkPduRxInfo  Pointer to a structure containing unpacked fields of the Network PDU.
 *
 *  \return    None.
 *
 *  \remarks   This function will set the pointer to the Lower Transport PDU at on offset of
 *             pNwkPdu and does not perform any copying of the actual LTR PDU.
 */
/*************************************************************************************************/
static void meshNwkGetRxInfoFromPdu(uint8_t *pNwkPdu,uint8_t pduLen, meshNwkPduRxInfo_t *pNwkPduRxInfo)
{
  uint8_t netMicSize = 0;

  /* Extract CTL. */
  pNwkPduRxInfo->ctl = MESH_UTILS_BF_GET(*(pNwkPdu + MESH_CTL_TTL_POS),
                                         MESH_CTL_SHIFT,
                                         MESH_CTL_SIZE);

  /* Extract TTL. */
  pNwkPduRxInfo->ttl = MESH_UTILS_BF_GET(*(pNwkPdu + MESH_CTL_TTL_POS),
                                         MESH_TTL_SHIFT,
                                         MESH_TTL_SIZE);
  /* Extract Sequence number. */
  pNwkPduRxInfo->seqNo = (*(pNwkPdu + MESH_SEQ_POS) << 16) |
                         (*(pNwkPdu + MESH_SEQ_POS + 1) << 8) |
                         (*(pNwkPdu + MESH_SEQ_POS + 2));

  /* Extract Source and Destination Addresses. */
  pNwkPduRxInfo->src = (*(pNwkPdu + MESH_SRC_ADDR_POS) << 8) |
                       (*(pNwkPdu + MESH_SRC_ADDR_POS + 1));

  pNwkPduRxInfo->dst = (*(pNwkPdu + MESH_DST_ADDR_POS) << 8) |
                       (*(pNwkPdu + MESH_DST_ADDR_POS + 1));


  /* Set pointer to Lower Transport PDU. */
  pNwkPduRxInfo->pLtrPdu = (pNwkPdu + MESH_NWK_LTR_PDU_BYTE_OFFSET);

  /* Compute NetMic Size. */
  netMicSize = pNwkPduRxInfo->ctl ? MESH_NETMIC_SIZE_CTL_PDU : MESH_NETMIC_SIZE_ACC_PDU;

  /* Set Lower Transport PDU length. */
  pNwkPduRxInfo->pduLen = pduLen - MESH_NWK_HEADER_LEN - netMicSize;
}

/*************************************************************************************************/
/*!
 *  \brief      Tags a received Network PDU with additional information.
 *
 *  \param[in]  pNwkPduRxInfo  Pointer an unpacked Network PDU with additional information.
 *  \param[in]  pNwkIf         Pointer to Network Interface which the PDU was received.
 *  \param[out] pOutFwdToLtr   Pointer to location where TRUE is stored if the PDU must be sent to
 *                             Lower Transport or FALSE otherwise.
 *  \param[out] pOutTag        Pointer to location where to store the tags associated to the
 *                             received Network PDU.
 *
 *  \return     None.
 *
 *  \remarks    This function decides if the PDU should be forwarded to the Lower Transport and if
 *              the PDU must be forwarded/relayed.
 */
/*************************************************************************************************/
static void meshNwkTagRxPdu(meshNwkPduRxInfo_t *pNwkPduRxInfo, meshNwkIf_t *pNwkIf,
                            bool_t *pOutFwdToLtr, meshNwkPduBitfieldTag_t *pOutTag)
{
  /* Declare relay state. */
  meshRelayStates_t relayState;

  /* Declare proxy state. */
  meshGattProxyStates_t proxyState;

  /* Declare element variable. */
  meshElement_t *pElement = NULL;

  /* Get Own Element address. */
  meshAddress_t elem0Addr;

  /* Set default to FALSE. */
  *pOutFwdToLtr = FALSE;

  /* Reset tag. */
  *pOutTag = 0;

  /* Read relay state. */
  relayState = MeshLocalCfgGetRelayState();

  /* Read proxy state. */
  proxyState = MeshLocalCfgGetGattProxyState();

  /* Determine if PDU needs to be sent to LTR. */

  /* Bearer should always be valid. */
  WSF_ASSERT(pNwkIf != NULL);

  /* If address is unicast check elements. */
  if (MESH_IS_ADDR_UNICAST(pNwkPduRxInfo->dst))
  {
    /* Search address in the element list */
    MeshLocalCfgGetElementFromAddress(pNwkPduRxInfo->dst, (const meshElement_t**) &pElement);

    if(pElement != NULL)
    {
      *pOutFwdToLtr = TRUE;
    }
  }
  else
  {
    /* Check if the non-unicast destination address is in any subscription list. */
    if (MeshLocalCfgFindSubscrAddr(pNwkPduRxInfo->dst) ||
        MESH_IS_ADDR_FIXED_GROUP(pNwkPduRxInfo->dst))
    {
      *pOutFwdToLtr = TRUE;
    }
  }

  /* Check if PDU is not a replay attack on local elements or subscribed addresses. */
  if(*pOutFwdToLtr &&
      MeshRpIsReplayAttack(pNwkPduRxInfo->src, pNwkPduRxInfo->seqNo, pNwkPduRxInfo->ivIndex))
  {
    *pOutFwdToLtr = FALSE;
  }

  /* Determine if PDU needs to be sent to LTR due to friendships established. */

  /* Search in friend module if at least one LPN is destination for the PDU. */
  if(!(*pOutFwdToLtr) && nwkCb.lpnDstCheckCback(pNwkPduRxInfo->dst, pNwkPduRxInfo->netKeyIndex))
  {
    *pOutFwdToLtr = TRUE;
  }

  /* Determine if PDU needs to be relayed or forwarded. */

  /* If TTL is less or equal to 1, don't add tags. */
  if (pNwkPduRxInfo->ttl <= MESH_TX_TTL_FILTER_VALUE)
  {
    return;
  }

  /* Determine tags. */
  /* If address is unicast and is requested by the upper layer, do not apply any tags. */
  if (MESH_IS_ADDR_UNICAST(pNwkPduRxInfo->dst) && (*pOutFwdToLtr))
  {
    return;
  }

  /* The address may be unicast and not destination to any elements of this node or LPN elements or
   * group/virtual.
   * For all cases, check features and received PDU interface to determine tags.
   */
  switch (pNwkIf->brIfType)
  {
    case MESH_ADV_BEARER:
      if (relayState == MESH_RELAY_FEATURE_ENABLED)
      {
        /* Get own element address. */
        MeshLocalCfgGetAddrFromElementId(0, &elem0Addr);

        /* Do not relay your own messages. */
        if ((pNwkPduRxInfo->src  < elem0Addr) || (pNwkPduRxInfo->src > elem0Addr + pMeshConfig->elementArrayLen))
        {
          *pOutTag |= MESH_NWK_TAG_RLY_ON_ADV_IF;
        }
      }

      if (proxyState == MESH_GATT_PROXY_FEATURE_ENABLED)
      {
        *pOutTag |= MESH_NWK_TAG_FWD_ON_ALL_IF;
      }
      break;
    case MESH_GATT_BEARER:
      /* GATT bearers should not exist if Proxy is not enabled. */
      if (proxyState == MESH_GATT_PROXY_FEATURE_ENABLED)
      {
        *pOutTag |= MESH_NWK_TAG_FWD_ON_ALL_IF_EXCEPT;
      }
      break;
    default:
      /* Should never happen. */
      WSF_ASSERT((pNwkIf->brIfType) < MESH_INVALID_BEARER);
      break;
  }
}
/*************************************************************************************************/
/*!
 *  \brief     Processes a received decrypted Network PDU which has meta information attached.
 *
 *  \param[in] pNwkPduMeta  Pointer to a received Network PDU with meta information.
 *  \param[in] ivIndex      IV index that successfully decrypted the PDU.
 *
 *  \return    None.
 *
 *  \remarks   Processing includes cache checking, determining if the PDU should be delivered
 *             to the upper layer and/or forwarded or relayed on network interfaces.
 */
/*************************************************************************************************/
static void meshNwkProcessRxPdu(meshNwkPduMeta_t *pNwkPduMeta, uint32_t ivIndex)
{
  meshNwkPduRxInfo_t      nwkPduRxInfo;
  meshNwkIf_t             *pNwkIf;
  meshNwkPduBitfieldTag_t nwkPduTag  = 0;
  meshNwkCacheRetVal_t    retVal;
  bool_t                  fwdToLtr   = FALSE;

  /* Obtain Rx PDU Info. */
  meshNwkGetRxInfoFromPdu((uint8_t *)(pNwkPduMeta->nwkPdu),
                          pNwkPduMeta->pduLen,
                          &nwkPduRxInfo);

  /* Store NetKey Index. */
  nwkPduRxInfo.netKeyIndex = pNwkPduMeta->netKeyIndex;

  /* Set friend or LPN address to identify what security material was used on decrypt. */
  nwkPduRxInfo.friendLpnAddr = pNwkPduMeta->friendLpnAddr;

  /* Store IV Index. */
  nwkPduRxInfo.ivIndex = ivIndex;

  /* Check for invalid addresses. */
  if ((MESH_IS_ADDR_RFU(nwkPduRxInfo.dst)) || (MESH_IS_ADDR_UNASSIGNED(nwkPduRxInfo.dst)))
  {
    /* Free the PDU as the destination address is invalid. */
    WsfBufFree(pNwkPduMeta);

    return;
  }

  /* Extract interface. */
  pNwkIf = meshNwkIfBrIdToNwkIf(pNwkPduMeta->rcvdBrIfId);

  if (pNwkIf == NULL)
  {
    /* Free the PDU as the bearer is invalid. */
    WsfBufFree(pNwkPduMeta);

    return;
  }

  /* Check for GATT interface. */
  if ((meshCb.proxyIsServer) && (pNwkIf->brIfType == MESH_GATT_BEARER))
  {
    if (pNwkIf->outputFilter.filterType == MESH_NWK_WHITE_LIST)
    {
      MeshNwkIfAddAddressToFilter(pNwkIf->brIfId, nwkPduRxInfo.src);
    }
    else
    {
      MeshNwkIfRemoveAddressFromFilter(pNwkIf->brIfId, nwkPduRxInfo.src);
    }
  }

  /* Check if the PDU should be filtered by friendship. */
  if (nwkCb.lpnRxPduFilterCback(pNwkPduMeta->netKeyIndex) &&
      !MESH_IS_ADDR_UNICAST(pNwkPduMeta->friendLpnAddr))
  {
    /* Free the PDU as it was received with Master Security Credentials on a subnet with
     * Friendship established. */
    WsfBufFree(pNwkPduMeta);

    return;
  }

  /* Check if Friend message was received. */
  if (MESH_IS_ADDR_UNICAST(nwkPduRxInfo.friendLpnAddr))
  {
    /* Notify LPN. */
    nwkCb.lpnRxPduNotifyCback(&nwkPduRxInfo);
  }

  /* Run through Level 2 Cache. */
  retVal = meshNwkCacheAdd(MESH_NWK_CACHE_L2, pNwkPduMeta->nwkPdu, pNwkPduMeta->pduLen);

  /* Validate that cache returns only success or already exists due to correct initialization and
   * param validation.
   */
  WSF_ASSERT((retVal == MESH_SUCCESS) || (retVal == MESH_NWK_CACHE_ALREADY_EXISTS));

  /* Check if the PDU is already in L2 Cache. */
  if (retVal == MESH_NWK_CACHE_ALREADY_EXISTS)
  {
    /* The PDU meta is provided in the request and is allocated. */
    WsfBufFree(pNwkPduMeta);

    return;
  }

#if ((defined MESH_ENABLE_TEST) && (MESH_ENABLE_TEST==1))
  meshTestNwkPduRcvdInd_t nwkPduRcvdInd;

  if (meshTestCb.listenMask & MESH_TEST_NWK_LISTEN)
  {
    nwkPduRcvdInd.hdr.event = MESH_TEST_EVENT;
    nwkPduRcvdInd.hdr.param = MESH_TEST_NWK_PDU_RCVD_IND;
    nwkPduRcvdInd.hdr.status = MESH_SUCCESS;

    nwkPduRcvdInd.pLtrPdu     = nwkPduRxInfo.pLtrPdu;
    nwkPduRcvdInd.pduLen      = nwkPduRxInfo.pduLen;
    nwkPduRcvdInd.nid         = (pNwkPduMeta->nwkPdu[0] & MESH_NID_MASK);
    nwkPduRcvdInd.ctl         = nwkPduRxInfo.ctl;
    nwkPduRcvdInd.ttl         = nwkPduRxInfo.ttl;
    nwkPduRcvdInd.src         = nwkPduRxInfo.src;
    nwkPduRcvdInd.dst         = nwkPduRxInfo.dst;
    nwkPduRcvdInd.seqNo       = nwkPduRxInfo.seqNo;
    nwkPduRcvdInd.ivIndex     = nwkPduRxInfo.ivIndex;
    nwkPduRcvdInd.netKeyIndex = nwkPduRxInfo.netKeyIndex;

    meshTestCb.testCback((meshTestEvt_t *)&nwkPduRcvdInd);
  }
#endif

  /* Get tag and if should forward to LTR based on address and features. */
  meshNwkTagRxPdu(&nwkPduRxInfo, pNwkIf, &fwdToLtr, &nwkPduTag);

  /* If the PDU should be forward to Lower transport, invoke callback. */
  if (fwdToLtr)
  {
    /* Check for replay attack. */
    if (nwkCb.nwkToLtrPduRecvCback != NULL)
    {
      /* Invoke callback to send the PDU to Lower Transport. */
      nwkCb.nwkToLtrPduRecvCback(&nwkPduRxInfo);
    }
  }

  /* If the PDU is tagged, it must be relayed or forwarded (implicitly TTL > 1). */
  if (nwkPduTag != 0)
  {
    /* Decrement TTL. */
    --(nwkPduRxInfo.ttl);

    /* Make sure relay uses master credentials. */
    pNwkPduMeta->friendLpnAddr = MESH_ADDR_TYPE_UNASSIGNED;

    /* Set received IV index. */
    pNwkPduMeta->ivIndex = ivIndex;

    /* Set new TTL into the PDU. */
    MESH_UTILS_BF_SET((pNwkPduMeta->nwkPdu[MESH_CTL_TTL_POS]),
                      (nwkPduRxInfo.ttl),
                      MESH_TTL_SHIFT,
                      MESH_TTL_SIZE);

    /* Set meta information based on tag. */
    meshNwkSetMetaInfo(pNwkPduMeta,
                       pNwkPduMeta->rcvdBrIfId,
                       nwkPduTag,
                       nwkPduRxInfo.dst,
                       FALSE);

    /* Check if another encryption is in progress. */
    if (nwkCb.nwkEncryptInProgress)
    {
      /* Enqueue the PDU in the TX input queue. */
      WsfQueueEnq(&(nwkCb.txSecQueue), (void *)pNwkPduMeta);
    }
    else
    {
      /* Set encrypt in progress flag. */
      nwkCb.nwkEncryptInProgress = TRUE;

      /* Prepare encrypt request. */
      if (meshNwkEncryptRequest(pNwkPduMeta, meshNwkEncObfCompleteCback) != MESH_SUCCESS)
      {
        /* Free memory. */
        WsfBufFree(pNwkPduMeta);

        /* Clear encrypt in progress flag. */
        nwkCb.nwkEncryptInProgress = FALSE;
      }
    }
  }
  else
  {
    /* Free the PDU as it is not needed. */
    WsfBufFree(pNwkPduMeta);
  }

  return;
}

/*************************************************************************************************/
/*!
 *  \brief     Triggers a network decrypt request.
 *
 *  \param[in] pRecvPduMeta  Pointer to a received Network PDU with meta information.
 *  \param[in] secCback      Security module callback to be invoked at the end of decryption.
 *
 *  \return    Success or error reason. See ::meshReturnValues.
 */
/*************************************************************************************************/
static meshNwkRetVal_t meshNwkDecryptRequest(meshNwkPduMeta_t *pRecvPduMeta,
                                             meshSecNwkDeobfDecCback_t secCback)
{
  meshSecNwkDeobfDecParams_t nwkDecParams;

  /* Set deobfuscation and decryption parameters. */

  /* Set pointer to encrypted and obfuscated Network PDU. */
  nwkDecParams.pObfEncAuthNwkPdu = (uint8_t *)(pRecvPduMeta->nwkPdu);

  /* Set length of the PDU. */
  nwkDecParams.nwkPduSize = pRecvPduMeta->pduLen;

  /* Set pointer to destination buffer for the decrypted PDU. */
  nwkDecParams.pNwkPduNoMic = (uint8_t *)(pRecvPduMeta->nwkPdu);

  /* Request Security module to attempt to decrypt. Set pointer to received
   * interface as generic parameter.
   */
  return (meshNwkRetVal_t)MeshSecNwkDeobfDec(FALSE, &nwkDecParams, secCback, (void *)pRecvPduMeta);
}

/*************************************************************************************************/
/*! \brief     Mesh Security Network deobfuscation and decryption complete callback implementation.
 *
 *  \param[in] isSuccess        TRUE if operation completed successfully.
 *  \param[in] isProxyConfig    TRUE if Network PDU is a Proxy Configuration Message.
 *  \param[in] pNwkPduNoMic     Pointer to buffer the deobfuscated and decrypted network PDU is
 *                              stored.
 *  \param[in] nwkPduSizeNoMic  Size of the deobfuscated and decrypted network PDU excluding NetMIC.
 *  \param[in] netKeyIndex      Global network key index associated to the key that successfully
 *                              processed the received network PDU.
 *  \param[in] ivIndex          IV index that successfully authenticated the PDU.
 *  \param[in] friendOrLpnAddr  Friend or LPN address if friendship credentials were used.
 *  \param[in] pParam           Pointer to generic callback parameter provided in the request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkDeobfDecCompleteCback(bool_t isSuccess, bool_t isProxyConfig,
                                         uint8_t *pNwkPduNoMic, uint8_t nwkPduSizeNoMic,
                                         uint16_t netKeyIndex, uint32_t ivIndex,
                                         meshAddress_t friendOrLpnAddr, void *pParam)
{
  (void)isProxyConfig;
  (void)nwkPduSizeNoMic;
  meshNwkPduMeta_t *pNwkPduMeta = (meshNwkPduMeta_t *)pParam;

  /* If decryption failed, free the memory and abort. */
  if ((isSuccess == FALSE)                         ||
      (pNwkPduNoMic == NULL)                       ||
      (pNwkPduMeta == NULL)                        ||
      (pNwkPduMeta->pduLen > MESH_NWK_MAX_PDU_LEN) ||
      (pNwkPduMeta->pduLen < MESH_NWK_MIN_PDU_LEN))
  {
    /* The PDU meta is provided in the request and is allocated. */
    WsfBufFree((void *)pNwkPduMeta);
  }
  else
  {
    /* Set NetKey index. */
    pNwkPduMeta->netKeyIndex = netKeyIndex;

    /* Set friend or LPN address to identify what security material was used on decrypt. */
    pNwkPduMeta->friendLpnAddr = friendOrLpnAddr;

    /* Process the PDU. */
    meshNwkProcessRxPdu(pNwkPduMeta, ivIndex);
  }
  /* Clear decrypt in progress flag. */
  nwkCb.nwkDecryptInProgress = FALSE;

  /* Run maintenance on decryption. */
  pNwkPduMeta = WsfQueueDeq(&(nwkCb.rxSecQueue));

  while (pNwkPduMeta != NULL)
  {
    /* Set decrypt in progress flag. */
    nwkCb.nwkDecryptInProgress = TRUE;

    /* Request decryption. */
    if (meshNwkDecryptRequest(pNwkPduMeta, meshNwkDeobfDecCompleteCback) != MESH_SUCCESS)
    {
      /* Free memory. */
      WsfBufFree(pNwkPduMeta);

      /* Clear decrypt in progress flag. */
      nwkCb.nwkDecryptInProgress = FALSE;
    }
    else
    {
      /* Break loop. */
      break;
    }

    /* Dequeue next on previous fail. */
    pNwkPduMeta = WsfQueueDeq(&(nwkCb.rxSecQueue));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles incoming network PDU's from the bearer.
 *
 *  \param[in] brIfId   Unique identifier of the interface.
 *  \param[in] pNwkPdu  Pointer to the network PDU.
 *  \param[in] pduLen   Length of the network PDU.
 *
 *  \return    None.
 */
 /*************************************************************************************************/
static void meshBrToNwkPduRecvCback(meshBrInterfaceId_t brIfId, const uint8_t *pNwkPdu, uint8_t pduLen)
{
  meshNwkPduMeta_t *pRecvPduMeta;
  meshNwkCacheRetVal_t retVal;

  /* Should never happen since bearer validates this. */
  WSF_ASSERT(brIfId != MESH_BR_INVALID_INTERFACE_ID);

  /* Validate incoming PDU. */
  if (pNwkPdu == NULL)
  {
    return;
  }

  /* Validate PDU length. */
  if ((pduLen > MESH_NWK_MAX_PDU_LEN) || (pduLen < MESH_NWK_MIN_PDU_LEN))
  {
    return;
  }

  /* Validate interface ID is registered. */
  if (brIfId == MESH_BR_INVALID_INTERFACE_ID || meshNwkIfBrIdToNwkIf(brIfId) == NULL)
  {
    return;
  }

  /* Check if there is any NID matching existing key material. This is used to protect cache from
   * accumulating useless entries.
   */
  if (!MeshSecNidExists(MESH_UTILS_BF_GET(pNwkPdu[0], MESH_NID_SHIFT, MESH_NID_SIZE)))
  {
    return;
  }

  /* Check Level 1 cache to see if duplicate and return. */
  retVal = meshNwkCacheAdd(MESH_NWK_CACHE_L1, pNwkPdu, pduLen);

  /* Validate other results should not be possible if correct initialization and param validation.
   */
  WSF_ASSERT((retVal == MESH_SUCCESS) || (retVal == MESH_NWK_CACHE_ALREADY_EXISTS));

  /* If entry is found in cache, discard. */
  if (retVal == MESH_NWK_CACHE_ALREADY_EXISTS)
  {
    return;
  }

  /* Allocate memory for the PDU and meta information. The structure already contains one byte of
   * the PDU, so substract 1 from length.
   */
  pRecvPduMeta = WsfBufAlloc(sizeof(meshNwkPduMeta_t) + pduLen - 1);

  /* Check if allocation successful and abort on error. */
  if (pRecvPduMeta == NULL)
  {
    return;
  }

  /* Set received interface ID. */
  pRecvPduMeta->rcvdBrIfId = brIfId;

  /* Copy PDU. */
  memcpy(pRecvPduMeta->nwkPdu, (void *)pNwkPdu, pduLen);

  /* Set PDU length. */
  pRecvPduMeta->pduLen = pduLen;

  /* Check if another decryption is in progress. */
  if (nwkCb.nwkDecryptInProgress)
  {
    /* Enqueue the request. */
    WsfQueueEnq(&(nwkCb.rxSecQueue), (void *)pRecvPduMeta);
  }
  else
  {
    /* Set decrypt in progress flag. */
    nwkCb.nwkDecryptInProgress = TRUE;

    if (meshNwkDecryptRequest(pRecvPduMeta, meshNwkDeobfDecCompleteCback) != MESH_SUCCESS)
    {
      /* Free memory. */
      WsfBufFree(pRecvPduMeta);

      /* Clear decrypt in progress flag. */
      nwkCb.nwkDecryptInProgress = FALSE;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Manages the TX queue when a PDU has been sent by the bearer.
 *
 *  \param[in] pNwkPdu  Pointer to the network PDU that is to be searched in the Tx Queue.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshNwkHandlePduSentEvent(uint8_t *pNwkPdu)
{
  meshNwkPduMeta_t *pNwkPduMeta, *pPrev;

  if (pNwkPdu == NULL)
  {
    return;
  }

  /* If the Network Tx Queue is empty return. */
  if (WsfQueueEmpty(&(nwkCb.txPduQueue)))
  {
    return;
  }

  /* Set previous to NULL. */
  pPrev = NULL;
  /* Point to start of the queue. */
  pNwkPduMeta = (meshNwkPduMeta_t*)(&(nwkCb.txPduQueue))->pHead;

  /* Iterate through the network queue. */
  while (pNwkPduMeta != NULL)
  {
    /* Check if the sent PDU is a reference of the PDU stored by the meta information. */
    /* Note: pNwkPduMeta cannot be NULL */
    /* coverity[dereference] */
    if (pNwkPduMeta->nwkPdu == pNwkPdu)
    {
      /* Additional validation on reference count. */
      /* coverity[dereference] */
      if (pNwkPduMeta->pduRefCount > 0)
      {
        /* Decrement reference count. */
        --(pNwkPduMeta->pduRefCount);
      }

      /* If the PDU was found, check if it can be removed. */
      if ((pNwkPduMeta->pduRefCount == 0) &&
          (pNwkPduMeta->pduRetransCount == 0) &&
          (pNwkPduMeta->pduRetransTime == 0))
      {
        /* Remove from queue */
        WsfQueueRemove(&(nwkCb.txPduQueue), pNwkPduMeta, pPrev);
        WsfBufFree(pNwkPduMeta);
      }

      /* If the PDU was found end search. */
      break;
    }
    /* Move to next entry */
    pPrev = pNwkPduMeta;
    pNwkPduMeta = (meshNwkPduMeta_t*)(pNwkPduMeta->pNext);
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
  /* Parameter validation. */
  if (brIfId == MESH_BR_INVALID_INTERFACE_ID ||
      pEventParams == NULL)
  {
    /* Should never happen as these are validated by the bearer. */
    WSF_ASSERT(brIfId != MESH_BR_INVALID_INTERFACE_ID);
    WSF_ASSERT(pEventParams != NULL);
    return;
  }

  /* Handle events by type. */
  switch (event)
  {
    /* Handle new interface open. */
    case MESH_BR_INTERFACE_OPENED_EVT:
      /* Should never happen as this is verified by the bearer. */
      WSF_ASSERT(pEventParams->brConfig.bearerType < MESH_INVALID_BEARER);

      if (pEventParams->brConfig.bearerType < MESH_INVALID_BEARER)
      {
        /* Add the interface. */
        meshNwkIfAddInterface(brIfId, pEventParams->brConfig.bearerType);
      }
      break;
    /* Handle interface closed. */
    case MESH_BR_INTERFACE_CLOSED_EVT:
      /* Should never happen as this is verified by the bearer. */
      WSF_ASSERT(pEventParams->brConfig.bearerType < MESH_INVALID_BEARER);

      if (pEventParams->brConfig.bearerType < MESH_INVALID_BEARER)
      {
        /* Remove interface. */
        meshNwkIfRemoveInterface(brIfId);
      }
      break;
    case MESH_BR_INTERFACE_PACKET_SENT_EVT:
      /* Should never happen as these are validated by the bearer. */
      WSF_ASSERT(pEventParams->brPduStatus.pPdu != NULL);

      /* Handle PDU sent event. */
      meshNwkHandlePduSentEvent(pEventParams->brPduStatus.pPdu);
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
 *  \brief  Computes the required memory to be provided based on the given configuration.
 *
 *  \return Required memory in bytes or ::MESH_MEM_REQ_INVALID_CFG in case of fail.
 */
/*************************************************************************************************/
uint32_t MeshNwkGetRequiredMemory(void)
{
  uint32_t reqMem = MESH_MEM_REQ_INVALID_CFG;

  if ((pMeshConfig->pMemoryConfig == NULL) ||
      (pMeshConfig->pMemoryConfig->nwkCacheL1Size < MESH_NWK_CACHE_MIN_SIZE) ||
      (pMeshConfig->pMemoryConfig->nwkCacheL2Size < MESH_NWK_CACHE_MIN_SIZE))
  {
    return reqMem;
  }

  /* Compute the required memory. */
  reqMem = meshNwkCacheGetRequiredMemory() +
           meshNwkIfGetRequiredMemory(pMeshConfig->pMemoryConfig->nwkOutputFilterSize);

  return reqMem;
}

/*************************************************************************************************/
/*!
 *  \brief  Initializes the network layer.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshNwkInit(void)
{
  MESH_TRACE_INFO0("MESH NWK: init");

  /* Initialize network cache */
  meshNwkCacheInit();

  /* Initialize the Network Interfaces */
  meshNwkIfInit();

  /* Store empty callbacks into local structure. */
  nwkCb.nwkToLtrPduRecvCback = meshNwkEmptyRecvCback;
  nwkCb.nwkToLtrEventCback = meshNwkEmptyEventNotifyCback;
  nwkCb.lpnDstCheckCback = meshNwkEmptyFriendLpnDstCheckCback;
  nwkCb.lpnRxPduNotifyCback = meshNwkEmptyLpnRxPduNotifyCback;
  nwkCb.lpnRxPduFilterCback = meshNwkEmptyLpnRxPduFilterCback;

  /* Clear security in progress flags. */
  nwkCb.nwkEncryptInProgress = FALSE;
  nwkCb.nwkDecryptInProgress = FALSE;

  /* Initialize Tx PDU (output) queue. */
  WSF_QUEUE_INIT(&(nwkCb.txPduQueue));

  /* Initialize Tx and Rx security (input) queues. */
  WSF_QUEUE_INIT(&(nwkCb.txSecQueue));
  WSF_QUEUE_INIT(&(nwkCb.rxSecQueue));

  /* Initialize bearer. */
  MeshBrRegisterNwk(meshBrEventNotificationCback, meshBrToNwkPduRecvCback);

  /* Register WSF message callback. */
  meshCb.nwkMsgCback = meshNwkWsfMsgHandlerCback;

  /* Reset Tx timer UID generator. */
  nwkCb.tmrUidGen = 0;
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the required callbacks.
 *
 *  \param[in] recvCback   Callback to be invoked when a Network PDU is received.
 *  \param[in] eventCback  Event notification callback for the upper layer.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshNwkRegister(meshNwkRecvCback_t recvCback,
                     meshNwkEventNotifyCback_t eventCback)
{
  /* Validate function parameters. */
  if ((recvCback == NULL) || (eventCback == NULL))
  {
    MESH_TRACE_ERR0("MESH NWK: Invalid callbacks registered!");
    return;
  }

  /* Store callbacks into local structure. */
  nwkCb.nwkToLtrPduRecvCback = recvCback;
  nwkCb.nwkToLtrEventCback = eventCback;
}

/*************************************************************************************************/
/*!
 *  \brief     Sends the given transport PDU to the network layer.
 *
 *  \param[in] pNwkPduTxInfo  Pointer to a buffer containing a Lower Transport PDU and additional
 *                            fields. See ::meshNwkPduTxInfo_t
 *
 *  \return    Success or error code. See ::meshReturnValues
 */
/*************************************************************************************************/
meshNwkRetVal_t MeshNwkSendLtrPdu(const meshNwkPduTxInfo_t *pNwkPduTxInfo)
{
  uint8_t pduLen;

  /* NULL pointer validation. */
  if (pNwkPduTxInfo == NULL          ||
      pNwkPduTxInfo->pLtrHdr == NULL ||
      pNwkPduTxInfo->pUtrPdu == NULL)
  {
    return MESH_NWK_INVALID_PARAMS;
  }

  /* Source adress validation. */
  if (!MESH_IS_ADDR_UNICAST(pNwkPduTxInfo->src))
  {
    return MESH_NWK_INVALID_PARAMS;
  }

  /* Destination address validation. */
  if (!MESH_IS_ADDR_UNICAST(pNwkPduTxInfo->dst)     &&
      !MESH_IS_ADDR_GROUP(pNwkPduTxInfo->dst)       &&
      !MESH_IS_ADDR_VIRTUAL(pNwkPduTxInfo->dst))
  {
    return MESH_NWK_INVALID_PARAMS;
  }

  /* TTL validation. */
  if (pNwkPduTxInfo->ttl > MESH_TTL_MASK)
  {
    return MESH_NWK_INVALID_PARAMS;
  }

  /* CTL validation. */
  if (pNwkPduTxInfo->ctl > (MESH_CTL_MASK >> MESH_CTL_SHIFT))
  {
    return MESH_NWK_INVALID_PARAMS;
  }

  /* Sequence number validation. */
  if (pNwkPduTxInfo->seqNo > MESH_SEQ_MAX_VAL)
  {
    return MESH_NWK_INVALID_PARAMS;
  }

  /* Compute Network PDU length. */
  pduLen = MESH_NWK_HEADER_LEN + pNwkPduTxInfo->ltrHdrLen + pNwkPduTxInfo->utrPduLen +
           ((pNwkPduTxInfo->ctl) ? MESH_NETMIC_SIZE_CTL_PDU : MESH_NETMIC_SIZE_ACC_PDU);

  /* Verify that the total length doesn't exceed maximum transmission unit. */
  if (pduLen > MESH_NWK_MAX_PDU_LEN)
  {
      return MESH_NWK_INVALID_PARAMS;
  }

  /* Validate NetKey range. */
  if (pNwkPduTxInfo->netKeyIndex > MESH_NET_KEY_INDEX_MAX_VAL)
  {
    return MESH_SEC_TOOL_INVALID_PARAMS;
  }

  /* Prepare PDU for sending. */
  return meshNwkSendLtrPduInternal(pNwkPduTxInfo,
                                   (MESH_NWK_TAG_SEND_ON_ADV_IF | MESH_NWK_TAG_SEND_ON_GATT_IF));
}

/*************************************************************************************************/
/*!
 *  \brief     Packs a Network PDU header using the parameters provided in the request
 *
 *  \param[in] pNwkPduTxInfo  Pointer to a structure holding network PDU. See ::meshNwkPduTxInfo_t
 *  \param[in] pHdr           Pointer to a buffer where the packed Network PDU Header is stored
 *  \param[in] ivi            Least significant bit of the IV Index
 *  \param[in] nid            Network Identifier derived from the Network Key
 *
 *  \remarks   The NID is not deduced internally due to insufficient information such as type of
 *             credentials used which differ in case of operations involving friendship.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshNwkPackHeader(const meshNwkPduTxInfo_t *pNwkPduTxInfo, uint8_t *pHdr, uint8_t ivi,
                       uint8_t nid)
{
  /* Pack NID and IVI. */
  *(pHdr + MESH_IVI_NID_POS) = (((ivi << MESH_IVI_SHIFT) & MESH_IVI_MASK) |
                               ((nid << MESH_NID_SHIFT) & MESH_NID_MASK));


  /* Pack CTL and TTL. */
  *(pHdr + MESH_CTL_TTL_POS) = (((pNwkPduTxInfo->ctl << MESH_CTL_SHIFT) & MESH_CTL_MASK) |
                               ((pNwkPduTxInfo->ttl << MESH_TTL_SHIFT) & MESH_TTL_MASK));

  /* Pack Sequence number. */
  *(pHdr + MESH_SEQ_POS)     = (uint8_t)(pNwkPduTxInfo->seqNo >> 16);
  *(pHdr + MESH_SEQ_POS + 1) = (uint8_t)(pNwkPduTxInfo->seqNo >> 8);
  *(pHdr + MESH_SEQ_POS + 2) = (uint8_t)(pNwkPduTxInfo->seqNo);


  /* Pack Source Address */
  *(pHdr + MESH_SRC_ADDR_POS)     = (uint8_t)(pNwkPduTxInfo->src >> 8);
  *(pHdr + MESH_SRC_ADDR_POS + 1) = (uint8_t)(pNwkPduTxInfo->src);

  /* Pack Destination Address */
  *(pHdr + MESH_DST_ADDR_POS)     = (uint8_t)(pNwkPduTxInfo->dst >> 8);
  *(pHdr + MESH_DST_ADDR_POS + 1) = (uint8_t)(pNwkPduTxInfo->dst);
}

/*************************************************************************************************/
/*!
 *  \brief     Registers callback that verifies if an LPN is destination for a PDU.
 *
 *  \param[in] rxPduCheckCback  Rx PDU check callback.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshNwkRegisterFriend(meshNwkFriendRxPduCheckCback_t rxPduCheckCback)
{
  if (rxPduCheckCback != NULL)
  {
    nwkCb.lpnDstCheckCback = rxPduCheckCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Registers LPN callbacks.
 *
 *  \param[in] rxPduNotifyCback  PDU received from Friend notification callback.
 *  \param[in] rxPduFilterCback  Filter received packets on a specific subnet if Friendship is
 *                               established.
 *
 *  \return    None.
 */
/*************************************************************************************************/

void MeshNwkRegisterLpn(meshNwkLpnRxPduNotifyCback_t rxPduNotifyCback,
                        meshNwkLpnRxPduFilterCback_t rxPduFilterCback)
{
  if ((rxPduNotifyCback != NULL) && (rxPduFilterCback != NULL))
  {
    nwkCb.lpnRxPduNotifyCback = rxPduNotifyCback;
    nwkCb.lpnRxPduFilterCback = rxPduFilterCback;
  }
}
