/*************************************************************************************************/
/*!
 *  \file   mmdl_schedurel_setup_sr.c
 *
 *  \brief  Implementation of the Scheduler Setup Server model.
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
#include "mmdl_common.h"
#include "mesh_api.h"

#include "mmdl_types.h"
#include "mmdl_defs.h"
#include "mmdl_bindings.h"
#include "mmdl_gen_default_trans_sr.h"
#include "mmdl_scheduler_sr_api.h"
#include "mmdl_scheduler_sr_main.h"
#include "mmdl_scheduler_setup_sr.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Scheduler Setup Server message handler type definition */
typedef void (*mmdlSchedulerSetupSrHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Supported opcodes */
const meshMsgOpcode_t mmdlSchedulerSetupSrRcvdOpcodes[MMDL_SCHEDULER_SETUP_SR_NUM_RCVD_OPCODES] =
{
  {{ UINT8_OPCODE_TO_BYTES(MMDL_SCHEDULER_ACTION_SET_OPCODE) }},
  {{ UINT8_OPCODE_TO_BYTES(MMDL_SCHEDULER_ACTION_SET_NO_ACK_OPCODE) }},
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlSchedulerSetupSrHandleMsg_t mmdlSchedulerSetupSrHandleMsg[MMDL_SCHEDULER_SETUP_SR_NUM_RCVD_OPCODES] =
{
  mmdlSchedulerSetupSrHandleActionSet,
  mmdlSchedulerSetupSrHandleActionSetNoAck
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/
/*************************************************************************************************/
/*!
 *  \brief     Validates unpacked Action Set or Set Unacknowledged parameters.
 *
 *  \param[in] index   Scheduler Register State entry index.
 *  \param[in] pParam  Pointer to Scheduler Register State entry values.
 *
 *  \return    TRUE if parameters are valid.
 */
/*************************************************************************************************/
static inline bool_t mmdlSchedulerSetupIsValidActionSet(uint8_t index,
                                                        mmdlSchedulerRegisterEntry_t *pParam)
{
  /* Validate parameters. */
  if((index > MMDL_SCHEDULER_REGISTER_ENTRY_MAX) ||
     (pParam == NULL) ||
     (pParam->year > MMDL_SCHEDULER_REGISTER_YEAR_ALL) ||
     (pParam->months >= MMDL_SCHEDULER_SCHED_IN_PROHIBITED_START) ||
     (pParam->day > MMDL_SCHEDULER_DAY_LAST) ||
     (pParam->hour >= MMDL_SCHEDULER_HOUR_PROHIBITED_START) ||
     (pParam->minute >= MMDL_SCHEDULER_MINUTE_PROHIBITED_START) ||
     (pParam->second >= MMDL_SCHEDULER_SECOND_PROHIBITED_START) ||
     (pParam->daysOfWeek >= MMDL_SCHEDULER_SCHED_ON_PROHIBITED_START) ||
     (MMDL_SCHEDULER_ACTION_IS_RFU(pParam->action)) ||
     ((pParam->action == MMDL_SCHEDULER_ACTION_SCENE_RECALL) &&
      (pParam->sceneNumber == MMDL_SCENE_NUM_PROHIBITED)))
  {
    return FALSE;
  }

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles Scheduler Action Set and Set Unacknowledged commands.
 *
 *  \param[in] pMsg     Received model message.
 *  \param[in] sendAck  TRUE if an Action Status message should be sent.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlSchedulerSetupHandleAction(const meshModelMsgRecvEvt_t *pMsg, bool_t sendAck)
{
  mmdlSchedulerSrDesc_t *pDesc;
  mmdlSchedulerRegisterEntry_t regEntry;
  uint8_t index;

  /* Check if descriptor exists. */
  mmdlSchedulerSrGetDesc(pMsg->elementId, &pDesc);
  if(pDesc == NULL)
  {
    return;
  }

  /* Unpack parameters. */
  mmdlSchedulerUnpackActionParams(pMsg->pMessageParams, &index, &regEntry);

  /* Check if values are valid. */
  if(mmdlSchedulerSetupIsValidActionSet(index, &regEntry))
  {
    /* Update entry. */
    pDesc->registerState[index].regEntry = regEntry;

    /* Send status if needed. */
    if(sendAck)
    {
      mmdlSchedulerSrSendActionStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                                      pMsg->recvOnUnicast, index);
    }

    /* Schedule entry. */
    mmdlSchedulerSrScheduleEvent(pMsg->elementId, index, &(pDesc->registerState[index]));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Scheduler Action Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlSchedulerSetupSrHandleActionSet(const meshModelMsgRecvEvt_t *pMsg)
{
  if(pMsg->messageParamsLen != MMDL_SCHEDULER_ACTION_SET_LEN)
  {
    return;
  }
  mmdlSchedulerSetupHandleAction(pMsg, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Scheduler Action Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlSchedulerSetupSrHandleActionSetNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  if(pMsg->messageParamsLen != MMDL_SCHEDULER_ACTION_SET_NO_ACK_LEN)
  {
    return;
  }
  mmdlSchedulerSetupHandleAction(pMsg, FALSE);
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Scheduler Setup Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerSetupSrHandler(wsfMsgHdr_t *pMsg)
{
  meshModelEvt_t *pModelMsg;
  uint8_t opcodeIdx;

  /* Handle message */
  if (pMsg != NULL)
  {
    switch (pMsg->event)
    {
      case MESH_MODEL_EVT_MSG_RECV:
        pModelMsg = (meshModelEvt_t *)pMsg;

        /* Validate opcode size and value */
        if (MESH_OPCODE_SIZE(pModelMsg->msgRecvEvt.opCode) == 1)
        {
          /* Match the received opcode */
          for (opcodeIdx = 0; opcodeIdx < MMDL_SCHEDULER_SETUP_SR_NUM_RCVD_OPCODES; opcodeIdx++)
          {
            if (mmdlSchedulerSetupSrRcvdOpcodes[opcodeIdx].opcodeBytes[0] == pModelMsg->msgRecvEvt.opCode.opcodeBytes[0])
            {
              /* Process message */
              mmdlSchedulerSetupSrHandleMsg[opcodeIdx]((const meshModelMsgRecvEvt_t *)pModelMsg);
            }
          }
        }
        break;

      default:
        MMDL_TRACE_WARN0("SCHEDULER SETUP SR: Invalid event message received!");
        break;
    }
  }
}
