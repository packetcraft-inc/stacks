/*************************************************************************************************/
/*!
 *  \file   mmdl_light_ctl_cl_main.c
 *
 *  \brief  Implementation of the Light CTL Client model.
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
#include "mmdl_light_ctl_cl_api.h"
#include "mmdl_light_ctl_cl_main.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Light CTL Client control block type definition */
typedef struct mmdlLightCtlClCb_tag
{
  mmdlEventCback_t recvCback;    /*!< Model received callback */
}mmdlLightCtlClCb_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler id */
wsfHandlerId_t mmdlLightCtlClHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlLightCtlClRcvdOpcodes[MMDL_LIGHT_CTL_CL_NUM_RCVD_OPCODES] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_CTL_STATUS_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_CTL_RANGE_STATUS_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_CTL_TEMP_STATUS_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_LIGHT_CTL_DEFAULT_STATUS_OPCODE)} }
};

/*! Light CTL Client message handler type definition */
typedef void (*mmdlLightCtlClHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlLightCtlClHandleMsg_t mmdlLightCtlClHandleMsg[MMDL_LIGHT_CTL_CL_NUM_RCVD_OPCODES] =
{
  mmdlLightCtlClHandleStatus,
  mmdlLightCtlClHandleRangeStatus,
  mmdlLightCtlClHandleTemperatureStatus,
  mmdlLightCtlClHandleDefStatus
};

/*! Light CTL Client control block */
static mmdlLightCtlClCb_t  lightCtlClCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light CTL Client message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the message parameters.
 *  \param[in] paramLen     Length of message parameters structure.
 *  \param[in] opcode       Light CTL Client message opcode.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightCtlSendMessage(meshElementId_t elementId, meshAddress_t serverAddr,
                                    uint8_t ttl, uint16_t appKeyIndex, const uint8_t *pParam,
                                    uint8_t paramLen, uint16_t opcode)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_LIGHT_CTL_CL_MDL_ID, opcode);

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
 *  \brief     Publishes a Light CTL message to the publication address.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] pParam     Pointer to structure containing the parameters.
 *  \param[in] paramLen   Length of the parameters structure.
 *  \param[in] opcode     Command opcode.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlLightCtlPublishMessage(meshElementId_t elementId, const uint8_t *pParam,
                                       uint8_t paramLen, uint16_t opcode)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_LIGHT_CTL_CL_MDL_ID, opcode);

  /* Fill in the msg info parameters */
  pubMsgInfo.elementId = elementId;

  /* Send message to the Mesh Core. Parameters are already stored in over-the-air order */
  MeshPublishMessage(&pubMsgInfo, (uint8_t *)pParam, paramLen);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light CTL Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightCtlClHandleStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlLightCtlClStatusEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if ((pMsg->messageParamsLen != MMDL_LIGHT_CTL_STATUS_MAX_LEN) &&
      (pMsg->messageParamsLen != MMDL_LIGHT_CTL_STATUS_MIN_LEN))
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_LIGHT_CTL_CL_EVENT;
  event.hdr.param = MMDL_LIGHT_CTL_CL_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  /* Extract status event parameters */
  pParams = pMsg->pMessageParams;
  BSTREAM_TO_UINT16(event.presentLightness, pParams);
  BSTREAM_TO_UINT16(event.presentTemperature, pParams);

  /* Check if optional parameters are present */
  if (pMsg->messageParamsLen == MMDL_LIGHT_CTL_STATUS_MAX_LEN)
  {
    BSTREAM_TO_UINT16(event.targetLightness, pParams);
    BSTREAM_TO_UINT16(event.targetTemperature, pParams);
    BSTREAM_TO_UINT8(event.remainingTime, pParams);
  }
  else
  {
    event.targetLightness = 0;
    event.targetTemperature = 0;
    event.remainingTime = 0;
  }

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  lightCtlClCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light CTL Temperature Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightCtlClHandleTemperatureStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlLightCtlClTemperatureStatusEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_LIGHT_CTL_TEMP_STATUS_MAX_LEN &&
      pMsg->messageParamsLen != MMDL_LIGHT_CTL_TEMP_STATUS_MIN_LEN)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_LIGHT_CTL_CL_EVENT;
  event.hdr.param = MMDL_LIGHT_CTL_CL_TEMP_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  /* Extract status event parameters */
  pParams = pMsg->pMessageParams;
  BSTREAM_TO_UINT16(event.presentTemperature, pParams);
  BSTREAM_TO_UINT16(event.presentDeltaUV, pParams);

  /* Check if optional parameters are present */
  if (pMsg->messageParamsLen == MMDL_LIGHT_CTL_TEMP_STATUS_MAX_LEN)
  {
    /* Extract target state */
    BSTREAM_TO_UINT16(event.targetTemperature, pParams);
    BSTREAM_TO_UINT16(event.targetDeltaUV, pParams);
    BSTREAM_TO_UINT8(event.remainingTime, pParams);
  }
  else
  {
    event.remainingTime = 0;
    event.targetTemperature = 0;
    event.targetDeltaUV = 0;
  }

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  lightCtlClCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light CTL Default Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightCtlClHandleDefStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlLightCtlClDefStatusEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_LIGHT_CTL_DEFAULT_STATUS_LEN)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_LIGHT_CTL_CL_EVENT;
  event.hdr.param = MMDL_LIGHT_CTL_DEFAULT_STATUS_LEN;
  event.hdr.status = MMDL_SUCCESS;

  /* Extract status event parameters */
  pParams = pMsg->pMessageParams;
  BSTREAM_TO_UINT16(event.lightness, pParams);
  BSTREAM_TO_UINT16(event.temperature, pParams);
  BSTREAM_TO_UINT16(event.deltaUV, pParams);

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  lightCtlClCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light CTL Range Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightCtlClHandleRangeStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlLightCtlClRangeStatusEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_LIGHT_CTL_TEMP_RANGE_STATUS_LEN)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_LIGHT_CTL_CL_EVENT;
  event.hdr.param = MMDL_LIGHT_CTL_CL_RANGE_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  /* Extract status event parameters */
  pParams = pMsg->pMessageParams;
  BSTREAM_TO_UINT8(event.opStatus, pParams);
  BSTREAM_TO_UINT16(event.minTemperature, pParams);
  BSTREAM_TO_UINT16(event.maxTemperature, pParams);

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  lightCtlClCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Set message to the destination address.
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
static void mmdlLightCtlClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                              uint16_t appKeyIndex, const mmdlLightCtlSetParam_t *pParam,
                              bool_t ackReq)
{
  uint8_t param[MMDL_LIGHT_CTL_SET_MAX_LEN];
  uint8_t *pCursor = param;
  uint16_t opcode = MMDL_LIGHT_CTL_SET_NO_ACK_OPCODE;

  if (pParam == NULL)
  {
    return;
  }

  /* Select opcode */
  if (ackReq)
  {
    opcode = MMDL_LIGHT_CTL_SET_OPCODE;
  }

  /* Build OTA fields */
  UINT16_TO_BSTREAM(pCursor, pParam->lightness);
  UINT16_TO_BSTREAM(pCursor, pParam->temperature);
  UINT16_TO_BSTREAM(pCursor, pParam->deltaUV);
  UINT8_TO_BSTREAM(pCursor, pParam->tid);

  /* Do not include transition time and delay in the message if it is not used */
  if (pParam->transitionTime != MMDL_GEN_TR_UNKNOWN)
  {
    UINT8_TO_BSTREAM(pCursor, pParam->transitionTime);
    UINT8_TO_BSTREAM(pCursor, pParam->delay);
  }

  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightCtlSendMessage(elementId, serverAddr, ttl, appKeyIndex, param,
                            (uint8_t)(pCursor - param), opcode);
  }
  else
  {
    mmdlLightCtlPublishMessage(elementId, param, (uint8_t)(pCursor - param), opcode);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Temperature Set message to the destination address.
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
static void mmdlLightCtlClTemperatureSet(meshElementId_t elementId, meshAddress_t serverAddr,
                                         uint8_t ttl, uint16_t appKeyIndex,
                                         const mmdlLightCtlTemperatureSetParam_t *pParam,
                                         bool_t ackReq)
{
  uint8_t param[MMDL_LIGHT_CTL_TEMP_SET_MAX_LEN];
  uint8_t *pCursor = param;
  uint16_t opcode = MMDL_LIGHT_CTL_TEMP_SET_NO_ACK_OPCODE;

  if (pParam == NULL)
  {
    return;
  }

  /* Select opcode */
  if (ackReq)
  {
    opcode = MMDL_LIGHT_CTL_TEMP_SET_OPCODE;
  }

  /* Build OTA fields */
  UINT16_TO_BSTREAM(pCursor, pParam->temperature);
  UINT16_TO_BSTREAM(pCursor, pParam->deltaUV);
  UINT8_TO_BSTREAM(pCursor, pParam->tid);

  /* Do not include transition time and delay in the message if it is not used */
  if (pParam->transitionTime != MMDL_GEN_TR_UNKNOWN)
  {
    UINT8_TO_BSTREAM(pCursor, pParam->transitionTime);
    UINT8_TO_BSTREAM(pCursor, pParam->delay);
  }

  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightCtlSendMessage(elementId, serverAddr, ttl, appKeyIndex, param,
                            (uint8_t)(pCursor - param), opcode);
  }
  else
  {
    mmdlLightCtlPublishMessage(elementId, param, (uint8_t)(pCursor - param), opcode);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Default Set message to the destination address.
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
static void mmdlLightCtlClDefSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                 uint16_t appKeyIndex, const mmdlLightCtlParam_t *pParam,
                                 bool_t ackReq)
{
  uint8_t param[MMDL_LIGHT_CTL_DEFAULT_SET_LEN];
  uint8_t *pCursor = param;
  uint16_t opcode = MMDL_LIGHT_CTL_DEFAULT_SET_NO_ACK_OPCODE;

  if (pParam == NULL)
  {
    return;
  }

  /* Select opcode */
  if (ackReq)
  {
    opcode = MMDL_LIGHT_CTL_DEFAULT_SET_OPCODE;
  }

  /* Build OTA fields */
  UINT16_TO_BSTREAM(pCursor, pParam->lightness);
  UINT16_TO_BSTREAM(pCursor, pParam->temperature);
  UINT16_TO_BSTREAM(pCursor, pParam->deltaUV);

  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightCtlSendMessage(elementId, serverAddr, ttl, appKeyIndex, param,
                            (uint8_t)(pCursor - param), opcode);
  }
  else
  {
    mmdlLightCtlPublishMessage(elementId, param, (uint8_t)(pCursor - param), opcode);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Range Set message to the destination address.
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
static void mmdlLightCtlClRangeSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                   uint16_t appKeyIndex, const mmdlLightCtlRangeSetParam_t *pParam,
                                   bool_t ackReq)
{
  uint8_t param[MMDL_LIGHT_CTL_TEMP_RANGE_SET_LEN];
  uint8_t *pCursor = param;
  uint16_t opcode = MMDL_LIGHT_CTL_TEMP_RANGE_SET_NO_ACK_OPCODE;

  if (pParam == NULL)
  {
    return;
  }

  /* Select opcode */
  if (ackReq)
  {
    opcode = MMDL_LIGHT_CTL_TEMP_RANGE_SET_OPCODE;
  }

  /* Build OTA fields */
  UINT16_TO_BSTREAM(pCursor, pParam->minTemperature);
  UINT16_TO_BSTREAM(pCursor, pParam->maxTemperature);

  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightCtlSendMessage(elementId, serverAddr, ttl, appKeyIndex, param,
                            (uint8_t)(pCursor - param), opcode);
  }
  else
  {
    mmdlLightCtlPublishMessage(elementId, param, (uint8_t)(pCursor - param), opcode);
  }
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Light CTL Client Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlClHandlerInit(wsfHandlerId_t handlerId)
{
  /* Set handler ID */
  mmdlLightCtlClHandlerId = handlerId;

  /* Initialize control block */
  lightCtlClCb.recvCback = MmdlEmptyCback;
}

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Light CTL Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlClHandler(wsfMsgHdr_t *pMsg)
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
        for (opcodeIdx = 0; opcodeIdx < MMDL_LIGHT_CTL_CL_NUM_RCVD_OPCODES; opcodeIdx++)
        {
          opcodeSize = MESH_OPCODE_SIZE(pModelMsg->opCode);
          if (!memcmp(&mmdlLightCtlClRcvdOpcodes[opcodeIdx], pModelMsg->opCode.opcodeBytes,
              opcodeSize))
          {
            /* Process message */
            (void)mmdlLightCtlClHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
          }
        }
        break;

      default:
        MMDL_TRACE_WARN0("LIGHT CTL CL: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                    uint16_t appKeyIndex)
{
  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightCtlSendMessage(elementId, serverAddr, ttl, appKeyIndex, NULL, 0,
                            MMDL_LIGHT_CTL_GET_OPCODE);
  }
  else
  {
    mmdlLightCtlPublishMessage(elementId, NULL, 0, MMDL_LIGHT_CTL_GET_OPCODE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Set message to the destination address.
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
void MmdlLightCtlClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                       uint16_t appKeyIndex, const mmdlLightCtlSetParam_t *pParam)
{
  mmdlLightCtlClSet(elementId, serverAddr, ttl, appKeyIndex, pParam, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Set Unacknowledged message to the destination address.
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
void MmdlLightCtlClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            uint16_t appKeyIndex, const mmdlLightCtlSetParam_t *pParam)
{
  mmdlLightCtlClSet(elementId, serverAddr, ttl, appKeyIndex, pParam, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Temperature Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlClTemperatureGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          uint16_t appKeyIndex)
{
  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightCtlSendMessage(elementId, serverAddr, ttl, appKeyIndex, NULL, 0,
                            MMDL_LIGHT_CTL_TEMP_GET_OPCODE);
  }
  else
  {
    mmdlLightCtlPublishMessage(elementId, NULL, 0, MMDL_LIGHT_CTL_TEMP_GET_OPCODE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Temperature Set message to the destination address.
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
void MmdlLightCtlClTemperatureSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                  uint16_t appKeyIndex,
                                  const mmdlLightCtlTemperatureSetParam_t *pParam)
{
  mmdlLightCtlClTemperatureSet(elementId, serverAddr, ttl, appKeyIndex, pParam, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Temperature Set Unacknowledged message to the destination address.
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
void MmdlLightCtlClTemperatureSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr,
                                       uint8_t ttl, uint16_t appKeyIndex,
                                       const mmdlLightCtlTemperatureSetParam_t *pParam)
{
  mmdlLightCtlClTemperatureSet(elementId, serverAddr, ttl, appKeyIndex, pParam, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Default Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlClDefGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          uint16_t appKeyIndex)
{
  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightCtlSendMessage(elementId, serverAddr, ttl, appKeyIndex, NULL, 0,
                            MMDL_LIGHT_CTL_DEFAULT_GET_OPCODE);
  }
  else
  {
    mmdlLightCtlPublishMessage(elementId, NULL, 0, MMDL_LIGHT_CTL_DEFAULT_GET_OPCODE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Default Set message to the destination address.
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
void MmdlLightCtlClDefSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          uint16_t appKeyIndex, const mmdlLightCtlParam_t *pParam)
{
  mmdlLightCtlClDefSet(elementId, serverAddr, ttl, appKeyIndex, pParam, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Default Set Unacknowledged message to the destination address.
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
void MmdlLightCtlClDefSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                               uint16_t appKeyIndex, const mmdlLightCtlParam_t *pParam)
{
  mmdlLightCtlClDefSet(elementId, serverAddr, ttl, appKeyIndex, pParam, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Range Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlClRangeGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            uint16_t appKeyIndex)
{
  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlLightCtlSendMessage(elementId, serverAddr, ttl, appKeyIndex, NULL, 0,
                            MMDL_LIGHT_CTL_TEMP_RANGE_GET_OPCODE);
  }
  else
  {
    mmdlLightCtlPublishMessage(elementId, NULL, 0, MMDL_LIGHT_CTL_TEMP_RANGE_GET_OPCODE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Range Set message to the destination address.
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
void MmdlLightCtlClRangeSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            uint16_t appKeyIndex, const mmdlLightCtlRangeSetParam_t *pParam)
{
  mmdlLightCtlClRangeSet(elementId, serverAddr, ttl, appKeyIndex, pParam, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Light CTL Range Set Unacknowledged message to the destination address.
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
void MmdlLightCtlClRangeSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                 uint16_t appKeyIndex, const mmdlLightCtlRangeSetParam_t *pParam)
{
  mmdlLightCtlClRangeSet(elementId, serverAddr, ttl, appKeyIndex, pParam, FALSE);
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
void MmdlLightCtlClRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback */
  if (recvCback != NULL)
  {
    lightCtlClCb.recvCback = recvCback;
  }
}
