/*************************************************************************************************/
/*!
 *  \file   mmdl_gen_level_sr_main.c
 *
 *  \brief  Implementation of the Generic Level Server model.
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
#include "mmdl_bindings.h"

#include "mmdl_gen_level_sr.h"
#include "mmdl_gen_default_trans_sr.h"
#include "mmdl_gen_level_sr_main.h"
#include "mmdl_gen_level_sr_api.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Present state index in stored states */
#define PRESENT_STATE_IDX                   0

/*! Target state index in stored states */
#define TARGET_STATE_IDX                    1

/*! Scene states start index in stored states */
#define SCENE_STATE_IDX                     2

/*  Timeout for filtering duplicate messages from same source */
#define MSG_RCVD_TIMEOUT_MS                 6000

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Generic Level Server control block type definition */
typedef struct mmdlGenLevelSrCb_tag
{
  mmdlSceneStore_t          fStoreScene;      /*!< Pointer to the function that stores
                                               *    a scene on the model instance
                                               */
  mmdlSceneRecall_t         fRecallScene;     /*!< Pointer to the function that recalls
                                               *    a scene on the model instance
                                               */
  mmdlBindResolve_t         fResolveBind;     /*!< Pointer to the function that checks
                                               *   and resolves a bind triggered by a
                                               *   change in this model instance
                                               */
  mmdlEventCback_t          recvCback;        /*!< Model Generic Level received callback */
} mmdlGenLevelSrCb_t;

/*! Generic Level Server message handler type definition */
typedef void (*mmdlGenLevelSrHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler ID */
wsfHandlerId_t mmdlGenLevelSrHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlGenLevelSrRcvdOpcodes[MMDL_GEN_LEVEL_SR_NUM_RCVD_OPCODES] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_LEVEL_GET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_LEVEL_SET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_LEVEL_SET_NO_ACK_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_LEVEL_DELTA_SET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_LEVEL_DELTA_SET_NO_ACK_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_LEVEL_MOVE_SET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_LEVEL_MOVE_SET_NO_ACK_OPCODE)} }
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlGenLevelSrHandleMsg_t mmdlGenLevelSrHandleMsg[MMDL_GEN_LEVEL_SR_NUM_RCVD_OPCODES] =
{
  mmdlGenLevelSrHandleGet,
  mmdlGenLevelSrHandleSet,
  mmdlGenLevelSrHandleSetNoAck,
  mmdlGenLevelSrHandleDeltaSet,
  mmdlGenLevelSrHandleDeltaSetNoAck,
  mmdlGenLevelSrHandleMoveSet,
  mmdlGenLevelSrHandleMoveSetNoAck,
};

/*! Generic Level Server Control Block */
static mmdlGenLevelSrCb_t  levelSrCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Searches for the Generic Level model instance descriptor on the specified element.
 *
 *  \param[in]  elementId  Identifier of the Element implementing the model.
 *  \param[out] ppOutDesc  Double pointer to the descriptor.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void mmdlGenLevelSrGetDesc(meshElementId_t elementId, mmdlGenLevelSrDesc_t **ppOutDesc)
{
  uint8_t modelIdx;

  *ppOutDesc = NULL;

  /* Check if element exists. */
  if (elementId >= pMeshConfig->elementArrayLen)
  {
    return;
  }

  /* Look for the model instance */
  for (modelIdx = 0; modelIdx < pMeshConfig->pElementArray[elementId].numSigModels; modelIdx ++)
  {
    if (pMeshConfig->pElementArray[elementId].pSigModelArray[modelIdx].modelId ==
        MMDL_GEN_LEVEL_SR_MDL_ID)
    {
      /* Matching model ID on elementId */
      *ppOutDesc = pMeshConfig->pElementArray[elementId].pSigModelArray[modelIdx].pModelDescriptor;
      break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local present state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model.
 *  \param[in] pDesc           Pointer to the model descriptor
 *  \param[in] targetState     Target State for this transaction. See ::mmdlGenLevelState_t.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenLevelSrSetPresentState(meshElementId_t elementId, mmdlGenLevelSrDesc_t *pDesc,
                                          mmdlGenLevelState_t targetState,
                                          mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlGenLevelSrEvent_t event;

  /* Update State */
  pDesc->pStoredStates[PRESENT_STATE_IDX] = targetState;

  /* Check for bindings on this state. Trigger bindings */
  if ((stateUpdateSrc != MMDL_STATE_UPDATED_BY_BIND) &&
      (stateUpdateSrc != MMDL_STATE_UPDATED_BY_SCENE) && (levelSrCb.fResolveBind))
  {
    levelSrCb.fResolveBind(elementId, MMDL_STATE_GEN_LEVEL,
                           &pDesc->pStoredStates[PRESENT_STATE_IDX]);
  }

  /* Publish state change */
  MmdlGenLevelSrPublish(elementId);

  /* Set event type */
  event.hdr.status = MMDL_SUCCESS;
  event.hdr.event = MMDL_GEN_LEVEL_SR_EVENT;
  event.hdr.param = MMDL_GEN_LEVEL_SR_STATE_UPDATE_EVENT;

  /* Set event parameters */
  event.statusEvent.elemId = elementId;
  event.statusEvent.state = targetState;
  event.statusEvent.stateUpdateSource = stateUpdateSrc;

  /* Send event to the upper layer */
  levelSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model.
 *  \param[in] targetState     Target State for this transaction. See ::mmdlGenLevelState_t.
 *  \param[in] transitionMs    Transition time for this transaction in ms.
 *  \param[in] delay5Ms        Delay for executing transaction. Unit is 5ms.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenLevelSrSetState(meshElementId_t elementId, mmdlGenLevelState_t targetState,
                                   uint32_t transitionMs, uint8_t delay5Ms,
                                   mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlGenLevelSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO3("GEN LEVEL SR: Set Target=%d, TimeRem=%d ms, Delay=0x%X",
                    targetState, transitionMs, delay5Ms);

  /* Get model instance descriptor */
  mmdlGenLevelSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Update descriptor */
    pDesc->remainingTimeMs = transitionMs;
    pDesc->delay5Ms = delay5Ms;
    pDesc->updateSource = stateUpdateSrc;

    /* Update Target State */
    pDesc->pStoredStates[TARGET_STATE_IDX] = targetState;

    /* Check if the set is delayed */
    if (pDesc->delay5Ms > 0)
    {
      /* Start Timer */
      WsfTimerStartMs(&pDesc->transitionTimer, DELAY_5MS_TO_MS(pDesc->delay5Ms));
    }
    /* Check if state will change after a transition or immediately */
    else if (pDesc->remainingTimeMs > 0)
    {
      /* Start Timer */
      if (pDesc->steps > 0)
      {
        /* If transition is divided into steps, use defined timer update interval */
        WsfTimerStartMs(&pDesc->transitionTimer, MMDL_TRANSITION_STATE_UPDATE_INTERVAL);
      }
      else
      {
        WsfTimerStartMs(&pDesc->transitionTimer, pDesc->remainingTimeMs);
      }
    }
    else
    {
      /* Stop transition */
      if (pDesc->transitionTimer.isStarted)
      {
        WsfTimerStop(&pDesc->transitionTimer);
      }

      mmdlGenLevelSrSetPresentState(elementId, pDesc, targetState, stateUpdateSrc);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Generic Level Status command to the specified destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  Indicates if message that triggered the status was received on unicast.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenLevelSrSendStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                     uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_LEVEL_SR_MDL_ID, MMDL_GEN_LEVEL_STATUS_OPCODE);
  uint8_t msgParams[MMDL_GEN_LEVEL_STATUS_MAX_LEN];
  mmdlGenLevelSrDesc_t *pDesc = NULL;
  uint8_t *pParams;
  uint8_t tranTime;

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = dstAddr;
  msgInfo.ttl = MESH_USE_DEFAULT_TTL;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Get the model instance descriptor */
  mmdlGenLevelSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    pParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT16_TO_BSTREAM(pParams, pDesc->pStoredStates[PRESENT_STATE_IDX]);

    if (pDesc->remainingTimeMs != 0)
    {
      UINT16_TO_BSTREAM(pParams, pDesc->pStoredStates[TARGET_STATE_IDX]);

      if (pDesc->isMoveSet)
      {
        UINT8_TO_BSTREAM(pParams, MMDL_GEN_TR_UNKNOWN);
      }
      else
      {
        /* Timer is running the transition */
        if (pDesc->steps > 0)
        {
          /* Transition is divided into steps. Compute remaining time based on remaining steps. */
          tranTime = MmdlGenDefaultTimeMsToTransTime(pDesc->transitionTimer.ticks * WSF_MS_PER_TICK +
                     (pDesc->steps - 1) * MMDL_TRANSITION_STATE_UPDATE_INTERVAL);
        }
        else
        {
          tranTime = MmdlGenDefaultTimeMsToTransTime(pDesc->transitionTimer.ticks * WSF_MS_PER_TICK);
        }
        UINT8_TO_BSTREAM(pParams, tranTime);
      }

      MMDL_TRACE_INFO3("GEN LEVEL SR: Send Status Present=0x%X, Target=0x%X, TimeRem=0x%X",
                        pDesc->pStoredStates[PRESENT_STATE_IDX],
                        pDesc->pStoredStates[TARGET_STATE_IDX], pDesc->remainingTimeMs);
    }
    else
    {
      MMDL_TRACE_INFO1("GEN LEVEL SR: Send Status Present=0x%X",
                       pDesc->pStoredStates[PRESENT_STATE_IDX]);
    }

    /* Send message to the Mesh Core */
    MeshSendMessage(&msgInfo, msgParams, (uint16_t)(pParams - msgParams),
                    MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                    MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Level Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenLevelSrHandleGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    mmdlGenLevelSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Level Set Unacknowledged command.
 *
 *  \param[in] pMsg         Received model message.
 *  \param[in] ackRequired  TRUE if acknowledgement is required in response,  FALSE otherwise.
 *
 *  \return    TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlGenLevelSrProcessSet(const meshModelMsgRecvEvt_t *pMsg,  bool_t ackRequired)
{
  mmdlGenLevelState_t state;
  mmdlGenLevelSrDesc_t *pDesc = NULL;
  uint8_t tid;
  uint32_t transMs;
  uint32_t delayMs;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length. It can take only min and max values. */
  if (pMsg->messageParamsLen != MMDL_GEN_LEVEL_SET_MAX_LEN &&
      pMsg->messageParamsLen != MMDL_GEN_LEVEL_SET_MIN_LEN)
  {
    return FALSE;
  }

  /* Set the state value from pMessageParams buffer. */
  BYTES_TO_UINT16(state, &pMsg->pMessageParams[0]);

  /* Check if it contains optional parameters */
  if (pMsg->messageParamsLen == MMDL_GEN_LEVEL_SET_MAX_LEN)
  {
    /* Check prohibited values for Transition Time */
    if (TRANSITION_TIME_STEPS(pMsg->pMessageParams[MMDL_GEN_LEVEL_SET_TRANSITION_IDX]) ==
        MMDL_GEN_TR_UNKNOWN)
    {
      return FALSE;
    }

    /* Get Transition time */
    transMs = MmdlGenDefaultTransTimeToMs(pMsg->pMessageParams[MMDL_GEN_LEVEL_SET_TRANSITION_IDX]);
    delayMs = pMsg->pMessageParams[MMDL_GEN_LEVEL_SET_DELAY_IDX];
  }
  else
  {
    /* Get Default Transition time */
    transMs = MmdlGenDefaultTransGetTime(pMsg->elementId);
    delayMs = 0;
  }

  /* Get the model instance descriptor */
  mmdlGenLevelSrGetDesc(pMsg->elementId, &pDesc);

  if (pDesc != NULL)
  {
    WsfTimerStartMs(&pDesc->msgRcvdTimer, MSG_RCVD_TIMEOUT_MS);

    /* Get Transaction ID */
    tid = pMsg->pMessageParams[MMDL_GEN_LEVEL_SET_TID_IDX];

    /* Validate message against last transaction */
    if ((pMsg->srcAddr == pDesc->srcAddr) && (tid == pDesc->transactionId))
    {
      return FALSE;
    }

    /* Update last transaction fields and restart 6 seconds timer */
    pDesc->ackPending = ackRequired;
    pDesc->srcAddr = pMsg->srcAddr;
    pDesc->transactionId = tid;
    pDesc->ackAppKeyIndex = pMsg->appKeyIndex;
    pDesc->ackForUnicast = pMsg->recvOnUnicast;
    pDesc->isMoveSet = FALSE;

    /* Determine the number of transition steps */
    pDesc->steps = transMs / MMDL_TRANSITION_STATE_UPDATE_INTERVAL;

    if (pDesc->steps > 0)
    {
      /* Compute the transition step increment */
      pDesc->transitionStep = (state - pDesc->pStoredStates[PRESENT_STATE_IDX]) / pDesc->steps;
    }

    /* Change state */
    mmdlGenLevelSrSetState(pMsg->elementId, state, transMs, delayMs, MMDL_STATE_UPDATED_BY_CL);

    return (pDesc->delay5Ms == 0);
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Processes Generic Level Move Set commands.
 *
 *  \param[in] pMsg         Received model message.
 *  \param[in] ackRequired  TRUE if acknowledgement is required in response,  FALSE otherwise.
 *
 *  \return    TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlGenLevelSrProcessMoveSet(const meshModelMsgRecvEvt_t *pMsg, bool_t ackRequired)
{
  mmdlGenLevelState_t deltaLevel;
  mmdlGenLevelState_t targetState;
  mmdlGenLevelSrDesc_t *pDesc = NULL;
  uint8_t tid;
  uint32_t transitionTimeMs;
  uint32_t updateIntervalMs;
  uint32_t delayMs;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length. It can take only min and max values. */
  if (pMsg->messageParamsLen != MMDL_GEN_LEVEL_SET_MAX_LEN &&
    pMsg->messageParamsLen != MMDL_GEN_LEVEL_SET_MIN_LEN)
  {
    return FALSE;
  }

  /* Check if it contains optional parameters */
  if (pMsg->messageParamsLen == MMDL_GEN_LEVEL_SET_MAX_LEN)
  {
    /* Check prohibited values for Transition Time and if Transition
     * Time is set to 0. */
    if ((TRANSITION_TIME_STEPS(pMsg->pMessageParams[MMDL_GEN_LEVEL_SET_TRANSITION_IDX]) ==
        MMDL_GEN_TR_UNKNOWN) || (pMsg->pMessageParams[MMDL_GEN_LEVEL_SET_TRANSITION_IDX] ==
        0))
    {
      return FALSE;
    }
  }

  /* Get the model instance descriptor */
  mmdlGenLevelSrGetDesc(pMsg->elementId, &pDesc);

  if (pDesc != NULL)
  {
    WsfTimerStartMs(&pDesc->msgRcvdTimer, MSG_RCVD_TIMEOUT_MS);

    /* Get Transaction ID */
    tid = pMsg->pMessageParams[MMDL_GEN_LEVEL_SET_TID_IDX];

    /* Validate message against last transaction */
    if ((pMsg->srcAddr == pDesc->srcAddr) && (tid == pDesc->transactionId))
    {
      return FALSE;
    }

    /* Update last transaction fields. No 6 seconds timer */
    pDesc->ackPending = ackRequired;
    pDesc->srcAddr = pMsg->srcAddr;
    pDesc->transactionId = tid;
    pDesc->ackAppKeyIndex = pMsg->appKeyIndex;
    pDesc->ackForUnicast = pMsg->recvOnUnicast;

    /* Set the state value from pMessageParams buffer. */
    BYTES_TO_UINT16(deltaLevel, &pMsg->pMessageParams[0]);

    /* Stop changing the Generic Level state when delta is 0 */
    if ((deltaLevel == 0) && pDesc->transitionTimer.isStarted)
    {
      WsfTimerStop(&pDesc->transitionTimer);
      pDesc->remainingTimeMs = 0;
      pDesc->isMoveSet = FALSE;
      return TRUE;
    }

    /* Check if it contains optional parameters */
    if (pMsg->messageParamsLen == MMDL_GEN_LEVEL_SET_MAX_LEN)
    {
      /* Use transition time from the received packet. */
      transitionTimeMs = MmdlGenDefaultTransTimeToMs(pMsg->pMessageParams[MMDL_GEN_LEVEL_SET_TRANSITION_IDX]);
      delayMs = pMsg->pMessageParams[MMDL_GEN_LEVEL_SET_DELAY_IDX];
    }
    else
    {
      /* Else use default transition time */
      transitionTimeMs = MmdlGenDefaultTransGetTime(pMsg->elementId);
      delayMs = 0;
    }

    if (transitionTimeMs == 0)
    {
      /* No Transition so no need to initiate state change. */
      pDesc->isMoveSet = FALSE;
      return TRUE;
    }

    /* Set the isMoveSet flag and the deltaLevelStep to
     * handle the move set behavior in mmdlGenLevelSrHandleTmrCback.
     */
    pDesc->isMoveSet = TRUE;
    pDesc->deltaLevelStep = deltaLevel * MMDL_GEN_LEVEL_MOVE_UPDATE_INTERVAL / transitionTimeMs;

    /* Check if the transition speed is zero.
     * This may happen if the transition time is much more greater than the delta level.
     */
    if (pDesc->deltaLevelStep != 0)
    {
      /* Non-zero transition speed. Use default timer interval. */
      updateIntervalMs = MMDL_GEN_LEVEL_MOVE_UPDATE_INTERVAL;
    }
    else
    {
      /* The resulted transition speed is zero. Adjust transition timer interval. */
      pDesc->deltaLevelStep = 1;
      updateIntervalMs = transitionTimeMs / deltaLevel;
    }

    /* Determine the target state value. */
    if (deltaLevel < 0)
    {
      targetState = MMDL_GEN_LEVEL_MIN_SIGNED_LEVEL;
    }
    else
    {
      targetState = MMDL_GEN_LEVEL_MAX_SIGNED_LEVEL;
    }

    /* Change state */
    mmdlGenLevelSrSetState(pMsg->elementId, targetState, updateIntervalMs, delayMs,
                           MMDL_STATE_UPDATED_BY_CL);

    return (pDesc->delay5Ms == 0);
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Processes Generic Level Delta Set commands.
 *
 *  \param[in] pMsg         Received model message.
 *  \param[in] ackRequired  TRUE if acknowledgement is required in response,  FALSE otherwise.
 *  \return    TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlGenLevelSrProcessDeltaSet(const meshModelMsgRecvEvt_t *pMsg, bool_t ackRequired)
{
  int32_t delta;
  mmdlGenLevelState_t targetState;
  mmdlGenLevelSrDesc_t *pDesc = NULL;
  uint8_t tid;
  uint32_t transMs;
  uint32_t delayMs;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length. It can take only min and max values. */
  if (pMsg->messageParamsLen != MMDL_GEN_LEVEL_DELTA_SET_MAX_LEN &&
      pMsg->messageParamsLen != MMDL_GEN_LEVEL_DELTA_SET_MIN_LEN)
  {
    return FALSE;
  }

  /* Check if it contains optional parameters */
  if (pMsg->messageParamsLen == MMDL_GEN_LEVEL_DELTA_SET_MAX_LEN)
  {
    /* Check prohibited values for Transition Time */
    if (TRANSITION_TIME_STEPS(pMsg->pMessageParams[MMDL_GEN_LEVEL_DELTA_SET_TRANSITION_IDX]) ==
                              MMDL_GEN_TR_UNKNOWN)
    {
      return FALSE;
    }
  }

  /* Get the model instance descriptor */
  mmdlGenLevelSrGetDesc(pMsg->elementId, &pDesc);

  if (pDesc != NULL)
  {
    WsfTimerStartMs(&pDesc->msgRcvdTimer, MSG_RCVD_TIMEOUT_MS);

    /* Get Transaction ID */
    tid = pMsg->pMessageParams[MMDL_GEN_LEVEL_DELTA_SET_TID_IDX];

    /* Validate message against last transaction */
    if (!((pMsg->srcAddr == pDesc->srcAddr) && (tid == pDesc->transactionId)))
    {
      /* New transaction */
      pDesc->initialState = pDesc->pStoredStates[PRESENT_STATE_IDX];
    }

    /* Update last transaction fields. No 6 seconds timer for Delta */
    pDesc->ackPending = ackRequired;
    pDesc->srcAddr = pMsg->srcAddr;
    pDesc->transactionId = tid;
    pDesc->ackAppKeyIndex = pMsg->appKeyIndex;
    pDesc->ackForUnicast = pMsg->recvOnUnicast;
    pDesc->isMoveSet = FALSE;

    /* Set the state value from pMessageParams buffer. */
    BYTES_TO_UINT32(delta, &pMsg->pMessageParams[0]);

    /* When calculating the targetState the limitation behavior
     * is implemented.
     */
    if (delta > 0)
    {
      if (delta + (int32_t)(pDesc->initialState) > MMDL_GEN_LEVEL_MAX_SIGNED_LEVEL)
      {
        targetState = MMDL_GEN_LEVEL_MAX_SIGNED_LEVEL;
      }
      else
      {
        targetState = pDesc->initialState + (int16_t)delta;
      }
    }
    else
    {
      if (delta + (int32_t)(pDesc->initialState) < (int16_t)MMDL_GEN_LEVEL_MIN_SIGNED_LEVEL)
      {
        targetState = MMDL_GEN_LEVEL_MIN_SIGNED_LEVEL;
      }
      else
      {
        targetState = pDesc->initialState + (int16_t)delta;
      }
    }

    /* Check if it contains optional parameters */
    if (pMsg->messageParamsLen == MMDL_GEN_LEVEL_DELTA_SET_MAX_LEN)
    {
      /* Get Transition time */
      transMs = MmdlGenDefaultTransTimeToMs(pMsg->pMessageParams[MMDL_GEN_LEVEL_DELTA_SET_TRANSITION_IDX]);
      delayMs = pMsg->pMessageParams[MMDL_GEN_LEVEL_DELTA_SET_DELAY_IDX];
    }
    else
    {
      /* Get Default Transition time */
      transMs = MmdlGenDefaultTransGetTime(pMsg->elementId);
      delayMs = 0;
    }

    /* Determine the number of transition steps */
    pDesc->steps = transMs / MMDL_TRANSITION_STATE_UPDATE_INTERVAL;

    if (pDesc->steps > 0)
    {
      /* Compute the transition step increment */
      pDesc->transitionStep = (targetState - pDesc->pStoredStates[PRESENT_STATE_IDX]) / pDesc->steps;
    }

    /* Change state */
    mmdlGenLevelSrSetState(pMsg->elementId, targetState, transMs, delayMs,
                           MMDL_STATE_UPDATED_BY_CL);

    return (pDesc->delay5Ms == 0);
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Level Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenLevelSrHandleSet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  if (mmdlGenLevelSrProcessSet(pMsg, TRUE))
  {
    /* Send Status message as a response to the Set message */
    mmdlGenLevelSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Level Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenLevelSrHandleSetNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  (void)mmdlGenLevelSrProcessSet(pMsg, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Level Delta Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenLevelSrHandleDeltaSet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  if (mmdlGenLevelSrProcessDeltaSet(pMsg, TRUE))
  {
    /* Send Status message as a response to the Set message */
    mmdlGenLevelSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Level Delta Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenLevelSrHandleDeltaSetNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  mmdlGenLevelSrProcessDeltaSet(pMsg, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Level Move Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenLevelSrHandleMoveSet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  if (mmdlGenLevelSrProcessMoveSet(pMsg, TRUE))
  {
    /* Send Status message as a response to the Set message */
    mmdlGenLevelSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Level Move Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenLevelSrHandleMoveSetNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  (void)mmdlGenLevelSrProcessMoveSet(pMsg, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Generic Level Server timer callback.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenLevelSrHandleTmrCback(uint8_t elementId)
{
  mmdlGenLevelSrDesc_t *pDesc = NULL;
  uint32_t remainingTimeMs;
  int32_t target;

  /* Get model instance descriptor */
  mmdlGenLevelSrGetDesc(elementId, &pDesc);

  /* Transition timeout. Move to Target State. */
  if (pDesc != NULL)
  {
    if (pDesc->delay5Ms != 0)
    {
      /* Reset Delay */
      pDesc->delay5Ms = 0;

      /* Timeout. Set state. */
      mmdlGenLevelSrSetState(elementId, pDesc->pStoredStates[TARGET_STATE_IDX],
                             pDesc->remainingTimeMs, 0, pDesc->updateSource);

      /* Send Status if it was a delayed Acknowledged Set */
      if (pDesc->ackPending)
      {
        mmdlGenLevelSrSendStatus(elementId, pDesc->srcAddr, pDesc->ackAppKeyIndex,
                               pDesc->ackForUnicast);
      }
    }
    else if (pDesc->remainingTimeMs != 0)
    {
      /* Keep transition time */
      remainingTimeMs = pDesc->remainingTimeMs;

      if (pDesc->steps > 0)
      {
        /* Transition is divided into steps. Decrement the remaining time and steps */
        pDesc->steps--;
        remainingTimeMs -= MMDL_TRANSITION_STATE_UPDATE_INTERVAL;

        /* Compute intermediate state value */
        target = pDesc->pStoredStates[PRESENT_STATE_IDX] + pDesc->transitionStep;

        /* Update present state only. */
        mmdlGenLevelSrSetPresentState(elementId, pDesc, target, pDesc->updateSource);

        if (pDesc->steps == 1)
        {
          /* Next is the last step.
           * Program the remaining time (can be more than MMDL_TRANSITION_STATE_UPDATE_INTERVAL).
           * Also, the last step increment can be greater then the intermediate ones.
           */
          pDesc->steps = 0;
        }

        /* Program next transition */
        mmdlGenLevelSrSetState(elementId, pDesc->pStoredStates[TARGET_STATE_IDX],
                               remainingTimeMs, 0, pDesc->updateSource);
      }
      else
      {
        /* To handle Move Set behavior, after the new state value verify
         * that new level state value does not exceed the max delta value
         * or the min delta value.
         */
        if (pDesc->isMoveSet)
        {
          /* Add the delta level step value to the present level value. */
          target = pDesc->pStoredStates[PRESENT_STATE_IDX] + pDesc->deltaLevelStep;

          if (target > MMDL_GEN_LEVEL_MAX_SIGNED_LEVEL)
          {
            target = MMDL_GEN_LEVEL_MAX_SIGNED_LEVEL;
          }
          else if (target < (int16_t)MMDL_GEN_LEVEL_MIN_SIGNED_LEVEL)
          {
            target = MMDL_GEN_LEVEL_MIN_SIGNED_LEVEL;
          }
        }
        else
        {
          target = pDesc->pStoredStates[TARGET_STATE_IDX];
        }

        /* Transition timeout. Move to Target State. */
        mmdlGenLevelSrSetState(elementId, target, 0, 0, pDesc->updateSource);

        /* Restart transition for Move if there is non zero velocity*/
        if ((pDesc->isMoveSet) &&
            (pDesc->pStoredStates[TARGET_STATE_IDX] != pDesc->pStoredStates[PRESENT_STATE_IDX]))
        {
          mmdlGenLevelSrSetState(elementId, pDesc->pStoredStates[TARGET_STATE_IDX], remainingTimeMs,
                                 0, pDesc->updateSource);
        }
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Generic Level Server message received timer callback.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenLevelSrHandleMsgRcvdTmrCback(uint8_t elementId)
{
  mmdlGenLevelSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlGenLevelSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Reset source address and transaction ID for last stored transaction */
    pDesc->srcAddr = MESH_ADDR_TYPE_UNASSIGNED;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Stores the present state in a scene.
 *
 *  \param[in] pDesc    Model instance descriptor.
 *  \param[in] sceneIdx Identifier of the stored scene.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenLevelSrStoreScene(void *pDesc, uint8_t sceneIdx)
{
  mmdlGenLevelSrDesc_t *pGenLevelDesc = (mmdlGenLevelSrDesc_t *)pDesc;

  MMDL_TRACE_INFO1("GEN LEVEL SR: Store Level=%d", pGenLevelDesc->pStoredStates[PRESENT_STATE_IDX]);

  /* Store present state */
  pGenLevelDesc->pStoredStates[SCENE_STATE_IDX + sceneIdx] = pGenLevelDesc->pStoredStates[PRESENT_STATE_IDX];
}

/*************************************************************************************************/
/*!
 *  \brief     Recalls the scene.
 *
 *  \param[in] elementId     Identifier of the element implementing the model instance.
 *  \param[in] sceneIdx      Identifier of the stored scene.
 *  \param[in] transitionMs  Transition time in ms.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenLevelSrRecallScene(meshElementId_t elementId, uint8_t sceneIdx, uint32_t transitionMs)
{
  mmdlGenLevelSrDesc_t *pDesc = NULL;

  /* Get the model instance descriptor */
  mmdlGenLevelSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    MMDL_TRACE_INFO3("GEN LEVEL SR: Recall elemid=%d onoff=%d transMs=%d",
                     elementId, pDesc->pStoredStates[SCENE_STATE_IDX + sceneIdx], transitionMs);

    /* Recall state */
    mmdlGenLevelSrSetState(elementId, pDesc->pStoredStates[SCENE_STATE_IDX + sceneIdx], transitionMs, 0,
                           MMDL_STATE_UPDATED_BY_SCENE);
  }
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Generic Level Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlGenLevelSrInit(void)
{
  meshElementId_t elemId;
  mmdlGenLevelSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO0("GEN LEVEL SR: init");

  /* Set event callbacks */
  levelSrCb.recvCback = MmdlEmptyCback;
  levelSrCb.fResolveBind = MmdlBindResolve;
  levelSrCb.fRecallScene = mmdlGenLevelSrRecallScene;
  levelSrCb.fStoreScene = mmdlGenLevelSrStoreScene;

  /* Initialize timers */
  for(elemId = 0; elemId < pMeshConfig->elementArrayLen; elemId++)
  {
    /* Get the model instance descriptor */
    mmdlGenLevelSrGetDesc(elemId, &pDesc);

    if (pDesc != NULL)
    {
      pDesc->srcAddr = MESH_ADDR_TYPE_UNASSIGNED;

      /* Set transition timer parameters*/
      pDesc->transitionTimer.handlerId = mmdlGenLevelSrHandlerId;
      pDesc->transitionTimer.msg.event = MMDL_GEN_LEVEL_SR_EVT_TMR_CBACK;
      pDesc->transitionTimer.msg.param = elemId;

      /* Set msg Received timer parameters*/
      pDesc->msgRcvdTimer.handlerId = mmdlGenLevelSrHandlerId;
      pDesc->msgRcvdTimer.msg.event = MMDL_GEN_LEVEL_SR_MSG_RCVD_TMR_CBACK;
      pDesc->msgRcvdTimer.msg.param = elemId;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Generic Level Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Generic Level Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelSrHandlerInit(wsfHandlerId_t handlerId)
{
  mmdlGenLevelSrHandlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Generic Level Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelSrHandler(wsfMsgHdr_t *pMsg)
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
        if (MESH_OPCODE_SIZE(pModelMsg->msgRecvEvt.opCode) == MMDL_GEN_LEVEL_OPCODES_SIZE)
        {
          /* Match the received opcode */
          for (opcodeIdx = 0; opcodeIdx < MMDL_GEN_LEVEL_SR_NUM_RCVD_OPCODES; opcodeIdx++)
          {
            if (!memcmp(&mmdlGenLevelSrRcvdOpcodes[opcodeIdx],
                        pModelMsg->msgRecvEvt.opCode.opcodeBytes, MMDL_GEN_LEVEL_OPCODES_SIZE))
            {
              /* Process message */
              (void)mmdlGenLevelSrHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
            }
          }
        }
        break;

      case MESH_MODEL_EVT_PERIODIC_PUB:
        pModelMsg = (meshModelEvt_t *)pMsg;

        /* Check if periodic publishing was not disabled. */
        if(pModelMsg->periodicPubEvt.nextPubTimeMs != 0)
        {
        /* Publishing is requested part of the periodic publishing */
        MmdlGenLevelSrPublish(pModelMsg->periodicPubEvt.elementId);
        }
        break;

      case MMDL_GEN_LEVEL_SR_EVT_TMR_CBACK:
        mmdlGenLevelSrHandleTmrCback((meshElementId_t)pMsg->param);
        break;

      case MMDL_GEN_LEVEL_SR_MSG_RCVD_TMR_CBACK:
        mmdlGenLevelSrHandleMsgRcvdTmrCback((meshElementId_t)pMsg->param);
        break;

      default:
        MMDL_TRACE_WARN0("GEN LEVEL SR: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publish a GenLevel Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelSrPublish(meshElementId_t elementId)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_GEN_LEVEL_SR_MDL_ID,
                                             MMDL_GEN_LEVEL_STATUS_OPCODE);
  uint8_t msgParams[MMDL_GEN_LEVEL_STATUS_MAX_LEN];
  mmdlGenLevelSrDesc_t *pDesc = NULL;
  uint8_t *pParams, tranTime;

  /* Fill in the msg info parameters */
  pubMsgInfo.elementId = elementId;

  /* Get the model instance descriptor */
  mmdlGenLevelSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    pParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT16_TO_BSTREAM(pParams, pDesc->pStoredStates[PRESENT_STATE_IDX]);

    if (pDesc->remainingTimeMs > 0)
    {
      if (pDesc->steps > 0)
      {
        /* Transition is divided into steps. Compute remaining time based on remaining steps. */
        tranTime = MmdlGenDefaultTimeMsToTransTime(pDesc->transitionTimer.ticks * WSF_MS_PER_TICK +
                   (pDesc->steps - 1) * MMDL_TRANSITION_STATE_UPDATE_INTERVAL);
      }
      else
      {
        tranTime = MmdlGenDefaultTimeMsToTransTime(pDesc->transitionTimer.ticks * WSF_MS_PER_TICK);
      }

      UINT16_TO_BSTREAM(pParams, pDesc->pStoredStates[TARGET_STATE_IDX]);
      UINT8_TO_BSTREAM(pParams, tranTime);
      MMDL_TRACE_INFO3("GEN LEVEL SR: Publish Present=0x%X, Target=0x%X, TimeRem=0x%X",
                       pDesc->pStoredStates[PRESENT_STATE_IDX],
                       pDesc->pStoredStates[TARGET_STATE_IDX], tranTime);
    }
    else
    {
      MMDL_TRACE_INFO1("GEN LEVEL SR: Publish Present=0x%X",
                        pDesc->pStoredStates[PRESENT_STATE_IDX]);
    }

    /* Send message to the Mesh Core */
    MeshPublishMessage(&pubMsgInfo, msgParams, (uint16_t)(pParams - msgParams));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the Generic Level state of the element.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] targetState  Target State for this transaction. See ::mmdlGenLevelState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelSrSetState(meshElementId_t elementId, mmdlGenLevelState_t targetState)
{
  /* Change state locally. No transition time or delay required. */
  mmdlGenLevelSrSetState(elementId, targetState, 0, 0, MMDL_STATE_UPDATED_BY_APP);
}

/*************************************************************************************************/
/*!
 *  \brief     Get the Generic Level state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelSrGetState(meshElementId_t elementId)
{
  mmdlGenLevelSrEvent_t event;
  mmdlGenLevelSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlGenLevelSrGetDesc(elementId, &pDesc);

  /* Set event type */
  event.hdr.event = MMDL_GEN_LEVEL_SR_EVENT;
  event.hdr.param = MMDL_GEN_LEVEL_SR_CURRENT_STATE_EVENT;

  /* Set event parameters */
  event.currentStateEvent.elemId = elementId;

  if (pDesc == NULL)
  {
    /* No descriptor found on element */
    event.hdr.status = MMDL_INVALID_ELEMENT;

    /* Zero out parameters */
    event.currentStateEvent.state = 0;
  }
  else
  {
    /* Descriptor found on element */
    event.hdr.status = MMDL_SUCCESS;

    /* Set event parameters */
    event.currentStateEvent.state = pDesc->pStoredStates[PRESENT_STATE_IDX];
  }

  /* Send event to the upper layer */
  levelSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local state as a result of a binding. The set is instantaneous.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] newState   New State. See ::mmdlGenLevelState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelSrSetBoundState(meshElementId_t elementId, mmdlGenLevelState_t newState)
{
  /* Change state locally. No transition time or delay is allowed. */
  mmdlGenLevelSrSetState(elementId, newState, 0, 0, MMDL_STATE_UPDATED_BY_BIND);
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the local states that can be stored in a scene.
 *
 *  \param[in] pDesc    Model instance descriptor.
 *  \param[in] sceneIdx Identifier of the stored scene.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelSrStoreScene(void *pDesc, uint8_t sceneIdx)
{
  if (levelSrCb.fStoreScene != NULL)
  {
    levelSrCb.fStoreScene(pDesc, sceneIdx);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the state according to the previously stored scene.
 *
 *  \param[in] elementId     Identifier of the element implementing the model instance.
 *  \param[in] sceneIdx      Identifier of the recalled scene.
 *  \param[in] transitionMs  Transition time in ms.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelSrRecallScene(meshElementId_t elementId, uint8_t sceneIdx, uint32_t transitionMs)
{
  if (levelSrCb.fRecallScene != NULL)
  {
    levelSrCb.fRecallScene(elementId, sceneIdx, transitionMs);
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
void MmdlGenLevelSrRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback*/
  if (recvCback != NULL)
  {
    levelSrCb.recvCback = recvCback;
  }
}
