/*************************************************************************************************/
/*!
 *  \file   mmdl_gen_powonoff_sr_main.c
 *
 *  \brief  Implementation of the Generic Power OnOff Server model.
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

#include "mmdl_gen_powonoff_sr.h"
#include "mmdl_gen_powonoff_sr_main.h"
#include "mmdl_gen_powonoff_sr_api.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Present state index in stored states */
#define PRESENT_STATE_IDX             0

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Generic Power OnOff Server control block type definition */
typedef struct mmdlGenPowOnOffSrCb_tag
{
  mmdlBindResolve_t fResolveBind;     /*!< Pointer to the function that checks
                                       *   and resolves a bind triggered by a
                                       *   change in this model instance
                                       */
  mmdlEventCback_t  recvCback;        /*!< Model Generic Power OnOff received callback */
} mmdlGenPowOnOffSrCb_t;

/*! Generic Power OnOff Server message handler type definition */
typedef void (*mmdlGenPowOnOffSrHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler ID */
wsfHandlerId_t mmdlGenPowOnOffSrHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlGenPowOnOffSrRcvdOpcodes[MMDL_GEN_POWER_ONOFF_SR_NUM_RCVD_OPCODES] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_ONPOWERUP_GET_OPCODE)} }
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlGenPowOnOffSrHandleMsg_t mmdlGenPowOnOffSrHandleMsg[MMDL_GEN_POWER_ONOFF_SR_NUM_RCVD_OPCODES] =
{
  mmdlGenPowOnOffSrHandleGet,
};

/*! Generic Power OnOff Server Control Block */
static mmdlGenPowOnOffSrCb_t  powOnOffSrCb;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Searches for the Generic Power OnOff model instance descriptor on the specified element.
 *
 *  \param[in]  elementId  Identifier of the Element implementing the model.
 *  \param[out] ppOutDesc  Double pointer to the descriptor.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void mmdlGenPowOnOffSrGetDesc(meshElementId_t elementId, mmdlGenPowOnOffSrDesc_t **ppOutDesc)
{
  uint8_t modelIdx;

  *ppOutDesc = NULL;

  if (elementId > pMeshConfig->elementArrayLen)
  {
    return;
  }

  /* Look for the model instance */
  for (modelIdx = 0; modelIdx < pMeshConfig->pElementArray[elementId].numSigModels; modelIdx ++)
  {
    if (pMeshConfig->pElementArray[elementId].pSigModelArray[modelIdx].modelId ==
        MMDL_GEN_POWER_ONOFF_SR_MDL_ID)
    {
      /* Matching model ID on elementId */
      *ppOutDesc = pMeshConfig->pElementArray[elementId].pSigModelArray[modelIdx].pModelDescriptor;
      break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Generic Power OnOff Status command to the specified destination address.
 *
 *  \param[in] modelId      Model identifier.
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] dstAddr      Element address of the destination
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffSrSendStatus(uint16_t modelId, meshElementId_t elementId,
                                 meshAddress_t dstAddr, uint16_t appKeyIndex)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(modelId, MMDL_GEN_ONPOWERUP_STATUS_OPCODE);
  uint8_t msgParams[MMDL_GEN_POWER_ONOFF_MSG_LEN];
  mmdlGenPowOnOffSrDesc_t *pDesc = NULL;

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = dstAddr;
  msgInfo.ttl = MESH_USE_DEFAULT_TTL;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Get the model instance descriptor */
  mmdlGenPowOnOffSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Copy the message parameters from the descriptor */
    msgParams[0] = pDesc->pStoredStates[PRESENT_STATE_IDX];

    /* Send message to the Mesh Core instantly */
    MeshSendMessage(&msgInfo, msgParams, MMDL_GEN_POWER_ONOFF_MSG_LEN, 20, 50);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Power OnOff Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    TRUE if handled successful, FALSE otherwise.
 */
/*************************************************************************************************/
void mmdlGenPowOnOffSrHandleGet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Validate message length */
  if (pMsg->messageParamsLen == 0)
  {
    /* Send Status message as a response to the Get message */
    MmdlGenPowOnOffSrSendStatus(MMDL_GEN_POWER_ONOFF_SR_MDL_ID, pMsg->elementId, pMsg->srcAddr,
                                pMsg->appKeyIndex);
  }
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Generic Power OnOff Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffSrInit(void)
{
  MMDL_TRACE_INFO0("GEN POWER ONOFF SR: init");

  /* Set event callbacks */
  powOnOffSrCb.recvCback = MmdlEmptyCback;
  powOnOffSrCb.fResolveBind = MmdlBindResolve;
}

/*************************************************************************************************/
/*!
 *  \brief  Execute the PowerUp procedure.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffOnPowerUp(void)
{
  uint8_t elemIdx;
  mmdlGenPowOnOffSrDesc_t *pDesc = NULL;

  if (powOnOffSrCb.fResolveBind)
  {
    for (elemIdx = 0; elemIdx < pMeshConfig->elementArrayLen; elemIdx ++)
    {
      /* Get the model instance descriptor */
      mmdlGenPowOnOffSrGetDesc(elemIdx, &pDesc);

      if ((pDesc != NULL) &&(pDesc->pStoredStates != NULL))
      {
        powOnOffSrCb.fResolveBind(elemIdx, MMDL_STATE_GEN_ONPOWERUP, &pDesc->pStoredStates[PRESENT_STATE_IDX]);
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Generic Power OnOff Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Generic Power OnOff Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffSrHandlerInit(wsfHandlerId_t handlerId)
{
  /* Set handler ID */
  mmdlGenPowOnOffSrHandlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *
 *  \brief     WSF message handler for Generic Power OnOff Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffSrHandler(wsfMsgHdr_t *pMsg)
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

        /* Validate opcode size and value */
        if (MESH_OPCODE_SIZE(pModelMsg->opCode) == MMDL_GEN_POWER_ONOFF_OPCODES_SIZE)
        {
          /* Match the received opcode */
          for (opcodeIdx = 0; opcodeIdx < MMDL_GEN_POWER_ONOFF_SR_NUM_RCVD_OPCODES; opcodeIdx++)
          {
            if (!memcmp(&mmdlGenPowOnOffSrRcvdOpcodes[opcodeIdx], pModelMsg->opCode.opcodeBytes,
                        MMDL_GEN_POWER_ONOFF_OPCODES_SIZE))
            {
              /* Process message */
              (void)mmdlGenPowOnOffSrHandleMsg[opcodeIdx](pModelMsg);
            }
          }
        }
        break;

      case MESH_MODEL_EVT_PERIODIC_PUB:
        pModelMsg = (meshModelMsgRecvEvt_t *)pMsg;

        /* Publishing is requested part of the periodic publishing */
        MmdlGenPowOnOffSrPublish(pModelMsg->elementId);
        break;

      default:
        MMDL_TRACE_WARN0("GEN POWER ONOFF SR: Invalid event message received!");
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Publish a GenPowOnOff Status message to the subscription list.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffSrPublish(meshElementId_t elementId)
{
  meshPubMsgInfo_t pubMsgInfo = MESH_PUB_MSG_INFO(MMDL_GEN_POWER_ONOFF_SR_MDL_ID,
                                             MMDL_GEN_ONPOWERUP_STATUS_OPCODE);
  uint8_t msgParams[MMDL_GEN_POWER_ONOFF_MSG_LEN];
  mmdlGenPowOnOffSrDesc_t *pDesc = NULL;

  /* Fill in the msg info parameters */
  pubMsgInfo.elementId = elementId;

  /* Get the model instance descriptor */
  mmdlGenPowOnOffSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Copy the message parameters from the descriptor */
    msgParams[0] = pDesc->pStoredStates[PRESENT_STATE_IDX];

    /* Send message to the Mesh Core */
    MeshPublishMessage(&pubMsgInfo, msgParams, MMDL_GEN_POWER_ONOFF_MSG_LEN);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Get the Generic OnPowerUp state of the element.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffSrGetState(meshElementId_t elementId)
{
  mmdlGenPowOnOffSrCurrentState_t event;
  mmdlGenPowOnOffSrDesc_t *pDesc = NULL;

  /* Get model instance descriptor */
  mmdlGenPowOnOffSrGetDesc(elementId, &pDesc);

  /* Set event type */
  event.hdr.event = MMDL_GEN_POWER_ONOFF_SR_EVENT;
  event.hdr.param = MMDL_GEN_POWER_ONOFF_SR_CURRENT_STATE_EVENT;

  /* Set event parameters */
  event.elemId = elementId;

  if (pDesc == NULL)
  {
    /* No descriptor found on element */
    event.hdr.status = MMDL_INVALID_ELEMENT;

    /* Zero out parameters */
    event.state = 0;
  }
  else
  {
    /* Descriptor found on element */
    event.hdr.status = MMDL_SUCCESS;

    /* Set event parameters */
    event.state = pDesc->pStoredStates[PRESENT_STATE_IDX];
  }

  /* Send event to the upper layer */
  powOnOffSrCb.recvCback((wsfMsgHdr_t *)&event);
}

/*************************************************************************************************/
/*!
 *  \brief     Set the Generic OnPowerUp state of the element.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] targetState  Target State for this transaction. See ::mmdlGenOnPowerUpState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffSrSetState(meshElementId_t elementId, mmdlGenOnPowerUpState_t targetState)
{
  /* Change state locally. */
  MmdlGenPowOnOffOnPowerUpSrSetState(elementId, targetState, MMDL_STATE_UPDATED_BY_APP);
}

/*************************************************************************************************/
/*!
 *  \brief     Local setter of the generic state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model
 *  \param[in] newState        Newly set state.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffOnPowerUpSrSetState(meshElementId_t elementId, mmdlGenOnPowerUpState_t newState,
                                        mmdlStateUpdateSrc_t stateUpdateSrc)
{
  mmdlGenPowOnOffSrStateUpdate_t event;
  mmdlGenPowOnOffSrDesc_t *pDesc = NULL;

  /* Get the model instance descriptor */
  mmdlGenPowOnOffSrGetDesc(elementId, &pDesc);

  /* Set event parameters */
  event.elemId = elementId;
  event.stateUpdateSource = stateUpdateSrc;
  event.state = newState;
  event.hdr.event = MMDL_GEN_POWER_ONOFF_SR_EVENT;
  event.hdr.param = MMDL_GEN_POWER_ONOFF_SR_STATE_UPDATE_EVENT;

  if (pDesc != NULL)
  {
    event.hdr.status = MMDL_SUCCESS;

    /* Copy the message parameters from the descriptor */
    pDesc->pStoredStates[PRESENT_STATE_IDX] = newState;

    /* Update Generic OnPowerUp state entry in NVM. */
    if(pDesc->fNvmSaveStates)
    {
      pDesc->fNvmSaveStates(elementId);
    }
  }
  else
  {
    event.hdr.status = MMDL_INVALID_ELEMENT;
  }

  MMDL_TRACE_INFO1("GEN POWER ONOFF SR: Set=0x%X", newState);

  /* Send event to the upper layer */
  powOnOffSrCb.recvCback((wsfMsgHdr_t *)&event);
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
void MmdlGenPowOnOffSrRegister(mmdlEventCback_t recvCback)
{
  /* Store valid callback */
  if (recvCback != NULL)
  {
    powOnOffSrCb.recvCback = recvCback;
  }
}
