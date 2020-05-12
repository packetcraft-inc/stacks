/*************************************************************************************************/
/*!
 *  \file   mmdl_light_hsl_sr.h
 *
 *  \brief  Interface of the Light HSL Server model.
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

#ifndef MMDL_LIGHT_HSL_SR_H
#define MMDL_LIGHT_HSL_SR_H

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
void MmdlLightHslSrSetBoundLtLtness(meshElementId_t elementId, uint16_t ltness);

/*************************************************************************************************/
/*!
 *  \brief     Set the local state as a result of a binding with a Light HSL Hue state.
 *             The set is instantaneous.
 *
 *  \param[in] elementId   Identifier of the Element implementing the model.
 *  \param[in] presentHue  Present Hue value.
 *  \param[in] targetHue   Target Hue value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrSetBoundHue(meshElementId_t elementId, uint16_t presentHue, uint16_t targetHue);

/*************************************************************************************************/
/*!
 *  \brief     Set the local state as a result of a binding with a Light HSL Saturation state.
 *             The set is instantaneous.
 *
 *  \param[in] elementId   Identifier of the Element implementing the model.
 *  \param[in] presentSat  Present saturation value.
 *  \param[in] targetSat   Target saturation value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrSetBoundSaturation(meshElementId_t elementId, uint16_t presentSat,
                                      uint16_t targetSat);

/*************************************************************************************************/
/*!
 *  \brief     Searches for the Light HSL Server model instance descriptor on the specified element.
 *
 *  \param[in]  elementId  Identifier of the Element implementing the model.
 *  \param[out] ppOutDesc  Double pointer to the descriptor.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrGetDesc(meshElementId_t elementId, mmdlLightHslSrDesc_t **ppOutDesc);

/*************************************************************************************************/
/*!
 *  \brief     Local getter of the Light HSL Hue Default state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    Default Hue value.
 */
/*************************************************************************************************/
uint16_t MmdlLightHslSrGetDefaultHue(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Local getter of the Light HSL Saturation Default state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    Default Saturation value.
 */
/*************************************************************************************************/
uint16_t MmdlLightHslSrGetDefaultSaturation(meshElementId_t elementId);

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
void MmdlLightHslSrStoreScene(void *pDesc, uint8_t sceneIdx);

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
void MmdlLightHslSrRecallScene(meshElementId_t elementId, uint8_t sceneIdx,
                               uint32_t transitionMs);

/*************************************************************************************************/
/*!
 *  \brief     Set the local state as a result of an OnOff binding. The set is instantaneous.
 *
 *  \param[in] elementId   Identifier of the Element implementing the model.
 *  \param[in] onOffState  New State. See ::mmdlGenOnOffStates
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSrSetBoundStateOnOff(meshElementId_t elementId, mmdlGenOnOffState_t onOffState);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_LIGHT_HSL_SR_H */
