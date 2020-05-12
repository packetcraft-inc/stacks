/*************************************************************************************************/
/*!
 *  \file   mmdl_scheduler_sr_main.c
 *
 *  \brief  Implementation of the Scheduler Server model.
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
#include "wsf_timer.h"
#include "wsf_os.h"
#include "wsf_cs.h"
#include "wsf_trace.h"
#include "wsf_buf.h"
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"

#include "mmdl_types.h"
#include "mmdl_defs.h"
#include "mmdl_common.h"
#include "mmdl_bindings.h"
#include "mmdl_gen_default_trans_sr.h"
#include "mmdl_gen_onoff_sr.h"
#include "mmdl_scene_sr.h"
#include "mmdl_scheduler_sr_api.h"
#include "mmdl_scheduler_sr_main.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Packs element ID and Register State entry index into WSF timer param field */
#define ELEMID_REGIDX_TO_TMR_PARAM(elementId, index) ((elementId << 8) + index)

/*! Extracts element ID from WSF timer param field */
#define TMR_PARAM_TO_ELEMID(param)                   ((meshElementId_t)((param) >> 8))

/*! Extracts Register State entry index from WSF timer param field */
#define TMR_PARAM_TO_REGIDX(param)                   ((uint8_t)((param) & 0x00FF))

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Scheduler Server control block type definition */
typedef struct mmdlSchedulerSrCb_tag
{
  mmdlEventCback_t          recvCback;            /*!< Model Scheduler Server received callback */
}mmdlSchedulerSrCb_t;

/*! Scheduler Server message handler type definition */
typedef void (*mmdlSchedulerSrHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler ID */
wsfHandlerId_t mmdlSchedulerSrHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlSchedulerSrRcvdOpcodes[MMDL_SCHEDULER_SR_NUM_RCVD_OPCODES] =
{
  {{ UINT16_OPCODE_TO_BYTES(MMDL_SCHEDULER_GET_OPCODE) }},
  {{ UINT16_OPCODE_TO_BYTES(MMDL_SCHEDULER_ACTION_GET_OPCODE) }}
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlSchedulerSrHandleMsg_t mmdlSchedulerSrHandleMsg[MMDL_SCHEDULER_SR_NUM_RCVD_OPCODES] =
{
  mmdlSchedulerSrHandleGet,
  mmdlSchedulerSrHandleActionGet,
};

/*! Scheduler Server Control Block */
static mmdlSchedulerSrCb_t  schedulerSrCb;

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
 *
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
void mmdlSchedulerUnpackActionParams(uint8_t *pMsgParams, uint8_t *pOutIndex,
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
 *  \brief      Searches for the Scheduler model instance descriptor on the specified element.
 *
 *  \param[in]  elementId  Identifier of the Element implementing the model.
 *  \param[out] ppOutDesc  Double pointer to the descriptor.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void mmdlSchedulerSrGetDesc(meshElementId_t elementId, mmdlSchedulerSrDesc_t **ppOutDesc)
{
  uint8_t modelIdx;

  *ppOutDesc = NULL;

  /* Check if element exists. */
  if (elementId >= pMeshConfig->elementArrayLen)
  {
    return;
  }

  /* Look for the model instance */
  for (modelIdx = 0; modelIdx < pMeshConfig->pElementArray[elementId].numSigModels; modelIdx ++)
  {
    if (pMeshConfig->pElementArray[elementId].pSigModelArray[modelIdx].modelId ==
        MMDL_SCHEDULER_SR_MDL_ID)
    {
      /* Matching model ID on elementId */
      *ppOutDesc = pMeshConfig->pElementArray[elementId].pSigModelArray[modelIdx].pModelDescriptor;
      break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Schedules the event for an entry of the Register State.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] index      Register State entry identifier.
 *  \param[in] pEntry     Pointer to entry.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlSchedulerSrScheduleEvent(meshElementId_t elementId, uint8_t index,
                                  mmdlSchedulerSrRegisterEntry_t *pEntry)
{
  mmdlSchedulerSrEvent_t event;

  /* Check if action is none. */
  if(pEntry->regEntry.action == MMDL_SCHEDULER_ACTION_NONE)
  {
    /* If the entry is in use notify application that event was cancelled remotely. */
    if(pEntry->inUse)
    {
      event.hdr.event  = MMDL_SCHEDULER_SR_EVENT;
      event.hdr.param  = MMDL_SCHEDULER_SR_STOP_SCHEDULE_EVENT;
      event.hdr.status = MMDL_SUCCESS;
      event.schedStopEvent.elementId = elementId;
      event.schedStopEvent.id = index;

      /* Zero out parameters */
      event.schedStartEvent.year = 0;
      event.schedStartEvent.months = 0;
      event.schedStartEvent.day = 0;
      event.schedStartEvent.hour = 0;
      event.schedStartEvent.minute = 0;
      event.schedStartEvent.second = 0;
      event.schedStartEvent.daysOfWeek = 0;

      schedulerSrCb.recvCback((const wsfMsgHdr_t *)&event);

      /* Clear entry */
      pEntry->inUse = FALSE;
    }
    return;
  }

  /* Mark entry in use. */
  pEntry->inUse = TRUE;

  /* Notify application to schedule an event. */
  event.hdr.event  = MMDL_SCHEDULER_SR_EVENT;
  event.hdr.param  = MMDL_SCHEDULER_SR_START_SCHEDULE_EVENT;
  event.hdr.status = MMDL_SUCCESS;
  event.schedStartEvent.elementId = elementId;
  event.schedStartEvent.id = index;

  /* Configure event parameters. */
  event.schedStartEvent.year = pEntry->regEntry.year;
  event.schedStartEvent.months = pEntry->regEntry.months;
  event.schedStartEvent.day = pEntry->regEntry.day;
  event.schedStartEvent.hour = pEntry->regEntry.hour;
  event.schedStartEvent.minute = pEntry->regEntry.minute;
  event.schedStartEvent.second = pEntry->regEntry.second;
  event.schedStartEvent.daysOfWeek = pEntry->regEntry.daysOfWeek;

  schedulerSrCb.recvCback((const wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Scheduler Status command to the specified destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  Indicates if message that triggered the status was received on unicast.
 *  \param[in] opStatus       Operation status.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlSchedulerSrSendStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                      uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MSG_INFO(MMDL_SCHEDULER_SR_MDL_ID);
  uint8_t msgParams[MMDL_SCHEDULER_STATUS_LEN], idx;
  mmdlSchedulerSrDesc_t *pDesc = NULL;

  /* Get the model instance descriptor. */
  mmdlSchedulerSrGetDesc(elementId, &pDesc);

  if(pDesc != NULL)
  {
    /* Fill in the msg info parameters */
    msgInfo.elementId = elementId;
    msgInfo.dstAddr = dstAddr;
    msgInfo.ttl = MESH_USE_DEFAULT_TTL;
    msgInfo.appKeyIndex = appKeyIndex;
    UINT16_TO_BE_BUF(msgInfo.opcode.opcodeBytes, MMDL_SCHEDULER_STATUS_OPCODE);

    memset(msgParams, 0, sizeof(msgParams));

    /* Set the schedules bit-field. */
    for(idx = 0; idx <= MMDL_SCHEDULER_REGISTER_ENTRY_MAX; idx++)
    {
      if(pDesc->registerState[idx].inUse)
      {
        msgParams[idx >> 3] |= 1 << (idx & 0x07);
      }
    }

    /* Send message to the Mesh Core */
    MeshSendMessage(&msgInfo, (const uint8_t *)&msgParams, MMDL_SCHEDULER_STATUS_LEN,
                    MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                    MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Scheduler Action Status command to the specified destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  Indicates if message that triggered the status was received on unicast.
 *  \param[in] index          Scheduler Register State entry index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlSchedulerSrSendActionStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                     uint16_t appKeyIndex, bool_t recvOnUnicast,
                                     uint8_t index)
{
  meshMsgInfo_t msgInfo = MSG_INFO(MMDL_SCHEDULER_SR_MDL_ID);
  uint8_t msgParams[MMDL_SCHEDULER_ACTION_STATUS_LEN];
  mmdlSchedulerSrDesc_t *pDesc = NULL;

  /* Get the model instance descriptor. */
  mmdlSchedulerSrGetDesc(elementId, &pDesc);

 if(pDesc != NULL)
 {
   /* Fill in the msg info parameters */
   msgInfo.elementId = elementId;
   msgInfo.dstAddr = dstAddr;
   msgInfo.ttl = MESH_USE_DEFAULT_TTL;
   msgInfo.appKeyIndex = appKeyIndex;
   msgInfo.opcode.opcodeBytes[0] = MMDL_SCHEDULER_ACTION_STATUS_OPCODE;

   /* Pack the Action Status fields. */
   mmdlSchedulerPackActionParams(msgParams, index, &(pDesc->registerState[index].regEntry));

   /* Send message to the Mesh Core */
   MeshSendMessage(&msgInfo, (const uint8_t *)&msgParams, MMDL_SCHEDULER_ACTION_STATUS_LEN,
                   MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                   MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));

 }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Scheduler Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlSchedulerSrHandleGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    mmdlSchedulerSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Scheduler Action Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlSchedulerSrHandleActionGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length and parameters */
  if ((pMsg->messageParamsLen == MMDL_SCHEDULER_ACTION_GET_LEN) &&
      (pMsg->pMessageParams[0] <= MMDL_SCHEDULER_REGISTER_ENTRY_MAX))
  {
    /* Send Status message as a response to the Get message */
    mmdlSchedulerSrSendActionStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                                    pMsg->recvOnUnicast, pMsg->pMessageParams[0]);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between a Scheduler Register state and a Generic On Off state as
 *             a result of an updated Scheduler Register state.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveSchedReg2GenOnOff(meshElementId_t tgtElementId, void *pStateValue)
{
  mmdlSchedulerRegisterEntry_t *pSchedRegEntry;

  pSchedRegEntry = (mmdlSchedulerRegisterEntry_t *)pStateValue;

  switch(pSchedRegEntry->action)
  {
    case MMDL_SCHEDULER_ACTION_TURN_OFF:
      MmdlGenOnOffSrSetBoundStateWithTrans(tgtElementId,
                                           MMDL_GEN_ONOFF_STATE_OFF,
                                           pSchedRegEntry->transTime);
      break;
    case MMDL_SCHEDULER_ACTION_TURN_ON:
      MmdlGenOnOffSrSetBoundStateWithTrans(tgtElementId,
                                           MMDL_GEN_ONOFF_STATE_ON,
                                           pSchedRegEntry->transTime);
      break;
    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Resolves a bind between a Scheduler Register state and a Scene Register state as
 *             a result of an updated Scheduler Register state.
 *
 *  \param[in] tgtElementId  Target element identifier.
 *  \param[in] pStateValue   Updated source state value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlBindResolveSchedReg2SceneReg(meshElementId_t tgtElementId, void *pStateValue)
{
  mmdlSchedulerRegisterEntry_t *pSchedRegEntry;

  pSchedRegEntry = (mmdlSchedulerRegisterEntry_t *)pStateValue;

  if((pSchedRegEntry->action == MMDL_SCHEDULER_ACTION_SCENE_RECALL) &&
     (pSchedRegEntry->sceneNumber != MMDL_SCENE_NUM_PROHIBITED))
  {
    MmdlSceneSrRecallSceneWithTrans(tgtElementId,
                                    pSchedRegEntry->sceneNumber,
                                    pSchedRegEntry->transTime);
  }
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Scheduler Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlSchedulerSrInit(void)
{
  mmdlSchedulerSrDesc_t *pDesc = NULL;
  meshElementId_t elemId;
  uint8_t index;

  MMDL_TRACE_INFO0("SCHEDULER SR: init");

  /* Set event callbacks */
  schedulerSrCb.recvCback = MmdlEmptyCback;

  /* Initialize timers */
  for (elemId = 0; elemId < pMeshConfig->elementArrayLen; elemId++)
  {
    /* Get the model instance descriptor */
    mmdlSchedulerSrGetDesc(elemId, &pDesc);

    if (pDesc != NULL)
    {
      for(index = 0; index <= MMDL_SCHEDULER_REGISTER_ENTRY_MAX; index++)
      {
        /* If in use, schedule. */
        if(pDesc->registerState[index].inUse)
        {
          mmdlSchedulerSrScheduleEvent(elemId, index, &(pDesc->registerState[index]));
        }
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Scheduler Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Scheduler Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerSrHandlerInit(wsfHandlerId_t handlerId)
{
  mmdlSchedulerSrDesc_t *pDesc = NULL;
  meshElementId_t elemId;
  uint8_t index;

  /* Initialize timers */
  for (elemId = 0; elemId < pMeshConfig->elementArrayLen; elemId++)
  {
    /* Get the model instance descriptor */
    mmdlSchedulerSrGetDesc(elemId, &pDesc);

    if (pDesc != NULL)
    {
      for(index = 0; index <= MMDL_SCHEDULER_REGISTER_ENTRY_MAX; index++)
      {
        pDesc->registerState[index].inUse = FALSE;
        pDesc->registerState[index].regEntry.action = MMDL_SCHEDULER_ACTION_NONE;
      }
    }
  }

  /* Set handler id */
  mmdlSchedulerSrHandlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Scheduler Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerSrHandler(wsfMsgHdr_t *pMsg)
{
  meshModelEvt_t *pModelMsg;
  uint8_t opcodeIdx;

  /* Handle message */
  if (pMsg != NULL)
  {
    switch (pMsg->event)
    {
      case MESH_MODEL_EVT_MSG_RECV:
        pModelMsg = (meshModelEvt_t *)pMsg;

        for (opcodeIdx = 0; opcodeIdx < MMDL_SCHEDULER_SR_NUM_RCVD_OPCODES; opcodeIdx++)
        {
          if (!memcmp(&mmdlSchedulerSrRcvdOpcodes[opcodeIdx],
                      pModelMsg->msgRecvEvt.opCode.opcodeBytes,
                      MESH_OPCODE_SIZE(pModelMsg->msgRecvEvt.opCode)))
          {
            /* Process message */
            (void)mmdlSchedulerSrHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
          }
        }
        break;

      case MESH_MODEL_EVT_PERIODIC_PUB:
        pModelMsg = (meshModelEvt_t *)pMsg;

        /* Check if periodic publishing was not disabled. */
        if(pModelMsg->periodicPubEvt.nextPubTimeMs != 0)
        {
          /* Publishing is requested part of the periodic publishing. */
          MmdlSchedulerSrPublish(pModelMsg->periodicPubEvt.elementId);
        }
        break;

      default:
        MMDL_TRACE_WARN0("SCHEDULER SR: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publish a Scheduler Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerSrPublish(meshElementId_t elementId)
{
  meshPubMsgInfo_t pubMsgInfo = PUB_MSG_INFO(MMDL_SCHEDULER_SR_MDL_ID);
  uint8_t msgParams[MMDL_SCHEDULER_STATUS_LEN];
  mmdlSchedulerSrDesc_t *pDesc = NULL;
  uint8_t idx;

  /* Get the model instance descriptor */
  mmdlSchedulerSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Fill in the msg info parameters */
    pubMsgInfo.elementId = elementId;
    UINT16_TO_BE_BUF(pubMsgInfo.opcode.opcodeBytes, MMDL_SCHEDULER_STATUS_OPCODE);

    /* Set the schedules bit-field. */
    for(idx = 0; idx <= MMDL_SCHEDULER_REGISTER_ENTRY_MAX; idx++)
    {
      if(pDesc->registerState[idx].inUse)
      {
        msgParams[idx >> 3] = 1 << (idx & 0x07);
      }
    }
    /* Send message to the Mesh Core */
    MeshPublishMessage(&pubMsgInfo, (const uint8_t *)&msgParams, MMDL_SCHEDULER_STATUS_LEN);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerSrRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback*/
  if (recvCback != NULL)
  {
    schedulerSrCb.recvCback = recvCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Triggers the action associated to the scheduled event identified by the id field.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] id         Identifier of the scheduled event.
 *                        See ::mmdlSchedulerSrStartScheduleEvent_t
 *
 *  \return    None.
 *
 *  \remarks   Upon receiving a ::mmdlSchedulerSrStartScheduleEvent_t the application can start
 *             scheduling an event and call this function to perform the associated action. If the
 *             event is not periodical then ::MmdlSchedulerSrClearEvent must be called.
 */
/*************************************************************************************************/
void MmdlSchedulerSrTriggerEventAction(meshElementId_t elementId, uint8_t id)
{
  mmdlSchedulerSrDesc_t *pDesc = NULL;

  /* Get the model instance descriptor */
  mmdlSchedulerSrGetDesc(elementId, &pDesc);

  if((pDesc != NULL) && (id <= MMDL_SCHEDULER_REGISTER_ENTRY_MAX))
  {
    if(!(pDesc->registerState[id].inUse))
    {
      return;
    }

    /* Check action type. */
    switch(pDesc->registerState[id].regEntry.action)
    {
      case MMDL_SCHEDULER_ACTION_TURN_OFF:
      case MMDL_SCHEDULER_ACTION_TURN_ON:
      case MMDL_SCHEDULER_ACTION_SCENE_RECALL:
        MmdlBindResolve(elementId, MMDL_STATE_SCH_REG, &(pDesc->registerState[id].regEntry));
        break;
      default:
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Clears the scheduled event identified by the id field.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] id         Identifier of the scheduled event.
 *                        See ::mmdlSchedulerSrStartScheduleEvent_t
 *
 *  \return    None.
 *
 *  \remarks   This function should be called if the application cannot schedule an event or if
 *             the event is not periodical and the scheduled time elapsed.
 */
/*************************************************************************************************/
void MmdlSchedulerSrClearEvent(meshElementId_t elementId, uint8_t id)
{
   mmdlSchedulerSrDesc_t *pDesc = NULL;

   /* Get the model instance descriptor */
   mmdlSchedulerSrGetDesc(elementId, &pDesc);

   /* Clear entry. */
   if((pDesc != NULL) && (id <= MMDL_SCHEDULER_REGISTER_ENTRY_MAX))
   {
     pDesc->registerState[id].inUse = FALSE;
     memset(&pDesc->registerState[id].regEntry, 0, sizeof(mmdlSchedulerRegisterEntry_t));
     pDesc->registerState[id].regEntry.action = MMDL_SCHEDULER_ACTION_NONE;
   }
}

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Scheduler Register State and a Generic On Off state.
 *
 *  \param[in] schedElemId  Element identifier where the Scheduler Register state resides.
 *  \param[in] onoffElemId  Element identifier where the On Off state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerSrBind2GenOnOff(meshElementId_t schedElemId, meshElementId_t onoffElemId)
{
  /* Add Scheduler Register -> Generic On Off binding */
  MmdlAddBind(MMDL_STATE_SCH_REG, MMDL_STATE_GEN_ONOFF, schedElemId, onoffElemId,
              mmdlBindResolveSchedReg2GenOnOff);
}

/*************************************************************************************************/
/*!
 *  \brief     Creates a bind between a Scheduler Register State and a Scene Register state.
 *
 *  \param[in] schedElemId  Element identifier where the Scheduler Register state resides.
 *  \param[in] sceneElemId  Element identifier where the Scene Register state resides.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSchedulerSrBind2SceneReg(meshElementId_t schedElemId, meshElementId_t sceneElemId)
{
  /* Add Scheduler Register -> Scene Register binding */
  MmdlAddBind(MMDL_STATE_SCH_REG, MMDL_STATE_SCENE_REG, schedElemId, sceneElemId,
              mmdlBindResolveSchedReg2SceneReg);
}
