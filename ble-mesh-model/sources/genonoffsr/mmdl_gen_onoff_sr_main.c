/*************************************************************************************************/
/*!
 *  \file   mmdl_gen_onoff_sr_main.c
 *
 *  \brief  Implementation of the Generic On Off Server model.
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
#include "mmdl_gen_onoff_sr_main.h"
#include "mmdl_gen_onoff_sr_api.h"

#include "mmdl_gen_onoff_sr.h"
#include "mmdl_gen_powonoff_sr.h"

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

/*! Generic On Off Server control block type definition */
typedef struct mmdlGenOnOffSrCb_tag
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
  mmdlEventCback_t          recvCback;            /*!< Model Generic OnOff received callback */
} mmdlGenOnOffSrCb_t;

/*! Generic On Off Server message handler type definition */
typedef void (*mmdlGenOnOffSrHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler ID */
wsfHandlerId_t mmdlGenOnOffSrHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlGenOnOffSrRcvdOpcodes[MMDL_GEN_ONOFF_SR_NUM_RCVD_OPCODES] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_ONOFF_GET_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_ONOFF_SET_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_ONOFF_SET_NO_ACK_OPCODE) }}
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlGenOnOffSrHandleMsg_t mmdlGenOnOffSrHandleMsg[MMDL_GEN_ONOFF_SR_NUM_RCVD_OPCODES] =
{
  mmdlGenOnOffSrHandleGet,
  mmdlGenOnOffSrHandleSet,
  mmdlGenOnOffSrHandleSetNoAck
};

/*! Generic On Off Server Control Block */
static mmdlGenOnOffSrCb_t  onOffSrCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Searches for the Generic On Off model instance descriptor on the specified element.
 *
 *  \param[in]  elementId  Identifier of the Element implementing the model.
 *  \param[out] ppOutDesc  Double pointer to the descriptor.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void mmdlGenOnOffSrGetDesc(meshElementId_t elementId, mmdlGenOnOffSrDesc_t **ppOutDesc)
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
        MMDL_GEN_ONOFF_SR_MDL_ID)
    {
      /* Matching model ID on elementId */
      *ppOutDesc = pMeshConfig->pElementArray[elementId].pSigModelArray[modelIdx].pModelDescriptor;
      break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model.
 *  \param[in] targetState     Target State for this transaction. See ::mmdlGenOnOffStates.
 *  \param[in] transitionMs    Transition time in ms for this transaction.
 *  \param[in] delayMs         Delay in for executing transaction. Unit is 5ms.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenOnOffSrSetState(meshElementId_t elementId, mmdlGenOnOffState_t targetState,
                                   uint32_t transitionMs, uint8_t delay5Ms,
                                   mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlGenOnOffSrStateUpdate_t event;
  mmdlGenOnOffSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO3("GEN ONOFF SR: Set Target=0x%X, TimeRem=%d ms, Delay=0x%X",
                    targetState, transitionMs, delay5Ms);

  /* Get model instance descriptor */
  mmdlGenOnOffSrGetDesc(elementId, &pDesc);

  if (pDesc == NULL)
  {
    event.hdr.status = MMDL_INVALID_ELEMENT;
  }
  else
  {
    event.hdr.status = MMDL_SUCCESS;

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

      /* State change is delayed */
      return;
    }
    /* Check if state will change after a transition or immediately */
    else if (pDesc->remainingTimeMs > 0)
    {
      /* Start Timer */
      WsfTimerStartMs(&pDesc->transitionTimer, pDesc->remainingTimeMs);

      if (targetState == MMDL_GEN_ONOFF_STATE_ON)
      {
        /* Binary state changes to 0x01 when transition starts */
        pDesc->pStoredStates[PRESENT_STATE_IDX] = targetState;

        /* Check for bindings on this state */
        if (onOffSrCb.fResolveBind)
        {
          onOffSrCb.fResolveBind(elementId, MMDL_STATE_GEN_ONOFF,
                                 &pDesc->pStoredStates[PRESENT_STATE_IDX]);
        }
      }
      else
      {
        /* State change event will be sent after the transition */
        return;
      }
    }
    else
    {
      /* Stop transition */
      if (pDesc->transitionTimer.isStarted)
      {
        WsfTimerStop(&pDesc->transitionTimer);
      }

      /* Update State */
      pDesc->pStoredStates[PRESENT_STATE_IDX] = targetState;

      /* Check for bindings on this state. Trigger bindings */
      if ((stateUpdateSrc != MMDL_STATE_UPDATED_BY_BIND) &&
          (stateUpdateSrc != MMDL_STATE_UPDATED_BY_SCENE) && (onOffSrCb.fResolveBind))
      {
        onOffSrCb.fResolveBind(elementId, MMDL_STATE_GEN_ONOFF,
                               &pDesc->pStoredStates[PRESENT_STATE_IDX]);
      }

      /* Publish state change */
      MmdlGenOnOffSrPublish(elementId);
    }
  }

  /* Set event type */
  event.hdr.event = MMDL_GEN_ONOFF_SR_EVENT;
  event.hdr.param = MMDL_GEN_ONOFF_SR_STATE_UPDATE_EVENT;

  /* Set event parameters */
  event.elemId = elementId;
  event.state = targetState;
  event.stateUpdateSource = stateUpdateSrc;

  /* Send event to the upper layer */
  onOffSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Generic On Off Status command to the specified destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  Indicates if message that triggered the status was received on unicast.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenOnOffSrSendStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                     uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_ONOFF_SR_MDL_ID, MMDL_GEN_ONOFF_STATUS_OPCODE);
  mmdlGenOnOffSrDesc_t *pDesc = NULL;
  uint8_t *pParams;
  uint8_t msgParams[MMDL_GEN_ONOFF_STATUS_MAX_LEN];
  uint8_t transTime;

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = dstAddr;
  msgInfo.ttl = MESH_USE_DEFAULT_TTL;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Get the model instance descriptor */
  mmdlGenOnOffSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    pParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT8_TO_BSTREAM(pParams, pDesc->pStoredStates[PRESENT_STATE_IDX]);

    if (pDesc->remainingTimeMs != 0)
    {
      UINT8_TO_BSTREAM(pParams, pDesc->pStoredStates[TARGET_STATE_IDX]);

      if (pDesc->delay5Ms == 0)
      {
        /* Timer is running the transition */
        transTime = MmdlGenDefaultTimeMsToTransTime(pDesc->transitionTimer.ticks * WSF_MS_PER_TICK);
      }
      else
      {
        /* Timer is running the delay. Transition did not start. */
        transTime = MmdlGenDefaultTimeMsToTransTime(pDesc->remainingTimeMs);
      }

      UINT8_TO_BSTREAM(pParams, transTime);
      MMDL_TRACE_INFO3("GEN ON OFF SR: Send Status Present=0x%02X, Target=0x%02X, TimeRem=0x%02X",
                        pDesc->pStoredStates[PRESENT_STATE_IDX],
                        pDesc->pStoredStates[TARGET_STATE_IDX], transTime);
    }
    else
    {
      MMDL_TRACE_INFO1("GEN ON OFF SR: Send Status Present=0x%02X",
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
 *  \brief     Handles a Generic On Off Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenOnOffSrHandleGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    mmdlGenOnOffSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Processes Generic On Off Set commands.
 *
 *  \param[in] pMsg         Received model message.
 *  \param[in] ackRequired  TRUE if acknowledgment is required in response,  FALSE otherwise.
 *
 *  \return    TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlGenOnOffSrProcessSet(const meshModelMsgRecvEvt_t *pMsg, bool_t ackRequired)
{
  mmdlGenOnOffSrDesc_t *pDesc = NULL;
  uint8_t transactionId;
  uint32_t transMs;
  uint32_t delayMs;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length. It can take only min and max values. */
  if (pMsg->messageParamsLen != MMDL_GEN_ONOFF_SET_MAX_LEN &&
      pMsg->messageParamsLen != MMDL_GEN_ONOFF_SET_MIN_LEN)
  {
    return FALSE;
  }

  /* Check prohibited values for On Off State */
  if (pMsg->pMessageParams[0] >= MMDL_GEN_ONOFF_STATE_PROHIBITED)
  {
    return FALSE;
  }

  /* Check if it contains optional parameters */
  if (pMsg->messageParamsLen == MMDL_GEN_ONOFF_SET_MAX_LEN)
  {
    /* Check prohibited values for Transition Time */
    if (TRANSITION_TIME_STEPS(pMsg->pMessageParams[MMDL_GEN_ONOFF_SET_TRANSITION_IDX]) ==
        MMDL_GEN_TR_UNKNOWN)
    {
      return FALSE;
    }

    /* Get Transition time */
    transMs = MmdlGenDefaultTransTimeToMs(pMsg->pMessageParams[MMDL_GEN_ONOFF_SET_TRANSITION_IDX]);
    delayMs = pMsg->pMessageParams[MMDL_GEN_ONOFF_SET_DELAY_IDX];
  }
  else
  {
    /* Get Default Transition time */
    transMs = MmdlGenDefaultTransGetTime(pMsg->elementId);
    delayMs = 0;
  }

  /* Get model instance descriptor */
  mmdlGenOnOffSrGetDesc(pMsg->elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Get Transaction ID */
    transactionId = pMsg->pMessageParams[MMDL_GEN_ONOFF_SET_TID_IDX];

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
    WsfTimerStartMs(&pDesc->msgRcvdTimer, MSG_RCVD_TIMEOUT_MS);

    /* Change state */
    mmdlGenOnOffSrSetState(pMsg->elementId, pMsg->pMessageParams[0], transMs, delayMs,
                           MMDL_STATE_UPDATED_BY_CL);

    /* Save states */
    if(pDesc->fNvmSaveStates)
    {
      pDesc->fNvmSaveStates(pMsg->elementId);
    }

    return (pDesc->delay5Ms == 0);
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic On Off Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenOnOffSrHandleSet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  if (mmdlGenOnOffSrProcessSet(pMsg, TRUE))
  {
    /* Send Status message as a response to the Set message */
    mmdlGenOnOffSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic On Off Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenOnOffSrHandleSetNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  (void)mmdlGenOnOffSrProcessSet(pMsg, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Generic On Off Server timeout callback on a specific element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenOnOffSrHandleTmrCback(meshElementId_t elementId)
{
  mmdlGenOnOffSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlGenOnOffSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    if (pDesc->delay5Ms != 0)
    {
      /* Reset Delay */
      pDesc->delay5Ms = 0;

      /* Timeout. Set state. */
      mmdlGenOnOffSrSetState(elementId, pDesc->pStoredStates[TARGET_STATE_IDX],
                             pDesc->remainingTimeMs, 0, pDesc->updateSource);

      /* Save states */
      if(pDesc->fNvmSaveStates)
      {
        pDesc->fNvmSaveStates(elementId);
      }

      /* Send Status if it was a delayed Acknowledged Set */
      if (pDesc->ackPending)
      {
        mmdlGenOnOffSrSendStatus(elementId, pDesc->srcAddr, pDesc->ackAppKeyIndex,
                               pDesc->ackForUnicast);
      }
    }
    else if (pDesc->remainingTimeMs != 0)
    {
      /* Reset Transition Time */
      pDesc->remainingTimeMs = 0;

      /* Transition to 'On' was made at the beginning. */
      if (pDesc->pStoredStates[TARGET_STATE_IDX] == MMDL_GEN_ONOFF_STATE_ON)
      {
        /* Publish state change */
        MmdlGenOnOffSrPublish(elementId);

        return;
      }

      /* Timeout. Set state. */
      mmdlGenOnOffSrSetState(elementId, pDesc->pStoredStates[TARGET_STATE_IDX], 0, 0,
                             pDesc->updateSource);

      /* Save states. */
      if(pDesc->fNvmSaveStates)
      {
        pDesc->fNvmSaveStates(elementId);
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles the Generic On Off Server Message Received 6 seconds timeout callback on
 *             a specific element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenOnOffSrHandleMsgRcvdTmrCback(meshElementId_t elementId)
{
  mmdlGenOnOffSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlGenOnOffSrGetDesc(elementId, &pDesc);

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
static void mmdlGenOnOffSrStoreScene(void *pDesc, uint8_t sceneIdx)
{
  mmdlGenOnOffSrDesc_t *pGenOnOffDesc = (mmdlGenOnOffSrDesc_t *)pDesc;

  MMDL_TRACE_INFO1("GEN ONOFF SR: Store onoff=%d",pGenOnOffDesc->pStoredStates[PRESENT_STATE_IDX]);

  /* Store present state */
  pGenOnOffDesc->pStoredStates[SCENE_STATE_IDX + sceneIdx] = pGenOnOffDesc->pStoredStates[PRESENT_STATE_IDX];
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
static void mmdlGenOnOffSrRecallScene(meshElementId_t elementId, uint8_t sceneIdx, uint32_t transitionMs)
{
  mmdlGenOnOffSrDesc_t *pDesc = NULL;

  /* Get the model instance descriptor */
  mmdlGenOnOffSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    MMDL_TRACE_INFO3("GEN ONOFF SR: Recall elemid=%d onoff=%d transMs=%d",
                     elementId, pDesc->pStoredStates[SCENE_STATE_IDX + sceneIdx], transitionMs);

    /* Recall state */
    mmdlGenOnOffSrSetState(elementId, pDesc->pStoredStates[SCENE_STATE_IDX + sceneIdx], transitionMs, 0,
                           MMDL_STATE_UPDATED_BY_SCENE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between the Generic OnPowerUp and a Generic OnOff state as
 *             a result of a Power Up procedure.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveOnPowerUp2OnOff(meshElementId_t tgtElementId, void *pStateValue)
{
  mmdlGenOnOffState_t state;
  mmdlGenOnPowerUpState_t powerUpState;
  mmdlGenOnOffSrDesc_t *pDesc = NULL;

  powerUpState = *(mmdlGenOnPowerUpState_t *)pStateValue;

  /* Get model instance descriptor */
  mmdlGenOnOffSrGetDesc(tgtElementId, &pDesc);

  if (!pDesc)
  {
    return;
  }

  switch (powerUpState)
  {
    case MMDL_GEN_ONPOWERUP_STATE_OFF:
      state = MMDL_GEN_ONOFF_STATE_OFF;
      break;

    case MMDL_GEN_ONPOWERUP_STATE_DEFAULT:
      state = MMDL_GEN_ONOFF_STATE_ON;
      break;

    case MMDL_GEN_ONPOWERUP_STATE_RESTORE:
      mmdlGenOnOffSrGetDesc(tgtElementId, &pDesc);
      if(pDesc == NULL)
      {
        return;
      }
      /* Always restore target value (unless a transition is pending, target is present) */
      state = pDesc->pStoredStates[TARGET_STATE_IDX];
      break;

    default:
      return;
  }

  /* Change state locally. No transition time or delay is allowed. */
  mmdlGenOnOffSrSetState(tgtElementId, state, 0, 0, MMDL_STATE_UPDATED_BY_BIND);

  /* Save states. */
  if(pDesc->fNvmSaveStates)
  {
    pDesc->fNvmSaveStates(tgtElementId);
  }
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Generic On Off Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlGenOnOffSrInit(void)
{
  mmdlGenOnOffSrDesc_t *pDesc = NULL;
  meshElementId_t elementId;

  MMDL_TRACE_INFO0("ON OFF SR: init");

  /* Set event callbacks */
  onOffSrCb.recvCback = MmdlEmptyCback;
  onOffSrCb.fResolveBind = MmdlBindResolve;
  onOffSrCb.fStoreScene = mmdlGenOnOffSrStoreScene;
  onOffSrCb.fRecallScene = mmdlGenOnOffSrRecallScene;

  /* Initialize timers */
  for(elementId = 0; elementId < pMeshConfig->elementArrayLen; elementId++)
  {
    /* Get the model instance descriptor */
    mmdlGenOnOffSrGetDesc(elementId, &pDesc);

    if (pDesc != NULL)
    {
      pDesc->srcAddr = MESH_ADDR_TYPE_UNASSIGNED;
      /* Set transition timer parameters*/
      pDesc->transitionTimer.handlerId = mmdlGenOnOffSrHandlerId;
      pDesc->transitionTimer.msg.event = MMDL_GEN_ON_OFF_SR_EVT_TMR_CBACK;
      pDesc->transitionTimer.msg.param = elementId;

      /* Set msg Received timer parameters*/
      pDesc->msgRcvdTimer.handlerId = mmdlGenOnOffSrHandlerId;
      pDesc->msgRcvdTimer.msg.event = MMDL_GEN_ON_OFF_SR_MSG_RCVD_TMR_CBACK;
      pDesc->msgRcvdTimer.msg.param = elementId;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Generic On Off Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Generic On Off Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenOnOffSrHandlerInit(wsfHandlerId_t handlerId)
{
  mmdlGenOnOffSrHandlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Generic On Off Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenOnOffSrHandler(wsfMsgHdr_t *pMsg)
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
        if (MESH_OPCODE_SIZE(pModelMsg->msgRecvEvt.opCode) == MMDL_GEN_ONOFF_OPCODES_SIZE)
        {
          /* Match the received opcode */
          for (opcodeIdx = 0; opcodeIdx < MMDL_GEN_ONOFF_SR_NUM_RCVD_OPCODES; opcodeIdx++)
          {
            if (!memcmp(&mmdlGenOnOffSrRcvdOpcodes[opcodeIdx],
                        pModelMsg->msgRecvEvt.opCode.opcodeBytes, MMDL_GEN_ONOFF_OPCODES_SIZE))
            {
              /* Process message */
              (void)mmdlGenOnOffSrHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
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
          MmdlGenOnOffSrPublish(pModelMsg->periodicPubEvt.elementId);
        }
        break;

      case MMDL_GEN_ON_OFF_SR_EVT_TMR_CBACK:
        mmdlGenOnOffSrHandleTmrCback((meshElementId_t)pMsg->param);
        break;

      case MMDL_GEN_ON_OFF_SR_MSG_RCVD_TMR_CBACK:
        mmdlGenOnOffSrHandleMsgRcvdTmrCback((meshElementId_t)pMsg->param);
        break;

      default:
        MMDL_TRACE_WARN0("GEN ON OFF SR: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publish a GenOnOff Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenOnOffSrPublish(meshElementId_t elementId)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_GEN_ONOFF_SR_MDL_ID,
                                             MMDL_GEN_ONOFF_STATUS_OPCODE);
  mmdlGenOnOffSrDesc_t *pDesc = NULL;
  uint8_t *pParams, tranTime;
  uint8_t msgParams[MMDL_GEN_ONOFF_STATUS_MAX_LEN];

  /* Fill in the msg info parameters */
  pubMsgInfo.elementId = elementId;

  /* Get the model instance descriptor */
  mmdlGenOnOffSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    pParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT8_TO_BSTREAM(pParams, pDesc->pStoredStates[PRESENT_STATE_IDX]);

    if (pDesc->remainingTimeMs > 0)
    {
      tranTime = MmdlGenDefaultTimeMsToTransTime(pDesc->transitionTimer.ticks * WSF_MS_PER_TICK);

      UINT8_TO_BSTREAM(pParams, pDesc->pStoredStates[TARGET_STATE_IDX]);
      UINT8_TO_BSTREAM(pParams, tranTime);
      MMDL_TRACE_INFO3("GEN ONOFF SR: Publish Present=0x%X, Target=0x%X, TimeRem=0x%X",
                       pDesc->pStoredStates[PRESENT_STATE_IDX],
                       pDesc->pStoredStates[TARGET_STATE_IDX], tranTime);
    }
    else
    {
      MMDL_TRACE_INFO1("GEN ONOFF SR: Publish Present=0x%X",
                        pDesc->pStoredStates[PRESENT_STATE_IDX]);
    }

    /* Send message to the Mesh Core */
    MeshPublishMessage(&pubMsgInfo, msgParams, (uint16_t)(pParams - msgParams));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local state.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] targetState  Target State for this transaction. See ::mmdlGenOnOffState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenOnOffSrSetState(meshElementId_t elementId, mmdlGenOnOffState_t targetState)
{
  mmdlGenOnOffSrDesc_t *pDesc;
  mmdlGenOnOffSrStateUpdate_t event;

  if (targetState >= MMDL_GEN_ONOFF_STATE_PROHIBITED)
  {
    /* Set event type */
    event.hdr.event = MMDL_GEN_ONOFF_SR_EVENT;
    event.hdr.param = MMDL_GEN_ONOFF_SR_STATE_UPDATE_EVENT;
    event.hdr.status = MMDL_INVALID_PARAM;
    event.elemId = elementId;
    event.stateUpdateSource = MMDL_STATE_UPDATED_BY_APP;
    event.state = targetState;

    /* Send event to the upper layer */
    onOffSrCb.recvCback((wsfMsgHdr_t *)&event);
  }
  else
  {
    /* Change state locally. No transition time or delay required. */
    mmdlGenOnOffSrSetState(elementId, targetState, 0, 0, MMDL_STATE_UPDATED_BY_APP);

    /* Get descriptor */
    mmdlGenOnOffSrGetDesc(elementId, &pDesc);
    /* Save states. */
    if((pDesc != NULL) && (pDesc->fNvmSaveStates != NULL))
    {
      pDesc->fNvmSaveStates(elementId);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Get the local state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenOnOffSrGetState(meshElementId_t elementId)
{
  mmdlGenOnOffSrCurrentState_t event;
  mmdlGenOnOffSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlGenOnOffSrGetDesc(elementId, &pDesc);

  /* Set event type */
  event.hdr.event = MMDL_GEN_ONOFF_SR_EVENT;
  event.hdr.param = MMDL_GEN_ONOFF_SR_CURRENT_STATE_EVENT;

  /* Set event parameters */
  event.elemId = elementId;

  if (pDesc == NULL)
  {
    /* No descriptor found on element */
    event.hdr.status = MMDL_INVALID_ELEMENT;

    /* Zero out parameters */
    event.state = 0;
  }
  else
  {
    /* Descriptor found on element */
    event.hdr.status = MMDL_SUCCESS;

    /* Set event parameters */
    event.state = pDesc->pStoredStates[PRESENT_STATE_IDX];
  }

  /* Send event to the upper layer */
  onOffSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local state as a result of a binding. The set is instantaneous.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] newState   New State. See ::mmdlGenOnOffState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenOnOffSrSetBoundState(meshElementId_t elementId, mmdlGenOnOffState_t newState)
{
  mmdlGenOnOffSrDesc_t *pDesc;

  /* Change state locally. No transition time or delay is allowed. */
  mmdlGenOnOffSrSetState(elementId, newState, 0, 0, MMDL_STATE_UPDATED_BY_BIND);

  /* Get descriptor */
  mmdlGenOnOffSrGetDesc(elementId, &pDesc);
  /* Save states. */
  if((pDesc != NULL) && (pDesc->fNvmSaveStates != NULL))
  {
    pDesc->fNvmSaveStates(elementId);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local state as a result of a binding with transition time.
 *
 *  \param[in] elementId   Identifier of the Element implementing the model.
 *  \param[in] newState    New State. See ::mmdlGenOnOffState_t
 *  \param[in] transState  Transition time state.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenOnOffSrSetBoundStateWithTrans(meshElementId_t elementId, mmdlGenOnOffState_t newState,
                                          uint8_t transTime)
{
  mmdlGenOnOffSrDesc_t *pDesc;
  uint32_t transTimeMs;

  /* Calculate value in ms. */
  transTimeMs = MmdlGenDefaultTransTimeToMs(transTime);

  /* Change state locally. No transition time or delay is allowed. */
  mmdlGenOnOffSrSetState(elementId, newState, transTimeMs, 0, MMDL_STATE_UPDATED_BY_BIND);

  if(transTimeMs == 0)
  {
    /* Get descriptor */
    mmdlGenOnOffSrGetDesc(elementId, &pDesc);
    /* Save states. */
    if((pDesc != NULL) && (pDesc->fNvmSaveStates != NULL))
    {
      pDesc->fNvmSaveStates(elementId);
    }
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
void MmdlGenOnOffSrStoreScene(void *pDesc, uint8_t sceneIdx)
{
  if (onOffSrCb.fStoreScene != NULL)
  {
    onOffSrCb.fStoreScene(pDesc, sceneIdx);
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
void MmdlGenOnOffSrRecallScene(meshElementId_t elementId, uint8_t sceneIdx,
                               uint32_t transitionMs)
{
  if (onOffSrCb.fRecallScene != NULL)
  {
    onOffSrCb.fRecallScene(elementId, sceneIdx, transitionMs);
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
void MmdlGenOnOffSrRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback*/
  if (recvCback != NULL)
  {
    onOffSrCb.recvCback = recvCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between the Generic OnPowerUp and the Generic OnOff state.
 *
 *  \param[in] onPowerUpElemId  Element identifier where the OnPowerUp state resides.
 *  \param[in] onOffElemId      Element identifier where the Generic OnOff state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenOnOffSrBind2OnPowerUp(meshElementId_t onPowerUpElemId, meshElementId_t onOffElemId)
{
  /* Add Generic Power OnOff -> Light Lightness Actual binding */
  MmdlAddBind(MMDL_STATE_GEN_ONPOWERUP, MMDL_STATE_GEN_ONOFF, onPowerUpElemId, onOffElemId,
              mmdlBindResolveOnPowerUp2OnOff);
}
