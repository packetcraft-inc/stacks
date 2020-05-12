/*************************************************************************************************/
/*!
 *  \file   mmdl_time_cl_main.c
 *
 *  \brief  Implementation of the Time Client model.
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
#include "wsf_assert.h"
#include "wsf_os.h"
#include "wsf_trace.h"
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"

#include "mmdl_types.h"
#include "mmdl_common.h"
#include "mmdl_time_cl_api.h"
#include "mmdl_time_cl_main.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Time control block type definition */
typedef struct mmdlTImeClCb_tag
{
  mmdlEventCback_t recvCback;    /*!< Model Time received callback */
}mmdlTimeClCb_t;

/*! Time Client message handler type definition */
typedef void (*mmdlTimeClHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler id */
wsfHandlerId_t mmdlTimeClHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlTimeClRcvdOpcodes[MMDL_TIME_CL_NUM_RCVD_OPCODES] =
{
  { {UINT8_OPCODE_TO_BYTES(MMDL_TIME_STATUS_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_TIMEZONE_STATUS_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_TIMEDELTA_STATUS_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_TIMEROLE_STATUS_OPCODE)} }
};


/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Time Client control block */
static mmdlTimeClCb_t  timeClCb;

/*! Handler functions for supported opcodes */
const mmdlTimeClHandleMsg_t mmdlTimeClHandleMsg[MMDL_TIME_CL_NUM_RCVD_OPCODES] =
{
  mmdlTimeClHandleStatus,
  mmdlTimeClHandleZoneStatus,
  mmdlTimeClHandleDeltaStatus,
  mmdlTimeClHandleRoleStatus
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Sends a Time message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlTimeSendSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            const mmdlTimeSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_TIME_CL_MDL_ID, 0);
  uint8_t *pParam;
  uint8_t paramMsg[MMDL_TIME_SET_LENGTH];

  if (pSetParam != NULL)
  {
    /* Fill in the message information */
    msgInfo.elementId = elementId;
    msgInfo.dstAddr = serverAddr;
    msgInfo.ttl = ttl;
    msgInfo.appKeyIndex = appKeyIndex;

    msgInfo.opcode.opcodeBytes[0] = MMDL_TIME_SET_OPCODE;

    /* Build param message. */
    pParam = paramMsg;
    UINT40_TO_BSTREAM(pParam, pSetParam->state.taiSeconds);
    UINT8_TO_BSTREAM(pParam, pSetParam->state.subSecond);
    UINT8_TO_BSTREAM(pParam, pSetParam->state.uncertainty);
    UINT8_TO_BSTREAM(pParam, ((pSetParam->state.taiUtcDelta & 0x7F) << 1) | pSetParam->state.timeAuthority);
    UINT8_TO_BSTREAM(pParam, (uint8_t)(pSetParam->state.taiUtcDelta >> 7));
    UINT8_TO_BSTREAM(pParam, pSetParam->state.timeZoneOffset);

    /* Send message to the Mesh Core */
    MeshSendMessage(&msgInfo, paramMsg, MMDL_TIME_SET_LENGTH, 0, 0);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Time Zone message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlTimeSendZoneSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                const mmdlTimeZoneSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_TIME_CL_MDL_ID, MMDL_TIMEZONE_SET_OPCODE);
  uint8_t *pParam;
  uint8_t paramMsg[MMDL_TIMEZONE_SET_LENGTH];

  if (pSetParam != NULL)
  {
    /* Fill in the message information */
    msgInfo.elementId = elementId;
    msgInfo.dstAddr = serverAddr;
    msgInfo.ttl = ttl;
    msgInfo.appKeyIndex = appKeyIndex;

    /* Build param message. */
    pParam = paramMsg;
    UINT8_TO_BSTREAM(pParam, pSetParam->state.offsetNew);
    UINT40_TO_BSTREAM(pParam, pSetParam->state.taiZoneChange);

    /* Send message to the Mesh Core */
    MeshSendMessage(&msgInfo, paramMsg, MMDL_TIMEZONE_SET_LENGTH, 0, 0);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Time Delta message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlTimeSendDeltaSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                 const mmdlTimeDeltaSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_TIME_CL_MDL_ID, MMDL_TIMEDELTA_SET_OPCODE);
  uint8_t *pParams;
  uint8_t paramMsg[MMDL_TIMEDELTA_SET_LENGTH];

  if (pSetParam != NULL)
  {
    /* Fill in the message information */
    msgInfo.elementId = elementId;
    msgInfo.dstAddr = serverAddr;
    msgInfo.ttl = ttl;
    msgInfo.appKeyIndex = appKeyIndex;

    /* Build param message. */
    pParams = paramMsg;
    UINT16_TO_BSTREAM(pParams, pSetParam->state.deltaNew);
    UINT40_TO_BSTREAM(pParams, pSetParam->state.deltaChange);

    /* Send message to the Mesh Core */
    MeshSendMessage(&msgInfo, paramMsg, MMDL_TIMEDELTA_SET_LENGTH, 0, 0);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Time Role message to the destination address.
 *
 *  \param[in] opcode       Opcode.
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlTimeSendRoleSet(uint16_t opcode, meshElementId_t elementId,
                                meshAddress_t serverAddr, uint8_t ttl,
                                const mmdlTimeRoleSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_TIME_CL_MDL_ID, MMDL_TIMEROLE_SET_OPCODE);
  uint8_t paramMsg[MMDL_TIMEROLE_SET_LENGTH];

  if (pSetParam != NULL)
  {

    /* Fill in the message information */
    msgInfo.elementId = elementId;
    msgInfo.dstAddr = serverAddr;
    msgInfo.ttl = ttl;
    msgInfo.appKeyIndex = appKeyIndex;
    UINT16_TO_BE_BUF(msgInfo.opcode.opcodeBytes, opcode);

    /* Build param message. */
    paramMsg[0] = pSetParam->state.timeRole;

    /* Send message to the Mesh Core */
    MeshSendMessage(&msgInfo, paramMsg, MMDL_TIMEROLE_SET_LENGTH, 0, 0);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Time Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlTimeClHandleStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlTimeClStatusEvent_t event;
  uint8_t *pParams, byte7, byte8;

  /* Validate message length */
  if ((pMsg->messageParamsLen != MMDL_TIME_STATUS_MAX_LENGTH) &&
      (pMsg->messageParamsLen != MMDL_TIME_STATUS_MIN_LENGTH))
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_TIME_CL_EVENT;
  event.hdr.param = MMDL_TIME_CL_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  pParams = pMsg->pMessageParams;
  event.state.taiSeconds = 0;
  BSTREAM_TO_UINT40(event.state.taiSeconds, pParams);

  if (event.state.taiSeconds != 0)
  {
    BSTREAM_TO_UINT8(event.state.subSecond, pParams);
    BSTREAM_TO_UINT8(event.state.uncertainty, pParams);
    BSTREAM_TO_UINT8(byte7, pParams);
    BSTREAM_TO_UINT8(byte8, pParams);
    BSTREAM_TO_UINT8(event.state.timeZoneOffset, pParams);
    event.state.timeAuthority = byte7 & 0x01;
    event.state.taiUtcDelta = ((uint16_t)byte7 >> 1) + ((uint16_t)byte8 << 7);
  }

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  timeClCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Time Zone Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlTimeClHandleZoneStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlTimeClZoneStatusEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_TIMEZONE_STATUS_LENGTH)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_TIME_CL_EVENT;
  event.hdr.param = MMDL_TIMEZONE_CL_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  pParams = pMsg->pMessageParams;
  BSTREAM_TO_UINT8(event.offsetCurrent, pParams);
  BSTREAM_TO_UINT8(event.offsetNew, pParams);
  BSTREAM_TO_UINT40(event.taiZoneChange, pParams);

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  timeClCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Time TAI-UTC Delta Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlTimeClHandleDeltaStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlTimeClDeltaStatusEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_TIMEDELTA_STATUS_LENGTH)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_TIME_CL_EVENT;
  event.hdr.param = MMDL_TIMEDELTA_CL_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  pParams = pMsg->pMessageParams;

  BSTREAM_TO_UINT16(event.deltaCurrent, pParams);
  BSTREAM_TO_UINT16(event.deltaNew, pParams);
  BSTREAM_TO_UINT40(event.deltaChange, pParams);

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  timeClCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Time Role Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlTimeClHandleRoleStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlTimeClRoleStatusEvent_t event;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_TIMEROLE_STATUS_LENGTH)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_TIME_CL_EVENT;
  event.hdr.param = MMDL_TIMEROLE_CL_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  event.timeRole = pMsg->pMessageParams[0];

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  timeClCb.recvCback((wsfMsgHdr_t *)&event);
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Level Client Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeClHandlerInit(wsfHandlerId_t handlerId)
{
  /* Set handler ID */
  mmdlTimeClHandlerId = handlerId;

  /* Initialize control block */
  timeClCb.recvCback = MmdlEmptyCback;
}

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Time Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeClHandler(wsfMsgHdr_t *pMsg)
{
  meshModelMsgRecvEvt_t *pModelMsg;
  uint8_t opcodeIdx, opcodeSize;

  /* Handle message */
  if (pMsg != NULL)
  {
    switch (pMsg->event)
    {
      case MESH_MODEL_EVT_MSG_RECV:
        pModelMsg = (meshModelMsgRecvEvt_t *)pMsg;

        /* Match the received opcode */
        for (opcodeIdx = 0; opcodeIdx < MMDL_TIME_CL_NUM_RCVD_OPCODES; opcodeIdx++)
        {
          opcodeSize = MESH_OPCODE_SIZE(pModelMsg->opCode);
          if (!memcmp(&mmdlTimeClRcvdOpcodes[opcodeIdx], pModelMsg->opCode.opcodeBytes,
              opcodeSize))
          {
            /* Process message */
            (void)mmdlTimeClHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
          }
        }
        break;

      default:
        MMDL_TRACE_WARN0("TIME CL: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Time Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                   uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_TIME_CL_MDL_ID, MMDL_TIME_GET_OPCODE);

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = serverAddr;
  msgInfo.ttl = ttl;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Send message to the Mesh Core instantly */
  MeshSendMessage(&msgInfo, NULL, 0, 0, 0);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Time Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                       const mmdlTimeSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  mmdlTimeSendSet(elementId, serverAddr, ttl, pSetParam, appKeyIndex);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Time Zone Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeClZoneGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                       uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_TIME_CL_MDL_ID, MMDL_TIMEZONE_GET_OPCODE);

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = serverAddr;
  msgInfo.ttl = ttl;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Send message to the Mesh Core instantly */
  MeshSendMessage(&msgInfo, NULL, 0, 0, 0);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Time Zone Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeClZoneSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                       const mmdlTimeZoneSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  mmdlTimeSendZoneSet(elementId, serverAddr, ttl, pSetParam, appKeyIndex);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Time Delta Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeClDeltaGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                        uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_TIME_CL_MDL_ID, MMDL_TIMEDELTA_GET_OPCODE);

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = serverAddr;
  msgInfo.ttl = ttl;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Send message to the Mesh Core instantly */
  MeshSendMessage(&msgInfo, NULL, 0, 0, 0);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Time Delta Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeClDeltaSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                       const mmdlTimeDeltaSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  mmdlTimeSendDeltaSet(elementId, serverAddr, ttl, pSetParam, appKeyIndex);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Time Role Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeClRoleGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                       uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_TIME_CL_MDL_ID, MMDL_TIMEROLE_GET_OPCODE);

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = serverAddr;
  msgInfo.ttl = ttl;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Send message to the Mesh Core instantly */
  MeshSendMessage(&msgInfo, NULL, 0, 0, 0);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Time Role Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeClRoleSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                       const mmdlTimeRoleSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  mmdlTimeSendRoleSet(MMDL_TIMEROLE_SET_OPCODE, elementId, serverAddr, ttl, pSetParam,
                      appKeyIndex);
}

/*************************************************************************************************/
/*!
 *  \brief     Install the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlTimeClRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback*/
  if (recvCback != NULL)
  {
    timeClCb.recvCback = recvCback;
  }
}
