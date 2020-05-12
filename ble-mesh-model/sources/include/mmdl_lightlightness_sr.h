/*************************************************************************************************/
/*!
 *  \file   mmdl_lightlightness_sr.h
 *
 *  \brief  Internal interface of the Light Lightness Server model for the Setup Server.
 *
 *  Copyright (c) 2010-2019 Arm Ltd. All Rights Reserved.
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

#ifndef MMDL_LIGHT_LIGHTNESS_SR_H
#define MMDL_LIGHT_LIGHTNESS_SR_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light Lightness Default Status command to the specified destination address.
 *
 *  \param[in] modelId        Model identifier.
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  Indicates if initial destination address was unicast.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessDefaultSrSendStatus(uint16_t modelId, meshElementId_t elementId,
                                           meshAddress_t dstAddr, uint16_t appKeyIndex,
                                           bool_t recvOnUnicast);

/*************************************************************************************************/
/*!
 *  \brief     Sends a Light Lightness Range Status command to the specified destination address.
 *
 *  \param[in] modelId        Model identifier.
 *  \param[in] elementId      Identifier of the Element implementing the model
 *  \param[in] dstAddr        Element address of the destination
 *  \param[in] appKeyIndex    Application Key Index.
 *  \param[in] recvOnUnicast  Indicates if initial destination address was unicast.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessRangeSrSendStatus(uint16_t modelId, meshElementId_t elementId,
                                         meshAddress_t dstAddr, uint16_t appKeyIndex,
                                         bool_t recvOnUnicast);

/*************************************************************************************************/
/*!
 *  \brief     Local setter of the Light Lightness Default state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model
 *  \param[in] targetState     New Light Lightness Default state.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void mmdlLightLightnessDefaultSrSetState(meshElementId_t elementId,
                                         mmdlLightLightnessState_t targetState,
                                         mmdlStateUpdateSrc_t stateUpdateSrc);

/*************************************************************************************************/
/*!
 *  \brief     Local getter of the Light Lightness Default state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    Lightness state. See::mmdlLightLightnessState_t.
 */
/*************************************************************************************************/
mmdlLightLightnessState_t mmdlLightLightnessDefaultSrGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Local getter of the Light Lightness Actual state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    Lightness state. See::mmdlLightLightnessState_t.
 */
/*************************************************************************************************/
mmdlLightLightnessState_t mmdlLightLightnessActualSrGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Local getter of the Light Lightness Last state.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model
 *
 *  \return    Lightness state. See::mmdlLightLightnessState_t.
 */
/*************************************************************************************************/
mmdlLightLightnessState_t mmdlLightLightnessLastSrGetState(meshElementId_t elementId);

/*************************************************************************************************/
/*!
 *  \brief     Local setter of the Light Lightness Range state.
 *
 *  \param[in] elementId       Identifier of the Element implementing the model
 *  \param[in] pRangeState     New Light Lightness Range state.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    TRUE if handled successful, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t mmdlLightLightnessRangeSrSetState(meshElementId_t elementId,
                                   const mmdlLightLightnessRangeState_t *pRangeState,
                                         mmdlStateUpdateSrc_t stateUpdateSrc);

/*************************************************************************************************/
/*!
 *  \brief     Set the local state as a result of a Generic Level binding. The set is instantaneous
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] state      New State. See ::mmdlLightLightnessState_t
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrSetBoundState(meshElementId_t elementId, mmdlLightLightnessState_t state);

/*************************************************************************************************/
/*!
 *  \brief     Gets the local states that can be stored in a scene.
 *
 *  \param[in] pDesc    Model instance descriptor.
 *  \param[in] sceneIdx Identifier of the stored scene.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrStoreScene(void *pDesc, uint8_t sceneIdx);

/*************************************************************************************************/
/*!
 *  \brief     Sets the local states values according to the previously stored scene.
 *
 *  \param[in] elementId     Identifier of the element implementing the model instance.
 *  \param[in] sceneIdx      Identifier of the recalled scene.
 *  \param[in] transitionMs  Transition time in ms.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightLightnessSrRecallScene(meshElementId_t elementId, uint8_t sceneIdx,
                                     uint32_t transitionMs);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_LIGHT_LIGHTNESS_SR_H */
