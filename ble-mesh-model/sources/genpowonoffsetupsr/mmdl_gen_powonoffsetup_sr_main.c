/*************************************************************************************************/
/*!
 *  \file   mmdl_gen_powonoff_setup_sr_main.c
 *
 *  \brief  Implementation of the Generic Power OnOff Setup Server model.
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
#include "mesh_api.h"

#include "mmdl_types.h"
#include "mmdl_defs.h"
#include "mmdl_common.h"

#include "mmdl_gen_powonoff_sr.h"
#include "mmdl_gen_powonoff_sr_api.h"
#include "mmdl_gen_powonoffsetup_sr_main.h"
#include "mmdl_gen_powonoffsetup_sr_api.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Generic Power OnOff Setup Server control block type definition */
typedef struct mmdlGenPowOnOffSetupSrCb_tag
{
  mmdlEventCback_t recvCback;   /*!< Model Generic Power OnOff Setup received callback */
} mmdlGenPowOnOffSetupSrCb_t;

/*! Generic Power OnOff Server message handler type definition */
typedef void (*mmdlGenPowOnOffSetupSrHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler ID */
wsfHandlerId_t mmdlGenPowOnOffSetupSrHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlGenPowOnOffSetupSrRcvdOpcodes[MMDL_GEN_POWER_ONOFFSETUP_SR_NUM_RCVD_OPCODES] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_ONPOWERUP_SET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_ONPOWERUP_SET_NO_ACK_OPCODE)} }
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlGenPowOnOffSetupSrHandleMsg_t mmdlGenPowOnOffSetupSrHandleMsg[MMDL_GEN_POWER_ONOFFSETUP_SR_NUM_RCVD_OPCODES] =
{
  mmdlGenPowOnOffSetupSrHandleSet,
  mmdlGenPowOnOffSetupSrHandleSetNoAck
};

/*! Generic Power OnOff Server Control Block */
static mmdlGenPowOnOffSetupSrCb_t  powOnOffSetupSrCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Set the local state.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlGenPowOnOffSetupSrSet(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlGenPowOnOffSrStateUpdate_t event;

  MMDL_TRACE_INFO1("GEN POWER ONOFF SETUP SR: Set State on elemId %d", pMsg->elementId);

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length and check prohibited values for OnPowerUp State */
  if ((pMsg->messageParamsLen != MMDL_GEN_POWER_ONOFFSETUP_SET_LEN) ||
      (pMsg->pMessageParams[0] >= MMDL_GEN_ONPOWERUP_STATE_PROHIBITED))
  {
    /* Set event parameters */
    event.hdr.event = MMDL_GEN_POWER_ONOFF_SR_EVENT;
    event.hdr.param = MMDL_GEN_POWER_ONOFF_SR_STATE_UPDATE_EVENT;
    event.hdr.status = MMDL_INVALID_PARAM;
    event.elemId = pMsg->elementId;
    event.stateUpdateSource = MMDL_STATE_UPDATED_BY_CL;
    event.state = pMsg->pMessageParams[0];

    /* Send event to the upper layer */
    powOnOffSetupSrCb.recvCback((wsfMsgHdr_t *)&event);

    return FALSE;
  }
  else
  {
    /* Change state */
    MmdlGenPowOnOffOnPowerUpSrSetState(pMsg->elementId, pMsg->pMessageParams[0],
                                       MMDL_STATE_UPDATED_BY_CL);
    return TRUE;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Power OnOff Setup Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenPowOnOffSetupSrHandleSetNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  (void)mmdlGenPowOnOffSetupSrSet(pMsg);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Power OnOff Setup Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenPowOnOffSetupSrHandleSet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  if (mmdlGenPowOnOffSetupSrSet(pMsg))
  {
    /* Send Status message as a response to the Set message */
    MmdlGenPowOnOffSrSendStatus(MMDL_GEN_POWER_ONOFFSETUP_SR_MDL_ID, pMsg->elementId, pMsg->srcAddr,
                                pMsg->appKeyIndex);
  }
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Generic Power OnOff Setup Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffSetupSrInit(void)
{
  MMDL_TRACE_INFO0("GEN POWER ONOFF SETUP SR: init");

  /* Set event callbacks */
  powOnOffSetupSrCb.recvCback = MmdlEmptyCback;
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Generic Power OnOff Setup Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Generic Power OnOff Setup Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffSetupSrHandlerInit(wsfHandlerId_t handlerId)
{
  /* Set handler ID */
  mmdlGenPowOnOffSetupSrHandlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Generic Power OnOff Setup Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffSetupSrHandler(wsfMsgHdr_t *pMsg)
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
        if (MESH_OPCODE_SIZE(pModelMsg->opCode) == MMDL_GEN_POWER_ONOFF_OPCODES_SIZE)
        {
          /* Match the received opcode */
          for (opcodeIdx = 0; opcodeIdx < MMDL_GEN_POWER_ONOFFSETUP_SR_NUM_RCVD_OPCODES; opcodeIdx++)
          {
            if (!memcmp(&mmdlGenPowOnOffSetupSrRcvdOpcodes[opcodeIdx], pModelMsg->opCode.opcodeBytes,
                        MMDL_GEN_POWER_ONOFF_OPCODES_SIZE))
            {
              /* Process message */
              (void)mmdlGenPowOnOffSetupSrHandleMsg[opcodeIdx](pModelMsg);
            }
          }
        }
        break;

      default:
        MMDL_TRACE_WARN0("GEN POWER ONOFF SETUP SR: Invalid event message received!");
        break;
    }
  }
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
void MmdlGenPowOnOffSetupSrRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback */
  if (recvCback != NULL)
  {
    powOnOffSetupSrCb.recvCback = recvCback;
  }
}
