/*************************************************************************************************/
/*!
 *  \file   mmdl_scheduler_cl_main.c
 *
 *  \brief  Implementation of the Scheduler Client model.
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
#include "wsf_buf.h"
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"

#include "mmdl_types.h"
#include "mmdl_common.h"
#include "mmdl_scheduler_cl_api.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Initializer of a message info for the specified model ID and opcode */
#define MSG_INFO(modelId) {{(meshSigModelId_t)modelId }, {{0,0,0}},\
                           0xFF, NULL, MESH_ADDR_TYPE_UNASSIGNED, 0xFF, 0xFF}

/*! Initializer of a publish message info for the specified model ID and opcode */
#define PUB_MSG_INFO(modelId) {{{0,0,0}}, 0xFF , {(meshSigModelId_t)modelId }}

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Scheduler Client control block type definition */
typedef struct mmdlSchedulerClCb_tag
{
  mmdlEventCback_t recvCback;    /*!< Model received callback */
} mmdlSchedulerClCb_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler id */
wsfHandlerId_t mmdlSchedulerClHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlSchedulerClRcvdOpcodes[MMDL_SCHEDULER_CL_NUM_RCVD_OPCODES] =
{
  { { UINT8_OPCODE_TO_BYTES(MMDL_SCHEDULER_ACTION_STATUS_OPCODE) } },
  { { UINT16_OPCODE_TO_BYTES(MMDL_SCHEDULER_STATUS_OPCODE) } }
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Scheduler Client control block */
static mmdlSchedulerClCb_t  schedulerClCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 * \brief     Gets next field in a Scheduler Action Set/Set NoAck/Status message parameters.
 *
 * \param[in] pMsgParams  Pointer to message parameters to start or NULL to continue parsing.
 * \param[in] fieldSize   Size of the field getting extracted.
 *
 * \return    Extracted field.
 *
 */
/*************************************************************************************************/
static uint32_t mmdlSchedulerActionMsgParamsGetNextField(uint8_t *pMsgParams, uint8_t fieldSize)
{
  static uint8_t *pBuf = NULL;
  static uint8_t numBitsLeft = 8;
  uint32_t field;
  uint8_t tempValue;
  uint8_t nextShift;
  uint8_t minBits;

  /* Re-init if input buffer is not NULL. */
  if (pMsgParams != NULL)
  {
    pBuf = pMsgParams;
    numBitsLeft = 8;
  }

  field = 0;
  nextShift = 0;
  while (fieldSize)
  {
    /* Check if number of bits left are sufficient. */
    minBits = fieldSize <= numBitsLeft ? fieldSize : numBitsLeft;
    /* Extract those bits. */
    tempValue = (*pBuf >> (8 - numBitsLeft)) & ((1 << minBits) - 1);
    field = (tempValue << nextShift) + field;

    nextShift += minBits;
    fieldSize -= minBits;
    numBitsLeft -= minBits;

    /* Check if move to next byte is required. */
    if (numBitsLeft == 0)
    {
      pBuf++;
      numBitsLeft = 8;
    }
  }
  return field;
}

/*************************************************************************************************/
/*!
 * \brief     Sets next field in a Scheduler Action Set/Set NoAck/Status message parameters.
 *
 * \param[in] pMsgParams  Pointer to message parameters to start or NULL to continue parsing.
 * \param[in] field       Field to be set in the message parameters.
 * \param[in] fieldSize   Size of the field getting extracted.
 *
 * \return    None.
 */
/*************************************************************************************************/
static void mmdlSchedulerActionMsgParamsSetNextField(uint8_t *pMsgParams, uint32_t field,
                                                     uint8_t fieldSize)
{
  static uint8_t *pBuf = NULL;
  static uint8_t numBitsLeft = 8;
  uint8_t tempByte;
  uint8_t nextShift;
  uint8_t minBits;

  /* Re-init if input buffer is not NULL. */
  if (pMsgParams != NULL)
  {
    pBuf = pMsgParams;
    *pBuf = 0;
    numBitsLeft = 8;
  }

  nextShift = 0;
  tempByte = 0;
  while (fieldSize)
  {
    /* Check if number of bits left are sufficient. */
    minBits = fieldSize <= numBitsLeft ? fieldSize : numBitsLeft;

    /* Extract those bits. */
    tempByte = (field >> nextShift) & ((1 << minBits) - 1);

    /* Set the bits in the correct position of the buffer. */
    *pBuf = (tempByte << (8 - numBitsLeft)) + *pBuf;

    fieldSize -= minBits;
    numBitsLeft -= minBits;
    nextShift += minBits;

    /* Check if move to next byte is required. */
    if (numBitsLeft == 0)
    {
      pBuf++;
      *pBuf = 0;
      numBitsLeft = 8;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Unpacks Scheduler Action Status message parameters.
 *
 *  \param[in]  pMsgParams  Pointer to Scheduler Action Status message parameters.
 *  \param[out] pOutIndex   Pointer to memory where the Register entry index is stored.
 *  \param[out] pOutEntry   Pointer to memory where the Register entry values are stored.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void mmdlSchedulerUnpackActionParams(uint8_t *pMsgParams, uint8_t *pOutIndex,
                                            mmdlSchedulerRegisterEntry_t *pOutEntry)
{
  /* Unpack index */
  *pOutIndex = (uint8_t)
      mmdlSchedulerActionMsgParamsGetNextField(pMsgParams,
                                               MMDL_SCHEDULER_REGISTER_FIELD_INDEX_SIZE);
  /* Unpack year. */
  pOutEntry->year = (uint8_t)
      mmdlSchedulerActionMsgParamsGetNextField(NULL,
                                               MMDL_SCHEDULER_REGISTER_FIELD_YEAR_SIZE);
  /* Unpack month. */
  pOutEntry->months = (mmdlSchedulerRegisterMonthBf_t)
      mmdlSchedulerActionMsgParamsGetNextField(NULL,
                                               MMDL_SCHEDULER_REGISTER_FIELD_MONTH_SIZE);
  /* Unpack day. */
  pOutEntry->day = (mmdlSchedulerRegisterDay_t)
      mmdlSchedulerActionMsgParamsGetNextField(NULL,
                                               MMDL_SCHEDULER_REGISTER_FIELD_DAY_SIZE);
  /* Unpack hour. */
  pOutEntry->hour = (mmdlSchedulerRegisterHour_t)
      mmdlSchedulerActionMsgParamsGetNextField(NULL,
                                               MMDL_SCHEDULER_REGISTER_FIELD_HOUR_SIZE);
  /* Unpack minute. */
  pOutEntry->minute = (mmdlSchedulerRegisterMinute_t)
      mmdlSchedulerActionMsgParamsGetNextField(NULL,
                                               MMDL_SCHEDULER_REGISTER_FIELD_MINUTE_SIZE);
  /* Unpack second. */
  pOutEntry->second = (mmdlSchedulerRegisterSecond_t)
      mmdlSchedulerActionMsgParamsGetNextField(NULL,
                                               MMDL_SCHEDULER_REGISTER_FIELD_SECOND_SIZE);
  /* Unpack days of week. */
  pOutEntry->daysOfWeek = (mmdlSchedulerRegisterDayOfWeekBf_t)
      mmdlSchedulerActionMsgParamsGetNextField(NULL,
                                               MMDL_SCHEDULER_REGISTER_FIELD_DAYOFWEEK_SIZE);
  /* Unpack action. */
  pOutEntry->action = (mmdlSchedulerRegisterAction_t)
      mmdlSchedulerActionMsgParamsGetNextField(NULL,
                                               MMDL_SCHEDULER_REGISTER_FIELD_ACTION_SIZE);
  /* Unpack transition time. */
  pOutEntry->transTime = (mmdlGenDefaultTransState_t)
      mmdlSchedulerActionMsgParamsGetNextField(NULL,
                                               MMDL_SCHEDULER_REGISTER_FIELD_TRANS_TIME_SIZE);
  /* Unpack scene number. */
  pOutEntry->sceneNumber = (mmdlSceneNumber_t)
      mmdlSchedulerActionMsgParamsGetNextField(NULL,
                                               MMDL_SCHEDULER_REGISTER_FIELD_SCENE_NUM_SIZE);
}

/*************************************************************************************************/
/*!
 *  \brief     Packs Scheduler Action Get/Set/Set NoAck message parameters.
 *
 *  \param[in] pMsgParams  Pointer to message parameters.
 *  \param[in] index       Register entry index.
 *  \param[in] pEntry      Pointer to Register entry values.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlSchedulerPackActionParams(uint8_t *pMsgParams, uint8_t index,
                                          const mmdlSchedulerRegisterEntry_t *pEntry)
{
  /* Pack index. */
  mmdlSchedulerActionMsgParamsSetNextField(pMsgParams, (uint32_t)index,
                                           MMDL_SCHEDULER_REGISTER_FIELD_INDEX_SIZE);

  /* Pack year. */
  mmdlSchedulerActionMsgParamsSetNextField(NULL, (uint32_t)(pEntry->year),
                                           MMDL_SCHEDULER_REGISTER_FIELD_YEAR_SIZE);
  /* Pack month. */
  mmdlSchedulerActionMsgParamsSetNextField(NULL, (uint32_t)(pEntry->months),
                                           MMDL_SCHEDULER_REGISTER_FIELD_MONTH_SIZE);
  /* Pack day. */
  mmdlSchedulerActionMsgParamsSetNextField(NULL, (uint32_t)(pEntry->day),
                                           MMDL_SCHEDULER_REGISTER_FIELD_DAY_SIZE);
  /* Pack hour. */
  mmdlSchedulerActionMsgParamsSetNextField(NULL, (uint32_t)(pEntry->hour),
                                           MMDL_SCHEDULER_REGISTER_FIELD_HOUR_SIZE);
  /* Pack minute. */
  mmdlSchedulerActionMsgParamsSetNextField(NULL, (uint32_t)(pEntry->minute),
                                           MMDL_SCHEDULER_REGISTER_FIELD_MINUTE_SIZE);
  /* Pack second. */
  mmdlSchedulerActionMsgParamsSetNextField(NULL, (uint32_t)(pEntry->second),
                                           MMDL_SCHEDULER_REGISTER_FIELD_SECOND_SIZE);
  /* Pack days of week. */
  mmdlSchedulerActionMsgParamsSetNextField(NULL, (uint32_t)(pEntry->daysOfWeek),
                                           MMDL_SCHEDULER_REGISTER_FIELD_DAYOFWEEK_SIZE);
  /* Pack action. */
  mmdlSchedulerActionMsgParamsSetNextField(NULL, (uint32_t)(pEntry->action),
                                           MMDL_SCHEDULER_REGISTER_FIELD_ACTION_SIZE);
  /* Pack transition time. */
  mmdlSchedulerActionMsgParamsSetNextField(NULL, (uint32_t)(pEntry->transTime),
                                           MMDL_SCHEDULER_REGISTER_FIELD_TRANS_TIME_SIZE);
  /* Pack scene number. */
  mmdlSchedulerActionMsgParamsSetNextField(NULL, (uint32_t)(pEntry->sceneNumber),
                                           MMDL_SCHEDULER_REGISTER_FIELD_SCENE_NUM_SIZE);
}


/*************************************************************************************************/
/*!
 *  \brief     Sends a Scheduler Client message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] pParam       Pointer to structure containing the message parameters.
 *  \param[in] paramLen     Length of message parameters structure.
 *  \param[in] opcode       Scheduler Client message opcode.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlSchedulerSendMessage(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                     uint16_t appKeyIndex, const uint8_t *pParam, uint8_t paramLen,
                                     uint16_t opcode)
{
  meshMsgInfo_t msgInfo = MSG_INFO(MMDL_SCHEDULER_CL_MDL_ID);

  /* Fill in the message information */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = serverAddr;
  msgInfo.ttl = ttl;
  msgInfo.appKeyIndex = appKeyIndex;

  if((opcode >> 8) == 0)
  {
    msgInfo.opcode.opcodeBytes[0] = (uint8_t)(opcode & 0x00FF);
  }
  else
  {
    UINT16_TO_BE_BUF(msgInfo.opcode.opcodeBytes, opcode);
  }

  /* Send message to the Mesh Core. Parameters are already stored in over-the-air order */
  MeshSendMessage(&msgInfo, (uint8_t *)pParam, paramLen, 0, 0);
}

/*************************************************************************************************/
/*!
 *  \brief     Publishes a Scheduler message to the publication address.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] pParam     Pointer to structure containing the parameters.
 *  \param[in] paramLen   Length of the parameters structure.
 *  \param[in] opcode     Command opcode.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlSchedulerPublishMessage(meshElementId_t elementId, const uint8_t *pParam,
                                        uint8_t paramLen, uint16_t opcode)
{
  meshPubMsgInfo_t pubMsgInfo = PUB_MSG_INFO(MMDL_SCHEDULER_CL_MDL_ID);

  /* Fill in the msg info parameters */
  pubMsgInfo.elementId = elementId;

  if((opcode >> 8) == 0)
  {
    pubMsgInfo.opcode.opcodeBytes[0] = (uint8_t)(opcode & 0x00FF);
  }
  else
  {
    UINT16_TO_BE_BUF(pubMsgInfo.opcode.opcodeBytes, opcode);
  }

  /* Send message to the Mesh Core. Parameters are already stored in over-the-air order */
  MeshPublishMessage(&pubMsgInfo, (uint8_t *)pParam, paramLen);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Scheduler Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlSchedulerClHandleStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlSchedulerClStatusEvent_t event;

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_SCHEDULER_STATUS_LEN)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_SCHEDULER_CL_EVENT;
  event.hdr.param = MMDL_SCHEDULER_CL_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  /* Extract status event parameters */
  BYTES_TO_UINT16(event.schedulesBf, pMsg->pMessageParams);

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  schedulerClCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Scheduler Action Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlSchedulerClHandleActionStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlSchedulerClActionStatusEvent_t event;

  /* Validate message length */
  if (pMsg->messageParamsLen !=  MMDL_SCHEDULER_ACTION_STATUS_LEN)
  {
    return;
  }

  /* Set event type and status */
  event.hdr.event = MMDL_SCHEDULER_CL_EVENT;
  event.hdr.param = MMDL_SCHEDULER_CL_ACTION_STATUS_EVENT;
  event.hdr.status = MMDL_SUCCESS;

  /* Unpack state. */
  mmdlSchedulerUnpackActionParams(pMsg->pMessageParams, &event.index, &event.scheduleRegister);

  /* Set event contents */
  event.elementId = pMsg->elementId;
  event.serverAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  schedulerClCb.recvCback((wsfMsgHdr_t *)&event);
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Mesh WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Scheduler Client Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerClHandlerInit(wsfHandlerId_t handlerId)
{
  /* Set handler ID */
  mmdlSchedulerClHandlerId = handlerId;

  /* Initialize control block */
  schedulerClCb.recvCback = MmdlEmptyCback;
}

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Scheduler Client Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerClHandler(wsfMsgHdr_t *pMsg)
{
  meshModelMsgRecvEvt_t *pModelMsg;
  uint8_t opcodeIdx;

  /* Handle message */
  if (pMsg != NULL)
  {
    switch (pMsg->event)
    {
      case MESH_MODEL_EVT_MSG_RECV:
        pModelMsg = (meshModelMsgRecvEvt_t *)pMsg;

        for (opcodeIdx = 0; opcodeIdx < MMDL_SCHEDULER_CL_NUM_RCVD_OPCODES; opcodeIdx++)
        {
          if (!memcmp(&mmdlSchedulerClRcvdOpcodes[opcodeIdx], pModelMsg->opCode.opcodeBytes,
                      MESH_OPCODE_SIZE(pModelMsg->opCode)))
          {
            if (MESH_OPCODE_IS_SIZE_ONE(pModelMsg->opCode))
            {
              /* Process Action Status message */
              mmdlSchedulerClHandleActionStatus(pModelMsg);
            }
            else
            {
              /* Process Status message */
              mmdlSchedulerClHandleStatus(pModelMsg);
            }
          }
        }
        break;

      default:
        MMDL_TRACE_WARN0("SCHEDULER CL: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Scheduler Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerClGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                        uint16_t appKeyIndex)
{
  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlSchedulerSendMessage(elementId, serverAddr, ttl, appKeyIndex, NULL, 0, MMDL_SCHEDULER_GET_OPCODE);
  }
  else
  {
    mmdlSchedulerPublishMessage(elementId, NULL, 0, MMDL_SCHEDULER_GET_OPCODE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Scheduler Action Get message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] index        Entry index of the Scheduler Register state.
 *                          Values 0x10–0xFF are Prohibited.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerClActionGet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                              uint16_t appKeyIndex, uint8_t index)
{
  /* Validate message parameters. */
  if(index > MMDL_SCHEDULER_REGISTER_ENTRY_MAX)
  {
    return;
  }

  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlSchedulerSendMessage(elementId, serverAddr, ttl, appKeyIndex, &index,
                             MMDL_SCHEDULER_ACTION_GET_LEN, MMDL_SCHEDULER_ACTION_GET_OPCODE);
  }
  else
  {
    mmdlSchedulerPublishMessage(elementId, &index, MMDL_SCHEDULER_ACTION_GET_LEN,
                                MMDL_SCHEDULER_ACTION_GET_OPCODE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Scheduler Action Set message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] index        Entry index of the Scheduler Register state.
 *                          Values 0x10–0xFF are Prohibited.
 *  \param[in] pParam       Pointer to Scheduler Register State entry parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerClActionSet(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                              uint16_t appKeyIndex, uint8_t index,
                              const mmdlSchedulerRegisterEntry_t *pParam)
{
  uint8_t param[MMDL_SCHEDULER_ACTION_SET_LEN];

  /* Validate parameters. */
  if((index > MMDL_SCHEDULER_REGISTER_ENTRY_MAX) ||
     (pParam == NULL) ||
     (pParam->year > MMDL_SCHEDULER_REGISTER_YEAR_ALL) ||
     (pParam->months >= MMDL_SCHEDULER_SCHED_IN_PROHIBITED_START) ||
     (pParam->day > MMDL_SCHEDULER_DAY_LAST) ||
     (pParam->hour >= MMDL_SCHEDULER_HOUR_PROHIBITED_START) ||
     (pParam->minute >= MMDL_SCHEDULER_MINUTE_PROHIBITED_START) ||
     (pParam->second >= MMDL_SCHEDULER_SECOND_PROHIBITED_START) ||
     (pParam->daysOfWeek >= MMDL_SCHEDULER_SCHED_ON_PROHIBITED_START) ||
     (MMDL_SCHEDULER_ACTION_IS_RFU(pParam->action)) ||
     ((pParam->action == MMDL_SCHEDULER_ACTION_SCENE_RECALL) &&
      (pParam->sceneNumber == MMDL_SCENE_NUM_PROHIBITED)))
  {
    return;
  }

  /* Pack message parameters. */
  mmdlSchedulerPackActionParams(param, index, pParam);

  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlSchedulerSendMessage(elementId, serverAddr, ttl, appKeyIndex, param,
                             MMDL_SCHEDULER_ACTION_SET_LEN, MMDL_SCHEDULER_ACTION_SET_OPCODE);
  }
  else
  {
    mmdlSchedulerPublishMessage(elementId, param, MMDL_SCHEDULER_ACTION_SET_LEN,
                                MMDL_SCHEDULER_ACTION_SET_OPCODE);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Send a Scheduler Action Set Unacknowledged message to the destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] serverAddr   Element address of the server or ::MMDL_USE_PUBLICATION_ADDR.
 *  \param[in] ttl          TTL value as defined by the specification.
 *  \param[in] appKeyIndex  Application Key Index.
 *  \param[in] index        Entry index of the Scheduler Register state.
 *                          Values 0x10–0xFF are Prohibited.
 *  \param[in] pParam       Pointer to Scheduler Register State entry parameters.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerClActionSetNoAck(meshElementId_t elementId, meshAddress_t serverAddr, uint8_t ttl,
                                   uint16_t appKeyIndex, uint8_t index,
                                   const mmdlSchedulerRegisterEntry_t *pParam)
{
  uint8_t param[MMDL_SCHEDULER_ACTION_SET_NO_ACK_LEN];

  /* Validate parameters. */
  if((index > MMDL_SCHEDULER_REGISTER_ENTRY_MAX) ||
     (pParam == NULL) ||
     (pParam->year > MMDL_SCHEDULER_REGISTER_YEAR_ALL) ||
     (pParam->months >= MMDL_SCHEDULER_SCHED_IN_PROHIBITED_START) ||
     (pParam->day > MMDL_SCHEDULER_DAY_LAST) ||
     (pParam->hour >= MMDL_SCHEDULER_HOUR_PROHIBITED_START) ||
     (pParam->minute >= MMDL_SCHEDULER_MINUTE_PROHIBITED_START) ||
     (pParam->second >= MMDL_SCHEDULER_SECOND_PROHIBITED_START) ||
     (pParam->daysOfWeek >= MMDL_SCHEDULER_SCHED_ON_PROHIBITED_START) ||
     (MMDL_SCHEDULER_ACTION_IS_RFU(pParam->action)))
  {
    return;
  }

  /* Pack message parameters. */
  mmdlSchedulerPackActionParams(param, index, pParam);

  if (serverAddr != MMDL_USE_PUBLICATION_ADDR)
  {
    mmdlSchedulerSendMessage(elementId, serverAddr, ttl, appKeyIndex, param,
                             MMDL_SCHEDULER_ACTION_SET_NO_ACK_LEN, MMDL_SCHEDULER_ACTION_SET_NO_ACK_OPCODE);
  }
  else
  {
    mmdlSchedulerPublishMessage(elementId, param, MMDL_SCHEDULER_ACTION_SET_NO_ACK_LEN,
                                MMDL_SCHEDULER_ACTION_SET_NO_ACK_OPCODE);
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
void MmdlSchedulerClRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback */
  if (recvCback != NULL)
  {
    schedulerClCb.recvCback = recvCback;
  }
}
