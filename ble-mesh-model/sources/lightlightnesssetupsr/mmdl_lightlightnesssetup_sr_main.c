/*************************************************************************************************/
/*!
 *  \file   mmdl_lightlightnesssetup_sr_api.c
 *
 *  \brief  Implementation of the Light Lightness Setup Server model.
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
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mmdl_common.h"
#include "mesh_api.h"

#include "mmdl_types.h"
#include "mmdl_defs.h"
#include "mmdl_lightlightness_sr_api.h"
#include "mmdl_lightlightness_sr.h"
#include "mmdl_lightlightnesssetup_sr_main.h"
#include "mmdl_lightlightnesssetup_sr_api.h"

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler ID */
wsfHandlerId_t mmdlLightLightnessSetupSrHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlLightLightnessSetupSrRcvdOpcodes[MMDL_LIGHT_LIGHTNESSSETUP_SR_NUM_RCVD_OPCODES] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_LIGHTNESS_DEFAULT_SET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_LIGHTNESS_DEFAULT_SET_NO_ACK_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_LIGHTNESS_RANGE_SET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_LIGHTNESS_RANGE_SET_NO_ACK_OPCODE)} },
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlModelHandleMsg_t mmdlLightLightnessSetupSrHandleMsg[MMDL_LIGHT_LIGHTNESSSETUP_SR_NUM_RCVD_OPCODES] =
{
  mmdlLightLightnessDefaultSetupSrHandleSet,
  mmdlLightLightnessDefaultSetupSrHandleSetNoAck,
  mmdlLightLightnessRangeSetupSrHandleSet,
  mmdlLightLightnessRangeSetupSrHandleSetNoAck,
};

/*! Light Lightness Setup Server received callback */
/* For future enhancement */
/* static mmdlEventCback_t llSetupSrRecvCback; */

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Model Light Lightness Setup Server receive empty callback
 *
 *  \param[in] pEvent  Light Lightness Setup Server event.
 *
 *  \return    None.
 */
/*************************************************************************************************/
/* For future enhancement */
/*
static void mmdlLightLightnessSetupSrRecvEmptyCback(const wsfMsgHdr_t *pEvent)
{
  MMDL_TRACE_WARN0("LIGHT LIGHTNESS SETUP SR: Receive callback not set!");
  (void)pEvent;
}
*/

/*************************************************************************************************/
/*!
 *  \brief     Set the local Light Lightness Default state.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static bool_t mmdlLightLightnessDefaultSetupSrSet(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlLightLightnessState_t state;

  MMDL_TRACE_INFO1("LIGHT LIGHTNESS SETUP SR: Set Default State on elemId %d", pMsg->elementId);

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length. It can take only one value. */
  if (pMsg->messageParamsLen == MMDL_LIGHT_LIGHTNESS_DEFAULT_SET_LEN)
  {
    /* Set the state value from pMessageParams buffer. */
    BYTES_TO_UINT16(state, &pMsg->pMessageParams[0]);

    mmdlLightLightnessDefaultSrSetState(pMsg->elementId, state, MMDL_STATE_UPDATED_BY_CL);

    return TRUE;
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local Light Lightness Range state.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static bool_t mmdlLightLightnessRangeSetupSrSet(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlLightLightnessRangeState_t rangeState;
  uint8_t *pParams;

  MMDL_TRACE_INFO1("LIGHT LIGHTNESS SETUP SR: Set Range State on elemId %d", pMsg->elementId);

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length. It can take only one value. */
  if (pMsg->messageParamsLen == MMDL_LIGHT_LIGHTNESS_RANGE_SET_LEN)
  {
    /* Set the state value from pMessageParams buffer. */
    pParams = pMsg->pMessageParams;
    BSTREAM_TO_UINT16(rangeState.rangeMin, pParams);
    BSTREAM_TO_UINT16(rangeState.rangeMax, pParams);

    return mmdlLightLightnessRangeSrSetState(pMsg->elementId, &rangeState, MMDL_STATE_UPDATED_BY_CL);
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Default Setup Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessDefaultSetupSrHandleSetNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  (void)mmdlLightLightnessDefaultSetupSrSet(pMsg);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Default Setup Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessDefaultSetupSrHandleSet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  if (mmdlLightLightnessDefaultSetupSrSet(pMsg))
  {
    /* Send Status message as a response to the Set message */
    mmdlLightLightnessDefaultSrSendStatus(MMDL_LIGHT_LIGHTNESSSETUP_SR_MDL_ID, pMsg->elementId,
                                          pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Range Setup Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessRangeSetupSrHandleSetNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  (void)mmdlLightLightnessRangeSetupSrSet(pMsg);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Range Setup Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessRangeSetupSrHandleSet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  if (mmdlLightLightnessRangeSetupSrSet(pMsg))
  {
    /* Send Status message as a response to the Set message */
    mmdlLightLightnessRangeSrSendStatus(MMDL_LIGHT_LIGHTNESSSETUP_SR_MDL_ID, pMsg->elementId,
                                          pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Light Lightness Setup Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSetupSrInit(void)
{
  MMDL_TRACE_INFO0("LIGHT LIGHTNESS SETUP SR: init");

  /* Set event callbacks */
  /* For future enhancement */
  /* llSetupSrRecvCback = mmdlLightLightnessSetupSrRecvEmptyCback; */
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Light Lightness Setup Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Light Lightness Setup Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSetupSrHandlerInit(wsfHandlerId_t handlerId)
{
  /* Set handler ID */
  mmdlLightLightnessSetupSrHandlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Light Lightness Setup Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSetupSrHandler(wsfMsgHdr_t *pMsg)
{
  meshModelMsgRecvEvt_t *pModelMsg;
  uint8_t opcodeIdx;

  /* Handle message */
  if (pMsg != NULL)
  {
    switch (pMsg->event)
    {
      case MESH_MODEL_EVT_MSG_RECV:
        pModelMsg = (meshModelMsgRecvEvt_t *)pMsg;

        /* Validate opcode size and value */
        if (MESH_OPCODE_SIZE(pModelMsg->opCode) == MMDL_LIGHT_LIGHTNESS_OPCODES_SIZE)
        {
          /* Match the received opcode */
          for (opcodeIdx = 0; opcodeIdx < MMDL_LIGHT_LIGHTNESSSETUP_SR_NUM_RCVD_OPCODES; opcodeIdx++)
          {
            if (!memcmp(&mmdlLightLightnessSetupSrRcvdOpcodes[opcodeIdx], pModelMsg->opCode.opcodeBytes,
                MMDL_LIGHT_LIGHTNESS_OPCODES_SIZE))
            {
              /* Process message */
              (void)mmdlLightLightnessSetupSrHandleMsg[opcodeIdx](pModelMsg);
            }
          }
        }
        break;

      default:
        MMDL_TRACE_WARN0("LIGHT LIGHTNESS SETUP SR: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
*  \brief     Set the Light Lightness Default state.
*
*  \param[in] elementId    Identifier of the Element implementing the model.
*  \param[in] targetState  Target State for this transaction. See ::mmdlLightLightnessState_t
*
*  \return    None.
*/
/*************************************************************************************************/
void MmdlLightLightnessDefaultSetupSrSetState(meshElementId_t elementId,
                                              mmdlLightLightnessState_t targetState)
{
  /* Change state locally. */
  mmdlLightLightnessDefaultSrSetState(elementId, targetState, MMDL_STATE_UPDATED_BY_APP);
}

/*************************************************************************************************/
/*!
*  \brief     Set the Light Lightness Range state.
*
*  \param[in] elementId    Identifier of the Element implementing the model.
*  \param[in] targetState  Target State for this transaction. See ::mmdlLightLightnessRangeState_t
*
*  \return    None.
*/
/*************************************************************************************************/
void MmdlLightLightnessRangeSetupSrSetState(meshElementId_t elementId,
                                      const mmdlLightLightnessRangeState_t *pTargetState)
{
  /* Change state locally. */
  (void)mmdlLightLightnessRangeSrSetState(elementId, pTargetState, MMDL_STATE_UPDATED_BY_APP);
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSetupSrRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback */
  /* For future enhancement */
  /*
  if (recvCback != NULL)
  {
    llSetupSrRecvCback = recvCback;
  }
  */
}
