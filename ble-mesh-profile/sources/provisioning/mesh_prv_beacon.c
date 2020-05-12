/*************************************************************************************************/
/*!
 *  \file   mesh_prv_beacon.c
 *
 *  \brief  Mesh Provisioning beacon module implementation.
 *
 *  Copyright (c) 2010-2018 Arm Ltd. All Rights Reserved.
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
#include "wsf_cs.h"
#include "wsf_os.h"
#include "wsf_assert.h"
#include "wsf_trace.h"
#include "util/bstream.h"

#include "mesh_error_codes.h"
#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"
#include "mesh_main.h"

#include "mesh_utils.h"
#include "mesh_bearer.h"
#include "mesh_security_toolbox.h"
#include "mesh_prv_defs.h"
#include "mesh_prv.h"
#include "mesh_prv_beacon.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Provisioning Beacon Control Block */
typedef struct meshPrvBeaconCb_tag
{
  uint32_t            beaconInterval;                 /*!< Beacon interval */
  wsfTimer_t          beaconTmr;                      /*!< Beacon timer */
  uint8_t             *pUriData;                      /*!< Pointer to URI data */
  uint8_t             pdu[MESH_PRV_MAX_BEACON_SIZE];  /*!< Beacon PDU to be sent */
  uint8_t             pduLen;                         /*!< Beacon PDU length */
  uint8_t             uriLen;
  meshBrInterfaceId_t brIfId;                         /*!< Bearer interface used */
} meshPrvBeaconCb_t;

/*! Provisioning Beacon WSF message events */
enum meshPrvBeaconWsfMsgEvents
{
  MESH_PRV_BEACON_MSG_TMR_EXPIRED = MESH_PRV_BEACON_MSG_START, /*!< Beacon timer expired */
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

static meshPrvBeaconCb_t  prvBeaconCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Callback invoked when a salt value has been computed.
 *
 *  \param[in] pCmacResult  Pointer to the result buffer.
 *  \param[in] pParam       Generic parameter.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshPrvSaltCback(const uint8_t *pCmacResult, void *pParam)
{
  /* Check that the beacon is still in use. Maybe it was stopped during the salt calculation. */
  if (prvBeaconCb.pduLen != 0)
  {
    /* Handle success. */
    if (pCmacResult != NULL)
    {
      /* Copy hash */
      memcpy(&prvBeaconCb.pdu[MESH_PRV_BEACON_URI_HASH_OFFSET], pCmacResult,
             MESH_PRV_BEACON_URI_HASH_SIZE);

      /* Send beacon to bearer */
      MeshBrSendBeaconPdu(prvBeaconCb.brIfId, prvBeaconCb.pdu, prvBeaconCb.pduLen);

      /* Start timer. */
      WsfTimerStartMs(&(prvBeaconCb.beaconTmr), prvBeaconCb.beaconInterval);
    }
  }

  /* Free buffer used for URI */
  WsfBufFree(prvBeaconCb.pUriData);

  (void)pParam;
}

/*************************************************************************************************/
/*! \brief  Mesh Provisioning Beacon Timer callback.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void meshPrvBeaconTimerCback(void)
{
  /* Send beacon to bearer */
  MeshBrSendBeaconPdu(prvBeaconCb.brIfId, prvBeaconCb.pdu, prvBeaconCb.pduLen);

  /* Restart timer. */
  WsfTimerStartMs(&(prvBeaconCb.beaconTmr), prvBeaconCb.beaconInterval);
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
static void meshPrvBeaconWsfMsgHandlerCback(wsfMsgHdr_t *pMsg)
{
  /* Check event type to handle timer expiration. */
  switch(pMsg->event)
  {
    case MESH_PRV_BEACON_MSG_TMR_EXPIRED:
      meshPrvBeaconTimerCback();
      break;
    default:
      break;
  }
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Provisioning Beacon functionality.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshPrvBeaconInit(void)
{
  /* Register WSF message callback. */
  meshCb.prvBeaconMsgCback = meshPrvBeaconWsfMsgHandlerCback;

  /* Configure timer. */
  prvBeaconCb.beaconTmr.msg.event = MESH_PRV_BEACON_MSG_TMR_EXPIRED;
  prvBeaconCb.beaconTmr.handlerId = meshCb.handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief     Initiates the sending of an Unprovisioned Device beacon on the specified interface
 *
 *  \param[in] brIfId          PB-ADV interface ID.
 *  \param[in] beaconInterval  Unprovisioned Device beacon interval in ms.
 *  \param[in] pUuid           16 bytes of UUID data.
 *  \param[in] oobInfoSrc      OOB information indicating the availability of OOB data.
 *  \param[in] pUriData        Uniform Resource Identifier (URI) data.
 *  \param[in] uriLen          URI data length.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshPrvBeaconStart(meshBrInterfaceId_t brIfId, uint32_t beaconInterval, const uint8_t *pUuid,
                        uint16_t oobInfoSrc, const uint8_t *pUriData,
                        uint8_t uriLen)
{
  MESH_TRACE_INFO0("MESH PROV: Send Unprovisioned Beacon");

  /* Should never happen since provisioning servers validates this. */
  WSF_ASSERT(brIfId != MESH_BR_INVALID_INTERFACE_ID);
  WSF_ASSERT(pUuid != NULL);

  /* Stop timer */
  WsfTimerStop(&(prvBeaconCb.beaconTmr));

  /* Set bearer inteface */
  prvBeaconCb.brIfId = brIfId;

  /* Set beacon period */
  prvBeaconCb.beaconInterval = beaconInterval;

  /* Populate Beacon PDU */
  prvBeaconCb.pdu[0] = MESH_BEACON_TYPE_UNPROV;
  memcpy(&prvBeaconCb.pdu[MESH_PRV_BEACON_DEVICE_UUID_OFFSET], pUuid, MESH_PRV_DEVICE_UUID_SIZE);
  UINT16_TO_BE_BUF(&prvBeaconCb.pdu[MESH_PRV_BEACON_OOB_INFO_OFFSET], oobInfoSrc);

  /* Get Beacon PDU length based on URI data presence */
  if (pUriData == NULL)
  {
    prvBeaconCb.pduLen = MESH_PRV_MAX_NO_URI_BEACON_SIZE;

    /* Send beacon to bearer */
    MeshBrSendBeaconPdu(prvBeaconCb.brIfId, prvBeaconCb.pdu, prvBeaconCb.pduLen);

    /* Start timer. */
    WsfTimerStartMs(&(prvBeaconCb.beaconTmr), prvBeaconCb.beaconInterval);
  }
  else
  {
    prvBeaconCb.pduLen = MESH_PRV_MAX_BEACON_SIZE;

    /* Allocate buffer for URI data */
    prvBeaconCb.pUriData = WsfBufAlloc(uriLen);

    if (prvBeaconCb.pUriData == NULL)
    {
      MESH_TRACE_ERR0("MESH PROV: No memory for Unprovisioned Beacon URI Hash");
      return;
    }

    /* Copy in data for SALT generation */
    memcpy(prvBeaconCb.pUriData, pUriData, uriLen);
    prvBeaconCb.uriLen = uriLen;

    /* Generate SALT for URI data */
    if (MeshSecToolGenerateSalt(prvBeaconCb.pUriData, uriLen, meshPrvSaltCback, NULL) != MESH_SUCCESS)
    {
      MESH_TRACE_ERR0("MESH PROV:  Unprovisioned Beacon URI Hash failed");

      /* Free buffer used for URI */
      WsfBufFree(prvBeaconCb.pUriData);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Stops the sending of Unprovisioned Device beacons.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshPrvBeaconStop(void)
{
  MESH_TRACE_INFO0("MESH PROV: Stop sending Unprovisioned Beacon");

  /* Stop timer if it is started. */
  WsfTimerStop(&(prvBeaconCb.beaconTmr));

  /* Reset PDU length to 0 */
  prvBeaconCb.pduLen = 0;
}

/*************************************************************************************************/
/*!
 *  \brief  Matches the UUID with the one in the Unprovisioned Device beacon.
 *
 *  \return TRUE if matches, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t MeshPrvBeaconMatch(const uint8_t *pUuid)
{
  if (pUuid != NULL)
  {
    return (memcmp(&prvBeaconCb.pdu[MESH_PRV_BEACON_DEVICE_UUID_OFFSET], pUuid,
                   MESH_PRV_DEVICE_UUID_SIZE) == 0x00);
  }

  return FALSE;
}
