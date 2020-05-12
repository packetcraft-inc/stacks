/*************************************************************************************************/
/*!
 *  \file   light_main.c
 *
 *  \brief  Light application.
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
 */
/*************************************************************************************************/

#include <stdio.h>
#include <string.h>
#include "wsf_types.h"
#include "wsf_timer.h"
#include "wsf_trace.h"
#include "wsf_msg.h"
#include "wsf_nvm.h"
#include "wsf_buf.h"
#include "dm_api.h"
#include "util/bstream.h"

#include "att_api.h"
#include "app_api.h"
#include "app_cfg.h"
#include "svc_mprxs.h"
#include "svc_mprvs.h"
#include "mprxs/mprxs_api.h"
#include "mprvs/mprvs_api.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_error_codes.h"
#include "mesh_api.h"

#include "adv_bearer.h"
#include "gatt_bearer_sr.h"

#include "mesh_prv.h"
#include "mesh_prv_sr_api.h"
#include "mesh_cfg_mdl_api.h"
#include "mesh_cfg_mdl_sr_api.h"
#include "mesh_cfg_mdl_cl_api.h"
#include "mesh_friend_api.h"
#include "mesh_replay_protection.h"
#include "mesh_local_config.h"

#include "mmdl_types.h"
#include "mmdl_defs.h"
#include "mesh_ht_sr_api.h"
#include "mmdl_bindings_api.h"
#include "mmdl_gen_onoff_sr_api.h"
#include "mmdl_gen_level_sr_api.h"
#include "mmdl_gen_powonoff_sr_api.h"
#include "mmdl_gen_powonoffsetup_sr_api.h"
#include "mmdl_scene_sr_api.h"
#include "mmdl_gen_default_trans_sr_api.h"
#include "mmdl_lightlightness_sr_api.h"
#include "mmdl_lightlightnesssetup_sr_api.h"
#include "mmdl_light_hsl_sr_api.h"
#include "mmdl_light_hsl_hue_sr_api.h"
#include "mmdl_light_hsl_sat_sr_api.h"

#include "app_mesh_api.h"
#include "app_bearer.h"

#include "light_api.h"
#include "light_config.h"
#include "light_version.h"

#include "pal_btn.h"
#include "pal_led.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Health Server company ID registered in the instance */
#define LIGHT_HT_SR_COMPANY_ID              0xFFFF

/*! Health Server test ID for the associated to the test company ID */
#define LIGHT_HT_SR_TEST_ID                 0x00

/*! Friend receive window in milliseconds */
#define LIGHT_FRIEND_RECEIVE_WINDOW         100

#define LIGHT_NEWLINE
#define LIGHT_PRINT0(msg)                   APP_TRACE_INFO0(msg)
#define LIGHT_PRINT1(msg, var1)             APP_TRACE_INFO1(msg, var1)
#define LIGHT_PRINT2(msg, var1, var2)       APP_TRACE_INFO2(msg, var1, var2)
#define LIGHT_PRINT3(msg, var1, var2, var3) APP_TRACE_INFO3(msg, var1, var2, var3)
#define LIGHT_PRINT4(msg, var1, var2, var3, var4) \
                                            APP_TRACE_INFO4(msg, var1, var2, var3, var4)
#define LIGHT_PRINT5(msg, var1, var2, var3, var4, var5) \
                                            APP_TRACE_INFO5(msg, var1, var2, var3, var4, var5)
#define LIGHT_PRINT6(msg, var1, var2, var3, var4, var5, var6) \
                                            APP_TRACE_INFO6(msg, var1, var2, var3, var4, var5, var6)

/* Button identifiers */
enum
{
  LIGHT_BUTTON_1,
  LIGHT_BUTTON_MAX
};

/* Events */
#define LIGHT_BUTTON_EVENT                  1

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Enumeration of client characteristic configuration descriptors */
enum
{
  LIGHT_DOUT_CCC_IDX,      /*! Mesh Proxy/Provisioning service, Data Out */
  LIGHT_NUM_CCC_IDX
};

/*! Light application control block structure */
typedef struct lightCb_tag
{
  wsfTimer_t  nodeIdentityTmr;      /*!< WSF Timer for Node Identity timeout */
  uint16_t    netKeyIndexAdv;       /*!< Net Key Index used for GATT advertising */
  bool_t      nodeIdentityRunning;  /*!< TRUE if Node Identity is started
                                     *   FALSE otherwise */
  bool_t      proxyFeatEnabled;     /*!< TRUE if GATT Proxy Server is enabled
                                     *   FALSE otherwise */
  bool_t      prvSrStarted;         /*!< TRUE if Provisioning Server is started
                                     *   FALSE otherwise */
  uint8_t     newBtnStates;         /*!< bitmask of changed button states */
} lightCb_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! \brief  Client Characteristic Configuration Descriptors */

/*! Client characteristic configuration descriptors settings, indexed by above enumeration */
static const attsCccSet_t lightPrvCccSet[LIGHT_NUM_CCC_IDX] =
{
  {
    MPRVS_DOUT_CH_CCC_HDL,       /*!< CCCD handle */
    ATT_CLIENT_CFG_NOTIFY,       /*!< Value range */
    DM_SEC_LEVEL_NONE            /*!< Security level */
  },
};

/*! Client characteristic configuration descriptors settings, indexed by above enumeration */
static const attsCccSet_t lightPrxCccSet[LIGHT_NUM_CCC_IDX] =
{
  {
    MPRXS_DOUT_CH_CCC_HDL,       /*!< CCCD handle */
    ATT_CLIENT_CFG_NOTIFY,       /*!< Value range */
    DM_SEC_LEVEL_NONE            /*!< Security level */
  },
};

/*! WSF handler ID */
static wsfHandlerId_t lightHandlerId;

/*! Light application control block structure */
static lightCb_t lightCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Platform button callback.
 *
 *  \param[in] btnId  button ID.
 *  \param[in] state  button state. See ::PalBtnPos_t.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightBtnCback(uint8_t btnId, PalBtnPos_t state)
{
  /* Only alert application of button press and not release. */
  if ((btnId < LIGHT_BUTTON_MAX) && (state == PAL_BTN_POS_DOWN))
  {
    lightCb.newBtnStates |= 1 << btnId;

    WsfSetEvent(lightHandlerId, LIGHT_BUTTON_EVENT);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Application DM callback.
 *
 *  \param[in] pDmEvt  DM callback event
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightDmCback(dmEvt_t *pDmEvt)
{
  dmEvt_t *pMsg;
  uint16_t len;

  len = DmSizeOfEvt(pDmEvt);

  if ((pMsg = WsfMsgAlloc(len)) != NULL)
  {
    memcpy(pMsg, pDmEvt, len);
    WsfMsgSend(lightHandlerId, pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Application ATTS client characteristic configuration callback.
 *
 *  \param[in] pDmEvt  DM callback event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightCccCback(attsCccEvt_t *pEvt)
{
  attsCccEvt_t  *pMsg;
  appDbHdl_t    dbHdl;

  /* If CCC not set from initialization and there's a device record. */
  if ((pEvt->handle != ATT_HANDLE_NONE) &&
      ((dbHdl = AppDbGetHdl((dmConnId_t)pEvt->hdr.param)) != APP_DB_HDL_NONE))
  {
    /* Store value in device database. */
    AppDbSetCccTblValue(dbHdl, pEvt->idx, pEvt->value);
  }

  if ((pMsg = WsfMsgAlloc(sizeof(attsCccEvt_t))) != NULL)
  {
    memcpy(pMsg, pEvt, sizeof(attsCccEvt_t));
    WsfMsgSend(lightHandlerId, pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Application Mesh Stack callback.
 *
 *  \param[in] pEvt  Mesh Stack callback event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightMeshCback(meshEvt_t *pEvt)
{
  meshEvt_t *pMsg;
  uint16_t len;

  len = MeshSizeOfEvt(pEvt);

  if ((pMsg = WsfMsgAlloc(len)) != NULL)
  {
    memcpy(pMsg, pEvt, len);
    WsfMsgSend(lightHandlerId, pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Application Mesh Provisioning Server callback.
 *
 *  \param[in] pEvt  Mesh Provisioning Server callback event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightMeshPrvSrCback(meshPrvSrEvt_t *pEvt)
{
  meshPrvSrEvt_t *pMsg;
  uint16_t len;

  len = MeshPrvSrSizeOfEvt(pEvt);

  if ((pMsg = WsfMsgAlloc(len)) != NULL)
  {
    memcpy(pMsg, pEvt, len);
    WsfMsgSend(lightHandlerId, pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Application Mesh Configuration Server callback.
 *
 *  \param[in] pEvt  Mesh Provisioning Server callback event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightMeshCfgMdlSrCback(const meshCfgMdlSrEvt_t* pEvt)
{
  meshCfgMdlSrEvt_t *pMsg;
  uint16_t len;

  len = MeshCfgSizeOfEvt((wsfMsgHdr_t *) pEvt);

  if ((pMsg = WsfMsgAlloc(len)) != NULL)
  {
    if (MeshCfgMsgDeepCopy((wsfMsgHdr_t *) pMsg, (wsfMsgHdr_t *) pEvt))
    {
      WsfMsgSend(lightHandlerId, pMsg);
    }
    else
    {
      WsfMsgFree(pMsg);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Application ATT callback.
 *
 *  \param[in] pEvt  ATT callback event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightAttCback(attEvt_t *pEvt)
{
  attEvt_t *pMsg;

  if ((pMsg = WsfMsgAlloc(sizeof(attEvt_t) + pEvt->valueLen)) != NULL)
  {
    memcpy(pMsg, pEvt, sizeof(attEvt_t));
    pMsg->pValue = (uint8_t *) (pMsg + 1);
    memcpy(pMsg->pValue, pEvt->pValue, pEvt->valueLen);
    WsfMsgSend(lightHandlerId, pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Application bearer callback that signals the scheduled slot that has run.
 *
 *  \param[in] slot  Bearer slot. See ::bearerSlots.
 *
 *  \retval    None.
 */
/*************************************************************************************************/
static void lightBearerCback(uint8_t slot)
{
  meshProxyIdType_t idType;

  /* Switch ADV data on Proxy and Node Identity*/
  if ((slot == BR_GATT_SLOT) && MeshIsProvisioned())
  {
    idType = (lightCb.nodeIdentityRunning) ? MESH_PROXY_NODE_IDENTITY_TYPE : MESH_PROXY_NWK_ID_TYPE;

    if (lightCb.netKeyIndexAdv == 0xFFFF)
    {
      /* No specific netKey is used for advertising. Cycle through all.*/
      MeshProxySrGetNextServiceData(idType);
    }
    else
    {
      /* Advertise only on the specified netKey .*/
      MeshProxySrGetServiceData(lightCb.netKeyIndexAdv, idType);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh Provisioning Server messages from the event handler.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightProcMeshPrvSrMsg(const meshPrvSrEvt_t *pMsg)
{
  meshPrvData_t prvData;

  switch (pMsg->hdr.param)
  {
    case MESH_PRV_SR_PROVISIONING_COMPLETE_EVENT:
      /* Store Provisioning NetKey index. */
      lightCb.netKeyIndexAdv = ((meshPrvSrEvtPrvComplete_t *)pMsg)->netKeyIndex;

      prvData.pDevKey = ((meshPrvSrEvtPrvComplete_t *)pMsg)->devKey;
      prvData.pNetKey = ((meshPrvSrEvtPrvComplete_t *)pMsg)->netKey;
      prvData.primaryElementAddr = ((meshPrvSrEvtPrvComplete_t *)pMsg)->address;
      prvData.ivIndex = ((meshPrvSrEvtPrvComplete_t *)pMsg)->ivIndex;
      prvData.netKeyIndex = ((meshPrvSrEvtPrvComplete_t *)pMsg)->netKeyIndex;
      prvData.flags = ((meshPrvSrEvtPrvComplete_t *)pMsg)->flags;

      /* Load provisioning data. */
      MeshLoadPrvData(&prvData);

      /* Start Node. */
      MeshStartNode();

      LIGHT_PRINT1("prvsr_ind prv_complete elemaddr=0x%x" LIGHT_NEWLINE,
                   prvData.primaryElementAddr);
      break;

    case MESH_PRV_SR_PROVISIONING_FAILED_EVENT:
      LIGHT_PRINT1("prvsr_ind prv_failed reason=0x%x" LIGHT_NEWLINE,
                   pMsg->prvFailed.reason);

      /* Re-enter provisioning mode. */
      if (pMeshPrvSrCfg->pbAdvRestart)
      {
        MeshPrvSrEnterPbAdvProvisioningMode(pMeshPrvSrCfg->pbAdvIfId, pMeshPrvSrCfg->pbAdvInterval);
        LIGHT_PRINT0("prvsr_ind prv_restarted" LIGHT_NEWLINE);
      }
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Notification callback triggered after a Configuration Client modifies a local state.
 *
 *  \param[in] pEvt  Pointer to the event structure.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightProcMeshCfgMdlSrMsg(const meshCfgMdlSrEvt_t* pEvt)
{
  switch (pEvt->hdr.param)
  {
    case MESH_CFG_MDL_GATT_PROXY_SET_EVENT:

      /* Stop Node Identity timer. */
      WsfTimerStop(&lightCb.nodeIdentityTmr);

      if (pEvt->gattProxy.gattProxy == MESH_GATT_PROXY_FEATURE_ENABLED)
      {
        MeshProxySrGetNextServiceData(MESH_PROXY_NWK_ID_TYPE);
        lightCb.netKeyIndexAdv = 0xFFFF;
        lightCb.proxyFeatEnabled = TRUE;
        /* Stop Node Identity ADV. */
        lightCb.nodeIdentityRunning = FALSE;

        /* Enable bearer slot */
        AppBearerEnableSlot(BR_GATT_SLOT);
      }
      else if (pEvt->gattProxy.gattProxy == MESH_GATT_PROXY_FEATURE_DISABLED)
      {
        lightCb.proxyFeatEnabled = FALSE;

        /* Disable bearer slot */
        AppBearerDisableSlot(BR_GATT_SLOT);
      }
      break;

    case MESH_CFG_MDL_NODE_IDENTITY_SET_EVENT:

      if (pEvt->nodeIdentity.state == MESH_NODE_IDENTITY_RUNNING)
      {
        /* Get Service Data for the specified netkey index. */
        MeshProxySrGetServiceData(pEvt->nodeIdentity.netKeyIndex, MESH_PROXY_NODE_IDENTITY_TYPE);
        lightCb.netKeyIndexAdv = pEvt->nodeIdentity.netKeyIndex;
        lightCb.nodeIdentityRunning = TRUE;

        /* Start Node Identity timer. */
        WsfTimerStartMs(&lightCb.nodeIdentityTmr, APP_MESH_NODE_IDENTITY_TIMEOUT_MS);

        /* Enable bearer slot */
        AppBearerEnableSlot(BR_GATT_SLOT);
      }
      else if (pEvt->nodeIdentity.state == MESH_NODE_IDENTITY_STOPPED)
      {
        /* Stop Node Identity timer. */
        WsfTimerStop(&lightCb.nodeIdentityTmr);

        /* Node Identity stopped. */
        MeshProxySrGetNextServiceData(MESH_PROXY_NWK_ID_TYPE);
        lightCb.netKeyIndexAdv = 0xFFFF;
        lightCb.nodeIdentityRunning = FALSE;

        /* Check if Proxy is started. */
        if (!lightCb.proxyFeatEnabled)
        {
          /* Disable bearer slot. */
          AppBearerDisableSlot(BR_GATT_SLOT);
        }
      }
      break;

    case MESH_CFG_MDL_NODE_RESET_EVENT:
      /* Clear NVM. */
      MeshLocalCfgEraseNvm();
      MeshRpNvmErase();
      LightConfigErase();

      /* Reset system. */
      AppMeshReset();
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process messages from the event handler.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightProcMeshCoreMsg(meshEvt_t *pMsg)
{
  switch (pMsg->hdr.param)
  {
    case MESH_CORE_ADV_IF_ADD_EVENT:
      if (pMsg->hdr.status == MESH_SUCCESS)
      {
        /* Register advertising interface into bearer. */
        AdvBearerRegisterIf(((meshAdvIfEvt_t *)pMsg)->ifId);

        /* Schedule and enable ADV bearer. */
        AppBearerScheduleSlot(BR_ADV_SLOT, AdvBearerStart, AdvBearerStop, AdvBearerProcDmMsg, 5000);
        AppBearerEnableSlot(BR_ADV_SLOT);

        APP_TRACE_INFO0("LIGHT: Interface added");
      }
      else
      {
        APP_TRACE_ERR1("LIGHT: Interface add error, %d", pMsg->hdr.status);
      }
      break;

    case MESH_CORE_ADV_IF_REMOVE_EVENT:
      if (pMsg->hdr.status == MESH_SUCCESS)
      {
        /* Unregister advertising interface from bearer. */
        AdvBearerDeregisterIf();

        /* Disable ADV bearer scheduling. */
        AppBearerDisableSlot(BR_ADV_SLOT);

        APP_TRACE_INFO0("LIGHT: Interface removed");
      }
      else
      {
        APP_TRACE_ERR1("LIGHT: Interface remove error, %d", pMsg->hdr.status);
      }
      break;

    case MESH_CORE_ADV_IF_CLOSE_EVENT:
      if (pMsg->hdr.status == MESH_SUCCESS)
      {
        APP_TRACE_INFO0("LIGHT: Interface closed");
      }
      else
      {
        APP_TRACE_ERR1("LIGHT: Interface close error, %d", pMsg->hdr.status);
      }
      break;

    case MESH_CORE_GATT_CONN_ADD_EVENT:
      if (pMsg->hdr.status == MESH_SUCCESS)
      {
         APP_TRACE_INFO0("LIGHT: GATT Interface added");

         LIGHT_PRINT1("gatt_ind added connid=%d" LIGHT_NEWLINE, pMsg->gattConn.connId);

         /* Check if provisioned. */
         if (!MeshIsProvisioned())
         {
           /* Start Provisioning over PB-GATT. */
           MeshPrvSrEnterPbGattProvisioningMode(pMsg->gattConn.connId);
         }
         else if (lightCb.nodeIdentityRunning)
         {
           /* Stop Node Identity timer. */
           WsfTimerStop(&lightCb.nodeIdentityTmr);

           /* Stop Node Identity ADV. */
           lightCb.nodeIdentityRunning = FALSE;
         }
      }
      else
      {
        APP_TRACE_ERR1("LIGHT: GATT Interface add error, %d", pMsg->hdr.status);
      }
      break;

    case MESH_CORE_GATT_CONN_CLOSE_EVENT:
      if (pMsg->hdr.status == MESH_SUCCESS)
      {
        LIGHT_PRINT1("gatt_ind close connid=%d" LIGHT_NEWLINE, pMsg->gattConn.connId);
        /* Disconnect from peer. */
        AppConnClose(pMsg->gattConn.connId);
      }
    break;

    case MESH_CORE_GATT_CONN_REMOVE_EVENT:
      if (pMsg->hdr.status == MESH_SUCCESS)
      {
        LIGHT_PRINT1("gatt_ind removed connid=%d" LIGHT_NEWLINE, pMsg->gattConn.connId);

        if (lightCb.prvSrStarted && MeshIsProvisioned())
        {
          /* We are provisioned. Remove the Mesh Provisioning Service. */
          SvcMprvsRemoveGroup();

          lightCb.prvSrStarted = FALSE;

          /* Register the Mesh Proxy Service. */
          SvcMprxsRegister(MprxsWriteCback);

          /* Add the Mesh Proxy Service. */
          SvcMprxsAddGroup();

          /* Register Mesh Proxy Service CCC*/
          AttsCccRegister(LIGHT_NUM_CCC_IDX, (attsCccSet_t *) lightPrxCccSet, lightCccCback);

          /* Configure GATT server for Mesh Proxy. */
          MprxsSetCccIdx(LIGHT_DOUT_CCC_IDX);

          /* Register GATT Bearer callback */
          MeshRegisterGattProxyPduSendCback(MprxsSendDataOut);

          /* Start advertising with node identity on the primary subnet. */
          MeshProxySrGetServiceData(lightCb.netKeyIndexAdv, MESH_PROXY_NODE_IDENTITY_TYPE);

          lightCb.nodeIdentityRunning = TRUE;
        }
      }
      else
      {
        APP_TRACE_ERR1("LIGHT: GATT Interface close/remove error, %d", pMsg->hdr.status);
      }
      break;

    case MESH_CORE_ATTENTION_CHG_EVENT:
      if (pMsg->attention.attentionOn)
      {
        LIGHT_PRINT1("mesh_ind attention=on elemid=%d" LIGHT_NEWLINE,
                     pMsg->attention.elementId);
      }
      else
      {
        LIGHT_PRINT1("mesh_ind attention=off elemid=%d" LIGHT_NEWLINE,
                     pMsg->attention.elementId);
      }
      break;

    case MESH_CORE_NODE_STARTED_EVENT:
      if(pMsg->nodeStarted.hdr.status == MESH_SUCCESS)
      {
        LIGHT_PRINT2(LIGHT_NEWLINE "mesh_ind node_started elemaddr=0x%x elemcnt=%d"
                     LIGHT_NEWLINE, pMsg->nodeStarted.address, pMsg->nodeStarted.elemCnt);

        /* Bind the interface. */
        MeshAddAdvIf(LIGHT_ADV_IF_ID);

        /* OnPowerUp procedure must called after states and binding restoration. To ensure
         * models publish state changes the node must be started and an interface must exist.
         */
        MmdlGenPowOnOffOnPowerUp();
      }
      else
      {
        LIGHT_PRINT0(LIGHT_NEWLINE "mesh_ind node_started failed"
                     LIGHT_NEWLINE);
      }
      break;

    case MESH_CORE_PROXY_SERVICE_DATA_EVENT:

      if (pMsg->serviceData.serviceDataLen != 0)
      {
        /* Set ADV data for a Proxy server. */
        GattBearerSrSetPrxSvcData(pMsg->serviceData.serviceData, pMsg->serviceData.serviceDataLen);
      }
      break;

    case MESH_CORE_PROXY_FILTER_STATUS_EVENT:
      LIGHT_PRINT2("mesh_ind proxy_filter type=%d, list_size=%d" LIGHT_NEWLINE,
                   pMsg->filterStatus.filterType, pMsg->filterStatus.listSize);
      break;

    case MESH_CORE_IV_UPDATED_EVENT:
      LIGHT_PRINT1(LIGHT_NEWLINE "mesh_ind ividx=0x%x" LIGHT_NEWLINE,
                   pMsg->ivUpdt.ivIndex);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process messages from the mesh profile event handler.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightProcMeshMsg(wsfMsgHdr_t *pMsg)
{
  switch (pMsg->event)
  {
    case MESH_CORE_EVENT:
      lightProcMeshCoreMsg((meshEvt_t *) pMsg);
      break;

    case MESH_PRV_SR_EVENT:
      lightProcMeshPrvSrMsg((meshPrvSrEvt_t *) pMsg);
      break;

    case MESH_CFG_MDL_SR_EVENT:
      lightProcMeshCfgMdlSrMsg((meshCfgMdlSrEvt_t *) pMsg);
      break;

    default:
      break;
  }
}


/*************************************************************************************************/
/*!
 *  \brief     Process Mesh Model Generic On Off event callback.
 *
 *  \param[in] pEvt  Mesh Model event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightProcessMmdlGenOnOffEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->param)
  {
    case MMDL_GEN_ONOFF_SR_STATE_UPDATE_EVENT:
      switch (((mmdlGenOnOffSrStateUpdate_t *)pEvt)->state)
      {
        case MMDL_GEN_ONOFF_STATE_OFF:
          LIGHT_PRINT1("genonoff_ind elemid%d=off" LIGHT_NEWLINE,
                       ((mmdlGenOnOffSrStateUpdate_t *)pEvt)->elemId);

          if(((mmdlGenOnOffSrStateUpdate_t *)pEvt)->elemId == 0)
          {
            PalLedOff(2);
          }
          else
          {
            PalLedOff(0);
          }
          break;

        case MMDL_GEN_ONOFF_STATE_ON:
          LIGHT_PRINT1("genonoff_ind elemid%d=on" LIGHT_NEWLINE,
                       ((mmdlGenOnOffSrStateUpdate_t *)pEvt)->elemId);

          if(((mmdlGenOnOffSrStateUpdate_t *)pEvt)->elemId == 0)
          {
            PalLedOn(2);
          }
          else
          {
            PalLedOn(0);
          }
          break;

        default:
          break;
      }

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh Model Generic Level event callback.
 *
 *  \param[in] pEvt  Mesh Model event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightProcessMmdlGenLevelEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->param)
  {
    case MMDL_GEN_LEVEL_SR_STATE_UPDATE_EVENT:
      LIGHT_PRINT2("genlevel_ind elemid%d=%d" LIGHT_NEWLINE,
                   ((mmdlGenLevelSrStateUpdate_t *)pEvt)->elemId,
                   ((mmdlGenLevelSrStateUpdate_t *)pEvt)->state);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh Model Light Lightness event callback.
 *
 *  \param[in] pEvt  Mesh Model event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightProcessMmdlLightLightnessEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->param)
  {
    case MMDL_LIGHT_LIGHTNESS_SR_STATE_UPDATE_EVENT:
      LIGHT_PRINT2("lightl_ind elemid%d=%d" LIGHT_NEWLINE,
                   ((mmdlLightLightnessSrStateUpdate_t *)pEvt)->elemId,
                   ((mmdlLightLightnessSrStateUpdate_t *)pEvt)->lightnessState.state);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh Model Light HSL event callback.
 *
 *  \param[in] pEvt  Mesh Model event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightProcessMmdlLighHslEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->param)
  {
    case MMDL_LIGHT_HSL_SR_STATE_UPDATE_EVENT:
      LIGHT_PRINT4("lighthsl_ind elemid%d h=%d s=%d l=%d " LIGHT_NEWLINE,
                   ((mmdlLightHslSrStateUpdate_t *)pEvt)->elemId,
                   ((mmdlLightHslSrStateUpdate_t *)pEvt)->hslStates.state.hue,
                   ((mmdlLightHslSrStateUpdate_t *)pEvt)->hslStates.state.saturation,
                   ((mmdlLightHslSrStateUpdate_t *)pEvt)->hslStates.state.ltness);
      break;

    case MMDL_LIGHT_HSL_HUE_SR_STATE_UPDATE_EVENT:
      LIGHT_PRINT2("lighthue_ind elemid%d=%d " LIGHT_NEWLINE,
                   ((mmdlLightHslHueSrStateUpdate_t *)pEvt)->elemId,
                   ((mmdlLightHslHueSrStateUpdate_t *)pEvt)->state);
      break;

    case MMDL_LIGHT_HSL_SAT_SR_STATE_UPDATE_EVENT:
      LIGHT_PRINT2("lightsat_ind elemid%d=%d " LIGHT_NEWLINE,
                   ((mmdlLightHslSatSrStateUpdate_t *)pEvt)->elemId,
                   ((mmdlLightHslSatSrStateUpdate_t *)pEvt)->state);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Health Server event callback.
 *
 *  \param[in] pEvt  Health Server event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightMeshHtSrEventCback(const wsfMsgHdr_t *pEvt)
{
  meshHtSrEvt_t *pHtSrEvt = (meshHtSrEvt_t *)pEvt;
  switch(pHtSrEvt->hdr.param)
  {
    case MESH_HT_SR_TEST_START_EVENT:
      /* Default behavior is to log 0 faults and just update test id. */
      MeshHtSrAddFault(pHtSrEvt->testStartEvt.elemId, pHtSrEvt->testStartEvt.companyId,
                       pHtSrEvt->testStartEvt.testId, MESH_HT_MODEL_FAULT_NO_FAULT);

      /* Check if response is needed. */
      if(pHtSrEvt->testStartEvt.notifTestEnd)
      {
        /* Signal test end. */
        MeshHtSrSignalTestEnd(pHtSrEvt->testStartEvt.elemId, pHtSrEvt->testStartEvt.companyId,
                              pHtSrEvt->testStartEvt.htClAddr, pHtSrEvt->testStartEvt.appKeyIndex,
                              pHtSrEvt->testStartEvt.useTtlZero, pHtSrEvt->testStartEvt.unicastReq);
      }
      break;
    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Model event callback.
 *
 *  \param[in] pEvt  Mesh Model event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightMmdlEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->event)
  {
    case MESH_HT_SR_EVENT:
      lightMeshHtSrEventCback(pEvt);
      break;

    case MMDL_GEN_ONOFF_SR_EVENT:
      lightProcessMmdlGenOnOffEventCback(pEvt);
      break;

    case MMDL_GEN_LEVEL_SR_EVENT:
      lightProcessMmdlGenLevelEventCback(pEvt);
      break;

    case MMDL_LIGHT_LIGHTNESS_SR_EVENT:
      lightProcessMmdlLightLightnessEventCback(pEvt);
      break;

    case MMDL_LIGHT_HSL_SR_EVENT:
      lightProcessMmdlLighHslEventCback(pEvt);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Set up the node if provisioned, otherwise start provisioning procedure.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void lightSetup(void)
{
  static bool_t setupComplete = FALSE;

  /* This function is called once. */
  if (setupComplete)
  {
    return;
  }

  /* Check if device is provisioned. */
  if (MeshIsProvisioned())
  {
    /* Start Node. */
    MeshStartNode();

    /* Register the Mesh Proxy Service. */
    SvcMprxsRegister(MprxsWriteCback);

    /* Add the Mesh Proxy Service. */
    SvcMprxsAddGroup();

    /* Register Mesh Proxy Service CCC*/
    AttsCccRegister(LIGHT_NUM_CCC_IDX, (attsCccSet_t *) lightPrxCccSet, lightCccCback);

    /* Configure GATT server for Mesh Proxy. */
    MprxsSetCccIdx(LIGHT_DOUT_CCC_IDX);

    /* Register GATT Bearer callback */
    MeshRegisterGattProxyPduSendCback(MprxsSendDataOut);

    if (MeshIsGattProxyEnabled())
    {
      lightCb.netKeyIndexAdv = 0xFFFF;
      lightCb.proxyFeatEnabled = TRUE;

      /* Enable bearer slot */
      AppBearerEnableSlot(BR_GATT_SLOT);
    }
  }
  else
  {
    LIGHT_PRINT0(LIGHT_NEWLINE "mesh_ind device_unprovisioned" LIGHT_NEWLINE);

    /* Initialize Provisioning Server. */
    MeshPrvSrInit(&lightPrvSrUpdInfo);

    /* Register Provisioning Server callback. */
    MeshPrvSrRegister(lightMeshPrvSrCback);

    /* Register the Mesh Prov Service. */
    SvcMprvsRegister(MprvsWriteCback);

    /* Add the Mesh Prov Service. */
    SvcMprvsAddGroup();

    /* Register Mesh Provisioning Service CCC. */
    AttsCccRegister(LIGHT_NUM_CCC_IDX, (attsCccSet_t *) lightPrvCccSet, lightCccCback);

    /* Configure GATT server for Mesh Provisioning. */
    MprvsSetCccIdx(LIGHT_DOUT_CCC_IDX);

    /* Register GATT Bearer callback. */
    MeshRegisterGattProxyPduSendCback(MprvsSendDataOut);

    /* Set ADV data for an unprovisioned node. */
    GattBearerSrSetPrvSvcData(pMeshPrvSrCfg->devUuid, lightPrvSrUpdInfo.oobInfoSrc);

    /* Enable bearer slot. */
    /* TODO Enable Proxy Bearer */
    /*AppBearerEnableSlot(BR_GATT_SLOT); */

    /* Bind the interface. */
    MeshAddAdvIf(LIGHT_ADV_IF_ID);

    /* Enter provisioning. */
    MeshPrvSrEnterPbAdvProvisioningMode(LIGHT_ADV_IF_ID, 500);

    /* Provisioning started. */
    lightCb.prvSrStarted = TRUE;

    LIGHT_PRINT0("prvsr_ind prv_started" LIGHT_NEWLINE);
  }

  setupComplete = TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Process messages from the event handler.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightProcMsg(dmEvt_t *pMsg)
{
  switch(pMsg->hdr.event)
  {
    case DM_RESET_CMPL_IND:
      lightSetup();
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     event handler for button events.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void LightBtnHandler(void)
{
  uint8_t newBtns;
  uint8_t btn;

  WsfTaskLock();

  newBtns = lightCb.newBtnStates;

  /* clear new buttons */
  lightCb.newBtnStates &= ~newBtns;

  WsfTaskUnlock();

  for (btn = 0; btn < LIGHT_BUTTON_MAX; btn++)
  {
    if (newBtns & (1 << btn))
    {
      switch(btn)
      {
        case LIGHT_BUTTON_1:
          /* Clear NVM. */
          AppMeshClearNvm();
          LightConfigErase();

          /* Reset system. */
          AppMeshReset();
          break;

        default:
          break;
      }
    }
    else if ((newBtns >> btn) == 0)
    {
      /* Nothing left to process. Exit early. */
      break;
    }
  }
}

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Start the application.
 *
 *  \return None.
 */
/*************************************************************************************************/
void LightStart(void)
{
  /* Initialize the LE Stack. */
  DmConnRegister(DM_CLIENT_ID_APP, lightDmCback);

  /* Register for stack callbacks. */
  DmRegister(lightDmCback);
  AttRegister(lightAttCback);

  /* Reset the device. */
  DmDevReset();

  /* Set application version. */
  AppMeshSetVersion(LIGHT_VERSION);

  /* Register callback. */
  MeshRegister(lightMeshCback);

  /* Initialize GATT Proxy */
  MeshGattProxyInit();

  /* Set timer parameters. */
  lightCb.nodeIdentityTmr.isStarted = FALSE;
  lightCb.nodeIdentityTmr.handlerId = lightHandlerId;
  lightCb.nodeIdentityTmr.msg.event = APP_MESH_NODE_IDENTITY_TIMEOUT_EVT;

  /* Register server callback. */
  AttConnRegister(AppServerConnCback);

  /* Initialize GATT Bearer Server */
  GattBearerSrInit((gattBearerSrCfg_t *)&lightGattBearerSrCfg);

  /* Initialize Proxy Server. */
  MeshProxySrInit();

  /* Initialize Configuration Server. */
  MeshCfgMdlSrInit();

  /* Register Configuration Server callback. */
  MeshCfgMdlSrRegister(lightMeshCfgMdlSrCback);

  /* Initialize Mesh Friend. */
  MeshFriendInit(LIGHT_FRIEND_RECEIVE_WINDOW);

  /* Initialize Health Server. */
  MeshHtSrInit();

  /* Register callback. */
  MeshHtSrRegister(lightMmdlEventCback);

  /* Configure company ID to an unused one. */
  MeshHtSrSetCompanyId(0, 0, LIGHT_HT_SR_COMPANY_ID);

  /* Add 0 faults to update recent test ID. */
  MeshHtSrAddFault(0, 0xFFFF, LIGHT_HT_SR_TEST_ID, MESH_HT_MODEL_FAULT_NO_FAULT);

  /* Initialize application bearer scheduler. */
  AppBearerInit(lightHandlerId);

  /* Initialize the Advertising Bearer. */
  AdvBearerInit((advBearerCfg_t *)&lightAdvBearerCfg);

  /* Register callback for application bearer events. */
  AppBearerRegister(lightBearerCback);

  /* TODO Schedule GATT bearer. */
  /*AppBearerScheduleSlot(BR_GATT_SLOT, GattBearerSrStart, GattBearerSrStop, GattBearerSrProcDmMsg,
                          5000); */

  /* Register ADV Bearer callback. */
  MeshRegisterAdvIfPduSendCback(AdvBearerSendPacket);

  LightConfig();

  /* Initialize the models. */
  MmdlGenOnOffSrInit();
  MmdlGenLevelSrInit();
  MmdlGenPowOnOffSrInit();
  MmdlGenPowOnOffSetupSrInit();
  MmdlGenDefaultTransSrInit();
  MmdlLightLightnessSrInit();
  MmdlLightLightnessSetupSrInit();
  MmdlSceneSrInit();
  MmdlLightHslSrInit();
  MmdlLightHslHueSrInit();
  MmdlLightHslSatSrInit();

  /* Install Generic model callback. */
  MmdlGenOnOffSrRegister(lightMmdlEventCback);
  MmdlGenPowOnOffSrRegister(lightMmdlEventCback);
  MmdlGenPowOnOffSetupSrRegister(lightMmdlEventCback);
  MmdlGenLevelSrRegister(lightMmdlEventCback);

  /* Install Lighting model callbacks. */
  MmdlGenDefaultTransSrRegister(lightMmdlEventCback);
  MmdlLightLightnessSrRegister(lightMmdlEventCback);
  MmdlLightLightnessSetupSrRegister(lightMmdlEventCback);
  MmdlLightHslSrRegister(lightMmdlEventCback);
  MmdlLightHslHueSrRegister(lightMmdlEventCback);
  MmdlLightHslSatSrRegister(lightMmdlEventCback);

  /* Initializes the bind resolver module. */
  MmdlBindingsInit();

  /* Add bindings. */
  MmdlLightHslHueSrBind2GenLevel(ELEM_HUE, ELEM_HUE);
  MmdlLightHslSatSrBind2GenLevel(ELEM_SAT, ELEM_SAT);
  MmdlLightLightnessSrBind2GenLevel(ELEM_HSL, ELEM_HSL);
  MmdlLightLightnessSrBind2OnOff(ELEM_HSL, ELEM_HSL);
  MmdlLightHslSrBind2LtLtnessAct(ELEM_HSL, ELEM_HSL);

  /* Link Main, Hue and Sat elements. */
  MmdlLightHslSrLinkElements(ELEM_HSL, ELEM_HUE, ELEM_SAT);

  /* Add OnPowerUp bindings. */
  MmdlGenOnOffSrBind2OnPowerUp(ELEM_MAIN, ELEM_MAIN);
  MmdlGenOnOffSrBind2OnPowerUp(ELEM_HSL, ELEM_HSL);
  MmdlLightLightnessSrBind2OnPowerUp(ELEM_HSL, ELEM_HSL);
  MmdlLightHslSrBind2OnPowerUp(ELEM_HSL, ELEM_HSL);

  /* Set provisioning configuration pointer. */
  pMeshPrvSrCfg = &lightMeshPrvSrCfg;

  /* Initialize common Mesh Application functionality. */
  AppMeshNodeInit();

  /* Initialize on board LEDs. */
  PalLedInit();

  /* Initialize with buttons */
  PalBtnInit(lightBtnCback);
}

/*************************************************************************************************/
/*!
 *  \brief     Application handler init function called during system initialization.
 *
 *  \param[in] handlerID  WSF handler ID for App.
 *
 *  \retval    None
 */
/*************************************************************************************************/
void LightHandlerInit(wsfHandlerId_t handlerId)
{
  APP_TRACE_INFO0("LIGHT: Light Application Initialize");

  lightHandlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief     Initialize Mesh configuration for the application.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void LightConfigInit(void)
{
  /* Initialize configuration. */
  pMeshConfig = &lightMeshConfig;
}

/*************************************************************************************************/
/*!
 *  \brief     The WSF event handler for the Light App
 *
 *  \param[in] event   Event mask.
 *  \param[in] pMsg    Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void LightHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
  if (pMsg != NULL)
  {
    APP_TRACE_INFO1("LIGHT: App got evt %d", pMsg->event);

    /* Process ATT messages. */
    if (pMsg->event <= ATT_CBACK_END)
    {
      /* Process discovery-related ATT messages. */
      AppDiscProcAttMsg((attEvt_t *) pMsg);
    }
    else if (pMsg->event <= DM_CBACK_END)
    {
      /* Process advertising and connection-related messages. */
      AppBearerProcDmMsg((dmEvt_t *) pMsg);

      if (pMsg->status == HCI_SUCCESS)
      {
        if(pMsg->event == DM_CONN_OPEN_IND)
        {
          /* Disable GATT bearer slot while in connection. */
          AppBearerDisableSlot(BR_GATT_SLOT);
        }
        else if(pMsg->event == DM_CONN_CLOSE_IND)
        {
          if (lightCb.prvSrStarted || lightCb.proxyFeatEnabled || lightCb.nodeIdentityRunning)
          {
            /* Enable GATT bearer after connection closed. */
            AppBearerEnableSlot(BR_GATT_SLOT);
          }
        }
      }
    }
    else if ((pMsg->event >= MESH_CBACK_START) && (pMsg->event <= MESH_CBACK_END))
    {
      /* Process Mesh message. */
      lightProcMeshMsg(pMsg);
    }
    else
    {
      /* Application events. */
      if (pMsg->event == APP_BR_TIMEOUT_EVT)
      {
        AppBearerSchedulerTimeout();
      }

      if (pMsg->event == APP_MESH_NODE_IDENTITY_TIMEOUT_EVT)
      {
        /* Node Identity stopped. */
        MeshProxySrGetNextServiceData(MESH_PROXY_NWK_ID_TYPE);

        lightCb.netKeyIndexAdv = 0xFFFF;
        lightCb.nodeIdentityRunning = FALSE;

        /* Check if Proxy is enabled. */
        if (!lightCb.proxyFeatEnabled)
        {
          /* Disable bearer slot. */
          AppBearerDisableSlot(BR_GATT_SLOT);
        }
      }

      if (pMsg->event == APP_MESH_NODE_IDENTITY_USER_INTERACTION_EVT)
      {
        /* Get Service Data for the specified NetKey index*/
        MeshProxySrGetNextServiceData(MESH_PROXY_NODE_IDENTITY_TYPE);
        lightCb.netKeyIndexAdv = 0xFFFF;
        lightCb.nodeIdentityRunning = TRUE;

        /* Start Node Identity timer. */
        WsfTimerStartMs(&lightCb.nodeIdentityTmr, APP_MESH_NODE_IDENTITY_TIMEOUT_MS);

        /* Enable bearer slot. */
        AppBearerEnableSlot(BR_GATT_SLOT);
      }
    }

    if (lightCb.prvSrStarted)
    {
      MprvsProcMsg(pMsg);
    }
    else
    {
      MprxsProcMsg(pMsg);
    }

    lightProcMsg((dmEvt_t *)pMsg);
  }

  /* Check for events. */
  switch(event)
  {
    case LIGHT_BUTTON_EVENT:
      LightBtnHandler();
      break;

    default:
      break;
  }
}
