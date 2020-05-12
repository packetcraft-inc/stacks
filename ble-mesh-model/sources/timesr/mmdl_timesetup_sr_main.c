/*************************************************************************************************/
/*!
 *  \file   mmdl_timesetup_sr_main.c
 *
 *  \brief  Implementation of the Time Setup Server model.
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
#include "mmdl_bindings.h"

#include "mmdl_timesetup_sr_main.h"
#include "mmdl_timesetup_sr_api.h"
#include "mmdl_time_sr_api.h"
#include "mmdl_time_sr_main.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Time Setup Server control block type definition */
typedef struct mmdlTimeSetupSrCb_tag
{
  mmdlEventCback_t          recvCback;        /*!< Model Time Setup received callback */
}mmdlTimeSetupSrCb_t;

/*! Time Setup Server message handler type definition */
typedef void (*mmdlTimeSetupSrHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler ID */
wsfHandlerId_t mmdlTimeSetupSrHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlTimeSetupSrRcvdOpcodes[MMDL_TIME_SETUP_SR_NUM_RCVD_OPCODES] =
{
  { {UINT8_OPCODE_TO_BYTES(MMDL_TIME_SET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_TIMEZONE_SET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_TIMEDELTA_SET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_TIMEROLE_GET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_TIMEROLE_SET_OPCODE)} },
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlTimeSetupSrHandleMsg_t mmdlTimeSetupSrHandleMsg[MMDL_TIME_SETUP_SR_NUM_RCVD_OPCODES] =
{
  mmdlTimeSetupSrHandleSet,
  mmdlTimeSetupSrHandleZoneSet,
  mmdlTimeSetupSrHandleDeltaSet,
  mmdlTimeSetupSrHandleRoleGet,
  mmdlTimeSetupSrHandleRoleSet,
};

/*! Time Setup Server Control Block */
static mmdlTimeSetupSrCb_t  timeSetupSrCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

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
static void mmdlTimeSetupSrSetState(meshElementId_t elementId, mmdlTimeState_t *pTargetState,
                                    mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlTimeSrEvent_t event;
  mmdlTimeSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO3("TIME SETUP SR: Set taiSeconds=0x%x subsecond=0x%x uncertainty=0x%x", pTargetState->taiSeconds,
                   pTargetState->subSecond, pTargetState->uncertainty);
  MMDL_TRACE_INFO3("TIME SETUP SR: Set timeauthority=%d delta=0x%x timezoneoffset=0x%x", pTargetState->timeAuthority,
                   pTargetState->taiUtcDelta, pTargetState->timeZoneOffset);

  /* Get model instance descriptor */
  mmdlTimeSrGetDesc(elementId, &pDesc);


  /* Set event type */
  event.hdr.event = MMDL_TIME_SR_EVENT;
  event.hdr.param = MMDL_TIME_SR_STATE_UPDATE_EVENT;

  /* Set event parameters */
  event.statusEvent.elemId = elementId;
  memcpy(&event.statusEvent.state.timeState, pTargetState, sizeof(mmdlTimeState_t));
  event.statusEvent.stateUpdateSource = stateUpdateSrc;

  if (pDesc == NULL)
  {
    event.hdr.status = MMDL_INVALID_ELEMENT;
  }
  else
  {
    event.hdr.status = MMDL_SUCCESS;

    /* Update Target State */
    memcpy(&pDesc->storedTimeState, pTargetState, sizeof(mmdlTimeState_t));
  }

  /* Send event to the upper layer */
  timeSetupSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Time Setup Set command.
 *
 *  \param[in] pMsg         Received model message.
 *  \param[in] ackRequired  TRUE if acknowledgement is required in response,  FALSE otherwise.
 *
 *  \return    TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlTimeSetupSrProcessSet(const meshModelMsgRecvEvt_t *pMsg, bool_t ackRequired)
{
  mmdlTimeState_t state;
  mmdlTimeSrDesc_t *pDesc = NULL;
  uint8_t *pParams, byte7, byte8;

  (void)ackRequired;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length. It can take only min and max values. */
  if (pMsg->messageParamsLen != MMDL_TIME_SET_LENGTH)
  {
    return FALSE;
  }

  pParams = pMsg->pMessageParams;
  BSTREAM_TO_UINT40(state.taiSeconds, pParams)
  BSTREAM_TO_UINT8(state.subSecond, pParams)
  BSTREAM_TO_UINT8(state.uncertainty, pParams)
  BSTREAM_TO_UINT8(byte7, pParams)
  BSTREAM_TO_UINT8(byte8, pParams)
  state.timeAuthority = byte7 & 0x01;
  state.taiUtcDelta = ((uint16_t)byte7 >> 1) + ((uint16_t)byte8 << 7);
  BSTREAM_TO_UINT8(state.timeZoneOffset, pParams);

  /* Get the model instance descriptor */
  mmdlTimeSrGetDesc(pMsg->elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Change state */
    mmdlTimeSetupSrSetState(pMsg->elementId, &state, MMDL_STATE_UPDATED_BY_CL);

    return TRUE;
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
static void mmdlTimeSetupSrZoneSetState(meshElementId_t elementId,
                                        mmdlTimeZoneState_t *pTargetState,
                                        mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlTimeSrEvent_t event;
  mmdlTimeSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO2("TIME SETUP SR: Set offsetnew=0x%x taizonechange=0x%x",
                    pTargetState->offsetNew, pTargetState->taiZoneChange);

  /* Get model instance descriptor */
  mmdlTimeSrGetDesc(elementId, &pDesc);

  /* Set event type */
  event.hdr.event = MMDL_TIME_SR_EVENT;
  event.hdr.param = MMDL_TIMEZONE_SR_STATE_UPDATE_EVENT;

  /* Set event parameters */
  event.statusEvent.elemId = elementId;
  event.statusEvent.state.timeZoneState.offsetNew = pTargetState->offsetNew;
  event.statusEvent.state.timeZoneState.taiZoneChange = pTargetState->taiZoneChange;
  event.statusEvent.stateUpdateSource = stateUpdateSrc;

  if (pDesc == NULL)
  {
    event.hdr.status = MMDL_INVALID_ELEMENT;
  }
  else
  {
    event.hdr.status = MMDL_SUCCESS;

    /* Update Target State */
    pDesc->storedTimeZoneState.offsetNew = pTargetState->offsetNew;
    pDesc->storedTimeZoneState.taiZoneChange = pTargetState->taiZoneChange;
  }

  /* Send event to the upper layer */
  timeSetupSrCb.recvCback((wsfMsgHdr_t *)&event);
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
static void mmdlTimeSetupSrSendZoneStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                     uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_TIMESETUP_SR_MDL_ID, MMDL_TIMEZONE_STATUS_OPCODE);
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

    MMDL_TRACE_INFO3("TIME SETUP ZONE SR: Send Status current=0x%x new=0x%x change=0x%x",
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
 *  \brief     Handles a Time Setup Zone Set command.
 *
 *  \param[in] pMsg         Received model message.
 *  \param[in] ackRequired  TRUE if acknowledgement is required in response,  FALSE otherwise.
 *
 *  \return    TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlTimeSetupSrProcessZoneSet(const meshModelMsgRecvEvt_t *pMsg, bool_t ackRequired)
{
  mmdlTimeZoneState_t state;
  mmdlTimeSrDesc_t *pDesc = NULL;
  uint8_t *pParams;

  (void)ackRequired;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length. It can take only min and max values. */
  if (pMsg->messageParamsLen != MMDL_TIMEZONE_SET_LENGTH)
  {
    return FALSE;
  }

  pParams = pMsg->pMessageParams;
  BSTREAM_TO_UINT8(state.offsetNew, pParams);
  BSTREAM_TO_UINT40(state.taiZoneChange, pParams);

  /* Get the model instance descriptor */
  mmdlTimeSrGetDesc(pMsg->elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Change state */
    mmdlTimeSetupSrZoneSetState(pMsg->elementId, &state, MMDL_STATE_UPDATED_BY_CL);
    return TRUE;
  }

  return FALSE;
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
static void mmdlTimeSetupSrDeltaSetState(meshElementId_t elementId,
                                         mmdlTimeDeltaState_t *pTargetState,
                                         mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlTimeSrEvent_t event;
  mmdlTimeSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO2("TIME SETUP DELTA SR: Set new=0x%X, change0x%X,",
                    pTargetState->deltaNew, pTargetState->deltaChange);

  /* Get model instance descriptor */
  mmdlTimeSrGetDesc(elementId, &pDesc);

  /* Set event type */
  event.hdr.event = MMDL_TIME_SR_EVENT;
  event.hdr.param = MMDL_TIMEDELTA_SR_STATE_UPDATE_EVENT;

  /* Set event parameters */
  event.statusEvent.elemId = elementId;
  event.statusEvent.state.timeDeltaState.deltaChange = pTargetState->deltaChange;
  event.statusEvent.state.timeDeltaState.deltaNew = pTargetState->deltaNew;
  event.statusEvent.stateUpdateSource = stateUpdateSrc;

  if (pDesc == NULL)
  {
    event.hdr.status = MMDL_INVALID_ELEMENT;
  }
  else
  {
    event.hdr.status = MMDL_SUCCESS;

    /* Update Target State */
    pDesc->storedTimeDeltaState.deltaChange = pTargetState->deltaChange;
    pDesc->storedTimeDeltaState.deltaNew = pTargetState->deltaNew;
  }

  /* Send event to the upper layer */
  timeSetupSrCb.recvCback((wsfMsgHdr_t *)&event);
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
static void mmdlTimeSetupSrSendDeltaStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                      uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_TIMESETUP_SR_MDL_ID, MMDL_TIMEDELTA_STATUS_OPCODE);
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

    MMDL_TRACE_INFO3("TIME SETUP DELTA SR: Send Status current=0x%X new=0x%X change=0x%X",
                     pDesc->storedTimeState.taiUtcDelta,
                     pDesc->storedTimeDeltaState.deltaNew,
                     pDesc->storedTimeDeltaState.deltaNew);

    /* Send message to the Mesh Core */
    MeshSendMessage(&msgInfo, msgBuffer, MMDL_TIMEDELTA_STATUS_LENGTH,
                    MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                    MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Time Setup Delta Set command.
 *
 *  \param[in] pMsg         Received model message.
 *  \param[in] ackRequired  TRUE if acknowledgement is required in response,  FALSE otherwise.
 *
 *  \return    TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlTimeSetupSrProcessDeltaSet(const meshModelMsgRecvEvt_t *pMsg, bool_t ackRequired)
{
  mmdlTimeDeltaState_t state;
  mmdlTimeSrDesc_t *pDesc = NULL;
  uint8_t *pParams;

  (void)ackRequired;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length. */
  if (pMsg->messageParamsLen != MMDL_TIMEDELTA_SET_LENGTH)
  {
    return FALSE;
  }

  pParams = pMsg->pMessageParams;
  BSTREAM_TO_UINT16(state.deltaNew, pParams);
  BSTREAM_TO_UINT40(state.deltaChange, pParams);

  /* Get the model instance descriptor */
  mmdlTimeSrGetDesc(pMsg->elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Change state */
    mmdlTimeSetupSrDeltaSetState(pMsg->elementId, &state, MMDL_STATE_UPDATED_BY_CL);

    return TRUE;
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model.
 *  \param[in] pTargetState    Target State for this transaction. See ::mmdlTimeRoleState_t.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlTimeSetupSrRoleSetState(meshElementId_t elementId,
                                        mmdlTimeRoleState_t *pTargetState,
                                        mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlTimeSrEvent_t event;
  mmdlTimeSrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO1("TIME SETUP ROLE SR: Set role=%d", pTargetState->timeRole);

  /* Get model instance descriptor */
  mmdlTimeSrGetDesc(elementId, &pDesc);

  /* Set event type */
  event.hdr.event = MMDL_TIME_SR_EVENT;
  event.hdr.param = MMDL_TIMEROLE_SR_STATE_UPDATE_EVENT;

  /* Set event parameters */
  event.statusEvent.elemId = elementId;
  event.statusEvent.state.timeRoleState.timeRole = pTargetState->timeRole;
  event.statusEvent.stateUpdateSource = stateUpdateSrc;


  if (pDesc == NULL)
  {
    event.hdr.status = MMDL_INVALID_ELEMENT;
  }
  else
  {
    event.hdr.status = MMDL_SUCCESS;

    /* Update Target State */
    pDesc->storedTimeRoleState.timeRole = pTargetState->timeRole;
  }

  /* Send event to the upper layer */
  timeSetupSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Time Setup Role Status command to the specified destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  Indicates if message that triggered the status was received on unicast.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlTimeSetupSrSendRoleStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                          uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_TIMESETUP_SR_MDL_ID, MMDL_TIMEROLE_STATUS_OPCODE);
  mmdlTimeSrDesc_t *pDesc = NULL;
  uint8_t msgBuffer[MMDL_TIMEROLE_STATUS_LENGTH];

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
    msgBuffer[0] = pDesc->storedTimeRoleState.timeRole;

    /* It is recommend  to transmit a Time Status message when the
     * Time Role state has been changed to Mesh Time Authority (0x01)
     */
    if (pDesc->storedTimeRoleState.timeRole == MMDL_TIME_ROLE_STATE_AUTHORITY)
    {
      mmdlTimeSrSendStatus(elementId, dstAddr, appKeyIndex, recvOnUnicast);
    }

    /* Send message to the Mesh Core */
    MeshSendMessage(&msgInfo, msgBuffer, MMDL_TIMEROLE_STATUS_LENGTH,
                    MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                    MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Time Setup Role Set command.
 *
 *  \param[in] pMsg         Received model message.
 *  \param[in] ackRequired  TRUE if acknowledgement is required in response,  FALSE otherwise.
 *
 *  \return    TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlTimeSetupSrProcessRoleSet(const meshModelMsgRecvEvt_t *pMsg, bool_t ackRequired)
{
  mmdlTimeRoleState_t state;
  mmdlTimeSrDesc_t *pDesc = NULL;

  (void)ackRequired;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length. It can take only min and max values. */
  if (pMsg->messageParamsLen != MMDL_TIMEROLE_SET_LENGTH)
  {
    return FALSE;
  }

  /* Check prohibited values for Role State */
  if (pMsg->pMessageParams[0] >= MMDL_TIME_ROLE_STATE_PROHIBITED)
  {
    return FALSE;
  }

  state.timeRole = pMsg->pMessageParams[0];

  /* Get the model instance descriptor */
  mmdlTimeSrGetDesc(pMsg->elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Change state */
    mmdlTimeSetupSrRoleSetState(pMsg->elementId, &state, MMDL_STATE_UPDATED_BY_CL);

    return TRUE;
  }
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Time Setup Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlTimeSetupSrHandleSet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  if (mmdlTimeSetupSrProcessSet(pMsg, TRUE))
  {
    /* Send Status message as a response to the Set message */
    mmdlTimeSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Time Setup Zone Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlTimeSetupSrHandleZoneSet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  if (mmdlTimeSetupSrProcessZoneSet(pMsg, TRUE))
  {
    /* Send Status message as a response to the Set message */
    mmdlTimeSetupSrSendZoneStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Time Setup TAI-UTC Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlTimeSetupSrHandleDeltaSet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  if (mmdlTimeSetupSrProcessDeltaSet(pMsg, TRUE))
  {
    /* Send Status message as a response to the Set message */
    mmdlTimeSetupSrSendDeltaStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Time Setup Role Level Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlTimeSetupSrHandleRoleGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    mmdlTimeSetupSrSendRoleStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Time Setup Role Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlTimeSetupSrHandleRoleSet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  if (mmdlTimeSetupSrProcessRoleSet(pMsg, TRUE))
  {
    /* Send Status message as a response to the Set message */
    mmdlTimeSetupSrSendRoleStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Time Setup Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlTimeSetupSrInit(void)
{
  MMDL_TRACE_INFO0("TIME SETUP SR: init");

  /* Set event callbacks */
  timeSetupSrCb.recvCback = MmdlEmptyCback;
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Time Setup0 Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Time Setup Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSetupSrHandlerInit(wsfHandlerId_t handlerId)
{
  mmdlTimeSetupSrHandlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Time Setup Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSetupSrHandler(wsfMsgHdr_t *pMsg)
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
        for (opcodeIdx = 0; opcodeIdx < MMDL_TIME_SETUP_SR_NUM_RCVD_OPCODES; opcodeIdx++)
        {
          opcodeSize = MESH_OPCODE_SIZE(pModelMsg->msgRecvEvt.opCode);
          if (!memcmp(&mmdlTimeSetupSrRcvdOpcodes[opcodeIdx],
                     pModelMsg->msgRecvEvt.opCode.opcodeBytes, opcodeSize))
          {
            /* Process message */
            (void)mmdlTimeSetupSrHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
          }
        }
        break;

      default:
        MMDL_TRACE_WARN0("TIME SETUP SR: Invalid event message received!");
        break;
    }
  }
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
void MmdlTimeSetupSrGetState(meshElementId_t elementId)
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
  timeSetupSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Set the Time state of the element.
 *
 *  \param[in] elementId     Identifier of the Element implementing the model.
 *  \param[in] pTargetState  Target State for this transaction. See ::mmdlGenLevelState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSetupSrSetState(meshElementId_t elementId, mmdlTimeState_t *pTargetState)
{
  /* Change state locally. No transition time or delay required. */
  mmdlTimeSetupSrSetState(elementId, pTargetState, MMDL_STATE_UPDATED_BY_APP);
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
void MmdlTimeSetupSrZoneGetState(meshElementId_t elementId)
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

    /* Zero out parameter */
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
  timeSetupSrCb.recvCback((wsfMsgHdr_t *)&event);
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
void MmdlTimeSetupSrZoneSetState(meshElementId_t elementId, mmdlTimeZoneState_t *pTargetState)
{
  /* Change state locally. No transition time or delay required. */
  mmdlTimeSetupSrZoneSetState(elementId, pTargetState, MMDL_STATE_UPDATED_BY_APP);
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
void MmdlTimeSetupSrDeltaGetState(meshElementId_t elementId)
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
  timeSetupSrCb.recvCback((wsfMsgHdr_t *)&event);
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
void MmdlTimeSetupSrDeltaSetState(meshElementId_t elementId, mmdlTimeDeltaState_t *pTargetState)
{
  /* Change state locally. No transition time or delay required. */
  mmdlTimeSetupSrDeltaSetState(elementId, pTargetState, MMDL_STATE_UPDATED_BY_APP);
}

/*************************************************************************************************/
/*!
 *  \brief     Get the Time Role state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSetupSrRoleGetState(meshElementId_t elementId)
{
  mmdlTimeSrEvent_t event;
  mmdlTimeSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlTimeSrGetDesc(elementId, &pDesc);

  /* Set event type */
  event.hdr.event = MMDL_TIME_SR_EVENT;
  event.hdr.param = MMDL_TIMEROLE_SR_CURRENT_STATE_EVENT;

  /* Set event parameters */
  event.currentStateEvent.elemId = elementId;

  if (pDesc == NULL)
  {
    /* No descriptor found on element */
    event.hdr.status = MMDL_INVALID_ELEMENT;

    /* Zero out parameters */
    memset(&event.currentStateEvent.state.timeRoleState, 0,
           sizeof(event.currentStateEvent.state.timeRoleState));
  }
  else
  {
    /* Descriptor found on element */
    event.hdr.status = MMDL_SUCCESS;

    /* Set event parameters */
    event.currentStateEvent.state.timeRoleState = pDesc->storedTimeRoleState;
  }

  /* Send event to the upper layer */
  timeSetupSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Set the Time Role state of the element.
 *
 *  \param[in] elementId     Identifier of the Element implementing the model.
 *  \param[in] pTargetState  Target State for this transaction. See ::mmdlTimeRoleState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeSetupSrRoleSetState(meshElementId_t elementId, mmdlTimeRoleState_t *pTargetState)
{
  /* Change state locally. No transition time or delay required. */
  mmdlTimeSetupSrRoleSetState(elementId, pTargetState, MMDL_STATE_UPDATED_BY_APP);
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
void MmdlTimeSetupSrRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback*/
  if (recvCback != NULL)
  {
    timeSetupSrCb.recvCback = recvCback;
  }
}
