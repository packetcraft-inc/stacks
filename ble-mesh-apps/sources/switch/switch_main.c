/*************************************************************************************************/
/*!
 *  \file   switch_main.c
 *
 *  \brief  Switch application.
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

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_error_codes.h"
#include "mesh_api.h"
#include "mesh_lpn_api.h"

#include "adv_bearer.h"

#include "mesh_prv.h"
#include "mesh_prv_sr_api.h"
#include "mesh_cfg_mdl_api.h"
#include "mesh_cfg_mdl_sr_api.h"
#include "mesh_cfg_mdl_cl_api.h"
#include "mesh_replay_protection.h"
#include "mesh_local_config.h"

#include "mmdl_types.h"
#include "mmdl_defs.h"
#include "mesh_ht_sr_api.h"
#include "mmdl_gen_onoff_cl_api.h"
#include "mmdl_gen_powonoff_cl_api.h"
#include "mmdl_gen_level_cl_api.h"
#include "mmdl_lightlightness_cl_api.h"
#include "mmdl_light_hsl_cl_api.h"

#include "app_mesh_api.h"
#include "app_mesh_terminal.h"
#include "app_bearer.h"

#include "switch_config.h"
#include "switch_api.h"
#include "switch_terminal.h"
#include "switch_version.h"

#include "pal_btn.h"
#include "pal_led.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Health Server company ID registered in the instance. */
#define SWITCH_HT_SR_COMPANY_ID              0xFFFF
/*! Health Server test ID for the associated to the test company ID */
#define SWITCH_HT_SR_TEST_ID                 0x00

#define SWITCH_NEWLINE
#define SWITCH_PRINT0(msg)                   APP_TRACE_INFO0(msg)
#define SWITCH_PRINT1(msg, var1)             APP_TRACE_INFO1(msg, var1)
#define SWITCH_PRINT2(msg, var1, var2)       APP_TRACE_INFO2(msg, var1, var2)
#define SWITCH_PRINT3(msg, var1, var2, var3) APP_TRACE_INFO3(msg, var1, var2, var3)
#define SWITCH_PRINT4(msg, var1, var2, var3, var4) \
                                             APP_TRACE_INFO4(msg, var1, var2, var3, var4)
#define SWITCH_PRINT5(msg, var1, var2, var3, var4, var5) \
                                             APP_TRACE_INFO5(msg, var1, var2, var3, var4, var5)
#define SWITCH_PRINT6(msg, var1, var2, var3, var4, var5, var6) \
                                             APP_TRACE_INFO6(msg, var1, var2, var3, var4, var5, var6)

/* Button identifiers */
enum
{
  SWITCH_BUTTON_1,
  SWITCH_BUTTON_2,
  SWITCH_BUTTON_3,
  SWITCH_BUTTON_MAX
};

/* Events */
#define SWITCH_BUTTON_EVENT                  1

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Switch App control block structure */
typedef struct switchCb_tag
{
  uint16_t    prvNetKeyIndex;  /*!< Provisioning NetKey index. */
  uint8_t     newBtnStates;    /*!< bitmask of changed button states */
} switchCb_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Switch App control block structure */
switchCb_t  switchCb;

/*! Switch element control block */
switchElemCb_t switchElemCb[SWITCH_ELEMENT_COUNT] =
{
  {
    .state          = MMDL_GEN_ONOFF_STATE_OFF,
    .tid            = 0,
  },
  {
    .state          = MMDL_GEN_ONOFF_STATE_OFF,
    .tid            = 0,
  }
};

/*! WSF handler ID */
wsfHandlerId_t switchHandlerId;

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
static void switchBtnCback(uint8_t btnId, PalBtnPos_t state)
{
  /* Only alert application of button press and not release. */
  if ((btnId < SWITCH_BUTTON_MAX) && (state == PAL_BTN_POS_DOWN))
  {
    switchCb.newBtnStates |= 1 << btnId;

    WsfSetEvent(switchHandlerId, SWITCH_BUTTON_EVENT);
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
static void switchDmCback(dmEvt_t *pDmEvt)
{
  dmEvt_t *pMsg;
  uint16_t len;

  len = DmSizeOfEvt(pDmEvt);

  if ((pMsg = WsfMsgAlloc(len)) != NULL)
  {
    memcpy(pMsg, pDmEvt, len);
    WsfMsgSend(switchHandlerId, pMsg);
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
static void switchMeshCback(meshEvt_t *pEvt)
{
  meshEvt_t *pMsg;
  uint16_t len;

  len = MeshSizeOfEvt(pEvt);

  if ((pMsg = WsfMsgAlloc(len)) != NULL)
  {
    memcpy(pMsg, pEvt, len);
    WsfMsgSend(switchHandlerId, pMsg);
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
static void switchMeshPrvSrCback(meshPrvSrEvt_t *pEvt)
{
  meshPrvSrEvt_t *pMsg;
  uint16_t len;

  len = MeshPrvSrSizeOfEvt(pEvt);

  if ((pMsg = WsfMsgAlloc(len)) != NULL)
  {
    memcpy(pMsg, pEvt, len);
    WsfMsgSend(switchHandlerId, pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh LPN event callback.
 *
 *  \param[in] pEvt  Mesh LPN event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void switchMeshLpnEvtNotifyCback(const meshLpnEvt_t *pEvt)
{
  meshLpnEvt_t *pMsg;
  uint16_t len;

  len = MeshLpnSizeOfEvt((wsfMsgHdr_t *) pEvt);

  if ((pMsg = WsfMsgAlloc(len)) != NULL)
  {
    memcpy(pMsg, pEvt, len);
    WsfMsgSend(switchHandlerId, pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Mesh Configuration Server event callback.
 *
 *  \param[in] pEvt  Configuration Server event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void switchMeshCfgMdlSrCback(const meshCfgMdlSrEvt_t* pEvt)
{
  meshCfgMdlSrEvt_t *pMsg;
  uint16_t len;

  len = MeshCfgSizeOfEvt((wsfMsgHdr_t *) pEvt);

  if ((pMsg = WsfMsgAlloc(len)) != NULL)
  {
    if (MeshCfgMsgDeepCopy((wsfMsgHdr_t *) pMsg, (wsfMsgHdr_t *) pEvt))
    {
      WsfMsgSend(switchHandlerId, pMsg);
    }
    else
    {
      WsfMsgFree(pMsg);
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
static void switchProcMeshPrvSrMsg(const meshPrvSrEvt_t *pMsg)
{
  meshPrvData_t prvData;

  switch (pMsg->hdr.param)
  {
    case MESH_PRV_SR_PROVISIONING_COMPLETE_EVENT:
      /* Stop PB-ADV provisioning loop */
      pMeshPrvSrCfg->pbAdvRestart = FALSE;

      /* Store Provisioning NetKey index. */
      switchCb.prvNetKeyIndex = ((meshPrvSrEvtPrvComplete_t *)pMsg)->netKeyIndex;

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

      SWITCH_PRINT1("prvsr_ind prv_complete elemaddr=0x%x" SWITCH_NEWLINE,
                    prvData.primaryElementAddr);
      break;

    case MESH_PRV_SR_PROVISIONING_FAILED_EVENT:
      SWITCH_PRINT1("prvsr_ind prv_failed reason=0x%x" SWITCH_NEWLINE,
                    pMsg->prvFailed.reason);

      /* Re-enter provisioning mode */
      if (pMeshPrvSrCfg->pbAdvRestart)
      {
        MeshPrvSrEnterPbAdvProvisioningMode(pMeshPrvSrCfg->pbAdvIfId, pMeshPrvSrCfg->pbAdvInterval);

        SWITCH_PRINT0("prvsr_ind prv_restarted" SWITCH_NEWLINE);
      }
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh Configuration Server messages from the event handler.
 *
 *  \param[in] pEvt  Configuration Server event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void switchProcMeshCfgMdlSrMsg(meshCfgMdlSrEvt_t *pEvt)
{
  switch (pEvt->hdr.param)
  {
    case MESH_CFG_MDL_NODE_RESET_EVENT:
      /* Clear NVM. */
      MeshLocalCfgEraseNvm();
      MeshRpNvmErase();

      /* Reset system. */
      AppMeshReset();
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh LPN event from the event handler.
 *
 *  \param[in] pEvt  Mesh LPN event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void switchProcMeshLpnMsg(meshLpnEvt_t *pEvt)
{
  switch(pEvt->hdr.param)
  {
    case MESH_LPN_FRIENDSHIP_ESTABLISHED_EVENT:
      SWITCH_PRINT1("lpn_ind est nidx=0x%x" SWITCH_NEWLINE,
                    ((meshLpnFriendshipEstablishedEvt_t *)pEvt)->netKeyIndex);
      break;

    case MESH_LPN_FRIENDSHIP_TERMINATED_EVENT:
      SWITCH_PRINT1("lpn_ind term nidx=0x%x" SWITCH_NEWLINE,
                    ((meshLpnFriendshipTerminatedEvt_t *)pEvt)->netKeyIndex);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh Core messages from the event handler.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void switchProcMeshCoreMsg(meshEvt_t *pMsg)
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

        APP_TRACE_INFO0("SWITCH: Interface added");
      }
      else
      {
        APP_TRACE_ERR1("SWITCH: Interface add error, %d", pMsg->hdr.status);
      }
      break;

    case MESH_CORE_ADV_IF_REMOVE_EVENT:
      if (pMsg->hdr.status == MESH_SUCCESS)
      {
        /* Unregister advertising interface from bearer. */
        AdvBearerDeregisterIf();

        /* Disable ADV bearer scheduling. */
        AppBearerDisableSlot(BR_ADV_SLOT);

        APP_TRACE_INFO0("SWITCH: Interface removed");
      }
      else
      {
        APP_TRACE_ERR1("SWITCH: Interface remove error, %d", pMsg->hdr.status);
      }
      break;

    case MESH_CORE_ADV_IF_CLOSE_EVENT:
      if (pMsg->hdr.status == MESH_SUCCESS)
      {
        APP_TRACE_INFO0("SWITCH: Interface closed");
      }
      else
      {
        APP_TRACE_ERR1("SWITCH: Interface close error, %d", pMsg->hdr.status);
      }
      break;

    case MESH_CORE_ATTENTION_CHG_EVENT:
      if (pMsg->attention.attentionOn)
      {
        SWITCH_PRINT1("mesh_ind attention=on elemid=%d" SWITCH_NEWLINE,
                      pMsg->attention.elementId);
      }
      else
      {
        SWITCH_PRINT1("mesh_ind attention=off elemid=%d" SWITCH_NEWLINE,
                      pMsg->attention.elementId);
      }
      break;

    case MESH_CORE_NODE_STARTED_EVENT:
      if(pMsg->nodeStarted.hdr.status == MESH_SUCCESS)
      {
        SWITCH_PRINT2(SWITCH_NEWLINE "mesh_ind node_started elemaddr=0x%x elemcnt=%d"
                      SWITCH_NEWLINE, pMsg->nodeStarted.address, pMsg->nodeStarted.elemCnt);

        /* Bind the interface. */
        MeshAddAdvIf(SWITCH_ADV_IF_ID);
      }
      else
      {
        SWITCH_PRINT0(SWITCH_NEWLINE "mesh_ind node_started failed"
                      SWITCH_NEWLINE);
      }
      break;

    case MESH_CORE_IV_UPDATED_EVENT:
      SWITCH_PRINT1(SWITCH_NEWLINE "mesh_ind ividx=0x%x" SWITCH_NEWLINE,
                    pMsg->ivUpdt.ivIndex);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh messages from the event handler.
 *
 *  \param[in] pMsg  Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void switchProcMeshMsg(wsfMsgHdr_t *pMsg)
{
  switch(pMsg->event)
  {
    case MESH_CORE_EVENT:
      switchProcMeshCoreMsg((meshEvt_t *) pMsg);
      break;

    case MESH_CFG_MDL_SR_EVENT:
      switchProcMeshCfgMdlSrMsg((meshCfgMdlSrEvt_t *) pMsg);
      break;

    case MESH_LPN_EVENT:
      switchProcMeshLpnMsg((meshLpnEvt_t *) pMsg);
      break;

    case MESH_PRV_SR_EVENT:
      switchProcMeshPrvSrMsg((meshPrvSrEvt_t *) pMsg);
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
static void switchProcessMmdlGenOnOffEventCback(const wsfMsgHdr_t *pEvt)
{
  meshElementId_t elementId;

  switch(pEvt->param)
  {
    case MMDL_GEN_ONOFF_CL_STATUS_EVENT:
      elementId = ((mmdlGenOnOffClEvent_t *)pEvt)->statusEvent.elementId;

      /* Update GenOnOffSr state. */
      switchElemCb[elementId].state = ((mmdlGenOnOffClEvent_t *)pEvt)->statusEvent.state;

      SWITCH_PRINT2("genonoff_ind status addr=0x%x state=%s" SWITCH_NEWLINE,
                    ((mmdlGenOnOffClEvent_t *)pEvt)->statusEvent.serverAddr,
                    (((mmdlGenOnOffClEvent_t *)pEvt)->statusEvent.state == MMDL_GEN_ONOFF_STATE_ON) ?
                    "on" : "off");
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Process Mesh Model Generic Power On Off event callback.
 *
 *  \param[in] pEvt  Mesh Model event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void switchProcessMmdlGenPowerOnOffEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->param)
  {
    case MMDL_GEN_POWER_ONOFF_CL_STATUS_EVENT:
        SWITCH_PRINT2("genonpowup_ind status addr=0x%x state=0x%X" SWITCH_NEWLINE,
                      ((mmdlGenPowOnOffClStatusEvent_t *)pEvt)->serverAddr,
                      ((mmdlGenPowOnOffClStatusEvent_t *)pEvt)->state);
      break;

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
static void switchProcessMmdlGenLevelEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->param)
  {
    case MMDL_GEN_LEVEL_CL_STATUS_EVENT:
      if (((mmdlGenLevelClStatusEvent_t *)pEvt)->remainingTime > 0)
      {
        SWITCH_PRINT4("genlvl_ind status addr=0x%x state=0x%X target=0x%X remtime=0x%X"
                      SWITCH_NEWLINE,
                      ((mmdlGenLevelClStatusEvent_t *)pEvt)->serverAddr,
                      ((mmdlGenLevelClStatusEvent_t *)pEvt)->state,
                      ((mmdlGenLevelClStatusEvent_t *)pEvt)->targetState,
                      ((mmdlGenLevelClStatusEvent_t *)pEvt)->remainingTime);
      }
      else
      {
        SWITCH_PRINT2("genlvl_ind status addr=0x%x state=0x%X" SWITCH_NEWLINE,
                      ((mmdlGenLevelClStatusEvent_t *)pEvt)->serverAddr,
                      ((mmdlGenLevelClStatusEvent_t *)pEvt)->state);
      }
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
static void switchProcessMmdlLightLightnessEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->param)
  {
    case MMDL_LIGHT_LIGHTNESS_CL_STATUS_EVENT:
      if (((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.actualStatusEvent.remainingTime > 0)
      {
        SWITCH_PRINT4("lightl_ind status addr=0x%x state=0x%X target=0x%X remtime=0x%X" SWITCH_NEWLINE,
                      ((mmdlLightLightnessClEvent_t *)pEvt)->serverAddr,
                      ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.actualStatusEvent.presentLightness,
                      ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.actualStatusEvent.targetLightness,
                      ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.actualStatusEvent.remainingTime);
      }
      else
      {
        SWITCH_PRINT2("lightl_ind status addr=0x%x state=0x%X" SWITCH_NEWLINE,
                      ((mmdlLightLightnessClEvent_t *)pEvt)->serverAddr,
                      ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.actualStatusEvent.presentLightness);
      }
      break;

    case MMDL_LIGHT_LIGHTNESS_LINEAR_CL_STATUS_EVENT:
      if (((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.linearStatusEvent.remainingTime > 0)
      {
        SWITCH_PRINT4("lightl_ind linstatus addr=0x%x state=0x%X target=0x%X remtime=0x%X" SWITCH_NEWLINE,
                      ((mmdlLightLightnessClEvent_t *)pEvt)->serverAddr,
                      ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.linearStatusEvent.presentLightness,
                      ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.linearStatusEvent.targetLightness,
                      ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.linearStatusEvent.remainingTime);
      }
      else
      {
        SWITCH_PRINT2("lightl_ind linstatus addr=0x%x state=0x%X" SWITCH_NEWLINE,
                      ((mmdlLightLightnessClEvent_t *)pEvt)->serverAddr,
                      ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.linearStatusEvent.presentLightness);
      }
      break;

    case MMDL_LIGHT_LIGHTNESS_LAST_CL_STATUS_EVENT:
      SWITCH_PRINT2("lightl_ind laststatus addr=0x%x state=0x%X" SWITCH_NEWLINE,
                    ((mmdlLightLightnessClEvent_t *)pEvt)->serverAddr,
                    ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.lastStatusEvent.lightness);
      break;

    case MMDL_LIGHT_LIGHTNESS_DEFAULT_CL_STATUS_EVENT:
      SWITCH_PRINT2("lldef_ind status elemid=%d state=0x%X" SWITCH_NEWLINE,
                    ((mmdlLightLightnessClEvent_t *)pEvt)->elementId,
                    ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.defaultStatusEvent.lightness);
      break;

    case MMDL_LIGHT_LIGHTNESS_RANGE_CL_STATUS_EVENT:
      SWITCH_PRINT4("lightl_ind rangestatus addr=0x%x status=0x%X min=0x%X max=0x%X" SWITCH_NEWLINE,
                    ((mmdlLightLightnessClEvent_t *)pEvt)->serverAddr,
                    ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.rangeStatusEvent.statusCode,
                    ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.rangeStatusEvent.rangeMin,
                    ((mmdlLightLightnessClEvent_t *)pEvt)->statusParam.rangeStatusEvent.rangeMax);
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
static void switchProcessMmdlLightHslEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->param)
  {
    case MMDL_LIGHT_HSL_CL_STATUS_EVENT:
      if (((mmdlLightHslClStatusEvent_t *)pEvt)->remainingTime > 0)
      {
        SWITCH_PRINT5("lighthsl_ind status addr=0x%x lightness=0x%X hue=0x%X sat=0x%X remtime=0x%X"
                      SWITCH_NEWLINE,
                      ((mmdlLightHslClStatusEvent_t *)pEvt)->serverAddr,
                      ((mmdlLightHslClStatusEvent_t *)pEvt)->lightness,
                      ((mmdlLightHslClStatusEvent_t *)pEvt)->hue,
                      ((mmdlLightHslClStatusEvent_t *)pEvt)->saturation,
                      ((mmdlLightHslClStatusEvent_t *)pEvt)->remainingTime);
      }
      else
      {
        SWITCH_PRINT4("lighthsl_ind status addr=0x%x lightness=0x%X hue=0x%X sat=0x%X "
                      SWITCH_NEWLINE,
                      ((mmdlLightHslClStatusEvent_t *)pEvt)->serverAddr,
                      ((mmdlLightHslClStatusEvent_t *)pEvt)->lightness,
                      ((mmdlLightHslClStatusEvent_t *)pEvt)->hue,
                      ((mmdlLightHslClStatusEvent_t *)pEvt)->saturation);
      }
      break;

    case MMDL_LIGHT_HSL_CL_TARGET_STATUS_EVENT:
      if (((mmdlLightHslClStatusEvent_t *)pEvt)->remainingTime > 0)
      {
        SWITCH_PRINT5("lighthsl_ind targetstatus addr=0x%x lightness=0x%X hue=0x%X sat=0x%X remtime=0x%X"
                      SWITCH_NEWLINE,
                      ((mmdlLightHslClStatusEvent_t *)pEvt)->serverAddr,
                      ((mmdlLightHslClStatusEvent_t *)pEvt)->lightness,
                      ((mmdlLightHslClStatusEvent_t *)pEvt)->hue,
                      ((mmdlLightHslClStatusEvent_t *)pEvt)->saturation,
                      ((mmdlLightHslClStatusEvent_t *)pEvt)->remainingTime);
      }
      else
      {
        SWITCH_PRINT4("lighthsl_ind targetstatus addr=0x%x lightness=0x%X hue=0x%X sat=0x%X "
                      SWITCH_NEWLINE,
                      ((mmdlLightHslClStatusEvent_t *)pEvt)->serverAddr,
                      ((mmdlLightHslClStatusEvent_t *)pEvt)->lightness,
                      ((mmdlLightHslClStatusEvent_t *)pEvt)->hue,
                      ((mmdlLightHslClStatusEvent_t *)pEvt)->saturation);
      }
      break;

    case MMDL_LIGHT_HSL_CL_HUE_STATUS_EVENT:
      if (((mmdlLightHslClHueStatusEvent_t *)pEvt)->remainingTime > 0)
      {
        SWITCH_PRINT4("lighth_ind status addr=0x%x present=0x%X target=0x%X remtime=0x%X"
                      SWITCH_NEWLINE,
                      ((mmdlLightHslClHueStatusEvent_t *)pEvt)->serverAddr,
                      ((mmdlLightHslClHueStatusEvent_t *)pEvt)->presentHue,
                      ((mmdlLightHslClHueStatusEvent_t *)pEvt)->targetHue,
                      ((mmdlLightHslClHueStatusEvent_t *)pEvt)->remainingTime);
      }
      else
      {
        SWITCH_PRINT2("lighth_ind status addr=0x%x present=0x%X "
                      SWITCH_NEWLINE,
                      ((mmdlLightHslClHueStatusEvent_t *)pEvt)->serverAddr,
                      ((mmdlLightHslClHueStatusEvent_t *)pEvt)->presentHue);
      }
      break;

    case MMDL_LIGHT_HSL_CL_SAT_STATUS_EVENT:
      if (((mmdlLightHslClSatStatusEvent_t *)pEvt)->remainingTime > 0)
      {
        SWITCH_PRINT4("lights_ind status addr=0x%x present=0x%X target=0x%X remtime=0x%X"
                      SWITCH_NEWLINE,
                      ((mmdlLightHslClSatStatusEvent_t *)pEvt)->elementId,
                      ((mmdlLightHslClSatStatusEvent_t *)pEvt)->presentSat,
                      ((mmdlLightHslClSatStatusEvent_t *)pEvt)->targetSat,
                      ((mmdlLightHslClSatStatusEvent_t *)pEvt)->remainingTime);
      }
      else
      {
        SWITCH_PRINT2("lights_ind status addr=0x%x present=0x%X "
                      SWITCH_NEWLINE,
                      ((mmdlLightHslClSatStatusEvent_t *)pEvt)->serverAddr,
                      ((mmdlLightHslClSatStatusEvent_t *)pEvt)->presentSat);
      }
      break;

    case MMDL_LIGHT_HSL_CL_DEF_STATUS_EVENT:
      SWITCH_PRINT4("lighthsl_ind default addr=0x%x lightness=0x%X hue=0x%X sat=0x%X"
                    SWITCH_NEWLINE,
                    ((mmdlLightHslClDefStatusEvent_t *)pEvt)->serverAddr,
                    ((mmdlLightHslClDefStatusEvent_t *)pEvt)->lightness,
                    ((mmdlLightHslClDefStatusEvent_t *)pEvt)->hue,
                    ((mmdlLightHslClDefStatusEvent_t *)pEvt)->saturation);
      break;

    case MMDL_LIGHT_HSL_CL_RANGE_STATUS_EVENT:
      SWITCH_PRINT6("lighthsl_ind range addr=0x%x status=0x%X minhue=0x%X maxhue=0x%X \
                     minsat=0x%X maxsat=0x%X" SWITCH_NEWLINE,
                    ((mmdlLightHslClRangeStatusEvent_t *)pEvt)->serverAddr,
                    ((mmdlLightHslClRangeStatusEvent_t *)pEvt)->opStatus,
                    ((mmdlLightHslClRangeStatusEvent_t *)pEvt)->minHue,
                    ((mmdlLightHslClRangeStatusEvent_t *)pEvt)->maxHue,
                    ((mmdlLightHslClRangeStatusEvent_t *)pEvt)->minSaturation,
                    ((mmdlLightHslClRangeStatusEvent_t *)pEvt)->maxSaturation);
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
static void switchMeshHtSrEventCback(const wsfMsgHdr_t *pEvt)
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
static void switchMmdlEventCback(const wsfMsgHdr_t *pEvt)
{
  switch(pEvt->event)
  {
    case MESH_HT_CL_EVENT:
      switchMeshHtSrEventCback(pEvt);
      break;

    case MMDL_GEN_ONOFF_CL_EVENT:
      switchProcessMmdlGenOnOffEventCback(pEvt);
      break;

    case MMDL_GEN_POWER_ONOFF_CL_EVENT:
      switchProcessMmdlGenPowerOnOffEventCback(pEvt);
      break;

    case MMDL_GEN_LEVEL_CL_EVENT:
      switchProcessMmdlGenLevelEventCback(pEvt);
      break;

    case MMDL_LIGHT_LIGHTNESS_CL_EVENT:
      switchProcessMmdlLightLightnessEventCback(pEvt);
      break;

    case MMDL_LIGHT_HSL_CL_EVENT:
      switchProcessMmdlLightHslEventCback(pEvt);
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
static void switchSetup(void)
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
  }
  else
  {
    SWITCH_PRINT0(SWITCH_NEWLINE "mesh_ind device_unprovisioned" SWITCH_NEWLINE);

    /* Initialize Provisioning Server. */
    MeshPrvSrInit(&switchPrvSrUpdInfo);

    /* Register Provisioning Server callback. */
    MeshPrvSrRegister(switchMeshPrvSrCback);

    /* Bind the interface. */
    MeshAddAdvIf(SWITCH_ADV_IF_ID);

    /* Enter provisioning. */
    MeshPrvSrEnterPbAdvProvisioningMode(SWITCH_ADV_IF_ID, 500);

    pMeshPrvSrCfg->pbAdvRestart = TRUE;

    SWITCH_PRINT0("prvsr_ind prv_started" SWITCH_NEWLINE);
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
static void switchProcMsg(dmEvt_t *pMsg)
{
  switch(pMsg->hdr.event)
  {
    case DM_RESET_CMPL_IND:
      switchSetup();
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Send a Set Generic On Off No Acknowledgement message to publication address,
 *              toggling the last value sent.
 *
 *  \elementId  Identifier of the Element implementing the model.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void switchToggleOnOffClNoAck(uint8_t elementId)
{
  mmdlGenOnOffSetParam_t setParam;

  /* Toggle state. */
  switchElemCb[elementId].state = (switchElemCb[elementId].state == MMDL_GEN_ONOFF_STATE_ON) ?
                                   MMDL_GEN_ONOFF_STATE_OFF : MMDL_GEN_ONOFF_STATE_ON;

  /* Set Generic OnOff params. */
  setParam.state = switchElemCb[elementId].state;
  setParam.tid = switchElemCb[elementId].tid++;
  setParam.transitionTime = MMDL_GEN_TR_UNKNOWN;
  setParam.delay = 0;

  MmdlGenOnOffClSetNoAck(elementId, MMDL_USE_PUBLICATION_ADDR, 0, &setParam, 0);
}

/*************************************************************************************************/
/*!
 *  \brief     The WSF event handler for button events registered with platform.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void SwitchBtnHandler(void)
{
  uint8_t newBtns;
  uint8_t btn;

  WsfTaskLock();

  newBtns = switchCb.newBtnStates;

  /* clear new buttons */
  switchCb.newBtnStates &= ~newBtns;

  WsfTaskUnlock();

  for (btn = 0; btn < SWITCH_BUTTON_MAX; btn++)
  {
    if (newBtns & (1 << btn))
    {
      switch(btn)
      {
        case SWITCH_BUTTON_1:
          switchToggleOnOffClNoAck(SWITCH_ELEMENT_0);
          break;

        case SWITCH_BUTTON_2:
          switchToggleOnOffClNoAck(SWITCH_ELEMENT_1);
          break;

        case SWITCH_BUTTON_3:
          /* Clear NVM. */
          AppMeshClearNvm();

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
void SwitchStart(void)
{
  /* Initialize the LE Stack. */
  DmConnRegister(DM_CLIENT_ID_APP, switchDmCback);

  /* Register for stack callbacks. */
  DmRegister(switchDmCback);

  /* Reset the device. */
  DmDevReset();

  /* Set application version. */
  AppMeshSetVersion(SWITCH_VERSION);

  /* Register callback. */
  MeshRegister(switchMeshCback);

  /* Initialize Configuration Server. */
  MeshCfgMdlSrInit();

  /* Register Configuration Server callback. */
  MeshCfgMdlSrRegister(switchMeshCfgMdlSrCback);

  /* Initialize Mesh LPN */
  MeshLpnInit();

  /* Register LPN callback. */
  MeshLpnRegister(switchMeshLpnEvtNotifyCback);

  /* Initialize Health Server */
  MeshHtSrInit();

  /* Register callback. */
  MeshHtSrRegister(switchMmdlEventCback);

  /* Configure company ID to an unused one. */
  MeshHtSrSetCompanyId(0, 0, SWITCH_HT_SR_COMPANY_ID);

  /* Add 0 faults to update recent test ID. */
  MeshHtSrAddFault(0, 0xFFFF, SWITCH_HT_SR_TEST_ID, MESH_HT_MODEL_FAULT_NO_FAULT);

  /* Initialize application bearer scheduler */
  AppBearerInit(switchHandlerId);

  /* Initialize the Advertising Bearer. */
  AdvBearerInit((advBearerCfg_t *)&switchAdvBearerCfg);

  /* Register ADV Bearer callback. */
  MeshRegisterAdvIfPduSendCback(AdvBearerSendPacket);

  /* Install model client callback. */
  MmdlGenOnOffClRegister(switchMmdlEventCback);
  MmdlGenPowOnOffClRegister(switchMmdlEventCback);
  MmdlGenLevelClRegister(switchMmdlEventCback);
  MmdlLightLightnessClRegister(switchMmdlEventCback);
  MmdlLightHslClRegister(switchMmdlEventCback);

  /* Set provisioning configuration pointer. */
  pMeshPrvSrCfg = &switchMeshPrvSrCfg;

  /* Initialize common Mesh Application functionality. */
  AppMeshNodeInit();

  /* Initialize on board LEDs. */
  PalLedInit();

  /* Initialize with buttons */
  PalBtnInit(switchBtnCback);
}

/*************************************************************************************************/
/*!
 *  \brief     Application handler init function called during system initialization.
 *
 *  \param[in] handlerID  WSF handler ID for App.
 *
 *  \retval    None.
 */
/*************************************************************************************************/
void SwitchHandlerInit(wsfHandlerId_t handlerId)
{
  APP_TRACE_INFO0("SWITCH: Switch Application Initialize");

  switchHandlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief     Initialize Mesh configuration for the application.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void SwitchConfigInit(void)
{
  /* Initialize configuration. */
  pMeshConfig = &switchMeshConfig;
}

/*************************************************************************************************/
/*!
 *  \brief     The WSF event handler for the Switch App
 *
 *  \param[in] event  Event mask.
 *  \param[in] pMsg   Pointer to message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void SwitchHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{

  if (pMsg != NULL)
  {
    APP_TRACE_INFO1("SWITCH: App got evt %d", pMsg->event);

    if (pMsg->event >= DM_CBACK_START && pMsg->event <= DM_CBACK_END)
    {
      /* Process advertising and connection-related messages. */
      AppBearerProcDmMsg((dmEvt_t *) pMsg);
    }
    else if ((pMsg->event >= MESH_CBACK_START) && (pMsg->event <= MESH_CBACK_END))
    {
      /* Process Mesh message. */
      switchProcMeshMsg(pMsg);
    }
    else
    {
      /* Application events. */
      if (pMsg->event == APP_BR_TIMEOUT_EVT)
      {
        AppBearerSchedulerTimeout();
      }
    }

    switchProcMsg((dmEvt_t *)pMsg);
  }

  /* Check for events. */
  switch(event)
  {
    case SWITCH_BUTTON_EVENT:
      SwitchBtnHandler();
      break;

    default:
      break;
  }
}
