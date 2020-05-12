/*************************************************************************************************/
/*!
 *  \file   mmdl_gen_battery_cl_main.c
 *
 *  \brief  Implementation of the Generic Battery Client model.
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
#include "mmdl_gen_battery_cl_api.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Generic Battery control block type definition */
typedef struct mmdlGenBetteryClCb_tag
{
  mmdlEventCback_t recvCback;    /*!< Model Generic Battery received callback */
} mmdlGenBatteryClCb_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler id */
wsfHandlerId_t mmdlGenBatteryClHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlGenBatteryClRcvdOpcodes[] =
{
  { { UINT16_OPCODE_TO_BYTES(MMDL_GEN_BATTERY_STATUS_OPCODE)} }
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Battery Client control block */
static mmdlGenBatteryClCb_t  batteryClCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Battery Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenBatteryClHandleStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlGenBatteryClStatusEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_GEN_BATTERY_STATUS_LENGTH)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_GEN_BATTERY_CL_EVENT;
  event.hdr.param = MMDL_GEN_BATTERY_CL_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  pParams = pMsg->pMessageParams;

  /* Extract status event parameters */
  BSTREAM_TO_UINT8(event.state, pParams);
  BSTREAM_TO_UINT24(event.timeToDischarge, pParams);
  BSTREAM_TO_UINT24(event.timeToCharge, pParams);
  BSTREAM_TO_UINT8(event.flags, pParams);

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  batteryClCb.recvCback((wsfMsgHdr_t *)&event);
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Battery Client Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenBatteryClHandlerInit(wsfHandlerId_t handlerId)
{
  /* Set handler ID */
  mmdlGenBatteryClHandlerId = handlerId;

  /* Initialize control block */
  batteryClCb.recvCback = MmdlEmptyCback;
}

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Battery Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenBatteryClHandler(wsfMsgHdr_t *pMsg)
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
        if (MESH_OPCODE_SIZE(pModelMsg->opCode) == MMDL_GEN_BATTERY_OPCODES_SIZE &&
            !memcmp(&mmdlGenBatteryClRcvdOpcodes[0], pModelMsg->opCode.opcodeBytes,
                    MMDL_GEN_BATTERY_OPCODES_SIZE))
        {
          /* Process Status message */
          mmdlGenBatteryClHandleStatus(pModelMsg);
        }
        break;

      default:
        MMDL_TRACE_WARN0("GEN BATTERY CL: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a GenBatterylGet message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenBatteryClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                         uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_BATTERY_CL_MDL_ID, MMDL_GEN_BATTERY_GET_OPCODE);
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_GEN_BATTERY_CL_MDL_ID, MMDL_GEN_BATTERY_GET_OPCODE);

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
 *  \brief     Install the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenBatteryClRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback*/
  if (recvCback != NULL)
  {
    batteryClCb.recvCback = recvCback;
  }
}
