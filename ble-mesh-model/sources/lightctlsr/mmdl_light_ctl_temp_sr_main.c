/*************************************************************************************************/
/*!
 *  \file   mmdl_light_ctl_temperature_sr_main.c
 *
 *  \brief  Implementation of the Light CTL Temperature Server model.
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
#include "mmdl_light_ctl_temp_sr_api.h"
#include "mmdl_light_ctl_sr_api.h"
#include "mmdl_light_ctl_temp_sr_main.h"
#include "mmdl_light_ctl_sr_main.h"
#include "mmdl_light_ctl_sr.h"
#include "mmdl_gen_level_sr.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Light CTL Set Message TID index */
#define MMDL_SET_TID_IDX          4

/*! Light CTL Set Message TID index */
#define MMDL_SET_TRANSITION_IDX   5

/*! Light CTL Set Message TID index */
#define MMDL_SET_DELAY_IDX        6

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Light CTL Temperature Server control block type definition */
typedef struct mmdlLightCtlTemperatureSrCb_tag
{
  mmdlBindResolve_t         fResolveBind;         /*!< Pointer to the function that checks
                                                   *   and resolves a bind triggered by a
                                                   *   change in this model instance
                                                   */
  mmdlEventCback_t          recvCback;            /*!< Model Scene Server received callback */
}mmdlLightCtlTemperatureSrCb_t;

/*! Light CTL Temperature Server message handler type definition */
typedef void (*mmdlLightCtlTemperatureSrHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler ID */
wsfHandlerId_t mmdlLightCtlTemperatureSrHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlLightCtlTemperatureSrRcvdOpcodes[MMDL_LIGHT_CTL_TEMP_SR_NUM_RCVD_OPCODES] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_CTL_TEMP_GET_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_CTL_TEMP_SET_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_CTL_TEMP_SET_NO_ACK_OPCODE) }},
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlLightCtlTemperatureSrHandleMsg_t mmdlLightCtlTemperatureSrHandleMsg[MMDL_LIGHT_CTL_TEMP_SR_NUM_RCVD_OPCODES] =
{
  mmdlLightCtlTemperatureSrHandleGet,
  mmdlLightCtlTemperatureSrHandleSet,
  mmdlLightCtlTemperatureSrHandleSetNoAck
};

/*! Light CTL Temperature Server Control Block */
static mmdlLightCtlTemperatureSrCb_t  temperatureCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Searches for the Light CTL Temperature Server model instance descriptor on the
 *              specified element.
 *
 *  \param[in]  elementId  Identifier of the Element implementing the model.
 *  \param[out] ppOutDesc  Double pointer to the descriptor.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void mmdlLightCtlTempSrGetDesc(meshElementId_t elementId, mmdlLightCtlTempSrDesc_t **ppOutDesc)
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
        MMDL_LIGHT_CTL_TEMP_SR_MDL_ID)
    {
      /* Matching model ID on elementId */
      *ppOutDesc = pMeshConfig->pElementArray[elementId].pSigModelArray[modelIdx].pModelDescriptor;
      break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light CTL message to the destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model.
 *  \param[in] serverAddr     Element address of the server.
 *  \param[in] ttl            TTL value as defined by the specification.
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] pParam         Pointer to structure containing the message parameters.
 *  \param[in] paramLen       Length of message parameters structure.
 *  \param[in] opcode         Light CTL message opcode.
 *  \param[in] recvOnUnicast  TRUE if is a response to a unicast request, FALSE otherwise.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightCtlTempSrSendMessage(meshElementId_t elementId, meshAddress_t serverAddr,
                                          uint8_t ttl, uint16_t appKeyIndex, const uint8_t *pParam,
                                          uint8_t paramLen, uint16_t opcode, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_LIGHT_CTL_TEMP_SR_MDL_ID, opcode);

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
 *  \brief     Publishes a Light CTL message to the publication address.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] pParam     Pointer to structure containing the parameters.
 *  \param[in] paramLen   Length of the parameters structure.
 *  \param[in] opcode     Command opcode.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightCtlTempSrPublishMessage(meshElementId_t elementId, const uint8_t *pParam,
                                             uint8_t paramLen, uint16_t opcode)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_LIGHT_CTL_TEMP_SR_MDL_ID, opcode);

  /* Fill in the msg info parameters */
  pubMsgInfo.elementId = elementId;

  /* Send message to the Mesh Core. Parameters are already stored in over-the-air order */
  MeshPublishMessage(&pubMsgInfo, (uint8_t *)pParam, paramLen);
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the CTL Temperature present state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model.
 *  \param[in] pDesc           Pointer to model descriptor.
 *  \param[in] pState          Pointer to temperature state.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightCtlTempSrSetPresentState(meshElementId_t elementId,
                                              mmdlLightCtlTempSrDesc_t *pDesc,
                                              mmdlLightCtlTempSrState_t * pState,
                                              mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlLightCtlTempSrStateUpdate_t event;

  /* Update State */
  pDesc->pStoredState->present.temperature = pState->temperature;
  pDesc->pStoredState->present.deltaUV = pState->deltaUV;

  if (stateUpdateSrc != MMDL_STATE_UPDATED_BY_BIND)
  {
    /* Update State on bound main element state */
    MmdlLightCtlSrSetBoundTemp(pDesc->mainElementId, pState, pState);
  }

  /* Check for bindings on this state. Trigger bindings */
  if ((stateUpdateSrc != MMDL_STATE_UPDATED_BY_BIND) &&
      (stateUpdateSrc != MMDL_STATE_UPDATED_BY_SCENE) && (temperatureCb.fResolveBind))
  {
    temperatureCb.fResolveBind(elementId, MMDL_STATE_LT_CTL_TEMP, &pDesc->pStoredState->present.temperature);
  }

  /* Publish state change */
  MmdlLightCtlTemperatureSrPublish(elementId);

  /* Set event type */
  event.hdr.event = MMDL_LIGHT_CTL_SR_EVENT;
  event.hdr.param = MMDL_LIGHT_CTL_TEMP_SR_STATE_UPDATE_EVENT;

  /* Set event parameters */
  event.elemId = elementId;
  event.temperature = pDesc->pStoredState->present.temperature;
  event.deltaUV = pDesc->pStoredState->present.deltaUV;

  /* Send event to the upper layer */
  temperatureCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the CTL Temperature state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model.
 *  \param[in] pState          Pointer to temperature state.
 *  \param[in] transitionMs    Transition time in ms for this transaction.
 *  \param[in] delayMs         Delay in for executing transaction. Unit is 5ms.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightCtlTempSrSetState(meshElementId_t elementId, mmdlLightCtlTempSrState_t * pState,
                                       uint32_t transitionMs, uint8_t delay5Ms,
                                       mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlLightCtlTempSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlLightCtlTempSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    MMDL_TRACE_INFO3("LIGHT CTL TEMP SR: Set Target Temp=0x%X TimeRem=%d ms, Delay=0x%X",
                     pState->temperature, transitionMs, delay5Ms);

    /* Update descriptor */
    pDesc->remainingTimeMs = transitionMs;
    pDesc->delay5Ms = delay5Ms;
    pDesc->pStoredState->target.temperature = pState->temperature;
    pDesc->pStoredState->target.deltaUV = pState->deltaUV;

    /* Check if the set is delayed */
    if (pDesc->delay5Ms > 0)
    {
      /* Start Timer */
      WsfTimerStartMs(&pDesc->transitionTimer, DELAY_5MS_TO_MS(pDesc->delay5Ms));

      /* State change is delayed */
    }
    /* Check if state will change after a transition or immediately */
    else if (pDesc->remainingTimeMs > 0)
    {
      /* Start Timer */
      WsfTimerStartMs(&pDesc->transitionTimer, pDesc->remainingTimeMs);

      /* Update State on bound main element state for Power Up */
      MmdlLightCtlSrSetBoundTemp(pDesc->mainElementId, NULL, &pDesc->pStoredState->target);
    }
    else
    {
      /* Stop transition */
      if (pDesc->transitionTimer.isStarted)
      {
        WsfTimerStop(&pDesc->transitionTimer);
      }

      /* Set present state */
      mmdlLightCtlTempSrSetPresentState(elementId, pDesc, pState, stateUpdateSrc);
    }
  }

  (void)stateUpdateSrc;
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light CTL Temperature Status command to the specified destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  Indicates if message that triggered the status was received on unicast.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightCtlTempSrSendStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                         uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  uint8_t msgParams[MMDL_LIGHT_CTL_TEMP_STATUS_MAX_LEN];
  mmdlLightCtlTempSrDesc_t *pDesc = NULL;
  uint8_t *pMsgParams, tranTime;

  /* Get the model instance descriptor */
  mmdlLightCtlTempSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    pMsgParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->present.temperature);
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->present.deltaUV);

    if (pDesc->remainingTimeMs > 0)
    {
      UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->target.temperature);
      UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->target.deltaUV);

      if (pDesc->delay5Ms == 0)
      {
        /* Timer is running the transition */
        tranTime = MmdlGenDefaultTimeMsToTransTime(pDesc->transitionTimer.ticks * WSF_MS_PER_TICK);
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
      MMDL_TRACE_INFO3("LIGHT CTL TEMP SR: Send Temperature Status Present=0x%X Target=0x%X remTime=%d",
                       pDesc->pStoredState->present.temperature,
                       pDesc->pStoredState->target.temperature,
                       pDesc->remainingTimeMs);

      mmdlLightCtlTempSrSendMessage(elementId, dstAddr, MESH_USE_DEFAULT_TTL, appKeyIndex, msgParams,
                                    (uint8_t)(pMsgParams - msgParams),
                                    MMDL_LIGHT_CTL_TEMP_STATUS_OPCODE,
                                    recvOnUnicast);
    }
    else
    {
      MMDL_TRACE_INFO3("LIGHT CTL TEMP SR: Publish Temperature Present=0x%X Target=0x%X remTime=%d",
                       pDesc->pStoredState->present.temperature,
                       pDesc->pStoredState->target.temperature,
                       pDesc->remainingTimeMs);

      mmdlLightCtlTempSrPublishMessage(elementId, msgParams, (uint8_t)(pMsgParams - msgParams),
                                       MMDL_LIGHT_CTL_TEMP_STATUS_OPCODE);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light CTL Temperature Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightCtlTemperatureSrHandleGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    mmdlLightCtlTempSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Processes Light CTL Set commands.
 *
 *  \param[in]  pMsg         Received model message.
 *  \param[in]  ackRequired  TRUE if acknowledgement is required in response,  FALSE otherwise.
 *
 *  \return    TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlLightCtlTempSrProcessSet(const meshModelMsgRecvEvt_t *pMsg, bool_t ackRequired)
{
  mmdlLightCtlTempSrState_t state;
  mmdlLightCtlTempSrDesc_t *pDesc = NULL;
  mmdlLightCtlSrDesc_t *pCtlDesc = NULL;
  uint8_t tid, *pMsgParam;
  uint32_t transMs;
  uint32_t delayMs;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length. It can take only min and max values. */
  if (pMsg->messageParamsLen != MMDL_LIGHT_CTL_TEMP_SET_MAX_LEN &&
      pMsg->messageParamsLen != MMDL_LIGHT_CTL_TEMP_SET_MIN_LEN)
  {
    return FALSE;
  }

  /* Extract parameters */
  pMsgParam = pMsg->pMessageParams;
  BSTREAM_TO_UINT16(state.temperature, pMsgParam);
  BSTREAM_TO_UINT16(state.deltaUV, pMsgParam);

  if ((state.temperature < MMDL_LIGHT_CTL_TEMP_MIN) ||
      (state.temperature > MMDL_LIGHT_CTL_TEMP_MAX))
  {
    return FALSE;
  }

  /* Check if it contains optional parameters */
  if (pMsg->messageParamsLen == MMDL_LIGHT_CTL_TEMP_SET_MAX_LEN)
  {
    /* Check prohibited values for Transition Time */
    if (TRANSITION_TIME_STEPS(pMsg->pMessageParams[MMDL_SET_TRANSITION_IDX]) ==
        MMDL_GEN_TR_UNKNOWN)
    {
      return FALSE;
    }
  }

  /* Get the model instance descriptor */
  mmdlLightCtlTempSrGetDesc(pMsg->elementId, &pDesc);

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

    /* Get the CTL instance descriptor */
    MmdlLightCtlSrGetDesc(pDesc->mainElementId, &pCtlDesc);

    if ((pCtlDesc != NULL) && (pCtlDesc->pStoredState != NULL))
    {
      /* Check if target state is in range */
      if (state.temperature < pCtlDesc->pStoredState->minTemperature)
      {
        state.temperature = pCtlDesc->pStoredState->minTemperature;
      }
      else if (state.temperature > pCtlDesc->pStoredState->maxTemperature)
      {
        state.temperature = pCtlDesc->pStoredState->maxTemperature;
      }
    }

    /* Update last transaction fields and restart 6 seconds timer */
    pDesc->ackPending = ackRequired;
    pDesc->srcAddr = pMsg->srcAddr;
    pDesc->transactionId = tid;
    pDesc->ackAppKeyIndex = pMsg->appKeyIndex;
    pDesc->ackForUnicast = pMsg->recvOnUnicast;

    /* Check if it contains optional parameters */
    if (pMsg->messageParamsLen == MMDL_LIGHT_CTL_TEMP_SET_MAX_LEN)
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
    if ((state.temperature == pDesc->pStoredState->present.temperature) &&
        (state.deltaUV == pDesc->pStoredState->present.deltaUV))
    {
      /* Transition is considered complete*/
      transMs = 0;
    }

    /* Change state. Temperature element is always after the Main element */
    mmdlLightCtlTempSrSetState(pMsg->elementId, &state, transMs, delayMs, MMDL_STATE_UPDATED_BY_CL);

    return (pDesc->delay5Ms == 0);
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light CTL Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightCtlTemperatureSrHandleSet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  if (mmdlLightCtlTempSrProcessSet(pMsg, TRUE))
  {
    /* Send Status message as a response to the Set message */
    mmdlLightCtlTempSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light CTL Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightCtlTemperatureSrHandleSetNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  (void)mmdlLightCtlTempSrProcessSet(pMsg, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Light CTL Temperature Server timeout callback on a specific element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightCtlTempSrHandleTmrCback(meshElementId_t elementId)
{
  mmdlLightCtlTempSrDesc_t *pDesc = NULL;
  mmdlLightCtlTempSrState_t state;

  /* Get model instance descriptor */
  mmdlLightCtlTempSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    state.deltaUV = pDesc->pStoredState->target.deltaUV;
    state.temperature = pDesc->pStoredState->target.temperature;

    if (pDesc->delay5Ms != 0)
    {
      /* Reset Delay */
      pDesc->delay5Ms = 0;

      /* Timeout. Set state. */
      mmdlLightCtlTempSrSetState(elementId, &state, pDesc->remainingTimeMs, 0,
                                 pDesc->updateSource);

      /* Send Status if it was a delayed Acknowledged Set */
      if (pDesc->ackPending)
      {
        mmdlLightCtlTempSrSendStatus(elementId, pDesc->srcAddr, pDesc->ackAppKeyIndex,
                                     pDesc->ackForUnicast);
      }
    }
    else if (pDesc->remainingTimeMs != 0)
    {
      /* Reset Transition Time */
      pDesc->remainingTimeMs = 0;

      /* Timeout. Set state. */
      mmdlLightCtlTempSrSetState(elementId, &state, 0, 0, pDesc->updateSource);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Light CTL Temperature Server Message Received 6 seconds timeout callback
 *             on a specific element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightCtlTempSrHandleMsgRcvdTmrCback(meshElementId_t elementId)
{
  mmdlLightCtlTempSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlLightCtlTempSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    /* Reset source address and transaction ID for last stored transaction */
    pDesc->srcAddr = MESH_ADDR_TYPE_UNASSIGNED;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between a Light CTL Temperature state and a Generic Level state as
 *             a result of an updated Light CTL Temperature state.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveLightCtlTemp2GenLevel(meshElementId_t tgtElementId, void *pStateValue)
{
  mmdlLightCtlTempSrDesc_t *pDesc = NULL;
  mmdlLightCtlSrDesc_t *pCtlDesc = NULL;
  uint16_t temperature;
  int16_t level;

  /* Get model instance descriptor */
  mmdlLightCtlTempSrGetDesc(tgtElementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    /* Get model instance descriptor */
    MmdlLightCtlSrGetDesc(pDesc->mainElementId, &pCtlDesc);

    if ((pCtlDesc != NULL) && (pCtlDesc->pStoredState != NULL))
    {
      temperature = *(uint16_t *)pStateValue;

      /* Generic Level = (Light CTL Temperature - T _MIN) * 65535 / (T_MAX - T_MIN) - 32768 */
      level = (temperature - pCtlDesc->pStoredState->minTemperature) * 65535 /
              (pCtlDesc->pStoredState->maxTemperature - pCtlDesc->pStoredState->minTemperature) -
              32768;

      /* Update Generic Level state on target element */
      MmdlGenLevelSrSetBoundState(tgtElementId, level);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between Generic Level a state and a Light CTL Temperature state as
 *             a result of an updated Generic Level state.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveGenLevel2LightCtlTemp(meshElementId_t tgtElementId, void *pStateValue)
{
  mmdlLightCtlSrDesc_t *pCtlDesc = NULL;
  mmdlLightCtlTempSrDesc_t *pDesc = NULL;
  mmdlLightCtlTempSrState_t state;
  int16_t level;

  /* Get the model instance descriptor */
  mmdlLightCtlTempSrGetDesc(tgtElementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    /* Get the main model instance descriptor */
    MmdlLightCtlSrGetDesc(pDesc->mainElementId, &pCtlDesc);

    if ((pCtlDesc != NULL) && (pCtlDesc->pStoredState != NULL))
    {
      level = *(uint16_t *)pStateValue;
      state.deltaUV = pDesc->pStoredState->present.deltaUV;
      state.temperature = pCtlDesc->pStoredState->minTemperature + (level + 32768) *
                          (pCtlDesc->pStoredState->maxTemperature -
                           pCtlDesc->pStoredState->minTemperature) / 65535;

      /* Change state locally. No transition time or delay is allowed. */
      mmdlLightCtlTempSrSetState(tgtElementId, &state, 0, 0, MMDL_STATE_UPDATED_BY_BIND);
    }
  }
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Light CTL Temperature Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlLightCtlTemperatureSrInit(void)
{
  meshElementId_t elemId;
  mmdlLightCtlTempSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO0("LIGHT CTL TEMP SR: init");

  /* Set event callbacks */
  temperatureCb.recvCback = MmdlEmptyCback;
  temperatureCb.fResolveBind = MmdlBindResolve;

  /* Initialize timers */
  for (elemId = 0; elemId < pMeshConfig->elementArrayLen; elemId++)
  {
    /* Get the model instance descriptor */
    mmdlLightCtlTempSrGetDesc(elemId, &pDesc);

    if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
    {
      pDesc->srcAddr = MESH_ADDR_TYPE_UNASSIGNED;

      /* Set transition timer parameters*/
      pDesc->transitionTimer.handlerId = mmdlLightCtlTemperatureSrHandlerId;
      pDesc->transitionTimer.msg.event = MMDL_LIGHT_CTL_TEMP_SR_EVT_TMR_CBACK;
      pDesc->transitionTimer.msg.param = elemId;

      /* Set msg Received timer parameters*/
      pDesc->msgRcvdTimer.handlerId = mmdlLightCtlTemperatureSrHandlerId;
      pDesc->msgRcvdTimer.msg.event = MMDL_LIGHT_CTL_TEMP_SR_MSG_RCVD_TMR_CBACK;
      pDesc->msgRcvdTimer.msg.param = elemId;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Light CTL Temperature Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Light CTL Temperature Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlTemperatureSrHandlerInit(wsfHandlerId_t handlerId)
{
  mmdlLightCtlTemperatureSrHandlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Light CTL Temperature Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlTemperatureSrHandler(wsfMsgHdr_t *pMsg)
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
        for (opcodeIdx = 0; opcodeIdx < MMDL_LIGHT_CTL_TEMP_SR_NUM_RCVD_OPCODES; opcodeIdx++)
        {
          opcodeSize = MESH_OPCODE_SIZE(pModelMsg->msgRecvEvt.opCode);
          if (!memcmp(&mmdlLightCtlTemperatureSrRcvdOpcodes[opcodeIdx],
                      pModelMsg->msgRecvEvt.opCode.opcodeBytes,
                      opcodeSize))
          {
            /* Process message */
            (void)mmdlLightCtlTemperatureSrHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
          }
        }
        break;

      case MESH_MODEL_EVT_PERIODIC_PUB:
        pModelMsg = (meshModelEvt_t *)pMsg;

        /* Check if periodic publishing was not disabled. */
        if(pModelMsg->periodicPubEvt.nextPubTimeMs != 0)
        {
          /* Publishing is requested part of the periodic publishing. */
          MmdlLightCtlTemperatureSrPublish(pModelMsg->periodicPubEvt.elementId);
        }
        break;

      case MMDL_LIGHT_CTL_TEMP_SR_EVT_TMR_CBACK:
        mmdlLightCtlTempSrHandleTmrCback((meshElementId_t)pMsg->param);
        break;

      case MMDL_LIGHT_CTL_TEMP_SR_MSG_RCVD_TMR_CBACK:
        mmdlLightCtlTempSrHandleMsgRcvdTmrCback((meshElementId_t)pMsg->param);
        break;

      default:
        MMDL_TRACE_WARN0("LIGHT CTL TEMP SR: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publish a Light CTL Temperature Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlTemperatureSrPublish(meshElementId_t elementId)
{
  /* Publish Status */
  mmdlLightCtlTempSrSendStatus(elementId, MMDL_USE_PUBLICATION_ADDR, 0, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local state. The set is instantaneous.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] temperature        New temperature value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlTemperatureSrSetTemperature(meshElementId_t elementId, mmdlLightCtlTempSrState_t * pState)
{
  /* Change state locally. No transition time or delay is allowed. */
  mmdlLightCtlTempSrSetState(elementId, pState, 0, 0, MMDL_STATE_UPDATED_BY_APP);
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local state as a result of a binding with a Light CTL state.
 *             The set is instantaneous.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model.
 *  \param[in] pState         Pointer to the present Temperature state.
 *  \param[in] pTargetState   Pointer to the target Temperature state.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlTemperatureSrSetBoundState(meshElementId_t elementId,
                                            mmdlLightCtlTempSrState_t * pState,
                                            mmdlLightCtlTempSrState_t * pTargetState)
{
  mmdlLightCtlTempSrDesc_t *pDesc = NULL;

  /* Get the model instance descriptor */
  mmdlLightCtlTempSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    if (pTargetState != NULL)
    {
      memcpy(&pDesc->pStoredState->target, pTargetState, sizeof(mmdlLightCtlTempSrState_t));
    }

    if (pState != NULL)
    {
      mmdlLightCtlTempSrSetPresentState(elementId, pDesc, pState, MMDL_STATE_UPDATED_BY_BIND);
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
void MmdlLightCtlTemperatureSrRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback*/
  if (recvCback != NULL)
  {
    temperatureCb.recvCback = recvCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Light CTL Temperature State and a Generic Level state.
 *
 *  \param[in] temperatureElemId  Element identifier where the Light CTL Temperature state resides.
 *  \param[in] glvElemId  Element identifier where the Generic Level state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlTemperatureSrBind2GenLevel(meshElementId_t temperatureElemId,
                                            meshElementId_t glvElemId)
{
  /* Add Light CTL Temperature -> Generic Level binding */
  MmdlAddBind(MMDL_STATE_LT_CTL_TEMP, MMDL_STATE_GEN_LEVEL, temperatureElemId, glvElemId,
              mmdlBindResolveLightCtlTemp2GenLevel);

  /* Add Generic Level -> Light CTL Temperature binding */
  MmdlAddBind(MMDL_STATE_GEN_LEVEL, MMDL_STATE_LT_CTL_TEMP, glvElemId, temperatureElemId,
              mmdlBindResolveGenLevel2LightCtlTemp);
}
