/*************************************************************************************************/
/*!
 *  \file   mmdl_gen_power_level_sr_main.c
 *
 *  \brief  Implementation of the Generic Power Level Server model.
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

#include "mmdl_gen_default_trans_sr.h"
#include "mmdl_gen_onoff_sr.h"
#include "mmdl_gen_level_sr.h"
#include "mmdl_gen_powerlevel_sr.h"
#include "mmdl_gen_powerlevel_sr_main.h"
#include "mmdl_gen_powerlevel_sr_api.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*!  Timeout for filtering duplicate messages from same source */
#define MSG_RCVD_TIMEOUT_MS                  6000

/*! Generic Power Level Set Message TID index */
#define MMDL_SET_TID_IDX                     2

/*! Generic Power Level Set Message TID index */
#define MMDL_SET_TRANSITION_IDX              3

/*! Generic Power Level Set Message TID index */
#define MMDL_SET_DELAY_IDX                   4

/*! Present state index in stored states */
#define PRESENT_STATE_IDX                    0

/*! Target state index in stored states */
#define TARGET_STATE_IDX                     1

/*! Last state index in stored states */
#define LAST_STATE_IDX                       2

/*! Target state index in stored states */
#define DEFAULT_STATE_IDX                    3

/*! Target state index in stored states */
#define MIN_RANGE_STATE_IDX                  4

/*! Target state index in stored states */
#define MAX_RANGE_STATE_IDX                  5

/*! Scene states start index in stored states */
#define SCENE_STATE_IDX                      6

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Generic Power Level Server control block type definition */
typedef struct mmdlGenPowerLevelSrCb_tag
{
  mmdlSceneStore_t          fStoreScene;         /*!< Pointer to the function that stores
                                                  *    a scene on the model instance
                                                  */
  mmdlSceneRecall_t         fRecallScene;        /*!< Pointer to the function that recalls
                                                  *    a scene on the model instance
                                                  */
  mmdlBindResolve_t         fResolveBind;        /*!< Pointer to the function that checks
                                                  *   and resolves a bind triggered by a
                                                  *   change in this model instance
                                                  */
  mmdlEventCback_t          recvCback;           /*!< Model Generic Level received callback */
} mmdlGenPowerLevelSrCb_t;

/*! Generic Power Level Server message handler type definition */
typedef void (*mmdlGenPowerLevelSrHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler ID */
wsfHandlerId_t mmdlGenPowerLevelSrHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlGenPowerLevelSrRcvdOpcodes[MMDL_GEN_POWER_LEVEL_SR_NUM_RCVD_OPCODES] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_POWER_LEVEL_GET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_POWER_LEVEL_SET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_POWER_LEVEL_SET_NO_ACK_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_POWERLAST_GET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_POWERDEFAULT_GET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_POWERRANGE_GET_OPCODE)} },
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlGenPowerLevelSrHandleMsg_t mmdlGenPowerLevelSrHandleMsg[MMDL_GEN_POWER_LEVEL_SR_NUM_RCVD_OPCODES] =
{
  mmdlGenPowerLevelSrHandleGet,
  mmdlGenPowerLevelSrHandleSet,
  mmdlGenPowerLevelSrHandleSetNoAck,
  mmdlGenPowerLastSrHandleGet,
  mmdlGenPowerDefaultSrHandleGet,
  mmdlGenPowerRangeSrHandleGet,
};

/*! Generic Power Level Server Control Block */
static mmdlGenPowerLevelSrCb_t  powerLevelSrCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Searches for the Generic Power Level model instance descriptor on the
 *              specified element.
 *
 *  \param[in]  elementId  Identifier of the Element implementing the model.
 *  \param[out] ppOutDesc  Double pointer to the descriptor.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void mmdlGenPowerLevelSrGetDesc(meshElementId_t elementId, mmdlGenPowerLevelSrDesc_t **ppOutDesc)
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
        MMDL_GEN_POWER_LEVEL_SR_MDL_ID)
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
 *  \param[in] transitionTime  Transition time for this transaction.
 *  \param[in] delay5Ms        Delay for executing transaction. Unit is 5ms.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenPowerLevelSrSetPresentState(meshElementId_t elementId,
                                               mmdlGenPowerLevelSrDesc_t *pDesc,
                                               mmdlGenPowerLevelState_t targetState,
                                               uint32_t transitionMs, uint8_t delay5Ms,
                                               mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlGenPowerLevelSrEvent_t event;

  /* Update Present State. Check Range if target is a non-zero value. */
  if (targetState != 0)
  {
    if (targetState < pDesc->pStoredStates[MIN_RANGE_STATE_IDX])
    {
      pDesc->pStoredStates[PRESENT_STATE_IDX] = pDesc->pStoredStates[MIN_RANGE_STATE_IDX];
    }
    else if (targetState > pDesc->pStoredStates[MAX_RANGE_STATE_IDX])
    {
      pDesc->pStoredStates[PRESENT_STATE_IDX] = pDesc->pStoredStates[MAX_RANGE_STATE_IDX];
    }
    else
    {
      pDesc->pStoredStates[PRESENT_STATE_IDX] = targetState;
    }
  }
  else
  {
    pDesc->pStoredStates[PRESENT_STATE_IDX] = 0;
  }

  /* Update Last State */
  if (pDesc->pStoredStates[PRESENT_STATE_IDX] != 0)
  {
    pDesc->pStoredStates[LAST_STATE_IDX] = pDesc->pStoredStates[PRESENT_STATE_IDX];
  }

  /* Check for bindings on this state. Trigger bindings */
  if ((stateUpdateSrc != MMDL_STATE_UPDATED_BY_BIND) &&
      (stateUpdateSrc != MMDL_STATE_UPDATED_BY_SCENE) && (powerLevelSrCb.fResolveBind))
  {
    powerLevelSrCb.fResolveBind(elementId, MMDL_STATE_GEN_POW_ACT,
                                &pDesc->pStoredStates[PRESENT_STATE_IDX]);
  }

  /* Publish state change */
  MmdlGenPowerLevelSrPublish(elementId);

  /* Set event type */
  event.hdr.status = MMDL_SUCCESS;
  event.hdr.event = MMDL_GEN_POWER_LEVEL_SR_EVENT;
  event.hdr.param = MMDL_GEN_POWER_LEVEL_SR_STATE_UPDATE_EVENT;


  /* Set event parameters */
  event.statusEvent.elemId = elementId;
  event.statusEvent.state = targetState;
  event.statusEvent.stateUpdateSource = stateUpdateSrc;
  event.statusEvent.transitionMs = transitionMs;
  event.statusEvent.delay5Ms = delay5Ms;

  /* Send event to the upper layer */
  powerLevelSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model.
 *  \param[in] targetState     Target State for this transaction. See ::mmdlGenLevelState_t.
 *  \param[in] transitionTime  Transition time for this transaction.
 *  \param[in] delay5Ms        Delay for executing transaction. Unit is 5ms.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenPowerLevelSrSetState(meshElementId_t elementId, mmdlGenPowerLevelState_t targetState,
                                        uint32_t transitionMs, uint8_t delay5Ms,
                                        mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlGenPowerLevelSrDesc_t *pDesc = NULL;
  bool_t saveToNVM = FALSE;

  MMDL_TRACE_INFO3("GEN POWER LEVEL SR: Set Target=0x%X, TimeRem=%d ms, Delay=0x%X",
                    targetState, transitionMs, delay5Ms);

  /* Get model instance descriptor */
  mmdlGenPowerLevelSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Update descriptor */
    pDesc->remainingTimeMs = transitionMs;
    pDesc->delay5Ms = delay5Ms;
    pDesc->updateSource = stateUpdateSrc;

    /* Update Target State */
    if (pDesc->pStoredStates[TARGET_STATE_IDX] != targetState)
    {
      pDesc->pStoredStates[TARGET_STATE_IDX] = targetState;

      /* Save target state in NVM for Power Up. */
      saveToNVM = TRUE;
    }

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

      /* Update state entries in NVM. */
      saveToNVM = TRUE;

      mmdlGenPowerLevelSrSetPresentState(elementId, pDesc, targetState, transitionMs, delay5Ms,
                                         stateUpdateSrc);
    }

    /* Update state entries in NVM. */
    if(pDesc->fNvmSaveStates && (saveToNVM == TRUE))
    {
      pDesc->fNvmSaveStates(elementId);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Generic Power Level Status command to the specified destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] dstAddr      Element address of the destination
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenPowerLevelSrSendStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                          uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_POWER_LEVEL_SR_MDL_ID, MMDL_GEN_POWER_LEVEL_STATUS_OPCODE);
  uint8_t msgParams[MMDL_GEN_POWER_LEVEL_STATUS_MAX_LEN];
  uint8_t *pMsgParams, tranTime;
  mmdlGenPowerLevelSrDesc_t *pDesc = NULL;

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = dstAddr;
  msgInfo.ttl = MESH_USE_DEFAULT_TTL;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Get the model instance descriptor */
  mmdlGenPowerLevelSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredStates != NULL))
  {
    pMsgParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredStates[PRESENT_STATE_IDX]);

    if (pDesc->remainingTimeMs != 0)
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

      UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredStates[TARGET_STATE_IDX]);
      UINT8_TO_BSTREAM(pMsgParams, tranTime);

      MMDL_TRACE_INFO3("GEN POWER LEVEL SR: Send Status Present=0x%X, Target=0x%X, TimeRem=0x%X",
                       pDesc->pStoredStates[PRESENT_STATE_IDX],
                       pDesc->pStoredStates[TARGET_STATE_IDX], tranTime);
    }
    else
    {
      MMDL_TRACE_INFO1("GEN POWER LEVEL SR: Send Status Present=0x%X",
                        pDesc->pStoredStates[PRESENT_STATE_IDX]);
    }

    /* Send message to the Mesh Core */
    MeshSendMessage(&msgInfo, msgParams, (uint16_t)(pMsgParams - msgParams),
                    MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                    MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));
  }
}

/*************************************************************************************************/
/*!
*  \brief     Sends a Generic Power Last Status command to the specified destination address.
*
*  \param[in] elementId    Identifier of the Element implementing the model
*  \param[in] dstAddr      Element address of the destination
*  \param[in] appKeyIndex  Application Key Index.
*
*  \return    None.
*/
/*************************************************************************************************/
static void mmdlGenPowerLastSrSendStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                         uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_POWER_LEVEL_SR_MDL_ID, MMDL_GEN_POWERLAST_STATUS_OPCODE);
  mmdlGenPowerLevelSrDesc_t *pDesc = NULL;
  uint8_t msgParams[MMDL_GEN_POWERLAST_STATUS_LEN];
  uint8_t *pParams;

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = dstAddr;
  msgInfo.ttl = MESH_USE_DEFAULT_TTL;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Get the model instance descriptor */
  mmdlGenPowerLevelSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredStates != NULL))
  {
    pParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT16_TO_BE_BSTREAM(pParams, pDesc->pStoredStates[LAST_STATE_IDX]);

    /* Send message to the Mesh Core instantly */
    MeshSendMessage(&msgInfo, msgParams, MMDL_GEN_POWERLAST_STATUS_LEN,
                    MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                    MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the Generic Power Default state.
 *
 *  \param[in] elementId     Identifier of the Element implementing the model.
 *  \param[in] defaultState  Default State.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenPowerDefaultSrSetState(meshElementId_t elementId,
                                          mmdlGenPowerLevelState_t defaultState)
{
  mmdlGenPowerLevelSrEvent_t event;
  mmdlGenPowerLevelSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO1("GEN POWER DEFAULT SR: Set Default=0x%X", defaultState);

  /* Set event type */
  event.hdr.event = MMDL_GEN_POWER_LEVEL_SR_EVENT;
  event.hdr.param = MMDL_GEN_POWER_DEFAULT_SR_STATE_UPDATE_EVENT;

  /* Set event parameters */
  event.currentStateEvent.elemId = elementId;
  event.currentStateEvent.state = defaultState;

  /* Get model instance descriptor */
  mmdlGenPowerLevelSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredStates != NULL))
  {
    event.hdr.status = MMDL_SUCCESS;

    /* Update Default State */
    pDesc->pStoredStates[DEFAULT_STATE_IDX] = defaultState;

    /* Update state entries in NVM. */
    if(pDesc->fNvmSaveStates)
    {
      pDesc->fNvmSaveStates(elementId);
    }
  }
  else
  {
    event.hdr.status = MMDL_INVALID_ELEMENT;
  }
  
  /* Send event to the upper layer */
  powerLevelSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Generic Power Default Status command to the specified destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] dstAddr      Element address of the destination
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenPowerDefaultSrSendStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                            uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_POWER_LEVEL_SR_MDL_ID, MMDL_GEN_POWERDEFAULT_STATUS_OPCODE);
  mmdlGenPowerLevelSrDesc_t *pDesc = NULL;
  uint8_t msgParams[MMDL_GEN_POWERDEFAULT_STATUS_LEN];
  uint8_t *pParams;

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = dstAddr;
  msgInfo.ttl = MESH_USE_DEFAULT_TTL;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Get the model instance descriptor */
  mmdlGenPowerLevelSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredStates != NULL))
  {
    pParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT16_TO_BSTREAM(pParams, pDesc->pStoredStates[DEFAULT_STATE_IDX]);

    MMDL_TRACE_INFO1("GEN POWER DEFAULT SR: Send Status Present=0x%X",
                      pDesc->pStoredStates[DEFAULT_STATE_IDX]);

    /* Send message to the Mesh Core instantly */
    MeshSendMessage(&msgInfo, msgParams, MMDL_GEN_POWERDEFAULT_STATUS_LEN,
                    MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                    MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Generic Power Range Status command to the specified destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] dstAddr      Element address of the destination
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenPowerRangeSrSendStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                          uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_POWER_LEVEL_SR_MDL_ID, MMDL_GEN_POWERRANGE_STATUS_OPCODE);
  uint8_t msgParams[MMDL_GEN_POWERRANGE_STATUS_LEN];
  mmdlGenPowerLevelSrDesc_t *pDesc = NULL;
  uint8_t *pMsgParams;

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = dstAddr;
  msgInfo.ttl = MESH_USE_DEFAULT_TTL;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Get the model instance descriptor */
  mmdlGenPowerLevelSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredStates != NULL))
  {
    pMsgParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT8_TO_BSTREAM(pMsgParams, 0);
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredStates[MIN_RANGE_STATE_IDX]);
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredStates[MAX_RANGE_STATE_IDX]);

    MMDL_TRACE_INFO2("GEN POWER RANGE SR: Send Status MinPower=0x%X, MaxPower=0x%X",
                      pDesc->pStoredStates[MIN_RANGE_STATE_IDX],
                      pDesc->pStoredStates[MAX_RANGE_STATE_IDX]);

    /* Send message to the Mesh Core instantly */
    MeshSendMessage(&msgInfo, msgParams, MMDL_GEN_POWERRANGE_STATUS_LEN,
                    MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                    MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Power Level Set Unacknowledged command.
 *
 *  \param[in] pMsg         Received model message.
 *  \param[in] ackRequired  TRUE if acknowledgement is required in response,  FALSE otherwise.
 *
 *  \return    TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlGenPowerLevelSrProcessSet(const meshModelMsgRecvEvt_t *pMsg, bool_t ackRequired)
{
  mmdlGenPowerLevelState_t state;
  mmdlGenPowerLevelSrDesc_t *pDesc = NULL;
  uint8_t tid;
  uint32_t transMs;
  uint32_t delayMs;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length. It can take only min and max values. */
  if (pMsg->messageParamsLen != MMDL_GEN_POWER_LEVEL_SET_MAX_LEN &&
      pMsg->messageParamsLen != MMDL_GEN_POWER_LEVEL_SET_MIN_LEN)
  {
    return FALSE;
  }

  /* Set the state value from pMessageParams buffer. */
  BYTES_TO_UINT16(state, &pMsg->pMessageParams[0]);

  /* Check if it contains optional parameters */
  if (pMsg->messageParamsLen == MMDL_GEN_POWER_LEVEL_SET_MAX_LEN)
  {
    /* Check prohibited values for Transition Time */
    if (TRANSITION_TIME_STEPS(pMsg->pMessageParams[MMDL_SET_TRANSITION_IDX]) ==
        MMDL_GEN_TR_UNKNOWN)
    {
      return FALSE;
    }
  }

  /* Get the model instance descriptor */
  mmdlGenPowerLevelSrGetDesc(pMsg->elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredStates != NULL))
  {
    WsfTimerStartMs(&pDesc->msgRcvdTimer, MSG_RCVD_TIMEOUT_MS);

    /* Get Transaction ID */
    tid = pMsg->pMessageParams[MMDL_SET_TID_IDX];

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

    /* Check if it contains optional parameters */
    if (pMsg->messageParamsLen == MMDL_GEN_POWER_LEVEL_SET_MAX_LEN)
    {
      /* Get Transition time */
      transMs = MmdlGenDefaultTransTimeToMs(pMsg->pMessageParams[MMDL_SET_TRANSITION_IDX]);
      delayMs = pMsg->pMessageParams[MMDL_SET_DELAY_IDX];
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
      pDesc->transitionStep = (state - pDesc->pStoredStates[PRESENT_STATE_IDX]) / pDesc->steps;
    }

    /* Change state */
    mmdlGenPowerLevelSrSetState(pMsg->elementId, state, transMs, delayMs, MMDL_STATE_UPDATED_BY_CL);

    return (pDesc->delay5Ms == 0);
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Power Level Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    TRUE if handled successful, FALSE otherwise.
 */
/*************************************************************************************************/
void mmdlGenPowerLevelSrHandleGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    mmdlGenPowerLevelSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Power Level Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenPowerLevelSrHandleSet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  if (mmdlGenPowerLevelSrProcessSet(pMsg, TRUE))
  {
    /* Send Status message as a response to the Set message */
    mmdlGenPowerLevelSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Power Level Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenPowerLevelSrHandleSetNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  (void)mmdlGenPowerLevelSrProcessSet(pMsg, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Power Last Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenPowerLastSrHandleGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    mmdlGenPowerLastSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Power Default Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenPowerDefaultSrHandleGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    mmdlGenPowerDefaultSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Power Range Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenPowerRangeSrHandleGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    mmdlGenPowerRangeSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Generic Power Level Server timer callback.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenPowerLevelSrHandleTmrCback(uint8_t elementId)
{
  mmdlGenPowerLevelSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlGenPowerLevelSrGetDesc(elementId, &pDesc);

  /* Transition timeout. Move to Target State. */
  if ((pDesc != NULL) && (pDesc->pStoredStates != NULL))
  {
    if (pDesc->delay5Ms != 0)
    {
      /* Reset Delay */
      pDesc->delay5Ms = 0;

      /* Timeout. Set state. */
      mmdlGenPowerLevelSrSetState(elementId, pDesc->pStoredStates[TARGET_STATE_IDX],
                                  pDesc->remainingTimeMs, 0, pDesc->updateSource);

      /* Send Status if it was a delayed Acknowledged Set */
      if (pDesc->ackPending)
      {
        mmdlGenPowerLevelSrSendStatus(elementId, pDesc->srcAddr, pDesc->ackAppKeyIndex,
                                      pDesc->ackForUnicast);
      }
    }
    else if (pDesc->remainingTimeMs != 0)
    {
      if (pDesc->steps > 0)
      {
        mmdlGenPowerLevelState_t state;
        uint32_t remainingTimeMs;

        /* Transition is divided into steps. Decrement the remaining time and steps */
        pDesc->steps--;
        remainingTimeMs = pDesc->remainingTimeMs - MMDL_TRANSITION_STATE_UPDATE_INTERVAL;

        /* Compute intermediate state value */
        state = pDesc->pStoredStates[PRESENT_STATE_IDX] + pDesc->transitionStep;

        /* Update present state only. */
        mmdlGenPowerLevelSrSetPresentState(elementId, pDesc, state, remainingTimeMs, 0,
                                           pDesc->updateSource);

        if (pDesc->steps == 1)
        {
          /* Next is the last step.
           * Program the remaining time (can be more than MMDL_TRANSITION_STATE_UPDATE_INTERVAL).
           * Also, the last step increment can be greater then the intermediate ones.
           */
          pDesc->steps = 0;
        }

        /* Program next transition */
        mmdlGenPowerLevelSrSetState(elementId, pDesc->pStoredStates[TARGET_STATE_IDX], remainingTimeMs, 0,
                                    pDesc->updateSource);
      }
      else
      {
        /* Transition timeout. Move to Target State. */
        mmdlGenPowerLevelSrSetState(elementId, pDesc->pStoredStates[TARGET_STATE_IDX], 0, 0,
                                    pDesc->updateSource);
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Generic Power Level Server message timer callback.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenPowerLevelSrHandleMsgTmrCback(uint8_t elementId)
{
  mmdlGenPowerLevelSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlGenPowerLevelSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredStates != NULL))
  {
    /* Reset source address and transaction ID for last stored transaction */
    pDesc->srcAddr = MESH_ADDR_TYPE_UNASSIGNED;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the Generic Power Range state.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] targetState  Range Minimum. See ::mmdlGenPowerLevelState_t.
 *  \param[in] targetState  Range maximum. See ::mmdlGenPowerLevelState_t.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenPowerRangeSrSetState(meshElementId_t elementId, mmdlGenPowerLevelState_t rangeMin,
                                        mmdlGenPowerLevelState_t rangeMax)
{
  mmdlGenPowerLevelSrEvent_t event;
  mmdlGenPowerLevelSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO2("GEN POWER RANGE SR: Set TargetMin=0x%X, TargetMax=0x%X", rangeMin, rangeMax);

  /* Set event type */
  event.hdr.event = MMDL_GEN_POWER_LEVEL_SR_EVENT;
  event.hdr.param = MMDL_GEN_POWER_RANGE_SR_STATE_UPDATE_EVENT;

  /* Set event parameters */
  event.rangeStatusEvent.elemId = elementId;
  event.rangeStatusEvent.minState = rangeMin;
  event.rangeStatusEvent.maxState = rangeMax;

  /* Get model instance descriptor */
  mmdlGenPowerLevelSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredStates != NULL))
  {
    event.hdr.status = MMDL_SUCCESS;

    /* Validate that the range values. */
    if ((rangeMin != 0) && (rangeMax != 0))
    {
      /* Update Target State */
      pDesc->pStoredStates[MIN_RANGE_STATE_IDX] = rangeMin;
      pDesc->pStoredStates[MAX_RANGE_STATE_IDX] = rangeMax;

      /* Update state entries in NVM. */
      if(pDesc->fNvmSaveStates)
      {
        pDesc->fNvmSaveStates(elementId);
      }
    }
  }
  else
  {
    event.hdr.status = MMDL_INVALID_ELEMENT;
  }
  
  /* Send event to the upper layer */
  powerLevelSrCb.recvCback((wsfMsgHdr_t *)&event);
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
static void mmdlGenPowerLevelSrStoreScene(void *pDesc, uint8_t sceneIdx)
{
  mmdlGenPowerLevelSrDesc_t *pGenPowerLevelDesc = (mmdlGenPowerLevelSrDesc_t *)pDesc;

  MMDL_TRACE_INFO1("GEN POWER LEVEL SR: Store Level=%d",
                   pGenPowerLevelDesc->pStoredStates[PRESENT_STATE_IDX]);

  /* Store present state */
  pGenPowerLevelDesc->pStoredStates[SCENE_STATE_IDX + sceneIdx] =
      pGenPowerLevelDesc->pStoredStates[PRESENT_STATE_IDX];
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
static void mmdlGenPowerLevelSrRecallScene(meshElementId_t elementId, uint8_t sceneIdx,
                                           uint32_t transitionMs)
{
  mmdlGenPowerLevelSrDesc_t *pDesc = NULL;

  /* Get the model instance descriptor */
  mmdlGenPowerLevelSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredStates != NULL))
  {
    MMDL_TRACE_INFO3("GEN POWER LEVEL SR: Recall elemid=%d powerlevel=%d transMs=%d",
                     elementId, pDesc->pStoredStates[SCENE_STATE_IDX + sceneIdx], transitionMs);

    /* Recall state */
    mmdlGenPowerLevelSrSetState(elementId, pDesc->pStoredStates[SCENE_STATE_IDX + sceneIdx],
                                transitionMs, 0, MMDL_STATE_UPDATED_BY_SCENE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between the Generic OnPowerUp and a Power Level Actual state as
 *             a result of a Power Up procedure.
 *
 *  \param[in] tgtElementId  Target element ID.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveOnPowerUp2PowAct(meshElementId_t tgtElementId, void *pStateValue)
{
  mmdlGenOnPowerUpState_t powerUpState;
  mmdlGenPowerLevelState_t state;
  mmdlGenPowerLevelSrDesc_t *pDesc = NULL;

  powerUpState = *(mmdlGenOnPowerUpState_t *)pStateValue;

  MMDL_TRACE_INFO1("GEN POWER LEVEL SR: PowerUpState=0x%X", powerUpState);

  mmdlGenPowerLevelSrGetDesc(tgtElementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredStates != NULL))
  {
    switch (powerUpState)
    {
      case MMDL_GEN_ONPOWERUP_STATE_OFF:
        state = 0;
        break;

      case MMDL_GEN_ONPOWERUP_STATE_DEFAULT:
        if (pDesc->pStoredStates[DEFAULT_STATE_IDX] != 0)
        {
          state = pDesc->pStoredStates[DEFAULT_STATE_IDX];
        }
        else
        {
          state = pDesc->pStoredStates[LAST_STATE_IDX];
        }
        break;

      case MMDL_GEN_ONPOWERUP_STATE_RESTORE:
        if (pDesc->pStoredStates[PRESENT_STATE_IDX] != pDesc->pStoredStates[TARGET_STATE_IDX])
        {
          /* Transition was in progress. Restore target */
          state = pDesc->pStoredStates[TARGET_STATE_IDX];
        }
        else
        {
          /* Keep last known value. */
          return;
        }
        break;

      default:
        return;
    }

    /* Change state locally. No transition time or delay is allowed. */
    mmdlGenPowerLevelSrSetState(tgtElementId, state, 0, 0, MMDL_STATE_UPDATED_BY_BIND);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between a Generic On Off state and a Generic Power Actual state as
 *             a result of an updated Generic On Off state.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveGenOnOff2GenPowAct(meshElementId_t tgtElementId, void *pStateValue)
{
  mmdlGenPowerLevelState_t level;
  mmdlGenPowerLevelSrDesc_t *pDesc = NULL;
  mmdlGenOnOffState_t onoff;

  onoff = *(mmdlGenOnOffState_t *)pStateValue;

  /* Get model instance descriptor */
  mmdlGenPowerLevelSrGetDesc(tgtElementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredStates != NULL))
  {
    if (onoff == MMDL_GEN_ONOFF_STATE_OFF)
    {
      level = 0;
    }
    else if (pDesc->pStoredStates[DEFAULT_STATE_IDX] == 0)
    {
      level = pDesc->pStoredStates[LAST_STATE_IDX];
    }
    else
    {
      level = pDesc->pStoredStates[DEFAULT_STATE_IDX];
    }

    /* Update Generic Level state on target element. Implicit bind via Generic Power Level */
    MmdlGenLevelSrSetBoundState(tgtElementId, level - 0x8000);

    /* Change state locally. No transition time or delay is allowed. */
    mmdlGenPowerLevelSrSetState(tgtElementId, level, 0, 0, MMDL_STATE_UPDATED_BY_BIND);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between a Generic On Off state and a Generic Power Actual state as
 *             a result of an updated Generic Power Actual Actual state.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveGenPowAct2GenOnOff(meshElementId_t tgtElementId, void *pStateValue)
{
  uint16_t powerLevel;
  mmdlGenOnOffState_t onoff = MMDL_GEN_ONOFF_STATE_OFF;

  powerLevel = *(uint16_t*)pStateValue;

  if (powerLevel > 0)
  {
    onoff = MMDL_GEN_ONOFF_STATE_ON;
  }

  /* Update target state. */
  MmdlGenOnOffSrSetBoundState(tgtElementId, onoff);
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between a Generic Power Actual state and a Generic Level state as
 *             a result of an updated Generic Power Actual state.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveGenPowAct2GenLevel(meshElementId_t tgtElementId, void *pStateValue)
{
  uint16_t level = *(int16_t*)pStateValue + 32768;

  /* Update Generic Level state on target element */
  MmdlGenLevelSrSetBoundState(tgtElementId, level);
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between Generic Level a state and a Generic Power Actual state as
 *             a result of an updated Generic Level state.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveGenLevel2GenPowAct(meshElementId_t tgtElementId, void *pStateValue)
{
  mmdlGenLevelState_t level;

  level =  *(mmdlGenLevelState_t*)pStateValue;

  /* Update Generic On Off state on target element. Implicit bind via Generic Power Level */
  MmdlGenOnOffSrSetBoundState(tgtElementId, level > 0);

  /* Change state locally. No transition time or delay is allowed. */
  mmdlGenPowerLevelSrSetState(tgtElementId, level + 0x8000, 0, 0, MMDL_STATE_UPDATED_BY_BIND);
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Generic Power Level Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSrInit(void)
{
  meshElementId_t elemId;
  mmdlGenPowerLevelSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO0("POWER LEVEL SR: init");

  /* Set event callbacks */
  powerLevelSrCb.recvCback = MmdlEmptyCback;
  powerLevelSrCb.fResolveBind = MmdlBindResolve;
  powerLevelSrCb.fRecallScene = mmdlGenPowerLevelSrRecallScene;
  powerLevelSrCb.fStoreScene = mmdlGenPowerLevelSrStoreScene;

  /* Initialize timers */
  for (elemId = 0; elemId < pMeshConfig->elementArrayLen; elemId++)
  {
    /* Get the model instance descriptor */
    mmdlGenPowerLevelSrGetDesc(elemId, &pDesc);

    if ((pDesc != NULL) && (pDesc->pStoredStates != NULL))
    {
      pDesc->srcAddr = MESH_ADDR_TYPE_UNASSIGNED;

      /* Set transition timer parameters*/
      pDesc->transitionTimer.handlerId = mmdlGenPowerLevelSrHandlerId;
      pDesc->transitionTimer.msg.event = MMDL_GEN_POWER_LEVEL_SR_EVT_TMR_CBACK;
      pDesc->transitionTimer.msg.param = elemId;

      /* Set msg Received timer parameters*/
      pDesc->msgRcvdTimer.handlerId = mmdlGenPowerLevelSrHandlerId;
      pDesc->msgRcvdTimer.msg.event = MMDL_GEN_POWER_LEVEL_SR_MSG_RCVD_TMR_CBACK;
      pDesc->msgRcvdTimer.msg.param = elemId;

      /* Set default range */
      pDesc->pStoredStates[MIN_RANGE_STATE_IDX] = MMDL_GEN_POWERRANGE_MIN;
      pDesc->pStoredStates[MAX_RANGE_STATE_IDX] = MMDL_GEN_POWERRANGE_MAX;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Generic Power Level Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Generic Power Level Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSrHandlerInit(wsfHandlerId_t handlerId)
{
  /* Set handler ID */
  mmdlGenPowerLevelSrHandlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Generic Power Level Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSrHandler(wsfMsgHdr_t *pMsg)
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
        if (MESH_OPCODE_SIZE(pModelMsg->msgRecvEvt.opCode) == MMDL_GEN_POWER_LEVEL_OPCODES_SIZE)
        {
          /* Match the received opcode */
          for (opcodeIdx = 0; opcodeIdx < MMDL_GEN_POWER_LEVEL_SR_NUM_RCVD_OPCODES; opcodeIdx++)
          {
            if (!memcmp(&mmdlGenPowerLevelSrRcvdOpcodes[opcodeIdx], pModelMsg->msgRecvEvt.opCode.opcodeBytes,
                        MMDL_GEN_POWER_LEVEL_OPCODES_SIZE))
            {
              /* Process message */
              (void)mmdlGenPowerLevelSrHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
            }
          }
        }
        break;

      case MESH_MODEL_EVT_PERIODIC_PUB:
        pModelMsg = (meshModelEvt_t *)pMsg;

        /* Check if periodic publishing was not disabled. */
        if (pModelMsg->periodicPubEvt.nextPubTimeMs != 0)
        {
          /* Publishing is requested part of the periodic publishing */
          MmdlGenPowerLevelSrPublish(pModelMsg->periodicPubEvt.elementId);
        }
        break;

      case MMDL_GEN_POWER_LEVEL_SR_EVT_TMR_CBACK:
        mmdlGenPowerLevelSrHandleTmrCback((uint8_t)pMsg->param);
        break;

      case MMDL_GEN_POWER_LEVEL_SR_MSG_RCVD_TMR_CBACK:
        mmdlGenPowerLevelSrHandleMsgTmrCback((uint8_t)pMsg->param);
        break;

      default:
        MMDL_TRACE_WARN0("GEN POWER LELVEL SR: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publish a GenPowerLevel Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSrPublish(meshElementId_t elementId)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_GEN_POWER_LEVEL_SR_MDL_ID,
                                             MMDL_GEN_POWER_LEVEL_STATUS_OPCODE);
  uint8_t msgParams[MMDL_GEN_POWER_LEVEL_STATUS_MAX_LEN];
  uint8_t *pMsgParams, tranTime;
  mmdlGenPowerLevelSrDesc_t *pDesc = NULL;

  /* Fill in the msg info parameters */
  pubMsgInfo.elementId = elementId;

  /* Get the model instance descriptor */
  mmdlGenPowerLevelSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredStates != NULL))
  {
    pMsgParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredStates[PRESENT_STATE_IDX]);

    if (pDesc->remainingTimeMs != 0)
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

      UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredStates[TARGET_STATE_IDX]);
      UINT8_TO_BSTREAM(pMsgParams, tranTime);

      MMDL_TRACE_INFO3("GEN POWER LEVEL SR: Publish Present=0x%X, Target=0x%X, TimeRem=0x%X",
                       pDesc->pStoredStates[PRESENT_STATE_IDX],
                       pDesc->pStoredStates[TARGET_STATE_IDX], tranTime);
    }
    else
    {
      MMDL_TRACE_INFO1("GEN POWER LEVEL SR: Publish Present=0x%X",
                        pDesc->pStoredStates[PRESENT_STATE_IDX]);
    }

    /* Send message to the Mesh Core */
    MeshPublishMessage(&pubMsgInfo, msgParams, (uint16_t)(pMsgParams - msgParams));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the Generic Power Actual state of the element.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] targetState  Target State for this transaction. See ::mmdlGenPowerLevelState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSrSetState(meshElementId_t elementId, mmdlGenPowerLevelState_t targetState)
{
    /* Change state locally. No transition time or delay required. */
    mmdlGenPowerLevelSrSetState(elementId, targetState, 0, 0, MMDL_STATE_UPDATED_BY_APP);
}

/*************************************************************************************************/
/*!
 *  \brief     Get the Generic Power Actual state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSrGetState(meshElementId_t elementId)
{
  mmdlGenPowerLevelSrEvent_t event;
  mmdlGenPowerLevelSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlGenPowerLevelSrGetDesc(elementId, &pDesc);

  /* Set event type */
  event.hdr.event = MMDL_GEN_POWER_LEVEL_SR_EVENT;
  event.hdr.param = MMDL_GEN_POWER_LEVEL_SR_CURRENT_STATE_EVENT;

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
  powerLevelSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Get the Generic Power Last state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLastSrGetState(meshElementId_t elementId)
{
  mmdlGenPowerLevelSrEvent_t event;
  mmdlGenPowerLevelSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlGenPowerLevelSrGetDesc(elementId, &pDesc);

  /* Set event type */
  event.hdr.event = MMDL_GEN_POWER_LEVEL_SR_EVENT;
  event.hdr.param = MMDL_GEN_POWER_LAST_SR_CURRENT_STATE_EVENT;

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
    event.currentStateEvent.state = pDesc->pStoredStates[LAST_STATE_IDX];
  }

  /* Send event to the upper layer */
  powerLevelSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Get the Generic Power Default state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerDefaultSrGetState(meshElementId_t elementId)
{
  mmdlGenPowerLevelSrEvent_t event;
  mmdlGenPowerLevelSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlGenPowerLevelSrGetDesc(elementId, &pDesc);

  /* Set event type */
  event.hdr.event = MMDL_GEN_POWER_LEVEL_SR_EVENT;
  event.hdr.param = MMDL_GEN_POWER_DEFAULT_SR_CURRENT_STATE_EVENT;

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
  powerLevelSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Set the Generic Power Default state of the element.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] targetState  Target State for this transaction. See ::mmdlGenPowerLevelState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerDefaultSrSetState(meshElementId_t elementId, mmdlGenPowerLevelState_t defaultState)
{
  /* Change state locally. */
  mmdlGenPowerDefaultSrSetState(elementId, defaultState);
}

/*************************************************************************************************/
/*!
 *  \brief     Get the Generic Power Range state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerRangeSrGetState(meshElementId_t elementId)
{
  mmdlGenPowerLevelSrEvent_t event;
  mmdlGenPowerLevelSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlGenPowerLevelSrGetDesc(elementId, &pDesc);

  /* Set event type */
  event.hdr.event = MMDL_GEN_POWER_LEVEL_SR_EVENT;
  event.hdr.param = MMDL_GEN_POWER_RANGE_SR_CURRENT_EVENT;

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
  powerLevelSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Set the Generic Power Range state of the element.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] targetState  Range Minimum. See ::mmdlGenPowerLevelState_t.
 *  \param[in] targetState  Range maximum. See ::mmdlGenPowerLevelState_t.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerRangeSrSetState(meshElementId_t elementId, mmdlGenPowerLevelState_t rangeMin,
                                        mmdlGenPowerLevelState_t rangeMax)
{
  /* Change state locally. */
  mmdlGenPowerRangeSrSetState(elementId, rangeMin, rangeMax);
}

/*************************************************************************************************/
/*!
 *  \brief     Stores the local states that in a scene.
 *
 *  \param[in] pDesc    Model instance descriptor.
 *  \param[in] sceneIdx Identifier of the stored scene.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSrStoreScene(void *pDesc, uint8_t sceneIdx)
{
  if (powerLevelSrCb.fStoreScene != NULL)
  {
    powerLevelSrCb.fStoreScene(pDesc, sceneIdx);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the local states values according to the previously stored scene.
 *
 *  \param[in] elementId     Identifier of the element implementing the model instance.
 *  \param[in] sceneIdx      Identifier of the recalled scene.
 *  \param[in] transitionMs  Transition time in ms.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSrRecallScene(meshElementId_t elementId, uint8_t sceneIdx,
                                    uint32_t transitionMs)
{
  if (powerLevelSrCb.fRecallScene != NULL)
  {
    powerLevelSrCb.fRecallScene(elementId, sceneIdx, transitionMs);
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
void MmdlGenPowerLevelSrRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback*/
  if (recvCback != NULL)
  {
    powerLevelSrCb.recvCback = recvCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local state as a result of an OnPowerUp binding. The set is instantaneous.
 *
 *  \param[in] elementId     Identifier of the Element implementing the model.
 *  \param[in] powerUpState  New State. See ::mmdlGenOnPowerUpStates
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSrBind2OnPowerUp(meshElementId_t onPowerUpElemId, meshElementId_t powElemId)
{
  /* Add Generic Power OnOff -> Power Level Actual binding */
  MmdlAddBind(MMDL_STATE_GEN_ONPOWERUP, MMDL_STATE_GEN_POW_ACT, onPowerUpElemId, powElemId,
              mmdlBindResolveOnPowerUp2PowAct);
}

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Generic Power Actual State and a Generic Level state.
 *
 *  \param[in] gplElemId  Element identifier where the Generic Power Actual state resides.
 *  \param[in] glvElemId  Element identifier where the Generic Level state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSrBind2GenLevel(meshElementId_t gplElemId, meshElementId_t glvElemId)
{
  /* Add Generic Power Actual -> Generic Level binding */
  MmdlAddBind(MMDL_STATE_GEN_POW_ACT, MMDL_STATE_GEN_LEVEL, gplElemId, glvElemId,
              mmdlBindResolveGenPowAct2GenLevel);

  /* Add Generic Level -> Generic Power Actual binding */
  MmdlAddBind(MMDL_STATE_GEN_LEVEL, MMDL_STATE_GEN_POW_ACT, glvElemId, gplElemId,
              mmdlBindResolveGenLevel2GenPowAct);
}

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Generic Power Actual State and a Generic On Off state.
 *
 *  \param[in] ltElemId     Element identifier where the Generic Power Actual state resides.
 *  \param[in] onoffElemId  Element identifier where the On Off state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSrBind2GenOnOff(meshElementId_t gplElemId, meshElementId_t onoffElemId)
{
  /* Add Generic On Off -> Generic Power Actual binding */
  MmdlAddBind(MMDL_STATE_GEN_ONOFF, MMDL_STATE_GEN_POW_ACT, onoffElemId, gplElemId,
              mmdlBindResolveGenOnOff2GenPowAct);

  /* Add Generic Power Actual -> Generic On Off binding */
  MmdlAddBind(MMDL_STATE_GEN_POW_ACT, MMDL_STATE_GEN_ONOFF, gplElemId, onoffElemId,
              mmdlBindResolveGenPowAct2GenOnOff);
}
