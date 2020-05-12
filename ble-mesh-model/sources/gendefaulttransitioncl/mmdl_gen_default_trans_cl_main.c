/*************************************************************************************************/
/*!
 *  \file   mmdl_gen_default_trans_cl_main.c
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
#include "mmdl_gen_default_trans_cl_api.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Generic Default Transition control block type definition */
typedef struct mmdlGenDefaultTransClCb_tag
{
  mmdlEventCback_t recvCback;    /*!< Model Generic Default Transition received callback */
} mmdlGenDefaultTransClCb_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler id */
wsfHandlerId_t mmdlGenDefaultTransClHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlGenDefaultTransClRcvdOpcodes[] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_DEFAULT_TRANS_STATUS_OPCODE)} }
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Level Client control block */
static mmdlGenDefaultTransClCb_t  defaultTransClCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Sends a GenDefaultTransSet message to the destination address.
 *
 *  \param[in] pMsgInfo     Pointer to a Mesh message information structure. See ::meshMsgInfo_t.
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenDefaultTransSendSet(uint16_t opcode, meshElementId_t elementId,
                                       meshAddress_t serverAddr, uint8_t ttl,
                                       const mmdlGenDefaultTransSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_DEFAULT_TRANS_CL_MDL_ID,
                                   MMDL_GEN_DEFAULT_TRANS_SET_NO_ACK_OPCODE);
  uint8_t paramLen = MMDL_GEN_DEFAULT_TRANS_MSG_LENGTH;
  uint8_t paramMsg[MMDL_GEN_DEFAULT_TRANS_MSG_LENGTH];

  if (pSetParam != NULL)
  {
    /* Fill in the message information */
    msgInfo.elementId = elementId;
    msgInfo.dstAddr = serverAddr;
    msgInfo.ttl = ttl;
    msgInfo.appKeyIndex = appKeyIndex;

    UINT16_TO_BE_BUF(msgInfo.opcode.opcodeBytes, opcode);

    /* Build param message. */
    paramMsg[0] = pSetParam->state;

    /* Send message to the Mesh Core */
    MeshSendMessage(&msgInfo, paramMsg, paramLen, 0, 0);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publishes a Generic Default Trans message to the publication address.
 *
 *  \param[in] opcode     Opcode.
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] pSetParam  Pointer to structure containing the set parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenDefaultTransPublishSet(uint16_t opcode, meshElementId_t elementId,
                                          const mmdlGenDefaultTransSetParam_t *pSetParam)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_GEN_DEFAULT_TRANS_CL_MDL_ID,
                                             MMDL_GEN_DEFAULT_TRANS_SET_NO_ACK_OPCODE);
  uint8_t paramLen = MMDL_GEN_DEFAULT_TRANS_MSG_LENGTH;
  uint8_t paramMsg[MMDL_GEN_DEFAULT_TRANS_MSG_LENGTH];

  /* Fill in the msg info parameters */
  pubMsgInfo.elementId = elementId;
  UINT16_TO_BE_BUF(pubMsgInfo.opcode.opcodeBytes, opcode);

  /* Build param message. */
  paramMsg[0] = pSetParam->state;

  /* Send message to the Mesh Core. Parameters are already stored in over-the-air order */
  MeshPublishMessage(&pubMsgInfo, paramMsg, paramLen);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Default Transition Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenDefaultTransClHandleStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlGenDefaultTransClStatusEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_GEN_DEFAULT_TRANS_MSG_LENGTH)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_GEN_DEFAULT_TRANS_CL_EVENT;
  event.hdr.param = MMDL_GEN_DEFAULT_TRANS_CL_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  pParams = pMsg->pMessageParams;

  /* Extract status event parameters */
  event.state = pParams[0];

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  defaultTransClCb.recvCback((wsfMsgHdr_t *)&event);
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
void MmdlGenDefaultTransClHandlerInit(wsfHandlerId_t handlerId)
{
  /* Set handler ID */
  mmdlGenDefaultTransClHandlerId = handlerId;

  /* Initialize control block */
  defaultTransClCb.recvCback = MmdlEmptyCback;
}

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Default Transition Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenDefaultTransClHandler(wsfMsgHdr_t *pMsg)
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
        if (MESH_OPCODE_SIZE(pModelMsg->opCode) == MMDL_GEN_DEFAULT_TRANS_OPCODES_SIZE &&
            !memcmp(&mmdlGenDefaultTransClRcvdOpcodes[0], pModelMsg->opCode.opcodeBytes,
                    MMDL_GEN_DEFAULT_TRANS_OPCODES_SIZE))
        {
          /* Process Status message */
          mmdlGenDefaultTransClHandleStatus(pModelMsg);
        }
        break;

      default:
        MMDL_TRACE_WARN0("GEN DEFAULT TRANS CL: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a GenDefaultTransGet message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenDefaultTransClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                              uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_DEFAULT_TRANS_CL_MDL_ID,
                                   MMDL_GEN_DEFAULT_TRANS_GET_OPCODE);
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_GEN_DEFAULT_TRANS_CL_MDL_ID,
                                             MMDL_GEN_DEFAULT_TRANS_GET_OPCODE);

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
 *  \brief     Send a GenDefaultTransSet message to the destination address.
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
void MmdlGenDefaultTransClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                              const mmdlGenDefaultTransSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  if (serverAddr == MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlGenDefaultTransPublishSet(MMDL_GEN_DEFAULT_TRANS_SET_OPCODE, elementId, pSetParam);
  }
  else
  {
    mmdlGenDefaultTransSendSet(MMDL_GEN_DEFAULT_TRANS_SET_OPCODE, elementId, serverAddr, ttl, pSetParam,
                               appKeyIndex);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a GenDefaultTransSetUnacknowledged message to the destination address.
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
void MmdlGenDefaultTransClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                   const mmdlGenDefaultTransSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  if (serverAddr == MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlGenDefaultTransPublishSet(MMDL_GEN_DEFAULT_TRANS_SET_NO_ACK_OPCODE, elementId, pSetParam);
  }
  else
  {
    mmdlGenDefaultTransSendSet(MMDL_GEN_DEFAULT_TRANS_SET_NO_ACK_OPCODE, elementId, serverAddr, ttl, pSetParam,
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
void MmdlGenDefaultTransClRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback*/
  if (recvCback != NULL)
  {
    defaultTransClCb.recvCback = recvCback;
  }
}
