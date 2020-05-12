/*************************************************************************************************/
/*!
 *  \file   mmdl_gen_default_trans_sr_main.c
 *
 *  \brief  Implementation of the Generic Default Transition Server model.
 *
 *  Copyright (c) 2010-2019 Arm Ltd. All Rights Reserved.
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
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"

#include "mmdl_types.h"
#include "mmdl_defs.h"
#include "mmdl_common.h"

#include "mmdl_gen_default_trans_sr_main.h"
#include "mmdl_gen_default_trans_sr_api.h"
#include "mmdl_gen_default_trans_sr.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Present state index in stored states */
#define PRESENT_STATE_IDX                   0

/*! Target state index in stored states */
#define TARGET_STATE_IDX                    1

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Generic Default Transition Server control block type definition */
typedef struct mmdlGenDefaultTransSrCb_tag
{
  mmdlEventCback_t          recvCback;  /*!< Model Generic Default Transaction received callback */
} mmdlGenDefaultTransSrCb_t;

/*! Generic Default Transition Server message handler type definition */
typedef void (*mmdlGenDefaultTransSrHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler ID */
wsfHandlerId_t mmdlGenDefaultTransSrHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlGenDefaultTransSrRcvdOpcodes[MMDL_GEN_DEFAULT_TRANS_SR_NUM_RCVD_OPCODES] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_DEFAULT_TRANS_GET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_DEFAULT_TRANS_SET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_DEFAULT_TRANS_SET_NO_ACK_OPCODE)} },
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlGenDefaultTransSrHandleMsg_t mmdlGenDefaultTransSrHandleMsg[MMDL_GEN_DEFAULT_TRANS_SR_NUM_RCVD_OPCODES] =
{
  mmdlGenDefaultTransSrHandleGet,
  mmdlGenDefaultTransSrHandleSet,
  mmdlGenDefaultTransSrHandleSetNoAck,
};

/*! Generic Default Transition Server Control Block */
static mmdlGenDefaultTransSrCb_t  defaultTransSrCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Searches for the Generic Default Transition model instance descriptor
 *              on the specified element.
 *
 *  \param[in]  elementId  Identifier of the Element implementing the model.
 *  \param[out] ppOutDesc  Double pointer to the descriptor.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void mmdlGenDefaultTransSrGetDesc(meshElementId_t elementId, mmdlGenDefaultTransSrDesc_t **ppOutDesc)
{
  uint8_t modelIdx;

  *ppOutDesc = NULL;

  /* Verify that the elementId is in the element array */
  if (elementId < pMeshConfig->elementArrayLen)
  {
    /* Look for the model instance */
    for (modelIdx = 0; modelIdx < pMeshConfig->pElementArray[elementId].numSigModels; modelIdx++)
    {
      if (pMeshConfig->pElementArray[elementId].pSigModelArray[modelIdx].modelId ==
          MMDL_GEN_DEFAULT_TRANS_SR_MDL_ID)
      {
        /* Matching model ID on elementId */
        *ppOutDesc = pMeshConfig->pElementArray[elementId].pSigModelArray[modelIdx].pModelDescriptor;
        break;
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model.
 *  \param[in] targetState     Target State for this transaction. See ::mmdlGenDefaultTransState_t.
 *  \param[in] transitionTime  Transition time for this transaction.
 *  \param[in] delay5Ms        Delay for executing transaction. Unit is 5ms.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenDefaultTransSrSetState(meshElementId_t elementId, mmdlGenDefaultTransState_t targetState,
                                          uint8_t transitionTime, uint8_t delay5Ms,
                                          mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlGenDefaultTransSrEvent_t event;
  mmdlGenDefaultTransSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO1("DEFAULT TRANS SR: Set State on elemId %d", elementId);

  /* Get model instance descriptor */
  mmdlGenDefaultTransSrGetDesc(elementId, &pDesc);

  if (pDesc == NULL)
  {
    event.hdr.status = MMDL_INVALID_ELEMENT;
  }
  else
  {
    /* Validate the new default transition time */
    if (TRANSITION_TIME_STEPS(targetState) != MMDL_GEN_TR_UNKNOWN)
    {
      event.hdr.status = MMDL_SUCCESS;

      /* Set the new default transition time */
      pDesc->pStoredStates[PRESENT_STATE_IDX] = targetState;
    }
    else
    {
      event.hdr.status = MMDL_INVALID_PARAM;
    }
  }

  /* Set event type */
  event.hdr.event = MMDL_GEN_DEFAULT_TRANS_SR_EVENT;
  event.hdr.param = MMDL_GEN_DEFAULT_TRANS_SR_STATE_UPDATE_EVENT;

  /* Set event parameters */
  event.statusEvent.elemId = elementId;
  event.statusEvent.state = targetState;
  event.statusEvent.stateUpdateSource = stateUpdateSrc;

  /* Publish state change */
  MmdlGenDefaultTransSrPublish(elementId);

  /* Send event to the upper layer */
  defaultTransSrCb.recvCback((wsfMsgHdr_t *)&event);

  (void)transitionTime;
  (void)delay5Ms;
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Generic Default Transition Status command to the specified destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  Indicates if message that triggered the status was received on unicast.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenDefaultTransSrSendStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                            uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_DEFAULT_TRANS_SR_MDL_ID, MMDL_GEN_DEFAULT_TRANS_STATUS_OPCODE);
  uint8_t paramMsg[MMDL_GEN_DEFAULT_TRANS_MSG_LENGTH];
  mmdlGenDefaultTransSrDesc_t *pDesc = NULL;

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = dstAddr;
  msgInfo.ttl = MESH_USE_DEFAULT_TTL;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Get the model instance descriptor */
  mmdlGenDefaultTransSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Copy the message parameters from the descriptor */
    paramMsg[0] = pDesc->pStoredStates[PRESENT_STATE_IDX];

    /* Send message to the Mesh Core instantly */
    MeshSendMessage(&msgInfo, paramMsg, MMDL_GEN_DEFAULT_TRANS_MSG_LENGTH,
                    MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                    MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Default Transition Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenDefaultTransSrHandleGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    mmdlGenDefaultTransSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Default Transition Set Unacknowledged command.
 *
 *  \param[in] pMsg         Received model message.
 *  \param[in] ackRequired  TRUE if acknowledgement is required in response,  FALSE otherwise.
 *
 *  \return    TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlGenDefaultTransSrProcessSet(const meshModelMsgRecvEvt_t *pMsg, bool_t ackRequired)
{
  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  (void)ackRequired;

  /* Validate message length. It can take only min and max values. */
  if (pMsg->messageParamsLen != MMDL_GEN_DEFAULT_TRANS_MSG_LENGTH)
  {
    return FALSE;
  }

  /* Validate the new default transition time */
  if (TRANSITION_TIME_STEPS(pMsg->pMessageParams[0]) == MMDL_GEN_TR_UNKNOWN)
  {
    return FALSE;
  }

  /* Change state */
  mmdlGenDefaultTransSrSetState(pMsg->elementId, pMsg->pMessageParams[0], 0, 0, MMDL_STATE_UPDATED_BY_CL);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Default Transaction Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenDefaultTransSrHandleSet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  if (mmdlGenDefaultTransSrProcessSet(pMsg, TRUE))
  {
    /* Send Status message as a response to the Set message */
    mmdlGenDefaultTransSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Default Transition Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenDefaultTransSrHandleSetNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  (void)mmdlGenDefaultTransSrProcessSet(pMsg, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Converts the transition time to remaining time expressed in milliseconds.
 *
 *  \param[in] transitionTime  Transition time.
 *
 *  \return    Time in milliseconds.
 */
/*************************************************************************************************/
static uint32_t transitionTimeToMs(uint8_t transitionTime)
{
  switch (TRANSITION_TIME_RESOLUTION(transitionTime))
  {
    case MMDL_GEN_TR_RES100MS:
      return TRANSITION_TIME_STEPS(transitionTime) * MMDL_GEN_TR_TIME_RES0_MS;
    case MMDL_GEN_TR_RES1SEC:
      return TRANSITION_TIME_STEPS(transitionTime) * MMDL_GEN_TR_TIME_RES1_MS;
    case MMDL_GEN_TR_RES10SEC:
      return TRANSITION_TIME_STEPS(transitionTime) * MMDL_GEN_TR_TIME_RES2_MS;
    case MMDL_GEN_TR_RES10MIN:
      return TRANSITION_TIME_STEPS(transitionTime) * MMDL_GEN_TR_TIME_RES3_MS;
    default:
      break;
  }
  return 0;
}

/*************************************************************************************************/
/*!
 *  \brief     Converts the remaining time expressed in milliseconds to transition time.
 *
 *  \param[in] remainingTimeMs  Remaining time in milliseconds.
 *
 *  \return    Transition time.
 */
/*************************************************************************************************/
static uint8_t remainingTimeToTransitionTime(uint32_t remainingTimeMs)
{
  uint8_t transitionTime;

  WSF_ASSERT(remainingTimeMs < MMDL_GEN_TR_MAX_TIME_RES3_MS);

  /* Establish step resolution */
  if (remainingTimeMs > MMDL_GEN_TR_MAX_TIME_RES3_MS)
  {
    /* Remaining time cannot be represented. Set as unknown */
    transitionTime = MMDL_GEN_TR_UNKNOWN;
  }
  else if (remainingTimeMs > MMDL_GEN_TR_MAX_TIME_RES2_MS)
  {
    transitionTime = (uint8_t)(remainingTimeMs / MMDL_GEN_TR_TIME_RES3_MS);
    SET_TRANSITION_TIME_RESOLUTION(transitionTime, MMDL_GEN_TR_RES10MIN);
  }
  else if (remainingTimeMs > MMDL_GEN_TR_MAX_TIME_RES1_MS)
  {
    transitionTime = (uint8_t)(remainingTimeMs / MMDL_GEN_TR_TIME_RES2_MS);
    SET_TRANSITION_TIME_RESOLUTION(transitionTime, MMDL_GEN_TR_RES10SEC);
  }
  else if (remainingTimeMs > MMDL_GEN_TR_MAX_TIME_RES0_MS)
  {
    transitionTime = (uint8_t)(remainingTimeMs / MMDL_GEN_TR_TIME_RES1_MS);
    SET_TRANSITION_TIME_RESOLUTION(transitionTime, MMDL_GEN_TR_RES1SEC);
  }
  else
  {
    transitionTime = (uint8_t)(remainingTimeMs / MMDL_GEN_TR_TIME_RES0_MS);
    SET_TRANSITION_TIME_RESOLUTION(transitionTime, MMDL_GEN_TR_RES100MS);
  }
  return transitionTime;
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Generic Default Transition Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlGenDefaultTransSrInit(void)
{
  MMDL_TRACE_INFO0("DEFAULT TRANS SR: init");

  /* Set event callbacks */
  defaultTransSrCb.recvCback = MmdlEmptyCback;
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Generic Default Transition Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Generic Default Transition Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenDefaultTransSrHandlerInit(wsfHandlerId_t handlerId)
{
  /* Set handler ID */
  mmdlGenDefaultTransSrHandlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Generic Default Transition Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenDefaultTransSrHandler(wsfMsgHdr_t *pMsg)
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
        if (MESH_OPCODE_SIZE(pModelMsg->msgRecvEvt.opCode) == MMDL_GEN_DEFAULT_TRANS_OPCODES_SIZE)
        {
          /* Match the received opcode */
          for (opcodeIdx = 0; opcodeIdx < MMDL_GEN_DEFAULT_TRANS_SR_NUM_RCVD_OPCODES; opcodeIdx++)
          {
            if (!memcmp(&mmdlGenDefaultTransSrRcvdOpcodes[opcodeIdx],
                        pModelMsg->msgRecvEvt.opCode.opcodeBytes,
                        MMDL_GEN_DEFAULT_TRANS_OPCODES_SIZE))
            {
              /* Process message */
              (void)mmdlGenDefaultTransSrHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
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
          MmdlGenDefaultTransSrPublish(pModelMsg->periodicPubEvt.elementId);
        }
        break;

      default:
        MMDL_TRACE_WARN0("GEN DEFAULT TRANS SR: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publish a Gen Default Transition Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenDefaultTransSrPublish(meshElementId_t elementId)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_GEN_DEFAULT_TRANS_SR_MDL_ID,
                                             MMDL_GEN_DEFAULT_TRANS_STATUS_OPCODE);
  uint8_t paramMsg[MMDL_GEN_DEFAULT_TRANS_MSG_LENGTH];
  mmdlGenDefaultTransSrDesc_t *pDesc = NULL;

  /* Fill in the msg info parameters */
  pubMsgInfo.elementId = elementId;

  /* Get the model instance descriptor */
  mmdlGenDefaultTransSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Copy the message parameters from the descriptor */
    paramMsg[0] = pDesc->pStoredStates[PRESENT_STATE_IDX];

    /* Send message to the Mesh Core */
    MeshPublishMessage(&pubMsgInfo, paramMsg, MMDL_GEN_DEFAULT_TRANS_MSG_LENGTH);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local state.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] targetState  Target State for this transaction. See ::mmdlGenDefaultTransState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenDefaultTransSrSetState(meshElementId_t elementId, mmdlGenDefaultTransState_t targetState)
{
    /* Change state locally. No transition time or delay required. */
    mmdlGenDefaultTransSrSetState(elementId, targetState, 0, 0, MMDL_STATE_UPDATED_BY_APP);
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
void MmdlGenDefaultTransSrGetState(meshElementId_t elementId)
{
  mmdlGenDefaultTransSrEvent_t event;
  mmdlGenDefaultTransSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlGenDefaultTransSrGetDesc(elementId, &pDesc);

  /* Set event type */
  event.hdr.event = MMDL_GEN_DEFAULT_TRANS_SR_EVENT;
  event.hdr.param = MMDL_GEN_DEFAULT_TRANS_SR_CURRENT_STATE_EVENT;

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
  defaultTransSrCb.recvCback((wsfMsgHdr_t *)&event);
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
void MmdlGenDefaultTransSrRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback*/
  if (recvCback != NULL)
  {
    defaultTransSrCb.recvCback = recvCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Get the default transition value on the specified element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    Default transition time in milliseconds, or 0 if undefined.
 */
/*************************************************************************************************/
uint32_t MmdlGenDefaultTransGetTime(meshElementId_t elementId)
{
  mmdlGenDefaultTransSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlGenDefaultTransSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    return transitionTimeToMs(pDesc->pStoredStates[PRESENT_STATE_IDX]);
  }
  else
  {
    return 0;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Converts the transition time to remaining time expressed in milliseconds.
 *
 *  \param[in] transitionTime  Transition time.
 *
 *  \return    Time in milliseconds.
 */
/*************************************************************************************************/
uint32_t MmdlGenDefaultTransTimeToMs(uint8_t transitionTime)
{
  return transitionTimeToMs(transitionTime);
}

/*************************************************************************************************/
/*!
 *  \brief     Converts the remaining time expressed in milliseconds to transition time.
 *
 *  \param[in] remainingTimeMs  Remaining time in milliseconds.
 *
 *  \return    Transition time.
 */
/*************************************************************************************************/
uint8_t MmdlGenDefaultTimeMsToTransTime(uint32_t remainingTimeMs)
{
  return remainingTimeToTransitionTime(remainingTimeMs);
}
