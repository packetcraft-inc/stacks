/*************************************************************************************************/
/*!
 *  \file   mmdl_gen_level_sr.h
 *
 *  \brief  Interface of the Generic Level Server model.
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

#ifndef MMDL_GEN_LEVEL_SR_H
#define MMDL_GEN_LEVEL_SR_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Set the local state as a result of a binding. The set is instantaneous.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] newState   New State. See ::mmdlGenLevelState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenLevelSrSetBoundState(meshElementId_t elementId, mmdlGenLevelState_t newState);

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
void MmdlGenLevelSrStoreScene(void *pDesc, uint8_t sceneIdx);

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
void MmdlGenLevelSrRecallScene(meshElementId_t elementId, uint8_t sceneIdx,
                               uint32_t transitionMs);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_GEN_LEVEL_SR_H */
