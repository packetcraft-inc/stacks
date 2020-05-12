/*************************************************************************************************/
/*!
 *  \file   adv_bearer.c
 *
 *  \brief  Advertising Bearer Mesh module implementation. This module can be used with both
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

#include "mesh_defs.h"
#include "mesh_api.h"

#include "adv_bearer.h"
#include "app_bearer.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Offset of the AD data inside the advertising packet*/
#define AD_DATA_PDU_OFFSET  2

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Advertising Bearer control block */
typedef struct
{
  advBearerCfg_t* pConfig;                    /*!< Configuration of the Advertising Bearer */
  meshAdvIfId_t   ifId;                       /*!< Advertising Interface ID */
  uint8_t         advBuff[HCI_ADV_DATA_LEN];  /*!< Buffer for Advertising state machine */
  uint8_t         advBuffLen;                 /*!< Length of the Advertising buffer */
} advBearerCb_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Mesh Advertising Bearer control block */
static advBearerCb_t advBearerCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Starts scanning on the Advertising Bearer.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void advBearerStartScanning(void)
{
  uint8_t scanState;

  /* Get the scan state. */
  scanState = AppBearerGetScanState();

  /* Check if bearer interface ID and scanning state are valid. */
  if ((advBearerCb.ifId != ADV_BEARER_INVALID_IF_ID) &&
      (scanState == SCAN_STOPPED))
  {
    /* Set scan interval. */
    DmScanSetInterval(HCI_SCAN_PHY_LE_1M_BIT, &advBearerCb.pConfig->scanInterval,
                      &advBearerCb.pConfig->scanWindow);
    /* Start scan. */
    DmScanStart(HCI_SCAN_PHY_LE_1M_BIT, advBearerCb.pConfig->discMode,
                &advBearerCb.pConfig->scanType, FALSE, 0, 0);
    /* Set state to scan start request. */
    AppBearerSetScanState(SCAN_START_REQ);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Stops scanning on the Advertising Bearer.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void advBearerStopScanning(void)
{
  uint8_t scanState;

  /* Get the scan state. */
  scanState = AppBearerGetScanState();

  /* Check if Scanning is started. */
  if ((scanState == SCAN_STARTED) || (scanState == SCAN_START_REQ))
  {
    /* Stop scan. */
    DmScanStop();
    /* Set state to scan stop request. */
    AppBearerSetScanState(SCAN_STOP_REQ);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Starts advertising on the Advertising Bearer.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void advBearerStartAdvertising(void)
{
  uint8_t advHandle;
  uint8_t maxEaEvents;

  advHandle = DM_ADV_HANDLE_DEFAULT;
  maxEaEvents = 1;

  /* Set advertising data. */
  DmAdvSetData(DM_ADV_HANDLE_DEFAULT, HCI_ADV_DATA_OP_COMP_FRAG, DM_DATA_LOC_ADV,
               advBearerCb.advBuffLen, advBearerCb.advBuff);

  /* Start advertising. */
  DmAdvStart(1, &advHandle, &advBearerCb.pConfig->advDuration, &maxEaEvents);

  AppBearerSetAdvState(ADV_START_REQ);

  /* Clear advBuffLen to signal that the data has been set. */
  advBearerCb.advBuffLen = 0;
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initialize Advertising Bearer for the Mesh node.
 *
 *  \param[in] pAdvBearerCfg  Configuration pointer for Mesh Advertising Bearer.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void AdvBearerInit(advBearerCfg_t *pAdvBearerCfg)
{
  /* Initialize control block. */
  advBearerCb.advBuffLen = 0;
  advBearerCb.ifId = ADV_BEARER_INVALID_IF_ID;
  advBearerCb.pConfig = pAdvBearerCfg;
}

/*************************************************************************************************/
/*!
 *  \brief  Register Advertising Bearer for the Mesh node.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AdvBearerRegisterIf(meshAdvIfId_t advIfId)
{
  WSF_ASSERT(advBearerCb.ifId == ADV_BEARER_INVALID_IF_ID);

  /* Set bearer advertising interface ID. */
  advBearerCb.ifId = advIfId;
}

/*************************************************************************************************/
/*!
 *  \brief  Deregister Advertising Bearer for the Mesh node.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AdvBearerDeregisterIf(void)
{
  /* Set bearer advertising interface ID. */
  advBearerCb.ifId = ADV_BEARER_INVALID_IF_ID;
}

/*************************************************************************************************/
/*!
 *  \brief  Starts advertising bearer on the specified interface ID.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AdvBearerStart(void)
{
  bdAddr_t peerAddr;

  /* Set advertising address. */
  memset(peerAddr, 0, BDA_ADDR_LEN);

  /* Configure advertising interval. */
  DmAdvSetInterval(DM_ADV_HANDLE_DEFAULT, advBearerCb.pConfig->intervalMin,
                   advBearerCb.pConfig->intervalMax);

  /* Configure advertising parameters. */
  DmAdvConfig(DM_ADV_HANDLE_DEFAULT, DM_ADV_NONCONN_UNDIRECT, HCI_ADDR_TYPE_PUBLIC, peerAddr);

  /* Start scanning. */
  advBearerStartScanning();
}

/*************************************************************************************************/
/*!
 *  \brief  Stops current advertising bearer interface ID.
 *
 *  \return Always \ref TRUE.
 */
/*************************************************************************************************/
bool_t AdvBearerStop(void)
{
  uint8_t advHandle;
  uint8_t advState;
  uint8_t scanState;

  /* Get scanning state. */
  scanState = AppBearerGetScanState();

  /* Get advertising state. */
  advState = AppBearerGetAdvState();

  /* Check if Advertising is started. */
  if ((advState == ADV_STARTED) || (advState == ADV_START_REQ))
  {
    /* Stop advertising. */
    DmAdvStop(1, &advHandle);

    AppBearerSetAdvState(ADV_STOP_REQ);
  }
  else
  {
    if ((scanState == SCAN_STARTED) || (scanState == SCAN_START_REQ))
      {
        /* Stop scan. */
        DmScanStop();
        /* Set state to scan stop request. */
        AppBearerSetScanState(SCAN_STOP_REQ);
      }
  }

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief  Send Mesh message on the Advertising Bearer.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AdvBearerSendPacket(meshAdvPduSendEvt_t *pEvt)
{
  uint8_t advState;
  uint8_t scanState;

  /* Get scanning state. */
  scanState = AppBearerGetScanState();

  /* Get advertising state. */
  advState = AppBearerGetAdvState();

  /* Advertising must be stopped. Legacy advertising data length cannot exceed 31 bytes. */
  if ((advState == ADV_STOPPED) && ((pEvt->advPduLen + AD_DATA_PDU_OFFSET) <= HCI_ADV_DATA_LEN))
  {
    /* Store packet in ADV bearer buffer. */
    advBearerCb.advBuff[0] = pEvt->advPduLen + sizeof(pEvt->adType);
    advBearerCb.advBuff[1] = pEvt->adType;
    memcpy((advBearerCb.advBuff + AD_DATA_PDU_OFFSET), pEvt->pAdvPdu, pEvt->advPduLen);

    /* Update buffer length. */
    advBearerCb.advBuffLen = pEvt->advPduLen + AD_DATA_PDU_OFFSET;

    /* Check if scanning is enabled. */
    if (scanState != SCAN_STOPPED)
    {
      /* Stop scanning. */
      advBearerStopScanning();
    }
    else
    {
      /* Start advertising */
      advBearerStartAdvertising();
    }
  }
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
void AdvBearerProcDmMsg(dmEvt_t *pMsg)
{
  uint8_t *pData;
  uint8_t advState;

  /* Get advertising state. */
  advState = AppBearerGetAdvState();

  switch (pMsg->hdr.event)
  {
    case DM_ADV_START_IND:
    case DM_ADV_SET_START_IND:
      if (pMsg->hdr.status != HCI_SUCCESS)
      {
        /* Advertising start failed. Revert to scanning. */
        advBearerStartScanning();
      }
      break;

    case DM_ADV_STOP_IND:
    case DM_ADV_SET_STOP_IND:
      WSF_ASSERT((pMsg->hdr.status == HCI_SUCCESS) ||
                 (pMsg->hdr.status == HCI_ERR_LIMIT_REACHED) ||
                 (pMsg->hdr.status == HCI_ERR_ADV_TIMEOUT));

      /* Start scanning. */
      advBearerStartScanning();
      break;

    case DM_SCAN_START_IND:
    case DM_EXT_SCAN_START_IND:
      WSF_ASSERT(pMsg->hdr.status == HCI_SUCCESS);

      /* Signal interface ready. */
      MeshSignalAdvIfRdy(advBearerCb.ifId);
      break;
    case DM_SCAN_STOP_IND:
    case DM_EXT_SCAN_STOP_IND:
      WSF_ASSERT(pMsg->hdr.status == HCI_SUCCESS);

      if ((advBearerCb.advBuffLen != 0) && (advState == ADV_STOPPED))
      {
        /* Start advertising */
        advBearerStartAdvertising();
      }
      else
      {
        /* Restart scanning. */
        advBearerStartScanning();
      }
      break;

    case DM_SCAN_REPORT_IND:
      /* Find vendor-specific advertising data. */
      if ((((pData = DmFindAdType(MESH_AD_TYPE_PACKET, pMsg->scanReport.len, pMsg->scanReport.pData)) != NULL) ||
           ((pData = DmFindAdType(MESH_AD_TYPE_BEACON, pMsg->scanReport.len, pMsg->scanReport.pData)) != NULL) ||
           ((pData = DmFindAdType(MESH_AD_TYPE_PB, pMsg->scanReport.len, pMsg->scanReport.pData)) != NULL)) &&
          (pMsg->scanReport.eventType == 0x03))
      {
        if (advBearerCb.ifId != ADV_BEARER_INVALID_IF_ID)
        {
          MeshProcessAdvPdu(advBearerCb.ifId, pData, pData[0] + 1);
        }
      }
      break;

    case DM_EXT_SCAN_REPORT_IND:
      /* Find vendor-specific advertising data. */
      if ((((pData = DmFindAdType(MESH_AD_TYPE_PACKET, pMsg->extScanReport.len, pMsg->extScanReport.pData)) != NULL) ||
           ((pData = DmFindAdType(MESH_AD_TYPE_BEACON, pMsg->extScanReport.len, pMsg->extScanReport.pData)) != NULL) ||
           ((pData = DmFindAdType(MESH_AD_TYPE_PB, pMsg->extScanReport.len, pMsg->extScanReport.pData)) != NULL)) &&
          (pMsg->extScanReport.eventType == 0x10))
      {
        if (advBearerCb.ifId != ADV_BEARER_INVALID_IF_ID)
        {
          MeshProcessAdvPdu(advBearerCb.ifId, pData, pData[0] + 1);
        }
      }
      break;

    case DM_RESET_CMPL_IND:
      if (advBearerCb.ifId != ADV_BEARER_INVALID_IF_ID)
      {
        advBearerCb.advBuffLen = 0;
      }
      break;

    default:
      break;
  }
}
