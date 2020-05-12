/*************************************************************************************************/
/*!
 *  \file   mmdl_light_hsl_cl_main.c
 *
 *  \brief  Implementation of the Light HSL Client model.
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
#include "wsf_buf.h"
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"

#include "mmdl_types.h"
#include "mmdl_common.h"
#include "mmdl_light_hsl_cl_api.h"
#include "mmdl_light_hsl_cl_main.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Light HSL Client control block type definition */
typedef struct mmdlLightHslClCb_tag
{
  mmdlEventCback_t recvCback;    /*!< Model received callback */
}mmdlLightHslClCb_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler id */
wsfHandlerId_t mmdlLightHslClHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlLightHslClRcvdOpcodes[MMDL_LIGHT_HSL_CL_NUM_RCVD_OPCODES] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_HSL_STATUS_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_HSL_TARGET_STATUS_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_HSL_HUE_STATUS_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_HSL_SAT_STATUS_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_HSL_DEFAULT_STATUS_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_HSL_RANGE_STATUS_OPCODE)} }
};

/*! Light HSL Client message handler type definition */
typedef void (*mmdlLightHslClHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlLightHslClHandleMsg_t mmdlLightHslClHandleMsg[MMDL_LIGHT_HSL_CL_NUM_RCVD_OPCODES] =
{
  mmdlLightHslClHandleStatus,
  mmdlLightHslClHandleTargetStatus,
  mmdlLightHslClHandleHueStatus,
  mmdlLightHslClHandleSatStatus,
  mmdlLightHslClHandleDefStatus,
  mmdlLightHslClHandleRangeStatus
};

/*! Light HSL Client control block */
static mmdlLightHslClCb_t  lightHslClCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light HSL Client message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the message parameters.
 *  \param[in] paramLen     Length of message parameters structure.
 *  \param[in] opcode       Light HSL Client message opcode.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightHslSendMessage(meshElementId_t elementId, meshAddress_t serverAddr,
                                    uint8_t ttl, uint16_t appKeyIndex, const uint8_t *pParam,
                                    uint8_t paramLen, uint16_t opcode)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_LIGHT_HSL_CL_MDL_ID, opcode);

  /* Fill in the message information */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = serverAddr;
  msgInfo.ttl = ttl;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Send message to the Mesh Core. Parameters are already stored in over-the-air order */
  MeshSendMessage(&msgInfo, (uint8_t *)pParam, paramLen, 0, 0);
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
static void mmdlLightHslPublishMessage(meshElementId_t elementId, const uint8_t *pParam,
                                       uint8_t paramLen, uint16_t opcode)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_LIGHT_HSL_CL_MDL_ID, opcode);

  /* Fill in the msg info parameters */
  pubMsgInfo.elementId = elementId;

  /* Send message to the Mesh Core. Parameters are already stored in over-the-air order */
  MeshPublishMessage(&pubMsgInfo, (uint8_t *)pParam, paramLen);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light HSL Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightHslClHandleStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlLightHslClStatusEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_LIGHT_HSL_STATUS_MAX_LEN &&
      pMsg->messageParamsLen != MMDL_LIGHT_HSL_STATUS_MIN_LEN)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_LIGHT_HSL_CL_EVENT;
  event.hdr.param = MMDL_LIGHT_HSL_CL_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  /* Extract status event parameters */
  pParams = pMsg->pMessageParams;
  BSTREAM_TO_UINT16(event.lightness, pParams);
  BSTREAM_TO_UINT16(event.hue, pParams);
  BSTREAM_TO_UINT16(event.saturation, pParams);

  /* Check if optional parameters are present */
  if (pMsg->messageParamsLen == MMDL_LIGHT_HSL_STATUS_MAX_LEN)
  {
    BSTREAM_TO_UINT8(event.remainingTime, pParams);
  }
  else
  {
    event.remainingTime = 0;
  }

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  lightHslClCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light HSL Target Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightHslClHandleTargetStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlLightHslClStatusEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_LIGHT_HSL_STATUS_MAX_LEN &&
      pMsg->messageParamsLen != MMDL_LIGHT_HSL_STATUS_MIN_LEN)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_LIGHT_HSL_CL_EVENT;
  event.hdr.param = MMDL_LIGHT_HSL_CL_TARGET_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  /* Extract status event parameters */
  pParams = pMsg->pMessageParams;
  BSTREAM_TO_UINT16(event.lightness, pParams);
  BSTREAM_TO_UINT16(event.hue, pParams);
  BSTREAM_TO_UINT16(event.saturation, pParams);

  /* Check if optional parameters are present */
  if (pMsg->messageParamsLen == MMDL_LIGHT_HSL_STATUS_MAX_LEN)
  {
    BSTREAM_TO_UINT8(event.remainingTime, pParams);
  }
  else
  {
    event.remainingTime = 0;
  }

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  lightHslClCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light HSL Hue Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightHslClHandleHueStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlLightHslClHueStatusEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_LIGHT_HSL_HUE_STATUS_MAX_LEN &&
      pMsg->messageParamsLen != MMDL_LIGHT_HSL_HUE_STATUS_MIN_LEN)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_LIGHT_HSL_CL_EVENT;
  event.hdr.param = MMDL_LIGHT_HSL_CL_HUE_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  /* Extract status event parameters */
  pParams = pMsg->pMessageParams;
  BSTREAM_TO_UINT16(event.presentHue, pParams);

  /* Check if optional parameters are present */
  if (pMsg->messageParamsLen == MMDL_LIGHT_HSL_HUE_STATUS_MAX_LEN)
  {
    /* Extract target state */
    BSTREAM_TO_UINT16(event.targetHue, pParams);
    BSTREAM_TO_UINT8(event.remainingTime, pParams);
  }
  else
  {
    event.remainingTime = 0;
    event.targetHue = 0;
  }

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  lightHslClCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light HSL Saturation Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightHslClHandleSatStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlLightHslClSatStatusEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_LIGHT_HSL_SAT_STATUS_MAX_LEN &&
      pMsg->messageParamsLen != MMDL_LIGHT_HSL_SAT_STATUS_MIN_LEN)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_LIGHT_HSL_CL_EVENT;
  event.hdr.param = MMDL_LIGHT_HSL_CL_SAT_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  /* Extract status event parameters */
  pParams = pMsg->pMessageParams;
  BSTREAM_TO_UINT16(event.presentSat, pParams);

  /* Check if optional parameters are present */
  if (pMsg->messageParamsLen == MMDL_LIGHT_HSL_SAT_STATUS_MAX_LEN)
  {
    /* Extract target state */
    BSTREAM_TO_UINT16(event.targetSat, pParams);
    BSTREAM_TO_UINT8(event.remainingTime, pParams);
  }
  else
  {
    event.remainingTime = 0;
    event.targetSat = 0;
  }

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  lightHslClCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light HSL Default Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightHslClHandleDefStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlLightHslClDefStatusEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_LIGHT_HSL_DEF_STATUS_LEN)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_LIGHT_HSL_CL_EVENT;
  event.hdr.param = MMDL_LIGHT_HSL_CL_DEF_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  /* Extract status event parameters */
  pParams = pMsg->pMessageParams;
  BSTREAM_TO_UINT16(event.lightness, pParams);
  BSTREAM_TO_UINT16(event.hue, pParams);
  BSTREAM_TO_UINT16(event.saturation, pParams);

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  lightHslClCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light HSL Range Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightHslClHandleRangeStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlLightHslClRangeStatusEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_LIGHT_HSL_RANGE_STATUS_LEN)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_LIGHT_HSL_CL_EVENT;
  event.hdr.param = MMDL_LIGHT_HSL_CL_RANGE_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  /* Extract status event parameters */
  pParams = pMsg->pMessageParams;
  BSTREAM_TO_UINT8(event.opStatus, pParams);
  BSTREAM_TO_UINT16(event.minHue, pParams);
  BSTREAM_TO_UINT16(event.maxHue, pParams);
  BSTREAM_TO_UINT16(event.minSaturation, pParams);
  BSTREAM_TO_UINT16(event.maxSaturation, pParams);

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  lightHslClCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *  \param[in] ackReq       TRUE if Ack is requested, FALSE otherwise.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightHslClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                       uint16_t appKeyIndex, const mmdlLightHslSetParam_t *pParam, bool_t ackReq)
{
  uint8_t param[MMDL_LIGHT_HSL_SET_MAX_LEN];
  uint8_t *pCursor = param;
  uint16_t opcode = MMDL_LIGHT_HSL_SET_NO_ACK_OPCODE;

  if (pParam == NULL)
  {
    return;
  }

  /* Select opcode */
  if (ackReq)
  {
    opcode = MMDL_LIGHT_HSL_SET_OPCODE;
  }

  /* Build OTA fields */
  UINT16_TO_BSTREAM(pCursor, pParam->lightness);
  UINT16_TO_BSTREAM(pCursor, pParam->hue);
  UINT16_TO_BSTREAM(pCursor, pParam->saturation);
  UINT8_TO_BSTREAM(pCursor, pParam->tid);

  /* Do not include transition time and delay in the message if it is not used */
  if (pParam->transitionTime != MMDL_GEN_TR_UNKNOWN)
  {
    UINT8_TO_BSTREAM(pCursor, pParam->transitionTime);
    UINT8_TO_BSTREAM(pCursor, pParam->delay);
  }

  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightHslSendMessage(elementId, serverAddr, ttl, appKeyIndex, param,
                            (uint8_t)(pCursor - param), opcode);
  }
  else
  {
    mmdlLightHslPublishMessage(elementId, param, (uint8_t)(pCursor - param), opcode);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Hue Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *  \param[in] ackReq       TRUE if Ack is requested, FALSE otherwise.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightHslClHueSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          uint16_t appKeyIndex, const mmdlLightHslHueSetParam_t *pParam,
                          bool_t ackReq)
{
  uint8_t param[MMDL_LIGHT_HSL_HUE_SET_MAX_LEN];
  uint8_t *pCursor = param;
  uint16_t opcode = MMDL_LIGHT_HSL_HUE_SET_NO_ACK_OPCODE;

  if (pParam == NULL)
  {
    return;
  }

  /* Select opcode */
  if (ackReq)
  {
    opcode = MMDL_LIGHT_HSL_HUE_SET_OPCODE;
  }

  /* Build OTA fields */
  UINT16_TO_BSTREAM(pCursor, pParam->hue);
  UINT8_TO_BSTREAM(pCursor, pParam->tid);

  /* Do not include transition time and delay in the message if it is not used */
  if (pParam->transitionTime != MMDL_GEN_TR_UNKNOWN)
  {
    UINT8_TO_BSTREAM(pCursor, pParam->transitionTime);
    UINT8_TO_BSTREAM(pCursor, pParam->delay);
  }

  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightHslSendMessage(elementId, serverAddr, ttl, appKeyIndex, param,
                            (uint8_t)(pCursor - param), opcode);
  }
  else
  {
    mmdlLightHslPublishMessage(elementId, param, (uint8_t)(pCursor - param), opcode);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Saturation Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *  \param[in] ackReq       TRUE if Ack is requested, FALSE otherwise.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightHslClSatSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                 uint16_t appKeyIndex, const mmdlLightHslSatSetParam_t *pParam,
                                 bool_t ackReq)
{
  uint8_t param[MMDL_LIGHT_HSL_SAT_SET_MAX_LEN];
  uint8_t *pCursor = param;
  uint16_t opcode = MMDL_LIGHT_HSL_SAT_SET_NO_ACK_OPCODE;

  if (pParam == NULL)
  {
    return;
  }

  /* Select opcode */
  if (ackReq)
  {
    opcode = MMDL_LIGHT_HSL_SAT_SET_OPCODE;
  }

  /* Build OTA fields */
  UINT16_TO_BSTREAM(pCursor, pParam->saturation);
  UINT8_TO_BSTREAM(pCursor, pParam->tid);

  /* Do not include transition time and delay in the message if it is not used */
  if (pParam->transitionTime != MMDL_GEN_TR_UNKNOWN)
  {
    UINT8_TO_BSTREAM(pCursor, pParam->transitionTime);
    UINT8_TO_BSTREAM(pCursor, pParam->delay);
  }

  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightHslSendMessage(elementId, serverAddr, ttl, appKeyIndex, param,
                            (uint8_t)(pCursor - param), opcode);
  }
  else
  {
    mmdlLightHslPublishMessage(elementId, param, (uint8_t)(pCursor - param), opcode);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Default Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *  \param[in] ackReq       TRUE if Ack is requested, FALSE otherwise.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightHslClDefSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                 uint16_t appKeyIndex, const mmdlLightHslParam_t *pParam,
                                 bool_t ackReq)
{
  uint8_t param[MMDL_LIGHT_HSL_DEF_SET_LEN];
  uint8_t *pCursor = param;
  uint16_t opcode = MMDL_LIGHT_HSL_DEFAULT_SET_NO_ACK_OPCODE;

  if (pParam == NULL)
  {
    return;
  }

  /* Select opcode */
  if (ackReq)
  {
    opcode = MMDL_LIGHT_HSL_DEFAULT_SET_OPCODE;
  }

  /* Build OTA fields */
  UINT16_TO_BSTREAM(pCursor, pParam->lightness);
  UINT16_TO_BSTREAM(pCursor, pParam->hue);
  UINT16_TO_BSTREAM(pCursor, pParam->saturation);

  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightHslSendMessage(elementId, serverAddr, ttl, appKeyIndex, param,
                            (uint8_t)(pCursor - param), opcode);
  }
  else
  {
    mmdlLightHslPublishMessage(elementId, param, (uint8_t)(pCursor - param), opcode);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Range Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *  \param[in] ackReq       TRUE if Ack is requested, FALSE otherwise.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightHslClRangeSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                   uint16_t appKeyIndex, const mmdlLightHslRangeSetParam_t *pParam,
                                   bool_t ackReq)
{
  uint8_t param[MMDL_LIGHT_HSL_RANGE_SET_LEN];
  uint8_t *pCursor = param;
  uint16_t opcode = MMDL_LIGHT_HSL_RANGE_SET_NO_ACK_OPCODE;

  if (pParam == NULL)
  {
    return;
  }

  /* Select opcode */
  if (ackReq)
  {
    opcode = MMDL_LIGHT_HSL_RANGE_SET_OPCODE;
  }

  /* Build OTA fields */
  UINT16_TO_BSTREAM(pCursor, pParam->minHue);
  UINT16_TO_BSTREAM(pCursor, pParam->maxHue);
  UINT16_TO_BSTREAM(pCursor, pParam->minSaturation);
  UINT16_TO_BSTREAM(pCursor, pParam->maxSaturation);

  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightHslSendMessage(elementId, serverAddr, ttl, appKeyIndex, param,
                            (uint8_t)(pCursor - param), opcode);
  }
  else
  {
    mmdlLightHslPublishMessage(elementId, param, (uint8_t)(pCursor - param), opcode);
  }
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Light HSL Client Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClHandlerInit(wsfHandlerId_t handlerId)
{
  /* Set handler ID */
  mmdlLightHslClHandlerId = handlerId;

  /* Initialize control block */
  lightHslClCb.recvCback = MmdlEmptyCback;
}

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Light HSL Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClHandler(wsfMsgHdr_t *pMsg)
{
  uint8_t opcodeIdx, opcodeSize;
  meshModelMsgRecvEvt_t *pModelMsg;

  /* Handle message */
  if (pMsg != NULL)
  {
    switch (pMsg->event)
    {
      case MESH_MODEL_EVT_MSG_RECV:
        pModelMsg = (meshModelMsgRecvEvt_t *)pMsg;

        /* Match the received opcode */
        for (opcodeIdx = 0; opcodeIdx < MMDL_LIGHT_HSL_CL_NUM_RCVD_OPCODES; opcodeIdx++)
        {
          opcodeSize = MESH_OPCODE_SIZE(pModelMsg->opCode);
          if (!memcmp(&mmdlLightHslClRcvdOpcodes[opcodeIdx], pModelMsg->opCode.opcodeBytes,
              opcodeSize))
          {
            /* Process message */
            (void)mmdlLightHslClHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
          }
        }
        break;

      default:
        MMDL_TRACE_WARN0("LIGHT HSL CL: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                    uint16_t appKeyIndex)
{
  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightHslSendMessage(elementId, serverAddr, ttl, appKeyIndex, NULL, 0, MMDL_LIGHT_HSL_GET_OPCODE);
  }
  else
  {
    mmdlLightHslPublishMessage(elementId, NULL, 0, MMDL_LIGHT_HSL_GET_OPCODE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                       uint16_t appKeyIndex, const mmdlLightHslSetParam_t *pParam)
{
  mmdlLightHslClSet(elementId, serverAddr, ttl, appKeyIndex, pParam, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Set Unacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            uint16_t appKeyIndex, const mmdlLightHslSetParam_t *pParam)
{
  mmdlLightHslClSet(elementId, serverAddr, ttl, appKeyIndex, pParam, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Target Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClTargetGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                             uint16_t appKeyIndex)
{
  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightHslSendMessage(elementId, serverAddr, ttl, appKeyIndex, NULL, 0,
                            MMDL_LIGHT_HSL_TARGET_GET_OPCODE);
  }
  else
  {
    mmdlLightHslPublishMessage(elementId, NULL, 0, MMDL_LIGHT_HSL_TARGET_GET_OPCODE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Hue Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClHueGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          uint16_t appKeyIndex)
{
  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightHslSendMessage(elementId, serverAddr, ttl, appKeyIndex, NULL, 0,
                            MMDL_LIGHT_HSL_HUE_GET_OPCODE);
  }
  else
  {
    mmdlLightHslPublishMessage(elementId, NULL, 0, MMDL_LIGHT_HSL_HUE_GET_OPCODE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Hue Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClHueSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          uint16_t appKeyIndex, const mmdlLightHslHueSetParam_t *pParam)
{
  mmdlLightHslClHueSet(elementId, serverAddr, ttl, appKeyIndex, pParam, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Hue Set Unacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClHueSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                               uint16_t appKeyIndex, const mmdlLightHslHueSetParam_t *pParam)
{
  mmdlLightHslClHueSet(elementId, serverAddr, ttl, appKeyIndex, pParam, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Saturation Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClSatGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          uint16_t appKeyIndex)
{
  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightHslSendMessage(elementId, serverAddr, ttl, appKeyIndex, NULL, 0,
                            MMDL_LIGHT_HSL_SAT_GET_OPCODE);
  }
  else
  {
    mmdlLightHslPublishMessage(elementId, NULL, 0, MMDL_LIGHT_HSL_SAT_GET_OPCODE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Saturation Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClSatSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          uint16_t appKeyIndex, const mmdlLightHslSatSetParam_t *pParam)
{
  mmdlLightHslClSatSet(elementId, serverAddr, ttl, appKeyIndex, pParam, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Saturation Set Unacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClSatSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                               uint16_t appKeyIndex, const mmdlLightHslSatSetParam_t *pParam)
{
  mmdlLightHslClSatSet(elementId, serverAddr, ttl, appKeyIndex, pParam, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Default Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClDefGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          uint16_t appKeyIndex)
{
  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightHslSendMessage(elementId, serverAddr, ttl, appKeyIndex, NULL, 0,
                            MMDL_LIGHT_HSL_DEFAULT_GET_OPCODE);
  }
  else
  {
    mmdlLightHslPublishMessage(elementId, NULL, 0, MMDL_LIGHT_HSL_DEFAULT_GET_OPCODE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Default Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClDefSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          uint16_t appKeyIndex, const mmdlLightHslParam_t *pParam)
{
  mmdlLightHslClDefSet(elementId, serverAddr, ttl, appKeyIndex, pParam, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Default Set Unacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClDefSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                               uint16_t appKeyIndex, const mmdlLightHslParam_t *pParam)
{
  mmdlLightHslClDefSet(elementId, serverAddr, ttl, appKeyIndex, pParam, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Range Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClRangeGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          uint16_t appKeyIndex)
{
  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightHslSendMessage(elementId, serverAddr, ttl, appKeyIndex, NULL, 0,
                            MMDL_LIGHT_HSL_RANGE_GET_OPCODE);
  }
  else
  {
    mmdlLightHslPublishMessage(elementId, NULL, 0, MMDL_LIGHT_HSL_RANGE_GET_OPCODE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Range Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClRangeSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          uint16_t appKeyIndex, const mmdlLightHslRangeSetParam_t *pParam)
{
  mmdlLightHslClRangeSet(elementId, serverAddr, ttl, appKeyIndex, pParam, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light HSL Range Set Unacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslClRangeSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                               uint16_t appKeyIndex, const mmdlLightHslRangeSetParam_t *pParam)
{
  mmdlLightHslClRangeSet(elementId, serverAddr, ttl, appKeyIndex, pParam, FALSE);
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
void MmdlLightHslClRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback */
  if (recvCback != NULL)
  {
    lightHslClCb.recvCback = recvCback;
  }
}
