/*************************************************************************************************/
/*!
 *  \file   mmdl_gen_powonoff_sr.h
 *
 *  \brief  Internal interface of the Generic Power OnOff Server model.
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

#ifndef MMDL_GEN_POWER_ONOFF_SR_H
#define MMDL_GEN_POWER_ONOFF_SR_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief     Sends a Generic Power OnOff Status command to the specified destination address.
 *
 *  \param[in] modelId      Model identifier.
 *  \param[in] elementId    Identifier of the Element implementing the model
 *  \param[in] dstAddr      Element address of the destination
 *  \param[in] appKeyIndex  Application Key Index.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffSrSendStatus(uint16_t modelId, meshElementId_t elementId,
                                 meshAddress_t dstAddr, uint16_t appKeyIndex);

/*************************************************************************************************/
/*!
 *  \brief     Local setter of the generic state.
 *
 *  \param[in] elemId          Identifier of the Element implementing the model
 *  \param[in] newState        Newly set state.
 *  \param[in] stateUpdateSrc  Source that triggered the update. See ::mmdlStateUpdateSrcValues.
 *
 *  \return    None.
 */
/*************************************************************************************************/
void MmdlGenPowOnOffOnPowerUpSrSetState(meshElementId_t elemId, mmdlGenOnPowerUpState_t newState,
                                        mmdlStateUpdateSrc_t stateUpdateSrc);

#ifdef __cplusplus
}
#endif

#endif /* MMDL_GEN_POWER_ONOFF_SR_H */
