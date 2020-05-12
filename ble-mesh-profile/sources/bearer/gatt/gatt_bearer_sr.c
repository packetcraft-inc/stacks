/*************************************************************************************************/
/*!
 *  \file   gatt_bearer_sr.c
 *
 *  \brief  GATT Bearer Server module implementation. This module can be used with both
 *          DM legacy and extended advertising.
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
 */
/*************************************************************************************************/

#include <string.h>
#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_trace.h"
#include "dm_api.h"
#include "att_defs.h"
#include "att_api.h"
#include "svc_mprxs.h"
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_api.h"
#include "mesh_prv.h"

#include "gatt_bearer_sr.h"
#include "app_bearer.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Offset of Service Data AD inside the ADV data */
#define ADV_DATA_SVC_DATA_OFFSET    7

/*! Offset of Proxy Data inside the ADV data */
#define ADV_DATA_PROXY_DATA_OFFSET  (ADV_DATA_SVC_DATA_OFFSET + 1 + 1 + 2)

/*! ADV data length for the provisioning service */
#define PRV_ADV_DATA_LEN            29

/*! Extracts the PDU type from the first byte of the Proxy PDU */
#define EXTRACT_PDU_TYPE(byte)    (byte & 0x3F)

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh control block */
typedef struct
{
  gattBearerSrCfg_t *pCfg;                  /*!< Bearer configuration */

  /* Connectable Advertising parameters */
  uint8_t       advData[HCI_ADV_DATA_LEN];  /*!< Buffer for Advertising state machine */
  uint8_t       advDataLen;                 /*!< Length of the Advertising buffer */

  /* GATT Connection parameters */
  dmConnId_t    connId;                     /*!< Connection IDs */
} gattBearerSrCb_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Mesh Advertising Bearer control block */
static gattBearerSrCb_t gattBearerSrCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Starts advertising.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void gattBearerStartAdv(void)
{
  bdAddr_t peerAddr;
  uint8_t advHandle;
  uint8_t maxEaEvents;
  uint16_t duration;
  uint8_t advState;

  /* Get the advertising state. */
  advState = AppBearerGetAdvState();

  /* Start Advertising when adv data is available. */
  if (gattBearerSrCb.advDataLen == 0 || advState != ADV_STOPPED)
  {
    return;
  }

  /* Only 1 connection supported.*/
  advHandle = DM_ADV_HANDLE_DEFAULT;
  maxEaEvents = 0;
  duration = 0;

  /* Set advertising address. */
  memset(peerAddr, 0, BDA_ADDR_LEN);

  /* Configure advertising parameters. */
  DmAdvConfig(DM_ADV_HANDLE_DEFAULT, DM_ADV_CONN_UNDIRECT, HCI_ADDR_TYPE_PUBLIC, peerAddr);

  /* Configure advertising interval. */
  DmAdvSetInterval(DM_ADV_HANDLE_DEFAULT, gattBearerSrCb.pCfg->intervalMin,
                   gattBearerSrCb.pCfg->intervalMin);

#if (BT_VER == 9)
  /* Use Legacy PDU for GATT bearer */
  DmAdvUseLegacyPdu(DM_ADV_HANDLE_DEFAULT, TRUE);
#endif

  /* Set advertising data. */
  DmAdvSetData(DM_ADV_HANDLE_DEFAULT, HCI_ADV_DATA_OP_COMP_FRAG, DM_DATA_LOC_ADV,
               gattBearerSrCb.advDataLen, gattBearerSrCb.advData);

  DmAdvStart(1, &advHandle, &duration, &maxEaEvents);

  /* Set the advertising state. */
  AppBearerSetAdvState(ADV_START_REQ);
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes GATT Bearer for the Mesh node.
 *
 *  \param[in] pGattBearerSrCfg  Configuration pointer for GATT Bearer Server.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void GattBearerSrInit(gattBearerSrCfg_t *pGattBearerSrCfg)
{
  uint8_t *pData;

  /* Initialize control block. */
  gattBearerSrCb.advDataLen = 0;
  gattBearerSrCb.connId = DM_CONN_ID_NONE;
  gattBearerSrCb.pCfg = pGattBearerSrCfg;

  /* Initialize Connectable Advertising Data. */
  pData = gattBearerSrCb.advData;

  /* Add flags to ADV data. */
  UINT8_TO_BSTREAM(pData, sizeof(uint8_t) + 1);
  UINT8_TO_BSTREAM(pData, DM_ADV_TYPE_FLAGS);
  UINT8_TO_BSTREAM(pData, DM_FLAG_LE_GENERAL_DISC);

  /* Add service UUID list to ADV data. */
  UINT8_TO_BSTREAM(pData, sizeof(uint16_t) + 1);
  UINT8_TO_BSTREAM(pData, DM_ADV_TYPE_16_UUID);
  UINT16_TO_BSTREAM(pData, ATT_UUID_MESH_PRV_SERVICE);
}

/*************************************************************************************************/
/*!
 *  \brief     Configures the Advertising Data for the GATT Server hosting a Mesh Provisioning service.
 *
 *  \param[in] pDevUuid  Device UUID.
 *  \param[in] oobInfo   OOB information.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void GattBearerSrSetPrvSvcData(const uint8_t *pDevUuid, uint16_t oobInfo)
{
  uint8_t *pData;

  WSF_ASSERT(pDevUuid != NULL);

  pData = &gattBearerSrCb.advData[ADV_DATA_SVC_DATA_OFFSET - sizeof(uint16_t)];

  /* Update UUID in service UUID list. */
  UINT16_TO_BSTREAM(pData, ATT_UUID_MESH_PRV_SERVICE);

  /* Add Service Data to ADV data. */
  UINT8_TO_BSTREAM(pData, 1 + 2 * sizeof(uint16_t) + MESH_PRV_DEVICE_UUID_SIZE);
  UINT8_TO_BSTREAM(pData, DM_ADV_TYPE_SERVICE_DATA);
  UINT16_TO_BSTREAM(pData, ATT_UUID_MESH_PRV_SERVICE);
  memcpy(pData, pDevUuid, MESH_PRV_DEVICE_UUID_SIZE);
  pData += MESH_PRV_DEVICE_UUID_SIZE;
  UINT16_TO_BE_BSTREAM(pData, oobInfo);

  /* Set ADV data length. */
  gattBearerSrCb.advDataLen = PRV_ADV_DATA_LEN;
}

/*************************************************************************************************/
/*!
 *  \brief     Configures the Advertising Data for the GATT Server hosting a Mesh Proxy service.
 *
 *  \param[in] pSvcData    Service data.
 *  \param[in] svcDataLen  Service data length.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void GattBearerSrSetPrxSvcData(const uint8_t *pSvcData, uint8_t svcDataLen)
{
  uint8_t *pData;

  WSF_ASSERT(svcDataLen != 0);
  WSF_ASSERT(pSvcData != NULL);

  pData = &gattBearerSrCb.advData[ADV_DATA_SVC_DATA_OFFSET - sizeof(uint16_t)];

  /* Update UUID in service UUID list. */
  UINT16_TO_BSTREAM(pData, ATT_UUID_MESH_PROXY_SERVICE);

  /* Add Service Data to ADV data. */
  UINT8_TO_BSTREAM(pData, sizeof(uint16_t) + 1 + svcDataLen);
  UINT8_TO_BSTREAM(pData, DM_ADV_TYPE_SERVICE_DATA);
  UINT16_TO_BSTREAM(pData, ATT_UUID_MESH_PROXY_SERVICE);
  memcpy(pData, pSvcData, svcDataLen);

  /* Set ADV data length. */
  gattBearerSrCb.advDataLen = svcDataLen + ADV_DATA_PROXY_DATA_OFFSET;
}

/*************************************************************************************************/
/*!
 *  \brief  Start Advertising on the GATT Bearer for the Mesh node.
 *
 *  \return None.
 */
/*************************************************************************************************/
void GattBearerSrStart(void)
{
  if (gattBearerSrCb.connId == DM_CONN_ID_NONE)
  {
    /* Start advertising if no connection is up. */
    gattBearerStartAdv();
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Stops Advertising on the GATT Bearer for the Mesh node.
 *
 *  \return None.
 */
/*************************************************************************************************/
bool_t GattBearerSrStop(void)
{
  uint8_t advHandle = DM_ADV_HANDLE_DEFAULT;
  uint8_t advState;

  /* Get the advertising state. */
  advState = AppBearerGetAdvState();

  /* Check if Advertising is started. */
  if ((advState == ADV_STARTED) || (advState == ADV_START_REQ))
  {
    /* Stop advertising. */
    DmAdvStop(1, &advHandle);

    /* Update advertising state. */
    AppBearerSetAdvState(ADV_STOP_REQ);

    return TRUE;
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Process DM messages for a Mesh node.  This function should be called
 *             from the application's event handler.
 *
 *  \param[in] pMsg  Pointer to DM callback event message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void GattBearerSrProcDmMsg(dmEvt_t *pMsg)
{
  switch (pMsg->hdr.event)
  {
    case DM_CONN_OPEN_IND:
      if (pMsg->hdr.status == HCI_SUCCESS)
      {
        /* Store connection ID. */
        gattBearerSrCb.connId = (dmConnId_t) pMsg->hdr.param;
      }
      break;

    case DM_CONN_CLOSE_IND:
      if (pMsg->hdr.status == HCI_SUCCESS)
      {
        /* Reset connection ID. */
        gattBearerSrCb.connId = DM_CONN_ID_NONE;
      }
      break;

    default:
      break;
  }
}
