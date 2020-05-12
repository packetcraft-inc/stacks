/*************************************************************************************************/
/*!
 *  \file   mmdl_light_ctl_setup_sr_main.c
 *
 *  \brief  Implementation of the Light CTL Setup Server model.
 *
 *  Copyright (c) 2010-2018 Arm Ltd. All Rights Reserved.
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
 *
 */
/*************************************************************************************************/

#include <string.h>
#include "wsf_types.h"
#include "wsf_timer.h"
#include "wsf_os.h"
#include "wsf_cs.h"
#include "wsf_trace.h"
#include "wsf_assert.h"
#include "wsf_buf.h"
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"

#include "mmdl_types.h"
#include "mmdl_common.h"
#include "mmdl_defs.h"
#include "mmdl_bindings.h"
#include "mmdl_light_ctl_sr_api.h"
#include "mmdl_lightlightness_sr.h"
#include "mmdl_light_ctl_sr_main.h"
#include "mmdl_light_ctl_setup_sr.h"
#include "mmdl_light_ctl_sr.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Scenes Setup Server message handler type definition */
typedef void (*mmdlLightCtlSetupSrHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Supported opcodes */
const meshMsgOpcode_t mmdlLightCtlSetupSrRcvdOpcodes[MMDL_LIGHT_CTL_SETUP_SR_NUM_RCVD_OPCODES] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_CTL_DEFAULT_SET_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_CTL_DEFAULT_SET_NO_ACK_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_CTL_TEMP_RANGE_SET_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_CTL_TEMP_RANGE_SET_NO_ACK_OPCODE) }}
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlLightCtlSetupSrHandleMsg_t mmdlLightCtlSetupSrHandleMsg[MMDL_LIGHT_CTL_SETUP_SR_NUM_RCVD_OPCODES] =
{
  mmdlLightCtlSetupSrHandleDefaultSet,
  mmdlLightCtlSetupSrHandleDefaultSetNoAck,
  mmdlLightCtlSetupSrHandleRangeSet,
  mmdlLightCtlSetupSrHandleRangeSetNoAck,
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Processes Light CTL Default Set commands.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlLightCtlSetupSrProcessDefaultSet(const meshModelMsgRecvEvt_t *pMsg)
{
  bool_t status = FALSE;
  mmdlLightCtlSrDesc_t *pDesc;
  uint8_t *pMsgParam;
  uint16_t defaultLtness;
  uint16_t defaultTemp;
  uint16_t defaultDeltaUV;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Get the model instance descriptor */
  MmdlLightCtlSrGetDesc(pMsg->elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    /* Set the state value from pMessageParams buffer. */
    pMsgParam = pMsg->pMessageParams;
    BSTREAM_TO_UINT16(defaultLtness, pMsgParam);
    BSTREAM_TO_UINT16(defaultTemp, pMsgParam);
    BSTREAM_TO_UINT16(defaultDeltaUV, pMsgParam);

    if ((defaultTemp >= MMDL_LIGHT_CTL_TEMP_MIN) &&
        (defaultTemp <= MMDL_LIGHT_CTL_TEMP_MAX))
    {
      status = TRUE;
      pDesc->pStoredState->defaultTemperature = defaultTemp;
      pDesc->pStoredState->defaultDeltaUV = defaultDeltaUV;
      mmdlLightLightnessDefaultSrSetState(pMsg->elementId, defaultLtness,
                                          MMDL_STATE_UPDATED_BY_CL);

      /* Update default values in NVM. */
      if(pDesc->fNvmSaveStates)
      {
        pDesc->fNvmSaveStates(pMsg->elementId);
      }
    }
  }

  return status;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light CTL Range Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightCtlSetupSrHandleRangeSet(const meshModelMsgRecvEvt_t *pMsg)
{
  uint8_t opStatus = MMDL_RANGE_PROHIBITED;

  /* Change state */
  if (mmdlLightCtlSrProcessRangeSet(pMsg, &opStatus) && (opStatus!= MMDL_RANGE_PROHIBITED))
  {
    /* Send Status message as a response to the Range Set message */
    mmdlLightCtlSrSendRangeStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                                  pMsg->recvOnUnicast, opStatus);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light CTL Default Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightCtlSetupSrHandleRangeSetNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  uint8_t opStatus = MMDL_RANGE_PROHIBITED;

  (void)mmdlLightCtlSrProcessRangeSet(pMsg, &opStatus);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light CTL Default Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightCtlSetupSrHandleDefaultSet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  if (mmdlLightCtlSetupSrProcessDefaultSet(pMsg))
  {
    /* Send Status message as a response to the Default Set message */
    mmdlLightCtlSrSendDefaultStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                                    pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light CTL Default Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightCtlSetupSrHandleDefaultSetNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  (void)mmdlLightCtlSetupSrProcessDefaultSet(pMsg);
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Scenes Setup Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSetupSrHandler(wsfMsgHdr_t *pMsg)
{
  meshModelMsgRecvEvt_t *pModelMsg;
  uint8_t opcodeIdx, opcodeSize;

  /* Handle message */
  if (pMsg != NULL)
  {
    if (pMsg->event == MESH_MODEL_EVT_MSG_RECV)
    {
      pModelMsg = (meshModelMsgRecvEvt_t *)pMsg;

      /* Match the received opcode */
      for (opcodeIdx = 0; opcodeIdx < MMDL_LIGHT_CTL_SETUP_SR_NUM_RCVD_OPCODES; opcodeIdx++)
      {
        opcodeSize = MESH_OPCODE_SIZE(pModelMsg->opCode);
        if (!memcmp(&mmdlLightCtlSetupSrRcvdOpcodes[opcodeIdx], pModelMsg->opCode.opcodeBytes,
            opcodeSize))
        {
          /* Process message */
          (void)mmdlLightCtlSetupSrHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
        }
      }
    }
    else
    {
      MMDL_TRACE_WARN0("LIGHT CTL SETUP SR: Invalid event message received!");
    }
  }
}
