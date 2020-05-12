/*************************************************************************************************/
/*!
 *  \file   mmdl_light_hsl_sat_sr.h
 *
 *  \brief  Interface of the Light HSL Sat Server model.
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

#ifndef MMDL_LIGHT_HSL_SAT_SR_H
#define MMDL_LIGHT_HSL_SAT_SR_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Set the local saturation state. The set is instantaneous.
 *
 *  \param[in] elementId   Identifier of the Element implementing the model.
 *  \param[in] saturation  New saturation value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSatSrSetSaturation(meshElementId_t elementId, uint16_t saturation);

/*************************************************************************************************/
/*!
 *  \brief     Set the local saturation state. The set is instantaneous.
 *
 *  \param[in] elementId   Identifier of the Element implementing the model.
 *  \param[in] presentSat  New present saturation value.
 *  \param[in] targetSat   New target saturation value.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightHslSatSrSetBoundState(meshElementId_t elementId, uint16_t presentSat,
                                    uint16_t targetSat);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_LIGHT_HSL_SAT_SR_H */
