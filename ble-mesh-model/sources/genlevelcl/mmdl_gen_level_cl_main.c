/*************************************************************************************************/
/*!
 *  \file   mmdl_gen_level_cl_main.c
 *
 *  \brief  Implementation of the Generic Level Client model.
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
#include "wsf_os.h"
#include "wsf_trace.h"
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"

#include "mmdl_types.h"
#include "mmdl_common.h"
#include "mmdl_gen_level_cl_api.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Generic Level control block type definition */
typedef struct mmdlGenLevelClCb_tag
{
  mmdlEventCback_t recvCback;    /*!< Model Generic Level received callback */
} mmdlGenLevelClCb_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler id */
wsfHandlerId_t mmdlGenLevelClHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlGenLevelClRcvdOpcodes[] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_LEVEL_STATUS_OPCODE)} }
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Level Client control block */
static mmdlGenLevelClCb_t  levelClCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Sends a GenLevel message to the destination address.
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
static void mmdlGenLevelSendSet(uint16_t opcode, meshElementId_t elementId,
                               meshAddress_t serverAddr, uint8_t ttl,
                               const mmdlGenLevelSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_LEVEL_CL_MDL_ID, 0);
  uint8_t *pParams;
  uint8_t paramMsg[MMDL_GEN_LEVEL_SET_MAX_LEN];

  if (pSetParam != NULL)
  {

    /* Fill in the message information */
    msgInfo.elementId = elementId;
    msgInfo.dstAddr = serverAddr;
    msgInfo.ttl = ttl;
    msgInfo.appKeyIndex = appKeyIndex;
    UINT16_TO_BE_BUF(msgInfo.opcode.opcodeBytes, opcode);

    /* Build param message. */
    pParams = paramMsg;
    UINT16_TO_BSTREAM(pParams, pSetParam->state);
    UINT8_TO_BSTREAM(pParams,pSetParam->tid);

    /* Do not include transition time and delay in the message if it is not used */
    if (pSetParam->transitionTime != MMDL_GEN_TR_UNKNOWN)
    {
      UINT8_TO_BSTREAM(pParams, pSetParam->transitionTime);
      UINT8_TO_BSTREAM(pParams, pSetParam->delay);
    }

    /* Send message to the Mesh Core */
    MeshSendMessage(&msgInfo, paramMsg, (uint8_t)(pParams - paramMsg), 0, 0);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a GenLevel Delta message to the destination address.
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
static void mmdlGenLevelSendDeltaSet(uint16_t opcode, meshElementId_t elementId,
                                     meshAddress_t serverAddr, uint8_t ttl,
                                     const mmdlGenDeltaSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_LEVEL_CL_MDL_ID, MMDL_GEN_LEVEL_SET_NO_ACK_OPCODE);
  uint8_t paramMsg[MMDL_GEN_LEVEL_DELTA_SET_MAX_LEN];
  uint8_t *pParams;

  if (pSetParam != NULL)
  {
    /* Fill in the message information */
    msgInfo.elementId = elementId;
    msgInfo.dstAddr = serverAddr;
    msgInfo.ttl = ttl;
    msgInfo.appKeyIndex = appKeyIndex;
    UINT16_TO_BE_BUF(msgInfo.opcode.opcodeBytes, opcode);

    /* Build param message. */
    pParams = paramMsg;
    UINT32_TO_BSTREAM(pParams, pSetParam->delta);
    UINT8_TO_BSTREAM(pParams, pSetParam->tid);

    /* Do not include transition time and delay in the message if it is not used */
    if (pSetParam->transitionTime != MMDL_GEN_TR_UNKNOWN)
    {
      UINT8_TO_BSTREAM(pParams, pSetParam->transitionTime);
      UINT8_TO_BSTREAM(pParams, pSetParam->delay);
    }

    /* Send message to the Mesh Core */
    MeshSendMessage(&msgInfo, paramMsg, (uint8_t)(pParams - paramMsg), 0, 0);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publishes a Generic Level message to the publication address.
 *
 *  \param[in] opcode     Opcode.
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] pSetParam  Pointer to structure containing the set parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenLevelPublishSet(uint16_t opcode, meshElementId_t elementId,
                                   const mmdlGenLevelSetParam_t *pSetParam)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_GEN_LEVEL_CL_MDL_ID,
                                             MMDL_GEN_LEVEL_SET_NO_ACK_OPCODE);
  uint8_t *pParams;
  uint8_t paramMsg[MMDL_GEN_LEVEL_SET_MAX_LEN];

  /* Fill in the msg info parameters */
  pubMsgInfo.elementId = elementId;
  UINT16_TO_BE_BUF(pubMsgInfo.opcode.opcodeBytes, opcode);

  /* Build param message. */
  pParams = paramMsg;
  UINT16_TO_BSTREAM(pParams, pSetParam->state);
  UINT8_TO_BSTREAM(pParams, pSetParam->tid);

  /* Do not include transition time and delay in the message if it is not used */
  if (pSetParam->transitionTime != MMDL_GEN_TR_UNKNOWN)
  {
    UINT8_TO_BSTREAM(pParams, pSetParam->transitionTime);
    UINT8_TO_BSTREAM(pParams, pSetParam->delay);
  }

  /* Send message to the Mesh Core. Parameters are already stored in over-the-air order */
  MeshPublishMessage(&pubMsgInfo, paramMsg, (uint8_t)(pParams - paramMsg));
}

/*************************************************************************************************/
/*!
 *  \brief     Publishes a Generic Level Delta message to the publication address.
 *
 *  \param[in] opcode     Opcode.
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] pSetParam  Pointer to structure containing the set parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenLevelPublishDeltaSet(uint16_t opcode, meshElementId_t elementId,
                                        const mmdlGenDeltaSetParam_t *pSetParam)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_GEN_LEVEL_CL_MDL_ID,
                                             MMDL_GEN_LEVEL_DELTA_SET_NO_ACK_OPCODE);
  uint8_t paramMsg[MMDL_GEN_LEVEL_DELTA_SET_MAX_LEN];
  uint8_t *pParams;

  /* Fill in the msg info parameters */
  pubMsgInfo.elementId = elementId;
  UINT16_TO_BE_BUF(pubMsgInfo.opcode.opcodeBytes, opcode);

  /* Build param message. */
  pParams = paramMsg;
  UINT32_TO_BE_BSTREAM(pParams, pSetParam->delta);
  UINT8_TO_BSTREAM(pParams, pSetParam->tid);

  /* Do not include transition time and delay in the message if it is not used */
  if (pSetParam->transitionTime != MMDL_GEN_TR_UNKNOWN)
  {
    UINT8_TO_BSTREAM(pParams, pSetParam->transitionTime);
    UINT8_TO_BSTREAM(pParams, pSetParam->delay);
  }

  /* Send message to the Mesh Core. Parameters are already stored in over-the-air order */
  MeshPublishMessage(&pubMsgInfo, paramMsg, (uint8_t)(pParams - paramMsg));
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Level Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenLevelClHandleStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlGenLevelClStatusEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_GEN_LEVEL_STATUS_MAX_LEN &&
      pMsg->messageParamsLen != MMDL_GEN_LEVEL_STATUS_MIN_LEN)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_GEN_LEVEL_CL_EVENT;
  event.hdr.param = MMDL_GEN_LEVEL_CL_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  pParams = pMsg->pMessageParams;

  /* Extract status event parameters */
  BSTREAM_TO_UINT16(event.state, pParams);

  /* Check if optional parameters are present */
  if (pMsg->messageParamsLen == MMDL_GEN_LEVEL_STATUS_MAX_LEN)
  {
    /* Extract target state and check value */
    BSTREAM_TO_UINT16(event.targetState, pParams);

    /* Extract target state */
    BSTREAM_TO_UINT8(event.remainingTime, pParams);
  }
  else
  {
    event.targetState = 0;
    event.remainingTime = 0;
  }

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  levelClCb.recvCback((wsfMsgHdr_t *)&event);
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
void MmdlGenLevelClHandlerInit(wsfHandlerId_t handlerId)
{
  /* Set handler ID */
  mmdlGenLevelClHandlerId = handlerId;

  /* Initialize control block */
  levelClCb.recvCback = MmdlEmptyCback;
}

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Generic Level Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelClHandler(wsfMsgHdr_t *pMsg)
{
  meshModelMsgRecvEvt_t *pModelMsg;

  /* Handle message */
  if (pMsg != NULL)
  {
    switch (pMsg->event)
    {
      case MESH_MODEL_EVT_MSG_RECV:
        pModelMsg = (meshModelMsgRecvEvt_t *)pMsg;

        /* Validate opcode size and value */
        if (MESH_OPCODE_SIZE(pModelMsg->opCode) == MMDL_GEN_LEVEL_OPCODES_SIZE &&
            !memcmp(&mmdlGenLevelClRcvdOpcodes[0], pModelMsg->opCode.opcodeBytes,
                    MMDL_GEN_LEVEL_OPCODES_SIZE))
        {
          /* Process Status message */
          mmdlGenLevelClHandleStatus(pModelMsg);
        }
        break;

      default:
        MMDL_TRACE_WARN0("GEN LEVEL CL: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a GenLevelGet message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                       uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_LEVEL_CL_MDL_ID, MMDL_GEN_LEVEL_GET_OPCODE);
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_GEN_LEVEL_CL_MDL_ID, MMDL_GEN_LEVEL_GET_OPCODE);

  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    /* Fill in the msg info parameters */
    msgInfo.elementId = elementId;
    msgInfo.dstAddr = serverAddr;
    msgInfo.ttl = ttl;
    msgInfo.appKeyIndex = appKeyIndex;

    /* Send message to the Mesh Core instantly */
    MeshSendMessage(&msgInfo, NULL, 0, 0, 0);
  }
  else
  {
    /* Fill in the msg info parameters */
    pubMsgInfo.elementId = elementId;

    /* Send message to the Mesh Core */
    MeshPublishMessage(&pubMsgInfo, NULL, 0);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a GenLevelSet message to the destination address.
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
void MmdlGenLevelClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                       const mmdlGenLevelSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  if (serverAddr == MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlGenLevelPublishSet(MMDL_GEN_LEVEL_SET_OPCODE, elementId, pSetParam);
  }
  else
  {
    mmdlGenLevelSendSet(MMDL_GEN_LEVEL_SET_OPCODE, elementId, serverAddr, ttl, pSetParam,
                        appKeyIndex);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a GenLevelSetUnacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR
 *  \param[in] ttl          TTL value as defined by the specification
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            const mmdlGenLevelSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  if (serverAddr == MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlGenLevelPublishSet(MMDL_GEN_LEVEL_SET_NO_ACK_OPCODE, elementId, pSetParam);
  }
  else
  {
    mmdlGenLevelSendSet(MMDL_GEN_LEVEL_SET_NO_ACK_OPCODE, elementId, serverAddr, ttl, pSetParam,
                        appKeyIndex);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a GenDeltaSet message to the destination address.
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
void MmdlGenDeltaClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                       const mmdlGenDeltaSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  if (serverAddr == MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlGenLevelPublishDeltaSet(MMDL_GEN_LEVEL_DELTA_SET_OPCODE, elementId, pSetParam);
  }
  else
  {
    mmdlGenLevelSendDeltaSet(MMDL_GEN_LEVEL_DELTA_SET_OPCODE, elementId, serverAddr, ttl, pSetParam,
                             appKeyIndex);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a GenDeltaSetUnacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR
 *  \param[in] ttl          TTL value as defined by the specification
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenDeltaClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            const mmdlGenDeltaSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  if (serverAddr == MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlGenLevelPublishDeltaSet(MMDL_GEN_LEVEL_DELTA_SET_NO_ACK_OPCODE, elementId, pSetParam);
  }
  else
  {
    mmdlGenLevelSendDeltaSet(MMDL_GEN_LEVEL_DELTA_SET_NO_ACK_OPCODE, elementId, serverAddr, ttl,
                             pSetParam, appKeyIndex);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a GenMoveSet message to the destination address.
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
void MmdlGenMoveClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                      const mmdlGenLevelSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  if (serverAddr == MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlGenLevelPublishSet(MMDL_GEN_LEVEL_MOVE_SET_OPCODE, elementId, pSetParam);
  }
  else
  {
    mmdlGenLevelSendSet(MMDL_GEN_LEVEL_MOVE_SET_OPCODE, elementId, serverAddr, ttl, pSetParam,
                        appKeyIndex);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a GenMoveSetUnacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR
 *  \param[in] ttl          TTL value as defined by the specification
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenMoveClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                           const mmdlGenLevelSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  if (serverAddr == MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlGenLevelPublishSet(MMDL_GEN_LEVEL_MOVE_SET_NO_ACK_OPCODE, elementId, pSetParam);
  }
  else
  {
    mmdlGenLevelSendSet(MMDL_GEN_LEVEL_MOVE_SET_NO_ACK_OPCODE, elementId, serverAddr, ttl, pSetParam,
                        appKeyIndex);
  }
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
void MmdlGenLevelClRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback*/
  if (recvCback != NULL)
  {
    levelClCb.recvCback = recvCback;
  }
}
