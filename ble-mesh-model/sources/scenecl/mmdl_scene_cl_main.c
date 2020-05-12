/*************************************************************************************************/
/*!
 *  \file   mmdl_scene_cl_main.c
 *
 *  \brief  Implementation of the Scenes Client model.
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
#include "wsf_buf.h"
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"

#include "mmdl_types.h"
#include "mmdl_common.h"
#include "mmdl_scene_cl_api.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Scenes Client control block type definition */
typedef struct mmdlSceneClCb_tag
{
  mmdlEventCback_t recvCback;    /*!< Model received callback */
} mmdlSceneClCb_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler id */
wsfHandlerId_t mmdlSceneClHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlSceneClRcvdOpcodes[MMDL_SCENE_CL_NUM_RCVD_OPCODES] =
{
  { {UINT8_OPCODE_TO_BYTES(MMDL_SCENE_STATUS_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_SCENE_REGISTER_STATUS_OPCODE)} }
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Scene Client control block */
static mmdlSceneClCb_t  sceneClCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Sends a Scene Client message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the message parameters.
 *  \param[in] paramLen     Length of message parameters structure.
 *  \param[in] opcode       Scene Client message opcode.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlSceneSendMessage(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                 uint16_t appKeyIndex, const uint8_t *pParam, uint8_t paramLen,
                                 uint16_t opcode)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_SCENE_CL_MDL_ID, opcode);

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
 *  \brief     Publishes a Scene message to the publication address.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] pParam     Pointer to structure containing the parameters.
 *  \param[in] paramLen   Length of the parameters structure.
 *  \param[in] opcode     Command opcode.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlScenePublishMessage(meshElementId_t elementId, const uint8_t *pParam,
                                     uint8_t paramLen, uint16_t opcode)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_SCENE_CL_MDL_ID, opcode);

  /* Fill in the msg info parameters */
  pubMsgInfo.elementId = elementId;

  /* Send message to the Mesh Core. Parameters are already stored in over-the-air order */
  MeshPublishMessage(&pubMsgInfo, (uint8_t *)pParam, paramLen);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Scene Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlSceneClHandleStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlSceneClStatusEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_SCENE_STATUS_MAX_LEN &&
      pMsg->messageParamsLen != MMDL_SCENE_STATUS_MIN_LEN)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_SCENE_CL_EVENT;
  event.hdr.param = MMDL_SCENE_CL_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  /* Extract status event parameters */
  pParams = pMsg->pMessageParams;
  BSTREAM_TO_UINT8(event.status, pParams);

  if (event.status >= MMDL_SCENE_PROHIBITED)
  {
    return;
  }

  /* Extract current scene*/
  BSTREAM_TO_UINT16(event.currentScene, pParams);

  /* Check if optional parameters are present */
  if (pMsg->messageParamsLen == MMDL_SCENE_STATUS_MAX_LEN)
  {
    /* Extract target scene and remaining time */
    BSTREAM_TO_UINT16(event.targetScene, pParams);

    /* Extract target state */
    BSTREAM_TO_UINT8(event.remainingTime, pParams);
  }
  else
  {
    event.targetScene = 0;
    event.remainingTime = 0;
  }

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  sceneClCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Scene Register Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlSceneClHandleRegisterStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlSceneClRegStatusEvent_t *pEvent;
  uint8_t *pParams;
  mmdlSceneStatus_t status;
  uint8_t sceneCount, sceneIdx;

  /* Validate message length */
  if (pMsg->messageParamsLen < MMDL_SCENE_REG_STATUS_MIN_LEN ||
      pMsg->messageParamsLen > MMDL_SCENE_REG_STATUS_MAX_LEN ||
      (pMsg->messageParamsLen % 2 == 0))
  {
    return;
  }

  sceneCount = (pMsg->messageParamsLen - MMDL_SCENE_REG_STATUS_MIN_LEN) / sizeof(uint16_t);

  /* Extract status event parameters */
  pParams = pMsg->pMessageParams;
  BSTREAM_TO_UINT8(status, pParams);

  if (status >= MMDL_SCENE_PROHIBITED)
  {
    return;
  }

  /* Allocate memory for the event and number of scenes */
  pEvent = WsfBufAlloc((sceneCount - 1) * sizeof(uint16_t) + sizeof(mmdlSceneClRegStatusEvent_t));

  if (pEvent == NULL)
  {
    return;
  }

  /* Set event type and status */
  pEvent->hdr.event = MMDL_SCENE_CL_EVENT;
  pEvent->hdr.param = MMDL_SCENE_CL_REG_STATUS_EVENT;
  pEvent->hdr.status = MMDL_SUCCESS;

  /* Set status code */
  pEvent->status = status;

  /* Extract current scene*/
  BSTREAM_TO_UINT16(pEvent->currentScene, pParams);
  pEvent->scenesCount = sceneCount;

  /* Check if optional parameters are present */
  if (pMsg->messageParamsLen > MMDL_SCENE_STATUS_MIN_LEN)
  {
    /* Extract scenes */
    for (sceneIdx = 0; sceneIdx < sceneCount; sceneIdx++)
    {
      BSTREAM_TO_UINT16(pEvent->scenes[sceneIdx], pParams);
    }
  }

  /* Set event contents */
  pEvent->elementId = pMsg->elementId;
  pEvent->serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  sceneClCb.recvCback((wsfMsgHdr_t *)pEvent);

  WsfBufFree(pEvent);
}

/**************************************************************************************************
 Global Function
 **************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Scene Client Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneClHandlerInit(wsfHandlerId_t handlerId)
{
  /* Set handler ID */
  mmdlSceneClHandlerId = handlerId;

  /* Initialize control block */
  sceneClCb.recvCback = MmdlEmptyCback;
}

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Scene Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneClHandler(wsfMsgHdr_t *pMsg)
{
  meshModelMsgRecvEvt_t *pModelMsg;

  /* Handle message */
  if (pMsg != NULL)
  {
    switch (pMsg->event)
    {
      case MESH_MODEL_EVT_MSG_RECV:
        pModelMsg = (meshModelMsgRecvEvt_t *)pMsg;

        if (MESH_OPCODE_IS_SIZE_ONE(pModelMsg->opCode) &&
            mmdlSceneClRcvdOpcodes[0].opcodeBytes[0] == pModelMsg->opCode.opcodeBytes[0])
        {
          /* Process Status message */
          mmdlSceneClHandleStatus(pModelMsg);
        }

        /* Validate opcode size and value */
        if (MESH_OPCODE_SIZE(pModelMsg->opCode) == MMDL_SCENE_OPCODES_SIZE &&
            !memcmp(&mmdlSceneClRcvdOpcodes[1], pModelMsg->opCode.opcodeBytes,
                    MMDL_SCENE_OPCODES_SIZE))
        {
          /* Process Register Status message */
          mmdlSceneClHandleRegisterStatus(pModelMsg);
        }
        break;

      default:
        MMDL_TRACE_WARN0("GEN SCENE CL: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Scene Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                    uint16_t appKeyIndex)
{
  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlSceneSendMessage(elementId, serverAddr, ttl, appKeyIndex, NULL, 0, MMDL_SCENE_GET_OPCODE);
  }
  else
  {
    mmdlScenePublishMessage(elementId, NULL, 0, MMDL_SCENE_GET_OPCODE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Scene Register Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneClRegisterGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            uint16_t appKeyIndex)
{
  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlSceneSendMessage(elementId, serverAddr, ttl, appKeyIndex, NULL, 0,
                         MMDL_SCENE_REGISTER_GET_OPCODE);
  }
  else
  {
    mmdlScenePublishMessage(elementId, NULL, 0, MMDL_SCENE_REGISTER_GET_OPCODE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Scene Store message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] sceneNumber  Scene Number. Value 0x0000 is prohibited.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneClStore(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                      uint16_t appKeyIndex, mmdlSceneNumber_t sceneNumber)
{
  uint8_t param[MMDL_SCENE_STORE_LEN];

  UINT16_TO_BUF(param, sceneNumber);

  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlSceneSendMessage(elementId, serverAddr, ttl, appKeyIndex, param, MMDL_SCENE_STORE_LEN,
                         MMDL_SCENE_STORE_OPCODE);
  }
  else
  {
    mmdlScenePublishMessage(elementId, param, MMDL_SCENE_STORE_LEN, MMDL_SCENE_STORE_OPCODE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Scene Store Unacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] sceneNumber  Scene Number. Value 0x0000 is prohibited.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneClStoreNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                           uint16_t appKeyIndex, mmdlSceneNumber_t sceneNumber)
{
  uint8_t param[MMDL_SCENE_STORE_LEN];

  UINT16_TO_BUF(param, sceneNumber);

  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlSceneSendMessage(elementId, serverAddr, ttl, appKeyIndex, param, MMDL_SCENE_STORE_LEN,
                         MMDL_SCENE_STORE_NO_ACK_OPCODE);
  }
  else
  {
    mmdlScenePublishMessage(elementId, param, MMDL_SCENE_STORE_LEN, MMDL_SCENE_STORE_NO_ACK_OPCODE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Scene Recall message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] pParam       Pointer to structure containing the recall parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneClRecall(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                       uint16_t appKeyIndex, const mmdlSceneRecallParam_t *pParam)
{
  uint8_t param[MMDL_SCENE_RECALL_MAX_LEN];
  uint8_t *pCursor = param;

  if (pParam == NULL || pParam->sceneNum == 0)
  {
    return;
  }

  UINT16_TO_BSTREAM(pCursor, pParam->sceneNum);
  UINT8_TO_BSTREAM(pCursor, pParam->tid);

  /* Do not include transition time and delay in the message if it is not used */
  if (pParam->transitionTime != MMDL_GEN_TR_UNKNOWN)
  {
    UINT8_TO_BSTREAM(pCursor, pParam->transitionTime);
    UINT8_TO_BSTREAM(pCursor, pParam->delay);
  }

  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlSceneSendMessage(elementId, serverAddr, ttl, appKeyIndex, param, (uint8_t)(pCursor - param),
                         MMDL_SCENE_RECALL_OPCODE);
  }
  else
  {
    mmdlScenePublishMessage(elementId, param, (uint8_t)(pCursor - param),
                            MMDL_SCENE_RECALL_OPCODE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Scene Recall Unacknowledged message to the destination address.
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
void MmdlSceneClRecallNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            uint16_t appKeyIndex, const mmdlSceneRecallParam_t *pParam)
{
  uint8_t param[MMDL_SCENE_RECALL_MAX_LEN];
  uint8_t *pCursor = param;

  if (pParam == NULL || pParam->sceneNum == 0)
  {
    return;
  }

  UINT16_TO_BSTREAM(pCursor, pParam->sceneNum);
  UINT8_TO_BSTREAM(pCursor, pParam->tid);

  /* Do not include transition time and delay in the message if it is not used */
  if (pParam->transitionTime != MMDL_GEN_TR_UNKNOWN)
  {
    UINT8_TO_BSTREAM(pCursor, pParam->transitionTime);
    UINT8_TO_BSTREAM(pCursor, pParam->delay);
  }

  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlSceneSendMessage(elementId, serverAddr, ttl, appKeyIndex, param, (uint8_t)(pCursor - param),
                         MMDL_SCENE_RECALL_NO_ACK_OPCODE);
  }
  else
  {
    mmdlScenePublishMessage(elementId, param, (uint8_t)(pCursor - param),
                            MMDL_SCENE_RECALL_NO_ACK_OPCODE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Scene Delete message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] sceneNumber  Scene Number. Value 0x0000 is prohibited.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneClDelete(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                      uint16_t appKeyIndex, mmdlSceneNumber_t sceneNumber)
{
  uint8_t param[MMDL_SCENE_DELETE_LEN];

  UINT16_TO_BUF(param, sceneNumber);

  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlSceneSendMessage(elementId, serverAddr, ttl, appKeyIndex, param, MMDL_SCENE_DELETE_LEN,
                         MMDL_SCENE_DELETE_OPCODE);
  }
  else
  {
    mmdlScenePublishMessage(elementId, param, MMDL_SCENE_DELETE_LEN, MMDL_SCENE_DELETE_OPCODE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Scene Delete Unacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] sceneNumber  Scene Number. Value 0x0000 is prohibited.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneClDeleteNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                           uint16_t appKeyIndex, mmdlSceneNumber_t sceneNumber)
{
  uint8_t param[MMDL_SCENE_DELETE_LEN];

  UINT16_TO_BUF(param, sceneNumber);

  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlSceneSendMessage(elementId, serverAddr, ttl, appKeyIndex, param, MMDL_SCENE_DELETE_LEN,
                         MMDL_SCENE_DELETE_NO_ACK_OPCODE);
  }
  else
  {
    mmdlScenePublishMessage(elementId, param, MMDL_SCENE_DELETE_LEN, MMDL_SCENE_DELETE_NO_ACK_OPCODE);
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
void MmdlSceneClRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback */
  if (recvCback != NULL)
  {
    sceneClCb.recvCback = recvCback;
  }
}
