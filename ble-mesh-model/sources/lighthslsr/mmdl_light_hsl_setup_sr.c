/*************************************************************************************************/
/*!
 *  \file   mmdl_light_hsl_setup_sr_main.c
 *
 *  \brief  Implementation of the Light HSL Setup Server model.
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
#include "mmdl_light_hsl_sr_api.h"
#include "mmdl_lightlightness_sr.h"
#include "mmdl_light_hsl_sr_main.h"
#include "mmdl_light_hsl_setup_sr.h"
#include "mmdl_light_hsl_sr.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Scenes Setup Server message handler type definition */
typedef void (*mmdlLightHslSetupSrHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Supported opcodes */
const meshMsgOpcode_t mmdlLightHslSetupSrRcvdOpcodes[MMDL_LIGHT_HSL_SETUP_SR_NUM_RCVD_OPCODES] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_HSL_DEFAULT_SET_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_HSL_DEFAULT_SET_NO_ACK_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_HSL_RANGE_SET_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_HSL_RANGE_SET_NO_ACK_OPCODE) }}
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlLightHslSetupSrHandleMsg_t mmdlLightHslSetupSrHandleMsg[MMDL_LIGHT_HSL_SETUP_SR_NUM_RCVD_OPCODES] =
{
  mmdlLightHslSetupSrHandleDefaultSet,
  mmdlLightHslSetupSrHandleDefaultSetNoAck,
  mmdlLightHslSetupSrHandleRangeSet,
  mmdlLightHslSetupSrHandleRangeSetNoAck,
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Processes Light Hsl Default Set commands.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlLightHslSetupSrProcessDefaultSet(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlLightHslSrDesc_t *pDesc;
  uint8_t *pMsgParam;
  uint16_t defaultLtness;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Get the model instance descriptor */
  MmdlLightHslSrGetDesc(pMsg->elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    /* Set the state value from pMessageParams buffer. */
    pMsgParam = pMsg->pMessageParams;
    BSTREAM_TO_UINT16(defaultLtness, pMsgParam);
    mmdlLightLightnessDefaultSrSetState(pMsg->elementId, defaultLtness,
                                        MMDL_STATE_UPDATED_BY_CL);
    BSTREAM_TO_UINT16(pDesc->pStoredState->defaultHue, pMsgParam);
    BSTREAM_TO_UINT16(pDesc->pStoredState->defaultSat, pMsgParam);

    /* Update default values in NVM. */
    if(pDesc->fNvmSaveStates)
    {
      pDesc->fNvmSaveStates(pMsg->elementId);
    }
  }
  else
  {
    /* No descriptor found on element */
    return FALSE;
  }

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Hsl Range Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightHslSetupSrHandleRangeSet(const meshModelMsgRecvEvt_t *pMsg)
{
  uint8_t opStatus = MMDL_RANGE_PROHIBITED;

  /* Change state */
  if (mmdlLightHslSrProcessRangeSet(pMsg, &opStatus) && (opStatus!= MMDL_RANGE_PROHIBITED))
  {
    /* Send Status message as a response to the Range Set message */
    mmdlLightHslSrSendRangeStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                                  pMsg->recvOnUnicast, opStatus);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Hsl Default Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightHslSetupSrHandleRangeSetNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  uint8_t opStatus = MMDL_RANGE_PROHIBITED;

  (void)mmdlLightHslSrProcessRangeSet(pMsg, &opStatus);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light HSL Default Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightHslSetupSrHandleDefaultSet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  if (mmdlLightHslSetupSrProcessDefaultSet(pMsg))
  {
    /* Send Status message as a response to the Default Set message */
    mmdlLightHslSrSendDefaultStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                                    pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light HSL Default Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightHslSetupSrHandleDefaultSetNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  (void)mmdlLightHslSetupSrProcessDefaultSet(pMsg);
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
void MmdlLightHslSetupSrHandler(wsfMsgHdr_t *pMsg)
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
      for (opcodeIdx = 0; opcodeIdx < MMDL_LIGHT_HSL_SETUP_SR_NUM_RCVD_OPCODES; opcodeIdx++)
      {
        opcodeSize = MESH_OPCODE_SIZE(pModelMsg->opCode);
        if (!memcmp(&mmdlLightHslSetupSrRcvdOpcodes[opcodeIdx], pModelMsg->opCode.opcodeBytes,
            opcodeSize))
        {
          /* Process message */
          (void)mmdlLightHslSetupSrHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
        }
      }
    }
    else
    {
      MMDL_TRACE_WARN0("LIGHT HSL SETUP SR: Invalid event message received!");
    }
  }
}
