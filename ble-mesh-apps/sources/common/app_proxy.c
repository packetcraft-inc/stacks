/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief Application proxy module
 *
 *  Copyright (c) 2018 Arm Ltd. All Rights Reserved.
 *
 *  $$LICENSE$$
 */
/*************************************************************************************************/

#include <string.h>

#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_trace.h"
#include "wsf_timer.h"

#include "util/bstream.h"

#include "att_uuid.h"

#include "mesh_types.h"

#include "gatt_bearer_cl.h"
#include "app_mesh_api.h"

extern gattBearerClCfg_t *pGattBearerClCfg;


/*************************************************************************************************/
/*!
 *  \brief     Checks if the Service UUID is advertised
 *
 *  \param[in] pMsg  Pointer to DM callback event message.
 *
 *  \return    \ref TRUE if a matching service UUID was found.
 */
/*************************************************************************************************/
static bool_t appProxyCheckServiceUuid(dmEvt_t *pMsg)
{
  uint8_t *pData = NULL;
  bool_t serviceFound = FALSE;

  switch(pMsg->hdr.event)
  {
    case DM_EXT_SCAN_REPORT_IND:
      /* Find Service UUID list; if full list not found search for partial */
      if ((pData = DmFindAdType(DM_ADV_TYPE_16_UUID, pMsg->extScanReport.len,
                                pMsg->extScanReport.pData)) == NULL)
      {
        pData = DmFindAdType(DM_ADV_TYPE_16_UUID_PART, pMsg->extScanReport.len,
                             pMsg->extScanReport.pData);
      }
      break;
    case DM_SCAN_REPORT_IND:
      /* Find Service UUID list; if full list not found search for partial */
      if ((pData = DmFindAdType(DM_ADV_TYPE_16_UUID, pMsg->scanReport.len,
                                pMsg->scanReport.pData)) == NULL)
      {
        pData = DmFindAdType(DM_ADV_TYPE_16_UUID_PART, pMsg->scanReport.len,
                             pMsg->scanReport.pData);
      }
      break;

    default:
      break;
  }

  /* if found and length checks out ok */
  if (pData != NULL && pData[DM_AD_LEN_IDX] >= (ATT_16_UUID_LEN + 1))
  {
    uint8_t len = pData[DM_AD_LEN_IDX] - 1;
    pData += DM_AD_DATA_IDX;

    while ((!serviceFound) && (len >= ATT_16_UUID_LEN))
    {
      /* Connect if desired service is included */
      if (BYTES_UINT16_CMP(pData, pGattBearerClCfg->serviceUuid))
      {
        serviceFound = TRUE;
        break;
      }

      pData += ATT_16_UUID_LEN;
      len -= ATT_16_UUID_LEN;
    }
  }

  return serviceFound;
}

/*************************************************************************************************/
/*!
 *  \brief     Handle a scan report.
 *
 *  \param[in] pMsg  Pointer to DM callback event message.
 *  \param[in] pDevUuid Pointer to device UUID to search for.
 *
 *  \return   \ref TRUE if connection request was sent, else \ref FALSE.
 */
/*************************************************************************************************/
bool_t AppProxyScanReport(dmEvt_t *pMsg, uint8_t *pDevUuid)
{
  uint8_t *pData;
  uint8_t *pAddr;
  bool_t dataMatches = FALSE;
  uint8_t *pUuid = NULL;
  uint8_t serviceDataLen;
  uint8_t addrType = 0;

  /* Service is not found. Do not continue processing. */
  if (!appProxyCheckServiceUuid(pMsg))
  {
    return FALSE;
  }

  switch(pMsg->hdr.event)
  {
    case DM_EXT_SCAN_REPORT_IND:
      /* Service has been found. Look for service data. */
      pData = DmFindAdType(DM_ADV_TYPE_SERVICE_DATA, pMsg->extScanReport.len, pMsg->extScanReport.pData);
      pAddr = pMsg->extScanReport.addr;
      addrType = pMsg->extScanReport.addrType;
      break;

    case DM_SCAN_REPORT_IND:
      /* Service has been found. Look for service data. */
      pData = DmFindAdType(DM_ADV_TYPE_SERVICE_DATA, pMsg->scanReport.len, pMsg->scanReport.pData);
      pAddr = pMsg->scanReport.addr;
      addrType = pMsg->scanReport.addrType;
      break;

    default:
      pData = NULL;
      pAddr = NULL;
      addrType = 0;
      break;
  }

  if ((pData != NULL) && pData[DM_AD_LEN_IDX] >= (ATT_16_UUID_LEN + 1))
  {
    serviceDataLen = pData[DM_AD_LEN_IDX] - ATT_16_UUID_LEN - 1;
    pData += DM_AD_DATA_IDX;

    /* Match service UUID in service data. */
    if (!BYTES_UINT16_CMP(pData, pGattBearerClCfg->serviceUuid))
    {
      return FALSE;
    }
    else
    {
      if (pGattBearerClCfg->serviceUuid == ATT_UUID_MESH_PRV_SERVICE &&
          (serviceDataLen == (MESH_PRV_DEVICE_UUID_SIZE + sizeof(meshPrvOobInfoSource_t))) &&
          (pDevUuid != NULL))
      {
        pData += ATT_16_UUID_LEN;
        pUuid = pData;
        pData += MESH_PRV_DEVICE_UUID_SIZE;

        dataMatches = (memcmp(pUuid, pDevUuid,
                              MESH_PRV_DEVICE_UUID_SIZE) == 0);
      }
      else if (pGattBearerClCfg->serviceUuid == ATT_UUID_MESH_PROXY_SERVICE)
      {
        /* Connect to anyone */
        dataMatches = TRUE;
      }
    }
  }

  /* Found match in scan report */
  if (dataMatches)
  {
    /* Initiate connection */
    GattBearerClConnect(addrType, pAddr);

    return TRUE;
  }

  return FALSE;
}
