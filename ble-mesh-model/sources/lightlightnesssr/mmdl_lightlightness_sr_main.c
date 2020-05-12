/*************************************************************************************************/
/*!
 *  \file   mmdl_lightlightness_sr_main.c
 *
 *  \brief  Implementation of the Light Lightness Server model.
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
#include "wsf_buf.h"
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"

#include "mmdl_types.h"
#include "mmdl_defs.h"
#include "mmdl_common.h"
#include "mmdl_bindings.h"

#include "mmdl_lightlightness_sr_api.h"
#include "mmdl_lightlightness_sr_main.h"
#include "mmdl_lightlightness_sr.h"

#include "mmdl_gen_default_trans_sr.h"
#include "mmdl_gen_onoff_sr.h"
#include "mmdl_gen_powonoff_sr.h"
#include "mmdl_gen_level_sr.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Light Lightness Set Message TID index */
#define MMDL_SET_TID_IDX                    2

/*! Light Lightness Set Message TID index */
#define MMDL_SET_TRANSITION_IDX             3

/*! Light Lightness Set Message TID index */
#define MMDL_SET_DELAY_IDX                  4

/*! Actual state index in stored states */
#define ACTUAL_STATE_IDX                    0

/*! Linear state index in stored states */
#define LINEAR_STATE_IDX                    1

/*! Target state index in stored states */
#define TARGET_STATE_IDX                    2

/*! Last state index in stored states */
#define LAST_STATE_IDX                      3

/*! Default state index in stored states */
#define DEFAULT_STATE_IDX                   4

/*! Range Min state index in stored states */
#define RANGE_MIN_STATE_IDX                 5

/*! Range Max state index in stored states */
#define RANGE_MAX_STATE_IDX                 6

/*! Scene states start index in stored states */
#define SCENE_STATE_IDX                     7

/*! The default value for the Light Lightness Last state */
#define LIGHT_LIGHTNESS_LAST_INIT            0xFFFF

/*! The default value for the Light Lightness Default state */
#define LIGHT_LIGHTNESS_DEFAULT_INIT         0x0000

/*! The Prohibited value for the Light Lightness Range Minimum and Maximum state */
#define LIGHT_LIGHTNESS_RANGE_PROHIBITED     0x0000

/*! The initialization value for the Light Lightness Range Minimum state */
#define LIGHT_LIGHTNESS_RANGE_MIN_INIT       0x0001

/*! The initialization value for the Light Lightness Range Maximum state */
#define LIGHT_LIGHTNESS_RANGE_MAX_INIT       0xFFFF

/*! Shift value for Light Lightness Actual to Linear conversion */
#define SHIFT8                              8

/*! Shift value for Light Lightness Actual to Linear conversion */
#define SHIFT16                             16

/*! Identifier for Light Lightness Actual transition */
#define LIGHT_LIGHTNESS_ACTUAL_TRANSITION    0

/*! Identifier for Light Lightness Linear transition */
#define LIGHT_LIGHTNESS_LINEAR_TRANSITION    1

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Light Lightness Server control block type definition */
typedef struct mmdlLightLightnessSrCb_tag
{
  mmdlSceneStore_t          fStoreScene;          /*!< Pointer to the function that stores
                                                   *    a scene on the model instance
                                                   */
  mmdlSceneRecall_t         fRecallScene;         /*!< Pointer to the function that recalls
                                                   *    a scene on the model instance
                                                   */
  mmdlBindResolve_t         fResolveBind;         /*!< Pointer to the function that checks
                                                   *   and resolves a bind triggered by a
                                                   *   change in this model instance
                                                   */
  mmdlEventCback_t          recvCback;            /*!< Model Light Lightness received callback */
}mmdlLightLightnessSrCb_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler ID */
wsfHandlerId_t mmdlLightLightnessSrHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlLightLightnessSrRcvdOpcodes[MMDL_LIGHT_LIGHTNESS_SR_NUM_RCVD_OPCODES] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_LIGHTNESS_GET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_LIGHTNESS_SET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_LIGHTNESS_SET_NO_ACK_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_LIGHTNESS_LINEAR_GET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_LIGHTNESS_LINEAR_SET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_LIGHTNESS_LINEAR_SET_NO_ACK_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_LIGHTNESS_LAST_GET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_LIGHTNESS_DEFAULT_GET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_LIGHTNESS_RANGE_GET_OPCODE)} },
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlModelHandleMsg_t mmdlLightLightnessSrHandleMsg[MMDL_LIGHT_LIGHTNESS_SR_NUM_RCVD_OPCODES] =
{
  mmdlLightLightnessSrHandleGet,
  mmdlLightLightnessSrHandleSet,
  mmdlLightLightnessSrHandleSetNoAck,
  mmdlLightLightnessLinearSrHandleGet,
  mmdlLightLightnessLinearSrHandleSet,
  mmdlLightLightnessLinearSrHandleSetNoAck,
  mmdlLightLightnessLastSrHandleGet,
  mmdlLightLightnessDefaultSrHandleGet,
  mmdlLightLightnessRangeSrHandleGet,
};

/*! Light Lightness Server Control Block */
static mmdlLightLightnessSrCb_t  lightLightnessSrCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/
static uint16_t square_root(uint64_t param)
{
  int64_t root = param;
  int64_t last;
  int64_t diff;
  if (param == 0)
  {
    return 0;
  }

  do {
      last = root;
      root = (root + param / root) / 2;
      diff = root - last;
  } while (diff > 1 || diff < -1);

  return (uint16_t)root;
}

/*************************************************************************************************/
/*!
 *  \brief      Searches for the Light Lightness model instance descriptor on the
 *              specified element.
 *
 *  \param[in]  elementId  Identifier of the Element implementing the model.
 *  \param[out] ppOutDesc  Double pointer to the descriptor.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void mmdlLightLightnessSrGetDesc(meshElementId_t elementId,
                                        mmdlLightLightnessSrDesc_t **ppOutDesc)
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
        MMDL_LIGHT_LIGHTNESS_SR_MDL_ID)
    {
      /* Matching model ID on elementId */
      *ppOutDesc = pMeshConfig->pElementArray[elementId].pSigModelArray[modelIdx].pModelDescriptor;
      break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local Light Lightness Actual present state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model.
 *  \param[in] pDesc           Pointer to the model descriptor
 *  \param[in] targetState     Target State for this transaction. See ::mmdlLightLightnessState_t.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightLightnessSrSetPresentState(meshElementId_t elementId,
                                                mmdlLightLightnessSrDesc_t *pDesc,
                                                mmdlLightLightnessState_t targetState,
                                                mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlLightLightnessSrEvent_t event;
  uint32_t computedLinear;

  /* Update Light Lightness Last state */
  if (targetState != 0)
  {
    pDesc->pStoredStates[LAST_STATE_IDX] = targetState;
  }
  else if (pDesc->pStoredStates[ACTUAL_STATE_IDX] != 0)
  {
    pDesc->pStoredStates[LAST_STATE_IDX] = pDesc->pStoredStates[ACTUAL_STATE_IDX];
  }

  /* Update State */
  pDesc->pStoredStates[ACTUAL_STATE_IDX] = targetState;

  if (stateUpdateSrc != MMDL_STATE_UPDATED_BY_SCENE)
  {
    /* Compute the Light Lightness Linear state if not set by a recalled scene */
    computedLinear = (((uint32_t)targetState * (uint32_t)targetState) >> SHIFT16);
    pDesc->pStoredStates[LINEAR_STATE_IDX] = (mmdlLightLightnessState_t)(computedLinear);
  }

  /* Publish updated state */
  MmdlLightLightnessSrPublishLinear(elementId);

  /* Check for bindings on this state. Trigger bindings */
  if ((stateUpdateSrc != MMDL_STATE_UPDATED_BY_BIND) &&
      (stateUpdateSrc != MMDL_STATE_UPDATED_BY_SCENE) && (lightLightnessSrCb.fResolveBind))
  {
    lightLightnessSrCb.fResolveBind(elementId, MMDL_STATE_LT_LTNESS_ACT,
                                    &pDesc->pStoredStates[ACTUAL_STATE_IDX]);
  }

  /* Publish state change */
  MmdlLightLightnessSrPublish(elementId);

  /* Set event type */
  event.hdr.status = MMDL_SUCCESS;
  event.hdr.event = MMDL_LIGHT_LIGHTNESS_SR_EVENT;
  event.hdr.param = MMDL_LIGHT_LIGHTNESS_SR_STATE_UPDATE_EVENT;

  /* Set event parameters */
  event.statusEvent.elemId = elementId;
  event.statusEvent.lightnessState.state = targetState;
  event.statusEvent.stateUpdateSource = stateUpdateSrc;

  /* Send event to the upper layer */
  lightLightnessSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local Light Lightness Actual state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model.
 *  \param[in] targetState     Target State for this transaction. See ::mmdlLightLightnessState_t.
 *  \param[in] transitionTime  Transition time for this transaction.
 *  \param[in] delay5Ms        Delay for executing transaction. Unit is 5ms.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightLightnessSrSetState(meshElementId_t elementId,
                                         mmdlLightLightnessState_t targetState,
                                         uint32_t transitionMs, uint8_t delay5Ms,
                                         mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlLightLightnessSrDesc_t *pDesc = NULL;
  bool_t saveToNVM = FALSE;

  MMDL_TRACE_INFO3("LIGHT LIGHTNESS SR: Set Target=0x%X, TimeRem=%d, Delay=0x%X",
                    targetState, transitionMs, delay5Ms);

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Update descriptor */
    pDesc->remainingTimeMs = transitionMs;
    pDesc->delay5Ms = delay5Ms;
    pDesc->updateSource = stateUpdateSrc;

    /* Validate the minimum and maximum lightness of the element. */
    if ((pDesc->pStoredStates[RANGE_MIN_STATE_IDX] != LIGHT_LIGHTNESS_RANGE_PROHIBITED) &&
        (pDesc->pStoredStates[RANGE_MAX_STATE_IDX] != LIGHT_LIGHTNESS_RANGE_PROHIBITED))
    {
      if ( (targetState > 0) && (targetState < pDesc->pStoredStates[RANGE_MIN_STATE_IDX]))
      {
        targetState = pDesc->pStoredStates[RANGE_MIN_STATE_IDX];
      }
      else if (targetState > pDesc->pStoredStates[RANGE_MAX_STATE_IDX])
      {
        targetState = pDesc->pStoredStates[RANGE_MAX_STATE_IDX];
      }
    }

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
      pDesc->transitionType = LIGHT_LIGHTNESS_ACTUAL_TRANSITION;
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
      pDesc->transitionType = LIGHT_LIGHTNESS_ACTUAL_TRANSITION;
    }
    else
    {
      /* Stop transition */
      if ((pDesc->transitionTimer.isStarted) &&
          (pDesc->transitionType == LIGHT_LIGHTNESS_ACTUAL_TRANSITION))
      {
        WsfTimerStop(&pDesc->transitionTimer);
      }

      /* Update Light Lightness state entries in NVM. */
      saveToNVM = TRUE;

      mmdlLightLightnessSrSetPresentState(elementId, pDesc, targetState, stateUpdateSrc);
    }

    /* Save target state in NVM for Power Up. */
    if((pDesc->fNvmSaveStates != NULL) && (saveToNVM == TRUE))
    {
      pDesc->fNvmSaveStates(elementId);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local Light Lightness Linear present state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model.
 *  \param[in] pDesc           Pointer to the model descriptor
 *  \param[in] targetState     Target State for this transaction. See ::mmdlLightLightnessState_t.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightLightnessLinearSrSetPresentState(meshElementId_t elementId,
                                                      mmdlLightLightnessSrDesc_t *pDesc,
                                                      mmdlLightLightnessState_t targetState,
                                                      mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlLightLightnessSrEvent_t event;
  mmdlLightLightnessState_t computedActual;

  /* Update State */
  pDesc->pStoredStates[LINEAR_STATE_IDX] = targetState;

  /* Compute the corresponding Light Lightness Actual value */
  computedActual = square_root(targetState * 65535);

  /* Validate the minimum and maximum lightness of the element. */
  if ((pDesc->pStoredStates[RANGE_MIN_STATE_IDX] != LIGHT_LIGHTNESS_RANGE_PROHIBITED) &&
      (pDesc->pStoredStates[RANGE_MAX_STATE_IDX] != LIGHT_LIGHTNESS_RANGE_PROHIBITED))
  {
    if ( (computedActual > 0) && (computedActual < pDesc->pStoredStates[RANGE_MIN_STATE_IDX]))
    {
      computedActual = pDesc->pStoredStates[RANGE_MIN_STATE_IDX];
    }
    else if (computedActual > pDesc->pStoredStates[RANGE_MAX_STATE_IDX])
    {
      computedActual = pDesc->pStoredStates[RANGE_MAX_STATE_IDX];
    }
  }

  /* Update Light Lightness Last state */
  if(pDesc->pStoredStates[ACTUAL_STATE_IDX] != 0)
  {
    pDesc->pStoredStates[LAST_STATE_IDX] = pDesc->pStoredStates[ACTUAL_STATE_IDX];
  }

  /* Update the Light Lightness Actual state */
  pDesc->pStoredStates[ACTUAL_STATE_IDX] = computedActual;

  /* Publish updated state */
  MmdlLightLightnessSrPublish(elementId);

  /* Check for bindings on this state. Trigger bindings */
  if ((stateUpdateSrc != MMDL_STATE_UPDATED_BY_BIND) && (lightLightnessSrCb.fResolveBind))
  {
    lightLightnessSrCb.fResolveBind(elementId, MMDL_STATE_LT_LTNESS_ACT,
                                    &pDesc->pStoredStates[ACTUAL_STATE_IDX]);
  }

  /* Publish updated value */
  MmdlLightLightnessSrPublishLinear(elementId);

  /* Set event type */
  event.hdr.status = MMDL_SUCCESS;
  event.hdr.event = MMDL_LIGHT_LIGHTNESS_SR_EVENT;
  event.hdr.param = MMDL_LIGHT_LIGHTNESS_LINEAR_SR_STATE_UPDATE_EVENT;

  /* Set event parameters */
  event.statusEvent.elemId = elementId;
  event.statusEvent.lightnessState.state = targetState;
  event.statusEvent.stateUpdateSource = stateUpdateSrc;

  /* Send event to the upper layer */
  lightLightnessSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local Light Lightness Linear state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model.
 *  \param[in] targetState     Target State for this transaction. See ::mmdlLightLightnessState_t.
 *  \param[in] transitionTime  Transition time for this transaction.
 *  \param[in] delay5Ms        Delay for executing transaction. Unit is 5ms.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightLightnessLinearSrSetState(meshElementId_t elementId,
                                        mmdlLightLightnessState_t targetState,
                                        uint32_t transitionMs, uint8_t delay5Ms,
                                        mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlLightLightnessSrDesc_t *pDesc = NULL;
  bool_t saveToNVM = FALSE;

  MMDL_TRACE_INFO3("LIGHT LIGHTNESS SR: Set Linear Target=0x%X, TimeRem=%d, Delay=0x%X",
                    targetState, transitionMs, delay5Ms);

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Update descriptor */
    pDesc->remainingTimeMs = transitionMs;
    pDesc->delay5Ms = delay5Ms;

    /* Update Target State */
    if (pDesc->pStoredStates[TARGET_STATE_IDX] != targetState)
    {
      pDesc->pStoredStates[TARGET_STATE_IDX] = targetState;
      saveToNVM = TRUE;
    }

    /* Check if the set is delayed */
    if (pDesc->delay5Ms > 0)
    {
      /* Start Timer */
      WsfTimerStartMs(&pDesc->transitionTimer, DELAY_5MS_TO_MS(pDesc->delay5Ms));
      pDesc->transitionType = LIGHT_LIGHTNESS_LINEAR_TRANSITION;
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
      pDesc->transitionType = LIGHT_LIGHTNESS_LINEAR_TRANSITION;
    }
    else
    {
      /* Stop transition */
      if ((pDesc->transitionTimer.isStarted) &&
          (pDesc->transitionType == LIGHT_LIGHTNESS_LINEAR_TRANSITION))
      {
        WsfTimerStop(&pDesc->transitionTimer);
      }

      /* Update Light Lightness state entries in NVM. */
      saveToNVM = TRUE;

      mmdlLightLightnessLinearSrSetPresentState(elementId, pDesc, targetState, stateUpdateSrc);
    }

    /* Update Light Lightness state entries in NVM. */
    if ((saveToNVM == TRUE) && pDesc->fNvmSaveStates)
    {
      pDesc->fNvmSaveStates(elementId);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light Lightness Status command to the specified destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] dstAddr      Element address of the destination
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightLightnessSrSendStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                          uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_LIGHT_LIGHTNESS_SR_MDL_ID,
                                   MMDL_LIGHT_LIGHTNESS_STATUS_OPCODE);
  mmdlLightLightnessSrDesc_t *pDesc = NULL;
  uint8_t msgParams[MMDL_LIGHT_LIGHTNESS_STATUS_MAX_LEN];
  uint8_t *pParams, remainingTime;

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = dstAddr;
  msgInfo.ttl = MESH_USE_DEFAULT_TTL;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    pParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT16_TO_BSTREAM(pParams, pDesc->pStoredStates[ACTUAL_STATE_IDX]);

    if ((pDesc->remainingTimeMs != 0) &&
        (pDesc->transitionType == LIGHT_LIGHTNESS_ACTUAL_TRANSITION))
    {
      UINT16_TO_BSTREAM(pParams, pDesc->pStoredStates[TARGET_STATE_IDX]);

      if (pDesc->delay5Ms == 0)
      {
        /* Timer is running the transition */
        if (pDesc->steps > 0)
        {
          /* Transition is divided into steps. Compute remaining time based on remaining steps. */
          remainingTime = MmdlGenDefaultTimeMsToTransTime(pDesc->transitionTimer.ticks *
                                                          WSF_MS_PER_TICK +
                                                          (pDesc->steps - 1) *
                                                          MMDL_TRANSITION_STATE_UPDATE_INTERVAL);
        }
        else
        {
          remainingTime = MmdlGenDefaultTimeMsToTransTime(pDesc->transitionTimer.ticks *
                                                          WSF_MS_PER_TICK);
        }
      }
      else
      {
        /* Timer is running the delay. Transition did not start. */
        remainingTime = MmdlGenDefaultTimeMsToTransTime(pDesc->remainingTimeMs);
      }

      UINT8_TO_BSTREAM(pParams, remainingTime);

      MMDL_TRACE_INFO3("LIGHT LIGHTNESS SR: Send Status Present=0x%X, Target=0x%X, TimeRem=0x%X",
                        pDesc->pStoredStates[ACTUAL_STATE_IDX],
                        pDesc->pStoredStates[TARGET_STATE_IDX], remainingTime);
    }
    else
    {
      MMDL_TRACE_INFO1("LIGHT LIGHTNESS SR: Send Status Present=0x%X",
                        pDesc->pStoredStates[ACTUAL_STATE_IDX]);
    }

    /* Send message to the Mesh Core */
    MeshSendMessage(&msgInfo, msgParams, (uint16_t)(pParams - msgParams),
                    MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                    MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light Lightness Linear Status command to the specified destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] dstAddr      Element address of the destination
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightLightnessLinearSrSendStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                          uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_LIGHT_LIGHTNESS_SR_MDL_ID,
                                        MMDL_LIGHT_LIGHTNESS_LINEAR_STATUS_OPCODE);
  mmdlLightLightnessSrDesc_t *pDesc = NULL;
  uint8_t msgParams[MMDL_LIGHT_LIGHTNESS_LINEAR_STATUS_MAX_LEN];
  uint8_t *pParams, remainingTime;

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = dstAddr;
  msgInfo.ttl = MESH_USE_DEFAULT_TTL;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    pParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT16_TO_BSTREAM(pParams, pDesc->pStoredStates[LINEAR_STATE_IDX]);

    if ((pDesc->remainingTimeMs > 0) &&
        (pDesc->transitionType == LIGHT_LIGHTNESS_LINEAR_TRANSITION))
    {
      UINT16_TO_BSTREAM(pParams, pDesc->pStoredStates[TARGET_STATE_IDX]);

      if (pDesc->delay5Ms == 0)
      {
        /* Timer is running the transition */
        if (pDesc->steps > 0)
        {
          /* Transition is divided into steps. Compute remaining time based on remaining steps. */
          remainingTime = MmdlGenDefaultTimeMsToTransTime(pDesc->transitionTimer.ticks *
                                                          WSF_MS_PER_TICK +
                                                          (pDesc->steps - 1) *
                                                          MMDL_TRANSITION_STATE_UPDATE_INTERVAL);
        }
        else
        {
          remainingTime = MmdlGenDefaultTimeMsToTransTime(pDesc->transitionTimer.ticks *
                                                          WSF_MS_PER_TICK);
        }
      }
      else
      {
        /* Timer is running the delay. Transition did not start. */
        remainingTime = MmdlGenDefaultTimeMsToTransTime(pDesc->remainingTimeMs);
      }

      UINT8_TO_BSTREAM(pParams, remainingTime);

      MMDL_TRACE_INFO3("LIGHT LIGHTNESS SR: Send Status Linear Present=0x%X, Target=0x%X, TimeRem=0x%X",
                        pDesc->pStoredStates[LINEAR_STATE_IDX],
                        pDesc->pStoredStates[TARGET_STATE_IDX], remainingTime);
    }
    else
    {
      MMDL_TRACE_INFO1("LIGHT LIGHTNESS SR: Send Linear Status Present=0x%X",
                        pDesc->pStoredStates[LINEAR_STATE_IDX]);
    }

    /* Send message to the Mesh Core */
    MeshSendMessage(&msgInfo, msgParams, (uint16_t)(pParams - msgParams),
                    MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                    MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light Lightness Last Status command to the specified destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] dstAddr      Element address of the destination
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightLightnessLastSrSendStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                         uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_LIGHT_LIGHTNESS_SR_MDL_ID,
                                   MMDL_LIGHT_LIGHTNESS_LAST_STATUS_OPCODE);
  mmdlLightLightnessSrDesc_t *pDesc = NULL;
  uint8_t msgParams[MMDL_LIGHT_LIGHTNESS_LAST_STATUS_LEN];

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = dstAddr;
  msgInfo.ttl = MESH_USE_DEFAULT_TTL;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Copy the message parameters from the descriptor */
    UINT16_TO_BUF(msgParams, pDesc->pStoredStates[LAST_STATE_IDX]);

    MMDL_TRACE_INFO1("LIGHT LIGHTNESS SR: Send Status Last=0x%X",
                      pDesc->pStoredStates[LAST_STATE_IDX]);

    /* Send message to the Mesh Core instantly */
    MeshSendMessage(&msgInfo, msgParams, MMDL_LIGHT_LIGHTNESS_LAST_STATUS_LEN,
                    MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                    MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Set command.
 *
 *  \param[in] pMsg         Received model message.
 *  \param[in] ackRequired  TRUE if acknowledgment is required in response,  FALSE otherwise.
 *
 *  \return    TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlLightLightnessSrSet(const meshModelMsgRecvEvt_t *pMsg, bool_t ackRequired)
{
  mmdlLightLightnessState_t state;
  mmdlLightLightnessSrDesc_t *pDesc = NULL;
  uint32_t transMs;
  uint32_t delayMs;
  uint8_t tid;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length. It can take only min and max values. */
  if (pMsg->messageParamsLen != MMDL_LIGHT_LIGHTNESS_SET_MAX_LEN &&
      pMsg->messageParamsLen != MMDL_LIGHT_LIGHTNESS_SET_MIN_LEN)
  {
    return FALSE;
  }

  /* Set the state value from pMessageParams buffer. */
  BYTES_TO_UINT16(state, &pMsg->pMessageParams[0]);

  /* Check if it contains optional parameters */
  if (pMsg->messageParamsLen == MMDL_LIGHT_LIGHTNESS_SET_MAX_LEN)
  {
    /* Check prohibited values for Transition Time */
    if (TRANSITION_TIME_STEPS(pMsg->pMessageParams[MMDL_SET_TRANSITION_IDX]) ==
        MMDL_GEN_TR_UNKNOWN)
    {
      return FALSE;
    }
  }

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(pMsg->elementId, &pDesc);

  if (pDesc != NULL)
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
    if (pMsg->messageParamsLen == MMDL_LIGHT_LIGHTNESS_SET_MAX_LEN)
    {
      /* Get Transition time */
      transMs = MmdlGenDefaultTransTimeToMs(pMsg->pMessageParams[MMDL_SET_TRANSITION_IDX]);
      delayMs = pMsg->pMessageParams[MMDL_SET_DELAY_IDX];
      /* Set transition type */
      pDesc->transitionType = LIGHT_LIGHTNESS_ACTUAL_TRANSITION;
    }
    else
    {
      /* Get Default Transition time */
      transMs = MmdlGenDefaultTransGetTime(pMsg->elementId);
      delayMs = 0;
    }

    /* If the target value is the same, do not transition */
    if (pDesc->pStoredStates[ACTUAL_STATE_IDX] == state)
    {
      transMs = 0;
    }

    /* Determine the number of transition steps */
    pDesc->steps = transMs / MMDL_TRANSITION_STATE_UPDATE_INTERVAL;

    if (pDesc->steps > 0)
    {
      /* Compute the transition step increment */
      pDesc->transitionStep = (state - pDesc->pStoredStates[ACTUAL_STATE_IDX]) / pDesc->steps;
    }

    /* Change state */
    mmdlLightLightnessSrSetState(pMsg->elementId, state, transMs, delayMs,
                                 MMDL_STATE_UPDATED_BY_CL);

    return (pDesc->delay5Ms == 0);
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Linear Set command.
 *
 *  \param[in] pMsg         Received model message.
 *  \param[in] ackRequired  TRUE if acknowledgment is required in response,  FALSE otherwise.
 *
 *  \return    TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlLightLightnessLinearSrSet(const meshModelMsgRecvEvt_t *pMsg, bool_t ackRequired)
{
  mmdlLightLightnessState_t state;
  mmdlLightLightnessSrDesc_t *pDesc = NULL;
  uint32_t transMs;
  uint32_t delayMs;
  uint8_t tid;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length. It can take only min and max values. */
  if (pMsg->messageParamsLen != MMDL_LIGHT_LIGHTNESS_LINEAR_SET_MAX_LEN &&
      pMsg->messageParamsLen != MMDL_LIGHT_LIGHTNESS_LINEAR_SET_MIN_LEN)
  {
    return FALSE;
  }

  /* Set the state value from pMessageParams buffer. */
  BYTES_TO_UINT16(state, &pMsg->pMessageParams[0]);

  /* Check if it contains optional parameters */
  if (pMsg->messageParamsLen == MMDL_LIGHT_LIGHTNESS_LINEAR_SET_MAX_LEN)
  {
    /* Check prohibited values for Transition Time */
    if (TRANSITION_TIME_STEPS(pMsg->pMessageParams[MMDL_SET_TRANSITION_IDX]) ==
        MMDL_GEN_TR_UNKNOWN)
    {
      return FALSE;
    }
  }

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(pMsg->elementId, &pDesc);

  if (pDesc != NULL)
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
    if (pMsg->messageParamsLen == MMDL_LIGHT_LIGHTNESS_LINEAR_SET_MAX_LEN)
    {
      /* Get Transition time */
      transMs = MmdlGenDefaultTransTimeToMs(pMsg->pMessageParams[MMDL_SET_TRANSITION_IDX]);
      delayMs = pMsg->pMessageParams[MMDL_SET_DELAY_IDX];
      /* Set transition type */
      pDesc->transitionType = LIGHT_LIGHTNESS_LINEAR_TRANSITION;
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
      pDesc->transitionStep = (state - pDesc->pStoredStates[LINEAR_STATE_IDX]) / pDesc->steps;
    }

    /* Change state */
    mmdlLightLightnessLinearSrSetState(pMsg->elementId, state, transMs, delayMs,
                                       MMDL_STATE_UPDATED_BY_CL);

    return (pDesc->delay5Ms == 0);
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light Lightness Default Status command to the specified destination address.
 *
 *  \param[in] modelId        Model identifier.
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  Indicates if initial destination address was unicast.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessDefaultSrSendStatus(uint16_t modelId, meshElementId_t elementId,
                                           meshAddress_t dstAddr, uint16_t appKeyIndex,
                                           bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(modelId, MMDL_LIGHT_LIGHTNESS_DEFAULT_STATUS_OPCODE);
  mmdlLightLightnessSrDesc_t *pDesc = NULL;
  uint8_t msgParams[MMDL_LIGHT_LIGHTNESS_DEFAULT_STATUS_LEN];

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Fill in the msg info parameters */
    msgInfo.elementId = elementId;
    msgInfo.dstAddr = dstAddr;
    msgInfo.ttl = MESH_USE_DEFAULT_TTL;
    msgInfo.appKeyIndex = appKeyIndex;

    /* Copy the message parameters from the descriptor */
    UINT16_TO_BUF(msgParams, pDesc->pStoredStates[DEFAULT_STATE_IDX]);

    MMDL_TRACE_INFO1("LIGHT LIGHTNESS SR: Send Status Default=0x%X",
                      pDesc->pStoredStates[DEFAULT_STATE_IDX]);

    /* Send message to the Mesh Core instantly */
    MeshSendMessage(&msgInfo, msgParams, MMDL_LIGHT_LIGHTNESS_DEFAULT_STATUS_LEN,
                    MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                    MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Local setter of the Light Lightness Default state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model
 *  \param[in] defaultState    New Light Lightness Default state.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessDefaultSrSetState(meshElementId_t elementId,
                                         mmdlLightLightnessState_t targetState,
                                         mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlLightLightnessSrStateUpdate_t event;
  mmdlLightLightnessSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO1("LIGHT LIGHTNESS SR: Set Default=0x%X", targetState);

  /* Set event type */
  event.elemId = elementId;
  event.stateUpdateSource = stateUpdateSrc;
  event.hdr.event = MMDL_LIGHT_LIGHTNESS_SR_EVENT;
  event.hdr.param = MMDL_LIGHT_LIGHTNESS_DEFAULT_SR_STATE_UPDATE_EVENT;

  /* Set target state */
  event.lightnessState.state = targetState;

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(elementId, &pDesc);

  if (pDesc == NULL)
  {
    event.hdr.status = MMDL_INVALID_ELEMENT;
  }
  else
  {
    event.hdr.status = MMDL_SUCCESS;

    /* Update Target State */
    pDesc->pStoredStates[DEFAULT_STATE_IDX] = targetState;

    /* Update Light Lightness state entries in NVM. */
    if(pDesc->fNvmSaveStates)
    {
      pDesc->fNvmSaveStates(elementId);
    }
  }

  /* Send event to the upper layer */
  lightLightnessSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Local getter of the Light Lightness Default state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    Lightness state. See::mmdlLightLightnessState_t.
 */
/*************************************************************************************************/
mmdlLightLightnessState_t mmdlLightLightnessDefaultSrGetState(meshElementId_t elementId)
{
  mmdlLightLightnessSrDesc_t *pDesc = NULL;

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    return pDesc->pStoredStates[DEFAULT_STATE_IDX];
  }
  else
  {
    return MMDL_LIGHT_LIGHTNESS_STATE_PROHIBITED;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Local getter of the Light Lightness Actual state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    Lightness state. See::mmdlLightLightnessState_t.
 */
/*************************************************************************************************/
mmdlLightLightnessState_t mmdlLightLightnessActualSrGetState(meshElementId_t elementId)
{
  mmdlLightLightnessSrDesc_t *pDesc = NULL;

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    return pDesc->pStoredStates[ACTUAL_STATE_IDX];
  }
  else
  {
    return MMDL_LIGHT_LIGHTNESS_STATE_PROHIBITED;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Local getter of the Light Lightness Last state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    Lightness state. See::mmdlLightLightnessState_t.
 */
/*************************************************************************************************/
mmdlLightLightnessState_t mmdlLightLightnessLastSrGetState(meshElementId_t elementId)
{
  mmdlLightLightnessSrDesc_t *pDesc = NULL;

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    return pDesc->pStoredStates[LAST_STATE_IDX];
  }
  else
  {
    return MMDL_LIGHT_LIGHTNESS_STATE_PROHIBITED;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light Lightness Range Status command to the specified destination address.
 *
 *  \param[in] modelId        Model identifier.
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  Indicates if initial destination address was unicast.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessRangeSrSendStatus(uint16_t modelId, meshElementId_t elementId,
                                         meshAddress_t dstAddr, uint16_t appKeyIndex,
                                         bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(modelId, MMDL_LIGHT_LIGHTNESS_RANGE_STATUS_OPCODE);
  uint8_t msgParams[MMDL_LIGHT_LIGHTNESS_RANGE_STATUS_LEN];
  mmdlLightLightnessSrDesc_t *pDesc = NULL;
  uint8_t *p = msgParams;

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = dstAddr;
  msgInfo.ttl = MESH_USE_DEFAULT_TTL;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Copy the message parameters from the descriptor */
    UINT8_TO_BSTREAM(p, 0);

    UINT16_TO_BSTREAM(p, pDesc->pStoredStates[RANGE_MIN_STATE_IDX]);
    UINT16_TO_BSTREAM(p, pDesc->pStoredStates[RANGE_MAX_STATE_IDX]);

    MMDL_TRACE_INFO2("LIGHT LIGHTNESS SR: Send Status RangeMinLightness=0x%X, RangeMaxLightness=0x%X",
                     pDesc->pStoredStates[RANGE_MIN_STATE_IDX],
                     pDesc->pStoredStates[RANGE_MAX_STATE_IDX]);

    /* Send message to the Mesh Core instantly */
    MeshSendMessage(&msgInfo, (const uint8_t *)&msgParams, MMDL_LIGHT_LIGHTNESS_RANGE_STATUS_LEN,
                    MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                    MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Local setter of the Light Lightness Range state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model
 *  \param[in] pRangeState     New Light Lightness Range state.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    TRUE if handled successful, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t mmdlLightLightnessRangeSrSetState(meshElementId_t elementId,
                                 const mmdlLightLightnessRangeState_t *pRangeState,
                                       mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlLightLightnessSrStateUpdate_t event;
  mmdlLightLightnessSrDesc_t *pDesc = NULL;
  bool_t retVal = FALSE;

  MMDL_TRACE_INFO2("LIGHT LIGHTNESS SR: Set RangeMin=0x%X, RangeMax=0x%X",
                    pRangeState->rangeMin, pRangeState->rangeMax);

  /* Set event parameters */
  event.elemId = elementId;
  event.stateUpdateSource = stateUpdateSrc;
  event.lightnessState.rangeState.rangeMin = pRangeState->rangeMin;
  event.lightnessState.rangeState.rangeMax = pRangeState->rangeMax;
  event.hdr.event = MMDL_LIGHT_LIGHTNESS_SR_EVENT;
  event.hdr.param = MMDL_LIGHT_LIGHTNESS_RANGE_SR_STATE_UPDATE_EVENT;

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Validate the Range values. */
    if ((pRangeState->rangeMin != 0) && (pRangeState->rangeMax != 0) &&
        (pRangeState->rangeMin < pRangeState->rangeMax))
    {
      event.hdr.status = MMDL_SUCCESS;

      /* Update Range State */
      pDesc->pStoredStates[RANGE_MIN_STATE_IDX] = pRangeState->rangeMin;
      pDesc->pStoredStates[RANGE_MAX_STATE_IDX] = pRangeState->rangeMax;

      if ( pDesc->pStoredStates[ACTUAL_STATE_IDX] < pDesc->pStoredStates[RANGE_MIN_STATE_IDX])
      {
        mmdlLightLightnessSrSetState(elementId, pDesc->pStoredStates[RANGE_MIN_STATE_IDX], 0, 0,
                                     MMDL_STATE_UPDATED_BY_CL);
      }
      else if (pDesc->pStoredStates[ACTUAL_STATE_IDX] > pDesc->pStoredStates[RANGE_MAX_STATE_IDX])
      {
        mmdlLightLightnessSrSetState(elementId, pDesc->pStoredStates[RANGE_MAX_STATE_IDX], 0, 0,
                                     MMDL_STATE_UPDATED_BY_CL);
      }
      else
      {
        /* Update Light Lightness state entries in NVM. */
        if(pDesc->fNvmSaveStates)
        {
          pDesc->fNvmSaveStates(elementId);
        }
      }

      retVal = TRUE;
    }
    else
    {
      event.hdr.status = MMDL_INVALID_PARAM;
    }
  }
  else
  {
    event.hdr.status = MMDL_INVALID_ELEMENT;
  }

  /* Send event to the upper layer */
  lightLightnessSrCb.recvCback((wsfMsgHdr_t *)&event);

  return retVal;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    TRUE if handled successful, FALSE otherwise.
 */
/*************************************************************************************************/
void mmdlLightLightnessSrHandleGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    mmdlLightLightnessSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                                   pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessSrHandleSet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  if (mmdlLightLightnessSrSet(pMsg, TRUE))
  {
    /* Send Status message as a response to the Set message */
    mmdlLightLightnessSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                                   pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessSrHandleSetNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  (void)mmdlLightLightnessSrSet(pMsg, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Linear Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    TRUE if handled successful, FALSE otherwise.
 */
/*************************************************************************************************/
void mmdlLightLightnessLinearSrHandleGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Linear Get message */
    mmdlLightLightnessLinearSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                                         pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Linear Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessLinearSrHandleSet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  if (mmdlLightLightnessLinearSrSet(pMsg, TRUE))
  {
    /* Send Status message as a response to the Linear Set message */
    mmdlLightLightnessLinearSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                                         pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Linear Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessLinearSrHandleSetNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  (void)mmdlLightLightnessLinearSrSet(pMsg, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Last Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessLastSrHandleGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    mmdlLightLightnessLastSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                                       pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Default Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessDefaultSrHandleGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    mmdlLightLightnessDefaultSrSendStatus(MMDL_LIGHT_LIGHTNESS_SR_MDL_ID, pMsg->elementId,
                                          pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Range Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessRangeSrHandleGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    mmdlLightLightnessRangeSrSendStatus(MMDL_LIGHT_LIGHTNESS_SR_MDL_ID, pMsg->elementId,
                                        pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Light Lightness Server timer callback.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightLightnessSrHandleTmrCback(uint8_t elementId)
{
  mmdlLightLightnessSrDesc_t *pDesc = NULL;

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(elementId, &pDesc);

  /* Transition timeout. Move to Target State. */
  if (pDesc != NULL)
  {
    if (pDesc->delay5Ms != 0)
    {
      /* Reset Delay */
      pDesc->delay5Ms = 0;

      /* Timeout. Set state. */
      if (pDesc->transitionType == LIGHT_LIGHTNESS_ACTUAL_TRANSITION)
      {
        mmdlLightLightnessSrSetState(elementId, pDesc->pStoredStates[TARGET_STATE_IDX],
                                     pDesc->remainingTimeMs, 0, pDesc->updateSource);

        /* Send Status if it was a delayed Acknowledged Set */
        if (pDesc->ackPending)
        {
          mmdlLightLightnessSrSendStatus(elementId, pDesc->srcAddr, pDesc->ackAppKeyIndex,
                                         pDesc->ackForUnicast);
        }
      }
      else
      {
        mmdlLightLightnessLinearSrSetState(elementId, pDesc->pStoredStates[TARGET_STATE_IDX],
                                           pDesc->remainingTimeMs, 0, pDesc->updateSource);

        /* Send Status if it was a delayed Acknowledged Set */
        if (pDesc->ackPending)
        {
          mmdlLightLightnessLinearSrSendStatus(elementId, pDesc->srcAddr, pDesc->ackAppKeyIndex,
                                               pDesc->ackForUnicast);
        }
      }
    }
    else if (pDesc->remainingTimeMs != 0)
    {
      uint16_t state;
      uint32_t remainingTimeMs = pDesc->remainingTimeMs - MMDL_TRANSITION_STATE_UPDATE_INTERVAL;

      /* Transition timeout. Move to Target State. */
      if (pDesc->transitionType == LIGHT_LIGHTNESS_ACTUAL_TRANSITION)
      {
        if (pDesc->steps > 0)
        {
          /* Transition is divided into steps. Decrement the remaining steps */
          pDesc->steps--;

          /* Compute intermediate state value */
          state = pDesc->pStoredStates[ACTUAL_STATE_IDX] + pDesc->transitionStep;

          /* Update present state only. */
          mmdlLightLightnessSrSetPresentState(elementId, pDesc, state, pDesc->updateSource);

          if (pDesc->steps == 1)
          {
            /* Next is the last step.
             * Program the remaining time (can be more than MMDL_TRANSITION_STATE_UPDATE_INTERVAL).
             * Also, the last step increment can be greater then the intermediate ones.
             */
            pDesc->steps = 0;
          }

          /* Program next transition */
          mmdlLightLightnessSrSetState(elementId, pDesc->pStoredStates[TARGET_STATE_IDX],
                                       remainingTimeMs, 0, pDesc->updateSource);
        }
        else
        {
          mmdlLightLightnessSrSetState(elementId, pDesc->pStoredStates[TARGET_STATE_IDX], 0, 0,
                                       pDesc->updateSource);
        }
      }
      else
      {
        if (pDesc->steps > 0)
        {
          /* Transition is divided into steps. Decrement the remaining steps */
          pDesc->steps--;

          /* Compute intermediate state value */
          state = pDesc->pStoredStates[LINEAR_STATE_IDX] + pDesc->transitionStep;

          /* Update present state only. */
          mmdlLightLightnessLinearSrSetPresentState(elementId, pDesc, state, pDesc->updateSource);

          if (pDesc->steps == 1)
          {
            /* Next is the last step.
             * Program the remaining time (can be more than MMDL_TRANSITION_STATE_UPDATE_INTERVAL).
             * Also, the last step increment can be greater then the intermediate ones.
             */
            pDesc->steps = 0;
          }

          /* Program next transition */
          mmdlLightLightnessLinearSrSetState(elementId, pDesc->pStoredStates[TARGET_STATE_IDX],
                                             remainingTimeMs, 0, pDesc->updateSource);
        }
        else
        {
          mmdlLightLightnessLinearSrSetState(elementId, pDesc->pStoredStates[TARGET_STATE_IDX], 0,
                                             0, pDesc->updateSource);
        }
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Light Lightness Server message timer callback.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightLightnessSrHandleMsgTmrCback(uint8_t elementId)
{
  mmdlLightLightnessSrDesc_t *pDesc = NULL;

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Reset source address and transaction ID for last stored transaction */
    pDesc->srcAddr = MESH_ADDR_TYPE_UNASSIGNED;
  }
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
static void mmdlLightLightnessSrStoreScene(void *pDesc, uint8_t sceneIdx)
{
  mmdlLightLightnessSrDesc_t *pLtLtnessDesc = (mmdlLightLightnessSrDesc_t *)pDesc;

  MMDL_TRACE_INFO1("LIGHT LIGHTNESS SR: Store lightness=%d",
                   pLtLtnessDesc->pStoredStates[ACTUAL_STATE_IDX]);

  /* Store present states */
  pLtLtnessDesc->pStoredStates[SCENE_STATE_IDX + ACTUAL_STATE_IDX + (sceneIdx << 1)] =
        pLtLtnessDesc->pStoredStates[ACTUAL_STATE_IDX];
  pLtLtnessDesc->pStoredStates[SCENE_STATE_IDX + LINEAR_STATE_IDX + (sceneIdx << 1)] =
        pLtLtnessDesc->pStoredStates[LINEAR_STATE_IDX];
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
static void mmdlLightLightnessSrRecallScene(meshElementId_t elementId, uint8_t sceneIdx,
                                            uint32_t transitionMs)
{
  mmdlLightLightnessSrDesc_t *pDesc = NULL;

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    MMDL_TRACE_INFO3("LIGHT LIGHTNESS SR: Recall elemid=%d lightness=%d transMs=%d",
        elementId, pDesc->pStoredStates[SCENE_STATE_IDX +  ACTUAL_STATE_IDX + (sceneIdx << 1)],
        transitionMs);

    /* Overwrite inconsistent values due to square error root.  */
    pDesc->pStoredStates[LINEAR_STATE_IDX] =
        pDesc->pStoredStates[SCENE_STATE_IDX + LINEAR_STATE_IDX + (sceneIdx << 1)];

    /* Recall states */
    mmdlLightLightnessSrSetState(elementId,
        pDesc->pStoredStates[SCENE_STATE_IDX + ACTUAL_STATE_IDX + (sceneIdx << 1)], transitionMs, 0,
        MMDL_STATE_UPDATED_BY_SCENE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between the Generic OnPowerUp and a Light Lightness Actual state as
 *             a result of a Power Up procedure.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveOnPowerUp2LtLtnessAct(meshElementId_t tgtElementId, void *pStateValue)
{
  mmdlLightLightnessState_t state;
  mmdlLightLightnessSrDesc_t *pDesc = NULL;
  mmdlGenOnPowerUpState_t powerUpState;

  powerUpState = *(mmdlGenOnPowerUpState_t*)pStateValue;

  mmdlLightLightnessSrGetDesc(tgtElementId, &pDesc);

  if (!pDesc)
  {
    return;
  }

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
      if (pDesc->pStoredStates[ACTUAL_STATE_IDX] != pDesc->pStoredStates[TARGET_STATE_IDX])
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
  mmdlLightLightnessSrSetState(tgtElementId, state, 0, 0, MMDL_STATE_UPDATED_BY_BIND);

  /* Update implicit bind with GenLevel. */
  MmdlGenLevelSrSetBoundState(tgtElementId, (int16_t)(state - 0x8000));
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between a Generic On Off state and a Light Lightness Actual state as
 *             a result of an updated Light Lightness Actual state.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveLtLtnessAct2GenOnOff(meshElementId_t tgtElementId, void *pStateValue)
{
  uint16_t ltLtnessActValue = *(uint16_t*)pStateValue;

  /* Update Generic On Off State on target element */
  MmdlGenOnOffSrSetBoundState(tgtElementId, (ltLtnessActValue > 0));
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between a Light Lightness Actual state and a Generic Level state as
 *             a result of an updated Light Lightness Actual state.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveLtLtnessAct2GenLevel(meshElementId_t tgtElementId, void *pStateValue)
{
  uint16_t powerLevel;
  int16_t level;

  powerLevel = *(uint16_t*)pStateValue;
  level = powerLevel - 0x8000;

  /* Update Generic Level state on target element */
  MmdlGenLevelSrSetBoundState(tgtElementId, level);
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between a Generic On Off state and a Light Lightness Actual State as
 *             a result of an updated Generic On Off state.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveGenOnOff2LtLtnessAct(meshElementId_t tgtElementId, void *pStateValue)
{
  mmdlLightLightnessState_t state;
  mmdlLightLightnessSrDesc_t *pDesc = NULL;
  mmdlGenOnOffState_t onOffState;

  mmdlLightLightnessSrGetDesc(tgtElementId, &pDesc);

  if (!pDesc)
  {
    return;
  }

  onOffState = *(mmdlGenOnOffState_t*)pStateValue;

  if (onOffState == MMDL_GEN_ONOFF_STATE_OFF)
  {
    state = 0;
  }
  else if (pDesc->pStoredStates[DEFAULT_STATE_IDX] != 0)
  {
    state = pDesc->pStoredStates[DEFAULT_STATE_IDX];
  }
  else
  {
    state = pDesc->pStoredStates[LAST_STATE_IDX];
  }

  /* Change state locally. No transition time or delay is allowed. */
  mmdlLightLightnessSrSetState(tgtElementId, state, 0, 0, MMDL_STATE_UPDATED_BY_BIND);
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between a Light Lightness Actual state and a Generic Level state as
 *             a result of an updated Generic Level state.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveGenLevel2LtLtnessAct(meshElementId_t tgtElementId, void *pStateValue)
{
  int16_t level = *(int16_t*)pStateValue;

  /* Change state locally. No transition time or delay is allowed. */
  mmdlLightLightnessSrSetState(tgtElementId, level + 0x8000, 0, 0, MMDL_STATE_UPDATED_BY_BIND);
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Light Lightness Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrInit(void)
{
  mmdlLightLightnessSrDesc_t *pDesc = NULL;
  meshElementId_t elementId;

  MMDL_TRACE_INFO0("LIGHT LIGHTNESS SR: init");

  /* Set event callbacks */
  lightLightnessSrCb.fStoreScene = mmdlLightLightnessSrStoreScene;
  lightLightnessSrCb.fRecallScene = mmdlLightLightnessSrRecallScene;
  lightLightnessSrCb.fResolveBind = MmdlBindResolve;
  lightLightnessSrCb.recvCback = MmdlEmptyCback;

  /* Initialize timers */
  for (elementId = 0; elementId < pMeshConfig->elementArrayLen; elementId++)
  {
    /* Get the model instance descriptor */
    mmdlLightLightnessSrGetDesc(elementId, &pDesc);

    if (pDesc != NULL)
    {
      pDesc->srcAddr = MESH_ADDR_TYPE_UNASSIGNED;

      /* Set transition timer parameters */
      pDesc->transitionTimer.handlerId = mmdlLightLightnessSrHandlerId;
      pDesc->transitionTimer.msg.event = MMDL_LIGHT_LIGHTNESS_SR_EVT_TMR_CBACK;
      pDesc->transitionTimer.msg.param = elementId;

      /* Set msg Received timer parameters */
      pDesc->msgRcvdTimer.handlerId = mmdlLightLightnessSrHandlerId;
      pDesc->msgRcvdTimer.msg.event = MMDL_LIGHT_LIGHTNESS_SR_MSG_RCVD_TMR_CBACK;
      pDesc->msgRcvdTimer.msg.param = elementId;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Light Lightness Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Light Lightness Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrHandlerInit(wsfHandlerId_t handlerId)
{
  /* Set handler ID */
  mmdlLightLightnessSrHandlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Light Lightness Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrHandler(wsfMsgHdr_t *pMsg)
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
        if (MESH_OPCODE_SIZE(pModelMsg->msgRecvEvt.opCode) == MMDL_LIGHT_LIGHTNESS_OPCODES_SIZE)
        {
          /* Match the received opcode */
          for (opcodeIdx = 0; opcodeIdx < MMDL_LIGHT_LIGHTNESS_SR_NUM_RCVD_OPCODES; opcodeIdx++)
          {
            if (!memcmp(&mmdlLightLightnessSrRcvdOpcodes[opcodeIdx],
                        pModelMsg->msgRecvEvt.opCode.opcodeBytes,
                        MMDL_LIGHT_LIGHTNESS_OPCODES_SIZE))
            {
              /* Process message */
              (void)mmdlLightLightnessSrHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
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
          MmdlLightLightnessSrPublish(pModelMsg->periodicPubEvt.elementId);
        }
        break;

      case MMDL_LIGHT_LIGHTNESS_SR_EVT_TMR_CBACK:
        mmdlLightLightnessSrHandleTmrCback((uint8_t)pMsg->param);
        break;

      case MMDL_LIGHT_LIGHTNESS_SR_MSG_RCVD_TMR_CBACK:
        mmdlLightLightnessSrHandleMsgTmrCback((uint8_t)pMsg->param);
        break;

      default:
        MMDL_TRACE_WARN0("LIGHT LIGHTNESS SR: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publish a Light Lightness Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrPublish(meshElementId_t elementId)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_LIGHT_LIGHTNESS_SR_MDL_ID,
                                             MMDL_LIGHT_LIGHTNESS_STATUS_OPCODE);
  mmdlLightLightnessSrDesc_t *pDesc = NULL;
  uint8_t msgParams[MMDL_LIGHT_LIGHTNESS_STATUS_MAX_LEN];
  uint8_t *pParams = msgParams;
  uint8_t tranTime;

  /* Fill in the msg info parameters */
  pubMsgInfo.elementId = elementId;

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Copy the message parameters from the descriptor */
    UINT16_TO_BSTREAM(pParams, pDesc->pStoredStates[ACTUAL_STATE_IDX]);

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
    }

    MMDL_TRACE_INFO1("LIGHT LIGHTNESS SR: Publish Actual=0x%X",
                      pDesc->pStoredStates[ACTUAL_STATE_IDX]);

    /* Send message to the Mesh Core */
    MeshPublishMessage(&pubMsgInfo, msgParams, (uint16_t)(pParams - msgParams));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publish a Light Lightness Linear Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrPublishLinear(meshElementId_t elementId)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_LIGHT_LIGHTNESS_SR_MDL_ID,
                                                  MMDL_LIGHT_LIGHTNESS_LINEAR_STATUS_OPCODE);
  mmdlLightLightnessSrDesc_t *pDesc = NULL;
  uint8_t msgParams[MMDL_LIGHT_LIGHTNESS_LINEAR_STATUS_MAX_LEN];
  uint8_t remainingTime, *pParams = msgParams;

  /* Fill in the msg info parameters */
  pubMsgInfo.elementId = elementId;

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    pParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT16_TO_BSTREAM(pParams, pDesc->pStoredStates[LINEAR_STATE_IDX]);

    if ((pDesc->remainingTimeMs > 0) &&
        (pDesc->transitionType == LIGHT_LIGHTNESS_LINEAR_TRANSITION))
    {
      UINT16_TO_BSTREAM(pParams, pDesc->pStoredStates[TARGET_STATE_IDX]);

      if (pDesc->delay5Ms == 0)
      {
        /* Timer is running the transition */
        if (pDesc->steps > 0)
        {
          /* Transition is divided into steps. Compute remaining time based on remaining steps. */
          remainingTime = MmdlGenDefaultTimeMsToTransTime(pDesc->transitionTimer.ticks * WSF_MS_PER_TICK +
                          (pDesc->steps - 1) * MMDL_TRANSITION_STATE_UPDATE_INTERVAL);
        }
        else
        {
          remainingTime = MmdlGenDefaultTimeMsToTransTime(pDesc->transitionTimer.ticks * WSF_MS_PER_TICK);
        }
      }
      else
      {
        /* Timer is running the delay. Transition did not start. */
        remainingTime = MmdlGenDefaultTimeMsToTransTime(pDesc->remainingTimeMs);
      }

      UINT8_TO_BSTREAM(pParams, remainingTime);

      MMDL_TRACE_INFO3("LIGHT LIGHTNESS SR: Publish Linear Present=0x%X, Target=0x%X, TimeRem=0x%X",
                        pDesc->pStoredStates[LINEAR_STATE_IDX],
                        pDesc->pStoredStates[TARGET_STATE_IDX], remainingTime);
    }
    else
    {
      MMDL_TRACE_INFO1("LIGHT LIGHTNESS SR: Publish Linear Present=0x%X",
                        pDesc->pStoredStates[LINEAR_STATE_IDX]);
    }

    /* Send message to the Mesh Core */
    MeshPublishMessage(&pubMsgInfo, msgParams, (uint16_t)(pParams - msgParams));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Get the local Light Lightness Actual state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrGetState(meshElementId_t elementId)
{
  mmdlLightLightnessSrEvent_t event;
  mmdlLightLightnessSrDesc_t *pDesc = NULL;

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(elementId, &pDesc);

  /* Set event type */
  event.hdr.event = MMDL_LIGHT_LIGHTNESS_SR_EVENT;
  event.hdr.param = MMDL_LIGHT_LIGHTNESS_SR_CURRENT_STATE_EVENT;

  /* Set event parameters */
  event.currentStateEvent.elemId = elementId;

  if (pDesc == NULL)
  {
    /* No descriptor found on element */
    event.hdr.status = MMDL_INVALID_ELEMENT;

    /* Zero out parameters */
    event.currentStateEvent.lightnessState.state = 0;
  }
  else
  {
    /* Descriptor found on element */
    event.hdr.status = MMDL_SUCCESS;

    /* Set event parameters */
    event.currentStateEvent.lightnessState.state = pDesc->pStoredStates[ACTUAL_STATE_IDX];
  }

  /* Send event to the upper layer */
  lightLightnessSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local Light Lightness Actual state.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] targetState  Target State for this transaction. See ::mmdlLightLightnessState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrSetState(meshElementId_t elementId,
                                  mmdlLightLightnessState_t targetState)
{
    /* Change state locally. No transition time or delay required. */
    mmdlLightLightnessSrSetState(elementId, targetState, 0, 0, MMDL_STATE_UPDATED_BY_APP);
}

/*************************************************************************************************/
/*!
 *  \brief     Get the local Light Lightness Linear state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessLinearSrGetState(meshElementId_t elementId)
{
  mmdlLightLightnessSrEvent_t event;
  mmdlLightLightnessSrDesc_t *pDesc = NULL;

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(elementId, &pDesc);

  /* Set event type */
  event.hdr.event = MMDL_LIGHT_LIGHTNESS_SR_EVENT;
  event.hdr.param = MMDL_LIGHT_LIGHTNESS_LINEAR_SR_CURRENT_STATE_EVENT;

  /* Set event parameters */
  event.currentStateEvent.elemId = elementId;

  if (pDesc == NULL)
  {
    /* No descriptor found on element */
    event.hdr.status = MMDL_INVALID_ELEMENT;

    /* Zero out parameters */
    event.currentStateEvent.lightnessState.state = 0;
  }
  else
  {
    /* Descriptor found on element */
    event.hdr.status = MMDL_SUCCESS;

    /* Set event parameters */
    event.currentStateEvent.lightnessState.state = pDesc->pStoredStates[LINEAR_STATE_IDX];
  }

  /* Send event to the upper layer */
  lightLightnessSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local Light Lightness Linear state.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] targetState  Target State for this transaction. See ::mmdlLightLightnessState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessLinearSrSetState(meshElementId_t elementId,
                                  mmdlLightLightnessState_t targetState)
{
  /* Change state locally. No transition time or delay required. */
  mmdlLightLightnessLinearSrSetState(elementId, targetState, 0, 0, MMDL_STATE_UPDATED_BY_APP);
}

/*************************************************************************************************/
/*!
 *  \brief     Get the local Light Lightness Last state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessLastSrGetState(meshElementId_t elementId)
{
  mmdlLightLightnessSrEvent_t event;
  mmdlLightLightnessSrDesc_t *pDesc = NULL;

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(elementId, &pDesc);

  /* Set event type */
  event.hdr.event = MMDL_LIGHT_LIGHTNESS_SR_EVENT;
  event.hdr.param = MMDL_LIGHT_LIGHTNESS_LAST_SR_CURRENT_STATE_EVENT;

  /* Set event parameters */
  event.currentStateEvent.elemId = elementId;

  if (pDesc == NULL)
  {
    /* No descriptor found on element */
    event.hdr.status = MMDL_INVALID_ELEMENT;

    /* Zero out parameters */
    event.currentStateEvent.lightnessState.state = 0;
  }
  else
  {
    /* Descriptor found on element */
    event.hdr.status = MMDL_SUCCESS;

    /* Set event parameters */
    event.currentStateEvent.lightnessState.state = pDesc->pStoredStates[LAST_STATE_IDX];
  }

  /* Send event to the upper layer */
  lightLightnessSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Get the local Light Lightness Default state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessDefaultSrGetState(meshElementId_t elementId)
{
  mmdlLightLightnessSrEvent_t event;
  mmdlLightLightnessSrDesc_t *pDesc = NULL;

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(elementId, &pDesc);

  /* Set event type */
  event.hdr.event = MMDL_LIGHT_LIGHTNESS_SR_EVENT;
  event.hdr.param = MMDL_LIGHT_LIGHTNESS_DEFAULT_SR_CURRENT_STATE_EVENT;

  /* Set event parameters */
  event.currentStateEvent.elemId = elementId;

  if (pDesc == NULL)
  {
    /* No descriptor found on element */
    event.hdr.status = MMDL_INVALID_ELEMENT;

    /* Zero out parameters */
    event.currentStateEvent.lightnessState.state = 0;
  }
  else
  {
    /* Descriptor found on element */
    event.hdr.status = MMDL_SUCCESS;

    /* Set event parameters */
    event.currentStateEvent.lightnessState.state = pDesc->pStoredStates[DEFAULT_STATE_IDX];
  }

  /* Send event to the upper layer */
  lightLightnessSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Get the local Light Lightness Range state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessRangeSrGetState(meshElementId_t elementId)
{
  mmdlLightLightnessSrEvent_t event;
  mmdlLightLightnessSrDesc_t *pDesc = NULL;

  /* Get the model instance descriptor */
  mmdlLightLightnessSrGetDesc(elementId, &pDesc);

  /* Set event type */
  event.hdr.event = MMDL_LIGHT_LIGHTNESS_SR_EVENT;
  event.hdr.param = MMDL_LIGHT_LIGHTNESS_RANGE_SR_CURRENT_STATE_EVENT;

  /* Set event parameters */
  event.currentStateEvent.elemId = elementId;

  if (pDesc == NULL)
  {
    /* No descriptor found on element */
    event.hdr.status = MMDL_INVALID_ELEMENT;

    /* Zero out parameters */
    event.currentStateEvent.lightnessState.rangeState.rangeMin = 0;
    event.currentStateEvent.lightnessState.rangeState.rangeMax = 0;
  }
  else
  {
    /* Descriptor found on element */
    event.hdr.status = MMDL_SUCCESS;

    /* Set event parameters */
    event.currentStateEvent.elemId = elementId;
    event.currentStateEvent.lightnessState.rangeState.rangeMin = pDesc->pStoredStates[RANGE_MIN_STATE_IDX];
    event.currentStateEvent.lightnessState.rangeState.rangeMax = pDesc->pStoredStates[RANGE_MAX_STATE_IDX];
  }

  /* Send event to the upper layer */
  lightLightnessSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local state as a result of a Generic Level binding. The set is instantaneous
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] state      New State. See ::mmdlLightLightnessState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrSetBoundState(meshElementId_t elementId, mmdlLightLightnessState_t state)
{
  /* Change state locally. No transition time or delay is allowed. */
  mmdlLightLightnessSrSetState(elementId, state, 0, 0, MMDL_STATE_UPDATED_BY_BIND);
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
void MmdlLightLightnessSrStoreScene(void *pDesc, uint8_t sceneIdx)
{
  if (lightLightnessSrCb.fStoreScene != NULL)
  {
    lightLightnessSrCb.fStoreScene(pDesc, sceneIdx);
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
void MmdlLightLightnessSrRecallScene(meshElementId_t elementId, uint8_t sceneIdx,
                                     uint32_t transitionMs)
{
  if (lightLightnessSrCb.fRecallScene != NULL)
  {
    lightLightnessSrCb.fRecallScene(elementId, sceneIdx, transitionMs);
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
void MmdlLightLightnessSrRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback*/
  if (recvCback != NULL)
  {
    lightLightnessSrCb.recvCback = recvCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Light Lightness Actual State and a Generic OnPowerUp state.
 *
 *  \param[in] ltElemId         Element identifier where the Light Lightness Actual state resides.
 *  \param[in] onPowerUpElemId  Element identifier where the OnPowerUp state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrBind2OnPowerUp(meshElementId_t onPowerUpElemId, meshElementId_t ltElemId)
{
  /* Add Generic Power OnOff -> Light Lightness Actual binding */
  MmdlAddBind(MMDL_STATE_GEN_ONPOWERUP, MMDL_STATE_LT_LTNESS_ACT, onPowerUpElemId, ltElemId,
              mmdlBindResolveOnPowerUp2LtLtnessAct);
}

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Light Lightness Actual State and a Generic Level state.
 *
 *  \param[in] ltElemId   Element identifier where the Light Lightness Actual state resides.
 *  \param[in] glvElemId  Element identifier where the Generic Level state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrBind2GenLevel(meshElementId_t ltElemId, meshElementId_t glvElemId)
{
  /* Add Light Lightness Actual -> Generic Level binding */
  MmdlAddBind(MMDL_STATE_LT_LTNESS_ACT, MMDL_STATE_GEN_LEVEL, ltElemId, glvElemId,
              mmdlBindResolveLtLtnessAct2GenLevel);

  /* Add Generic Level -> Light Lightness Actual binding */
  MmdlAddBind(MMDL_STATE_GEN_LEVEL, MMDL_STATE_LT_LTNESS_ACT, glvElemId, ltElemId,
              mmdlBindResolveGenLevel2LtLtnessAct);
}

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Light Lightness Actual State and a  Generic On Off state.
 *
 *  \param[in] ltElemId     Element identifier where the Light Lightness Actual state resides.
 *  \param[in] onoffElemId  Element identifier where the On Off state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrBind2OnOff(meshElementId_t ltElemId, meshElementId_t onoffElemId)
{
  /* Add Generic On Off -> Light Lightness Actual binding */
  MmdlAddBind(MMDL_STATE_GEN_ONOFF, MMDL_STATE_LT_LTNESS_ACT, onoffElemId, ltElemId,
              mmdlBindResolveGenOnOff2LtLtnessAct);

  /* Add Light Lightness Actual -> Generic On Off binding */
  MmdlAddBind(MMDL_STATE_LT_LTNESS_ACT, MMDL_STATE_GEN_ONOFF, ltElemId, onoffElemId,
              mmdlBindResolveLtLtnessAct2GenOnOff);
}
