/*************************************************************************************************/
/*!
 *  \file   mmdl_scene_sr_main.c
 *
 *  \brief  Implementation of the Scenes Server model.
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
#include "mmdl_scene_sr_api.h"
#include "mmdl_scene_sr.h"
#include "mmdl_scene_sr_main.h"
#include "mmdl_gen_onoff_sr.h"
#include "mmdl_gen_level_sr.h"
#include "mmdl_gen_powerlevel_sr.h"
#include "mmdl_lightlightness_sr.h"
#include "mmdl_light_hsl_sr_api.h"
#include "mmdl_light_hsl_sr.h"
#include "mmdl_light_ctl_sr_api.h"
#include "mmdl_light_ctl_sr.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Present scene index in stored scenes */
#define PRESENT_SCENE_IDX                   0

/*! Scene register index in stored scenes */
#define SCENE_REGISTER_IDX                  1

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Scenes Server control block type definition */
typedef struct mmdlSceneSrCb_tag
{
  mmdlEventCback_t          recvCback;            /*!< Model Scene Server received callback */
} mmdlSceneSrCb_t;

/*! Scenes Server message handler type definition */
typedef void (*mmdlSceneSrHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler ID */
wsfHandlerId_t mmdlSceneSrHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlSceneSrRcvdOpcodes[MMDL_SCENE_SR_NUM_RCVD_OPCODES] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_SCENE_GET_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_SCENE_REGISTER_GET_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_SCENE_RECALL_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_SCENE_RECALL_NO_ACK_OPCODE) }}
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlSceneSrHandleMsg_t mmdlSceneSrHandleMsg[MMDL_SCENE_SR_NUM_RCVD_OPCODES] =
{
  mmdlSceneSrHandleGet,
  mmdlSceneSrHandleRegisterGet,
  mmdlSceneSrHandleRecall,
  mmdlSceneSrHandleRecallNoAck
};

/*! Scenes Server Control Block */
/* For future enhancements */
/* static mmdlSceneSrCb_t  sceneSrCb; */

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Searches for the Scenes model instance descriptor on the specified element.
 *
 *  \param[in]  elementId  Identifier of the Element implementing the model.
 *  \param[out] ppOutDesc  Double pointer to the descriptor.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void mmdlSceneSrGetDesc(meshElementId_t elementId, mmdlSceneSrDesc_t **ppOutDesc)
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
        MMDL_SCENE_SR_MDL_ID)
    {
      /* Matching model ID on elementId */
      *ppOutDesc = pMeshConfig->pElementArray[elementId].pSigModelArray[modelIdx].pModelDescriptor;
      break;
    }
  }
}

/*************************************************************************************************/
/*!
 * \brief      Recalls model data for states that support scenes.
 *
 *  \param[in] sceneIdx      Scene index.
 *  \param[in] transitionMs  Transition time in ms.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlSceneSrRecallModelData(uint8_t sceneIdx, uint32_t transitionMs)
{
  meshElementId_t elemId;
  uint8_t modelIdx;

  for (elemId = 0; elemId < pMeshConfig->elementArrayLen; elemId++)
  {
    for (modelIdx = 0; modelIdx < pMeshConfig->pElementArray[elemId].numSigModels; modelIdx ++)
    {
      switch (pMeshConfig->pElementArray[elemId].pSigModelArray[modelIdx].modelId)
      {
        case MMDL_GEN_ONOFF_SR_MDL_ID:
          MmdlGenOnOffSrRecallScene(elemId, sceneIdx, transitionMs);
          break;

        case MMDL_GEN_LEVEL_SR_MDL_ID:
          MmdlGenLevelSrRecallScene(elemId, sceneIdx, transitionMs);
          break;

        case MMDL_GEN_POWER_LEVEL_SR_MDL_ID:
          MmdlGenPowerLevelSrRecallScene(elemId, sceneIdx, transitionMs);
          break;

        case MMDL_LIGHT_HSL_SR_MDL_ID:
          MmdlLightHslSrRecallScene(elemId, sceneIdx, transitionMs);
          break;

        case MMDL_LIGHT_LIGHTNESS_SR_MDL_ID:
          MmdlLightLightnessSrRecallScene(elemId, sceneIdx, transitionMs);
          break;

        case MMDL_LIGHT_CTL_SR_MDL_ID:
          MmdlLightCtlSrRecallScene(elemId, sceneIdx, transitionMs);
          break;

        default:
          break;
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Recall the target scene.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model.
 *  \param[in] sceneIdx        Scene index in scene register.
 *  \param[in] transitionMs    Transition time in ms for this transaction.
 *  \param[in] delayMs         Delay in for executing transaction. Unit is 5ms.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlSceneSrRecall(meshElementId_t elementId, uint8_t sceneIdx,
                              uint32_t transitionMs, uint8_t delay5Ms,
                              mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlSceneSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlSceneSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    MMDL_TRACE_INFO3("SCENE SR: Recall Target=0x%X, TimeRem=%d ms, Delay=0x%X",
                     pDesc->pStoredScenes[SCENE_REGISTER_IDX + sceneIdx], transitionMs, delay5Ms);

    /* Update descriptor */
    pDesc->remainingTimeMs = transitionMs;
    pDesc->delay5Ms = delay5Ms;
    pDesc->targetSceneIdx = sceneIdx;

    /* Update Target and Present State */
    pDesc->pStoredScenes[PRESENT_SCENE_IDX] = 0;

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

      /* Recall scene with transition */
      mmdlSceneSrRecallModelData(sceneIdx, pDesc->remainingTimeMs);
    }
    else
    {
      /* Stop transition */
      if (pDesc->transitionTimer.isStarted)
      {
        WsfTimerStop(&pDesc->transitionTimer);
      }
      else
      {
        /* No transition with this scene. Recall it. */
        mmdlSceneSrRecallModelData(sceneIdx, 0);
      }

      /* Update Scene */
      pDesc->pStoredScenes[PRESENT_SCENE_IDX] = pDesc->pStoredScenes[SCENE_REGISTER_IDX + sceneIdx];

      /* Publish state change */
      MmdlSceneSrPublish(elementId);
    }
  }

  (void)stateUpdateSrc;
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Scenes Status command to the specified destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  Indicates if message that triggered the status was received on unicast.
 *  \param[in] opStatus       Operation status.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlSceneSrSendStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                  uint16_t appKeyIndex, bool_t recvOnUnicast,
                                  mmdlSceneStatus_t opStatus)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_SCENE_SR_MDL_ID, 0);
  uint8_t msgParams[MMDL_SCENE_STATUS_MAX_LEN];
  mmdlSceneSrDesc_t *pDesc = NULL;
  uint8_t *pMsgParams, tranTime;

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = dstAddr;
  msgInfo.ttl = MESH_USE_DEFAULT_TTL;
  msgInfo.appKeyIndex = appKeyIndex;
  msgInfo.opcode.opcodeBytes[0] = MMDL_SCENE_STATUS_OPCODE;

  /* Get the model instance descriptor */
  mmdlSceneSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    pMsgParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT8_TO_BSTREAM(pMsgParams, opStatus);
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredScenes[PRESENT_SCENE_IDX]);

    if ((pDesc->remainingTimeMs > 0) && (opStatus == MMDL_SCENE_SUCCESS))
    {
      UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredScenes[SCENE_REGISTER_IDX + pDesc->targetSceneIdx]);

      if (pDesc->delay5Ms == 0)
      {
        /* Timer is running the transition */
        tranTime = MmdlGenDefaultTimeMsToTransTime(pDesc->transitionTimer.ticks *
                                                             WSF_MS_PER_TICK);
      }
      else
      {
        /* Timer is running the delay. Transition did not start. */
        tranTime = MmdlGenDefaultTimeMsToTransTime(pDesc->remainingTimeMs);
      }

      UINT8_TO_BSTREAM(pMsgParams, tranTime);

      MMDL_TRACE_INFO3("SCENE SR: Send Status Present=0x%X, Target=0x%X Time=0x%X",
                       pDesc->pStoredScenes[PRESENT_SCENE_IDX],
                       pDesc->pStoredScenes[SCENE_REGISTER_IDX + pDesc->targetSceneIdx], tranTime);
    }
    else
    {
      MMDL_TRACE_INFO2("SCENE SR: Send Status OpStatus=%d Present=0x%X", opStatus,
                       pDesc->pStoredScenes[PRESENT_SCENE_IDX]);
    }

    /* Send message to the Mesh Core */
    MeshSendMessage(&msgInfo, msgParams, (uint8_t)(pMsgParams - msgParams),
                    MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                    MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Scene Register Status command to the specified destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  Indicates if message that triggered the status was received on unicast.
 *  \param[in] opStatus       Operation status.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlSceneSrSendRegisterStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                   uint16_t appKeyIndex, bool_t recvOnUnicast,
                                   mmdlSceneStatus_t opStatus)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_SCENE_SR_MDL_ID, MMDL_SCENE_REGISTER_STATUS_OPCODE);
  mmdlSceneSrDesc_t *pDesc = NULL;
  uint8_t *pMsgParams, *pMsg, sceneIdx;

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = dstAddr;
  msgInfo.ttl = MESH_USE_DEFAULT_TTL;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Get the model instance descriptor */
  mmdlSceneSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Allocate memory for the message params and number of scenes */
    if ((pMsg = WsfBufAlloc(MMDL_SCENE_REG_STATUS_MAX_LEN)) == NULL)
    {
      return;
    }

    /* Copy the message parameters from the descriptor */
    pMsgParams = pMsg;
    UINT8_TO_BSTREAM(pMsgParams, opStatus);
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredScenes[PRESENT_SCENE_IDX]);

    /* Add all available scenes */
    for (sceneIdx = 0; sceneIdx < MMDL_NUM_OF_SCENES; sceneIdx++)
    {
      if (pDesc->pStoredScenes[SCENE_REGISTER_IDX + sceneIdx] != 0)
      {
        UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredScenes[SCENE_REGISTER_IDX + sceneIdx]);
      }
    }

    MMDL_TRACE_INFO2("SCENE SR: Send Register Status = %d Present=0x%X", opStatus,
                     pDesc->pStoredScenes[PRESENT_SCENE_IDX]);

    /* Send message to the Mesh Core */
    MeshSendMessage(&msgInfo, pMsg, (uint8_t)(pMsgParams - pMsg),
                    MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                    MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));

    /* Free buffer */
    WsfBufFree(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Scenes Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlSceneSrHandleGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    mmdlSceneSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast,
                          MMDL_SCENE_SUCCESS);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Scenes Register Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlSceneSrHandleRegisterGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Register Status message as a response to the Register Get message */
    mmdlSceneSrSendRegisterStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                                  pMsg->recvOnUnicast, MMDL_SCENE_SUCCESS);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Processes Scene Recall commands.
 *
 *  \param[in]  pMsg          Received model message.
 *  \param[in]  ackRequired   TRUE if acknowledgement is required in response,  FALSE otherwise.
 *  \param[out] pOutOpStatus  Operation status. See ::mmdlSceneStatus.
 *
 *  \return     TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlSceneSrProcessRecall(const meshModelMsgRecvEvt_t *pMsg, bool_t ackRequired,
                                       mmdlSceneStatus_t *pOutOpStatus)
{
  mmdlSceneSrDesc_t *pDesc = NULL;
  uint8_t transactionId, sceneIdx;
  uint32_t transMs;
  mmdlSceneNumber_t sceneNum;

  /* Set default value */
  *pOutOpStatus = MMDL_SCENE_PROHIBITED;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length. It can take only min and max values. */
  if (pMsg->messageParamsLen != MMDL_SCENE_RECALL_MAX_LEN &&
      pMsg->messageParamsLen != MMDL_SCENE_RECALL_MIN_LEN)
  {
    return FALSE;
  }

  /* Extract scene number */
  BYTES_TO_UINT16(sceneNum, pMsg->pMessageParams);

  /* Check prohibited values for Scene Number */
  if (sceneNum == MMDL_SCENE_NUM_PROHIBITED)
  {
    return FALSE;
  }

  /* Check if it contains optional parameters */
  if (pMsg->messageParamsLen == MMDL_SCENE_RECALL_MAX_LEN)
  {
    /* Check prohibited values for Transition Time */
    if (TRANSITION_TIME_STEPS(pMsg->pMessageParams[MMDL_SCENE_RECALL_TRANSITION_IDX]) ==
        MMDL_GEN_TR_UNKNOWN)
    {
      return FALSE;
    }

    /* Get Transition time */
    transMs = MmdlGenDefaultTransTimeToMs(pMsg->pMessageParams[MMDL_SCENE_RECALL_TRANSITION_IDX]);
  }
  else
  {
    /* Get Default Transition time */
    transMs = MmdlGenDefaultTransGetTime(pMsg->elementId);
  }

  /* Get model instance descriptor */
  mmdlSceneSrGetDesc(pMsg->elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Get Transaction ID */
    transactionId = pMsg->pMessageParams[MMDL_SCENE_RECALL_TID_IDX];

    /* Validate message against last transaction */
    if ((pMsg->srcAddr == pDesc->srcAddr) && (transactionId == pDesc->transactionId))
    {
      return FALSE;
    }

    /* Update last transaction fields and restart 6 seconds timer */
    pDesc->ackPending = ackRequired;
    pDesc->srcAddr = pMsg->srcAddr;
    pDesc->transactionId = transactionId;
    pDesc->ackAppKeyIndex = pMsg->appKeyIndex;
    pDesc->ackForUnicast = pMsg->recvOnUnicast;

    /* Set default operation status */
    *pOutOpStatus = MMDL_SCENE_NOT_FOUND;

    /* Add all available scenes */
    for (sceneIdx = 0; sceneIdx < MMDL_NUM_OF_SCENES; sceneIdx++)
    {
      if (pDesc->pStoredScenes[SCENE_REGISTER_IDX + sceneIdx] == sceneNum)
      {
        /* Scene found */
        *pOutOpStatus = MMDL_SCENE_SUCCESS;
        break;
      }
    }

    pDesc->delayedStatus = *pOutOpStatus;

    WsfTimerStartMs(&pDesc->msgRcvdTimer, MSG_RCVD_TIMEOUT_MS);

    if (pDesc->delayedStatus == MMDL_SCENE_SUCCESS)
    {
      /* If target scene is present scene consider the transition complete*/
      if (pDesc->pStoredScenes[PRESENT_SCENE_IDX] == pDesc->pStoredScenes[SCENE_REGISTER_IDX + sceneIdx])
      {
        transMs = 0;
      }

      /* Change scene only when scene exists */
      if (pMsg->messageParamsLen == MMDL_SCENE_RECALL_MAX_LEN)
      {
        mmdlSceneSrRecall(pMsg->elementId, sceneIdx, transMs,
                          pMsg->pMessageParams[MMDL_SCENE_RECALL_DELAY_IDX],
                          MMDL_STATE_UPDATED_BY_CL);
      }
      else
      {
        mmdlSceneSrRecall(pMsg->elementId, sceneIdx, transMs, 0,
                          MMDL_STATE_UPDATED_BY_CL);
      }
    }

    /* If message is delayed return FALSE to avoid sending the ack */
    if (pDesc->delay5Ms != 0)
    {
      return FALSE;
    }
    else
    {
      return TRUE;
    }
  }
  else
  {
    return FALSE;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Scene Recall command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlSceneSrHandleRecall(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlSceneStatus_t opStatus = MMDL_SCENE_PROHIBITED;

  /* Change state */
  if (mmdlSceneSrProcessRecall(pMsg, TRUE, &opStatus) && (opStatus!= MMDL_SCENE_PROHIBITED))
  {
    /* Send Status message as a response to the Set message */
    mmdlSceneSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast,
                          opStatus);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Scene Recall Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlSceneSrHandleRecallNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlSceneStatus_t opStatus = MMDL_SCENE_PROHIBITED;

  (void)mmdlSceneSrProcessRecall(pMsg, FALSE, &opStatus);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Scenes Server timeout callback on a specific element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlSceneSrHandleTmrCback(meshElementId_t elementId)
{
  mmdlSceneSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlSceneSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    if (pDesc->delay5Ms != 0)
    {
      /* Reset Delay */
      pDesc->delay5Ms = 0;

      /* Timeout. Set state. */
      mmdlSceneSrRecall(elementId, pDesc->targetSceneIdx, pDesc->remainingTimeMs, 0,
                        MMDL_STATE_UPDATED_BY_CL);

      /* Send Status if it was a delayed Acknowledged Set */
      if (pDesc->ackPending)
      {
        mmdlSceneSrSendStatus(elementId, pDesc->srcAddr, pDesc->ackAppKeyIndex,
                              pDesc->ackForUnicast, pDesc->delayedStatus);
      }
    }
    else if (pDesc->remainingTimeMs != 0)
    {
      /* Reset Transition Time */
      pDesc->remainingTimeMs = 0;

      /* Timeout. Set state. */
      mmdlSceneSrRecall(elementId, pDesc->targetSceneIdx, 0, 0, MMDL_STATE_UPDATED_BY_CL);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Scenes Server Message Received 6 seconds timeout callback on
 *             a specific element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlSceneSrHandleMsgRcvdTmrCback(meshElementId_t elementId)
{
  mmdlSceneSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlSceneSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Reset source address and transaction ID for last stored transaction */
    pDesc->srcAddr = MESH_ADDR_TYPE_UNASSIGNED;
  }
}

/*************************************************************************************************/
/*!
 * \brief      Stores model data for states that support scenes.
 *
 *  \param[in] sceneIdx   Scene index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlSceneSrStoreModelData(uint8_t sceneIdx)
{
  uint8_t modelIdx;
  meshElementId_t elemId;
  void *pDesc;

  for (elemId = 0; elemId < pMeshConfig->elementArrayLen; elemId++)
  {
    for (modelIdx = 0; modelIdx < pMeshConfig->pElementArray[elemId].numSigModels; modelIdx ++)
    {
      pDesc = pMeshConfig->pElementArray[elemId].pSigModelArray[modelIdx].pModelDescriptor;

      switch (pMeshConfig->pElementArray[elemId].pSigModelArray[modelIdx].modelId)
      {
        case MMDL_GEN_ONOFF_SR_MDL_ID:
          MmdlGenOnOffSrStoreScene(pDesc, sceneIdx);
          break;

        case MMDL_GEN_LEVEL_SR_MDL_ID:
          MmdlGenLevelSrStoreScene(pDesc, sceneIdx);
          break;

        case MMDL_GEN_POWER_LEVEL_SR_MDL_ID:
          MmdlGenPowerLevelSrStoreScene(pDesc, sceneIdx);
          break;

        case MMDL_LIGHT_LIGHTNESS_SR_MDL_ID:
          MmdlLightLightnessSrStoreScene(pDesc, sceneIdx);
          break;

        case MMDL_LIGHT_HSL_SR_MDL_ID:
          MmdlLightHslSrStoreScene(pDesc, sceneIdx);
          break;

        case MMDL_LIGHT_CTL_SR_MDL_ID:
          MmdlLightCtlSrStoreScene(pDesc, sceneIdx);
          break;

        default:
          break;
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Stores the specified scene number on the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] sceneNum   Scene number.
 *
 *  \return    Scene store result. See ::mmdlSceneStatus.
 */
/*************************************************************************************************/
mmdlSceneStatus_t mmdlSceneSrStore(meshElementId_t elementId, mmdlSceneNumber_t sceneNum)
{
  uint8_t sceneIdx, emptyIdx = 0;
  mmdlSceneSrDesc_t *pDesc;

  /* Get scene descriptor */
  mmdlSceneSrGetDesc(elementId, &pDesc);

  if (pDesc == NULL)
  {
    return MMDL_SCENE_PROHIBITED;
  }

  for (sceneIdx = 0; sceneIdx < MMDL_NUM_OF_SCENES; sceneIdx++)
  {
    /* Find empty scene index */
    if ((emptyIdx == 0) && (pDesc->pStoredScenes[SCENE_REGISTER_IDX + sceneIdx] == 0))
    {
      emptyIdx = sceneIdx;
    }

    /* Find duplicate */
    if (pDesc->pStoredScenes[SCENE_REGISTER_IDX + sceneIdx] == sceneNum)
    {
      break;
    }
  }

  if (sceneIdx != MMDL_NUM_OF_SCENES)
  {
    /* Scene already found. Will be overwritten */
    pDesc->pStoredScenes[PRESENT_SCENE_IDX] = sceneNum;

    /* Store model data */
    mmdlSceneSrStoreModelData(sceneIdx);

    return MMDL_SCENE_SUCCESS;
  }
  else if (emptyIdx != 0)
  {
    /* No duplicate found. Write empty scene slot */
    pDesc->pStoredScenes[SCENE_REGISTER_IDX + emptyIdx] = sceneNum;
    pDesc->pStoredScenes[PRESENT_SCENE_IDX] = sceneNum;

    /* Store model data */
    mmdlSceneSrStoreModelData(emptyIdx);

    return MMDL_SCENE_SUCCESS;
  }
  else
  {
    /* No duplicate or empty slot found. */
    return MMDL_SCENE_REGISTER_FULL;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Deletes the specified scene number on the element.
 *
 *  \param[in] pDesc     Scene server descriptor.
 *  \param[in] sceneNum  Scene number.
 *
 *  \return    Scene store result. See ::mmdlSceneStatus.
 */
/*************************************************************************************************/
mmdlSceneStatus_t mmdlSceneSrDelete(mmdlSceneSrDesc_t *pDesc, mmdlSceneNumber_t sceneNum)
{
  uint8_t sceneIdx;

  for (sceneIdx = 0; sceneIdx < MMDL_NUM_OF_SCENES; sceneIdx++)
  {
    /* Find scene index */
    if (pDesc->pStoredScenes[SCENE_REGISTER_IDX + sceneIdx] == sceneNum)
    {
      /* Cancel ongoing transaction. */
      if ((pDesc->pStoredScenes[SCENE_REGISTER_IDX + pDesc->targetSceneIdx] == sceneNum) &&
          (pDesc->remainingTimeMs != 0))
      {
        pDesc->targetSceneIdx = 0;
        pDesc->remainingTimeMs = 0;

        if (pDesc->transitionTimer.isStarted)
        {
          WsfTimerStop(&pDesc->transitionTimer);
        }
      }

      /* Check if the deleted scene is the present scene. */
      if (pDesc->pStoredScenes[PRESENT_SCENE_IDX] == sceneNum)
      {
        pDesc->pStoredScenes[PRESENT_SCENE_IDX] = 0;
      }

      pDesc->pStoredScenes[SCENE_REGISTER_IDX + sceneIdx] = 0;
      break;
    }
  }

  if (sceneIdx == MMDL_NUM_OF_SCENES)
  {
    return MMDL_SCENE_NOT_FOUND;
  }
  else
  {
    return MMDL_SCENE_SUCCESS;
  }
}


/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Scenes Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlSceneSrInit(void)
{
  meshElementId_t elemId;
  mmdlSceneSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO0("SCENE SR: init");

  /* Set event callbacks */
  /* For future enhancement */
  /* sceneSrCb.recvCback = MmdlEmptyCback; */

  /* Initialize timers */
  for (elemId = 0; elemId < pMeshConfig->elementArrayLen; elemId++)
  {
    /* Get the model instance descriptor */
    mmdlSceneSrGetDesc(elemId, &pDesc);

    if (pDesc != NULL)
    {
      pDesc->srcAddr = MESH_ADDR_TYPE_UNASSIGNED;

      /* Set transition timer parameters*/
      pDesc->transitionTimer.handlerId = mmdlSceneSrHandlerId;
      pDesc->transitionTimer.msg.event = MMDL_SCENE_SR_EVT_TMR_CBACK;
      pDesc->transitionTimer.msg.param = elemId;

      /* Set msg Received timer parameters*/
      pDesc->msgRcvdTimer.handlerId = mmdlSceneSrHandlerId;
      pDesc->msgRcvdTimer.msg.event = MMDL_SCENE_SR_MSG_RCVD_TMR_CBACK;
      pDesc->msgRcvdTimer.msg.param = elemId;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Scenes Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Scenes Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneSrHandlerInit(wsfHandlerId_t handlerId)
{
  mmdlSceneSrHandlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Scenes Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneSrHandler(wsfMsgHdr_t *pMsg)
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
        if (MESH_OPCODE_SIZE(pModelMsg->msgRecvEvt.opCode) == 2)
        {
          /* Match the received opcode */
          for (opcodeIdx = 0; opcodeIdx < MMDL_SCENE_SR_NUM_RCVD_OPCODES; opcodeIdx++)
          {
            if (!memcmp(&mmdlSceneSrRcvdOpcodes[opcodeIdx],
                        pModelMsg->msgRecvEvt.opCode.opcodeBytes, MMDL_SCENE_OPCODES_SIZE))
            {
              /* Process message */
              (void)mmdlSceneSrHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
            }
          }
        }
        break;

      case MESH_MODEL_EVT_PERIODIC_PUB:
        pModelMsg = (meshModelEvt_t *)pMsg;

        /* Check if periodic publishing was not disabled. */
        if(pModelMsg->periodicPubEvt.nextPubTimeMs != 0)
        {
          /* Publishing is requested part of the periodic publishing. */
          MmdlSceneSrPublish(pModelMsg->periodicPubEvt.elementId);
        }
        break;

      case MMDL_SCENE_SR_EVT_TMR_CBACK:
        mmdlSceneSrHandleTmrCback((meshElementId_t)pMsg->param);
        break;

      case MMDL_SCENE_SR_MSG_RCVD_TMR_CBACK:
        mmdlSceneSrHandleMsgRcvdTmrCback((meshElementId_t)pMsg->param);
        break;

      default:
        MMDL_TRACE_WARN0("SCENE SR: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publish a Scene Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneSrPublish(meshElementId_t elementId)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_SCENE_SR_MDL_ID, 0);
  uint8_t msgParams[MMDL_SCENE_STATUS_MAX_LEN];
  mmdlSceneSrDesc_t *pDesc = NULL;
  uint8_t *pMsgParams, tranTime;

  /* Fill in the msg info parameters */
  pubMsgInfo.elementId = elementId;
  pubMsgInfo.opcode.opcodeBytes[0] = MMDL_SCENE_STATUS_OPCODE;

  /* Get the model instance descriptor */
  mmdlSceneSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    pMsgParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT8_TO_BSTREAM(pMsgParams, MMDL_SCENE_SUCCESS);
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredScenes[PRESENT_SCENE_IDX]);

    if (pDesc->remainingTimeMs > 0)
    {
      UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredScenes[SCENE_REGISTER_IDX + pDesc->targetSceneIdx]);

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

      MMDL_TRACE_INFO3("SCENE SR: Publish Present=0x%X, Target=0x%X Time=0x%X",
                       pDesc->pStoredScenes[PRESENT_SCENE_IDX],
                       pDesc->pStoredScenes[SCENE_REGISTER_IDX + pDesc->targetSceneIdx], tranTime);
    }
    else
    {
      MMDL_TRACE_INFO1("SCENE SR: Publish Present=0x%X", pDesc->pStoredScenes[PRESENT_SCENE_IDX]);
    }

    /* Send message to the Mesh Core */
    MeshPublishMessage(&pubMsgInfo, msgParams, (uint8_t)(pMsgParams - msgParams));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publish a Scene Register Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneSrPublishRegister(meshElementId_t elementId)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_SCENE_SR_MDL_ID, MMDL_SCENE_REGISTER_STATUS_OPCODE);
  mmdlSceneSrDesc_t *pDesc = NULL;
  uint8_t *pMsgParams, *pMsg, sceneIdx;

  /* Fill in the msg info parameters */
  pubMsgInfo.elementId = elementId;

  /* Get the model instance descriptor */
  mmdlSceneSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Allocate memory for the message params and number of scenes */
    if ((pMsg = WsfBufAlloc(MMDL_SCENE_REG_STATUS_MAX_LEN)) == NULL)
    {
      return;
    }

    /* Copy the message parameters from the descriptor */
    pMsgParams = pMsg;
    UINT8_TO_BSTREAM(pMsgParams, MMDL_SCENE_SUCCESS);
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredScenes[PRESENT_SCENE_IDX]);

    /* Add all available scenes */
    for (sceneIdx = 0; sceneIdx < MMDL_NUM_OF_SCENES; sceneIdx++)
    {
      if (pDesc->pStoredScenes[SCENE_REGISTER_IDX + sceneIdx] != 0)
      {
        UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredScenes[SCENE_REGISTER_IDX + sceneIdx]);
      }
    }

    MMDL_TRACE_INFO1("SCENE SR: Publish Register Present=0x%X", pDesc->pStoredScenes[PRESENT_SCENE_IDX]);

    /* Send message to the Mesh Core */
    MeshPublishMessage(&pubMsgInfo, pMsg, (uint8_t)(pMsgParams - pMsg));

    /* Free buffer */
    WsfBufFree(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Recalls a scene as a result of a binding with transition time.
 *
 *  \param[in] elementId   Identifier of the Element implementing the model.
 *  \param[in] sceneNumber Scene number.
 *  \param[in] transState  Transition time state.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneSrRecallSceneWithTrans(meshElementId_t elementId, mmdlSceneNumber_t sceneNumber,
                                     uint8_t transState)
{
  mmdlSceneSrDesc_t *pDesc;
  uint32_t transTimeMs = MmdlGenDefaultTransTimeToMs(transState);
  uint8_t sceneIdx;

  /* Get the model instance descriptor */
  mmdlSceneSrGetDesc(elementId, &pDesc);

  /* Search scene number */
  for (sceneIdx = 0; sceneIdx < MMDL_NUM_OF_SCENES; sceneIdx++)
  {
    if (pDesc->pStoredScenes[SCENE_REGISTER_IDX + sceneIdx] == sceneNumber)
    {
      /* Scene found */
      break;
    }
  }

  if(sceneIdx < MMDL_NUM_OF_SCENES)
  {
    mmdlSceneSrRecall(elementId, sceneIdx, transTimeMs, 0, MMDL_STATE_UPDATED_BY_BIND);
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
void MmdlSceneSrRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback*/
  /* For future enhancement */
  /*
  if (recvCback != NULL)
  {
    sceneSrCb.recvCback = recvCback;
  }
  */
}
