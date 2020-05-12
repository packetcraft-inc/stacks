/*************************************************************************************************/
/*!
 *  \file   mmdl_gen_level_sr_main.c
 *
 *  \brief  Implementation of the Time Server model.
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
#include "util/terminal.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"

#include "mmdl_types.h"
#include "mmdl_defs.h"
#include "mmdl_common.h"
#include "mmdl_time_sr_api.h"
#include "mmdl_time_sr_main.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Time Server control block type definition */
typedef struct mmdlTimeSrCb_tag
{
  mmdlEventCback_t          recvCback;        /*!< Model Time received callback */
}mmdlTimeSrCb_t;

/*! Time Server message handler type definition */
typedef void (*mmdlTimeSrHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler ID */
wsfHandlerId_t mmdlTimeSrHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlTimeSrRcvdOpcodes[MMDL_TIME_SR_NUM_RCVD_OPCODES] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_TIME_GET_OPCODE)} },
  { {UINT8_OPCODE_TO_BYTES(MMDL_TIME_STATUS_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_TIMEZONE_GET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_TIMEDELTA_GET_OPCODE)} },
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlTimeSrHandleMsg_t mmdlTimeSrHandleMsg[MMDL_TIME_SR_NUM_RCVD_OPCODES] =
{
  mmdlTimeSrHandleGet,
  mmdlTimeSrHandleStatus,
  mmdlTimeSrHandleZoneGet,
  mmdlTimeSrHandleDeltaGet,
};

/*! Time Server Control Block */
static mmdlTimeSrCb_t  timeSrCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Searches for the Time model instance descriptor on the specified element.
 *
 *  \param[in]  elementId  Identifier of the Element implementing the model.
 *  \param[out] ppOutDesc  Double pointer to the descriptor.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void mmdlTimeSrGetDesc(meshElementId_t elementId, mmdlTimeSrDesc_t **ppOutDesc)
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
        MMDL_TIME_SR_MDL_ID)
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
 *  \param[in] pTargetState    Target State for this transaction. See ::mmdlTimeState_t.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlTimeSrSetState(meshElementId_t elementId, mmdlTimeState_t *pTargetState,
                               mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlTimeSrEvent_t event;
  mmdlTimeSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO3("TIME SR: Set taiSeconds=0x%x subsecond=0x%x uncertainty=0x%x", pTargetState->taiSeconds,
                   pTargetState->subSecond, pTargetState->uncertainty);
  MMDL_TRACE_INFO3("TIME SR: Set timeauthority=%d delta=0x%x timezoneoffset=0x%x", pTargetState->timeAuthority,
                   pTargetState->taiUtcDelta, pTargetState->timeZoneOffset);

  /* Get model instance descriptor */
  mmdlTimeSrGetDesc(elementId, &pDesc);

  if (pDesc == NULL)
  {
    event.hdr.status = MMDL_INVALID_ELEMENT;
  }
  else
  {
    event.hdr.status = MMDL_SUCCESS;

    /* Update State */
    memcpy(&pDesc->storedTimeState, pTargetState, sizeof(mmdlTimeState_t));

    /* Publish state change */
    MmdlTimeSrPublish(elementId);
  }

  /* Set event type */
  event.hdr.event = MMDL_TIME_SR_EVENT;
  event.hdr.param = MMDL_TIME_SR_STATE_UPDATE_EVENT;

  /* Set event parameters */
  event.statusEvent.elemId = elementId;
  memcpy(&event.statusEvent.state.timeState, pTargetState, sizeof(mmdlTimeState_t));
  event.statusEvent.stateUpdateSource = stateUpdateSrc;

  /* Send event to the upper layer */
  timeSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Time Status command to the specified destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  Indicates if message that triggered the status was received on unicast.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlTimeSrSendStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                          uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_TIME_SR_MDL_ID, 0);
  mmdlTimeSrDesc_t *pDesc = NULL;
  uint8_t msgBuffer[MMDL_TIME_STATUS_MAX_LENGTH];
  uint8_t *pParams = msgBuffer, byte7;

  msgInfo.opcode.opcodeBytes[0] = MMDL_TIME_STATUS_OPCODE;

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = dstAddr;
  msgInfo.ttl = MESH_USE_DEFAULT_TTL;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Get the model instance descriptor */
  mmdlTimeSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Copy the message parameters from the descriptor */
    UINT40_TO_BSTREAM(pParams, pDesc->storedTimeState.taiSeconds);

    if (pDesc->storedTimeState.taiSeconds != 0x00)
    {
      UINT8_TO_BSTREAM(pParams, pDesc->storedTimeState.subSecond);
      UINT8_TO_BSTREAM(pParams, pDesc->storedTimeState.uncertainty);

      byte7 = ((pDesc->storedTimeState.taiUtcDelta & 0x7F) << 1) |
              pDesc->storedTimeState.timeAuthority;
      UINT8_TO_BSTREAM(pParams, byte7);
      UINT8_TO_BSTREAM(pParams, (uint8_t)(pDesc->storedTimeState.taiUtcDelta >> 7));
      UINT8_TO_BSTREAM(pParams, pDesc->storedTimeState.timeZoneOffset);

      MMDL_TRACE_INFO3("TIME SR: Send Status taiSeconds=0x%X subsecond=0x%X uncertainty=0x%X",
                       pDesc->storedTimeState.taiSeconds, pDesc->storedTimeState.subSecond,
                       pDesc->storedTimeState.uncertainty);
      MMDL_TRACE_INFO3("TIME SR: Set timeauthority=%d delta=0x%X timezoneoffset=0x%X",
                       pDesc->storedTimeState.timeAuthority, pDesc->storedTimeState.taiUtcDelta,
                       pDesc->storedTimeState.timeZoneOffset);
    }
    else
    {
      MMDL_TRACE_INFO1("TIME SR Send Status taiSeconds=0x%X", pDesc->storedTimeState.taiSeconds);
    }

    /* Send message to the Mesh Core */
    MeshSendMessage(&msgInfo, msgBuffer, (uint16_t)(pParams - msgBuffer),
                    MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                    MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Time Status command.
 *
 *  \param[in] pMsg         Received model message.
 *  \param[in] ackRequired  TRUE if acknowledgement is required in response,  FALSE otherwise.
 *
 *  \return    TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlTimeSrProcessStatus(const meshModelMsgRecvEvt_t *pMsg, bool_t ackRequired)
{
  mmdlTimeState_t state;
  mmdlTimeSrDesc_t *pDesc = NULL;
  uint8_t *pParams;

  (void)ackRequired;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length. It can take only min and max values. */
  if ((pMsg->messageParamsLen != MMDL_TIME_STATUS_MIN_LENGTH) &&
      (pMsg->messageParamsLen != MMDL_TIME_STATUS_MAX_LENGTH))
  {
    return FALSE;
  }

  /* Get model instance descriptor */
  mmdlTimeSrGetDesc(pMsg->elementId, &pDesc);

  if (pDesc != NULL)
  {
    if ((pDesc->storedTimeRoleState.timeRole != MMDL_TIME_ROLE_STATE_NONE) &&
        (pDesc->storedTimeRoleState.timeRole != MMDL_TIME_ROLE_STATE_AUTHORITY))
    {
      pParams = pMsg->pMessageParams;
      state.taiSeconds = 0;
      BSTREAM_TO_UINT40(state.taiSeconds, pParams);

      if (state.taiSeconds != 0)
      {
        BSTREAM_TO_UINT8(state.subSecond, pParams);
        BSTREAM_TO_UINT8(state.uncertainty, pParams);
        state.timeAuthority = ((0x80 & pParams[0]) >> 7);
        BSTREAM_TO_UINT16(state.taiUtcDelta, pParams);
        state.taiUtcDelta &= 0x7FFF;
        BSTREAM_TO_UINT8(state.timeZoneOffset, pParams);
      }

      /* Change state */
      mmdlTimeSrSetState(pMsg->elementId, &state, MMDL_STATE_UPDATED_BY_CL);
    }
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model.
 *  \param[in] pTargetState    Target State for this transaction. See ::mmdlTimeZoneState_t.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlTimeSrZoneSetState(meshElementId_t elementId, mmdlTimeZoneState_t *pTargetState,
                                   mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlTimeSrEvent_t event;
  mmdlTimeSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO2("TIME SR: Set offsetnew=0x%x taizonechange=0x%x",
                   pTargetState->offsetNew, pTargetState->taiZoneChange);

  /* Get model instance descriptor */
  mmdlTimeSrGetDesc(elementId, &pDesc);

  if (pDesc == NULL)
  {
    event.hdr.status = MMDL_INVALID_ELEMENT;
  }
  else
  {
    event.hdr.status = MMDL_SUCCESS;

    /* Update State */
    pDesc->storedTimeZoneState.offsetNew = pTargetState->offsetNew;
    pDesc->storedTimeZoneState.taiZoneChange = pTargetState->taiZoneChange;

    /* Publish state change */
    MmdlTimeSrPublish(elementId);
  }

  /* Set event type */
  event.hdr.event = MMDL_TIME_SR_EVENT;
  event.hdr.param = MMDL_TIMEZONE_SR_STATE_UPDATE_EVENT;

  /* Set event parameters */
  event.statusEvent.elemId = elementId;
  event.statusEvent.state.timeZoneState.offsetNew = pTargetState->offsetNew;
  event.statusEvent.state.timeZoneState.taiZoneChange = pTargetState->taiZoneChange;
  event.statusEvent.stateUpdateSource = stateUpdateSrc;

  /* Send event to the upper layer */
  timeSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Time Zone Status command to the specified destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  Indicates if message that triggered the status was received on unicast.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlTimeSrSendZoneStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                     uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_TIME_SR_MDL_ID, MMDL_TIMEZONE_STATUS_OPCODE);
  mmdlTimeSrDesc_t *pDesc = NULL;
  uint8_t msgBuffer[MMDL_TIMEZONE_STATUS_LENGTH];
  uint8_t *pParams = msgBuffer;

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = dstAddr;
  msgInfo.ttl = MESH_USE_DEFAULT_TTL;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Get the model instance descriptor */
  mmdlTimeSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Copy the message parameters from the descriptor */
    UINT8_TO_BSTREAM(pParams, pDesc->storedTimeState.timeZoneOffset);
    UINT8_TO_BSTREAM(pParams, pDesc->storedTimeZoneState.offsetNew);
    UINT40_TO_BSTREAM(pParams, pDesc->storedTimeZoneState.taiZoneChange);

    MMDL_TRACE_INFO3("TIME ZONE SR: Send Status current=0x%x new=0x%x change=0x%x",
                     pDesc->storedTimeState.timeZoneOffset,
                     pDesc->storedTimeZoneState.offsetNew,
                     pDesc->storedTimeZoneState.taiZoneChange);

    /* Send message to the Mesh Core */
    MeshSendMessage(&msgInfo, msgBuffer, MMDL_TIMEZONE_STATUS_LENGTH,
                    MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                    MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model.
 *  \param[in] pTargetState    Target State for this transaction. See ::mmdlTimeDeltaState_t.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlTimeDeltaSrSetState(meshElementId_t elementId, mmdlTimeDeltaState_t *pTargetState,
                                    mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlTimeSrEvent_t event;
  mmdlTimeSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO1("TIME DELTA: Set New=%d", pTargetState->deltaNew);

  /* Get model instance descriptor */
  mmdlTimeSrGetDesc(elementId, &pDesc);

  if (pDesc == NULL)
  {
    event.hdr.status = MMDL_INVALID_ELEMENT;
  }
  else
  {
    event.hdr.status = MMDL_SUCCESS;

    /* Update State */
    pDesc->storedTimeDeltaState.deltaChange = pTargetState->deltaChange;
    pDesc->storedTimeDeltaState.deltaNew = pTargetState->deltaNew;

    /* Publish state change */
    MmdlTimeSrPublish(elementId);
  }

  /* Set event type */
  event.hdr.event = MMDL_TIME_SR_EVENT;
  event.hdr.param = MMDL_TIMEDELTA_SR_STATE_UPDATE_EVENT;

  /* Set event parameters */
  event.statusEvent.elemId = elementId;
  event.statusEvent.state.timeDeltaState.deltaChange = pTargetState->deltaChange;
  event.statusEvent.state.timeDeltaState.deltaNew = pTargetState->deltaNew;
  event.statusEvent.stateUpdateSource = stateUpdateSrc;

  /* Send event to the upper layer */
  timeSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Time Delta Status command to the specified destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  Indicates if message that triggered the status was received on unicast.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlTimeSrSendDeltaStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                     uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_TIME_SR_MDL_ID, MMDL_TIMEDELTA_STATUS_OPCODE);
  mmdlTimeSrDesc_t *pDesc = NULL;
  uint8_t *pParams;
  uint8_t msgBuffer[MMDL_TIMEDELTA_STATUS_LENGTH];

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = dstAddr;
  msgInfo.ttl = MESH_USE_DEFAULT_TTL;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Get the model instance descriptor */
  mmdlTimeSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Copy the message parameters from the descriptor */
    pParams = msgBuffer;
    UINT16_TO_BSTREAM(pParams, pDesc->storedTimeState.taiUtcDelta);
    UINT16_TO_BSTREAM(pParams, pDesc->storedTimeDeltaState.deltaNew);
    UINT40_TO_BSTREAM(pParams, pDesc->storedTimeDeltaState.deltaChange);

    MMDL_TRACE_INFO3("TIME DELTA SR: Send Status current=0x%X new=0x%X change=0x%X",
                     pDesc->storedTimeState.taiUtcDelta, pDesc->storedTimeDeltaState.deltaNew,
                     pDesc->storedTimeDeltaState.deltaChange);

    /* Send message to the Mesh Core */
    MeshSendMessage(&msgInfo, msgBuffer, MMDL_TIMEDELTA_STATUS_LENGTH,
                    MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                    MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Time Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlTimeSrHandleGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    mmdlTimeSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Time Status command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlTimeSrHandleStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (mmdlTimeSrProcessStatus(pMsg, TRUE))
  {
    /* Send Status message as a response to the Get message */
    mmdlTimeSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Time Zone Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlTimeSrHandleZoneGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    mmdlTimeSrSendZoneStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Time Delta Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlTimeSrHandleDeltaGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    mmdlTimeSrSendDeltaStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Time Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlTimeSrInit(void)
{
  MMDL_TRACE_INFO0("TIME SR: init");

  /* Set event callbacks */
  timeSrCb.recvCback = MmdlEmptyCback;
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Time Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Time Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSrHandlerInit(wsfHandlerId_t handlerId)
{
  mmdlTimeSrHandlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Time Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSrHandler(wsfMsgHdr_t *pMsg)
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
        for (opcodeIdx = 0; opcodeIdx < MMDL_TIME_SR_NUM_RCVD_OPCODES; opcodeIdx++)
        {
          opcodeSize = MESH_OPCODE_SIZE(pModelMsg->msgRecvEvt.opCode);
          if (!memcmp(&mmdlTimeSrRcvdOpcodes[opcodeIdx], pModelMsg->msgRecvEvt.opCode.opcodeBytes,
              opcodeSize))
          {
            /* Process message */
            (void)mmdlTimeSrHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
          }
        }
        break;

      case MESH_MODEL_EVT_PERIODIC_PUB:
        pModelMsg = (meshModelEvt_t *)pMsg;

        /* Check if periodic publishing was not disabled. */
        if(pModelMsg->periodicPubEvt.nextPubTimeMs != 0)
        {
          /* Publishing is requested part of the periodic publishing */
          MmdlTimeSrPublish(pModelMsg->periodicPubEvt.elementId);
        }
        break;

      default:
        MMDL_TRACE_WARN0("TIME SR: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publish a Time Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSrPublish(meshElementId_t elementId)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_TIME_SR_MDL_ID,
                                             MMDL_TIME_STATUS_OPCODE);
  uint8_t msgBuffer[MMDL_TIME_STATUS_MAX_LENGTH];
  mmdlTimeSrDesc_t *pDesc = NULL;
  uint8_t *pParams = msgBuffer, byte7;

  pubMsgInfo.opcode.opcodeBytes[0] = MMDL_TIME_STATUS_OPCODE;

  /* Fill in the msg info parameters */
  pubMsgInfo.elementId = elementId;

  /* Get the model instance descriptor */
  mmdlTimeSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Copy the message parameters from the descriptor */
    UINT40_TO_BSTREAM(pParams, pDesc->storedTimeState.taiSeconds);

    if (pDesc->storedTimeState.taiSeconds != 0)
    {
      UINT8_TO_BSTREAM(pParams, pDesc->storedTimeState.subSecond);
      UINT8_TO_BSTREAM(pParams, pDesc->storedTimeState.uncertainty);

      byte7 = ((pDesc->storedTimeState.taiUtcDelta & 0x7F) << 1) |
              pDesc->storedTimeState.timeAuthority;
      UINT8_TO_BSTREAM(pParams, byte7);
      UINT8_TO_BSTREAM(pParams, (uint8_t)(pDesc->storedTimeState.taiUtcDelta >> 7));
      UINT8_TO_BSTREAM(pParams, pDesc->storedTimeState.timeZoneOffset);
    }

    /* Send message to the Mesh Core */
    MeshPublishMessage(&pubMsgInfo, msgBuffer, (uint16_t)(pParams - msgBuffer));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the Time state of the element.
 *
 *  \param[in] elementId     Identifier of the Element implementing the model.
 *  \param[in] pTargetState  Target State for this transaction. See ::mmdlTimeState_t.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSrSetState(meshElementId_t elementId, mmdlTimeState_t *pTargetState)
{
  /* Change state locally. No transition time or delay required. */
  mmdlTimeSrSetState(elementId, pTargetState, MMDL_STATE_UPDATED_BY_APP);
}

/*************************************************************************************************/
/*!
 *  \brief     Get the Time state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSrGetState(meshElementId_t elementId)
{
  mmdlTimeSrEvent_t event;
  mmdlTimeSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlTimeSrGetDesc(elementId, &pDesc);

  /* Set event type */
  event.hdr.event = MMDL_TIME_SR_EVENT;
  event.hdr.param = MMDL_TIME_SR_CURRENT_STATE_EVENT;


  /* Set event parameters */
  event.currentStateEvent.elemId = elementId;

  if (pDesc == NULL)
  {
    /* No descriptor found on element */
    event.hdr.status = MMDL_INVALID_ELEMENT;

    /* Zero out parameters */
    memset(&event.currentStateEvent.state.timeState, 0,
           sizeof(event.currentStateEvent.state.timeState));
  }
  else
  {
    /* Descriptor found on element */
    event.hdr.status = MMDL_SUCCESS;

    /* Set event parameters */
    event.currentStateEvent.state.timeState = pDesc->storedTimeState;
  }

  /* Send event to the upper layer */
  timeSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Set the Time Zone Offset New state of the element.
 *
 *  \param[in] elementId     Identifier of the Element implementing the model.
 *  \param[in] pTargetState  Target State for this transaction. See ::mmdlTimeZoneState_t.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSrZoneSetState(meshElementId_t elementId, mmdlTimeZoneState_t *pTargetState)
{
  /* Change state locally. No transition time or delay required. */
  mmdlTimeSrZoneSetState(elementId, pTargetState, MMDL_STATE_UPDATED_BY_APP);
}

/*************************************************************************************************/
/*!
 *  \brief     Get the Time Zone Offset Current state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSrZoneGetState(meshElementId_t elementId)
{
  mmdlTimeSrEvent_t event;
  mmdlTimeSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlTimeSrGetDesc(elementId, &pDesc);

  /* Set event type */
  event.hdr.event = MMDL_TIME_SR_EVENT;
  event.hdr.param = MMDL_TIMEZONE_SR_CURRENT_STATE_EVENT;

  /* Set event parameters */
  event.currentStateEvent.elemId = elementId;

  if (pDesc == NULL)
  {
    /* No descriptor found on element */
    event.hdr.status = MMDL_INVALID_ELEMENT;

    /* Zero out parameters */
    memset(&event.currentStateEvent.state.timeZoneState, 0,
           sizeof(event.currentStateEvent.state.timeZoneState));
  }
  else
  {
    /* Descriptor found on element */
    event.hdr.status = MMDL_SUCCESS;

    /* Set event parameters */
    event.currentStateEvent.state.timeZoneState = pDesc->storedTimeZoneState;
  }

  /* Send event to the upper layer */
  timeSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Set the TAI-UTC Delta New state of the element.
 *
 *  \param[in] elementId     Identifier of the Element implementing the model.
 *  \param[in] pTargetState  Target State for this transaction. See ::mmdlTimeDeltaState_t.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeDeltaSrSetState(meshElementId_t elementId, mmdlTimeDeltaState_t *pTargetState)
{
  /* Change state locally. No transition time or delay required. */
  mmdlTimeDeltaSrSetState(elementId, pTargetState, MMDL_STATE_UPDATED_BY_APP);
}

/*************************************************************************************************/
/*!
 *  \brief     Get the TAI-UTC Delta Current state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeDeltaSrGetState(meshElementId_t elementId)
{
  mmdlTimeSrEvent_t event;
  mmdlTimeSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlTimeSrGetDesc(elementId, &pDesc);

  /* Set event type */
  event.hdr.event = MMDL_TIME_SR_EVENT;
  event.hdr.param = MMDL_TIMEDELTA_SR_CURRENT_STATE_EVENT;

  /* Set event parameters */
  event.currentStateEvent.elemId = elementId;

  if (pDesc == NULL)
  {
    /* No descriptor found on element */
    event.hdr.status = MMDL_INVALID_ELEMENT;

    /* Zero out parameters */
    memset(&event.currentStateEvent.state.timeDeltaState, 0,
           sizeof(event.currentStateEvent.state.timeDeltaState));
  }
  else
  {
    /* Descriptor found on element */
    event.hdr.status = MMDL_SUCCESS;

    /* Set event parameters */
    event.currentStateEvent.state.timeDeltaState = pDesc->storedTimeDeltaState;
  }

  /* Send event to the upper layer */
  timeSrCb.recvCback((wsfMsgHdr_t *)&event);
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
void MmdlTimeSrRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback*/
  if (recvCback != NULL)
  {
    timeSrCb.recvCback = recvCback;
  }
}
