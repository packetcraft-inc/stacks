/*************************************************************************************************/
/*!
 *  \file   mmdl_scene_sr_main.h
 *
 *  \brief  Internal interface of the Scene Server model.
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

#ifndef MMDL_SCENE_SR_MAIN_H
#define MMDL_SCENE_SR_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Timeout for filtering duplicate messages from same source */
#define MSG_RCVD_TIMEOUT_MS                 6000

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

void mmdlSceneSrHandleGet(const meshModelMsgRecvEvt_t *pMsg);

void mmdlSceneSrHandleRegisterGet(const meshModelMsgRecvEvt_t *pMsg);

void mmdlSceneSrHandleRecallNoAck(const meshModelMsgRecvEvt_t *pMsg);

void mmdlSceneSrHandleRecall(const meshModelMsgRecvEvt_t *pMsg);

void mmdlSceneSrGetDesc(meshElementId_t elementId, mmdlSceneSrDesc_t **ppOutDesc);

void mmdlSceneSrSendRegisterStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                                   uint16_t appKeyIndex, bool_t recvOnUnicast,
                                   mmdlSceneStatus_t opStatus);

mmdlSceneStatus_t mmdlSceneSrStore(meshElementId_t elementId, mmdlSceneNumber_t sceneNum);

mmdlSceneStatus_t mmdlSceneSrDelete(mmdlSceneSrDesc_t *pDesc, mmdlSceneNumber_t sceneNum);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_SCENE_SR_MAIN_H */
