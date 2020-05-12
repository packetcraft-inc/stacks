/*************************************************************************************************/
/*!
*  \file   mmdl_gen_power_level_setup_sr_main.c
*
*  \brief  Implementation of the Generic Power Level Setup Server model.
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
#include "wsf_assert.h"
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mesh_api.h"

#include "mmdl_types.h"
#include "mmdl_defs.h"
#include "mmdl_common.h"
#include "mmdl_bindings.h"

#include "mmdl_gen_powerlevel_sr.h"
#include "mmdl_gen_powerlevelsetup_sr_main.h"
#include "mmdl_gen_powerlevelsetup_sr_api.h"
#include "mmdl_gen_powerlevel_sr_api.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Present state index in stored states */
#define PRESENT_STATE_IDX                    0

/*! Target state index in stored states */
#define TARGET_STATE_IDX                     1

/*! Present state index in stored states */
#define PRESENT_STATE_IDX                    0

/*! Target state index in stored states */
#define TARGET_STATE_IDX                     1

/*! Last state index in stored states */
#define LAST_STATE_IDX                       2

/*! Target state index in stored states */
#define DEFAULT_STATE_IDX                    3

/*! Target state index in stored states */
#define MIN_RANGE_STATE_IDX                  4

/*! Target state index in stored states */
#define MAX_RANGE_STATE_IDX                  5

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Generic Power Level Setup Server control block type definition */
typedef struct mmdlGenPowerLevelSetupSrCb_tag
{
  mmdlEventCback_t               recvCback;        /*!< Model Generic Level received callback */
} mmdlGenPowerLevelSetupSrCb_t;

/*! Generic Level Server message handler type definition */
typedef void(*mmdlGenPowerLevelSetupSrHandleMsg_t)(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler ID */
wsfHandlerId_t mmdlGenPowerLevelSetupSrHandlerId;

/*! Supported opcodes */
const meshMsgOpcode_t mmdlGenPowerLevelSetupSrRcvdOpcodes[MMDL_GEN_POWER_LEVELSETUP_SR_NUM_RCVD_OPCODES] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_POWERDEFAULT_SET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_POWERDEFAULT_SET_NO_ACK_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_POWERRANGE_SET_OPCODE)} },
  { {UINT16_OPCODE_TO_BYTES(MMDL_GEN_POWERRANGE_SET_NO_ACK_OPCODE)} },
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlGenPowerLevelSetupSrHandleMsg_t mmdlGenPowerLevelSetupSrHandleMsg[MMDL_GEN_POWER_LEVELSETUP_SR_NUM_RCVD_OPCODES] =
{
  mmdlGenPowerDefaultSrHandleSet,
  mmdlGenPowerDefaultSrHandleSetNoAck,
  mmdlGenPowerRangeSrHandleSet,
  mmdlGenPowerRangeSrHandleSetNoAck,
};

/*! Generic Power Level Server Control Block */
/* For future enhancement */
/* static mmdlGenPowerLevelSetupSrCb_t  powerLevelSetupSrCb; */

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Searches for the Generic Power Level Setup model instance descriptor on the
 *              specified element.
 *
 *  \param[in]  elementId  Identifier of the Element implementing the model.
 *  \param[out] ppOutDesc  Double pointer to the descriptor.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void mmdlGenPowerLevelSrGetDesc(meshElementId_t elementId, mmdlGenPowerLevelSrDesc_t **ppOutDesc)
{
  uint8_t modelIdx;

  *ppOutDesc = NULL;

  /* Check if element exists. */
  if (elementId >= pMeshConfig->elementArrayLen)
  {
    return;
  }

  /* Look for the model instance */
  for (modelIdx = 0; modelIdx < pMeshConfig->pElementArray[elementId].numSigModels; modelIdx++)
  {
    if (pMeshConfig->pElementArray[elementId].pSigModelArray[modelIdx].modelId ==
        MMDL_GEN_POWER_LEVEL_SR_MDL_ID)
    {
      /* Matching model ID on elementId */
      *ppOutDesc = pMeshConfig->pElementArray[elementId].pSigModelArray[modelIdx].pModelDescriptor;
      break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Generic Power Default Status command to the specified destination address.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] dstAddr      Element address of the destination
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenPowerDefaultSrSendStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                            uint16_t appKeyIndex, bool_t recvOnUnicast)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_POWER_LEVEL_SR_MDL_ID, MMDL_GEN_POWERDEFAULT_STATUS_OPCODE);
  mmdlGenPowerLevelSrDesc_t *pDesc = NULL;
  uint8_t msgParams[MMDL_GEN_POWERDEFAULT_STATUS_LEN];
  uint8_t *pParams;

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = dstAddr;
  msgInfo.ttl = MESH_USE_DEFAULT_TTL;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Get the model instance descriptor */
  mmdlGenPowerLevelSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    pParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT16_TO_BSTREAM(pParams, pDesc->pStoredStates[DEFAULT_STATE_IDX]);

    /* Send message to the Mesh Core instantly */
    MeshSendMessage(&msgInfo, msgParams, MMDL_GEN_POWERDEFAULT_STATUS_LEN,
                    MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                    MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Power Default Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    TRUE if handled successful, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlGenPowerDefaultSrProcessSet(const meshModelMsgRecvEvt_t *pMsg, bool_t ackRequired)
{
  mmdlGenPowerLevelState_t state;
  mmdlGenPowerLevelSrDesc_t *pDesc = NULL;

  (void)ackRequired;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length. It can take only min and max values. */
  if (pMsg->messageParamsLen != (MMDL_GEN_POWERDEFAULT_SET_LEN))
  {
    return FALSE;
  }

  /* Get the model instance descriptor */
  mmdlGenPowerLevelSrGetDesc(pMsg->elementId, &pDesc);

  if (pDesc != NULL)
  {
    /* Set the state value from pMessageParams buffer. */
    BYTES_TO_UINT16(state, &pMsg->pMessageParams[0]);

    /* Update Default State */
    pDesc->pStoredStates[DEFAULT_STATE_IDX] = state;
  }
  else
  {
    /* No descriptor found on element */
    return FALSE;
  }

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Sends a Generic Power Range Status command to the specified destination address.
 *
 *  \param[in] elementId      Identifier of the Element implementing the model.
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  Status is sent in response to a unicast.
 *  \param[in] opStatus       Operation status. See::mmdlGenPowerRangeStatus.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void mmdlGenPowerRangeSrSendStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                          uint16_t appKeyIndex, bool_t recvOnUnicast,
                                          mmdlGenPowerRangeStatus_t opStatus)
{
  meshMsgInfo_t msgInfo = MESH_MSG_INFO(MMDL_GEN_POWER_LEVEL_SR_MDL_ID, MMDL_GEN_POWERRANGE_STATUS_OPCODE);
  uint8_t msgParams[MMDL_GEN_POWERRANGE_STATUS_LEN];
  mmdlGenPowerLevelSrDesc_t *pDesc = NULL;
  uint8_t *pMsgParams;

  /* Fill in the msg info parameters */
  msgInfo.elementId = elementId;
  msgInfo.dstAddr = dstAddr;
  msgInfo.ttl = MESH_USE_DEFAULT_TTL;
  msgInfo.appKeyIndex = appKeyIndex;

  /* Get the model instance descriptor */
  mmdlGenPowerLevelSrGetDesc(elementId, &pDesc);

  if (pDesc != NULL)
  {
    pMsgParams = msgParams;

    /* Copy the message parameters from the descriptor */
    UINT8_TO_BSTREAM(pMsgParams, opStatus);
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredStates[MIN_RANGE_STATE_IDX]);
    UINT16_TO_BSTREAM(pMsgParams, pDesc->pStoredStates[MAX_RANGE_STATE_IDX]);

    MMDL_TRACE_INFO3("GEN POWER RANGE SR: Send Status=%d MinPower=0x%X, MaxPower=0x%X",
                      opStatus, pDesc->pStoredStates[MIN_RANGE_STATE_IDX],
                      pDesc->pStoredStates[MAX_RANGE_STATE_IDX]);

    /* Send message to the Mesh Core instantly */
    MeshSendMessage(&msgInfo, msgParams, MMDL_GEN_POWERRANGE_STATUS_LEN,
                    MMDL_STATUS_RSP_MIN_SEND_DELAY_MS,
                    MMDL_STATUS_RSP_MAX_SEND_DELAY_MS(recvOnUnicast));
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Power Range Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    Operation result. See ::mmdlGenPowerRangeStatus
 */
/*************************************************************************************************/
static mmdlGenPowerRangeStatus_t mmdlGenPowerRangeSrProcessSet(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlGenPowerRangeState_t state;
  uint8_t *pParams;
  mmdlGenPowerLevelSrDesc_t *pDesc = NULL;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length. It can take only min and max values. */
  if (pMsg->messageParamsLen != MMDL_GEN_POWERRANGE_SET_LEN)
  {
    return MMDL_RANGE_PROHIBITED;
  }

  /* Get the model instance descriptor */
  mmdlGenPowerLevelSrGetDesc(pMsg->elementId, &pDesc);

  if (pDesc != NULL)
  {
    pParams = pMsg->pMessageParams;

    /* Set the state value from pMessageParams buffer. */
    BSTREAM_TO_UINT16(state.rangeMin, pParams);
    BSTREAM_TO_UINT16(state.rangeMax, pParams);

    if (state.rangeMin > state.rangeMax)
    {
      return MMDL_RANGE_PROHIBITED;
    }

    /* Validate the range values. */
    if (state.rangeMin == 0)
    {
      return MMDL_RANGE_CANNOT_SET_MIN;
    }

    if (state.rangeMax == 0)
    {
      return MMDL_RANGE_CANNOT_SET_MAX;
    }

    /* Change state */
    MmdlGenPowerRangeSrSetState(pMsg->elementId, state.rangeMin, state.rangeMax);
  }
  else
  {
    /* No descriptor found on element */
    return MMDL_RANGE_PROHIBITED;
  }

  return MMDL_RANGE_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Power Default Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenPowerDefaultSrHandleSet(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  if (mmdlGenPowerDefaultSrProcessSet(pMsg, TRUE))
  {
    /* Send Status message as a response to the Set message */
    mmdlGenPowerDefaultSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex, pMsg->recvOnUnicast);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Power Default Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenPowerDefaultSrHandleSetNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  (void)mmdlGenPowerDefaultSrProcessSet(pMsg, FALSE);
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Power Range Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenPowerRangeSrHandleSet(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlGenPowerRangeStatus_t status;
  /* Change state */
  if ((status = mmdlGenPowerRangeSrProcessSet(pMsg)) != MMDL_RANGE_PROHIBITED)
  {
    /* Send Status message as a response to the Set message */
    mmdlGenPowerRangeSrSendStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                                  pMsg->recvOnUnicast, status);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Power Range Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenPowerRangeSrHandleSetNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  /* Change state */
  (void)mmdlGenPowerRangeSrProcessSet(pMsg);
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Generic Power Level Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSetupSrInit(void)
{
  MMDL_TRACE_INFO0("GEN POWER LEVEL SETUP SR: init");

  /* Set event callbacks */
  /* For future enhancement. */
  /* powerLevelSetupSrCb.recvCback = MmdlEmptyCback; */
}

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Generic Power Level Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Generic Power Level Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSetupSrHandlerInit(wsfHandlerId_t handlerId)
{
  /* Set handler ID */
  mmdlGenPowerLevelSetupSrHandlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Generic Power Level Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSetupSrHandler(wsfMsgHdr_t *pMsg)
{
  meshModelMsgRecvEvt_t *pModelMsg;
  uint8_t opcodeIdx;

  /* Handle message */
  if ((pMsg != NULL) && (pMsg->event == MESH_MODEL_EVT_MSG_RECV))
  {
    pModelMsg = (meshModelMsgRecvEvt_t *)pMsg;

    /* Validate opcode size and value */
    if (MESH_OPCODE_SIZE(pModelMsg->opCode) == MMDL_GEN_POWER_LEVEL_OPCODES_SIZE)
    {
      /* Match the received opcode */
      for (opcodeIdx = 0; opcodeIdx < MMDL_GEN_POWER_LEVELSETUP_SR_NUM_RCVD_OPCODES; opcodeIdx++)
      {
        if (!memcmp(&mmdlGenPowerLevelSetupSrRcvdOpcodes[opcodeIdx], pModelMsg->opCode.opcodeBytes,
                    MMDL_GEN_POWER_LEVEL_OPCODES_SIZE))
        {
          /* Process message */
          (void)mmdlGenPowerLevelSetupSrHandleMsg[opcodeIdx](pModelMsg);
        }
      }
    }
  }
  else
  {
    MMDL_TRACE_WARN0("GEN POWER SETUP LEVEL SR: Invalid event message received!");
  }
}
