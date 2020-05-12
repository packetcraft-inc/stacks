/*************************************************************************************************/
/*!
*  \file   mmdl_gen_powerlevel_sr.h
*
*  \brief  Interface of the Generic Power Level Server model.
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

#ifndef MMDL_GEN_POWER_LEVEL_SR_H
#define MMDL_GEN_POWER_LEVEL_SR_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Set the Generic Power Range state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] rangeMin   Range Minimum. See ::mmdlGenPowerLevelState_t.
 *  \param[in] rangeMax   Range maximum. See ::mmdlGenPowerLevelState_t.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerRangeSrSetState(meshElementId_t elementId, mmdlGenPowerLevelState_t rangeMin,
                                 mmdlGenPowerLevelState_t rangeMax);

/*************************************************************************************************/
/*!
 *  \brief     Stores the local states that in a scene.
 *
 *  \param[in] pDesc    Model instance descriptor.
 *  \param[in] sceneIdx Identifier of the stored scene.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSrStoreScene(void *pDesc, uint8_t sceneIdx);

/*************************************************************************************************/
/*!
 *  \brief     Recalls the scene.
 *
 *  \param[in] elementId     Identifier of the element implementing the model instance.
 *  \param[in] sceneIdx      Identifier of the stored scene.
 *  \param[in] transitionMs  Transition time in ms.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowerLevelSrRecallScene(meshElementId_t elementId, uint8_t sceneIdx,
                                    uint32_t transitionMs);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_GEN_POWER_LEVEL_SR_H */
