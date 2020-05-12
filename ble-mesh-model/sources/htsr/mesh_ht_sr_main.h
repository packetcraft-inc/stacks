/*************************************************************************************************/
/*!
 *  \file   mesh_ht_sr_main.h
 *
 *  \brief  Internal interface of the Health Server model.
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

#ifndef MESH_HT_SR_MAIN_H
#define MESH_HT_SR_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Calculates publication time when Fast Period Divisor is not zero */
#define FAST_PUB_TIME(pDesc) (((pDesc)->pubPeriodMs) >> ((pDesc)->fastPeriodDiv))

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Health Server control block type definition */
typedef struct meshHtSrCb_tag
{
  mmdlEventCback_t          recvCback;   /*!< Health Server event callback */
} meshHtSrCb_t;

/*! Health Server message handler type definition */
typedef void (*meshHtSrHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! Generic On Off Server Control Block */
extern meshHtSrCb_t  htSrCb;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

void meshHtSrGetDesc(meshElementId_t elementId, meshHtSrDescriptor_t **ppOutDesc);

void meshHtSrHandleFaultGet(const meshModelMsgRecvEvt_t *pMsg);
void meshHtSrHandleFaultClearUnack(const meshModelMsgRecvEvt_t *pMsg);
void meshHtSrHandleFaultClear(const meshModelMsgRecvEvt_t *pMsg);
void meshHtSrHandleFaultTest(const meshModelMsgRecvEvt_t *pMsg);
void meshHtSrHandleFaultTestUnack(const meshModelMsgRecvEvt_t *pMsg);
void meshHtSrHandlePeriodGet(const meshModelMsgRecvEvt_t *pMsg);
void meshHtSrHandlePeriodSetUnack(const meshModelMsgRecvEvt_t *pMsg);
void meshHtSrHandlePeriodSet(const meshModelMsgRecvEvt_t *pMsg);
void meshHtSrHandleAttentionGet(const meshModelMsgRecvEvt_t *pMsg);
void meshHtSrHandleAttentionSet(const meshModelMsgRecvEvt_t *pMsg);
void meshHtSrHandleAttentionSetUnack(const meshModelMsgRecvEvt_t *pMsg);

void meshHtSrSendFaultStatus(uint16_t companyId, meshElementId_t elementId,
                             meshAddress_t dstAddr, uint16_t appKeyIndex, uint8_t recvTtl,
                             bool_t unicastRsp);
void meshHtSrSendPeriodStatus(meshHtPeriod_t period, meshElementId_t elementId,
                              meshAddress_t dstAddr, uint16_t appKeyIndex,
                              uint8_t recvTtl, bool_t unicastRsp);
void meshHtSrSendAttentionStatus(uint8_t attTimerSec, meshElementId_t elementId,
                                 meshAddress_t dstAddr, uint16_t appKeyIndex,
                                 uint8_t recvTtl, bool_t unicastRsp);
void meshHtSrPublishCrtHt(meshElementId_t elementId);

uint8_t htSrGetNumFaults(uint8_t *pFaultArray);

#ifdef __cplusplus
}
#endif

#endif /* MESH_HT_SR_MAIN_H */
