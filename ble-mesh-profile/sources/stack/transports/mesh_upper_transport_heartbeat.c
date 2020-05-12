/*************************************************************************************************/
/*!
 *  \file   mesh_upper_transport_heartbeat.c
 *
 *  \brief  Heartbeat implementation.
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
#include "wsf_cs.h"
#include "wsf_timer.h"
#include "wsf_msg.h"
#include "wsf_os.h"
#include "wsf_assert.h"
#include "wsf_trace.h"
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_error_codes.h"

#include "mesh_api.h"
#include "mesh_main.h"
#include "mesh_seq_manager.h"
#include "mesh_local_config_types.h"
#include "mesh_local_config.h"
#include "mesh_lower_transport.h"
#include "mesh_upper_transport.h"

#include "mesh_upper_transport_heartbeat.h"

#include "mesh_utils.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Heartbeat control block */
typedef struct meshHbCb_tag
{
  uint32_t          pubCount;              /*!< Publication counter */
  uint32_t          pubPeriod;             /*!< Publication period */
  uint32_t          subCount;              /*!< Subscription counter */
  uint32_t          subPeriod;             /*!< Subscription period */
  wsfTimer_t        pubTmr;                /*!< Publication timer */
  wsfTimer_t        subTmr;                /*!< Subscription timer */
} meshHbCb_t;

/*! Mesh Heartbeat WSF message events */
enum meshHbMsgEvents
{
  MESH_HB_MSG_SUB_TMR_EXPIRED = MESH_HB_MSG_START, /*!< Subscription timer expired. */
  MESH_HB_MSG_PUB_TMR_EXPIRED,                     /*!< Publication timer expired. */
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Mesh Heartbeat control block */
static meshHbCb_t hbCb = { 0 };

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Checks if the Heartbeat Periodic Publication is enabled and messages shall be sent.
 *
 *  \return TRUE if enabled, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshHbPubEnabled(void)
{
  if (!MESH_IS_ADDR_UNASSIGNED(MeshLocalCfgGetHbPubDst()) &&
      (MeshLocalCfgGetHbPubPeriodLog() != 0x00) && (MeshLocalCfgGetHbPubCountLog() != 0x00))
  {
    return TRUE;
  }
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief  Checks if the Heartbeat Subscription is enabled and messages shall be processed.
 *
 *  \return TRUE if enabled, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t meshHbSubEnabled(void)
{
  if (!MESH_IS_ADDR_UNASSIGNED(MeshLocalCfgGetHbSubSrc()) &&
      !MESH_IS_ADDR_UNASSIGNED(MeshLocalCfgGetHbSubDst()) &&
      (MeshLocalCfgGetHbSubPeriodLog() != 0x00))
  {
    return TRUE;
  }
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief  Sends a Heartbeat Publish Message to the transport layer.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void meshHbPublishMessage(void)
{
  meshUtrCtlPduInfo_t hbCtlPduInfo;
  meshFeatures_t features;
  uint8_t ctlPdu[3];

  /* Set primary element address as source address. */
  MeshLocalCfgGetAddrFromElementId(0, &hbCtlPduInfo.src);

  /* Set destination address. */
  hbCtlPduInfo.dst = MeshLocalCfgGetHbPubDst();

  /* Set netKey index. */
  (void) MeshLocalCfgGetHbPubNetKeyIndex(&hbCtlPduInfo.netKeyIndex);

  /* Set TTL. */
  hbCtlPduInfo.ttl = MeshLocalCfgGetHbPubTtl();

  /* Set OpCode. */
  hbCtlPduInfo.opcode = MESH_UTR_CTL_HB_OPCODE;

  /* Set ACK Required to false. */
  hbCtlPduInfo.ackRequired = FALSE;

  /* Set priority to FALSE. */
  hbCtlPduInfo.prioritySend = FALSE;

  /* Set features. */
  features = MeshLocalCfgGetSupportedFeatures();

  /* Set InitTTL value. */
  ctlPdu[0] = hbCtlPduInfo.ttl;
  /* Set features bitmask value. */
  ctlPdu[1] = (features >> 8) & 0xFF;
  ctlPdu[2] = features & 0xFF;

  /* Add control PDU to PDU info. */
  hbCtlPduInfo.pCtlPdu = ctlPdu;
  hbCtlPduInfo.pduLen = sizeof(ctlPdu);

  /* Clear Friend or LPN address to use master credentials. */
  hbCtlPduInfo.friendLpnAddr = MESH_ADDR_TYPE_UNASSIGNED;
  hbCtlPduInfo.ifPassthr = FALSE;

  /* Send CTL PDU. */
  MeshUtrSendCtlPdu(&hbCtlPduInfo);
}

/*************************************************************************************************/
/*!
 *  \brief  Mesh Heartbeat Periodic Publishing Timer callback.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void meshHbPubTimerCback(void)
{
  uint8_t pubCountLog;

  /* Publish Heartbeat message. */
  meshHbPublishMessage();

  /* Decrement Publication Count if different than 0xFFFF and 0x0000. */
  if ((hbCb.pubCount != 0xFFFF) && (hbCb.pubCount > 0))
  {
    hbCb.pubCount--;

    pubCountLog = MeshUtilsGetLogFieldVal((uint16_t) hbCb.pubCount);

    /* Periodically update Heartbeat Publication Count Log value. */
    if (pubCountLog < MeshLocalCfgGetHbPubCountLog())
    {
      MeshLocalCfgSetHbPubCountLog(pubCountLog);
    }
  }

  /* Check if the timer should be restarted. */
  if (meshHbPubEnabled())
  {
    WsfTimerStartSec(&(hbCb.pubTmr), MESH_UTILS_GET_4OCTET_VALUE(MeshLocalCfgGetHbPubPeriodLog()));
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Mesh Heartbeat Subscription Period Timer callback.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void meshHbSubTimerCback(void)
{
  /* Clear period counts. */
  hbCb.subPeriod = 0x00000000;
  MeshLocalCfgSetHbSubPeriodLog(0x00);
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
static void meshHbWsfMsgHandlerCback(wsfMsgHdr_t *pMsg)
{
  switch(pMsg->event)
  {
    case MESH_HB_MSG_SUB_TMR_EXPIRED:
      meshHbSubTimerCback();
      break;
    case MESH_HB_MSG_PUB_TMR_EXPIRED:
      meshHbPubTimerCback();
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
 *  \brief  Initializes the Heartbeat module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshHbInit(void)
{
  uint8_t pubCountLog;

  MESH_TRACE_INFO0("MESH HBEAT: init");

  /* Set Heartbeat publication count. */
  pubCountLog = MeshLocalCfgGetHbPubCountLog();

  if (pubCountLog == 0xFF)
  {
    hbCb.pubCount = 0xFFFF;
  }
  else
  {
    hbCb.pubCount = MESH_UTILS_GET_4OCTET_VALUE(pubCountLog);
  }

  /* Set Heartbeat publication period. */
  hbCb.pubPeriod = MESH_UTILS_GET_4OCTET_VALUE(MeshLocalCfgGetHbPubPeriodLog());

  /* Initialize Heartbeat subscription count. */
  hbCb.subCount = 0x00000000;

  /* Store Heartbeat subscription period. */
  hbCb.subPeriod = MESH_UTILS_GET_4OCTET_VALUE(MeshLocalCfgGetHbSubPeriodLog());

  /* Register WSF message callback. */
  meshCb.hbMsgCback = meshHbWsfMsgHandlerCback;

  /* Configure timers. */
  hbCb.pubTmr.msg.event = MESH_HB_MSG_PUB_TMR_EXPIRED;
  hbCb.subTmr.msg.event = MESH_HB_MSG_SUB_TMR_EXPIRED;
  hbCb.pubTmr.handlerId = meshCb.handlerId;
  hbCb.subTmr.handlerId = meshCb.handlerId;

  /* Stop Heartbeat timers. */
  WsfTimerStop(&(hbCb.pubTmr));
  WsfTimerStop(&(hbCb.subTmr));

  /* Check if publication needs to be started. */
  if (meshHbPubEnabled() == TRUE)
  {
    /* Restart periodic publication. */
    meshHbPubTimerCback();
  }

  /* Check if subscription needs to be started. */
  if (meshHbSubEnabled() == TRUE)
  {
    /* Start subscription timer. */
    WsfTimerStartSec(&(hbCb.subTmr), hbCb.subPeriod);
  }
}

/*************************************************************************************************/
/*!
 *  \brief   Config Model Server module calls this function whenever Heartbeat Subscription State
 *           value is changed.
 *
 *  \return  None.
 */
/*************************************************************************************************/
void MeshHbSubscriptionStateChanged(void)
{
  /* Stop Heartbeat Subscription timer. */
  WsfTimerStop(&(hbCb.subTmr));

  /* Check if subscription needs to be started. */
  if (meshHbSubEnabled() == TRUE)
  {
    /* Clear the Heartbeat Subscription Count. */
    hbCb.subCount = 0x00000000;

    /* Clear the Heartbeat Subscription Count Log. */
    MeshLocalCfgSetHbSubCountLog(0x00);

    /* Set the Heartbeat Subscription Period. */
    hbCb.subPeriod = MESH_UTILS_GET_4OCTET_VALUE(MeshLocalCfgGetHbSubPeriodLog());

    /* Start subscription timer. */
    WsfTimerStartSec(&(hbCb.subTmr), hbCb.subPeriod);
  }
}

/*************************************************************************************************/
/*!
 *  \brief   Config Model Server module calls this function whenever Heartbeat Publication State
 *           value is changed.
 *
 *  \return  None.
 */
/*************************************************************************************************/
void MeshHbPublicationStateChanged(void)
{
  uint8_t pubCountLog;

  /* Stop Heartbeat Publication timer. */
  WsfTimerStop(&(hbCb.pubTmr));

  /* Set Heartbeat publication count. */
  pubCountLog = MeshLocalCfgGetHbPubCountLog();

  if (pubCountLog == 0xFF)
  {
    hbCb.pubCount = 0xFFFF;
  }
  else
  {
    hbCb.pubCount = MESH_UTILS_GET_4OCTET_VALUE(pubCountLog);
  }

  /* Set Heartbeat publication period. */
  hbCb.pubPeriod = MESH_UTILS_GET_4OCTET_VALUE(MeshLocalCfgGetHbPubPeriodLog());

  if (meshHbPubEnabled() == TRUE)
  {
    /* Start Period Publish Heartbeat timer. */
    WsfTimerStartSec(&(hbCb.pubTmr), MESH_UTILS_GET_4OCTET_VALUE(MeshLocalCfgGetHbPubPeriodLog()));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Signals the Heartbeat module that at least one Feature State value is changed.
 *
 *  \param[in] features  Bitmask for the changed features.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHbFeatureStateChanged(meshFeatures_t features)
{
  meshFeatures_t pubFeatures = MeshLocalCfgGetHbPubFeatures();

  /* Check if at least one feature bit is set and destination address is valid. */
  if (((features & pubFeatures) != 0) && !MESH_IS_ADDR_UNASSIGNED(MeshLocalCfgGetHbPubDst()))
  {
    /* Publish Heartbeat message. */
    meshHbPublishMessage();
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Asynchronously processes the given Heartbeat message PDU.
 *
 *  \param[in] pHbPdu  Pointer to the structure holding the received transport control PDU.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHbProcessHb(const meshLtrCtlPduInfo_t *pHbPdu)
{
  meshHbInfoEvt_t evt;
  uint8_t initTtl;
  uint8_t ttl;
  uint8_t *pPdu;

  WSF_ASSERT(pHbPdu != NULL);

  /* Check if Heartbeat Subscription is enabled. */
  if (meshHbSubEnabled())
  {
    /* Check if subscription source and destination addresses match the ones in Heartbeat
     * Subscription state.
     */
    if ((pHbPdu->src == MeshLocalCfgGetHbSubSrc()) && (pHbPdu->dst == MeshLocalCfgGetHbSubDst()))
    {
      /* Increment HB Subscription Count state. */
      if (hbCb.subCount < 0xFFFF)
      {
        hbCb.subCount++;

        if (MeshUtilsGetLogFieldVal((uint16_t) hbCb.subCount) > MeshLocalCfgGetHbSubCountLog())
        {
          /* Update Heartbeat Subscription Count Log. */
          MeshLocalCfgSetHbSubCountLog(MeshUtilsGetLogFieldVal((uint16_t) hbCb.subCount));
        }
      }

      /* Store the source address. */
      evt.src = pHbPdu->src;

      /* Store initial TTL value. */
      pPdu = pHbPdu->pUtrCtlPdu;
      BSTREAM_TO_UINT8(initTtl, pPdu);
      initTtl = MESH_UTILS_BF_GET(initTtl, MESH_TTL_SHIFT, MESH_TTL_SIZE);

      /* Store received TTL value. */
      ttl = pHbPdu->ttl;

      /* Compute hops value. */
      evt.hops = initTtl - ttl + 1;

      /* Get current maxHops and minHops. */
      evt.maxHops = MeshLocalCfgGetHbSubMaxHops();
      evt.minHops = MeshLocalCfgGetHbSubMinHops();

      /* Update maxHops value if lower than current hops value. */
      if (evt.maxHops < evt.hops)
      {
        evt.maxHops = evt.hops;
        MeshLocalCfgSetHbSubMaxHops(evt.maxHops);
      }

      /* Update minHops value if higher than current hops value. */
      if (evt.minHops > evt.hops)
      {
        evt.minHops = evt.hops;
        MeshLocalCfgSetHbSubMinHops(evt.minHops);
      }

      /* Store features */
      BSTREAM_BE_TO_UINT16(evt.features, pPdu);

      /* Signal event to the application. */
      evt.hdr.event = MESH_CORE_EVENT;
      evt.hdr.param = MESH_CORE_HB_INFO_EVENT;
      evt.hdr.status = MESH_SUCCESS;
      meshCb.evtCback((meshEvt_t*)&evt);
    }
  }
}
