/*************************************************************************************************/
/*!
 *  \file   light_mmdl_handler.c
 *
 *  \brief  Mesh Model Handler for Light App.
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
 */
/*************************************************************************************************/

#include "wsf_types.h"
#include "wsf_trace.h"
#include "wsf_msg.h"
#include "wsf_buf.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_error_codes.h"
#include "mesh_api.h"

#include "mmdl_types.h"
#include "mmdl_defs.h"
#include "mesh_ht_sr_api.h"
#include "mmdl_gen_onoff_sr_api.h"
#include "mmdl_gen_powonoff_sr_api.h"
#include "mmdl_gen_powonoffsetup_sr_api.h"
#include "mmdl_gen_level_sr_api.h"
#include "mmdl_scene_sr_api.h"
#include "mmdl_gen_default_trans_sr_api.h"
#include "mmdl_lightlightness_sr_api.h"
#include "mmdl_lightlightnesssetup_sr_api.h"
#include "mmdl_light_hsl_sr_api.h"
#include "mmdl_light_hsl_hue_sr_api.h"
#include "mmdl_light_hsl_sat_sr_api.h"

#include "light_mmdl_handler.h"

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Handles messages from SIG defined models.
 *
 *  \param[in] pMsg     WSF message.
 *  \param[in] modelId  SIG defined model identifier
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightHandleSigModelMsg(wsfMsgHdr_t *pMsg, meshSigModelId_t modelId)
{
  switch (modelId)
  {
    case MESH_HT_SR_MDL_ID:
      MeshHtSrHandler(pMsg);
      break;

    case MMDL_GEN_ONOFF_SR_MDL_ID:
      MmdlGenOnOffSrHandler(pMsg);
      break;

    case MMDL_GEN_POWER_ONOFF_SR_MDL_ID:
      MmdlGenPowOnOffSrHandler(pMsg);
      break;

    case MMDL_GEN_POWER_ONOFFSETUP_SR_MDL_ID:
      MmdlGenPowOnOffSetupSrHandler(pMsg);
      break;

    case MMDL_GEN_LEVEL_SR_MDL_ID:
      MmdlGenLevelSrHandler(pMsg);
      break;

    case MMDL_SCENE_SETUP_SR_MDL_ID:
      MmdlSceneSetupSrHandler(pMsg);
      break;

    case MMDL_SCENE_SR_MDL_ID:
      MmdlSceneSrHandler(pMsg);
      break;

    case MMDL_GEN_DEFAULT_TRANS_SR_MDL_ID:
      MmdlGenDefaultTransSrHandler(pMsg);
      break;

    case MMDL_LIGHT_LIGHTNESS_SR_MDL_ID:
      MmdlLightLightnessSrHandler(pMsg);
      break;

    case MMDL_LIGHT_LIGHTNESSSETUP_SR_MDL_ID:
      MmdlLightLightnessSetupSrHandler(pMsg);
      break;

    case MMDL_LIGHT_HSL_SR_MDL_ID:
      MmdlLightHslSrHandler(pMsg);
      break;

    case MMDL_LIGHT_HSL_SETUP_SR_MDL_ID:
      MmdlLightHslSetupSrHandler(pMsg);
      break;

    case MMDL_LIGHT_HSL_HUE_SR_MDL_ID:
      MmdlLightHslHueSrHandler(pMsg);
      break;

    case MMDL_LIGHT_HSL_SAT_SR_MDL_ID:
      MmdlLightHslSatSrHandler(pMsg);
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles messages from Vendor defined models.
 *
 *  \param[in] pMsg     WSF message.
 *  \param[in] modelId  Vendor model identifier
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void lightHandleVendorModelMsg(wsfMsgHdr_t *pMsg, meshVendorModelId_t modelId)
{
  (void)pMsg;
  (void)modelId;
}

/**************************************************************************************************
  Public Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     WSF event handler for Mesh Models used by the Light application.
 *
 *  \param[in] event  WSF event mask.
 *  \param[in] pMsg   WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void LightMmdlHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
  /* Handle message */
  if (pMsg != NULL)
  {
    switch (pMsg->event)
    {
      case MESH_MODEL_EVT_MSG_RECV:
        if (MESH_OPCODE_IS_VENDOR(((meshModelMsgRecvEvt_t *)pMsg)->opCode))
        {
          /* Handle message for SIG defined model. */
          lightHandleVendorModelMsg(pMsg, ((meshModelMsgRecvEvt_t *)pMsg)->modelId.vendorModelId);
        }
        else
        {
          /* Handle message for vendor defined model. */
          lightHandleSigModelMsg(pMsg, ((meshModelMsgRecvEvt_t *)pMsg)->modelId.sigModelId);
        }
        break;

      case MESH_MODEL_EVT_PERIODIC_PUB:
        if (((meshModelPeriodicPubEvt_t *)pMsg)->isVendorModel)
        {
          /* Handle message for SIG defined model. */
          lightHandleVendorModelMsg(pMsg, ((meshModelPeriodicPubEvt_t *)pMsg)->modelId.vendorModelId);
        }
        else
        {
          /* Handle message for vendor defined model. */
          lightHandleSigModelMsg(pMsg, ((meshModelPeriodicPubEvt_t *)pMsg)->modelId.sigModelId);
        }
        break;

      case HT_SR_EVT_TMR_CBACK:
        MeshHtSrHandler(pMsg);
        break;

      case MMDL_GEN_ON_OFF_SR_EVT_TMR_CBACK:
      case MMDL_GEN_ON_OFF_SR_MSG_RCVD_TMR_CBACK:
        MmdlGenOnOffSrHandler(pMsg);
        break;

      case MMDL_GEN_LEVEL_SR_EVT_TMR_CBACK:
      case MMDL_GEN_LEVEL_SR_MSG_RCVD_TMR_CBACK:
        MmdlGenLevelSrHandler(pMsg);
        break;

      case MMDL_SCENE_SR_EVT_TMR_CBACK:
      case MMDL_SCENE_SR_MSG_RCVD_TMR_CBACK:
        MmdlSceneSrHandler(pMsg);
        break;

      case MMDL_LIGHT_LIGHTNESS_SR_EVT_TMR_CBACK:
      case MMDL_LIGHT_LIGHTNESS_SR_MSG_RCVD_TMR_CBACK:
        MmdlLightLightnessSrHandler(pMsg);
        break;

      case MMDL_LIGHT_LIGHTNESSSETUP_SR_EVT_TMR_CBACK:
      case MMDL_LIGHT_LIGHTNESSSETUP_SR_MSG_RCVD_TMR_CBACK:
        MmdlLightLightnessSetupSrHandler(pMsg);
        break;

      case MMDL_LIGHT_HSL_SR_EVT_TMR_CBACK:
      case MMDL_LIGHT_HSL_SR_MSG_RCVD_TMR_CBACK:
        MmdlLightHslSrHandler(pMsg);
        break;

      case MMDL_LIGHT_HSL_HUE_SR_EVT_TMR_CBACK:
      case MMDL_LIGHT_HSL_HUE_SR_MSG_RCVD_TMR_CBACK:
        MmdlLightHslHueSrHandler(pMsg);
        break;

      case MMDL_LIGHT_HSL_SAT_SR_EVT_TMR_CBACK:
      case MMDL_LIGHT_HSL_SAT_SR_MSG_RCVD_TMR_CBACK:
        MmdlLightHslSatSrHandler(pMsg);
        break;

      default:
        MESH_TRACE_WARN0("MMDL: Invalid event message received!");
        break;
    }
  }
  /* Handle events */
  else if (event)
  {
  }
}
