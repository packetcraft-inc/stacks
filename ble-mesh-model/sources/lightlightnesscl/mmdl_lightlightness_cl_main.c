/*************************************************************************************************/
/*!
 *  \file   mmdl_lightlightness_cl_main.c
 *
 *  \brief  Implementation of the Light Lightness Client model.
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
#include "wsf_os.h"
#include "wsf_trace.h"
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"

#include "mmdl_types.h"
#include "mmdl_common.h"
#include "mmdl_lightlightness_defs.h"
#include "mmdl_lightlightness_cl_api.h"
#include "mmdl_lightlightness_cl_main.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Light Lightness Client control block type definition */
typedef struct mmdlLightLightnessClCb_tag
{
  mmdlEventCback_t            recvCback;        /*!< Model Light Lightness received callback */
}mmdlLightLightnessClCb_t;

/*! Light Lightness Client message handler type definition */
typedef void (*mmdlLightnessClHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler id */
wsfHandlerId_t mmdlLightLightnessClHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlLightLightnessClRcvdOpcodes[] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_LIGHTNESS_STATUS_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_LIGHTNESS_LINEAR_STATUS_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_LIGHTNESS_LAST_STATUS_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_LIGHTNESS_DEFAULT_STATUS_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_LIGHTNESS_RANGE_STATUS_OPCODE)} },
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/* Handler functions for supported opcodes */
const mmdlLightnessClHandleMsg_t mmdlLightLightnessClHandleMsg[MMDL_LIGHT_LIGHTNESS_CL_NUM_RCVD_OPCODES] =
{
  mmdlLightLightnessClHandleStatus,
  mmdlLightLightnessLinearClHandleStatus,
  mmdlLightLightnessLastClHandleStatus,
  mmdlLightLightnessDefaultClHandleStatus,
  mmdlLightLightnessRangeClHandleStatus
};
/*! Light Lightness Client control block */
static mmdlLightLightnessClCb_t  lightLightnessClCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light Lightness Set message to the destination address.
 *
 *  \param[in] pMsgInfo     Pointer to a Mesh message information structure. See ::meshMsgInfo_t.
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] ackRequired  Ack required for this command
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightLightnessSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            const mmdlLightLightnessSetParam_t *pSetParam, uint16_t appKeyIndex,
                            bool_t ackRequired)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_LIGHT_LIGHTNESS_CL_MDL_ID,
                                   MMDL_LIGHT_LIGHTNESS_SET_NO_ACK_OPCODE);
  uint8_t paramLen = MMDL_LIGHT_LIGHTNESS_SET_MAX_LEN;
  uint8_t paramMsg[MMDL_LIGHT_LIGHTNESS_SET_MAX_LEN];
  uint8_t *pCursor = paramMsg;

  if (pSetParam!= NULL)
  {
    /* Change to acknowledged set */
    if (ackRequired)
    {
      UINT16_TO_BE_BUF(msgInfo.opcode.opcodeBytes, MMDL_LIGHT_LIGHTNESS_SET_OPCODE);
    }

    /* Fill in the message information */
    msgInfo.elementId = elementId;
    msgInfo.dstAddr = serverAddr;
    msgInfo.ttl = ttl;
    msgInfo.appKeyIndex = appKeyIndex;

    /* Build param message. */
    UINT16_TO_BSTREAM(pCursor, pSetParam->lightness);
    UINT8_TO_BSTREAM(pCursor, pSetParam->tid);

    /* Do not include transition time and delay in the message if it is not used */
    if (pSetParam->transitionTime == MMDL_GEN_TR_UNKNOWN)
    {
      paramLen = MMDL_LIGHT_LIGHTNESS_SET_MIN_LEN;
    }
    else
    {
      UINT8_TO_BSTREAM(pCursor, pSetParam->transitionTime);
      UINT8_TO_BSTREAM(pCursor, pSetParam->delay);
    }

    /* Send message to the Mesh Core */
    MeshSendMessage(&msgInfo, paramMsg, paramLen, 0, 0);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light Lightness Set message to the destination address.
 *
 *  \param[in] pMsgInfo     Pointer to a Mesh message information structure. See ::meshMsgInfo_t.
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] ackRequired  Ack required for this command
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightLightnessLinearSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            const mmdlLightLightnessLinearSetParam_t *pSetParam, uint16_t appKeyIndex,
                            bool_t ackRequired)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_LIGHT_LIGHTNESS_CL_MDL_ID,
                                   MMDL_LIGHT_LIGHTNESS_LINEAR_SET_NO_ACK_OPCODE);
  uint8_t paramLen = MMDL_LIGHT_LIGHTNESS_LINEAR_SET_MAX_LEN;
  uint8_t paramMsg[MMDL_LIGHT_LIGHTNESS_LINEAR_SET_MAX_LEN];
  uint8_t *pCursor = paramMsg;

  if (pSetParam!= NULL)
  {
    /* Change to acknowledged set */
    if (ackRequired)
    {
      UINT16_TO_BE_BUF(msgInfo.opcode.opcodeBytes, MMDL_LIGHT_LIGHTNESS_LINEAR_SET_OPCODE);
    }

    /* Fill in the message information */
    msgInfo.elementId = elementId;
    msgInfo.dstAddr = serverAddr;
    msgInfo.ttl = ttl;
    msgInfo.appKeyIndex = appKeyIndex;

    /* Build param message. */
    UINT16_TO_BSTREAM(pCursor, pSetParam->lightness);
    UINT8_TO_BSTREAM(pCursor, pSetParam->tid);

    /* Do not include transition time and delay in the message if it is not used */
    if (pSetParam->transitionTime == MMDL_GEN_TR_UNKNOWN)
    {
      paramLen = MMDL_LIGHT_LIGHTNESS_LINEAR_SET_MIN_LEN;
    }
    else
    {
      UINT8_TO_BSTREAM(pCursor, pSetParam->transitionTime);
      UINT8_TO_BSTREAM(pCursor, pSetParam->delay);
    }

    /* Send message to the Mesh Core */
    MeshSendMessage(&msgInfo, paramMsg, paramLen, 0, 0);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light Lightness Default Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] ackRequired  Ack required for this command
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightLightnessDefaultSet(meshElementId_t elementId, meshAddress_t serverAddr,
                                uint8_t ttl, const mmdlLightLightnessDefaultSetParam_t *pSetParam,
                                uint16_t appKeyIndex, bool_t ackRequired)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_LIGHT_LIGHTNESS_CL_MDL_ID,
                                   MMDL_LIGHT_LIGHTNESS_DEFAULT_SET_NO_ACK_OPCODE);
  uint8_t paramLen = MMDL_LIGHT_LIGHTNESS_DEFAULT_SET_LEN;
  uint8_t paramMsg[MMDL_LIGHT_LIGHTNESS_DEFAULT_SET_LEN];
  uint8_t *pCursor = paramMsg;

  if (pSetParam != NULL)
  {
    /* Change to acknowledged set */
    if (ackRequired)
    {
      UINT16_TO_BE_BUF(msgInfo.opcode.opcodeBytes, MMDL_LIGHT_LIGHTNESS_DEFAULT_SET_OPCODE);
    }

    /* Fill in the message information */
    msgInfo.elementId = elementId;
    msgInfo.dstAddr = serverAddr;
    msgInfo.ttl = ttl;
    msgInfo.appKeyIndex = appKeyIndex;

    /* Build param message. */
    UINT16_TO_BSTREAM(pCursor, pSetParam->lightness);

    /* Send message to the Mesh Core */
    MeshSendMessage(&msgInfo, paramMsg, paramLen, 0, 0);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light Lightness Range Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] ackRequired  Ack required for this command
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightLightnessRangeSet(meshElementId_t elementId, meshAddress_t serverAddr,
                                         uint8_t ttl,
                                         const mmdlLightLightnessRangeSetParam_t *pSetParam,
                                         uint16_t appKeyIndex, bool_t ackRequired)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_LIGHT_LIGHTNESS_CL_MDL_ID,
                                   MMDL_LIGHT_LIGHTNESS_RANGE_SET_NO_ACK_OPCODE);
  uint8_t paramLen = MMDL_LIGHT_LIGHTNESS_RANGE_SET_LEN;
  uint8_t paramMsg[MMDL_LIGHT_LIGHTNESS_RANGE_SET_LEN];
  uint8_t *pCursor = paramMsg;

  if (pSetParam != NULL)
  {
    /* Change to acknowledged set */
    if (ackRequired)
    {
      UINT16_TO_BE_BUF(msgInfo.opcode.opcodeBytes, MMDL_LIGHT_LIGHTNESS_RANGE_SET_OPCODE);
    }

    /* Fill in the message information */
    msgInfo.elementId = elementId;
    msgInfo.dstAddr = serverAddr;
    msgInfo.ttl = ttl;
    msgInfo.appKeyIndex = appKeyIndex;

    /* Build param message. */
    UINT16_TO_BSTREAM(pCursor, pSetParam->rangeMin);
    UINT16_TO_BSTREAM(pCursor, pSetParam->rangeMax);

    /* Send message to the Mesh Core */
    MeshSendMessage(&msgInfo, paramMsg, paramLen, 0, 0);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publishes a Light Lightness Set message to the publication address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] ackRequired  Ack required for this command
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightLightnessPublishSet(meshElementId_t elementId,
                                const mmdlLightLightnessSetParam_t *pSetParam, bool_t ackRequired)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_LIGHT_LIGHTNESS_CL_MDL_ID,
                                             MMDL_LIGHT_LIGHTNESS_SET_NO_ACK_OPCODE);
  uint8_t paramLen = MMDL_LIGHT_LIGHTNESS_SET_MAX_LEN;
  uint8_t paramMsg[MMDL_LIGHT_LIGHTNESS_SET_MAX_LEN];
  uint8_t *pCursor = paramMsg;

  if (pSetParam != NULL)
  {
    /* Change to acknowledged set */
    if (ackRequired)
    {
      UINT16_TO_BE_BUF(pubMsgInfo.opcode.opcodeBytes, MMDL_LIGHT_LIGHTNESS_SET_OPCODE);
    }

    /* Fill in the msg info parameters */
    pubMsgInfo.elementId = elementId;

    /* Build param message. */
    UINT16_TO_BSTREAM(pCursor, pSetParam->lightness);
    UINT8_TO_BSTREAM(pCursor, pSetParam->tid);

    /* Do not include transition time and delay in the message if it is not used */
    if (pSetParam->transitionTime == MMDL_GEN_TR_UNKNOWN)
    {
      paramLen = MMDL_LIGHT_LIGHTNESS_SET_MIN_LEN;
    }
    else
    {
      UINT8_TO_BSTREAM(pCursor, pSetParam->transitionTime);
      UINT8_TO_BSTREAM(pCursor, pSetParam->delay);
    }

    /* Send message to the Mesh Core. Parameters are already stored in over-the-air order */
    MeshPublishMessage(&pubMsgInfo, paramMsg, paramLen);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publishes a Light Lightness Linear Set message to the publication address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] ackRequired  Ack required for this command
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightLightnessLinearPublishSet(meshElementId_t elementId,
                           const mmdlLightLightnessLinearSetParam_t *pSetParam, bool_t ackRequired)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_LIGHT_LIGHTNESS_CL_MDL_ID,
                                             MMDL_LIGHT_LIGHTNESS_LINEAR_SET_NO_ACK_OPCODE);
  uint8_t paramLen = MMDL_LIGHT_LIGHTNESS_LINEAR_SET_MAX_LEN;
  uint8_t paramMsg[MMDL_LIGHT_LIGHTNESS_LINEAR_SET_MAX_LEN];
  uint8_t *pCursor = paramMsg;

  if (pSetParam != NULL)
  {
    /* Change to acknowledged set */
    if (ackRequired)
    {
      UINT16_TO_BE_BUF(pubMsgInfo.opcode.opcodeBytes, MMDL_LIGHT_LIGHTNESS_LINEAR_SET_OPCODE);
    }

    /* Fill in the msg info parameters */
    pubMsgInfo.elementId = elementId;

    /* Build param message. */
    UINT16_TO_BSTREAM(pCursor, pSetParam->lightness);
    UINT8_TO_BSTREAM(pCursor, pSetParam->tid);

    /* Do not include transition time and delay in the message if it is not used */
    if (pSetParam->transitionTime == MMDL_GEN_TR_UNKNOWN)
    {
      paramLen = MMDL_LIGHT_LIGHTNESS_LINEAR_SET_MIN_LEN;
    }
    else
    {
      UINT8_TO_BSTREAM(pCursor, pSetParam->transitionTime);
      UINT8_TO_BSTREAM(pCursor, pSetParam->delay);
    }

    /* Send message to the Mesh Core. Parameters are already stored in over-the-air order */
    MeshPublishMessage(&pubMsgInfo, paramMsg, paramLen);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publishes a Light Lightness Default Set message to the publication address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] ackRequired  Ack required for this command
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightLightnessDefaultPublishSet(meshElementId_t elementId,
                          const mmdlLightLightnessDefaultSetParam_t *pSetParam, bool_t ackRequired)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_LIGHT_LIGHTNESS_CL_MDL_ID,
                                             MMDL_LIGHT_LIGHTNESS_DEFAULT_SET_NO_ACK_OPCODE);
  uint8_t paramLen = MMDL_LIGHT_LIGHTNESS_DEFAULT_SET_LEN;
  uint8_t paramMsg[MMDL_LIGHT_LIGHTNESS_DEFAULT_SET_LEN];
  uint8_t *pCursor = paramMsg;

  if (pSetParam != NULL)
  {
    /* Change to acknowledged set */
    if (ackRequired)
    {
      UINT16_TO_BE_BUF(pubMsgInfo.opcode.opcodeBytes, MMDL_LIGHT_LIGHTNESS_DEFAULT_SET_OPCODE);
    }

    /* Fill in the msg info parameters */
    pubMsgInfo.elementId = elementId;

    /* Build param message. */
    UINT16_TO_BSTREAM(pCursor, pSetParam->lightness);

    /* Send message to the Mesh Core. Parameters are already stored in over-the-air order */
    MeshPublishMessage(&pubMsgInfo, paramMsg, paramLen);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publishes a Light Lightness Range Set message to the publication address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] ackRequired  Ack required for this command
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightLightnessRangePublishSet(meshElementId_t elementId,
                          const mmdlLightLightnessRangeSetParam_t *pSetParam, bool_t ackRequired)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_LIGHT_LIGHTNESS_CL_MDL_ID,
                                             MMDL_LIGHT_LIGHTNESS_RANGE_SET_NO_ACK_OPCODE);
  uint8_t paramLen = MMDL_LIGHT_LIGHTNESS_RANGE_SET_LEN;
  uint8_t paramMsg[MMDL_LIGHT_LIGHTNESS_RANGE_SET_LEN];
  uint8_t *pCursor = paramMsg;

  if (pSetParam != NULL)
  {
    /* Change to acknowledged set */
    if (ackRequired)
    {
      UINT16_TO_BE_BUF(pubMsgInfo.opcode.opcodeBytes, MMDL_LIGHT_LIGHTNESS_RANGE_SET_OPCODE);
    }

    /* Fill in the msg info parameters */
    pubMsgInfo.elementId = elementId;

    /* Build param message. */
    UINT16_TO_BSTREAM(pCursor, pSetParam->rangeMin);
    UINT16_TO_BSTREAM(pCursor, pSetParam->rangeMax);

    /* Send message to the Mesh Core. Parameters are already stored in over-the-air order */
    MeshPublishMessage(&pubMsgInfo, paramMsg, paramLen);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessClHandleStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlLightLightnessClEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_LIGHT_LIGHTNESS_STATUS_MAX_LEN &&
      pMsg->messageParamsLen != MMDL_LIGHT_LIGHTNESS_STATUS_MIN_LEN)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_LIGHT_LIGHTNESS_CL_EVENT;
  event.hdr.param = MMDL_LIGHT_LIGHTNESS_CL_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  pParams = pMsg->pMessageParams;

  /* Extract status event parameters */
  BSTREAM_TO_UINT16(event.statusParam.actualStatusEvent.presentLightness, pParams);

  /* Check if optional parameters are present */
  if (pMsg->messageParamsLen == MMDL_LIGHT_LIGHTNESS_STATUS_MAX_LEN)
  {
    /* Extract target state and Remaining Time value */
    BSTREAM_TO_UINT16(event.statusParam.actualStatusEvent.targetLightness, pParams);
    BSTREAM_TO_UINT8(event.statusParam.actualStatusEvent.remainingTime, pParams);
  }
  else
  {
    /* Set state to the target state from pParams. */
    event.statusParam.actualStatusEvent.targetLightness = 0;
    event.statusParam.actualStatusEvent.remainingTime = 0;
  }

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  lightLightnessClCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Linear Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessLinearClHandleStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlLightLightnessClEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_LIGHT_LIGHTNESS_LINEAR_STATUS_MAX_LEN &&
      pMsg->messageParamsLen != MMDL_LIGHT_LIGHTNESS_LINEAR_STATUS_MIN_LEN)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_LIGHT_LIGHTNESS_CL_EVENT;
  event.hdr.param = MMDL_LIGHT_LIGHTNESS_LINEAR_CL_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  pParams = pMsg->pMessageParams;

  /* Extract status event parameters */
  BSTREAM_TO_UINT16(event.statusParam.linearStatusEvent.presentLightness, pParams);

  /* Check if optional parameters are present */
  if (pMsg->messageParamsLen == MMDL_LIGHT_LIGHTNESS_LINEAR_STATUS_MAX_LEN)
  {
    /* Extract target state and Remaining Time value */
    BSTREAM_TO_UINT16(event.statusParam.linearStatusEvent.targetLightness, pParams);
    BSTREAM_TO_UINT8(event.statusParam.linearStatusEvent.remainingTime, pParams);
  }
  else
  {
    /* Set state to the target state from pParams. */
    event.statusParam.linearStatusEvent.targetLightness = 0;
    event.statusParam.linearStatusEvent.remainingTime = 0;
  }

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  lightLightnessClCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Last Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessLastClHandleStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlLightLightnessClEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_LIGHT_LIGHTNESS_LAST_STATUS_LEN)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_LIGHT_LIGHTNESS_CL_EVENT;
  event.hdr.param = MMDL_LIGHT_LIGHTNESS_LAST_CL_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  pParams = pMsg->pMessageParams;

  /* Extract status event parameters */
  BSTREAM_TO_UINT16(event.statusParam.lastStatusEvent.lightness, pParams);

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  lightLightnessClCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Default Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessDefaultClHandleStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlLightLightnessClEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_LIGHT_LIGHTNESS_DEFAULT_STATUS_LEN)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_LIGHT_LIGHTNESS_CL_EVENT;
  event.hdr.param = MMDL_LIGHT_LIGHTNESS_DEFAULT_CL_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  pParams = pMsg->pMessageParams;

  /* Extract status event parameters */
  BSTREAM_TO_UINT16(event.statusParam.defaultStatusEvent.lightness, pParams);

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  lightLightnessClCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Range Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessRangeClHandleStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlLightLightnessClEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_LIGHT_LIGHTNESS_RANGE_STATUS_LEN)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_LIGHT_LIGHTNESS_CL_EVENT;
  event.hdr.param = MMDL_LIGHT_LIGHTNESS_RANGE_CL_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  pParams = pMsg->pMessageParams;

  /* Extract status event parameters */
  BSTREAM_TO_UINT8(event.statusParam.rangeStatusEvent.statusCode, pParams);
  BSTREAM_TO_UINT16(event.statusParam.rangeStatusEvent.rangeMin, pParams);
  BSTREAM_TO_UINT16(event.statusParam.rangeStatusEvent.rangeMax, pParams);

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  lightLightnessClCb.recvCback((wsfMsgHdr_t *)&event);
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Light Lightness Client Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessClHandlerInit(wsfHandlerId_t handlerId)
{
  /* Set handler ID */
  mmdlLightLightnessClHandlerId = handlerId;

  /* Initialize control block */
  lightLightnessClCb.recvCback = MmdlEmptyCback;
}

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Light Lightness Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessClHandler(wsfMsgHdr_t *pMsg)
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
        for (opcodeIdx = 0; opcodeIdx < MMDL_LIGHT_LIGHTNESS_CL_NUM_RCVD_OPCODES; opcodeIdx++)
        {
          opcodeSize = MESH_OPCODE_SIZE(pModelMsg->opCode);
          if (!memcmp(&mmdlLightLightnessClRcvdOpcodes[opcodeIdx], pModelMsg->opCode.opcodeBytes,
              opcodeSize))
          {
            /* Process message */
            (void)mmdlLightLightnessClHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
          }
        }
        break;

      default:
        MESH_TRACE_WARN0("LIGHT LIGHTNESS CL: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light Lightness Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                        uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_LIGHT_LIGHTNESS_CL_MDL_ID,
                                   MMDL_LIGHT_LIGHTNESS_GET_OPCODE);
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_LIGHT_LIGHTNESS_CL_MDL_ID,
                                             MMDL_LIGHT_LIGHTNESS_GET_OPCODE);

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
 *  \brief     Send a Light Lightness Set message to the destination address.
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
void MmdlLightLightnessClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          const mmdlLightLightnessSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  if (serverAddr == MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightLightnessPublishSet(elementId, pSetParam, TRUE);
  }
  else
  {
    mmdlLightLightnessSet(elementId, serverAddr, ttl, pSetParam, appKeyIndex, TRUE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light Lightness Set Unacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                               const mmdlLightLightnessSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  if (serverAddr == MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightLightnessPublishSet(elementId, pSetParam, FALSE);
  }
  else
  {
    mmdlLightLightnessSet(elementId, serverAddr, ttl, pSetParam, appKeyIndex, FALSE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light Lightness Linear Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessLinearClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                        uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_LIGHT_LIGHTNESS_CL_MDL_ID,
                                  MMDL_LIGHT_LIGHTNESS_LINEAR_GET_OPCODE);
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_LIGHT_LIGHTNESS_CL_MDL_ID,
                                              MMDL_LIGHT_LIGHTNESS_LINEAR_GET_OPCODE);

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
 *  \brief     Send a Light Lightness Linear Set message to the destination address.
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
void MmdlLightLightnessLinearClSet(meshElementId_t elementId, meshAddress_t serverAddr,
                          uint8_t ttl, const mmdlLightLightnessLinearSetParam_t *pSetParam,
                          uint16_t appKeyIndex)
{
  if (serverAddr == MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightLightnessLinearPublishSet(elementId, pSetParam, TRUE);
  }
  else
  {
    mmdlLightLightnessLinearSet(elementId, serverAddr, ttl, pSetParam, appKeyIndex, TRUE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light Lightness Linear Set Unacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessLinearClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr,
                               uint8_t ttl, const mmdlLightLightnessLinearSetParam_t *pSetParam,
                               uint16_t appKeyIndex)
{
  if (serverAddr == MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightLightnessLinearPublishSet(elementId, pSetParam, FALSE);
  }
  else
  {
    mmdlLightLightnessLinearSet(elementId, serverAddr, ttl, pSetParam, appKeyIndex, FALSE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light Lightness Last Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessLastClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                 uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_LIGHT_LIGHTNESS_CL_MDL_ID,
                                   MMDL_LIGHT_LIGHTNESS_LAST_GET_OPCODE);

  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_LIGHT_LIGHTNESS_CL_MDL_ID,
                                             MMDL_LIGHT_LIGHTNESS_LAST_GET_OPCODE);

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
 *  \brief     Send a Light Lightness Default Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessDefaultClGet(meshElementId_t elementId, meshAddress_t serverAddr,
                                    uint8_t ttl, uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_LIGHT_LIGHTNESS_CL_MDL_ID,
                                   MMDL_LIGHT_LIGHTNESS_DEFAULT_GET_OPCODE);
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_LIGHT_LIGHTNESS_CL_MDL_ID,
                                             MMDL_LIGHT_LIGHTNESS_DEFAULT_GET_OPCODE);

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
 *  \brief     Send a Light Lightness Default Set message to the destination address.
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
void MmdlLightLightnessDefaultClSet(meshElementId_t elementId, meshAddress_t serverAddr,
                                    uint8_t ttl,
                                    const mmdlLightLightnessDefaultSetParam_t *pSetParam,
                                    uint16_t appKeyIndex)
{
  if (serverAddr == MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightLightnessDefaultPublishSet(elementId, pSetParam, TRUE);
  }
  else
  {
    mmdlLightLightnessDefaultSet(elementId, serverAddr, ttl, pSetParam, appKeyIndex, TRUE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light Lightness Default Set Unacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] serverAddr   Element address of the server
 *  \param[in] ttl          TTL value as defined by the specification
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessDefaultClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr,
                                         uint8_t ttl,
                                         const mmdlLightLightnessDefaultSetParam_t *pSetParam,
                                         uint16_t appKeyIndex)
{
  if (serverAddr == MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightLightnessDefaultPublishSet(elementId, pSetParam, FALSE);
  }
  else
  {
    mmdlLightLightnessDefaultSet(elementId, serverAddr, ttl, pSetParam, appKeyIndex, FALSE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light Lightness Ranger Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessRangeClGet(meshElementId_t elementId, meshAddress_t serverAddr,
                                  uint8_t ttl, uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_LIGHT_LIGHTNESS_CL_MDL_ID,
                                   MMDL_LIGHT_LIGHTNESS_RANGE_GET_OPCODE);
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_LIGHT_LIGHTNESS_CL_MDL_ID,
                                             MMDL_LIGHT_LIGHTNESS_RANGE_GET_OPCODE);

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
 *  \brief     Send a Light Lightness Range Set message to the destination address.
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
void MmdlLightLightnessRangeClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                  const mmdlLightLightnessRangeSetParam_t *pSetParam,
                                  uint16_t appKeyIndex)
{
  if (serverAddr == MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightLightnessRangePublishSet(elementId, pSetParam, TRUE);
  }
  else
  {
    mmdlLightLightnessRangeSet(elementId, serverAddr, ttl, pSetParam, appKeyIndex, TRUE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light Lightness Range Set Unacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] serverAddr   Element address of the server
 *  \param[in] ttl          TTL value as defined by the specification
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessRangeClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr,
                                       uint8_t ttl,
                                       const mmdlLightLightnessRangeSetParam_t *pSetParam,
                                       uint16_t appKeyIndex)
{
  if (serverAddr == MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightLightnessRangePublishSet(elementId, pSetParam, FALSE);
  }
  else
  {
    mmdlLightLightnessRangeSet(elementId, serverAddr, ttl, pSetParam, appKeyIndex, FALSE);
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
void MmdlLightLightnessClRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback */
  if (recvCback != NULL)
  {
    lightLightnessClCb.recvCback = recvCback;
  }
}
