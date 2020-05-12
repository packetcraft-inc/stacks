/*************************************************************************************************/
/*!
 *  \file   mesh_ht_cl_main.c
 *
 *  \brief  Implementation of the Health Client model.
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

#include "mmdl_defs.h"
#include "mmdl_types.h"
#include "mmdl_common.h"
#include "mesh_ht_cl_api.h"
#include "mesh_ht_cl_main.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Health Client control block type definition */
typedef struct meshHtClCb_tag
{
  mmdlEventCback_t recvCback;    /*!< Health Client event callback */
} meshHtClCb_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler id */
wsfHandlerId_t meshHtClHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t meshHtClRcvdOpcodes[] =
{
  { MESH_HT_CRT_STATUS_OPCODE },
  { MESH_HT_FAULT_STATUS_OPCODE },
  { MESH_HT_PERIOD_STATUS_OPCODE },
  { MESH_HT_ATTENTION_STATUS_OPCODE }
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Number of operations handled by the Health Client */
static const uint8_t htClNumOps = sizeof(meshHtClRcvdOpcodes) / sizeof(meshMsgOpcode_t);

/*! Handler functions for supported opcodes */
static const meshHtClHandleMsg_t meshHtClHandleMsg[] =
{
  meshHtClHandleCurrentFaultStatus,
  meshHtClHandleFaultStatus,
  meshHtClHandlePeriodStatus,
  meshHtClHandleAttentionStatus,
};

/*! Health Client control block */
static meshHtClCb_t  htClCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Handles Health Current and Health Fault Status messages.
 *
 *  \param[in] pMsg   Received model message.
 *  \param[in] isCrt  TRUE if status is Health Current Status, FALSE if it is Health Fault Status.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void meshHtClHandleCrtAndRegStatus(const meshModelMsgRecvEvt_t *pMsg, bool_t isCrt)
{
  meshHtClFaultStatusEvt_t evt;
  uint8_t *pParams;

  /* Validate message length. At least test ID and company ID must be present. */
  if ((pMsg->messageParamsLen < (sizeof(meshHtMdlTestId_t) + sizeof(uint16_t))) ||
      (pMsg->pMessageParams == NULL))
  {
    return;
  }

  /* Extract status event parameters. */
  pParams = pMsg->pMessageParams;

  /* Extract Test ID. */
  BSTREAM_TO_UINT8(evt.healthStatus.testId, pParams);
  /* Extract Company ID. */
  BSTREAM_TO_UINT16(evt.healthStatus.companyId, pParams);

  /* Calculate number of faults from message length. */
  evt.healthStatus.faultIdArrayLen = pMsg->messageParamsLen -
                                     (sizeof(meshHtMdlTestId_t) + sizeof(uint16_t));

  /* Point to fault array. */
  if(evt.healthStatus.faultIdArrayLen == 0)
  {
    evt.healthStatus.pFaultIdArray = NULL;
  }
  else
  {
    evt.healthStatus.pFaultIdArray = pParams;
  }

  /* Set event type and status. */
  evt.hdr.event = MESH_HT_CL_EVENT;
  evt.hdr.param = isCrt ? MESH_HT_CL_CURRENT_STATUS_EVENT : MESH_HT_CL_FAULT_STATUS_EVENT;
  evt.hdr.status = MESH_HT_CL_SUCCESS;

  /* Set addressing information. */
  evt.elemId = pMsg->elementId;
  evt.htSrElemAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  htClCb.recvCback((wsfMsgHdr_t *)&evt);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Health Current Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshHtClHandleCurrentFaultStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  meshHtClHandleCrtAndRegStatus(pMsg, TRUE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Health Fault Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshHtClHandleFaultStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  meshHtClHandleCrtAndRegStatus(pMsg, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Health Period Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshHtClHandlePeriodStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  meshHtClPeriodStatusEvt_t evt;

  /* Validate message length. */
  if ((pMsg->messageParamsLen != sizeof(meshHtPeriod_t)) ||
      (pMsg->pMessageParams == NULL))
  {
    return;
  }

  /* Extract status event parameters. */
  evt.periodDivisor = pMsg->pMessageParams[0];

  /* Validate value. */
  if(evt.periodDivisor > MESH_HT_PERIOD_MAX_VALUE)
  {
    return;
  }

  /* Set event type and status */
  evt.hdr.event = MESH_HT_CL_EVENT;
  evt.hdr.param = MESH_HT_CL_PERIOD_STATUS_EVENT;
  evt.hdr.status = MESH_HT_CL_SUCCESS;

  /* Set event contents */
  evt.elemId = pMsg->elementId;
  evt.htSrElemAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  htClCb.recvCback((wsfMsgHdr_t *)&evt);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Health Attention Status message.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void meshHtClHandleAttentionStatus(const meshModelMsgRecvEvt_t *pMsg)
{
  meshHtClAttentionStatusEvt_t evt;

  /* Validate message length. */
  if ((pMsg->messageParamsLen != sizeof(meshHtAttTimer_t)) ||
      (pMsg->pMessageParams == NULL))
  {
    return;
  }

  /* Extract status event parameters. */
  evt.attTimerState = pMsg->pMessageParams[0];

  /* Set event type and status */
  evt.hdr.event = MESH_HT_CL_EVENT;
  evt.hdr.param = MESH_HT_CL_ATTENTION_STATUS_EVENT;
  evt.hdr.status = MESH_HT_CL_SUCCESS;

  /* Set event contents */
  evt.elemId = pMsg->elementId;
  evt.htSrElemAddr = pMsg->srcAddr;

  /* Send event to the upper layer */
  htClCb.recvCback((wsfMsgHdr_t *)&evt);
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Health Client Handler.
 *
 *  \param[in] handlerId  WSF handler ID of the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtClHandlerInit(wsfHandlerId_t handlerId)
{
  /* Set handler ID */
  meshHtClHandlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Health Client model.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshHtClInit(void)
{
  /* Initialize control block */
  htClCb.recvCback = MmdlEmptyCback;
}

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Health Client model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtClHandler(wsfMsgHdr_t *pMsg)
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
        /* Match the received opcode. */
        for (opcodeIdx = 0; opcodeIdx < htClNumOps; opcodeIdx++)
        {
          /* Validate opcode size and value */
          if (MESH_OPCODE_SIZE(pModelMsg->opCode) !=
              MESH_OPCODE_SIZE(meshHtClRcvdOpcodes[opcodeIdx]))
          {
            continue;
          }
          if (!memcmp(&meshHtClRcvdOpcodes[opcodeIdx], &(pModelMsg->opCode),
                      MESH_OPCODE_SIZE(meshHtClRcvdOpcodes[opcodeIdx])))
          {
            /* Process message. */
            meshHtClHandleMsg[opcodeIdx]((const meshModelMsgRecvEvt_t *)pModelMsg);
            return;
          }
        }
        break;

      default:
        MESH_TRACE_WARN0("HT CL: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback that is triggered when a message is received for this model.
 *
 *  \param[in] evtCback  Callback registered by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtClRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback. */
  if (recvCback != NULL)
  {
    htClCb.recvCback = recvCback;
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the current Registered Fault state identified by the company ID.
 *
 *  \param[in] elementId     Local element identifier for a Health Client model instance.
 *  \param[in] htSrElemAddr  Address of the element containing an instance of Health Server model.
 *  \param[in] appKeyIndex   AppKey Index.
 *  \param[in] ttl           TTL or ::MESH_USE_DEFAULT_TTL.
 *  \param[in] companyId     16-bit Bluetooth assigned Company Identifier.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtClFaultGet(meshElementId_t elementId, meshAddress_t htSrElemAddr, uint16_t appKeyIndex,
                      uint8_t ttl, uint16_t companyId)
{
  uint8_t msgParam[sizeof(uint16_t)];

  /* Configure message. */
  meshMsgInfo_t msgInfo =
  {
      .modelId.sigModelId = MESH_HT_CL_MDL_ID,
      .opcode = { MESH_HT_FAULT_GET_OPCODE },
      .elementId = elementId,
      .dstAddr = htSrElemAddr,
      .pDstLabelUuid = NULL,
      .appKeyIndex = appKeyIndex,
      .ttl = ttl
  };

  /* Pack message parameters. */
  UINT16_TO_BUF(msgParam, companyId);

  /* Use Mesh API to send message. */
  MeshSendMessage((const meshMsgInfo_t *)&msgInfo, (const uint8_t *)msgParam, sizeof(msgParam),
                  0, 0);
}

/*************************************************************************************************/
/*!
 *  \brief     Clears the current Registered Fault state identified by the company ID.
 *
 *  \param[in] elementId     Local element identifier for a Health Client model instance.
 *  \param[in] htSrElemAddr  Address of the element containing an instance of Health Server model.
 *  \param[in] appKeyIndex   AppKey Index.
 *  \param[in] ttl           TTL or ::MESH_USE_DEFAULT_TTL.
 *  \param[in] companyId     16-bit Bluetooth assigned Company Identifier.
 *  \param[in] ackRequired   TRUE if request type is acknowledged.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtClFaultClear(meshElementId_t elementId, meshAddress_t htSrElemAddr,
                        uint16_t appKeyIndex, uint8_t ttl, uint16_t companyId, bool_t ackRequired)
{
  uint8_t msgParam[sizeof(uint16_t)];

  const meshMsgOpcode_t opcode = { MESH_HT_FAULT_CLEAR_OPCODE };
  const meshMsgOpcode_t unackOpcode = { MESH_HT_FAULT_CLEAR_UNACK_OPCODE };

  /* Configure message. */
  meshMsgInfo_t msgInfo =
  {
      .modelId.sigModelId = MESH_HT_CL_MDL_ID,
      .elementId = elementId,
      .dstAddr = htSrElemAddr,
      .pDstLabelUuid = NULL,
      .appKeyIndex = appKeyIndex,
      .ttl = ttl
  };

  if(ackRequired)
  {
    msgInfo.opcode = opcode;
  }
  else
  {
    msgInfo.opcode = unackOpcode;
  }

  /* Pack message parameters. */
  UINT16_TO_BUF(msgParam, companyId);

  /* Use Mesh API to send message. */
  MeshSendMessage((const meshMsgInfo_t *)&msgInfo, (const uint8_t *)msgParam, sizeof(msgParam),
                  0, 0);
}

/*************************************************************************************************/
/*!
 *  \brief     Invokes a self-test procedure on an element implementing a Health Server.
 *
 *  \param[in] elementId     Local element identifier for a Health Client model instance.
 *  \param[in] htSrElemAddr  Address of the element containing an instance of Health Server model.
 *  \param[in] appKeyIndex   AppKey Index.
 *  \param[in] ttl           TTL or ::MESH_USE_DEFAULT_TTL.
 *  \param[in] testId        Identifier of a specific test to be performed.
 *  \param[in] companyId     16-bit Bluetooth assigned Company Identifier.
 *  \param[in] ackRequired   TRUE if request type is acknowledged.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtClFaultTest(meshElementId_t elementId, meshAddress_t htSrElemAddr, uint16_t appKeyIndex,
                       uint8_t ttl, meshHtMdlTestId_t testId, uint16_t companyId,
                       bool_t ackRequired)
{
  uint8_t *pMsgParam;
  uint8_t msgParam[sizeof(meshHtMdlTestId_t) + sizeof(uint16_t)];

  const meshMsgOpcode_t opcode = { MESH_HT_FAULT_TEST_OPCODE };
  const meshMsgOpcode_t unackOpcode = { MESH_HT_FAULT_TEST_UNACK_OPCODE };

  /* Configure message. */
  meshMsgInfo_t msgInfo =
  {
      .modelId.sigModelId = MESH_HT_CL_MDL_ID,
      .elementId = elementId,
      .dstAddr = htSrElemAddr,
      .pDstLabelUuid = NULL,
      .appKeyIndex = appKeyIndex,
      .ttl = ttl
  };

  if(ackRequired)
  {
    msgInfo.opcode = opcode;
  }
  else
  {
    msgInfo.opcode = unackOpcode;
  }

  pMsgParam = msgParam;

  /* Pack message parameters. */
  UINT8_TO_BSTREAM(pMsgParam, testId);
  UINT16_TO_BUF(pMsgParam, companyId);

  /* Use Mesh API to send message. */
  MeshSendMessage((const meshMsgInfo_t *)&msgInfo, (const uint8_t *)msgParam, sizeof(msgParam),
                  0, 0);
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the current Health Period state of an element.
 *
 *  \param[in] elementId     Local element identifier for a Health Client model instance.
 *  \param[in] htSrElemAddr  Address of the element containing an instance of Health Server model.
 *  \param[in] appKeyIndex   AppKey Index.
 *  \param[in] ttl           TTL or ::MESH_USE_DEFAULT_TTL.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtClPeriodGet(meshElementId_t elementId, meshAddress_t htSrElemAddr, uint16_t appKeyIndex,
                       uint8_t ttl)
{
  /* Configure message. */
  meshMsgInfo_t msgInfo =
  {
      .modelId.sigModelId = MESH_HT_CL_MDL_ID,
      .opcode = { MESH_HT_PERIOD_GET_OPCODE },
      .elementId = elementId,
      .dstAddr = htSrElemAddr,
      .pDstLabelUuid = NULL,
      .appKeyIndex = appKeyIndex,
      .ttl = ttl
  };

  /* Use Mesh API to send message. */
  MeshSendMessage((const meshMsgInfo_t *)&msgInfo, (const uint8_t *)NULL, 0, 0, 0);
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the current Health Period state of an element.
 *
 *  \param[in] elementId     Local element identifier for a Health Client model instance.
 *  \param[in] htSrElemAddr  Address of the element containing an instance of Health Server model.
 *  \param[in] appKeyIndex   AppKey Index.
 *  \param[in] ttl           TTL or ::MESH_USE_DEFAULT_TTL.
 *  \param[in] periodState   Health period divisor state.
 *  \param[in] ackRequired   TRUE if request type is acknowledged.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtClPeriodSet(meshElementId_t elementId, meshAddress_t htSrElemAddr, uint16_t appKeyIndex,
                       uint8_t ttl, meshHtPeriod_t periodState, bool_t ackRequired)
{
  const meshMsgOpcode_t opcode = { MESH_HT_PERIOD_SET_OPCODE };
  const meshMsgOpcode_t unackOpcode = { MESH_HT_PERIOD_SET_UNACK_OPCODE };

  /* Validate state. */
  if(periodState > MESH_HT_PERIOD_MAX_VALUE)
  {
    return;
  }

  /* Configure message. */
  meshMsgInfo_t msgInfo =
  {
      .modelId.sigModelId = MESH_HT_CL_MDL_ID,
      .elementId = elementId,
      .dstAddr = htSrElemAddr,
      .pDstLabelUuid = NULL,
      .appKeyIndex = appKeyIndex,
      .ttl = ttl
  };

  if(ackRequired)
  {
    msgInfo.opcode = opcode;
  }
  else
  {
    msgInfo.opcode = unackOpcode;
  }

  /* Use Mesh API to send message. */
  MeshSendMessage((const meshMsgInfo_t *)&msgInfo, (const uint8_t *)&periodState,
                  sizeof(periodState), 0, 0);
}

/*************************************************************************************************/
/*!
 *  \brief     Gets the current Attention Timer state of an element.
 *
 *  \param[in] elementId     Local element identifier for a Health Client model instance.
 *  \param[in] htSrElemAddr  Address of the element containing an instance of Health Server model.
 *  \param[in] appKeyIndex   AppKey Index.
 *  \param[in] ttl           TTL or ::MESH_USE_DEFAULT_TTL.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtClAttentionGet(meshElementId_t elementId, meshAddress_t htSrElemAddr,
                          uint16_t appKeyIndex, uint8_t ttl)
{
  /* Configure message. */
  meshMsgInfo_t msgInfo =
  {
      .modelId.sigModelId = MESH_HT_CL_MDL_ID,
      .opcode = { MESH_HT_ATTENTION_GET_OPCODE },
      .elementId = elementId,
      .dstAddr = htSrElemAddr,
      .pDstLabelUuid = NULL,
      .appKeyIndex = appKeyIndex,
      .ttl = ttl
  };

  /* Use Mesh API to send message. */
  MeshSendMessage((const meshMsgInfo_t *)&msgInfo, (const uint8_t *)NULL, 0, 0, 0);
}

/*************************************************************************************************/
/*!
 *  \brief     Sets the current Attention Timer state of an element.
 *
 *  \param[in] elementId      Local element identifier for a Health Client model instance.
 *  \param[in] htSrElemAddr   Address of the element containing an instance of Health Server model.
 *  \param[in] appKeyIndex    AppKey Index.
 *  \param[in] ttl            TTL or ::MESH_USE_DEFAULT_TTL.
 *  \param[in] attTimerState  Attention timer state.
 *  \param[in] ackRequired    TRUE if request type is acknowledged.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MeshHtClAttentionSet(meshElementId_t elementId, meshAddress_t htSrElemAddr,
                          uint16_t appKeyIndex, uint8_t ttl, meshHtAttTimer_t  attTimerState,
                          bool_t ackRequired)
{
  const meshMsgOpcode_t opcode = { MESH_HT_ATTENTION_SET_OPCODE };
  const meshMsgOpcode_t unackOpcode = { MESH_HT_ATTENTION_SET_UNACK_OPCODE };

  /* Configure message. */
  meshMsgInfo_t msgInfo =
  {
      .modelId.sigModelId = MESH_HT_CL_MDL_ID,
      .elementId = elementId,
      .dstAddr = htSrElemAddr,
      .pDstLabelUuid = NULL,
      .appKeyIndex = appKeyIndex,
      .ttl = ttl
  };

  if(ackRequired)
  {
    msgInfo.opcode = opcode;
  }
  else
  {
    msgInfo.opcode = unackOpcode;
  }

  /* Use Mesh API to send message. */
  MeshSendMessage((const meshMsgInfo_t *)&msgInfo, (const uint8_t *)&attTimerState,
                  sizeof(attTimerState), 0, 0);
}
