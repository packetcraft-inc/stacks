/*************************************************************************************************/
/*!
 *  \file   mmdl_gen_powonoff_cl_main.c
 *
 *  \brief  Implementation of the Generic Power On Off Client model.
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
#include "mmdl_gen_powonoff_cl_api.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Generic Power On Off Client control block type definition */
typedef struct mmdlGenPowOnOffClCb_tag
{
  mmdlEventCback_t recvCback;    /*!< Model Generic Power OnOff received callback */
} mmdlGenPowOnOffClCb_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler id */
wsfHandlerId_t mmdlGenPowOnOffClHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlGenPowOnOffClRcvdOpcodes[] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_ONPOWERUP_STATUS_OPCODE)} }
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Power OnOff Client control block */
static mmdlGenPowOnOffClCb_t  powOnOffClCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Sends a GenOnPowerUpSet message to the destination address.
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
static void mmdlGenPowOnOffSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                               const mmdlGenPowOnOffSetParam_t *pSetParam, uint16_t appKeyIndex,
                               bool_t ackRequired)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_POWER_ONOFF_CL_MDL_ID,
                                   MMDL_GEN_ONPOWERUP_SET_NO_ACK_OPCODE);

  if (pSetParam != NULL)
  {
    if (pSetParam->state < MMDL_GEN_ONPOWERUP_STATE_PROHIBITED)
    {
      /* Change to acknowledged set */
      if (ackRequired)
      {
        UINT16_TO_BE_BUF(msgInfo.opcode.opcodeBytes, MMDL_GEN_ONPOWERUP_SET_OPCODE);
      }
      /* Fill in the message information */
      msgInfo.elementId = elementId;
      msgInfo.dstAddr = serverAddr;
      msgInfo.ttl = ttl;
      msgInfo.appKeyIndex = appKeyIndex;

      /* Send message to the Mesh Core */
      MeshSendMessage(&msgInfo, &pSetParam->state, MMDL_GEN_POWER_ONOFF_MSG_LEN, 0, 0);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Published a GenPowOnOffSet message to the publication address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *  \param[in] ackRequired  Ack required for this command
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenPowOnOffPublishSet(meshElementId_t elementId,
                                const mmdlGenPowOnOffSetParam_t *pSetParam, bool_t ackRequired)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_GEN_POWER_ONOFF_CL_MDL_ID,
                                             MMDL_GEN_ONPOWERUP_SET_NO_ACK_OPCODE);

  if (pSetParam != NULL)
  {
    if (pSetParam->state < MMDL_GEN_ONPOWERUP_STATE_PROHIBITED)
    {
      /* Change to acknowledged set */
      if (ackRequired)
      {
        UINT16_TO_BE_BUF(pubMsgInfo.opcode.opcodeBytes, MMDL_GEN_ONPOWERUP_SET_OPCODE);
      }

      /* Fill in the msg info parameters */
      pubMsgInfo.elementId = elementId;

      /* Send message to the Mesh Core. Parameters are already stored in over-the-air order */
      MeshPublishMessage(&pubMsgInfo, &pSetParam->state, MMDL_GEN_POWER_ONOFF_MSG_LEN);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a GenOnPowerUpStatus message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenPowOnOffClHandleStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlGenPowOnOffClStatusEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_GEN_POWER_ONOFF_STATUS_LEN)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_GEN_POWER_ONOFF_CL_EVENT;
  event.hdr.param = MMDL_GEN_POWER_ONOFF_CL_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  pParams = pMsg->pMessageParams;

  /* Extract status event parameters */
  BSTREAM_TO_UINT8(event.state, pParams);

  if (event.state >= MMDL_GEN_ONPOWERUP_STATE_PROHIBITED)
  {
    return;
  }

  /* Set event contents */
  event.targetState = event.state;
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  powOnOffClCb.recvCback((wsfMsgHdr_t *)&event);
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Power OnOff Client Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffClHandlerInit(wsfHandlerId_t handlerId)
{
  /* Set handler ID */
  mmdlGenPowOnOffClHandlerId = handlerId;

  /* Initialize control block */
  powOnOffClCb.recvCback = MmdlEmptyCback;
}

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Power OnOff Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffClHandler(wsfMsgHdr_t *pMsg)
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
        if (MESH_OPCODE_SIZE(pModelMsg->opCode) == MMDL_GEN_POWER_ONOFF_OPCODES_SIZE &&
            !memcmp(&mmdlGenPowOnOffClRcvdOpcodes[0], pModelMsg->opCode.opcodeBytes,
                    MMDL_GEN_POWER_ONOFF_OPCODES_SIZE))
        {
          /* Process Status message */
          mmdlGenPowOnOffClHandleStatus(pModelMsg);
        }
        break;

      default:
        MMDL_TRACE_WARN0("GEN POWER ONOFF CL: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a GenOnPowerUpGet message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                        uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_POWER_ONOFF_CL_MDL_ID,
                                   MMDL_GEN_ONPOWERUP_GET_OPCODE);
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_GEN_POWER_ONOFF_CL_MDL_ID,
                                             MMDL_GEN_ONPOWERUP_GET_OPCODE);

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
 *  \brief     Send a GenOnPowerUpSet message to the destination address.
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
void MmdlGenPowOnOffClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                          const mmdlGenPowOnOffSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  if (serverAddr == MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlGenPowOnOffPublishSet(elementId, pSetParam, TRUE);
  }
  else
  {
    mmdlGenPowOnOffSet(elementId, serverAddr, ttl, pSetParam, appKeyIndex, TRUE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a GenOnPowerUpSetUnacknowledged message to the destination address.
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
void MmdlGenPowOnOffClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                               const mmdlGenPowOnOffSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  if (serverAddr == MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlGenPowOnOffPublishSet(elementId, pSetParam, FALSE);
  }
  else
  {
    mmdlGenPowOnOffSet(elementId, serverAddr, ttl, pSetParam, appKeyIndex, FALSE);
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
void MmdlGenPowOnOffClRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback */
  if (recvCback != NULL)
  {
    powOnOffClCb.recvCback = recvCback;
  }
}
