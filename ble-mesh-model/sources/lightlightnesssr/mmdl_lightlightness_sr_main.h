/*************************************************************************************************/
/*!
 *  \file   mmdl_lightlightness_sr_main.h
 *
 *  \brief  Internal interface of the Light Lightness Server model.
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

#ifndef MMDL_LIGHT_LIGHTNESS_SR_MAIN_H
#define MMDL_LIGHT_LIGHTNESS_SR_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*!  Timeout for filtering duplicate messages from same source */
#define MSG_RCVD_TIMEOUT_MS                 6000

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessSrHandleGet(const meshModelMsgRecvEvt_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessSrHandleSet(const meshModelMsgRecvEvt_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessSrHandleSetNoAck(const meshModelMsgRecvEvt_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Linear Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessLinearSrHandleGet(const meshModelMsgRecvEvt_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Linear Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessLinearSrHandleSet(const meshModelMsgRecvEvt_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Linear Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessLinearSrHandleSetNoAck(const meshModelMsgRecvEvt_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Last Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessLastSrHandleGet(const meshModelMsgRecvEvt_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Default Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessDefaultSrHandleGet(const meshModelMsgRecvEvt_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Default Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessDefaultSrHandleSet(const meshModelMsgRecvEvt_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Default Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessDefaultSrHandleSetNoAck(const meshModelMsgRecvEvt_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Range Get command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessRangeSrHandleGet(const meshModelMsgRecvEvt_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Range Set command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessRangeSrHandleSet(const meshModelMsgRecvEvt_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Handles a Light Lightness Range Set Unacknowledged command.
 *
 *  \param[in] pMsg  Received model message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessRangeSrHandleSetNoAck(const meshModelMsgRecvEvt_t *pMsg);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_LIGHT_LIGHTNESS_SR_MAIN_H */
