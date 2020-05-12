/*************************************************************************************************/
/*!
 *  \file   mmdl_gen_powerlevel_cl_main.c
 *
 *  \brief  Implementation of the Generic Power Level Client model.
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
#include "mmdl_gen_powerlevel_cl_api.h"
#include "mmdl_gen_powerlevel_cl_main.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Generic On Power Level Client control block type definition */
typedef struct mmdlGenPowerLevelClCb_tag
{
  mmdlEventCback_t recvCback;    /*!< Model Generic Power Level received callback */
} mmdlGenPowerLevelClCb_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler id */
wsfHandlerId_t mmdlGenPowerLevelClHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlGenPowerLevelClRcvdOpcodes[MMDL_GEN_POWER_LEVEL_CL_NUM_RCVD_OPCODES] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_POWER_LEVEL_STATUS_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_POWERLAST_STATUS_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_POWERDEFAULT_STATUS_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_POWERRANGE_STATUS_OPCODE)} },
};

/*! Generic Power Level Client Model Opcode Type Enum */
enum genPowerLevelClOpcodeType
{
  MMDL_GEN_POWER_LEVEL_CL_STATUS_OPCODE = 0x00, /*!< Generic Power Level Status Opcode */
  MMDL_GEN_POWER_LAST_CL_STATUS_OPCODE,         /*!< Generic Power Last Status Opcode */
  MMDL_GEN_POWER_DEFAULT_CL_STATUS_OPCODE,      /*!< Generic Power Default Status Opcode */
  MMDL_GEN_POWER_RANGE_CL_STATUS_OPCODE,        /*!< Generic Power Range Status Opcode */
};

/*! Generic Power Level Client message handler type definition */
typedef void (*mmdlGenPowerLevelClHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlGenPowerLevelClHandleMsg_t mmdlGenPowerLevelClHandleMsg[MMDL_GEN_POWER_LEVEL_CL_NUM_RCVD_OPCODES] =
{
  mmdlGenPowerLevelClHandleStatus,
  mmdlGenPowerLastClHandleStatus,
  mmdlGenPowerDefaultClHandleStatus,
  mmdlGenPowerRangeClHandleStatus
};
/*! On Off Client control block */
static mmdlGenPowerLevelClCb_t  powerLevelClCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Sends a GenPowerLevelSet message to the destination address.
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
static void mmdlGenPowerLevelSet(meshMsgInfo_t *pMsgInfo, meshElementId_t elementId,
                                 meshAddress_t serverAddr, uint8_t ttl,
                                 const mmdlGenPowerLevelSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  uint8_t *pParams;
  uint8_t paramMsg[MMDL_GEN_POWER_LEVEL_SET_MAX_LEN];

  if (pSetParam!= NULL)
  {
    pParams = paramMsg;

    /* Fill in the message information */
    pMsgInfo->elementId = elementId;
    pMsgInfo->dstAddr = serverAddr;
    pMsgInfo->ttl = ttl;
    pMsgInfo->appKeyIndex = appKeyIndex;

    /* Build param message. */
    UINT16_TO_BSTREAM(pParams, pSetParam->state);
    UINT8_TO_BSTREAM(pParams, pSetParam->tid);

    /* Do not include transition time and delay in the message if it is not used */
    if (pSetParam->transitionTime != MMDL_GEN_TR_UNKNOWN)
    {
      UINT8_TO_BSTREAM(pParams, pSetParam->transitionTime);
      UINT8_TO_BSTREAM(pParams, pSetParam->delay);
    }

    /* Send message to the Mesh Core */
    MeshSendMessage(pMsgInfo, paramMsg, (uint16_t)(pParams - paramMsg), 0, 0);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a GenPowerDefaultSet message to the destination address.
 *
 *  \param[in] pMsgInfo     Pointer to a Mesh message information structure. See ::meshMsgInfo_t.
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] powerLevel   Default power level.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenPowerDefaultClSet(meshMsgInfo_t *pMsgInfo, meshElementId_t elementId,
                                     meshAddress_t serverAddr, uint8_t ttl,
                                     uint16_t appKeyIndex, mmdlGenPowerLevelState_t powerLevel)
{
  uint8_t paramMsg[MMDL_GEN_POWERDEFAULT_SET_LEN];

  /* Fill in the message information */
  pMsgInfo->elementId = elementId;
  pMsgInfo->dstAddr = serverAddr;
  pMsgInfo->ttl = ttl;
  pMsgInfo->appKeyIndex = appKeyIndex;

  /* Build param message. */
  UINT16_TO_BUF(&paramMsg[0], powerLevel);

  /* Send message to the Mesh Core */
  MeshSendMessage(pMsgInfo, paramMsg, MMDL_GEN_POWERDEFAULT_SET_LEN, 0, 0);
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a GenPowerRangeSet message to the destination address.
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
static void mmdlGenPowerRangeClSet(meshMsgInfo_t *pMsgInfo, meshElementId_t elementId,
                                   meshAddress_t serverAddr, uint8_t ttl,
                                   const mmdlGenPowerRangeSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  uint8_t *pParam;
  uint8_t paramMsg[MMDL_GEN_POWERRANGE_SET_LEN];

  if (pSetParam != NULL)
  {
    pParam = paramMsg;

    /* Fill in the message information */
    pMsgInfo->elementId = elementId;
    pMsgInfo->dstAddr = serverAddr;
    pMsgInfo->ttl = ttl;
    pMsgInfo->appKeyIndex = appKeyIndex;

    /* Build param message. */
    UINT16_TO_BSTREAM(pParam, pSetParam->powerMin);
    UINT16_TO_BSTREAM(pParam, pSetParam->powerMax);

    /* Send message to the Mesh Core */
    MeshSendMessage(pMsgInfo, paramMsg, MMDL_GEN_POWERRANGE_SET_LEN, 0, 0);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Power Level Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenPowerLevelClHandleStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlGenPowerLevelClStatusEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_GEN_POWER_LEVEL_STATUS_MAX_LEN &&
      pMsg->messageParamsLen != MMDL_GEN_POWER_LEVEL_STATUS_MIN_LEN)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_GEN_POWER_LEVEL_CL_EVENT;
  event.hdr.param = MMDL_GEN_POWER_LEVEL_CL_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  pParams = pMsg->pMessageParams;

  /* Extract status event parameters */
  BSTREAM_TO_UINT16(event.state, pParams);

  /* Check if optional parameters are present */
  if (pMsg->messageParamsLen == MMDL_GEN_POWER_LEVEL_STATUS_MAX_LEN)
  {
    /* Extract target state and Remaining Time value */
    BSTREAM_TO_UINT16(event.targetState, pParams);
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
  powerLevelClCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Power Last Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenPowerLastClHandleStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlGenPowerLastClStatusEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_GEN_POWERLAST_STATUS_LEN)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_GEN_POWER_LEVEL_CL_EVENT;
  event.hdr.param = MMDL_GEN_POWER_LAST_CL_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  pParams = pMsg->pMessageParams;

  /* Extract status event parameters */
  BSTREAM_TO_UINT16(event.lastState, pParams);

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  powerLevelClCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Power Default Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenPowerDefaultClHandleStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlGenPowerDefaultClStatusEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_GEN_POWERDEFAULT_STATUS_LEN)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_GEN_POWER_LEVEL_CL_EVENT;
  event.hdr.param = MMDL_GEN_POWER_DEFAULT_CL_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  pParams = pMsg->pMessageParams;

  /* Extract status event parameters */
  BSTREAM_TO_UINT16(event.state, pParams);

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  powerLevelClCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Power Range Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenPowerRangeClHandleStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlGenPowerRangeClStatusEvent_t event;
  uint8_t *pParams;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_GEN_POWERRANGE_STATUS_LEN)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_GEN_POWER_LEVEL_CL_EVENT;
  event.hdr.param = MMDL_GEN_POWER_RANGE_CL_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  pParams = pMsg->pMessageParams;

  /* Extract status event parameters */
  BSTREAM_TO_UINT8(event.statusCode, pParams);
  BSTREAM_TO_UINT16(event.powerMin, pParams);
  BSTREAM_TO_UINT16(event.powerMax, pParams);

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  powerLevelClCb.recvCback((wsfMsgHdr_t *)&event);
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for On Off Client Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelClHandlerInit(wsfHandlerId_t handlerId)
{
  /* Set handler ID */
  mmdlGenPowerLevelClHandlerId = handlerId;

  /* Initialize control block */
  powerLevelClCb.recvCback = MmdlEmptyCback;
}

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Power Level Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelClHandler(wsfMsgHdr_t *pMsg)
{
  uint8_t opcodeIdx;
  meshModelMsgRecvEvt_t *pModelMsg;

  /* Handle message */
  if (pMsg != NULL)
  {
    switch (pMsg->event)
    {
      case MESH_MODEL_EVT_MSG_RECV:
        pModelMsg = (meshModelMsgRecvEvt_t *)pMsg;

        /* Validate opcode size and value */
        if (MESH_OPCODE_SIZE(pModelMsg->opCode) == MMDL_GEN_POWER_LEVEL_OPCODES_SIZE)
        {
          /* Match the received opcode */
          for (opcodeIdx = 0; opcodeIdx < MMDL_GEN_POWER_LEVEL_CL_NUM_RCVD_OPCODES; opcodeIdx++)
          {
            if (!memcmp(&mmdlGenPowerLevelClRcvdOpcodes[opcodeIdx], pModelMsg->opCode.opcodeBytes,
                MMDL_GEN_POWER_LEVEL_OPCODES_SIZE))
            {
              /* Process message */
              (void)mmdlGenPowerLevelClHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
            }
          }
        }
        break;
      default:
        MMDL_TRACE_WARN0("GEN POWER LEVEL CL: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a GenPowerLevelGet message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_POWER_LEVEL_CL_MDL_ID, MMDL_GEN_POWER_LEVEL_GET_OPCODE);

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
 *  \brief     Send a GenPowerLevelSet message to the destination address.
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
void MmdlGenPowerLevelClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            const mmdlGenPowerLevelSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_POWER_LEVEL_CL_MDL_ID, MMDL_GEN_POWER_LEVEL_SET_OPCODE);

  mmdlGenPowerLevelSet(&msgInfo, elementId, serverAddr, ttl, pSetParam, appKeyIndex);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a GenPowerLevelSetUnacknowledged message to the destination address.
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
void MmdlGenPowerLevelClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                 const mmdlGenPowerLevelSetParam_t *pSetParam, uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_POWER_LEVEL_CL_MDL_ID, MMDL_GEN_POWER_LEVEL_SET_NO_ACK_OPCODE);

  mmdlGenPowerLevelSet(&msgInfo, elementId, serverAddr, ttl, pSetParam, appKeyIndex);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a GenPowerLastGet message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLastClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                           uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_POWER_LEVEL_CL_MDL_ID, MMDL_GEN_POWERLAST_GET_OPCODE);

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
 *  \brief     Send a GenPowerDefaultGet message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerDefaultClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                              uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_POWER_LEVEL_CL_MDL_ID, MMDL_GEN_POWERDEFAULT_GET_OPCODE);

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
 *  \brief     Send a GenPowerDefaultSet message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] powerLevel   Default power level.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerDefaultClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                              uint16_t appKeyIndex, mmdlGenPowerLevelState_t powerLevel)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_POWER_LEVEL_CL_MDL_ID, MMDL_GEN_POWERDEFAULT_SET_OPCODE);

  mmdlGenPowerDefaultClSet(&msgInfo, elementId, serverAddr, ttl, appKeyIndex, powerLevel);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a GenPowerDefaultSetUnacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] serverAddr   Element address of the server
 *  \param[in] ttl          TTL value as defined by the specification
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] powerLevel   Default power level.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerDefaultClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                   uint16_t appKeyIndex, mmdlGenPowerLevelState_t powerLevel)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_POWER_LEVEL_CL_MDL_ID, MMDL_GEN_POWERDEFAULT_SET_NO_ACK_OPCODE);

  mmdlGenPowerDefaultClSet(&msgInfo, elementId, serverAddr, ttl, appKeyIndex, powerLevel);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a GenPowerRangeGet message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerRangeClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_POWER_LEVEL_CL_MDL_ID, MMDL_GEN_POWERRANGE_GET_OPCODE);

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
 *  \brief     Send a GenPowerRangeSet message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerRangeClSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                            uint16_t appKeyIndex, const mmdlGenPowerRangeSetParam_t *pSetParam)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_POWER_LEVEL_CL_MDL_ID, MMDL_GEN_POWERRANGE_SET_OPCODE);

  mmdlGenPowerRangeClSet(&msgInfo, elementId, serverAddr, ttl, pSetParam, appKeyIndex);
}

/*************************************************************************************************/
/*!
 *  \brief     Send a GenPowerRangeSetUnacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] serverAddr   Element address of the server
 *  \param[in] ttl          TTL value as defined by the specification
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pSetParam    Pointer to structure containing the set parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerRangeClSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                 uint16_t appKeyIndex, const mmdlGenPowerRangeSetParam_t *pSetParam)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_POWER_LEVEL_CL_MDL_ID, MMDL_GEN_POWERRANGE_SET_NO_ACK_OPCODE);

  mmdlGenPowerRangeClSet(&msgInfo, elementId, serverAddr, ttl, pSetParam, appKeyIndex);
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
void MmdlGenPowerLevelClRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback*/
  if (recvCback != NULL)
  {
    powerLevelClCb.recvCback = recvCback;
  }
}
