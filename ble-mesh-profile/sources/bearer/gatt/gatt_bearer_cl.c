/*************************************************************************************************/
/*!
 *  \file   gatt_bearer_cl.c
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
#include "util/terminal.h"

#include "mesh_defs.h"
#include "mesh_api.h"

#include "gatt_bearer_cl.h"
#include "app_bearer.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! GATT Bearer Client control block */
typedef struct
{
  /* Scanning parameters */
  uint8_t       addrType;                 /*!< Type of address of device to connect to */
  bdAddr_t      addr;                     /*!< Address of device to connect to */
  bool_t        doConnect;                /*!< TRUE to issue connect on scan complete */

  /* GATT Connection parameters */
  dmConnId_t    connId;                   /*!< Connection IDs */
} gattBearerClCb_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Mesh Advertising Bearer control block */
static gattBearerClCb_t gattBearerClCb;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Configuration pointer for Mesh GATT Client Bearer */
gattBearerClCfg_t *pGattBearerClCfg;

/*! Configuration pointer for GATT Bearer Client Connection */
hciConnSpec_t *pGattBearerClConnCfg;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Registers GATT bearer interface ID.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void gattBearerConnect(dmEvt_t *pMsg)
{
  if (pMsg->hdr.status == HCI_SUCCESS)
  {
    /* Store connection ID. */
    gattBearerClCb.connId = (dmConnId_t) pMsg->hdr.param;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Unregisters current GATT bearer interface ID.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void gattBearerDisconnect(meshGattProxyConnId_t connId)
{
  /* Check connection ID */
  if (gattBearerClCb.connId == connId)
  {
    gattBearerClCb.connId = DM_CONN_ID_NONE;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handle a scan stop.
 *
 *  \param[in] pMsg  Pointer to DM callback event message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void scanStop(dmEvt_t *pMsg)
{
  /* Check if connection is pending */
  if (gattBearerClCb.doConnect)
  {
    /* Connect to peer. */
#if (BT_VER == 9)
    DmExtConnSetScanInterval(HCI_INIT_PHY_LE_1M_BIT, &pGattBearerClCfg->scanInterval,
                             &pGattBearerClCfg->scanWindow);
    DmExtConnSetConnSpec(HCI_INIT_PHY_LE_1M_BIT, pGattBearerClConnCfg);
#endif
#if (BT_VER == 8)
    DmConnSetConnSpec(pGattBearerClConnCfg);
#endif
    gattBearerClCb.connId = DmConnOpen(DM_CLIENT_ID_APP, HCI_INIT_PHY_LE_1M_BIT,
                                       gattBearerClCb.addrType, gattBearerClCb.addr);

    /* Reset connect flag. */
    gattBearerClCb.doConnect = FALSE;
  }

  (void)pMsg;
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes GATT Bearer for the Mesh node.
 *
 *  \return None.
 */
/*************************************************************************************************/
void GattBearerClInit(void)
{
}

/*************************************************************************************************/
/*!
 *  \brief  Schedules the scanning on the GATT Bearer for the Mesh node.
 *
 *  \return None.
 */
/*************************************************************************************************/
void GattBearerClStart(void)
{
  if (gattBearerClCb.connId == DM_CONN_ID_NONE)
  {
    /* Reset connect flag. */
    gattBearerClCb.doConnect = FALSE;

    /* Set scanning parameters and start scanning. */
    DmScanSetInterval(HCI_SCAN_PHY_LE_1M_BIT, &pGattBearerClCfg->scanInterval, &pGattBearerClCfg->scanWindow);
    DmScanStart(HCI_SCAN_PHY_LE_1M_BIT, pGattBearerClCfg->discMode, &pGattBearerClCfg->scanType, TRUE, 0, 0);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Stops the scanning on the GATT Bearer for the Mesh node.
 *
 *  \return TRUE if access to DM is unavailable, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t GattBearerClStop(void)
{
  uint8_t scanState;

  /* Get the scan state. */
  scanState = AppBearerGetScanState();

  /* Check if Scanning is started. */
  if ((scanState == SCAN_STARTED) || (scanState == SCAN_START_REQ))
  {
    /* Stop scanning. */
    DmScanStop();

    /* Update state*/
    AppBearerSetScanState(SCAN_STOP_REQ);
    return TRUE;
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Initiates a GATT connection to a Mesh node.
 *
 *  \param[in] addrType  Address Type of the peer device.
 *  \param[in] addr      Address of the peer device.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void GattBearerClConnect(uint8_t addrType, uint8_t* pAddr)
{
  uint8_t scanState;

  /* Get the scan state. */
  scanState = AppBearerGetScanState();

  /* Check if Scanning is started. */
  if ((scanState == SCAN_STARTED) || (scanState == SCAN_START_REQ))
  {
    /* Stop scanning. */
    DmScanStop();

    /* Signal connect after scan stopped. */
    gattBearerClCb.doConnect = TRUE;
    gattBearerClCb.addrType = addrType;
    memcpy(gattBearerClCb.addr, pAddr, sizeof(bdAddr_t));
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
void GattBearerClProcDmMsg(dmEvt_t *pMsg)
{
  switch (pMsg->hdr.event)
  {
    case DM_CONN_OPEN_IND:
      gattBearerConnect(pMsg);
      break;

    case DM_CONN_CLOSE_IND:
      if (pMsg->hdr.status == HCI_SUCCESS)
      {
        gattBearerDisconnect((dmConnId_t) pMsg->hdr.param);
      }
      break;

    case DM_EXT_SCAN_STOP_IND:
    case DM_SCAN_STOP_IND:
      if (pMsg->hdr.status == HCI_SUCCESS)
      {
        scanStop(pMsg);
      }
      break;

    default:
      break;
  }
}
