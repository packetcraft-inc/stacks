/*************************************************************************************************/
/*!
 *  \file   mmdl_scene_setup_sr.c
 *
 *  \brief  Implementation of the Scenes Setup Server model.
 *
 *  Copyright (c) 2010-2018 Arm Ltd. All Rights Reserved.
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
#include "wsf_buf.h"
#include "util/bstream.h"

#include "mesh_defs.h"
#include "mesh_types.h"
#include "mmdl_common.h"
#include "mesh_api.h"

#include "mmdl_types.h"
#include "mmdl_defs.h"
#include "mmdl_bindings.h"
#include "mmdl_gen_default_trans_sr.h"
#include "mmdl_scene_sr_api.h"
#include "mmdl_scene_sr_main.h"
#include "mmdl_scene_setup_sr.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Scenes Setup Server message handler type definition */
typedef void (*mmdlSceneSetupSrHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Supported opcodes */
const meshMsgOpcode_t mmdlSceneSetupSrRcvdOpcodes[MMDL_SCENE_SETUP_SR_NUM_RCVD_OPCODES] =
{
  { {UINT16_OPCODE_TO_BYTES(MMDL_SCENE_STORE_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_SCENE_STORE_NO_ACK_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_SCENE_DELETE_OPCODE) }},
  { {UINT16_OPCODE_TO_BYTES(MMDL_SCENE_DELETE_NO_ACK_OPCODE) }}
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Handler functions for supported opcodes */
const mmdlSceneSetupSrHandleMsg_t mmdlSceneSetupSrHandleMsg[MMDL_SCENE_SETUP_SR_NUM_RCVD_OPCODES] =
{
  mmdlSceneSetupSrHandleStore,
  mmdlSceneSetupSrHandleStoreNoAck,
  mmdlSceneSetupSrHandleDelete,
  mmdlSceneSetupSrHandleDeleteNoAck,
};

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Processes Scene Store commands.
 *
 *  \param[in]  pMsg          Received model message.
 *  \param[in]  ackRequired   TRUE if acknowledgement is required in response,  FALSE otherwise.
 *  \param[out] pOutOpStatus  Operation status. See ::mmdlSceneStatus.
 *
 *  \return     TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlSceneSetupSrProcessStore(const meshModelMsgRecvEvt_t *pMsg, bool_t ackRequired,
                                           mmdlSceneStatus_t *pOutOpStatus)
{
  (void)ackRequired;

  mmdlSceneNumber_t sceneNum;

  /* Set default value */
  *pOutOpStatus = MMDL_SCENE_PROHIBITED;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_SCENE_STORE_LEN)
  {
    return FALSE;
  }

  /* Extract scene number */
  BYTES_TO_UINT16(sceneNum, pMsg->pMessageParams);

  /* Check prohibited values for Scene Number */
  if (sceneNum == MMDL_SCENE_NUM_PROHIBITED)
  {
    return FALSE;
  }

  /* Store scene number */
  *pOutOpStatus = mmdlSceneSrStore(pMsg->elementId, sceneNum);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Scene Store command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlSceneSetupSrHandleStore(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlSceneStatus_t opStatus = MMDL_SCENE_PROHIBITED;

  /* Change state */
  if (mmdlSceneSetupSrProcessStore(pMsg, TRUE, &opStatus) && (opStatus!= MMDL_SCENE_PROHIBITED))
  {
    /* Send Status message as a response to the Store message */
    mmdlSceneSrSendRegisterStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                                  pMsg->recvOnUnicast, opStatus);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Scene Store Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlSceneSetupSrHandleStoreNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlSceneStatus_t opStatus = MMDL_SCENE_PROHIBITED;

  (void)mmdlSceneSetupSrProcessStore(pMsg, FALSE, &opStatus);
}

/*************************************************************************************************/
/*!
 *  \brief      Processes Scene Delete commands.
 *
 *  \param[in]  pMsg          Received model message.
 *  \param[in]  ackRequired   TRUE if acknowledgement is required in response,  FALSE otherwise.
 *  \param[out] pOutOpStatus  Operation status. See ::mmdlSceneStatus.
 *
 *  \return     TRUE if handled successful and response is needed, FALSE otherwise.
 */
/*************************************************************************************************/
static bool_t mmdlSceneSetupSrProcessDelete(const meshModelMsgRecvEvt_t *pMsg, bool_t ackRequired,
                                           mmdlSceneStatus_t *pOutOpStatus)
{
  (void)ackRequired;

  mmdlSceneNumber_t sceneNum;
  mmdlSceneSrDesc_t *pDesc;

  /* Set default value */
  *pOutOpStatus = MMDL_SCENE_PROHIBITED;

  WSF_ASSERT(pMsg != NULL);
  WSF_ASSERT(pMsg->pMessageParams != NULL);

  /* Get scene descriptor */
  mmdlSceneSrGetDesc(pMsg->elementId, &pDesc);

  if (pDesc == NULL)
  {
    return FALSE;
  }

  /* Validate message length */
  if (pMsg->messageParamsLen != MMDL_SCENE_STORE_LEN)
  {
    return FALSE;
  }

  /* Extract scene number */
  BYTES_TO_UINT16(sceneNum, pMsg->pMessageParams);

  /* Check prohibited values for Scene Number */
  if (sceneNum == MMDL_SCENE_NUM_PROHIBITED)
  {
    return FALSE;
  }

  /* Get model instance descriptor */
  *pOutOpStatus = mmdlSceneSrDelete(pDesc, sceneNum);

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Scene Delete command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlSceneSetupSrHandleDelete(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlSceneStatus_t opStatus = MMDL_SCENE_PROHIBITED;

  /* Change state */
  if (mmdlSceneSetupSrProcessDelete(pMsg, TRUE, &opStatus) && (opStatus!= MMDL_SCENE_PROHIBITED))
  {
    /* Send Status message as a response to the Delete message */
    mmdlSceneSrSendRegisterStatus(pMsg->elementId, pMsg->srcAddr, pMsg->appKeyIndex,
                                  pMsg->recvOnUnicast, opStatus);
  }
}

/*************************************************************************************************/
/*!
 *  \brief     Handles a Scene Delete Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlSceneSetupSrHandleDeleteNoAck(const meshModelMsgRecvEvt_t *pMsg)
{
  mmdlSceneStatus_t opStatus = MMDL_SCENE_PROHIBITED;

  (void)mmdlSceneSetupSrProcessDelete(pMsg, FALSE, &opStatus);
}

/**************************************************************************************************
  Global Function
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Scenes Setup Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlSceneSetupSrHandler(wsfMsgHdr_t *pMsg)
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
        if (MESH_OPCODE_SIZE(pModelMsg->msgRecvEvt.opCode) == MMDL_SCENE_OPCODES_SIZE)
        {
          /* Match the received opcode */
          for (opcodeIdx = 0; opcodeIdx < MMDL_SCENE_SETUP_SR_NUM_RCVD_OPCODES; opcodeIdx++)
          {
            if (!memcmp(&mmdlSceneSetupSrRcvdOpcodes[opcodeIdx],
                        pModelMsg->msgRecvEvt.opCode.opcodeBytes, MMDL_SCENE_OPCODES_SIZE))
            {
              /* Process message */
              (void)mmdlSceneSetupSrHandleMsg[opcodeIdx]((meshModelMsgRecvEvt_t *)pModelMsg);
            }
          }
        }
        break;

      default:
        MMDL_TRACE_WARN0("SCENE SETUP SR: Invalid event message received!");
        break;
    }
  }
}
