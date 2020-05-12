/*************************************************************************************************/
/*!
 *  \file   provisioner_mmdl_handler.c
 *
 *  \brief  Mesh Model Handler for the Provisioner App.
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
#include "mesh_ht_cl_api.h"
#include "mmdl_gen_onoff_cl_api.h"
#include "mmdl_light_hsl_cl_api.h"

#include "provisioner_mmdl_handler.h"

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
static void provisionerHandleSigModelMsg(wsfMsgHdr_t *pMsg, meshSigModelId_t modelId)
{
  switch (modelId)
  {
    case MESH_HT_SR_MDL_ID:
      MeshHtSrHandler(pMsg);
      break;

    case MMDL_GEN_ONOFF_CL_MDL_ID:
      MmdlGenOnOffClHandler(pMsg);
      break;

    case MMDL_LIGHT_HSL_CL_MDL_ID:
      MmdlLightHslClHandler(pMsg);
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
static void provisionerHandleVendorModelMsg(wsfMsgHdr_t *pMsg, meshVendorModelId_t modelId)
{
  (void)pMsg;
  (void)modelId;
}

/**************************************************************************************************
  Public Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     WSF event handler for Mesh Models used by the Provisioner application.
 *
 *  \param[in] event  WSF event mask.
 *  \param[in] pMsg   WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void ProvisionerMmdlHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
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
          provisionerHandleVendorModelMsg(pMsg,
                                          ((meshModelMsgRecvEvt_t *)pMsg)->modelId.vendorModelId);
        }
        else
        {
          /* Handle message for vendor defined model. */
          provisionerHandleSigModelMsg(pMsg, ((meshModelMsgRecvEvt_t *)pMsg)->modelId.sigModelId);
        }
        break;

      case MESH_MODEL_EVT_PERIODIC_PUB:
        if (((meshModelPeriodicPubEvt_t *)pMsg)->isVendorModel)
        {
          /* Handle message for SIG defined model. */
          provisionerHandleVendorModelMsg(pMsg,
                                          ((meshModelPeriodicPubEvt_t *)pMsg)->modelId.vendorModelId);
        }
        else
        {
          /* Handle message for vendor defined model. */
          provisionerHandleSigModelMsg(pMsg, ((meshModelPeriodicPubEvt_t *)pMsg)->modelId.sigModelId);
        }
        break;

      case HT_SR_EVT_TMR_CBACK:
        MeshHtSrHandler(pMsg);
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
