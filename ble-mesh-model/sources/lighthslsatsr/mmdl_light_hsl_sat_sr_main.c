/*************************************************************************************************/
/*!
 *  \file   mmdl_light_hsl_sat_sr_main.c
 *
 *  \brief  Implementation of the Light HSL Saturation Server model.
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
#include "mmdl_light_hsl_sat_sr_api.h"
#include "mmdl_light_hsl_sr_api.h"
#include "mmdl_light_hsl_sat_sr_main.h"
#include "mmdl_light_hsl_sr.h"
#include "mmdl_light_hsl_sat_sr.h"
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

/*! Light HSL Saturation Server control block type definition */
typedef struct mmdlLightHslSatSrCb_tag
{
  mmdlBindResolve_t         fResolveBind;         /*!< Pointer to the function that checks
                                                   *   and resolves a bind triggered by a
                                                   *   change in this model instance
                                                   */
  mmdlEventCback_t          recvCback;            /*!< Model Scene Server received callback */
}mmdlLightHslSatSrCb_t;

/*! Light HSL Saturation Server message handler type definition */
typedef void (*mmdlLightHslSatSrHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler ID */
wsfHandlerId_t mmdlLightHslSatSrHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlLightHslSatSrRcvdOpcodes[MMDL_LIGHT_HSL_SAT_SR_NUM_RCVD_OPCODES] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_HSL_SAT_GET_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_HSL_SAT_SET_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_HSL_SAT_SET_NO_ACK_OPCODE) }},
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlLightHslSatSrHandleMsg_t mmdlLightHslSatSrHandleMsg[MMDL_LIGHT_HSL_SAT_SR_NUM_RCVD_OPCODES] =
{
  mmdlLightHslSatSrHandleGet,
  mmdlLightHslSatSrHandleSet,
  mmdlLightHslSatSrHandleSetNoAck
};

/*! Light HSL Saturation Server Control Block */
static mmdlLightHslSatSrCb_t  satCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Searches for the Light HSL Saturation Server model instance descriptor on the specified element.
 *
 *  \param[in]  elementId  Identifier of the Element implementing the model.
 *  \param[out] ppOutDesc  Double pointer to the descriptor.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void mmdlLightHslSatSrGetDesc(meshElementId_t elementId, mmdlLightHslSatSrDesc_t **ppOutDesc)
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
        MMDL_LIGHT_HSL_SAT_SR_MDL_ID)
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
static void mmdlLightHslSatSrSendMessage(meshElementId_t elementId, meshAddress_t serverAddr,
                                      uint8_t ttl, uint16_t appKeyIndex, const uint8_t *pParam,
                                      uint8_t paramLen, uint16_t opcode, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_LIGHT_HSL_SAT_SR_MDL_ID, opcode);

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
static void mmdlLightHslSatSrPublishMessage(meshElementId_t elementId, const uint8_t *pParam,
                                         uint8_t paramLen, uint16_t opcode)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_LIGHT_HSL_SAT_SR_MDL_ID, opcode);

  /* Fill in the msg info parameters */
  pubMsgInfo.elementId = elementId;

  /* Send message to the Mesh Core. Parameters are already stored in over-the-air order */
  MeshPublishMessage(&pubMsgInfo, (uint8_t *)pParam, paramLen);
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the HSL present state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model.
 *  \param[in] pDesc           Pointer to the model descriptor
 *  \param[in] sat             Sat value.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightHslSatSrSetPresentState(meshElementId_t elementId,
                                             mmdlLightHslSatSrDesc_t *pDesc, uint16_t sat,
                                             mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlLightHslSatSrStateUpdate_t event;
  int16_t level;

  /* Update State */
  pDesc->pStoredState->presentSat = sat;

  if (stateUpdateSrc != MMDL_STATE_UPDATED_BY_BIND)
  {
    /* Update State on bound main element state */
    MmdlLightHslSrSetBoundSaturation(pDesc->mainElementId, pDesc->pStoredState->presentSat,
                                     pDesc->pStoredState->targetSat);
  }

  /* Update Generic Level state on target element */
  level = sat - 0x8000;
  MmdlGenLevelSrSetBoundState(elementId, level);

  /* Check for bindings on this state. Trigger bindings */
  if ((stateUpdateSrc != MMDL_STATE_UPDATED_BY_BIND) &&
      (stateUpdateSrc != MMDL_STATE_UPDATED_BY_SCENE) && (satCb.fResolveBind))
  {
    satCb.fResolveBind(elementId, MMDL_STATE_LT_HSL_SATURATION, &pDesc->pStoredState->presentSat);
  }

  /* Publish state change */
  MmdlLightHslSatSrPublish(elementId);
  /* Set event type */
  event.hdr.event = MMDL_LIGHT_HSL_SR_EVENT;
  event.hdr.param = MMDL_LIGHT_HSL_SAT_SR_STATE_UPDATE_EVENT;

  /* Set event parameters */
  event.elemId = elementId;
  event.state = pDesc->pStoredState->presentSat;

  /* Send event to the upper layer */
  satCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the HSL state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model.
 *  \param[in] sat             Sat value.
 *  \param[in] transitionMs    Transition time in ms for this transaction.
 *  \param[in] delayMs         Delay in for executing transaction. Unit is 5ms.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightHslSatSrSetState(meshElementId_t elementId, uint16_t sat,
                                       uint32_t transitionMs, uint8_t delay5Ms,
                                       mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlLightHslSatSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlLightHslSatSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    MMDL_TRACE_INFO3("LIGHT HSL SAT SR: Set TargetSat=0x%X TimeRem=%d ms, Delay=0x%X",
                      sat, transitionMs, delay5Ms);

    /* Update descriptor */
    pDesc->remainingTimeMs = transitionMs;
    pDesc->delay5Ms = delay5Ms;
    pDesc->pStoredState->targetSat = sat;
    pDesc->updateSource = stateUpdateSrc;

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
      MmdlLightHslSrSetBoundSaturation(pDesc->mainElementId, pDesc->pStoredState->presentSat,
                                       pDesc->pStoredState->targetSat);
    }
    else
    {
      /* Stop transition */
      if (pDesc->transitionTimer.isStarted)
      {
        WsfTimerStop(&pDesc->transitionTimer);
      }

      mmdlLightHslSatSrSetPresentState(elementId, pDesc, sat, stateUpdateSrc);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light HSL Sat Status command to the specified destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  Indicates if message that triggered the status was received on unicast.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightHslSatSrSendStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                       uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  uint8_t msgParams[MMDL_LIGHT_HSL_SAT_STATUS_MAX_LEN];
  mmdlLightHslSatSrDesc_t *pDesc = NULL;
  uint8_t *pMsgParams, tranTime;

  /* Get the model instance descriptor */
  mmdlLightHslSatSrGetDesc(elementId, &pDesc);


  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    pMsgParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->presentSat);

    if (pDesc->remainingTimeMs > 0)
    {
      UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->targetSat);

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
      MMDL_TRACE_INFO3("LIGHT HSL SAT SR: Send Sat Status Present=0x%X Target=0x%X remTime=%d",
                       pDesc->pStoredState->presentSat, pDesc->pStoredState->targetSat,
                       pDesc->remainingTimeMs);

      mmdlLightHslSatSrSendMessage(elementId, dstAddr, MESH_USE_DEFAULT_TTL, appKeyIndex, msgParams,
                                (uint8_t)(pMsgParams - msgParams), MMDL_LIGHT_HSL_SAT_STATUS_OPCODE,
                                recvOnUnicast);
    }
    else
    {
      MMDL_TRACE_INFO3("LIGHT HSL SAT SR: Publish Sat Present=0x%X Target=0x%X remTime=%d",
                       pDesc->pStoredState->presentSat, pDesc->pStoredState->targetSat,
                       pDesc->remainingTimeMs);

      mmdlLightHslSatSrPublishMessage(elementId, msgParams, (uint8_t)(pMsgParams - msgParams),
                                   MMDL_LIGHT_HSL_SAT_STATUS_OPCODE);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light HSL Sat Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightHslSatSrHandleGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    mmdlLightHslSatSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Processes Light HSL Set commands.
 *
 *  \param[in] pMsg         Received model message.
 *  \param[in] ackRequired  TRUE if acknowledgement is required in response,  FALSE otherwise.
 *
 *  \return    TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlLightHslSatSrProcessSet(const meshModelMsgRecvEvt_t *pMsg, bool_t ackRequired)
{
  uint16_t sat;
  mmdlLightHslSatSrDesc_t *pDesc = NULL;
  mmdlLightHslSrDesc_t *pHslDesc = NULL;
  uint8_t tid;
  uint32_t transMs;
  uint32_t delayMs;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length. It can take only min and max values. */
  if (pMsg->messageParamsLen != MMDL_LIGHT_HSL_SAT_SET_MAX_LEN &&
      pMsg->messageParamsLen != MMDL_LIGHT_HSL_SAT_SET_MIN_LEN)
  {
    return FALSE;
  }

  /* Extract parameters */
  BYTES_TO_UINT16(sat, pMsg->pMessageParams);

  /* Check if it contains optional parameters */
  if (pMsg->messageParamsLen == MMDL_LIGHT_HSL_SAT_SET_MAX_LEN)
  {
    /* Check prohibited values for Transition Time */
    if (TRANSITION_TIME_STEPS(pMsg->pMessageParams[MMDL_SET_TRANSITION_IDX]) ==
        MMDL_GEN_TR_UNKNOWN)
    {
      return FALSE;
    }
  }

  /* Get the model instance descriptor */
  mmdlLightHslSatSrGetDesc(pMsg->elementId, &pDesc);

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
      if (sat < pHslDesc->pStoredState->minSat)
      {
        sat = pHslDesc->pStoredState->minSat;
      }
      else if (sat > pHslDesc->pStoredState->maxSat)
      {
        sat = pHslDesc->pStoredState->maxSat;
      }
    }

    /* Update last transaction fields and restart 6 seconds timer */
    pDesc->ackPending = ackRequired;
    pDesc->srcAddr = pMsg->srcAddr;
    pDesc->transactionId = tid;
    pDesc->ackAppKeyIndex = pMsg->appKeyIndex;
    pDesc->ackForUnicast = pMsg->recvOnUnicast;

    /* Check if it contains optional parameters */
    if (pMsg->messageParamsLen == MMDL_LIGHT_HSL_SAT_SET_MAX_LEN)
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
    if (sat == pDesc->pStoredState->presentSat)
    {
      /* Transition is considered complete*/
      transMs = 0;
    }

    /* Determine the number of transition steps */
    pDesc->steps = transMs / MMDL_TRANSITION_STATE_UPDATE_INTERVAL;

    if (pDesc->steps > 0)
    {
      /* Compute the transition step increment */
      pDesc->transitionStep = (sat - pDesc->pStoredState->presentSat) / pDesc->steps;
    }

    /* Change state. Sat element is always after the Main element */
    mmdlLightHslSatSrSetState(pMsg->elementId, sat, transMs, delayMs, MMDL_STATE_UPDATED_BY_CL);

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
void mmdlLightHslSatSrHandleSet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  if (mmdlLightHslSatSrProcessSet(pMsg, TRUE))
  {
    /* Send Status message as a response to the Set message */
    mmdlLightHslSatSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
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
void mmdlLightHslSatSrHandleSetNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  (void)mmdlLightHslSatSrProcessSet(pMsg, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Light HSL Saturation Server timeout callback on a specific element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightHslSatSrHandleTmrCback(meshElementId_t elementId)
{
  mmdlLightHslSatSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlLightHslSatSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    if (pDesc->delay5Ms != 0)
    {
      /* Reset Delay */
      pDesc->delay5Ms = 0;

      /* Timeout. Set state. */
      mmdlLightHslSatSrSetState(elementId, pDesc->pStoredState->targetSat, pDesc->remainingTimeMs, 0,
                                pDesc->updateSource);


      /* Send Status if it was a delayed Acknowledged Set */
      if (pDesc->ackPending)
      {
        mmdlLightHslSatSrSendStatus(elementId, pDesc->srcAddr, pDesc->ackAppKeyIndex,
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
        state = pDesc->pStoredState->presentSat + pDesc->transitionStep;

        /* Update present state only. */
        mmdlLightHslSatSrSetPresentState(elementId, pDesc, state, pDesc->updateSource);

        if (pDesc->steps == 1)
        {
          /* Next is the last step.
           * Program the remaining time (can be more than MMDL_TRANSITION_STATE_UPDATE_INTERVAL).
           * Also, the last step increment can be greater then the intermediate ones.
           */
          pDesc->steps = 0;
        }

        /* Program next transition */
        mmdlLightHslSatSrSetState(elementId, pDesc->pStoredState->targetSat, remainingTimeMs, 0,
                                  pDesc->updateSource);
      }
      else
      {
        /* Timeout. Set state. */
        mmdlLightHslSatSrSetState(elementId, pDesc->pStoredState->targetSat, 0, 0,
                                  pDesc->updateSource);
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Light HSL Saturation Server Message Received 6 seconds timeout callback on
 *             a specific element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightHslSatSrHandleMsgRcvdTmrCback(meshElementId_t elementId)
{
  mmdlLightHslSatSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlLightHslSatSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    /* Reset source address and transaction ID for last stored transaction */
    pDesc->srcAddr = MESH_ADDR_TYPE_UNASSIGNED;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between a Light HSL Saturation state and a Generic Level state as
 *             a result of an updated Light HSL Saturation state.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveLightHslSat2GenLevel(meshElementId_t tgtElementId, void *pStateValue)
{
  uint16_t hue;
  int16_t level;

  hue = *(uint16_t*)pStateValue;
  level = hue - 0x8000;

  /* Update Generic Level state on target element */
  MmdlGenLevelSrSetBoundState(tgtElementId, level);
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between Generic Level a state and a Light HSL Saturation state as
 *             a result of an updated Generic Level state.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveGenLevel2LightHslSat(meshElementId_t tgtElementId, void *pStateValue)
{
  uint16_t level;

  level = *(uint16_t *)pStateValue;

  /* Change state locally. No transition time or delay is allowed. */
  mmdlLightHslSatSrSetState(tgtElementId, level + 0x8000, 0, 0, MMDL_STATE_UPDATED_BY_BIND);
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Light HSL Saturation Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlLightHslSatSrInit(void)
{
  meshElementId_t elemId;
  mmdlLightHslSatSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO0("LIGHT HSL SAT SR: init");

  /* Set event callbacks */
  satCb.recvCback = MmdlEmptyCback;
  satCb.fResolveBind = MmdlBindResolve;

  /* Initialize timers */
  for (elemId = 0; elemId < pMeshConfig->elementArrayLen; elemId++)
  {
    /* Get the model instance descriptor */
    mmdlLightHslSatSrGetDesc(elemId, &pDesc);

    if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
    {
      pDesc->srcAddr = MESH_ADDR_TYPE_UNASSIGNED;

      /* Set transition timer parameters*/
      pDesc->transitionTimer.handlerId = mmdlLightHslSatSrHandlerId;
      pDesc->transitionTimer.msg.event = MMDL_LIGHT_HSL_SAT_SR_EVT_TMR_CBACK;
      pDesc->transitionTimer.msg.param = elemId;

      /* Set msg Received timer parameters*/
      pDesc->msgRcvdTimer.handlerId = mmdlLightHslSatSrHandlerId;
      pDesc->msgRcvdTimer.msg.event = MMDL_LIGHT_HSL_SAT_SR_MSG_RCVD_TMR_CBACK;
      pDesc->msgRcvdTimer.msg.param = elemId;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Light HSL Saturation Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Light HSL Saturation Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSatSrHandlerInit(wsfHandlerId_t handlerId)
{
  mmdlLightHslSatSrHandlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Light HSL Saturation Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSatSrHandler(wsfMsgHdr_t *pMsg)
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
        for (opcodeIdx = 0; opcodeIdx < MMDL_LIGHT_HSL_SAT_SR_NUM_RCVD_OPCODES; opcodeIdx++)
        {
          opcodeSize = MESH_OPCODE_SIZE(pModelMsg->msgRecvEvt.opCode);
          if (!memcmp(&mmdlLightHslSatSrRcvdOpcodes[opcodeIdx],
                      pModelMsg->msgRecvEvt.opCode.opcodeBytes,
                      opcodeSize))
          {
            /* Process message */
            (void)mmdlLightHslSatSrHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
          }
        }
        break;

      case MESH_MODEL_EVT_PERIODIC_PUB:
        pModelMsg = (meshModelEvt_t *)pMsg;

        /* Check if periodic publishing was not disabled. */
        if(pModelMsg->periodicPubEvt.nextPubTimeMs != 0)
        {
          /* Publishing is requested part of the periodic publishing. */
          MmdlLightHslSatSrPublish(pModelMsg->periodicPubEvt.elementId);
        }
        break;

      case MMDL_LIGHT_HSL_SAT_SR_EVT_TMR_CBACK:
        mmdlLightHslSatSrHandleTmrCback((meshElementId_t)pMsg->param);
        break;

      case MMDL_LIGHT_HSL_SAT_SR_MSG_RCVD_TMR_CBACK:
        mmdlLightHslSatSrHandleMsgRcvdTmrCback((meshElementId_t)pMsg->param);
        break;

      default:
        MMDL_TRACE_WARN0("LIGHT HSL SAT SR: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publish a Light HSL Sat Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSatSrPublish(meshElementId_t elementId)
{
  /* Publish Status */
  mmdlLightHslSatSrSendStatus(elementId, MMDL_USE_PUBLICATION_ADDR, 0, FALSE);
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
void MmdlLightHslSatSrRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback*/
  if (recvCback != NULL)
  {
    satCb.recvCback = recvCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local saturation state. The set is instantaneous.
 *
 *  \param[in] elementId   Identifier of the Element implementing the model.
 *  \param[in] saturation  New saturation value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSatSrSetSaturation(meshElementId_t elementId, uint16_t saturation)
{
  /* Change state locally. No transition time or delay is allowed. */
  mmdlLightHslSatSrSetState(elementId, saturation, 0, 0, MMDL_STATE_UPDATED_BY_APP);
}

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Light HSL Saturation State and a Generic Level state.
 *
 *  \param[in] satElemId  Element identifier where the Light HSL Saturation state resides.
 *  \param[in] glvElemId  Element identifier where the Generic Level state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSatSrBind2GenLevel(meshElementId_t satElemId, meshElementId_t glvElemId)
{
  /* Add Light HSL Saturation -> Generic Level binding */
  MmdlAddBind(MMDL_STATE_LT_HSL_SATURATION, MMDL_STATE_GEN_LEVEL, satElemId, glvElemId,
              mmdlBindResolveLightHslSat2GenLevel);

  /* Add Generic Level -> Light HSL Saturation binding */
  MmdlAddBind(MMDL_STATE_GEN_LEVEL, MMDL_STATE_LT_HSL_SATURATION, glvElemId, satElemId,
              mmdlBindResolveGenLevel2LightHslSat);
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local saturation state. The set is instantaneous.
 *
 *  \param[in] elementId   Identifier of the Element implementing the model.
 *  \param[in] presentSat  New present saturation value.
 *  \param[in] targetSat   New target saturation value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSatSrSetBoundState(meshElementId_t elementId, uint16_t presentSat,
                                    uint16_t targetSat)
{
  mmdlLightHslSatSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlLightHslSatSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    /* Update descriptor */
    pDesc->pStoredState->targetSat = targetSat;
    mmdlLightHslSatSrSetPresentState(elementId, pDesc, presentSat, MMDL_STATE_UPDATED_BY_BIND);
  }
}
