/*************************************************************************************************/
/*!
 *  \file   mmdl_scheduler_sr_main.h
 *
 *  \brief  Internal interface of the Scheduler Server model.
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

#ifndef MMDL_SCHEDULER_SR_MAIN_H
#define MMDL_SCHEDULER_SR_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/
/*! Initializer of a message info for the specified model ID */
#define MSG_INFO(modelId) {{(meshSigModelId_t)modelId }, {{0,0,0}},\
                           0xFF, NULL, MESH_ADDR_TYPE_UNASSIGNED, 0xFF, 0xFF}

/*! Initializer of a publish message info for the specified model ID */
#define PUB_MSG_INFO(modelId) {{{0,0,0}}, 0xFF , {(meshSigModelId_t)modelId}}

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

void mmdlSchedulerSrHandleGet(const meshModelMsgRecvEvt_t *pMsg);

void mmdlSchedulerSrHandleActionGet(const meshModelMsgRecvEvt_t *pMsg);

void mmdlSchedulerSrGetDesc(meshElementId_t elementId, mmdlSchedulerSrDesc_t **ppOutDesc);

void mmdlSchedulerSrScheduleEvent(meshElementId_t elementId, uint8_t index,
                                  mmdlSchedulerSrRegisterEntry_t *pEntry);

void mmdlSchedulerSrSendActionStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                     uint16_t appKeyIndex, bool_t recvOnUnicast,
                                     uint8_t index);
void mmdlSchedulerUnpackActionParams(uint8_t *pMsgParams, uint8_t *pOutIndex,
                                     mmdlSchedulerRegisterEntry_t *pOutEntry);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_SCHEDULER_SR_MAIN_H */
