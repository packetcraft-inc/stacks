/*************************************************************************************************/
/*!
 *  \file   mmdl_gen_default_trans_sr_main.h
 *
 *  \brief  Internal interface of the Generic Default Transition Server model.
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

#ifndef MMDL_GEN_DEFAULT_TRANS_SR_MAIN_H
#define MMDL_GEN_DEFAULT_TRANS_SR_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Default Transition Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenDefaultTransSrHandleGet(const meshModelMsgRecvEvt_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Default Transition Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenDefaultTransSrHandleSetNoAck(const meshModelMsgRecvEvt_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Handles a Generic Default Transition Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlGenDefaultTransSrHandleSet(const meshModelMsgRecvEvt_t *pMsg);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_GEN_DEFAULT_TRANS_SR_MAIN_H */
