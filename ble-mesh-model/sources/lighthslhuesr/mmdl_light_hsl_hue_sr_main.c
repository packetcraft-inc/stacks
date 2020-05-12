/*************************************************************************************************/
/*!
 *  \file   mmdl_light_hsl_hue_sr_main.c
 *
 *  \brief  Implementation of the Light HSL Hue Server model.
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
#include "mmdl_light_hsl_hue_sr_api.h"
#include "mmdl_light_hsl_sr_api.h"
#include "mmdl_light_hsl_hue_sr_main.h"
#include "mmdl_light_hsl_sr.h"
#include "mmdl_light_hsl_hue_sr.h"
#include "mmdl_gen_level_sr.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Light HSL Set Message TID index */
#define MMDL_SET_TID_IDX          2

/*! Light HSL Set Message TID index */
#define MMDL_SET_TRANSITION_IDX   3

/*! Light HSL Set Message TID index */
#define MMDL_SET_DELAY_IDX        4

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Light HSL Hue Server control block type definition */
typedef struct mmdlLightHslHueSrCb_tag
{
  mmdlBindResolve_t         fResolveBind;         /*!< Pointer to the function that checks
                                                   *   and resolves a bind triggered by a
                                                   *   change in this model instance
                                                   */
  mmdlEventCback_t          recvCback;            /*!< Model Scene Server received callback */
}mmdlLightHslHueSrCb_t;

/*! Light HSL Hue Server message handler type definition */
typedef void (*mmdlLightHslHueSrHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler ID */
wsfHandlerId_t mmdlLightHslHueSrHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlLightHslHueSrRcvdOpcodes[MMDL_LIGHT_HSL_HUE_SR_NUM_RCVD_OPCODES] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_HSL_HUE_GET_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_HSL_HUE_SET_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_HSL_HUE_SET_NO_ACK_OPCODE) }},
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlLightHslHueSrHandleMsg_t mmdlLightHslHueSrHandleMsg[MMDL_LIGHT_HSL_HUE_SR_NUM_RCVD_OPCODES] =
{
  mmdlLightHslHueSrHandleGet,
  mmdlLightHslHueSrHandleSet,
  mmdlLightHslHueSrHandleSetNoAck
};

/*! Light HSL Hue Server Control Block */
static mmdlLightHslHueSrCb_t  hueCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Searches for the Light HSL Hue Server model instance descriptor on the specified element.
 *
 *  \param[in]  elementId  Identifier of the Element implementing the model.
 *  \param[out] ppOutDesc  Double pointer to the descriptor.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void mmdlLightHslHueSrGetDesc(meshElementId_t elementId, mmdlLightHslHueSrDesc_t **ppOutDesc)
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
        MMDL_LIGHT_HSL_HUE_SR_MDL_ID)
    {
      /* Matching model ID on elementId */
      *ppOutDesc = pMeshConfig->pElementArray[elementId].pSigModelArray[modelIdx].pModelDescriptor;
      break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light HSL Client message to the destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model.
 *  \param[in] serverAddr     Element address of the server.
 *  \param[in] ttl            TTL value as defined by the specification.
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] pParam         Pointer to structure containing the message parameters.
 *  \param[in] paramLen       Length of message parameters structure.
 *  \param[in] opcode         Light HSL Client message opcode.
 *  \param[in] recvOnUnicast  TRUE if is a response to a unicast request, FALSE otherwise.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightHslHueSrSendMessage(meshElementId_t elementId, meshAddress_t serverAddr,
                                      uint8_t ttl, uint16_t appKeyIndex, const uint8_t *pParam,
                                      uint8_t paramLen, uint16_t opcode, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_LIGHT_HSL_HUE_SR_MDL_ID, opcode);

  /* Fill in the message information */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = serverAddr;
  msgInfo.ttl = ttl;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Send message to the Mesh Core. Parameters are already stored in over-the-air order */
  MeshSendMessage(&msgInfo, (uint8_t *)pParam, paramLen, MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                  MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));
}

/*************************************************************************************************/
/*!
 *  \brief     Publishes a Light HSL message to the publication address.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] pParam     Pointer to structure containing the parameters.
 *  \param[in] paramLen   Length of the parameters structure.
 *  \param[in] opcode     Command opcode.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightHslHueSrPublishMessage(meshElementId_t elementId, const uint8_t *pParam,
                                         uint8_t paramLen, uint16_t opcode)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_LIGHT_HSL_HUE_SR_MDL_ID, opcode);

  /* Fill in the msg info parameters */
  pubMsgInfo.elementId = elementId;

  /* Send message to the Mesh Core. Parameters are already stored in over-the-air order */
  MeshPublishMessage(&pubMsgInfo, (uint8_t *)pParam, paramLen);
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the HSL Present state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model.
 *  \param[in] pDesc           Pointer to the model descriptor
 *  \param[in] hue             Hue value.
 *  \param[in] transitionMs    Transition time in ms for this transaction.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightHslHueSrSetPresentState(meshElementId_t elementId,
                                             mmdlLightHslHueSrDesc_t *pDesc,
                                             uint16_t hue, mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlLightHslHueSrStateUpdate_t event;
  int16_t level;

  /* Update State */
  pDesc->pStoredState->presentHue = hue;

  /* Update State on bound main element state */
  if (stateUpdateSrc != MMDL_STATE_UPDATED_BY_BIND)
  {
    MmdlLightHslSrSetBoundHue(pDesc->mainElementId, pDesc->pStoredState->presentHue,
                              pDesc->pStoredState->targetHue);
  }

  /* Update Generic Level state on target element */
  level = hue - 0x8000;
  MmdlGenLevelSrSetBoundState(elementId, level);

  /* Check for bindings on this state. Trigger bindings */
  if ((stateUpdateSrc != MMDL_STATE_UPDATED_BY_BIND) &&
      (stateUpdateSrc != MMDL_STATE_UPDATED_BY_SCENE) && (hueCb.fResolveBind))
  {
    hueCb.fResolveBind(elementId, MMDL_STATE_LT_HSL_HUE, &pDesc->pStoredState->presentHue);
  }

  /* Publish state change */
  MmdlLightHslHueSrPublish(elementId);

  /* Set event type */
  event.hdr.event = MMDL_LIGHT_HSL_SR_EVENT;
  event.hdr.param = MMDL_LIGHT_HSL_HUE_SR_STATE_UPDATE_EVENT;

  /* Set event parameters */
  event.elemId = elementId;
  event.state = pDesc->pStoredState->presentHue;

  /* Send event to the upper layer */
  hueCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the HSL state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model.
 *  \param[in] hue             Hue value.
 *  \param[in] transitionMs    Transition time in ms for this transaction.
 *  \param[in] delayMs         Delay in for executing transaction. Unit is 5ms.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightHslHueSrSetState(meshElementId_t elementId, uint16_t hue,
                                       uint32_t transitionMs, uint8_t delay5Ms,
                                       mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlLightHslHueSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlLightHslHueSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    MMDL_TRACE_INFO3("LIGHT HSL HUE SR: Set TargetHue=0x%X TimeRem=%d ms, Delay=0x%X",
                     hue, transitionMs, delay5Ms);

    /* Update descriptor */
    pDesc->remainingTimeMs = transitionMs;
    pDesc->delay5Ms = delay5Ms;
    pDesc->pStoredState->targetHue = hue;

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

      /* Update State on bound main element state */
      MmdlLightHslSrSetBoundHue(pDesc->mainElementId, pDesc->pStoredState->presentHue,
                                pDesc->pStoredState->targetHue);
    }
    else
    {
      /* Stop transition */
      if (pDesc->transitionTimer.isStarted)
      {
        WsfTimerStop(&pDesc->transitionTimer);
      }

      mmdlLightHslHueSrSetPresentState(elementId, pDesc, hue, stateUpdateSrc);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light HSL Hue Status command to the specified destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  Indicates if message that triggered the status was received on unicast.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightHslHueSrSendStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                       uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  uint8_t msgParams[MMDL_LIGHT_HSL_HUE_STATUS_MAX_LEN];
  mmdlLightHslHueSrDesc_t *pDesc = NULL;
  uint8_t *pMsgParams, tranTime;

  /* Get the model instance descriptor */
  mmdlLightHslHueSrGetDesc(elementId, &pDesc);


  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    pMsgParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->presentHue);

    if (pDesc->remainingTimeMs > 0)
    {
      UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->targetHue);

      if (pDesc->delay5Ms == 0)
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
      }
      else
      {
        /* Timer is running the delay. Transition did not start. */
        tranTime = MmdlGenDefaultTimeMsToTransTime(pDesc->remainingTimeMs);
      }

      UINT8_TO_BSTREAM(pMsgParams, tranTime);
    }

    if (dstAddr != MMDL_USE_PUBLICATION_ADDR)
    {
      MMDL_TRACE_INFO3("LIGHT HSL HUE SR: Send Hue Status Present=0x%X Target=0x%X remTime=%d",
                       pDesc->pStoredState->presentHue, pDesc->pStoredState->targetHue,
                       pDesc->remainingTimeMs);

      mmdlLightHslHueSrSendMessage(elementId, dstAddr, MESH_USE_DEFAULT_TTL, appKeyIndex, msgParams,
                                (uint8_t)(pMsgParams - msgParams), MMDL_LIGHT_HSL_HUE_STATUS_OPCODE,
                                recvOnUnicast);
    }
    else
    {
      MMDL_TRACE_INFO3("LIGHT HSL HUE SR: Publish Hue Present=0x%X Target=0x%X remTime=%d",
                       pDesc->pStoredState->presentHue, pDesc->pStoredState->targetHue,
                       pDesc->remainingTimeMs);

      mmdlLightHslHueSrPublishMessage(elementId, msgParams, (uint8_t)(pMsgParams - msgParams),
                                   MMDL_LIGHT_HSL_HUE_STATUS_OPCODE);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light HSL Hue Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightHslHueSrHandleGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    mmdlLightHslHueSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Processes Light HSL Set commands.
 *
 *  \param[in]  pMsg         Received model message.
 *  \param[in]  ackRequired  TRUE if acknowledgement is required in response,  FALSE otherwise.
 *
 *  \return    TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlLightHslHueSrProcessSet(const meshModelMsgRecvEvt_t *pMsg, bool_t ackRequired)
{
  uint16_t hue;
  mmdlLightHslHueSrDesc_t *pDesc = NULL;
  mmdlLightHslSrDesc_t *pHslDesc = NULL;
  uint8_t tid;
  uint32_t transMs;
  uint32_t delayMs;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length. It can take only min and max values. */
  if (pMsg->messageParamsLen != MMDL_LIGHT_HSL_HUE_SET_MAX_LEN &&
      pMsg->messageParamsLen != MMDL_LIGHT_HSL_HUE_SET_MIN_LEN)
  {
    return FALSE;
  }

  /* Extract parameters */
  BYTES_TO_UINT16(hue, pMsg->pMessageParams);

  /* Check if it contains optional parameters */
  if (pMsg->messageParamsLen == MMDL_LIGHT_HSL_HUE_SET_MAX_LEN)
  {
    /* Check prohibited values for Transition Time */
    if (TRANSITION_TIME_STEPS(pMsg->pMessageParams[MMDL_SET_TRANSITION_IDX]) ==
        MMDL_GEN_TR_UNKNOWN)
    {
      return FALSE;
    }
  }

  /* Get the model instance descriptor */
  mmdlLightHslHueSrGetDesc(pMsg->elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    WsfTimerStartMs(&pDesc->msgRcvdTimer, MSG_RCVD_TIMEOUT_MS);

    /* Get Transaction ID */
    tid = pMsg->pMessageParams[MMDL_SET_TID_IDX];

    /* Validate message against last transaction */
    if ((pMsg->srcAddr == pDesc->srcAddr) && (tid == pDesc->transactionId))
    {
      return FALSE;
    }

    /* Get the HSL instance descriptor */
    MmdlLightHslSrGetDesc(pDesc->mainElementId, &pHslDesc);

    if ((pHslDesc != NULL) && (pHslDesc->pStoredState != NULL))
    {
      /* Check if target state is in range */
      if (hue < pHslDesc->pStoredState->minHue)
      {
        hue = pHslDesc->pStoredState->minHue;
      }
      else if (hue > pHslDesc->pStoredState->maxHue)
      {
        hue = pHslDesc->pStoredState->maxHue;
      }
    }

    /* Update last transaction fields and restart 6 seconds timer */
    pDesc->ackPending = ackRequired;
    pDesc->srcAddr = pMsg->srcAddr;
    pDesc->transactionId = tid;
    pDesc->ackAppKeyIndex = pMsg->appKeyIndex;
    pDesc->ackForUnicast = pMsg->recvOnUnicast;

    /* Check if it contains optional parameters */
    if (pMsg->messageParamsLen == MMDL_LIGHT_HSL_HUE_SET_MAX_LEN)
    {
      /* Get Transition time */
      transMs = MmdlGenDefaultTransTimeToMs(pMsg->pMessageParams[MMDL_SET_TRANSITION_IDX]);
      delayMs = pMsg->pMessageParams[MMDL_SET_DELAY_IDX];
    }
    else
    {
      /* Get Default Transition time from the Main element */
      transMs = MmdlGenDefaultTransGetTime(pDesc->mainElementId);
      delayMs = 0;
    }

    /* Check if target state is different from current state */
    if (hue == pDesc->pStoredState->presentHue)
    {
      /* Transition is considered complete*/
      transMs = 0;
    }

    /* Determine the number of transition steps */
    pDesc->steps = transMs / MMDL_TRANSITION_STATE_UPDATE_INTERVAL;

    if (pDesc->steps > 0)
    {
      /* Compute the transition step increment */
      pDesc->transitionStep = (hue - pDesc->pStoredState->presentHue) / pDesc->steps;
    }

    /* Change state. Hue element is always after the Main element */
    mmdlLightHslHueSrSetState(pMsg->elementId, hue, transMs, delayMs, MMDL_STATE_UPDATED_BY_CL);

    return (pDesc->delay5Ms == 0);
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light HSL Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightHslHueSrHandleSet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  if (mmdlLightHslHueSrProcessSet(pMsg, TRUE))
  {
    /* Send Status message as a response to the Set message */
    mmdlLightHslHueSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light HSL Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightHslHueSrHandleSetNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  (void)mmdlLightHslHueSrProcessSet(pMsg, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Light HSL Hue Server timeout callback on a specific element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightHslHueSrHandleTmrCback(meshElementId_t elementId)
{
  mmdlLightHslHueSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlLightHslHueSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    if (pDesc->delay5Ms != 0)
    {
      /* Reset Delay */
      pDesc->delay5Ms = 0;

      /* Timeout. Set state. */
      mmdlLightHslHueSrSetState(elementId, pDesc->pStoredState->targetHue, pDesc->remainingTimeMs, 0,
                                pDesc->updateSource);


      /* Send Status if it was a delayed Acknowledged Set */
      if (pDesc->ackPending)
      {
        mmdlLightHslHueSrSendStatus(elementId, pDesc->srcAddr, pDesc->ackAppKeyIndex,
                                 pDesc->ackForUnicast);
      }
    }
    else if (pDesc->remainingTimeMs != 0)
    {
      if (pDesc->steps > 0)
      {
        uint16_t state;
        uint32_t remainingTimeMs;

        /* Transition is divided into steps. Decrement the remaining time and steps */
        pDesc->steps--;
        remainingTimeMs = pDesc->remainingTimeMs - MMDL_TRANSITION_STATE_UPDATE_INTERVAL;

        /* Compute intermediate state value */
        state = pDesc->pStoredState->presentHue + pDesc->transitionStep;

        /* Update present state only. */
        mmdlLightHslHueSrSetPresentState(elementId, pDesc, state, pDesc->updateSource);

        if (pDesc->steps == 1)
        {
          /* Next is the last step.
           * Program the remaining time (can be more than MMDL_TRANSITION_STATE_UPDATE_INTERVAL).
           * Also, the last step increment can be greater then the intermediate ones.
           */
          pDesc->steps = 0;
        }

        /* Program next transition */
        mmdlLightHslHueSrSetState(elementId, pDesc->pStoredState->targetHue, remainingTimeMs, 0,
                                  pDesc->updateSource);
      }
      else
      {
        /* Timeout. Set state. */
        mmdlLightHslHueSrSetState(elementId, pDesc->pStoredState->targetHue, 0, 0,
                                  pDesc->updateSource);
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Light HSL Hue Server Message Received 6 seconds timeout callback on
 *             a specific element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightHslHueSrHandleMsgRcvdTmrCback(meshElementId_t elementId)
{
  mmdlLightHslHueSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlLightHslHueSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    /* Reset source address and transaction ID for last stored transaction */
    pDesc->srcAddr = MESH_ADDR_TYPE_UNASSIGNED;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between a Light HSL Hue state and a Generic Level state as
 *             a result of an updated Light HSL Hue state.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveLightHslHue2GenLevel(meshElementId_t tgtElementId, void *pStateValue)
{
  uint16_t hue;
  int16_t level;

  hue = *(uint16_t *)pStateValue;
  level = hue - 0x8000;

  /* Update Generic Level state on target element */
  MmdlGenLevelSrSetBoundState(tgtElementId, level);
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between Generic Level a state and a Light HSL Hue state as
 *             a result of an updated Generic Level state.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveGenLevel2LightHslHue(meshElementId_t tgtElementId, void *pStateValue)
{
  int16_t level;

  level = *(uint16_t *)pStateValue;

  /* Change state locally. No transition time or delay is allowed. */
  mmdlLightHslHueSrSetState(tgtElementId, level + 0x8000, 0, 0, MMDL_STATE_UPDATED_BY_BIND);
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Light HSL Hue Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlLightHslHueSrInit(void)
{
  meshElementId_t elemId;
  mmdlLightHslHueSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO0("LIGHT HSL HUE SR: init");

  /* Set event callbacks */
  hueCb.recvCback = MmdlEmptyCback;
  hueCb.fResolveBind = MmdlBindResolve;

  /* Initialize timers */
  for (elemId = 0; elemId < pMeshConfig->elementArrayLen; elemId++)
  {
    /* Get the model instance descriptor */
    mmdlLightHslHueSrGetDesc(elemId, &pDesc);

    if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
    {
      pDesc->srcAddr = MESH_ADDR_TYPE_UNASSIGNED;

      /* Set transition timer parameters*/
      pDesc->transitionTimer.handlerId = mmdlLightHslHueSrHandlerId;
      pDesc->transitionTimer.msg.event = MMDL_LIGHT_HSL_HUE_SR_EVT_TMR_CBACK;
      pDesc->transitionTimer.msg.param = elemId;

      /* Set msg Received timer parameters*/
      pDesc->msgRcvdTimer.handlerId = mmdlLightHslHueSrHandlerId;
      pDesc->msgRcvdTimer.msg.event = MMDL_LIGHT_HSL_HUE_SR_MSG_RCVD_TMR_CBACK;
      pDesc->msgRcvdTimer.msg.param = elemId;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Light HSL Hue Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Light HSL Hue Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslHueSrHandlerInit(wsfHandlerId_t handlerId)
{
  mmdlLightHslHueSrHandlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Light HSL Hue Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslHueSrHandler(wsfMsgHdr_t *pMsg)
{
  meshModelEvt_t *pModelMsg;
  uint8_t opcodeIdx, opcodeSize;

  /* Handle message */
  if (pMsg != NULL)
  {
    switch (pMsg->event)
    {
      case MESH_MODEL_EVT_MSG_RECV:
        pModelMsg = (meshModelEvt_t *)pMsg;

        /* Match the received opcode */
        for (opcodeIdx = 0; opcodeIdx < MMDL_LIGHT_HSL_HUE_SR_NUM_RCVD_OPCODES; opcodeIdx++)
        {
          opcodeSize = MESH_OPCODE_SIZE(pModelMsg->msgRecvEvt.opCode);
          if (!memcmp(&mmdlLightHslHueSrRcvdOpcodes[opcodeIdx],
                      pModelMsg->msgRecvEvt.opCode.opcodeBytes,
                      opcodeSize))
          {
            /* Process message */
            (void)mmdlLightHslHueSrHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
          }
        }
        break;

      case MESH_MODEL_EVT_PERIODIC_PUB:
        pModelMsg = (meshModelEvt_t *)pMsg;

        /* Check if periodic publishing was not disabled. */
        if(pModelMsg->periodicPubEvt.nextPubTimeMs != 0)
        {
          /* Publishing is requested part of the periodic publishing. */
          MmdlLightHslHueSrPublish(pModelMsg->periodicPubEvt.elementId);
        }
        break;

      case MMDL_LIGHT_HSL_HUE_SR_EVT_TMR_CBACK:
        mmdlLightHslHueSrHandleTmrCback((meshElementId_t)pMsg->param);
        break;

      case MMDL_LIGHT_HSL_HUE_SR_MSG_RCVD_TMR_CBACK:
        mmdlLightHslHueSrHandleMsgRcvdTmrCback((meshElementId_t)pMsg->param);
        break;

      default:
        MMDL_TRACE_WARN0("LIGHT HSL HUE SR: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publish a Light HSL Hue Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslHueSrPublish(meshElementId_t elementId)
{
  /* Publish Status */
  mmdlLightHslHueSrSendStatus(elementId, MMDL_USE_PUBLICATION_ADDR, 0, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local state. The set is instantaneous.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] hue        New hue value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslHueSrSetHue(meshElementId_t elementId, uint16_t hue)
{
  /* Change state locally. No transition time or delay is allowed. */
  mmdlLightHslHueSrSetState(elementId, hue, 0, 0, MMDL_STATE_UPDATED_BY_APP);
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
void MmdlLightHslHueSrRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback*/
  if (recvCback != NULL)
  {
    hueCb.recvCback = recvCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Light HSL Hue State and a Generic Level state.
 *
 *  \param[in] hueElemId  Element identifier where the Light HSL Hue state resides.
 *  \param[in] glvElemId  Element identifier where the Generic Level state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslHueSrBind2GenLevel(meshElementId_t hueElemId, meshElementId_t glvElemId)
{
  /* Add Light HSL Hue -> Generic Level binding */
  MmdlAddBind(MMDL_STATE_LT_HSL_HUE, MMDL_STATE_GEN_LEVEL, hueElemId, glvElemId,
              mmdlBindResolveLightHslHue2GenLevel);

  /* Add Generic Level -> Light HSL Hue binding */
  MmdlAddBind(MMDL_STATE_GEN_LEVEL, MMDL_STATE_LT_HSL_HUE, glvElemId, hueElemId,
              mmdlBindResolveGenLevel2LightHslHue);
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local hue state. The set is instantaneous.
 *
 *  \param[in] elementId   Identifier of the Element implementing the model.
 *  \param[in] presentHue  New present hue value.
 *  \param[in] targetHue   New target hue value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslHueSrSetBoundState(meshElementId_t elementId, uint16_t presentHue,
                                    uint16_t targetHue)
{
  mmdlLightHslHueSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlLightHslHueSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    /* Update descriptor */
    pDesc->pStoredState->targetHue = targetHue;
    mmdlLightHslHueSrSetPresentState(elementId, pDesc, presentHue, MMDL_STATE_UPDATED_BY_BIND);
  }
}
