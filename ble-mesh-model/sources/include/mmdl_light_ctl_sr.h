/*************************************************************************************************/
/*!
 *  \file   mmdl_light_ctl_sr.h
 *
 *  \brief  Interface of the Light CTL Server model.
 *
 *  Copyright (c) 2010-2018 Arm Ltd. All Rights Reserved.
 *
 *  Copyright (c) 2019-2020 Packetcraft, Inc.
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

#ifndef MMDL_LIGHT_CTL_SR_H
#define MMDL_LIGHT_CTL_SR_H

#include "mmdl_light_ctl_temp_sr_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Set the local state as a result of a binding with a Light Lightness Actual state.
 *             The set is instantaneous.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] ltness     Lightness value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrSetBoundLtLtness(meshElementId_t elementId, uint16_t ltness);

/*************************************************************************************************/
/*!
 *  \brief     Set the local state as a result of a binding with a Light CTL Temperature state.
 *             The set is instantaneous.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] pState     Pointer to the present Temperature state.
 *  \param[in] pState     Pointer to the target Temperature state.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrSetBoundTemp(meshElementId_t elementId, mmdlLightCtlTempSrState_t * pState,
                                mmdlLightCtlTempSrState_t * pTargetState);

/*************************************************************************************************/
/*!
 *  \brief     Searches for the Light CTL Server model instance descriptor on the specified element.
 *
 *  \param[in]  elementId  Identifier of the Element implementing the model.
 *  \param[out] ppOutDesc  Double pointer to the descriptor.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrGetDesc(meshElementId_t elementId, mmdlLightCtlSrDesc_t **ppOutDesc);

/*************************************************************************************************/
/*!
 *  \brief     Local getter of the Light CTL Temperature Default state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    Default Temperature value.
 */
/*************************************************************************************************/
uint16_t MmdlLightCtlSrGetDefaultTemp(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Stores the present state in a scene.
 *
 *  \param[in] pDesc    Model instance descriptor.
 *  \param[in] sceneIdx Identifier of the stored scene.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrStoreScene(void *pDesc, uint8_t sceneIdx);

/*************************************************************************************************/
/*!
 *  \brief     Sets the state according to the previously stored scene.
 *
 *  \param[in] elementId     Identifier of the element implementing the model instance.
 *  \param[in] sceneIdx      Identifier of the recalled scene.
 *  \param[in] transitionMs  Transition time in ms.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlSrRecallScene(meshElementId_t elementId, uint8_t sceneIdx, uint32_t transitionMs);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_LIGHT_CTL_SR_H */
