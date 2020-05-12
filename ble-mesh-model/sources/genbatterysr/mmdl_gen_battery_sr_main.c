/*************************************************************************************************/
/*!
 *  \file   mmdl_gen_battery_sr_main.c
 *
 *  \brief  Implementation of the Generic Battery Server model.
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
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"

#include "mmdl_types.h"
#include "mmdl_defs.h"
#include "mmdl_common.h"
#include "mmdl_gen_battery_sr_main.h"
#include "mmdl_gen_battery_sr_api.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Present state index in stored states */
#define PRESENT_STATE_IDX                   0

/*! Target state index in stored states */
#define TARGET_STATE_IDX                    1

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Generic Battery Server control block type definition */
typedef struct mmdlGenBatterySrCb_tag
{
  mmdlEventCback_t recvCback;      /*!< Model Generic Battery received callback */
} mmdlGenBatterySrCb_t;

/*! Generic Battery Server message handler type definition */
typedef void (*mmdlGenBatterySrHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler ID */
wsfHandlerId_t mmdlGenBatterySrHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlGenBatterySrRcvdOpcodes[MMDL_GEN_BATTERY_SR_NUM_RCVD_OPCODES] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_BATTERY_GET_OPCODE)} },
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/* Handler functions for supported opcodes */
const mmdlGenBatterySrHandleMsg_t mmdlGenBatterySrHandleMsg[MMDL_GEN_BATTERY_SR_NUM_RCVD_OPCODES] =
{
  mmdlGenBatterySrHandleGet
};

/*! Generic Battery Server Control Block */
static mmdlGenBatterySrCb_t  batterySrCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Searches for the Generic Battery model instance descriptor on the specified element.
 *
 *  \param[in]  elementId  Identifier of the Element implementing the model.
 *  \param[out] ppOutDesc  Double pointer to the descriptor.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void mmdlGenBatterySrGetDesc(meshElementId_t elementId, mmdlGenBatterySrDesc_t **ppOutDesc)
{
  uint8_t modelIdx;

  *ppOutDesc = NULL;

  /* Verify that the elementId is in the element array */
  if (elementId < pMeshConfig->elementArrayLen)
  {
    /* Look for the model instance */
    for (modelIdx = 0; modelIdx < pMeshConfig->pElementArray[elementId].numSigModels; modelIdx++)
    {
      if (pMeshConfig->pElementArray[elementId].pSigModelArray[modelIdx].modelId ==
          MMDL_GEN_BATTERY_SR_MDL_ID)
      {
        /* Matching model ID on elementId */
        *ppOutDesc = pMeshConfig->pElementArray[elementId].pSigModelArray[modelIdx].pModelDescriptor;
        break;
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model.
 *  \param[in] pTargetState    Target State for this transaction. See ::mmdlGenBatteryState_t.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenBatterySrSetState(meshElementId_t elementId,
                                     const mmdlGenBatteryState_t *pTargetState,
                                     mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlGenBatterySrEvent_t event;
  mmdlGenBatterySrDesc_t *pDesc = NULL;

  MMDL_TRACE_INFO1("BATTERY SR: Set State on elemId %d", elementId);

  /* Get model instance descriptor */
  mmdlGenBatterySrGetDesc(elementId, &pDesc);

  if (pDesc == NULL)
  {
    event.hdr.status = MMDL_INVALID_ELEMENT;
  }
  else
  {
    event.hdr.status = MMDL_SUCCESS;

    /* Update State */
    pDesc->pStoredStates[PRESENT_STATE_IDX] = *pTargetState;
  }

  /* Set event type */
  event.hdr.event = MMDL_GEN_BATTERY_SR_EVENT;
  event.hdr.param = MMDL_GEN_BATTERY_SR_STATE_UPDATE_EVENT;

  /* Set event parameters */
  event.statusEvent.elemId = elementId;
  event.statusEvent.state = *pTargetState;
  event.statusEvent.stateUpdateSource = stateUpdateSrc;

  /* Send event to the upper layer */
  batterySrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Generic Battery Status command to the specified destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  Indicates if message that triggered the status was received on unicast.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenBatterySrSendStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                       uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_BATTERY_SR_MDL_ID, MMDL_GEN_BATTERY_STATUS_OPCODE);
  uint8_t msgParams[MMDL_GEN_BATTERY_STATUS_LENGTH];
  mmdlGenBatterySrDesc_t *pDesc = NULL;
  uint8_t *pParams;

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = dstAddr;
  msgInfo.ttl = MESH_USE_DEFAULT_TTL;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Get the model instance descriptor */
  mmdlGenBatterySrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    pParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT8_TO_BSTREAM(pParams, pDesc->pStoredStates[PRESENT_STATE_IDX].batteryLevel);
    UINT24_TO_BSTREAM(pParams, pDesc->pStoredStates[PRESENT_STATE_IDX].timeToDischarge);
    UINT24_TO_BSTREAM(pParams, pDesc->pStoredStates[PRESENT_STATE_IDX].timeToCharge);
    UINT8_TO_BSTREAM(pParams, pDesc->pStoredStates[PRESENT_STATE_IDX].flags);

    /* Send message to the Mesh Core instantly */
    MeshSendMessage(&msgInfo, msgParams, MMDL_GEN_BATTERY_STATUS_LENGTH,
                    MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                    MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Battery Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenBatterySrHandleGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    mmdlGenBatterySrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Generic Battery Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlGenBatterySrInit(void)
{
  MMDL_TRACE_INFO0("BATTERY SR: init");

  /* Set event callbacks */
  batterySrCb.recvCback = MmdlEmptyCback;
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Generic Battery Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Generic Battery Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenBatterySrHandlerInit(wsfHandlerId_t handlerId)
{
  /* Set handler ID */
  mmdlGenBatterySrHandlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Generic Battery Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenBatterySrHandler(wsfMsgHdr_t *pMsg)
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

        /* Validate opcode size and value */
        if (MESH_OPCODE_SIZE(pModelMsg->msgRecvEvt.opCode) == MMDL_GEN_BATTERY_OPCODES_SIZE)
        {
          /* Match the received opcode */
          for (opcodeIdx = 0; opcodeIdx < MMDL_GEN_BATTERY_SR_NUM_RCVD_OPCODES; opcodeIdx++)
          {
            if (!memcmp(&mmdlGenBatterySrRcvdOpcodes[opcodeIdx], pModelMsg->msgRecvEvt.opCode.opcodeBytes,
                        MMDL_GEN_BATTERY_OPCODES_SIZE))
            {
              /* Process message */
              (void)mmdlGenBatterySrHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
            }
          }
        }
        break;

      case MESH_MODEL_EVT_PERIODIC_PUB:
        pModelMsg = (meshModelEvt_t *)pMsg;

        /* Check if periodic publishing was not disabled. */
        if (pModelMsg->periodicPubEvt.nextPubTimeMs != 0)
        {
          /* Publishing is requested part of the periodic publishing */
          MmdlGenBatterySrPublish(pModelMsg->periodicPubEvt.elementId);
        }
        break;

      default:
        MMDL_TRACE_WARN0("GEN BATTERY SR: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publish a Gen Battery Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenBatterySrPublish(meshElementId_t elementId)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_GEN_BATTERY_SR_MDL_ID,
                                             MMDL_GEN_BATTERY_STATUS_OPCODE);
  uint8_t msgParams[MMDL_GEN_BATTERY_STATUS_LENGTH];
  mmdlGenBatterySrDesc_t *pDesc = NULL;
  uint8_t *pParams;

  /* Fill in the msg info parameters */
  pubMsgInfo.elementId = elementId;

  /* Get the model instance descriptor */
  mmdlGenBatterySrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    pParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT8_TO_BSTREAM(pParams, pDesc->pStoredStates[PRESENT_STATE_IDX].batteryLevel);
    UINT24_TO_BSTREAM(pParams, pDesc->pStoredStates[PRESENT_STATE_IDX].timeToDischarge);
    UINT24_TO_BSTREAM(pParams, pDesc->pStoredStates[PRESENT_STATE_IDX].timeToCharge);
    UINT8_TO_BSTREAM(pParams, pDesc->pStoredStates[PRESENT_STATE_IDX].flags);

    /* Send message to the Mesh Core */
    MeshPublishMessage(&pubMsgInfo, msgParams, MMDL_GEN_BATTERY_STATUS_LENGTH);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Set the local Generic Battery state of the element.
 *
 *  \param[in] elementId     Identifier of the Element implementing the model.
 *  \param[in] pTargetState  Target State for this transaction. See ::mmdlGenBatteryState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenBatterySrSetState(meshElementId_t elementId,
                              const mmdlGenBatteryState_t *pTargetState)
{
    /* Change state locally. No transition time or delay required. */
    mmdlGenBatterySrSetState(elementId, pTargetState, MMDL_STATE_UPDATED_BY_APP);
}

/*************************************************************************************************/
/*!
 *  \brief     Get the local Generic Battery state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenBatterySrGetState(meshElementId_t elementId)
{
  mmdlGenBatterySrEvent_t event;
  mmdlGenBatterySrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlGenBatterySrGetDesc(elementId, &pDesc);

  /* Set event type */
  event.hdr.event = MMDL_GEN_BATTERY_SR_EVENT;
  event.hdr.param = MMDL_GEN_BATTERY_SR_CURRENT_STATE_EVENT;

  if (pDesc == NULL)
  {
    /* No descriptor found on element */
    event.hdr.status = MMDL_INVALID_ELEMENT;
  }
  else
  {
    /* Descriptor found on element */
    event.hdr.status = MMDL_SUCCESS;

    /* Set event parameters */
    event.statusEvent.elemId = elementId;
    event.currentStateEvent.state = pDesc->pStoredStates[PRESENT_STATE_IDX];
  }

  /* Send event to the upper layer */
  batterySrCb.recvCback((wsfMsgHdr_t *)&event);
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
void MmdlGenBatterySrRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback*/
  if (recvCback != NULL)
  {
    batterySrCb.recvCback = recvCback;
  }
}
