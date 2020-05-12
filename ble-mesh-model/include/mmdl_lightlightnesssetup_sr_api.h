/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Light Lightness Server Model API.
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
 */
/*************************************************************************************************/

/*! ***********************************************************************************************
 * \addtogroup ModelLightLightnessSr Light Lightness Server Model API
 * @{
 *************************************************************************************************/

#ifndef MMDL_LIGHT_LIGHTNESSSETUP_SR_API_H
#define MMDL_LIGHT_LIGHTNESSSETUP_SR_API_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/**************************************************************************************************
  Variables Declarations
**************************************************************************************************/

/*! \brief WSF handler id */
extern wsfHandlerId_t mmdlLightLightnessSetupSrHandlerId;

/*! \brief Supported opcodes */
extern const meshMsgOpcode_t mmdlLightLightnessSetupSrRcvdOpcodes[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initializes the Light Lightness Setup Server module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSetupSrInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Initializes the Light Lightness Setup Server WSF handler.
 *
 *  \param[in] handlerId  WSF handler ID for Light Lightness Server Model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSetupSrHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *  \brief     WSF message handler for Light Lightness Setup Server Model.
 *
 *  \param[in] pMsg  WSF message.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSetupSrHandler(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief     Set the local Light Lightness Default state.
 *
 *  \param[in] elementId    Identifier of the Element implementing the model.
 *  \param[in] targetState  Target State for this transaction. See ::mmdlLightLightnessState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessDefaultSetupSrSetState(meshElementId_t elementId,
                                              mmdlLightLightnessState_t targetState);

/*************************************************************************************************/
/*!
 *  \brief     Set the local Light Lightness Range state.
 *
 *  \param[in] elementId     Identifier of the Element implementing the model.
 *  \param[in] pTargetState  Target State for this transaction. See ::mmdlLightLightnessRangeState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessRangeSetupSrSetState(meshElementId_t elementId,
                                      const mmdlLightLightnessRangeState_t *pTargetState);

/*************************************************************************************************/
/*!
 *  \brief     Registers the callback that is triggered when a message is received for this model.
 *
 *  \param[in] recvCback  Callback installed by the upper layer to receive messages from the model.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSetupSrRegister(mmdlEventCback_t recvCback);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_LIGHT_LIGHTNESSSETUP_SR_API_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
