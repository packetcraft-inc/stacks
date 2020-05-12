/*************************************************************************************************/
/*!
 *  \file   mmdl_light_ctl_sr_main.c
 *
 *  \brief  Implementation of the Light CTL Server model.
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
#include "mmdl_gen_default_trans_sr.h"
#include "mmdl_light_ctl_sr_api.h"
#include "mmdl_light_ctl_temp_sr_api.h"
#include "mmdl_lightlightness_sr_api.h"
#include "mmdl_light_ctl_sr.h"
#include "mmdl_light_ctl_temp_sr.h"
#include "mmdl_light_ctl_sr_main.h"
#include "mmdl_light_ctl_setup_sr.h"
#include "mmdl_light_ctl_temp_sr_main.h"
#include "mmdl_lightlightness_sr.h"
#include "mmdl_gen_level_sr.h"
#include "mmdl_gen_onoff_sr.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Light CTL Set Message TID index */
#define MMDL_SET_TID_IDX          6

/*! Light CTL Set Message Transition Time index */
#define MMDL_SET_TRANSITION_IDX   7

/*! Light CTL Set Message Delay index */
#define MMDL_SET_DELAY_IDX        8

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Light CTL Server control block type definition */
typedef struct mmdlLightCtlSrCb_tag
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
  mmdlEventCback_t          recvCback;            /*!< Model Scene Server received callback */
}mmdlLightCtlSrCb_t;

/*! Light CTL Server message handler type definition */
typedef void (*mmdlLightCtlSrHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler ID */
wsfHandlerId_t mmdlLightCtlSrHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlLightCtlSrRcvdOpcodes[MMDL_LIGHT_CTL_SR_NUM_RCVD_OPCODES] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_CTL_GET_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_CTL_SET_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_CTL_SET_NO_ACK_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_CTL_DEFAULT_GET_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_CTL_TEMP_RANGE_GET_OPCODE) }}
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlLightCtlSrHandleMsg_t mmdlLightCtlSrHandleMsg[MMDL_LIGHT_CTL_SR_NUM_RCVD_OPCODES] =
{
  mmdlLightCtlSrHandleGet,
  mmdlLightCtlSrHandleSet,
  mmdlLightCtlSrHandleSetNoAck,
  mmdlLightCtlSrHandleDefaultGet,
  mmdlLightCtlSrHandleRangeGet,
};

/*! Light CTL Server Control Block */
static mmdlLightCtlSrCb_t  ctlCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Searches for the Light CTL Server model instance descriptor on the specified element.
 *
 *  \param[in]  elementId  Identifier of the Element implementing the model.
 *  \param[out] ppOutDesc  Double pointer to the descriptor.
 *  \param[out] modelId    Lighting Model ID.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void mmdlLightCtlSrGetDesc(meshElementId_t elementId, void **ppOutDesc, uint16_t modelId)
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
    if (pMeshConfig->pElementArray[elementId].pSigModelArray[modelIdx].modelId == modelId)
    {
      /* Matching model ID on elementId */
      *ppOutDesc = pMeshConfig->pElementArray[elementId].pSigModelArray[modelIdx].pModelDescriptor;
      break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Searches for the Light CTL Server model instance descriptor on the specified element.
 *
 *  \param[in]  elementId  Identifier of the Element implementing the model.
 *  \param[out] ppOutDesc  Double pointer to the descriptor.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrGetDesc(meshElementId_t elementId, mmdlLightCtlSrDesc_t **ppOutDesc)
{
  mmdlLightCtlSrGetDesc(elementId, (void *)ppOutDesc, MMDL_LIGHT_CTL_SR_MDL_ID);
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light CTL Server message to the destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model.
 *  \param[in] serverAddr     Element address of the server.
 *  \param[in] ttl            TTL value as defined by the specification.
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] pParam         Pointer to structure containing the message parameters.
 *  \param[in] paramLen       Length of message parameters structure.
 *  \param[in] opcode         Light CTL Server message opcode.
 *  \param[in] recvOnUnicast  TRUE if is a response to a unicast request, FALSE otherwise.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightCtlSrSendMessage(meshElementId_t elementId, meshAddress_t serverAddr,
                                      uint8_t ttl, uint16_t appKeyIndex, const uint8_t *pParam,
                                      uint8_t paramLen, uint16_t opcode, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_LIGHT_CTL_SR_MDL_ID, opcode);

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
static void mmdlLightCtlSrPublishMessage(meshElementId_t elementId, const uint8_t *pParam,
                                         uint8_t paramLen, uint16_t opcode)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_LIGHT_CTL_SR_MDL_ID, opcode);

  /* Fill in the msg info parameters */
  pubMsgInfo.elementId = elementId;

  /* Send message to the Mesh Core. Parameters are already stored in over-the-air order */
  MeshPublishMessage(&pubMsgInfo, (uint8_t *)pParam, paramLen);
}

/*************************************************************************************************/
/*!
 *  \brief      Processes Light Ctl Range Set commands.
 *
 *  \param[in]  pMsg          Received model message.
 *  \param[out] pOutOpStatus  Operation status. See ::mmdlRangeStatus.
 *
 *  \return     TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t mmdlLightCtlSrProcessRangeSet(const meshModelMsgRecvEvt_t *pMsg, uint8_t *pOutOpStatus)
{
  mmdlLightCtlSrDesc_t *pDesc;
  uint8_t *pMsgParam;
  uint16_t minTemp = 0;
  uint16_t maxTemp = 0;
  mmdlLightCtlSrStateUpdate_t event;

  /* Set default value */
  *pOutOpStatus = MMDL_RANGE_PROHIBITED;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Get the model instance descriptor */
  MmdlLightCtlSrGetDesc(pMsg->elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    /* Set the state value from pMessageParams buffer. */
    pMsgParam = pMsg->pMessageParams;
    BSTREAM_TO_UINT16(minTemp, pMsgParam);
    BSTREAM_TO_UINT16(maxTemp, pMsgParam);

    if (minTemp < MMDL_LIGHT_CTL_TEMP_MIN)
    {
      *pOutOpStatus = MMDL_RANGE_CANNOT_SET_MIN;
    }
    else if (maxTemp > MMDL_LIGHT_CTL_TEMP_MAX)
    {
      *pOutOpStatus = MMDL_RANGE_CANNOT_SET_MAX;
    }
    else if (minTemp <= maxTemp)
    {
      /* Change state */
      pDesc->pStoredState->minTemperature = minTemp;
      pDesc->pStoredState->maxTemperature = maxTemp;

      *pOutOpStatus = MMDL_RANGE_SUCCESS;
    }
  }
  
  /* Set event type */
  event.hdr.event = MMDL_LIGHT_CTL_SR_EVENT;
  event.hdr.param = MMDL_LIGHT_CTL_SR_RANGE_STATE_UPDATE_EVENT;
  event.hdr.status = *pOutOpStatus;

  /* Set event parameters */
  event.elemId = pMsg->elementId;
  event.ctlStates.rangeState.rangeMin = minTemp;
  event.ctlStates.rangeState.rangeMax = maxTemp;


  /* Send event to the upper layer */
  ctlCb.recvCback((wsfMsgHdr_t *)&event);

  return (*pOutOpStatus != MMDL_RANGE_PROHIBITED);
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light CTL Range Status command to the specified destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  TRUE if is a response to a unicast request, FALSE otherwise.
 *  \param[in] opStatus       Operation status.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightCtlSrSendRangeStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                   uint16_t appKeyIndex, bool_t recvOnUnicast, uint8_t opStatus)

{
  uint8_t msgParams[MMDL_LIGHT_CTL_TEMP_RANGE_STATUS_LEN];
  uint8_t *pMsgParams;
  mmdlLightCtlSrDesc_t *pDesc = NULL;

  /* Get the model instance descriptor */
  MmdlLightCtlSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    pMsgParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT8_TO_BSTREAM(pMsgParams, opStatus);
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->minTemperature);
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->maxTemperature);

    MMDL_TRACE_INFO3("LIGHT CTL SR: Send Range Status=%d MinTemperature=0x%X, MaxTemperature=0x%X",
                     opStatus, pDesc->pStoredState->minTemperature,
                     pDesc->pStoredState->maxTemperature);

    if (dstAddr != MMDL_USE_PUBLICATION_ADDR)
    {
      mmdlLightCtlSrSendMessage(elementId, dstAddr, MESH_USE_DEFAULT_TTL, appKeyIndex, msgParams,
                                MMDL_LIGHT_CTL_TEMP_RANGE_STATUS_LEN, MMDL_LIGHT_CTL_RANGE_STATUS_OPCODE,
                                recvOnUnicast);
    }
    else
    {
      mmdlLightCtlSrPublishMessage(elementId, msgParams, MMDL_LIGHT_CTL_TEMP_RANGE_STATUS_LEN,
                                   MMDL_LIGHT_CTL_RANGE_STATUS_OPCODE);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light CTL Default Status command to the specified destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  TRUE if is a response to a unicast request, FALSE otherwise.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightCtlSrSendDefaultStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                     uint16_t appKeyIndex, bool_t recvOnUnicast)

{
  uint8_t msgParams[MMDL_LIGHT_CTL_DEFAULT_STATUS_LEN];
  uint8_t *pMsgParams;
  mmdlLightCtlSrDesc_t *pDesc = NULL;
  mmdlLightLightnessState_t defaultLtness;

  /* Get the model instance descriptor */
  MmdlLightCtlSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    pMsgParams = msgParams;

    /* Copy the message parameters from the descriptor */
    defaultLtness = mmdlLightLightnessDefaultSrGetState(elementId);
    UINT16_TO_BSTREAM(pMsgParams, defaultLtness);
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->defaultTemperature);
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->defaultDeltaUV);

    MMDL_TRACE_INFO3("LIGHT CTL SR: Send Default Ltness=%d Temp=0x%X, deltaUV=0x%X",
                     defaultLtness, pDesc->pStoredState->defaultTemperature,
                     pDesc->pStoredState->defaultDeltaUV);

    if (dstAddr != MMDL_USE_PUBLICATION_ADDR)
    {
      mmdlLightCtlSrSendMessage(elementId, dstAddr, MESH_USE_DEFAULT_TTL, appKeyIndex, msgParams,
                                MMDL_LIGHT_CTL_DEFAULT_STATUS_LEN,
                                MMDL_LIGHT_CTL_DEFAULT_STATUS_OPCODE, recvOnUnicast);
    }
    else
    {
      mmdlLightCtlSrPublishMessage(elementId, msgParams, MMDL_LIGHT_CTL_DEFAULT_STATUS_LEN,
                                   MMDL_LIGHT_CTL_DEFAULT_STATUS_OPCODE);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the CTL state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model.
 *  \param[in] pDesc           Pointer to model descriptor
 *  \param[in] pState          Pointer to Light CTL state.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightCtlSrSetPresentState(meshElementId_t elementId,
                                          mmdlLightCtlSrDesc_t *pDesc,
                                          mmdlLightCtlState_t *pState,
                                          mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlLightCtlSrStateUpdate_t event;
  mmdlLightCtlTempSrState_t state;

  /* Update State */
  pDesc->pStoredState->present.ltness = pState->ltness;
  pDesc->pStoredState->present.temperature = pState->temperature;
  pDesc->pStoredState->present.deltaUV = pState->deltaUV;

  /* Update State on bound Lightness, Gen Level and Gen OnOff elements */
  if ((stateUpdateSrc != MMDL_STATE_UPDATED_BY_BIND) &&
      (stateUpdateSrc != MMDL_STATE_UPDATED_BY_SCENE))
  {
    MmdlLightLightnessSrSetBoundState(elementId, pDesc->pStoredState->present.ltness);
    MmdlGenOnOffSrSetBoundState(elementId, pDesc->pStoredState->present.ltness > 0);
    MmdlGenLevelSrSetBoundState(elementId, pDesc->pStoredState->present.ltness - 0x8000);
  }

  /* Update State on bound Temperature elements */
  state.temperature = pDesc->pStoredState->present.temperature;
  state.deltaUV = pDesc->pStoredState->present.deltaUV;
  MmdlLightCtlTemperatureSrSetBoundState(pDesc->tempElementId, &state, &state);

  /* Check for bindings on this state. Trigger bindings */
  if ((stateUpdateSrc != MMDL_STATE_UPDATED_BY_BIND) &&
      (stateUpdateSrc != MMDL_STATE_UPDATED_BY_SCENE) && (ctlCb.fResolveBind))
  {
    ctlCb.fResolveBind(elementId, MMDL_STATE_LT_CTL, &pDesc->pStoredState->present);
  }

  /* Publish state change */
  MmdlLightCtlSrPublish(elementId);

  /* Set event type */
  event.hdr.event = MMDL_LIGHT_CTL_SR_EVENT;
  event.hdr.param = MMDL_LIGHT_CTL_SR_STATE_UPDATE_EVENT;

  /* Set event parameters */
  event.elemId = elementId;
  event.ctlStates.state.ltness = pDesc->pStoredState->present.ltness;
  event.ctlStates.state.temperature = pDesc->pStoredState->present.temperature;
  event.ctlStates.state.deltaUV = pDesc->pStoredState->present.deltaUV;

  /* Send event to the upper layer */
  ctlCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the CTL state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model.
 *  \param[in] pState          Pointer to Light CTL state.
 *  \param[in] transitionMs    Transition time in ms for this transaction.
 *  \param[in] delayMs         Delay in for executing transaction. Unit is 5ms.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightCtlSrSetState(meshElementId_t elementId, mmdlLightCtlState_t *pState,
                                   uint32_t transitionMs, uint8_t delay5Ms,
                                   mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlLightCtlSrDesc_t *pDesc = NULL;
  mmdlLightCtlTempSrState_t state;
  bool_t saveToNVM = FALSE;

  /* Get model instance descriptor */
  MmdlLightCtlSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    MMDL_TRACE_INFO3("LIGHT CTL SR: Set Target Ltness=0x%X Temp=%d DeltaUV=0x%X",
                     pState->ltness, pState->temperature, pState->deltaUV);
    MMDL_TRACE_INFO2("LIGHT CTL SR: TimeRem=%d ms Delay=0x%X", transitionMs, delay5Ms);

    /* Update descriptor */
    pDesc->remainingTimeMs = transitionMs;
    pDesc->delay5Ms = delay5Ms;
    pDesc->updateSource = stateUpdateSrc;

    /* Update Target State if it has changed*/
    if (memcmp(&pDesc->pStoredState->target, pState, sizeof(mmdlLightCtlState_t)))
    {
      memcpy(&pDesc->pStoredState->target, pState, sizeof(mmdlLightCtlState_t));
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

      /* Update State on bound Temperature elements */
      state.temperature = pDesc->pStoredState->target.temperature;
      state.deltaUV = pDesc->pStoredState->target.deltaUV;
      MmdlLightCtlTemperatureSrSetBoundState(pDesc->tempElementId, NULL, &state);
    }
    else
    {
      /* Stop transition */
      if (pDesc->transitionTimer.isStarted)
      {
        WsfTimerStop(&pDesc->transitionTimer);
      }

      saveToNVM = TRUE;

      mmdlLightCtlSrSetPresentState(elementId, pDesc, pState, stateUpdateSrc);
    }

    /* Save target state in NVM for Power Up. */
    if((pDesc->fNvmSaveStates != NULL) && (saveToNVM == TRUE))
    {
      pDesc->fNvmSaveStates(elementId);
    }
  }

  (void)stateUpdateSrc;
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light CTL Status command to the specified destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  Indicates if message that triggered the status was received on unicast.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightCtlSrSendStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                     uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  uint8_t msgParams[MMDL_LIGHT_CTL_STATUS_MAX_LEN];
  mmdlLightCtlSrDesc_t *pDesc = NULL;
  uint8_t *pMsgParams, tranTime;

  /* Get the model instance descriptor */
  MmdlLightCtlSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    pMsgParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->present.ltness);
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->present.temperature);

    if (pDesc->remainingTimeMs > 0)
    {
      UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->target.ltness);
      UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->target.temperature);

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
      MMDL_TRACE_INFO2("LIGHT CTL SR: Send Status Ltness=0x%X Temp=0x%X",
                       pDesc->pStoredState->present.ltness, pDesc->pStoredState->present.temperature);
      MMDL_TRACE_INFO1(" remTime=%d", pDesc->remainingTimeMs);

      mmdlLightCtlSrSendMessage(elementId, dstAddr, MESH_USE_DEFAULT_TTL, appKeyIndex, msgParams,
                                (uint8_t)(pMsgParams - msgParams), MMDL_LIGHT_CTL_STATUS_OPCODE,
                                recvOnUnicast);
    }
    else
    {
      MMDL_TRACE_INFO2("LIGHT CTL SR: Publish Status Ltness=0x%X Temp=0x%X",
                       pDesc->pStoredState->present.ltness, pDesc->pStoredState->present.temperature);
      MMDL_TRACE_INFO1(" remTime=%d", pDesc->remainingTimeMs);

      mmdlLightCtlSrPublishMessage(elementId, msgParams, (uint8_t)(pMsgParams - msgParams),
                                   MMDL_LIGHT_CTL_STATUS_OPCODE);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light CTL Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightCtlSrHandleGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    mmdlLightCtlSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light CTL Default Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightCtlSrHandleDefaultGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Register Status message as a response to the Register Get message */
    mmdlLightCtlSrSendDefaultStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                                    pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light CTL Range Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightCtlSrHandleRangeGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Range Status message as a response to the Range Get message */
    mmdlLightCtlSrSendRangeStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                                  pMsg->recvOnUnicast, MMDL_RANGE_SUCCESS);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Processes Light CTL Set commands.
 *
 *  \param[in] pMsg         Received model message.
 *  \param[in] ackRequired  TRUE if acknowledgement is required in response,  FALSE otherwise.
 *
 *  \return    TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlLightCtlSrProcessSet(const meshModelMsgRecvEvt_t *pMsg, bool_t ackRequired)
{
  mmdlLightCtlState_t state;
  mmdlLightCtlSrDesc_t *pDesc = NULL;
  uint8_t tid, *pMsgParam;
  uint32_t transMs;
  uint32_t delay;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length. It can take only min and max values. */
  if (pMsg->messageParamsLen != MMDL_LIGHT_CTL_SET_MAX_LEN &&
      pMsg->messageParamsLen != MMDL_LIGHT_CTL_SET_MIN_LEN)
  {
    return FALSE;
  }

  /* Extract parameters */
  pMsgParam = pMsg->pMessageParams;
  BSTREAM_TO_UINT16(state.ltness, pMsgParam);
  BSTREAM_TO_UINT16(state.temperature, pMsgParam);
  BSTREAM_TO_UINT16(state.deltaUV, pMsgParam);

  if ((state.temperature < MMDL_LIGHT_CTL_TEMP_MIN) ||
      (state.temperature > MMDL_LIGHT_CTL_TEMP_MAX))
  {
    return FALSE;
  }

  /* Check if it contains optional parameters */
  if (pMsg->messageParamsLen == MMDL_LIGHT_CTL_SET_MAX_LEN)
  {
    /* Check prohibited values for Transition Time */
    if (TRANSITION_TIME_STEPS(pMsg->pMessageParams[MMDL_SET_TRANSITION_IDX]) ==
        MMDL_GEN_TR_UNKNOWN)
    {
      return FALSE;
    }
  }

  /* Get the model instance descriptor */
  MmdlLightCtlSrGetDesc(pMsg->elementId, &pDesc);

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

    /* Update last transaction fields and restart 6 seconds timer */
    pDesc->ackPending = ackRequired;
    pDesc->srcAddr = pMsg->srcAddr;
    pDesc->transactionId = tid;
    pDesc->ackAppKeyIndex = pMsg->appKeyIndex;
    pDesc->ackForUnicast = pMsg->recvOnUnicast;

    /* Check if it contains optional parameters */
    if (pMsg->messageParamsLen == MMDL_LIGHT_CTL_SET_MAX_LEN)
    {
      /* Get Transition time */
      transMs = MmdlGenDefaultTransTimeToMs(pMsg->pMessageParams[MMDL_SET_TRANSITION_IDX]);
      delay = pMsg->pMessageParams[MMDL_SET_DELAY_IDX];
    }
    else
    {
      /* Get Default Transition time */
      transMs = MmdlGenDefaultTransGetTime(pMsg->elementId);
      delay = 0;
    }

    /* Check if target state is different from current state */
    if ((state.temperature == pDesc->pStoredState->present.temperature) &&
        (state.deltaUV == pDesc->pStoredState->present.deltaUV) &&
        (state.ltness == pDesc->pStoredState->present.ltness))
    {
      /* Transition is considered complete */
      transMs = 0;
    }

    /* Determine the number of transition steps */
    pDesc->steps = transMs / MMDL_TRANSITION_STATE_UPDATE_INTERVAL;

    /* Determine the state transition step */
    if (pDesc->steps > 0)
    {
      mmdlLightCtlState_t *pPresent = &pDesc->pStoredState->present;

      /* Compute the transition step increment */
      pDesc->transitionStep.ltness = ((int32_t)state.ltness - pPresent->ltness) / pDesc->steps;
      pDesc->transitionStep.temperature = ((int32_t)state.temperature - pPresent->temperature) / pDesc->steps;
      pDesc->transitionStep.deltaUV = ((int32_t)state.deltaUV - pPresent->deltaUV) / pDesc->steps;
    }

    /* Change state */
    mmdlLightCtlSrSetState(pMsg->elementId, &state, transMs, delay, MMDL_STATE_UPDATED_BY_CL);

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
void mmdlLightCtlSrHandleSet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  if (mmdlLightCtlSrProcessSet(pMsg, TRUE))
  {
    /* Send Status message as a response to the Set message */
    mmdlLightCtlSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
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
void mmdlLightCtlSrHandleSetNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  (void)mmdlLightCtlSrProcessSet(pMsg, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Light CTL Server timeout callback on a specific element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightCtlSrHandleTmrCback(meshElementId_t elementId)
{
  mmdlLightCtlSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  MmdlLightCtlSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    if (pDesc->delay5Ms != 0)
    {
      /* Reset Delay */
      pDesc->delay5Ms = 0;

      /* Timeout. Set state. */
      mmdlLightCtlSrSetState(elementId, &pDesc->pStoredState->target, pDesc->remainingTimeMs, 0,
                             pDesc->updateSource);

      /* Send Status if it was a delayed Acknowledged Set */
      if (pDesc->ackPending)
      {
        mmdlLightCtlSrSendStatus(elementId, pDesc->srcAddr, pDesc->ackAppKeyIndex,
                                 pDesc->ackForUnicast);
      }
    }
    else if (pDesc->remainingTimeMs != 0)
    {
      if (pDesc->steps > 0)
      {
        uint32_t remainingTimeMs;
        mmdlLightCtlState_t nextState;

        /* Transition is divided into steps. Decrement the remaining time and steps */
        pDesc->steps--;
        remainingTimeMs = pDesc->remainingTimeMs - MMDL_TRANSITION_STATE_UPDATE_INTERVAL;

        /* Compute intermediate state value */
        nextState.ltness = pDesc->pStoredState->present.ltness + pDesc->transitionStep.ltness;
        nextState.temperature = pDesc->pStoredState->present.temperature +
                                pDesc->transitionStep.temperature;
        nextState.deltaUV = pDesc->pStoredState->present.deltaUV + pDesc->transitionStep.deltaUV;

        /* Update present state only. */
        mmdlLightCtlSrSetPresentState(elementId, pDesc, &nextState, pDesc->updateSource);

        if (pDesc->steps == 1)
        {
          /* Next is the last step.
           * Program the remaining time (can be more than MMDL_TRANSITION_STATE_UPDATE_INTERVAL).
           * Also, the last step increment can be greater then the intermediate ones.
           */
          pDesc->steps = 0;
        }

        /* Program next transition */
        mmdlLightCtlSrSetState(elementId, &pDesc->pStoredState->target, remainingTimeMs, 0,
                               pDesc->updateSource);

      }
      else
      {
        /* Timeout. Set state. */
        mmdlLightCtlSrSetState(elementId, &pDesc->pStoredState->target, 0, 0,
                               pDesc->updateSource);
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Light CTL Server Message Received 6 seconds timeout callback on
 *             a specific element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightCtlSrHandleMsgRcvdTmrCback(meshElementId_t elementId)
{
  mmdlLightCtlSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  MmdlLightCtlSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
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
static void mmdlLightCtlSrStoreScene(void *pDesc, uint8_t sceneIdx)
{
  mmdlLightCtlSrDesc_t *pCtlDesc = (mmdlLightCtlSrDesc_t *)pDesc;

  MMDL_TRACE_INFO0("LIGHT CTL SR: Store");

  /* Store present state */
  if (pCtlDesc->pStoredState != NULL)
  {
    memcpy(&(pCtlDesc->pStoredState->ctlScenes[sceneIdx]), &pCtlDesc->pStoredState->present,
           sizeof(mmdlLightCtlState_t));
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
static void mmdlLightCtlSrRecallScene(meshElementId_t elementId, uint8_t sceneIdx,
                                      uint32_t transitionMs)
{
  mmdlLightCtlSrDesc_t *pDesc = NULL;

  /* Get the model instance descriptor */
  MmdlLightCtlSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    MMDL_TRACE_INFO2("LIGHT CTL SR: Recall elemid=%d transMs=%d", elementId, transitionMs);

    /* Recall state */
    mmdlLightCtlSrSetState(elementId, &(pDesc->pStoredState->ctlScenes[sceneIdx]), transitionMs, 0,
                           MMDL_STATE_UPDATED_BY_SCENE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local state as a result of a binding with a Light Lightness Actual state.
 *             The set is instantaneous.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] ltness     Lightness value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrSetBoundLtLtness(meshElementId_t elementId, uint16_t ltness)
{
  mmdlLightCtlSrDesc_t *pDesc = NULL;
  mmdlLightCtlState_t state;

  MMDL_TRACE_INFO1("LIGHT CTL SR: Set bound Lightness=0x%X", ltness);

  /* Get model instance descriptor */
  MmdlLightCtlSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    /* Update Lightness */
    state.ltness = ltness;
    state.temperature = pDesc->pStoredState->present.temperature;
    state.deltaUV = pDesc->pStoredState->present.deltaUV;

    mmdlLightCtlSrSetState(elementId, &state, 0, 0, MMDL_STATE_UPDATED_BY_BIND);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local state as a result of a binding with a Light CTL Temperature state.
 *             The set is instantaneous.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] pState     Pointer to the present Temperature state.
 *  \param[in] pState     Pointer to the target Temperature state.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrSetBoundTemp(meshElementId_t elementId, mmdlLightCtlTempSrState_t * pState,
                                mmdlLightCtlTempSrState_t * pTargetState)
{
  mmdlLightCtlSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO2("LIGHT CTL SR: Set bound Temp=0x%X Delta=0x%X",
                   pState->temperature, pState->deltaUV);

  /* Get model instance descriptor */
  MmdlLightCtlSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    /* Save present state if any. */
    if (pState != NULL)
    {
      pDesc->pStoredState->present.temperature = pState->temperature;
      pDesc->pStoredState->present.deltaUV = pState->deltaUV;
    }
    /* Save target state if any. */
    if (pTargetState != NULL)
    {
      pDesc->pStoredState->target.temperature = pTargetState->temperature;
      pDesc->pStoredState->target.deltaUV = pTargetState->deltaUV;
    }

    /* Save state in NVM for Power Up. */
    if(pDesc->fNvmSaveStates)
    {
      pDesc->fNvmSaveStates(elementId);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Local getter of the Light CTL Temperature Default state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    Default Temperature value.
 */
/*************************************************************************************************/
uint16_t MmdlLightCtlSrGetDefaultTemperature(meshElementId_t elementId)
{
  mmdlLightCtlSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  MmdlLightCtlSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    return pDesc->pStoredState->defaultTemperature;
  }

  return 0;
}

/*************************************************************************************************/
/*!
 *  \brief     Local getter of the Light CTL Delta UV Default state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    Default Delta UV value.
 */
/*************************************************************************************************/
uint16_t MmdlLightCtlSrGetDefaultDelta(meshElementId_t elementId)
{
  mmdlLightCtlSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  MmdlLightCtlSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    return pDesc->pStoredState->defaultDeltaUV;
  }

  return 0;
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between the Generic OnPowerUp and a Light CTL state as
 *             a result of a Power Up procedure.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveOnPowerUp2LightCtl(meshElementId_t tgtElementId, void *pStateValue)
{
  mmdlLightCtlState_t state;
  mmdlLightCtlSrDesc_t *pDesc = NULL;
  mmdlGenOnPowerUpState_t powerUpState;

  powerUpState = *(mmdlGenOnPowerUpState_t*)pStateValue;

  MmdlLightCtlSrGetDesc(tgtElementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    switch (powerUpState)
    {
      case MMDL_GEN_ONPOWERUP_STATE_OFF:
      case MMDL_GEN_ONPOWERUP_STATE_DEFAULT:
        state.deltaUV = pDesc->pStoredState->defaultDeltaUV;
        state.temperature = pDesc->pStoredState->defaultTemperature;
        state.ltness = mmdlLightLightnessActualSrGetState(tgtElementId);
        break;

      case MMDL_GEN_ONPOWERUP_STATE_RESTORE:
        /* Restore Last state */
        state.ltness = mmdlLightLightnessActualSrGetState(tgtElementId);

        if (pDesc->pStoredState->target.temperature != pDesc->pStoredState->present.temperature)
        {
          /* Transition was in progress. Restore target */
          state.temperature = pDesc->pStoredState->target.temperature;
        }
        else
        {
          /* Transition was not in progress. Restore Last state */
          state.temperature = pDesc->pStoredState->present.temperature;
        }

        if (pDesc->pStoredState->target.deltaUV != pDesc->pStoredState->present.deltaUV)
        {
          /* Transition was in progress. Restore target */
          state.deltaUV = pDesc->pStoredState->target.deltaUV;
        }
        else
        {
          /* Transition was not in progress. Restore Last state */
          state.deltaUV = pDesc->pStoredState->present.deltaUV;
        }
        break;

      default:
        return;
    }

    /* Change state locally. No transition time or delay is allowed. */
    mmdlLightCtlSrSetState(tgtElementId, &state, 0, 0, MMDL_STATE_UPDATED_BY_BIND);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between a Generic Level state and a Light CTL State as
 *             a result of an updated Generic Level state.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveGenLevel2LightCtl(meshElementId_t tgtElementId, void *pStateValue)
{
  mmdlLightCtlSrDesc_t *pDesc = NULL;
  int16_t level = *(int16_t*)pStateValue;
  uint16_t temperature;

  /* Get the model instance descriptor */
  MmdlLightCtlSrGetDesc(tgtElementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    /* Light CTL Temperature = T_MIN + (Generic Level + 32768) * (T_MAX - T_MIN) / 65535 */
    temperature = pDesc->pStoredState->minTemperature + (level + 32768) *
                  (pDesc->pStoredState->maxTemperature - pDesc->pStoredState->minTemperature) /
                  65535;
    /* Update Light Ctl state on target element */
    pDesc->pStoredState->present.temperature = temperature;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between a Light CTL state and a Light Lightness Actual state as
 *             a result of an updated Light Lightness Actual state.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveLtLtnessAct2LightCtl(meshElementId_t tgtElementId, void *pStateValue)
{
  /* Update Light CTL State on target element */
  MmdlLightCtlSrSetBoundLtLtness(tgtElementId, *(uint16_t *)pStateValue);
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Light CTL Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrInit(void)
{
  meshElementId_t elemId;
  mmdlLightCtlSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO0("LIGHT CTL SR: init");

  /* Set event callbacks */
  ctlCb.recvCback = MmdlEmptyCback;
  ctlCb.fResolveBind = MmdlBindResolve;
  ctlCb.fStoreScene = mmdlLightCtlSrStoreScene;
  ctlCb.fRecallScene = mmdlLightCtlSrRecallScene;

  /* Initialize timers */
  for (elemId = 0; elemId < pMeshConfig->elementArrayLen; elemId++)
  {
    /* Get the model instance descriptor */
    MmdlLightCtlSrGetDesc(elemId, &pDesc);

    if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
    {
      pDesc->srcAddr = MESH_ADDR_TYPE_UNASSIGNED;

      /* Set transition timer parameters*/
      pDesc->transitionTimer.handlerId = mmdlLightCtlSrHandlerId;
      pDesc->transitionTimer.msg.event = MMDL_LIGHT_CTL_SR_EVT_TMR_CBACK;
      pDesc->transitionTimer.msg.param = elemId;

      /* Set msg Received timer parameters*/
      pDesc->msgRcvdTimer.handlerId = mmdlLightCtlSrHandlerId;
      pDesc->msgRcvdTimer.msg.event = MMDL_LIGHT_CTL_SR_MSG_RCVD_TMR_CBACK;
      pDesc->msgRcvdTimer.msg.param = elemId;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Light CTL Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Light CTL Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrHandlerInit(wsfHandlerId_t handlerId)
{
  mmdlLightCtlSrHandlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Light CTL Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrHandler(wsfMsgHdr_t *pMsg)
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
        for (opcodeIdx = 0; opcodeIdx < MMDL_LIGHT_CTL_SR_NUM_RCVD_OPCODES; opcodeIdx++)
        {
          opcodeSize = MESH_OPCODE_SIZE(pModelMsg->msgRecvEvt.opCode);
          if (!memcmp(&mmdlLightCtlSrRcvdOpcodes[opcodeIdx],
                      pModelMsg->msgRecvEvt.opCode.opcodeBytes,
                      opcodeSize))
          {
            /* Process message */
            (void)mmdlLightCtlSrHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
          }
        }
        break;

      case MESH_MODEL_EVT_PERIODIC_PUB:
        pModelMsg = (meshModelEvt_t *)pMsg;

        /* Check if periodic publishing was not disabled. */
        if(pModelMsg->periodicPubEvt.nextPubTimeMs != 0)
        {
          /* Publishing is requested part of the periodic publishing. */
          MmdlLightCtlSrPublish(pModelMsg->periodicPubEvt.elementId);
        }
        break;

      case MMDL_LIGHT_CTL_SR_EVT_TMR_CBACK:
        mmdlLightCtlSrHandleTmrCback((meshElementId_t)pMsg->param);
        break;

      case MMDL_LIGHT_CTL_SR_MSG_RCVD_TMR_CBACK:
        mmdlLightCtlSrHandleMsgRcvdTmrCback((meshElementId_t)pMsg->param);
        break;

      default:
        MMDL_TRACE_WARN0("LIGHT CTL SR: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publish a Light CTL Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrPublish(meshElementId_t elementId)
{
  /* Publish Status */
  mmdlLightCtlSrSendStatus(elementId, MMDL_USE_PUBLICATION_ADDR, 0, FALSE);
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
void MmdlLightCtlSrRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback*/
  if (recvCback != NULL)
  {
    ctlCb.recvCback = recvCback;
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
void MmdlLightCtlSrStoreScene(void *pDesc, uint8_t sceneIdx)
{
  if (ctlCb.fStoreScene != NULL)
  {
    ctlCb.fStoreScene(pDesc, sceneIdx);
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
void MmdlLightCtlSrRecallScene(meshElementId_t elementId, uint8_t sceneIdx, uint32_t transitionMs)
{
  if (ctlCb.fRecallScene != NULL)
  {
    ctlCb.fRecallScene(elementId, sceneIdx, transitionMs);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Links the Main Element to the Temperature element.
 *
 *  \param[in] mainElementId  Identifier of the Main element
 *  \param[in] tempElementId  Identifier of the Temperature element
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrLinkElements(meshElementId_t mainElementId, meshElementId_t tempElementId)
{
  meshElementId_t elemId;
  mmdlLightCtlTempSrDesc_t *pTempDesc = NULL;
  mmdlLightCtlSrDesc_t *pCtlDesc = NULL;

  for (elemId = 0; elemId < pMeshConfig->elementArrayLen; elemId++)
  {
    if (elemId == tempElementId)
    {
      /* Get the model instance descriptor */
      mmdlLightCtlSrGetDesc(elemId, (void *)&pTempDesc, MMDL_LIGHT_CTL_TEMP_SR_MDL_ID);

      if (pTempDesc != NULL)
      {
        pTempDesc->mainElementId = mainElementId;
      }
    }
    else if (elemId == mainElementId)
    {
      /* Get the model instance descriptor */
      mmdlLightCtlSrGetDesc(elemId, (void *)&pCtlDesc, MMDL_LIGHT_CTL_SR_MDL_ID);

      if (pCtlDesc != NULL)
      {
        pCtlDesc->tempElementId = tempElementId;
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Light CTL State and a Generic OnPowerUp state.
 *
 *  \param[in] onPowerUpElemId  Element identifier where the OnPowerUp state resides.
 *  \param[in] ctlElemId        Element identifier where the Light CTL state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrBind2OnPowerUp(meshElementId_t onPowerUpElemId, meshElementId_t ctlElemId)
{
  /* Add Generic Power OnOff -> Light CTL binding */
  MmdlAddBind(MMDL_STATE_GEN_ONPOWERUP, MMDL_STATE_LT_CTL, onPowerUpElemId, ctlElemId,
              mmdlBindResolveOnPowerUp2LightCtl);
}

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Light Lightness Actual State and a Light CTL state.
 *             A bind between the Generic OnOff and Light CTL and Generic Level and Light CTL is
 *             created to support the lightness extension.
 *
 *  \param[in] ltElemId   Element identifier where the Light Lightness Actual state resides.
 *  \param[in] ctlElemId  Element identifier where the Light CTL state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrBind2LtLtnessAct(meshElementId_t ltElemId, meshElementId_t ctlElemId)
{
  /* Add Light Lightness Actual -> Light CTL binding */
  MmdlAddBind(MMDL_STATE_LT_LTNESS_ACT, MMDL_STATE_LT_CTL, ltElemId, ctlElemId,
              mmdlBindResolveLtLtnessAct2LightCtl);

  /* Add Gen Level -> Light CTL binding */
  MmdlAddBind(MMDL_STATE_GEN_LEVEL, MMDL_STATE_LT_CTL, ltElemId, ctlElemId,
              mmdlBindResolveGenLevel2LightCtl);
}

/*************************************************************************************************/
/*!
 *  \brief     Get the local Light CTL state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrGetState(meshElementId_t elementId)
{
  mmdlLightCtlSrDesc_t *pDesc = NULL;
  mmdlLightCtlSrStateUpdate_t event;

  /* Get model instance descriptor */
  MmdlLightCtlSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    /* Set event type */
    event.hdr.event = MMDL_LIGHT_CTL_SR_EVENT;
    event.hdr.param = MMDL_LIGHT_CTL_SR_STATE_UPDATE_EVENT;

    /* Set event parameters */
    event.elemId = elementId;
    event.ctlStates.state.ltness = pDesc->pStoredState->present.ltness;
    event.ctlStates.state.temperature = pDesc->pStoredState->present.temperature;
    event.ctlStates.state.deltaUV = pDesc->pStoredState->present.deltaUV;

    /* Send event to the upper layer */
    ctlCb.recvCback((wsfMsgHdr_t *)&event);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local Light CTL state.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] pTargetState Target State for this transaction. See ::mmdlLightCtlState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrSetState(meshElementId_t elementId, mmdlLightCtlState_t *pTargetState)
{
  mmdlLightCtlSrSetState(elementId, pTargetState, 0, 0, MMDL_STATE_UPDATED_BY_APP);
}
