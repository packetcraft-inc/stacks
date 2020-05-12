/*************************************************************************************************/
/*!
 *  \file   mmdl_light_ctl_temp_sr.h
 *
 *  \brief  Interface of the Light CTL Temperature Server model.
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

#ifndef MMDL_LIGHT_CTL_TEMP_SR_H
#define MMDL_LIGHT_CTL_TEMP_SR_H

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
 *  \brief     Set the local state. The set is instantaneous.
 *
 *  \param[in] elementId  Identifier of the Element implementing the model.
 *  \param[in] pState     Pointer to the Temperature state
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlTemperatureSrSetTemperature(meshElementId_t elementId, mmdlLightCtlTempSrState_t * pState);

/*************************************************************************************************/
/*!
 *  \brief     Set the local state as a result of a binding with a Light CTL state.
 *             The set is instantaneous.
 *
 *  \param[in] elementId     Identifier of the Element implementing the model.
 *  \param[in] pState        Pointer to the present Temperature state.
 *  \param[in] pTargetState  Pointer to the target Temperature state.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlLightCtlTemperatureSrSetBoundState(meshElementId_t elementId,
                                            mmdlLightCtlTempSrState_t * pState,
                                            mmdlLightCtlTempSrState_t * pTargetState);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_LIGHT_CTL_TEMP_SR_H */
