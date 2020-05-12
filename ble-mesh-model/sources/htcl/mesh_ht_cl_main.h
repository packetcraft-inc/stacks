/*************************************************************************************************/
/*!
 *  \file   mesh_ht_cl_main.h
 *
 *  \brief  Internal interface of the Health Client model.
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

#ifndef MESH_HT_CL_MAIN_H
#define MESH_HT_CL_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Health Client message handler type definition */
typedef void (*meshHtClHandleMsg_t )(const meshModelMsgRecvEvt_t *pMsg);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

void meshHtClHandleCurrentFaultStatus(const meshModelMsgRecvEvt_t *pMsg);
void meshHtClHandleFaultStatus(const meshModelMsgRecvEvt_t *pMsg);
void meshHtClHandlePeriodStatus(const meshModelMsgRecvEvt_t *pMsg);
void meshHtClHandleAttentionStatus(const meshModelMsgRecvEvt_t *pMsg);

#ifdef __cplusplus
}
#endif

#endif /* MESH_HT_SR_MAIN_H */
