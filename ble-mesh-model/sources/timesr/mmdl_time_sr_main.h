/*************************************************************************************************/
/*!
 *  \file   mmdl_time_sr_main.h
 *
 *  \brief  Internal interface of the Time model.
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

#ifndef MMDL_TIME_SR_MAIN_H
#define MMDL_TIME_SR_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

void mmdlTimeSrHandleGet(const meshModelMsgRecvEvt_t *pMsg);

void mmdlTimeSrHandleStatus(const meshModelMsgRecvEvt_t *pMsg);

void mmdlTimeSrHandleZoneGet(const meshModelMsgRecvEvt_t *pMsg);

void mmdlTimeSrHandleDeltaGet(const meshModelMsgRecvEvt_t *pMsg);

void mmdlTimeSrGetDesc(meshElementId_t elementId, mmdlTimeSrDesc_t **ppOutDesc);

void mmdlTimeSrSendStatus(meshElementId_t elementId, meshAddress_t dstAddr,
                          uint16_t appKeyIndex, bool_t recvOnUnicast);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_GEN_LEVEL_SR_MAIN_H */
