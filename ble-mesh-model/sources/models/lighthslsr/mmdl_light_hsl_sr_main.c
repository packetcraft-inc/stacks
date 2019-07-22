/*************************************************************************************************/
/*!
 *  \file   mmdl_light_hsl_sr_main.c
 *
 *  \brief  Implementation of the Light HSL Server model.
 *
 *  Copyright (c) 2010-2019 Arm Ltd.
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
#include "mesh_api.h"

#include "mmdl_types.h"
#include "mmdl_defs.h"
#include "mmdl_common.h"
#include "mmdl_bindings.h"
#include "mmdl_gen_default_trans_sr.h"
#include "mmdl_light_hsl_sr_api.h"
#include "mmdl_light_hsl_hue_sr_api.h"
#include "mmdl_light_hsl_sat_sr_api.h"
#include "mmdl_lightlightness_sr_api.h"
#include "mmdl_light_hsl_sr_main.h"
#include "mmdl_light_hsl_sr.h"
#include "mmdl_light_hsl_hue_sr.h"
#include "mmdl_light_hsl_sat_sr.h"
#include "mmdl_lightlightness_sr.h"
#include "mmdl_gen_level_sr.h"
#include "mmdl_gen_onoff_sr.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Light HSL Set Message TID index */
#define MMDL_SET_TID_IDX          6

/*! Light HSL Set Message TID index */
#define MMDL_SET_TRANSITION_IDX   7

/*! Light HSL Set Message TID index */
#define MMDL_SET_DELAY_IDX        8

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Light HSL Server control block type definition */
typedef struct mmdlLightHslSrCb_tag
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
}mmdlLightHslSrCb_t;

/*! Light HSL Server message handler type definition */
typedef void (*mmdlLightHslSrHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler ID */
wsfHandlerId_t mmdlLightHslSrHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlLightHslSrRcvdOpcodes[MMDL_LIGHT_HSL_SR_NUM_RCVD_OPCODES] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_HSL_GET_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_HSL_SET_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_HSL_SET_NO_ACK_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_HSL_TARGET_GET_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_HSL_DEFAULT_GET_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_HSL_RANGE_GET_OPCODE) }}
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlLightHslSrHandleMsg_t mmdlLightHslSrHandleMsg[MMDL_LIGHT_HSL_SR_NUM_RCVD_OPCODES] =
{
  mmdlLightHslSrHandleGet,
  mmdlLightHslSrHandleSet,
  mmdlLightHslSrHandleSetNoAck,
  mmdlLightHslSrHandleTargetGet,
  mmdlLightHslSrHandleDefaultGet,
  mmdlLightHslSrHandleRangeGet,
};

/*! Light HSL Server Control Block */
static mmdlLightHslSrCb_t  hslCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Searches for the Light HSL Server model instance descriptor on the specified element.
 *
 *  \param[in]  elementId  Identifier of the Element implementing the model.
 *  \param[out] ppOutDesc  Double pointer to the descriptor.
 *  \param[out] modelId    Lighting Model ID.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void mmdlLightHslSrGetDesc(meshElementId_t elementId, void **ppOutDesc, uint16_t modelId)
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
 *  \brief      Searches for the Light HSL Server model instance descriptor on the specified element.
 *
 *  \param[in]  elementId  Identifier of the Element implementing the model.
 *  \param[out] ppOutDesc  Double pointer to the descriptor.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void MmdlLightHslSrGetDesc(meshElementId_t elementId, mmdlLightHslSrDesc_t **ppOutDesc)
{
  mmdlLightHslSrGetDesc(elementId, (void *)ppOutDesc, MMDL_LIGHT_HSL_SR_MDL_ID);
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
static void mmdlLightHslSrSendMessage(meshElementId_t elementId, meshAddress_t serverAddr,
                                      uint8_t ttl, uint16_t appKeyIndex, const uint8_t *pParam,
                                      uint8_t paramLen, uint16_t opcode, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_LIGHT_HSL_SR_MDL_ID, opcode);

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
static void mmdlLightHslSrPublishMessage(meshElementId_t elementId, const uint8_t *pParam,
                                         uint8_t paramLen, uint16_t opcode)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_LIGHT_HSL_SR_MDL_ID, opcode);

  /* Fill in the msg info parameters */
  pubMsgInfo.elementId = elementId;

  /* Send message to the Mesh Core. Parameters are already stored in over-the-air order */
  MeshPublishMessage(&pubMsgInfo, (uint8_t *)pParam, paramLen);
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light HSL Range Status command to the specified destination address.
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
void mmdlLightHslSrSendRangeStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                   uint16_t appKeyIndex, bool_t recvOnUnicast, uint8_t opStatus)

{
  uint8_t msgParams[MMDL_LIGHT_HSL_RANGE_STATUS_LEN];
  uint8_t *pMsgParams;
  mmdlLightHslSrDesc_t *pDesc = NULL;

  /* Get the model instance descriptor */
  MmdlLightHslSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    pMsgParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT8_TO_BSTREAM(pMsgParams, opStatus);
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->minHue);
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->maxHue);
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->minSat);
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->maxSat);

    MMDL_TRACE_INFO3("LIGHT HSL SR: Send Range Status=%d MinHue=0x%X, MaxHue=0x%X",  opStatus,
                     pDesc->pStoredState->minHue, pDesc->pStoredState->maxHue);

    MMDL_TRACE_INFO2(" MinSat=0x%X, MaxSat=0x%X", pDesc->pStoredState->minSat,
                     pDesc->pStoredState->maxSat);

    if (dstAddr != MMDL_USE_PUBLICATION_ADDR)
    {
      mmdlLightHslSrSendMessage(elementId, dstAddr, MESH_USE_DEFAULT_TTL, appKeyIndex, msgParams,
                                MMDL_LIGHT_HSL_RANGE_STATUS_LEN, MMDL_LIGHT_HSL_RANGE_STATUS_OPCODE,
                                recvOnUnicast);
    }
    else
    {
      mmdlLightHslSrPublishMessage(elementId, msgParams, MMDL_LIGHT_HSL_RANGE_STATUS_LEN,
                                   MMDL_LIGHT_HSL_RANGE_STATUS_OPCODE);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light HSL Default Status command to the specified destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  TRUE if is a response to a unicast request, FALSE otherwise.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightHslSrSendDefaultStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                     uint16_t appKeyIndex, bool_t recvOnUnicast)

{
  uint8_t msgParams[MMDL_LIGHT_HSL_DEF_STATUS_LEN];
  uint8_t *pMsgParams;
  mmdlLightHslSrDesc_t *pDesc = NULL;
  mmdlLightLightnessState_t defaultLtness;

  /* Get the model instance descriptor */
  MmdlLightHslSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    pMsgParams = msgParams;

    /* Copy the message parameters from the descriptor */
    defaultLtness = mmdlLightLightnessDefaultSrGetState(elementId);
    UINT16_TO_BSTREAM(pMsgParams, defaultLtness);
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->defaultHue);
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->defaultSat);

    MMDL_TRACE_INFO3("LIGHT HSL SR: Send Default Ltness=%d Hue=0x%X, Sat=0x%X",
                    defaultLtness, pDesc->pStoredState->defaultHue,
                    pDesc->pStoredState->defaultSat);

    if (dstAddr != MMDL_USE_PUBLICATION_ADDR)
    {
      mmdlLightHslSrSendMessage(elementId, dstAddr, MESH_USE_DEFAULT_TTL, appKeyIndex, msgParams,
                                MMDL_LIGHT_HSL_DEF_STATUS_LEN, MMDL_LIGHT_HSL_DEFAULT_STATUS_OPCODE,
                                recvOnUnicast);
    }
    else
    {
      mmdlLightHslSrPublishMessage(elementId, msgParams, MMDL_LIGHT_HSL_DEF_STATUS_LEN,
                                   MMDL_LIGHT_HSL_DEFAULT_STATUS_OPCODE);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the HSL state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model.
 *  \param[in] pState          Pointer to Light HSL state.
 *  \param[in] transitionMs    Transition time in ms for this transaction.
 *  \param[in] delayMs         Delay in for executing transaction. Unit is 5ms.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightHslSrSetState(meshElementId_t elementId, mmdlLightHslState_t *pState,
                                   uint32_t transitionMs, uint8_t delay5Ms,
                                   mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlLightHslSrDesc_t *pDesc = NULL;
  mmdlLightHslSrStateUpdate_t event;

  /* Get model instance descriptor */
  MmdlLightHslSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    MMDL_TRACE_INFO3("LIGHT HSL SR: Set Target Ltness=0x%X Hue=%d Sat=0x%X",
                    pState->ltness, pState->hue, pState->saturation);
    MMDL_TRACE_INFO2("LIGHT HSL SR: TimeRem=%d ms Delay=0x%X",
                    transitionMs, delay5Ms);

    /* Update descriptor */
    pDesc->remainingTimeMs = transitionMs;
    pDesc->delay5Ms = delay5Ms;
    pDesc->updateSource = stateUpdateSrc;

    /* Update Target State if it has changed*/
    if (memcmp(&pDesc->pStoredState->target, pState, sizeof(mmdlLightHslState_t)))
    {
      memcpy(&pDesc->pStoredState->target, pState, sizeof(mmdlLightHslState_t));

      /* Save target state in NVM for Power Up. */
      if(pDesc->fNvmSaveStates)
      {
        pDesc->fNvmSaveStates(elementId);
      }
    }

    /* Check if the set is delayed */
    if (pDesc->delay5Ms > 0)
    {
      /* Start Timer */
      WsfTimerStartMs(&pDesc->transitionTimer, DELAY_5MS_TO_MS(pDesc->delay5Ms));

      /* State change is delayed */
      return;
    }
    /* Check if state will change after a transition or immediately */
    else if (pDesc->remainingTimeMs > 0)
    {
      /* Start Timer */
      WsfTimerStartMs(&pDesc->transitionTimer, pDesc->remainingTimeMs);
    }
    else
    {
      /* Stop transition */
      if (pDesc->transitionTimer.isStarted)
      {
        WsfTimerStop(&pDesc->transitionTimer);
      }

      /* Update State */
      pDesc->pStoredState->present.ltness = pDesc->pStoredState->target.ltness;
      pDesc->pStoredState->present.hue = pDesc->pStoredState->target.hue;
      pDesc->pStoredState->present.saturation = pDesc->pStoredState->target.saturation;

      /* Update State on bound Lightness, Gen Level and Gen OnOff elements */
      if ((stateUpdateSrc != MMDL_STATE_UPDATED_BY_BIND) &&
          (stateUpdateSrc != MMDL_STATE_UPDATED_BY_SCENE))
      {
        MmdlLightLightnessSrSetBoundState(elementId, pDesc->pStoredState->present.ltness);
        MmdlGenOnOffSrSetBoundState(elementId, pDesc->pStoredState->present.ltness > 0);
        MmdlGenLevelSrSetBoundState(elementId, pDesc->pStoredState->present.ltness - 0x8000);
      }

      /* Update State on bound Hue and Saturation elements */
      MmdlLightHslHueSrSetHue(pDesc->hueElementId, pDesc->pStoredState->present.hue);
      MmdlLightHslSatSrSetSaturation(pDesc->satElementId, pDesc->pStoredState->present.saturation);

      /* Save target state in NVM for Power Up. */
      if(pDesc->fNvmSaveStates)
      {
        pDesc->fNvmSaveStates(elementId);
      }

      /* Check for bindings on this state. Trigger bindings */
      if ((stateUpdateSrc != MMDL_STATE_UPDATED_BY_BIND) &&
          (stateUpdateSrc != MMDL_STATE_UPDATED_BY_SCENE) && (hslCb.fResolveBind))
      {
        hslCb.fResolveBind(elementId, MMDL_STATE_LT_HSL, &pDesc->pStoredState->present);
      }

      /* Publish state change */
      MmdlLightHslSrPublish(elementId);

      /* Set event type */
      event.hdr.event = MMDL_LIGHT_HSL_SR_EVENT;
      event.hdr.param = MMDL_LIGHT_HSL_SR_STATE_UPDATE_EVENT;

      /* Set event parameters */
      event.elemId = elementId;
      event.state.ltness = pDesc->pStoredState->present.ltness;
      event.state.hue = pDesc->pStoredState->present.hue;
      event.state.saturation = pDesc->pStoredState->present.saturation;

      /* Send event to the upper layer */
      hslCb.recvCback((wsfMsgHdr_t *)&event);
    }
  }

  (void)stateUpdateSrc;
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light HSL Status command to the specified destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  Indicates if message that triggered the status was received on unicast.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightHslSrSendStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                     uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  uint8_t msgParams[MMDL_LIGHT_HSL_STATUS_MAX_LEN];
  mmdlLightHslSrDesc_t *pDesc = NULL;
  uint8_t *pMsgParams, tranTime;

  /* Get the model instance descriptor */
  MmdlLightHslSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    pMsgParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->present.ltness);
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->present.hue);
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->present.saturation);

    if (pDesc->remainingTimeMs > 0)
    {
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
      MMDL_TRACE_INFO3("LIGHT HSL SR: Send Status Ltness=0x%X Hue=0x%X Sat=0x%X",
                       pDesc->pStoredState->present.ltness, pDesc->pStoredState->present.hue,
                       pDesc->pStoredState->present.saturation);
      MMDL_TRACE_INFO1(" remTime=%d", pDesc->remainingTimeMs);

      mmdlLightHslSrSendMessage(elementId, dstAddr, MESH_USE_DEFAULT_TTL, appKeyIndex, msgParams,
                                (uint8_t)(pMsgParams - msgParams), MMDL_LIGHT_HSL_STATUS_OPCODE,
                                recvOnUnicast);
    }
    else
    {
      MMDL_TRACE_INFO3("LIGHT HSL SR: Publish Status Ltness=0x%X Hue=0x%X Sat=0x%X",
                       pDesc->pStoredState->present.ltness, pDesc->pStoredState->present.hue,
                       pDesc->pStoredState->present.saturation);
      MMDL_TRACE_INFO1(" remTime=%d", pDesc->remainingTimeMs);

      mmdlLightHslSrPublishMessage(elementId, msgParams, (uint8_t)(pMsgParams - msgParams),
                                   MMDL_LIGHT_HSL_STATUS_OPCODE);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light HSL Target Status command to the specified destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  Indicates if message that triggered the status was received on unicast.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightHslSrSendTargetStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                           uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  uint8_t msgParams[MMDL_LIGHT_HSL_STATUS_MAX_LEN];
  mmdlLightHslSrDesc_t *pDesc = NULL;
  uint8_t *pMsgParams, tranTime;

  /* Get the model instance descriptor */
  MmdlLightHslSrGetDesc(elementId, &pDesc);


  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    pMsgParams = msgParams;

    if (pDesc->remainingTimeMs > 0)
    {
      /* Copy the message parameters from the descriptor */
      UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->target.ltness);
      UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->target.hue);
      UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->target.saturation);

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
    else
    {

      /* Copy the message parameters from the descriptor */
      UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->present.ltness);
      UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->present.hue);
      UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredState->present.saturation);
    }

    if (dstAddr != MMDL_USE_PUBLICATION_ADDR)
    {
      MMDL_TRACE_INFO3("LIGHT HSL SR: Send Target Status Ltness=0x%X Hue=0x%X Sat=0x%X",
                        pDesc->pStoredState->target.ltness, pDesc->pStoredState->target.hue,
                        pDesc->pStoredState->target.saturation);
      MMDL_TRACE_INFO1(" remTime=%d", pDesc->remainingTimeMs);

      mmdlLightHslSrSendMessage(elementId, dstAddr, MESH_USE_DEFAULT_TTL, appKeyIndex, msgParams,
                                (uint8_t)(pMsgParams - msgParams), MMDL_LIGHT_HSL_TARGET_STATUS_OPCODE,
                                recvOnUnicast);
    }
    else
    {
      MMDL_TRACE_INFO3("LIGHT HSL SR: Publish Target Ltness=0x%X Hue=0x%X Sat=0x%X",
                        pDesc->pStoredState->target.ltness, pDesc->pStoredState->target.hue,
                        pDesc->pStoredState->target.saturation);
      MMDL_TRACE_INFO1(" remTime=%d", pDesc->remainingTimeMs);

      mmdlLightHslSrPublishMessage(elementId, msgParams, (uint8_t)(pMsgParams - msgParams),
                                   MMDL_LIGHT_HSL_TARGET_STATUS_OPCODE);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light HSL Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightHslSrHandleGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    mmdlLightHslSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light HSL Target Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightHslSrHandleTargetGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Register Status message as a response to the Register Get message */
    mmdlLightHslSrSendTargetStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                                   pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light HSL Default Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightHslSrHandleDefaultGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Register Status message as a response to the Register Get message */
    mmdlLightHslSrSendDefaultStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                                   pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light HSL Range Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightHslSrHandleRangeGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Range Status message as a response to the Range Get message */
    mmdlLightHslSrSendRangeStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                                  pMsg->recvOnUnicast, MMDL_RANGE_SUCCESS);
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
static bool_t mmdlLightHslSrProcessSet(const meshModelMsgRecvEvt_t *pMsg, bool_t ackRequired)
{
  mmdlLightHslState_t state;
  mmdlLightHslSrDesc_t *pDesc = NULL;
  uint8_t tid, *pMsgParam;
  uint32_t transMs;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length. It can take only min and max values. */
  if (pMsg->messageParamsLen != MMDL_LIGHT_HSL_SET_MAX_LEN &&
      pMsg->messageParamsLen != MMDL_LIGHT_HSL_SET_MIN_LEN)
  {
    return FALSE;
  }

  /* Extract parameters */
  pMsgParam = pMsg->pMessageParams;
  BSTREAM_TO_UINT16(state.ltness, pMsgParam);
  BSTREAM_TO_UINT16(state.hue, pMsgParam);
  BSTREAM_TO_UINT16(state.saturation, pMsgParam);

  /* Check if it contains optional parameters */
  if (pMsg->messageParamsLen == MMDL_LIGHT_HSL_SET_MAX_LEN)
  {
    /* Check prohibited values for Transition Time */
    if (TRANSITION_TIME_STEPS(pMsg->pMessageParams[MMDL_SET_TRANSITION_IDX]) ==
        MMDL_GEN_TR_UNKNOWN)
    {
      return FALSE;
    }
  }

  /* Get the model instance descriptor */
  MmdlLightHslSrGetDesc(pMsg->elementId, &pDesc);

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
    if (pMsg->messageParamsLen == MMDL_LIGHT_HSL_SET_MAX_LEN)
    {
      /* Check if target state is different from current state */
      if ((state.hue == pDesc->pStoredState->present.hue) &&
          (state.saturation == pDesc->pStoredState->present.saturation) &&
          (state.ltness == pDesc->pStoredState->present.ltness))
      {
        /* Transition is considered complete */
        transMs = 0;
      }
      else
      {
        /* Get Transition time */
        transMs = MmdlGenDefaultTransTimeToMs(pMsg->pMessageParams[MMDL_SET_TRANSITION_IDX]);
      }

      /* Change state */
      mmdlLightHslSrSetState(pMsg->elementId, &state, transMs,
                             pMsg->pMessageParams[MMDL_SET_DELAY_IDX],
                             MMDL_STATE_UPDATED_BY_CL);
    }
    else
    {
      /* Check if target state is different from current state */
      if ((state.hue == pDesc->pStoredState->present.hue) &&
          (state.saturation == pDesc->pStoredState->present.saturation) &&
          (state.ltness == pDesc->pStoredState->present.ltness))
      {
        /* Transition is considered complete */
        transMs = 0;
      }
      else
      {
        /* Get Default Transition time */
        transMs = MmdlGenDefaultTransGetTime(pMsg->elementId);
      }

      /* Change state */
      mmdlLightHslSrSetState(pMsg->elementId, &state, transMs, 0, MMDL_STATE_UPDATED_BY_CL);
    }

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
void mmdlLightHslSrHandleSet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  if (mmdlLightHslSrProcessSet(pMsg, TRUE))
  {
    /* Send Status message as a response to the Set message */
    mmdlLightHslSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
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
void mmdlLightHslSrHandleSetNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  (void)mmdlLightHslSrProcessSet(pMsg, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Light HSL Server timeout callback on a specific element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightHslSrHandleTmrCback(meshElementId_t elementId)
{
  mmdlLightHslSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  MmdlLightHslSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    if (pDesc->delay5Ms != 0)
    {
      /* Reset Delay */
      pDesc->delay5Ms = 0;

      /* Timeout. Set state. */
      mmdlLightHslSrSetState(elementId, &pDesc->pStoredState->target, pDesc->remainingTimeMs, 0,
                             pDesc->updateSource);

      /* Send Status if it was a delayed Acknowledged Set */
      if (pDesc->ackPending)
      {
        mmdlLightHslSrSendStatus(elementId, pDesc->srcAddr, pDesc->ackAppKeyIndex,
                                 pDesc->ackForUnicast);
      }
    }
    else if (pDesc->remainingTimeMs != 0)
    {
      /* Reset Transition Time */
      pDesc->remainingTimeMs = 0;

      /* Timeout. Set state. */
      mmdlLightHslSrSetState(elementId, &pDesc->pStoredState->target, 0, 0,
                             pDesc->updateSource);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Light HSL Server Message Received 6 seconds timeout callback on
 *             a specific element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightHslSrHandleMsgRcvdTmrCback(meshElementId_t elementId)
{
  mmdlLightHslSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  MmdlLightHslSrGetDesc(elementId, &pDesc);

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
static void mmdlLightHslSrStoreScene(void *pDesc, uint8_t sceneIdx)
{
  mmdlLightHslSrDesc_t *pHslDesc = (mmdlLightHslSrDesc_t *)pDesc;

  MMDL_TRACE_INFO0("LIGHT HSL SR: Store");

  /* Store present state */
  if (pHslDesc->pStoredState != NULL)
  {
    memcpy(&(pHslDesc->pStoredState->hslScenes[sceneIdx]), &pHslDesc->pStoredState->present,
           sizeof(mmdlLightHslState_t));
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
static void mmdlLightHslSrRecallScene(meshElementId_t elementId, uint8_t sceneIdx,
                                      uint32_t transitionMs)
{
  mmdlLightHslSrDesc_t *pDesc = NULL;

  /* Get the model instance descriptor */
  MmdlLightHslSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    MMDL_TRACE_INFO2("LIGHT HSL SR: Recall elemid=%d transMs=%d", elementId, transitionMs);

    /* Recall state */
    mmdlLightHslSrSetState(elementId, &(pDesc->pStoredState->hslScenes[sceneIdx]), transitionMs, 0,
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
void MmdlLightHslSrSetBoundLtLtness(meshElementId_t elementId, uint16_t ltness)
{
  mmdlLightHslSrDesc_t *pDesc = NULL;
  mmdlLightHslState_t state;

  MMDL_TRACE_INFO1("LIGHT HSL SR: Set bound Lightness=0x%X", ltness);

  /* Get model instance descriptor */
  MmdlLightHslSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    /* Update Lightness */
    state.ltness = ltness;
    state.hue = pDesc->pStoredState->present.hue;
    state.saturation = pDesc->pStoredState->present.saturation;

    mmdlLightHslSrSetState(elementId, &state, 0, 0, MMDL_STATE_UPDATED_BY_BIND);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local state as a result of a binding with a Light HSL Hue state.
 *             The set is instantaneous.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] hue        Hue value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrSetBoundHue(meshElementId_t elementId, uint16_t hue)
{
  mmdlLightHslSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO1("LIGHT HSL SR: Set bound Hue=0x%X", hue);

  /* Get model instance descriptor */
  MmdlLightHslSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    /* Update Hue */
    pDesc->pStoredState->present.hue = hue;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local state as a result of a binding with a Light HSL Saturation state.
 *             The set is instantaneous.
 *
 *  \param[in] elementId   Identifier of the Element implementing the model.
 *  \param[in] saturation  Saturation value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrSetBoundSaturation(meshElementId_t elementId, uint16_t saturation)
{
  mmdlLightHslSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO1("LIGHT HSL SR: Set bound Saturation=0x%X", saturation);

  /* Get model instance descriptor */
  MmdlLightHslSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    /* Update Saturation */
    pDesc->pStoredState->present.saturation = saturation;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Local getter of the Light HSL Hue Default state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    Default Hue value.
 */
/*************************************************************************************************/
uint16_t MmdlLightHslSrGetDefaultHue(meshElementId_t elementId)
{
  mmdlLightHslSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  MmdlLightHslSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    return pDesc->pStoredState->defaultHue;
  }

  return 0;
}

/*************************************************************************************************/
/*!
 *  \brief     Local getter of the Light HSL Saturation Default state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    Default Saturation value.
 */
/*************************************************************************************************/
uint16_t MmdlLightHslSrGetDefaultSaturation(meshElementId_t elementId)
{
  mmdlLightHslSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  MmdlLightHslSrGetDesc(elementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    return pDesc->pStoredState->defaultSat;
  }

  return 0;
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between the Generic OnPowerUp and a Light HSL state as
 *             a result of a Power Up procedure.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveOnPowerUp2LightHsl(meshElementId_t tgtElementId, void *pStateValue)
{
  mmdlLightHslState_t state;
  mmdlLightHslSrDesc_t *pDesc = NULL;
  mmdlGenOnPowerUpState_t powerUpState;

  powerUpState = *(mmdlGenOnPowerUpState_t*)pStateValue;

  MmdlLightHslSrGetDesc(tgtElementId, &pDesc);

  if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
  {
    switch (powerUpState)
    {
      case MMDL_GEN_ONPOWERUP_STATE_OFF:
      case MMDL_GEN_ONPOWERUP_STATE_DEFAULT:
        state.saturation = pDesc->pStoredState->defaultSat;
        state.hue = pDesc->pStoredState->defaultHue;
        state.ltness = mmdlLightLightnessDefaultSrGetState(tgtElementId);
        break;

      case MMDL_GEN_ONPOWERUP_STATE_RESTORE:
        if (pDesc->pStoredState->target.ltness != pDesc->pStoredState->present.ltness)
        {
          /* Transition was in progress. Restore target */
          state.ltness = pDesc->pStoredState->target.ltness;
        }
        else
        {
          /* Transition was not in progress. Restore Last state */
          state.ltness = pDesc->pStoredState->present.ltness;
        }

        if (pDesc->pStoredState->target.hue != pDesc->pStoredState->present.hue)
        {
          /* Transition was in progress. Restore target */
          state.hue = pDesc->pStoredState->target.hue;
        }
        else
        {
          /* Transition was not in progress. Restore Last state */
          state.hue = pDesc->pStoredState->present.hue;
        }

        if (pDesc->pStoredState->target.saturation != pDesc->pStoredState->present.saturation)
        {
          /* Transition was in progress. Restore target */
          state.saturation = pDesc->pStoredState->target.saturation;
        }
        else
        {
          /* Transition was not in progress. Restore Last state */
          state.saturation = pDesc->pStoredState->present.saturation;
        }
        break;

      default:
        return;
    }

    /* Change state locally. No transition time or delay is allowed. */
    mmdlLightHslSrSetState(tgtElementId, &state, 0, 0, MMDL_STATE_UPDATED_BY_BIND);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between a Generic On Off state and a Light HSL State as
 *             a result of an updated Generic On Off state.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveGenOnOff2LightHsl(meshElementId_t tgtElementId, void *pStateValue)
{
  /* Update Light Hsl state on target element */
  MmdlLightHslSrSetBoundStateOnOff(tgtElementId, *(mmdlGenOnOffState_t*)pStateValue);
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between a Generic Level state and a Light HSL State as
 *             a result of an updated Generic Level state.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveGenLevel2LightHsl(meshElementId_t tgtElementId, void *pStateValue)
{
  int16_t level = *(int16_t*)pStateValue;

  /* Update Light Hsl state on target element */
  MmdlLightHslSrSetBoundLtLtness(tgtElementId, level + 0x8000);
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between a Light HSL state and a Light Lightness Actual state as
 *             a result of an updated Light Lightness Actual state.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveLtLtnessAct2LightHsl(meshElementId_t tgtElementId, void *pStateValue)
{
  /* Update Light HSL State on target element */
  MmdlLightHslSrSetBoundLtLtness(tgtElementId, *(uint16_t *)pStateValue);
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Light HSL Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlLightHslSrInit(void)
{
  meshElementId_t elemId;
  mmdlLightHslSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO0("LIGHT HSL SR: init");

  /* Set event callbacks */
  hslCb.recvCback = MmdlEmptyCback;
  hslCb.fResolveBind = MmdlBindResolve;
  hslCb.fStoreScene = mmdlLightHslSrStoreScene;
  hslCb.fRecallScene = mmdlLightHslSrRecallScene;

  /* Initialize timers */
  for (elemId = 0; elemId < pMeshConfig->elementArrayLen; elemId++)
  {
    /* Get the model instance descriptor */
    MmdlLightHslSrGetDesc(elemId, &pDesc);

    if ((pDesc != NULL) && (pDesc->pStoredState != NULL))
    {
      pDesc->srcAddr = MESH_ADDR_TYPE_UNASSIGNED;

      /* Set transition timer parameters*/
      pDesc->transitionTimer.handlerId = mmdlLightHslSrHandlerId;
      pDesc->transitionTimer.msg.event = MMDL_LIGHT_HSL_SR_EVT_TMR_CBACK;
      pDesc->transitionTimer.msg.param = elemId;

      /* Set msg Received timer parameters*/
      pDesc->msgRcvdTimer.handlerId = mmdlLightHslSrHandlerId;
      pDesc->msgRcvdTimer.msg.event = MMDL_LIGHT_HSL_SR_MSG_RCVD_TMR_CBACK;
      pDesc->msgRcvdTimer.msg.param = elemId;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Light HSL Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Light HSL Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrHandlerInit(wsfHandlerId_t handlerId)
{
  mmdlLightHslSrHandlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Light HSL Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrHandler(wsfMsgHdr_t *pMsg)
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
        for (opcodeIdx = 0; opcodeIdx < MMDL_LIGHT_HSL_SR_NUM_RCVD_OPCODES; opcodeIdx++)
        {
          opcodeSize = MESH_OPCODE_SIZE(pModelMsg->msgRecvEvt.opCode);
          if (!memcmp(&mmdlLightHslSrRcvdOpcodes[opcodeIdx],
                      pModelMsg->msgRecvEvt.opCode.opcodeBytes,
                      opcodeSize))
          {
            /* Process message */
            (void)mmdlLightHslSrHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
          }
        }
        break;

      case MESH_MODEL_EVT_PERIODIC_PUB:
        pModelMsg = (meshModelEvt_t *)pMsg;

        /* Check if periodic publishing was not disabled. */
        if(pModelMsg->periodicPubEvt.nextPubTimeMs != 0)
        {
          /* Publishing is requested part of the periodic publishing. */
          MmdlLightHslSrPublish(pModelMsg->periodicPubEvt.elementId);
        }
        break;

      case MMDL_LIGHT_HSL_SR_EVT_TMR_CBACK:
        mmdlLightHslSrHandleTmrCback((meshElementId_t)pMsg->param);
        break;

      case MMDL_LIGHT_HSL_SR_MSG_RCVD_TMR_CBACK:
        mmdlLightHslSrHandleMsgRcvdTmrCback((meshElementId_t)pMsg->param);
        break;

      default:
        MMDL_TRACE_WARN0("LIGHT HSL SR: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publish a Light HSL Target Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrPublishTarget(meshElementId_t elementId)
{
  /* Publish Status */
  mmdlLightHslSrSendTargetStatus(elementId, MMDL_USE_PUBLICATION_ADDR, 0, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Publish a Light HSL Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrPublish(meshElementId_t elementId)
{
  /* Publish Status */
  mmdlLightHslSrSendStatus(elementId, MMDL_USE_PUBLICATION_ADDR, 0, FALSE);
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
void MmdlLightHslSrRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback*/
  if (recvCback != NULL)
  {
    hslCb.recvCback = recvCback;
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
void MmdlLightHslSrStoreScene(void *pDesc, uint8_t sceneIdx)
{
  if (hslCb.fStoreScene != NULL)
  {
    hslCb.fStoreScene(pDesc, sceneIdx);
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
void MmdlLightHslSrRecallScene(meshElementId_t elementId, uint8_t sceneIdx,
                               uint32_t transitionMs)
{
  if (hslCb.fRecallScene != NULL)
  {
    hslCb.fRecallScene(elementId, sceneIdx, transitionMs);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Links the Main Element to the Sat and Hue elements.
 *
 *  \param[in] mainElementId  Identifier of the Main element
 *  \param[in] hueElementId   Identifier of the Hue element
 *  \param[in] satElementId   Identifier of the Sat element
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrLinkElements(meshElementId_t mainElementId, meshElementId_t hueElementId,
                                meshElementId_t satElementId)
{
  meshElementId_t elemId;
  mmdlLightHslHueSrDesc_t *pHueDesc = NULL;
  mmdlLightHslSatSrDesc_t *pSatDesc = NULL;
  mmdlLightHslSrDesc_t *pHslDesc = NULL;

  for (elemId = 0; elemId < pMeshConfig->elementArrayLen; elemId++)
  {
    if (elemId == satElementId)
    {
      /* Get the model instance descriptor */
      mmdlLightHslSrGetDesc(elemId, (void *)&pSatDesc, MMDL_LIGHT_HSL_SAT_SR_MDL_ID);

      if (pSatDesc != NULL)
      {
        pSatDesc->mainElementId = mainElementId;
      }
    }
    else if (elemId == hueElementId)
    {
      /* Get the model instance descriptor */
      mmdlLightHslSrGetDesc(elemId, (void *)&pHueDesc, MMDL_LIGHT_HSL_HUE_SR_MDL_ID);

      if (pHueDesc != NULL)
      {
        pHueDesc->mainElementId = mainElementId;
      }
    }
    else if (elemId == mainElementId)
    {
      /* Get the model instance descriptor */
      mmdlLightHslSrGetDesc(elemId, (void *)&pHslDesc, MMDL_LIGHT_HSL_SR_MDL_ID);

      if (pHslDesc != NULL)
      {
        pHslDesc->hueElementId = hueElementId;
        pHslDesc->satElementId = satElementId;
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local state as a result of an OnOff binding. The set is instantaneous.
 *
 *  \param[in] elementId   Identifier of the Element implementing the model.
 *  \param[in] onOffState  New State. See ::mmdlGenOnOffStates
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrSetBoundStateOnOff(meshElementId_t elementId, mmdlGenOnOffState_t onOffState)
{
  uint16_t defState;
  mmdlLightHslSrDesc_t *pDesc = NULL;

  WSF_ASSERT(onOffState < MMDL_GEN_ONOFF_STATE_PROHIBITED);

  MmdlLightHslSrGetDesc(elementId, &pDesc);

  if (!pDesc)
  {
    return;
  }

  /* Get default state from the Light Lightness instance */
  defState = mmdlLightLightnessDefaultSrGetState(elementId);

  if (onOffState == MMDL_GEN_ONOFF_STATE_OFF)
  {
    pDesc->pStoredState->present.ltness = 0;
  }
  else if (defState != 0)
  {
    pDesc->pStoredState->present.ltness = defState;
  }
  else
  {
    pDesc->pStoredState->present.ltness = mmdlLightLightnessLastSrGetState(elementId);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Light HSL State and a Generic OnPowerUp state.
 *
 *  \param[in] onPowerUpElemId  Element identifier where the OnPowerUp state resides.
 *  \param[in] hslElemId        Element identifier where the Light HSL state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrBind2OnPowerUp(meshElementId_t onPowerUpElemId, meshElementId_t hslElemId)
{
  /* Add Generic Power OnOff -> Light HSL binding */
  MmdlAddBind(MMDL_STATE_GEN_ONPOWERUP, MMDL_STATE_LT_HSL, onPowerUpElemId, hslElemId,
              mmdlBindResolveOnPowerUp2LightHsl);
}

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Light Lightness Actual State and a Light HSL state.
 *             A bind between the Generic OnOff and Light HSL and Generic Level and Light HSL is
 *             created to support the lightness extension.
 *
 *  \param[in] ltElemId   Element identifier where the Light Lightness Actual state resides.
 *  \param[in] hslElemId  Element identifier where the Light HSL state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrBind2LtLtnessAct(meshElementId_t ltElemId, meshElementId_t hslElemId)
{
  /* Add Light Lightness Actual -> Light HSL binding */
  MmdlAddBind(MMDL_STATE_LT_LTNESS_ACT, MMDL_STATE_LT_HSL, ltElemId, hslElemId,
              mmdlBindResolveLtLtnessAct2LightHsl);

  /* Add Gen On Off -> Light HSL binding */
  MmdlAddBind(MMDL_STATE_GEN_ONOFF, MMDL_STATE_LT_HSL, ltElemId, hslElemId,
              mmdlBindResolveGenOnOff2LightHsl);

  /* Add Gen Level -> Light HSL binding */
  MmdlAddBind(MMDL_STATE_GEN_LEVEL, MMDL_STATE_LT_HSL, ltElemId, hslElemId,
              mmdlBindResolveGenLevel2LightHsl);
}
